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
#include <fcntl.h>
#include <ctype.h>
#include <libgen.h>
#include <assert.h>
#include <signal.h>
#include <stdbool.h>
#include <time.h>

#include <saAis.h>
#include <saImmOm.h>
#include <immsv_api.h>

#include <immutil.h>
#include <saf_error.h>
#include "osaf_extended_name.h"

#define PARAMDELIM ":"

extern struct ImmutilWrapperProfile immutilWrapperProfile;

static SaVersionT immVersion = { 'A', 2, 16 };

static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - perform an IMM admin operation\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [object DN]...\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is a IMM OM client used to ....\n", progname);

	printf("\nOPTIONS\n");
	printf("\t--disable-tryagain\n");
	printf("\t\tdisable try again handling [default=no]\n");
	printf("\t-h, --help\n");
	printf("\t\tthis help\n");
	printf("\t-o, --operation-id <id>\n");
	printf("\t\tnumerical operation ID (mandatory)\n");
	printf("\t-O, --operation-name <name>\n");
	printf("\t\toperation name (mandatory)\n");
	printf("\t-a, --admin-owner <admin owner name>\n");
	printf("\t-p, --parameter <p>\n");
	printf("\t\tparameter(s) to admin op\n");
	printf("\t\tParameter syntax: <name>:<type>:<value>\n");
	printf("\t\tValue types according to imm.xsd.\n"
	       "\t\tValid types: SA_INT32_T, SA_UINT32_T, SA_INT64_T, SA_UINT64_T\n"
	       "\t\t\tSA_TIME_T, SA_NAME_T, SA_FLOAT_T, SA_DOUBLE_T, SA_STRING_T\n");
	printf("\t-t, --timeout <sec>\n");
	printf("\t\tcommand timeout in seconds [default=60]\n");
	printf("\t-v, --verbose\n");

	printf("\nEXAMPLE\n");
	printf("\t%s -o 2 safAmfNode=SC-2,safAmfCluster=myAmfCluster\n", progname);
	printf("\t%s -o 1 -p lockOption:SA_STRING_T:trylock  safEE=SC-1,safDomain=domain_1\n", progname);

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
	char *paramType = NULL;
	char *paramName = NULL;
	int offset = 0;

	if ((paramName = strtok(tmp, PARAMDELIM)) == NULL) {
		res = -1;
		goto done;
	}

	offset += strlen(paramName);

	if ((param->paramName = strdup(paramName)) == NULL) {
		res = -1;
		goto done;
	}

	if ((paramType = strtok(NULL, PARAMDELIM)) == NULL) {
		res = -1;
		goto done;
	}

	offset += strlen(paramType);

	if ((param->paramType = str2_saImmValueTypeT(paramType)) == -1) {
		res = -1;
		goto done;
	}

	/* make sure there is a param value */
	if ((attrValue = strtok(NULL, PARAMDELIM)) == NULL) {
		res = -1;
		goto done;
	}

	/* get the attrValue. Also account for the 2 colons used to separate the parameters */
	attrValue = arg + offset + 2;

	param->paramBuffer = immutil_new_attrValue(param->paramType, attrValue);

	if (param->paramBuffer == NULL) {
		res = -1;
		goto done;
	}

 done:
	free(tmp);
	return res;
}

void sigalarmh(int sig)
{
	fprintf(stderr, "error - command timed out (alarm)\n");
	exit(EXIT_FAILURE);
}

void print_param(SaImmAdminOperationParamsT_2 *param) {
	switch(param->paramType) {
		case SA_IMM_ATTR_SAINT32T :
			printf("%-50s %-12s %d (0x%x)\n", param->paramName, "SA_INT32_T",
					(*((SaInt32T *)param->paramBuffer)), (*((SaInt32T *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SAUINT32T :
			printf("%-50s %-12s %u (0x%x)\n", param->paramName, "SA_UINT32_T",
					(*((SaUint32T *)param->paramBuffer)), (*((SaUint32T *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SAINT64T :
			printf("%-50s %-12s %lld (0x%llx)\n", param->paramName, "SA_INT64_T",
					(*((SaInt64T *)param->paramBuffer)), (*((SaInt64T *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SAUINT64T :
			printf("%-50s %-12s %llu (0x%llx)\n", param->paramName, "SA_UINT64_T",
					(*((SaUint64T *)param->paramBuffer)), (*((SaUint64T *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SATIMET :
			{
				char buf[32];
				const time_t time = *((SaTimeT *)param->paramBuffer) / SA_TIME_ONE_SECOND;

				ctime_r(&time, buf);
				buf[strlen(buf) - 1] = '\0';	/* Remove new line */
				printf("%-50s %-12s %llu (0x%llx, %s)\n", param->paramName, "SA_TIME_T",
						(*((SaTimeT *)param->paramBuffer)), (*((SaTimeT *)param->paramBuffer)), buf);
			}
			break;
		case SA_IMM_ATTR_SANAMET :
			printf("%-50s %-12s %s\n", param->paramName, "SA_NAME_T", saAisNameBorrow((SaNameT*) param->paramBuffer));
			break;
		case SA_IMM_ATTR_SAFLOATT :
			printf("%-50s %-12s %f\n", param->paramName, "SA_FLOAT_T", (*((SaFloatT *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SADOUBLET :
			printf("%-50s %-12s %lf\n", param->paramName, "SA_DOUBLE_T", (*((SaDoubleT *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SASTRINGT :
			printf("%-50s %-12s %s\n", param->paramName, "SA_STRING_T", (*((SaStringT *)param->paramBuffer)));
			break;
		case SA_IMM_ATTR_SAANYT :
			printf("%-50s %-12s %s\n", param->paramName, "SA_ANY_T", "<Not shown>");
			break;
		default:
			printf("%-50s <%-12s (%d)>\n", param->paramName, "Unknown value type", (int)param->paramType);
			exit(EXIT_FAILURE);
	}
}

void print_params(char *objectDn, SaImmAdminOperationParamsT_2 **params) {
	int ix = 0;

	printf("Object: %s\n", objectDn);
	printf("%-50s %-12s Value(s)\n", "Name", "Type");
	printf("========================================================================\n");
	while(params[ix]) {
		print_param(params[ix]);
		++ix;
	}
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"disable-tryagain", no_argument, 0, 'd'},
		{"parameter", required_argument, 0, 'p'},
		{"operation-id", required_argument, 0, 'o'},
		{"operation-name", required_argument, 0, 'O'},
		{"admin-owner", required_argument, 0, 'a'},
		{"help", no_argument, 0, 'h'},
		{"timeout", required_argument, 0, 't'},
		{"verbose", no_argument, 0, 'v'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmAdminOwnerNameT adminOwnerName = basename(argv[0]);
	bool releaseAdmo=true;
	bool explicitAdmo=false;
	SaImmAdminOwnerHandleT ownerHandle;
	SaNameT objectName;
	const SaNameT *objectNames[] = { &objectName, NULL };
	SaAisErrorT operationReturnValue = -1;
	SaImmAdminOperationParamsT_2 *param;
	const SaImmAdminOperationParamsT_2 **params;
	SaImmAdminOperationParamsT_2 **out_params=NULL;
	SaImmAdminOperationIdT operationId = -1;
	unsigned long timeoutVal = 60;  /* Default timeout value */
	int disable_tryagain = false;
	int isFirst = 1;
	int verbose = 0;

	int params_len = 0;

	/* Support for long DN */
	setenv("SA_ENABLE_EXTENDED_NAMES", "1", 1);
	/* osaf_extended_name_init() is added to prevent future safe use of
	 * osaf_extended_name_* before saImmOmInitialize and saImmOiInitialize */
	osaf_extended_name_init();

	params = realloc(NULL, sizeof(SaImmAdminOperationParamsT_2 *));
	params[0] = NULL;
	SaStringT opName = NULL;

	while (1) {
		c = getopt_long(argc, argv, "dp:o:O:a:t:hv", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'd':
			disable_tryagain = true;
			break;
		case 'o':
			if(operationId != -1) {
				fprintf(stderr, "Cannot set admin operation more then once");
				exit(EXIT_FAILURE);
			}
			operationId = strtoll(optarg, (char **)NULL, 10);
			if ((operationId == 0) && ((errno == EINVAL) || (errno == ERANGE))) {
				fprintf(stderr, "Illegal operation ID\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'O':
			if(operationId != -1) {
				fprintf(stderr, "Cannot set admin operation more then once");
				exit(EXIT_FAILURE);
			}
			operationId = SA_IMM_PARAM_ADMOP_ID_ESC;
			params_len++;
			params = realloc(params, (params_len + 1) * sizeof(SaImmAdminOperationParamsT_2 *));
			param = malloc(sizeof(SaImmAdminOperationParamsT_2));
			params[params_len - 1] = param;
			params[params_len] = NULL;
			param->paramName = strdup(SA_IMM_PARAM_ADMOP_NAME);
			param->paramType = SA_IMM_ATTR_SASTRINGT;
			param->paramBuffer = malloc(sizeof(SaStringT));
			*((SaStringT *)(param->paramBuffer)) = strdup(optarg);
			opName = strdup(optarg);
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
		case 't':
			timeoutVal = strtoll(optarg, (char **)NULL, 10);

			if ((timeoutVal == 0) || (errno == EINVAL) || (errno == ERANGE)) {
				fprintf(stderr, "Illegal timeout value\n");
				exit(EXIT_FAILURE);
			}
			break;
		case 'a':
			adminOwnerName = (SaImmAdminOwnerNameT)malloc(strlen(optarg) + 1);
			strcpy(adminOwnerName, optarg);
			releaseAdmo=false;
			explicitAdmo=true;
			break;
		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		case 'v':
			verbose = 1;
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	if (operationId == -1) {
		fprintf(stderr, "error - must specify admin operation ID %llx\n", operationId);
		exit(EXIT_FAILURE);
	}

	/* Need at least one object to operate on */
	if ((argc - optind) == 0) {
		fprintf(stderr, "error - wrong number of arguments\n");
		exit(EXIT_FAILURE);
	}

	signal(SIGALRM, sigalarmh);
	(void) alarm(timeoutVal);

	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.nTries = disable_tryagain ? 0 : timeoutVal;
	immutilWrapperProfile.retryInterval = 1000;

	error = immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	if((optind < argc) && (!explicitAdmo)) {
		saAisNameLend(argv[optind], &objectName);

		if(strcmp(saAisNameBorrow(&objectName), OPENSAF_IMM_OBJECT_DN)==0) {
			releaseAdmo=false;
			adminOwnerName = (SaImmAdminOwnerNameT) malloc(strlen(OPENSAF_IMM_SERVICE_NAME) + 1);
			strcpy(adminOwnerName, OPENSAF_IMM_SERVICE_NAME);
			printf("[using admin-owner: '%s']\n", adminOwnerName);
		}
	}

	error = immutil_saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, releaseAdmo?SA_TRUE:SA_FALSE, &ownerHandle);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerInitialize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	/* Remaining arguments should be object names on which the admin op should be performed. */
	while (optind < argc) {
		saAisNameLend(argv[optind], &objectName);

		error = immutil_saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE);
		if (error != SA_AIS_OK) {
			if (error == SA_AIS_ERR_NOT_EXIST) {
				if(strcmp(adminOwnerName, saAisNameBorrow(&objectName))==0) {
					/* AdminOwnerName == ImplementerName - Could be direct admin-op on OI */
					goto retry;
				}
				fprintf(stderr, "error - saImmOmAdminOwnerSet - object '%s' does not exist\n",
					saAisNameBorrow(&objectName));
			}
			else
				fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));
			exit(EXIT_FAILURE);
		}
retry:
		error = immutil_saImmOmAdminOperationInvoke_o2(ownerHandle, &objectName, 0, operationId,
			params, &operationReturnValue, SA_TIME_ONE_SECOND * timeoutVal, &out_params);

		if (error != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmAdminOperationInvoke_2 FAILED: %s\n",
				saf_error(error));
			exit(EXIT_FAILURE);
		}

		if (operationReturnValue != SA_AIS_OK ) {
			unsigned int ix = 0;

			if ((operationReturnValue == SA_AIS_ERR_TRY_AGAIN) && !disable_tryagain) {
				sleep(1);
				goto retry;
			}

			fprintf(stderr, "error - saImmOmAdminOperationInvoke_2 admin-op RETURNED: %s\n",
				saf_error(operationReturnValue));
			
			while(out_params && out_params[ix]) {
				if(strcmp(out_params[ix]->paramName, SA_IMM_PARAM_ADMOP_ERROR) == 0) {
					assert(out_params[ix]->paramType == SA_IMM_ATTR_SASTRINGT);
					SaStringT errStr = (*((SaStringT *) out_params[ix]->paramBuffer));
					fprintf(stderr, "error-string: %s\n", errStr);
				}
				++ix;
			}

			/* After printing error string, print all returned parameters */
			if (verbose && out_params && out_params[0]) {
				if(!isFirst)
					printf("\n");

				print_params(argv[optind], out_params);
			}
			

			exit(EXIT_FAILURE);
		}


		if (((opName && (strncmp(opName,"display",7)==0))||verbose) &&out_params && out_params[0]) {
			if(!isFirst)
				printf("\n");
			else
				isFirst = 0;

			print_params(argv[optind], out_params);
		}


		error = saImmOmAdminOperationMemoryFree(ownerHandle, out_params);
		if (error != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmAdminOperationMemoryFree FAILED: %s\n", saf_error(error));
			exit(EXIT_FAILURE);
		}

		if(releaseAdmo) {
			error = immutil_saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE);
			if (error != SA_AIS_OK) {
				fprintf(stderr, "error - saImmOmAdminOwnerRelease FAILED: %s\n", saf_error(error));
				exit(EXIT_FAILURE);
			}
		}

		optind++;
	}

	error = immutil_saImmOmAdminOwnerFinalize(ownerHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmAdminOwnerFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	error = immutil_saImmOmFinalize(immHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmFinalize FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

