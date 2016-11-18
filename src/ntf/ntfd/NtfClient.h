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
 *   This object handles information about NTF clients.
 *
 */

#ifndef NTF_NTFD_NTFCLIENT_H_
#define NTF_NTFD_NTFCLIENT_H_

#include "ntf/ntfd/NtfSubscription.h"
#include "ntf/ntfd/NtfNotification.h"
#include "ntf/ntfd/NtfReader.h"

class NtfClient{

 public:

  NtfClient(unsigned int clientId,
            MDS_DEST mds_dest);
  virtual ~NtfClient();
  void subscriptionAdded(NtfSubscription* subscription,
                         MDS_SYNC_SND_CTXT *mdsCtxt);
  void notificationReceived(unsigned int clientId,
                            NtfSmartPtr& notification,
                            MDS_SYNC_SND_CTXT *mdsCtxt);
  void confirmNtfSend();
  unsigned int getClientId() const;
  MDS_DEST getMdsDest() const;
  void subscriptionRemoved(SaNtfSubscriptionIdT subscriptionId,
                           MDS_SYNC_SND_CTXT *mdsCtxt);
  void syncRequest(NCS_UBAID *uba);
  void sendNotConfirmedNotification(NtfSmartPtr notification, SaNtfSubscriptionIdT subscriptionId);

  void confirmNtfNotification(SaNtfIdentifierT notificationId,
                              MDS_SYNC_SND_CTXT *mdsCtxt,
                              MDS_DEST mdsDest);
  void confirmNotificationLogged(SaNtfIdentifierT notificationId);
  void confirmNtfSubscribe(SaNtfSubscriptionIdT subscriptionId, MDS_SYNC_SND_CTXT *mds_ctxt);
  void confirmNtfUnsubscribe(SaNtfSubscriptionIdT subscriptionId,
                             MDS_SYNC_SND_CTXT *mdsCtxt);

  void newReader(SaNtfSearchCriteriaT searchCriteria,
                 ntfsv_filter_ptrs_t *f_rec,
                 MDS_SYNC_SND_CTXT *mdsCtxt);

  void readNext(unsigned int readerId,
                SaNtfSearchDirectionT searchDirection,
                MDS_SYNC_SND_CTXT *mdsCtxt);
  void deleteReader(unsigned int readerId, MDS_SYNC_SND_CTXT *mdsCtxt);

  void discardedAdd(SaNtfSubscriptionIdT subscriptionId, SaNtfIdentifierT notificationId);
  void discardedClear(SaNtfSubscriptionIdT subscriptionId);
  void printInfo();
  bool IsA11Client() const;
  void set_client_version(SaVersionT *ver);
  SaVersionT *getSafVersion();

 private:
  void newReaderResponse(SaAisErrorT* error,
                         unsigned int readerId,
                         MDS_SYNC_SND_CTXT *mdsCtxt);
  void readNextResponse(SaAisErrorT* error,
                        NtfSmartPtr& notification,
                        MDS_SYNC_SND_CTXT *mdsCtxt);
  void deleteReaderResponse(SaAisErrorT* error,
                            MDS_SYNC_SND_CTXT *mdsCtxt);
  void addDiscardedNotification(NtfSmartPtr notification);

  unsigned int clientId_;
  unsigned int readerId_;

  MDS_DEST mdsDest_;
  SaVersionT safVersion_;
  typedef std::map<SaNtfSubscriptionIdT,NtfSubscription*> SubscriptionMap;
  SubscriptionMap subscriptionMap;

  typedef std::map<unsigned int, NtfReader*> ReaderMapT;
  ReaderMapT readerMap;

};

#endif  // NTF_NTFD_NTFCLIENT_H_
