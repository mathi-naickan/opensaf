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

#ifndef J_AIS_CLM_H_
#define J_AIS_CLM_H_

/**************************************************************************
 * Include files
 *************************************************************************/

#include <jni.h>

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

// CLASS ais.clm.ClusterNotificationBuffer
extern jclass ClassClusterNotificationBuffer;
extern jmethodID CID_ClusterNotificationBuffer_constructor;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CALLBACKS

extern void SaClmClusterNodeGetCallback(SaInvocationT saInvocation,
					const SaClmClusterNodeT
					*saClusterNodePtr, SaAisErrorT saError);

extern void SaClmClusterNodeGetCallback_4(SaInvocationT saInvocation,
					  const SaClmClusterNodeT_4
					  *saClusterNodePtr,
					  SaAisErrorT saError);

extern void SaClmClusterTrackCallback(const SaClmClusterNotificationBufferT
				      *saNotificationBufferPtr,
				      SaUint32T saNumberOfMembers,
				      SaAisErrorT saError);

extern void SaClmClusterTrackCallback_4(const SaClmClusterNotificationBufferT_4
					*saNotificationBufferPtr,
					SaUint32T saNumberOfMembers,
					SaInvocationT invocation,
					const SaNameT *rootCauseEntity,
					const SaNtfCorrelationIdsT
					*correlationIds, SaClmChangeStepT step,
					SaTimeT timeSupervision,
					SaAisErrorT saError);

// CLASS ais.clm.ClusterNode
extern jboolean JNU_ClusterNode_initIDs_OK(JNIEnv *jniEnv);
extern jobject JNU_ClusterNode_create(JNIEnv *jniEnv,
				      const SaClmClusterNodeT
				      *saClusterNodePtr);
extern jobject JNU_ClusterNode_create_4(JNIEnv *jniEnv,
					const SaClmClusterNodeT_4
					*saClusterNodePtr);

// CLASS ais.clm.NodeAddress
extern jboolean JNU_NodeAddress_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.clm.NodeAddressIPv4
extern jboolean JNU_NodeAddressIPv4_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.clm.NodeAddressIPv6
extern jboolean JNU_NodeAddressIPv6_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.clm.ClusterNotificationBuffer
extern jboolean JNU_ClusterNotificationBuffer_initIDs_OK(JNIEnv *jniEnv);

extern jboolean JNU_ClusterNotificationBuffer_set(JNIEnv *jniEnv,
						  jobject
						  sClusterNotificationBuffer,
						  const
						  SaClmClusterNotificationBufferT
						  *saNotificationBufferPtr);

extern jboolean JNU_ClusterNotificationBuffer_set_4(JNIEnv *jniEnv,
						    jobject
						    sClusterNotificationBuffer,
						    const
						    SaClmClusterNotificationBufferT_4
						    *saNotificationBufferPtr);

// CLASS ais.clm.ClusterNotification
extern jboolean JNU_ClusterNotification_initIDs_OK(JNIEnv *jniEnv);

#endif				//J_AIS_CLM_H_
