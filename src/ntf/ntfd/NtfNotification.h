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
 *   This object handles information about NTF notifications.
 */

#ifndef NTF_NTFD_NTFNOTIFICATION_H_
#define NTF_NTFD_NTFNOTIFICATION_H_

#include <list>
#include <deque>
#include <iterator>
#include <map>
#include <queue>
#include "ntfs_com.h"
#include "ntf/ntfsv_msg.h"
#include <tr1/memory>

#define NACK_THRESHOLD 4

typedef struct
{
  unsigned int clientId;
  SaNtfSubscriptionIdT subscriptionId;
} UniqueSubscriptionId;

class NtfNotification{

 public:

  NtfNotification(SaNtfIdentifierT notificationId,
                  SaNtfNotificationTypeT notificationType,
                  ntfsv_send_not_req_t* sendNotInfo);

  virtual ~NtfNotification();
  SaNtfNotificationTypeT getNotificationType() const;
  SaNtfIdentifierT getNotificationId() const;
  void storeMatchingSubscription(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId);
  void notificationSentConfirmed(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId);
  void notificationLoggedConfirmed();
  bool loggedOk() const;
  bool isSubscriptionListEmpty() const;
  void removeSubscription(unsigned int clientId);
  void removeSubscription(unsigned int clientId,
                          SaNtfSubscriptionIdT subscriptionId);
  ntfsv_send_not_req_t* getNotInfo();
  void syncRequest(NCS_UBAID *uba);
  void resetSubscriptionIdList();
  SaAisErrorT getNextSubscription(UniqueSubscriptionId& subId);
  void printInfo();
  SaNtfNotificationHeaderT* header();
  ntfsv_send_not_req_t *sendNotInfo_;
  bool loggFromCallback_;

 private:
  NtfNotification();
  NtfNotification(const NtfNotification&);
  NtfNotification& operator=(const NtfNotification&);

  bool logged;
  SaNtfIdentifierT notificationId_;
  SaNtfNotificationTypeT notificationType_;
  typedef std::list<UniqueSubscriptionId> SubscriptionList;
  SubscriptionList subscriptionList;
  SubscriptionList::iterator idListPos;
};

typedef std::tr1::shared_ptr<NtfNotification> NtfSmartPtr;

#endif  // NTF_NTFD_NTFNOTIFICATION_H_

