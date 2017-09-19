// Copyright (c) 2014 The omegaUp Contributors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package com.omegaup.transact;

import java.io.IOException;

public class Message {
	private long messagePtr;
	public int msgid;

	Message(long interfacePtr) throws IOException {
		messagePtr = nativeInit(interfacePtr);
	}

	@Override
	public void finalize() {
		nativeFinalize(this.messagePtr);
		this.messagePtr = 0;
	}

	public native void allocate(int msgid, long bytes) throws IOException;
	public native void receive() throws IOException;
	public native int send() throws IOException;

	public void writeInt(int x, int minValue, int maxValue) {
		if (x < minValue || x > maxValue) {
			throw new IllegalArgumentException("Value outside " +
					"of legal range: [" + minValue + ", " + maxValue +
					"]");
		}
		writeInt(x);
	}

	public native void writeByte(int x);
	public native void writeShort(short x);
	public native void writeInt(int x);
	public native void writeLong(long x);

	public void writeChar(char x) {
		writeByte((int)x);
	}

	public void writeBool(boolean x) {
		writeByte(x ? 1 : 0);
	}

	public void writeDouble(double x) {
		writeLong(Double.doubleToLongBits(x));
	}

	public void writeFloat(float x) {
		writeInt(Float.floatToIntBits(x));
	}

	public int readInt(int minValue, int maxValue) {
		int x = readInt();
		if (x < minValue || x > maxValue) {
			throw new IllegalArgumentException("Value outside " +
					"of legal range: [" + minValue + ", " + maxValue +
					"]");
		}
		return x;
	}

	public native int readByte();
	public native short readShort();
	public native int readInt();
	public native long readLong();

	public char readChar() {
		return (char)readByte();
	}

	public double readDouble() {
		return Double.longBitsToDouble(readLong());
	}

	public float readFloat() {
		return Float.intBitsToFloat(readInt());
	}

	public boolean readBool() {
		return readByte() != 0;
	}

	private native long nativeInit(long interfacePtr) throws IOException;
	private native void nativeFinalize(long messagePtr);
}
