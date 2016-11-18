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
 *   This object handles information about NTF subscriptions.
 */

#ifndef NTF_NTFD_NTFSUBSCRIPTION_H_
#define NTF_NTFD_NTFSUBSCRIPTION_H_

#include "ntf/ntfd/NtfFilter.h"
#include "ntf/ntfd/NtfNotification.h"
#include "ntf/saf/saNtf.h"

class NtfClient;

class NtfSubscription{

 public:
  NtfSubscription(ntfsv_subscribe_req_t* s);
  virtual ~NtfSubscription();
  bool checkSubscription(NtfSmartPtr& notification);
  void confirmNtfSend();
  SaNtfSubscriptionIdT getSubscriptionId() const;
  ntfsv_subscribe_req_t* getSubscriptionInfo();
  void printInfo();
  void sendNotification(NtfSmartPtr& notification, NtfClient *client);
  void discardedAdd(SaNtfIdentifierT n_id);
  void discardedClear();
  void syncRequest(NCS_UBAID *uba);
 private:
  FilterMap filterMap;
  SaNtfSubscriptionIdT subscriptionId_;
  ntfsv_subscribe_req_t s_info_;
  typedef std::list<SaNtfIdentifierT> DiscardedNotificationIdList;
  DiscardedNotificationIdList discardedNotificationIdList;

};

#endif  // NTF_NTFD_NTFSUBSCRIPTION_H_
