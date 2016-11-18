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
#include <saAmf.h>
#include <jni.h>
#include "j_utils.h"
#include "j_utilsPrint.h"
#include "j_ais.h"
#include "j_ais_amf_libHandle.h"
#include "jni_ais_amf.h"

#include "j_ais_amf_pgManager.h"

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

// CLASS ais.amf.CsiDescriptor
static jclass ClassCsiDescriptor = NULL;
static jmethodID CID_CsiDescriptor_constructor = NULL;
static jfieldID FID_csiFlags = NULL;
static jfieldID FID_csiName = NULL;
static jfieldID FID_csiStateDescriptor = NULL;
static jfieldID FID_csiAttribute = NULL;

// CLASS ais.amf.CsiActiveDescriptor
static jclass ClassCsiActiveDescriptor = NULL;
static jmethodID CID_CsiActiveDescriptor_constructor = NULL;
static jfieldID FID_transitionDescriptor = NULL;
static jfieldID FID_AD_activeComponentName = NULL;

// ENUM ais.amf.CsiActiveDescriptor$TransitionDescriptor
static jclass EnumTransitionDescriptor = NULL;	// TODO could be static
static jfieldID FID_TD_NEW_ASSIGN = NULL;
static jfieldID FID_TD_QUIESCED = NULL;
static jfieldID FID_TD_NOT_QUIESCED = NULL;
static jfieldID FID_TD_STILL_ACTIVE = NULL;

// CLASS ais.amf.CsiStandbyDescriptor
static jclass ClassCsiStandbyDescriptor = NULL;
static jmethodID CID_CsiStandbyDescriptor_constructor = NULL;
static jfieldID FID_SD_activeComponentName = NULL;
static jfieldID FID_standbyRank = NULL;

// ENUM ais.amf.HaState  // TODO could be static
static jclass EnumHaState = NULL;
static jfieldID FID_ACTIVE = NULL;
static jfieldID FID_STANDBY = NULL;
static jfieldID FID_QUIESCED = NULL;
static jfieldID FID_QUIESCING = NULL;

// ENUM ais.amf.CsiDescriptor$CsiFlags
static jclass EnumCsiFlags = NULL;	// TODO could be static
static jfieldID FID_ADD_ONE = NULL;
static jfieldID FID_TARGET_ONE = NULL;
static jfieldID FID_TARGET_ALL = NULL;

// CLASS ais.amf.CsiAttribute
static jclass ClassCsiAttribute = NULL;
static jmethodID CID_CsiAttribute_constructor = NULL;
static jfieldID FID_CA_name = NULL;
static jfieldID FID_CA_value = NULL;

// ENUM ais.amf.RecommendedRecovery
static jclass EnumRecommendedRecovery = NULL;
jfieldID FID_RR_value = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CALLBACKS
void SaAmfHealthcheckCallback(SaInvocationT saInvocation,
			      const SaNameT *saCompNamePtr,
			      SaAmfHealthcheckKeyT *saHealthcheckKeyPtr);
void SaAmfCSISetCallback(SaInvocationT saInvocation,
			 const SaNameT *saCompNamePtr,
			 SaAmfHAStateT saHaState,
			 SaAmfCSIDescriptorT saCsiDescriptor);
void SaAmfCSIRemoveCallback(SaInvocationT saInvocation,
			    const SaNameT *saCompNamePtr,
			    const SaNameT *saCsiNamePtr,
			    SaAmfCSIFlagsT saCsiFlags);
void SaAmfComponentTerminateCallback(SaInvocationT saInvocation,
				     const SaNameT *saCompNamePtr);
void SaAmfProxiedComponentInstantiateCallback(SaInvocationT saInvocation,
					      const SaNameT
					      *saProxiedCompNamePtr);
void SaAmfProxiedComponentCleanupCallback(SaInvocationT saInvocation,
					  const SaNameT *saProxiedCompNamePtr);
void SaAmfProtectionGroupTrackCallback(const SaNameT *saCsiNamePtr,
				       SaAmfProtectionGroupNotificationBufferT
				       *saNotificationBufferPtr,
				       SaUint32T saNumberOfMembers,
				       SaAisErrorT saError);

// CLASS ais.amf.CsiDescriptor
jboolean JNU_CsiDescriptor_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
						       jclass
						       classCsiDescriptor);
jobject JNU_CsiDescriptor_create(JNIEnv *jniEnv,
				 const SaAmfCSIDescriptorT *saCsiDescriptorPtr,
				 SaAmfHAStateT saHaState);
static jboolean JNU_CsiDescriptor_set(JNIEnv *jniEnv, jobject sCsiDescriptor,
				      const SaAmfCSIDescriptorT
				      *saCsiDescriptorPtr,
				      SaAmfHAStateT saHaState);

// CLASS ais.amf.CsiActiveDescriptor
jboolean JNU_CsiActiveDescriptor_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiActiveDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     classCsiActiveDescriptor);
jobject JNU_CsiActiveDescriptor_create(JNIEnv *jniEnv,
				       const SaAmfCSIActiveDescriptorT
				       *saCsiActiveDescriptorPtr);
static jboolean JNU_CsiActiveDescriptor_set(JNIEnv *jniEnv,
					    jobject sCsiActiveDescriptor,
					    const SaAmfCSIActiveDescriptorT
					    *saCsiActiveDescriptorPtr);

// ENUM ais.amf.CsiActiveDescriptor$TransitionDescriptor
jboolean JNU_TransitionDescriptor_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_TransitionDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							      jclass
							      enumTransitionDescriptor);
static jobject JNU_TransitionDescriptor_getEnum(JNIEnv *jniEnv,
						SaAmfCSITransitionDescriptorT
						saTransitionDescriptor);

// CLASS ais.amf.CsiStandbyDescriptor
jboolean JNU_CsiStandbyDescriptor_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiStandbyDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							      jclass
							      classCsiStandbyDescriptor);
jobject JNU_CsiStandbyDescriptor_create(JNIEnv *jniEnv,
					const SaAmfCSIStandbyDescriptorT
					*saCsiStandbyDescriptorPtr);
static jboolean JNU_CsiStandbyDescriptor_set(JNIEnv *jniEnv,
					     jobject sCsiStandbyDescriptor,
					     const SaAmfCSIStandbyDescriptorT
					     *saCsiStandbyDescriptorPtr);

// ENUM ais.amf.HaState
jboolean JNU_HaState_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_HaState_initIDs_FromClass_OK(JNIEnv *jniEnv,
						 jclass enumHaState);
jobject JNU_HaState_getEnum(JNIEnv *jniEnv, SaAmfHAStateT saHaState);

// ENUM ais.amf.CsiDescriptor$CsiFlags
jboolean JNU_CsiFlags_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiFlags_initIDs_FromClass_OK(JNIEnv *jniEnv,
						  jclass enumCsiFlags);
static jobject JNU_CsiFlags_getEnum(JNIEnv *jniEnv, SaAmfCSIFlagsT saCsiFlags);

// CLASS ais.amf.CsiAttribute
jboolean JNU_CsiAttribute_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_CsiAttribute_initIDs_FromClass_OK(JNIEnv *jniEnv,
						      jclass classCsiAttribute);
static jobject JNU_CsiAttribute_create(JNIEnv *jniEnv,
				       const SaAmfCSIAttributeT
				       *saCsiAttributePtr);
static jboolean JNU_CsiAttribute_set(JNIEnv *jniEnv, jobject sCsiAttribute,
				     const SaAmfCSIAttributeT
				     *saCsiAttributePtr);

// ENUM ais.amf.RecommendedRecovery
jboolean JNU_RecommendedRecovery_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_RecommendedRecovery_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     enumRecommendedRecovery);

/**************************************************************************
 * Function definitions
 *************************************************************************/

// CALLBACKS

/**************************************************************************
 * FUNCTION:      SaAmfHealthcheckCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfHealthcheckCallback(SaInvocationT saInvocation,
			      const SaNameT *saCompNamePtr,
			      SaAmfHealthcheckKeyT *saHealthcheckKeyPtr)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _compNameString;
	jsize _hKeyLen;
	jbooleanArray _healthcheckKeyArray;

	// BODY
	_TRACE2("NATIVE CALLBACK: Executing SaAmfHealthcheckCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saCompNamePtr: ",
		      saCompNamePtr);
	// TODO healthcheck key printing

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// component name
	_compNameString = JNU_newStringFromSaNameT(_jniEnv, saCompNamePtr);
	if (_compNameString == NULL) {
		// TODO error handling
		_TRACE2("NATIVE CALLBACK ERROR: _compNameString is NULL  \n");
		return;		// OutOfMemoryError thrown already...
	}
	// healthcheck key
	_hKeyLen = (jsize)saHealthcheckKeyPtr->keyLen;

	assert(_hKeyLen <= SA_AMF_HEALTHCHECK_KEY_MAX);

	// create new []boolean object
	_healthcheckKeyArray = (jbyteArray)
		(*_jniEnv)->NewByteArray(_jniEnv, _hKeyLen);
	if (_healthcheckKeyArray == NULL) {
		_TRACE2
			("NATIVE CALLBACK ERROR: _healthcheckKeyArray is NULL\n");
		// TODO change exception??
		return;		// OutOfMemoryError thrown already...
	}
	// copy native content to Java _healthcheckKeyArray
	(*_jniEnv)->SetByteArrayRegion(_jniEnv,
				       _healthcheckKeyArray,
				       (jsize)0,
				       _hKeyLen,
				       (jbyte *)saHealthcheckKeyPtr->key);

	assert(((*_jniEnv)->ExceptionCheck(_jniEnv)) == JNI_FALSE);

	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeHealthcheckCallback,
					 (jlong)saInvocation,
					 _compNameString, _healthcheckKeyArray);
}

/**************************************************************************
 * FUNCTION:      SaAmfCSISetCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfCSISetCallback(SaInvocationT saInvocation,
			 const SaNameT *saCompNamePtr,
			 SaAmfHAStateT saHaState,
			 SaAmfCSIDescriptorT saCsiDescriptor)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _compNameString;
	jobject _haState;
	jobject _csiDescriptor;

	// BODY

	_TRACE2("NATIVE CALLBACK: Executing SaAmfCSISetCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saCompNamePtr: ",
		      saCompNamePtr);
	U_printSaAmfHAState("NATIVE CALLBACK: \tParameter saHaState: ",
			    saHaState);
	U_printSaAmfCSIDescriptor
		("NATIVE CALLBACK: \tParameter saCsiDescriptor: ", saHaState,
		 &saCsiDescriptor);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// component name
	_compNameString = JNU_newStringFromSaNameT(_jniEnv, saCompNamePtr);
	if (_compNameString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _compNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// HA state
	_haState = JNU_HaState_getEnum(_jniEnv, saHaState);
	if (_haState == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _haState is NULL  \n");

		return;		// exception thrown already...
	}
	// CSI descriptor
	_csiDescriptor = JNU_CsiDescriptor_create(_jniEnv,
						  &saCsiDescriptor, saHaState);
	if (_csiDescriptor == NULL) {

		_TRACE2("NATIVE CALLBACK ERROR: _csiDescriptor is NULL\n");

		return;		// OutOfMemoryError thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeSetCsiCallback,
					 (jlong)saInvocation,
					 _compNameString,
					 _haState, _csiDescriptor);
}

/**************************************************************************
 * FUNCTION:      SaAmfCSIRemoveCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfCSIRemoveCallback(SaInvocationT saInvocation,
			    const SaNameT *saCompNamePtr,
			    const SaNameT *saCsiNamePtr,
			    SaAmfCSIFlagsT saCsiFlags)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _compNameString;
	jstring _csiNameString;
	jobject _csiFlags;

	// BODY

	_TRACE2("NATIVE CALLBACK: Executing SaAmfCSIRemoveCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saCompNamePtr: ",
		      saCompNamePtr);
	U_printSaName("NATIVE CALLBACK: \tParameter saCsiNamePtr: ",
		      saCsiNamePtr);
	U_printSaAmfCSIFlags("NATIVE CALLBACK: \tParameter saCsiFlags: ",
			     saCsiFlags);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// component name
	_compNameString = JNU_newStringFromSaNameT(_jniEnv, saCompNamePtr);
	if (_compNameString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _compNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// CSI name
	_csiNameString = JNU_newStringFromSaNameT(_jniEnv, saCsiNamePtr);
	if (_csiNameString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _csiNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// CSI flags
	_csiFlags = JNU_CsiFlags_getEnum(_jniEnv, saCsiFlags);
	if (_csiFlags == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _csiFlags is NULL  \n");

		return;		// exception thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeRemoveCsiCallback,
					 (jlong)saInvocation,
					 _compNameString,
					 _csiNameString, _csiFlags);
}

/**************************************************************************
 * FUNCTION:      SaAmfComponentTerminateCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfComponentTerminateCallback(SaInvocationT saInvocation,
				     const SaNameT *saCompNamePtr)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _compNameString;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaAmfComponentTerminateCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saCompNamePtr: ",
		      saCompNamePtr);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// component name
	_compNameString = JNU_newStringFromSaNameT(_jniEnv, saCompNamePtr);
	if (_compNameString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _compNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeTerminateComponentCallback,
					 (jlong)saInvocation, _compNameString);
}

/**************************************************************************
 * FUNCTION:      SaAmfProxiedComponentInstantiateCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfProxiedComponentInstantiateCallback(SaInvocationT saInvocation,
					      const SaNameT
					      *saProxiedCompNamePtr)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _proxiedCompNameString;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaAmfProxiedComponentInstantiateCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saProxiedCompNamePtr: ",
		      saProxiedCompNamePtr);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// proxied component name
	_proxiedCompNameString = JNU_newStringFromSaNameT(_jniEnv,
							  saProxiedCompNamePtr);
	if (_proxiedCompNameString == NULL) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _proxiedCompNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeInstantiateProxiedComponentCallback,
					 (jlong)saInvocation,
					 _proxiedCompNameString);
}

/**************************************************************************
 * FUNCTION:      SaAmfProxiedComponentCleanupCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfProxiedComponentCleanupCallback(SaInvocationT saInvocation,
					  const SaNameT *saProxiedCompNamePtr)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _proxiedCompNameString;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaAmfProxiedComponentCleanupCallback(...)\n");
	_TRACE2("NATIVE CALLBACK: \tParameter saInvocation: %lu\n",
		(unsigned long)saInvocation);
	U_printSaName("NATIVE CALLBACK: \tParameter saProxiedCompNamePtr: ",
		      saProxiedCompNamePtr);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// proxied component name
	_proxiedCompNameString = JNU_newStringFromSaNameT(_jniEnv,
							  saProxiedCompNamePtr);
	if (_proxiedCompNameString == NULL) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _proxiedCompNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeCleanupProxiedComponentCallback,
					 (jlong)saInvocation,
					 _proxiedCompNameString);
}

/**************************************************************************
 * FUNCTION:      SaAmfProtectionGroupTrackCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaAmfProtectionGroupTrackCallback(const SaNameT *saCsiNamePtr,
				       SaAmfProtectionGroupNotificationBufferT
				       *saNotificationBufferPtr,
				       SaUint32T saNumberOfMembers,
				       SaAisErrorT saError)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jstring _csiNameString;
	jobject _sProtectionGroupNotificationBuffer;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaAmfProtectionGroupTrackCallback(...)\n");
	U_printSaName("NATIVE CALLBACK: \tParameter saCsiNamePtr: ",
		      saCsiNamePtr);
	// TODO notificATION BUFFER print
	_TRACE2("NATIVE CALLBACK: \tParameter saNumberOfMembers: %u\n",
		(unsigned int)saNumberOfMembers);
	_TRACE2("NATIVE CALLBACK: \tParameter saError: %d\n", saError);

	if (saError != SA_AIS_OK) {
		_TRACE2
			("WARNING NATIVE CALLBACK ERROR IN SERVICE LIBRARY: See SaAmfProtectionGroupTrackCallbackT error=%d For more info.\n",
			 saError);
		_TRACE2
			("WARNING NATIVE CALLBACK ERROR IN SERVICE LIBRARY: These errors are not handled in the native implementation yet.\n");
	}

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// CSI name
	_csiNameString = JNU_newStringFromSaNameT(_jniEnv, saCsiNamePtr);
	if (_csiNameString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE CALLBACK ERROR: _csiNameString is NULL  \n");

		return;		// OutOfMemoryError thrown already...
	}
	// notification buffer
	/*
	  _sClusterNotificationBuffer = JNU_ClusterNotificationBuffer_create(
	  _jniEnv,
	  saNotificationBufferPtr );
	  if ( _sClusterNotificationBuffer == NULL ) {

	  _TRACE2( "NATIVE CALLBACK ERROR: _sClusterNotificationBuffer is NULL\n" );

	  return; // OutOfMemoryError thrown already...
	  }
	*/

	// create PG nofication buffer
	if (saNotificationBufferPtr != NULL) {
		_sProtectionGroupNotificationBuffer =
			JNU_ProtectionGroupNotificationArray_create(_jniEnv,
								    saNotificationBufferPtr);
	} else {
		_sProtectionGroupNotificationBuffer = NULL;
	}

	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassAmfHandle,
					 MID_s_invokeTrackProtectionGroupCallback,
					 _csiNameString,
					 _sProtectionGroupNotificationBuffer,
					 (jint)saNumberOfMembers,
					 (jint)saError);
	_TRACE2
		("NATIVE CALLBACK: Return SaAmfProtectionGroupTrackCallback(...)\n");
}

// INTERNAL METHODS

//******************************************
// CLASS ais.amf.CsiDescriptor
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiDescriptor_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiDescriptor_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiDescriptor_initIDs_OK(...)\n");

	// get CsiDescriptor class & create a global reference right away
	/*
	  ClassCsiDescriptor =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/amf/CsiDescriptor" )
	  ); */
	ClassCsiDescriptor = JNU_GetGlobalClassRef(jniEnv,
						   "org/saforum/ais/amf/CsiDescriptor");
	if (ClassCsiDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: ClassCsiDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiDescriptor_initIDs_FromClass_OK(jniEnv,
						      ClassCsiDescriptor);
}

/**************************************************************************
 * FUNCTION:      JNU_CsiDescriptor_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
						       jclass
						       classCsiDescriptor)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_CsiDescriptor_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_CsiDescriptor_constructor = (*jniEnv)->GetMethodID(jniEnv,
							       classCsiDescriptor,
							       "<init>", "()V");
	if (CID_CsiDescriptor_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_CsiDescriptor_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_csiFlags = (*jniEnv)->GetFieldID(jniEnv,
					     classCsiDescriptor,
					     "csiFlags",
					     "Lorg/saforum/ais/amf/CsiDescriptor$CsiFlags;");
	if (FID_csiFlags == NULL) {

		_TRACE2("NATIVE ERROR: FID_csiFlags is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_csiName = (*jniEnv)->GetFieldID(jniEnv,
					    classCsiDescriptor,
					    "csiName", "Ljava/lang/String;");
	if (FID_csiName == NULL) {

		_TRACE2("NATIVE ERROR: FID_csiName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_csiStateDescriptor = (*jniEnv)->GetFieldID(jniEnv,
						       classCsiDescriptor,
						       "csiStateDescriptor",
						       "Lorg/saforum/ais/amf/CsiStateDescriptor;");
	if (FID_csiStateDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: FID_csiStateDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_csiAttribute = (*jniEnv)->GetFieldID(jniEnv,
						 classCsiDescriptor,
						 "csiAttribute",
						 "[Lorg/saforum/ais/amf/CsiAttribute;");
	if (FID_csiAttribute == NULL) {

		_TRACE2("NATIVE ERROR: FID_csiAttribute is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiDescriptor_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiDescriptor_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created CsiDescriptor object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_CsiDescriptor_create(JNIEnv *jniEnv,
				 const SaAmfCSIDescriptorT *saCsiDescriptorPtr,
				 SaAmfHAStateT saHaState)
{
	// VARIABLES
	// JNI
	jobject _sCsiDescriptor;

	// BODY

	assert(saCsiDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiDescriptor_create(...)\n");

	// create new CsiDescriptor object
	_sCsiDescriptor = (*jniEnv)->NewObject(jniEnv,
					       ClassCsiDescriptor,
					       CID_CsiDescriptor_constructor);
	if (_sCsiDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: _sCsiDescriptor is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set CsiDescriptor object
	if (JNU_CsiDescriptor_set(jniEnv,
				  _sCsiDescriptor,
				  saCsiDescriptorPtr, saHaState) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2("NATIVE: JNU_CsiDescriptor_create(...) returning normally\n");

	return _sCsiDescriptor;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiDescriptor_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiDescriptor_set(JNIEnv *jniEnv,
				      jobject sCsiDescriptor,
				      const SaAmfCSIDescriptorT
				      *saCsiDescriptorPtr,
				      SaAmfHAStateT saHaState)
{
	// VARIABLES
	unsigned int _idx;
	// JNI
	jobject _csiFlags = NULL;
	jstring _csiNameString = NULL;
	jobject _csiActiveDescriptor = NULL;
	jobject _csiStandbyDescriptor = NULL;
	jobjectArray _arraySCsiAttribute;
	jobject _sCsiAttribute;

	// BODY

	assert(sCsiDescriptor != NULL);
	assert(saCsiDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiDescriptor_set(...)\n");
	//U_printSaCsiDescriptor( "Input values of saCsiDescriptor: \n", saCsiDescriptorPtr );

	// csiFlags
	_csiFlags = JNU_CsiFlags_getEnum(jniEnv, saCsiDescriptorPtr->csiFlags);
	if (_csiFlags == NULL) {
		// TODO error handling
		return JNI_FALSE;	// exception thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiDescriptor, FID_csiFlags, _csiFlags);
	// csiName
	if (saCsiDescriptorPtr->csiFlags == SA_AMF_CSI_TARGET_ALL) {
		// When TARGET_ALL is set in csiFlags, csiName is not used
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor, FID_csiName, NULL);
	} else {
		_csiNameString = JNU_newStringFromSaNameT(jniEnv,
							  &(saCsiDescriptorPtr->
							    csiName));
		if (_csiNameString == NULL) {
			// TODO error handling
			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiName, _csiNameString);
	}
	// csiStateDescriptor

	//_TRACE2( "NATIVE: saHaState is %d\n", saHaState );

	switch (saHaState) {
	case SA_AMF_HA_ACTIVE:
		_csiActiveDescriptor = JNU_CsiActiveDescriptor_create(jniEnv,
								      &
								      (saCsiDescriptorPtr->
								       csiStateDescriptor.
								       activeDescriptor));
		if (_csiActiveDescriptor == NULL) {

			_TRACE2("NATIVE ERROR: _csiActiveDescriptor is NULL\n");

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiStateDescriptor,
					  _csiActiveDescriptor);
		break;
	case SA_AMF_HA_STANDBY:
		_csiStandbyDescriptor = JNU_CsiStandbyDescriptor_create(jniEnv,
									&
									(saCsiDescriptorPtr->
									 csiStateDescriptor.
									 standbyDescriptor));
		if (_csiStandbyDescriptor == NULL) {

			_TRACE2
				("NATIVE ERROR: _csiStandbyDescriptor is NULL\n");

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiStateDescriptor,
					  _csiStandbyDescriptor);
		break;
	case SA_AMF_HA_QUIESCED:
	case SA_AMF_HA_QUIESCING:
		// this is only necessary if the CsiDescriptor object is not newly created...
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiStateDescriptor, NULL);
		break;
	default:
		// this should not happen here!

		_TRACE2("NATIVE ERROR: saHaState is invalid: \n");
		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return JNI_FALSE;
	}
	// csiAttribute
	if (saCsiDescriptorPtr->csiFlags == SA_AMF_CSI_ADD_ONE) {
		// When ADD_ONE is set in csiFlags, csiAttribute refers to the attributes of
		// the newly assigned component service instance
		// create attribute array
		_arraySCsiAttribute = (*jniEnv)->NewObjectArray(jniEnv,
								saCsiDescriptorPtr->
								csiAttr.number,
								ClassCsiAttribute,
								NULL);
		if (_arraySCsiAttribute == NULL) {

			_TRACE2("NATIVE ERROR: _arraySCsiAttribute is NULL\n");

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		// assign array
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiAttribute,
					  _arraySCsiAttribute);
		// create & assign array elements
		for (_idx = 0; _idx < saCsiDescriptorPtr->csiAttr.number;
		     _idx++) {
			_sCsiAttribute =
				JNU_CsiAttribute_create(jniEnv,
							&(saCsiDescriptorPtr->
							  csiAttr.attr[_idx]));
			if (_sCsiAttribute == NULL) {

				_TRACE2
					("NATIVE ERROR: _sCsiAttribute[%d] is NULL\n",
					 _idx);

				return JNI_FALSE;	// OutOfMemoryError thrown already...
			}
			(*jniEnv)->SetObjectArrayElement(jniEnv,
							 _arraySCsiAttribute,
							 (jsize)_idx,
							 _sCsiAttribute);
			(*jniEnv)->DeleteLocalRef(jniEnv, _sCsiAttribute);
		}

	} else {
		(*jniEnv)->SetObjectField(jniEnv,
					  sCsiDescriptor,
					  FID_csiAttribute, NULL);
	}

	_TRACE2("NATIVE: JNU_CsiDescriptor_set(...) returning normally\n");

	return JNI_TRUE;
}

//******************************************
// CLASS ais.amf.CsiActiveDescriptor
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiActiveDescriptor_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiActiveDescriptor_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiActiveDescriptor_initIDs_OK(...)\n");

	// get CsiActiveDescriptor class & create a global reference right away
	ClassCsiActiveDescriptor =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/CsiActiveDescriptor")
			);
	if (ClassCsiActiveDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: ClassCsiActiveDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiActiveDescriptor_initIDs_FromClass_OK(jniEnv,
							    ClassCsiActiveDescriptor);
}

/**************************************************************************
 * FUNCTION:      JNU_CsiActiveDescriptor_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiActiveDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     classCsiActiveDescriptor)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_CsiActiveDescriptor_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_CsiActiveDescriptor_constructor = (*jniEnv)->GetMethodID(jniEnv,
								     classCsiActiveDescriptor,
								     "<init>",
								     "()V");
	if (CID_CsiActiveDescriptor_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_CsiActiveDescriptor_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_transitionDescriptor = (*jniEnv)->GetFieldID(jniEnv,
							 classCsiActiveDescriptor,
							 "transitionDescriptor",
							 "Lorg/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor;");
	if (FID_transitionDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: FID_transitionDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_AD_activeComponentName = (*jniEnv)->GetFieldID(jniEnv,
							   classCsiActiveDescriptor,
							   "activeComponentName",
							   "Ljava/lang/String;");
	if (FID_AD_activeComponentName == NULL) {

		_TRACE2("NATIVE ERROR: FID_AD_activeComponentName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiActiveDescriptor_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiActiveDescriptor_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created CsiActiveDescriptor object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_CsiActiveDescriptor_create(JNIEnv *jniEnv,
				       const SaAmfCSIActiveDescriptorT
				       *saCsiActiveDescriptorPtr)
{
	// VARIABLES
	// JNI
	jobject _sCsiActiveDescriptor;

	// BODY

	assert(saCsiActiveDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiActiveDescriptor_create(...)\n");

	// create new CsiActiveDescriptor object
	_sCsiActiveDescriptor = (*jniEnv)->NewObject(jniEnv,
						     ClassCsiActiveDescriptor,
						     CID_CsiActiveDescriptor_constructor);
	if (_sCsiActiveDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: _sCsiActiveDescriptor is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set CsiActiveDescriptor object
	if (JNU_CsiActiveDescriptor_set(jniEnv,
					_sCsiActiveDescriptor,
					saCsiActiveDescriptorPtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_CsiActiveDescriptor_create(...) returning normally\n");

	return _sCsiActiveDescriptor;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiActiveDescriptor_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiActiveDescriptor_set(JNIEnv *jniEnv,
					    jobject sCsiActiveDescriptor,
					    const SaAmfCSIActiveDescriptorT
					    *saCsiActiveDescriptorPtr)
{
	// VARIABLES
	// JNI
	jobject _transitionDescriptor = NULL;
	jstring _activeComponentNameString = NULL;

	// BODY

	assert(sCsiActiveDescriptor != NULL);
	assert(saCsiActiveDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiActiveDescriptor_set(...)\n");
	//U_printSaCsiActiveDescriptor( "Input values of saCsiActiveDescriptor: \n", saCsiActiveDescriptorPtr );

	// transition descriptor
	_transitionDescriptor = JNU_TransitionDescriptor_getEnum(jniEnv,
								 saCsiActiveDescriptorPtr->
								 transitionDescriptor);
	if (_transitionDescriptor == NULL) {
		// TODO error handling
		return JNI_FALSE;	// exception thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiActiveDescriptor,
				  FID_transitionDescriptor,
				  _transitionDescriptor);
	// active Component name
	_activeComponentNameString = JNU_newStringFromSaNameT(jniEnv,
							      &
							      (saCsiActiveDescriptorPtr->
							       activeCompName));
	if (_activeComponentNameString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiActiveDescriptor,
				  FID_AD_activeComponentName,
				  _activeComponentNameString);

	_TRACE2
		("NATIVE: JNU_CsiActiveDescriptor_set(...) returning normally\n");

	return JNI_TRUE;
}

//***************************************
// ENUM ais.amf.CsiActiveDescriptor$TransitionDescriptor
//***************************************

/**************************************************************************
 * FUNCTION:      JNU_TransitionDescriptor_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_TransitionDescriptor_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_TransitionDescriptor_initIDs_OK(...)\n");

	// get TransitionDescriptor class & create a global reference right away
	EnumTransitionDescriptor =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor")
			);
	if (EnumTransitionDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: EnumTransitionDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_TransitionDescriptor_initIDs_FromClass_OK(jniEnv,
							     EnumTransitionDescriptor);

}

/**************************************************************************
 * FUNCTION:      JNU_TransitionDescriptor_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_TransitionDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							      jclass
							      enumTransitionDescriptor)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_TransitionDescriptor_initIDs_FromClass_OK(...)\n");

	// get field IDs (the two predefined enum values)
	FID_TD_NEW_ASSIGN = (*jniEnv)->GetStaticFieldID(jniEnv,
							enumTransitionDescriptor,
							"NEW_ASSIGN",
							"Lorg/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor;");
	if (FID_TD_NEW_ASSIGN == NULL) {

		_TRACE2("NATIVE ERROR: FID_TD_NEW_ASSIGN is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_TD_QUIESCED = (*jniEnv)->GetStaticFieldID(jniEnv,
						      enumTransitionDescriptor,
						      "QUIESCED",
						      "Lorg/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor;");
	if (FID_TD_QUIESCED == NULL) {

		_TRACE2("NATIVE ERROR: FID_TD_QUIESCED is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_TD_NOT_QUIESCED = (*jniEnv)->GetStaticFieldID(jniEnv,
							  enumTransitionDescriptor,
							  "NOT_QUIESCED",
							  "Lorg/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor;");
	if (FID_TD_NOT_QUIESCED == NULL) {

		_TRACE2("NATIVE ERROR: FID_TD_NOT_QUIESCED is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_TD_STILL_ACTIVE = (*jniEnv)->GetStaticFieldID(jniEnv,
							  enumTransitionDescriptor,
							  "STILL_ACTIVE",
							  "Lorg/saforum/ais/amf/CsiActiveDescriptor$TransitionDescriptor;");
	if (FID_TD_STILL_ACTIVE == NULL) {

		_TRACE2("NATIVE ERROR: FID_TD_STILL_ACTIVE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_TransitionDescriptor_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_TransitionDescriptor_getEnum
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     a TransitionDescriptor object equivalent with the specified
 *                SaAmfCSITransitionDescriptorT parameter or
 *                NULL if an illegal SaAmfCSITransitionDescriptorT value was passed.
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_TransitionDescriptor_getEnum(JNIEnv *jniEnv,
						SaAmfCSITransitionDescriptorT
						saTransitionDescriptor)
{
	// VARIABLES
	jobject _transitionDescriptor = NULL;

	// BODY

	_TRACE2("NATIVE: Executing JNU_TransitionDescriptor_getEnum(...)\n");

	//_TRACE2( "NATIVE: saTransitionDescriptor is %d\n", saTransitionDescriptor );

	switch (saTransitionDescriptor) {
	case SA_AMF_CSI_NEW_ASSIGN:
		_transitionDescriptor = (*jniEnv)->GetStaticObjectField(jniEnv,
									EnumTransitionDescriptor,
									FID_TD_NEW_ASSIGN);

		//_TRACE2( "NATIVE: _transitionDescriptor(NEW_ASSIGN): %p\n", (void *) _transitionDescriptor );
		assert(_transitionDescriptor != NULL);

		break;
	case SA_AMF_CSI_QUIESCED:
		_transitionDescriptor = (*jniEnv)->GetStaticObjectField(jniEnv,
									EnumTransitionDescriptor,
									FID_TD_QUIESCED);

		//_TRACE2( "NATIVE: _transitionDescriptor(QUIESCED): %p\n", (void *) _transitionDescriptor );
		assert(_transitionDescriptor != NULL);

		break;
	case SA_AMF_CSI_NOT_QUIESCED:
		_transitionDescriptor = (*jniEnv)->GetStaticObjectField(jniEnv,
									EnumTransitionDescriptor,
									FID_TD_NOT_QUIESCED);

		//_TRACE2( "NATIVE: _transitionDescriptor(NOT_QUIESCED): %p\n", (void *) _transitionDescriptor );
		assert(_transitionDescriptor != NULL);

		break;
	case SA_AMF_CSI_STILL_ACTIVE:
		_transitionDescriptor = (*jniEnv)->GetStaticObjectField(jniEnv,
									EnumTransitionDescriptor,
									FID_TD_STILL_ACTIVE);

		//_TRACE2( "NATIVE: _transitionDescriptor(STILL_ACTIVE): %p\n", (void *) _transitionDescriptor );
		assert(_transitionDescriptor != NULL);

		break;
	default:
		// this should not happen here!

		_TRACE2("NATIVE ERROR: _transitionDescriptor(INVALID): \n");
		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return NULL;
	}

	_TRACE2
		("NATIVE: JNU_TransitionDescriptor_getEnum(...) returning normally\n");

	return _transitionDescriptor;
}

//******************************************
// CLASS ais.amf.CsiStandbyDescriptor
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiStandbyDescriptor_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiStandbyDescriptor_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiStandbyDescriptor_initIDs_OK(...)\n");

	// get CsiStandbyDescriptor class & create a global reference right away
	ClassCsiStandbyDescriptor =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/CsiStandbyDescriptor")
			);
	if (ClassCsiStandbyDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: ClassCsiStandbyDescriptor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiStandbyDescriptor_initIDs_FromClass_OK(jniEnv,
							     ClassCsiStandbyDescriptor);
}

/**************************************************************************
 * FUNCTION:      JNU_CsiStandbyDescriptor_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiStandbyDescriptor_initIDs_FromClass_OK(JNIEnv *jniEnv,
							      jclass
							      classCsiStandbyDescriptor)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_CsiStandbyDescriptor_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_CsiStandbyDescriptor_constructor = (*jniEnv)->GetMethodID(jniEnv,
								      classCsiStandbyDescriptor,
								      "<init>",
								      "()V");
	if (CID_CsiStandbyDescriptor_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_CsiStandbyDescriptor_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_SD_activeComponentName = (*jniEnv)->GetFieldID(jniEnv,
							   classCsiStandbyDescriptor,
							   "activeComponentName",
							   "Ljava/lang/String;");
	if (FID_SD_activeComponentName == NULL) {

		_TRACE2("NATIVE ERROR: FID_SD_activeComponentName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_standbyRank = (*jniEnv)->GetFieldID(jniEnv,
						classCsiStandbyDescriptor,
						"standbyRank", "I");
	if (FID_standbyRank == NULL) {

		_TRACE2("NATIVE ERROR: FID_standbyRank is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiStandbyDescriptor_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiStandbyDescriptor_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created CsiStandbyDescriptor object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_CsiStandbyDescriptor_create(JNIEnv *jniEnv,
					const SaAmfCSIStandbyDescriptorT
					*saCsiStandbyDescriptorPtr)
{
	// VARIABLES
	// JNI
	jobject _sCsiStandbyDescriptor;

	// BODY

	assert(saCsiStandbyDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiStandbyDescriptor_create(...)\n");

	// create new CsiStandbyDescriptor object
	_sCsiStandbyDescriptor = (*jniEnv)->NewObject(jniEnv,
						      ClassCsiStandbyDescriptor,
						      CID_CsiStandbyDescriptor_constructor);
	if (_sCsiStandbyDescriptor == NULL) {

		_TRACE2("NATIVE ERROR: _sCsiStandbyDescriptor is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set CsiStandbyDescriptor object
	if (JNU_CsiStandbyDescriptor_set(jniEnv,
					 _sCsiStandbyDescriptor,
					 saCsiStandbyDescriptorPtr) !=
	    JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_CsiStandbyDescriptor_create(...) returning normally\n");

	return _sCsiStandbyDescriptor;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiStandbyDescriptor_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiStandbyDescriptor_set(JNIEnv *jniEnv,
					     jobject sCsiStandbyDescriptor,
					     const SaAmfCSIStandbyDescriptorT
					     *saCsiStandbyDescriptorPtr)
{
	// VARIABLES
	// JNI
	jstring _activeComponentNameString = NULL;

	// BODY

	assert(sCsiStandbyDescriptor != NULL);
	assert(saCsiStandbyDescriptorPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiStandbyDescriptor_set(...)\n");
	//U_printSaCsiStandbyDescriptor( "Input values of saCsiStandbyDescriptor: \n", saCsiStandbyDescriptorPtr );

	// active Component name
	_activeComponentNameString = JNU_newStringFromSaNameT(jniEnv,
							      &
							      (saCsiStandbyDescriptorPtr->
							       activeCompName));
	if (_activeComponentNameString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiStandbyDescriptor,
				  FID_SD_activeComponentName,
				  _activeComponentNameString);
	// standby rank
	(*jniEnv)->SetIntField(jniEnv,
			       sCsiStandbyDescriptor,
			       FID_standbyRank,
			       saCsiStandbyDescriptorPtr->standbyRank);

	_TRACE2
		("NATIVE: JNU_CsiStandbyDescriptor_set(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// ENUM ais.amf.HaState
//********************************

/**************************************************************************
 * FUNCTION:      JNU_HaState_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_HaState_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_HaState_initIDs_OK(...)\n");

	// get HaState class & create a global reference right away
	EnumHaState =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/HaState")
			);
	if (EnumHaState == NULL) {

		_TRACE2("NATIVE ERROR: EnumHaState is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_HaState_initIDs_FromClass_OK(jniEnv, EnumHaState);

}

/**************************************************************************
 * FUNCTION:      JNU_HaState_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_HaState_initIDs_FromClass_OK(JNIEnv *jniEnv,
						 jclass enumHaState)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_HaState_initIDs_FromClass_OK(...)\n");

	// get field IDs (the two predefined enum values)
	FID_ACTIVE = (*jniEnv)->GetStaticFieldID(jniEnv,
						 enumHaState,
						 "ACTIVE",
						 "Lorg/saforum/ais/amf/HaState;");
	if (FID_ACTIVE == NULL) {

		_TRACE2("NATIVE ERROR: FID_ACTIVE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_STANDBY = (*jniEnv)->GetStaticFieldID(jniEnv,
						  enumHaState,
						  "STANDBY",
						  "Lorg/saforum/ais/amf/HaState;");
	if (FID_STANDBY == NULL) {

		_TRACE2("NATIVE ERROR: FID_STANDBY is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_QUIESCED = (*jniEnv)->GetStaticFieldID(jniEnv,
						   enumHaState,
						   "QUIESCED",
						   "Lorg/saforum/ais/amf/HaState;");
	if (FID_QUIESCED == NULL) {

		_TRACE2("NATIVE ERROR: FID_QUIESCED is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_QUIESCING = (*jniEnv)->GetStaticFieldID(jniEnv,
						    enumHaState,
						    "QUIESCING",
						    "Lorg/saforum/ais/amf/HaState;");
	if (FID_QUIESCING == NULL) {

		_TRACE2("NATIVE ERROR: FID_QUIESCING is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_HaState_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_HaState_getEnum
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     an HaState object equivalent with the specified SaAmfHAStateT parameter or
 *                NULL if an illegal SaAmfHAStateT value was passed.
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_HaState_getEnum(JNIEnv *jniEnv, SaAmfHAStateT saHaState)
{
	// VARIABLES
	jobject _haState = NULL;

	// BODY

	_TRACE2("NATIVE: Executing JNU_HaState_getEnum(...)\n");

	//_TRACE2( "NATIVE: saHaState is %d\n", saHaState );

	switch (saHaState) {
	case SA_AMF_HA_ACTIVE:
		_haState = (*jniEnv)->GetStaticObjectField(jniEnv,
							   EnumHaState,
							   FID_ACTIVE);

		//_TRACE2( "NATIVE: _haState(ACTIVE): %p\n", (void *) _haState );
		assert(_haState != NULL);

		break;
	case SA_AMF_HA_STANDBY:
		_haState = (*jniEnv)->GetStaticObjectField(jniEnv,
							   EnumHaState,
							   FID_STANDBY);

		//_TRACE2( "NATIVE: _haState(STANDBY): %p\n", (void *) _haState );
		assert(_haState != NULL);

		break;
	case SA_AMF_HA_QUIESCED:
		_haState = (*jniEnv)->GetStaticObjectField(jniEnv,
							   EnumHaState,
							   FID_QUIESCED);

		//_TRACE2( "NATIVE: _haState(QUIESCED): %p\n", (void *) _haState );
		assert(_haState != NULL);

		break;
	case SA_AMF_HA_QUIESCING:
		_haState = (*jniEnv)->GetStaticObjectField(jniEnv,
							   EnumHaState,
							   FID_QUIESCING);

		//_TRACE2( "NATIVE: _haState(QUIESCING): %p\n", (void *) _haState );
		assert(_haState != NULL);

		break;
	default:
		// this should not happen here!

		_TRACE2("NATIVE ERROR: _haState(INVALID): %d\n", saHaState);
		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return NULL;
	}

	_TRACE2("NATIVE: JNU_HaState_getEnum(...) returning normally\n");

	return _haState;
}

//***************************************
// ENUM ais.amf.CsiDescriptor$CsiFlags
//***************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiFlags_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiFlags_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiFlags_initIDs_OK(...)\n");

	// get CsiFlags class & create a global reference right away
	EnumCsiFlags =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/CsiDescriptor$CsiFlags")
			);
	if (EnumCsiFlags == NULL) {

		_TRACE2("NATIVE ERROR: EnumCsiFlags is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiFlags_initIDs_FromClass_OK(jniEnv, EnumCsiFlags);

}

/**************************************************************************
 * FUNCTION:      JNU_CsiFlags_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiFlags_initIDs_FromClass_OK(JNIEnv *jniEnv,
						  jclass enumCsiFlags)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiFlags_initIDs_FromClass_OK(...)\n");

	// get field IDs (the two predefined enum values)
	FID_ADD_ONE = (*jniEnv)->GetStaticFieldID(jniEnv,
						  enumCsiFlags,
						  "ADD_ONE",
						  "Lorg/saforum/ais/amf/CsiDescriptor$CsiFlags;");
	if (FID_ADD_ONE == NULL) {

		_TRACE2("NATIVE ERROR: FID_ADD_ONE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_TARGET_ONE = (*jniEnv)->GetStaticFieldID(jniEnv,
						     enumCsiFlags,
						     "TARGET_ONE",
						     "Lorg/saforum/ais/amf/CsiDescriptor$CsiFlags;");
	if (FID_TARGET_ONE == NULL) {

		_TRACE2("NATIVE ERROR: FID_TARGET_ONE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_TARGET_ALL = (*jniEnv)->GetStaticFieldID(jniEnv,
						     enumCsiFlags,
						     "TARGET_ALL",
						     "Lorg/saforum/ais/amf/CsiDescriptor$CsiFlags;");
	if (FID_TARGET_ALL == NULL) {

		_TRACE2("NATIVE ERROR: FID_TARGET_ALL is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiFlags_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiFlags_getEnum
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     an CsiFlags object equivalent with the specified SaAmfCSIFlagsT parameter or
 *                NULL if an illegal SaAmfCSIFlagsT value was passed.
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_CsiFlags_getEnum(JNIEnv *jniEnv, SaAmfCSIFlagsT saCsiFlags)
{
	// VARIABLES
	jobject _csiFlags = NULL;

	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiFlags_getEnum(...)\n");

	//_TRACE2( "NATIVE: csiFlags is %d\n", csiFlags );

	switch (saCsiFlags) {
	case SA_AMF_CSI_ADD_ONE:
		_csiFlags = (*jniEnv)->GetStaticObjectField(jniEnv,
							    EnumCsiFlags,
							    FID_ADD_ONE);

		//_TRACE2( "NATIVE: _csiFlags(ADD_ONE): %p\n", (void *) _csiFlags );
		assert(_csiFlags != NULL);

		break;
	case SA_AMF_CSI_TARGET_ONE:
		_csiFlags = (*jniEnv)->GetStaticObjectField(jniEnv,
							    EnumCsiFlags,
							    FID_TARGET_ONE);

		//_TRACE2( "NATIVE: _csiFlags(TARGET_ONE): %p\n", (void *) _csiFlags );
		assert(_csiFlags != NULL);

		break;
	case SA_AMF_CSI_TARGET_ALL:
		_csiFlags = (*jniEnv)->GetStaticObjectField(jniEnv,
							    EnumCsiFlags,
							    FID_TARGET_ALL);

		//_TRACE2( "NATIVE: _csiFlags(TARGET_ALL): %p\n", (void *) _csiFlags );
		assert(_csiFlags != NULL);

		break;
	default:
		// this should not happen here!

		_TRACE2("NATIVE ERROR: _csiFlags(INVALID): \n");
		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return NULL;
	}

	_TRACE2("NATIVE: JNU_CsiFlags_getEnum(...) returning normally\n");

	return _csiFlags;
}

//******************************************
// CLASS ais.amf.CsiAttribute
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_CsiAttribute_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_CsiAttribute_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_CsiAttribute_initIDs_OK(...)\n");

	// get CsiAttribute class & create a global reference right away
	ClassCsiAttribute =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/CsiAttribute")
			);
	if (ClassCsiAttribute == NULL) {

		_TRACE2("NATIVE ERROR: ClassCsiAttribute is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_CsiAttribute_initIDs_FromClass_OK(jniEnv, ClassCsiAttribute);
}

/**************************************************************************
 * FUNCTION:      JNU_CsiAttribute_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiAttribute_initIDs_FromClass_OK(JNIEnv *jniEnv,
						      jclass classCsiAttribute)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_CsiAttribute_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_CsiAttribute_constructor = (*jniEnv)->GetMethodID(jniEnv,
							      classCsiAttribute,
							      "<init>", "()V");
	if (CID_CsiAttribute_constructor == NULL) {

		_TRACE2("NATIVE ERROR: CID_CsiAttribute_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_CA_name = (*jniEnv)->GetFieldID(jniEnv,
					    classCsiAttribute,
					    "name", "Ljava/lang/String;");
	if (FID_CA_name == NULL) {

		_TRACE2("NATIVE ERROR: FID_name is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_CA_value = (*jniEnv)->GetFieldID(jniEnv,
					     classCsiAttribute,
					     "value", "Ljava/lang/String;");
	if (FID_CA_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_CsiAttribute_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiAttribute_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created CsiAttribute object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_CsiAttribute_create(JNIEnv *jniEnv,
				       const SaAmfCSIAttributeT
				       *saCsiAttributePtr)
{

	// VARIABLES
	// JNI
	jobject _sCsiAttribute;

	// BODY

	assert(saCsiAttributePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiAttribute_create(...)\n");
	//U_printSaCsiAttribute( "NATIVE: Input value of saCsiAttribute: \n", saCsiAttributePtr );

	// create new CsiAttribute object
	_sCsiAttribute = (*jniEnv)->NewObject(jniEnv,
					      ClassCsiAttribute,
					      CID_CsiAttribute_constructor);
	if (_sCsiAttribute == NULL) {

		_TRACE2("NATIVE ERROR: _sCsiAttribute is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set CsiAttribute object
	if (JNU_CsiAttribute_set(jniEnv,
				 _sCsiAttribute,
				 saCsiAttributePtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2("NATIVE: JNU_CsiAttribute_create(...) returning normally\n");

	return _sCsiAttribute;
}

/**************************************************************************
 * FUNCTION:      JNU_CsiAttribute_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_CsiAttribute_set(JNIEnv *jniEnv,
				     jobject sCsiAttribute,
				     const SaAmfCSIAttributeT
				     *saCsiAttributePtr)
{
	// VARIABLES
	// JNI
	jstring _nameString = NULL;
	jobject _valueString = NULL;

	// BODY

	assert(sCsiAttribute != NULL);
	assert(saCsiAttributePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_CsiAttribute_set(...)\n");
	//U_printSaCsiAttribute( "NATIVE: Input value of saCsiAttribute: \n", saCsiAttributePtr );

	// name
	_nameString = (*jniEnv)->NewStringUTF(jniEnv,
					      (char *)saCsiAttributePtr->
					      attrName);
	if (_nameString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiAttribute, FID_CA_name, _nameString);
	// value
	_valueString = (*jniEnv)->NewStringUTF(jniEnv,
					       (char *)saCsiAttributePtr->
					       attrValue);
	if (_valueString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sCsiAttribute, FID_CA_value, _valueString);

	_TRACE2("NATIVE: JNU_CsiAttribute_set(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// ENUM ais.amf.RecommendedRecovery
//********************************

/**************************************************************************
 * FUNCTION:      JNU_RecommendedRecovery_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_RecommendedRecovery_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_RecommendedRecovery_initIDs_OK(...)\n");

	// get RecommendedRecovery class & create a global reference right away
	EnumRecommendedRecovery =
		(*jniEnv)->NewGlobalRef(jniEnv,
					(*jniEnv)->FindClass(jniEnv,
							     "org/saforum/ais/amf/RecommendedRecovery")
			);
	if (EnumRecommendedRecovery == NULL) {

		_TRACE2("NATIVE ERROR: EnumRecommendedRecovery is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_RecommendedRecovery_initIDs_FromClass_OK(jniEnv,
							    EnumRecommendedRecovery);

}

/**************************************************************************
 * FUNCTION:      JNU_RecommendedRecovery_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_RecommendedRecovery_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     enumRecommendedRecovery)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_RecommendedRecovery_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_RR_value = (*jniEnv)->GetFieldID(jniEnv,
					     enumRecommendedRecovery,
					     "value", "I");
	if (FID_RR_value == NULL) {

		_TRACE2("NATIVE ERROR: FID_RR_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_RecommendedRecovery_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}
