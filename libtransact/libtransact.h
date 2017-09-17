/*
 * Copyright (c) 2017, The omegaUp Contributors
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of the omegaUp nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _LIBTRANSACT_H
#define _LIBTRANSACT_H

/*
 * All functions follow POSIX API semantics: they return 0 (or a valid pointer)
 * on success, and -1 (or a NULL pointer) on failure, and sets errno
 * appropriately.
 */

#include <stddef.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * An opaque structure that holds the state of a transact connection.
 */
struct transact_interface;

/*
 * Opens a transact connection with another process. The processes that calls
 * this with |is_parent| set to 1 will run unimpeded until a call to
 * transact_message_send() is made. The process that calls this with
 * |is_parent| set to 0 will wait until the other process has called
 * transact_message_send().
 *
 * After both processes call this function, they establish a peer relationship
 * and at most one can run at any given point in time.
 */
struct transact_interface* transact_interface_open(
    int is_parent,
    const char* transact_filename,
    const char* shm_filename,
    size_t shm_len);

/*
 * Closes the transact connection. The peer process will be notified of the
 * closure.
 */
void transact_interface_close(struct transact_interface* interface);

/*
 * A structure representing a transact message. Can be allocated statically in
 * the stack, or via transact_message_new().
 */
struct transact_message {
  struct transact_interface* interface;
  void* message;
  int method_id;
  int padding;

  char* data;
  char* end;
};

/*
 * Allocates a transact_message on the heap. Must be freed with
 * transact_message_free().
 */
struct transact_message* transact_message_new(void);

/*
 * Releases the memory of a transact_message allocated with
 * transact_message_new().
 */
void transact_message_free(struct transact_message* message);

/*
 * Associates the transact_message |message| with |interface|.
 */
int transact_message_init(struct transact_interface* interface,
                          struct transact_message* message);

/*
 * Allocates |len| bytes in the shared memory area and sets up |message|'s
 * |data| and |end| pointers so that they can be used to write the message.
 */
int transact_message_allocate(struct transact_message* message,
                              int id,
                              size_t len);

/*
 * Fills the |message| data structure in a way its |data| and |end| pointers
 * can be used for reading the message after control has been handed off from
 * the peer process.
 */
int transact_message_recv(struct transact_message* message);

/*
 * Sends |message| to the peer process. This also transfers control of
 * execution to the peer, so this process will be stopped until the peer calls
 * transact_message_send(). The contents of |message| will be invalidated after
 * this function returns.
 */
int transact_message_send(struct transact_message* message);

/*
 * Reads exactly |len| bytes from |message|.
 */
ssize_t transact_message_read(struct transact_message* message,
                              void* target,
                              size_t len);

/*
 * Makes |*target| point to a block of memory with exactly |len| bytes.
 */
ssize_t transact_message_read_array(struct transact_message* message,
                                    void** target,
                                    size_t len);

/*
 * Writes exactly |len| bytes into |message|.
 */
ssize_t transact_message_write(struct transact_message* message,
                               const void* source,
                               size_t len);

#ifdef __cplusplus
}
#endif

#endif  // _LIBTRANSACT_H
