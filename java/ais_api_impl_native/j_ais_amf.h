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

#ifndef J_AIS_AMF_H_
#define J_AIS_AMF_H_

/**************************************************************************
 * Include files
 *************************************************************************/

#include <jni.h>
#include <saAmf.h>

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

// ENUM ais.amf.RecommendedRecovery
extern jfieldID FID_RR_value;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CALLBACKS
extern void SaAmfHealthcheckCallback(SaInvocationT saInvocation,
				     const SaNameT *saCompNamePtr,
				     SaAmfHealthcheckKeyT *saHealthcheckKeyPtr);
extern void SaAmfCSISetCallback(SaInvocationT saInvocation,
				const SaNameT *saCompNamePtr,
				SaAmfHAStateT saHaState,
				SaAmfCSIDescriptorT saCsiDescriptor);
extern void SaAmfCSIRemoveCallback(SaInvocationT saInvocation,
				   const SaNameT *saCompNamePtr,
				   const SaNameT *saCsiNamePtr,
				   SaAmfCSIFlagsT saCsiFlags);
extern void SaAmfComponentTerminateCallback(SaInvocationT saInvocation,
					    const SaNameT *saCompNamePtr);
extern void SaAmfProxiedComponentInstantiateCallback(SaInvocationT saInvocation,
						     const SaNameT
						     *saProxiedCompNamePtr);
extern void SaAmfProxiedComponentCleanupCallback(SaInvocationT saInvocation,
						 const SaNameT
						 *saProxiedCompNamePtr);
extern void SaAmfProtectionGroupTrackCallback(const SaNameT *saCsiNamePtr,
					      SaAmfProtectionGroupNotificationBufferT
					      *saNotificationBufferPtr,
					      SaUint32T saNumberOfMembers,
					      SaAisErrorT saError);

// CLASS ais.amf.CsiDescriptor
extern jboolean JNU_CsiDescriptor_initIDs_OK(JNIEnv *jniEnv);
extern jobject JNU_CsiDescriptor_create(JNIEnv *jniEnv,
					const SaAmfCSIDescriptorT
					*saCsiDescriptorPtr,
					SaAmfHAStateT saHaState);

// CLASS ais.amf.CsiActiveDescriptor
extern jboolean JNU_CsiActiveDescriptor_initIDs_OK(JNIEnv *jniEnv);
extern jobject JNU_CsiActiveDescriptor_create(JNIEnv *jniEnv,
					      const SaAmfCSIActiveDescriptorT
					      *saCsiActiveDescriptorPtr);

// ENUM ais.amf.CsiActiveDescriptor$TransitionDescriptor
extern jboolean JNU_TransitionDescriptor_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.amf.CsiStandbyDescriptor
extern jboolean JNU_CsiStandbyDescriptor_initIDs_OK(JNIEnv *jniEnv);
extern jobject JNU_CsiStandbyDescriptor_create(JNIEnv *jniEnv,
					       const SaAmfCSIStandbyDescriptorT
					       *saCsiStandbyDescriptorPtr);

// ENUM ais.amf.HaState
extern jboolean JNU_HaState_initIDs_OK(JNIEnv *jniEnv);
extern jobject JNU_HaState_getEnum(JNIEnv *jniEnv, SaAmfHAStateT saHaState);

// ENUM ais.amf.CsiDescriptor$CsiFlags
extern jboolean JNU_CsiFlags_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.amf.CsiAttribute
extern jboolean JNU_CsiAttribute_initIDs_OK(JNIEnv *jniEnv);

// ENUM ais.amf.RecommendedRecovery
extern jboolean JNU_RecommendedRecovery_initIDs_OK(JNIEnv *jniEnv);

#endif				//J_AIS_AMF_H_
