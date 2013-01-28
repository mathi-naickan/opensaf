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

#ifndef SMFROLLBACK_HH
#define SMFROLLBACK_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <string>
#include <list>

#include <saImm.h>
#include <saImmOm.h>
#include <saImmOi.h>

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */
class SmfRollbackCcb;
class SmfRollbackData;
class SmfImmOperation;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
SaAisErrorT smfCreateRollbackElement(const std::string & i_dn, SaImmOiHandleT i_oiHandle);

///
/// Purpose: Holds information about rollback data.
///

class SmfRollbackData {
public:

///
/// Purpose: Constructor.
/// @param   i_ccb A pointer to the rollback CCB this Data is part of
/// @return  None
///
	SmfRollbackData(SmfRollbackCcb* i_ccb);

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfRollbackData();

///
/// Purpose: Set the rollback type.
/// @param   i_tape A reference to a std::string containing the rollback type.
/// @return  None.
///
        void setType(const std::string & i_type);

///
/// Purpose: Set the rollback dn.
/// @param   i_dn A reference to a std::string containing the rollback dn.
/// @return  None.
///
        void setDn(const std::string & i_dn);

///
/// Purpose: Set the rollback class.
/// @param   i_class A reference to a std::string containing the rollback class.
/// @return  None.
///
        void setClass(const std::string & i_class);

///
/// Purpose: Add a rollback attribute value.
/// @param   i_attrName A reference to a std::string containing the rollback attribute name.
/// @param   i_attrType A reference to a std::string containing the rollback attribute type.
/// @param   i_attrValue A reference to a std::string containing the rollback attribute value.
/// @return  None.
///
        void addAttrValue(const std::string & i_attrName,
                          const std::string & i_attrType,
                          const std::string & i_attrValue);

///
/// Purpose: Execute this rollback data. (Store it in the IMM)
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute(SaImmOiHandleT i_oiHandle);

///
/// Purpose: Rollback this rollback data. (Read it from the IMM and execute it)
/// @param   None.
/// @return  None.
///
	SaAisErrorT rollback(const std::string& i_dn, std::list < SmfImmOperation * >& io_operationList);

///
/// Purpose: Set unique id for this data
/// @param   None.
/// @return  None.
///
	void setId(unsigned int i_id) {m_id = i_id;}

private:

///
/// Purpose: Rollback a create operation
/// @param   None.
/// @return  None.
///
        SaAisErrorT rollbackCreateOperation(const SaImmAttrValuesT_2 ** attributes, 
                                            std::list < SmfImmOperation * >& io_operationList);

///
/// Purpose: Rollback a delete operation
/// @param   None.
/// @return  None.
///
        SaAisErrorT rollbackDeleteOperation(const SaImmAttrValuesT_2 ** attributes, 
                                            std::list < SmfImmOperation * >& io_operationList);

///
/// Purpose: Rollback a modify replace operation
/// @param   None.
/// @return  None.
///
        SaAisErrorT rollbackModifyOperation(const SaImmAttrValuesT_2 ** attributes, 
                                            std::list < SmfImmOperation * >& io_operationList);


        std::string m_type;	                /* rollback type */
        std::string m_dn;	                /* dn to manipulated object or parent in case of create */
        std::string m_class;	                /* class name of object */
        std::list < std::string > m_attrValues;	/* Attribute values */
        SmfRollbackCcb* m_ccb;
        unsigned int m_id;
};

///
/// Purpose: .
///

class SmfRollbackCcb {
public:

///
/// Purpose: Constructor.
/// @param   i_dn An IMM dn to where this rollback CCB should be stored/read
/// @return  None
///
	SmfRollbackCcb(const std::string& i_dn, SaImmOiHandleT i_oiHandle);

///
/// Purpose: Destructor.
/// @param   None
/// @return  None
///
	~SmfRollbackCcb();

///
/// Purpose: Execute this rollback CCB. (Store it in the IMM)
/// @param   None.
/// @return  None.
///
	SaAisErrorT execute();

///
/// Purpose: Rollback this rollback CCB. (Read it from the IMM and execute it)
/// @param   None.
/// @return  None.
///
	SaAisErrorT rollback();

///
/// Purpose: Get the DN for this rollback CCB.
/// @param   None.
/// @return  None.
///
	const std::string& getDn() {return m_dn;}

///
/// Purpose: Add a rollback data to this rollback CCB.
/// @param   i_data a pointer a rollback data object.
/// @return  None.
///
	void addCcbData(SmfRollbackData* i_data);

protected:
        std::string                    m_dn;             /* dn to where this rollback CCB should be stored/read */
	std::list < SmfRollbackData* > m_rollbackData;	 /* Rollback data for this CCB */
        unsigned int                   m_dataId;         /* sequence id for containing data objects */
        SaImmOiHandleT                 m_oiHandle;
};

#endif
