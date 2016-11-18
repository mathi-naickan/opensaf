/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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
 * Author(s): Hewlett-Packard Development Company, L.P.
 *
 */

/*
 * PLMc commands file
 *
 * This header file is used by the plmcd and plmc_lib implementations.
 * Users of the PLMc library should not include this header file.  Users
 * of the PLMc library should instead include the plmc_lib.h header file.
 *
 */

#ifndef PLM_PLMCD_PLMC_CMDS_H_
#define PLM_PLMCD_PLMC_CMDS_H_

/* The PLMC daemon command names.                                */
/* Names with SA in them refer to the PLM specification actions. */
static char *PLMC_cmd_name[] = { 
  "PLMC_NOOP",
  "PLMC_GET_ID",
  "PLMC_GET_PROTOCOL_VER",
  "PLMC_GET_OSINFO",
  "PLMC_SA_PLM_ADMIN_UNLOCK",
  "PLMC_SA_PLM_ADMIN_LOCK_INSTANTIATION",
  "PLMC_SA_PLM_ADMIN_LOCK",
  "PLMC_SA_PLM_ADMIN_RESTART",
  "PLMC_OSAF_START",
  "PLMC_OSAF_STOP",
  "PLMC_OSAF_SERVICES_START",
  "PLMC_OSAF_SERVICES_STOP",
  "PLMC_PLMCD_RESTART"
};

#endif  // PLM_PLMCD_PLMC_CMDS_H_
