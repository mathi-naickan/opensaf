/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 */

/*****************************************************************************

  DESCRIPTION:
  This include file contains Control block and entity related data structures

******************************************************************************/
#ifndef PLM_PLMS_HRB_H_
#define PLM_PLMS_HRB_H_

#include <SaHpi.h>
#include "osaf/saf/saAis.h"

#if 0
/* macro definitions */
#define EPATH_BEGIN_CHAR          '{'
#define EPATH_END_CHAR            '}'
#define EPATH_SEPARATOR_CHAR      ','
#endif

#define PLMS_HRB_TASK_PRIORITY   5
#define PLMS_HRB_STACKSIZE       NCS_STACKSIZE_HUGEX2 

 /* In Service upgrade support */
#define PLMS_MDS_SUB_PART_VERSION  1


/* PLMS HRB Control block contains the following fields */
typedef struct plms_hrb_cb
{
	pthread_mutex_t    mutex; /*To synchronize access to session_id*/
	SaHpiSessionIdT    session_id;
	pthread_t          thread_id;
	SaUint32T          session_valid;
	NCSCONTEXT         task_hdl; /* TBD can be removed*/
	MDS_DEST           mdest;
	MDS_DEST           plms_mdest;
	SaUint32T          plms_up;
	MDS_HDL            mds_hdl;
	SYSF_MBX           mbx; /* PLMS HRBs mailbox  */
}PLMS_HRB_CB;


PLMS_HRB_CB *hrb_cb;

/* Function Declarations */
SaUint32T  plms_hrb_initialize(void);
SaUint32T  plms_hrb_finalize(void);
SaUint32T hrb_mds_initialize();
SaUint32T hrb_mds_finalize();

SaUint32T plms_hrb_mds_msg_sync_send(MDS_HDL mds_hdl,
			SaUint32T from_svc,
			SaUint32T to_svc,
			MDS_DEST    to_dest,
			PLMS_HPI_REQ *i_evt,
			PLMS_HPI_RSP **o_evt,
			SaTimeT timeout);
SaUint32T hrb_mds_msg_send(PLMS_HPI_RSP *response, MDS_SYNC_SND_CTXT context);
#endif  // PLM_PLMS_HRB_H_

