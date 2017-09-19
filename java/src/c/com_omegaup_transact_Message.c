// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "com_omegaup_transact_Message.h"

#include <stdint.h>
#include <stdio.h>
#include <libtransact.h>

#include "transact.h"

JNIEXPORT jlong JNICALL
Java_com_omegaup_transact_Message_nativeInit(JNIEnv* env,
		jobject thisObj, jlong interfacePtr) {
	struct transact_message* message = transact_message_new();
	if (!message) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_message_init: %m");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/lang/OutOfMemoryError"),
				buffer);
		return 0;
	}
	if (transact_message_init((struct transact_interface*)interfacePtr, message)) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_message_init: %m");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return 0;
	}
	return (jlong)message;
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_nativeFinalize(JNIEnv* env,
		jobject thisObj, jlong messagePtr) {
	transact_message_free((struct transact_message*)messagePtr);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_allocate(JNIEnv* env,
		jobject thisObj, jint msgid, jlong bytes) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	if (transact_message_allocate(message, msgid, bytes)) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_message_allocate: %m");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
	}
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_receive(JNIEnv* env,
		jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	if (transact_message_recv(message)) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_message_recv: %m");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return;
	}
	(*env)->SetIntField(env, thisObj, g_fields.message_msgid,
			message->method_id);
}

JNIEXPORT int JNICALL
Java_com_omegaup_transact_Message_send(JNIEnv* env,
		jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	int res = transact_message_send(message);
	if (res == -1) {
		char buffer[1024];
		snprintf(buffer, sizeof(buffer), "transact_message_send: %m");
		(*env)->ThrowNew(env, (*env)->FindClass(env, "java/io/IOException"),
				buffer);
		return -1;
	}
	return res;
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeByte(JNIEnv* env, jobject thisObj,
		jint x) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	*(int8_t*)message->data = (int8_t)x;
	message->data += sizeof(int8_t);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeShort(JNIEnv* env, jobject
		thisObj, jshort x) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	*(int16_t*)message->data = x;
	message->data += sizeof(int16_t);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeInt(JNIEnv* env, jobject thisObj,
		jint x) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	*(int32_t*)message->data = x;
	message->data += sizeof(int32_t);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeLong(JNIEnv* env, jobject thisObj,
		jlong x) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	*(int64_t*)message->data = x;
	message->data += sizeof(int64_t);
}

JNIEXPORT jint JNICALL
Java_com_omegaup_transact_Message_readByte(JNIEnv* env, jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	jint x = *(int8_t*)message->data;
	message->data += sizeof(int8_t);

	return (jint)x;
}

JNIEXPORT jshort JNICALL
Java_com_omegaup_transact_Message_readShort(JNIEnv* env, jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	jshort x = *(int16_t*)message->data;
	message->data += sizeof(int16_t);

	return x;
}

JNIEXPORT jint JNICALL
Java_com_omegaup_transact_Message_readInt(JNIEnv* env, jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	jint x = *(int32_t*)message->data;
	message->data += sizeof(int32_t);

	return x;
}

JNIEXPORT jlong JNICALL
Java_com_omegaup_transact_Message_readLong(JNIEnv* env, jobject thisObj) {
	struct transact_message* message =
			(struct transact_message*)(*env)->GetLongField(env, thisObj,
					g_fields.message_messagePtr);
	jlong x = *(int64_t*)message->data;
	message->data += sizeof(int64_t);

	return x;
}
