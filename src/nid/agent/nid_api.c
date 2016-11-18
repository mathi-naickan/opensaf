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
*                                                                            *
*  MODULE NAME:  nid_api.c                                                   *
*                                                                            *
*                                                                            *
*  DESCRIPTION                                                               *
*  This module defines the API used by bladeinitd spawned services. From ser *
*  vices point of view, this API provides interface to communicate the       *
*  initialization status to bladeinitd. Bladeinitd spawned services are      *
*  HPM,RDF,XND,LHC,DRBD,NETWORKING,TIPC,PCAP,SCAP.                      *
*                                                                            *
*****************************************************************************/

#include "nid/agent/nid_api.h"
#include <stdbool.h>

static bool is_nid_notified = false;

/****************************************************************************
 * Name          : nid_notify                                               *
 *                                                                          *
 * Description   : Opens the FIFO to bladeinitd and write the service and   *
 *                 status code to FIFO.                                     *
 *                                                                          *
 * Arguments     : service  - input parameter providing service code        *
 *                 status   - input parameter providing status code         *
 *                 error    - output parameter to return error code if any  *
 * Return Values : NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE..                      *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
uint32_t nid_notify(const char* service, uint32_t status, uint32_t* error)
{
	uint32_t scode;
	char msg[250];
	int32_t fd = -1;
	uint32_t retry = 3;
	char strbuff[256];

	if (is_nid_notified) return NCSCC_RC_SUCCESS;

	scode = status;

	if ((scode < 0)) {
		if (error != NULL)
			*error = NID_INV_PARAM;
		return NCSCC_RC_FAILURE;
	}

	while (retry) {
		if (nid_open_ipc(&fd, strbuff) != NCSCC_RC_SUCCESS) {
			retry--;
		} else
			break;
	}

	if ((fd < 0) && (retry == 0)) {
		if (error != NULL)
			*error = NID_OFIFO_ERR;
		return NCSCC_RC_FAILURE;
	}

	/* Prepare the message to be sent */
	sprintf(msg, "%x:%s:%d", NID_MAGIC, service, scode);

	/* Send the message */
	retry = 3;
	while (retry) {
		if (write(fd, msg, strlen(msg)) == strlen(msg))
			break;
		else
			retry--;
	}

	if (retry == 0) {
		if (error != NULL)
			*error = NID_WFIFO_ERR;
		return NCSCC_RC_FAILURE;
	}

	is_nid_notified = true;
	nid_close_ipc();
	return NCSCC_RC_SUCCESS;
}
