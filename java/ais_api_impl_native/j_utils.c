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
 * This file defines various utility functions for the Java AIS API
 * implementation.
 * TODO add a bit more on this...
 *
 *************************************************************************/

/**************************************************************************
 * Include files
 *************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/select.h>
#include <jni.h>
#include <saAis.h>
#include <saClm.h>
// TODO Attila Sragli: commented out until CKPT will be finished
//#include <saCkpt.h>
#include <saAmf.h>
#include "j_ais.h"
#include "j_ais_clm.h"
#include "j_ais_libHandle.h"
#include "j_ais_clm_libHandle.h"
#include "j_ais_clm_manager.h"
// TODO Attila Sragli: commented out until CKPT will be finished
//#include "j_ais_ckpt.h"
//#include "j_ais_ckpt_libHandle.h"
//#include "j_ais_ckpt_ckptHandle.h"
//#include "j_ais_ckpt_sectionHandle.h"
//#include "j_ais_ckpt_sectionItorHandle.h"
//#include "j_ais_ckpt_manager.h"
#include "j_ais_amf.h"
#include "j_ais_amf_libHandle.h"
#include "j_ais_amf_compRegistry.h"
#include "j_ais_amf_csiManager.h"
#include "j_ais_amf_errReporting.h"
#include "j_ais_amf_healthcheck.h"
#include "j_ais_amf_pgManager.h"
#include "j_ais_amf_pm.h"

#include "j_utils.h"

/* JNU_ThrowByName is declared in the header file jni_util.h, which is not
   distributed in OpenJDK. Is it a private function? */
JNIEXPORT void JNICALL
JNU_ThrowByName(JNIEnv *env, const char *name, const char *msg);

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
 * Variable definitions
 *************************************************************************/

JavaVM *cachedJVM = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

/**************************************************************************
 * Function definitions
 *************************************************************************/

/**************************************************************************
 * Class:     none
 * Method:    JNI_OnLoad
 * OVERVIEW:  This method is called when the library gets loaded.
 *            Its current implementation will cause the immediate loading of
 *            several java classes, as it will cache the necessary IDs
 *            (classes, furthermore method and field IDs) of these classes.
 *            The drawback of this solution is that it causes and "early"
 *            loading of the classes. potentially unnecessarily.
 *
 *            IMPLEMENTATION NOTE:
 *
 *            A potential different solution is to cache these IDs in the
 *            static initialisers of the classes, in which case the class
 *            is not loaded before it is needed. The drawback would be that
 *            it would "pollute" the Java code of these classes.
 *
 *            A third solution would be to check the ID variables upon
 *            use and do the caching if they contain NULL. The drawback is
 *            the added load of the check upon each use.
 *
 *            The second solution may be a viable alternative to the current
 *            implementation and the code is written so that this change
 *            could be made fairly easily: the initialisers could
 *            use the ...FromClass... version of the initializer functions.
 *************************************************************************/
JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved)
{
	// VARIABLES
	JNIEnv *_jniEnv;

	// BODY
	_TRACE2("NATIVE: Executing JNI_OnLoad(...)\n");

	//
	cachedJVM = jvm;

//   Workaround for warnings like "warning: dereferencing type-punned
//   pointer will break strict-aliasing rules" is to force an indirection
//   by casting to void*.
//   Solution found here:
//   http://mail.opensolaris.org/pipermail/tools-gcc/2005-August/000048.html
	if ((*jvm)->GetEnv(jvm, (void **)(void *)&_jniEnv, JNI_VERSION_1_2)) {
		return JNI_ERR;	// JNI version not supported
	}
	_TRACE2("NATIVE: Caching jvm pointer %p \n", (void *)cachedJVM);

	// Initialize ais package
	// ais.Version initialization
	if (!JNU_Version_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.TrackFlags initialization
	if (!JNU_TrackFlags_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	/* ais.CallbackResponse initialization */
	if (!JNU_CallbackResponse_initIDs_OK(_jniEnv)) {
		return JNI_ERR;
	}
	// ais.Handle initialization
	if (!JNU_Handle_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// Initialize ais.clm package
	// ais.clm.ClmHandle initialization
	if (!JNU_ClmHandle_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.ClusterMembershipManager initialization
	if (!JNU_ClusterMembershipManager_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.ClusterNode initialization
	if (!JNU_ClusterNode_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.NodeAddress initialization
	if (!JNU_NodeAddress_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.NodeAddressIPv4 initialization
	if (!JNU_NodeAddressIPv4_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.NodeAddressIPv6 initialization
	if (!JNU_NodeAddressIPv6_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.ClusterNotificationBuffer initialization
	if (!JNU_ClusterNotificationBuffer_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.clm.ClusterNotification initialization
	if (!JNU_ClusterNotification_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// TODO Attila Sragli: commented out until CKPT will be finished
	// Initialize ais.ckpt package
	// ais.ckpt.CkptHandle initialization
	/*if ( ! JNU_CkptLibraryHandle_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.CheckpointManager initialization
	  if ( ! JNU_CheckpointManager_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.CheckpointHandle initialization
	  if ( ! JNU_CheckpointHandle_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SectionHandle initialization
	  if ( ! JNU_SectionHandle_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SectionIterationHandle initialization
	  if ( ! JNU_SectionIterationHandle_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SectionIterationHandle$SectionSelectionFlags initialization
	  if ( ! JNU_SectionSelectionFlags_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SCheckpointCreationAttributes initialization
	  if ( ! JNU_SCheckpointCreationAttributes_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SCheckpointDescriptor initialization
	  if ( ! JNU_SCheckpointDescriptor_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SIOVectorElement itialization
	  if ( ! JNU_SIOVectorElement_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SSectionCreationAttributes initialization
	  if ( ! JNU_SSectionCreationAttributes_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SSectionDescriptor initialization
	  if ( ! JNU_SSectionDescriptor_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  }
	  // ais.ckpt.SSectionDescriptor$SectionState initialization
	  if ( ! JNU_SectionState_initIDs_OK( _jniEnv ) ) {
	  return JNI_ERR; // EXIT POINT! Exception pending...
	  } */

	// Initialize ais.amf package
	// ais.amf.AmfHandle initialization
	if (!JNU_AmfHandle_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ComponentRegistry initialization
	if (!JNU_ComponentRegistry_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiManager initialization
	if (!JNU_CsiManager_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ErrorReporting initialization
	if (!JNU_ErrorReporting_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.Healthcheck initialization
	if (!JNU_Healthcheck_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.Healthcheck$HealthcheckInvocation initialization
	if (!JNU_HealthcheckInvocation_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ProtectionGroupManager initialization
	if (!JNU_ProtectionGroupManager_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ProtectionGroupNotification initialization
	if (!JNU_ProtectionGroupNotification_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ProtectionGroupMember initialization
	if (!JNU_ProtectionGroupMember_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.ProtectionGroupNotification$ProtectionGroupChanges initialization
	if (!JNU_ProtectionGroupChanges_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	if (!JNU_ProcessMonitoring_initIDs_OK(_jniEnv)) {
		return JNI_ERR;
	}
	// ais.amf.CsiDescriptor initialization
	if (!JNU_CsiDescriptor_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiActiveDescriptor initialization
	if (!JNU_CsiActiveDescriptor_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiActiveDescriptor$TransitionDescriptor initialization
	if (!JNU_TransitionDescriptor_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiStandbyDescriptor initialization
	if (!JNU_CsiStandbyDescriptor_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.HaState initialization
	if (!JNU_HaState_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiDescriptor$CsiFlags initialization
	if (!JNU_CsiFlags_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.CsiAttribute initialization
	if (!JNU_CsiAttribute_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}
	// ais.amf.RecommendedRecovery initialization
	if (!JNU_RecommendedRecovery_initIDs_OK(_jniEnv)) {
		return JNI_ERR;	// EXIT POINT! Exception pending...
	}

	return JNI_VERSION_1_2;
}

/**************************************************************************
 * FUNCTION:      JNU_NewStringNative
 * TYPE:          internal function
 * OVERVIEW:      Creates a jstring from the given native string.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
jstring JNU_NewStringNative(JNIEnv *env, const char *str)
{
	// VARIABLES
	int len;

	// JNI
	jstring result;
	jbyteArray bytes = 0;

	// BODY
	if ((*env)->EnsureLocalCapacity(env, 2) < 0) {
		return NULL;	/* out of memory error */
	}

	len = strlen(str);
	bytes = (*env)->NewByteArray(env, len);

	if (bytes != NULL) {
		jclass Class_java_lang_String;
		jmethodID MID_String_init;

		Class_java_lang_String =
			(*env)->FindClass(env, "java/lang/String");
		MID_String_init =
			(*env)->GetMethodID(env, Class_java_lang_String, "<init>",
					    "([B)V");
		(*env)->SetByteArrayRegion(env, bytes, 0, len, (jbyte *)str);
		result = (*env)->NewObject(env, Class_java_lang_String,
					   MID_String_init, bytes);
		(*env)->DeleteLocalRef(env, bytes);
		return result;
	}
	/* else fall through */
	return NULL;
}

/**************************************************************************
 * FUNCTION:      JNU_GetStringNativeChars
 * TYPE:          internal function
 * OVERVIEW:      Returns a char pointer to the chars in the given jstring.
 *                The string must be freed by the caller.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
char *JNU_GetStringNativeChars(JNIEnv *env, jstring jstr)
{
	// VARIABLES
	char *result = 0;

	//JNI
	jbyteArray bytes = 0;
	jthrowable exc;
	jclass Class_java_lang_String;
	jmethodID MID_String_getBytes;

	//BODY
	if ((*env)->EnsureLocalCapacity(env, 2) < 0) {
		return 0;	/* out of memory error */
	}

	Class_java_lang_String = (*env)->FindClass(env, "java/lang/String");
	MID_String_getBytes =
		(*env)->GetMethodID(env, Class_java_lang_String, "getBytes",
				    "()[B");

	bytes = (*env)->CallObjectMethod(env, jstr, MID_String_getBytes);
	exc = (*env)->ExceptionOccurred(env);

	if (!exc) {
		jint len;

		len = (*env)->GetArrayLength(env, bytes);
		result = (char *)malloc(len + 1);

		if (result == 0) {
			JNU_ThrowByName(env, "java/lang/OutOfMemoryError", 0);
			(*env)->DeleteLocalRef(env, bytes);
			return 0;
		}

		(*env)->GetByteArrayRegion(env, bytes, 0, len, (jbyte *)result);
		result[len] = 0;	/* NULL-terminate */
	} else {
		(*env)->DeleteLocalRef(env, exc);
	}
	(*env)->DeleteLocalRef(env, bytes);
	return result;
}

/**************************************************************************
 * FUNCTION:      JNU_throwNewByName
 * TYPE:          internal function
 * OVERVIEW:      Throws a newly created Exception specified by its class
 *                  name. The constructor taking a String parameter
 *                  (a message describing the cause of the Exception)
 *                  is used.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void JNU_throwNewByName(JNIEnv *jniEnv, const char *className, const char *msg)
{
	// BODY
	_TRACE2("NATIVE: Executing JNU_ThrowNewByName(...)\n");
	jclass _class = (*jniEnv)->FindClass(jniEnv, className);
	if (_class != NULL) {
		(*jniEnv)->ThrowNew(jniEnv, _class, msg);
		_TRACE2("NATIVE: Throwing %s (with message: >>%s<<)\n",
			className, msg);
	}
	(*jniEnv)->DeleteLocalRef(jniEnv, _class);
}

/**************************************************************************
 * FUNCTION:      JNU_ExceptionDescribeDoNotClear
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 *************************************************************************/
void JNU_ExceptionDescribeDoNotClear(JNIEnv *jniEnv)
{
	// VARIABLES
	// jni
	jthrowable _exc;

	// BODY
	_exc = (*jniEnv)->ExceptionOccurred(jniEnv);
	if (_exc != NULL) {
		(*jniEnv)->ExceptionDescribe(jniEnv);
		(*jniEnv)->Throw(jniEnv, _exc);
	}
}

/**************************************************************************
 * FUNCTION:      JNU_newStringFromSaNameT
 * TYPE:          internal function
 * OVERVIEW:      Creates a new String object from the specified
 *                  SaNameT parameter.
 * INTERFACE:
 *   parameters:
 * 		saNamePtr [in]
 * 			The SaName to be stringified
 *   returns:     the newly created String
 * NOTE:
 *************************************************************************/
jstring JNU_newStringFromSaNameT(JNIEnv *jniEnv, const SaNameT *saNamePtr)
{
	// BODY
	_TRACE2("NATIVE: Executing JNU_newStringFromSaNameT(...)\n");

	SaUint8T _eValue[SA_MAX_NAME_LENGTH + 1];
	memcpy(_eValue, saNamePtr->value, SA_MAX_NAME_LENGTH);
	_eValue[saNamePtr->length] = 0x00;

	_TRACE2("NATIVE: Creating a new String from %s\n", _eValue);

	return (*jniEnv)->NewStringUTF(jniEnv, (char *)_eValue);
}

/**************************************************************************
 * FUNCTION:      JNU_copyFromStringToSaNameT
 * TYPE:          internal function
 * OVERVIEW:      copies the content of the specified String object to
 *                the specified SaNameT parameter.
 * INTERFACE:
 *   parameters:
 * 		fromStr [in]
 * 			- the source Java string.
 * 			If null, then *toSaNamePtrPtr will be set to NULL and JNI_TRUE is returned
 *          If longer than SA_MAX_NAME_LENGTH, AisInvalidParamException is thrown.
 *      toSaNamePtrPtr [in/out]
 * 			- a pointer to a pointer to the SaNameT structure into which
 * 			the content of the source Java string is copied.
 *
 *   returns:		JNI_TRUE if the copy operation succeeded.
 * 					If JNI_FALSE, then there is a pending Java exception.
 * NOTE:
 *************************************************************************/
jboolean JNU_copyFromStringToSaNameT(JNIEnv *jniEnv,
				     jstring fromStr, SaNameT **toSaNamePtrPtr)
{
	// VARIABLES
	jsize _strLen;

	// BODY
	//assert( fromStr != NULL );
	assert(toSaNamePtrPtr != NULL);
	assert(*toSaNamePtrPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_copyFromStringToSaNameT(...)\n");

	// check if string is not null
	if (fromStr == NULL) {
		*toSaNamePtrPtr = NULL;
		return JNI_TRUE;
	}
	// check if string is not too long
	_strLen = (*jniEnv)->GetStringUTFLength(jniEnv, fromStr);
	if (_strLen > SA_MAX_NAME_LENGTH) {
		_TRACE2("NATIVE ERROR: fromStr is too long %d\n", _strLen);
		JNU_throwNewByName(jniEnv,
				   "ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return JNI_FALSE;
	}
	// copy string content
	(*toSaNamePtrPtr)->length = (SaUint16T)_strLen;
	(*jniEnv)->GetStringUTFRegion(jniEnv,
				      fromStr,
				      (jsize)0,
				      (*jniEnv)->GetStringLength(jniEnv,
								 fromStr),
				      (char *)(*toSaNamePtrPtr)->value);
	assert((*jniEnv)->ExceptionCheck(jniEnv) == JNI_FALSE);
	_TRACE2
		("NATIVE: JNU_copyFromStringToSaNameT(...) returning normally\n");
	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_copyFromStringToSaNameT_NotNull
 * TYPE:          internal function
 * OVERVIEW:      copies the content of the specified String object to
 *                the specified SaNameT parameter.
 * INTERFACE:
 *   parameters:
 * 		fromStr [in]
 * 			- the source Java string. It must not be null.
 *          If longer than SA_MAX_NAME_LENGTH, AisInvalidParamException is thrown.
 *      toSaNamePtr [in/out]
 * 			- a pointer to the SaNameT structure into which
 * 			the content of the source Java string is copied.
 *
 *   returns:		JNI_TRUE if the copy operation succeeded.
 * 					If JNI_FALSE, then there is a pending Java exception.
 * NOTE:
 *************************************************************************/
jboolean JNU_copyFromStringToSaNameT_NotNull(JNIEnv *jniEnv,
					     jstring fromStr,
					     SaNameT *toSaNamePtr)
{
	// VARIABLES
	jsize _strLen;

	// BODY
	assert(fromStr != NULL);
	assert(toSaNamePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_copyFromStringToSaNameT_NotNull(...)\n");
	// check if string is not too long
	_strLen = (*jniEnv)->GetStringUTFLength(jniEnv, fromStr);
	if (_strLen > SA_MAX_NAME_LENGTH) {
		_TRACE2("NATIVE ERROR: fromStr is too long %d\n", _strLen);
		JNU_throwNewByName(jniEnv,
				   "ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return JNI_FALSE;
	}
	// copy string content
	toSaNamePtr->length = (SaUint16T)_strLen;
	(*jniEnv)->GetStringUTFRegion(jniEnv,
				      fromStr,
				      (jsize)0,
				      (*jniEnv)->GetStringLength(jniEnv,
								 fromStr),
				      (char *)toSaNamePtr->value);
	assert((*jniEnv)->ExceptionCheck(jniEnv) == JNI_FALSE);
	_TRACE2
		("NATIVE: JNU_copyFromStringToSaNameT(...) returning normally\n");
	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_newStringFromSaNodeAddressT
 * TYPE:          internal function
 * OVERVIEW:      Creates a new String object from the specified
 *                  SaNodeAddressT parameter.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the newly created String
 * NOTE:
 *************************************************************************/
jstring JNU_newStringFromSaNodeAddressT(JNIEnv *jniEnv,
					const SaClmNodeAddressT
					*saClmNodeAddressPtr)
{
	// BODY
	assert(saClmNodeAddressPtr != NULL);
	assert(saClmNodeAddressPtr->length <= SA_CLM_MAX_ADDRESS_LENGTH);

	_TRACE2
		("NATIVE: Executing JNU_newStringFromSaNodeAddressT(...) MODIFIED \n");
	_TRACE2("NATIVE: saClmNodeAddress = %p ; saClmNodeAddress.length: %d\n",
		saClmNodeAddressPtr, saClmNodeAddressPtr->length);

	SaUint8T _eValue[SA_CLM_MAX_ADDRESS_LENGTH + 1];
	memcpy(_eValue, saClmNodeAddressPtr->value,
	       saClmNodeAddressPtr->length);
	_eValue[saClmNodeAddressPtr->length] = 0x00;

	_TRACE2("NATIVE: Creating a new String from %s\n", _eValue);

	return (*jniEnv)->NewStringUTF(jniEnv, (char *)_eValue);
}

/**************************************************************************
 * FUNCTION:      JNU_jByteArray_link
 * TYPE:          internal function
 * OVERVIEW:      links the content of the Java
 *                byte array object to
 *                the specified native buffer.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: -> If JNI_FALSE is returned, then an exception is already pending!
 *       -> A zero length java byte array is allowed, in which case
 *          the native buffer will be set to NULL
 *************************************************************************/
jboolean JNU_jByteArray_link(JNIEnv *jniEnv,
			     jbyteArray byteArray,
			     SaSizeT *saSizePtr, void **saBufferPtr)
{
	// VARIABLES
	jsize _len;
	// BODY
#ifndef NDEBUG
	assert(byteArray != NULL);
	assert(saSizePtr != NULL);
	assert(saBufferPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_jByteArray_link(...)\n");
#endif				// NDEBUG
	// length
	_len = (*jniEnv)->GetArrayLength(jniEnv, byteArray);
	*saSizePtr = (SaSizeT)_len;
	// id
	if (_len == 0) {
		// we allow byteArray length to be 0 at this level!
#ifndef NDEBUG
		_TRACE2("NATIVE WARNING: _len is NULL\n");
#endif				// NDEBUG
		*saBufferPtr = NULL;
	} else {
		*saBufferPtr = (void *)
			(*jniEnv)->GetByteArrayElements(jniEnv, byteArray, NULL);
		if (*saBufferPtr == NULL) {
#ifndef NDEBUG
			_TRACE2("NATIVE ERROR: *saBufferPtr is NULL\n");
#endif				// NDEBUG
			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
	}

#ifndef NDEBUG
	_TRACE2("NATIVE: JNU_jByteArray_link(...) returning normally\n");
#endif				// NDEBUG
	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_jByteArray_unlink
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 *************************************************************************/
void JNU_jByteArray_unlink(JNIEnv *jniEnv, jbyteArray byteArray, void *saBuffer)
{
	// BODY
#ifndef NDEBUG
	assert(byteArray != NULL);
	assert(saBuffer != NULL);
	_TRACE2("NATIVE: Executing JNU_jByteArray_unlink(...)\n");
#endif				// NDEBUG
	(*jniEnv)->ReleaseByteArrayElements(jniEnv, byteArray, saBuffer, 0);

#ifndef NDEBUG
	_TRACE2("NATIVE: JNU_jByteArray_unlink(...) returning normally\n");
#endif				// NDEBUG
	return;
}

/**************************************************************************
 * FUNCTION:      JNU_GetEnvForCallback
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_OK if a valid jniEnv pointer was returned, a negative value otherwise
 * NOTE: -> no exception is thrown!
 *************************************************************************/
jint JNU_GetEnvForCallback(JavaVM *vmPtr, JNIEnv **envPtrPtr)
//jint JNU_GetEnvForCallback( JavaVM* vmPtr, void** envPtrPtr )
{

	// VARIABLES
	// jni
	jint _status;

	// BODY
	_TRACE2("NATIVE: Executing JNU_GetEnvForCallback(%p,...)\n",
		(void *)vmPtr);
	// get context
	_status = (*cachedJVM)->GetEnv(vmPtr,
				       (void **)envPtrPtr, JNI_VERSION_1_2);
	if (_status != JNI_OK) {
		// TODO error handling
		_TRACE2("NATIVE ERROR: _status by GetEnv() is %d\n", _status);
	}
	return _status;
}

/**************************************************************************
 * FUNCTION:      JNU_GetGlobalClassRef
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     a global class reference for the specified class, or NULL
 *************************************************************************/
jclass JNU_GetGlobalClassRef(JNIEnv *jniEnv, const char *className)
{
	// VARIABLES
	// jni
	jclass _cls;
	jclass _globalCls;

	// BODY
#ifndef NDEBUG
	_TRACE2("NATIVE: Executing JNU_GetGlobalClassRef(...,%s)\n", className);
#endif				// NDEBUG
	_cls = (*jniEnv)->FindClass(jniEnv, className);
	if (_cls == NULL) {
#ifndef NDEBUG
		_TRACE2("NATIVE ERROR: FindClass() returned NULL!\n");
		JNU_ExceptionDescribeDoNotClear(jniEnv);
#endif				// NDEBUG
		return NULL;	// EXIT POINT! Exception pending...
	}
	_globalCls = (*jniEnv)->NewGlobalRef(jniEnv, _cls);
	if (_globalCls == NULL) {
#ifndef NDEBUG
		_TRACE2("NATIVE ERROR: NewGlobalRef() returned NULL\n");
		JNU_ExceptionDescribeDoNotClear(jniEnv);
#endif				// NDEBUG
		return NULL;	// EXIT POINT! Exception pending...
	}
	return _globalCls;
}

/**************************************************************************
 * FUNCTION:      U_convertTimeout
 * TYPE:          internal function
 * OVERVIEW:      converts the timeout value specified in nanoseconds to
 *                POSIX struct timeval.
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void U_convertTimeout(struct timeval *to_timeout, const jlong from_timeout)
{
	// BODY
#ifndef NDEBUG
	// _TRACE2( "NATIVE: Executing U_convertTimeout(..., %ld )\n", (long) from_timeout );
#endif				// NDEBUG
	// TODO types checking! negative values
	if (from_timeout < SA_TIME_ONE_MICROSECOND) {
		// no timout
		to_timeout->tv_sec = 0;
		to_timeout->tv_usec = 0;
		return;		// EXIT POINT!!!
	}
	if (from_timeout < SA_TIME_ONE_SECOND) {
		// subsecond timout
		to_timeout->tv_sec = 0;
		to_timeout->tv_usec = from_timeout / SA_TIME_ONE_MICROSECOND;
		return;		// EXIT POINT!!!
	}
	to_timeout->tv_sec = from_timeout / SA_TIME_ONE_SECOND;
	to_timeout->tv_usec =
		(from_timeout % SA_TIME_ONE_SECOND) / SA_TIME_ONE_MICROSECOND;
	return;
}
