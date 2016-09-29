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
 *   NtfAdmin handles all incoming requests to the NTF server.
 *   All mapping between the C and C++ is done via ntf_com.h.
 *
 */

 /* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include "NtfAdmin.hh"
#include <logtrace.h>
#include "ntfsv_mem.h"

NtfAdmin* NtfAdmin::theNtfAdmin = NULL;

/**
 * This is the constructor. The cluster-wide unique counter for
 * notifications and the local counter for the clients are
 * initialized.
 */
NtfAdmin::NtfAdmin()
{
    // initilalize variables
    notificationIdCounter = 0;
    clientIdCounter = 0;
}

NtfAdmin::~NtfAdmin()
{
}

/**
 * A new client object is created and a reference to the 
 * connection object is added. 
 *  
 * If the client has an id higher than the actual value of the
 * client counter, this means that this process was restarted 
 * and the client counter value needs to be updated. This could 
 * only occur during synchronization. 
 *
 * @param clientId A node-wide unique id for the client that was added. 
 * @param mdsDest
 * @param mdsCtxt
 */
void NtfAdmin::clientAdded(unsigned int clientId,
                           MDS_DEST mdsDest,
                           MDS_SYNC_SND_CTXT *mdsCtxt, SaVersionT *version)
{
    SaAisErrorT rc = SA_AIS_OK;

    if (clientId == 0) /* clientId == 0, means this is a new client */
    {                
        clientIdCounter++;
        if (clientIdCounter == 0)
            clientIdCounter++;
        clientId = clientIdCounter;
    }

    if (clientIdCounter < clientId)
    {
        // it can only occur during synchronization when we were restarted and
        // we do not know how many clients we had before
        clientIdCounter = clientId;
        TRACE_6("NtfAdmin::clientAdded clientIdCounter synchronized,"
                " new value is %u", clientIdCounter);
    }

    /*  The client object is deleted in NtfAdmin::clientRemoved if a client is removed */
    NtfClient* client = new NtfClient(clientId,
                                      mdsDest);
    client->set_client_version(version);
    // check if client already exists
    ClientMap::iterator pos;
    pos = clientMap.find(client->getClientId());
    if (pos != clientMap.end())
    {
        // client found
        TRACE_1("NtfAdmin::clientAdded client %u already exists",
                client->getClientId());
        delete client;
        rc = SA_AIS_ERR_EXIST;
    }
    else
    {
        // store new client in clientMap
        clientMap[client->getClientId()] = client;
        TRACE_1("NtfAdmin::clientAdded client %u added, clientMap size is %u",
                client->getClientId(), (unsigned int)clientMap.size());

    }
    if (NULL != mdsCtxt)
    {
        client_added_res_lib(rc,
                             clientId,
                             mdsDest,
                             mdsCtxt,
                             version);
    }
}

/**
 * This method creates a subscription object and pass it to the
 * client object associated with the clientId. 
 *
 * @param ntfsv_subscribe_req_t contains subscribtions and 
 *                              filter information
 */
void NtfAdmin::subscriptionAdded(ntfsv_subscribe_req_t s, MDS_SYNC_SND_CTXT *mdsCtxt)
{
        // create subscription
        // deleted in different methods depending on if object exists or not:
        // NtfAdmin::subscriptionAdded
        // NtfClient::subscriptionAdded
        // NtfClient::subscriptionRemoved
        // NtfClient::~NtfClient()
    NtfSubscription* subscription = new NtfSubscription(&s);
        // find client
    ClientMap::iterator pos;
    pos = clientMap.find(s.client_id);
    if (pos != clientMap.end())
    {
            // client found
        NtfClient* client = pos->second;
        client->subscriptionAdded(subscription, mdsCtxt);
    }
    else
    {
        LOG_ER("NtfAdmin::subscriptionAdded client %u not found", s.client_id);
        delete subscription;
    }
}

/**
 * Update the notificationIdCounter which is the unique id 
 * assiged to each new notification. 
 *
 * @param notificationId 
 */
void NtfAdmin::updateNotIdCounter(SaNtfIdentifierT notificationId)
{
    TRACE_ENTER();
    if (notificationId == 0)
    {
        // it is a new notification, assign a new notificationId to it
        notificationIdCounter++;
    }
    else
    {
        /* update or coldsync */
        TRACE_2("old notification received, id is %llu", notificationId);
        if (notificationIdCounter < notificationId && !activeController())
        {
            TRACE_2("update notificationIdCounter %llu -> %llu",
                    notificationIdCounter,
                    notificationId);
            notificationIdCounter = notificationId;
        }
    }
    TRACE_LEAVE();
}

/**
 * Creates a new notification object. Then the notification is
 * sent to all client who is interested in receiving this 
 * notification. If no client is interested in this 
 * notification, the notification object is deleted. 
 *  
 * @param  clientId
 * @param  notificationType
 * @param  sendNotInfo struct holding all notification information 
 * @param  mdsCtxt 
 * @param  notificationId 
*/
void NtfAdmin::processNotification(unsigned int clientId,
                                   SaNtfNotificationTypeT notificationType,
                                   ntfsv_send_not_req_t* sendNotInfo,
                                   MDS_SYNC_SND_CTXT *mdsCtxt,
                                   SaNtfIdentifierT notificationId)
{
    TRACE_ENTER();
    // The notification can be deleted in these methods also:
    // NtfAdmin::notificationSentConfirmed
    // NtfAdmin::clientRemoved
    // NtfAdmin::subscriptionRemoved
    NtfSmartPtr notification(new NtfNotification(notificationId,
                                                 notificationType,
                                                 sendNotInfo));
    // store notification in a map for tracking purposes
    notificationMap[notificationId] = notification;
    TRACE_2("notification %llu with type %d added, notificationMap size is %u",
            notificationId,
            notificationType,
            (unsigned int)notificationMap.size());

    // log the notification. Callback from SAF log will confirm later.
    logger.log(notification, activeController());

    /* send notification to standby */
    sendNotificationUpdate(clientId, notification->getNotInfo());

    ClientMap::iterator pos;
    for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
    {
        NtfClient* client = pos->second;
        client->notificationReceived(clientId, notification, mdsCtxt);
    }

    /* remove notification if sent to all subscribers and logged */
    if (notification->isSubscriptionListEmpty() &&
        notification->loggedOk())
    {
        NotificationMap::iterator posNot;
        posNot = notificationMap.find(notificationId);
        if (posNot != notificationMap.end())
        {
            TRACE_2("NtfAdmin::notificationReceived no subscription found"
                    " for notification %llu", notificationId);
            notificationMap.erase(notificationMap.find(notificationId));
            TRACE_2("NtfAdmin::notificationReceived notification %llu removed,"
                    " notificationMap size is %u",
                    notificationId, (unsigned int)notificationMap.size());
        }
    }
    TRACE_LEAVE();
}

/**
 * Call methods to update notificationIdCounter and process 
 * the notification. 
 *  
 * @param clientId Cluster-wide unique id for the client who 
 *                 sent the notifiaction.
 * @param notificationType Type of the notification (alarm,
 *                         object change etc.).
 * @param sendNotInfo Pointer to the struct that holds 
 *                    information about the notification.
 * @param mdsCtxt mds context for sending response to the 
 *                client.
 */
void NtfAdmin::notificationReceived(unsigned int clientId,
                                    SaNtfNotificationTypeT notificationType,
                                    ntfsv_send_not_req_t* sendNotInfo,
                                    MDS_SYNC_SND_CTXT *mdsCtxt)
{
    TRACE_ENTER();
    SaNtfNotificationHeaderT *header;
    ntfsv_get_ntf_header(sendNotInfo, &header);
    updateNotIdCounter(*header->notificationId);
    TRACE_2("New notification received, id: %llu", notificationIdCounter);
    processNotification(clientId,
                        notificationType,
                        sendNotInfo,
                        mdsCtxt,
                        notificationIdCounter);
    TRACE_LEAVE();
}

/**
 * Check if the notification already exist. If it doesn't exist
 * call methods to update notificationIdCounter and process the
 * notification. 
 *  
 * @param clientId Cluster-wide unique id for the client who 
 *                 sent the notifiaction.
 * @param notificationType Type of the notification (alarm,
 *                         object change etc.).
 * @param sendNotInfo Pointer to the struct that holds 
 *                    information about the notification.
 */
void NtfAdmin::notificationReceivedUpdate(unsigned int clientId,
                                          SaNtfNotificationTypeT notificationType,
                                          ntfsv_send_not_req_t* sendNotInfo)
{
    TRACE_ENTER();
    SaNtfNotificationHeaderT *header;
    ntfsv_get_ntf_header(sendNotInfo, &header);
    updateNotIdCounter(*header->notificationId);
    SaNtfIdentifierT notificationId = *header->notificationId;
    // check if notification object already exists
    NotificationMap::iterator posNot;
    posNot = notificationMap.find(notificationId);
    if (posNot != notificationMap.end())
    {
        // we have got the notification
        TRACE_2("notification %u received"
                " again, skipped", (unsigned int)notificationId);
        ntfsv_dealloc_notification(sendNotInfo);
        delete sendNotInfo;
    }
    else
    {
        TRACE_2("notification %u",
                (unsigned int)notificationId);
        processNotification(clientId,
                            notificationType,
                            sendNotInfo,
                            NULL,
                            notificationId);  
    }
    TRACE_LEAVE();
}

/**
 * A new notification object is created if it doesn't already 
 * exist. 
 *  
 * @param clientId Node-wide unique id for the client who sent the notifiaction.
 * @param notificationType
 *                 Type of the notification (alarm, object change etc.).
 * @param sendNotInfo
 *                 Pointer to the struct that holds information about the
 *                 notification.
 */
void NtfAdmin::notificationReceivedColdSync(unsigned int clientId,
                                            SaNtfNotificationTypeT notificationType,
                                            ntfsv_send_not_req_t* sendNotInfo)
{
    TRACE_ENTER();
    SaNtfNotificationHeaderT *header;
    ntfsv_get_ntf_header(sendNotInfo, &header);
    updateNotIdCounter(*header->notificationId);
    SaNtfIdentifierT notificationId = *header->notificationId;
    // check if notification object already exists
    NotificationMap::iterator posNot;
    posNot = notificationMap.find(notificationId);
    if (posNot != notificationMap.end())
    {
        // we have got the notification
        TRACE_2("notification %u received"
                " again, skipped", (unsigned int)notificationId);
        ntfsv_dealloc_notification(sendNotInfo);
        delete sendNotInfo;				
    }
    else
    {
        TRACE_2("notification %u received for"
                " the first time", (unsigned int)notificationId);
        // The notification can be deleted in these methods also:
        // NtfAdmin::notificationSentConfirmed
        // NtfAdmin::clientRemoved
        // NtfAdmin::subscriptionRemoved
        NtfSmartPtr notification(new NtfNotification(notificationId,
                                                     notificationType,
                                                     sendNotInfo));
        // store notification in a map for tracking purposes
        notificationMap[notificationId] = notification;
        TRACE_2("notification %llu with type %d"
                " added, notificationMap size is %u",
                notificationId, notificationType,
                (unsigned int)notificationMap.size());
    }
    TRACE_LEAVE();
}

/**
 * The notification object is notified that a notification has 
 * been sent to a subscriber, so it can remove the referenced 
 * subscription from its list. If the subscription list in the 
 * notification object is empty (i.e. the notification was 
 * delivered to all matching subscriptions), the notification 
 * object is deleted. 
 *
 * @param clientId Node-wide unique id of the client who received the
 * notification.
 * @param subscriptionId
 *                 Client-wide unique id for the subscription that matched the
 *                 delivered notification.
 * @param notificationId
 *                 Cluster-wide unique id of the notification that was
 *                 delivered.
 */
void NtfAdmin::notificationSentConfirmed(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId,
    SaNtfIdentifierT notificationId, int discarded)
{

    // find notification
    NotificationMap::iterator pos;
    pos = notificationMap.find(notificationId);
    if (pos != notificationMap.end())
    {
        // we have got the notification
        NtfSmartPtr notification = pos->second;
        notification->notificationSentConfirmed(clientId,
                                                subscriptionId);
        if (activeController())
        {
            /* no delete if active */
            sendNotConfirmUpdate(clientId, subscriptionId, notificationId, discarded);
        }
        else
        {
            deleteConfirmedNotification(notification, pos);
        }
    }
    else
    {
        LOG_WA("NtfAdmin::notificationSentConfirmed"
               " notification %llu not found",
               notificationId);
    }
}

/**
 * Confirm that a notification has been logged. Call method to 
 * delete the notification if it has been sent to all 
 * subscribers. 
 *  
 * @param notificationId
 */
void NtfAdmin::notificationLoggedConfirmed(SaNtfIdentifierT notificationId)
{
    TRACE_ENTER();
    // find notification
    NotificationMap::iterator pos;
    pos = notificationMap.find(notificationId);

    if (pos != notificationMap.end())
    {
        NtfSmartPtr notification = pos->second;
        notification->notificationLoggedConfirmed();
        if (activeController())
        {
            if (notification->loggFromCallback_)
            {
                sendLoggedConfirmUpdate(notificationId);
            }
        }
        deleteConfirmedNotification(notification, pos);
    }
    else
    {
        /* TODO: This could happend for not logged notification */
        TRACE_2("NtfAdmin::notificationLoggedConfirmed"
                " notification %llu not found",
                notificationId);
    }
    TRACE_LEAVE();
}

/**
 * Remove the client object corresponding to the clientId. 
 * Call all notification objects to remove subscriptions 
 * belonging to that client. Call method to eventually remove 
 * the notification object. 
 *
 * @param clientId Node-wide unique id for the removed client.
 */
void NtfAdmin::clientRemoved(unsigned int clientId)
{
    // find client
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        // client found
        NtfClient* client = pos->second;
        delete client;
        // remove client from client map
        clientMap.erase(pos);
    }
    else
    {
        TRACE_2("NtfAdmin::clientRemoved client %u not found", clientId);
        return;
    }
    // notifications do not need to be sent to that client, remove them
    // scan through all notifications, remove subscriptions belonging to
    //  the removed client
    NotificationMap::iterator posN = notificationMap.begin();
    while (posN != notificationMap.end())
    {
        NtfSmartPtr notification = posN->second;
        notification->removeSubscription(clientId);
        deleteConfirmedNotification(notification, posN++);
    }
}


/**
 * Find the clientIds corresponding to the mds_dest. Call method
 * to remove clients belonging to that mds_dest. 
 *
 * @param mds_dest 
 */
void NtfAdmin::clientRemoveMDS(MDS_DEST mds_dest)
{
	TRACE_ENTER2("REMOVE mdsDest: %" PRIu64, mds_dest);
	ClientMap::iterator pos;
	bool found = false;
	do {
		found = false;		
		for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
		{
			NtfClient* client = pos->second;
			if (client->getMdsDest() == mds_dest)
			{
				clientRemoved(client->getClientId());
				found = true;
				/* avoid continue iterate after erase */
				break;
			}
		}
	} while (found);
	TRACE_LEAVE();
}

/**
 * The node object where the client who had the subscription is notified
 * so it can delete the appropriate subscription and filter object.
 *
 * Then the notification objects are notified about it so the
 * subscription can be deleted from their matching subscription list.
 * If the subscription list of a notification becomes empty (i.e. only
 * the removed subscription matched that notification), the notification
 * object is deleted.
 *
 * @param clientId Node-wide unique id for the client who had the subscription.
 * @param subscriptionId
 *                 Client-wide unique id for the removed subscription.
 */
void NtfAdmin::subscriptionRemoved(unsigned int clientId,
                                   SaNtfSubscriptionIdT subscriptionId,
                                   MDS_SYNC_SND_CTXT *mdsCtxt)
{
    // find client
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        // client found
        NtfClient* client = pos->second;
        client->subscriptionRemoved(subscriptionId, mdsCtxt);
    }
    else
    {
        LOG_WA("NtfAdmin::subscriptionRemoved client %u not found", clientId);
    }

    // notifications do not need to be sent on that subscription, remove
    //  them scan through all notifications, remove subscriptions
    NotificationMap::iterator posN = notificationMap.begin();
    while (posN != notificationMap.end())
    {
        NtfSmartPtr notification = posN->second;
        notification->removeSubscription(clientId,
                                         subscriptionId);
        deleteConfirmedNotification(notification, posN++);
    }
}

void NtfAdmin::discardedAdd(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId, SaNtfIdentifierT notificationId)
{
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        NtfClient* client = pos->second;
        client->discardedAdd(subscriptionId, notificationId);
    }
    else
    {
        LOG_ER("client %u not found", clientId);
    }
}

void NtfAdmin::discardedClear(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId)
{
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        NtfClient* client = pos->second;
        client->discardedClear(subscriptionId);
    }
    else
    {
        LOG_ER("client %u not found", clientId);
    }
}

/**
 *  Encode the whole data structure. 
 *  
 * @param uba buffer context to store data
 */
void NtfAdmin::syncRequest(NCS_UBAID *uba)
{
    TRACE_ENTER();
    sendNoOfClients(clientMap.size(),uba);
    // send syncRequest to all clients
    ClientMap::iterator pos;
    for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
    {
        NtfClient* client = pos->second;
        TRACE_1("NtfAdmin::syncRequest sending info about client %u",
                client->getClientId());
        int retval = sendNewClient(client->getClientId(),
                                   client->getMdsDest(),
                                   client->getSafVersion(),
                                   uba);
        if (retval != 1)
        {
            LOG_ER("sendNewClient: %u failed",
                   client->getClientId());
        }
    }
    for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
    {
        pos->second->syncRequest(uba); /* subscriptions are synched here */
    }

    // send information about global variables like notificationIdCounter
    TRACE_6("NtfAdmin::syncRequest sending info global variables:"
            " notificationIdCounter %llu", notificationIdCounter);
    struct NtfGlobals ntfGlobals;
    memset(&ntfGlobals, 0, sizeof(struct NtfGlobals));
    ntfGlobals.notificationId = notificationIdCounter;
    ntfGlobals.clientIdCounter = clientIdCounter;
    int retval = sendSyncGlobals(&ntfGlobals, uba);
    if (retval != 1)
    {
        LOG_ER("NtfAdmin::syncRequest sendSyncGlobals"
               " request was not sent, "
               "error code is %d", retval);
    }

    // send notifications 
    TRACE_2("sendNoOfNotifications mapsize=%u", (unsigned int)notificationMap.size());
    sendNoOfNotifications(notificationMap.size(),uba);
    NotificationMap::iterator posNot;
    for (posNot = notificationMap.begin();
        posNot != notificationMap.end();
        posNot++)
    {
        NtfSmartPtr notification = posNot->second;
        notification->syncRequest( uba);
    }
    TRACE_LEAVE();
}

/**
 * Set global variables in this NtfAdmin object.
 *
 * @param ntfGlobals Structure for global variables.
 */
void NtfAdmin::syncGlobals(const struct NtfGlobals& ntfGlobals)
{

    TRACE_6("NtfAdmin::syncGlobals setting notificationIdCounter to %llu",
            ntfGlobals.notificationId);
    notificationIdCounter = ntfGlobals.notificationId;
    clientIdCounter = ntfGlobals.clientIdCounter;
}

void NtfAdmin::deleteConfirmedNotification(NtfSmartPtr notification,
                                           NotificationMap::iterator pos)
{
    TRACE_ENTER();
    if (notification->isSubscriptionListEmpty() &&
        notification->loggedOk())
    {
        // notification sent to all nodes in the cluster, it can be deleted
        notificationMap.erase(pos);
        TRACE_2("Notification %llu removed, notificationMap size is %u",
                notification->getNotificationId(),
                (unsigned int)notificationMap.size());
    }
    TRACE_LEAVE();
}

/**
 * Log notifications that has not been confirmed as logged. Send
 * notifications to subscribers left in the subscriptionlist.
 */
void NtfAdmin::checkNotificationList()
{
    TRACE_ENTER();
    NotificationMap::iterator posNot;
    for (posNot = notificationMap.begin();
        posNot != notificationMap.end();
        posNot++)
    {
        NtfSmartPtr notification = posNot->second;
        if (notification->loggedOk() == false)
        {
            /* When reader API works check if already logged */
            logger.log(notification, true);
        }
        if (!notification->isSubscriptionListEmpty())
        {
            TRACE_2("list not empty check subscriptions for %llu",
                    notification->getNotificationId());
            notification->resetSubscriptionIdList();
            UniqueSubscriptionId uSubId;
            while (notification->getNextSubscription(uSubId) == SA_AIS_OK)
            {
                NtfClient* client = getClient(uSubId.clientId);
                if (NULL != client)
                {
                    client->sendNotConfirmedNotification(notification, uSubId.subscriptionId);
                }
                else
                {
                    TRACE_2("Client: %u not exist", uSubId.clientId);
                }
            }
            deleteConfirmedNotification(notification, posNot);
        }
    }
    TRACE_LEAVE();
}

/**
 * Check if a certain client exists.
 *
 * @param clientId Node-wide unique id of the client whose existence is to be checked.
 *
 * @return true if the client exists
 *         false if the client does not exist
 */
NtfClient* NtfAdmin::getClient(unsigned int clientId)
{
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        return pos->second;
    }
    else
    {
        return NULL;
    }
}

/**
 * Find the client and call method newReader
 *
 * @param clientId 
 * @param searchCriteria  
 * @param f_rec - filter record
 * @param mdsCtxt 
 */
void NtfAdmin::newReader(unsigned int clientId,
	SaNtfSearchCriteriaT searchCriteria,  
	ntfsv_filter_ptrs_t *f_rec,
	MDS_SYNC_SND_CTXT *mdsCtxt)
{
    TRACE_ENTER();
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        // we have got the client
        NtfClient* client = pos->second;
		  client->newReader(searchCriteria, f_rec, mdsCtxt);
    }
    else
    {
        // client object does not exist
        LOG_WA("NtfAdmin::newReader  client not found %u",
               clientId);
    }
    TRACE_LEAVE();
}

/**
 * Find the client and call method readNext
 *
 * @param clientId 
 * @param readerId unique readerId for this client
 * @param mdsCtxt 
 */
void NtfAdmin::readNext(unsigned int clientId,
                        unsigned int readerId,
                        SaNtfSearchDirectionT searchDirection,
                        MDS_SYNC_SND_CTXT *mdsCtxt)
{
    TRACE_ENTER();
    TRACE_6("clientId %u, readerId %u", clientId, readerId);
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        // we have got the client
        NtfClient* client = pos->second;
        client->readNext(readerId, searchDirection, mdsCtxt);
    }
    else
    {
        // client object does not exist
        LOG_WA(
              "NtfAdmin::readNext  client not found %u",
              clientId);
    }
    TRACE_LEAVE();
}

/**
 * Find the client and call method deleteReader
 *
 * @param clientId 
 * @param readerId unique readerId for this client
 * @param mdsCtxt 
 */
void NtfAdmin::deleteReader(unsigned int clientId,
                            unsigned int readerId,
                            MDS_SYNC_SND_CTXT *mdsCtxt)
{
    TRACE_ENTER();
    ClientMap::iterator pos;
    pos = clientMap.find(clientId);
    if (pos != clientMap.end())
    {
        // we have got the client
        NtfClient* client = pos->second;
        client->deleteReader(readerId, mdsCtxt);
    }
    else
    {
        // client object does not exist
        LOG_WA("NtfAdmin::deleteReader  client not found %u",
               clientId);
    }
    TRACE_LEAVE();
}

/**
 * Find the notification from the notificationId return the 
 * NtfNotification object. 
 *
 * @param clientId 
 * @param readerId unique readerId for this client
 * @param mdsCtxt 
 * @return pointer to NtfNotification object 
 */
NtfSmartPtr NtfAdmin::getNotificationById(SaNtfIdentifierT id)
{
    NotificationMap::iterator posNot;

    TRACE_ENTER();


    for (posNot = notificationMap.begin();
        posNot != notificationMap.end();
        posNot++)
    {
        NtfSmartPtr notification = posNot->second;

        if (notification->getNotificationId() == id)
        {
            break;
        }
    }

    if (posNot == notificationMap.end())
    {
        NtfSmartPtr tmp;
        return tmp;
    }

    return(posNot->second);
    TRACE_LEAVE();
}

/** 
 * Print information about this object and call printInfo method for all client 
 * objects and all notification objects. TRACE debug has to be on to get output. 
 * It will describe the data structure of this node. 
 */
void NtfAdmin::printInfo()
{
    TRACE("Admin information");
    TRACE("  notificationIdCounter:    %llu", notificationIdCounter);
    TRACE("  clientIdCounter:    %u", clientIdCounter);
    logger.printInfo();

    ClientMap::iterator pos;
    for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
    {
        NtfClient* client = pos->second;
        client->printInfo();
    }

    NotificationMap::iterator posNot;
    for (posNot = notificationMap.begin();
        posNot != notificationMap.end();
        posNot++)
    {
        NtfSmartPtr notification = posNot->second;
        notification->printInfo();
    }
}

/**
 * Add subscription to a specific notification. 
 *
 * @param notificationId
 * @param clientId
 * @param subscriptionId 
 */
void NtfAdmin::storeMatchingSubscription(SaNtfIdentifierT notificationId,
                                         unsigned int clientId,
                                         SaNtfSubscriptionIdT subscriptionId)
{
    NtfSmartPtr notification = getNotificationById(notificationId);
    if (NULL != notification.get())
    {
        notification->storeMatchingSubscription(clientId, subscriptionId);
    }
    else
    {
        TRACE_2("Notification: %llu does not exist", notificationId);
    }
}
/**
 * @brief Adds NCS node to list of memeber nodes. 
 *
 * @param node_id 
 */
void NtfAdmin::AddMemberNode(NODE_ID node_id) 
{
	NODE_ID *node = new NODE_ID;
	*node = node_id;
	member_node_list.push_back(node);
}

/**
 * @brief Finds a NCS node in the list of memeber nodes.
 *
 * @param node_id
 * @return pointer to the node.
 */
NODE_ID* NtfAdmin::FindMemberNode(NODE_ID node_id) 
{
	std::list<NODE_ID*>::iterator it;
	for (it = member_node_list.begin(); it != member_node_list.end(); ++it) {
		NODE_ID *node = *it;
		if (*node == node_id)
			return node; 
	}
	return nullptr;
}

/**
 * @brief Removes a NCS node from the list of memeber nodes.
 *
 * @param node_id
 */
void NtfAdmin::RemoveMemberNode(NODE_ID node_id) 
{
	std::list<NODE_ID*>::iterator it;
	for (it = member_node_list.begin(); it != member_node_list.end(); ++it) {
		NODE_ID *node = *it;
		if (*node == node_id) {
			member_node_list.erase(it);
			TRACE_2("Deleted:%x",*node);
			delete node;
			return;
		}
	}
}

/**
 * @brief Count no. of member nodes.
 *
 * @return member node counts.
 */
uint32_t NtfAdmin::MemberNodeListSize()
{
	return member_node_list.size();
}

/**
 * @brief Print node_ids of member nodes.
 */
void NtfAdmin::PrintMemberNodes()
{
        std::list<NODE_ID*>::iterator it;
        for (it = member_node_list.begin(); it != member_node_list.end(); ++it) {
                NODE_ID *node = *it;
		TRACE_1("NODE_ID:%x",*node);
        }
}

/**
 * @brief  Sends CLM membership status of the node to all the clients
 *         on the node except A11 clients.
 * @param  cluster_change (CLM membership status of node).
 * @param  NCS node_id. 
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
uint32_t NtfAdmin::send_cluster_membership_msg_to_clients(SaClmClusterChangesT cluster_change, NODE_ID node_id)
{
	uint32_t rc = NCSCC_RC_SUCCESS;
	unsigned int client_id;
	MDS_DEST mds_dest;

        TRACE_ENTER();
        TRACE_3("node_id: %x, change:%u", node_id, cluster_change);
	ClientMap::iterator pos;
	for (pos = clientMap.begin(); pos != clientMap.end(); pos++)
	{
		NtfClient* client = pos->second;
		client_id = client->getClientId();
		mds_dest = client->getMdsDest();
		NODE_ID tmp_node_id = m_NTFS_GET_NODE_ID_FROM_ADEST(mds_dest);
		//Do not send to A11 client.
		if ((tmp_node_id == node_id) && (client->IsA11Client() == false))
			rc = send_clm_node_status_lib(cluster_change, client_id, mds_dest);	
	}
	TRACE_LEAVE();
        return rc; 
}

/**
 * @brief  Checks CLM membership status of a client using clientId.
 * @param  clientId MDS_DEST
 * @return true/false.
 */
bool NtfAdmin::is_stale_client(unsigned int client_id) 
{
    //Find client.
    ClientMap::iterator pos;
    pos = clientMap.find(client_id);
    if (pos != clientMap.end())
    {
	MDS_DEST mds_dest;
        // Client found
        NtfClient* client = pos->second;
	mds_dest = client->getMdsDest();
	return (!is_client_clm_member(m_NTFS_GET_NODE_ID_FROM_ADEST(mds_dest),
				client->getSafVersion()));
    } else
	//Client not found.
        return false;
}

// C wrapper funcions start here

/** 
 * Create the singelton NtfAdmin object.
 */
void initAdmin()
{
    // NtfAdmin is never deleted only one instance exists
    if (NtfAdmin::theNtfAdmin == NULL)
    {
        NtfAdmin::theNtfAdmin = new NtfAdmin();
    }
}

void clientAdded(unsigned int clientId,
                 MDS_DEST mdsDest,
                 MDS_SYNC_SND_CTXT *mdsCtxt, SaVersionT *version)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->clientAdded(clientId, mdsDest, mdsCtxt, version);
}

void subscriptionAdded(ntfsv_subscribe_req_t s, MDS_SYNC_SND_CTXT *mdsCtxt)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->subscriptionAdded(s, mdsCtxt);
}

void notificationReceived(unsigned int clientId,
                          SaNtfNotificationTypeT notificationType,
                          ntfsv_send_not_req_t* sendNotInfo,
                          MDS_SYNC_SND_CTXT *mdsCtxt)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->notificationReceived(clientId,
                                                notificationType,
                                                sendNotInfo,
                                                mdsCtxt);
}
void notificationReceivedUpdate(unsigned int clientId,
                                SaNtfNotificationTypeT notificationType,
                                ntfsv_send_not_req_t* sendNotInfo)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->notificationReceivedUpdate(clientId,
                                                      notificationType,
                                                      sendNotInfo);
}

void notificationReceivedColdSync(unsigned int clientId,
                                  SaNtfNotificationTypeT notificationType,
                                  ntfsv_send_not_req_t* sendNotInfo)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->notificationReceivedColdSync(clientId,
                                                        notificationType,
                                                        sendNotInfo);
}

void notificationSentConfirmed(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId,
    SaNtfIdentifierT notificationId, int discarded)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->notificationSentConfirmed(clientId, subscriptionId, notificationId, discarded);
}

void notificationLoggedConfirmed(SaNtfIdentifierT notificationId)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->notificationLoggedConfirmed(notificationId);
}

void clientRemoved(unsigned int clientId)
{

    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->clientRemoved(clientId);
}
void clientRemoveMDS(MDS_DEST mds_dest)
{

    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->clientRemoveMDS(mds_dest);
}

void subscriptionRemoved(
                        unsigned int clientId,
                        SaNtfSubscriptionIdT subscriptionId,
                        MDS_SYNC_SND_CTXT *mdsCtxt)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->subscriptionRemoved(
                                              clientId,
                                              subscriptionId,
                                              mdsCtxt);
}

void syncRequest( NCS_UBAID *uba)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->syncRequest(uba);
}

void syncGlobals(const struct NtfGlobals *ntfGlobals)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->syncGlobals(*ntfGlobals);
}

void newReader(unsigned int clientId,
	SaNtfSearchCriteriaT searchCriteria,
	ntfsv_filter_ptrs_t *f_rec,
	MDS_SYNC_SND_CTXT *mdsCtxt)
{
	osafassert(NtfAdmin::theNtfAdmin != NULL);
	NtfAdmin::theNtfAdmin->newReader(clientId, searchCriteria, f_rec, mdsCtxt);	
}

void readNext(unsigned int clientId,
              unsigned int readerId,
              SaNtfSearchDirectionT searchDirection,
              MDS_SYNC_SND_CTXT *mdsCtxt)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    return NtfAdmin::theNtfAdmin->readNext(clientId,
                                           readerId,
                                           searchDirection,
                                           mdsCtxt);
}

void deleteReader(unsigned int clientId,
                  unsigned int readerId,
                  MDS_SYNC_SND_CTXT *mdsCtxt)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    return NtfAdmin::theNtfAdmin->deleteReader(clientId,
                                               readerId,
                                               mdsCtxt);
}

void printAdminInfo ()
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->printInfo();
}

void storeMatchingSubscription(SaNtfIdentifierT notificationId,
                               unsigned int clientId,
                               SaNtfSubscriptionIdT subscriptionId)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    NtfAdmin::theNtfAdmin->storeMatchingSubscription(notificationId,
                                                     clientId,
                                                     subscriptionId);
}

void checkNotificationList()
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    return NtfAdmin::theNtfAdmin->checkNotificationList();
}

void discardedAdd(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId, SaNtfIdentifierT notificationId)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    return NtfAdmin::theNtfAdmin->discardedAdd(clientId, subscriptionId, notificationId);    
}

void discardedClear(unsigned int clientId, SaNtfSubscriptionIdT subscriptionId)
{
    osafassert(NtfAdmin::theNtfAdmin != NULL);
    return NtfAdmin::theNtfAdmin->discardedClear(clientId, subscriptionId);    
}

/************************C Wrappers related to CLM Integration **************************/
void add_member_node(NODE_ID node_id)
{
	NtfAdmin::theNtfAdmin->AddMemberNode(node_id);
	return;
}

NODE_ID *find_member_node(NODE_ID node_id)
{
	return NtfAdmin::theNtfAdmin->FindMemberNode(node_id);
}

void remove_member_node(NODE_ID node_id)
{
	NtfAdmin::theNtfAdmin->RemoveMemberNode(node_id);
	return;
}

uint32_t count_member_nodes()
{
	return NtfAdmin::theNtfAdmin->MemberNodeListSize();
}

void print_member_nodes()
{
	NtfAdmin::theNtfAdmin->PrintMemberNodes();
	return;
}

bool is_stale_client(unsigned int clientId)
{
	return (NtfAdmin::theNtfAdmin->is_stale_client(clientId));
}

uint32_t send_clm_node_status_change(SaClmClusterChangesT cluster_change, NODE_ID node_id)
{
        return (NtfAdmin::theNtfAdmin->send_cluster_membership_msg_to_clients(cluster_change, node_id));

}

/**
 * @brief  Checks CLM membership status of a client.
 *         A0101 clients are always CLM member.
 * @param  Client MDS_DEST 
 * @param  Client saf version.
 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE.
 */
bool is_client_clm_member(NODE_ID node_id, SaVersionT *ver)
{
	NODE_ID *node = NULL;

	//Before CLM init all clients are clm member.
	if (is_clm_init() == false)
		return true;
	//CLM integration is supported from A.01.02. So old clients A.01.01 are always clm member.
        if ((ver->releaseCode == NTF_RELEASE_CODE_0) &&
                (ver->majorVersion == NTF_MAJOR_VERSION_0) &&
                (ver->minorVersion == NTF_MINOR_VERSION_0))
                return true;
	/*
	   It means CLM initialization is successful and this is atleast a A.01.02 client.
	   So check CLM membership status of client's node.
	 */
        if ((node = NtfAdmin::theNtfAdmin->FindMemberNode(node_id)) == NULL)
                return false;
        else
                return true;
}

