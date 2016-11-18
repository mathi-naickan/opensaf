/* include files */
#include "mbcsv_api.h"
#include "tet_api.h"

pthread_mutex_t  mutex_cb =  PTHREAD_MUTEX_INITIALIZER;
uint32_t mbcstm_sync_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys, 
                           MBCSTM_CB_TEST sync,uint32_t warm,uint32_t wait)
{
  /*change*/
  uint32_t mbcstm_create_data_point(uint32_t , uint32_t );
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  
  uint32_t test_result;
  MBCSTM_PEERS_DATA peers;
  
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      sleep(wait+7);
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        return  NCSCC_RC_FAILURE;
      
      return NCSCC_RC_SUCCESS; 
    }
  
  if(asys == mbcstm_cb.sys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
      if(warm != 0)
        {
          mbcstm_create_data_point(svc_index, ssn_index);
          mbcstm_create_data_point(svc_index, ssn_index);
          mbcstm_create_data_point(svc_index, ssn_index);
          mbcstm_create_data_point(svc_index, ssn_index);
          mbcstm_create_data_point(svc_index, ssn_index);
        }
    }
  
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    return  NCSCC_RC_FAILURE;
  
  sleep(1);
  
  /* if(mbcstm_cb.sys == MBCSTM_SVC_INS1) */
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = sync; 
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    return  NCSCC_RC_FAILURE;
  /*
    if(warm)
    if(asys != mbcstm_cb.sys)
    {
    sleep(5);
    mbcstm_cb.svces[svc_index].ssns[ssn_index].data_req = 2;
    mbcstm_cb.svces[svc_index].ssns[ssn_index].data_req_count = 2;
    
    } */
  
  
  /* Block on the mutex */
  sleep(wait + 4);
  if(pthread_mutex_lock( &mutex_cb) == 0)
    tet_printf( "\n lock taken by case ");
  
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    return  NCSCC_RC_FAILURE;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    test_result = mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result;
  
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = false;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_svc_finalize (svc_index);
  sleep(2);
  return(test_result);
}


uint32_t mbcstm_data_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                           uint32_t wait, MBCSTM_CB_TEST sync)
{
  /*change*/
  uint32_t mbcstm_create_data_point(uint32_t , uint32_t );
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  
  uint32_t test_result = NCSCC_RC_FAILURE;
  MBCSTM_PEERS_DATA peers;
  int PASS=0;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS3 || mbcstm_cb.sys == MBCSTM_SVC_INS4)
    {
      sleep(wait+5);
      if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
        goto final;
      
      goto final; 
    }
  
  if(asys == mbcstm_cb.sys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
      
      mbcstm_create_data_point(svc_index, ssn_index);
      mbcstm_create_data_point(svc_index, ssn_index);
      mbcstm_create_data_point(svc_index, ssn_index);
      mbcstm_create_data_point(svc_index, ssn_index);
      mbcstm_create_data_point(svc_index, ssn_index);
    }
  
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;
  
  sleep(1);
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)    
    mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = sync;
  
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto final;
  
  if(asys != mbcstm_cb.sys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].data_req = 2;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].data_req_count = 2;
      sleep(wait + 3); 
      /*        while(mbcstm_cb.svces[svc_index].ssns[ssn_index].warm_flag
                != MBCSTM_SYNC_COMPLETE);
      */
      if(mbcstm_svc_data_request(svc_index, ssn_index) != NCSCC_RC_SUCCESS)
        goto final;
      else
        {
          printf( "\n Data request sent successfully from system %d", 
                  mbcstm_cb.sys);
          PASS=1;
        }
    }
  else
    sleep(wait + 3);
  
  sleep(5);
  
  if(pthread_mutex_lock( &mutex_cb) == 0)
    tet_printf( "\n lock taken by case ");
  
  /* if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
     if(mbcstm_cb.svces[svc_index].ssns[ssn_index].warm_flag
     == MBCSTM_DATA_REQUEST)
  */
  test_result = NCSCC_RC_SUCCESS;
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
  
 final:
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
  
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = false;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_svc_finalize (svc_index);
  sleep(2);
  return(test_result);
}

uint32_t mbcstm_notify_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                             uint32_t isys, MBCSTM_CB_TEST sync, 
                             NCS_MBCSV_NTFY_MSG_DEST msg_dest,char *str, 
                             uint32_t len)
{    
  uint32_t test_result=NCSCC_RC_FAILURE;
  MBCSTM_PEERS_DATA peers;
  int PASS=0;
  /*change*/
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t   mbcstm_svc_send_notify(uint32_t svc_index, uint32_t ssn_index,
                                 NCS_MBCSV_NTFY_MSG_DEST msg_dest,char *str, 
                                 uint32_t len);
    
  if(mbcstm_cb.sys == asys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }
    
  if(isys >= 0 && mbcstm_cb.sys == isys)
    mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = sync;
    
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    {
      test_result = NCSCC_RC_FAILURE;
      goto final;
    }
    
  sleep(1);
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    {
      test_result = NCSCC_RC_FAILURE;
      goto final;
    }
    
  sleep(3);
  if( mbcstm_svc_send_notify(svc_index,ssn_index,msg_dest,str,len) != 
      NCSCC_RC_SUCCESS)
    {
      tet_printf( "\n Send notify failed");
      test_result = NCSCC_RC_FAILURE;
      goto final;
    }
  else
    PASS=1;  
    
  sleep(2);
  test_result = NCSCC_RC_SUCCESS;
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
 final:    
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = false;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_svc_finalize (svc_index);
  sleep(2);
  if(PASS)
    test_result=NCSCC_RC_SUCCESS;
  return (test_result);
}


uint32_t mbcstm_cb_test_cases(NCS_MBCSV_CB_ARG *arg)
{
  MBCSTM_SSN *ssn = (MBCSTM_SSN *)((long)arg->i_client_hdl);
  /*change*/
  /*    uint32_t  svc_index = ssn->svc_index;*/
  
  switch(ssn->cb_test)
    {
    case  MBCSTM_CB_PEER_INFO_FAIL:
      if(arg->i_op == NCS_MBCSV_CBOP_PEER)
        {
          printf("\n In NCS_MBCSV_CBOP_PEER");
          return NCSCC_RC_SUCCESS;
        }
      break;
    case  MBCSTM_CB_PEER_INFO_PASS:
      if(arg->i_op == NCS_MBCSV_CBOP_PEER)
        {
          ssn->cb_test_result = NCSCC_RC_SUCCESS; 
          printf("\n In NCS_MBCSV_CBOP_PEER");
        }
      break;
    case  MBCSTM_CB_PEER_DISC_PERFORM :
      if(arg->i_op == NCS_MBCSV_CBOP_PEER)
        {
          struct timeval t;
          gettimeofday(&t,NULL);
          ssn->perf_final_time = (double) t.tv_sec + (double) t.tv_usec * 1e-6;
          ssn->cb_test_result = NCSCC_RC_SUCCESS;
        }
      break;
    case MBCSTM_CB_STANDBY_COLD_ENCODE_FAIL:
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
            {
              tet_printf( "\n got NCS_MBCSV_MSG_COLD_SYNC_REQ");
              printf( "\nNot Calling My Encode Callback for COLD_SYNC_REQ");
              if(ssn->cb_flag == 0)
                ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  if(pthread_mutex_unlock( &mutex_cb) == 0);
                  tet_printf( "\n mutex given");
                }
              return NCSCC_RC_SUCCESS;
            }
        }    
      return NCSCC_RC_FAILURE;    
    case MBCSTM_CB_STANDBY_COLD_DECODE_FAIL :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP
             || arg->info.encode.io_msg_type == 
             NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE)
            {
              printf( "\nNot Calling My Decode Callback for COLD_SYNC_RESP or\
 COLD_SYNC_RESP_COMPLETE");
              tet_printf( "\n cb_flag values is %d", ssn->cb_flag);
              if(ssn->cb_flag == 0) ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  pthread_mutex_unlock( &mutex_cb);
                }
              
              return NCSCC_RC_SUCCESS;
            }
        }
      break;
    case MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL :
      if(ssn->csi_role == SA_AMF_HA_ACTIVE)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
            {
              printf( "\nNot Calling My Decode Callback for COLD_SYNC_REQ");
              if(ssn->cb_flag == 0) ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  tet_printf( "\n cb test results is success");
                  pthread_mutex_unlock( &mutex_cb);
                }
              return NCSCC_RC_SUCCESS;
            }
        }
      break;
    case MBCSTM_CB_ACTIVE_COLD_ENCODE_FAIL :
      if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP
         || arg->info.encode.io_msg_type == 
         NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE)
        {
          printf( "\nNot Calling My Encode Callback for COLD_SYNC_RESP or \
COLD_SYNC_RESP_COMPLETE");
          if(ssn->cb_flag == 0) ssn->cb_flag = 1;
          else
            {
              ssn->cb_test_result = NCSCC_RC_SUCCESS;
              pthread_mutex_unlock( &mutex_cb);
            }
          return NCSCC_RC_SUCCESS;
        }
      break;
    case MBCSTM_CB_STANDBY_WARM_ENCODE_FAIL :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_REQ)
            {
              printf( "\nNot Calling My Encode Callback for WARM_SYNC_REQ");
              if(ssn->cb_flag == 0) ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  if(pthread_mutex_unlock( &mutex_cb) == 0);
                  tet_printf( "\n mutex given");
                }
              return NCSCC_RC_SUCCESS;
            }
        }
      break;
    case MBCSTM_CB_STANDBY_WARM_DECODE_FAIL :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_RESP
             || arg->info.encode.io_msg_type == 
             NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE)
            {
              printf( "\nNot Calling My Decode Callback for WARM_SYNC_RESP or\
 WARM_SYNC_RESP_COMPLETE");
              tet_printf( "\n cb_flag values is %d", ssn->cb_flag);
              if(ssn->cb_flag == 0) ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  pthread_mutex_unlock( &mutex_cb);
                }
              
              return NCSCC_RC_SUCCESS;
            }
        }
      break;
    case MBCSTM_CB_ACTIVE_WARM_DECODE_FAIL :
      if(ssn->csi_role == SA_AMF_HA_ACTIVE)
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_REQ)
            {
              printf( "\nNot Calling My Decode Callback for WARM_SYNC_REQ");
              if(ssn->cb_flag == 0) ssn->cb_flag = 1;
              else
                {
                  ssn->cb_test_result = NCSCC_RC_SUCCESS;
                  pthread_mutex_unlock( &mutex_cb);
                }
              return NCSCC_RC_SUCCESS;
            }
        }
      break;
    case MBCSTM_CB_ACTIVE_WARM_ENCODE_FAIL :
      if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_RESP
         || arg->info.encode.io_msg_type == 
         NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE)
        {
          printf( "\nNot Calling My Encode Callback for WARM_SYNC_RESP or\
 WARM_SYNC_RESP_COMPLETE");
          if(ssn->cb_flag == 0) ssn->cb_flag = 1;
          else
            {
              ssn->cb_test_result = NCSCC_RC_SUCCESS;
              pthread_mutex_unlock( &mutex_cb);
            }
          return NCSCC_RC_SUCCESS;
        }
      break;
      
    case  MBCSTM_CB_STANDBY_WARM_CMPL_FAIL:
      if(arg->info.decode.i_msg_type == NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE)
        {
          ssn->cb_test_result = NCSCC_RC_SUCCESS;
          return NCSCC_RC_SUCCESS;
        }
      break;
    case MBCSTM_CB_ACTIVE_WARM_ON_OFF :
      break;
      
    case MBCSTM_CB_ACTIVE_COLD_TIMER_EXP :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          struct timeval tv;
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
            {
              if(ssn->cb_flag == 0)
                {
                  gettimeofday(&tv, NULL);
                  ssn->cb_flag = tv.tv_sec;
                }
            }
          
          if(arg->info.error.i_code == NCS_MBCSV_COLD_SYNC_TMR_EXP)
            {
              gettimeofday(&tv, NULL);
              if((tv.tv_sec - ssn->cb_flag) == 
                 MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD)
                ssn->cb_test_result = NCSCC_RC_SUCCESS;
              
              tet_printf( "\n time gap %d", tv.tv_sec - ssn->cb_flag);
              ssn->cb_flag = 0;
              if(pthread_mutex_unlock( &mutex_cb) == 0);
              tet_printf( "\n mutex given");
            }
        }
      else
        {
          if(arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
            return NCSCC_RC_SUCCESS;
        }
      
      break;    
    case MBCSTM_CB_ACTIVE_COLD_CMP_TIMER_EXP :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          struct timeval tv;
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
            {       if(ssn->cb_flag == 0)
              {
                gettimeofday(&tv, NULL);
                ssn->cb_flag = tv.tv_sec;
              }
            }
          if(arg->info.error.i_code == NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP)
            {
              gettimeofday(&tv, NULL);
              if((tv.tv_sec - ssn->cb_flag) == 
                 MBCSTM_TIMER_COLD_SYNC_CMPLT_PERIOD)
                ssn->cb_test_result = NCSCC_RC_SUCCESS;
              tet_printf( "\n time gap %d", tv.tv_sec - ssn->cb_flag);
              ssn->cb_flag = 0;
            }
        }
      else
        {
          if(arg->info.encode.io_msg_type ==  NCS_MBCSV_MSG_COLD_SYNC_RESP)
            return NCSCC_RC_SUCCESS;
        }
      break;
      
    case MBCSTM_CB_ACTIVE_WARM_TIMER_EXP :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          struct timeval tv;
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_REQ)
            {
              if(ssn->cb_flag == 0)
                {
                  gettimeofday(&tv, NULL);
                  ssn->cb_flag = tv.tv_sec;
                }
            }
          
          if(arg->info.error.i_code == NCS_MBCSV_WARM_SYNC_TMR_EXP )
            {
              gettimeofday(&tv, NULL);    
              if((tv.tv_sec - ssn->cb_flag) ==  
                 MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD)
                ssn->cb_test_result = NCSCC_RC_SUCCESS;    
              tet_printf( "\n time gap %d", tv.tv_sec - ssn->cb_flag);
              ssn->cb_flag = 0;
              if(pthread_mutex_unlock( &mutex_cb) == 0);
              tet_printf( "\n mutex given");
              
            }
        }
      else
        {
          if(arg->info.decode.i_msg_type == NCS_MBCSV_MSG_WARM_SYNC_REQ)
            return NCSCC_RC_SUCCESS;
        }
      break;
    case MBCSTM_CB_ACTIVE_WARM_CMP_TIMER_EXP :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        {
          struct timeval tv;
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_REQ)
            {
              if(ssn->cb_flag == 0)
                {
                  gettimeofday(&tv, NULL);
                  ssn->cb_flag = tv.tv_sec;
                }
            }
          
          if(arg->info.error.i_code == NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP)
            {
              gettimeofday(&tv, NULL);
              if((tv.tv_sec - ssn->cb_flag) == 
                 MBCSTM_TIMER_WARM_SYNC_CMPLT_PERIOD)
                ssn->cb_test_result = NCSCC_RC_SUCCESS;
              tet_printf( "\n time gap %d", tv.tv_sec - ssn->cb_flag);
              ssn->cb_flag = 0;
            }
        }
      else
        {
          if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_WARM_SYNC_RESP)
            return NCSCC_RC_SUCCESS;
        }
      
      break;
    case MBCSTM_CB_ACTIVE_DRSP_TIMER_EXP :
      break;
      
    case  MBCSTM_CB_STANDBY_COLD_ENCODE_CHECK :
      if(ssn->csi_role == SA_AMF_HA_STANDBY)
        if(arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_REQ)
          {
            ssn->cb_test_result = NCSCC_RC_SUCCESS;
            return NCSCC_RC_SUCCESS;
          }
      break;
    case MBCSTM_CB_VERSION_CHECK :
      if(arg->i_op == NCS_MBCSV_CBOP_DEC)
        {
        }
      break;
      /*change*/
    case MBCSTM_CB_NO_TEST:
      break;
      
    }
  return NCSCC_RC_FAILURE;    
}

void mbcstm_coldsync_standby_encode_fail() 
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_standby_encode_fail";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2, 
                            MBCSTM_CB_STANDBY_COLD_ENCODE_FAIL,0,11) 
      !=  NCSCC_RC_SUCCESS)
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

void mbcstm_coldsync_coldrequest_noresponse() 
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_coldrequest_noresponse";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS1, MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL,
                            0, 11) !=  NCSCC_RC_SUCCESS)
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


void mbcstm_coldsync_stdby_decode_fail() 
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_stdby_decode_fail";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2, 
                            MBCSTM_CB_STANDBY_COLD_DECODE_FAIL,0,11)
      !=  NCSCC_RC_SUCCESS)
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

void mbcstm_coldsync_active_encode_fail() 
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_active_encode_fail";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS1, MBCSTM_CB_ACTIVE_COLD_ENCODE_FAIL,
                            0,11) !=  NCSCC_RC_SUCCESS)
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


void mbcstm_coldsync_timer() 
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_timer";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2,MBCSTM_CB_ACTIVE_COLD_TIMER_EXP,0,
                            2*MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD)
      !=  NCSCC_RC_SUCCESS)
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

void mbcstm_coldsync_cmp_timer()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_coldsync_cmp_timer";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2, 
                            MBCSTM_CB_ACTIVE_COLD_CMP_TIMER_EXP,0, 
                            2*MBCSTM_TIMER_COLD_SYNC_CMPLT_PERIOD )
      !=  NCSCC_RC_SUCCESS)
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

void mbcstm_warmsync_timer()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_warmsync_timer";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2, MBCSTM_CB_ACTIVE_WARM_TIMER_EXP ,
                            0,2*MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD)
      !=  NCSCC_RC_SUCCESS)
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

void mbcstm_warmsync_cmp_timer()
{
  uint32_t svc_index = 1, ssn_index = 1;
  
  char case_name[] = "mbcstm_warmsync_cmp_timer";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  if( mbcstm_sync_purposes( svc_index, ssn_index,
                            MBCSTM_SVC_INS2, 
                            MBCSTM_CB_ACTIVE_WARM_CMP_TIMER_EXP ,0,
                            2*MBCSTM_TIMER_WARM_SYNC_CMPLT_PERIOD)
      !=  NCSCC_RC_SUCCESS)
    
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

/* NEW */
/* this case just tests Complteness of the warm sync */

void mbcstm_warmsync_complete()
{
  uint32_t svc_index = 1, ssn_index = 1, asys;
  
  char case_name[] = "mbcstm_warmsync_complete";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  asys = MBCSTM_SVC_INS2;
  if( mbcstm_data_purposes( svc_index, ssn_index,asys, 
                            MBCSTM_TIMER_WARM_SYNC_CMPLT_PERIOD,
                            MBCSTM_CB_STANDBY_WARM_CMPL_FAIL)
      !=  NCSCC_RC_SUCCESS)
    
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

/*change:changed the if condition*/
void mbcstm_data_request_idle()
{
  uint32_t svc_index = 1, ssn_index = 1, asys;
  
  char case_name[] = "mbcstm_data_request_idle";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_FAIL, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);
  
  asys = MBCSTM_SVC_INS2;
  
  if( mbcstm_data_purposes( svc_index, ssn_index,asys, 
                            1, MBCSTM_CB_STANDBY_COLD_ENCODE_FAIL)
      ==  NCSCC_RC_SUCCESS)
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


/* NEW change: changed its expected result from Fail to PASS and the 
   if condition
*/
void mbcstm_send_notify_idle()
{
  
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_idle";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS1;                                    
  isys = MBCSTM_SVC_INS1;
  
  len = strlen(case_disc)+1;
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_PEER_INFO_FAIL, NCS_MBCSV_ALL_PEERS,
                              case_disc,len) ==  NCSCC_RC_SUCCESS)
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

/* NEW */
void mbcstm_notify_send_all()
{
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_send_all";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS1;                                    
  isys = -1;
  len = strlen(case_disc)+1;
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_NO_TEST, NCS_MBCSV_ALL_PEERS,case_disc,
                              len) !=  NCSCC_RC_SUCCESS)
    
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
/* NEW */

void mbcstm_notify_send_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_send_to_active";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS2;                                    
  isys = -1;
  len = strlen(case_disc)+1;
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_NO_TEST, NCS_MBCSV_ACTIVE,case_disc,
                              len) !=  NCSCC_RC_SUCCESS)
    
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

void mbcstm_notify_send_to_standby()
{
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_send_to_standby";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS2;                                    
  isys = MBCSTM_SVC_INS3;
  len = strlen(case_disc)+1;
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_NO_TEST, NCS_MBCSV_STANDBY,case_disc,
                              len) !=  NCSCC_RC_SUCCESS)
    
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

/* NEW */
void mbcstm_notify_send_active_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_active_to_active";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_FAIL, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS1;                                    
  isys = -1;
  len = strlen(case_disc)+1;
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_NO_TEST, NCS_MBCSV_ACTIVE,case_disc,
                              len) !=  NCSCC_RC_SUCCESS)
    
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

/* NEW */
void mbcstm_notify_send_active_to_multiactive()
{
  uint32_t svc_index = 1, ssn_index = 1, asys, isys, len; 
  char case_name[] = "mbcstm_notify_active_to_multiactive";
  char case_disc[] = "This is notify test_message";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_FAIL, final_res =  TET_MBCSV_FAIL;
  
  case_num++;
  tet_printf( "\n\n              CASE %d: %s\n", case_num, case_name);       
  asys = MBCSTM_SVC_INS1;                                    
  isys = -1;
  len = strlen(case_disc)+1;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role=V_DEST_RL_STANDBY; 
    }
  
  if( mbcstm_notify_purposes( svc_index, ssn_index,asys,isys, 
                              MBCSTM_CB_NO_TEST, NCS_MBCSV_ACTIVE,case_disc,
                              len) !=  NCSCC_RC_SUCCESS)
    
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
