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

#ifndef FM_H
#define FM_H

#include <stdio.h>
#include <poll.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <termios.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>

#include "ncs_main_papi.h"
#include "ncsgl_defs.h"
#include "ncs_osprm.h"

#include "ncs_svd.h"
#include "ncs_hdl_pub.h"
#include "ncssysf_lck.h"
#include "ncsusrbuf.h"
#include "ncssysf_def.h"
#include "ncssysfpool.h"
#include "ncssysf_tmr.h"
#include "ncssysf_mem.h"
#include "ncssysf_ipc.h"
#include "ncssysf_tsk.h"
#include "ncspatricia.h"

#include "ncs_queue.h"

#include "ncs_lib.h"
#include "ncs_edu_pub.h"
#include "nid_api.h"
#include "mds_papi.h"
#include "logtrace.h"
#include "nid_start_util.h"
/* SAF Include file. */
#include "saAis.h"
#include "saAmf.h"


#include "rda_papi.h"

/* The below files are very specific to fm. */
#include "fm_amf.h"
#include "fm_cb.h"
#include "fm_mem.h"
#include "fm_mds.h"
#include "fm_evt.h"

extern void amfnd_down_callback(void);
extern void ava_install_amf_down_cb(void (*cb) (void));
#endif
