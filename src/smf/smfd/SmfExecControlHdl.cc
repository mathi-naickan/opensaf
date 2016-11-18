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

#include "SmfExecControlHdl.h"

#include <string>
#include <vector>

#include "base/osaf_extended_name.h"
#include "osaf/saf/saAis.h"
#include "base/saf_error.h"
#include "osaf/immutil/immutil.h"
#include "smf/smfd/SmfUtils.h"
#include "smf/smfd/SmfCampaign.h"

// Note: Info about public methods can be found in .h file

SmfExecControlObjHandler::SmfExecControlObjHandler() :
  m_smfProtectExecControlDuringInit(0),
  m_smfProtectExecControlDuringInit_valid(false),
  m_procExecMode(0),
  m_procExecMode_valid(false),
  m_numberOfSingleSteps(0),
  m_numberOfSingleSteps_valid(false),
  m_nodesForSingleStep_valid(false),
  m_attributes(0),
  m_exec_ctrl_name_ad(0),
  m_procExecMode_ad(0),
  m_numberOfSingleSteps_ad(0),
  m_nodesForSingleStep_ad(0),
  m_omHandle(0),
  m_ownerHandle(0),
  m_ccbHandle(0)
{
  p_immutil_object = new SmfImmUtils; // Deleted by uninstall and in destructor
}

SmfExecControlObjHandler::~SmfExecControlObjHandler() {
  // Note 1:
  // The exec control object copy cannot be removed here since it may
  // be needed after restart or SI-swap. Use uninstall() to cleanup and remove
  // the exec control object copy.
  // Note 2:
  // If the exec control object copy is not removed it will remain as a "stray"
  // IMM object, however this is not a resource leek since it will be
  // overwritten the next time a copy has to be created
  //
  TRACE_ENTER();
  if (p_immutil_object != NULL) {
    delete p_immutil_object;
    p_immutil_object = NULL;
  }
  TRACE_LEAVE();
}

bool SmfExecControlObjHandler::install() {
  TRACE_ENTER();

  // Read smf configuration data needed for handling exec control
  //
  if (readOpenSafSmfConfig() == false) {
    LOG_NO("%s: readOpenSafSmfConfig Fail", __FUNCTION__);
    return false;
  }

  // Read the original exec control object and save attribute values in member
  // variables and handle valid flags.
  // 
  const char* exec_ctrl_name = c_openSafSmfExecControl;
  if (readExecControlObject(exec_ctrl_name) == false) {
    LOG_NO("%s: readExecControlObject Fail", __FUNCTION__);
    return false;
  }

  // Create a copy of the exec control object. If already exist it's overwritten
  removeExecControlObjectCopy();
  if (copyExecControlObject() == false) {
    LOG_NO("%s: copyExecControlObject Fail", __FUNCTION__);
    return false;
  }

  TRACE_LEAVE();
  return true;
}

void SmfExecControlObjHandler::uninstall() {
  TRACE_ENTER();
  if (p_immutil_object != NULL) {
    delete p_immutil_object;
    p_immutil_object = NULL;
  }
  removeExecControlObjectCopy();
  TRACE_LEAVE();
}

SaUint32T SmfExecControlObjHandler::procExecMode(bool *errinfo) {
  TRACE_ENTER();
  bool i_errinfo = true; //No error

  if (m_procExecMode_valid == false) {
    TRACE("%s has no valid value, try to read the IMM copy",__FUNCTION__);
    i_errinfo = getValuesFromImmCopy();
  }

  if (errinfo != NULL) {
    *errinfo = i_errinfo;
  }

  TRACE_LEAVE();
  return m_procExecMode;
}

SaUint32T SmfExecControlObjHandler::numberOfSingleSteps(bool *errinfo) {
  TRACE_ENTER();
  bool i_errinfo = true; //No error

  if (m_numberOfSingleSteps_valid == false) {
    TRACE("%s has no valid value, try to read the IMM copy",__FUNCTION__);
    i_errinfo = getValuesFromImmCopy();
  }

  if (errinfo != NULL) {
    *errinfo = i_errinfo;
  }

  TRACE_LEAVE();
  return m_numberOfSingleSteps;
}

std::vector <std::string> SmfExecControlObjHandler::nodesForSingleStep
  (bool *errinfo) {
  TRACE_ENTER();
  bool i_errinfo = true; //No error

  if (m_nodesForSingleStep_valid == false) {
    TRACE("%s has no valid value, try to read the IMM copy",__FUNCTION__);
    i_errinfo = getValuesFromImmCopy();
  }

  if (errinfo != NULL) {
    *errinfo = i_errinfo;
  }

  TRACE_LEAVE();
  return m_nodesForSingleStep;
}

bool SmfExecControlObjHandler::smfProtectExecControlDuringInit(
  bool *errinfo=NULL) {
  TRACE_ENTER();
  bool i_errinfo = true;

  if (m_smfProtectExecControlDuringInit_valid == false) {
    i_errinfo = readOpenSafSmfConfig();
  }

  if (errinfo != NULL) {
    *errinfo = i_errinfo;
  }

  if (m_smfProtectExecControlDuringInit > 0) {
    return true;
  } else {
    return false;
  }
}


/**
 * Read all values from the exec control object copy if one exist
 * 
 * @return
 */
bool SmfExecControlObjHandler::getValuesFromImmCopy() {
  bool errinfo = true;

  TRACE_ENTER();
    if (readExecControlObject(c_openSafSmfExecControl_copy) == false) {
      LOG_NO("%s readExecControlObject(c_openSafSmfExecControl_copy) Fail",
             __FUNCTION__);
      errinfo = false;
    }

  TRACE_LEAVE();
  return errinfo;
}

/**
 * Read the exec control object and save its values into member variables
 * The attribute descriptors are saved so that a copy can be made
 * 
 * @return false on Fail
 */
bool SmfExecControlObjHandler::readExecControlObject
  (const char* exec_ctrl_name) {
  TRACE_ENTER();

  TRACE("%s: Name of exec control object '%s'",
        __FUNCTION__, exec_ctrl_name);

  // Get all attributes of the exec control object
  if (p_immutil_object->getObject(exec_ctrl_name, &m_attributes) == false) {
    TRACE("%s: Failed to get object from attribute %s",
           __FUNCTION__, OPENSAF_SMF_EXEC_CONTROL);
    return false;
  }

  // Store all existing attributes in member variables. Ignore if not existing
  // attribute and keep variable default value
  // Values are not validated here

  saveAttributeDescriptors();

  const SaUint32T* p_procExecMode = immutil_getUint32Attr(
    (const SaImmAttrValuesT_2 **) m_attributes, "procExecMode", 0);
  if (p_procExecMode == 0) {
    LOG_NO("%s: Could not read procExecMode", __FUNCTION__);
  } else {
    m_procExecMode = *p_procExecMode;
    m_procExecMode_valid = true;
  }
  
  const SaUint32T* p_numberOfSingleSteps = immutil_getUint32Attr(
    (const SaImmAttrValuesT_2 **) m_attributes, "numberOfSingleSteps", 0);
  if (p_numberOfSingleSteps == 0) {
    LOG_NO("%s: Could not read numberOfSingleSteps", __FUNCTION__);
  } else {
    m_numberOfSingleSteps = *p_numberOfSingleSteps;
    m_numberOfSingleSteps_valid = true;
  }

  // Fill in node name vector
  SaAisErrorT ais_rc;
  SaUint32T number_of_nodes;
  ais_rc = immutil_getAttrValuesNumber(const_cast<char*> ("nodesForSingleStep"),
                                   (const SaImmAttrValuesT_2 **) m_attributes,
                                   &number_of_nodes);
  if (ais_rc != SA_AIS_OK) {
    LOG_NO("%s: No nodesForSingleStep found", __FUNCTION__);
    return true;
  }

  const char* p_node_name;
  for (unsigned int i = 0; i < number_of_nodes; i++) {
    p_node_name =immutil_getStringAttr(
        (const SaImmAttrValuesT_2**) m_attributes,
        "nodesForSingleStep", i);
    if (p_node_name == NULL) {
      break;
    }
    m_nodesForSingleStep.push_back(p_node_name);
    m_nodesForSingleStep_valid = true;
  }

  TRACE_LEAVE();
  return true;
}
/**
 * Read the smf config object and save openSafSmfExecControl and
 * smfProtectExecControlDuringInit
 *
 * @return false on fail
 */
bool SmfExecControlObjHandler::readOpenSafSmfConfig() {
  // Get the name of the exec control object
  if (p_immutil_object->getObject(SMF_CONFIG_OBJECT_DN, &m_attributes) == false) {
    LOG_WA("%s: Could not get SMF config object from IMM %s",
           __FUNCTION__, SMF_CONFIG_OBJECT_DN);
    return "";
  }

  const char* exec_ctrl_name = immutil_getStringAttr(
    (const SaImmAttrValuesT_2 **) m_attributes, OPENSAF_SMF_EXEC_CONTROL, 0);
  if (exec_ctrl_name == NULL || strcmp(exec_ctrl_name, "") == 0) {
    LOG_WA("%s: Could not get %s attrs from SmfConfig",
           __FUNCTION__, OPENSAF_SMF_EXEC_CONTROL);
    return "";
  }
  c_openSafSmfExecControl = exec_ctrl_name;

  const SaUint32T* p_protect_flag = immutil_getUint32Attr(
    (const SaImmAttrValuesT_2 **) m_attributes,
    "smfProtectExecControlDuringInit", 0);
  if (p_protect_flag == NULL) {
    LOG_ER("could not read smfProtectExecControlDuringInit");
    return false;
  }
  m_smfProtectExecControlDuringInit = *p_protect_flag;
  m_smfProtectExecControlDuringInit_valid = true;

  return true;
}

/**
 * Delete the exec control object copy if exist
 */
void SmfExecControlObjHandler::removeExecControlObjectCopy() {
  SaAisErrorT ais_rc = SA_AIS_OK;

  TRACE_ENTER();
  SaNameT node_name;
  osaf_extended_name_lend(c_openSafSmfExecControl_copy,
          &node_name);

  TRACE("Deleting object '%s'", c_openSafSmfExecControl_copy);

  if (createImmOmHandles() == false) {
    TRACE("createCcbHandle Fail");
  }

  const SaNameT *names[2];
  names[0] = &node_name;
  names[1] = NULL;
  ais_rc = immutil_saImmOmAdminOwnerSet(m_ownerHandle, names, SA_IMM_ONE);

  if (ais_rc != SA_AIS_OK) {
    LOG_NO("%s - saImmOmAdminOwnerSet FAILED: %s",
           __FUNCTION__, saf_error(ais_rc));
  }

  ais_rc = immutil_saImmOmCcbObjectDelete(m_ccbHandle,
          &node_name);
  if (ais_rc != SA_AIS_OK) {
          LOG_NO("%s: saImmOmCcbObjectDelete '%s' Fail %s",
           __FUNCTION__, c_openSafSmfExecControl_copy,
           saf_error(ais_rc));
  } else {
          ais_rc = saImmOmCcbApply(m_ccbHandle);
          if (ais_rc != SA_AIS_OK) {
          LOG_NO("%s: saImmOmCcbApply() Fail '%s'",
                  __FUNCTION__, saf_error(ais_rc));
          }
  }

  finalizeImmOmHandles();

  TRACE_LEAVE();
}

/**
 * Create a copy of the exec control object. The exec control object must have
 * been read before a copy can be made.
 *
 * @return false on fail
 */
bool SmfExecControlObjHandler::copyExecControlObject() {
  bool rc = true;

  TRACE_ENTER();

  SaAisErrorT ais_rc = SA_AIS_OK;

  // ------------------------------------
  // Create handles needed
  //
  if (createImmOmHandles() == false) {
    TRACE("createCcbHandle Fail");
  }

  // ------------------------------------
  // A new attribute descriptor for the node
  // name has to be created
  // Attribute: openSafSmfExecControl
  //
  SaImmAttrValuesT_2 smfExecControl_copy_ad;
  smfExecControl_copy_ad.attrName =
      const_cast<SaImmAttrNameT> ("openSafSmfExecControl");
  smfExecControl_copy_ad.attrValueType = SA_IMM_ATTR_SASTRINGT;
  char *object_name = const_cast<char *> (c_openSafSmfExecControl_copy);
  smfExecControl_copy_ad.attrValuesNumber = 1;
  SaImmAttrValueT object_names[] = {&object_name};
  smfExecControl_copy_ad.attrValues = object_names;


  // ------------------------------------
  // NULL terminated array of pointers to the list of attributes.
  // In this case only one attribute, saAmfNGNodeListAttr
  const SaImmAttrValuesT_2 *attrValues[] = {
    &smfExecControl_copy_ad,
    m_procExecMode_ad,
    m_numberOfSingleSteps_ad,
    m_nodesForSingleStep_ad,
    NULL
  };

  // ---------------------------------------
  // Create the node group. Top level object
  SaImmClassNameT className = const_cast<SaImmClassNameT> (c_class_name);
  ais_rc = immutil_saImmOmCcbObjectCreate_2(
              m_ccbHandle,
              className,
              NULL,
              attrValues);

  if (ais_rc != SA_AIS_OK) {
    LOG_NO("%s: saImmOmCcbObjectCreate_2() '%s' Fail %s",
           __FUNCTION__, object_name, saf_error(ais_rc));
    rc = false;
  } else {
    ais_rc = saImmOmCcbApply(m_ccbHandle);
    if (ais_rc != SA_AIS_OK) {
      LOG_NO("%s: saImmOmCcbApply() Fail '%s'",
             __FUNCTION__, saf_error(ais_rc));
      rc = false;
    }
  }

  finalizeImmOmHandles();
  TRACE_LEAVE();
  return rc;
}

/**
 * Save attribute descriptors for all attributes needed to create a copy
 * of the exec control object
 */
void SmfExecControlObjHandler::saveAttributeDescriptors() {
  TRACE_ENTER();
  for (int i = 0; m_attributes[i] != NULL; i++) {
    if (strcmp(m_attributes[i]->attrName, "openSafSmfExecControl") == 0) {
      m_exec_ctrl_name_ad = m_attributes[i];
    } else if (strcmp(m_attributes[i]->attrName, "procExecMode") == 0) {
      m_procExecMode_ad = m_attributes[i];
    } else if (strcmp(m_attributes[i]->attrName, "numberOfSingleSteps") == 0) {
      m_numberOfSingleSteps_ad = m_attributes[i];
    } else if (strcmp(m_attributes[i]->attrName, "nodesForSingleStep") == 0) {
      m_nodesForSingleStep_ad = m_attributes[i];
    }
  }
  TRACE_LEAVE();
}
/**
 * Get all needed IMM handles and store them in member variables
 * NOTE: This is a copy of a method in the SmfAdminOperation class.
 *       This should be fixed
 * 
 * @return false on Fail
 */
bool SmfExecControlObjHandler::createImmOmHandles() {
  SaAisErrorT ais_rc = SA_AIS_ERR_TRY_AGAIN;
  int timeout_try_cnt = 6;
  bool rc = true;

  TRACE_ENTER();

  finalizeImmOmHandles();

  // OM handle
  while (timeout_try_cnt > 0) {
    ais_rc = immutil_saImmOmInitialize(&m_omHandle, NULL,
                                       &m_immVersion);
    if (ais_rc != SA_AIS_ERR_TIMEOUT)
      break;
    timeout_try_cnt--;
  }
  if (ais_rc != SA_AIS_OK) {
    LOG_NO("%s: saImmOmInitialize Fail %s", __FUNCTION__,
           saf_error(ais_rc));
    rc = false;
  }

  // Admin owner handle
  if (rc == true) {
    timeout_try_cnt = 6;
    while (timeout_try_cnt > 0) {
      ais_rc = immutil_saImmOmAdminOwnerInitialize(m_omHandle,
                                                   const_cast<char*>
                                                   ("SmfExecControlObjHandlerOwner"),
                                                   SA_TRUE, &m_ownerHandle);
      if (ais_rc != SA_AIS_ERR_TIMEOUT)
        break;
      timeout_try_cnt--;
    }
    if (ais_rc != SA_AIS_OK) {
      LOG_NO("%s: saImmOmAdminOwnerInitialize Fail %s",
             __FUNCTION__, saf_error(ais_rc));
      rc = false;
    }
  }

  // CCB handle
  if (rc == true) {
    timeout_try_cnt = 6;
    while (timeout_try_cnt > 0) {
      ais_rc = immutil_saImmOmCcbInitialize(m_ownerHandle,
                                            0, &m_ccbHandle);
      if (ais_rc != SA_AIS_ERR_TIMEOUT)
        break;
      timeout_try_cnt--;
    }
    if (ais_rc != SA_AIS_OK) {
      LOG_NO("%s: saImmOmCcbInitialize Fail %s",
             __FUNCTION__, saf_error(ais_rc));
      rc = false;
    }
  }

  TRACE_LEAVE();
  return rc;
}

void SmfExecControlObjHandler::finalizeImmOmHandles() {
  if (m_omHandle != 0) {
    SaAisErrorT ais_rc = immutil_saImmOmFinalize(m_omHandle);
    if (ais_rc != SA_AIS_OK) {
      LOG_NO("%s: immutil_saImmOmFinalize Fail %s",
             __FUNCTION__, saf_error(ais_rc));
    }
  }

  m_omHandle = 0;
  m_ownerHandle = 0;
  m_ccbHandle = 0;
}
