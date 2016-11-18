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
 *   FILE NAME: plms_virt.h
 *
 *   DESCRIPTION: C++ implementation of interface to libvirt
 *
 ****************************************************************************/
#ifndef PLM_PLMS_VIRT_H_
#define PLM_PLMS_VIRT_H_

#include "osaf/saf/saAis.h"
#include <libvirt/libvirt.h>
#include <cstring>
#include <map>
#include <string>

struct ltSaNameT {
  bool operator()(const SaNameT& n1, const SaNameT& n2) const {
    bool status(false);

    if (n1.length < n2.length)
      status = true;
    else if (n1.length == n2.length)
      status = memcmp(n1.value, n2.value, n1.length) < 0;

    return status;
  }
};

// class to represent Virtual Machine configuration
class PlmsVmConfig {
 public:
  PlmsVmConfig(const std::string& ee, const std::string& domain);

  const std::string& getEE(void) const { return ee; }
  const std::string& getDomainName(void) const { return domainName; }

  void setEE(const std::string& e) { ee = e; }
  void setDomainName(const std::string& d) { domainName = d; }

 private:
  std::string ee;
  std::string domainName;
};

// class to represent Virtual Machine Monitor configuration
class PlmsVmmConfig {
 public:
  PlmsVmmConfig(const std::string& ee, const std::string& session);

  const std::string& getEE(void) const { return ee; }
  const std::string& getSession(void) const { return session; }

  void setEE(const std::string& e) { ee = e; }
  void setSession(const std::string& s) { session = s; }

 private:
  std::string ee;
  std::string session;
};

// class to represent Virtual Machine
class PlmsVm {
 public:
  explicit PlmsVm(const char *domain);

  int instantiate(virDomainPtr);
  int restart(virDomainPtr);
  int isolate(virDomainPtr);

  const std::string& getDomainName(void) const {
    return domainName;
  }

  void updateDomainName(const std::string& d) { domainName = d; }

 private:
  std::string domainName;
};

// class to represent Virtual Machine Monitor (Hypervisor)
class PlmsVmm {
 public:
  explicit PlmsVmm(const char *session);
  ~PlmsVmm(void);

  // VMM functions
  int instantiated(void);
  int isolate(void);

  // VM functions
  void addVm(const SaNameT& vmEE, const char *domainName);
  void removeVm(const SaNameT& vmEE);
  int instantiate(const SaNameT& vmEE);
  int restart(const SaNameT& vmEE);
  int isolate(const SaNameT& vmEE);

  // modify functions
  void updateSession(const std::string& s) { libVirtSession = s; }
  void updateVM(const SaNameT& oldEE, const PlmsVmConfig&);

  const std::string& getSession(void) const { return libVirtSession; }

 private:
  typedef std::map<SaNameT /*SaPlmEE*/, PlmsVm *, ltSaNameT> VmMap;

  std::string libVirtSession;
  VmMap       vmMap;

  virConnectPtr vPtr;

  void connect(void);

  virDomainPtr getDomainPtr(const PlmsVm&);

  static void libVirtError(void *userData, virErrorPtr);
};

#endif  // PLM_PLMS_VIRT_H_
