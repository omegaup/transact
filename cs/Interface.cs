using System.IO;
using System.Runtime.InteropServices;
using System;

namespace Omegaup.Transact
{
	public class Interface
	{
		internal unsafe class Transact {
			[DllImport("libtransact.so", EntryPoint = "transact_interface_open", SetLastError = true)]
			public static extern IntPtr InterfaceOpen(int is_parent, string
					transact_filename, string shm_filename, ulong shm_len);
			[DllImport("libtransact.so", EntryPoint = "transact_interface_close", SetLastError = true)]
			public static extern int InterfaceClose(IntPtr interface_ptr);
		}

		private IntPtr interfacePtr;

		public Interface(bool parent, string name, string transactName,
				string shmName, ulong size) {
			interfacePtr = Transact.InterfaceOpen(parent ? 1 : 0,
					transactName, shmName, size);
			if (interfacePtr == IntPtr.Zero) {
				throw new IOException("Unable to initialize " + name,
						Marshal.GetExceptionForHR(Marshal.GetHRForLastWin32Error()));
			}
		}

		~Interface() {
			if (interfacePtr != IntPtr.Zero) {
				Transact.InterfaceClose(interfacePtr);
			}
		}

		public Message BuildMessage() {
			return new Message(interfacePtr);
		}
	}
}
