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
#include <list>
#include <string>
#include <string.h>
#include "stdio.h"
#include "logtrace.h"
#include "SmfImmOperation.hh"
#include "SmfRollback.hh"
#include "SmfUtils.hh"

#include "saAis.h"
#include <saImmOm.h>
#include <immutil.h>
#include <saImm.h>
#include <saf_error.h>
#include "osaf_extended_name.h"

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

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

//================================================================================
// Class SmfImmAttribute
// Purpose:
// Comments:
//================================================================================

SmfImmAttribute::SmfImmAttribute()
{
}

// ------------------------------------------------------------------------------
// ~SmfImmAttribute()
// ------------------------------------------------------------------------------
SmfImmAttribute::~SmfImmAttribute()
{
}

//------------------------------------------------------------------------------
// setName()
//------------------------------------------------------------------------------
void
SmfImmAttribute::setName(const std::string & i_name)
{
	m_name = i_name;
}

//------------------------------------------------------------------------------
// getName()
//------------------------------------------------------------------------------
const std::string & 
SmfImmAttribute::getName()
{
        return m_name;
}

//------------------------------------------------------------------------------
// setType()
//------------------------------------------------------------------------------
void 
SmfImmAttribute::setType(const std::string & i_type)
{
	m_type = i_type;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmAttribute::addValue(const std::string & i_value)
{
	m_values.push_back(i_value);
}

//------------------------------------------------------------------------------
// getValues()
//------------------------------------------------------------------------------
const std::list < std::string > & 
SmfImmAttribute::getValues()
{
	return m_values;
}

//================================================================================
// Class SmfImmOperation
// Purpose:
// Comments:
//================================================================================

SmfImmOperation::SmfImmOperation():
	m_ccbHandle(0),
	m_immOwnerHandle(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfImmOperation()
// ------------------------------------------------------------------------------
SmfImmOperation::~SmfImmOperation()
{
}

//------------------------------------------------------------------------------
// setCcbHandle()
//------------------------------------------------------------------------------
void
SmfImmOperation::setCcbHandle(SaImmCcbHandleT i_ccbHandle)
{
	m_ccbHandle = i_ccbHandle;
}

//------------------------------------------------------------------------------
// setImmOwnerHandle()
//------------------------------------------------------------------------------
void 
SmfImmOperation::setImmOwnerHandle(SaImmAdminOwnerHandleT i_immOwnerHandle)
{
	m_immOwnerHandle = i_immOwnerHandle;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmOperation::addValue(const SmfImmAttribute & i_value)
{
	LOG_NO("addValue must be specialised");
}

//------------------------------------------------------------------------------
// addAttrValue()
//------------------------------------------------------------------------------
void 
SmfImmOperation::addAttrValue(const std::string & i_name,
                              const std::string & i_type,
                              const std::string & i_value)
{
	LOG_NO("addAttrValue must be specialised");
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmOperation::execute(SmfRollbackData* o_rollbackData)
{
	LOG_NO("execute must be specialised");
	return (SaAisErrorT)-1;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
int 
SmfImmOperation::rollback()
{
	LOG_NO("rollback must be specialised");
	return -1;
}

//================================================================================
// Class SmfImmCreateOperation
// Purpose:
// Comments:
//================================================================================

SmfImmCreateOperation::SmfImmCreateOperation():
   SmfImmOperation(), 
   m_className(""), 
   m_parentDn(""), 
   m_values(0), 
   m_immAttrValues(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfImmCreateOperation()
// ------------------------------------------------------------------------------
SmfImmCreateOperation::~SmfImmCreateOperation()
{	
}

//------------------------------------------------------------------------------
// setClassName()
//------------------------------------------------------------------------------
void
SmfImmCreateOperation::setClassName(const std::string & i_name)
{
	m_className = i_name;
}

//------------------------------------------------------------------------------
// getClassName()
//------------------------------------------------------------------------------
const std::string &
SmfImmCreateOperation::getClassName()
{
	return m_className;
}

//------------------------------------------------------------------------------
// setParentDn()
//------------------------------------------------------------------------------
void 
SmfImmCreateOperation::setParentDn(const std::string & i_dn)
{
	m_parentDn = i_dn;
}

//------------------------------------------------------------------------------
// getParentDn()
//------------------------------------------------------------------------------
const std::string & 
SmfImmCreateOperation::getParentDn()
{
	return m_parentDn;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmCreateOperation::addValue(const SmfImmAttribute & i_value)
{
	m_values.push_back(i_value);
}

//------------------------------------------------------------------------------
// addAttrValue()
//------------------------------------------------------------------------------
void 
SmfImmCreateOperation::addAttrValue(const std::string & i_name,
                                    const std::string & i_type,
                                    const std::string & i_value)
{
        /* Check if attribute already exists (MULTI_VALUE) then add the 
           value to this attribute */

	std::list < SmfImmAttribute >::iterator iter;

	// For all attributes 
	for (iter = m_values.begin(); iter != m_values.end(); iter++) {
                if ((*iter).m_name == i_name) {
                        (*iter).addValue(i_value);
                        return;
                }
	}
        /* Attribute don't exists, create a new one and add to list */
        SmfImmAttribute newAttr;
        newAttr.setName(i_name);
        newAttr.setType(i_type);
        newAttr.addValue(i_value);

	m_values.push_back(newAttr);
}


//------------------------------------------------------------------------------
// getValues()
//------------------------------------------------------------------------------
const std::list < SmfImmAttribute > & 
SmfImmCreateOperation::getValues()
{
        return m_values;
}

//------------------------------------------------------------------------------
// createAttrValues()
//------------------------------------------------------------------------------
bool 
SmfImmCreateOperation::createAttrValues(void)
{
	TRACE_ENTER();

	//Create space for attributes
	SaImmAttrValuesT_2 **attributeValues = (SaImmAttrValuesT_2 **) new SaImmAttrValuesT_2 *[m_values.size() + 1];

	std::list < SmfImmAttribute >::iterator iter;
	std::list < SmfImmAttribute >::iterator iterE;

	int i = 0;		//Index to a SaImmAttrValuesT pointer in the attributeValues array

	iter = m_values.begin();
	iterE = m_values.end();

	//For all attribures to create
	while (iter != iterE) {
		//Create structure for one attribute
		SaImmAttrValuesT_2 *attr = new(std::nothrow) SaImmAttrValuesT_2();
		osafassert(attr != 0);
		attr->attrName = (char *)(*iter).m_name.c_str();
		if (smf_stringToImmType((char *)(*iter).m_type.c_str(), attr->attrValueType) == false) {
			delete [] attributeValues;
			delete attr;
			LOG_NO("Fails to convert string (%s) to imm type", (char *)(*iter).m_type.c_str());
			TRACE_LEAVE();
			return false;
		}

		if ((*iter).m_values.size() == 0) {	//Must have at least one valueSmfImmRTCreateOperation::execute:
			LOG_NO("Attr value is not given for attr name %s", (*iter).m_name.c_str());
			attr->attrValuesNumber = 0;
		} else if ((*iter).m_values.size() == 1 && (!strcmp((*iter).getValues().front().c_str(),"<_empty_>"))) {
			attr->attrValuesNumber = 0;
		} else {
			attr->attrValuesNumber = (*iter).m_values.size();
			if (smf_stringsToValues(attr, (*iter).m_values) == false) {	//Convert the string to a SA Forum type
				free(attr->attrValues);
				delete attr;
				delete [] attributeValues;
				LOG_NO("Fails to convert strings to values");
				TRACE_LEAVE();
				return false;
			}
		}
        
		//Add the pointer to the SaImmAttrValuesT_2 structure to the attributes list
		attributeValues[i++] = attr;

		iter++;
	}

	attributeValues[i] = NULL;	//Null terminate the list of attribute pointers
	this->setAttrValues(attributeValues);

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// setAttrValues()
//------------------------------------------------------------------------------
void 
SmfImmCreateOperation::setAttrValues(SaImmAttrValuesT_2 ** i_values)
{
	m_immAttrValues = i_values;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmCreateOperation::execute(SmfRollbackData* o_rollbackData)
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	if (this->createAttrValues() == false) {
		LOG_NO("Faied to convert to IMM attr structure");
		TRACE_LEAVE();
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	const char *className = m_className.c_str();

	if (m_parentDn.length() > smfd_cb->maxDnLength) {
		LOG_NO("Object create op failed, parent name too long [%zu] max=[%zu], parent=[%s]",
                        m_parentDn.length(), static_cast<size_t>(smfd_cb->maxDnLength),
                        m_parentDn.c_str());
		TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (!m_ccbHandle) {
		LOG_NO("SmfImmCreateOperation::execute: no ccb handle set");
		TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_NO("SmfImmCreateOperation::execute: no imm owner handle set");
		TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immAttrValues) {
		LOG_NO("SmfImmCreateOperation::execute: no SaImmAttrValuesT_2** is set");
		TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}
	TRACE("ObjectCreate; parent=[%s], class=[%s]", m_parentDn.c_str(),m_className.c_str());
#if 1
	std::list<SmfImmAttribute>::iterator i;
	for (i = m_values.begin(); i != m_values.end(); i++) {
		TRACE("    %s %s:", i->m_type.c_str(), i->m_name.c_str());
		if (i->m_values.size() > 0) {
			std::list<std::string>::iterator s;
			for (s = i->m_values.begin(); s != i->m_values.end(); s++) {
				TRACE("       %s", s->c_str());
			}
		}
	}
#endif

	SaNameT objectName;
        osaf_extended_name_lend(m_parentDn.c_str(), &objectName);

        //Set IMM ownership.
        //When creating objects at top level the parent name is empty and the ownership shall not be set
        if (!osaf_is_extended_name_empty(&objectName)) {
		const SaNameT *objectNames[2];
		objectNames[0] = &objectName;
		objectNames[1] = NULL;
		result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
		if (result != SA_AIS_OK) {
			TRACE("SmfImmCreateOperation::execute:saImmOmAdminOwnerSet failed %s\n", saf_error(result));
                        TRACE_LEAVE();
			return result;
		} 
        }

	//Create CCB
	result = immutil_saImmOmCcbObjectCreate_2(m_ccbHandle, (SaImmClassNameT)className, 
						  &objectName, (const SaImmAttrValuesT_2 **)m_immAttrValues);
	if (result != SA_AIS_OK) {
		if (result == SA_AIS_ERR_EXIST) {
			TRACE("SmfImmCreateOperation::execute: object already exists");
		} else {
			LOG_NO("Failed to create object of class=[%s] to parent=[%s]. rc=%s,", m_className.c_str(), m_parentDn.c_str(), saf_error(result));
			TRACE_LEAVE();
			return result;
		}
	}

	//Free the m_immAttrValues mem
	for (int i = 0; m_immAttrValues[i] != 0; i++) {
		SaImmAttrValueT* value = m_immAttrValues[i]->attrValues;
		for (unsigned int k = 0; m_immAttrValues[i]->attrValuesNumber > k; k++) {
			if (m_immAttrValues[i]->attrValueType == SA_IMM_ATTR_SASTRINGT) {
				free(*((SaStringT *) *value));
			} else if (m_immAttrValues[i]->attrValueType == SA_IMM_ATTR_SAANYT) {
				free(((SaAnyT *) * value)->bufferAddr);
			}

			free(*value);
			value++;
		}
			
		free(m_immAttrValues[i]->attrValues);
		delete m_immAttrValues[i];
	}
	delete [] m_immAttrValues;

        if (o_rollbackData != NULL) {
                SaAisErrorT rollbackResult;
                if ((rollbackResult = this->prepareRollback(o_rollbackData)) != SA_AIS_OK) {
                        LOG_NO("SmfImmCreateOperation::execute, Failed to prepare rollback data rc=%s", saf_error(rollbackResult));
                        TRACE_LEAVE();
                        return SA_AIS_ERR_FAILED_OPERATION;
                }
        }

	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to true
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize

	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
int 
SmfImmCreateOperation::rollback()
{
	LOG_NO("Rollback not implemented yet");
	return -1;
}

//------------------------------------------------------------------------------
// prepareRollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmCreateOperation::prepareRollback(SmfRollbackData* o_rollbackData)
{
        SmfImmUtils immUtil;
        SaImmAttrDefinitionT_2 ** attributeDefs = NULL;
 
        if (immUtil.getClassDescription(m_className, &attributeDefs) == false) {
                LOG_NO("SmfImmCreateOperation::prepareRollback, Could not find class %s", m_className.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        // Find the SA_IMM_ATTR_RDN attribute
        std::string rdnAttrName;
        for (int i = 0; attributeDefs[i] != 0; i++) {
                SaImmAttrFlagsT flags = attributeDefs[i]->attrFlags;
                if ((flags & SA_IMM_ATTR_RDN) == SA_IMM_ATTR_RDN) {
                        rdnAttrName = (char*)attributeDefs[i]->attrName;
                        break;
                }
        }

        immUtil.classDescriptionMemoryFree(attributeDefs);
        if (rdnAttrName.length() == 0) {
                LOG_NO("SmfImmCreateOperation::prepareRollback, could not find RDN attribute in class %s", m_className.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        // Find the value of the RDN
	std::list < SmfImmAttribute >::iterator iter;
        std::string rdnAttrValue;

	for (iter = m_values.begin(); iter != m_values.end(); iter++) {
		if (rdnAttrName == (*iter).m_name) {
			if ((*iter).m_values.size() != 1){
				LOG_NO("SmfImmCreateOperation::prepareRollback, attribute %s contain %zu values, must contain exactly one value",
				       rdnAttrName.c_str(), (*iter).m_values.size());
				return SA_AIS_ERR_FAILED_OPERATION;
			}
                        rdnAttrValue = (*iter).m_values.front();
                        break;
                }
	}
        if (rdnAttrValue.length() == 0) {
                LOG_NO("SmfImmCreateOperation::prepareRollback, could not find RDN value for %s, class %s", rdnAttrName.c_str(), m_className.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        TRACE("SmfImmCreateOperation::prepareRollback, Found RDN %s=%s", rdnAttrName.c_str(), rdnAttrValue.c_str());

        /* Prepare deletion of created object at rollback */
        o_rollbackData->setType("DELETE");
        std::string delDn;
        delDn = rdnAttrValue;
        if (m_parentDn.length() > 0) {
                delDn += ",";
                delDn += m_parentDn;
        }
        o_rollbackData->setDn(delDn);

	return SA_AIS_OK;
}

//================================================================================
// Class SmfImmDeleteOperation
// Purpose:
// Comments:
//================================================================================

SmfImmDeleteOperation::SmfImmDeleteOperation():
   SmfImmOperation()
{
}

// ------------------------------------------------------------------------------
// ~SmfImmDeleteOperation()
// ------------------------------------------------------------------------------
SmfImmDeleteOperation::~SmfImmDeleteOperation()
{
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void
SmfImmDeleteOperation::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// getDn()
//------------------------------------------------------------------------------
const std::string &
SmfImmDeleteOperation::getDn()
{
	return m_dn;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmDeleteOperation::execute(SmfRollbackData* o_rollbackData)
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	if (!m_ccbHandle) {
		LOG_NO("SmfImmDeleteOperation::execute: no ccb handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_NO("SmfImmDeleteOperation::execute: no imm owner handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (m_dn.length() > smfd_cb->maxDnLength) {
		LOG_NO("SmfImmDeleteOperation::execute: failed Too long dn %zu",
		       m_dn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	//Set IMM ownership
	SaNameT objectName;
        osaf_extended_name_lend(m_dn.c_str(), &objectName);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	//Set IMM ownership
	result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmDeleteOperation::execute, saImmOmAdminOwnerSet failed rc=%s", saf_error(result));
                TRACE_LEAVE();
		return result;
	}

        if (o_rollbackData != NULL) {
                if ((result = this->prepareRollback(o_rollbackData)) != SA_AIS_OK) {
                        LOG_NO("SmfImmDeleteOperation::execute, failed to prepare rollback data rc=%s", saf_error(result));
                        TRACE_LEAVE();
                        return SA_AIS_ERR_FAILED_OPERATION;
                }
        }

	result = immutil_saImmOmCcbObjectDelete(m_ccbHandle, &objectName);
	if (result != SA_AIS_OK) {
		LOG_NO("SmfImmDeleteOperation::execute, immutil_saImmOmCcbObjectDelete failed rc=%s (child objects maybe exists)", saf_error(result));
                TRACE_LEAVE();
		return result;
	}

	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to true
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize
#if 0
	result = immutil_saImmOmAdminOwnerRelease(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("saImmOmAdminOwnerRelease failed %s", saf_error(result));
		return result;
	}
#endif

	TRACE_LEAVE();

	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
int 
SmfImmDeleteOperation::rollback()
{
	LOG_NO("Rollback not implemented yet");
	return -1;
}

//------------------------------------------------------------------------------
// prepareRollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmDeleteOperation::prepareRollback(SmfRollbackData* o_rollbackData)
{
        SmfImmUtils               immUtil;
        SaImmAttrDefinitionT_2 ** attributeDefs;
        SaImmAttrValuesT_2 **     attributes;
        SaImmAttrValuesT_2 *      attr;
        const char*               className;
        int                       i = 0;

        if (immUtil.getObject(m_dn, &attributes) == false) {
                LOG_NO("Could not find object %s", m_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        className = immutil_getStringAttr((const SaImmAttrValuesT_2 **)attributes, SA_IMM_ATTR_CLASS_NAME, 0);
        if (className == NULL) {
                LOG_NO("SmfImmDeleteOperation::prepareRollback, could not find class name for %s", m_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        if (immUtil.getClassDescription(className, &attributeDefs) == false) {
                LOG_NO("SmfImmDeleteOperation::prepareRollback, could not find class %s", className);
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        /* For each attribute in the object that is not RUNTIME or IMM internal 
           store the current value in rollback data for future use at rollback */

        std::string parentDn;
        const char* tmpParentDn = m_dn.c_str();
        const char* commaPos;

        /* Find first comma not prepended by a \ */
        while ((commaPos = strchr(tmpParentDn, ',')) != NULL) {
                if (*(commaPos - 1) != '\\') {
                        parentDn = (commaPos + 1);
                        break;
                }
                tmpParentDn = commaPos + 1;
        }

        o_rollbackData->setType("CREATE");
        o_rollbackData->setDn(parentDn);
        o_rollbackData->setClass(className);

        while ((attr = attributes[i++]) != NULL) {

                // Check if IMM internal attribute
                if (!strcmp(attr->attrName, SA_IMM_ATTR_CLASS_NAME)) {
                        continue;
                }
                else if (!strcmp(attr->attrName, SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                        continue;
                }
                else if (!strcmp(attr->attrName, SA_IMM_ATTR_IMPLEMENTER_NAME)) {
                        continue;
                }

                // Exclude RUNTIME attributes without PERSISTENT
                bool saveAttribute = true;
                for (int i = 0; attributeDefs[i] != 0; i++) {
                        if (!strcmp(attr->attrName, attributeDefs[i]->attrName)) {
                                SaImmAttrFlagsT flags = attributeDefs[i]->attrFlags;
                                if ((flags & SA_IMM_ATTR_RUNTIME) == SA_IMM_ATTR_RUNTIME) {
                                        if ((flags & SA_IMM_ATTR_PERSISTENT) != SA_IMM_ATTR_PERSISTENT) {
                                                saveAttribute = false;
                                        }
                                } 
                                break;
                        }
                }

                if (saveAttribute == true) {
                        if (attr->attrValuesNumber == 0) {
                                o_rollbackData->addAttrValue(attr->attrName, 
                                                             smf_immTypeToString(attr->attrValueType), 
                                                             "<_empty_>");
                        } else {
                                for (unsigned int j = 0; j < attr->attrValuesNumber; j++) {
                                        o_rollbackData->addAttrValue(attr->attrName, 
                                                                     smf_immTypeToString(attr->attrValueType), 
                                                                     smf_valueToString(attr->attrValues[j],attr->attrValueType));
                                }       
                        }
                }
        }

        immUtil.classDescriptionMemoryFree(attributeDefs);

	return SA_AIS_OK;
}

//================================================================================
// Class SmfImmModifyOperation
// Purpose:
// Comments:
//================================================================================

SmfImmModifyOperation::SmfImmModifyOperation():
   SmfImmOperation(), 
   m_dn(""), 
   m_rdn(""), 
   m_op(""), 
   m_values(0), 
   m_immAttrMods(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfImmModifyOperation()
// ------------------------------------------------------------------------------
SmfImmModifyOperation::~SmfImmModifyOperation()
{
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void
SmfImmModifyOperation::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// getDn()
//------------------------------------------------------------------------------
const std::string & 
SmfImmModifyOperation::getDn()
{
	return m_dn;
}

//------------------------------------------------------------------------------
// setRdn()
//------------------------------------------------------------------------------
void
SmfImmModifyOperation::setRdn(const std::string & i_rdn)
{
	m_rdn = i_rdn;
}

//------------------------------------------------------------------------------
// getRdn()
//------------------------------------------------------------------------------
const std::string & 
SmfImmModifyOperation::getRdn()
{
	return m_rdn;
}

//------------------------------------------------------------------------------
// setOp()
//------------------------------------------------------------------------------
void 
SmfImmModifyOperation::setOp(const std::string & i_op)
{
	m_op = i_op;
}

//------------------------------------------------------------------------------
// createAttrMods()
//------------------------------------------------------------------------------
bool 
SmfImmModifyOperation::createAttrMods(void)
{
	TRACE_ENTER();

	SaImmAttrModificationT_2 **attributeModifications =
	    (SaImmAttrModificationT_2 **) new(std::nothrow) SaImmAttrModificationT_2 *[m_values.size() + 1];
	osafassert(attributeModifications != 0);

	int i = 0;		//Index to a SaImmAttrModificationT_2 pointer in the attributeModificationss array

	std::list < SmfImmAttribute >::iterator iter;
	std::list < SmfImmAttribute >::iterator iterE;

	iter = m_values.begin();
	iterE = m_values.end();
	while (iter != iterE) {
		SaImmAttrModificationT_2 *mod = new(std::nothrow) SaImmAttrModificationT_2();
		osafassert(mod != 0);
		mod->modType = smf_stringToImmAttrModType((char *)m_op.c_str());	//Convert an store the modification type from string to SA Forum type
		if (mod->modType == 0) {
			delete [] attributeModifications;
			delete mod;
			LOG_NO("SmfImmModifyOperation::createAttrMods, fails to convert string (%s) to type", (char *)m_op.c_str());
			TRACE_LEAVE();
			return false;
		}
		mod->modAttr.attrName = (char *)(*iter).m_name.c_str();
		if (smf_stringToImmType((char *)(*iter).m_type.c_str(), mod->modAttr.attrValueType) == false) {
			LOG_NO("SmfImmModifyOperation::createAttrMods, failes to convert attr [%s] type to valid IMM type", mod->modAttr.attrName);
			delete [] attributeModifications;
			delete mod;
			TRACE_LEAVE();			
			return false;
		}
		TRACE("Modifying %s:%s = %s", m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.front().c_str());

		if ((*iter).m_values.size() <= 0){
			LOG_NO("SmfImmModifyOperation::createAttrMods, attribute %s contain %zu values, must contain at least one value",
			       (*iter).m_name.c_str(),(*iter).m_values.size());
			delete [] attributeModifications;
			delete mod;
			TRACE_LEAVE();
			return false;
		}
                if ((*iter).m_values.size() == 1 && (!strcmp((*iter).getValues().front().c_str(),"<_empty_>"))) {
                        mod->modAttr.attrValuesNumber = 0;
                }
                else {
                        mod->modAttr.attrValuesNumber = (*iter).m_values.size();
                        if (smf_stringsToValues(&mod->modAttr, (*iter).m_values) == false) {	//Convert the string to a SA Forum type
				LOG_NO("SmfImmModifyOperation::createAttrMods, failes to convert attr [%s] value string to attr value", mod->modAttr.attrName);
				delete [] attributeModifications;
				delete mod;
				TRACE_LEAVE();
				return false;
			}
		}

                //Add the pointer to the SaImmAttrModificationT_2 structure to the modifications list
                attributeModifications[i++] = mod;

		iter++;
	}

	attributeModifications[i] = NULL;	//Null terminate the list of modification pointers
	m_immAttrMods = attributeModifications;

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmModifyOperation::addValue(const SmfImmAttribute & i_value)
{
	m_values.push_back(i_value);
}

//------------------------------------------------------------------------------
// addAttrValue()
//------------------------------------------------------------------------------
void 
SmfImmModifyOperation::addAttrValue(const std::string & i_name,
                                    const std::string & i_type,
                                    const std::string & i_value)
{
        /* Check if attribute already exists (MULTI_VALUE) then add the 
           value to this attribute */

	std::list < SmfImmAttribute >::iterator iter;

	// For all attributes 
	for (iter = m_values.begin(); iter != m_values.end(); iter++) {
                if ((*iter).m_name == i_name) {
                        (*iter).addValue(i_value);
                        return;
                }
	}
        /* Attribute don't exists, create a new one and add to list */
        SmfImmAttribute newAttr;
        newAttr.setName(i_name);
        newAttr.setType(i_type);
        newAttr.addValue(i_value);

	m_values.push_back(newAttr);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmModifyOperation::execute(SmfRollbackData* o_rollbackData)
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	if (this->createAttrMods() == false){
		LOG_NO("SmfImmModifyOperation::execute, fail to convert to IMM attr structure");
		TRACE_LEAVE();
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	if (!m_ccbHandle) {
		LOG_NO("SmfImmModifyOperation::execute, no ccb handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_NO("SmfImmModifyOperation::execute, no imm owner handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immAttrMods) {
		LOG_NO("SmfImmModifyOperation::execute, no SaImmAttrModificationT_2** is set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (m_dn.length() > smfd_cb->maxDnLength) {
		LOG_NO("SmfImmModifOperation::execute: failed Too long dn %zu",
		       m_dn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	//Set IMM ownership
	SaNameT objectName;
        osaf_extended_name_lend(m_dn.c_str(), &objectName);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	//Set IMM ownership
	result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmModifyOperation::execute, SaImmOmAdminOwnerSet failed %s", saf_error(result));
                TRACE_LEAVE();
		return result;
	}
	//Create CCB
//   SaNameT objectName;
//   objectName.length = (SaUint16T) objectLen;
//   memcpy(objectName.value, object, objectLen);

	result = immutil_saImmOmCcbObjectModify_2(m_ccbHandle, &objectName, (const SaImmAttrModificationT_2 **)
						  m_immAttrMods);
	if (result != SA_AIS_OK) {
		LOG_NO("SmfImmModifyOperation::execute, saImmOmCcbObjectModify failed %s", saf_error(result));
                TRACE_LEAVE();
		return result;
	}

	//Free the m_immAttrMods mem
	for (int i = 0; m_immAttrMods[i] != 0; i++) {
		SaImmAttrValueT* value = m_immAttrMods[i]->modAttr.attrValues;
		for (unsigned int k = 0; m_immAttrMods[i]->modAttr.attrValuesNumber > k; k++) {
			if (m_immAttrMods[i]->modAttr.attrValueType == SA_IMM_ATTR_SASTRINGT) {
				free(*((SaStringT *) *value));
			} else if (m_immAttrMods[i]->modAttr.attrValueType == SA_IMM_ATTR_SAANYT) {
				free(((SaAnyT *) * value)->bufferAddr);
			}

			free(*value);
			value++;
		}
			
		free(m_immAttrMods[i]->modAttr.attrValues);
		delete m_immAttrMods[i];
	}
	delete [] m_immAttrMods;


        if (o_rollbackData != NULL) {
                if ((result = this->prepareRollback(o_rollbackData)) != SA_AIS_OK) {
                        LOG_NO("SmfImmModifyOperation::execute, failed to prepare rollback data %s", saf_error(result));
                        TRACE_LEAVE();
                        return SA_AIS_ERR_FAILED_OPERATION;
                }
        }

	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to true
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize
#if 0
	result = immutil_saImmOmAdminOwnerRelease(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("saImmOmAdminOwnerRelease failed %s", saf_error(result));
		return result;
	}
#endif

	TRACE_LEAVE();
	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
int 
SmfImmModifyOperation::rollback()
{
	LOG_NO("Rollback not implemented yet");
	return -1;
}

//------------------------------------------------------------------------------
// prepareRollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmModifyOperation::prepareRollback(SmfRollbackData* o_rollbackData)
{
        SmfImmUtils               immUtil;
        SaImmAttrValuesT_2 **     attributes;
        SaImmAttrValuesT_2 *      attr;
        int                       i;

        if (immUtil.getObject(m_dn, &attributes) == false) {
                LOG_NO("Could not find object %s", m_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        o_rollbackData->setType("MODIFY");
        o_rollbackData->setDn(m_dn);

	std::list < SmfImmAttribute >::iterator iter;

       /* For each modified attribute in the object 
          store the current value in rollback data for future use at rollback */

	for (iter = m_values.begin(); iter != m_values.end(); iter++) {
                i = 0;
                while ((attr = attributes[i++]) != NULL) {
                        if (!strcmp((*iter).m_name.c_str(), attr->attrName)) {        
                                if (attr->attrValuesNumber == 0) {
                                        o_rollbackData->addAttrValue(attr->attrName, 
                                                                     smf_immTypeToString(attr->attrValueType), 
                                                                     "<_empty_>");
                                } else {
                                        for (unsigned int j = 0; j < attr->attrValuesNumber; j++) {
                                                o_rollbackData->addAttrValue(attr->attrName, 
                                                                             smf_immTypeToString(attr->attrValueType), 
                                                                             smf_valueToString(attr->attrValues[j],attr->attrValueType));
                                        }       
                                }
                                break;
                        }
                }
        }
	return SA_AIS_OK;
}

//================================================================================
// Class SmfImmRTCreateOperation
// Purpose:
// Comments:
//================================================================================

SmfImmRTCreateOperation::SmfImmRTCreateOperation():
   m_className(""), 
   m_parentDn(""),
   m_immHandle(0),
   m_values(0), 
   m_immAttrValues(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfImmRTCreateOperation()
// ------------------------------------------------------------------------------
SmfImmRTCreateOperation::~SmfImmRTCreateOperation()
{
}

//------------------------------------------------------------------------------
// setClassName()
//------------------------------------------------------------------------------
void
SmfImmRTCreateOperation::setClassName(const std::string & i_name)
{
	m_className = i_name;
}

//------------------------------------------------------------------------------
// setParentDn()
//------------------------------------------------------------------------------
void 
SmfImmRTCreateOperation::setParentDn(const std::string & i_dn)
{
	m_parentDn = i_dn;
}

//------------------------------------------------------------------------------
// setImmHandle()
//------------------------------------------------------------------------------
void 
SmfImmRTCreateOperation::setImmHandle(const SaImmOiHandleT & i_handle)
{
	m_immHandle = i_handle;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmRTCreateOperation::addValue(const SmfImmAttribute & i_value)
{
	m_values.push_back(i_value);
}

//------------------------------------------------------------------------------
// createAttrValues()
//------------------------------------------------------------------------------
bool 
SmfImmRTCreateOperation::createAttrValues(void)
{
	TRACE_ENTER();

	//Create space for attributes
	SaImmAttrValuesT_2 **attributeValues = (SaImmAttrValuesT_2 **) new(std::nothrow) SaImmAttrValuesT_2 *[m_values.size() + 1];
	osafassert(attributeValues != 0);

	std::list < SmfImmAttribute >::iterator iter;
	std::list < SmfImmAttribute >::iterator iterE;

	int i = 0;		//Index to a SaImmAttrValuesT pointer in the attributeValues array

	iter = m_values.begin();
	iterE = m_values.end();

	//For all attribures to create
	while (iter != iterE) {
		//Create structure for one attribute
		SaImmAttrValuesT_2 *attr = new(std::nothrow) SaImmAttrValuesT_2();
		osafassert(attr != 0);

		attr->attrName = (char *)(*iter).m_name.c_str();
		if (smf_stringToImmType((char *)(*iter).m_type.c_str(), attr->attrValueType) == false) {
			LOG_NO("SmfImmRTCreateOperation::createAttrValues, fail to convert attr [%s] type to valid IMM type", attr->attrName);
			delete attr;
			for (int k = 0; i > k; k++) {
				delete attributeValues[k];
			}
			delete [] attributeValues;
			TRACE_LEAVE();
			return false;
		}

		TRACE("c=[%s], p=[%s], attr=[%s]", m_className.c_str(), m_parentDn.c_str(), attr->attrName);

		attr->attrValuesNumber = (*iter).m_values.size();

		if (smf_stringsToValues(attr, (*iter).m_values) == false) {	//Convert the string to a SA Forum type
			LOG_NO("SmfImmRTCreateOperation::createAttrValues, fail to convert attr [%s] value string to attr value", attr->attrName);
			free(attr->attrValues);
			delete attr;
			for (int k = 0; i > k; k++) {
				delete attributeValues[k];
			}
			delete [] attributeValues;
			TRACE_LEAVE();
			return false;
		}
		//Add the pointer to the SaImmAttrValuesT_2 structure to the attributes list
		attributeValues[i++] = attr;
		iter++;
	}

	attributeValues[i] = NULL;	//Null terminate the list of attribute pointers
	this->setAttrValues(attributeValues);

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// setAttrValues()
//------------------------------------------------------------------------------
void 
SmfImmRTCreateOperation::setAttrValues(SaImmAttrValuesT_2 ** i_values)
{
	m_immAttrValues = i_values;
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmRTCreateOperation::execute()
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	if (this->createAttrValues() == false) {
		LOG_NO("SmfImmRTCreateOperation::execute, fail to convert to IMM attr structure");
		TRACE_LEAVE();
		return SA_AIS_ERR_FAILED_OPERATION;
	}

	const char *className = m_className.c_str();

	if (m_parentDn.length() > smfd_cb->maxDnLength) {
		LOG_NO("SmfImmRTCreateOperation::execute, createObject failed Too long parent name %zu",
		       m_parentDn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (!m_immAttrValues) {
		LOG_NO("SmfImmRTCreateOperation::execute, no SaImmAttrValuesT_2** is set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	SaNameT parentName;
        osaf_extended_name_lend(m_parentDn.c_str(), &parentName);

	result = immutil_saImmOiRtObjectCreate_2(m_immHandle,
                                                 (char*)className, 
                                                 &parentName, 
                                                 (const SaImmAttrValuesT_2**)m_immAttrValues);

	if (result != SA_AIS_OK) {
		TRACE("saImmOiRtObjectCreate_2 returned %s for %s, parent %s", saf_error(result), className, osaf_extended_name_borrow(&parentName));
	}

	//Free the m_immAttrValues mem
	for (int i = 0; m_immAttrValues[i] != 0; i++) {
		SaImmAttrValueT* value = m_immAttrValues[i]->attrValues;
		for (unsigned int k = 0; m_immAttrValues[i]->attrValuesNumber > k; k++) {
			if (m_immAttrValues[i]->attrValueType == SA_IMM_ATTR_SASTRINGT) {
				free(*((SaStringT *) *value));
			} else if (m_immAttrValues[i]->attrValueType == SA_IMM_ATTR_SANAMET) {
				osaf_extended_name_free((SaNameT *) * value);
			} else if (m_immAttrValues[i]->attrValueType == SA_IMM_ATTR_SAANYT) {
				free(((SaAnyT *) * value)->bufferAddr);
			}

			free(*value);
			value++;
		}
			
		free(m_immAttrValues[i]->attrValues);
		delete m_immAttrValues[i];
	}
	delete [] m_immAttrValues;

	TRACE_LEAVE();
	return result;
}

//================================================================================
// Class SmfImmRTUpdateOperation
// Purpose:
// Comments:
//================================================================================

SmfImmRTUpdateOperation::SmfImmRTUpdateOperation():
   m_dn(""), 
   m_op(""),
   m_immHandle(0),
   m_values(0), 
   m_immAttrMods(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfImmRTUpdateOperation()
// ------------------------------------------------------------------------------
SmfImmRTUpdateOperation::~SmfImmRTUpdateOperation()
{
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void
SmfImmRTUpdateOperation::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// setOp()
//------------------------------------------------------------------------------
void 
SmfImmRTUpdateOperation::setOp(const std::string & i_op)
{
	m_op = i_op;
}

//------------------------------------------------------------------------------
// setImmHandle()
//------------------------------------------------------------------------------
void 
SmfImmRTUpdateOperation::setImmHandle(const SaImmOiHandleT & i_handle)
{
	m_immHandle = i_handle;
}

//------------------------------------------------------------------------------
// createAttrMods()
//------------------------------------------------------------------------------
bool 
SmfImmRTUpdateOperation::createAttrMods(void)
{
	TRACE_ENTER();

	SaImmAttrModificationT_2 **attributeModifications =
	    (SaImmAttrModificationT_2 **) new SaImmAttrModificationT_2 *[m_values.size() + 1];

	int i = 0;		//Index to a SaImmAttrModificationT_2 pointer in the attributeModificationss array

	std::list < SmfImmAttribute >::iterator iter;
	std::list < SmfImmAttribute >::iterator iterE;

	iter = m_values.begin();
	iterE = m_values.end();
	while (iter != iterE) {
		SaImmAttrModificationT_2 *mod = new(std::nothrow) SaImmAttrModificationT_2();
		osafassert(mod != 0);
		mod->modType = smf_stringToImmAttrModType((char *)m_op.c_str());	//Convert an store the modification type from string to SA Forum type
		if (mod->modType == 0) {
			delete [] attributeModifications;
			delete mod;
			LOG_NO("SmfImmRTUpdateOperation::createAttrMods, fail convert string to IMM attr mod type [%s:%s]",  m_dn.c_str(), (*iter).m_name.c_str());
			TRACE_LEAVE();
			return false;
		}
		mod->modAttr.attrName = (char *)(*iter).m_name.c_str();
		if (smf_stringToImmType((char *)(*iter).m_type.c_str(), mod->modAttr.attrValueType) == false) {
			delete [] attributeModifications;
			delete mod;
			LOG_NO("SmfImmRTUpdateOperation::createAttrMods, fail to convert string to IMM attr type [%s:%s]",  m_dn.c_str(), (*iter).m_name.c_str());
			TRACE_LEAVE();
			return false;
		}
		TRACE("Modifying %s:%s = %s", m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.front().c_str());

		if ((*iter).m_values.size() <= 0) { //Must have at least one value
			LOG_NO("SmfImmRTUpdateOperation::createAttrMods, No values %s:%s (size=%zu)", m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.size());
			delete [] attributeModifications;
			delete mod;
			TRACE_LEAVE();
			return false;
		}
		mod->modAttr.attrValuesNumber = (*iter).m_values.size();

		if (smf_stringsToValues(&mod->modAttr, (*iter).m_values) == false) { //Convert the string to a SA Forum type
			delete [] attributeModifications;
			delete mod;
			LOG_NO("SmfImmRTUpdateOperation::createAttrMods, fail to conv string to value [%s:%s = %s]",  m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.front().c_str());
			TRACE_LEAVE();
			return false;
		}
		//Add the pointer to the SaImmAttrModificationT_2 structure to the modifications list
		attributeModifications[i++] = mod;

		iter++;
	}

	attributeModifications[i] = NULL;	//Null terminate the list of modification pointers
	m_immAttrMods = attributeModifications;

	TRACE_LEAVE();
	return true;
}

//------------------------------------------------------------------------------
// addValue()
//------------------------------------------------------------------------------
void 
SmfImmRTUpdateOperation::addValue(const SmfImmAttribute & i_value)
{
	m_values.push_back(i_value);
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmRTUpdateOperation::execute()
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	if (this->createAttrMods() == false){
		LOG_NO("SmfImmRTUpdateOperation::execute, fail to convert to IMM attr structure");
		result = SA_AIS_ERR_FAILED_OPERATION;
		goto exit;
	}

	if (m_dn.length() > smfd_cb->maxDnLength) {
		LOG_NO("SmfImmRTUpdateOperation::execute, too long DN [%zu], max=[%zu], dn=[%s]",
                 m_dn.length(), static_cast<size_t>(smfd_cb->maxDnLength), m_dn.c_str());
		result = SA_AIS_ERR_NAME_TOO_LONG;
		goto exit;
	}

	if (m_immAttrMods == NULL) {
		LOG_NO("SmfImmRTUpdateOperation::execute, no SaImmAttrValuesT_2** is set");
		result = SA_AIS_ERR_UNAVAILABLE;
		goto exit;
	}

	SaNameT objectName;
        osaf_extended_name_lend(m_dn.c_str(), &objectName);

	result = immutil_saImmOiRtObjectUpdate_2(m_immHandle,
                                                 &objectName, 
                                                 (const SaImmAttrModificationT_2**)m_immAttrMods);

	if (result != SA_AIS_OK) {
		TRACE("saImmOiRtObjectUpdate_2 returned %s for %s", saf_error(result), osaf_extended_name_borrow(&objectName));
	}

exit:
	//Free the m_immAttrMods mem
	if (m_immAttrMods != NULL) {
		for (int i = 0; m_immAttrMods[i] != 0; i++) {
			SaImmAttrValueT* value = m_immAttrMods[i]->modAttr.attrValues;
			for (unsigned int k = 0; m_immAttrMods[i]->modAttr.attrValuesNumber > k; k++) {
				if (m_immAttrMods[i]->modAttr.attrValueType == SA_IMM_ATTR_SASTRINGT) {
					free(*((SaStringT *) *value));
				} else if (m_immAttrMods[i]->modAttr.attrValueType == SA_IMM_ATTR_SAANYT) {
					free(((SaAnyT *) * value)->bufferAddr);
				}

				free(*value);
				value++;
			}
			
			free(m_immAttrMods[i]->modAttr.attrValues);
			delete m_immAttrMods[i];
		}

		delete [] m_immAttrMods;
	}

	TRACE_LEAVE();
	return result;
}
