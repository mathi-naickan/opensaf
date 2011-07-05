/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2008 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.
 *
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................



..............................................................................

  DESCRIPTION:

  This file contains the AMF API implementation.
..............................................................................

  FUNCTIONS INCLUDED in this module:
  

******************************************************************************
*/

#include "ava.h"

/* macro for api pre-processing */
#define m_AVA_API_PRE_PROCESSING(api, cbhdl, o_cb, hdl, o_hdlrec, o_rc) \
{ \
   /* retrieve AvA CB */ \
   if ( \
        (!((o_cb) = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, (cbhdl)))) || \
        ( !((AVSV_AMF_PG_START == (api)) || \
            (AVSV_AMF_PG_STOP == (api)) || \
            (AVSV_AMF_INITIALIZE == (api)) || \
            (AVSV_AMF_ERR_REP == (api)) || \
            (AVSV_AMF_PM_START == (api)) || \
            (AVSV_AMF_PM_STOP == (api)) \
           ) && \
          !m_AVA_FLAG_IS_COMP_NAME((o_cb)) ) \
      ) \
      (o_rc) = SA_AIS_ERR_LIBRARY; \
   if ( SA_AIS_OK  == (o_rc) ) { \
      /* acquire cb read/write lock */ \
      if ( \
           ( AVSV_AMF_INITIALIZE == (api) ) || \
           ( AVSV_AMF_PG_START == (api) ) || \
           ( AVSV_AMF_DISPATCH == (api) ) || \
           ( AVSV_AMF_FINALIZE == (api) ) \
         ) \
         m_NCS_LOCK(&(o_cb)->lock, NCS_LOCK_WRITE); \
      else \
         m_NCS_LOCK(&(o_cb)->lock, NCS_LOCK_READ); \
      /* retrieve hdl rec */ \
      if ( !((AVSV_AMF_INITIALIZE == (api)) || (AVSV_AMF_FINALIZE == (api))) ) { \
         if ( !((o_hdlrec) = (AVA_HDL_REC *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, \
                                                           (hdl))) ) \
            (o_rc) = SA_AIS_ERR_BAD_HANDLE; \
      } \
   } \
};

/* macro for api post-processing */
#define m_AVA_API_POST_PROCESSING(api, cbhdl, cb, hdl, hdlrec, cn, rc) \
{ \
   /* release cb read/write lock */ \
   if ((cb)) { \
      if ( \
           (AVSV_AMF_INITIALIZE == (api)) || \
           (AVSV_AMF_DISPATCH == (api)) || \
           (AVSV_AMF_FINALIZE == (api)) \
         ) \
         m_NCS_UNLOCK(&(cb)->lock, NCS_LOCK_WRITE); \
      else \
         m_NCS_UNLOCK(&(cb)->lock, NCS_LOCK_READ); \
   } \
   /* return AvA CB & hdl rec */ \
   if ((cb)) ncshm_give_hdl((cbhdl)); \
   if ( (hdlrec) && \
        ( (AVSV_AMF_INITIALIZE != (api)) || (AVSV_AMF_FINALIZE != (api)) ) ) \
      ncshm_give_hdl((hdl)); \
   /* log the status of api processing */ \
   m_AVA_LOG_API((api), (rc)+AVA_LOG_API_ERR_OFFSET, (cn), NCSFL_SEV_INFO); \
};

/* Macro for Verifying the input Handle & global handle */
#define m_AVA_API_HDL_VERIFY(cbhdl, hdl, o_rc) \
{ \
   /* is library Initialized && handle a 32 bit value*/ \
   if(!(cbhdl) || (hdl) > AVSV_UNS32_HDL_MAX) \
      (o_rc) = SA_AIS_ERR_BAD_HANDLE;\
};

/* Macro for Verifying the input Invocation type */
#define m_AVA_API_HC_INV_VERIFY(i_inv, o_rc) \
{ \
   if((SA_AMF_HEALTHCHECK_AMF_INVOKED != (i_inv)) && (SA_AMF_HEALTHCHECK_COMPONENT_INVOKED != (i_inv))) \
      (o_rc) = SA_AIS_ERR_INVALID_PARAM;\
};


/****************************************************************************
  Name          : saAmfInitialize
 
  Description   : This function initializes the AMF for the invoking process
                  and registers the various callback functions.
 
  Arguments     : o_hdl    - ptr to the AMF handle
                  reg_cbks - ptr to a SaAmfCallbacksT structure
                  io_ver   - Version of the AMF implementation being used 
                             by the invoking process.
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfInitialize (SaAmfHandleT *o_hdl, 
                             const SaAmfCallbacksT *reg_cbks,
                             SaVersionT *io_ver)
{
   AVA_CB       *cb = 0;
   AVA_HDL_DB   *hdl_db = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   SaAisErrorT  rc = SA_AIS_OK;
   
   if ( !o_hdl || !io_ver )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* Initialize the environment */
   if (ncs_agents_startup() != NCSCC_RC_SUCCESS)
   {
      return SA_AIS_ERR_LIBRARY;
   }
 
   /* Create AVA/CLA  CB */ 
   if (ncs_ava_startup() != NCSCC_RC_SUCCESS)
   {
      ncs_agents_shutdown();
      return SA_AIS_ERR_LIBRARY;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_INITIALIZE, gl_ava_hdl, 
                            cb, 0, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* validate the version */
   if ( !m_AVA_VER_IS_VALID(io_ver) )
      rc = SA_AIS_ERR_VERSION;

   /* fill the supported version no */
   io_ver->releaseCode = 'B';
   io_ver->majorVersion = 1;
   io_ver->minorVersion = 1;

   if (SA_AIS_OK != rc) goto done;

   /* get the ptr to the hdl db */
   hdl_db = &cb->hdl_db;

   /* create the hdl record & store the callbacks */
   if ( !(hdl_rec = ava_hdl_rec_add(cb, hdl_db, reg_cbks)) )
   {
      rc = SA_AIS_ERR_NO_MEMORY;
      goto done;
   }

   /* pass the handle value to the appl */
   if (SA_AIS_OK == rc) *o_hdl = hdl_rec->hdl;

done:
   /* free the hdl rec if there's some error */
   if (hdl_rec && SA_AIS_OK != rc)
      ava_hdl_rec_del (cb, hdl_db, hdl_rec);

   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_INITIALIZE, gl_ava_hdl, cb, 0, 0, 0, rc);

   if (SA_AIS_OK != rc)
   {
      ncs_ava_shutdown();
      ncs_agents_shutdown();
   }

   return rc;
}


/****************************************************************************
  Name          : saAmfSelectionObjectGet
 
  Description   : This function creates & returns the operating system handle 
                  associated with the AMF Handle.
 
  Arguments     : hdl       - AMF handle      - AMF handle
                  o_sel_obj - ptr to the selection object
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfSelectionObjectGet (SaAmfHandleT hdl, 
                                     SaSelectionObjectT *o_sel_obj)
{
   AVA_CB      *cb = 0;
   AVA_HDL_REC *hdl_rec = 0;
   SaAisErrorT rc = SA_AIS_OK;

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !o_sel_obj )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_SEL_OBJ_GET, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* everything's fine.. pass the sel obj to the appl */
   *o_sel_obj = (SaSelectionObjectT)m_GET_FD_FROM_SEL_OBJ(hdl_rec->sel_obj);

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_SEL_OBJ_GET, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);
   return rc;
}


/****************************************************************************
  Name          : saAmfDispatch
 
  Description   : This function invokes, in the context of the calling thread,
                  the next pending callback for the AMF handle.
 
  Arguments     : hdl   - AMF handle
                  flags - flags that specify the callback execution behavior
                  of the saAmfDispatch() function,
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfDispatch (SaAmfHandleT hdl,
                           SaDispatchFlagsT flags)
{
   AVA_CB      *cb = 0;
   AVA_HDL_REC *hdl_rec = 0;
   SaAisErrorT rc = SA_AIS_OK;
   uns32 pend_fin = 0;
   uns32 pend_dis = 0;

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if (!m_AVA_DISPATCH_FLAG_IS_VALID(flags))
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_DISPATCH, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* Increment Dispatch usgae count */
   cb->pend_dis++;

   rc = ava_hdl_cbk_dispatch(&cb, &hdl_rec, flags);

   /* Decrement Dispatch usage count */
   cb->pend_dis--;

   /* can't use cb after we do agent shutdown, so copy all counts */
   pend_dis = cb->pend_dis;
   pend_fin = cb->pend_fin;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_DISPATCH, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);
  
  /* see if we are still in any dispact context */ 
   if(pend_dis == 0)
      while(pend_fin != 0)
      {
         /* call agent shutdown,for each finalize done before */
         ncs_ava_shutdown();
         ncs_agents_shutdown();
         pend_fin--;
      }

   return rc;
}


/****************************************************************************
  Name          : saAmfFinalize
 
  Description   : This function closes the association, represented by the 
                  AMF handle, between the invoking process and the AMF.
 
  Arguments     : hdl - AMF handle
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfFinalize (SaAmfHandleT hdl)
{
   AVA_CB       *cb = 0;
   AVA_HDL_DB   *hdl_db = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   SaAisErrorT  rc = SA_AIS_OK;
   NCS_BOOL agent_flag = FALSE; /* flag = FALSE, we should not call agent shutdown */

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_FINALIZE, gl_ava_hdl, 
                            cb, 0, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* get the ptr to the hdl db */
   hdl_db = &cb->hdl_db;

   /* get the hdl rec */
   m_AVA_HDL_REC_FIND(hdl_db, hdl, hdl_rec);
   if (!hdl_rec)
   {
      rc = SA_AIS_ERR_BAD_HANDLE;
      goto done;
   }

   /* populate & send the finalize message */
   m_AVA_AMF_FINALIZE_MSG_FILL(msg, cb->ava_dest, hdl, cb->comp_name);
   rc = ava_mds_send(cb, &msg, 0);
   if (NCSCC_RC_SUCCESS != rc)
      rc = SA_AIS_ERR_TRY_AGAIN;

   /* delete the hdl rec */
   if (SA_AIS_OK == rc) ava_hdl_rec_del(cb, hdl_db, hdl_rec);

   /* Fialize the environment */  
   if ( SA_AIS_OK == rc && cb->pend_dis == 0)
      agent_flag = TRUE;
   else if(SA_AIS_OK == rc && cb->pend_dis > 0)
      cb->pend_fin++;
   

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_FINALIZE, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);

   /* free the contents of the request message */
   avsv_nda_ava_msg_content_free(&msg);

   /* we are not in any dispatch context, we can do agent shutdown */
   if(agent_flag == TRUE) 
   {
      ncs_ava_shutdown();
      ncs_agents_shutdown();
   }

   return rc;
}


/****************************************************************************
  Name          : saAmfComponentRegister
 
  Description   : This function registers the component with the AMF.
 
  Arguments     : hdl             - AMF handle
                  comp_name       - ptr to the comp name
                  proxy_comp_name - ptr to the proxy comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfComponentRegister (SaAmfHandleT  hdl,
                                    const SaNameT *comp_name,
                                    const SaNameT *proxy_comp_name)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaNameT          pcomp_name;
   SaAisErrorT      rc = SA_AIS_OK;

   /* initialize the msg * proxy-comp name */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));
   memset(&pcomp_name, 0, sizeof(SaNameT));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) ||
         (comp_name->length > SA_MAX_NAME_LENGTH) ||
         (proxy_comp_name && (!proxy_comp_name->length ||
          proxy_comp_name->length > SA_MAX_NAME_LENGTH)))
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* if comp-name is not set, it should be available now */
   cb = (AVA_CB *)ncshm_take_hdl(NCS_SERVICE_ID_AVA, gl_ava_hdl);
   if (!cb)
   {
      return SA_AIS_ERR_LIBRARY;
   }
   if ( !m_AVA_FLAG_IS_COMP_NAME(cb) )
   {
      if ( getenv("SA_AMF_COMPONENT_NAME") )
      {
         if(strlen(getenv("SA_AMF_COMPONENT_NAME")) < SA_MAX_NAME_LENGTH) {
             strcpy((char*)cb->comp_name.value, getenv("SA_AMF_COMPONENT_NAME"));
             cb->comp_name.length = (uns16)strlen((char*)cb->comp_name.value);
             m_AVA_FLAG_SET(cb, AVA_FLAG_COMP_NAME);
         }else {
             rc = SA_AIS_ERR_INVALID_PARAM;
             goto done;
         }
      }
      else
      {
         rc = SA_AIS_ERR_LIBRARY;
         goto done;
      }
   }
   if(cb)
      ncshm_give_hdl(gl_ava_hdl);

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_COMP_REG, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;
 
    /* non-proxied, component Length part of component name should be OK */
   if(!proxy_comp_name && ( comp_name->length != cb->comp_name.length))
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

   /* proxy component, Length part of proxy component name should be Ok */
   if(proxy_comp_name && (proxy_comp_name->length != cb->comp_name.length)) 
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }
  
    /* non-proxied component should not forge its name while registering */
   if(!proxy_comp_name && 
         strncmp((char*)comp_name->value, (char*)cb->comp_name.value, comp_name->length))
   {
      rc = SA_AIS_ERR_BAD_OPERATION;
      goto done;
   }

   /* proxy component should not forge its name while registering its proxied*/
   if(proxy_comp_name && 
         strncmp((char*)proxy_comp_name->value, (char*)cb->comp_name.value, proxy_comp_name->length))
   {
      rc = SA_AIS_ERR_BAD_OPERATION;
      goto done;
   }

   /* check if the required callbacks were supplied during saAmfInitialize */
   if ( !m_AVA_HDL_ARE_REG_CBKS_PRESENT(hdl_rec, proxy_comp_name) )
   {
      rc = SA_AIS_ERR_INIT;
      goto done;
   }

   /* populate & send the register message */
   if (proxy_comp_name) pcomp_name = *proxy_comp_name;
   m_AVA_COMP_REG_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, pcomp_name);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_COMP_REG == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_COMP_REG, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfComponentUnregister
 
  Description   : This function unregisters the component with the AMF.
 
  Arguments     : hdl             - AMF handle
                  comp_name       - ptr to the comp name
                  proxy_comp_name - ptr to the proxy comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfComponentUnregister (SaAmfHandleT hdl,
                                      const SaNameT *comp_name,
                                      const SaNameT *proxy_comp_name)
{
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   AVA_CB           *cb = 0;
   AVA_HDL_REC      *hdl_rec = 0;
   SaNameT          pcomp_name;
   SaAisErrorT      rc = SA_AIS_OK;

   /* initialize the msg & proxy-comp name */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));
   memset(&pcomp_name, 0, sizeof(SaNameT));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || 
        (comp_name->length > SA_MAX_NAME_LENGTH) ||
        (proxy_comp_name && (!proxy_comp_name->length || 
         proxy_comp_name->length > SA_MAX_NAME_LENGTH)))
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_COMP_UNREG, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;
 
    /* non-proxied, component Length part of component name should be OK */
   if(!proxy_comp_name && ( comp_name->length != cb->comp_name.length))
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

   /* proxy component, Length part of proxy component name should be Ok */
   if(proxy_comp_name && (proxy_comp_name->length != cb->comp_name.length))
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

  
    /* non-proxied component should not forge its name while unregistering */
   if(!proxy_comp_name && 
         strncmp((char*)comp_name->value, (char*)cb->comp_name.value, comp_name->length))
   {
      rc = SA_AIS_ERR_BAD_OPERATION;
      goto done;
   }


   /* proxy component should not forge its name while unregistering proxied */
   if(proxy_comp_name && 
         strncmp((char*)proxy_comp_name->value, (char*)cb->comp_name.value, proxy_comp_name->length))
   {
      rc = SA_AIS_ERR_BAD_OPERATION;
      goto done;
   }

   /* populate & send the unregister message */
   if (proxy_comp_name) pcomp_name = *proxy_comp_name;
   m_AVA_COMP_UNREG_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, pcomp_name);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_COMP_UNREG == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_COMP_UNREG, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfHealthcheckStart
 
  Description   : This function allows a process (as part of a comp) to start
                  a specific configured healthcheck.
 
  Arguments     : hdl         - AMF handle
                  comp_name   - ptr to the comp name
                  hc_key      - ptr to the healthcheck type
                  inv         - indicates whether the healthcheck is
                                AMF initiated / comp initiated
                  rec_rcvr    - recommended recovery

 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckStart (SaAmfHandleT hdl,
                                   const SaNameT *comp_name,
                                   const SaAmfHealthcheckKeyT *hc_key,
                                   SaAmfHealthcheckInvocationT inv,
                                   SaAmfRecommendedRecoveryT rec_rcvr)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   /* verify input inv  */
   m_AVA_API_HC_INV_VERIFY(inv, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
        !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_HC_START, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* check if hc cbk was supplied during saAmfInitialize (for AMF invoked healthcheck) */
   if ( (SA_AMF_HEALTHCHECK_AMF_INVOKED == inv) &&
        (!m_AVA_HDL_IS_HC_CBK_PRESENT(hdl_rec)) )
   {
      rc = SA_AIS_ERR_INIT;
      goto done;
   }

   /* populate & send the healthcheck start message */
   m_AVA_HC_START_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key, 
                           inv, rec_rcvr);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_HC_START == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_HC_START, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfHealthcheckStop
 
  Description   : This function allows a process to stop an already started
                  healthcheck.
 
  Arguments     : hdl       - AMF handle
                  comp_name - ptr to the comp name
                  hc_key    - ptr to the healthcheck type
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckStop (SaAmfHandleT hdl,
                                  const SaNameT *comp_name,
                                  const SaAmfHealthcheckKeyT *hc_key)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
        !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_HC_STOP, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the healthcheck stop message */
   m_AVA_HC_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_HC_STOP == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_HC_STOP, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfHealthcheckConfirm
 
  Description   : This function allows a process (as part of a component) to 
                  inform the AMF that it has performed the healthcheck.
 
  Arguments     : hdl       - AMF handle
                  comp_name - ptr to the comp name
                  hc_key    - ptr to the healthcheck type
                  hc_result - healthcheck result
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfHealthcheckConfirm (SaAmfHandleT hdl,
                                     const SaNameT *comp_name,
                                     const SaAmfHealthcheckKeyT *hc_key,
                                     SaAisErrorT hc_result)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
        !hc_key || !(hc_key->keyLen) || (hc_key->keyLen > SA_AMF_HEALTHCHECK_KEY_MAX) ||
        !m_AVA_AMF_RESP_ERR_CODE_IS_VALID(hc_result) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_HC_CONFIRM, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the healthcheck confirm message */
   m_AVA_HC_CONFIRM_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, cb->comp_name, *hc_key, hc_result);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_HC_CONFIRM == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_HC_CONFIRM, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}



/****************************************************************************
  Name          : saAmfPmStart
 
  Description   : This function allows a process (as part of a comp) to start
                  Passive Monitoring.
 
  Arguments     : hdl            - AMF handle
                  comp_name      - ptr to the comp name
                  processId      - Identifier of a process to be monitored 
                  desc_TreeDepth - Depth of the tree of descendents of the process
                  pmErr          - Specifies the type of process errors to monitor
                  rec_Recovery   - recommended recovery

 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfPmStart (SaAmfHandleT hdl,
                          const SaNameT *comp_name,
                          SaUint64T processId,
                          SaInt32T desc_TreeDepth,
                          SaAmfPmErrorsT pmErr,
                          SaAmfRecommendedRecoveryT rec_Recovery)

{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   /* input validation of component name */
   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* input validation of descendant tree depth */
   if (desc_TreeDepth < AVA_PM_START_ALL_DESCENDENTS) 
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }
   
   /* input validation of pmError */
   if (pmErr != SA_AMF_PM_NON_ZERO_EXIT &&  pmErr != SA_AMF_PM_ZERO_EXIT &&
           pmErr != (SA_AMF_PM_NON_ZERO_EXIT | SA_AMF_PM_ZERO_EXIT)) 
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* input validation of Recomended recovery */   
   if (rec_Recovery < SA_AMF_NO_RECOMMENDATION ||
                                     rec_Recovery > SA_AMF_CLUSTER_RESET) 
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* input validation of Process ID */
   if(processId == 0)
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }  


   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_PM_START, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   
   /* populate & send the passive monitor start message */
   m_AVA_PM_START_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, processId, 
                     desc_TreeDepth, pmErr, rec_Recovery);

   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_PM_START == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_PM_START, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}




/****************************************************************************
  Name          : saAmfPmStop
 
  Description   : This function allows a process to stop an already started
                  passive monitor.
 
  Arguments     : hdl        - AMF handle
                  comp_name  - ptr to the comp name
                  stopQual   - Qualifies which processes should stop being monitored 
                  processId  - Identifier of a process to be monitored
                  pmErr      - type of process errors that the Availability 
                                  Management Framework should stop monitoring

  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfPmStop (SaAmfHandleT hdl,
                                  const SaNameT *comp_name,
                                  SaAmfPmStopQualifierT stopQual,
                                  SaInt64T processId,
                                  SaAmfPmErrorsT pmErr )
                                  
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   /* input validation of component name */
   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* input validation of StopQualifier */
   if(stopQual != SA_AMF_PM_PROC && stopQual != SA_AMF_PM_PROC_AND_DESCENDENTS
              && stopQual != SA_AMF_PM_ALL_PROCESSES)
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }
 
   /* input validation of pmError */
   if( pmErr != SA_AMF_PM_NON_ZERO_EXIT &&  pmErr != SA_AMF_PM_ZERO_EXIT &&
                    pmErr != (SA_AMF_PM_NON_ZERO_EXIT | SA_AMF_PM_ZERO_EXIT))
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* input validation of Process ID */
   if(processId == 0)
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }
   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_PM_STOP, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the Passive Monitoring stop message */
   m_AVA_PM_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, stopQual,
                                                        processId, pmErr );
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_PM_STOP == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_PM_STOP, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}



/****************************************************************************
  Name          : saAmfComponentNameGet
 
  Description   : This function returns the name of the component to which the
                  process belongs.
 
  Arguments     : hdl         - AMF handle
                  o_comp_name - ptr to the comp name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfComponentNameGet(SaAmfHandleT hdl,
                                  SaNameT *o_comp_name)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   SaAisErrorT  rc = SA_AIS_OK;

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !o_comp_name )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   memset(o_comp_name, '\0', sizeof(SaNameT));

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_COMP_NAME_GET, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* fetch the comp name */
   if ( m_AVA_FLAG_IS_COMP_NAME(cb) )
   {
      *o_comp_name = cb->comp_name;
      o_comp_name->length = cb->comp_name.length;
   }
   else
      rc = SA_AIS_ERR_NOT_EXIST;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_COMP_NAME_GET, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);
   return rc;
}


/****************************************************************************
  Name          : saAmfCSIQuiescingComplete
 
  Description   : This functions allows a component to inform the AMF that it
                  has successfully completed its ongoing work and is now idle.
 
  Arguments     : hdl   - AMF handle
                  inv   - invocation value (used to match the corresponding 
                          callback)
                  error - status of the operation
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfCSIQuiescingComplete(SaAmfHandleT hdl,
                                      SaInvocationT inv,
                                      SaAisErrorT error)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVA_PEND_RESP     *list_resp = 0;
   AVA_PEND_CBK_REC *rec = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( (!m_AVA_AMF_RESP_ERR_CODE_IS_VALID(error)) || (!inv) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_CSI_QUIESCING_COMPLETE, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* get the list of pending responses for this handle */
   list_resp = &hdl_rec->pend_resp;
   if(!list_resp)
   {
      rc = SA_AIS_ERR_LIBRARY;
      goto done;
   }

   /* get the pending response rec from list */
   m_AVA_HDL_PEND_RESP_GET(list_resp, rec, inv);
   if(!rec)
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

   /* This API should only be called after a quiescing callback from AMF */
   if(rec->cbk_info->type != AVSV_AMF_CSI_SET || 
         rec->cbk_info->param.csi_set.ha != SA_AMF_HA_QUIESCING)
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

   /* populate & send the 'Quiescing Complete' message */
   m_AVA_CSI_QUIESCING_COMPL_MSG_FILL(msg, cb->ava_dest, hdl, inv, 
                                      error, cb->comp_name);
   rc = ava_mds_send(cb, &msg, 0);
   if (NCSCC_RC_SUCCESS != rc)
      rc = SA_AIS_ERR_TRY_AGAIN;

   /* if we are done with this rec, free it */
   if((rec->cbk_info->type == AVSV_AMF_CSI_SET) && 
                     !m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(rec) )
   {
      m_AVA_HDL_PEND_RESP_POP(list_resp, rec, inv);
      ava_hdl_cbk_rec_del(rec);
   }
   else
   {
      m_AVA_HDL_CBK_RESP_DONE_SET(rec);
   }

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_CSI_QUIESCING_COMPLETE, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);

   /* free the contents of the message */
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfHAStateGet
 
  Description   : This function returns the HA state of the CSI assigned to 
                  the component.
 
  Arguments     : hdl       - AMF handle
                  comp_name - ptr to the comp name
                  csi_name  - ptr to the CSI name
                  o_ha      - ptr to the CSI HA state
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfHAStateGet (SaAmfHandleT hdl,
                             const SaNameT *comp_name,
                             const SaNameT *csi_name,
                             SaAmfHAStateT *o_ha)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) ||
        !csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH) || !o_ha )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_HA_STATE_GET, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the 'ha state get' message */
   m_AVA_HA_STATE_GET_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name, *csi_name);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_HA_STATE_GET == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

   /* return the ha state */
   if (SA_AIS_OK == rc)
      *o_ha = msg_rsp->info.api_resp_info.param.ha_get.ha;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_HA_STATE_GET, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfProtectionGroupTrack
 
  Description   : This fuction requests the AMF to start tracking the changes
                  in the PG associated with the specified CSI.
 
  Arguments     : hdl       - AMF handle
                  csi_name  - ptr to the CSI name
                  flags     - flag that determines when the PG callback is 
                              called
                  buf       - ptr to the linear buffer provided by the 
                              application
                  num       - buffer size
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupTrack(SaAmfHandleT hdl,
                                      const SaNameT *csi_name, 
                                      SaUint8T flags,
                                      SaAmfProtectionGroupNotificationBufferT *buf)
{
   SaAmfProtectionGroupNotificationBufferT *rsp_buf = 0;
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   NCS_BOOL         is_syn = FALSE, create_memory = FALSE;
   SaAisErrorT      rc = SA_AIS_OK;
   uns32            i = 0;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   if (!m_AVA_PG_FLAG_IS_VALID(flags))
   {
      return SA_AIS_ERR_BAD_FLAGS;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_PG_START, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* check if pg cbk was supplied during saAmfInitialize (for change-only & change track) 
      and
      check if track flag is TRACK_CURRENT and neither buffer nor callback is provided */
   if (( ( (flags & SA_TRACK_CHANGES) || (flags & SA_TRACK_CHANGES_ONLY) ) &&
        (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec)) ) ||
      ( (flags & SA_TRACK_CURRENT) && (!buf) &&
        (!m_AVA_HDL_IS_PG_CBK_PRESENT(hdl_rec))))

   {
      rc = SA_AIS_ERR_INIT;
      goto done;
   }


   /* validate the notify buffer */
   if ( (flags & SA_TRACK_CURRENT) && buf && buf->notification )
   {
      if (!buf->numberOfItems)
      {
         rc = SA_AIS_ERR_INVALID_PARAM;
         goto done;
      }
      is_syn = TRUE;
   }

   /* Check whether we have to allocate the memory */
   if ((flags & SA_TRACK_CURRENT) && buf)
   {
     if(buf->notification == NULL)
	 {
       /* This means that we have to allocate the memory. In this case we will ignore buf->numberOfItems */
	   is_syn = TRUE;
	   create_memory = TRUE;

	 }
   }

   /* populate & send the pg start message */
   m_AVA_PG_START_MSG_FILL(msg, cb->ava_dest, hdl, *csi_name, flags, 
                           is_syn);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      /* the resp may contain only the oper result or the curr pg members */
      if ( AVSV_AVND_AMF_CBK_MSG == msg_rsp->type )
      {
         /* => resp msg contains the curr pg mems */
         assert(AVSV_AMF_PG_TRACK == msg_rsp->info.cbk_info->type);

         /* get the err code */
         rc = msg_rsp->info.cbk_info->param.pg_track.err;
         if ( SA_AIS_OK != rc ) goto done;

         /* get the notify buffer from the resp msg */
         rsp_buf = &msg_rsp->info.cbk_info->param.pg_track.buf;
         if(create_memory == FALSE)
		 {
         /* now copy the msg-resp buffer contents to appl provided buffer */
         if (rsp_buf->numberOfItems <= buf->numberOfItems)
         {
            /* user supplied buffer is sufficient.. copy all the members */
            memcpy(buf->notification, rsp_buf->notification, 
               rsp_buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
            buf->numberOfItems = rsp_buf->numberOfItems;

            /* convert comp-name len into host order */
            for (i = 0; i < rsp_buf->numberOfItems; i++)
               buf->notification[i].member.compName.length = 
                  buf->notification[i].member.compName.length;
         }
         else
         {
            /* user supplied buffer isnt sufficient.. copy whatever is possible */
            memcpy(buf->notification, rsp_buf->notification, 
               buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
            rc = SA_AIS_ERR_NO_SPACE;

            /* convert comp-name len into host order */
            for (i = 0; i < buf->numberOfItems; i++)
               buf->notification[i].member.compName.length = 
                  buf->notification[i].member.compName.length;
         }
		 }
		 else /* if(create_memory == FALSE) */
		 {
           /* create_momory is TRUE, so let us create the memory for the Use, User has to free it. */
           buf->numberOfItems = rsp_buf->numberOfItems;
		   if(buf->numberOfItems != 0)
		   {
		     buf->notification = malloc(buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));
			 if(buf->notification != NULL)
			 {
                  memcpy(buf->notification, rsp_buf->notification, 
                     buf->numberOfItems * sizeof(SaAmfProtectionGroupNotificationT));

                  /* convert comp-name len into host order */
                  for (i = 0; i < buf->numberOfItems; i++)
                     buf->notification[i].member.compName.length = 
                        buf->notification[i].member.compName.length;
			 }
			 else
			 {
                  rc = SA_AIS_ERR_NO_MEMORY;
                  buf->numberOfItems = 0; 
			 }
		   }
		   else /* if(buf->numberOfItems != 0) */
		   {
              /* buf->numberOfItems is zero. Nothing to be done. */

		   }

		 } /* else of if(create_memory == FALSE) */
      }
      else
      {
         /* => it's a regular resp msg */
         assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
         assert(AVSV_AMF_PG_START == msg_rsp->info.api_resp_info.type);
         rc = msg_rsp->info.api_resp_info.rc;
      }
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_PG_START, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfProtectionGroupTrackStop
 
  Description   : This fuction requests the AMF to stop tracking the changes
                  in the PG associated with the CSI.
 
  Arguments     : hdl       - AMF handle
                  csi_name  - ptr to the CSI name
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfProtectionGroupTrackStop (SaAmfHandleT hdl, 
                                           const SaNameT *csi_name)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !csi_name || !(csi_name->length) || (csi_name->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_PG_STOP, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the pg stop message */
   m_AVA_PG_STOP_MSG_FILL(msg, cb->ava_dest, hdl, *csi_name);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_PG_STOP == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_PG_STOP, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfComponentErrorReport
 
  Description   : This function reports an error and a recovery recommendation
                  to the AMF.
 
  Arguments     : hdl       - AMF handle
                  err_comp - ptr to the erroneous comp name
                  err_time - error detection time
                  rec_rcvr - recommended recovery
                  ntf_id   - notification identifier sent to SysMan
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT saAmfComponentErrorReport(SaAmfHandleT hdl,
                                      const SaNameT *err_comp,
                                      SaTimeT err_time, 
                                      SaAmfRecommendedRecoveryT rec_rcvr,
                                      SaNtfIdentifierT ntf_id)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   /* validate the component */
   if ( !err_comp || !(err_comp->length) || (err_comp->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* validate the recommended recovery */
   if( rec_rcvr < SA_AMF_NO_RECOMMENDATION || rec_rcvr > SA_AMF_CLUSTER_RESET )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_ERR_REP, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the 'error report' message */
   m_AVA_ERR_REP_MSG_FILL(msg, cb->ava_dest, hdl, *err_comp, 
                          err_time, rec_rcvr);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_ERR_REP == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_ERR_REP, gl_ava_hdl, 
                             cb, hdl, hdl_rec, err_comp, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfComponentErrorClear
 
  Description   : This function clears the previous errors reported about 
                  the component.
 
  Arguments     : hdl       - AMF handle
                  comp_name - ptr to the comp name
                  ntf_id    - notification identifier sent to SysMan
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : Notification Identifier is currently not used.
******************************************************************************/
SaAisErrorT 
saAmfComponentErrorClear(SaAmfHandleT hdl,
                         const SaNameT *comp_name,
                         SaNtfIdentifierT ntf_id)
{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVSV_NDA_AVA_MSG *msg_rsp = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( !comp_name || !(comp_name->length) || (comp_name->length > SA_MAX_NAME_LENGTH) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_ERR_CLEAR, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* populate & send the 'error clear' message */
   m_AVA_ERR_CLEAR_MSG_FILL(msg, cb->ava_dest, hdl, *comp_name);
   rc = ava_mds_send(cb, &msg, &msg_rsp);
   if (NCSCC_RC_SUCCESS == rc)
   {
      assert(AVSV_AVND_AMF_API_RESP_MSG == msg_rsp->type);
      assert(AVSV_AMF_ERR_CLEAR == msg_rsp->info.api_resp_info.type);
      rc = msg_rsp->info.api_resp_info.rc;
   }
   else if (NCSCC_RC_FAILURE == rc)
      rc = SA_AIS_ERR_TRY_AGAIN;
   else if (NCSCC_RC_REQ_TIMOUT == rc)
      rc = SA_AIS_ERR_TIMEOUT;

done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_ERR_CLEAR, gl_ava_hdl, 
                             cb, hdl, hdl_rec, comp_name, rc);

   /* free the contents of the request/response message */
   if (msg_rsp) avsv_nda_ava_msg_free(msg_rsp);
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}


/****************************************************************************
  Name          : saAmfResponse
 
  Description   : The component responds to the AMF with the result of its 
                  execution of a particular AMF request.
 
  Arguments     : hdl   - AMF handle
                  inv   - invocation value (used to match the corresponding 
                          callback)
                  error - status of the operation
 
  Return Values : Refer to SAI-AIS specification for various return values.
 
  Notes         : None.
******************************************************************************/
SaAisErrorT saAmfResponse (SaAmfHandleT hdl,
                           SaInvocationT inv,
                           SaAisErrorT error)

{
   AVA_CB       *cb = 0;
   AVA_HDL_REC  *hdl_rec = 0;
   AVSV_NDA_AVA_MSG msg;
   AVA_PEND_RESP     *list_resp = 0;
   AVA_PEND_CBK_REC *rec = 0;
   SaAisErrorT     rc = SA_AIS_OK;

   /* initialize the msg */
   memset(&msg, 0, sizeof(AVSV_NDA_AVA_MSG));

   /* verify CB-hdl & input hdl  */
   m_AVA_API_HDL_VERIFY(gl_ava_hdl, hdl, rc);
   if(SA_AIS_OK != rc) return rc;

   if ( (!m_AVA_AMF_RESP_ERR_CODE_IS_VALID(error)) || (!inv) )
   {
      return SA_AIS_ERR_INVALID_PARAM;
   }

   /* api pre-processing */
   m_AVA_API_PRE_PROCESSING(AVSV_AMF_RESP, gl_ava_hdl, 
                            cb, hdl, hdl_rec, rc);
   if ( SA_AIS_OK != rc ) goto done;

   /* get the list of pending responses for this handle */
   list_resp = &hdl_rec->pend_resp;
   if(!list_resp)
   {
      rc = SA_AIS_ERR_LIBRARY;
      goto done;
   }

   /* get the pending response rec from list */
   m_AVA_HDL_PEND_RESP_GET(list_resp, rec, inv);
   if(!rec)
   {
      rc = SA_AIS_ERR_INVALID_PARAM;
      goto done;
   }

   /* populate & send the 'AMF response' message */
   m_AVA_AMF_RESP_MSG_FILL(msg, cb->ava_dest, hdl, inv, error, cb->comp_name);
   rc = ava_mds_send(cb, &msg, 0);
   if (NCSCC_RC_SUCCESS != rc)
      rc = SA_AIS_ERR_TRY_AGAIN;

   /* if we are done with this rec, free it */
   if((rec->cbk_info->type != AVSV_AMF_PG_TRACK) && 
                     !m_AVA_HDL_IS_CBK_REC_IN_DISPATCH(rec))
   {
      /* In case of Quiescing callback dont free the rec,
         free it after app makes a QuiescingComplete */
      if(rec->cbk_info->type == AVSV_AMF_CSI_SET && 
            rec->cbk_info->param.csi_set.ha == SA_AMF_HA_QUIESCING)
         goto done;

      m_AVA_HDL_PEND_RESP_POP(list_resp, rec, inv);
      ava_hdl_cbk_rec_del(rec);
   }
   else
   {
      /* In case of Quiescing callback dont set the response done */
      if(rec->cbk_info->type == AVSV_AMF_CSI_SET &&
            rec->cbk_info->param.csi_set.ha == SA_AMF_HA_QUIESCING)
         goto done;

      m_AVA_HDL_CBK_RESP_DONE_SET(rec);
   }


done:
   /* api post processing */
   m_AVA_API_POST_PROCESSING(AVSV_AMF_RESP, gl_ava_hdl, 
                             cb, hdl, hdl_rec, 0, rc);

   /* free the contents of the request message */
   avsv_nda_ava_msg_content_free(&msg);

   return rc;
}
