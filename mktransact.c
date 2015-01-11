#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "transact.h"

int main(int argc, char* argv[]) {
	if (argc < 2) {
		fprintf(stderr, "%s <path>\n", argv[0]);
		return 1;
	}

	if (mknod(argv[1], S_IFCHR | 0666, makedev(TRANSACT_MAJOR, 0)) != 0) {
		perror("mknod");
		return 1;
	}

	// Make the file be owned by the caller if it is created by using sudo.
	char* caller_uid = getenv("SUDO_UID");
	char* caller_gid = getenv("SUDO_GID");
	if (caller_uid && caller_gid) {
		if (chown(argv[1], atoi(caller_uid), atoi(caller_gid)) != 0) {
			perror("chown");
			return 1;
		}
	}

	return 0;
}
