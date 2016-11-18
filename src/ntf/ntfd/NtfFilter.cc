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

/**
 *   This file contains the implementaion of base class NtfFilter and the
 *   derived classes NtfAlarmFilter, NtfSecurityAlarmFilter, NtfStateChangeFilter,
 *   NtfAttributeChangeFilter and NtfObjectCreateDeleteFilter. The method checkFilter
 *   is implemented in each class, checkFilter returns true if the filter matches the
 *   notification.
 */

#include "ntf/ntfd/NtfFilter.h"
#include <string>
#include "base/logtrace.h"
#include "ntf/ntfsv_mem.h"
#include "base/osaf_extended_name.h"
/**
 * Constructor.
 *
 * @param filterType
 *          Type of this filter object (i.e. alarm, object
 *          change etc.).
 */
NtfFilter::NtfFilter(SaNtfNotificationTypeT filterType) {

  filterType_ = filterType;
  TRACE_2("Filter %#x created", filterType_);
}

/**
 * This is the destructor.
 */
NtfFilter::~NtfFilter() {
  TRACE_2("Filter %#x destroyed", filterType_);
}

SaNtfNotificationTypeT NtfFilter::type() {
  return filterType_;
}

bool NtfFilter::checkEventType(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h) {
  TRACE_8("num EventTypes: %hd", fh->numEventTypes);
  if (fh->numEventTypes) {
    for (int i = 0; i < fh->numEventTypes; i++) {
      if(*h->eventType == fh->eventTypes[i]) {
        TRACE_2("EventTypes matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfFilter::checkNtfClassId(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h) {
  TRACE_8("numNotificationClassIds: %hd", fh->numNotificationClassIds);
  if (fh->numNotificationClassIds) {
    for (int i = 0; i < fh->numNotificationClassIds; i++)
      if(h->notificationClassId->vendorId == fh->notificationClassIds[i].vendorId &&
         h->notificationClassId->minorId == fh->notificationClassIds[i].minorId &&
         h->notificationClassId->majorId == fh->notificationClassIds[i].majorId){
        TRACE_2("notificationClassId matches");
        return true;
      }
  } else {
    return true;
  }
  return false;
}

bool NtfFilter::checkSourceIndicator(SaUint16T numSi,   SaNtfSourceIndicatorT *sis, SaNtfSourceIndicatorT *s) {
  TRACE_8("numSi: %hd", numSi);
  if (numSi) {
    for (int i = 0; i < numSi; i++) {
      if(*s == sis[i]) {
        TRACE_2("Sourceind matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

/**
 * cmpSaNameT - compare two saNameT
 *
 * @param n
 *      saNameT filter value
 * @param n2
 *      saNameT value to check
 * @return bool
 *    true if the two saNameT are equal or n2 contains n
 */
bool NtfFilter::cmpSaNameT(SaNameT *n, SaNameT *n2) {
  bool rc;
  SaConstStringT str2, str;
  size_t length, length2;

  rc = false;
  str = osaf_extended_name_borrow(n);
  length = strlen(str);
  str2 = osaf_extended_name_borrow(n2);
  length2 = strlen(str2);

  if (length != length2) {
    if (length < length2)
      rc = strstr(str2, str) != NULL;
  } else if(memcmp(str, str2, length) == 0)
    rc = true;

  return rc;
}

/**
 * cmpSaNtfValueT - compare two cmpSaNtfValueT
 *
 * @param t
 *      SaNtfValueTypeT
 * @param v
 *    SaNtfValueT
 * @param t2
 *    SaNtfValueTypeT
 * @param v2
 *      SaNtfValueT
 * @return
 *    true if the two saNtfValue's has the same value type and
 *    are equal.
 */
bool NtfFilter::cmpSaNtfValueT(SaNtfValueTypeT t, SaNtfValueT *v, SaNtfValueTypeT t2, SaNtfValueT *v2) {
  if (t != t2)
    return false;
  switch(t) {
    case SA_NTF_VALUE_UINT8:
      return (v->uint8Val == v2->uint8Val);
    case SA_NTF_VALUE_INT8:
      return (v->int8Val == v2->int8Val);
    case SA_NTF_VALUE_UINT16:
      return (v->uint16Val == v2->uint16Val);
    case SA_NTF_VALUE_INT16:
      return (v->int16Val == v2->int16Val);
    case SA_NTF_VALUE_UINT32:
      return (v->uint32Val == v2->uint32Val);
    case SA_NTF_VALUE_INT32:
      return (v->int32Val == v2->int32Val);
    case SA_NTF_VALUE_FLOAT:
      return (v->floatVal == v2->floatVal);
    case SA_NTF_VALUE_UINT64:
      return (v->uint64Val == v2->uint64Val);
    case SA_NTF_VALUE_INT64:
      return (v->int64Val == v2->int64Val);
    case SA_NTF_VALUE_DOUBLE:
      return (v->doubleVal == v2->doubleVal);

      /* TODO: Not supported yet due to no allocation function for filter handle */
    case SA_NTF_VALUE_LDAP_NAME:
    case SA_NTF_VALUE_STRING:
    case SA_NTF_VALUE_IPADDRESS:
    case SA_NTF_VALUE_BINARY:
    case SA_NTF_VALUE_ARRAY:
      return true;
    default:
      osafassert(0);
      return false;
      break;
  }
}

bool NtfFilter::checkNotificationObject(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h) {

  TRACE_8("num notificationObjects: %hd", fh->numNotificationObjects);
  if (fh->numNotificationObjects) {
    for (int i = 0; i < fh->numNotificationObjects; i++) {
      if(cmpSaNameT(&fh->notificationObjects[i], h->notificationObject)){
        TRACE_2("notificationObject matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfFilter::checkNotifyingObject(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h) {
  TRACE_8("num NotifyingObjects: %hd", fh->numNotifyingObjects);
  if (fh->numNotifyingObjects) {
    for (int i = 0; i < fh->numNotifyingObjects; i++) {
      if(cmpSaNameT(&fh->notifyingObjects[i], h->notifyingObject)){
        TRACE_2("NotifyingObject matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfFilter::checkHeader(SaNtfNotificationFilterHeaderT *fh, NtfSmartPtr& notif) {
  const SaNtfNotificationHeaderT *h = notif->header();
  if (notif->getNotificationType() != this->type())
    return false;
  bool rv = checkNtfClassId(fh, h) && checkEventType(fh, h) &&
      checkNotificationObject(fh, h) && checkNotifyingObject(fh, h);
  if (rv)
    TRACE_2("hdfilter matches");
  else
    TRACE_2("hdfilter no match");
  return rv;
}

NtfAlarmFilter::NtfAlarmFilter(SaNtfAlarmNotificationFilterT *f):NtfFilter(SA_NTF_TYPE_ALARM), filter_(f) {
  TRACE_8("Alarm filter created");
}

NtfAlarmFilter::~NtfAlarmFilter() {
  TRACE_8("destructor p = %p", filter_);
  ntfsv_filter_alarm_free(filter_, true);
  free(filter_);
}

bool NtfAlarmFilter::checkTrend(SaNtfAlarmNotificationT *a) {
  TRACE_8("num Trends: %hd", filter_->numTrends);
  if (filter_->numTrends) {
    for (int i = 0; i < filter_->numTrends; i++) {
      if(*a->trend == filter_->trends[i]) {
        TRACE_2("trends matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfAlarmFilter::checkPerceivedSeverity(SaNtfAlarmNotificationT *a) {
  TRACE_8("num Perceivedseverities: %hd", filter_->numPerceivedSeverities);
  if (filter_->numPerceivedSeverities) {
    for (int i = 0; i < filter_->numPerceivedSeverities; i++) {
      if(*a->perceivedSeverity == filter_->perceivedSeverities[i]) {
        TRACE_2("perceivedseverities matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfAlarmFilter::checkprobableCause(SaNtfAlarmNotificationT *a) {
  TRACE_8("num ProbableCauses: %hd", filter_->numProbableCauses);
  if (filter_->numProbableCauses) {
    for (int i = 0; i < filter_->numProbableCauses; i++) {
      if(*a->probableCause == filter_->probableCauses[i]) {
        TRACE_2("probableCauses matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

/**
 * checkFilter - check if the notification matches the filter.
 *
 * @param notif
 *      the notification to be checked
 * @return bool
 *    true if the filter matches the notification
 */
bool NtfAlarmFilter::checkFilter(NtfSmartPtr& notif) {
  bool rv = false;
  TRACE_ENTER();
  rv = this->checkHeader(&filter_->notificationFilterHeader, notif);
  if (rv) {
    SaNtfAlarmNotificationT *a = &(notif->getNotInfo()->notification.alarm);
    rv = checkTrend(a) && checkPerceivedSeverity(a) && checkprobableCause(a);
  }
  TRACE_LEAVE();
  return rv;
}

NtfSecurityAlarmFilter::NtfSecurityAlarmFilter(SaNtfSecurityAlarmNotificationFilterT *f):NtfFilter(SA_NTF_TYPE_SECURITY_ALARM), filter_(f) {
  TRACE_8("NtfSecurityAlarmFilter created");
}

NtfSecurityAlarmFilter::~NtfSecurityAlarmFilter() {
  TRACE_8("destructor p = %p", filter_);
  ntfsv_filter_sec_alarm_free(filter_, true);
  free(filter_);
}

/**
 * checkFilter - check if the notification matches the filter.
 *
 * @param notif
 *      the notification to be checked
 * @return bool
 *    true if the filter matches the notification
 */
bool NtfSecurityAlarmFilter::checkFilter(NtfSmartPtr& notif) {
  bool rv = false;
  TRACE_ENTER();
  rv = this->checkHeader(&filter_->notificationFilterHeader, notif);
  if (rv) {
    SaNtfSecurityAlarmNotificationT *s = &(notif->getNotInfo()->notification.securityAlarm);
    rv = checkProbableCause(s) && checkSeverity(s) && checkSecurityAlarmDetector(s) && checkServiceUser(s) && checkServiceProvider(s);
  }
  TRACE_LEAVE();
  return rv;
}

bool NtfSecurityAlarmFilter::checkProbableCause(SaNtfSecurityAlarmNotificationT *s) {
  TRACE_8("num ProbableCauses: %hd", filter_->numProbableCauses);
  if (filter_->numProbableCauses) {
    for (int i = 0; i < filter_->numProbableCauses; i++) {
      if(*s->probableCause == filter_->probableCauses[i]) {
        TRACE_2("probableCauses matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfSecurityAlarmFilter::checkSeverity(SaNtfSecurityAlarmNotificationT *s) {
  TRACE_8("severities: %hd", filter_->numSeverities);
  if (filter_->numSeverities) {
    for (int i = 0; i < filter_->numSeverities; i++) {
      if(*s->severity == filter_->severities[i]) {
        TRACE_2("Severity matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfSecurityAlarmFilter::checkServiceUser(SaNtfSecurityAlarmNotificationT *s) {
  TRACE_8("serverUsers: %hd", filter_->numServiceUsers);
  if (filter_->numServiceUsers) {
    for (int i = 0; i < filter_->numServiceUsers; i++) {
      if(cmpSaNtfValueT(s->serviceUser->valueType, &s->serviceUser->value,
                        filter_->serviceUsers[i].valueType, &filter_->serviceUsers[i].value)) {
        TRACE_2("ServiceUser matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfSecurityAlarmFilter::checkServiceProvider(SaNtfSecurityAlarmNotificationT *s) {
  TRACE_8("serverProviders: %hd", filter_->numServiceProviders);
  if (filter_->numServiceProviders) {
    for (int i = 0; i < filter_->numServiceProviders; i++) {
      if(cmpSaNtfValueT(s->serviceProvider->valueType, &s->serviceProvider->value,
                        filter_->serviceProviders[i].valueType, &filter_->serviceProviders[i].value)) {
        TRACE_2("ServiceProvider matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

bool NtfSecurityAlarmFilter::checkSecurityAlarmDetector(SaNtfSecurityAlarmNotificationT *s) {
  TRACE_8("serverProviders: %hd", filter_->numSecurityAlarmDetectors);
  if (filter_->numSecurityAlarmDetectors) {
    for (int i = 0; i < filter_->numSecurityAlarmDetectors; i++) {
      if(cmpSaNtfValueT(s->securityAlarmDetector->valueType, &s->securityAlarmDetector->value,
                        filter_->securityAlarmDetectors[i].valueType, &filter_->securityAlarmDetectors[i].value)) {
        TRACE_2("SecurityAlarmDetector matches");
        return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

NtfObjectCreateDeleteFilter::NtfObjectCreateDeleteFilter(SaNtfObjectCreateDeleteNotificationFilterT *f):
    NtfFilter(SA_NTF_TYPE_OBJECT_CREATE_DELETE), filter_(f) {
  TRACE_8("NtfObjectCreateDeleteFilter created");
}

NtfObjectCreateDeleteFilter::~NtfObjectCreateDeleteFilter() {
  TRACE_8("destructor p = %p", filter_);
  ntfsv_filter_obj_cr_del_free(filter_, true);
  free(filter_);
}

/**
 * checkFilter - check if the notification matches the filter.
 *
 * @param notif
 *      the notification to be checked
 * @return bool
 *    true if the filter matches the notification
 */
bool NtfObjectCreateDeleteFilter::checkFilter(NtfSmartPtr& notif) {
  bool rv = false;
  TRACE_ENTER();
  rv = this->checkHeader(&filter_->notificationFilterHeader, notif);
  if (rv) {
    SaNtfObjectCreateDeleteNotificationT *o = &(notif->getNotInfo()->notification.objectCreateDelete);
    rv = checkSourceIndicator(filter_->numSourceIndicators, filter_->sourceIndicators, o->sourceIndicator);
  }
  TRACE_LEAVE();
  return rv;
}

NtfStateChangeFilter::NtfStateChangeFilter(SaNtfStateChangeNotificationFilterT *f):NtfFilter(SA_NTF_TYPE_STATE_CHANGE),
                                                                                   filter_(f) {
  TRACE_8("NtfStateChangeFilter created");
                                                                                   }

NtfStateChangeFilter::~NtfStateChangeFilter() {
  TRACE_8("destructor p = %p", filter_);
  ntfsv_filter_state_ch_free(filter_, true);
  free(filter_);
}

/**
 * checkFilter - check if the notification matches the filter.
 *
 * @param notif
 *      the notification to be checked
 * @return bool
 *    true if the filter matches the notification
 */
bool NtfStateChangeFilter::checkFilter(NtfSmartPtr& notif) {
  bool rv = false;
  TRACE_ENTER();
  rv = this->checkHeader(&filter_->notificationFilterHeader, notif);
  if (rv) {
    SaNtfStateChangeNotificationT *s = &(notif->getNotInfo()->notification.stateChange);
    rv = checkSourceIndicator(filter_->numSourceIndicators, filter_->sourceIndicators, s->sourceIndicator) &&
        checkStateId(s->numStateChanges, s->changedStates);
  }
  TRACE_LEAVE();
  return rv;
}

bool NtfStateChangeFilter::checkStateId(SaUint16T ns, SaNtfStateChangeT *sc) {
  int i, j;
  if (filter_->numStateChanges) {
    for (i = 0; i< filter_->numStateChanges; i++) {
      for (j= 0; j< ns; j++){
        if (filter_->changedStates[i].stateId == sc[j].stateId)
          return true;
      }
    }
  } else {
    return true;
  }
  return false;
}

NtfAttributeChangeFilter::NtfAttributeChangeFilter(SaNtfAttributeChangeNotificationFilterT *f):
    NtfFilter(SA_NTF_TYPE_ATTRIBUTE_CHANGE), filter_(f) {
  TRACE_8("NtfAttributeChangeFilter created");
}

/**
 * checkFilter - check if the notification matches the filter.
 *
 * @param notif
 *      the notification to be checked
 * @return bool
 *    true if the filter matches the notification
 */
bool NtfAttributeChangeFilter::checkFilter(NtfSmartPtr& notif) {
  bool rv = false;
  TRACE_ENTER();
  rv = this->checkHeader(&filter_->notificationFilterHeader, notif);
  if (rv) {
    SaNtfAttributeChangeNotificationT *a = &(notif->getNotInfo()->notification.attributeChange);
    rv = checkSourceIndicator(filter_->numSourceIndicators, filter_->sourceIndicators, a->sourceIndicator);
  }
  TRACE_LEAVE();
  return rv;
}

NtfAttributeChangeFilter::~NtfAttributeChangeFilter() {
  TRACE_8("destructor p = %p", filter_);
  ntfsv_filter_attr_ch_free(filter_, true);
  free(filter_);
}


