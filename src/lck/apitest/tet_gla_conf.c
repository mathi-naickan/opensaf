#if (TET_A == 1)

#include "tet_glsv.h"
#include "tet_gla_conf.h"

#define END_OF_WHILE while((rc == SA_AIS_ERR_TRY_AGAIN) || \
                           (rc == SA_AIS_ERR_TIMEOUT && exp_output != SA_AIS_ERR_TIMEOUT))

#define GLSV_RETRY_WAIT sleep(1)

static int gl_lck_thrd_cnt;
static int gl_try_again_cnt;

int glsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,GLSV_CONFIG_FLAG flg)
{
   int result = 0;

   if(rc == SA_AIS_ERR_TRY_AGAIN)
   {
      if(gl_try_again_cnt++ == 0)
      {
         m_TET_GLSV_PRINTF("\n RETRY           : %s\n",test_case);
         m_TET_GLSV_PRINTF(" Return Value    : %s\n\n",glsv_saf_error_string[rc]);
      }

      if(!(gl_try_again_cnt%10))
         m_TET_GLSV_PRINTF(" Try again count : %d \n",gl_try_again_cnt);

      GLSV_RETRY_WAIT;

      return(result);
   }

   if(rc == exp_out)
   {
      if(flg != TEST_CLEANUP_MODE)
         m_TET_GLSV_PRINTF("\n SUCCESS         : %s\n",test_case);
      result = TET_PASS;
   }
   else
   {
      m_TET_GLSV_PRINTF("\n FAILED          : %s\n",test_case);
      if(flg == TEST_CLEANUP_MODE)
         tet_printf("\n FAILED          : %s\n",test_case);
      result = TET_FAIL;
      if(flg == TEST_CONFIG_MODE)
         result = TET_UNRESOLVED;
   }

   if(flg != TEST_CLEANUP_MODE || (flg == TEST_CLEANUP_MODE && result != TET_PASS))
   {
      m_TET_GLSV_PRINTF(" Return Value    : %s\n",glsv_saf_error_string[rc]);
      if(flg == TEST_CLEANUP_MODE)
         tet_printf(" Return Value    : %s\n",glsv_saf_error_string[rc]);
   }
   return(result);
}

void print_res_info(SaNameT *res_name,SaLckResourceOpenFlagsT flgs)
{
   if(res_name)
      m_TET_GLSV_PRINTF("\n Resource Name   : %s\n",res_name->value);
   else
      m_TET_GLSV_PRINTF("\n Resource Name   : NULL\n");

   if(flgs>= 0 && flgs< 2)
   {
      if(flgs == 0)
         m_TET_GLSV_PRINTF(" open flags      : ZERO");
      if(flgs == 1)
         m_TET_GLSV_PRINTF(" open flags      : SA_LCK_RESOURCE_CREATE");
   }
   else
      m_TET_GLSV_PRINTF(" open flags      : %u",flgs);
}

void print_lock_info(SaLckResourceHandleT *res_hdl,SaLckLockModeT lck_mode,SaLckLockFlagsT lck_flags,
                     SaLckWaiterSignalT waiter_sig)
{
   if(res_hdl)
      m_TET_GLSV_PRINTF("\n Resource Handle : %llu\n",*res_hdl);
   else
      m_TET_GLSV_PRINTF("\n Resource Handle : NULL\n");

   if(lck_mode < 3 && lck_mode >= 0)
      m_TET_GLSV_PRINTF(" Lock Mode       : %s\n",saf_lck_mode_string[lck_mode]);
   else
      m_TET_GLSV_PRINTF(" Lock Mode       : %u\n",lck_mode);

   if(lck_flags< 3)
      m_TET_GLSV_PRINTF(" Lock Flags      : %s\n",saf_lck_flags_string[lck_flags]);
   else
      m_TET_GLSV_PRINTF(" Lock Flags      : %u\n",lck_flags);

   m_TET_GLSV_PRINTF(" Waiter Signal   : %llu",waiter_sig);
}

/* ***************  Lock Initialization Test cases  ***************** */

char *API_Glsv_Initialize_resultstring[] = {
   [LCK_INIT_NULL_HANDLE_T]         = "saLckInitialize with Null handle",
   [LCK_INIT_NULL_VERSION_T]        = "saLckInitialize with Null version and callbacks",
   [LCK_INIT_NULL_PARAMS_T]         = "saLckInitialize with all null parameters",
   [LCK_INIT_NULL_CBK_PARAM_T]      = "saLckInitialize with Null callback parameter",
   [LCK_INIT_NULL_VERSION_CBKS_T]   = "saLckInitialize with Null version and callbacks",
   [LCK_INIT_BAD_VERSION_T]         = "saLckInitialize with invalid version",
   [LCK_INIT_BAD_REL_CODE_T]        = "saLckInitialize with version with invalid release code",
   [LCK_INIT_BAD_MAJOR_VER_T]       = "saLckInitialize with version with invalid major version",
   [LCK_INIT_SUCCESS_T]             = "saLckInitialize with valid parameters",
   [LCK_INIT_SUCCESS_HDL2_T]        = "saLckInitialize with valid parameters",
   [LCK_INIT_NULL_CBKS_T]           = "saLckInitialize with Null callbacks",
   [LCK_INIT_NULL_CBKS2_T]          = "saLckInitialize with Null callbacks",
   [LCK_INIT_NULL_WT_CLBK_T]        = "saLckInitialize with Null waiter callback",
   [LCK_INIT_ERR_TRY_AGAIN_T]       = "saLckInitialize when service is not available",
};

struct SafLckInitialize API_Glsv_Initialize[] = {
   [LCK_INIT_NULL_HANDLE_T]         = {NULL,&gl_gla_env.version,&gl_gla_env.gen_clbks,SA_AIS_ERR_INVALID_PARAM},
   [LCK_INIT_NULL_VERSION_T]        = {&gl_gla_env.lck_hdl1,NULL,&gl_gla_env.gen_clbks,SA_AIS_ERR_INVALID_PARAM},
   [LCK_INIT_NULL_PARAMS_T]         = {NULL,NULL,NULL,SA_AIS_ERR_INVALID_PARAM},
   [LCK_INIT_NULL_CBK_PARAM_T]      = {&gl_gla_env.lck_hdl1,&gl_gla_env.version,NULL,SA_AIS_OK},
   [LCK_INIT_NULL_VERSION_CBKS_T]   = {&gl_gla_env.lck_hdl1,NULL,NULL,SA_AIS_ERR_INVALID_PARAM},
   [LCK_INIT_BAD_VERSION_T]         = {&gl_gla_env.lck_hdl1,&gl_gla_env.inv_params.inv_version,
                                       &gl_gla_env.gen_clbks,SA_AIS_ERR_VERSION},
   [LCK_INIT_BAD_REL_CODE_T]        = {&gl_gla_env.lck_hdl1,&gl_gla_env.inv_params.inv_ver_bad_rel_code,
                                       &gl_gla_env.gen_clbks,SA_AIS_ERR_VERSION},
   [LCK_INIT_BAD_MAJOR_VER_T]       = {&gl_gla_env.lck_hdl1,&gl_gla_env.inv_params.inv_ver_not_supp,
                                       &gl_gla_env.gen_clbks,SA_AIS_ERR_VERSION},
   [LCK_INIT_SUCCESS_T]             = {&gl_gla_env.lck_hdl1,&gl_gla_env.version,&gl_gla_env.gen_clbks,SA_AIS_OK},
   [LCK_INIT_SUCCESS_HDL2_T]        = {&gl_gla_env.lck_hdl2,&gl_gla_env.version,&gl_gla_env.gen_clbks,SA_AIS_OK},
   [LCK_INIT_NULL_CBKS_T]           = {&gl_gla_env.null_clbks_lck_hdl,&gl_gla_env.version,&gl_gla_env.null_clbks,SA_AIS_OK},
   [LCK_INIT_NULL_CBKS2_T]          = {&gl_gla_env.lck_hdl1,&gl_gla_env.version,&gl_gla_env.null_clbks,SA_AIS_OK},
   [LCK_INIT_NULL_WT_CLBK_T]        = {&gl_gla_env.lck_hdl1,&gl_gla_env.version,&gl_gla_env.null_wt_clbks,SA_AIS_OK},
   [LCK_INIT_ERR_TRY_AGAIN_T]       = {&gl_gla_env.lck_hdl1,&gl_gla_env.version,&gl_gla_env.gen_clbks,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckInitialize(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckInitialize(API_Glsv_Initialize[i].lckHandle,API_Glsv_Initialize[i].callbks,
                        API_Glsv_Initialize[i].version);

   result = glsv_test_result(rc,API_Glsv_Initialize[i].exp_output,
                             API_Glsv_Initialize_resultstring[i],flg);

   if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF(" LckHandle       : %llu\n", *API_Glsv_Initialize[i].lckHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckInitialize(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Initialize[i].exp_output;

   do
   {
      rc = saLckInitialize(API_Glsv_Initialize[i].lckHandle,API_Glsv_Initialize[i].callbks,
                           API_Glsv_Initialize[i].version);

      result = glsv_test_result(rc,API_Glsv_Initialize[i].exp_output,
                                API_Glsv_Initialize_resultstring[i],flg);

      if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
         m_TET_GLSV_PRINTF(" LckHandle       : %llu\n", *API_Glsv_Initialize[i].lckHandle);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Selection Object Test cases  ***************** */

char *API_Glsv_Selection_resultstring[] = {
   [LCK_SEL_OBJ_BAD_HANDLE_T]    = "saLckSelectionObjGet with invalid lock handle",
   [LCK_SEL_OBJ_FINALIZED_HDL_T] = "saLckSelectionObjGet with finalized lock handle",
   [LCK_SEL_OBJ_NULL_SEL_OBJ_T]  = "saLckSelectionObjGet with NULL selection object parameter",
   [LCK_SEL_OBJ_SUCCESS_T]       = "saLckSelectionObjGet with valid parameters",
   [LCK_SEL_OBJ_SUCCESS_HDL2_T]  = "saLckSelectionObjGet with valid parameters",
   [LCK_SEL_OBJ_ERR_TRY_AGAIN_T] = "saLckSelectionObjGet when service is not available",
};
 
struct SafSelectionObject API_Glsv_Selection[] = {
   [LCK_SEL_OBJ_BAD_HANDLE_T]    = {&gl_gla_env.inv_params.inv_lck_hdl,&gl_gla_env.sel_obj,SA_AIS_ERR_BAD_HANDLE},
   [LCK_SEL_OBJ_FINALIZED_HDL_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.sel_obj,SA_AIS_ERR_BAD_HANDLE},
   [LCK_SEL_OBJ_NULL_SEL_OBJ_T]  = {&gl_gla_env.lck_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [LCK_SEL_OBJ_SUCCESS_T]       = {&gl_gla_env.lck_hdl1,&gl_gla_env.sel_obj,SA_AIS_OK},
   [LCK_SEL_OBJ_SUCCESS_HDL2_T]  = {&gl_gla_env.lck_hdl2,&gl_gla_env.sel_obj,SA_AIS_OK},
   [LCK_SEL_OBJ_ERR_TRY_AGAIN_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.sel_obj,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckSelectionObject(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckSelectionObjectGet(*API_Glsv_Selection[i].lckHandle,API_Glsv_Selection[i].selobj);

   result = glsv_test_result(rc,API_Glsv_Selection[i].exp_output,
                             API_Glsv_Selection_resultstring[i],flg);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckSelectionObject(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Selection[i].exp_output;

   do
   {
      rc = saLckSelectionObjectGet(*API_Glsv_Selection[i].lckHandle,API_Glsv_Selection[i].selobj);

      result = glsv_test_result(rc,API_Glsv_Selection[i].exp_output,
                                API_Glsv_Selection_resultstring[i],flg);

      m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Option Check Test cases  ***************** */

char *API_Glsv_Options_resultstring[] = {
   [LCK_OPT_CHCK_BAD_HDL_T]       = "saLckOptionCheck with invalid lock handle", 
   [LCK_OPT_CHCK_FINALIZED_HDL_T] = "saLckOptionCheck with finalized lock handle", 
   [LCK_OPT_CHCK_INVALID_PARAM]   = "saLckOptionCheck with NULL options parameter",
   [LCK_OPT_CHCK_SUCCESS_T]       = "saLckOptionCheck with all valid parameters",
};

struct SafOptionCheck API_Glsv_Options[] = {
   [LCK_OPT_CHCK_BAD_HDL_T]       = {&gl_gla_env.inv_params.inv_lck_hdl,&gl_gla_env.options,SA_AIS_ERR_BAD_HANDLE}, 
   [LCK_OPT_CHCK_FINALIZED_HDL_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.options,SA_AIS_ERR_BAD_HANDLE}, 
   [LCK_OPT_CHCK_INVALID_PARAM]   = {&gl_gla_env.lck_hdl1,NULL,SA_AIS_ERR_INVALID_PARAM},
   [LCK_OPT_CHCK_SUCCESS_T]       = {&gl_gla_env.lck_hdl1,&gl_gla_env.options,SA_AIS_OK},
};

int tet_test_lckOptionCheck(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckOptionCheck(*API_Glsv_Options[i].lckHandle,API_Glsv_Options[i].lckOptions);

   result = glsv_test_result(rc,API_Glsv_Options[i].exp_output,
                             API_Glsv_Options_resultstring[i],flg);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckOptionCheck(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Options[i].exp_output;

   do
   {
      rc = saLckOptionCheck(*API_Glsv_Options[i].lckHandle,API_Glsv_Options[i].lckOptions);

      result = glsv_test_result(rc,API_Glsv_Options[i].exp_output,
                                API_Glsv_Options_resultstring[i],flg);

      m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Dispatch Test cases  ***************** */

char *API_Glsv_Dispatch_resultstring[] = {
   [LCK_DISPATCH_ONE_BAD_HANDLE_T]            = "saLckDispatch with invalid lock handle - DISPATCH_ONE",
   [LCK_DISPATCH_ONE_FINALIZED_HDL_T]         = "saLckDispatch with finalized lock handle - DISPATCH_ONE",
   [LCK_DISPATCH_ALL_BAD_HANDLE_T]            = "saLckDispatch with invalid lock handle - DISPATCH_ALL",
   [LCK_DISPATCH_ALL_FINALIZED_HDL_T]         = "saLckDispatch with finalized lock handle - DISPATCH_ALL",
   [LCK_DISPATCH_BLOCKING_BAD_HANDLE_T]       = "saLckDispatch with invalid lock handle - DISPATCH_BLOCKING",
   [LCK_DISPATCH_BLOCKING_FINALIZED_HDL_T]    = "saLckDispatch with finalized lock handle - DISPATCH_BLOCKING",
   [LCK_DISPATCH_BAD_FLAGS_T]                 = "saLckDispatch with invalid diapatch flags", 
   [LCK_DISPATCH_DISPATCH_ONE_SUCCESS_T]      = "saLckDispatch with valid parameters - DISPATCH_ONE",
   [LCK_DISPATCH_DISPATCH_ONE_SUCCESS2_T]     = "saLckDispatch with valid parameters - DISPATCH_ONE",
   [LCK_DISPATCH_DISPATCH_ALL_SUCCESS_T]      = "saLckDispatch with valid parameters - DISPATCH_ALL",
   [LCK_DISPATCH_DISPATCH_ALL_SUCCESS2_T]     = "saLckDispatch with valid parameters - DISPATCH_ALL",
   [LCK_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T] = "saLckDispatch with valid parameters - DISPATCH_BLOCKING",
   [LCK_DISPATCH_ERR_TRY_AGAIN_T]             = "saLckDispatch when service is not available",
};

struct SafDispatch API_Glsv_Dispatch[] = {
   [LCK_DISPATCH_ONE_BAD_HANDLE_T]            = {&gl_gla_env.inv_params.inv_lck_hdl,SA_DISPATCH_ONE,SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_ONE_FINALIZED_HDL_T]         = {&gl_gla_env.lck_hdl1,SA_DISPATCH_ONE,SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_ALL_BAD_HANDLE_T]            = {&gl_gla_env.inv_params.inv_lck_hdl,SA_DISPATCH_ALL,SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_ALL_FINALIZED_HDL_T]         = {&gl_gla_env.lck_hdl1,SA_DISPATCH_ALL,SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_BLOCKING_BAD_HANDLE_T]       = {&gl_gla_env.inv_params.inv_lck_hdl,SA_DISPATCH_BLOCKING,
                                                 SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_BLOCKING_FINALIZED_HDL_T]    = {&gl_gla_env.lck_hdl1,SA_DISPATCH_BLOCKING,SA_AIS_ERR_BAD_HANDLE},
   [LCK_DISPATCH_BAD_FLAGS_T]                 = {&gl_gla_env.lck_hdl1,-1,SA_AIS_ERR_INVALID_PARAM},
   [LCK_DISPATCH_DISPATCH_ONE_SUCCESS_T]      = {&gl_gla_env.lck_hdl1,SA_DISPATCH_ONE,SA_AIS_OK},
   [LCK_DISPATCH_DISPATCH_ONE_SUCCESS2_T]     = {&gl_gla_env.lck_hdl2,SA_DISPATCH_ONE,SA_AIS_OK},
   [LCK_DISPATCH_DISPATCH_ALL_SUCCESS_T]      = {&gl_gla_env.lck_hdl1,SA_DISPATCH_ALL,SA_AIS_OK},
   [LCK_DISPATCH_DISPATCH_ALL_SUCCESS2_T]     = {&gl_gla_env.lck_hdl2,SA_DISPATCH_ALL,SA_AIS_OK},
   [LCK_DISPATCH_DISPATCH_BLOCKING_SUCCESS_T] = {&gl_gla_env.lck_hdl1,SA_DISPATCH_BLOCKING,SA_AIS_OK},
   [LCK_DISPATCH_ERR_TRY_AGAIN_T]             = {&gl_gla_env.lck_hdl1,SA_DISPATCH_ONE,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckDispatch(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckDispatch(*API_Glsv_Dispatch[i].lckHandle,API_Glsv_Dispatch[i].flags);

   result = glsv_test_result(rc,API_Glsv_Dispatch[i].exp_output,
                             API_Glsv_Dispatch_resultstring[i],flg);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckDispatch(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Dispatch[i].exp_output;

   do
   {
      rc = saLckDispatch(*API_Glsv_Dispatch[i].lckHandle,API_Glsv_Dispatch[i].flags);

      result = glsv_test_result(rc,API_Glsv_Dispatch[i].exp_output,
                                API_Glsv_Dispatch_resultstring[i],flg);

      m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Finalize Test cases  ***************** */

char *API_Glsv_Finalize_resultstring[] = {
   [LCK_FINALIZE_BAD_HDL_T]       = "saLckFinalize with invalid lock handle",
   [LCK_FINALIZE_SUCCESS_T]       = "saLckFinalize with all valid parameters",
   [LCK_FINALIZE_SUCCESS_HDL2_T]  = "saLckFinalize with all valid parameters",
   [LCK_FINALIZE_SUCCESS_HDL3_T]  = "saLckFinalize with all valid parameters",
   [LCK_FINALIZE_FINALIZED_HDL_T] = "saLckFinalize with finalized lock handle",
   [LCK_FINALIZE_ERR_TRY_AGAIN_T] = "saLckFinalize when service is not available",
};

struct SafFinalize API_Glsv_Finalize[] = {
   [LCK_FINALIZE_BAD_HDL_T]      = {&gl_gla_env.inv_params.inv_lck_hdl,SA_AIS_ERR_BAD_HANDLE},
   [LCK_FINALIZE_SUCCESS_T]      = {&gl_gla_env.lck_hdl1,SA_AIS_OK},
   [LCK_FINALIZE_SUCCESS_HDL2_T] = {&gl_gla_env.lck_hdl2,SA_AIS_OK},
   [LCK_FINALIZE_SUCCESS_HDL3_T] = {&gl_gla_env.null_clbks_lck_hdl,SA_AIS_OK},
   [LCK_FINALIZE_FINALIZED_HDL_T]= {&gl_gla_env.lck_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_FINALIZE_ERR_TRY_AGAIN_T]= {&gl_gla_env.lck_hdl1,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckFinalize(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckFinalize(*API_Glsv_Finalize[i].lckHandle);

   result = glsv_test_result(rc,API_Glsv_Finalize[i].exp_output,
                             API_Glsv_Finalize_resultstring[i],flg);

   if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF(" Finalized Lock Handle : %llu\n",*API_Glsv_Finalize[i].lckHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n");
   return(result);
};

int tet_test_red_lckFinalize(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Finalize[i].exp_output;

   do
   {
      rc = saLckFinalize(*API_Glsv_Finalize[i].lckHandle);

      result = glsv_test_result(rc,API_Glsv_Finalize[i].exp_output,
                                API_Glsv_Finalize_resultstring[i],flg);

      if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
         m_TET_GLSV_PRINTF(" Finalized Lock Handle : %llu\n",*API_Glsv_Finalize[i].lckHandle);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
};

/* ***************  Lock Resource Open Test cases  ***************** */

char *API_Glsv_ResourceOpen_resultstring[] = {
   [LCK_RESOURCE_OPEN_BAD_HANDLE_T]                   = "saLckResourceOpen with invalid lock handle",
   [LCK_RESOURCE_OPEN_FINALIZED_HANDLE_T]             = "saLckResourceOpen with finalized lock handle",
   [LCK_RESOURCE_OPEN_NULL_RSC_NAME_T]                = "saLckResourceOpen with Null resource name",
   [LCK_RESOURCE_OPEN_NULL_RSC_HDL_T]                 = "saLckResourceOpen with Null resource handle",
   [LCK_RESOURCE_OPEN_BAD_FLAGS_T]                    = "saLckResourceOpen with invalid resource flags",
   [LCK_RESOURCE_OPEN_TIMEOUT_T]                      = "saLckResourceOpen - ERR_TIMEOUT case",
   [LCK_RESOURCE_OPEN_RSC_NOT_EXIST_T]                = "Open a resource that does not exist w/o create flag",
   [LCK_RESOURCE_OPEN_RSC_NOT_EXIST2_T]               = "Open a resource that does not exist w/o create flag",
   [LCK_RESOURCE_OPEN_HDL1_NAME1_SUCCESS_T]           = "saLckResourceOpen with valid parameters", 
   [LCK_RESOURCE_OPEN_HDL1_NAME2_SUCCESS_T]           = "saLckResourceOpen with valid parameters",
   [LCK_RESOURCE_OPEN_HDL1_NAME3_SUCCESS_T]           = "saLckResourceOpen with valid parameters",
   [LCK_RESOURCE_OPEN_HDL2_NAME1_SUCCESS_T]           = "saLckResourceOpen with valid parameters",
   [LCK_RESOURCE_OPEN_HDL2_NAME2_SUCCESS_T]           = "saLckResourceOpen with valid parameters",
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME1_SUCCESS_T] = "Open a resource that already exists with zero resource flag",
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME2_SUCCESS_T] = "Open a resource that already exists with zero resource flag",
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME1_SUCCESS_T] = "Open a resource that already exists with zero resource flag",
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME2_SUCCESS_T] = "Open a resource that already exists with zero resource flag",
   [LCK_RESOURCE_OPEN_NAME1_EXIST_SUCCESS_T]          = "Open a resource that exists with create flag", 
   [LCK_RESOURCE_OPEN_NAME2_EXIST_SUCCESS_T]          = "Open a resource that exists with create flag", 
   [LCK_RESOURCE_OPEN_ERR_TRY_AGAIN_T]                = "saLckResourceOpen when service is not available",
};

struct SafResourceOpen API_Glsv_ResourceOpen[] = {

   [LCK_RESOURCE_OPEN_BAD_HANDLE_T]         = {&gl_gla_env.inv_params.inv_lck_hdl,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                               &gl_gla_env.res_hdl1,SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_OPEN_FINALIZED_HANDLE_T]   = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                               &gl_gla_env.res_hdl1,SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_OPEN_NULL_RSC_NAME_T]      = {&gl_gla_env.lck_hdl1,NULL,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl1,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_INVALID_PARAM},
   [LCK_RESOURCE_OPEN_NULL_RSC_HDL_T]       = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,NULL,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_INVALID_PARAM},
   [LCK_RESOURCE_OPEN_BAD_FLAGS_T]          = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl1,
                                               100,SA_AIS_ERR_BAD_FLAGS},
   [LCK_RESOURCE_OPEN_TIMEOUT_T]            = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,0,&gl_gla_env.res_hdl1,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_TIMEOUT},
   [LCK_RESOURCE_OPEN_RSC_NOT_EXIST_T]      = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                               &gl_gla_env.res_hdl1,0,SA_AIS_ERR_NOT_EXIST},
   [LCK_RESOURCE_OPEN_RSC_NOT_EXIST2_T]     = {&gl_gla_env.lck_hdl1,&gl_gla_env.res2,RES_OPEN_TIMEOUT,
                                               &gl_gla_env.res_hdl1,0,SA_AIS_ERR_NOT_EXIST},
   [LCK_RESOURCE_OPEN_HDL1_NAME1_SUCCESS_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl1,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_HDL1_NAME2_SUCCESS_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.res2,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl2,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_HDL1_NAME3_SUCCESS_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.res3,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl3,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_HDL2_NAME1_SUCCESS_T] = {&gl_gla_env.lck_hdl2,&gl_gla_env.res1,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl1,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_HDL2_NAME2_SUCCESS_T] = {&gl_gla_env.lck_hdl2,&gl_gla_env.res2,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl2,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME1_SUCCESS_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl1,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL1_NAME2_SUCCESS_T] = {&gl_gla_env.lck_hdl1,&gl_gla_env.res2,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl2,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME1_SUCCESS_T] = {&gl_gla_env.lck_hdl2,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl1,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_RSC_EXIST_HDL2_NAME2_SUCCESS_T] = {&gl_gla_env.lck_hdl2,&gl_gla_env.res2,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl2,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_NAME1_EXIST_SUCCESS_T]          = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl1,SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_NAME2_EXIST_SUCCESS_T]          = {&gl_gla_env.lck_hdl1,&gl_gla_env.res2,RES_OPEN_TIMEOUT,
                                                         &gl_gla_env.res_hdl2,SA_LCK_RESOURCE_CREATE,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ERR_TRY_AGAIN_T]                = {&gl_gla_env.lck_hdl1,&gl_gla_env.res1,RES_OPEN_TIMEOUT,&gl_gla_env.res_hdl1,
                                               SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckResourceOpen(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceOpen(*API_Glsv_ResourceOpen[i].lckHandle,API_Glsv_ResourceOpen[i].lockResourceName,
                          API_Glsv_ResourceOpen[i].resourceFlags,API_Glsv_ResourceOpen[i].timeout,
                          API_Glsv_ResourceOpen[i].lockResourceHandle);

   if(flg != TEST_CLEANUP_MODE)
      print_res_info(API_Glsv_ResourceOpen[i].lockResourceName,API_Glsv_ResourceOpen[i].resourceFlags);

   result = glsv_test_result(rc,API_Glsv_ResourceOpen[i].exp_output,
                             API_Glsv_ResourceOpen_resultstring[i],flg);

   if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF(" Resource Handle : %llu\n", *API_Glsv_ResourceOpen[i].lockResourceHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceOpen(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceOpen[i].exp_output;

   do
   {
      rc = saLckResourceOpen(*API_Glsv_ResourceOpen[i].lckHandle,
                             API_Glsv_ResourceOpen[i].lockResourceName,
                             API_Glsv_ResourceOpen[i].resourceFlags,API_Glsv_ResourceOpen[i].timeout,
                             API_Glsv_ResourceOpen[i].lockResourceHandle);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         print_res_info(API_Glsv_ResourceOpen[i].lockResourceName,
                        API_Glsv_ResourceOpen[i].resourceFlags);

      result = glsv_test_result(rc,API_Glsv_ResourceOpen[i].exp_output,
                                API_Glsv_ResourceOpen_resultstring[i],flg);

      if(rc == SA_AIS_OK && flg != TEST_CLEANUP_MODE)
         m_TET_GLSV_PRINTF(" Resource Handle : %llu\n", *API_Glsv_ResourceOpen[i].lockResourceHandle);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Open Async Test cases  ***************** */

char *API_Glsv_ResourceOpenAsync_resultstring[] = {
   [LCK_RESOURCE_OPEN_ASYNC_BAD_HANDLE_T]               = "saLckResourceOpenAsync with invalid lock handle",
   [LCK_RESOURCE_OPEN_ASYNC_FINALIZED_HDL_T]            = "saLckResourceOpenAsync with finalized lock handle",
   [LCK_RESOURCE_OPEN_ASYNC_NULL_RSC_NAME_T]            = "saLckResourceOpenAsync with Null resource name",
   [LCK_RESOURCE_OPEN_ASYNC_BAD_FLAGS_T]                = "saLckResourceOpenAsync with invalid resource flags",
   [LCK_RESOURCE_OPEN_ASYNC_NULL_OPEN_CBK_T]            = "saLckResourceOpenAsync - ERR_INIT case",
   [LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST_T]            = "Open a resource that does not exist w/o create flag",
   [LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST2_T]           = "Open a resource that does not exist w/o create flag",
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_SUCCESS_T]       = "saLckResourceOpenAsync with valid parameters",
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME2_SUCCESS_T]       = "saLckResourceOpenAsync with valid parameters",
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_SUCCESS_T]       = "saLckResourceOpenAsync with valid parameters",
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME2_SUCCESS_T]       = "saLckResourceOpenAsync with valid parameters",
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_EXIST_SUCCESS_T] = "Open a resource that already exists with zero resource flags",
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_EXIST_SUCCESS_T] = "Open a resource that already exists with zero resource flags",
   [LCK_RESOURCE_OPEN_ASYNC_NAME1_EXIST_T]              = "Open a resource that already exists with create flag",
   [LCK_RESOURCE_OPEN_ASYNC_NAME2_EXIST_T]              = "Open a resource that already exists with create flag",
   [LCK_RESOURCE_OPEN_ASYNC_ERR_TRY_AGAIN_T]            = "saLckResourceOpenAsync when service is not available",
};

struct SafAsyncResourceOpen API_Glsv_ResourceOpenAsync[] = {
   [LCK_RESOURCE_OPEN_ASYNC_BAD_HANDLE_T]               = {&gl_gla_env.inv_params.inv_lck_hdl,200,&gl_gla_env.res1,
                                                           SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_OPEN_ASYNC_FINALIZED_HDL_T]            = {&gl_gla_env.lck_hdl1,201,&gl_gla_env.res1,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_OPEN_ASYNC_NULL_RSC_NAME_T]            = {&gl_gla_env.lck_hdl1,203,NULL,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_ERR_INVALID_PARAM},
   [LCK_RESOURCE_OPEN_ASYNC_NULL_OPEN_CBK_T]            = {&gl_gla_env.null_clbks_lck_hdl,204,&gl_gla_env.res1,
                                                           SA_LCK_RESOURCE_CREATE,SA_AIS_ERR_INIT},
   [LCK_RESOURCE_OPEN_ASYNC_BAD_FLAGS_T]                = {&gl_gla_env.lck_hdl1,205,&gl_gla_env.res1,100,
                                                           SA_AIS_ERR_BAD_FLAGS},
   [LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST_T]            = {&gl_gla_env.lck_hdl1,206,&gl_gla_env.res1,0,SA_AIS_OK}, 
   [LCK_RESOURCE_OPEN_ASYNC_RSC_NOT_EXIST2_T]           = {&gl_gla_env.lck_hdl1,207,&gl_gla_env.res2,0,SA_AIS_OK}, 
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_SUCCESS_T]       = {&gl_gla_env.lck_hdl1,208,&gl_gla_env.res1,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME2_SUCCESS_T]       = {&gl_gla_env.lck_hdl1,209,&gl_gla_env.res2,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_SUCCESS_T]       = {&gl_gla_env.lck_hdl2,210,&gl_gla_env.res1,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME2_SUCCESS_T]       = {&gl_gla_env.lck_hdl2,211,&gl_gla_env.res2,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_HDL1_NAME1_EXIST_SUCCESS_T] = {&gl_gla_env.lck_hdl1,212,&gl_gla_env.res1,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_HDL2_NAME1_EXIST_SUCCESS_T] = {&gl_gla_env.lck_hdl2,213,&gl_gla_env.res1,0,SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_NAME1_EXIST_T]              = {&gl_gla_env.lck_hdl1,214,&gl_gla_env.res1,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_NAME2_EXIST_T]              = {&gl_gla_env.lck_hdl1,215,&gl_gla_env.res2,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_OK},
   [LCK_RESOURCE_OPEN_ASYNC_ERR_TRY_AGAIN_T]            = {&gl_gla_env.lck_hdl1,216,&gl_gla_env.res1,SA_LCK_RESOURCE_CREATE,
                                                           SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckResourceOpenAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceOpenAsync(*API_Glsv_ResourceOpenAsync[i].lckHandle,API_Glsv_ResourceOpenAsync[i].Invocation,
                               API_Glsv_ResourceOpenAsync[i].lockResourceName,
                               API_Glsv_ResourceOpenAsync[i].resourceFlags);

   print_res_info(API_Glsv_ResourceOpenAsync[i].lockResourceName,API_Glsv_ResourceOpenAsync[i].resourceFlags);

   result = glsv_test_result(rc,API_Glsv_ResourceOpenAsync[i].exp_output,
                             API_Glsv_ResourceOpenAsync_resultstring[i],flg);

   if(rc == SA_AIS_OK)
      m_TET_GLSV_PRINTF(" Invocation      : %llu\n",API_Glsv_ResourceOpenAsync[i].Invocation);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceOpenAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceOpenAsync[i].exp_output;

   do
   {
      rc = saLckResourceOpenAsync(*API_Glsv_ResourceOpenAsync[i].lckHandle,
                                  API_Glsv_ResourceOpenAsync[i].Invocation,
                                  API_Glsv_ResourceOpenAsync[i].lockResourceName,
                                  API_Glsv_ResourceOpenAsync[i].resourceFlags);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         print_res_info(API_Glsv_ResourceOpenAsync[i].lockResourceName,
                        API_Glsv_ResourceOpenAsync[i].resourceFlags);

      result = glsv_test_result(rc,API_Glsv_ResourceOpenAsync[i].exp_output,
                                API_Glsv_ResourceOpenAsync_resultstring[i],flg);

      if(rc == SA_AIS_OK)
         m_TET_GLSV_PRINTF(" Invocation      : %llu\n",API_Glsv_ResourceOpenAsync[i].Invocation);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Close Test cases  ***************** */

char *API_Glsv_ResourceClose_resultstring[] = {
   [LCK_RESOURCE_CLOSE_BAD_RSC_HDL_T]      = "saLckResourceClose with invalid resource handle",
   [LCK_RESOURCE_CLOSE_BAD_HANDLE_T]       = "saLckResourceClose with closed resource handle",
   [LCK_RESOURCE_CLOSE_BAD_HANDLE2_T]      = "saLckResourceClose after finalizing the lock handle",
   [LCK_RESOURCE_CLOSE_RSC_HDL1_SUCCESS_T] = "saLckResourceClose with valid parameters",
   [LCK_RESOURCE_CLOSE_RSC_HDL2_SUCCESS_T] = "saLckResourceClose with valid parameters",
   [LCK_RESOURCE_CLOSE_ERR_TRY_AGAIN_T]    = "saLckResourceClose when service is not available",
};

struct SafResourceClose API_Glsv_ResourceClose[] = {
   [LCK_RESOURCE_CLOSE_BAD_RSC_HDL_T]      = {&gl_gla_env.inv_params.inv_res_hdl,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_CLOSE_BAD_HANDLE_T]       = {&gl_gla_env.res_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_CLOSE_BAD_HANDLE2_T]      = {&gl_gla_env.res_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RESOURCE_CLOSE_RSC_HDL1_SUCCESS_T] = {&gl_gla_env.res_hdl1,SA_AIS_OK},
   [LCK_RESOURCE_CLOSE_RSC_HDL2_SUCCESS_T] = {&gl_gla_env.res_hdl2,SA_AIS_OK},
   [LCK_RESOURCE_CLOSE_ERR_TRY_AGAIN_T]    = {&gl_gla_env.res_hdl1,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckResourceClose(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceClose(*API_Glsv_ResourceClose[i].lockResourceHandle);

   result = glsv_test_result(rc,API_Glsv_ResourceClose[i].exp_output,
                             API_Glsv_ResourceClose_resultstring[i],flg);

   if(rc == SA_AIS_OK)
      m_TET_GLSV_PRINTF(" Closed Res Hdl  : %llu\n",*API_Glsv_ResourceClose[i].lockResourceHandle);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceClose(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceClose[i].exp_output;

   do
   {
      rc = saLckResourceClose(*API_Glsv_ResourceClose[i].lockResourceHandle);

      result = glsv_test_result(rc,API_Glsv_ResourceClose[i].exp_output,
                                API_Glsv_ResourceClose_resultstring[i],flg);

      if(rc == SA_AIS_OK)
         m_TET_GLSV_PRINTF(" Closed Res Hdl  : %llu\n",*API_Glsv_ResourceClose[i].lockResourceHandle);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");
      
   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Lock Test cases  ***************** */

char *API_Glsv_ResourceLock_resultstring[] = {
   [LCK_RSC_LOCK_BAD_RSC_HDL_T]           = "saLckResourceLock with invalid resource handle",
   [LCK_RSC_LOCK_BAD_HDL_T]               = "saLckResourceLock with invalid resource handle",
   [LCK_RSC_LOCK_CLOSED_RSC_HDL_T]        = "saLckResourceLock with closed resource handle",
   [LCK_RSC_LOCK_FINALIZED_HDL_T]         = "saLckResourceLock after finalizing the lock handle",
   [LCK_RSC_LOCK_NULL_LCKID_T]            = "saLckResourceLock with Null lock id parameter",
   [LCK_RSC_LOCK_NULL_LCK_STATUS_T]       = "saLckResourceLock with Null lock status parameter",
   [LCK_RSC_LOCK_INVALID_LOCK_MODE_T]     = "saLckResourceLock with invalid lock mode",
   [LCK_RSC_LOCK_BAD_FLGS_T]              = "saLckResourceLock with invalid lock flags",
   [LCK_RSC_LOCK_ZERO_TIMEOUT_T]          = "saLckResourceLock - ERR_TIMEOUT case",
   [LCK_RSC_LOCK_PR_ERR_TIMEOUT_T]        = "saLckResourceLock - ERR_TIMEOUT case",
   [LCK_RSC_LOCK_EX_ERR_TIMEOUT_T]        = "saLckResourceLock - ERR_TIMEOUT case",
   [LCK_RSC_LOCK_PR_LOCK_SUCCESS_T]       = "Request lock in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_SUCCESS_T]       = "Request lock in EX mode",
   [LCK_RSC_LOCK_PR_LOCK_RSC2_SUCCESS_T]  = "Request lock in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_RSC2_SUCCESS_T]  = "Request lock in EX mode",
   [LCK_RSC_LOCK_EX_LOCK_RSC3_SUCCESS_T]  = "Request lock in EX mode",
   [LCK_RSC_LOCK_DUPLICATE_EXLCK_T]       = "saLckResourceLock - SA_LCK_LOCK_DUPLICATE_EX lock status case",
   [LCK_RSC_LOCK_NO_QUEUE_PRLCK_T]        = "saLckResourceLock with SA_LCK_LOCK_NO_QUEUE flag in PR mode",
   [LCK_RSC_LOCK_NO_QUEUE_EXLCK_T]        = "saLckResourceLock with SA_LCK_LOCK_NO_QUEUE flag in EX mode",
   [LCK_RSC_LOCK_ORPHAN_PRLCK_T]          = "saLckResourceLock with SA_LCK_LOCK_ORPHAN flag in PR mode",
   [LCK_RSC_LOCK_ORPHAN_EXLCK_T]          = "saLckResourceLock with SA_LCK_LOCK_ORPHAN flag in EX mode",
   [LCK_RSC_LOCK_ORPHAN_EXLCK_RSC2_T]     = "Successful saLckResourceLock in EX mode with Orphan Flag2",
   [LCK_RSC_LOCK_PR_LOCK_NOT_QUEUED_T]    = "saLckResourceLock - SA_LCK_LOCK_NOT_QUEUED lock status case in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_NOT_QUEUED_T]    = "saLckResourceLock - SA_LCK_LOCK_NOT_QUEUED lock status case in EX mode",
   [LCK_RSC_LOCK_PR_LOCK_DEADLOCK_T]      = "saLckResourceLock - SA_LCK_LOCK_DEADLOCK lock status case in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_DEADLOCK_T]      = "saLckResourceLock - SA_LCK_LOCK_DEADLOCK lock status case in EX mode",
   [LCK_RSC_LOCK_PR_LOCK_RSC2_DEADLOCK_T] = "saLckResourceLock - SA_LCK_LOCK_DEADLOCK lock status case in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_RSC2_DEADLOCK_T] = "saLckResourceLock - SA_LCK_LOCK_DEADLOCK lock status case in EX mode",
   [LCK_RSC_LOCK_EX_LOCK_ORPHANED_T]      = "saLckResourceLock - SA_LCK_LOCK_ORPHANED lock status case in EX mode",
   [LCK_RSC_LOCK_PR_LOCK_ORPHANED_T]      = "saLckResourceLock - SA_LCK_LOCK_ORPHANED lock status case in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_RSC2_ORPHANED_T] = "saLckResourceLock - SA_LCK_LOCK_ORPHANED lock status case in EX mode",
   [LCK_RSC_LOCK_PR_LOCK_RSC2_ORPHANED_T] = "saLckResourceLock - SA_LCK_LOCK_ORPHANED lock status case in PR mode",
   [LCK_RSC_LOCK_EX_LOCK_ORPHAN_DDLCK_T]  = "saLckResourceLock - Deadlock case with orphan flag",
   [LCK_RSC_LOCK_ERR_TRY_AGAIN_T]         = "saLckResourceLock when service is not available",
};

struct SafResourceLock API_Glsv_ResourceLock[] = {
   [LCK_RSC_LOCK_BAD_RSC_HDL_T]           = {&gl_gla_env.inv_params.inv_res_hdl,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             0,1000,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_BAD_HANDLE,0},
   [LCK_RSC_LOCK_BAD_HDL_T]               = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             0,1000,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_BAD_HANDLE,0},
   [LCK_RSC_LOCK_CLOSED_RSC_HDL_T]        = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             0,1100,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_BAD_HANDLE,0},
   [LCK_RSC_LOCK_FINALIZED_HDL_T]         = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             0,1200,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_BAD_HANDLE,0},
   [LCK_RSC_LOCK_NULL_LCKID_T]            = {&gl_gla_env.res_hdl1,NULL,SA_LCK_PR_LOCK_MODE,0,1300,RES_LOCK_TIMEOUT,
                                             &gl_gla_env.lck_status,SA_AIS_ERR_INVALID_PARAM,0},
   [LCK_RSC_LOCK_NULL_LCK_STATUS_T]       = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,1400,
                                             RES_LOCK_TIMEOUT,NULL,SA_AIS_ERR_INVALID_PARAM,0},
   [LCK_RSC_LOCK_INVALID_LOCK_MODE_T]     = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,100,0,1500,RES_LOCK_TIMEOUT,
                                             &gl_gla_env.lck_status,SA_AIS_ERR_INVALID_PARAM,0}, 
   [LCK_RSC_LOCK_BAD_FLGS_T]              = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,100,1600,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_BAD_FLAGS,0},
   [LCK_RSC_LOCK_ZERO_TIMEOUT_T]          = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,1700,
                                             0,&gl_gla_env.lck_status,SA_AIS_ERR_TIMEOUT,0},
   [LCK_RSC_LOCK_PR_ERR_TIMEOUT_T]        = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,1700,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_TIMEOUT,0},
   [LCK_RSC_LOCK_EX_ERR_TIMEOUT_T]        = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,1700,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_TIMEOUT,0},
   [LCK_RSC_LOCK_PR_LOCK_SUCCESS_T]       = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,1800,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_EX_LOCK_SUCCESS_T]       = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,1900,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_PR_LOCK_RSC2_SUCCESS_T]  = {&gl_gla_env.res_hdl2,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,2000,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_EX_LOCK_RSC2_SUCCESS_T]  = {&gl_gla_env.res_hdl2,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,2100,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_DUPLICATE_EXLCK_T]       = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,2200,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_DUPLICATE_EX},
   [LCK_RSC_LOCK_NO_QUEUE_PRLCK_T]        = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             SA_LCK_LOCK_NO_QUEUE,2300,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_NO_QUEUE_EXLCK_T]        = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_NO_QUEUE,2400,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_ORPHAN_PRLCK_T]          = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,2500,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_ORPHAN_EXLCK_T]          = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,2600,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_ORPHAN_EXLCK_RSC2_T]     = {&gl_gla_env.res_hdl2,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,2700,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_PR_LOCK_NOT_QUEUED_T]    = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             SA_LCK_LOCK_NO_QUEUE,2800,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_NOT_QUEUED},
   [LCK_RSC_LOCK_EX_LOCK_NOT_QUEUED_T]    = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_NO_QUEUE,2900,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_NOT_QUEUED},
   [LCK_RSC_LOCK_PR_LOCK_DEADLOCK_T]      = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,3000,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_DEADLOCK},
   [LCK_RSC_LOCK_EX_LOCK_DEADLOCK_T]      = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,3100,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_DEADLOCK},
   [LCK_RSC_LOCK_PR_LOCK_RSC2_DEADLOCK_T] = {&gl_gla_env.res_hdl2,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,3200,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_DEADLOCK},
   [LCK_RSC_LOCK_EX_LOCK_RSC2_DEADLOCK_T] = {&gl_gla_env.res_hdl2,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,3300,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_DEADLOCK},
   [LCK_RSC_LOCK_EX_LOCK_ORPHANED_T]      = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,3400,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_ORPHANED},
   [LCK_RSC_LOCK_PR_LOCK_ORPHANED_T]      = {&gl_gla_env.res_hdl1,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,3500,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_ORPHANED},
   [LCK_RSC_LOCK_PR_LOCK_RSC2_ORPHANED_T] = {&gl_gla_env.res_hdl2,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,3600,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_ORPHANED},
   [LCK_RSC_LOCK_EX_LOCK_RSC2_ORPHANED_T] = {&gl_gla_env.res_hdl2,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,3700,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_ORPHANED},
   [LCK_RSC_LOCK_EX_LOCK_RSC3_SUCCESS_T]  = {&gl_gla_env.res_hdl3,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,3800,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_OK,SA_LCK_LOCK_GRANTED},
   [LCK_RSC_LOCK_EX_LOCK_ORPHAN_DDLCK_T]  = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                             SA_LCK_LOCK_ORPHAN,3900,RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,
                                             SA_AIS_OK,SA_LCK_LOCK_DEADLOCK},
   [LCK_RSC_LOCK_ERR_TRY_AGAIN_T]         = {&gl_gla_env.res_hdl1,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,1900,
                                             RES_LOCK_TIMEOUT,&gl_gla_env.lck_status,SA_AIS_ERR_TRY_AGAIN,SA_LCK_LOCK_GRANTED},
};

int tet_test_lckResourceLock(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceLock(*API_Glsv_ResourceLock[i].lockResourceHandle,API_Glsv_ResourceLock[i].lockId,
                          API_Glsv_ResourceLock[i].lockMode,API_Glsv_ResourceLock[i].lockFlags,
                          API_Glsv_ResourceLock[i].waiterSignal,API_Glsv_ResourceLock[i].timeout,
                          API_Glsv_ResourceLock[i].lockStatus);

   print_lock_info(API_Glsv_ResourceLock[i].lockResourceHandle,API_Glsv_ResourceLock[i].lockMode,
                   API_Glsv_ResourceLock[i].lockFlags,API_Glsv_ResourceLock[i].waiterSignal);

   result = glsv_test_result(rc,API_Glsv_ResourceLock[i].exp_output,
                             API_Glsv_ResourceLock_resultstring[i],flg);

   if (rc == SA_AIS_OK)
   {
      m_TET_GLSV_PRINTF(" LockId          : %llu\n",*API_Glsv_ResourceLock[i].lockId);
      m_TET_GLSV_PRINTF(" Lock Status     : %s\n",saf_lck_status_string[*(API_Glsv_ResourceLock[i].lockStatus)]);
   }

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceLock(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceLock[i].exp_output;

   do
   {
      rc = saLckResourceLock(*API_Glsv_ResourceLock[i].lockResourceHandle,
                             API_Glsv_ResourceLock[i].lockId,
                             API_Glsv_ResourceLock[i].lockMode,API_Glsv_ResourceLock[i].lockFlags,
                             API_Glsv_ResourceLock[i].waiterSignal,API_Glsv_ResourceLock[i].timeout,
                             API_Glsv_ResourceLock[i].lockStatus);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         print_lock_info(API_Glsv_ResourceLock[i].lockResourceHandle,API_Glsv_ResourceLock[i].lockMode,
                         API_Glsv_ResourceLock[i].lockFlags,API_Glsv_ResourceLock[i].waiterSignal);

      result = glsv_test_result(rc,API_Glsv_ResourceLock[i].exp_output,
                                API_Glsv_ResourceLock_resultstring[i],flg);

      if (rc == SA_AIS_OK)
      {
         m_TET_GLSV_PRINTF(" LockId          : %llu\n",*API_Glsv_ResourceLock[i].lockId);
         m_TET_GLSV_PRINTF(" Lock Status     : %s\n",
           saf_lck_status_string[*(API_Glsv_ResourceLock[i].lockStatus)]);
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Lock Async Test cases  ***************** */

char *API_Glsv_ResourceLockAsync_resultstring[] = {
   [LCK_RSC_LOCK_ASYNC_BAD_RSC_HDL_T]        = "saLckResourceLockAsync with invalid resource handle",
   [LCK_RSC_LOCK_ASYNC_CLOSED_RSC_HDL_T]     = "saLckResourceLockAsync with closed resource handle",
   [LCK_RSC_LOCK_ASYNC_FINALIZED_HDL_T]      = "saLckResourceLockAsync after finalizing the lock handle",
   [LCK_RSC_LOCK_ASYNC_NULL_LCKID_T]         = "saLckResourceLockAsync with Null lock id parameter",
   [LCK_RSC_LOCK_ASYNC_INVALID_LOCK_MODE_T]  = "saLckResourceLockAsync with invalid Lock mode",
   [LCK_RSC_LOCK_ASYNC_BAD_FLGS_T]           = "saLckResourceLockAsync with invalid lock flags",
   [LCK_RSC_LOCK_ASYNC_ERR_INIT_T]           = "saLckResourceLockAsync - ERR_INIT case",
   [LCK_RSC_LOCK_ASYNC_PRLCK_SUCCESS_T]      = "Request lock in PR mode",
   [LCK_RSC_LOCK_ASYNC_EXLCK_SUCCESS_T]      = "Request lock in EX mode",
   [LCK_RSC_LOCK_ASYNC_PRLCK_RSC2_SUCCESS_T] = "Request lock in PR mode",
   [LCK_RSC_LOCK_ASYNC_EXLCK_RSC2_SUCCESS_T] = "Request lock in EX mode",
   [LCK_RSC_LOCK_ASYNC_NO_QUEUE_PRLCK_T]     = "saLckResourceLockAsync with SA_LCK_LOCK_NO_QUEUE lock flag in PR mode",
   [LCK_RSC_LOCK_ASYNC_NO_QUEUE_EXLCK_T]     = "saLckResourceLockAsync with SA_LCK_LOCK_NO_QUEUE lock flag in PR mode",
   [LCK_RSC_LOCK_ASYNC_ORPHAN_PRLCK_T]       = "saLckResourceLockAsync with SA_LCK_LOCK_ORPHAN lock flag in PR mode",
   [LCK_RSC_LOCK_ASYNC_ORPHAN_EXLCK_T]       = "saLckResourceLockAsync with SA_LCK_LOCK_ORPHAN lock flag in EX mode",
   [LCK_RSC_LOCK_ASYNC_ERR_TRY_AGAIN_T]      = "saLckResourceLockAsync when service is not available",
};

struct SafAsyncResourceLock API_Glsv_ResourceLockAsync[] = {
   [LCK_RSC_LOCK_ASYNC_BAD_RSC_HDL_T]        = {&gl_gla_env.inv_params.inv_res_hdl,700,&gl_gla_env.ex_lck_id,
                                                SA_LCK_EX_LOCK_MODE,0,5000,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_LOCK_ASYNC_CLOSED_RSC_HDL_T]     = {&gl_gla_env.res_hdl1,701,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                5100,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_LOCK_ASYNC_NULL_LCKID_T]         = {&gl_gla_env.res_hdl1,702,NULL,SA_LCK_EX_LOCK_MODE,0,
                                                5200,SA_AIS_ERR_INVALID_PARAM},
   [LCK_RSC_LOCK_ASYNC_INVALID_LOCK_MODE_T]  = {&gl_gla_env.res_hdl1,703,&gl_gla_env.pr_lck_id,90,0,
                                                5300,SA_AIS_ERR_INVALID_PARAM},
   [LCK_RSC_LOCK_ASYNC_BAD_FLGS_T]           = {&gl_gla_env.res_hdl1,704,&gl_gla_env.pr_lck_id,SA_LCK_EX_LOCK_MODE,23,
                                                5400,SA_AIS_ERR_BAD_FLAGS},
   [LCK_RSC_LOCK_ASYNC_ERR_INIT_T]           = {&gl_gla_env.res_hdl1,705,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                5500,SA_AIS_ERR_INIT}, 
   [LCK_RSC_LOCK_ASYNC_PRLCK_SUCCESS_T]      = {&gl_gla_env.res_hdl1,706,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,
                                                5600,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_EXLCK_SUCCESS_T]      = {&gl_gla_env.res_hdl1,707,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                5700,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_PRLCK_RSC2_SUCCESS_T] = {&gl_gla_env.res_hdl2,708,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,0,
                                                5800,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_EXLCK_RSC2_SUCCESS_T] = {&gl_gla_env.res_hdl2,709,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                5900,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_NO_QUEUE_PRLCK_T]     = {&gl_gla_env.res_hdl1,710,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                                SA_LCK_LOCK_NO_QUEUE,6000,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_NO_QUEUE_EXLCK_T]     = {&gl_gla_env.res_hdl1,711,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                                SA_LCK_LOCK_NO_QUEUE,6100,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_ORPHAN_PRLCK_T]       = {&gl_gla_env.res_hdl1,712,&gl_gla_env.pr_lck_id,SA_LCK_PR_LOCK_MODE,
                                                SA_LCK_LOCK_ORPHAN,6200,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_ORPHAN_EXLCK_T]       = {&gl_gla_env.res_hdl1,713,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,
                                                SA_LCK_LOCK_ORPHAN,6300,SA_AIS_OK},
   [LCK_RSC_LOCK_ASYNC_FINALIZED_HDL_T]      = {&gl_gla_env.res_hdl1,714,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                6500,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_LOCK_ASYNC_ERR_TRY_AGAIN_T]      = {&gl_gla_env.res_hdl1,707,&gl_gla_env.ex_lck_id,SA_LCK_EX_LOCK_MODE,0,
                                                5700,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckResourceLockAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceLockAsync(*API_Glsv_ResourceLockAsync[i].lockResourceHandle,API_Glsv_ResourceLockAsync[i].invocation,
                               API_Glsv_ResourceLockAsync[i].lockId,API_Glsv_ResourceLockAsync[i].lockMode,
                               API_Glsv_ResourceLockAsync[i].lockFlags,API_Glsv_ResourceLockAsync[i].waiterSignal);

   print_lock_info(API_Glsv_ResourceLockAsync[i].lockResourceHandle,API_Glsv_ResourceLockAsync[i].lockMode,
                   API_Glsv_ResourceLockAsync[i].lockFlags,API_Glsv_ResourceLockAsync[i].waiterSignal);

   result = glsv_test_result(rc,API_Glsv_ResourceLockAsync[i].exp_output,
                             API_Glsv_ResourceLockAsync_resultstring[i],flg);

   if(rc == SA_AIS_OK)
   {
      m_TET_GLSV_PRINTF(" LockId          : %llu\n",*API_Glsv_ResourceLockAsync[i].lockId);
      m_TET_GLSV_PRINTF(" Invocation      : %llu\n",API_Glsv_ResourceLockAsync[i].invocation);
   }

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceLockAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceLockAsync[i].exp_output;

   do
   {
      rc = saLckResourceLockAsync(*API_Glsv_ResourceLockAsync[i].lockResourceHandle,
                                  API_Glsv_ResourceLockAsync[i].invocation,
                                  API_Glsv_ResourceLockAsync[i].lockId,
                                  API_Glsv_ResourceLockAsync[i].lockMode,
                                  API_Glsv_ResourceLockAsync[i].lockFlags,
                                  API_Glsv_ResourceLockAsync[i].waiterSignal);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         print_lock_info(API_Glsv_ResourceLockAsync[i].lockResourceHandle,
                         API_Glsv_ResourceLockAsync[i].lockMode,
                         API_Glsv_ResourceLockAsync[i].lockFlags,
                         API_Glsv_ResourceLockAsync[i].waiterSignal);

      result = glsv_test_result(rc,API_Glsv_ResourceLockAsync[i].exp_output,
                                API_Glsv_ResourceLockAsync_resultstring[i],flg);

      if(rc == SA_AIS_OK)
      {
         m_TET_GLSV_PRINTF(" LockId          : %llu\n",*API_Glsv_ResourceLockAsync[i].lockId);
         m_TET_GLSV_PRINTF(" Invocation      : %llu\n",API_Glsv_ResourceLockAsync[i].invocation);
      }

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Unlock Async Test cases  ***************** */

char *API_Glsv_ResourceUnlockAsync_resultstring[] = {
   [LCK_RSC_UNLOCK_ASYNC_BAD_LOCKID_T]      = "saLckResourceUnlockAsync with invalid lock id",
   [LCK_RSC_UNLOCK_ASYNC_UNLOCKED_LOCKID_T] = "saLckResourceUnlockAsync with unlocked lock id",
   [LCK_RSC_UNLOCK_ASYNC_BAD_HDL_T]         = "saLckResourceUnlockAsync after closing the resource",
   [LCK_RSC_UNLOCK_ASYNC_BAD_HDL2_T]        = "saLckResourceUnlockAsync after finalizing the lock handle",
   [LCK_RSC_UNLOCK_ASYNC_SUCCESS_T]         = "saLckResourceUnlockAsync with valid parameters", 
   [LCK_RSC_UNLOCK_ASYNC_SUCCESS_ID2_T]     = "saLckResourceUnlockAsync with valid parameters", 
   [LCK_RSC_UNLOCK_ASYNC_ERR_INIT_T]        = "saLckResourceUnlockAsync - ERR_INIT case",
   [LCK_RSC_UNLOCK_ASYNC_ERR_TRY_AGAIN_T]   = "saLckResourceUnlockAsync when service is not available",
   [LCK_RSC_FINALIZED_ASYNC_UNLOCKED_LOCKID_T]    = "saLckResourceUnlockAsync after finalizing the lock handle"
};

struct SafAsyncResourceUnlock API_Glsv_ResourceUnlockAsync[] = {
   [LCK_RSC_UNLOCK_ASYNC_BAD_LOCKID_T]      = {900,&gl_gla_env.inv_params.inv_lck_id,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_ASYNC_UNLOCKED_LOCKID_T] = {901,&gl_gla_env.ex_lck_id,SA_AIS_ERR_NOT_EXIST},
   [LCK_RSC_UNLOCK_ASYNC_BAD_HDL_T]         = {902,&gl_gla_env.ex_lck_id,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_ASYNC_BAD_HDL2_T]        = {903,&gl_gla_env.ex_lck_id,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_ASYNC_SUCCESS_T]         = {904,&gl_gla_env.ex_lck_id,SA_AIS_OK},
   [LCK_RSC_UNLOCK_ASYNC_SUCCESS_ID2_T]     = {905,&gl_gla_env.pr_lck_id,SA_AIS_OK},
   [LCK_RSC_UNLOCK_ASYNC_ERR_INIT_T]        = {906,&gl_gla_env.ex_lck_id,SA_AIS_ERR_INIT},
   [LCK_RSC_UNLOCK_ASYNC_ERR_TRY_AGAIN_T]   = {904,&gl_gla_env.ex_lck_id,SA_AIS_ERR_TRY_AGAIN},
   [LCK_RSC_FINALIZED_ASYNC_UNLOCKED_LOCKID_T] = {901,&gl_gla_env.ex_lck_id,SA_AIS_ERR_BAD_HANDLE},
};

int tet_test_lckResourceUnlockAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceUnlockAsync(API_Glsv_ResourceUnlockAsync[i].invocation,
                                 *API_Glsv_ResourceUnlockAsync[i].lockId);

   m_TET_GLSV_PRINTF("\n LockId          : %llu",*API_Glsv_ResourceUnlockAsync[i].lockId);
   m_TET_GLSV_PRINTF("\n Invocation      : %llu",API_Glsv_ResourceUnlockAsync[i].invocation);

   result = glsv_test_result(rc,API_Glsv_ResourceUnlockAsync[i].exp_output,
                             API_Glsv_ResourceUnlockAsync_resultstring[i],flg);

   m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceUnlockAsync(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceUnlockAsync[i].exp_output;

   do
   {
      rc = saLckResourceUnlockAsync(API_Glsv_ResourceUnlockAsync[i].invocation,
                                    *API_Glsv_ResourceUnlockAsync[i].lockId);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
      {
         m_TET_GLSV_PRINTF("\n LockId          : %llu",*API_Glsv_ResourceUnlockAsync[i].lockId);
         m_TET_GLSV_PRINTF("\n Invocation      : %llu",API_Glsv_ResourceUnlockAsync[i].invocation);
      }

      result = glsv_test_result(rc,API_Glsv_ResourceUnlockAsync[i].exp_output,
                                API_Glsv_ResourceUnlockAsync_resultstring[i],flg);

      if(rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Resource Unlock Test cases  ***************** */

char *API_Glsv_ResourceUnlock_resultstring[] = {
   [LCK_RSC_UNLOCK_BAD_LOCKID_T]      = "saLckResourceUnlock with invalid lock id",
   [LCK_RSC_UNLOCK_UNLOCKED_LOCKID_T] = "saLckResourceUnlock with unlocked lock id",
   [LCK_RSC_UNLOCK_BAD_HDL_T]         = "saLckResourceUnlock after closing the resource",
   [LCK_RSC_UNLOCK_BAD_HDL2_T]        = "saLckResourceUnlock after finalizing the lock handle",
   [LCK_RSC_UNLOCK_ERR_TIMEOUT_T]     = "saLckResourceUnlock - ERR_TIMEOUT case",
   [LCK_RSC_UNLOCK_LCKID1_SUCCESS_T]  = "saLckResourceUnlock with valid parameters", 
   [LCK_RSC_UNLOCK_LCKID2_SUCCESS_T]  = "saLckResourceUnlock with valid parameters",
   [LCK_RSC_UNLOCK_ERR_TRY_AGAIN_T]   = "saLckResourceUnlock when service is not available",
   [LCK_RSC_FINALIZED_UNLOCKED_LOCKID_T]    = "saLckResourceUnlock after finalizing the lock handle"   
};

struct SafResourceUnlock API_Glsv_ResourceUnlock[] = {
   [LCK_RSC_UNLOCK_BAD_LOCKID_T]      = {&gl_gla_env.inv_params.inv_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_UNLOCKED_LOCKID_T] = {&gl_gla_env.ex_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_NOT_EXIST},
   [LCK_RSC_UNLOCK_BAD_HDL_T]         = {&gl_gla_env.ex_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_BAD_HDL2_T]        = {&gl_gla_env.pr_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
   [LCK_RSC_UNLOCK_ERR_TIMEOUT_T]     = {&gl_gla_env.ex_lck_id,100,SA_AIS_ERR_TIMEOUT},
   [LCK_RSC_UNLOCK_LCKID1_SUCCESS_T]  = {&gl_gla_env.ex_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_OK},
   [LCK_RSC_UNLOCK_LCKID2_SUCCESS_T]  = {&gl_gla_env.pr_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_OK},
   [LCK_RSC_UNLOCK_ERR_TRY_AGAIN_T]   = {&gl_gla_env.ex_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_TRY_AGAIN},
   [LCK_RSC_FINALIZED_UNLOCKED_LOCKID_T] = {&gl_gla_env.ex_lck_id,RES_UNLOCK_TIMEOUT,SA_AIS_ERR_BAD_HANDLE},
};

int tet_test_lckResourceUnlock(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckResourceUnlock(*API_Glsv_ResourceUnlock[i].lockId,API_Glsv_ResourceUnlock[i].timeout);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n LockId          : %llu",*API_Glsv_ResourceUnlock[i].lockId);

   result = glsv_test_result(rc,API_Glsv_ResourceUnlock[i].exp_output,
                             API_Glsv_ResourceUnlock_resultstring[i],flg);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckResourceUnlock(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_ResourceUnlock[i].exp_output;

   do
   {
      rc = saLckResourceUnlock(*API_Glsv_ResourceUnlock[i].lockId,API_Glsv_ResourceUnlock[i].timeout);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n LockId          : %llu",*API_Glsv_ResourceUnlock[i].lockId);

      result = glsv_test_result(rc,API_Glsv_ResourceUnlock[i].exp_output,
                                API_Glsv_ResourceUnlock_resultstring[i],flg);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* ***************  Lock Purge Test cases  ***************** */

char *API_Glsv_Purge_resultstring[] = {
   [LCK_LOCK_PURGE_BAD_HDL_T]       = "saLckLockPurge with invalid resource handle",
   [LCK_LOCK_PURGE_CLOSED_HDL_T]    = "saLckLockPurge with closed resource handle",
   [LCK_LOCK_PURGE_FINALIZED_HDL_T] = "saLckLockPurge after finalzing the lock handle",
   [LCK_LOCK_PURGE_SUCCESS_T]       = "saLckLockPurge with valid parameters",
   [LCK_LOCK_PURGE_SUCCESS_RSC2_T]  = "saLckLockPurge with valid parameters",
   [LCK_LOCK_PURGE_NO_ORPHAN_T]     = "saLckLockPurge when there are no orphan locks the resource",
   [LCK_LOCK_PURGE_ERR_TRY_AGAIN_T] = "saLckLockPurge when service is not available",
};

struct SafPurge API_Glsv_Purge[]={
   [LCK_LOCK_PURGE_BAD_HDL_T]       = {&gl_gla_env.res_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_LOCK_PURGE_CLOSED_HDL_T]    = {&gl_gla_env.res_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_LOCK_PURGE_FINALIZED_HDL_T] = {&gl_gla_env.res_hdl1,SA_AIS_ERR_BAD_HANDLE},
   [LCK_LOCK_PURGE_SUCCESS_T]       = {&gl_gla_env.res_hdl1,SA_AIS_OK},
   [LCK_LOCK_PURGE_SUCCESS_RSC2_T]  = {&gl_gla_env.res_hdl2,SA_AIS_OK},
   [LCK_LOCK_PURGE_NO_ORPHAN_T]     = {&gl_gla_env.res_hdl1,SA_AIS_OK},
   [LCK_LOCK_PURGE_ERR_TRY_AGAIN_T] = {&gl_gla_env.res_hdl1,SA_AIS_ERR_TRY_AGAIN},
};

int tet_test_lckLockPurge(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   int result;

   rc = saLckLockPurge(*API_Glsv_Purge[i].lockResourceHandle);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n Resource Hdl    : %llu",*API_Glsv_Purge[i].lockResourceHandle);

   result = glsv_test_result(rc,API_Glsv_Purge[i].exp_output,API_Glsv_Purge_resultstring[i],flg);

   if(flg != TEST_CLEANUP_MODE)
      m_TET_GLSV_PRINTF("\n");
   return(result);
}

int tet_test_red_lckLockPurge(int i,GLSV_CONFIG_FLAG flg)
{
   SaAisErrorT rc;
   SaAisErrorT exp_output;
   int result;

   gl_try_again_cnt = 0;
   exp_output = API_Glsv_Purge[i].exp_output;

   do
   {
      rc = saLckLockPurge(*API_Glsv_Purge[i].lockResourceHandle);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n Resource Hdl    : %llu",*API_Glsv_Purge[i].lockResourceHandle);

      result = glsv_test_result(rc,API_Glsv_Purge[i].exp_output,API_Glsv_Purge_resultstring[i],flg);

      if(flg != TEST_CLEANUP_MODE && rc != SA_AIS_ERR_TRY_AGAIN)
         m_TET_GLSV_PRINTF("\n");

   }END_OF_WHILE;

   if(gl_try_again_cnt)
      m_TET_GLSV_PRINTF("\n Final Retries   : %d \n",gl_try_again_cnt);

   return(result);
}

/* *************** GLSV Threads ***************** */

/* Dispatch thread - DISPATCH_BLOCKING */

void glsv_selection_thread (NCSCONTEXT arg)
{
   SaLckHandleT *lck_Handle = (SaLckHandleT *)arg;
   uint32_t rc;

   rc = saLckDispatch(*lck_Handle, SA_DISPATCH_BLOCKING);
   if(rc != SA_AIS_OK)
   {
      m_TET_GLSV_PRINTF("dispatching failed %s \n", glsv_saf_error_string[rc]);
      pthread_exit(0);
   }
}

void glsv_createthread(SaLckHandleT *lck_Handle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_lck_thrd_cnt >= 2)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)glsv_selection_thread, (NCSCONTEXT)lck_Handle,
                          "gla_asynctest_blocking", 5, 8000, &thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
        m_TET_GLSV_PRINTF("failed to create thread\n");
        return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
     m_TET_GLSV_PRINTF("failed to start thread\n");
     return ;
   }
}

/* Dispatch thread - DISPATCH_ONE */

void glsv_selection_thread_one (NCSCONTEXT arg)
{
   SaLckHandleT *lck_Handle = (SaLckHandleT *)arg;
   uint32_t rc;

   rc = saLckDispatch(*lck_Handle, SA_DISPATCH_ONE);
   if(rc != SA_AIS_OK)
      m_TET_GLSV_PRINTF("dispatching failed %s \n", glsv_saf_error_string[rc]);
   pthread_exit(0);
}

void glsv_createthread_one(SaLckHandleT *lck_Handle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_lck_thrd_cnt >= 2)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)glsv_selection_thread_one,(NCSCONTEXT)lck_Handle,
                           "gla_asynctest",5,8000,&thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
        m_TET_GLSV_PRINTF("failed to create thread\n");
        return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
     m_TET_GLSV_PRINTF("failed to start thread\n");
     return ;
   }
}

/* Dispatch thread - DISPATCH_ALL */

void glsv_selection_thread_all (NCSCONTEXT arg)
{
   SaLckHandleT *lck_Handle = (SaLckHandleT *)arg;
   uint32_t rc;

   rc = saLckDispatch(*lck_Handle, SA_DISPATCH_ALL);
   if(rc != SA_AIS_OK)
      m_TET_GLSV_PRINTF("dispatching failed %s \n", glsv_saf_error_string[rc]);
   pthread_exit(0);
}

void glsv_createthread_all(SaLckHandleT *lck_Handle)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_lck_thrd_cnt >= 2)
      return;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)glsv_selection_thread_all,(NCSCONTEXT)lck_Handle,
                           "gla_asynctest",5,8000,&thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
        m_TET_GLSV_PRINTF("failed to create thread\n");
        return ;
   }

   rc = m_NCS_TASK_START(thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
     m_TET_GLSV_PRINTF("failed to start thread\n");
     return ;
   }
}

/* Dispatch Thread - DISPATCH_ONE Infinite loop */

void glsv_selection_thread_all_loop (NCSCONTEXT arg)
{
   while(1)
   {
      saLckDispatch(gl_gla_env.lck_hdl1, SA_DISPATCH_ALL);
      sleep(1);
   }
}

void glsv_selection_thread_all_loop_hdl2 (NCSCONTEXT arg)
{
   while(1)
   {
      saLckDispatch(gl_gla_env.lck_hdl2, SA_DISPATCH_ALL);
      sleep(1);
   }
}

void glsv_createthread_all_loop(int hdl)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   if(gl_lck_thrd_cnt >= 2)
      return;

   if(hdl == 1)
   {
      rc = m_NCS_TASK_CREATE((NCS_OS_CB)glsv_selection_thread_all_loop,(NCSCONTEXT)NULL,
                           "gla_asynctest",5,8000,&thread_handle);
      if(rc != NCSCC_RC_SUCCESS)
      {
          m_TET_GLSV_PRINTF("failed to create thread\n");
          return ;
      }
   }
   if(hdl == 2)
   {
      rc = m_NCS_TASK_CREATE((NCS_OS_CB)glsv_selection_thread_all_loop_hdl2,(NCSCONTEXT)NULL,
                           "gla_asynctest",5,8000,&thread_handle);
      if(rc != NCSCC_RC_SUCCESS)
      {
          m_TET_GLSV_PRINTF("failed to create thread\n");
          return ;
      }
   }

   gl_lck_thrd_cnt++;
   rc = m_NCS_TASK_START(thread_handle);
   if(rc != NCSCC_RC_SUCCESS)
   {
     m_TET_GLSV_PRINTF("failed to start thread\n");
     return ;
   }
}

/* ********* GLSV RESTORE FUNCTION ********** */

void glsv_restore_params(GLSV_RESTORE_OPT opt)
{
   switch(opt)
   {
      /* Restore - Initialization Version */

      case LCK_RESTORE_INIT_BAD_VERSION_T:
         glsv_fill_lck_version(&gl_gla_env.inv_params.inv_version,'C',0,1);
         break;

      case LCK_RESTORE_INIT_BAD_REL_CODE_T:
         glsv_fill_lck_version(&gl_gla_env.inv_params.inv_ver_bad_rel_code,'\0',1,0);
         break;

      case LCK_RESTORE_INIT_BAD_MAJOR_VER_T:
         glsv_fill_lck_version(&gl_gla_env.inv_params.inv_ver_not_supp,'B',3,0);
         break;
   }
}

/* *************** GLSV CLEANUP FUNCTIONS ***************** */

/* Cleanup Output parameters */

void glsv_clean_output_params()
{
   gl_gla_env.lck_hdl1 = 0;
   gl_gla_env.lck_hdl2 = 0;
   gl_gla_env.null_clbks_lck_hdl = 0;
   gl_gla_env.sel_obj = 0;
   gl_gla_env.res_hdl1 = 0;
   gl_gla_env.res_hdl2 = 0;
   gl_gla_env.res_hdl3 = 0;
   gl_gla_env.lck_status = 0;
}

/* Cleanup Callback parameters */

void glsv_clean_clbk_params()
{
   gl_gla_env.open_clbk_invo = 0;
   gl_gla_env.open_clbk_err = 0;
   gl_gla_env.open_clbk_res_hdl = 0;

   gl_gla_env.gr_clbk_invo = 0;
   gl_gla_env.gr_clbk_err = 0;
   gl_gla_env.gr_clbk_status = 0;

   gl_gla_env.waiter_sig = 0;
   gl_gla_env.waiter_clbk_lck_id = 0;
   gl_gla_env.waiter_clbk_mode_held = 0;
   gl_gla_env.waiter_clbk_mode_req = 0;

   gl_gla_env.unlck_clbk_invo = 0;
   gl_gla_env.unlck_clbk_err = 0;
}

/* Cleanup Initialization Handles */

void glsv_init_cleanup(GLSV_INIT_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case LCK_CLEAN_INIT_NULL_CBK_PARAM_T:
      case LCK_CLEAN_INIT_NULL_CBKS2_T:
      case LCK_CLEAN_INIT_SUCCESS_T:
         result = tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_INIT_SUCCESS_HDL2_T:
         result = tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_HDL2_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_INIT_NULL_CBKS_T:
         result = tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_HDL3_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_GLSV_PRINTF("\n+++++++++ FAILED - Cleaning up Initialization Handles +++++++\n\n");
}

void glsv_init_red_cleanup(GLSV_INIT_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case LCK_CLEAN_INIT_NULL_CBK_PARAM_T:
      case LCK_CLEAN_INIT_NULL_CBKS2_T:
      case LCK_CLEAN_INIT_SUCCESS_T:
         result = tet_test_red_lckFinalize(LCK_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_INIT_SUCCESS_HDL2_T:
         result = tet_test_red_lckFinalize(LCK_FINALIZE_SUCCESS_HDL2_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_INIT_NULL_CBKS_T:
         result = tet_test_red_lckFinalize(LCK_FINALIZE_SUCCESS_HDL3_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_GLSV_PRINTF("\n+++++++++ FAILED - Cleaning up Initialization Handles +++++++\n\n");
}

/* Cleanup Orphan locks */

void glsv_orphan_cleanup(GLSV_ORPHAN_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case LCK_CLEAN_RSC_LOCK_ORPHAN_PRLCK_T:
         result = tet_test_lckResourceUnlock(LCK_RSC_UNLOCK_LCKID2_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_RSC_LOCK_ORPHAN_EXLCK_T:
         result = tet_test_lckResourceUnlock(LCK_RSC_UNLOCK_LCKID1_SUCCESS_T,TEST_CLEANUP_MODE);
         break;
 
      case LCK_PURGE_RSC1_T:
         tet_test_lckInitialize(LCK_INIT_SUCCESS_T,TEST_CLEANUP_MODE);
         tet_test_lckResourceOpen(LCK_RESOURCE_OPEN_HDL1_NAME1_SUCCESS_T,TEST_CLEANUP_MODE);
         result = tet_test_lckLockPurge(LCK_LOCK_PURGE_SUCCESS_T,TEST_CLEANUP_MODE);
         tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_GLSV_PRINTF("\n+++++++++ FAILED - Cleaning up Orphan Locks +++++++\n\n");
}

void glsv_orphan_red_cleanup(GLSV_ORPHAN_CLEANUP_OPT opt)
{
   int result;

   switch(opt)
   {
      case LCK_CLEAN_RSC_LOCK_ORPHAN_PRLCK_T:
         result = tet_test_red_lckResourceUnlock(LCK_RSC_UNLOCK_LCKID2_SUCCESS_T,TEST_CLEANUP_MODE);
         break;

      case LCK_CLEAN_RSC_LOCK_ORPHAN_EXLCK_T:
         result = tet_test_red_lckResourceUnlock(LCK_RSC_UNLOCK_LCKID1_SUCCESS_T,TEST_CLEANUP_MODE);
         break;
 
      case LCK_PURGE_RSC1_T:
         tet_test_red_lckInitialize(LCK_INIT_SUCCESS_T,TEST_CLEANUP_MODE);
         tet_test_red_lckResourceOpen(LCK_RESOURCE_OPEN_HDL1_NAME1_SUCCESS_T,TEST_CLEANUP_MODE);
         result = tet_test_red_lckLockPurge(LCK_LOCK_PURGE_SUCCESS_T,TEST_CLEANUP_MODE);
         tet_test_red_lckFinalize(LCK_FINALIZE_SUCCESS_T,TEST_CLEANUP_MODE);
         break;
   }

   if(result != TET_PASS)
      m_TET_GLSV_PRINTF("\n+++++++++ FAILED - Cleaning up Orphan Locks +++++++\n\n");
}

#endif

