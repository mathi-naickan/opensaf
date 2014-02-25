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

#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/types.h>
#include <sys/time.h>
#include <fcntl.h>
#include <ctype.h>
#include <libgen.h>
#include <signal.h>
#include <sys/stat.h>
#include <dlfcn.h>
#include <wordexp.h>

#include <saAis.h>
#include <saImmOm.h>
#include <immutil.h>
#include <saf_error.h>


static SaVersionT immVersion = { 'A', 2, 13 };
int verbose = 0;
int ccb_safe = 1;

int transaction_mode = 0;
SaImmHandleT immHandle = 0;
SaImmAdminOwnerNameT adminOwnerName = NULL;
SaImmAdminOwnerHandleT ownerHandle = 0;
SaImmCcbHandleT ccbHandle = -1;

extern struct ImmutilWrapperProfile immutilWrapperProfile;
typedef enum {
	INVALID = 0,
	CREATE_OBJECT = 1,
	DELETE_OBJECT = 2,
	DELETE_CLASS = 3,
	MODIFY_OBJECT = 4,
	LOAD_IMMFILE = 5,
	VALIDATE_IMMFILE = 6,
	CHANGE_CLASS = 7,
	ADMINOWNER_CLEAR = 8,
	TRANSACTION_MODE = 9,
	CCB_APPLY = 10,
	CCB_ABORT = 11
} op_t;

typedef enum {
	NOTIFY_UNDEFINED = 0,
	NOTIFY_ENABLED = 1,
	NOTIFY_DISABLED = 2
} attr_notify_t;

#define VERBOSE_INFO(format, args...) if (verbose) { fprintf(stderr, format, ##args); }

// Interface functions which implement -f and -L options (imm_import.cc)
int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ccb_safe,
		SaImmHandleT *immHandle, SaImmAdminOwnerHandleT *ownerHandle,
		SaImmCcbHandleT *ccbHandle, int mode, const char *xsdPath);
int validateImmXML(const char *xmlfile, int verbose, int mode);
static int imm_operation(int argc, char *argv[]);

char *(*readln)(const char *);
void (*addhistory)(const char *);


const SaImmCcbFlagsT defCcbFlags = SA_IMM_CCB_REGISTERED_OI | SA_IMM_CCB_ALLOW_NULL_OI;

static void usage(const char *progname)
{
	printf("\nNAME\n");
	printf("\t%s - create, delete or modify IMM configuration object(s)\n", progname);

	printf("\nSYNOPSIS\n");
	printf("\t%s [options] [object DN]...\n", progname);

	printf("\nDESCRIPTION\n");
	printf("\t%s is an IMM OM client used to create, delete an IMM or modify attribute(s) for IMM object(s)\n", progname);
	printf("\tThe default operation if none specified is modify.\n");
	printf("\tWhen creating or modifying several objects, they have to be of the same class.\n");
	printf("\tRunning immcfg without arguments, immcfg starts in explicit commit mode where is possible "
		"to perform more IMM operations within one CCB.\n");
	printf("\tIn explicit commit mode, immcfg accepts immcfg commands from stdin, pipe or a file.\n");
	printf("\tImmcfg command has the same syntax as immcfg in the command line. "
		"For example: \"immcfg -d safAmfNode=Node01,safAmfCluster=1\"\n");

	printf("\nOPTIONS\n");
	printf("\t-a, --attribute name[+|-]=value [object DN]... \n");
	printf("\t-c, --create-object <class name> [object DN]... \n");
	printf("\t-d, --delete-object [object DN]... \n");
	printf("\t-h, --help                    this help\n");
	printf("\t-m, --modify-object [object DN]... \n");
	printf("\t-v, --verbose (only valid with -f/--file, -L/--validate options and --class-name)\n");
	printf("\t-f, --file <imm.xml file containing classes and/or objects>\n");
	printf("\t-t, --timeout <sec>\n");
	printf("\t\tutility timeout in seconds\n");
	printf("\t--ignore-duplicates  (only valid with -f/--file option, default)\n");
	printf("\t--delete-class <classname> [classname2]... \n");
	printf("\t--class-name <classname> [attribute name]...\n");
	printf("\t--enable-attr-notify (only valid with --class-name)\n");
	printf("\t--disable-attr-notify (only valid with --class-name)\n");
	printf("\t-u, --unsafe\n");
	printf("\t-L, --validate <imm.xml file>\n");
	printf("\t-o, --admin-owner <admin owner name>\n");
	printf("\t--admin-owner-clear\n");
	printf("\t--ccb-apply (only in a transaction mode)\n");
	printf("\t--ccb-abort (only in a transaction mode)\n");
	printf("\t-X, --xsd <path_to_schema.xsd>\n");

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
	printf("\timmcfg -u .....\n");
	printf("\t\tThe CCBs generated by immcfg will have SA_IMM_CCB_REGISTERED_OI set to false, allowing ccb commit when OIs are missing\n");
	printf("\timmcfg --class-name OpensafImmTest --enable-attr-notify testUint32 testString\n");
	printf("\t\tEnable attribute notifications for testUint32 and testString attributes in OpensafImmTest class\n");
	printf("\timmcfg --class-name OpensafImmTest --disable-attr-notify testUint32 testString\n");
	printf("\t\tDisable attribute notifications for testUint32 and testString attributes in OpensafImmTest class\n");
	printf("\timmcfg -o myAdminOwner -a saAmfNodeSuFailoverMax=7 safAmfNode=Node01,safAmfCluster=1\n");
	printf("\t\tuse 'myAdminOwnerName' as admin owner name for changing one attribute of one object\n");
	printf("\timmcfg --admin-owner-clear safAmfNode=Node01,safAmfCluster=1\n");
	printf("\t\tclear admin owner from one object\n");
	printf("\tcat test.cfg | immcfg\n");
	printf("\t\tRunning immcfg in explicit commit mode where immfg accepts immcfg commands from a pipe\n");
	printf("\timmcfg < test.cfg\n");
	printf("\t\tRunning immcfg in explicit commit mode where immcfg accepts immcfg commands from test.cfg file\n");
	printf("\timmcfg\n");
	printf("\t\tRunning immcfg in explicit commit mode where immcfg accepts immcfg commands from command line\n");
	printf("\t\tCtrl+D - commit changes and exit, Ctrl+C - abort CCB and exit\n");
	printf("\timmcfg -X /etc/opensaf/schema.xsd -f imm.xml\n");
	printf("\t\timmcfg will load unsupported attribute flags in the current OpenSAF version from /etc/opensaf/schema.xsd, and use them to successfully import imm.xml");
	printf("\timmcfg -X /etc/opensaf -f imm.xml\n");
	printf("\t\timmcfg will load unsupported attribute flags in the current OpenSAF version from the schema specified in imm.xml which is stored in /etc/opensaf, and use loaded flags to successfully import imm.xml");
}

/* signal handler for SIGALRM */
void sigalarmh(int sig)
{
        fprintf(stderr, "error - immcfg command timed out (alarm)\n");
        exit(EXIT_FAILURE);
}

static void free_attr_value(SaImmValueTypeT attrValueType, SaImmAttrValueT attrValue) {
	if(attrValue) {
		if(attrValueType == SA_IMM_ATTR_SASTRINGT)
			free(*((SaStringT *)attrValue));
		else if(attrValueType == SA_IMM_ATTR_SAANYT)
			free(((SaAnyT*)attrValue)->bufferAddr);
		free(attrValue);
	}
}

static void free_attr_values(SaImmAttrValuesT_2 *attrValues) {
	int i;

	if(attrValues) {
		free(attrValues->attrName);

		for(i=0; i<attrValues->attrValuesNumber; i++)
			free_attr_value(attrValues->attrValueType, attrValues->attrValues[i]);

		free(attrValues->attrValues);
		free(attrValues);
	}
}

static void free_attr_mod(SaImmAttrModificationT_2 *attrMod) {
	if(attrMod) {
		if(attrMod->modAttr.attrName)
			free(attrMod->modAttr.attrName);
		if(attrMod->modAttr.attrValues) {
			free_attr_value(attrMod->modAttr.attrValueType, attrMod->modAttr.attrValues[0]);
			free(attrMod->modAttr.attrValues);
		}
		free(attrMod);
	}
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
	char *name = strdup(nameval);
	char *value;
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

	if ((value = strstr(name, "=")) == NULL) {
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
	name = NULL;
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

	if(name)
		free(name);

	if (res != 0) {
		if(attrMod) {
			free(attrMod);
			attrMod = NULL;
		}
	}
	return attrMod;
}

static SaAisErrorT get_attrValueType(SaImmAttrDefinitionT_2 **attrDefinitions, SaImmAttrNameT attrName, SaImmValueTypeT *attrValueType) {
	if(!attrDefinitions || !attrName)
		return SA_AIS_ERR_NOT_EXIST;

	int i=0;
	while(attrDefinitions[i]) {
		if(!strcmp(attrDefinitions[i]->attrName, attrName)) {
			if(attrValueType)
				*attrValueType = attrDefinitions[i]->attrValueType;
			return SA_AIS_OK;
		}
		i++;
	}

	return SA_AIS_ERR_NOT_EXIST;
}

/**
 * Alloc SaImmAttrValuesT_2 object and initialize its attributes from nameval (x=y)
 * @param attrDefinitions
 * @param nameval
 * @param isRdn
 *
 * @return SaImmAttrValuesT_2*
 */
static SaImmAttrValuesT_2 *new_attr_value(SaImmAttrDefinitionT_2 **attrDefinitions, char *nameval, int isRdn)
{
	int res = 0;
	char *name = strdup(nameval), *p;
	char *value;
	SaImmAttrValuesT_2 *attrValue = NULL;
	SaAisErrorT error;

	attrValue = calloc(1, sizeof(SaImmAttrValuesT_2));

	p = strchr(name, '=');
	if (p == NULL){
		fprintf(stderr, "The Attribute '%s' does not contain a equal sign ('=')\n", nameval);
		res = -1;
		goto done;
	}
	*p = '\0';
	value = p + 1;

	attrValue->attrName = name;
	name = NULL;
	VERBOSE_INFO("new_attr_value attrValue->attrName: '%s' value:'%s'\n", attrValue->attrName, isRdn ? nameval : value);

	error = get_attrValueType(attrDefinitions, attrValue->attrName, &attrValue->attrValueType);

	if (error != SA_AIS_OK) {
		fprintf(stderr, "Attribute '%s' does not exist in class\n", attrValue->attrName);
		res = -1;
		goto done;
	}

	attrValue->attrValuesNumber = 1;
	attrValue->attrValues = malloc(sizeof(SaImmAttrValueT *));
	attrValue->attrValues[0] = immutil_new_attrValue(attrValue->attrValueType, isRdn ? nameval : value);
	if (attrValue->attrValues[0] == NULL)
		res = -1;

 done:
	if(name)
		free(name);
	if (res != 0) {
		if(attrValue->attrName)
			free(attrValue->attrName);
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
	char *str = NULL, *delim;
	const SaNameT *parentNames[] = {parentName, NULL};
	const SaStringT* errStrings=NULL;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions = NULL;

	if((error = saImmOmClassDescriptionGet_2(immHandle, className, &classCategory, &attrDefinitions)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmClassDescriptionGet_2. FAILED: %s\n", saf_error(error));
		goto done;
	}

	for (i = 0; i < optargs_len; i++) {
		VERBOSE_INFO("object_create optargs[%d]: '%s'\n", i, optargs[i]);
		if ((attrValue = new_attr_value(attrDefinitions, optargs[i], 0)) == NULL){
			fprintf(stderr, "error - creating attribute from '%s'\n", optargs[i]);
			goto done;
		}

		attrValues = realloc(attrValues, (attr_len + 1) * sizeof(SaImmAttrValuesT_2 *));
		attrValues[attr_len - 1] = attrValue;
		attrValues[attr_len] = NULL;
		attr_len++;
	}

	attrValues = realloc(attrValues, (attr_len + 1) * sizeof(SaImmAttrValuesT_2 *));
	attrValues[attr_len] = NULL;

	if(ccbHandle == -1) {
		if ((error = immutil_saImmOmCcbInitialize(ownerHandle, ccb_safe?defCcbFlags:0x0, &ccbHandle))
			!= SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
			goto done;
		}
	}

	i = 0;
	while (objectNames[i] != NULL) {
		str = strdup((char*)objectNames[i]->value);
		if ((delim = strchr(str, ',')) != NULL) {
			/* a parent exist */
			while (delim && *(delim - 1) == 0x5c) {
				/* comma delimiter is escaped, search again */
				delim += 2;
				delim = strchr(delim, ',');
			}

			if(delim) {
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
				if ((error = immutil_saImmOmAdminOwnerSet(ownerHandle, parentNames, SA_IMM_SUBTREE)) != SA_AIS_OK) {
					if (error == SA_AIS_ERR_NOT_EXIST)
						fprintf(stderr, "error - parent '%s' does not exist\n", dn.value);
					else {
						fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));
						goto done;
					}
				}
			}
		}

		VERBOSE_INFO("object_create rdn attribute attrValues[%d]: '%s' \n", attr_len - 1, str);
		if ((attrValue = new_attr_value(attrDefinitions, str, 1)) == NULL){
			fprintf(stderr, "error - creating rdn attribute from '%s'\n", str);
			goto done;
		}
		attrValues[attr_len - 1] = attrValue;
		attrValue = NULL;

		if ((error = immutil_saImmOmCcbObjectCreate_2(ccbHandle, className, parentName,
			(const SaImmAttrValuesT_2**)attrValues)) != SA_AIS_OK) {

			fprintf(stderr, "error - saImmOmCcbObjectCreate_2 FAILED with %s\n",
				saf_error(error));

			if((error == SA_AIS_ERR_NOT_EXIST) && ccb_safe) {
				fprintf(stderr, "Missing: implementer, or object, or attribute "
					"(see: immcfg -h under '--unsafe')\n");
			} else {
				SaAisErrorT rc2 = saImmOmCcbGetErrorStrings(ccbHandle, &errStrings);
				if(errStrings) {
					int ix = 0;
					while(errStrings[ix]) {
						fprintf(stderr, "OI reports: %s\n", errStrings[ix]);
						++ix;
					}
				} else if(rc2 != SA_AIS_OK) {
					fprintf(stderr, "saImmOmCcbGetErrorStrings failed: %u\n", rc2);
				}
			}

			free_attr_values(attrValues[attr_len - 1]);

			goto done;
		}

		free_attr_values(attrValues[attr_len - 1]);
		attrValues[attr_len - 1] = NULL;

		free(str);
		str = NULL;

		i++;
	}

	rc = 0;

done:

	if(str)
		free(str);

	if(attrValues) {
		for(i=0; i<attr_len-1; i++)
			free_attr_values(attrValues[i]);
		free(attrValues);
	}

	if(attrDefinitions)
		saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);

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
	const SaStringT* errStrings=NULL;

	for (i = 0; i < optargs_len; i++) {
		if ((attrMod = new_attr_mod(objectNames[0], optargs[i])) == NULL) {
			fprintf(stderr, "error - creating attribute from '%s'\n", optargs[i]);
			goto done;
		}

		attrMods = realloc(attrMods, (attr_len + 1) * sizeof(SaImmAttrModificationT_2 *));
		attrMods[attr_len - 1] = attrMod;
		attrMods[attr_len] = NULL;
		attr_len++;
	}

	if ((error = immutil_saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames, SA_IMM_ONE)) != SA_AIS_OK) {
		if (error == SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "error - object '%s' does not exist\n", objectNames[0]->value);
		else
			fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));

		goto done;
	}

	if(ccbHandle == -1) {
		if ((error = immutil_saImmOmCcbInitialize(ownerHandle, ccb_safe?defCcbFlags:0x0, &ccbHandle))
			!= SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
			goto done;
		}
	}

	i = 0;
	while (objectNames[i] != NULL) {
		if ((error = immutil_saImmOmCcbObjectModify_2(ccbHandle, objectNames[i],
			(const SaImmAttrModificationT_2 **)attrMods)) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbObjectModify_2 FAILED: %s\n", saf_error(error));

			if((error == SA_AIS_ERR_NOT_EXIST) && ccb_safe) {
				fprintf(stderr, "Missing: implementer, or object, or attribute "
					"(see: immcfg -h under '--unsafe')\n");
			} else {
				SaAisErrorT rc2 = saImmOmCcbGetErrorStrings(ccbHandle, &errStrings);
				if(errStrings) {
					int ix = 0;
					while(errStrings[ix]) {
						fprintf(stderr, "OI reports: %s\n", errStrings[ix]);
						++ix;
					}
				} else if(rc2 != SA_AIS_OK) {
					fprintf(stderr, "saImmOmCcbGetErrorStrings failed: %u\n", rc2);
				}
			}
			goto done;
		}
		i++;
	}

	rc = 0;

 done:

	if(attrMods) {
		for(i=0; i<attr_len; i++)
			free_attr_mod(attrMods[i]);
		free(attrMods);
	}

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
	int i = 0;
	const SaStringT* errStrings=NULL;

	if ((error = immutil_saImmOmAdminOwnerSet(ownerHandle, (const SaNameT **)objectNames,
		SA_IMM_SUBTREE)) != SA_AIS_OK) {

		if (error == SA_AIS_ERR_NOT_EXIST)
			fprintf(stderr, "error - object does not exist\n");
		else
			fprintf(stderr, "error - saImmOmAdminOwnerSet FAILED: %s\n", saf_error(error));

		goto done;
	}

	if(ccbHandle == -1) {
		if ((error = immutil_saImmOmCcbInitialize(ownerHandle, ccb_safe?defCcbFlags:0x0, &ccbHandle))
			!= SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbInitialize FAILED: %s\n", saf_error(error));
			goto done;
		}
	}

	while (objectNames[i] != NULL) {
		if ((error = immutil_saImmOmCcbObjectDelete(ccbHandle, objectNames[i])) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbObjectDelete for '%s' FAILED: %s\n",
				objectNames[i]->value, saf_error(error));

			if((error == SA_AIS_ERR_NOT_EXIST) && ccb_safe) {
				fprintf(stderr, "Missing: implementer, or object, or attribute "
					"(see: immcfg -h under '--unsafe')\n");
			} else {
				SaAisErrorT rc2 = saImmOmCcbGetErrorStrings(ccbHandle, &errStrings);
				if(errStrings) {
					int ix = 0;
					while(errStrings[ix]) {
						fprintf(stderr, "OI reports: %s\n", errStrings[ix]);
						++ix;
					}
				} else if(rc2 != SA_AIS_OK) {
					fprintf(stderr, "saImmOmCcbGetErrorStrings failed: %u\n", rc2);
				}
			}

			goto done;
		}
		i++;
	}

	rc = 0;
done:
	return rc;
}

/**
 * Delete class(es) in the NULL terminated array
 * @param classNames
 * @param immHandle
 *
 * @return int
 */
int class_delete(const SaImmClassNameT *classNames, SaImmHandleT immHandle)
{
	SaAisErrorT error;
	int rc = EXIT_FAILURE;
	int i = 0;

	while (classNames[i] != NULL) {
		if ((error = immutil_saImmOmClassDelete(immHandle, classNames[i])) != SA_AIS_OK) {
			if (error == SA_AIS_ERR_NOT_EXIST)
				fprintf(stderr, "error - class does not exist :%s\n", classNames[i]);
			else
				fprintf(stderr, "error - saImmOmClassDelete FAILED: %s\n", saf_error(error));

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
		if(transaction_mode)
			return NULL;
		else
			exit(EXIT_FAILURE);
	}
	sprintf(unique_adminOwner, "%s_%s_%d", base, hostname, getpid());
	return unique_adminOwner;
}

static int admin_owner_clear(const SaNameT **objectNames, SaImmHandleT immHandle) {
	SaAisErrorT error;

	if((error = saImmOmAdminOwnerClear(immHandle, objectNames, SA_IMM_ONE)) != SA_AIS_OK) {
		fprintf(stderr, "error - saImmOmAdminOwnerClear FAILED: %s\n", saf_error(error));
		return EXIT_FAILURE;
	}

	return 0;
}

static int class_change(SaImmHandleT immHandle, const SaImmAdminOwnerNameT adminOwnerName,
		const SaImmClassNameT className, const char **attributeNames, attr_notify_t attrNotify)
{
	SaAisErrorT error;
	SaImmAccessorHandleT accessorHandle;
	SaNameT opensafImmObjectName = { strlen("opensafImm=opensafImm,safApp=safImmService"),
									"opensafImm=opensafImm,safApp=safImmService" };
	const SaNameT *objectNameList[] = { &opensafImmObjectName, NULL };
	SaImmAttrNameT opensafImmAttrName[2] = { "opensafImmNostdFlags", NULL };
	SaImmAttrValuesT_2 **attributes;
	int isSchemaChangeEnabled = 2;	/*	0 - schema changes disabled
										1 - schema changes enabled
										2 - PBE disabled			*/
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions = NULL;
	int rc = 0;

	int attrNum = 0;

	/* Check if the schema change is enabled (isSchemaChangeEnabled = 1).
	 * If PBE is not enabled, then this information cannot be retrieved from IMM,
	 * and in that case, the schema change is enabled by default (isSchemaChangeEnabled = 2)
	 */
	if((error = immutil_saImmOmAccessorInitialize(immHandle, &accessorHandle)) != SA_AIS_OK) {
		fprintf(stderr, "Failed to initialize accessor handle. Error: %d\n", error);
		return EXIT_FAILURE;
	}

	error = immutil_saImmOmAccessorGet_2(accessorHandle, &opensafImmObjectName, opensafImmAttrName, &attributes);
	if(error == SA_AIS_OK && attributes && attributes[0] && attributes[0]->attrValuesNumber)
		isSchemaChangeEnabled = *((SaUint32T *)attributes[0]->attrValues[0]) & 0x01;

	immutil_saImmOmAccessorFinalize(accessorHandle);

	/* Check for the existence of attributes */
	if((error = immutil_saImmOmClassDescriptionGet_2(immHandle, className, &classCategory, &attrDefinitions)) != SA_AIS_OK) {
		fprintf(stderr, "Cannot get class definition for '%s'. Error: %d\n", className, error);
		return EXIT_FAILURE;
	}

	while(attributeNames[attrNum]) {
		int i = 0;
		while(attrDefinitions[i]) {
			if(!strcmp(attributeNames[attrNum], attrDefinitions[i]->attrName))
				break;
			i++;
		}

		if(!attrDefinitions[i]) {
			fprintf(stderr, "Class '%s' does not have attribute '%s'\n", className, attributeNames[attrNum]);
			rc = EXIT_FAILURE;
			goto done;
		}

		if(attrNotify == NOTIFY_ENABLED) {
			if(attrDefinitions[i]->attrFlags & SA_IMM_ATTR_NOTIFY) {
				VERBOSE_INFO("Notify flag has already been set to attribute '%s'\n", attrDefinitions[i]->attrName);
			} else {
				VERBOSE_INFO("Notify flag set to attribute '%s'\n", attrDefinitions[i]->attrName);
				attrDefinitions[i]->attrFlags |= SA_IMM_ATTR_NOTIFY;
			}
		} else if(attrDefinitions[i]->attrFlags & SA_IMM_ATTR_NOTIFY) {
			VERBOSE_INFO("Notify flag removed from attribute '%s'\n", attrDefinitions[i]->attrName);
			attrDefinitions[i]->attrFlags ^= SA_IMM_ATTR_NOTIFY;
		} else
			VERBOSE_INFO("Notify flag is not set to attribute '%s'\n", attrDefinitions[i]->attrName);

		attrNum++;
	}

	/* if schema change is disable, then turn it on until the class change is done */
	if(!isSchemaChangeEnabled) {
		SaAisErrorT err;
		SaUint32T nostdFlag = 1;
		SaImmAdminOperationParamsT_2 param = { "opensafImmNostdFlags", SA_IMM_ATTR_SAUINT32T, (SaImmAttrValueT)&nostdFlag };
		const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };

		if((error = immutil_saImmOmAdminOwnerSet(ownerHandle, objectNameList, SA_IMM_ONE)) != SA_AIS_OK) {
			fprintf(stderr, "Cannot set admin owner on 'opensafImm=opensafImm,safApp=safImmService'\n");
			rc = EXIT_FAILURE;
			goto done;
		}

		if((error = immutil_saImmOmAdminOperationInvoke_2(ownerHandle, &opensafImmObjectName, 1, 1, params, &err, SA_TIME_ONE_SECOND * 60)) != SA_AIS_OK) {
			fprintf(stderr, "Failed to enable schema changes. Error: %d\n", error);
			rc = EXIT_FAILURE;
			goto done;
		}
	}

	if((error = immutil_saImmOmClassCreate_2(immHandle, className, classCategory, (const SaImmAttrDefinitionT_2 **)attrDefinitions)) != SA_AIS_OK) {
		rc = EXIT_FAILURE;
		if(isSchemaChangeEnabled == 2)
			fprintf(stderr, "Cannot change the class. Probable cause is that PBE is disabled. Try to turn on schema changes manually. Error: %d\n", error);
		else {
			if(error == SA_AIS_ERR_EXIST)
				fprintf(stderr, "No change in the class. Error: %d\n", error);
			else
				fprintf(stderr, "Failed to change the class flags. Error: %d\n", error);
		}
	}

	if(!isSchemaChangeEnabled) {
		SaAisErrorT err = SA_AIS_OK;
		SaUint32T nostdFlag = 1;
		SaImmAdminOperationParamsT_2 param = { "opensafImmNostdFlags", SA_IMM_ATTR_SAUINT32T, (SaImmAttrValueT)&nostdFlag };
		const SaImmAdminOperationParamsT_2 *params[2] = { &param, NULL };

		if((error = immutil_saImmOmAdminOperationInvoke_2(ownerHandle, &opensafImmObjectName, 0, 2, params, &err, SA_TIME_ONE_SECOND * 60)) != SA_AIS_OK) {
			fprintf(stderr, "Failed to disable schema changes. Error: %d\n", error);
			rc = EXIT_FAILURE;
		}
	}

done:
	if(attrDefinitions)
		immutil_saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);

	return rc;
}

static int ccb_apply() {
	int rc = 0;
	SaAisErrorT error;

	if(ccbHandle != -1) {
		if((error = immutil_saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) {
			if(error == SA_AIS_ERR_TIMEOUT)
				fprintf(stderr, "saImmOmCcbApply returned SA_AIS_ERR_TIMEOUT, result for CCB is unknown\n");
			else
				fprintf(stderr, "error - saImmOmCcbApply FAILED: %s\n", saf_error(error));
			rc = EXIT_FAILURE;
			goto done;
		}

		if ((error = immutil_saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
			fprintf(stderr, "warning - successfully applied changes but saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
			rc = EXIT_FAILURE;
		}

		ccbHandle = -1;
	}

done:
	return (transaction_mode) ? rc : 0;
}

static int ccb_abort() {
	int rc = 0;
	SaAisErrorT error;

	if(ccbHandle != -1) {
		if ((error = immutil_saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
			fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
			rc = EXIT_FAILURE;
		}

		ccbHandle = -1;
	}

	return (transaction_mode) ? rc : 0;
}

static char *stdreadline(const char *prompt) {
	if(prompt)
		printf("%s", prompt);

	size_t len = 0;
	char *line = NULL;
	if(getline(&line, &len, stdin) == -1) {
		free(line);
		return NULL;
	}

	return line;
}

static void stdaddhistory(const char *line) {
}

static char *readinput(char *prompt) {
	char *line;
	char *rdline = NULL;
	int len;
	int rdlen = 0;
	char *pmt = prompt;

	while((line = readln(pmt))) {
		if(!line) {
			if(rdline)
				free(rdline);
			return NULL;
		}

		len = strlen(line);
		if(len == 0) {
			if(!rdline)
				rdline = line;
			break;
		}

		addhistory(line);

		if(line[len - 1] == '\\') {
			line[len - 1] = 0;
			if(rdlen) {
				rdline = realloc(rdline, rdlen + len);
				strncat(rdline, line, len);
				rdlen += len - 1;
			} else {
				rdline = line;
				rdlen = len;
			}
			pmt = NULL;
		} else {
			if(rdline) {
				rdline = realloc(rdline, rdlen + len + 1);
				strncat(rdline, line, len + 1);
			} else
				rdline = line;

			break;
		}
	}

	return rdline;
}

static int start_cmd() {
	char *line = NULL;
	int rd;
	int rc = 0;
	struct stat st;
	char *prompt = "> ";
	void *dlhdl = NULL;
	int isCmdLn = 0;

	readln = stdreadline;
	addhistory = stdaddhistory;

	fstat(fileno(stdin), &st);
	if(S_ISCHR(st.st_mode)) {
		/* If immcfg is running in command line mode,
		 * try to add readline features if it's supported by the system,
		 * otherwise use simple getline() without history
		 */
		isCmdLn = 1;
		dlhdl = dlopen("libreadline.so", RTLD_LAZY);
		dlerror();
		if(dlhdl) {
			readln = (char *(*)(const char *))dlsym(dlhdl, "readline");
			if(!dlerror() && readln) {
				addhistory = (void (*)(const char *))dlsym(dlhdl, "add_history");
				if(dlerror() != NULL || !addhistory)
					addhistory = stdaddhistory;
			} else
				readln = stdreadline;
		}
	} else
		prompt = NULL;

	transaction_mode = 1;
	optind = 0;

	wordexp_t p;
	while((line = readinput(prompt))) {
		rd = strlen(line);
		while(rd > 0 && (line[rd - 1] == 13 || line[rd - 1] == 10)) {
			line[rd - 1] = 0;
			rd--;
		}

		if(rd == 0)
			goto done;

		rc = wordexp(line, &p, 0);

		if(rc) {
			switch(rc) {
				case WRDE_BADCHAR :
					fprintf(stderr, "Illegal occurrence of newline or one of |, &, ;, <, >, (, ), {, }. Error: WRDE_BADCHAR\n");
					break;
				case WRDE_SYNTAX :
					fprintf(stderr, "Shell syntax error, such as unbalanced parentheses or unmatched quotes. Error: WRDE_SYNTAX\n");
					break;
				default:
					fprintf(stderr, "Error in parsing line. RC: %d\n", rc);
					break;
			}
			if(!isCmdLn) {
				free(line);
				break;
			}
		}

		if(p.we_wordc > 0) {
			if(!strcmp(p.we_wordv[0], "immcfg")) {
				optind = 0;
				if((rc = imm_operation(p.we_wordc, p.we_wordv)) && !isCmdLn) {
					free(line);
					wordfree(&p);
					break;
				}
				alarm(0);
			} else
				fprintf(stderr, "Not immcfg command\n");
		}

		wordfree(&p);

done:
		if(line) {
			free(line);
			line = NULL;
		}
	}

	transaction_mode = 0;

	if(dlhdl)
		dlclose(dlhdl);

	if(S_ISREG(st.st_mode))
		printf("\n");

	return rc;
}

static op_t verify_setoption(op_t prevValue, op_t newValue)
{
	if (prevValue == INVALID)
		return newValue;
	else {
		fprintf(stderr, "error - only one operation at a time supported\n");
		if(transaction_mode)
			return INVALID;
		else
			exit(EXIT_FAILURE);
	}
}

static int imm_operation(int argc, char *argv[])
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
		{"timeout", required_argument, NULL, 't'},
		{"verbose", no_argument, NULL, 'v'},
		{"unsafe", no_argument, NULL, 'u'},
		{"validate", required_argument, NULL, 'L'},
		{"class-name", required_argument, NULL, 0},
		{"enable-attr-notify", no_argument, NULL, 0},
		{"disable-attr-notify", no_argument, NULL, 0},
		{"admin-owner", required_argument, NULL, 'o'},
		{"admin-owner-clear", no_argument, NULL, 0},
		{"ccb-apply", no_argument, NULL, 0},
		{"ccb-abort", no_argument, NULL, 0},
		{"xsd", required_argument, NULL, 'X'},
		{0, 0, 0, 0}
	};
	SaAisErrorT error;
	int useAdminOwner = 1;
	SaNameT **objectNames = NULL;
	int objectNames_len = 1;

	SaImmClassNameT *classNames = NULL;
	int classNames_len = 1;

	char **attributeNames = NULL;	/* NULL terminated array of attribute names */

	SaNameT *objectName;
	int optargs_len = 0;	/* one off */
	char **optargs = NULL;
	SaImmClassNameT className = NULL;
	op_t op = INVALID;
	char* xmlFilename=NULL;
	int i;
	unsigned long timeoutVal = 60;
	attr_notify_t attrNotify = NOTIFY_UNDEFINED;

	char *xsdPath = NULL;

	while (1) {
		int option_index = 0;
		c = getopt_long(argc, argv, "a:c:f:t:dhmvuL:o:X:", long_options, &option_index);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 0:
			VERBOSE_INFO("Long option[%d]: %s\n", option_index, long_options[option_index].name);
			if (strcmp("delete-class", long_options[option_index].name) == 0) {
				op = verify_setoption(op, DELETE_CLASS);
			} else if (!strcmp("admin-owner-clear", long_options[option_index].name)) {
				op = verify_setoption(op, ADMINOWNER_CLEAR);
				useAdminOwner = 0;
			} else if (strcmp("class-name", long_options[option_index].name) == 0) {
				op = verify_setoption(op, CHANGE_CLASS);
				className = optarg;
			} else if (strcmp("enable-attr-notify", long_options[option_index].name) == 0) {
				if(attrNotify == NOTIFY_UNDEFINED)
					attrNotify = NOTIFY_ENABLED;
				else {
					fprintf(stderr, "error - enable-attr-notify cannot be used with %s\n",
							(attrNotify == NOTIFY_DISABLED) ? "--disable-attr-notify" : "--enable-attr-notify");
					if(transaction_mode)
						return -1;
					else
						exit(EXIT_FAILURE);
				}
			} else if (strcmp("disable-attr-notify", long_options[option_index].name) == 0) {
				if(attrNotify == NOTIFY_UNDEFINED)
					attrNotify = NOTIFY_DISABLED;
				else {
					fprintf(stderr, "error - disable-attr-notify cannot be used with %s\n",
							(attrNotify == NOTIFY_ENABLED) ? "--enable-attr-notify" : "--disable-attr-notify");
					if(transaction_mode)
						return -1;
					else
						exit(EXIT_FAILURE);
				}
			} else if (strcmp("ccb-apply", long_options[option_index].name) == 0) {
				if(argc != 2) {
					fprintf(stderr, "error - ccb-apply option does not have any argument\n");
					if(transaction_mode)
						return -1;
					else
						exit(EXIT_FAILURE);
				}
				op = verify_setoption(op, CCB_APPLY);
			} else if (strcmp("ccb-abort", long_options[option_index].name) == 0) {
				if(argc != 2) {
					fprintf(stderr, "error - ccb-abort option does not have any argument\n");
					if(transaction_mode)
						return -1;
					else
						exit(EXIT_FAILURE);
				}
				op = verify_setoption(op, CCB_ABORT);
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
			if(!transaction_mode)
				free(adminOwnerName);
			usage(basename(argv[0]));
			return 0;
		case 'f':
			op = verify_setoption(op, LOAD_IMMFILE);
			xmlFilename = optarg;
			break;
		case 't':
			timeoutVal = strtol(optarg, (char **)NULL, 10);			
			break;

		case 'v':
			verbose = 1;
			break;
		case 'u': /* Unsafe mode */
			ccb_safe = 0;
			break;
		case 'm': {
			op = verify_setoption(op, MODIFY_OBJECT);
			break;
		}
		case 'L':
			op = verify_setoption(op, VALIDATE_IMMFILE);
			xmlFilename = optarg;
			break;
		case 'o':
			if(adminOwnerName) {
				fprintf(stderr, "Administrative owner name can be set only once\n");
				if(transaction_mode)
					return -1;
				else
					exit(EXIT_FAILURE);
			}
			adminOwnerName = (SaImmAdminOwnerNameT)malloc(strlen(optarg) + 1);
			strcpy(adminOwnerName, optarg);
			break;
		case 'X':
			if(xsdPath) {
				fprintf(stderr, "XSD path is already set\n");
				if(transaction_mode)
					return -1;
				else
					exit(EXIT_FAILURE);
			}
			xsdPath = strdup(optarg);
			break;
		default:
			fprintf(stderr, "Try '%s --help' for more information\n", argv[0]);
			if(transaction_mode)
				return -1;
			else {
				free(adminOwnerName);
				exit(EXIT_FAILURE);
			}
			break;
		}
	}

	if(argc == 1)
		op = TRANSACTION_MODE;

	if(!transaction_mode && !adminOwnerName)
		adminOwnerName = create_adminOwnerName(basename(argv[0]));

	if(op != TRANSACTION_MODE) {
		signal(SIGALRM, sigalarmh);
		alarm(timeoutVal);
	}

	immutilWrapperProfile.errorsAreFatal = 0;
	immutilWrapperProfile.nTries = timeoutVal;
	immutilWrapperProfile.retryInterval = 1000;

	if (op == VALIDATE_IMMFILE) {
		VERBOSE_INFO("validateImmXML(xmlFilename=%s, verbose=%d)\n", xmlFilename, verbose);
		rc = validateImmXML(xmlFilename, verbose, transaction_mode);

		if(rc == 0)
			printf("Validation is successful\n");

		if(transaction_mode)
			return rc;
		else {
			free(adminOwnerName);
			exit(rc);
		}
	}
	
	if (op == LOAD_IMMFILE) {
		VERBOSE_INFO("importImmXML(xmlFilename=%s, verbose=%d)\n", xmlFilename, verbose);
		rc = importImmXML(xmlFilename, adminOwnerName, verbose, ccb_safe,
				&immHandle, &ownerHandle, &ccbHandle, transaction_mode, xsdPath);
		if(transaction_mode) {
			if(rc) {
				fprintf(stderr, "CCB is aborted\n");
				ccb_abort();
				return rc;
			} else
				return rc;
		} else {
			free(adminOwnerName);
			exit(rc);
		}
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

	/* Remaining arguments should be object names, class names or attribute names. Need at least one... */
	if (((argc - optind) < 1) && (op != TRANSACTION_MODE) && (op != CCB_APPLY) && (op != CCB_ABORT)) {
		fprintf(stderr, "error - specify at least one object or class\n");
		if(transaction_mode)
			return -1;
		else {
			free(adminOwnerName);
			exit(EXIT_FAILURE);
		}
	}

	if (op == DELETE_CLASS) {
		while (optind < argc) {
			classNames = realloc(classNames, (classNames_len + 1) * sizeof(SaImmClassNameT*));
			classNames[classNames_len - 1] = ((SaImmClassNameT) argv[optind++]);
			classNames[classNames_len++] = NULL;
		}
	} else if (op == CHANGE_CLASS) {
		int i = 0;

		if(attrNotify == NOTIFY_UNDEFINED) {
			fprintf(stderr, "--class-name must be used in combination with --enable-attr-notify or --disable-attr-notify\n");
			if(transaction_mode)
				return -1;
			else {
				free(adminOwnerName);
				exit(EXIT_FAILURE);
			}
		}

		attributeNames = (char **)malloc(((argc - optind) + 1) * sizeof(char *));
		while (optind < argc)
			attributeNames[i++] = argv[optind++];
		attributeNames[i] = NULL;
	} else if (op == TRANSACTION_MODE) {
		if(transaction_mode) {
			fprintf(stderr, "immcfg is already in transaction mode\n");
			return -1;
		}
	} else if (op == CCB_APPLY || op == CCB_ABORT) {
		if(!transaction_mode) {
			/* printing the error is not necessary. 0 will be returned if it is not in transaction mode.
			 * 0 is on exit that an immcfg script does not get break if it is not run in transaction mode */
			/* fprintf(stderr, "The command works only in transaction mode\n"); */
			rc = EXIT_SUCCESS;
			goto done_om_finalize;
		}
	} else {
		while (optind < argc) {
			objectNames = realloc(objectNames, (objectNames_len + 1) * sizeof(SaNameT*));
			objectName = objectNames[objectNames_len - 1] = malloc(sizeof(SaNameT));
			objectNames[objectNames_len++] = NULL;
			objectName->length = snprintf((char*)objectName->value, SA_MAX_NAME_LENGTH, "%s", argv[optind++]);
		}
	}

	if(!transaction_mode) {
		error = immutil_saImmOmInitialize(&immHandle, NULL, &immVersion);
        	if (error != SA_AIS_OK) {
                	fprintf(stderr, "error - saImmOmInitialize FAILED: %s\n", saf_error(error));
	                rc = EXIT_FAILURE;
        	        goto done_om_finalize;
	        }

		if(useAdminOwner) {
			error = immutil_saImmOmAdminOwnerInitialize(immHandle, adminOwnerName, SA_TRUE, &ownerHandle);
			if (error != SA_AIS_OK) {
				fprintf(stderr, "error - saImmOmAdminOwnerInitialize FAILED: %s\n", saf_error(error));
				rc = EXIT_FAILURE;
				goto done_om_finalize;
			}
		}
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
	case ADMINOWNER_CLEAR:
		rc = admin_owner_clear((const SaNameT **)objectNames, immHandle);
		break;
	case CHANGE_CLASS :
		rc = class_change(immHandle, adminOwnerName, className, (const char **)attributeNames, attrNotify);
		break;
	case TRANSACTION_MODE :
		rc = start_cmd();
		break;
	case CCB_APPLY :
		rc = ccb_apply();
		break;
	case CCB_ABORT :
		rc = ccb_abort();
		break;
	default:
		fprintf(stderr, "error - no operation specified\n");
		op = INVALID;
		rc = EXIT_FAILURE;
		break;
	}

	if(!transaction_mode) {
		if(ccbHandle != -1) {
			if((error = immutil_saImmOmCcbApply(ccbHandle)) != SA_AIS_OK) {
				if(error == SA_AIS_ERR_TIMEOUT)
					fprintf(stderr, "saImmOmCcbApply returned SA_AIS_ERR_TIMEOUT, result for CCB is unknown\n");
				else
					fprintf(stderr, "error - saImmOmCcbApply FAILED: %s\n", saf_error(error));

				const SaStringT *errStrings = NULL;
				SaAisErrorT rc2 = saImmOmCcbGetErrorStrings(ccbHandle, &errStrings);
				if(errStrings) {
					int ix = 0;
					while(errStrings[ix]) {
						fprintf(stderr, "OI reports: %s\n", errStrings[ix]);
						++ix;
					}
				} else if(rc2 != SA_AIS_OK) {
					fprintf(stderr, "saImmOmCcbGetErrorStrings failed: %u\n", rc2);
				}

				rc = EXIT_FAILURE;
			}

			if ((error = immutil_saImmOmCcbFinalize(ccbHandle)) != SA_AIS_OK) {
				fprintf(stderr, "error - saImmOmCcbFinalize FAILED: %s\n", saf_error(error));
				rc = EXIT_FAILURE;
			}
		}

		if(useAdminOwner) {
			error = immutil_saImmOmAdminOwnerFinalize(ownerHandle);
			if (SA_AIS_OK != error) {
				fprintf(stderr, "error - saImmOmAdminOwnerFinalize FAILED: %s\n", saf_error(error));
				rc = EXIT_FAILURE;
			}
		}
	} else if(rc && op != INVALID) {
		fprintf(stderr, "CCB is aborted\n");
		ccb_abort();
	}

 done_om_finalize:
	if(optargs) {
		for(i=0; i<optargs_len; i++)
			free(optargs[i]);
		free(optargs);
	}
	if(objectNames) {
		for(i=0; i<objectNames_len; i++)
			free(objectNames[i]);
		free(objectNames);
	}
	if(classNames)
		free(classNames);
	if(attributeNames)
		free(attributeNames);
	if(!transaction_mode) {
		if(adminOwnerName)
			free(adminOwnerName);
		if(immHandle) {
			error = immutil_saImmOmFinalize(immHandle);
			if (SA_AIS_OK != error) {
				fprintf(stderr, "error - saImmOmFinalize FAILED: %s\n", saf_error(error));
				rc = EXIT_FAILURE;
			}
		}
	}
	if(xsdPath) {
		free(xsdPath);
		xsdPath = NULL;
	}

	return rc;
}

int main(int argc, char *argv[]) {
	return imm_operation(argc, argv);
}
