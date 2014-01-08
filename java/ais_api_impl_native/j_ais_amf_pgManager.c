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

// TODO move this to some header file, together with the similar clm constants
//#define DEFAULT_NUMBER_OF_ITEMS 1
#define DEFAULT_NUMBER_OF_ITEMS 2

/**************************************************************************
 * Macros
 *************************************************************************/

// TODO move this to some header file, together with the similar clm define
// #define IMPL_CLIENT_ALLOC 0 //

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable declarations
 *************************************************************************/

/**************************************************************************
 * Variable definitions
 *************************************************************************/

// CLASS ais.amf.ProtectionGroupManager
static jclass ClassProtectionGroupManager = NULL;
static jfieldID FID_amfLibraryHandle = NULL;

// CLASS ais.amf.ProtectionGroupNotification
static jclass ClassProtectionGroupNotification = NULL;
static jmethodID CID_ProtectionGroupNotification_constructor = NULL;
static jfieldID FID_PGN_change = NULL;
static jfieldID FID_PGN_member = NULL;

// CLASS ais.amf.ProtectionGroupMember
static jclass ClassProtectionGroupMember = NULL;
static jmethodID CID_ProtectionGroupMember_constructor = NULL;
static jfieldID FID_PGM_rank = NULL;
static jfieldID FID_PGM_haState = NULL;
static jfieldID FID_PGM_componentName = NULL;

// ENUM ais.amf.ProtectionGroupNotification$ProtectionGroupChanges
static jclass EnumProtectionGroupChanges = NULL;
static jfieldID FID_NO_CHANGE = NULL;
static jfieldID FID_ADDED = NULL;
static jfieldID FID_REMOVED = NULL;
static jfieldID FID_STATE_CHANGE = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.ProtectionGroupManager
jboolean JNU_ProtectionGroupManager_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ProtectionGroupManager_initIDs_FromClass_OK(JNIEnv *jniEnv,
								jclass
								classAmfHandle);
static void JNU_invokeSaAmfProtectionGroupTrack_Async(JNIEnv *jniEnv,
						      jobject
						      thisProtectionGroupManager,
						      jstring csiName,
						      const SaUint8T
						      saTrackFlags);
static jobjectArray JNU_invokeSaAmfProtectionGroupTrack_Sync(JNIEnv *jniEnv,
							     jobject
							     thisProtectionGroupManager,
							     jstring csiName,
							     const SaUint8T
							     saTrackFlags);
#ifndef IMPL_CLIENT_ALLOC
static jboolean JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI(JNIEnv
								  *jniEnv,
								  SaAmfHandleT
								  saAmfHandle,
								  const SaNameT
								  *saCsiNamePtr,
								  const SaUint8T
								  saTrackFlags,
								  SaAmfProtectionGroupNotificationBufferT
								  *saNotificationBufferPtr);
#else				// IMPL_CLIENT_ALLOC
static jboolean JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient(JNIEnv
								     *jniEnv,
								     SaAmfHandleT
								     saAmfHandle,
								     const
								     SaNameT
								     *saCsiNamePtr,
								     const
								     SaUint8T
								     saTrackFlags,
								     SaAmfProtectionGroupNotificationBufferT
								     *saNotificationBufferPtr,
								     SaAmfProtectionGroupNotificationT
								     *saNotifications,
								     jboolean
								     *bufferOnStack);
#endif				// IMPL_CLIENT_ALLOC
static jboolean JNU_callSaAmfProtectionGroupTrack_Sync(JNIEnv *jniEnv,
						       SaAmfHandleT saAmfHandle,
						       const SaNameT
						       *saCsiNamePtr,
						       const SaUint8T
						       saTrackFlags,
						       SaAmfProtectionGroupNotificationBufferT
						       *saNotificationBufferPtr,
						       SaAisErrorT
						       *saStatusPtr);

// CLASS ais.amf.ProtectionGroupNotification
jboolean JNU_ProtectionGroupNotification_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ProtectionGroupNotification_initIDs_FromClass_OK(JNIEnv
								     *jniEnv,
								     jclass
								     classProtectionGroupNotification);
static jobject JNU_ProtectionGroupNotification_create(JNIEnv *jniEnv,
						      const
						      SaAmfProtectionGroupNotificationT
						      *saProtectionGroupNotificationPtr);
jobjectArray JNU_ProtectionGroupNotificationArray_create(JNIEnv *jniEnv,
							 const
							 SaAmfProtectionGroupNotificationBufferT
							 *saNotificationBufferPtr);
static jboolean JNU_ProtectionGroupNotification_set(JNIEnv *jniEnv,
						    jobject
						    sProtectionGroupNotification,
						    const
						    SaAmfProtectionGroupNotificationT
						    *saProtectionGroupNotificationPtr);
static jboolean JNU_ProtectionGroupNotificationArray_set(JNIEnv *jniEnv,
							 jobjectArray
							 sProtectionGroupNotificationArray,
							 const
							 SaAmfProtectionGroupNotificationBufferT
							 *saNotificationBufferPtr);

// CLASS ais.amf.ProtectionGroupMember
jboolean JNU_ProtectionGroupMember_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ProtectionGroupMember_initIDs_FromClass_OK(JNIEnv *jniEnv,
							       jclass
							       classProtectionGroupMember);
static jobject JNU_ProtectionGroupMember_create(JNIEnv *jniEnv,
						const
						SaAmfProtectionGroupMemberT
						*saProtectionGroupMemberPtr);
static jboolean JNU_ProtectionGroupMember_set(JNIEnv *jniEnv,
					      jobject sProtectionGroupMember,
					      const SaAmfProtectionGroupMemberT
					      *saProtectionGroupMemberPtr);

// ENUM ais.amf.ProtectionGroupNotification$ProtectionGroupChanges
jboolean JNU_ProtectionGroupChanges_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ProtectionGroupChanges_initIDs_FromClass_OK(JNIEnv *jniEnv,
								jclass
								EnumProtectionGroupChanges);
static jobject JNU_ProtectionGroupChanges_getEnum(JNIEnv *jniEnv,
						  SaAmfProtectionGroupChangesT
						  saProtectionGroupChanges);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.ProtectionGroupManager
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupManager_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ProtectionGroupManager_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupManager_initIDs_OK(...)\n");

	// get ProtectionGroupManager class & create a global reference right away
	/*
	  ClassProtectionGroupManager =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/opensaf/ais/amf/ProtectionGroupManagerImpl" )
	  ); */
	ClassProtectionGroupManager = JNU_GetGlobalClassRef(jniEnv,
							    "org/opensaf/ais/amf/ProtectionGroupManagerImpl");
	if (ClassProtectionGroupManager == NULL) {

		_TRACE2("NATIVE ERROR: ClassProtectionGroupManager is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ProtectionGroupManager_initIDs_FromClass_OK(jniEnv,
							       ClassProtectionGroupManager);

}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupManager_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupManager_initIDs_FromClass_OK(JNIEnv *jniEnv,
								jclass
								classProtectionGroupManager)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupManager_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classProtectionGroupManager,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupManager_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroup
 * TYPE:      native method
 *  Class:     ais_amf_ProtectionGroupManager
 *  Method:    getProtectionGroup
 *  Signature: (Ljava/lang/String;)[Lorg/saforum/ais/amf/ProtectionGroupNotification;
 *************************************************************************/
JNIEXPORT jobjectArray JNICALL
Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroup(JNIEnv
								       *jniEnv,
								       jobject
								       thisProtectionGroupManager,
								       jstring
								       csiName)
{
	// BODY

	assert(thisProtectionGroupManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroup(...)\n");

	return JNU_invokeSaAmfProtectionGroupTrack_Sync(jniEnv,
							thisProtectionGroupManager,
							csiName, 0);
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupAsync
 * TYPE:      native method
 *  Class:     ais_amf_ProtectionGroupManager
 *  Method:    getProtectionGroupAsync
 *  Signature: (Ljava/lang/String;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupAsync
(JNIEnv *jniEnv, jobject thisProtectionGroupManager, jstring csiName)
{

	// BODY

	assert(thisProtectionGroupManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupAsync(...)\n");

	JNU_invokeSaAmfProtectionGroupTrack_Async(jniEnv,
						  thisProtectionGroupManager,
						  csiName, SA_TRACK_CURRENT);

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_startProtectionGroupTracking
 * TYPE:      native method
 *  Class:     org_opensaf_ais_amf_ProtectionGroupManagerImpl
 *  Method:    startProtectionGroupTracking
 *  Signature: (Ljava/lang/String;Lorg/saforum/ais/TrackFlags;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_startProtectionGroupTracking
(JNIEnv *jniEnv, jobject thisProtectionGroupManager, jstring csiName, jobject trackFlags)
{
	// VARIABLES
	SaUint8T _saTrackFlags;

	// BODY

	assert(thisProtectionGroupManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_startProtectionGroupTracking(...)\n");

	// get track flag
	if (trackFlags == NULL) {
		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	_saTrackFlags = (SaUint8T)
		(*jniEnv)->GetIntField(jniEnv, trackFlags, FID_TF_value);

	_saTrackFlags |= SA_TRACK_CURRENT;
	// check track flags
	/*
	  if ( JNU_TrackFlagsForChanges_OK( jniEnv, trackFlags ) != JNI_TRUE ) {
	  return; // EXIT POINT! Exception pending...
	  }
	*/
	// invoke saAmfProtectionGroupTrack()
	JNU_invokeSaAmfProtectionGroupTrack_Async(jniEnv,
						  thisProtectionGroupManager,
						  csiName, _saTrackFlags);
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupThenStartTracking
 * TYPE:      native method
 *  Class:     ais_amf_ProtectionGroupManager
 *  Method:    getProtectionGroupThenStartTracking
 *  Signature: (Ljava/lang/String;Lorg/saforum/ais/TrackFlags;)[Lorg/saforum/ais/amf/ProtectionGroupNotification;
 *************************************************************************/
JNIEXPORT jobjectArray JNICALL
Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupThenStartTracking
(JNIEnv *jniEnv, jobject thisProtectionGroupManager, jstring csiName, jobject trackFlags)
{
	// VARIABLES
	SaUint8T _saTrackFlags;

	// BODY

	assert(thisProtectionGroupManager != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_getProtectionGroupThenStartTracking(...)\n");

	// get track flag
	if (trackFlags == NULL) {
		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return NULL;	// EXIT POINT!
	}
	_saTrackFlags = (SaUint8T)
		(*jniEnv)->GetIntField(jniEnv, trackFlags, FID_TF_value);
	// check track flags
	/*
	  if ( JNU_TrackFlagsForChanges_OK( jniEnv, trackFlags ) != JNI_TRUE ) {
	  return NULL; // EXIT POINT! Exception pending...
	  }
	*/
	return JNU_invokeSaAmfProtectionGroupTrack_Sync(jniEnv,
							thisProtectionGroupManager,
							csiName, _saTrackFlags);

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_stopProtectionGroupTracking
 * TYPE:      native method
 *  Class:     ais_amf_ProtectionGroupManager
 *  Method:    stopProtectionGroupTracking
 *  Signature: (Ljava/lang/String;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_stopProtectionGroupTracking
(JNIEnv *jniEnv, jobject thisProtectionGroupManager, jstring csiName)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saCsiName;
	SaNameT *_saCsiNamePtr = &_saCsiName;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_stopProtectionGroupTracking(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisProtectionGroupManager,
						      FID_amfLibraryHandle);
	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// copy Java csi name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					csiName, &_saCsiNamePtr) != JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfProtectionGroupTrack
	_saStatus = saAmfProtectionGroupTrackStop(_saAmfHandle, _saCsiNamePtr);

	_TRACE2
		("NATIVE: saAmfProtectionGroupTrackStop(...) has returned with %d...\n",
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
		return;		// EXIT POINT!!!
	}

	// normal exit

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ProtectionGroupManagerImpl_stopProtectionGroupTracking(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:      JNU_invokeSaAmfProtectionGroupTrack_Async
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
static void JNU_invokeSaAmfProtectionGroupTrack_Async(JNIEnv *jniEnv,
						      jobject
						      thisProtectionGroupManager,
						      jstring csiName,
						      const SaUint8T
						      saTrackFlags)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saCsiName;
	SaNameT *_saCsiNamePtr = &_saCsiName;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;

	// BODY

	assert(thisProtectionGroupManager != NULL);
	assert((saTrackFlags &
		(~
		 (SA_TRACK_CURRENT | SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY)))
	       == 0);
	_TRACE2
		("NATIVE: Executing JNU_invokeSaAmfProtectionGroupTrack_Async(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisProtectionGroupManager,
						      FID_amfLibraryHandle);
	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// copy Java csi name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					csiName, &_saCsiNamePtr) != JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfProtectionGroupTrack
	_saStatus = saAmfProtectionGroupTrack(_saAmfHandle,
					      _saCsiNamePtr,
					      saTrackFlags, NULL);

	_TRACE2
		("NATIVE: saAmfProtectionGroupTrack(...) has returned with %d...\n",
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
		case SA_AIS_ERR_NO_SPACE:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			//JNU_throwNewByName( jniEnv,
			//                    "org/saforum/ais/AisNoSpaceException",
			//                    AIS_ERR_NO_SPACE_MSG );
			break;
		case SA_AIS_ERR_NOT_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNotExistException",
					   AIS_ERR_NOT_EXIST_MSG);
			break;
		case SA_AIS_ERR_BAD_FLAGS:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			//JNU_throwNewByName( jniEnv,
			//                    "org/saforum/ais/AisBadFlagsException",
			//                    AIS_ERR_BAD_FLAGS_MSG );
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
	// normal exit

	_TRACE2
		("NATIVE: JNU_invokeSaAmfProtectionGroupTrack_Async(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  JNU_invokeSaAmfProtectionGroupTrack_Sync
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     TODO
 *************************************************************************/
static jobjectArray JNU_invokeSaAmfProtectionGroupTrack_Sync(JNIEnv *jniEnv,
							     jobject
							     thisProtectionGroupManager,
							     jstring csiName,
							     const SaUint8T
							     saTrackFlags)
{
	// VARIABLES
	// JNI
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaNameT _saCsiName;
	SaNameT *_saCsiNamePtr = &_saCsiName;
	SaAmfProtectionGroupNotificationBufferT _saNotificationBuffer;
	// JNI
	jobjectArray _sProtectionGroupNotificationArray;
	jobject _amfLibraryHandle;
#ifndef IMPL_CLIENT_ALLOC
	// if we use JNU_invokeSaAmfProtectionGroupTrack_SyncAllocAPI, then the API reserves
	// the buffer from the heap
	jboolean _bufferOnStack = JNI_FALSE;
#else				// IMPL_CLIENT_ALLOC
	// if we use JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient, then initially we try to
	// create the buffer on the stack (if the default size for the buffer is not enough,
	// we will then allocate the buffer from the heap and then we change thi initial
	// value.
	jboolean _bufferOnStack = JNI_TRUE;
	SaAmfProtectionGroupNotificationT
		_saNotifications[DEFAULT_NUMBER_OF_ITEMS];
	// this is not really necessary, but may be useful to catch bugs...
	memset(_saNotifications,
	       0,
	       (DEFAULT_NUMBER_OF_ITEMS *
		sizeof(SaAmfProtectionGroupNotificationT)));
#endif				// IMPL_CLIENT_ALLOC

	// BODY

	assert(thisProtectionGroupManager != NULL);
	assert((saTrackFlags & (~(SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) ==
	       0);
	_TRACE2
		("NATIVE: Executing JNU_invokeSaAmfProtectionGroupTrack_Sync(...)\n");
	// _TRACE2( "NATIVE: _saNotificationBuffer: %p\n", &_saNotificationBuffer );

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisProtectionGroupManager,
						      FID_amfLibraryHandle);
	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// copy Java csi name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					csiName, &_saCsiNamePtr) != JNI_TRUE) {
		return NULL;	// EXIT POINT! Exception pending...
	}
	//
#ifndef IMPL_CLIENT_ALLOC
	if (JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI(jniEnv,
							      _saAmfHandle,
							      _saCsiNamePtr,
							      saTrackFlags,
							      &_saNotificationBuffer)
	    != JNI_TRUE) {
		// error, some exception has been thrown already!
		return NULL;	// EXIT POINT!!!
	}
#else				// IMPL_CLIENT_ALLOC
	if (JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient(jniEnv,
								 _saAmfHandle,
								 &_saCsiName,
								 saTrackFlags,
								 &_saNotificationBuffer,
								 _saNotifications,
								 &_bufferOnStack)
	    != JNI_TRUE) {
		// error, some exception has been thrown already!
		return NULL;	// EXIT POINT!!!
	}
#endif				// IMPL_CLIENT_ALLOC

	// process results
	_sProtectionGroupNotificationArray =
		JNU_ProtectionGroupNotificationArray_create(jniEnv,
							    &_saNotificationBuffer);
	/* Do not check for NULL, because we do not want to return before some clean up...)
	   if ( _sProtectionGroupNotificationArray == NULL ) {

	   _TRACE2( "NATIVE ERROR: _sProtectionGroupNotificationArray is NULL\n" );

	   return NULL; // exception thrown already...
	   }
	*/
	// free notification buffer
	if ((_saNotificationBuffer.notification != NULL) &&
	    (_bufferOnStack == JNI_FALSE)) {
		free(_saNotificationBuffer.notification);
	}
	// normal exit

	_TRACE2
		("NATIVE: JNU_invokeSaAmfProtectionGroupTrack_Sync(...) returning...\n");

	return _sProtectionGroupNotificationArray;
}

#ifndef IMPL_CLIENT_ALLOC

/**************************************************************************
 * FUNCTION:  JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI(JNIEnv
								  *jniEnv,
								  SaAmfHandleT
								  saAmfHandle,
								  const SaNameT
								  *saCsiNamePtr,
								  const SaUint8T
								  saTrackFlags,
								  SaAmfProtectionGroupNotificationBufferT
								  *saNotificationBufferPtr)
{
	// VARIABLES
	SaAisErrorT _saStatus;

	// BODY

	// assert( saAmfHandle != (SaAmfHandleT) NULL );
	assert((saTrackFlags & (~(SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) ==
	       0);
	assert(saNotificationBufferPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI(...)\n");

	// init notification buffer
	saNotificationBufferPtr->numberOfItems = 0;
	saNotificationBufferPtr->notification = NULL;	// let AIS to reserve the memory
	// call saAmfProtectionGroupTrack
	if (JNU_callSaAmfProtectionGroupTrack_Sync(jniEnv,
						   saAmfHandle,
						   saCsiNamePtr,
						   saTrackFlags,
						   saNotificationBufferPtr,
						   &_saStatus) != JNI_TRUE) {
		// error, some exception has been thrown already!
		// TODO handle NO_SPACE case!!
		return JNI_FALSE;	// EXIT POINT!!!
	}
	// normal exit

	_TRACE2
		("NATIVE: JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocAPI(...) returning normally\n");

	return JNI_TRUE;
}

#else				// IMPL_CLIENT_ALLOC

/**************************************************************************
 * FUNCTION:  JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient(JNIEnv
								     *jniEnv,
								     SaAmfHandleT
								     saAmfHandle,
								     const
								     SaNameT
								     *saCsiNamePtr,
								     const
								     SaUint8T
								     saTrackFlags,
								     SaAmfProtectionGroupNotificationBufferT
								     *saNotificationBufferPtr,
								     SaAmfProtectionGroupNotificationT
								     *saNotifications,
								     jboolean
								     *bufferOnStackPtr)
{
	// VARIABLES
	SaAisErrorT _saStatus;
	SaAmfProtectionGroupNotificationT *_saNotificationsPtr = NULL;

	// BODY

	// assert( saAmfHandle != (SaAmfHandleT) NULL );
	assert((saTrackFlags & (~(SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) ==
	       0);
	assert(saNotificationBufferPtr != NULL);
	assert(bufferOnStackPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient(...)\n");

	// init notification buffer
	saNotificationBufferPtr->numberOfItems = DEFAULT_NUMBER_OF_ITEMS;
	saNotificationBufferPtr->notification = saNotifications;
	*bufferOnStackPtr = JNI_TRUE;
	// call saAmfProtectionGroupTrack

	_TRACE2
		("NATIVE: 1st try: calling saAmfProtectionGroupTrack with buffer on the stack...\n");

	if (JNU_callSaAmfProtectionGroupTrack_Sync(jniEnv,
						   saAmfHandle,
						   saCsiNamePtr,
						   saTrackFlags,
						   saNotificationBufferPtr,
						   &_saStatus) != JNI_TRUE) {
		// error handling (some exception has been thrown already!)
		if (_saStatus == SA_AIS_ERR_NO_SPACE) {

			_TRACE2
				("NATIVE: 1st try failed: SA_AIS_ERR_NO_SPACE\n");

			// clear exception
			(*jniEnv)->ExceptionClear(jniEnv);
			// reserve enough memory for notification buffer
			saNotificationBufferPtr->notification =
				calloc(saNotificationBufferPtr->numberOfItems,
				       sizeof(SaAmfProtectionGroupNotificationT));
			if (saNotificationBufferPtr->notification == NULL) {
				JNU_throwNewByName(jniEnv,
						   "org/saforum/ais/AisNoMemoryException",
						   AIS_ERR_NO_MEMORY_MSG);
				return JNI_FALSE;	// EXIT POINT!!!
			}
			// record ptr for free()
			_saNotificationsPtr =
				saNotificationBufferPtr->notification;
			*bufferOnStackPtr = JNI_FALSE;
			// try again

			_TRACE2
				("NATIVE: 2nd try: calling saAmfProtectionGroupTrack with a bigger buffer on the heap...\n");

			if (JNU_callSaAmfProtectionGroupTrack_Sync(jniEnv,
								   saAmfHandle,
								   saCsiNamePtr,
								   saTrackFlags,
								   saNotificationBufferPtr,
								   &_saStatus)
			    != JNI_TRUE) {
				// error handling (some exception has been thrown already!)
				return JNI_FALSE;	// EXIT POINT!!!
			}
		} else {

			_TRACE2("NATIVE: 1st try failed, no more tries\n");

			return JNI_FALSE;	// EXIT POINT!!!
		}
	}

	_TRACE2
		("NATIVE: JNU_invokeSaAmfProtectionGroupTrack_Sync_AllocClient(...) returning normally\n");

	return JNI_TRUE;
}

#endif				// IMPL_CLIENT_ALLOC

/**************************************************************************
 * FUNCTION:      JNU_callSaAmfProtectionGroupTrack_Sync
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_callSaAmfProtectionGroupTrack_Sync(JNIEnv *jniEnv,
						       SaAmfHandleT saAmfHandle,
						       const SaNameT
						       *saCsiNamePtr,
						       const SaUint8T
						       saTrackFlags,
						       SaAmfProtectionGroupNotificationBufferT
						       *saNotificationBufferPtr,
						       SaAisErrorT *saStatusPtr)
{
	// BODY

	// assert( saAmfHandle != (SaAmfHandleT) NULL );
	assert((saTrackFlags & (~(SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY))) ==
	       0);
	assert(saNotificationBufferPtr != NULL);
	assert(saStatusPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_callSaAmfProtectionGroupTrack_Sync(...)\n");

	// call saAmfProtectionGroupTrack

	// U_printSaProtectionGroupNotificationBuffer( "Values of saNotificationBuffer BEFORE calling saAmfProtectionGroupTrack: \n", saNotificationBufferPtr );

	*saStatusPtr = saAmfProtectionGroupTrack(saAmfHandle,
						 saCsiNamePtr,
						 SA_TRACK_CURRENT |
						 saTrackFlags,
						 saNotificationBufferPtr);

	_TRACE2
		("NATIVE: saAmfProtectionGroupTrack(...) has returned with %d...\n",
		 *saStatusPtr);
	_TRACE2
		("NATIVE: saAmfProtectionGroupTrack(...) notification %d %d...\n",
		 saNotificationBufferPtr->numberOfItems,
		 saNotificationBufferPtr->notification);
	// U_printSaProtectionGroupNotificationBuffer( "Values of saNotificationBuffer AFTER calling saAmfProtectionGroupTrack: \n", saNotificationBufferPtr );

	// error handling
	if ((*saStatusPtr) != SA_AIS_OK) {
		switch (*saStatusPtr) {
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
			// TODO this should not happen here!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisInitException",
					   AIS_ERR_INIT_MSG);
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
		case SA_AIS_ERR_NO_SPACE:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNoSpaceException",
					   AIS_ERR_NO_SPACE_MSG);
			break;
		case SA_AIS_ERR_NOT_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNotExistException",
					   AIS_ERR_NOT_EXIST_MSG);
			break;
		case SA_AIS_ERR_BAD_FLAGS:
			// this should not happen here! because no invalid track flag can come from the Java side

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			//JNU_throwNewByName( jniEnv,
			//                    "org/saforum/ais/AisBadFlagsException",
			//                    AIS_ERR_BAD_FLAGS_MSG );
			break;
		default:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			break;
		}
		return JNI_FALSE;	// EXIT POINT!!!
	}
	// check numberOfItems
	if (saNotificationBufferPtr->numberOfItems == 0) {

		_TRACE2
			("NATIVE ERROR: saNotificationBufferptr->numberOfItems is 0\n");

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return JNI_FALSE;
	}
	// check notification
	if (saNotificationBufferPtr->notification == NULL) {

		_TRACE2
			("NATIVE ERROR: saNotificationBufferPtr->notification is NULL\n");

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return JNI_FALSE;
	}
	// normal exit

	_TRACE2
		("NATIVE: JNU_callSaAmfProtectionGroupTrack_Sync(...) returning normally\n");

	return JNI_TRUE;
}

//******************************************
// CLASS ais.amf.ProtectionGroupNotification
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotification_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ProtectionGroupNotification_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupNotification_initIDs_OK(...)\n");

	// get ProtectionGroupNotification class & create a global reference right away
	/*
	  ClassProtectionGroupNotification =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/amf/ProtectionGroupNotification" )
	  ); */
	ClassProtectionGroupNotification = JNU_GetGlobalClassRef(jniEnv,
								 "org/saforum/ais/amf/ProtectionGroupNotification");
	if (ClassProtectionGroupNotification == NULL) {

		_TRACE2
			("NATIVE ERROR: ClassProtectionGroupNotification is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ProtectionGroupNotification_initIDs_FromClass_OK(jniEnv,
								    ClassProtectionGroupNotification);
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotification_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupNotification_initIDs_FromClass_OK(JNIEnv
								     *jniEnv,
								     jclass
								     classProtectionGroupNotification)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupNotification_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_ProtectionGroupNotification_constructor =
		(*jniEnv)->GetMethodID(jniEnv, classProtectionGroupNotification,
				       "<init>", "()V");
	if (CID_ProtectionGroupNotification_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_ProtectionGroupNotification_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_PGN_change = (*jniEnv)->GetFieldID(jniEnv,
					       classProtectionGroupNotification,
					       "change",
					       "Lorg/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges;");
	if (FID_PGN_change == NULL) {

		_TRACE2("NATIVE ERROR: FID_PGN_change is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_PGN_member = (*jniEnv)->GetFieldID(jniEnv,
					       classProtectionGroupNotification,
					       "member",
					       "Lorg/saforum/ais/amf/ProtectionGroupMember;");
	if (FID_PGN_member == NULL) {

		_TRACE2("NATIVE ERROR: FID_PGN_member is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupNotification_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotification_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ProtectionGroupNotification object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ProtectionGroupNotification_create(JNIEnv *jniEnv,
						      const
						      SaAmfProtectionGroupNotificationT
						      *saProtectionGroupNotificationPtr)
{

	// VARIABLES
	// JNI
	jobject _sProtectionGroupNotification;

	// BODY

	assert(saProtectionGroupNotificationPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupNotification_create(...)\n");
	//U_printSaClusterNotification( "NATIVE: Input value of saNotification: \n", saNotificationPtr );

	// create new ProtectionGroupNotification object
	_sProtectionGroupNotification = (*jniEnv)->NewObject(jniEnv,
							     ClassProtectionGroupNotification,
							     CID_ProtectionGroupNotification_constructor);
	if (_sProtectionGroupNotification == NULL) {

		_TRACE2
			("NATIVE ERROR: _sProtectionGroupNotification is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ProtectionGroupNotification object
	if (JNU_ProtectionGroupNotification_set(jniEnv,
						_sProtectionGroupNotification,
						saProtectionGroupNotificationPtr)
	    != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupNotification_create(...) returning normally\n");

	return _sProtectionGroupNotification;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotification_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupNotification_set(JNIEnv *jniEnv,
						    jobject
						    sProtectionGroupNotification,
						    const
						    SaAmfProtectionGroupNotificationT
						    *saProtectionGroupNotificationPtr)
{
	// VARIABLES
	// JNI
	jobject _pgChange;
	jobject _member;

	// BODY

	assert(sProtectionGroupNotification != NULL);
	assert(saProtectionGroupNotificationPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ProtectionGroupNotification_set(...)\n");
	//U_printSaClusterNotification( "NATIVE: Input value of saNotification: \n", saProtectionGroupNotificationPtr );

	// change
	_pgChange = JNU_ProtectionGroupChanges_getEnum(jniEnv,
						       saProtectionGroupNotificationPtr->
						       change);
	if (_pgChange == NULL) {

		_TRACE2("NATIVE ERROR: _pgChange is NULL\n");

		return JNI_FALSE;	// AisLibraryException thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sProtectionGroupNotification,
				  FID_PGN_change, _pgChange);
	// clusterNode
	_member = JNU_ProtectionGroupMember_create(jniEnv,
						   &
						   (saProtectionGroupNotificationPtr->
						    member));
	if (_member == NULL) {

		_TRACE2("NATIVE ERROR: _member is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sProtectionGroupNotification,
				  FID_PGN_member, _member);

	_TRACE2
		("NATIVE: JNU_ProtectionGroupNotification_set(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotificationArray_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ProtectionGroupNotificationArray object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jobjectArray JNU_ProtectionGroupNotificationArray_create(JNIEnv *jniEnv,
							 const
							 SaAmfProtectionGroupNotificationBufferT
							 *saNotificationBufferPtr)
{
	// VARIABLES
	// JNI
	jobjectArray _sProtectionGroupNotificationArray;

	// BODY

	assert(saNotificationBufferPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupNotificationArray_create(...)\n");

	// create notification array
	_sProtectionGroupNotificationArray = (*jniEnv)->NewObjectArray(jniEnv,
								       saNotificationBufferPtr->
								       numberOfItems,
								       ClassProtectionGroupNotification,
								       NULL);
	if (_sProtectionGroupNotificationArray == NULL) {

		_TRACE2
			("NATIVE ERROR: _arraySProtectionGroupNotification is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	// create & assign array elements
	if (JNU_ProtectionGroupNotificationArray_set(jniEnv,
						     _sProtectionGroupNotificationArray,
						     saNotificationBufferPtr) !=
	    JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupNotificationArray_create(...) returning normally\n");

	return _sProtectionGroupNotificationArray;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupNotificationArray_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupNotificationArray_set(JNIEnv *jniEnv,
							 jobjectArray
							 sProtectionGroupNotificationArray,
							 const
							 SaAmfProtectionGroupNotificationBufferT
							 *saNotificationBufferPtr)
{
	// VARIABLES
	unsigned int _idx;
	// JNI
	jobject _sProtectionGroupNotification;

	// BODY

	assert(sProtectionGroupNotificationArray != NULL);
	assert(saNotificationBufferPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupNotificationArray_set(...)\n");
	//U_printSaProtectionGroupNotificationArray( "Input values of saNotificationBuffer: \n", saNotificationBufferPtr );

	// create & assign array elements
	for (_idx = 0; _idx < saNotificationBufferPtr->numberOfItems; _idx++) {
		_sProtectionGroupNotification =
			JNU_ProtectionGroupNotification_create(jniEnv,
							       &
							       (saNotificationBufferPtr->
								notification
								[_idx]));
		if (_sProtectionGroupNotification == NULL) {

			_TRACE2
				("NATIVE ERROR: _sProtectionGroupNotification[%d] is NULL\n",
				 _idx);

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectArrayElement(jniEnv,
						 sProtectionGroupNotificationArray,
						 (jsize)_idx,
						 _sProtectionGroupNotification);
		(*jniEnv)->DeleteLocalRef(jniEnv,
					  _sProtectionGroupNotification);
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupNotificationArray_set(...) returning normally\n");

	return JNI_TRUE;
}

//******************************************
// CLASS ais.amf.ProtectionGroupMember
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupMember_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ProtectionGroupMember_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupMember_initIDs_OK(...)\n");

	// get ProtectionGroupMember class & create a global reference right away
	/*
	  ClassProtectionGroupMember =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/amf/ProtectionGroupMember" )
	  ); */
	ClassProtectionGroupMember = JNU_GetGlobalClassRef(jniEnv,
							   "org/saforum/ais/amf/ProtectionGroupMember");
	if (ClassProtectionGroupMember == NULL) {

		_TRACE2("NATIVE ERROR: ClassProtectionGroupMember is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ProtectionGroupMember_initIDs_FromClass_OK(jniEnv,
							      ClassProtectionGroupMember);
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupMember_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupMember_initIDs_FromClass_OK(JNIEnv *jniEnv,
							       jclass
							       classProtectionGroupMember)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupMember_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_ProtectionGroupMember_constructor = (*jniEnv)->GetMethodID(jniEnv,
								       classProtectionGroupMember,
								       "<init>",
								       "()V");
	if (CID_ProtectionGroupMember_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_ProtectionGroupMember_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_PGM_rank = (*jniEnv)->GetFieldID(jniEnv,
					     classProtectionGroupMember,
					     "rank", "I");
	if (FID_PGM_rank == NULL) {

		_TRACE2("NATIVE ERROR: FID_PGM_rank is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_PGM_haState = (*jniEnv)->GetFieldID(jniEnv,
						classProtectionGroupMember,
						"haState",
						"Lorg/saforum/ais/amf/HaState;");
	if (FID_PGM_haState == NULL) {

		_TRACE2("NATIVE ERROR: FID_PGM_haState is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_PGM_componentName = (*jniEnv)->GetFieldID(jniEnv,
						      classProtectionGroupMember,
						      "componentName",
						      "Ljava/lang/String;");
	if (FID_PGM_componentName == NULL) {

		_TRACE2("NATIVE ERROR: FID_PGM_componentName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupMember_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupMember_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ProtectionGroupMember object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ProtectionGroupMember_create(JNIEnv *jniEnv,
						const
						SaAmfProtectionGroupMemberT
						*saProtectionGroupMemberPtr)
{

	// VARIABLES
	// JNI
	jobject _sProtectionGroupMember;

	// BODY

	assert(saProtectionGroupMemberPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ProtectionGroupMember_create(...)\n");
	//U_printSaClusterNotification( "NATIVE: Input value of saProtectionGroupMember: \n", saProtectionGroupMembePtr );

	// create new ProtectionGroupMember object
	_sProtectionGroupMember = (*jniEnv)->NewObject(jniEnv,
						       ClassProtectionGroupMember,
						       CID_ProtectionGroupMember_constructor);
	if (_sProtectionGroupMember == NULL) {

		_TRACE2("NATIVE ERROR: _sProtectionGroupMember is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ProtectionGroupMember object
	if (JNU_ProtectionGroupMember_set(jniEnv,
					  _sProtectionGroupMember,
					  saProtectionGroupMemberPtr) !=
	    JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupMember_create(...) returning normally\n");

	return _sProtectionGroupMember;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupMember_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupMember_set(JNIEnv *jniEnv,
					      jobject sProtectionGroupMember,
					      const SaAmfProtectionGroupMemberT
					      *saProtectionGroupMemberPtr)
{
	// VARIABLES
	// JNI
	jobject _haState;
	jstring _componentName;

	// BODY

	assert(sProtectionGroupMember != NULL);
	assert(saProtectionGroupMemberPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ProtectionGroupMember_set(...)\n");
	//U_printSaClusterNotification( "NATIVE: Input value of saNotification: \n", saProtectionGroupMemberPtr );

	// rank
	(*jniEnv)->SetIntField(jniEnv,
			       sProtectionGroupMember,
			       FID_PGM_rank,
			       (jint)saProtectionGroupMemberPtr->rank);
	// ha state
	if ((saProtectionGroupMemberPtr->haState >= SA_AMF_HA_ACTIVE) &&
	    (saProtectionGroupMemberPtr->haState <= SA_AMF_HA_QUIESCING)) {
		_haState = JNU_HaState_getEnum(jniEnv,
					       saProtectionGroupMemberPtr->
					       haState);
	} else {
		_haState = JNU_HaState_getEnum(jniEnv, SA_AMF_HA_ACTIVE);
	}
	if (_haState == NULL) {

		_TRACE2("NATIVE ERROR: _haState is NULL\n");

		return JNI_FALSE;	// ... thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sProtectionGroupMember,
				  FID_PGM_haState, _haState);
	// componentName
	_componentName = JNU_newStringFromSaNameT(jniEnv,
						  &(saProtectionGroupMemberPtr->
						    compName));
	if (_componentName == NULL) {

		_TRACE2("NATIVE ERROR: _componentName is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sProtectionGroupMember,
				  FID_PGM_componentName, _componentName);

	_TRACE2
		("NATIVE: JNU_ProtectionGroupMember_set(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// ENUM ais.amf.ProtectionGroupNotification$ProtectionGroupChanges
//********************************

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupChanges_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ProtectionGroupChanges_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupChanges_initIDs_OK(...)\n");

	// get ProtectionGroupChanges class & create a global reference right away
	/*
	  EnumProtectionGroupChanges =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges" )
	  ); */
	EnumProtectionGroupChanges = JNU_GetGlobalClassRef(jniEnv,
							   "org/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges");
	if (EnumProtectionGroupChanges == NULL) {

		_TRACE2("NATIVE ERROR: EnumProtectionGroupChanges is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ProtectionGroupChanges_initIDs_FromClass_OK(jniEnv,
							       EnumProtectionGroupChanges);

}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupChanges_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ProtectionGroupChanges_initIDs_FromClass_OK(JNIEnv *jniEnv,
								jclass
								enumProtectionGroupChanges)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ProtectionGroupChanges_initIDs_FromClass_OK(...)\n");

	// get field IDs (the two predefined enum values)
	FID_NO_CHANGE = (*jniEnv)->GetStaticFieldID(jniEnv,
						    enumProtectionGroupChanges,
						    "NO_CHANGE",
						    "Lorg/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges;");
	if (FID_NO_CHANGE == NULL) {

		_TRACE2("NATIVE ERROR: FID_NO_CHANGE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_ADDED = (*jniEnv)->GetStaticFieldID(jniEnv,
						enumProtectionGroupChanges,
						"ADDED",
						"Lorg/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges;");
	if (FID_ADDED == NULL) {

		_TRACE2("NATIVE ERROR: FID_ADDED is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_REMOVED = (*jniEnv)->GetStaticFieldID(jniEnv,
						  enumProtectionGroupChanges,
						  "REMOVED",
						  "Lorg/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges;");
	if (FID_REMOVED == NULL) {

		_TRACE2("NATIVE ERROR: FID_REMOVED is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_STATE_CHANGE = (*jniEnv)->GetStaticFieldID(jniEnv,
						       enumProtectionGroupChanges,
						       "STATE_CHANGE",
						       "Lorg/saforum/ais/amf/ProtectionGroupNotification$ProtectionGroupChanges;");
	if (FID_STATE_CHANGE == NULL) {

		_TRACE2("NATIVE ERROR: FID_STATE_CHANGE is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupChanges_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ProtectionGroupChanges_getEnum
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     an ProtectionGroupChanges object equivalent with the specified SaAmfProtectionGroupChangesT parameter or
 *                NULL if an illegal SaAmfProtectionGroupChangesT value was passed.
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ProtectionGroupChanges_getEnum(JNIEnv *jniEnv,
						  SaAmfProtectionGroupChangesT
						  saProtectionGroupChanges)
{
	// VARIABLES
	jobject _pgChanges = NULL;

	// BODY

	_TRACE2("NATIVE: Executing JNU_ProtectionGroupChanges_getEnum(...)\n");

	//_TRACE2( "NATIVE: pgChanges is %d\n", csiFlags );

	switch (saProtectionGroupChanges) {
	case SA_AMF_PROTECTION_GROUP_NO_CHANGE:
		_pgChanges = (*jniEnv)->GetStaticObjectField(jniEnv,
							     EnumProtectionGroupChanges,
							     FID_NO_CHANGE);

		//_TRACE2( "NATIVE: _pgChanges(NO_CHANGE): %p\n", (void *) _pgChanges );
		assert(_pgChanges != NULL);

		break;
	case SA_AMF_PROTECTION_GROUP_ADDED:
		_pgChanges = (*jniEnv)->GetStaticObjectField(jniEnv,
							     EnumProtectionGroupChanges,
							     FID_ADDED);

		//_TRACE2( "NATIVE: _pgChanges(ADDED): %p\n", (void *) _pgChanges );
		assert(_pgChanges != NULL);

		break;
	case SA_AMF_PROTECTION_GROUP_REMOVED:
		_pgChanges = (*jniEnv)->GetStaticObjectField(jniEnv,
							     EnumProtectionGroupChanges,
							     FID_REMOVED);

		//_TRACE2( "NATIVE: _pgChanges(REMOVED): %p\n", (void *) _pgChanges );
		assert(_pgChanges != NULL);

		break;
	case SA_AMF_PROTECTION_GROUP_STATE_CHANGE:
		_pgChanges = (*jniEnv)->GetStaticObjectField(jniEnv,
							     EnumProtectionGroupChanges,
							     FID_STATE_CHANGE);

		//_TRACE2( "NATIVE: _pgChanges(STATE_CHANGE): %p\n", (void *) _pgChanges );
		assert(_pgChanges != NULL);

		break;
	default:
		// this should not happen here!

		_TRACE2("NATIVE ERROR: _pgChanges(INVALID): \n");
		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return NULL;
	}

	_TRACE2
		("NATIVE: JNU_ProtectionGroupChanges_getEnum(...) returning normally\n");

	return _pgChanges;
}
