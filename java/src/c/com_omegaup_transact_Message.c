// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "com_omegaup_transact_Message.h"

#include "transact.h"

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeByte(JNIEnv* env, jobject thisObj,
		jint x) {
	char* writeptr = (char*)(*env)->GetLongField(env, thisObj,
			g_fields.message_writeptr);
	*(writeptr++) = x;
	(*env)->SetLongField(env, thisObj, g_fields.message_writeptr,
			(jlong)writeptr);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeShort(JNIEnv* env, jobject
		thisObj, jshort x) {
	short* writeptr = (short*)(*env)->GetLongField(env, thisObj,
			g_fields.message_writeptr);
	*(writeptr++) = x;
	(*env)->SetLongField(env, thisObj, g_fields.message_writeptr,
			(jlong)writeptr);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeInt(JNIEnv* env, jobject thisObj,
		jint x) {
	int* writeptr = (int*)(*env)->GetLongField(env, thisObj,
			g_fields.message_writeptr);
	*(writeptr++) = x;
	(*env)->SetLongField(env, thisObj, g_fields.message_writeptr,
			(jlong)writeptr);
}

JNIEXPORT void JNICALL
Java_com_omegaup_transact_Message_writeLong(JNIEnv* env, jobject thisObj,
		jlong x) {
	long long* writeptr = (long long*)(*env)->GetLongField(env, thisObj,
			g_fields.message_writeptr);
	*(writeptr++) = x;
	(*env)->SetLongField(env, thisObj, g_fields.message_writeptr,
			(jlong)writeptr);
}

JNIEXPORT jint JNICALL
Java_com_omegaup_transact_Message_readByte(JNIEnv* env, jobject thisObj) {
	char* readptr = (char*)(*env)->GetLongField(env, thisObj,
			g_fields.message_readptr);
	char x = *(readptr++);
	(*env)->SetLongField(env, thisObj, g_fields.message_readptr,
			(jlong)readptr);

	return (jint)x;
}

JNIEXPORT jshort JNICALL
Java_com_omegaup_transact_Message_readShort(JNIEnv* env, jobject thisObj) {
	short* readptr = (short*)(*env)->GetLongField(env, thisObj,
			g_fields.message_readptr);
	short x = *(readptr++);
	(*env)->SetLongField(env, thisObj, g_fields.message_readptr,
			(jlong)readptr);

	return (jshort)x;
}

JNIEXPORT jint JNICALL
Java_com_omegaup_transact_Message_readInt(JNIEnv* env, jobject thisObj) {
	int* readptr = (int*)(*env)->GetLongField(env, thisObj,
			g_fields.message_readptr);
	int x = *(readptr++);
	(*env)->SetLongField(env, thisObj, g_fields.message_readptr,
			(jlong)readptr);

	return (jint)x;
}

JNIEXPORT jlong JNICALL
Java_com_omegaup_transact_Message_readLong(JNIEnv* env, jobject thisObj) {
	long long* readptr = (long long*)(*env)->GetLongField(env, thisObj,
			g_fields.message_readptr);
	long long x = *(readptr++);
	(*env)->SetLongField(env, thisObj, g_fields.message_readptr,
			(jlong)readptr);

	return (jlong)x;
}
