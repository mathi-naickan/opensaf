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
 * This file contains a command line utility to access IMM objects/attributes.
 * Example: immlist ..."
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
#include <time.h>
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
	fprintf(stderr, "error - immlist command timed out (alarm)\n");
	exit(EXIT_FAILURE);
}

static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - list IMM objects\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] <object name> [object name]\n", progname);
	printf("\t%s [options] <class name>\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is an IMM OM client used to print attributes of IMM objects.\n", progname);

	printf("\nOPTIONS\n");
	printf("\t-a, --attribute=NAME \n");
	printf("\t-c, --class - display class definition\n");
	printf("\t-h, --help - display this help and exit\n");
	printf("\t-p, --pretty-print=<yes|no> - select pretty print, default yes\n");
	printf("\t-t, --timeout <sec>\n");
	printf("\t\tutility timeout in seconds\n");

	printf("\nEXAMPLE\n");
	printf("\timmlist -a saAmfApplicationAdminState safApp=OpenSAF\n");
	printf("\timmlist safApp=myApp1 safApp=myApp2\n");
	printf("\timmlist --pretty-print=no -a saAmfAppType safApp=OpenSAF\n");
}

static void print_attr_value_raw(SaImmValueTypeT attrValueType, SaImmAttrValueT *attrValue)
{
    switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		printf("%d", *((SaInt32T *)attrValue));
		break;
	case SA_IMM_ATTR_SAUINT32T:
		printf("%u", *((SaUint32T *)attrValue));
		break;
	case SA_IMM_ATTR_SAINT64T:
		printf("%lld", *((SaInt64T *)attrValue));
		break;
	case SA_IMM_ATTR_SAUINT64T:
		printf("%llu", *((SaUint64T *)attrValue));
		break;
	case SA_IMM_ATTR_SATIMET:
		printf("%llu",  *((SaTimeT *)attrValue));
		break;
	case SA_IMM_ATTR_SAFLOATT:
		printf("%f", *((SaFloatT *)attrValue));
		break;
	case SA_IMM_ATTR_SADOUBLET:
		printf("%17.15f", *((SaDoubleT *)attrValue));
		break;
	case SA_IMM_ATTR_SANAMET: {
		SaNameT *myNameT = (SaNameT *)attrValue;
		printf("%s", myNameT->value);
		break;
	}
	case SA_IMM_ATTR_SASTRINGT:
		printf("%s", *((char **)attrValue));
		break;
	case SA_IMM_ATTR_SAANYT: {
		SaAnyT *anyp = (SaAnyT *)attrValue;
		unsigned int i = 0;
		if(anyp->bufferSize == 0) {
			printf("-empty-");
		} else {
			printf("0x");
			for (; i < anyp->bufferSize; i++)
			{
				if(((int) anyp->bufferAddr[i]) < 0x10) {
					printf("0");
				}
				printf("%x", (int)anyp->bufferAddr[i]);
			}
		}
		break;
	}
	default:
		printf("Unknown");
		break;
	}
}

static void print_attr_value(SaImmValueTypeT attrValueType, SaImmAttrValueT *attrValue)
{
	switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		printf("%d (0x%x)", *((SaInt32T *)attrValue), *((SaInt32T *)attrValue));
		break;
	case SA_IMM_ATTR_SAUINT32T:
		printf("%u (0x%x)", *((SaUint32T *)attrValue), *((SaUint32T *)attrValue));
		break;
	case SA_IMM_ATTR_SAINT64T:
		printf("%lld (0x%llx)", *((SaInt64T *)attrValue), *((SaInt64T *)attrValue));
		break;
	case SA_IMM_ATTR_SAUINT64T:
		printf("%llu (0x%llx)", *((SaUint64T *)attrValue), *((SaUint64T *)attrValue));
		break;
	case SA_IMM_ATTR_SATIMET:
		{
			char buf[32];
			const time_t time = *((SaTimeT *)attrValue) / SA_TIME_ONE_SECOND;

			ctime_r(&time, buf);
			buf[strlen(buf) - 1] = '\0';	/* Remove new line */
			printf("%llu (0x%llx, %s)", *((SaTimeT *)attrValue), *((SaTimeT *)attrValue), buf);
			break;
		}
	case SA_IMM_ATTR_SAFLOATT:
		printf("%f ", *((SaFloatT *)attrValue));
		break;
	case SA_IMM_ATTR_SADOUBLET:
		printf("%17.15f", *((SaDoubleT *)attrValue));
		break;
	case SA_IMM_ATTR_SANAMET:
		{
			SaNameT *myNameT = (SaNameT *)attrValue;
			printf("%s (%u) ", myNameT->value, myNameT->length);
			break;
		}
	case SA_IMM_ATTR_SASTRINGT:
		printf("%s ", *((char **)attrValue));
		break;
	case SA_IMM_ATTR_SAANYT: {
		SaAnyT *anyp = (SaAnyT *)attrValue;
		unsigned int i = 0;
		if(anyp->bufferSize) {
			printf("0x");
			for (; i < anyp->bufferSize; i++)
			{
				if(((int) anyp->bufferAddr[i]) < 0x10) {
					printf("0");
				}
				printf("%x", (int)anyp->bufferAddr[i]);
			}
		}
		printf(" size(%u)", (unsigned int) anyp->bufferSize);

		break;
	}
	default:
		printf("Unknown");
		break;
	}
}

static char *get_attr_type_name(SaImmValueTypeT attrValueType)
{
	switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		return "SA_INT32_T";
		break;
	case SA_IMM_ATTR_SAUINT32T:
		return "SA_UINT32_T";
		break;
	case SA_IMM_ATTR_SAINT64T:
		return "SA_INT64_T";
		break;
	case SA_IMM_ATTR_SAUINT64T:
		return "SA_UINT64_T";
		break;
	case SA_IMM_ATTR_SATIMET:
		return "SA_TIME_T";
		break;
	case SA_IMM_ATTR_SANAMET:
		return "SA_NAME_T";
		break;
	case SA_IMM_ATTR_SAFLOATT:
		return "SA_FLOAT_T";
		break;
	case SA_IMM_ATTR_SADOUBLET:
		return "SA_DOUBLE_T";
		break;
	case SA_IMM_ATTR_SASTRINGT:
		return "SA_STRING_T";
		break;
	case SA_IMM_ATTR_SAANYT:
		return "SA_ANY_T";
		break;
	default:
		return "Unknown";
		break;
	}
}

/**
 * Display a class definition in "SAF spec style"
 * @param className
 * @param immHandle
 */
static void display_class_definition(const SaImmClassNameT className,
	SaImmHandleT immHandle)
{
        SaImmClassCategoryT classCategory;
	SaAisErrorT error;
        SaImmAttrDefinitionT_2 **attrDefinitions;
	SaImmAttrDefinitionT_2 *attrDefinition;
	int i;

	error = immutil_saImmOmClassDescriptionGet_2(immHandle, className, &classCategory, &attrDefinitions);

	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmClassDescriptionGet_2 FAILED: %s\n", saf_error(error));
		exit(EXIT_FAILURE);
	}

	switch (classCategory) {
	case SA_IMM_CLASS_CONFIG:
		printf("\n<< %s - CONFIG >>\n", className);
		break;
	case SA_IMM_CLASS_RUNTIME:
		printf("\n<< %s - RUNTIME >>\n", className);
		break;
	default:
		fprintf(stderr, "Unknown class category\n");
		exit(EXIT_FAILURE);
		break;
	}

	for (i = 0, attrDefinition = attrDefinitions[i];
	      attrDefinition;
	      i++, attrDefinition = attrDefinitions[i]) {

		// Skip IMM added attributes
		if (strncmp(attrDefinition->attrName, "SaImmAttr", 9) == 0)
			continue;

		printf("%s : ", attrDefinition->attrName);
		printf("%s ", get_attr_type_name(attrDefinition->attrValueType));

		// Print multiplicity
		if (attrDefinition->attrFlags & SA_IMM_ATTR_INITIALIZED) {
			printf("[1");
		} else
			printf("[0");

		if (attrDefinition->attrFlags & SA_IMM_ATTR_MULTI_VALUE) {
			printf("..*]");
		} else
			printf("]");


		if (attrDefinition->attrFlags & SA_IMM_ATTR_CONFIG) {
			if (!(attrDefinition->attrFlags & SA_IMM_ATTR_INITIALIZED)) {
				printf(" = ");
				if (attrDefinition->attrDefaultValue == NULL) {
					printf("Empty");
				} else
					print_attr_value(attrDefinition->attrValueType, attrDefinition->attrDefaultValue);			
			}

			printf(" {");

			if (attrDefinition->attrFlags & SA_IMM_ATTR_RDN)
				printf("RDN, ");

			printf("CONFIG");

			if (attrDefinition->attrFlags & SA_IMM_ATTR_WRITABLE)
				printf(", WRITEABLE");

		} else if (attrDefinition->attrFlags & SA_IMM_ATTR_RUNTIME) {
			if (attrDefinition->attrDefaultValue != NULL) {
				printf(" = ");
				print_attr_value(attrDefinition->attrValueType, attrDefinition->attrDefaultValue);			
			}

			printf(" {");

			if (attrDefinition->attrFlags & SA_IMM_ATTR_RDN)
				printf("RDN, ");

			printf("RUNTIME");

			if (attrDefinition->attrFlags & SA_IMM_ATTR_CACHED)
				printf(", CACHED");

			if (attrDefinition->attrFlags & SA_IMM_ATTR_PERSISTENT)
				printf(", PERSISTENT");
		}

		printf("}\n");
	}

	(void)immutil_saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
}

static void display_object(const char *name,
						   SaImmAccessorHandleT accessorHandle,
						   int pretty_print,
						   const SaImmAttrNameT *attributeNames)
{
	int i = 0, j;
	SaImmAttrValuesT_2 *attr;
	SaNameT objectName;
	SaAisErrorT error;
	SaImmAttrValuesT_2 **attributes;

	strncpy((char *)objectName.value, name, SA_MAX_NAME_LENGTH);
	objectName.length = strlen((char *)objectName.value);

	error = immutil_saImmOmAccessorGet_2(accessorHandle, &objectName, attributeNames, &attributes);
	if (SA_AIS_OK != error) {
		if (error == SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "error - object or attribute does not exist\n");
		else
			fprintf(stderr, "error - saImmOmAccessorGet_2 FAILED: %s\n", saf_error(error));

		exit(EXIT_FAILURE);
	}

	if (pretty_print) {
		printf("%-50s %-12s Value(s)\n", "Name", "Type");
		printf("========================================================================");
		while ((attr = attributes[i++]) != NULL) {
			printf("\n%-50s %-12s ", attr->attrName, get_attr_type_name(attr->attrValueType));
			if (attr->attrValuesNumber > 0) {
				for (j = 0; j < attr->attrValuesNumber; j++)
					print_attr_value(attr->attrValueType, attr->attrValues[j]);
			} else
				printf("<Empty>");
		}
		printf("\n\n");
	} else {
		while ((attr = attributes[i++]) != NULL) {
			printf("%s=", attr->attrName);
			if (attr->attrValuesNumber > 0) {
				for (j = 0; j < attr->attrValuesNumber; j++) {
					print_attr_value_raw(attr->attrValueType, attr->attrValues[j]);
					if ((j + 1) < attr->attrValuesNumber)
						printf(":");
				}
				printf("\n");
			} else
				printf("<Empty>\n");
		}
	}
}

int main(int argc, char *argv[])
{
	int c;
	struct option long_options[] = {
		{"attribute", required_argument, 0, 'a'},
		{"class", no_argument, 0, 'c'},
		{"timeout", required_argument, 0, 't'},
		{"help", no_argument, 0, 'h'},
		{"pretty-print", required_argument, 0, 'p'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmAccessorHandleT accessorHandle;
	int len = 1;
	SaImmAttrNameT *attributeNames = NULL;
	int pretty_print = 1;
	int class_desc_print = 0;
	int rc = EXIT_SUCCESS;
	unsigned long timeoutVal = 60;

	while (1) {
		c = getopt_long(argc, argv, "a:p:t:ch", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'a':
			attributeNames = realloc(attributeNames, ++len * sizeof(SaImmAttrNameT));
			attributeNames[len - 2] = strdup(optarg);
			attributeNames[len - 1] = NULL;
			pretty_print = 0;
			break;
		case 'c':
			class_desc_print = 1;
			break;
		case 't':
			timeoutVal = strtol(optarg, (char **)NULL, 10);
			break;
		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		case 'p':
			if (!strcasecmp(optarg, "no"))
				pretty_print = 0;
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	signal(SIGALRM, sigalarmh);
	alarm(timeoutVal);

	/* Need at least one object to operate on */
	if ((argc - optind) == 0) {
		fprintf(stderr, "error - wrong number of arguments\n");
		rc = EXIT_FAILURE;
		goto done;
	}

	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.nTries = timeoutVal;
	immutilWrapperProfile.retryInterval = 1000;

	error = immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmInitialize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
		goto done;
	}

	if (class_desc_print) {
		while (optind < argc) {
			display_class_definition(argv[optind], immHandle);
			optind++;
		}
	} else {
		error = immutil_saImmOmAccessorInitialize(immHandle, &accessorHandle);
		if (SA_AIS_OK != error) {
			fprintf(stderr, "error - saImmOmAccessorInitialize FAILED: %s\n", saf_error(error));
			rc = EXIT_FAILURE;
			goto done_finalize;
		}

		/* Remaining arguments should be object names to print attributes for. */
		while (optind < argc) {
			display_object(argv[optind], accessorHandle, pretty_print, attributeNames);
			optind++;
		}

		error = immutil_saImmOmAccessorFinalize(accessorHandle);
		if (SA_AIS_OK != error) {
			fprintf(stderr, "error - saImmOmAccessorFinalize FAILED: %s\n", saf_error(error));
			rc = EXIT_FAILURE;
			goto done_finalize;
		}
	}

done_finalize:
	error = immutil_saImmOmFinalize(immHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmFinalize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
	}

done:
	exit(rc);
}
