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

#ifndef AMF_AMFND_IMM_H_
#define AMF_AMFND_IMM_H_
#include<poll.h>

typedef struct {
	SYSF_MBX mbx;
}ir_cb_t;
extern ir_cb_t ir_cb;

class ImmReader {
	public:
		static void imm_reader_thread();
		static void imm_reader_thread_create();
		static void ir_process_event(AVND_EVT *evt);
	private:

		enum {
			FD_MBX = 0
		};

		static struct pollfd fds[FD_MBX + 1];
		static const nfds_t irfds {FD_MBX + 1};

		// disallow copy and assign
		ImmReader(const ImmReader&);
		void operator=(const ImmReader&);
};

extern uint32_t avnd_evt_ir_evh(struct avnd_cb_tag *cb, struct avnd_evt_tag *evt);

#endif  // AMF_AMFND_IMM_H_
