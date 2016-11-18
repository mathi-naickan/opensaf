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

#ifndef LCK_LCKD_GLD_AMF_H_
#define LCK_LCKD_GLD_AMF_H_

void
gld_amf_CSI_set_callback(SaInvocationT invocation,
			 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);
void
gld_amf_health_chk_callback(SaInvocationT invocation, const SaNameT *compName, SaAmfHealthcheckKeyT *healthcheckKey);

void gld_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

void
gld_amf_csi_rmv_callback(SaInvocationT invocation,
			 const SaNameT *compName, const SaNameT *csiName, const SaAmfCSIFlagsT csiFlags);
uint32_t gld_amf_init(GLSV_GLD_CB *gld_cb);

#endif  // LCK_LCKD_GLD_AMF_H_
