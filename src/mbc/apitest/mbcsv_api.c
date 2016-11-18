#include  "tet_api.h"
#include "mbcsv_api.h"

const char role[] = { 'I', 'A', 'S', 'Q'};

const char *fsm_state[4][6] =
  {
    {"IDLE","----","----","----","----","----"},
    {"IDLE","WFCS","KSIS","MACT","----","----"},
    {"IDLE","WTCS","SISY","WTWS","VWSD","WFDR"},
    {"QUIE","----","----","----","----","----"}

  };
extern pthread_mutex_t mutex_cb;

extern int fill_syncparameters(int);

uint32_t    mbcstm_system_startup()
{
  char fun_name[] = "mbcstm_system_startup";

  /* Step1: start agent to start MDS */    
  /* already don in configuratin files */    
  /* step2 :start all the vdest which are required for the services */
  if(mbcstm_dest_start() != NCSCC_RC_SUCCESS)
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"VDEST_START",
                   NCSCC_RC_FAILURE);            
      return  NCSCC_RC_FAILURE;
    }
  /* step3: start mbcstm lock */
  m_NCS_LOCK_INIT(&mbcstm_cb.mbcstm_lock);
  return NCSCC_RC_SUCCESS;
}

uint32_t     mbcstm_system_close()
{
  m_NCS_LOCK_DESTROY(&mbcstm_cb.mbcstm_lock);
  return NCSCC_RC_SUCCESS;
}

uint32_t     mbcstm_dest_start()
{
  NCSVDA_INFO vda_info;
  MDS_DEST    dest;
  uint32_t svc_index, svc_count, ssn_index, ssn_count;
  char fun_name[] = "mbcstm_system_close";

  memset(&vda_info, 0, sizeof(vda_info));
  memset(&dest , 0, sizeof(dest));
  /* Fill all common feilds */
  vda_info.req = NCSVDA_VDEST_CREATE;
  vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;
  vda_info.info.vdest_create.i_persistent = true; /* Up-to-the application */
  vda_info.info.vdest_create.i_policy = NCS_VDEST_TYPE_DEFAULT;
  ssn_count = mbcstm_cb.vdest_count;
  svc_count = mbcstm_cb.svc_count;
  for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
    {
      /*  dest.info.v1.vcard = mbcstm_cb.vdests[ssn_index].dest_id;  */
      MBCSTM_SET_VDEST_ID_IN_MDS_DEST(dest,
                                      mbcstm_cb.vdests[ssn_index].dest_id); 
      vda_info.info.vdest_create.info.specified.i_vdest = dest; 

      if (ncsvda_api(&vda_info) != NCSCC_RC_SUCCESS)
        {
          mbcstm_print(__FILE__, fun_name, __LINE__,"VDA_CREAT",
                       NCSCC_RC_FAILURE);
          return NCSCC_RC_FAILURE;
        }
      mbcstm_cb.vdests[ssn_index].status = true;
      for(svc_index = 1; svc_index <= svc_count; svc_index++)
        {
          mbcstm_cb.svces[svc_index].ssns[ssn_index].pwe_hdl  = 
            vda_info.info.vdest_create.o_mds_pwe1_hdl;
        }

      /*    printf("\n vdest %d started successfully",
            mbcstm_cb.vdests[ssn_index].dest_id); */
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t mbcstm_dest_close()
{
  NCSVDA_INFO vda_info;
  MDS_DEST        dest;
  uint32_t ssn_count,ssn_index,svc_index;
  char fun_name[] = "mbcstm_dest_close";

  memset(&vda_info, 0, sizeof(vda_info));
  memset(&dest , 0, sizeof(dest));
  vda_info.req = NCSVDA_VDEST_DESTROY;
  vda_info.info.vdest_create.i_create_type = NCSVDA_VDEST_CREATE_SPECIFIC;

  for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
    {
      MBCSTM_SET_VDEST_ID_IN_MDS_DEST(dest,
                                      mbcstm_cb.vdests[ssn_index].dest_id);
      vda_info.info.vdest_create.info.specified.i_vdest = dest;
      vda_info.info.vdest_destroy.i_make_vdest_non_persistent = false;

      if (ncsvda_api(&vda_info) != NCSCC_RC_SUCCESS)
        {
          mbcstm_print(__FILE__, fun_name, __LINE__,"VDA_DESTROY",
                       NCSCC_RC_FAILURE);
          return NCSCC_RC_FAILURE;
        }

      mbcstm_cb.vdests[ssn_index].status = false;
      for(svc_index = 1; svc_index <= mbcstm_cb.svc_count; svc_index++)
        {
          mbcstm_cb.svces[svc_index].ssns[ssn_index].pwe_hdl  = 0;
        }


    }

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_svc_registration(uint32_t svc_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC    *svc;
  char        fun_name[] = "mbcstm_svc_registration";            

  svc = &mbcstm_cb.svces[svc_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
  mbcsv_arg.info.initialize.i_service  = svc->svc_id;
  mbcsv_arg.info.initialize.i_mbcsv_cb = mbcstm_svc_cb;
  mbcsv_arg.info.initialize.i_version  = svc->version;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"REGS",NCSCC_RC_FAILURE); 
      return NCSCC_RC_FAILURE;
    }

  svc->mbcsv_hdl = mbcsv_arg.info.initialize.o_mbcsv_hdl;

  if(NCSCC_RC_SUCCESS != mbcstm_ssn_get_select(svc_index))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"GET_SELECT",NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  if(svc->task_flag == true)
    mbcstm_start_process_thread(svc_index);

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_svc_finalize (uint32_t svc_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  char            fun_name[] = "mbcstm_svc_finalize";

  svc = &mbcstm_cb.svces[svc_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  /* first zero select objest */
  svc->sel_obj = 0;

  mbcsv_arg.i_op = NCS_MBCSV_OP_FINALIZE;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"FINALIZE",NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    } 

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_ssn_open(uint32_t svc_index, uint32_t ssn_index)
{    
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN    *ssn;
  char        fun_name[] = "mbcstm_svc_open";    

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_OPEN;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.open.i_pwe_hdl =(uint32_t) ssn->pwe_hdl;
  mbcsv_arg.info.open.i_client_hdl =(long) ssn;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"OPEN",NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }    

  ssn->ckpt_hdl = mbcsv_arg.info.open.o_ckpt_hdl;

  if (NCSCC_RC_SUCCESS != mbcstm_ssn_set_role(svc_index, ssn_index))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"SSN_SET_ROLE",
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t    mbcstm_ssn_open_all(uint32_t svc_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  uint32_t        ssn_count,ssn_index;
  char            fun_name[] = "mbcstm_svc_open_all";

  svc = &mbcstm_cb.svces[svc_index];
  ssn_count = svc->ssn_count;
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
    {

      if (NCSCC_RC_SUCCESS != mbcstm_ssn_open(svc_index, ssn_index))
        {
          mbcstm_print(__FILE__, fun_name, __LINE__,"SSN_OPEN",
                       NCSCC_RC_FAILURE);
          return NCSCC_RC_FAILURE;
        }
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_ssn_set_role (uint32_t svc_index, uint32_t ssn_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;   
  char            fun_name[] = "mbcstm_ssn_set_role"; 

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.chg_role.i_ckpt_hdl = ssn->ckpt_hdl;
  mbcsv_arg.info.chg_role.i_ha_state = ssn->csi_role;

  sleep(1);

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"ROLE", NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_ssn_close(uint32_t svc_index, uint32_t ssn_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_ssn_close";

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_CLOSE;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.close.i_ckpt_hdl = ssn->ckpt_hdl;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"CLOSE", NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }
  ssn->ckpt_hdl=0; /*change*/
  return NCSCC_RC_SUCCESS;

}

uint32_t   mbcstm_ssn_get_select(uint32_t svc_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  char            fun_name[] = "mbcstm_ssn_get_selection";

  svc = &mbcstm_cb.svces[svc_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"SELECT_GET", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  svc->sel_obj = mbcsv_arg.info.sel_obj_get.o_select_obj;
  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_svc_dispatch (uint32_t  svc_index)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  char            fun_name[] = "mbcstm_svc_dispatch";

  svc = &mbcstm_cb.svces[svc_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_DISPATCH;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.dispatch.i_disp_flags = svc->disp_flags;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"DISPATCH", NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;

}

static void mbcstm_process_events(NCSCONTEXT svc_index)
{
  uint32_t index = *((uint32_t *)svc_index);
  pthread_mutex_init( &mutex_cb, NULL);

  if(pthread_mutex_lock( &mutex_cb) == 0)
    printf("\t");
  /*printf("\n mutex taken by thread");*/

  if(mbcstm_cb.svces[index].disp_flags !=  SA_DISPATCH_BLOCKING)
    {
      while(mbcstm_cb.flag)
        mbcstm_svc_dispatch (index);
    }
  else
    mbcstm_svc_dispatch (index);

  /*printf("\n mbcstm_process_events comming out");*/
  free(svc_index);

}

uint32_t mbcstm_start_process_thread(uint32_t svc_index)
{
  NCSCONTEXT task_hdl;
  uint32_t *index = (uint32_t *)malloc(sizeof(uint32_t));
  *index = svc_index;

  if(m_NCS_TASK_CREATE((NCS_OS_CB)mbcstm_process_events,
                       index,"MBCSV_DISPATCH",
                       NCS_TASK_PRIORITY_7,
                       NCS_STACKSIZE_MEDIUM,
                       &task_hdl) !=  NCSCC_RC_SUCCESS)
    {
      printf("Task Creation failed \n");
      return NCSCC_RC_FAILURE;
    }

  if(m_NCS_TASK_START(task_hdl) != NCSCC_RC_SUCCESS)
    {
      m_NCS_TASK_RELEASE(task_hdl);
      printf("Task Creation failed \n");
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t  mbcstm_svc_obj(uint32_t svc_index, uint32_t ssn_index, uint32_t action, 
                      NCS_MBCSV_OBJ obj)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_svc_obj";

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];
  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = action;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;

  if(action == NCS_MBCSV_OP_OBJ_SET)
    {
      mbcsv_arg.info.obj_set.i_ckpt_hdl = ssn->ckpt_hdl;
      mbcsv_arg.info.obj_set.i_obj = obj;
      if(obj ==  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
        mbcsv_arg.info.obj_set.i_val = ssn->ws_flag;
      else
        mbcsv_arg.info.obj_set.i_val = ssn->ws_timer;
    }
  else
    {
      mbcsv_arg.info.obj_get.i_ckpt_hdl = ssn->ckpt_hdl;
      mbcsv_arg.info.obj_get.i_obj = obj;
    }

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"OBJ SET", NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  if(action == NCS_MBCSV_OP_OBJ_GET)
    {
      if(obj ==  NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
        {
          ssn->ws_flag = mbcsv_arg.info.obj_get.o_val;
          printf("\n Warm Sync Flage= %d\n",ssn->ws_flag);
        }
      else
        {
          ssn->ws_timer = mbcsv_arg.info.obj_get.o_val;
          printf("\n Warm Sync Timer Value =  %d\n", ssn->ws_timer);
        }
    }

  return NCSCC_RC_SUCCESS;    
}

/* SND MEG ROUTINES */
uint32_t   mbcstm_svc_cp_send(uint32_t svc_index, uint32_t ssn_index,  
                           NCS_MBCSV_ACT_TYPE  action,uint32_t reo_type,
                           long reo_hdl, NCS_MBCSV_MSG_TYPE send_type)
{
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_svc_cp_send";

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];


  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.send_ckpt.i_action = action;
  mbcsv_arg.info.send_ckpt.i_ckpt_hdl = ssn->ckpt_hdl;
  mbcsv_arg.info.send_ckpt.i_reo_hdl  = reo_hdl;
  mbcsv_arg.info.send_ckpt.i_reo_type = reo_type;
  mbcsv_arg.info.send_ckpt.i_send_type = send_type;

  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"SEND_CKPT", 
                   NCSCC_RC_FAILURE);        
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;

}

uint32_t   mbcstm_svc_data_request(uint32_t svc_index, uint32_t ssn_index)
{   
  NCS_MBCSV_ARG     mbcsv_arg;
  NCS_UBAID  *uba = NULL;
  uint8_t*      data;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_svc_data_request";

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];

  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;

  uba = &mbcsv_arg.info.send_data_req.i_uba;
  if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"UBA_INIT_SPACE", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  data = ncs_enc_reserve_space(uba, 2*sizeof(uint32_t));
  if(data == NULL)
    {
      tet_printf("\n fake_encode_elem: DATA NULL");
      return NCSCC_RC_FAILURE;
    }
  ncs_encode_32bit(&data, ssn->data_req);
  ncs_encode_32bit(&data, ssn->data_req_count);
  ncs_enc_claim_space(uba, 2*sizeof(uint32_t));
  mbcsv_arg.info.send_data_req.i_ckpt_hdl = ssn->ckpt_hdl;
  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"DATA_REQUEST", 
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  ssn->warm_flag = MBCSTM_DATA_REQUEST;

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_svc_send_notify(uint32_t svc_index, uint32_t ssn_index,
                               NCS_MBCSV_NTFY_MSG_DEST msg_dest, 
                               char *str, uint32_t len)
{   
  NCS_MBCSV_ARG     mbcsv_arg;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_svc_send_notify";

  svc = &mbcstm_cb.svces[svc_index];
  ssn = &svc->ssns[ssn_index];

  memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));

  mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_NOTIFY;
  mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl;
  mbcsv_arg.info.send_notify.i_ckpt_hdl = ssn->ckpt_hdl;
  mbcsv_arg.info.send_notify.i_msg_dest = msg_dest;
  mbcsv_arg.info.send_notify.i_msg= (NCSCONTEXT) 1; /*what to pass?*/


  if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"SEND_NOTIFY",
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;
}

/* CALL BACK ROUTINES */ 

uint32_t  mbcstm_svc_err_ind(NCS_MBCSV_CB_ARG *arg)
{
  switch(arg->info.error.i_code)
    {
    case NCS_MBCSV_COLD_SYNC_TMR_EXP:
      tet_printf("\n ERROR:NCS_MBCSV_COLD_SYNC_TMR_EXP");
      printf("\n ERROR:NCS_MBCSV_COLD_SYNC_TMR_EXP");
      break;

    case NCS_MBCSV_WARM_SYNC_TMR_EXP:
      tet_printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_TMR_EXP");
      printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_TMR_EXP");
      break;

    case NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP:
      tet_printf("\n ERROR: NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP");
      printf("\n ERROR: NCS_MBCSV_DATA_RSP_CMPLT_TMR_EXP");
      break;

    case NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP:
      tet_printf("\n ERROR: NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP");
      printf("\n ERROR: NCS_MBCSV_COLD_SYNC_CMPL_TMR_EXP");
      break;

    case NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP:
      tet_printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP");
      printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_CMPL_TMR_EXP");
      break;

    case NCS_MBCSV_DATA_RESP_TERMINATED:
      tet_printf("\n ERROR:  NCS_MBCSV_DATA_RESP_TERMINATED");
      printf("\n ERROR:  NCS_MBCSV_DATA_RESP_TERMINATED");
      break;
    case NCS_MBCSV_COLD_SYNC_RESP_TERMINATED:
      tet_printf("\n ERROR:  NCS_MBCSV_COLD_SYNC_RESP_TERMINATED");
      printf("\n ERROR:  NCS_MBCSV_COLD_SYNC_RESP_TERMINATED");
      break;
    case NCS_MBCSV_WARM_SYNC_RESP_TERMINATED:
      tet_printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_RESP_TERMINATED");
      printf("\n ERROR:  NCS_MBCSV_WARM_SYNC_RESP_TERMINATED");
      break;
    }
  return NCSCC_RC_SUCCESS; /*change*/
}

uint32_t   mbcstm_svc_cb(NCS_MBCSV_CB_ARG *arg)
{
  uint32_t status = NCSCC_RC_SUCCESS;
  MBCSTM_SSN *ssn = (MBCSTM_SSN *)((long)arg->i_client_hdl);

  /*change
    uint32_t  svc_index = ssn->svc_index;*/
  uint32_t mbcstm_cb_test_cases(NCS_MBCSV_CB_ARG *);

  /* if any cb test cases are there */
  if(ssn->cb_test != MBCSTM_CB_NO_TEST)
    if(mbcstm_cb_test_cases(arg) == NCSCC_RC_SUCCESS) 
      /* note this return, carefull */
      {
        tet_printf("\n Returning NCSCC_RC_FAILURE from call back");
        switch(arg->i_op)
          {
          case NCS_MBCSV_CBOP_ENC:
            printf("\n Returning NCSCC_RC_FAILURE from NCS_MBCSV_CBOP_ENC:");
            break;
          case NCS_MBCSV_CBOP_DEC:
            printf("\n Returning NCSCC_RC_FAILURE from NCS_MBCSV_CBOP_DEC::");
            break;
          case NCS_MBCSV_CBOP_PEER:
            printf("\n Returning NCSCC_RC_FAILURE from NCS_MBCSV_CBOP_PEER:");
            break;
          case NCS_MBCSV_CBOP_NOTIFY:
            printf("\nReturning NCSCC_RC_FAILURE from NCS_MBCSV_CBOP_NOTIFY:");
            break;
          case NCS_MBCSV_CBOP_ERR_IND:
            printf("\nReturning NCSCC_RC_FAILURE frm NCS_MBCSV_CBOP_ERR_IND:");
            break; 
          case NCS_MBCSV_CBOP_ENC_NOTIFY:
            printf("\nReturning NCSCC_RC_FAILURE from NCS_MBCSV_CBOP_ENC_NOTIFY:");
            break;
          }
        return NCSCC_RC_FAILURE;                               
      }

  pthread_mutex_unlock( &mutex_cb);

  switch (arg->i_op)
    {
    case NCS_MBCSV_CBOP_ENC:
      status = mbcstm_svc_encode_cb(arg);
      break;

    case NCS_MBCSV_CBOP_DEC:
      status = mbcstm_svc_decode_cb(arg);
      break;

    case NCS_MBCSV_CBOP_PEER:
      status = mbcstm_svc_peer_cb(arg);
      break;

    case NCS_MBCSV_CBOP_NOTIFY:
      status = mbcstm_svc_notify_cb(arg);
      break;

    case NCS_MBCSV_CBOP_ERR_IND:
      status = mbcstm_svc_err_ind(arg);
      break;

    case NCS_MBCSV_CBOP_ENC_NOTIFY:
     status = mbcstm_svc_enc_notify_cb(arg);
      break;

    default:
      status = NCSCC_RC_FAILURE;
      break;
    }

  /*Note:  we need to add our own code here for standbys */
  return status;
}

static uint32_t mbcstm_data_encode(uint32_t reo_type, uint32_t reo_hdl, 
                                NCS_UBAID *uba,  MBCSTM_SSN *ssn);

uint32_t   mbcstm_svc_encode_cb(NCS_MBCSV_CB_ARG *arg)
{
 
  MBCSTM_SSN      *ssn = (MBCSTM_SSN *) ((long) arg->i_client_hdl);
  /*change
    uint32_t data_index;*/
  NCS_UBAID *uba = &arg->info.encode.io_uba;
  uint8_t*    data;
  uint32_t  mbcstm_destroy_data_point(uint32_t,uint32_t);

  if(ssn == NULL)
    {
      tet_printf("\n mbcsv_fake_process_enc_cb: Client handle is NULL");
      return NCSCC_RC_FAILURE;
    }    
  switch(arg->info.encode.io_msg_type)
    {
    case 1:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_ASYNC_UPDATE");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_ASYNC_UPDATE");
      break;
    case 2:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_REQ");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_REQ");
      break;
    case 3:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP");
      break;
    case 4:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE");
      break;
    case 5:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_REQ");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_REQ");
      break;
    case 6:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP");
      break;
    case 7:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE");
      break;
    case 8:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_REQ");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_REQ");
      break;
    case 9:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP");
      break;
    case 10:tet_printf("ENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP_COMPLETE");
      printf("\nENCODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP_COMPLETE");
      break;
    }
  fflush(stdout);
  switch (arg->info.encode.io_msg_type)
    {
    case NCS_MBCSV_MSG_ASYNC_UPDATE:
      if(arg->info.encode.io_action==NCS_MBCSV_ACT_ADD)
        {
          if(mbcstm_data_encode(arg->info.encode.io_reo_type, 
                                arg->info.encode.io_reo_hdl, 
                                &arg->info.encode.io_uba, ssn)
             ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR : Not able to Encode Data\n");
              return NCSCC_RC_FAILURE;        
            }
        }
      if(arg->info.encode.io_action==NCS_MBCSV_ACT_RMV)
        {
          /*
            U r supposed to ask all the peers to REMOVE the data element
            arg->info.encode.io_reo_hdl from their data inventory
            remove the last data element
          */
          mbcstm_destroy_data_point(1,1);
#if 0
          if(mbcstm_data_encode(arg->info.encode.io_reo_type, 
                                arg->info.encode.io_reo_hdl, 
                                &arg->info.encode.io_uba, ssn)
             ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR : Not able to Encode Data\n");
              return NCSCC_RC_FAILURE;        
            }
#endif
        }
      if(arg->info.encode.io_action==NCS_MBCSV_ACT_UPDATE)
        {
          /*
            U r supposed to ask all the peers to UPDATE the data element
            arg->info.encode.io_reo_hdl with this new data
          */
          if(mbcstm_data_encode(arg->info.encode.io_reo_type, 
                                arg->info.encode.io_reo_hdl, 
                                &arg->info.encode.io_uba, ssn)
             ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR : Not able to Encode Data\n");
              return NCSCC_RC_FAILURE;        
            }
        }
      if(arg->info.encode.io_action==NCS_MBCSV_ACT_DONT_CARE)
        {
          /*Dont Care: no encode*/
          printf("\t");
        }
      break;

    case NCS_MBCSV_MSG_COLD_SYNC_REQ:
      if(ssn->perf_flag == 1)
        {
          struct timeval t;
          gettimeofday(&t, NULL);
          ssn->perf_init_time = (double) t.tv_sec + (double) t.tv_usec * 1e-6;
        }
      break;

    case NCS_MBCSV_MSG_COLD_SYNC_RESP:
      if(ssn->perf_flag != 1)
        {
          if(mbcstm_data_encode(NORMAL_DATA, arg->info.encode.io_reo_hdl,
                                &arg->info.encode.io_uba, ssn)
             ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR : Not able to Encode Data\n");
              return NCSCC_RC_FAILURE;
            }
          if(ssn->data_count == 0 || ssn->sync_count++ == ssn->data_count)
            {
              arg->info.encode.io_msg_type = 
                NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
              ssn->sync_count = 0;
              ssn->cold_flag = MBCSTM_SYNC_COMPLETE;
            }
          else
            {
              arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP;
              ssn->cold_flag = MBCSTM_SYNC_PROCESS;
            }
        }
      else
        {
          uint32_t size,crc;
          char *msg;
          if(ssn->perf_data_sent > ssn->perf_data_size)
            {
              arg->info.encode.io_msg_type = 
                NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
              break;
            }

          arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP;
          size = (ssn->perf_msg_size += ssn->perf_msg_inc);

          msg = (char *) malloc(sizeof(ssn->perf_msg_size));
          memset(msg,'c',ssn->perf_msg_size);
          msg[size-1] = '\0';
          crc = mbcstm_crc(msg,size);

          data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
          ncs_encode_32bit(&data,size);
          ncs_enc_claim_space(uba, sizeof(uint32_t));

          data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
          ncs_encode_32bit(&data,crc);
          ncs_enc_claim_space(uba, sizeof(uint32_t));

          ncs_encode_n_octets_in_uba(uba, (uint8_t*)msg, size);
        }
      break;

    case NCS_MBCSV_MSG_WARM_SYNC_REQ:
      break;

    case NCS_MBCSV_MSG_WARM_SYNC_RESP:
      if(mbcstm_data_encode(NORMAL_DATA, 
                            arg->info.encode.io_reo_hdl,
                            &arg->info.encode.io_uba, ssn)==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR : Not able to Encode Data\n");
          return NCSCC_RC_FAILURE;
        }

      if(ssn->data_count == 0 || ssn->sync_count++ == ssn->data_count)
        {
          arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;
          ssn->sync_count = 0;
          ssn->warm_flag =  MBCSTM_SYNC_COMPLETE;
        }
      else
        {
          arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP;
          ssn->warm_flag =  MBCSTM_SYNC_PROCESS;
        }
      break;

    case NCS_MBCSV_MSG_DATA_REQ:
      /*    mbcstm_data_encode(CONTROL_DATA, ssn->data_req,
            &arg->info.encode.io_uba, ssn); */
      break;
    case NCS_MBCSV_MSG_DATA_RESP:
#if 0
      printf("\n\t1  Encode Data Resp: data_req= %d data_req_count = %d",
             ssn->data_req,ssn->data_req_count);
#endif
      if(mbcstm_data_encode(NORMAL_DATA, ssn->data_req,
                            &arg->info.encode.io_uba, ssn)==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR : Not able to Encode Data\n");
          return NCSCC_RC_FAILURE;
        }
      ssn->data_req++;
      arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP;
      if(--ssn->data_req_count == 0)
        arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;    
#if 0
      printf("\n\t2  Encode Data Resp: data_req= %d data_req_count = %d",
             ssn->data_req,ssn->data_req_count);
#endif
      break;

    default:
      perror("\n\n\nFrom Encode Callback ... returning Failure");
      return NCSCC_RC_FAILURE;
    }
  /*  printf(".... Success");  */
  return NCSCC_RC_SUCCESS;
}

static uint32_t mbcstm_data_encode(uint32_t reo_type, uint32_t reo_hdl, 
                                NCS_UBAID *uba,  MBCSTM_SSN *ssn)
{
  uint8_t*    data;
  SSN_PERF_DATA *info;

  if(uba == NULL)
    {
      tet_printf("\n fake_encode_elem: User buff is NULL");
      return NCSCC_RC_FAILURE;
    } 
  switch (reo_type)
    {
    case  NORMAL_DATA: 
      data = ncs_enc_reserve_space(uba, sizeof(MBCSTM_CSI_DATA)+sizeof(uint32_t));
      if(data == NULL)
        {
          tet_printf("\n fake_encode_elem: DATA NULL");
          return NCSCC_RC_FAILURE;
        }
      ncs_encode_32bit(&data, reo_hdl);
      ncs_encode_32bit(&data, ssn->data[reo_hdl].data_id);
      ncs_encode_32bit(&data, ssn->data[reo_hdl].sec);
      ncs_encode_32bit(&data, ssn->data[reo_hdl].usec);                
      ncs_enc_claim_space(uba, sizeof(MBCSTM_CSI_DATA)+sizeof(uint32_t));
#if 0
      printf("\n\tEndcode data :%d  %d %d %d\n",reo_hdl,
             ssn->data[reo_hdl].data_id,
             ssn->data[reo_hdl].sec,ssn->data[reo_hdl].usec);
      fflush(stdout);
#endif
      break;

    case  CONTROL_DATA:
      data = ncs_enc_reserve_space(uba, 2*sizeof(uint32_t));
      if(data == NULL)
        {
          tet_printf("\n fake_encode_elem: DATA NULL");
          return NCSCC_RC_FAILURE;
        }
      ncs_encode_32bit(&data, ssn->data_req);
      ncs_encode_32bit(&data, ssn->data_req_count);
      ncs_enc_claim_space(uba, 2*sizeof(uint32_t));
      break;

    case  PERFORMANCE_DATA:
      info = (SSN_PERF_DATA *)((long) reo_hdl);
      data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
      ncs_encode_32bit(&data,info->length);
      ncs_enc_claim_space(uba, sizeof(uint32_t));

      data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
      ncs_encode_32bit(&data,info->crc);
      ncs_enc_claim_space(uba, sizeof(uint32_t));

      ncs_encode_n_octets_in_uba(uba, (uint8_t*)info->msg, info->length);
      break;
    default:
      return NCSCC_RC_FAILURE;
    }

  return NCSCC_RC_SUCCESS;
}

static uint32_t mbcstm_data_decode(MBCSTM_CSI_DATA_TYPE type, NCS_UBAID *uba,
                                MBCSTM_SSN *ssn)
{
  uint8_t*    data;
  uint32_t    data_index;
  uint8_t     data_buff[1024];
  SSN_PERF_DATA *info;
  /*change*/    
  uint32_t mbcstm_verify_sync_msg(SSN_PERF_DATA *,MBCSTM_SSN *);

  if(uba == NULL)
    {
      tet_printf("\n fake_encode_elem: User buff is NULL");
      return NCSCC_RC_FAILURE;
    }              
  switch(type)
    {
    case NORMAL_DATA :        
      data = ncs_dec_flatten_space(uba,data_buff, 
                                   sizeof(MBCSTM_CSI_DATA)+sizeof(uint32_t));
      data_index = ncs_decode_32bit(&data);
      ssn->data[data_index].data_id = ncs_decode_32bit(&data);
      ssn->data[data_index].sec = ncs_decode_32bit(&data);
      ssn->data[data_index].usec = ncs_decode_32bit(&data);
      ssn->data_count=ssn->data[data_index].data_id;
#if 0
      printf("\n\tDecode Data.. %d %d %d\n",ssn->data[data_index].data_id,
             ssn->data[data_index].sec,ssn->data[data_index].usec);
#endif
      ncs_dec_skip_space(uba, sizeof(MBCSTM_CSI_DATA)+sizeof(uint32_t));
      break;

    case CONTROL_DATA :
      data = ncs_dec_flatten_space(uba,data_buff, sizeof(uint32_t));
      ssn->data_req  = ncs_decode_32bit(&data);
      ncs_dec_skip_space(uba, sizeof(uint32_t));     
      ssn->data_req_count = ncs_decode_32bit(&data);
      ncs_dec_skip_space(uba, sizeof(uint32_t));
#if 0
      printf("\nDecoded Data_req = %d Data_req_count = %d\n",ssn->data_req,
             ssn->data_req_count);
      fflush(stdout);
#endif
      break;

    case PERFORMANCE_DATA :
      info = (SSN_PERF_DATA *) malloc(sizeof(SSN_PERF_DATA));

      data = ncs_dec_flatten_space(uba,data_buff, sizeof(uint32_t));
      info->length  = ncs_decode_32bit(&data);
      ncs_dec_skip_space(uba, sizeof(uint32_t));
      info->crc = ncs_decode_32bit(&data);
      ncs_dec_skip_space(uba, sizeof(uint32_t));

      info->msg = (char *) malloc(info->length);
      memset(info->msg, 0, sizeof(info->length));
      ncs_decode_n_octets_from_uba(uba, (uint8_t*)info->msg, info->length);
      if(mbcstm_verify_sync_msg(info,ssn) != NCSCC_RC_SUCCESS)
        return NCSCC_RC_FAILURE;
      break;
    default:
      tet_printf("\n wrong decode type");
    }

  return NCSCC_RC_SUCCESS;
}

uint32_t   mbcstm_svc_decode_cb(NCS_MBCSV_CB_ARG *arg)
{
 
  MBCSTM_SSN *ssn  = (MBCSTM_SSN  *)((long)arg->i_client_hdl);

  switch(arg->info.decode.i_msg_type)
    {
    case 1:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_ASYNC_UPDATE");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_ASYNC_UPDATE");
      break;
    case 2:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_REQ");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_REQ");
      break;
    case 3:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP");
      break;
    case 4:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE");
      break;
    case 5:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_REQ");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_REQ");
      break;
    case 6:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP");
      break;
    case 7:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE");
      break;
    case 8:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_DATA_REQ");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_DATA_REQ");
      break;
    case 9:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP");
      break;
    case 10:tet_printf("DECODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP_COMPLETE");
      printf("\nDECODE MSG TYPE:NCS_MBCSV_MSG_DATA_RESP_COMPLETE");
      break;
    }
  fflush(stdout);
  switch (arg->info.decode.i_msg_type)
    {
    case NCS_MBCSV_MSG_ASYNC_UPDATE:
      if(ssn->perf_flag == 1)
        mbcstm_data_decode(PERFORMANCE_DATA,&arg->info.decode.i_uba, ssn);
      else
        {
          if(arg->info.decode.i_action==NCS_MBCSV_ACT_ADD)
            {
              if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
                  ==NCSCC_RC_FAILURE)
                {
                  perror("\nCB ERROR: Not able to Decode data\n");
                  return NCSCC_RC_FAILURE;
                }  
            }   
          if(arg->info.decode.i_action==NCS_MBCSV_ACT_RMV)
            {
              /*
                u r supposed to remove the data element with index
                decrement the data_count
              */
              mbcstm_cb.svces[1].ssns[1].data_count--;
#if 0
              if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
                  ==NCSCC_RC_FAILURE)
                {
                  perror("\nCB ERROR: Not able to Decode data\n");
                  return NCSCC_RC_FAILURE;
                }  
#endif
            }   
          if(arg->info.decode.i_action==NCS_MBCSV_ACT_UPDATE)
            {
              if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
                  ==NCSCC_RC_FAILURE)
                {
                  perror("\nCB ERROR: Not able to Decode data\n");
                  return NCSCC_RC_FAILURE;
                }  
            }   
          if(arg->info.decode.i_action==NCS_MBCSV_ACT_DONT_CARE)
            {
              /*Dont Care: no encode ... so no decode*/
              printf("\t");
            }   
        }
      break;

    case NCS_MBCSV_MSG_COLD_SYNC_REQ:
      ssn->cold_flag = MBCSTM_SYNC_PROCESS;
      break;

    case NCS_MBCSV_MSG_COLD_SYNC_RESP:
      if(ssn->perf_flag == 1)
        mbcstm_data_decode(PERFORMANCE_DATA,&arg->info.decode.i_uba, ssn);
      else            
        {
          if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
              ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR: Not able to Decode data\n");
              return NCSCC_RC_FAILURE;
            }
        }
      break;

    case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:
      if(ssn->perf_flag != 1)
        {
          if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
              ==NCSCC_RC_FAILURE)
            {
              perror("\nCB ERROR: Not able to Decode data\n");
              return NCSCC_RC_FAILURE;
            }
        }
      else
        {
          struct timeval t;
          gettimeofday(&t, NULL);
          ssn->perf_final_time = (double) t.tv_sec + (double) t.tv_usec * 1e-6;
        }
      ssn->cold_flag = MBCSTM_SYNC_COMPLETE;
      break;

    case NCS_MBCSV_MSG_WARM_SYNC_REQ:
      ssn->warm_flag = MBCSTM_SYNC_PROCESS;
      break;

    case NCS_MBCSV_MSG_WARM_SYNC_RESP:
      if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
          ==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR: Not able to Decode data\n");
          return NCSCC_RC_FAILURE;
        }
      break;

    case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:
      if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
          ==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR: Not able to Decode data\n");
          return NCSCC_RC_FAILURE;
        }
      ssn->warm_flag = MBCSTM_SYNC_COMPLETE;
      break;

    case NCS_MBCSV_MSG_DATA_REQ:
      tet_printf("\n data request decode");
      mbcstm_data_decode(CONTROL_DATA,&arg->info.decode.i_uba, ssn);    
      break;

    case NCS_MBCSV_MSG_DATA_RESP:
      if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
          ==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR: Not able to Decode data\n");
          return NCSCC_RC_FAILURE;
        }
      break;
    case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
      if( mbcstm_data_decode(NORMAL_DATA,&arg->info.decode.i_uba, ssn)
          ==NCSCC_RC_FAILURE)
        {
          perror("\nCB ERROR: Not able to Decode data\n");
          return NCSCC_RC_FAILURE;
        }
      break;
    default :
      return NCSCC_RC_FAILURE;
    }
  /*printf(".... Success");  */
  return NCSCC_RC_SUCCESS;
}


uint32_t   mbcstm_svc_peer_cb(NCS_MBCSV_CB_ARG *arg)
{
  struct timeval t;
  MBCSTM_SSN *ssn  = (MBCSTM_SSN  *)((long)arg->i_client_hdl);

  gettimeofday(&t,NULL);
  ssn->perf_final_time = (double) t.tv_sec + (double) t.tv_usec * 1e-6;
  ssn->cb_test_result = NCSCC_RC_SUCCESS;    
#if 0
  printf("\n Peers Svc_id %d Version %d \n",arg->info.peer.i_service,
         arg->info.peer.i_peer_version);
#endif
  return NCSCC_RC_SUCCESS;

}
uint32_t   mbcstm_svc_enc_notify_cb(NCS_MBCSV_CB_ARG *arg)
{
  char    fun_name[]  = "mbcstm_svc_enc_notify_cb";
  uint8_t* data;
  char  str[100]="Hello Peer";
  int len=strlen(str)+1;
  NCS_UBAID  *uba = &arg->info.notify.i_uba;
  uint16_t  peer_version = arg->info.notify.i_peer_version;
/*  NCSCONTEXT msg = &arg->info.notify.i_msg; how to use this*/
  printf("\nCallback Enc Notify Msg: Peer's Version = %d \n",peer_version);

  if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba))
    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"UBA_INIT_SPACE",
                   NCSCC_RC_FAILURE);
      return NCSCC_RC_FAILURE;
    }

  data = ncs_enc_reserve_space(uba, sizeof(uint32_t));
  ncs_encode_32bit(&data,len);
  ncs_enc_claim_space(uba, sizeof(uint32_t));
  ncs_encode_n_octets_in_uba(uba, (uint8_t*)str, len);

  return NCSCC_RC_SUCCESS;
}
uint32_t   mbcstm_svc_notify_cb(NCS_MBCSV_CB_ARG *arg)
{
  uint32_t length;
  uint8_t* data;
  uint8_t  data_buff[1024];
  char  str[100];
  NCS_UBAID  *uba = &arg->info.notify.i_uba;
  uint16_t  peer_version = arg->info.notify.i_peer_version;
/*  NCSCONTEXT msg = &arg->info.notify.i_msg; how to use this*/

  data = ncs_dec_flatten_space(uba, data_buff, sizeof(uint32_t));
  length = ncs_decode_32bit(&data);
  ncs_dec_skip_space(uba, sizeof(uint32_t));
  ncs_decode_n_octets_from_uba(uba, (uint8_t *)str, length);
  tet_printf("\n NTFY MSG: %s \n", str);
   printf("\n NTFY MSG: %s \t Length= %ld Peer Version= %d \n", str,(long)strlen(str),peer_version);

  return NCSCC_RC_SUCCESS;
}

/* utility functions */

uint32_t mbcstm_print(char *file, char *fun, uint32_t line, char *event,uint32_t status)
{
  if (status == NCSCC_RC_FAILURE)
    {
      /*tet_printf("\n AT LINE %d IN FUNCTION %s IN FILE %s CALL TO %s FAILED",
        line, fun, file, event);*/

      printf("\n AT LINE %d IN FUNCTION %s IN FILE %s CALL TO %s FAILED", 
             line, fun, file, event);
      /*change
        printf(mbcstm_cb.log_file_fd,"\n AT LINE %d IN FUNCTION %s IN FILE \
        %s CALL TO %s FAILED", line, fun, file, event);*/
    }
  else
    {
      /*tet_printf("\n AT LINE %d IN FUNCTION %s IN FILE %s CALL TO %s \
        SUCCEED", line, fun, file, event);*/

      printf("\n AT LINE %d IN FUNCTION %s IN FILE %s CALL TO %s SUCCEED", 
             line, fun, file, event);
      /*change
        tet_printf(mbcstm_cb.log_file_fd,"\n AT LINE %d IN FUNCTION %s IN\
        FILE %s CALL TO %s SUCCEED",line, fun, file, event);*/
    }
  /*change*/
  return 1;
}

uint32_t mbcstm_config_print()
{
  uint32_t svc_index, ssn_index, svc_count,ssn_count;
  MBCSTM_SVC *svc;
  MBCSTM_SSN *ssn;

  svc_count = mbcstm_cb.svc_count;
  for(svc_index = 1; svc_index <= svc_count; svc_index++)
    {
      tet_printf("\n NUMBER OF SVCES IN SYSTEM : %d",svc_count);
      tet_printf("\n DATA ON SVC %d", svc_index);
      svc = &mbcstm_cb.svces[svc_index];
      ssn_count = mbcstm_cb.svces[svc_index].ssn_count;
      tet_printf("\n \t MY CSIS COUNT %d",ssn_count);
      tet_printf("\n \t MY SELECT OBJECT %x",svc->sel_obj);
      for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
        {
          ssn = &mbcstm_cb.svces[svc_index].ssns[ssn_index];
          tet_printf("\n \t DEST_ID:%d::ANC:%llx::PWE_HADLE:%llx::CSI_HANDLE:%x::CSI_ROLE:%d", 
                     ssn->dest_id,ssn->anchor,ssn->pwe_hdl,ssn->ckpt_hdl,
                     ssn->csi_role);
        }
    }
  /*change*/
  return 1;
}

static uint32_t sync_id;
uint32_t   mbcstm_sync_point()
{
  char    fun_name[]  = "mbcstm_sync_point";
  /*change
    struct  tet_synmsg tet_msg;*/

  /* tet_msg.tsm_data = (char *) NULL;
     tet_msg.tsm_dlen = sizeof(MBCSTM_MSG);
     tet_msg.tsm_flags = msg_type;    */
   int num_sys;
   int *sysnames;
   int *sys;
   int i;

   num_sys = tet_remgetlist(&sysnames);
   sys = (int *)malloc((num_sys+1)*sizeof(int));
   for(i=0;i<num_sys;i++)
      sys[i] = sysnames[i];
   sys[num_sys] = tet_remgetsys();

  if (tet_remsync(0, sys, num_sys,MBCSTM_SYNC_TIMEOUT, 
                  TET_SV_NO,NULL) != 0)

    {
      mbcstm_print(__FILE__, fun_name, __LINE__,"TET SYNC", NCSCC_RC_FAILURE);

      return NCSCC_RC_FAILURE;                  
    }

  sync_id++;
  return NCSCC_RC_SUCCESS;
} 
