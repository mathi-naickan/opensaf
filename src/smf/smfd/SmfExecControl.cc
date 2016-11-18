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

#include "SmfExecControl.h"
#include <list>
#include <limits>
#include <algorithm>
#include "base/logtrace.h"
#include "osaf/immutil/immutil.h"
#include "smf/smfd/SmfUpgradeProcedure.h"
#include "smf/smfd/SmfUpgradeMethod.h"
#include "smf/smfd/SmfUtils.h"
#include "smf/smfd/SmfUpgradeAction.h"
#include "smf/smfd/SmfUpgradeCampaign.h"

namespace execctrl {

static std::vector<SmfUpgradeStep*> getStepsMatchingBalancedGroup(SmfUpgradeProcedure* procedure,
                                                                  SmfUpgradeCampaign* ucamp);

static SmfUpgradeStep* mergeStep(SmfUpgradeProcedure* procedure,
                                 const std::vector<SmfUpgradeStep*>& steps);

static bool removeDuplicateActivationUnits(SmfUpgradeProcedure * i_newproc, SmfUpgradeStep *newStep,
                                           const std::list<unitNameAndState>& deact);


static bool isNodeInGroup(const std::string& node,
                          const std::vector<std::string>& group) {
  return std::find(group.begin(), group.end(), node) != group.end();
}

bool createBalancedProcs() {
  // Creates new procedures based on the ExecControl configuration
  TRACE_ENTER();
  SmfCampaign* camp = SmfCampaignThread::instance()->campaign();
  SmfUpgradeCampaign* ucamp = camp->getUpgradeCampaign();
  SmfExecControlObjHandler* exechdl = ucamp->getExecControlHdl();

  bool errinfo = false;
  SaUint32T numberofss = exechdl->numberOfSingleSteps(&errinfo);
  if (errinfo == false) {
    TRACE("Failed reading attribute from ExecControlHdl");
    return false;
  }
  // Assume that nodesForSingleStep only contains nodes used by the campaign.
  std::vector<std::string> nodesforss = exechdl->nodesForSingleStep(&errinfo);
  if (errinfo == false) {
    TRACE("Failed reading attribute from ExecControlHdl");
    return false;
  }
  for (auto& node : nodesforss) {
    node = "safAmfNode=" + std::string(node) + ",safAmfCluster=myAmfCluster";
  }
  std::vector<SmfUpgradeProcedure*> balancedprocs;
  // chunk is the size of the balanced group
  unsigned int chunk = (nodesforss.size() + numberofss - 1) / numberofss;
  TRACE("balanced group size will be %i", chunk);
  std::vector<std::string>::iterator itr;
  std::vector<std::string>::iterator iend;
  // Set the procedure exec level to be as high as possible
  int procExecLvl = std::numeric_limits<int>::max() - numberofss - 1;
  int procnum = 0;
  for (itr = nodesforss.begin(); iend < nodesforss.end(); itr += chunk) {
    iend = itr + chunk;
    SmfUpgradeProcedure *ssproc = new(std::nothrow) SmfUpgradeProcedure;
    if (iend >= nodesforss.end()) {
      iend = nodesforss.end();
    }
    ssproc->setUpgradeMethod(new(std::nothrow) SmfSinglestepUpgrade);
    ssproc->setProcName("safSmfProc=SmfBalancedProc" + std::to_string(procnum));
    ssproc->setExecLevel(std::to_string(procExecLvl));
    ssproc->setIsMergedProcedure(true);  // For cleanup purposes
    // Each new procedure holds the balanced group it steps over
    ssproc->setBalancedGroup(std::vector<std::string>(itr, iend));
    balancedprocs.push_back(ssproc);
    procExecLvl++;
    procnum++;
  }
  for (auto proc : balancedprocs) {
      ucamp->addUpgradeProcedure(proc);
  }
  TRACE_LEAVE();
  return true;
}

bool createStepForBalancedProc(SmfUpgradeProcedure* procedure) {
  TRACE_ENTER();
  SmfCampaign* camp = SmfCampaignThread::instance()->campaign();
  SmfUpgradeCampaign* ucamp = camp->getUpgradeCampaign();

  auto steps = getStepsMatchingBalancedGroup(procedure, ucamp);
  std::list <unitNameAndState> deact;
  for (auto step : steps) {
    SmfUpgradeProcedure* oproc = step->getProcedure();
    // copy callbacks to the new procedure
    procedure->getCallbackList(oproc->getUpgradeMethod());
    deact.insert(deact.end(),
                 step->getDeactivationUnitList().begin(),
                 step->getDeactivationUnitList().end());
  }
  if (!steps.empty()) {
    SmfUpgradeStep* newstep = mergeStep(procedure, steps);
    removeDuplicateActivationUnits(procedure, newstep, deact);
    procedure->addProcStep(newstep);
  }
  const std::vector<SmfUpgradeProcedure*>& allprocs = ucamp->getProcedures();
  if (procedure == (*--allprocs.end())) {
    // This is the last balanced procedure configure the wrapup actions.
    for (auto wac : procedure->getWrapupActions()) {
      const SmfCallbackAction* cba = dynamic_cast<const SmfCallbackAction*>(wac);
      if (cba != nullptr) {
        const_cast<SmfCallbackAction*>(cba)->setCallbackProcedure(procedure);
      }
    }
  }
  TRACE_LEAVE();
  return true;
}

void disableMergeSteps(SmfUpgradeProcedure* procedure) {
  TRACE_ENTER();
  if (!procedure->getBalancedGroup().empty()) {
    TRACE("not an original proc");
    return;
  }
  SmfCampaign* camp = SmfCampaignThread::instance()->campaign();
  SmfUpgradeCampaign* ucamp = camp->getUpgradeCampaign();
  SmfExecControlObjHandler* exechdl = ucamp->getExecControlHdl();
  std::vector<std::string> nodesforss = exechdl->nodesForSingleStep(NULL);
  for (auto& node : nodesforss) {
    node = "safAmfNode=" + std::string(node) + ",safAmfCluster=myAmfCluster";
  }

  bool allmerged = true;
  for (auto step : procedure->getProcSteps()) {
    if (!isNodeInGroup(step->getSwNode(), nodesforss)) {
      TRACE("node not in group stepNode:%s", step->getSwNode().c_str());
      allmerged = false;
    } else {
      TRACE("node in group stepNode:%s", step->getSwNode().c_str());
      // disable this step, it will be executed in a balanced step
      step->setStepState(SA_SMF_STEP_COMPLETED);
    }
  }

  if (allmerged) {
    // All steps in this procedure are merged to balanced steps therefore we
    // move the wrapup actions to the last balanced procedure.
    const std::vector<SmfUpgradeProcedure*>& allprocs = ucamp->getProcedures();
    auto lastproc = --allprocs.end();
    for (auto& wact : procedure->getWrapupActions()) {
      (*lastproc)->addProcWrapupAction(wact);
    }
    procedure->clearWrapupActions();
  }

  TRACE_LEAVE();
}

std::vector<SmfUpgradeStep*> getStepsMatchingBalancedGroup(
    SmfUpgradeProcedure* procedure, SmfUpgradeCampaign* ucamp) {
  TRACE_ENTER();
  // For all original procedures check if the steps are in the balanced group
  // of the procedure. Return the matching steps.
  std::vector<SmfUpgradeStep*> steps;
  for (auto proc : ucamp->getProcedures()) {
    if (proc->getBalancedGroup().empty()) {
      for (auto ostep : proc->getProcSteps()) {
        if (isNodeInGroup(ostep->getSwNode(), procedure->getBalancedGroup())) {
          TRACE("step in group: %s", ostep->getDn().c_str());
          steps.push_back(ostep);
        } else {
          TRACE("step not in group: %s", ostep->getDn().c_str());
        }
      }
    }
  }
  TRACE_LEAVE();
  return steps;
}

SmfUpgradeStep* mergeStep(SmfUpgradeProcedure* procedure,
    const std::vector<SmfUpgradeStep*>& steps) {
  // Create a merged step based on the upgrade step passed in. The in/out
  // parameter procedure must be a balanced procedure.
  TRACE_ENTER();
  SmfUpgradeStep* newstep = new(std::nothrow)SmfUpgradeStep;
  osafassert(newstep != nullptr);
  newstep->setRdn("safSmfStep=0001");
  newstep->setDn(newstep->getRdn() + "," + procedure->getDn());
  newstep->setMaxRetry(0);
  newstep->setRestartOption(0);
  for (auto step : steps) {
    if (isNodeInGroup(step->getSwNode(), procedure->getBalancedGroup())) {
      TRACE("adding modifications and bundle ref from node:%s",
            step->getSwNode().c_str());
      newstep->addModifications(step->getModifications());
      procedure->mergeBundleRefRollingToSingleStep(newstep, step);
    }
  }
  TRACE_LEAVE();
  return newstep;
}

bool removeDuplicateActivationUnits(SmfUpgradeProcedure * i_newproc, SmfUpgradeStep *newStep,
                                    const std::list<unitNameAndState>& deact) {
  // Remove any (de)activation unit duplicates and add them to the step.
  // Activation and deactivation units are the same because rolling and
  // formodify is symetric.
  TRACE_ENTER();
  std::list < unitNameAndState > tmpDU;
  tmpDU.insert(tmpDU.begin(), deact.begin(), deact.end());
  std::multimap<std::string, objectInst> objInstances;
  if (i_newproc->getImmComponentInfo(objInstances) == false) {
    TRACE("Config info from IMM could not be read");
    return false;
  }

  // Remove DU duplicates
  tmpDU.sort(compare_du_part);
  tmpDU.unique(unique_du_part);

  // Reduce the DU list, check if smaller scope is within bigger scope.
  std::pair<std::multimap<std::string, objectInst>::iterator,
            std::multimap<std::string, objectInst>::iterator> nodeName_mm;
  std::multimap<std::string, objectInst>::iterator iter;

  std::list < unitNameAndState > nodeLevelDU;
  std::list < unitNameAndState >::iterator unit_iter;
  // Find DU on node level and save them in a separate list
  for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
    if ((*unit_iter).name.find("safAmfNode=") == 0) {  // DU is a node node
      // A node will never be optimized away, save it
      nodeLevelDU.push_back(*unit_iter);
      // Remove the node and update iterator
      unit_iter = tmpDU.erase(unit_iter);
    } else {
      ++unit_iter;
    }
  }

  // For all found nodes, look if some other DU (comp/SU) is within scope
  // tmpDU contain all DU except the node level ones which was removed above
  // and saved in nodeLevelDU list
  std::list < unitNameAndState >::iterator itr;
  for (itr = nodeLevelDU.begin(); itr != nodeLevelDU.end(); ++itr) {
    // For all comp/SU found in the scope of the node.  Find out if any
    // remaining DU is within it.
    // Get all components/SU within the node
    nodeName_mm = objInstances.equal_range((*itr).name);
    for (iter = nodeName_mm.first;  iter != nodeName_mm.second;  ++iter) {
      // For all comp/SU found in the scope of the node.
      // Find out if any remaining DU is within it
      for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
        if ((*unit_iter).name == (*iter).second.suDN) {  // Check SU
          TRACE("[%s] is in scope of [%s], remove it from DU list",
                 (*unit_iter).name.c_str(), (*itr).name.c_str());
          // Remove the node and update iterator
          unit_iter = tmpDU.erase(unit_iter);
        } else if ((*unit_iter).name == (*iter).second.compDN) {  // Check comp
          TRACE("[%s] is in scope of [%s], remove it from DU list",
                 (*unit_iter).name.c_str(), (*itr).name.c_str());
          // Remove the node and update iterator
          unit_iter = tmpDU.erase(unit_iter);
        } else {
          ++unit_iter;
        }
      }
    }
  }

  // tmpDU contain all DU which was not in the scope of an included node Find
  // DU on SU level and save them in a separate list. Remove SU from tmpDU list
  std::list < unitNameAndState > suLevelDU;
  for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
    if ((*unit_iter).name.find("safSu=") == 0) {  // DU is a SU
      // A node will never be optimized away, save it
      suLevelDU.push_back(*unit_iter);
      unit_iter = tmpDU.erase(unit_iter);  // Remove the SU and update iterator
    } else {
      ++unit_iter;
    }
  }

  // For all SU in the suLevelDU list, look if remaining DU in tmpDU is within
  // scope of the SU
  std::list < unitNameAndState >::iterator su_iter;
  for (su_iter = suLevelDU.begin(); su_iter != suLevelDU.end(); ++su_iter) {
    for (unit_iter = tmpDU.begin(); unit_iter != tmpDU.end();) {
      if ((*unit_iter).name.find((*su_iter).name) != std::string::npos) {
        // The component was in the scope of the SU
        TRACE("[%s] is in scope of [%s], remove it from DU list",
              (*unit_iter).name.c_str(), (*su_iter).name.c_str());
        // Remove the Component and update iterator
        unit_iter = tmpDU.erase(unit_iter);
      } else {
        ++unit_iter;
      }
    }
  }

  newStep->addDeactivationUnits(nodeLevelDU);  // Add the node level DU
  newStep->addDeactivationUnits(suLevelDU);  // Add the SU level DU
  newStep->addDeactivationUnits(tmpDU);  // Add the comp level DU

  // Rolling and forModify are symetric, add the node level DU
  newStep->addActivationUnits(nodeLevelDU);
  // Rolling and forModify are symetric, Add the SU level DU
  newStep->addActivationUnits(suLevelDU);
  // Rolling and forModify are symetric, Add the comp level DU
  newStep->addActivationUnits(tmpDU);

  TRACE_LEAVE();
  return true;
}

} // namespace execctrl
