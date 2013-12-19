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

#ifndef J_UTILS_H_
#define J_UTILS_H_

/**************************************************************************
 * Include files
 *************************************************************************/

#include <jni.h>
#include <saAis.h>
#include <saClm.h>
#include <saCkpt.h>
#include <saAmf.h>

#include "tracer.h"
#include "j_utilsPrint.h"
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

extern JavaVM *cachedJVM;

/**************************************************************************
 * Function declarations
 *************************************************************************/

extern jstring JNU_NewStringNative(JNIEnv *env, const char *str);
extern char *JNU_GetStringNativeChars(JNIEnv *env, jstring jstr);
extern void JNU_throwNewByName(JNIEnv *jniEnv,
			       const char *className, const char *msg);
extern void JNU_ExceptionDescribeDoNotClear(JNIEnv *jniEnv);
extern jstring JNU_newStringFromSaNameT(JNIEnv *jniEnv,
					const SaNameT *saNamePtr);
extern jboolean JNU_copyFromStringToSaNameT(JNIEnv *jniEnv,
					    jstring fromStr,
					    SaNameT **toSaNamePtrPtr);
extern jboolean JNU_copyFromStringToSaNameT_NotNull(JNIEnv *jniEnv,
						    jstring fromStr,
						    SaNameT *toSaNamePtr);
extern jstring JNU_newStringFromSaNodeAddressT(JNIEnv *jniEnv,
					       const SaClmNodeAddressT
					       *saClmNodeAddressPtr);
extern jboolean JNU_jByteArray_link(JNIEnv *jniEnv, jbyteArray byteArray,
				    SaSizeT *saSizePtr, void **saBufferPtr);
extern void JNU_jByteArray_unlink(JNIEnv *jniEnv, jbyteArray byteArray,
				  void *saBuffer);
extern jint JNU_GetEnvForCallback(JavaVM *vmPtr, JNIEnv **envPtrPtr);
jclass JNU_GetGlobalClassRef(JNIEnv *jniEnv, const char *className);

#endif				//J_UTILS_H_
