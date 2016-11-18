/* include files */
#include "mbcsv_api.h"
#include "tet_api.h"

uint32_t mbcstm_create_data_point(uint32_t svc_index, uint32_t ssn_index)
{
  MBCSTM_SSN *ssn;
  MBCSTM_CSI_DATA *data;
  struct timeval tv;
  char time_str[26];

  ssn =  &mbcstm_cb.svces[svc_index].ssns[ssn_index];
  ssn->sync_count = ssn->data_count;
  data = &ssn->data[++ssn->data_count];
  data->data_id = ssn->data_count;

  if(gettimeofday(&tv, NULL) != 0)
    {
      printf("\n get time of day failed");
      return NCSCC_RC_FAILURE;
    }
  else
    {
      ctime_r((time_t *)&tv,time_str);
      time_str[24]='\0';
    }

  data->sec = tv.tv_sec;
  data->usec = tv.tv_usec;

  printf("\nAdding Data Element : Sync Count %d Data count %d -  Data id : %d \
Sec : %d Usec : %d",ssn->sync_count,
         ssn->data_count,data->data_id,data->sec,data->usec);
  return NCSCC_RC_SUCCESS;
}
uint32_t mbcstm_destroy_data_point(uint32_t svc_index, uint32_t ssn_index)
{
  MBCSTM_SSN *ssn;
  
  ssn =  &mbcstm_cb.svces[svc_index].ssns[ssn_index];    
  printf("\nDeleting Last Data Element");
  ssn->data_count--;
  
  if(ssn->sync_count != 0)
    if(ssn->sync_count ==  ssn->data_count)
      ssn->sync_count = 0;
  
  return NCSCC_RC_FAILURE;
}
uint32_t mbcstm_print_data_points(uint32_t svc_index, uint32_t ssn_index)
{
  MBCSTM_SSN *ssn;
  MBCSTM_CSI_DATA *data;
  int i=0;
  ssn =  &mbcstm_cb.svces[svc_index].ssns[ssn_index];
  printf("\n\n\t----- PRINTING MY DATA INVENTORY -----\n");
  for(i=1;i<=ssn->data_count;i++)
    {
      data=&ssn->data[i];
      printf("\n Data id : %d Sec : %d Usec : %d",data->data_id,data->sec,
             data->usec);
    }
  printf("\n\t-----        END                 -----\n");
  return NCSCC_RC_SUCCESS;
}
uint32_t mbcstm_ckpt_send_purpose(uint32_t svc_index, uint32_t ssn_index,uint32_t asys,
                               uint32_t send_index, uint32_t send_count,
                               NCS_MBCSV_ACT_TYPE action, 
                               NCS_MBCSV_MSG_TYPE send_type,
                               MBCSTM_CB_TEST sync)
{
  uint32_t index;
  uint32_t  test_result = NCSCC_RC_FAILURE;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        goto final;
      goto final;
    }
  
  if(mbcstm_cb.sys ==  MBCSTM_SVC_INS1)
    if(sync !=  MBCSTM_CB_NO_TEST)
      mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = sync;
  
  if(mbcstm_cb.sys == asys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }
  
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;
  sleep(1);
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto final;
  sleep(4);
  
  if(mbcstm_cb.sys ==  MBCSTM_SVC_INS1)
    for(index = send_index; index <= send_count; index++)
      {
        if(mbcstm_svc_cp_send(svc_index,ssn_index,action, NORMAL_DATA,index, 
                              send_type) != NCSCC_RC_SUCCESS)
          goto final;
      }
  
  test_result = NCSCC_RC_SUCCESS;
  
 final:
  
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
  
  /*    
   mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
  */
  
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].data_count = 0;
  mbcstm_svc_finalize (svc_index);
  sleep(2);
  return test_result;
}

void  mbcstm_ckpt_sendsync()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_ckpt_sendsync";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  printf("\n\n              CASE %d: %s\n", case_num, case_name);
  
  mbcstm_create_data_point(svc_index, ssn_index);
  
  if( mbcstm_ckpt_send_purpose(svc_index, ssn_index, MBCSTM_SVC_INS1, 1, 1, 
                               NCS_MBCSV_ACT_ADD,NCS_MBCSV_SND_SYNC,
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

void  mbcstm_ckpt_standby_sendsync()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_ckpt_standby_sendsync";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  printf("\n\n              CASE %d: %s\n", case_num, case_name);
  
  mbcstm_create_data_point(svc_index, ssn_index);
  
  if( mbcstm_ckpt_send_purpose(svc_index, ssn_index, MBCSTM_SVC_INS2, 1, 1, 
                               NCS_MBCSV_ACT_ADD,NCS_MBCSV_SND_SYNC,
                               MBCSTM_CB_NO_TEST) ==  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
  else
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}

/*vishnu*/
void  mbcstm_ckpt_idle_sendsync()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_ckpt_idle_sendsync";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_FAIL, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  printf("\n\n              CASE %d: %s\n", case_num, case_name);
  
  mbcstm_create_data_point(svc_index, ssn_index);
  
  if( mbcstm_ckpt_send_purpose(svc_index, ssn_index, MBCSTM_SVC_INS1, 1, 1, 
                               NCS_MBCSV_ACT_ADD,NCS_MBCSV_SND_SYNC,
                               MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL) 
      !=  NCSCC_RC_SUCCESS)
    {
      final_res = TET_MBCSV_PASS;
      cases_passed++;
    }
  else
    {
      final_res = TET_MBCSV_FAIL;
      cases_failed++;
    }
  
  mbcstm_test_print(case_num, case_name,case_disc, exp_res, final_res);
}

void mbcstm_ckpt_send_usrasync()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_ckpt_send_usrasync";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  printf("\n\n              CASE %d: %s\n", case_num, case_name);
  
  mbcstm_create_data_point(svc_index, ssn_index);
  
  if( mbcstm_ckpt_send_purpose(svc_index, ssn_index, MBCSTM_SVC_INS1, 1, 1,
                               NCS_MBCSV_ACT_ADD, NCS_MBCSV_SND_USR_ASYNC,
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

void mbcstm_ckpt_send_mbcasync()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_ckpt_send_mbcasync";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  printf("\n\n              CASE %d: %s\n", case_num, case_name);
  
  mbcstm_create_data_point(svc_index, ssn_index);
  
  if( mbcstm_ckpt_send_purpose(svc_index, ssn_index, MBCSTM_SVC_INS1, 1, 1,
                               NCS_MBCSV_ACT_ADD, NCS_MBCSV_SND_MBC_ASYNC,
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

