/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
*                                                                            *
*  MODULE NAME:  plms_hrb_mds.c                                              *
*                                                                            *
*                                                                            *
*  DESCRIPTION: This module deals wih registering PLMS_HRB with MDS	     *
*  		and provides the callbacks for MDS                           *
*                                                                            *
*****************************************************************************/

/***********************************************************************
 *   INCLUDE FILES
***********************************************************************/
#include "base/ncsgl_defs.h"
#include "plms.h"
#include "plms_hrb.h"


/***********************************************************************
*   FUNCTION PROTOTYPES
***********************************************************************/
SaUint32T hrb_mds_initialize();
SaUint32T hrb_mds_finalize();
SaUint32T hrb_mds_register();
SaUint32T hrb_mds_msg_send(PLMS_HPI_RSP *response, MDS_SYNC_SND_CTXT context);
SaUint32T plms_hrb_mds_msg_sync_send(MDS_HDL mds_hdl,
                SaUint32T from_svc,SaUint32T to_svc,
                MDS_DEST    to_dest,PLMS_HPI_REQ *i_evt,
		PLMS_HPI_RSP **o_evt,SaTimeT timeout);

/* MDS callback function declaration */
SaUint32T hrb_mds_callback (struct ncsmds_callback_info *info);
SaUint32T hrb_mds_cpy(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_enc(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_dec(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_enc_flat(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_dec_flat(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_rcv(struct ncsmds_callback_info *info);
SaUint32T hrb_mds_svc_evt(struct ncsmds_callback_info *info);


/****************************************************************************
Name          : hrb_mds_initialize

Description   : This routine registers the HRB to MDS and subscribes for  
		PLMS events throgh MDS

Arguments     : 

Return Values : NCSCC_RC_SUCCESS
		NCSCC_RC_FAILURE

Notes         : 
******************************************************************************/
SaUint32T hrb_mds_initialize()
{
	PLMS_HRB_CB 	*cb = hrb_cb;
	NCSMDS_INFO     mds_info;
	MDS_SVC_ID      plms_svc_id[] = { NCSMDS_SVC_ID_PLMS };
	SaUint32T	rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	if(NCSCC_RC_SUCCESS != (rc = hrb_mds_register()))
	{
		LOG_ER("HRB:MDS registration failed error:%d",rc);
		return rc;
	}


	/* subscribe for PLMS events through MDS */
	memset(&mds_info,'\0',sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_PLMS_HRB;
	mds_info.i_op = MDS_SUBSCRIBE;

	mds_info.info.svc_subscribe.i_num_svcs = 1;
	mds_info.info.svc_subscribe.i_scope = NCSMDS_SCOPE_NONE;
	mds_info.info.svc_subscribe.i_svc_ids = plms_svc_id;

	/* register to MDS */
	if(NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info)))
	{
		LOG_ER("HRB:MDS subscription failed ret value is:%d",rc);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("HRB:MDS initialization success");

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
/****************************************************************************
Name          : hrb_mds_register

Description   : This routine registers HRB with MDS.

Arguments     : 

Return Values : NCSCC_RC_SUCCESS
		NCSCC_RC_FAILURE

Notes         : 
******************************************************************************/
SaUint32T hrb_mds_register()
{
	PLMS_HRB_CB 	  *cb = hrb_cb;
	NCSADA_INFO  	  ada_info;
	NCSMDS_INFO       mds_info;
	SaUint32T	  rc;

	/* Create the ADEST for HRB and get the hdl */
	memset(&ada_info, 0, sizeof(ada_info));

	ada_info.req = NCSADA_GET_HDLS;

	if ( NCSCC_RC_SUCCESS != (rc = ncsada_api(&ada_info)))
	{
		LOG_ER("HRB:NCSADA_GET_HDLS failed with error:%d",rc);
		return NCSCC_RC_FAILURE;
	}

	/* Store the info obtained from MDS ADEST creation */
	cb->mds_hdl   = ada_info.info.adest_get_hdls.o_mds_pwe1_hdl;
	cb->mdest     = ada_info.info.adest_get_hdls.o_adest;



	/* Install with MDS */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id  = NCSMDS_SVC_ID_PLMS_HRB;
	mds_info.i_op 	   = MDS_INSTALL;
	mds_info.info.svc_install.i_install_scope =  NCSMDS_SCOPE_NONE;
	mds_info.info.svc_install.i_svc_cb = hrb_mds_callback;
	mds_info.info.svc_install.i_mds_q_ownership = false;
	mds_info.info.svc_install.i_mds_svc_pvt_ver = PLMS_MDS_SUB_PART_VERSION;

	if ( NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info)))
	{
		LOG_ER("HRB:MDS_INSTALL failed with error:%d",rc);
		return rc;
	}

	LOG_NO("HRB:MDS registration success");
	return NCSCC_RC_SUCCESS;

}
/****************************************************************************
* Name          : hrb_mds_callback
*
* Description   : Callback Dispatcher for various MDS operations.
*
* Arguments     : clbk_info(input) 
*
* Return Values : NCSCC_RC_SUCCESS
*                 NCSCC_RC_FAILURE
*		  Error Code
* Notes         : None.
*****************************************************************************/
SaUint32T hrb_mds_callback(struct ncsmds_callback_info *clbk_info)
{
	NCSMDS_CALLBACK_API clbk_opr_set[MDS_CALLBACK_SVC_MAX] = {
		hrb_mds_cpy,      /* MDS_CALLBACK_COPY      */
		hrb_mds_enc,      /* MDS_CALLBACK_ENC       */
		hrb_mds_dec,      /* MDS_CALLBACK_DEC       */
		hrb_mds_enc_flat, /* MDS_CALLBACK_ENC_FLAT  */
		hrb_mds_dec_flat, /* MDS_CALLBACK_DEC_FLAT  */
		hrb_mds_rcv,      /* MDS_CALLBACK_RECEIVE   */
		hrb_mds_svc_evt   /* MDS_CALLBACK_SVC_EVENT */
		};
	
	if((MDS_CALLBACK_COPY <= clbk_info->i_op) &&
	(MDS_CALLBACK_QUIESCED_ACK >= clbk_info->i_op)){
		return (*clbk_opr_set[clbk_info->i_op])(clbk_info);
	}else{
		LOG_ER("HRB:MDS Invalid operation");
		return NCSCC_RC_FAILURE;
	}


}
/***********************************************************************
* Name          : hrb_mds_cpy
*
* Description   : This function copies an events received.
*
* Arguments     : clbk_info(input) 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
**********************************************************************/
SaUint32T hrb_mds_cpy(struct ncsmds_callback_info *clbk_info)
{
	PLMS_HPI_RSP	*dst_msg = NULL;
	TRACE_ENTER();

	dst_msg    = (PLMS_HPI_RSP *)clbk_info->info.cpy.i_msg;
   	clbk_info->info.cpy.o_cpy = (uint8_t*)dst_msg;

	/* TBD need to see whether we need to allocate memory for data*/
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

SaUint32T hrb_mds_enc(struct ncsmds_callback_info *info)
{
	   LOG_ER("HRB: MDS not supposed to Invoke enc func");
	   return NCSCC_RC_FAILURE;
}

SaUint32T hrb_mds_dec(struct ncsmds_callback_info *info)
{
	  LOG_ER("HRB:MDS not suppoded to invoke dec func");
	  return NCSCC_RC_FAILURE;
}

SaUint32T hrb_mds_enc_flat(struct ncsmds_callback_info *info)
{
	   LOG_ER("HRB:MDS not supposed to invoke enc flat func");
	   return NCSCC_RC_FAILURE;
}

SaUint32T hrb_mds_dec_flat(struct ncsmds_callback_info *info)
{
	   LOG_ER("HRB:MDS not supposed to invoke dec flat func");
	   return NCSCC_RC_FAILURE;
}

SaUint32T hrb_mds_sys_evt(struct ncsmds_callback_info *info)
{
	   LOG_ER("HRB:MDS not supposed to invoke sys evt func");
	   return NCSCC_RC_FAILURE;

}

/**********************************************************************
* Name          : hrb_mds_rcv
*
* Description   : This is a callback routine that is invoked when msg
*		  is received at HRB. This function simply posts the 
*                 mesage to mailbox .
*
* Arguments     : pointer to struct ncsmds_callback_info
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
SaUint32T hrb_mds_rcv(struct ncsmds_callback_info *mds_cb_info)
{
	PLMS_HRB_CB *cb = hrb_cb;
	PLMS_HPI_REQ   *hpi_req = NULL;
	SaUint32T      rc = NCSCC_RC_SUCCESS;
	
	TRACE_ENTER();

	if(NULL == mds_cb_info->info.receive.i_msg){
		LOG_ER("HRB:hrb_mds_rcv invalid value");
		return NCSCC_RC_FAILURE;
	}


	if ((mds_cb_info->info.receive.i_fr_svc_id == NCSMDS_SVC_ID_PLMS) &&
	(mds_cb_info->info.receive.i_to_svc_id==NCSMDS_SVC_ID_PLMS_HRB)){

		hpi_req = (PLMS_HPI_REQ *)mds_cb_info->info.receive.i_msg; 
		hpi_req->mds_context = mds_cb_info->info.receive.i_msg_ctxt;

		if(hpi_req->cmd > PLMS_HPI_CMD_BASE && 
			hpi_req->cmd < PLMS_HPI_CMD_MAX )
		{
			/* put the event in the mail box */
			if(m_NCS_IPC_SEND(&cb->mbx, hpi_req, 
				   NCS_IPC_PRIORITY_HIGH)== NCSCC_RC_FAILURE)
			{
				LOG_ER("HRB:m_NCS_IPC_SEND failed"); 
				rc = NCSCC_RC_FAILURE;
			}
		}

	}
	else{
		LOG_ER("HRB:hrb_mds_rcv from invalid service");
	        rc = NCSCC_RC_FAILURE;
	}


	TRACE_LEAVE();
	return rc;
}
		
/***********************************************************************
* Name          : hrb_mds_svc_event
*
* Description   : 
* Arguments     : 
*
* Return Values : NCSCC_RC_SUCCESS
*		  NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
SaUint32T hrb_mds_svc_evt (struct ncsmds_callback_info *mds_cb_info)
{
	PLMS_HRB_CB     *cb = hrb_cb; 

	/* make sure that this event is indeed for us*/
	if (mds_cb_info->info.svc_evt.i_svc_id != NCSMDS_SVC_ID_PLMS)
	{
		LOG_ER("HRB:hrb_mds_svc_event invalid event from svc:%d",
				mds_cb_info->info.svc_evt.i_your_id);
		return NCSCC_RC_SUCCESS;
	}

	switch (mds_cb_info->info.svc_evt.i_change)
	{
		case NCSMDS_DOWN:
			LOG_ER("HRB:MDS down received from PLMS ");
			cb->plms_up = false;
			memset(&cb->plms_mdest, 0, sizeof(MDS_DEST));
			break;
			
		case NCSMDS_UP:
			LOG_ER("HRB:MDS up received from PLMS ");
			cb->plms_mdest = mds_cb_info->info.svc_evt.i_dest;
			cb->plms_up = true;
			break;
		default:
			break;
	}
	return NCSCC_RC_SUCCESS;

}

/****************************************************************************
* Name          : hrb_mds_msg_send
*
* Description   : 
*
* Arguments     : cb  - ptr to the HRB CB
*                 response - 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
*****************************************************************************/
SaUint32T hrb_mds_msg_send(PLMS_HPI_RSP *response, MDS_SYNC_SND_CTXT context)
{
	PLMS_HRB_CB     *cb = hrb_cb; 
	NCSMDS_INFO     mds_info;
	SaUint32T	rc;

	if(NULL == response)
		return NCSCC_RC_FAILURE;

	/* populate the mds params */
	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = cb->mds_hdl;
	mds_info.i_svc_id = NCSMDS_SVC_ID_PLMS_HRB;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)response;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = NCSMDS_SVC_ID_PLMS;
	//mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SND;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_RSP;
	mds_info.info.svc_send.info.rsp.i_msg_ctxt = context;
	mds_info.info.svc_send.info.rsp.i_sender_dest = cb->plms_mdest;


	/* send the message */
	if( NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info)))
	{
		LOG_ER("HRB:MDS_SEND failed with error:%d",rc);
		return NCSCC_RC_FAILURE;
	}
	return NCSCC_RC_SUCCESS;

}
/****************************************************************************
* Name          : hrb_mds_finalize
*
* Description   : This routine unregisters the HRB from MDS.
*
* Arguments     : cb 
*
* Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
*
* Notes         : None.
******************************************************************************/
SaUint32T hrb_mds_finalize()
{
	PLMS_HRB_CB 	*cb = hrb_cb;
	NCSMDS_INFO     mds_info;
	SaUint32T       rc;
	
	/* Un-install service from MDS
	No need to cancel the services that are subscribed*/
	memset(&mds_info,'\0',sizeof(NCSMDS_INFO));

	mds_info.i_mds_hdl     = cb->mds_hdl;
	mds_info.i_svc_id      = NCSMDS_SVC_ID_PLMS_HRB;
	mds_info.i_op          = MDS_UNINSTALL;

	if ( NCSCC_RC_SUCCESS != (rc = ncsmds_api(&mds_info))){
		LOG_ER("HRB:MDS_UNINSTALL failed with error:%d",rc);
		return NCSCC_RC_FAILURE;
	}

	LOG_NO("HRB:hrb_mds_finalize success");
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : plms_hrb_mds_msg_sync_send 

  Description   : This routine sends the mds sync msgs from/to PLM/PLMS

  Arguments     : i_evt - ptr to the message to be sent
                  o_evt - ptr to the response msg
                  timeout - timeout value in 10 ms

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
SaUint32T plms_hrb_mds_msg_sync_send(MDS_HDL mds_hdl,
                              SaUint32T	from_svc,
                              SaUint32T	to_svc,
                              MDS_DEST    to_dest,
			      PLMS_HPI_REQ *i_evt,
			      PLMS_HPI_RSP **o_evt,
                              SaTimeT timeout)
{
	NCSMDS_INFO                mds_info;
	PLMS_CB			   *cb = plms_cb; 
	SaUint32T		   rc;
	PLMS_HPI_RSP 		   *response = NULL;

	
	TRACE_ENTER();
	if(!i_evt){
		LOG_ER("Invalid hpi_req");
		return NCSCC_RC_FAILURE;
	}

	/* If not HPI supported system return Success without processing */
	if(!cb->hpi_cfg.hpi_support){
		response = (PLMS_HPI_RSP *)malloc(sizeof(PLMS_HPI_RSP));
		if(NULL == response){
			LOG_ER("Memory alloc failed error:%s",strerror(errno));
			assert(0);
		}
		memset(response,0,sizeof(PLMS_HPI_RSP));
		
		/* Populate the response message */
		response->ret_val = NCSCC_RC_SUCCESS;
		response->data_len = 0;
		response->data = NULL;
		*o_evt= response;
		return NCSCC_RC_SUCCESS;
	}

	memset(&mds_info, 0, sizeof(NCSMDS_INFO));
	mds_info.i_mds_hdl = mds_hdl;
	mds_info.i_svc_id = from_svc;
	mds_info.i_op = MDS_SEND;

	/* fill the send structure */
	mds_info.info.svc_send.i_msg = (NCSCONTEXT)i_evt;
	mds_info.info.svc_send.i_priority = MDS_SEND_PRIORITY_MEDIUM;
	mds_info.info.svc_send.i_to_svc = to_svc;
	mds_info.info.svc_send.i_sendtype = MDS_SENDTYPE_SNDRSP;

	/* fill the send rsp strcuture */
	mds_info.info.svc_send.info.sndrsp.i_time_to_wait = timeout;
	mds_info.info.svc_send.info.sndrsp.i_to_dest = to_dest;

	/* send the message */
	rc = ncsmds_api(&mds_info);
	if ( rc == NCSCC_RC_SUCCESS)
		*o_evt=(PLMS_HPI_RSP *)mds_info.info.svc_send.info.sndrsp.o_rsp; 

	TRACE_LEAVE2("ret val:%d",rc);
	return rc;
}
