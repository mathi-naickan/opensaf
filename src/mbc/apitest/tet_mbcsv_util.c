/*******************************************************************************************
#
# modification:
# 2008-11   Jony <jiangnan_hw@huawei.com>  Huawei Technologies CO., LTD.
#  1.  Provide test case tet_startup and tet_cleanup functions. The tet_startup function
#      is defined as tet_mbcsv_startup(), as well as the tet_cleanup function is defined 
#      as tet_mbcsv_end().
#  2.  If "tetware-patch" is not used, change the name of test case array from api_test[]
#      to tet_testlist[]. 
#  3.  If "tetware-patch" is not used, delete the third parameter of each item in this array. 
#      And add some functions to implement the function of those parameters.
#  4.  If "tetware-patch" is not used, delete parts of mbcstm_startup() function, which 
#      invoke the functions in the patch.
#  5.  If "tetware-patch" is required, one can add a macro definition "TET_PATCH=1" in 
#      compilation process.
#
********************************************************************************************/



#include "tet_startup.h"
#include "ncs_main_papi.h"
#include "mbcsv_api.h"

uint32_t mbcstm_startup(void);

void tet_mbcsv_startup(void);
void tet_mbcsv_cleanup(void);

void (*tet_startup)()=tet_mbcsv_startup;
void (*tet_cleanup)()=tet_mbcsv_cleanup;


#if (TET_PATCH==1)

struct tet_testlist api_testlist[]={
  {tet_mbcsv_initialize,1,1},
  {tet_mbcsv_initialize,1,2},
  {tet_mbcsv_initialize,1,3},
  {tet_mbcsv_initialize,1,4},
  {tet_mbcsv_initialize,1,6},
  {tet_mbcsv_initialize,1,7},
  {tet_mbcsv_initialize,1,8},

  {tet_mbcsv_open_close,2,1},
  {tet_mbcsv_open_close,2,2},
  {tet_mbcsv_open_close,2,3},
  {tet_mbcsv_open_close,2,4},
  {tet_mbcsv_open_close,2,5},
  {tet_mbcsv_open_close,2,6},

  {tet_mbcsv_change_role,3,1},
  {tet_mbcsv_change_role,3,2},
  {tet_mbcsv_change_role,3,3},
  {tet_mbcsv_change_role,3,4},
  {tet_mbcsv_change_role,3,5},
  {tet_mbcsv_change_role,3,6},
  {tet_mbcsv_change_role,3,7},
  {tet_mbcsv_change_role,3,8},
  {tet_mbcsv_change_role,3,9},
  {tet_mbcsv_change_role,3,10},
  {tet_mbcsv_change_role,3,11},
  {tet_mbcsv_change_role,3,12},

  {tet_mbcsv_op,4,1},
  {tet_mbcsv_op,4,2},
  {tet_mbcsv_op,4,3},

  {NULL,0},
};

#else


/* ******** Function modification  ********* */
void tet_mbcsv_initialize_01()
{
  tet_mbcsv_initialize(1);
}

void tet_mbcsv_initialize_02()
{
  tet_mbcsv_initialize(2);
}

void tet_mbcsv_initialize_03()
{
  tet_mbcsv_initialize(3);
}

void tet_mbcsv_initialize_04()
{
  tet_mbcsv_initialize(4);
}

void tet_mbcsv_initialize_06()
{
  tet_mbcsv_initialize(6);
}

void tet_mbcsv_initialize_07()
{
  tet_mbcsv_initialize(7);
}

void tet_mbcsv_initialize_08()
{
  tet_mbcsv_initialize(8);
}

void tet_mbcsv_open_close_01()
{
  tet_mbcsv_open_close(1);
}

void tet_mbcsv_open_close_02()
{
  tet_mbcsv_open_close(2);
}

void tet_mbcsv_open_close_03()
{
  tet_mbcsv_open_close(3);
}

void tet_mbcsv_open_close_04()
{
  tet_mbcsv_open_close(4);
}

void tet_mbcsv_open_close_05()
{
  tet_mbcsv_open_close(5);
}

void tet_mbcsv_open_close_06()
{
  tet_mbcsv_open_close(6);
}

void tet_mbcsv_change_role_01()
{
  tet_mbcsv_change_role(1);
}

void tet_mbcsv_change_role_02()
{
  tet_mbcsv_change_role(2);
}

void tet_mbcsv_change_role_03()
{
  tet_mbcsv_change_role(3);
}

void tet_mbcsv_change_role_04()
{
  tet_mbcsv_change_role(4);
}

void tet_mbcsv_change_role_05()
{
  tet_mbcsv_change_role(5);
}

void tet_mbcsv_change_role_06()
{
  tet_mbcsv_change_role(6);
}

void tet_mbcsv_change_role_07()
{
  tet_mbcsv_change_role(7);
}

void tet_mbcsv_change_role_08()
{
  tet_mbcsv_change_role(8);
}

void tet_mbcsv_change_role_09()
{
  tet_mbcsv_change_role(9);
}

void tet_mbcsv_change_role_10()
{
  tet_mbcsv_change_role(10);
}

void tet_mbcsv_change_role_11()
{
  tet_mbcsv_change_role(11);
}

void tet_mbcsv_change_role_12()
{
  tet_mbcsv_change_role(12);
}

void tet_mbcsv_op_01()
{
  tet_mbcsv_op(1);
}

void tet_mbcsv_op_02()
{
  tet_mbcsv_op(2);
}

void tet_mbcsv_op_03()
{
  tet_mbcsv_op(3);
}


/* ******** TET_LIST Arrays ********* */
struct tet_testlist tet_testlist[]={
  {tet_mbcsv_initialize_01, 1},
  {tet_mbcsv_initialize_02, 1},
  {tet_mbcsv_initialize_03, 1},
  {tet_mbcsv_initialize_04, 1},
  {tet_mbcsv_initialize_06, 1},
  {tet_mbcsv_initialize_07, 1},
  {tet_mbcsv_initialize_08, 1},
  
  {tet_mbcsv_open_close_01, 2},
  {tet_mbcsv_open_close_02, 2},
  {tet_mbcsv_open_close_03, 2},
  {tet_mbcsv_open_close_04, 2},
  {tet_mbcsv_open_close_05, 2},
  {tet_mbcsv_open_close_06, 2},
  
  {tet_mbcsv_change_role_01, 3},
  {tet_mbcsv_change_role_02, 3},
  {tet_mbcsv_change_role_03, 3},
  {tet_mbcsv_change_role_04, 3},
  {tet_mbcsv_change_role_05, 3},
  {tet_mbcsv_change_role_06, 3},
  {tet_mbcsv_change_role_07, 3},
  {tet_mbcsv_change_role_08, 3},
  {tet_mbcsv_change_role_09, 3},
  {tet_mbcsv_change_role_10, 3},
  {tet_mbcsv_change_role_11, 3},
  {tet_mbcsv_change_role_12, 3},

  {tet_mbcsv_op_01, 4},
  {tet_mbcsv_op_02, 4},
  {tet_mbcsv_op_03, 4},

  {NULL,0},
};
#endif

uint32_t mbcstm_startup()
{
#if (TET_PATCH==1)

  int distributed,test_case,single;
  char *tmpprt=NULL;
  tmpprt= (char *) getenv("DISTRIBUTED");
  distributed= atoi(tmpprt);
  tmpprt= (char *) getenv("TET_CASE");
  test_case= atoi(tmpprt);
  tmpprt= (char *) getenv("SINGLE");
  single= atoi(tmpprt);

  if(!distributed)
    {
      if(single)
        {
          tware_mem_ign();  
          tet_test_start(test_case,api_testlist);
          sleep(1);
          tware_mem_dump();
          tet_test_cleanup();
        }    
    }
#endif

  /*change*/

  return 0;
}



void tet_mbcsv_startup()
{
/* Using the default mode for startup */
    ncs_agents_startup(); 
  
    mbcstm_startup();
    /*change*/
}

void tet_mbcsv_cleanup()
{
  tet_infoline(" Ending the agony.. ");
}
