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

/*
 * This file contains a command line utility to perform IMM search operations.
 * Example: immfind"
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <libgen.h>
#include <signal.h>

#include <saAis.h>
#include <saImmOm.h>
#include <immutil.h>
#include <saf_error.h>

static SaVersionT immVersion = { 'A', 2, 11 };
extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* signal handler for SIGALRM */
void sigalarmh(int sig)
{
        fprintf(stderr, "error - immfind command timed out (alarm)\n");
        exit(EXIT_FAILURE);
}


static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - search for IMM objects\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [path ...] [options]\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is an IMM OM client used to find IMM objects.\n", progname);
	printf("\tAll objects or objects of a certain class can be searched for.\n");

	printf("\nOPTIONS\n");
	printf("\t-c, --class=NAME\n");
	printf("\t\tonly search for objects of the specified class\n");
	printf("\t-s, --scope=SCOPE\n");
	printf("\t\tspecify search scope, valid scopes: sublevel subtree\n");
	printf("\t-t, --timeout <sec>\n");
	printf("\t\tutility timeout in seconds\n");
	printf("\t-h, --help\n");
	printf("\t\tthis help\n");

	printf("\nEXAMPLE\n");
	printf("\timmfind\n");
	printf("\t\tsearch for all objects\n");
	printf("\timmfind safApp=myApp\n");
	printf("\t\tsearch for all objects rooted under safApp=myApp\n");
	printf("\timmfind safApp=myApp -s sublevel\n");
	printf("\t\tsearch for all objects rooted under safApp=myApp scope sublevel\n");
	printf("\timmfind safApp=myApp --scope subtree\n");
	printf("\t\tsearch for all objects rooted under safApp=myApp scope subtree\n");
	printf("\timmfind -c SaAmfApplication\n");
	printf("\t\tsearch for all objects of class SaAmfApplication\n");
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"class", required_argument, 0, 'c'},
		{"scope", required_argument, 0, 's'},
		{"timeout", required_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmSearchHandleT searchHandle;
	SaImmSearchParametersT_2 searchParam;
	SaNameT objectName;
	SaImmAttrValuesT_2 **attributes;
	SaNameT rootName = { 0, "" };
	SaImmScopeT scope = SA_IMM_SUBTREE;	/* default search scope */
	char classNameBuf[SA_MAX_NAME_LENGTH] = {0};
	const char *className = classNameBuf;
	unsigned long timeoutVal = 60;

	while (1) {
		c = getopt_long(argc, argv, "c:s:t:h", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'c':
			strncpy(classNameBuf, optarg, SA_MAX_NAME_LENGTH);
			break;
		case 's':
			if (strcmp(optarg, "sublevel") == 0)
				scope = SA_IMM_SUBLEVEL;
			else if (strcmp(optarg, "subtree") == 0)
				scope = SA_IMM_SUBTREE;
			else {
				fprintf(stderr, "error - illegal scope: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
			break;
		case 't':
                        timeoutVal = strtol(optarg, (char **)NULL, 10);
                        break;

		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if ((argc - optind) > 1) {
		fprintf(stderr, "error - too many arguments\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGALRM, sigalarmh);
	alarm(timeoutVal);

        immutilWrapperProfile.errorsAreFatal = 0;
        immutilWrapperProfile.nTries = timeoutVal;
        immutilWrapperProfile.retryInterval = 1000;

	if (optind < argc) {
		strncpy((char *)rootName.value, argv[optind], SA_MAX_NAME_LENGTH);
		rootName.length = strlen((char *)rootName.value);
	}

	error = immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	if (className[0] != 0) {
		searchParam.searchOneAttr.attrName = "SaImmAttrClassName";
		searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
		searchParam.searchOneAttr.attrValue = &className;
	} else {
		searchParam.searchOneAttr.attrName = NULL;
		searchParam.searchOneAttr.attrValue = NULL;
	}

	error = immutil_saImmOmSearchInitialize_2(immHandle, &rootName, scope,
					  SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, &searchParam, NULL,
					  &searchHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmSearchInitialize_2 FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	do {
		error = immutil_saImmOmSearchNext_2(searchHandle, &objectName, &attributes);
		if (error != SA_AIS_OK && error != SA_AIS_ERR_NOT_EXIST) {
			fprintf(stderr, "error - saImmOmSearchNext_2 FAILED: %s\n", saf_error(error));
			exit(EXIT_FAILURE);
		}
		if (error == SA_AIS_OK)
			printf("%s\n", objectName.value);
	} while (error != SA_AIS_ERR_NOT_EXIST);

	error = immutil_saImmOmSearchFinalize(searchHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmSearchFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = immutil_saImmOmFinalize(immHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

