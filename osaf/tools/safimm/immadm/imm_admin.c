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
 * This file contains a command line utility to perform IMM admin operations.
 * Example: immadm -o 3 -p saAmfNodeSuFailoverMax:SA_INT32_T:7 "safAmfNode=Node01,safAmfCluster=1"
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

#include <saAis.h>
#include <saImmOm.h>

#include <immutil.h>
#include "saf_error.h"

#define PARAMDELIM ":"

static SaVersionT immVersion = { 'A', 2, 1 };

/**
 * 
 */
static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - perform an IMM admin operation\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [object DN]...\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a IMM OM client used to ....\n", progname);

	printf("\nOPTIONS\n");
	printf("\t-h, --help\n");
	printf("\t\tthis help\n");
	printf("\t-o, --operation-id <id>\n");
	printf("\t\tnumerical operation ID (mandatory)\n");
	printf("\t-p, --parameter <p>\n");
	printf("\t\tparameter(s) to admin op\n");
	printf("\t\tParameter syntax: <name>:<type>:<value>\n");
	printf("\t\tValue types according to imm.xsd.\n"
	       "\t\tValid types: SA_INT32_T, SA_UINT32_T, SA_INT64_T, SA_UINT64_T\n"
	       "\t\t\tSA_TIME_T, SA_NAME_T, SA_FLOAT_T, SA_DOUBLE_T, SA_STRING_T\n");

	printf("\nEXAMPLE\n");
	printf("\timmadm -o 2 safAmfNode=SC-2,safAmfCluster=myAmfCluster\n");
	printf("\timmadm -o 1 -p lockOption:SA_STRING_T:trylock  safEE=SC-1,safDomain=domain_1\n");
}

static SaImmValueTypeT str2_saImmValueTypeT(const char *str)
{
	if (!str)
		return -1;

	if (strcmp(str, "SA_INT32_T") == 0)
		return SA_IMM_ATTR_SAINT32T;
	if (strcmp(str, "SA_UINT32_T") == 0)
		return SA_IMM_ATTR_SAUINT32T;
	if (strcmp(str, "SA_INT64_T") == 0)
		return SA_IMM_ATTR_SAINT64T;
	if (strcmp(str, "SA_UINT64_T") == 0)
		return SA_IMM_ATTR_SAUINT64T;
	if (strcmp(str, "SA_TIME_T") == 0)
		return SA_IMM_ATTR_SATIMET;
	if (strcmp(str, "SA_NAME_T") == 0)
		return SA_IMM_ATTR_SANAMET;
	if (strcmp(str, "SA_FLOAT_T") == 0)
		return SA_IMM_ATTR_SAFLOATT;
	if (strcmp(str, "SA_DOUBLE_T") == 0)
		return SA_IMM_ATTR_SADOUBLET;
	if (strcmp(str, "SA_STRING_T") == 0)
		return SA_IMM_ATTR_SASTRINGT;
	if (strcmp(str, "SA_ANY_T") == 0)
		return SA_IMM_ATTR_SAANYT;

	return -1;
}

static int init_param(SaImmAdminOperationParamsT_2 *param, char *arg)
{
	int res = 0;
	char *attrValue;
	char *tmp = strdup(arg);

	if ((param->paramName = strtok(tmp, PARAMDELIM)) == NULL) {
		res = -1;
		goto done;
	}

	if ((param->paramType = str2_saImmValueTypeT(strtok(NULL, PARAMDELIM))) == -1) {
		res = -1;
		goto done;
	}

	if ((attrValue = strtok(NULL, PARAMDELIM)) == NULL) {
		res = -1;
		goto done;
	}

	param->paramBuffer = immutil_new_attrValue(param->paramType, attrValue);

	if (param->paramBuffer == NULL)
		return -1;

 done:
	return res;
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"parameter", required_argument, 0, 'p'},
		{"operation-id", required_argument, 0, 'o'},
		{"help", no_argument, 0, 'h'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmAdminOwnerNameT adminOwnerName = basename(argv[0]);
	SaImmAdminOwnerHandleT ownerHandle;
	SaNameT objectName;
	const SaNameT *objectNames[] = { &objectName, NULL };
	SaAisErrorT operationReturnValue = -1;
	SaImmAdminOperationParamsT_2 *param;
	const SaImmAdminOperationParamsT_2 **params;
	SaImmAdminOperationIdT operationId = -1;
	int params_len = 0;

	params = realloc(NULL, sizeof(SaImmAdminOperationParamsT_2 *));
	params[0] = NULL;

	while (1) {
		c = getopt_long(argc, argv, "p:o:h", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'o':
			operationId = strtoll(optarg, (char **)NULL, 10);
			if ((operationId == 0) && ((errno == EINVAL) || (errno == ERANGE))) {
				fprintf(stderr, "Illegal operation ID\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'p':
			params_len++;
			params = realloc(params, (params_len + 1) * sizeof(SaImmAdminOperationParamsT_2 *));
			param = malloc(sizeof(SaImmAdminOperationParamsT_2));
			params[params_len - 1] = param;
			params[params_len] = NULL;
			if (init_param(param, optarg) == -1) {
				fprintf(stderr, "Illegal parameter: %s\n", optarg);
				exit(EXIT_FAILURE);
			}
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

	if (operationId == -1) {
		fprintf(stderr, "error - must specify numerical operation ID\n");
		exit(EXIT_FAILURE);
	}

	/* Need at least one object to operate on */
	if ((argc - optind) == 0) {
		fprintf(stderr, "error - wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	error = saImmOmInitialize(&immHandle, NULL, &immVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, &ownerHandle);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	/* Remaining arguments should be object names on which the admin op should be performed. */
	while (optind < argc) {
		strncpy((char *)objectName.value, argv[optind], SA_MAX_NAME_LENGTH);
		objectName.length = strlen((char *)objectName.value);

		error = saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE);
		if (error != SA_AIS_OK) {
			if (error == SA_AIS_ERR_NOT_EXIST)
				fprintf(stderr, "error - saImmOmAdminOwnerSet - object '%s' does not exist\n",
					objectName.value);
			else
				fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));
			exit(EXIT_FAILURE);
		}

		error = saImmOmAdminOperationInvoke_2(ownerHandle, &objectName, 0, operationId,
			params, &operationReturnValue, SA_TIME_ONE_SECOND * 60);

		if (error != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmAdminOperationInvoke_2 FAILED: %s\n",
				saf_error(error));
			exit(EXIT_FAILURE);
		}

		if (operationReturnValue != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmAdminOperationInvoke_2 admin-op RETURNED: %s\n",
				saf_error(operationReturnValue));
			exit(EXIT_FAILURE);
		}

		error = saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE);
		if (error != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmAdminOwnerRelease FAILED: %s\n", saf_error(error));
			exit(EXIT_FAILURE);
		}

		optind++;
	}

	error = saImmOmAdminOwnerFinalize(ownerHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmAdminOwnerFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = saImmOmFinalize(immHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}
