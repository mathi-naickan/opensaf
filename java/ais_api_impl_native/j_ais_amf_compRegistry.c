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
#include "jni_ais_amf.h"	// not really needed, but good for syntax checking!

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

// CLASS ais.amf.ComponentRegistry
static jclass ClassComponentRegistry = NULL;
static jfieldID FID_amfLibraryHandle = NULL;
static jfieldID FID_componentName = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.amf.ComponentRegistry
jboolean JNU_ComponentRegistry_initIDs_OK(JNIEnv *jniEnv);
static jboolean JNU_ComponentRegistry_initIDs_FromClass_OK(JNIEnv *jniEnv,
							   jclass
							   classAmfHandle);

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.amf.ComponentRegistry
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_ComponentRegistry_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ComponentRegistry_initIDs_OK(JNIEnv *jniEnv)
{

	// BODY

	_TRACE2("NATIVE: Executing JNU_ComponentRegistry_initIDs_OK(...)\n");

	// get ComponentRegistry class & create a global reference right away
	/*
	  ClassComponentRegistry =
	  (*jniEnv)->NewGlobalRef( jniEnv,
	  (*jniEnv)->FindClass( jniEnv,
	  "ais/amf/ComponentRegistry" )
	  ); */
	ClassComponentRegistry = JNU_GetGlobalClassRef(jniEnv,
						       "org/opensaf/ais/amf/ComponentRegistryImpl");
	if (ClassComponentRegistry == NULL) {

		_TRACE2("NATIVE ERROR: ClassComponentRegistry is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	// get IDs
	return JNU_ComponentRegistry_initIDs_FromClass_OK(jniEnv,
							  ClassComponentRegistry);

}

/**************************************************************************
 * FUNCTION:      JNU_ComponentRegistry_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ComponentRegistry_initIDs_FromClass_OK(JNIEnv *jniEnv,
							   jclass
							   classComponentRegistry)
{

	// BODY

	_TRACE2
		("NATIVE: Executing JNU_ComponentRegistry_initIDs_FromClass_OK(...)\n");

	// get field IDs
	FID_amfLibraryHandle = (*jniEnv)->GetFieldID(jniEnv,
						     classComponentRegistry,
						     "amfLibraryHandle",
						     "Lorg/saforum/ais/amf/AmfHandle;");
	if (FID_amfLibraryHandle == NULL) {

		_TRACE2("NATIVE ERROR: FID_amfLibraryHandle is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}
	FID_componentName = (*jniEnv)->GetFieldID(jniEnv,
						  classComponentRegistry,
						  "componentName",
						  "Ljava/lang/String;");
	if (FID_componentName == NULL) {

		_TRACE2("NATIVE ERROR: FID_componentName is NULL\n");

		return JNI_FALSE;	// EXIT POINT! Exception pending...
	}

	_TRACE2
		("NATIVE: JNU_ComponentRegistry_initIDs_FromClass_OK(...) returning normally\n");

	return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ComponentRegistryImpl_invokeSaAmfComponentNameGet
 * TYPE:      native method
 *  Class:     ais_amf_ComponentRegistry
 *  Method:    invokeSaAmfComponentNameGet
 *  Signature: ()Ljava/lang/String;
 *************************************************************************/
JNIEXPORT jstring JNICALL
Java_org_opensaf_ais_amf_ComponentRegistryImpl_invokeSaAmfComponentNameGet
(JNIEnv *jniEnv, jobject thisComponentRegistry)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jstring _thisComponentName;

	// BODY

	assert(thisComponentRegistry != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ComponentRegistryImpl_invokeSaAmfComponentNameGet(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisComponentRegistry,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);

	// call saAmfComponentNameGet
	_saStatus = saAmfComponentNameGet(_saAmfHandle, &_saComponentName);

	_TRACE2("NATIVE: saAmfComponentNameGet(...) has returned with %d...\n",
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
		case SA_AIS_ERR_INVALID_PARAM:

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
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
		return NULL;	// EXIT POINT! Exception pending...
	}
	// return Component name
	_thisComponentName = JNU_newStringFromSaNameT(jniEnv,
						      &_saComponentName);
	if (_thisComponentName == NULL) {
		// this should not happen here!

		assert(JNI_FALSE);

		JNU_throwNewByName(jniEnv,
				   "org/saforum/ais/AisLibraryException",
				   AIS_ERR_LIBRARY_MSG);
		return NULL;	// EXIT POINT! Exception pending...
	}

	U_printSaName
		("NATIVE: Java_org_opensaf_ais_amf_ComponentRegistryImpl_invokeSaAmfComponentNameGet(...) returning normally with Component name: ",
		 &_saComponentName);

	return _thisComponentName;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerComponent
 * TYPE:      native method
 * Class:     ais_amf_ComponentRegistry
 * Method:    registerComponent
 * Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerComponent(JNIEnv *jniEnv,
								 jobject
								 thisComponentRegistry)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jstring _thisComponentName;

	// BODY

	assert(thisComponentRegistry != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerComponent(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisComponentRegistry,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// get Java component name
	_thisComponentName = (*jniEnv)->GetObjectField(jniEnv,
						       thisComponentRegistry,
						       FID_componentName);
	// copy Java component name object
	if (JNU_copyFromStringToSaNameT_NotNull(jniEnv,
						_thisComponentName,
						&_saComponentName) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfComponentRegister
	_saStatus = saAmfComponentRegister(_saAmfHandle,
					   &_saComponentName, NULL);

	_TRACE2("NATIVE: saAmfComponentRegister(...) has returned with %d...\n",
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

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
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
		case SA_AIS_ERR_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisExistException",
					   AIS_ERR_EXIST_MSG);
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

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerComponent(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerProxiedComponent
 * TYPE:      native method
 *  Class:     ais_amf_ComponentRegistry
 *  Method:    registerProxiedComponent
 *  Signature: (Ljava/lang/String;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerProxiedComponent(JNIEnv
									*jniEnv,
									jobject
									thisComponentRegistry,
									jstring
									proxiedComponentName)
{
	// VARIABLES
	SaNameT _saProxyComponentName;
	SaNameT _saProxiedComponentName;
	SaNameT *_saProxiedComponentNamePtr = &_saProxiedComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jstring _thisComponentName;

	// BODY

	assert(thisComponentRegistry != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerProxiedComponent(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisComponentRegistry,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// get Java component name
	_thisComponentName = (*jniEnv)->GetObjectField(jniEnv,
						       thisComponentRegistry,
						       FID_componentName);
	// copy Java component name object
	if (JNU_copyFromStringToSaNameT_NotNull(jniEnv,
						_thisComponentName,
						&_saProxyComponentName) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// copy Java proxied component name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					proxiedComponentName,
					&_saProxiedComponentNamePtr) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfComponentRegister
	_saStatus = saAmfComponentRegister(_saAmfHandle,
					   _saProxiedComponentNamePtr,
					   &_saProxyComponentName);

	_TRACE2("NATIVE: saAmfComponentRegister(...) has returned with %d...\n",
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
			// can happen if _saProxyComponentNamePtr is NULL
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
					   "ais/AisNoResourcesException",
					   AIS_ERR_NO_RESOURCES_MSG);
			break;
		case SA_AIS_ERR_NOT_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNotExistException",
					   AIS_ERR_NOT_EXIST_MSG);
			break;
		case SA_AIS_ERR_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisExistException",
					   AIS_ERR_EXIST_MSG);
			break;
		case SA_AIS_ERR_BAD_OPERATION:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadOperationException",
					   AIS_ERR_BAD_OPERATION_MSG);
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

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ComponentRegistryImpl_registerProxiedComponent(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterComponent
 * TYPE:      native method
 *  Class:     ais_amf_ComponentRegistry
 *  Method:    unregisterComponent
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterComponent(JNIEnv
								   *jniEnv,
								   jobject
								   thisComponentRegistry)
{
	// VARIABLES
	SaNameT _saComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jstring _thisComponentName;

	// BODY

	assert(thisComponentRegistry != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterComponent(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisComponentRegistry,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// get Java component name
	_thisComponentName = (*jniEnv)->GetObjectField(jniEnv,
						       thisComponentRegistry,
						       FID_componentName);
	// copy Java component name object
	if (JNU_copyFromStringToSaNameT_NotNull(jniEnv,
						_thisComponentName,
						&_saComponentName) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// call saAmfComponentUnregister
	_saStatus = saAmfComponentUnregister(_saAmfHandle,
					     &_saComponentName, NULL);

	_TRACE2
		("NATIVE: saAmfComponentUnregister(...) has returned with %d...\n",
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
		case SA_AIS_ERR_INVALID_PARAM:

			assert(JNI_FALSE);

			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisLibraryException",
					   AIS_ERR_LIBRARY_MSG);
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
		case SA_AIS_ERR_BAD_OPERATION:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadOperationException",
					   AIS_ERR_BAD_OPERATION_MSG);
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

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterComponent(...) returning normally\n");

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterProxiedComponent
 * TYPE:      native method
 *  Class:     ais_amf_ComponentRegistry
 *  Method:    unregisterProxiedComponent
 *  Signature: (Ljava/lang/String;)V
 *************************************************************************/
JNIEXPORT void JNICALL
Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterProxiedComponent(JNIEnv
									  *jniEnv,
									  jobject
									  thisComponentRegistry,
									  jstring
									  proxiedComponentName)
{
	SaNameT _saProxyComponentName;
	SaNameT _saProxiedComponentName;
	SaNameT *_saProxiedComponentNamePtr = &_saProxiedComponentName;
	SaAmfHandleT _saAmfHandle;
	SaAisErrorT _saStatus;
	// JNI
	jobject _amfLibraryHandle;
	jstring _thisComponentName;

	// BODY

	assert(thisComponentRegistry != NULL);
	_TRACE2
		("NATIVE: Executing Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterProxiedComponent(...)\n");

	// get Java library handle
	_amfLibraryHandle = (*jniEnv)->GetObjectField(jniEnv,
						      thisComponentRegistry,
						      FID_amfLibraryHandle);

	assert(_amfLibraryHandle != NULL);

	// get native library handle
	_saAmfHandle = (SaAmfHandleT)(*jniEnv)->GetLongField(jniEnv,
							     _amfLibraryHandle,
							     FID_saAmfHandle);
	// get Java component name
	_thisComponentName = (*jniEnv)->GetObjectField(jniEnv,
						       thisComponentRegistry,
						       FID_componentName);
	// copy Java component name object
	if (JNU_copyFromStringToSaNameT_NotNull(jniEnv,
						_thisComponentName,
						&_saProxyComponentName) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}
	// copy Java proxied component name object
	if (JNU_copyFromStringToSaNameT(jniEnv,
					proxiedComponentName,
					&_saProxiedComponentNamePtr) !=
	    JNI_TRUE) {
		return;		// EXIT POINT! Exception pending...
	}

	// call saAmfComponentUnregister
	_saStatus = saAmfComponentUnregister(_saAmfHandle,
					     _saProxiedComponentNamePtr,
					     &_saProxyComponentName);

	_TRACE2
		("NATIVE: saAmfComponentUnregister(...) has returned with %d...\n",
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
		case SA_AIS_ERR_NOT_EXIST:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisNotExistException",
					   AIS_ERR_NOT_EXIST_MSG);
			break;
		case SA_AIS_ERR_BAD_OPERATION:
			JNU_throwNewByName(jniEnv,
					   "org/saforum/ais/AisBadOperationException",
					   AIS_ERR_BAD_OPERATION_MSG);
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

	_TRACE2
		("NATIVE: Java_org_opensaf_ais_amf_ComponentRegistryImpl_unregisterProxiedComponent(...) returning normally\n");

}
