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

#ifndef SMF_SMFD_SMFUPGRADESTEP_H_
#define SMF_SMFD_SMFUPGRADESTEP_H_

#include "base/macros.h"

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <semaphore.h>
#include "base/ncsgl_defs.h"

#include <atomic>
#include <string>
#include <vector>
#include <list>

#include "amf/saf/saAmf.h"
#include "smf/saf/saSmf.h"
#include "imm/saf/saImmOi.h"
#include "smf/smfd/SmfStepState.h"
#include "smf/smfd/SmfTargetTemplate.h"

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

class SmfUpgradeProcedure;
class SmfCallback;

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */
#define m_LOAD_THREAD_STACKSIZE NCS_STACKSIZE_SMALL

typedef enum {
	SMF_NO_CLUSTER_REBOOT = 0,
	SMF_INSTALLATION_REBOOT = 1,
	SMF_REMOVAL_REBOOT = 2
} SmfRebootT;

struct SmfNodeUpInfo {
        std::string node_name;
        unsigned int nd_up_cntr;
};

struct unitNameAndState{
	  std::string name;
	  SaAmfAdminStateT initState;
	  SaAmfAdminStateT currentState;
};

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */
class SmfStepType;

///------------------------------------------------------------------------------
/// Purpose: Class for activation unit
///------------------------------------------------------------------------------
class SmfActivationUnit {
 public:

///
/// Purpose:  Constructor
/// @param    -
/// @return   None
///
      SmfActivationUnit() 
      {
      }
///
/// Purpose:  Destructor
/// @param    None
/// @return   None
/// 
      ~SmfActivationUnit() 
      {
      }

      std::list < unitNameAndState > m_actedOn;
};

///
/// Purpose: Class for upgrade step
///
class SmfUpgradeStep {
 public:

///
/// Purpose:  Constructor
/// @param    -
/// @return   None
///
	SmfUpgradeStep();

///
/// Purpose:  Destructor
/// @param    None
/// @return   None
///
	~SmfUpgradeStep();

///
/// Purpose:  Init attributes from IMM object
/// @param    Imm attributes
/// @return   SA_AIS_OK on OK or error code
///
	SaAisErrorT init(const SaImmAttrValuesT_2 ** attrValues);

///
/// Purpose:  Get current step state
/// @param    None
/// @return   A pointer to the state
///
	SaSmfStepStateT getState() const;

///
/// Purpose:  Set the name of the step
/// @param    A string specifying the name of the step
/// @return   None
///
	void setRdn(const std::string & i_rdn);

///
/// Purpose:  Get the name of the step
/// @param    -
/// @return   A string specifying the name of the step
///
	const std::string & getRdn() const;

///
/// Purpose:  Set the DN of the step
/// @param    -
/// @return   -
///
	void setDn(const std::string & i_dn);

///
/// Purpose:  Get the DN of the step
/// @param    -
/// @return   -
///
	const std::string & getDn() const;

///
/// Purpose:  Set the max retry
/// @param    The max retry
/// @return   None
///
	void setMaxRetry(unsigned int i_maxRetry);

///
/// Purpose:  Get the max retry
/// @param    -
/// @return   The max retry
///
	unsigned int getMaxRetry() const;

///
/// Purpose:  Set the retry count
/// @param    The retry count
/// @return   None
///
	void setRetryCount(unsigned int i_retryCount);

///
/// Purpose:  Get the retry count
/// @param    -
/// @return   The retry count
///
	unsigned int getRetryCount() const;

///
/// Purpose:  Set the restart option
/// @param    The restart option
/// @return   None
///
	void setRestartOption(unsigned int i_restartOption);

///
/// Purpose:  Get the restart option
/// @param    -
/// @return   The restart option
///
	unsigned int getRestartOption() const;

///
/// Purpose:  Execute the upgrade step
/// @param    None
/// @return   None
///
	SmfStepResultT execute();

///
/// Purpose:  Rollback the upgrade step
/// @param    None
/// @return   None
///
	SmfStepResultT rollback();

///
/// Purpose:  Set the procedure
/// @param    A ptr to our procedure 
/// @return   None
///
	void setProcedure(SmfUpgradeProcedure * i_procedure);

///
/// Purpose:  Get the procedure
/// @param    None
/// @return   A pointer to a SmfUpgradeProcedure
///
	SmfUpgradeProcedure *getProcedure();

///
/// Purpose:  Add an activation unit DN
/// @param    A DN to an activation unit 
/// @return   None
///
	void addActivationUnit(const unitNameAndState & i_activationUnit);

///
/// Purpose:  Add a list of activation units
/// @param    i_activationUnits A list of unitNameAndState
/// @return   None
///
	void addActivationUnits(const std::list < unitNameAndState > & i_activationUnits)
                { m_activationUnit.m_actedOn.insert(m_activationUnit.m_actedOn.end(),
                                     i_activationUnits.begin(),
                                     i_activationUnits.end()); }

///
/// Purpose:  Get the activation unit DN
/// @param     
/// @return   A list of DN to activation units
///
	const std::list < unitNameAndState > &getActivationUnitList();

///
/// Purpose:  Add an deactivation unit DN
/// @param    A DN to an deactivation unit 
/// @return   None
///
	void addDeactivationUnit(const unitNameAndState & i_deactivationUnit);
  
///
/// Purpose:  Add a list of deactivation units
/// @param    i_deactivationUnits A list of unitNameAndState
/// @return   None
///
	void addDeactivationUnits(const std::list < unitNameAndState > & i_deactivationUnits)
                { m_deactivationUnit.m_actedOn.insert(m_deactivationUnit.m_actedOn.end(),
                                     i_deactivationUnits.begin(),
                                     i_deactivationUnits.end()); }

///
/// Purpose:  Get the deactivation unit DN
/// @param     
/// @return   A list of DN to deactivation units
///
	const std::list < unitNameAndState > &getDeactivationUnitList();
  
///
/// Purpose:  Clean the deactivation unit list
/// @param    None
/// @return   None
///
	void cleanDeactivationUnitList(void)
                { m_deactivationUnit.m_actedOn.clear(); }

///
/// Purpose:  Add a sw bundle to remove
/// @param    A DN to a bundle 
/// @return   None
///
	void addSwRemove(const std::list < SmfBundleRef * >&i_swRemove);
	void addSwRemove(std::list<SmfBundleRef> const& i_swRemove);
	void addSwRemove(SmfBundleRef& i_swRemove)
                { m_swRemoveList.push_back(i_swRemove); }

///
/// Purpose:  Get the sw remove list
/// @param     
/// @return   A list of bundles to remove
///
	std::list < SmfBundleRef > &getSwRemoveList();

///
/// Purpose:  Add a sw bundle to add
/// @param    A DN to a bundle 
/// @return   None
///
	void addSwAdd(const std::list < SmfBundleRef* >&i_swAdd);
	void addSwAdd(std::list<SmfBundleRef> const& i_swAdd);
	void addSwAdd(SmfBundleRef& i_swAdd)
                { m_swAddList.push_back(i_swAdd); }
///
/// Purpose:  Get the sw add list
/// @param     
/// @return   A list of bundles to add
///
	std::list < SmfBundleRef > &getSwAddList();

///
/// Purpose:  Add a modification
/// @param    A Imm modification 
/// @return   None
///
	void addModification(SmfImmModifyOperation * i_modification);

///
/// Purpose:  Add a list of modifications
/// @param    i_modifications A list of Imm modifications
/// @return   None
///
	void addModifications(std::list < SmfImmOperation * >& i_modifications)
                { m_modificationList.insert(m_modificationList.end(),
                                     i_modifications.begin(),
                                     i_modifications.end()); }

///
/// Purpose:  Get modifications
/// @param    None
/// @return   A list of pointers to SmfImmOperation objects
///
	std::list < SmfImmOperation * >& getModifications();

/// Purpose:  Add an Imm operation (add/remove/modify)
/// @param    i_immoperation A Imm operation (free'ed by this object!)
/// @return   None
///
	void addImmOperation(SmfImmOperation* i_immoperation);

///
/// Purpose:  Set the node where sw should be added/removed (rolling)
/// @param    A DN to an AMF node 
/// @return   None
///
	void setSwNode(const std::string & i_swNode);

///
/// Purpose:  Add a node where sw should be added/removed (single step)
/// @param    A DN to an AMF node 
/// @return   None
///
	void addSwNode(const std::string& i_swNode);

///
/// Purpose:  Remove duplicate nodes in list where sw should be added/removed (single step)
/// @param    None
/// @return   None
///
	void removeSwNodeListDuplicates();

///
/// Purpose:  Get the node where sw should be added/removed (rolling)
/// @param     
/// @return   A DN to an AMF node
///
	const std::string & getSwNode();

///
/// Purpose:  Get the list of nodes where sw should be added/removed (single step)
/// @param     
/// @return   A list of DNs to AMF nodes
///
	const std::list<std::string>  & getSwNodeList();
  
///
/// Purpose:  Get the list of affected nodes (single step)
/// @param     
/// @return   A list of DNs to AMF nodes
///
	const std::list<std::string>  & getSsAffectedNodeList();

///
/// Purpose:  Set type of step
/// @param    type 
/// @return   None
///
	void setStepType(SmfStepType* i_type);

///
/// Purpose:  Get type of step
/// @param     
/// @return   type
///
	SmfStepType* getStepType();

///
/// Purpose:  Offline install new bundles
/// @param    -
/// @return   true on success else false
///
	bool offlineInstallNewBundles();

///
/// Purpose:  Online install new bundles
/// @param    -
/// @return   true on success else false
///
	bool onlineInstallNewBundles();

///
/// Purpose:  Offline remove old bundles
/// @param    - 
/// @return   true on success else false
///
	bool offlineRemoveOldBundles();

///
/// Purpose:  Online remove old bundles
/// @param    - 
/// @return   true on success else false
///
	bool onlineRemoveOldBundles();

///
/// Purpose:  Offline install old bundles
/// @param    -
/// @return   true on success else false
///
	bool offlineInstallOldBundles();

///
/// Purpose:  Online install old bundles
/// @param    -
/// @return   true on success else false
///
	bool onlineInstallOldBundles();

///
/// Purpose:  Offline remove new bundles
/// @param    - 
/// @return   true on success else false
///
	bool offlineRemoveNewBundles();

///
/// Purpose:  Online remove bundles
/// @param    - 
/// @return   true on success else false
///
	bool onlineRemoveNewBundles();

///
/// Purpose:  Online remove bundles according to user supplied list
/// @param    on what node to offline remove bundles 
/// @param    list od bundle references 
/// @return   true on success else false
///
	bool onlineRemoveBundlesUserList(const std::string & i_node, const std::list < SmfBundleRef > &i_bundleList);

///
/// Purpose:  Lock deactivation units 
/// @param    - 
/// @return   true on success else false
///
	bool lockDeactivationUnits();

///
/// Purpose:  Unlock deactivation units 
/// @param    - 
/// @return   true on success else false
///
	bool unlockDeactivationUnits();

///
/// Purpose:  Terminate deactivation units 
/// @param    - 
/// @return   true on success else false
///
	bool terminateDeactivationUnits();

///
/// Purpose:  Instantiate deactivation units 
/// @param    - 
/// @return   true on success else false
///
	bool instantiateDeactivationUnits();

///
/// Purpose:  Lock activation units 
/// @param    - 
/// @return   true on success else false
///
	bool lockActivationUnits();

///
/// Purpose:  Unlock activation units 
/// @param    - 
/// @return   true on success else false
///
	bool unlockActivationUnits();

///
/// Purpose:  Terminate activation units 
/// @param    - 
/// @return   true on success else false
///
	bool terminateActivationUnits();

///
/// Purpose:  Instantiate activation units 
/// @param    - 
/// @return   true on success else false
///
	bool instantiateActivationUnits();

///
/// Purpose:  restart activation units 
/// @param    - 
/// @return   true on success else false
///
	bool restartActivationUnits();

///
/// Purpose:  modifyInformationModel  
/// @param    - 
/// @return   true on success else false
///
	SaAisErrorT modifyInformationModel();

///
/// Purpose:  reverseInformationModel  
/// @param    - 
/// @return   true on success else false
///
	SaAisErrorT reverseInformationModel();

///
/// Purpose:  setMaintenanceStateActUnits  
/// @param    - 
/// @return   true on success else false
///
	bool setMaintenanceStateActUnits();

///
/// Purpose:  setMaintenanceStateActUnits  
/// @param    - 
/// @return   true on success else false
///
	bool setMaintenanceStateDeactUnits();

///
/// Purpose:  createSaAmfNodeSwBundlesNew 
/// @param    - 
/// @return   true on success else false
///
	bool createSaAmfNodeSwBundlesNew();

///
/// Purpose:  deleteSaAmfNodeSwBundlesNew 
/// @param    - 
/// @return   true on success else false
///
	bool deleteSaAmfNodeSwBundlesNew();

///
/// Purpose:  deleteSaAmfNodeSwBundlesOld 
/// @param    - 
/// @return   true on success else false
///
	bool deleteSaAmfNodeSwBundlesOld();

///
/// Purpose:  createSaAmfNodeSwBundlesOld 
/// @param    - 
/// @return   true on success else false
///
	bool createSaAmfNodeSwBundlesOld();


///
/// Purpose:  calculateSingleStepNodes, calculate the nodes to e.g. install a sw bundle.
/// @param    i_plmExecEnvList, the list of specified PLM exec env for the item e.g. bundle
/// @param    o_nodelist, the resulting list.
/// @return   true on success else false
///
	bool calculateSingleStepNodes(const std::list<SmfPlmExecEnv> & i_plmExecEnvList,
				      std::list<std::string>& o_nodelist);

///
/// Purpose:  setSwitchOver  
/// @param    i_switchover true if switch over ordered 
/// @return   true on success else false
///
	void setSwitchOver(bool i_switchover);

///
/// Purpose:  getSwitchOver  
/// @param    - 
/// @return   true if switch over ordered
///
	bool getSwitchOver();

///
/// Purpose:  calculateStepType  
/// @param    none
/// @return   SA_AIS_OK if calculation went OK 
///
	SaAisErrorT calculateStepType();

///
/// Purpose:  calculateStepTypeForMergedSingle
/// @param    none
/// @return   SA_AIS_OK if calculation went OK
///
	SaAisErrorT calculateStepTypeForMergedSingle();

  ///
/// Purpose:  isCurrentNode  
/// @param    - 
/// @return   true if the calling component executes on the given AMF node, otherwise false.
///
	bool isCurrentNode(const std::string & i_amfNodeDN);

///
/// Purpose:  isSingleNodeSystem  
/// @param    bool& i_result, true if single node otherwise false 
/// @return   SaAisErrorT
///
	SaAisErrorT isSingleNodeSystem(bool& i_result);

///
/// Purpose:  setSingleStepRebootInfo(int i_rebootInfo)
/// @param    An int containing a SmfRebootT enum, specifying where to continue after a cluster reboot
/// @return   SaAisErrorT
///
	SaAisErrorT setSingleStepRebootInfo(int i_rebootInfo);

///
/// Purpose:  getSingleStepRebootInfo  
/// @param    An int containing a SmfRebootT enum, specifying where to continue after a cluster reboot
/// @return   SaAisErrorT
///
	SaAisErrorT getSingleStepRebootInfo(int* o_rebootInfo);

///
/// Purpose:  Reboots the whole cluster  
/// @param    None
/// @return   SaAisErrorT
///
	SaAisErrorT clusterReboot();

///
/// Purpose:  Save the IMM content to a persistant location (which is read at reboot) 
/// @param    None
/// @return   SaAisErrorT
///
	SaAisErrorT saveImmContent();

///
/// Purpose:  Call activation command on remote node
/// @param    -
/// @return   -
///
	bool callActivationCmd();

///
/// Purpose:  Change the step stste.  If i_onlyInternalState == false, the IMM step object is updated and 
///           a state change event is sent
/// @param    -
/// @return   A ptr to the step thread
///
	void changeState(const SmfStepState * i_state);

///
/// Purpose:  Reboot the SW node and wait until possible to communicate
/// @param    -
/// @return   Bool true if successful otherwise false
///
	bool nodeReboot();

///
/// Purpose:  Copy initial admin state of deactivation units to activation units
/// @param    -
/// @return   -
///
	void copyDuInitStateToAu();

	bool checkAndInvokeCallback (const std::list < SmfCallback * > &callback_list, unsigned int camp_phase);
///
/// Purpose:  Read the possibly defined cluster controller names
/// @param    None
/// @return   true on success else false
///
	bool readSmfClusterControllers();
///
/// Purpose:  Set step state
/// @param    The state
/// @return   None
///
	void setStepState(SaSmfStepStateT i_state);

        friend class SmfStepState;

	typedef enum {
		SMF_STEP_OFFLINE_INSTALL = 1,
		SMF_STEP_ONLINE_INSTALL  = 2,
		SMF_STEP_OFFLINE_REMOVE  = 3,
		SMF_STEP_ONLINE_REMOVE   = 4,
		SMF_ACTIVATE_ALL         = 5
	} SmfInstallRemoveT;

 private:

///
/// Purpose:  Call bundle script on remote node
/// @param    -
/// @return   -
///
	bool callBundleScript(SmfInstallRemoveT i_order, const std::list < SmfBundleRef > &i_bundleList,
			      const std::string & i_node);

///
/// Purpose:  Wait for all prarallell bundle cmd threads to answer 
/// @param    Reference to the list of nodes
/// @return   Bool true if successful, otherwise false.
///
	bool waitForBundleCmdResult(std::list<std::string>& i_swNodeList);

///
/// Purpose:  setMaintenanceState  
/// @param    -
/// @return   true on success else false
///
	bool setMaintenanceState(SmfActivationUnit& i_units);

///
/// Purpose:  Create/Delete NodeSwBundle objects.
///

	bool createOneSaAmfNodeSwBundle(
		const std::string& i_node,
		const SmfBundleRef& bundle);
	bool deleteOneSaAmfNodeSwBundle(
		const std::string & i_node,
		const SmfBundleRef& i_bundle);

///
/// Purpose:  Set the state in IMM step object and send state change notification
/// @param    -
/// @return   -
///
	void setImmStateAndSendNotification(SaSmfStepStateT i_state);

///
/// Purpose: Disables copy constructor
///
	 SmfUpgradeStep(const SmfUpgradeStep &);

///
/// Purpose: Disables assignment operator
///
	SmfUpgradeStep & operator=(const SmfUpgradeStep &);

	SmfStepState *m_state;	          // Pointer to current step state class
	SaSmfStepStateT m_stepState;      // The step state in IMM
	unsigned int m_maxRetry;	  // Max Retry
	unsigned int m_retryCount; 	  // Retry count
	unsigned int m_restartOption;	  // Restart option
        std::string m_rdn;
	std::string m_dn;
	SmfUpgradeProcedure *m_procedure; // The procedure that we are part of
	SmfActivationUnit m_activationUnit;
	SmfActivationUnit m_deactivationUnit;
	std::list < SmfBundleRef > m_swRemoveList;
	std::list < SmfBundleRef > m_swAddList;
	std::list < SmfImmOperation * >m_modificationList;
	std::string m_swNode;	             // The node where bundles should be added/removed
	std::list<std::string> m_swNodeList; // Same as "m_swNode" but for single-step
	std::list<std::string> m_ssAffectedNodeList; // Total list of affected nodes in a single-step
	SmfStepType* m_stepType;	     // Type of step
        bool     m_switchOver;               // Switchover executed 
};

//////////////////////////////////////////////////
//Class SmfNodeSwLoadThread
//Used for node sw update
//////////////////////////////////////////////////
class SmfNodeSwLoadThread {
 public:
	SmfNodeSwLoadThread(SmfUpgradeStep * i_step, std::string i_nodeName, SmfUpgradeStep::SmfInstallRemoveT i_order,
			    const std::list<SmfBundleRef> *i_bundleList=NULL);
	~SmfNodeSwLoadThread();
	int start(void);

 private:

	void main(void);
	int init(void);

	static void main(NCSCONTEXT info);

	NCSCONTEXT        m_task_hdl;
	SmfUpgradeStep *  m_step;
	std::string       m_amfNode;
	SmfUpgradeStep::SmfInstallRemoveT m_order;
	sem_t             m_semaphore;
	const std::list < SmfBundleRef > *m_bundleList;
};

//==============================================================================
// Operates on the units of an external unitNameAndState list that is provided
// when an object of this class is created. Nodes admin state is handled in
// parallel using a node group.
//
// The unit initial admin state state is saved in the provided list (initState)
// when locking.
// When unlocking, unlock is done until initial admin state is reached if keep
// flag is true else end state will be unlocked regardless of initial state.
// Units handled can be Nodes or SUs
//
// All methods return bool false if the admin operation fail.
// An error log is written to inform why the operation failed.
// All IMM handles needed are created by the constructor. If this fail an
// error log is created and abort is done.
//
//==============================================================================
class SmfAdminOperation {
    public:
	explicit SmfAdminOperation(std::list <unitNameAndState> *i_allUnits);
	~SmfAdminOperation();

	bool lock();
	bool lock_in();
	bool unlock_in();
	bool unlock();
	bool restart();

    private:
	bool initNodeGroupOm();
        bool becomeAdminOwnerOfAmfClusterObj();
        void finalizeNodeGroupOm();
	bool isRestartError(SaAisErrorT ais_rc);

	// Result in m_nodeList and m_suList
	void createNodeAndSULockLists(SaAmfAdminStateT adminState);
	void createNodeAndSUUnlockLists(SaAmfAdminStateT adminState);
	void createUnitLists(SaAmfAdminStateT adminState, bool checkInitState);
	// Updates list pointed to by m_allUnits
	bool saveInitAndCurrentStateForAllUnits();
	bool saveCurrentStateForAllUnits();

	// Using m_nodeList
	bool createNodeGroup(SaAmfAdminStateT i_initState);

	bool deleteNodeGroup();

	bool changeNodeGroupAdminState(SaAmfAdminStateT fromState,
						SaAmfAdminOperationIdT toState);
	bool nodeGroupAdminOperation(SaAmfAdminOperationIdT adminState);
	// Using m_suList
	bool adminOperationSerialized(SaAmfAdminOperationIdT adminState,
				const std::list <unitNameAndState> &i_nodeList);
	bool adminOperation(SaAmfAdminOperationIdT adminState,
					     const std::string &unitName);

	SaAmfAdminStateT getAdminState(const std::string&  i_unit);
	bool setNodeGroupParentDn();

	std::list <unitNameAndState> *m_allUnits;
	std::list <unitNameAndState> m_suList;
	std::list <unitNameAndState> m_nodeList;

	const SaVersionT m_immVersion {'A', 2, 1};

	std::string m_nodeGroupParentDn;

	SaImmHandleT m_omHandle {0};
	SaImmAdminOwnerHandleT m_ownerHandle {0};
	SaImmCcbHandleT m_ccbHandle {0};
	SaImmAccessorHandleT m_accessorHandle {0};
	// Indicate if creation was succesful
	bool m_creation_fail {false};

	// This variable will contain the AIS return code after a calling a
	// method using OpenSAF API. If this variable does not contain
	// SA_AIS_OK the called method has failed.
	//
	SaAisErrorT m_errno {SA_AIS_OK};

	// Count instances of this class
	static std::atomic<unsigned int> m_next_instance_number;
	bool m_smfKeepDuState {false};
	unsigned int m_instance_number {0};
	std::string m_instanceNodeGroupName {""};
        std::string m_admin_owner_name {""};

	DELETE_COPY_AND_MOVE_OPERATORS(SmfAdminOperation);
};
#endif  // SMF_SMFD_SMFUPGRADESTEP_H_
