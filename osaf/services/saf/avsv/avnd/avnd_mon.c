/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

  DESCRIPTION:

  This module deals with the creation, accessing and deletion of the passive
  monitoring records and lists on the AVND.

..............................................................................

  FUNCTIONS INCLUDED in this file:

******************************************************************************
*/

#include "avnd.h"
#include "avnd_mon.h"

#include <sched.h>
/*****************************************************************************
 * structure for holding PID monitoring node                                 *
 *****************************************************************************/
typedef struct avnd_mon_req_tag {
	NCS_DB_LINK_LIST_NODE mon_dll_node;	/* key is pid */
	SaUint64T pid;		/* pid that is being monitored (index) */
	AVND_COMP_PM_REC *pm_rec;	/* ptr to comp pm rec */
} AVND_MON_REQ;

/* Passive Monitoring time interval in milli secs */
#define AVND_PM_MONITORING_INTERVAL 1000

NCSCONTEXT gl_avnd_mon_task_hdl = 0;
static uint32_t avnd_send_pid_exit_evt(AVND_CB *cb, AVND_COMP_PM_REC *pm_rec);
static void avnd_mon_pids(AVND_CB *cb);

/****************************************************************************
  Name          : avnd_pid_mon_list_init

  Description   : This routine initializes the pid_mon_list.

  Arguments     : cb - ptr to the AvND control block

  Return Values : nothing

  Notes         : None.
******************************************************************************/
void avnd_pid_mon_list_init(AVND_CB *cb)
{
	NCS_DB_LINK_LIST *pid_mon_list = &cb->pid_mon_list;

	/* initialize the PID mon dll list */
	pid_mon_list->order = NCS_DBLIST_ANY_ORDER;
	pid_mon_list->cmp_cookie = avsv_dblist_uns64_cmp;
	pid_mon_list->free_cookie = avnd_mon_req_free;
}

/****************************************************************************
  Name          : avnd_pid_mon_list_destroy

  Description   : This routine destroys the entire pid_mon_list. It deletes
                  all the records in the list.

  Arguments     : cb - ptr to the AvND control block

  Return Values : nothing

  Notes         : This dosen't destroy the records directly. It parses thru
                  the mon_req's and get the corresponding PM_REC, del the
                  PM_REC and as result the mon_req table entry will get
                  deleted.
******************************************************************************/
void avnd_pid_mon_list_destroy(AVND_CB *cb)
{
	NCS_DB_LINK_LIST *pid_mon_list;
	AVND_MON_REQ *mon_req;

	/* get pid_mon_list */
	pid_mon_list = &cb->pid_mon_list;

	/* traverse & delete all the MON records and their corresponding PM_REC */
	while (0 != (mon_req = (AVND_MON_REQ *) m_NCS_DBLIST_FIND_FIRST(pid_mon_list))) {
		osafassert(mon_req->pm_rec);

		avnd_comp_pm_rec_del(cb, mon_req->pm_rec->comp, mon_req->pm_rec);
	}
}

/****************************************************************************
  Name          : avnd_mon_req_add

  Description   : This routine adds a request (node) to the pid_mon list. If
                  the record is already present, it is modified with the new
                  parameters.

  Arguments     : cb        - ptr to the AvND control block
                  pm_rec    - pointer to component PM_REC

  Return Values : ptr to the newly added/modified record

  Notes         : This will be called from the pm_rec_add function only.
******************************************************************************/
AVND_MON_REQ *avnd_mon_req_add(AVND_CB *cb, AVND_COMP_PM_REC *pm_rec)
{
	NCS_DB_LINK_LIST *pid_mon_list = &cb->pid_mon_list;
	AVND_MON_REQ *mon_req;
	uint32_t rc = NCSCC_RC_SUCCESS;

	m_NCS_LOCK(&cb->mon_lock, NCS_LOCK_WRITE);

	/* get the record, if any */
	mon_req = (AVND_MON_REQ *) ncs_db_link_list_find(pid_mon_list, (uint8_t *)&pm_rec->pid);
	if (!mon_req) {
		/* a new record.. alloc & link it to the dll */
		mon_req = (AVND_MON_REQ *) malloc(sizeof(AVND_MON_REQ));
		if (mon_req) {
			memset(mon_req, 0, sizeof(AVND_MON_REQ));

			mon_req->pid = pm_rec->pid;

			/* update the record key */
			mon_req->mon_dll_node.key = (uint8_t *)&mon_req->pid;

			rc = ncs_db_link_list_add(pid_mon_list, &mon_req->mon_dll_node);
			if (NCSCC_RC_SUCCESS != rc) {
				m_NCS_UNLOCK(&cb->mon_lock, NCS_LOCK_WRITE);
				goto done;
			}
		} else {
			m_NCS_UNLOCK(&cb->mon_lock, NCS_LOCK_WRITE);
			LOG_ER("Memory Alloc Failed for MON_REQ structure");
			rc = NCSCC_RC_FAILURE;
			goto done;
		}
	}

	/* update the params */
	mon_req->pm_rec = pm_rec;

	m_NCS_UNLOCK(&cb->mon_lock, NCS_LOCK_WRITE);

	TRACE_1("PID: %lld added for (passive) Monitoring", mon_req->pid);

	/* PID monitoring task not yet created, so create */
	if (gl_avnd_mon_task_hdl == 0) {
		rc = avnd_mon_task_create();
	}

 done:
	if (NCSCC_RC_SUCCESS != rc) {
		if (mon_req) {
			avnd_mon_req_free(&mon_req->mon_dll_node);
			mon_req = 0;
		}
	}

	return mon_req;
}

/****************************************************************************
  Name          : avnd_mon_req_del

  Description   : This routine deletes (unlinks & frees) the specified record
                  (node) from the pid_mon list.

  Arguments     : cb  - ptr to the AvND control block
                  pid - PID of the req node that is to be deleted

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uint32_t avnd_mon_req_del(AVND_CB *cb, SaUint64T pid)
{
	NCS_DB_LINK_LIST *pid_mon_list = &cb->pid_mon_list;
	uint32_t rc;
	AVND_MON_REQ *mon_rec;

	m_NCS_LOCK(&cb->mon_lock, NCS_LOCK_WRITE);

	rc = ncs_db_link_list_del(pid_mon_list, (uint8_t *)&pid);

	TRACE_1("PID: %lld deleted from (passive) Monitoring", pid);

	mon_rec = (AVND_MON_REQ *) m_NCS_DBLIST_FIND_FIRST(pid_mon_list);

	/* No more PIDs exists in the pid_mon_list for monitoring */
	if (!mon_rec) {
		/* destroy the task */
		if (gl_avnd_mon_task_hdl) {
			/* release the task */
			m_NCS_TASK_RELEASE(gl_avnd_mon_task_hdl);
			gl_avnd_mon_task_hdl = 0;

			TRACE_1("Passive Monitoring thread was released");
		}
	}

	m_NCS_UNLOCK(&cb->mon_lock, NCS_LOCK_WRITE);

	return rc;
}

/****************************************************************************
  Name          : avnd_mon_req_free

  Description   : This routine free the memory alloced to the specified
                  record in the pid_mon list.

  Arguments     : node - ptr to the dll node

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None.
******************************************************************************/
uint32_t avnd_mon_req_free(NCS_DB_LINK_LIST_NODE *node)
{
	AVND_MON_REQ *mon_req = (AVND_MON_REQ *) node;

	if (mon_req)
		free(mon_req);

	return NCSCC_RC_SUCCESS;
}

/****************************************************************************
  Name          : avnd_mon_task_create

  Description   : This routine creates & starts AvND Passive monitoring task.

  Arguments     : None.

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_mon_task_create(void)
{
	uint32_t rc;

	/* create avnd task */
	int policy = SCHED_OTHER; /*root defaults */
	int prio_val = sched_get_priority_min(policy);
	
	rc = m_NCS_TASK_CREATE((NCS_OS_CB)avnd_mon_process, NULL,
			       (char *)"OSAF_AVND_MON", prio_val, policy, m_AVND_STACKSIZE, &gl_avnd_mon_task_hdl);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("Passive Monitoring thread CREATE failed");
		goto err;
	}
	TRACE_1("Created Passive Monitoring thread");

	/* now start the task */
	rc = m_NCS_TASK_START(gl_avnd_mon_task_hdl);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("Passive Monitoring thread START failed");
		goto err;
	}
	TRACE_1("Started Passive Monitoring thread");

	return rc;

 err:
	/* destroy the task */
	if (gl_avnd_mon_task_hdl) {
		/* release the task */
		m_NCS_TASK_RELEASE(gl_avnd_mon_task_hdl);

		gl_avnd_mon_task_hdl = 0;
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_send_pid_exit_evt

  Description   : This routine creates a PID exit event and send to AVND 
		  thread for further processing.

  Arguments     : cb - ptr to AVND control block
		  pm_rec - ptr to AVND_COMP_PM_REC struct

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_send_pid_exit_evt(AVND_CB *cb, AVND_COMP_PM_REC *pm_rec)
{
	AVND_EVT *evt;
	uint32_t rc = NCSCC_RC_FAILURE;

	/* create & send the timer event */
	evt = avnd_evt_create(cb, AVND_EVT_PID_EXIT, 0, 0, (void *)pm_rec, 0, 0);
	if (evt) {
		rc = avnd_evt_send(cb, evt);
	}

	if (rc == NCSCC_RC_SUCCESS) {
		TRACE_1("Sent PM (PID: %lld) Exit event", pm_rec->pid);
	} else {
		LOG_ER("Failed to send PM (PID: %lld) exit event", pm_rec->pid);
	}

	return rc;
}

/****************************************************************************
  Name          : avnd_mon_pids

  Description   : This routine traverses through the list of PIDs to be 
		  monitored & checks their existence in the system/node. Sends
		  an event to AVND thread if PID doesn't exists.

  Arguments     : cb - ptr to AVND control block

  Return Values : 

  Notes         : None
******************************************************************************/
void avnd_mon_pids(AVND_CB *cb)
{
	AVND_MON_REQ *mon_rec;
	NCS_DB_LINK_LIST *pid_mon_list;

	/* get pid_mon_list */
	pid_mon_list = &cb->pid_mon_list;

	m_NCS_LOCK(&cb->mon_lock, NCS_LOCK_WRITE);

	for (mon_rec = (AVND_MON_REQ *) m_NCS_DBLIST_FIND_FIRST(pid_mon_list);
	     mon_rec; mon_rec = (AVND_MON_REQ *) m_NCS_DBLIST_FIND_NEXT(&mon_rec->mon_dll_node)) {

		if (mon_rec->pm_rec == NULL) {
			/* Should not happen, LOG the err message */
			LOG_ER("Weird issue, PID (%lld) monitor rec doesn't have context of PM record", mon_rec->pid);
			continue;
		}

		switch (kill(mon_rec->pid, 0)) {
		case 0:
			break;
		
		case EPERM:
			LOG_ER("PM not able send signal to PID: %lld", mon_rec->pid);
			break;

		case ESRCH:	/* process died */
		default:
			avnd_send_pid_exit_evt(cb, mon_rec->pm_rec);
			break;
		}
	}

	m_NCS_UNLOCK(&cb->mon_lock, NCS_LOCK_WRITE);
}

/****************************************************************************
  Name          : avnd_mon_process

  Description   : This routine is an entry point for the AvND PM task. 

  Arguments     : arg - ptr to the cb handle

  Return Values : None

  Notes         : None
******************************************************************************/
void avnd_mon_process(void *arg)
{
	unsigned int mon_rate;
	char *tmp_ptr;

	tmp_ptr = getenv("AVND_PM_MONITORING_RATE");
	if (tmp_ptr) 
		mon_rate = atoi(tmp_ptr);
	else
		mon_rate = AVND_PM_MONITORING_INTERVAL;

	while (1) {
		avnd_mon_pids(avnd_cb);
		m_NCS_TASK_SLEEP(mon_rate);
	}
}


/****************************************************************************
  Name          : avnd_evt_pid_exit_evt

  Description   : This routine is called by AvND thread upon receiving PID
		  exit event from AvND PM thread. 

  Arguments     : cb - ptr to the AvND control block
		  evt - ptr to AVND Evt struct

  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE

  Notes         : None
******************************************************************************/
uint32_t avnd_evt_pid_exit_evh(AVND_CB *cb, AVND_EVT *evt)
{
	AVND_COMP_PM_REC *pm_rec;
	AVND_COMP *comp;
	AVND_ERR_INFO err;

	TRACE_ENTER();
	comp = m_AVND_COMPDB_REC_GET(cb->compdb, evt->info.pm_evt.comp_name);
	if (NULL == comp) {
		LOG_NO("Comp '%s' not found ", evt->info.pm_evt.comp_name.value);
		goto done;
	}
	pm_rec = (AVND_COMP_PM_REC *)ncs_db_link_list_find(&comp->pm_list, 
			(uint8_t *)&evt->info.pm_evt.pid);	
	if (NULL == pm_rec) {
		LOG_NO("PID '%llu' not found for Comp '%s'", evt->info.pm_evt.pid, evt->info.pm_evt.comp_name.value);
		goto done;
	}

	pm_rec = evt->info.pm_evt.pm_rec;
	if (!pm_rec)
		return NCSCC_RC_FAILURE;

	/* store the info */
	err.src = AVND_ERR_SRC_PM;
	err.rec_rcvr.raw = pm_rec->rec_rcvr.raw;
	comp = pm_rec->comp;

	/* free up the rec */
	avnd_comp_pm_rec_del(cb, comp, pm_rec);

	/*** process the error ***/
	avnd_err_process(cb, comp, &err);

done:
	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;
}
