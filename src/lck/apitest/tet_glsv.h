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

#ifndef LCK_APITEST_TET_GLSV_H_
#define LCK_APITEST_TET_GLSV_H_

#include "tet_startup.h"
#include "ncs_lib.h"
#include "saLck.h"

#define SMOKETEST_NODE1 12
#define SMOKETEST_NODE2 4
#define RES_OPEN_TIMEOUT 1000000000ULL
#define RES_LOCK_TIMEOUT 10000000000ULL
#define RES_UNLOCK_TIMEOUT 1000000000ULL
#define m_TET_GLSV_PRINTF printf
#define m_GLSV_WAIT sleep(2)

/* saLckInitialize */

typedef enum {
   LCK_INIT_NULL_HANDLE_T=1,
   LCK_INIT_NULL_VERSION_T,
   LCK_INIT_NULL_PARAMS_T,
   LCK_INIT_NULL_CBK_PARAM_T,
   LCK_INIT_NULL_VERSION_CBKS_T,
   LCK_INIT_BAD_VERSION_T,
   LCK_INIT_BAD_REL_CODE_T,
   LCK_INIT_BAD_MAJOR_VER_T,
   LCK_INIT_SUCCESS_T,
   LCK_INIT_SUCCESS_HDL2_T,
   LCK_INIT_NULL_CBKS_T,
   LCK_INIT_NULL_CBKS2_T,
   LCK_INIT_NULL_WT_CLBK_T,
   LCK_INIT_ERR_TRY_AGAIN_T,
   LCK_INIT_MAX_T
}LCK_INIT_TC_TYPE;

struct SafLckInitialize {
   SaLckHandleT *lckHandle;
   SaVersionT *version;
   SaLckCallbacksT *callbks;
   SaAisErrorT exp_output;
};

/* saLckSelectionObjectGet */

typedef enum {
   LCK_SEL_OBJ_BAD_HANDLE_T=1,
   LCK_SEL_OBJ_FINALIZED_HDL_T,
   LCK_SEL_OBJ_NULL_SEL_OBJ_T,
   LCK_SEL_OBJ_SUCCESS_T,
   LCK_SEL_OBJ_SUCCESS_HDL2_T,
   LCK_SEL_OBJ_ERR_TRY_AGAIN_T,
   LCK_SEL_OBJ_MAX_T
}LCK_SEL_OBJ_TC_TYPE;

struct SafSelectionObject {
   SaLckHandleT *lckHandle;
   SaSelectionObjectT *selobj; 
   SaAisErrorT exp_output;
};

/* saLckOptionCheck */

typedef enum {
   LCK_OPT_CHCK_BAD_HDL_T=1,
   LCK_OPT_CHCK_FINALIZED_HDL_T,
   LCK_OPT_CHCK_INVALID_PARAM,
   LCK_OPT_CHCK_SUCCESS_T,
   LCK_OPT_CHCK_MAX_T
}LCK_OPT_CHCK_TC_TYPE;

struct SafOptionCheck {
   SaLckHandleT *lckHandle;
   SaLckOptionsT *lckOptions; 
   SaAisErrorT exp_output;
};

/* saLckDispatch */

typedef enum {
   LCK_DISPATCH_ONE_BAD_HANDLE_T=1,
   LCK_DISPATCH_ONE_FINALIZED_HDL_T,
   LCK_DISPATCH_ALL_BAD_HANDLE_T,
   LCK_DISPATCH_ALL_FINALIZED_HDL_T,
   LCK_DISPATCH_BLOCKING_BAD_HANDLE_T,
   LCK_DISPATCH_BLOCKING_FINALIZED_HDL_T,
   LCK_DISPATCH_BAD_FLAGS_T,
   LCK_DISPATCH_DISPATCH_ONE_SUCCESS_T,
   LCK_DISPATCH_DISPATCH_ONE_SUCCESS2_T,
   LCK_DISPATCH_DISPATCH_ALL_SUCCESS_T,
   LCK_DISPATCH_DISPATCH_ALL_SUCCESS2_T,
   LCK_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T,
   LCK_DISPATCH_ERR_TRY_AGAIN_T,
   LCK_DISPATCH_MAX_T
}LCK_DISPATCH_TC_TYPE;

struct SafDispatch {
   SaLckHandleT *lckHandle;
   SaDispatchFlagsT flags;
   SaAisErrorT exp_output;
};

/* saLckFinalize */

typedef enum {
   LCK_FINALIZE_BAD_HDL_T=1,
   LCK_FINALIZE_SUCCESS_T,
   LCK_FINALIZE_SUCCESS_HDL2_T,
   LCK_FINALIZE_SUCCESS_HDL3_T,
   LCK_FINALIZE_FINALIZED_HDL_T,
   LCK_FINALIZE_ERR_TRY_AGAIN_T,
   CK_FINALIZE_MAX_T
}LCK_FINALIZE_TC_TYPE;

struct SafFinalize {
   SaLckHandleT *lckHandle;
   SaAisErrorT exp_output;
};

/* saLckResourceOpen */

typedef enum {
   LCK_RESOURCE_OPEN_BAD_HANDLE_T=1,
   LCK_RESOURCE_OPEN_FINALIZED_HANDLE_T,
   LCK_RESOURCE_OPEN_NULL_RSC_NAME_T,
   LCK_RESOURCE_OPEN_NULL_RSC_HDL_T,
   LCK_RESOURCE_OPEN_BAD_FLAGS_T,
   LCK_RESOURCE_OPEN_TIMEOUT_T,
   LCK_RESOURCE_OPEN_RSC_NOT_EXIST_T,
   LCK_RESOURCE_OPEN_RSC_NOT_EXIST2_T,
   LCK_RESOURCE_OPEN_HDL1_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_HDL1_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_HDL1_NAME3_SUCCESS_T,
   LCK_RESOURCE_OPEN_HDL2_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_HDL2_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_NAME1_EXIST_SUCCESS_T,
   LCK_RESOURCE_OPEN_NAME2_EXIST_SUCCESS_T,
   LCK_RESOURCE_OPEN_ERR_TRY_AGAIN_T,
   LCK_RESOURCE_OPEN_MAX_T
}LCK_RESOURCE_OPEN_TC_TYPE;

struct SafResourceOpen {
   SaLckHandleT *lckHandle;
   SaNameT *lockResourceName;
   SaTimeT timeout;
   SaLckResourceHandleT *lockResourceHandle;
   SaLckResourceOpenFlagsT resourceFlags;
   SaAisErrorT exp_output;
};

/* saLckResourceOpenAsync */

typedef enum {
   LCK_RESOURCE_OPEN_ASYNC_BAD_HANDLE_T=1,
   LCK_RESOURCE_OPEN_ASYNC_FINALIZED_HDL_T,
   LCK_RESOURCE_OPEN_ASYNC_NULL_RSC_NAME_T,
   LCK_RESOURCE_OPEN_ASYNC_BAD_FLAGS_T,
   LCK_RESOURCE_OPEN_ASYNC_NULL_OPEN_CBK_T,
   LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST_T,
   LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST2_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME2_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_EXIST_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_EXIST_SUCCESS_T,
   LCK_RESOURCE_OPEN_ASYNC_NAME1_EXIST_T,
   LCK_RESOURCE_OPEN_ASYNC_NAME2_EXIST_T,
   LCK_RESOURCE_OPEN_ASYNC_ERR_TRY_AGAIN_T,
   LCK_RESOURCE_OPEN_ASYNC_MAX_T
}LCK_RESOURCE_OPEN_ASYNC_TC_TYPE;

struct SafAsyncResourceOpen {
   SaLckHandleT *lckHandle;
   SaInvocationT Invocation;
   SaNameT *lockResourceName;
   SaLckResourceOpenFlagsT resourceFlags;
   SaAisErrorT exp_output; 
};

/* saLckResourceClose */

typedef enum {
   LCK_RESOURCE_CLOSE_BAD_RSC_HDL_T=1,
   LCK_RESOURCE_CLOSE_BAD_HANDLE_T,
   LCK_RESOURCE_CLOSE_BAD_HANDLE2_T,
   LCK_RESOURCE_CLOSE_RSC_HDL1_SUCCESS_T,
   LCK_RESOURCE_CLOSE_RSC_HDL2_SUCCESS_T,
   LCK_RESOURCE_CLOSE_ERR_TRY_AGAIN_T,
   LCK_RESOURCE_CLOSE_MAX_T
}LCK_RESOURCE_CLOSE_TC_TYPE;

struct SafResourceClose {
   SaLckResourceHandleT *lockResourceHandle;
   SaAisErrorT exp_output;
};

/* saLckResourceLock */

typedef enum {
   LCK_RSC_LOCK_BAD_RSC_HDL_T=1,
   LCK_RSC_LOCK_BAD_HDL_T,
   LCK_RSC_LOCK_CLOSED_RSC_HDL_T,
   LCK_RSC_LOCK_FINALIZED_HDL_T,
   LCK_RSC_LOCK_NULL_LCKID_T,
   LCK_RSC_LOCK_NULL_LCK_STATUS_T,
   LCK_RSC_LOCK_INVALID_LOCK_MODE_T,
   LCK_RSC_LOCK_BAD_FLGS_T,
   LCK_RSC_LOCK_ZERO_TIMEOUT_T,
   LCK_RSC_LOCK_PR_ERR_TIMEOUT_T,
   LCK_RSC_LOCK_EX_ERR_TIMEOUT_T,
   LCK_RSC_LOCK_PR_LOCK_SUCCESS_T,
   LCK_RSC_LOCK_EX_LOCK_SUCCESS_T,
   LCK_RSC_LOCK_PR_LOCK_RSC2_SUCCESS_T,
   LCK_RSC_LOCK_EX_LOCK_RSC2_SUCCESS_T,
   LCK_RSC_LOCK_EX_LOCK_RSC3_SUCCESS_T,
   LCK_RSC_LOCK_DUPLICATE_EXLCK_T,
   LCK_RSC_LOCK_NO_QUEUE_PRLCK_T,
   LCK_RSC_LOCK_NO_QUEUE_EXLCK_T,
   LCK_RSC_LOCK_ORPHAN_PRLCK_T,
   LCK_RSC_LOCK_ORPHAN_EXLCK_T,
   LCK_RSC_LOCK_ORPHAN_EXLCK_RSC2_T,
   LCK_RSC_LOCK_PR_LOCK_NOT_QUEUED_T,
   LCK_RSC_LOCK_EX_LOCK_NOT_QUEUED_T,
   LCK_RSC_LOCK_PR_LOCK_DEADLOCK_T,
   LCK_RSC_LOCK_EX_LOCK_DEADLOCK_T,
   LCK_RSC_LOCK_PR_LOCK_RSC2_DEADLOCK_T,
   LCK_RSC_LOCK_EX_LOCK_RSC2_DEADLOCK_T,
   LCK_RSC_LOCK_EX_LOCK_ORPHANED_T,
   LCK_RSC_LOCK_PR_LOCK_ORPHANED_T,
   LCK_RSC_LOCK_EX_LOCK_RSC2_ORPHANED_T,
   LCK_RSC_LOCK_PR_LOCK_RSC2_ORPHANED_T,
   LCK_RSC_LOCK_EX_LOCK_ORPHAN_DDLCK_T,
   LCK_RSC_LOCK_ERR_TRY_AGAIN_T,
   LCK_RSC_LOCK_MAX_T
}LCK_RSC_LOCK_TC_TYPE;

struct SafResourceLock {
   SaLckResourceHandleT *lockResourceHandle;
   SaLckLockIdT  *lockId;
   SaLckLockModeT lockMode;
   SaLckLockFlagsT lockFlags;
   SaLckWaiterSignalT waiterSignal;
   SaTimeT timeout;
   SaLckLockStatusT *lockStatus;
   SaAisErrorT exp_output;
   SaAisErrorT exp_lck_status;
};

/* saLckResourceLockAsync */

typedef enum {
   LCK_RSC_LOCK_ASYNC_BAD_RSC_HDL_T=1,
   LCK_RSC_LOCK_ASYNC_CLOSED_RSC_HDL_T,
   LCK_RSC_LOCK_ASYNC_NULL_LCKID_T,
   LCK_RSC_LOCK_ASYNC_INVALID_LOCK_MODE_T,
   LCK_RSC_LOCK_ASYNC_BAD_FLGS_T,
   LCK_RSC_LOCK_ASYNC_ERR_INIT_T,
   LCK_RSC_LOCK_ASYNC_PRLCK_SUCCESS_T,
   LCK_RSC_LOCK_ASYNC_EXLCK_SUCCESS_T,
   LCK_RSC_LOCK_ASYNC_PRLCK_RSC2_SUCCESS_T,
   LCK_RSC_LOCK_ASYNC_EXLCK_RSC2_SUCCESS_T,
   LCK_RSC_LOCK_ASYNC_NO_QUEUE_PRLCK_T,
   LCK_RSC_LOCK_ASYNC_NO_QUEUE_EXLCK_T,
   LCK_RSC_LOCK_ASYNC_ORPHAN_PRLCK_T,
   LCK_RSC_LOCK_ASYNC_ORPHAN_EXLCK_T,
   LCK_RSC_LOCK_ASYNC_FINALIZED_HDL_T,
   LCK_RSC_LOCK_ASYNC_ERR_TRY_AGAIN_T,
   LCK_RSC_LOCK_ASYNC_MAX_T
}LCK_RSC_LOCK_ASYNC_TC_TYPE;

struct SafAsyncResourceLock {
   SaLckResourceHandleT *lockResourceHandle;
   SaInvocationT invocation;
   SaLckLockIdT  *lockId;
   SaLckLockModeT lockMode;
   SaLckLockFlagsT lockFlags;
   SaLckWaiterSignalT waiterSignal;
   SaAisErrorT exp_output;
};

/* saLckResourceUnlockAsync */

typedef enum {
   LCK_RSC_UNLOCK_ASYNC_BAD_LOCKID_T=1,
   LCK_RSC_UNLOCK_ASYNC_UNLOCKED_LOCKID_T,
   LCK_RSC_UNLOCK_ASYNC_BAD_HDL_T,
   LCK_RSC_UNLOCK_ASYNC_BAD_HDL2_T,
   LCK_RSC_UNLOCK_ASYNC_SUCCESS_T,
   LCK_RSC_UNLOCK_ASYNC_SUCCESS_ID2_T,
   LCK_RSC_UNLOCK_ASYNC_ERR_INIT_T,
   LCK_RSC_UNLOCK_ASYNC_ERR_TRY_AGAIN_T,
   LCK_RSC_UNLOCK_ASYNC_MAX_T,
   LCK_RSC_FINALIZED_ASYNC_UNLOCKED_LOCKID_T
}LCK_RSC_UNLOCK_ASYNC_TC_TYPE;

struct SafAsyncResourceUnlock {
   SaInvocationT invocation;
   SaLckLockIdT *lockId;
   SaAisErrorT exp_output;
};

/* saLckResourceUnlock */

typedef enum {
   LCK_RSC_UNLOCK_BAD_LOCKID_T=1,
   LCK_RSC_UNLOCK_UNLOCKED_LOCKID_T,
   LCK_RSC_UNLOCK_BAD_HDL_T,
   LCK_RSC_UNLOCK_BAD_HDL2_T,
   LCK_RSC_UNLOCK_ERR_TIMEOUT_T,
   LCK_RSC_UNLOCK_LCKID1_SUCCESS_T,
   LCK_RSC_UNLOCK_LCKID2_SUCCESS_T,
   LCK_RSC_UNLOCK_ERR_TRY_AGAIN_T,
   LCK_RSC_UNLOCK_MAX_T,
   LCK_RSC_FINALIZED_UNLOCKED_LOCKID_T
}LCK_RSC_UNLOCK_TC_TYPE;

struct SafResourceUnlock {
   SaLckLockIdT *lockId;
   SaTimeT timeout;
   SaAisErrorT exp_output;
};

/* saLckLockPurge */

typedef enum {
   LCK_LOCK_PURGE_BAD_HDL_T=1,
   LCK_LOCK_PURGE_CLOSED_HDL_T,
   LCK_LOCK_PURGE_FINALIZED_HDL_T,
   LCK_LOCK_PURGE_SUCCESS_T,
   LCK_LOCK_PURGE_SUCCESS_RSC2_T,
   LCK_LOCK_PURGE_NO_ORPHAN_T,
   LCK_LOCK_PURGE_ERR_TRY_AGAIN_T,
   LCK_LOCK_PURGE_MAX_T
}LCK_LOCK_PURGE_TC_TYPE;

struct SafPurge {
   SaLckResourceHandleT *lockResourceHandle;
   SaAisErrorT exp_output;
};

/* Tetlist Array Indices */

typedef enum {
   GLSV_ONE_NODE_LIST=1,
}GLSV_TEST_LIST_INDEX;

/* GLSv Redundancy flag */

typedef enum {
   GLND_RESTART=1,
   GL_SWITCHOVER,
   GL_FAILOVER,
   GL_MANUAL
}GLSV_RED_FLAG;

/* GLSv Instance Structure */

typedef struct tet_glsv_inst {
   int inst_num;
   int tetlist_index;
   int test_case_num;
   int num_of_iter;
   int node_id;
   SaNameT res_name1;
   SaNameT res_name2;
   SaNameT res_name3;
}TET_GLSV_INST;

extern const char *saf_lck_status_string[];
extern const char *saf_lck_mode_string[];
extern const char *saf_lck_flags_string[];
extern const char *glsv_saf_error_string[];

extern void glsv_fill_lck_version(SaVersionT *version,SaUint8T rel_code,SaUint8T mjr_ver,SaUint8T mnr_ver);

 void glsv_it_init_01(void);
 void glsv_it_init_02(void);
 void glsv_it_init_03(void);
 void glsv_it_init_04(void);
 void glsv_it_init_05(void);
 void glsv_it_init_06(void);
 void glsv_it_init_07(void);
 void glsv_it_init_08(void);
 void glsv_it_init_09(void);
 void glsv_it_init_10(void);
 void glsv_it_selobj_01(void);
 void glsv_it_selobj_02(void);
 void glsv_it_selobj_03(void);
 void glsv_it_selobj_04(void);
 void glsv_it_selobj_05(void);
 void glsv_it_option_chk_01(void);
 void glsv_it_option_chk_02(void);
 void glsv_it_option_chk_03(void);
 void glsv_it_dispatch_01(void);
 void glsv_it_dispatch_02(void);
 void glsv_it_dispatch_03(void);
 void glsv_it_dispatch_04(void);
 void glsv_it_dispatch_05(void);
 void glsv_it_dispatch_06(void);
 void glsv_it_dispatch_07(void);
 void glsv_it_dispatch_08(void);
 void glsv_it_dispatch_09(void);
 void glsv_it_finalize_01(void);
 void glsv_it_finalize_02(void);
 void glsv_it_finalize_03(void);
 void glsv_it_finalize_04(void);
 void glsv_it_finalize_05(void);
 void glsv_it_finalize_06(void);
 void glsv_it_res_open_01(void);
 void glsv_it_res_open_02(void);
 void glsv_it_res_open_03(void);
 void glsv_it_res_open_04(void);
 void glsv_it_res_open_05(void);
 void glsv_it_res_open_06(void);
 void glsv_it_res_open_07(void);
 void glsv_it_res_open_08(void);
 void glsv_it_res_open_09(void);
 void glsv_it_res_open_10(void);
 void glsv_it_res_open_async_01(void);
 void glsv_it_res_open_async_02(void);
 void glsv_it_res_open_async_03(void);
 void glsv_it_res_open_async_04(void);
 void glsv_it_res_open_async_05(void);
 void glsv_it_res_open_async_06(void);
 void glsv_it_res_open_async_07(void);
 void glsv_it_res_open_async_08(void);
 void glsv_it_res_open_async_09(void);
 void glsv_it_res_open_async_10(void);
 void glsv_it_res_close_01(void);
 void glsv_it_res_close_02(void);
 void glsv_it_res_close_03(void);
 void glsv_it_res_close_04(void);
 void glsv_it_res_close_05(void);
 void glsv_it_res_close_06(void);
 void glsv_it_res_close_07(void);
 void glsv_it_res_close_08(void);
 void glsv_it_res_close_09(void);
 void glsv_it_res_close_10(void);
 void glsv_it_res_close_11(void);
 void glsv_it_res_lck_01(void);
 void glsv_it_res_lck_02(void);
 void glsv_it_res_lck_03(void);
 void glsv_it_res_lck_04(void);
 void glsv_it_res_lck_05(void);
 void glsv_it_res_lck_06(void);
 void glsv_it_res_lck_07(void);
 void glsv_it_res_lck_08(void);
 void glsv_it_res_lck_09(void);
 void glsv_it_res_lck_10(void);
 void glsv_it_res_lck_11(void);
 void glsv_it_res_lck_12(void);
 void glsv_it_res_lck_13(void);
 void glsv_it_res_lck_14(void);
 void glsv_it_res_lck_15(void);
 void glsv_it_res_lck_16(void);
 void glsv_it_res_lck_17(void);
 void glsv_it_res_lck_18(void);
 void glsv_it_res_lck_async_01(void);
 void glsv_it_res_lck_async_02(void);
 void glsv_it_res_lck_async_03(void);
 void glsv_it_res_lck_async_04(void);
 void glsv_it_res_lck_async_05(void);
 void glsv_it_res_lck_async_06(void);
 void glsv_it_res_lck_async_07(void);
 void glsv_it_res_lck_async_08(void);
 void glsv_it_res_lck_async_09(void);
 void glsv_it_res_lck_async_10(void);
 void glsv_it_res_lck_async_11(void);
 void glsv_it_res_lck_async_12(void);
 void glsv_it_res_lck_async_13(void);
 void glsv_it_res_lck_async_14(void);
 void glsv_it_res_lck_async_15(void);
 void glsv_it_res_lck_async_16(void);
 void glsv_it_res_lck_async_17(void);
 void glsv_it_res_lck_async_18(void);
 void glsv_it_res_lck_async_19(void);
 void glsv_it_res_lck_async_20(void);
 void glsv_it_res_unlck_01(void);
 void glsv_it_res_unlck_02(void);
 void glsv_it_res_unlck_03(void);
 void glsv_it_res_unlck_04(void);
 void glsv_it_res_unlck_05(void);
 void glsv_it_res_unlck_06(void);
 void glsv_it_res_unlck_07(void);
 void glsv_it_res_unlck_08(void);
 void glsv_it_res_unlck_async_01(void);
 void glsv_it_res_unlck_async_02(void);
 void glsv_it_res_unlck_async_03(void);
 void glsv_it_res_unlck_async_04(void);
 void glsv_it_res_unlck_async_05(void);
 void glsv_it_res_unlck_async_06(void);
 void glsv_it_res_unlck_async_07(void);
 void glsv_it_res_unlck_async_08(void);
 void glsv_it_res_unlck_async_09(void);
 void glsv_it_res_unlck_async_10(void);
 void glsv_it_lck_purge_01(void);
 void glsv_it_lck_purge_02(void);
 void glsv_it_lck_purge_03(void);
 void glsv_it_lck_purge_04(void);
 void glsv_it_lck_purge_05(void);
 void glsv_it_res_cr_del_01(void);
 void glsv_it_res_cr_del_02(void);
 void glsv_it_res_cr_del_03(void);
 void glsv_it_res_cr_del_04(void);
 void glsv_it_res_cr_del_05(void);
 void glsv_it_res_cr_del_06(int async);
 void glsv_it_lck_modes_wt_clbk_01(int async);
 void glsv_it_lck_modes_wt_clbk_02(int async);
 void glsv_it_lck_modes_wt_clbk_03(int async);
 void glsv_it_lck_modes_wt_clbk_04(int async);
 void glsv_it_lck_modes_wt_clbk_05(void);
 void glsv_it_lck_modes_wt_clbk_06(void);
 void glsv_it_lck_modes_wt_clbk_07(void);
 void glsv_it_lck_modes_wt_clbk_08(void);
 void glsv_it_lck_modes_wt_clbk_09(void);
 void glsv_it_ddlcks_orplks_01(void);
 void glsv_it_ddlcks_orplks_02(int async);
 void glsv_it_ddlcks_orplks_03(int async);
 void glsv_it_ddlcks_orplks_04(int async);
 void glsv_it_ddlcks_orplks_05(void);
 void glsv_it_ddlcks_orplks_06(void);
 void glsv_it_ddlcks_orplks_07(int async);
 void glsv_it_lck_strip_purge_01(void);
 void glsv_it_lck_strip_purge_02(void);
 void glsv_it_lck_strip_purge_03(void);
 void glsv_it_err_try_again_01(void);
 void glsv_it_err_try_again_02(void);
 void glsv_it_err_try_again_03(void);
 void glsv_it_err_try_again_04(void);

void App_ResourceOpenCallback(SaInvocationT invocation,
                              SaLckResourceHandleT res_hdl,
                              SaAisErrorT error);
void App_LockGrantCallback(SaInvocationT invocation,
                           SaLckLockStatusT lockStatus,
                           SaAisErrorT error);
void App_LockGrantCallback_withunlock_lock(SaInvocationT invocation,
                            SaLckLockStatusT lockStatus,
                            SaAisErrorT error);
void App_LockWaiterCallback(SaLckWaiterSignalT waiterSignal,
                            SaLckLockIdT lockId,
                            SaLckLockModeT modeHeld,
                            SaLckLockModeT modeRequested);

void App_ResourceUnlockCallback(SaInvocationT invocation,
                                SaAisErrorT error);
void glsv_fill_lck_clbks(SaLckCallbacksT *clbk,SaLckResourceOpenCallbackT opn_clbk,SaLckLockGrantCallbackT gr_clbk,
                         SaLckLockWaiterCallbackT wt_clbk,SaLckResourceUnlockCallbackT unlck_clbk);
void glsv_fill_res_names(SaNameT *name,char *string,char *inst_num_char);
void init_glsv_test_env(void);
void glsv_print_testcase(char *string);
void glsv_result(int result);

void tet_glsv_get_inputs(TET_GLSV_INST *inst);
void tet_glsv_fill_inputs(TET_GLSV_INST *inst);
void tet_glsv_start_instance(TET_GLSV_INST *inst);
void tet_run_glsv_app(void);
void tet_run_glsv_dist_cases(void);
void print_res_info(SaNameT *res_name,SaLckResourceOpenFlagsT flgs);
void print_lock_info(SaLckResourceHandleT *res_hdl,SaLckLockModeT lck_mode,SaLckLockFlagsT lck_flags,
                     SaLckWaiterSignalT waiter_sig);
void glsv_selection_thread (NCSCONTEXT arg);
void glsv_selection_thread_one (NCSCONTEXT arg);
void glsv_createthread_one(SaLckHandleT *lck_Handle);
void glsv_selection_thread_all (NCSCONTEXT arg);
void glsv_createthread_all(SaLckHandleT *lck_Handle);
void glsv_selection_thread_all_loop (NCSCONTEXT arg);
void glsv_selection_thread_all_loop_hdl2 (NCSCONTEXT arg);
void glsv_clean_output_params(void);
void glsv_clean_clbk_params(void);

#endif  // LCK_APITEST_TET_GLSV_H_
