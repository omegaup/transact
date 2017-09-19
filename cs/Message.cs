using System.Runtime.InteropServices;
using System;

namespace Omegaup.Transact
{
	public class Message
	{
		[StructLayout (LayoutKind.Sequential)]
		internal unsafe struct State {
			public IntPtr interfacePtr;
			IntPtr messagePtr;
			public int methodId;
			int padding;

			public byte* shmPtr;
			public byte* end;

			[DllImport("libtransact.so", EntryPoint = "transact_message_init", SetLastError = true)]
			public static extern int MessageInit(IntPtr interface_ptr,
					Message.State* message_ptr);

			[DllImport("libtransact.so", EntryPoint = "transact_message_allocate", SetLastError = true)]
			public static extern int Allocate(State* message_ptr, int msgid,
					ulong bytes);
			[DllImport("libtransact.so", EntryPoint = "transact_message_recv", SetLastError = true)]
			public static extern int Receive(State* message_ptr);
			[DllImport("libtransact.so", EntryPoint = "transact_message_send", SetLastError = true)]
			public static extern int Send(State* message_ptr);
		}

		internal State state;

		internal unsafe Message(IntPtr interfacePtr) {
			state = new State();
			fixed (State *statePtr = &state) {
				if (State.MessageInit(interfacePtr, statePtr) != 0) {
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}
			}
		}

		public int methodId {
			get { return state.methodId; }
		}

		public unsafe void Allocate(int msgid, ulong bytes) {
			fixed (State *statePtr = &state) {
				if (State.Allocate(statePtr, msgid, bytes) != 0) {
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}
			}	
		}

		public unsafe void Receive() {
			fixed (State *statePtr = &state) {
				if (State.Receive(statePtr) != 0) {
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}
			}
		}

		public unsafe int Send() {
			fixed (State *statePtr = &state) {
				int retval = State.Send(statePtr);
				if (retval == -1) {
					Marshal.ThrowExceptionForHR(Marshal.GetHRForLastWin32Error());
				}
				return retval;
			}
		}

		public void Write(int x, int minValue, int maxValue) {
			if (x < minValue || x > maxValue) {
				throw new ArgumentOutOfRangeException("Value outside " +
						"of legal range: [" + minValue + ", " + maxValue +
						"]");
			}
			Write(x);
		}

		public unsafe void Write(byte x) {
			*((byte*)state.shmPtr) = x;
			state.shmPtr += sizeof(byte);
		}

		public unsafe void Write(short x) {
			*((short*)state.shmPtr) = x;
			state.shmPtr += sizeof(short);
		}

		public unsafe void Write(int x) {
			*((int*)state.shmPtr) = x;
			state.shmPtr += sizeof(int);
		}

		public unsafe void Write(long x) {
			*((long*)state.shmPtr) = x;
			state.shmPtr += sizeof(long);
		}

		public void Write(char x) {
			Write((byte)x);
		}

		public void Write(bool x) {
			Write((byte)(x ? 1 : 0));
		}

		public void Write(double x) {
			Write(BitConverter.DoubleToInt64Bits(x));
		}

		public void Write(float x) {
			Write(BitConverter.ToInt32(BitConverter.GetBytes(x), 0));
		}

		public int ReadInt32(int minValue, int maxValue) {
			int x = ReadInt32();
			if (x < minValue || x > maxValue) {
				throw new ArgumentOutOfRangeException("Value outside " +
						"of legal range: [" + minValue + ", " + maxValue +
						"]");
			}
			return x;
		}

		public unsafe byte ReadByte() {
			byte x = *((byte*)state.shmPtr);
			state.shmPtr += sizeof(byte);
			return x;
		}

		public unsafe short ReadShort() {
			short x = *((short*)state.shmPtr);
			state.shmPtr += sizeof(short);
			return x;
		}

		public unsafe int ReadInt32() {
			int x = *((int*)state.shmPtr);
			state.shmPtr += sizeof(int);
			return x;
		}

		public unsafe long ReadInt64() {
			long x = *((long*)state.shmPtr);
			state.shmPtr += sizeof(long);
			return x;
		}

		public char ReadChar() {
			return (char)ReadByte();
		}

		public double ReadDouble() {
			return BitConverter.Int64BitsToDouble(ReadInt64());
		}

		public float ReadSingle() {
			return BitConverter.ToSingle(BitConverter.GetBytes(ReadInt32()), 0);
		}

		public bool ReadBool() {
			return ReadByte() != 0;
		}
	}
}
