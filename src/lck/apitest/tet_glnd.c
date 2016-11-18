#if ( (NCS_GLND == 1) && (TET_ND == 1) )

#include "tet_glsv.h"

extern uint32_t gl_glnd_hdl;

void tet_run_glnd() {

   tet_create_glnd();
   tet_glnd_show();
}
   
void tet_glnd_show() {
   char c;
   while(1) {
      c=getchar();
      switch(c) {
         case 'n':
            printf("Show node information \n");
            break;
         case 'r':
            print_resource_cb_info();
            break;
         case 'c':
            print_glnd_cb();
            break;
         case 'e':
            tet_destroy_glnd();
            break;
         default:
            printf("enter appropriate code,codes supported are glnd-n,rsc-r,end-e...\n");
            break;
      }
     if (c == 'e')exit(1);
  }


}

void tet_destroy_glnd() 
{

   NCS_LIB_REQ_INFO lib_create;
   lib_create.i_op = NCS_LIB_REQ_DESTROY;

   if ( glnd_lib_req(&lib_create) != NCSCC_RC_SUCCESS)
   {
      printf("GLND lib req failed ... ");
      return;
   }
   printf("GLND destroyed ...");

}

void tet_create_glnd() 
{
   
   NCS_LIB_REQ_INFO lib_create;
   lib_create.i_op = NCS_LIB_REQ_CREATE;

   if ( glnd_lib_req(&lib_create) != NCSCC_RC_SUCCESS)
   {
      printf("GLND lib req failed ... ");
      sleep(1);
   }
   printf("GLND lib req Success ....\n");

}

void print_glnd_cb() {

   GLND_CB           *cb;
   GLSV_GLND_EVT     *glnd_evt;
                                                                                
   glnd_evt = m_MMGR_ALLOC_GLND_EVT;
   memset(glnd_evt,0,sizeof(GLSV_GLND_EVT));
   glnd_evt->type = GLSV_GLND_EVT_CB_DUMP;
                                                                                
   cb =   (GLND_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLND, m_GLND_RETRIEVE_GLND_CB_HDL);
   glnd_evt->glnd_hdl = cb->cb_hdl_id;
   m_NCS_IPC_SEND(&cb->glnd_mbx, glnd_evt, MDS_SEND_PRIORITY_MEDIUM);
   ncshm_give_hdl(gl_glnd_hdl);
}



void print_resource_cb_info() {

   GLND_CB       *glnd_cb;
   GLND_RESOURCE_INFO *glnd_rsc;
   uint32_t res = NCSCC_RC_SUCCESS;
   uint8_t *key=NULL;

   printf("print_resource_cb_info  %x\n",glnd_rsc);

   glnd_cb =   (GLND_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLND, gl_glnd_hdl);
   if ( glnd_cb == NULL) {
      printf(" GLND Startup failed ...\n");
      return;
   } else {
      printf("GLND Startup Success ... %x\n",glnd_cb);
   }

   glnd_rsc=(GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree,(uint8_t *)0);
   while(glnd_rsc != NULL) {

      printf("\n********************************************\n");
      printf("GLND RSC INFORMATION %x\n",glnd_rsc);
      printf("GLND RSC ID %x\n",glnd_rsc->resource_id);
      printf("GLND RSC NAME %s\n",glnd_rsc->resource_name.value);
      printf("GLND RSC MASTER NODE_ID %d\n",glnd_rsc->master_mds_dest.node_id);
      printf("Locks on this resource ...\n");
      printf("\n********************************************\n");

      key=(uint8_t *)&glnd_rsc->resource_id;
      glnd_rsc=(GLND_RESOURCE_INFO *)ncs_patricia_tree_getnext(&glnd_cb->glnd_res_tree,key);
   }
}

#if 0
void tet_glnd_testcase_1() {
      
     tet_infoline(" In glnd_testcase_1 ....");
     if (tet_remsync(101L,sys0,1,TIMEOUT,TET_SV_YES,
                      (struct tet_synmsg *)0) !=0 ) {
        tet_infoline("sync not succeeded...");
     } else {
        tet_infoline("sync succeeds...");
     }
}


void tet_glnd_testcase_2 () 
{

   NCS_LIB_REQ_INFO lib_create;
   lib_create.i_op = NCS_LIB_REQ_DESTROY;

   if ( glnd_lib_req(&lib_create) != NCSCC_RC_SUCCESS)
   {
      printf("GLND lib req failed ... ");
      sleep(1);
   }
   printf("GLND destroyed ...");

}

uint32_t tet_glnd_testcase_3(SaNameT rscName)
{

  GLSV_GLD_EVT            gld_evt;
  uint32_t                   ret;
  GLND_CB       *glnd_cb=0;
  
  glnd_cb = (GLND_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLND,gl_glnd_hdl);
  if (!glnd_cb)
  {
    printf(" Unable to retrieve GLND_CB ..\n");
    return NCSCC_RC_FAILURE;
  }

  memset(&gld_evt,0,sizeof(GLSV_GLD_EVT)); 
  gld_evt.evt_type = GLSV_GLD_EVT_RSC_OPEN;

  memcpy(&gld_evt.info.rsc_open_info.rsc_name, \
                &rscName,sizeof(SaNameT));

  ret=glnd_mds_msg_send_gld(glnd_cb,&gld_evt,glnd_cb->gld_mdest_id);
  if ( ret != NCSCC_RC_SUCCESS) {
     printf(" Unable to send GLSV_GLD_EVT_RSC_OPEN message \n");
  }

}
#endif
#endif


