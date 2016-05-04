/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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
 * Author(s): Oracle
 *
 */

#include <thread>
#include "avnd.h"
#include <poll.h>
#include <imm.hh>

ir_cb_t ir_cb;
struct pollfd ImmReader::fds[];
extern const AVND_EVT_HDLR g_avnd_func_list[AVND_EVT_MAX];

/**
 * This thread will read classes and object information thereby allowing main
 * thread to proceed with other important work.
 * As of now, SU instantiation needs imm data to be read, so
 * only this event is being handled here.
 * Going forward, we need to use this function for similar purposes.
 * @param _cb
 *
 * @return void*
 */
void ImmReader::imm_reader_thread() {
	uint32_t rc = SA_AIS_OK;
	AVND_EVT *evt;
	TRACE_ENTER();
	NCS_SEL_OBJ mbx_fd;

	/* create the mail box */
	rc = m_NCS_IPC_CREATE(&ir_cb.mbx);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("AvND Mailbox creation failed");
		exit(EXIT_FAILURE);
	}
	TRACE("Ir mailbox creation success");

	/* attach the mail box */
	rc = m_NCS_IPC_ATTACH(&ir_cb.mbx);
	if (NCSCC_RC_SUCCESS != rc) {
		LOG_CR("Ir mailbox attach failed");
		exit(EXIT_FAILURE);
	}

	mbx_fd = ncs_ipc_get_sel_obj(&ir_cb.mbx);
	fds[FD_MBX].fd = mbx_fd.rmv_obj;
	fds[FD_MBX].events = POLLIN;

	while (1) {
		if (poll(fds, irfds, -1) == (-1)) {
			if (errno == EINTR) {
				continue;
			}

			LOG_ER("poll Fail - %s", strerror(errno));
			exit(EXIT_FAILURE);
		}

		if (fds[FD_MBX].revents & POLLIN) {
			while (nullptr != (evt = (AVND_EVT *)ncs_ipc_non_blk_recv(&ir_cb.mbx)))
				ir_process_event(evt);
		}
	}
	TRACE_LEAVE();
}

/**
 * Start a imm reader thread.
 * @param Nothing.
 * @return Nothing. 
 */
void ImmReader::imm_reader_thread_create() {
	TRACE_ENTER();

	std::thread t {imm_reader_thread};
	t.detach();

	TRACE_LEAVE();
}

/**
 * This function process events, which requires reading data from imm.
 * As of now, SU instantiation needs to read data, so diverting SU
 * instantiation in this thread. Going forward, we need to use this
 * function for similar purposes.
 * @param Event pointer.
 * @return Nothing.
 */
void ImmReader::ir_process_event(AVND_EVT *evt) {
	uint32_t res = NCSCC_RC_SUCCESS;
	AVND_EVT_TYPE evt_type =  evt->type;
	TRACE_ENTER2("Evt type:%u", evt_type);

	osafassert(AVND_EVT_AVD_SU_PRES_MSG == evt_type);

	res = g_avnd_func_list[evt_type] (avnd_cb, evt);

	/* log the result of event processing */
	TRACE("Evt Type: '%u', '%s'", evt_type, ((res == NCSCC_RC_SUCCESS) ? "success" : "failure"));

	if (evt)
		avnd_evt_destroy(evt);

	TRACE_LEAVE2("res '%u'", res);
}
