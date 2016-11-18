/********************************************************************************************
#
# modification:
# 2008-11   Jony <jiangnan_hw@huawei.com>  Huawei Technologies CO., LTD.
#  1.  Provide test case tet_startup and tet_cleanup functions. The tet_startup function 
#      is defined as tet_mqsv_startup(), as well as the tet_cleanup function is defined 
#      as tet_mqsv_end().
#  2.  If "tetware-patch" is not used, change the name of test case array from 
#      mqsv_onenode_testlist[] to tet_testlist[]. 
#  3.  If "tetware-patch" is not used, delete the third parameter of each item in this array. 
#      And add some functions to implement the function of those parameters.
#  4.  If "tetware-patch" is not used, delete parts of tet_run_mqsv_app () function, 
#      which invoke the functions in the patch.
#  5.  If "tetware-patch" is required, one can add a macro definition "TET_PATCH=1" in 
#      compilation process.
#
*********************************************************************************************/



#include "tet_startup.h"
#include "ncs_main_papi.h"
#include "tet_mqsv.h"

void tet_create_mqd(void);
void tet_create_mqnd(void);
void tet_run_mqsv_app(void);

void tet_mqsv_startup(void);
void tet_mqsv_cleanup(void);

void (*tet_startup)()=tet_mqsv_startup;
void (*tet_cleanup)()=tet_mqsv_cleanup;


/******** TET_LIST Arrays *********/

#if (TET_PATCH==1)
struct tet_testlist mqsv_onenode_testlist[] = {
#else
struct tet_testlist tet_testlist[]={
#endif
   {mqsv_it_init_01,1},
   {mqsv_it_init_02,2},
   {mqsv_it_init_03,3},
   {mqsv_it_init_04,4},
   {mqsv_it_init_05,5},
   {mqsv_it_init_06,6},
   {mqsv_it_init_07,7},
   {mqsv_it_init_08,8},
   {mqsv_it_init_09,9},
   {mqsv_it_init_10,10},

   {mqsv_it_selobj_01,11},
   {mqsv_it_selobj_02,12},
   {mqsv_it_selobj_03,13},
   {mqsv_it_selobj_04,14},

   {mqsv_it_dispatch_01,15},
   {mqsv_it_dispatch_02,16},
   {mqsv_it_dispatch_03,17},
   {mqsv_it_dispatch_04,18},
   {mqsv_it_dispatch_05,19},
   {mqsv_it_dispatch_06,20},
   {mqsv_it_dispatch_07,21},
   {mqsv_it_dispatch_08,22},
   {mqsv_it_dispatch_09,23},

   {mqsv_it_finalize_01,24},
   {mqsv_it_finalize_02,25},
   {mqsv_it_finalize_03,26},
   {mqsv_it_finalize_04,27},
   {mqsv_it_finalize_05,28},
   {mqsv_it_finalize_06,29},

   {mqsv_it_qopen_01,30},
   {mqsv_it_qopen_02,31},
   {mqsv_it_qopen_03,32},
   {mqsv_it_qopen_04,33},
   {mqsv_it_qopen_05,34},
   {mqsv_it_qopen_06,35},
   {mqsv_it_qopen_07,36},
   {mqsv_it_qopen_08,37},
   {mqsv_it_qopen_09,38},
   {mqsv_it_qopen_10,39},
   {mqsv_it_qopen_11,40},
   {mqsv_it_qopen_12,41},
   {mqsv_it_qopen_13,42},
   {mqsv_it_qopen_14,43},
   {mqsv_it_qopen_15,44},
   {mqsv_it_qopen_16,45},
   {mqsv_it_qopen_17,46},
   {mqsv_it_qopen_18,47},
   {mqsv_it_qopen_19,48},
   {mqsv_it_qopen_20,49},
   {mqsv_it_qopen_21,50},
   {mqsv_it_qopen_22,51},
   {mqsv_it_qopen_23,52},
   {mqsv_it_qopen_24,53},
   {mqsv_it_qopen_25,54},
   {mqsv_it_qopen_26,55},
   {mqsv_it_qopen_27,56},
   {mqsv_it_qopen_28,57},
   {mqsv_it_qopen_29,58},
   {mqsv_it_qopen_30,59},
   {mqsv_it_qopen_31,60},
   {mqsv_it_qopen_32,61},
   {mqsv_it_qopen_33,62},
   {mqsv_it_qopen_34,63},
   {mqsv_it_qopen_35,64},
   {mqsv_it_qopen_36,65},
   {mqsv_it_qopen_37,66},
   {mqsv_it_qopen_38,67},
   {mqsv_it_qopen_39,68},
   {mqsv_it_qopen_40,69},
   {mqsv_it_qopen_41,70},
   {mqsv_it_qopen_42,71},
   {mqsv_it_qopen_43,72},
   {mqsv_it_qopen_44,73},
   {mqsv_it_qopen_45,74},
   {mqsv_it_qopen_46,75},
   {mqsv_it_qopen_47,76},
   {mqsv_it_qopen_48,77},
   {mqsv_it_qopen_49,78},
   {mqsv_it_qopen_50,79},
   {mqsv_it_qopen_51,80},

   {mqsv_it_close_01,81},
   {mqsv_it_close_02,82},
   {mqsv_it_close_03,83},
   {mqsv_it_close_04,84},
   {mqsv_it_close_05,85},
   {mqsv_it_close_06,86},
   {mqsv_it_close_07,87},

   {mqsv_it_qstatus_01,88},
   {mqsv_it_qstatus_02,89},
   {mqsv_it_qstatus_03,90},
   {mqsv_it_qstatus_04,91},
   {mqsv_it_qstatus_05,92},
   {mqsv_it_qstatus_06,93},
   {mqsv_it_qstatus_07,94},

   {mqsv_it_qunlink_01,95},
   {mqsv_it_qunlink_02,96},
   {mqsv_it_qunlink_03,97},
   {mqsv_it_qunlink_04,98},
   {mqsv_it_qunlink_05,99},
   {mqsv_it_qunlink_06,100},
   {mqsv_it_qunlink_07,101},

   {mqsv_it_qgrp_create_01,102},
   {mqsv_it_qgrp_create_02,103},
   {mqsv_it_qgrp_create_03,104},
   {mqsv_it_qgrp_create_04,105},
   {mqsv_it_qgrp_create_05,106},
   {mqsv_it_qgrp_create_06,107},
   {mqsv_it_qgrp_create_07,108},
   {mqsv_it_qgrp_create_08,109},
   {mqsv_it_qgrp_create_09,110},
   {mqsv_it_qgrp_create_10,111},
   {mqsv_it_qgrp_create_11,112},

   {mqsv_it_qgrp_insert_01,113},
   {mqsv_it_qgrp_insert_02,114},
   {mqsv_it_qgrp_insert_03,115},
   {mqsv_it_qgrp_insert_04,116},
   {mqsv_it_qgrp_insert_05,117},
   {mqsv_it_qgrp_insert_06,118},
   {mqsv_it_qgrp_insert_07,119},
   {mqsv_it_qgrp_insert_08,120},
   {mqsv_it_qgrp_insert_09,121},

   {mqsv_it_qgrp_remove_01,122},
   {mqsv_it_qgrp_remove_02,123},
   {mqsv_it_qgrp_remove_03,124},
   {mqsv_it_qgrp_remove_04,125},
   {mqsv_it_qgrp_remove_05,126},
   {mqsv_it_qgrp_remove_06,127},
   {mqsv_it_qgrp_remove_07,128},

   {mqsv_it_qgrp_delete_01,129},
   {mqsv_it_qgrp_delete_02,130},
   {mqsv_it_qgrp_delete_03,131},
   {mqsv_it_qgrp_delete_04,132},
   {mqsv_it_qgrp_delete_05,133},
   {mqsv_it_qgrp_delete_06,134},
   {mqsv_it_qgrp_delete_07,135},

   {mqsv_it_qgrp_track_01,136},
   {mqsv_it_qgrp_track_02,137},
   {mqsv_it_qgrp_track_03,138},
   {mqsv_it_qgrp_track_04,139},
   {mqsv_it_qgrp_track_05,140},
   {mqsv_it_qgrp_track_06,141},
   {mqsv_it_qgrp_track_07,142},
   {mqsv_it_qgrp_track_08,143},
   {mqsv_it_qgrp_track_09,144},
   {mqsv_it_qgrp_track_10,145},
   {mqsv_it_qgrp_track_11,146},
   {mqsv_it_qgrp_track_12,147},
   {mqsv_it_qgrp_track_13,148},
   {mqsv_it_qgrp_track_14,149},
   {mqsv_it_qgrp_track_15,150},
   {mqsv_it_qgrp_track_16,151},
   {mqsv_it_qgrp_track_17,152},
   {mqsv_it_qgrp_track_18,153},
   {mqsv_it_qgrp_track_19,154},
   {mqsv_it_qgrp_track_20,155},
   {mqsv_it_qgrp_track_21,156},
   {mqsv_it_qgrp_track_22,157},
   {mqsv_it_qgrp_track_23,158},
   {mqsv_it_qgrp_track_24,159},
   {mqsv_it_qgrp_track_25,160},
   {mqsv_it_qgrp_track_26,161},
   {mqsv_it_qgrp_track_27,162},
   {mqsv_it_qgrp_track_28,163},
   {mqsv_it_qgrp_track_29,164},
   {mqsv_it_qgrp_track_30,165},
   {mqsv_it_qgrp_track_31,166},
   {mqsv_it_qgrp_track_32,167},
   {mqsv_it_qgrp_track_33,168},

   {mqsv_it_qgrp_track_stop_01,169},
   {mqsv_it_qgrp_track_stop_02,170},
   {mqsv_it_qgrp_track_stop_03,171},
   {mqsv_it_qgrp_track_stop_04,172},
   {mqsv_it_qgrp_track_stop_05,173},
   {mqsv_it_qgrp_track_stop_06,174},
   {mqsv_it_qgrp_track_stop_07,175},

   {mqsv_it_msg_send_01,176},
   {mqsv_it_msg_send_02,177},
   {mqsv_it_msg_send_03,178},
   {mqsv_it_msg_send_04,179},
   {mqsv_it_msg_send_05,180},
   {mqsv_it_msg_send_06,181},
   {mqsv_it_msg_send_07,182},
   {mqsv_it_msg_send_08,183},
   {mqsv_it_msg_send_09,184},
   {mqsv_it_msg_send_10,185},
   {mqsv_it_msg_send_11,186},
   {mqsv_it_msg_send_12,187},
   {mqsv_it_msg_send_13,188},
   {mqsv_it_msg_send_14,189},
   {mqsv_it_msg_send_15,190},
   {mqsv_it_msg_send_16,191},
   {mqsv_it_msg_send_17,192},
   {mqsv_it_msg_send_18,193},
   {mqsv_it_msg_send_19,194},
   {mqsv_it_msg_send_20,195},
   {mqsv_it_msg_send_21,196},
   {mqsv_it_msg_send_22,197},
   {mqsv_it_msg_send_23,198},
   {mqsv_it_msg_send_24,199},
   {mqsv_it_msg_send_25,200},
   {mqsv_it_msg_send_26,201},
   {mqsv_it_msg_send_27,202},
   {mqsv_it_msg_send_28,203},
   {mqsv_it_msg_send_29,204},
   {mqsv_it_msg_send_30,205},
   {mqsv_it_msg_send_31,206},
   {mqsv_it_msg_send_32,207},
   {mqsv_it_msg_send_33,208},
   {mqsv_it_msg_send_34,209},
   {mqsv_it_msg_send_35,210},
   {mqsv_it_msg_send_36,211},
   {mqsv_it_msg_send_37,212},
   {mqsv_it_msg_send_38,213},
   {mqsv_it_msg_send_39,214},
   {mqsv_it_msg_send_40,215},
   {mqsv_it_msg_send_41,216},
   {mqsv_it_msg_send_42,217},
   {mqsv_it_msg_send_43,218},

   {mqsv_it_msg_get_01,219},
   {mqsv_it_msg_get_02,220},
   {mqsv_it_msg_get_03,221},
   {mqsv_it_msg_get_04,222},
   {mqsv_it_msg_get_05,223},
   {mqsv_it_msg_get_06,224},
   {mqsv_it_msg_get_07,225},
   {mqsv_it_msg_get_08,226},
   {mqsv_it_msg_get_09,227},
   {mqsv_it_msg_get_10,228},
   {mqsv_it_msg_get_11,229},
   {mqsv_it_msg_get_12,230},
   {mqsv_it_msg_get_13,231},
   {mqsv_it_msg_get_14,232},
   {mqsv_it_msg_get_15,233},
   {mqsv_it_msg_get_16,234},
   {mqsv_it_msg_get_17,235},
   {mqsv_it_msg_get_18,236},
   {mqsv_it_msg_get_19,237},
   {mqsv_it_msg_get_20,238},
   {mqsv_it_msg_get_21,239},
   {mqsv_it_msg_get_22,240},

   {mqsv_it_msg_cancel_01,241},
   {mqsv_it_msg_cancel_02,242},
   {mqsv_it_msg_cancel_03,243},
   {mqsv_it_msg_cancel_04,244},

   {mqsv_it_msg_sendrcv_01,245},
   {mqsv_it_msg_sendrcv_02,246},
   {mqsv_it_msg_sendrcv_03,247},
   {mqsv_it_msg_sendrcv_04,248},
   {mqsv_it_msg_sendrcv_05,249},
   {mqsv_it_msg_sendrcv_06,250},
   {mqsv_it_msg_sendrcv_07,251},
   {mqsv_it_msg_sendrcv_08,252},
   {mqsv_it_msg_sendrcv_09,253},
   {mqsv_it_msg_sendrcv_10,254},
   {mqsv_it_msg_sendrcv_11,255},
   {mqsv_it_msg_sendrcv_12,256},
   {mqsv_it_msg_sendrcv_13,257},
   {mqsv_it_msg_sendrcv_14,258},
   {mqsv_it_msg_sendrcv_15,259},
   {mqsv_it_msg_sendrcv_16,260},
   {mqsv_it_msg_sendrcv_17,261},
   {mqsv_it_msg_sendrcv_18,262},
   {mqsv_it_msg_sendrcv_19,263},
   {mqsv_it_msg_sendrcv_20,264},
   {mqsv_it_msg_sendrcv_21,265},
   {mqsv_it_msg_sendrcv_22,266},
   {mqsv_it_msg_sendrcv_23,267},
   {mqsv_it_msg_sendrcv_24,268},
   {mqsv_it_msg_sendrcv_25,269},
   {mqsv_it_msg_sendrcv_26,270},

   {mqsv_it_msg_reply_01,271},
   {mqsv_it_msg_reply_02,272},
   {mqsv_it_msg_reply_03,273},
   {mqsv_it_msg_reply_04,274},
   {mqsv_it_msg_reply_05,275},
   {mqsv_it_msg_reply_06,276},
   {mqsv_it_msg_reply_07,277},
   {mqsv_it_msg_reply_08,278},
   {mqsv_it_msg_reply_09,279},
   {mqsv_it_msg_reply_10,280},
   {mqsv_it_msg_reply_11,281},
   {mqsv_it_msg_reply_12,282},
   {mqsv_it_msg_reply_13,283},
   {mqsv_it_msg_reply_14,284},
   {mqsv_it_msg_reply_15,285},
   {mqsv_it_msg_reply_16,286},
   {mqsv_it_msg_reply_17,287},
   {mqsv_it_msg_reply_18,288},
   {mqsv_it_msg_reply_19,289},
   {mqsv_it_msg_reply_20,290},
   {mqsv_it_msg_reply_21,291},
   {mqsv_it_msg_reply_22,292},
   {mqsv_it_msg_reply_23,293},
   {mqsv_it_msg_reply_24,294},

   {mqsv_it_msgqs_01,295},
   {mqsv_it_msgqs_02,296},
   {mqsv_it_msgqs_03,297},
   {mqsv_it_msgqs_04,298},
   {mqsv_it_msgqs_05,299},
   {mqsv_it_msgqs_06,300},
   {mqsv_it_msgqs_07,301},
   {mqsv_it_msgqs_08,302},
   {mqsv_it_msgqs_09,303},
   {mqsv_it_msgqs_10,304},
   {mqsv_it_msgqs_11,305},
   {mqsv_it_msgqs_12,306},
   {mqsv_it_msgqs_13,307},
   {mqsv_it_msgqs_14,308},
   {mqsv_it_msgqs_15,309},
   {mqsv_it_msgqs_16,310},
   {mqsv_it_msgqs_17,311},
   {mqsv_it_msgqs_18,312},
   {mqsv_it_msgqs_19,313},
   {mqsv_it_msgqs_20,314},
   {mqsv_it_msgqs_21,315},
   {mqsv_it_msgqs_22,316},
   {mqsv_it_msgqs_23,317},

   {mqsv_it_msgq_grps_01,318},
   {mqsv_it_msgq_grps_02,319},
   {mqsv_it_msgq_grps_03,320},
   {mqsv_it_msgq_grps_04,321},
   {mqsv_it_msgq_grps_05,322},
   {mqsv_it_msgq_grps_06,323},
   {mqsv_it_msgq_grps_07,324},
   {mqsv_it_msgq_grps_08,325},
   {mqsv_it_msgq_grps_09,326},
   {mqsv_it_msgq_grps_10,327},
   {mqsv_it_msgq_grps_11,328},
   {mqsv_it_msgq_grps_12,329},

   {mqsv_it_msg_delprop_01,330},
   {mqsv_it_msg_delprop_02,331},
   {mqsv_it_msg_delprop_03,332},
   {mqsv_it_msg_delprop_04,333},
   {mqsv_it_msg_delprop_05,334},
   {mqsv_it_msg_delprop_06,335},
   {mqsv_it_msg_delprop_07,336},
   {mqsv_it_msg_delprop_08,337},
   {mqsv_it_msg_delprop_09,338},
   {mqsv_it_msg_delprop_10,339},
   {mqsv_it_msg_delprop_11,340},
   {mqsv_it_msg_delprop_12,341},
   {mqsv_it_msg_delprop_13,342},
   {NULL,-1}
};

#if (TET_PATCH==1)

void tet_mqsv_start_instance(TET_MQSV_INST *inst)
{
   int iter;
   int num_of_iter = 1;
   int test_case_num = -1;
   struct tet_testlist *mqsv_testlist = mqsv_onenode_testlist;

   if(inst->tetlist_index == MQSV_ONE_NODE_LIST)
      mqsv_testlist = mqsv_onenode_testlist;

   if(inst->num_of_iter)
      num_of_iter = inst->num_of_iter;

   if(inst->test_case_num)
      test_case_num = inst->test_case_num;

   for(iter = 0; iter < num_of_iter; iter++)
      tet_test_start(test_case_num,mqsv_testlist);
}
#endif

void tet_run_mqsv_app()
{
   TET_MQSV_INST inst;

   tet_mqsv_get_inputs(&inst);
   init_mqsv_test_env();
   tet_mqsv_fill_inputs(&inst);

#if (TET_PATCH==1)
#ifndef TET_ALL 

   tet_mqsv_start_instance(&inst);

   m_TET_MQSV_PRINTF("\n ##### End of Testlist #####\n");
   printf("\n MEM DUMP\n");

   ncs_tmr_whatsout();
   sleep(5);

   tet_test_cleanup();

#else

   tet_test_msgInitialize(MSG_INIT_SUCCESS_T,TEST_CONFIG_MODE);
   mqsv_createthread_all_loop();
   tet_test_msgFinalize(MSG_FINALIZE_SUCCESS_T,TEST_CONFIG_MODE);

   while(1)
   {
      m_TET_MQSV_PRINTF ("\n\n*******  Starting MQSv testcases *******\n");
      tet_test_start(-1,mqsv_onenode_testlist);
      m_TET_MQSV_PRINTF ("\n\n*******  Ending MQSv testcases *******\n");
   }
#endif

#endif
}

void tet_run_mqsv_dist_cases()
{

}

void tet_mqsv_startup(void) 
{
/* Using the default mode for startup */
    ncs_agents_startup(); 

#if (TET_D == 1)
   tet_create_mqd();
#endif

#if (TET_ND == 1)
   tet_create_mqnd();
#endif

#if (TET_A == 1)
   tet_run_mqsv_app();
#endif
}

void tet_mqsv_cleanup()
{
  tet_infoline(" Ending the agony.. ");
}
