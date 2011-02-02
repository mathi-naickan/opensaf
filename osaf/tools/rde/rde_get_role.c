/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2011 The OpenSAF Foundation
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
 * Author(s): Goahead Software 
 *
 */

#include <configmake.h> 
#include "rda_papi.h"  /* for RDE interfacing */

/****************************************************************************
 * Name          : main                                                     *
 *                                                                          *
 * Description   : get the RDE role and print it to stdout + exit code      *
 *                                                                          *
 * Arguments     : argc - no of command line args                           *
 *                 argv - List of arguments.                                *
 *                                                                          *
 * Return Values : role                                                     *
 *                                                                          *
 * Notes         : None.                                                    *
 ***************************************************************************/
int main(int argc, char **argv)
{
	SaAmfHAStateT ha_state = 0;
	uns32 ret_val = NCSCC_RC_SUCCESS;

	ret_val = rda_get_role(&ha_state);

	if (NCSCC_RC_SUCCESS == ret_val) {
		if (SA_AMF_HA_ACTIVE == ha_state) {
			printf("ACTIVE\n");
		}
		else if (SA_AMF_HA_STANDBY == ha_state) {
			printf("STANDBY\n");
		}
		else {
			printf("UNKNOWN\n");
			return EXIT_FAILURE;
		}
		return EXIT_SUCCESS;
	} else {
		printf("rda_get_role failed\n");
		return EXIT_FAILURE;
	}
}
