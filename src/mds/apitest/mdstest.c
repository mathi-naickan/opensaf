/*       OpenSAF
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

#include <sys/time.h>
#include <unistd.h>
#include <limits.h>
#include "osaf/configmake.h"
#include "imm/saf/saImmOm.h"
#include "osaf/immutil/immutil.h"
#include "imm/saf/saImm.h"
#include "mds/mds_papi.h"
#include "base/ncs_mda_papi.h"
#include "base/ncs_main_papi.h"
#include "mdstipc.h"

/* The test framework */
#include "osaf/apitest/utest.h"
#include "osaf/apitest/util.h"

/* The tests */
//#include "mdstest.h"

SaAisErrorT rc;

int mds_startup(void)
{
  if(ncs_agents_shutdown()!=NCSCC_RC_SUCCESS)
  {
    printf("\n ----- NCS AGENTS SHUTDOWN FAILED ----- \n");
    return 1;
  }
  sleep(5);
  if(ncs_agents_startup()!=NCSCC_RC_SUCCESS)
  {
    printf("\n ----- NCS AGENTS START UP FAILED ------- \n");
    return 1;
  }

  return 0;
}

int mds_shutdown(void)
{
  if(ncs_agents_shutdown()!=NCSCC_RC_SUCCESS)
  {
    printf("\n ----- NCS AGENTS SHUTDOWN FAILED ----- \n");
    return 1;
  }

 return 0;
}

int main(int argc, char **argv) 
{
  int suite = ALL_SUITES, tcase = ALL_TESTS;

  srandom(getpid());

  if (argc > 1)
  {
    suite = atoi(argv[1]);
  }

  if (argc > 2)
  {
    tcase = atoi(argv[2]);
  }

  if (suite == 0)
  {
    test_list();
    return 0;
  }
	
  if (suite == 999) {
    return 0;
  }

  if (mds_startup() != 0) {
    printf("Fail to start mds agents\n");
    return 1;
  }

  int rc = test_run(suite, tcase);
  mds_shutdown();
  return rc;
}
