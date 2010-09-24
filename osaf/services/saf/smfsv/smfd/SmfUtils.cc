/*
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <unistd.h>

#include <saAmf.h>
#include <saAis.h>

#include <immutil.h>
#include <string.h>
#include <algorithm>

#include "logtrace.h"
#include "SmfUtils.hh"
#include "smfd_smfnd.h"
#include "SmfImmOperation.hh"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

extern struct ImmutilWrapperProfile immutilWrapperProfile;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

SaVersionT SmfImmUtils::s_immVersion = { 'A', 2, 1 };

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

int 
executeRemoteCmd(const std::string & i_command, const std::string & i_node, SaTimeT i_timeout)
{
	/* TODO implement */
	TRACE("execute command '%s' on node '%s', timeout %llu", i_command.c_str(), i_node.c_str(), i_timeout);

	MDS_DEST nodeDest = getNodeDestination(i_node);
	if (nodeDest == 0) {
		LOG_ER("no node destination found for node %s", i_node.c_str());
		return -1;
	}

	/* Execute the command remote on node */
	return smfnd_remote_cmd(i_command.c_str(), nodeDest, i_timeout / 10000000);	/* convert ns to 10 ms timeout */
}

MDS_DEST 
getNodeDestination(const std::string & i_node)
{
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	TRACE("Find destination for node '%s'", i_node.c_str());

	/* It seems SaAmfNode objects can be stored, but the code
	 * indicates that SaClmNode's are expected. Anyway an attempt
	 * to go for it is probably faster that examining IMM classes
	 * in most cases. /uablrek */
	MDS_DEST dest = smfnd_dest_for_name(i_node.c_str());
	if (dest != 0) {
		TRACE("Found dest for [%s]", i_node.c_str());
		return smfnd_dest_for_name(i_node.c_str());
	}

	if (immUtil.getObject(i_node, &attributes) == false) {
		LOG_ER("Failed to get IMM node object %s", i_node.c_str());
		return 0;
	}

	const char *className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
						      SA_IMM_ATTR_CLASS_NAME, 0);

	if (className == NULL) {
		LOG_ER("Failed to get class name for node object %s", i_node.c_str());
		return 0;
	}

	if (strcmp(className, "SaClmNode") == 0) {
		return smfnd_dest_for_name(i_node.c_str());
	} else if (strcmp(className, "SaAmfNode") == 0) {
		const SaNameT *clmNode;
		clmNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfNodeClmNode", 0);

		if (clmNode == NULL) {
			LOG_ER("Failed to get clm node for amf node object %s", i_node.c_str());
			return 0;
		}

		char *nodeName = strndup((const char *)clmNode->value, clmNode->length);
		dest = smfnd_dest_for_name(nodeName);
		if (dest == 0) {
			LOG_ER("Failed to get node dest for clm node %s", nodeName);
		}
		free(nodeName);
		return dest;
	}

	LOG_ER("Failed to get destination for node object %s, class %s", i_node.c_str(), className);

	return 0;
}

//-----------------------------------------------------------------------------------------------
// This function replaces all copies in the string of a char.sequence with an other char. sequence
// Example: cout << replace_all_copy("zipmeowbam", "meow", "zam") << "\n";
//-----------------------------------------------------------------------------------------------
std::string 
replaceAllCopy(const std::string& i_haystack, const std::string& i_needle, const std::string& i_replacement) {
        if (i_needle.empty()) {
                return i_haystack;
        }
        std::string result;
        for (std::string::const_iterator substart = i_haystack.begin(), subend; ; ) {
                subend = search(substart, i_haystack.end(), i_needle.begin(), i_needle.end());
                copy(substart, subend, back_inserter(result));
                if (subend == i_haystack.end()) {
                        break;
                }
                copy(i_replacement.begin(), i_replacement.end(), back_inserter(result));
                substart = subend + i_needle.size();
        }
        return result;
}

//================================================================================
// Class SmfImmUtils
// Purpose:
// Comments:
//================================================================================

SmfImmUtils::SmfImmUtils():
   m_omHandle(0), 
   m_ownerHandle(0), 
   m_accessorHandle(0)
{
	initialize();
}

// ------------------------------------------------------------------------------
// ~SmfImmUtils()
// ------------------------------------------------------------------------------
SmfImmUtils::~SmfImmUtils()
{
	finalize();
}

// ------------------------------------------------------------------------------
// initialize()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::initialize(void)
{
	if (m_omHandle == 0) {
		(void)immutil_saImmOmInitialize(&m_omHandle, NULL, &s_immVersion);
	}

	if (m_ownerHandle == 0) {
		(void)immutil_saImmOmAdminOwnerInitialize(m_omHandle, (char*)"SMFSERVICE", SA_TRUE, &m_ownerHandle);
	}

	if (m_accessorHandle == 0) {
		(void)immutil_saImmOmAccessorInitialize(m_omHandle, &m_accessorHandle);
	}

	return true;
}

// ------------------------------------------------------------------------------
// finalize()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::finalize(void)
{

	if (m_ownerHandle != 0) {
		(void)immutil_saImmOmAdminOwnerFinalize(m_ownerHandle);
	}

	if (m_accessorHandle != 0) {
		(void)immutil_saImmOmAccessorFinalize(m_accessorHandle);
	}

	if (m_omHandle != 0) {
		(void)immutil_saImmOmFinalize(m_omHandle);
	}

	return true;
}
// ------------------------------------------------------------------------------
// getClassDescription()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::getClassDescription(const std::string & i_className, SaImmAttrDefinitionT_2 *** o_attributeDefs)
{
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;
	SaImmClassCategoryT classCategory;
        SaAisErrorT rc = immutil_saImmOmClassDescriptionGet_2(
		m_omHandle, (SaImmClassNameT)i_className.c_str(), &classCategory, o_attributeDefs);

	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	if (rc != SA_AIS_OK) {
		LOG_ER("saImmOmClassDescriptionGet_2 for [%s], rc = %d", i_className.c_str(), rc);
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// classDescriptionMemoryFree()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::classDescriptionMemoryFree(SaImmAttrDefinitionT_2 ** i_attributeDefs)
{
	SaAisErrorT rc = SA_AIS_OK;

//	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
//	immutilWrapperProfile.errorsAreFatal = 0;

        rc = immutil_saImmOmClassDescriptionMemoryFree_2(m_omHandle, i_attributeDefs);

//	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	if (rc != SA_AIS_OK) {
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// getObject()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::getObject(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT objectName;
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;

	objectName.length = i_dn.length();
	strncpy((char *)objectName.value, i_dn.c_str(), objectName.length);
	objectName.value[objectName.length] = 0;

	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOmAccessorGet_2(m_accessorHandle, &objectName, NULL, o_attributes);
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	if (rc != SA_AIS_OK) {
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// getObjectAisRC()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfImmUtils::getObjectAisRC(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes)
{
	SaAisErrorT rc = SA_AIS_OK;
	SaNameT objectName;
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;

	objectName.length = i_dn.length();
	strncpy((char *)objectName.value, i_dn.c_str(), objectName.length);
	objectName.value[objectName.length] = 0;

	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOmAccessorGet_2(m_accessorHandle, &objectName, NULL, o_attributes);
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;

	return rc;
}

// ------------------------------------------------------------------------------
// getParentObject()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::getParentObject(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes)
{
	std::string parentDn = i_dn.substr(i_dn.find(',') + 1, std::string::npos);
	return getObject(parentDn, o_attributes);
}

// ------------------------------------------------------------------------------
// getChildren()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::getChildren(const std::string & i_dn, std::list < std::string > &o_childList, SaImmScopeT i_scope,
			      const char *i_className)
{
	SaImmSearchHandleT immSearchHandle;
	SaImmSearchParametersT_2 objectSearch;
	SaAisErrorT result;
	bool rc = true;
	SaNameT objectName;
	SaNameT *objectNamePtr = NULL;
	const SaStringT className = (const SaStringT)i_className;
	SaImmAttrValuesT_2 **attributes;

	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;

	TRACE_ENTER();

	if (i_dn.size() > 0) {
		objectName.length = i_dn.length();
		strncpy((char *)objectName.value, i_dn.c_str(), objectName.length);
		objectName.value[objectName.length] = 0;
		objectNamePtr = &objectName;
	}

	immutilWrapperProfile.errorsAreFatal = 0;

	if (i_className != NULL) {
		/* Search for all objects of class i_className */
		objectSearch.searchOneAttr.attrName = (char*)SA_IMM_ATTR_CLASS_NAME;
		objectSearch.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
		objectSearch.searchOneAttr.attrValue = (void *)&className;

		result = immutil_saImmOmSearchInitialize_2(m_omHandle, objectNamePtr,	/* Search below i_dn object */
							i_scope, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR, &objectSearch, NULL,	/* Get no attributes */
							&immSearchHandle);
		if(result != SA_AIS_OK) {
			if (result == SA_AIS_ERR_NOT_EXIST) {
				TRACE("immutil_saImmOmSearchInitialize_2 return rc=%d (class name = %s, parent = %s", (int)result, i_className, i_dn.c_str());
				goto done;
			} else {
				LOG_ER("immutil_saImmOmSearchInitialize_2 return rc=%d (class name = %s, parent = %s", (int)result, i_className, i_dn.c_str());
				rc = false;
				goto done;
			}
		}
	} else {
		/* Search for all objects */
		result = immutil_saImmOmSearchInitialize_2(m_omHandle, objectNamePtr,	/* Search below i_dn object */
							i_scope, SA_IMM_SEARCH_GET_NO_ATTR, NULL,	/* No search criteria */
							NULL,	/* Get no attributes */
							&immSearchHandle);
		if(result != SA_AIS_OK) {
			if (result == SA_AIS_ERR_NOT_EXIST) {
				TRACE("immutil_saImmOmSearchInitialize_2 return rc=%d (class name = %s", (int)result, i_className);
				goto done;
			} else {
				LOG_ER("immutil_saImmOmSearchInitialize_2 return rc=%d (class name = %s", (int)result, i_className);
				rc = false;
				goto done;
			}
		}
	}

	while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
		/* Stor found child in child list */
		std::string childDn;
		childDn.append((char *)objectName.value, objectName.length);

		o_childList.push_back(childDn);
	}

done:
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
	TRACE_LEAVE();
	return rc;
}

// ------------------------------------------------------------------------------
// callAdminOperation()
// ------------------------------------------------------------------------------
SaAisErrorT
SmfImmUtils::callAdminOperation(const std::string & i_dn, unsigned int i_operationId,
				     const SaImmAdminOperationParamsT_2 ** i_params, SaTimeT i_timeout)
{
	SaAisErrorT rc;
	SaAisErrorT returnValue;
	SaNameT objectName;
	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	int retry          = 100;

	/* First set admin owner on the object */
	objectName.length = i_dn.length();
	memcpy(objectName.value, i_dn.c_str(), objectName.length);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	TRACE("calling admin operation %u on object %s", i_operationId, i_dn.c_str());
        if (i_params != 0) {
                TRACE("contains parameters");
        } else {
                TRACE("contains NO parameters");
        }

	immutilWrapperProfile.errorsAreFatal = 0;

	rc = immutil_saImmOmAdminOwnerSet(m_ownerHandle, objectNames, SA_IMM_ONE);
	if ( rc != SA_AIS_OK) {
		LOG_ER("SmfImmUtils::callAdminOperation: immutil_saImmOmAdminOwnerSet rc = %u", rc);
		goto done;
	}

	/* Call the admin operation */
	do {
		TRACE("call immutil_saImmOmAdminOperationInvoke_2");
		rc = immutil_saImmOmAdminOperationInvoke_2(m_ownerHandle, &objectName, 0, i_operationId, i_params,
							   &returnValue, i_timeout);
		if (retry <= 0) {
			LOG_ER("immutil_saImmOmAdminOperationInvoke_2 returnValue = SA_AIS_ERR_TRY_AGAIN, giving up");
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
		sleep(2);
		retry--;
	} while ((rc == SA_AIS_OK) && (returnValue == SA_AIS_ERR_TRY_AGAIN));

	if ( rc != SA_AIS_OK) {
		LOG_ER("SmfImmUtils::callAdminOperation: immutil_saImmOmAdminOperationInvoke_2 rc = %u", rc);
		goto done;
	}
       
	if ((returnValue != SA_AIS_OK ) && (returnValue != SA_AIS_ERR_REPAIR_PENDING)) {
		TRACE("SmfImmUtils::callAdminOperation: admin operation %u on %s returned %u", i_operationId, i_dn.c_str(),
		      returnValue);
	}
	
	rc = returnValue;

done:
	immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
	return rc;
}	


// ------------------------------------------------------------------------------
// doImmOperations()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfImmUtils::doImmOperations(std::list < SmfImmOperation * >&i_immOperationList)
{
	/* Initialize CCB and get the IMM CCB handle */
	SaAisErrorT result;
	SaImmCcbHandleT immCcbHandle;

	int errorsAreFatal = immutilWrapperProfile.errorsAreFatal;
	immutilWrapperProfile.errorsAreFatal = 0;

	SaImmCcbFlagsT ccbFlags = 0;

	result = immutil_saImmOmCcbInitialize(m_ownerHandle, ccbFlags, &immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_ER("SmfImmUtils::doImmOperations:saImmOmCcbInitialize failed  SaAisErrorT=%u", result);
                immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
		return result;
	}

	std::list < SmfImmOperation * >::iterator iter;

	/* Do all operations within the CCB  */
	for (iter = i_immOperationList.begin(); iter != i_immOperationList.end(); ++iter) {
		(*iter)->setImmOwnerHandle(m_ownerHandle);
		(*iter)->setCcbHandle(immCcbHandle);
		result = (*iter)->execute();
		if (result != SA_AIS_OK){
			LOG_ER("SmfImmUtils::doImmOperations:execute failed SaAisErrorT=%u", result);
			return result;
		}
	}

	/* Apply the CCB */
	result = immutil_saImmOmCcbApply(immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_ER("SmfImmUtils::doImmOperations:saImmOmCcbApply failed SaAisErrorT=%u", result);
                immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
		return result;
	}

	/* Finalize CCB and release the IMM CCB handle */
	result = immutil_saImmOmCcbFinalize(immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_ER("SmfImmUtils::doImmOperations:saImmOmCcbFinalize failed SaAisErrorT=%u", result);
                immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
		return result;
	}

        immutilWrapperProfile.errorsAreFatal = errorsAreFatal;
	return result;
}

// ------------------------------------------------------------------------------
// smf_stringToImmType()
// ------------------------------------------------------------------------------
SaImmValueTypeT 
smf_stringToImmType(char *i_type)
{
	if (!strcmp("SA_IMM_ATTR_SAINT32T", i_type))
		return SA_IMM_ATTR_SAINT32T;
	else if (!strcmp("SA_IMM_ATTR_SAUINT32T", i_type))
		return SA_IMM_ATTR_SAUINT32T;
	else if (!strcmp("SA_IMM_ATTR_SAINT64T", i_type))
		return SA_IMM_ATTR_SAINT64T;
	else if (!strcmp("SA_IMM_ATTR_SAUINT64T", i_type))
		return SA_IMM_ATTR_SAUINT64T;
	else if (!strcmp("SA_IMM_ATTR_SATIMET", i_type))
		return SA_IMM_ATTR_SATIMET;
	else if (!strcmp("SA_IMM_ATTR_SANAMET", i_type))
		return SA_IMM_ATTR_SANAMET;
	else if (!strcmp("SA_IMM_ATTR_SAFLOATT", i_type))
		return SA_IMM_ATTR_SAFLOATT;
	else if (!strcmp("SA_IMM_ATTR_SADOUBLET", i_type))
		return SA_IMM_ATTR_SADOUBLET;
	else if (!strcmp("SA_IMM_ATTR_SASTRINGT", i_type))
		return SA_IMM_ATTR_SASTRINGT;
	else if (!strcmp("SA_IMM_ATTR_SAANYT", i_type))
		return SA_IMM_ATTR_SAANYT;
	else {
		TRACE("SmfUtils::smf_stringToImmType type %s not found", i_type);
		assert(0);
	}
	return (SaImmValueTypeT)0;
}

// ------------------------------------------------------------------------------
// smf_stringToImmAttrModType()
// ------------------------------------------------------------------------------
SaImmAttrModificationTypeT 
smf_stringToImmAttrModType(char *i_type)
{
	if (!strcmp("SA_IMM_ATTR_VALUES_ADD", i_type))
		return SA_IMM_ATTR_VALUES_ADD;
	else if (!strcmp("SA_IMM_ATTR_VALUES_DELETE", i_type))
		return SA_IMM_ATTR_VALUES_DELETE;
	else if (!strcmp("SA_IMM_ATTR_VALUES_REPLACE", i_type))
		return SA_IMM_ATTR_VALUES_REPLACE;
	else {
		TRACE("SmfUtils::smf_stringToImmAttrModType type %s not found", i_type);
		assert(0);
	}
	return (SaImmAttrModificationTypeT)0;
}

//------------------------------------------------------------------------------
// smf_stringsToValues()
//------------------------------------------------------------------------------
bool 
smf_stringsToValues(SaImmAttrValuesT_2 * i_attribute, std::list < std::string > i_values)
{
//	assert(i_attribute->attrValuesNumber > 0);
//	size_t len;

	//Allocate space for pointers to all values of the attributes. attrValues points to the first element
	i_attribute->attrValues = (SaImmAttrValueT *) malloc(sizeof(SaImmAttrValueT) * i_attribute->attrValuesNumber);

	//The value variable is increased at the end of the loop for each new value in a multivalue attribute
	SaImmAttrValueT *value = i_attribute->attrValues;

	std::list < std::string >::iterator iter;
	std::list < std::string >::iterator iterE;
	iter = i_values.begin();
	iterE = i_values.end();

	while (iter != iterE) {
                if (!smf_stringToValue(i_attribute->attrValueType, value,  (*iter).c_str())) {
                        LOG_ER("SmfUtils:smf_stringsToValues: Fails to convert a string to value");
                        assert(0);
                        return false;
                }

		iter++;
		value++;	//Next value in (case of multivalue attribute)
	}			//End whle

	return true;
}

// ------------------------------------------------------------------------------
// smf_stringToValue()
// ------------------------------------------------------------------------------
bool 
smf_stringToValue(SaImmValueTypeT i_type, SaImmAttrValueT *i_value, const char* i_str)
{
	size_t len;

        switch (i_type) {
        case SA_IMM_ATTR_SAINT32T:
                *i_value = malloc(sizeof(SaInt32T));
                *((SaInt32T *) * i_value) = strtol(i_str, NULL, 10);
                break;
        case SA_IMM_ATTR_SAUINT32T:
                *i_value = malloc(sizeof(SaUint32T));
                *((SaUint32T *) * i_value) = (SaUint32T) strtol(i_str, NULL, 10);
                break;
        case SA_IMM_ATTR_SAINT64T:
                *i_value = malloc(sizeof(SaInt64T));
                *((SaInt64T *) * i_value) = (SaInt64T) strtoll(i_str, NULL, 10);
                break;
        case SA_IMM_ATTR_SAUINT64T:
                *i_value = malloc(sizeof(SaUint64T));
                *((SaUint64T *) * i_value) = (SaUint64T) strtoull(i_str, NULL, 10);
                break;
        case SA_IMM_ATTR_SATIMET:	// Int64T
                *i_value = malloc(sizeof(SaInt64T));
                *((SaTimeT *) * i_value) = (SaTimeT) strtoll(i_str, NULL, 10);
                break;
        case SA_IMM_ATTR_SANAMET:
                len = strlen(i_str);
                *i_value = malloc(sizeof(SaNameT));
                ((SaNameT *) * i_value)->length = (SaUint16T) len;
                strncpy((char *)((SaNameT *) * i_value)->value, i_str, len);
                ((SaNameT *) * i_value)->value[len] = '\0';
                break;
        case SA_IMM_ATTR_SAFLOATT:
                *i_value = malloc(sizeof(SaFloatT));
                *((SaFloatT *) * i_value) = (SaFloatT) atof(i_str);
                break;
        case SA_IMM_ATTR_SADOUBLET:
                *i_value = malloc(sizeof(SaDoubleT));
                *((SaDoubleT *) * i_value) = (SaDoubleT) atof(i_str);
                break;
        case SA_IMM_ATTR_SASTRINGT:
                len = strlen(i_str);
                *i_value = malloc(sizeof(SaStringT));
                *((SaStringT *) * i_value) = (SaStringT) malloc(len + 1);
                strncpy(*((SaStringT *) * i_value), i_str, len);
                (*((SaStringT *) * i_value))[len] = '\0';
                break;
        case SA_IMM_ATTR_SAANYT:
                unsigned int i;
                char byte[5];
                char *endMark;

                len = strlen(i_str) / 2;
                *i_value = malloc(sizeof(SaAnyT));
                ((SaAnyT *) * i_value)->bufferAddr = (SaUint8T *) malloc(sizeof(SaUint8T) * len);
                ((SaAnyT *) * i_value)->bufferSize = len;

                byte[0] = '0';
                byte[1] = 'x';
                byte[4] = '\0';

                endMark = byte + 4;

                for (i = 0; i < len; i++) {
                        byte[2] = i_str[2 * i];
                        byte[3] = i_str[2 * i + 1];

                        ((SaAnyT *) * i_value)->bufferAddr[i] = (SaUint8T) strtod(byte, &endMark);
                }

                TRACE("SIZE: %d", (int)((SaAnyT *) * i_value)->bufferSize);
                break;
        default:
                LOG_ER("BAD VALUE TYPE");
                assert(0);
                return false;
        }		//End switch

        return true;
}

// ------------------------------------------------------------------------------
// smf_opStringToInt()
// ------------------------------------------------------------------------------
int 
smf_opStringToInt(const char *i_str)
{
	if (!strcmp("SA_AMF_ADMIN_UNLOCK", i_str))
		return SA_AMF_ADMIN_UNLOCK;
	else if (!strcmp("SA_AMF_ADMIN_LOCK", i_str))
		return SA_AMF_ADMIN_LOCK;
	else if (!strcmp("SA_AMF_ADMIN_LOCK_INSTANTIATION", i_str))
		return SA_AMF_ADMIN_LOCK_INSTANTIATION;
	else if (!strcmp("SA_AMF_ADMIN_UNLOCK_INSTANTIATION", i_str))
		return SA_AMF_ADMIN_UNLOCK_INSTANTIATION;
	else if (!strcmp("SA_AMF_ADMIN_SHUTDOWN", i_str))
		return SA_AMF_ADMIN_SHUTDOWN;
	else if (!strcmp("SA_AMF_ADMIN_RESTART", i_str))
		return SA_AMF_ADMIN_RESTART;
	else if (!strcmp("SA_AMF_ADMIN_SI_SWAP", i_str))
		return SA_AMF_ADMIN_SI_SWAP;
	else if (!strcmp("SA_AMF_ADMIN_SG_ADJUST", i_str))
		return SA_AMF_ADMIN_SG_ADJUST;
	else if (!strcmp("SA_AMF_ADMIN_REPAIRED", i_str))
		return SA_AMF_ADMIN_REPAIRED;
	else if (!strcmp("SA_AMF_ADMIN_EAM_START", i_str))
		return SA_AMF_ADMIN_EAM_START;
	else if (!strcmp("SA_AMF_ADMIN_EAM_STOP", i_str))
		return SA_AMF_ADMIN_EAM_STOP;
	else {
		TRACE("SmfUtils::smf_opStringToInt type %s not found", i_str);
		assert(0);
	}
	return (SaAmfAdminOperationIdT)0;
}

//------------------------------------------------------------------------------
// smfSystem()
//------------------------------------------------------------------------------
int
smf_system(std::string i_cmd)
{
	int rc = 0;

	TRACE("smf_system: trying command ""%s""",i_cmd.c_str());
	int status = system(i_cmd.c_str());

	if(!WIFEXITED(status)) //returns  true  if  the  child  terminated  normally,  that is, by calling exit(3) or _exit(2), or by returning from  main().
	{
		//Something went wrong
		TRACE("smf_system:child  was not terminated  normally, status = %d, command string = %s", status, i_cmd.c_str());
		if(WIFSIGNALED(status)) //returns true if the child process was terminated by a signal.
		{
			int signal = WTERMSIG(status); // returns the number of the signal that caused the child process to terminate.
			TRACE("smf_system: The child process was terminated by signal = %d", signal);
		}

		if(WIFSTOPPED(status)) //returns  true  if the child process was stopped by delivery of a signal; this is only possible if the call was done
			//using WUNTRACED or when the child is being traced (see ptrace(2))
		{
			int signal = WSTOPSIG(status); //returns the number of the signal which caused the child to stop. 
			// This macro should only be employed if  WIFSTOPPED returned true.
			TRACE("smf_system: Process was stopped by delivery of a signal = %d", signal);
		}
	}
	else //WIFEXITED returned true
	{
		rc = WEXITSTATUS(status); //returns the exit status of the child.  This consists of the least significant 8 bits of the  status  argument  that
		//the child specified in a call to exit() or _exit() or as the argument for a return statement in main().  This macro
		//should only be employed if WIFEXITED returned true.
		if(rc == 0)
		{
			TRACE("smf_system: command ""%s"", sucessfully executed", i_cmd.c_str());
		}
		else 
		{
			TRACE("smf_system: command ""%s"", fails rc=%d", i_cmd.c_str(), rc);
		}
	}

	return rc;
}
