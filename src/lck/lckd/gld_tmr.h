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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:
  
  GND Timer related definitions.
    
******************************************************************************
*/

#ifndef LCK_LCKD_GLD_TMR_H_
#define LCK_LCKD_GLD_TMR_H_

#include "base/ncssysf_tmr.h"

#define GLSV_GLND_MASTER_REELECTION_WAIT_TIME     300000000LL	/* platform dependent time for waiting during master relection */
#define GLSV_GLND_NODE_RESTART_WAIT     30000000	/* platform dependent time for waiting during node restrt */

#define GLD_NODE_RESTART_TIMEOUT 10000000000LL

struct glsv_gld_cb_tag;

/* timer type enums */
typedef enum gld_tmr_type {
	GLD_TMR_NODE_RESTART_TIMEOUT = 1,
	GLD_TMR_RES_REELECTION_WAIT,
	GLD_TMR_MAX,
} GLD_TMR_TYPE;

/* GLD Timer definition */
typedef struct gld_tmr {
	tmr_t tmr_id;
	GLD_TMR_TYPE type;	/* timer type */
	uint32_t cb_hdl;		/* cb hdl to retrieve the GLD cb ptr */
	uint32_t opq_hdl;		/* hdl to retrive the timer context */
	bool is_active;
	MDS_DEST mdest_id;
	SaLckResourceIdT resource_id;
} GLD_TMR;

/*** Extern function declarations ***/

void gld_tmr_exp(void *);

uint32_t gld_start_tmr(struct glsv_gld_cb_tag *, GLD_TMR *, GLD_TMR_TYPE, SaTimeT, uint32_t);

void gld_stop_tmr(GLD_TMR *);

#endif  // LCK_LCKD_GLD_TMR_H_
