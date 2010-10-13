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
#include <assert.h>
#include <list>
#include <string>
#include "stdio.h"
#include "logtrace.h"
#include "SmfImmOperation.hh"
#include "SmfUtils.hh"

#include <saImmOm.h>
#include <immutil.h>
#include <saImm.h>

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
	LOG_ER("addValue must be specialised");
}

//------------------------------------------------------------------------------
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmOperation::execute()
{
	LOG_ER("execute must be specialised");
	return (SaAisErrorT)-1;
}

//------------------------------------------------------------------------------
// rollback()
//------------------------------------------------------------------------------
int 
SmfImmOperation::rollback()
{
	LOG_ER("rollback must be specialised");
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
void 
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
		assert(attr != 0);

		attr->attrName = (char *)(*iter).m_name.c_str();
		attr->attrValueType = smf_stringToImmType((char *)(*iter).m_type.c_str());

		assert((*iter).m_values.size() > 0);	//Must have at least one value
		attr->attrValuesNumber = (*iter).m_values.size();

		smf_stringsToValues(attr, (*iter).m_values);	//Convert the string to a SA Forum type

		//Add the pointer to the SaImmAttrValuesT_2 structure to the attributes list
		attributeValues[i++] = attr;

		iter++;
	}

	attributeValues[i] = NULL;	//Null terminate the list of attribute pointers
	this->setAttrValues(attributeValues);

	TRACE_LEAVE();
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
SmfImmCreateOperation::execute()
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	this->createAttrValues();

	const char *className = m_className.c_str();

	if (m_parentDn.length() > SA_MAX_NAME_LENGTH) {
		LOG_ER("SmfImmCreateOperation::execute:createObject failed Too long parent name %zu",
		       m_parentDn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (!m_ccbHandle) {
		LOG_ER("SmfImmCreateOperation::execute: no ccb handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_ER("SmfImmCreateOperation::execute: no imm owner handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immAttrValues) {
		LOG_ER("SmfImmCreateOperation::execute: no SaImmAttrValuesT_2** is set");
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
	objectName.length = (SaUint16T) m_parentDn.length();
	memcpy(objectName.value, m_parentDn.c_str(), objectName.length);

        //Set IMM ownership.
        //When creating objects at top level the parent name is empty and the ownership shall not be set
        if (objectName.length > 0) { 
		const SaNameT *objectNames[2];
		objectNames[0] = &objectName;
		objectNames[1] = NULL;
		result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
		if (result != SA_AIS_OK) {
			TRACE("SmfImmCreateOperation::execute:saImmOmAdminOwnerSet failed %u\n", result);
                        TRACE_LEAVE();
			return result;
		} 
        }

	//Create CCB
	result = immutil_saImmOmCcbObjectCreate_2(m_ccbHandle, (const SaImmClassNameT)
						  className, &objectName, (const SaImmAttrValuesT_2 **)
						  m_immAttrValues);
	if (result != SA_AIS_OK) {
		//The creation may fail if the base type already exist, it shall not be considered an error
		if ((result == SA_AIS_ERR_EXIST) & (m_className.find("BaseType") != std::string::npos)) {
			TRACE("SmfImmCreateOperation::execute: base type object already created, OK");
			result = SA_AIS_OK;
		} else {
			LOG_ER("SmfImmCreateOperation::execute:saImmOmCcbObjectCreate_2 failed %u", result);
			TRACE_LEAVE();
			return result;
		}
	}
	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to TRUE
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize
#if 0
	result = immutil_saImmOmAdminOwnerRelease(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmDeleteOperation::execute:saImmOmAdminOwnerRelease failed %u\n", result);
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
SmfImmCreateOperation::rollback()
{
	LOG_ER("Rollback not implemented yet");
	return -1;
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
SmfImmDeleteOperation::execute()
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	if (!m_ccbHandle) {
		LOG_ER("SmfImmDeleteOperation::execute: no ccb handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_ER("SmfImmDeleteOperation::execute: no imm owner handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}
	//Set IMM ownership
	SaNameT objectName;
	objectName.length = (SaUint16T) m_dn.length();
	memcpy(objectName.value, m_dn.c_str(), objectName.length);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	//Set IMM ownership
	result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmDeleteOperation::execute:saImmOmAdminOwnerSet failed %u\n", result);
                TRACE_LEAVE();
		return result;
	}

	result = immutil_saImmOmCcbObjectDelete(m_ccbHandle, &objectName);
	if (result != SA_AIS_OK) {
		LOG_ER("SmfImmDeleteOperation::execute:immutil_saImmOmCcbObjectDelete failed %u", result);
                TRACE_LEAVE();
		return result;
	}
	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to TRUE
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize
#if 0
	result = immutil_saImmOmAdminOwnerRelease(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmDeleteOperation::execute:saImmOmAdminOwnerRelease failed %u\n", result);
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
	LOG_ER("Rollback not implemented yet");
	return -1;
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
void 
SmfImmModifyOperation::createAttrMods(void)
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
		assert(mod != 0);
		mod->modType = smf_stringToImmAttrModType((char *)m_op.c_str());	//Convert an store the modification type from string to SA Forum type
		mod->modAttr.attrName = (char *)(*iter).m_name.c_str();
		mod->modAttr.attrValueType = smf_stringToImmType((char *)(*iter).m_type.c_str());
		TRACE("Modifying %s:%s = %s", m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.front().c_str());

		assert((*iter).m_values.size() > 0);	//Must have at least one value
		mod->modAttr.attrValuesNumber = (*iter).m_values.size();

		smf_stringsToValues(&mod->modAttr, (*iter).m_values);	//Convert the string to a SA Forum type

		//Add the pointer to the SaImmAttrModificationT_2 structure to the modifications list
		attributeModifications[i++] = mod;

		iter++;
	}

	attributeModifications[i] = NULL;	//Null terminate the list of modification pointers
	m_immAttrMods = attributeModifications;

	TRACE_LEAVE();
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
// execute()
//------------------------------------------------------------------------------
SaAisErrorT 
SmfImmModifyOperation::execute()
{
	TRACE_ENTER();

	SaAisErrorT result = SA_AIS_OK;

	//Convert the strings to structures and types accepted by the IMM interface
	this->createAttrMods();

	if (!m_ccbHandle) {
		LOG_ER("SmfImmModifyOperation::execute: no ccb handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immOwnerHandle) {
		LOG_ER("SmfImmModifyOperation::execute: no imm owner handle set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	if (!m_immAttrMods) {
		LOG_ER("SmfImmModifOperation::execute: no SaImmAttrModificationT_2** is set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}
	//Set IMM ownership
	SaNameT objectName;
	objectName.length = (SaUint16T) m_dn.length();
	memcpy(objectName.value, m_dn.c_str(), objectName.length);

	const SaNameT *objectNames[2];
	objectNames[0] = &objectName;
	objectNames[1] = NULL;

	//Set IMM ownership
	result = immutil_saImmOmAdminOwnerSet(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmModifOperation::execute:saImmOmAdminOwnerSet failed %u\n", result);
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
		LOG_ER("SmfImmModifOperation::execute:saImmOmCcbObjectModify failed %u", result);
                TRACE_LEAVE();
		return result;
	}
	//Release IMM ownership
	//This is not needed when saImmOmAdminOwnerInitialize "releaseOwnershipOnFinalize" is set to TRUE
	//The ownership will be automatically released at saImmOmAdminOwnerFinalize
#if 0
	result = immutil_saImmOmAdminOwnerRelease(m_immOwnerHandle, objectNames, SA_IMM_ONE);
	if (result != SA_AIS_OK) {
		TRACE("SmfImmCreateOperation::execute:saImmOmAdminOwnerRelease failed %u\n", result);
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
	LOG_ER("Rollback not implemented yet");
	return -1;
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
void 
SmfImmRTCreateOperation::createAttrValues(void)
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
		assert(attr != 0);

		attr->attrName = (char *)(*iter).m_name.c_str();
		attr->attrValueType = smf_stringToImmType((char *)(*iter).m_type.c_str());

#if 0
		if (iter->m_values.size() == 0) {
			TRACE("c=[%s], p=[%s], attr=[%s]", 
			      m_className.c_str(), m_parentDn.c_str(), attr->attrName);
		}
		assert((*iter).m_values.size() > 0);	//Must have at least one value
#endif
		TRACE("c=[%s], p=[%s], attr=[%s]", m_className.c_str(), m_parentDn.c_str(), attr->attrName);

		attr->attrValuesNumber = (*iter).m_values.size();

		smf_stringsToValues(attr, (*iter).m_values);	//Convert the string to a SA Forum type

		//Add the pointer to the SaImmAttrValuesT_2 structure to the attributes list
		attributeValues[i++] = attr;

		iter++;
	}

	attributeValues[i] = NULL;	//Null terminate the list of attribute pointers
	this->setAttrValues(attributeValues);

	TRACE_LEAVE();
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
	this->createAttrValues();

	const char *className = m_className.c_str();

	if (m_parentDn.length() > SA_MAX_NAME_LENGTH) {
		LOG_ER("SmfImmRTCreateOperation::execute:createObject failed Too long parent name %zu",
		       m_parentDn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (!m_immAttrValues) {
		LOG_ER("SmfImmRTCreateOperation::execute: no SaImmAttrValuesT_2** is set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	SaNameT parentName;
	parentName.length = m_parentDn.length();
	strncpy((char *)parentName.value, m_parentDn.c_str(), parentName.length);
	parentName.value[parentName.length] = 0;

	result = immutil_saImmOiRtObjectCreate_2(m_immHandle,
                                                 (char*)className, 
                                                 &parentName, 
                                                 (const SaImmAttrValuesT_2**)m_immAttrValues);

	if (result != SA_AIS_OK) {
		TRACE("saImmOiRtObjectCreate_2 returned %u for %s, parent %s", result, className, parentName.value);
	}

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
void 
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
		assert(mod != 0);
		mod->modType = smf_stringToImmAttrModType((char *)m_op.c_str());	//Convert an store the modification type from string to SA Forum type
		mod->modAttr.attrName = (char *)(*iter).m_name.c_str();
		mod->modAttr.attrValueType = smf_stringToImmType((char *)(*iter).m_type.c_str());
		TRACE("Modifying %s:%s = %s", m_dn.c_str(), (*iter).m_name.c_str(), (*iter).m_values.front().c_str());

		assert((*iter).m_values.size() > 0);	//Must have at least one value
		mod->modAttr.attrValuesNumber = (*iter).m_values.size();

		smf_stringsToValues(&mod->modAttr, (*iter).m_values);	//Convert the string to a SA Forum type

		//Add the pointer to the SaImmAttrModificationT_2 structure to the modifications list
		attributeModifications[i++] = mod;

		iter++;
	}

	attributeModifications[i] = NULL;	//Null terminate the list of modification pointers
	m_immAttrMods = attributeModifications;

	TRACE_LEAVE();
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
	this->createAttrMods();

	if (m_dn.length() > SA_MAX_NAME_LENGTH) {
		LOG_ER("SmfImmRTCreateOperation::execute:createObject failed Too long DN %zu",
		       m_dn.length());
                TRACE_LEAVE();
		return SA_AIS_ERR_NAME_TOO_LONG;
	}

	if (!m_immAttrMods) {
		LOG_ER("SmfImmRTCreateOperation::execute: no SaImmAttrValuesT_2** is set");
                TRACE_LEAVE();
		return SA_AIS_ERR_UNAVAILABLE;
	}

	SaNameT objectName;
	objectName.length = m_dn.length();
	strncpy((char *)objectName.value, m_dn.c_str(), objectName.length);
	objectName.value[objectName.length] = 0;

	result = immutil_saImmOiRtObjectUpdate_2(m_immHandle,
                                                 &objectName, 
                                                 (const SaImmAttrModificationT_2**)m_immAttrMods);

	if (result != SA_AIS_OK) {
		TRACE("saImmOiRtObjectUpdate_2 returned %u for %s", result, objectName.value);
	}

	TRACE_LEAVE();

	return result;

}
