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
#include "osaf/apitest/utest.h"
#include "tet_ntf.h"

/* Change this to current highest supported version */
#define NTF_HIGHEST_SUPPORTED_VERSION {'A', 0x01, 0x02}
const SaVersionT refVersion = NTF_HIGHEST_SUPPORTED_VERSION;
const SaVersionT lowestVersion =  {'A', 0x01, 0x01};
SaVersionT ntfVersion = NTF_HIGHEST_SUPPORTED_VERSION;
SaAisErrorT rc = SA_AIS_OK;
SaNtfHandleT ntfHandle = 0;
SaNtfCallbacksT ntfCallbacks = {NULL, NULL};
SaSelectionObjectT selectionObject;
extern void add_scOutage_reinitializeHandle_test(void);
extern int verbose;
extern int gl_tag_mode;
extern int gl_prompt_mode;

struct tet_testlist
{
    void (*testfunc)();
    int icref;
    int tpnum;
};
static void print_usage(void)
{
	printf("\nNAME\n");
	printf("\nntftest - Test NTF service\n");

	printf("\nSYNOPSIS\n");
	printf("\t ntftest [options]\n");

	printf(	"\nDESCRIPTION\n");
	printf(	"\t Run test suite or single test case for NTF service.\n"
			"\t Tests include automatic and manual tests. If ntftest\n"
			"\t is used with option -e, only manual test will be \n"
			"\t executed. Otherwise, automatic test will be executed\n"
			"\t It is also possible to run one test case, one test\n"
			"\t suite or all test suites.\n");


	printf("\nOPTIONS\n");

	printf(	"  '0'\t\t List all automatic test cases. No test case\n"
			"  \t\t is executed.\n");
	printf(	"  -l\t\t List all automatic and manual test cases \n");
	printf(	"  -e <s> [t]\t Run manual test case 't' in suite 's'\n");
	printf(	"  <s> [t]\t Run automatic test case 't' in suite 's'\n");

	printf(	"  \t\t In both cases of with or without -e, if 't' is\n"
			"  \t\t omitted, all test cases in suite 's' will be\n");
	printf(	"  \t\t run. If both 's' and 't' are omitted, all test\n");
	printf(	"  \t\t suites of manual tests will only be run. EX:\n");
	printf( "  \t\t ntftest 1       Run test suite 1\n"
		"  \t\t ntftest 1 1     Run test case 1 in suite 1\n"
		"  \t\t ntftest -e 37   Run test suite 37\n"
			"  \t\t ntftest -e 37 1 Run test case 1 of suite 37\n"
			"  \t\t ntftest \t Run all automatic test suites\n");
	printf(	"  -v\t\t Activate verbose printing (applicable for \n"
			"  \t\t some test cases)\n");

	printf(	"\n");
	printf( "  The following options can only be used together with option\n");
	printf(	"  -e. Running manual test case requires interaction from tester\n");
	printf(	"  or external test program. That means ntftest needs to inform\n");
	printf(	"  the next step to be executed.\n");
	printf(	"\n");

	printf(	"  -p\t\t Activate prompt mode. EX: ntftest prints out a\n"
			"  \t\t message to manually start/stop SC, or press 'key'\n"
			"  \t\t to continue the test,...\n");

	printf(	"  -t\t\t Activate tag mode. A tag is a short and \n"
			"  \t\t CONSISTENT string that is printed to inform \n"
			"  \t\t external test programs to take appropriate \n"
			"  \t\t actions. EX:\n"
			"  \t\t - \"TAG_ND\" means the test case asks the SCs to\n"
			"  \t\t be stopped, and external test programs send USR2\n"
			"  \t\t to continue test case after successfully stop SCs.\n"
			"  \t\t - \"TAG_UP\" means the test case waits for the SCs \n"
			"  \t\t starting up, and external test programs send USR2\n"
			"  \t\t to continue test case after successfully start SCs.\n");
	printf(	"  -h\t\t This help\n");
	printf(	"\n");
}

static void err_exit(void)
{
	print_usage();
	exit(1);
}

int main(int argc, char **argv)
{
    int suite = ALL_SUITES, tcase = ALL_TESTS;
	int option = 0;
	int extendedTest = 0;

    srandom(getpid());

    verbose = 0;
    gl_tag_mode = 0;
    gl_prompt_mode = 0;
	/* Check option */
	char optstr[] ="hle:vpt";

	if (argc >= 1) {
		while ((option = getopt(argc, argv, optstr)) != -1) {
			switch (option) {
			case 'h':
				print_usage();
				exit(0);
			case 'l':
				add_scOutage_reinitializeHandle_test();
				test_list();
				exit(0);
			case 'v':
				verbose = 1;
				gl_prompt_mode = 1;
				break;
			case 't':
				// Can't use with prompt mode or verbose
				if (gl_prompt_mode == 1 || verbose == 1)
					err_exit();

				gl_tag_mode = 1;
				break;
			case 'p':
				// Can't use with tag mode
				if (gl_tag_mode == 1)
					err_exit();
				gl_prompt_mode = 1;
				break;
			case 'e':
				if (argv[optind-1] != NULL) {
					suite = atoi(argv[optind-1]);
					extendedTest = 1;
					add_scOutage_reinitializeHandle_test();
				} else
					err_exit();
				if (optind < argc && argv[optind] != NULL)
					tcase = atoi(argv[optind]);

				break;
			}
		}
	}

	// Can't run -t/-p without -e
	if (!extendedTest && (gl_prompt_mode == 1 || gl_tag_mode == 1))
		err_exit();

	// If none of modes is set with -e, prompt is chosen
	if (extendedTest && gl_prompt_mode == 0 && gl_tag_mode == 0)
		gl_prompt_mode = 1;

	if (!extendedTest) {
		if (argc > 1)
			suite = atoi(argv[1]);

		if (argc > 2)
			tcase = atoi(argv[2]);

		if (suite == 0) {
			test_list();
			return 0;
		}
	}
    return test_run(suite, tcase);

}


