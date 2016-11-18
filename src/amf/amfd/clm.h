/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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


#include "clm/saf/saClm.h"

#ifndef AMF_AMFD_CLM_H_
#define AMF_AMFD_CLM_H_

struct cl_cb_tag;


extern SaAisErrorT avd_clm_init(struct cl_cb_tag*);
extern SaAisErrorT avd_clm_track_start(void);
extern SaAisErrorT avd_clm_track_stop(void);
extern void clm_node_terminate(AVD_AVND *node);
extern SaAisErrorT avd_start_clm_init_bg(void);

#endif  // AMF_AMFD_CLM_H_


