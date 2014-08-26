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
 * Author(s): Ericsson AB
 *
 */

/*****************************************************************************
  DESCRIPTION:
  This file containts some NON STANDARD private API structures and prototypes
  used by the OpenSaf IMM implementation.

*****************************************************************************/

#ifndef IMMSV_API_H
#define IMMSV_API_H

#ifdef  __cplusplus
extern "C" {
#endif

/* The OpensafImm IMM class and opensafImm singleton IMM object is used by 
   the opensaf IMM implementation. It is not part of the IMM standard. */

#define OPENSAF_IMM_CLASS_NAME   "OpensafImm"
#define OPENSAF_IMM_ATTR_CLASSES "opensafImmClassNames"
#define OPENSAF_IMM_ATTR_EPOCH "opensafImmEpoch"
#define OPENSAF_IMM_ATTR_RDN "opensafImm"
#define OPENSAF_IMM_ATTR_NOSTD_FLAGS "opensafImmNostdFlags"
#define OPENSAF_IMM_ATTR_OTHER_SC_UP "mIsOtherScUp"
#define OPENSAF_IMM_OBJECT_DN "opensafImm=opensafImm,safApp=safImmService"
#define OPENSAF_IMM_OBJECT_RDN "opensafImm=opensafImm"
#define OPENSAF_IMM_OBJECT_PARENT "safApp=safImmService"
#define OPENSAF_IMM_PBE_RT_CLASS_NAME   "OsafImmPbeRt"
#define OPENSAF_IMM_PBE_IMPL_NAME "OpenSafImmPBE"
#define OPENSAF_IMM_2PBE_APPL_NAME "@OpenSafImmPBE"
#define OPENSAF_IMM_PBE_RT_IMPL_NAME_A "OsafImmPbeRt_A"
#define OPENSAF_IMM_PBE_RT_IMPL_NAME_B "OsafImmPbeRt_B"
#define OPENSAF_IMM_ATTR_PBE_RT_RDN "osafImmPbeRt"
#define OPENSAF_IMM_PBE_RT_OBJECT_DN_A "osafImmPbeRt=A,opensafImm=opensafImm,safApp=safImmService"
#define OPENSAF_IMM_PBE_RT_OBJECT_DN_B "osafImmPbeRt=B,opensafImm=opensafImm,safApp=safImmService"
#define OPENSAF_IMM_ATTR_PBE_RT_EPOCH "epoch"
#define OPENSAF_IMM_ATTR_PBE_RT_CCB "lastCcbApply"
#define OPENSAF_IMM_ATTR_PBE_RT_TIME "lastApplyTime"

#define OPENSAF_IMM_SYNC_BATCH_SIZE "opensafImmSyncBatchSize"
/* Adjust to 90% of MDS_DIRECT_BUF_MAXSIZE  */
#define IMMSV_DEFAULT_MAX_SYNC_BATCH_SIZE ((MDS_DIRECT_BUF_MAXSIZE / 100) * 90) 
#define IMMSV_MAX_OBJS_IN_SYNCBATCH (IMMSV_DEFAULT_MAX_SYNC_BATCH_SIZE / 10) 

#define OPENSAF_IMM_LONG_DNS_ALLOWED "longDnsAllowed"
#define OPENSAF_IMM_ACCESS_CONTROL_MODE "accessControlMode"
#define OPENSAF_IMM_AUTHORIZED_GROUP "authorizedGroup"

typedef enum {
	ACCESS_CONTROL_DISABLED = 0,
	ACCESS_CONTROL_PERMISSIVE = 1,
	ACCESS_CONTROL_ENFORCING = 2
} OsafImmAccessControlModeT;

/*Max # of outstanding fevs messages towards director.*/
/*Note max-max is 255. cb->fevs_replies_pending is an uint8_t*/
#define IMMSV_DEFAULT_FEVS_MAX_PENDING 16

#define IMMSV_MAX_OBJECTS 10000
#define IMMSV_MAX_ATTRIBUTES 128
#define IMMSV_MAX_ADMO_NAME_LENGTH 256
#define IMMSV_MAX_ATTR_NAME_LENGTH 256
#define IMMSV_MAX_PARAM_NAME_LENGTH 256
#define IMMSV_MAX_IMPL_NAME_LENGTH 256
#define IMMSV_MAX_CLASS_NAME_LENGTH 256

/* The pair of magic names for special appliers (#2873)
   If such an appliers are attached, then the immsv will generate
   callbacks directed at them, for ccb-create, ccb-delete, ccb-modify,
   rto-create, rto-delete or rta-update, when/if the operation
   includes any attributes that have been flagged SA_IMM_ATTR_NOTIFY
   in the class definition.
*/
#define OPENSAF_IMM_REPL_A_NAME "@OpenSafImmReplicatorA"
#define OPENSAF_IMM_REPL_B_NAME "@OpenSafImmReplicatorB"

/* Used internally in the SaImmCcbFlagsT space to indicate that
   a ccb currently contains at lest one operation that either
   a) creates an object with no dangling attribute(s) (that is/are not empty).
   b) deletes an object that has no dangling attribute(s) (that are not empty).
   c) modifies a NO_DANGLING attribute,
   d) deletes an object that is referred to by some NO_DANGLING attribute.
*/
#define OPENSAF_IMM_CCB_NO_DANGLING_MUTATE 0x40000000

/* Used internally in the SaImmCcbFlagsT space to indicate that
   a ccb currently contains at least one create or at least one
   modify on an object that has SA_IMM_ATTR_NOTIFY set on one of
   its attributes and special applier is involved. This means that
   the admin-owner for the ccb has already been communicated to
   the special applier. 
 */
#define OPENSAF_IMM_CCB_ADMO_PROVIDED 0x80000000


/* Used to encode fake OI reply on admin-op, when the OI has not 
   registered any admin-op callback. Converted to SA_AIS_ERR_BAD_OPERATION
   return code on the OM invoking side. 
*/
#define IMMSV_IMPOSSIBLE_ERROR 0xffff0000 


/* Admin operation IDs */
/* Internal PBE operation ids. */
#define OPENSAF_IMM_PBE_CLASS_CREATE 0x10000000
#define OPENSAF_IMM_PBE_CLASS_DELETE 0x20000000
#define OPENSAF_IMM_PBE_UPDATE_EPOCH 0x30000000
#define OPENSAF_IMM_2PBE_PRELOAD_STAT 0x40000000
#define OPENSAF_IMM_PBE_CCB_PREPARE 0x50000000
#define OPENSAF_IMM_BAD_OP_BOUNCE 0x60000000

/* Public operation ids on OpenSafImmPBE */
#define OPENSAF_IMM_NOST_FLAG_ON     0x00000001
#define OPENSAF_IMM_NOST_FLAG_OFF    0x00000002

/* Flag values for nostdflags. */
#define OPENSAF_IMM_FLAG_SCHCH_ALLOW 0x00000001
#define OPENSAF_IMM_FLAG_PRT41_ALLOW 0x00000002
#define OPENSAF_IMM_FLAG_PRT43_ALLOW 0x00000004
#define OPENSAF_IMM_FLAG_2PBE1_ALLOW 0x00000008
#define OPENSAF_IMM_FLAG_PRT45_ALLOW 0x00000010


#define OPENSAF_IMM_SERVICE_NAME "safImmService"

typedef enum {
    SA_IMM_ADMIN_EXPORT = 1,  /* Defined in A.02.01 declared in  A.03.01 */
    SA_IMM_ADMIN_INIT_FROM_FILE = 100 /* Non standard, force PBE disable. */
} SaImmMngtAdminOperationT;

/* 
 * Special flags only to be used by the imm-dummper or the imm-loader.
 *
 * The first excludes non persistent runtime attributes from the dump.
 * A normal iteration would want to return these values, but the dump should
 * not dump non-persistent attributes.
 *
 * The second includes cached attributes even when there is no implementer
 * currently. An implementer that re-attaches after a failure may continue
 * with the cached value provided by its predecessor (i.e. not change it).
 * It is then important that the sync has fetched this value even when there
 * is no implementer. A normal iteration will exclude such values.
 *
 * The use of these flags in search options is NON STANDARD.
 * It is only allowed for imm internal use. 
 * We are messing with the a part of the value space for search options.
 * This may not be possible in future implementations.
 *
 * The defines should really not use the SA_IMM prefix as they are non
 * standard. But I choose use the same prefix as the corrsponding defines
 * in the standard, as a reminder of where I am tresspassing. 
 * This includefile is not part of the public API anyway.
*/
#define SA_IMM_SEARCH_PERSISTENT_ATTRS         0x0010
#define SA_IMM_SEARCH_SYNC_CACHED_ATTRS        0x0020

/* These functions are private and nonstandard parts of the IMM client
   (agent) API. They are used by the process that drives the immnd sync.
   The immsv_sync function is NOT reentrant. It must be used by only
   one thread.
*/
	SaAisErrorT
	 immsv_sync(SaImmHandleT immHandle,
		    const SaImmClassNameT className, const SaNameT *objectName,
		    const SaImmAttrValuesT_2 **atributes, void** batch,
		    int* remainingSpace, int objsInBatch);

	SaAisErrorT
	 immsv_finalize_sync(SaImmHandleT immHandle);

#ifdef  __cplusplus
}
#endif

#endif   /* IMMSV_API_H */
