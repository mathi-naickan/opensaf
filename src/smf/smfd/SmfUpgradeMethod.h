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

#ifndef SMF_SMFD_SMFUPGRADEMETHOD_H_
#define SMF_SMFD_SMFUPGRADEMETHOD_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <vector>
#include <set>
#include "osaf/saf/saAis.h"
#include "smf/saf/saSmf.h"
#include "smf/smfd/SmfTargetTemplate.h"
#include "smf/smfd/SmfCallback.h"

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

class SmfUpgradeScope;
class SmfCallback;

///
/// Purpose: Base class for the upgrade method, must be specialized
///

class SmfUpgradeMethod {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfUpgradeMethod();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	virtual ~ SmfUpgradeMethod();

///
/// Purpose: Get the upgrade method (Rolling or Single Step).
/// @param   None.
/// @return  A SaSmfUpgrMethodT specifying the upgrade method.
///
	virtual SaSmfUpgrMethodT getUpgradeMethod(void) const = 0;

///
/// Purpose: Set the upgrade scope.
/// @param   i_scope A pointer to a SmfUpgradeScope object.
/// @return  None.
///
	virtual void setUpgradeScope(SmfUpgradeScope * i_scope);

///
/// Purpose: Get the upgrade scope.
/// @param   None.
/// @return  A pointer to a SmfUpgradeScope object.
///
	virtual const SmfUpgradeScope *getUpgradeScope(void) const;

///
/// Purpose: Set the StepRestartOption.
/// @param   i_opt A SaUint32T containing the restart option.
/// @return  None.
///
	void setStepRestartOption(const SaUint32T i_opt);

///
/// Purpose: Get the StepRestartOption.
/// @param   None.
/// @return  A SaUint32T containing the restart option.
///
	SaUint32T getStepRestartOption(void) const;

///
/// Purpose: Set the number of step retries allowed before the steps in this procedure has failed..
/// @param   i_opt A SaUint32T containing the max number of retries.
/// @return  None.
///
	void setStepMaxRetryCount(const SaUint32T i_count);

///
/// Purpose: Get the number of step retries allowed before the steps in this procedure has failed..
/// @param   None.
/// @return  A SaUint32T containing the max number of retries.
///
	SaUint32T getStepMaxRetryCount(void) const;

///
/// Purpose: Get the list of Callbacks
/// @return  The list of Callbacks
///
	std::list<SmfCallback *>& getCallbackList();

///
/// Purpose: Add a Callback objects to the list of Callbacks.
/// @return  The list of Callbacks
///
	void addCallback(SmfCallback *callback);

protected:

	SmfUpgradeScope * m_scope;
	SaUint32T m_stepRestartOption;
	SaUint32T m_stepMaxRetryCount;
	std::list <SmfCallback *> callback_list;

};

///
/// Purpose: Class for RollingUpgrade (specialization of base class SmfUpgradeMethod)
///

class SmfRollingUpgrade:public SmfUpgradeMethod {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfRollingUpgrade();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	~SmfRollingUpgrade();

///
/// Purpose: Get the upgrade method (Rolling or Single Step).
/// @param   None.
/// @return  A SaSmfUpgrMethodT specifying the upgrade method.
///
	SaSmfUpgrMethodT getUpgradeMethod(void) const;

};

///
/// Purpose: Class for Single Step Upgrade (specialization of base class SmfUpgradeMethod)
///

class SmfSinglestepUpgrade:public SmfUpgradeMethod {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfSinglestepUpgrade();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	~SmfSinglestepUpgrade();

///
/// Purpose: Get the upgrade method (Rolling or Single Step).
/// @param   None.
/// @return  A SaSmfUpgrMethodT specifying the upgrade method.
///
	SaSmfUpgrMethodT getUpgradeMethod(void) const;

};

///
/// Purpose: Base class for upgrade scope.
///

class SmfUpgradeScope {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfUpgradeScope();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	virtual ~SmfUpgradeScope();

 protected:

///
/// Purpose: Remove duplicates of SwAdd and SwRemove.
/// @param   io_addList: addList of SmfBundleRef objects
/// @param   io_removeList: removeList of SmfBundleRef objects
/// @return  None.
///
	void removeSwAddRemoveDuplicates(std::list<SmfBundleRef> &io_addList, std::list<SmfBundleRef> &io_removeList);

///
/// Purpose: Remove duplicates of SwAdd and SwRemove.
///          Based on the given DN, the specified swBundleRef objects will be deleted from addList/removeList
/// @param   io_addOrRemoveList: addList or removeList
/// @param   i_dn: DN
/// @return  None.
///
	void removeSwAddRemoveDuplicate(std::list<SmfBundleRef> &io_addOrRemoveList, const std::string &i_dn);

};

///
/// Purpose: Upgrade scope by template (Specialization of base class SmfUpgradeScope).
///

class SmfByTemplate:public SmfUpgradeScope {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfByTemplate();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	~SmfByTemplate();

///
/// Purpose: Set the target node template.
/// @param   i_entity A pointer to a SmfTargetNodeTemplate object.
/// @return  None.
///
	void setTargetNodeTemplate(SmfTargetNodeTemplate * i_entity);

///
/// Purpose: Get the target node template.
/// @param   None.
/// @return  A pointer to a SmfTargetNodeTemplate object.
///
	const SmfTargetNodeTemplate *getTargetNodeTemplate(void) const;

///
/// Purpose: Add a target entity template.
/// @param   i_entity A pointer to a SmfTargetEntityTemplate object.
/// @return  None.
///
	void addTargetEntityTemplate(SmfTargetEntityTemplate * i_entity);

///
/// Purpose: Get target entity template list.
/// @param   None.
/// @return  A std::list of pointers to SmfTargetEntityTemplate objecs.
///
	const std::list < SmfTargetEntityTemplate * >&getTargetEntityTemplate() const;

 private:
	 SmfTargetNodeTemplate * m_targetNodeTemplate;	/* The target node templates */
	 std::list < SmfTargetEntityTemplate * >m_targetEntityTemplate;	/* The list of target entity templates */

};

///
/// Purpose: Holds information about an Entity.
///   This is a part of a Single-Step upgrade.
///
class SmfEntity {
public:
	SmfEntity(){};
	~SmfEntity(){};

	inline std::string const getName() const {
		return m_name;
	};

	inline std::string const getParent() const {
		return m_parent;
	};

	inline std::string const getType() const {
		return m_type;
	};

private:
	friend class SmfCampaignXmlParser;
	std::string m_name;
	std::string m_parent;
	std::string m_type;
};

///
/// Purpose: Holds information about an Activation Unit. This type is also used for
///   Deactivation Unit. This is a part of a Single-Step upgrade.
///
class SmfActivationUnitType {
public:
	SmfActivationUnitType(){};
	~SmfActivationUnitType(){};

	inline std::list<SmfEntity> const& getActedOn() const {
		return m_actedOn;
	};
	inline std::list<SmfEntity> const& getRemoved() const {
		return m_removed;
	};
	inline std::list<SmfImmCreateOperation> const& getAdded() const {
		return m_added;
	};
	inline std::list<SmfBundleRef> const& getSwRemove() const {
		return m_swRemove;
	};
	inline std::list<SmfBundleRef> const& getSwAdd() const {
		return m_swAdd;
	};

private:
	friend class SmfCampaignXmlParser;
	std::list<SmfEntity> m_actedOn;
	std::list<SmfEntity> m_removed;
	std::list<SmfImmCreateOperation> m_added;
	std::list<SmfBundleRef> m_swRemove;
	std::list<SmfBundleRef> m_swAdd;
};


///
/// Purpose: Upgrade scope for add/remove (Specialization of base class SmfUpgradeScope).
///   This is a part of a Single-Step upgrade.
///

class SmfForAddRemove : public SmfUpgradeScope {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfForAddRemove();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	~SmfForAddRemove();

///
/// Purpose: Set the activationUnitType.
/// @param   actunit The ActivationUnitType object. It must be heap allocated
///   and will be deleted when this object is deleted.
/// @return  Bool true if successful, otherwise false.
///
	bool setActivationUnit(SmfActivationUnitType* actunit);

///
/// Purpose: Get the activationUnitType.
/// @param   None.
/// @return  A pointer to a SmfActivationUnitType object.
///
	const SmfActivationUnitType *getActivationUnit(void) const;

///
/// Purpose: Set thede activationUnitType.
/// @param   deactunit The DeactivationUnitType object. It must be heap allocated
///   and will be deleted when this object is deleted.
/// @return  Bool true if successful, otherwise false.
///
	bool setDeactivationUnit(SmfActivationUnitType* deactunit);

///
/// Purpose: Get the deactivationUnitType.
/// @param   None.
/// @return  A pointer to a SmfActivationUnitType object.
///
	const SmfActivationUnitType *getDeactivationUnit(void) const;

///
/// Purpose: Remove duplicates of SwAdd and SwRemove.
/// @param   None.
/// @return  None.
///
	void removeSwAddRemoveDuplicates(void);

 private:

	SmfActivationUnitType* m_activationUnit;
	SmfActivationUnitType* m_deactivationUnit;

};

///
/// Purpose: Upgrade scope for Modify (Specialization of base class SmfUpgradeScope).
///   This is a part of a Single-Step upgrade.
///

class SmfForModify : public SmfUpgradeScope {
 public:

///
/// The constructor
/// @param    None.
/// @return   None.
///
	SmfForModify();

///
/// The destructor
/// @param    None.
/// @return   None.
///
	~SmfForModify();

///
/// Purpose: Add a target entity template.
/// @param   i_entity A pointer to a SmfTargetEntityTemplate object.
/// @return  None.
///
	void addTargetEntityTemplate(SmfTargetEntityTemplate * i_entity);

///
/// Purpose: Get target entity template list.
/// @param   None.
/// @return  A std::list of pointers to SmfTargetEntityTemplate objecs.
///
	const std::list < SmfTargetEntityTemplate * >&getTargetEntityTemplate() const;

///
/// Purpose: Set the activationUnit.
/// @param   actunit The ActivationUnitType object. It must be heap allocated
///   and will be deleted when this object is deleted.
/// @return  Bool true if successful, otherwise false.
///
	bool setActivationUnit(SmfActivationUnitType* actunit);

///
/// Purpose: Get the activationUnit.
/// @param   None.
/// @return  A pointer to a SmfActivationUnitType object.
///
	const SmfActivationUnitType *getActivationUnit(void) const;

///
/// Purpose: Remove duplicates of SwAdd and SwRemove.
/// @param   None.
/// @return  None.
///
	void removeSwAddRemoveDuplicates(void);

 private:

	/* The list of target entity templates */
	std::list < SmfTargetEntityTemplate * >m_targetEntityTemplate;	
	SmfActivationUnitType* m_activationUnit;
};

#endif  // SMF_SMFD_SMFUPGRADEMETHOD_H_
