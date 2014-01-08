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
 * Author(s): Nokia Siemens Networks, OptXware Research & Development LLC.
 */

#ifndef J_UTILS_PRINT_H_
#define J_UTILS_PRINT_H_

/**************************************************************************
 * Include files
 *************************************************************************/

#include <saAis.h>
#include <saClm.h>
#include <saCkpt.h>
#include <saAmf.h>

/**************************************************************************
 * Constants
 *************************************************************************/

/**************************************************************************
 * Macros
 *************************************************************************/

/**************************************************************************
 * Data types and structures
 *************************************************************************/

/**************************************************************************
 * Variable declarations
 *************************************************************************/

/**************************************************************************
 * Function declarations
 *************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

// AIS
	void U_printSaName(const char *msg, const SaNameT *saNamePtr);
	void U_printSaSelectionObjectInfo(const char *msg,
					  const SaSelectionObjectT
					  saSelectionObject);
// CLM
	void U_printSaClusterNotificationBuffer(const char *msg,
						const
						SaClmClusterNotificationBufferT
						*saClusterNotificationBufferPtr);
	void U_printSaClusterNotificationBuffer_4(const char *msg,
						  const
						  SaClmClusterNotificationBufferT_4
						  *saClusterNotificationBufferPtr);
	void U_printSaClusterNotification(const char *msg,
					  const SaClmClusterNotificationT
					  *saClusterNotificationPtr);
	void U_printSaClusterNotification_4(const char *msg,
					    const SaClmClusterNotificationT_4
					    *saClusterNotificationPtr);
	void U_printSaClusterNode(const char *msg,
				  const SaClmClusterNodeT *saClusterNodePtr);
	void U_printSaClusterNode_4(const char *msg,
				    const SaClmClusterNodeT_4
				    *saClusterNodePtr);
	void U_printSaNodeAddress(const char *msg,
				  const SaClmNodeAddressT *saClmNodeAddressPtr);
// CKPT
	void U_printSaCheckpointDescriptor(const char *msg,
					   const SaCkptCheckpointDescriptorT
					   *saCheckpointDescriptorPtr);
	void U_printSaCheckpointCreationAttributes(const char *msg,
						   const
						   SaCkptCheckpointCreationAttributesT
						   *saCheckpointCreationAttributesPtr);
// AMF
	void U_printSaAmfHAState(const char *msg, const SaAmfHAStateT haState);
	void U_printSaAmfCSIDescriptor(const char *msg,
				       const SaAmfHAStateT haState,
				       const SaAmfCSIDescriptorT
				       *csiDescriptor);
	void U_printSaAmfCSIFlags(const char *msg,
				  const SaAmfCSIFlagsT csiFlags);
	void U_printSaAmfCSIActiveDescriptor(const char *msg,
					     const SaAmfCSIActiveDescriptorT
					     *activeDescriptor);
	void U_printSaAmfCSIStandbyDescriptor(const char *msg,
					      const SaAmfCSIStandbyDescriptorT
					      *standbyDescriptor);
	void U_printSaAmfCSIAttributeList(const char *msg,
					  const SaAmfCSIAttributeListT
					  *csiAttrListPtr);

#ifdef __cplusplus
}				/* extern "C" */
#endif				/* __cplusplus */
#endif				//J_UTILS_PRINT_H_
