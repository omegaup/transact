/*
 * Copyright (c) 2017, The omegaUp Contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the omegaUp nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "libtransact.h"

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include <memory>

namespace {

#define DISALLOW_COPY_AND_ASSIGN(T) \
  T(const T&) = delete;             \
  T& operator=(T&) = delete

class ScopedFD {
 public:
  static constexpr int kInvalidFd = -1;

  ScopedFD(int fd = kInvalidFd) : fd_(fd) {}
  ~ScopedFD() { reset(); }

  void reset(int fd = kInvalidFd) {
    std::swap(fd, fd_);
    if (fd == kInvalidFd)
      return;
    close(fd);
  }

  int release() {
    int fd = kInvalidFd;
    std::swap(fd, fd_);
    return fd;
  }

  int get() const { return fd_; }

  operator bool() const { return fd_ != kInvalidFd; }

 private:
  int fd_;
  DISALLOW_COPY_AND_ASSIGN(ScopedFD);
};

struct Message {
  ptrdiff_t next;
  size_t blocks_len;
  size_t free;
  size_t msgid;
  char data[32];
};

static_assert(sizeof(Message) == 64, "Invalid Message size");

struct MessageHeader {
  volatile ptrdiff_t current_msg_offset;
  volatile ptrdiff_t free_offset;
  volatile ptrdiff_t small_message_list;
  volatile ptrdiff_t large_message_list;
  char padding[32];

  Message root[0];
};

static_assert(sizeof(MessageHeader) == 64, "Invalid MessageHeader size");

}  // namespace

inline void* operator new(size_t len) {
  return malloc(len);
}

inline void operator delete(void* p) {
  free(p);
}

struct transact_interface {
  ScopedFD transact_fd;
  ScopedFD shm_fd;
  size_t blocks_len;
  MessageHeader* shm = reinterpret_cast<MessageHeader*>(-1);

  ~transact_interface() {
    if (shm == reinterpret_cast<MessageHeader*>(-1))
      return;
    munmap(shm, blocks_len * sizeof(Message));
  }
};

static void MessageInitialize(struct transact_message* message, Message* msg) {
  if (!message)
    return;

  if (!msg) {
    message->method_id = 0;
    message->message = nullptr;
    message->data = message->end = nullptr;
    return;
  }

  message->message = msg;
  message->method_id = msg->msgid;
  message->data = msg->data;
  message->end = reinterpret_cast<char*>(msg + msg->blocks_len);
}

transact_interface* transact_interface_open(int is_parent,
                                            const char* transact_filename,
                                            const char* shm_filename,
                                            size_t shm_len) {
  std::unique_ptr<transact_interface> interface(new transact_interface());

  if (!interface) {
    errno = ENOMEM;
    return nullptr;
  }

  interface->blocks_len = shm_len / sizeof(MessageHeader);
  interface->transact_fd.reset(open(transact_filename, O_RDWR));
  if (!interface->transact_fd)
    return nullptr;

  // Make sure the child process waits until the parent issues a read() call.
  {
    unsigned long long handshake = is_parent;
    ssize_t written = TEMP_FAILURE_RETRY(
        write(interface->transact_fd.get(), &handshake, sizeof(handshake)));
		if (written == 0)
			errno = EPIPE;
    if (written != sizeof(handshake))
      return nullptr;
  }

  interface->shm_fd.reset(open(shm_filename, O_RDWR));
  if (!interface->shm_fd)
    return nullptr;
  if (is_parent && ftruncate(interface->shm_fd.get(), shm_len) == -1)
    return nullptr;
  interface->shm = reinterpret_cast<MessageHeader*>(
      mmap(NULL, shm_len, PROT_READ | PROT_WRITE, MAP_SHARED,
           interface->shm_fd.get(), 0));
  if (interface->shm == reinterpret_cast<MessageHeader*>(-1))
    return nullptr;
  if (is_parent) {
    interface->shm->free_offset = 0;
    interface->shm->small_message_list = static_cast<ptrdiff_t>(-1);
    interface->shm->large_message_list = static_cast<ptrdiff_t>(-1);
  }
  return interface.release();
}

void transact_interface_close(struct transact_interface* interface) {
	if (!interface)
		return;
  delete interface;
}

struct transact_message* transact_message_new() {
  struct transact_message* message = new transact_message();
  if (!message)
    errno = ENOMEM;
  return message;
}

void transact_message_free(struct transact_message* message) {
  if (!message)
    return;
  delete message;
}

int transact_message_init(struct transact_interface* interface,
                          struct transact_message* message) {
  if (!interface) {
    errno = EFAULT;
    return -1;
  }
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  memset(message, 0, sizeof(*message));
  message->interface = interface;
  return 0;
}

int transact_message_allocate(struct transact_message* message,
                              int id,
                              size_t len) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  len += 32;                   // For the page header.
  len += (~(len - 1) & 0x3F);  // Align to blocks.
  size_t blocks = len / sizeof(Message);

  ptrdiff_t head = blocks == 1 ? message->interface->shm->small_message_list
                               : message->interface->shm->large_message_list;
  ptrdiff_t next = head;
  ptrdiff_t prev_next = message->interface->blocks_len + 1;

  // Try to reuse an old allocation.
  while (next != static_cast<ptrdiff_t>(-1)) {
    // The messages form a linked list that goes backwards. If any next pointer
    // lies outside of the legal values, or it does not appear before in the
    // shared memory region, it is definitely invalid.
    if (next < 0 || next >= message->interface->blocks_len ||
        next >= prev_next) {
      errno = EINVAL;
      return -1;
    }
    Message* prev = message->interface->shm->root + next;
    if (prev->free && prev->blocks_len == blocks) {
      prev->free = 0;
      prev->msgid = id;
      MessageInitialize(message, prev);
      return 0;
    }
    prev_next = next;
    next = prev->next;
  }

  // Sanity check.
  ptrdiff_t free_offset = message->interface->shm->free_offset;
  if (free_offset < 0 || free_offset >= message->interface->blocks_len) {
    errno = EINVAL;
    return -1;
  }

  // Need to perform allocation.
  if (message->interface->blocks_len < free_offset + blocks) {
    errno = ENOMEM;
    return -1;
  }

  Message* ptr = message->interface->shm->root + free_offset;
  ptr->next = head;
  ptr->blocks_len = blocks;
  ptr->free = 0;
  ptr->msgid = id;
  if (blocks == 1) {
    message->interface->shm->small_message_list = free_offset;
  } else {
    message->interface->shm->large_message_list = free_offset;
  }
  message->interface->shm->free_offset = free_offset + blocks;

  MessageInitialize(message, ptr);
  return 0;
}

int transact_message_recv(struct transact_message* message) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  ptrdiff_t offset = message->interface->shm->current_msg_offset;
  if (offset < 0 || offset >= message->interface->blocks_len) {
    errno = EMSGSIZE;
    return -1;
  }
  MessageInitialize(message, message->interface->shm->root + offset);
  return 0;
}

int transact_message_send(struct transact_message* message) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  message->interface->shm->current_msg_offset =
      reinterpret_cast<Message*>(message->message) -
      message->interface->shm->root;
  unsigned long long response;
  ssize_t read_bytes = TEMP_FAILURE_RETRY(
      read(message->interface->transact_fd.get(), &response, sizeof(response)));
  if (read_bytes == 0)
    return 0;
  if (read_bytes != sizeof(response))
    return -1;
  reinterpret_cast<Message*>(message->message)->free = 1;
  MessageInitialize(message, nullptr);
  return 1;
}

ssize_t transact_message_read(struct transact_message* message,
                              void* target,
                              size_t len) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  if (message->data + len > message->end)
    return 0;
  memcpy(target, message->data, len);
  message->data += len;
  return len;
}

ssize_t transact_message_read_array(struct transact_message* message,
                                    void** target,
                                    size_t len) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  if (message->data + len > message->end)
    return 0;
  *target = reinterpret_cast<void*>(message->data);
  message->data += len;
  return len;
}

ssize_t transact_message_write(struct transact_message* message,
                               const void* source,
                               size_t len) {
  if (!message) {
    errno = EFAULT;
    return -1;
  }

  if (message->data + len > message->end)
    return 0;
  memcpy(message->data, source, len);
  message->data += len;
  return len;
}
