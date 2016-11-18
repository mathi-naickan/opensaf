/******************************************************************************************
#
# modification:
# 2008-11   Jony <jiangnan_hw@huawei.com>  Huawei Technologies CO., LTD.
#   1.  Provide test case tet_startup and tet_cleanup functions. The tet_startup function 
#       is defined as tet_glsv_startup(), as well as the tet_cleanup function is defined 
#       as tet_glsv_end().
#   2.  If "tetware-patch" is not used, change the name of test case array from 
#       glsv_onenode_testlist[] to tet_testlist[]. 
#   3.  If "tetware-patch" is not used, delete the third parameter of each item in this 
#       array. And add some functions to implement the function of those parameters.
#   4.  If "tetware-patch" is not used, delete tet_glsv_start_instance() function and parts 
#       of tet_run_glsv_app(), which invoke the functions in the patch.
#   5.  If "tetware-patch" is required, one can add a macro definition "TET_PATCH=1" in 
#       compilation process.
#
********************************************************************************************/


#include "tet_startup.h"
#include "ncs_main_papi.h"
#include "tet_glsv.h"

void tet_run_gld();
void tet_run_glnd();
void tet_run_glsv_app();

void tet_glsv_startup(void);
void tet_glsv_end(void);

void (*tet_startup)()=tet_glsv_startup;
void (*tet_cleanup)()=tet_glsv_end;

#if (TET_PATCH==1)

/* ******** TET_LIST Arrays ********* */

struct tet_testlist glsv_onenode_testlist[] = {
   {glsv_it_init_01,1},
   {glsv_it_init_02,2},
   {glsv_it_init_03,3},
   {glsv_it_init_04,4},
   {glsv_it_init_05,5},
   {glsv_it_init_06,6},
   {glsv_it_init_07,7},
   {glsv_it_init_08,8},
   {glsv_it_init_09,9},
   {glsv_it_init_10,10},

   {glsv_it_selobj_01,11},
   {glsv_it_selobj_02,12},
   {glsv_it_selobj_03,13},
   {glsv_it_selobj_04,14},
   {glsv_it_selobj_05,15},

   {glsv_it_option_chk_01,16},
   {glsv_it_option_chk_02,17},
   {glsv_it_option_chk_03,18},

   {glsv_it_dispatch_01,19},
   {glsv_it_dispatch_02,20},
   {glsv_it_dispatch_03,21},
   {glsv_it_dispatch_04,22},
   {glsv_it_dispatch_05,23},
   {glsv_it_dispatch_06,24},
   {glsv_it_dispatch_07,25},
   {glsv_it_dispatch_08,26},
   {glsv_it_dispatch_09,27},

   {glsv_it_finalize_01,28},
   {glsv_it_finalize_02,29},
   {glsv_it_finalize_03,30},
   {glsv_it_finalize_04,31},
   {glsv_it_finalize_05,32},
   {glsv_it_finalize_06,33},

   {glsv_it_res_open_01,34},
   {glsv_it_res_open_02,35},
   {glsv_it_res_open_03,36},
   {glsv_it_res_open_04,37},
   {glsv_it_res_open_05,38},
   {glsv_it_res_open_06,39},
   {glsv_it_res_open_07,40},
   {glsv_it_res_open_08,41},
   {glsv_it_res_open_09,42},
   {glsv_it_res_open_10,43},

   {glsv_it_res_open_async_01,44},
   {glsv_it_res_open_async_02,45},
   {glsv_it_res_open_async_03,46},
   {glsv_it_res_open_async_04,47},
   {glsv_it_res_open_async_05,48},
   {glsv_it_res_open_async_06,49},
   {glsv_it_res_open_async_07,50},
   {glsv_it_res_open_async_08,51},
   {glsv_it_res_open_async_09,52},
   {glsv_it_res_open_async_10,53},

   {glsv_it_res_close_01,54},
   {glsv_it_res_close_02,55},
   {glsv_it_res_close_03,56},
   {glsv_it_res_close_04,57},
   {glsv_it_res_close_05,58},
   {glsv_it_res_close_06,59},
   {glsv_it_res_close_07,60},
   {glsv_it_res_close_08,61},
   {glsv_it_res_close_09,62},
   {glsv_it_res_close_10,63},
   {glsv_it_res_close_11,64},

   {glsv_it_res_lck_01,65},
   {glsv_it_res_lck_02,66},
   {glsv_it_res_lck_03,67},
   {glsv_it_res_lck_04,68},
   {glsv_it_res_lck_05,69},
   {glsv_it_res_lck_06,70},
   {glsv_it_res_lck_07,71},
   {glsv_it_res_lck_08,72},
   {glsv_it_res_lck_09,73},
   {glsv_it_res_lck_10,74},
   {glsv_it_res_lck_11,75},
   {glsv_it_res_lck_12,76},
   {glsv_it_res_lck_13,77},
   {glsv_it_res_lck_14,78},
   {glsv_it_res_lck_15,79},
   {glsv_it_res_lck_16,80},
   {glsv_it_res_lck_17,81},
   {glsv_it_res_lck_18,82},

   {glsv_it_res_lck_async_01,83},
   {glsv_it_res_lck_async_02,84},
   {glsv_it_res_lck_async_03,85},
   {glsv_it_res_lck_async_04,86},
   {glsv_it_res_lck_async_05,87},
   {glsv_it_res_lck_async_06,88},
   {glsv_it_res_lck_async_07,89},
   {glsv_it_res_lck_async_08,90},
   {glsv_it_res_lck_async_09,91},
   {glsv_it_res_lck_async_10,92},
   {glsv_it_res_lck_async_11,93},
   {glsv_it_res_lck_async_12,94},
   {glsv_it_res_lck_async_13,95},
   {glsv_it_res_lck_async_14,96},
   {glsv_it_res_lck_async_15,97},
   {glsv_it_res_lck_async_16,98},
   {glsv_it_res_lck_async_17,99},
   {glsv_it_res_lck_async_18,100},
   {glsv_it_res_lck_async_19,101},
   {glsv_it_res_lck_async_20,102},

   {glsv_it_res_unlck_01,103},
   {glsv_it_res_unlck_02,104},
   {glsv_it_res_unlck_03,105},
   {glsv_it_res_unlck_04,106},
   {glsv_it_res_unlck_05,107},
   {glsv_it_res_unlck_06,108},
   {glsv_it_res_unlck_07,109},
   {glsv_it_res_unlck_08,110},

   {glsv_it_res_unlck_async_01,111},
   {glsv_it_res_unlck_async_02,112},
   {glsv_it_res_unlck_async_03,113},
   {glsv_it_res_unlck_async_04,114},
   {glsv_it_res_unlck_async_05,115},
   {glsv_it_res_unlck_async_06,116},
   {glsv_it_res_unlck_async_07,117},
   {glsv_it_res_unlck_async_08,118},
   {glsv_it_res_unlck_async_09,119},
   {glsv_it_res_unlck_async_10,120},

   {glsv_it_lck_purge_01,121},
   {glsv_it_lck_purge_02,122},
   {glsv_it_lck_purge_03,123},
   {glsv_it_lck_purge_04,124},
   {glsv_it_lck_purge_05,125},

   {glsv_it_res_cr_del_01,126},
   {glsv_it_res_cr_del_02,127},
   {glsv_it_res_cr_del_03,128},
   {glsv_it_res_cr_del_04,129},
   {glsv_it_res_cr_del_05,130},
   {glsv_it_res_cr_del_06,131,0},
   {glsv_it_res_cr_del_06,132,1},

   {glsv_it_lck_modes_wt_clbk_01,133,0},
   {glsv_it_lck_modes_wt_clbk_01,134,1},
   {glsv_it_lck_modes_wt_clbk_02,135,0},
   {glsv_it_lck_modes_wt_clbk_02,136,1},
   {glsv_it_lck_modes_wt_clbk_03,137,0},
   {glsv_it_lck_modes_wt_clbk_03,138,1},
   {glsv_it_lck_modes_wt_clbk_04,139,0},
   {glsv_it_lck_modes_wt_clbk_04,140,1},
   {glsv_it_lck_modes_wt_clbk_05,141},
   {glsv_it_lck_modes_wt_clbk_06,142},
   {glsv_it_lck_modes_wt_clbk_07,143},
   {glsv_it_lck_modes_wt_clbk_08,144},
   {glsv_it_lck_modes_wt_clbk_09,145},

   {glsv_it_ddlcks_orplks_01,146},
   {glsv_it_ddlcks_orplks_02,147,0},
   {glsv_it_ddlcks_orplks_02,148,1},
   {glsv_it_ddlcks_orplks_03,149,0},
   {glsv_it_ddlcks_orplks_03,150,1},
   {glsv_it_ddlcks_orplks_04,151,0},
   {glsv_it_ddlcks_orplks_04,152,1},
   {glsv_it_ddlcks_orplks_05,153},
   {glsv_it_ddlcks_orplks_06,154},
   {glsv_it_ddlcks_orplks_07,155,0},
   {glsv_it_ddlcks_orplks_07,156,1},

   {glsv_it_lck_strip_purge_01,157},
   {glsv_it_lck_strip_purge_02,158},
   {glsv_it_lck_strip_purge_03,159},
   {NULL,-1}
};

void tet_glsv_start_instance(TET_GLSV_INST *inst)
{
   int iter;
   int num_of_iter = 1;
   int test_case_num = -1;
   struct tet_testlist *glsv_testlist = glsv_onenode_testlist;

   if(inst->tetlist_index == GLSV_ONE_NODE_LIST)
      glsv_testlist = glsv_onenode_testlist;

   if(inst->num_of_iter)
      num_of_iter = inst->num_of_iter;

   if(inst->test_case_num)
      test_case_num = inst->test_case_num;

   for(iter = 0; iter < num_of_iter; iter++)
      tet_test_start(test_case_num,glsv_testlist);
}


#else

/* ******** Function modification  ********* */
void glsv_it_res_cr_del_06_01()
{
   glsv_it_res_cr_del_06(0);
}

void glsv_it_res_cr_del_06_02()
{
   glsv_it_res_cr_del_06(1);
}

void glsv_it_lck_modes_wt_clbk_01_01()
{
   glsv_it_lck_modes_wt_clbk_01(0);
}

void glsv_it_lck_modes_wt_clbk_01_02()
{
   glsv_it_lck_modes_wt_clbk_01(1);
}

void glsv_it_lck_modes_wt_clbk_02_01()
{
   glsv_it_lck_modes_wt_clbk_02(0);
}

void glsv_it_lck_modes_wt_clbk_02_02()
{
   glsv_it_lck_modes_wt_clbk_02(1);
}

void glsv_it_lck_modes_wt_clbk_03_01()
{
   glsv_it_lck_modes_wt_clbk_03(0);
}

void glsv_it_lck_modes_wt_clbk_03_02()
{
   glsv_it_lck_modes_wt_clbk_03(1);
}

void glsv_it_lck_modes_wt_clbk_04_01()
{
   glsv_it_lck_modes_wt_clbk_04(0);
}

void glsv_it_lck_modes_wt_clbk_04_02()
{
   glsv_it_lck_modes_wt_clbk_04(1);
}

void glsv_it_ddlcks_orplks_02_01()
{
   glsv_it_ddlcks_orplks_02(0);
}

void glsv_it_ddlcks_orplks_02_02()
{
   glsv_it_ddlcks_orplks_02(1);
}

void glsv_it_ddlcks_orplks_03_01()
{
   glsv_it_ddlcks_orplks_03(0);
}

void glsv_it_ddlcks_orplks_03_02()
{
   glsv_it_ddlcks_orplks_03(1);
}

void glsv_it_ddlcks_orplks_04_01()
{
   glsv_it_ddlcks_orplks_04(0);
}

void glsv_it_ddlcks_orplks_04_02()
{
   glsv_it_ddlcks_orplks_04(1);
}

void glsv_it_ddlcks_orplks_07_01()
{
   glsv_it_ddlcks_orplks_07(0);
}

void glsv_it_ddlcks_orplks_07_02()
{
   glsv_it_ddlcks_orplks_07(1);
}

/* ******** TET_LIST Arrays ********* */
struct tet_testlist tet_testlist[] = {
   {glsv_it_init_01,1},
   {glsv_it_init_02,2},
   {glsv_it_init_03,3},
   {glsv_it_init_04,4},
   {glsv_it_init_05,5},
   {glsv_it_init_06,6},
   {glsv_it_init_07,7},
   {glsv_it_init_08,8},
   {glsv_it_init_09,9},
   {glsv_it_init_10,10},

   {glsv_it_selobj_01,11},
   {glsv_it_selobj_02,12},
   {glsv_it_selobj_03,13},
   {glsv_it_selobj_04,14},
   {glsv_it_selobj_05,15},

   {glsv_it_option_chk_01,16},
   {glsv_it_option_chk_02,17},
   {glsv_it_option_chk_03,18},

   {glsv_it_dispatch_01,19},
   {glsv_it_dispatch_02,20},
   {glsv_it_dispatch_03,21},
   {glsv_it_dispatch_04,22},
   {glsv_it_dispatch_05,23},
   {glsv_it_dispatch_06,24},
   {glsv_it_dispatch_07,25},
   {glsv_it_dispatch_08,26},
   {glsv_it_dispatch_09,27},

   {glsv_it_finalize_01,28},
   {glsv_it_finalize_02,29},
   {glsv_it_finalize_03,30},
   {glsv_it_finalize_04,31},
   {glsv_it_finalize_05,32},
   {glsv_it_finalize_06,33},

   {glsv_it_res_open_01,34},
   {glsv_it_res_open_02,35},
   {glsv_it_res_open_03,36},
   {glsv_it_res_open_04,37},
   {glsv_it_res_open_05,38},
   {glsv_it_res_open_06,39},
   {glsv_it_res_open_07,40},
   {glsv_it_res_open_08,41},
   {glsv_it_res_open_09,42},
   {glsv_it_res_open_10,43},

   {glsv_it_res_open_async_01,44},
   {glsv_it_res_open_async_02,45},
   {glsv_it_res_open_async_03,46},
   {glsv_it_res_open_async_04,47},
   {glsv_it_res_open_async_05,48},
   {glsv_it_res_open_async_06,49},
   {glsv_it_res_open_async_07,50},
   {glsv_it_res_open_async_08,51},
   {glsv_it_res_open_async_09,52},
   {glsv_it_res_open_async_10,53},

   {glsv_it_res_close_01,54},
   {glsv_it_res_close_02,55},
   {glsv_it_res_close_03,56},
   {glsv_it_res_close_04,57},
   {glsv_it_res_close_05,58},
   {glsv_it_res_close_06,59},
   {glsv_it_res_close_07,60},
   {glsv_it_res_close_08,61},
   {glsv_it_res_close_09,62},
   {glsv_it_res_close_10,63},
   {glsv_it_res_close_11,64},

   {glsv_it_res_lck_01,65},
   {glsv_it_res_lck_02,66},
   {glsv_it_res_lck_03,67},
   {glsv_it_res_lck_04,68},
   {glsv_it_res_lck_05,69},
   {glsv_it_res_lck_06,70},
   {glsv_it_res_lck_07,71},
   {glsv_it_res_lck_08,72},
   {glsv_it_res_lck_09,73},
   {glsv_it_res_lck_10,74},
   {glsv_it_res_lck_11,75},
   {glsv_it_res_lck_12,76},
   {glsv_it_res_lck_13,77},
   {glsv_it_res_lck_14,78},
   {glsv_it_res_lck_15,79},
   {glsv_it_res_lck_16,80},
   {glsv_it_res_lck_17,81},
   {glsv_it_res_lck_18,82},

   {glsv_it_res_lck_async_01,83},
   {glsv_it_res_lck_async_02,84},
   {glsv_it_res_lck_async_03,85},
   {glsv_it_res_lck_async_04,86},
   {glsv_it_res_lck_async_05,87},
   {glsv_it_res_lck_async_06,88},
   {glsv_it_res_lck_async_07,89},
   {glsv_it_res_lck_async_08,90},
   {glsv_it_res_lck_async_09,91},
   {glsv_it_res_lck_async_10,92},
   {glsv_it_res_lck_async_11,93},
   {glsv_it_res_lck_async_12,94},
   {glsv_it_res_lck_async_13,95},
   {glsv_it_res_lck_async_14,96},
   {glsv_it_res_lck_async_15,97},
   {glsv_it_res_lck_async_16,98},
   {glsv_it_res_lck_async_17,99},
   {glsv_it_res_lck_async_18,100},
   {glsv_it_res_lck_async_19,101},
   {glsv_it_res_lck_async_20,102},

   {glsv_it_res_unlck_01,103},
   {glsv_it_res_unlck_02,104},
   {glsv_it_res_unlck_03,105},
   {glsv_it_res_unlck_04,106},
   {glsv_it_res_unlck_05,107},
   {glsv_it_res_unlck_06,108},
   {glsv_it_res_unlck_07,109},
   {glsv_it_res_unlck_08,110},

   {glsv_it_res_unlck_async_01,111},
   {glsv_it_res_unlck_async_02,112},
   {glsv_it_res_unlck_async_03,113},
   {glsv_it_res_unlck_async_04,114},
   {glsv_it_res_unlck_async_05,115},
   {glsv_it_res_unlck_async_06,116},
   {glsv_it_res_unlck_async_07,117},
   {glsv_it_res_unlck_async_08,118},
   {glsv_it_res_unlck_async_09,119},
   {glsv_it_res_unlck_async_10,120},

   {glsv_it_lck_purge_01,121},
   {glsv_it_lck_purge_02,122},
   {glsv_it_lck_purge_03,123},
   {glsv_it_lck_purge_04,124},
   {glsv_it_lck_purge_05,125},

   {glsv_it_res_cr_del_01,126},
   {glsv_it_res_cr_del_02,127},
   {glsv_it_res_cr_del_03,128},
   {glsv_it_res_cr_del_04,129},
   {glsv_it_res_cr_del_05,130},
   {glsv_it_res_cr_del_06_01,131},
   {glsv_it_res_cr_del_06_02,132},

   {glsv_it_lck_modes_wt_clbk_01_01,133},
   {glsv_it_lck_modes_wt_clbk_01_02,134},
   {glsv_it_lck_modes_wt_clbk_02_01,135},
   {glsv_it_lck_modes_wt_clbk_02_02,136},
   {glsv_it_lck_modes_wt_clbk_03_01,137},
   {glsv_it_lck_modes_wt_clbk_03_02,138},
   {glsv_it_lck_modes_wt_clbk_04_01,139},
   {glsv_it_lck_modes_wt_clbk_04_02,140},
   {glsv_it_lck_modes_wt_clbk_05,141},
   {glsv_it_lck_modes_wt_clbk_06,142},
   {glsv_it_lck_modes_wt_clbk_07,143},
   {glsv_it_lck_modes_wt_clbk_08,144},
   {glsv_it_lck_modes_wt_clbk_09,145},

   {glsv_it_ddlcks_orplks_01,146},
   {glsv_it_ddlcks_orplks_02_01,147},
   {glsv_it_ddlcks_orplks_02_02,148},
   {glsv_it_ddlcks_orplks_03_01,149},
   {glsv_it_ddlcks_orplks_03_02,150},
   {glsv_it_ddlcks_orplks_04_01,151},
   {glsv_it_ddlcks_orplks_04_02,152},
   {glsv_it_ddlcks_orplks_05,153},
   {glsv_it_ddlcks_orplks_06,154},
   {glsv_it_ddlcks_orplks_07_01,155},
   {glsv_it_ddlcks_orplks_07_02,156},

   {glsv_it_lck_strip_purge_01,157},
   {glsv_it_lck_strip_purge_02,158},
   {glsv_it_lck_strip_purge_03,159},

   {NULL,-1}
};


#endif

void tet_run_glsv_app()
{
   TET_GLSV_INST inst;

   tet_glsv_get_inputs(&inst);
   init_glsv_test_env();
   tet_glsv_fill_inputs(&inst);

#if (TET_PATCH==1)

#ifndef TET_ALL

#ifdef TET_SMOKE_TEST
#endif


   tet_glsv_start_instance(&inst);

   m_TET_GLSV_PRINTF("\n ##### End of Testcase #####\n");
   printf("\n PRESS ENTER TO GET THE MEM DUMP\n");
   getchar();

   sleep(5);

   tet_test_cleanup();

#else

   while(1)
   {
      tet_test_lckInitialize(LCK_INIT_SUCCESS_T,TEST_CONFIG_MODE);
      tet_test_lckInitialize(LCK_INIT_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
      glsv_createthread_all_loop(1);
      glsv_createthread_all_loop(2);
      tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);
      tet_test_lckFinalize(LCK_FINALIZE_SUCCESS_HDL2_T,TEST_CONFIG_MODE);
     
      m_TET_GLSV_PRINTF ("\n\n ******  Starting the API testcases ****** \n");
      tet_test_start(-1,glsv_onenode_testlist);
      m_TET_GLSV_PRINTF ("\n ****** Ending the API testcases ****** \n");
   }
   
#endif

#endif
}

void tet_run_glsv_dist_cases()
{

}



void tet_glsv_startup(void) 
{
/* Using the default mode for startup */
    ncs_agents_startup(); 

#if (TET_D == 1)
   tet_run_gld();
#endif

#if (TET_ND == 1)
   tet_run_glnd();
#endif

#if (TET_A == 1)
   tet_run_glsv_app();
#endif

}

void tet_glsv_end()
{
  tet_infoline(" Ending the agony.. ");
}
