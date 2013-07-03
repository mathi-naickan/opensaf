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

#include "eds.h"

static SaVersionT imm_version = { EDSV_IMM_RELEASE_CODE,
	EDSV_IMM_MAJOR_VERSION,
	EDSV_IMM_MINOR_VERSION
};

extern struct ImmutilWrapperProfile immutilWrapperProfile;

static const SaImmOiImplementerNameT implementer_name = EDSV_IMM_IMPLEMENTER_NAME;

/**
 * Callback from IMM to fetch values of run time non-cached attributes from us.
 * 
 * @return SaAisErrorT
 */

SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT immOiHandle,
					const SaNameT *objectName, const SaImmAttrNameT *attributeNames)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaImmAttrNameT attributeName;
	unsigned int i = 0, attr_count = 0;
	SaUint32T num_users, num_subscribers, num_publishers, num_ret_evts, num_lost_evts;
	EDS_CB *cb = NULL;
	EDS_WORKLIST *wp = NULL;

	SaImmAttrModificationT_2 attr_output[5];
	const SaImmAttrModificationT_2 *attrMods[6];
	TRACE_ENTER();

	SaImmAttrValueT attrUpdateValues1[] = { &num_users };
	SaImmAttrValueT attrUpdateValues2[] = { &num_subscribers };
	SaImmAttrValueT attrUpdateValues3[] = { &num_publishers };
	SaImmAttrValueT attrUpdateValues4[] = { &num_ret_evts };
	SaImmAttrValueT attrUpdateValues5[] = { &num_lost_evts };

	/* Get EDS CB Handle. */
	if (NULL == (cb = (NCSCONTEXT)ncshm_take_hdl(NCS_SERVICE_ID_EDS, gl_eds_hdl))) {
		LOG_ER("Global take handle failed");
		TRACE_LEAVE();
		return (rc = SA_AIS_ERR_FAILED_OPERATION);	/* Log it */
	}

	/* Look up the channel worklist DB to find the channel indicated by objectName */
	wp = get_channel_from_worklist(cb, *objectName);
	if (wp == NULL) {
		rc = SA_AIS_ERR_FAILED_OPERATION;
		ncshm_give_hdl(gl_eds_hdl);
		LOG_WA("channel record not found for : %s", objectName->value);
		TRACE_LEAVE();
		return rc;
	}

	/* Walk through the attribute Name list */
	while ((attributeName = attributeNames[i]) != NULL) {

		if (strcmp(attributeName, "saEvtChannelNumOpeners") == 0) {
			TRACE("Attribute : saEvtChannelNumOpeners");
			num_users = wp->chan_row.num_users;
			attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
			attr_output[attr_count].modAttr.attrName = attributeName;
			attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
			attr_output[attr_count].modAttr.attrValuesNumber = 1;
			attr_output[attr_count].modAttr.attrValues = attrUpdateValues1;
			attrMods[attr_count] = &attr_output[attr_count];
			++attr_count;
		} else if (strcmp(attributeName, "saEvtChannelNumSubscribers") == 0) {
			TRACE("Attribute : saEvtChannelNumSubscribers");
			num_subscribers = wp->chan_row.num_subscribers;
			attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
			attr_output[attr_count].modAttr.attrName = attributeName;
			attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
			attr_output[attr_count].modAttr.attrValuesNumber = 1;
			attr_output[attr_count].modAttr.attrValues = attrUpdateValues2;
			attrMods[attr_count] = &attr_output[attr_count];
			++attr_count;
		} else if (strcmp(attributeName, "saEvtChannelNumPublishers") == 0) {
			TRACE("Attribute : saEvtChannelNumPublishers");
			num_publishers = wp->chan_row.num_publishers;
			attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
			attr_output[attr_count].modAttr.attrName = attributeName;
			attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
			attr_output[attr_count].modAttr.attrValuesNumber = 1;
			attr_output[attr_count].modAttr.attrValues = attrUpdateValues3;
			attrMods[attr_count] = &attr_output[attr_count];
			++attr_count;
		} else if (strcmp(attributeName, "saEvtChannelNumRetainedEvents") == 0) {
			TRACE("Attribute : saEvtChannelNumRetainedEvents");
			num_ret_evts = wp->chan_row.num_ret_evts;
			attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
			attr_output[attr_count].modAttr.attrName = attributeName;
			attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
			attr_output[attr_count].modAttr.attrValuesNumber = 1;
			attr_output[attr_count].modAttr.attrValues = attrUpdateValues4;
			attrMods[attr_count] = &attr_output[attr_count];
			++attr_count;
		} else if (strcmp(attributeName, "saEvtChannelLostEventsEventCount") == 0) {
			TRACE("Attribute : saEvtChannelLostEventsEventCount");
			num_lost_evts = wp->chan_row.num_lost_evts;
			attr_output[attr_count].modType = SA_IMM_ATTR_VALUES_REPLACE;
			attr_output[attr_count].modAttr.attrName = attributeName;
			attr_output[attr_count].modAttr.attrValueType = SA_IMM_ATTR_SAUINT32T;
			attr_output[attr_count].modAttr.attrValuesNumber = 1;
			attr_output[attr_count].modAttr.attrValues = attrUpdateValues5;
			attrMods[attr_count] = &attr_output[attr_count];
			++attr_count;
		} else {
			rc = SA_AIS_ERR_FAILED_OPERATION;
			ncshm_give_hdl(gl_eds_hdl);
			LOG_ER("Invalid Attribute name in saImmOiRtAttrUpdateCallback");
			TRACE_LEAVE();
			return rc;
		}
		++i;
	}			/*End while attributesNames() */
	attrMods[attr_count] = NULL;
	rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
	if (rc != SA_AIS_OK)
		LOG_ER("saImmOiRtObjectUpdate_2 for '%s' returned : %u", objectName->value, rc);

	ncshm_give_hdl(gl_eds_hdl);

	TRACE_LEAVE();
	return rc;
}

SaImmOiCallbacksT_2 oi_cbks = {
	.saImmOiAdminOperationCallback = NULL,
	.saImmOiCcbAbortCallback = NULL,
	.saImmOiCcbApplyCallback = NULL,
	.saImmOiCcbCompletedCallback = NULL,
	.saImmOiCcbObjectCreateCallback = NULL,
	.saImmOiCcbObjectDeleteCallback = NULL,
	.saImmOiCcbObjectModifyCallback = NULL,
	.saImmOiRtAttrUpdateCallback = saImmOiRtAttrUpdateCallback
};

/**
 * Initialize with IMM and get a selection object.
 * @param cb
 * @return SaAisErrorT
 */

SaAisErrorT eds_imm_init(EDS_CB *cb)
{
	SaAisErrorT rc;
	immutilWrapperProfile.errorsAreFatal = 0;
	TRACE_ENTER();

	rc = immutil_saImmOiInitialize_2(&cb->immOiHandle, &oi_cbks, &imm_version);
	if (rc == SA_AIS_OK) {
		rc = immutil_saImmOiSelectionObjectGet(cb->immOiHandle, &cb->imm_sel_obj);
		if (rc != SA_AIS_OK)
			LOG_ER("saImmOiSelectionObjectGet failed with error: - %u", rc);	
	} else {
		LOG_ER("saImmOiInitialize_2 failed with error: %d", rc);
	}	

	TRACE_LEAVE();
	return rc;
}

/**
 * Declare yourself (ACTIVE) as the implementer for the event service runtime objects.
 * @return void *
 */
void * _eds_imm_declare_implementer(void *_cb)
{
	SaAisErrorT rc;
	EDS_CB *cb = (EDS_CB *) _cb;
	rc = immutil_saImmOiImplementerSet(cb->immOiHandle, implementer_name); 
	if(rc != SA_AIS_OK) {
		LOG_ER("saImmOiImplementerSet failed with error: %u", rc);
		exit(EXIT_FAILURE);
	}

	cb->is_impl_set = true;

	return NULL;
}

/**************************************************************************\
 Function: eds_imm_declare_implementer

 Purpose:  creates a new thread for _eds_imm_declare_implementer
        
 Returns:  void

 Notes:    If it fails it will exit the process
\**************************************************************************/
void eds_imm_declare_implementer(EDS_CB *cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	if (pthread_create(&thread, &attr, _eds_imm_declare_implementer, cb) != 0) {
		LOG_ER("pthread create failed: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
}

/**
 * Walkthrough the worklist and find the specified channel record. 
 * @param cb, channel name
 * 
 * @return EDS_WORKLIST * 
 */

EDS_WORKLIST *get_channel_from_worklist(EDS_CB *cb, SaNameT chan_name)
{
	EDS_WORKLIST *wp = NULL;
	TRACE_ENTER();

	wp = cb->eds_work_list;
	if (wp == NULL) {
		TRACE_LEAVE();
		return wp;	/*No channels yet */
	}

	/* Loop through the list for a matching channel name */

	while (wp) {
		/* Compare channel length */
		if (wp->cname_len == chan_name.length) {
		/* Compare channel name */
			if (memcmp(wp->cname, chan_name.value, chan_name.length) == 0)
			{
				TRACE_LEAVE2("match found");
				return wp;	/* match found */
			}
		}
		wp = wp->next;
	}			/*End while */

	/* if we reached here, no channel by that name */
	TRACE("OiRtAttrUpdate: Channel %s not found",chan_name.value);
	TRACE_LEAVE();
	return ((EDS_WORKLIST *)NULL);
}	/*End get_channel_from_worklist */


/**
 * Re-Initialize the OI interface and get a selection object.
 * This is performed wihen IMM returns SA_AIS_ERR_BAD_HANDLE.
 * This is needed currently because IMM is incapable of handling IMMND restarts.
 * i.e. IMMND restarts affect the application.
 * @param cb
 * 
 * @return SaAisErrorT
 */
static void  *eds_imm_reinit_thread(void * _cb)
{
	SaAisErrorT error = SA_AIS_OK;
	EDS_CB *cb = (EDS_CB *)_cb;
	TRACE_ENTER();
	/* Reinitiate IMM */
	error = eds_imm_init(cb);
	if (error == SA_AIS_OK) {
		/* If this is the active server, become implementer again. */
		if (cb->ha_state == SA_AMF_HA_ACTIVE)
			_eds_imm_declare_implementer(&cb->immOiHandle);
	} else
		LOG_ER("eds_imm_init FAILED: %s", strerror(error));

	TRACE_LEAVE();
	return NULL;
}


/**
 * Become object and class implementer, non-blocking.
 * @param cb
 */
void eds_imm_reinit_bg(EDS_CB * cb)
{
	pthread_t thread;
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);

	TRACE_ENTER();

	if (pthread_create(&thread, &attr, eds_imm_reinit_thread, cb) != 0) {
		LOG_ER("pthread_create FAILED: %s", strerror(errno));
		exit(EXIT_FAILURE);
	}
	pthread_attr_destroy(&attr);
	TRACE_LEAVE();
}

