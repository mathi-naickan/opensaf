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

#ifndef FM_FMD_FM_AMF_H_
#define FM_FMD_FM_AMF_H_

#include "osaf/configmake.h"
#include <stdbool.h>

/*
 * Macro used to get the AMF version used
 */
#define m_FM_GET_AMF_VER(amf_ver) amf_ver.releaseCode='B'; amf_ver.majorVersion=0x01; amf_ver.minorVersion=0x00;

#define FM_HA_COMP_NAMED_PIPE PKGLOCALSTATEDIR "/fmd_comp_name"

/*
 * FM AMF control information
 */
typedef struct fm_amf_cb {
	char comp_name[256];
	SaAmfHandleT amf_hdl;	/* AMF handle */
	SaSelectionObjectT amf_fd;	/* AMF selection fd */
	bool is_amf_up;	/* For amf_fd and pipe_fd */
	bool nid_started;	/**< true if started by NID */
} FM_AMF_CB;

FM_AMF_CB *fm_amf_get_cb(void);

void fm_saf_CSI_set_callback(SaInvocationT invocation,
			     const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);

void fm_saf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *checkType);

void fm_saf_CSI_rem_callback(SaInvocationT invocation,
			     const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags);

void fm_saf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

uint32_t fm_amf_open(FM_AMF_CB *fm_amf_cb);
uint32_t fm_amf_close(FM_AMF_CB *fm_amf_cb);
uint32_t fm_amf_pipe_process_msg(FM_AMF_CB *fm_amf_cb);
uint32_t fm_amf_process_msg(FM_AMF_CB *fm_amf_cb);

#endif  // FM_FMD_FM_AMF_H_
