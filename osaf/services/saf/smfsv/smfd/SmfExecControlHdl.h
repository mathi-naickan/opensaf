/*
 *
 * (C) Copyright 2016 The OpenSAF Foundation
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

#ifndef SMFEXECCONTROLHDL_H
#define SMFEXECCONTROLHDL_H

#include <string>
#include <vector>

#include "saAis.h"
#include "base/macros.h"
#include "saImmOm.h"
#include "SmfUtils.hh"

/*
 * Handler for the OpenSafSmfExecControl IMM object. An object of this class can
 * read all information in an exec control object and has methods for making
 * this information available.
 * Information about name of the OpenSafSmfExecControl is read from the
 * openSafSmfExecControl attribute in the OpenSafSmfConfig IMM object.
 * This handler will create and use a copy of the OpenSafSmfExecControl object.
 * The reason for this is that changes affecting the OpenSafSmfExecControl or
 * the openSafSmfExecControl attribute in the OpenSafSmfConfig IMM object shall
 * not affect an ongoing campaign.
 * This copy will be created when the install() method is called.
 * To cleanup the uninstall() method shall be used. This will delete the copy
 * of the OpenSafSmfExecControl object.
 * If a campaign shall continue after a SI-swap or reboot this SmfExecControlHdl
 * object has to be created again but the install() method shall not be used.
 * The other method can be used as long as the OpenSafSmfExecControl copy still
 * exist.
 */
class SmfExecControlObjHandler {
 public:

  SmfExecControlObjHandler();
  ~SmfExecControlObjHandler();

  /**
   * Setup the exec control handling
   *
   * @return true if success
   */
  bool install();

  /**
   * Cleanup the exec control handling and remove the exec control object copy
   * Note:
   * This is also done in the destructor except removal of the exec control
   * object copy. However, if this is not done there is no resource leek since
   * an existing copy will be overwritten.
   */
  void uninstall();

  
  /* Return corresponding values
   * The errinfo parameter will be given a value true if success or false
   * if fail
   */
  SaUint32T procExecMode(bool *errinfo);
  SaUint32T numberOfSingleSteps(bool *errinfo);
  std::vector <std::string> nodesForSingleStep(bool *errinfo);
  bool smfProtectExecControlDuringInit(bool *errinfo);


 private:
  // NOTE: Info about private methods can be found in .cc file

  bool getValuesFromImmCopy();
  bool readExecControlObject(const char* exec_ctrl_name);
  bool readOpenSafSmfConfig();
  bool copyExecControlObject();
  void removeExecControlObjectCopy();
  bool createAllImmHandles();
  void saveAttributeDescriptors();


  // For OpenSafSmfExecControl object data
  SmfImmUtils *p_immutil_object;
  const char* c_class_name = "OpenSafSmfExecControl";
  const char* c_openSafSmfExecControl = "";
  SaUint32T m_smfProtectExecControlDuringInit;
  bool m_smfProtectExecControlDuringInit_valid;
  SaUint32T m_procExecMode;
  bool m_procExecMode_valid;
  SaUint32T m_numberOfSingleSteps;
  bool m_numberOfSingleSteps_valid;
  std::vector <std::string> m_nodesForSingleStep;
  bool m_nodesForSingleStep_valid;

  
  // For OpenSafSmfExecControl object copy
  const char* c_openSafSmfExecControl_copy = "openSafSmfExecControl_copy";
  SaImmAttrValuesT_2 **m_attributes;
  SaImmAttrValuesT_2 *m_exec_ctrl_name_ad;
  SaImmAttrValuesT_2 *m_procExecMode_ad;
  SaImmAttrValuesT_2 *m_numberOfSingleSteps_ad;
  SaImmAttrValuesT_2 *m_nodesForSingleStep_ad;


  // For storing IMM handles
  const SaVersionT m_immVersion {'A', 2, 1};
  SaImmHandleT m_omHandle {0};
  SaImmAdminOwnerHandleT m_ownerHandle {0};
  SaImmCcbHandleT m_ccbHandle {0};
  SaImmAccessorHandleT m_accessorHandle {0};


  DELETE_COPY_AND_MOVE_OPERATORS(SmfExecControlObjHandler);
};

#endif /* SMFEXECCONTROLHDL_H */
