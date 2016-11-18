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

#ifndef SMF_SMFD_SMFTARGETTEMPLATE_H_
#define SMF_SMFD_SMFTARGETTEMPLATE_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>

#include "smf/smfd/SmfImmOperation.h"


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

///
/// Purpose: Holds information about SMF ParentType.
///

class SmfParentType {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfParentType();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfParentType();

///
/// Purpose: Set parent DN.
/// @param   i_parentDn A std::string specifying the parent DN.
/// @return  None.
///
	void setParentDn(const std::string & i_parentDn);

///
/// Purpose: Get parent DN.
/// @param   None.
/// @return  A std::string specifying the parent DN.
///
	const std::string & getParentDn(void) const;

///
/// Purpose: Set type DN.
/// @param   i_typeDn A std::string specifying the type DN.
/// @return  None.
///
	void setTypeDn(const std::string & i_typeDn);

///
/// Purpose: Get type DN.
/// @param   None
/// @return  A std::string specifying the type DN.
///
	const std::string & getTypeDn(void) const;

 private:
	 std::string m_parentDn;	/* The parent DN */
	 std::string m_typeDn;	/* The type DN */
};

///
/// Purpose: Hold information about PlmExecEnv
///
class SmfPlmExecEnv {
public:

	SmfPlmExecEnv(){};
	~SmfPlmExecEnv(){};

	inline std::string const& getPlmExecEnviron() const {
		return m_plmExecEnviron;
	};
	inline std::string const& getClmNode() const {
		return m_clmNode;
	};
	inline void setAmfNode(std::string& i_amfNode) {
		m_amfNode = i_amfNode;
	};
	inline std::string const& getAmfNode() const {
		return m_amfNode;
	};
	inline std::string const& getPrefered() const {
		if (m_amfNode.length() > 0) return m_amfNode;
		if (m_clmNode.length() > 0) return m_clmNode;
		return m_plmExecEnviron;
	};

private:
	friend class SmfCampaignXmlParser;
	std::string m_plmExecEnviron;
	std::string m_clmNode;
	std::string m_amfNode;
};


///
/// Purpose:Holds information about SMF Bundle reference.
///

class SmfBundleRef {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfBundleRef();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfBundleRef();

///
/// Purpose: Set bundle DN.
/// @param   i_bundleDn A std::string specifying the bundle DN.
/// @return  None.
///
	void setBundleDn(const std::string & i_bundleDn);

///
/// Purpose: Get bundle DN.
/// @param   None
/// @return  A std::string specifying the bundle DN.
///
	const std::string & getBundleDn(void) const;

///
/// Purpose: Set path name prefix.
/// @param   i_prefix A std::string specifying the bundle path name prefix.
/// @return  None.
///
	void setPathNamePrefix(const std::string & i_prefix);

///
/// Purpose: Get  path name prefix.
/// @param   None
/// @return  A std::string specifying the path name prefix.
///
	const std::string & getPathNamePrefix(void) const;

///
/// Purpose: Get the list of PlmExecEnv objects.
/// @param   None
/// @return  The list.
///
	inline const std::list<SmfPlmExecEnv>& getPlmExecEnvList() const {
		return m_plmExecEnvList;
	};
  
///
/// Purpose: Add a PlmExecEnv objects.
/// @param   i_plmExecEnv A PlmExecEnv object
/// @return  None
///
	void addPlmExecEnv(const SmfPlmExecEnv& i_plmExecEnv)
                { m_plmExecEnvList.push_back(i_plmExecEnv); }

///
/// Purpose: Append the list of PlmExecEnv objects.
/// @param   i_plmExecEnvList A list of PlmExecEnv objects
/// @return  None
///
	void addPlmExecEnvList(const std::list<SmfPlmExecEnv>& i_plmExecEnvList)
                { m_plmExecEnvList.insert(m_plmExecEnvList.end(),
                                          i_plmExecEnvList.begin(),
                                          i_plmExecEnvList.end()); }

 private:
	friend class SmfCampaignXmlParser;
	std::list<SmfPlmExecEnv> m_plmExecEnvList;
	std::string m_bundleDn;	/* The bundle DN */
	std::string m_pathNamePrefix;	/* The path name prefix */
};

///
///  Purpose:  Holds information about SMF target node template.
///

class SmfTargetNodeTemplate {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfTargetNodeTemplate();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfTargetNodeTemplate();

///
/// Purpose: Set parent object DN i.e. cluster or node group
/// @param   i_objectDn A std::string specifying the cluster or node group object DN.
/// @return  None.
///
	void setObjectDn(const std::string & i_objectDn);

///
/// Purpose: Get parent object DN i.e. cluster or node group.
/// @param   None
/// @return  A std::string specifying the cluster or node group object DN.
///
	const std::string & getObjectDn(void) const;

///
/// Purpose: Add an activation unit template
/// @param   i_activationUnit A pointer to a SmfParentType.
/// @return  None.
///
	void addActivationUnitTemplate(SmfParentType * i_activationUnit);

///
/// Purpose: Get the list of  activation unit templates
/// @param   None.
/// @return  A std::list containing pointers to added SmfParentType.
///
	const std::list < SmfParentType * >&getActivationUnitTemplateList(void) const;

///
/// Purpose: Add a bundle to be removed
/// @param   i_bundle A pointer to a SmfBundleRef.
/// @return  None.
///
	void addSwRemove(SmfBundleRef * i_bundle);

///
/// Purpose: Get the list of bundles to be removed.
/// @param   None.
/// @return  A std::list containing pointers to added SmfBundleRef.
///
	const std::list < SmfBundleRef * >&getSwRemoveList(void) const;

///
/// Purpose: Add a bundle to be installed
/// @param   i_bundle A pointer to a SmfBundleRef.
/// @return  None.
///
	void addSwInstall(SmfBundleRef * i_bundle);

///
/// Purpose: Remove bundles defined in both add and remove lists
/// @param   None.
/// @return  None.
///
	void removeSwAddRemoveDuplicates();

///
/// Purpose: Get the list of bundles to be installed.
/// @param   None.
/// @return  A std::list containing pointers to added SmfBundleRef.
///
	const std::list < SmfBundleRef * >&getSwInstallList(void) const;

 private:
	 std::string m_objectDn;	/* The object DN */
	 std::list < SmfBundleRef * >m_swRemove;	/* The list of bundles to be removed */
	 std::list < SmfBundleRef * >m_swInstall;	/* The list of bundles to be installed */
	 std::list < SmfParentType * >m_activationUnitTemplate;	/* The list of activation unit templates */
};

///
///  Purpose:  Holds information about SMF target entity template.
///

class SmfTargetEntityTemplate {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfTargetEntityTemplate();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfTargetEntityTemplate();

///
/// Purpose: Set entity template
/// @param   i_entity A pointer to a SmfParentType.
/// @return  None.
///
	void setEntityTemplate(SmfParentType * i_entity);

///
/// Purpose: Get entity template
/// @param   None.
/// @return  A A pointer to a SmfParentType.
///
	const SmfParentType *getEntityTemplate(void) const;

///
/// Purpose: Add a Imm modification which should be performed for this type.
/// @param   i_op A pointer to a SmfImmModifyOperation.
/// @return  None.
///
	void addModifyOperation(SmfImmModifyOperation * i_op);

///
/// Purpose: Get the list of Imm modifications which should be performed for this type.
/// @param   None.
/// @return  A list of pointers to SmfImmModifyOperation.
///
	const std::list < SmfImmModifyOperation * >&getModifyOperationList(void) const;

 private:
	 SmfParentType * m_entityTemplate;	/* The entity template */
	 std::list < SmfImmModifyOperation * >m_modifyOperations;	/* The list of Imm modifications */
};

#endif  // SMF_SMFD_SMFTARGETTEMPLATE_H_
