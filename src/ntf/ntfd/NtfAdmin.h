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

#ifndef NTF_NTFD_NTFADMIN_H_
#define NTF_NTFD_NTFADMIN_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include "ntf/ntfd/NtfNotification.h"
#include "ntfs_com.h"
#include "ntf/ntfd/NtfClient.h"
#include "ntf/ntfd/NtfFilter.h"
#include "ntf/ntfd/NtfSubscription.h"
#include "assert.h"
#include "ntf/ntfd/NtfLogger.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

typedef std::map<SaNtfIdentifierT,NtfSmartPtr> NotificationMap;

class NtfAdmin{
 public:

  NtfAdmin();
  virtual ~NtfAdmin();
  void clientAdded(unsigned int clientId,
                   MDS_DEST mdsDest,
                   MDS_SYNC_SND_CTXT *mdsCtxt, SaVersionT *version);
  void subscriptionAdded(ntfsv_subscribe_req_t s, MDS_SYNC_SND_CTXT *mdsDest);
  void notificationReceived(unsigned int clientId,
                            SaNtfNotificationTypeT notificationType,
                            ntfsv_send_not_req_t* sendNotInfo,
                            MDS_SYNC_SND_CTXT *mdsCtxt);
  void notificationReceivedUpdate(unsigned int clientId,
                                  SaNtfNotificationTypeT notificationType,
                                  ntfsv_send_not_req_t* sendNotInfo);
  void notificationReceivedColdSync(unsigned int clientId,
                                    SaNtfNotificationTypeT notificationType,
                                    ntfsv_send_not_req_t* sendNotInfo);
  void notificationSentConfirmed(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId,
                                 SaNtfIdentifierT notificationId,  int discarded);
  void notificationLoggedConfirmed(SaNtfIdentifierT notificationId);
  void clientRemoved(unsigned int clientId);
  void clientRemoveMDS(MDS_DEST mds_dest);
  void subscriptionRemoved(unsigned int clientId,
                           SaNtfSubscriptionIdT subscriptionId,
                           MDS_SYNC_SND_CTXT *mdsCtxt);
  void syncRequest(NCS_UBAID *uba);
  void syncGlobals(const struct NtfGlobals& ntfGlobals);
  void newReader(unsigned int clientId,
                 SaNtfSearchCriteriaT searchCriteria,
                 ntfsv_filter_ptrs_t *f_rec,
                 MDS_SYNC_SND_CTXT *mdsCtxt);

  void readNext(unsigned int connectionId,
                unsigned int readerId,
                SaNtfSearchDirectionT searchDirection,
                MDS_SYNC_SND_CTXT *mdsCtxt);
  void deleteReader(unsigned int connectionId,
                    unsigned int readerId,
                    MDS_SYNC_SND_CTXT *mdsCtxt);

  void printInfo();
  void storeMatchingSubscription(SaNtfIdentifierT notificationId,
                                 unsigned int clientId,
                                 SaNtfSubscriptionIdT subscriptionId);

  NtfSmartPtr getNotificationById(SaNtfIdentifierT id);
  void checkNotificationList();
  NtfClient* getClient(unsigned int clientId);
  void deleteConfirmedNotification(NtfSmartPtr notification,
                                   NotificationMap::iterator pos);
  void discardedAdd(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId, SaNtfIdentifierT notificationId);
  void discardedClear(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId);
  static NtfAdmin* theNtfAdmin;
  NtfLogger logger;

  void AddMemberNode(NODE_ID node_id);
  NODE_ID* FindMemberNode(NODE_ID node_id);
  void RemoveMemberNode(NODE_ID node_id);
  uint32_t MemberNodeListSize();
  void PrintMemberNodes();
  uint32_t send_cluster_membership_msg_to_clients(SaClmClusterChangesT cluster_change, NODE_ID node_id);
  bool is_stale_client(unsigned int clientId);
 private:
  void processNotification(unsigned int clientId,
                           SaNtfNotificationTypeT notificationType,
                           ntfsv_send_not_req_t* sendNotInfo,
                           MDS_SYNC_SND_CTXT *mdsCtxt,
                           SaNtfIdentifierT notificationId);

  void updateNotIdCounter(SaNtfIdentifierT notification);

  typedef std::map<unsigned int,NtfClient*> ClientMap;
  ClientMap clientMap;
  NotificationMap notificationMap;
  SaNtfIdentifierT notificationIdCounter;
  unsigned int clientIdCounter;
  std::list<NODE_ID*> member_node_list; /*To maintain NCS node_ids of CLM memeber nodes.*/
};

#endif  // NTF_NTFD_NTFADMIN_H_
