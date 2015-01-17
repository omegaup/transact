#ifndef __TRANSACT_H
#define __TRANSACT_H

#include <stddef.h>

struct message {
	ptrdiff_t next;
	size_t size;
	size_t free;
	size_t msgid;
	char data[32];
};

struct message_root {
	volatile ptrdiff_t current_msg_offset;
	volatile ptrdiff_t free_offset;
	volatile ptrdiff_t small_message_list;
	volatile ptrdiff_t large_message_list;
	char padding[32];

	struct message root[0];
};

struct classes {
	jclass interface;
	jclass message;
};

extern struct classes g_classes;

struct fields {
	jfieldID interface_name;
	jfieldID interface_size;
	jfieldID interface_transactFd;
	jfieldID interface_shmFd;
	jfieldID interface_shm;
	jfieldID message_msgid;
	jfieldID message_ptr;
	jfieldID message_writeptr;
	jfieldID message_readptr;
	jfieldID message_end;
};

extern struct fields g_fields;

struct methods {
};

extern struct methods g_methods;

#endif  // __TRANSACT_H
