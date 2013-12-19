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
#include <errno.h>
#include <sys/select.h>
#include <saClm.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "j_ais_clm.h"
#include "jni_ais_clm.h" // not really needed, but good for syntax checking!

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

// CLASS ais.clm.ClmHandle
jclass ClassClmHandle = NULL;
jmethodID MID_s_invokeGetClusterNodeCallback = NULL;
jmethodID MID_s_invokeTrackClusterCallback_4 = NULL;
jmethodID MID_s_invokeTrackClusterCallback = NULL; 
jfieldID FID_saClmHandle = NULL;
jfieldID FID_saVersion = NULL;
static jfieldID FID_getClusterNodeCallback = NULL;
static jfieldID FID_trackClusterCallback = NULL;
static jfieldID FID_selectionObject = NULL;

jmethodID MID_s_getClusterChange = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

jboolean JNU_ClmHandle_initIDs_OK(
    JNIEnv* jniEnv );
static jboolean JNU_ClmHandle_initIDs_FromClass_OK(
    JNIEnv* jniEnv,
    jclass classClmHandle );
void JNU_Exception_Throw(JNIEnv* jniEnv, SaAisErrorT _saStatus);
/**************************************************************************
 * Function definitions
 *************************************************************************/
void JNU_Exception_Throw(
    JNIEnv* jniEnv, 
    SaAisErrorT _saStatus) 
{
        switch( _saStatus ){

            case SA_AIS_ERR_VERSION:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisVersionException",
                                    AIS_ERR_VERSION_MSG );			
                                    break;
            case SA_AIS_ERR_LIBRARY:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                                    break;
            case SA_AIS_ERR_TIMEOUT:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisTimeoutException",
                                    AIS_ERR_TIMEOUT_MSG );
                                    break;
            case SA_AIS_ERR_TRY_AGAIN:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisTryAgainException",
                                    AIS_ERR_TRY_AGAIN_MSG );
                                    break;
            case SA_AIS_ERR_BAD_HANDLE:

                /* TODO library handle invalid (e.g finalized): this check could be done at Java level! */
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
                                    break;
            case SA_AIS_ERR_INVALID_PARAM:

                /* this should not happen here! */
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInvalidParamException",
                                    AIS_ERR_INVALID_PARAM_MSG );
                                    break;
            case SA_AIS_ERR_INIT:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInitException",
                                    AIS_ERR_INIT_MSG );
                                    break;
            case SA_AIS_ERR_NO_SPACE:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoSpaceException",
                                    AIS_ERR_NO_SPACE_MSG );
                                    break;
            case SA_AIS_ERR_NOT_EXIST:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNotExistException",
                                    AIS_ERR_NOT_EXIST_MSG );
                                    break;
            case SA_AIS_ERR_NO_MEMORY:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoMemoryException",
                                    AIS_ERR_NO_MEMORY_MSG );
                                    break;
            case SA_AIS_ERR_NO_RESOURCES:
		JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoResourcesException",
                                    AIS_ERR_NO_RESOURCES_MSG );
                                    break;
            case SA_AIS_ERR_UNAVAILABLE:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisUnavailableException",
                                    AIS_ERR_UNAVAILABLE_MSG );
                                    break;
            default:
                // this should not happen here!
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                                    break;
        }

}


//********************************
// CLASS ais.clm.ClmHandle
//********************************

/**************************************************************************
 * FUNCTION:      JNU_ClmHandle_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClmHandle_initIDs_OK(
    JNIEnv* jniEnv )
{

    // BODY

    _TRACE2( "NATIVE: Executing JNU_ClmHandle_initIDs_OK(...)\n" );

    // get ClmHandle class & create a global reference right away
    /*
    ClassClmHandle =
        (*jniEnv)->NewGlobalRef( jniEnv,
                                 (*jniEnv)->FindClass( jniEnv,
                                                       "org/opensaf/ais/clm/ClmHandleImpl" )
                               );*/
    ClassClmHandle = JNU_GetGlobalClassRef( jniEnv,
                                                   "org/opensaf/ais/clm/ClmHandleImpl" );
    if( ClassClmHandle == NULL ){

        _TRACE2( "NATIVE ERROR: ClassClmHandle is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }

    // get IDs
    return JNU_ClmHandle_initIDs_FromClass_OK( jniEnv, ClassClmHandle );

}

/**************************************************************************
 * FUNCTION:      JNU_ClmHandle_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClmHandle_initIDs_FromClass_OK(
    JNIEnv* jniEnv,
    jclass classClmHandle )
{

    // BODY

    _TRACE2( "NATIVE: Executing JNU_ClmHandle_initIDs_FromClass_OK(...)\n" );


    // get method IDs

    MID_s_getClusterChange = (*jniEnv)->GetStaticMethodID( jniEnv,
                                                            classClmHandle,
                                                            "s_getClusterChange",
                                                            "(I)Lorg/saforum/ais/clm/ClusterNotification$ClusterChange;" );
    if( MID_s_getClusterChange == NULL ){

        _TRACE2( "NATIVE ERROR: MID_s_getClusterChange is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }

    MID_s_invokeGetClusterNodeCallback = (*jniEnv)->GetStaticMethodID( jniEnv,
                                                            classClmHandle,
                                                            "s_invokeGetClusterNodeCallback",
                                                            "(JLorg/saforum/ais/clm/ClusterNode;I)V" );
    if( MID_s_invokeGetClusterNodeCallback == NULL ){

        _TRACE2( "NATIVE ERROR: MID_s_invokeGetClusterNodeCallback is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }
    MID_s_invokeTrackClusterCallback_4 = (*jniEnv)->GetStaticMethodID( jniEnv,
                                                            classClmHandle,
                                                            "s_invokeTrackClusterCallback",
                                                            "(Lorg/saforum/ais/clm/ClusterNotificationBuffer;IJLjava/lang/String;Lorg/saforum/ais/CorrelationIds;IJI)V" );

    if( MID_s_invokeTrackClusterCallback_4 == NULL ){

        _TRACE2( "NATIVE ERROR: MID_s_invokeTrackClusterCallback is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }

    MID_s_invokeTrackClusterCallback = (*jniEnv)->GetStaticMethodID( jniEnv,
                                                                     classClmHandle,   					
                                                                     "s_invokeTrackClusterCallback",
                                                                     "(Lorg/saforum/ais/clm/ClusterNotificationBuffer;II)V" );
    // get field IDs	
    FID_getClusterNodeCallback = (*jniEnv)->GetFieldID(
                                            jniEnv,
                                            classClmHandle,
                                            "getClusterNodeCallback",
                                            "Lorg/saforum/ais/clm/GetClusterNodeCallback;" );
    if( FID_getClusterNodeCallback == NULL ){

        _TRACE2( "NATIVE ERROR: FID_getClusterNodeCallback is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }
    FID_trackClusterCallback = (*jniEnv)->GetFieldID(
                                            jniEnv,
                                            classClmHandle,
                                            "trackClusterCallback",
                                            "Lorg/saforum/ais/clm/TrackClusterCallback;" );
    if( FID_trackClusterCallback == NULL ){

        _TRACE2( "NATIVE ERROR: FID_trackClusterCallback is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }
    FID_saClmHandle = (*jniEnv)->GetFieldID( jniEnv,
                                           classClmHandle,
                                           "saClmHandle",
                                           "J" );
    if( FID_saClmHandle == NULL ){

        _TRACE2( "NATIVE ERROR: FID_saClmHandle is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }

    FID_saVersion = (*jniEnv)->GetFieldID( jniEnv,
                                           classClmHandle,
                                           "version",
                                           "Lorg/saforum/ais/Version;" );  	 	 	

    if( FID_saVersion == NULL ){
	
        _TRACE2( "NATIVE ERROR: FID_saVersion is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...

    }	

    FID_selectionObject = (*jniEnv)->GetFieldID( jniEnv,
                                           classClmHandle,
                                           "selectionObject",
                                           "J" );
    if( FID_selectionObject == NULL ){

        _TRACE2( "NATIVE ERROR: FID_selectionObject is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }


    _TRACE2( "NATIVE: JNU_ClmHandle_initIDs_FromClass_OK(...) returning normally\n" );

    return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmInitialize
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    invokeSaClmInitialize
 *  Signature: (Lorg/saforum/ais/Version;)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmInitialize(
    JNIEnv* jniEnv,
    jobject thisClmHandle,
    jobject sVersion )
{
    // VARIABLES
    // ais
    SaClmHandleT _saClmHandle;
    SaClmCallbacksT_4 _saClmCallbacks_4;
    SaClmCallbacksT _saClmCallbacks;
    SaVersionT _saVersion;
    SaAisErrorT _saStatus;
    // jni
    jobject _getClusterNodeCallback;
    jobject _trackClusterCallback;
    jobject _version;
    jchar _releaseCode;
    jshort _majorVersion;
    jshort _minorVersion;

    // BODY

      assert( thisClmHandle != NULL );
    
    // TODO assert for sVersion
      _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmInitialize(...)\n" );

	// create callback struct
       //  get cluster node cb
    _getClusterNodeCallback = (*jniEnv)->GetObjectField(
                                            jniEnv,
                                            thisClmHandle,
                                            FID_getClusterNodeCallback );
    
    _trackClusterCallback = (*jniEnv)->GetObjectField(
                                            jniEnv,
                                            thisClmHandle,
                                            FID_trackClusterCallback );
    
    if( sVersion == NULL ){
        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisInvalidParamException",
                            AIS_ERR_INVALID_PARAM_MSG );
        return; // EXIT POINT!
    }
    // release code
    _releaseCode = (*jniEnv)->GetCharField( jniEnv,
                                            sVersion,
                                            FID_releaseCode );
    _saVersion.releaseCode = (SaUint8T) _releaseCode;
    // major version
    _majorVersion = (*jniEnv)->GetShortField( jniEnv,
                                              sVersion,
                                              FID_majorVersion );
    _saVersion.majorVersion = (SaUint8T) _majorVersion;
    // minor version
    _minorVersion = (*jniEnv)->GetShortField( jniEnv,
                                             sVersion,
                                             FID_minorVersion );
    _saVersion.minorVersion = (SaUint8T) _minorVersion;

         if( _majorVersion == 1 && _minorVersion == 1)
         {
             if( _getClusterNodeCallback != NULL )
             {
                   _TRACE2( "NATIVE: SaClmClusterNodeGetCallback assigned as callback!\n" );

                   _saClmCallbacks.saClmClusterNodeGetCallback = SaClmClusterNodeGetCallback;
             }
             else
             {
                   _TRACE2( "NATIVE: NO SaClmClusterNodeGetCallback assigned!\n" );

                   _saClmCallbacks.saClmClusterNodeGetCallback = NULL;
             }

             if( _trackClusterCallback != NULL )
             {
                   _TRACE2( "NATIVE: SaClmClusterTrackCallback assigned as callback!\n" );

                   _saClmCallbacks.saClmClusterTrackCallback = SaClmClusterTrackCallback;
             }
             else 
             {
                   _TRACE2( "NATIVE: NO SaClmClusterTrackCallback assigned!\n" );

                   _saClmCallbacks.saClmClusterTrackCallback = NULL;
             }
	 	// call saClmInitialize
               	   _saStatus = saClmInitialize( &_saClmHandle,
                                                &_saClmCallbacks,
                                                &_saVersion );

		   _TRACE2( "NATIVE: saClmInitialize(...) has returned with %d...\n", _saStatus );
         }	
        else
        {
          if( _getClusterNodeCallback != NULL )
            {

                  _TRACE2( "NATIVE: SaClmClusterNodeGetCallback assigned as callback!\n" );

                  _saClmCallbacks_4.saClmClusterNodeGetCallback = SaClmClusterNodeGetCallback_4;

            }
            else
            {
                   _TRACE2( "NATIVE: NO SaClmClusterNodeGetCallback assigned!\n" );

                   _saClmCallbacks_4.saClmClusterNodeGetCallback = NULL;

            }
				
            if( _trackClusterCallback != NULL )
            {

                   _TRACE2( "NATIVE: SaClmClusterTrackCallback assigned as callback!\n" );

                   _saClmCallbacks_4.saClmClusterTrackCallback = SaClmClusterTrackCallback_4;

            }
            else
            {
                   _TRACE2( "NATIVE: NO SaClmClusterTrackCallback assigned!\n" );

                   _saClmCallbacks_4.saClmClusterTrackCallback = NULL;

            }
                // call saClmInitialize   
                   _saStatus = saClmInitialize_4( &_saClmHandle,
                                                  &_saClmCallbacks_4,
                                                  &_saVersion );

                _TRACE2( "NATIVE: saClmInitialize_4(...) has returned with %d...\n", _saStatus ); 
       }	

    /* set version param:
            see page 19 of the Cluster Membership Service spec B.01.01! */
         //releaseCode
     if( _saVersion.releaseCode != _releaseCode ) {
                    (*jniEnv)->SetCharField( jniEnv,
                                             sVersion,
                                             FID_releaseCode,
                                             (jchar) _saVersion.releaseCode );
     }
         //majorVersion 		
     if( _saVersion.majorVersion != _majorVersion ) {
                    (*jniEnv)->SetShortField( jniEnv,
                                              sVersion,
                                              FID_majorVersion,
                                              (jshort) _saVersion.majorVersion );
     }
         // minor version
     if( _saVersion.minorVersion != _minorVersion ) {
                    (*jniEnv)->SetShortField( jniEnv,
                                              sVersion,
                                              FID_minorVersion,
                                              (jshort) _saVersion.minorVersion );
     }
			
	// error handling
     if( _saStatus != SA_AIS_OK ){
            JNU_Exception_Throw( jniEnv, _saStatus );
            return; /* EXIT POINT! */
     }
    // set library handle

    _TRACE2( "NATIVE: Retreived clmHandle is: %lu \n", (unsigned long) _saClmHandle );	

    (*jniEnv)->SetLongField( jniEnv,
                             thisClmHandle,
                             FID_saClmHandle,
                             (jlong) _saClmHandle );
    
     
    // set Version        
    _version = (*jniEnv)->NewObject( jniEnv,
                                     ClassVersion,
                                     CID_Version_constructor,
                                     (char) _saVersion.releaseCode, 
                                     (jshort) _saVersion.majorVersion,							
                                     (jshort) _saVersion.minorVersion );

    (*jniEnv)->SetObjectField( jniEnv,
                               thisClmHandle,				 	
                               FID_saVersion,
                               _version );																						  
    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmInitialize(...) returning normally\n" );

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmSelectionObjectGet
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    invokeSaClmSelectionObjectGet
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmSelectionObjectGet(
	JNIEnv* jniEnv,
	jobject thisClmHandle )
{
    // VARIABLES
    // ais
    SaClmHandleT _saClmHandle;
    SaAisErrorT _saStatus;
    SaSelectionObjectT _saSelectionObject;
    // jni

    // BODY

    assert( thisClmHandle != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmSelectionObjectGet(...)\n" );

    // get library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           thisClmHandle,
                                                           FID_saClmHandle );
    _TRACE2( "NATIVE: SaClmHandleT %lu", (unsigned long)_saClmHandle ); 
    // call saClmSelectionObjectGet
    _saStatus = saClmSelectionObjectGet( _saClmHandle,
                                         &_saSelectionObject );

    _TRACE2( "NATIVE: saClmSelectionObjectGet(...) has returned with %d...\n", _saStatus );


    // error handling
    if( _saStatus != SA_AIS_OK ){
        JNU_Exception_Throw( jniEnv, _saStatus );
        return; /* EXIT POINT! */
    }
        
    
    // set selection object

    _TRACE2( "NATIVE: Retreived selectionObject is: %lu \n", (unsigned long) _saSelectionObject );
    U_printSaSelectionObjectInfo( "NATIVE: more info:", _saSelectionObject );

    (*jniEnv)->SetLongField( jniEnv,
                             thisClmHandle,
                             FID_selectionObject,
                             (jlong) _saSelectionObject );
    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmSelectionObjectGet(...) returning normally\n" );

}

/**************************************************************************
 *  FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmResponse
 *  TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    invokeSaClmResponse
 *  Signature: (JLorg/saforum/ais/CallbackResponse;)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmResponse(
    JNIEnv* jniEnv,
    jobject thisClmHandle,
    jlong invocation,
    jobject callbackResponse )
{

    SaClmHandleT _saClmHandle;
    SaClmResponseT _response;
    SaAisErrorT _saStatus;
    int value;

    assert( thisClmHandle != NULL );

    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmResponse(...)\n" );

    _saClmHandle = (SaClmHandleT)(*jniEnv)->GetLongField( jniEnv,
                                                          thisClmHandle,
                                                          FID_saClmHandle );

    value = (*jniEnv)->GetIntField( jniEnv,
                                    callbackResponse, 
                                    FID_CR_value );

    _TRACE2("NATIVE: CallbackResponse value: %d\n", value);

    if( value == 1 )
        _response = SA_CLM_CALLBACK_RESPONSE_OK;
    else if( value == 2)
        _response = SA_CLM_CALLBACK_RESPONSE_REJECTED;
    else if( value == 3)
        _response = SA_CLM_CALLBACK_RESPONSE_ERROR;
    else
        _response = SA_CLM_CALLBACK_RESPONSE_ERROR;

    /* call saClmResponse_4 */

      _saStatus = saClmResponse_4( _saClmHandle, 
                                  (SaInvocationT)invocation, 
                                  _response );

    _TRACE2(" NATIVE: saClmResponse_4 has returned with %d\n", _saStatus);

    if( _saStatus != SA_AIS_OK ){
        JNU_Exception_Throw( jniEnv, _saStatus );
        return;
    }

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmResponse(...) returning normally\n" );
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_checkSelectionObject
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    checkSelectionObject
 *  Signature: (J)Z
 *************************************************************************/
/*
 * MODIFICATION: removed (to be tested)
JNIEXPORT jboolean JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_checkSelectionObject(
	JNIEnv* jniEnv,
	jobject thisClmHandle,
    jlong timeout )
{

    // VARIABLES
    // ais
    SaSelectionObjectT _saSelectionObject;
    // linux
    fd_set _readFDs;
    struct timeval _lxTimeout;
    int _selectStatus;

    // BODY

    assert( thisClmHandle != NULL );
    // TODO assert for timeout
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_checkSelectionObject(...)\n" );

    // get selection object
    _saSelectionObject = (SaSelectionObjectT) (*jniEnv)->GetLongField(
                                                            jniEnv,
                                                            thisClmHandle,
                                                            FID_selectionObject );
    // call select(2)
    FD_ZERO( &_readFDs );
    FD_SET( _saSelectionObject, &_readFDs );
    U_convertTimeout( &_lxTimeout, timeout );

    _TRACE2( "NATIVE: timout is { %ld seconds, %ld microseconds }\n", _lxTimeout.tv_sec, _lxTimeout.tv_usec );

    _selectStatus = select( (_saSelectionObject + 1 ),
                            &_readFDs,
                            NULL,
                            NULL,
                            &_lxTimeout );

    _TRACE2( "NATIVE: select(...) has returned with %d...\n", _selectStatus );


    // error handling
    if( _selectStatus == -1 ){
        switch( errno ){
            case EBADF: // invalid file descriptor

                // this should not happen here!
                _TRACE2( "NATIVE ERROR : select(EBADF): invalid file descriptor\n" );
                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case EINTR: // non blocked signal caught

                _TRACE2( "NATIVE ERROR : select(EINTR): non blocked signal caught\n" );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case EINVAL: // n is negative or the value within timeout is invalid

                // this should not happen here!
                _TRACE2( "NATIVE ERROR : select(EINVAL): n is negative or the value within timeout is invalid\n" );
                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case ENOMEM: // select was unable to allocate memory for internal tables

                _TRACE2( "NATIVE ERROR : select(ENOMEM): select was unable to allocate memory for internal tables\n" );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoMemoryException",
                                    AIS_ERR_NO_MEMORY_MSG );
                break;
            default:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
        }
        return JNI_FALSE; // EXIT POINT!!! return value is in fact ignored
    }
    if( _selectStatus == 1 ){

        _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_checkSelectionObject() returning true\n" );

        return JNI_TRUE;
    }
    else{

        _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_checkSelectionObject() returning false\n" );

        return JNI_FALSE;
    }
}
*/


/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatch
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    invokeSaClmDispatch
 *  Signature: (I)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatch(
	JNIEnv* jniEnv,
	jobject thisClmHandle,
	jint dispatchFlags )
{
	// VARIABLES
	SaClmHandleT _saClmHandle;
	SaAisErrorT _saStatus;

	// BODY

    assert( thisClmHandle != NULL );
    assert( ( dispatchFlags == SA_DISPATCH_ONE ) || ( dispatchFlags == SA_DISPATCH_ALL ) || ( dispatchFlags == SA_DISPATCH_BLOCKING ) );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatch(...)\n" );

    // get library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           thisClmHandle,
                                                           FID_saClmHandle );
    _TRACE2(" NATIVE: saClmHandle %lu ", (unsigned long)_saClmHandle );

	// call saClmDispatch
	_saStatus = saClmDispatch( _saClmHandle,
							   (SaDispatchFlagsT) dispatchFlags );

    _TRACE2( "NATIVE: saClmDispatch(...) has returned with %d...\n", _saStatus );


	// error handling
  	if( _saStatus != SA_AIS_OK ){
            JNU_Exception_Throw( jniEnv, _saStatus );
            return;
	}
    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatch() returning normally\n" );


}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatchWhenReady
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    invokeSaClmDispatchWhenReady
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatchWhenReady(
    JNIEnv* jniEnv,
    jobject thisClmHandle )
{
    // VARIABLES
    // ais
    SaClmHandleT _saClmHandle;
    SaAisErrorT _saStatus;
    SaSelectionObjectT _saSelectionObject;
    // linux
    fd_set _readFDs;
    int _selectStatus;

    // BODY

    assert( thisClmHandle != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatchWhenReady(...)\n" );

    // get library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           thisClmHandle,
                                                           FID_saClmHandle );
    _TRACE2( "NATIVE: saClmHandle %d\n", _saClmHandle );
    // get selection object
    _saSelectionObject = (SaSelectionObjectT) (*jniEnv)->GetLongField(
                                                            jniEnv,
                                                            thisClmHandle,
                                                            FID_selectionObject );
    
    _TRACE2(" NATIVE: _saSelectionObject %d\n", _saSelectionObject );

    // wait until there is a pending callback
    // call select(2)
    FD_ZERO( &_readFDs );
    FD_SET( _saSelectionObject, &_readFDs );
    _selectStatus = select( ( _saSelectionObject + 1 ),
                            &_readFDs,
                            NULL,
                            NULL,
                            NULL );

    _TRACE2( "NATIVE: select(...) has returned with %d...\n", _selectStatus );

    // error handling
    if( _selectStatus == -1 ){
        switch( errno ){
            case EBADF: // invalid file descriptor

                // this should not happen here!
                _TRACE2( "NATIVE ERROR : select(EBADF): invalid file descriptor\n" );
                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case EINTR: // non blocked signal caught

                _TRACE2( "NATIVE ERROR : select(EINTR): non blocked signal caught\n" );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case EINVAL: // n is negative or the value within timeout is invalid

                // this should not happen here!
                _TRACE2( "NATIVE ERROR : select(EINVAL): n is negative or the value within timeout is invalid\n" );
                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
            case ENOMEM: // select was unable to allocate memory for internal tables

                _TRACE2( "NATIVE ERROR : select(ENOMEM): select was unable to allocate memory for internal tables\n" );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoMemoryException",
                                    AIS_ERR_NO_MEMORY_MSG );
                break;
            default:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
        }
        return; // EXIT POINT!!!
    }
    if( _selectStatus != 1 ){

                _TRACE2( "NATIVE ERROR : unexpected select return value!\n" );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
        return; // EXIT POINT!!!
    }

    // call saClmDispatch
    _saStatus = saClmDispatch( _saClmHandle,
                               SA_DISPATCH_ONE );

    _TRACE2( "NATIVE: saClmDispatch(...) has returned with %d...\n", _saStatus );


    // error handling
    if( _saStatus != SA_AIS_OK ){
        JNU_Exception_Throw( jniEnv, _saStatus );
        return;
    }
    
    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatchWhenReady() returning normally\n" );


}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClmHandleImpl_finalizeClmHandle
 * TYPE:      native method
 *  Class:     ais_clm_ClmHandle
 *  Method:    finalizeClmHandle
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_finalizeClmHandle(
    JNIEnv * jniEnv,
    jobject thisClmHandle )
{
	// VARIABLES
	SaClmHandleT _saClmHandle;
	SaAisErrorT _saStatus;

	// BODY

    assert( thisClmHandle != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClmHandleImpl_finalizeClmHandle()\n" );


    // get library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           thisClmHandle,
                                                           FID_saClmHandle );

	// call saClmFinalize
	_saStatus = saClmFinalize( _saClmHandle );

    _TRACE2( "NATIVE: saClmFinalize(...) has returned with %d...\n", _saStatus );


    // error handling
  	if( _saStatus != SA_AIS_OK ){
            JNU_Exception_Throw( jniEnv, _saStatus );
            return;
        }

    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClmHandleImpl_finalizeClmHandle() returning normally\n" );
}
