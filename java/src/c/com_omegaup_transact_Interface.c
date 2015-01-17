// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "com_omegaup_transact_Interface.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "transact.h"

struct classes g_classes;
struct methods g_methods;
struct fields g_fields;

JNIEXPORT jint JNICALL
JNI_OnLoad(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK)
		return -1;

	jclass clazz;
	clazz = (*env)->FindClass(env, "com/omegaup/transact/Interface");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_classes.interface = (*env)->NewGlobalRef(env, clazz);
	(*env)->DeleteLocalRef(env, clazz);

	clazz = (*env)->FindClass(env, "com/omegaup/transact/Message");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_classes.message = (*env)->NewGlobalRef(env, clazz);
	(*env)->DeleteLocalRef(env, clazz);

	g_fields.interface_name = (*env)->GetFieldID(env, g_classes.interface,
			"name", "Ljava/lang/String;");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.interface_size = (*env)->GetFieldID(env, g_classes.interface,
			"size", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.interface_transactFd = (*env)->GetFieldID(env, g_classes.interface,
			"transactFd", "I");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.interface_shmFd = (*env)->GetFieldID(env, g_classes.interface,
			"shmFd", "I");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.interface_shm = (*env)->GetFieldID(env, g_classes.interface,
			"shm", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_msgid = (*env)->GetFieldID(env, g_classes.message,
			"msgid", "I");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_ptr = (*env)->GetFieldID(env, g_classes.message,
			"ptr", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_writeptr = (*env)->GetFieldID(env, g_classes.message,
			"writeptr", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_readptr = (*env)->GetFieldID(env, g_classes.message,
			"readptr", "J");
	if ((*env)->ExceptionCheck(env)) return -1;

	return JNI_VERSION_1_6;
}

JNIEXPORT void JNICALL
JNI_OnUnload(JavaVM* vm, void* reserved) {
	JNIEnv* env;
	if ((*vm)->GetEnv(vm, (void**)&env, JNI_VERSION_1_6) != JNI_OK)
		return;

	(*env)->DeleteGlobalRef(env, g_classes.interface);
	(*env)->DeleteGlobalRef(env, g_classes.message);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Interface_nativeInit(JNIEnv* env, jobject
		thisObj, jboolean parent, jstring transact_nameJNI, jstring shm_nameJNI,
		jlong size) {
	(*env)->SetLongField(env, thisObj, g_fields.interface_size,
			size / sizeof(struct message));
	const char* transact_name = (*env)->GetStringUTFChars(
			env, transact_nameJNI, NULL);
	int transact_fd = open(transact_name, O_RDWR);
	if (transact_fd == -1) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "open %s: %s", transact_name,
				strerror(errno));
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	(*env)->ReleaseStringUTFChars(env, transact_nameJNI, transact_name);

	// Make sure the child process waits until the parent issues a read() call.
	unsigned long long message = parent;
	write(transact_fd, &message, sizeof(message));

	const char* shm_name = (*env)->GetStringUTFChars(env, shm_nameJNI, NULL);
	int shm_fd = open(shm_name, O_RDWR);
	if (shm_fd == -1) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "open %s: %s", shm_name, strerror(errno));
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	(*env)->ReleaseStringUTFChars(env, shm_nameJNI, shm_name);

	if (parent && ftruncate(shm_fd, size) == -1) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "ftruncate: %s", strerror(errno));
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	struct message_root* shm = (struct message_root*)mmap(NULL, size,
			PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
	if (shm == (struct message_root*)-1) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "mmap: %s", strerror(errno));
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	if (parent) {
		shm->free_offset = 0;
		shm->small_message_list = (ptrdiff_t)-1;
		shm->large_message_list = (ptrdiff_t)-1;
	}

	// Write everything back into the Java object.
	(*env)->SetIntField(env, thisObj, g_fields.interface_transactFd,
			transact_fd);
	(*env)->SetIntField(env, thisObj, g_fields.interface_shmFd, shm_fd);
	(*env)->SetLongField(env, thisObj, g_fields.interface_shm, (jlong)shm);
}

static void
buildMessage(JNIEnv* env, jobject message, int msgid, struct message* msg) {
	msg->msgid = msgid;
	(*env)->SetIntField(env, message, g_fields.message_msgid, msg->msgid);
	(*env)->SetLongField(env, message, g_fields.message_ptr, (jlong)msg);
	(*env)->SetLongField(env, message, g_fields.message_readptr, (jlong)msg->data);
	(*env)->SetLongField(env, message, g_fields.message_writeptr, (jlong)msg->data);
	(*env)->SetLongField(env, message, g_fields.message_end,
			(jlong)((char*)msg + msg->size * sizeof(struct message)));
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Interface_allocate(JNIEnv* env,
		jobject thisObj, jobject messageObj, jint msgid, jlong bytes) {
	struct message_root* shm = (struct message_root*)(*env)->GetLongField(env,
			thisObj, g_fields.interface_shm);
	jlong size = (*env)->GetLongField(env, thisObj, g_fields.interface_size);
	bytes += 32; // For the page header.
	bytes += (~(bytes - 1) & 0x3F);
	size_t blocks = bytes / sizeof(struct message);

	ptrdiff_t head = blocks == 1 ?
			shm->small_message_list :
			shm->large_message_list;
	ptrdiff_t next = head;
	ptrdiff_t prev_next = size + 1;

	// Try to reuse an old allocation.
	while (next != (ptrdiff_t)-1) {
		// The messages form a linked list that goes backwards. If any next pointer
		// lies outside of the legal values, or it does not appear before in the
		// shared memory region, it is definitely invalid.
		if (next < 0 || next >= size || next >= prev_next) {
			(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
					"Illegal next pointer");
			return;
		}
		struct message* prev = shm->root + next;
		if (prev->free && prev->size == blocks) {
			prev->free = 0;
			buildMessage(env, messageObj, msgid, prev);
			return;
		}
		prev_next = next;
		next = prev->next;
	}

	// Sanity check.
	ptrdiff_t free_offset = shm->free_offset;
	if (free_offset < 0 || free_offset >= size) {
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				"Illegal free offset");
		return;
	}

	// Need to perform allocation.
	if (size < free_offset + blocks) {
		// No more memory.
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "No more memory for arena allocation. "
				"Need shm size to be at least %zu\n", (free_offset + blocks) *
				sizeof(struct message) + sizeof(struct message_root));
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	
	struct message* ptr = shm->root + free_offset;
	ptr->next = head;
	ptr->size = blocks;
	ptr->free = 0;
	if (blocks == 1) {
		shm->small_message_list = free_offset;
	} else {
		shm->large_message_list = free_offset;
	}
	shm->free_offset = free_offset + blocks;

	buildMessage(env, messageObj, msgid, ptr);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Interface_get(JNIEnv* env, jobject thisObj,
		jobject messageObj) {
	struct message_root* shm = (struct message_root*)(*env)->GetLongField(env,
			thisObj, g_fields.interface_shm);
	jlong size = (*env)->GetLongField(env, thisObj, g_fields.interface_size);
	ptrdiff_t offset = shm->current_msg_offset;
	if (offset < 0 || offset >= size) {
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				"Illegal message offset");
		return;
	}

	struct message* msg = shm->root + offset;

	buildMessage(env, messageObj, msg->msgid, msg);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Interface_call(JNIEnv* env, jobject thisObj,
		jobject messageObj, jboolean noret, jboolean nofree) {
	int transact_fd = (*env)->GetIntField(env, thisObj,
			g_fields.interface_transactFd);
	struct message_root* shm = (struct message_root*)(*env)->GetLongField(env,
			thisObj, g_fields.interface_shm);
	struct message* request = (struct message*)(*env)->GetLongField(env,
			messageObj, g_fields.message_ptr);

	shm->current_msg_offset = request - shm->root;
	unsigned long long message;
	if (read(transact_fd, &message, sizeof(message)) != sizeof(message)) {
		if (noret) {
			jclass system = (*env)->FindClass(env, "java/lang/System");
			jmethodID exit = (*env)->GetStaticMethodID(env, system, "exit", "(I)V");
			(*env)->CallStaticIntMethod(env, system, exit, 0);
			return;
		}
		jstring interface_nameJNI = (*env)->GetObjectField(env, thisObj,
				g_fields.interface_name);
		const char* interface_name = (*env)->GetStringUTFChars(env,
				interface_nameJNI, NULL);
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "%s died unexpectedly while "
				"calling 0x%zx", interface_name, request->msgid);
		(*env)->ReleaseStringUTFChars(env, interface_nameJNI, interface_name);
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	if (!nofree) {
		request->free = 1;
	}

	Java_com_omegaup_transact_Interface_get(env, thisObj, messageObj);
}
