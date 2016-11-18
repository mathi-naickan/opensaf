/* include files */
#include "mbcsv_api.h"
#include "tet_api.h"

#define         M16     0xA001


static uint32_t updcrc(uint32_t crc,uint32_t c,uint32_t mask)
{
  int i;
  c<<=8;
  for(i=0;i<8;i++)
    {
      if((crc ^ c) & 0x8000) crc=(crc<<1)^mask;
      else crc<<=1;
      c<<=1;
    }
  return crc;
}
                                                                                                                       
uint32_t mbcstm_crc(char *str, uint32_t len)
{
  char ch;
  uint32_t crc = 0;
  int i;
  for(i = 0;i<len;i++)
    {
      ch = str[i];
      crc = updcrc(crc,ch,M16);
    }

  return crc;                                                                                                    
}
                                                                                                                       
uint32_t mbcstm_perf_sync_msg(uint32_t svc_index, uint32_t ssn_index,uint32_t size,uint32_t send_type)
{
  SSN_PERF_DATA *data;

  data = ( SSN_PERF_DATA *) malloc(sizeof(SSN_PERF_DATA));
  data->msg = (char *) malloc(size);
  memset(data->msg,'c',size);
  data->msg[size-1] = '\0';    
  data->length = size;
  data->crc = mbcstm_crc(data->msg,size);

  /*    printf("\n msg: %s", data->msg);
        printf("\n crc of msg %d", data->crc); */

  if(mbcstm_svc_cp_send(svc_index,ssn_index,NCS_MBCSV_ACT_ADD,PERFORMANCE_DATA, (long) data, send_type) 
     != NCSCC_RC_SUCCESS)
    return NCSCC_RC_FAILURE;

  return NCSCC_RC_SUCCESS;
}

uint32_t mbcstm_verify_sync_msg(SSN_PERF_DATA *data, MBCSTM_SSN *ssn)
{
  uint32_t crc;
  crc = mbcstm_crc(data->msg,data->length);

  /*    printf("\n msg len: %d", data->length);
        printf("\n msg: %s", data->msg); 
        printf("crc calculated %d and crc of message %d",crc, data->crc); */

  if(crc != data->crc)
    {
      ssn->cb_test_result = NCSCC_RC_FAILURE;
    }
  else
    {
      ssn->cb_flag++;
      /* printf("\n crc checking is successfull for msg %d", ssn->cb_flag);    */
    }
  return NCSCC_RC_SUCCESS;
}

uint32_t mbcstm_msg_check_purposes(uint32_t svc_index, uint32_t ssn_index,NCS_MBCSV_MSG_TYPE send_type)
{
  uint32_t size = 2;
  uint32_t msg_count = 20,count = 20;
  uint32_t test_result = NCSCC_RC_FAILURE;
  /* open session */
  if (mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto  final;
  /* wait for two seconds */
  sleep(3);
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_SUCCESS;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_flag = 1;
    
  /*change: */
  sleep(2);
  /* start sending syncronous updates of various checks */
  if (mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      while(count-- != 0)
        {
          /* sleep(1); */
          if(mbcstm_perf_sync_msg(svc_index,ssn_index,size,send_type) != NCSCC_RC_SUCCESS)
            goto final;
          size += 500;
        }
      test_result = NCSCC_RC_SUCCESS;
    }
  else
    {
      sleep(2);
      if ((mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result != NCSCC_RC_SUCCESS) ||
          (mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag != msg_count))
        goto final;
      printf("\n number of messages received %d", mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag);
      test_result = NCSCC_RC_SUCCESS;
    }
 final:
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag =  0;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_flag = 0;
  mbcstm_svc_finalize (svc_index);
  sleep(1);
  return (test_result);
}


uint32_t mbcstm_disc_perf_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys, uint32_t nrepeat,MBCSTM_CB_TEST sync)
{
  uint32_t test_result = NCSCC_RC_FAILURE;
  struct timeval open_time;

  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      sleep(3*nrepeat);
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        goto final;
      test_result = NCSCC_RC_SUCCESS;
      goto final;
    }

  /* open session */
  if (mbcstm_cb.sys == asys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;

  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      int i;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = sync;
      for(i = 0; i < nrepeat; i++)
        {
          gettimeofday(&open_time,NULL);
          if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
            goto  final;

          sleep(2);
          printf("\n cb test result %d", mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result);
          if(mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result == NCSCC_RC_SUCCESS)
            {
              double t1;
              double t0;;
              t1 = mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_final_time;
              t0 = (double) open_time.tv_sec + (double) open_time.tv_usec * 1e-6;
              t0 = t1-t0;
              printf("\n time takan to disc: %f", t0);
            }

          if(mbcstm_ssn_close(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
            goto  final;
          sleep(1);    
        }
    }
  else
    {
      if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
        goto  final;
      sleep(3*nrepeat);
    }

  test_result = NCSCC_RC_SUCCESS;
    
 final:
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
                                                                                                                       
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag =  0;
  mbcstm_svc_finalize (svc_index);
                                                                                                                       
  return (test_result);
}


uint32_t mbcstm_csync_perf_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys)
{
  uint32_t test_result = NCSCC_RC_FAILURE;

  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        goto final;
      test_result = NCSCC_RC_SUCCESS;
      goto final;
    }

  /* open session */
  mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_flag = 1;
  if (mbcstm_cb.sys == asys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;

  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto  final;

  if (mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      double t;
      while(mbcstm_cb.svces[svc_index].ssns[ssn_index].cold_flag != MBCSTM_SYNC_COMPLETE);
      t = mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_final_time - 
        mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_init_time;
      printf("\n time to complete cold sync: %f",t);             
    }

  test_result = NCSCC_RC_SUCCESS;
    
 final:
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
                                                                                                                       
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag =  0;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].perf_flag =  0;

  mbcstm_svc_finalize (svc_index);                                                       
  return (test_result);
}


void mbcstm_test_usrasync_send_messages()
{
  uint32_t svc_index = 1, ssn_index = 1;
                                                                                                                       
  char case_name[] = "mbcstm_test_usrasync_send_messages";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
                                                                                                                       
  if( mbcstm_msg_check_purposes( svc_index, ssn_index, NCS_MBCSV_SND_USR_ASYNC) !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
                                                                                                                       
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}


void mbcstm_test_sync_send_messages()
{
  uint32_t svc_index = 1, ssn_index = 1;
                                                                                                                       
  char case_name[] = "mbcstm_test_sync_send_messages";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
                                                                                                                       
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
                                                                                                                       
  if( mbcstm_msg_check_purposes( svc_index, ssn_index, NCS_MBCSV_SND_SYNC) !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
                                                                                                                       
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}


void mbcstm_test_mbcasync_send_messages()
{
  uint32_t svc_index = 1, ssn_index = 1;
                                                                                                                       
  char case_name[] = "mbcstm_test_mbcasync_send_messages";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
                                                                                                                       
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
  if( mbcstm_msg_check_purposes( svc_index, ssn_index, NCS_MBCSV_SND_MBC_ASYNC) !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
                                                                                                                       
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}

void mbcstm_peer_discover_performance()
{                                                                                                                
  uint32_t svc_index = 1, ssn_index = 1;
                                                                                                                       
  char case_name[] = "mbcstm_peer_discover_performance";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
                                                                                                                       
  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);
                                                                                                                       
  if( mbcstm_disc_perf_purposes( svc_index, ssn_index,MBCSTM_SVC_INS3,10,
                                 MBCSTM_CB_NO_TEST) !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  else
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
                                                                                                                       
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
                                                                                                                       
}

