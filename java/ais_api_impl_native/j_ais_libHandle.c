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
 * Author(s): Nokia Siemens Networks, OptXware Research & Development LLC.
 */

/**************************************************************************
 * DESCRIPTION:
 * This file defines native methods for the Cluster Membership Service.
 * TODO add a bit more on this...
 *************************************************************************/

/**************************************************************************
 * Include files
 *************************************************************************/

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "j_utilsPrint.h"
#include <errno.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "jni_ais.h"
#include "osaf_poll.h"
#include "osaf_time.h"

/**************************************************************************
 * Constants
 *************************************************************************/

/**************************************************************************
 * Macros
 *************************************************************************/

#ifndef J_AIS_LIBHANDLE_SELECT_MAX
#define J_AIS_LIBHANDLE_SELECT_MAX 100
#endif

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable declarations
 *************************************************************************/

/**************************************************************************
 * Variable definitions
 *************************************************************************/

// CLASS ais.Handle
jclass ClassHandle = NULL;
static jfieldID FID_selectionObject = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

jboolean JNU_Handle_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_Handle_initIDs_FromClass_OK(JNIEnv *jniEnv,
						jclass classHandle);
static jint JNU_invokeSelect(JNIEnv *jniEnv,
			     jobjectArray libraryHandleArray,
			     struct timespec *lxTimeout_Ptr);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//********************************
// CLASS ais.Handle
//********************************

/**************************************************************************
 * FUNCTION:      JNU_Handle_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_Handle_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_Handle_initIDs_OK(...)\n");

	// get Handle class & create a global reference right away
	/*
	  ClassHandle =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/opensaf/ais/HandleImpl" )
	  ); */
	ClassHandle = JNU_GetGlobalClassRef(jniEnv,
					    "org/opensaf/ais/HandleImpl");
	if (ClassHandle == NULL) {

		_TRACE2("NATIVE ERROR: ClassHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_Handle_initIDs_FromClass_OK(jniEnv, ClassHandle);

}

/**************************************************************************
 * FUNCTION:      JNU_Handle_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_Handle_initIDs_FromClass_OK(JNIEnv *jniEnv,
						jclass classHandle)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_Handle_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_selectionObject = (*jniEnv)->GetFieldID(jniEnv,
						    classHandle,
						    "selectionObject", "J");
	if (FID_selectionObject == NULL) {

		_TRACE2("NATIVE ERROR: FID_selectionObject is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_Handle_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_HandleImpl_checkSelectionObject
 * TYPE:      native method
 *  Class:     ais_Handle
 *  Method:    checkSelectionObject
 *  Signature: (J)Z
 *************************************************************************/
JNIEXPORT jboolean JNICALL
Java_org_opensaf_ais_HandleImpl_checkSelectionObject(JNIEnv *jniEnv,
						     jobject thisLibraryHandle,
						     jlong timeout)
{

	// VARIABLES
	// ais
	SaSelectionObjectT _saSelectionObject;
	// linux
	struct pollfd _readFDs;
	struct timespec _lxTimeout;
	unsigned _pollStatus;

	// BODY
	assert(thisLibraryHandle != NULL);
	// TODO assert for timeout
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_HandleImpl_checkSelectionObject(...)\n");
	// get selection object
	_saSelectionObject = (SaSelectionObjectT)(*jniEnv)->GetLongField(jniEnv,
									 thisLibraryHandle,
									 FID_selectionObject);

	// call osaf_ppoll()
	_readFDs.fd = _saSelectionObject;
	_readFDs.events = POLLIN;
	osaf_nanos_to_timespec(timeout >= 0 ? timeout : 0, &_lxTimeout);

	_TRACE2("NATIVE: timout is { %ld seconds, %ld nanoseconds } \n",
		_lxTimeout.tv_sec, _lxTimeout.tv_nsec);

	_pollStatus = osaf_ppoll(&_readFDs, 1, &_lxTimeout, NULL);

	_TRACE2("NATIVE: osaf_ppoll(...) has returned with %u...\n",
		_pollStatus);

	if (_pollStatus == 1) {

		_TRACE2
			("NATIVE: Java_org_opensaf_ais_HandleImpl_checkSelectionObject() returning true\n");

		return JNI_TRUE;
	} else {

		_TRACE2
			("NATIVE: Java_org_opensaf_ais_HandleImpl_checkSelectionObject() returning false\n");

		return JNI_FALSE;
	}
}

/**************************************************************************
 * FUNCTION:  JJava_org_opensaf_ais_HandleImpl_s_1invokeSelect___3J
 * TYPE:      native method
 *  Class:     ais_Handle
 *  Method:    s_invokeSelect
 *  Signature: ([J)I
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_HandleImpl_s_1invokeSelect___3J(JNIEnv *jniEnv,
						     jclass
						     thisClassLibraryHandle,
						     jlongArray
						     selectionObjectArray)
{

	// BODY

	assert(thisClassLibraryHandle != NULL);
	//assert( thisClassLibraryHandle == ClassHandle );
	assert(selectionObjectArray != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_HandleImpl_s_1invokeSelect___3J(...)\n");

	// invoke select
	JNU_invokeSelect(jniEnv, selectionObjectArray, NULL);
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_HandleImpl_s_1invokeSelect___3JJ
 * TYPE:      native method
 *  Class:     ais_Handle
 *  Method:    s_invokeSelect
 *  Signature: ([JJ)I
 *************************************************************************/
JNIEXPORT jint JNICALL
Java_org_opensaf_ais_HandleImpl_s_1invokeSelect___3JJ(JNIEnv *jniEnv,
						      jclass
						      thisClassLibraryHandle,
						      jlongArray
						      selectionObjectArray,
						      jlong timeout)
{

	// VARIABLES
	// ais
	// linux
	struct timespec _lxTimeout;

	// BODY

	assert(thisClassLibraryHandle != NULL);
	//assert( thisClassLibraryHandle == ClassHandle );
	assert(selectionObjectArray != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_HandleImpl_s_1invokeSelect___3JJ(...)\n");

	// convert timeout
	osaf_nanos_to_timespec(timeout >= 0 ? timeout : 0, &_lxTimeout);

	// invoke select
	return JNU_invokeSelect(jniEnv, selectionObjectArray, &_lxTimeout);

}

/**************************************************************************
 * FUNCTION:  JNU_invokeSelect
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:
 * NOTE:
 *************************************************************************/
static jint JNU_invokeSelect(JNIEnv *jniEnv,
			     jlongArray selectionObjectArray,
			     struct timespec *lxTimeoutPtr)
{
	// VARIABLES
	// JNI
	jsize _len;
	unsigned int _idx;
	// ais
	SaSelectionObjectT _saSelectionObjectArray[J_AIS_LIBHANDLE_SELECT_MAX];
	// linux
	struct pollfd *_readFDs;
	unsigned _pollStatus;

	// BODY
	_TRACE2("NATIVE: Executing JNU_invokeSelect(...)\n");

	// copy  native selection objects from the Java array to a native array
	_len = (*jniEnv)->GetArrayLength(jniEnv, selectionObjectArray);

	assert(_len > 0);

	// check the number of selection objects
	if (_len > J_AIS_LIBHANDLE_SELECT_MAX) {

		_TRACE2
			("NATIVE ERROR: Too many selection objects. Recompile native library "
			 " with -DJ_AIS_LIBHANDLE_SELECT_MAX bigger than %d\n",
			 J_AIS_LIBHANDLE_SELECT_MAX);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   "Too many selection objects. Recompile native library "
				   " with higher -DJ_AIS_LIBHANDLE_SELECT_MAX.");
		return 0;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->GetLongArrayRegion(jniEnv,
				      selectionObjectArray,
				      (jsize)0,
				      _len, (jlong *)_saSelectionObjectArray);
	// call osaf_ppoll()
	_readFDs = malloc(_len * sizeof(struct pollfd));
	if (_readFDs == NULL) {
		_TRACE2
			("NATIVE ERROR : unable to allocate memory for osaf_ppoll()\n");
		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisNoMemoryException",
				   AIS_ERR_NO_MEMORY_MSG);
		return JNI_FALSE;	// EXIT POINT!!! return value is in fact ignored
	}

	for (_idx = 0; _idx < _len; _idx++) {
		// get selection object

		_TRACE2("NATIVE: %u. selection object is: %lu\n", _idx,
			(unsigned long)_saSelectionObjectArray[_idx]);

		_readFDs[_idx].fd = _saSelectionObjectArray[_idx];
		_readFDs[_idx].events = POLLIN;
	}

	_TRACE2("NATIVE: nfds is: %lu\n", (unsigned long)_len);
	_TRACE2("NATIVE: timout is { %ld seconds, %ld nanoseconds } \n",
		lxTimeoutPtr->tv_sec, lxTimeoutPtr->tv_nsec);

	_pollStatus = osaf_ppoll(_readFDs, _len, lxTimeoutPtr, NULL);

	_TRACE2("NATIVE: osaf_ppoll(...) has returned with %u...\n",
		_pollStatus);

	if (_pollStatus == 0) {

		_TRACE2("NATIVE: JNU_invokeSelect() returning 0\n");

		free(_readFDs);
		return 0;	// EXIT POINT!!!
	}
	// mark unselected selection objects
	for (_idx = 0; _idx < _len; _idx++) {
		if (_readFDs[_idx].revents == 0) {
			_saSelectionObjectArray[_idx] = 0xFFFFFFFFFFFFFFFFULL;
		}

		_TRACE2("NATIVE: %u. selection object is: %lu\n", _idx,
			(unsigned long)_saSelectionObjectArray[_idx]);

	}
	(*jniEnv)->SetLongArrayRegion(jniEnv,
				      selectionObjectArray,
				      (jsize)0,
				      _len, (jlong *)_saSelectionObjectArray);

	_TRACE2("NATIVE: JNU_invokeSelect() returning %u\n", _pollStatus);

	free(_readFDs);
	return _pollStatus;	// EXIT POINT!!!
}
