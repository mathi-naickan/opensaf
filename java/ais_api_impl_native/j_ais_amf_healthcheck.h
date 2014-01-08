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

#ifndef J_AIS_AMF_HEALTHCHECK_H_
#define J_AIS_AMF_HEALTHCHECK_H_

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

// CLASS ais.amf.Healthcheck
extern jboolean JNU_Healthcheck_initIDs_OK(JNIEnv *jniEnv);

// ENUM ais.amf.Healthcheck$HealthcheckInvocation
jboolean JNU_HealthcheckInvocation_initIDs_OK(JNIEnv *jniEnv);

#endif				// J_AIS_AMF_HEALTHCHECK_H_
