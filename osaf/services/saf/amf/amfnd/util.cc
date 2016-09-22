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

  This file contains AvND utility routines. 
..............................................................................

  FUNCTIONS INCLUDED in this module:

  
******************************************************************************
*/
#include <immutil.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <configmake.h>
#include "avnd.h"

const char *presence_state[] =
{
	"OUT_OF_RANGE",
	"UNINSTANTIATED",
	"INSTANTIATING",
	"INSTANTIATED",
	"TERMINATING",
	"RESTARTING",
	"INSTANTIATION_FAILED",
	"TERMINATION_FAILED",
	"ORPHANED"
};

const char *ha_state[] = {
	"INVALID",
	"ACTIVE",
	"STANDBY",
	"QUIESCED",
	"QUIESCING"
};

/* name of file created when a comp is found in term-failed presence state */
static const char *failed_state_file_name = PKGPIDDIR "/amf_failed_state";

/****************************************************************************
  Name          : avnd_msg_content_free
 
  Description   : This routine frees the content of the AvND message.
 
  Arguments     : cb  - ptr to the AvND control block
                  evt - ptr to the AvND event
 
  Return Values : None
 
  Notes         : AVND_MSG structure is used as a wrapper that is never 
                  allocated. Hence it's not freed.
******************************************************************************/
void avnd_msg_content_free(AVND_CB *cb, AVND_MSG *msg)
{
	if (!msg)
		return;

	switch (msg->type) {
	case AVND_MSG_AVD:
		if (msg->info.avd) {
			avsv_dnd_msg_free(msg->info.avd);
			msg->info.avd = 0;
		}
		break;

	case AVND_MSG_AVND:
		if (msg->info.avnd) {
			avsv_nd2nd_avnd_msg_free(msg->info.avnd);
			msg->info.avnd = 0;
		}
		break;

	case AVND_MSG_AVA:
		if (msg->info.ava) {
			avsv_nda_ava_msg_free(msg->info.ava);
			msg->info.ava = 0;
		}
		break;

	default:
		break;
	}

	return;
}

/****************************************************************************
  Name          : avnd_msg_copy
 
  Description   : This routine copies the AvND message.
 
  Arguments     : cb   - ptr to the AvND control block
                  dmsg - ptr to the dest msg
                  smsg - ptr to the source msg
 
  Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 
  Notes         : None
******************************************************************************/
uint32_t avnd_msg_copy(AVND_CB *cb, AVND_MSG *dmsg, AVND_MSG *smsg)
{
	uint32_t rc = NCSCC_RC_SUCCESS;

	if (!dmsg || !smsg) {
		rc = NCSCC_RC_FAILURE;
		goto done;
	}

	/* copy the common fields */
	memcpy(dmsg, smsg, sizeof(AVND_MSG));

	switch (smsg->type) {
	case AVND_MSG_AVD:
		dmsg->info.avd = static_cast<AVSV_DND_MSG*>(calloc(1, sizeof(AVSV_DND_MSG)));
		rc = avsv_dnd_msg_copy(dmsg->info.avd, smsg->info.avd);
		break;

	case AVND_MSG_AVND:
		dmsg->info.avnd = static_cast<AVSV_ND2ND_AVND_MSG*>(calloc(1, sizeof(AVSV_ND2ND_AVND_MSG)));
		rc = avsv_ndnd_avnd_msg_copy(dmsg->info.avnd, smsg->info.avnd);
		break;

	case AVND_MSG_AVA:
		dmsg->info.ava = static_cast<AVSV_NDA_AVA_MSG*>(calloc(1, sizeof(AVSV_NDA_AVA_MSG)));
		rc = avsv_nda_ava_msg_copy(dmsg->info.ava, smsg->info.ava);
		break;

	default:
		osafassert(0);
	}

 done:
	/* if failure, free the dest msg */
	if (NCSCC_RC_SUCCESS != rc && dmsg)
		avnd_msg_content_free(cb, dmsg);

	return rc;
}

/**
 * Verify that msg ID is in sequence otherwise order node reboot
 * and abort process.
 * @param rcv_msg_id
 */
void avnd_msgid_assert(uint32_t rcv_msg_id)
{
	uint32_t expected_msg_id = avnd_cb->rcv_msg_id + 1;

	if (rcv_msg_id != expected_msg_id) {
		char reason[128];
		snprintf(reason, sizeof(reason), "Message ID mismatch, rec %u, expected %u",
				 rcv_msg_id, expected_msg_id);
		opensaf_reboot(0, nullptr, reason);
		abort();
	}
}

/**
 * Execute CLEANUP CLC-CLI command for component @a comp, failfast local node
 * at failure. The function is asynchronous, it does not wait for the cleanup
 * operation to completely finish. The CLEANUP script is just started/launched.
 * @param comp
 */
void avnd_comp_cleanup_launch(AVND_COMP *comp)
{
	char str[128];
	uint32_t rc;

	rc = avnd_comp_clc_fsm_run(avnd_cb, comp, AVND_COMP_CLC_PRES_FSM_EV_CLEANUP);
	if (rc != NCSCC_RC_SUCCESS) {
		LOG_ER("Failed to launch cleanup of '%s'", comp->name.c_str());
		snprintf(str, sizeof(str), "Stopping OpenSAF failed due to '%s'", comp->name.c_str());
		opensaf_reboot(avnd_cb->node_info.nodeId,
				osaf_extended_name_borrow(&avnd_cb->node_info.executionEnvironment), str);
		LOG_ER("exiting to aid fast reboot");
		exit(1);
	}
}

/**
 * Return true if the the failed state file exist.
 * The existence of this file means that AMF has lost control over some
 * component. A reboot or manual cleanup is needed in order to restart opensaf.
 */
bool avnd_failed_state_file_exist(void)
{
	struct stat statbuf;

	if (stat(failed_state_file_name, &statbuf) == 0) {
		return true;
	} else
		return false;
}

/**
 * Create the failed state file
 */
void avnd_failed_state_file_create(void)
{
	int fd = open(failed_state_file_name, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

	if (fd >= 0)
		(void)close(fd);
	else
		LOG_ER("cannot create failed state file %s: %s",
				failed_state_file_name, strerror(errno));
}

/**
 * Delete the failed state file
 */
void avnd_failed_state_file_delete(void)
{
	if (unlink(failed_state_file_name) == -1)
		LOG_ER("cannot unlink failed state file %s: %s",
				failed_state_file_name, strerror(errno));
}

/**
 * Return name of failed state file
 */
const char *avnd_failed_state_file_location(void)
{
	return failed_state_file_name;
}

/****************************************************************************
  Name          : saImmOmInitialize_cond

  Description   : A wrapper of saImmOmInitialize for headless.

  Arguments     : msg - ptr to the msg

  Return Values : SA_AIS_OK or other SA_AIS_ERR_xxx code

  Notes         : None.
******************************************************************************/
SaAisErrorT saImmOmInitialize_cond(SaImmHandleT *immHandle,
	const SaImmCallbacksT *immCallbacks, SaVersionT *version)
{
	if (avnd_cb->scs_absence_max_duration == 0) {
		return immutil_saImmOmInitialize(immHandle, immCallbacks, version);
	}

	// if headless mode is enabled, don't retry as IMMA already has a 30s
	// initial connection timeout towards IMMND. If we retry, we may
	// cause the watchdog to kill AMFND.
	return saImmOmInitialize(immHandle, immCallbacks, version);
}

/*****************************************************************************
 * Function: avnd_cpy_SU_DN_from_DN
 *
 * Purpose:  This function copies the SU DN from the given DN and places
 *           it in the provided buffer.
 *
 * Input: d_su_dn - where the SU DN should be copied.
 *        s_dn_name - contains the SU DN.
 *
 * Returns: NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
 *
 * NOTES: none.
 *
 * 
 **************************************************************************/
uint32_t avnd_cpy_SU_DN_from_DN(std::string& d_su_dn, const std::string& s_dn_name)
{
	std::size_t pos;

	/* SU DN name is  SU name + NODE name */
	/* First get the SU name */
	pos = s_dn_name.find("safSu");

	/* It might be external SU. */
	if (pos == std::string::npos) {
		pos = s_dn_name.find("safEsu");
	}

	if (pos == std::string::npos)
		return NCSCC_RC_FAILURE;

	d_su_dn = s_dn_name.substr(pos);

	return NCSCC_RC_SUCCESS;
}

SaAisErrorT amf_saImmOmInitialize(SaImmHandleT& immHandle)
{
	SaVersionT immVersion = { 'A', 2, 15 };
	return saImmOmInitialize_cond(&immHandle, nullptr, &immVersion);
}

SaAisErrorT amf_saImmOmAccessorInitialize(SaImmHandleT& immHandle,
      SaImmAccessorHandleT& accessorHandle)
{
	// note: this will handle SA_AIS_ERR_BAD_HANDLE just once
	SaAisErrorT rc = immutil_saImmOmAccessorInitialize(immHandle, &accessorHandle);
	if (rc == SA_AIS_ERR_BAD_HANDLE) {
		saImmOmFinalize(immHandle);
		rc = amf_saImmOmInitialize(immHandle);

		// re-attempt immutil_saImmOmAccessorInitialize once more
		if (rc == SA_AIS_OK) {
			rc = immutil_saImmOmAccessorInitialize(immHandle, &accessorHandle);
		}
	}

	return rc;
}

SaAisErrorT amf_saImmOmSearchInitialize_o2(SaImmHandleT& immHandle,
      const std::string& rootName,
      SaImmScopeT scope,
      SaImmSearchOptionsT searchOptions,
      const SaImmSearchParametersT_2 *
      searchParam,
      const SaImmAttrNameT *
      attributeNames,
      SaImmSearchHandleT& searchHandle)
{
	// note: this will handle SA_AIS_ERR_BAD_HANDLE just once
	SaAisErrorT rc = immutil_saImmOmSearchInitialize_o2(immHandle,
		rootName.c_str(),
		scope,
		searchOptions,
		searchParam,
		attributeNames,
		&searchHandle);

	if (rc == SA_AIS_ERR_BAD_HANDLE) {
		immutil_saImmOmFinalize(immHandle);
		rc = amf_saImmOmInitialize(immHandle);

		// re-attempt immutil_saImmOmSearchInitialize_o2 once more
		if (rc == SA_AIS_OK) {
			rc = immutil_saImmOmSearchInitialize_o2(immHandle,
				rootName.c_str(),
				scope,
				searchOptions,
				searchParam,
				attributeNames,
				&searchHandle);
		}
	}
	return rc;
}

SaAisErrorT amf_saImmOmAccessorGet_o2(SaImmHandleT& immHandle,
	SaImmAccessorHandleT& accessorHandle,
	const std::string& objectName,
	const SaImmAttrNameT * attributeNames,
	SaImmAttrValuesT_2 *** attributes)
{
	// note: this will handle SA_AIS_ERR_BAD_HANDLE just once
	SaAisErrorT rc = immutil_saImmOmAccessorGet_o2(accessorHandle,
		objectName.c_str(), attributeNames, attributes);
	
	if (rc == SA_AIS_ERR_BAD_HANDLE) {
		immutil_saImmOmAccessorFinalize(accessorHandle);
		immutil_saImmOmFinalize(immHandle);
		rc = amf_saImmOmInitialize(immHandle);

		if (rc == SA_AIS_OK) {
			rc = amf_saImmOmAccessorInitialize(immHandle, accessorHandle);
		}

		if (rc == SA_AIS_OK) {
			rc = immutil_saImmOmAccessorGet_o2(accessorHandle,
				objectName.c_str(), attributeNames, attributes);
		}
	}

	return rc;
}

void amfnd_free_csi_attr_list(AVSV_CSI_ATTRS *attrs) {
  if (attrs == nullptr)
    return;
  for (uint16_t i = 0; i < attrs->number; i++) {
    osaf_extended_name_free(&attrs->list[i].name);
    osaf_extended_name_free(&attrs->list[i].value);
    free(attrs->list[i].string_ptr);
    attrs->list[i].string_ptr = nullptr;
  }
  free(attrs->list);
  attrs->list = nullptr;
}

void amfnd_copy_csi_attrs(AVSV_CSI_ATTRS *src_attrs, AVSV_CSI_ATTRS *dest_attrs) {
  for (uint16_t i = 0; i < src_attrs->number; i++) {
    osaf_extended_name_alloc(osaf_extended_name_borrow(&src_attrs->list[i].name),
      &dest_attrs->list[i].name);
    osaf_extended_name_alloc(osaf_extended_name_borrow(&src_attrs->list[i].value),
      &dest_attrs->list[i].value);
     //Let string.ptr points to original one, we never free it. Also
     //encode callback takes care of encoding it.
    if (src_attrs->list[i].string_ptr != nullptr) 
      dest_attrs->list[i].string_ptr = src_attrs->list[i].string_ptr; 
  }
}

