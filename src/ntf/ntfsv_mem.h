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
 * Author(s): Ericsson AB
 *
 */

#ifndef NTF_NTFSV_MEM_H_
#define NTF_NTFSV_MEM_H_

#include "ntf/saf/saNtf.h"
#include "ntf/ntfsv_msg.h"

#ifdef  __cplusplus
extern "C" {
#endif
SaAisErrorT ntfsv_alloc_ntf_header(SaNtfNotificationHeaderT *notificationHeader,
                                   SaUint16T numCorrelatedNotifications,
                                   SaUint16T lengthAdditionalText, SaUint16T numAdditionalInfo);
SaAisErrorT ntfsv_alloc_ntf_alarm(SaNtfAlarmNotificationT *alarmNotification,
                                  SaUint16T numSpecificProblems,
                                  SaUint16T numMonitoredAttributes, SaUint16T numProposedRepairActions);
SaAisErrorT ntfsv_alloc_ntf_obj_create_del(SaNtfObjectCreateDeleteNotificationT *objCrDelNotification,
                                           SaUint16T numAttributes);
SaAisErrorT ntfsv_alloc_ntf_attr_change(SaNtfAttributeChangeNotificationT *attrChangeNotification,
                                        SaUint16T numAttributes);
SaAisErrorT ntfsv_alloc_ntf_state_change(SaNtfStateChangeNotificationT *stateChangeNotification,
                                         SaUint16T numStateChanges);
SaAisErrorT ntfsv_alloc_ntf_security_alarm(SaNtfSecurityAlarmNotificationT *securityAlarm);

void ntfsv_free_header(const SaNtfNotificationHeaderT *notificationHeader, bool deallocate_longdn);
void ntfsv_free_alarm(SaNtfAlarmNotificationT *alarm, bool deallocate_longdn);
void ntfsv_free_state_change(SaNtfStateChangeNotificationT *stateChange, bool deallocate_longdn);
void ntfsv_free_attribute_change(SaNtfAttributeChangeNotificationT *attrChange, bool deallocate_longdn);
void ntfsv_free_obj_create_del(SaNtfObjectCreateDeleteNotificationT *objCrDel, bool deallocate_longdn);
void ntfsv_free_security_alarm(SaNtfSecurityAlarmNotificationT *secAlarm, bool deallocate_longdn);
void ntfsv_dealloc_notification(ntfsv_send_not_req_t *param);

void ntfsv_copy_ntf_header(SaNtfNotificationHeaderT *dest, const SaNtfNotificationHeaderT *src);
void ntfsv_copy_ntf_alarm(SaNtfAlarmNotificationT *dest, const SaNtfAlarmNotificationT *src);
void ntfsv_copy_ntf_obj_cr_del(SaNtfObjectCreateDeleteNotificationT *dest,
                               const SaNtfObjectCreateDeleteNotificationT *src);
void ntfsv_copy_ntf_security_alarm(SaNtfSecurityAlarmNotificationT *dest,
                                   const SaNtfSecurityAlarmNotificationT *src);
void ntfsv_copy_ntf_attr_change(SaNtfAttributeChangeNotificationT *dest,
                                const SaNtfAttributeChangeNotificationT *src);
void ntfsv_copy_ntf_state_change(SaNtfStateChangeNotificationT *dest, const SaNtfStateChangeNotificationT *src);

SaAisErrorT ntfsv_alloc_and_copy_not(ntfsv_send_not_req_t *dest, const ntfsv_send_not_req_t *src);
void ntfsv_get_ntf_header(ntfsv_send_not_req_t *notif, SaNtfNotificationHeaderT **ntfHeader);

SaAisErrorT ntfsv_variable_data_init(v_data * vd, SaInt32T max_data_size, SaUint32T ntfsv_var_data_limit);
SaAisErrorT ntfsv_ptr_val_alloc(v_data * vd, SaNtfValueT *nv, SaUint16T data_size, void **data_ptr);
SaAisErrorT ntfsv_ptr_val_get(v_data * vd, const SaNtfValueT *nv, void **data_ptr, SaUint16T *data_size);
SaAisErrorT ntfsv_array_val_alloc(v_data * vd,
                                  SaNtfValueT *nv,
                                  SaUint16T numElements, SaUint16T elementSize, void **array_ptr);
SaAisErrorT ntfsv_array_val_get(v_data * vd,
                                const SaNtfValueT *nv,
                                void **arrayPtr, SaUint16T *numElements, SaUint16T *elementSize);
SaAisErrorT ntfsv_v_data_cp(v_data * dest, const v_data * src);

SaAisErrorT ntfsv_filter_header_alloc(SaNtfNotificationFilterHeaderT *header,
                                      SaUint16T numEventTypes,
                                      SaUint16T numNotificationObjects,
                                      SaUint16T numNotifyingObjects,
                                      SaUint16T numNotificationClassIds);
SaAisErrorT ntfsv_filter_obj_cr_del_alloc(SaNtfObjectCreateDeleteNotificationFilterT *filter,
                                          SaUint16T numSourceIndicators);
SaAisErrorT ntfsv_filter_attr_change_alloc(SaNtfAttributeChangeNotificationFilterT *filter,
                                           SaUint16T numSourceIndicators);
SaAisErrorT ntfsv_filter_state_ch_alloc(SaNtfStateChangeNotificationFilterT *f, SaUint32T numSourceIndicators, SaUint32T numChangedStates);
SaAisErrorT ntfsv_filter_alarm_alloc(SaNtfAlarmNotificationFilterT *filter, SaUint32T numProbableCauses,
                                     SaUint32T numPerceivedSeverities,
                                     SaUint32T numTrends);
SaAisErrorT ntfsv_filter_sec_alarm_alloc(SaNtfSecurityAlarmNotificationFilterT *filter,
                                         SaUint32T numProbableCauses,
                                         SaUint32T numSeverities,
                                         SaUint32T numSecurityAlarmDetectors,
                                         SaUint32T numServiceUsers,
                                         SaUint32T numServiceProviders);

void ntfsv_filter_header_free(SaNtfNotificationFilterHeaderT *header, bool deallocate_longdn);
void ntfsv_filter_sec_alarm_free(SaNtfSecurityAlarmNotificationFilterT *s_filter, bool deallocate_longdn);
void ntfsv_filter_alarm_free(SaNtfAlarmNotificationFilterT *a_filter, bool deallocate_longdn);
void ntfsv_filter_state_ch_free(SaNtfStateChangeNotificationFilterT *f, bool deallocate_longdn);
void ntfsv_filter_obj_cr_del_free(SaNtfObjectCreateDeleteNotificationFilterT *f, bool deallocate_longdn);
void ntfsv_filter_attr_ch_free(SaNtfAttributeChangeNotificationFilterT *f, bool deallocate_longdn);

SaAisErrorT ntfsv_sanamet_copy(SaNameT* pDes, SaNameT* pSrc);
bool ntfsv_sanamet_is_valid(const SaNameT* pName);
size_t ntfs_sanamet_length(const SaNameT* pName);
void ntfs_sanamet_steal(SaStringT value, size_t length, SaNameT* pName);
void ntfs_sanamet_alloc(SaConstStringT value, size_t length, SaNameT* pName);

SaAisErrorT ntfsv_copy_ntf_filter_header(SaNtfNotificationFilterHeaderT*, const SaNtfNotificationFilterHeaderT*);
SaAisErrorT ntfsv_copy_ntf_filter_alarm(SaNtfAlarmNotificationFilterT*, const SaNtfAlarmNotificationFilterT*);
SaAisErrorT ntfsv_copy_ntf_filter_sec_alarm(SaNtfSecurityAlarmNotificationFilterT*, const SaNtfSecurityAlarmNotificationFilterT*);
SaAisErrorT ntfsv_copy_ntf_filter_state_ch(SaNtfStateChangeNotificationFilterT*, const SaNtfStateChangeNotificationFilterT*);
SaAisErrorT ntfsv_copy_ntf_filter_obj_cr_del(SaNtfObjectCreateDeleteNotificationFilterT*, const SaNtfObjectCreateDeleteNotificationFilterT*);
SaAisErrorT ntfsv_copy_ntf_filter_attr_ch(SaNtfAttributeChangeNotificationFilterT*, const SaNtfAttributeChangeNotificationFilterT*);
#ifdef  __cplusplus
}
#endif

#endif  // NTF_NTFSV_MEM_H_
