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

#ifndef _SmfProcedureThread_hh_
#define _SmfProcedureThread_hh_

#include <semaphore.h>
#include <ncsgl_defs.h>
#include <ncssysf_ipc.h>
#include <string>

#include <saAis.h>
#include <saImmOi.h>

/**** Macro for PROCEDURE task name ****/
#define m_PROCEDURE_TASKNAME "PROCEDURE"

/**** Macro for PROCEDURE task stack size ****/
#define m_PROCEDURE_STACKSIZE NCS_STACKSIZE_HUGE

/* SmfProcedureThread event enums */
typedef enum {
	PROCEDURE_EVT_TERMINATE = 1,
	PROCEDURE_EVT_EXECUTE = 2,
	PROCEDURE_EVT_EXECUTE_STEP = 4,
	PROCEDURE_EVT_ROLLBACK_STEP = 5,
	PROCEDURE_EVT_SUSPEND = 6,
	PROCEDURE_EVT_COMMIT = 7,
	PROCEDURE_EVT_ROLLBACK = 8,
	PROCEDURE_EVT_MAX
} PROCEDURE_EVT_TYPE;

/*** PROCEDURE event definitions ***/
typedef struct {
	uint32_t dummy;
} procedure_evt_terminate;

typedef struct {
	uint32_t dummy;
} procedure_evt_execute;

typedef struct {
	uint32_t dummy;
} procedure_evt_execute_init;

typedef struct {
	uint32_t dummy;
} procedure_evt_execute_step;

typedef struct {
	uint32_t dummy;
} procedure_evt_execute_wrapup;

typedef struct {
	uint32_t dummy;
} procedure_evt_suspend;

typedef struct {
	uint32_t dummy;
} procedure_evt_commit;

typedef struct {
	uint32_t dummy;
} procedure_evt_rollback;

typedef struct {
	void *next;		/* needed by mailbox send/receive */
	PROCEDURE_EVT_TYPE type;	/* evt type */
	union {
		procedure_evt_terminate terminate;
		procedure_evt_execute execute;
		procedure_evt_execute_init executeInit;
		procedure_evt_execute_step executeStep;
		procedure_evt_execute_wrapup executeWrapup;
		procedure_evt_suspend suspend;
		procedure_evt_commit commit;
		procedure_evt_rollback rollback;
	} event;
} PROCEDURE_EVT;

class SmfUpgradeProcedure;

/*********************************************
  Class SmfProcedureThread
**********************************************/

class SmfProcedureThread {
 public:
	SmfProcedureThread(SmfUpgradeProcedure * procedure);
	~SmfProcedureThread();

	int start(void);
	int stop(void);

	int updateImmAttr(const char *dn, SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType, void *value);

	int send(PROCEDURE_EVT * evt);

	SaImmOiHandleT getImmHandle();

	SYSF_MBX & getCbkMbx();


 private:

	void main(void);
	int init(void);
	int handleEvents(void);
	void processEvt(void);

	SaAisErrorT getImmProcedure(SmfUpgradeProcedure * procedure);
	SaAisErrorT createImmProcedure(SmfUpgradeProcedure * procedure);
	SaAisErrorT createImmHandle(void);
	SaAisErrorT deleteImmHandle(void);

        static void main(NCSCONTEXT info);

	NCSCONTEXT m_task_hdl;
	SYSF_MBX m_mbx;		/* mailbox */
	SYSF_MBX m_cbk_mbx;	/* mailbox to send/receive callback/response */
	bool m_running;
	SmfUpgradeProcedure *m_procedure;
	SaImmOiHandleT m_procOiHandle;	/* IMM OI handle */
	bool m_useCampaignOiHandle;
	sem_t* m_semaphore;
};

#endif
