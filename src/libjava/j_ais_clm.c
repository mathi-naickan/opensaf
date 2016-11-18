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
#include <saClm.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "j_ais_clm_libHandle.h"
#include "jni_ais_clm.h"	// not really needed, but good for syntax checking!

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

// CLASS ais.clm.ClusterNode
static jclass ClassClusterNode = NULL;
static jmethodID CID_ClusterNode_constructor = NULL;
static jfieldID FID_nodeId = NULL;
static jfieldID FID_nodeAddress = NULL;
static jfieldID FID_nodeName = NULL;
static jfieldID FID_member = NULL;
static jfieldID FID_bootTimestamp = NULL;
static jfieldID FID_initialViewNumber = NULL;
static jfieldID FID_executionEnvironment = NULL;

// CLASS ais.clm.NodeAddress
static jclass ClassNodeAddress = NULL;
static jfieldID FID_NodeAddress_value = NULL;

// CLASS ais.clm.NodeAddressIPv4
static jclass ClassNodeAddressIPv4 = NULL;
static jmethodID CID_NodeAddressIPv4_constructor = NULL;

// CLASS ais.clm.NodeAddressIPv6
static jclass ClassNodeAddressIPv6 = NULL;
static jmethodID CID_NodeAddressIPv6_constructor = NULL;

// CLASS ais.clm.ClusterNotificationBuffer
jclass ClassClusterNotificationBuffer = NULL;
jmethodID CID_ClusterNotificationBuffer_constructor = NULL;
static jfieldID FID_viewNumber = NULL;
static jfieldID FID_notifications = NULL;

// CLASS ais.clm.ClusterNotification
static jclass ClassClusterNotification = NULL;
static jmethodID CID_ClusterNotification_constructor = NULL;
static jfieldID FID_clusterChange = NULL;
static jfieldID FID_clusterNode = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CALLBACKS
void SaClmClusterNodeGetCallback(SaInvocationT saInvocation,
				 const SaClmClusterNodeT *saClusterNodePtr,
				 SaAisErrorT saError);

void SaClmClusterNodeGetCallback_4(SaInvocationT saInvocation,
				   const SaClmClusterNodeT_4 *saClusterNodePtr,
				   SaAisErrorT saError);

void SaClmClusterTrackCallback(const SaClmClusterNotificationBufferT
			       *saNotificationBufferPtr,
			       SaUint32T saNumberOfMembers,
			       SaAisErrorT saError);

void SaClmClusterTrackCallback_4(const SaClmClusterNotificationBufferT_4
				 *saNotificationBufferPtr,
				 SaUint32T saNumberOfMembers,
				 SaInvocationT invocation,
				 const SaNameT *rootCauseEntity,
				 const SaNtfCorrelationIdsT *correlationIds,
				 SaClmChangeStepT step, SaTimeT timeSupervision,
				 SaAisErrorT saError);

// CLASS ais.clm.ClusterNode
jboolean JNU_ClusterNode_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ClusterNode_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classClusterNode);
jobject JNU_ClusterNode_create(JNIEnv *jniEnv,
			       const SaClmClusterNodeT *saClusterNodePtr);
jobject JNU_ClusterNode_create_4(JNIEnv *jniEnv,
				 const SaClmClusterNodeT_4 *saClusterNodePtr);
static jboolean JNU_ClusterNode_set(JNIEnv *jniEnv,
				    jobject sClusterNode,
				    const SaClmClusterNodeT *saClusterNodePtr);
static jboolean JNU_ClusterNode_set_4(JNIEnv *jniEnv,
				      jobject sClusterNode,
				      const SaClmClusterNodeT_4
				      *saClusterNodePtr);

// CLASS ais.clm.NodeAddress
jboolean JNU_NodeAddress_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_NodeAddress_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classNodeAddress);

// CLASS ais.clm.NodeAddressIPv4
jboolean JNU_NodeAddressIPv4_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_NodeAddressIPv4_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 classNodeAddressIPv4);

// CLASS ais.clm.NodeAddressIPv6
jboolean JNU_NodeAddressIPv6_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_NodeAddressIPv6_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 classNodeAddressIPv6);

// CLASS ais.clm.ClusterNotificationBuffer
jboolean JNU_ClusterNotificationBuffer_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ClusterNotificationBuffer_initIDs_FromClass_OK(JNIEnv
								   *jniEnv,
								   jclass
								   classClusterNotificationBuffer);
static jobject JNU_ClusterNotificationBuffer_create(JNIEnv *jniEnv,
						    const
						    SaClmClusterNotificationBufferT
						    *saNotificationBufferPtr);
static jobject JNU_ClusterNotificationBuffer_create_4(JNIEnv *jniEnv,
						      const
						      SaClmClusterNotificationBufferT_4
						      *saNotificationBufferPtr);
jboolean JNU_ClusterNotificationBuffer_set(JNIEnv *jniEnv,
					   jobject sClusterNotificationBuffer,
					   const SaClmClusterNotificationBufferT
					   *saNotificationBufferPtr);
jboolean JNU_ClusterNotificationBuffer_set_4(JNIEnv *jniEnv,
					     jobject sClusterNotificationBuffer,
					     const
					     SaClmClusterNotificationBufferT_4
					     *saNotificationBufferPtr);
// CLASS ais.clm.ClusterNotification
jboolean JNU_ClusterNotification_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ClusterNotification_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     classClusterNotification);
static jobject JNU_ClusterNotification_create(JNIEnv *jniEnv,
					      const SaClmClusterNotificationT
					      *saNotificationPtr);
static jobject JNU_ClusterNotification_create_4(JNIEnv *jniEnv,
						const
						SaClmClusterNotificationT_4
						*saNotificationPtr);
static jboolean JNU_ClusterNotification_set(JNIEnv *jniEnv,
					    jobject sClusterNotification,
					    const SaClmClusterNotificationT
					    *saClusterNotificationPtr);
static jboolean JNU_ClusterNotification_set_4(JNIEnv *jniEnv,
					      jobject sClusterNotification,
					      const SaClmClusterNotificationT_4
					      *saClusterNotificationPtr);

/**************************************************************************
 * Function definitions
 *************************************************************************/

// CALLBACKS

/**************************************************************************
 * FUNCTION:      SaClmClusterNodeGetCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaClmClusterNodeGetCallback(SaInvocationT saInvocation,
				 const SaClmClusterNodeT *saClusterNodePtr,
				 SaAisErrorT saError)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jobject _sClusterNode;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaClmClusterNodeGetCallback(...)\n");

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// create new ClusterNode object
	_sClusterNode = JNU_ClusterNode_create(_jniEnv, saClusterNodePtr);
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE CALLBACK ERROR: _sClusterNode is NULL\n");

		return;		// OutOfMemoryError thrown already...
	}

	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassClmHandle,
					 MID_s_invokeGetClusterNodeCallback,
					 (jlong)saInvocation,
					 _sClusterNode, (jint)saError);

	_TRACE2("NATIVE: SaClmClusterNodeGetCallback returning normally\n");

}

/**************************************************************************
 * FUNCTION:      SaClmClusterNodeGetCallback_4
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaClmClusterNodeGetCallback_4(SaInvocationT saInvocation,
				   const SaClmClusterNodeT_4 *saClusterNodePtr,
				   SaAisErrorT saError)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jobject _sClusterNode;

	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaClmClusterNodeGetCallback(...)\n");

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// create new ClusterNode object
	_sClusterNode = JNU_ClusterNode_create_4(_jniEnv, saClusterNodePtr);
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE CALLBACK ERROR: _sClusterNode is NULL\n");

		return;		// OutOfMemoryError thrown already...
	}
	// invoke Java callback
	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassClmHandle,
					 MID_s_invokeGetClusterNodeCallback,
					 (jlong)saInvocation,
					 _sClusterNode, (jint)saError);

	_TRACE2("NATIVE:SaClmClusterNodeGetCallback_4 returning normally\n");

}

/**************************************************************************
 * FUNCTION:      SaClmClusterTrackCallback
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaClmClusterTrackCallback(const SaClmClusterNotificationBufferT
			       *saNotificationBufferPtr,
			       SaUint32T saNumberOfMembers, SaAisErrorT saError)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jobject _sClusterNotificationBuffer;
	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaClmClusterTrackCallback( %p, %u, %d )\n",
		 saNotificationBufferPtr, (unsigned int)saNumberOfMembers, saError);

	U_printSaClusterNotificationBuffer
		("Values of saNotificationBufferPtr in CALLBACK SaClmClusterTrackCallback: \n",
		 saNotificationBufferPtr);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// create new ClusterNotificationBuffer object
	_sClusterNotificationBuffer =
		JNU_ClusterNotificationBuffer_create(_jniEnv,
						     saNotificationBufferPtr);
	if (_sClusterNotificationBuffer == NULL) {

		_TRACE2
			("NATIVE CALLBACK ERROR: _sClusterNotificationBuffer is NULL\n");

		return;		// OutOfMemoryError thrown already...
	}

	/* invoke Java callback */

	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassClmHandle,
					 MID_s_invokeTrackClusterCallback,
					 _sClusterNotificationBuffer,
					 (jint)saNumberOfMembers,
					 (jint)saError);

	_TRACE2("NATIVE: SaClmClusterTrack returning normally\n");

}

/**************************************************************************
 * FUNCTION:      SaClmClusterTrackCallback_4
 * TYPE:          native AIS callback
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
void SaClmClusterTrackCallback_4(const SaClmClusterNotificationBufferT_4
				 *saNotificationBufferPtr,
				 SaUint32T saNumberOfMembers,
				 SaInvocationT invocation,
				 const SaNameT *rootCauseEntity,
				 const SaNtfCorrelationIdsT *correlationIds,
				 SaClmChangeStepT step, SaTimeT timeSupervision,
				 SaAisErrorT saError)
{

	// VARIABLES
	// jni
	JNIEnv *_jniEnv;
	jint _status;
	jobject _sClusterNotificationBuffer;
	jstring _rootCauseEntity;
	// BODY

	_TRACE2
		("NATIVE CALLBACK: Executing SaClmClusterTrackCallback( %p, %u, %d )\n",
		 saNotificationBufferPtr, (unsigned int)saNumberOfMembers, saError);

	U_printSaClusterNotificationBuffer_4
		("Values of saNotificationBufferPtr in CALLBACK SaClmClusterTrackCallback: \n",
		 saNotificationBufferPtr);

	// get context
	_status = JNU_GetEnvForCallback(cachedJVM, &_jniEnv);
	if (_status != JNI_OK) {
		// TODO error handling

		_TRACE2
			("NATIVE CALLBACK ERROR: _status by JNU_GetEnvForCallback() is %d\n",
			 _status);

		return;
	}
	// create new ClusterNotificationBuffer object
	_sClusterNotificationBuffer =
		JNU_ClusterNotificationBuffer_create_4(_jniEnv,
						       saNotificationBufferPtr);
	if (_sClusterNotificationBuffer == NULL) {

		_TRACE2
			("NATIVE CALLBACK ERROR: _sClusterNotificationBuffer is NULL\n");

		return;		// OutOfMemoryError thrown already...
	}

	/* convert  char* string to jstring */
	_rootCauseEntity = (*_jniEnv)->NewStringUTF(_jniEnv,
						    (char *)rootCauseEntity->
						    value);

	/* invoke Java callback */

	(*_jniEnv)->CallStaticVoidMethod(_jniEnv,
					 ClassClmHandle,
					 MID_s_invokeTrackClusterCallback_4,
					 _sClusterNotificationBuffer,
					 (jint)saNumberOfMembers,
					 (jlong)invocation,
					 _rootCauseEntity,
					 NULL,
					 (jint)step,
					 (jlong)timeSupervision, (jint)saError);

	_TRACE2("NATIVE: SaClmClusterTrackCallback_4 returning normally\n");

}

// INTERNAL METHODS

//****************************
// CLASS ais.clm.ClusterNode
//****************************

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterNode_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_ClusterNode_initIDs_OK(...)\n");

	// get ClusterNode class & create a global reference right away
	/*
	  ClassClusterNode =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/ClusterNode" )
	  ); */
	ClassClusterNode = JNU_GetGlobalClassRef(jniEnv,
						 "org/saforum/ais/clm/ClusterNode");
	if (ClassClusterNode == NULL) {

		_TRACE2("NATIVE ERROR: ClassClusterNode is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ClusterNode_initIDs_FromClass_OK(jniEnv, ClassClusterNode);
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNode_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classClusterNode)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ClusterNode_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_ClusterNode_constructor = (*jniEnv)->GetMethodID(jniEnv,
							     classClusterNode,
							     "<init>", "()V");
	if (CID_ClusterNode_constructor == NULL) {

		_TRACE2("NATIVE ERROR: CID_ClusterNode_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_nodeId = (*jniEnv)->GetFieldID(jniEnv,
					   classClusterNode, "nodeId", "I");
	if (FID_nodeId == NULL) {

		_TRACE2("NATIVE ERROR: FID_nodeId is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_nodeAddress = (*jniEnv)->GetFieldID(jniEnv,
						classClusterNode,
						"nodeAddress",
						"Lorg/saforum/ais/clm/NodeAddress;");
	if (FID_nodeAddress == NULL) {

		_TRACE2("NATIVE ERROR: FID_nodeAddress is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_nodeName = (*jniEnv)->GetFieldID(jniEnv,
					     classClusterNode,
					     "nodeName", "Ljava/lang/String;");
	if (FID_nodeName == NULL) {

		_TRACE2("NATIVE ERROR: FID_nodeName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_member = (*jniEnv)->GetFieldID(jniEnv,
					   classClusterNode, "member", "Z");
	if (FID_member == NULL) {

		_TRACE2("NATIVE ERROR: FID_member is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_bootTimestamp = (*jniEnv)->GetFieldID(jniEnv,
						  classClusterNode,
						  "bootTimestamp", "J");
	if (FID_bootTimestamp == NULL) {

		_TRACE2("NATIVE ERROR: FID_bootTimestamp is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_initialViewNumber = (*jniEnv)->GetFieldID(jniEnv,
						      classClusterNode,
						      "initialViewNumber", "J");

	if (FID_initialViewNumber == NULL) {

		_TRACE2("NATIVE ERROR: FID_initialViewNumber is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_executionEnvironment = (*jniEnv)->GetFieldID(jniEnv,
							 classClusterNode,
							 "executionEnvironment",
							 "Ljava/lang/String;");

	if (FID_executionEnvironment == NULL) {

		_TRACE2("NATIVE ERROR: FID_initialViewNumber is NULL\n");

		return JNI_FALSE;
	}

	_TRACE2
		("NATIVE: JNU_ClusterNode_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNode object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_ClusterNode_create(JNIEnv *jniEnv,
			       const SaClmClusterNodeT *saClusterNodePtr)
{

	// VARIABLES
	// JNI
	jobject _sClusterNode;

	// BODY

	assert(saClusterNodePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNode_create(...)\n");

	// create new ClusterNode object
	_sClusterNode = (*jniEnv)->NewObject(jniEnv,
					     ClassClusterNode,
					     CID_ClusterNode_constructor);
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNode is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNode object
	if (JNU_ClusterNode_set(jniEnv,
				_sClusterNode, saClusterNodePtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2("NATIVE: JNU_ClusterNode_create(...) returning normally\n");

	return _sClusterNode;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_create_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNode object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
jobject JNU_ClusterNode_create_4(JNIEnv *jniEnv,
				 const SaClmClusterNodeT_4 *saClusterNodePtr)
{

	// VARIABLES
	// JNI
	jobject _sClusterNode;

	// BODY

	assert(saClusterNodePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNode_create_4(...)\n");

	// create new ClusterNode object
	_sClusterNode = (*jniEnv)->NewObject(jniEnv,
					     ClassClusterNode,
					     CID_ClusterNode_constructor);
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNode is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNode object
	if (JNU_ClusterNode_set_4(jniEnv,
				  _sClusterNode,
				  saClusterNodePtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2("NATIVE: JNU_ClusterNode_create_4(...) returning normally\n");

	return _sClusterNode;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNode_set(JNIEnv *jniEnv,
				    jobject sClusterNode,
				    const SaClmClusterNodeT *saClusterNodePtr)
{

	// VARIABLES
	// JNI
	jstring _nodeNameString;
	jstring _nodeAddressString;
	jobject _sNodeAddress;

	// BODY

	assert(sClusterNode != NULL);
	assert(saClusterNodePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNode_set(...)\n");

	U_printSaClusterNode("NATIVE: Native Cluster Node info: \n",
			     saClusterNodePtr);

	// nodeId
	(*jniEnv)->SetIntField(jniEnv,
			       sClusterNode,
			       FID_nodeId, (jint)saClusterNodePtr->nodeId);
	// nodeAddress
	_nodeAddressString = JNU_newStringFromSaNodeAddressT(jniEnv,
							     &
							     (saClusterNodePtr->
							      nodeAddress));
	if (_nodeAddressString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE ERROR: _nodeAddressString is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	_sNodeAddress = (saClusterNodePtr->nodeAddress.family == SA_CLM_AF_INET)
		? (*jniEnv)->NewObject(jniEnv,
				       ClassNodeAddressIPv4,
				       CID_NodeAddressIPv4_constructor)
		: (*jniEnv)->NewObject(jniEnv,
				       ClassNodeAddressIPv4,
				       CID_NodeAddressIPv4_constructor);

	if (_sNodeAddress == NULL) {

		_TRACE2("NATIVE ERROR: _sNodeAddress is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  _sNodeAddress,
				  FID_NodeAddress_value, _nodeAddressString);
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNode, FID_nodeAddress, _sNodeAddress);
	// nodeName
	_nodeNameString = JNU_newStringFromSaNameT(jniEnv,
						   &(saClusterNodePtr->
						     nodeName));
	if (_nodeNameString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNode, FID_nodeName, _nodeNameString);
	// member
	(*jniEnv)->SetBooleanField(jniEnv,
				   sClusterNode,
				   FID_member,
				   (jboolean)saClusterNodePtr->member);

	// bootTimestamp
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNode,
				FID_bootTimestamp,
				(jlong)saClusterNodePtr->bootTimestamp);
	// initialViewNumber
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNode,
				FID_initialViewNumber,
				(jlong)saClusterNodePtr->initialViewNumber);

	_TRACE2("NATIVE: JNU_ClusterNode_set(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNode_set_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNode_set_4(JNIEnv *jniEnv,
				      jobject sClusterNode,
				      const SaClmClusterNodeT_4
				      *saClusterNodePtr)
{

	// VARIABLES
	// JNI
	jstring _nodeNameString;
	jstring _nodeAddressString;
	jobject _sNodeAddress;
	jstring _executionEnvironment;

	// BODY

	assert(sClusterNode != NULL);
	assert(saClusterNodePtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNode_set_4(...)\n");

	U_printSaClusterNode_4("NATIVE: Native Cluster Node info: \n",
			       saClusterNodePtr);

	// nodeId
	(*jniEnv)->SetIntField(jniEnv,
			       sClusterNode,
			       FID_nodeId, (jint)saClusterNodePtr->nodeId);
	// nodeAddress
	_nodeAddressString = JNU_newStringFromSaNodeAddressT(jniEnv,
							     &
							     (saClusterNodePtr->
							      nodeAddress));
	if (_nodeAddressString == NULL) {
		// TODO error handling

		_TRACE2("NATIVE ERROR: _nodeAddressString is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	_sNodeAddress = (saClusterNodePtr->nodeAddress.family == SA_CLM_AF_INET)
		? (*jniEnv)->NewObject(jniEnv,
				       ClassNodeAddressIPv4,
				       CID_NodeAddressIPv4_constructor)
		: (*jniEnv)->NewObject(jniEnv,
				       ClassNodeAddressIPv4,
				       CID_NodeAddressIPv4_constructor);

	if (_sNodeAddress == NULL) {

		_TRACE2("NATIVE ERROR: _sNodeAddress is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  _sNodeAddress,
				  FID_NodeAddress_value, _nodeAddressString);
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNode, FID_nodeAddress, _sNodeAddress);
	// nodeName
	_nodeNameString = JNU_newStringFromSaNameT(jniEnv,
						   &(saClusterNodePtr->
						     nodeName));
	if (_nodeNameString == NULL) {
		// TODO error handling
		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNode, FID_nodeName, _nodeNameString);
	// member
	(*jniEnv)->SetBooleanField(jniEnv,
				   sClusterNode,
				   FID_member,
				   (jboolean)saClusterNodePtr->member);

	//executionEnvironment
	_executionEnvironment = JNU_newStringFromSaNameT(jniEnv,
							 &(saClusterNodePtr->
							   executionEnvironment));

	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNode,
				  FID_executionEnvironment,
				  _executionEnvironment);

	// bootTimestamp
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNode,
				FID_bootTimestamp,
				(jlong)saClusterNodePtr->bootTimestamp);
	// initialViewNumber
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNode,
				FID_initialViewNumber,
				(jlong)saClusterNodePtr->initialViewNumber);

	_TRACE2("NATIVE: JNU_ClusterNode_set_4(...) returning normally\n");

	return JNI_TRUE;
}

//****************************
// CLASS ais.clm.NodeAddress
//****************************

/**************************************************************************
 * FUNCTION:      JNU_NodeAddress_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_NodeAddress_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_NodeAddress_initIDs_OK(...)\n");

	// get NodeAddress class & create a global reference right away
	/*
	  ClassNodeAddress =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/NodeAddress" )
	  ); */
	ClassNodeAddress = JNU_GetGlobalClassRef(jniEnv,
						 "org/saforum/ais/clm/NodeAddress");
	if (ClassNodeAddress == NULL) {

		_TRACE2("NATIVE ERROR: ClassNodeAddress is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_NodeAddress_initIDs_FromClass_OK(jniEnv, ClassNodeAddress);
}

/**************************************************************************
 * FUNCTION:      JNU_NodeAddress_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_NodeAddress_initIDs_FromClass_OK(JNIEnv *jniEnv,
						     jclass classNodeAddress)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_NodeAddress_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_NodeAddress_value = (*jniEnv)->GetFieldID(jniEnv,
						      classNodeAddress,
						      "value",
						      "Ljava/lang/String;");
	if (FID_NodeAddress_value == NULL) {

		_TRACE2("NATIVE: FID_NodeAddress_value is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_NodeAddress_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// CLASS ais.clm.NodeAddressIPv4
//********************************

/**************************************************************************
 * FUNCTION:      JNU_NodeAddressIPv4_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_NodeAddressIPv4_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_NodeAddressIPv4_initIDs_OK(...)\n");

	// get NodeAddressIPv4 class & create a global reference right away
	/*
	  ClassNodeAddressIPv4 =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/NodeAddressIPv4" )
	  ); */
	ClassNodeAddressIPv4 = JNU_GetGlobalClassRef(jniEnv,
						     "org/saforum/ais/clm/NodeAddressIPv4");
	if (ClassNodeAddressIPv4 == NULL) {

		_TRACE2("NATIVE ERROR: ClassNodeAddressIPv4 is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_NodeAddressIPv4_initIDs_FromClass_OK(jniEnv,
							ClassNodeAddressIPv4);
}

/**************************************************************************
 * FUNCTION:      JNU_NodeAddressIPv4_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_NodeAddressIPv4_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 classNodeAddressIPv4)
{

	_TRACE2
		("NATIVE: Executing JNU_NodeAddressIPv4_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_NodeAddressIPv4_constructor = (*jniEnv)->GetMethodID(jniEnv,
								 classNodeAddressIPv4,
								 "<init>",
								 "()V");
	if (CID_NodeAddressIPv4_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_NodeAddressIPv4_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_NodeAddressIPv4_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

//********************************
// CLASS ais.clm.NodeAddressIPv6
//********************************

/**************************************************************************
 * FUNCTION:      JNU_NodeAddressIPv6_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_NodeAddressIPv6_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_NodeAddressIPv6_initIDs_OK(...)\n");

	// get NodeAddressIPv6 class & create a global reference right away
	/*
	  ClassNodeAddressIPv6 =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/NodeAddressIPv6" )
	  ); */
	ClassNodeAddressIPv6 = JNU_GetGlobalClassRef(jniEnv,
						     "org/saforum/ais/clm/NodeAddressIPv6");
	if (ClassNodeAddressIPv6 == NULL) {

		_TRACE2("NATIVE ERROR: ClassNodeAddressIPv6 is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_NodeAddressIPv6_initIDs_FromClass_OK(jniEnv,
							ClassNodeAddressIPv6);
}

/**************************************************************************
 * FUNCTION:      JNU_NodeAddressIPv6_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_NodeAddressIPv6_initIDs_FromClass_OK(JNIEnv *jniEnv,
							 jclass
							 classNodeAddressIPv6)
{

	_TRACE2
		("NATIVE: Executing JNU_NodeAddressIPv6_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_NodeAddressIPv6_constructor = (*jniEnv)->GetMethodID(jniEnv,
								 classNodeAddressIPv6,
								 "<init>",
								 "()V");
	if (CID_NodeAddressIPv6_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_NodeAddressIPv6_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_NodeAddressIPv6_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

//******************************************
// CLASS ais.clm.ClusterNotificationBuffer
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterNotificationBuffer_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ClusterNotificationBuffer_initIDs_OK(...)\n");

	// get ClusterNotificationBuffer class & create a global reference right away
	/*
	  ClassClusterNotificationBuffer =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/ClusterNotificationBuffer" )
	  ); */
	ClassClusterNotificationBuffer = JNU_GetGlobalClassRef(jniEnv,
							       "org/saforum/ais/clm/ClusterNotificationBuffer");
	if (ClassClusterNotificationBuffer == NULL) {

		_TRACE2
			("NATIVE ERROR: ClassClusterNotificationBuffer is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ClusterNotificationBuffer_initIDs_FromClass_OK(jniEnv,
								  ClassClusterNotificationBuffer);
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNotificationBuffer_initIDs_FromClass_OK(JNIEnv
								   *jniEnv,
								   jclass
								   classClusterNotificationBuffer)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ClusterNotificationBuffer_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_ClusterNotificationBuffer_constructor =
		(*jniEnv)->GetMethodID(jniEnv, classClusterNotificationBuffer,
				       "<init>", "()V");
	if (CID_ClusterNotificationBuffer_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_ClusterNotificationBuffer_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_viewNumber = (*jniEnv)->GetFieldID(jniEnv,
					       classClusterNotificationBuffer,
					       "viewNumber", "J");
	if (FID_viewNumber == NULL) {

		_TRACE2("NATIVE ERROR: FID_viewNumber is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_notifications = (*jniEnv)->GetFieldID(jniEnv,
						  classClusterNotificationBuffer,
						  "notifications",
						  "[Lorg/saforum/ais/clm/ClusterNotification;");
	if (FID_notifications == NULL) {

		_TRACE2("NATIVE ERROR: FID_notifications is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotificationBuffer_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_create_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNotificationBuffer object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ClusterNotificationBuffer_create_4(JNIEnv *jniEnv,
						      const
						      SaClmClusterNotificationBufferT_4
						      *saNotificationBufferPtr)
{
	// VARIABLES
	// JNI
	jobject _sClusterNotificationBuffer;

	// BODY

	assert(saNotificationBufferPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_ClusterNotificationBuffer_create_4(...)\n");

	// create new ClusterNotificationBuffer object
	_sClusterNotificationBuffer = (*jniEnv)->NewObject(jniEnv,
							   ClassClusterNotificationBuffer,
							   CID_ClusterNotificationBuffer_constructor);
	if (_sClusterNotificationBuffer == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNotificationBuffer is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNotificationBuffer object
	if (JNU_ClusterNotificationBuffer_set_4(jniEnv,
						_sClusterNotificationBuffer,
						saNotificationBufferPtr) !=
	    JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotificationBuffer_create_4(...) returning normally\n");

	return _sClusterNotificationBuffer;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNotificationBuffer object or NULL
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ClusterNotificationBuffer_create(JNIEnv *jniEnv,
						    const
						    SaClmClusterNotificationBufferT
						    *saNotificationBufferPtr)
{
	// VARIABLES
	// JNI
	jobject _sClusterNotificationBuffer;

	// BODY

	assert(saNotificationBufferPtr != NULL);
	_TRACE2
		("NATIVE: Executing JNU_ClusterNotificationBuffer_create(...)\n");

	// create new ClusterNotificationBuffer object
	_sClusterNotificationBuffer = (*jniEnv)->NewObject(jniEnv,
							   ClassClusterNotificationBuffer,
							   CID_ClusterNotificationBuffer_constructor);
	if (_sClusterNotificationBuffer == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNotificationBuffer is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNotificationBuffer object
	if (JNU_ClusterNotificationBuffer_set(jniEnv,
					      _sClusterNotificationBuffer,
					      saNotificationBufferPtr) !=
	    JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotificationBuffer_create(...) returning normally\n");

	return _sClusterNotificationBuffer;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_set_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterNotificationBuffer_set_4(JNIEnv *jniEnv,
					     jobject sClusterNotificationBuffer,
					     const
					     SaClmClusterNotificationBufferT_4
					     *saNotificationBufferPtr)
{
	// VARIABLES
	unsigned int _idx;
	// JNI
	jobjectArray _arraySClusterNotification;
	jobject _sClusterNotification;

	// BODY

	assert(sClusterNotificationBuffer != NULL);
	assert(saNotificationBufferPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotificationBuffer_set_4(...)\n");
	U_printSaClusterNotificationBuffer_4
		("Input values of saNotificationBuffer: \n",
		 saNotificationBufferPtr);

	// viewNumber
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNotificationBuffer,
				FID_viewNumber,
				(jlong)saNotificationBufferPtr->viewNumber);

	// create notification array
	_arraySClusterNotification = (*jniEnv)->NewObjectArray(jniEnv,
							       saNotificationBufferPtr->
							       numberOfItems,
							       ClassClusterNotification,
							       NULL);
	if (_arraySClusterNotification == NULL) {

		_TRACE2("NATIVE ERROR: _arraySClusterNotification is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	// assign array
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotificationBuffer,
				  FID_notifications,
				  _arraySClusterNotification);
	// create & assign array elements
	for (_idx = 0; _idx < saNotificationBufferPtr->numberOfItems; _idx++) {
		_sClusterNotification = JNU_ClusterNotification_create_4(jniEnv,
									 &
									 (saNotificationBufferPtr->
									  notification
									  [_idx]));
		if (_sClusterNotification == NULL) {

			_TRACE2
				("NATIVE ERROR: _sClusterNotification[%d] is NULL\n",
				 _idx);

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectArrayElement(jniEnv,
						 _arraySClusterNotification,
						 (jsize)_idx,
						 _sClusterNotification);
		(*jniEnv)->DeleteLocalRef(jniEnv, _sClusterNotification);
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotificationBuffer_set_4(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotificationBuffer_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterNotificationBuffer_set(JNIEnv *jniEnv,
					   jobject sClusterNotificationBuffer,
					   const SaClmClusterNotificationBufferT
					   *saNotificationBufferPtr)
{
	// VARIABLES
	unsigned int _idx;
	// JNI
	jobjectArray _arraySClusterNotification;
	jobject _sClusterNotification;

	// BODY

	assert(sClusterNotificationBuffer != NULL);
	assert(saNotificationBufferPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotificationBuffer_set(...)\n");
	U_printSaClusterNotificationBuffer
		("Input values of saNotificationBuffer: \n",
		 saNotificationBufferPtr);

	// viewNumber
	(*jniEnv)->SetLongField(jniEnv,
				sClusterNotificationBuffer,
				FID_viewNumber,
				(jlong)saNotificationBufferPtr->viewNumber);

	// create notification array
	_arraySClusterNotification = (*jniEnv)->NewObjectArray(jniEnv,
							       saNotificationBufferPtr->
							       numberOfItems,
							       ClassClusterNotification,
							       NULL);
	if (_arraySClusterNotification == NULL) {

		_TRACE2("NATIVE ERROR: _arraySClusterNotification is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	// assign array
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotificationBuffer,
				  FID_notifications,
				  _arraySClusterNotification);
	// create & assign array elements
	for (_idx = 0; _idx < saNotificationBufferPtr->numberOfItems; _idx++) {
		_sClusterNotification = JNU_ClusterNotification_create(jniEnv,
								       &
								       (saNotificationBufferPtr->
									notification
									[_idx]));
		if (_sClusterNotification == NULL) {

			_TRACE2
				("NATIVE ERROR: _sClusterNotification[%d] is NULL\n",
				 _idx);

			return JNI_FALSE;	// OutOfMemoryError thrown already...
		}
		(*jniEnv)->SetObjectArrayElement(jniEnv,
						 _arraySClusterNotification,
						 (jsize)_idx,
						 _sClusterNotification);
		(*jniEnv)->DeleteLocalRef(jniEnv, _sClusterNotification);
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotificationBuffer_set(...) returning normally\n");

	return JNI_TRUE;
}

//******************************************
// CLASS ais.clm.ClusterNotification
//******************************************

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterNotification_initIDs_OK(JNIEnv *jniEnv)
{
	// BODY

	_TRACE2("NATIVE: Executing JNU_ClusterNotification_initIDs_OK(...)\n");

	// get ClusterNotification class & create a global reference right away
	/*
	  ClassClusterNotification =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/saforum/ais/clm/ClusterNotification" )
	  ); */
	ClassClusterNotification = JNU_GetGlobalClassRef(jniEnv,
							 "org/saforum/ais/clm/ClusterNotification");
	if (ClassClusterNotification == NULL) {

		_TRACE2("NATIVE ERROR: ClassClusterNotification is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ClusterNotification_initIDs_FromClass_OK(jniEnv,
							    ClassClusterNotification);
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNotification_initIDs_FromClass_OK(JNIEnv *jniEnv,
							     jclass
							     classClusterNotification)
{
	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ClusterNotification_initIDs_FromClass_OK(...)\n");

	// get constructor IDs
	CID_ClusterNotification_constructor = (*jniEnv)->GetMethodID(jniEnv,
								     classClusterNotification,
								     "<init>",
								     "()V");
	if (CID_ClusterNotification_constructor == NULL) {

		_TRACE2
			("NATIVE ERROR: CID_ClusterNotification_constructor is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_clusterChange = (*jniEnv)->GetFieldID(jniEnv,
						  classClusterNotification,
						  "clusterChange",
						  "Lorg/saforum/ais/clm/ClusterNotification$ClusterChange;");
	if (FID_clusterChange == NULL) {

		_TRACE2("NATIVE ERROR: FID_clusterChange is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	FID_clusterNode = (*jniEnv)->GetFieldID(jniEnv,
						classClusterNotification,
						"clusterNode",
						"Lorg/saforum/ais/clm/ClusterNode;");
	if (FID_clusterNode == NULL) {

		_TRACE2("NATIVE ERROR: FID_clusterNode is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotification_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_create
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNotification object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ClusterNotification_create(JNIEnv *jniEnv,
					      const SaClmClusterNotificationT
					      *saNotificationPtr)
{

	// VARIABLES
	// JNI
	jobject _sClusterNotification;

	// BODY

	assert(saNotificationPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotification_create(...)\n");
	U_printSaClusterNotification
		("NATIVE: Input value of saNotification: \n", saNotificationPtr);

	// create new ClusterNotification object
	_sClusterNotification = (*jniEnv)->NewObject(jniEnv,
						     ClassClusterNotification,
						     CID_ClusterNotification_constructor);
	if (_sClusterNotification == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNotification is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNotification object
	if (JNU_ClusterNotification_set(jniEnv,
					_sClusterNotification,
					saNotificationPtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotification_create(...) returning normally\n");

	return _sClusterNotification;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_create_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     the created ClusterNotification object or NULL
 * NOTE: If NULL is returned, then an exception is already pending!
 *************************************************************************/
static jobject JNU_ClusterNotification_create_4(JNIEnv *jniEnv,
						const
						SaClmClusterNotificationT_4
						*saNotificationPtr)
{

	// VARIABLES
	// JNI
	jobject _sClusterNotification;

	// BODY

	assert(saNotificationPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotification_create_4(...)\n");
	U_printSaClusterNotification_4
		("NATIVE: Input value of saNotification: \n", saNotificationPtr);

	// create new ClusterNotification object
	_sClusterNotification = (*jniEnv)->NewObject(jniEnv,
						     ClassClusterNotification,
						     CID_ClusterNotification_constructor);
	if (_sClusterNotification == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNotification is NULL\n");

		return NULL;	// EXIT POINT! Exception pending...
	}
	// set ClusterNotification object
	if (JNU_ClusterNotification_set_4(jniEnv,
					  _sClusterNotification,
					  saNotificationPtr) != JNI_TRUE) {
		// TODO error handling
		return NULL;	// exception thrown already...
	}

	_TRACE2
		("NATIVE: JNU_ClusterNotification_create_4(...) returning normally\n");

	return _sClusterNotification;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_set
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNotification_set(JNIEnv *jniEnv,
					    jobject sClusterNotification,
					    const SaClmClusterNotificationT
					    *saClusterNotificationPtr)
{
	// VARIABLES
	// JNI
	jobject _sClusterNode;

	// BODY

	assert(sClusterNotification != NULL);
	assert(saClusterNotificationPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotification_set(...)\n");
	U_printSaClusterNotification
		("NATIVE: Input value of saNotification: \n",
		 saClusterNotificationPtr);

	// create an enum from an int
	jobject _clusterChange = (*jniEnv)->CallStaticObjectMethod(jniEnv,
								   ClassClmHandle,
								   MID_s_getClusterChange,
								   saClusterNotificationPtr->
								   clusterChange);

	assert(_clusterChange != NULL);

	// clusterChange
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotification,
				  FID_clusterChange, _clusterChange);
	// clusterNode
	_sClusterNode = JNU_ClusterNode_create(jniEnv,
					       &(saClusterNotificationPtr->
						 clusterNode));
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNode is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotification,
				  FID_clusterNode, _sClusterNode);

	_TRACE2
		("NATIVE: JNU_ClusterNotification_set(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:      JNU_ClusterNotification_set_4
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterNotification_set_4(JNIEnv *jniEnv,
					      jobject sClusterNotification,
					      const SaClmClusterNotificationT_4
					      *saClusterNotificationPtr)
{
	// VARIABLES
	// JNI
	jobject _sClusterNode;

	// BODY

	assert(sClusterNotification != NULL);
	assert(saClusterNotificationPtr != NULL);
	_TRACE2("NATIVE: Executing JNU_ClusterNotification_set_4(...)\n");
	U_printSaClusterNotification_4
		("NATIVE: Input value of saNotification: \n",
		 saClusterNotificationPtr);

	// create an enum from an int
	jobject _clusterChange = (*jniEnv)->CallStaticObjectMethod(jniEnv,
								   ClassClmHandle,
								   MID_s_getClusterChange,
								   saClusterNotificationPtr->
								   clusterChange);

	assert(_clusterChange != NULL);

	// clusterChange
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotification,
				  FID_clusterChange, _clusterChange);
	// clusterNode
	_sClusterNode = JNU_ClusterNode_create_4(jniEnv,
						 &(saClusterNotificationPtr->
						   clusterNode));
	if (_sClusterNode == NULL) {

		_TRACE2("NATIVE ERROR: _sClusterNode is NULL\n");

		return JNI_FALSE;	// OutOfMemoryError thrown already...
	}
	(*jniEnv)->SetObjectField(jniEnv,
				  sClusterNotification,
				  FID_clusterNode, _sClusterNode);

	_TRACE2
		("NATIVE: JNU_ClusterNotification_set_4(...) returning normally\n");

	return JNI_TRUE;
}
