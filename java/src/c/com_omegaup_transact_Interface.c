// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "com_omegaup_transact_Interface.h"

#include <errno.h>
#include <string.h>

#include <libtransact.h>

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

	g_fields.interface_interfacePtr = (*env)->GetFieldID(env, g_classes.interface,
			"interfacePtr", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_messagePtr = (*env)->GetFieldID(env, g_classes.message,
			"messagePtr", "J");
	if ((*env)->ExceptionCheck(env)) return -1;
	g_fields.message_msgid = (*env)->GetFieldID(env, g_classes.message,
			"msgid", "I");
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

JNIEXPORT jlong JNICALL
Java_com_omegaup_transact_Interface_nativeInit(JNIEnv* env,
		jobject thisObj, jboolean parent, jstring transact_nameJNI,
		jstring shm_nameJNI, jlong size) {
	const char* transact_name = (*env)->GetStringUTFChars(
			env, transact_nameJNI, NULL);
	if (!transact_name) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transactName");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/NullPointerExceptin"),
				buffer);
		return 0;
	}
	const char* shm_name = (*env)->GetStringUTFChars(env, shm_nameJNI, NULL);
	if (!transact_name) {
		(*env)->ReleaseStringUTFChars(env, transact_nameJNI, transact_name);
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "shmName");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/NullPointerExceptin"),
				buffer);
		return 0;
	}
	struct transact_interface* interface =
			transact_interface_open(parent ? 1 : 0, transact_name, shm_name, size);
	int saved_errno = errno;
	if (!interface) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_interface_open %s: %s", transact_name,
				strerror(saved_errno));
		(*env)->ReleaseStringUTFChars(env, shm_nameJNI, shm_name);
		(*env)->ReleaseStringUTFChars(env, transact_nameJNI, transact_name);
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return 0;
	}
	(*env)->ReleaseStringUTFChars(env, shm_nameJNI, shm_name);
	(*env)->ReleaseStringUTFChars(env, transact_nameJNI, transact_name);
	return (jlong)interface;
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Interface_nativeFinalize(JNIEnv* env,
		jobject thisObj, jlong interfacePtr) {
	transact_interface_close((struct transact_interface*)interfacePtr);
}
