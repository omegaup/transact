// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.omegaup.transact;

import java.io.IOException;

public class Interface {
	static {
		System.loadLibrary("transact");
	}

	public final String name;
	private final long size;
	private final int transactFd;
	private final int shmFd;
	private final long shm;

	public Interface(boolean parent, String name, String transactName,
			String shmName, long size) throws IOException {
		this.name = name;
		this.size = this.shm = this.transactFd = this.shmFd = 0;
		nativeInit(parent, transactName, shmName, size);
	}

	public native void get(Message message) throws IOException;
	public native void allocate(Message message, int msgid, long bytes) throws IOException;
	public native void call(Message message, boolean noret, boolean nofree) throws IOException;

	private native void nativeInit(boolean parent, String transactName, String shmName, long size) throws IOException;
}
