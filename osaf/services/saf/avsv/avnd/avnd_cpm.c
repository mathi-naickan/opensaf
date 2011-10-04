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
  
  This module deals with the creation, accessing and deletion of the passive 
  monitoring records and lists on the AVND. 

..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/

#include <sys/types.h>
#include <signal.h>

#include "avnd.h"
#include "avnd_mon.h"

/*****************************************************************************
 *                                                                           *
 *          FUNCTIONS FOR MAINTAINING COMP_PM_REC LIST                       *
 *                                                                           *
 *****************************************************************************/

/****************************************************************************
  Name          : avnd_pm_list_init
 
  Description   : This routine initializes the pm_list.
 
  Arguments     : cb  - ptr to the AvND control block
 
  Return Values : None
 
  Notes         : None.
******************************************************************************/
void avnd_pm_list_init(AVND_COMP *comp)
{
	NCS_DB_LINK_LIST *pm_list = &comp->pm_list;

	/* initialize the pm_list dll  */
	pm_list->order = NCS_DBLIST_ANY_ORDER;
	pm_list->cmp_cookie = avsv_dblist_uns64_cmp;
	pm_list->free_cookie = avnd_pm_rec_free;
}

/****************************************************************************
  Name          : avnd_pm_rec_free
 
  Description   : This routine free the memory alloced to the specified 
                  record in the pm_list.
 
  Arguments     : node - ptr to the dll node
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_pm_rec_free(NCS_DB_LINK_LIST_NODE *node)
{
	AVND_COMP_PM_REC *pm_rec = (AVND_COMP_PM_REC *)node;

	free(pm_rec);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avnd_comp_pm_rec_add
 
  Description   : This routine adds the pm_rec to the corresponding component's 
                  pm_list.
 
  Arguments     : node - ptr to the dll node
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_pm_rec_add(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_PM_REC *rec)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/*update the key */
	rec->comp_dll_node.key = (uns8 *)&rec->pid;

	/* add rec */
	rc = ncs_db_link_list_add(&comp->pm_list, &rec->comp_dll_node);
	if (NCSCC_RC_SUCCESS != rc)
		return rc;

	/* add the record to the mon req list */
	if(avnd_mon_req_add(cb, rec) == NULL) {
		ncs_db_link_list_remove(&comp->pm_list, rec->comp_dll_node.key);
		return NCSCC_RC_FAILURE;
	}

	return rc;
}


/****************************************************************************
  Name          : avnd_comp_pm_rec_del
 
  Description   : This routine deletes the passive monitoring record  
                  maintained by the component.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr the the component
                  rec  - ptr to healthcheck record
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_pm_rec_del(AVND_CB *cb, AVND_COMP *comp, AVND_COMP_PM_REC *rec)
{
	uns32 rc = NCSCC_RC_SUCCESS;
	SaUint64T pid = rec->pid;

	/* delete the PM_REC from pm_list */
	rc = ncs_db_link_list_del(&comp->pm_list, (uns8 *)&rec->pid);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_NO("PM Rec doesn't exist in Comp '%s' of pid %llu", comp->name.value, pid);
	}
	rec = NULL;		/* rec is no more, dont use it */

	/* remove the corresponding element from mon_req list */
	rc = avnd_mon_req_del(cb, pid);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_NO("PM Rec doesn't exist in cb for Comp '%s' of pid %llu", comp->name.value, pid);
	}

	return;
}

/****************************************************************************
  Name          : avnd_comp_pm_rec_del_all
 
  Description   : This routine clears all the pm records.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the component
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_pm_rec_del_all(AVND_CB *cb, AVND_COMP *comp)
{
	AVND_COMP_PM_REC *rec = 0;

	/* No passive monitoring for external component. */
	if (TRUE == comp->su->su_is_external)
		return;

	/* scan & delete each pm_rec record */
	while (0 != (rec = (AVND_COMP_PM_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->pm_list)))
		avnd_comp_pm_rec_del(cb, comp, rec);

	return;
}

/*****************************************************************************
 *                                                                           *
 *          FUNCTIONS FOR MANAGING PMSTART AND PM STOP EVT                   *
 *                                                                           *
 *****************************************************************************/

/****************************************************************************
  Name          : avnd_comp_pm_stop_process
 
  Description   : This routine processes the pm stop by stoping the mon. and
                  deleting the tables and records  
 
  Arguments     : cb       - ptr to the AvND control block
                  comp     - ptr the the component
                  pm_stop  - ptr to the pm stop parameters
                  sa_err   - ptr to sa_err
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : As of Now AvSv does not validate the stop qualifiers and
                  other param. It just stops the PM for the matching PID
******************************************************************************/
uns32 avnd_comp_pm_stop_process(AVND_CB *cb, AVND_COMP *comp, AVSV_AMF_PM_STOP_PARAM *pm_stop, SaAisErrorT *sa_err)
{
	AVND_COMP_PM_REC *rec = 0;
	SaUint64T pid;
	uns32 rc = NCSCC_RC_SUCCESS;

	/* get the pid */
	pid = pm_stop->pid;

	/* search the existing list for the pid in the comp's PM_rec */
	rec = (AVND_COMP_PM_REC *)ncs_db_link_list_find(&comp->pm_list, (uns8 *)&pid);

	/* this rec has to be present */
	assert(rec);

	/* del the pm_rec */
	avnd_comp_pm_rec_del(cb, comp, rec);

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_pm_start_process
 
  Description   : This routine checks the param and either starts a new mon req  
                  or modify an existing mon req.
 
  Arguments     : cb        - ptr to the AvND control block
                  comp      - ptr the the component
                  pm_start  - ptr to the pm start parameters
                  sa_err    - ptr to sa_err
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_pm_start_process(AVND_CB *cb, AVND_COMP *comp, AVSV_AMF_PM_START_PARAM *pm_start, SaAisErrorT *sa_err)
{
	AVND_COMP_PM_REC *rec = 0;
	SaUint64T pid;
	uns32 rc = NCSCC_RC_SUCCESS;
	*sa_err = SA_AIS_OK;

	/* get the pid */
	pid = pm_start->pid;

	/* search the existing list for the pid in the comp's PM_rec */
	rec = (AVND_COMP_PM_REC *)ncs_db_link_list_find(&comp->pm_list, (uns8 *)&pid);
	if (rec) {		/* is rec present or not */
		/* compare the parametrs for PM start */
		if (0 != avnd_comp_pm_rec_cmp(pm_start, rec)) {
			rc = avnd_comp_pmstart_modify(cb, pm_start, rec, sa_err);
			return rc;
		} else
			return rc;	/* nothing to be done */
	}

	/* no previous rec found, its a fresh mon request */
	rec = (AVND_COMP_PM_REC *)avnd_comp_new_rsrc_mon(cb, comp, pm_start, sa_err);
	if (!rec)
		rc = NCSCC_RC_FAILURE;

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_pmstart_modify
 
  Description   : This routine modifies pm record for the component and calls
                  modify the mon req
  Arguments     : cb        - ptr to the AvND control block
                  pm_start  - ptr to the pm start parameters
                  rec       - ptr to the PM_REC 
                  sa_err    - ptr to sa_err
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None.
******************************************************************************/
uns32 avnd_comp_pmstart_modify(AVND_CB *cb, AVSV_AMF_PM_START_PARAM *pm_start, AVND_COMP_PM_REC *rec,
			       SaAisErrorT *sa_err)
{
	uns32 rc = NCSCC_RC_SUCCESS;

	/* check whether the hdl for start & modify match */
	if (pm_start->hdl != rec->req_hdl) {
		*sa_err = SA_AIS_ERR_BAD_HANDLE;
		rc = NCSCC_RC_FAILURE;
		return rc;
	}

	/* fill the modified parameters */
	rec->desc_tree_depth = pm_start->desc_tree_depth;
	rec->err = pm_start->pm_err;
	rec->rec_rcvr.raw = pm_start->rec_rcvr.raw;

	return rc;
}

/****************************************************************************
  Name          : avnd_comp_new_rsrc_mon
 
  Description   : This routine adds a new pm record for the component

  Arguments     : cb        - ptr to the AvND control block
                  comp      - ptr the the component
                  pm_start  - ptr to the pm start parameters
                  sa_err    - ptr to sa_err
 
  Return Values : ptr to the passive monitor (pm) record.
 
  Notes         : None.
******************************************************************************/
AVND_COMP_PM_REC *avnd_comp_new_rsrc_mon(AVND_CB *cb, AVND_COMP *comp, AVSV_AMF_PM_START_PARAM *pm_start,
					 SaAisErrorT *sa_err)
{

	AVND_COMP_PM_REC *rec = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	*sa_err = SA_AIS_OK;

	if ((0 == (rec = calloc(1, sizeof(AVND_COMP_PM_REC)))))
		return rec;

	/* assign the pm params */
	rec->desc_tree_depth = pm_start->desc_tree_depth;
	rec->err = pm_start->pm_err;
	rec->rec_rcvr.raw = pm_start->rec_rcvr.raw;
	rec->pid = pm_start->pid;
	rec->req_hdl = pm_start->hdl;
	/* store the comp bk ptr */
	rec->comp = comp;

	/* add the rec to comp's PM_REC */
	rc = avnd_comp_pm_rec_add(cb, comp, rec);
	if (NCSCC_RC_SUCCESS != rc) {
		free(rec);
		rec = 0;
	}

	return rec;
}

/****************************************************************************
  Name          : avnd_comp_pm_rec_cmp
 
  Description   : This routine compares the information in the pm start param  
                  the component pm_rec.
 
  Arguments     : pm_start - ptr to the  pm_start_param
                  rec      - ptr to healthcheck record
 
  Return Values : returns zero if both are same else 1 .
 
  Notes         : This routine does not check the hdl with PM Start hdl.
                  If the handle differ, the modify routine will throw error.
******************************************************************************/
NCS_BOOL avnd_comp_pm_rec_cmp(AVSV_AMF_PM_START_PARAM *pm_start, AVND_COMP_PM_REC *rec)
{

	if (pm_start->desc_tree_depth != rec->desc_tree_depth)
		return 1;

	if (pm_start->pm_err != rec->err)
		return 1;

	if (pm_start->rec_rcvr.raw != rec->rec_rcvr.raw)
		return 1;

	return 0;

}

/****************************************************************************
  Name          : avnd_evt_ava_pm_start
 
  Description   : This routine processes the pm start message from 
                  AvA.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_ava_pm_start_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_PM_START_PARAM *pm_start = &api_info->param.pm_start;
	AVND_COMP *comp = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	NCS_BOOL msg_from_avnd = FALSE, int_ext_comp = FALSE;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = TRUE;
	}

	if (FALSE == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (TRUE == int_ext_comp) {
			goto done;
		}
	}

	/* validate the pm start message */
	avnd_comp_pm_param_val(cb, AVSV_AMF_PM_START, (uns8 *)pm_start, &comp, 0, &amf_rc);

	/* try starting the monitor */
	if (SA_AIS_OK == amf_rc)
		rc = avnd_comp_pm_start_process(cb, comp, pm_start, &amf_rc);

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_PM_START, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		m_AVND_AVND_ERR_LOG("avnd_evt_ava_pm_start():Comp,Hdl,pid,desc_tree_depth and pm_err are",
				    &pm_start->comp_name, pm_start->hdl, pm_start->pid,
				    pm_start->desc_tree_depth, pm_start->pm_err);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_evt_ava_pm_stop
 
  Description   : This routine processes the pm stop message from 
                  AvA.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 
  Notes         : None.
******************************************************************************/
uns32 avnd_evt_ava_pm_stop_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVSV_AMF_API_INFO *api_info = &evt->info.ava.msg->info.api_info;
	AVSV_AMF_PM_STOP_PARAM *pm_stop = &api_info->param.pm_stop;
	AVND_COMP *comp = 0;
	AVND_COMP_PM_REC *rec = 0;
	uns32 rc = NCSCC_RC_SUCCESS;
	SaAisErrorT amf_rc = SA_AIS_OK;
	NCS_BOOL msg_from_avnd = FALSE, int_ext_comp = FALSE;

	TRACE_ENTER();

	if (AVND_EVT_AVND_AVND_MSG == evt->type) {
		/* This means that the message has come from proxy AvND to this AvND. */
		msg_from_avnd = TRUE;
	}

	if (FALSE == msg_from_avnd) {
		/* Check for internode or external coomponent first
		   If it is, then forward it to the respective AvND. */
		rc = avnd_int_ext_comp_hdlr(cb, api_info, &evt->mds_ctxt, &amf_rc, &int_ext_comp);
		if (TRUE == int_ext_comp) {
			goto done;
		}
	}

	/* validate the pm stop message */
	avnd_comp_pm_param_val(cb, AVSV_AMF_PM_STOP, (uns8 *)pm_stop, &comp, &rec, &amf_rc);

	/* stop the passive monitoring */
	if (SA_AIS_OK == amf_rc)
		rc = avnd_comp_pm_stop_process(cb, comp, pm_stop, &amf_rc);

	/* send the response back to AvA */
	rc = avnd_amf_resp_send(cb, AVSV_AMF_PM_STOP, amf_rc, 0, &api_info->dest, &evt->mds_ctxt, comp, msg_from_avnd);

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		m_AVND_AVND_ERR_LOG("avnd_evt_ava_pm_stop():Comp,Hdl,pid,stop_qual and pm_err are",
				    &pm_stop->comp_name, pm_stop->hdl, pm_stop->pid,
				    pm_stop->stop_qual, pm_stop->pm_err);
	}

	TRACE_LEAVE();
	return rc;
}

/****************************************************************************
  Name          : avnd_comp_pm_param_val
 
  Description   : This routine validates the component pm start & stop
                  parameters.
 
  Arguments     : cb       - ptr to the AvND control block
                  api_type - pm api
                  param    - ptr to the pm start/stop params
                  o_comp   - double ptr to the comp (o/p)
                  o_pm_rec - double ptr to the comp pm record (o/p)
                  o_amf_rc - double ptr to the amf-rc (o/p)
 
  Return Values : None
 
  Notes         : None
******************************************************************************/
void avnd_comp_pm_param_val(AVND_CB *cb,
			    AVSV_AMF_API_TYPE api_type,
			    uns8 *param, AVND_COMP **o_comp, AVND_COMP_PM_REC **o_pm_rec, SaAisErrorT *o_amf_rc)
{
	*o_amf_rc = SA_AIS_OK;

	switch (api_type) {
	case AVSV_AMF_PM_START:
		{
			AVSV_AMF_PM_START_PARAM *pm_start = (AVSV_AMF_PM_START_PARAM *)param;

			/* get the comp */
			if (0 == (*o_comp = m_AVND_COMPDB_REC_GET(cb->compdb, pm_start->comp_name))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			if (TRUE == (*o_comp)->su->su_is_external) {
				/* This is the case when pm start request has come to controller
				   for external component. We don't support pm start for external
				   component. */
				*o_amf_rc = SA_AIS_ERR_INVALID_PARAM;
				return;
			}

			/* non-existing component should not interact with AMF */
			if (m_AVND_COMP_PRES_STATE_IS_UNINSTANTIATED(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_INSTANTIATIONFAILED(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_TERMINATING(*o_comp) ||
			    m_AVND_COMP_PRES_STATE_IS_TERMINATIONFAILED(*o_comp)) {
				*o_amf_rc = SA_AIS_ERR_TRY_AGAIN;
				return;
			}

			if (kill(pm_start->pid, 0) == -1) {
				assert(errno == ESRCH);
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}
		}
		break;

	case AVSV_AMF_PM_STOP:
		{
			AVSV_AMF_PM_STOP_PARAM *pm_stop = (AVSV_AMF_PM_STOP_PARAM *)param;

			/* get the comp */
			if (0 == (*o_comp = m_AVND_COMPDB_REC_GET(cb->compdb, pm_stop->comp_name))) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			if (TRUE == (*o_comp)->su->su_is_external) {
				/* This is the case when pm stop request has come to controller
				   for external component. We don't support pm stop for external
				   component. */
				*o_amf_rc = SA_AIS_ERR_INVALID_PARAM;
				return;
			}

			/* get the record from component passive monitoring list */
			*o_pm_rec =
			    (AVND_COMP_PM_REC *)ncs_db_link_list_find(&(*o_comp)->pm_list, (uns8 *)&pm_stop->pid);

			if (0 == *o_pm_rec) {
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}

			/* if Handle dosen't match with that of PM Start */
			if ((*o_pm_rec)->req_hdl != pm_stop->hdl) {
				*o_amf_rc = SA_AIS_ERR_BAD_HANDLE;
				return;
			}

			if (kill(pm_stop->pid, 0) == -1) {
				assert(errno == ESRCH);
				*o_amf_rc = SA_AIS_ERR_NOT_EXIST;
				return;
			}
		}
		break;

	default:
		assert(0);
	}			/* switch */

	return;
}

/****************************************************************************
  Name          : avnd_comp_pm_finalize
 
  Description   : This routine stops all pm req made by this comp
                  via this hdl and del the PM_REC and MON_REQ.
                  if the hld is the reg hdl of the given comp, it
                  stops all PM associated with this comp.
 
  Arguments     : cb   - ptr to the AvND control block
                  comp - ptr to the component
                  hdl  - AMF handle 
 
  Return Values : None.
 
  Notes         : None.
******************************************************************************/
void avnd_comp_pm_finalize(AVND_CB *cb, AVND_COMP *comp, SaAmfHandleT hdl)
{
	AVND_COMP_PM_REC *rec = 0;

	/* No passive monitoring for external component. */
	if (TRUE == comp->su->su_is_external)
		return;

	while (0 != (rec = (AVND_COMP_PM_REC *)m_NCS_DBLIST_FIND_FIRST(&comp->pm_list))) {
		/* stop PM if comp's reg handle or hdl used for PM start is finalized */
		if (hdl == rec->req_hdl || hdl == comp->reg_hdl) {
			avnd_comp_pm_rec_del(cb, comp, rec);
		}
	}

	return;
}

/************************** END OF FILE ***************************************/
