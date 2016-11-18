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
#include "base/logtrace.h"
#include "smf/smfd/SmfUpgradeMethod.h"
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
// Class SmfUpgradeMethod
// Purpose:
// Comments:
//================================================================================

SmfUpgradeMethod::SmfUpgradeMethod()
	: m_scope(NULL),
	  m_stepRestartOption(0),
	  m_stepMaxRetryCount(0),
	  callback_list(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfUpgradeMethod()
// ------------------------------------------------------------------------------
SmfUpgradeMethod::~SmfUpgradeMethod()
{
	delete m_scope;

	std::list < SmfCallback * >::iterator cbkit;
	for (cbkit = callback_list.begin(); cbkit != callback_list.end(); ++cbkit) {
		delete(*cbkit);
	}
}

//------------------------------------------------------------------------------
// setUpgradeScope()
//------------------------------------------------------------------------------
void 
SmfUpgradeMethod::setUpgradeScope(SmfUpgradeScope * i_scope)
{
	m_scope = i_scope;
}

//------------------------------------------------------------------------------
// setUpgradeScope()
//------------------------------------------------------------------------------
const SmfUpgradeScope *
SmfUpgradeMethod::getUpgradeScope(void) const
{
	return m_scope;
}

//------------------------------------------------------------------------------
// setStepRestartOption()
//------------------------------------------------------------------------------
void 
SmfUpgradeMethod::setStepRestartOption(const SaUint32T i_opt)
{
	m_stepRestartOption = i_opt;
}

//------------------------------------------------------------------------------
// getStepRestartOption()
//------------------------------------------------------------------------------
SaUint32T 
SmfUpgradeMethod::getStepRestartOption(void) const
{
	return m_stepRestartOption;
}

//------------------------------------------------------------------------------
// setStepMaxRetryCount()
//------------------------------------------------------------------------------
void 
SmfUpgradeMethod::setStepMaxRetryCount(const SaUint32T i_count)
{
	m_stepMaxRetryCount = i_count;
}

//------------------------------------------------------------------------------
// getStepMaxRetryCount()
//------------------------------------------------------------------------------
SaUint32T 
SmfUpgradeMethod::getStepMaxRetryCount(void) const
{
	return m_stepMaxRetryCount;
}

std::list<SmfCallback *>& 
SmfUpgradeMethod::getCallbackList() 
{
	return callback_list;
}

void 
SmfUpgradeMethod::addCallback(SmfCallback *callback)
{
	callback_list.push_back(callback);
}

//================================================================================
// Class SmfRollingUpgrade
// Purpose:
// Comments:
//================================================================================
SmfRollingUpgrade::SmfRollingUpgrade():
    SmfUpgradeMethod()
{
}

// ------------------------------------------------------------------------------
// ~SmfRollingUpgrade()
// ------------------------------------------------------------------------------
SmfRollingUpgrade::~SmfRollingUpgrade()
{
}

//------------------------------------------------------------------------------
// getUpgradeMethod()
//------------------------------------------------------------------------------
SaSmfUpgrMethodT 
SmfRollingUpgrade::getUpgradeMethod(void) const
{
	return SA_SMF_ROLLING;
}


//================================================================================
// Class SmfSinglestepUpgrade
// Purpose:
// Comments:
//================================================================================
SmfSinglestepUpgrade::SmfSinglestepUpgrade():
    SmfUpgradeMethod()
{
}

// ------------------------------------------------------------------------------
// ~SmfSinglestepUpgrade()
// ------------------------------------------------------------------------------
SmfSinglestepUpgrade::~SmfSinglestepUpgrade()
{
}

//------------------------------------------------------------------------------
// getUpgradeMethod()
//------------------------------------------------------------------------------
SaSmfUpgrMethodT 
SmfSinglestepUpgrade::getUpgradeMethod(void) const
{
	return SA_SMF_SINGLE_STEP;
}

//================================================================================
// Class SmfUpgradeScope
//================================================================================
SmfUpgradeScope::SmfUpgradeScope()
{
}

// ------------------------------------------------------------------------------
// ~SmfTargetNodeTemplate()
// ------------------------------------------------------------------------------
SmfUpgradeScope::~SmfUpgradeScope()
{
}

// ------------------------------------------------------------------------------
// removeSwAddRemoveDuplicates()
// ------------------------------------------------------------------------------
void
SmfUpgradeScope::removeSwAddRemoveDuplicates(std::list<SmfBundleRef> &io_addList, std::list<SmfBundleRef> &io_removeList)
{
	TRACE_ENTER();
	std::list<SmfBundleRef>::iterator itAdd;
	std::list<SmfBundleRef>::iterator itRemove;
	std::set<std::string> toBeRemoved;

	// Find out which bundles are specified in both swAdd and SwRemove.
	// Create a set of DN, since the comparison is based on that.
	for (itAdd = io_addList.begin(); itAdd != io_addList.end(); itAdd++) {
		for (itRemove = io_removeList.begin(); itRemove != io_removeList.end(); itRemove++) {
			const std::string& swAddBundleDn = itAdd->getBundleDn();
			if (swAddBundleDn == itRemove->getBundleDn()) {
				TRACE("SmfUpgradeScope::removeSwAddRemoveDuplicates(): Bundle=%s found in <swAdd> and <swRemove> within a procedure, remove from both lists\n", swAddBundleDn.c_str());
				toBeRemoved.insert(swAddBundleDn);
				// Can break now since only need to store the DN once.
				// DNs stored in set, no way to insert a DN twice.
				break;
			}
		}
	}

	// Iterate through the DNs which need to be removed from add and remove lists.
	// Remove all the "add" and "remove" objects which have that specific DN.
	for (std::set<std::string>::iterator i = toBeRemoved.begin(); i != toBeRemoved.end(); i++) {
		removeSwAddRemoveDuplicate(io_addList, *i);
		removeSwAddRemoveDuplicate(io_removeList, *i);
	}
	TRACE_LEAVE();
}
// ------------------------------------------------------------------------------
// removeSwAddRemoveDuplicate()
// ------------------------------------------------------------------------------
void
SmfUpgradeScope::removeSwAddRemoveDuplicate(std::list<SmfBundleRef> &io_addOrRemoveList, const std::string &i_dn)
{
	TRACE_ENTER();
	std::list<SmfBundleRef>::iterator iter = io_addOrRemoveList.begin();
	while(iter != io_addOrRemoveList.end()) {
		if(iter->getBundleDn() == i_dn) {
			TRACE("SmfUpgradeScope::removeSwAddRemoveDuplicate(): erase element \"%s\"",i_dn.c_str());
			io_addOrRemoveList.erase(iter);
			// For safety, set the iterator to the beginning of the list.
			iter = io_addOrRemoveList.begin();
		}
		else
			iter++;
	}
	TRACE_LEAVE();
}

//================================================================================
// Class SmfByTemplate
// Purpose:
// Comments:
//================================================================================
SmfByTemplate::SmfByTemplate():
	m_targetNodeTemplate(0)
{
}

// ------------------------------------------------------------------------------
// ~SmfByTemplate()
// ------------------------------------------------------------------------------
SmfByTemplate::~SmfByTemplate()
{
	delete m_targetNodeTemplate;

	std::list < SmfTargetEntityTemplate * >::iterator it;

	/* Delete target entities */
	for (it = m_targetEntityTemplate.begin(); it != m_targetEntityTemplate.end(); ++it) {
		delete(*it);
	}
}

//------------------------------------------------------------------------------
// setTargetNodeTemplate()
//------------------------------------------------------------------------------
void 
SmfByTemplate::setTargetNodeTemplate(SmfTargetNodeTemplate * i_entity)
{
	m_targetNodeTemplate = i_entity;
}

//------------------------------------------------------------------------------
// getTargetNodeTemplate()
//------------------------------------------------------------------------------
const SmfTargetNodeTemplate *
SmfByTemplate::getTargetNodeTemplate(void) const
{
	return m_targetNodeTemplate;
}

//------------------------------------------------------------------------------
// addTargetEntityTemplate()
//------------------------------------------------------------------------------
void 
SmfByTemplate::addTargetEntityTemplate(SmfTargetEntityTemplate * i_entity)
{
	m_targetEntityTemplate.push_back(i_entity);
}

//------------------------------------------------------------------------------
// getTargetEntityTemplate()
//------------------------------------------------------------------------------
const std::list < SmfTargetEntityTemplate * >&
SmfByTemplate::getTargetEntityTemplate() const
{
	return m_targetEntityTemplate;
}

//================================================================================
// Class SmfForAddRemove
// Purpose:
// Comments:
//================================================================================
SmfForAddRemove::SmfForAddRemove()
	: m_activationUnit(NULL),
	  m_deactivationUnit(NULL)
{
}

SmfForAddRemove::~SmfForAddRemove()
{
	if (m_activationUnit != NULL) delete m_activationUnit;
	if (m_deactivationUnit != NULL) delete m_deactivationUnit;
}

bool 
SmfForAddRemove::setActivationUnit(SmfActivationUnitType* actunit)
{
	if (m_activationUnit != NULL) {
		LOG_ER("Activation unit already set");
		return false;
	}
	m_activationUnit = actunit;
	return true;
}
const SmfActivationUnitType*
SmfForAddRemove::getActivationUnit(void) const
{
	return m_activationUnit;
}
bool 
SmfForAddRemove::setDeactivationUnit(SmfActivationUnitType* deactunit)
{
	if (m_deactivationUnit != NULL) {
		LOG_ER("Deactivation unit already set");
		return false;
	}
	m_deactivationUnit = deactunit;
	return true;
}
const SmfActivationUnitType*
SmfForAddRemove::getDeactivationUnit(void) const
{
	return m_deactivationUnit;
}
void
SmfForAddRemove::removeSwAddRemoveDuplicates(void)
{
	TRACE_ENTER();
	std::list<SmfBundleRef> &addList = const_cast<std::list<SmfBundleRef> &>(getActivationUnit()->getSwAdd());
	std::list<SmfBundleRef> &removeList = const_cast<std::list<SmfBundleRef> &>(getDeactivationUnit()->getSwRemove());
	SmfUpgradeScope::removeSwAddRemoveDuplicates(addList, removeList);
	TRACE_LEAVE();
}
//================================================================================
// Class SmfForModify
// Purpose:
// Comments:
//================================================================================
SmfForModify::SmfForModify()
	: m_activationUnit(NULL)
{
}

SmfForModify::~SmfForModify()
{
	if (m_activationUnit != NULL) delete m_activationUnit;
	/* Delete target entities */
	std::list < SmfTargetEntityTemplate * >::iterator it;
	for (it = m_targetEntityTemplate.begin(); it != m_targetEntityTemplate.end(); ++it) {
		delete(*it);
	}
}

bool 
SmfForModify::setActivationUnit(SmfActivationUnitType* actunit)
{
	if (m_activationUnit != NULL) {
		LOG_ER("Activation unit already set");
		return false;
	}
	m_activationUnit = actunit;
	return true;
}
const SmfActivationUnitType*
SmfForModify::getActivationUnit(void) const
{
	return m_activationUnit;
}

//------------------------------------------------------------------------------
// addTargetEntityTemplate()
//------------------------------------------------------------------------------
void 
SmfForModify::addTargetEntityTemplate(SmfTargetEntityTemplate * i_entity)
{
	m_targetEntityTemplate.push_back(i_entity);
}

//------------------------------------------------------------------------------
// getTargetEntityTemplate()
//------------------------------------------------------------------------------
const std::list < SmfTargetEntityTemplate * >&
SmfForModify::getTargetEntityTemplate() const
{
	return m_targetEntityTemplate;
}

//------------------------------------------------------------------------------
// removeSwAddRemoveDuplicates()
//------------------------------------------------------------------------------
void
SmfForModify::removeSwAddRemoveDuplicates(void)
{
	TRACE_ENTER();
	std::list<SmfBundleRef> &addList = const_cast<std::list<SmfBundleRef> &>(getActivationUnit()->getSwAdd());
	std::list<SmfBundleRef> &removeList = const_cast<std::list<SmfBundleRef> &>(getActivationUnit()->getSwRemove());
	SmfUpgradeScope::removeSwAddRemoveDuplicates(addList, removeList);
	TRACE_LEAVE();
}
