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

  DESCRIPTION: This file contains the SG processing routines for the AVD.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */

#include <stdbool.h>
#include <immutil.h>
#include <logtrace.h>

#include <avd.h>
#include <avd_imm.h>
#include <avd_su.h>
#include <avd_clm.h>

static SaAisErrorT avd_d2n_reboot_snd(AVD_AVND *node);

/*****************************************************************************
 * Function: avd_new_assgn_susi
 *
 * Purpose:  This function creates and assigns the given role to the SUSI
 * relationship and sends the message to the AVND having the
 * SU accordingly. 
 *
 * Input: cb - the AVD control block
 *        su - The pointer to SU.
 *        si - The pointer to SI.
 *      ha_state - The HA role that needs to be assigned.
 *      ckpt - Flag indicating if called on standby.
 *      ret_ptr - Pointer to the pointer of the SUSI structure created.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

uns32 avd_new_assgn_susi(AVD_CL_CB *cb, AVD_SU *su, AVD_SI *si,
			 SaAmfHAStateT ha_state, NCS_BOOL ckpt, AVD_SU_SI_REL **ret_ptr)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *susi;
	AVD_COMP_CSI_REL *compcsi;
	NCS_BOOL l_flag;
	AVD_COMP *l_comp;
	AVD_CSI *l_csi;

	TRACE_ENTER2("'%s' '%s' state=%u", su->name.value, si->name.value, ha_state);

	if ((susi = avd_susi_create(cb, si, su, ha_state, ckpt)) == NULL) {
		LOG_ER("%s: Could not create SUSI '%s' '%s'", __FUNCTION__,
			su->name.value, si->name.value);
		goto done;
	}

	susi->fsm = AVD_SU_SI_STATE_ASGN;
	susi->state = ha_state;

	l_flag = TRUE;

	l_csi = si->list_of_csi;

	/* reset the assign flag */
	l_comp = su->list_of_comp;
	while (l_comp != NULL) {
		l_comp->assign_flag = FALSE;
		l_comp = l_comp->su_comp_next;
	}

	while ((l_csi != NULL) && (l_flag == TRUE)) {
		/* find the component that has to be assigned this CSI */

		l_comp = su->list_of_comp;
		while (l_comp != NULL) {
			if ((l_comp->assign_flag == FALSE) &&
			    (avd_compcstype_find_match(l_csi, l_comp) == NCSCC_RC_SUCCESS))
				break;

			l_comp = l_comp->su_comp_next;
		}

		if (l_comp == NULL) {

			l_flag = FALSE;
			continue;
		}

		if ((compcsi = avd_compcsi_create(susi, l_csi, l_comp, true)) == NULL) {
			/* free all the CSI assignments and end this loop */
			avd_compcsi_delete(cb, susi, TRUE);
			l_flag = FALSE;
			continue;
		}

		l_comp->assign_flag = TRUE;
		l_csi = l_csi->si_list_of_csi_next;
	}			/* while((l_csi != AVD_CSI_NULL) && (l_flag == TRUE)) */

	if (l_flag == FALSE) {
		LOG_ER("%s: Component type missing for SU '%s'", __FUNCTION__, su->name.value);
		LOG_ER("%s: Component type missing for CSI '%s'", __FUNCTION__, l_csi->name.value);

		/* free all the CSI assignments and end this loop */
		avd_compcsi_delete(cb, susi, TRUE);
		/* Unassign the SUSI */
		avd_susi_delete(cb, susi, TRUE);
		goto done;
	}

	/* Now send the message about the SU SI assignment to
	 * the AvND. Send message only if this function is not 
	 * called while doing checkpoint update.
	 */

	if (FALSE == ckpt) {
		if (avd_snd_susi_msg(cb, su, susi, AVSV_SUSI_ACT_ASGN) != NCSCC_RC_SUCCESS) {
			/* free all the CSI assignments and end this loop */
			avd_compcsi_delete(cb, susi, TRUE);
			/* Unassign the SUSI */
			avd_susi_delete(cb, susi, TRUE);

			goto done;
		}

		m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, susi, AVSV_CKPT_AVD_SI_ASS);
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_AVD_SU_CONFIG);
	}

	*ret_ptr = susi;
	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_su_oper_state_func
 *
 * Purpose:  This function is the handler for the operational state change event
 * indicating the arrival of the operational state change message from 
 * node director. It will process the message and call the redundancy model
 * specific event processing routine.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES:
 *
 * 
 **************************************************************************/

void avd_su_oper_state_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;
	AVD_SU *su, *i_su;
	SaAmfReadinessStateT old_state;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("from %x, '%s' state=%u", n2d_msg->msg_info.n2d_opr_state.node_id,
	     n2d_msg->msg_info.n2d_opr_state.su_name.value,
		n2d_msg->msg_info.n2d_opr_state.su_oper_state);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_opr_state.node_id, AVSV_N2D_OPERATION_STATE_MSG,
		n2d_msg->msg_info.n2d_opr_state.msg_id)) == NULL) {
		/* sanity failed return */
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) ||(node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_opr_state.msg_id));

	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_node_ack_msg failed", __FUNCTION__);
	}

	/* Find and validate the SU. */

	/* get the SU from the tree */

	if ((su = avd_su_get(&n2d_msg->msg_info.n2d_opr_state.su_name)) == NULL) {
		LOG_ER("%s: %s not found", __FUNCTION__, n2d_msg->msg_info.n2d_opr_state.su_name.value);
		goto done;
	}

	m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

	/* Verify that the SU and node oper state is diabled and rcvr is failfast */
	if ((n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_DISABLED) &&
	    (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) &&
	    (n2d_msg->msg_info.n2d_opr_state.rec_rcvr.saf_amf == SA_AMF_NODE_FAILFAST)) {
		/* as of now do the same opearation as ncs su failure */
		avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_DISABLED);
		if ((node->type == AVSV_AVND_CARD_SYS_CON) && (node->node_info.nodeId == cb->node_id_avd)) {
			TRACE("Component in %s requested FAILFAST", su->name.value);
		}

		avd_nd_ncs_su_failed(cb, node);
		goto done;
	}

	/* Verify that the SU operation state is disable and do the processing. */
	if (n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
		/* if the SU is NCS SU, call the node FSM routine to handle the failure.
		 */
		if (su->sg_of_su->sg_ncs_spec == SA_TRUE) {
			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_DISABLED);
			avd_nd_ncs_su_failed(cb, node);
			goto done;
		}

		/* If the cluster timer hasnt expired, mark the SU operation state
		 * disabled.      
		 */

		if (cb->init_state == AVD_INIT_DONE) {
			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_DISABLED);
			avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
			if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
				/* Mark the node operational state as disable and make all the
				 * application SUs in the node as O.O.S.
				 */
				avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
				node->recvr_fail_sw = TRUE;
				i_su = node->list_of_su;
				while (i_su != NULL) {
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					i_su = i_su->avnd_list_su_next;
				}
			}	/* if(n2d_msg->msg_info.n2d_opr_state.node_oper_state == NCS_OPER_STATE_DISABLE) */
		} /* if(cb->init_state == AVD_INIT_DONE) */
		else if (cb->init_state == AVD_APP_STATE) {
			avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_DISABLED);
			avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
			if (n2d_msg->msg_info.n2d_opr_state.node_oper_state == SA_AMF_OPERATIONAL_DISABLED) {
				/* Mark the node operational state as disable and make all the
				 * application SUs in the node as O.O.S. Also call the SG FSM
				 * to do the reallignment of SIs for assigned SUs.
				 */
				avd_node_oper_state_set(node, SA_AMF_OPERATIONAL_DISABLED);
				node->recvr_fail_sw = TRUE;
				i_su = node->list_of_su;
				while (i_su != NULL) {
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
						/* Since assignments exists call the SG FSM.
						 */
						switch (i_su->sg_of_su->sg_redundancy_model) {
						case SA_AMF_2N_REDUNDANCY_MODEL:
							if (avd_sg_2n_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
								/* Bad situation. Free the message and return since
								 * receive id was not processed the event will again
								 * comeback which we can then process.
								 */
								LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
								goto done;
							}
							break;

						case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
							if (avd_sg_nacvred_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
								/* Bad situation. Free the message and return since
								 * receive id was not processed the event will again
								 * comeback which we can then process.
								 */
								LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
								goto done;
							}
							break;

						case SA_AMF_N_WAY_REDUNDANCY_MODEL:
							if (avd_sg_nway_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
								/* Bad situation. Free the message and return since
								 * receive id was not processed the event will again
								 * comeback which we can then process.
								 */
								LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
								goto done;
							}
							break;

						case SA_AMF_NPM_REDUNDANCY_MODEL:
							if (avd_sg_npm_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
								/* Bad situation. Free the message and return since
								 * receive id was not processed the event will again
								 * comeback which we can then process.
								 */
								LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
								goto done;
							}
							break;

						case SA_AMF_NO_REDUNDANCY_MODEL:
						default:
							if (avd_sg_nored_su_fault_func(cb, i_su) == NCSCC_RC_FAILURE) {
								/* Bad situation. Free the message and return since
								 * receive id was not processed the event will again
								 * comeback which we can then process.
								 */
								LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
								goto done;
							}
							break;
						}
					}

					/* Verify the SG to check if any instantiations need
					 * to be done for the SG on which this SU exists.
					 */
					if (avd_sg_app_su_inst_func(cb, i_su->sg_of_su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, i_su->name.value);
						goto done;
					}

					i_su = i_su->avnd_list_su_next;
				}	/* while(i_su != AVD_SU_NULL) */

			} else {	/* if(n2d_msg->msg_info.n2d_opr_state.node_oper_state == NCS_OPER_STATE_DISABLE) */

				if (su->list_of_susi != AVD_SU_SI_REL_NULL) {
					/* Since assignments exists call the SG FSM.
					 */
					switch (su->sg_of_su->sg_redundancy_model) {
					case SA_AMF_2N_REDUNDANCY_MODEL:
						if (avd_sg_2n_su_fault_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_N_WAY_REDUNDANCY_MODEL:
						if (avd_sg_nway_su_fault_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_NPM_REDUNDANCY_MODEL:
						if (avd_sg_npm_su_fault_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
						if (avd_sg_nacvred_su_fault_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					case SA_AMF_NO_REDUNDANCY_MODEL:
					default:
						if (avd_sg_nored_su_fault_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					}
				}

				/* Verify the SG to check if any instantiations need
				 * to be done for the SG on which this SU exists.
				 */
				if (avd_sg_app_su_inst_func(cb, su->sg_of_su) == NCSCC_RC_FAILURE) {
					/* Bad situation. Free the message and return since
					 * receive id was not processed the event will again
					 * comeback which we can then process.
					 */
					LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->sg_of_su->name.value);
					goto done;
				}

			}	/*else (n2d_msg->msg_info.n2d_opr_state.node_oper_state == NCS_OPER_STATE_DISABLE) */

		}
		/* else if(cb->init_state == AVD_APP_STATE) */
	} /* if(n2d_msg->msg_info.n2d_opr_state.su_oper_state == NCS_OPER_STATE_DISABLE) */
	else if (n2d_msg->msg_info.n2d_opr_state.su_oper_state == SA_AMF_OPERATIONAL_ENABLED) {
		avd_su_oper_state_set(su, SA_AMF_OPERATIONAL_ENABLED);
		/* if the SU is NCS SU, mark the SU readiness state as in service and call
		 * the SG FSM.
		 */
		if (su->sg_of_su->sg_ncs_spec == SA_TRUE) {
			if (su->saAmfSuReadinessState == SA_AMF_READINESS_OUT_OF_SERVICE) {
				avd_su_readiness_state_set(su, SA_AMF_READINESS_IN_SERVICE);
				/* Run the SG FSM */
				switch (su->sg_of_su->sg_redundancy_model) {
				case SA_AMF_2N_REDUNDANCY_MODEL:
					if (avd_sg_2n_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
						avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
						goto done;
					}
					break;

				case SA_AMF_N_WAY_REDUNDANCY_MODEL:
					if (avd_sg_nway_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
						avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
						goto done;
					}
					break;

				case SA_AMF_NPM_REDUNDANCY_MODEL:
					if (avd_sg_npm_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
						avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
						goto done;
					}
					break;

				case SA_AMF_NO_REDUNDANCY_MODEL:
				default:
					if (avd_sg_nored_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
						/* Bad situation. Free the message and return since
						 * receive id was not processed the event will again
						 * comeback which we can then process.
						 */
						LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
						avd_su_readiness_state_set(su, SA_AMF_READINESS_OUT_OF_SERVICE);
						goto done;
					}
					break;
				}
			} else
				assert(0);
		} else {	/* if(su->sg_of_su->sg_ncs_spec == SA_TRUE) */

			old_state = su->saAmfSuReadinessState;
			m_AVD_GET_SU_NODE_PTR(cb, su, su_node_ptr);

			if (m_AVD_APP_SU_IS_INSVC(su, su_node_ptr)) {
				avd_su_readiness_state_set(su, SA_AMF_READINESS_IN_SERVICE);
				if ((cb->init_state == AVD_APP_STATE) && (old_state == SA_AMF_READINESS_OUT_OF_SERVICE)) {
					/* An application SU has become in service call SG FSM */
					switch (su->sg_of_su->sg_redundancy_model) {
					case SA_AMF_2N_REDUNDANCY_MODEL:
						if (avd_sg_2n_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_N_WAY_REDUNDANCY_MODEL:
						if (avd_sg_nway_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_NPM_REDUNDANCY_MODEL:
						if (avd_sg_npm_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;

					case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
						if (avd_sg_nacvred_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					case SA_AMF_NO_REDUNDANCY_MODEL:
					default:
						if (avd_sg_nored_su_insvc_func(cb, su) == NCSCC_RC_FAILURE) {
							/* Bad situation. Free the message and return since
							 * receive id was not processed the event will again
							 * comeback which we can then process.
							 */
							LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, su->name.value);
							goto done;
						}
						break;
					}
				}
			}
		}
	}

 done:
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_ncs_su_mod_rsp
 *
 * Purpose:  This function is the handler for su si assign response 
 * from the node director for the NCS SU modify  su si assign message.    
 * it verify if all the other NCS SU have also got assigned. If assigned
 * it does the role specific functionality.
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_ncs_su_mod_rsp(AVD_CL_CB *cb, AVD_AVND *avnd, AVSV_N2D_INFO_SU_SI_ASSIGN_MSG_INFO *assign)
{
	AVD_SU *i_su = NULL;
	AVD_AVND *avnd_other = NULL;
	SaBoolT ncs_done = SA_TRUE;
	uns32 rc = NCSCC_RC_SUCCESS;

	TRACE_ENTER();

	/* Active -> Quiesced && resp = Success */
	if ((cb->avail_state_avd == SA_AMF_HA_QUIESCED) &&
	    (assign->ha_state == SA_AMF_HA_QUIESCED) && (assign->error == NCSCC_RC_SUCCESS)) {
		i_su = avnd->list_of_ncs_su;
		while (i_su != NULL) {
			if ((i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
			    (i_su->list_of_susi->fsm != AVD_SU_SI_STATE_ASGND)) {
				ncs_done = SA_FALSE;
				break;
			}

			i_su = i_su->avnd_list_su_next;
		}

		if (ncs_done == SA_TRUE) {
			/* If other AvD is present and we are able to set mds role */
			if ((cb->node_id_avd_other != 0) &&
			    (NCSCC_RC_SUCCESS == avd_mds_set_vdest_role(cb, SA_AMF_HA_QUIESCED))) {
				/* We need to send the role to AvND. */
				rc = avd_avnd_send_role_change(cb, cb->node_id_avd, SA_AMF_HA_QUIESCED);
				if (NCSCC_RC_SUCCESS != rc) {
					LOG_ER("%s: avd_avnd_send_role_change failed", __FUNCTION__);
				} else {
					/* we should send the above data verify msg right now */
					avd_d2n_msg_dequeue(cb);
				}

				goto done;
			}

			/*  We failed to switch, Send Active to all NCS Su's having 2N redun model &
			   present in this node */
			cb->avail_state_avd = SA_AMF_HA_ACTIVE;

			for (i_su = avnd->list_of_ncs_su; i_su != NULL; i_su = i_su->avnd_list_su_next) {
				if ((i_su->list_of_susi != 0) &&
				    (i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
				    (i_su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
					m_AVD_SET_SG_FSM(cb, (i_su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_ACTIVE);
				}
			}
		}
		/* ncs_done == SA_TRUE */
		goto done;
	}

	/* Active -> Quiesed && resp = Failure */
	if ((cb->avail_state_avd == SA_AMF_HA_QUIESCED) &&
	    (assign->ha_state == SA_AMF_HA_QUIESCED) && (assign->error == NCSCC_RC_FAILURE)) {
		cb->avail_state_avd = SA_AMF_HA_ACTIVE;

		for (i_su = avnd->list_of_ncs_su; i_su != NULL; i_su = i_su->avnd_list_su_next) {
			if ((i_su->list_of_susi != 0) &&
			    (i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
			    (i_su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
				m_AVD_SET_SG_FSM(cb, (i_su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_ACTIVE);
			}
		}		/* for */

		goto done;
	}

	/* Standby -> Active && Success */
	if ((cb->avail_state_avd == SA_AMF_HA_ACTIVE) &&
	    (assign->ha_state == SA_AMF_HA_ACTIVE) && (assign->error == NCSCC_RC_SUCCESS)) {
		i_su = avnd->list_of_ncs_su;
		while (i_su != NULL) {
			if ((i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
			    (i_su->list_of_susi->fsm != AVD_SU_SI_STATE_ASGND)) {
				ncs_done = SA_FALSE;
				break;
			}

			i_su = i_su->avnd_list_su_next;
		}

		if (ncs_done == SA_TRUE) {
			assert(avd_clm_track_start() == SA_AIS_OK);

			/* get the avnd on other SCXB from node_id of other AvD */
			if (NULL == (avnd_other = avd_node_find_nodeid(cb->node_id_avd_other))) {
				LOG_ER("%s: cannot find other avd", __FUNCTION__);
				goto done;
			}

			/* Now change All the NCS SU's of the other SCXB which should be
			   in Quiesed state to Standby */

			for (i_su = avnd_other->list_of_ncs_su; i_su != NULL; i_su = i_su->avnd_list_su_next) {
				if ((i_su->list_of_susi != 0) &&
				    (i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
				    (i_su->list_of_susi->state == SA_AMF_HA_QUIESCED)) {
					avd_sg_su_si_mod_snd(cb, i_su, SA_AMF_HA_STANDBY);
					avd_sg_su_oper_list_add(cb, i_su, FALSE);
					m_AVD_SET_SG_FSM(cb, (i_su->sg_of_su), AVD_SG_FSM_SG_REALIGN);
				}
			}

			/* Switch the APP SU's who have are Active on other node and has 2N redund */
			i_su = avnd_other->list_of_su;
			while (i_su != NULL) {
				if ((i_su->sg_of_su->sg_redundancy_model == SA_AMF_2N_REDUNDANCY_MODEL) &&
				    (i_su->sg_of_su->sg_fsm_state == AVD_SG_FSM_STABLE) &&
				    (i_su->list_of_susi != 0) && (i_su->list_of_susi->state == SA_AMF_HA_ACTIVE)) {
					m_AVD_SET_SU_SWITCH(cb, i_su, AVSV_SI_TOGGLE_SWITCH);

					if (avd_sg_2n_suswitch_func(cb, i_su) != NCSCC_RC_SUCCESS) {
						m_AVD_SET_SU_SWITCH(cb, i_su, AVSV_SI_TOGGLE_STABLE);
					}
				}

				i_su = i_su->avnd_list_su_next;
			}
		}

		goto done;
	}

	/* Standby -> Active && resp = Failure */
	/* We are expecting a su faiolver for this ncs su */

done:
	TRACE_LEAVE();
}

static void susi_assign_msg_dump(const char *func, unsigned int line,
	AVSV_N2D_INFO_SU_SI_ASSIGN_MSG_INFO *info)
{
	LOG_ER("%s:%d %u %u %u %u %x", func, line, info->error,
		info->ha_state, info->msg_act, info->msg_id, info->node_id);
	LOG_ER("%s:%d %s", func, line, info->si_name.value);
	LOG_ER("%s:%d %s", func, line, info->su_name.value);
}

/*****************************************************************************
 * Function: avd_su_si_assign_func
 *
 * Purpose:  This function is the handler for su si assign event
 * indicating the arrival of the response to the su si assign message from
 * node director. It then call the redundancy model specific routine to process
 * this event. 
 *
 * Input: cb - the AVD control block
 *        evt - The event information.
 *
 * Returns: None.
 *
 * NOTES: None.
 *
 * 
 **************************************************************************/

void avd_su_si_assign_evh(AVD_CL_CB *cb, AVD_EVT *evt)
{
	AVD_DND_MSG *n2d_msg = evt->info.avnd_msg;
	AVD_AVND *node;
	AVD_SU *su = NULL, *temp_su;
	AVD_SU_SI_REL *susi;
	NCS_BOOL q_flag = FALSE, qsc_flag = FALSE, all_su_unassigned = TRUE;

	TRACE_ENTER2("%x", n2d_msg->msg_info.n2d_su_si_assign.node_id);

	if ((node = avd_msg_sanity_chk(evt, n2d_msg->msg_info.n2d_su_si_assign.node_id, AVSV_N2D_INFO_SU_SI_ASSIGN_MSG,
	     n2d_msg->msg_info.n2d_su_si_assign.msg_id)) == NULL) {
		/* sanity failed return */
		avsv_dnd_msg_free(n2d_msg);
		evt->info.avnd_msg = NULL;
		goto done;
	}

	if ((node->node_state == AVD_AVND_STATE_ABSENT) || (node->node_state == AVD_AVND_STATE_GO_DOWN)) {
		LOG_ER("%s: invalid node state %u", __FUNCTION__, node->node_state);
		goto done;
	}

	/* update the receive id count */
	m_AVD_SET_AVND_RCV_ID(cb, node, (n2d_msg->msg_info.n2d_su_si_assign.msg_id));

	/* 
	 * Send the Ack message to the node, indicationg that the message with this
	 * message ID is received successfully.
	 */
	if (avd_snd_node_ack_msg(cb, node, node->rcv_msg_id) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_node_ack_msg failed", __FUNCTION__);
	}

	if (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) {

		/* get the SU from the tree since this is across the
		 * SU operation. 
		 */

		if ((su = avd_su_get(&n2d_msg->msg_info.n2d_su_si_assign.su_name)) == NULL) {
			LOG_ER("%s:%d %s", __FUNCTION__, __LINE__, n2d_msg->msg_info.n2d_su_si_assign.su_name.value);
			goto done;
		}

		if (su->list_of_susi == AVD_SU_SI_REL_NULL) {
			LOG_ER("%s: no susis", __FUNCTION__);
			goto done;
		}

		TRACE("%u", n2d_msg->msg_info.n2d_su_si_assign.msg_act);
		switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) {
		case AVSV_SUSI_ACT_DEL:
			break;

		case AVSV_SUSI_ACT_MOD:
			/* Verify that the SUSI is in the modify state for the same HA state. */
			susi = su->list_of_susi;
			while (susi != AVD_SU_SI_REL_NULL) {
				if ((susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)
				    && (susi->state != SA_AMF_HA_QUIESCING)
				    && (n2d_msg->msg_info.n2d_su_si_assign.ha_state != SA_AMF_HA_QUIESCED)
				    && (susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					/* some other event has caused further state change ignore
					 * this message, by accepting the receive id and  droping the message.
					 * message id has already been accepted above.
					 */

					avsv_dnd_msg_free(n2d_msg);
					evt->info.avnd_msg = NULL;
					return;
				} else if ((susi->state == SA_AMF_HA_QUIESCING)
					   && (susi->fsm != AVD_SU_SI_STATE_UNASGN)) {
					qsc_flag = TRUE;
				}

				susi = susi->su_next;

			}	/* while (susi != AVD_SU_SI_REL_NULL) */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_QUIESCING) {
					q_flag = TRUE;
					avd_sg_su_asgn_del_util(cb, su, FALSE, FALSE);
				} else {
					if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_ACTIVE) {
						if (su->saAmfSUNumCurrStandbySIs != 0)
							su->saAmfSUNumCurrActiveSIs = su->saAmfSUNumCurrStandbySIs;
						su->saAmfSUNumCurrStandbySIs = 0;
					} else if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_STANDBY) {
						su->saAmfSUNumCurrStandbySIs = su->saAmfSUNumCurrActiveSIs;
						su->saAmfSUNumCurrActiveSIs = 0;
					}

					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_AVD_SU_CONFIG);

					/* set the  assigned or quiesced state in the SUSIs. */
					avd_sg_su_asgn_del_util(cb, su, FALSE, qsc_flag);
				}
			}
			break;
		default:
			LOG_ER("%s: invalid act %u", __FUNCTION__, n2d_msg->msg_info.n2d_su_si_assign.msg_act);
			goto done;
			break;
		}		/* switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) */

		{
			/* Call the redundancy model specific procesing function. Dont call
			 * in case of acknowledgment for quiescing.
			 */
			switch (su->sg_of_su->sg_redundancy_model) {
			case SA_AMF_2N_REDUNDANCY_MODEL:
				/* Now process the acknowledge message based on
				 * Success or failure.
				 */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (q_flag == FALSE) {
						avd_sg_2n_susi_sucss_func(cb, su, AVD_SU_SI_REL_NULL,
									  n2d_msg->msg_info.n2d_su_si_assign.msg_act,
									  n2d_msg->msg_info.n2d_su_si_assign.ha_state);
					}
				} else {
					susi_assign_msg_dump(__FUNCTION__, __LINE__,
						&n2d_msg->msg_info.n2d_su_si_assign);
					avd_sg_2n_susi_fail_func(cb, su, AVD_SU_SI_REL_NULL,
								 n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								 n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
				break;

			case SA_AMF_N_WAY_REDUNDANCY_MODEL:
				/* Now process the acknowledge message based on
				 * Success or failure.
				 */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (q_flag == FALSE) {
						avd_sg_nway_susi_sucss_func(cb, su, AVD_SU_SI_REL_NULL,
									    n2d_msg->msg_info.n2d_su_si_assign.msg_act,
									    n2d_msg->msg_info.
									    n2d_su_si_assign.ha_state);
					}
				} else {
					susi_assign_msg_dump(__FUNCTION__, __LINE__,
						&n2d_msg->msg_info.n2d_su_si_assign);
					avd_sg_nway_susi_fail_func(cb, su, AVD_SU_SI_REL_NULL,
								   n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								   n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
				break;

			case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
				/* Now process the acknowledge message based on
				 * Success or failure.
				 */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (q_flag == FALSE) {
						avd_sg_nacvred_susi_sucss_func(cb, su, AVD_SU_SI_REL_NULL,
									       n2d_msg->msg_info.
									       n2d_su_si_assign.msg_act,
									       n2d_msg->msg_info.
									       n2d_su_si_assign.ha_state);
					}
				} else {
					susi_assign_msg_dump(__FUNCTION__, __LINE__,
						&n2d_msg->msg_info.n2d_su_si_assign);
					avd_sg_nacvred_susi_fail_func(cb, su, AVD_SU_SI_REL_NULL,
								      n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								      n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
				break;

			case SA_AMF_NPM_REDUNDANCY_MODEL:
				/* Now process the acknowledge message based on
				 * Success or failure.
				 */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (q_flag == FALSE) {
						avd_sg_npm_susi_sucss_func(cb, su, AVD_SU_SI_REL_NULL,
									   n2d_msg->msg_info.n2d_su_si_assign.msg_act,
									   n2d_msg->msg_info.n2d_su_si_assign.ha_state);
					}
				} else {
					susi_assign_msg_dump(__FUNCTION__, __LINE__,
						&n2d_msg->msg_info.n2d_su_si_assign);
					avd_sg_npm_susi_fail_func(cb, su, AVD_SU_SI_REL_NULL,
								  n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								  n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
				break;

			case SA_AMF_NO_REDUNDANCY_MODEL:
			default:
				/* Now process the acknowledge message based on
				 * Success or failure.
				 */
				if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
					if (q_flag == FALSE) {
						avd_sg_nored_susi_sucss_func(cb, su, AVD_SU_SI_REL_NULL,
									     n2d_msg->msg_info.n2d_su_si_assign.msg_act,
									     n2d_msg->msg_info.
									     n2d_su_si_assign.ha_state);
					}
				} else {
					susi_assign_msg_dump(__FUNCTION__, __LINE__,
						&n2d_msg->msg_info.n2d_su_si_assign);
					avd_sg_nored_susi_fail_func(cb, su, AVD_SU_SI_REL_NULL,
								    n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								    n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
				break;
			}
		}

	} else {		/* if (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) */

		/* Single SU SI assignment find the SU SI structure */

		if ((susi = avd_susi_find(cb, &n2d_msg->msg_info.n2d_su_si_assign.su_name,
			&n2d_msg->msg_info.n2d_su_si_assign.si_name)) == AVD_SU_SI_REL_NULL) {

			/* Acknowledgement for a deleted SU SI ignore the message */
			LOG_IN("%s: avd_susi_find failed for %s %s", __FUNCTION__,
				n2d_msg->msg_info.n2d_su_si_assign.su_name.value,
				n2d_msg->msg_info.n2d_su_si_assign.si_name.value);
			goto done;
		}

		TRACE("%u", n2d_msg->msg_info.n2d_su_si_assign.msg_act);
		switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) {
		case AVSV_SUSI_ACT_DEL:
			break;

		case AVSV_SUSI_ACT_ASGN:
			/* Verify that the SUSI is in the assign state for the same HA state. */
			if ((susi->fsm != AVD_SU_SI_STATE_ASGN) ||
			    (susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)) {
				/* some other event has caused further state change ignore
				 * this message, by accepting the receive id and
				 * droping the message.
				 */
				LOG_IN("%s: assign susi not in proper state %u %u %u", __FUNCTION__,
					susi->fsm, susi->state, n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				LOG_IN("%s: %s %s", __FUNCTION__,
					susi->su->name.value, susi->si->name.value);
				goto done;
			}

			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				susi->fsm = AVD_SU_SI_STATE_ASGND;
				m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);

				/* trigger pg upd */
				avd_pg_susi_chg_prc(cb, susi);
			}
			break;

		case AVSV_SUSI_ACT_MOD:
			/* Verify that the SUSI is in the modify state for the same HA state. */
			if ((susi->fsm != AVD_SU_SI_STATE_MODIFY) ||
			    ((susi->state != n2d_msg->msg_info.n2d_su_si_assign.ha_state)
			     && (susi->state != SA_AMF_HA_QUIESCING)
			     && (n2d_msg->msg_info.n2d_su_si_assign.ha_state != SA_AMF_HA_QUIESCED))) {
				/* some other event has caused further state change ignore
				 * this message, by accepting the receive id and
				 * droping the message.
				 */

				/* log Info error that the susi mentioned is not in proper state. */
				LOG_IN("%s: mod susi not in proper state %u %u %u", __FUNCTION__,
					susi->fsm, susi->state, n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				LOG_IN("%s: %s %s", __FUNCTION__,
					susi->su->name.value, susi->si->name.value);
				goto done;
			}

			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_QUIESCING) {
					q_flag = TRUE;
					susi->fsm = AVD_SU_SI_STATE_ASGND;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				} else {
					if (susi->state == SA_AMF_HA_QUIESCING) {
						susi->state = SA_AMF_HA_QUIESCED;
						m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
						avd_gen_su_ha_state_changed_ntf(cb, susi);
					} else {
						if (n2d_msg->msg_info.n2d_su_si_assign.ha_state == SA_AMF_HA_ACTIVE) {
							if (susi->su->saAmfSUNumCurrStandbySIs != 0) {
								avd_su_inc_curr_act_si(susi->su);
								avd_su_dec_curr_stdby_si(susi->su);
								avd_si_inc_curr_act_dec_std_ass(susi->si);
							}

						} else if (n2d_msg->msg_info.n2d_su_si_assign.ha_state ==
							   SA_AMF_HA_STANDBY) {
							avd_su_dec_curr_act_si(susi->su);
							avd_su_inc_curr_stdby_si(susi->su);
							avd_si_inc_curr_stdby_dec_act_ass(susi->si);
						}
					}

					/* set the assigned in the SUSIs. */
					susi->fsm = AVD_SU_SI_STATE_ASGND;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, susi, AVSV_CKPT_AVD_SI_ASS);
				}

				/* trigger pg upd */
				avd_pg_susi_chg_prc(cb, susi);
			}
			break;

		default:
			LOG_ER("%s: invalid action %u", __FUNCTION__, n2d_msg->msg_info.n2d_su_si_assign.msg_act);
			goto done;
			break;
		}		/* switch (n2d_msg->msg_info.n2d_su_si_assign.msg_act) */

		/* Call the redundancy model specific procesing function Dont call
		 * in case of acknowledgment for quiescing.
		 */

		switch (susi->si->sg_of_si->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			/* Now process the acknowledge message based on
			 * Success or failure.
			 */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (q_flag == FALSE) {
					avd_sg_2n_susi_sucss_func(cb, susi->su, susi,
								  n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								  n2d_msg->msg_info.n2d_su_si_assign.ha_state);
					if ((n2d_msg->msg_info.n2d_su_si_assign.msg_act == AVSV_SUSI_ACT_ASGN)
					    && (susi->su->sg_of_su->sg_ncs_spec == SA_TRUE)) {
						/* Since a NCS SU has been assigned trigger the node FSM. */
						/* For (ncs_spec == SA_TRUE), su will not be external, so su
						   will have node attached. */
						avd_nd_ncs_su_assigned(cb, susi->su->su_on_node);
					}

				}
			} else {
				avd_sg_2n_susi_fail_func(cb, susi->su, susi,
							 n2d_msg->msg_info.n2d_su_si_assign.msg_act,
							 n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			/* Now process the acknowledge message based on
			 * Success or failure.
			 */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (q_flag == FALSE) {
					avd_sg_nway_susi_sucss_func(cb, susi->su, susi,
								    n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								    n2d_msg->msg_info.n2d_su_si_assign.ha_state);
					if ((n2d_msg->msg_info.n2d_su_si_assign.msg_act == AVSV_SUSI_ACT_ASGN)
					    && (susi->su->sg_of_su->sg_ncs_spec == SA_TRUE)) {
						/* Since a NCS SU has been assigned trigger the node FSM. */
						/* For (ncs_spec == SA_TRUE), su will not be external, so su
						   will have node attached. */
						avd_nd_ncs_su_assigned(cb, susi->su->su_on_node);
					}
				}
			} else {
				avd_sg_nway_susi_fail_func(cb, susi->su, susi,
							   n2d_msg->msg_info.n2d_su_si_assign.msg_act,
							   n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			/* Now process the acknowledge message based on
			 * Success or failure.
			 */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (q_flag == FALSE) {
					avd_sg_nacvred_susi_sucss_func(cb, susi->su, susi,
								       n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								       n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
			} else {
				avd_sg_nacvred_susi_fail_func(cb, susi->su, susi,
							      n2d_msg->msg_info.n2d_su_si_assign.msg_act,
							      n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			/* Now process the acknowledge message based on
			 * Success or failure.
			 */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (q_flag == FALSE) {
					avd_sg_npm_susi_sucss_func(cb, susi->su, susi,
								   n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								   n2d_msg->msg_info.n2d_su_si_assign.ha_state);
				}
			} else {
				avd_sg_npm_susi_fail_func(cb, susi->su, susi,
							  n2d_msg->msg_info.n2d_su_si_assign.msg_act,
							  n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}
			break;

		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			/* Now process the acknowledge message based on
			 * Success or failure.
			 */
			if (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS) {
				if (q_flag == FALSE) {
					avd_sg_nored_susi_sucss_func(cb, susi->su, susi,
								     n2d_msg->msg_info.n2d_su_si_assign.msg_act,
								     n2d_msg->msg_info.n2d_su_si_assign.ha_state);
					if ((n2d_msg->msg_info.n2d_su_si_assign.msg_act == AVSV_SUSI_ACT_ASGN)
					    && (susi->su->sg_of_su->sg_ncs_spec == SA_TRUE)) {
						/* Since a NCS SU has been assigned trigger the node FSM. */
						/* For (ncs_spec == SA_TRUE), su will not be external, so su
						   will have node attached. */
						avd_nd_ncs_su_assigned(cb, susi->su->su_on_node);
					}

				}
			} else {
				avd_sg_nored_susi_fail_func(cb, susi->su, susi,
							    n2d_msg->msg_info.n2d_su_si_assign.msg_act,
							    n2d_msg->msg_info.n2d_su_si_assign.ha_state);
			}

			break;

		}		/* switch(susi->si->sg_of_si->su_redundancy_model) */

	}			/* else (n2d_msg->msg_info.n2d_su_si_assign.si_name.length == 0) */

	/* If there is any admin action going on SU and it is complete then send its result to admin.
	   Lock/Shutdown is successful if all SIs have been unassigned. Unlock is successful if
	   SI could be assigned to SU successfully if there was any. The operation failed if
	   AvND encountered error while assigning/unassigning SI to the SU. */

	su = avd_su_get(&n2d_msg->msg_info.n2d_su_si_assign.su_name);

	if (su != NULL) {
		if (su->pend_cbk.invocation != 0) {
			if ((su->pend_cbk.admin_oper == SA_AMF_ADMIN_LOCK)
			    || (su->pend_cbk.admin_oper == SA_AMF_ADMIN_SHUTDOWN)) {
				if ((su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0)) {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
									    SA_AIS_OK);
					su->pend_cbk.invocation = 0;
					su->pend_cbk.admin_oper = 0;
				} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
									    SA_AIS_ERR_REPAIR_PENDING);
					su->pend_cbk.invocation = 0;
					su->pend_cbk.admin_oper = 0;
				}
				/* else lock is still not complete so don't send result. */
			} else if (su->pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK) {
				if (((su->saAmfSUNumCurrActiveSIs != 0) || (su->saAmfSUNumCurrStandbySIs != 0)) &&
				    (n2d_msg->msg_info.n2d_su_si_assign.error == NCSCC_RC_SUCCESS)) {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
									    SA_AIS_OK);
					su->pend_cbk.invocation = 0;
					su->pend_cbk.admin_oper = 0;
				} else {
					immutil_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
									    SA_AIS_ERR_TIMEOUT);
					su->pend_cbk.invocation = 0;
					su->pend_cbk.admin_oper = 0;
				}
			}
		} else if (su->su_on_node->admin_node_pend_cbk.invocation != 0) {
			/* decrement the SU count on the node undergoing admin operation  
			   when all SIs have been unassigned for a SU on the node undergoing 
			   LOCK/SHUTDOWN or when successful SI assignment has happened for 
			   a SU on the node undergoing UNLOCK */
			if ((((su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_LOCK) ||
			      (su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_SHUTDOWN) ||
			      (su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK_INSTANTIATION)) &&
			     (su->saAmfSUNumCurrActiveSIs == 0) && (su->saAmfSUNumCurrStandbySIs == 0)) ||
			    ((su->su_on_node->admin_node_pend_cbk.admin_oper == SA_AMF_ADMIN_UNLOCK) &&
			     ((su->saAmfSUNumCurrActiveSIs != 0) || (su->saAmfSUNumCurrStandbySIs != 0)))) {
				su->su_on_node->su_cnt_admin_oper--;
			}

			/* if this last su to undergo admin operation then report to IMM */
			if (su->su_on_node->su_cnt_admin_oper == 0) {
				immutil_saImmOiAdminOperationResult(cb->immOiHandle,
								    su->su_on_node->admin_node_pend_cbk.invocation,
								    SA_AIS_OK);
				su->su_on_node->admin_node_pend_cbk.invocation = 0;
				su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
			} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
				immutil_saImmOiAdminOperationResult(cb->immOiHandle, su->pend_cbk.invocation,
								    SA_AIS_ERR_REPAIR_PENDING);
				su->su_on_node->admin_node_pend_cbk.invocation = 0;
				su->su_on_node->admin_node_pend_cbk.admin_oper = 0;
				su->su_on_node->su_cnt_admin_oper = 0;
			}
			/* else admin oper still not complete */
		}
		/* also check for pending clm callback operations */ 
		if (su->su_on_node->clm_pend_inv != 0) {
			if((su->saAmfSUNumCurrActiveSIs == 0) && 
					(su->saAmfSUNumCurrStandbySIs == 0))
				su->su_on_node->su_cnt_admin_oper--;
			if (su->su_on_node->su_cnt_admin_oper == 0) {
				/* since unassignment of all SIs on this node has been done
				   now go on with the terminataion */
				/* clm admin lock/shutdown operations were on, so we need to reset 
				   node admin state.*/
				su->su_on_node->saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
				clm_node_terminate(su->su_on_node);
			} else if (n2d_msg->msg_info.n2d_su_si_assign.error != NCSCC_RC_SUCCESS) {
				/* clm admin lock/shutdown operations were on, so we need to reset 
				   node admin state.*/
				su->su_on_node->saAmfNodeAdminState = SA_AMF_ADMIN_UNLOCKED;
				/* just report error to clm let CLM take the action */
				saClmResponse_4(cb->clmHandle, su->su_on_node->clm_pend_inv, SA_CLM_CALLBACK_RESPONSE_ERROR);
				su->su_on_node->clm_pend_inv = 0;
			} /* else wait for some more time */
		}
	}

	/* Check whether the node belonging to this su is disable and susi of all su got removed.
	   This is the case of node_failover/node_switchover. */
	if ((SA_AMF_OPERATIONAL_DISABLED == node->saAmfNodeOperState) && (TRUE == node->recvr_fail_sw)) {

		/* We are checking only application components as on payload all ncs comp are in no_red model.
		   We are doing the same thing for controller also. */
		temp_su = node->list_of_su;
		while (temp_su) {
			if (NULL != temp_su->list_of_susi) {
				all_su_unassigned = FALSE;
			}
			temp_su = temp_su->avnd_list_su_next;
		}
		if (TRUE == all_su_unassigned) {
			/* All app su got unassigned, Safe to reboot the blade now. */
			/* Check if it matches with controller id, then rather than sending to amfnd, reboot from here*/
			if (cb->node_id_avd == node->node_info.nodeId) {
				opensaf_reboot(node->node_info.nodeId, (char *)node->node_info.executionEnvironment.value,
						"Rebooting the node because of Node Failover/Switchover.");
			} else {
				avd_d2n_reboot_snd(node);
			}
		}
	}
	/* Free the messages */
	avsv_dnd_msg_free(n2d_msg);
	evt->info.avnd_msg = NULL;

 done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_app_node_su_inst_func
 *
 * Purpose:  This function processes the request to instantiate all the
 *           application SUs on a node. If the
 *           AvD is in AVD_INIT_DONE state i.e the AMF timer hasnt expired
 *           all the pre-instantiable SUs will be instantiated by sending presence
 *           message. The non pre-instantiable SUs operation state will be made
 *           enable. If the state is AVD_APP_STATE, then for each SU the
 *           corresponding SG will be evaluated for instantiations.
 *
 * Input: cb - the AVD control block
 *        avnd - The pointer to the node whose application SUs need to
 *               be instantiated.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

void avd_sg_app_node_su_inst_func(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *i_su;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("'%s'", avnd->name.value);

	if (avnd->saAmfNodeAdminState == SA_AMF_ADMIN_LOCKED_INSTANTIATION) {
		TRACE("Node is in SA_AMF_ADMIN_LOCKED_INSTANTIATION state, can't instantiate");
		goto done;
	}

	if (cb->init_state == AVD_INIT_DONE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {
			if ((i_su->num_of_comp == i_su->curr_num_comp) &&
			    (i_su->term_state == FALSE) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (i_su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			    (i_su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION)) {
				if (i_su->saAmfSUPreInstantiable == TRUE) {
					/* instantiate all the pre-instatiable SUs */
					avd_snd_presence_msg(cb, i_su, FALSE);
				} else {
					/* mark the non preinstatiable as enable. */
					avd_su_oper_state_set(i_su, SA_AMF_OPERATIONAL_ENABLED);

					m_AVD_GET_SU_NODE_PTR(cb, i_su, su_node_ptr);

					if (m_AVD_APP_SU_IS_INSVC(i_su, su_node_ptr)) {
						avd_su_readiness_state_set(i_su, SA_AMF_READINESS_IN_SERVICE);
					}
				}
			}

			i_su = i_su->avnd_list_su_next;
		}

	} else if (cb->init_state == AVD_APP_STATE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {
			if ((i_su->num_of_comp == i_su->curr_num_comp) &&
			    (i_su->term_state == FALSE) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED)) {
				/* Look at the SG and do the instantiations. */
				avd_sg_app_su_inst_func(cb, i_su->sg_of_su);
			}

			i_su = i_su->avnd_list_su_next;
		}
	}
done:
	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_app_su_inst_func
 *
 * Purpose:  This function processes the request to evaluate the SG for
 *           Instantiations and terminations of SUs in the SG. This routine
 *           is used only when AvD is in AVD_APP_STATE state i.e the AMF 
 *           timer has expired. It Instantiates all the high ranked 
 *           pre-instantiable SUs and for all non pre-instanble SUs if
 *           they are in uninstantiated state, the operation state will
 *           be made active. Once the preffered inservice SU count is
 *           meet all the instatiated,unassigned pre-instantiated SUs will
 *           be terminated. 
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to SG whose SUs need to be instantiated.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: Only if the AvD is in AVD_APP_STATE this routine should be used.
 *
 * 
 **************************************************************************/

uns32 avd_sg_app_su_inst_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	uns32 num_insvc_su = 0;
	uns32 num_asgd_su = 0;
	uns32 num_su = 0;
	uns32 num_try_insvc_su = 0;
	AVD_SU *i_su;
	AVD_AVND *su_node_ptr = NULL;

	TRACE_ENTER2("'%s'", sg->name.value);

	i_su = sg->list_of_su;
	while (i_su != NULL) {
		m_AVD_GET_SU_NODE_PTR(cb, i_su, su_node_ptr);
		num_su++;
		/* Check if the SU is inservice */
		if (i_su->saAmfSuReadinessState == SA_AMF_READINESS_IN_SERVICE) {
			num_insvc_su++;
			if ((i_su->list_of_susi == AVD_SU_SI_REL_NULL) &&
			    (i_su->saAmfSUPreInstantiable == TRUE) &&
			    (i_su->saAmfSUAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
			    (sg->saAmfSGNumPrefInserviceSUs < (num_insvc_su + num_try_insvc_su))) {
				/* enough inservice SUs are already there terminate this
				 * SU.
				 */
				if (avd_snd_presence_msg(cb, i_su, TRUE) == NCSCC_RC_SUCCESS) {
					/* mark the SU operation state as disable and readiness state
					 * as out of service.
					 */
					avd_su_oper_state_set(i_su, SA_AMF_OPERATIONAL_DISABLED);
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					num_insvc_su--;
				}
			} else if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
				num_asgd_su++;
			}
		} /* if(i_su->readiness_state == NCS_IN_SERVICE) */
		else if (i_su->num_of_comp == i_su->curr_num_comp) {
			/* if the SU is non preinstantiable and if the node operational state
			 * is enable. Put the SU in service.
			 */
			if ((i_su->saAmfSUPreInstantiable == FALSE) &&
			    (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
			    (su_node_ptr->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
			    (i_su->term_state == FALSE)) {
				avd_su_oper_state_set(i_su, SA_AMF_OPERATIONAL_ENABLED);
				m_AVD_GET_SU_NODE_PTR(cb, i_su, su_node_ptr);

				if (m_AVD_APP_SU_IS_INSVC(i_su, su_node_ptr)) {
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_IN_SERVICE);
					switch (i_su->sg_of_su->sg_redundancy_model) {
					case SA_AMF_2N_REDUNDANCY_MODEL:
						avd_sg_2n_su_insvc_func(cb, i_su);
						break;

					case SA_AMF_N_WAY_REDUNDANCY_MODEL:
						avd_sg_nway_su_insvc_func(cb, i_su);
						break;

					case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
						avd_sg_nacvred_su_insvc_func(cb, i_su);
						break;

					case SA_AMF_NPM_REDUNDANCY_MODEL:
						avd_sg_npm_su_insvc_func(cb, i_su);
						break;

					case SA_AMF_NO_REDUNDANCY_MODEL:
					default:
						avd_sg_nored_su_insvc_func(cb, i_su);
						break;
					}
					if (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {
						num_asgd_su++;
					}
					num_insvc_su++;
				}

			} else if ((i_su->saAmfSUPreInstantiable == TRUE) &&
				   (sg->saAmfSGNumPrefInserviceSUs > (num_insvc_su + num_try_insvc_su)) &&
				   (i_su->saAmfSUPresenceState == SA_AMF_PRESENCE_UNINSTANTIATED) &&
				   ((i_su->saAmfSUAdminState == SA_AMF_ADMIN_UNLOCKED) ||
				    (i_su->saAmfSUAdminState == SA_AMF_ADMIN_LOCKED)) &&
				   (i_su->sg_of_su->saAmfSGAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
				   (i_su->su_on_node->saAmfNodeAdminState != SA_AMF_ADMIN_LOCKED_INSTANTIATION) &&
				   (su_node_ptr->saAmfNodeOperState == SA_AMF_OPERATIONAL_ENABLED) &&
				   (i_su->term_state == FALSE)) {
				/* Try to Instantiate this SU */
				if (avd_snd_presence_msg(cb, i_su, FALSE) == NCSCC_RC_SUCCESS) {
					num_try_insvc_su++;
				}
			}
		}
		/* else if (i_su->num_of_comp == i_su->curr_num_comp) */
		i_su = i_su->sg_list_su_next;

	}			/* while (i_su != AVD_SU_NULL) */

	/* The entire SG has been scanned for reinstatiations and terminations.
	 * Fill the numbers gathered into the SG.
	 */
	sg->saAmfSGNumCurrAssignedSUs = num_asgd_su;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_SU_ASSIGNED_NUM);

	sg->saAmfSGNumCurrInstantiatedSpareSUs = num_insvc_su - num_asgd_su;
	m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, sg, AVSV_CKPT_SG_SU_SPARE_NUM);

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_app_sg_admin_func
 *
 * Purpose:  This function processes the request to do UNLOCK or LOCK or shutdown
 * of the AMF application SUs on the SG. It first verifies that the
 * SGs is stable and then it sets the readiness 
 * state of each of the SU on the node and calls the SG FSM for the
 * SG.
 *
 * Input: cb - the AVD control block
 *        sg - The pointer to the sg which needs to be administratively modified.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uns32 avd_sg_app_sg_admin_func(AVD_CL_CB *cb, AVD_SG *sg)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SU *i_su;
	AVD_AVND *i_su_node_ptr = NULL;

	TRACE_ENTER();

	/* Based on the admin operation that is been done call the corresponding.
	 * Redundancy model specific functionality for the SG.
	 */

	switch (sg->saAmfSGAdminState) {
	case SA_AMF_ADMIN_UNLOCKED:
		/* Dont allow UNLOCK if the SG FSM is not stable. */
		if (sg->sg_fsm_state != AVD_SG_FSM_STABLE)
			return NCSCC_RC_FAILURE;
		/* For each of the SUs calculate the readiness state. This routine is called
		 * only when AvD is in AVD_APP_STATE. call the SG FSM with the new readiness
		 * state.
		 */

		i_su = sg->list_of_su;
		while (i_su != NULL) {
			m_AVD_GET_SU_NODE_PTR(cb, i_su, i_su_node_ptr);

			if (m_AVD_APP_SU_IS_INSVC(i_su, i_su_node_ptr)) {
				avd_su_readiness_state_set(i_su, SA_AMF_READINESS_IN_SERVICE);
			}
			/* get the next SU on the node */
			i_su = i_su->sg_list_su_next;
		}

		switch (sg->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			if (avd_sg_2n_realign_func(cb, sg) == NCSCC_RC_FAILURE) {
				/* set all the SUs to OOS return failure */
				i_su = sg->list_of_su;
				while (i_su != NULL) {
					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					/* get the next SU of the SG */
					i_su = i_su->sg_list_su_next;
				}

				return NCSCC_RC_FAILURE;
			}	/* if (avd_sg_2n_realign_func(cb,sg) == NCSCC_RC_FAILURE) */
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			if (avd_sg_nway_realign_func(cb, sg) == NCSCC_RC_FAILURE) {
				/* set all the SUs to OOS return failure */
				i_su = sg->list_of_su;
				while (i_su != NULL) {

					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					/* get the next SU of the SG */
					i_su = i_su->sg_list_su_next;
				}

				return NCSCC_RC_FAILURE;
			}	/* if (avd_sg_nway_realign_func(cb,sg) == NCSCC_RC_FAILURE) */
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			if (avd_sg_npm_realign_func(cb, sg) == NCSCC_RC_FAILURE) {
				/* set all the SUs to OOS return failure */
				i_su = sg->list_of_su;
				while (i_su != NULL) {

					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					/* get the next SU of the SG */
					i_su = i_su->sg_list_su_next;
				}

				return NCSCC_RC_FAILURE;
			}	/* if (avd_sg_nway_realign_func(cb,sg) == NCSCC_RC_FAILURE) */
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			if (avd_sg_nacvred_realign_func(cb, sg) == NCSCC_RC_FAILURE) {
				/* set all the SUs to OOS return failure */
				i_su = sg->list_of_su;
				while (i_su != NULL) {

					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					/* get the next SU of the SG */
					i_su = i_su->sg_list_su_next;
				}

				return NCSCC_RC_FAILURE;
			}	/* if (avd_sg_nacvred_realign_func(cb,sg) == NCSCC_RC_FAILURE) */
			break;
		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			if (avd_sg_nored_realign_func(cb, sg) == NCSCC_RC_FAILURE) {
				/* set all the SUs to OOS return failure */
				i_su = sg->list_of_su;
				while (i_su != NULL) {

					avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
					/* get the next SU of the SG */
					i_su = i_su->sg_list_su_next;
				}

				return NCSCC_RC_FAILURE;
			}	/* if (avd_sg_nored_realign_func(cb,sg) == NCSCC_RC_FAILURE) */
			break;
		}

		break;		/* case NCS_ADMIN_STATE_UNLOCK: */
	case SA_AMF_ADMIN_LOCKED:
	case SA_AMF_ADMIN_SHUTTING_DOWN:

		if ((sg->sg_fsm_state != AVD_SG_FSM_STABLE) && (sg->sg_fsm_state != AVD_SG_FSM_SG_ADMIN))
			return NCSCC_RC_FAILURE;

		switch (sg->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			if (avd_sg_2n_sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			if (avd_sg_nway_sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			if (avd_sg_npm_sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			if (avd_sg_nacvred_sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
				return NCSCC_RC_FAILURE;
			}
			break;

		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			if (avd_sg_nored_sg_admin_down(cb, sg) == NCSCC_RC_FAILURE) {
				return NCSCC_RC_FAILURE;
			}
			break;
		}

		i_su = sg->list_of_su;

		while (i_su != NULL) {
			avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);
			/* get the next SU of the SG */
			i_su = i_su->sg_list_su_next;
		}
		break;		/* case NCS_ADMIN_STATE_LOCK: case NCS_ADMIN_STATE_SHUTDOWN: */
	default:
		LOG_ER("%s: invalid adm state %u", __FUNCTION__, sg->saAmfSGAdminState);
		goto done;
		break;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_node_susi_fail_func
 *
 * Purpose:  This function is called to un assign all the SUSIs on
 * the node after the node is found to be down. This function Makes all 
 * the SUs on the node as O.O.S and failover all the SUSI assignments based 
 * on their service groups. It will then delete all the SUSI assignments 
 * corresponding to the SUs on this node if any left.
 *
 * Input: cb - the AVD control block
 *        avnd - The AVND pointer of the node whose SU SI assignments need
 *                to be deleted.
 *
 * Returns: None.
 *
 * NOTES: none.
 * 
 **************************************************************************/

void avd_node_susi_fail_func(AVD_CL_CB *cb, AVD_AVND *avnd)
{
	AVD_SU *i_su;
	AVD_COMP *i_comp;
	AVD_SU_SI_REL *l_susi;

	TRACE_ENTER();

	/* run through all the MW SUs, make all of them O.O.S. Set
	 * assignments for the MW SGs of which the SUs are members. Also
	 * Set the operation state and presence state for the SUs and components to
	 * disable and uninstantiated.  All the functionality for MW SUs is done in
	 * one loop as more than one MW SU per SG in one node is not supported.
	 */

	i_su = avnd->list_of_ncs_su;
	assert(i_su != 0);
	while (i_su != NULL) {
		avd_su_oper_state_set(i_su, SA_AMF_OPERATIONAL_DISABLED);
		avd_su_pres_state_set(i_su, SA_AMF_PRESENCE_UNINSTANTIATED);
		avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);

		/* Check if there was any admin operations going on this SU. */
		if (i_su->pend_cbk.invocation != 0) {
			immutil_saImmOiAdminOperationResult(cb->immOiHandle, i_su->pend_cbk.invocation,
							    SA_AIS_ERR_TIMEOUT);
			i_su->pend_cbk.invocation = 0;
			i_su->pend_cbk.admin_oper = 0;
		}


                /* Check if there was any admin operations going on SIs. */
                for (l_susi = i_su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
                        if (l_susi->si->invocation != 0) {
                                immutil_saImmOiAdminOperationResult(cb->immOiHandle, l_susi->si->invocation,
                                                SA_AIS_ERR_TIMEOUT);
                                l_susi->si->invocation = 0;
                        }
                }

		i_comp = i_su->list_of_comp;
		while (i_comp != NULL) {
			i_comp->curr_num_csi_actv = 0;
			i_comp->curr_num_csi_stdby = 0;
			avd_comp_oper_state_set(i_comp, SA_AMF_OPERATIONAL_DISABLED);
			avd_comp_pres_state_set(i_comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			i_comp->saAmfCompRestartCount = 0;
			if (i_comp->admin_pend_cbk.invocation != 0) {
				immutil_saImmOiAdminOperationResult(cb->immOiHandle, i_comp->admin_pend_cbk.invocation, SA_AIS_ERR_TIMEOUT);
				i_comp->admin_pend_cbk.invocation = 0;
				i_comp->admin_pend_cbk.admin_oper = 0;
			}
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_comp, AVSV_CKPT_AVD_COMP_CONFIG);
			i_comp = i_comp->su_comp_next;
		}

		switch (i_su->sg_of_su->sg_redundancy_model) {
		case SA_AMF_2N_REDUNDANCY_MODEL:
			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			avd_sg_2n_node_fail_func(cb, i_su);
			break;

		case SA_AMF_N_WAY_REDUNDANCY_MODEL:
			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			avd_sg_nway_node_fail_func(cb, i_su);
			break;

		case SA_AMF_NPM_REDUNDANCY_MODEL:
			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			avd_sg_npm_node_fail_func(cb, i_su);
			break;

		case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			avd_sg_nacvred_node_fail_func(cb, i_su);
			break;
		case SA_AMF_NO_REDUNDANCY_MODEL:
		default:
			/* Now analyze the service group for the new HA state
			 * assignments and send the SU SI assign messages
			 * accordingly.
			 */
			avd_sg_nored_node_fail_func(cb, i_su);
			break;
		}

		/* Free all the SU SI assignments for all the SIs on the
		 * the SU if there are any.
		 */

		while (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {

			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, i_su->list_of_susi, FALSE);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, i_su->list_of_susi);
		}

		i_su->saAmfSUNumCurrActiveSIs = 0;
		i_su->saAmfSUNumCurrStandbySIs = 0;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_su, AVSV_CKPT_AVD_SU_CONFIG);

		i_su = i_su->avnd_list_su_next;

	}			/* while (i_su != AVD_SU_NULL) */

	/* send pending callback for this node if any */
	if (avnd->admin_node_pend_cbk.invocation != 0) {
		LOG_WA("Response to admin callback due to node fail");
		immutil_saImmOiAdminOperationResult(cb->immOiHandle, avnd->admin_node_pend_cbk.invocation,
						    SA_AIS_ERR_REPAIR_PENDING);
		avnd->admin_node_pend_cbk.invocation = 0;
		avnd->admin_node_pend_cbk.admin_oper = 0;
		avnd->su_cnt_admin_oper = 0;
	}

	/* Run through the list of application SUs make all of them O.O.S. 
	 */
	i_su = avnd->list_of_su;
	while (i_su != NULL) {
		avd_su_oper_state_set(i_su, SA_AMF_OPERATIONAL_DISABLED);
		avd_su_pres_state_set(i_su, SA_AMF_PRESENCE_UNINSTANTIATED);
		avd_su_readiness_state_set(i_su, SA_AMF_READINESS_OUT_OF_SERVICE);

		/* Check if there was any admin operations going on this SU. */
		if (i_su->pend_cbk.invocation != 0) {
			immutil_saImmOiAdminOperationResult(cb->immOiHandle, i_su->pend_cbk.invocation,
							    SA_AIS_ERR_TIMEOUT);
			i_su->pend_cbk.invocation = 0;
			i_su->pend_cbk.admin_oper = 0;
		}

                /* Check if there was any admin operations going on SIs. */
                for (l_susi = i_su->list_of_susi; l_susi != NULL; l_susi = l_susi->su_next) {
                        if (l_susi->si->invocation != 0) {
                                immutil_saImmOiAdminOperationResult(cb->immOiHandle, l_susi->si->invocation,
                                                SA_AIS_ERR_TIMEOUT);
                                l_susi->si->invocation = 0;
                        }
                }

		i_comp = i_su->list_of_comp;
		while (i_comp != NULL) {
			i_comp->curr_num_csi_actv = 0;
			i_comp->curr_num_csi_stdby = 0;
			avd_comp_oper_state_set(i_comp, SA_AMF_OPERATIONAL_DISABLED);
			avd_comp_pres_state_set(i_comp, SA_AMF_PRESENCE_UNINSTANTIATED);
			i_comp->saAmfCompRestartCount = 0;
			if (i_comp->admin_pend_cbk.invocation != 0) {
				immutil_saImmOiAdminOperationResult(cb->immOiHandle, i_comp->admin_pend_cbk.invocation, SA_AIS_ERR_TIMEOUT);
				i_comp->admin_pend_cbk.invocation = 0;
				i_comp->admin_pend_cbk.admin_oper = 0;
			}
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_comp, AVSV_CKPT_AVD_COMP_CONFIG);
			i_comp = i_comp->su_comp_next;
		}

		i_su = i_su->avnd_list_su_next;
	}			/* while (i_su != AVD_SU_NULL) */

	/* If the AvD is in AVD_APP_STATE run through all the application SUs and 
	 * reassign all the SUSI assignments for the SG of which the SU is a member
	 */

	if (cb->init_state == AVD_APP_STATE) {
		i_su = avnd->list_of_su;
		while (i_su != NULL) {
			switch (i_su->sg_of_su->sg_redundancy_model) {
			case SA_AMF_2N_REDUNDANCY_MODEL:
				/* Now analyze the service group for the new HA state
				 * assignments and send the SU SI assign messages
				 * accordingly.
				 */
				avd_sg_2n_node_fail_func(cb, i_su);
				break;

			case SA_AMF_N_WAY_REDUNDANCY_MODEL:
				/* Now analyze the service group for the new HA state
				 * assignments and send the SU SI assign messages
				 * accordingly.
				 */
				avd_sg_nway_node_fail_func(cb, i_su);
				break;

			case SA_AMF_NPM_REDUNDANCY_MODEL:
				/* Now analyze the service group for the new HA state
				 * assignments and send the SU SI assign messages
				 * accordingly.
				 */
				avd_sg_npm_node_fail_func(cb, i_su);
				break;

			case SA_AMF_N_WAY_ACTIVE_REDUNDANCY_MODEL:
				/* Now analyze the service group for the new HA state
				 * assignments and send the SU SI assign messages
				 * accordingly.
				 */
				avd_sg_nacvred_node_fail_func(cb, i_su);
				break;

			case SA_AMF_NO_REDUNDANCY_MODEL:
			default:
				/* Now analyze the service group for the new HA state
				 * assignments and send the SU SI assign messages
				 * accordingly.
				 */
				avd_sg_nored_node_fail_func(cb, i_su);
				break;
			}
			/* Free all the SU SI assignments for all the SIs on the
			 * the SU if there are any.
			 */

			while (i_su->list_of_susi != AVD_SU_SI_REL_NULL) {

				/* free all the CSI assignments  */
				avd_compcsi_delete(cb, i_su->list_of_susi, FALSE);
				/* Unassign the SUSI */
				m_AVD_SU_SI_TRG_DEL(cb, i_su->list_of_susi);
			}

			i_su->saAmfSUNumCurrActiveSIs = 0;
			i_su->saAmfSUNumCurrStandbySIs = 0;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_su, AVSV_CKPT_AVD_SU_CONFIG);

			/* Since a SU has gone out of service relook at the SG to
			 * re instatiate and terminate SUs if needed.
			 */
			avd_sg_app_su_inst_func(cb, i_su->sg_of_su);

			i_su = i_su->avnd_list_su_next;

		}		/* while (i_su != AVD_SU_NULL) */

	}

	TRACE_LEAVE();
}

/*****************************************************************************
 * Function: avd_sg_su_oper_list_add
 *
 * Purpose:  This function adds the SU to the list of SUs undergoing
 * operation. It allocates the holding structure.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU.
 *        ckpt - TRUE - add is called from ckpt update.
 *               FALSE - add is called from fsm.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uns32 avd_sg_su_oper_list_add(AVD_CL_CB *cb, AVD_SU *su, NCS_BOOL ckpt)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	AVD_SG_OPER **i_su_opr;

	TRACE_ENTER2("'%s'", su->name.value);

	/* Check that the current pointer in the SG is empty and not same as
	 * the SU to be added.
	 */
	if (su->sg_of_su->su_oper_list.su == NULL) {
		su->sg_of_su->su_oper_list.su = su;

		if (!ckpt)
			m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);
		goto done;
	}

	if (su->sg_of_su->su_oper_list.su == su) {
		TRACE("already added");
		goto done;
	}

	i_su_opr = &su->sg_of_su->su_oper_list.next;
	while (*i_su_opr != NULL) {
		if ((*i_su_opr)->su == su) {
			TRACE("already added");
			goto done;
		}
		i_su_opr = &((*i_su_opr)->next);
	}

	/* Allocate the holder structure for having the pointer to the SU */
	*i_su_opr = malloc(sizeof(AVD_SG_OPER));
	if (*i_su_opr == NULL) {
		LOG_ER("%s: malloc failed", __FUNCTION__);
		assert(0);
	}

	/* Fill the content */
	(*i_su_opr)->su = su;
	(*i_su_opr)->next = NULL;

	if (!ckpt)
		m_AVSV_SEND_CKPT_UPDT_ASYNC_ADD(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);

done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_su_oper_list_del
 *
 * Purpose:  This function deletes the SU from the list of SUs undergoing
 * operation. It frees the holding structure.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU.
 *        ckpt - TRUE - add is called from ckpt update.
 *               FALSE - add is called from fsm.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uns32 avd_sg_su_oper_list_del(AVD_CL_CB *cb, AVD_SU *su, NCS_BOOL ckpt)
{
	AVD_SG_OPER **i_su_opr, *temp_su_opr;

	TRACE_ENTER();

	if (su->sg_of_su->su_oper_list.su == NULL) {
		LOG_ER("%s: su_oper_list empty", __FUNCTION__);
		return NCSCC_RC_SUCCESS;
	}

	if (su->sg_of_su->su_oper_list.su == su) {
		if (su->sg_of_su->su_oper_list.next != NULL) {
			temp_su_opr = su->sg_of_su->su_oper_list.next;
			su->sg_of_su->su_oper_list.su = temp_su_opr->su;
			su->sg_of_su->su_oper_list.next = temp_su_opr->next;
			temp_su_opr->next = NULL;
			temp_su_opr->su = NULL;
			free(temp_su_opr);
		} else {
			su->sg_of_su->su_oper_list.su = NULL;
		}

		if (!ckpt)
			m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);
		return NCSCC_RC_SUCCESS;
	}

	i_su_opr = &su->sg_of_su->su_oper_list.next;
	while (*i_su_opr != NULL) {
		if ((*i_su_opr)->su == su) {
			temp_su_opr = *i_su_opr;
			*i_su_opr = temp_su_opr->next;
			temp_su_opr->next = NULL;
			temp_su_opr->su = NULL;
			free(temp_su_opr);

			if (!ckpt)
				m_AVSV_SEND_CKPT_UPDT_ASYNC_RMV(cb, su, AVSV_CKPT_AVD_SG_OPER_SU);

			return NCSCC_RC_SUCCESS;
		}

		i_su_opr = &((*i_su_opr)->next);
	}

	TRACE_LEAVE();
	return NCSCC_RC_FAILURE;
}

/*****************************************************************************
 * Function: avd_sg_su_asgn_del_util
 *
 * Purpose:  This function is a utility routine that changes the assigning or
 * modifing FSM to assigned for all the SUSIs for the SU. If delete it removes
 * all the SUSIs assigned to the SU.    
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU.
 *        del_flag - The delete flag indicating if this is a delete.
 *        q_flag - The flag indicating if the HA state needs to be changed to
 *                 quiesced.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/

uns32 avd_sg_su_asgn_del_util(AVD_CL_CB *cb, AVD_SU *su, NCS_BOOL del_flag, NCS_BOOL q_flag)
{
	AVD_SU_SI_REL *i_susi;

	TRACE_ENTER();

	i_susi = su->list_of_susi;
	if (del_flag == TRUE) {
		while (su->list_of_susi != AVD_SU_SI_REL_NULL) {
			/* free all the CSI assignments  */
			avd_compcsi_delete(cb, su->list_of_susi, FALSE);
			/* Unassign the SUSI */
			m_AVD_SU_SI_TRG_DEL(cb, su->list_of_susi);
		}

		su->saAmfSUNumCurrStandbySIs = 0;
		su->saAmfSUNumCurrActiveSIs = 0;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, su, AVSV_CKPT_AVD_SU_CONFIG);
	} else {
		if (q_flag == TRUE) {
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if (i_susi->fsm != AVD_SU_SI_STATE_UNASGN) {
					i_susi->state = SA_AMF_HA_QUIESCED;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
					avd_gen_su_ha_state_changed_ntf(cb, i_susi);

					/* trigger pg upd */
					avd_pg_susi_chg_prc(cb, i_susi);

				}

				i_susi = i_susi->su_next;
			}

		} else {
			while (i_susi != AVD_SU_SI_REL_NULL) {
				if (i_susi->fsm != AVD_SU_SI_STATE_UNASGN) {
					i_susi->fsm = AVD_SU_SI_STATE_ASGND;
					m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);

					/* trigger pg upd */
					avd_pg_susi_chg_prc(cb, i_susi);
				}

				/* update the si counters */
				if (SA_AMF_HA_ACTIVE == i_susi->state) {
					avd_si_inc_curr_act_dec_std_ass(i_susi->si);
				} else if (SA_AMF_HA_STANDBY == i_susi->state) {
					avd_si_inc_curr_stdby_dec_act_ass(i_susi->si);
				}

				i_susi = i_susi->su_next;
			}
		}
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}

/*****************************************************************************
 * Function: avd_sg_su_si_mod_snd
 *
 * Purpose:  This function is a utility function that assigns the state specified
 * to all the SUSIs that are assigned to the SU. If a failure happens it will
 * revert back to the orginal state.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU whose SUSIs needs to be modified.
 *        state - The HA state to which the state need to be modified. 
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This utility is used by 2N and N-way actice redundancy models.
 *
 * 
 **************************************************************************/

uns32 avd_sg_su_si_mod_snd(AVD_CL_CB *cb, AVD_SU *su, SaAmfHAStateT state)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *i_susi;
	SaAmfHAStateT old_ha_state = SA_AMF_HA_ACTIVE;
	AVD_SU_SI_STATE old_state = AVD_SU_SI_STATE_ASGN;

	TRACE_ENTER2("'%s', state %u", su->name.value, state);

	/* change the state for all assignments to the specified state. */
	i_susi = su->list_of_susi;
	while (i_susi != AVD_SU_SI_REL_NULL) {

		if (i_susi->fsm == AVD_SU_SI_STATE_UNASGN) {
			/* Ignore the SU SI that are getting deleted. */
			i_susi = i_susi->su_next;
			continue;
		}

		/* All The SU SIs will be in the same state */

		old_ha_state = i_susi->state;
		old_state = i_susi->fsm;

		i_susi->state = state;
		i_susi->fsm = AVD_SU_SI_STATE_MODIFY;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
		i_susi = i_susi->su_next;
	}

	/* Now send a single message about the SU SI assignment to
	 * the AvND for all the SIs assigned to the SU.
	 */
	if (avd_snd_susi_msg(cb, su, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_MOD) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_susi_msg failed, %s", __FUNCTION__, su->name.value);
		i_susi = su->list_of_susi;
		while (i_susi != AVD_SU_SI_REL_NULL) {

			if (i_susi->fsm == AVD_SU_SI_STATE_UNASGN) {
				/* Ignore the SU SI that are getting deleted. */
				i_susi = i_susi->su_next;
				continue;
			}

			i_susi->state = old_ha_state;
			i_susi->fsm = old_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
			i_susi = i_susi->su_next;
		}

		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/*****************************************************************************
 * Function: avd_sg_su_si_del_snd
 *
 * Purpose:  This function is a utility function that makes all the SUSIs that
 * are assigned to the SU as unassign. If a failure happens it will
 * revert back to the orginal state.
 *
 * Input: cb - the AVD control block
 *        su - The pointer to the SU whose SUSIs needs to be modified.
 *        
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 *
 * NOTES: This utility is used by 2N and N-way actice redundancy models.
 *
 * 
 **************************************************************************/

uns32 avd_sg_su_si_del_snd(AVD_CL_CB *cb, AVD_SU *su)
{
	uns32 rc = NCSCC_RC_FAILURE;
	AVD_SU_SI_REL *i_susi;
	AVD_SU_SI_STATE old_state = AVD_SU_SI_STATE_ASGN;

	TRACE_ENTER2("'%s'", su->name.value);

	/* change the state for all assignments to the specified state. */
	i_susi = su->list_of_susi;
	while (i_susi != AVD_SU_SI_REL_NULL) {
		old_state = i_susi->fsm;
		i_susi->fsm = AVD_SU_SI_STATE_UNASGN;
		m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
		i_susi = i_susi->su_next;
	}

	/* Now send a single delete message about the SU SI assignment to
	 * the AvND for all the SIs assigned to the SU.
	 */
	if (avd_snd_susi_msg(cb, su, AVD_SU_SI_REL_NULL, AVSV_SUSI_ACT_DEL) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: avd_snd_susi_msg failed, %s", __FUNCTION__, su->name.value);
		i_susi = su->list_of_susi;
		while (i_susi != AVD_SU_SI_REL_NULL) {
			i_susi->fsm = old_state;
			m_AVSV_SEND_CKPT_UPDT_ASYNC_UPDT(cb, i_susi, AVSV_CKPT_AVD_SI_ASS);
			i_susi = i_susi->su_next;
		}

		goto done;
	}

	rc = NCSCC_RC_SUCCESS;
done:
	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avd_d2n_reboot_snd
                        
  Description   : This is a routine sends reboot command to amfnd. 
 
  Arguments     : node:  Node director node to which this message
                         will be sent.
                                
  Return Values : OK/ERROR 
 
  Notes         : None.         
*****************************************************************************/
static SaAisErrorT avd_d2n_reboot_snd(AVD_AVND *node) 
{
	SaAisErrorT rc = SA_AIS_OK;

	TRACE("Sending %u to %x", AVSV_D2N_REBOOT_MSG, node->node_info.nodeId);

	/* Send reboot request to amfnd to reboot that node. */
	AVD_DND_MSG *d2n_msg;

	if ((d2n_msg = calloc(1, sizeof(AVSV_DND_MSG))) == NULL) {
		LOG_ER("%s: calloc failed", __FUNCTION__);
		return SA_AIS_ERR_NO_MEMORY;
	}

	d2n_msg->msg_type = AVSV_D2N_REBOOT_MSG;
	d2n_msg->msg_info.d2n_reboot_info.node_id = node->node_info.nodeId;
	d2n_msg->msg_info.d2n_reboot_info.msg_id = ++(node->snd_msg_id);

	/* Now send the message to the node director */
	if (avd_d2n_msg_snd(avd_cb, node, d2n_msg) != NCSCC_RC_SUCCESS) {
		LOG_ER("%s: snd to %x failed", __FUNCTION__, node->node_info.nodeId);
		avsv_dnd_msg_free(d2n_msg);
		rc = SA_AIS_ERR_FAILED_OPERATION;
	}
	return rc;
}
