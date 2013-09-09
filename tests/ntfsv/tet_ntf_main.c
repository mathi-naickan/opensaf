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
#include <sys/time.h>
#include <unistd.h>
#include <utest.h>
#include "tet_ntf.h"

/* Change this to current highest supported version */
#define NTF_HIGHEST_SUPPORTED_VERSION {'A', 0x01, 0x01}

const SaVersionT refVersion = NTF_HIGHEST_SUPPORTED_VERSION;
SaVersionT ntfVersion = NTF_HIGHEST_SUPPORTED_VERSION;
SaAisErrorT rc = SA_AIS_OK;
SaNtfHandleT ntfHandle = 0;
SaNtfCallbacksT ntfCallbacks = {NULL, NULL};
SaSelectionObjectT selectionObject;
int verbose = 0;

struct tet_testlist
{
    void (*testfunc)();
    int icref;
    int tpnum;
};

int main(int argc, char **argv)
{
    int suite = -1, tcase = -1;

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

    return test_run(suite, tcase);

}


