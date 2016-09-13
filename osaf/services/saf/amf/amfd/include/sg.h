/*      -*- OpenSAF  -*-            
 *
 * (C) Copyright 2008 The OpenSAF Foundation
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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

..............................................................................

  DESCRIPTION:

  This module is the include file for handling Availability Directors 
  Service group structure.
  
******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef AVD_SG_H
#define AVD_SG_H

#include <saAmf.h>
#include <cb.h>
#include <def.h>
#include <sgtype.h>
#include <amf_defs.h>
#include <amf_d2nmsg.h>
#include "node.h"
#include <list>

class AVD_SU;
class AVD_SI;
class AVD_APP;
class AVD_AMF_NG;

/* The valid SG FSM states. */
typedef enum {
	AVD_SG_FSM_STABLE = 0,
	AVD_SG_FSM_SG_REALIGN,
	AVD_SG_FSM_SU_OPER,
	AVD_SG_FSM_SI_OPER,
	AVD_SG_FSM_SG_ADMIN
} AVD_SG_FSM_STATE;

/* The structure used for containing the list of SUs
 * undergoing operations on them.
 */
typedef std::list<AVD_SU*> AVD_SG_OPER; // we don't own these pointers

/**
 * Service group abstract base class
 */
class AVD_SG {
public:
	AVD_SG();
	virtual ~AVD_SG() {};

	std::string name;		/* the service group DN used as the index.
				 * Checkpointing - Sent as a one time update.
				 */

	bool saAmfSGAutoRepair_configured; /* True when user configures saAmfSGAutoRepair else false */
   /******************** B.04 model *************************************************/
	std::string saAmfSGType;	/* Network order. */
	std::string saAmfSGSuHostNodeGroup;	/* Network order. */
	SaBoolT saAmfSGAutoRepair;
	SaBoolT saAmfSGAutoAdjust;

	SaUint32T saAmfSGNumPrefActiveSUs;	/* the N value in the redundancy model, where
						 * applicable.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefStandbySUs;	/* the M value in the redundancy model, where
						 * applicable.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefInserviceSUs;	/* The preferred number of in service
						 * SUs.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGNumPrefAssignedSUs;	/* The number of active SU assignments
						 * an SI can have.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGMaxActiveSIsperSU;	/* The maximum number of active 
						 * instance of Sis that can be 
						 * assigned to an SU.
						 * Checkpointing - Sent as a one time update.
						 */

	SaUint32T saAmfSGMaxStandbySIsperSU;	/* The maximum number of standby 
						 * instance of Sis that can be
						 * assigned to an SU.
						 * Checkpointing - Sent as a one time update.
						 */
	SaTimeT saAmfSGAutoAdjustProb;
	SaTimeT saAmfSGCompRestartProb;	/* component restart probation period
					 * Checkpointing - Sent as a one time update.
					 */
	SaUint32T saAmfSGCompRestartMax;	/* max component restart count 
						 * Checkpointing - Sent as a one time update.
						 */
	SaTimeT saAmfSGSuRestartProb;	/* SU restart probation period 
					 * Checkpointing - Sent as a one time update.
					 */

	SaUint32T saAmfSGSuRestartMax;	/* max SU restart count
					 * Checkpointing - Sent as a one time update.
					 */

	SaAmfAdminStateT saAmfSGAdminState;	/* admin state of the group.
						 * Checkpointing - Updated independently.
						 */

	SaUint32T saAmfSGNumCurrAssignedSUs;	/* Num of Sus that have been assigned a SI 
						 * Checkpointing - Updated independently.
						 */

	SaUint32T saAmfSGNumCurrInstantiatedSpareSUs;	/* Num of Sus that are in service but 
							 * not yet assigned a SI.
							 * Checkpointing - Updated independently.
							 */

	SaUint32T saAmfSGNumCurrNonInstantiatedSpareSUs;	/* Num of Sus that have been 
								 * configured but not yet instantiated.
								 * Checkpointing - Updated independently.
								 */

   /******************** B.04 model *************************************************/

	SaAdjustState adjust_state;	/* Field to re adjust the SG.
					 * Checkpointing - Updated independently.
					 */

	bool sg_ncs_spec;	/* This is set to true if the SG
				 * is a NCS specific SG.
				 * Checkpointing - Sent as a one time update.
				 */

	AVD_SG_FSM_STATE sg_fsm_state;	/* The different flows of the SU SI
					 * transitions for the SUs and SIs
					 * in the SG is orchestrated based on
					 * this FSM.
					 * Checkpointing - Updated independently.
					 */

	AVD_SI *admin_si;	/* Applicable when sg_fsm_state has
				 * AVD_SG_FSM_SI_OPER.It will contain
				 * the SI undergoing admin
				 * operation.
				 */

	AVD_SG_OPER su_oper_list;	/* The list of SUs that have operations
					 * happening on them used in parallel
					 * with sg_fsm_state.
					 * Checkpointing - Sent as a one time update.
					 */
	SaAmfRedundancyModelT sg_redundancy_model;	/* the redundancy model in the service group 
							 * see sec 4.7 for values
							 * Checkpointing - Sent as a one time update.
							 */

	/* the list of service units in this
	 * group in the descending order of
	 * the rank.
	 */
	std::vector<AVD_SU*> list_of_su;
	AVD_SU* first_su();
	AVD_SU* get_su_by_name(SaNameT su_name);
	/* the list of service instances in 
	 * this group in the descending order 
	 * of the rank.
	 */
	std::vector<AVD_SI*> list_of_si;
	SaInvocationT adminOp_invocationId;
	SaAmfAdminOperationIdT adminOp;

	AVD_AMF_SG_TYPE *sg_type;
	AVD_SG *sg_list_app_next;
	AVD_APP *app;
	bool equal_ranked_su; /* This flag is set when ranks of all SU is the same.
				     It is used in equal distribution of SIs on SU 
				     in Nway, N+M and Nway-Act Red models.*/
	AVD_SU *max_assigned_su;
	AVD_SU *min_assigned_su;
	AVD_SI *si_tobe_redistributed;
	uint32_t try_inst_counter; /* It should be used when amfd try to send
				      instantiate command to amfnd in a loop
				      for all those SUs hosted on a particular
				      node. It should be reset to zero after
				      use.*/
	
	void add_si(AVD_SI* si);
	void remove_si(AVD_SI* si);

	/**
	 * Set SG admin state, logs, checkpoints and sends notification
	 * @param state
	 */
	void set_admin_state(SaAmfAdminStateT state);

	/**
	 * Set FSM state
	 * @param sg_fsm_state
	 */
	void set_fsm_state(AVD_SG_FSM_STATE sg_fsm_state, bool wrt_to_imm = true);

	/**
	 * Set adjust state
	 * @param state
	 */
	void set_adjust_state(SaAdjustState state);

	/**
	 * Set admin SI
	 * @param si
	 */
	void set_admin_si(AVD_SI *si);

	/**
	 * Clear admin SI
	 */
	void clear_admin_si();

	/**
	 * For all SUs in SG set readiness state
	 * @param state
	 */
	void for_all_su_set_readiness_state(SaAmfReadinessStateT state);

	/**
	 * Checks if su is in operlist
	 * @param su
	 * @return
	 */
	bool in_su_oper_list(const AVD_SU *su);

	/**
	 * Add SU to operlist
	 * @param su
	 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
	 */
	uint32_t su_oper_list_add(AVD_SU *su); // TODO(hafe) add const when using container for operlist

	/**
	 * Remove SU from operlist
	 * @param su
	 * @return NCSCC_RC_SUCCESS/NCSCC_RC_FAILURE
	 */
	uint32_t su_oper_list_del(AVD_SU *su); // TODO(hafe) add const when using container for operlist

	void su_oper_list_clear();

	/**
	 * Retrieve first SU from operlist
	 * @param none
	 * @return first SU or NULL if empty
	 */
	const AVD_SU* su_oper_list_front();

	/**
	 * Handle node failure and fail over assignments
	 * Called when the node hosting the SU has already failed and the SIs
	 * assigned to the specified SU needs to be failed over.
	 *
	 * @param cb
	 * @param su
	 */
	virtual void node_fail(AVD_CL_CB *cb, AVD_SU *su) = 0;

	/**
	 * Handle SG realign
	 * Assign SIs if needed. If any assigning is gets done it adds
	 * the SUs to the operation list and sets the SG FSM state to SG realign.
	 * If everything is fine, it calls the routine to bring the preferred
	 * number of SUs to inservice state and change the SG state to stable.
	 *
	 * @param cb
	 * @param sg
	 * @return
	 */
	virtual uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg) = 0;

	/**
	 * Handle new SI or admin op UNLOCK of SI
	 * @param cb
	 * @param si
	 * @return
	 */
	virtual uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si) = 0;

	/**
	 * Handle SI admin op LOCK/SHUTDOWN
	 * @param cb
	 * @param si
	 * @return
	 */
	virtual uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si) = 0;

	/**
	 * Handle SI admin operation SWAP
	 * Default implementation in base class (not pure virtual)
	 * @param si
	 * @param invocation
	 * @return
	 */
	virtual SaAisErrorT si_swap(AVD_SI *si, SaInvocationT invocation);

	/**
	 * Handle SG admin op LOCK/SHUTDOWN
	 * @param cb
	 * @param sg
	 * @return
	 */
	virtual uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg) = 0;

	/**
	 * Handle SU inservice event, possibly assign the SU
	 * @param cb
	 * @param su
	 * @return
	 */
	virtual uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su) = 0;

	/**
	 * Handle SU failure and switch over assignments
	 * @param cb
	 * @param su
	 * @return
	 */
	virtual uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su) = 0;

	/**
	 * Handle SU admin op LOCK/SHUTDOWN
	 * @param cb
	 * @param su
	 * @param avnd
	 * @return
	 */
	virtual uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su,
			AVD_AVND *avnd) = 0;

	/**
	 * Handle successful SUSI assignment
	 * @param cb
	 * @param su
	 * @param susi
	 * @param act
	 * @param state
	 * @return
	 */
	virtual uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
			struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state) = 0;

	/**
	 * Handle failed SUSI assignment
	 *
	 * Called when a SU SI ack function is received from the ND with some error
	 * value. The message may be an ack for a particular SU SI or for the entire
	 * SU. Since if a CSI set callback returns error it is considered as
	 * failure of the component, ND would have updated that info for each of
	 * the components that failed and also for the SU an operation state
	 * message would be sent the processing will be done in that event context.
	 * For faulted SU this event would be considered as completion of action,
	 * for healthy SU no SUSI state change will be done.
	 *
	 * @param cb
	 * @param su
	 * @param susi
	 * @param act
	 * @param state
	 * @return
	 */
	virtual uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act,
		SaAmfHAStateT state) = 0;
	/**
         * Handle NG admin operation lock and shutdown.
         * Default implementation in base class (not pure virtual)
         * @param su
         * @param ng 
         * @return
         */
	virtual void ng_admin(AVD_SU *su, AVD_AMF_NG *ng);
	 /**
         * Checks if SG has assignments only on the nodes of nodegroup. 
         * @param ng
         * @return
         */
	bool is_sg_assigned_only_in_ng(const AVD_AMF_NG *ng);
	/**
         * Does an instantiable or an inservice SU exist outside the nodegroup.
         * @param ng
         * @return
         */
	bool is_sg_serviceable_outside_ng(const AVD_AMF_NG *ng);
	SaAisErrorT check_sg_stability();
	bool any_assignment_in_progress();
	bool ng_using_saAmfSGAdminState;
	
	uint32_t term_su_list_in_reverse();
       //Runtime calculates value of saAmfSGNumCurrAssignedSUs;
	uint32_t curr_assigned_sus() const;
	//Runtime calculates value of saAmfSGNumCurrInstantiatedSpareSUs;
	uint32_t curr_instantiated_spare_sus() const;
	//Runtime calculates value of saAmfSGNumCurrNonInstantiatedSpareSUs;
	uint32_t curr_non_instantiated_spare_sus() const;

private:
	// disallow copy and assign, TODO(hafe) add common macro for this
	AVD_SG(const AVD_SG&);
	void operator=(const AVD_SG&);
};

extern AmfDb<std::string, AVD_SG> *sg_db;

/**
 * 2N redundancy model SG specialization
 */
class SG_2N : public AVD_SG {
public:
	~SG_2N();
	void node_fail(AVD_CL_CB*, AVD_SU*);
	uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
	SaAisErrorT si_swap(AVD_SI *si, SaInvocationT invocation);
	uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
	uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	void ng_admin(AVD_SU *su, AVD_AMF_NG *ng);
private:
	void node_fail_si_oper(AVD_SU *su);
	void node_fail_su_oper(AVD_SU *su);
	uint32_t susi_success_si_oper(AVD_SU *su, struct avd_su_si_rel_tag *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_success_su_oper(AVD_SU *su, struct avd_su_si_rel_tag *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_success_sg_realign(AVD_SU *su, struct avd_su_si_rel_tag *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t su_fault_si_oper(AVD_SU *su);
	uint32_t su_fault_su_oper(AVD_SU *su);
};

/**
 * No redundancy specialization
 */
class SG_NORED : public AVD_SG {
public:
	~SG_NORED();
	void node_fail(AVD_CL_CB*, AVD_SU*);
	uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
	uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	void ng_admin(AVD_SU *su, AVD_AMF_NG *ng) ;
private:
	AVD_SU *assign_sis_to_sus();
};

/**
 * N+M redundancy specialization
 */
class SG_NPM : public AVD_SG {
public:
	~SG_NPM();
	void node_fail(AVD_CL_CB*, AVD_SU*);
	uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
	uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
        void node_fail_si_oper(AVD_CL_CB *cb, AVD_SU *su);
	void ng_admin(AVD_SU *su, AVD_AMF_NG *ng);
        
private:
        uint32_t su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su);
        uint32_t susi_success_su_oper(AVD_CL_CB *cb, AVD_SU *su, struct avd_su_si_rel_tag *susi,
                AVSV_SUSI_ACT act, SaAmfHAStateT state);
        void node_fail_su_oper(AVD_CL_CB *cb, AVD_SU *su);
        uint32_t su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su);
        uint32_t su_fault_sg_relgn(AVD_CL_CB *cb, AVD_SU *su);
        uint32_t susi_sucss_sg_reln(AVD_CL_CB *cb, AVD_SU *su, struct avd_su_si_rel_tag *susi,
	   AVSV_SUSI_ACT act, SaAmfHAStateT state);
};

/**
 * N-Way active specialization
 */
class SG_NACV : public AVD_SG {
public:
	~SG_NACV();
	void node_fail(AVD_CL_CB*, AVD_SU*);
	uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
	uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	void ng_admin(AVD_SU *su, AVD_AMF_NG *ng);
};

/**
 * N-Way specialization
 */
class SG_NWAY : public AVD_SG {
public:
	~SG_NWAY();
	void node_fail(AVD_CL_CB*, AVD_SU*);
	uint32_t realign(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t si_assign(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t si_admin_down(AVD_CL_CB *cb, AVD_SI *si);
	uint32_t sg_admin_down(AVD_CL_CB *cb, AVD_SG *sg);
	uint32_t su_insvc(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_admin_down(AVD_CL_CB *cb, AVD_SU *su, AVD_AVND *avnd);
	uint32_t susi_success(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t susi_failed(AVD_CL_CB *cb, AVD_SU *su,
		struct avd_su_si_rel_tag *susi, AVSV_SUSI_ACT act, SaAmfHAStateT state);
	void node_fail_si_oper(AVD_SU *su);
	SaAisErrorT si_swap(AVD_SI *si, SaInvocationT invocation);
	void ng_admin(AVD_SU *su, AVD_AMF_NG *ng);

private:
	uint32_t susi_success_su_oper(AVD_CL_CB *cb, AVD_SU *su, struct avd_su_si_rel_tag *susi,
		AVSV_SUSI_ACT act, SaAmfHAStateT state);
	uint32_t su_fault_su_oper(AVD_CL_CB *cb, AVD_SU *su);
	uint32_t su_fault_si_oper(AVD_CL_CB *cb, AVD_SU *su);
	void node_fail_su_oper(AVD_SU *su);
};

// TODO(hafe) remove when all code has been changed
#define m_AVD_SET_SG_FSM(cb,sg,state) (sg)->set_fsm_state(state)
#define m_AVD_SET_SG_ADMIN_SI(cb,si) (si)->sg_of_si->set_admin_si((si))
#define m_AVD_CLEAR_SG_ADMIN_SI(cb,sg) (sg)->clear_admin_si()
#define m_AVD_CHK_OPLIST(i_su,flag) (flag) = (i_su)->sg_of_su->in_su_oper_list(i_su)
void avd_sg_read_headless_fsm_state_cached_rta(AVD_CL_CB *cb);
void avd_sg_read_headless_su_oper_list_cached_rta(AVD_CL_CB *cb);

extern void avd_sg_delete(AVD_SG *sg);
extern void avd_sg_db_add(AVD_SG *sg);
extern void avd_sg_db_remove(AVD_SG *sg);
extern SaAisErrorT avd_sg_config_get(const std::string& app_dn, AVD_APP *app);
extern void avd_sg_add_su(AVD_SU *su);
extern void avd_sg_remove_su(AVD_SU *su);
extern void avd_sg_constructor(void);

extern void avd_sg_admin_state_set(AVD_SG* sg, SaAmfAdminStateT state);
extern void avd_sg_nwayact_screening_for_si_distr(AVD_SG *avd_sg);
extern void avd_sg_nway_screen_si_distr_equal(AVD_SG *sg);
extern void avd_su_role_failover(AVD_SU *su, AVD_SU *stdby_su);
extern bool sg_is_tolerance_timer_running_for_any_si(AVD_SG *sg);
extern void avd_sg_adjust_config(AVD_SG *sg);
extern uint32_t sg_instantiated_su_count(const AVD_SG *sg);
extern bool sg_stable_after_lock_in_or_unlock_in(AVD_SG *sg);
extern void process_su_si_response_for_ng(AVD_SU *su, SaAisErrorT res);

#endif
