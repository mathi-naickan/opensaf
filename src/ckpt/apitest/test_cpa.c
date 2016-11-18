#include <stdlib.h>
#include <unistd.h>

#include "osaf/saf/saAis.h"
#include "base/osaf_extended_name.h"
#include "test_cpsv.h"
#include "test_cpsv_conf.h"
#include "base/ncs_main_papi.h"

const char *saf_error_string[] = {
        "SA_AIS_NOT_VALID",
        "SA_AIS_OK",
        "SA_AIS_ERR_LIBRARY",
        "SA_AIS_ERR_VERSION",
        "SA_AIS_ERR_INIT",
        "SA_AIS_ERR_TIMEOUT",
        "SA_AIS_ERR_TRY_AGAIN",
        "SA_AIS_ERR_INVALID_PARAM",
        "SA_AIS_ERR_NO_MEMORY",
        "SA_AIS_ERR_BAD_HANDLE",
        "SA_AIS_ERR_BUSY",
        "SA_AIS_ERR_ACCESS",
        "SA_AIS_ERR_NOT_EXIST",
        "SA_AIS_ERR_NAME_TOO_LONG",
        "SA_AIS_ERR_EXIST",
        "SA_AIS_ERR_NO_SPACE",
        "SA_AIS_ERR_INTERRUPT",
        "SA_AIS_ERR_NAME_NOT_FOUND",
        "SA_AIS_ERR_NO_RESOURCES",
        "SA_AIS_ERR_NOT_SUPPORTED",
        "SA_AIS_ERR_BAD_OPERATION",
        "SA_AIS_ERR_FAILED_OPERATION",
        "SA_AIS_ERR_MESSAGE_ERROR",
        "SA_AIS_ERR_QUEUE_FULL",
        "SA_AIS_ERR_QUEUE_NOT_AVAILABLE",
        "SA_AIS_ERR_BAD_FLAGS",
        "SA_AIS_ERR_TOO_BIG",
        "SA_AIS_ERR_NO_SECTIONS",
};

#if FULL_LOG
#define m_TEST_CPSV_PRINTF printf
#else 
#define m_TEST_CPSV_PRINTF(...) 
#endif

#define VALID_EXTENDED_NAME_LENGTH 400
#define INVALID_EXTENDED_NAME_LENGTH 2050


extern int gl_prev_act;

/********** Ultility Functions ************/

void cpsv_clean_clbk_params()
{
   tcd.open_clbk_hdl = 0;
   tcd.open_clbk_err = 0;
   tcd.open_clbk_invo = 0;
   tcd.sync_clbk_invo = 0;
   tcd.sync_clbk_err = 0;
   tcd.arr_clbk_flag = 0;
   tcd.arr_clbk_err = 0;
}

void printHead(char *str)
{
  m_TEST_CPSV_PRINTF("\n************************************************************* \n");
  m_TEST_CPSV_PRINTF("%s",str);
  m_TEST_CPSV_PRINTF("\n************************************************************* \n");
}

void printResult(int result)
{
  gl_sync_pointnum=1;
  if (result == TEST_PASS)
  {
     m_TEST_CPSV_PRINTF("\n ##### TEST CASE SUCCEEDED #####\n\n");
  }
  else if(result == TEST_FAIL)
  {
     m_TEST_CPSV_PRINTF("\n ##### TEST CASE FAILED #####\n\n");
  }
  else if(result == TEST_UNRESOLVED)
  {
     m_TEST_CPSV_PRINTF("\n ##### TEST CASE UNRESOLVED #####\n\n");
  }

}

bool is_extended_name_enable() {

  char *extended_name_env = getenv("SA_ENABLE_EXTENDED_NAMES");
  if (extended_name_env == 0) 
	  return false;

  if (strcmp(extended_name_env, "1") != 0)
	  return false;

  return true;
}

void handleAssigner(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle)
{
   if (invocation == 1014)
    tcd.all_collocated_Writehdl=checkpointHandle;
   else if (invocation == 1015)
    tcd.all_collocated_Readhdl=checkpointHandle;
   else if (invocation == 1016)
    tcd.weak_collocated_Writehdl=checkpointHandle;
   else if (invocation == 1017)
    tcd.weak_collocated_Readhdl=checkpointHandle;
   else if (invocation == 1018)
    tcd.async_all_replicas_hdl = checkpointHandle;
}

void fill_ckpt_version(SaVersionT *version,SaUint8T rel_code,SaUint8T mjr_ver,SaUint8T mnr_ver)
{
   version->releaseCode = rel_code;
   version->majorVersion = mjr_ver;
   version->minorVersion = mnr_ver;
}
                                                                                                                                                                      
void fill_ckpt_callbacks(SaCkptCallbacksT *clbks,SaCkptCheckpointOpenCallbackT open_clbk,SaCkptCheckpointSynchronizeCallbackT sync_clbk)
{
   clbks->saCkptCheckpointOpenCallback = open_clbk;
   clbks->saCkptCheckpointSynchronizeCallback = sync_clbk;
}
                                                                                                                                                                      
void fill_ckpt_attri(SaCkptCheckpointCreationAttributesT *cr_attr,SaCkptCheckpointCreationFlagsT cr_flags,SaSizeT ckptSize, SaTimeT ret,
                                                         SaUint32T max_sec, SaSizeT max_sec_size, SaSizeT max_sec_id_size)
{
   cr_attr->creationFlags = cr_flags;
   cr_attr->checkpointSize = ckptSize;
   cr_attr->retentionDuration = ret;
   cr_attr->maxSections = max_sec;
   cr_attr->maxSectionSize = max_sec_size;
   cr_attr->maxSectionIdSize = max_sec_id_size;
}
                                                                                                                                                                      
void fill_ckpt_name(SaNameT *name,char *string)
{
   saAisNameLend(string, name);
}

void fill_sec_attri(SaCkptSectionCreationAttributesT *sec_cr_attr,SaCkptSectionIdT *sec,SaTimeT exp_time)
{
   sec_cr_attr->sectionId = sec;
   sec_cr_attr->expirationTime = exp_time;
}
                                                                                                                                                                      
void fill_write_iovec(SaCkptIOVectorElementT *iovec,SaCkptSectionIdT sec,char *buf,SaSizeT size,SaOffsetT offset,SaSizeT read_size)
{
   iovec->sectionId = sec;
   iovec->dataBuffer = buf;
   iovec->dataSize = size;
   iovec->dataOffset = offset;
   iovec->readSize = read_size;
}
                                                                                                                                                                      
void fill_read_iovec(SaCkptIOVectorElementT *iovec,SaCkptSectionIdT sec,char *buf,SaSizeT size,SaOffsetT offset,SaSizeT read_size)
{
   iovec->sectionId = sec;
   iovec->dataBuffer = buf;
   iovec->dataSize = size;
   iovec->dataOffset = offset;
   iovec->readSize = read_size;
}
                                                                                                                                                                      
void AppCkptArrivalCallback(const SaCkptCheckpointHandleT ckptArrivalHandle, SaCkptIOVectorElementT *ioVector, SaUint32T num_of_elem)
{
  int x;

  tcd.arr_clbk_flag = 1;
  for (x=0;x<num_of_elem;x++)
  {
     m_TEST_CPSV_PRINTF(" Section Id is %s\n",(char *)(ioVector[x].sectionId.id)); 
     m_TEST_CPSV_PRINTF(" Section IdLen is %d\n",ioVector[x].sectionId.idLen);
     m_TEST_CPSV_PRINTF(" DataSize:%llu\n",ioVector[x].dataSize);
     m_TEST_CPSV_PRINTF(" ReadSize:%llu\n",ioVector[x].readSize);
     if(ioVector[x].dataBuffer != NULL)
     {
        tcd.arr_clbk_err = 1;
        break;
     }
  }
}

void AppCkptOpenCallback(SaInvocationT invocation, SaCkptCheckpointHandleT checkpointHandle, SaAisErrorT error)
{
  tcd.open_clbk_invo = invocation;
  tcd.open_clbk_err = error;

  if (error != SA_AIS_OK)
  {
    if ((invocation == 1006) && (error == SA_AIS_ERR_NOT_EXIST) )
    {
      m_TEST_CPSV_PRINTF("\n Checkpoint Open Async callback success with invocation %llu and error %s\n",invocation, saf_error_string[error]);
      return;
    }
    else if ((invocation == 1019) && (error == SA_AIS_ERR_EXIST) )
    {
      m_TEST_CPSV_PRINTF("\n Checkpoint Open Async callback success with invocation %llu and error %s\n",invocation,saf_error_string[error]);
      return;
    }
    else
    {
      m_TEST_CPSV_PRINTF("\n %s - Checkpoint Open Async callback unsuccessful with invocation %llu\n",saf_error_string[error],invocation);
      return;
    }
  }
  else
  {
    tcd.open_clbk_hdl = checkpointHandle;
    handleAssigner(invocation,checkpointHandle);
    m_TEST_CPSV_PRINTF("\n Checkpoint Open Async callback success with invocation %llu\n",invocation);
    return;
  }

}

void AppCkptSyncCallback(SaInvocationT invocation, SaAisErrorT error)
{
  tcd.sync_clbk_invo = invocation;
  tcd.sync_clbk_err = error;

  if (error != SA_AIS_OK)
  {
    if ((invocation == 2112) && (error == SA_AIS_ERR_NOT_EXIST) )
    {
      m_TEST_CPSV_PRINTF("\n Checkpoint Sync Callback success with invocation %llu\n",invocation);
      return;
    }
    else
    {
      m_TEST_CPSV_PRINTF("\n %s - Checkpoint Sync Callback unsuccessful with invocation %llu\n",saf_error_string[error],invocation);
      return;
    }
  }
  else
  {
    m_TEST_CPSV_PRINTF("\n Checkpoint Sync Callback success with invocation %llu\n",invocation);
    return;
  }
}

struct cpsv_testcase_data tcd= {
	.section3=SA_CKPT_DEFAULT_SECTION_ID,
	.section4=SA_CKPT_GENERATED_SECTION_ID,
	.section6=SA_CKPT_GENERATED_SECTION_ID,
	.gen_sec_del=SA_CKPT_GENERATED_SECTION_ID,
	.gen_sec=SA_CKPT_GENERATED_SECTION_ID,
	.section7=SA_CKPT_GENERATED_SECTION_ID };

void fill_testcase_data()
{
   /* Variables for initialization */
   fill_ckpt_version(&tcd.version_err,'A',0x01,0x01);
   fill_ckpt_version(&tcd.version_supported,'B',0x02,0x03);
   fill_ckpt_version(&tcd.version_err2,'C',0x01,0x01);

   fill_ckpt_callbacks(&tcd.general_callbks,AppCkptOpenCallback,AppCkptSyncCallback);
   fill_ckpt_callbacks(&tcd.open_null_callbk,NULL,AppCkptSyncCallback);
   fill_ckpt_callbacks(&tcd.synchronize_null_callbk,AppCkptOpenCallback,NULL);
   fill_ckpt_callbacks(&tcd.null_callbks,NULL,NULL);

   /* Variables for ckpt open */
   fill_ckpt_attri(&tcd.all_replicas,SA_CKPT_WR_ALL_REPLICAS,169,100000,2,85,3);
   fill_ckpt_attri(&tcd.smoke_test_replica,SA_CKPT_CHECKPOINT_COLLOCATED |SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
   fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
   fill_ckpt_attri(&tcd.replica_weak,SA_CKPT_WR_ACTIVE_REPLICA_WEAK,85,100,1,85,3);
   fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
   fill_ckpt_attri(&tcd.ckpt_all_collocated_replica,SA_CKPT_WR_ALL_REPLICAS|SA_CKPT_CHECKPOINT_COLLOCATED,169,10,2,85,3);
   fill_ckpt_attri(&tcd.ckpt_weak_collocated_replica,SA_CKPT_WR_ACTIVE_REPLICA_WEAK|SA_CKPT_CHECKPOINT_COLLOCATED,1069,10,2,850,3);
   fill_ckpt_attri(&tcd.multi_io_replica,SA_CKPT_WR_ALL_REPLICAS,4000,50,10,400,3);
   fill_ckpt_attri(&tcd.invalid,SA_CKPT_WR_ACTIVE_REPLICA,300,100,3,85,3);
   fill_ckpt_attri(&tcd.invalid2,SA_CKPT_WR_ALL_REPLICAS|SA_CKPT_WR_ACTIVE_REPLICA,255,100,3,85,3);
   fill_ckpt_attri(&tcd.invalid3,SA_CKPT_WR_ALL_REPLICAS|SA_CKPT_WR_ACTIVE_REPLICA_WEAK,255,100,3,85,3);
   fill_ckpt_attri(&tcd.invalid4,SA_CKPT_WR_ACTIVE_REPLICA|SA_CKPT_WR_ACTIVE_REPLICA_WEAK,255,100,3,85,3);
   fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,140,100,2,85,3);
   fill_ckpt_attri(&tcd.my_app,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ALL_REPLICAS,140,SA_TIME_END,2,85,3);

   fill_ckpt_name(&tcd.all_replicas_ckpt,"safCkpt=all_replicas_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.active_replica_ckpt,"safCkpt=active_replica_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.weak_replica_ckpt,"safCkpt=weak_replica_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.collocated_ckpt,"safCkpt=collocated_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.async_all_replicas_ckpt,"safCkpt=async_all_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.async_active_replica_ckpt,"safCkpt=async_active_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.smoketest_ckpt,"safCkpt=active_ckpt_test,safApp=safCkptService");
   fill_ckpt_name(&tcd.all_collocated_ckpt,"safCkpt=all_collocated_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.weak_collocated_ckpt,"safCkpt=weak_collocated_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.non_existing_ckpt,"safCkpt=nonexisting_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.multi_vector_ckpt,"safCkpt=multi_vector_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.all_replicas_ckpt_large,"safCkpt=all_replicas_large_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.active_replica_ckpt_large,"safCkpt=active_replica_large_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.weak_replica_ckpt_large,"safCkpt=weak_replica_large_ckpt,safApp=safCkptService");
   fill_ckpt_name(&tcd.collocated_ckpt_large,"safCkpt=collocated_large_ckpt,safApp=safCkptService");

   char *ckpt_name = malloc(VALID_EXTENDED_NAME_LENGTH);
   memset(ckpt_name, 0, VALID_EXTENDED_NAME_LENGTH);
   memset(ckpt_name, '.', VALID_EXTENDED_NAME_LENGTH - 1);
   int length = sprintf(ckpt_name, "safCkpt=all_replicas_ckpt_with_valid_extended_name_length");
   *(ckpt_name + length) = '.';
   saAisNameLend(ckpt_name, &tcd.all_replicas_ckpt_with_valid_extended_name_length);

   ckpt_name = malloc(INVALID_EXTENDED_NAME_LENGTH);
   memset(ckpt_name, 0, INVALID_EXTENDED_NAME_LENGTH);
   memset(ckpt_name, '.', INVALID_EXTENDED_NAME_LENGTH - 1);
   length = sprintf(ckpt_name, "safCkpt=all_replicas_ckpt_with_invalid_extended_name_length");
   *(ckpt_name + length) = '.';
   saAisNameLend(ckpt_name, &tcd.all_replicas_ckpt_with_invalid_extended_name_length);

   /* Variables for sec create */
   tcd.sec_id1 = (SaUint8T*)"11";
   tcd.section1.idLen = 2;
   tcd.section1.id = tcd.sec_id1;

   tcd.sec_id2 = (SaUint8T*)"21";
   tcd.section2.idLen = 2;
   tcd.section2.id = tcd.sec_id2;

   tcd.sec_id3 = (SaUint8T*)"31";
   tcd.section5.idLen = 2;
   tcd.section5.id = tcd.sec_id3;

   tcd.invalid_sec.idLen = 8;
   tcd.invalid_sec.id = tcd.sec_id2;

   tcd.inv_sec = (SaUint8T*)"222";
   tcd.invalidsection.idLen = 3;
   tcd.invalidsection.id = tcd.inv_sec;
   
   tcd.sec_id4 = (SaUint8T*)"123";
   tcd.invalidSection.idLen = 3;
   tcd.invalidSection.id = tcd.sec_id4;

   tcd.long_section_id.idLen = 49;
   tcd.long_section_id.id = (SaUint8T *)"long_section_id_size=30_0000000000000000000000000";

   tcd.too_long_section_id.idLen = 50;
   tcd.too_long_section_id.id = (SaUint8T *)"long_section_id_size=30_00000000000000000000000000";

   fill_sec_attri(&tcd.general_attr,&tcd.section1,SA_TIME_ONE_DAY);
   fill_sec_attri(&tcd.expiration_attr,&tcd.section2,SA_TIME_END);
   fill_sec_attri(&tcd.section_attr,&tcd.section3,SA_TIME_ONE_DAY);
   fill_sec_attri(&tcd.special_attr,&tcd.section4,SA_TIME_END);
   fill_sec_attri(&tcd.multi_attr,&tcd.section5,SA_TIME_END);
   fill_sec_attri(&tcd.special_attr2,&tcd.section6,SA_TIME_END);
   fill_sec_attri(&tcd.special_attr3,&tcd.section7,SA_TIME_END);
   fill_sec_attri(&tcd.invalid_attr,&tcd.invalid_sec,SA_TIME_END);
   fill_sec_attri(&tcd.section_attr_with_long_id, &tcd.long_section_id, SA_TIME_END);
   fill_sec_attri(&tcd.section_attr_with_too_long_id, &tcd.too_long_section_id, SA_TIME_END);

   strcpy(tcd.data1,"This is data1");
   strcpy(tcd.data2,"This is data2");
   strcpy(tcd.data3,"This is data3");

   tcd.size = strlen("This is datax");
   tcd.size_zero=0;

   fill_write_iovec(&tcd.generate_write,tcd.section4,"Data in generated section",strlen("Data in generated section"),30,0);
   fill_write_iovec(&tcd.generate_write_large,tcd.section1,tcd.filedata2,350,0,0);
   fill_write_iovec(&tcd.general_write,tcd.section1,"Data in general east or west india is the best",strlen("Data in general east or west india is the best"),0,0);
   fill_write_iovec(&tcd.general_write2,tcd.section1,"Data in general east or west india is the best",
                                                      strlen("Data in general east or west india is the best"),590,0);
   fill_write_iovec(&tcd.generate_write1,tcd.section6,"Data in second generated section",strlen("Data in second generated section"),30,0);
   fill_write_iovec(&tcd.multi_vector[0],tcd.section1,"Data in general east or west india is the best",50,0,0);
   fill_write_iovec(&tcd.multi_vector[1],tcd.section2,(char *)tcd.filedata2,300,0,0);
   fill_write_iovec(&tcd.multi_vector[2],tcd.section5,(char *)tcd.filedata3,300,0,0);
   fill_write_iovec(&tcd.invalid_write,tcd.invalidsection,"invalid",7,0,0);
   fill_write_iovec(&tcd.default_write,tcd.section3,"default data",strlen("default data"),0,0);
   fill_write_iovec(&tcd.invalid_write2,tcd.section1,"Data in general east or west india is the best",1024,0,0);
   tcd.nOfE=1;
   tcd.nOfE2=3;

   fill_read_iovec(&tcd.generate_read,tcd.section1,tcd.buffer,75,0,0);
   fill_read_iovec(&tcd.generate_read_large,tcd.section1,tcd.buffer_large,325,0,0);
   fill_read_iovec(&tcd.general_read,tcd.section1,(char *)tcd.buffer,strlen("Data in general east or west india is the best"),0,0);
   fill_read_iovec(&tcd.default_read,tcd.section3,(char *)tcd.buffer,strlen("default data"),0,0);
   fill_read_iovec(&tcd.multi_read_vector[0],tcd.section1,(char *)tcd.buffer1,200,0,0);
   fill_read_iovec(&tcd.multi_read_vector[1],tcd.section2,(char *)tcd.buffer2,200,0,0);
   fill_read_iovec(&tcd.multi_read_vector[2],tcd.section5,(char *)tcd.buffer3,200,0,0);
   fill_read_iovec(&tcd.invalid_read,tcd.invalidsection,(char *)tcd.buffer,13,0,0);
   fill_read_iovec(&tcd.invalid_read2,tcd.section1,(char *)tcd.buffer,1024,0,0);
   fill_read_iovec(&tcd.invalid_read3,tcd.section1,(char *)tcd.buffer,10,10240,0);
   fill_read_iovec(&tcd.buffer_null,tcd.section3,NULL,13,0,0);
   strcpy(tcd.overdata,"overwritten data into the section");
   tcd.size1 = sizeof("Overwritten data");
   tcd.size2 = 1024;

   tcd.sec_forever = SA_CKPT_SECTIONS_FOREVER;
   tcd.sec_any = SA_CKPT_SECTIONS_ANY;
   tcd.exp_leq = SA_CKPT_SECTIONS_LEQ_EXPIRATION_TIME;
   tcd.exp_geq = SA_CKPT_SECTIONS_GEQ_EXPIRATION_TIME;
   tcd.sec_corrupt = SA_CKPT_SECTIONS_CORRUPTED;
   tcd.sec_invalid = 6;

   fill_ckpt_name(&tcd.invalidName2,"none");
}

void test_cpsv_cleanup(CPSV_CLEANUP_TC_TYPE tc)
{
  int error = TEST_FAIL;

  switch(tc)
  {
     case CPSV_CLEAN_INIT_SUCCESS_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;

     case CPSV_CLEAN_INIT_NULL_CBKS_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS6_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;

     case CPSV_CLEAN_INIT_OPEN_NULL_CBK_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS3_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;
  
     case CPSV_CLEAN_INIT_SYNC_NULL_CBK_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS4_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;
  
     case CPSV_CLEAN_INIT_NULL_CBKS2_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS5_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;
  
     case CPSV_CLEAN_INIT_SUCCESS_HDL2_T:
          error = test_ckptFinalize(CKPT_FIN_SUCCESS2_T,TEST_CONFIG_MODE);
          cpsv_clean_clbk_params();
          break;

  }

  if(error != TEST_PASS)
    m_TEST_CPSV_PRINTF("\n Handle not cleanedup\n");
}

void test_ckpt_cleanup(CPSV_CLEANUP_CKPT_TC_TYPE tc)
{
  int error = TEST_FAIL;
  switch(tc)
  {
     case CPSV_CLEAN_ASYNC_ALL_MODES_SUCCESS_T:
     case CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS4_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_ALL_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_ACTIVE_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS3_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_WEAK_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS10_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS5_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_ALL_COLLOCATED_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS6_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_WEAK_COLLOCATED_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS7_T,TEST_CONFIG_MODE);
          break;
     
     case CPSV_CLEAN_ASYNC_ACTIVE_REPLICAS_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS8_T,TEST_CONFIG_MODE);
          break;
    
     case CPSV_CLEAN_MULTI_VECTOR_CKPT:
          error = test_ckptUnlink(CKPT_UNLINK_SUCCESS9_T,TEST_CONFIG_MODE);
          break;

     case CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT_EXTENDED_NAME:
          error = test_ckptUnlink(CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_SUCCESS_T,TEST_CONFIG_MODE);
          break;
  }

  if(error != TEST_PASS)
    m_TEST_CPSV_PRINTF("\n Unlink failed ckpt not cleanedup\n");
}


/********** START OF TEST CASES ************/
                                                                                                                             

/********* saCkptInitialize TestCases ********/


void cpsv_it_init_01()
{
  int result;
  printHead("To verify that saCkptInitialize initializes checkpoint service");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_02()
{
  int result;
  printHead("To verify initialization when NULL pointer is fed as clbks");

  result = test_ckptInitialize(CKPT_INIT_NULL_CBKS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_NULL_CBKS_T);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_03()
{
  int result;
  printHead("To verify initialization with NULL version pointer");

  result = test_ckptInitialize(CKPT_INIT_NULL_VERSION_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_04()
{
  int result;
  printHead("To verify initialization with NULL as paramaters for clbks and version");

  result = test_ckptInitialize(CKPT_INIT_VER_CBKS_NULL_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_05()
{
  int result;
  printHead("To verify initialization with error version relCode < supported release code");

  fill_ckpt_version(&tcd.version_err,'A',0x01,0x01);
  result = test_ckptInitialize(CKPT_INIT_ERR_VERSION_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_06()
{
  int result;
  printHead("To verify initialization with error version relCode > supported");

  fill_ckpt_version(&tcd.version_err2,'C',0x01,0x01);
  result = test_ckptInitialize(CKPT_INIT_ERR_VERSION2_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_07()
{
  int result;
  printHead("To verify initialization with error version majorVersion > supported");
  fill_ckpt_version(&tcd.version_err2,'B',0x03,0x01);

  result = test_ckptInitialize(CKPT_INIT_ERR_VERSION2_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_08()
{
  int result;
  printHead("To verify init whether correct version is returned when wrong version is passed");
  fill_ckpt_version(&tcd.version_err2,'B',0x03,0x01);

  result = test_ckptInitialize(CKPT_INIT_ERR_VERSION2_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS)
  {
     if(tcd.version_err2.releaseCode == 'B' &&
        tcd.version_err2.majorVersion == 2 &&
        tcd.version_err2.minorVersion == 3)
        result = TEST_PASS;
      else
        result = TEST_FAIL;
  }
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_09()
{
  int result;
  printHead("To verify saCkptInitialize with NULL handle");

  result = test_ckptInitialize(CKPT_INIT_NULL_HDL_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_init_10()
{
  int result;
  printHead("To verify saCkptInitialize with one NULL clbk");

  result = test_ckptInitialize(CKPT_INIT_OPEN_NULL_CBK_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_OPEN_NULL_CBK_T);
  printResult(result);

  test_validate(result, TEST_PASS);
}

/****** saCkptSelectionObjectGet *****/

void cpsv_it_sel_01()
{
  int result;
  printHead("To verify saCkptSelectionObjectGet returns operating system handle");

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
      goto final;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sel_02()
{
  int result;
  printHead("To verify SelObj api with uninitialized handle");

  result = test_ckptSelectionObject(CKPT_SEL_UNINIT_HDL_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_sel_03()
{
  int result;
  printHead("To verify SelObj api with finalized handle");

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptSelectionObject(CKPT_SEL_FIN_HANDLE_T,TEST_NONCONFIG_MODE);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sel_04()
{
  int result;
  printHead("To verify SelObj api with NULL handle");

  result = test_ckptSelectionObject(CKPT_SEL_HDL_NULL_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}

void cpsv_it_sel_05()
{
  int result;
  printHead("To verify SelObj api with NULL selObj");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptSelectionObject(CKPT_SEL_NULL_OBJ_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sel_06()
{
  int result;
  printHead("To verify SelObj api with both NULL handle and selObj");

  result = test_ckptSelectionObject(CKPT_SEL_HDL_OBJ_NULL_T,TEST_NONCONFIG_MODE);
  printResult(result);

  test_validate(result, TEST_PASS);
}


/******* saCkptDispatch ********/

void cpsv_it_disp_01()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify that callback is invoked when dispatch is called with DISPATCH_ONE");

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1018 && tcd.open_clbk_err == SA_AIS_OK)
      result = TEST_PASS;
   else
      result = TEST_FAIL;

   test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_MODES_SUCCESS_T);
      
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_02()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify that callback is invoked when dispatch is called with DISPATCH_ALL");

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 90;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);

  result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_CONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1018 && tcd.open_clbk_err == SA_AIS_OK)
      result = TEST_PASS;
  else
  {
      result = TEST_FAIL;
      goto final2;
  }

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.sync_clbk_err == SA_AIS_OK && tcd.sync_clbk_invo == 2117)
      result = TEST_PASS;
   else
      result = TEST_FAIL;

final3:
     test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_MODES_SUCCESS_T);
final2:
     test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_03()
{
  int result;
  printHead("To verify that dispatch blocks for callback when dispatchFlag is DISPATCH_BLOCKING");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  cpsv_createthread(&tcd.ckptHandle);

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  sleep(5);
  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  sleep(5);
  if(tcd.open_clbk_invo == 1018 && tcd.sync_clbk_invo == 2117 && tcd.open_clbk_err == SA_AIS_OK
                                && tcd.sync_clbk_err == SA_AIS_OK)
      result = TEST_PASS;
   else
      result = TEST_FAIL;

final3:
     test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_MODES_SUCCESS_T);
final2:
     test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_04()
{
  int result;
  printHead("To verify dispatch with invalid dispatchFlag");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptDispatch(CKPT_DISPATCH_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_05()
{
  int result;
  printHead("To verify dispatch with NULL ckptHandle and DISPATCH_ONE");
  result = test_ckptDispatch(CKPT_DISPATCH_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_06()
{
  int result;
  printHead("To verify dispatch with NULL ckptHandle and DISPATCH_ALL");
  result = test_ckptDispatch(CKPT_DISPATCH_BAD_HANDLE2_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_07()
{
  int result;
  printHead("To verify dispatch with NULL ckptHandle and DISPATCH_BLOCKING");
  result = test_ckptDispatch(CKPT_DISPATCH_BAD_HANDLE3_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_08()
{
  int result;
  printHead("To verify dispatch with invalid ckptHandle and DISPATCH_ONE");
  result = test_ckptDispatch(CKPT_DISPATCH_BAD_HANDLE4_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_disp_09()
{
  int result;
  printHead("To verify dispatch after finalizing ckpt service");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptDispatch(CKPT_DISPATCH_BAD_HANDLE5_T,TEST_NONCONFIG_MODE);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******* saCkptFinalize ********/


void cpsv_it_fin_01()
{
  int result;
  printHead("To verify that finalize closes association between service and process");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_BAD_HANDLE2_T,TEST_NONCONFIG_MODE);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_fin_02()
{
  int result;
  printHead("To verify finalize when service is not initialized");
  result = test_ckptFinalize(CKPT_FIN_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_fin_03()
{
  int result;
  fd_set read_fd;
  struct timeval tv;

  printHead("To verify that after finalize selobj gets invalid");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 5;
  tv.tv_usec = 0;
                                                                                                                                                                      
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
  if(result == -1)
     result = TEST_PASS;
  else
     result = TEST_FAIL;
  goto final1;

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_fin_04()
{
  int result;
  printHead("To verify that after finalize the handle is invalid");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptClose(CKPT_CLOSE_BAD_HANDLE2_T,TEST_NONCONFIG_MODE);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/********* saCkptCheckpointOpen ************/


void cpsv_it_open_01()
{
  int result;
  printHead("To verify opening a ckpt with synchronous update option");

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
 
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_02()
{
  int result;
  printHead("To verify opening a ckpt with asynchronous update option");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_03()
{
  int result;
  printHead("To verify opening an existing ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_04()
{
  int result;
  printHead("To verify opening an existing ckpt when its creation attributes match");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_05()
{
  printHead("To verify opening an nonexisting ckpt");
  int result;
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_BAD_NAME_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_06()
{
  int result;
  printHead("To verify opening an ckpt when SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR is NOT NULL");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_INVALID_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_07()
{
  int result;
  printHead("To verify opening an ckpt when SA_CKPT_CHECKPOINT_CREATE flag set and CR_ATTR is NULL");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_NULL_ATTR_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_08()
{
  int result;
  printHead("To verify opening an ckpt with ckptSize > maxSec * maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ATTR_INVALID_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_open_10()
{
  int result;
  printHead("To verify opening an ckpt when ALL_REPLICAS|ACTIVE_REPLICA specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ATTR_INVALID2_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_11()
{
  int result;
  printHead("To verify opening an ckpt when ALL_REPLICAS|ACTIVE_REPLICA_WEAK specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ATTR_INVALID3_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_12()
{
  int result;
  printHead("To verify opening an ckpt when ACTIVE_REPLICA|ACTIVE_REPLICA_WEAK specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ATTR_INVALID4_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_13()
{
  int result;
  printHead("To verify opening an ckpt when NULL name is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_14()
{
  int result;
  printHead("To verify opening an ckpt with SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR NOT NULL");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_INVALID_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_15()
{
  int result;
  printHead("To verify opening an ckpt when NULL is passed as checkpointHandle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_INVALID_PARAM2_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_16()
{
  int result;
  printHead("To verify opening an ckpt when ALL_REPLICAS|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_ALL_COLLOC_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_ALL_COLLOCATED_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_17()
{
  int result;
  printHead("To verify opening an ckpt when REPLICA_WEAK|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_WEAK_COLLOC_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_COLLOCATED_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_18()
{
  int result;
  printHead("To verify opening an ckpt when ACTIVE_REPLICA|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_19()
{
  int result;
  printHead("To verify opening an ckpt when invalid openFlags is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpen(CKPT_OPEN_WEAK_BAD_FLAGS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_20()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different creation attributes");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ERR_EXIST_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_21()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different ckpt size");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1100,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_ERR_EXIST2_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_22()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different retention duration");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,1000000,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_SUCCESS_EXIST2_T,TEST_CONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_23()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different maxSections");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,3,600,3);
  result = test_ckptOpen(CKPT_OPEN_ERR_EXIST2_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_24()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,800,3);
  result = test_ckptOpen(CKPT_OPEN_ERR_EXIST2_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_25()
{
  int result;
  printHead("To verify creating a ckpt when already exists but with different maxSectionIdSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,5);
  result = test_ckptOpen(CKPT_OPEN_ERR_EXIST2_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_26()
{
  int result;
  printHead("To verify creating a ckpt when ckpt service has not been initialized");
  result = test_ckptOpen(CKPT_OPEN_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_27()
{
  int result;
  printHead("To verify creating a ckpt when NULL open clbk is provided during initialization");
  result = test_ckptInitialize(CKPT_INIT_OPEN_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_INIT_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_OPEN_NULL_CBK_T);
final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_28()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync with synchronous update option");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1002 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

   test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT);
      
final2:
     test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_29()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync with asynchronous update option");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ACTIVE_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1003 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ACTIVE_REPLICAS_CKPT);
      
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_30()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync to open an existing ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpen(CKPT_OPEN_WEAK_COLLOC_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_WEAK_COLLOC_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1016 && tcd.open_clbk_err == SA_AIS_OK)
      result = TEST_PASS;
   else
      result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_31()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync to open an existing ckpt if its creation attributes match");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_COLLOCATE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1005 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_32()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync to open an nonexisting ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_BAD_NAME_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1006 && tcd.open_clbk_err == SA_AIS_ERR_NOT_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final2:
   test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_33()
{
  int result;
  printHead("To verify openAsync with SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR NOT NULL");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_34()
{
  int result;
  printHead("To verify openAsync with SA_CKPT_CHECKPOINT_CREATE flag set and NULL CR_ATTR");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM2_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_35()
{
  int result;
  printHead("To verify openAsync with ckptSize > maxSec * maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  fill_ckpt_attri(&tcd.invalid,SA_CKPT_WR_ACTIVE_REPLICA,300,100,3,85,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM3_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_36()
{
  int result;
  printHead("To verify openAsync when ALL_REPLICAS|ACTIVE_REPLICA specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  fill_ckpt_attri(&tcd.invalid2,SA_CKPT_WR_ALL_REPLICAS|SA_CKPT_WR_ACTIVE_REPLICA,255,100,3,85,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM4_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_37()
{
  int result;
  printHead("To verify openAsync when ALL_REPLICAS|ACTIVE_REPLICA_WEAK specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  fill_ckpt_attri(&tcd.invalid3,SA_CKPT_WR_ALL_REPLICAS|SA_CKPT_WR_ACTIVE_REPLICA_WEAK,255,100,3,85,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM5_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_38()
{
  int result;
  printHead("To verify openAsync when ACTIVE_REPLICA|ACTIVE_REPLICA_WEAK specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  fill_ckpt_attri(&tcd.invalid4,SA_CKPT_WR_ACTIVE_REPLICA|SA_CKPT_WR_ACTIVE_REPLICA_WEAK,255,100,3,85,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM6_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_39()
{
  int result;
  printHead("To verify openAsync when NULL name is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM7_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_40()
{
  int result;
  printHead("To verify openAsync for reading/writing which is not created");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_INVALID_PARAM2_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_41()
{
  int result;
  printHead("To verify openAsync when NULL invocation is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_NULL_INVOCATION,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_42()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when ALL_REPLICAS|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_COLLOC_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_COLLOC_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1014 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_43()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when REPLICA_WEAK|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_COLLOC_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_WEAK_COLLOC_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1016 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_44()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when ACTIVE_REPLICA|COLLOCATED is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_COLLOCATE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1005 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_45()
{
  int result;
  printHead("To verify openAsync when invalid openFlags is specified");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_BAD_FLAGS_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_46()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different creation attributes");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_47()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different checkpointSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1100,100,2,600,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_48()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different retentionDuration");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,1000000,2,600,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_49()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different maxSections");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,3,600,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_50()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,800,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_51()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync when already exists but with different maxSectionIdSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,5);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_52()
{
  int result;
  printHead("To verify creating a ckpt with invalid creation flags");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_CHECKPOINT_COLLOCATED,1069,1000000,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_INVALID_CR_FLAGS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_53()
{
  int result;
  printHead("To verify creating a ckpt with invalid creation flags");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.invalid_collocated,0,1069,1000000,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_INVALID_CR_FLAGS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_54()
{
  int result;
  printHead("To verify creating a ckpt with invalid creation flags");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.invalid_collocated,16,1069,1000000,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_INVALID_CR_FLAGS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_55()
{
  int result;
  printHead("To verify creating a ckpt with valid extended name length");

  /* Skip the test if Extended Name is not enable */
  if (is_extended_name_enable() == false)
	  return test_validate(TEST_PASS, TEST_PASS);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_SUCCESS_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_56()
{
  int result;
  printHead("To verify creating a ckpt with invalid extended name length");

  /* Skip the test if Extended Name is not enable */
  if (is_extended_name_enable() == false)
	  return test_validate(TEST_PASS, TEST_PASS);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_57()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openAsync a ckpt with valid extended name length");

  /* Skip the test if Extended Name is not enable */
  if (is_extended_name_enable() == false)
	  return test_validate(TEST_PASS, TEST_PASS);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1021 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT_EXTENDED_NAME);
      
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_open_58()
{
  int result;
  printHead("To verify openAsync a ckpt with invalid extended name length");

  /* Skip the test if Extended Name is not enable */
  if (is_extended_name_enable() == false)
	  return test_validate(TEST_PASS, TEST_PASS);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_CREATE_EXTENDED_NAME_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/****** saCkptCheckpointClose *******/

void cpsv_it_close_01() 
{
  int result;
  printHead("To verify Closing of the checkpoint designated by checkpointHandle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
  goto final2;
 
final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_02() 
{
  int result;
  printHead("To verify that after close checkpointHandle is no longer valid");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptClose(CKPT_CLOSE_BAD_HANDLE2_T,TEST_NONCONFIG_MODE);
  goto final2;
 
final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_03()
{
  int result;
  printHead("To test Closing of the checkpoint after unlink");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptClose(CKPT_CLOSE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_04() 
{
  int result;
  printHead("To test Closing of the checkpoint before unlink");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.all_replicas,SA_CKPT_WR_ALL_REPLICAS,169,1000000000000000ULL,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS3_T,TEST_CONFIG_MODE);

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_05() 
{
  int result;
  printHead("To test close api when calling process is not registered with checkpoint service");
  result = test_ckptClose(CKPT_CLOSE_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_06() 
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify that close cancels all pending callbacks");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
                                                                                                                            
  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                            
  cpsv_clean_clbk_params();
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                            
  tv.tv_sec = 30;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  m_TEST_CPSV_PRINTF("\n Select returned %d \n",result);

  result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1018 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }
                                                                                                                            
  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                            
  tv.tv_sec = 30;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  m_TEST_CPSV_PRINTF("\n Select ckptSynchronizeAsync returned  %d \n",result);

  result = test_ckptClose(CKPT_CLOSE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                            
  tv.tv_sec = 120;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
  if(result == 0)
     result = TEST_FAIL;
  else
  {
     m_TEST_CPSV_PRINTF("\n Select ckptClose returned  %d \n",result);
     /* Initialize sync_clbk_invo */
     tcd.sync_clbk_invo = 0;
     result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
     /* verify that the sync_callback is not invoked */
     if(tcd.sync_clbk_invo == 0)
        result = TEST_PASS;
     else
        result = TEST_FAIL;
  }
  goto final2;
                                                                                                                            
final3:
  test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_07()
{
  int result;
  printHead("To test that the checkpoint can be opened after close and before checkpoint expiry");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,SA_TIME_ONE_HOUR,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS9_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  m_TEST_CPSV_PRINTF("\n Wait for a while  ... \n");
  sleep(60);

  result = test_red_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_red_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_close_08()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  SaCkptCheckpointHandleT hdl1;
  printHead("To verify that close cancels pending callbacks for that handle only");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  cpsv_clean_clbk_params();
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 30;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  m_TEST_CPSV_PRINTF("\n Select returned %d \n",result);

  result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1018 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  hdl1 = tcd.async_all_replicas_hdl;

  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ALL_MODES_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 30;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  m_TEST_CPSV_PRINTF("\n Select returned %d \n",result);

  result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1018 && tcd.open_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final2;
  }

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 30;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  m_TEST_CPSV_PRINTF("\n First Sync returned %d \n",result);

  tcd.async_all_replicas_hdl = hdl1;
  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  sleep(5);
  result = test_ckptClose(CKPT_CLOSE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 120;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
  if(result == 0)
     result = TEST_PASS;
  else
  {
     m_TEST_CPSV_PRINTF("\n Select returned %d \n",result);
     result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
     if(tcd.sync_clbk_invo == 2117)
        result = TEST_PASS;
     else
        result = TEST_FAIL;
  }
  goto final2;
                                                                                                                             
final3:
  test_ckpt_cleanup(CPSV_CLEAN_ASYNC_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}
                                                                                                                             


/****** saCkptCheckpointUnlink *******/


void cpsv_it_unlink_01() 
{
  int result;
  printHead("To test Unlink deletes the existing checkpoint from the cluster");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptOpen(CKPT_OPEN_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_02() 
{
  int result;
  printHead("To test that name is no longer valid after unlink");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_03() 
{
  int result;
  printHead("To test that the ckpt with same name can be created after unlinking and yet not closed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_04() 
{
  int result;
  printHead("To test that ckpt gets deleted immediately when no process has opened it");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.all_replicas,SA_CKPT_WR_ALL_REPLICAS,169,1000000000000000ULL,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS3_T,TEST_CONFIG_MODE);

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST2_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_05() 
{
  int result;
  printHead("To test that the ckpt can be accessed after unlinking till it is closed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_MULTI_IO_REPLICA_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS9_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_MULTI_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptClose(CKPT_CLOSE_SUCCESS8_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
 
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_MULTI4_T,TEST_NONCONFIG_MODE);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_06()  
{
  int result;
  printHead("To test unlink after retention duration of ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS9_T,TEST_CONFIG_MODE);

  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST3_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_07() /* To test unlink after close */
{
  cpsv_it_close_04(); /* To test Closing of the checkpoint before unlink*/
}

void cpsv_it_unlink_08() 
{
  int result;
  printHead("To test unlink with correct handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS3_T,TEST_NONCONFIG_MODE);
 
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_09() 
{
  int result;
  printHead("To test unlink with uninitialized handle");
  result = test_ckptUnlink(CKPT_UNLINK_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_10() /* invoke unlink in child process */
{
  printHead("To test unlink in the child process - NOT SUPPORTED");
/* fork not supported */
  test_validate(TEST_PASS, TEST_PASS);
}

void cpsv_it_unlink_11() 
{
  int result;
  printHead("To test unlink with NULL ckpt name");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptUnlink(CKPT_UNLINK_INVALID_PARAM_T,TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_unlink_12() 
{
  int result;
  printHead("To test unlink a ckpt with invalid extended name");

  /* Skip the test if Extended Name is not enable */
  if (is_extended_name_enable() == false)
	  return test_validate(TEST_PASS, TEST_PASS);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final;

  result = test_ckptUnlink(CKPT_UNLINK_ALL_REPLICAS_EXTENDED_NAME_INVALID_PARAM_T, TEST_NONCONFIG_MODE);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

final:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/******* saCkptRetentionDurationSet ******/


void cpsv_it_rdset_01() 
{
  int result;
  printHead("To test that invoking rdset changes the rd for the checkpoint");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,SA_TIME_ONE_HOUR,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptDurationSet(CKPT_SECTION_DUR_SET_ACTIVE_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS9_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  sleep(1);
  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST3_T,TEST_NONCONFIG_MODE);
  goto final2;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_rdset_02()/* To test ckpt gets deleted if no process opened during ret. duration*/
{
  cpsv_it_rdset_01();
}

void cpsv_it_rdset_03() 
{
  int result;
  printHead("To test rdset api with invalid handle");
  result = test_ckptDurationSet(CKPT_SECTION_DUR_SET_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_rdset_04() 
{
  int result;
  printHead("To test when rdset is invoked after unlink");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,SA_TIME_ONE_HOUR,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptDurationSet(CKPT_SECTION_DUR_SET_BAD_OPERATION_T,TEST_NONCONFIG_MODE);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_rdset_05() 
{
  int result;
  printHead("To test when ret.duration to be set is zero");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,SA_TIME_ONE_HOUR,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptDurationSet(CKPT_SECTION_DUR_SET_ZERO_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS9_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptUnlink(CKPT_UNLINK_NOT_EXIST3_T,TEST_NONCONFIG_MODE);
  goto final2;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_rdset_06() /* child process invokes rdset api */
{
  printHead("child process invokes rdset api");
}


/******* saCkptActiveReplicaSet ********/


void cpsv_it_repset_01()
{
  int result;
  printHead("To verify that local replica becomes active after invoking replicaset");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_NOT_EXIST_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_repset_02() 
{
  int result;
  printHead("To test that replica can be set for only for collocated ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,SA_TIME_ONE_HOUR,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_BAD_OPERATION2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  fill_ckpt_attri(&tcd.active_replica,SA_CKPT_WR_ACTIVE_REPLICA,169,100,2,85,3);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_repset_03()  
{
  int result;
  printHead("To test replica can be set active only for asynchronous ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_BAD_OPERATION_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_repset_04() 
{
  int result;
  printHead("To test replica set when handle with read option is passed to the api");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_repset_05()
{
  int result;
  printHead("To verify replicaset when an invalid handle is passed");
  result = test_ckptReplicaSet(CKPT_SET_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}



/******* saCkptCheckpointStatusGet *******/


void cpsv_it_status_01() 
{
  int result;
  printHead("To test that this api can be used to retrieve the checkpoint status");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.replica_weak,SA_CKPT_WR_ACTIVE_REPLICA_WEAK,85,100,1,85,3);
  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptStatusGet(CKPT_STATUS_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.status.numberOfSections == 1 &&
                           tcd.status.memoryUsed == 0 &&
                           tcd.status.checkpointCreationAttributes.creationFlags == SA_CKPT_WR_ACTIVE_REPLICA_WEAK &&
                           tcd.status.checkpointCreationAttributes.checkpointSize == 85 &&
                           tcd.status.checkpointCreationAttributes.retentionDuration == 100 &&
                           tcd.status.checkpointCreationAttributes.maxSections == 1 &&
                           tcd.status.checkpointCreationAttributes.maxSectionSize == 85) 
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_status_02() 
{
  int result;
  printHead("To test when arbitrary handle is passed to status get api");
  result = test_ckptStatusGet(CKPT_STATUS_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_status_03() 
{
  int result;
  printHead("To test status get when there is no active replica");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptStatusGet(CKPT_STATUS_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_status_05() 
{
    cpsv_it_status_01();
}

void cpsv_it_status_06() 
{
  int result;
  printHead("To test status get when invalid param is passed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptStatusGet(CKPT_STATUS_INVALID_PARAM_T,TEST_NONCONFIG_MODE);
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******* saCkptSectionCreate ********/


void cpsv_it_seccreate_01() 
{
  int result;
  printHead("To verify section create with arbitrary handle");
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_02() 
{
  int result;
  printHead("To verify section create with correct handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_03() 
{
  int result;
  printHead("To verify creating sections greater than maxSections");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_NO_SPACE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_seccreate_04() 
{
   cpsv_it_expset_02();
}

void cpsv_it_seccreate_05() 
{
    cpsv_it_seccreate_02(); /* can be extended for two node case open the same checkpoint, delete the first replica, u will still have the section in the ckpt*/ 
}


void cpsv_it_seccreate_07() 
{
  int result;
  printHead("To verify section create when there is no active replica");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_09() 
{
  int result;
  printHead("To verify section create when ckpt is not opened in write mode");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_10() 
{
  int result;
  printHead("To verify section create when maxSections is 1");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_11() 
{
  int result;
  printHead("To verify section create with NULL section creation attributes");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_12() 
{
  int result;
  printHead("To verify section create with NULL initial data and datasize > 0");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_NULL_DATA_SIZE_NOTZERO_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_13() 
{
  int result;
  printHead("To verify section create with NULL initial data and datasize = 0");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_NULL_DATA_SIZE_ZERO_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_14()
{
  int result;
  printHead("To verify section create with sectionId which is already present");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_ERR_EXIST_T,TEST_CONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_15()
{
  int result;
  printHead("To verify section create with generated sectionId");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_GEN_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_GEN_EXIST_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptStatusGet(CKPT_STATUS_SUCCESS4_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.status.numberOfSections == 1 &&
                           (tcd.status.checkpointCreationAttributes.creationFlags == (SA_CKPT_WR_ACTIVE_REPLICA|SA_CKPT_CHECKPOINT_COLLOCATED)) &&
                           tcd.status.checkpointCreationAttributes.checkpointSize == 1069 &&
                           tcd.status.checkpointCreationAttributes.retentionDuration == 100 &&
                           tcd.status.checkpointCreationAttributes.maxSections == 2 &&
                           tcd.status.checkpointCreationAttributes.maxSectionSize == 600)
     result = TEST_PASS;
   else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

 void cpsv_it_seccreate_18()
{
  int result;
  printHead("To verify free of section create with generated sectionId");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_GEN_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
#if 0
    result = test_ckptSectionDelete(CKPT_DEL_GEN_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
 #endif 
   result = test_saCkptSectionIdFree(CKPT_FREE_GEN_T,TEST_CONFIG_MODE);
   if(result != TEST_PASS)
     goto final3;
final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}
void cpsv_it_seccreate_16() 
{
  int result;
  printHead("To verify section create with section idLen greater than maxsec id size");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_INVALID_PARAM2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}
 
void cpsv_it_seccreate_17() 
{
  int result;
  printHead("To verify section create with section idSize zero");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tcd.invalid_sec.idLen = 0;
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_INVALID_PARAM3_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  tcd.invalid_sec.idLen = 8;
  printResult(result);
  test_validate(result, TEST_PASS);
}
 
void cpsv_it_seccreate_19() 
{
  int result;
  printHead("To verify section create with long section id");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  tcd.all_replicas.maxSectionIdSize = 50;
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_LONG_SECION_ID_SUCCESS_T, TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  tcd.invalid_sec.idLen = 8;
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_seccreate_20()
{
  int result;
  printHead("To verify section create with section id length longer than MAX_SEC_ID_LEN(50)");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  tcd.all_replicas.maxSectionIdSize = 100;
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_TOO_LONG_SECION_ID_SUCCESS_T, TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/******* saCkptSectionDelete ******/


void cpsv_it_secdel_01() 
{
  int result;
  printHead("To verify section delete with arbitrary handle");
  result = test_ckptSectionDelete(CKPT_DEL_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_secdel_02() 
{
  int result;
  printHead("To verify section delete with correct handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_NOT_EXIST3_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_secdel_03() 
{
  int result;
  printHead("To verify section delete when ckpt is not opened in write mode");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_secdel_04() 
{
  int result;
  printHead("To verify section delete with an invalid section id");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_secdel_08() 
{
  int result;
  printHead("To verify that when a section is deleted it is deleted atleast in active replica");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_SUCCESS3_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_NOT_EXIST2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_secdel_09() 
{
  int result;
  printHead("To verify section delete when maxSections is 1");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_SUCCESS2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******* saCkptSectionExpirationTimeSet *******/


void cpsv_it_expset_01() 
{
  int result;
  printHead("To verify section expset with arbitrary handle");
  result = test_ckptExpirationSet(CKPT_EXP_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_expset_02() 
{
  int result;
  printHead("To verify section expset with correct handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptExpirationSet(CKPT_EXP_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  sleep(2);

  result = test_ckptSectionDelete(CKPT_DEL_NOT_EXIST4_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_expset_03() 
{
  int result;
  printHead("To verify that section exp time of a default section cannot be changed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptExpirationSet(CKPT_EXP_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_expset_04() /*To test that except for SA_TIME_END the remaining sections gets deleted after expiration time */
{
  printHead("To verify that if section exp time is SA_TIME_END it is not deleted - TO DO");

}

void cpsv_it_expset_05() /*To test that section gets deleted automatically after exp time */
{
   cpsv_it_expset_02();
}

void cpsv_it_expset_06() 
{
  int result;
  printHead("To verify section exp set when ckpt is opened in read mode");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptExpirationSet(CKPT_EXP_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_expset_07() 
{
  int result;
  printHead("To verify section exp set with invalid sectionid");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptExpirationSet(CKPT_EXP_ERR_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/********* saCkptSectionIterationInitialize *****/


void cpsv_it_iterinit_01() 
{
  int result;
  printHead("To verify that iter init returns handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_ANY_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_NO_SECTIONS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_02() 
{
  int result;
  printHead("To verify iter init with bad handle");
  result = test_ckptIterationInit(CKPT_ITER_INIT_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_03() 
{
  int result;
  printHead("To verify iter init with finalized handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
                                                                                                                             
  result = test_ckptIterationInit(CKPT_ITER_INIT_FINALIZE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_04() 
{
  int result;
  printHead("To verify sections with SA_TIME_END are returned when sectionsChosen is FOREVER");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_FOREVER2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_NO_SECTIONS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_05() 
{
  int result;
  printHead("To verify iter init when sectionsChosen is LEQ_EXP_TIME");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS10_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_LEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_NO_SECTIONS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_06() 
{
  int result;
  printHead("To verify iter init when sectionsChosen is GEQ_EXP_TIME");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS10_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_GEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_NO_SECTIONS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_07() /* To verify when sectionsChosen is CORRUPTED */
{
    printHead("No fixed procedure known for generating corrupted sections");
}

void cpsv_it_iterinit_08() 
{
    cpsv_it_iterinit_01();
}

void cpsv_it_iterinit_09() /* api selects sections as specified in sectionsChosen*/
{
    cpsv_it_iterinit_05();
}

void cpsv_it_iterinit_10()  
{
  int result;
  printHead("To verify iter init with NULL section iter handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_NULL_HANDLE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterinit_12() 
{
  int result;
  printHead("To verify iter init with invalid sectionsChosen value");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/********* saCkptSectionIterationNext *********/

void cpsv_it_iternext_01() /* to test that iter next goes through sections chosen by iter init */
{
    cpsv_it_iterinit_06();
}

void cpsv_it_iternext_02()   
{
  int result;
  printHead("To verify iter next with uninitialized handle");
  result = test_ckptIterationNext(CKPT_ITER_NEXT_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iternext_03() /* To test api with correct handle*/
{
    cpsv_it_iterinit_04();
}

void cpsv_it_iternext_04() /*To test that section descriptor contains details about the section */
{
    cpsv_it_iterinit_05();
}

void cpsv_it_iternext_05() /*To test that the api returns ERR_NO_SEC when there are no more sections that match the criteria*/
{
    cpsv_it_iterinit_05();
}

void cpsv_it_iternext_06() 
{
  int result;
  printHead("To verify that sec created before iter init and not deleted before iterfinalize are returned");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_ANY_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionDelete(CKPT_DEL_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_NO_SECTIONS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iternext_08() 
{
  int result;
  printHead("To verify iter next with NULL sec descriptor");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_GEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iternext_09() 
{
  int result;
  printHead("To verify iter next after Finalize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_GEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);

  result = test_ckptIterationNext(CKPT_ITER_NEXT_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final1;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/****** saCkptSectionIterationFinalize *******/


void cpsv_it_iterfin_01() 
{
  int result;
  printHead("To verify iter finalize with correct handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_GEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationFin(CKPT_ITER_FIN_HANDLE2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationNext(CKPT_ITER_NEXT_FIN_HANDLE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterfin_02() 
{
  int result;
  printHead("To verify iter finalize with arbitrary handle");
  result = test_ckptIterationFin(CKPT_ITER_FIN_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_iterfin_04() 
{
  int result;
  printHead("To verify iter finalize when ckpt has been closed and unlinked");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptIterationInit(CKPT_ITER_INIT_GEQ_EXP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptUnlink(CKPT_UNLINK_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS9_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptClose(CKPT_CLOSE_SUCCESS11_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptIterationFin(CKPT_ITER_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final2;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******* saCkptCheckpointWrite *******/


void cpsv_it_write_01() 
{
  int result;
  printHead("To verify that write api writes into the ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_02() 
{
    cpsv_it_write_01();
}

void cpsv_it_write_03() 
{
  int result;
  printHead("To verify that write api writes into the ACTIVE_REPLICA ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS8_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS7_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0)
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_04() 
{
  int result;
  printHead("To verify that write api writes into the ACTIVE_REPLICA_WEAK ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_WEAK_REP_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.default_write.dataBuffer,tcd.default_read.dataBuffer,tcd.default_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_05() 
{
  int result;
  printHead("To verify that write api writes into the COLLOCATED ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS3_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS5_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_06() 
{
  int result;
  printHead("To verify write api with an arbitrary handle");
  result = test_ckptWrite(CKPT_WRITE_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_07() 
{
  int result;
  printHead("To verify write api after finalize is called");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final1;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_08() 
{
  int result;
  printHead("To verify write when there is no such section in the ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_09() 
{
  int result;
  printHead("To verify write when the ckpt is not opened for writing");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_10() 
{
  int result;
  printHead("To verify write when NULL iovector is passed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_NULL_VECTOR_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_11() 
{
  int result;
  printHead("To verify write when the data to be written exceeds the maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_12() 
{
  int result;
  printHead("To verify write with NULL err index");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_NULL_INDEX_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_14() 
{
  int result;
  printHead("To verify write when the dataOffset+dataSize exceeds maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  fill_write_iovec(&tcd.err_no_res,tcd.section1,"Data in general east or west india is the best",strlen("Data in general east or west india is the best"),80,0);
  result = test_ckptWrite(CKPT_WRITE_ERR_NO_RESOURCES_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_write_16() 
{
  int result;
  printHead("To verify write when there is more than one section to be written and read from ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_MULTI_IO_REPLICA_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_MULTI_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_MULTI2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_MULTI3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_MULTI_IO_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_MULTI_IO_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if(strncmp(tcd.multi_vector[0].dataBuffer,tcd.multi_read_vector[0].dataBuffer,tcd.multi_read_vector[0].readSize) != 0 &&
     strncmp(tcd.multi_vector[1].dataBuffer,tcd.multi_read_vector[1].dataBuffer,tcd.multi_read_vector[1].readSize) != 0 &&
     strncmp(tcd.multi_vector[2].dataBuffer,tcd.multi_read_vector[2].dataBuffer,tcd.multi_read_vector[2].readSize) != 0)
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_MULTI_VECTOR_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/******* saCkptCheckpointRead *********/

void cpsv_it_read_01() /* to test that the api read from the checkpoint */
{
    cpsv_it_write_01();
}

void cpsv_it_read_02() 
{
  int result;
  printHead("To verify that when data is read the data is copied");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
  {
     result = TEST_FAIL;
     goto final3;
  }
  else 
     result = TEST_PASS;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_03() 
{
  int result;
  printHead("To verify read with arbitrary handle");
  result = test_ckptRead(CKPT_READ_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_04() 
{
  int result;
  printHead("To verify read with already finalized handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  cpsv_clean_clbk_params();
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptRead(CKPT_READ_FIN_HANDLE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_05() 
{
  int result;
  printHead("To verify read with sectionid that doesnot exist");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_read_06() 
{
  int result;
  printHead("To verify read when the ckpt is not opened for reading");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_read_07() 
{
  int result;
  printHead("To verify read when the dataSize > maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_08() 
{
  int result;
  printHead("To verify read when the dataOffset > maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_INVALID_PARAM2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_10() 
{
  int result;
  printHead("To verify read with NULL err index");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_NULL_INDEX_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_11() /*To test that readSize is returned */
{
   cpsv_it_read_02();
}

void cpsv_it_read_13() /*To test when there are multiple sections to read */
{
   cpsv_it_write_16();
}

void cpsv_it_read_14()
{
  int result;
  printHead("To verify read api when there is no data to be read");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_WEAK_REP_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_read_15()
{
  int result;
  printHead("To verify read api with NULL data buffer");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
  result = test_ckptRead(CKPT_READ_BUFFER_NULL_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/********* saCkptCheckpointSynchronize **************/

void cpsv_it_sync_01() 
{
  int result;
  printHead("To verify sync api");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_SUCCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_02() 
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify sync async api");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.sync_clbk_invo == 2116 && tcd.sync_clbk_err == SA_AIS_OK)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_03() 
{
  int result;
  printHead("To verify sync api when the ckpt is opened in read mode");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_04() 
{
  int result;
  printHead("To verify sync api when the ckpt is synchronous");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_BAD_OPERATION_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_05() 
{
  int result;
  printHead("To verify sync async api when the ckpt is synchronous");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_BAD_OPERATION_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_06() 
{
  int result;
  printHead("To verify sync async api when the ckpt is opened in read mode");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_ERR_ACCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_sync_07() 
{
  int result;
  printHead("To verify sync async api when process is not registered to have sync callback");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS7_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_ERR_INIT_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_sync_08() 
{
  int result;
  printHead("To verify sync api when there is no active replica");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_09() 
{
  int result;
  printHead("To verify sync api when there are no sections in the ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_SUCCESS2_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_10() 
{
  int result;
  printHead("To verify sync api when service is not initialized");
  result = test_ckptSynchronize(CKPT_SYNC_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_11() 
{
  int result;
  printHead("To verify sync async api when service is not initialized");
  result = test_ckptSynchronizeAsync(CKPT_ASYNC_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_12() 
{
  int result;
  printHead("To verify sync api when service is finalized");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  cpsv_clean_clbk_params();
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final1;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_13() 
{
  int result;
  printHead("To verify sync async api when service is finalized");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  cpsv_clean_clbk_params();
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final1;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_sync_14() 
{
  int result;
  printHead("To verify sync api with NULL handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronize(CKPT_SYNC_NULL_HANDLE_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/********** saCkptSectionOverwrite **************/


void cpsv_it_overwrite_01() 
{
  int result;
  printHead("To verify that overwrite writes into a section other than default section");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
  {
     result = TEST_FAIL;
     goto final3;
  }

  result = test_ckptOverwrite(CKPT_OVERWRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.overdata,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_02() 
{
  int result;
  printHead("To verify that overwrite writes into a default section");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_WEAK_REP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.default_write.dataBuffer,tcd.default_read.dataBuffer,tcd.default_read.readSize) != 0 )
  {
     result = TEST_FAIL;
     goto final3;
  }

  result = test_ckptOverwrite(CKPT_OVERWRITE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_WEAK_REP_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.overdata,tcd.default_read.dataBuffer,tcd.default_read.readSize) != 0 )
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_overwrite_03() 
{
  int result;  
  printHead("To verify overwrite with dataSize > maxSectionSize");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_INVALID_PARAM_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_04() /*When ALL REPLICA  */
{
    cpsv_it_overwrite_01();
}

void cpsv_it_overwrite_05() 
{
  int result;
  printHead("To verify overwrite in ACTIVE_REPLICA ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_SUCCESS6_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS7_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.overdata,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_06() /*When REPLICA WEAK */
{
    cpsv_it_overwrite_02();
}

void cpsv_it_overwrite_07() 
{
  int result;
  printHead("To verify overwrite with an arbitrary handle");
  result = test_ckptOverwrite(CKPT_OVERWRITE_BAD_HANDLE_T,TEST_NONCONFIG_MODE);
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_08() 
{
  int result;
  printHead("To verify overwrite with already finalized handle");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
  result = test_ckptFinalize(CKPT_FIN_SUCCESS_T,TEST_CONFIG_MODE);
  cpsv_clean_clbk_params();

  result = test_ckptOverwrite(CKPT_OVERWRITE_FIN_HANDLE_T,TEST_NONCONFIG_MODE);
  goto final1;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_09() 
{
  int result;
  printHead("To verify overwrite when section doesnot exist");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_overwrite_10() 
{
  int result;
  printHead("To verify overwrite when ckpt is not opened for writing");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ACTIVE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS4_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_ERR_ACCESS_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ACTIVE_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_11() 
{
  int result;
  printHead("To verify overwrite when NULL dataBuffer is provided");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_NULL_BUF_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_overwrite_12() 
{
  int result;
  printHead("To verify overwrite when NULL sectionId is provided");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOverwrite(CKPT_OVERWRITE_NULL_SEC_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******** OpenCallback *******/


void cpsv_it_openclbk_01()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify opencallback when opening an nonexisting ckpt");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_BAD_NAME_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);
                                                                                                                             
  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1006 && tcd.open_clbk_err == SA_AIS_ERR_NOT_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

final2:
   test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_openclbk_02()
{
  int result;
  fd_set read_fd;
  struct timeval tv;
  printHead("To verify openclbk when ckpt already exists but trying to open with different creation attributes");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  fill_ckpt_attri(&tcd.invalid_collocated,SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpenAsync(CKPT_OPEN_ASYNC_ERR_EXIST_T,TEST_NONCONFIG_MODE);

  tv.tv_sec = 60;
  tv.tv_usec = 0;
  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  select(tcd.selobj + 1, &read_fd, NULL, NULL, &tv);

  result = test_ckptDispatch(CKPT_DISPATCH_ONE_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.open_clbk_invo == 1019 && tcd.open_clbk_err == SA_AIS_ERR_EXIST)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);

final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}



/********* SynchronizeCallback ********/


void cpsv_it_syncclbk_01() 
{
  int result;
  printHead("To verify sync clbk when there is no active replica");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                             
  fill_ckpt_attri(&tcd.collocated,SA_CKPT_CHECKPOINT_COLLOCATED|SA_CKPT_WR_ACTIVE_REPLICA,1069,100,2,600,3);
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSynchronizeAsync(CKPT_ASYNC_NOT_EXIST_T,TEST_NONCONFIG_MODE);

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/******* Single node test cases *******/

void cpsv_it_onenode_01()
{
  int result;
  printHead("To verify that process can write into ckpt incrementally");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_ALL_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

  fill_write_iovec(&tcd.general_write,tcd.section1,"Data written incrementally",strlen("Data written incrementally"),46,0);
  result = test_ckptWrite(CKPT_WRITE_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  fill_read_iovec(&tcd.general_read,tcd.section1,(char *)tcd.buffer,strlen("Data written incrementally"),46,0);
  result = test_ckptRead(CKPT_READ_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  if( strncmp(tcd.general_write.dataBuffer,tcd.general_read.dataBuffer,tcd.general_read.readSize) != 0 )
     result = TEST_FAIL;
  else 
     result = TEST_PASS;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  fill_write_iovec(&tcd.general_write,tcd.section1,"Data in general east or west india is the best",strlen("Data in general east or west india is the best"),0,0);
  fill_read_iovec(&tcd.general_read,tcd.section1,(char *)tcd.buffer,strlen("Data in general east or west india is the best"),0,0);
  test_validate(result, TEST_PASS);
}

void cpsv_it_onenode_02()
{
    cpsv_it_onenode_01();
}

void cpsv_it_onenode_03()
{
   cpsv_it_expset_02();
}

void cpsv_it_onenode_04()
{
    cpsv_it_onenode_01();
}

void cpsv_it_onenode_05()
{
   cpsv_it_write_16();
}

void cpsv_it_onenode_07()
{
   cpsv_it_open_01();
}

void cpsv_it_onenode_08()
{
   cpsv_it_open_02();
}

void cpsv_it_onenode_10()
{
  //TODO:
  printHead("No fixed procedure known");
}

void cpsv_it_onenode_11()
{
  //TODO:
  printHead("No fixed procedure known for generating corrupted sections");
}

void cpsv_it_onenode_12()
{
  //TODO:
  printHead("No fixed procedure known for generating corrupted sections");
}

void cpsv_it_onenode_13()
{
  cpsv_it_repset_01();
}

void cpsv_it_onenode_17()
{
  cpsv_it_repset_03();
}

void cpsv_it_onenode_18()
{
  int result;
  printHead("To verify status get after a section has been deleted");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
                                                                                                                            
  fill_ckpt_attri(&tcd.all_replicas,SA_CKPT_WR_ALL_REPLICAS,169,100000,2,85,3);
  result = test_ckptOpen(CKPT_OPEN_ALL_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                            
  result = test_ckptOpen(CKPT_OPEN_ALL_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                            
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS3_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                            
  result = test_ckptStatusGet(CKPT_STATUS_SUCCESS6_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.status.numberOfSections == 1 &&
                           tcd.status.checkpointCreationAttributes.creationFlags == SA_CKPT_WR_ALL_REPLICAS &&
                           tcd.status.checkpointCreationAttributes.checkpointSize == 169 &&
                           tcd.status.checkpointCreationAttributes.retentionDuration == 100000 &&
                           tcd.status.checkpointCreationAttributes.maxSections == 2 &&
                           tcd.status.checkpointCreationAttributes.maxSectionSize == 85)
     result = TEST_PASS;
  else
  {
     result = TEST_FAIL;
     goto final3;
  }
                                                                                                                            
  result = test_ckptSectionDelete(CKPT_DEL_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                            
  result = test_ckptStatusGet(CKPT_STATUS_SUCCESS6_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.status.numberOfSections == 0 &&
                           tcd.status.memoryUsed == 0 &&
                           tcd.status.checkpointCreationAttributes.creationFlags == SA_CKPT_WR_ALL_REPLICAS &&
                           tcd.status.checkpointCreationAttributes.checkpointSize == 169 &&
                           tcd.status.checkpointCreationAttributes.retentionDuration == 100000 &&
                           tcd.status.checkpointCreationAttributes.maxSections == 2 &&
                           tcd.status.checkpointCreationAttributes.maxSectionSize == 85)
     result = TEST_PASS;
  else
     result = TEST_FAIL;
                                                                                                                            
final3:
  test_ckpt_cleanup(CPSV_CLEAN_ALL_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


void cpsv_it_onenode_19()
{ 
  int result;
  printHead("To verify write and read with generated sectionId");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptInitialize(CKPT_INIT_SYNC_NULL_CBK_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
    
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
    
  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptOpen(CKPT_OPEN_COLLOCATE_WRITE_SUCCESS2_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptReplicaSet(CKPT_SET_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  tcd.section6 = tcd.section4;
  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_SUCCESS5_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_GEN_T,TEST_NONCONFIG_MODE); /*contains sec generated using CKPT_SECTION_CREATE_SUCCESS5_T */
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptRead(CKPT_READ_SUCCESS2_T,TEST_NONCONFIG_MODE);/*contains sec gen using CREATE_SUCCESS5T */
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptSectionCreate(CKPT_SECTION_CREATE_GEN_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptWrite(CKPT_WRITE_SUCCESS_GEN2_T,TEST_NONCONFIG_MODE); /*contains sec generated using CKPT_SECTION_CREATE_GEN_T */
  if(result != TEST_PASS)
     goto final3;

  /*tcd.generate_read = tcd.generate_write1; */
  result = test_ckptRead(CKPT_READ_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  result = test_ckptStatusGet(CKPT_STATUS_SUCCESS4_T,TEST_NONCONFIG_MODE);
  if(result == TEST_PASS && tcd.status.numberOfSections == 2 &&
                           (tcd.status.checkpointCreationAttributes.creationFlags == (SA_CKPT_WR_ACTIVE_REPLICA|SA_CKPT_CHECKPOINT_COLLOCATED)) &&
                           tcd.status.checkpointCreationAttributes.checkpointSize == 1069 &&
                           tcd.status.checkpointCreationAttributes.retentionDuration == 100 &&
                           tcd.status.checkpointCreationAttributes.maxSections == 2 &&
                           tcd.status.checkpointCreationAttributes.maxSectionSize == 600)
     result = TEST_PASS;
   else
     result = TEST_FAIL;

final3:
  test_ckpt_cleanup(CPSV_CLEAN_COLLOCATED_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SYNC_NULL_CBK_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

/******* CKPT Arrival Callback *******/

void cpsv_it_arrclbk_01()
{
  int result;
  fd_set read_fd;
  tcd.arr_clbk_flag = 0;
  tcd.arr_clbk_err = 0;
  printHead("To verify the functionality of arrival callback");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;
                                                                                                                         
  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;
                                                                                                                         
  ncsCkptRegisterCkptArrivalCallback(tcd.ckptHandle,AppCkptArrivalCallback);
                                                                                                                         
  result = test_ckptOpen(CKPT_OPEN_WEAK_WRITE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                         
  result = test_ckptOpen(CKPT_OPEN_WEAK_READ_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;
                                                                                                                         
  result = test_ckptWrite(CKPT_WRITE_SUCCESS2_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final3;

  FD_ZERO(&read_fd);
  FD_SET(tcd.selobj, &read_fd);
  result = select(tcd.selobj + 1, &read_fd, NULL, NULL, NULL);
  if(result == 1)
  {
     result = test_ckptDispatch(CKPT_DISPATCH_ALL_T,TEST_NONCONFIG_MODE);
     if(result != TEST_PASS)
        goto final3;
  }
  else
  {
     result = TEST_FAIL;
     goto final3;
  }

  if(!tcd.arr_clbk_flag)
  {
     m_TEST_CPSV_PRINTF("\nCallback didnot arrive at the application\n");
     result = TEST_FAIL;
     goto final3;
  }

  if(tcd.arr_clbk_err)
  {
     m_TEST_CPSV_PRINTF("\nCallback values not proper\n");
     result = TEST_FAIL;
  }

final3:
  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}

void cpsv_it_arrclbk_02()
{
  int result;
  SaAisErrorT rc;
  printHead("To verify the arrival callback when NULL clbk is passed");
  result = test_ckptInitialize(CKPT_INIT_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final1;

  result = test_ckptSelectionObject(CKPT_SEL_SUCCESS_T,TEST_NONCONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  result = test_ckptOpen(CKPT_OPEN_WEAK_CREATE_SUCCESS_T,TEST_CONFIG_MODE);
  if(result != TEST_PASS)
     goto final2;

  rc = ncsCkptRegisterCkptArrivalCallback(tcd.ckptHandle,NULL);
  m_TEST_CPSV_PRINTF(" Return Value    : %s\n\n",saf_error_string[rc]);

  if(rc == SA_AIS_ERR_INVALID_PARAM)
     result = TEST_PASS;
  else
     result = TEST_FAIL;

  test_ckpt_cleanup(CPSV_CLEAN_WEAK_REPLICAS_CKPT);
final2:
  test_cpsv_cleanup(CPSV_CLEAN_INIT_SUCCESS_T);
final1:
  printResult(result);
  test_validate(result, TEST_PASS);
}


/********** END OF TEST CASES ************/


__attribute__ ((constructor)) static void ckpt_cpa_test_constructor(void) {
  test_suite_add(1, "CKPT API saCkptInitialize()");
  test_case_add(1, cpsv_it_init_01, "To verify that saCkptInitialize initializes checkpoint service");
  test_case_add(1, cpsv_it_init_02, "To verify initialization when NULL pointer is fed as clbks");
  test_case_add(1, cpsv_it_init_03, "To verify initialization with NULL version pointer");
  test_case_add(1, cpsv_it_init_04, "To verify initialization with NULL as paramaters for clbks and version");
  test_case_add(1, cpsv_it_init_05, "To verify initialization with error version relCode < supported release code");
  test_case_add(1, cpsv_it_init_06, "To verify initialization with error version relCode > supported");
  test_case_add(1, cpsv_it_init_07, "To verify initialization with error version majorVersion > supported");
  test_case_add(1, cpsv_it_init_08, "To verify init whether correct version is returned when wrong version is passed");
  test_case_add(1, cpsv_it_init_09, "To verify saCkptInitialize with NULL handle");
  test_case_add(1, cpsv_it_init_10, "To verify saCkptInitialize with one NULL clbk");

  test_suite_add(2, "CKPT API saCkptSelectObjectGet()");
  test_case_add(2, cpsv_it_sel_01, "To verify saCkptSelectionObjectGet returns operating system handle");
  test_case_add(2, cpsv_it_sel_02, "To verify SelObj api with uninitialized handle");
  test_case_add(2, cpsv_it_sel_03, "To verify SelObj api with finalized handle");
  test_case_add(2, cpsv_it_sel_04, "To verify SelObj api with NULL handle");
  test_case_add(2, cpsv_it_sel_05, "To verify SelObj api with NULL selObj");
  test_case_add(2, cpsv_it_sel_06, "To verify SelObj api with both NULL handle and selObj");

  test_suite_add(3, "CKPT API saCkptDispatch()");
  test_case_add(3, cpsv_it_disp_01, "To verify that callback is invoked when dispatch is called with DISPATCH_ONE");
  test_case_add(3, cpsv_it_disp_02, "To verify that callback is invoked when dispatch is called with DISPATCH_ALL");
  test_case_add(3, cpsv_it_disp_03, "To verify that dispatch blocks for callback when dispatchFlag is DISPATCH_BLOCKING");
  test_case_add(3, cpsv_it_disp_04, "To verify dispatch with invalid dispatchFlag");
  test_case_add(3, cpsv_it_disp_05, "To verify dispatch with NULL ckptHandle and DISPATCH_ONE");
  test_case_add(3, cpsv_it_disp_06, "To verify dispatch with NULL ckptHandle and DISPATCH_ALL");
  test_case_add(3, cpsv_it_disp_07, "To verify dispatch with NULL ckptHandle and DISPATCH_BLOCKING");
  test_case_add(3, cpsv_it_disp_08, "To verify dispatch with invalid ckptHandle and DISPATCH_ONE");
  test_case_add(3, cpsv_it_disp_09, "To verify dispatch after finalizing ckpt service");

  test_suite_add(4, "CKPT API saCkptFinalize()");
  test_case_add(4, cpsv_it_fin_01, "To verify that finalize closes association between service and process");
  test_case_add(4, cpsv_it_fin_02, "To verify finalize when service is not initialized");
  test_case_add(4, cpsv_it_fin_03, "To verify that after finalize selobj gets invalid");
  test_case_add(4, cpsv_it_fin_04, "To verify that after finalize ckpts are closed");

  test_suite_add(5, "CKPT API saCkptCheckpointOpen()");
  test_case_add(5, cpsv_it_open_01, "To verify opening a ckpt with synchronous update option");
  test_case_add(5, cpsv_it_open_02, "To verify opening a ckpt with asynchronous update option");
  test_case_add(5, cpsv_it_open_03, "To verify opening an existing ckpt");
  test_case_add(5, cpsv_it_open_04, "To verify opening an existing ckpt when its creation attributes match");
  test_case_add(5, cpsv_it_open_05, "To verify opening an nonexisting ckpt");
  test_case_add(5, cpsv_it_open_06, "To verify opening an ckpt when SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR is NOT NULL");
  test_case_add(5, cpsv_it_open_07, "To verify opening an ckpt when SA_CKPT_CHECKPOINT_CREATE flag set and CR_ATTR is NULL");
  test_case_add(5, cpsv_it_open_08, "To verify opening an ckpt with ckptSize > maxSec * maxSectionSize");
  test_case_add(5, cpsv_it_open_10, "To verify opening an ckpt when ALL_REPLICAS|ACTIVE_REPLICA specified");
  test_case_add(5, cpsv_it_open_11, "To verify opening an ckpt when ALL_REPLICAS|ACTIVE_REPLICA_WEAK specified");
  test_case_add(5, cpsv_it_open_12, "To verify opening an ckpt when ACTIVE_REPLICA|ACTIVE_REPLICA_WEAK specified");
  test_case_add(5, cpsv_it_open_13, "To verify opening an ckpt when NULL name is specified");
  test_case_add(5, cpsv_it_open_14, "To verify opening an ckpt with SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR NOT NULL");
  test_case_add(5, cpsv_it_open_15, "To verify opening an ckpt when NULL is passed as checkpointHandle");
  test_case_add(5, cpsv_it_open_16, "To verify opening an ckpt when ALL_REPLICAS|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_17, "To verify opening an ckpt when REPLICA_WEAK|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_18, "To verify opening an ckpt when ACTIVE_REPLICA|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_19, "To verify opening an ckpt when invalid openFlags is specified");
  test_case_add(5, cpsv_it_open_20, "To verify creating a ckpt when already exists but with different creation attributes");
  test_case_add(5, cpsv_it_open_21, "To verify creating a ckpt when already exists but with different ckpt size");
  test_case_add(5, cpsv_it_open_22, "To verify creating a ckpt when already exists but with different retention duration");
  test_case_add(5, cpsv_it_open_23, "To verify creating a ckpt when already exists but with different maxSections");
  test_case_add(5, cpsv_it_open_24, "To verify creating a ckpt when already exists but with different maxSectionSize");
  test_case_add(5, cpsv_it_open_25, "To verify creating a ckpt when already exists but with different maxSectionIdSize");
  test_case_add(5, cpsv_it_open_26, "To verify creating a ckpt when ckpt service has not been initialized");
  test_case_add(5, cpsv_it_open_27, "To verify creating a ckpt when NULL open clbk is provided during initialization");
  test_case_add(5, cpsv_it_open_28, "To verify openAsync with synchronous update option");
  test_case_add(5, cpsv_it_open_29, "To verify openAsync with asynchronous update option");
  test_case_add(5, cpsv_it_open_30, "To verify openAsync to open an existing ckpt");
  test_case_add(5, cpsv_it_open_31, "To verify openAsync to open an existing ckpt if its creation attributes match");
  test_case_add(5, cpsv_it_open_32, "To verify openAsync to open an nonexisting ckpt");
  test_case_add(5, cpsv_it_open_33, "To verify openAsync with SA_CKPT_CHECKPOINT_CREATE flag not set and CR_ATTR NOT NULL");
  test_case_add(5, cpsv_it_open_34, "To verify openAsync with SA_CKPT_CHECKPOINT_CREATE flag set and NULL CR_ATTR");
  test_case_add(5, cpsv_it_open_35, "To verify openAsync with ckptSize > maxSec * maxSectionSize");
  test_case_add(5, cpsv_it_open_36, "To verify openAsync when ALL_REPLICAS|ACTIVE_REPLICA specified");
  test_case_add(5, cpsv_it_open_37, "To verify openAsync when ALL_REPLICAS|ACTIVE_REPLICA_WEAK specified");
  test_case_add(5, cpsv_it_open_38, "To verify openAsync when ACTIVE_REPLICA|ACTIVE_REPLICA_WEAK specified");
  test_case_add(5, cpsv_it_open_39, "To verify openAsync when NULL name is specified");
  test_case_add(5, cpsv_it_open_40, "To verify openAsync for reading/writing which is not created");
  test_case_add(5, cpsv_it_open_41, "To verify openAsync when NULL invocation is specified");
  test_case_add(5, cpsv_it_open_42, "To verify openAsync when ALL_REPLICAS|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_43, "To verify openAsync when REPLICA_WEAK|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_44, "To verify openAsync when ACTIVE_REPLICA|COLLOCATED is specified");
  test_case_add(5, cpsv_it_open_45, "To verify openAsync when invalid openFlags is specified");
  test_case_add(5, cpsv_it_open_46, "To verify openAsync when already exists but with different creation attributes");
  test_case_add(5, cpsv_it_open_47, "To verify openAsync when already exists but with different checkpointSize");
  test_case_add(5, cpsv_it_open_48, "To verify openAsync when already exists but with different retentionDuration");
  test_case_add(5, cpsv_it_open_49, "To verify openAsync when already exists but with different maxSections");
  test_case_add(5, cpsv_it_open_50, "To verify openAsync when already exists but with different maxSectionSize");
  test_case_add(5, cpsv_it_open_51, "To verify openAsync when already exists but with different maxSectionIdSize");
  test_case_add(5, cpsv_it_open_52, "To verify creating a ckpt with invalid creation flags");
  test_case_add(5, cpsv_it_open_53, "To verify creating a ckpt with invalid creation flags");
  test_case_add(5, cpsv_it_open_54, "To verify creating a ckpt with invalid creation flags");
  test_case_add(5, cpsv_it_open_55, "To verify creating a ckpt with valid extended name length");
  test_case_add(5, cpsv_it_open_56, "To verify creating a ckpt with invalid extended name length");
  test_case_add(5, cpsv_it_open_57, "To verify openAsync a ckpt with valid extended name length");
  test_case_add(5, cpsv_it_open_58, "To verify openAsync a ckpt with invalid extended name length");

  test_suite_add(6, "CKPT API saCkptCheckpointClose()");
  test_case_add(6, cpsv_it_close_01, "To verify Closing of the checkpoint designated by checkpointHandle");
  test_case_add(6, cpsv_it_close_02, "To verify that after close checkpointHandle is no longer valid");
  test_case_add(6, cpsv_it_close_03, "To test Closing of the checkpoint after unlink");
  test_case_add(6, cpsv_it_close_04, "To test Closing of the checkpoint before unlink");
  test_case_add(6, cpsv_it_close_05, "To test close api when calling process is not registered with checkpoint service");
  test_case_add(6, cpsv_it_close_06, "To verify that close cancels all pending callbacks");
  test_case_add(6, cpsv_it_close_07, "To test that the checkpoint can be opened after close and before checkpoint expiry");
  test_case_add(6, cpsv_it_close_08, "To verify that close cancels pending callbacks for that handle only");

  test_suite_add(7, "CKPT API saCkptCheckpointUnlink()");
  test_case_add(7, cpsv_it_unlink_01, "To test Unlink deletes the existing checkpoint from the cluster");
  test_case_add(7, cpsv_it_unlink_02, "To test that name is no longer valid after unlink");
  test_case_add(7, cpsv_it_unlink_03, "To test that the ckpt with same name can be created after unlinking and yet not closed");
  test_case_add(7, cpsv_it_unlink_04, "To test that ckpt gets deleted immediately when no process has opened it");
  test_case_add(7, cpsv_it_unlink_05, "To test that the ckpt can be accessed after unlinking till it is closed");
  test_case_add(7, cpsv_it_unlink_06, "To test unlink after retention duration of ckpt");
  test_case_add(7, cpsv_it_unlink_07, "To test Closing of the checkpoint before unlink");
  test_case_add(7, cpsv_it_unlink_08, "To test unlink with correct handle");
  test_case_add(7, cpsv_it_unlink_09, "To test unlink with uninitialized handle");
  test_case_add(7, cpsv_it_unlink_10, "To test unlink in the child process - NOT SUPPORTED");
  test_case_add(7, cpsv_it_unlink_11, "To test unlink with NULL ckpt name");
  test_case_add(7, cpsv_it_unlink_12, "To test unlink a ckpt with invalid extended name");

  test_suite_add(8, "CKPT API saCkptRetenionDurationSet()");
  test_case_add(8, cpsv_it_rdset_01, "To test that invoking rdset changes the rd for the checkpoint");
  test_case_add(8, cpsv_it_rdset_02, "To test that invoking rdset changes the rd for the checkpoint");
  test_case_add(8, cpsv_it_rdset_03, "To test rdset api with invalid handle");
  test_case_add(8, cpsv_it_rdset_04, "To test when rdset is invoked after unlink");
  test_case_add(8, cpsv_it_rdset_05, "To test when ret.duration to be set is zero");

  test_suite_add(9, "CKPT API saCkptActiveReplicaSet()");
  test_case_add(9, cpsv_it_repset_01, "To verify that local replica becomes active after invoking replicaset");
  test_case_add(9, cpsv_it_repset_02, "To test that replica can be set for only for collocated ckpt");
  test_case_add(9, cpsv_it_repset_03, "To test replica can be set active only for asynchronous ckpt");
  test_case_add(9, cpsv_it_repset_04, "To test replica set when handle with read option is passed to the api");
  test_case_add(9, cpsv_it_repset_05, "To verify replicaset when an invalid handle is passed");

  test_suite_add(10, "CKPT API saCkptCheckpointStatusGet()");
  test_case_add(10, cpsv_it_status_01, "To test that this api can be used to retrieve the checkpoint status");
  test_case_add(10, cpsv_it_status_02, "To test when arbitrary handle is passed to status get api");
  test_case_add(10, cpsv_it_status_03, "To test status get when there is no active replica");
  test_case_add(10, cpsv_it_status_06, "To test status get when invalid param is passed");

  test_suite_add(11, "CKPT API saCkptSectionCreate()");
  test_case_add(11, cpsv_it_seccreate_01, "To verify section create with arbitrary handle");
  test_case_add(11, cpsv_it_seccreate_02, "To verify section create with correct handle");
  test_case_add(11, cpsv_it_seccreate_03, "To verify creating sections greater than maxSections");
  test_case_add(11, cpsv_it_seccreate_07, "To verify section create when there is no active replica");
  test_case_add(11, cpsv_it_seccreate_09, "To verify section create when ckpt is not opened in write mode");
  test_case_add(11, cpsv_it_seccreate_10, "To verify section create when maxSections is 1");
  test_case_add(11, cpsv_it_seccreate_11, "To verify section create with NULL section creation attributes");
  test_case_add(11, cpsv_it_seccreate_12, "To verify section create with NULL initial data and datasize > 0");
  test_case_add(11, cpsv_it_seccreate_13, "To verify section create with NULL initial data and datasize = 0");
  test_case_add(11, cpsv_it_seccreate_14, "To verify section create with sectionId which is already present");
  test_case_add(11, cpsv_it_seccreate_15, "To verify section create with generated sectionId");
  test_case_add(11, cpsv_it_seccreate_16, "To verify section create with section idLen greater than maxsec id size");
  test_case_add(11, cpsv_it_seccreate_17, "To verify section create with section idSize zero");
  test_case_add(11, cpsv_it_seccreate_18, "To verify free of section create with generated sectionId");
  test_case_add(11, cpsv_it_seccreate_19, "To verify section create with long section id");
  test_case_add(11, cpsv_it_seccreate_20, "To verify section create with section id length longer than MAX_SEC_ID_LEN(50)");

  test_suite_add(12, "CKPT API saCkptSectionDelete()");
  test_case_add(12, cpsv_it_secdel_01, "To verify section delete with arbitrary handle");
  test_case_add(12, cpsv_it_secdel_02, "To verify section delete with correct handle");
  test_case_add(12, cpsv_it_secdel_03, "To verify section delete when ckpt is not opened in write mode");
  test_case_add(12, cpsv_it_secdel_04, "To verify section delete with an invalid section id");
  test_case_add(12, cpsv_it_secdel_08, "To verify that when a section is deleted it is deleted atleast in active replica");
  test_case_add(12, cpsv_it_secdel_09, "To verify section delete when maxSections is 1");

  test_suite_add(13, "CKPT API saCkptSectionExpirationTimeSet()");
  test_case_add(13, cpsv_it_expset_01, "To verify section expset with arbitrary handle");
  test_case_add(13, cpsv_it_expset_02, "To verify section expset with correct handle");
  test_case_add(13, cpsv_it_expset_03, "To verify that section exp time of a default section cannot be changed");
  test_case_add(13, cpsv_it_expset_06, "To verify section exp set when ckpt is opened in read mode");
  test_case_add(13, cpsv_it_expset_07, "To verify section exp set with invalid sectionid");

  test_suite_add(14, "CKPT API saCkptSectionIterationInitialize()");
  test_case_add(14, cpsv_it_iterinit_01, "To verify that iter init returns handle");
  test_case_add(14, cpsv_it_iterinit_02, "To verify iter init with bad handle");
  test_case_add(14, cpsv_it_iterinit_03, "To verify iter init with finalized handle");
  test_case_add(14, cpsv_it_iterinit_04, "To verify sections with SA_TIME_END are returned when sectionsChosen is FOREVER");
  test_case_add(14, cpsv_it_iterinit_05, "To verify iter init when sectionsChosen is LEQ_EXP_TIME");
  test_case_add(14, cpsv_it_iterinit_06, "To verify iter init when sectionsChosen is GEQ_EXP_TIME");
  test_case_add(14, cpsv_it_iterinit_10, "To verify iter init with NULL section iter handle");
  test_case_add(14, cpsv_it_iterinit_12, "To verify iter init with invalid sectionsChosen value");

  test_suite_add(15, "CKPT API saCkptSectionIterationNext()");
  test_case_add(15, cpsv_it_iternext_02, "To verify iter next with uninitialized handle");
  test_case_add(15, cpsv_it_iternext_03, "To verify that sec created before iter init and not deleted before iterfinalize are returned");
  test_case_add(15, cpsv_it_iternext_08, "To verify iter next with NULL sec descriptor");
  test_case_add(15, cpsv_it_iternext_09, "To verify iter next after Finalize");

  test_suite_add(16, "CKPT API saCkptSectionIterationFinalize()");
  test_case_add(16, cpsv_it_iterfin_01, "To verify iter finalize with correct handle");
  test_case_add(16, cpsv_it_iterfin_02, "To verify iter finalize with arbitrary handle");
  test_case_add(16, cpsv_it_iterfin_04, "To verify iter finalize when ckpt has been closed and unlinked");

  test_suite_add(17, "CKPT API saCkptCheckpointWrite()");
  test_case_add(17, cpsv_it_write_01, "To verify that write api writes into the ckpt");
  test_case_add(17, cpsv_it_write_03, "To verify that write api writes into the ACTIVE_REPLICA ckpt");
  test_case_add(17, cpsv_it_write_04, "To verify that write api writes into the ACTIVE_REPLICA_WEAK ckpt");
  test_case_add(17, cpsv_it_write_05, "To verify that write api writes into the COLLOCATED ckpt");
  test_case_add(17, cpsv_it_write_06, "To verify write api with an arbitrary handle");
  test_case_add(17, cpsv_it_write_07, "To verify write api after finalize is called");
  test_case_add(17, cpsv_it_write_08, "To verify write when there is no such section in the ckpt");
  test_case_add(17, cpsv_it_write_09, "To verify write when the ckpt is not opened for writing");
  test_case_add(17, cpsv_it_write_10, "To verify write when NULL iovector is passed");
  test_case_add(17, cpsv_it_write_11, "To verify write when the data to be written exceeds the maxSectionSize");
  test_case_add(17, cpsv_it_write_12, "To verify write with NULL err index");
  test_case_add(17, cpsv_it_write_14, "To verify write when the dataOffset+dataSize exceeds maxSectionSize");
  test_case_add(17, cpsv_it_write_16, "To verify write when there is more than one section to be written and read from ckpt");

  test_suite_add(18, "CKPT API saCkptCheckpointRead()");
  test_case_add(18, cpsv_it_read_02, "To verify that when data is read the data is copied");
  test_case_add(18, cpsv_it_read_03, "To verify read with arbitrary handle");
  test_case_add(18, cpsv_it_read_04, "To verify read with already finalized handle");
  test_case_add(18, cpsv_it_read_05, "To verify read with sectionid that doesnot exist");
  test_case_add(18, cpsv_it_read_06, "To verify read when the ckpt is not opened for reading");
  test_case_add(18, cpsv_it_read_07, "To verify read when the dataSize > maxSectionSize");
  test_case_add(18, cpsv_it_read_08, "To verify read when the dataOffset > maxSectionSize");
  test_case_add(18, cpsv_it_read_10, "To verify read with NULL err index");
  test_case_add(18, cpsv_it_read_14, "To verify read api when there is no data to be read");
  test_case_add(18, cpsv_it_read_15, "To verify read api with NULL data buffer");

  test_suite_add(19, "CKPT API saCkptCheckpointSynchronize()");
  test_case_add(19, cpsv_it_sync_01, "To verify sync api");
  test_case_add(19, cpsv_it_sync_02, "To verify sync async api");
  test_case_add(19, cpsv_it_sync_03, "To verify sync api when the ckpt is opened in read mode");
  test_case_add(19, cpsv_it_sync_04, "To verify sync api when the ckpt is synchronous");
  test_case_add(19, cpsv_it_sync_05, "To verify sync async api when the ckpt is synchronous");
  test_case_add(19, cpsv_it_sync_06, "To verify sync async api when the ckpt is opened in read mode");
  test_case_add(19, cpsv_it_sync_07, "To verify sync async api when process is not registered to have sync callback");
  test_case_add(19, cpsv_it_sync_08, "To verify sync api when there is no active replica");
  test_case_add(19, cpsv_it_sync_09, "To verify sync api when there are no sections in the ckpt");
  test_case_add(19, cpsv_it_sync_10, "To verify sync api when service is not initialized");
  test_case_add(19, cpsv_it_sync_11, "To verify sync async api when service is not initialized");
  test_case_add(19, cpsv_it_sync_12, "To verify sync api when service is finalized");
  test_case_add(19, cpsv_it_sync_13, "To verify sync async api when service is finalized");
  test_case_add(19, cpsv_it_sync_14, "To verify sync api with NULL handle");

  test_suite_add(20, "CKPT API saCkptSectionOverwrite()");
  test_case_add(20, cpsv_it_overwrite_01, "To verify that overwrite writes into a section other than default section");
  test_case_add(20, cpsv_it_overwrite_02, "To verify that overwrite writes into a default section");
  test_case_add(20, cpsv_it_overwrite_03, "To verify overwrite with dataSize > maxSectionSize");
  test_case_add(20, cpsv_it_overwrite_05, "To verify overwrite in ACTIVE_REPLICA ckpt");
  test_case_add(20, cpsv_it_overwrite_07, "To verify overwrite with an arbitrary handle");
  test_case_add(20, cpsv_it_overwrite_08, "To verify overwrite with already finalized handle");
  test_case_add(20, cpsv_it_overwrite_09, "To verify overwrite when section doesnot exist");
  test_case_add(20, cpsv_it_overwrite_10, "To verify overwrite when ckpt is not opened for writing");
  test_case_add(20, cpsv_it_overwrite_11, "To verify overwrite when NULL dataBuffer is provided");
  test_case_add(20, cpsv_it_overwrite_12, "To verify overwrite when NULL sectionId is provided");

  test_suite_add(21, "CKPT OpenCallBack");
  test_case_add(21, cpsv_it_openclbk_01, "To verify opencallback when opening an nonexisting ckpt");
  test_case_add(21, cpsv_it_openclbk_02, "To verify openclbk when ckpt already exists but trying to open with different creation attributes");

  test_suite_add(22, "CKPT SynchronizeCallback");
  test_case_add(22, cpsv_it_syncclbk_01, "To verify sync clbk when there is no active replica");

  test_suite_add(23, "CKPT Single node test cases");
  test_case_add(23, cpsv_it_onenode_01, "To verify that process can write into ckpt incrementally");
  test_case_add(23, cpsv_it_onenode_18, "To verify status get after a section has been deleted");
  test_case_add(23, cpsv_it_onenode_19, "To verify write and read with generated sectionId");

  test_suite_add(24, "CKPT Arrival Callback");
  test_case_add(24, cpsv_it_arrclbk_01, "To verify the functionality of arrival callback");
  test_case_add(24, cpsv_it_arrclbk_02, "To verify the arrival callback when NULL clbk is passed");
}
