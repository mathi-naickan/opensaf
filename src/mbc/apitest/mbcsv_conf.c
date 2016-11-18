/* include files */

#include "mbcsv_api.h"

uint32_t mbcstm_input()
{
  char     *input_ptr;
  uint32_t    sys_id;
  
  input_ptr = (char *) getenv("MBCSTM_SYS_ID");
  sys_id = atoi(input_ptr);
  memset(&mbcstm_cb,'\0', sizeof(mbcstm_cb));
  switch(sys_id)
    {
    case 0 :
      mbcstm_cb.sys = MBCSTM_SVC_INS1;
      break;
    case 1 : 
      mbcstm_cb.sys = MBCSTM_SVC_INS2;
      break;
    case 2 :
      mbcstm_cb.sys = MBCSTM_SVC_INS3;
      break;
    case 3 :
      mbcstm_cb.sys = MBCSTM_SVC_INS4;
      break;
    default :
      return NCSCC_RC_FAILURE;
    }
  
  return NCSCC_RC_SUCCESS;
}
MDS_DEST get_vdest_anchor()
{
  NCSADA_INFO    ada_info;
  MDS_DEST dest;
  memset(&ada_info,'\0', sizeof(ada_info));
  /*request*/
  ada_info.req=NCSADA_GET_HDLS;    
  /*api call*/
  if(ncsada_api(&ada_info)==NCSCC_RC_SUCCESS)
    {
      /*store return values*/
      dest=ada_info.info.adest_get_hdls.o_adest; /*output*/
      printf("\nVDEST Anchor = %llx : is SUCCESSFUL",dest);
      fflush(stdout);
      return dest;
    }
  else
    {
      printf("\nRequest to ncsada_api: GET_HDLS has FAILED");
      fflush(stdout);
      return NCSCC_RC_FAILURE;
    }
}
uint32_t mbcstm_config()
{
  uint32_t    svc_count, ssn_count, ssn_index;
  uint64_t    anc_index;
  uint32_t    vdest_id = NCSVDA_EXTERNAL_UNNAMED_MIN;
  uint32_t    svc_id =  MBCSTM_SVC_ID1;
  
  switch(mbcstm_cb.sys)
    {
    case MBCSTM_SVC_INS1 :
      anc_index=MBCSTM_SSN_ANC1=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS2 :
      anc_index=MBCSTM_SSN_ANC2=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS3 :
      anc_index=MBCSTM_SSN_ANC3=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS4 :
      anc_index=MBCSTM_SSN_ANC4=get_vdest_anchor();
      break;
    default :
      return NCSCC_RC_FAILURE;
    }
  
  for(ssn_index = 1; ssn_index < MBCSTM_SSN_MAX; ssn_index++)
    {
      mbcstm_cb.vdest_count++;
      mbcstm_cb.vdests[ssn_index].dest_id = ++vdest_id;
      mbcstm_cb.vdests[ssn_index].anchor = anc_index;
    }
  
  /* memset(&mbcstm_cb, '\0', sizeof(mbcstm_cb)); */
  for(svc_count = 1; svc_count < MBCSTM_SVC_MAX; svc_count++)
    {
      ++mbcstm_cb.svc_count;
      mbcstm_cb.flag = 1;
      mbcstm_cb.svces[svc_count].svc_id = svc_id++;
#if 0
      mbcstm_cb.svces[svc_count].version.releaseCode = 'B';
      mbcstm_cb.svces[svc_count].version.minorVersion = 1;
      mbcstm_cb.svces[svc_count].version.majorVersion = 1;
#endif
      mbcstm_cb.svces[svc_count].version = 1;
      mbcstm_cb.svces[svc_count].disp_flags =  SA_DISPATCH_BLOCKING;
      mbcstm_cb.svces[svc_count].task_flag = true;
      /* DISPATCH_ONE, DISPATCH_ALL, DISPATCH_BLOCKING */
      vdest_id = NCSVDA_EXTERNAL_UNNAMED_MIN;
      for(ssn_index = 1; ssn_index < MBCSTM_SSN_MAX; ssn_index++)
        {
        ssn_count = ++mbcstm_cb.svces[svc_count].ssn_count;
        /* printf("\n ssn_count %d", mbcstm_cb.svces[svc_count].ssn_count); */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].svc_index = svc_count;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].cb_test = MBCSTM_CB_NO_TEST;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].dest_id = mbcstm_cb.vdests[ssn_index].dest_id;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].anchor =  mbcstm_cb.vdests[ssn_index].anchor;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].dest_role = V_DEST_RL_STANDBY;
        /* V_DEST_RL_INIT,V_DEST_RL_ACTIVE,V_DEST_RL_STANDBY */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].csi_role = SA_AMF_HA_STANDBY;
        /* SA_AMF_HA_ACTIVE, SA_AMF_HA_STANDBY, SA_AMF_HA_QUIESCED, SA_AMF_HA_STOPPING */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ssn_index = ssn_count;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ws_flag = true; 
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ws_timer =  MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD*100;
        
        switch(mbcstm_cb.sys)
          {
          case MBCSTM_SVC_INS1 :
            break;
          case MBCSTM_SVC_INS2 :
            break;
          case MBCSTM_SVC_INS3 :
            break;
          case MBCSTM_SVC_INS4 :
            break;
          default :
            return NCSCC_RC_FAILURE;
          }
        
        }
      
    }
  
  mbcstm_config_print();
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mbcsv_config()
{
  uint32_t    svc_count, ssn_count, ssn_index;
  uint64_t    anc_index;
  uint32_t    vdest_id = NCSVDA_EXTERNAL_UNNAMED_MIN;
  uint32_t    svc_id =  MBCSTM_SVC_ID1;
  
  switch(mbcstm_cb.sys)
    {
    case MBCSTM_SVC_INS1 :
      anc_index=MBCSTM_SSN_ANC1=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS2 :
      anc_index=MBCSTM_SSN_ANC2=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS3 :
      anc_index=MBCSTM_SSN_ANC3=get_vdest_anchor();
      break;
    case MBCSTM_SVC_INS4 :
      anc_index=MBCSTM_SSN_ANC4=get_vdest_anchor();
      break;
    default :
      return NCSCC_RC_FAILURE;
    }
  
  for(ssn_index = 1; ssn_index <=1; ssn_index++)
    {
      mbcstm_cb.vdest_count++;
      mbcstm_cb.vdests[ssn_index].dest_id = ++vdest_id;
      mbcstm_cb.vdests[ssn_index].anchor = anc_index;
    }
  
  /* memset(&mbcstm_cb, '\0', sizeof(mbcstm_cb)); */
  for(svc_count = 1; svc_count <=1; svc_count++)
    {
      ++mbcstm_cb.svc_count;
      mbcstm_cb.flag = 1;
      mbcstm_cb.svces[svc_count].svc_id = svc_id++;
#if 0
      mbcstm_cb.svces[svc_count].version.minorVersion = 1;
      mbcstm_cb.svces[svc_count].version.majorVersion = 1;
#endif
      mbcstm_cb.svces[svc_count].version = 1;
      mbcstm_cb.svces[svc_count].disp_flags =  SA_DISPATCH_BLOCKING;
      mbcstm_cb.svces[svc_count].task_flag = true;
      /* DISPATCH_ONE, DISPATCH_ALL, DISPATCH_BLOCKING */
      vdest_id = NCSVDA_EXTERNAL_UNNAMED_MIN;
      for(ssn_index = 1; ssn_index <=1; ssn_index++)
        {
        ssn_count = ++mbcstm_cb.svces[svc_count].ssn_count;
        /* printf("\n ssn_count %d", mbcstm_cb.svces[svc_count].ssn_count); */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].svc_index = svc_count;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].cb_test = MBCSTM_CB_NO_TEST;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].dest_id = mbcstm_cb.vdests[ssn_index].dest_id;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].anchor =  mbcstm_cb.vdests[ssn_index].anchor;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].dest_role = V_DEST_RL_STANDBY;
        /* V_DEST_RL_INIT,V_DEST_RL_ACTIVE,V_DEST_RL_STANDBY */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].csi_role = SA_AMF_HA_STANDBY;
        /* SA_AMF_HA_ACTIVE, SA_AMF_HA_STANDBY, SA_AMF_HA_QUIESCED, SA_AMF_HA_STOPPING */
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ssn_index = ssn_count;
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ws_flag = true; 
        mbcstm_cb.svces[svc_count].ssns[ssn_count].ws_timer =  MBCSTM_TIMER_SEND_WARM_SYNC_PERIOD*100;
        
        switch(mbcstm_cb.sys)
          {
          case MBCSTM_SVC_INS1 :
            break;
          case MBCSTM_SVC_INS2 :
            break;
          case MBCSTM_SVC_INS3 :
            break;
          case MBCSTM_SVC_INS4 :
            break;
          default :
            return NCSCC_RC_FAILURE;
          }
        
        }
      
    }
/*  mbcstm_config_print();*/
  return NCSCC_RC_SUCCESS;
}
