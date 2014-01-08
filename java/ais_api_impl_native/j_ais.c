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
 * This file defines native methods common for the Java AIS API
 * implementation.
 * TODO add a bit more on this...
 *
 *************************************************************************/

/**************************************************************************
 * Include files
 *************************************************************************/

#include <stdio.h>
#include <jni.h>
#include "j_utils.h"
#include "jni_ais.h"		// not really needed, but good for syntax checking!

/**************************************************************************
 * Constants
 *************************************************************************/

const char *AIS_ERR_LIBRARY_MSG =
	"An unexpected problem occurred: the library cannot be used anymore.";
const char *AIS_ERR_VERSION_MSG = "Incompatible version parameter.";
const char *AIS_ERR_INIT_MSG =
	"A required callback function has not been supplied during initialisation.";
const char *AIS_ERR_TIMEOUT_MSG =
	"A timeout occurred before the call could complete.";
const char *AIS_ERR_TRY_AGAIN_MSG = "The service currently cannot be provided.";
const char *AIS_ERR_INVALID_PARAM_MSG = "A parameter is not set correctly.";
const char *AIS_ERR_NO_MEMORY_MSG = "The service is out of memory.";
const char *AIS_ERR_BAD_HANDLE_MSG = "Handle is invalid.";
const char *AIS_ERR_BUSY_MSG = "Resource is already in use.";
const char *AIS_ERR_ACCESS_MSG = "Access is denied.";
const char *AIS_ERR_NOT_EXIST_MSG = "The entity does not exist.";
const char *AIS_ERR_NAME_TOO_LONG_MSG = "Name exceeds maximum length.";
const char *AIS_ERR_EXIST_MSG = "The entity already exists.";
const char *AIS_ERR_NO_SPACE_MSG =
	"The buffer provided by the component is too small.";
const char *AIS_ERR_INTERRUPT_MSG =
	"The request was canceled by a timeout or other interrupt.";
const char *AIS_ERR_NAME_NOT_FOUND_MSG = "The name could not be found.";
const char *AIS_ERR_NOT_SUPPORTED_MSG =
	"The requested function is not supported.";
const char *AIS_ERR_BAD_OPERATION_MSG =
	"The requested operation is not allowed.";
const char *AIS_ERR_FAILED_OPERATION_MSG = "The component reported an error.";
const char *AIS_ERR_NO_RESOURCES_MSG = "Not enough resources.";
const char *AIS_ERR_MESSAGE_ERROR_MSG = "A communication error occurred.";
const char *AIS_ERR_QUEUE_FULL_MSG =
	"Not enough space in the queue for the message.";
const char *AIS_ERR_QUEUE_NOT_AVAILABLE_MSG =
	"The destination queue is not available.";
const char *AIS_ERR_BAD_FLAGS_MSG = "The flags are invalid.";
const char *AIS_ERR_TOO_BIG_MSG = "A value is larger than permitted.";
const char *AIS_ERR_NO_SECTIONS_MSG = "No or no more sections.";
const char *AIS_ERR_UNAVAILABLE_MSG = "Service Unavailable.";

/**************************************************************************
 * Macros
 *************************************************************************/

/*
  #define SA_TRACK_CHANGES_MASK          ( SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) // 00000110
  #define SA_TRACK_CHANGES_INVALID_MASK  ( ~ SA_TRACK_CHANGES_MASK ) // 11111001
*/

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable definitions
 *************************************************************************/

// CLASS ais.Version
jclass ClassVersion = NULL;
jfieldID FID_releaseCode = NULL;
jfieldID FID_majorVersion = NULL;
jfieldID FID_minorVersion = NULL;
jmethodID CID_Version_constructor = NULL;

// ENUM ais.TrackFlags
static jclass EnumTrackFlags = NULL;
jfieldID FID_TF_value = NULL;

/* ENUM ais.CallbackResponse */
static jclass EnumCallbackResponse = NULL;
jfieldID FID_CR_value = NULL;
/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.Version
jboolean JNU_Version_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_Version_initIDs_FromClass_OK(JNIEnv *jniEnv,
						 jclass classVersion);

// ENUM ais.TrackFlags
jboolean JNU_TrackFlags_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_TrackFlags_initIDs_FromClass_OK(JNIEnv *jniEnv,
						    jclass enumTrackFlags);

/* ENUM ais.CallbackResponse */
jboolean JNU_CallbackResponse_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CallbackResponse_initIDs_FromClass_OK(JNIEnv *jniEnv,
							  jclass
							  enumCallbackResponse);

// MISC
/*
  jboolean JNU_TrackFlagsForChanges_OK(
  JNIEnv* jniEnv,
  jbyte trackFlags );
*/

/**************************************************************************
 * Function definitions
 *************************************************************************/

//********************************
// CLASS ais.Version
//********************************

/**************************************************************************
 * FUNCTION:      JNU_Version_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_Version_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_Version_initIDs_OK(...)\n");

	// get Version class & create a global reference right away
	/*
	  ClassVersion =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/Version" )
	  ); */
	ClassVersion = JNU_GetGlobalClassRef(jniEnv, "org/saforum/ais/Version");
	if (ClassVersion == NULL) {

		_TRACE2("NATIVE ERROR: ClassVersion is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_Version_initIDs_FromClass_OK(jniEnv, ClassVersion);
}

/**************************************************************************
 * FUNCTION:      JNU_Version_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_Version_initIDs_FromClass_OK(JNIEnv *jniEnv,
						 jclass classVersion)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_Version_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_releaseCode = (*jniEnv)->GetFieldID(jniEnv,
						classVersion,
						"releaseCode", "C");
	if (FID_releaseCode == NULL) {

		_TRACE2("NATIVE ERROR: FID_releaseCode is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_majorVersion = (*jniEnv)->GetFieldID(jniEnv,
						 classVersion,
						 "majorVersion", "S");
	if (FID_majorVersion == NULL) {

		_TRACE2("NATIVE: FID_majorVersion is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_minorVersion = (*jniEnv)->GetFieldID(jniEnv,
						 classVersion,
						 "minorVersion", "S");
	if (FID_minorVersion == NULL) {

		_TRACE2("NATIVE: FID_minorVersion is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	CID_Version_constructor = (*jniEnv)->GetMethodID(jniEnv,
							 classVersion,
							 "<init>", "(CSS)V");

	_TRACE2
		("NATIVE: JNU_Version_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// ENUM ais.TrackFlags
//********************************

/**************************************************************************
 * FUNCTION:      JNU_TrackFlags_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_TrackFlags_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_TrackFlags_initIDs_OK(...)\n");

	// get TrackFlags class & create a global reference right away
	/*
	  EnumTrackFlags =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/TrackFlags" )
	  ); */
	EnumTrackFlags =
		JNU_GetGlobalClassRef(jniEnv, "org/saforum/ais/TrackFlags");
	if (EnumTrackFlags == NULL) {

		_TRACE2("NATIVE ERROR: EnumTrackFlags is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_TrackFlags_initIDs_FromClass_OK(jniEnv, EnumTrackFlags);

}

/**************************************************************************
 * FUNCTION:      JNU_TrackFlags_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_TrackFlags_initIDs_FromClass_OK(JNIEnv *jniEnv,
						    jclass enumTrackFlags)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_TrackFlags_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_TF_value = (*jniEnv)->GetFieldID(jniEnv,
					     enumTrackFlags, "value", "I");
	if (FID_TF_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_TF_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_TrackFlags_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

// MISC

/**************************************************************************
 * FUNCTION:      JNU_TrackFlagsForChanges_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
jboolean JNU_TrackFlagsForChanges_OK(
    JNIEnv* jniEnv,
    jbyte trackFlags )
{

// BODY

    _TRACE2( "NATIVE: JNU_TrackFlagsForChanges_OK(...)\n" );

    // check track flags
    //  1: invalid flags have been set:
    if ( ( trackFlags & SA_TRACK_CHANGES_INVALID_MASK ) != 0 ) {
	JNU_throwNewByName( jniEnv,
			    "org/saforum/ais/AisBadFlagsException",
			    AIS_ERR_BAD_FLAGS_MSG );
	return JNI_FALSE; // EXIT POINT!!!
    }
    //  2: cannot set both flags:
    if ( trackFlags == SA_TRACK_CHANGES_MASK ) {
	JNU_throwNewByName( jniEnv,
			    "org/saforum/ais/AisBadFlagsException",
			    AIS_ERR_BAD_FLAGS_MSG );
	return JNI_FALSE; // EXIT POINT!!!
    }

    _TRACE2( "NATIVE: JNU_TrackFlagsForChanges_OK(...) returning true\n" );

    return JNI_TRUE;
}
*************************************************************************/

/**************************************************************************
 * FUNCTION:      JNU_CallbackResponse_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CallbackResponse_initIDs_OK(JNIEnv *jniEnv)
{

	_TRACE2("NATIVE: Executing JNU_CallbackResponse_initIDs_OK(...)\n");

	/* get CallbackResponse class & create a global reference right away */

	EnumCallbackResponse =
		JNU_GetGlobalClassRef(jniEnv, "org/saforum/ais/CallbackResponse");

	if (EnumCallbackResponse == NULL) {

		_TRACE2("NATIVE ERROR: EnumCallbackResponse is NULL\n");

		return JNI_FALSE;	/* EXIT POINT! */
	}

	/* get IDs */

	return JNU_CallbackResponse_initIDs_FromClass_OK(jniEnv,
							 EnumCallbackResponse);
}

/**************************************************************************
 * FUNCTION:      JNU_CallbackResponse_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CallbackResponse_initIDs_FromClass_OK(JNIEnv *jniEnv,
							  jclass
							  enumCallbackResponse)
{

	_TRACE2
		("NATIVE: Executing JNU_CallbackResponse_initIDs_FromClass_OK(..)\n");

	/* get field IDs */

	FID_CR_value = (*jniEnv)->GetFieldID(jniEnv,
					     enumCallbackResponse,
					     "value", "I");
	if (FID_CR_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_CR_value is NULL\n");

		return JNI_FALSE;	/* EXIT POINT!  */
	}

	_TRACE2
		("NATIVE: JNU_CallbackResponse_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}
