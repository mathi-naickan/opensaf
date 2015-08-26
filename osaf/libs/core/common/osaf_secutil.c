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

/*
 * This file contains infrastructure for sending, receiving over a UNIX socket
 * This can be used by services to securely get credentials for a client.
 *
 * When initialized on the server side a tiny server thread is created
 * that listen to a named connection oriented UNIX socket. For each incoming
 * connection request it calls a user specified callback with file descriptor
 * and peer credentials.
 *
 * A typical use case is for a library to create the MDS socket and then
 * use the client side function to send the MDS address as a data message.
 * The client credentials are stored on the server side and verified
 * when needed.
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <pthread.h>
#include <osaf_poll.h>

#include "logtrace.h"
#include "include/osaf_secutil.h"

// singleton (one per process) callback
static client_auth_data_callback_t client_auth_data_callback;

/**
 * accepts a new client and get its credentials, call the handler
 * @param servsock
 */
static void handle_new_connection(int servsock)
{
	int client_fd;
	struct sockaddr_un remote;
	socklen_t addrlen = sizeof(remote);
	struct ucred client_cred;
	socklen_t ucred_length = sizeof(struct ucred);

	TRACE_ENTER();

	// a client wants to connect, accept
	if ((client_fd = accept(servsock, (struct sockaddr *)&remote, &addrlen)) == -1) {
		LOG_ER("accept failed - %s", strerror(errno));
		goto done;
	}

	// get client credentials
	if (getsockopt(client_fd, SOL_SOCKET, SO_PEERCRED, &client_cred, &ucred_length) == -1) {
		LOG_WA("could not get credentials from socket - %s", strerror(errno));
		goto done;
	}

	// wait a while for data to get available on socket
	struct pollfd fds;
	fds.fd = client_fd;
	fds.events = POLLIN;
	int timeout = 10000;  // TODO allow configuration?
	int res = osaf_poll(&fds, 1, timeout);

	if (res == 0) {
		TRACE_3("poll timeout %d", timeout);
		// timeout
		goto done;
	}

	// callback with file descriptor and credentials
	if (fds.revents & POLLIN) {
		client_auth_data_callback(client_fd, &client_cred);
	} else
		osafassert(0); // TODO

done:
	// the socket is short lived, close it
	if (client_fd > 0)
		close(client_fd);

	TRACE_LEAVE();
}

/**
 * Single time used helper function to the socket server
 */
static int server_sock_create(const char *pathname)
{
	int server_sockfd;
	socklen_t addrlen;
	struct sockaddr_un unaddr;

	if ((server_sockfd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		LOG_ER("%s: socket failed - %s", __FUNCTION__, strerror(errno));
		return -1;
	}

	int flags = fcntl(server_sockfd, F_GETFD, 0);
	if ((flags < 0) || (flags > 1)) {
		LOG_ER("%s: fcntl F_GETFD failed - %s", __FUNCTION__, strerror(errno));
		close(server_sockfd);
		return -1;
	}

	if (fcntl(server_sockfd, F_SETFD, flags | FD_CLOEXEC) == -1) {
		LOG_ER("%s: fcntl F_SETFD failed - %s", __FUNCTION__, strerror(errno));
		close(server_sockfd);
		return -1;
	}

	unlink(pathname);

	unaddr.sun_family = AF_UNIX;
	strcpy(unaddr.sun_path, pathname);
	addrlen = strlen(unaddr.sun_path) + sizeof(unaddr.sun_family);
	if (bind(server_sockfd, (struct sockaddr *)&unaddr, addrlen) == -1) {
		LOG_ER("%s: bind failed - %s", __FUNCTION__, strerror(errno));
		return -1;
	}

	/* Connecting to the socket object requires read/write permission. */
	if (chmod(pathname, 0777) == -1) {
		LOG_ER("%s: chmod failed - %s", __FUNCTION__, strerror(errno));
		return -1;
	}

	if (listen(server_sockfd, 5) == -1) {
		LOG_ER("%s: listen failed - %s", __FUNCTION__, strerror(errno));
		return -1;
	}

	return server_sockfd;
}

/**
 * main for authentication server
 */
static void *auth_server_main(void *_fd)
{
	int fd = *((int*) _fd);
	struct pollfd fds[1];

	TRACE_ENTER();

	free(_fd);

	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (1) {
		(void) osaf_poll(fds, 1, -1);

		if (fds[0].revents & POLLIN) {
			handle_new_connection(fd);
		}
	}

	osafassert(0);
	return 0;
}

/*************** public interface follows*************************** */

int osaf_auth_server_create(const char *pathname,
                            client_auth_data_callback_t callback)
{
	pthread_t thread;
	pthread_attr_t attr;

	TRACE_ENTER();

	// callback is singleton
	osafassert(client_auth_data_callback == NULL);

	// save provided callback for later use when clients connect
	client_auth_data_callback = callback;

	// create server socket
	int *fd = malloc(sizeof(int));
	*fd = server_sock_create(pathname);

	osafassert(pthread_attr_init(&attr) == 0);
	osafassert(pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) == 0);

	if (pthread_create(&thread, &attr, auth_server_main, fd) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		return -1;
	}

	osafassert(pthread_attr_destroy(&attr) == 0);

	TRACE_LEAVE();
	return 0;
}

/* used by server, logging is OK */
bool osaf_user_is_member_of_group(uid_t uid, const char *groupname)
{
	int res;
	char **member;
	struct group grp;
	struct group *result;
	char grpmembuf[16384]; // can use sysconf(_SC_GETPW_R_SIZE_MAX)

	// get group file entry with list of member user names
	res = getgrnam_r(groupname, &grp, grpmembuf, sizeof(grpmembuf), &result);
	if (res > 0) {
		LOG_ER("%s: get group file entry failed for '%s' - %s",
			__FUNCTION__, groupname, strerror(res));
		return false;
	}

	if (result == NULL) {
		LOG_ER("%s: group '%s' does not exist", __FUNCTION__, groupname);
		return false;
	}

	// get password file entry for user
	errno = 0;
	struct passwd *client_pwd = getpwuid(uid);
	if (client_pwd == NULL) {
		LOG_WA("%s: get password file entry failed for uid=%d - %s",
			__FUNCTION__, uid, strerror(errno));
		return false;
	}

	// check the primary group of the user
	if (client_pwd->pw_gid == grp.gr_gid)
		return true;

	/* loop list of usernames that are members of the group trying find a
	 * match with the specified user name */
	for (member = grp.gr_mem; *member != NULL; member++) {
		if (strcmp(client_pwd->pw_name, *member) == 0)
			return true;
	}

	return false;
}

/* used in libraries, do not log. Only trace */
int osaf_auth_server_connect(const char *path, const void *req_buf,
                             size_t req_size, void *resp_buf, size_t resp_size,
                             int timeout)
{
	int sock_fd, len;
	struct sockaddr_un remote;

	TRACE_ENTER();

	if ((sock_fd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
		TRACE_3("socket failed - %s", strerror(errno));
		return -errno;
	}

	remote.sun_family = AF_UNIX;
	strcpy(remote.sun_path, path);
	len = strlen(remote.sun_path) + sizeof(remote.sun_family);
	if (connect(sock_fd, (struct sockaddr *)&remote, len) == -1) {
		TRACE_3("connect to '%s' failed - %s", remote.sun_path, strerror(errno));
		len = -errno;
		goto done;
	}

	if (send(sock_fd, req_buf, req_size, 0) == -1) {
		TRACE_3("send failed - %s", strerror(errno));
		len = -errno;
		goto done;
	}

	struct pollfd fds;
	fds.fd = sock_fd;
	fds.events = POLLIN;

	int res = osaf_poll(&fds, 1, timeout);

	if (res == 0) {
		TRACE_3("poll timeout %d", timeout);
		len = 0;
		goto done;
	} else if (res == 1) {
		if (fds.revents & POLLIN) {
			if ((len = recv(sock_fd, resp_buf, resp_size, 0)) == -1) {
				TRACE_3("recv failed - %s", strerror(errno));
				len = -errno;
				goto done;
			}
		} else {
			TRACE_3("revents:%x", fds.revents);
			len = -1;
			goto done;
		}
	} else {
		// osaf_poll has failed (which it shouldn't)
		osafassert(0);
	}

done:
	(void)close(sock_fd);  // ignore error, server side normally closes it

	TRACE_LEAVE();
	return len;
}

int osaf_get_group_list(const uid_t uid, const gid_t gid, gid_t *groups, int *ngroups)
{
	int rc = -1;	// Initially set to error
	int size = 0;
	int max_groups = sysconf(_SC_NGROUPS_MAX);
	if (max_groups == -1){
		LOG_ER("Could not get NGROUPS_MAX, %s",strerror(errno));
	}

	if (!ngroups){
		LOG_ER("ngroups must not be NULL");
		return -1;
	}

	if (*ngroups > max_groups){
		LOG_ER("nGroups greater than NGROUPS_MAX");
		return -1;
	}

	if (groups){
		groups[size] = gid;
	}
	/* User always belong to at least one group */
	size++;

	struct passwd *pwd = getpwuid(uid);
	if (!pwd){
		LOG_ER("Could not getpwnam of user %d, %s", uid, strerror(errno));
		return -1;
	}

	/* Reset entry to beginning */
	errno = 0;
	setgrent();
	/* setgrent() sometimes returns ENOENT on UML
	 * Explicitly treats it as not an error */
	if (errno != 0 && errno != ENOENT) {
		LOG_NO("setgrent failed: %s", strerror(errno));
		return -1;
	}

	errno = 0;
	struct group *gr = getgrent();
	if (errno != 0) {
		LOG_NO("setgrent failed: %s", strerror(errno));
		goto done;
	}

	while (gr){
		if (gr->gr_gid == gid){
			errno = 0;
			gr = getgrent();
			if (errno != 0) {
				LOG_NO("setgrent failed: %s", strerror(errno));
				goto done;
			}
			continue;
		}

		int i = 0;
		for (i = 0; gr->gr_mem[i]; i++){
			if(strcmp(gr->gr_mem[i], pwd->pw_name) == 0){
				/* Found matched group */
				if ((groups) && (size < *ngroups)){
					groups[size] = gr->gr_gid;
				}
				size++;
				break;
			}
		}
		errno = 0;
		gr = getgrent();
		if (errno != 0) {
			LOG_NO("setgrent failed: %s", strerror(errno));
			goto done;
		}
	}

	// rc will be set to non-error value
	if (groups){
		*ngroups = (size < *ngroups)? size : *ngroups;
		rc = size;
	} else {
		*ngroups = size;
		rc = 0;
	}

done:
	endgrent();

	return rc;
}
