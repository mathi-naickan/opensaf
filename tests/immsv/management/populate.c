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
 * This file contains a command line utility to populate IMM with configuration objects.
 * A sample class to load using immcfg -f could be:
 * 

<?xml version="1.0" encoding="utf-8"?>
<imm:IMM-contents xmlns:imm="http://www.saforum.org/IMMSchema" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:noNamespaceSchemaLocation="SAI-AIS-
IMM-XSD-A.01.01.xsd">
        <class name="TestClass">
                <category>SA_CONFIG</category>
                <rdn>
                        <name>rdn</name>
                        <type>SA_STRING_T</type>
                        <category>SA_CONFIG</category>
                        <flag>SA_INITIALIZED</flag>
                </rdn>
        </class>
</imm:IMM-contents>

 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <ctype.h>
#include <libgen.h>
#include <assert.h>

#include <saAis.h>
#include <saImmOm.h>
#include <immutil.h>
#include <saf_error.h>

int verbose = 0;
int ccb_safe = 0;

#define VERBOSE_INFO(format, args...) if (verbose) { fprintf(stderr, format, ##args); }

static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - populate IMM with configuration object(s)\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] <class name>\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is an IMM OM test client used to populate IMM with a specified number of object(s)\n", progname);
	printf("\tThe objects created are all of the same class and their DNs are automatically created.");

	printf("\nOPTIONS\n");
	printf("\t-c, --class                   output a suitable class definition to stdout\n");
	printf("\t-h, --help                    this help\n");
	printf("\t-v, --verbose                 enable debug printouts\n");
	printf("\t-p, --populate <cardinality>  create <cardinality> number of objects of class <class name>\n");

	printf("\nEXAMPLE\n");
	printf("\t%s -c > testclass.xml\n", progname);
	printf("\t%s -p 1000 TestClass\n", progname);
	printf("\t\tcreate 1000 instances of TestClass\n");
}

static char *create_adminOwnerName(char *base){
	char hostname[HOST_NAME_MAX];
	char *unique_adminOwner = malloc(HOST_NAME_MAX+10+strlen(base)+5);

	if (gethostname(hostname, sizeof(hostname)) != 0){
		fprintf(stderr, "error while retrieving hostname\n");
		exit(EXIT_FAILURE);
	}
	sprintf(unique_adminOwner, "%s_%s_%d", base, hostname, getpid());
	return unique_adminOwner;
}

typedef struct subtree_range {
	SaNameT parentDn;
	struct subtree_range* sublevel[100];
	unsigned int botRdn;
	unsigned int topRdn;
} range_obj_t;

static range_obj_t *gen_pop_tree(unsigned int level, range_obj_t* rootObj, unsigned int rdn, unsigned int base,
	unsigned int reminder, SaImmAttrNameT rdnAttName)
{
	int ix;
	range_obj_t *rangeObj = calloc(1, sizeof(range_obj_t));
	unsigned int partition;

	if (level > 2) {
		fprintf(stderr, "error - Level > 2 !!!!\n");
		exit(EXIT_FAILURE);
	}

	if (level == 0) {
		assert(!rootObj && !rdn && base==1);
		strncpy((char *) rangeObj->parentDn.value, rdnAttName, SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, "=", SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, "0", SA_MAX_NAME_LENGTH);
		rangeObj->parentDn.length = strlen((char *) rangeObj->parentDn.value);
	} else {
		rangeObj->parentDn.length = snprintf((char *) rangeObj->parentDn.value,
			sizeof(rangeObj->parentDn.value), "%s=%u,%s",
			rdnAttName, rdn, (char *) rootObj->parentDn.value);
	}

	rangeObj->botRdn = base;

	--reminder;

	if (reminder < 100) {
		rangeObj->topRdn = base + reminder;
		reminder = 0;
	} else {
		rangeObj->topRdn = base + 99;
		reminder -= 100;
	}

	VERBOSE_INFO("Generated level %u rangeObj(%u) %u<-->%u %s\n", level, rdn, rangeObj->botRdn, rangeObj->topRdn,
		rangeObj->parentDn.value);

	partition = (reminder > 10001) ? 10001 : 100;

	ix=0;
	while (reminder && (ix < 100)) {

		unsigned int subreminder = (reminder < partition)?reminder:partition;
		rangeObj->sublevel[ix] = gen_pop_tree(level + 1, rangeObj, /* rdn: */ base + ix, 
			/*new base:*/ base + 100 + (ix)*partition, subreminder, rdnAttName);
		reminder -= subreminder;
		++ix;
	}

	if (reminder) {
		fprintf(stderr, "error - Returning with nonzero reminder r:%u ix:%u\n", reminder, ix);
		abort();
	}

	return rangeObj;
}

static void ccb_create_obj(SaNameT* parentDn, SaNameT* rdnVal, SaImmCcbHandleT ccbHandle, 
	const SaImmClassNameT className, SaImmAttrNameT rdnAttName, SaImmValueTypeT rdnAttType) 
{
	unsigned int retries=0;
	SaAisErrorT err = SA_AIS_OK;
	SaImmAttrValueT theValue;
	SaStringT str = (SaStringT) rdnVal->value;

	if (rdnAttType == SA_IMM_ATTR_SANAMET) {
		theValue = rdnVal;
	} else {
		assert(rdnAttType == SA_IMM_ATTR_SASTRINGT);
		theValue = &str;
	}

	SaImmAttrValuesT_2 v1 = {rdnAttName, rdnAttType, 1, &theValue};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, NULL};

	do {
		err = saImmOmCcbObjectCreate_2(ccbHandle, className, parentDn, attrValues);
		if (err == SA_AIS_ERR_TRY_AGAIN) {
			usleep(250 * 1000);
		}
	} while ((err == SA_AIS_ERR_TRY_AGAIN) && (retries < 15));

	if (err != SA_AIS_OK) {
		if ((err == SA_AIS_ERR_NOT_EXIST) && ccb_safe) {
			fprintf(stderr, "Missing: implementer, or object, or attribute "
				"(see immcfg -h under '--unsafe')\n");
		}

		fprintf(stderr, "error - Failed to create object parent%s rdn:%s error:%u\n", 
			(parentDn && parentDn->length)?(char *) parentDn->value:NULL, 
			(rdnVal->length)?(char *) rdnVal->value:NULL, err);
		exit(1);
	}
}

static void generate_pop(range_obj_t* rootObj, SaImmCcbHandleT ccbHandle, const SaImmClassNameT className,
	SaImmAttrNameT rdnAttName, SaImmValueTypeT rdnAttType, SaImmAdminOwnerHandleT ownerHandle)
{
	SaAisErrorT err = SA_AIS_OK;
	unsigned int ix;
	unsigned int rdn = rootObj->botRdn;
	SaNameT rdnAttrVal;
	unsigned int retries = 0;
	const SaNameT* objectNames[] = {&(rootObj->parentDn), NULL};

	for (ix = 0; ix < 100 && rdn <= rootObj->topRdn; ++ix, ++rdn) {
		rdnAttrVal.length = snprintf((char *) rdnAttrVal.value,
			sizeof(rdnAttrVal.value), "%s=%u", rdnAttName, rdn);

		ccb_create_obj(&rootObj->parentDn, &rdnAttrVal, ccbHandle, className, rdnAttName, rdnAttType);
	}

	do {
		err = saImmOmCcbApply(ccbHandle);
		if(err == SA_AIS_ERR_TRY_AGAIN) {
			usleep(250 * 1000);
		}
	} while ((err == SA_AIS_ERR_TRY_AGAIN) && (retries < 15));

	if (err != SA_AIS_OK) {
		fprintf(stderr, "error - Failed to apply ccb for parent:%s range %u<->%u, error:%s\n",
			(char *) rootObj->parentDn.value, rootObj->botRdn, rootObj->topRdn, saf_error(err));
		exit(1);
	}

	VERBOSE_INFO("successfull Apply for parent %s range %u<->%u\n", 
		(char *) rootObj->parentDn.value, rootObj->botRdn, rootObj->topRdn);

	for (ix = 0; ix < 100 && rootObj->sublevel[ix]; ++ix) {
		generate_pop(rootObj->sublevel[ix], ccbHandle, className, rdnAttName, rdnAttType, ownerHandle);
	}

	err = saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE);
	if (err != SA_AIS_OK) {
		fprintf(stderr, "error - Failed to release admo - ignoring\n");
	}
}

static int populate_imm(const SaImmClassNameT className, 
	unsigned int pop, SaImmAdminOwnerHandleT ownerHandle, SaImmHandleT immHandle)
{
	SaAisErrorT error;
	int i;
	int rc = EXIT_FAILURE;
	SaImmCcbHandleT ccbHandle;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;
	SaImmAttrDefinitionT_2 *att;
	SaImmAttrNameT rdnAttName = NULL;
	SaImmValueTypeT rdnAttType = SA_IMM_ATTR_SAANYT;
	range_obj_t *rootObj = NULL;

	if ((error = saImmOmClassDescriptionGet_2(immHandle, className, &classCategory, &attrDefinitions)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmClassDescriptionGet_2 FAILED: %s\n", saf_error(error));
		goto done;
	}

	if (classCategory == SA_IMM_CLASS_RUNTIME) {
		fprintf(stderr, "error - Class %s is a runtime class\n", className);
		goto done;		
	}

	for (i = 0, att = attrDefinitions[i]; att != NULL; att = attrDefinitions[++i]) {
		if (att->attrFlags & SA_IMM_ATTR_RDN) {
			rdnAttName = att->attrName;
			rdnAttType = att->attrValueType;
		} else if (att->attrFlags & SA_IMM_ATTR_INITIALIZED) {
			fprintf(stderr, "error - Attribute %s has INITIALIZED flag, cant handle\n", att->attrName);
			goto done;
		}
		VERBOSE_INFO("attrName: %s\n", att->attrName);
	}

	if (!rdnAttName) {
		fprintf(stderr, "error - Could not find any RDN attribure\n");
		goto done;
	}

	VERBOSE_INFO("Rdn attrName:%s type:%s\n", rdnAttName, (rdnAttType==SA_IMM_ATTR_SASTRINGT)?"SA_STRINGT":
		(rdnAttType==SA_IMM_ATTR_SANAMET)?"SA_NAMET":"WRONG");

	if ((error = saImmOmCcbInitialize(ownerHandle, (ccb_safe ? SA_IMM_CCB_REGISTERED_OI : 0x0), &ccbHandle))
		!= SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
		goto done;
	}

	VERBOSE_INFO("className: %s po:%u\n", className, pop);

	rootObj = gen_pop_tree(0, NULL, 0, 1, pop, rdnAttName);

	ccb_create_obj(NULL, &rootObj->parentDn, ccbHandle, className, rdnAttName, rdnAttType);

	generate_pop(rootObj, ccbHandle, className, rdnAttName, rdnAttType, ownerHandle);

	if ((error = saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
		goto done;
	}

	rc = 0;

done:
	return rc;
}

static void output_testclass(void)
{
	puts(
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<imm:IMM-contents xmlns:imm=\"http://www.saforum.org/IMMSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\""
		" xsi:noNamespaceSchemaLocation=\"SAI-AIS-IMM-XSD-A.01.01.xsd\">\n"
		"	<class name=\"TestClass\">\n"
		"		<category>SA_CONFIG</category>\n"
                "		<rdn>\n"
		"			<name>testClass</name>\n"
                "			<type>SA_STRING_T</type>\n"
		"			<category>SA_CONFIG</category>\n"
		"			<flag>SA_INITIALIZED</flag>\n"
		"		</rdn>\n"
		"	</class>\n"
		"</imm:IMM-contents>");
}

int main(int argc, char *argv[])
{
	int rc = EXIT_SUCCESS;
	int c;
	struct option long_options[] = {
		{"class", no_argument, NULL, 'c'},
		{"help", no_argument, NULL, 'h'},
		{"populate", required_argument, NULL, 'p'},
		{"verbose", no_argument, NULL, 'v'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmAdminOwnerNameT adminOwnerName = create_adminOwnerName(basename(argv[0]));
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmClassNameT className = NULL;
	static SaVersionT immVersion = { 'A', 2, 11 };
	int population = 0;

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "chp:v", long_options, &option_index);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'c':
			output_testclass();
			exit(EXIT_SUCCESS);
			break;
		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		case 'p':
			population = atol(optarg);
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

	/* The remaining argument needs to be a class name */
	if ((argc - optind) != 1) {
		fprintf(stderr, "error - need to specify a class name\n");
		exit(EXIT_FAILURE);
	}

	className = strdup(argv[optind]);

	(void)immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);

	error = saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, &ownerHandle);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerInitialize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
		goto done_om_finalize;
	}

	rc = populate_imm(className, population, ownerHandle, immHandle);

	error = saImmOmAdminOwnerFinalize(ownerHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmAdminOwnerFinalize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
	}

 done_om_finalize:
	(void)immutil_saImmOmFinalize(immHandle);

	exit(rc);
}
