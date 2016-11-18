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
 *   This file contains the declaration of base class NtfFilter
 *   and the derived classes NtfAlarmFilter,
 *   NtfSecurityAlarmFilter, NtfStateChangeFilter,
 *   NtfAttributeChangeFilter and NtfObjectCreateDeleteFilter.
 */

#ifndef NTF_NTFD_NTFFILTER_H_
#define NTF_NTFD_NTFFILTER_H_

#include "ntf/saf/saNtf.h"
#include "ntf/ntfd/NtfNotification.h"

class NtfFilter
{
 public:
  NtfFilter(SaNtfNotificationTypeT filterType);
  virtual ~NtfFilter();
  virtual bool checkFilter(NtfSmartPtr& notif) = 0;
  SaNtfNotificationTypeT type();
  bool checkHeader(SaNtfNotificationFilterHeaderT *h, NtfSmartPtr& notif);
  bool checkEventType(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h);
  bool checkNtfClassId(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h);
  bool checkNotificationObject(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h);
  bool checkNotifyingObject(SaNtfNotificationFilterHeaderT *fh, const SaNtfNotificationHeaderT *h);
  bool checkSourceIndicator(SaUint16T numSi, SaNtfSourceIndicatorT *sis, SaNtfSourceIndicatorT *s);
  bool cmpSaNameT(SaNameT *n, SaNameT *n2);
  bool cmpSaNtfValueT(SaNtfValueTypeT t, SaNtfValueT *v, SaNtfValueTypeT t2, SaNtfValueT *v2);

 private:

  SaNtfNotificationTypeT filterType_;
};

class NtfAlarmFilter:public NtfFilter
{
 public:
  NtfAlarmFilter(SaNtfAlarmNotificationFilterT *f);
  ~NtfAlarmFilter();
  bool checkFilter(NtfSmartPtr& notif);
  bool checkTrend(SaNtfAlarmNotificationT *a);
  bool checkPerceivedSeverity(SaNtfAlarmNotificationT *a);
  bool checkprobableCause(SaNtfAlarmNotificationT *a);

 private:
  SaNtfAlarmNotificationFilterT *filter_;
};

class NtfSecurityAlarmFilter: public NtfFilter
{
 public:
  NtfSecurityAlarmFilter(SaNtfSecurityAlarmNotificationFilterT *f);
  ~NtfSecurityAlarmFilter();
  bool checkFilter(NtfSmartPtr& notif);
  bool checkProbableCause(SaNtfSecurityAlarmNotificationT *s);
  bool checkSeverity(SaNtfSecurityAlarmNotificationT *s);
  bool checkSecurityAlarmDetector(SaNtfSecurityAlarmNotificationT *s);
  bool checkServiceUser(SaNtfSecurityAlarmNotificationT *s);
  bool checkServiceProvider(SaNtfSecurityAlarmNotificationT *s);

 private:
  SaNtfSecurityAlarmNotificationFilterT *filter_;
};

class NtfObjectCreateDeleteFilter: public NtfFilter
{
 public:
  NtfObjectCreateDeleteFilter(SaNtfObjectCreateDeleteNotificationFilterT *f);
  ~NtfObjectCreateDeleteFilter();
  bool checkFilter(NtfSmartPtr& notif);

 private:
  SaNtfObjectCreateDeleteNotificationFilterT *filter_;
};

class NtfStateChangeFilter: public NtfFilter
{
 public:
  NtfStateChangeFilter(SaNtfStateChangeNotificationFilterT *f);
  ~NtfStateChangeFilter();
  bool checkFilter(NtfSmartPtr& notif);
  bool checkStateId(SaUint16T ns, SaNtfStateChangeT * sc);

 private:
  SaNtfStateChangeNotificationFilterT *filter_;
};

class NtfAttributeChangeFilter: public NtfFilter
{
 public:
  NtfAttributeChangeFilter(SaNtfAttributeChangeNotificationFilterT *f);
  ~NtfAttributeChangeFilter();
  bool checkFilter(NtfSmartPtr& notif);

 private:
  SaNtfAttributeChangeNotificationFilterT *filter_;
};

typedef std::map<SaNtfNotificationTypeT,NtfFilter*> FilterMap;


#endif  // NTF_NTFD_NTFFILTER_H_




