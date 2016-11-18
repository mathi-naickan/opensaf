
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

/**************************************************************************

Description: This File contains the Common utility routines used by PLMS and PLMA

****************************************************************************/
#include "plms.h"

SaUint32T  plms_free_evt(PLMS_EVT *evt)
{
	if (!evt)
	{
		LOG_ER("Received NULL evt ptr to free");
		return NCSCC_RC_FAILURE;
	}

	if (evt->req_res == PLMS_REQ)
	{
		switch(evt->req_evt.req_type)
		{
			case PLMS_AGENT_LIB_REQ_EVT_T:
			case PLMS_MDS_INFO_EVT_T:
			case PLMS_DUMP_CB_EVT_T:
			case PLMS_IMM_ADM_OP_EVT_T:
				free(evt);
				break;
			case PLMS_AGENT_GRP_OP_EVT_T:
				switch(evt->req_evt.agent_grp_op.grp_evt_type)
				{
					case PLMS_AGENT_GRP_CREATE_EVT:
					case PLMS_AGENT_GRP_DEL_EVT:
						free(evt);
						break;
					case PLMS_AGENT_GRP_ADD_EVT:
					case PLMS_AGENT_GRP_REMOVE_EVT:
						if(evt->req_evt.agent_grp_op.entity_names)
						{
							free(evt->req_evt.agent_grp_op.entity_names);
						}
						free(evt);
						break;
				}
				break;
			case PLMS_AGENT_TRACK_EVT_T:	
				 switch(evt->req_evt.agent_track.evt_type)
				 {
					 
					 case PLMS_AGENT_TRACK_START_EVT:
						free(evt);
						break;
					 case PLMS_AGENT_TRACK_STOP_EVT:
					 	free(evt);
						break;
					 case PLMS_AGENT_TRACK_READINESS_IMPACT_EVT:
					 	if (evt->req_evt.agent_track.readiness_impact.impacted_entity)
						{
							free(evt->req_evt.agent_track.readiness_impact.impacted_entity);
							/* FIXME see if correlation ids needs to be freed */
						}
						free(evt);
						break;
					 case PLMS_AGENT_TRACK_RESPONSE_EVT:
						free(evt);
						break;
					 case PLMS_AGENT_TRACK_CBK_EVT:
					 	if (evt->req_evt.agent_track.track_cbk.tracked_entities.entities)
						{
							free(evt->req_evt.agent_track.track_cbk.tracked_entities.entities);
						}
						free(evt);
						break;
				 }
				 break;
			case PLMS_HPI_EVT_T:
				if (evt->req_evt.hpi_evt.entity_path)
				{
					free(evt->req_evt.hpi_evt.entity_path);
					free(evt);
				}
				break;
			case PLMS_PLMC_EVT_T:
				switch(evt->req_evt.plms_plmc_evt.plmc_evt_type){
				case PLMS_PLMC_EE_INSTING:
				case PLMS_PLMC_EE_INSTED:
				case PLMS_PLMC_EE_TRMTED:
				case PLMS_PLMC_EE_TRMTING:
				case PLMS_PLMC_EE_OS_RESP:
					if(evt->req_evt.plms_plmc_evt.ee_os_info.version)
					{
						free(evt->req_evt.plms_plmc_evt.ee_os_info.version);
					}
					if(evt->req_evt.plms_plmc_evt.ee_os_info.product)
					{
						free(evt->req_evt.plms_plmc_evt.ee_os_info.product);
					}
					if(evt->req_evt.plms_plmc_evt.ee_os_info.vendor)
					{
						free(evt->req_evt.plms_plmc_evt.ee_os_info.vendor);
					}
					if(evt->req_evt.plms_plmc_evt.ee_os_info.release)
					{
						free(evt->req_evt.plms_plmc_evt.ee_os_info.release);
					}
					break;

				case PLMS_PLMC_EE_VER_RESP:
					if (evt->req_evt.plms_plmc_evt.ee_ver)
					{
						free(evt->req_evt.plms_plmc_evt.ee_ver);
					}
					break;	
					
				default:
					break;

				}

				free(evt);
				break;
			case PLMS_TMR_EVT_T:
				break;
			
		}
	} else if (evt->req_res == PLMS_RES)
	{
		switch(evt->res_evt.res_type)
		{
			case PLMS_AGENT_INIT_RES:
			case PLMS_AGENT_FIN_RES:
			case PLMS_AGENT_GRP_CREATE_RES:
			case PLMS_AGENT_GRP_REMOVE_RES:
			case PLMS_AGENT_GRP_ADD_RES:
			case PLMS_AGENT_GRP_DEL_RES:
			case PLMS_AGENT_TRACK_START_RES:
			case PLMS_AGENT_TRACK_STOP_RES:
				free(evt);
				break;
			case PLMS_AGENT_TRACK_READINESS_IMPACT_RES:
				if (evt->res_evt.entities)
				{
					free(evt->res_evt.entities);
				}
				free(evt);
				break;
			default:
				free(evt);
				break;
		}
	}
	return NCSCC_RC_SUCCESS;
		
}
