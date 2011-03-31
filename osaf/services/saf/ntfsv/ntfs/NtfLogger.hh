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
 *   NtfLogger uses the SAF Log API and reads from logfile
 */

#ifndef __NTF_LOGGER_HH
#define __NTF_LOGGER_HH


/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <list>
#include "saLog.h"

#include "NtfNotification.hh"
#include "NtfReader.hh"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */
#define NTF_LOG_CASH_SIZE 100

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
//class NtfReader;

class NtfLogger
{
    friend class NtfReader;
public:
    NtfLogger();
//    virtual ~NtfLogger();

    void log(NtfNotification& notif, bool isLocal);
    void checkQueueAndLog(NtfNotification& notif);
    SaAisErrorT logNotification(NtfNotification& notif);    
    void queueNotifcation(NtfNotification& notif);
    void printInfo();

private:
    SaAisErrorT initLog();

    readerNotificationListT coll_;
    unsigned int readCounter;
    bool first;
    typedef std::list<NtfNotification*> QueuedNotificationsList;
    QueuedNotificationsList queuedNotificationList;
};

#endif // NTF_LOGGER_HH

