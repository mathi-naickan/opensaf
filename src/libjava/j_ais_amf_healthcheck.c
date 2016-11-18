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

// CLASS ais.amf.Healthcheck
static jclass ClassHealthcheck = NULL;
static jfieldID FID_amfLibraryHandle = NULL;

// ENUM ais.amf.Healthcheck$HealthcheckInvocation
static jclass EnumHealthcheckInvocation = NULL;
static jfieldID FID_HI_value = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.Healthcheck
jboolean JNU_Healthcheck_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_Healthcheck_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classAmfHandle);
static jboolean JNU_SaAmfHealthcheckKeyT_set(JNIEnv *jniEnv,
					     jbyteArray healthcheckKey,
					     SaAmfHealthcheckKeyT
					     *saHealthcheckKeyPtr);

// ENUM ais.amf.Healthcheck$HealthcheckInvocation
jboolean JNU_HealthcheckInvocation_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_HealthcheckInvocation_initIDs_FromClass_OK(JNIEnv *jniEnv,
							       jclass
							       EnumHealthcheckInvocation);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.Healthcheck
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_Healthcheck_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_Healthcheck_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_Healthcheck_initIDs_OK(...)\n");

	// get Healthcheck class & create a global reference right away
	/*
	  ClassHealthcheck =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "ais/amf/Healthcheck" )
	  ); */
	ClassHealthcheck = JNU_GetGlobalClassRef(jniEnv,
						 "org/opensaf/ais/amf/HealthcheckImpl");
	if (ClassHealthcheck == NULL) {

		_TRACE2("NATIVE ERROR: ClassHealthcheck is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_Healthcheck_initIDs_FromClass_OK(jniEnv, ClassHealthcheck);

}

/**************************************************************************
 * FUNCTION:      JNU_Healthcheck_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_Healthcheck_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classHealthcheck)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_Healthcheck_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classHealthcheck,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_Healthcheck_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_HealthcheckImpl_startHealthcheck
 * TYPE:      native method
 *  Class:     ais_amf_Healthcheck
 *  Method:    startHealthcheck
 *  Signature: (Ljava/lang/String;[BLorg/saforum/ais/amf/Healthcheck$HealthcheckInvocation;Lorg/saforum/ais/amf/RecommendedRecovery;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_HealthcheckImpl_startHealthcheck(JNIEnv *jniEnv,
							  jobject
							  thisHealthcheck,
							  jstring componentName,
							  jbyteArray
							  healthcheckKey,
							  jobject
							  invocationType,
							  jobject
							  recommendedRecovery)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfHealthcheckKeyT _saHealthcheckKey;
	SaAmfHealthcheckInvocationT _saInvocationType;
	SaAmfRecommendedRecoveryT _saRecommendedRecovery;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisHealthcheck != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_HealthcheckImpl_startHealthcheck(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisHealthcheck,
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
	// healthcheck key
	if (JNU_SaAmfHealthcheckKeyT_set(jniEnv,
					 healthcheckKey,
					 &_saHealthcheckKey) != JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// invocation type
	if (invocationType == NULL) {
		JNU_throwNewByName(jniEnv,
				   "ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saInvocationType = (SaAmfHealthcheckInvocationT)
		(*jniEnv)->GetIntField(jniEnv, invocationType, FID_HI_value);
	// recommended recovery
	if (recommendedRecovery == NULL) {
		JNU_throwNewByName(jniEnv,
				   "ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saRecommendedRecovery = (SaAmfRecommendedRecoveryT)
		(*jniEnv)->GetIntField(jniEnv, recommendedRecovery, FID_RR_value);

	// call saAmfHealthcheckStart
	_saStatus = saAmfHealthcheckStart(_saAmfHandle,
					  _saComponentNamePtr,
					  &_saHealthcheckKey,
					  _saInvocationType,
					  _saRecommendedRecovery);

	_TRACE2("NATIVE: saAmfHealthcheckStart(...) has returned with %d...\n",
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
		case SA_AIS_ERR_INIT:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisInitException",
					   AIS_ERR_INIT_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			// e.g. component name null
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
		("NATIVE: Java_org_opensaf_ais_amf_HealthcheckImpl_startHealthcheck(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_HealthcheckImpl_stopHealthcheck
 * TYPE:      native method
 *  Class:     ais_amf_Healthcheck
 *  Method:    stopHealthcheck
 *  Signature: (Ljava/lang/String;[B)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_HealthcheckImpl_stopHealthcheck(JNIEnv *jniEnv,
							 jobject
							 thisHealthcheck,
							 jstring componentName,
							 jbyteArray
							 healthcheckKey)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfHealthcheckKeyT _saHealthcheckKey;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisHealthcheck != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_HealthcheckImpl_stopHealthcheck(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisHealthcheck,
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
	// healthcheck key
	if (JNU_SaAmfHealthcheckKeyT_set(jniEnv,
					 healthcheckKey,
					 &_saHealthcheckKey) != JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfHealthcheckStop
	_saStatus = saAmfHealthcheckStop(_saAmfHandle,
					 _saComponentNamePtr,
					 &_saHealthcheckKey);

	_TRACE2("NATIVE: saAmfHealthcheckStop(...) has returned with %d...\n",
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
		("NATIVE: Java_org_opensaf_ais_amf_HealthcheckImpl_stopHealthcheck(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_HealthcheckImpl_confirmHealthcheck
 * TYPE:      native method
 *  Class:     ais_amf_Healthcheck
 *  Method:    confirmHealthcheck
 *  Signature: (Ljava/lang/String;[BI)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_HealthcheckImpl_confirmHealthcheck(JNIEnv *jniEnv,
							    jobject
							    thisHealthcheck,
							    jstring
							    componentName,
							    jbyteArray
							    healthcheckKey,
							    jint
							    healthcheckResult)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saComponentName;
	SaNameT *_saComponentNamePtr = &_saComponentName;
	SaAmfHealthcheckKeyT _saHealthcheckKey;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisHealthcheck != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_HealthcheckImpl_confirmHealthcheck(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisHealthcheck,
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
	// healthcheck key
	if (JNU_SaAmfHealthcheckKeyT_set(jniEnv,
					 healthcheckKey,
					 &_saHealthcheckKey) != JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfHealthcheckConfirm
	_saStatus = saAmfHealthcheckConfirm(_saAmfHandle,
					    _saComponentNamePtr,
					    &_saHealthcheckKey,
					    (SaAisErrorT)healthcheckResult);

	_TRACE2
		("NATIVE: saAmfHealthcheckConfirm(...) has returned with %d...\n",
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
		("NATIVE: Java_org_opensaf_ais_amf_HealthcheckImpl_confirmHealthcheck(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:      JNU_SaAmfHealthcheckKeyT_set
 * TYPE:          internal function
 * OVERVIEW:      checks and copies the content of the Java
 *                healthcheckKey object (a byte array) to
 *                the specified SaAmfHealthcheckKeyT parameter.
 * INTERFACE:
 *   parameters:
 * 		healthcheckKey [in]
 * 			- the source Java byte array.
 * 			If null, AisInvalidParamException is thrown.
 * 			Its length can be zero, however: in this case a zero length SaAmfHealthcheckKeyT is returned.
 *          If longer than SA_AMF_HEALTHCHECK_KEY_MAX, AisInvalidParamException is thrown.
 *      saHealthcheckKeyPtr [in/out]
 * 			- a pointer to the SaAmfHealthcheckKeyT structure into which
 * 			the content of the source Java byte array is copied. It must not be NULL.
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_SaAmfHealthcheckKeyT_set(JNIEnv *jniEnv,
					     jbyteArray healthcheckKey,
					     SaAmfHealthcheckKeyT
					     *saHealthcheckKeyPtr)
{
	// VARIABLES
	jsize _len;
	// BODY

	assert(saHealthcheckKeyPtr != NULL);
	//assert( healthcheckKey != NULL );
	_TRACE2("NATIVE: Executing JNU_SaAmfHealthcheckKeyT_set(...)\n");

	if (healthcheckKey == NULL) {

		_TRACE2("NATIVE: healthcheckKey is null!\n");

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return JNI_FALSE;
	}
	// length
	_len = (*jniEnv)->GetArrayLength(jniEnv, healthcheckKey);
	saHealthcheckKeyPtr->keyLen = (SaUint16T)_len;
	// id
	if (_len > SA_AMF_HEALTHCHECK_KEY_MAX) {

		_TRACE2("NATIVE ERROR: healthcheckKey is too long %d\n", _len);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return JNI_FALSE;
	}
	(*jniEnv)->GetByteArrayRegion(jniEnv,
				      healthcheckKey,
				      (jsize)0,
				      _len, (jbyte *)saHealthcheckKeyPtr->key);

	assert(((*jniEnv)->ExceptionCheck(jniEnv)) == JNI_FALSE);

	_TRACE2
		("NATIVE: JNU_SaAmfHealthcheckKeyT_set(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_SaAmfHealthcheckKeyT_set
 * TYPE:          internal function
 * OVERVIEW:      checks and copies the content of the Java
 *                healthcheckKey object (a byte array) to
 *                the specified SaAmfHealthcheckKeyT parameter.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
static jboolean JNU_SaAmfHealthcheckKeyT_set(
    JNIEnv* jniEnv,
    jbyteArray healthcheckKey,
    SaAmfHealthcheckKeyT* saHealthcheckKeyPtr )
{
// VARIABLES
    jsize _len;
    // BODY

    assert( saHealthcheckKeyPtr != NULL );
    assert( healthcheckKey != NULL );
    _TRACE2( "NATIVE: Executing JNU_SaAmfHealthcheckKeyT_set(...)\n" );

    // length
    _len = (*jniEnv)->GetArrayLength( jniEnv, healthcheckKey );
    saHealthcheckKeyPtr->keyLen = ( SaUint16T ) _len;
    // id
    if ( _len == 0 ) {

	_TRACE2( "NATIVE ERROR: the length of healthcheckKeys is ZERO!\n" );

	JNU_throwNewByName( jniEnv,
			    "org/saforum/ais/AisInvalidParamException",
			    AIS_ERR_INVALID_PARAM_MSG );
	return JNI_FALSE;
    }
    else {
	if ( _len > SA_AMF_HEALTHCHECK_KEY_MAX ) {

	    _TRACE2( "NATIVE ERROR: healthcheckKey is too long %d\n", _len );

	    JNU_throwNewByName( jniEnv,
				"org/saforum/ais/AisInvalidParamException",
				AIS_ERR_INVALID_PARAM_MSG );
	    return JNI_FALSE;
	}
	(*jniEnv)->GetByteArrayRegion( jniEnv,
				       healthcheckKey,
				       (jsize) 0,
				       _len,
				       (jbyte*) saHealthcheckKeyPtr->key );

	assert( ( (*jniEnv)->ExceptionCheck( jniEnv ) ) == JNI_FALSE );

    }

    _TRACE2( "NATIVE: JNU_SaAmfHealthcheckKeyT_set(...) returning normally\n" );

    return JNI_TRUE;
}
*************************************************************************/

//********************************
// ENUM ais.amf.Healthcheck$HealthcheckInvocation
//********************************

/**************************************************************************
 * FUNCTION:      JNU_HealthcheckInvocation_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_HealthcheckInvocation_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_HealthcheckInvocation_initIDs_OK(...)\n");

	// get HealthcheckInvocation class & create a global reference right away
	/*
	  EnumHealthcheckInvocation =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/amf/Healthcheck$HealthcheckInvocation" )
	  ); */
	EnumHealthcheckInvocation = JNU_GetGlobalClassRef(jniEnv,
							  "org/saforum/ais/amf/Healthcheck$HealthcheckInvocation");
	if (EnumHealthcheckInvocation == NULL) {

		_TRACE2("NATIVE ERROR: enum HealthcheckInvocation is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_HealthcheckInvocation_initIDs_FromClass_OK(jniEnv,
							      EnumHealthcheckInvocation);

}

/**************************************************************************
 * FUNCTION:      JNU_HealthcheckInvocation_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_HealthcheckInvocation_initIDs_FromClass_OK(JNIEnv *jniEnv,
							       jclass
							       enumHealthcheckInvocation)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_HealthcheckInvocation_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_HI_value = (*jniEnv)->GetFieldID(jniEnv,
					     enumHealthcheckInvocation,
					     "value", "I");
	if (FID_HI_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_HI_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_HealthcheckInvocation_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}
