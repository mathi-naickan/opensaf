/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): OptXware Research & Development LLC.
 */

#include <stdio.h>
#include <unistd.h>
#include <jni.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/select.h>
#include <assert.h>
#include "j_utilsPrint.h"
#include "j_utils.h"
#include "jni_ais.h"

/**
 * Opens a socket pair and returns the two descriptors.
 * @return The sink and source descriptors of the socket pair created.
 */
JNIEXPORT jintArray JNICALL
Java_org_opensaf_ais_SelectionObjectMediator_openSocketpair(JNIEnv *jniEnv,
							    jobject object)
{

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_SelectionObjectMediator_openSocketpair\n");

	int sockets[2];
	if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) < 0) {
		perror("socketpair()");
	}

	_TRACE2
		("NATIVE: Finishing Java_org_opensaf_ais_SelectionObjectMediator_openSocketpair\n");

	jintArray ia = (*jniEnv)->NewIntArray(jniEnv, 2);
	(*jniEnv)->SetIntArrayRegion(jniEnv, ia, 0, 2, sockets);
	return ia;
}

/**
 * Closes a socket pair.
 * @param sockets The sockets to be closed.
 */
JNIEXPORT void JNICALL
Java_org_opensaf_ais_SelectionObjectMediator_closeSocketpair(JNIEnv *jniEnv,
							     jobject object,
							     jintArray sockets)
{

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_SelectionObjectMediator_closeSocketpair\n");

	int *ia = (*jniEnv)->GetIntArrayElements(jniEnv, sockets, 0);
	int i;
	for (i = 0; i < sizeof(ia); i++) {
		if (close(ia[i]) < 0) {

			_TRACE2("NATIVE: Error closing fd\n");

		}
	}

	_TRACE2
		("NATIVE: Finishing Java_org_opensaf_ais_SelectionObjectMediator_closeSocketpair\n");

}

/**
 * Reads data from the given socket.
 * @return The data read.
 */
JNIEXPORT jint JNICALL
Java_org_opensaf_ais_SelectionObjectMediator_readSocketpair(JNIEnv *jniEnv,
							    jobject object,
							    jint source)
{

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_SelectionObjectMediator_readSocketpair\n");

	int buf[1];
	if (read(source, buf, sizeof(buf)) < 0) {
		perror("read()");
	}

	_TRACE2
		("NATIVE: Finishing Java_org_opensaf_ais_SelectionObjectMediator_readSocketpair\n");

	return buf[0];
}

/**
 * Writes data to a socket.
 * @param sink The socket where the data is to be written.
 * @param data The data to be written.
 */
JNIEXPORT void JNICALL
Java_org_opensaf_ais_SelectionObjectMediator_writeSocketpair(JNIEnv *jniEnv,
							     jobject object,
							     jint sink,
							     jint data)
{

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_SelectionObjectMediator_writeSocketpair\n");

	int buf[1];
	buf[0] = data;
	if (write(sink, buf, sizeof(buf)) < 0) {
		perror("write()");
	}

	_TRACE2
		("NATIVE: Finishing Java_org_opensaf_ais_SelectionObjectMediator_writeSocketpair\n");

}

/**
 * Calls select() on the given socket descriptors and returns an array of the modified ones.
 * In case of any error, it returns NULL.
 * @param sockets The socket descriptors on whom we call select().
 * @param socketSize Size of the array of socket descriptors.
 * @return An array of sockets returned by select().
 */
JNIEXPORT jintArray JNICALL
Java_org_opensaf_ais_SelectionObjectMediator_00024Worker_doSelect(JNIEnv
								  *jniEnv,
								  jobject
								  object,
								  jintArray
								  sockets,
								  jint
								  socketSize)
{
	jintArray selectedSockets = NULL;

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_SelectionObjectMediator_00024Worker_doSelect\n");

	int *fds = (*jniEnv)->GetIntArrayElements(jniEnv, sockets, 0);

	fd_set rfds;
	FD_ZERO(&rfds);
	int i;
	int maxfd = 0;
	for (i = 0; i < socketSize; i++) {
		FD_SET(fds[i], &rfds);
		if (maxfd < fds[i])
			maxfd = fds[i];
	}

	int numFds = select(maxfd + 1, &rfds, NULL, NULL, NULL);
	if (numFds < 1) {
		perror("select()");
		fflush(stderr);
	} else {
		selectedSockets = (*jniEnv)->NewIntArray(jniEnv, numFds);
		int a = 0;
		for (i = 0; i < socketSize; i++) {
			if (FD_ISSET(fds[i], &rfds)) {
				(*jniEnv)->SetIntArrayRegion(jniEnv,
							     selectedSockets, a,
							     1, &fds[i]);
				a++;
			}
		}
	}

	_TRACE2
		("NATIVE: Finishing Java_org_opensaf_ais_SelectionObjectMediator_00024Worker_doSelect\n");

	return selectedSockets;
}
