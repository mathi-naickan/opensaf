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

#ifndef J_AIS_AMF_PG_MANAGER_H_
#define J_AIS_AMF_PG_MANAGER_H_

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

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.ProtectionGroupManager
extern jboolean JNU_ProtectionGroupManager_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.amf.SProtectionGroupNotification
extern jboolean JNU_ProtectionGroupNotification_initIDs_OK(JNIEnv *jniEnv);

// CLASS ais.amf.SProtectionGroupMember
extern jboolean JNU_ProtectionGroupMember_initIDs_OK(JNIEnv *jniEnv);

// ENUM ais.amf.SProtectionGroupNotification$ProtectionGroupChanges
extern jboolean JNU_ProtectionGroupChanges_initIDs_OK(JNIEnv *jniEnv);

extern jobjectArray JNU_ProtectionGroupNotificationArray_create(JNIEnv *jniEnv,
								const
								SaAmfProtectionGroupNotificationBufferT
								*saNotificationBufferPtr);

#endif				// J_AIS_AMF_PG_MANAGER_H_
