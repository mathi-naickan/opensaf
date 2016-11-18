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
 */

#ifndef CKPT_APITEST_TEST_CPSV_H_
#define CKPT_APITEST_TEST_CPSV_H_

#include "saAmf.h"
#include "saCkpt.h"
#include "string.h"
#include "stdio.h"
#include "math.h"
#include "errno.h"
#include "cpsv_papi.h"
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"

#define TEST_PASS	0
#define TEST_FAIL	1
#define TEST_UNRESOLVED	2
#define TEST_NOTINUSE	3
#define TEST_UNSUPPORTED	4
#define TEST_UNTESTED	5
#define TEST_UNINITIATED	6
#define TEST_NORESULT	7

struct SafCkptInitialize {
        SaCkptHandleT *ckptHandle;
        SaVersionT *vers;
        SaCkptCallbacksT *callbks;
        SaAisErrorT exp_output;
        char *result_string;
};

  
struct SafSelectionObject {
        SaCkptHandleT *ckptHandle;
        SaSelectionObjectT *selobj;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
 
struct SafCheckpointOpen {
       SaCkptHandleT *ckptHandle;
       const SaNameT *ckptName;
       const SaCkptCheckpointCreationAttributesT *ckptCreateAttr;
       SaCkptCheckpointOpenFlagsT ckptOpenFlags;
       SaTimeT timeout;
       SaCkptCheckpointHandleT *checkpointhandle;
       SaAisErrorT exp_output;
       char *result_string;
};
                                                                                                                                                                     
                                                                                                                                                                     
struct SafCheckpointOpenAsync {
       SaCkptHandleT *ckptHandle;
       SaInvocationT invocation;
       const SaNameT *ckptName;
       const SaCkptCheckpointCreationAttributesT *ckptCreateAttr;
       SaCkptCheckpointOpenFlagsT ckptOpenFlags;
       SaAisErrorT exp_output;
       char *result_string;
};
                                                                                                                                                                     
 
struct SafCheckpointSectionCreate {
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptSectionCreationAttributesT *sectionCreationAttributes;
        const void *initialData;
        SaSizeT *initialDataSize;
        SaAisErrorT exp_output;
        char *result_string;
};
 
struct SafCheckpointExpirationTimeSet{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptSectionIdT *sectionId;
        SaTimeT expirationTime;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafCheckpointDurationSet{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaTimeT retentionDuration;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
 
struct SafCheckpointWrite{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptIOVectorElementT *ioVector;
        SaUint32T *numberOfElements;
        SaUint32T *erroneousVectorIndex;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointRead{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptIOVectorElementT *ioVector;
        SaUint32T *numberOfElements;
        SaUint32T *erroneousVectorIndex;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafCheckpointOverwrite{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptSectionIdT *sectionId;
        const void *dataBuffer;
        SaSizeT *dataSize;
        SaAisErrorT exp_output;
        char *result_string;
};

                                                                                                                                                                     
struct SafCheckpointSynchronize{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaTimeT timeout;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointSynchronizeAsync{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaInvocationT invocation;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointActiveReplicaSet{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointStatusGet{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptCheckpointDescriptorT *checkpointStatus;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafCheckpointIterationInitialize{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptSectionsChosenT *sectionsChosen;
        SaTimeT expirationTime;
        SaCkptSectionIterationHandleT *sectionIterationHandle;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointIterationNext{
        SaCkptSectionIterationHandleT *sectionIterationHandle;
        SaCkptSectionDescriptorT *sectionDescriptor;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafCheckpointIterationFinalize{
        SaCkptSectionIterationHandleT *secIterHandle;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointUnlink {
        SaCkptHandleT *checkpointHandle;
        const SaNameT *checkpointName;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafCheckpointSectionDelete{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptSectionIdT *sectionId;
        SaAisErrorT exp_output;
        char *result_string;
};
struct SafCheckpointSectionFree{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptSectionIdT *sectionId;
         SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
                                                                                                                                                                     
struct SafCheckpointClose {
        SaCkptCheckpointHandleT *checkpointHandle;
        SaAisErrorT exp_output;
        char *result_string;
};

struct SafFinalize {
       SaCkptHandleT *ckptHandle;
       SaAisErrorT exp_output;
       char *result_string;
};

struct SafDispatch {
      SaCkptHandleT *ckptHandle;
      SaDispatchFlagsT flags;
      SaAisErrorT exp_output;
       char *result_string;
};

struct SafCheckpointOpenLarge {
       SaCkptHandleT *ckptHandle;
       const SaNameT *ckptName;
       const SaCkptCheckpointCreationAttributesT *ckptCreateAttr;
       SaCkptCheckpointOpenFlagsT ckptOpenFlags;
       SaTimeT timeout;
       SaCkptCheckpointHandleT *checkpointhandle;
       SaAisErrorT exp_output;
       char *result_string;
};
                                                                                                                                                                     
struct SafCheckpointSectionCreateLarge {
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptSectionCreationAttributesT *sectionCreationAttributes;
        const void *initialData;
        SaSizeT *initialDataSize;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
                                                                                                                                                                     
struct SafCheckpointWriteLarge{
        SaCkptCheckpointHandleT *checkpointHandle;
        const SaCkptIOVectorElementT *ioVector;
        SaUint32T *numberOfElements;
        SaUint32T *erroneousVectorIndex;
        SaAisErrorT exp_output;
        char *result_string;
};

                                                                                                                                                                     
struct SafCheckpointReadLarge{
        SaCkptCheckpointHandleT *checkpointHandle;
        SaCkptIOVectorElementT *ioVector;
        SaUint32T *numberOfElements;
        SaUint32T *erroneousVectorIndex;
        SaAisErrorT exp_output;
        char *result_string;
};
                                                                                                                                                                     
typedef enum {
    CKPT_INIT_SUCCESS_T=1,
    CKPT_INIT_NULL_CBKS_T,
    CKPT_INIT_NULL_VERSION_T,
    CKPT_INIT_VER_CBKS_NULL_T,
    CKPT_INIT_ERR_VERSION_T,
    CKPT_INIT_ERR_VERSION2_T,
    CKPT_INIT_OPEN_NULL_CBK_T,
    CKPT_INIT_SYNC_NULL_CBK_T,
    CKPT_INIT_NULL_CBKS2_T,
    CKPT_INIT_NULL_HDL_T,
    CKPT_INIT_NULL_PARAMS_T,
    CKPT_INIT_HDL_VER_NULL_T,
    CKPT_INIT_SUCCESS_HDL2_T,
}CKPT_INIT_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_SEL_SUCCESS_T=1,
    CKPT_SEL_HDL_NULL_T,
    CKPT_SEL_UNINIT_HDL_T,
    CKPT_SEL_NULL_OBJ_T,
    CKPT_SEL_HDL_OBJ_NULL_T,
    CKPT_SEL_FIN_HANDLE_T,
}CKPT_SEL_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_OPEN_BAD_HANDLE_T=1,
    CKPT_OPEN_BAD_HANDLE2_T,
    CKPT_OPEN_INVALID_PARAM_T,
    CKPT_OPEN_INVALID_PARAM2_T,
    CKPT_OPEN_ALL_CREATE_SUCCESS_T,
    CKPT_OPEN_ALL_WRITE_SUCCESS_T,
    CKPT_OPEN_ALL_READ_SUCCESS_T,
    CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,
    CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,
    CKPT_OPEN_ACTIVE_READ_SUCCESS_T,
    CKPT_OPEN_WEAK_CREATE_SUCCESS_T,
    CKPT_OPEN_WEAK_BAD_FLAGS_T,
    CKPT_OPEN_WEAK_WRITE_SUCCESS_T,
    CKPT_OPEN_WEAK_READ_SUCCESS_T,
    CKPT_OPEN_COLLOCATE_SUCCESS_T,
    CKPT_OPEN_COLLOCATE_SUCCESS2_T,
    CKPT_OPEN_BAD_NAME_T,
    CKPT_OPEN_COLLOCATE_INVALID_T,
    CKPT_OPEN_NULL_ATTR_T,
    CKPT_OPEN_ATTR_INVALID_T,
    CKPT_OPEN_ATTR_INVALID2_T,
    CKPT_OPEN_ATTR_INVALID3_T,
    CKPT_OPEN_ATTR_INVALID4_T,
    CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,
    CKPT_OPEN_COLLOCATE_READ_SUCCESS_T,
    CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,
    CKPT_OPEN_ERR_EXIST_T,
    CKPT_OPEN_ERR_EXIST2_T,
    CKPT_OPEN_ALL_CREATE_SUCCESS2_T,
    CKPT_OPEN_COLLOCATE_SUCCESS3_T,
    CKPT_OPEN_ALL_COLLOC_CREATE_SUCCESS_T,
    CKPT_OPEN_ALL_COLLOC_WRITE_SUCCESS_T,
    CKPT_OPEN_ALL_COLLOC_READ_SUCCESS_T,
    CKPT_OPEN_WEAK_COLLOC_CREATE_SUCCESS_T,
    CKPT_OPEN_WEAK_COLLOC_WRITE_SUCCESS_T,
    CKPT_OPEN_WEAK_COLLOC_READ_SUCCESS_T,
    CKPT_OPEN_MULTI_IO_REPLICA_T,
    CKPT_OPEN_ERR_NOT_EXIST_T,
    CKPT_OPEN_ALL_MODES_SUCCESS_T,
    CKPT_OPEN_INVALID_CR_FLAGS_T,
    CKPT_OPEN_ACTIVE_CREATE_WRITE_SUCCESS_T,
    CKPT_OPEN_SUCCESS_EXIST2_T,
    CKPT_OPEN_WEAK_CREATE_READ_SUCCESS_T,
    CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,
    CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_SUCCESS_T,
    CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T
}CKPT_OPEN_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_OPEN_ASYNC_BAD_HANDLE_T=1,
    CKPT_OPEN_ASYNC_ERR_INIT_T,
    CKPT_OPEN_ASYNC_ALL_CREATE_SUCCESS_T,
    CKPT_OPEN_ASYNC_ACTIVE_CREATE_SUCCESS_T,
    CKPT_OPEN_ASYNC_ACTIVE_WRITE_SUCCESS_T,
    CKPT_OPEN_ASYNC_COLLOCATE_SUCCESS2_T,
    CKPT_OPEN_ASYNC_BAD_NAME_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM2_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM3_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM4_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM5_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM6_T,
    CKPT_OPEN_ASYNC_INVALID_PARAM7_T,
    CKPT_OPEN_ASYNC_BAD_FLAGS_T,
    CKPT_OPEN_ASYNC_ALL_COLLOC_WRITE_SUCCESS_T,
    CKPT_OPEN_ASYNC_ALL_COLLOC_READ_SUCCESS_T,
    CKPT_OPEN_ASYNC_WEAK_COLLOC_WRITE_SUCCESS_T,
    CKPT_OPEN_ASYNC_WEAK_COLLOC_READ_SUCCESS_T,
    CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,
    CKPT_OPEN_ASYNC_NULL_INVOCATION,
    CKPT_OPEN_ASYNC_ERR_EXIST_T,
    CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_SUCCESS_T,
    CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T
}CKPT_OPEN_ASYNC_TC_TYPE;
 
                                                                                                                                                                     
typedef enum {
    CKPT_SECTION_CREATE_BAD_HANDLE_T=1,
    CKPT_SECTION_CREATE_INVALID_PARAM_T,
    CKPT_SECTION_CREATE_INVALID_PARAM2_T,
    CKPT_SECTION_CREATE_SUCCESS_T,
    CKPT_SECTION_CREATE_NULL_DATA_SIZE_ZERO_T,
    CKPT_SECTION_CREATE_NULL_DATA_SIZE_NOTZERO_T,
    CKPT_SECTION_CREATE_NOT_EXIST_T,
    CKPT_SECTION_CREATE_SUCCESS2_T,
    CKPT_SECTION_CREATE_ERR_ACCESS_T,
    CKPT_SECTION_CREATE_SUCCESS3_T,
    CKPT_SECTION_CREATE_NO_SPACE_T,
    CKPT_SECTION_CREATE_SUCCESS4_T,
    CKPT_SECTION_CREATE_ERR_EXIST_T,
    CKPT_SECTION_CREATE_SUCCESS5_T,
    CKPT_SECTION_CREATE_GEN_T,
    CKPT_SECTION_CREATE_SUCCESS7_T,
    CKPT_SECTION_CREATE_SUCCESS8_T,
    CKPT_SECTION_CREATE_SUCCESS9_T,
    CKPT_SECTION_CREATE_SUCCESS6_T,
    CKPT_SECTION_CREATE_MULTI_T,
    CKPT_SECTION_CREATE_MULTI2_T,
    CKPT_SECTION_CREATE_MULTI3_T,
    CKPT_SECTION_CREATE_MULTI4_T,
    CKPT_SECTION_CREATE_SUCCESS10_T,
    CKPT_SECTION_CREATE_GEN_EXIST_T,
    CKPT_SECTION_CREATE_GEN2_T,
    CKPT_SECTION_CREATE_INVALID_PARAM3_T,
    CKPT_SECTION_CREATE_LONG_SECION_ID_SUCCESS_T,
    CKPT_SECTION_CREATE_TOO_LONG_SECION_ID_SUCCESS_T,
}CKPT_SECTION_CREATE_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum{
    CKPT_EXP_BAD_HANDLE_T=1,
    CKPT_EXP_INVALID_PARAM_T,
    CKPT_EXP_ERR_ACCESS_T,
    CKPT_EXP_ERR_NOT_EXIST_T,
    CKPT_EXP_SUCCESS_T,
    CKPT_EXP_ERR_NOT_EXIST2_T,
    CKPT_EXP_SUCCESS2_T,
}CKPT_EXP_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_SECTION_DUR_SET_BAD_HANDLE_T=1,
    CKPT_SECTION_DUR_SET_SUCCESS_T,
    CKPT_SECTION_DUR_SET_ASYNC_SUCCESS_T,
    CKPT_SECTION_DUR_SET_ACTIVE_T,
    CKPT_SECTION_DUR_SET_BAD_OPERATION_T,
    CKPT_SECTION_DUR_SET_ZERO_T,
}CKPT_SECTION_DUR_SET_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_WRITE_BAD_HANDLE_T=1,
    CKPT_WRITE_NOT_EXIST_T,
    CKPT_WRITE_ERR_ACCESS_T,
    CKPT_WRITE_SUCCESS_T,
    CKPT_WRITE_SUCCESS2_T,
    CKPT_WRITE_INVALID_PARAM_T,
    CKPT_WRITE_SUCCESS3_T,
    CKPT_WRITE_SUCCESS5_T,
    CKPT_WRITE_SUCCESS6_T,
    CKPT_WRITE_SUCCESS4_T,
    CKPT_WRITE_NOT_EXIST2_T,
    CKPT_WRITE_SUCCESS_GEN_T,
    CKPT_WRITE_SUCCESS_GEN2_T,
    CKPT_WRITE_EXCEED_SIZE_T,
    CKPT_WRITE_MULTI_IO_T,
    CKPT_WRITE_MULTI_IO2_T,
    CKPT_WRITE_MULTI_IO3_T,
    CKPT_WRITE_SUCCESS7_T,
    CKPT_WRITE_SUCCESS8_T,
    CKPT_WRITE_FIN_HANDLE_T,
    CKPT_WRITE_NULL_VECTOR_T,
    CKPT_WRITE_NULL_INDEX_T,
    CKPT_WRITE_ERR_NO_RESOURCES_T,
    CKPT_WRITE_NOT_EXIST3_T,
}CKPT_WRITE_TC_TYPE;

                                                                                                                                                                     
typedef enum {
    CKPT_READ_BAD_HANDLE_T=1,
    CKPT_READ_NOT_EXIST_T,
    CKPT_READ_ERR_ACCESS_T,
    CKPT_READ_SUCCESS_T,
    CKPT_READ_BUFFER_NULL_T,
    CKPT_READ_INVALID_PARAM_T,
    CKPT_READ_INVALID_PARAM2_T,
    CKPT_READ_NOT_EXIST2_T,
    CKPT_READ_SUCCESS2_T,
    CKPT_READ_SUCCESS4_T,
    CKPT_READ_SUCCESS5_T,
    CKPT_READ_SUCCESS6_T,
    CKPT_READ_SUCCESS3_T,
    CKPT_READ_MULTI_IO_T,
    CKPT_READ_SUCCESS7_T,
    CKPT_READ_WEAK_REP_T,
    CKPT_READ_FIN_HANDLE_T,
    CKPT_READ_NULL_INDEX_T,
    CKPT_READ_NOT_EXIST3_T,
}CKPT_READ_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_OVERWRITE_BAD_HANDLE_T=1,
    CKPT_OVERWRITE_SUCCESS_T,
    CKPT_OVERWRITE_NOT_EXIST_T,
    CKPT_OVERWRITE_ERR_ACCESS_T,
    CKPT_OVERWRITE_SUCCESS2_T,
    CKPT_OVERWRITE_INVALID_PARAM_T,
    CKPT_OVERWRITE_NOT_EXIST2_T,
    CKPT_OVERWRITE_SUCCESS3_T,
    CKPT_OVERWRITE_SUCCESS4_T,
    CKPT_OVERWRITE_SUCCESS5_T,
    CKPT_OVERWRITE_GEN_T,
    CKPT_OVERWRITE_SUCCESS6_T,
    CKPT_OVERWRITE_FIN_HANDLE_T,
    CKPT_OVERWRITE_NULL_BUF_T,
    CKPT_OVERWRITE_NULL_SEC_T,
    CKPT_OVERWRITE_NOT_EXIST3_T,
}CKPT_OVERWRITE_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_SYNC_BAD_HANDLE_T=1,
    CKPT_SYNC_NOT_EXIST_T,
    CKPT_SYNC_ERR_ACCESS_T,
    CKPT_SYNC_BAD_OPERATION_T,
    CKPT_SYNC_SUCCESS_T,
    CKPT_SYNC_SUCCESS3_T,
    CKPT_SYNC_SUCCESS2_T,
    CKPT_SYNC_FIN_HANDLE_T,
    CKPT_SYNC_NULL_HANDLE_T,
    CKPT_SYNC_TRY_AGAIN_T,
}CKPT_SYNC_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_ASYNC_BAD_HANDLE_T=1,
    CKPT_ASYNC_NOT_EXIST_T,
    CKPT_ASYNC_ERR_ACCESS_T,
    CKPT_ASYNC_BAD_OPERATION_T,
    CKPT_ASYNC_ERR_INIT_T,
    CKPT_ASYNC_SUCCESS_T,
    CKPT_ASYNC_SUCCESS2_T,
    CKPT_ASYNC_FIN_HANDLE_T,
}CKPT_ASYNC_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_SET_BAD_HANDLE_T=1,
    CKPT_SET_BAD_OPERATION_T,
    CKPT_SET_ERR_ACCESS_T,
    CKPT_SET_SUCCESS_T,
    CKPT_SET_SUCCESS2_T,
    CKPT_SET_SUCCESS3_T,
    CKPT_SET_SUCCESS4_T,
    CKPT_SET_SUCCESS5_T,
    CKPT_SET_BAD_OPERATION2_T,
}CKPT_SET_TC_TYPE;

typedef enum {
    CKPT_STATUS_BAD_HANDLE_T=1,
    CKPT_STATUS_INVALID_PARAM_T,
    CKPT_STATUS_SUCCESS_T,
    CKPT_STATUS_SUCCESS2_T,
    CKPT_STATUS_SUCCESS4_T,
    CKPT_STATUS_SUCCESS3_T,
    CKPT_STATUS_SUCCESS5_T,
    CKPT_STATUS_SUCCESS6_T,
    CKPT_STATUS_SUCCESS7_T,
    CKPT_STATUS_ERR_NOT_EXIST_T,
    CKPT_STATUS_ERR_NOT_EXIST2_T,
}CKPT_STATUS_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_ITER_INIT_BAD_HANDLE_T=1,
    CKPT_ITER_INIT_FOREVER_T,
    CKPT_ITER_INIT_ANY_T,
    CKPT_ITER_INIT_INVALID_PARAM_T,
    CKPT_ITER_INIT_NOT_EXIST_T,
    CKPT_ITER_INIT_FINALIZE_T,
    CKPT_ITER_INIT_FOREVER2_T,
    CKPT_ITER_INIT_LEQ_EXP_T,
    CKPT_ITER_INIT_GEQ_EXP_T,
    CKPT_ITER_INIT_NULL_HANDLE_T,
    CKPT_ITER_INIT_SUCCESS_T,
    CKPT_ITER_INIT_SUCCESS2_T,
}CKPT_ITER_INIT_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_ITER_NEXT_BAD_HANDLE_T=1,
    CKPT_ITER_NEXT_INVALID_PARAM_T,
    CKPT_ITER_NEXT_SUCCESS_T,
    CKPT_ITER_NEXT_NO_SECTIONS_T,
    CKPT_ITER_NEXT_NOT_EXIST_T,
    CKPT_ITER_NEXT_FIN_HANDLE_T,
}CKPT_ITER_NEXT_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_ITER_FIN_BAD_HANDLE_T=1,
    CKPT_ITER_FIN_HANDLE_T,
    CKPT_ITER_FIN_HANDLE2_T,
    CKPT_ITER_FIN_FOREVER_T,
    CKPT_ITER_FIN_NOT_EXIST_T,
}CKPT_ITER_FIN_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_DEL_BAD_HANDLE_T=1,
    CKPT_DEL_ERR_ACCESS_T,
    CKPT_DEL_NOT_EXIST_T,
    CKPT_DEL_GEN2_T,
    CKPT_DEL_SUCCESS_T,
    CKPT_DEL_SUCCESS2_T,
    CKPT_DEL_NOT_EXIST2_T,
    CKPT_DEL_NOT_EXIST3_T,
    CKPT_DEL_NOT_EXIST4_T,
    CKPT_DEL_GEN_T,
    CKPT_DEL_SUCCESS3_T,
}CKPT_DEL_TC_TYPE;
                                                                                                                                                                     
typedef enum {                                                                                                                                                                     
CKPT_FREE_GEN_T=1   
}CKPT_FREE_TC_TYPE;
typedef enum {
    CKPT_UNLINK_BAD_HANDLE_T=1,
    CKPT_UNLINK_SUCCESS_T,
    CKPT_UNLINK_INVALID_PARAM_T,
    CKPT_UNLINK_SUCCESS2_T,
    CKPT_UNLINK_NOT_EXIST_T,
    CKPT_UNLINK_SUCCESS3_T,
    CKPT_UNLINK_SUCCESS4_T,
    CKPT_UNLINK_SUCCESS5_T,
    CKPT_UNLINK_SUCCESS6_T,
    CKPT_UNLINK_SUCCESS7_T,
    CKPT_UNLINK_SUCCESS8_T,
    CKPT_UNLINK_SUCCESS9_T,
    CKPT_UNLINK_SUCCESS10_T,
    CKPT_UNLINK_NOT_EXIST2_T,
    CKPT_UNLINK_NOT_EXIST3_T,
    CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_SUCCESS_T,
    CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_INVALID_PARAM_T,
}CKPT_UNLINK_TC_TYPE;

                                                                                                                                                                     
typedef enum {
    CKPT_CLOSE_BAD_HANDLE_T=1,
    CKPT_CLOSE_SUCCESS_T,
    CKPT_CLOSE_SUCCESS2_T,
    CKPT_CLOSE_SUCCESS3_T,
    CKPT_CLOSE_SUCCESS4_T,
    CKPT_CLOSE_SUCCESS5_T,
    CKPT_CLOSE_SUCCESS6_T,
    CKPT_CLOSE_SUCCESS7_T,
    CKPT_CLOSE_SUCCESS8_T,
    CKPT_CLOSE_SUCCESS9_T,
    CKPT_CLOSE_SUCCESS10_T,
    CKPT_CLOSE_SUCCESS11_T,
    CKPT_CLOSE_BAD_HANDLE2_T,
}CKPT_CLOSE_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_FIN_BAD_HANDLE_T=1,
    CKPT_FIN_SUCCESS_T,
    CKPT_FIN_SUCCESS2_T,
    CKPT_FIN_SUCCESS3_T,
    CKPT_FIN_NULL_HANDLE_T,
    CKPT_FIN_SUCCESS4_T,
    CKPT_FIN_SUCCESS5_T,
    CKPT_FIN_SUCCESS6_T,
    CKPT_FIN_SUCCESS7_T,
    CKPT_FIN_SUCCESS8_T,
}CKPT_FIN_TC_TYPE;
                                                                                                                                                                     
typedef enum {
    CKPT_DISPATCH_BAD_HANDLE_T=1,
    CKPT_DISPATCH_BAD_HANDLE2_T,
    CKPT_DISPATCH_BAD_HANDLE3_T,
    CKPT_DISPATCH_BAD_HANDLE4_T,
    CKPT_DISPATCH_BAD_HANDLE5_T,
    CKPT_DISPATCH_INVALID_PARAM_T,
    CKPT_DISPATCH_ONE_T,
    CKPT_DISPATCH_ALL_T,
    CKPT_DISPATCH_BLOCKING_T,
}CKPT_DISPATCH_TC_TYPE;

                                                                                                                                                                     
typedef enum {
    CKPT_OPEN_LARGE_ALL_CREATE_SUCCESS_T=1,
    CKPT_OPEN_LARGE_ALL_WRITE_SUCCESS_T,
    CKPT_OPEN_LARGE_ALL_READ_SUCCESS_T,
    CKPT_OPEN_LARGE_ACTIVE_CREATE_SUCCESS_T,
    CKPT_OPEN_LARGE_ACTIVE_WRITE_SUCCESS_T,
    CKPT_OPEN_LARGE_ACTIVE_READ_SUCCESS_T,
    CKPT_OPEN_LARGE_WEAK_CREATE_SUCCESS_T,
    CKPT_OPEN_LARGE_WEAK_WRITE_SUCCESS_T,
    CKPT_OPEN_LARGE_WEAK_READ_SUCCESS_T,
    CKPT_OPEN_LARGE_COLLOCATE_SUCCESS_T,
    CKPT_OPEN_LARGE_COLLOCATE_SUCCESS2_T,
    CKPT_OPEN_LARGE_COLLOCATE_WRITE_SUCCESS_T,
    CKPT_OPEN_LARGE_COLLOCATE_READ_SUCCESS_T,
    CKPT_OPEN_LARGE_COLLOCATE_WRITE_SUCCESS2_T,
}CKPT_OPEN_LARGE_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_SECTION_LARGE_CREATE_SUCCESS_T=1,
    CKPT_SECTION_LARGE_CREATE_SUCCESS2_T,
    CKPT_SECTION_LARGE_CREATE_SUCCESS3_T,
    CKPT_SECTION_LARGE_CREATE_SUCCESS4_T,
    CKPT_SECTION_LARGE_CREATE_SUCCESS5_T,
    CKPT_SECTION_LARGE_CREATE_SUCCESS6_T,
}CKPT_SECTION_LARGE_CREATE_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_WRITE_LARGE_SUCCESS_T=1,
    CKPT_WRITE_LARGE_SUCCESS2_T,
    CKPT_WRITE_LARGE_SUCCESS3_T,
    CKPT_WRITE_LARGE_SUCCESS4_T,
}CKPT_WRITE_LARGE_TC_TYPE;
                                                                                                                                                                     
                                                                                                                                                                     
typedef enum {
    CKPT_READ_LARGE_SUCCESS_T=1,
    CKPT_READ_LARGE_SUCCESS3_T,
    CKPT_READ_LARGE_BUFFER_NULL_T,
    CKPT_READ_LARGE_SUCCESS2_T,
}CKPT_READ_LARGE_TC_TYPE;

typedef struct test_cpsv_inst {
    int inst_num;
    int test_list;
    int test_case_num;
    int num_of_iter;
    int node_id;
    int redFlag;
    SaNameT all_rep_ckpt_name;
    SaNameT active_rep_ckpt_name;
    SaNameT weak_rep_ckpt_name;
    SaNameT collocated_ckpt_name;
    SaNameT async_all_rep_name;
    SaNameT async_active_rep_name;
    SaNameT all_colloc_name;
    SaNameT weak_colloc_name;
    SaNameT multi_vector_ckpt_name;
    SaNameT all_rep_lckpt_name;
    SaNameT active_rep_lckpt_name;
    SaNameT weak_rep_lckpt_name;
    SaNameT collocated_lckpt_name;
}TEST_CPSV_INST;
 void cpsv_it_init_01(void);
 void cpsv_it_init_02(void);
 void cpsv_it_init_03(void);
 void cpsv_it_init_04(void);
 void cpsv_it_init_05(void);
 void cpsv_it_init_06(void);
 void cpsv_it_init_07(void);
 void cpsv_it_init_08(void);
 void cpsv_it_init_09(void);
 void cpsv_it_init_10(void);
 void cpsv_it_sel_01(void);
 void cpsv_it_sel_02(void);
 void cpsv_it_sel_03(void);
 void cpsv_it_sel_04(void);
 void cpsv_it_sel_05(void);
 void cpsv_it_sel_06(void);
 void cpsv_it_disp_01(void);
 void cpsv_it_disp_02(void);
 void cpsv_it_disp_03(void);
 void cpsv_it_disp_04(void);
 void cpsv_it_disp_05(void);
 void cpsv_it_disp_06(void);
 void cpsv_it_disp_07(void);
 void cpsv_it_disp_08(void);
 void cpsv_it_disp_09(void);
 void cpsv_it_fin_01(void);
 void cpsv_it_fin_02(void);
 void cpsv_it_fin_03(void);
 void cpsv_it_fin_04(void);
 void cpsv_it_open_01(void);
 void cpsv_it_open_02(void);
 void cpsv_it_open_03(void);
 void cpsv_it_open_04(void);
 void cpsv_it_open_05(void);
 void cpsv_it_open_06(void);
 void cpsv_it_open_07(void);
 void cpsv_it_open_08(void);
 void cpsv_it_open_10(void);
 void cpsv_it_open_11(void);
 void cpsv_it_open_12(void);
 void cpsv_it_open_13(void);
 void cpsv_it_open_14(void);
 void cpsv_it_open_15(void);
 void cpsv_it_open_16(void);
 void cpsv_it_open_17(void);
 void cpsv_it_open_18(void);
 void cpsv_it_open_19(void);
 void cpsv_it_open_20(void);
 void cpsv_it_open_21(void);
 void cpsv_it_open_22(void);
 void cpsv_it_open_23(void);
 void cpsv_it_open_24(void);
 void cpsv_it_open_25(void);
 void cpsv_it_open_26(void);
 void cpsv_it_open_27(void);
 void cpsv_it_open_28(void);
 void cpsv_it_open_29(void);
 void cpsv_it_open_30(void);
 void cpsv_it_open_31(void);
 void cpsv_it_open_32(void);
 void cpsv_it_open_33(void);
 void cpsv_it_open_34(void);
 void cpsv_it_open_35(void);
 void cpsv_it_open_36(void);
 void cpsv_it_open_37(void);
 void cpsv_it_open_38(void);
 void cpsv_it_open_39(void);
 void cpsv_it_open_40(void);
 void cpsv_it_open_41(void);
 void cpsv_it_open_42(void);
 void cpsv_it_open_43(void);
 void cpsv_it_open_44(void);
 void cpsv_it_open_45(void);
 void cpsv_it_open_46(void);
 void cpsv_it_open_47(void);
 void cpsv_it_open_48(void);
 void cpsv_it_open_49(void);
 void cpsv_it_open_50(void);
 void cpsv_it_open_51(void);
 void cpsv_it_open_52(void);
 void cpsv_it_open_53(void);
 void cpsv_it_open_54(void);
 void cpsv_it_close_01(void);
 void cpsv_it_close_02(void);
 void cpsv_it_close_03(void);
 void cpsv_it_close_04(void);
 void cpsv_it_close_05(void);
 void cpsv_it_close_06(void);
 void cpsv_it_close_07(void);
 void cpsv_it_close_08(void);
 void cpsv_it_unlink_01(void);
 void cpsv_it_unlink_02(void);
 void cpsv_it_unlink_03(void);
 void cpsv_it_unlink_04(void);
 void cpsv_it_unlink_05(void);
 void cpsv_it_unlink_06(void);
 void cpsv_it_unlink_07(void);
 void cpsv_it_unlink_08(void);
 void cpsv_it_unlink_09(void);
 void cpsv_it_unlink_10(void);
 void cpsv_it_unlink_11(void);
 void cpsv_it_rdset_01(void);
 void cpsv_it_rdset_02(void);
 void cpsv_it_rdset_03(void);
 void cpsv_it_rdset_04(void);
 void cpsv_it_rdset_05(void);
 void cpsv_it_rdset_06(void);
 void cpsv_it_repset_01(void);
 void cpsv_it_repset_02(void);
 void cpsv_it_repset_03(void);
 void cpsv_it_repset_04(void);
 void cpsv_it_repset_05(void);
 void cpsv_it_status_01(void);
 void cpsv_it_status_02(void);
 void cpsv_it_status_03(void);
 void cpsv_it_status_05(void);
 void cpsv_it_status_06(void);
 void cpsv_it_seccreate_01(void);
 void cpsv_it_seccreate_02(void);
 void cpsv_it_seccreate_03(void);
 void cpsv_it_seccreate_04(void);
 void cpsv_it_seccreate_05(void);
 void cpsv_it_seccreate_07(void);
 void cpsv_it_seccreate_09(void);
 void cpsv_it_seccreate_10(void);
 void cpsv_it_seccreate_11(void);
 void cpsv_it_seccreate_12(void);
 void cpsv_it_seccreate_13(void);
 void cpsv_it_seccreate_14(void);
 void cpsv_it_seccreate_15(void);
 void cpsv_it_seccreate_16(void);
 void cpsv_it_seccreate_17(void);
 void cpsv_it_seccreate_18();
 void cpsv_it_secdel_01(void);
 void cpsv_it_secdel_02(void);
 void cpsv_it_secdel_03(void);
 void cpsv_it_secdel_04(void);
 void cpsv_it_secdel_08(void);
 void cpsv_it_secdel_09(void);
 void cpsv_it_expset_01(void);
 void cpsv_it_expset_02(void);
 void cpsv_it_expset_03(void);
 void cpsv_it_expset_04(void);
 void cpsv_it_expset_05(void);
 void cpsv_it_expset_06(void);
 void cpsv_it_expset_07(void);
 void cpsv_it_iterinit_01(void);
 void cpsv_it_iterinit_02(void);
 void cpsv_it_iterinit_03(void);
 void cpsv_it_iterinit_04(void);
 void cpsv_it_iterinit_05(void);
 void cpsv_it_iterinit_06(void);
 void cpsv_it_iterinit_07(void);
 void cpsv_it_iterinit_08(void);
 void cpsv_it_iterinit_09(void);
 void cpsv_it_iterinit_10(void);
 void cpsv_it_iterinit_12(void);
 void cpsv_it_iternext_01(void);
 void cpsv_it_iternext_02(void);
 void cpsv_it_iternext_03(void);
 void cpsv_it_iternext_04(void);
 void cpsv_it_iternext_05(void);
 void cpsv_it_iternext_06(void);
 void cpsv_it_iternext_08(void);
 void cpsv_it_iternext_09(void);
 void cpsv_it_iterfin_01(void);
 void cpsv_it_iterfin_02(void);
 void cpsv_it_iterfin_04(void);
 void cpsv_it_write_01(void);
 void cpsv_it_write_02(void);
 void cpsv_it_write_03(void);
 void cpsv_it_write_04(void);
 void cpsv_it_write_05(void);
 void cpsv_it_write_06(void);
 void cpsv_it_write_07(void);
 void cpsv_it_write_08(void);
 void cpsv_it_write_09(void);
 void cpsv_it_write_10(void);
 void cpsv_it_write_11(void);
 void cpsv_it_write_12(void);
 void cpsv_it_write_14(void);
 void cpsv_it_write_16(void);
 void cpsv_it_read_01(void);
 void cpsv_it_read_02(void);
 void cpsv_it_read_03(void);
 void cpsv_it_read_04(void);
 void cpsv_it_read_05(void);
 void cpsv_it_read_06(void);
 void cpsv_it_read_07(void);
 void cpsv_it_read_08(void);
 void cpsv_it_read_10(void);
 void cpsv_it_read_11(void);
 void cpsv_it_read_13(void);
 void cpsv_it_sync_01(void);
 void cpsv_it_sync_02(void);
 void cpsv_it_sync_03(void);
 void cpsv_it_sync_04(void);
 void cpsv_it_sync_05(void);
 void cpsv_it_sync_06(void);
 void cpsv_it_sync_07(void);
 void cpsv_it_sync_08(void);
 void cpsv_it_sync_09(void);
 void cpsv_it_sync_10(void);
 void cpsv_it_sync_11(void);
 void cpsv_it_sync_12(void);
 void cpsv_it_sync_13(void);
 void cpsv_it_sync_14(void);
 void cpsv_it_overwrite_01(void);
 void cpsv_it_overwrite_02(void);
 void cpsv_it_overwrite_03(void);
 void cpsv_it_overwrite_04(void);
 void cpsv_it_overwrite_05(void);
 void cpsv_it_overwrite_06(void);
 void cpsv_it_overwrite_07(void);
 void cpsv_it_overwrite_08(void);
 void cpsv_it_overwrite_09(void);
 void cpsv_it_overwrite_10(void);
 void cpsv_it_overwrite_11(void);
 void cpsv_it_overwrite_12(void);
 void cpsv_it_openclbk_01(void);
 void cpsv_it_openclbk_02(void);
 void cpsv_it_syncclbk_01(void);
 void cpsv_it_onenode_01(void);
 void cpsv_it_onenode_02(void);
 void cpsv_it_onenode_03(void);
 void cpsv_it_onenode_04(void);
 void cpsv_it_onenode_05(void);
 void cpsv_it_onenode_07(void);
 void cpsv_it_onenode_08(void);
 void cpsv_it_onenode_10(void);
 void cpsv_it_onenode_11(void);
 void cpsv_it_onenode_12(void);
 void cpsv_it_onenode_13(void);
 void cpsv_it_onenode_17(void);
 void cpsv_it_onenode_18(void);
 void cpsv_it_onenode_19(void);
 void printStatus(SaCkptCheckpointDescriptorT ckptStatus);
 void printHandles(void);
 void selection_thread (NCSCONTEXT arg);
 void selection_thread1(NCSCONTEXT arg);
 void selection_thread2(NCSCONTEXT arg);
 void selection_thread3(NCSCONTEXT arg);
 void selection_thread4(NCSCONTEXT arg);
 void createBlockingThreads(void);
 void test_readFile(void);
 void cpsv_clean_clbk_params(void);
 void handleAssigner(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle);
 void fill_ckpt_version(SaVersionT *version,SaUint8T rel_code,SaUint8T mjr_ver,SaUint8T mnr_ver);
 void fill_ckpt_callbacks(SaCkptCallbacksT *clbks,SaCkptCheckpointOpenCallbackT open_clbk,SaCkptCheckpointSynchronizeCallbackT sync_clbk);
 void fill_ckpt_name(SaNameT *name,char *string);
 void fill_sec_attri(SaCkptSectionCreationAttributesT *sec_cr_attr,SaCkptSectionIdT *sec,SaTimeT exp_time);
 void AppCkptOpenCallback(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle, SaAisErrorT error);
 void AppCkptSyncCallback(SaInvocationT invocation, SaAisErrorT error);
 void fill_testcase_data(void);
 void test_cpsv_get_inputs(TEST_CPSV_INST *inst);
 void test_initialize(void);
 void test_cpsv_fill_inputs(TEST_CPSV_INST *inst);
 void app1(void);
 void app2(void);
 void cpsv_it_noncolloc_01(void);
 void cpsv_it_unlinktest(void);
 void cpsv_it_arr_default_sec(void);
 void cpsv_it_read_withoutwrite(void);
 void cpsv_it_read_withnullbuf(void);
 void cpsv_it_arr_invalid_param(void);
 void test_run_cpsv_instance(TEST_CPSV_INST *inst);
 void test_run_cpsv_app(void);
 void selection_thread5(NCSCONTEXT arg);
 


#endif  // CKPT_APITEST_TEST_CPSV_H_

