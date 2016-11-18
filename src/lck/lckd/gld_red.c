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
  FILE NAME: gld_red.c

  DESCRIPTION: This file contains routines to fill and send the A2S messages 

******************************************************************************/
#include "lck/lckd/gld.h"

/*****************************************************************************
 * Name            : glsv_gld_a2s_ckpt_resource 

 * Description     : This routine fills async update message for resource open 
 *                    event
 *
 * Return Values   :
 *
 * None            : None
****************************************************************************/
void glsv_gld_a2s_ckpt_resource(GLSV_GLD_CB *gld_cb, SaNameT *rsc_name, SaLckResourceIdT rsc_id, MDS_DEST mdest_id,
				SaTimeT creation_time)
{
	GLSV_GLD_A2S_CKPT_EVT a2s_evt;
	SaAisErrorT rc = SA_AIS_OK;

	if (gld_cb == NULL) {
		LOG_ER("GLD a2s rsc open async failed");
	} else {
		memset(&a2s_evt, '\0', sizeof(GLSV_GLD_A2S_CKPT_EVT));

		a2s_evt.evt_type = GLSV_GLD_EVT_RSC_OPEN;
		a2s_evt.info.rsc_open_info.rsc_id = rsc_id;
		memcpy(&a2s_evt.info.rsc_open_info.rsc_name, rsc_name, sizeof(SaNameT));
		memcpy(&a2s_evt.info.rsc_open_info.mdest_id, &mdest_id, sizeof(MDS_DEST));
		a2s_evt.info.rsc_open_info.rsc_creation_time = creation_time;

		/* send msg to MBCSv */
		rc = glsv_gld_mbcsv_async_update(gld_cb, &a2s_evt);
		if (rc != SA_AIS_OK)
			LOG_ER("GLD a2s rsc open async failed");
		else
			TRACE_1("GLD a2s rsc open async success");
	}

}

/*****************************************************************************
 * Name            : glsv_gld_a2s_ckpt_node_details 

 * Description     : This routine fills async update message for node details 
 *                    event
 *
 * Return Values   :
 *
 * None            : None
****************************************************************************/
void glsv_gld_a2s_ckpt_node_details(GLSV_GLD_CB *gld_cb, MDS_DEST mdest_id, uint32_t evt_type)
{
	GLSV_GLD_A2S_CKPT_EVT a2s_evt;
	SaAisErrorT rc = SA_AIS_OK;

	memset(&a2s_evt, '\0', sizeof(GLSV_GLD_A2S_CKPT_EVT));

	if (gld_cb != NULL) {
		a2s_evt.evt_type = evt_type;
		memcpy(&a2s_evt.info.glnd_mds_info.mdest_id, &mdest_id, sizeof(MDS_DEST));
		/* send msg to MBCSv */
		rc = glsv_gld_mbcsv_async_update(gld_cb, &a2s_evt);
		if (rc != SA_AIS_OK)
			LOG_ER("GLD a2s rsc node down async failed");
		else
			TRACE_1("GLD a2s rsc node down async success");
	}
}

/*****************************************************************************
 * Name            : glsv_gld_a2s_ckpt_rsc_detail 
 *
 * Description     : This routine fills async update message for resource detail *                   s event for resource close and set_orphan operations 
 *
 * Return Values   :
 *
 * None            : None
****************************************************************************/
void glsv_gld_a2s_ckpt_rsc_details(GLSV_GLD_CB *gld_cb, GLSV_GLD_EVT_TYPE evt_type,
				   GLSV_RSC_DETAILS rsc_details, MDS_DEST mdest_id, uint32_t lcl_ref_cnt)
{
	GLSV_GLD_A2S_CKPT_EVT a2s_evt;
	SaAisErrorT rc = SA_AIS_OK;

	if (gld_cb == NULL) {
		if (evt_type == GLSV_GLD_EVT_RSC_CLOSE)
			LOG_ER("GLD a2s rsc close async failed");
		if (evt_type == GLSV_GLD_EVT_SET_ORPHAN)
			LOG_ER("GLD a2s rsc set orphan async failed");
	} else {
		memset(&a2s_evt, '\0', sizeof(GLSV_GLD_A2S_CKPT_EVT));

		a2s_evt.evt_type = evt_type;
		a2s_evt.info.rsc_details.rsc_id = rsc_details.rsc_id;
		a2s_evt.info.rsc_details.orphan = rsc_details.orphan;
		a2s_evt.info.rsc_details.lck_mode = rsc_details.lck_mode;
		a2s_evt.info.rsc_details.lcl_ref_cnt = lcl_ref_cnt;
		memcpy(&a2s_evt.info.rsc_details.mdest_id, &mdest_id, sizeof(MDS_DEST));

		/* send msg to MBCSv */
		rc = glsv_gld_mbcsv_async_update(gld_cb, &a2s_evt);
		if (rc != SA_AIS_OK) {
			if (evt_type == GLSV_GLD_EVT_RSC_CLOSE)
				LOG_ER("GLD a2s rsc close async failed");
			if (evt_type == GLSV_GLD_EVT_SET_ORPHAN)
				LOG_ER("GLD a2s rsc set orphan async failed");
		} else {
			if (evt_type == GLSV_GLD_EVT_RSC_CLOSE)
				TRACE_1("GLD a2s rsc close async success");
			if (evt_type == GLSV_GLD_EVT_SET_ORPHAN)
				TRACE_1("GLD a2s rsc set orphan async success");
		}
	}
}
