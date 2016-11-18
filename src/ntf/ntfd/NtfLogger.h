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

#ifndef NTF_NTFD_NTFLOGGER_H_
#define NTF_NTFD_NTFLOGGER_H_


/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <list>
#include "log/saf/saLog.h"

#include "ntf/ntfd/NtfNotification.h"
#include "ntf/ntfd/NtfReader.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

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

  void log(NtfSmartPtr& notif, bool isLocal);
  void checkQueueAndLog(NtfSmartPtr& notif);
  SaAisErrorT logNotification(NtfSmartPtr& notif);
  void queueNotifcation(NtfSmartPtr& notif);
  void printInfo();

 private:
  SaAisErrorT initLog();

  readerNotificationListT coll_;
  unsigned int readCounter;
  typedef std::list<NtfSmartPtr> QueuedNotificationsList;
  QueuedNotificationsList queuedNotificationList;
};

#endif  // NTF_NTFD_NTFLOGGER_H_

