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

#include <string.h>
#include <stdlib.h>
#include <saClm.h>
#include <jni.h>
#include "j_utils.h"
#include "j_ais.h"
#include "j_ais_clm.h"
#include "j_ais_clm_libHandle.h"
#include "jni_ais_clm.h" // not really needed, but good for syntax checking!

/**************************************************************************
 * Constants
 *************************************************************************/

//#define DEFAULT_NUMBER_OF_ITEMS 1
#define DEFAULT_NUMBER_OF_ITEMS 2

/**************************************************************************
 * Macros
 *************************************************************************/

 // #define IMPL_CLIENT_ALLOC 0 //

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable declarations
 *************************************************************************/


/**************************************************************************
 * Variable definitions
 *************************************************************************/

// CLASS ais.clm.ClusterMembershipManager
static jclass ClassClusterMembershipManager = NULL;
static jfieldID FID_clmLibraryHandle = NULL;

/**************************************************************************
 * Function declarations
 *************************************************************************/

// CLASS ais.clm.ClusterMembershipManager
jboolean JNU_ClusterMembershipManager_initIDs_OK(
    JNIEnv* jniEnv );
static jboolean JNU_ClusterMembershipManager_initIDs_FromClass_OK(
    JNIEnv* jniEnv,
    jclass classClmHandle );
static void JNU_invokeSaClmClusterTrack_Async(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags );
static jobject JNU_invokeSaClmClusterTrack_Sync(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags );
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags,
    jobject sNotificationBuffer );
#ifndef IMPL_CLIENT_ALLOC
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr );
#else // IMPL_CLIENT_ALLOC
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr,
    SaClmClusterNotificationT* saNotifications,
    jboolean* bufferOnStack );
#endif // IMPL_CLIENT_ALLOC
static jboolean JNU_callSaClmClusterTrack_Sync(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr,
    SaAisErrorT* saStatusPtr );

/**************************************************************************
 * Function definitions
 *************************************************************************/

//****************************************
// CLASS ais.clm.ClusterMembershipManager
//****************************************

/**************************************************************************
 * FUNCTION:      JNU_ClusterMembershipManager_initIDs_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
jboolean JNU_ClusterMembershipManager_initIDs_OK(
    JNIEnv* jniEnv )
{

    // BODY

    _TRACE2( "NATIVE: Executing JNU_ClusterMembershipManager_initIDs_OK(...)\n" );

    // get ClusterMembershipManager class & create a global reference right away
    /*
    ClassClusterMembershipManager =
        (*jniEnv)->NewGlobalRef( jniEnv,
                                 (*jniEnv)->FindClass( jniEnv,
                                                       "org/opensaf/ais/clm/ClusterMembershipManagerImpl" )
                               );*/
    ClassClusterMembershipManager = JNU_GetGlobalClassRef(  jniEnv,
                                                   			"org/opensaf/ais/clm/ClusterMembershipManagerImpl" );
    if( ClassClusterMembershipManager == NULL ){

        _TRACE2( "NATIVE ERROR: ClassClusterMembershipManager is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }

    // get IDs
    return JNU_ClusterMembershipManager_initIDs_FromClass_OK( jniEnv, ClassClusterMembershipManager );

}

/**************************************************************************
 * FUNCTION:      JNU_ClusterMembershipManager_initIDs_FromClass_OK
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_ClusterMembershipManager_initIDs_FromClass_OK(
    JNIEnv* jniEnv,
    jclass classClusterMembershipManager )
{

    // BODY

    _TRACE2( "NATIVE: Executing JNU_ClusterMembershipManager_initIDs_FromClass_OK(...)\n" );


    // get field IDs
    FID_clmLibraryHandle = (*jniEnv)->GetFieldID(
                                            jniEnv,
                                            classClusterMembershipManager,
                                            "clmLibraryHandle",
                                            "Lorg/saforum/ais/clm/ClmHandle;" );
    if( FID_clmLibraryHandle == NULL ){

        _TRACE2( "NATIVE ERROR: FID_clmLibraryHandle is NULL\n" );

        return JNI_FALSE; // EXIT POINT! Exception pending...
    }


    _TRACE2( "NATIVE: JNU_ClusterMembershipManager_initIDs_FromClass_OK(...) returning normally\n" );

        return JNI_TRUE;
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getCluster
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    getCluster
 *  Signature: ()Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 *************************************************************************/
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getCluster(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager )
{
    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getCluster(...)\n" );


    return JNU_invokeSaClmClusterTrack_Sync(    jniEnv,
                                                thisClusterMembershipManager,
                                                0 );
}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsync
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    getClusterAsync
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsync(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager )
{

    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsync(...)\n" );


    JNU_invokeSaClmClusterTrack_Async( jniEnv,
                                       thisClusterMembershipManager,
                                       SA_TRACK_CURRENT );

}

/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsyncThenStartTracking
 * TYPE:      native method
 *  Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 *  Method:    getClusterAsyncThenStartTracking
 *  Signature: (Lorg/saforum/ais/TrackFlags;)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsyncThenStartTracking(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    jobject trackFlags)
{
    // VARIABLES
    SaUint8T _saTrackFlags;

    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsyncThenStartTracking(...)\n" );


    // get track flag
    if( trackFlags == NULL ){
        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisInvalidParamException",
                            AIS_ERR_INVALID_PARAM_MSG );
        return; // EXIT POINT!
    }
    _saTrackFlags = (SaUint8T)
                    (*jniEnv)->GetIntField( jniEnv,
                                            trackFlags,
                                            FID_TF_value );
    _saTrackFlags |= SA_TRACK_CURRENT;
    // check track flags
    /*
    if( JNU_TrackFlagsForChanges_OK( jniEnv, trackFlags ) != JNI_TRUE ) {
        return; // EXIT POINT! Exception pending...
    }
    */
    // invoke saClmClusterTrack()
    JNU_invokeSaClmClusterTrack_Async( jniEnv,
                                       thisClusterMembershipManager,
                                       _saTrackFlags );
}


/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_startClusterTracking
 * TYPE:      native method
 *  Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 *  Method:    startClusterTracking
 *  Signature: (Lorg/saforum/ais/TrackFlags;)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_startClusterTracking(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    jobject trackFlags)
{
    // VARIABLES
    SaUint8T _saTrackFlags;

    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_startClusterTracking(...)\n" );


    // get track flag
    if( trackFlags == NULL ){
        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisInvalidParamException",
                            AIS_ERR_INVALID_PARAM_MSG );
        return; // EXIT POINT!
    }
    _saTrackFlags = (SaUint8T)
                    (*jniEnv)->GetIntField( jniEnv,
                                            trackFlags,
                                            FID_TF_value );
    _saTrackFlags |= SA_TRACK_CURRENT;
    // check track flags
    /*
    if( JNU_TrackFlagsForChanges_OK( jniEnv, trackFlags ) != JNI_TRUE ) {
        return; // EXIT POINT! Exception pending...
    }
    */
    // invoke saClmClusterTrack()
    JNU_invokeSaClmClusterTrack_Async( jniEnv,
                                       thisClusterMembershipManager,
                                       _saTrackFlags );
}


/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterThenStartTracking
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    getClusterThenStartTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;)Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 *************************************************************************/
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterThenStartTracking(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    jobject trackFlags )
{
    // VARIABLES
    SaUint8T _saTrackFlags;

    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterThenStartTracking(...)\n" );


    // get track flag
    if( trackFlags == NULL ){
        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisInvalidParamException",
                            AIS_ERR_INVALID_PARAM_MSG );
        return NULL; // EXIT POINT!
    }
    _saTrackFlags = (SaUint8T)
                    (*jniEnv)->GetIntField( jniEnv,
                                            trackFlags,
                                            FID_TF_value );
    // check track flags
    /*
    if( JNU_TrackFlagsForChanges_OK( jniEnv, trackFlags ) != JNI_TRUE ) {
        return NULL; // EXIT POINT! Exception pending...
    }
    */
    return JNU_invokeSaClmClusterTrack_Sync(jniEnv, thisClusterMembershipManager,
			_saTrackFlags);
}



/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_stopClusterTracking
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    stopClusterTracking
 *  Signature: ()V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_stopClusterTracking(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager )
{
    // VARIABLES
    SaClmHandleT _saClmHandle;
    SaAisErrorT _saStatus;
    // JNI
    jobject _clmLibraryHandle;

    // BODY

    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_stopClusterTracking(...)\n" );


    // get Java library handle
    _clmLibraryHandle = (*jniEnv)->GetObjectField( jniEnv,
                                                   thisClusterMembershipManager,
                                                   FID_clmLibraryHandle );
    // get native library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           _clmLibraryHandle,
                                                           FID_saClmHandle );
    // call saClmClusterTrack
    _saStatus = saClmClusterTrackStop( _saClmHandle );

    _TRACE2( "NATIVE: saClmClusterTrackStop(...) has returned with %d...\n", _saStatus );

    // error handling
    if( _saStatus != SA_AIS_OK ){
        switch( _saStatus ){
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
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
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
            case SA_AIS_ERR_NOT_EXIST:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNotExistException",
                                    AIS_ERR_NOT_EXIST_MSG );
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


    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_stopClusterTracking(...) returning normally\n" );

}



/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNode
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    getClusterNode
 *  Signature: (IJ)Lorg/saforum/ais/clm/ClusterNode;
 *************************************************************************/
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNode(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    jint nodeId,
    jlong timeout )
{

    // VARIABLES
    SaClmHandleT _saClmHandle;
    SaClmClusterNodeT _saClusterNode;
    SaAisErrorT _saStatus;
    // JNI
    jobject _clmLibraryHandle;

    // BODY

    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNode(...)\n" );


    // get Java library handle
    _clmLibraryHandle = (*jniEnv)->GetObjectField( jniEnv,
                                                   thisClusterMembershipManager,
                                                   FID_clmLibraryHandle );
    // get native library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                                  _clmLibraryHandle,
                                                                  FID_saClmHandle );

    U_printSaClusterNode( "NATIVE: Native Cluster Node info BEFORE calling saClmClusterNodeGet: \n", &_saClusterNode );

    // call saClmClusterNodeGet
    _saStatus = saClmClusterNodeGet( _saClmHandle,
                                     (SaClmNodeIdT) nodeId,
                                     (SaTimeT) timeout,
                                     &_saClusterNode );

    _TRACE2( "NATIVE: saClmClusterNodeGet(...) has returned with %d...\n", _saStatus );


    U_printSaClusterNode( "NATIVE: Native Cluster Node info AFTER calling saClmClusterNodeGet: \n", &_saClusterNode );


    // error handling
    if( _saStatus != SA_AIS_OK ){
        switch( _saStatus ){
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
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
                break;
            case SA_AIS_ERR_INVALID_PARAM:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInvalidParamException",
                                    AIS_ERR_INVALID_PARAM_MSG );
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
            case SA_AIS_ERR_NOT_EXIST:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNotExistException",
                                    AIS_ERR_NOT_EXIST_MSG );
                break;
            default:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
        }
        return NULL; // EXIT POINT!!!
    }


    // normal exit, at least so it seems...
    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNode(...) returning normally\n" );

    // create cluster node
    return JNU_ClusterNode_create( jniEnv, &_saClusterNode );
}


/**************************************************************************
 * FUNCTION:  Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNodeAsync
 * TYPE:      native method
 *  Class:     ais_clm_ClusterMembershipManager
 *  Method:    getClusterNodeAsync
 *  Signature: (JI)V
 *************************************************************************/
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNodeAsync(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    jlong saInvocation,
    jint nodeId )
{

    // VARIABLES
    //
    SaClmHandleT _saClmHandle;
    SaAisErrorT _saStatus;
    // JNI
    jobject _clmLibraryHandle;

    // BODY

    _TRACE2( "NATIVE: Executing Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNodeAsync(...)\n" );


    // get Java library handle
    _clmLibraryHandle = (*jniEnv)->GetObjectField( jniEnv,
                                                   thisClusterMembershipManager,
                                                   FID_clmLibraryHandle );
    // get native library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                                  _clmLibraryHandle,
                                                                  FID_saClmHandle );
    // call saClmClusterNodeGetAsync
    _saStatus = saClmClusterNodeGetAsync( _saClmHandle,
                                         (SaInvocationT) saInvocation,
                                         (SaClmNodeIdT) nodeId );

    _TRACE2( "NATIVE: saClmClusterNodeGetAsync(...) has returned with %d...\n", _saStatus );


    // error handling
    if( _saStatus != SA_AIS_OK ){
        switch( _saStatus ){
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
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
                break;
            case SA_AIS_ERR_INVALID_PARAM:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInvalidParamException",
                                    AIS_ERR_INVALID_PARAM_MSG );
                break;
            case SA_AIS_ERR_INIT:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInitException",
                                    AIS_ERR_INIT_MSG );
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
    // normal exit

    _TRACE2( "NATIVE: Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNodeAsync(...) returning normally\n" );

}


/**************************************************************************
 * FUNCTION:      JNU_invokeSaClmClusterTrack_Async
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     none
 * NOTE:
 *************************************************************************/
static void JNU_invokeSaClmClusterTrack_Async(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags )
{
    // VARIABLES
    SaClmHandleT _saClmHandle;
    SaAisErrorT _saStatus;
    // JNI
    jobject _clmLibraryHandle;

    // BODY

    assert( thisClusterMembershipManager != NULL );
    assert( ( saTrackFlags & ( ~ ( SA_TRACK_CURRENT | SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) ) ) == 0 );
    _TRACE2( "NATIVE: Executing JNU_invokeSaClmClusterTrack_Async(...)\n" );


    // get Java library handle
    _clmLibraryHandle = (*jniEnv)->GetObjectField( jniEnv,
                                                   thisClusterMembershipManager,
                                                   FID_clmLibraryHandle );
    // get native library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           _clmLibraryHandle,
                                                           FID_saClmHandle );
    // call saClmClusterTrack
    _saStatus = saClmClusterTrack( _saClmHandle,
                                   saTrackFlags,
                                   NULL );

    _TRACE2( "NATIVE: saClmClusterTrack(...) has returned with %d...\n", _saStatus );

    // error handling
    if( _saStatus != SA_AIS_OK ){
        switch( _saStatus ){
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
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
                break;
            case SA_AIS_ERR_INIT:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInitException",
                                    AIS_ERR_INIT_MSG );
                break;
            case SA_AIS_ERR_INVALID_PARAM:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                //JNU_throwNewByName( jniEnv,
                //                    "org/saforum/ais/AisInvalidParamException",
                //                    AIS_ERR_INVALID_PARAM_MSG );
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
            case SA_AIS_ERR_NO_SPACE:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                //JNU_throwNewByName( jniEnv,
                //                    "org/saforum/ais/AisNoSpaceException",
                //                    AIS_ERR_NO_SPACE_MSG );
                break;
            case SA_AIS_ERR_BAD_FLAGS:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                //JNU_throwNewByName( jniEnv,
                //                    "org/saforum/ais/AisBadFlagsException",
                //                    AIS_ERR_BAD_FLAGS_MSG );
                break;
            default:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
        }
        return; // EXIT POINT! Exception pending...
    }

    // normal exit

    _TRACE2( "NATIVE: JNU_invokeSaClmClusterTrack_Async(...) returning normally\n" );

}

/**************************************************************************
 * FUNCTION:  JNU_getClusterNotificationBuffer
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     TODO
 *************************************************************************/
static jobject JNU_invokeSaClmClusterTrack_Sync(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags )
{
    // VARIABLES
    // JNI
    jobject _sClusterNotificationBuffer;

    // BODY

    assert( thisClusterMembershipManager != NULL );
    _TRACE2( "NATIVE: Executing JNU_getClusterNotificationBuffer(...)\n" );


    // create new ClusterNotificationBuffer object
    _sClusterNotificationBuffer = (*jniEnv)->NewObject( jniEnv,
                                           ClassClusterNotificationBuffer,
                                           CID_ClusterNotificationBuffer_constructor );
    if( _sClusterNotificationBuffer == NULL ){

        _TRACE2( "NATIVE ERROR: _sClusterNotificationBuffer is NULL\n" );

        return NULL; // EXIT POINT! Exception pending...
    }
    // invoke saClmClusterTrack
    if( JNU_invokeSaClmClusterTrack_SyncNtfBuf( jniEnv,
                                          thisClusterMembershipManager,
                                          saTrackFlags,
                                          _sClusterNotificationBuffer ) != JNI_TRUE ){
        return NULL; // EXIT POINT! Exception pending...
    }

    // normal exit

    _TRACE2( "NATIVE: JNU_getClusterNotificationBuffer(...) returning normally\n" );

    return _sClusterNotificationBuffer;
}

/**************************************************************************
 * FUNCTION:  JNU_invokeSaClmClusterTrack_SyncNtfBuf
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf(
    JNIEnv* jniEnv,
    jobject thisClusterMembershipManager,
    const SaUint8T saTrackFlags,
    jobject sNotificationBuffer )
{
    // VARIABLES
    SaClmHandleT _saClmHandle;
    SaClmClusterNotificationBufferT _saNotificationBuffer;
    // JNI
    jobject _clmLibraryHandle;
    jboolean _ok = JNI_TRUE;
#ifndef IMPL_CLIENT_ALLOC
    // if we use JNU_invokeSaClmClusterTrack_SyncAllocAPI, then the API reserves
    // the buffer from the heap
    jboolean _bufferOnStack = JNI_FALSE;
#else // IMPL_CLIENT_ALLOC
    // if we use JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient, then initially we try to
    // create the buffer on the stack (if the default size for the buffer is not enough,
    // we will then allocate the buffer from the heap and then we change thi initial
    // value.
    jboolean _bufferOnStack = JNI_TRUE;
    SaClmClusterNotificationT _saNotifications[DEFAULT_NUMBER_OF_ITEMS];
    // this is not really necessary, but may be useful to catch bugs...
    memset( _saNotifications,
            0,
            ( DEFAULT_NUMBER_OF_ITEMS * sizeof(SaClmClusterNotificationT) ) );
#endif // IMPL_CLIENT_ALLOC


    // BODY

    assert( thisClusterMembershipManager != NULL );
    assert( ( saTrackFlags & ( ~ ( SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) ) ) == 0 );
    assert( sNotificationBuffer != NULL );
    _TRACE2( "NATIVE: Executing JNU_invokeSaClmClusterTrack_SyncNtfBuf(...)\n" );
    _TRACE2( "NATIVE: _saNotificationBuffer: %p\n", &_saNotificationBuffer );


    // get Java library handle
    _clmLibraryHandle = (*jniEnv)->GetObjectField( jniEnv,
                                                   thisClusterMembershipManager,
                                                   FID_clmLibraryHandle );
    // get native library handle
    _saClmHandle = (SaClmHandleT) (*jniEnv)->GetLongField( jniEnv,
                                                           _clmLibraryHandle,
                                                           FID_saClmHandle );



    // call saClmClusterTrack
#ifndef IMPL_CLIENT_ALLOC
    if( JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI(
                                            jniEnv,
                                            _saClmHandle,
                                            saTrackFlags,
                                            &_saNotificationBuffer ) != JNI_TRUE ){
        // error, some exception has been thrown already!
        return JNI_FALSE; // EXIT POINT!!!
    }
#else // IMPL_CLIENT_ALLOC
    if( JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient(
                                            jniEnv,
                                            _saClmHandle,
                                            saTrackFlags,
                                            &_saNotificationBuffer,
                                            _saNotifications,
                                            &_bufferOnStack ) != JNI_TRUE ){
        // error, some exception has been thrown already!
        return JNI_FALSE; // EXIT POINT!!!
    }
#endif // IMPL_CLIENT_ALLOC

    // process results
    if( JNU_ClusterNotificationBuffer_set( jniEnv,
                                            sNotificationBuffer,
                                            &_saNotificationBuffer ) != JNI_TRUE ){
        _ok = JNI_FALSE;
    }
    // free notification buffer
    if( ( _saNotificationBuffer.notification != NULL ) &&
        ( _bufferOnStack == JNI_FALSE ) ){
        free( _saNotificationBuffer.notification );
    }
    //
    return _ok;
}


#ifndef IMPL_CLIENT_ALLOC

/**************************************************************************
 * FUNCTION:  JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr )
{
    // VARIABLES
    SaAisErrorT _saStatus;

    // BODY

    // assert( saClmHandle != (SaClmHandleT) NULL );
    assert( ( saTrackFlags & ( ~ ( SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) ) ) == 0 );
    assert( saNotificationBufferPtr != NULL );
    _TRACE2( "NATIVE: Executing JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI(...)\n" );


    // init notification buffer
    saNotificationBufferPtr->viewNumber = 0;
    saNotificationBufferPtr->numberOfItems = 0;
    saNotificationBufferPtr->notification = NULL; // let AIS to reserve the memory
    // call saClmClusterTrack
    if( JNU_callSaClmClusterTrack_Sync( jniEnv,
                                        saClmHandle,
                                        saTrackFlags,
                                        saNotificationBufferPtr,
                                        &_saStatus ) != JNI_TRUE ){
        // error, some exception has been thrown already!
        // TODO handle NO_SPACE case!!
        return JNI_FALSE; // EXIT POINT!!!
    }
    // normal exit

    _TRACE2( "NATIVE: JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocAPI(...) returning normally\n" );

    return JNI_TRUE;
}

#else // IMPL_CLIENT_ALLOC

/**************************************************************************
 * FUNCTION:  JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr,
    SaClmClusterNotificationT* saNotifications,
    jboolean* bufferOnStackPtr )
{
    // VARIABLES
    SaAisErrorT _saStatus;
    SaClmClusterNotificationT* _saNotificationsPtr = NULL;

    // BODY

    // assert( saClmHandle != (SaClmHandleT) NULL );
    assert( ( saTrackFlags & ( ~ ( SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) ) ) == 0 );
    assert( saNotificationBufferPtr != NULL );
    assert( bufferOnStackPtr != NULL );
    _TRACE2( "NATIVE: Executing JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient(...)\n" );


    // init notification buffer
    saNotificationBufferPtr->viewNumber = 0;
    saNotificationBufferPtr->numberOfItems = DEFAULT_NUMBER_OF_ITEMS;
    saNotificationBufferPtr->notification = saNotifications;
    *bufferOnStackPtr = JNI_TRUE;
    // call saClmClusterTrack

    _TRACE2( "NATIVE: 1st try: calling saClmClusterTrack with buffer on the stack...\n" );

    if( JNU_callSaClmClusterTrack_Sync( jniEnv,
                                            saClmHandle,
                                            saTrackFlags,
                                            saNotificationBufferPtr,
                                            &_saStatus ) != JNI_TRUE ){
        // error handling (some exception has been thrown already!)
        if( _saStatus == SA_AIS_ERR_NO_SPACE ){

            _TRACE2( "NATIVE: 1st try failed: SA_AIS_ERR_NO_SPACE\n" );

            // clear exception
            (*jniEnv)->ExceptionClear( jniEnv );
            // reserve enough memory for notification buffer
            saNotificationBufferPtr->notification =
                            calloc( saNotificationBufferPtr->numberOfItems,
                                    sizeof( SaClmClusterNotificationT ) );
            if( saNotificationBufferPtr->notification == NULL ){
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoMemoryException",
                                    AIS_ERR_NO_MEMORY_MSG );
                return JNI_FALSE; // EXIT POINT!!!
            }
            // record ptr for free()
            _saNotificationsPtr = saNotificationBufferPtr->notification;
            *bufferOnStackPtr = JNI_FALSE;
            // try again

            _TRACE2( "NATIVE: 2nd try: calling saClmClusterTrack with a bigger buffer on the heap...\n" );

            if( JNU_callSaClmClusterTrack_Sync( jniEnv,
                                                    saClmHandle,
                                                    saTrackFlags,
                                                    saNotificationBufferPtr,
                                                    &_saStatus ) != JNI_TRUE ){
                // error handling (some exception has been thrown already!)
                return JNI_FALSE; // EXIT POINT!!!
            }
        }
        else{

            _TRACE2( "NATIVE: 1st try failed, no more tries\n" );

            return JNI_FALSE; // EXIT POINT!!!
        }
    }


    _TRACE2( "NATIVE: JNU_invokeSaClmClusterTrack_SyncNtfBuf_AllocClient(...) returning normally\n" );

    return JNI_TRUE;
}

#endif // IMPL_CLIENT_ALLOC


/**************************************************************************
 * FUNCTION:      JNU_callSaClmClusterTrack_Sync
 * TYPE:          internal function
 * OVERVIEW:
 * INTERFACE:
 *   parameters:  TODO
 *   returns:     JNI_FALSE if an error occured, JNI_TRUE otherwise
 * NOTE: If JNI_FALSE is returned, then an exception is already pending!
 *************************************************************************/
static jboolean JNU_callSaClmClusterTrack_Sync(
    JNIEnv* jniEnv,
    SaClmHandleT saClmHandle,
    const SaUint8T saTrackFlags,
    SaClmClusterNotificationBufferT* saNotificationBufferPtr,
    SaAisErrorT* saStatusPtr )
{
    // BODY

    // assert( saClmHandle != (SaClmHandleT) NULL );
    assert( ( saTrackFlags & ( ~ ( SA_TRACK_CHANGES | SA_TRACK_CHANGES_ONLY ) ) ) == 0 );
    assert( saNotificationBufferPtr != NULL );
    assert( saStatusPtr != NULL );
    _TRACE2( "NATIVE: Executing JNU_callSaClmClusterTrack_Sync(...)\n" );

    // call saClmClusterTrack

    U_printSaClusterNotificationBuffer( "Values of saNotificationBuffer BEFORE calling saClmClusterTrack: \n", saNotificationBufferPtr );

    *saStatusPtr = saClmClusterTrack( saClmHandle,
                                      SA_TRACK_CURRENT | saTrackFlags,
                                      saNotificationBufferPtr );

    _TRACE2( "NATIVE: saClmClusterTrack(...) has returned with %d...\n", *saStatusPtr );
    U_printSaClusterNotificationBuffer( "Values of saNotificationBuffer AFTER calling saClmClusterTrack: \n", saNotificationBufferPtr );


    // error handling
    if( ( *saStatusPtr ) != SA_AIS_OK ){
        switch( *saStatusPtr ){
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
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisBadHandleException",
                                    AIS_ERR_BAD_HANDLE_MSG );
                break;
            case SA_AIS_ERR_INIT:
                // TODO this should not happen here!
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisInitException",
                                    AIS_ERR_INIT_MSG );
                break;
            case SA_AIS_ERR_INVALID_PARAM:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                //JNU_throwNewByName( jniEnv,
                //                    "org/saforum/ais/AisInvalidParamException",
                //                    AIS_ERR_INVALID_PARAM_MSG );
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
            case SA_AIS_ERR_NO_SPACE:
                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisNoSpaceException",
                                    AIS_ERR_NO_SPACE_MSG );
                break;
            case SA_AIS_ERR_BAD_FLAGS:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                //JNU_throwNewByName( jniEnv,
                //                    "org/saforum/ais/AisBadFlagsException",
                //                    AIS_ERR_BAD_FLAGS_MSG );
                break;
            default:
                // this should not happen here!

                assert( JNI_FALSE );

                JNU_throwNewByName( jniEnv,
                                    "org/saforum/ais/AisLibraryException",
                                    AIS_ERR_LIBRARY_MSG );
                break;
        }
        return JNI_FALSE; // EXIT POINT!!!
    }
    // check numberOfItems
    if( saNotificationBufferPtr->numberOfItems == 0 ){

        _TRACE2( "NATIVE ERROR: saNotificationBufferptr->numberOfItems is 0\n" );

        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisLibraryException",
                            AIS_ERR_LIBRARY_MSG );
        return JNI_FALSE;
    }
    // check notification
    if( saNotificationBufferPtr->notification == NULL ){

        _TRACE2( "NATIVE ERROR: saNotificationBufferPtr->notification is NULL\n" );

        JNU_throwNewByName( jniEnv,
                            "org/saforum/ais/AisLibraryException",
                            AIS_ERR_LIBRARY_MSG );
        return JNI_FALSE;
    }

    // normal exit

    _TRACE2( "NATIVE: JNU_callSaClmClusterTrack_Sync(...) returning normally\n" );

    return JNI_TRUE;
}
