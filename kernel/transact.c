/* transact.c 
 * 
 * A named semaphore with death notification. Has similar semantics to eventfd
 * in semaphore mode, but requires a character device file.
 *
 */

/* Standard headers for LKMs */
#include <linux/module.h>  
#include <linux/poll.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/cdev.h>
#include <linux/kref.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <asm/current.h>

#include "transact.h"

struct transact_ctx;
struct transact_cdev;

struct transact_proc {
	struct transact_ctx *ctx;
	int index;
	int initialized;
};

struct transact_ctx {
	struct transact_proc child[2];
	struct transact_cdev *dev;
	__u64 token;
	wait_queue_head_t wqh;
	struct list_head contexts;
	spinlock_t lock;
	unsigned long ino;
	struct kref kref;
	int current_child;
	int count;
	int uncontested;
	int parent_initialized;
	int child_initialized;
};

struct transact_cdev {
	dev_t devnum;
	struct cdev cdev;
	spinlock_t lock;
	struct list_head ctx;
};

static struct transact_cdev g_cdev;

static void transact_notify_death(struct transact_proc *p)
{
	struct transact_ctx *ctx = p->ctx;
	spin_lock_irq(&ctx->lock);
	if (ctx->uncontested) {
		// Other side already dead.
		spin_unlock_irq(&ctx->lock);
		return;
	}
	ctx->current_child = !p->index;
	ctx->uncontested = 1;
	spin_unlock_irq(&ctx->lock);

	spin_lock_irq(&ctx->wqh.lock);
	// Wake up other process, if waiting.
	if (waitqueue_active(&ctx->wqh))
		wake_up_locked_poll(&ctx->wqh, POLLHUP);
	spin_unlock_irq(&ctx->wqh.lock);
}

static int transact_switch_locked(struct transact_proc *p)
{
	struct transact_ctx *ctx = p->ctx;
	int res = 0;
	DECLARE_WAITQUEUE(wait, current);

	if (unlikely(ctx->uncontested)) {
		res = -EDEADLOCK;
		spin_unlock_irq(&ctx->lock);
		return res;
	}
	ctx->current_child = !p->index;
	spin_unlock_irq(&ctx->lock);

	spin_lock_irq(&ctx->wqh.lock);
	// Wake up other process, if waiting.
	if (waitqueue_active(&ctx->wqh))
		wake_up_locked_poll(&ctx->wqh, POLLIN);

	// And sleep until it is our turn.
	__add_wait_queue(&ctx->wqh, &wait);
	for (;;) {
		set_current_state(TASK_INTERRUPTIBLE);
		if (ctx->current_child == p->index || ctx->uncontested) {
			if (ctx->uncontested)
				res = -EDEADLOCK;
			break;
		}
		if (signal_pending(current)) {
			res = -ERESTARTSYS;
			break;
		}
		spin_unlock_irq(&ctx->wqh.lock);
		schedule();
		spin_lock_irq(&ctx->wqh.lock);
	}
	__remove_wait_queue(&ctx->wqh, &wait);
	__set_current_state(TASK_RUNNING);
	spin_unlock_irq(&ctx->wqh.lock);

	return res;
}

static struct transact_ctx* transact_get_ctx(struct transact_cdev *dev,
		unsigned long ino)
{
	struct transact_ctx* ctx = NULL;

	spin_lock_irq(&dev->lock);
	// Try to get the context from the list of previously created contexts.
	list_for_each_entry(ctx, &dev->ctx, contexts) {
		if (ctx->ino == ino) {
			// And increase the reference count in case another process tries to
			// destroy it.
			kref_get(&ctx->kref);
			spin_unlock_irq(&dev->lock);
			return ctx;
		}
	}
	// There is no previously created context. Allocate a new one.
	ctx = kmalloc(sizeof(struct transact_ctx), GFP_ATOMIC);
	if (ctx) {
		memset(ctx, 0, sizeof(struct transact_ctx));
		kref_init(&ctx->kref);
		ctx->current_child = -1;
		ctx->dev = dev;
		ctx->ino = ino;
		spin_lock_init(&ctx->lock);
		init_waitqueue_head(&ctx->wqh);
		list_add(&ctx->contexts, &dev->ctx);
	}
	spin_unlock_irq(&dev->lock);

	return ctx;
}

static void transact_release_ctx_locked(struct kref *ref)
{
	struct transact_ctx *ctx = container_of(ref, struct transact_ctx, kref);
	list_del(&ctx->contexts);
	kfree(ctx);
}

int transact_open(struct inode *inode, struct file *filp)
{
	struct transact_cdev *dev;
	struct transact_ctx *ctx;
	struct transact_proc *p;
	int current_child, res;

	dev = container_of(inode->i_cdev, struct transact_cdev, cdev);
	ctx = transact_get_ctx(dev, inode->i_ino);
	if (!ctx) {
		return -ENOMEM;
	}

	spin_lock_irq(&ctx->lock);
	if (ctx->count == 2) {
		spin_unlock_irq(&ctx->lock);

		spin_lock_irq(&dev->lock);
		kref_put(&ctx->kref, transact_release_ctx_locked);
		spin_unlock_irq(&dev->lock);

		return -EBUSY;
	}
	current_child = ctx->count++;
	spin_unlock_irq(&ctx->lock);

	p = &ctx->child[current_child];
	p->ctx = ctx;
	p->index = current_child;
	filp->private_data = p;

	// Always wait for the other process. This is done to avoid a situation where
	// the parent gets so far ahead of the child process, that it opens the
	// transact file and then is terminated before the child even had the chance
	// to open it itself. This causes the child to be stuck waiting for a peer
	// that will never arrive.
	spin_lock_irq(&p->ctx->lock);
	res = transact_switch_locked(p);  // releases p->ctx->lock.

	if (res != 0) {
		spin_lock_irq(&dev->lock);
		kref_put(&ctx->kref, transact_release_ctx_locked);
		spin_unlock_irq(&dev->lock);

		// If the other side is gone by now, return ENXIO so it is not confused
		// with EBUSY above.
		return res == -EDEADLK ? -ENXIO : res;
	}

	return 0;
}

ssize_t transact_read(struct file *filp, char __user *buf, size_t count,
		loff_t *ppos)
{
	struct transact_proc *p = (struct transact_proc *)filp->private_data;
	ssize_t res;
	__u64 data;

	if (count < sizeof(data))
		return -EINVAL;
	spin_lock_irq(&p->ctx->lock);
	res = transact_switch_locked(p);  // releases p->ctx->lock.
	data = p->ctx->token;
	if (unlikely(res == -EDEADLOCK)) {
		// Special case. EDEADLOCK means the other process has already closed the
		// file, so return 0 to make the caller think the file is closed.
		return 0;
	} else if (unlikely(res < 0)) {
		return res;
	}

	return put_user(data, (__u64 __user *)buf) ? -EFAULT : sizeof(data);
}

ssize_t transact_write(struct file *filp, const char __user *buf, size_t count,
		loff_t *ppos)
{
	struct transact_proc *p = (struct transact_proc *)filp->private_data;
	__u64 data, token;
	int parent;
	ssize_t res;

	if (count < sizeof(data))
		return -EINVAL;
	if (p->initialized || count > sizeof(data))
		return -ENOSPC;
	if (get_user(data, (const __u64 __user *)buf))
		return -EFAULT;

	parent = (data & 1ULL) == 1ULL;
	token = data & ~1ULL;

	// Passing nonzero to the first write means that this process is Main and
	// should pass through unobstructed.
	//
	// If the first process to issue the write is Main, it should run
	// unobstructed and set the initialized flag, so that the other process
	// does not need to stop when it issues the write. If, on the other hand,
	// the first process to get here is the child, then it should wait until
	// Main has issued its first read call.
	res = sizeof(data);
	spin_lock_irq(&p->ctx->lock);
	if (parent) {
		if (p->ctx->parent_initialized) {
			res = -EINVAL;
			goto unlock;
		}
		p->ctx->parent_initialized = 1;
		p->ctx->token = token;
	} else {
		if (p->ctx->child_initialized) {
			res = -EINVAL;
			goto unlock;
		}
		p->ctx->child_initialized = 1;
		if (!p->ctx->parent_initialized) {
			int switch_res = transact_switch_locked(p);  // releases ctx->lock.
			if (switch_res == -EDEADLOCK) {
				res = 0;
				goto out;
			}
			if (switch_res < 0) {
				res = switch_res;
				goto out;
			}
			spin_lock_irq(&p->ctx->lock);
		}
		if (token != p->ctx->token) {
			res = -EINVAL;
			goto unlock;
		}
	}

unlock:
	spin_unlock_irq(&p->ctx->lock);
out:
	return res;
}

int transact_release(struct inode *inode, struct file *filp)
{
	struct transact_proc *p = (struct transact_proc *)filp->private_data;
	struct transact_ctx *ctx = p->ctx;

	transact_notify_death(p);
	spin_lock_irq(&ctx->dev->lock);
	kref_put(&ctx->kref, transact_release_ctx_locked);
	spin_unlock_irq(&ctx->dev->lock);

	return 0;
}

static struct file_operations transact_fops = {
	.owner = THIS_MODULE,
	.open = transact_open,
	.read = transact_read,
	.write = transact_write,
	.release = transact_release,
};

/* Initialize the LKM */
__init int init_module()
{
	int err;
	
	memset(&g_cdev, 0, sizeof(g_cdev));
	g_cdev.devnum = MKDEV(TRANSACT_MAJOR, 0);
	err = register_chrdev_region(g_cdev.devnum, 1, "transact");
	if (err < 0) {
		printk(KERN_WARNING "Failed to allocate major/minor numbers\n");
		return err;
	}

	cdev_init(&g_cdev.cdev, &transact_fops);
	g_cdev.cdev.owner = THIS_MODULE;
	g_cdev.cdev.ops = &transact_fops;
	spin_lock_init(&g_cdev.lock);
	INIT_LIST_HEAD(&g_cdev.ctx);
	err = cdev_add(&g_cdev.cdev, g_cdev.devnum, 1);
	if (err < 0) {
		printk(KERN_WARNING "Failed to add cdev\n");
		goto unregister;
	}

  return 0;

unregister:
	unregister_chrdev_region(g_cdev.devnum, 1);
	return err;
}

__exit void cleanup_module()
{
	cdev_del(&g_cdev.cdev);
	unregister_chrdev_region(g_cdev.devnum, 1);
}

MODULE_AUTHOR("lhchavez");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("A named semaphore for two processes with death notification.");
