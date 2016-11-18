#if ( (NCS_GLD == 1) && (TET_D == 1) )

#include "tet_glsv.h"

extern uint32_t gl_gld_hdl;

void tet_run_gld() {

     tet_create_gld();
}

void tet_destroy_gld() {

   NCS_LIB_REQ_INFO lib_create;
   lib_create.i_op = NCS_LIB_REQ_DESTROY;
   char c;

   if ( gld_lib_req(&lib_create) != NCSCC_RC_SUCCESS)
   {
      printf("GlD lib req failed ... \n");
      sleep(1);
   } else {
      printf("GLD lib req Success ....\n");
   }
}

void tet_create_gld() 
{
   
   NCS_LIB_REQ_INFO lib_create;
   lib_create.i_op = NCS_LIB_REQ_CREATE;
   char c;

   if ( gld_lib_req(&lib_create) != NCSCC_RC_SUCCESS)
   {
      printf("GlD lib req failed ... \n");
      sleep(1);
   } else {
      printf("GLD lib req Success ....\n");
   }

   printf(" Enter getchar to see gld_cb information \n");
   while(1) {
      c=getchar();
      switch(c) {
         case 'n':
            print_glnd_cb_info();
            break;
         case 'r':
            print_rsc_cb_info();
            break;
         case 'c':
            tet_gld_dump_cb();
            break;
         case 'e':
            tet_destroy_gld();
            exit(1);
            break;

         default:
            printf("enter appropriate code,codes supported are glnd-n,rsc-r,end-e...\n");
            break;
      }
     if (c == 'e') break;
  }

}

void tet_gld_dump_cb(void) {

   GLSV_GLD_CB       *cb;
   GLSV_GLD_EVT      *gld_evt;
                                                                                
   gld_evt = m_MMGR_ALLOC_GLSV_GLD_EVT;
   m_NCS_OS_MEMSET(gld_evt, 0,sizeof(GLSV_GLD_EVT));
   cb =   (GLSV_GLD_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
   gld_evt->gld_cb = cb;
   gld_evt->evt_type = GLSV_GLD_EVT_CB_DUMP;
   m_NCS_IPC_SEND(&cb->mbx, gld_evt, NCS_IPC_PRIORITY_NORMAL);
   ncshm_give_hdl(gl_gld_hdl);

}

void tet_create_gld_and_continue() {
   tet_create_gld();
   while(1)sleep(10000);

}

void print_rsc_cb_info() {

   GLSV_GLD_CB       *gld_cb;
   GLSV_GLD_RSC_INFO *gld_rsc;
   GLSV_NODE_LIST    *temp_node;
   uint32_t res = NCSCC_RC_SUCCESS;
   uint8_t *key=NULL;

   printf("print_rsc_cb_info  %x\n",gld_rsc);

   gld_cb =   (GLSV_GLD_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);
   if ( gld_cb == NULL) {
      printf(" GLD Startup failed ...\n");
   } else {
      printf("GLD Startup Success ... %x\n",gld_cb);
   }

   gld_rsc=(GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id,(uint8_t *)0);
   while(gld_rsc != NULL) {

      printf("\n********************************************\n");
      printf("GLD RSC INFORMATION %x\n",gld_rsc);
      printf("GLD RSC NAME %s\n",gld_rsc->lck_name.value);
      printf("GLD rsc_id %d\n",gld_rsc->rsc_id);
      temp_node=gld_rsc->node_list;
      while(temp_node != NULL) {
         printf("Refrence nodes node id %d\n",temp_node->dest_id.node_id);
         temp_node=temp_node->next;  
      }
      printf("\n********************************************\n");

      key=(uint8_t *)&gld_rsc->rsc_id;
      gld_rsc=(GLSV_GLD_RSC_INFO *)ncs_patricia_tree_getnext(&gld_cb->rsc_info_id,key);
   }
}

void print_glnd_cb_info() {

   GLSV_GLD_CB       *gld_cb;
   GLSV_GLD_GLND_DETAILS *node_details=NULL;
   GLSV_GLD_GLND_RSC_REF *glnd_rsc=NULL;
   uint32_t res = NCSCC_RC_SUCCESS;
   uint8_t *key=NULL;
   
   gld_cb =   (GLSV_GLD_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);

   if ( gld_cb == NULL) {
      printf(" GLD Startup failed ...\n");
   } else {
      printf("GLD Startup Success ... %x\n",gld_cb);
   }

   node_details  = (GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details,(uint8_t*)0); 

   while (node_details != NULL) {

       printf("\n********************************************\n");
       printf("GLND Node information %x\n",node_details);
       printf("GLND NODE-ID %d\n",node_details->node_id);
       printf("\n********************************************\n");

       key=(uint8_t *)&node_details->dest_id;
       node_details=(GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_getnext(&gld_cb->glnd_details,key);
   }
}

#if 0
void tet_gld_testcase_1 () 
{
   GLSV_GLD_CB       *cb;
   cb =   (GLSV_GLD_CB*)ncshm_take_hdl(NCS_SERVICE_ID_GLD, gl_gld_hdl);

   if ( cb == NULL) {
      tet_infoline(" GLD Startup failed ...\n");
   } 

   /* check for mds handle */
   if (cb->mds_handle != NULL ) {
      tet_infoline(" GLD Initialisation with mds successfully .\n");     
      tet_result(TET_PASS);
   } else {
      tet_infoline(" GLD Initialisation with mds failed  ..\n");
      tet_result(TET_FAIL);
   }

   ncshm_give_hdl(gl_gld_hdl);
   tet_result(TET_PASS);
 
}

void tet_gld_testcase_2 () 
{
   GLSV_GLD_CB *gld_cb;
   GLSV_GLD_GLND_DETAILS   *node_details;
   int num;
   int nodes=2;

   gld_cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD,gl_gld_hdl);
   if (gld_cb == NULL) {
      printf(" GLD Startup failed ...\n");
   }

   /* sync with other two systems */
     if (tet_remsync(101L,sys1,1,TIMEOUT,TET_SV_YES,
                                (struct tet_synmsg *)0) != 0) {
        tet_infoline("synchronisation not succeds");
     } else 
        tet_infoline("synchrnisation succeeds");
  
 
   num=(GLSV_GLD_GLND_DETAILS *)ncs_patricia_tree_size(&gld_cb->glnd_details);

   if ( num == nodes) {
      printf(" GLD updated the node ...\n");
   } else { 
      printf(" GLD cb contains %d GLND...\n",num);
   } 

   ncshm_give_hdl(gl_gld_hdl);
}

void tet_gld_testcase_3 (SaNameT rsc_name)
{
   GLSV_GLD_CB *gld_cb;
   GLSV_GLD_RSC_INFO *rsc_info;

   gld_cb = (GLSV_GLD_CB *)ncshm_take_hdl(NCS_SERVICE_ID_GLD,gl_gld_hdl);
   if (gld_cb == NULL) {
      printf(" GLD_CB retreival failed ...\n");
   }

   rsc_info = gld_find_add_rsc_name(&gld_cb,rsc_name);
   if (rsc_info != NULL) {
      printf(" Resource is added ...\n");
   } else {
      printf(" Unable to add resource ...\n");
   }

   ncshm_give_hdl(gl_gld_hdl);

}
#endif

#endif
