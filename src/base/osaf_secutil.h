/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2014 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Ericsson AB
 *
 */

#ifndef BASE_OSAF_SECUTIL_H_
#define BASE_OSAF_SECUTIL_H_

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stddef.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>

/* Support for building with LSB compiler */
#ifndef SO_PEERCRED
#define SO_PEERCRED     17
struct ucred {
  pid_t pid;
  uid_t uid;
  gid_t gid;
};
#endif

#ifdef  __cplusplus
extern "C" {
#endif

/**
 * Connects with server via UNIX stream socket and send and receive message
 * The function is typically used on the client side to register an MDS address
 * with a server.
 *
 * @param path file system path to server UNIX stream socket
 * @param req_buf ptr to buffer with data to be sent
 * @param req_size size of data to send
 * @param resp_buf ptr to buffer where to put received data
 * @param resp_size size of received data
 * @param timeout timeout to wait (same semantics as for poll timeout)
 *
 * @return on success length of received message, on failure negated errno,
 *                      on timeout 0.
 */
int osaf_auth_server_connect(const char *path,
                             const void *req_buf, size_t req_size, void *resp_buf, size_t resp_size,
                             int64_t timeout);

/**
 * Type for callback installed on server side
 *
 * @param fd socket file descriptor from where data is available
 * @param client_cred credentials for peer
 */
typedef void (*client_auth_data_callback_t)(int fd,
                                            const struct ucred *client_cred);

/**
 * Creates a new authentication server
 *
 * @param _pathname name to use for server socket
 * @param callback callback to call when a client has connect and data is
 *           available on socket
 */
int osaf_auth_server_create(const char *_pathname,
                            client_auth_data_callback_t callback);

/**
 * Checks if user represented by uid is member of group
 *
 * @param uid
 * @param groupname
 * @return true if member
 */
bool osaf_user_is_member_of_group(uid_t uid, const char *groupname);

/**
 * Get list of groups that a user belong to
 * There already is a function in LSB for this purpose (getgrouplist) but it is not standard.
 *
 * This function provides the same interface as getgrouplist(3) in glibc. Refer to the man
 * page for the interface description.
 */
int osaf_getgrouplist(const char* user, gid_t group, gid_t* groups, int* ngroups);

#ifdef  __cplusplus
}
#endif

#endif  // BASE_OSAF_SECUTIL_H_
