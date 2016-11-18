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
 * Author(s): Ericsson
 *
 */

#ifndef AMF_AMFD_ROLE_H_
#define AMF_AMFD_ROLE_H_

#include "amf/amfd/cb.h"

typedef enum avd_rol_chg_cause_type {
        AVD_SWITCH_OVER,
        AVD_FAIL_OVER
} AVD_ROLE_CHG_CAUSE_T;

extern void amfd_switch(AVD_CL_CB *cb);
extern uint32_t avd_post_amfd_switch_role_change_evt(AVD_CL_CB *cb, SaAmfHAStateT role);
extern uint32_t avd_d2d_chg_role_rsp(AVD_CL_CB *cb, uint32_t status, SaAmfHAStateT role);
extern uint32_t avd_d2d_chg_role_req(AVD_CL_CB *cb, AVD_ROLE_CHG_CAUSE_T cause, SaAmfHAStateT role);

extern uint32_t amfd_switch_qsd_stdby(AVD_CL_CB *cb);
extern uint32_t amfd_switch_stdby_actv(AVD_CL_CB *cb);
extern uint32_t amfd_switch_qsd_actv(AVD_CL_CB *cb);
extern uint32_t amfd_switch_actv_qsd(AVD_CL_CB *cb);
extern uint32_t initialize_for_assignment(cl_cb_tag* cb,
                                          SaAmfHAStateT ha_state);

#endif  // AMF_AMFD_ROLE_H_

