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

#ifndef J_AIS_CLM_LIBHANDLE_H_
#define J_AIS_CLM_LIBHANDLE_H_

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

// CLASS ais.clm.ClmHandle
extern jclass ClassClmHandle;
extern jmethodID MID_s_invokeGetClusterNodeCallback;
extern jmethodID MID_s_invokeTrackClusterCallback;
extern jmethodID MID_s_invokeTrackClusterCallback_4;
extern jfieldID FID_saClmHandle;
extern jfieldID FID_saVersion;
// MODIFICATION: Added to return ClusterChange instance
extern jmethodID MID_s_getClusterChange;

/*************************************************************************
 * Function declarations
 *************************************************************************/

extern jboolean JNU_ClmHandle_initIDs_OK(JNIEnv *jniEnv);

#endif				// J_AIS_CLM_LIBHANDLE_H_
