/*      -- OpenSAF  --
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

#ifndef SMF_SMFD_SMFCBKUTIL_H_
#define SMF_SMFD_SMFCBKUTIL_H_

#include <semaphore.h>
#include "base/ncsgl_defs.h"
#include "base/ncssysf_ipc.h"
#include "smf/saf/saSmf.h"
#include <string>

#define SMF_UTIL_LABEL "OsafSmfCbkUtil-"

/* SmfCbkUtilThread event enums */
typedef enum {
	SMFCBKUTIL_EVT_TERMINATE = 1,
	SMFCBKUTIL_EVT_MAX
} SMFCBKUTIL_EVT_TYPE;

/* SmfCbkUtilThread event definitions */
typedef struct {
	uint32_t dummy;
} SmfCbkUtilThread_evt_terminate;

typedef struct {
	void *next;		/* needed by mailbox send/receive */
	SMFCBKUTIL_EVT_TYPE type;	/* evt type */
	union {
		SmfCbkUtilThread_evt_terminate terminate;
	} event;
} SMFCBKUTIL_EVT;

///
/// Class for the SMF callback utility thread. This is a singleton.
///

class SmfCbkUtilThread {
 public:
	static SmfCbkUtilThread *instance(void);

///
/// Purpose: Starts the SmfCbkUtilThread
/// @param   -
/// @return  An interger returning 0 on success.
///
	static int start();

///
/// Purpose: Terminate the SmfCbkUtilThread
/// @param   None.
/// @return  None.
///
	static void terminate(void);

///
/// Purpose: Send an event to the SmfCbkUtilThread
/// @param   evt A pointer to a SmfCbkUtilThread event.
/// @return  An interger returning 0 on success.
///
	int send(SMFCBKUTIL_EVT * evt);

 private:
	 SmfCbkUtilThread();
	~SmfCbkUtilThread();

	int startThread(void);
	int stop(void);
	void mainThread(void);
	int init(void);
	int initSmfCbkApi(void);
	int finalizeSmfCbkApi(void);
	int handleEvents(void);
	void processEvt(void);

	static void* main(NCSCONTEXT info);
	static SmfCbkUtilThread *s_instance;

	SYSF_MBX m_mbx;		/* mailbox */
	bool m_running;
	sem_t* m_semaphore;
	SaSmfHandleT m_smfHandle;
};

#endif  // SMF_SMFD_SMFCBKUTIL_H_
