/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#ifndef LGA_STATE_H
#define	LGA_STATE_H

#ifdef	__cplusplus
extern "C" {
#endif

#include "lga.h"

/* Recovery functions */
void lga_no_server_state_set(void);
void lga_serv_recov1state_set(void);
int lga_recover_one_client(lga_client_hdl_rec_t *p_client);
void lga_recovery2_lock(void);
void lga_recovery2_unlock(void);

void set_lga_state(lga_state_t state);
bool is_lga_state(lga_state_t state);


/**
 * Protect critical areas AIS functions handling global client data so that
 * this data is not handled at the same time by the recovery 2 thread
 * Lock must be done before checking if lga_state is RECOVERY2
 * Unlock must be done just before returning from function. It is allowed to
 * call lga_recovery2_unlock() also if no previous call to lock is done
 */
static inline void recovery2_lock(bool *is_locked)
{
	if (is_lga_state(LGA_RECOVERY1) || is_lga_state(LGA_RECOVERY2)) {
		lga_recovery2_lock();
		*is_locked = true;
	}
}

static inline void recovery2_unlock(bool *is_locked)
{
	if (*is_locked) {
		*is_locked = false;
		lga_recovery2_unlock();
	}
}

#ifdef	__cplusplus
}
#endif

#endif	/* LGA_STATE_H */

