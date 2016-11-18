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
 * Author(s): Ericsson AB
 *
 */

#ifndef AMF_AGENT_AMF_AGENT_H_
#define AMF_AGENT_AMF_AGENT_H_

class AmfAgent {
 public:
  static SaAisErrorT Initialize(SaAmfHandleT *o_hdl, const SaAmfCallbacksT *reg_cbks, SaVersionT *io_ver);

  static SaAisErrorT SelectionObjectGet(SaAmfHandleT hdl, SaSelectionObjectT *o_sel_obj);

  static SaAisErrorT Dispatch(SaAmfHandleT hdl, SaDispatchFlagsT flags);

  static SaAisErrorT Finalize(SaAmfHandleT hdl);

  static SaAisErrorT ComponentRegister(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *proxy_comp_name);

  static SaAisErrorT ComponentUnregister(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *proxy_comp_name);

  static SaAisErrorT HealthcheckStart(SaAmfHandleT hdl, const SaNameT *comp_name,
                                      const SaAmfHealthcheckKeyT *hc_key,
                                      SaAmfHealthcheckInvocationT inv, SaAmfRecommendedRecoveryT rec_rcvr);

  static SaAisErrorT HealthcheckStop(SaAmfHandleT hdl, const SaNameT *comp_name, const SaAmfHealthcheckKeyT *hc_key);

  static SaAisErrorT HealthcheckConfirm(SaAmfHandleT hdl,
                                        const SaNameT *comp_name, const SaAmfHealthcheckKeyT *hc_key, SaAisErrorT hc_result);

  static SaAisErrorT PmStart(SaAmfHandleT hdl,
                             const SaNameT *comp_name,
                             SaUint64T processId,
                             SaInt32T desc_TreeDepth, SaAmfPmErrorsT pmErr, SaAmfRecommendedRecoveryT rec_Recovery);

  static SaAisErrorT PmStop(SaAmfHandleT hdl,
                            const SaNameT *comp_name,
                            SaAmfPmStopQualifierT stopQual, SaInt64T processId, SaAmfPmErrorsT pmErr);

  static SaAisErrorT ComponentNameGet(SaAmfHandleT hdl, SaNameT *o_comp_name);

  static SaAisErrorT CSIQuiescingComplete(SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error);

  static SaAisErrorT HAStateGet(SaAmfHandleT hdl, const SaNameT *comp_name, const SaNameT *csi_name, SaAmfHAStateT *o_ha);

  static SaAisErrorT ProtectionGroupTrack(SaAmfHandleT hdl,
                                          const SaNameT *csi_name,
                                          SaUint8T flags, SaAmfProtectionGroupNotificationBufferT *buf);

  static SaAisErrorT ProtectionGroupTrackStop(SaAmfHandleT hdl, const SaNameT *csi_name);

  static SaAisErrorT ComponentErrorReport(SaAmfHandleT hdl,
                                          const SaNameT *err_comp,
                                          SaTimeT err_time, SaAmfRecommendedRecoveryT rec_rcvr, SaNtfIdentifierT ntf_id);

  static SaAisErrorT ComponentErrorClear(SaAmfHandleT hdl, const SaNameT *comp_name, SaNtfIdentifierT ntf_id);

  static SaAisErrorT Response(SaAmfHandleT hdl, SaInvocationT inv, SaAisErrorT error);

  static SaAisErrorT Initialize_4(SaAmfHandleT *o_hdl, const SaAmfCallbacksT_4 *reg_cbks, SaVersionT *io_ver);

  static SaAisErrorT PmStart_3(SaAmfHandleT hdl,
                               const SaNameT *comp_name,
                               SaInt64T processId,
                               SaInt32T desc_TreeDepth, SaAmfPmErrorsT pmErr, SaAmfRecommendedRecoveryT rec_Recovery);

  static SaAisErrorT ProtectionGroupTrack_4(SaAmfHandleT hdl,
                                            const SaNameT *csi_name,
                                            SaUint8T flags, SaAmfProtectionGroupNotificationBufferT_4 *buf);

  static SaAisErrorT ProtectionGroupNotificationFree_4(SaAmfHandleT hdl, SaAmfProtectionGroupNotificationT_4 *notification);

  static SaAisErrorT ComponentErrorReport_4(SaAmfHandleT hdl,
                                            const SaNameT *err_comp,
                                            SaTimeT err_time, SaAmfRecommendedRecoveryT rec_rcvr, SaNtfCorrelationIdsT *corr_ids);

  static SaAisErrorT ComponentErrorClear_4(SaAmfHandleT hdl, const SaNameT *comp_name, SaNtfCorrelationIdsT *corr_ids);

  static SaAisErrorT Response_4(SaAmfHandleT hdl, SaInvocationT inv, SaNtfCorrelationIdsT *corr_ids, SaAisErrorT error);

};

#endif  // AMF_AGENT_AMF_AGENT_H_
