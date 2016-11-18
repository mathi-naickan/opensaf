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

#include "ntf/ntfd/NtfNotification.h"
#include "base/logtrace.h"
#include "ntf/ntfsv_enc_dec.h"
#include "ntf/ntfsv_mem.h"

/**
 * This is the constructor.
 *
 * Initial variables are set. Local copy is made about the received
 * notification struct.
 *
 * @param notificationId
 *               Cluster-wide unique id of the notification.
 * @param notificationType
 *               Type of the notification (e.g. alarm, object change etc.).
 * @param sendNotInfo
 *               Pointer to the notification struct.
 */
NtfNotification::NtfNotification
(SaNtfIdentifierT notificationId,
 SaNtfNotificationTypeT notificationType,
 ntfsv_send_not_req_t* sendNotInfo):notificationId_(notificationId) {
  logged = false;
  loggFromCallback_ = false;
  TRACE_3("constructor %p, notId: %llu", this, notificationId);
  sendNotInfo_ = sendNotInfo;
  SaNtfNotificationHeaderT *header;
  ntfsv_get_ntf_header(sendNotInfo_, &header);
  *header->notificationId = notificationId_;
  notificationType_ = notificationType; /* deleted in destructor */
}

/**
 * This is the destructor.
 */
NtfNotification::~NtfNotification() {
  TRACE_3("Notification %llu with type %x destroyed.\n destructor this = %p",
          notificationId_, notificationType_, this);
  osafassert(sendNotInfo_);
  ntfsv_dealloc_notification(sendNotInfo_);
  free(sendNotInfo_);
  sendNotInfo_ = NULL;
}

/**
 * This method is called to get the type of the notification.
 *
 * @return Type of the notification (e.g. alarm, object change etc.).
 */
SaNtfNotificationTypeT NtfNotification::getNotificationType() const
{
  return notificationType_;
}

/**
 * This method is called to get the id of the notification.
 *
 * @return Cluster-wide unique id of the notification.
 */
SaNtfIdentifierT NtfNotification::getNotificationId() const
{
  return notificationId_;
}

/**
 * This method is called to store the id of a matching subscription in a list.
 *
 * When a new notifiaction is received, it is sent to all client objects.
 * The client objects check their subscriptions and they store the
 * id of the matching subscriptions here.
 *
 * @param clientId Node-wide unique id of a client who has a matching subscription.
 * @param subscriptionId
 *                 Client-wide unique id of a matching subscription.
 */
void NtfNotification::storeMatchingSubscription(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId) {

  UniqueSubscriptionId uniqueSubscriptionId;
  uniqueSubscriptionId.clientId = clientId;
  uniqueSubscriptionId.subscriptionId = subscriptionId;

  subscriptionList.push_back(uniqueSubscriptionId);
  TRACE_1("Subscription %u added to list in notification %llu client %u"
          ", subscriptionList size is %u",
          subscriptionId, notificationId_, clientId, (unsigned int)subscriptionList.size());
}

/**
 * This method is called when sending the notification to a certain subscription is confirmed.
 *
 * The subscriptions that match this notification are stored in a list.
 * When the notification was delivered to a subscription, this subscription
 * is removed from the list.
 *
 * @param clientId Node-wide unique id of the client who received the notification.
 * @param subscriptionId
 *                 Client-wide unique id of the subscription that matched the delivered notification.
 */
void NtfNotification::notificationSentConfirmed(unsigned int clientId,
                                                SaNtfSubscriptionIdT subscriptionId) {

  SubscriptionList::iterator pos = subscriptionList.begin();

  // scan until the end of the list or subscription found
  while ( (pos != subscriptionList.end())&&
          !((pos->clientId == clientId)&&
            (pos->subscriptionId == subscriptionId) ) ) {
    TRACE_1("Searching for subscription %u"
            " client %u in notification %llu",
            subscriptionId,
            clientId,
            notificationId_);
    pos++;
  }

  if (pos != subscriptionList.end()) {
    (void)subscriptionList.erase(pos);
    TRACE_1("Removing subscription %u"
            " client %u from notification %llu,"
            " subscriptionList size is %u",
            subscriptionId, clientId,
            notificationId_, (unsigned int)subscriptionList.size());
  }
  else
  {
    TRACE_1("Subscription %u,"
            " client %u not found in notification %llu",
            subscriptionId, clientId, notificationId_);
  }
}

void NtfNotification::resetSubscriptionIdList() {
  idListPos = subscriptionList.begin();
}

/**
 * Get the UniqueSubscriptionId for the first subscription in
 * subscriptionList.
 *
 * @return  SA_AIS_ERR_NOT_EXIST or SA_AIS_OK
 *
 */
SaAisErrorT NtfNotification::getNextSubscription(UniqueSubscriptionId& subId) {
  if (idListPos != subscriptionList.end()) {
    subId = *idListPos;
    idListPos++;
    return SA_AIS_OK;
  }
  else
  {
    return SA_AIS_ERR_NOT_EXIST;
  }
}

void NtfNotification::notificationLoggedConfirmed() {
  this->logged = true;
}

/**
 *
 */
bool NtfNotification::loggedOk() const
{
  TRACE_ENTER();
  if ((getNotificationType() != SA_NTF_TYPE_ALARM) &&
      (getNotificationType() != SA_NTF_TYPE_SECURITY_ALARM)) {
    TRACE("Not alarm and Not recurity alarm");
    return true;
  }
  TRACE_LEAVE();
  return logged;
}

/**
 * This method is called to check whether the list of matching subscriptions is empty.
 *
 * @return true if the list is empty
 *         false if there are matching subscriptions in the list
 */
bool NtfNotification::isSubscriptionListEmpty() const
{
  return(subscriptionList.size() == 0) ? true : false;
}

/**
 * This method is called when subscriptions of a certain client should be removed.
 *
 * If a client is removed, this notification does not need to be delivered
 * to its subscriptions.
 *
 * @param clientId Node-wide unique id of the removed client.
 */
void NtfNotification::removeSubscription(unsigned int clientId) {

  SubscriptionList::iterator pos = subscriptionList.begin();

  // scan until the end of the list
  while (pos != subscriptionList.end()) {
    TRACE_1("Searching for subscription for client %u in notification %llu",
            clientId, notificationId_);
    if ((pos->clientId == clientId) ) {
      TRACE_1("Removing subscription %u for client %u in notification %llu",
              pos->subscriptionId, clientId, notificationId_);
      pos = subscriptionList.erase(pos);
      TRACE_1("New subscriptionList size is %u",
              (unsigned int)subscriptionList.size());
    }
    else
    {
      pos++;
    }
  }
}

/**
 * This method is called when a certain subscription was removed.
 *
 * @param clientId Node-wide unique id of the client whose subscription was removed.
 * @param subscriptionId
 *                 Client-wide unique id of the removed client.
 */
void NtfNotification::removeSubscription(unsigned int clientId,
                                         SaNtfSubscriptionIdT subscriptionId) {

  SubscriptionList::iterator pos = subscriptionList.begin();

  // scan until the end of the list
  while (pos != subscriptionList.end()) {
    TRACE_1("Searching for subscription for client %u, subscription %d in notification %llu",
            clientId, subscriptionId, notificationId_);
    if ( (pos->clientId == clientId) &&
         (pos->subscriptionId == subscriptionId) ) {
      TRACE_1("Removing subscription %u for client %u in notification %llu",
              subscriptionId, clientId, notificationId_);
      pos = subscriptionList.erase(pos);
      TRACE_1("New subscriptionList size is %u",
              (unsigned int)subscriptionList.size());
      break;
    }
    else
    {
      pos++;
    }
  }
}

/**
 * This method is called to fetch the pointer to the notification struct.
 *
 * @return Pointer to the notification struct.
 */
ntfsv_send_not_req_t* NtfNotification::getNotInfo() {
  return sendNotInfo_;
}

/**
 * This method is called if a newly started standby asks for
 * synchronization.
 *
 */
void NtfNotification::syncRequest(NCS_UBAID *uba) {
  TRACE_1("NtfNotification::syncRequest received"
          " in notification %llu", notificationId_);
  int retval = sendNewNotification(0, sendNotInfo_, uba);
  if (retval != 1) {
    LOG_ER("NtfNotification::syncRequest sendNewNotification was not sent,"
           " error code is %d", retval);
    osafassert(0);
  }
  sendMapNoOfSubscriptionToNotification(subscriptionList.size(), uba);
  SubscriptionList::iterator pos;
  for (pos = subscriptionList.begin(); pos != subscriptionList.end(); pos++) {
    TRACE_1("Matching subscription found, client %u,"
            " subscription %u in notification %llu",
            pos->clientId, pos->subscriptionId, notificationId_);
    sendMapSubscriptionToNotification(pos->clientId, pos->subscriptionId, uba);
  }
  syncLoggedConfirm((unsigned int) loggedOk(), uba);
}

void NtfNotification::printInfo() {
  TRACE("Notification information");
  TRACE("  notificationId:        %llu", notificationId_);
  TRACE("  notificationType:      %d", (int)notificationType_);
  TRACE("  logged:             %d", (int)logged);
  TRACE("  subscriptionList size  %u", (unsigned int)subscriptionList.size());
  SubscriptionList::iterator pos = subscriptionList.begin();
  // scan until the end of the list
  while (pos != subscriptionList.end()) {
    TRACE("  clientId %u, subscriptionId: %u",
          pos->clientId, pos->subscriptionId);
    pos++;
  }
}

SaNtfNotificationHeaderT* NtfNotification::header() {
  SaNtfNotificationHeaderT *header;
  ntfsv_get_ntf_header(this->getNotInfo(), &header);
  return header;
}

