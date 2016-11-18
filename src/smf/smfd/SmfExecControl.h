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
#ifndef SMF_SMFD_SMFEXECCONTROL_H_
#define SMF_SMFD_SMFEXECCONTROL_H_

#include <vector>
#include <string>
#include "base/macros.h"
#include "smf/smfd/SmfUpgradeStep.h"
#include "smf/smfd/SmfUpgradeAction.h"
#include "osaf/saf/saAis.h"

/*
 * These set of functions enables the balanced mode feature. This mode changes
 * the execution of rolling procedures to be merged into one or several single
 * steps that are spread out across the cluster nodes. This feature is used to
 * give a faster upgrade time compared to rolling one node at a time, possibly
 * several times for each node. By splitting the procedures it into several
 * single steps across the nodes a total service outage can be avoided.
 */
namespace execctrl {
  /*
   * Creates empty procedures with the configuration provided in the
   * ExecControl object. The Campaign object need these empty procedures to
   * spawn procedure threads. Once started the steps will be built and added to
   * the procedure.
   */
  bool createBalancedProcs();
  /*
   * Create the merged step for a balanced procedure. This merged step is based
   * on steps from original procedures matching the balanced group.
   */
  bool createStepForBalancedProc(SmfUpgradeProcedure* procedure);
  /*
   * Set the steps that will be merged to completed state and move the wrapup
   * actions to the last procedure if all steps are to be merged. This is
   * because we don't want to call the wrapup actions twice if it is not
   * needed.
   */
  void disableMergeSteps(SmfUpgradeProcedure* procedure);

} // namespace execctrl

#endif  // SMF_SMFD_SMFEXECCONTROL_H_
