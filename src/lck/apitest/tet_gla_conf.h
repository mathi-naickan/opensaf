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

#ifndef LCK_APITEST_TET_GLA_CONF_H_
#define LCK_APITEST_TET_GLA_CONF_H_

typedef struct glsv_inv_params
{
   SaLckHandleT inv_lck_hdl;
   SaVersionT inv_version;
   SaVersionT inv_ver_bad_rel_code;
   SaVersionT inv_ver_not_supp;
   SaLckResourceHandleT inv_res_hdl;
   SaLckLockIdT inv_lck_id;
}GLSV_INV_PARAMS;

typedef struct gla_test_env
{
   GLSV_INV_PARAMS inv_params;

   SaLckHandleT lck_hdl1;
   SaLckHandleT lck_hdl2;
   SaLckHandleT null_clbks_lck_hdl;

   SaLckCallbacksT gen_clbks;
   SaLckCallbacksT null_clbks;
   SaLckCallbacksT null_wt_clbks;

   SaVersionT version;

   SaSelectionObjectT sel_obj;

   SaLckOptionsT options;

   SaNameT res1;
   SaNameT res2;
   SaNameT res3;

   SaLckResourceHandleT res_hdl1;
   SaLckResourceHandleT res_hdl2;
   SaLckResourceHandleT res_hdl3;

   SaLckLockIdT ex_lck_id;
   SaLckLockIdT pr_lck_id;

   SaLckLockStatusT lck_status;

   /* Callback Info */

   SaInvocationT open_clbk_invo;
   SaLckResourceHandleT open_clbk_res_hdl;
   SaAisErrorT open_clbk_err;

   SaInvocationT gr_clbk_invo;
   SaLckLockStatusT gr_clbk_status;
   SaAisErrorT gr_clbk_err;

   SaLckWaiterSignalT waiter_sig;
   SaLckLockIdT waiter_clbk_lck_id;
   SaLckLockModeT waiter_clbk_mode_held;
   SaLckLockModeT waiter_clbk_mode_req;

   SaInvocationT unlck_clbk_invo;
   SaAisErrorT unlck_clbk_err;

}GLA_TEST_ENV;

extern GLA_TEST_ENV gl_gla_env;

typedef enum
{
   TEST_NONCONFIG_MODE = 0,
   TEST_CONFIG_MODE,
   TEST_CLEANUP_MODE,
   TEST_USER_DEFINED_MODE
}GLSV_CONFIG_FLAG;

typedef enum
{
   LCK_RESTORE_INIT_BAD_VERSION_T = 0,
   LCK_RESTORE_INIT_BAD_REL_CODE_T,
   LCK_RESTORE_INIT_BAD_MAJOR_VER_T
}GLSV_RESTORE_OPT;

typedef enum
{
   LCK_CLEAN_INIT_NULL_CBK_PARAM_T = 0,
   LCK_CLEAN_INIT_NULL_CBKS2_T,
   LCK_CLEAN_INIT_SUCCESS_T,
   LCK_CLEAN_INIT_SUCCESS_HDL2_T,
   LCK_CLEAN_INIT_NULL_CBKS_T
}GLSV_INIT_CLEANUP_OPT;

typedef enum
{
   LCK_CLEAN_RSC_LOCK_ORPHAN_PRLCK_T = 0,
   LCK_CLEAN_RSC_LOCK_ORPHAN_EXLCK_T,
   LCK_PURGE_RSC1_T
}GLSV_ORPHAN_CLEANUP_OPT;


/* GLSV thread creation functions */

extern void glsv_createthread(SaLckHandleT *lck_Handle);
extern void glsv_createthread_all_loop(int hdl);


extern int tet_test_lckInitialize(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckSelectionObject(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckOptionCheck(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckDispatch(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckFinalize(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceOpen(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceOpenAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceClose(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceLock(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceLockAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceUnlock(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckResourceUnlockAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_lckLockPurge(int i,GLSV_CONFIG_FLAG flg);

extern int tet_test_red_lckInitialize(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckSelectionObject(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckOptionCheck(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckDispatch(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckFinalize(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceOpen(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceOpenAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceClose(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceLock(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceLockAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceUnlock(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckResourceUnlockAsync(int i,GLSV_CONFIG_FLAG flg);
extern int tet_test_red_lckLockPurge(int i,GLSV_CONFIG_FLAG flg);

/* Cleanup Functions */

extern void glsv_clean_output_params();
extern void glsv_clean_clbk_params();
extern void glsv_init_cleanup(GLSV_INIT_CLEANUP_OPT opt);
extern void glsv_orphan_cleanup(GLSV_ORPHAN_CLEANUP_OPT opt);
int glsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,GLSV_CONFIG_FLAG flg);
void glsv_init_red_cleanup(GLSV_INIT_CLEANUP_OPT opt);
void glsv_orphan_red_cleanup(GLSV_ORPHAN_CLEANUP_OPT opt);

/* Restore Function */

extern void glsv_restore_params(GLSV_RESTORE_OPT opt);

#endif  // LCK_APITEST_TET_GLA_CONF_H_

