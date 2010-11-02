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

#ifndef SMFUPGRADEPROCEDURE_HH
#define SMFUPGRADEPROCEDURE_HH

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <semaphore.h>
#include <ncsgl_defs.h>

#include <string>
#include <vector>
#include <list>

#include <saSmf.h>
#include <saImmOi.h>
#include "SmfImmOperation.hh"

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
#define m_PROCEDURE_TASK_PRI (5)

typedef enum {
	SMF_AU_NONE = 1,
	SMF_AU_AMF_NODE = 2,
	SMF_AU_SU_COMP = 3
} SmfAuT;

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
	void execute();

///
/// Purpose:  Execute the procedure init actions
/// @param    None
/// @return   None
///
	void executeInit();

///
/// Purpose:  Execute the procedure steps
/// @param    None
/// @return   None
///
	void executeStep();

///
/// Purpose:  Execute the procedure wrapup actions
/// @param    None
/// @return   None
///
	void executeWrapup();

///
/// Purpose:  Rollback the upgrade procedure
/// @param    None
/// @return   None
///
	void rollback();

///
/// Purpose: Suspend the upgrade procedure
/// @param    None
/// @return   None
///
	void suspend();

///
/// Purpose: Commit the upgrade procedure
/// @param    None
/// @return   None
///
	void commit();

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

///
/// Purpose:  Calculate upgrade steps for rolling upgrade
/// @param    None.
/// @return   None.
///
	bool calculateRollingSteps(SmfRollingUpgrade * i_rollingUpgrade);

///
/// Purpose:  Calculate upgrade steps for single-step upgrade
/// @param    Upgrade object.
/// @return   None.
///
	bool calculateSingleStep(SmfSinglestepUpgrade* i_upgrade);

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
                                             std::list < std::string >& o_actDeactUnits,
					     std::list<std::string>* o_nodeList = NULL);

	bool calcActivationUnitsFromTemplateSingleStep(
		SmfEntity const& i_entity,
		std::list<std::string>& o_actDeactUnits,
		std::list<std::string>& o_nodeList);
///
/// Purpose:  Fetch node for a SU or Component DN
/// @param    i_dn The DN for the SU or component
/// @return   The DN of the node hosting the input DN
///
        std::string getNodeForCompSu(const std::string & i_objectDn);

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
                                  SmfAuT i_auType);

///
/// Purpose:  Add IMM step modifications for AU of type node
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   True if successful otherwise false
///
	bool addStepModificationsNode(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
				      const std::list < SmfImmModifyOperation * >&i_modificationList);

///
/// Purpose:  Add IMM step modifications for AU of type SU
/// @param    i_newStep A pointer to a SmfUpgradeStep object.
/// @param    i_parentType A pointer to a SmfParentType object.
/// @param    i_modificationList A list of pointers to SmfImmModifyOperation objects.
/// @return   None.
///
	bool addStepModificationsSuComp(SmfUpgradeStep * i_newStep, const SmfParentType * i_parentType,
				    const std::list < SmfImmModifyOperation * >&i_modificationList);

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
/// Purpose:  Register the DNs of the added, removed or modified objects in the step
/// @param    i_step The SmfUpgradeStep from where to read the modifications
/// @param    io_smfAttr The attribute where to write the DNs
/// @return   True if sucessful otherwise false.
///
	bool setEntitiesToAddRemMod(SmfUpgradeStep * i_step, SmfImmAttribute* io_smfEntityToAddRemove);

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
	bool getActDeactUnitsAndNodes(const std::string &i_dn, std::string& io_unit, std::string& io_node);

	friend class SmfProcState;
	friend class SmfProcStateInitial;
	friend class SmfProcStateExecuting;
	friend class SmfProcStateExecutionCompleted;
	friend class SmfProcStateSuspended;
	friend class SmfProcStateFailed;

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
        SaTimeT     m_time;     // The expected execution time
	int         m_execLevel;// The execution level of the upgrade procedure
        std::string m_dn;	// The DN of the upgrade procedure
	SmfUpgradeMethod *m_upgradeMethod;	              // Pointer to the upgrade method (rolling or synchronous)
        std::vector < SmfUpgradeAction * >m_procInitAction;   //Container of the procedure initiation commands
        std::vector < SmfUpgradeAction * >m_procWrapupAction; //Container of the procedure wrap up commands
        std::vector < SmfUpgradeStep * >m_procSteps;	      //Container of the procedure wrap up commands
	sem_t m_semaphore;
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

 private:

	void main(void);
	int init(void);

	static void main(NCSCONTEXT info);

	NCSCONTEXT  m_task_hdl;
	SmfUpgradeProcedure * m_proc;
	sem_t       m_semaphore;
};

#endif				// SMFUPGRADEPROCEDURE_HH
