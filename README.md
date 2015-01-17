# transact

A super fast semaphore for two processes with death notifications. This module
is essentially a named version of eventfd (i.e. it communicates through a file
in the filesystem) with less bells and whistles.

## Semantics

First you need to create a character device file with major 2038 and minor 0.
There is a tool in the kernel/ directory (`mktransact`) that will do that for
you, and make you the owner of the file if called using `sudo`.

    sudo mktransact file

After opening the file from your program, you need to write an 8-byte value to
inform transact if you are the parent process (which will run first) or the
child (which will wait until the parent yields control). Every time you want to
yield control to the other process, issue a read with an 8-byte buffer. That
will atomically make your process sleep and wake up the other. Once one process
closes its file descriptor (or is terminated), the other process will be woken
up and the read will return empty, signalling an EOF. From this point on, any
further attempts to read will return immediately.

## Performance

Given that other synchronization methods need two system calls to work
(typically one to notify other processes, and another one to wait) and transact
only needs one, it is at least ~100ns faster on modern Linux systems.
Furthermore, since it is limited to a case with exactly two processes/threads,
it is typically 33% faster to perform the switch than pipes or semaphores.

## Isolation

Since transact uses files in the filesystem to coordinate between processes,
the permissions on the file will dictate which processes can communicate using
this method. transact only requires access to the following syscalls: `open`,
`close`, `read`, `write`.

## Example

    int fd = open("file", O_RDWR);

		// Immediately after opening, we need to tell transact which process is
		// the parent and which one is the child. To do that, the child process
		// must write an 8-byte value of zero and the parent a non-zero 8-byte
		// value.
		uint64_t parent = is_parent ? 1 : 0;
		write(fd, &parent, sizeof(parent));

		// At this point, the child will sleep and the parent will continue.
		// Perform any initialization you need to do. Once you are ready to switch
		// to the other process, you just need to read an 8-byte value out of the
		// fd.

		uint64_t value;
		// This will block until the other process calls read. This will mean that
		// only one process will be running at any given time.
		if (read(fd, &value, sizeof(value)) != sizeof(value)) {
		    // If the read fails, it means that the other process is dead. All future
				// read attempts will return immediately.
				exit(0);
		}

## License

The actual kernel module is GPL licensed to avoid license conflicts within the
kernel.  The language libraries are BSD.
