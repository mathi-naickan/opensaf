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
 * This file defines native methods for the Availability Management Framework.
 * The methods implemented here are related to CSI management.
 *************************************************************************/

/**************************************************************************
 * Include files
 *************************************************************************/

#include <stdio.h>
#include <assert.h>
#include "j_utilsPrint.h"

#include <string.h>
#include <stdlib.h>
#include <saAmf.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "j_ais_amf.h"
#include "j_ais_amf_libHandle.h"

#include "jni_ais_amf.h"

/**************************************************************************
 * Constants
 *************************************************************************/

/**************************************************************************
 * Macros
 *************************************************************************/

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable declarations
 *************************************************************************/

/**************************************************************************
 * Variable definitions
 *************************************************************************/

// CLASS ais.amf.CsiManager
static jclass ClassCsiManager = NULL;
static jfieldID FID_amfLibraryHandle = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.CsiManager
jboolean JNU_CsiManager_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiManager_initIDs_FromClass_OK(JNIEnv *jniEnv,
						    jclass classAmfHandle);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.CsiManager
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiManager_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiManager_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiManager_initIDs_OK(...)\n");

	// get CsiManager class & create a global reference right away
	/*
	  ClassCsiManager =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/opensaf/ais/amf/CsiManagerImpl" )
	  ); */
	ClassCsiManager = JNU_GetGlobalClassRef(jniEnv,
						"org/opensaf/ais/amf/CsiManagerImpl");
	if (ClassCsiManager == NULL) {

		_TRACE2("NATIVE ERROR: ClassCsiManager is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiManager_initIDs_FromClass_OK(jniEnv, ClassCsiManager);

}

/**************************************************************************
 * FUNCTION:      JNU_CsiManager_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiManager_initIDs_FromClass_OK(JNIEnv *jniEnv,
						    jclass classCsiManager)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiManager_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classCsiManager,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiManager_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_CsiManagerImpl_getHaState
 * TYPE:      native method
 *  Class:     ais_amf_CsiManager
 *  Method:    getHaState
 *  Signature: (Ljava/lang/String;Ljava/lang/String;)Lorg/saforum/ais/amf/HaState;
 *************************************************************************/
JNIEXPORT jobject JNICALL
Java_org_opensaf_ais_amf_CsiManagerImpl_getHaState(JNIEnv *jniEnv,
						   jobject thisCsiManager,
						   jstring componentName,
						   jstring csiName)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaNameT _saCsiName;
	SaNameT *_saCsiNamePtr = &_saCsiName;
	SaAmfHandleT _saAmfHandle;
	SaAmfHAStateT _saHaState = SA_AMF_HA_QUIESCING + 1;	// this is an invalid value...
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jobject _haState;

	// BODY

	assert(thisCsiManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_CsiManagerImpl_getHaState()\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisCsiManager,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// copy Java component name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					componentName,
					&_saComponentNamePtr) != JNI_TRUE) {
		return NULL;	// EXIT POINT! Exception pending...
	}

	U_printSaName("NATIVE: component name is ", _saComponentNamePtr);

	// copy Java CSI name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					csiName, &_saCsiNamePtr) != JNI_TRUE) {
		return NULL;	// EXIT POINT! Exception pending...
	}

	U_printSaName("NATIVE: CSI name is ", _saCsiNamePtr);
	_TRACE2
		("NATIVE: _saHaState before calling saAmfHAStateGet(...) is: %d\n",
		 _saHaState);

	// call saAmfHAStateGet
	_saStatus = saAmfHAStateGet(_saAmfHandle,
				    _saComponentNamePtr,
				    _saCsiNamePtr, &_saHaState);

	_TRACE2
		("NATIVE: _saHaState after saAmfHAStateGet(...) has returned is: %d\n",
		 _saHaState);
	_TRACE2("NATIVE: saAmfHAStateGet(...) has returned with %d...\n",
		_saStatus);

	// error handling
	if (_saStatus != SA_AIS_OK) {
		switch (_saStatus) {
		case SA_AIS_ERR_LIBRARY:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			break;
		case SA_AIS_ERR_TIMEOUT:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisTimeoutException",
					   AIS_ERR_TIMEOUT_MSG);
			break;
		case SA_AIS_ERR_TRY_AGAIN:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisTryAgainException",
					   AIS_ERR_TRY_AGAIN_MSG);
			break;
		case SA_AIS_ERR_BAD_HANDLE:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisInvalidParamException",
					   AIS_ERR_INVALID_PARAM_MSG);
			break;
		case SA_AIS_ERR_NO_MEMORY:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNoMemoryException",
					   AIS_ERR_NO_MEMORY_MSG);
			break;
		case SA_AIS_ERR_NO_RESOURCES:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNoResourcesException",
					   AIS_ERR_NO_RESOURCES_MSG);
			break;
		case SA_AIS_ERR_NOT_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNotExistException",
					   AIS_ERR_NOT_EXIST_MSG);
			break;
		default:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			break;
		}
		return NULL;	// EXIT POINT! Exception pending...
	}
	// return HA state
	_haState = JNU_HaState_getEnum(jniEnv, _saHaState);
	if (_haState == NULL) {
		// this should not happen here!

		_TRACE2("NATIVE ERROR: _haState is NULL\n");

		return NULL;	// AisLibraryException thrown already...
	}

	U_printSaAmfHAState
		("NATIVE: Java_org_opensaf_ais_amf_CsiManagerImpl_getHaState(...) returning normally with HA state: ",
		 _saHaState);

	return _haState;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_CsiManagerImpl_completedCsiQuiescing
 * TYPE:      native method
 *  Class:     ais_amf_CsiManager
 *  Method:    completedCsiQuiescing
 *  Signature: (JLorg/saforum/ais/AisStatus;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_CsiManagerImpl_completedCsiQuiescing(JNIEnv *jniEnv,
							      jobject
							      thisCsiManager,
							      jlong invocation,
							      jint error)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	SaAisErrorT _error;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisCsiManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_CsiManagerImpl_completedCsiQuiescing()\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisCsiManager,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);

	// convert AisStatus -> int
	// MODIFICATION: screwed up
	_error = (SaAisErrorT)error;

	// call saAmfCSIQuiescingComplete
	_saStatus = saAmfCSIQuiescingComplete(_saAmfHandle,
					      (SaInvocationT)invocation,
					      (SaAisErrorT)_error);

	_TRACE2
		("NATIVE: saAmfCSIQuiescingComplete(...) has returned with %d...\n",
		 _saStatus);

	// error handling
	if (_saStatus != SA_AIS_OK) {
		switch (_saStatus) {
		case SA_AIS_ERR_LIBRARY:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			break;
		case SA_AIS_ERR_TIMEOUT:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisTimeoutException",
					   AIS_ERR_TIMEOUT_MSG);
			break;
		case SA_AIS_ERR_TRY_AGAIN:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisTryAgainException",
					   AIS_ERR_TRY_AGAIN_MSG);
			break;
		case SA_AIS_ERR_BAD_HANDLE:
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisInvalidParamException",
					   AIS_ERR_INVALID_PARAM_MSG);
			break;
		case SA_AIS_ERR_NO_MEMORY:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNoMemoryException",
					   AIS_ERR_NO_MEMORY_MSG);
			break;
		case SA_AIS_ERR_NO_RESOURCES:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNoResourcesException",
					   AIS_ERR_NO_RESOURCES_MSG);
			break;
		default:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			break;
		}
		return;		// EXIT POINT!!!
	}
	// normal exit

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_CsiManagerImpl_completedCsiQuiescing() returning normally\n");

}
