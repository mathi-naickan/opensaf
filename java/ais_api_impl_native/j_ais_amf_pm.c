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
 * TODO add a bit more on this...
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
//#include "jni_ais.h"
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

// CLASS ais.amf.ProcessMonitoring
static jclass ClassProcessMonitoring = NULL;
static jfieldID FID_amfLibraryHandle = NULL;

// ENUM ais.amf.ProcessMonitoring$PmStopQualifier
static jclass EnumPmStopQualifier = NULL;
static jfieldID FID_PSQ_value = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.ProcessMonitoring
jboolean JNU_ProcessMonitoring_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ProcessMonitoring_initIDs_FromClass_OK(JNIEnv *jniEnv,
							   jclass
							   classAmfHandle);

// ENUM ais.amf.ProcessMonitoring$PmStopQualifier
jboolean JNU_PmStopQualifier_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_PmStopQualifier_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 EnumPmStopQualifier);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.ProcessMonitoring
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_ProcessMonitoring_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ProcessMonitoring_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_ProcessMonitoring_initIDs_OK(...)\n");

	// get ProcessMonitoring class & create a global reference right away
	ClassProcessMonitoring =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/opensaf/ais/amf/ProcessMonitoringImpl")
			);
	if (ClassProcessMonitoring == NULL) {

		_TRACE2("NATIVE ERROR: ClassProcessMonitoring is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ProcessMonitoring_initIDs_FromClass_OK(jniEnv,
							  ClassProcessMonitoring);

}

/**************************************************************************
 * FUNCTION:      JNU_ProcessMonitoring_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProcessMonitoring_initIDs_FromClass_OK(JNIEnv *jniEnv,
							   jclass
							   classProcessMonitoring)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProcessMonitoring_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classProcessMonitoring,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ProcessMonitoring_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProcessMonitoringImpl_startProcessMonitoring
 * TYPE:      native method
 *  Class:     ais_amf_ProcessMonitoring
 *  Method:    startProcessMonitoring
 *  Signature: (Ljava/lang/String;JIILorg/saforum/ais/amf/RecommendedRecovery;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ProcessMonitoringImpl_startProcessMonitoring(JNIEnv
								      *jniEnv,
								      jobject
								      thisProcessMonitoring,
								      jstring
								      componentName,
								      jlong
								      processId,
								      jint
								      descendentsTreeDepth,
								      jint
								      pmErrors,
								      jobject
								      recommendedRecovery)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfRecommendedRecoveryT _saRecommendedRecovery;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisProcessMonitoring != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProcessMonitoringImpl_startProcessMonitoring(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisProcessMonitoring,
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
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saRecommendedRecovery = (SaAmfRecommendedRecoveryT)
		(*jniEnv)->GetIntField(jniEnv, recommendedRecovery, FID_RR_value);

	// call saAmfPmStart
	_saStatus = saAmfPmStart(_saAmfHandle,
				 _saComponentNamePtr,
				 (SaUint64T)processId,
				 (SaInt32T)descendentsTreeDepth,
				 (SaAmfPmErrorsT)pmErrors,
				 _saRecommendedRecovery);

	_TRACE2("NATIVE: saAmfPmStart(...) has returned with %d...\n",
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
		("NATIVE: Java_org_opensaf_ais_amf_ProcessMonitoringImpl_startProcessMonitoring(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProcessMonitoringImpl_stopProcessMonitoring
 * TYPE:      native method
 *  Class:     ais_amf_ProcessMonitoring
 *  Method:    stopProcessMonitoring
 * Signature: (Ljava/lang/String;Lorg/opensaf/ais/amf/ProcessMonitoringImpl$PmStopQualifier;JI)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ProcessMonitoringImpl_stopProcessMonitoring(JNIEnv
								     *jniEnv,
								     jobject
								     thisProcessMonitoring,
								     jstring
								     componentName,
								     jobject
								     stopQualifier,
								     jlong
								     processId,
								     jint
								     pmErrors)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfPmStopQualifierT _saStopQualifier;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisProcessMonitoring != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProcessMonitoringImpl_stopProcessMonitoring(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisProcessMonitoring,
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
	// stop qualifier
	if (stopQualifier == NULL) {
		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saStopQualifier = (SaAmfPmStopQualifierT)
		(*jniEnv)->GetIntField(jniEnv, stopQualifier, FID_PSQ_value);

	// call saAmfPmStop
	_saStatus = saAmfPmStop(_saAmfHandle,
				_saComponentNamePtr,
				_saStopQualifier,
				(SaUint64T)processId, (SaAmfPmErrorsT)pmErrors);

	_TRACE2("NATIVE: saAmfPmStop(...) has returned with %d...\n",
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
		("NATIVE: Java_org_opensaf_ais_amf_ProcessMonitoringImpl_stopProcessMonitoring(...) returning normally\n");

}

//****************************************
// ENUM ais.amf.ProcessMonitoring$PmStopQualifier
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_PmStopQualifier_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_PmStopQualifier_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_PmStopQualifier_initIDs_OK(...)\n");

	// get PmStopQualifier class & create a global reference right away
	EnumPmStopQualifier =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/opensaf/ais/amf/ProcessMonitoringImpl$PmStopQualifier")
			);
	if (EnumPmStopQualifier == NULL) {

		_TRACE2("NATIVE ERROR: EnumPmStopQualifier is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_PmStopQualifier_initIDs_FromClass_OK(jniEnv,
							EnumPmStopQualifier);

}

/**************************************************************************
 * FUNCTION:      JNU_PmStopQualifier_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_PmStopQualifier_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 enumPmStopQualifier)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_PmStopQualifier_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_PSQ_value = (*jniEnv)->GetFieldID(jniEnv,
					      enumPmStopQualifier,
					      "value", "I");
	if (FID_PSQ_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_PSQ_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_PmStopQualifier_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}
