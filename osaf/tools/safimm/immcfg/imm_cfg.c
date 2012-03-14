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
 * This file contains a command line utility to configure attributes for an IMM object.
 * Example: immcfg [-a attr-name[+|-]=attr-value]+ "safAmfNode=Node01,safAmfCluster=1"
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
#include "ncsgl_defs.h"
//#ifdef HAVE_CONFIG_H
//#include <config.h>
//#endif

//#define HAVE_TESTS

static SaVersionT immVersion = { 'A', 2, 1 };
int verbose = 0;
int population = 0;

typedef enum {
	INVALID = 0,
	CREATE_OBJECT = 1,
	DELETE_OBJECT = 2,
	DELETE_CLASS = 3,
	MODIFY_OBJECT = 4,
	LOAD_IMMFILE = 5,
	POPULATE_IMM = 6
} op_t;

#define VERBOSE_INFO(format, args...) if (verbose) { fprintf(stderr, format, ##args); }

// The interface function which implements the -f opton (imm_import.cc)
int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ignore_duplicates);


/**
 *
 */
static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - create, delete or modify IMM configuration object(s)\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [object DN]...\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is an IMM OM client used to create, delete an IMM or modify attribute(s) for IMM object(s)\n", progname);
	printf("\tThe default operation if none specified is modify.\n");
	printf("\tWhen creating or modifying several objects, they have to be of the same class.");

	printf("\nOPTIONS\n");
	printf("\t-a, --attribute name[+|-]=value [object DN]... \n");
	printf("\t-c, --create-object <class name> [object DN]... \n");
	printf("\t-d, --delete-object [object DN]... \n");
	printf("\t-h, --help                    this help\n");
	printf("\t-m, --modify-object [object DN]... \n");
	printf("\t-v, --verbose (only valid with -f/--file option)\n");
	printf("\t-f, --file <imm.xml file containing classes and/or objects>\n");
	printf("\t--ignore-duplicates  (only valid with -f/--file option)\n");
	printf("\t--delete-class <classname> [classname2]... \n");
	printf("\t-p, --populate <cardinality> <classname>  (requires 'configure --enable-tests')\n");

	printf("\nEXAMPLE\n");
	printf("\timmcfg -a saAmfNodeSuFailoverMax=7 safAmfNode=Node01,safAmfCluster=1\n");
	printf("\t\tchange one attribute for one object\n");
	printf("\timmcfg -c SaAmfApplication -a saAmfAppType=Test safApp=myTestApp1\n");
	printf("\t\tcreate one object setting one initialized attribute\n");
	printf("\timmcfg -d safAmfNode=Node01,safAmfCluster=1\n");
	printf("\t\tdelete one object\n");
	printf("\timmcfg -d safAmfNode=Node01,safAmfCluster=1 safAmfNode=Node02,safAmfCluster=1\n");
	printf("\t\tdelete two objects\n");
	printf("\timmcfg -a saAmfNGNodeList+=safAmfNode=PL_2_6,safAmfCluster=myAmfCluster safAmfNodeGroup=PLs,safAmfCluster=myAmfCluster\n");
	printf("\t\tadd a value to an attribute\n");
	printf("\timmcfg -a saAmfNGNodeList-=safAmfNode=PL_2_6,safAmfCluster=myAmfCluster safAmfNodeGroup=PLs,safAmfCluster=myAmfCluster\n");
	printf("\t\tremove a value from an attribute\n");
}

/**
 * Alloc SaImmAttrModificationT_2 object and initialize its attributes from nameval (x=y)
 * @param objectName
 * @param nameval
 *
 * @return SaImmAttrModificationT_2*
 */
static SaImmAttrModificationT_2 *new_attr_mod(const SaNameT *objectName, char *nameval)
{
	int res = 0;
	char *tmp = strdup(nameval);
	char *name, *value;
	SaImmAttrModificationT_2 *attrMod = NULL;
	SaImmClassNameT className = immutil_get_className(objectName);
	SaAisErrorT error;
	SaImmAttrModificationTypeT modType = SA_IMM_ATTR_VALUES_REPLACE;

	if (className == NULL) {
		fprintf(stderr, "Object with DN '%s' does not exist\n", objectName->value);
		res = -1;
		goto done;
	}

	attrMod = malloc(sizeof(SaImmAttrModificationT_2));

	if ((value = strstr(tmp, "=")) == NULL) {
		res = -1;
		goto done;
	}

	if (value[-1] == '+') {
		modType = SA_IMM_ATTR_VALUES_ADD;
		value[-1] = 0;
	}
	else if (value[-1] == '-') {
		modType = SA_IMM_ATTR_VALUES_DELETE;
		value[-1] = 0;
	}

	name = tmp;
	*value = '\0';
	value++;

	error = immutil_get_attrValueType(className, name, &attrMod->modAttr.attrValueType);
	if (error == SA_AIS_ERR_NOT_EXIST) {
		fprintf(stderr, "Class '%s' does not exist\n", className);
		res = -1;
		goto done;
	}

	if (error != SA_AIS_OK) {
		fprintf(stderr, "Attribute '%s' does not exist in class '%s'\n", name, className);
		res = -1;
		goto done;
	}

	attrMod->modType = modType;
	attrMod->modAttr.attrName = name;
	if (strlen(value)) {
		attrMod->modAttr.attrValuesNumber = 1;
		attrMod->modAttr.attrValues = malloc(sizeof(SaImmAttrValueT *));
		attrMod->modAttr.attrValues[0] = immutil_new_attrValue(attrMod->modAttr.attrValueType, value);
		if (attrMod->modAttr.attrValues[0] == NULL)
			res = -1;
	} else {
		attrMod->modAttr.attrValuesNumber = 0;
		attrMod->modAttr.attrValues = NULL;
	}
	
 done:
	free(className);
	if (res != 0) {
		free(attrMod);
		attrMod = NULL;
	}
	return attrMod;
}

/**
 * Alloc SaImmAttrValuesT_2 object and initialize its attributes from nameval (x=y)
 * @param className
 * @param nameval
 *
 * @return SaImmAttrValuesT_2*
 */
static SaImmAttrValuesT_2 *new_attr_value(const SaImmClassNameT className, char *nameval, int isRdn)
{
	int res = 0;
	char *name = strdup(nameval), *p;
	char *value;
	SaImmAttrValuesT_2 *attrValue;
	SaAisErrorT error;

	attrValue = malloc(sizeof(SaImmAttrValuesT_2));

	p = strchr(name, '=');
	if (p == NULL){
		fprintf(stderr, "The Attribute '%s' does not contain a equal sign ('=')\n", nameval);
		res = -1;
		goto done;
	}
	*p = '\0';
	value = p + 1;

	attrValue->attrName = strdup(name);
	VERBOSE_INFO("new_attr_value attrValue->attrName: '%s' value:'%s'\n", attrValue->attrName, isRdn ? nameval : value);

	error = immutil_get_attrValueType(className, attrValue->attrName, &attrValue->attrValueType);

	if (error == SA_AIS_ERR_NOT_EXIST) {
		fprintf(stderr, "Class '%s' does not exist\n", className);
		res = -1;
		goto done;
	}

	if (error != SA_AIS_OK) {
		fprintf(stderr, "Attribute '%s' does not exist in class '%s'\n", name, className);
		res = -1;
		goto done;
	}

	attrValue->attrValuesNumber = 1;
	attrValue->attrValues = malloc(sizeof(SaImmAttrValueT *));
	attrValue->attrValues[0] = immutil_new_attrValue(attrValue->attrValueType, isRdn ? nameval : value);
	if(attrValue->attrValues[0] = NULL)
		res = -1;

 done:
	free(name);
	if (res != 0) {
		free(attrValue);
		attrValue = NULL;
	}

	return attrValue;
}

/**
 * Create object(s) of the specified class, initialize attributes with values from optarg.
 *
 * @param objectNames
 * @param className
 * @param ownerHandle
 * @param optargs
 * @param optargs_len
 *
 * @return int
 */
int object_create(const SaNameT **objectNames, const SaImmClassNameT className,
	SaImmAdminOwnerHandleT ownerHandle, char **optargs, int optargs_len)
{
	SaAisErrorT error;
	int i;
	SaImmAttrValuesT_2 *attrValue;
	SaImmAttrValuesT_2 **attrValues = NULL;
	int attr_len = 1;
	int rc = EXIT_FAILURE;
	char *parent = NULL;
	SaNameT dn;
	SaNameT *parentName = NULL;
	char *str, *delim;
	const SaNameT *parentNames[] = {parentName, NULL};
	SaImmCcbHandleT ccbHandle;

	for (i = 0; i < optargs_len; i++) {
		attrValues = realloc(attrValues, (attr_len + 1) * sizeof(SaImmAttrValuesT_2 *));
		VERBOSE_INFO("object_create optargs[%d]: '%s'\n", i, optargs[i]);
		if ((attrValue = new_attr_value(className, optargs[i], 0)) == NULL){
			fprintf(stderr, "error - creating attribute from '%s'\n", optargs[i]);
			goto done;
		}

		attrValues[attr_len - 1] = attrValue;
		attrValues[attr_len] = NULL;
		attr_len++;
	}

	if ((error = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
		goto done;
	}

	i = 0;
	while (objectNames[i] != NULL) {
		str = strdup((char*)objectNames[i]->value);
		if ((delim = strchr(str, ',')) != NULL) {
			/* a parent exist */
			while (*(delim - 1) == 0x5c) {
				/* comma delimiter is escaped, search again */
				delim += 2;
				delim = strchr(delim, ',');
			}

			*delim = '\0';
			parent = delim + 1;
			if (!parent) {
				fprintf(stderr, "error - malformed object DN\n");
				goto done;
			}

			dn.length = sprintf((char*)dn.value, "%s", parent);
			parentName = &dn;
			parentNames[0] = parentName;

			VERBOSE_INFO("call saImmOmAdminOwnerSet for parent: %s\n", parent);
			if ((error = saImmOmAdminOwnerSet(ownerHandle, parentNames, SA_IMM_SUBTREE)) != SA_AIS_OK) {
				if (error == SA_AIS_ERR_NOT_EXIST)
					fprintf(stderr, "error - parent '%s' does not exist\n", dn.value);
				else {
					fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));
					goto done;
				}
			}
		}

		attrValues = realloc(attrValues, (attr_len + 1) * sizeof(SaImmAttrValuesT_2 *));
		VERBOSE_INFO("object_create rdn attribute attrValues[%d]: '%s' \n", attr_len - 1, str);
		if ((attrValue = new_attr_value(className, str, 1)) == NULL){
			fprintf(stderr, "error - creating rdn attribute from '%s'\n", str);
			goto done;
		}
		attrValues[attr_len - 1] = attrValue;
		attrValues[attr_len] = NULL;

		if ((error = saImmOmCcbObjectCreate_2(ccbHandle, className, parentName,
			(const SaImmAttrValuesT_2**)attrValues)) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbObjectCreate_2 FAILED with %s\n",
				saf_error(error));
			goto done;
		}
		i++;
	}

	if ((error = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbApply FAILED: %s\n", saf_error(error));
		goto done_release;
	}

	if ((error = saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
		goto done_release;
	}

	rc = 0;

done_release:
	/*  Skip explicit release of admin owner. It just causes problems when new objects
	    (e.g. runtime objects have appeared in the subtree. Instead just rely on 
	    releaseOwnershipOnFinalize being set to true in saImm*OmAdminOwnerinitialize. 
	if (parent && (error = saImmOmAdminOwnerRelease(
		ownerHandle, (const SaNameT **)parentNames, SA_IMM_SUBTREE)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerRelease FAILED: %s\n", saf_error(error));
		goto done;
	}
	*/
done:
	return rc;
}

/**
 * Modify object(s) with the attributes specifed in the optargs array
 *
 * @param objectNames
 * @param ownerHandle
 * @param optargs
 * @param optargs_len
 *
 * @return int
 */
int object_modify(const SaNameT **objectNames, SaImmAdminOwnerHandleT ownerHandle, char **optargs, int optargs_len)
{
	SaAisErrorT error;
	int i;
	int attr_len = 1;
	int rc = EXIT_FAILURE;
	SaImmAttrModificationT_2 *attrMod;
	SaImmAttrModificationT_2 **attrMods = NULL;
	SaImmCcbHandleT ccbHandle;

	for (i = 0; i < optargs_len; i++) {
		attrMods = realloc(attrMods, (attr_len + 1) * sizeof(SaImmAttrModificationT_2 *));
		if ((attrMod = new_attr_mod(objectNames[i], optargs[i])) == NULL)
			exit(EXIT_FAILURE);

		attrMods[attr_len - 1] = attrMod;
		attrMods[attr_len] = NULL;
		attr_len++;
	}

	if ((error = saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames, SA_IMM_ONE)) != SA_AIS_OK) {
		if (error == SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "error - object '%s' does not exist\n", objectNames[0]->value);
		else
			fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));

		goto done;
	}

	if ((error = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
		goto done_release;
	}

	i = 0;
	while (objectNames[i] != NULL) {
		if ((error = saImmOmCcbObjectModify_2(ccbHandle, objectNames[i],
			(const SaImmAttrModificationT_2 **)attrMods)) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbObjectModify_2 FAILED: %s\n", saf_error(error));
			goto done_release;
		}
		i++;
	}

	if ((error = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbApply FAILED: %s\n", saf_error(error));
		goto done_release;
	}

	if ((error = saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
		goto done_release;
	}

	rc = 0;

 done_release:
	if ((error = saImmOmAdminOwnerRelease(ownerHandle, (const SaNameT **)objectNames, SA_IMM_ONE)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerRelease FAILED: %s\n", saf_error(error));
		goto done;
	}
 done:
	return rc;
}

/**
 * Delete object(s) in the NULL terminated array using one CCB.
 * @param objectNames
 * @param ownerHandle
 *
 * @return int
 */
int object_delete(const SaNameT **objectNames, SaImmAdminOwnerHandleT ownerHandle)
{
	SaAisErrorT error;
	int rc = EXIT_FAILURE;
	SaImmCcbHandleT ccbHandle;
	int i = 0;

	if ((error = saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames,
		SA_IMM_SUBTREE)) != SA_AIS_OK) {

		if (error == SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "error - object does not exist\n");
		else
			fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));

		goto done;
	}

	if ((error = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
		goto done;
	}

	while (objectNames[i] != NULL) {
		if ((error = saImmOmCcbObjectDelete(ccbHandle, objectNames[i])) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbObjectDelete for '%s' FAILED: %s\n",
				objectNames[i]->value, saf_error(error));
			goto done;
		}
		i++;
	}

	if ((error = saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbApply FAILED: %s\n", saf_error(error));
		goto done;
	}

	if ((error = saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
		goto done;
	}

	rc = 0;
done:
	return rc;
}

/**
 * Delete class(es) in the NULL terminated array
 * @param classNames
 * @param ownerHandle
 *
 * @return int
 */
int class_delete(const SaImmClassNameT *classNames, SaImmHandleT immHandle)
{
	SaAisErrorT error;
	int rc = EXIT_FAILURE;
	int i = 0;

	while (classNames[i] != NULL) {
		if ((error = saImmOmClassDelete(immHandle, classNames[i])) != SA_AIS_OK) {
			if (error == SA_AIS_ERR_NOT_EXIST)
				fprintf(stderr, "error - class does not exist :%s\n", classNames[i]);
			else
				fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));

			goto done;
		}
		i++;
	}

	rc = 0;
done:
	return rc;
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


static op_t verify_setoption(op_t prevValue, op_t newValue)
{
	if (prevValue == INVALID)
		return newValue;
	else {
		fprintf(stderr, "error - only one operation at a time supported\n");
		exit(EXIT_FAILURE);
	}
}

#ifdef HAVE_TESTS
typedef struct subtree_range {
	SaNameT parentDn;
	struct subtree_range* sublevel[100];
	unsigned int botRdn;
	unsigned int topRdn;
} range_obj;

static range_obj * gen_pop_tree(unsigned int level, range_obj* rootObj, unsigned int rdn, unsigned int base,
	unsigned int remainder, SaImmAttrNameT rdnAttName)
{
	//printf("gen_pop_tree(level:%u, range_obj:%p, rdn:%u, remainder:%u\n", level, rootObj, rdn, remainder);
	if(level > 2) {
		fprintf(stderr, "error - Level > 2 !!!!\n");
		abort();
	}
	int ix;
	range_obj* rangeObj = malloc(sizeof(range_obj));
	memset(rangeObj, '\0', sizeof(range_obj));
	--remainder;
	//printf("Decremented remainder to: %u\n", remainder);
	unsigned int partition = 0;
	if(level == 0) {
		assert(!rootObj && !rdn && base==1);
		strncpy((char *) rangeObj->parentDn.value, rdnAttName, SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, "=", SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, "0", SA_MAX_NAME_LENGTH);
		rangeObj->parentDn.length = strlen((char *) rangeObj->parentDn.value);
	} else {
		char rdnBuf[16];
		sprintf(rdnBuf, "%u", rdn);
		strncpy((char *) rangeObj->parentDn.value, rdnAttName, SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, "=", SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value,	rdnBuf, SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, ",", SA_MAX_NAME_LENGTH);
		strncat((char *) rangeObj->parentDn.value, (char *) rootObj->parentDn.value, SA_MAX_NAME_LENGTH);
		rangeObj->parentDn.length = strlen((char *) rangeObj->parentDn.value);
	}

	rangeObj->botRdn = base;

	if(remainder < 100) {
		rangeObj->topRdn = base + remainder;
		remainder = 0;
	} else {
		rangeObj->topRdn = base + 99;
		remainder -= 100;
	}

	printf("Generated level %u rangeObj(%u) %u<-->%u %s\n", level, rdn, rangeObj->botRdn, rangeObj->topRdn,
		rangeObj->parentDn.value);

	partition = (remainder > 10001) ? 10001 : 100;

	ix=0;
	while(remainder && (ix < 100)) {

		unsigned int subremainder = (remainder < partition)?remainder:partition;
		rangeObj->sublevel[ix] = gen_pop_tree(level + 1, rangeObj, /* rdn: */ base + ix, 
			/*new base:*/ base + 100 + (ix)*partition, subremainder, rdnAttName);
		remainder -= subremainder;
		++ix;
	}

	if(remainder) {
		fprintf(stderr, "error - Returning with nonzero remainder r:%u ix:%u\n", remainder, ix);
		abort();
	}

	return rangeObj;
}

static void ccb_create_obj(SaNameT* parentDn, SaNameT* rdnVal, SaImmCcbHandleT ccbHandle, 
	const SaImmClassNameT className, SaImmAttrNameT rdnAttName, SaImmValueTypeT rdnAttType) 
{
	/*printf("Entering ccb_create_obj parentDn->value: %s\n", 
	  (parentDn && parentDn->length)?parentDn->value:NULL);*/
	unsigned int retries=0;
	SaAisErrorT err = SA_AIS_OK;
	SaImmAttrValueT theValue;
	if(rdnAttType == SA_IMM_ATTR_SANAMET) {
		theValue = rdnVal;
	} else {
		SaStringT str = (SaStringT) rdnVal->value;
		assert(rdnAttType == SA_IMM_ATTR_SASTRINGT);
		theValue = &str;
	}
	SaImmAttrValuesT_2 v1 = {rdnAttName, rdnAttType, 1, &theValue};
	const SaImmAttrValuesT_2 * attrValues[] = {&v1, NULL};


	do {
		err = saImmOmCcbObjectCreate_2(ccbHandle, className, parentDn, attrValues);
		if(err == SA_AIS_ERR_TRY_AGAIN) {
			usleep(250 * 1000);
		}
	} while ((err == SA_AIS_ERR_TRY_AGAIN) && (retries < 15));

	/*printf("saImmOmCcbObjectCreate_2 returned err == %u\n", err);*/
	if(err != SA_AIS_OK) {
		fprintf(stderr, "error - Failed to create object parent%s rdn:%s error:%u\n", 
			(parentDn && parentDn->length)?(char *) parentDn->value:NULL, 
			(rdnVal->length)?(char *) rdnVal->value:NULL, err);
		exit(1);
	}
	/*printf("Leaving ccb_create_obj\n");*/
}

static void generate_pop(range_obj* rootObj, SaImmCcbHandleT ccbHandle, const SaImmClassNameT className,
	SaImmAttrNameT rdnAttName, SaImmValueTypeT rdnAttType, SaImmAdminOwnerHandleT ownerHandle)
{
	SaAisErrorT err = SA_AIS_OK;
	unsigned int ix=0;
	unsigned int rdn = rootObj->botRdn;
	SaNameT rdnAttrVal;
	char rdnBuf[16];
	unsigned int retries=0;
	const SaNameT* objectNames[] = {&(rootObj->parentDn), NULL};

	/*err = saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE);*/

	for(; ix<100 && rdn <= rootObj->topRdn; ++ix, ++rdn) {
		sprintf(rdnBuf, "%u", rdn);
		strncpy((char *) rdnAttrVal.value, rdnAttName, SA_MAX_NAME_LENGTH);
		strncat((char *) rdnAttrVal.value, "=", SA_MAX_NAME_LENGTH);
		strncat((char *) rdnAttrVal.value, rdnBuf, SA_MAX_NAME_LENGTH);
		rdnAttrVal.length = strlen((char *) rdnAttrVal.value);
		ccb_create_obj(&rootObj->parentDn, &rdnAttrVal, ccbHandle, className, rdnAttName, rdnAttType);
	}

	do {
		err = saImmOmCcbApply(ccbHandle);
		if(err == SA_AIS_ERR_TRY_AGAIN) {
			usleep(250 * 1000);
		}
	} while ((err == SA_AIS_ERR_TRY_AGAIN) && (retries < 15));

	if(err != SA_AIS_OK) {
		fprintf(stderr, "error - Failed to apply ccb for parent:%s range %u<->%u, error:%u\n",
			(char *) rootObj->parentDn.value, rootObj->botRdn, rootObj->topRdn, err);
		exit(1);
	}

	printf("successfull Apply for parent %s range %u<->%u\n", 
		(char *) rootObj->parentDn.value, rootObj->botRdn, rootObj->topRdn);

	for(ix=0; ix<100 && rootObj->sublevel[ix]; ++ix) {
		generate_pop(rootObj->sublevel[ix], ccbHandle, className, rdnAttName, rdnAttType, ownerHandle);
	}

	err = saImmOmAdminOwnerRelease(ownerHandle, objectNames, SA_IMM_ONE);
	if(err != SA_AIS_OK) {
		fprintf(stderr, "error - Failed to release admo - ignoring\n");
	}
}

int populate_imm(const SaImmClassNameT className, unsigned int pop, SaImmAdminOwnerHandleT ownerHandle, SaImmHandleT immHandle)
{
	SaAisErrorT error;
	int i=0;
	int rc = EXIT_FAILURE;
	SaImmCcbHandleT ccbHandle;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2** attrDefinitions;
	SaImmAttrDefinitionT_2* att;
	SaImmAttrNameT rdnAttName=NULL;
	SaImmValueTypeT rdnAttType=SA_IMM_ATTR_SAANYT;
	range_obj* rootObj = NULL;

	/*
	SaImmAttrValuesT_2 *attrValue;
	SaImmAttrValuesT_2 **attrValues = NULL;
	char *parent = NULL;
	SaNameT parentName;
	const SaNameT *parentNames[] = {&parentName, NULL};
	*/

	if((error = saImmOmClassDescriptionGet_2(immHandle, className, &classCategory, &attrDefinitions)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmClassDescriptionGet_2 FAILED: %s\n", saf_error(error));
		goto done;
	}

	if(classCategory == SA_IMM_CLASS_RUNTIME) {
		fprintf(stderr, "error - Class %s is a runtime class\n", className);
		goto done;		
	}

	for(att=attrDefinitions[i]; att!=NULL; att=attrDefinitions[++i]) {
		if(att->attrFlags & SA_IMM_ATTR_RDN) {
			rdnAttName = att->attrName;
			rdnAttType = att->attrValueType;
		} else if(att->attrFlags & SA_IMM_ATTR_INITIALIZED) {
			fprintf(stderr, "error - Attribute %s has INITIALIZED flag, cant handle\n", att->attrName);
			goto done;
		}
		printf("attrName: %s\n", att->attrName);
	}
	if(!rdnAttName) {
		fprintf(stderr, "error - Could not find any RDN attribure\n");
		goto done;
	}

	printf("Rdn attrName:%s type:%s\n", rdnAttName, (rdnAttType==SA_IMM_ATTR_SASTRINGT)?"SA_STRINGT":
		(rdnAttType==SA_IMM_ATTR_SANAMET)?"SA_NAMET":"WRONG");

	if ((error = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
		goto done;
	}

	printf("className: %s po:%u\n", className, pop);
	if(pop > 1000000) {
		fprintf(stderr, "error - Can not handle more than 1000000 objects\n");
		goto done;
	}

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
#endif

int main(int argc, char *argv[])
{
	int rc = EXIT_SUCCESS;
	int c;
	struct option long_options[] = {
		{"attribute", required_argument, NULL, 'a'},
		{"create-object", required_argument, NULL, 'c'},
		{"file", required_argument, NULL, 'f'},
		{"ignore-duplicates", no_argument, NULL, 0},
		{"delete-class", no_argument, NULL, 0},    /* Note: should be 'no_arg'! treated as "Remaining args" below*/
		{"delete-object", no_argument, NULL, 'd'},
		{"help", no_argument, NULL, 'h'},
		{"modify-object", no_argument, NULL, 'm'},
		{"verbose", no_argument, NULL, 'v'},
		{"populate", required_argument, NULL, 'p'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	SaImmHandleT immHandle;
	SaImmAdminOwnerNameT adminOwnerName = create_adminOwnerName(basename(argv[0]));
	SaImmAdminOwnerHandleT ownerHandle;
	SaNameT **objectNames = NULL;
	int objectNames_len = 1;

	SaImmClassNameT *classNames = NULL;
	int classNames_len = 1;

	SaNameT *objectName;
	int optargs_len = 0;	/* one off */
	char **optargs = NULL;
	SaImmClassNameT className = NULL;
	op_t op = INVALID;
	char* xmlFilename=NULL;
	int ignore_duplicates = 0;
	int i;

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "a:c:f:p:dhmv", long_options, &option_index);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 0:
			VERBOSE_INFO("Long option[%d]: %s\n", option_index, long_options[option_index].name);
			if (strcmp("delete-class", long_options[option_index].name) == 0) {
				op = verify_setoption(op, DELETE_CLASS);
			}
			if (strcmp("ignore-duplicates", long_options[option_index].name) == 0) {
				ignore_duplicates = 1;
			}
		break;
		case 'a':
			optargs = realloc(optargs, ++optargs_len * sizeof(char *));
			optargs[optargs_len - 1] = strdup(optarg);
			break;
		case 'c':
			className = optarg;
			op = verify_setoption(op, CREATE_OBJECT);
			break;
		case 'd': {
			op = verify_setoption(op, DELETE_OBJECT);
			break;
		}
		case 'h':
			usage(basename(argv[0]));
			exit(EXIT_SUCCESS);
			break;
		case 'f':
			op = verify_setoption(op, LOAD_IMMFILE);
			xmlFilename = optarg;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'm': {
			op = verify_setoption(op, MODIFY_OBJECT);
			break;
		case 'p':
			op = POPULATE_IMM;
			population = atol(optarg);
			className = argv[3];
			break;
		}
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}

		if(op == POPULATE_IMM)
			break;
	}

	if (op == LOAD_IMMFILE) {
		VERBOSE_INFO("importImmXML(xmlFilename=%s, verbose=%d, ignore_duplicates=%d)\n", xmlFilename, verbose, ignore_duplicates);
		rc = importImmXML(xmlFilename, adminOwnerName, verbose, ignore_duplicates);
		exit(rc);
	}

	if (op == INVALID) {
		VERBOSE_INFO("no option specified - defaults to MODIFY\n");
		/* Modify is default */
		op = MODIFY_OBJECT;
	}

	if (verbose) {
		VERBOSE_INFO("operation:%d argc:%d optind:%d\n", op, argc, optind);

		if (optind < argc) {
			VERBOSE_INFO("non-option ARGV-elements: ");
			for (i=optind; i < argc; i++) {
				VERBOSE_INFO("%s ", argv[i]);
			}
			VERBOSE_INFO("\n");
		}
	}

	/* Remaining arguments should be object names or class names. Need at least one... */
	if ((argc - optind) < 1) {
		fprintf(stderr, "error - specify at least one object or class\n");
		exit(EXIT_FAILURE);
	}

	if (op == DELETE_CLASS) {
		while (optind < argc) {
			classNames = realloc(classNames, (classNames_len + 1) * sizeof(SaImmClassNameT*));
			classNames[classNames_len - 1] = ((SaImmClassNameT) argv[optind++]);
			classNames[classNames_len++] = NULL;
		}
	} else {
		while (optind < argc) {
			objectNames = realloc(objectNames, (objectNames_len + 1) * sizeof(SaNameT*));
			objectName = objectNames[objectNames_len - 1] = malloc(sizeof(SaNameT));
			objectNames[objectNames_len++] = NULL;
			objectName->length = snprintf((char*)objectName->value, SA_MAX_NAME_LENGTH, "%s", argv[optind++]);
		}
	}


	(void)immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);

	error = saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, &ownerHandle);
	if (error != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerInitialize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
		goto done_om_finalize;
	}

	switch (op) {
	case CREATE_OBJECT:
		rc = object_create((const SaNameT **)objectNames, className, ownerHandle, optargs, optargs_len);
		break;
	case DELETE_OBJECT:
		rc = object_delete((const SaNameT **)objectNames, ownerHandle);
		break;
	case MODIFY_OBJECT:
		rc = object_modify((const SaNameT **)objectNames, ownerHandle, optargs, optargs_len);
		break;
	case DELETE_CLASS:
		rc = class_delete(classNames, immHandle);
		break;
	case POPULATE_IMM:
#ifdef HAVE_TESTS
		rc = populate_imm(className, population, ownerHandle, immHandle);
#else 
		rc = EXIT_FAILURE;
		fprintf(stderr, "error - populate available only with 'configure --enable-tests'\n");
#endif
		break;
	default:
		fprintf(stderr, "error - no operation specified\n");
		break;
	}

	error = saImmOmAdminOwnerFinalize(ownerHandle);
	if (SA_AIS_OK != error) {
		fprintf(stderr, "error - saImmOmAdminOwnerFinalize FAILED: %s\n", saf_error(error));
		rc = EXIT_FAILURE;
		goto done_om_finalize;
	}

 done_om_finalize:
	(void)immutil_saImmOmFinalize(immHandle);

	exit(rc);
}
