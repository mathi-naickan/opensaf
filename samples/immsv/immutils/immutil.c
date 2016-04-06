/*       OpenSAF
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

#define _GNU_SOURCE
#ifndef SA_EXTENDED_NAME_SOURCE
#define SA_EXTENDED_NAME_SOURCE
#endif
#include "immutil.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdarg.h>
#include <syslog.h>
#include <errno.h>

#include "saAis.h"
#include "logtrace.h"
#include "osaf_extended_name.h"

static const SaVersionT immVersion = { 'A', 2, 11 };

/* Memory handling functions */
#define CHUNK	4000
static struct Chunk *newChunk(struct Chunk *next, size_t size);
static void *clistMalloc(struct Chunk *clist, size_t size);
static void deleteClist(struct Chunk *clist);

/* SA-item duplicate functions */
static const SaNameT *dupSaNameT(struct Chunk *clist, const SaNameT *original);
static SaImmClassNameT dupSaImmClassNameT(struct Chunk *clist, const SaImmClassNameT original);
static const SaImmAttrValuesT_2 **dupSaImmAttrValuesT_array(struct Chunk *clist, const SaImmAttrValuesT_2 **original);
static const SaImmAttrModificationT_2 **dupSaImmAttrModificationT_array(struct Chunk *clist,
									const SaImmAttrModificationT_2 **original);
static char *dupStr(struct Chunk *clist, const char *original);

static void defaultImmutilError(char const *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));

ImmutilErrorFnT immutilError = defaultImmutilError;
static struct CcbUtilCcbData *ccbList = NULL;

/**
 * Report to stderr and syslog and abort process
 * @param fmt
 */
static void defaultImmutilError(char const *fmt, ...)
{
	va_list ap;
	va_list ap2;

	va_start(ap, fmt);
	va_copy(ap2, ap);
	vfprintf(stderr, fmt, ap);
	vsyslog(LOG_ERR, fmt, ap2);
	va_end(ap2);
	va_end(ap);
	abort();
}

static struct CcbUtilCcbData *ccbutil_createCcbData(SaImmOiCcbIdT ccbId)
{
	struct Chunk *clist = newChunk(NULL, CHUNK);
	struct CcbUtilCcbData *obj = (struct CcbUtilCcbData *)
	    clistMalloc(clist, sizeof(struct CcbUtilCcbData));
	obj->ccbId = ccbId;
	obj->memref = clist;
	obj->next = ccbList;
	ccbList = obj;
	return obj;
}

struct CcbUtilCcbData *ccbutil_findCcbData(SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbitem = ccbList;
	while (ccbitem != NULL) {
		if (ccbitem->ccbId == ccbId)
			return ccbitem;
		ccbitem = ccbitem->next;
	}
	return NULL;
}

bool ccbutil_EmptyCcbExists() {
	if (ccbList == NULL) {
		return true;
	}
	return false;
}

struct CcbUtilCcbData *ccbutil_getCcbData(SaImmOiCcbIdT ccbId)
{
	struct CcbUtilCcbData *ccbitem = ccbutil_findCcbData(ccbId);
	if (ccbitem == NULL)
		ccbitem = ccbutil_createCcbData(ccbId);
	return ccbitem;
}

void ccbutil_deleteCcbData(struct CcbUtilCcbData *ccb)
{
	struct CcbUtilCcbData *item = ccbList;
	struct CcbUtilCcbData *prev = NULL;
	struct CcbUtilOperationData *op;
	if (ccb == NULL)
		return;
	while (item != NULL) {
		if (ccb->ccbId == item->ccbId) {
			if (prev == NULL) {
				ccbList = item->next;
			} else {
				prev->next = item->next;
			}

			op = item->operationListHead;
			while(op) {
				osaf_extended_name_free(&op->objectName);
				op = op->next;
				if(op == item->operationListTail)
					break;
			}
		}
		prev = item;
		item = item->next;
	}
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	deleteClist(clist);
}

static struct CcbUtilOperationData *newOperationData(struct CcbUtilCcbData *ccb, enum CcbUtilOperationType type)
{
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	struct CcbUtilOperationData *operation = (struct CcbUtilOperationData *)
	    clistMalloc(clist, sizeof(struct CcbUtilOperationData));
	operation->operationType = type;
	if (ccb->operationListTail == NULL) {
		ccb->operationListTail = operation;
		ccb->operationListHead = operation;
	} else {
		ccb->operationListTail->next = operation;
		ccb->operationListTail = operation;
	}

	operation->ccbId = ccb->ccbId;
	return operation;
}

CcbUtilOperationData_t *ccbutil_ccbAddCreateOperation(struct CcbUtilCcbData *ccb,
	const SaImmClassNameT className,
	const SaNameT *parentName,
	const SaImmAttrValuesT_2 **attrValues)
{
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	struct CcbUtilOperationData *operation = newOperationData(ccb, CCBUTIL_CREATE);
	operation->param.create.className = dupSaImmClassNameT(clist, className);
	operation->param.create.parentName = dupSaNameT(clist, parentName);
	operation->param.create.attrValues = dupSaImmAttrValuesT_array(clist, attrValues);
	saAisNameLend("", &operation->objectName);
	return operation;
}

CcbUtilOperationData_t *ccbutil_ccbAddCreateOperation_2(struct CcbUtilCcbData *ccb,
	const SaNameT *objectName,
	const SaImmClassNameT className,
	const SaNameT *parentName,
	const SaImmAttrValuesT_2 **attrValues)
{
	const char *str;
	size_t len;
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	struct CcbUtilOperationData *operation = newOperationData(ccb, CCBUTIL_CREATE);
	operation->param.create.className = dupSaImmClassNameT(clist, className);
	operation->param.create.parentName = dupSaNameT(clist, parentName);
	operation->param.create.attrValues = dupSaImmAttrValuesT_array(clist, attrValues);

	str = saAisNameBorrow(objectName);
	assert(str != NULL);
	len = strlen(str);
	saAisNameLend(len < SA_MAX_UNEXTENDED_NAME_LENGTH ? str : strdup(str), &operation->objectName);

	return operation;
}

void ccbutil_ccbAddDeleteOperation(struct CcbUtilCcbData *ccb, const SaNameT *objectName)
{
	const char *str;
	size_t len;
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	struct CcbUtilOperationData *operation = newOperationData(ccb, CCBUTIL_DELETE);
	operation->param.deleteOp.objectName = dupSaNameT(clist, objectName);

	str = saAisNameBorrow(objectName);
	assert(str != NULL);
	len = strlen(str);
	saAisNameLend(len < SA_MAX_UNEXTENDED_NAME_LENGTH ? str : strdup(str), &operation->objectName);
}

int ccbutil_ccbAddModifyOperation(struct CcbUtilCcbData *ccb,
				   const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	const char *str;
	size_t len;
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	struct CcbUtilOperationData *operation;

	///* Do not allow multiple operations on object in same CCB */
	//	if (ccbutil_getCcbOpDataByDN(ccb->ccbId, objectName) != NULL)
	//		return -1;

	operation = newOperationData(ccb, CCBUTIL_MODIFY);
	operation->param.modify.objectName = dupSaNameT(clist, objectName);
	operation->param.modify.attrMods = dupSaImmAttrModificationT_array(clist, attrMods);

	str = saAisNameBorrow(objectName);
	assert(str != NULL);
	len = strlen(str);
	saAisNameLend(len < SA_MAX_UNEXTENDED_NAME_LENGTH ? str : strdup(str), &operation->objectName);

	return 0;
}

CcbUtilOperationData_t *ccbutil_getNextCcbOp(SaImmOiCcbIdT ccbId, CcbUtilOperationData_t *opData)
{
	if (opData == NULL) {
		CcbUtilCcbData_t *ccb = ccbutil_getCcbData(ccbId);
		return ccb->operationListHead;
	}
	else
		return opData->next;
}

CcbUtilOperationData_t *ccbutil_getCcbOpDataByDN(SaImmOiCcbIdT ccbId, const SaNameT *dn)
{
	CcbUtilOperationData_t *opData = ccbutil_getNextCcbOp(ccbId, NULL);
	const char *dnStr = saAisNameBorrow(dn);
	assert(dnStr != NULL);

	while (opData != NULL) {
		if (strcmp(dnStr, saAisNameBorrow(&opData->objectName)) == 0)
			break;

		opData = ccbutil_getNextCcbOp(ccbId, opData);
	}

	return opData;
}

/* ----------------------------------------------------------------------
 * General IMM help utilities;
 */

char *immutil_strdup(struct CcbUtilCcbData *ccb, char const *source)
{
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	return dupStr(clist, source);
}

char const *immutil_getClassName(struct CcbUtilCcbData *ccb, SaImmHandleT immHandle, const SaNameT *objectName)
{
	struct Chunk *clist = (struct Chunk *)ccb->memref;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;
	SaImmAttrValuesT_2 *cnameattr;
	SaImmAttrNameT classNameAttr[2] = { SA_IMM_ATTR_CLASS_NAME, NULL };
	char *cname = NULL;

	if (objectName == NULL)
		return NULL;
	if (immutil_saImmOmAccessorInitialize(immHandle, &accessorHandle) != SA_AIS_OK)
		return NULL;

	if (immutil_saImmOmAccessorGet_2(accessorHandle, objectName, classNameAttr, &attributes)
	    != SA_AIS_OK)
		goto finalize;
	if (attributes == NULL || *attributes == NULL)
		goto finalize;

	cnameattr = *attributes;
	if (strcmp(cnameattr->attrName, SA_IMM_ATTR_CLASS_NAME) != 0)
		goto finalize;
	assert(cnameattr->attrValueType == SA_IMM_ATTR_SASTRINGT);
	assert(cnameattr->attrValuesNumber == 1);

	cname = dupStr(clist, *((char **)*(cnameattr->attrValues)));

 finalize:
	(void)immutil_saImmOmAccessorFinalize(accessorHandle);
	return cname;
}

char const *immutil_getStringValue(char const *key, SaNameT const *name)
{
	const char* buffer = saAisNameBorrow(name);
	unsigned int klen;
	char *cp;

	assert(buffer != NULL);
	assert(key != NULL);
	klen = strlen(key);
	assert(klen > 1 || key[klen - 1] == '=');

	cp = strstr(buffer, key);
	while (cp != NULL) {
		if (cp == buffer || cp[-1] == ',') {
			char *value = cp + klen;
			if (*value == 0 || *value == ',')
				return NULL;
			cp = strchr(value, ',');
			if (cp != NULL)
				*cp = 0;
			return value;
		}
		cp += klen;
		cp = strstr(cp, key);
	}
	return NULL;
}

char const *immutil_getDnItem(SaNameT const *name, unsigned int index)
{
	static char* buffer = NULL;
	char *cp;
	char *value;
	size_t size;
	const char *objName = saAisNameBorrow(name);
	assert(objName != NULL);

	size = strlen(objName) + 1;
	buffer = realloc(buffer, size);
	memcpy(buffer, objName, size);
	value = buffer;
	cp = strchr(value, ',');
	while (index > 0) {
		if (cp == NULL)
			return NULL;
		value = cp + 1;
		cp = strchr(value, ',');
		index--;
	}
	if (cp != NULL)
		*cp = 0;
	return value;
}

long immutil_getNumericValue(char const *key, SaNameT const *name)
{
	char const *vp = immutil_getStringValue(key, name);
	char *endptr;
	long value;

	if (vp == NULL)
		return LONG_MIN;
	value = strtol(vp, &endptr, 0);
	if (endptr == NULL || endptr == vp || (*endptr != 0 && *endptr != ','))
		return LONG_MIN;
	return value;
}

char const *immutil_strnchr(char const *str, int c, size_t length)
{
	while (length > 0 && *str != 0) {
		if (*str == c)
			return str;
		str++;
		length--;
	}
	return NULL;
}

const SaNameT *immutil_getNameAttr(const SaImmAttrValuesT_2 **attr, char const *name, unsigned int index)
{
	unsigned int i;
	if (attr == NULL || attr[0] == NULL)
		return NULL;
	for (i = 0; attr[i] != NULL; i++) {
		if (strcmp(attr[i]->attrName, name) == 0) {
			if (index >= attr[i]->attrValuesNumber
			    || attr[i]->attrValues == NULL || attr[i]->attrValueType != SA_IMM_ATTR_SANAMET)
				return NULL;
			return (SaNameT *)attr[i]->attrValues[index];
		}
	}
	return NULL;
}

char const *immutil_getStringAttr(const SaImmAttrValuesT_2 **attr, char const *name, unsigned int index)
{
	unsigned int i;
	if (attr == NULL || attr[0] == NULL)
		return NULL;
	for (i = 0; attr[i] != NULL; i++) {
		if (strcmp(attr[i]->attrName, name) == 0) {
			if (index >= attr[i]->attrValuesNumber
			    || attr[i]->attrValues == NULL || attr[i]->attrValueType != SA_IMM_ATTR_SASTRINGT)
				return NULL;
			return *((const SaStringT *)attr[i]->attrValues[index]);
		}
	}
	return NULL;
}

SaAisErrorT immutil_getAttrValuesNumber(
	const char *attrName, const SaImmAttrValuesT_2 **attr, SaUint32T *attrValuesNumber)
{
        SaAisErrorT error = SA_AIS_ERR_NAME_NOT_FOUND;
        int i;

        if (attr == NULL || attr[0] == NULL)
                return SA_AIS_ERR_INVALID_PARAM;

        for (i = 0; attr[i] != NULL; i++) {
                if (strcmp(attr[i]->attrName, attrName) == 0) {
			*attrValuesNumber = attr[i]->attrValuesNumber;
                        error = SA_AIS_OK;
                        break;
                }
        }

	return error;
}

/* note: SA_IMM_ATTR_SASTRINGT is intentionally not supported */
SaAisErrorT immutil_getAttr(const char *attrName,
	const SaImmAttrValuesT_2 **attr, SaUint32T index, void *param)
{
        SaAisErrorT error = SA_AIS_ERR_NAME_NOT_FOUND;
        int i;

        if (attr == NULL || attr[0] == NULL)
                return SA_AIS_ERR_INVALID_PARAM;

        for (i = 0; attr[i] != NULL; i++) {
/*
           LOG_NO("TESTING immutil_getAttr1 attr[i]->attrName=%s attrName=%s i=%d attr[i]->attrValuesNumber=%d", attr[i]->attrName, attrName, i, attr[i]->attrValuesNumber);
*/
                if (strcmp(attr[i]->attrName, attrName) == 0) {
                        if ((index >= attr[i]->attrValuesNumber) || (attr[i]->attrValues == NULL)) {
                                error = SA_AIS_ERR_INVALID_PARAM;
                                goto done;
                        }
/*
                        LOG_NO("TESTING immutil_getAttr2 attrName=%s i=%d attr[i]->attrValuesNumber=%d", attrName, i, attr[i]->attrValuesNumber);
*/
                        switch (attr[i]->attrValueType) {
                                case SA_IMM_ATTR_SAINT32T:
                                        *((SaInt32T*) param) = *((SaInt32T*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SAUINT32T:
                                        *((SaUint32T*) param) = *((SaUint32T*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SAINT64T:
                                        *((SaInt64T*) param) = *((SaInt64T*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SAUINT64T:
                                        *((SaUint64T*) param) = *((SaUint64T*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SATIMET:
                                        *((SaTimeT*) param) = *((SaTimeT*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SANAMET:
                                        *((SaNameT*) param) = *((SaNameT*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SAFLOATT:
                                        *((SaFloatT*) param) = *((SaFloatT*) attr[i]->attrValues[index]);
                                        break;
                                case SA_IMM_ATTR_SADOUBLET:
                                        *((SaDoubleT*) param) = *((SaDoubleT*) attr[i]->attrValues[index]);
                                        break;
                                default:
                                        error = SA_AIS_ERR_INVALID_PARAM;
					abort();
                                        goto done;
                                        break;
                        }

                        error = SA_AIS_OK;
                        break;
                }
        }

 done:
        return error;
}

const SaTimeT* immutil_getTimeAttr(const SaImmAttrValuesT_2 **attr, char const* name, unsigned int index)
{
        unsigned int i;
        if (attr == NULL || attr[0] == NULL) 
                return NULL;
        for (i = 0; attr[i] != NULL; i++) {
                if (strcmp(attr[i]->attrName, name) == 0) {
                        if (index >= attr[i]->attrValuesNumber 
                            || attr[i]->attrValues == NULL
                            || attr[i]->attrValueType != SA_IMM_ATTR_SATIMET)
                                return NULL;
                        return (SaTimeT*)attr[i]->attrValues[index];
                }		
        }
        return NULL;
}

const SaUint32T *immutil_getUint32Attr(const SaImmAttrValuesT_2 **attr, char const *name, unsigned int index)
{
	unsigned int i;
	if (attr == NULL || attr[0] == NULL)
		return NULL;
	for (i = 0; attr[i] != NULL; i++) {
		if (strcmp(attr[i]->attrName, name) == 0) {
			if (index >= attr[i]->attrValuesNumber
			    || attr[i]->attrValues == NULL || attr[i]->attrValueType != SA_IMM_ATTR_SAUINT32T)
				return NULL;
			return (SaUint32T *)attr[i]->attrValues[index];
		}
	}
	return NULL;
}

int immutil_matchName(SaNameT const *name, regex_t const *preg)
{
	const char* buffer;
	assert(name != NULL && preg != NULL);
	buffer = saAisNameBorrow(name);
	assert(buffer != NULL);
	return regexec(preg, buffer, 0, NULL, 0);
}

SaAisErrorT immutil_update_one_rattr(SaImmOiHandleT immOiHandle,
				     const char *dn,
				     SaImmAttrNameT attributeName, SaImmValueTypeT attrValueType, void *value)
{
	SaImmAttrModificationT_2 attrMod;
	const SaImmAttrModificationT_2 *attrMods[] = { &attrMod, NULL };
	SaImmAttrValueT attrValues[] = { value };
	SaNameT objectName;

	saAisNameLend(dn, &objectName);

	attrMod.modType = SA_IMM_ATTR_VALUES_REPLACE;
	attrMod.modAttr.attrName = attributeName;
	attrMod.modAttr.attrValuesNumber = 1;
	attrMod.modAttr.attrValueType = attrValueType;
	attrMod.modAttr.attrValues = attrValues;
	return immutil_saImmOiRtObjectUpdate_2(immOiHandle, &objectName, attrMods);
}

SaImmClassNameT immutil_get_className(const SaNameT *objectName)
{
	SaImmHandleT omHandle;
	SaImmClassNameT className = NULL;
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2 **attributes;
	SaImmAttrNameT attributeNames[] = { "SaImmAttrClassName", NULL };

	if(immutil_saImmOmInitialize(&omHandle, NULL, &immVersion) != SA_AIS_OK)
		goto done;
	if(immutil_saImmOmAccessorInitialize(omHandle, &accessorHandle) != SA_AIS_OK)
		goto finalize_om_handle;
	if (immutil_saImmOmAccessorGet_2(accessorHandle, objectName, attributeNames, &attributes) == SA_AIS_OK)
		className = strdup(*((char **)attributes[0]->attrValues[0]));
	(void)immutil_saImmOmAccessorFinalize(accessorHandle);

finalize_om_handle:
	(void)immutil_saImmOmFinalize(omHandle);

done:
	return className;
}

SaAisErrorT immutil_get_attrValueType(const SaImmClassNameT className,
	SaImmAttrNameT attrName, SaImmValueTypeT *attrValueType)
{
	SaAisErrorT rc;
	SaImmHandleT omHandle;
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 *attrDef;
	SaImmAttrDefinitionT_2 **attrDefinitions;
	int i = 0;

	if((rc = immutil_saImmOmInitialize(&omHandle, NULL, &immVersion)) != SA_AIS_OK) {
		return rc;
	}

	if ((rc = saImmOmClassDescriptionGet_2(omHandle, className, &classCategory, &attrDefinitions)) != SA_AIS_OK)
		goto done;

	rc = SA_AIS_ERR_INVALID_PARAM;
	while ((attrDef = attrDefinitions[i++]) != NULL) {
		if (!strcmp(attrName, attrDef->attrName)) {
			*attrValueType = attrDef->attrValueType;
			rc = SA_AIS_OK;
			break;
		}
	}

	(void)saImmOmClassDescriptionMemoryFree_2(omHandle, attrDefinitions);

done:
	(void)immutil_saImmOmFinalize(omHandle);
	return rc;
}

void *immutil_new_attrValue(SaImmValueTypeT attrValueType, const char *str)
{
	void *attrValue = NULL;
	size_t len;
	char *endptr;

	/*
	** sizeof(long) varies between 32 and 64 bit machines. Therefore on a 
	** 64 bit machine, a check is needed to ensure that the value returned
	** from strtol() or strtoul() is not greater than what fits into 32 bits.
	*/
	switch (attrValueType) {
	case SA_IMM_ATTR_SAINT32T: {
		errno = 0;
		long value = strtol(str, &endptr, 0);
		SaInt32T attr_value = value;
		if ((errno != 0) || (endptr == str) || (*endptr != '\0')) {
			fprintf(stderr, "int32 conversion failed\n");
			return NULL;
		}
		if (value != attr_value) {
			printf("int32 conversion failed, value too large\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaInt32T));
		*((SaInt32T *)attrValue) = value;
		break;
	}
	case SA_IMM_ATTR_SAUINT32T: {
		errno = 0;
		unsigned long value = strtoul(str, &endptr, 0);
		SaUint32T attr_value = value;
		if ((errno != 0) || (endptr == str) || (*endptr != '\0')) {
			fprintf(stderr, "uint32 conversion failed\n");
			return NULL;
		}
		if (value != attr_value) {
			printf("uint32 conversion failed, value too large\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaUint32T));
		*((SaUint32T *)attrValue) = value;
		break;
	}
	case SA_IMM_ATTR_SAINT64T:
		// fall-through, same basic data type
	case SA_IMM_ATTR_SATIMET: {
		errno = 0;
		long long value = strtoll(str, &endptr, 0);
		if ((errno != 0) || (endptr == str) || (*endptr != '\0')) {
			fprintf(stderr, "int64 conversion failed\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaInt64T));
		*((SaInt64T *)attrValue) = value;
		break;
	}
	case SA_IMM_ATTR_SAUINT64T: {
		errno = 0;
		unsigned long long value = strtoull(str, &endptr, 0);
		if ((errno != 0) || (endptr == str) || (*endptr != '\0')) {
			fprintf(stderr, "uint64 conversion failed\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaUint64T));
		*((SaUint64T *)attrValue) = value;
		break;
	}
	case SA_IMM_ATTR_SAFLOATT: {
		errno = 0;
		float myfloat = strtof(str, &endptr);
		if (((myfloat == 0) && (endptr == str)) ||
		    (errno == ERANGE) || (*endptr != '\0')) {
			fprintf(stderr, "float conversion failed\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaFloatT));
		*((SaFloatT *)attrValue) = myfloat;
		break;
	}
	case SA_IMM_ATTR_SADOUBLET: {
		errno = 0;
		double mydouble = strtod(str, &endptr);
		if (((mydouble == 0) && (endptr == str)) ||
		    (errno == ERANGE) || (*endptr != '\0')) {
			fprintf(stderr, "double conversion failed\n");
			return NULL;
		}
		attrValue = malloc(sizeof(SaDoubleT));
		*((SaDoubleT *)attrValue) = mydouble;
		break;
	}
	case SA_IMM_ATTR_SANAMET: {
		SaNameT *mynamet;
		len = strlen(str);
		attrValue = mynamet = malloc(sizeof(SaNameT));
		saAisNameLend(len < SA_MAX_UNEXTENDED_NAME_LENGTH ? str : strdup(str), mynamet);
		break;
	}
	case SA_IMM_ATTR_SASTRINGT: {
		attrValue = malloc(sizeof(SaStringT));
		*((SaStringT *)attrValue) = strdup(str);
		break;
	}
	case SA_IMM_ATTR_SAANYT: {
		char* endMark;
		SaBoolT even = SA_TRUE;
		char byte[5];
		unsigned int i;

		len = strlen(str);
		if(len % 2) {
			len = len/2 + 1;
			even = SA_FALSE;
		} else {
			len = len/2;
		}
		attrValue = malloc(sizeof(SaAnyT));
		((SaAnyT*)attrValue)->bufferAddr = 
			(SaUint8T*)malloc(sizeof(SaUint8T) * len);
		((SaAnyT*)attrValue)->bufferSize = len;

		byte[0] = '0';
		byte[1] = 'x';
		byte[4] = '\0';

		endMark = byte + 4;

		for (i = 0; i < len; i++) {
			byte[2] = str[2*i];
			if(even || (i + 1 < len)) {
				byte[3] = str[2*i + 1];
			} else {
				byte[3] = '0';
			}
			((SaAnyT*)attrValue)->bufferAddr[i] = 
				(SaUint8T)strtod(byte, &endMark);
		}
	}
	default:
		break;
	}

	return attrValue;
}

/* ----------------------------------------------------------------------
 * SA-item duplicate functions
 */

static char *dupStr(struct Chunk *clist, const char *original)
{
	unsigned int len;
	if (original == NULL)
		return NULL;
	len = strlen(original) + 1;
	SaImmClassNameT copy = (SaImmClassNameT)clistMalloc(clist, len);
	memcpy(copy, original, len);
	return copy;
}

static const SaNameT *dupSaNameT(struct Chunk *clist, const SaNameT *original)
{
	SaNameT *copy;
	if (original == NULL)
		return NULL;
	const char* value = saAisNameBorrow(original);
	assert(value != NULL);
	copy = (SaNameT *)clistMalloc(clist, sizeof(SaNameT));
	saAisNameLend(strlen(value) < SA_MAX_UNEXTENDED_NAME_LENGTH ? value : dupStr(clist, value), copy);
	return copy;
}

static SaImmClassNameT dupSaImmClassNameT(struct Chunk *clist, const SaImmClassNameT original)
{
	return dupStr(clist, (const char *)original);
}

/*
 * The "SaImmAttrValueT* attrValues" field;
 *
 *                  +-------+
 * attrValues ----> ! void* + --------> value0
 *                  +-------+
 *                  ! void* + --------> value1
 *                  +-------+
 *                  ! void* + --------> value2
 *                  +-------+
 *                  !       !
 *
 * Especially silly case for one SaString;
 *
 *                  +-------+         +-------+
 * attrValues ----> ! void* + ------->! char* !------> char-data
 *                  +-------+         +-------+
 *
 */

static void copySaImmAttrValuesT(struct Chunk *clist, SaImmAttrValuesT_2 *copy, const SaImmAttrValuesT_2 *original)
{
	size_t valueSize = 0;
	unsigned int i, valueCount = original->attrValuesNumber;
	char *databuffer;
	copy->attrName = dupStr(clist, (const char *)original->attrName);
	copy->attrValuesNumber = valueCount;
	copy->attrValueType = original->attrValueType;
	if (valueCount == 0)
		return;		/* (just in case...) */
	copy->attrValues = clistMalloc(clist, valueCount * sizeof(SaImmAttrValueT));

	switch (original->attrValueType) {
	case SA_IMM_ATTR_SAINT32T:
		valueSize = sizeof(SaInt32T);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		valueSize = sizeof(SaUint32T);
		break;
	case SA_IMM_ATTR_SAINT64T:
		valueSize = sizeof(SaInt64T);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		valueSize = sizeof(SaUint64T);
		break;
	case SA_IMM_ATTR_SATIMET:
		valueSize = sizeof(SaTimeT);
		break;
	case SA_IMM_ATTR_SANAMET:
		valueSize = sizeof(SaNameT);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		valueSize = sizeof(SaFloatT);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		valueSize = sizeof(SaDoubleT);
		break;
	case SA_IMM_ATTR_SASTRINGT:
		valueSize = sizeof(SaStringT);
		break;
	case SA_IMM_ATTR_SAANYT:
		valueSize = sizeof(SaAnyT);
		break;
	}

	databuffer = (char *)clistMalloc(clist, valueCount * valueSize);
	for (i = 0; i < valueCount; i++) {
		copy->attrValues[i] = databuffer;
		if (original->attrValueType == SA_IMM_ATTR_SASTRINGT) {
			char *cporig = *((char **)original->attrValues[i]);
			char **cpp = (char **)databuffer;
			*cpp = dupStr(clist, cporig);
		} else if(original->attrValueType == SA_IMM_ATTR_SANAMET) {
			SaNameT* cporig = (SaNameT*) original->attrValues[i];
			SaNameT* cpdest = (SaNameT*) copy->attrValues[i];
			const char* value = saAisNameBorrow(cporig);
			assert(value != NULL);
			saAisNameLend(strlen(value) < SA_MAX_UNEXTENDED_NAME_LENGTH ? value :
				dupStr(clist, value), cpdest);
		} else if(original->attrValueType == SA_IMM_ATTR_SAANYT) {
			SaAnyT* cporig = (SaAnyT *) original->attrValues[i];
			SaAnyT* cpdest = (SaAnyT *) copy->attrValues[i];
			cpdest->bufferSize = cporig->bufferSize;
			if(cpdest->bufferSize) {
				cpdest->bufferAddr = clistMalloc(clist, cpdest->bufferSize);
				memcpy(cpdest->bufferAddr, cporig->bufferAddr, cpdest->bufferSize);
			}
		} else {
			memcpy(databuffer, original->attrValues[i], valueSize);
		}
		databuffer += valueSize;
	}
}

static const SaImmAttrValuesT_2 *dupSaImmAttrValuesT(struct Chunk *clist, const SaImmAttrValuesT_2 *original)
{
	SaImmAttrValuesT_2 *copy = (SaImmAttrValuesT_2 *)
	    clistMalloc(clist, sizeof(SaImmAttrValuesT_2));
	copySaImmAttrValuesT(clist, copy, original);
	return copy;
}

static const SaImmAttrModificationT_2 *dupSaImmAttrModificationT(struct Chunk *clist,
								 const SaImmAttrModificationT_2 *original)
{
	SaImmAttrModificationT_2 *copy = (SaImmAttrModificationT_2 *)
	    clistMalloc(clist, sizeof(SaImmAttrModificationT_2));
	copy->modType = original->modType;
	copySaImmAttrValuesT(clist, &(copy->modAttr), &(original->modAttr));
	return copy;
}

static const SaImmAttrValuesT_2 **dupSaImmAttrValuesT_array(struct Chunk *clist, const SaImmAttrValuesT_2 **original)
{
	const SaImmAttrValuesT_2 **copy;
	unsigned int i, alen = 0;
	if (original == NULL)
		return NULL;
	while (original[alen] != NULL)
		alen++;
	copy = (const SaImmAttrValuesT_2 **)
	    clistMalloc(clist, (alen + 1) * sizeof(SaImmAttrValuesT_2 *));
	for (i = 0; i < alen; i++) {
		copy[i] = dupSaImmAttrValuesT(clist, original[i]);
	}
	return copy;
}

static const SaImmAttrModificationT_2 **dupSaImmAttrModificationT_array(struct Chunk *clist,
									const SaImmAttrModificationT_2 **original)
{
	const SaImmAttrModificationT_2 **copy;
	unsigned int i, alen = 0;
	if (original == NULL)
		return NULL;
	while (original[alen] != NULL)
		alen++;
	copy = (const SaImmAttrModificationT_2 **)
	    clistMalloc(clist, (alen + 1) * sizeof(SaImmAttrModificationT_2 *));
	for (i = 0; i < alen; i++) {
		copy[i] = dupSaImmAttrModificationT(clist, original[i]);
	}
	return copy;
}

/* ----------------------------------------------------------------------
 * Memory handling
 */

struct Chunk {
	struct Chunk *next;
	unsigned int capacity;
	unsigned int free;
	unsigned char data[1];
};

static struct Chunk *newChunk(struct Chunk *next, size_t size)
{
	struct Chunk *chunk = (struct Chunk *)malloc(sizeof(struct Chunk) + size);
	if (chunk == NULL)
		immutilError("Out of memory");
	chunk->next = next;
	chunk->capacity = size;
	chunk->free = size;
	return chunk;
}

static void deleteClist(struct Chunk *clist)
{
	while (clist != NULL) {
		struct Chunk *chunk = clist;
		clist = clist->next;
		free(chunk);
	}
}

static void *clistMalloc(struct Chunk *clist, size_t size)
{
	struct Chunk *chunk;

	size = (size + 3) & ~3;

	if (size > CHUNK) {
		chunk = newChunk(clist->next, size);
		clist->next = chunk;
		chunk->free = 0;
		memset(chunk->data, 0, size);
		return chunk->data;
	}

	for (chunk = clist; chunk != NULL; chunk = chunk->next) {
		if (chunk->free >= size) {
			unsigned char *mem = chunk->data + (chunk->capacity - chunk->free);
			chunk->free -= size;
			memset(mem, 0, size);
			return mem;
		}
	}

	chunk = newChunk(clist->next, CHUNK);
	clist->next = chunk;
	chunk->free -= size;
	memset(chunk->data, 0, size);
	return chunk->data;
}

/* ----------------------------------------------------------------------
 * IMM call wrappers; This wrapper interface offloads the burden to
 * handle return values and retries for each and every IMM-call. It
 * makes the code cleaner.
 */

struct ImmutilWrapperProfile immutilWrapperProfile = { 1, 25, 400 };

SaAisErrorT immutil_saImmOiInitialize_2(SaImmOiHandleT *immOiHandle,
    const SaImmOiCallbacksT_2 *immOiCallbacks,
    const SaVersionT *version)
{
    /* Version parameter is in/out i.e. must be mutable and should not be
       re-used from previous call in a retry loop. */
    SaVersionT localVer = *version;

	SaAisErrorT rc = saImmOiInitialize_2(immOiHandle, immOiCallbacks, &localVer);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
        localVer = *version;
		rc = saImmOiInitialize_2(immOiHandle, immOiCallbacks, &localVer);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiInitialize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiSelectionObjectGet(SaImmOiHandleT immOiHandle, SaSelectionObjectT *selectionObject)
{
	SaAisErrorT rc = saImmOiSelectionObjectGet(immOiHandle, selectionObject);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiSelectionObjectGet(immOiHandle, selectionObject);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiSelectionObjectGet FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiClassImplementerSet(SaImmOiHandleT immOiHandle, const char *className)
{
	SaAisErrorT rc = saImmOiClassImplementerSet(immOiHandle,
			(const SaImmClassNameT) className);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiClassImplementerSet(immOiHandle,
				(const SaImmClassNameT) className);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiClassImplementerSet FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiClassImplementerRelease(SaImmOiHandleT immOiHandle, const char *className)
{
	SaAisErrorT rc = saImmOiClassImplementerRelease(immOiHandle,
			(const SaImmClassNameT) className);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiClassImplementerRelease(immOiHandle,
				(const SaImmClassNameT) className);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiClassImplementerRelease FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiImplementerSet(SaImmOiHandleT immOiHandle, const SaImmOiImplementerNameT implementerName)
{
	SaAisErrorT rc = saImmOiImplementerSet(immOiHandle, implementerName);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiImplementerSet(immOiHandle, implementerName);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiImplementerSet FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiImplementerClear(SaImmOiHandleT immOiHandle)
{
	SaAisErrorT rc = saImmOiImplementerClear(immOiHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiImplementerClear(immOiHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiImplementerClear FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectCreate_2(SaImmOiHandleT immOiHandle,
					    const SaImmClassNameT className,
					    const SaNameT *parentName, const SaImmAttrValuesT_2 **attrValues)
{
	SaAisErrorT rc = saImmOiRtObjectCreate_2(immOiHandle, className, parentName, attrValues);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiRtObjectCreate_2(immOiHandle, className, parentName, attrValues);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiRtObjectCreate_2 FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectCreate_o2(SaImmOiHandleT immOiHandle,
                                            const SaImmClassNameT className,
                                            const char* parentName, const SaImmAttrValuesT_2 **attrValues)
{
	SaNameT parent_name;
	if(parentName)
		osaf_extended_name_lend(parentName,&parent_name); 
	else
		osaf_extended_name_clear(&parent_name);
	
	SaAisErrorT rc = immutil_saImmOiRtObjectCreate_2(immOiHandle, className, &parent_name, attrValues);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectDelete(SaImmOiHandleT immOiHandle, const SaNameT *objectName)
{
	SaAisErrorT rc = saImmOiRtObjectDelete(immOiHandle, objectName);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiRtObjectDelete(immOiHandle, objectName);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiRtObjectDelete FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectDelete_o2(SaImmOiHandleT immOiHandle, const char *objectName)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name); 
	else
		osaf_extended_name_clear(&obj_name);

	SaAisErrorT rc = immutil_saImmOiRtObjectDelete(immOiHandle, &obj_name);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectUpdate_2(SaImmOiHandleT immOiHandle,
					    const SaNameT *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaAisErrorT rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiRtObjectUpdate_2(immOiHandle, objectName, attrMods);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiRtObjectUpdate_2 FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiRtObjectUpdate_o2(SaImmOiHandleT immOiHandle,
                                            const char *objectName, const SaImmAttrModificationT_2 **attrMods)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name); 
	else
		osaf_extended_name_clear(&obj_name);

	SaAisErrorT rc = immutil_saImmOiRtObjectUpdate_2(immOiHandle, &obj_name, attrMods);
	return rc;
}


SaAisErrorT immutil_saImmOiAdminOperationResult(SaImmOiHandleT immOiHandle,
						SaInvocationT invocation, SaAisErrorT result)
{
	SaAisErrorT rc = saImmOiAdminOperationResult(immOiHandle, invocation, result);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiAdminOperationResult(immOiHandle, invocation, result);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiAdminOperationResult FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOiAdminOperationResult_o2(SaImmOiHandleT immOiHandle,
                                                SaInvocationT invocation, SaAisErrorT result,
						const SaImmAdminOperationParamsT_2 **returnParams)
{
	SaAisErrorT rc = saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, returnParams);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOiAdminOperationResult_o2(immOiHandle, invocation, result, returnParams);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOiAdminOperationResult FAILED, rc = %d", (int)rc);
	return rc;
}


SaAisErrorT immutil_saImmOmInitialize(SaImmHandleT *immHandle, const SaImmCallbacksT *immCallbacks, const SaVersionT *version)
{
    /* Version parameter is in/out i.e. must be mutable and should not be
       re-used from previous call in a retry loop. */
    SaVersionT localVer = *version;

	SaAisErrorT rc = saImmOmInitialize(immHandle, immCallbacks, &localVer);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
        localVer = *version;
		rc = saImmOmInitialize(immHandle, immCallbacks, &localVer);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmInitialize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmFinalize(SaImmHandleT immHandle)
{
	SaAisErrorT rc = saImmOmFinalize(immHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmFinalize(immHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmFinalize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmAccessorInitialize(SaImmHandleT immHandle, SaImmAccessorHandleT *accessorHandle)
{
	SaAisErrorT rc = saImmOmAccessorInitialize(immHandle, accessorHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmAccessorInitialize(immHandle, accessorHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmAccessorInitialize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmAccessorGet_2(SaImmAccessorHandleT accessorHandle,
					 const SaNameT *objectName,
					 const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes)
{
	SaAisErrorT rc = saImmOmAccessorGet_2(accessorHandle, objectName, attributeNames, attributes);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmAccessorGet_2(accessorHandle, objectName, attributeNames, attributes);
		nTries++;
	}
	if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST) &&
            immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmAccessorGet FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmAccessorGet_o2(SaImmAccessorHandleT accessorHandle,
                                         const char *objectName,
                                         const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name);
	else
		osaf_extended_name_clear(&obj_name);
		
	SaAisErrorT rc = immutil_saImmOmAccessorGet_2(accessorHandle, &obj_name, attributeNames, attributes);
	return rc; 
}

SaAisErrorT immutil_saImmOmAccessorGetConfigAttrs(SaImmAccessorHandleT accessorHandle,
					 const SaNameT *objectName,
					 SaImmAttrValuesT_2 ***attributes)
{
	SaImmAttrNameT accessorGetConfigAttrsToken[2] = {"SA_IMM_SEARCH_GET_CONFIG_ATTR", NULL };
	/* This is a hack to cater for the very common simple case of the OM user needing
	   to access ONE object, but only its config attributes. The saImmOmAccessorGet_2 call 
	   has no search-options. The trick here is to "tunnel" a search option through the
	   attributes parameter. The user will get ALL config attributes for the object in
	   this way. If they only want some of the config attributes, then they just do a
	   regular accessor get and enumerate the attributes they want.

	   The support for thus is really inside the implementation of saImmOmSearchInitialize,
	   which saImmOmAccessorGet_2 uses in its implementation. If it detects 
	   only one attribute in the attributes list and the name of that attribute is
	   'SA_IMM_SEARCH_GET_CONFIG_ATTR' then it will assume that the user does not actually
	   want an attribute with *that* name, but wants all config attribues. 
	   This feature is only available with the A.2.11 version of the IMMA-API.
	 */

	SaAisErrorT rc = saImmOmAccessorGet_2(accessorHandle, objectName, accessorGetConfigAttrsToken, attributes);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmAccessorGet_2(accessorHandle, objectName, NULL, attributes);
		nTries++;
	}
	if ((rc != SA_AIS_OK) && (rc != SA_AIS_ERR_NOT_EXIST) &&
            immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmAccessorGet FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmAccessorFinalize(SaImmAccessorHandleT accessorHandle)
{
	SaAisErrorT rc = saImmOmAccessorFinalize(accessorHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmAccessorFinalize(accessorHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmAccessorFinalize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmSearchInitialize_2(SaImmHandleT immHandle,
					      const SaNameT *rootName,
					      SaImmScopeT scope,
					      SaImmSearchOptionsT searchOptions,
					      const SaImmSearchParametersT_2 *searchParam,
					      const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	SaAisErrorT rc = saImmOmSearchInitialize_2(immHandle, rootName, scope, searchOptions, searchParam,
						   attributeNames, searchHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmSearchInitialize_2(immHandle, rootName, scope, searchOptions, searchParam,
					       attributeNames, searchHandle);
		nTries++;
	}
	if (rc == SA_AIS_ERR_NOT_EXIST)
		return rc;
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmSearchInitialize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmSearchInitialize_o2(SaImmHandleT immHandle,
                                              const char *rootName,
                                              SaImmScopeT scope,
                                              SaImmSearchOptionsT searchOptions,
                                              const SaImmSearchParametersT_2 *searchParam,
                                              const SaImmAttrNameT *attributeNames, SaImmSearchHandleT *searchHandle)
{
	SaNameT root_name;
	if(rootName)
		osaf_extended_name_lend(rootName,&root_name); 
	else
		osaf_extended_name_clear(&root_name);

	SaAisErrorT rc = immutil_saImmOmSearchInitialize_2(immHandle, &root_name, scope, searchOptions, searchParam,
                                              			attributeNames, searchHandle);
	return rc;
}


SaAisErrorT immutil_saImmOmSearchFinalize(SaImmSearchHandleT searchHandle)
{
	SaAisErrorT rc = saImmOmSearchFinalize(searchHandle);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmSearchFinalize(searchHandle);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmSearchFinalize FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmSearchNext_2(SaImmSearchHandleT searchHandle,
					SaNameT *objectName, SaImmAttrValuesT_2 ***attributes)
{
	SaAisErrorT rc = saImmOmSearchNext_2(searchHandle, objectName, attributes);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmSearchNext_2(searchHandle, objectName, attributes);
		nTries++;
	}
	if (rc == SA_AIS_ERR_NOT_EXIST)
		return rc;
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmSearchNext FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmSearchNext_o2(SaImmSearchHandleT searchHandle,
                                        char **objectName, SaImmAttrValuesT_2 ***attributes)
{
	SaNameT obj_name;
	const char * obj;

	SaAisErrorT rc = immutil_saImmOmSearchNext_2(searchHandle, &obj_name, attributes);
	if (rc == SA_AIS_OK){
		obj= osaf_extended_name_borrow(&obj_name);
		*objectName = (char *) malloc (strlen(obj) +1);
		strcpy(*objectName, obj);
	}
	else
		objectName = NULL;

	return rc;
}


SaAisErrorT immutil_saImmOmAdminOwnerClear(SaImmHandleT immHandle, const SaNameT **objectNames, SaImmScopeT scope)
{
	SaAisErrorT rc = saImmOmAdminOwnerClear(immHandle, objectNames, scope);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmAdminOwnerClear(immHandle, objectNames, scope);
		nTries++;
	}
	if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmAdminOwnerClear FAILED, rc = %d", (int)rc);
	return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerClear_o2(SaImmHandleT immHandle, const char **objectNames, SaImmScopeT scope)
{
	int i=0;
	
	while (objectNames[i]){
		 i++;
	}
	SaNameT ** obj_names = (SaNameT **) malloc((i+1)* sizeof(SaNameT *));
	i=0;
	
	while (objectNames[i]){
		obj_names[i]=(SaNameT *) malloc(sizeof(SaNameT));
		osaf_extended_name_lend(objectNames[i],obj_names[i]);
		i++;
	}
	obj_names[i]=NULL;
			
	SaAisErrorT rc = immutil_saImmOmAdminOwnerClear(immHandle, (const SaNameT**)obj_names, scope);

	i=0;
	while(obj_names[i]){
		free(obj_names[i]);
		i++;
	}
	free(obj_names);

	return rc;
}

SaAisErrorT immutil_saImmOmClassCreate_2(SaImmCcbHandleT immCcbHandle,
                                             const SaImmClassNameT className,
                                             const SaImmClassCategoryT classCategory,
                                             const SaImmAttrDefinitionT_2** attrDefinitions)
{
        SaAisErrorT rc = saImmOmClassCreate_2(immCcbHandle, className, classCategory, attrDefinitions);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmClassCreate_2(immCcbHandle, className, classCategory, attrDefinitions);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmClassCreate_2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmClassDelete(SaImmCcbHandleT immCcbHandle,
					 const SaImmClassNameT className)
{
	SaAisErrorT rc = saImmOmClassDelete(immCcbHandle, className);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmClassDelete(immCcbHandle, className);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmClassDelete FAILED, rc = %d", (int)rc);
        return rc;
}



////////////////////////////////////////////
SaAisErrorT immutil_saImmOiFinalize(SaImmOiHandleT immOiHandle)
{
        SaAisErrorT rc = saImmOiFinalize(immOiHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOiFinalize(immOiHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOiFinalize FAILED, rc = %d", (int)rc);
        return rc;
}


SaAisErrorT immutil_saImmOmAdminOwnerInitialize(SaImmHandleT immHandle,
                                                const SaImmAdminOwnerNameT admOwnerName,
                                                SaBoolT relOwnOnFinalize,
                                                SaImmAdminOwnerHandleT *ownerHandle)
{
        SaAisErrorT rc = saImmOmAdminOwnerInitialize(immHandle, admOwnerName, relOwnOnFinalize, ownerHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOwnerInitialize(immHandle, admOwnerName, relOwnOnFinalize, ownerHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOwnerInitialize FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerFinalize(SaImmAdminOwnerHandleT ownerHandle)
{
        SaAisErrorT rc = saImmOmAdminOwnerFinalize(ownerHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOwnerFinalize(ownerHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOwnerFinalize FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbInitialize(SaImmAdminOwnerHandleT ownerHandle,
                                         SaImmCcbFlagsT ccbFlags,
                                         SaImmCcbHandleT *immCcbHandle)
{
        SaAisErrorT rc = saImmOmCcbInitialize(ownerHandle, ccbFlags, immCcbHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbInitialize(ownerHandle, ccbFlags, immCcbHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbInitialize FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbFinalize(SaImmCcbHandleT immCcbHandle)
{
        SaAisErrorT rc = saImmOmCcbFinalize(immCcbHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbFinalize(immCcbHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbFinalize FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbApply(SaImmCcbHandleT immCcbHandle)
{
        SaAisErrorT rc = saImmOmCcbApply(immCcbHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbApply(immCcbHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbApply FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbAbort(SaImmCcbHandleT immCcbHandle)
{
        SaAisErrorT rc = saImmOmCcbAbort(immCcbHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbAbort(immCcbHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbAbort FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbValidate(SaImmCcbHandleT immCcbHandle)
{
        SaAisErrorT rc = saImmOmCcbValidate(immCcbHandle);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbValidate(immCcbHandle);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbValidate FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerSet(SaImmAdminOwnerHandleT ownerHandle,
                                         const SaNameT** name,
                                         SaImmScopeT scope)
{
        SaAisErrorT rc = saImmOmAdminOwnerSet(ownerHandle, name, scope);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOwnerSet(ownerHandle, name, scope);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOwnerSet FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerSet_o2(SaImmAdminOwnerHandleT ownerHandle,
                                         const char** name,
                                         SaImmScopeT scope)
{
	int i=0;

        while (name[i]){
                 i++;
        }
        SaNameT ** obj_names = (SaNameT **) malloc((i+1)* sizeof(SaNameT *));

        i=0;
        while (name[i]){
                obj_names[i]=(SaNameT *) malloc(sizeof(SaNameT));
                osaf_extended_name_lend(name[i],obj_names[i]);
                i++;
        }
        obj_names[i]=NULL;
	SaAisErrorT rc = immutil_saImmOmAdminOwnerSet(ownerHandle, (const SaNameT**)obj_names, scope);
	
	i=0;
	while(obj_names[i]){
		free(obj_names[i]);
		i++;
	}
	free(obj_names);

	return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerRelease(SaImmAdminOwnerHandleT ownerHandle,
                                             const SaNameT** name,
                                             SaImmScopeT scope)
{
        SaAisErrorT rc = saImmOmAdminOwnerRelease(ownerHandle, name, scope);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOwnerRelease(ownerHandle, name, scope);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOwnerRelease FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmAdminOwnerRelease_o2(SaImmAdminOwnerHandleT ownerHandle,
                                             const char** name,
                                             SaImmScopeT scope)
{
	int i=0;

	while (name[i]){
		i++;
	}
	SaNameT ** obj_names = (SaNameT **) malloc((i+1)* sizeof(SaNameT *));

	i=0;
	while (name[i]){
		obj_names[i]=(SaNameT *) malloc(sizeof(SaNameT));
		osaf_extended_name_lend(name[i],obj_names[i]);
		i++;
	}
	obj_names[i]=NULL;

	SaAisErrorT rc = immutil_saImmOmAdminOwnerRelease(ownerHandle, (const SaNameT**) obj_names, scope);
	i=0;
	while(obj_names[i]){
		free(obj_names[i]);
		i++;
	}
	free(obj_names);

	return rc;
}

SaAisErrorT immutil_saImmOmAdminOperationInvoke_o214(SaImmAdminOwnerHandleT ownerHandle,
                                                   const char *objectName,
                                                   SaImmContinuationIdT continuationId,
                                                   SaImmAdminOperationIdT operationId,
                                                   const SaImmAdminOperationParamsT_2 **params,
                                                   SaAisErrorT *operationReturnValue,
                                                   SaTimeT timeout,
                                                   SaImmAdminOperationParamsT_2 ***returnParams)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name);
	else
		osaf_extended_name_clear(&obj_name);
		
	SaAisErrorT rc = immutil_saImmOmAdminOperationInvoke_o2(ownerHandle, &obj_name, continuationId,
	operationId, params, operationReturnValue, timeout, returnParams);
	return rc;
}

SaAisErrorT immutil_saImmOmAdminOperationInvoke_o2(SaImmAdminOwnerHandleT ownerHandle,
                                                   const SaNameT *objectName,
                                                   SaImmContinuationIdT continuationId,
                                                   SaImmAdminOperationIdT operationId,
                                                   const SaImmAdminOperationParamsT_2 **params,
                                                   SaAisErrorT *operationReturnValue,
                                                   SaTimeT timeout,
                                                   SaImmAdminOperationParamsT_2 ***returnParams)
{
        SaAisErrorT rc = saImmOmAdminOperationInvoke_o2(ownerHandle, objectName, continuationId,
		operationId, params, operationReturnValue, timeout, returnParams);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOperationInvoke_o2(ownerHandle, objectName, continuationId,
			operationId, params, operationReturnValue, timeout, returnParams);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOperationInvoke_o2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmAdminOperationInvoke_2(SaImmAdminOwnerHandleT ownerHandle,
                                                  const SaNameT *objectName,
                                                  SaImmContinuationIdT continuationId,
                                                  SaImmAdminOperationIdT operationId,
                                                  const SaImmAdminOperationParamsT_2 **params,
                                                  SaAisErrorT *operationReturnValue,
                                                  SaTimeT timeout)

{
        SaAisErrorT rc = saImmOmAdminOperationInvoke_2(ownerHandle, objectName, continuationId, 
                                                       operationId, params, operationReturnValue, timeout);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmAdminOperationInvoke_2(ownerHandle, objectName, continuationId, 
                                                   operationId, params, operationReturnValue, timeout);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmAdminOperationInvoke_2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectCreate_2(SaImmCcbHandleT immCcbHandle,
                                             const SaImmClassNameT className,
                                             const SaNameT *parent,
                                             const SaImmAttrValuesT_2** attrValues)
{
        SaAisErrorT rc = saImmOmCcbObjectCreate_2(immCcbHandle, className, parent, attrValues);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbObjectCreate_2(immCcbHandle, className, parent, attrValues);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbObjectCreate_2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectCreate_o2(SaImmCcbHandleT immCcbHandle,
                                             const SaImmClassNameT className,
                                             const char *parent,
                                             const SaImmAttrValuesT_2** attrValues)
{
	SaNameT parent_name;
	if(parent)
		osaf_extended_name_lend(parent,&parent_name);
	else
		osaf_extended_name_clear(&parent_name);

	SaAisErrorT rc = immutil_saImmOmCcbObjectCreate_2(immCcbHandle, className, &parent_name, attrValues);
	return rc;
} 

SaAisErrorT immutil_saImmOmCcbObjectModify_2(SaImmCcbHandleT immCcbHandle,
                                             const SaNameT *objectName,
                                             const SaImmAttrModificationT_2** attrMods)
{
        SaAisErrorT rc = saImmOmCcbObjectModify_2(immCcbHandle, objectName, attrMods);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbObjectModify_2(immCcbHandle, objectName, attrMods);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbObjectModify_2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectModify_o2(SaImmCcbHandleT immCcbHandle,
                                             const char *objectName,
                                             const SaImmAttrModificationT_2** attrMods)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name); 
	else 
		osaf_extended_name_clear(&obj_name);

	SaAisErrorT rc = immutil_saImmOmCcbObjectModify_2(immCcbHandle, &obj_name, attrMods);
	return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectDelete(SaImmCcbHandleT immCcbHandle,
                                           const SaNameT *objectName)
{
        SaAisErrorT rc = saImmOmCcbObjectDelete(immCcbHandle, objectName);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmCcbObjectDelete(immCcbHandle, objectName);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmCcbObjectDelete FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectDelete_o2(SaImmCcbHandleT immCcbHandle,
                                           const char *objectName)
{
	SaNameT obj_name;
	if(objectName)
		osaf_extended_name_lend(objectName,&obj_name);
	else
		osaf_extended_name_clear(&obj_name);

	SaAisErrorT rc = immutil_saImmOmCcbObjectDelete(immCcbHandle, &obj_name);
	return rc;
}

SaAisErrorT immutil_saImmOmCcbObjectRead(SaImmCcbHandleT ccbHandle, SaConstStringT objectName,
										 const SaImmAttrNameT *attributeNames, SaImmAttrValuesT_2 ***attributes) {
	SaAisErrorT rc = saImmOmCcbObjectRead(ccbHandle, objectName, attributeNames, attributes);
	unsigned int nTries = 1;
	while (rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries) {
		usleep(immutilWrapperProfile.retryInterval * 1000);
		rc = saImmOmCcbObjectRead(ccbHandle, objectName, attributeNames, attributes);
		nTries++;
	}
	if (rc != SA_AIS_OK && rc != SA_AIS_ERR_NOT_EXIST
			&& immutilWrapperProfile.errorsAreFatal)
		immutilError("saImmOmCcbObjectRead FAILED, rc = %d", (int) rc);
	return rc;
}

SaAisErrorT immutil_saImmOmClassDescriptionGet_2(SaImmHandleT immHandle,
                                                 const SaImmClassNameT className,
                                                 SaImmClassCategoryT * classCategory,
                                                 SaImmAttrDefinitionT_2 ***attrDefinitions)
{
   SaAisErrorT rc = saImmOmClassDescriptionGet_2(immHandle, className, classCategory, attrDefinitions);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmClassDescriptionGet_2(immHandle, className, classCategory, attrDefinitions);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmClassDescriptionGet_2 FAILED, rc = %d", (int)rc);
        return rc;
}

SaAisErrorT immutil_saImmOmClassDescriptionMemoryFree_2(SaImmHandleT immHandle,
                                                        SaImmAttrDefinitionT_2 **attrDef)
{
        SaAisErrorT rc = saImmOmClassDescriptionMemoryFree_2(immHandle, attrDef);
        unsigned int nTries = 1;
        while(rc == SA_AIS_ERR_TRY_AGAIN && nTries < immutilWrapperProfile.nTries){
                usleep(immutilWrapperProfile.retryInterval * 1000);
                rc = saImmOmClassDescriptionMemoryFree_2(immHandle, attrDef);
                nTries++;
        }
        if (rc != SA_AIS_OK && immutilWrapperProfile.errorsAreFatal)
                immutilError("saImmOmClassDescriptionMemoryFree_2 FAILED, rc = %d", (int)rc);
        return rc;
}
