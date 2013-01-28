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
#include "stdio.h"
#include "logtrace.h"
#include "SmfRollback.hh"
#include "SmfImmOperation.hh"
#include "SmfCampaignThread.hh"
#include "SmfUtils.hh"

#include <saImmOm.h>
#include <immutil.h>
#include <saImm.h>

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

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */
SaAisErrorT 
smfCreateRollbackElement(const std::string & i_dn, SaImmOiHandleT i_oiHandle)
{
	TRACE("Create rollback element '%s'", i_dn.c_str());

        SaAisErrorT result = SA_AIS_OK;
	SmfImmRTCreateOperation icoRollbackCcb;

        std::string parentDn = i_dn.substr(i_dn.find(',') + 1, std::string::npos);
        std::string rdnStr = i_dn.substr(0, i_dn.find(','));

	TRACE("Create rollback element, parent '%s', rdn '%s'", parentDn.c_str(), rdnStr.c_str());

        icoRollbackCcb.setClassName("OpenSafSmfRollbackElement");
	icoRollbackCcb.setParentDn(parentDn);
	icoRollbackCcb.setImmHandle(i_oiHandle);

        SmfImmAttribute rdn;
        rdn.setName("smfRollbackElement");
        rdn.setType("SA_IMM_ATTR_SASTRINGT");
        rdn.addValue(rdnStr);
        icoRollbackCcb.addValue(rdn);

        result = icoRollbackCcb.execute(); // Create the object
        return result;
}

//================================================================================
// Class SmfRollbackData
// Purpose:
// Comments:
//================================================================================

SmfRollbackData::SmfRollbackData(SmfRollbackCcb* i_ccb) :
        m_ccb(i_ccb)
{
}

// ------------------------------------------------------------------------------
// ~SmfRollbackData()
// ------------------------------------------------------------------------------
SmfRollbackData::~SmfRollbackData()
{
}

//------------------------------------------------------------------------------
// setType()
//------------------------------------------------------------------------------
void 
SmfRollbackData::setType(const std::string & i_type)
{
	m_type = i_type;
}

//------------------------------------------------------------------------------
// setDn()
//------------------------------------------------------------------------------
void 
SmfRollbackData::setDn(const std::string & i_dn)
{
	m_dn = i_dn;
}

//------------------------------------------------------------------------------
// setClass()
//------------------------------------------------------------------------------
void 
SmfRollbackData::setClass(const std::string & i_class)
{
	m_class = i_class;
}

//------------------------------------------------------------------------------
// addAttrValue()
//------------------------------------------------------------------------------
void 
SmfRollbackData::addAttrValue(const std::string & i_attrName,
                              const std::string & i_attrType,
                              const std::string & i_attrValue)
{
	m_attrValues.push_back(std::string(i_attrName + ":" + i_attrType + "#" + i_attrValue));
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackData::execute(SaImmOiHandleT i_oiHandle)
{
        SaAisErrorT result = SA_AIS_OK;

        /* 
           1) Create rollback data object with the rollback CCB as parent
        */

	SmfImmRTCreateOperation icoRollbackData;

	icoRollbackData.setClassName("OpenSafSmfRollbackData");
	icoRollbackData.setParentDn(m_ccb->getDn());
	icoRollbackData.setImmHandle(i_oiHandle);

        char idStr[16];
        sprintf(idStr, "%08u", m_id);
        std::string rdnValue = "smfRollbackData=";
        rdnValue += idStr;

        SmfImmAttribute rdn;
        rdn.setName("smfRollbackData");
        rdn.setType("SA_IMM_ATTR_SASTRINGT");
        rdn.addValue(rdnValue);
        icoRollbackData.addValue(rdn);

        SmfImmAttribute typeAttr;
        typeAttr.setName("smfRollbackType");
        typeAttr.setType("SA_IMM_ATTR_SASTRINGT");
        typeAttr.addValue(m_type);
        icoRollbackData.addValue(typeAttr);

        SmfImmAttribute dnAttr;
        dnAttr.setName("smfRollbackDn");
        dnAttr.setType("SA_IMM_ATTR_SANAMET");
        dnAttr.addValue(m_dn);
        icoRollbackData.addValue(dnAttr);

        SmfImmAttribute classAttr;
        classAttr.setName("smfRollbackClass");
        classAttr.setType("SA_IMM_ATTR_SASTRINGT");
        classAttr.addValue(m_class);
        icoRollbackData.addValue(classAttr);

        SmfImmAttribute attrValueAttr;
        attrValueAttr.setName("smfRollbackAttrValue");
        attrValueAttr.setType("SA_IMM_ATTR_SASTRINGT");

        if (m_attrValues.size() == 0) {
                attrValueAttr.addValue("");
        }
        else {
                std::list < std::string >::iterator iter;

                for (iter = m_attrValues.begin(); iter != m_attrValues.end(); iter++) {
                        attrValueAttr.addValue((*iter));
                }
        }

        icoRollbackData.addValue(attrValueAttr);
        
        result = icoRollbackData.execute(); // Create the object
	if (result != SA_AIS_OK){
                LOG_ER("Failed to create IMM rollback data object. returned %d", result);
        }

	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackData::rollback(const std::string& i_dn, 
                          std::list < SmfImmOperation * >& io_operationList)
{
        SmfImmUtils immUtil;
        SaImmAttrValuesT_2 **attributes;
        const char *typeAttr;

        /* 
           1) Read IMM object representing this rollback data and create an IMM operation object
        */

        TRACE("Rollback of data in %s", i_dn.c_str());

        if (immUtil.getObject(i_dn, &attributes) == false) {
                LOG_ER("failed to get rollback data imm object %s", i_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        typeAttr = immutil_getStringAttr((const SaImmAttrValuesT_2 **) attributes, 
                                         "smfRollbackType", 0);

        if (typeAttr == NULL) {
                LOG_ER("Could not find smfRollbackType attribute in %s", i_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        if (!strcmp(typeAttr, "CREATE")) {
                return rollbackCreateOperation((const SaImmAttrValuesT_2 **)attributes, io_operationList);
        }
        else if (!strcmp(typeAttr, "DELETE")) {
                return rollbackDeleteOperation((const SaImmAttrValuesT_2 **)attributes, io_operationList);
        }
        else if (!strcmp(typeAttr, "MODIFY")) {
                return rollbackModifyOperation((const SaImmAttrValuesT_2 **)attributes, io_operationList);
        }
        else {
                LOG_ER("unknown type attribute value %s in %s", typeAttr, i_dn.c_str());
                return SA_AIS_ERR_FAILED_OPERATION;
        }

	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// rollbackCreateOperation()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackData::rollbackCreateOperation(const SaImmAttrValuesT_2 ** i_attributes, 
                                         std::list < SmfImmOperation * >& io_operationList)
{
        const SaNameT *dnAttr;
        const char *classAttr;
        const char *attrValueString;
        SaAisErrorT result;
        SaUint32T   noOfAttrValues;

        /* 
           1) Read rollback data for a create operation and create corresponding IMM operation
        */

        /* TODO not finished yet */

        dnAttr = immutil_getNameAttr(i_attributes, "smfRollbackDn", 0);
        if (dnAttr == NULL) {
                LOG_ER("Could not find smfRollbackDn attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        classAttr = immutil_getStringAttr(i_attributes, "smfRollbackClass", 0);
        if (classAttr == NULL) {
                LOG_ER("Could not find smfRollbackClass attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        result = immutil_getAttrValuesNumber((char*)"smfRollbackAttrValue", i_attributes, &noOfAttrValues);

        if (result != SA_AIS_OK) {
                LOG_ER("Could not find smfRollbackAttrValue attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        TRACE("Rollback create object %s no of attributes %d", (const char*) dnAttr->value, noOfAttrValues);

        if (noOfAttrValues == 0) {
                LOG_ER("At least one attribute (rdn) is needed when createing object");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        SmfImmCreateOperation* immOp = new (std::nothrow) SmfImmCreateOperation();
        if (immOp == NULL) {
                LOG_ER("Could not create SmfImmCreateOperation");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        immOp->setParentDn((const char*) dnAttr->value);
        immOp->setClassName(classAttr);

        for (unsigned int index = 0; index < noOfAttrValues; index ++) {
                attrValueString = immutil_getStringAttr(i_attributes, "smfRollbackAttrValue", index);
                if (attrValueString == NULL) {
                        LOG_ER("Could not get smfRollbackAttrValue index %u, no of values %u", index, noOfAttrValues);
                        delete immOp;
                        return SA_AIS_ERR_FAILED_OPERATION;
                }

                TRACE("Rollback create object attribute value string %s", attrValueString);
                std::string attrValueStr(attrValueString);
                unsigned int colonPos = attrValueStr.find(':');
                unsigned int hashPos = attrValueStr.find('#');
                std::string attrName = attrValueStr.substr(0, colonPos);
                std::string attrType = attrValueStr.substr(colonPos + 1, hashPos - colonPos - 1);
                std::string attrValue = attrValueStr.substr(hashPos + 1, std::string::npos);

                TRACE("Rollback create object attribute name %s, type %s, value '%s'", 
                      attrName.c_str(), attrType.c_str(), attrValue.c_str());
                immOp->addAttrValue(attrName, attrType, attrValue);
        }

        io_operationList.push_back(immOp);

	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// rollbackDeleteOperation()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackData::rollbackDeleteOperation(const SaImmAttrValuesT_2 ** i_attributes, 
                                         std::list < SmfImmOperation * >& io_operationList)
{
        const SaNameT *dnAttr;

        /* 
           1) Read rollback data for a delete operation and create corresponding IMM operation
        */

        dnAttr = immutil_getNameAttr(i_attributes, "smfRollbackDn", 0);
        if (dnAttr == NULL) {
                LOG_ER("Could not find smfRollbackDn attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        SmfImmDeleteOperation* immOp = new (std::nothrow) SmfImmDeleteOperation();
        if (immOp == NULL) {
                LOG_ER("Could not create SmfImmDeleteOperation");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        TRACE("Rollback delete object %s", (const char*) dnAttr->value);

        immOp->setDn((const char*) dnAttr->value);

        io_operationList.push_back(immOp);

	return SA_AIS_OK;
}

//------------------------------------------------------------------------------
// rollbackModifyOperation()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackData::rollbackModifyOperation(const SaImmAttrValuesT_2 ** i_attributes, 
                                         std::list < SmfImmOperation * >& io_operationList)
{
        const SaNameT *dnAttr;
        const char *attrValueString;
        SaAisErrorT result;
        SaUint32T   noOfAttrValues;

        /* 
           1) Read rollback data for a modify replace operation and create corresponding IMM operation
        */

        /* TODO not finished yet */

        dnAttr = immutil_getNameAttr(i_attributes, "smfRollbackDn", 0);
        if (dnAttr == NULL) {
                LOG_ER("Could not find smfRollbackDn attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        result = immutil_getAttrValuesNumber((char*)"smfRollbackAttrValue", i_attributes, &noOfAttrValues);

        if (result != SA_AIS_OK) {
                LOG_ER("Could not find smfRollbackAttrValue attribute");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        if (noOfAttrValues == 0) {
                LOG_ER("At least one attribute needs to be modified");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        SmfImmModifyOperation* immOp = new (std::nothrow) SmfImmModifyOperation();
        if (immOp == NULL) {
                LOG_ER("Could not create SmfImmCreateOperation");
                return SA_AIS_ERR_FAILED_OPERATION;
        }

        TRACE("Rollback modify object %s, no of attributes %d", dnAttr->value, noOfAttrValues);

        immOp->setDn((const char*)dnAttr->value);
        immOp->setOp("SA_IMM_ATTR_VALUES_REPLACE");

        for (unsigned int index = 0; index < noOfAttrValues; index ++) {
                attrValueString = immutil_getStringAttr(i_attributes, "smfRollbackAttrValue", index);
                if (attrValueString == NULL) {
                        LOG_ER("Could not get smfRollbackAttrValue index %u, no of values %u", index, noOfAttrValues);
                        delete immOp;
                        return SA_AIS_ERR_FAILED_OPERATION;
                }

                TRACE("Rollback modify object attribute value string %s", attrValueString);
                std::string attrValueStr(attrValueString);
                unsigned int colonPos = attrValueStr.find(':');
                unsigned int hashPos = attrValueStr.find('#');
                std::string attrName = attrValueStr.substr(0, colonPos);
                std::string attrType = attrValueStr.substr(colonPos + 1, hashPos - colonPos - 1);
                std::string attrValue = attrValueStr.substr(hashPos + 1, std::string::npos);

                TRACE("Rollback modify object attribute name %s, type %s, value '%s'", 
                      attrName.c_str(), attrType.c_str(), attrValue.c_str());

                immOp->addAttrValue(attrName, attrType, attrValue);
        }

        io_operationList.push_back(immOp);

	return SA_AIS_OK;
}

//================================================================================
// Class SmfRollbackCcb
// Purpose:
// Comments:
//================================================================================

SmfRollbackCcb::SmfRollbackCcb(const std::string& i_dn, SaImmOiHandleT i_oiHandle):
	m_dn(i_dn),
        m_dataId(1),
        m_oiHandle(i_oiHandle)
{
}

// ------------------------------------------------------------------------------
// ~SmfRollbackCcb()
// ------------------------------------------------------------------------------
SmfRollbackCcb::~SmfRollbackCcb()
{
	std::list < SmfRollbackData * >::iterator iter;

	for (iter = m_rollbackData.begin();
              iter != m_rollbackData.end();
              iter++) {
                delete((*iter));
	}
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackCcb::execute()
{
        SaAisErrorT result = SA_AIS_OK;
        /* 
           1) For each rollback data : call execute 
        */

        std::list < SmfRollbackData * >::iterator dataIter;

	for (dataIter = m_rollbackData.begin(); dataIter != m_rollbackData.end(); dataIter++) {
                if ((result = (*dataIter)->execute(m_oiHandle)) != SA_AIS_OK) {
                        LOG_ER("Failed to execute rollback data %d", result);
                        break;
                }
	}

	return result;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfRollbackCcb::rollback()
{
        SaAisErrorT result = SA_AIS_OK;

        /* 
           1) Search for all rollback data IMM objects below this ccb object
        */

        std::list < std::string > rollbackData;
        SmfImmUtils immUtil;
        immUtil.getChildren(m_dn, rollbackData, SA_IMM_SUBLEVEL, "OpenSafSmfRollbackData");

        if (rollbackData.size() == 0) {
                LOG_NO("No rollback data found for ccb %s", m_dn.c_str());
                return SA_AIS_OK;
        }

        TRACE("Rollback %zu operations in CCB %s", rollbackData.size(), m_dn.c_str());

        /* Sort the rollback data list */
        rollbackData.sort();

        /* 
           2) For each rollback data object (in reverse order) : 
                  create a new SmfRollbackData and call rollback 
        */

        std::list < SmfImmOperation* > operationList;
        std::list < std::string >::reverse_iterator iter;

        /* Loop through the rollback data list in reverse order */
        for (iter = rollbackData.rbegin(); iter != rollbackData.rend(); iter++) {
                SmfRollbackData rollbackData(this);
                if ((result = rollbackData.rollback((*iter), operationList)) != SA_AIS_OK) {
                        LOG_ER("rollback of %s failed %d", (*iter).c_str(), result);
                        break;
                }
        }
        /* 
           3) Use SmfImmUtils object and call doImmOperations
                  with list of IMM operations created by rollback data objects
        */

        if (result == SA_AIS_OK) {
                if ((result = immUtil.doImmOperations(operationList)) != SA_AIS_OK) {
                        LOG_ER("rollback ccb operations failed for %s rc %d", m_dn.c_str(), result);
                }
        }

        /* Remove all operations */
        std::list < SmfImmOperation* >::iterator opIter;

        for (opIter = operationList.begin(); opIter != operationList.end(); opIter++) {
                delete (*opIter);
        }

	return result;
}

//------------------------------------------------------------------------------
// addCcbData()
//------------------------------------------------------------------------------
void 
SmfRollbackCcb::addCcbData(SmfRollbackData* i_data)
{
        i_data->setId(m_dataId++);
        m_rollbackData.push_back(i_data);
}

