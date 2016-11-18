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

#include <stdio.h>
#include <stdlib.h>
#include "osaf/saf/saAis.h"
#include "base/saf_error.h"

static const char *saf_error_name[] = {
	"OUT_OF_RANGE",
	"SA_AIS_OK (1)",
	"SA_AIS_ERR_LIBRARY (2)",
	"SA_AIS_ERR_VERSION (3)",
	"SA_AIS_ERR_INIT (4)",
	"SA_AIS_ERR_TIMEOUT (5)",
	"SA_AIS_ERR_TRY_AGAIN (6)",
	"SA_AIS_ERR_INVALID_PARAM (7)",
	"SA_AIS_ERR_NO_MEMORY (8)",
	"SA_AIS_ERR_BAD_HANDLE (9)",
	"SA_AIS_ERR_BUSY (10)",
	"SA_AIS_ERR_ACCESS (11)",
	"SA_AIS_ERR_NOT_EXIST (12)",
	"SA_AIS_ERR_NAME_TOO_LONG (13)",
	"SA_AIS_ERR_EXIST (14)",
	"SA_AIS_ERR_NO_SPACE (15)",
	"SA_AIS_ERR_INTERRUPT (16)",
	"SA_AIS_ERR_NAME_NOT_FOUND (17)",
	"SA_AIS_ERR_NO_RESOURCES (18)",
	"SA_AIS_ERR_NOT_SUPPORTED (19)",
	"SA_AIS_ERR_BAD_OPERATION (20)",
	"SA_AIS_ERR_FAILED_OPERATION (21)",
	"SA_AIS_ERR_MESSAGE_ERROR (22)",
	"SA_AIS_ERR_QUEUE_FULL (23)",
	"SA_AIS_ERR_QUEUE_NOT_AVAILABLE (24)",
	"SA_AIS_ERR_BAD_FLAGS (25)",
	"SA_AIS_ERR_TOO_BIG (26)",
	"SA_AIS_ERR_NO_SECTIONS (27)",
	"SA_AIS_ERR_NO_OP (28)",
	"SA_AIS_ERR_REPAIR_PENDING (29)",
	"SA_AIS_ERR_NO_BINDINGS (30)",
	"SA_AIS_ERR_UNAVAILABLE (31)",
	"SA_AIS_ERR_CAMPAIGN_ERROR_DETECTED (32)",
	"SA_AIS_ERR_CAMPAIGN_PROC_FAILED (33)",
	"SA_AIS_ERR_CAMPAIGN_CANCELED (34)",
	"SA_AIS_ERR_CAMPAIGN_FAILED (35)",
	"SA_AIS_ERR_CAMPAIGN_SUSPENDED (36)",
	"SA_AIS_ERR_CAMPAIGN_SUSPENDING (37)",
	"SA_AIS_ERR_ACCESS_DENIED (38)",
	"SA_AIS_ERR_NOT_READY (39)",
	"SA_AIS_ERR_DEPLOYMENT (40)"
};

const char *saf_error(SaAisErrorT error)
{
	if ((int)error < 0 || error > SA_AIS_ERR_DEPLOYMENT)
		return (char *)saf_error_name[0]; /* out of range */

	return ((char *)saf_error_name[error]);
}
