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
 * Author(s): Ericsson AB
 *
 */

/*
 * This file contains a command line utility to test IMM multiple appliers feature.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <libgen.h>
#include <assert.h>

#include <saAis.h>
#include <saImmOm.h>

#include <immutil.h>
#include "saf_error.h"
#include <poll.h>
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

static SaImmOiImplementerNameT implName;

static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - an IMM OI applier\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [class name]...\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a IMM OI client used to test ....\n", progname);

	printf("\nOPTIONS\n");
	printf("\t-h, --help\n");
	printf("\t\tthis help\n");
	printf("\t-a, --applier <OI-name> <class-name>\n");
	printf("\t\tRegister OI for class. Prefix OI-name with '@' for applier OI\n");

	printf("\nEXAMPLE\n");
	printf("\t%s -a \n", progname);
	printf("\timmadm -a test SaAmfNode SaAmfSU\n");
}

static SaAisErrorT saImmOiCcbObjectModifyCallback(SaImmOiHandleT immOiHandle,
						  SaImmOiCcbIdT ccbId,
						  const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = SA_AIS_OK;
	printf("Modify callback on %s - object:%s ccbId:%llu\n", implName, objectName->value, ccbId);

	struct CcbUtilCcbData *ccbUtilCcbData;
	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			fprintf(stderr, "Failed to get CCB objectfor %llu\n", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	/* "memorize the modification request" */
	ccbutil_ccbAddModifyOperation(ccbUtilCcbData, objectName, attrMods);

	/*rc = SA_AIS_ERR_BAD_OPERATION;*/

 done:
	return rc;
}

static SaAisErrorT saImmOiCcbObjectCreateCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaImmClassNameT className, const SaNameT *parentName, const SaImmAttrValuesT_2 **attr)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;

	printf("Create callback on %s - parent:%s ccbId:%llu\n", implName, parentName->value, ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			fprintf(stderr, "Failed to get CCB object for %llu\n", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}

	ccbutil_ccbAddCreateOperation(ccbUtilCcbData, className, parentName, attr);
 done:
	return rc;
}

static SaAisErrorT saImmOiCcbObjectDeleteCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId,
	const SaNameT *objectName)
{
	SaAisErrorT rc = SA_AIS_OK;
	struct CcbUtilCcbData *ccbUtilCcbData;
	printf("Delete callback on %s - object:%s ccbId:%llu\n", implName, objectName->value, ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		if ((ccbUtilCcbData = ccbutil_getCcbData(ccbId)) == NULL) {
			fprintf(stderr, "Failed to get CCB object for %llu\n", ccbId);
			rc = SA_AIS_ERR_NO_MEMORY;
			goto done;
		}
	}
	ccbutil_ccbAddDeleteOperation(ccbUtilCcbData, objectName);
 done:
	return rc;
}


static SaAisErrorT saImmOiCcbCompletedCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	printf("Completed callback on %s - ccbId:%llu\n", implName, ccbId);
	return SA_AIS_OK;
}

static void saImmOiCcbAbortCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	printf("ABORT callbackon %s. Cleanup CCB %llu\n", implName, ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) != NULL)
		/* Verify nok outcome with ccbUtilCcbData->userData */
		ccbutil_deleteCcbData(ccbUtilCcbData);
	else
		fprintf(stderr, "Failed to get CCB object for %llu\n", ccbId);
}


static void saImmOiCcbApplyCallback(SaImmOiHandleT immOiHandle, SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbUtilCcbData;

	printf("APPLY CALLBACK on %s cleanup CCB:%llu\n", implName, ccbId);

	if ((ccbUtilCcbData = ccbutil_findCcbData(ccbId)) == NULL) {
		fprintf(stderr, "Failed to get CCB object for %llu\n", ccbId);
		goto done;
	}

	ccbutil_deleteCcbData(ccbUtilCcbData);
 done:
	return;
}

static SaAisErrorT saImmOiRtAttrUpdateCallback(SaImmOiHandleT handle,
    const SaNameT *objectName,
    const SaImmAttrNameT *attributeNames)
{
    assert(objectName != NULL);
    assert(attributeNames != NULL);

    //    return saImmOiRtObjectUpdate_2(handle, objectName, (const SaImmAttrModificationT_2**) attrMods);
    return SA_AIS_OK;
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"parameter", required_argument, 0, 'p'},
		{"operation-id", required_argument, 0, 'o'},
		{"help", no_argument, 0, 'h'},
                {"timeout", required_argument, 0, 't'},
		{"applier", required_argument, 0, 'a'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaNameT objectName;
	const SaImmAdminOperationParamsT_2 **params;
	const SaImmOiCallbacksT_2 callbacks = {
		.saImmOiAdminOperationCallback = NULL,
		.saImmOiCcbAbortCallback = saImmOiCcbAbortCallback,
		.saImmOiCcbApplyCallback = saImmOiCcbApplyCallback,
		.saImmOiCcbCompletedCallback = saImmOiCcbCompletedCallback,
		.saImmOiCcbObjectCreateCallback = saImmOiCcbObjectCreateCallback,
		.saImmOiCcbObjectDeleteCallback = saImmOiCcbObjectDeleteCallback,
		.saImmOiCcbObjectModifyCallback = saImmOiCcbObjectModifyCallback,
		.saImmOiRtAttrUpdateCallback = saImmOiRtAttrUpdateCallback
	};
	struct pollfd fds[1];
	SaImmOiHandleT immOiHandle = 0LL;
	SaSelectionObjectT immOiSelectionObject = 0;
	SaVersionT immVersion = { 'A', 2, 11 };

	params = realloc(NULL, sizeof(SaImmAdminOperationParamsT_2 *));
	params[0] = NULL;

	while (1) {
		c = getopt_long(argc, argv, "p:o:t:a:h", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		case 'a':
			implName = strdup(optarg);
			if ((errno == EINVAL) || (errno == ERANGE)) {
				fprintf(stderr, "Illegal applier implementer name\n");
				exit(EXIT_FAILURE);
			}
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* Need at least one class to operate on */
	if ((argc - optind) == 0) {
		fprintf(stderr, "error - wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	printf("Implementer: %s\n", implName);
	error = saImmOiInitialize_2(&immOiHandle, &callbacks, &immVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOiInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}
	printf("ImmVersion: %c %u %u\n", immVersion.releaseCode, immVersion.majorVersion, immVersion.minorVersion);

	error = saImmOiImplementerSet(immOiHandle, implName);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOiImplementerSet FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	/* Remaining arguments should be class names for which implementer is set. */
	while (optind < argc) {
		strncpy((char *)objectName.value, argv[optind], SA_MAX_NAME_LENGTH);
		objectName.length = strlen((char *)objectName.value);

		printf("Class: %s\n", objectName.value);

		if(!strcmp((const char *) objectName.value, "OpensafImmRtTest")) {
			/* Special test case for RTO's. 
			   Class OpensafImmRtTest foundd at samples/immsv/immsv_test_classes_rtobj.xml
			*/
			SaStringT str1="testRdn=ZZZ";
			SaImmAttrValueT strValues[] = {&str1};
			SaImmAttrValuesT_2 v1 = { "testRdn",  SA_IMM_ATTR_SASTRINGT, 1, (void**)strValues };
			const SaImmAttrValuesT_2* attrValues[] = {&v1, NULL};

			error = saImmOiRtObjectCreate_2(immOiHandle, (SaImmClassNameT) objectName.value, NULL, attrValues);
			if (error != SA_AIS_OK && error != SA_AIS_ERR_EXIST) {
				fprintf(stderr, "error - saImmOiClassImplementerSet FAILED: %s\n", saf_error(error));
				exit(EXIT_FAILURE);
			}
			if(error == SA_AIS_OK) {
				printf("Runtime object: %s created\n", str1);
			} else {
				printf("Runtime object: %s exists\n", str1);
			}
		} else {
			error = saImmOiClassImplementerSet(immOiHandle, (SaImmClassNameT) objectName.value);
			if (error != SA_AIS_OK) {
				fprintf(stderr, "error - saImmOiClassImplementerSet FAILED: %s\n", saf_error(error));
				exit(EXIT_FAILURE);
			}
		}

		optind++;
	}

	error = saImmOiSelectionObjectGet(immOiHandle, &immOiSelectionObject);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOiSelectionObjectGet FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	fds[0].fd = immOiSelectionObject;
	fds[0].events = POLLIN;

	/* go into dispatch loop */
        while(1) {
		int ret = poll(fds, 1, -1);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			fprintf(stderr, "poll failed - %s\n", strerror(errno));
			break;
		}

		if (immOiHandle && fds[0].revents & POLLIN) {
			error = saImmOiDispatch(immOiHandle, SA_DISPATCH_ALL);

			if (error != SA_AIS_OK) {
				fprintf(stderr, "saImmOiDispatch returned %s\n", saf_error(error));
				exit(EXIT_FAILURE);
				break;
			}
		}
	}

	exit(EXIT_SUCCESS);
}
