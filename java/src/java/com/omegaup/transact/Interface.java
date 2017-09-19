// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.omegaup.transact;

import java.io.IOException;

public class Interface {
	static {
		System.loadLibrary("transact_java");
	}

	private long interfacePtr;
	public final String name;

	public Interface(boolean parent, String name, String transactName,
			String shmName, long size) throws IOException {
		this.name = name;
		this.interfacePtr = nativeInit(parent, transactName, shmName, size);
	}

	@Override
	public void finalize() {
		nativeFinalize(this.interfacePtr);
		this.interfacePtr = 0;
	}

	public Message buildMessage() throws IOException {
		return new Message(interfacePtr);
	}

	private native long nativeInit(boolean parent, String transactName,
			String shmName, long size) throws IOException;
	private native void nativeFinalize(long interfacePtr);
}
