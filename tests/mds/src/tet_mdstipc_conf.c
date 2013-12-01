#include <tet_api.h>
#include "mds_papi.h"
#include "ncs_mda_papi.h"
#include "tet_mdstipc.h"
#include "osaf_poll.h"
extern int fill_syncparameters(int);
/****************** ADEST WRAPPERS ***********************/
uint32_t adest_get_handle(void)
{
  memset(&ada_info,'\0', sizeof(ada_info));
  memset(&gl_tet_adest,'\0', sizeof(gl_tet_adest));

  ada_info.req=NCSADA_GET_HDLS;    

  if(ncsada_api(&ada_info)==NCSCC_RC_SUCCESS)
    {

      gl_tet_adest.adest=ada_info.info.adest_get_hdls.o_adest; 
      printf("\nADEST <%llx > : GET_HDLS is SUCCESSFUL",gl_tet_adest.adest);
      gl_tet_adest.mds_pwe1_hdl=ada_info.info.adest_get_hdls.o_mds_pwe1_hdl; 
      gl_tet_adest.mds_adest_hdl=ada_info.info.adest_get_hdls.o_mds_adest_hdl;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsada_api: GET_HDLS has FAILED");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t create_pwe_on_adest(MDS_HDL mds_adest_hdl,
                          PW_ENV_ID  pwe_id)
{
  ada_info.req=NCSADA_PWE_CREATE;

  ada_info.info.pwe_create.i_mds_adest_hdl=mds_adest_hdl;
  ada_info.info.pwe_create.i_pwe_id=pwe_id;      

  if(ncsada_api(&ada_info)==NCSCC_RC_SUCCESS)
    {

      printf("\nPWE_CREATE is SUCCESSFUL : PWE = %d",pwe_id);
      gl_tet_adest.pwe[gl_tet_adest.pwe_count].pwe_id=pwe_id;
      gl_tet_adest.pwe[gl_tet_adest.pwe_count].mds_pwe_hdl=
        ada_info.info.pwe_create.o_mds_pwe_hdl;
      gl_tet_adest.pwe_count++;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to PWE_CREATE on ADEST has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t destroy_pwe_on_adest(MDS_HDL mds_pwe_hdl)
{

  ada_info.req=NCSADA_PWE_DESTROY;    

  ada_info.info.pwe_destroy.i_mds_pwe_hdl=mds_pwe_hdl; 

  if(ncsada_api(&ada_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nADEST: PWE_DESTROY is SUCCESSFUL");
      gl_tet_adest.pwe_count--;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest tO PWE_DESTROY on ADEST: has FAILED");
      return NCSCC_RC_FAILURE;
    }
}

/*********************** VDEST WRAPPERS **********************/

uint32_t create_vdest(NCS_VDEST_TYPE policy,
                   MDS_DEST vdest)
{
  memset(&vda_info,'\0', sizeof(vda_info));
  memset(&gl_tet_vdest[gl_vdest_indx],'\0',sizeof(TET_VDEST));

  vda_info.req=NCSVDA_VDEST_CREATE;  

  vda_info.info.vdest_create.i_policy=policy;
  vda_info.info.vdest_create.i_create_type=NCSVDA_VDEST_CREATE_SPECIFIC;
  vda_info.info.vdest_create.info.specified.i_vdest=
    gl_tet_vdest[gl_vdest_indx].vdest=vdest;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {

      printf("\n %lld : VDEST_CREATE is SUCCESSFUL", vdest);
      fflush(stdout);
      gl_tet_vdest[gl_vdest_indx].mds_pwe1_hdl=
        vda_info.info.vdest_create.o_mds_pwe1_hdl;
      gl_tet_vdest[gl_vdest_indx].mds_vdest_hdl=
        vda_info.info.vdest_create.o_mds_vdest_hdl;
      gl_vdest_indx++;    
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_CREATE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t destroy_vdest(MDS_DEST vdest)
{
  memset(&vda_info,'\0', sizeof(vda_info)); /*zeroizing*/
  /*request*/
  vda_info.req=NCSVDA_VDEST_DESTROY;

  vda_info.info.vdest_destroy.i_create_type=NCSVDA_VDEST_CREATE_SPECIFIC;
  vda_info.info.vdest_destroy.i_vdest=vdest;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n %lld : VDEST_DESTROY is SUCCESSFULL",vdest);
      fflush(stdout);
      memset(&gl_tet_vdest[gl_vdest_indx],'\0',sizeof(TET_VDEST));
      gl_vdest_indx--;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_DESTROY has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t create_named_vdest(bool persistent,
                         NCS_VDEST_TYPE policy,
                         char *vname)
{
  memset(&vda_info,'\0', sizeof(vda_info));
  memset(&gl_tet_vdest[gl_vdest_indx],'\0',sizeof(TET_VDEST));  

  vda_info.req=NCSVDA_VDEST_CREATE;  
  

  vda_info.info.vdest_create.i_persistent=persistent;
  vda_info.info.vdest_create.i_policy=policy;
  vda_info.info.vdest_create.i_create_type=NCSVDA_VDEST_CREATE_NAMED; 
  if(vname)
    {
      vda_info.info.vdest_create.info.named.i_name.length =
        (uint16_t)(strlen(vname)+1);/*FIX THIS*/
      memcpy(vda_info.info.vdest_create.info.named.i_name.value,vname,
                      (uint16_t)(strlen(vname)+1));
    }
  else
    {
      vda_info.info.vdest_create.info.named.i_name.length =0;
       memset(&(vda_info.info.vdest_create.info.named.i_name.value), 0, 
                                        sizeof(vda_info.info.vdest_create.info.named.i_name.value));
    }
  if(persistent)
    {
      printf("Instructs VDS to retrain the vname and the vdest mapping\n");
    }

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {

      printf("\nNAMED VDEST_CREATE is SUCCESSFULL\n");
      gl_tet_vdest[gl_vdest_indx].vdest=
        vda_info.info.vdest_create.info.named.o_vdest; 
      gl_tet_vdest[gl_vdest_indx].mds_pwe1_hdl=
        vda_info.info.vdest_create.o_mds_pwe1_hdl;
      gl_tet_vdest[gl_vdest_indx].mds_vdest_hdl=
        vda_info.info.vdest_create.o_mds_vdest_hdl;
      gl_vdest_indx++;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nNAMED VDEST_CREATE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t destroy_named_vdest(bool non_persistent,
                          MDS_DEST vdest,
                          char *vname)
{
  
  memset(&vda_info,'\0', sizeof(vda_info));

  vda_info.req=NCSVDA_VDEST_DESTROY;

  vda_info.info.vdest_destroy.i_create_type=NCSVDA_VDEST_CREATE_NAMED;
  vda_info.info.vdest_destroy.i_vdest=vdest;
  vda_info.info.vdest_destroy.i_make_vdest_non_persistent=non_persistent;
  vda_info.info.vdest_destroy.i_name.length=(uint16_t)(strlen(vname)+1);
  memcpy(vda_info.info.vdest_destroy.i_name.value,vname,
                  (uint16_t)(strlen(vname)+1));

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n %lld : VDEST_NAMED DESTROY is SUCCESSFULL\n",vdest);
      memset(&gl_tet_vdest[gl_vdest_indx],'\0',sizeof(TET_VDEST));
      gl_vdest_indx--;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_NAMED DESTROY has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

MDS_DEST vdest_lookup(char *vname)
{
  memset(&vda_info,'\0', sizeof(vda_info));

  vda_info.req=NCSVDA_VDEST_LOOKUP;

  if(vname)
    {
      vda_info.info.vdest_lookup.i_name.length =(uint16_t)(strlen(vname)+1);
      memcpy(vda_info.info.vdest_lookup.i_name.value,vname,
                      (uint16_t)(strlen(vname)+1));
    }
  else
    {
      vda_info.info.vdest_lookup.i_name.length =0;
      memset(&(vda_info.info.vdest_lookup.i_name.value), 0, 
                                        sizeof(vda_info.info.vdest_lookup.i_name.value));
    }
  

  if(ncsvda_api(&vda_info)==NCSCC_RC_FAILURE)
    {
      printf("\nRequest to ncsvda_api: VDEST_LOOKUP has FAILED\n");  
      return NCSCC_RC_FAILURE;
    }
  else
    printf("\n %lld : VDEST_LOOKUP is SUCCESSFULL\n",
           vda_info.info.vdest_lookup.o_vdest);
  return(vda_info.info.vdest_lookup.o_vdest);
}

uint32_t vdest_change_role(MDS_DEST vdest,
                        V_DEST_RL new_role)
{
  memset(&vda_info,'\0', sizeof(vda_info));

  vda_info.req=NCSVDA_VDEST_CHG_ROLE;

  vda_info.info.vdest_chg_role.i_vdest=vdest;
  vda_info.info.vdest_chg_role.i_new_role=new_role;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nVDEST_CHANGE ROLE to %d is SUCCESSFULL",new_role);
      return NCSCC_RC_SUCCESS;
    }
  else
    return NCSCC_RC_FAILURE;
}

uint32_t create_pwe_on_vdest(MDS_HDL mds_vdest_hdl,
                          PW_ENV_ID pwe_id)
{
  int i;
  memset(&vda_info,'\0', sizeof(vda_info));

  vda_info.req=NCSVDA_PWE_CREATE;

  vda_info.info.pwe_create.i_mds_vdest_hdl=mds_vdest_hdl;
  vda_info.info.pwe_create.i_pwe_id=pwe_id;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nVDEST_PWE CREATE PWE= %d is SUCCESSFULL",pwe_id);

      for(i=0;i<gl_vdest_indx;i++)
        {
          if(gl_tet_vdest[i].mds_vdest_hdl==mds_vdest_hdl)
            {
              gl_tet_vdest[i].pwe[gl_tet_vdest[i].pwe_count].pwe_id=pwe_id;
              gl_tet_vdest[i].pwe[gl_tet_vdest[i].pwe_count].mds_pwe_hdl=
                vda_info.info.pwe_create.o_mds_pwe_hdl;
              gl_tet_vdest[i].pwe_count++; 
            }
        }
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_PWE CREATE has FAILED");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t destroy_pwe_on_vdest(MDS_HDL mds_pwe_hdl)
{ 
  int i,j;
  memset(&vda_info,'\0',sizeof(vda_info)); 

  vda_info.req=NCSVDA_PWE_DESTROY;

  vda_info.info.pwe_destroy.i_mds_pwe_hdl=mds_pwe_hdl;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      for(i=0;i<gl_vdest_indx;i++)
        {
          for(j=gl_tet_vdest[i].pwe_count-1;j>=0;j--)
            {
              if(gl_tet_vdest[i].pwe[j].mds_pwe_hdl==mds_pwe_hdl)
                {
                  gl_tet_vdest[i].pwe_count--;    
                  break;
                }
            }
        }   
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_PWE DESTROY has FAILED");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t tet_mds_svc_callback(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  switch(mds_to_svc_info->i_op)
    {
    case MDS_CALLBACK_COPY: 
      if(gl_COPY_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return (tet_mds_cb_cpy(mds_to_svc_info));
      break;
    case MDS_CALLBACK_ENC:
      if(gl_ENC_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return (tet_mds_cb_enc(mds_to_svc_info));
      break;
    case MDS_CALLBACK_DEC: 
      if(gl_DEC_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return( tet_mds_cb_dec(mds_to_svc_info));
      break;
    case MDS_CALLBACK_ENC_FLAT: 
      if(gl_ENC_FLAT_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return ( tet_mds_cb_enc_flat(mds_to_svc_info));
      break;
    case MDS_CALLBACK_DEC_FLAT:
      if(gl_DEC_FLAT_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return(tet_mds_cb_dec_flat(mds_to_svc_info));
      break;
    case MDS_CALLBACK_RECEIVE:
      if(gl_RECEIVE_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return(tet_mds_cb_rcv(mds_to_svc_info));
      break;
    case MDS_CALLBACK_SVC_EVENT:
      if(gl_SYS_EVENT_CB_FAIL)
        return NCSCC_RC_FAILURE;
      else
        return(tet_mds_svc_event(mds_to_svc_info));
      break;
    case MDS_CALLBACK_SYS_EVENT:
      return(tet_mds_sys_event(mds_to_svc_info));
      break;
    case MDS_CALLBACK_QUIESCED_ACK: 
      return( tet_mds_cb_quiesced_ack(mds_to_svc_info));
      break;
    case MDS_CALLBACK_DIRECT_RECEIVE:
      return( tet_mds_cb_direct_rcv(mds_to_svc_info));
      break;
    default: perror("No such call back occurs");
      return 0;
    }
}

/************ SERVICE RELATED WRAPPERS **********************/
uint32_t mds_service_install(MDS_HDL mds_hdl,
                          MDS_SVC_ID svc_id,
                          MDS_SVC_PVT_SUB_PART_VER svc_pvt_ver,
                          NCSMDS_SCOPE_TYPE install_scope,
                          bool mds_q_ownership,
                          bool fail_no_active_sends)
{
  int i;
  memset(&svc_to_mds_info, 0, sizeof(svc_to_mds_info));

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_INSTALL;

  svc_to_mds_info.info.svc_install.i_mds_svc_pvt_ver= svc_pvt_ver;
  svc_to_mds_info.info.svc_install.i_fail_no_active_sends=fail_no_active_sends;
  svc_to_mds_info.info.svc_install.i_svc_cb=tet_mds_svc_callback;
  svc_to_mds_info.info.svc_install.i_yr_svc_hdl=gl_tet_svc.yr_svc_hdl;
  svc_to_mds_info.info.svc_install.i_install_scope=install_scope;
  svc_to_mds_info.info.svc_install.i_mds_q_ownership=mds_q_ownership;

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n %d : SERVICE INSTALL is SUCCESSFULL",svc_id);

      gl_tet_svc.dest=svc_to_mds_info.info.svc_install.o_dest;
      if(m_MDS_DEST_IS_AN_ADEST(gl_tet_svc.dest)==0)
        {
          gl_tet_svc.anc=svc_to_mds_info.info.svc_install.o_anc; 
        }
      gl_tet_svc.sel_obj=svc_to_mds_info.info.svc_install.o_sel_obj;
      gl_tet_svc.svc_cb=tet_mds_svc_callback;
      gl_tet_svc.install_scope=install_scope;
      gl_tet_svc.mds_q_ownership=mds_q_ownership;
      gl_tet_svc.svc_id=svc_id;
      gl_tet_svc.mds_svc_pvt_ver=svc_pvt_ver;
      
      if(m_MDS_DEST_IS_AN_ADEST(gl_tet_svc.dest)==0)
        {
          for(i=0;i<gl_vdest_indx;i++) 
            if(gl_tet_vdest[i].vdest==gl_tet_svc.dest)
              gl_tet_vdest[i].svc[gl_tet_vdest[i].svc_count++]=gl_tet_svc;
        }
      else
        gl_tet_adest.svc[gl_tet_adest.svc_count++]=gl_tet_svc;
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS INSTALL has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t mds_service_uninstall(MDS_HDL mds_hdl,
                            MDS_SVC_ID svc_id)
{
  int i,j,k,FOUND;
  uint32_t YES_ADEST;
  /*Find whether this Service is on Adest or Vdest*/
  YES_ADEST=is_service_on_adest(mds_hdl,svc_id);

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_UNINSTALL;    

  svc_to_mds_info.info.svc_uninstall.i_msg_free_cb=tet_mds_free_msg;

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n %d : SERVICE UNINSTALL is SUCCESSFULL",svc_id);

      FOUND=0;
      /*VDEST*/
      if(YES_ADEST==0)
        {
          for(i=0;i<gl_vdest_indx;i++)
            {
              for(j=0;j<gl_tet_vdest[i].svc_count;j++)
                {
                  if(gl_tet_vdest[i].svc[j].svc_id==
                     svc_id && gl_tet_vdest[i].mds_pwe1_hdl==mds_hdl)
                    {
                      --gl_tet_vdest[i].svc_count;
                      FOUND=1;
                      break;
                    }
                  if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                     gl_tet_vdest[i].mds_pwe1_hdl!=mds_hdl)
                    {
                      for(k=0;k<gl_tet_vdest[i].pwe_count;k++)
                        {
                          if(gl_tet_vdest[i].pwe[k].mds_pwe_hdl==mds_hdl)
                            {
                              --gl_tet_vdest[i].svc_count;
                              FOUND=1;
                              break;
                            }  
                        }
                    }
                }
              if(FOUND)
                break;
            }
        }
      /*ADEST*/
      if(YES_ADEST==1)
        {
          for(j=0;j<gl_tet_adest.svc_count;j++)
            {
              if(gl_tet_adest.svc[j].svc_id==svc_id && 
                 gl_tet_adest.mds_pwe1_hdl==mds_hdl)
                --gl_tet_adest.svc_count;
              if(gl_tet_adest.svc[j].svc_id==svc_id && 
                 gl_tet_adest.mds_pwe1_hdl!=mds_hdl)
                {
                  for(k=0;k<gl_tet_adest.pwe_count;k++)
                    {
                      if(gl_tet_adest.pwe[k].mds_pwe_hdl==mds_hdl)
                        --gl_tet_adest.svc_count;      
                    } 
                }  
            }  
        }
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS UNINSTALL has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
void tet_mds_free_msg(NCSCONTEXT msg_to_be_freed)
{
  TET_MDS_RECVD_MSG_INFO *p_recvd_info;
  
  p_recvd_info = (TET_MDS_RECVD_MSG_INFO *)msg_to_be_freed;
  if (p_recvd_info != NULL)
    {
      printf("Freeing up all the messages in the MDS Q");
      if (p_recvd_info->msg)
        free(p_recvd_info->msg);
      free(p_recvd_info);
    }
}

uint32_t mds_service_subscribe(MDS_HDL mds_hdl,
                            MDS_SVC_ID svc_id,
                            NCSMDS_SCOPE_TYPE scope,
                            uint8_t num_svcs,
                            MDS_SVC_ID *svc_ids)
{
  int i,j,k,l,FOUND;
  uint32_t YES_ADEST;
  /*Find whether this Service is on Adest or Vdest*/
  YES_ADEST=is_service_on_adest(mds_hdl,svc_id);

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SUBSCRIBE;    

  svc_to_mds_info.info.svc_subscribe.i_scope=scope;
  svc_to_mds_info.info.svc_subscribe.i_num_svcs=num_svcs;
  svc_to_mds_info.info.svc_subscribe.i_svc_ids=svc_ids;    

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS SERVICE SUBSCRIBE is SUCCESSFULL");

      FOUND=0;
      /*VDEST*/
      if(YES_ADEST==0)
        {
          for(i=0;i<gl_vdest_indx;i++)
            {
              for(j=0;j<gl_tet_vdest[i].svc_count;j++)
                {
                  if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                     gl_tet_vdest[i].mds_pwe1_hdl==mds_hdl)
                    {
                      gl_tet_vdest[i].svc[j].subscr.num_svcs=num_svcs;
                      gl_tet_vdest[i].svc[j].subscr.scope=scope;
                      gl_tet_vdest[i].svc[j].subscr.svc_ids=svc_ids;
                      /*gl_tet_vdest[i].svc[j].subscr.evt_map=*/
                      gl_tet_vdest[i].svc[j].subscr_count+=num_svcs;
                      for(k=0;k<num_svcs;k++)
                        {
                          gl_tet_vdest[i].svc[j].svcevt[k].svc_id=
                            gl_tet_vdest[i].svc[j].subscr.svc_ids[k];
                          gl_tet_vdest[i].svc[j].svcevt[k].ur_svc_id=svc_id;
                        }
                      FOUND=1;
                      break;
                    }
                  if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                     gl_tet_vdest[i].mds_pwe1_hdl!=mds_hdl)
                    {
                      for(l=0;l<gl_tet_vdest[i].pwe_count;l++)
                        {
                          if(gl_tet_vdest[i].pwe[l].mds_pwe_hdl==mds_hdl)
                            {
                              gl_tet_vdest[i].svc[j].subscr.num_svcs=num_svcs;
                              gl_tet_vdest[i].svc[j].subscr.scope=scope;
                              gl_tet_vdest[i].svc[j].subscr.svc_ids=svc_ids;
                              /*gl_tet_vdest[i].svc[j].subscr.evt_map=*/
                              gl_tet_vdest[i].svc[j].subscr_count+=num_svcs;
                              for(k=0;k<num_svcs;k++)
                                {
                                  gl_tet_vdest[i].svc[j].svcevt[k].svc_id=
                                    gl_tet_vdest[i].svc[j].subscr.svc_ids[k];
                                  gl_tet_vdest[i].svc[j].svcevt[k].ur_svc_id=
                                    svc_id;
                                }
                              FOUND=1;
                              break;
                            }  
                        }
                    }
              
                }  
              if(FOUND)
                break;
            }
        }
      /*ADEST*/
      if(YES_ADEST==1)
        {
          for(i=0;i<gl_tet_adest.svc_count;i++)
            if((gl_tet_adest.svc[i].svc_id==svc_id)&&
               (gl_tet_adest.mds_pwe1_hdl==mds_hdl))
              {
                gl_tet_adest.svc[i].subscr.num_svcs=num_svcs;
                gl_tet_adest.svc[i].subscr.scope=scope;
                gl_tet_adest.svc[i].subscr.svc_ids=svc_ids;
                gl_tet_adest.svc[i].subscr_count+=num_svcs;
                for(k=0;k<num_svcs;k++)
                  {
                    gl_tet_adest.svc[i].svcevt[k].svc_id=
                      gl_tet_adest.svc[i].subscr.svc_ids[k];
                    gl_tet_adest.svc[i].svcevt[k].ur_svc_id=svc_id;
                  }
                break;
              }
          if((gl_tet_adest.svc[i].svc_id==svc_id)&&
             (gl_tet_adest.mds_pwe1_hdl!=mds_hdl))
            {
              for(l=0;l<gl_tet_adest.pwe_count;l++)
                {
                  if(gl_tet_adest.pwe[l].mds_pwe_hdl==mds_hdl)
                    {
                      gl_tet_adest.svc[i].subscr.num_svcs=num_svcs;
                      gl_tet_adest.svc[i].subscr.scope=scope;
                      gl_tet_adest.svc[i].subscr.svc_ids=svc_ids;
                      gl_tet_adest.svc[i].subscr_count+=num_svcs;
                      for(k=0;k<num_svcs;k++)
                        {
                          gl_tet_adest.svc[i].svcevt[k].svc_id=
                            gl_tet_adest.svc[i].subscr.svc_ids[k];
                          gl_tet_adest.svc[i].svcevt[k].ur_svc_id=svc_id;
                        }
                      break;
                    } 
                }
            }
        }
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS SUBSCRIBE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t mds_service_redundant_subscribe(MDS_HDL mds_hdl,
                                      MDS_SVC_ID svc_id,
                                      NCSMDS_SCOPE_TYPE scope,
                                      uint8_t num_svcs,
                                      MDS_SVC_ID *svc_ids)
{
  int i,j,k,l,FOUND;
  uint32_t YES_ADEST;
  /*Find whether this Service is on Adest or Vdest*/
  YES_ADEST=is_service_on_adest(mds_hdl,svc_id);

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_RED_SUBSCRIBE;    

  svc_to_mds_info.info.red_subscribe.i_scope=scope;
  svc_to_mds_info.info.red_subscribe.i_num_svcs=num_svcs;
  svc_to_mds_info.info.red_subscribe.i_svc_ids=svc_ids;    

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS RED SUBSCRIBE is SUCCESSFULL");

      FOUND=0;
      /*VDEST*/
      if(YES_ADEST==0)
        {
          for(i=0;i<gl_vdest_indx;i++)
            {
              for(j=0;j<gl_tet_vdest[i].svc_count;j++)
                {
                  if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                     gl_tet_vdest[i].mds_pwe1_hdl==mds_hdl)
                    {
                      gl_tet_vdest[i].svc[j].subscr.num_svcs=num_svcs;
                      gl_tet_vdest[i].svc[j].subscr.scope=scope;
                      gl_tet_vdest[i].svc[j].subscr.svc_ids=svc_ids;
                      /*gl_tet_vdest[i].svc[j].subscr.evt_map=*/
                      gl_tet_vdest[i].svc[j].subscr_count+=num_svcs;
                      for(k=0;k<num_svcs;k++)
                        {
                          gl_tet_vdest[i].svc[j].svcevt[k].svc_id=
                            gl_tet_vdest[i].svc[j].subscr.svc_ids[k];
                          gl_tet_vdest[i].svc[j].svcevt[k].ur_svc_id=svc_id;
                        }
                      FOUND=1;
                      break;
                    }
                  if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                     gl_tet_vdest[i].mds_pwe1_hdl!=mds_hdl) 
                    {
                      for(l=0;l<gl_tet_vdest[i].pwe_count;l++)
                        {
                          if(gl_tet_vdest[i].pwe[l].mds_pwe_hdl==mds_hdl)
                            {
                              gl_tet_vdest[i].svc[j].subscr.num_svcs=num_svcs;
                              gl_tet_vdest[i].svc[j].subscr.scope=scope;
                              gl_tet_vdest[i].svc[j].subscr.svc_ids=svc_ids;
                              /*gl_tet_vdest[i].svc[j].subscr.evt_map=*/
                              gl_tet_vdest[i].svc[j].subscr_count+=num_svcs;
                              for(k=0;k<num_svcs;k++)
                                {
                                  gl_tet_vdest[i].svc[j].svcevt[k].svc_id=
                                    gl_tet_vdest[i].svc[j].subscr.svc_ids[k];
                                  gl_tet_vdest[i].svc[j].svcevt[k].ur_svc_id=
                                    svc_id;
                                }
                              FOUND=1;
                              break;
                            }  
                        }
                    }
              
                }  
              if(FOUND)
                break;
            }
        }
      /*ADEST*/
      if(YES_ADEST==1)
        {
          for(i=0;i<gl_tet_adest.svc_count;i++)
            if((gl_tet_adest.svc[i].svc_id==svc_id)&&
               (gl_tet_adest.mds_pwe1_hdl==mds_hdl))
              {
                gl_tet_adest.svc[i].subscr.num_svcs=num_svcs;
                gl_tet_adest.svc[i].subscr.scope=scope;
                gl_tet_adest.svc[i].subscr.svc_ids=svc_ids;
                gl_tet_adest.svc[i].subscr_count+=num_svcs;
                for(k=0;k<num_svcs;k++)
                  {
                    gl_tet_adest.svc[i].svcevt[k].svc_id=
                      gl_tet_adest.svc[i].subscr.svc_ids[k];
                    gl_tet_adest.svc[i].svcevt[k].ur_svc_id=svc_id;
                  }
                break;
              }
          if((gl_tet_adest.svc[i].svc_id==svc_id)&&(gl_tet_adest.mds_pwe1_hdl!=
                                                    mds_hdl))
            {
              for(l=0;l<gl_tet_adest.pwe_count;l++)
                {
                  if(gl_tet_adest.pwe[l].mds_pwe_hdl==mds_hdl)
                    {
                      gl_tet_adest.svc[i].subscr.num_svcs=num_svcs;
                      gl_tet_adest.svc[i].subscr.scope=scope;
                      gl_tet_adest.svc[i].subscr.svc_ids=svc_ids;
                      gl_tet_adest.svc[i].subscr_count+=num_svcs;
                      for(k=0;k<num_svcs;k++)
                        {
                          gl_tet_adest.svc[i].svcevt[k].svc_id=
                            gl_tet_adest.svc[i].subscr.svc_ids[k];
                          gl_tet_adest.svc[i].svcevt[k].ur_svc_id=svc_id;
                        }
                      break;
                    } 
                }
            }
        }
      
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS RED SUBSCRIBE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t mds_service_cancel_subscription(MDS_HDL mds_hdl,
                                      MDS_SVC_ID svc_id,
                                      uint8_t num_svcs,
                                      MDS_SVC_ID *svc_ids)
{
  int i,j,k,FOUND;

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_CANCEL;    

  svc_to_mds_info.info.svc_cancel.i_num_svcs=num_svcs;
  svc_to_mds_info.info.svc_cancel.i_svc_ids=svc_ids;

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS CANCEL SUBSCRIBE is SUCCESSFULL");

      FOUND=0;
      for(i=0;i<gl_vdest_indx;i++) 
        {
          for(j=0;j<gl_tet_vdest[i].svc_count;j++)
            {
              if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                 gl_tet_vdest[i].mds_pwe1_hdl==mds_hdl)
                {
                  gl_tet_vdest[i].svc[j].subscr_count-=num_svcs;
                  FOUND=1;
                  break;
                }
              if(gl_tet_vdest[i].svc[j].svc_id==svc_id && 
                 gl_tet_vdest[i].mds_pwe1_hdl!=mds_hdl)
                {
                  for(k=0;k<gl_tet_vdest[i].pwe_count;k++)
                    {
                      if(gl_tet_vdest[i].pwe[k].mds_pwe_hdl==mds_hdl)
                        {
                          gl_tet_vdest[i].svc[j].subscr_count-=num_svcs;
                          FOUND=1;
                          break;
                        } 
                    }  
                }        
            }  
          if(FOUND)
            break;
        }
      if(!FOUND)
        {
          for(i=0;i<gl_tet_adest.svc_count;i++)
            if(gl_tet_adest.svc[i].svc_id==svc_id)
              {
                gl_tet_adest.svc[i].subscr_count-=num_svcs;
                break;
              }
        }     
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS CANCEL SUBSCRIBE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}


/*******************************  SEND WRAPPERS   ***************************/


uint32_t mds_just_send(MDS_HDL mds_hdl,
                    MDS_SVC_ID svc_id,
                    MDS_SVC_ID to_svc,
                    MDS_DEST to_dest,
                    MDS_SEND_PRIORITY_TYPE priority,
                    TET_MDS_MSG *message)
{
  uint32_t rs;
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=message;
  svc_to_mds_info.info.svc_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_send.i_priority=priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_SND;
  
  svc_to_mds_info.info.svc_send.info.snd.i_to_dest=to_dest;
  rs = ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\nMDS SEND is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nMDS SEND  has failed as there is no active instance\n");
          return MDS_RC_MSG_NO_BUFFERING;
        }
      else
        {
          printf("\nRequest to ncsmds_api: MDS SEND ACK has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
  
#if 0 
  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS JUST SEND is SUCCESSFULL\t");
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS JUST SEND has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
#endif
}
uint32_t mds_send_get_ack(MDS_HDL mds_hdl,
                       MDS_SVC_ID svc_id,
                       MDS_SVC_ID to_svc,
                       MDS_DEST to_dest,
                       uint32_t time_to_wait,
                       MDS_SEND_PRIORITY_TYPE priority,
                       TET_MDS_MSG *message)
{
  uint32_t rs;

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=message;
  svc_to_mds_info.info.svc_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_send.i_priority=priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_SNDACK;
  
  svc_to_mds_info.info.svc_send.info.sndack.i_to_dest=to_dest;
  svc_to_mds_info.info.svc_send.info.sndack.i_time_to_wait=time_to_wait;
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\nMDS SEND ACK is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else
    {  
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nMDS SEND ACK has TIMED OUT\n");
          return NCSCC_RC_REQ_TIMOUT;
        }
      else if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nMDS SEND ACK has failed as there is no active instance\n");
          return MDS_RC_MSG_NO_BUFFERING;
        }
      else
        {
          printf("\nRequest to ncsmds_api: MDS SEND ACK has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}
uint32_t mds_send_get_response(MDS_HDL mds_hdl,
                            MDS_SVC_ID svc_id,
                            MDS_SVC_ID to_svc,
                            MDS_DEST to_dest,
                            uint32_t time_to_wait,
                            MDS_SEND_PRIORITY_TYPE priority,
                            TET_MDS_MSG *message)
{
  uint32_t rs;
  TET_MDS_MSG *rsp;

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=message;
  svc_to_mds_info.info.svc_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_send.i_priority=priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_SNDRSP;
  
  svc_to_mds_info.info.svc_send.info.sndrsp.i_to_dest=to_dest;
  svc_to_mds_info.info.svc_send.info.sndrsp.i_time_to_wait=time_to_wait;
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS SEND RESPONSE is SUCCESSFULL\n");
      if(svc_to_mds_info.info.svc_send.info.sndrsp.o_rsp==NULL)
        {
          printf("Failed to receive the response from the receiver");
        }
      else
        {
          rsp=svc_to_mds_info.info.svc_send.info.sndrsp.o_rsp;
          printf("The response got from the receiver is : \n message length \
= %d \n message = %s",rsp->recvd_len,rsp->recvd_data);
        }
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest to ncsmds_api: MDS SEND RESPONSE has TIMED OUT\n");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nRequest to ncsmds_api: MDS SEND RESPONSE has no active instance\n");
          return MDS_RC_MSG_NO_BUFFERING;

        }
      else
        {
          printf("\nRequest to ncsmds_api: MDS SEND RESPONSE has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}

uint32_t mds_send_get_redack(MDS_HDL mds_hdl,
                          MDS_SVC_ID svc_id,
                          MDS_SVC_ID to_svc,
                          MDS_DEST to_vdest,
                          V_DEST_QA to_anc,
                          uint32_t time_to_wait,
                          MDS_SEND_PRIORITY_TYPE priority,
                          TET_MDS_MSG *message)
{
  uint32_t rs;
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    
 
  svc_to_mds_info.info.svc_send.i_msg=message;
  svc_to_mds_info.info.svc_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_send.i_priority=priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_REDACK;
  
  svc_to_mds_info.info.svc_send.info.redack.i_to_vdest=to_vdest;
  svc_to_mds_info.info.svc_send.info.redack.i_to_anc=to_anc;
  svc_to_mds_info.info.svc_send.info.redack.i_time_to_wait=time_to_wait;
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS SEND REDUNDANT ACK is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nReq to ncsmds_api:MDS SEND REDUNDANT ACK has TIMEDOUT\n");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else
        {
          printf("\nReqest to ncsmds_api:MDS SEND REDUNDANT ACK has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}



uint32_t mds_broadcast_to_svc(MDS_HDL mds_hdl,
                           MDS_SVC_ID svc_id,
                           MDS_SVC_ID to_svc,
                           NCSMDS_SCOPE_TYPE bcast_scope,
                           MDS_SEND_PRIORITY_TYPE priority,
                           TET_MDS_MSG *message)
{

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=message;
  svc_to_mds_info.info.svc_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_send.i_priority=priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_BCAST;
  
  svc_to_mds_info.info.svc_send.info.bcast.i_bcast_scope=bcast_scope;

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS SEND BROADCAST is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }  
  else 
    {
      printf("\nRequest to ncsmds_api: MDS SEND BROADCAST has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t mds_send_response(MDS_HDL mds_hdl,
                        MDS_SVC_ID svc_id,
                        TET_MDS_MSG *response)
{
  uint32_t rs;
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=response;
  svc_to_mds_info.info.svc_send.i_to_svc=gl_rcvdmsginfo.fr_svc_id;
  svc_to_mds_info.info.svc_send.i_priority=gl_rcvdmsginfo.priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_RSP;
  
  svc_to_mds_info.info.svc_send.info.rsp.i_sender_dest=gl_rcvdmsginfo.fr_dest;
  svc_to_mds_info.info.svc_send.info.rsp.i_msg_ctxt=gl_rcvdmsginfo.msg_ctxt;
  
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS RESPONSE is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE has TIMEDOUT\n");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE has failed as there is no active response\n");
          return MDS_RC_MSG_NO_BUFFERING;
        }

      else
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}
uint32_t mds_sendrsp_getack(MDS_HDL mds_hdl,
                         MDS_SVC_ID svc_id,
                         uint32_t time_to_wait,
                         TET_MDS_MSG *response)
{
  uint32_t rs;
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=response;
  svc_to_mds_info.info.svc_send.i_to_svc=gl_rcvdmsginfo.fr_svc_id;
  svc_to_mds_info.info.svc_send.i_priority=gl_rcvdmsginfo.priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_SNDRACK;
  
  svc_to_mds_info.info.svc_send.info.sndrack.i_sender_dest=
    gl_rcvdmsginfo.fr_dest;
  svc_to_mds_info.info.svc_send.info.sndrack.i_msg_ctxt=
    gl_rcvdmsginfo.msg_ctxt;
  svc_to_mds_info.info.svc_send.info.sndrack.i_time_to_wait=time_to_wait;
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS RESPONSE ACKNOWLEDGE is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE ACK has TIMEDOUT\n");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE ACK has no buffering\n");
          return MDS_RC_MSG_NO_BUFFERING;
        }
      else
        {
          printf("\nRequest to ncsmds_api: MDS RESPONSE ACK has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}

uint32_t mds_send_redrsp_getack(MDS_HDL mds_hdl,
                             MDS_SVC_ID svc_id,
                             uint32_t time_to_wait,
                             TET_MDS_MSG *response)
{
  uint32_t rs;
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SEND;    

  svc_to_mds_info.info.svc_send.i_msg=response;
  svc_to_mds_info.info.svc_send.i_to_svc=gl_rcvdmsginfo.fr_svc_id;
  svc_to_mds_info.info.svc_send.i_priority=gl_rcvdmsginfo.priority;
  svc_to_mds_info.info.svc_send.i_sendtype=MDS_SENDTYPE_REDRACK;
  
  svc_to_mds_info.info.svc_send.info.redrack.i_to_vdest=gl_rcvdmsginfo.fr_dest;
  svc_to_mds_info.info.svc_send.info.redrack.i_to_anc=gl_rcvdmsginfo.fr_anc;
  svc_to_mds_info.info.svc_send.info.redrack.i_msg_ctxt=
    gl_rcvdmsginfo.msg_ctxt;
  
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS REDUNDANT RESPONSE ACK is SUCCESSFULL\n");
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest : MDS REDUNDANT RESPONSE ACK has TIMEDOUT");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else
        {
          
          printf("\nRequest : MDS REDUNDANT RESPONSE ACK has FAILED");
          return NCSCC_RC_FAILURE;
        }
    }
}

/***************************  DIRECT SEND WRAPPERS *******************/

uint32_t mds_direct_send_message(MDS_HDL mds_hdl,
                              MDS_SVC_ID svc_id,
                              MDS_SVC_ID to_svc,
                              MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver,
                              MDS_SENDTYPES sendtype,
                              MDS_DEST to_dest,
                              uint32_t time_to_wait,
                              MDS_SEND_PRIORITY_TYPE priority,
                              char *message)
{
  uint32_t rs;
  uint16_t direct_buff_len=0;
  if(message)
    {
      /*Allocating memory for the direct buffer*/
      direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(message)+1);
      if(direct_buff==NULL)
        perror("Direct Buffer not allocated properly");
      else
        memset(direct_buff, 0, sizeof(direct_buff));
    }
  if(direct_buff&&message)
    {
      memcpy(direct_buff,(uint8_t *)message,strlen(message)+1);
      direct_buff_len=strlen(message)+1;
    }
  MDS_DIRECT_BUFF rsp;
  uint16_t rsp_len=0;
  
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_DIRECT_SEND;    
  
  svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
  svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=direct_buff_len;
  svc_to_mds_info.info.svc_direct_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=msg_fmt_ver;
  svc_to_mds_info.info.svc_direct_send.i_priority=priority;
  svc_to_mds_info.info.svc_direct_send.i_sendtype=sendtype;
  switch(sendtype)
    {
    case MDS_SENDTYPE_SND:
      svc_to_mds_info.info.svc_direct_send.info.snd.i_to_dest=to_dest;
      break;
    case MDS_SENDTYPE_SNDACK:
      svc_to_mds_info.info.svc_direct_send.info.sndack.i_to_dest=to_dest;
      svc_to_mds_info.info.svc_direct_send.info.sndack.i_time_to_wait=
        time_to_wait;
      break;
    case MDS_SENDTYPE_SNDRSP:
      svc_to_mds_info.info.svc_direct_send.info.sndrsp.i_to_dest=to_dest;
      svc_to_mds_info.info.svc_direct_send.info.sndrsp.i_time_to_wait=
        time_to_wait;
      break;
    default:

      break;
    }
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api: MDS DIRECT SEND is SUCCESSFULL");
      if(sendtype==MDS_SENDTYPE_SNDRSP)
        {   
          if(svc_to_mds_info.info.svc_direct_send.info.sndrsp.buff==NULL)
            printf("Failed to receive the response from the receiver");
          else
            {
              rsp=svc_to_mds_info.info.svc_direct_send.info.sndrsp.buff;
              rsp_len=svc_to_mds_info.info.svc_direct_send.info.sndrsp.len;
              printf("\nThe response got from the receiver is :  message\
 length = %d \t message = %s",rsp_len,rsp);
              fflush(stdout);
              /*Now Free the Response Message*/
              m_MDS_FREE_DIRECT_BUFF(svc_to_mds_info.info.svc_direct_send.info.sndrsp.buff);
            }
        } 
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest to ncsmds_api: MDS DIRECT SEND has TIMED OUT\n");
          /*Free the above allocated Direct Buff*/
          m_MDS_FREE_DIRECT_BUFF(direct_buff);
          direct_buff=NULL;
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else if(rs==MDS_RC_MSG_NO_BUFFERING)
        {
          printf("\nRequest to ncsmds_api: MDS DIRECT SEND has failed as no active instance\n");
          /*Free the above allocated Direct Buff*/
          return MDS_RC_MSG_NO_BUFFERING;
        }

      else
        {
          printf("\nRequest to ncsmds_api: MDS DIRECT SEND has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}

uint32_t mds_direct_response(MDS_HDL mds_hdl,
                          MDS_SVC_ID svc_id,
                          MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver,
                          MDS_SENDTYPES sendtype,
                          uint32_t time_to_wait)
{
  uint32_t rs;
  char msg[]="Resp Message";
  uint16_t direct_buff_len;
  /*Before Sending the Message: Allocate the Direct Buffer*/
  direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(msg)+1);
  memset(direct_buff, 0, sizeof(direct_buff));
  if(direct_buff==NULL)
    perror("Direct Buffer not allocated properly");

  memcpy(direct_buff,(uint8_t *)msg,sizeof(msg));
  direct_buff_len=sizeof(msg);
  
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_DIRECT_SEND;    
  
  svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
  svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=direct_buff_len;
  svc_to_mds_info.info.svc_direct_send.i_to_svc=gl_direct_rcvmsginfo.fr_svc_id;
  svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=msg_fmt_ver;
  svc_to_mds_info.info.svc_direct_send.i_priority=
    gl_direct_rcvmsginfo.priority;
  svc_to_mds_info.info.svc_direct_send.i_sendtype=sendtype;
  switch(sendtype)
    {
    case MDS_SENDTYPE_RSP:
      svc_to_mds_info.info.svc_direct_send.info.rsp.i_sender_dest=
        gl_direct_rcvmsginfo.fr_dest;
      svc_to_mds_info.info.svc_direct_send.info.rsp.i_msg_ctxt=
        gl_direct_rcvmsginfo.msg_ctxt;
      break;
    case MDS_SENDTYPE_RRSP:
      svc_to_mds_info.info.svc_direct_send.info.rrsp.i_to_dest=
        gl_direct_rcvmsginfo.fr_dest;
      svc_to_mds_info.info.svc_direct_send.info.rrsp.i_to_anc=
        gl_direct_rcvmsginfo.fr_anc;
      svc_to_mds_info.info.svc_direct_send.info.rrsp.i_msg_ctxt=
        gl_direct_rcvmsginfo.msg_ctxt;
      break;
    case MDS_SENDTYPE_SNDRACK:
      svc_to_mds_info.info.svc_direct_send.info.sndrack.i_sender_dest=
        gl_direct_rcvmsginfo.fr_dest;
      svc_to_mds_info.info.svc_direct_send.info.sndrack.i_msg_ctxt=
        gl_direct_rcvmsginfo.msg_ctxt;
      svc_to_mds_info.info.svc_direct_send.info.sndrack.i_time_to_wait=
        time_to_wait;
      break;
    case MDS_SENDTYPE_REDRACK:
      svc_to_mds_info.info.svc_direct_send.info.redrack.i_to_vdest=
        gl_direct_rcvmsginfo.fr_dest;
      svc_to_mds_info.info.svc_direct_send.info.redrack.i_to_anc=
        gl_direct_rcvmsginfo.fr_anc;
      svc_to_mds_info.info.svc_direct_send.info.redrack.i_msg_ctxt=
        gl_direct_rcvmsginfo.msg_ctxt;
      svc_to_mds_info.info.svc_direct_send.info.redrack.i_time_to_wait=
        time_to_wait;
      break;
    default:
      break;
    }
  rs=ncsmds_api(&svc_to_mds_info);
  if(rs==NCSCC_RC_SUCCESS)  
    {
      printf("\nRequest to ncsmds_api: MDS DIRECT RESPONSE is SUCCESSFULL");
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      if(rs==NCSCC_RC_REQ_TIMOUT)
        {
          printf("\nRequest to ncsmds_api:MDS DIRECT RESPONSE has TIMEDOUT\n");
          return NCSCC_RC_REQ_TIMOUT;      
        }
      else
        {
          printf("\nRequest to ncsmds_api: MDS DIRECT RESPONSE has FAILED\n");
          return NCSCC_RC_FAILURE;
        }
    }
}
uint32_t mds_direct_broadcast_message(MDS_HDL mds_hdl,
                                   MDS_SVC_ID svc_id,
                                   MDS_SVC_ID to_svc,
                                   MDS_CLIENT_MSG_FORMAT_VER msg_fmt_ver,
                                   MDS_SENDTYPES sendtype,
                                   NCSMDS_SCOPE_TYPE bscope,
                                   MDS_SEND_PRIORITY_TYPE priority)
{
  char msg[]="Direct Message";
  uint16_t direct_buff_len;
 /* if(msg)*/
    {
      /*Allocating memory for the direct buffer*/
      direct_buff=m_MDS_ALLOC_DIRECT_BUFF(strlen(msg)+1);
      memset(direct_buff, 0, sizeof(direct_buff));
      if(direct_buff==NULL)
        perror("Direct Buffer not allocated properly");
    }

  if(direct_buff)
    {
      memcpy(direct_buff,(uint8_t *)msg,sizeof(msg));
      direct_buff_len=sizeof(msg);
    }
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_DIRECT_SEND;    
  
  svc_to_mds_info.info.svc_direct_send.i_direct_buff=direct_buff;
  svc_to_mds_info.info.svc_direct_send.i_direct_buff_len=direct_buff_len;
  svc_to_mds_info.info.svc_direct_send.i_to_svc=to_svc;
  svc_to_mds_info.info.svc_direct_send.i_msg_fmt_ver=msg_fmt_ver;
  svc_to_mds_info.info.svc_direct_send.i_priority=priority;
  svc_to_mds_info.info.svc_direct_send.i_sendtype=sendtype;
  switch(sendtype)
    {
    case MDS_SENDTYPE_BCAST:
      svc_to_mds_info.info.svc_direct_send.info.bcast.i_bcast_scope=bscope;
      break;
    case MDS_SENDTYPE_RBCAST:
      svc_to_mds_info.info.svc_direct_send.info.rbcast.i_bcast_scope=bscope;
      break;
    default:
      break;
    }
  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api: MDS DIRECT BROADCAST is SUCCESSFULL");
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS DIRECT BROADCAST has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}



#if 0
int is_sel_obj_found(int indx)
{
  uint32_t count=0;
  /* int count=0;*/
  NCS_SEL_OBJ numfds;
  NCS_SEL_OBJ_SET readfd;
  struct timeval tv;
  
  numfds.raise_obj=0;
  numfds.rmv_obj=0;
  tv.tv_sec = 100;
  tv.tv_usec = 10;
  
  m_NCS_SEL_OBJ_ZERO(&readfd);
  m_NCS_SEL_OBJ_SET(gl_tet_svc[indx].sel_obj,&readfd);
  numfds=m_GET_HIGHER_SEL_OBJ(gl_tet_svc[indx].sel_obj,numfds);
  count=m_NCS_SEL_OBJ_SELECT(numfds,&readfd,NULL,NULL,0);/*infinite time*/
  switch(count)
    {
    case 0: printf("                       TIMED OUT                  \n");
      break;
    case -1:printf("......................couldn't select..............\n");
      break;
    }
  if( m_NCS_SEL_OBJ_ISSET(gl_tet_svc[indx].sel_obj,&readfd) )
    return gl_tet_svc[indx].svc_id;
  else
    return 0; 
}
#endif
int is_vdest_sel_obj_found(int vi,int si)
{
  struct pollfd sel;
  unsigned count;
  sel.fd = m_GET_FD_FROM_SEL_OBJ(gl_tet_vdest[vi].svc[si].sel_obj);
  sel.events = POLLIN;
  count = osaf_poll(&sel, 1, -1);
  switch (count) {
  case 0:
    printf("                       TIMED OUT                  \n");
    break;
  case 1:
    if ((sel.revents & POLLIN) != 0) return gl_tet_vdest[vi].svc[si].svc_id;
    break;
  }
  return 0;
}
int is_adest_sel_obj_found(int si)
{
  struct pollfd sel;
  unsigned count;
  sel.fd = m_GET_FD_FROM_SEL_OBJ(gl_tet_adest.svc[si].sel_obj);
  sel.events = POLLIN;
  count = osaf_poll(&sel, 1, -1);
  switch (count) {
  case 0:
    printf("                       TIMED OUT                  \n");
    break;
  case 1:
    if ((sel.revents & POLLIN) != 0) return gl_tet_adest.svc[si].svc_id;
    break;
  }
  return 0;
}
uint32_t tet_create_task(NCS_OS_CB task_startup,
                      NCSCONTEXT t_handle)
{
  char taskname[]="My Task";
  if ( m_NCS_TASK_CREATE(task_startup,
                         NULL,
                         taskname,
                         NCS_TASK_PRIORITY_6,
                         NCS_STACKSIZE_MEDIUM,
                         &t_handle)==NCSCC_RC_SUCCESS )
    return NCSCC_RC_SUCCESS;
  else
    return NCSCC_RC_FAILURE;
}
uint32_t mds_service_retrieve(MDS_HDL mds_hdl,
                           MDS_SVC_ID svc_id,
                           SaDispatchFlagsT dispatchFlags)
{

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_RETRIEVE;    

  svc_to_mds_info.info.retrieve_msg.i_dispatchFlags=dispatchFlags;
  switch(dispatchFlags){
  case SA_DISPATCH_BLOCKING:printf("\tBlocking Retrieve");break;
  case SA_DISPATCH_ONE:break;
  case SA_DISPATCH_ALL:break;
  }

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n MDS RETRIEVE is SUCCESSFULL");

      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      printf("\nRequest to ncsmds_api: MDS RETRIEVE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t mds_query_vdest_for_role(MDS_HDL mds_hdl,
                               MDS_SVC_ID svc_id,
                               MDS_DEST dest,
                               MDS_SVC_ID query_svc_id,
                               V_DEST_QA anc)
{

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_QUERY_DEST;    

  svc_to_mds_info.info.query_dest.i_dest=dest;
  svc_to_mds_info.info.query_dest.i_svc_id=query_svc_id;
  svc_to_mds_info.info.query_dest.i_query_for_role= true;
  svc_to_mds_info.info.query_dest.info.query_for_role.i_anc=anc;

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api:MDS QUERY DEST for ROLE is SUCCESSFULL");
      /*store*/
      /*=svc_to_mds_info.info.query_dest.o_adest*/
      /*=svc_to_mds_info.info.query_dest.o_vdest_rl*/
      if(svc_to_mds_info.info.query_dest.o_local)
        printf("\n\tLOCAL service on Node = %x with Role = %d and its ADEST = \
<%llx>\n ", svc_to_mds_info.info.query_dest.o_node_id,
               svc_to_mds_info.info.query_dest.info.query_for_role.o_vdest_rl,
               svc_to_mds_info.info.query_dest.o_adest);
      else
        printf("\n\tNON-LOCAL service on Node = %x with Role = %d and its \
ADEST = <%llx>\n",svc_to_mds_info.info.query_dest.o_node_id,
               svc_to_mds_info.info.query_dest.info.query_for_role.o_vdest_rl,
               svc_to_mds_info.info.query_dest.o_adest);
      return  NCSCC_RC_SUCCESS; 
    }
  else 
    {
      printf("\nRequest to ncsmds_api: MDS QUERY DEST has FAILED\n");
      return NCSCC_RC_FAILURE;
    }    

  
}
uint32_t mds_query_vdest_for_anchor(MDS_HDL mds_hdl,
                                 MDS_SVC_ID svc_id,
                                 MDS_DEST dest,
                                 MDS_SVC_ID query_svc_id,
                                 V_DEST_RL vdest_rl)
{

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_QUERY_DEST;    

  svc_to_mds_info.info.query_dest.i_dest=dest;
  svc_to_mds_info.info.query_dest.i_svc_id=query_svc_id;
  svc_to_mds_info.info.query_dest.i_query_for_role=false;
  svc_to_mds_info.info.query_dest.info.query_for_anc.i_vdest_rl=vdest_rl;
  

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api: MDS QUERY VDEST for ANCHOR is \
SUCCESSFULL");
      /*store*/
      /*=svc_to_mds_info.info.query_dest.o_adest*/
      /*=svc_to_mds_info.info.query_dest.o_anc*/
      
      if(svc_to_mds_info.info.query_dest.o_local)
        printf("\n\tLOCAL service on Node = %x with Anchor = <%llx> and its\
 ADEST = <%llx>\n ", svc_to_mds_info.info.query_dest.o_node_id,
               svc_to_mds_info.info.query_dest.info.query_for_anc.o_anc,
               svc_to_mds_info.info.query_dest.o_adest);
      else
        printf("\n\tNON-LOCAL service on Node = %x with Anchor = <%llx> and \
its ADEST = <%llx>\n",svc_to_mds_info.info.query_dest.o_node_id,
               svc_to_mds_info.info.query_dest.info.query_for_anc.o_anc,
               svc_to_mds_info.info.query_dest.o_adest);
      return  NCSCC_RC_SUCCESS; 
    }
  else 
    {
      printf("\nRequst to ncsmds_api:MDS QUERY VDEST for ANCHOR has FAILED\n");
      return NCSCC_RC_FAILURE;
    }    
  
}

uint32_t is_service_on_adest(MDS_HDL mds_hdl,
                          MDS_SVC_ID svc_id)
{
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_QUERY_PWE;    

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      return svc_to_mds_info.info.query_pwe.o_absolute;
    }
  else
    return NCSCC_RC_FAILURE;
}
uint32_t mds_service_query_for_pwe(MDS_HDL mds_hdl,
                                MDS_SVC_ID svc_id)
{

  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_op=MDS_QUERY_PWE;    

  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api: MDS QUERY PWE is SUCCESSFULL");
      /*store*/
      if(svc_to_mds_info.info.query_pwe.o_absolute)
        {
          /*=svc_to_mds_info.info.query_pwe.info.abs_info.o_adest;*/
          printf("\nThe Service %d lives on ADEST = <%llx> of PWE id = %d\n",
                 svc_id,svc_to_mds_info.info.query_pwe.info.abs_info.o_adest,
                 svc_to_mds_info.info.query_pwe.o_pwe_id);
        }
      else
        {
          /*=svc_to_mds_info.info.query_pwe.virt_info.o_vdest;*/
          /*=svc_to_mds_info.info.query_pwe.virt_info.o_anc;*/
          /*=svc_to_mds_info.info.query_pwe.virt_info.o_role;*/
          printf("\nThe Service %d lives on VDEST id = <%llx> with Anchor =\
 <%llx> and Role = %d on PWE id = %d\n",svc_id,
                 svc_to_mds_info.info.query_pwe.info.virt_info.o_vdest,
                 svc_to_mds_info.info.query_pwe.info.virt_info.o_anc,
                 svc_to_mds_info.info.query_pwe.info.virt_info.o_role,
                 svc_to_mds_info.info.query_pwe.o_pwe_id);
        }
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      printf("\nRequest to ncsmds_api: MDS QUERY PWE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }    
}

/*******************CALL BACK ROUTINES*********************************/

/* call back routine's definition */
uint32_t tet_mds_cb_cpy(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  TET_MDS_MSG *out_msg,*in_msg;
  
  /*printf(" Inside callback copy\n"); usually the senders callback copy will\
    be called*/
  printf("\nThe service which is sending the message is = %d",
         mds_to_svc_info->i_yr_svc_id);
  out_msg = (TET_MDS_MSG *)malloc(sizeof(TET_MDS_MSG)); /*where are u clean it\
                                                          up?*/
  memset(out_msg, 0, sizeof(TET_MDS_MSG));
  in_msg =  (TET_MDS_MSG *)mds_to_svc_info->info.cpy.i_msg;
  out_msg->recvd_len = in_msg->send_len;
  /*out_msg->recvd_data = (char *)malloc(out_msg->recvd_len);*/
  strcpy(out_msg->recvd_data,in_msg->send_data);
  mds_to_svc_info->info.cpy.o_msg_fmt_ver=mds_to_svc_info->info.cpy.i_rem_svc_pvt_ver;
  mds_to_svc_info->info.cpy.o_cpy = out_msg;
  printf("\nThe service to which the message needs to be delivered = %d",
         mds_to_svc_info->info.cpy.i_to_svc_id);
  /*printf("\nThe message received is %s \t %d\n",out_msg->recvd_data,
    out_msg->recvd_len);*/
  /*would you like to check the status of i_last
    if(mds_to_svc_info->info.cpy.i_last)
    free(i_msg);
  */
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mds_cb_enc(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  uint8_t *p8;
  TET_MDS_MSG *msg;
  
  msg = (TET_MDS_MSG*)mds_to_svc_info->info.enc.i_msg;
  mds_to_svc_info->info.enc.o_msg_fmt_ver=mds_to_svc_info->info.enc.i_rem_svc_pvt_ver;        
  printf("Encoding the message sent Sender svc = %d with msg fmt ver =%d\n",
         mds_to_svc_info->i_yr_svc_id,mds_to_svc_info->info.enc.o_msg_fmt_ver);
  
  /* ENCODE length */
  p8 = ncs_enc_reserve_space(mds_to_svc_info->info.enc.io_uba, 2);
  ncs_encode_16bit(&p8, msg->send_len);
  ncs_enc_claim_space(mds_to_svc_info->info.enc.io_uba, 2);
  
  /* ENCODE data */
  ncs_encode_n_octets_in_uba(mds_to_svc_info->info.enc.io_uba, 
                             (uint8_t*)msg->send_data, msg->send_len);
  
  printf("Successfully encoded message for Receiver svc = %d\n",
         mds_to_svc_info->info.enc.i_to_svc_id);
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mds_cb_dec(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  uint8_t *p8;
  TET_MDS_MSG *msg;
  
  printf("Decoding the message from Sender svc = %d with msg fmt ver = %d on Node =%x\n",
         mds_to_svc_info->info.dec.i_fr_svc_id,
         mds_to_svc_info->info.dec.i_msg_fmt_ver,
         mds_to_svc_info->info.dec.i_node_id);
  if (mds_to_svc_info->info.dec.o_msg != NULL)
    {
      /* We are receiving a synchronous reply! */
      msg = mds_to_svc_info->info.dec.o_msg;
    }
  else
    {
      /* We are receiving an asynchronous message! */
      msg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
      memset(msg, 0, sizeof(TET_MDS_MSG));
      mds_to_svc_info->info.dec.o_msg = (uint8_t*)msg;
    }
  
  /* DECODE length */
  p8 = ncs_dec_flatten_space(mds_to_svc_info->info.dec.io_uba, 
                             (uint8_t*)&msg->recvd_len, 2);
  msg->recvd_len = ncs_decode_16bit(&p8);
  ncs_dec_skip_space(mds_to_svc_info->info.dec.io_uba, 2);
  
  /*Decode data*/
  /*msg->recvd_data = (char *) malloc(msg->recvd_len+1);*/
  memset(msg->recvd_data, 0, sizeof(msg->recvd_len+1));
  ncs_decode_n_octets_from_uba(mds_to_svc_info->info.dec.io_uba, 
                               (uint8_t*)msg->recvd_data, msg->recvd_len);
  msg->recvd_data[msg->recvd_len] = 0; /* NULL termination for string */
  if(mds_to_svc_info->info.dec.i_is_resp)
    printf("This is a Response");
  else
    printf("This is Not a Response");
  return NCSCC_RC_SUCCESS;
}

uint32_t tet_mds_cb_enc_flat (NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  uint8_t *p8;
  TET_MDS_MSG *msg;
  
  msg = (TET_MDS_MSG*)mds_to_svc_info->info.enc_flat.i_msg;
  mds_to_svc_info->info.enc_flat.o_msg_fmt_ver=mds_to_svc_info->info.enc_flat.i_rem_svc_pvt_ver;
  printf("Flat encoding the message sent from Sender svc = %d with msg_fmt_ver=%d\n",
         mds_to_svc_info->i_yr_svc_id,mds_to_svc_info->info.enc_flat.o_msg_fmt_ver);

  /* ENCODE length */
  p8 = ncs_enc_reserve_space(mds_to_svc_info->info.enc_flat.io_uba, 2);
  ncs_encode_16bit(&p8, msg->send_len);
  ncs_enc_claim_space(mds_to_svc_info->info.enc_flat.io_uba, 2);
  
  /* ENCODE data */
  ncs_encode_n_octets_in_uba(mds_to_svc_info->info.enc_flat.io_uba, 
                             (uint8_t*)msg->send_data, msg->send_len);
  /*
    printf("Successfully flat encoded message for Receiver svc = %d\n\n",
    mds_to_svc_info->info.enc_flat.i_to_svc_id);*/
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mds_cb_dec_flat (NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  uint8_t *p8;
  TET_MDS_MSG *msg;
  
  printf("\nFlat Decoding the message from Sender svc = %d with msg fmt ver=%d" ,
         mds_to_svc_info->info.dec_flat.i_fr_svc_id,
         mds_to_svc_info->info.dec_flat.i_msg_fmt_ver);
  if (mds_to_svc_info->info.dec_flat.o_msg != NULL)
    {
      /* We are receiving a synchronous reply! */
      msg = mds_to_svc_info->info.dec_flat.o_msg;
    }
  else
    {
      /* We are receiving an asynchronous message! */
      msg = (TET_MDS_MSG*)malloc(sizeof(TET_MDS_MSG));
      memset(msg, 0, sizeof(TET_MDS_MSG));
      mds_to_svc_info->info.dec_flat.o_msg = (uint8_t*)msg;
    }
  
  /* DECODE length */
  p8 = ncs_dec_flatten_space(mds_to_svc_info->info.dec_flat.io_uba, 
                             (uint8_t*)&msg->recvd_len, 2);
  msg->recvd_len = ncs_decode_16bit(&p8);
  ncs_dec_skip_space(mds_to_svc_info->info.dec_flat.io_uba, 2);
  
  /*Decode data*/
  /*msg->recvd_data = (char *) malloc(msg->recvd_len+1);*/
  memset(msg->recvd_data, 0, sizeof(msg->recvd_len+1));
  ncs_decode_n_octets_from_uba(mds_to_svc_info->info.dec_flat.io_uba, 
                               (uint8_t*)msg->recvd_data, msg->recvd_len);
  msg->recvd_data[msg->recvd_len] = 0; /* NULL termination for string */
  if(mds_to_svc_info->info.dec_flat.i_is_resp)
    printf("\nThis is a RESPONSE");
  else
    printf("\nThis is a MESSAGE");
  
  return NCSCC_RC_SUCCESS;
  
}
uint32_t tet_mds_cb_rcv(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  /*as usal this is preceded by one of cb_cpy or cb_dec or cb_dec_flat*/
  TET_MDS_MSG *msg;
  
  msg=mds_to_svc_info->info.receive.i_msg;
  
  /* printf(" The Sender service is = %d \n",
     mds_to_svc_info->info.receive.i_fr_svc_id);*/
  printf("\nThe Sender service = %d is on destination =<%llx> with anchor =\
 <%llx> Node %x and msg fmt ver = %d\n" ,mds_to_svc_info->info.receive.i_fr_svc_id,
         mds_to_svc_info->info.receive.i_fr_dest,
         mds_to_svc_info->info.receive.i_fr_anc,
         mds_to_svc_info->info.receive.i_node_id,
         mds_to_svc_info->info.receive.i_msg_fmt_ver);
  /*
    printf("\nThe Receiver service is = %d ",
    mds_to_svc_info->info.receive.i_to_svc_id);
  */
  printf("The Receiver service = %d is on destination =<%llx> \n",
         mds_to_svc_info->info.receive.i_to_svc_id,
         mds_to_svc_info->info.receive.i_to_dest);
  
  /*Now Display what message did u receive*/
  printf("\nReceived Message len = %d \nThe message is=%s",
         msg->recvd_len,msg->recvd_data);
  
  /*Now storing the message context on a global structure*/
  gl_rcvdmsginfo.yr_svc_hdl=mds_to_svc_info->i_yr_svc_hdl; /*the decider*/
  
  gl_rcvdmsginfo.msg=mds_to_svc_info->info.receive.i_msg;
  gl_rcvdmsginfo.rsp_reqd=mds_to_svc_info->info.receive.i_rsp_reqd;
  if(gl_rcvdmsginfo.rsp_reqd)
    {
      gl_rcvdmsginfo.msg_ctxt=mds_to_svc_info->info.receive.i_msg_ctxt;
    }
  gl_rcvdmsginfo.fr_svc_id=mds_to_svc_info->info.receive.i_fr_svc_id;
  gl_rcvdmsginfo.fr_dest=mds_to_svc_info->info.receive.i_fr_dest;
  gl_rcvdmsginfo.fr_anc=mds_to_svc_info->info.receive.i_fr_anc;
  gl_rcvdmsginfo.to_svc_id=mds_to_svc_info->info.receive.i_to_svc_id;
  gl_rcvdmsginfo.msg_fmt_ver=mds_to_svc_info->info.receive.i_msg_fmt_ver;
  gl_rcvdmsginfo.priority=mds_to_svc_info->info.receive.i_priority;
  gl_rcvdmsginfo.to_dest=mds_to_svc_info->info.receive.i_to_dest;/*FIX THIS*/
  gl_rcvdmsginfo.node_id=mds_to_svc_info->info.receive.i_node_id;
  
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mds_cb_direct_rcv(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  /*when the message is not encoded, no need for decode. this is called in 
    case of a direct send*/
  /* this call is not preceded by cb_dec or cb_dec_flat:what about cb_cpy*/
#if 0
  MDS_DIRECT_BUFF msg;
  uint16_t recvd_len;
  msg=mds_to_svc_info->info.direct_receive.i_direct_buff;
  recvd_len=mds_to_svc_info->info.direct_receive.i_direct_buff_len;
#endif
  printf("\n\t\tDirect Receive callback\n");
  printf(" The Sender service is = %d is on <%llx> destination with anchor =\
 <%llx> on Node = %x with msg fmt ver=%d\n" ,mds_to_svc_info->info.direct_receive.i_fr_svc_id,
         mds_to_svc_info->info.direct_receive.i_fr_dest,
         mds_to_svc_info->info.direct_receive.i_fr_anc,
         mds_to_svc_info->info.direct_receive.i_node_id,
         mds_to_svc_info->info.direct_receive.i_msg_fmt_ver);
  
  printf("\nThe Receiver service is = %d is on <%llx> destination",
         mds_to_svc_info->info.direct_receive.i_to_svc_id,
         mds_to_svc_info->info.direct_receive.i_to_dest);
  
  /*Now Display what message did u receive*/
  printf("\n\n\tReceived Message len = %d and the message is=%s\n",mds_to_svc_info->info.direct_receive.i_direct_buff_len,
         mds_to_svc_info->info.direct_receive.i_direct_buff);
  fflush(stdout);
  /*Now storing the message context on a global structure*/
  gl_direct_rcvmsginfo.yr_svc_hdl=mds_to_svc_info->i_yr_svc_hdl;/*the decider*/
  
  gl_direct_rcvmsginfo.buff=mds_to_svc_info->info.direct_receive.i_direct_buff;
  gl_direct_rcvmsginfo.len=
    mds_to_svc_info->info.direct_receive.i_direct_buff_len;
  gl_direct_rcvmsginfo.fr_svc_id=
    mds_to_svc_info->info.direct_receive.i_fr_svc_id;
  gl_direct_rcvmsginfo.fr_dest=
    mds_to_svc_info->info.direct_receive.i_fr_dest;
  gl_direct_rcvmsginfo.fr_anc=mds_to_svc_info->info.direct_receive.i_fr_anc;
  gl_direct_rcvmsginfo.to_svc_id=
    mds_to_svc_info->info.direct_receive.i_to_svc_id;
  gl_direct_rcvmsginfo.to_dest=mds_to_svc_info->info.direct_receive.i_to_dest;
  gl_direct_rcvmsginfo.priority=
    mds_to_svc_info->info.direct_receive.i_priority;
  gl_direct_rcvmsginfo.node_id=mds_to_svc_info->info.direct_receive.i_node_id;
  gl_direct_rcvmsginfo.rsp_reqd=
    mds_to_svc_info->info.direct_receive.i_rsp_reqd;
  if(mds_to_svc_info->info.direct_receive.i_rsp_reqd)
    {
      gl_direct_rcvmsginfo.msg_ctxt=
        mds_to_svc_info->info.direct_receive.i_msg_ctxt;
    }
  mds_free_direct_buff(mds_to_svc_info->info.direct_receive.i_direct_buff);
  return NCSCC_RC_SUCCESS;
}

uint32_t tet_mds_svc_event(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  int i,j,k;
  gl_event_data.ur_svc_id=mds_to_svc_info->info.svc_evt.i_your_id;
  
  gl_event_data.event  = mds_to_svc_info->info.svc_evt.i_change; 
  gl_event_data.dest   = mds_to_svc_info->info.svc_evt.i_dest;   
  gl_event_data.anc    = mds_to_svc_info->info.svc_evt.i_anc;    
  gl_event_data.role   = mds_to_svc_info->info.svc_evt.i_role;   
  gl_event_data.node_id= mds_to_svc_info->info.svc_evt.i_node_id;
  gl_event_data.pwe_id = mds_to_svc_info->info.svc_evt.i_pwe_id; 
  gl_event_data.svc_id = mds_to_svc_info->info.svc_evt.i_svc_id; 
  gl_event_data.rem_svc_pvt_ver =  mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver;
  gl_event_data.svc_pwe_hdl = mds_to_svc_info->info.svc_evt.svc_pwe_hdl;
  if(is_service_on_adest(gl_event_data.svc_pwe_hdl,gl_event_data.ur_svc_id)== NCSCC_RC_SUCCESS)
    {
      printf("\nThe Subscriber Service id = %d is on ADEST",
             mds_to_svc_info->info.svc_evt.i_your_id);
      for(i=0;i<gl_tet_adest.svc_count;i++)
        {
          /*find to which service this EVENT came*/
          if(gl_tet_adest.svc[i].svc_id==gl_event_data.ur_svc_id)
            {
              for(j=0;j<gl_tet_adest.svc[i].subscr_count;j++)
                {
                  if(gl_tet_adest.svc[i].svcevt[j].svc_id==gl_event_data.svc_id)
                    gl_tet_adest.svc[i].svcevt[j]=gl_event_data;
                }
            }
        }
    }
  else
    {
      printf("\nThe Subscriber Service id = %d is on VDEST",
             mds_to_svc_info->info.svc_evt.i_your_id);
      for(i=0;i<gl_vdest_indx;i++)
        {
          for(j=0;j<gl_tet_vdest[i].svc_count;j++)
            {
              if(gl_tet_vdest[i].svc[j].svc_id==gl_event_data.ur_svc_id)
                {
                  for(k=0;k<gl_tet_vdest[i].svc[j].subscr_count;k++)
                    {
                      if(gl_tet_vdest[i].svc[j].svcevt[k].svc_id==gl_event_data.svc_id)
                        gl_tet_vdest[i].svc[j].svcevt[k]=gl_event_data;
                    }
                }
          
            }
        }
    }
  /*fill in the event info of this service*/
  /* If this service is installed with MDS queue ownership model hen use
     MDS_RETREIVE */
  
  if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_UP)
    {
      printf("\nUP: Subscribed Svc = %d with svc pvt ver = %d is UP on dest= <%llx> anchor=\
 <%llx> role= %d with PWE id = %d on node = %x \n",
             mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest,
             mds_to_svc_info->info.svc_evt.i_anc,
             mds_to_svc_info->info.svc_evt.i_role,
             mds_to_svc_info->info.svc_evt.i_pwe_id,
             mds_to_svc_info->info.svc_evt.i_node_id);

      fflush(stdout);
      /*would you like to send a message to that service */
      /*set a FLAG*/
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_DOWN)
    {
      printf("\nDOWN: Subscribed Svc = %d with svc pvt ver = %d is DOWN on dest= <%llx> anchor=\
 <%llx> role= %d with PWE id = %d on node = %x \n", 
             mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest,
             mds_to_svc_info->info.svc_evt.i_anc,
             mds_to_svc_info->info.svc_evt.i_role,
             mds_to_svc_info->info.svc_evt.i_pwe_id,
             mds_to_svc_info->info.svc_evt.i_node_id);
      fflush(stdout);
      /*dont send any messages to this service*/   
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_NO_ACTIVE)
    {
      printf("\nNO ACTIVE: Received NO ACTIVE Event\n");
      printf(" In the system no active instance of Subscribed srv= %d with svc pvt ver = %d on dest=\
 <%llx> found \n",mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest); 
      /*any messages send to this service will be stored internally*/
      fflush(stdout);
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_NEW_ACTIVE)
    {
      printf("\nNEW ACTIVE: Received NEW_ACTIVE Event\n");
      printf(" In the system atleast one active instance of Subscribed service\
 = %d with svc pvt ver = %d  on destinatin = <%llx> found \n",mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest); 
      fflush(stdout);
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_RED_UP)
    {
      printf("\nRED UP: Received RED_UP Event\n");
      printf(" The Subscribed Service = %d with svc pvt ver = %d is RED UP on destination = <%llx> \
 anchor = <%llx> role = %d with PWE id = %d on node = %x \n ", 
             mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest,
             mds_to_svc_info->info.svc_evt.i_anc,
             mds_to_svc_info->info.svc_evt.i_role,
             mds_to_svc_info->info.svc_evt.i_pwe_id,
             mds_to_svc_info->info.svc_evt.i_node_id);
      fflush(stdout);
      /*check out the role of the destination*/    
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_RED_DOWN)
    {
      printf("\nRED DOWN: Received RED_DOWN Event\n");
      printf(" The Subscribed Service = %d with svc pvt ver = %d is RED DOWN on destination = <%llx>\
  anchor = <%llx> role = %d with PWE id = %d on node = %x \n ", 
             mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,
             mds_to_svc_info->info.svc_evt.i_dest,
             mds_to_svc_info->info.svc_evt.i_anc,
             mds_to_svc_info->info.svc_evt.i_role,
             mds_to_svc_info->info.svc_evt.i_pwe_id,
             mds_to_svc_info->info.svc_evt.i_node_id);
      fflush(stdout);
      /*check out the role of the destination*/    
    }
  else if (mds_to_svc_info->info.svc_evt.i_change == NCSMDS_CHG_ROLE)
    {
      printf("\nCHG ROLE: Received CHG_ROLE Event\n");
      printf(" The Subscribed service = %d with svc pvt ver = %d on destinatin = <%llx> is changing\
                          role\n ", mds_to_svc_info->info.svc_evt.i_svc_id,
             mds_to_svc_info->info.svc_evt.i_rem_svc_pvt_ver,    
             mds_to_svc_info->info.svc_evt.i_dest);
      fflush(stdout);
    }
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mds_sys_event(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  printf("The Subscriber Service id = %d\n",
         mds_to_svc_info->info.svc_evt.i_your_id);
  if (mds_to_svc_info->info.sys_evt.i_change == NCSMDS_UP)
    {
      printf(" NODE UP: the node = %x is UP \n",
             mds_to_svc_info->info.sys_evt.i_node_id);
    }
  else if (mds_to_svc_info->info.sys_evt.i_change == NCSMDS_DOWN)
    {
      printf(" NODE DOWN: the node = %x is DOWN \n",
             mds_to_svc_info->info.sys_evt.i_node_id);
    }
  /*catch the Up/Down events of the Nodes*/
  return NCSCC_RC_SUCCESS;
}

uint32_t tet_mds_cb_quiesced_ack(NCSMDS_CALLBACK_INFO *mds_to_svc_info)
{
  /*just to ackowledge the user that role got changed to quiesced*/
  printf("The Callback for Quiesced Ack got acknowledgement\n");
  fflush(stdout);
  return NCSCC_RC_SUCCESS;
}
#if 0
uint32_t mds_service_system_subscribe(MDS_HDL mds_hdl,MDS_SVC_ID svc_id,
                                   EVT_FLTR evt_map)
{
  /*request*/
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_SYS_SUBSCRIBE;    
  /*inputs*/
  svc_to_mds_info.info.svc_sys_subscribe.i_evt_map=evt_map;
  /*api call*/
  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nRequest to ncsmds_api: MDS SYSTEM SUBSCRIBE is SUCCESSFULL");
      /*store*/
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsmds_api: MDS SYSTEM SUBSCRIBE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}

uint32_t change_role(MDS_HDL mds_hdl,MDS_SVC_ID svc_id,V_DEST_RL new_role)
{
  /*request*/
  svc_to_mds_info.i_mds_hdl=mds_hdl;
  svc_to_mds_info.i_svc_id=svc_id;
  svc_to_mds_info.i_op=MDS_CHG_ROLE;
  /*inputs*/
  svc_to_mds_info.info.chg_role.new_role=new_role;
  /*api call*/
  if(ncsmds_api(&svc_to_mds_info)==NCSCC_RC_SUCCESS)
    {
      printf("\nSvc = %d Role = %d MDS CHANGE ROLE is SUCCESSFULL",svc_id,
             new_role);
      return NCSCC_RC_SUCCESS;
    }
  else 
    {
      printf("\nRequest to ncsmds_api: MDS CHANGE ROLE has FAILED\n");
      return NCSCC_RC_FAILURE;
    }    
}
static uint32_t sys_names[] = {0,1};
#endif
uint32_t   tet_sync_point()
{
#if 0
  if (tet_remsync(0, sys_names, 4 ,30,
                  TET_SV_NO,NULL) != 0)

    {
      perror("Tet Sync Failed");
      return NCSCC_RC_FAILURE;
    }
  sync_id++;

  fill_syncparameters(TET_SV_YES);
#endif
  return NCSCC_RC_SUCCESS;
}

