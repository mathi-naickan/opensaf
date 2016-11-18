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
 * Author(s): Hewlett-Packard Development Company, L.P.
 *
 */

/*
 * plmc_cmd_string_to_enum() utility function.
 *
 * plmc_cmd_string_to_enum() converts a string value of a PLMc
 * command to its enum equivalent.  If the command cannot be
 * identified, -1 is returned.
 *
 */

#include <string.h>
#include "plm/plmcd/plmc.h"
#include "plm/plmcd/plmc_cmds.h"

/*
 * plmc_cmd_string_to_enum()
 *
 * A function to convert a PLMc string command name to its
 * enum equivalent.
 *
 * Inputs:
 *   cmd : A string version of a PLMc command name.
 *
 * Returns: enum value of PLMc command.
 * Returns: -1 on error.
 *
 */
int plmc_cmd_string_to_enum(char *cmd)
{
  int i;

  for (i=0; i<=PLMC_LAST_CMD; i++) {
    if (strcmp(cmd, PLMC_cmd_name[i]) == 0)
      return i;
  }

  return -1;
}

