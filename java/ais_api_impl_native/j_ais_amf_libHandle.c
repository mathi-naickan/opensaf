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
#include <stdlib.h>
#include <errno.h>
#include <assert.h>
#include "j_utilsPrint.h"

#include <errno.h>
#include <saAmf.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "j_ais_amf.h"
#include "jni_ais_amf.h"
#include "osaf_poll.h"

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

// CLASS ais.amf.AmfHandle
jclass ClassAmfHandle = NULL;
jmethodID MID_s_invokeHealthcheckCallback;
jmethodID MID_s_invokeSetCsiCallback;
jmethodID MID_s_invokeRemoveCsiCallback;
jmethodID MID_s_invokeTerminateComponentCallback;
jmethodID MID_s_invokeInstantiateProxiedComponentCallback;
jmethodID MID_s_invokeCleanupProxiedComponentCallback;
jmethodID MID_s_invokeTrackProtectionGroupCallback;
jfieldID FID_saAmfHandle = NULL;
static jfieldID FID_healthcheckCallback = NULL;
static jfieldID FID_setCsiCallback = NULL;
static jfieldID FID_removeCsiCallback = NULL;
static jfieldID FID_terminateComponentCallback = NULL;
static jfieldID FID_instantiateProxiedComponentCallback = NULL;
static jfieldID FID_cleanupProxiedComponentCallback = NULL;
static jfieldID FID_trackProtectionGroupCallback = NULL;
static jfieldID FID_selectionObject = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.AmfHandle
jboolean JNU_AmfHandle_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_AmfHandle_initIDs_FromClass_OK(JNIEnv *jniEnv,
						   jclass classAmfHandle);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//********************************
// CLASS ais.amf.AmfHandle
//********************************

/**************************************************************************
 * FUNCTION:      JNU_AmfHandle_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_AmfHandle_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_AmfHandle_initIDs_OK(...)\n");

	// get AmfHandle class & create a global reference right away
	/*
	  ClassAmfHandle =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "org/opensaf/ais/amf/AmfHandleImpl" )
	  ); */
	ClassAmfHandle = JNU_GetGlobalClassRef(jniEnv,
					       "org/opensaf/ais/amf/AmfHandleImpl");
	if (ClassAmfHandle == NULL) {

		_TRACE2("NATIVE ERROR: ClassAmfHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_AmfHandle_initIDs_FromClass_OK(jniEnv, ClassAmfHandle);

}

/**************************************************************************
 * FUNCTION:      JNU_AmfLHandle_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_AmfHandle_initIDs_FromClass_OK(JNIEnv *jniEnv,
						   jclass classAmfHandle)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_AmfHandle_initIDs_FromClass_OK(...)\n");

	// get method IDs
	MID_s_invokeHealthcheckCallback = (*jniEnv)->GetStaticMethodID(jniEnv,
								       classAmfHandle,
								       "s_invokeHealthcheckCallback",
								       "(JLjava/lang/String;[B)V");
	if (MID_s_invokeHealthcheckCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeHealthcheckCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeSetCsiCallback = (*jniEnv)->GetStaticMethodID(jniEnv,
								  classAmfHandle,
								  "s_invokeSetCsiCallback",
								  "(JLjava/lang/String;Lorg/saforum/ais/amf/HaState;Lorg/saforum/ais/amf/CsiDescriptor;)V");
	if (MID_s_invokeSetCsiCallback == NULL) {

		_TRACE2("NATIVE ERROR: MID_s_invokeSetCsiCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeRemoveCsiCallback = (*jniEnv)->GetStaticMethodID(jniEnv,
								     classAmfHandle,
								     "s_invokeRemoveCsiCallback",
								     "(JLjava/lang/String;Ljava/lang/String;Lorg/saforum/ais/amf/CsiDescriptor$CsiFlags;)V");
	if (MID_s_invokeRemoveCsiCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeRemoveCsiCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeTerminateComponentCallback =
		(*jniEnv)->GetStaticMethodID(jniEnv, classAmfHandle,
					     "s_invokeTerminateComponentCallback",
					     "(JLjava/lang/String;)V");
	if (MID_s_invokeTerminateComponentCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeTerminateComponentCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeInstantiateProxiedComponentCallback =
		(*jniEnv)->GetStaticMethodID(jniEnv, classAmfHandle,
					     "s_invokeInstantiateProxiedComponentCallback",
					     "(JLjava/lang/String;)V");
	if (MID_s_invokeInstantiateProxiedComponentCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeInstantiateProxiedComponentCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeCleanupProxiedComponentCallback =
		(*jniEnv)->GetStaticMethodID(jniEnv, classAmfHandle,
					     "s_invokeCleanupProxiedComponentCallback",
					     "(JLjava/lang/String;)V");
	if (MID_s_invokeCleanupProxiedComponentCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeCleanupProxiedComponentCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	MID_s_invokeTrackProtectionGroupCallback =
		(*jniEnv)->GetStaticMethodID(jniEnv, classAmfHandle,
					     "s_invokeTrackProtectionGroupCallback",
					     "(Ljava/lang/String;[Lorg/saforum/ais/amf/ProtectionGroupNotification;II)V");
	if (MID_s_invokeTrackProtectionGroupCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: MID_s_invokeTrackProtectionGroupCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get field IDs
	FID_healthcheckCallback = (*jniEnv)->GetFieldID(jniEnv,
							classAmfHandle,
							"healthcheckCallback",
							"Lorg/saforum/ais/amf/HealthcheckCallback;");
	if (FID_healthcheckCallback == NULL) {

		_TRACE2("NATIVE ERROR: FID_healthcheckCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_setCsiCallback = (*jniEnv)->GetFieldID(jniEnv,
						   classAmfHandle,
						   "setCsiCallback",
						   "Lorg/saforum/ais/amf/SetCsiCallback;");
	if (FID_setCsiCallback == NULL) {

		_TRACE2("NATIVE ERROR: FID_setCsiCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_removeCsiCallback = (*jniEnv)->GetFieldID(jniEnv,
						      classAmfHandle,
						      "removeCsiCallback",
						      "Lorg/saforum/ais/amf/RemoveCsiCallback;");
	if (FID_removeCsiCallback == NULL) {

		_TRACE2("NATIVE ERROR: FID_removeCsiCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_terminateComponentCallback = (*jniEnv)->GetFieldID(jniEnv,
							       classAmfHandle,
							       "terminateComponentCallback",
							       "Lorg/saforum/ais/amf/TerminateComponentCallback;");
	if (FID_terminateComponentCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: FID_terminateComponentCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_instantiateProxiedComponentCallback = (*jniEnv)->GetFieldID(jniEnv,
									classAmfHandle,
									"instantiateProxiedComponentCallback",
									"Lorg/saforum/ais/amf/InstantiateProxiedComponentCallback;");
	if (FID_instantiateProxiedComponentCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: FID_instantiateProxiedCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_cleanupProxiedComponentCallback = (*jniEnv)->GetFieldID(jniEnv,
								    classAmfHandle,
								    "cleanupProxiedComponentCallback",
								    "Lorg/saforum/ais/amf/CleanupProxiedComponentCallback;");
	if (FID_cleanupProxiedComponentCallback == NULL) {

		_TRACE2("NATIVE ERROR: FID_cleanupProxiedCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_trackProtectionGroupCallback = (*jniEnv)->GetFieldID(jniEnv,
								 classAmfHandle,
								 "trackProtectionGroupCallback",
								 "Lorg/saforum/ais/amf/TrackProtectionGroupCallback;");
	if (FID_trackProtectionGroupCallback == NULL) {

		_TRACE2
			("NATIVE ERROR: FID_trackProtectionGroupCallback is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_saAmfHandle = (*jniEnv)->GetFieldID(jniEnv,
						classAmfHandle,
						"saAmfHandle", "J");
	if (FID_saAmfHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_saAmfHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	//
	FID_selectionObject = (*jniEnv)->GetFieldID(jniEnv,
						    classAmfHandle,
						    "selectionObject", "J");
	if (FID_selectionObject == NULL) {

		_TRACE2("NATIVE ERROR: FID_selectionObject is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_AmfHandle_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfInitialize
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    invokeSaAmfInitialize
 *  Signature: (Lorg/saforum/ais/Version;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfInitialize(JNIEnv *jniEnv,
							     jobject
							     thisAmfHandle,
							     jobject sVersion)
{
	// VARIABLES
	// ais
	SaAmfHandleT _saAmfHandle;
	SaAmfCallbacksT _saAmfCallbacks;
	SaVersionT _saVersion;
	SaAisErrorT _saStatus;
	// jni
	jobject _healthcheckCallback;
	jobject _setCsiCallback;
	jobject _removeCsiCallback;
	jobject _terminateComponentCallback;
	jobject _instantiateProxiedComponentCallback;
	jobject _cleanupProxiedComponentCallback;
	jobject _trackProtectionGroupCallback;
	jchar _releaseCode;
	jshort _majorVersion;
	jshort _minorVersion;
	jclass _classSystem;
	jstring _componentName;
	jstring _componentNameProperty;
	jmethodID MID_getSystemProperty;

	// BODY

	assert(thisAmfHandle != NULL);
	// TODO assert for sVersion
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfInitialize(...)\n");

	// check if the java system property org.saforum.ais.amf.component-name is set

	// get the java.lang.System class
	_classSystem = (*jniEnv)->FindClass(jniEnv, "java/lang/System");
	if (_classSystem == NULL) {

		_TRACE2("NATIVE ERROR: _classSystem is NULL\n");

		return;		// EXIT POINT! Exception pending...
	}
	// Get a jstring with the name of the component name property
	_componentNameProperty =
		JNU_NewStringNative(jniEnv, "org.saforum.ais.amf.component-name");
	if (_componentNameProperty == NULL) {

		_TRACE2("NATIVE ERROR: _componentNameProperty is NULL\n");

		return;		// EXIT POINT! Exception pending...
	}
	// Get the getProperty method
	MID_getSystemProperty = (*jniEnv)->GetStaticMethodID(jniEnv,
							     _classSystem,
							     "getProperty",
							     "(Ljava/lang/String;)Ljava/lang/String;");
	if (MID_getSystemProperty == NULL) {

		_TRACE2("NATIVE ERROR: MID_getSystemProperty is NULL\n");

		return;		// EXIT POINT! Exception pending...
	}
	// Invoke the getProperty method
	_componentName = (*jniEnv)->CallStaticObjectMethod(jniEnv,
							   _classSystem,
							   MID_getSystemProperty,
							   _componentNameProperty);

	// If the property was set, set the component name environment variable to its value
	if (_componentName != NULL) {
		int result;
		char *componentNameChars;

		componentNameChars =
			JNU_GetStringNativeChars(jniEnv, _componentName);

		result = setenv("SA_AMF_COMPONENT_NAME", componentNameChars, 1);

		// Return and throw an exception if the setenv call failed
		if (-1 == result) {
			_TRACE2
				("NATIVE ERROR: failed to set the SA_AMF_COMPONENT_NAME env variable to %s. Error %i\n",
				 _componentName, errno);
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisErrorLibraryException",
					   AIS_ERR_TRY_AGAIN_MSG);
			return;	// EXIT POINT! Exception thrown...
		}

		free(componentNameChars);
	}
	// create callback struct
	//  healthcheck cb
	_healthcheckCallback = (*jniEnv)->GetObjectField(jniEnv,
							 thisAmfHandle,
							 FID_healthcheckCallback);
	if (_healthcheckCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfHealthcheckCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfHealthcheckCallback =
			SaAmfHealthcheckCallback;
	} else {

		_TRACE2("NATIVE: NO SaAmfHealthcheckCallback assigned!\n");

		_saAmfCallbacks.saAmfHealthcheckCallback = NULL;
	}
	// set CSI cb
	_setCsiCallback = (*jniEnv)->GetObjectField(jniEnv,
						    thisAmfHandle,
						    FID_setCsiCallback);
	if (_setCsiCallback != NULL) {

		_TRACE2("NATIVE: SaAmfCSISetCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfCSISetCallback = SaAmfCSISetCallback;
	} else {

		_TRACE2("NATIVE: NO SaAmfCSISetCallback assigned!\n");

		_saAmfCallbacks.saAmfCSISetCallback = NULL;
	}
	// remove CSI cb
	_removeCsiCallback = (*jniEnv)->GetObjectField(jniEnv,
						       thisAmfHandle,
						       FID_removeCsiCallback);
	if (_removeCsiCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfCSIRemoveCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfCSIRemoveCallback = SaAmfCSIRemoveCallback;
	} else {

		_TRACE2("NATIVE: NO SaAmfCSIRemoveCallback assigned!\n");

		_saAmfCallbacks.saAmfCSIRemoveCallback = NULL;
	}
	// terminate component cb
	_terminateComponentCallback = (*jniEnv)->GetObjectField(jniEnv,
								thisAmfHandle,
								FID_terminateComponentCallback);
	if (_terminateComponentCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfComponentTerminateCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfComponentTerminateCallback =
			SaAmfComponentTerminateCallback;
	} else {

		_TRACE2
			("NATIVE: NO SaAmfComponentTerminateCallback assigned!\n");

		_saAmfCallbacks.saAmfComponentTerminateCallback = NULL;
	}
	// instantiate proxied component cb
	_instantiateProxiedComponentCallback = (*jniEnv)->GetObjectField(jniEnv,
									 thisAmfHandle,
									 FID_instantiateProxiedComponentCallback);
	if (_instantiateProxiedComponentCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfProxiedComponentInstantiateCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfProxiedComponentInstantiateCallback =
			SaAmfProxiedComponentInstantiateCallback;
	} else {

		_TRACE2
			("NATIVE: NO saAmfProxiedComponentInstantiateCallback assigned!\n");

		_saAmfCallbacks.saAmfProxiedComponentInstantiateCallback = NULL;
	}
	// cleanup proxied component cb
	_cleanupProxiedComponentCallback = (*jniEnv)->GetObjectField(jniEnv,
								     thisAmfHandle,
								     FID_cleanupProxiedComponentCallback);
	if (_cleanupProxiedComponentCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfProxiedComponentCleanupCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfProxiedComponentCleanupCallback =
			SaAmfProxiedComponentCleanupCallback;
	} else {

		_TRACE2
			("NATIVE: NO saAmfProxiedComponentCleanupCallback assigned!\n");

		_saAmfCallbacks.saAmfProxiedComponentCleanupCallback = NULL;
	}
	//  track protection group cb
	_trackProtectionGroupCallback = (*jniEnv)->GetObjectField(jniEnv,
								  thisAmfHandle,
								  FID_trackProtectionGroupCallback);
	if (_trackProtectionGroupCallback != NULL) {

		_TRACE2
			("NATIVE: SaAmfProtectionGroupTrackCallback assigned as callback!\n");

		_saAmfCallbacks.saAmfProtectionGroupTrackCallback =
			SaAmfProtectionGroupTrackCallback;
	} else {

		_TRACE2
			("NATIVE: NO SaAmfProtectionGroupTrackCallback assigned!\n");

		_saAmfCallbacks.saAmfProtectionGroupTrackCallback = NULL;
	}

	// create version struct
	if (sVersion == NULL) {
		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisInvalidParamException",
				   AIS_ERR_INVALID_PARAM_MSG);
		return;		// EXIT POINT!
	}
	// release code
	_releaseCode = (*jniEnv)->GetCharField(jniEnv,
					       sVersion, FID_releaseCode);
	_saVersion.releaseCode = (SaUint8T)_releaseCode;
	// major version
	_majorVersion = (*jniEnv)->GetShortField(jniEnv,
						 sVersion, FID_majorVersion);
	_saVersion.majorVersion = (SaUint16T)_majorVersion;
	// minor version
	_minorVersion = (*jniEnv)->GetShortField(jniEnv,
						 sVersion, FID_minorVersion);
	_saVersion.minorVersion = (SaUint16T)_minorVersion;

	// call saAmfInitialize
	_saStatus = saAmfInitialize(&_saAmfHandle,
				    &_saAmfCallbacks, &_saVersion);

	_TRACE2("NATIVE: saAmfInitialize(...) has returned with %d...\n",
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
		case SA_AIS_ERR_INVALID_PARAM:
			// TODO unclear whether this can happen here or not:
			// probably not, but native API declares this, so leave it for now
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
		case SA_AIS_ERR_VERSION:
			/* set version param:
			   see page 20 of the Cluster Membership Service spec B.01.01! */
			// major version
			if (_saVersion.releaseCode != _releaseCode) {
				(*jniEnv)->SetCharField(jniEnv,
							sVersion,
							FID_releaseCode,
							(jchar)_saVersion.
							releaseCode);
			}
			if (_saVersion.majorVersion != _majorVersion) {
				(*jniEnv)->SetShortField(jniEnv,
							 sVersion,
							 FID_majorVersion,
							 (jshort)_saVersion.
							 majorVersion);
			}
			// minor version
			if (_saVersion.minorVersion != _minorVersion) {
				(*jniEnv)->SetShortField(jniEnv,
							 sVersion,
							 FID_minorVersion,
							 (jshort)_saVersion.
							 minorVersion);
			}
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisVersionException",
					   AIS_ERR_VERSION_MSG);
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
	// set library handle

	_TRACE2("NATIVE: Retreived amfHandle is: %lu \n",
		(unsigned long)_saAmfHandle);

	(*jniEnv)->SetLongField(jniEnv,
				thisAmfHandle,
				FID_saAmfHandle, (jlong)_saAmfHandle);
	/* set version param:
	   see page 19 of the Cluster Membership Service spec B.01.01! */
	// major version
	if (_saVersion.majorVersion != _majorVersion) {
		(*jniEnv)->SetShortField(jniEnv,
					 sVersion,
					 FID_majorVersion,
					 (jshort)_saVersion.majorVersion);
	}
	// minor version
	if (_saVersion.minorVersion != _minorVersion) {
		(*jniEnv)->SetShortField(jniEnv,
					 sVersion,
					 FID_minorVersion,
					 (jshort)_saVersion.minorVersion);
	}
	// normal exit

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfInitialize(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfSelectionObjectGet
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    invokeSaAmfSelectionObjectGet
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfSelectionObjectGet(JNIEnv
								     *jniEnv,
								     jobject
								     thisAmfHandle)
{
	// VARIABLES
	// ais
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	SaSelectionObjectT _saSelectionObject;
	// jni

	// BODY

	assert(thisAmfHandle != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfSelectionObjectGet(...)\n");

	// get library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     thisAmfHandle,
							     FID_saAmfHandle);
	// call saAmfSelectionObjectGet
	_saStatus = saAmfSelectionObjectGet(_saAmfHandle, &_saSelectionObject);

	_TRACE2
		("NATIVE: saAmfSelectionObjectGet(...) has returned with %d...\n",
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
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			// unclear whether this can happen here or not: probably not...
			// TODO remove this from Java method declaration!
			//JNU_throwNewByName( jniEnv,
			//                    "org/saforum/ais/AisInvalidParamException",
			//                    AIS_ERR_INVALID_PARAM_MSG );
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
	// set selection object

	_TRACE2("NATIVE: Retreived selectionObject is: %lu \n",
		(unsigned long)_saSelectionObject);

	(*jniEnv)->SetLongField(jniEnv,
				thisAmfHandle,
				FID_selectionObject, (jlong)_saSelectionObject);
	// normal exit

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfSelectionObjectGet(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatch
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    invokeSaAmfDispatch
 *  Signature: (I)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatch(JNIEnv *jniEnv,
							   jobject
							   thisAmfHandle,
							   jint dispatchFlags)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;

	// BODY

	assert(thisAmfHandle != NULL);
	assert((dispatchFlags == SA_DISPATCH_ONE)
	       || (dispatchFlags == SA_DISPATCH_ALL)
	       || (dispatchFlags == SA_DISPATCH_BLOCKING));
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatch(...)\n");

	// get library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     thisAmfHandle,
							     FID_saAmfHandle);
	// call saAmfDispatch
	_saStatus = saAmfDispatch(_saAmfHandle,
				  (SaDispatchFlagsT)dispatchFlags);

	_TRACE2("NATIVE: saAmfDispatch(...) has returned with %d...\n",
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
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
			// TODO dispatch flags are invalid: this check could be done at Java level!
			//JNU_throwNewByName( jniEnv,
			//                    "org/saforum/ais/AisInvalidparamException",
			//                    AIS_ERR_INVALID_PARAM_MSG );
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
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatch() returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatchWhenReady
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    invokeSaAmfDispatchWhenReady
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatchWhenReady(JNIEnv
								    *jniEnv,
								    jobject
								    thisAmfHandle)
{
	// VARIABLES
	// ais
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	SaSelectionObjectT _saSelectionObject;
	// linux
	struct pollfd _readFDs;
	unsigned _pollStatus;

	// BODY

	assert(thisAmfHandle != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatchWhenReady(...)\n");

	// get library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     thisAmfHandle,
							     FID_saAmfHandle);
	// get selection object
	_saSelectionObject = (SaSelectionObjectT)(*jniEnv)->GetLongField(jniEnv,
									 thisAmfHandle,
									 FID_selectionObject);
	// wait until there is a pending callback
	// call osaf_poll()
	_readFDs.fd = _saSelectionObject;
	_readFDs.events = POLLIN;
	_pollStatus = osaf_poll(&_readFDs, 1, -1);

	_TRACE2("NATIVE: osaf_poll(...) has returned with %u...\n",
		_pollStatus);

	// error handling
	if (_pollStatus != 1) {
		_TRACE2("NATIVE ERROR : unexpected osaf_poll return value!\n");

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return;		// EXIT POINT!!!
	}
	// call saAmfDispatch
	_saStatus = saAmfDispatch(_saAmfHandle, SA_DISPATCH_ONE);

	_TRACE2("NATIVE: saAmfDispatch(...) has returned with %d...\n",
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
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
			break;
		case SA_AIS_ERR_INVALID_PARAM:
			// this should not happen here!

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisInvalidparamException",
					   AIS_ERR_INVALID_PARAM_MSG);
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
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_invokeSaAmfDispatchWhenReady() returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_finalizeAmfHandle
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    finalizeAmfHandle
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_AmfHandleImpl_finalizeAmfHandle(JNIEnv *jniEnv,
							 jobject thisAmfHandle)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;

	// BODY

	assert(thisAmfHandle != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_finalizeAmfHandle()\n");

	// get library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     thisAmfHandle,
							     FID_saAmfHandle);

	// call saAmfFinalize
	_saStatus = saAmfFinalize(_saAmfHandle);

	_TRACE2("NATIVE: saAmfFinalize(...) has returned with %d...\n",
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
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
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
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_finalizeAmfHandle() returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_AmfHandleImpl_response
 * TYPE:      native method
 *  Class:     ais_amf_AmfHandle
 *  Method:    response
 *  Signature: (JI)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_amf_AmfHandleImpl_response(JNIEnv
								       *jniEnv,
								       jobject
								       thisAmfHandle,
								       jlong
								       invocation,
								       jobject
								       error)
{
	// VARIABLES
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;

	SaAisErrorT _error;

	// BODY

	assert(thisAmfHandle != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_AmfHandleImpl_response()\n");

	// get library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     thisAmfHandle,
							     FID_saAmfHandle);

	// convert AisStatus -> int
	jclass aisStatusClass = JNU_GetGlobalClassRef(jniEnv,
						      "org/saforum/ais/AisStatus");
	jmethodID _MID_AisStatus_getValue = (*jniEnv)->GetMethodID(jniEnv,
								   aisStatusClass,
								   "getValue",
								   "()I");
	_error = (*jniEnv)->CallIntMethod(jniEnv,
					  error, _MID_AisStatus_getValue);

	// call saAmfResponse
	_saStatus = saAmfResponse(_saAmfHandle,
				  (SaInvocationT)invocation, _error);

	_TRACE2("NATIVE: saAmfResponse(...) has returned with %d...\n",
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
			// TODO library handle invalid (e.g finalized): this check could be done at Java level!
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadHandleException",
					   AIS_ERR_BAD_HANDLE_MSG);
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
		("NATIVE: Java_org_opensaf_ais_amf_AmfHandleImpl_response() returning normally\n");

}
