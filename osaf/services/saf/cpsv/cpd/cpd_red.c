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
  FILE NAME: cpd_red.c

  DESCRIPTION: CPD Redundancy Processing Routines at Active CPD

******************************************************************************/

#include "cpd.h"

/************************************************************************************
 * Name            : cpd_a2s_ckpt_create

 * Description     : This routine will update the CPD_MBCSV_MSG (message) which has
 *                   to be sent to Standby for checkpoint create Async Update 
 
 * Input Values    : CPD_CB , CPD_CKPT_INFO_NODE - CPD checkpoint node

 * Return Values   : SA_AIS_OK if success else return appropriate error
 *
 * Notes           : The information present in the ckpt_node is copied into cpd_msg(which has to be sent to Standby )
**************************************************************************************************************/
uint32_t cpd_a2s_ckpt_create(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_MBCSV_MSG cpd_msg;
	SaAisErrorT rc = SA_AIS_OK;
	uint32_t count = 0;

	TRACE_ENTER();
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));

	cpd_msg.type = CPD_A2S_MSG_CKPT_CREATE;
	cpd_msg.info.ckpt_create.ckpt_name = ckpt_node->ckpt_name;
	cpd_msg.info.ckpt_create.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.ckpt_create.ckpt_attrib = ckpt_node->attributes;
	cpd_msg.info.ckpt_create.is_unlink_set = ckpt_node->is_unlink_set;
	cpd_msg.info.ckpt_create.create_time = ckpt_node->create_time;
	cpd_msg.info.ckpt_create.num_users = ckpt_node->num_users;
	cpd_msg.info.ckpt_create.num_readers = ckpt_node->num_readers;
	cpd_msg.info.ckpt_create.num_writers = ckpt_node->num_writers;
	cpd_msg.info.ckpt_create.num_sections = ckpt_node->num_sections;
	cpd_msg.info.ckpt_create.is_active_exists = ckpt_node->is_active_exists;
	if (cpd_msg.info.ckpt_create.is_active_exists)
		cpd_msg.info.ckpt_create.active_dest = ckpt_node->active_dest;

	if (ckpt_node->dest_cnt) {
		CPD_NODE_REF_INFO *node_list = ckpt_node->node_list;
		cpd_msg.info.ckpt_create.dest_cnt = ckpt_node->dest_cnt;
		cpd_msg.info.ckpt_create.dest_list = m_MMGR_ALLOC_CPSV_CPND_DEST_INFO(ckpt_node->dest_cnt);
		if (cpd_msg.info.ckpt_create.dest_list == NULL) {
			LOG_CR("cpd cpnd dest info memory allocation failed");
			rc = SA_AIS_ERR_NO_MEMORY;
			goto end;
		} else {
			memset(cpd_msg.info.ckpt_create.dest_list, '\0',
			       (sizeof(CPSV_CPND_DEST_INFO) * ckpt_node->dest_cnt));

			for (count = 0; count < ckpt_node->dest_cnt; count++) {
				cpd_msg.info.ckpt_create.dest_list[count].dest = node_list->dest;
				node_list = node_list->next;
			}
		}
	}

	/* send it to MBCSv */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt create async failed for ckptid :%llx",ckpt_node->ckpt_id);
	else
		TRACE_1("cpd A2S ckpt create async success for ckptid :%llx",ckpt_node->ckpt_id);

	if (cpd_msg.info.ckpt_create.dest_list)
		m_MMGR_FREE_CPSV_CPND_DEST_INFO(cpd_msg.info.ckpt_create.dest_list);

 end:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
 * Name           : cpd_a2s_ckpt_unlink_set
 *
 * Description    : Function to send the Unlink information of the checkpoint to Standby
 *
 * Arguments      : CPD_CKPT_INFO_NODE  - CPD Checkpoint info node
 *                  CPSV_SEND_INFO      - Send info
 * 
 * Return Values  : None
 *
 * Notes          : Async update are sent from active to standby  
 ****************************************************************************/

void cpd_a2s_ckpt_unlink_set(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;

	TRACE_ENTER();
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	cpd_msg.type = CPD_A2S_MSG_CKPT_UNLINK;
	cpd_msg.info.ckpt_ulink.is_unlink_set = ckpt_node->is_unlink_set;
	cpd_msg.info.ckpt_ulink.ckpt_name = ckpt_node->ckpt_name;

	/* send it to MBCSv  */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt unlink async failed %s",ckpt_node->ckpt_name.value);
	else
		TRACE_1("cpd A2S ckpt unlink async successfull ");
	TRACE_LEAVE();
}

/***********************************************************************************
 * Name                 : cpd_a2s_ckpt_rdset
 *
 * Description          : Function to set the retention duration of the checkpoint
 *                        and to send it to standby
 *
 * Arguments            : CPD_CKPT_INFO_NODE   - CPD Checkpoint info node
 *
 * Return Values        : None, Error message is logged
 *
 * Notes                : None
 ************************************************************************************/
void cpd_a2s_ckpt_rdset(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;
	
	TRACE_ENTER();

	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	cpd_msg.type = CPD_A2S_MSG_CKPT_RDSET;
	cpd_msg.info.rd_set.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.rd_set.reten_time = ckpt_node->ret_time;

	/* send it to MBCSv */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt rdset async failed for ckpt_id :%llx",ckpt_node->ckpt_id);
	else
		TRACE_1("cpd A2S ckpt rdes async success for ckpt_id :%llx",ckpt_node->ckpt_id);

	TRACE_LEAVE();
}

/*************************************************************************************
 * Name                  : cpd_a2s_ckpt_arep_set
 *
 * Description           : Function to set the active replica and send it to standby
 *
 * Arguments             : CPD_CKPT_INFO_NODE   -  CPD Checkpoint info node
 *
 * Return Values         : None , Error message is logged
 *
 * Notes                 : None
 ************************************************************************************/
void cpd_a2s_ckpt_arep_set(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;

	TRACE_ENTER();
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	cpd_msg.type = CPD_A2S_MSG_CKPT_AREP_SET;
	cpd_msg.info.arep_set.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.arep_set.mds_dest = ckpt_node->active_dest;

	/* send it to MBCSv */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt arep set async failed for ckpt_id :%llx active_dest :%"PRIu64,ckpt_node->ckpt_id,ckpt_node->active_dest);
	else
		TRACE_1("cpd A2S ckpt arep set async success for ckpt_id :%llx active_dest :%"PRIu64,ckpt_node->ckpt_id,ckpt_node->active_dest);
	TRACE_LEAVE();
}

/*******************************************************************************************
 * Name                   : cpd_a2s_ckpt_dest_add
 *
 * Description            : Function to add the destination of the already existing 
 *                          checkpoint
 *
 * Arguments              : CPD_CKPT_INFO_NODE - CPD Checkpoint info node
 *
 * Return Values          : None , Error message is logged
 *
 * Notes                  : None 
********************************************************************************************/
void cpd_a2s_ckpt_dest_add(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node, MDS_DEST *dest)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;
	
	TRACE_ENTER();
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	cpd_msg.type = CPD_A2S_MSG_CKPT_DEST_ADD;
	cpd_msg.info.dest_add.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.dest_add.mds_dest = *dest;

	/*  add the destination for the given checkpoint */
	/* send it to MBCSv */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt async update add failed for ckpt_id:%llx dest:%"PRIu64,ckpt_node->ckpt_id,*dest);
	else
		TRACE_2("cpd A2S ckpt async update success for ckpt_id:%llx dest:%"PRIu64,ckpt_node->ckpt_id,*dest);

	TRACE_LEAVE();
}

/*******************************************************************************************
 * Name                   : cpd_a2s_ckpt_dest_down
 * 
 * Description            : Function to delete the destination of the already existing 
 *                          checkpoint
 *
 * Arguments              : CPD_CKPT_INFO_NODE - CPD Checkpoint info node
 *
 * Return Values          : None , Error message is logged
 *
********************************************************************************************/
void cpd_a2s_ckpt_dest_down(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node, MDS_DEST *dest)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;

	TRACE_ENTER();
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	cpd_msg.type = CPD_A2S_MSG_CKPT_DEST_DOWN;
	cpd_msg.info.dest_down.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.dest_down.mds_dest = *dest;

	TRACE("CPND 1 IS IN RESTART NOW ");

	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt async update failed for ckptid:%llx,dest: %"PRIu64,cpd_msg.info.dest_down.ckpt_id, *dest);
	else
		TRACE_1("cpd A2S ckpt async update del success for ckptid:%llx,dest:%"PRIu64,cpd_msg.info.dest_down.ckpt_id, *dest);
	TRACE_LEAVE();
}

/*******************************************************************************************
 * Name                   : cpd_a2s_ckpt_dest_del
 *
 * Description            : Function to delete the destination of the already existing 
 *                          checkpoint
 *
 * Arguments              : CPD_CKPT_INFO_NODE - CPD Checkpoint info node
 *
 * Return Values          : None , Error message is logged
 *
********************************************************************************************/
void cpd_a2s_ckpt_dest_del(CPD_CB *cb, SaCkptCheckpointHandleT ckpt_hdl, MDS_DEST *cpnd_dest, bool ckptid_flag)
{
	CPD_MBCSV_MSG cpd_msg;
	uint32_t rc = SA_AIS_OK;
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));
	
	TRACE_ENTER();
	cpd_msg.type = CPD_A2S_MSG_CKPT_DEST_DEL;
	if (ckptid_flag) {
		cpd_msg.info.dest_del.ckpt_id = 0;
	} else {
		cpd_msg.info.dest_del.ckpt_id = ckpt_hdl;
	}
	cpd_msg.info.dest_del.mds_dest = *cpnd_dest;

	/* Send it to MBCSV */
	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt dest del async update failed for ckpt_id:%llx dest:%"PRIu64,cpd_msg.info.dest_del.ckpt_id, *cpnd_dest);
	else
		TRACE_1("cpd A2S ckpt dest del async update success for ckpt_id:%llx dest:%"PRIu64,cpd_msg.info.dest_del.ckpt_id, *cpnd_dest);
	TRACE_LEAVE();
}

/*****************************************************************************
 * Name  : cpd_a2s_ckpt_usr_info
 *
 * Description :
 *
 ****************************************************************************/
void cpd_a2s_ckpt_usr_info(CPD_CB *cb, CPD_CKPT_INFO_NODE *ckpt_node)
{
	CPD_MBCSV_MSG cpd_msg;
	int count = 0;
	uint32_t rc = SA_AIS_OK;
	memset(&cpd_msg, '\0', sizeof(CPD_MBCSV_MSG));

	TRACE_ENTER();
	cpd_msg.type = CPD_A2S_MSG_CKPT_USR_INFO;
	cpd_msg.info.usr_info_2.ckpt_id = ckpt_node->ckpt_id;
	cpd_msg.info.usr_info_2.num_user = ckpt_node->num_users;
	cpd_msg.info.usr_info_2.num_writer = ckpt_node->num_writers;
	cpd_msg.info.usr_info_2.num_reader = ckpt_node->num_readers;
	cpd_msg.info.usr_info_2.num_sections = ckpt_node->num_sections;
	cpd_msg.info.usr_info_2.ckpt_on_scxb1 = ckpt_node->ckpt_on_scxb1;
	cpd_msg.info.usr_info_2.ckpt_on_scxb2 = ckpt_node->ckpt_on_scxb2;

	if (ckpt_node->node_users_cnt) {
		CPD_NODE_USER_INFO *node_user = ckpt_node->node_users;
		cpd_msg.info.usr_info_2.node_users_cnt = ckpt_node->node_users_cnt;
		cpd_msg.info.usr_info_2.node_list = malloc(ckpt_node->node_users_cnt * sizeof(CPD_NODE_USER_INFO));
		memset(cpd_msg.info.usr_info_2.node_list, '\0', (sizeof(CPD_NODE_USER_INFO) * ckpt_node->node_users_cnt));

		for (count = 0; count < ckpt_node->node_users_cnt; count++) {
			cpd_msg.info.usr_info_2.node_list[count].dest = node_user->dest;
			cpd_msg.info.usr_info_2.node_list[count].num_users = node_user->num_users;
			cpd_msg.info.usr_info_2.node_list[count].num_readers = node_user->num_readers;
			cpd_msg.info.usr_info_2.node_list[count].num_writers = node_user->num_writers;
			node_user = node_user->next;
		}
	}

	rc = cpd_mbcsv_async_update(cb, &cpd_msg);
	if (rc != SA_AIS_OK)
		TRACE_4("cpd A2S ckpt user info async update failed");
	else
		TRACE_1("cpd A2S ckpt user info async update success");
	TRACE_LEAVE();
}
