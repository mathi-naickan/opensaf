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
 * The methods implemented here are related to component error handling.
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

#include "jni_ais_amf.h"	// not really needed, but good for syntax checking!

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

// CLASS ais.amf.ErrorReporting
static jclass ClassErrorReporting = NULL;
static jfieldID FID_amfLibraryHandle = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.ErrorReporting
jboolean JNU_ErrorReporting_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ErrorReporting_initIDs_FromClass_OK(JNIEnv *jniEnv,
							jclass classAmfHandle);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.ErrorReporting
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_ErrorReporting_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ErrorReporting_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_ErrorReporting_initIDs_OK(...)\n");

	// get ErrorReporting class & create a global reference right away
	/*
	  ClassErrorReporting =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "ais/amf/ErrorReporting" )
	  ); */
	ClassErrorReporting = JNU_GetGlobalClassRef(jniEnv,
						    "org/opensaf/ais/amf/ErrorReportingImpl");
	if (ClassErrorReporting == NULL) {
		_TRACE2("NATIVE ERROR: ClassErrorReporting is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ErrorReporting_initIDs_FromClass_OK(jniEnv,
						       ClassErrorReporting);

}

/**************************************************************************
 * FUNCTION:      JNU_ErrorReporting_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ErrorReporting_initIDs_FromClass_OK(JNIEnv *jniEnv,
							jclass
							classErrorReporting)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ErrorReporting_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classErrorReporting,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ErrorReporting_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ErrorReportingImpl_reportComponentError
 * TYPE:      native method
 *  Class:     ais_amf_ErrorReporting
 *  Method:    reportComponentError
 *  Signature: (Ljava/lang/String;JLais/amf/RecommendedRecovery;J)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ErrorReportingImpl_reportComponentError(JNIEnv *jniEnv,
								 jobject
								 thisErrorReporting,
								 jstring
								 componentName,
								 jlong
								 detectionTime,
								 jobject
								 recommendedRecovery,
								 jlong
								 ntfIdentifier)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAmfRecommendedRecoveryT _saRecommendedRecovery;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisErrorReporting != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ErrorReportingImpl_reportComponentError(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisErrorReporting,
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
		return;		// EXIT POINT! Exception pending...
	}
	// recommended recovery
	if (recommendedRecovery == NULL) {
		JNU_throwNewByName(jniEnv,
				   "ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saRecommendedRecovery = (SaAmfRecommendedRecoveryT)
		(*jniEnv)->GetIntField(jniEnv, recommendedRecovery, FID_RR_value);

	// call saAmfComponentErrorReport
	_saStatus = saAmfComponentErrorReport(_saAmfHandle,
					      _saComponentNamePtr,
					      (SaTimeT)detectionTime,
					      _saRecommendedRecovery,
					      (SaNtfIdentifierT)ntfIdentifier);

	_TRACE2
		("NATIVE: saAmfComponentErrorReport(...) has returned with %d...\n",
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
		return;		// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ErrorReportingImpl_reportComponentError(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ErrorReportingImpl_clearComponentError
 * TYPE:      native method
 *  Class:     ais_amf_ErrorReporting
 *  Method:    clearComponentError
 *  Signature: (Ljava/lang/String;J)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ErrorReportingImpl_clearComponentError(JNIEnv *jniEnv,
								jobject
								thisErrorReporting,
								jstring
								componentName,
								jlong
								ntfIdentifier)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisErrorReporting != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ErrorReportingImpl_clearComponentError(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisErrorReporting,
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
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfComponentErrorClear
	_saStatus = saAmfComponentErrorClear(_saAmfHandle,
					     _saComponentNamePtr,
					     (SaNtfIdentifierT)ntfIdentifier);

	_TRACE2
		("NATIVE: saAmfComponentErrorClear(...) has returned with %d...\n",
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
		return;		// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ErrorReportingImpl_clearComponentError(...) returning normally\n");

}
