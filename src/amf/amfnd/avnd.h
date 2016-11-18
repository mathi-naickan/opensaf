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

  This module is the main include file for Availability Node Director.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AMF_AMFND_AVND_H_
#define AMF_AMFND_AVND_H_

#include "imm/saf/saImmOm.h"

#include "amf/amf.h"
#include "base/ncs_main_papi.h"
#include "base/ncssysf_def.h"
#include "base/ncssysf_tsk.h"
#include "base/ncsgl_defs.h"
#include "base/logtrace.h"

/* Porting Include Files */
#include "avnd_defs.h"

/* AvSv Common Files */
#include "amf/amf_amfparam.h"
#include "amf/amf_d2nmsg.h"
#include "amf/amf_n2avamsg.h"
#include "amf/amf_nd2ndmsg.h"
#include "amf/amf_db_template.h"

/* AvND Files */
#include "avnd_tmr.h"
#include "avnd_mds.h"
#include "avnd_proc.h"
#include "avnd_hc.h"
#include "avnd_err.h"
#include "avnd_comp.h"
#include "avnd_evt.h"
#include "amf/amfnd/avnd_su.h"
#include "avnd_pg.h"
#include "avnd_di.h"
#include "avnd_util.h"
#include "avnd_clm.h"
#include "avnd_proxy.h"
#include "avnd_cb.h"

extern uint32_t avnd_create(void);
extern void avnd_sigterm_handler(void);

#endif  // AMF_AMFND_AVND_H_
