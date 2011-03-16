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
#define OPENSAF_IMM_OBJECT_DN "opensafImm=opensafImm,safApp=safImmService"
#define OPENSAF_IMM_OBJECT_RDN "opensafImm=opensafImm"
#define OPENSAF_IMM_OBJECT_PARENT "safApp=safImmService"

#define OPENSAF_IMM_SYNC_BATCH_SIZE "opensafImmSyncBatchSize"
#define IMMSV_DEFAULT_MAX_SYNC_BATCH_SIZE 4096
/*MDS_DIRECT_BUF_MAXSIZE 8000 possibly adjust fevs to this in the future. */
#define IMMSV_MAX_OBJS_IN_SYNCBATCH 400

/*Max # of outstanding fevs messages towards director.*/
/*Note max-max is 255. cb->fevs_replies_pending is an uns8*/
#define IMMSV_DEFAULT_FEVS_MAX_PENDING 16


#define OPENSAF_IMM_PBE_IMPL_NAME "OpenSafImmPBE"

#define IMMSV_MAX_OBJECTS 10000


/* Admin operation IDs */
/* Internal PBE operation ids. */
#define OPENSAF_IMM_PBE_CLASS_CREATE 0x10000000
#define OPENSAF_IMM_PBE_CLASS_DELETE 0x20000000
#define OPENSAF_IMM_PBE_UPDATE_EPOCH 0x30000000
/* Public operation ids on OpenSafImmPBE */
#define OPENSAF_IMM_NOST_FLAG_ON     0x00000001
#define OPENSAF_IMM_NOST_FLAG_OFF    0x00000002

#define OPENSAF_IMM_FLAG_SCHCH_ALLOW 0x00000001

#define OPENSAF_IMM_SERVICE_NAME "safImmService"

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
