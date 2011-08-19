/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class org_opensaf_ais_clm_ClmHandleImpl */

#ifndef _Included_org_opensaf_ais_clm_ClmHandleImpl
#define _Included_org_opensaf_ais_clm_ClmHandleImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    finalizeClmHandle
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_finalizeClmHandle
  (JNIEnv *, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    invokeSaClmInitialize
 * Signature: (Lorg/saforum/ais/Version;)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmInitialize
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    invokeSaClmDispatch
 * Signature: (I)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatch
  (JNIEnv *, jobject, jint);

/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    invokeSaClmDispatchWhenReady
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmDispatchWhenReady
  (JNIEnv *, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    invokeSaClmSelectionObjectGet
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmSelectionObjectGet
  (JNIEnv *, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClmHandleImpl
 * Method:    invokeSaClmResponse
 * Signature: (JLorg/saforum/ais/CallbackResponse;)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClmHandleImpl_invokeSaClmResponse
  (JNIEnv *, jobject, jlong, jobject);

#ifdef __cplusplus
}
#endif
#endif

/* Header for class org_opensaf_ais_clm_ClusterMembershipManagerImpl */

#ifndef _Included_org_opensaf_ais_clm_ClusterMembershipManagerImpl
#define _Included_org_opensaf_ais_clm_ClusterMembershipManagerImpl
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getCluster
 * Signature: ()Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 */
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getCluster__
  (JNIEnv *, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getCluster
 * Signature: (Z)Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 */
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getCluster__Z
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterAsync
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsync__
  (JNIEnv *, jobject);
/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterAsync
 * Signature: (Z)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsync__Z
  (JNIEnv *, jobject, jboolean);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterNode
 * Signature: (IJ)Lorg/saforum/ais/clm/ClusterNode;
 */
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNode
  (JNIEnv *, jobject, jint, jlong);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    stopClusterTracking
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_stopClusterTracking
  (JNIEnv *, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterAsyncThenStartTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsyncThenStartTracking__Lorg_saforum_ais_TrackFlags_2
  (JNIEnv *, jobject, jobject);
/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterAsyncThenStartTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;ZI)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterAsyncThenStartTracking__Lorg_saforum_ais_TrackFlags_2ZI
  (JNIEnv *, jobject, jobject, jboolean, jint);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterThenStartTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;)Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 */
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterThenStartTracking__Lorg_saforum_ais_TrackFlags_2
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterThenStartTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;ZI)Lorg/saforum/ais/clm/ClusterNotificationBuffer;
 */
JNIEXPORT jobject JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterThenStartTracking__Lorg_saforum_ais_TrackFlags_2ZI
  (JNIEnv *, jobject, jobject, jboolean, jint);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    getClusterNodeAsync
 * Signature: (JI)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_getClusterNodeAsync
  (JNIEnv *, jobject, jlong, jint);
/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    startClusterTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_startClusterTracking__Lorg_saforum_ais_TrackFlags_2
  (JNIEnv *, jobject, jobject);

/*
 * Class:     org_opensaf_ais_clm_ClusterMembershipManagerImpl
 * Method:    startClusterTracking
 * Signature: (Lorg/saforum/ais/TrackFlags;ZI)V
 */
JNIEXPORT void JNICALL Java_org_opensaf_ais_clm_ClusterMembershipManagerImpl_startClusterTracking__Lorg_saforum_ais_TrackFlags_2ZI
  (JNIEnv *, jobject, jobject, jboolean, jint);

#ifdef __cplusplus
}
#endif
#endif

