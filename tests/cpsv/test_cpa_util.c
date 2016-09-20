#include <unistd.h>
#include "test_cpsv.h"
#include "test_cpsv_conf.h"
#include "ncssysf_tsk.h"
#include "ncssysf_def.h"

#define APP_TIMEOUT 1000000000000000000ULL

#if FULL_LOG
#define m_TEST_CPSV_PRINTF printf
extern const char *saf_error_string[];
#else 
#define m_TEST_CPSV_PRINTF(...) 
#endif

#define END_OF_WHILE while(((rc == SA_AIS_ERR_TRY_AGAIN) || ( rc == SA_AIS_ERR_TIMEOUT)) && (tmoutFlag == 0)); \
                     gl_tmout_cnt = 0 ; \
                     gl_try_again_cnt = 0 ; \
                     tmoutFlag = 0

int gl_try_again_cnt;
int gl_tmout_cnt;
int gl_sync_pointnum;
int tmoutFlag;

int cpsv_test_result(SaAisErrorT rc,SaAisErrorT exp_out,char *test_case,CONFIG_FLAG flg)
{
   int result = 0;

   if(rc == SA_AIS_ERR_TRY_AGAIN)
   {
      result = TEST_FAIL;
      if(gl_try_again_cnt++ == 0)
      {
         m_TEST_CPSV_PRINTF("\n FAILED          : %s\n",test_case);
         m_TEST_CPSV_PRINTF(" Return Value    : %s\n\n",saf_error_string[rc]);
      }

      if(!(gl_try_again_cnt%10))
         m_TEST_CPSV_PRINTF(" Try again count : %d \n",gl_try_again_cnt);

      usleep(500000); /* 500 ms */
      return(result);
   }
   
   if(rc == SA_AIS_ERR_TIMEOUT)
   {
      result = TEST_FAIL;
      if(gl_tmout_cnt++ == 0)
      {
         m_TEST_CPSV_PRINTF("\n FAILED          : %s\n",test_case);
         m_TEST_CPSV_PRINTF(" Return Value    : %s\n\n",saf_error_string[rc]);
      }

      if(gl_try_again_cnt > 0)
      {
         m_TEST_CPSV_PRINTF("\n TIMEOUT AFTER TRY_AGAIN POSSIBLE ERROR  : %s\n",test_case);
         tmoutFlag = 1;
      }

      if(gl_tmout_cnt == 10)
      {
         m_TEST_CPSV_PRINTF(" Timeout count : %d \n",gl_tmout_cnt);
         tmoutFlag = 1;
      }
        
      usleep(500000); /* 500 ms */
      return(result);
   }
   
   if(gl_try_again_cnt)
   {
       m_TEST_CPSV_PRINTF(" Try again count : %d \n",gl_try_again_cnt);
       gl_try_again_cnt = 0;
   }

   if(gl_tmout_cnt)
   {
       m_TEST_CPSV_PRINTF(" Timeout count : %d \n",gl_tmout_cnt);
       gl_tmout_cnt = 0;
       tmoutFlag = 0;
   }

   if(rc == exp_out)
   {
      m_TEST_CPSV_PRINTF("\n SUCCESS       : %s  \n",test_case);
      result = TEST_PASS;
   }
   else
   {
      m_TEST_CPSV_PRINTF("\n FAILED        : %s  \n",test_case);
      result = TEST_FAIL;
      if(flg == TEST_CONFIG_MODE)
         result = TEST_UNRESOLVED;
   }

   m_TEST_CPSV_PRINTF(" Return Value  : %s\n",saf_error_string[rc]);
   return(result);
}

void printStatus(SaCkptCheckpointDescriptorT ckptStatus)
{
  m_TEST_CPSV_PRINTF(" Number of Sections: %u\n",ckptStatus.numberOfSections);
  m_TEST_CPSV_PRINTF(" Memory Used: %llu\n", ckptStatus.memoryUsed);
  tcd.memLeft = (ckptStatus.checkpointCreationAttributes.checkpointSize)-(ckptStatus.memoryUsed);
  m_TEST_CPSV_PRINTF(" Memory Left: %llu\n",tcd.memLeft);
  m_TEST_CPSV_PRINTF(" CreationFlags: %u\n",ckptStatus.checkpointCreationAttributes.creationFlags);
  m_TEST_CPSV_PRINTF(" Checkpoint Size: %llu\n",ckptStatus.checkpointCreationAttributes.checkpointSize);
  m_TEST_CPSV_PRINTF(" Retention Duration: %llu\n",ckptStatus.checkpointCreationAttributes.retentionDuration);
  m_TEST_CPSV_PRINTF(" Max Sections: %u\n",ckptStatus.checkpointCreationAttributes.maxSections);
  m_TEST_CPSV_PRINTF(" Max Section Size: %llu\n",ckptStatus.checkpointCreationAttributes.maxSectionSize);
}


/************ Initialization **************/


struct SafCkptInitialize API_Initialize[]={

  [CKPT_INIT_SUCCESS_T]     =  {&tcd.ckptHandle,&tcd.version_supported,&tcd.general_callbks,SA_AIS_OK,"Init with general calbaks"},

  [CKPT_INIT_NULL_CBKS_T]   =  {&tcd.junkHandle1,&tcd.version_supported,NULL,SA_AIS_OK,"Init with NULL pointer to callbks"},

  [CKPT_INIT_NULL_VERSION_T]   =  {&tcd.junkHandle2,NULL,&tcd.general_callbks,SA_AIS_ERR_INVALID_PARAM,"Init with NULL version pointer"},

  [CKPT_INIT_VER_CBKS_NULL_T]   =  {&tcd.junkHandle2,NULL,NULL,SA_AIS_ERR_INVALID_PARAM,"Init with version and calbks NULL"}, 

  [CKPT_INIT_ERR_VERSION_T]   =  {&tcd.junkHandle2,&tcd.version_err,&tcd.general_callbks,SA_AIS_ERR_VERSION,"Init with WRONG version"},  

  [CKPT_INIT_ERR_VERSION2_T]   =  {&tcd.junkHandle3,&tcd.version_err2,&tcd.general_callbks,SA_AIS_ERR_VERSION,"Init with WRONG version"},  

  [CKPT_INIT_OPEN_NULL_CBK_T]   =  {&tcd.ckptHandle3,&tcd.version_supported,&tcd.open_null_callbk,SA_AIS_OK,"Init with open NULL calbk "},

  [CKPT_INIT_SYNC_NULL_CBK_T]   =  {&tcd.ckptHandle4,&tcd.version_supported,&tcd.synchronize_null_callbk,SA_AIS_OK,"Init with sync NULL calbk "},

  [CKPT_INIT_NULL_CBKS2_T]   =  {&tcd.ckptHandle5,&tcd.version_supported,&tcd.null_callbks,SA_AIS_OK,"Init with both calbks as NULL "},

  [CKPT_INIT_NULL_HDL_T]     =  {NULL,&tcd.version_supported,&tcd.general_callbks,SA_AIS_ERR_INVALID_PARAM,"Init with NULL as handle"},

  [CKPT_INIT_NULL_PARAMS_T]   =  {NULL,NULL,NULL,SA_AIS_ERR_INVALID_PARAM,"Init with all params NULL"},

  [CKPT_INIT_HDL_VER_NULL_T]   =  {NULL,NULL,&tcd.general_callbks,SA_AIS_ERR_INVALID_PARAM,"Init with handle and version NULL"},

  [CKPT_INIT_SUCCESS_HDL2_T]   =  {&tcd.ckptHandle2,&tcd.version_supported,&tcd.general_callbks,SA_AIS_OK,"ckptHandle2 init with gen. calbks"},


};

int test_ckptInitialize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;

  rc = saCkptInitialize(API_Initialize[i].ckptHandle,API_Initialize[i].callbks,API_Initialize[i].vers);
  
  result = cpsv_test_result( rc,API_Initialize[i].exp_output,API_Initialize[i].result_string,cfg_flg); 
                                                                                                                                                                      
   if(rc == SA_AIS_OK)
      m_TEST_CPSV_PRINTF(" Ckpt Handle   :  %llu\n",*API_Initialize[i].ckptHandle);
                                                                                                                                                                      
   return(result);
}

int test_red_ckptInitialize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  do
  {
     rc = saCkptInitialize(API_Initialize[i].ckptHandle,API_Initialize[i].callbks,API_Initialize[i].vers);
  
     result = cpsv_test_result( rc,API_Initialize[i].exp_output,API_Initialize[i].result_string,cfg_flg); 
     if(rc == SA_AIS_OK)
        m_TEST_CPSV_PRINTF(" Ckpt Handle   :  %llu\n",*API_Initialize[i].ckptHandle);
  }END_OF_WHILE;
     
  return result;
}

void printHandles()
{
  m_TEST_CPSV_PRINTF("ckptHandle - %llu\n", tcd.ckptHandle);
  m_TEST_CPSV_PRINTF("ckptHandle2 - %llu\n", tcd.ckptHandle2);
  m_TEST_CPSV_PRINTF("ckptHandle3 - %llu\n", tcd.ckptHandle3);
  m_TEST_CPSV_PRINTF("ckptHandle4 - %llu\n", tcd.ckptHandle4);
  m_TEST_CPSV_PRINTF("ckptHandle5 - %llu\n", tcd.ckptHandle5);
}

/* END OF INTIALIZATION */


/**************** selection object *****************/


struct SafSelectionObject API_Selection[]={

  [CKPT_SEL_SUCCESS_T]     =  {&tcd.ckptHandle,&tcd.selobj,SA_AIS_OK,"SelObj get"},        

  [CKPT_SEL_HDL_NULL_T]     =  {NULL,&tcd.selobj,SA_AIS_ERR_BAD_HANDLE,"SelObj with NULL handle"},

  [CKPT_SEL_UNINIT_HDL_T]   =  {&tcd.uninitHandle,&tcd.selobj,SA_AIS_ERR_BAD_HANDLE,"SelObj with uninit handle"},

  [CKPT_SEL_NULL_OBJ_T]     =  {&tcd.ckptHandle,NULL,SA_AIS_ERR_INVALID_PARAM,"SelObj with NULL selobj"},

  [CKPT_SEL_HDL_OBJ_NULL_T]  =  {NULL,NULL,SA_AIS_ERR_INVALID_PARAM,"SelObj with hdl and Obj NULL"},

  [CKPT_SEL_FIN_HANDLE_T]     =  {&tcd.ckptHandle,&tcd.selobj,SA_AIS_ERR_BAD_HANDLE,"Testing for sel object after finalizing handle"},        

};

int test_ckptSelectionObject(int i,CONFIG_FLAG cfg_flg) 
{

  SaAisErrorT rc;
  SaCkptHandleT selHandle;
        
  if (API_Selection[i].ckptHandle == NULL) 
  {
     selHandle=0;
  } 
  else 
  {
     selHandle=*(API_Selection[i].ckptHandle);
  }

  rc=saCkptSelectionObjectGet(selHandle,API_Selection[i].selobj);
  return(cpsv_test_result( rc,API_Selection[i].exp_output,API_Selection[i].result_string,cfg_flg));
}

int test_red_ckptSelectionObject(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  SaCkptHandleT selHandle;
  gl_try_again_cnt=0;

  do
  {
     if (API_Selection[i].ckptHandle == NULL) 
        selHandle=0;
     else 
        selHandle=*(API_Selection[i].ckptHandle);

     rc=saCkptSelectionObjectGet(selHandle,API_Selection[i].selobj);
     result = cpsv_test_result( rc,API_Selection[i].exp_output,API_Selection[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}



/**************** Checkpoint Open *******************/



struct SafCheckpointOpen API_Open[]={

    [CKPT_OPEN_BAD_HANDLE_T]         = {&tcd.uninitHandle,&tcd.all_replicas_ckpt,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                        APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_BAD_HANDLE,"Open with bad handle"},

    [CKPT_OPEN_BAD_HANDLE2_T]  = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                         APP_TIMEOUT,&tcd.all_replicas_Createhdl,SA_AIS_ERR_BAD_HANDLE,"ckpt open with finalized handle"},

    [CKPT_OPEN_INVALID_PARAM_T]      = {&tcd.ckptHandle,NULL,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                       APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with NULL ckptName"},

    [CKPT_OPEN_INVALID_PARAM2_T]     = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                       APP_TIMEOUT,NULL,SA_AIS_ERR_INVALID_PARAM,"hdl NULL"},

    [CKPT_OPEN_ALL_CREATE_SUCCESS_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                         APP_TIMEOUT,&tcd.all_replicas_Createhdl,SA_AIS_OK,"ckpt with ALL_REPLICAS created"},

    [CKPT_OPEN_ALL_WRITE_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,NULL,(SA_CKPT_CHECKPOINT_WRITE | SA_CKPT_CHECKPOINT_READ),
                                        APP_TIMEOUT,&tcd.all_replicas_Writehdl ,SA_AIS_OK,"ckpt with ALL_REPLICAS opened for writing"},   

    [CKPT_OPEN_ALL_READ_SUCCESS_T]    = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                         APP_TIMEOUT,&tcd.all_replicas_Readhdl ,SA_AIS_OK,"ckpt with ALL_REPLICAS opened for reading"},

    [CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T] = {&tcd.ckptHandle,&tcd.active_replica_ckpt,&tcd.active_replica,(SA_CKPT_CHECKPOINT_CREATE | SA_CKPT_CHECKPOINT_READ),
                                            APP_TIMEOUT,&tcd.active_replica_Createhdl ,SA_AIS_OK,"ckpt with ACTIVE REPLICA created"},

    [CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T]  = {&tcd.ckptHandle,&tcd.active_replica_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                           APP_TIMEOUT,&tcd.active_replica_Writehdl ,SA_AIS_OK,"ckpt with ACTIVE REPLICA opened for writing"},

    [CKPT_OPEN_ACTIVE_READ_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.active_replica_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                          APP_TIMEOUT,&tcd.active_replica_Readhdl ,SA_AIS_OK,"ckpt with ACTIVE REPLICA opened for reading"},

    [CKPT_OPEN_WEAK_CREATE_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,&tcd.replica_weak,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.weak_replica_Createhdl ,SA_AIS_OK,"ckpt with WEAK REPLICA created"},

    [CKPT_OPEN_WEAK_BAD_FLAGS_T]        = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,&tcd.replica_weak,8,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_BAD_FLAGS,"Open with bad flags"},  

    [CKPT_OPEN_WEAK_WRITE_SUCCESS_T]    = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                          APP_TIMEOUT,&tcd.weak_replica_Writehdl ,SA_AIS_OK,"ckpt with WEAK REPLICA opened for writing"},

    [CKPT_OPEN_WEAK_READ_SUCCESS_T]     = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                           APP_TIMEOUT,&tcd.weak_replica_Readhdl ,SA_AIS_OK,"ckpt with WEAK REPLICA opened for reading"},

    [CKPT_OPEN_COLLOCATE_SUCCESS_T]     = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.collocated,(SA_CKPT_CHECKPOINT_CREATE |SA_CKPT_CHECKPOINT_READ) ,
                                          APP_TIMEOUT,&tcd.active_colloc_Createhdl ,SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED created"},

    [CKPT_OPEN_COLLOCATE_SUCCESS2_T]    = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.collocated,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.active_colloc_create_test ,SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED opened again"},    

    [CKPT_OPEN_BAD_NAME_T]              = {&tcd.ckptHandle,&tcd.non_existing_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_NOT_EXIST,"Open with bad name"},     

    [CKPT_OPEN_COLLOCATE_INVALID_T]     = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.collocated,SA_CKPT_CHECKPOINT_READ,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with creation attribute but No CREATE flag"},  

    [CKPT_OPEN_NULL_ATTR_T]             = {&tcd.ckptHandle,&tcd.collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with NULL creation attributes and CREATE flag"},  

    [CKPT_OPEN_ATTR_INVALID_T]          = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with invalid param"},  

    [CKPT_OPEN_ATTR_INVALID2_T]         = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid2,SA_CKPT_CHECKPOINT_CREATE,
                                           APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with invalid param"},  

    [CKPT_OPEN_ATTR_INVALID3_T]         = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid3,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with invalid param"},  

    [CKPT_OPEN_ATTR_INVALID4_T]         = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid4,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_INVALID_PARAM,"Open with invalid param"},  

    [CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T]   = {&tcd.ckptHandle4,&tcd.collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                              APP_TIMEOUT,&tcd.active_colloc_sync_nullcb_Writehdl ,SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED opened for writing"},

    [CKPT_OPEN_COLLOCATE_READ_SUCCESS_T]    = {&tcd.ckptHandle,&tcd.collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                              APP_TIMEOUT,&tcd.active_colloc_Readhdl ,SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED opened for reading"},

    [CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T]  = {&tcd.ckptHandle,&tcd.collocated_ckpt,NULL,(SA_CKPT_CHECKPOINT_WRITE | SA_CKPT_CHECKPOINT_READ),
                                              APP_TIMEOUT,&tcd.active_colloc_Writehdl ,SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED opened for writing"},

    [CKPT_OPEN_ERR_EXIST_T]                 = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.active_replica,SA_CKPT_CHECKPOINT_CREATE,
                                              APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_EXIST,"Open already existing ckpt but diff attributes"},

    [CKPT_OPEN_ERR_EXIST2_T]                = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_CREATE,
                                               APP_TIMEOUT,&tcd.testHandle,SA_AIS_ERR_EXIST,"Open already existing ckpt but diff ckptSize"},

    [CKPT_OPEN_ALL_CREATE_SUCCESS2_T]       = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                              APP_TIMEOUT,&tcd.all_replicas_create_after_unlink ,SA_AIS_OK,"ckpt1 created"},

    [CKPT_OPEN_COLLOCATE_SUCCESS3_T]        = {&tcd.ckptHandle,&tcd.smoketest_ckpt,&tcd.smoke_test_replica,
                                              SA_CKPT_CHECKPOINT_CREATE|SA_CKPT_CHECKPOINT_WRITE|SA_CKPT_CHECKPOINT_READ,APP_TIMEOUT,
                                              &tcd.allflags_set_hdl ,SA_AIS_OK,"ACTIVE_REPLICA opened with all openflags on"},        

    [CKPT_OPEN_ALL_COLLOC_CREATE_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.all_collocated_ckpt,&tcd.ckpt_all_collocated_replica,SA_CKPT_CHECKPOINT_CREATE,
                                                APP_TIMEOUT,&tcd.all_collocated_Createhdl,SA_AIS_OK,"ckpt with all+collocted flags created"},

    [CKPT_OPEN_ALL_COLLOC_WRITE_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.all_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                               APP_TIMEOUT,&tcd.all_collocated_Writehdl,SA_AIS_OK,"all+collocated opened with write mode"},

    [CKPT_OPEN_ALL_COLLOC_READ_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.all_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                               APP_TIMEOUT,&tcd.all_collocated_Readhdl,SA_AIS_OK,"all_collocated opened with read mode"},

    [CKPT_OPEN_WEAK_COLLOC_CREATE_SUCCESS_T] = {&tcd.ckptHandle,&tcd.weak_collocated_ckpt,&tcd.ckpt_weak_collocated_replica,SA_CKPT_CHECKPOINT_CREATE,
                                               APP_TIMEOUT,&tcd.weak_collocated_Createhdl,SA_AIS_OK,"ckpt with weak+collocted flags created"},

    [CKPT_OPEN_WEAK_COLLOC_WRITE_SUCCESS_T]  = {&tcd.ckptHandle,&tcd.weak_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                               APP_TIMEOUT,&tcd.weak_collocated_Writehdl,SA_AIS_OK,"weak+collocated opened with write mode"},

    [CKPT_OPEN_WEAK_COLLOC_READ_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.weak_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                               APP_TIMEOUT,&tcd.weak_collocated_Readhdl,SA_AIS_OK,"weak+collocated opened with read mode"},

    [CKPT_OPEN_MULTI_IO_REPLICA_T]           = {&tcd.ckptHandle,&tcd.multi_vector_ckpt,&tcd.multi_io_replica,
                                               SA_CKPT_CHECKPOINT_CREATE|SA_CKPT_CHECKPOINT_READ|SA_CKPT_CHECKPOINT_WRITE,
                                               APP_TIMEOUT,&tcd.multi_io_hdl,SA_AIS_OK,"ckpt with all flags on created"},

    [CKPT_OPEN_ERR_NOT_EXIST_T]    = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                         APP_TIMEOUT,&tcd.all_replicas_Readhdl ,SA_AIS_ERR_NOT_EXIST,"ckpt not exist"},

    [CKPT_OPEN_ALL_MODES_SUCCESS_T]     = {&tcd.ckptHandle,&tcd.async_all_replicas_ckpt,&tcd.active_replica,
                                            SA_CKPT_CHECKPOINT_CREATE|SA_CKPT_CHECKPOINT_WRITE|SA_CKPT_CHECKPOINT_READ,APP_TIMEOUT,&tcd.async_all_replicas_hdl,
                                            SA_AIS_OK,"ckpt5 created with invocation 1018"},    

    [CKPT_OPEN_INVALID_CR_FLAGS_T]     = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_CREATE,
                                          APP_TIMEOUT,&tcd.active_colloc_Createhdl ,SA_AIS_ERR_INVALID_PARAM,"ckpt with COLLOCATED cr_flags"},
   [CKPT_OPEN_ACTIVE_CREATE_WRITE_SUCCESS_T] = {&tcd.ckptHandle,&tcd.active_replica_ckpt,&tcd.active_replica,(SA_CKPT_CHECKPOINT_CREATE | SA_CKPT_CHECKPOINT_WRITE),
                                            APP_TIMEOUT,&tcd.active_replica_Createhdl ,SA_AIS_OK,"ckpt with ACTIVE REPLICA created"},
    [CKPT_OPEN_SUCCESS_EXIST2_T]                = {&tcd.ckptHandle,&tcd.collocated_ckpt,&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_CREATE,
                                               APP_TIMEOUT,&tcd.testHandle,SA_AIS_OK,"Open already existing ckpt but different retention duration"},
   [CKPT_OPEN_WEAK_CREATE_READ_SUCCESS_T]   = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,&tcd.replica_weak,(SA_CKPT_CHECKPOINT_CREATE | SA_CKPT_CHECKPOINT_READ) ,
                                          APP_TIMEOUT,&tcd.weak_replica_Createhdl ,SA_AIS_OK,"ckpt with WEAK REPLICA created"},
    [CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T]  = {&tcd.ckptHandle,&tcd.active_replica_ckpt,NULL,(SA_CKPT_CHECKPOINT_WRITE | SA_CKPT_CHECKPOINT_READ),
                                           APP_TIMEOUT,&tcd.active_replica_Writehdl ,SA_AIS_OK,"ckpt with ACTIVE REPLICA opened for writing"},
    [CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_SUCCESS_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt_with_valid_extended_name_length,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                         APP_TIMEOUT,&tcd.all_replicas_Createhdl,SA_AIS_OK,"ckpt with ALL_REPLICAS and valid extended name created"},
    [CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt_with_invalid_extended_name_length,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                         APP_TIMEOUT,&tcd.all_replicas_Createhdl,SA_AIS_ERR_TOO_BIG,"ckpt with ALL_REPLICAS and invalid extended name created"},

    /* NULL ckptHandle, */

};

int test_ckptOpen(int i,CONFIG_FLAG cfg_flg) 
{
   SaAisErrorT rc;
   int result;

   rc=saCkptCheckpointOpen(*(API_Open[i].ckptHandle),API_Open[i].ckptName,API_Open[i].ckptCreateAttr,API_Open[i].ckptOpenFlags,
                                                                  API_Open[i].timeout,API_Open[i].checkpointhandle);
   result = cpsv_test_result( rc,API_Open[i].exp_output,API_Open[i].result_string,cfg_flg);

   if(rc == SA_AIS_OK)
      m_TEST_CPSV_PRINTF("\n Checkpoint Handle   : %llu\n",*API_Open[i].checkpointhandle);
                                                                                                                                                                      
   return(result);
}

int test_red_ckptOpen(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  do
  {
      rc=saCkptCheckpointOpen(*(API_Open[i].ckptHandle),API_Open[i].ckptName,API_Open[i].ckptCreateAttr,
                                 API_Open[i].ckptOpenFlags,API_Open[i].timeout,API_Open[i].checkpointhandle);
      result = cpsv_test_result( rc,API_Open[i].exp_output,API_Open[i].result_string,cfg_flg);
   
      if(rc == SA_AIS_OK)
         m_TEST_CPSV_PRINTF("\n Checkpoint Handle   : %llu\n",*API_Open[i].checkpointhandle);
  }END_OF_WHILE;

  return result;
}



/******************** Open Aysnc ***************/

void selection_thread_blocking (NCSCONTEXT arg)
{
                                                                                                                             
   fd_set read_fd;
   SaSelectionObjectT     selection_object;
   SaCkptHandleT threadHandle = *(SaCkptHandleT *)arg;
   SaAisErrorT rc;
                                                                                                                             
   m_TEST_CPSV_PRINTF("\n Executing Thread selection\n");
   rc = saCkptSelectionObjectGet(threadHandle, &selection_object);
   if (rc != SA_AIS_OK)
   {
        m_TEST_CPSV_PRINTF(" saCkptSelectionObjectGet failed  - %d", rc);
        return;
   }

   FD_ZERO(&read_fd);
   FD_SET(selection_object, &read_fd);
   rc = select(selection_object + 1, &read_fd, NULL, NULL, NULL);
   if (rc == -1)
   {
      m_TEST_CPSV_PRINTF("Select FAILED\n");
      return;
   }
                                                                                                                             
   rc = saCkptDispatch(threadHandle, SA_DISPATCH_BLOCKING);
   if (rc != SA_AIS_OK)
      m_TEST_CPSV_PRINTF("\n Dispatching FAILED %d \n", rc);
   else
      m_TEST_CPSV_PRINTF("\n Thread selected \n");
}

void cpsv_createthread(SaCkptHandleT *cl_hdl)
{
   SaAisErrorT  rc;
   NCSCONTEXT thread_handle;

   rc = m_NCS_TASK_CREATE((NCS_OS_CB)selection_thread_blocking, (NCSCONTEXT)cl_hdl,
                           "cpsv_block_test", 0, SCHED_OTHER, 8000, &thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TEST_CPSV_PRINTF(" Failed to create thread\n");
       return ;
   }
                                                                                                                                                                      
   rc = m_NCS_TASK_START(thread_handle);
   if (rc != NCSCC_RC_SUCCESS)
   {
       m_TEST_CPSV_PRINTF(" Failed to start thread\n");
       return ;
   }

}

struct SafCheckpointOpenAsync API_OpenAsync[]={

    [CKPT_OPEN_ASYNC_BAD_HANDLE_T]       = {&tcd.uninitHandle,1000,&tcd.async_all_replicas_ckpt,&tcd.all_replicas,
                                           SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_BAD_HANDLE,"Open Async bad handle 1000"},    

    [CKPT_OPEN_ASYNC_ERR_INIT_T]       = {&tcd.ckptHandle3,1001,&tcd.async_all_replicas_ckpt,&tcd.all_replicas,
                                         SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INIT,"Open Async err init invocation 1001"},   

    [CKPT_OPEN_ASYNC_ALL_CREATE_SUCCESS_T]     = {&tcd.ckptHandle,1002,&tcd.async_all_replicas_ckpt,&tcd.all_replicas,
                                                 SA_CKPT_CHECKPOINT_CREATE,SA_AIS_OK,"all replicas ckpt created with invocation 1002"},    

    [CKPT_OPEN_ASYNC_ACTIVE_CREATE_SUCCESS_T]   = {&tcd.ckptHandle,1003,&tcd.async_active_replica_ckpt,&tcd.active_replica,
                                                  SA_CKPT_CHECKPOINT_CREATE,SA_AIS_OK,"active replica ckpt created with invocation 1003"},      

    [CKPT_OPEN_ASYNC_ACTIVE_WRITE_SUCCESS_T]  = {&tcd.ckptHandle2,1004,&tcd.async_all_replicas_ckpt,NULL,
                                                SA_CKPT_CHECKPOINT_WRITE,SA_AIS_OK,"all replicas ckpt opened for writing with invocation 1004"},  

    [CKPT_OPEN_ASYNC_COLLOCATE_SUCCESS2_T]    = {&tcd.ckptHandle,1005,&tcd.collocated_ckpt,&tcd.collocated,SA_CKPT_CHECKPOINT_CREATE,
                                                SA_AIS_OK,"ckpt with ACTIVE|COLLOCATED opened async call"},    

    [CKPT_OPEN_ASYNC_BAD_NAME_T]       = {&tcd.ckptHandle,1006,&tcd.non_existing_ckpt,NULL,
                                           SA_CKPT_CHECKPOINT_WRITE,SA_AIS_OK,"Open async bad ckptname 1006"},     

    [CKPT_OPEN_ASYNC_INVALID_PARAM_T]     = {&tcd.ckptHandle,1007,&tcd.async_all_replicas_ckpt,&tcd.all_replicas,
                                             SA_CKPT_CHECKPOINT_READ,SA_AIS_ERR_INVALID_PARAM,"Open async invalid param"},    

    [CKPT_OPEN_ASYNC_INVALID_PARAM2_T]     = {&tcd.ckptHandle,1008,&tcd.async_all_replicas_ckpt,NULL,
                                             SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INVALID_PARAM,"Open Async invalid param2"},

    [CKPT_OPEN_ASYNC_INVALID_PARAM3_T]     = {&tcd.ckptHandle,1009,&tcd.collocated_ckpt,&tcd.invalid,
                                             SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INVALID_PARAM,"Open Async invalid param3"},

    [CKPT_OPEN_ASYNC_INVALID_PARAM4_T]     = {&tcd.ckptHandle,1010,&tcd.collocated_ckpt,&tcd.invalid2,
                                             SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INVALID_PARAM,"Open Async invalid param4"},

    [CKPT_OPEN_ASYNC_INVALID_PARAM5_T]     = {&tcd.ckptHandle,1011,&tcd.collocated_ckpt,&tcd.invalid3,
                                             SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INVALID_PARAM,"Open Async invalid param5"},  

    [CKPT_OPEN_ASYNC_INVALID_PARAM6_T]     = {&tcd.ckptHandle,1012,&tcd.collocated_ckpt,&tcd.invalid4,
                                             SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_INVALID_PARAM,"Open Async invalid param6"},

    [CKPT_OPEN_ASYNC_BAD_FLAGS_T]       = {&tcd.ckptHandle,1013,&tcd.weak_replica_ckpt,&tcd.replica_weak,
                                          8,SA_AIS_ERR_BAD_FLAGS,"Open Async bad flags"},              

    [CKPT_OPEN_ASYNC_ALL_COLLOC_WRITE_SUCCESS_T]   = {&tcd.ckptHandle,1014,&tcd.all_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                                     SA_AIS_OK,"all+collocated opened with write mode Async open"},

    [CKPT_OPEN_ASYNC_ALL_COLLOC_READ_SUCCESS_T]     = {&tcd.ckptHandle,1015,&tcd.all_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                                      SA_AIS_OK,"all_collocated opened with read mode Async open"},
                                                                                                                                                                      
    [CKPT_OPEN_ASYNC_WEAK_COLLOC_WRITE_SUCCESS_T]   = {&tcd.ckptHandle,1016,&tcd.weak_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_WRITE,
                                                       SA_AIS_OK,"weak+collocated opened with write mode Async open"},

    [CKPT_OPEN_ASYNC_WEAK_COLLOC_READ_SUCCESS_T]     = {&tcd.ckptHandle,1017,&tcd.weak_collocated_ckpt,NULL,SA_CKPT_CHECKPOINT_READ,
                                                       SA_AIS_OK,"weak+collocated opened with read mode Async open"},

    [CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T]     = {&tcd.ckptHandle,1018,&tcd.async_all_replicas_ckpt,&tcd.active_replica,
                                                 SA_CKPT_CHECKPOINT_CREATE|SA_CKPT_CHECKPOINT_WRITE|SA_CKPT_CHECKPOINT_READ,SA_AIS_OK,
                                                    "ckpt5 created with invocation 1018"},    

    [CKPT_OPEN_ASYNC_NULL_INVOCATION]     = {&tcd.ckptHandle,0,&tcd.async_all_replicas_ckpt,&tcd.all_replicas,
                                                 SA_CKPT_CHECKPOINT_CREATE,SA_AIS_OK,"NULL invocation"},    

    [CKPT_OPEN_ASYNC_ERR_EXIST_T]         = {&tcd.ckptHandle,1019,&tcd.collocated_ckpt,&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_CREATE,
                                               SA_AIS_OK,"Open already existing ckpt but diff params"},

    [CKPT_OPEN_ASYNC_INVALID_PARAM7_T]     = {&tcd.ckptHandle,1020,NULL,&tcd.all_replicas,SA_CKPT_CHECKPOINT_CREATE,
                                             SA_AIS_ERR_INVALID_PARAM,"Open Async with invalid param - NUll name"},
    [CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_SUCCESS_T]     = {&tcd.ckptHandle,1021, &tcd.all_replicas_ckpt_with_valid_extended_name_length, &tcd.all_replicas,
                                                 SA_CKPT_CHECKPOINT_CREATE,SA_AIS_OK,"all replicas ckpt with valid extended name created with invocation 1021"},
    [CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T]     = {&tcd.ckptHandle,0, &tcd.all_replicas_ckpt_with_invalid_extended_name_length, &tcd.all_replicas,
                                                 SA_CKPT_CHECKPOINT_CREATE,SA_AIS_ERR_TOO_BIG,"all replicas ckpt invalid with extended name created"},
};

int test_ckptOpenAsync(int i,CONFIG_FLAG cfg_flg) 
{
   SaAisErrorT rc;
   rc = saCkptCheckpointOpenAsync(*(API_OpenAsync[i].ckptHandle),API_OpenAsync[i].invocation,API_OpenAsync[i].ckptName,
                                            API_OpenAsync[i].ckptCreateAttr,API_OpenAsync[i].ckptOpenFlags);

   return(cpsv_test_result( rc,API_OpenAsync[i].exp_output,API_OpenAsync[i].result_string,cfg_flg));
}

int test_red_ckptOpenAsync(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  do
  {
      rc = saCkptCheckpointOpenAsync(*(API_OpenAsync[i].ckptHandle),API_OpenAsync[i].invocation,API_OpenAsync[i].ckptName,
                                            API_OpenAsync[i].ckptCreateAttr,API_OpenAsync[i].ckptOpenFlags);

      result = cpsv_test_result( rc,API_OpenAsync[i].exp_output,API_OpenAsync[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}



/*************** Section Create ****************/


struct SafCheckpointSectionCreate API_SectionCreate[]= {

    [CKPT_SECTION_CREATE_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,&tcd.general_attr,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_BAD_HANDLE,"Section Create with bad handle"},  

    [CKPT_SECTION_CREATE_INVALID_PARAM_T]   = {&tcd.all_replicas_Writehdl ,NULL,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_INVALID_PARAM,"Section Create with NULL attributes"},

    [CKPT_SECTION_CREATE_INVALID_PARAM2_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalid_attr,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_INVALID_PARAM,"idlen >mex sec id size"},

    [CKPT_SECTION_CREATE_SUCCESS_T]    = {&tcd.all_replicas_Writehdl ,&tcd.general_attr,tcd.data1,&tcd.size,
                                           SA_AIS_OK,"Section id 11 Created in all replicas ckpt"},    

    [CKPT_SECTION_CREATE_NULL_DATA_SIZE_ZERO_T] = {&tcd.all_replicas_Writehdl ,&tcd.general_attr,NULL,&tcd.size_zero,
                                               SA_AIS_OK,"section create data NULL and size 0"},

    [CKPT_SECTION_CREATE_NULL_DATA_SIZE_NOTZERO_T] = {&tcd.all_replicas_Writehdl ,&tcd.general_attr,NULL,&tcd.size,
                                               SA_AIS_ERR_INVALID_PARAM,"data NULL and size greater than zero"},

    [CKPT_SECTION_CREATE_NOT_EXIST_T]   = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.special_attr,tcd.data2,&tcd.size,  
                                           SA_AIS_ERR_NOT_EXIST,"Section Create but no active replica"},    

    [CKPT_SECTION_CREATE_SUCCESS2_T]   = {&tcd.weak_replica_Writehdl ,&tcd.section_attr,tcd.data3,&tcd.size,
                                           SA_AIS_ERR_EXIST,"Section DEFAULT_ID in ckpt3"},

    [CKPT_SECTION_CREATE_ERR_ACCESS_T]   = {&tcd.all_replicas_Readhdl ,&tcd.special_attr,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_ACCESS,"Section Create with wrong access"},  

    [CKPT_SECTION_CREATE_SUCCESS3_T]   = {&tcd.all_replicas_Writehdl ,&tcd.expiration_attr,tcd.data2,&tcd.size,
                                           SA_AIS_OK,"Section Id 21 Created in all replicas ckpt"},

    [CKPT_SECTION_CREATE_NO_SPACE_T]   = {&tcd.all_replicas_Writehdl ,&tcd.special_attr,tcd.data3,&tcd.size,
                                           SA_AIS_ERR_NO_SPACE,"Section Create NO SPACE"},    

    [CKPT_SECTION_CREATE_SUCCESS4_T]   = {&tcd.active_replica_Writehdl ,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section Create in active replica ckpt"},  

    [CKPT_SECTION_CREATE_ERR_EXIST_T]   = {&tcd.active_replica_Writehdl ,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_ERR_EXIST,"Section Create which already exists"},  

    [CKPT_SECTION_CREATE_SUCCESS5_T]   = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.special_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section GENERATED_SECTION_ID created in active collocated ckpt"},    

    [CKPT_SECTION_CREATE_GEN_T]     = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.special_attr2,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section created in ckpt"},    

    [CKPT_SECTION_CREATE_SUCCESS6_T]   = {&tcd.allflags_set_hdl ,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section created in ckpt"},    

    [CKPT_SECTION_CREATE_SUCCESS7_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 11 created in active colloc ckpt"},    

    [CKPT_SECTION_CREATE_SUCCESS8_T]   = {&tcd.all_collocated_Writehdl,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 11 created in all+colloc ckpt"},    

    [CKPT_SECTION_CREATE_SUCCESS9_T]   = {&tcd.weak_collocated_Writehdl,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 11 created in weak+colloc ckpt"},    

    [CKPT_SECTION_CREATE_MULTI_T]     = {&tcd.multi_io_hdl,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 11 created in ckpt"},    

    [CKPT_SECTION_CREATE_MULTI2_T]     = {&tcd.multi_io_hdl,&tcd.expiration_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 21 created in ckpt"},    

    [CKPT_SECTION_CREATE_MULTI3_T]     = {&tcd.multi_io_hdl,&tcd.multi_attr,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section 31 created in ckpt"},    

    [CKPT_SECTION_CREATE_MULTI4_T]     = {&tcd.multi_io_hdl,&tcd.general_attr,tcd.data3,&tcd.size,
                                           SA_AIS_ERR_BAD_HANDLE,"Section 11 created in ckpt"},    

    [CKPT_SECTION_CREATE_SUCCESS10_T]   = {&tcd.active_replica_Writehdl ,&tcd.expiration_attr,tcd.data2,&tcd.size,
                                           SA_AIS_OK,"Section Id 21 Created in active replicas ckpt"},

    [CKPT_SECTION_CREATE_GEN_EXIST_T]     = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.special_attr2,tcd.data3,&tcd.size,
                                           SA_AIS_ERR_EXIST,"Section create in ckpt"},    

    [CKPT_SECTION_CREATE_GEN2_T]     = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.special_attr3,tcd.data3,&tcd.size,
                                           SA_AIS_OK,"Section created in ckpt"},    

    [CKPT_SECTION_CREATE_INVALID_PARAM3_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalid_attr,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_INVALID_PARAM,"idsize is zero"},

    [CKPT_SECTION_CREATE_LONG_SECION_ID_SUCCESS_T]    = {&tcd.all_replicas_Writehdl ,&tcd.section_attr_with_long_id,tcd.data1,&tcd.size,
                                           SA_AIS_OK,"Section id long_id Created in all replicas ckpt"},    

    [CKPT_SECTION_CREATE_TOO_LONG_SECION_ID_SUCCESS_T]    = {&tcd.all_replicas_Writehdl ,&tcd.section_attr_with_too_long_id,tcd.data1,&tcd.size,
                                           SA_AIS_ERR_INVALID_PARAM,"Section id long_id Created in all replicas ckpt"},    
};


int test_ckptSectionCreate(int i,CONFIG_FLAG cfg_flg) 
{

  SaAisErrorT rc;
  int result;
  SaTimeT now, duration,temp = 0;
  int64_t gigasec = 1000000000;

  duration = m_GET_TIME_STAMP(now) * gigasec;
  if(API_SectionCreate[i].sectionCreationAttributes != NULL)
  {
     if(API_SectionCreate[i].sectionCreationAttributes->expirationTime != SA_TIME_END)
     {
        temp = API_SectionCreate[i].sectionCreationAttributes->expirationTime;
        duration = API_SectionCreate[i].sectionCreationAttributes->expirationTime + duration ;
        API_SectionCreate[i].sectionCreationAttributes->expirationTime = duration;
     }
  }

retry:
  rc = saCkptSectionCreate(*(API_SectionCreate[i].checkpointHandle),API_SectionCreate[i].sectionCreationAttributes,
                                            API_SectionCreate[i].initialData,*(API_SectionCreate[i].initialDataSize));

  result = cpsv_test_result(rc,API_SectionCreate[i].exp_output,API_SectionCreate[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  if ( rc == SA_AIS_OK )
  {
     if (i == CKPT_SECTION_CREATE_SUCCESS5_T)
     {
        m_TEST_CPSV_PRINTF("Section Id generated is \n");
        tcd.generate_write.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_write.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.generate_read.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_read.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.gen_sec.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.gen_sec.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
#if 0
        free(API_SectionCreate[i].sectionCreationAttributes->sectionId->id);
#endif
     }
     else if (i == CKPT_SECTION_CREATE_GEN_T)
     {
        m_TEST_CPSV_PRINTF("Section Id generated is \n");
        tcd.generate_write1.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_write1.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.generate_read.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_read.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.gen_sec.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.gen_sec.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
     }
  }

  if(API_SectionCreate[i].sectionCreationAttributes != NULL)
  {
     if(API_SectionCreate[i].sectionCreationAttributes->expirationTime != SA_TIME_END)
        API_SectionCreate[i].sectionCreationAttributes->expirationTime = temp;
  }
  return result;
}

int test_red_ckptSectionCreate(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  int prev_rc = SA_AIS_OK;
  int err_not_exist_cnt=0;
  SaTimeT now, duration,temp = 0;
  int64_t gigasec = 1000000000;

  duration = m_GET_TIME_STAMP(now) * gigasec;
  if(API_SectionCreate[i].sectionCreationAttributes != NULL)
  {
     if(API_SectionCreate[i].sectionCreationAttributes->expirationTime != SA_TIME_END)
     {
        temp = API_SectionCreate[i].sectionCreationAttributes->expirationTime;
        duration = API_SectionCreate[i].sectionCreationAttributes->expirationTime + duration ;
        API_SectionCreate[i].sectionCreationAttributes->expirationTime = duration;
     }
  }

  do{
retry:
     rc = saCkptSectionCreate(*(API_SectionCreate[i].checkpointHandle),API_SectionCreate[i].sectionCreationAttributes,
                                            API_SectionCreate[i].initialData,*(API_SectionCreate[i].initialDataSize));

     result = cpsv_test_result(rc,API_SectionCreate[i].exp_output,API_SectionCreate[i].result_string,cfg_flg);
     if(rc == SA_AIS_ERR_NOT_EXIST)
     {
        err_not_exist_cnt++;
        m_TEST_CPSV_PRINTF("ERR_NOT_EXIST_COUNT %d \n",err_not_exist_cnt);
        if((i == CKPT_SECTION_CREATE_GEN_T) && (tcd.redFlag == 1))
        {
          sleep(1);
          if(err_not_exist_cnt == 10)
             break;
          else 
             goto retry;
        }
     }

     if(rc == SA_AIS_ERR_EXIST && prev_rc == SA_AIS_ERR_TIMEOUT && API_SectionCreate[i].exp_output == SA_AIS_OK)
        result = TEST_PASS;
                                                                                                                             
      prev_rc = rc;
  }END_OF_WHILE;

  if ( rc == SA_AIS_OK )
  {
     if (i == CKPT_SECTION_CREATE_SUCCESS5_T)
     {
        m_TEST_CPSV_PRINTF("Section Id generated is \n");
        tcd.generate_write.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_write.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.generate_read.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_read.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.gen_sec.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.gen_sec.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
#if 0
        free(API_SectionCreate[i].sectionCreationAttributes->sectionId->id);
#endif
     }
     else if (i == CKPT_SECTION_CREATE_GEN_T)
     {
        m_TEST_CPSV_PRINTF("Section Id generated is \n");
        tcd.generate_write1.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_write1.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.generate_read.sectionId.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.generate_read.sectionId.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
        tcd.gen_sec.id = API_SectionCreate[i].sectionCreationAttributes->sectionId->id;
        tcd.gen_sec.idLen = API_SectionCreate[i].sectionCreationAttributes->sectionId->idLen;
     }
  }

  if(API_SectionCreate[i].sectionCreationAttributes != NULL)
  {
     if(API_SectionCreate[i].sectionCreationAttributes->expirationTime != SA_TIME_END)
        API_SectionCreate[i].sectionCreationAttributes->expirationTime = temp;
  }
  return result;
}



/**************** Section Expiration Time set ****************/

struct SafCheckpointExpirationTimeSet API_ExpirationSet[] = {
  
    [CKPT_EXP_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,&tcd.section1,APP_TIMEOUT,
                               SA_AIS_ERR_BAD_HANDLE,"Expiration Timeset bad handle"},  

    [CKPT_EXP_INVALID_PARAM_T]   = {&tcd.weak_replica_Writehdl ,&tcd.section3,APP_TIMEOUT,
                                   SA_AIS_ERR_INVALID_PARAM,"Expiration Timeset invalid param"}, 

    [CKPT_EXP_ERR_ACCESS_T]   = {&tcd.all_replicas_Readhdl ,&tcd.section1,APP_TIMEOUT,
                                   SA_AIS_ERR_ACCESS,"Expiration Timeset err access"},    

    [CKPT_EXP_ERR_NOT_EXIST_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalidsection,APP_TIMEOUT,
                                   SA_AIS_ERR_NOT_EXIST,"Expiration Timeset not exist"},    

    [CKPT_EXP_ERR_NOT_EXIST2_T]   = {&tcd.active_colloc_Writehdl ,&tcd.section1,APP_TIMEOUT,
                                   SA_AIS_ERR_NOT_EXIST,"Expiration Timeset no active replica"},  

    [CKPT_EXP_SUCCESS_T]     = {&tcd.all_replicas_Writehdl ,&tcd.section1,SA_TIME_ONE_SECOND,
                                   SA_AIS_OK,"Expiration Timeset to ONE_SECOND for section 11 on ckpt1"},

    [CKPT_EXP_SUCCESS2_T]     = {&tcd.active_colloc_Writehdl ,&tcd.section1,APP_TIMEOUT,
                                   SA_AIS_OK,"Expiration Timeset success for ckpt4"},    
};

int test_ckptExpirationSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  SaTimeT now, duration;
  int64_t gigasec = 1000000000;
                                                                                                                            
  duration = m_GET_TIME_STAMP(now) * gigasec;
                                                                                                                            
  duration = API_ExpirationSet[i].expirationTime + duration ;

retry:
  rc = saCkptSectionExpirationTimeSet(*(API_ExpirationSet[i].checkpointHandle),API_ExpirationSet[i].sectionId,
                                      duration);

  result = cpsv_test_result(rc,API_ExpirationSet[i].exp_output,API_ExpirationSet[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  return result;
}

int test_red_ckptExpirationSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;
  SaTimeT now, duration;
  int64_t gigasec = 1000000000;
                                                                                                                            
  duration = m_GET_TIME_STAMP(now) * gigasec;
  duration = API_ExpirationSet[i].expirationTime + duration ;

  do
  {
    rc = saCkptSectionExpirationTimeSet(*(API_ExpirationSet[i].checkpointHandle),API_ExpirationSet[i].sectionId,
                                      duration);

    result = cpsv_test_result(rc,API_ExpirationSet[i].exp_output,API_ExpirationSet[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}



/**************** Duration Set ********************/


struct SafCheckpointDurationSet API_DurationSet[]={

    [CKPT_SECTION_DUR_SET_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,1000,
                                            SA_AIS_ERR_BAD_HANDLE,"Section Duration set bad handle"},  

    [CKPT_SECTION_DUR_SET_SUCCESS_T]   = {&tcd.all_replicas_Createhdl,1000,
                                            SA_AIS_OK,"Section duration set opened with open call"},  

    [CKPT_SECTION_DUR_SET_ASYNC_SUCCESS_T]   = {&tcd.weak_collocated_Writehdl,SA_TIME_ONE_HOUR,
                                            SA_AIS_OK,"Section Duration set opened with async call"},

    [CKPT_SECTION_DUR_SET_ACTIVE_T]   = {&tcd.active_replica_Createhdl,1000,
                                            SA_AIS_OK,"Section duration set opened with open call"},  

    [CKPT_SECTION_DUR_SET_BAD_OPERATION_T]   = {&tcd.active_replica_Createhdl,1000,
                                            SA_AIS_ERR_BAD_OPERATION,"Section duration set invoked after unlink"},  

    [CKPT_SECTION_DUR_SET_ZERO_T]   = {&tcd.active_replica_Createhdl,0,
                                            SA_AIS_OK,"Section duration set to zero"},  

};


int test_ckptDurationSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;

retry:
  rc = saCkptCheckpointRetentionDurationSet(*(API_DurationSet[i].checkpointHandle),API_DurationSet[i].retentionDuration);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  return(cpsv_test_result(rc,API_DurationSet[i].exp_output,API_DurationSet[i].result_string,cfg_flg));
}

int test_red_ckptDurationSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc = saCkptCheckpointRetentionDurationSet(*(API_DurationSet[i].checkpointHandle),API_DurationSet[i].retentionDuration);
     result = cpsv_test_result(rc,API_DurationSet[i].exp_output,API_DurationSet[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/**************** Checkpoint Write *******************/


struct SafCheckpointWrite API_Write[]={

    [CKPT_WRITE_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_BAD_HANDLE,"Write bad handle"},    

    [CKPT_WRITE_NOT_EXIST_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalid_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_NOT_EXIST,"Write not exist"},    

    [CKPT_WRITE_ERR_ACCESS_T]   = {&tcd.all_replicas_Readhdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_ACCESS,"Write err access"},       

    [CKPT_WRITE_INVALID_PARAM_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalid_write2,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_INVALID_PARAM,"Write inavlid param"},       

    [CKPT_WRITE_SUCCESS_T]     = {&tcd.all_replicas_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write general in ckpt"},

    [CKPT_WRITE_SUCCESS2_T]   = {&tcd.weak_replica_Writehdl ,&tcd.default_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write default in ckpt3"},

    [CKPT_WRITE_SUCCESS3_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write general in collocated ckpt"},

    [CKPT_WRITE_SUCCESS4_T]   = {&tcd.allflags_set_hdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Write general in ckpt"},

    [CKPT_WRITE_SUCCESS5_T]   = {&tcd.all_collocated_Writehdl,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Write in all colloc"},

    [CKPT_WRITE_SUCCESS6_T]   = {&tcd.weak_collocated_Writehdl,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Write into weak colloc"},

    [CKPT_WRITE_NOT_EXIST2_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_NOT_EXIST,"Write general in ckpt"},

    [CKPT_WRITE_SUCCESS_GEN_T]   = {&tcd.active_colloc_sync_nullcb_Writehdl,&tcd.generate_write,&tcd.nOfE,&tcd.ind,
                                      SA_AIS_OK,"Write using gene section id"},

    [CKPT_WRITE_SUCCESS_GEN2_T]   = {&tcd.active_colloc_Writehdl,&tcd.generate_write1,&tcd.nOfE,&tcd.ind,
                                      SA_AIS_OK,"Write using gene section id 1"},

    [CKPT_WRITE_MULTI_IO_T]   = {&tcd.multi_io_hdl ,&tcd.multi_vector[0],&tcd.nOfE2,&tcd.ind,
                                  SA_AIS_OK,"Write into ckpt using multi iovector"},

    [CKPT_WRITE_EXCEED_SIZE_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_write2,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write general in ckpt4"},

    [CKPT_WRITE_SUCCESS7_T]   = {&tcd.all_replicas_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write to ckpt1"},    

    [CKPT_WRITE_MULTI_IO2_T]   = {&tcd.multi_io_hdl ,&tcd.multi_vector[1],&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write into section 21 of multiio"},

    [CKPT_WRITE_MULTI_IO3_T]   = {&tcd.multi_io_hdl ,&tcd.multi_vector[2],&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write into section 31 of multiio"},

    [CKPT_WRITE_SUCCESS8_T]   = {&tcd.active_replica_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_OK,"Write in ckpt2"},       

    [CKPT_WRITE_FIN_HANDLE_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_BAD_HANDLE,"Write general in collocated ckpt"},

    [CKPT_WRITE_NULL_VECTOR_T]   = {&tcd.active_colloc_Writehdl ,NULL,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_INVALID_PARAM,"Write in collocated ckpt with NULL iovector"},

    [CKPT_WRITE_NULL_INDEX_T]   = {&tcd.active_colloc_Writehdl ,&tcd.general_write,&tcd.nOfE,NULL,
                                  SA_AIS_OK,"Write in collocated ckpt NULL err index"},

    [CKPT_WRITE_ERR_NO_RESOURCES_T]     = {&tcd.all_replicas_Writehdl ,&tcd.err_no_res,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_NO_RESOURCES,"Write in ckpt checking for err no resources"},

    [CKPT_WRITE_NOT_EXIST3_T]   = {&tcd.weak_replica_Writehdl ,&tcd.default_write,&tcd.nOfE,&tcd.ind,
                                  SA_AIS_ERR_NOT_EXIST,"Write default in ckpt3"},


};


int test_ckptWrite(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result,j;

retry:
  rc = saCkptCheckpointWrite(*(API_Write[i].checkpointHandle),API_Write[i].ioVector,*(API_Write[i].numberOfElements),
                               API_Write[i].erroneousVectorIndex);

  result = cpsv_test_result(rc,API_Write[i].exp_output,API_Write[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  if ( rc == SA_AIS_OK)
  {
     for(j=0;j<(*(API_Write[i].numberOfElements));j++)
     {
         m_TEST_CPSV_PRINTF(" DataSize:%llu\n",API_Write[i].ioVector[j].dataSize);
#if 0
         m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Write[i].ioVector[j].dataBuffer);
#endif
     }
  }

  return result;
}

int test_red_ckptWrite(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result,j;
  gl_try_again_cnt=0;

  do
  {
     rc = saCkptCheckpointWrite(*(API_Write[i].checkpointHandle),API_Write[i].ioVector,*(API_Write[i].numberOfElements),
                                API_Write[i].erroneousVectorIndex);

     result = cpsv_test_result(rc,API_Write[i].exp_output,API_Write[i].result_string,cfg_flg);
     if ( rc == SA_AIS_OK)
     {
        for(j=0;j<(*(API_Write[i].numberOfElements));j++)
        {
            m_TEST_CPSV_PRINTF(" DataSize:%llu\n",API_Write[i].ioVector[j].dataSize);
#if 0
            m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Write[i].ioVector[j].dataBuffer);
#endif
        }
     }
  }END_OF_WHILE;

  return result;
}


/**************** Checkpoint Read *******************/


struct SafCheckpointRead API_Read[]={

    [CKPT_READ_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_BAD_HANDLE,"Read bad handle"},    

    [CKPT_READ_NOT_EXIST_T]   = {&tcd.all_replicas_Readhdl ,&tcd.invalid_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_NOT_EXIST,"Read invalid"},      

    [CKPT_READ_ERR_ACCESS_T]   = {&tcd.weak_replica_Createhdl ,&tcd.default_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_ACCESS,"Read err access"},      

    [CKPT_READ_SUCCESS_T]     = {&tcd.all_replicas_Readhdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from ckpt1"},

    [CKPT_READ_NOT_EXIST2_T]   = {&tcd.active_colloc_Readhdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_NOT_EXIST,"Read from ckpt4 which has no active replica"},

    [CKPT_READ_SUCCESS2_T]     = {&tcd.active_colloc_Readhdl ,&tcd.generate_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from gen_sec ckpt4"},

    [CKPT_READ_BUFFER_NULL_T]   = {&tcd.weak_replica_Readhdl ,&tcd.buffer_null,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read with null buffer"},      

    [CKPT_READ_INVALID_PARAM_T]   = {&tcd.all_replicas_Readhdl ,&tcd.invalid_read2,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_INVALID_PARAM,"Read invalid param"},    

    [CKPT_READ_INVALID_PARAM2_T]   = {&tcd.all_replicas_Readhdl ,&tcd.invalid_read3,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_INVALID_PARAM,"Read invalid param"},    

    [CKPT_READ_SUCCESS3_T]     = {&tcd.allflags_set_hdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from active replica "},

    [CKPT_READ_SUCCESS4_T]     = {&tcd.all_collocated_Readhdl,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from all colloc replica "},

    [CKPT_READ_SUCCESS5_T]     = {&tcd.active_colloc_Readhdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from active colloc replica "},

    [CKPT_READ_SUCCESS6_T]     = {&tcd.weak_collocated_Readhdl,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from weak colloc replica "},

    [CKPT_READ_MULTI_IO_T]     = {&tcd.multi_io_hdl ,&tcd.multi_read_vector[0],&tcd.nOfE2,&tcd.ind,
                                 SA_AIS_OK,"Read from ckpt using multi iovector"},

    [CKPT_READ_SUCCESS7_T]     = {&tcd.active_replica_Readhdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from ckpt2"},       

    [CKPT_READ_WEAK_REP_T]   = {&tcd.weak_replica_Readhdl ,&tcd.default_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_OK,"Read from default section"},      

    [CKPT_READ_FIN_HANDLE_T]     = {&tcd.all_replicas_Readhdl ,&tcd.general_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_BAD_HANDLE,"Read from ckpt with already finalized handle"},

    [CKPT_READ_NULL_INDEX_T]     = {&tcd.all_replicas_Readhdl ,&tcd.general_read,&tcd.nOfE,NULL,
                                 SA_AIS_OK,"Read from ckpt1"},

    [CKPT_READ_NOT_EXIST3_T]   = {&tcd.weak_replica_Readhdl ,&tcd.default_read,&tcd.nOfE,&tcd.ind,
                                 SA_AIS_ERR_NOT_EXIST,"Read from default section"},      

};

int test_ckptRead(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result,j;

retry:
  rc = saCkptCheckpointRead(*(API_Read[i].checkpointHandle),API_Read[i].ioVector,*(API_Read[i].numberOfElements),API_Read[i].erroneousVectorIndex);
  result = cpsv_test_result(rc,API_Read[i].exp_output,API_Read[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;
  
  if ( rc == SA_AIS_OK)
  {
     for(j=0;j<(*(API_Read[i].numberOfElements));j++)
     {
         m_TEST_CPSV_PRINTF(" DataSize:%llu\n",API_Read[i].ioVector[j].dataSize);
         m_TEST_CPSV_PRINTF(" ReadSize:%llu\n",API_Read[i].ioVector[j].readSize);
#if 1      
         if(i == CKPT_READ_BUFFER_NULL_T) 
         { 
         m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Read[i].ioVector[j].dataBuffer);
	  	 if (API_Read[i].ioVector[j].readSize != 0)
	  	 rc = saCkptIOVectorElementDataFree(*(API_Read[i].checkpointHandle),(API_Read[i].ioVector[j].dataBuffer));
               else
		  rc = SA_AIS_OK;
		 if ( rc == SA_AIS_OK)
	   	  m_TEST_CPSV_PRINTF(" saCkptIOVectorElementDataFree of dataBuffer SUCCESS  \n");
	 	 else
	 	 {
	     		 m_TEST_CPSV_PRINTF(" saCkptIOVectorElementDataFree of dataBuffer FAIL rc-%d\n",rc);
			 result = TEST_FAIL;
	 	 }
         }
#endif
     }
  }

  return result;
}

int test_red_ckptRead(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result,j;
  gl_try_again_cnt=0;

  do
  {
      rc = saCkptCheckpointRead(*(API_Read[i].checkpointHandle),API_Read[i].ioVector,*(API_Read[i].numberOfElements),
                                  API_Read[i].erroneousVectorIndex);
      result = cpsv_test_result(rc,API_Read[i].exp_output,API_Read[i].result_string,cfg_flg);
  
      if ( rc == SA_AIS_OK)
      {
         for(j=0;j<(*(API_Read[i].numberOfElements));j++)
         {
             m_TEST_CPSV_PRINTF(" DataSize:%llu\n",API_Read[i].ioVector[j].dataSize);
             m_TEST_CPSV_PRINTF(" ReadSize:%llu\n",API_Read[i].ioVector[j].readSize);
#if 1      
         if(i == CKPT_READ_BUFFER_NULL_T)
         { 
             m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Read[i].ioVector[j].dataBuffer);
	        if (API_Read[i].ioVector[j].readSize != 0)
	  	 rc = saCkptIOVectorElementDataFree(*(API_Read[i].checkpointHandle),(API_Read[i].ioVector[j].dataBuffer));
               else
		  rc = SA_AIS_OK;
		 if ( rc == SA_AIS_OK)
	   	  m_TEST_CPSV_PRINTF(" saCkptIOVectorElementDataFree of dataBuffer SUCESS  \n");
	 	 else
	     	{
	     		 m_TEST_CPSV_PRINTF(" saCkptIOVectorElementDataFree of dataBuffer FAIL rc-%d\n",rc);
			 result = TEST_FAIL;
	 	 }
         }
#endif
         }
      }
  }END_OF_WHILE;

  return result;
}



/***************** Checkpoint Overwrite *****************/

struct SafCheckpointOverwrite API_Overwrite[]={

    [CKPT_OVERWRITE_BAD_HANDLE_T]   = {&tcd.uninitHandle,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_BAD_HANDLE,"OverWrite with bad handle"},  

    [CKPT_OVERWRITE_SUCCESS_T]   = {&tcd.all_replicas_Writehdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite for section 11 in ckpt"},

    [CKPT_OVERWRITE_NOT_EXIST_T]   = {&tcd.all_replicas_Writehdl ,&tcd.invalidsection,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_NOT_EXIST,"OverWrite not exist"},    

    [CKPT_OVERWRITE_NOT_EXIST2_T]   = {&tcd.active_colloc_Writehdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_NOT_EXIST,"OverWrite not exist"},    

    [CKPT_OVERWRITE_ERR_ACCESS_T]   = {&tcd.active_replica_Readhdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_ACCESS,"OverWrite err access"},    

    [CKPT_OVERWRITE_SUCCESS2_T]   = {&tcd.weak_replica_Writehdl ,&tcd.section3,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite in ckpt3"},

    [CKPT_OVERWRITE_INVALID_PARAM_T]= {&tcd.weak_replica_Writehdl ,&tcd.section3,&tcd.overdata,&tcd.size2,
                                      SA_AIS_ERR_INVALID_PARAM,"OverWrite invalid param"},  

    [CKPT_OVERWRITE_SUCCESS3_T]   = {&tcd.all_collocated_Writehdl,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite for section 11 in all colloc ckpt"},

    [CKPT_OVERWRITE_SUCCESS4_T]   = {&tcd.active_colloc_Writehdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite for section 11 in active colloc ckpt"},

    [CKPT_OVERWRITE_SUCCESS5_T]   = {&tcd.weak_collocated_Writehdl,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite for section 11 in weak colloc ckpt"},

    [CKPT_OVERWRITE_GEN_T]     = {&tcd.active_colloc_Writehdl ,&tcd.gen_sec,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite in generated sec"},    

    [CKPT_OVERWRITE_SUCCESS6_T]   = {&tcd.active_replica_Writehdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_OK,"OverWrite in section 11"},    

    [CKPT_OVERWRITE_FIN_HANDLE_T]   = {&tcd.active_replica_Writehdl ,&tcd.section1,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_BAD_HANDLE,"OverWrite in section 11"},    

    [CKPT_OVERWRITE_NULL_BUF_T]   = {&tcd.weak_replica_Writehdl ,&tcd.section3,NULL,&tcd.size1,
                                      SA_AIS_ERR_INVALID_PARAM,"OverWrite in ckpt3 with NULL buffer"},

    [CKPT_OVERWRITE_NULL_SEC_T]   = {&tcd.weak_replica_Writehdl ,NULL,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_INVALID_PARAM,"OverWrite in ckpt3 with NULL sectionid"},

    [CKPT_OVERWRITE_NOT_EXIST3_T]   = {&tcd.weak_replica_Writehdl ,&tcd.section3,&tcd.overdata,&tcd.size1,
                                      SA_AIS_ERR_NOT_EXIST,"OverWrite in ckpt3"},

};

int test_ckptOverwrite(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;

retry:
  rc = saCkptSectionOverwrite(*(API_Overwrite[i].checkpointHandle),API_Overwrite[i].sectionId,API_Overwrite[i].dataBuffer,
                              *(API_Overwrite[i].dataSize));
  result = cpsv_test_result(rc,API_Overwrite[i].exp_output,API_Overwrite[i].result_string,cfg_flg);
  
  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  if ( rc == SA_AIS_OK)
  {
     m_TEST_CPSV_PRINTF(" DataSize:%llu\n",*API_Overwrite[i].dataSize);
#if 0
     m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Overwrite[i].dataBuffer);
#endif
  }

  return result;
}


int test_red_ckptOverwrite(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  do
  {
     rc = saCkptSectionOverwrite(*(API_Overwrite[i].checkpointHandle),API_Overwrite[i].sectionId,API_Overwrite[i].dataBuffer,
                              *(API_Overwrite[i].dataSize));
     result = cpsv_test_result(rc,API_Overwrite[i].exp_output,API_Overwrite[i].result_string,cfg_flg);
  
     if ( rc == SA_AIS_OK)
     {
        m_TEST_CPSV_PRINTF(" DataSize:%llu\n",*API_Overwrite[i].dataSize);
#if 0
        m_TEST_CPSV_PRINTF(" DataBuffer:%s\n",(char *)API_Overwrite[i].dataBuffer);
#endif
     }
  }END_OF_WHILE;

  return result;
}


/**************** Checkpoint Synchronize ****************/


struct SafCheckpointSynchronize API_Sync[]={

    [CKPT_SYNC_BAD_HANDLE_T]   = {&tcd.uninitHandle,APP_TIMEOUT,
                                 SA_AIS_ERR_BAD_HANDLE,"Synchronize bad handle"},  

    [CKPT_SYNC_NOT_EXIST_T]   = {&tcd.active_colloc_sync_nullcb_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_ERR_NOT_EXIST,"Synchronize not exist"},  

    [CKPT_SYNC_ERR_ACCESS_T]   = {&tcd.active_replica_Readhdl ,APP_TIMEOUT,
                                 SA_AIS_ERR_ACCESS,"Synchronize err access"},  

    [CKPT_SYNC_BAD_OPERATION_T]   = {&tcd.all_replicas_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_ERR_BAD_OPERATION,"Synchronize bad operation"},  

    [CKPT_SYNC_SUCCESS_T]     = {&tcd.active_replica_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_OK,"Synchronize Success"},

    [CKPT_SYNC_SUCCESS2_T]     = {&tcd.active_colloc_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_OK,"Synchronize Success for collocated"},

    [CKPT_SYNC_SUCCESS3_T]     = {&tcd.allflags_set_hdl ,APP_TIMEOUT,
                                  SA_AIS_OK,"Synchronize Success "},        
                                                                                                                                                                      
    [CKPT_SYNC_FIN_HANDLE_T]     = {&tcd.active_colloc_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_ERR_BAD_HANDLE,"Synchronize Success for collocated handle finalized"},

    [CKPT_SYNC_NULL_HANDLE_T]     = {NULL ,APP_TIMEOUT,
                                 SA_AIS_ERR_BAD_HANDLE,"Synchronize Success"},

    [CKPT_SYNC_TRY_AGAIN_T]     = {&tcd.active_replica_Writehdl ,APP_TIMEOUT,
                                 SA_AIS_OK,"Synchronize Success"},

};

int test_ckptSynchronize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaCkptCheckpointHandleT hdl;
  
  if(API_Sync[i].checkpointHandle == NULL)
      hdl = 0;
  else
      hdl = *API_Sync[i].checkpointHandle;

retry:
  rc = saCkptCheckpointSynchronize(hdl,API_Sync[i].timeout);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  if(i == CKPT_SYNC_TRY_AGAIN_T)
  {
     if(rc == SA_AIS_ERR_NOT_EXIST)
     {
        m_TEST_CPSV_PRINTF("Return value is SA_AIS_ERR_NOT_EXIST trying again");
        rc = saCkptCheckpointSynchronize(hdl,API_Sync[i].timeout);
     }
  }

  return(cpsv_test_result(rc,API_Sync[i].exp_output,API_Sync[i].result_string,cfg_flg));
}

int test_red_ckptSynchronize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaCkptCheckpointHandleT hdl;
  gl_try_again_cnt=0;
  int result;
  
  if(API_Sync[i].checkpointHandle == NULL)
      hdl = 0;
  else
      hdl = *API_Sync[i].checkpointHandle;

  do
  {
     rc = saCkptCheckpointSynchronize(hdl,API_Sync[i].timeout);
     if(i == CKPT_SYNC_TRY_AGAIN_T)
     {
        if(rc == SA_AIS_ERR_NOT_EXIST)
        {
           m_TEST_CPSV_PRINTF("Return value is SA_AIS_ERR_NOT_EXIST trying again");
           rc = saCkptCheckpointSynchronize(hdl,API_Sync[i].timeout);
        }
     }

     result = cpsv_test_result(rc,API_Sync[i].exp_output,API_Sync[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/***************** Synchronize Async ********************/


struct SafCheckpointSynchronizeAsync API_SyncAsync[]={

    [CKPT_ASYNC_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,2111,
                                  SA_AIS_ERR_BAD_HANDLE,"Synchronize async bad handle"},  

    [CKPT_ASYNC_NOT_EXIST_T]   = {&tcd.active_colloc_Writehdl ,2112,
                                  SA_AIS_ERR_NOT_EXIST,"Synchronize async not exist"},  

    [CKPT_ASYNC_ERR_ACCESS_T]   = {&tcd.active_replica_Readhdl ,2113,
                                  SA_AIS_ERR_ACCESS,"Synchronize async err access"},    

    [CKPT_ASYNC_BAD_OPERATION_T]   = {&tcd.all_replicas_Writehdl ,2114,
                                  SA_AIS_ERR_BAD_OPERATION,"Synchronize async bad operation"},  

    [CKPT_ASYNC_ERR_INIT_T]   = {&tcd.active_colloc_sync_nullcb_Writehdl ,2115,
                                  SA_AIS_ERR_INIT,"Synchronize async err init"},    

    [CKPT_ASYNC_SUCCESS_T]     = {&tcd.active_replica_Writehdl ,2116,
                                  SA_AIS_OK,"Synchronize async success"},        

    [CKPT_ASYNC_SUCCESS2_T]    = {&tcd.async_all_replicas_hdl, 2117,
                                  SA_AIS_OK,"Synchronize async success"},

    [CKPT_ASYNC_FIN_HANDLE_T]   = {&tcd.active_colloc_Writehdl ,2118,
                                  SA_AIS_ERR_BAD_HANDLE,"Synchronize async bad handle"},  

};

int test_ckptSynchronizeAsync(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  rc=saCkptCheckpointSynchronizeAsync(*(API_SyncAsync[i].checkpointHandle),API_SyncAsync[i].invocation);
  return(cpsv_test_result(rc,API_SyncAsync[i].exp_output,API_SyncAsync[i].result_string,cfg_flg));
}

int test_red_ckptSynchronizeAsync(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptCheckpointSynchronizeAsync(*(API_SyncAsync[i].checkpointHandle),API_SyncAsync[i].invocation);
     result = cpsv_test_result(rc,API_SyncAsync[i].exp_output,API_SyncAsync[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}

/*************** Active Replica Set *****************/


struct SafCheckpointActiveReplicaSet API_ReplicaSet[]={

    [CKPT_SET_BAD_HANDLE_T]   = {&tcd.uninitckptHandle,SA_AIS_ERR_BAD_HANDLE,"Replica set bad handle"},  

    [CKPT_SET_BAD_OPERATION_T]   = {&tcd.all_replicas_Writehdl ,SA_AIS_ERR_BAD_OPERATION,"Replica set bad operation"}, 

    [CKPT_SET_ERR_ACCESS_T]   = {&tcd.active_colloc_Readhdl ,SA_AIS_ERR_ACCESS,"Replica set err access"},  

    [CKPT_SET_SUCCESS_T]     = {&tcd.active_colloc_Writehdl ,SA_AIS_OK,"Replica set active for collocated active replica ckpt"},

    [CKPT_SET_SUCCESS2_T]     = {&tcd.allflags_set_hdl ,SA_AIS_OK,"Replica set active for ckpt1"},

    [CKPT_SET_SUCCESS3_T]     = {&tcd.weak_collocated_Writehdl,SA_AIS_OK,"Replica set active for weak+colloc"},
    
    [CKPT_SET_SUCCESS4_T]     = {&tcd.active_colloc_sync_nullcb_Writehdl,SA_AIS_OK,"Replica set active for active+colloc"},

    [CKPT_SET_SUCCESS5_T]     = {&tcd.all_replicas_Writehdl ,SA_AIS_OK,"Replica set active ckpt1"},   

    [CKPT_SET_BAD_OPERATION2_T] = {&tcd.active_replica_Writehdl ,SA_AIS_ERR_BAD_OPERATION,"Replica set invoked on non-collocated ckpt"}, 

};


int test_ckptReplicaSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  rc=saCkptActiveReplicaSet(*(API_ReplicaSet[i].checkpointHandle));
  return(cpsv_test_result(rc,API_ReplicaSet[i].exp_output,API_ReplicaSet[i].result_string,cfg_flg));
}

int test_red_ckptReplicaSet(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptActiveReplicaSet(*(API_ReplicaSet[i].checkpointHandle));
     result = cpsv_test_result(rc,API_ReplicaSet[i].exp_output,API_ReplicaSet[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/*************** Checkpoint Status Get *****************/


struct SafCheckpointStatusGet API_StatusGet[]={

  [CKPT_STATUS_BAD_HANDLE_T] = {&tcd.uninitckptHandle,&tcd.status,SA_AIS_ERR_BAD_HANDLE,"Status get bad handle"},

  [CKPT_STATUS_ERR_NOT_EXIST_T] = {&tcd.active_colloc_Createhdl ,&tcd.status,SA_AIS_ERR_NOT_EXIST,"Status get active replica not exist"},

  [CKPT_STATUS_INVALID_PARAM_T] = {&tcd.active_replica_Createhdl ,NULL,SA_AIS_ERR_INVALID_PARAM,"Status get invalid param"},

  [CKPT_STATUS_SUCCESS_T] = {&tcd.active_replica_Writehdl ,&tcd.status,SA_AIS_OK,"Status Get for active replica ckpt"},

  [CKPT_STATUS_SUCCESS2_T] = {&tcd.weak_replica_Createhdl ,&tcd.status,SA_AIS_OK,"Status Get for weak replica ckpt"},

  [CKPT_STATUS_SUCCESS3_T] = {&tcd.all_collocated_Writehdl,&tcd.status,SA_AIS_OK,"Status Get for all colloc ckpt"},

  [CKPT_STATUS_SUCCESS4_T] = {&tcd.active_colloc_Writehdl ,&tcd.status,SA_AIS_OK,"Status Get for active colloc ckpt"},

  [CKPT_STATUS_SUCCESS5_T] = {&tcd.weak_collocated_Writehdl,&tcd.status,SA_AIS_OK,"Status Get for weak colloc ckpt"},

  [CKPT_STATUS_SUCCESS6_T] = {&tcd.all_replicas_Writehdl,&tcd.status,SA_AIS_OK,"Status Get for ckpt1"},

  [CKPT_STATUS_SUCCESS7_T] = {&tcd.multi_io_hdl,&tcd.status,SA_AIS_OK,"Status Get for multi iovec"},

  [CKPT_STATUS_ERR_NOT_EXIST2_T] = {&tcd.active_colloc_Writehdl ,&tcd.status,SA_AIS_ERR_NOT_EXIST,"Status get active replica not exist"},


};


int test_ckptStatusGet(int i,CONFIG_FLAG cfg_flg) 
{
  SaCkptCheckpointHandleT localHandle;
  SaAisErrorT rc;
  int result;

  if (API_StatusGet[i].checkpointHandle == NULL) 
     localHandle=0;
  else 
     localHandle=*(API_StatusGet[i].checkpointHandle);

retry:
  rc=saCkptCheckpointStatusGet(localHandle,API_StatusGet[i].checkpointStatus);
  result = cpsv_test_result(rc,API_StatusGet[i].exp_output,API_StatusGet[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  if ( rc == SA_AIS_OK )
     printStatus(*(API_StatusGet[i].checkpointStatus));

  return result;
}

int test_red_ckptStatusGet(int i,CONFIG_FLAG cfg_flg) 
{
  SaCkptCheckpointHandleT localHandle;
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  if (API_StatusGet[i].checkpointHandle == NULL) 
     localHandle=0;
  else 
     localHandle=*(API_StatusGet[i].checkpointHandle);

  do
  {
     rc=saCkptCheckpointStatusGet(localHandle,API_StatusGet[i].checkpointStatus);
     result = cpsv_test_result(rc,API_StatusGet[i].exp_output,API_StatusGet[i].result_string,cfg_flg);

     if ( rc == SA_AIS_OK )
        printStatus(*(API_StatusGet[i].checkpointStatus));
  }END_OF_WHILE;

  return result;
}


/**************** Section Iteration ******************/

struct SafCheckpointIterationInitialize API_IterationInit[] = {

  [CKPT_ITER_INIT_BAD_HANDLE_T] = {&tcd.uninitckptHandle,&tcd.sec_any,APP_TIMEOUT,&tcd.secjunkHandle,
                                   SA_AIS_ERR_BAD_HANDLE,"Iteration Init bad handle"},

  [CKPT_ITER_INIT_FOREVER_T] = {&tcd.active_colloc_sync_nullcb_Writehdl ,&tcd.sec_forever,APP_TIMEOUT,&tcd.secIterationHandle,
                                   SA_AIS_OK,"Iteration Init on ckpt4 FOREVER"},

  [CKPT_ITER_INIT_ANY_T] = {&tcd.all_replicas_Writehdl ,&tcd.sec_any,APP_TIMEOUT,&tcd.secIterationHandle2,
                                   SA_AIS_OK,"Iteration Init on all replicas ckpt ANY"},

  [CKPT_ITER_INIT_INVALID_PARAM_T]= {&tcd.all_replicas_Writehdl ,&tcd.sec_invalid,APP_TIMEOUT,&tcd.secjunkHandle,
                                   SA_AIS_ERR_INVALID_PARAM,"Iteration Init invalid param"},

  [CKPT_ITER_INIT_NOT_EXIST_T] = {&tcd.active_colloc_Writehdl ,&tcd.sec_any,APP_TIMEOUT,&tcd.secjunkHandle,
                                  SA_AIS_ERR_NOT_EXIST,"Iteration Init not exist"},

  [CKPT_ITER_INIT_FINALIZE_T] = {&tcd.all_replicas_Writehdl ,&tcd.sec_any,APP_TIMEOUT,&tcd.secIterationHandle2,
                                   SA_AIS_ERR_BAD_HANDLE,"Iteration Init with finalized handle"},

  [CKPT_ITER_INIT_FOREVER2_T] = {&tcd.all_replicas_Writehdl ,&tcd.sec_forever,APP_TIMEOUT,&tcd.secIterationHandle2,
                                   SA_AIS_OK,"Iteration Init on ckpt FOREVER"},

  [CKPT_ITER_INIT_LEQ_EXP_T] = {&tcd.active_replica_Writehdl ,&tcd.exp_leq,(2*SA_TIME_ONE_DAY),&tcd.secIterationHandle2,
                                   SA_AIS_OK,"Iteration Init on active replica ckpt with sectionsChosen LEQ_EXP"},

  [CKPT_ITER_INIT_GEQ_EXP_T] = {&tcd.active_replica_Writehdl ,&tcd.exp_geq,SA_TIME_ONE_DAY+100,&tcd.secIterationHandle2,
                                   SA_AIS_OK,"Iteration Init on active replica ckpt with sectionsChosen GEQ_EXP"},

  [CKPT_ITER_INIT_NULL_HANDLE_T] = {&tcd.all_replicas_Writehdl ,&tcd.sec_any,APP_TIMEOUT,NULL,
                                   SA_AIS_ERR_INVALID_PARAM,"Iteration Init on all replicas ckpt ANY"},

  [CKPT_ITER_INIT_SUCCESS_T] = {&tcd.active_colloc_Writehdl ,&tcd.sec_any,APP_TIMEOUT,&tcd.secIterationHandle2,
                                  SA_AIS_OK,"Iteration Init active collocated ckpt"},

  [CKPT_ITER_INIT_SUCCESS2_T] = {&tcd.allflags_set_hdl,&tcd.sec_any,APP_TIMEOUT,&tcd.secIterationHandle2,
                                  SA_AIS_OK,"Iteration Init active collocated ckpt"},

};

int test_ckptIterationInit(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaTimeT now, duration;
  int64_t gigasec = 1000000000;
                                                                                                                            
  duration = m_GET_TIME_STAMP(now) * gigasec;

  if(API_IterationInit[i].expirationTime != SA_TIME_END)
     duration = API_IterationInit[i].expirationTime + duration ;

retry:
  rc=saCkptSectionIterationInitialize(*(API_IterationInit[i].checkpointHandle),*(API_IterationInit[i].sectionsChosen),
                                       duration,API_IterationInit[i].sectionIterationHandle);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  return(cpsv_test_result(rc,API_IterationInit[i].exp_output,API_IterationInit[i].result_string,cfg_flg));
}

int test_red_ckptIterationInit(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaTimeT now, duration;
  gl_try_again_cnt=0;
  int64_t gigasec = 1000000000;
  int result;
                                                                                                                            
  duration = m_GET_TIME_STAMP(now) * gigasec;

  if(API_IterationInit[i].expirationTime != SA_TIME_END)
     duration = API_IterationInit[i].expirationTime + duration ;

  do
  {
      rc=saCkptSectionIterationInitialize(*(API_IterationInit[i].checkpointHandle),*(API_IterationInit[i].sectionsChosen),
                                          duration,API_IterationInit[i].sectionIterationHandle);
      result = cpsv_test_result(rc,API_IterationInit[i].exp_output,API_IterationInit[i].result_string,cfg_flg);

  }END_OF_WHILE;

  return result;
}


/**************** Section Iteration Next ******************/


struct SafCheckpointIterationNext API_IterationNext[] = {

  [CKPT_ITER_NEXT_BAD_HANDLE_T] = {&tcd.badHandle,&tcd.secDesc,
                                  SA_AIS_ERR_BAD_HANDLE,"Iteration Next bad handle"}, 

  [CKPT_ITER_NEXT_INVALID_PARAM_T] = {&tcd.secIterationHandle2,NULL,
                                     SA_AIS_ERR_INVALID_PARAM,"Iteration Next NULL sec descriptor"}, 

  [CKPT_ITER_NEXT_NOT_EXIST_T] = {&tcd.secIterationHandle2,&tcd.secDesc,
                                 SA_AIS_ERR_NOT_EXIST,"Iteration Next not exist"}, 

  [CKPT_ITER_NEXT_SUCCESS_T] = {&tcd.secIterationHandle2,&tcd.secDesc,
                               SA_AIS_OK,"Iteration Next success"}, 

  [CKPT_ITER_NEXT_NO_SECTIONS_T] = {&tcd.secIterationHandle2,&tcd.secDesc,
                                    SA_AIS_ERR_NO_SECTIONS,"Iteration Next no further sections"}, 

  [CKPT_ITER_NEXT_FIN_HANDLE_T] = {&tcd.secIterationHandle2,&tcd.secDesc,
                               SA_AIS_ERR_BAD_HANDLE,"Iteration Next finalized handle"}, 

};

int test_ckptIterationNext(int i,CONFIG_FLAG cfg_flg)
{
  SaAisErrorT rc;
  int result;

retry:
  rc=saCkptSectionIterationNext(*(API_IterationNext[i].sectionIterationHandle),API_IterationNext[i].sectionDescriptor);
  result = cpsv_test_result(rc,API_IterationNext[i].exp_output,API_IterationNext[i].result_string,cfg_flg);

  if (rc == SA_AIS_OK)
  {
     m_TEST_CPSV_PRINTF(" Section Id is %s\n",API_IterationNext[i].sectionDescriptor->sectionId.id);
     m_TEST_CPSV_PRINTF(" Section IdLen is %d\n",API_IterationNext[i].sectionDescriptor->sectionId.idLen);
     m_TEST_CPSV_PRINTF(" Expiration Time is %llu\n",API_IterationNext[i].sectionDescriptor->expirationTime);
     m_TEST_CPSV_PRINTF(" Section Size is %llu\n",API_IterationNext[i].sectionDescriptor->sectionSize);
     m_TEST_CPSV_PRINTF(" Section State is %d\n",API_IterationNext[i].sectionDescriptor->sectionState);
     m_TEST_CPSV_PRINTF(" Last Update happened at %llu\n",API_IterationNext[i].sectionDescriptor->lastUpdate);
  }
  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  return result;
}

int test_red_ckptIterationNext(int i,CONFIG_FLAG cfg_flg)
{
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  do
  {
     rc=saCkptSectionIterationNext(*(API_IterationNext[i].sectionIterationHandle),API_IterationNext[i].sectionDescriptor);
     result = cpsv_test_result(rc,API_IterationNext[i].exp_output,API_IterationNext[i].result_string,cfg_flg);
  }END_OF_WHILE;

  if (rc == SA_AIS_OK)
  {
     m_TEST_CPSV_PRINTF(" Section Id is %s\n",API_IterationNext[i].sectionDescriptor->sectionId.id);
     m_TEST_CPSV_PRINTF(" Section IdLen is %d\n",API_IterationNext[i].sectionDescriptor->sectionId.idLen);
     m_TEST_CPSV_PRINTF(" Expiration Time is %llu\n",API_IterationNext[i].sectionDescriptor->expirationTime);
     m_TEST_CPSV_PRINTF(" Section Size is %llu\n",API_IterationNext[i].sectionDescriptor->sectionSize);
     m_TEST_CPSV_PRINTF(" Section State is %d\n",API_IterationNext[i].sectionDescriptor->sectionState);
     m_TEST_CPSV_PRINTF(" Last Update happened at %llu\n",API_IterationNext[i].sectionDescriptor->lastUpdate);
  }

  return result;
}


/**************** Section Iteration Finalize******************/


struct SafCheckpointIterationFinalize API_IterationFin[] = {

  [CKPT_ITER_FIN_BAD_HANDLE_T] = {&tcd.badHandle,SA_AIS_ERR_BAD_HANDLE,"Iteration Finalize bad handle"},

  [CKPT_ITER_FIN_NOT_EXIST_T] = {&tcd.secIterationHandle2,SA_AIS_ERR_NOT_EXIST,"Iteration Finalize not exist"},

  [CKPT_ITER_FIN_FOREVER_T] = {&tcd.secIterationHandle,SA_AIS_OK,"Iteration Finalize on ckpt4 FOREVER"},

  [CKPT_ITER_FIN_HANDLE_T] = {&tcd.secIterationHandle2,SA_AIS_ERR_BAD_HANDLE,"Iteration Finalize on ckpt1 "},

  [CKPT_ITER_FIN_HANDLE2_T] = {&tcd.secIterationHandle2,SA_AIS_OK,"Iteration Finalize on ckpt1 "},

};

int test_ckptIterationFin(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
retry:
  rc=saCkptSectionIterationFinalize(*(API_IterationFin[i].secIterHandle));

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;

  return(cpsv_test_result(rc,API_IterationFin[i].exp_output,API_IterationFin[i].result_string,cfg_flg));
}

int test_red_ckptIterationFin(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptSectionIterationFinalize(*(API_IterationFin[i].secIterHandle));
     result = cpsv_test_result(rc,API_IterationFin[i].exp_output,API_IterationFin[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}

struct SafCheckpointSectionFree API_SectionFree[]={
[CKPT_FREE_GEN_T] = {&tcd.active_colloc_Writehdl ,&tcd.gen_sec,SA_AIS_OK,"Section Free of Gen  "},
};

/**************** Section Delete ********************/



struct SafCheckpointSectionDelete API_SectionDelete[]={

  [CKPT_DEL_BAD_HANDLE_T] = {&tcd.uninitckptHandle,&tcd.section1,SA_AIS_ERR_BAD_HANDLE,"Section Delete bad handle"},

  [CKPT_DEL_ERR_ACCESS_T] = {&tcd.all_replicas_Readhdl ,&tcd.section1,SA_AIS_ERR_ACCESS,"Section Delete error access"},

  [CKPT_DEL_NOT_EXIST_T]= {&tcd.all_replicas_Writehdl ,&tcd.invalidSection,SA_AIS_ERR_NOT_EXIST,"Section Delete section does not exist"},

  [CKPT_DEL_GEN_T] = {&tcd.active_colloc_Writehdl ,&tcd.gen_sec,SA_AIS_OK,"Section Delete from ckpt4"},

  [CKPT_DEL_GEN2_T] = {&tcd.active_colloc_Writehdl ,&tcd.gen_sec_del,SA_AIS_ERR_INVALID_PARAM,"Section Delete invalid param"},

  [CKPT_DEL_NOT_EXIST2_T] = {&tcd.active_colloc_Writehdl ,&tcd.section1,SA_AIS_ERR_NOT_EXIST,"Section Delete not exist no active rep"},

  [CKPT_DEL_SUCCESS_T] = {&tcd.all_replicas_Writehdl ,&tcd.section2,SA_AIS_OK,"Section Delete on ckpt1"},

  [CKPT_DEL_SUCCESS2_T]= {&tcd.weak_replica_Writehdl ,&tcd.section3,SA_AIS_ERR_INVALID_PARAM,"Section Delete invalid param"},

  [CKPT_DEL_NOT_EXIST3_T] = {&tcd.all_replicas_Writehdl ,&tcd.section2,SA_AIS_ERR_NOT_EXIST,"Section Deleted before"},

  [CKPT_DEL_NOT_EXIST4_T] = {&tcd.all_replicas_Writehdl ,&tcd.section1,SA_AIS_ERR_NOT_EXIST,"Section Deleted after exp time"},

  [CKPT_DEL_SUCCESS3_T] = {&tcd.active_colloc_Writehdl ,&tcd.section1,SA_AIS_OK,"Section Deleted in active colloc replica"},

};

int test_ckptSectionDelete(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;

retry:
  rc=saCkptSectionDelete(*(API_SectionDelete[i].checkpointHandle),API_SectionDelete[i].sectionId);
  result = cpsv_test_result(rc,API_SectionDelete[i].exp_output,API_SectionDelete[i].result_string,cfg_flg);

  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;
  
  return result;
}
int test_saCkptSectionIdFree(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
retry:
  rc=saCkptSectionIdFree(*(API_SectionFree[i].checkpointHandle),API_SectionFree[i].sectionId->id);
  result = cpsv_test_result(rc,API_SectionFree[i].exp_output,API_SectionFree[i].result_string,cfg_flg);
  if(rc == SA_AIS_ERR_TRY_AGAIN)
     goto retry;
  return result;
}

int test_red_ckptSectionDelete(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptSectionDelete(*(API_SectionDelete[i].checkpointHandle),API_SectionDelete[i].sectionId);
     result = cpsv_test_result(rc,API_SectionDelete[i].exp_output,API_SectionDelete[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/*************** Checkpoint Unlink ****************/

struct SafCheckpointUnlink API_Unlink[]= {

  [CKPT_UNLINK_BAD_HANDLE_T] = {&tcd.uninitHandle,&tcd.all_replicas_ckpt,SA_AIS_ERR_BAD_HANDLE,"Unlink bad handle"},

  [CKPT_UNLINK_SUCCESS_T] = {&tcd.ckptHandle,&tcd.collocated_ckpt,SA_AIS_OK,"Unlinked ckpt4 collocated"}, 

  [CKPT_UNLINK_INVALID_PARAM_T]= {&tcd.ckptHandle,NULL,SA_AIS_ERR_INVALID_PARAM,"Unlink invalid param"},

  [CKPT_UNLINK_SUCCESS2_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,SA_AIS_OK,"Unlinked ckpt all replicas option"},

  [CKPT_UNLINK_SUCCESS3_T] = {&tcd.ckptHandle,&tcd.active_replica_ckpt,SA_AIS_OK,"Unlinked ckpt active replicas option"},

  [CKPT_UNLINK_NOT_EXIST_T] = {&tcd.ckptHandle2,&tcd.invalidName2,SA_AIS_ERR_NOT_EXIST,"ckpt doesnot exist"},

  [CKPT_UNLINK_SUCCESS4_T] = {&tcd.ckptHandle,&tcd.async_all_replicas_ckpt,SA_AIS_OK,"Unlinked Asynchrously opened ckpt"},

  [CKPT_UNLINK_SUCCESS5_T] = {&tcd.ckptHandle,&tcd.collocated_ckpt,SA_AIS_OK,"Unlinked ckpt collcated replicas"},

  [CKPT_UNLINK_SUCCESS6_T] = {&tcd.ckptHandle,&tcd.all_collocated_ckpt,SA_AIS_OK,"Unlinked ckpt all collocated replica"},

  [CKPT_UNLINK_SUCCESS7_T] = {&tcd.ckptHandle,&tcd.weak_collocated_ckpt,SA_AIS_OK,"Unlinked ckpt weak collocated replica"},

  [CKPT_UNLINK_SUCCESS8_T] = {&tcd.ckptHandle,&tcd.async_active_replica_ckpt,SA_AIS_OK,"Unlinked ckpt active replicas async"},

  [CKPT_UNLINK_SUCCESS9_T] = {&tcd.ckptHandle,&tcd.multi_vector_ckpt,SA_AIS_OK,"Unlinked ckpt all replica"},

  [CKPT_UNLINK_NOT_EXIST2_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt,SA_AIS_ERR_NOT_EXIST,"Unlinked ckpt all replicas option"},

  [CKPT_UNLINK_NOT_EXIST3_T] = {&tcd.ckptHandle,&tcd.active_replica_ckpt,SA_AIS_ERR_NOT_EXIST,"Unlinked ckpt active replica"},

  [CKPT_UNLINK_SUCCESS10_T] = {&tcd.ckptHandle,&tcd.weak_replica_ckpt,SA_AIS_OK,"Unlinked ckpt weak replica"},

  [CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_SUCCESS_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt_with_valid_extended_name_length,SA_AIS_OK,"Unlinked ckpt all replicas with extended name length"},

  [CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_INVALID_PARAM_T] = {&tcd.ckptHandle,&tcd.all_replicas_ckpt_with_invalid_extended_name_length,SA_AIS_ERR_TOO_BIG,"Unlinked ckpt all replicas with invalid extended name length"},

  #if 0
  [CKPT_UNLINK_NOT_EXIST4_T] = {&tcd.ckptHandle,&tcd.collocated_ckpt,SA_AIS_OK,"Unlinked ckpt collcated replicas"},
#endif

};


int test_ckptUnlink(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  int result;
  int retry_counter = 3;

  do
  {
     rc=saCkptCheckpointUnlink(*(API_Unlink[i].checkpointHandle),API_Unlink[i].checkpointName);
     result = cpsv_test_result(rc,API_Unlink[i].exp_output,API_Unlink[i].result_string,cfg_flg);

     retry_counter--;

  } while ((rc == SA_AIS_ERR_TRY_AGAIN || rc == SA_AIS_ERR_TIMEOUT) && (retry_counter != 0));

  return result;
}

int test_red_ckptUnlink(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptCheckpointUnlink(*(API_Unlink[i].checkpointHandle),API_Unlink[i].checkpointName);
     result = cpsv_test_result(rc,API_Unlink[i].exp_output,API_Unlink[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/*************** Checkpoint Close ****************/


struct SafCheckpointClose API_Close[]= {

  [CKPT_CLOSE_BAD_HANDLE_T] = {&tcd.uninitckptHandle,SA_AIS_ERR_BAD_HANDLE,"Close bad handle"},

  [CKPT_CLOSE_SUCCESS_T]= {&tcd.active_colloc_Createhdl ,SA_AIS_OK,"Close of collocated ckpt"},

  [CKPT_CLOSE_SUCCESS2_T] = {&tcd.active_colloc_create_test ,SA_AIS_OK,"Close of collocated ckpt"},

  [CKPT_CLOSE_SUCCESS3_T] = {&tcd.all_replicas_Createhdl,SA_AIS_OK,"Close of ckpt1 "},

  [CKPT_CLOSE_SUCCESS4_T] = {&tcd.all_replicas_Writehdl ,SA_AIS_OK,"Close of ckpt1"},

  [CKPT_CLOSE_SUCCESS5_T] = {&tcd.all_replicas_Readhdl ,SA_AIS_OK,"Close of ckpt1"},

  [CKPT_CLOSE_SUCCESS6_T] = {&tcd.all_replicas_create_after_unlink ,SA_AIS_OK,"Close of ckpt1"},

  [CKPT_CLOSE_SUCCESS7_T] = {&tcd.async_all_replicas_hdl,SA_AIS_OK,"Close of async ckpt"},

  [CKPT_CLOSE_SUCCESS8_T] = {&tcd.multi_io_hdl,SA_AIS_OK,"Close of ckpt"},

  [CKPT_CLOSE_SUCCESS9_T] = {&tcd.active_replica_Createhdl,SA_AIS_OK,"Close of ckpt"},

  [CKPT_CLOSE_SUCCESS10_T] = {&tcd.active_colloc_Writehdl,SA_AIS_OK,"Close of ckpt"},

  [CKPT_CLOSE_SUCCESS11_T] = {&tcd.active_replica_Writehdl,SA_AIS_OK,"Close of ckpt"},

  [CKPT_CLOSE_BAD_HANDLE2_T] = {&tcd.all_replicas_Createhdl ,SA_AIS_ERR_BAD_HANDLE,"Close of ckpt1 - bad handle"},
};


int test_ckptClose(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  rc=saCkptCheckpointClose(*(API_Close[i].checkpointHandle));
  return(cpsv_test_result(rc,API_Close[i].exp_output,API_Close[i].result_string,cfg_flg));
}

int test_red_ckptClose(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  gl_try_again_cnt=0;
  int result;

  do
  {
     rc=saCkptCheckpointClose(*(API_Close[i].checkpointHandle));
     result = cpsv_test_result(rc,API_Close[i].exp_output,API_Close[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}


/*************** Finalization ******************/

struct SafFinalize API_Finalize[]={

  [CKPT_FIN_BAD_HANDLE_T] = {&tcd.uninitHandle,SA_AIS_ERR_BAD_HANDLE,"Finalize bad handle"},

  [CKPT_FIN_SUCCESS_T] = {&tcd.ckptHandle,SA_AIS_OK,"Finalize ckptHandle"},

  [CKPT_FIN_SUCCESS2_T] = {&tcd.ckptHandle2,SA_AIS_OK,"Finalize ckptHandle2"},

  [CKPT_FIN_SUCCESS3_T] = {&tcd.ckptHandle3,SA_AIS_OK,"Finalise ckptHandle3"},

  [CKPT_FIN_NULL_HANDLE_T] = {NULL,SA_AIS_ERR_BAD_HANDLE,"Finalize null handle"},

  [CKPT_FIN_SUCCESS4_T] = {&tcd.ckptHandle4,SA_AIS_OK,"Finalize ckptHandle4"},

  [CKPT_FIN_SUCCESS5_T] = {&tcd.ckptHandle5,SA_AIS_OK,"Finalize ckptHandle5"},

  [CKPT_FIN_SUCCESS6_T] = {&tcd.junkHandle1,SA_AIS_OK,"Finalize junkHandle1"},

  [CKPT_FIN_SUCCESS7_T] = {&tcd.junkHandle2,SA_AIS_OK,"Finalize junkHandle2"},

  [CKPT_FIN_SUCCESS8_T] = {&tcd.junkHandle3,SA_AIS_OK,"Finalize junkHandle3"},

};

int test_ckptFinalize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaCkptHandleT localHandle;
  int result;

  if (API_Finalize[i].ckptHandle == NULL) 
     localHandle=0;
  else 
     localHandle=*(API_Finalize[i].ckptHandle);

  rc=saCkptFinalize(localHandle);
  result = cpsv_test_result(rc,API_Finalize[i].exp_output,API_Finalize[i].result_string,cfg_flg);

  if(rc == SA_AIS_OK)
     m_TEST_CPSV_PRINTF(" Ckpt Handle   :  %llu Finalized  \n",*API_Finalize[i].ckptHandle);

  return result;
}

int test_red_ckptFinalize(int i,CONFIG_FLAG cfg_flg) 
{
  SaAisErrorT rc;
  SaCkptHandleT localHandle;
  gl_try_again_cnt=0;
  int result;

  if (API_Finalize[i].ckptHandle == NULL) 
     localHandle=0;
  else 
     localHandle=*(API_Finalize[i].ckptHandle);

  do
  {
     rc=saCkptFinalize(localHandle);
     result = cpsv_test_result(rc,API_Finalize[i].exp_output,API_Finalize[i].result_string,cfg_flg);

     if(rc == SA_AIS_OK)
        m_TEST_CPSV_PRINTF(" Ckpt Handle   :  %llu Finalized  \n",*API_Finalize[i].ckptHandle);
  }END_OF_WHILE;

  return result;
}



/*************** Dispatch ******************/


struct SafDispatch API_Dispatch[]={

  [CKPT_DISPATCH_BAD_HANDLE_T] = {NULL,SA_DISPATCH_ONE,SA_AIS_ERR_BAD_HANDLE,"Dispatch bad handle"},

  [CKPT_DISPATCH_BAD_HANDLE2_T] = {NULL,SA_DISPATCH_ALL,SA_AIS_ERR_BAD_HANDLE,"Dispatch bad handle"},

  [CKPT_DISPATCH_BAD_HANDLE3_T] = {NULL,SA_DISPATCH_BLOCKING,SA_AIS_ERR_BAD_HANDLE,"Dispatch bad handle"},

  [CKPT_DISPATCH_INVALID_PARAM_T] = {&tcd.ckptHandle,-1,SA_AIS_ERR_INVALID_PARAM,"Dispatch invalid param"}, 

  [CKPT_DISPATCH_ONE_T]= {&tcd.ckptHandle,SA_DISPATCH_ONE,SA_AIS_OK,"Dispatch SA_DISPATCH_ONE"},

  [CKPT_DISPATCH_ALL_T]= {&tcd.ckptHandle,SA_DISPATCH_ALL,SA_AIS_OK,"Dispatch SA_DISPATCH_ALL"},

  [CKPT_DISPATCH_BLOCKING_T] = {&tcd.ckptHandle,SA_DISPATCH_BLOCKING,SA_AIS_OK}, 
  
  [CKPT_DISPATCH_BAD_HANDLE4_T] = {&tcd.uninitHandle,SA_DISPATCH_ONE,SA_AIS_ERR_BAD_HANDLE,"Dispatch bad handle"},
  
  [CKPT_DISPATCH_BAD_HANDLE5_T] = {&tcd.ckptHandle,SA_DISPATCH_ONE,SA_AIS_ERR_BAD_HANDLE,"Dispatch bad handle"},
};

int test_ckptDispatch(int i,CONFIG_FLAG cfg_flg) 
{
  SaCkptHandleT localHandle;
  SaAisErrorT rc;

  if (API_Dispatch[i].ckptHandle == NULL)
     localHandle=0;
  else 
     localHandle=*(API_Dispatch[i].ckptHandle);

  rc = saCkptDispatch(localHandle,API_Dispatch[i].flags);
  return(cpsv_test_result(rc,API_Dispatch[i].exp_output,API_Dispatch[i].result_string,cfg_flg));
}

int test_red_ckptDispatch(int i,CONFIG_FLAG cfg_flg) 
{
  SaCkptHandleT localHandle;
  SaAisErrorT rc;
  int result;
  gl_try_again_cnt=0;

  if (API_Dispatch[i].ckptHandle == NULL)
     localHandle=0;
  else 
     localHandle=*(API_Dispatch[i].ckptHandle);

  do
  {
     rc = saCkptDispatch(localHandle,API_Dispatch[i].flags);
     result = cpsv_test_result(rc,API_Dispatch[i].exp_output,API_Dispatch[i].result_string,cfg_flg);
  }END_OF_WHILE;

  return result;
}
