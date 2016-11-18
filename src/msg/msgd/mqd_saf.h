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

  FILE NAME: mqd_saf.h

..............................................................................

  DESCRIPTION:
  
  API decleration for MQD SAF library..
    
******************************************************************************/
#ifndef MSG_MSGD_MQD_SAF_H_
#define MSG_MSGD_MQD_SAF_H_

void mqd_saf_hlth_chk_cb(SaInvocationT, const SaNameT *, SaAmfHealthcheckKeyT *);
void mqd_saf_csi_set_cb(SaInvocationT invocation,
				 const SaNameT *compName, SaAmfHAStateT haState, SaAmfCSIDescriptorT csiDescriptor);

void mqd_amf_comp_terminate_callback(SaInvocationT invocation, const SaNameT *compName);

void mqd_amf_csi_rmv_callback(SaInvocationT invocation,
				       const SaNameT *compName, const SaNameT *csiName, SaAmfCSIFlagsT csiFlags);

#endif  // MSG_MSGD_MQD_SAF_H_
