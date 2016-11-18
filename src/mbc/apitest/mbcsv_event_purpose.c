/* include files */
#include "mbcsv_api.h"
#include "tet_api.h"

uint32_t mbcstm_event_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                            uint32_t bsys, MBCSTM_TEST_ACTION action,
                            uint32_t act_on_sys,uint32_t act_val,
                            MBCSTM_CB_TEST cb_check,uint32_t wait,
                            uint32_t peer_count,uint64_t peer_anchor, 
                            MBCSTM_FSM_STATES state_check);


uint32_t mbcstm_event_purposes(uint32_t svc_index, uint32_t ssn_index, uint32_t asys,
                            uint32_t bsys, MBCSTM_TEST_ACTION action,
                            uint32_t act_on_sys,uint32_t act_val,
                            MBCSTM_CB_TEST cb_check,uint32_t wait,
                            uint32_t peer_count,uint64_t peer_anchor, 
                            MBCSTM_FSM_STATES state_check)
{
  uint32_t test_result = NCSCC_RC_FAILURE;
  MBCSTM_PEERS_DATA peers;
  MBCSTM_PEER_INST       *pr;
  /*change*/
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  
  memset(&peers, '\0', sizeof(MBCSTM_PEERS_DATA));
  
  if(asys == mbcstm_cb.sys)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }
  
  if(bsys == mbcstm_cb.sys)
    if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
      goto final;    
  
  if(mbcstm_svc_registration(svc_index) != NCSCC_RC_SUCCESS)
    goto final;
  sleep(2);
  if(mbcstm_ssn_open(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
    goto final;
  sleep(2);
  if(bsys != mbcstm_cb.sys)
    if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
      goto final;
  sleep(2);
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    goto final;

  /* on flag to check in call back here */
  if(MBCSTM_SVC_INS1 ==  mbcstm_cb.sys)
    mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = cb_check;
    
  sleep(2);
  if(act_on_sys == mbcstm_cb.sys)
    {
      switch(action)
        {
        case MBCSTM_TEST_ACTION_NO :
          break;
        case MBCSTM_TEST_ACTION_ROLE :
          if(mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role == 
             SA_AMF_HA_ACTIVE && act_val == SA_AMF_HA_STANDBY )
            {
              mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = 
                SA_AMF_HA_QUIESCED;
              if(mbcstm_ssn_set_role(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
                goto final;
            } 
          sleep(1);    
          mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = act_val;
          mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = act_val;
          if(mbcstm_ssn_set_role(svc_index,ssn_index) != NCSCC_RC_SUCCESS)
            goto final;
          break;
        case MBCSTM_TEST_ACTION_CLOSE :
          if( mbcstm_ssn_close(svc_index, ssn_index) != NCSCC_RC_SUCCESS)
            goto final;    
          break;
          /*change*/
        case MBCSTM_TEST_ACTION_GET:
          break;
        case MBCSTM_TEST_ACTION_SET:
          break;
          
        }
        
    } 
  else
    {
      sleep(4);
    }
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    goto final;

  /* check on target system */
  sleep(wait + 1);
  if (mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      uint32_t index;
      mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers);
      /* check number peers is ok or not */
      if(peers.peer_count !=  peer_count)
        goto final;
      /* check state of the peer is ok or not */
      for(index = 0; index < peer_count; index++)
        {
          pr = & peers.peers[index];
          if(peer_anchor == pr->peer_anchor)
            {
              /*              tet_printf(  "\n peer anchor is %llx", pr->peer_anchor);                              tet_printf(  "\n peer fsm state %s", pr->state);*/
              /*  CROSS CHECKING
                  if(pr->state != state_check)
                  goto final;*/

            }
        }
        
      if(mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test 
         !=  MBCSTM_CB_NO_TEST)
        {
          if(mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result 
             != NCSCC_RC_SUCCESS)
            goto final;
          else
            tet_printf(  "test result is going to be SUCCESS");
        }
    }
    
  test_result = NCSCC_RC_SUCCESS;
    

 final:
    
  if(mbcstm_cb.sys != MBCSTM_SVC_INS1)
    mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, svc_index, ssn_index, &peers); 
    
  if(mbcstm_sync_point() != NCSCC_RC_SUCCESS)
    test_result = NCSCC_RC_FAILURE;
    
  mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].ws_flag = false;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[svc_index].ssns[ssn_index].cb_flag = 0;
  mbcstm_svc_finalize (svc_index);sleep(2);
  return (test_result);
}

void mbcstm_peerup_event_newactive_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_peerup_event_newactive_to_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_ACTIVE, MBCSTM_CB_NO_TEST,1,3,
                             MBCSTM_SSN_ANC2,MBCSTM_ACT_STATE_MULTIPLE_ACTIVE )
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

void mbcstm_peerup_event_newactive_to_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;  
  char case_name[] = "mbcstm_peerup_event_newactive_to_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_NO,0,0, 
                             MBCSTM_CB_STANDBY_COLD_ENCODE_CHECK,
                             MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_STBY_STATE_WAIT_TO_COLD_SYNC)
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
  /* change to original state */

}

void mbcstm_peerup_event_newstandby_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_peerup_event_newstandby_to_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_NO,0,0, 
                             MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL,
                             MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC)
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


void mbcstm_peerup_event_newstandby_to_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_peerup_event_newstandby_to_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS3,
                             MBCSTM_TEST_ACTION_NO,0,0, 
                             MBCSTM_CB_NO_TEST,1,
                             3,MBCSTM_SSN_ANC3,
                             MBCSTM_STBY_STATE_IDLE ) !=  NCSCC_RC_SUCCESS)
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


void mbcstm_peerdown_event_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_peerdown_event_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_CLOSE,MBCSTM_SVC_INS2,0, 
                             MBCSTM_CB_NO_TEST,1,2,0,0) !=  NCSCC_RC_SUCCESS)
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


void mbcstm_peerdown_event_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_peerdown_event_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS1,
                             MBCSTM_TEST_ACTION_CLOSE,MBCSTM_SVC_INS2,0, 
                             MBCSTM_CB_NO_TEST,1,2,0,0) !=  NCSCC_RC_SUCCESS)
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


void mbcstm_cngrole_event_newactive_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_cngrole_event_newactive_to_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_ACTIVE,MBCSTM_CB_NO_TEST,1,3,
                             MBCSTM_SSN_ANC2,MBCSTM_ACT_STATE_MULTIPLE_ACTIVE)
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


void mbcstm_cngrole_event_newactive_to_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_cngrole_event_newactive_to_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS3,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS3,
                             SA_AMF_HA_ACTIVE, 
                             MBCSTM_CB_STANDBY_COLD_ENCODE_CHECK,1,
                             3,MBCSTM_SSN_ANC3,
                             MBCSTM_STBY_STATE_WAIT_TO_COLD_SYNC)
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


void mbcstm_cngrole_event_newstandby_to_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_cngrole_event_newstandby_to_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }
  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_STANDBY, 
                             MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL,
                             MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC)
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
void mbcstm_cngrole_event_newstandby_to_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_cngrole_event_newstandby_to_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS3,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_STANDBY, 
                             MBCSTM_CB_NO_TEST,1,3,MBCSTM_SSN_ANC2,
                             MBCSTM_STBY_STATE_IDLE) !=  NCSCC_RC_SUCCESS)
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

void mbcstm_active_multiactive_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_active_multiactive_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    } 

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_STANDBY, 
                             MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL,
                             MBCSTM_TIMER_SEND_COLD_SYNC_PERIOD,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_ACT_STATE_WAIT_FOR_COLD_WARM_SYNC)
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
void mbcstm_standby_active_quiesced()
{
  uint32_t svc_index = 1, ssn_index = 1;                                        
  char case_name[] = "mbcstm_standby_active_quiesced";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS2,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS2,
                             SA_AMF_HA_QUIESCED, 
                             MBCSTM_CB_NO_TEST,1,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_STBY_STATE_IDLE) !=  NCSCC_RC_SUCCESS)
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
/* Note: Below two cases are not added in to the arry because behaviour is undefined at present */
void mbcstm_multi_multi_active()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_multi_multi_active";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }


  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS2,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS3,
                             SA_AMF_HA_ACTIVE, 
                             MBCSTM_CB_NO_TEST,1,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_STBY_STATE_IDLE) !=  NCSCC_RC_SUCCESS)
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

void mbcstm_multi_multi_active_standby()
{
  uint32_t svc_index = 1, ssn_index = 1;
  char case_name[] = "mbcstm_multi_multi_active_standby";
  char case_disc[] = "";
  TET_MBCSV_RESULT exp_res = TET_MBCSV_PASS, final_res = TET_MBCSV_FAIL;

  case_num++;
  tet_printf(  "\n\n              CASE %d: %s\n", case_num, case_name);

  if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
    {
      mbcstm_cb.svces[svc_index].ssns[ssn_index].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_cb.svces[svc_index].ssns[ssn_index].dest_role = V_DEST_RL_ACTIVE;
    }

  if( mbcstm_event_purposes( svc_index, ssn_index,
                             MBCSTM_SVC_INS1,MBCSTM_SVC_INS3,
                             MBCSTM_TEST_ACTION_ROLE,MBCSTM_SVC_INS1,
                             SA_AMF_HA_STANDBY, 
                             MBCSTM_CB_NO_TEST,1,
                             3,MBCSTM_SSN_ANC2,
                             MBCSTM_STBY_STATE_IDLE) !=  NCSCC_RC_SUCCESS)
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

/* Notes:

1. While acitve in wait for cold sync request , async updates can not be sent, case need to come.

*/
