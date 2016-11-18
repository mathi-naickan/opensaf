/*      -*- OpenSAF  -*-
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
 * Author(s): Genband
 *
 */

/*****************************************************************************
 *   FILE NAME: plms_virt.cc
 *
 *   DESCRIPTION: C++ implementation of interface to libvirt
 *
 ****************************************************************************/

#include <libvirt/virterror.h>
#include <cstring>
#include <set>
#include "osaf/immutil/immutil.h"
#include "plm/plms_virt.h"
#include "plm/plms.h"

typedef std::map<SaNameT /*config*/, PlmsVmConfig *, ltSaNameT> PlmsVmConfigMap;
typedef std::map<SaNameT /*config*/, PlmsVmmConfig *, ltSaNameT>
  PlmsVmmConfigMap;

typedef std::map<SaNameT /*SaPlmEE*/, PlmsVmm *, ltSaNameT> PlmsVmmMap;
typedef std::set<SaNameT /*SaPlmEE*/, ltSaNameT> SavedEEList;

static PlmsVmConfigMap  plmsVmConfigMap;
static PlmsVmmConfigMap plmsVmmConfigMap;

static PlmsVmmMap plmsVmmMap;
static SavedEEList savedEEList;

static PLMS_ENTITY *
getEE(const SaNameT& eeName) {
  PLMS_ENTITY *ee(reinterpret_cast<PLMS_ENTITY *>(
    ncs_patricia_tree_get(&plms_cb->entity_info,
                          reinterpret_cast<const SaUint8T *>(&eeName))));
  return ee;
}

static PLMS_ENTITY *
getEE(const std::string& name) {
  SaNameT eeName = { 0, 0 };
  eeName.length = name.length();
  memcpy(eeName.value, name.c_str(), eeName.length);

  return getEE(eeName);
}

static bool
check_ee_presence_state(const PLMS_ENTITY& ee) {
  bool status(true);

  if (ee.entity.ee_entity.saPlmEEPresenceState !=
        SA_PLM_EE_PRESENCE_UNINSTANTIATED) {
    status = false;
  }

  return status;
}

static bool
isInCcb(SaImmOiCcbIdT ccbId, const SaNameT& attr_value) {
  CcbUtilOperationData_t *opdata(0);

  while ((opdata = ccbutil_getNextCcbOp(ccbId, opdata))) {
    if (opdata->operationType == CCBUTIL_CREATE &&
        memcmp(&opdata->objectName, &attr_value, sizeof(SaNameT)) == 0) {
      break;
    }
  }

  return opdata;
}

static void
find_hypervisor_ee(const SaNameT& vm, SaNameT *hypervisor) {
  // given a vm EE, locate the hypervisor EE
  const char *vmmBegin(static_cast<const char *>(memchr(vm.value,
                                                        ',',
                                                        vm.length)) + 1);

  memset(hypervisor, 0, sizeof(SaNameT));

  memcpy(hypervisor->value,
         vmmBegin,
         hypervisor->length = vm.length -
           (vmmBegin - reinterpret_cast<const char *>(vm.value)));
}

extern "C" void
plms_ee_vm_save(const SaNameT *ee) {
  TRACE("adding %s to savedEEList", ee->value);
  std::pair<SavedEEList::iterator, bool> p(savedEEList.insert(*ee));
}

extern "C" void
plms_create_vmm_obj(const SaNameT *name, SaImmAttrValuesT_2 **attrs) {
  const SaNameT *saPlmEE(0);
  const char *session(0);

  TRACE_ENTER();

  for (int i(0); attrs[i]; i++) {
    if (!strcmp(attrs[i]->attrName, "saPlmEE"))
      saPlmEE = static_cast<SaNameT *>(*attrs[i]->attrValues);
    else if (!strcmp(attrs[i]->attrName, "libVirtSession"))
      session = *static_cast<SaStringT *>(*attrs[i]->attrValues);
  }

  TRACE("saPlmEE: %s session: %s", saPlmEE->value, session);

  PlmsVmmMap::iterator it(plmsVmmMap.find(*saPlmEE));

  if (it == plmsVmmMap.end()) {
    TRACE("creating new virtual machine monitor");
    PlmsVmm *vmm(new PlmsVmm(session));

    std::pair<PlmsVmmMap::iterator, bool> p(
      plmsVmmMap.insert(std::make_pair(*saPlmEE, vmm)));
  }

  // add to the config map
  PlmsVmmConfigMap::iterator cfgIt(plmsVmmConfigMap.find(*name));

  if (cfgIt == plmsVmmConfigMap.end()) {
    TRACE("creating new virtual machine monitor config");

    PlmsVmmConfig *vmmConfig(new PlmsVmmConfig(reinterpret_cast<const char *>
      (saPlmEE->value), session));

    std::pair<PlmsVmmConfigMap::iterator, bool> p(
      plmsVmmConfigMap.insert(std::make_pair(*name, vmmConfig)));
  }

  TRACE_LEAVE();
}

extern "C" void
plms_create_vm_obj(const SaNameT *name, SaImmAttrValuesT_2 **attrs) {
  const SaNameT *saPlmEE(0);
  const char *domainName(0);

  TRACE_ENTER();

  for (int i(0); attrs[i]; i++) {
    if (!strcmp(attrs[i]->attrName, "saPlmEE"))
      saPlmEE = static_cast<SaNameT *>(*attrs[i]->attrValues);
    else if (!strcmp(attrs[i]->attrName, "libVirtDomainName"))
      domainName = *static_cast<SaStringT *>(*attrs[i]->attrValues);
  }

  TRACE("saPlmEE: %s domain name: %s", saPlmEE->value, domainName);

  // find the hypervisor EE
  SaNameT vmm;
  find_hypervisor_ee(*saPlmEE, &vmm);

  PlmsVmmMap::iterator it(plmsVmmMap.find(vmm));

  if (it != plmsVmmMap.end()) {
    it->second->addVm(*saPlmEE, domainName);

    SavedEEList::iterator savedEEIt(savedEEList.find(*saPlmEE));

    if (savedEEIt != savedEEList.end()) {
      TRACE("setting up vm %s in tree with parent %s",
            saPlmEE->value,
            vmm.value);

      PLMS_ENTITY *vmmEE(getEE(vmm));
      assert(vmmEE);

      PLMS_ENTITY *vmEE(getEE(reinterpret_cast<const char *>(saPlmEE->value)));
      assert(vmEE);

      if (!vmmEE->leftmost_child) {
        vmmEE->leftmost_child = vmEE;
      } else {
        PLMS_ENTITY *tmp(vmmEE->leftmost_child);

        while (tmp->right_sibling)
          tmp = tmp->right_sibling;

        tmp->right_sibling = vmEE;
      }

      vmEE->parent = vmmEE;

      savedEEList.erase(savedEEIt);
    }

    // add to the config map
    PlmsVmConfigMap::iterator cfgIt(plmsVmConfigMap.find(*name));

    if (cfgIt == plmsVmConfigMap.end()) {
      TRACE("creating new virtual machine config");

      PlmsVmConfig *vmConfig(new PlmsVmConfig(
        reinterpret_cast<const char *>(saPlmEE->value), domainName));

      std::pair<PlmsVmConfigMap::iterator, bool> p(
        plmsVmConfigMap.insert(std::make_pair(*name, vmConfig)));
    }
  } else {
    LOG_ER("unable to add VM %s: cannot find VMM %s",
           saPlmEE->value,
           vmm.value);
  }

  TRACE_LEAVE();
}

extern "C" SaAisErrorT
plms_validate_modify_vmm_obj(SaImmOiCcbIdT ccbId,
                             const SaNameT *name,
                             const SaImmAttrModificationT_2 **attrs) {
  TRACE_ENTER();

  SaAisErrorT rc(SA_AIS_OK);

  do {
    PlmsVmmConfigMap::iterator it(plmsVmmConfigMap.find(*name));

    if (it != plmsVmmConfigMap.end()) {
      // is the currrent EE in a good state where we can modify it?
      PLMS_ENTITY *ee(getEE(it->second->getEE()));
      assert(ee);

      if (!check_ee_presence_state(*ee)) {
        LOG_ER("cannot modify hypervisor config as its corresponding current EE"
               " presence state is not UNINSTANTIATED");
        rc = SA_AIS_ERR_BAD_OPERATION;
        break;
      }

      for (int i(0); attrs[i]; i++) {
        SaStringT attr_name(attrs[i]->modAttr.attrName);

        if (strcmp(attr_name, "saPlmEE") == 0) {
          // does the new EE exist in the model?
          const SaNameT *eeName(static_cast<const SaNameT *>
                       (*attrs[i]->modAttr.attrValues));

          ee = getEE(*eeName);

          if (ee) {
            if (!check_ee_presence_state(*ee)) {
              LOG_ER("cannot modify hypervisor config as its corresponding new "
                    "EE presence state is not UNINSTANTIATED");
              rc = SA_AIS_ERR_BAD_OPERATION;
              break;
            }
          } else {
            // not in the patricia tree
            if (isInCcb(ccbId, *eeName)) {
              // it is in the current CCB, so we are OK
              break;
            }

            LOG_ER("EE %s does not exist in current configuration",
                   eeName->value);
            rc = SA_AIS_ERR_NOT_EXIST;
            break;
          }

          break;
        }
      }
    } else {
      rc = SA_AIS_ERR_NOT_EXIST;
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

extern "C" SaAisErrorT
plms_validate_modify_vm_obj(SaImmOiCcbIdT ccbId,
                            const SaNameT *name,
                            const SaImmAttrModificationT_2 **attrs) {
  TRACE_ENTER();

  SaAisErrorT rc(SA_AIS_OK);

  do {
    PlmsVmConfigMap::iterator it(plmsVmConfigMap.find(*name));

    if (it != plmsVmConfigMap.end()) {
      // is the currrent EE in a good state where we can modify it?
      PLMS_ENTITY *ee(getEE(it->second->getEE()));
      assert(ee);

      if (!check_ee_presence_state(*ee)) {
        LOG_ER("cannot modify vm config as its corresponding current EE"
               " presence state is not UNINSTANTIATED");
        rc = SA_AIS_ERR_BAD_OPERATION;
        break;
      }

      for (int i(0); attrs[i]; i++) {
        SaStringT attr_name(attrs[i]->modAttr.attrName);

        if (strcmp(attr_name, "saPlmEE") == 0) {
          // does the new EE exist in the model?
          const SaNameT *eeName(static_cast<const SaNameT *>
                       (*attrs[i]->modAttr.attrValues));
          ee = getEE(*eeName);

          if (ee) {
            if (!check_ee_presence_state(*ee)) {
              LOG_ER("cannot modify vm config as its corresponding new EE "
                     "presence state is not UNINSTANTIATED");
              rc = SA_AIS_ERR_BAD_OPERATION;
              break;
            }
          } else {
            // not in the patricia tree
            if (isInCcb(ccbId, *eeName)) {
              // it is in the current CCB, so we are OK
              break;
            }

            LOG_ER("EE %s does not exist in current configuration",
                   eeName->value);
            rc = SA_AIS_ERR_NOT_EXIST;
            break;
          }

          break;
        }
      }
    } else {
      rc = SA_AIS_ERR_NOT_EXIST;
    }
  } while (false);

  TRACE_LEAVE();
  return rc;
}

extern "C" SaAisErrorT
plms_validate_delete_vmm_obj(const SaNameT *name) {
  TRACE_ENTER();

  SaAisErrorT rc(SA_AIS_OK);

  do {
    PlmsVmmConfigMap::iterator it(plmsVmmConfigMap.find(*name));

    if (it != plmsVmmConfigMap.end()) {
      // is the EE uninstantiated?
      PLMS_ENTITY *ee(getEE(it->second->getEE()));

      /*
       * PLM guarantees that if the hypervisor is UNINSTANTIATED, then its VMs
       * will be, too.
       */
      if (ee && !check_ee_presence_state(*ee)) {
        LOG_ER("cannot delete hypervisor config as its corresponding EE "
               "presence state is not UNINSTANTIATED");
        rc = SA_AIS_ERR_BAD_OPERATION;
        break;
      }
    } else {
      rc = SA_AIS_ERR_NOT_EXIST;
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

extern "C" SaAisErrorT
plms_validate_delete_vm_obj(const SaNameT *name) {
  TRACE_ENTER();

  SaAisErrorT rc(SA_AIS_OK);

  do {
    PlmsVmConfigMap::iterator it(plmsVmConfigMap.find(*name));

    if (it != plmsVmConfigMap.end()) {
      // is the EE uninstantiated?
      PLMS_ENTITY *ee(getEE(it->second->getEE()));
      assert(ee);

      if (!check_ee_presence_state(*ee)) {
        LOG_ER("cannot delete vm config as its corresponding EE presence state "
               "is not UNINSTANTIATED");
        rc = SA_AIS_ERR_BAD_OPERATION;
        break;
      }
    } else {
      rc = SA_AIS_ERR_NOT_EXIST;
    }
  } while (false);

  TRACE_LEAVE();
  return rc;
}

extern "C" void
plms_modify_vmm_obj(const SaNameT *name, SaImmAttrModificationT_2 **attrs) {
  TRACE_ENTER();

  PlmsVmmConfigMap::iterator it(plmsVmmConfigMap.find(*name));

  if (it != plmsVmmConfigMap.end()) {
    for (int i(0); attrs[i]; i++) {
      SaStringT attr_name(attrs[i]->modAttr.attrName);
      TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->modType);

      if (strcmp(attr_name, "saPlmEE") == 0) {
        // EE name is changing -- save the old one
        SaNameT ee;
        ee.length = it->second->getEE().length();
        memcpy(ee.value, it->second->getEE().c_str(), ee.length);

        it->second->setEE(reinterpret_cast<const char *>(static_cast<SaNameT *>
          (*attrs[i]->modAttr.attrValues)->value));

        // EE changed -- update map
        PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(ee));

        if (vmmIt != plmsVmmMap.end()) {
          PlmsVmm *vmm(vmmIt->second);

          plmsVmmMap.erase(vmmIt);

          // setup the new EE name
          ee.length = it->second->getEE().length();
          memcpy(ee.value, it->second->getEE().c_str(), ee.length);

          plmsVmmMap.insert(std::make_pair(ee, vmm));
        } else {
          LOG_ER("unable to find VMM to change EE");
          assert(false);
        }
      } else if (strcmp(attr_name, "libVirtSession") == 0) {
        it->second->setSession(*static_cast<SaStringT *>
          (*attrs[i]->modAttr.attrValues));
      } else {
        LOG_ER("unhandled attr type in modify vm");
      }
    }
  } else {
    LOG_ER("unable to find VMM config %s to modify", name->value);
    assert(false);
  }

  TRACE_LEAVE();
}

extern "C" void
plms_modify_vm_obj(const SaNameT *name, SaImmAttrModificationT_2 **attrs) {
  TRACE_ENTER();

  PlmsVmConfigMap::iterator it(plmsVmConfigMap.find(*name));

  if (it != plmsVmConfigMap.end()) {
    SaNameT oldEE;
    oldEE.length = it->second->getEE().length();
    memcpy(oldEE.value, it->second->getEE().c_str(), oldEE.length);

    for (int i(0); attrs[i]; i++) {
      SaStringT attr_name(attrs[i]->modAttr.attrName);
      TRACE_2("attr_name: %s, mod_type: %u", attr_name, attrs[i]->modType);
      if (strcmp(attr_name, "saPlmEE") == 0) {
        it->second->setEE(reinterpret_cast<const char *>(static_cast<SaNameT *>
          (*attrs[i]->modAttr.attrValues)->value));
      } else if (strcmp(attr_name, "libVirtDomainName") == 0) {
        it->second->setDomainName(*static_cast<SaStringT *>
          (*attrs[i]->modAttr.attrValues));
      } else {
        LOG_ER("unhandled attr type in modify vm");
      }

      // update hypervisor with new VM config
      SaNameT vmm;
      find_hypervisor_ee(oldEE, &vmm);

      PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(vmm));

      if (vmmIt != plmsVmmMap.end()) {
        vmmIt->second->updateVM(oldEE, *it->second);
      } else {
        LOG_ER("unable to find VMM to change VM config");
        assert(false);
      }
    }
  } else {
    LOG_ER("unable to find VMM config %s to modify", name->value);
    assert(false);
  }

  TRACE_LEAVE();
}

extern "C" void
plms_delete_vmm_obj(const SaNameT *name) {
  TRACE_ENTER();

  PlmsVmmConfigMap::iterator it(plmsVmmConfigMap.find(*name));

  if (it != plmsVmmConfigMap.end()) {
    // remove the hypervisor from the VMM map, too
    SaNameT eeName = { 0, 0 };
    eeName.length = it->second->getEE().length();
    memcpy(eeName.value, it->second->getEE().c_str(), eeName.length);

    PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(eeName));

    if (vmmIt != plmsVmmMap.end()) {
      delete vmmIt->second;

      plmsVmmMap.erase(vmmIt);
    }

    delete it->second;

    plmsVmmConfigMap.erase(it);
  }

  TRACE_LEAVE();
}

extern "C" void
plms_delete_vm_obj(const SaNameT *name) {
  TRACE_ENTER();

  PlmsVmConfigMap::iterator it(plmsVmConfigMap.find(*name));

  if (it != plmsVmConfigMap.end()) {
    // remove the VM from the VMM's map, too
    SaNameT eeName = { 0, 0 };
    eeName.length = it->second->getEE().length();
    memcpy(eeName.value, it->second->getEE().c_str(), eeName.length);

    // find the hypervisor to remove the vm mapping
    SaNameT vmm;
    find_hypervisor_ee(eeName, &vmm);

    PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(vmm));

    if (vmmIt != plmsVmmMap.end())
      vmmIt->second->removeVm(eeName);

    delete it->second;

    plmsVmConfigMap.erase(it);
  }

  TRACE_LEAVE();
}

extern "C" SaUint32T
plms_ee_instantiate_vm(const PLMS_ENTITY *entity) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  // find the hypervisor and tell it to boot the vm
  PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(entity->parent->dn_name));

  if (vmmIt != plmsVmmMap.end()) {
    rc = vmmIt->second->instantiate(entity->dn_name);
  } else {
    LOG_ER("unable to find hypervisor to instantiate vm: %s",
           entity->dn_name.value);
  }

  TRACE_LEAVE();

  return rc;
}

extern "C" SaUint32T
plms_ee_hypervisor_instantiated(const PLMS_ENTITY *entity) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  PlmsVmmMap::iterator vmmIt(plmsVmmMap.find(entity->dn_name));

  if (vmmIt != plmsVmmMap.end()) {
    rc = vmmIt->second->instantiated();
  } else {
    LOG_ER("unable to find hypervisor %s", entity->dn_name.value);
  }

  TRACE_LEAVE();

  return rc;
}

extern "C" SaUint32T
plms_ee_restart_vm(const PLMS_ENTITY *entity) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  // find the hypervisor and tell it to restart the vm
  PlmsVmmMap::iterator vmmIt(
    plmsVmmMap.find(entity->parent->dn_name));

  if (vmmIt != plmsVmmMap.end())
    rc = vmmIt->second->restart(entity->dn_name);

  TRACE_LEAVE();

  return rc;
}

extern "C" SaUint32T
plms_ee_isolate_vm(const PLMS_ENTITY *entity) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  // find the hypervisor and tell it to terminate the vm
  PlmsVmmMap::iterator vmmIt(
    plmsVmmMap.find(entity->parent->dn_name));

  if (vmmIt != plmsVmmMap.end())
    rc = vmmIt->second->isolate(entity->dn_name);

  TRACE_LEAVE();

  return rc;
}

PlmsVmmConfig::PlmsVmmConfig(const std::string& e, const std::string& s)
  : ee(e), session(s) {}

PlmsVmConfig::PlmsVmConfig(const std::string& e, const std::string& d)
  : ee(e), domainName(d) {}

PlmsVmm::PlmsVmm(const char *session)
  : libVirtSession(session), vPtr(0) {
  TRACE_ENTER();

  int rc(virInitialize());

  if (rc < 0) {
    LOG_ER("virInitialize returned error");
    assert(false);
  }

  static bool libVirtInit(false);

  if (!libVirtInit) {
    virSetErrorFunc(0, libVirtError);

    libVirtInit = true;
  }

  TRACE_LEAVE();
}

PlmsVmm::~PlmsVmm(void) {
  TRACE_ENTER();

  for (VmMap::iterator it(vmMap.begin()); it != vmMap.end(); ++it)
    delete it->second;

  if (vPtr)
    virConnectClose(vPtr);

  TRACE_LEAVE();
}

int
PlmsVmm::instantiated(void) {
  int rc(NCSCC_RC_SUCCESS);

  TRACE_ENTER2("hypervisor at %s instantiated", libVirtSession.c_str());

  if (vPtr) {
    TRACE("connection already established with hypervisor: closing it");
    virConnectClose(vPtr);
  }

  connect();

  TRACE_LEAVE();

  return rc;
}

void
PlmsVmm::addVm(const SaNameT& name, const char *domainName) {
  TRACE_ENTER();

  PlmsVm *vm(new PlmsVm(domainName));

  std::pair<VmMap::iterator, bool> p(vmMap.insert(std::make_pair(name, vm)));

  assert(p.second);

  TRACE_LEAVE();
}

void
PlmsVmm::removeVm(const SaNameT& name) {
  TRACE_ENTER();

  VmMap::iterator it(vmMap.find(name));

  if (it != vmMap.end()) {
    delete it->second;

    vmMap.erase(it);
  }

  TRACE_LEAVE();
}

int
PlmsVmm::instantiate(const SaNameT& name) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  do {
    VmMap::iterator vmIt(vmMap.find(name));

    if (vmIt != vmMap.end()) {
      virDomainPtr domain(getDomainPtr(*vmIt->second));

      if (!domain)
        break;

      if (vmIt->second->instantiate(domain) == 0)
        rc = NCSCC_RC_SUCCESS;

      virDomainFree(domain);
    } else {
      LOG_ER("unable to find VM %s to instantiate", name.value);
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

int
PlmsVmm::restart(const SaNameT& name) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  do {
    VmMap::iterator vmIt(vmMap.find(name));

    if (vmIt != vmMap.end()) {
      virDomainPtr domain(getDomainPtr(*vmIt->second));

      if (!domain)
        break;

      if (vmIt->second->restart(domain) == 0)
        rc = NCSCC_RC_SUCCESS;

      virDomainFree(domain);
    } else {
      LOG_ER("unable to find VM %s to restart", name.value);
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

int
PlmsVmm::isolate(const SaNameT& name) {
  int rc(NCSCC_RC_FAILURE);

  TRACE_ENTER();

  do {
    TRACE("looking for vm: %s", name.value);

    VmMap::iterator vmIt(vmMap.find(name));

    if (vmIt != vmMap.end()) {
      virDomainPtr domain(getDomainPtr(*vmIt->second));

      if (!domain)
        break;

      if (vmIt->second->isolate(domain) == 0)
        rc = NCSCC_RC_SUCCESS;

      virDomainFree(domain);
    } else {
      LOG_ER("unable to find VM %s to isolate", name.value);
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

void
PlmsVmm::connect(void) {
  TRACE_ENTER2("attempting to connect to libvirtd with session: %s",
               libVirtSession.c_str());

  vPtr = virConnectOpen(libVirtSession.c_str());

  if (!vPtr) {
    LOG_ER("unable to connect to hypervisor at %s", libVirtSession.c_str());
  }

  TRACE_LEAVE();
}

virDomainPtr
PlmsVmm::getDomainPtr(const PlmsVm& vm) {
  TRACE_ENTER();

  virDomainPtr domain(virDomainLookupByName(vPtr, vm.getDomainName().c_str()));

  if (!domain) {
    LOG_ER("unable to get domain ptr from libvirt for domain: %s",
           vm.getDomainName().c_str());
  }

  TRACE_LEAVE();

  return domain;
}

void
PlmsVmm::updateVM(const SaNameT& oldEE, const PlmsVmConfig& config) {
  VmMap::iterator it(vmMap.find(oldEE));

  if (it != vmMap.end()) {
    PlmsVm *vm(it->second);

    vmMap.erase(it);

    vm->updateDomainName(config.getDomainName());

    SaNameT ee;
    ee.length = config.getEE().length();
    memcpy(ee.value, config.getEE().c_str(), ee.length);

    vmMap.insert(std::make_pair(ee, vm));
  } else {
    LOG_ER("unable to find VM to update config");
    assert(false);
  }
}

void
PlmsVmm::libVirtError(void *userData, virErrorPtr error) {
  LOG_ER("libvirt error code: %i message: %s domain: %i",
         error->code,
         error->message,
         error->domain);
}


// PLMS VM
PlmsVm::PlmsVm(const char *d)
  : domainName(d)
{}

int
PlmsVm::instantiate(virDomainPtr domain) {
  TRACE_ENTER();

  int rc(-1);

  do {
    virDomainInfo info;

    rc = virDomainGetInfo(domain, &info);

    if (rc < 0)
      break;

    if (info.state == VIR_DOMAIN_RUNNING) {
      TRACE("vm is already running: restarting it");

      rc = restart(domain);

      if (rc < 0)
        break;
    } else {
      TRACE("calling virDomainCreate to instantiate vm");

      rc = virDomainCreate(domain);

      if (rc < 0)
        break;
    }
  } while (false);

  TRACE_LEAVE();

  return rc;
}

int
PlmsVm::restart(virDomainPtr domain) {
  TRACE("calling virDomainReset to restart vm");
  return virDomainReset(domain, 0);
}

int
PlmsVm::isolate(virDomainPtr domain) {
  TRACE("calling virDomainDestroy to isolate vm");
  return virDomainDestroy(domain);
}
