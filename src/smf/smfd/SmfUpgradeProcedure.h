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

#ifndef SMF_SMFD_SMFUPGRADEPROCEDURE_H_
#define SMF_SMFD_SMFUPGRADEPROCEDURE_H_

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <semaphore.h>
#include "base/ncsgl_defs.h"

#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>

#include "smf/saf/saSmf.h"
#include "imm/saf/saImmOi.h"
#include "smf/smfd/SmfImmOperation.h"
#include "smf/smfd/SmfCampaignThread.h"
#include "smf/smfd/SmfCallback.h"

class SmfCallback;
class SmfUpgradeMethod;
class SmfProcedureThread;
class SmfProcState;
class SmfUpgradeStep;
class SmfUpgradeAction;
class SmfRollingUpgrade;
class SmfSinglestepUpgrade;
class SmfParentType;
class SmfTargetEntityTemplate;
class SmfEntity;

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */
#define m_PROCEDURE_TASKNAME "PROCEDURE"
#define m_PROCEDURE_STACKSIZE NCS_STACKSIZE_HUGE

typedef enum {
	SMF_AU_NONE = 1,
	SMF_AU_AMF_NODE = 2,
	SMF_AU_SU_COMP = 3
} SmfAuT;

struct objectInst {
	objectInst(std::string& i_node, 
		   std::string& i_sg, 
		   std::string& i_su, 
		   std::string& i_suType,
		   std::string& i_comp, 
		   std::string& i_compType) 
		: nodeDN(i_node), sgDN(i_sg),  suDN(i_su), suTypeDN(i_suType), compDN(i_comp), compTypeDN(i_compType){}
	std::string nodeDN; 
	std::string sgDN; 
	std::string suDN; 
	std::string suTypeDN; 
	std::string compDN; 
	std::string compTypeDN; 
};

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

///================================================================================
/// Class UpgradeProcedure
/// Purpose:
/// Comments:
///================================================================================

///
/// Purpose:
///
class SmfUpgradeProcedure {
 public:

///
/// Purpose:  Constructor
/// @param    None.
/// @return   None.
///
	SmfUpgradeProcedure();

///
/// Purpose:  Destructor
/// @param    None
/// @return   None
///
	~SmfUpgradeProcedure();

///
/// Purpose:  Init attributes from Imm object
/// @param    attrValues Imm attributes.
/// @return   SA_AIS_OK on OK else error code
///
	SaAisErrorT init(const SaImmAttrValuesT_2 ** attrValues);

///
/// Purpose:  Get current procedure state
/// @param    None.
/// @return   The state.
///
	SaSmfProcStateT getState() const;

///
/// Purpose:  Set current procedure state.
/// @param    i_state The SaSmfProcStateT to set.
/// @return   None.
///
	void setProcState(SaSmfProcStateT i_state);

///
/// Purpose:  Set the name of the procedure.
/// @param    i_name A string specifying the name of the procedure.
/// @return   None.
///
	void setProcName(std::string i_name);

///
/// Purpose:  Get the name of the procedure.
/// @param    None.
/// @return   A string specifying the name of the procedure.
///
	const std::string & getProcName();

///
/// Purpose:  Set the OI name of the procedure.
/// @param    i_oiName A string specifying the OI name of the procedure.
/// @return   None.
///
	void setProcOiName(const std::string &i_name);

///
/// Purpose:  Get the OI name of the procedure.
/// @param    None.
/// @return   A string specifying the OI name of the procedure.
///
	const std::string & getProcOiName();

///
/// Purpose:  Set the procedure execution level. Procedures executed in order low-->high number.
/// @param    i_level A string specifying the execution level.
/// @return   None.
///
	void setExecLevel(std::string i_level);

///
/// Purpose:  Get the execution level of the procedure
/// @param    Node
/// @return   An integer specifying the execution level.
///
	const int &getExecLevel();

///
/// Purpose:  Set the estimated procedure time.
/// @param    i_time A SaTimeT specifying the time.
/// @return   None.
///
	void setProcedurePeriod(SaTimeT i_time);

///
/// Purpose:  Get the estimated procedure time.
/// @param    Node
/// @return   An integer SaTimeT specifying the estimated procedure time.
///
	const SaTimeT &getProcedurePeriod();

///
/// Purpose:  Set the DN of the procedure
/// @param    i_dn A std::string containing the DN of the procedure object in IMM.
/// @return   -
///
	void setDn(const std::string & i_dn);

///
/// Purpose:  Get the DN of the procedure
/// @param    None
/// @return   A std::string containing the DN of the procedure object in IMM.
///
	const std::string & getDn();

///
/// Purpose:  Get the name of the procedure
/// @param    None
/// @return   A std::string containing the name of the procedure.
///
	const std::string & getName() { return m_name; };

///
/// Purpose:  Set the state in IMM procedure object and send state change notification
/// @param    i_state The SaSmfProcStateT to set.
/// @return   None.
///
	void setImmStateAndSendNotification(SaSmfProcStateT i_state);

///
/// Purpose:  Set the upgrade method
/// @param    i_method A pointer to the upgrade method object.
/// @return   None
///
	void setUpgradeMethod(SmfUpgradeMethod * i_method);

///
/// Purpose:  Get the upgrade method
/// @param    None
/// @return   A pointer to the upgrade method object.
///
	SmfUpgradeMethod *getUpgradeMethod(void);

///
/// Purpose:  Add an upgrade procedure init action
/// @param    i_action A string containing a command
/// @return   None
///
	void addProcInitAction(SmfUpgradeAction * i_action);

///
/// Purpose:  Add an upgrade procedure wrapup action
/// @param    i_action A string containing a command
/// @return   None
///
	void addProcWrapupAction(SmfUpgradeAction * i_action);

///
/// Purpose:  Execute the upgrade procedure
/// @param    None
/// @return   None
///
	SmfProcResultT execute();

///
/// Purpose:  Execute the procedure steps
/// @param    None
/// @return   None
///
	SmfProcResultT executeStep();

///
/// Purpose:  Rollback the procedure steps
/// @param    None
/// @return   None
///
	SmfProcResultT rollbackStep();

///
/// Purpose:  Rollback the upgrade procedure
/// @param    None
/// @return   None
///
	SmfProcResultT rollback();

///
/// Purpose: Suspend the upgrade procedure
/// @param    None
/// @return   None
///
	SmfProcResultT suspend();

///
/// Purpose: Commit the upgrade procedure
/// @param    None
/// @return   None
///
	SmfProcResultT commit();

///
/// Purpose:  Set the procedure thread
/// @param    i_procThread A ptr to the procedure thread
/// @return   None
///
	void setProcThread(SmfProcedureThread * i_procThread);

///
/// Purpose:  Get the procedure thread
/// @param    None.
/// @return   A ptr to the procedure thread
///
	SmfProcedureThread *getProcThread();

///
/// Purpose:  Switchover smfnd control to other controller
/// @param    None.
/// @return   None.
///
        void switchOver();

///
/// Purpose:  Calculate upgrade steps
/// @param    -
/// @return   True if successful otherwise false
///
	bool calculateSteps();
	bool calculateSteps(std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Calculate upgrade steps for rolling upgrade
/// @param    None.
/// @return   None.
///
	bool calculateRollingSteps(SmfRollingUpgrade * i_rollingUpgrade,
				   std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Calculate upgrade steps for single-step upgrade
/// @param    Upgrade object.
/// @return   None.
///
	bool calculateSingleStep(SmfSinglestepUpgrade* i_upgrade,
				 std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Merge procedure steps into a single-step
/// @param    i_proc A SmfUpgradeProcedure* pointing to the procedure
/// @return   None.
///
        bool mergeStepIntoSingleStep(SmfUpgradeProcedure * i_proc, SmfUpgradeStep *i_newStep = 0);

///
/// Purpose:  Merge merge existing single-step bundle ref into a new single-step
/// @param    io_newStep a SmfUpgradeStep * pointion to the new step
/// @param    i_oldStep a SmfUpgradeStep * pointion to the step to copy from
/// @return   None.
///
        bool mergeBundleRefSingleStepToSingleStep(SmfUpgradeStep * io_newStep,
                                                 SmfUpgradeStep * i_oldStep);

///
/// Purpose:  Merge merge existing step (rolling) bundle ref into a new single-step
/// @param    io_newStep a SmfUpgradeStep * pointion to the new step
/// @param    i_oldStep a SmfUpgradeStep * pointion to the step to copy from
/// @return   None.
///
	bool mergeBundleRefRollingToSingleStep(SmfUpgradeStep * io_newStep,
                                               SmfUpgradeStep * i_oldStep);

///
/// Purpose:  Calculate list of nodes from objectDn
/// @param    i_objectDn A DN to a cluster or node group.
/// @param    o_nodeList The resulting list of nodes.
/// @return   True if successful otherwise false
///
	bool calculateNodeList(const std::string & i_objectDn, std::list < std::string > &o_nodeList);

///
/// Purpose:  Calculate activation units from template
/// @param    i_parentType The parent/type pair.
/// @param    i_nodeList The list of nodes the procedure shall operate on.
/// @param    o_actDeactUnits The resulting list of act/deact units.
/// @param    o_nodeList If this is non-NULL the i_nodeList is ignored, instead all nodes where the
///              units are hosted are added to the map. This is used for single-step only.
/// @return   True if successful otherwise false
///
        bool calcActivationUnitsFromTemplate(SmfParentType * i_parentType, 
                                             const std::list < std::string >& i_nodeList,
					     std::multimap<std::string, objectInst> &i_objects,
                                             std::list < std::string >& o_actDeactUnits,
					     std::list<std::string>* o_nodeList = NULL);

	bool calcActivationUnitsFromTemplateSingleStep(
		SmfEntity const& i_entity,
		std::multimap<std::string, objectInst> &i_objects,
		std::list<std::string>& o_actDeactUnits,
		std::list<std::string>& o_nodeList);
///
/// Purpose:  Fetch node for a SU or Component DN
/// @param    i_dn The DN for the SU or component
/// @return   The DN of the node hosting the input DN
///
        std::string getNodeForCompSu(const std::string & i_objectDn, std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Fetch callbacks from the upgrade method
/// @param    i_upgradeMethod A pointer to the upgrade method to copy from
/// @return   None
///
        void getCallbackList(const SmfUpgradeMethod* i_upgradeMethod);

///
/// Purpose:  Add an proc step
/// @param    i_step A pointer to the SmfUpgradeStep object to be added.
/// @return   None.
///
	void addProcStep(SmfUpgradeStep * i_step);

///
/// Purpose:  Add IMM step modifications
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_targetEntityTemplate A list of pointers to SmfTargetEntityTemplate objects.
/// @return   True if successful otherwise false
///
	bool addStepModifications(SmfUpgradeStep * i_newStep,
				  const std::list < SmfTargetEntityTemplate * >&i_targetEntityTemplate,
                                  SmfAuT i_auType,
				  std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Add IMM step modifications for AU of type node
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationsNode(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
				      const std::list < SmfImmModifyOperation * >&i_modificationList,
				      std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Add IMM step modifications for AU of type SU
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   None.
///
	bool addStepModificationsSuOrComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
					  const std::list < SmfImmModifyOperation * >&i_modificationList,
					  std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Add IMM modification list to step
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationList(SmfUpgradeStep * i_newStep, const std::string & i_dn,
				     const std::list < SmfImmModifyOperation * >&i_modificationList);

///
/// Purpose:  Add IMM step modifications for SU type
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationsSu(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
				    const std::list < SmfImmModifyOperation * >&i_modificationList,
				    std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Add IMM step modifications for Comp type
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationsComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
				      const std::list < SmfImmModifyOperation * >&i_modificationList,
				      std::multimap<std::string, objectInst> &i_objects);
///
/// Purpose:  Add IMM step modifications for children (SUs) to given parent
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationsParentOnly(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
					    const std::list < SmfImmModifyOperation * >&i_modificationList,
					    std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Create imm step objects for the calculated steps
/// @param    None.
/// @return   True if successful otherwise false
///
	bool createImmSteps();

///
/// Purpose:  Create an imm step
/// @param    -
/// @return   -
///
	SaAisErrorT createImmStep(SmfUpgradeStep * i_step);

///
/// Purpose:  Get procedure steps
/// @param    -
/// @return   -
///
	SaAisErrorT getImmSteps();

///
/// Purpose:  Get procedure steps for rolling upgrade
/// @param    -
/// @return   -
///
	SaAisErrorT getImmStepsRolling();

///
/// Purpose:  Get procedure steps for Single step upgrade
/// @param    -
/// @return   -
///
	SaAisErrorT getImmStepsSingleStep();

///
/// Purpose:  Get procedure steps for merged Single step upgrade
/// @param    -
/// @return   -
///
	SaAisErrorT getImmStepsMergedSingleStep();

///
/// Purpose:  Read campaign data from IMM and store the information in i_newStep
/// @param    -
/// @return   -
///
	SaAisErrorT readCampaignImmModel(SmfUpgradeStep * i_newStep);

  ///
/// Purpose:  Create lists of SmfBundleRef from a single step campaign IMM model.
/// @param    -
/// @return   -
///
	SaAisErrorT bundleRefFromSsCampaignImmModel(SmfUpgradeStep * i_newStep);

///
/// Purpose:  Register the DNs of the added, removed or modified objects in the step
/// @param    i_step The SmfUpgradeStep from where to read the modifications
/// @param    io_smfAttr The attribute where to write the DNs
/// @return   True if sucessful otherwise false.
///
	bool setEntitiesToAddRemMod(SmfUpgradeStep * i_step, SmfImmAttribute* io_smfEntityToAddRemove);

///
/// Purpose:  Get the list of upgrade steps
/// @param    -
/// @return   The list of upgrade steps.
///
        std::vector < SmfUpgradeStep * >& getProcSteps() { return m_procSteps; }

///
/// Purpose:  Add the list of upgrade steps
/// @param    The list of upgrade steps to add
/// @return   none
///
        void addProcSteps(const std::vector < SmfUpgradeStep * >& i_procSteps)
                { m_procSteps.insert(m_procSteps.end(),
                                     i_procSteps.begin(),
                                     i_procSteps.end()); }
///
/// Purpose:  Get the list of init actions
/// @param    -
/// @return   The list of init actions.
///
        const std::vector < SmfUpgradeAction * >& getInitActions() { return m_procInitAction; }

///
/// Purpose:  Add a list of init actions
/// @param    The list of init actions to add
/// @return   none
///
        void addInitActions(const std::vector < SmfUpgradeAction * >& i_initActions)
                { m_procInitAction.insert(m_procInitAction.end(),
                                          i_initActions.begin(),
                                          i_initActions.end()); }

///
/// Purpose:  Get the list of wrapup actions
/// @param    -
/// @return   The list of wrapup actions.
///
        const std::vector < SmfUpgradeAction * >& getWrapupActions() { return m_procWrapupAction; }

        void clearWrapupActions() { m_procWrapupAction.clear(); }

///
/// Purpose:  Add the list of wrapup actions
/// @param    The list of wrapup actions to add
/// @return   none
///
        void  addWrapupActions(const std::vector < SmfUpgradeAction * >& i_wrapupActions)
                { m_procWrapupAction.insert(m_procWrapupAction.end(),
                                            i_wrapupActions.begin(),
                                            i_wrapupActions.end()); }

///
/// Purpose:  Check if the component pointed out by DN is restartable
/// @param    i_compDN The DN of the components
/// @return   True if restartable otherwise false.
///
	bool isCompRestartable(const std::string &i_compDN);

///
/// Purpose:  Analyze the act/deact unit and resturn the resulting act/deact unit and node
/// @param    i_dn The DN of the act/deact unit
/// @param    io_units The resulting act/deact unit
/// @param    io_nodes The hosting node
/// @return   True if restartable otherwise false.
///
	bool getActDeactUnitsAndNodes(const std::string &i_dn, std::string& io_unit, 
				      std::string& io_node,
				      std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose:  Get the list of callbacks beforeLock
/// @param    -
/// @return   The list of callbacks.
///
        std::list < SmfCallback * >& getCbksBeforeLock() { return m_beforeLock; }

///
/// Purpose:  Add the list of before lock callbacks
/// @param    The list of  before lock callbacks to add
/// @return   none
///
        void  addCbksBeforeLock(const std::list < SmfCallback * >& i_beforeLock)
                { m_beforeLock.insert(m_beforeLock.end(),
                                      i_beforeLock.begin(),
                                      i_beforeLock.end()); }

///
/// Purpose:  Get the list of callbacks beforeTerm
/// @param    -
/// @return   The list of callbacks.
///
        std::list < SmfCallback * >& getCbksBeforeTerm() { return m_beforeTerm; }

///
/// Purpose:  Add the list of before term callbacks
/// @param    The list of  before term callbacks to add
/// @return   none
///
        void  addCbksBeforeTerm(const std::list < SmfCallback * >& i_beforeTerm)
                { m_beforeTerm.insert(m_beforeTerm.end(),
                                      i_beforeTerm.begin(),
                                      i_beforeTerm.end()); }

///
/// Purpose:  Get the list of callbacks afterImmModify
/// @param    -
/// @return   The list of callbacks.
///
        std::list < SmfCallback * >& getCbksAfterImmModify() { return m_afterImmModify; }

///
/// Purpose:  Add the list of before term callbacks
/// @param    The list of before term callbacks to add
/// @return   none
///
        void  addCbksAfterImmModify(const std::list < SmfCallback * >& i_afterImmModify)
                { m_afterImmModify.insert(m_afterImmModify.end(),
                                      i_afterImmModify.begin(),
                                      i_afterImmModify.end()); }

///
/// Purpose:  Get the list of callbacks afterInstall
/// @param    -
/// @return   The list of callbacks.
///
        std::list < SmfCallback * >& getCbksAfterInstantiate() { return m_afterInstantiate; }

///
/// Purpose:  Add the list of after instantiate callbacks
/// @param    The list of after instantiate callbacks to add
/// @return   none
///
        void  addCbksAfterInstantiate(const std::list < SmfCallback * >& i_afterInstantiate)
                { m_afterInstantiate.insert(m_afterInstantiate.end(),
                                      i_afterInstantiate.begin(),
                                      i_afterInstantiate.end()); }

///
/// Purpose:  Get the list of callbacks afterUnlock
/// @param    -
/// @return   The list of callbacks.
///
        std::list < SmfCallback * >& getCbksAfterUnlock() { return m_afterUnlock; }

///
/// Purpose:  Add the list of after unlock callbacks
/// @param    The list of after unlock callbacks to add
/// @return   none
///
        void  addCbksAfterUnlock(const std::list < SmfCallback * >& i_afterUnlock)
                { m_afterUnlock.insert(m_afterUnlock.end(),
                                      i_afterUnlock.begin(),
                                      i_afterUnlock.end()); }

///
/// Purpose:  Mark the procedure as merged or not
/// @param    i_state a bool indicating if merged
/// @return   -
///
        void setIsMergedProcedure(bool i_state)
                { m_isMergedProcedure = i_state; }

///
/// Purpose:  Reset the object counter of upgrade procedures
/// @param    -
/// @return   -
///
        static void resetProcCounter();

	friend class SmfProcState;
	friend class SmfCampStateSuspendingExec;

///
/// Purpose:  Get iformation from AMF config in IMM about Components, SUs and nodes needed for upgrade 
/// @param    A reference to a std::multimap<std::string, objectInst>
/// @return   Bool true if successful otherwise false.
///
	bool getImmComponentInfo(std::multimap<std::string, objectInst> &i_objects);

///
/// Purpose: When merging with SMF_BALANCED_MODE we need to keep track of which balanced group procedures belong to.
///
        const std::vector<std::string>& getBalancedGroup() { return m_balancedGroup; }

///
/// Purpose: When merging with SMF_BALANCED_MODE we need to keep track of which balanced group procedures belong to.
///
        void setBalancedGroup(std::vector<std::string> group) { m_balancedGroup = group; }

 private:


///
/// Purpose:  Change the procedure stste. If i_onlyInternalState == false, the IMM procedure object is updated and 
///           a state change event is sent
/// @param    -
/// @return   A ptr to the procedure thread
///
	void changeState(const SmfProcState * i_state);

///
/// Purpose: Disables copy constructor
        ///
        SmfUpgradeProcedure(const SmfUpgradeProcedure &);

///
        /// Purpose: Disables assignment operator
        ///
        SmfUpgradeProcedure & operator=(const SmfUpgradeProcedure &);

	SmfProcState *m_state;       // Pointer to current procedure state class
	SaSmfProcStateT m_procState; // The procedure state in IMM
	SmfProcedureThread *m_procedureThread;	// The thread object we are executing in
        std::string m_name;	// The name of the upgrade procedure
        std::string m_oiName;	// The OI name of the upgrade procedure
        static unsigned long s_procCounter;	// This is the object counter for upgrade procedures
        SaTimeT     m_time;     // The expected execution time
	int         m_execLevel;// The execution level of the upgrade procedure
        std::string m_dn;	// The DN of the upgrade procedure
	SmfUpgradeMethod *m_upgradeMethod;	              // Pointer to the upgrade method (rolling or synchronous)
        std::vector < SmfUpgradeAction * >m_procInitAction;   //Container of the procedure initiation commands
        std::vector < SmfUpgradeAction * >m_procWrapupAction; //Container of the procedure wrap up commands
        std::vector < SmfUpgradeStep * >m_procSteps;	      //Container of the procedure upgrade steps

        std::list < SmfCallback * >m_beforeLock;   //Container of the procedure callbacks to be invoked onstep, ataction
        std::list < SmfCallback * >m_beforeTerm;   //Container of the procedure callbacks to be invoked onstep, ataction
        std::list < SmfCallback * >m_afterImmModify;   //Container of the procedure callbacks to be invoked onstep, ataction
        std::list < SmfCallback * >m_afterInstantiate;   //Container of the procedure callbacks to be invoked onstep, ataction
        std::list < SmfCallback * >m_afterUnlock;   //Container of the procedure callbacks to be invoked onstep, ataction
	sem_t m_semaphore;
        bool m_isMergedProcedure;
        std::vector<std::string> m_balancedGroup;
};

//////////////////////////////////////////////////
//Class SmfSwapThread
//Used for just one task, the SI_SWAP operation
//////////////////////////////////////////////////
class SmfSwapThread {
 public:
	explicit SmfSwapThread(SmfUpgradeProcedure * i_proc);
	~SmfSwapThread();
	int start(void);

	static bool running(void);
	static void setProc(SmfUpgradeProcedure *);

 private:
	static SmfSwapThread *me;
	static std::mutex m_mutex;

	void main(void);
	int init(void);

	static void main(NCSCONTEXT info);

	NCSCONTEXT  m_task_hdl;
	SmfUpgradeProcedure * m_proc;
	sem_t       m_semaphore;
};

#endif  // SMF_SMFD_SMFUPGRADEPROCEDURE_H_
