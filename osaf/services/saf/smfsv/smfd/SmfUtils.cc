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

#include "saAis.h"
#include <saAmf.h>
#include <saSmf.h>
#include <saAis.h>
#include <saf_error.h>
#include "osaf_time.h"

#include <immutil.h>
#include <iostream>
#include <sstream>
#include <string.h>
#include <algorithm>

#include "osaf_extended_name.h"
#include "osaf_time.h"
#include "logtrace.h"
#include "SmfUtils.hh"
#include "smfd_smfnd.h"
#include "SmfImmOperation.hh"
#include "SmfRollback.hh"


/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

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
bool 
waitForNodeDestination(const std::string & i_node, SmfndNodeDest* o_nodeDest)
{
        int interval = 2;  //seconds
        int nodetimeout = smfd_cb->rebootTimeout/1000000000; //seconds
        while (!getNodeDestination(i_node, o_nodeDest)) {
                if (nodetimeout > 0) {
                        TRACE("No destination found, try again wait %d seconds", interval);
                        struct timespec sleepTime = { interval, 0 };
                        osaf_nanosleep(&sleepTime);
                        nodetimeout -= interval;
                } else {
                        LOG_NO("no node destination found whitin time limit for node %s", i_node.c_str());
                        return false;
               }
        }

        return true;
}

bool 
getNodeDestination(const std::string & i_node, SmfndNodeDest* o_nodeDest)
{
	SmfImmUtils immUtil;
	SaImmAttrValuesT_2 **attributes;

	TRACE("Find destination for node '%s'", i_node.c_str());

	/* It seems SaAmfNode objects can be stored, but the code
	 * indicates that SaClmNode's are expected. Anyway an attempt
	 * to go for it is probably faster that examining IMM classes
	 * in most cases. /uablrek */
	if (smfnd_for_name(i_node.c_str(), o_nodeDest)) {
		TRACE("Found dest for [%s]", i_node.c_str());
		return true;
	}

	if (immUtil.getObject(i_node, &attributes) == false) {
		LOG_NO("Failed to get IMM node object %s", i_node.c_str());
		return false;
	}

	const char *className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
						      SA_IMM_ATTR_CLASS_NAME, 0);

	if (className == NULL) {
		LOG_NO("Failed to get class name for node object %s", i_node.c_str());
		return false;
	}

        //Try to get the node director for a while. If the nodes reboot very fast
        //after a cluster reboot, it could happend the rebooted nodes comes up before
        //the last one is rebooted. This could make the campaign to fail when the campaign continue.
	if (strcmp(className, "SaClmNode") == 0) {
                int timeout = 10;
                while(smfnd_for_name(i_node.c_str(), o_nodeDest) == false) {
                        if (timeout <= 0) {
                                LOG_NO("Failed to get node dest for clm node %s", i_node.c_str());
                                return false;
                        }
                        struct timespec time = { 2, 0 };
                        osaf_nanosleep(&time);
                        timeout--;
                }
                return true;

	} else if (strcmp(className, "SaAmfNode") == 0) {
		const SaNameT *clmNode;
		clmNode = immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes, "saAmfNodeClmNode", 0);

		if (clmNode == NULL) {
			LOG_NO("Failed to get clm node for amf node object %s", i_node.c_str());
			return false;
		}

		char *nodeName = strdup(osaf_extended_name_borrow(clmNode));
                int timeout = 10;
                while(smfnd_for_name(nodeName, o_nodeDest) == false) {
                        if (timeout <= 0) {
                                LOG_NO("Failed to get node dest for clm node %s", nodeName);
                                free(nodeName);
                                return false;
                        }
                        struct timespec time = { 2, 0 };
                        osaf_nanosleep(&time);
                        timeout--;
                }
                free(nodeName);
        } else {
                LOG_NO("Unknown class name %s", className);
                return false;
        }

        return true;
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
	SaImmClassCategoryT classCategory;
        SaAisErrorT rc = immutil_saImmOmClassDescriptionGet_2(
		m_omHandle, (SaImmClassNameT)i_className.c_str(), &classCategory, o_attributeDefs);

	if (rc != SA_AIS_OK) {
		LOG_NO("saImmOmClassDescriptionGet_2 for [%s], rc=%s", i_className.c_str(), saf_error(rc));
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
        rc = immutil_saImmOmClassDescriptionMemoryFree_2(m_omHandle, i_attributeDefs);

	if (rc != SA_AIS_OK) {
		LOG_NO("saImmOmClassDescriptionMemoryFree_2 failed, rc=%s", saf_error(rc));
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// getClassNameForObject()
// ------------------------------------------------------------------------------
bool
SmfImmUtils::getClassNameForObject(const std::string& i_dn, std::string& o_className)
{
        SaImmAttrValuesT_2 **attributes;
        if (this->getObject(i_dn, &attributes) == false) {
                LOG_NO("Failed to get object %s", i_dn.c_str());
                return false;
       }

        o_className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
                                            SA_IMM_ATTR_CLASS_NAME, 0);
        if (o_className.empty()) {
                LOG_NO("Failed to get class name for object %s", i_dn.c_str());
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

        if (i_dn.length() > smfd_cb->maxDnLength) {
		LOG_NO("getObject error, dn too long (%zu), max %zu",
                        i_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength));
		return false;
        }

	osaf_extended_name_lend(i_dn.c_str(), &objectName);

	rc = immutil_saImmOmAccessorGet_2(m_accessorHandle, &objectName, NULL, o_attributes);

	if (rc != SA_AIS_OK) {
		TRACE("saImmOmAccessorGet_2 failed, rc=%s, dn=[%s]", saf_error(rc), i_dn.c_str());
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

        if (i_dn.length() > smfd_cb->maxDnLength) {
		LOG_NO("getObjectAisRC error, dn too long (%zu), max %zu",
                        i_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength));
		return SA_AIS_ERR_NAME_TOO_LONG;
        }

        osaf_extended_name_lend(i_dn.c_str(), &objectName);

	rc = immutil_saImmOmAccessorGet_2(m_accessorHandle, &objectName, NULL, o_attributes);

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
	SaImmSearchHandleT immSearchHandle = 0;
	SaImmSearchParametersT_2 objectSearch;
	SaAisErrorT result = SA_AIS_OK;
	bool rc = true;
	SaNameT objectName;
	SaNameT *objectNamePtr = NULL;
	const SaStringT className = (SaStringT)i_className;
	SaImmAttrValuesT_2 **attributes;

	TRACE_ENTER();

	if (i_dn.size() > 0) {
                if (i_dn.length() > smfd_cb->maxDnLength) {
                        LOG_NO("getChildren error, dn too long (%zu), max %zu",
                                i_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength));
                        return false;
                }

                osaf_extended_name_lend(i_dn.c_str(), &objectName);
		objectNamePtr = &objectName;
	}

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
				TRACE("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s], parent=[%s]", saf_error(result), i_className, i_dn.c_str());
				goto done;
			} else {
				LOG_NO("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s], parent=[%s]", saf_error(result), i_className, i_dn.c_str());
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
				TRACE("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s]", saf_error(result), i_className);
				goto done;
			} else {
				LOG_NO("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s]", saf_error(result), i_className);
				rc = false;
				goto done;
			}
		}
	}

	while (immutil_saImmOmSearchNext_2(immSearchHandle, &objectName, &attributes) == SA_AIS_OK) {
		/* Stor found child in child list */
		std::string childDn;
		childDn.append(osaf_extended_name_borrow(&objectName));

		o_childList.push_back(childDn);
	}

done:
        (void) immutil_saImmOmSearchFinalize(immSearchHandle);
	TRACE_LEAVE();
	return rc;
}

// ------------------------------------------------------------------------------
// getChildrenAndAttrBySearchHandle()
// ------------------------------------------------------------------------------
bool 
SmfImmUtils::getChildrenAndAttrBySearchHandle(const std::string& i_dn, 
					      SaImmSearchHandleT& io_immSearchHandle, 
					      SaImmScopeT i_scope,  
					      SaImmAttrNameT* i_attrNames, 
					      const char *i_className)
{
	//The caller of this method is resposible to finalize the search handle in case of success
	//To release the handler the call immutil_saImmOmSearchFinalize(io_immSearchHandle) shall be used.

	SaImmSearchParametersT_2 objectSearch;
	SaAisErrorT result;
	bool rc = true;
	SaNameT objectName;
	SaNameT *objectNamePtr = NULL;
	const SaStringT className = (SaStringT)i_className;

	TRACE_ENTER();

	if (i_dn.size() > 0) {
                if (i_dn.length() > smfd_cb->maxDnLength) {
                        LOG_NO("getChildren error, dn too long (%zu), max %zu",
                                i_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength));
			rc =  false;
			goto done;
                }

                osaf_extended_name_lend(i_dn.c_str(), &objectName);
		objectNamePtr = &objectName;
	}

	if (i_className != NULL) {
		/* Search for all objects of class i_className */
		objectSearch.searchOneAttr.attrName = (char*)SA_IMM_ATTR_CLASS_NAME;
		objectSearch.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
		objectSearch.searchOneAttr.attrValue = (void *)&className;

		result = immutil_saImmOmSearchInitialize_2(m_omHandle, objectNamePtr,	/* Search below i_dn object */
							   i_scope, SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_SOME_ATTR, 
							   &objectSearch, i_attrNames,	&io_immSearchHandle);
		if(result != SA_AIS_OK) {
			if (result == SA_AIS_ERR_NOT_EXIST) {
				TRACE("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s], parent=[%s]", saf_error(result), i_className, i_dn.c_str());
				goto done;
			} else {
				LOG_NO("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s], parent=[%s]", saf_error(result), i_className, i_dn.c_str());
				(void) immutil_saImmOmSearchFinalize(io_immSearchHandle);
				rc =  false;
				goto done;
			}
		}
	} else {
		/* Search for all objects */
		result = immutil_saImmOmSearchInitialize_2(m_omHandle, objectNamePtr,	/* Search below i_dn object */
							   i_scope, SA_IMM_SEARCH_GET_NO_ATTR,
							   NULL, i_attrNames, &io_immSearchHandle);
		if(result != SA_AIS_OK) {
			if (result == SA_AIS_ERR_NOT_EXIST) {
				TRACE("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s]", saf_error(result), i_className);
				goto done;
			} else {
				LOG_NO("immutil_saImmOmSearchInitialize_2, rc=%s, class name=[%s]", saf_error(result), i_className);
				(void) immutil_saImmOmSearchFinalize(io_immSearchHandle);
				rc = false;
				goto done;
			}
		}
	}

done:
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
	int retry          = 100;

        if (i_dn.length() > smfd_cb->maxDnLength) {
                LOG_NO("callAdminOperation error, dn too long (%zu), max %zu",
                        i_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength));
                return SA_AIS_ERR_NAME_TOO_LONG;
        }

	/* First set admin owner on the object */
        osaf_extended_name_lend(i_dn.c_str(), &objectName);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	TRACE("calling admin operation %u on object %s", i_operationId, i_dn.c_str());
        if (i_params != 0) {
                TRACE("contains parameters");
        } else {
                TRACE("contains NO parameters");
        }

	rc = immutil_saImmOmAdminOwnerSet(m_ownerHandle, objectNames, SA_IMM_ONE);
	if ( rc != SA_AIS_OK) {
		LOG_NO("Fail to set admin owner, rc=%s, dn=[%s]", saf_error(rc), i_dn.c_str());
		goto done;
	}

	/* Call the admin operation */
	do {
		TRACE("call immutil_saImmOmAdminOperationInvoke_2");
		rc = immutil_saImmOmAdminOperationInvoke_2(m_ownerHandle, &objectName, 0, i_operationId, i_params,
							   &returnValue, i_timeout);
		if (retry <= 0) {
			LOG_NO("Fail to invoke admin operation, too many SA_AIS_ERR_TRY_AGAIN, giving up. dn=[%s], opId=[%u]",
			       i_dn.c_str(), i_operationId);
			rc = SA_AIS_ERR_TRY_AGAIN;
			goto done;
		}
		sleep(2);
		retry--;
	} while ((rc == SA_AIS_OK) && (returnValue == SA_AIS_ERR_TRY_AGAIN));

	if ( rc != SA_AIS_OK) {
		LOG_NO("Fail to invoke admin operation, rc=%s. dn=[%s], opId=[%u]",saf_error(rc), i_dn.c_str(), i_operationId);
		goto done;
	}
       
	if ((returnValue != SA_AIS_OK ) && (returnValue != SA_AIS_ERR_REPAIR_PENDING)) {
		TRACE("Admin operation %u on %s, return value=%s", i_operationId, i_dn.c_str(), saf_error(returnValue));
	}
	
	rc = returnValue;

done:
	return rc;
}	


// ------------------------------------------------------------------------------
// doImmOperations()
// ------------------------------------------------------------------------------
SaAisErrorT 
SmfImmUtils::doImmOperations(std::list < SmfImmOperation * >&i_immOperationList, SmfRollbackCcb* io_rollbackCcb)
{
	/* Initialize CCB and get the IMM CCB handle */
	SaAisErrorT result;
	SaImmCcbHandleT immCcbHandle;

	SaImmCcbFlagsT ccbFlags = 0;

	result = immutil_saImmOmCcbInitialize(m_ownerHandle, ccbFlags, &immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_NO("Fail to initialize OM CCB, rc=%s", saf_error(result));
		return result;
	}

	std::list < SmfImmOperation * >::iterator iter;

	/* Do all operations within the CCB  */
	for (iter = i_immOperationList.begin(); iter != i_immOperationList.end(); ++iter) {
                SmfRollbackData* rollbackData = NULL;

		(*iter)->setImmOwnerHandle(m_ownerHandle);
		(*iter)->setCcbHandle(immCcbHandle);

                if (io_rollbackCcb != NULL) {
                        rollbackData = new (std::nothrow) SmfRollbackData(io_rollbackCcb);
                        if (rollbackData == NULL) {
                                LOG_NO("Failed to create SmfRollbackData C++ object, no memory");
                                return SA_AIS_ERR_NO_MEMORY;
                        }
                }

		if ((result = (*iter)->execute(rollbackData)) != SA_AIS_OK){
                        SmfImmCreateOperation* createOperation = dynamic_cast<SmfImmCreateOperation*>(*iter);
                        TRACE("SmfImmUtils::doImmOperations: Check if create operation");
                        if (createOperation != NULL) {
                		//The base type or versioned type may already exist, this shall not be considered an error
				std::string className = createOperation->getClassName();
				if((result == SA_AIS_ERR_EXIST) && 
				   (className.find("SaAmf") == 0) &&                                       //Begins with "SaAmf"
				   (className.rfind("Type") + sizeof("Type") - 1 == className.size())) {   //Ends with "Type"
                			TRACE("Base/versioned type (%s) already exists, continue",createOperation->getClassName().c_str());
                                        /* We should not rollback this non creation */
                                        delete rollbackData; 
                                        rollbackData = NULL;
                		} else {
                			LOG_NO("Creation of object failed, rc=%s, class=[%s], parent=[%s]", 
					       saf_error(result), createOperation->getClassName().c_str(), createOperation->getParentDn().c_str() );
                                        delete rollbackData;
                			return result;
                		}
                        }
                        else {
        			LOG_NO("Execution of IMM operation failed, rc=%s", saf_error(result));
                                delete rollbackData;
        			return result;
                        }
		}

                if (rollbackData != NULL) {
                        io_rollbackCcb->addCcbData(rollbackData);
                }
	}

	/* Apply the CCB */
	result = immutil_saImmOmCcbApply(immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_NO("saImmOmCcbApply failed rc=%s", saf_error(result));
		return result;
	}

	/* Finalize CCB and release the IMM CCB handle */
	result = immutil_saImmOmCcbFinalize(immCcbHandle);
	if (result != SA_AIS_OK) {
		LOG_NO("saImmOmCcbFinalize failed rc=%s", saf_error(result));
		return result;
	}

	return result;
}

// ------------------------------------------------------------------------------
// nodeToCmlNode()
// ------------------------------------------------------------------------------
bool
SmfImmUtils::nodeToClmNode(const std::string& i_node, std::string& o_clmNode)
{
        //Convert the AMF nodes in activation unit list to CLM-nodes
        std::string className;
        SaImmAttrValuesT_2 **attributes;
        if (getObject(i_node, &attributes) == false) {
                LOG_NO("Failed to get IMM node object [%s]", i_node.c_str());
                return false;
        }

        className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes,
                                          SA_IMM_ATTR_CLASS_NAME, 0);

        if (className.empty()) {
                LOG_NO("Failed to get class name for node object [%s]", i_node.c_str());
                return false;
        }

        if (className == "SaClmNode") {
                o_clmNode = i_node;
        } else if (className == "SaAmfNode") {
                o_clmNode = osaf_extended_name_borrow(immutil_getNameAttr((const SaImmAttrValuesT_2 **)attributes,
                                                                          "saAmfNodeClmNode", 0));
                if (o_clmNode.empty()) {
                        LOG_NO("Failed to get clm node for amf node object [%s]", i_node.c_str());
                        return false;
                }
        } else {
                LOG_NO("Unknown class name [%s] for node object [%s]", className.c_str(), i_node.c_str());
                return false;
        }

        return true;
}
//------------------------------------------------------------------------------
// Reads IMM configuration data for long DNs and sets cb data structure
//------------------------------------------------------------------------------
bool
SmfImmUtils::read_IMM_long_DN_config_and_set_control_block(smfd_cb_t * cb)
{
	TRACE_ENTER();

	//here is the only place where "kOsafMaxDnLength" constant is directly used
	uint32_t maxDnLength = kOsafMaxDnLength;

	/* First check if long DNs already enabled.
	 * If enabled then there is no need to check it again,
	 * since once it is enabled, is never turned off again.
	 */
	if(cb->maxDnLength == maxDnLength) {
		TRACE("read_IMM_long_DN_config_and_set_control_block(): Long DNs already enabled");
		TRACE_LEAVE();
		return true;
	}

	/* Set the default value first,
	 * to make sure that a value is set,
	 * even if this function fails to get the config from IMM.
	 */
	cb->maxDnLength = DEFAULT_MAX_DN_LENGTH;

	SaImmAttrValuesT_2 **attributes;

	if(getObject(IMM_CONFIG_OBJECT_DN, &attributes) == false) {
                LOG_ER("Could not get IMM config object from IMM %s", IMM_CONFIG_OBJECT_DN);
                TRACE_LEAVE();
                return false;
	}

	const SaUint32T *longDnsAllowed = immutil_getUint32Attr((const SaImmAttrValuesT_2 **)attributes, IMM_LONG_DN_CONFIG_ATTRIBUTE_NAME, 0);
	if(longDnsAllowed) {
		TRACE("%s=%u", IMM_LONG_DN_CONFIG_ATTRIBUTE_NAME, *longDnsAllowed);
		if(*longDnsAllowed == 0)
			cb->maxDnLength = DEFAULT_MAX_DN_LENGTH;
		else
			cb->maxDnLength = maxDnLength;
	} else
		LOG_NO("Could not get long DN config [%s %s], use default DN length", IMM_LONG_DN_CONFIG_ATTRIBUTE_NAME, IMM_CONFIG_OBJECT_DN);
	TRACE_LEAVE();
	return true;
}

// ------------------------------------------------------------------------------
// smf_stringToImmType()
// ------------------------------------------------------------------------------
bool 
smf_stringToImmType(char *i_type, SaImmValueTypeT& o_type)
{
	if (!strcmp("SA_IMM_ATTR_SAINT32T", i_type)) {
		o_type = SA_IMM_ATTR_SAINT32T;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SAUINT32T", i_type)) {
		o_type = SA_IMM_ATTR_SAUINT32T;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SAINT64T", i_type)) {
		o_type = SA_IMM_ATTR_SAINT64T;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SAUINT64T", i_type)) {
		o_type = SA_IMM_ATTR_SAUINT64T;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SATIMET", i_type)) {
		o_type = SA_IMM_ATTR_SATIMET;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SANAMET", i_type)) {
		o_type = SA_IMM_ATTR_SANAMET;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SAFLOATT", i_type)) {
		o_type = SA_IMM_ATTR_SAFLOATT;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SADOUBLET", i_type)) {
		o_type = SA_IMM_ATTR_SADOUBLET;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SASTRINGT", i_type)) {
		o_type = SA_IMM_ATTR_SASTRINGT;
		return true;
	}
	else if (!strcmp("SA_IMM_ATTR_SAANYT", i_type)) {
		o_type = SA_IMM_ATTR_SAANYT;
		return true;
	}
	else {
		LOG_NO("SmfUtils::smf_stringToImmType unknown type (%s)", i_type);
	}
	return false;
}

static const char* immTypeNames[SA_IMM_ATTR_SAANYT + 1] = {
        NULL,
        "SA_IMM_ATTR_SAINT32T",
        "SA_IMM_ATTR_SAUINT32T",
        "SA_IMM_ATTR_SAINT64T",
        "SA_IMM_ATTR_SAUINT64T",
        "SA_IMM_ATTR_SATIMET",
        "SA_IMM_ATTR_SANAMET",
        "SA_IMM_ATTR_SAFLOATT",
        "SA_IMM_ATTR_SADOUBLET",
        "SA_IMM_ATTR_SASTRINGT",
        "SA_IMM_ATTR_SAANYT"
};

// ------------------------------------------------------------------------------
// smf_immTypeToString()
// ------------------------------------------------------------------------------
const char* 
smf_immTypeToString(SaImmValueTypeT i_type)
{
        if (i_type < SA_IMM_ATTR_SAINT32T || i_type > SA_IMM_ATTR_SAANYT) {
		LOG_NO("SmfUtils::smf_immTypeToString type %d not found", i_type);
                return NULL;
        }
	return immTypeNames[i_type];
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
		LOG_NO("SmfUtils::smf_stringToImmAttrModType type %s not found", i_type);
	}
	return (SaImmAttrModificationTypeT)0;
}

//------------------------------------------------------------------------------
// smf_stringsToValues()
//------------------------------------------------------------------------------
bool 
smf_stringsToValues(SaImmAttrValuesT_2 * i_attribute, std::list < std::string >& i_values)
{
	//Allocate space for pointers to all values of the attributes. attrValues points to the first element
	i_attribute->attrValues = (SaImmAttrValueT *) malloc(sizeof(SaImmAttrValueT) * i_attribute->attrValuesNumber);

	//The value variable is increased at the end of the loop for each new value in a multivalue attribute
	SaImmAttrValueT *value = i_attribute->attrValues;

	std::list < std::string >::iterator iter;

	for (iter = i_values.begin(); iter != i_values.end(); iter++) {
                if (!smf_stringToValue(i_attribute->attrValueType, value,  (*iter).c_str())) {
                        LOG_NO("SmfUtils:smf_stringsToValues: Fails to convert a string to value");
                        return false;
                }
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

                if (len > smfd_cb->maxDnLength) {
                        LOG_NO("smf_stringToValue error, SaNameT value too long (%zu), max %zu",
                                len, static_cast<size_t>(smfd_cb->maxDnLength));
                        return false;
                }

                *i_value = malloc(sizeof(SaNameT));
                osaf_extended_name_alloc(i_str, ((SaNameT *) * i_value));
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
                LOG_NO("smf_stringToValue: BAD TYPE (%d) specified for value (%s)",i_type ,i_str);
                return false;
        }		//End switch

        return true;
}


std::string smf_valueToString(SaImmAttrValueT value, SaImmValueTypeT type)
{
    SaNameT* namep;
    char* str;
    SaAnyT* anyp;
    std::ostringstream ost;

    switch (type)
    {
        case SA_IMM_ATTR_SAINT32T: 
            ost << *((int *) value);
            break;
        case SA_IMM_ATTR_SAUINT32T: 
            ost << *((unsigned int *) value);
            break;
        case SA_IMM_ATTR_SAINT64T:  
            ost << *((long long *) value);
            break;
        case SA_IMM_ATTR_SAUINT64T: 
            ost << *((unsigned long long *) value);
            break;
        case SA_IMM_ATTR_SATIMET:   
            ost << *((unsigned long long *) value);
            break;
        case SA_IMM_ATTR_SAFLOATT:  
            ost << *((float *) value);
            break;
        case SA_IMM_ATTR_SADOUBLET: 
            ost << *((double *) value);
            break;
        case SA_IMM_ATTR_SANAMET:
            namep = (SaNameT *) value;

            if (!osaf_is_extended_name_empty(namep))
            {
                ost << (char*) osaf_extended_name_borrow(namep);
            }
            break;
        case SA_IMM_ATTR_SASTRINGT:
            str = *((SaStringT *) value);
            ost << str;
            break;
        case SA_IMM_ATTR_SAANYT:
            anyp = (SaAnyT *) value;

            for (unsigned int i = 0; i < anyp->bufferSize; i++)
            {
                ost << std::hex
                    << (((int)anyp->bufferAddr[i] < 0x10)? "0":"")
                << (int)anyp->bufferAddr[i];
            }

            break;
        default:
            std::cout << "Unknown value type - exiting" << std::endl;
            exit(1);
    }

    return ost.str().c_str();
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
		LOG_NO("SmfUtils::smf_opStringToInt type %s not found, aborting", i_str);
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

//------------------------------------------------------------------------------
// updateSaflog()
//------------------------------------------------------------------------------
void
updateSaflog(const std::string& i_dn, const uint32_t& i_stateId, const uint32_t& i_newState, const uint32_t& i_oldState)
{
	const std::string newStateStr = smfStateToString(i_stateId, i_newState);
	const std::string oldStateStr = smfStateToString(i_stateId, i_oldState);
	saflog(LOG_NOTICE, smfApplDN, "%s State %s => %s", i_dn.c_str(), oldStateStr.c_str(), newStateStr.c_str());
}

//------------------------------------------------------------------------------
// smfStateToString()
//------------------------------------------------------------------------------
const std::string 
smfStateToString(const uint32_t& i_stateId, const uint32_t& i_state)
{
	if (i_stateId == SA_SMF_STEP_STATE) {
		switch (i_state) {
		case 1:
			return "INITIAL";
			break;
		case 2:
			return "EXECUTING";
			break;
		case 3:
			return "UNDOING";
			break;
		case 4:
			return "COMPLETED";
			break;
		case 5:
			return "UNDONE";
			break;
		case 6:
			return "FAILED";
			break;
		case 7:
			return "ROLLING_BACK";
			break;
		case 8:
			return "UNDOING_ROLLBACK";
			break;
		case 9:
			return "ROLLED_BACK";
			break;
		case 10:
			return "ROLLBACK_UNDONE";
			break;
		case 11:
			return "ROLLBACK_FAILED";
			break;
		default:
			return "Unknown step state";
			break;
		}
			
	} else if (i_stateId == SA_SMF_PROCEDURE_STATE) {
		switch (i_state) {
		case 1:
			return "INITIAL";
			break;
		case 2:
			return "EXECUTING";
			break;
		case 3:
			return "SUSPENDED";
			break;
		case 4:
			return "COMPLETED";
			break;
		case 5:
			return "STEP_UNDONE";
			break;
		case 6:
			return "FAILED";
			break;
		case 7:
			return "ROLLING_BACK";
			break;
		case 8:
			return "ROLLBACK_SUSPENDED";
			break;
		case 9:
			return "ROLLED_BACK";
			break;
		case 10:
			return "ROLLBACK_FAILED";
			break;
		default:
			return "Unknown procedure state";
			break;
		}

	} else if (i_stateId == SA_SMF_CAMPAIGN_STATE) {
		switch (i_state) {
		case 1:
			return "INITIAL";
			break;
		case 2:
			return "EXECUTING";
			break;
		case 3:
			return "SUSPENDING_EXECUTION";
			break;
		case 4:
			return "EXECUTION_SUSPENDED";
			break;
		case 5:
			return "EXECUTION_COMPLETED";
			break;
		case 6:
			return "CAMPAIGN_COMMITTED";
			break;
		case 7:
			return "ERROR_DETECTED";
			break;
		case 8:
			return "SUSPENDED_BY_ERROR_DETECTED";
			break;
		case 9:
			return "ERROR_DETECTED_IN_SUSPENDING";
			break;
		case 10:
			return "EXECUTION_FAILED";
			break;
		case 11:
			return "ROLLING_BACK";
			break;
		case 12:
			return "SUSPENDING_ROLLBACK";
			break;
		case 13:
			return "ROLLBACK_SUSPENDED";
			break;
		case 14:
			return "ROLLBACK_COMPLETED";
			break;
		case 15:
			return "ROLLBACK_COMMITTED";
			break;
		case 16:
			return "ROLLBACK_FAILED";
			break;
		default:
			return "Unknown campaign state";
			break;
		}
	} else {
		return "Unknown state ID";
	}
}
