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

#ifndef SMF_SMFD_SMFIMMOPERATION_H_
#define SMF_SMFD_SMFIMMOPERATION_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>

#include "imm/saf/saImmOm.h"
#include "imm/saf/saImmOi.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

class SmfRollbackData;

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

///
/// Purpose: Holds information about an IMM attribute.
///

class SmfImmAttribute {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmAttribute();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmAttribute();

///
/// Purpose: Set the attribute name.
/// @param   i_name A reference to a std::string containing the attribute name.
/// @return  None.
///
	void setName(const std::string & i_name);

///
/// Purpose: Get the attribute name.
/// @param   None.
/// @return  A reference to a std::string containing the attribute name.
///
	const std::string & getName();

///
/// Purpose: Set the attribute type.
/// @param   i_name A reference to a std::string containing the type name.
/// @return  None.
///
	void setType(const std::string & i_type);

///
/// Purpose: Add an attribute value.
/// @param   i_name A reference to a std::string containing the attribute value.
/// @return  None.
///
	void addValue(const std::string & i_value);

///
/// Purpose: Get the attribute values.
/// @param   i_name A reference to a std::string containing the attribute value.
/// @return  A reference to a std::list <std::string> containing the attribute values.
///
	const std::list < std::string > & getValues();

	friend class SmfImmOperation;
	friend class SmfImmCreateOperation;
	friend class SmfImmModifyOperation;
	friend class SmfImmDeleteOperation;
	friend class SmfImmRTCreateOperation;
	friend class SmfImmRTUpdateOperation;

 private:
	 std::string m_name;	/* Attribute name */
	 std::string m_type;	/* Attribute type */
	 std::list < std::string > m_values;	/* Attribute values */
};

///
/// Purpose: Base class for IMM operations.
///

class SmfImmOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	virtual ~ SmfImmOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	virtual SaAisErrorT execute(SmfRollbackData* o_rollbackData = NULL);

///
/// Purpose: Rollback the operation.
/// @param   None.
/// @return  None.
///
	virtual int rollback();

///
/// Purpose: Set the IMM CCB handle to be used by the operation.
/// @param   i_ccbHandle The SaImmCcbHandleT for the IMM operation.
/// @return  None.
///
	virtual void setCcbHandle(SaImmCcbHandleT i_ccbHandle);

///
/// Purpose: Set the IMM owner handle to be used by the operation.
/// @param   i_immOwnerHandle The SaImmAdminOwnerHandleT for the IMM operation.
/// @return  None.
///
	virtual void setImmOwnerHandle(SaImmAdminOwnerHandleT i_immOwnerHandle);

///
/// Purpose: Add a value to be used in the IMM operation.
/// @param   i_value A reference to a SmfImmAttribute to be added.
/// @return  None.
///
	virtual void addValue(const SmfImmAttribute & i_value);

///
/// Purpose: Add a attribute value to be used in the IMM operation.
/// @param   i_name The name of the attribute to add.
/// @param   i_type The type of the attribute to add.
/// @param   i_value The value of the attribute to add.
/// @return  None.
///
	virtual void addAttrValue(const std::string & i_name,
                                  const std::string & i_type,
                                  const std::string & i_value);

 protected:
	 SaImmCcbHandleT m_ccbHandle;
	SaImmAdminOwnerHandleT m_immOwnerHandle;
};

///
/// Purpose: Class for creation of IMM object.
///

class SmfImmCreateOperation:public SmfImmOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmCreateOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmCreateOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute(SmfRollbackData* o_rollbackData = NULL);

///
/// Purpose: Rollback the operation.
/// @param   None.
/// @return  None.
///
	int rollback();

///
/// Purpose: Set the class name of the object to be created.
/// @param   i_name The name of the class.
/// @return  None.
///
	void setClassName(const std::string & i_name);

///
/// Purpose: Get the class name of the object to be created.
/// @param   None.
/// @return  A const std::string containing the class name.
///
	const std::string & getClassName();

///
/// Purpose: Set the parent DN for the created object.
/// @param   i_dn The parent DN.
/// @return  None.
///
	void setParentDn(const std::string & i_dn);

///
/// Purpose: Get the parent DN for the created object.
/// @param   None.
/// @return  A const std::string containing the parent DN.
///
	const std::string & getParentDn();

///
/// Purpose: Add a value for an attribute of the object to be created.
/// @param   i_value A SmfImmAttribute containing the attribute information.
/// @return  None.
///
	void addValue(const SmfImmAttribute & i_value);

///
/// Purpose: Add a attribute value to be used in the IMM operation.
/// @param   i_name The name of the attribute to add.
/// @param   i_type The type of the attribute to add.
/// @param   i_value The value of the attribute to add.
/// @return  None.
///
	void addAttrValue(const std::string & i_name,
                          const std::string & i_type,
                          const std::string & i_value);

///
/// Purpose: Get the values set for the object to create
/// @param   None.
/// @return  A list of SmfImmAttribute.
///
	const std::list < SmfImmAttribute > & getValues();

///
/// Purpose: Create the attr values structure from previously added string values.
/// @param   None.
/// @return  Bool true if successful, otherwise false.
///
	bool createAttrValues(void);

 private:
///
/// Purpose: Set the SaImmAttrValuesT_2**, pointing to the array of pointers to SaImmAttrValuesT_2.
/// @param   i_values A SaImmAttrValuesT_2** pointing to a null terminated array of SaImmAttrValuesT_2 pointers.
/// @return  None.
///
	void setAttrValues(SaImmAttrValuesT_2 ** i_values);

///
/// Purpose: Prepare rollback of the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT prepareRollback(SmfRollbackData* o_rollbackData);

        std::string m_className;	/* class name for the object to be created */
        std::string m_parentDn;	/* dn to the parent object */
        std::list < SmfImmAttribute > m_values;	/* Attribute creation values */
	SaImmAttrValuesT_2 **m_immAttrValues;	/* The array of opinters to SaImmAttrValuesT_2 structures */
};

///
/// Purpose: Class for deletion of an IMM object.
///

class SmfImmDeleteOperation:public SmfImmOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmDeleteOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmDeleteOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute(SmfRollbackData* o_rollbackData = NULL);

///
/// Purpose: Rollback the operation.
/// @param   None.
/// @return  None.
///
	int rollback();

///
/// Purpose: Set the DN for the object to be deleted.
/// @param   i_dn The DN.
/// @return  None.
///
	void setDn(const std::string & i_dn);

///
/// Purpose: Get the DN for the object to be deleted.
/// @param   None
/// @return  const std::string
///
	const std::string & getDn();

 private:

///
/// Purpose: Prepare rollback of the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT prepareRollback(SmfRollbackData* o_rollbackData);

        std::string m_dn;	/* dn to the object to delete */
};

///
/// Purpose: Class for modification of IMM object.
///

class SmfImmModifyOperation:public SmfImmOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmModifyOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmModifyOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute(SmfRollbackData* o_rollbackData = NULL);

///
/// Purpose: Rollback the operation.
/// @param   None.
/// @return  None.
///
	int rollback();

///
/// Purpose: Set the DN of the object to be modified.
/// @param   i_dn The DN of the object to be modified.
/// @return  None.
///
	void setDn(const std::string & i_dn);

///
/// Purpose: Get the DN of the object to be modified.
/// @param   None.
/// @return  std::string The DN of the object to be modified.
///
	const std::string & getDn();

///
/// Purpose: Set the RDN of the object to be modified.
/// @param   i_dn The RDN of the object to be modified.
/// @return  None.
///
	void setRdn(const std::string & i_rdn);

///
/// Purpose: Get the RDN of the object to be modified.
/// @param   None.
/// @return  std::string The RDN of the object to be modified.
///
	const std::string & getRdn();

///
/// Purpose: Set the type of modification operation.
/// @param   i_op The name of the operation (SA_IMM_ATTR_VALUES_ADD/SA_IMM_ATTR_VALUES_DELETE/SA_IMM_ATTR_VALUES_REPLACE).
/// @return  None.
///
	void setOp(const std::string & i_op);

///
/// Purpose: Create the attr modification structure from string representation.
/// @param   None.
/// @return  Bool true if successful, otherwise false.
///
	bool createAttrMods(void);

///
/// Purpose: Add a value for an attribute of the object to be modified.
/// @param   i_value A SmfImmAttribute containing the attribute information.
/// @return  None.
///
	void addValue(const SmfImmAttribute & i_value);

///
/// Purpose: Add a attribute value to be used in the IMM operation.
/// @param   i_name The name of the attribute to add.
/// @param   i_type The type of the attribute to add.
/// @param   i_value The value of the attribute to add.
/// @return  None.
///
	void addAttrValue(const std::string & i_name,
                          const std::string & i_type,
                          const std::string & i_value);

private:

///
/// Purpose: Prepare rollback of the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT prepareRollback(SmfRollbackData* o_rollbackData);

        std::string m_dn;	/* dn to the object to be modified */
        std::string m_rdn;      /* rdn is an optional attribute which may be set in the campaign taregtEntityTemplate */
        std::string m_op;	/* type of modification operation */
        std::list < SmfImmAttribute > m_values;	/* Attribute modification values */
	SaImmAttrModificationT_2 **m_immAttrMods;	/* The array of opinters to SaImmAttrModificationT_2 structures */
};

///
/// Purpose: Class for creation of run time IMM object.
///

class SmfImmRTCreateOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmRTCreateOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmRTCreateOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute();

///
/// Purpose: Set the class name of the object to be created.
/// @param   i_name The name of the class.
/// @return  None.
///
	void setClassName(const std::string & i_name);

///
/// Purpose: Set the parent DN for the created object.
/// @param   i_dn The parent DN.
/// @return  None.
///
	void setParentDn(const std::string & i_dn);

///
/// Purpose: Set the IMM OI handle.
/// @param   i_handle The SaImmOiHandleT.
/// @return  None.
///
	void setImmHandle(const SaImmOiHandleT & i_handle);

///
/// Purpose: Add a value for an attribute of the object to be created.
/// @param   i_value A SmfImmAttribute containing the attribute information.
/// @return  None.
///
	void addValue(const SmfImmAttribute & i_value);

///
/// Purpose: Create the attr values structure from previously added string values.
/// @param   None.
/// @return  Bool true if successful, otherwise false.
///
	bool createAttrValues(void);

 private:
///
/// Purpose: Set the SaImmAttrValuesT_2**, pointing to the array of pointers to SaImmAttrValuesT_2.
/// @param   i_values A SaImmAttrValuesT_2** pointing to a null terminated array of SaImmAttrValuesT_2 pointers.
/// @return  None.
///
	void setAttrValues(SaImmAttrValuesT_2 ** i_values);

        std::string    m_className;	/* class name for the object to be created */
        std::string    m_parentDn;	/* dn to the parent object */
        SaImmOiHandleT m_immHandle;     /* The IMM OI handle */
        std::list < SmfImmAttribute > m_values;	/* Attribute creation values */
	SaImmAttrValuesT_2 **m_immAttrValues;	/* The array of opinters to SaImmAttrValuesT_2 structures */
};

///
/// Purpose: Class for update of run time IMM objects.
///

class SmfImmRTUpdateOperation {
 public:

///
/// Purpose: Constructor.
/// @param   None
/// @return  None
///
	SmfImmRTUpdateOperation();

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfImmRTUpdateOperation();

///
/// Purpose: Execute the operation.
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute();

///
/// Purpose: Set the DN for the object to modify
/// @param   i_dn The object DN.
/// @return  None.
///
	void setDn(const std::string & i_dn);

///
/// Purpose: Set the type of modification operation.
/// @param   i_op The name of the operation (SA_IMM_ATTR_VALUES_ADD/SA_IMM_ATTR_VALUES_DELETE/SA_IMM_ATTR_VALUES_REPLACE).
/// @return  None.
///
	void setOp(const std::string & i_op);

///
/// Purpose: Set the IMM OI handle.
/// @param   i_handle The SaImmOiHandleT.
/// @return  None.
///
	void setImmHandle(const SaImmOiHandleT & i_handle);

///
/// Purpose: Add a value for an attribute of the object to be created.
/// @param   i_value A SmfImmAttribute containing the attribute information.
/// @return  None.
///
	void addValue(const SmfImmAttribute & i_value);

///
/// Purpose: Create the attr values structure from previously added string values.
/// @param   None.
/// @return  Bool true if successful, otherwise false.
///
	bool createAttrMods(void);

 private:
///
/// Purpose: Set the SaImmAttrValuesT_2**, pointing to the array of pointers to SaImmAttrValuesT_2.
/// @param   i_values A SaImmAttrValuesT_2** pointing to a null terminated array of SaImmAttrValuesT_2 pointers.
/// @return  None.
///
        std::string    m_dn;	        /* dn to the parent object */
        std::string    m_op;	        /* type of modification operation */
        SaImmOiHandleT m_immHandle;     /* The IMM OI handle */
        std::list < SmfImmAttribute > m_values;	/* Attribute creation values */
	SaImmAttrModificationT_2 **m_immAttrMods;  /* The array of opinters to SaImmAttrValuesT_2 structures */
};

#endif  // SMF_SMFD_SMFIMMOPERATION_H_
