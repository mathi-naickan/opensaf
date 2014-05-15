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

#ifndef SMFUTILS_HH
#define SMFUTILS_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>
#include <saImmOm.h>
#include <saAis.h>
#include <saflog.h>

#include "smfd.h"
#include "smfd_smfnd.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

class SmfImmOperation;
class SmfRollbackCcb;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */


/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
#ifdef __cplusplus
extern "C" {
#endif

	extern bool smf_stringToImmType(char *i_type, SaImmValueTypeT& o_type);
        extern const char* smf_immTypeToString(SaImmValueTypeT i_type);
	extern SaImmAttrModificationTypeT smf_stringToImmAttrModType(char
								     *i_type);
	extern bool smf_stringsToValues(SaImmAttrValuesT_2 * i_attribute, std::list < std::string >& i_values);
        extern bool smf_stringToValue(SaImmValueTypeT i_type, SaImmAttrValueT *i_value, const char* i_str);
        extern std::string smf_valueToString(SaImmAttrValueT value, SaImmValueTypeT type);
        extern int  smf_opStringToInt(const char* i_str);
        extern int  smf_system(std::string i_cmd);
	extern void updateSaflog(const std::string& i_dn, const uint32_t& i_stateId, const uint32_t& i_newState, const uint32_t& i_oldState);
	extern const std::string smfStateToString(const uint32_t& i_stateId, const uint32_t& i_state);

#ifdef __cplusplus
}
#endif
extern bool waitForNodeDestination(const std::string & i_node, SmfndNodeDest* o_nodeDest);
extern bool getNodeDestination(const std::string & i_node, SmfndNodeDest* o_nodeDest);
extern std::string replaceAllCopy(const std::string& i_haystack, const  std::string& i_needle, const  std::string& i_replacement);

///
/// IMM utility functions used by SMF
///

class SmfImmUtils {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfImmUtils();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfImmUtils();

///
/// Purpose: Get all attributes for an IMM object.
/// @param   i_dn DN of the object to get.
/// @param   o_attributes Resulting attribute values
/// @return  True if successful, otherwise false
///
        bool getClassDescription(const std::string & i_className, SaImmAttrDefinitionT_2 *** o_attributeDefs);

///
/// Purpose: Delete attributeDefs struct received from IMM in call getClassDescription
/// @param   i_attributes Structure to delete
/// @return  True if successful, otherwise false
///
        bool classDescriptionMemoryFree(SaImmAttrDefinitionT_2 ** i_attributeDefs);

///
/// Purpose: Get the class name for an IMM object.
/// @param   i_dn DN of the object to read the class name from.
/// @param   o_className Resulting class name
/// @return  True if successful, otherwise false
///
        bool getClassNameForObject(const std::string& i_dn, std::string& o_className);

///
/// Purpose: Get all attributes for an IMM object.
/// @param   i_dn DN of the object to get.
/// @param   o_attributes Resulting attribute values
/// @return  True if successful, otherwise false
///
	bool getObject(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes);

///
/// Purpose: Get all attributes for an IMM object.
/// @param   i_dn DN of the object to get.
/// @param   o_attributes Resulting attribute values
/// @return  SaAisErrorT
///
	SaAisErrorT getObjectAisRC(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes);

///
/// Purpose: Get all attributes for the parent to an IMM object.
/// @param   i_dn DN of the children to the parent object to get.
/// @param   o_attributes Resulting attribute values
/// @return  True if successful, otherwise false
///
	bool getParentObject(const std::string & i_dn, SaImmAttrValuesT_2 *** o_attributes);

///
/// Purpose: Get children objects to a parent.
/// @param   i_dn DN of the parent object.
/// @param   o_childList Result list of childrens DN
/// @param   i_className Get children only of this type. NULL = all children.
/// @return  True if successful, otherwise false
///
	bool getChildren(const std::string & i_dn, std::list < std::string > &o_childList, SaImmScopeT i_scope =
			 SA_IMM_SUBLEVEL, const char *i_className = NULL);

///
/// Purpose: Get children objects to a parent. The caller of this method is resposible to finalize the search handle in case of success.
/// @param   i_dn DN of the parent object.
/// @param   io_immSearchHandle The resulting search handle
/// @param   i_scope, The scope of the search
/// @param   i_className Get children only of this type. NULL = all children.
/// @return  True if successful, otherwise false
///
	bool getChildrenAndAttrBySearchHandle(const std::string& i_dn, 
					      SaImmSearchHandleT& io_immSearchHandle, 
					      SaImmScopeT i_scope,
					      SaImmAttrNameT* i_attrNames,
					      const char *i_className = NULL);

///
/// Purpose: Call administrative operation on an object.
/// @param   i_dn DN of object to call.
/// @param   i_operationId The operation id.
/// @param   i_params Parameter values or NULL.
/// @param   i_timeout Max time it may take
/// @return  True if successful, otherwise false
///
	SaAisErrorT callAdminOperation(const std::string & i_dn, unsigned int i_operationId,
				       const SaImmAdminOperationParamsT_2 ** i_params = NULL, SaTimeT i_timeout =
				       SA_TIME_ONE_MINUTE);

///
/// Purpose: Call administrative operation on an object.
/// @param   i_immOperationList A list of Imm operations to be done.
/// @return  True if successful, otherwise false
///
	SaAisErrorT doImmOperations(std::list < SmfImmOperation * >&i_immOperationList, SmfRollbackCcb* io_rollbackCcb = NULL);

 private:
	bool initialize(void);
	bool finalize(void);

	static SaVersionT s_immVersion;

	SaImmHandleT m_omHandle;
	SaImmAdminOwnerHandleT m_ownerHandle;
	SaImmAccessorHandleT m_accessorHandle;
};

#endif				// SMFUTILS_HH
