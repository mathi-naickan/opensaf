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

  FILE NAME: mqd_mbcsv.c

............................................................................................
  DESCRIPTION: This file contains the following routines :
 mqd_a2s_async_update...............active to standby async message update 
 mqd_a2s_async_reg_info.............fills the register message contents into async message
 mqd_a2s_async_dereg_info...........fills the deregister message contents into async message
 mqd_a2s_async_track_info...........fills the track info message contents into async message
 mqd_a2s_async_userevent_info.......fills the user event message contents into async message
 mqd_mbcsv_register................ innitializes mbcsv session and opens checkpoint and does selection object
 mqd_mbcsv_init....................initializes the mbcsv instance
 mqd_mbcsv_open....................Opens a checkpoint with mbcsv
 mqd_mbcsv_finalize................to close the mbcsv service instance
 mqd_mbcsv_selobj_get..............to do the selection object on mbcsv session
 mqd_mbcsv_chg_role................to set the role of mbcsv instance depending on hastate
 mqd_mbcsv_async_update............to process the async update from active mqd to standby mqd
 mqd_mbcsv_callback................call back function to process mbcsv callbacks
  
*******************************************************************************************/
#include "mqd.h"

static void mqd_queue_db_clean_up(MQD_CB *pMqd);
static void mqd_a2s_mqnd_timer_exp_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static void mqd_a2s_async_mqnd_stat_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static void mqd_a2s_async_userevent_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static void mqd_a2s_async_track_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static void mqd_a2s_async_dereg_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static void mqd_a2s_async_reg_info(MQD_A2S_MSG *pasync_msg, void *pmesg);
static uint32_t mqd_mbcsv_open(MQD_CB *pMqd);
static uint32_t mqd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_mbcsv_init(MQD_CB *pMqd);
static uint32_t mqd_mbcsv_selobj_get(MQD_CB *pMqd);
static uint32_t mqd_mbcsv_async_update(MQD_CB *pMqd, MQD_A2S_MSG *pasync_msg);
static uint32_t mqd_ckpt_encode_async_update(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_ckpt_decode_async_update(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_mbcsv_ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_mbcsv_ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_ckpt_encode_warm_sync_response(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_ckpt_decode_warm_sync_response(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_ckpt_encode_cold_sync_data(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_copy_data_to_cold_sync_structure(MQD_OBJ_INFO *obj_info, MQD_A2S_QUEUE_INFO *mbcsv_info);
static uint32_t mqd_ckpt_decode_cold_sync_resp(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg);
static uint32_t mqd_a2s_make_record_from_coldsync(MQD_CB *pMqd, MQD_A2S_QUEUE_INFO q_data_msg);
static uint32_t mqd_db_del_all_records(MQD_CB *pMqd);

extern  MQDLIB_INFO gl_mqdinfo;

/****************************************************************************************** 
 *  Name             : mqd_a2s_async_update
 *
 *  Description      : Function to do the active to standby mqd async message update
 *
 *  Parameters       : MQD_CB * pMqd         -- MQD Control Block
 *                     MQD_A2S_MSG_TYPE type --Active to Standby message type
 *                     void *pmesg            --Active to Standby message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 ******************************************************************************************/
void mqd_a2s_async_update(MQD_CB *pMqd, MQD_A2S_MSG_TYPE type, void *pmesg)
{
	MQD_A2S_MSG async_msg;
	memset(&async_msg, 0, sizeof(MQD_A2S_MSG));
	async_msg.type = type;
	switch (type) {
	case MQD_A2S_MSG_TYPE_REG:
		mqd_a2s_async_reg_info(&async_msg, pmesg);
		break;
	case MQD_A2S_MSG_TYPE_DEREG:
		mqd_a2s_async_dereg_info(&async_msg, pmesg);
		break;
	case MQD_A2S_MSG_TYPE_TRACK:
		mqd_a2s_async_track_info(&async_msg, pmesg);
		break;
	case MQD_A2S_MSG_TYPE_USEREVT:
		mqd_a2s_async_userevent_info(&async_msg, pmesg);
		break;
	case MQD_A2S_MSG_TYPE_MQND_STATEVT:
		mqd_a2s_async_mqnd_stat_info(&async_msg, pmesg);
		break;
	case MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT:
		mqd_a2s_mqnd_timer_exp_info(&async_msg, pmesg);
		break;
	default:
		LOG_ER("Bad Active to Standby Message Type");
	}

/* Send the aync_msg to the standby MQD using the MBCSv_SEND function */

	mqd_mbcsv_async_update(pMqd, &async_msg);
}

/******************************************************************************************* 
 *  Name             : mqd_a2s_async_reg_info
 *
 *  Description      : Function to fill the register info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- resister message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 *******************************************************************************************/
static void mqd_a2s_async_reg_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.reg from the register message *mesg */
	memcpy(&pasync_msg->info.reg, (ASAPi_REG_INFO *)pmesg, sizeof(ASAPi_REG_INFO));
	return;
}

/******************************************************************************************* 
 *  Name             : mqd_a2s_async_dereg_info
 *
 *  Description      : Function to fill the deregister info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- deresister message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 *******************************************************************************************/
static void mqd_a2s_async_dereg_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.dereg from the deregister message *mesg */
	memcpy(&pasync_msg->info.dereg, (ASAPi_DEREG_INFO *)pmesg, sizeof(ASAPi_DEREG_INFO));
	return;

}

/******************************************************************************************* 
 *  Name             : mqd_a2s_async_track_info
 *
 *  Description      : Function to fill the track info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- track message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 ******************************************************************************************/
static void mqd_a2s_async_track_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.track from the track message *mesg */
	/*pasync_msg->track=(MQD_A2S_TRACK_INFO) (*pmesg);     */
	memcpy(&pasync_msg->info.track, (MQD_A2S_TRACK_INFO *)pmesg, sizeof(MQD_A2S_TRACK_INFO));
	return;

}

/******************************************************************************************* 
 *  Name             : mqd_a2s_async_userevent_info
 *
 *  Description      : Function to fill the user event info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- user event message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 ******************************************************************************************/
static void mqd_a2s_async_userevent_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.user_evt from the user event message *mesg */
	memcpy(&pasync_msg->info.user_evt, (MQD_A2S_USER_EVENT_INFO *)pmesg, sizeof(MQD_A2S_USER_EVENT_INFO));
	return;
}

/******************************************************************************************* 
 *  Name             : mqd_a2s_async_mqnd_stat_info
 *
 *  Description      : Function to fill the MQND Status event info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- MQD_ND_STAT_IFNO event message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 ******************************************************************************************/
static void mqd_a2s_async_mqnd_stat_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.user_evt from the user event message *mesg */
	memcpy(&pasync_msg->info.nd_stat_evt, (MQD_A2S_ND_STAT_INFO *)pmesg, sizeof(MQD_A2S_ND_STAT_INFO));
	return;
}

/******************************************************************************************* 
 *  Name             : mqd_a2s_mqnd_timer_exp_info
 *
 *  Description      : Function to fill the MQND Timer expiry info to the async message
 *
 *  Parameters       : MQD_A2S_MSG *pasyn_msg -- MQD_A2S_MSG async message to standby MQD
 *                     void *pmesg            -- MQND Timer expiry message
 *
 *  Return values    : None
 *
 *  Notes            : None
 *
 ******************************************************************************************/
static void mqd_a2s_mqnd_timer_exp_info(MQD_A2S_MSG *pasync_msg, void *pmesg)
{
	/* Fill the async_msg.info.user_evt from the user event message *mesg */
	memcpy(&pasync_msg->info.nd_tmr_exp_evt, (MQD_A2S_ND_TIMER_EXP_INFO *)pmesg, sizeof(MQD_A2S_ND_TIMER_EXP_INFO));
	return;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_register
 *
 * Description            : Function to initialze the mbcsv instance and
 *                          open a session and select 
 *
 * Parameters             : MQD_CB : pMqd pointer
 *
 * Return Values          :
 *
 * Notes                  : None
********************************************************************************
*****************/
uint32_t mqd_mbcsv_register(MQD_CB *pMqd)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	rc = mqd_mbcsv_init(pMqd);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;

	rc = mqd_mbcsv_open(pMqd);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;
	/* Selection object get */
	rc = mqd_mbcsv_selobj_get(pMqd);
	if (rc != NCSCC_RC_SUCCESS)
		return rc;
	else
		return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_init
 *
 * Description            : Function to initialze the mbcsv instance.
 *
 * Parameters             : MQD_CB : pMqd pointer
 *
 * Return Values          :
 *
 * Notes                  : None
********************************************************************************
*****************/
static uint32_t mqd_mbcsv_init(MQD_CB *pMqd)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_INITIALIZE;
	arg.info.initialize.i_mbcsv_cb = mqd_mbcsv_callback;
	arg.info.initialize.i_version = MQSV_MQD_MBCSV_VERSION;
	arg.info.initialize.i_service = NCS_SERVICE_ID_CPD;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("MBCSV initialization Failed");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	pMqd->mbcsv_hdl = arg.info.initialize.o_mbcsv_hdl;
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_open
 *
 * Description            : To open a chekpoint session 
 *
 * Parameters             : pMqd: MQD_CB pointer
 *
 * Return Values          :
 *
 * Notes                  : None
********************************************************************************
*****************/
static uint32_t mqd_mbcsv_open(MQD_CB *pMqd)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	arg.i_op = NCS_MBCSV_OP_OPEN;
	arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;

	arg.info.open.i_pwe_hdl = (uint32_t)pMqd->my_mds_hdl;
	arg.info.open.i_client_hdl = pMqd->hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("MBCSV Open Failed");
		rc = NCSCC_RC_FAILURE;
		mqd_mbcsv_finalize(pMqd);
		return rc;
	}
	pMqd->o_ckpt_hdl = arg.info.open.o_ckpt_hdl;
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_finalize
 *
 * Description            : To close the servise instance of MBCSV 
 *
 * Parameters             : pMqd: MQD_CB pointer
 *
 * Return Values          :
 *
 * Notes                  : Closes the association, represented by i_mbc_hdl, between th
e invoking process and MBCSV
********************************************************************************
*****************/
uint32_t mqd_mbcsv_finalize(MQD_CB *pMqd)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	arg.i_op = NCS_MBCSV_OP_FINALIZE;

	arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("MBCSV Finalize Failed");
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_selobj_get
 *
 * Description            : 
 *
 * Parameters             : pMqd: MQD_CB pointer
 *
 * Return Values          :
 *
 * Notes                  : 
 *******************************************************************************
*****************/

static uint32_t mqd_mbcsv_selobj_get(MQD_CB *pMqd)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));
	arg.i_op = NCS_MBCSV_OP_SEL_OBJ_GET;
	arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;
	if (ncs_mbcsv_svc(&arg) != NCSCC_RC_SUCCESS) {
		LOG_ER("MBCSV SelObjGet Failed");
		rc = NCSCC_RC_FAILURE;
	}
	pMqd->mbcsv_sel_obj = arg.info.sel_obj_get.o_select_obj;
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_chg_role
 *
 * Description            : Set the role of MBCSV instance depending on
 *                          hastate of the client
 *
 * Parameters             : pMqd: MQD_CB pointer
 *
 * Return Values          :
 *
 * Notes                  : 
 *******************************************************************************
*****************/
uint32_t mqd_mbcsv_chgrole(MQD_CB *pMqd)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_CHG_ROLE;
	arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;
	arg.info.chg_role.i_ckpt_hdl = pMqd->o_ckpt_hdl;
	arg.info.chg_role.i_ha_state = pMqd->ha_state;	/*  ha_state is assigned
							   at the time of amf_init where csi_set_callback will assign the state */

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("MBCSV ChangeRole Failed");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	TRACE_1("MBCSV ChangeRole Success with HA_STATE %d", pMqd->ha_state);
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_async_update
 *
 * Description            : To process the async update from active mqd 
 *                          to standby.
 *
 * Parameters             : pMqd: MQD_CB pointer
 *                          pasync_msg: Active to standby message pointer
 * Return Values          :
 *
 * Notes                  : 
 *******************************************************************************
*****************/
static uint32_t mqd_mbcsv_async_update(MQD_CB *pMqd, MQD_A2S_MSG *pasync_msg)
{
	NCS_MBCSV_ARG arg;
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();

	memset(&arg, '\0', sizeof(NCS_MBCSV_ARG));

	arg.i_op = NCS_MBCSV_OP_SEND_CKPT;
	arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;
	arg.info.send_ckpt.i_ckpt_hdl = pMqd->o_ckpt_hdl;
	/* arg.info.send_ckpt.i_send_type= NCS_MBCSV_SND_USR_ASYNC; */
	arg.info.send_ckpt.i_send_type = NCS_MBCSV_SND_SYNC;
	arg.info.send_ckpt.i_reo_type = pasync_msg->type;
	arg.info.send_ckpt.i_reo_hdl = NCS_PTR_TO_UNS64_CAST(pasync_msg);
	arg.info.send_ckpt.i_action = NCS_MBCSV_ACT_UPDATE;

	if (ncs_mbcsv_svc(&arg) != SA_AIS_OK) {
		LOG_ER("MBCSV AsyncUpdate Failure for type %d", pasync_msg->type);
		rc = NCSCC_RC_FAILURE;
	}
	TRACE_1("MBCSV AsyncUpdate Success for type %d", pasync_msg->type);
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_callback
 *
 * Description            : Call back function to process the callbacks for 
 *                          encode,decode,notify,peerinfo,errorind.
 *
 * Parameters             : arg: NCS_MBCSV_CB_ARG pointer
 *
 * Return Values          :
 *
 * Notes                  : None
********************************************************************************
*****************/
static uint32_t mqd_mbcsv_callback(NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	
	if (arg == NULL) {
		LOG_ER("MBCSv Callback NULL arg received");
		return NCSCC_RC_FAILURE;
	}
	switch (arg->i_op) {
	case NCS_MBCSV_CBOP_ENC:
		/*encode request from mbcsv */
		rc = mqd_mbcsv_ckpt_encode_cbk_handler(arg);
		break;

	case NCS_MBCSV_CBOP_DEC:
		/* decode request from mbcsv */
		rc = mqd_mbcsv_ckpt_decode_cbk_handler(arg);
		break;

	case NCS_MBCSV_CBOP_PEER:
		/* Peer info request from mbcsv */
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_CBOP_NOTIFY:
		/* notify request from mbcsv */
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_CBOP_ERR_IND:
		rc = NCSCC_RC_SUCCESS;
		break;
	default:
		break;

	}

	TRACE_1("MBCSv Callback Success");
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_ckpt_encode_async_update
 *
 * Description            : Encode the Sync Updates
 *                          
 *
 * Parameters             : arg: NCS_MBCSV_CB_ARG pointer
 *                          pMqd: Control Block pointer 
 * Return Values          :
 *
 * otes                  : None
********************************************************************************
*****************/
static uint32_t mqd_ckpt_encode_async_update(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	MQD_A2S_MSG *mqd_msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	TRACE_ENTER();

	/*  Increment the async update count  */
	pMqd->mqd_sync_updt_count++;

	mqd_msg = (MQD_A2S_MSG *)NCS_INT64_TO_PTR_CAST(arg->info.encode.io_reo_hdl);
	rc = m_NCS_EDU_EXEC(&pMqd->edu_hdl, mqsv_edp_mqd_a2s_msg, &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, mqd_msg,
			    &ederror);
	if (rc != NCSCC_RC_SUCCESS) {
		switch (mqd_msg->type) {
		case MQD_A2S_MSG_TYPE_REG:
			LOG_ER("MBCSV Reg Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;

		case MQD_A2S_MSG_TYPE_DEREG:
			LOG_ER("MBCSV Dereg Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;

		case MQD_A2S_MSG_TYPE_TRACK:
			LOG_ER("MBCSV Track Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;

		case MQD_A2S_MSG_TYPE_USEREVT:
			LOG_ER("MBCSV Userevt Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;
		case MQD_A2S_MSG_TYPE_MQND_STATEVT:
			LOG_ER("MBCSV Ndstat Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;
		case MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT:
			LOG_ER("MBCSV Ndtimer Expiry Encode EDU Error");
			rc = NCSCC_RC_FAILURE;
			break;
		default:
			break;
		}

	}
	TRACE_LEAVE();
	return rc;
}

/*******************************************************************************
*****************
 * Name                   : mqd_ckpt_decode_async_update
 *
 * Description            : Decode the Sync Updates
 *
 *
 * Parameters             : arg: NCS_MBCSV_CB_ARG pointer
 *                          pMqd: Control Block pointer
 * Return Values          :
 *
 * otes                  : None
********************************************************************************
*****************/

static uint32_t mqd_ckpt_decode_async_update(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	MQD_A2S_MSG *mqd_msg = 0;
	uint32_t rc = NCSCC_RC_SUCCESS;
	EDU_ERR ederror = 0;
	TRACE_ENTER();

	mqd_msg = m_MMGR_ALLOC_MQD_A2S_MSG;
	if (mqd_msg == NULL) {
		LOG_CR("%s:%u: Failed To Allocate Memory", __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	/* To store the value of Async Update received */
	pMqd->mqd_sync_updt_count++;

	memset(mqd_msg, 0, sizeof(MQD_A2S_MSG));

	rc = m_NCS_EDU_EXEC(&pMqd->edu_hdl, mqsv_edp_mqd_a2s_msg, &arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &mqd_msg,
			    &ederror);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("MBCSV Reg Decode EDU Error");
		rc = NCSCC_RC_FAILURE;
		goto end;
	}
	rc = mqd_process_a2s_event(pMqd, mqd_msg);
	if (rc != NCSCC_RC_SUCCESS) {
		switch (mqd_msg->type) {
		case MQD_A2S_MSG_TYPE_REG:
			TRACE_2("ASAPi_REG_INFO processing at Standby failed");
			goto end;
		case MQD_A2S_MSG_TYPE_DEREG:
			TRACE_2("ASAPi_DEREG_INFO processing at Standby failed");
			goto end;
		case MQD_A2S_MSG_TYPE_TRACK:
			TRACE_2("ASAPi_TRACK_INFO processing at Standby failed");
			goto end;
		case MQD_A2S_MSG_TYPE_USEREVT:
			TRACE_2("ASAPi_USEREVT_INFO processing at Standby failed");
			goto end;
		case MQD_A2S_MSG_TYPE_MQND_STATEVT:
			TRACE_2("ASAPi_NDSTATINFO_INFO processing at Standby failed");
			goto end;
		case MQD_A2S_MSG_TYPE_MQND_TIMER_EXPEVT:
			TRACE_2("ASAPi_NDTMREXP_INFO processing at Standby failed");
			goto end;
		default:
			goto end;
		}
	}

 end:
	m_MMGR_FREE_MQD_A2S_MSG(mqd_msg);
	TRACE_LEAVE();
	return rc;

}

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_ckpt_encode_cbk_handler
 *
 * Description            : Encode Call back function to process the 
 *                          the encode data to be sent to the peer.
 *
 * Parameters             : arg: NCS_MBCSV_CB_ARG pointer
 *
 * Return Values          :
 *
 * otes                  : None
********************************************************************************
*****************/
static uint32_t mqd_mbcsv_ckpt_encode_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{

	uint32_t rc = NCSCC_RC_SUCCESS, msg_fmt_version;
	MQD_CB *pMqd = NULL;
	TRACE_ENTER();

	if (arg == NULL) {
		return NCSCC_RC_SUCCESS;
	}
	/* Get MQD control block handle */
	if (NULL == (pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl))) {
		LOG_ER("%s:%u: CB Handle Take Failed", __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.encode.i_peer_version,
					      MQSV_MQD_MBCSV_VERSION, MQSV_MQD_MBCSV_VERSION_MIN);

	if (!msg_fmt_version) {
		/* Drop The Message */
		LOG_ER("INVALID MSG FORMAT %d", msg_fmt_version);
		return NCSCC_RC_FAILURE;
	}

	switch (arg->info.encode.io_msg_type) {
	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* enocde Async update message */
		rc = mqd_ckpt_encode_async_update(pMqd, arg);
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:

		/* Encode Cold Sync */
		pMqd->cold_or_warm_sync_on = true;

		if (pMqd->ha_state == SA_AMF_HA_STANDBY) {
			/* There is no opaque data so no data is encoded here */
		}
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
		/* Encode Cold Sync Response */
		if (pMqd->ha_state == SA_AMF_HA_ACTIVE) {
			if (pMqd->qdb_up) {
				rc = mqd_ckpt_encode_cold_sync_data(pMqd, arg);
				if (rc != NCSCC_RC_SUCCESS) {
#if (NCS_MQA_DEBUG==1)
					TRACE_2("Active: Encode call back of Cold Sync Response Failed");
#endif
				} else {
					TRACE_1("Active: Encode call back of Cold Sync Response Success");
				}
			} else {
				arg->info.decode.i_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
				rc = NCSCC_RC_SUCCESS;
			}
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		rc = NCSCC_RC_SUCCESS;
		/* Encode Warm Sync Req data No action needed now */
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
		/* Encode Warm Sync response */
		rc = mqd_ckpt_encode_warm_sync_response(pMqd, arg);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_2("Active: Encode call back of Warm Sync Response Failed");
		} else {
			TRACE_1("Active: Encode call back of Warm Sync Response Success");
		}
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
		rc = mqd_ckpt_encode_cold_sync_data(pMqd, arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Active: Encode call back of Data Response Failed");
		else
			TRACE_1("Active: Encode call back of Data Response Success");
		break;

	default:
		LOG_ER("Invalid NCS_MBCSV_MSG encode type %d", arg->info.encode.io_msg_type);
		rc = NCSCC_RC_FAILURE;
	}
	ncshm_give_hdl(gl_mqdinfo.inst_hdl);	/* TBD  */
	TRACE_LEAVE();
	return rc;
}	/* end of mqd_mbcsv_ckpt_encode_cbk_handler */

/*******************************************************************************
*****************
 * Name                   : mqd_mbcsv_ckpt_decode_cbk_handler
 *
 * Description            : Decode Call back function to process the 
 *                          the decode data.
 *
 * Parameters             : arg: NCS_MBCSV_CB_ARG pointer
 *
 * Return Values          :
 *
 * Notes                  : None
********************************************************************************
*****************/
static uint32_t mqd_mbcsv_ckpt_decode_cbk_handler(NCS_MBCSV_CB_ARG *arg)
{

	uint32_t rc = NCSCC_RC_SUCCESS, msg_fmt_version;
	MQD_CB *pMqd = NULL;
	TRACE_ENTER();

	if (arg == NULL) {
		return NCSCC_RC_SUCCESS;
	}
	/* Get MQD control block handle */
	if (NULL == (pMqd = ncshm_take_hdl(NCS_SERVICE_ID_MQD, gl_mqdinfo.inst_hdl))) {
		LOG_ER("%s:%u: CB Handle Take Failed", __FILE__, __LINE__);
		return NCSCC_RC_FAILURE;
	}

	msg_fmt_version = m_NCS_MBCSV_FMT_GET(arg->info.decode.i_peer_version,
					      MQSV_MQD_MBCSV_VERSION, MQSV_MQD_MBCSV_VERSION_MIN);

	if (!msg_fmt_version) {
		/* Drop The Message */
		LOG_ER("INVALID MSG FORMAT %d", msg_fmt_version);
		return NCSCC_RC_FAILURE;
	}

	switch (arg->info.decode.i_msg_type) {

	case NCS_MBCSV_MSG_ASYNC_UPDATE:
		/* decode Async update message */
		rc = mqd_ckpt_decode_async_update(pMqd, arg);
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_REQ:
		/* Encode Cold Sync */
		/* There is no opaque data so no data is encoded here */
		/* Now reset the ifindex counter for the next warm sync. */
		memset(&pMqd->record_qindex_name, 0, sizeof(SaNameT));
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_MSG_COLD_SYNC_RESP:
	case NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE:

		/* decode Cold Sync Response */
		rc = mqd_ckpt_decode_cold_sync_resp(pMqd, arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Standby: Decode call back of Cold Sync Response Failed");
		else
			TRACE_1("Standby: Decode call back of Cold Sync Response Success");
		if (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) {
			rc = NCSCC_RC_SUCCESS;
			break;
		}
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_REQ:
		rc = NCSCC_RC_SUCCESS;
		/* Decode Warm Sync Req data No action needed now */
		break;

	case NCS_MBCSV_MSG_WARM_SYNC_RESP:
	case NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE:

		/* Encode Warm Sync response */
		rc = mqd_ckpt_decode_warm_sync_response(pMqd, arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Standby: Decode call back of Warm Sync Response Failed");
		else
			TRACE_1("Standby: Decode call back of Warm Sync Response Success");
		break;

	case NCS_MBCSV_MSG_DATA_REQ:
		rc = NCSCC_RC_SUCCESS;
		break;

	case NCS_MBCSV_MSG_DATA_RESP:
	case NCS_MBCSV_MSG_DATA_RESP_COMPLETE:
		rc = mqd_ckpt_decode_cold_sync_resp(pMqd, arg);
		if (rc != NCSCC_RC_SUCCESS)
			TRACE_2("Standby: Decode call back of Data Response Failed");
		else
			TRACE_1("Standby: Decode call back of Data Response Success");
		break;

	default:
		TRACE_2("Invalid NCS_MBCSV_MSG decode type %d", arg->info.decode.i_msg_type);
		rc = NCSCC_RC_FAILURE;
	}
	ncshm_give_hdl(gl_mqdinfo.inst_hdl);	/* TBD  */
	TRACE_LEAVE();
	return rc;
}	/* end of mqd_mbcsv_ckpt_decode_cbk_handler */

/****************************************************************************
 * Name          : mqd_ckpt_encode_warm_sync_response
 *
 * Description   : This function encodes the warm sync resp message.
 *
 * Arguments     : pMqd - Control block
 *                 arg - Pointer to the structure containing information
 *                 pertaining to the callback being dispatched by MBCSV.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqd_ckpt_encode_warm_sync_response(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	uint8_t *wsync_ptr = 0;

	/* Reserve space to send the async update counter */
	wsync_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint32_t));

	if (wsync_ptr == NULL) {
		/*TBD */
		LOG_ER("Failed to send the async update counter");
		return NCSCC_RC_FAILURE;
	}

	/* SEND THE ASYNC UPDATE COUNTER */
	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	ncs_encode_32bit(&wsync_ptr, pMqd->mqd_sync_updt_count);
	arg->info.encode.io_msg_type = NCS_MBCSV_MSG_WARM_SYNC_RESP_COMPLETE;

	return rc;

}	/* function mqd_ckpt_encode_warm_sync_response() ends here. */

/****************************************************************************
 * Name          : mqd_ckpt_decode_warm_sync_response
 *
 * Description   : This function decodes the warm sync resp message.
 *
 * Arguments     : pMqd - Control block
 *                 arg - Pointer to the structure containing information
 *                 pertaining to the callback being dispatched by MBCSV.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/
static uint32_t mqd_ckpt_decode_warm_sync_response(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t num_of_async_upd = 0, rc = NCSCC_RC_SUCCESS;
	uint8_t data[16], *ptr;
	NCS_MBCSV_ARG ncs_arg;
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, data, sizeof(int32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(int32_t));

	memset(&ncs_arg, '\0', sizeof(NCS_MBCSV_ARG));

	if (pMqd->mqd_sync_updt_count == num_of_async_upd) {
		return rc;
	} else {
		mqd_db_del_all_records(pMqd);
		pMqd->cold_or_warm_sync_on = true;
		ncs_arg.i_op = NCS_MBCSV_OP_SEND_DATA_REQ;
		ncs_arg.i_mbcsv_hdl = pMqd->mbcsv_hdl;
		ncs_arg.info.send_data_req.i_ckpt_hdl = pMqd->o_ckpt_hdl;
		rc = ncs_mbcsv_svc(&ncs_arg);
		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("MBCSV Data Request Send Failed with error %d", rc);
			return rc;
		}
	}
	return rc;
}	/* function mqd_ckpt_decode_warm_sync_response() ends here. */

/****************************************************************************
 * Name          : mqd_ckpt_encode_cold_sync_data
 *
 * Description   : This function encodes data resp message. Each 
 *                 queue record, includes queue info to Standby MQD. It
 *                 sends FIXED no. of record at each time, if no. of records to
 *                 to be sent is more than FIXED no., else the no. of records
 *                 left.   
 *
 * Arguments     : pMqd - Control block
 *                 arg - Pointer to the structure containing information
 *                 pertaining to the callback being dispatched by MBCSV.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 |------------------|---------------|-----------|------|-----------|-----------| |No. of Ckpts      | ckpt record 1 |ckpt rec 2 |..... |ckpt rec n | async upd |
 |that will be sent |               |           |      |           | cnt ( 0 ) |
 |------------------|---------------------------|------|-----------|-----------|
 *****************************************************************************/
static uint32_t mqd_ckpt_encode_cold_sync_data(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	MQD_OBJ_NODE *queue_record = 0;
	MQD_OBJ_INFO queue_obj_info;
	MQD_A2S_MSG cold_sync_data;
	SaNameT queue_name;
	SaNameT queue_index_name;
	NCS_PATRICIA_NODE *q_node = 0;
	NCS_LOCK *q_rec_lock = &pMqd->mqd_cb_lock;
	uint32_t rc = NCSCC_RC_SUCCESS, num_of_ckpts = 0;
	uint32_t last_message = false;
	EDU_ERR ederror = 0;
	uint8_t *header, *sync_cnt_ptr;
	TRACE_ENTER();

	/* COLD_SYNC_RESP IS DONE BY THE ACTIVE */
	if (pMqd->ha_state == SA_AMF_HA_STANDBY) {
		LOG_ER("AMF state is Not Active");
		rc = NCSCC_RC_FAILURE;
		return rc;
	}
	memset(&queue_obj_info, 0, sizeof(MQD_OBJ_INFO));
	memset(&cold_sync_data, 0, sizeof(MQD_A2S_MSG));
	memset(&queue_name, 0, sizeof(SaNameT));
	memset(&queue_index_name, 0, sizeof(SaNameT));

	/*First reserve space to store the number of checkpoints that will be sent */
	header = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	if (header == NULL) {
		LOG_ER("Encode Reserve Space for Async Count Failed");
		rc = NCSCC_RC_FAILURE;
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	queue_index_name = pMqd->record_qindex_name;
	m_NCS_LOCK(q_rec_lock, NCS_LOCK_WRITE);
	q_node = ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&queue_index_name);
	queue_record = (MQD_OBJ_NODE *)q_node;

	while (queue_record != NULL) {
		queue_obj_info = queue_record->oinfo;
		queue_index_name = queue_record->oinfo.name;

		/* Copy the Node info from Patrecia node to the ColdSync Structure */
		rc = mqd_copy_data_to_cold_sync_structure(&queue_obj_info, &cold_sync_data.info.qinfo);
		if (rc != NCSCC_RC_SUCCESS) {
			/* TBD to free the memory already allocated */
			TRACE_4("Failed to copy Node info to ColdSync Structure");
			m_NCS_UNLOCK(q_rec_lock, NCS_LOCK_WRITE);
			return rc;
		}

		num_of_ckpts++;

		rc = m_NCS_EDU_EXEC(&pMqd->edu_hdl, mqsv_edp_mqd_a2s_queue_info,
				    &arg->info.encode.io_uba, EDP_OP_TYPE_ENC, &cold_sync_data.info.qinfo, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			/* TBD to free the memory already allocated */
			LOG_ER("MBCSV Encode EDU Error");
			m_NCS_UNLOCK(q_rec_lock, NCS_LOCK_WRITE);
			return rc;
		}
		if (cold_sync_data.info.qinfo.ilist_info) {
			m_MMGR_FREE_MQD_DEFAULT_VAL(cold_sync_data.info.qinfo.ilist_info);
			cold_sync_data.info.qinfo.ilist_info = NULL;
		}
		if (cold_sync_data.info.qinfo.track_info) {
			m_MMGR_FREE_MQD_DEFAULT_VAL(cold_sync_data.info.qinfo.track_info);
			cold_sync_data.info.qinfo.track_info = NULL;
		}

		if (num_of_ckpts == MAX_NO_MQD_MSGS_A2S) {
			/* Just check whether, it is a last message or not. It will help
			   in deciding last message */
			q_node = ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&queue_index_name);
			if (q_node == NULL)
				last_message = true;
			break;
		}

		q_node = ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&queue_index_name);
		queue_record = (MQD_OBJ_NODE *)q_node;

	}			/* while */

	m_NCS_UNLOCK(q_rec_lock, NCS_LOCK_WRITE);

	pMqd->record_qindex_name = queue_index_name;

	ncs_encode_32bit(&header, num_of_ckpts);

	/* This will have the count of async updates that have been sent, 
	   this will be 0 initially */
	sync_cnt_ptr = ncs_enc_reserve_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	if (sync_cnt_ptr == NULL) {
		LOG_ER("Encode Reserve Space for Async Count Failed");
		rc = NCSCC_RC_FAILURE;
		/* TBD to free the memory already allocated */
		return NCSCC_RC_FAILURE;
	}

	ncs_enc_claim_space(&arg->info.encode.io_uba, sizeof(uint32_t));
	ncs_encode_32bit(&sync_cnt_ptr, pMqd->mqd_sync_updt_count);

	q_node = ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&queue_index_name);
	if (q_node == NULL) {
		last_message = true;
	}

	if ((num_of_ckpts < MAX_NO_MQD_MSGS_A2S) || (last_message == true)) {
		if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP)
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE;
		else if (arg->info.encode.io_msg_type == NCS_MBCSV_MSG_DATA_RESP)
			arg->info.encode.io_msg_type = NCS_MBCSV_MSG_DATA_RESP_COMPLETE;

		/* Now reset the ifindex counter for the next warm sync. */
		memset(&pMqd->record_qindex_name, 0, sizeof(SaNameT));
	}

	TRACE_LEAVE();
	return rc;

}	/* function mqd_ckpt_encode_cold_sync_data() ends here. */

/****************************************************************************
 * Name          : mqd_copy_data_to_cold_sync_structure
 *
 * Description   : This function copies the data from the patretia tree node to  
 *                the  mbcsv message.
 *
 * Arguments     : obj_info - MQD database object info
 *                 mbcsv_info - Pointer to the structure containing information
 *                 pertaining to the callback being dispatched by MBCSV.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqd_copy_data_to_cold_sync_structure(MQD_OBJ_INFO *obj_info, MQD_A2S_QUEUE_INFO *mbcsv_info)
{
	uint32_t index;
	NCS_Q_ITR itr;
	MQD_OBJECT_ELEM *pOelm = 0;
	MQD_TRACK_OBJ *pTrkObj = 0;
	TRACE_ENTER();

	memset(mbcsv_info, 0, sizeof(MQD_A2S_QUEUE_INFO));
	mbcsv_info->name = obj_info->name;
	mbcsv_info->type = obj_info->type;
	mbcsv_info->info.q = obj_info->info.q;	/* Assigning the union here */
	mbcsv_info->ilist_cnt = obj_info->ilist.count;
	if (mbcsv_info->ilist_cnt) {
		mbcsv_info->ilist_info = m_MMGR_ALLOC_MQD_DEFAULT_VAL(mbcsv_info->ilist_cnt * sizeof(SaNameT));
		if (!mbcsv_info->ilist_info) {
			LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
			return SA_AIS_ERR_NO_MEMORY;
		}
	}
	mbcsv_info->track_cnt = obj_info->tlist.count;
	if (mbcsv_info->track_cnt) {
		mbcsv_info->track_info =
		    m_MMGR_ALLOC_MQD_DEFAULT_VAL(mbcsv_info->track_cnt * sizeof(MQD_A2S_TRACK_INFO));
		if (!mbcsv_info->track_info) {
			LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
			return SA_AIS_ERR_NO_MEMORY;
		}
	}
	itr.state = 0;

	for (index = 0; index < mbcsv_info->ilist_cnt; index++) {

		pOelm = (MQD_OBJECT_ELEM *)ncs_walk_items(&obj_info->ilist, &itr);
		mbcsv_info->ilist_info[index] = pOelm->pObject->name;
	}
	itr.state = 0;
	for (index = 0; index < mbcsv_info->track_cnt; index++) {

		pTrkObj = (MQD_TRACK_OBJ *)ncs_walk_items(&obj_info->tlist, &itr);
		mbcsv_info->track_info[index].dest = pTrkObj->dest;
		mbcsv_info->track_info[index].to_svc = pTrkObj->to_svc;
	}
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqd_ckpt_decode_cold_sync_resp
 *
 * Description   : This function decodes the data resp message or cold sync 
                   response sent by Active MQD to the standby MQD.
 *                 It sends FIXED no. of record at each time, if no. of records 
 *                 to be sent is more than FIXED no., else the no. of records
 *                 left.
 *
 * Arguments     : pMqd - Control block
 *                 arg - Pointer to the structure containing information
 *                 pertaining to the callback being dispatched by MBCSV.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 |------------------|---------------|-----------|------|-----------|-----------| 
 |No. of Ckpts      | ckpt record 1 |ckpt rec 2 |..... |ckpt rec n | async upd |
 |that will be sent |               |           |      |           | cnt ( 0 ) |
 |------------------|---------------------------|------|-----------|-----------|
 *****************************************************************************/
static uint32_t mqd_ckpt_decode_cold_sync_resp(MQD_CB *pMqd, NCS_MBCSV_CB_ARG *arg)
{
	uint32_t num_of_ckpts, data[sizeof(uint32_t)];
	uint32_t count = 0, rc = NCSCC_RC_SUCCESS, num_of_async_upd;
	uint8_t *ptr;
	EDU_ERR ederror = 0;
	MQD_A2S_QUEUE_INFO *mbcsv_info = 0;
	TRACE_ENTER();

	if (arg->info.decode.i_uba.ub == NULL) {	/* There is no data */
		return NCSCC_RC_SUCCESS;
	}
	mbcsv_info = m_MMGR_ALLOC_MQD_DEFAULT_VAL(sizeof(MQD_A2S_QUEUE_INFO));
	if (mbcsv_info == NULL) {
		LOG_CR("%s:%u: Failed To Allocate Memory", __FILE__, __LINE__);
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	memset(mbcsv_info, 0, sizeof(MQD_A2S_QUEUE_INFO));

	/* 1. Decode the 1st uint8_t region ,  we will get the num of ckpts */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, (uint8_t *)data, sizeof(uint32_t));
	num_of_ckpts = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uint32_t));

	/* Decode the data */

	while (count < num_of_ckpts) {
		rc = m_NCS_EDU_EXEC(&pMqd->edu_hdl, mqsv_edp_mqd_a2s_queue_info,
				    &arg->info.decode.i_uba, EDP_OP_TYPE_DEC, &mbcsv_info, &ederror);

		if (rc != NCSCC_RC_SUCCESS) {
			LOG_ER("m_NCS_EDU_EXEC failed");
			if (mbcsv_info)
				m_MMGR_FREE_MQD_DEFAULT_VAL(mbcsv_info);
			return rc;
		}

		rc = mqd_a2s_make_record_from_coldsync(pMqd, *mbcsv_info);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_2("mqd_a2s_make_record_from_coldsync function failed");
			m_MMGR_FREE_MQD_DEFAULT_VAL(mbcsv_info);
			return rc;
		}

		count++;
		if (mbcsv_info->ilist_info)
			m_MMGR_FREE_MQSV_OS_MEMORY(mbcsv_info->ilist_info);
		if (mbcsv_info->track_info)
			m_MMGR_FREE_MQSV_OS_MEMORY(mbcsv_info->track_info);
		memset(mbcsv_info, 0, sizeof(MQD_A2S_QUEUE_INFO));
	}
	m_MMGR_FREE_MQD_DEFAULT_VAL(mbcsv_info);

	/* Get the async update count */
	ptr = ncs_dec_flatten_space(&arg->info.decode.i_uba, (uint8_t *)data, sizeof(uint32_t));
	num_of_async_upd = ncs_decode_32bit(&ptr);
	ncs_dec_skip_space(&arg->info.decode.i_uba, sizeof(uint32_t));

	pMqd->mqd_sync_updt_count = num_of_async_upd;

	if ((arg->info.decode.i_msg_type == NCS_MBCSV_MSG_COLD_SYNC_RESP_COMPLETE) ||
	    (arg->info.decode.i_msg_type == NCS_MBCSV_MSG_DATA_RESP_COMPLETE)) {
		pMqd->cold_or_warm_sync_on = false;
	}

	TRACE_LEAVE();
	return rc;
}	/*mqd_ckpt_decode_cold_sync_resp()  */

/****************************************************************************
 * Name          : mqd_a2s_make_record_from_coldsync
 *
 * Description   : This function copies the data from the coldsync structure to
 *                the  control block patreciatree.
 *
 * Arguments     : MQD_CB *pMqd- Control block pointer
 *                 MQD_A2S_QUEUE_INFO q_data_msg Data in the coldsync structure.
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqd_a2s_make_record_from_coldsync(MQD_CB *pMqd, MQD_A2S_QUEUE_INFO q_data_msg)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	MQD_OBJ_NODE *q_obj_node = 0, *q_node = 0;
	MQD_TRACK_OBJ *q_track_obj = 0;
	uint32_t index = 0;
	SaNameT record_qindex_name;
	MQD_OBJECT_ELEM *pOelm = 0;
	uint32_t new_record = 0;
	TRACE_ENTER();

	/* Read the data of queueinformation and write to the q_obj_node */
	memset(&record_qindex_name, 0, sizeof(SaNameT));
	record_qindex_name = q_data_msg.name;
	/* m_HTON_SANAMET_LEN(record_qindex_name.length);a */
	q_obj_node = (MQD_OBJ_NODE *)ncs_patricia_tree_get(&pMqd->qdb, (uint8_t *)&record_qindex_name);
	if (!q_obj_node) {
		rc = mqd_db_node_create(pMqd, &q_obj_node);
		if (rc != NCSCC_RC_SUCCESS) {
			TRACE_4("OBJ_NODE allocation Failed");
			rc = NCSCC_RC_FAILURE;
			return NCSCC_RC_FAILURE;
		}
		new_record = 1;
	}
	q_obj_node->oinfo.name = q_data_msg.name;
	q_obj_node->oinfo.type = q_data_msg.type;
	q_obj_node->oinfo.info.q = q_data_msg.info.q;

	/* Add the ilist information to the patricia node */
	for (index = 0; index < q_data_msg.ilist_cnt; index++) {
		record_qindex_name = q_data_msg.ilist_info[index];
		/* m_HTON_SANAMET_LEN(record_qindex_name.length); */
		q_node = (MQD_OBJ_NODE *)ncs_patricia_tree_get(&pMqd->qdb, (uint8_t *)&record_qindex_name);
		if (!q_node) {
			rc = mqd_db_node_create(pMqd, &q_node);
			if (rc != NCSCC_RC_SUCCESS) {
				/* Error while creating the node */
				TRACE_4("OBJ_NODE allocation Failed");
				return NCSCC_RC_FAILURE;
			}
			q_node->oinfo.name = q_data_msg.ilist_info[index];
			rc = mqd_db_node_add(pMqd, q_node);
			if (rc != NCSCC_RC_SUCCESS) {
				LOG_ER("Addition of Object node into the Tree failed");
				return NCSCC_RC_FAILURE;
			}
		}

		/* Search if the qnode is already present in the ilist */
		pOelm = ncs_find_item(&q_obj_node->oinfo.ilist, &record_qindex_name, mqd_obj_cmp);
		if (pOelm == NULL) {
			/*Create the Queue Element */
			pOelm = m_MMGR_ALLOC_MQD_OBJECT_ELEM;
			if (!pOelm) {
				LOG_CR("ERR_MEMORY: Failed To Allocate Memory for Queue Element");
				rc = NCSCC_RC_FAILURE;
				return SA_AIS_ERR_NO_MEMORY;
			}
			memset(pOelm, 0, sizeof(MQD_OBJECT_ELEM));
			pOelm->pObject = &q_node->oinfo;
			ncs_enqueue(&q_obj_node->oinfo.ilist, pOelm);
		}
		q_node = 0;
		pOelm = 0;
	}			/* End of for. ilist is updated */

	/* Filling the track info to the queue database */
	for (index = 0; index < q_data_msg.track_cnt; index++) {
		q_track_obj = m_MMGR_ALLOC_MQD_TRACK_OBJ;
		if (q_track_obj == NULL) {
			LOG_CR("%s:%u: ERR_MEMORY: Failed To Allocate Memory", __FILE__, __LINE__);
			rc = NCSCC_RC_FAILURE;
			return NCSCC_RC_FAILURE;
		}
		memset(q_track_obj, 0, sizeof(MQD_TRACK_OBJ));
		q_track_obj->dest = q_data_msg.track_info[index].dest;
		q_track_obj->to_svc = q_data_msg.track_info[index].to_svc;
		ncs_enqueue(&q_obj_node->oinfo.tlist, q_track_obj);
	}
	if (new_record)
		rc = mqd_db_node_add(pMqd, q_obj_node);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Database Operation (ADD) Failed");
	}
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name          : mqd_db_del_all_records
 *
 * Description   : This function deletes the records form the database when warmsync
 *                happens and the data mismatch occurs.
 *
 * Arguments     : MQD_CB *pMqd- Control block pointer
 *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * Notes         : None.
 *****************************************************************************/

static uint32_t mqd_db_del_all_records(MQD_CB *pMqd)
{
	if (!pMqd->qdb_up) {
		LOG_ER("Queue Database is down"); 
		return NCSCC_RC_FAILURE;
	}

	mqd_queue_db_clean_up(pMqd);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
 * Name          : mqd_queue_db_clean_up
 *
 * Description   : This function deletes the records form the database. 
 *
 * Arguments     : MQD_CB *pMqd- Control block pointer
 *
 * Return Values : None
 *
 * Notes         : None.
 *****************************************************************************/

static void mqd_queue_db_clean_up(MQD_CB *pMqd)
{
	MQD_OBJ_NODE *qnode = 0;
	SaNameT record_qindex_name;
	NODE_ID prev_node_id = 0;
	MQD_ND_DB_NODE *nd_node = 0;

	memset(&record_qindex_name, 0, sizeof(SaNameT));

	qnode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&record_qindex_name);
	nd_node = (MQD_ND_DB_NODE *)ncs_patricia_tree_getnext(&pMqd->node_db, (uint8_t *)&prev_node_id);

	while (qnode) {
		record_qindex_name = qnode->oinfo.name;
		mqd_db_node_del(pMqd, qnode);

		qnode = (MQD_OBJ_NODE *)ncs_patricia_tree_getnext(&pMqd->qdb, (uint8_t *)&record_qindex_name);

	}
	while (nd_node) {
		prev_node_id = nd_node->info.nodeid;

		mqd_red_db_node_del(pMqd, nd_node);

		nd_node = (MQD_ND_DB_NODE *)ncs_patricia_tree_getnext(&pMqd->node_db, (uint8_t *)&prev_node_id);

	}
	return;
}
