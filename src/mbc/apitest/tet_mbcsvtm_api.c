#include "mds_papi.h"
#include "ncs_mda_papi.h"
#include"mbcsv_api.h"
#include"tet_api.h"
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#define KEY (1492)
/*Declarations and Global variables*/
uint32_t tet_mbcsv_config();
NCSVDA_INFO    vda_info;
uint32_t tet_mbcsv_dest_close(void);
uint32_t create_vdest(MDS_DEST vdest);
uint32_t destroy_vdest(MDS_DEST vdest);
uint32_t tet_mbcsv_dest_start(void);
uint32_t initsemaphore();
uint32_t V_operation();
uint32_t P_operation();
int fill_syncparameters(int vote);
/*---------------  TEST CASES --------------------------------*/
void tet_mbcsv_op(int choice)
{
  int FAIL=0;
  NCS_MBCSV_ARG mbcsv_arg;
  memset(&mbcsv_arg,'\0',sizeof(NCS_MBCSV_ARG));
  printf("\n------- tet_mbcsv_op: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_initialize");
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  switch(choice)
    {
    case 1:
      tet_printf("Not able to Succeed:Supply Invalid OP code to MBCSv ");
      mbcsv_arg.i_op=12; /*0 to 11 are valid*/
      if(ncs_mbcsv_svc(&mbcsv_arg)==NCSCC_RC_SUCCESS)
        FAIL=1;
      else
        printf("\nInvalid OP code to MBCSv\n");
      fflush(stdout);
      break;
    case 2:
      tet_printf("Not able to Succeed:Supply NULL callback to INITIALIZE ");
      mbcsv_arg.i_op=0;
      if(ncs_mbcsv_svc(&mbcsv_arg)==NCSCC_RC_SUCCESS)
        FAIL=1;
      else
        printf("\nNULL callback to INITIALIZE\n");
      fflush(stdout);
      break;
    case 3:
      tet_printf("Not able to Succeed:Supply invalid version number");
      mbcsv_arg.i_op = NCS_MBCSV_OP_INITIALIZE;
      mbcsv_arg.info.initialize.i_service  = 2000;
      mbcsv_arg.info.initialize.i_mbcsv_cb = mbcstm_svc_cb;
      mbcsv_arg.info.initialize.i_version  = 0;
      if(ncs_mbcsv_svc(&mbcsv_arg)==NCSCC_RC_SUCCESS)
        FAIL=1;
      else
        printf("\nVersion 0 to INITIALIZE\n");
      fflush(stdout);
      break;
    }
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)  
    {
      if(FAIL)
        tet_result(TET_FAIL);
      else
        tet_result(TET_PASS);
    }   
  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout);
}
void tet_mbcsv_initialize(int choice)
{
  int FAIL=0;
  NCS_MBCSV_HDL temp_hdl;
  printf("\n------- tet_mbcsv_initialize: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_initialize");
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  switch(choice)
    {
    case 1:
      tet_printf("Initialize and Finalize an MBCSV instance");
      if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      
      if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;   
      break;
    case 2:
      tet_printf("Not able to Finalize an already Finalized MBCSV instance");
      if(mbcstm_svc_finalize(1)==NCSCC_RC_SUCCESS)
        FAIL=1;      
      break;
    case 3:
      tet_printf("Not able to Register already registered service");
      mbcstm_svc_registration(1);      
      if(mbcstm_svc_registration(1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_svc_finalize(1);   
      break;
    case 4:
      tet_printf("Not able to Register with Service id=0");
      mbcstm_cb.svces[1].svc_id=0;
      if(mbcstm_svc_registration(1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
      /*    case 5:
            tet_printf("Not able to Register with Service id > 32767");
            mbcstm_cb.svces[1].svc_id=32768;
            if(mbcstm_svc_registration(1)==NCSCC_RC_SUCCESS)
            FAIL=1;
            mbcstm_svc_finalize(1);
            break;
      */
    case 6:
      tet_printf("Dispatch One");
      mbcstm_cb.svces[1].disp_flags =SA_DISPATCH_ONE;
      mbcstm_cb.svces[1].task_flag=false;
      if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_svc_dispatch(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;  
      break;
    case 7:
      tet_printf("Dispatch All");
      mbcstm_cb.svces[1].disp_flags =SA_DISPATCH_ALL;
      mbcstm_cb.svces[1].task_flag=false;
      if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_svc_dispatch(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;  
      break;
    case 8:
      tet_printf("Not able to Dispatch One with NULL Mbcsv Handle");
      mbcstm_cb.svces[1].disp_flags =SA_DISPATCH_ONE;
      mbcstm_cb.svces[1].task_flag=false;
      if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      temp_hdl=mbcstm_cb.svces[1].mbcsv_hdl;
      mbcstm_cb.svces[1].mbcsv_hdl=(NCS_MBCSV_HDL)(long)NULL;
      if(mbcstm_svc_dispatch(1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_cb.svces[1].mbcsv_hdl=temp_hdl;
      if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
        FAIL=1;  
      break;
      /*    case 9:
            tet_printf("Not able to Dispatch ALL with NULL Mbcsv Handle");
            mbcstm_cb.svces[1].disp_flags =SA_DISPATCH_ALL;
            mbcstm_cb.svces[1].task_flag=false;
            if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
            FAIL=1;
            if(mbcstm_svc_dispatch(1)!=NCSCC_RC_SUCCESS)
            FAIL=1;
            if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
            FAIL=1;  
            break;
      */
    }
  
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)  
    {
      if(FAIL)
        tet_result(TET_FAIL);
      else
        tet_result(TET_PASS);
    }   
  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout);
}
void tet_mbcsv_open_close(int choice)
{
  int FAIL=0;
  NCS_MBCSV_HDL temp_hdl;
  printf("\n----------- tet_mbcsv_open_close: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_open_close");
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
 
  switch(choice)
    {
    case 1:
      tet_printf("Open a session and Close it");
      if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
    case 2:
      tet_printf("Not able to close an already closed session");   
      if(mbcstm_ssn_close(1,1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
    case 3:
      tet_printf("Not able to open an already Opened session");
      if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_ssn_open(1,1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
    case 4:
      tet_printf("Not able to open with Invalid pwe handle handle");
      mbcstm_cb.svces[1].ssns[1].pwe_hdl  = 0;
      if(mbcstm_ssn_open(1,1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
    case 5:
      tet_printf("Not able to open with Invalid MBCSV Handle");
      temp_hdl=mbcstm_cb.svces[1].mbcsv_hdl;
      mbcstm_cb.svces[1].mbcsv_hdl=(NCS_MBCSV_HDL)(long)NULL;
      if(mbcstm_ssn_open(1,1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_cb.svces[1].mbcsv_hdl=temp_hdl;
      break;
    case 6:
      tet_printf("Not able to close with Invalid MBCSV Handle");
      temp_hdl=mbcstm_cb.svces[1].mbcsv_hdl;
      mbcstm_cb.svces[1].mbcsv_hdl=(NCS_MBCSV_HDL)(long)NULL;
      if(mbcstm_ssn_close(1,1)==NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_cb.svces[1].mbcsv_hdl=temp_hdl;
      break;
    }
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)  
    {
      if(FAIL)
        tet_result(TET_FAIL);
      else
        tet_result(TET_PASS);
    }   
  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout);
}
void tet_mbcsv_change_role(int choice)
{
  int FAIL=0;
  NCS_MBCSV_HDL temp_hdl;
  uint32_t temp_ckpt_hdl;

  printf("\n---------- tet_mbcsv_change_role: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_change_role");
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   

  switch(choice)
    {
    case 1:
      tet_printf("StandBy to Active");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 2:
      tet_printf("StandBy to Quiescing");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
    case 3:
      tet_printf("Active to Quiesced");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 4:
      tet_printf("Active to Quiescing");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 5:
      tet_printf("Quiesced to Active");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
      mbcstm_ssn_set_role(1,1);   
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 6:
      tet_printf("Quiesced to Quiescing");   
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
      mbcstm_ssn_set_role(1,1);   
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 7:
      tet_printf("Quiesced to StandBy");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
      mbcstm_ssn_set_role(1,1);   
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 8:
      tet_printf("Not able to change from Quiescing to StandBy");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
      if(mbcstm_ssn_set_role(1,1) == NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 9:
      tet_printf("Quiescing to Active");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
      mbcstm_ssn_set_role(1,1);
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    case 10:
      tet_printf("Not able to Change Role with Invalid MBCSv handle");
      temp_hdl=mbcstm_cb.svces[1].mbcsv_hdl;
      mbcstm_cb.svces[1].mbcsv_hdl=(NCS_MBCSV_HDL)(long)NULL;
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      if(mbcstm_ssn_set_role(1,1) == NCSCC_RC_SUCCESS)  
        FAIL=1;
      mbcstm_cb.svces[1].mbcsv_hdl=temp_hdl;
      break;
    case 11:
      tet_printf("Not able to Change Role with Invalid Checkpoint handle");
      temp_ckpt_hdl=mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
      mbcstm_cb.svces[1].ssns[1].ckpt_hdl=(NCS_MBCSV_HDL)(long)NULL;
      if(mbcstm_ssn_set_role(1,1) == NCSCC_RC_SUCCESS)  
        FAIL=1;
      mbcstm_cb.svces[1].ssns[1].ckpt_hdl=temp_ckpt_hdl;
      break;
    case 12:
      tet_printf("Not able to Change Role : Trying to set Same role");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
      if(mbcstm_ssn_set_role(1,1) == NCSCC_RC_SUCCESS)  
        FAIL=1;
      break;
    }
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)  
    {
      if(FAIL)
        tet_result(TET_FAIL);
      else
        tet_result(TET_PASS);
    }   
  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout);
}
/*--------------------- DUAL PROCESS --------------------------*/
void tet_mbcsv_Notify(int choice)
{
  int FAIL=0;

  uint32_t temp_ckpt_hdl;
  char str[]="Yes My Dear Peer";
  int len=strlen(str)+1;
  char BIG[1200];
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t mbcstm_svc_send_notify(uint32_t, uint32_t, NCS_MBCSV_NTFY_MSG_DEST, char *,
                               uint32_t );
    
  printf("\n----------- tet_mbcsv_Notify: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_Notify %d",choice);
  tet_mbcsv_config();


  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  mbcstm_sync_point();
  switch(choice)
    { 

    case 1:
      tet_printf("Send Notify message to STANDBY from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }
        }
      else
        {
          mbcstm_sync_point();
          fflush(stdout);
        }
      break;
    case 2:
      tet_printf("Send Notify message to ALL PEERS from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
        }
      else
        {
          mbcstm_sync_point();
          fflush(stdout);
        }
      break;
    case 3:
      tet_printf("Send Notify message to ACTIVE from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          fflush(stdout);
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to ACTIVE failed");
              FAIL=1;
            }
        }
      break;

    case 4:
      tet_printf("Send Notify message to ALL PEERS from QUIESCING");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 5:
      tet_printf("Send Notify message to ALL PEERs from QUIESCED");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
          mbcstm_sync_point();
        }
        
      break;


    case 6:
      tet_printf("Send Notify message to STANDBY from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
          mbcstm_sync_point();

        }
      break;
    case 7:
      tet_printf("BIG(1151 Bytes) Notify message to STANDBY from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          memset(BIG,'S',1200);
          BIG[1199]='\0';
          printf("\n BIG length = %ld\n",(long)strlen(BIG)); /*1100*/        
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,BIG,1150) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            } 
        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
        }
      break;
    case 8:
      tet_printf("NOT able to Send Notify message to ACTIVE from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 9:
      tet_printf("Send Notify message to ALL PEERS from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 10:
      tet_printf("Not able to Send Notify message with invalid Destination");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,3,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        sleep(1);
      break;
    case 11:
      tet_printf("Not able to Send Notify message with invalid Ckpt Handle");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          temp_ckpt_hdl=mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
          mbcstm_cb.svces[1].ssns[1].ckpt_hdl=(NCS_MBCSV_HDL)(long)NULL;
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          mbcstm_cb.svces[1].ssns[1].ckpt_hdl=temp_ckpt_hdl;
        }
      else
        sleep(1);
      break;

    case 12:
      tet_printf("Not able to Send Notify message to STANDBY from ACTIVE \
when No STANDBY peer available");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      break;
    case 13:
      tet_printf("Not able to Send Notify message to ACTIVE from STANDBY \
when No ACTIVE peer available");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Active failed");
              FAIL=1;
            }   
        }
      else
        printf("\n");
      break;
    case 14:
      tet_printf("Send Notify message to IDLE STANDBY Peer  from IDLE ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_PEER_INFO_FAIL;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }  
        }
      else
        {
          mbcstm_sync_point();
          sleep(1);
          fflush(stdout);
        }
      break;
    case 15:
      tet_printf("Not able to Send Notify message after closing the session");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
            FAIL=1;
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }
        }
      else
        {
          mbcstm_sync_point();
          fflush(stdout);
        }
      break;
    }  
  fflush(stdout);
  sleep(4);
  /*mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);*/
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  if((choice==15) && (mbcstm_cb.sys == MBCSTM_SVC_INS1))
    printf("\n");
  else
    if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
      FAIL=1;
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;

  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 
}




void tet_mbcsv_cold_sync(choice)
{
  int FAIL=0;
  /*
    MBCSTM_PEERS_DATA peers;
    int i=0;
  */
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);

  
  printf("\n----------- tet_mbcsv_cold_sync: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_cold_sync %d",choice);
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  mbcstm_sync_point();

  switch(choice)
    {
    case 1:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Cold Sync Process");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 2:
      tet_printf("All Standbys Fail to Encode Cold Sync Request");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          printf("\nFailing to Encode Cold Sync Request\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_COLD_ENCODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 3:
      tet_printf("All Standbys Fail to Decode Cold Sync Response");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          printf("\nFailing to Decode Cold Sync Request\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_COLD_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 4:
      tet_printf("Active Fail to Decode Cold Sync Request");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          printf("\nFailing to Decode Cold Sync Request\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_COLD_DECODE_FAIL;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 5:
      tet_printf("Active Fail to Encode Cold Sync Response");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          printf("\nFailing to Encode Cold Sync Response\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_COLD_ENCODE_FAIL;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
#if 0
      /*Duplicate of Case 4*/
    case 6:
      tet_printf("Active Cold Sync Timer Expiry");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          printf("\nExpiry of Cold Sync Timer\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_COLD_TIMER_EXP;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
      /*Duplicate of Case 5*/
    case 7:
      tet_printf("Active Cold Sync Complete Timer Expiry");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          printf("\nExpiry of Cold Sync Complete Timer\n");
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_ACTIVE_COLD_CMP_TIMER_EXP;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
#endif
    }  
  fflush(stdout); 
  sleep(4);
  mbcstm_sync_point();
#if 0
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);
  printf("\n\n \t My Peers Count = %d \n",peers.peer_count);
  for(i=1;i<=peers.peer_count;i++)
    {
      printf("\ni = %d Role : %c State = %s Anchor=%llx\n",
             i,peers.peers[i].peer_role, 
             peers.peers[i].state,peers.peers[i].peer_anchor);
    }
#endif
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 

}




void tet_mbcsv_warm_sync(int choice)
{
  int FAIL=0;
  /*
    MBCSTM_PEERS_DATA peers;
    int i=0;
  */
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);

  
  printf("\n----------- tet_mbcsv_warm_sync: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_warm_sync %d",choice);
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  mbcstm_sync_point();

  switch(choice)
    {
    case 1:
      tet_printf("Warm Sync Process");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 2:
      tet_printf("All Standbys Fail to Encode Wam Sync Request");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_ENCODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\nFailing to Encode Warm Sync Request\n");
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 3:
      tet_printf("All Standbys Fail to Decode Warm Sync Response");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\nFailing to Decode Warm Sync Responset\n");
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 4:
      tet_printf("Active Fail to Decode Warm Sync Request");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_WARM_DECODE_FAIL;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\nFailing to Decode Warm Sync Request\n");
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 5:
      tet_printf("Active Fail to Encode Warm Sync Response");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_WARM_ENCODE_FAIL;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\nFailing to Encode Warm Sync Response\n");
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
#if 0
      /*Duplicate of Case 4*/
    case 6:
      tet_printf("Active Warm Sync Timer Expired");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_ACTIVE_WARM_TIMER_EXP;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\n Warm Sync Timer Expired\n");
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
#endif
    case 7:
      tet_printf("Active Warm Sync Complete Timer Expired");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_ACTIVE_WARM_CMP_TIMER_EXP;
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          printf("\n Warm Sync Complete Timer Expired\n");
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(60);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    }  
  fflush(stdout); 
  sleep(4);
  mbcstm_sync_point();
#if 0
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);
  printf("\n\n \t My Peers Count = %d \n",peers.peer_count);
  for(i=1;i<=peers.peer_count;i++)
    {
      printf("\ni = %d Role : %c State = %s Anchor=%llx\n",
             i,peers.peers[i].peer_role, 
             peers.peers[i].state,peers.peers[i].peer_anchor);
    }
#endif
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 

}

void tet_get_set_warm_sync(int choice)
{
  int FAIL=0;
  NCS_MBCSV_ARG     mbcsv_arg;
  /*
    MBCSTM_PEERS_DATA peers;
    int i=0;
  */
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);

  
  printf("\n-----------tet_get_set_warm_sync: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_get_set_warm_sync %d",choice);
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;

  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(choice==19)
    mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_PEER_INFO_FAIL;
  mbcstm_sync_point();

  switch(choice)
    {
    case 1:
      tet_printf("Warm Sync Process");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 2:
      tet_printf("Check Whether Is_Warm_Sync_ON is populated properly");
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      fflush(stdout);
      sleep(2);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      mbcsv_prt_inv(); 
      fflush(stdout);
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      sleep(2);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      fflush(stdout);
      break;
    case 3:
      tet_printf("Not able to  Enable already enabled Warm Sync Flag");
      mbcstm_cb.svces[1].ssns[1].ws_flag=1;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,
                        NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)!= NCSCC_RC_SUCCESS)
        FAIL=1;
      sleep(2);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      fflush(stdout);
      break;
    case 4:
      tet_printf("Setting the Warm Sync Timer within the Range");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC)
         != NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      mbcstm_cb.svces[1].ssns[1].ws_timer=360000;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC)
         != NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      break;
    case 5:
      tet_printf("Setting the Warm Sync Timer OUT of the Range");
      mbcstm_cb.svces[1].ssns[1].ws_timer=100;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC)
         == NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      mbcstm_cb.svces[1].ssns[1].ws_timer=400000;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC)
         == NCSCC_RC_SUCCESS)
        FAIL=1;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      break;
    case 6:
      tet_printf("Active Warm Sync Flag DISABLED");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].ws_flag=0;
          mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,
                         NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
          mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,
                         NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 7:
      tet_printf("One of the  STANDBY Warm Sync Flag DISABLED");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      else
        {
          if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
            {
              
              mbcstm_cb.svces[1].ssns[1].ws_flag=0;
              mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,
                             NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
              mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,
                             NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
            }
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 8:
      tet_printf("All STANDBYs  Warm Sync Flag DISABLED");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].ws_flag=0;
          mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,
                         NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
          mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,
                         NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 9:
      tet_printf("GET with invalid Checkpoint Handle");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_GET;
      mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
      mbcsv_arg.info.obj_get.i_obj=NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
      mbcsv_arg.info.obj_get.i_ckpt_hdl =(NCS_MBCSV_HDL)(long)NULL ; /*wrong valude*/
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 10:
      tet_printf("GET with invalid Object ID");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_GET;
      mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
      mbcsv_arg.info.obj_get.i_obj=6; /*wrong valude*/
      mbcsv_arg.info.obj_get.i_ckpt_hdl =mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 11:
      tet_printf("GET with invalid Mbcsv Handle");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_GET;
      mbcsv_arg.i_mbcsv_hdl = (NCS_MBCSV_HDL)(long)NULL;/*wrong valude*/
      mbcsv_arg.info.obj_get.i_obj=NCS_MBCSV_OBJ_TMR_WSYNC;
      mbcsv_arg.info.obj_get.i_ckpt_hdl =mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 12:
      tet_printf("SET with invalid Checkpoint Handle");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
      mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
      mbcsv_arg.info.obj_set.i_obj=NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF;
      mbcsv_arg.info.obj_set.i_val = 0;
      mbcsv_arg.info.obj_set.i_ckpt_hdl =(NCS_MBCSV_HDL)(long)NULL ; /*wrong valude*/
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 13:
      tet_printf("SET with invalid Object ID");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
      mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
      mbcsv_arg.info.obj_set.i_obj=6; /*wrong valude*/
      mbcsv_arg.info.obj_set.i_ckpt_hdl =mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 14:
      tet_printf("SET with invalid Mbcsv Handle");
      memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
      mbcsv_arg.i_op = NCS_MBCSV_OP_OBJ_SET;
      mbcsv_arg.i_mbcsv_hdl = (NCS_MBCSV_HDL)(long)NULL;/*wrong valude*/
      mbcsv_arg.info.obj_set.i_obj=NCS_MBCSV_OBJ_TMR_WSYNC;
      mbcsv_arg.info.obj_set.i_val = 1100;
      mbcsv_arg.info.obj_set.i_ckpt_hdl =mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
      if (NCSCC_RC_SUCCESS ==ncs_mbcsv_svc(&mbcsv_arg))
        FAIL=1;
      break;
    case 15:
      tet_printf("Try to set the same value for WARN SYNC ON flag again");
      mbcstm_cb.svces[1].ssns[1].ws_flag=mbcstm_cb.svces[1].ssns[1].ws_flag;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,
                        NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF)
         != NCSCC_RC_SUCCESS)
        FAIL=1;
      sleep(2);
      tet_printf("Try to set the same value for WARN SYNC TIMER again");
      mbcstm_cb.svces[1].ssns[1].ws_timer=6000;
      if(mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC)
         != NCSCC_RC_SUCCESS)
        FAIL=1;
      break;
    }  
  fflush(stdout); 
  sleep(4);
  mbcstm_sync_point();
#if 0
  mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);
  printf("\n\n \t My Peers Count = %d \n",peers.peer_count);
  for(i=1;i<=peers.peer_count;i++)
    {
      printf("\ni = %d Role : %c State = %s Anchor=%llx\n",
             i,peers.peers[i].peer_role, 
             peers.peers[i].state,peers.peers[i].peer_anchor);
    }
#endif
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 
}









void tet_mbcsv_data_request(int choice)
{
  int FAIL=0;
  int i=0,DATA_COUNT=10;
  NCS_MBCSV_ARG     mbcsv_arg;
  NCS_UBAID  *uba = NULL;
  uint8_t*      data;
  MBCSTM_SVC      *svc;
  MBCSTM_SSN      *ssn;
  char            fun_name[] = "mbcstm_svc_data_request";
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t mbcstm_create_data_point(uint32_t , uint32_t );
  uint32_t mbcstm_print_data_points(uint32_t,uint32_t);
  uint32_t mbcstm_destroy_data_point(uint32_t , uint32_t );
  
  printf("\n----------- tet_mbcsv_data_request: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_data_request %d",choice);
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  mbcstm_sync_point();

  switch(choice)
    {
    case 1:
      tet_printf("Data Req Process: Standby Requesting the Missing DATA");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
          for(i=1;i<=DATA_COUNT;i++)
            mbcstm_create_data_point(1,1);

          /*data count*/

          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);

          if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
            {
              mbcstm_cb.svces[1].ssns[1].cb_test=
                MBCSTM_CB_NO_TEST;
              mbcstm_sync_point();
              mbcstm_cb.svces[1].ssns[1].data_req=1;/*dont change*/
              mbcstm_cb.svces[1].ssns[1].data_req_count=DATA_COUNT;/*data count*/
              sleep(2);
              if(mbcstm_svc_data_request(1,1) != NCSCC_RC_SUCCESS)
                FAIL=1;
            }
          else
            mbcstm_sync_point();
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 2:
      tet_printf("Data Req Process: Quiescing Requesting the Missing DATA");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
          for(i=1;i<=DATA_COUNT;i++)
            mbcstm_create_data_point(1,1);

          /*data count*/

          mbcstm_sync_point();
        }
      else
        {
#if 0
          if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
            {
              mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
              if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
                {
                  perror("Standby to Active role change Failed");
                  FAIL=1;
                }
            }
#endif
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          
          if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
            {
              mbcstm_cb.svces[1].ssns[1].cb_test=
                MBCSTM_CB_NO_TEST;
              /*Changing to Quiescing*/
              mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
              if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
                {
                  perror("Standby to Active role change Failed");
                  FAIL=1;
                }
              sleep(2);
              mbcstm_sync_point();
              mbcstm_cb.svces[1].ssns[1].data_req=1;/*dont change*/
              mbcstm_cb.svces[1].ssns[1].data_req_count=DATA_COUNT;/*data count*/
              sleep(2);
              if(mbcstm_svc_data_request(1,1) != NCSCC_RC_SUCCESS)
                FAIL=1;
            }
          else
            mbcstm_sync_point();
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 3:
      tet_printf("Standby Requesting the Missing DATA with Invalid Mbcsv Hdl");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
          
          mbcstm_create_data_point(1,1);

          /*data count*/

          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);

          if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
            {
              mbcstm_cb.svces[1].ssns[1].cb_test=
                MBCSTM_CB_NO_TEST;
              mbcstm_sync_point();
              mbcstm_cb.svces[1].ssns[1].data_req=1;/*dont change*/
              mbcstm_cb.svces[1].ssns[1].data_req_count=1;/*data count*/
              sleep(2);
              /*
                if(mbcstm_svc_data_request(1,1) != NCSCC_RC_SUCCESS)
                FAIL=1;
              */
              /*Manually*/
              svc = &mbcstm_cb.svces[1];
              ssn = &svc->ssns[1];
              
              memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
              
              mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
              mbcsv_arg.i_mbcsv_hdl = (NCS_MBCSV_HDL)(long)NULL; /*Wrong Value*/
              
              uba = &mbcsv_arg.info.send_data_req.i_uba;
              if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba))
                {
                  mbcstm_print(__FILE__, fun_name, __LINE__,"UBA_INIT_SPACE", 
                               NCSCC_RC_FAILURE);
                  FAIL=1;
                }
              
              data = ncs_enc_reserve_space(uba, 2*sizeof(uint32_t));
              if(data == NULL)
                {
                  tet_printf("\n fake_encode_elem: DATA NULL");
                  FAIL=1;
                }
              ncs_encode_32bit(&data, ssn->data_req);
              ncs_encode_32bit(&data, ssn->data_req_count);
              ncs_enc_claim_space(uba, 2*sizeof(uint32_t));
              mbcsv_arg.info.send_data_req.i_ckpt_hdl = ssn->ckpt_hdl;
              if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
                {
                  mbcstm_print(__FILE__, fun_name, __LINE__,"DATA_REQUEST", 
                               NCSCC_RC_FAILURE);

                }
              else
                FAIL=1;              
              ssn->warm_flag = MBCSTM_DATA_REQUEST;         
              
            }
          else
            mbcstm_sync_point();
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;
    case 4:
      tet_printf("Standby Requesting the Missing DATA with Invalid Checkpoint Hdl");
      mbcstm_cb.svces[1].ssns[1].ws_timer=1000;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_TMR_WSYNC);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);
          mbcstm_sync_point();
          
          mbcstm_create_data_point(1,1);

          /*data count*/

          mbcstm_sync_point();
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].cb_test=
            MBCSTM_CB_STANDBY_WARM_DECODE_FAIL;
          sleep(2);
          mbcstm_sync_point();
          mbcsv_prt_inv();
          sleep(10);

          if(mbcstm_cb.sys == MBCSTM_SVC_INS2)
            {
              mbcstm_cb.svces[1].ssns[1].cb_test=
                MBCSTM_CB_NO_TEST;
              mbcstm_sync_point();
              mbcstm_cb.svces[1].ssns[1].data_req=1;/*dont change*/
              mbcstm_cb.svces[1].ssns[1].data_req_count=1;/*data count*/
              sleep(2);
              /*
                if(mbcstm_svc_data_request(1,1) != NCSCC_RC_SUCCESS)
                FAIL=1;
              */
              /*Manually*/
              svc = &mbcstm_cb.svces[1];
              ssn = &svc->ssns[1];
              
              memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
              
              mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
              mbcsv_arg.i_mbcsv_hdl = svc->mbcsv_hdl; 
              
              uba = &mbcsv_arg.info.send_data_req.i_uba;
              if (NCSCC_RC_SUCCESS != ncs_enc_init_space(uba))
                {
                  mbcstm_print(__FILE__, fun_name, __LINE__,"UBA_INIT_SPACE", 
                               NCSCC_RC_FAILURE);
                  FAIL=1;
                }
              
              data = ncs_enc_reserve_space(uba, 2*sizeof(uint32_t));
              if(data == NULL)
                {
                  tet_printf("\n fake_encode_elem: DATA NULL");
                  FAIL=1;
                }
              ncs_encode_32bit(&data, ssn->data_req);
              ncs_encode_32bit(&data, ssn->data_req_count);
              ncs_enc_claim_space(uba, 2*sizeof(uint32_t));
              mbcsv_arg.info.send_data_req.i_ckpt_hdl =(NCS_MBCSV_HDL)(long)NULL;/*Wrong Value*/
              if (NCSCC_RC_SUCCESS != ncs_mbcsv_svc(&mbcsv_arg))
                {
                  mbcstm_print(__FILE__, fun_name, __LINE__,"DATA_REQUEST", 
                               NCSCC_RC_FAILURE);
                 
                }
              else
                FAIL=1;
              
              ssn->warm_flag = MBCSTM_DATA_REQUEST;         
              
            }
          else
            mbcstm_sync_point();
          mbcstm_sync_point();
        }
      fflush(stdout);
      break;

    }  
  fflush(stdout); 
  sleep(4);
  mbcstm_sync_point();
  mbcstm_print_data_points(1,1);
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 
}










void tet_mbcsv_send_checkpoint(int choice)
{
  int FAIL=0;
  int i=0;
  NCS_MBCSV_ARG     mbcsv_arg;
  /*  MBCSTM_PEERS_DATA peers;
      int i=0;
  */
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t mbcstm_create_data_point(uint32_t , uint32_t );
  uint32_t mbcstm_print_data_points(uint32_t,uint32_t);
  uint32_t mbcstm_destroy_data_point(uint32_t , uint32_t );

  printf("\n--------- tet_mbcsv_send_checkpoint: Case %d ---------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_send_checkpoint %d",choice);
  tet_mbcsv_config();
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  memset(mbcstm_cb.svces[1].ssns[1].data, '\0', 
                  sizeof(MBCSTM_CSI_DATA));
  mbcstm_sync_point();

  switch(choice)
    {


    case 1:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Sync Send: Add Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 2:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("USR Async Send: Add Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_USR_ASYNC);

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
        }
      break;
    case 3:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("MBC ASync Send: Add Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_MBC_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_MBC_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_MBC_ASYNC);

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
        }
      break;
    case 4:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Sync Send: Remove Last Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,  
                             NCS_MBCSV_SND_SYNC);

          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_RMV, NORMAL_DATA,
                             1, NCS_MBCSV_SND_SYNC);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 5:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("USR Async Send: Remove Last Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,  
                             NCS_MBCSV_SND_USR_ASYNC);

          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_RMV, NORMAL_DATA,
                             1, NCS_MBCSV_SND_USR_ASYNC);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 6:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("MBC ASync Send: Remove Last Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_MBC_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_MBC_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,  
                             NCS_MBCSV_SND_MBC_ASYNC);

          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_RMV, NORMAL_DATA,
                             1, NCS_MBCSV_SND_MBC_ASYNC);
          mbcstm_sync_point();
        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
          sleep(4);
          mbcstm_print_data_points(1,1);
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 7:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Sync Send, USR Async, MBC Async: Update Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count, 
                             NCS_MBCSV_SND_MBC_ASYNC);
#if 0
          /*Mixing the Send types*/
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_MBC_ASYNC);

          
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_USR_ASYNC);
          
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_UPDATE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
#endif
          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 8:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Sync Send, USR Async, MBC Async: Dont Care Action");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_DONT_CARE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_DONT_CARE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_USR_ASYNC);
          sleep(1);
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_DONT_CARE, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_MBC_ASYNC);

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 9:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Not able to checkpiont with invalid Mbcsv handle");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);

          memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
          
          mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
          mbcsv_arg.i_mbcsv_hdl = (NCS_MBCSV_HDL)(long)NULL;/*wrong value*/
          mbcsv_arg.info.send_ckpt.i_action =NCS_MBCSV_ACT_ADD;
          mbcsv_arg.info.send_ckpt.i_ckpt_hdl = mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
          mbcsv_arg.info.send_ckpt.i_reo_hdl  = NORMAL_DATA;
          mbcsv_arg.info.send_ckpt.i_reo_type = 1;
          mbcsv_arg.info.send_ckpt.i_send_type =NCS_MBCSV_SND_SYNC;
          
          if (NCSCC_RC_SUCCESS == ncs_mbcsv_svc(&mbcsv_arg))
            FAIL=1;

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 10:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Not able to checkpoint with invalid Checkpoint Handle");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);

          memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
          
          mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
          mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
          mbcsv_arg.info.send_ckpt.i_action =NCS_MBCSV_ACT_ADD;
          mbcsv_arg.info.send_ckpt.i_ckpt_hdl =(NCS_MBCSV_HDL)(long)NULL ; /*wrong valude*/
          mbcsv_arg.info.send_ckpt.i_reo_hdl  = NORMAL_DATA;
          mbcsv_arg.info.send_ckpt.i_reo_type = 1;
          mbcsv_arg.info.send_ckpt.i_send_type =NCS_MBCSV_SND_SYNC;
          
          if (NCSCC_RC_SUCCESS == ncs_mbcsv_svc(&mbcsv_arg))
            FAIL=1;

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 11:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Not able to Sync Send with Invalid Action Item");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);

          memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
          
          mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
          mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
          mbcsv_arg.info.send_ckpt.i_action =6;/*wrong value: 0 to 5*/
          mbcsv_arg.info.send_ckpt.i_ckpt_hdl = mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
          mbcsv_arg.info.send_ckpt.i_reo_hdl  = NORMAL_DATA;
          mbcsv_arg.info.send_ckpt.i_reo_type = 1;
          mbcsv_arg.info.send_ckpt.i_send_type =NCS_MBCSV_SND_SYNC;
          
          if (NCSCC_RC_SUCCESS == ncs_mbcsv_svc(&mbcsv_arg))
            FAIL=1;

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 12:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Not able to Sync Send with Invalid Send Type");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);

          memset(&mbcsv_arg, '\0', sizeof(NCS_MBCSV_ARG));
          
          mbcsv_arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
          mbcsv_arg.i_mbcsv_hdl = mbcstm_cb.svces[1].mbcsv_hdl;
          mbcsv_arg.info.send_ckpt.i_action =1;
          mbcsv_arg.info.send_ckpt.i_ckpt_hdl = mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
            mbcsv_arg.info.send_ckpt.i_reo_hdl  = NORMAL_DATA;
          mbcsv_arg.info.send_ckpt.i_reo_type = 1;
          mbcsv_arg.info.send_ckpt.i_send_type =3;/*wrong value: 0 to 2*/
          
          if (NCSCC_RC_SUCCESS == ncs_mbcsv_svc(&mbcsv_arg))
            FAIL=1;

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 13:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("StandBy Client Not able to Send Check Point Data");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
            {
              mbcstm_create_data_point(1,1);
              if(mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                    mbcstm_cb.svces[1].ssns[1].data_count ,
                                    NCS_MBCSV_SND_SYNC)!=NCSCC_RC_SUCCESS)
                FAIL=1;
              sleep(1);
              mbcstm_create_data_point(1,1);
              if(mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                    mbcstm_cb.svces[1].ssns[1].data_count,
                                    NCS_MBCSV_SND_USR_ASYNC)!=NCSCC_RC_SUCCESS)
                FAIL=1;
              sleep(1);
              mbcstm_create_data_point(1,1);
              if( mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                     mbcstm_cb.svces[1].ssns[1].data_count,
                                     NCS_MBCSV_SND_MBC_ASYNC)!=NCSCC_RC_SUCCESS)
                FAIL=1;

            }
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 14:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("No Standby Available: Active trying to Send Check Point Data");
      mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
      if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
        {
          perror("Standby to Active role change Failed");
          FAIL=1;
        }

      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          if(mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                mbcstm_cb.svces[1].ssns[1].data_count,
                                NCS_MBCSV_SND_SYNC)!=NCSCC_RC_SUCCESS)
            FAIL=1;
          sleep(1);
          mbcstm_create_data_point(1,1);
          if(mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                mbcstm_cb.svces[1].ssns[1].data_count,
                                NCS_MBCSV_SND_USR_ASYNC)!=NCSCC_RC_SUCCESS)
            FAIL=1;
          sleep(1);
          mbcstm_create_data_point(1,1);
          if( mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                 mbcstm_cb.svces[1].ssns[1].data_count,
                                 NCS_MBCSV_SND_MBC_ASYNC)!=NCSCC_RC_SUCCESS)
            FAIL=1;

          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 15:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Client in QUIESCING: Sync Send: Add Check  point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          /*QUIESCING*/
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Quiescing role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);


          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 16:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Client in QUIESCED: Not able to Sync Send: Add Check  point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);

          /*QUIESCED*/
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Quiesced role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(2);
          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 17:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Reciever Client in QUIESCING: able to get Sync Send Check  point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();

          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(2);
          mbcstm_sync_point();
          mbcstm_create_data_point(1,1);
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             mbcstm_cb.svces[1].ssns[1].data_count,
                             NCS_MBCSV_SND_SYNC);
          sleep(2);
          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          /*QUIESCING*/
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Quiescing role change Failed");
              FAIL=1;
            }
          
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 18:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("Sync Send: Add Check Point 710 times");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();
          for(i=1;i<=710;i++)
            {
              mbcstm_create_data_point(1,1);
              if( mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                     mbcstm_cb.svces[1].ssns[1].data_count,
                                     NCS_MBCSV_SND_SYNC)!=NCSCC_RC_SUCCESS)
                {
                  FAIL=1;
                  break;
                }
            }
          if(i!=711)
            FAIL=1;
          mbcstm_sync_point();

        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 19:
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      tet_printf("IDLE Active Not able to Sync Send Check Point");
      /*PEER FAIL*/
      mbcstm_cb.svces[1].ssns[1].cb_test=MBCSTM_CB_PEER_INFO_FAIL;
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);

          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();
          for(i=1;i<=5;i++)
            {
              fflush(stdout);
              mbcstm_create_data_point(1,1);
              if( mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                                     mbcstm_cb.svces[1].ssns[1].data_count,
                                     NCS_MBCSV_SND_SYNC)!=NCSCC_RC_SUCCESS)
                FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        {
          sleep(2);
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_sync_point();
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;

    }  
  fflush(stdout); 
  sleep(4);
  mbcstm_sync_point();
  
  mbcstm_print_data_points(1,1);
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 

}






void tet_mbcsv_test()
{
  int FAIL=0;
  int choice=1;
  /*
    int i=0;
    MBCSTM_PEERS_DATA peers;
  */
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t mbcstm_svc_send_notify(uint32_t, uint32_t, NCS_MBCSV_NTFY_MSG_DEST, char *,
                               uint32_t );
  uint32_t mbcstm_create_data_point(uint32_t , uint32_t );

  printf("\n----------- tet_mbcsv_test: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    tet_printf("tet_mbcsv_Notify %d",choice);
  tet_mbcsv_config();


  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  mbcstm_sync_point();
  choice=5;
  switch(choice)
    {
    case 5:
      tet_printf("Send Check Point");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          mbcstm_create_data_point(1,1);
          printf("\n\t\tNow sending the Checkpoint");
          fflush(stdout);
          mbcstm_sync_point();
          mbcstm_svc_cp_send(1,1,NCS_MBCSV_ACT_ADD, NORMAL_DATA,
                             1, NCS_MBCSV_SND_SYNC);
        }
      else
        {
          mbcstm_sync_point();
          sleep(2);
          fflush(stdout);
          printf("\n\t\tReceiving the checkpoint");
          fflush(stdout);
          mbcstm_sync_point();
        }
      break;
    case 3:
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      fflush(stdout);
      sleep(2);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_TMR_WSYNC);
      sleep(2);
      mbcsv_prt_inv(); 
      fflush(stdout);
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      sleep(2);
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_GET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      fflush(stdout);
      break; 
    case 2:
      /*seg fault IR 82375 */
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point();
        }
      else
        mbcstm_sync_point();
      sleep(5);
      mbcsv_prt_inv(); 
      printf("\n\n");
      sleep(60);
      fflush(stdout);
      break;
    case 4:
      /*Intially Active to Quiesced, a new Active came: print inv ar Quiesced*/
      tet_printf("Send Notify message to STANDBY from ACTIVE");
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point(); 
          mbcsv_prt_inv();     
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();    
          fflush(stdout);
          /*          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
                  if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
                  {
                  perror("Standby to Active role change Failed");
                  FAIL=1;
                  }
                  mbcstm_sync_point(); 
                  mbcsv_prt_inv(); */
        }
      else
        {
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();     
          if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
            {
              mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
              if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
                {
                  perror("Standby to Active role change Failed");
                  FAIL=1;
                }
            }
          mbcstm_sync_point(); 
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();    
          fflush(stdout);
          /*          mbcstm_sync_point();
                  mbcsv_prt_inv(); */
        }
      break;
    case 1:
      /*Intially Active to Quiesced to Standby, 
        a new Active came: Warm sync problem : seg fault*/
      tet_printf("Send Notify message to STANDBY from ACTIVE");
      printf("\nDisabling the Warm Sync Flag\n");
      mbcstm_cb.svces[1].ssns[1].ws_flag=0;
      mbcstm_svc_obj(1,1,NCS_MBCSV_OP_OBJ_SET,NCS_MBCSV_OBJ_WARM_SYNC_ON_OFF);
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point(); 
          mbcsv_prt_inv();     
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();    
          fflush(stdout);
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_sync_point(); 
          mbcsv_prt_inv(); 
        }
      else
        {
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();     
          if(mbcstm_cb.sys == MBCSTM_SVC_INS3)
            {
              mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
              if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
                {
                  perror("Standby to Active role change Failed");
                  FAIL=1;
                }
            }
          mbcstm_sync_point(); 
          fflush(stdout);
          sleep(2);
          mbcstm_sync_point(); 
          mbcsv_prt_inv();    
          fflush(stdout);
          mbcstm_sync_point();
          mbcsv_prt_inv(); 
        }
      break;
    }  
  fflush(stdout);
  sleep(4);
  /*
    mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);
    printf("\n\n \t My Peers Count = %d \n",peers.peer_count);
    for(i=1;i<4;i++)
    {
    printf("\nState = %s Role : %c\n", peers.peers[i].state,
    peers.peers[i].peer_role);
    }
  */
  mbcsv_prt_inv(); 
  mbcstm_sync_point();
  
  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  
  mbcstm_sync_point();
  mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_STANDBY;
  mbcstm_cb.svces[1].ssns[1].dest_role =  V_DEST_RL_STANDBY;
  mbcstm_cb.svces[1].ssns[1].ws_flag = false;
  mbcstm_cb.svces[1].ssns[1].cb_test = MBCSTM_CB_NO_TEST;
  mbcstm_cb.svces[1].ssns[1].cb_test_result = NCSCC_RC_FAILURE;
  mbcstm_cb.svces[1].ssns[1].cb_flag = 0;
  
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  mbcstm_sync_point();
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;


  if(FAIL)
    tet_result(TET_FAIL);
  else
    tet_result(TET_PASS);

  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 

}
/*-----------  END TEST CASES ----------------------------------*/
/*utility */

uint32_t create_vdest(MDS_DEST vdest)
{
  memset(&vda_info,'\0', sizeof(vda_info)); 

  vda_info.req=NCSVDA_VDEST_CREATE;  

  vda_info.info.vdest_create.i_policy=NCS_VDEST_TYPE_MxN;
  vda_info.info.vdest_create.i_create_type=NCSVDA_VDEST_CREATE_SPECIFIC;
  vda_info.info.vdest_create.info.specified.i_vdest=vdest;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {

      printf("\n %lld : VDEST_CREATE is SUCCESSFUL\n Svc count = %d Session \
count  = %d\n", vdest,mbcstm_cb.svc_count,mbcstm_cb.vdest_count);
      
      mbcstm_cb.svces[mbcstm_cb.svc_count].ssns[mbcstm_cb.vdest_count].pwe_hdl=
        vda_info.info.vdest_create.o_mds_pwe1_hdl;
      
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
  memset(&vda_info,'\0', sizeof(vda_info)); 

  vda_info.req=NCSVDA_VDEST_DESTROY;

  vda_info.info.vdest_destroy.i_create_type=NCSVDA_VDEST_CREATE_SPECIFIC;
  vda_info.info.vdest_destroy.i_vdest=vdest;

  if(ncsvda_api(&vda_info)==NCSCC_RC_SUCCESS)
    {
      printf("\n %lld : VDEST_DESTROY is SUCCESSFULL\n",vdest);
      return NCSCC_RC_SUCCESS;
    }
  else
    {
      printf("\nRequest to ncsvda_api: VDEST_DESTROY has FAILED\n");
      return NCSCC_RC_FAILURE;
    }
}
uint32_t tet_mbcsv_dest_start(void)
{
  MDS_DEST    dest;
  uint32_t svc_count, ssn_index, ssn_count;
  
  memset(&dest , 0, sizeof(dest));
  ssn_count = mbcstm_cb.vdest_count;
  svc_count = mbcstm_cb.svc_count;
  for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
    {
      MBCSTM_SET_VDEST_ID_IN_MDS_DEST(dest,
                                      mbcstm_cb.vdests[ssn_index].dest_id); 
      if(create_vdest(dest)!=NCSCC_RC_SUCCESS)
        {
          printf("In tet_mbcsv_dest_start function.. VDEST creation failed\n");
          return NCSCC_RC_FAILURE;
        }
      mbcstm_cb.vdests[ssn_index].status = true;
    }
  
  return NCSCC_RC_SUCCESS;
}
uint32_t tet_mbcsv_dest_close(void)
{
  MDS_DEST        dest;
  uint32_t ssn_count,ssn_index,svc_index;
  
  memset(&dest , 0, sizeof(dest));
  ssn_count = mbcstm_cb.vdest_count;
  for(ssn_index = 1; ssn_index <= ssn_count; ssn_index++)
    {
      MBCSTM_SET_VDEST_ID_IN_MDS_DEST(dest,
                                      mbcstm_cb.vdests[ssn_index].dest_id);
      if(destroy_vdest(dest)!=NCSCC_RC_SUCCESS)
        {
          printf("In tet_mbcsv_dest_start function.. VDEST Destroy failed\n");
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
#if 0
uint32_t initsemaphore()
{
  int sem_id; 
  union semun {
    int val;
    struct semid_ds *buf;
    ushort * array;
  } argument;
  
  argument.val = 0;
  
  sem_id = semget(KEY, 1, 0666 | IPC_CREAT);
  
  if(sem_id < 0)
    {
      fprintf(stderr, "Unable to obtain semaphore.\n");
      exit(0);
    }
  if( semctl(sem_id, 0, SETVAL, argument) < 0)
    {
      fprintf( stderr, "Cannot set semaphore value.\n");
      return NCSCC_RC_FAILURE;
    }
  else
    {
      fprintf(stderr, "Semaphore %d initialized.\n", KEY);
      return NCSCC_RC_SUCCESS;
    }
}
uint32_t V_operation(int noprocess)
{
  int sem_id;
  struct sembuf operations[1];
  int retval;
  
  sem_id = semget(KEY, 1, 0666);
  if(sem_id < 0)
    {
      fprintf(stderr, "V_operatoin cannot find semaphore, exiting.\n");
      return NCSCC_RC_FAILURE;
    }
  operations[0].sem_num = 0;
  operations[0].sem_op = noprocess-1; /*total no. of processes*/
  operations[0].sem_flg = 0;
   
  retval = semop(sem_id, operations, 1);
   
  if(retval != 0)
    {
      perror("In MBCSTM_SVC_INS1: semop failed.\n");
      return NCSCC_RC_FAILURE;
    }    
  return NCSCC_RC_SUCCESS;
}
uint32_t P_operation(int sys)
{
  int sem_id;
  struct sembuf operations[1];
  int retval;
  
  sem_id = semget(KEY, 1, 0666);
  if(sem_id < 0)
    {
      fprintf(stderr, "P_operatoin cannot find semaphore, exiting.\n");
      return NCSCC_RC_FAILURE;
    }
  
  operations[0].sem_num = 0;
  operations[0].sem_op = -1; /*each no.of processes*/
  operations[0].sem_flg = 0;
  
  retval = semop(sem_id, operations, 1);
  
  if(retval != 0)
    {
      printf("In %d: semop failed.\n",sys);
      return NCSCC_RC_FAILURE;
    }   
  return NCSCC_RC_SUCCESS;
}
void tet_mbcsv_Notify(int choice)
{
  int FAIL=0;

  uint32_t temp_ckpt_hdl;
  char str[]="Yes My Dear Peer";
  int len=strlen(str)+1;
  char BIG[1200];
  uint32_t mbcstm_check_inv(MBCSTM_CHECK , uint32_t , uint32_t , void *);
  uint32_t mbcstm_svc_send_notify(uint32_t, uint32_t, NCS_MBCSV_NTFY_MSG_DEST, char *,
                               uint32_t );
    
  printf("\n----------- tet_mbcsv_Notify: Case %d -------------\n",choice);
  mbcstm_input();
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      tet_printf("tet_mbcsv_Notify %d",choice);
      V_operation(2);
    }
  else
    P_operation(mbcstm_cb.sys);

  tet_mbcsv_config();
  
  if(tet_mbcsv_dest_start()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_registration(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_ssn_open(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  switch(choice)
    { 

    case 1:
      tet_printf("Send Notify message to STANDBY from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        sleep(1);
      break;
    case 2:
      tet_printf("Send Notify message to ALL PEERS from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
        }
      else
        sleep(1);
      break;
    case 3:
      tet_printf("Send Notify message to ACTIVE from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          V_operation(2);
        }
      else
        {
          P_operation(mbcstm_cb.sys);
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to ACTIVE failed");
              FAIL=1;
            }
        }
      break;

    case 4:
      tet_printf("Send Notify message to ALL PEERS from QUIESCING");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCING;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
          V_operation(2);
        }
      else
        {
          sleep(1);
          P_operation(mbcstm_cb.sys);
        }
      break;
    case 5:
      tet_printf("Send Notify message to ALL PEERs from QUIESCED");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_QUIESCED;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to All Peers failed");
              FAIL=1;
            }   
          V_operation(2);
        }
      else
        {
          sleep(1);
          P_operation(mbcstm_cb.sys);
        }
        
      break;


    case 6:
      tet_printf("Send Notify message to STANDBY from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          V_operation(2);
        }
      else
        {
          P_operation(mbcstm_cb.sys);
        }
      break;
    case 7:
      tet_printf("BIG(1151 Bytes) Notify message to STANDBY from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          memset(BIG,'S',1200);
          BIG[1199]='\0';
          printf("\n BIG length = %d\n",strlen(BIG)); /*1100*/        
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,BIG,1150) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          V_operation(2);
        }
      else
        {
          P_operation(mbcstm_cb.sys);
        }
      break;
    case 8:
      tet_printf("Send Notify message to ACTIVE from ACTIVE");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
        }
      break;
    case 9:
      tet_printf("Send Notify message to ALL PEERS from STANDBY");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ALL_PEERS,str,len) 
              != NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          V_operation(2);
        }
      else
        {
          P_operation(mbcstm_cb.sys);
        }
      break;
    case 10:
      tet_printf("Not able to Send Notify message with invalid Destination");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          if( mbcstm_svc_send_notify(1,1,3,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        sleep(1);
      break;
    case 11:
      tet_printf("Not able to Send Notify message with invalid Ckpt Handle");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          temp_ckpt_hdl=mbcstm_cb.svces[1].ssns[1].ckpt_hdl;
          mbcstm_cb.svces[1].ssns[1].ckpt_hdl=(NCS_MBCSV_HDL)NULL;
          if( mbcstm_svc_send_notify(1,1,3,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
          mbcstm_cb.svces[1].ssns[1].ckpt_hdl=temp_ckpt_hdl;
        }
      else
        sleep(1);
      break;
#if 0
    case 12:
      tet_printf("Not able to Send Notify message to STANDBY from ACTIVE \
when No STANDBY peer available");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_STANDBY,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Standby failed");
              FAIL=1;
            }   
        }
      else
        {
          mbcstm_cb.svces[1].ssns[1].csi_role = SA_AMF_HA_ACTIVE;
          if(mbcstm_ssn_set_role(1,1) != NCSCC_RC_SUCCESS)  
            {
              perror("Standby to Active role change Failed");
              FAIL=1;
            }
          sleep(2);
        }
      break;
    case 13:
      tet_printf("Not able to Send Notify message to ACTIVE from STANDBY \
when No ACTIVE peer available");
      if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
        {
          if( mbcstm_svc_send_notify(1,1,NCS_MBCSV_ACTIVE,str,len) 
              == NCSCC_RC_SUCCESS)
            {
              tet_printf( "\n Send notify to Active failed");
              FAIL=1;
            }   
        }
      else
        printf("\n");
      break;
#endif
    }  
  fflush(stdout);
  sleep(4);
  /*mbcstm_check_inv(MBCSTM_CHECK_PEER_LIST, 1, 1, &peers);*/
  mbcsv_prt_inv(); 
  /* Semaphore Solution */
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)
    {
      V_operation(2);
      sleep(1);
    }
  else
    P_operation(mbcstm_cb.sys);

  if(mbcstm_ssn_close(1,1)!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_svc_finalize(1)!=NCSCC_RC_SUCCESS)
    FAIL=1;   
  if(tet_mbcsv_dest_close()!=NCSCC_RC_SUCCESS)
    FAIL=1;
  if(mbcstm_cb.sys == MBCSTM_SVC_INS1)  
    {
      if(FAIL)
        tet_result(TET_FAIL);
      else
        tet_result(TET_PASS);
    }   
  printf("\n--------------------------- End ------------------------------\n");
  fflush(stdout); 
}
#endif
