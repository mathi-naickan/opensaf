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
 * Author(s): Ericsson AB
 *
 */

#include <set>
#include <algorithm>
#include <assert.h>
#include <time.h>

#include "ImmModel.hh"
#include "ImmAttrValue.hh"
#include "ImmSearchOp.hh"


#include "immnd.h"
#include "immsv_api.h"

// Local types
#define DEFAULT_TIMEOUT_SEC 6 /* Should be saImmOiTimeout in SaImmMngt */

struct ContinuationInfo2
{
    ContinuationInfo2():mCreateTime(0), mConn(0), mTimeout(0){}
    ContinuationInfo2(SaUint32T conn, SaUint32T timeout):mConn(conn), mTimeout(timeout) 
        {mCreateTime = time(NULL);}
    
    time_t  mCreateTime;
    SaUint32T mConn;
    SaUint32T mTimeout; //0=> no timeout. Otherwise timeout in SECONDS.
    //Default copy constructor and assignement operator used in ContinuationMap2
};
typedef std::map<SaInvocationT, ContinuationInfo2> ContinuationMap2;


struct ContinuationInfo3
{
    ContinuationInfo3(): mConn(0), mReply_dest(0LL), mImplementer(NULL) {}
    ContinuationInfo3(SaUint32T conn, SaUint64T reply_dest, ImplementerInfo* i):
        mConn(conn),mReply_dest(reply_dest), mImplementer(i) {}
    
    //time_t  mCreateTime;
    SaUint32T mConn;
    SaUint64T mReply_dest;
    ImplementerInfo* mImplementer;
    //Default copy constructor and assignement operator used in ContinuationMap3
};

typedef std::map<SaInvocationT, ContinuationInfo3> ContinuationMap3;

//typedef std::map<SaInvocationT, SaUint32T> ContinuationMap;

struct AttrInfo
{
    SaUint32T       mValueType;
    SaUint32T       mFlags;
    SaUint32T       mNtfId;
    ImmAttrValue    mDefaultValue;
};
typedef std::map<std::string, AttrInfo*> AttrMap;

struct ImplementerInfo
{
    SaUint32T       mId;
    SaUint32T       mConn; //Current implementer, only valid on one node.
    //NULL otherwise.
    unsigned int    mNodeId;
    std::string     mImplementerName;
    SaUint64T       mMds_dest;
    unsigned int    mAdminOpBusy;
    bool            mDying;
};

typedef std::vector<ImplementerInfo*> ImplementerVector;

struct ImplementerCcbAssociation
{
    ImplementerCcbAssociation(ImplementerInfo* impl) : mImplementer(impl),
                                                       mContinuationId(0),
                                                       mWaitForImplAck(false){}
    ImplementerInfo* mImplementer;
    SaUint32T mContinuationId;//used to identify related pending msg replies.
    bool mWaitForImplAck;
};

typedef std::map<SaUint32T, ImplementerCcbAssociation*> CcbImplementerMap;

struct ClassInfo
{
    ClassInfo(SaUint32T category) : mRefCount(0),  mCategory(category),
                                    mImplementer(NULL) { }
    ~ClassInfo() { }
    
    SaInt32T         mRefCount;
    SaUint32T        mCategory;
    AttrMap          mAttrMap;   //<-Each AttrInfo requires explicit delete
    ImplementerInfo* mImplementer;//<- Points INTO ImplementerVector
};
typedef std::map<std::string, ClassInfo*> ClassMap;

typedef std::map<std::string, ImmAttrValue*> ImmAttrValueMap;

typedef SaUint32T ImmObjectFlags;
#define IMM_CREATE_LOCK 0x00000001
//If create lock is on, it signifies that a ccb has reserved space in
//the name-tree for the object, pending the ccb-apply. The object must
//be invisible in contents (invisinble to search and get), visible in name
//to creates from any ccb, and finally, invisible as 'parent' to creates
//from other Ccb's. Creates of subobjects in the same ccb must be allowed. 
//Deletes from other ccbs need not test on ths lock because they will conflict
//on the ccbId of the object anyway. 
//The lock is also used during creates of persistent runtime objects (PRTOs).
//If the Persistent Back End (PBE) is enabled, then the create of the PRTO
//has to wait on ack from the PBE before being released to the client. 
//During the wait, the create lock is set on the object.

#define IMM_DELETE_LOCK 0x00000002
//If the delete-lock is on, it signifies that a ccb has registered a delete
//of this object, pending the apply of the ccb. The only signifficance of this
//is to prevent the creates of subobjects to this object in any ccb.
//Even in the SAME ccb it is not allowed to create subobject to a registered
//delete. 
//The lock is also used during deletes of persistent runtime objects (PRTOs).
//If the Persistent Back End (PBE) is enabled, then the deletes of a subtree,
//which may include both PRTOs and RTOs, has to wait on ack from the PBE,
//before being released to the client. 
//During the wait, the delete lock is set on the object.

#define IMM_DN_INTERNAL_REP 0x00000004
//If this flag is set it indicates that the DN has been changed in rep
//from external rep to an internal rep, where any escaped commas "\," 
//in external rep have been replaced by an escape hash "\#" in internal rep;
//Escape hash in external rep are not allowed, will be rejected in create.

#define IMM_PRTO_FLAG 0x00000008
//Flag indicates that the object is a persistent runtime object (PRTO).
//This flag is not reliably set in general. I.e. it can not (yet) be
//used in general to test if the object is a PRTO.
//If the flag is set then the object must be a PRTO.
//If the flag is not set, the object could still be a PRTO.
//It is currently only used in rtObjectDelete() and deleteRtObject()

#define IMM_RT_UPDATE_LOCK 0x00000010
//This is only set by rtObjectUpdate() when the update includes persistent
//runtime attributes PRTAs. The PRTAs may belong to either a runtime object
//or a configureation object. If the PBE is enabled, then the updates of
//cached RTAs (peristent or just cached) included in the call, has to wait
//for ack from the PBE on the perisistification of the PRTAs. 
//The lock must be inspected/read by both ccb calls and RT calls.

struct ObjectInfo  
{
    void             getAdminOwnerName(std::string *str) const;
    
    ImmAttrValue*    mAdminOwnerAttrVal; //Pointer INTO mAttrValueMap
    //CcbInfo* mCcb;    //<- Points INTO CcbVector.
    SaUint32T        mCcbId;
    ImmAttrValueMap  mAttrValueMap; //<-Each ImmAttrValue needs explicit delete
    ClassInfo*       mClassInfo;    //<-Points INTO ClassMap. Not own copy!
    ImplementerInfo* mImplementer;  //<-Points INTO ImplementerVector
    ImmObjectFlags   mObjFlags;
};
//typedef std::map<std::string, ObjectInfo*> ObjectMap;  
typedef std::set<ObjectInfo*> ObjectSet;

void
ObjectInfo::getAdminOwnerName(std::string *str) const
{
    assert(this->mAdminOwnerAttrVal);
    str->clear();
    if(!(this->mAdminOwnerAttrVal->empty())) {
        str->assign(this->mAdminOwnerAttrVal->getValueC_str());
    } 
}

typedef enum {
    IMM_CREATE = 1,
    IMM_MODIFY = 2,
    IMM_DELETE = 3,
    IMM_CREATE_CLASS = 4,
    IMM_DELETE_CLASS = 5,
    IMM_UPDATE_EPOCH = 6
} ImmMutationType;

struct ObjectMutation
{
    ObjectMutation(ImmMutationType opType) : mOpType(opType), 
                                             //mBeforeImage(NULL),
                                             mAfterImage(NULL), 
                                             mWaitForImplAck(false),
                                             mContinuationId(0) { }
    ~ObjectMutation() { assert(mAfterImage == NULL);}
    
    ImmMutationType mOpType; 
    ObjectInfo* mAfterImage;
    bool mWaitForImplAck;  //ack required from implementer for THIS object
    SaUint32T mContinuationId;//used to identify related pending msg replies.
};
typedef std::map<std::string, ObjectMutation*> ObjectMutationMap;  

typedef enum {
    IMM_CCB_EMPTY = 1,      //State after creation 
    IMM_CCB_READY = 2,      //Ready for new ops, or commit or abort.
    IMM_CCB_CREATE_OP = 3,  //Ongoing create (pending implementer calls/replies)
    IMM_CCB_MODIFY_OP = 4,  //Ongoing modify (pending implementer calls/replies)
    IMM_CCB_DELETE_OP = 5,  //Ongoing delete (pending implementer calls/replies)
    IMM_CCB_PREPARE = 6,    //Waiting for nodes prepare & completed calls/replies
    IMM_CCB_CRITICAL = 7,   //Unilateral abort no longer allowed (except by PBE). 
    IMM_CCB_PBE_ABORT = 8,  //The Persistent back end replied with abort
    IMM_CCB_COMMITTED = 9,  //Committed at nodes pending implementer apply calls
    IMM_CCB_ABORTED = 10,   //READY->ABORTED PREPARE->ABORTED
    IMM_CCB_ILLEGAL = 11   //CCB has been removed.
} ImmCcbState;

struct CcbInfo
{
    bool isOk() {return mVeto == SA_AIS_OK;}
    bool isActive() {return (mState < IMM_CCB_COMMITTED);}
    SaUint32T         mId;
    SaUint32T         mAdminOwnerId;
    SaUint32T         mCcbFlags;

    SaUint32T         mOriginatingConn; //If !NULL then originating at this node
    //with this conn.
    unsigned int      mOriginatingNode;  //Needed for node crash Ccb GC
    ImmCcbState       mState;
    CcbImplementerMap mImplementers;
    ObjectMutationMap mMutations;
    SaAisErrorT       mVeto;  //SA_AIS_OK as long as no "participan" voted error.
    time_t            mWaitStartTime;
    SaUint32T         mOpCount;
    SaUint32T         mPbeRestartId;
};
typedef std::vector<CcbInfo*> CcbVector;

struct AdminOwnerInfo
{
    SaUint32T       mId;
    std::string     mAdminOwnerName;
    SaUint32T       mConn;
    unsigned int    mNodeId;
    ObjectSet       mTouchedObjects;  //No good, sync needs the object names.
    bool            mReleaseOnFinalize;
    bool            mDying;
};
typedef std::vector<AdminOwnerInfo*> AdminOwnerVector;


typedef enum {
    IMM_NODE_UNKNOWN = 0,      //Initial state
    IMM_NODE_LOADING = 1,      //We are participating in a cluster restart.
    IMM_NODE_FULLY_AVAILABLE = 2, //Normal fully available state.
    IMM_NODE_ISOLATED = 3,     //We are trying to join an established cluster.
    IMM_NODE_W_AVAILABLE = 4,  //We are being synced, no reads allowed here
    IMM_NODE_R_AVAILABLE = 5   //We are write locked while other nodes are 
                               //being synced.
} ImmNodeState;

typedef std::set<std::string> ObjectNameSet;

// Local variables

static ClassMap          sClassMap;
static AdminOwnerVector  sOwnerVector;
static CcbVector         sCcbVector;
static ObjectMap         sObjectMap;
static ImplementerVector sImplementerVector;
static ObjectNameSet     sMissingParents;
static ContinuationMap2  sAdmReqContinuationMap;
static ContinuationMap3  sAdmImplContinuationMap;
static ContinuationMap2  sSearchReqContinuationMap;  
static ContinuationMap3  sSearchImplContinuationMap; 
static ContinuationMap2  sPbeRtReqContinuationMap;
static ObjectMutationMap sPbeRtMutations; /* Persistent Runtime Mutations 
                                              At most one mutating op per Prto 
                                              is allowed. Entry removed on on 
                                              ack from PBE. */
static SaUint32T        sPbeRtMinContId = 0; /* Monitors that no PbePrto gets stuck. */
static SaUint32T        sPbeRtBacklog = 0;   /* Monitors PbePrto capacity problems. */
static SaUint32T        sPbeRegressPeriods = 0; 

static IdVector         sNodesDeadDuringSync; // Keep track of implementers/nodes that
static IdVector         sImplsDeadDuringSync; // die after finalizeSync is sent by coord,
                                              // but before it arrives over fevs. 
                                              // This to avoid apparent implementor
                                              // re-create by finalizeSync (at non coord). 

static SaUint32T        sLastContinuationId = 0;

static ImmNodeState     sImmNodeState = IMM_NODE_UNKNOWN;

static std::string immObjectDn(OPENSAF_IMM_OBJECT_DN);
static std::string immAttrClasses(OPENSAF_IMM_ATTR_CLASSES);
static std::string immAttrEpoch(OPENSAF_IMM_ATTR_EPOCH);
static std::string immClassName(OPENSAF_IMM_CLASS_NAME);

static std::string immManagementDn("safRdn=immManagement,safApp=safImmService");
static std::string saImmRepositoryInit("saImmRepositoryInit");
static SaImmRepositoryInitModeT immInitMode = SA_IMM_INIT_FROM_FILE;

SaAisErrorT 
immModel_ccbResult(IMMND_CB *cb, SaUint32T ccbId)
{
    return ImmModel::instance(&cb->immModel)->ccbResult(ccbId);
}

void
immModel_abortSync(IMMND_CB *cb)
{
    ImmModel::instance(&cb->immModel)->abortSync();
}

void
immModel_pbePrtoPurgeMutations(IMMND_CB *cb, SaUint32T nodeId, SaUint32T *reqArrSize,
    SaUint32T **reqConnArr)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix = 0;
    ImmModel::instance(&cb->immModel)->pbePrtoPurgeMutations(nodeId, cv);
    *reqArrSize = cv.size();
    if(*reqArrSize) {
            *reqConnArr = (SaUint32T *) malloc((*reqArrSize)* sizeof(SaUint32T));
             for(cvi = cv.begin(); cvi!= cv.end();++cvi,++ix) {
                 (*reqConnArr)[ix] = (*cvi);
             }
    }
}

SaAisErrorT
immModel_adminOwnerCreate(IMMND_CB *cb, 
    const ImmsvOmAdminOwnerInitialize* req,
    SaUint32T ownerId,
    SaUint32T conn,
    SaClmNodeIdT nodeId)
{
    return ImmModel::instance(&cb->immModel)->
        adminOwnerCreate(req, ownerId, conn, nodeId);
}

SaAisErrorT
immModel_adminOwnerDelete(IMMND_CB *cb, SaUint32T ownerId, SaUint32T hard)
{
    return ImmModel::instance(&cb->immModel)->
        adminOwnerDelete(ownerId, hard);
}


SaAisErrorT
immModel_ccbCreate(IMMND_CB *cb, 
    SaUint32T adminOwnerwnerId,
    SaUint32T ccbFlags,
    SaUint32T ccbId,
    SaClmNodeIdT nodeId,
    SaUint32T conn)
{
    return ImmModel::instance(&cb->immModel)->
        ccbCreate(adminOwnerwnerId, ccbFlags, ccbId, nodeId, conn);
    
}

SaAisErrorT
immModel_classCreate(IMMND_CB *cb, const struct ImmsvOmClassDescr* req,
    SaUint32T reqConn,
    unsigned int nodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    unsigned int* pbeNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        classCreate(req, reqConn, nodeId, continuationId, pbeConn, pbeNodeId);
}

SaAisErrorT
immModel_classDelete(IMMND_CB *cb, const struct ImmsvOmClassDescr* req,
    SaUint32T reqConn,
    unsigned int nodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    unsigned int* pbeNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        classDelete(req, reqConn, nodeId, continuationId, pbeConn, pbeNodeId);
}

SaAisErrorT
immModel_ccbObjectCreate(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectCreate* req,
    SaUint32T* implConn,
    SaClmNodeIdT* implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        ccbObjectCreate(req, implConn, implNodeId, continuationId, 
            pbeConn, pbeNodeId);
}

SaAisErrorT
immModel_ccbObjectModify(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectModify* req,
    SaUint32T* implConn,
    SaClmNodeIdT* implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        ccbObjectModify(req, implConn, implNodeId, continuationId,
            pbeConn, pbeNodeId);
}


SaAisErrorT
immModel_ccbObjectDelete(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectDelete* req,
    SaUint32T reqConn,
    SaUint32T* arrSize,
    SaUint32T** implConnArr,
    SaUint32T** implIdArr,
    SaStringT** objNameArr,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId)
    
{
    ConnVector cv;
    IdVector iv;
    ObjectNameVector ov;
    ConnVector::iterator cvi;
    IdVector::iterator ivi;
    ObjectNameVector::iterator oni;
    unsigned int ix = 0;
    
    SaAisErrorT err = 
        ImmModel::instance(&cb->immModel)->ccbObjectDelete(req,
            reqConn, ov, cv, iv, pbeConn, pbeNodeId);
    *arrSize = cv.size();
    assert(*arrSize == iv.size());
    assert(*arrSize == ov.size());
    if((err == SA_AIS_OK) && (*arrSize)) {
        *implConnArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        *implIdArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        *objNameArr = (SaStringT *) malloc((*arrSize)* sizeof(SaStringT));
        
        for(cvi = cv.begin(), ivi=iv.begin(), oni=ov.begin();
            cvi!=cv.end();
            ++cvi,++ivi,++oni, ++ix) {
            std::string delObjName = (*oni);
            (*implConnArr)[ix] = (*cvi);
            (*implIdArr)[ix] = (*ivi);
            (*objNameArr)[ix] = (SaStringT) malloc(delObjName.size() + 1);
            strncpy((*objNameArr)[ix], delObjName.c_str(), delObjName.size()+1);
        }
    }
    assert(ix==(*arrSize));
    return err;
}

void
immModel_getNonCriticalCcbs(IMMND_CB *cb,
    SaUint32T** ccbIdArr,
    SaUint32T* ccbIdArrSize)
{
    IdVector ccbs;
    IdVector::iterator ix2;
    unsigned int ix;
    
    ImmModel::instance(&cb->immModel)->getNonCriticalCcbs(ccbs);
    *ccbIdArrSize = ccbs.size();
    if(*ccbIdArrSize) {
        *ccbIdArr = (SaUint32T *) malloc((*ccbIdArrSize) * sizeof(SaUint32T));
        
        for(ix2=ccbs.begin(), ix=0;
            ix2!=ccbs.end();
            ++ix2, ++ix) {
            (*ccbIdArr)[ix] = (*ix2);
        }
        assert(ix==(*ccbIdArrSize));
    }
}

void
immModel_getOldCriticalCcbs(IMMND_CB *cb,
    SaUint32T** ccbIdArr,
    SaUint32T* ccbIdArrSize,
    SaUint32T *pbeConn,
    unsigned int *pbeNodeId,
    SaUint32T* pbeId)
{
    IdVector ccbs;
    IdVector::iterator ix2;
    unsigned int ix;

    if(ImmModel::instance(&cb->immModel)->getPbeOi(pbeConn, pbeNodeId)) {
        ImmModel::instance(&cb->immModel)->getOldCriticalCcbs(ccbs, pbeConn,
            pbeNodeId, pbeId);
        *ccbIdArrSize = ccbs.size();
        if(*ccbIdArrSize) {
            *ccbIdArr = (SaUint32T *)
                malloc((*ccbIdArrSize) * sizeof(SaUint32T));
        
            for(ix2=ccbs.begin(), ix=0;
                ix2!=ccbs.end();
                ++ix2, ++ix) {
                (*ccbIdArr)[ix] = (*ix2);
            }
            assert(ix==(*ccbIdArrSize));
        }
    }
}

SaBoolT
immModel_pbeOiExists(IMMND_CB *cb)
{
    SaUint32T pbeConn=0;
    unsigned int pbeNode=0;
    return (ImmModel::instance(&cb->immModel)->getPbeOi(&pbeConn, &pbeNode)) ? 
        SA_TRUE : SA_FALSE;
}

SaUint32T
immModel_cleanTheBasement(IMMND_CB *cb, 
    SaUint32T seconds,
    SaInvocationT** admReqArr,
    SaUint32T* admReqArrSize,
    SaInvocationT** searchReqArr,
    SaUint32T* searchReqArrSize,
    SaUint32T** ccbIdArr,
    SaUint32T* ccbIdArrSize,
    SaUint32T** pbePrtoReqArr,
    SaUint32T* pbePrtoReqArrSize,
    SaBoolT iAmCoordNow)
{
    InvocVector admReqs;
    InvocVector searchReqs;
    InvocVector::iterator ix1;
    IdVector ccbs;
    IdVector pbePrtoReqs;
    IdVector::iterator ix2;
    unsigned int ix;
    
    SaUint32T stuck = 
        ImmModel::instance(&cb->immModel)->cleanTheBasement(seconds, 
        admReqs, 
        searchReqs, 
        ccbs,
        pbePrtoReqs,
        iAmCoordNow);

    *admReqArrSize = admReqs.size();
    if(*admReqArrSize) {
        *admReqArr = (SaInvocationT *) malloc((*admReqArrSize) *
            sizeof(SaInvocationT));
        for(ix1=admReqs.begin(), ix=0; 
            ix1!=admReqs.end();
            ++ix1, ++ix) {
            (*admReqArr)[ix] = (*ix1);
        }
        assert(ix==(*admReqArrSize));
    }
    
    *searchReqArrSize = searchReqs.size();
    if(*searchReqArrSize) {
        *searchReqArr = (SaInvocationT *) malloc((*searchReqArrSize) * 
            sizeof(SaInvocationT));
        for(ix1=searchReqs.begin(), ix=0;
            ix1!=searchReqs.end();
            ++ix1, ++ix) {
            (*searchReqArr)[ix] = (*ix1);
        }
        assert(ix==(*searchReqArrSize));
    }
    
    *ccbIdArrSize = ccbs.size();
    if(*ccbIdArrSize) {
        *ccbIdArr = (SaUint32T *) malloc((*ccbIdArrSize) * sizeof(SaUint32T));
        
        for(ix2=ccbs.begin(), ix=0;
            ix2!=ccbs.end();
            ++ix2, ++ix) {
            (*ccbIdArr)[ix] = (*ix2);
        }
        assert(ix==(*ccbIdArrSize));
    }

    *pbePrtoReqArrSize = pbePrtoReqs.size();
    if(*pbePrtoReqArrSize) {
        *pbePrtoReqArr = (SaUint32T *) 
            malloc((*pbePrtoReqArrSize) * sizeof(SaUint32T));
        
        for(ix2=pbePrtoReqs.begin(), ix=0;
            ix2!=pbePrtoReqs.end();
            ++ix2, ++ix) {
            (*pbePrtoReqArr)[ix] = (*ix2);
        }
        assert(ix==(*pbePrtoReqArrSize));
    }

    return stuck;
}

SaAisErrorT
immModel_ccbApply(IMMND_CB *cb, 
    SaUint32T ccbId,
    SaUint32T reqConn,
    SaUint32T* arrSize,
    SaUint32T** implConnArr,
    SaUint32T** implIdArr,
    SaUint32T** ctnArr)
{
    ConnVector cv;
    IdVector implsv;
    IdVector ctnv;
    ConnVector::iterator cvi;
    IdVector::iterator ivi1;
    IdVector::iterator ivi2;
    unsigned int ix=0;
    
    
    SaAisErrorT err = ImmModel::instance(&cb->immModel)->
        ccbApply(ccbId, reqConn, cv, implsv, ctnv);
    
    *arrSize = cv.size();
    if(*arrSize) {
        *implConnArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        *implIdArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        *ctnArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        for(cvi = cv.begin(), ivi1=implsv.begin(), ivi2=ctnv.begin();
            cvi!=cv.end();
            ++cvi,++ivi1,++ivi2, ++ix) {
            (*implConnArr)[ix] = (*cvi);
            (*implIdArr)[ix] = (*ivi1);
            (*ctnArr)[ix] = (*ivi2);
        }
    }
    assert(ix==(*arrSize));
    return err;
}

void
immModel_ccbAbort(IMMND_CB *cb, 
    SaUint32T ccbId,
    SaUint32T* arrSize,
    SaUint32T** implConnArr,
    SaUint32T* client,
    SaClmNodeIdT* pbeNodeId)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix=0;
    
    
    ImmModel::instance(&cb->immModel)->ccbAbort(ccbId, cv, client, pbeNodeId);
    
    *arrSize = cv.size();
    if(*arrSize) {
        *implConnArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        for(cvi = cv.begin(); cvi!=cv.end(); ++cvi, ++ix) {
            (*implConnArr)[ix] = (*cvi);
        }
    }
    assert(ix==(*arrSize));
}

void
immModel_getCcbIdsForOrigCon(IMMND_CB *cb, 
    SaUint32T deadCon,
    SaUint32T* arrSize,
    SaUint32T** ccbIdArr)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix=0;
    ImmModel::instance(&cb->immModel)->getCcbIdsForOrigCon(deadCon, cv);
    *arrSize = cv.size();
    if(*arrSize) {
        *ccbIdArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        for(cvi = cv.begin(); cvi!=cv.end(); ++cvi, ++ix) {
            (*ccbIdArr)[ix] = (*cvi);
        }
    }
    assert(ix==(*arrSize));
}

void
immModel_discardNode(IMMND_CB *cb, 
    SaUint32T nodeId,
    SaUint32T* arrSize,
    SaUint32T** ccbIdArr)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix=0;
    ImmModel::instance(&cb->immModel)->discardNode(nodeId, cv);
    *arrSize = cv.size();
    if(*arrSize) {
        *ccbIdArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        for(cvi = cv.begin(); cvi!=cv.end(); ++cvi, ++ix) {
            (*ccbIdArr)[ix] = (*cvi);
        }
    }
    assert(ix==(*arrSize));
}

void
immModel_getAdminOwnerIdsForCon(IMMND_CB *cb, 
    SaUint32T deadCon,
    SaUint32T* arrSize,
    SaUint32T** admoIdArr)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix=0;
    ImmModel::instance(&cb->immModel)->getAdminOwnerIdsForCon(deadCon, cv);
    *arrSize = cv.size();
    if(*arrSize) {
        *admoIdArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        for(cvi = cv.begin(); cvi!=cv.end(); ++cvi, ++ix) {
            (*admoIdArr)[ix] = (*cvi);
        }
    }
    assert(ix==(*arrSize));
}

SaAisErrorT
immModel_adminOperationInvoke(IMMND_CB *cb, 
    const struct ImmsvOmAdminOperationInvoke* req,
    SaUint32T reqConn,
    SaUint64T reply_dest,
    SaInvocationT inv,
    SaUint32T* implConn,
    SaClmNodeIdT* implNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        adminOperationInvoke(req, reqConn, reply_dest, inv, implConn, implNodeId);
}

/*
  void
  immModel_setCcbObjCreateImplContinuation(IMMND_CB *cb, 
  SaInvocationT saInv,
  SaUint32T implConn, 
  SaUint64T reply_dest)
  {
  ImmModel::instance(&cb->immModel)->
  setCcbObjCreateImplContinuation(saInv, implConn, reply_dest);
  }
  
  void
  immModel_setCcbObjDelImplContinuation(IMMND_CB *cb, 
  SaInvocationT saInv,
  SaUint32T implConn, 
  SaUint64T reply_dest)
  {
  ImmModel::instance(&cb->immModel)->
  setCcbObjDelImplContinuation(saInv, implConn, reply_dest);
  }
  
  void
  immModel_setCcbCompletedImplContinuation(IMMND_CB *cb, 
  SaInvocationT saInv,
  SaUint32T implConn,
  SaUint64T reply_dest)
  {
  ImmModel::instance(&cb->immModel)->
  setCcbCompletedImplContinuation(saInv, implConn, reply_dest);
  }
  
  void
  immModel_fetchCcbCompletedContinuation(IMMND_CB *cb, 
  SaInvocationT saInv,
  SaUint32T* implConn,
  SaUint64T* reply_dest)
  {
  ImmModel::instance(&cb->immModel)->
  fetchCcbCompletedContinuation(saInv, implConn, reply_dest);
  }
*/

SaUint32T 
immModel_findConnForImplementerOfObject(IMMND_CB *cb, 
    IMMSV_OCTET_STRING* objectName)
{
    size_t sz = strnlen((char *) objectName->buf, 
        (size_t)objectName->size);
    
    std::string objectDn((const char *) objectName->buf, sz);
    
    return ImmModel::instance(&cb->immModel)->
        findConnForImplementerOfObject(objectDn);
}

SaBoolT
immModel_ccbWaitForCompletedAck(IMMND_CB *cb, 
    SaUint32T ccbId,
    SaAisErrorT* err,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId,
    SaUint32T* pbeId,
    SaUint32T* pbeCtn)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->
        ccbWaitForCompletedAck(ccbId, err, pbeConn, pbeNodeId, pbeId, pbeCtn);
}

SaBoolT
immModel_ccbWaitForDeleteImplAck(IMMND_CB *cb, 
    SaUint32T ccbId,
    SaAisErrorT* err)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->
        ccbWaitForDeleteImplAck(ccbId, err);
}


SaBoolT
immModel_ccbCommit(IMMND_CB *cb, 
    SaUint32T ccbId,
    SaUint32T* arrSize,
    SaUint32T** implConnArr)
{
    ConnVector cv;
    ConnVector::iterator cvi;
    unsigned int ix=0;
    
    SaBoolT pbeModeChange = (SaBoolT) 
        ImmModel::instance(&cb->immModel)->ccbCommit(ccbId, cv);
    
    *arrSize = cv.size();
    if(*arrSize) {
        *implConnArr = (SaUint32T *) malloc((*arrSize)* sizeof(SaUint32T));
        
        for(cvi = cv.begin(); cvi!=cv.end(); ++cvi, ++ix) {
            (*implConnArr)[ix] = (*cvi);
        }
    }
    
    assert(ix==(*arrSize));
    return pbeModeChange;
}

SaAisErrorT
immModel_ccbFinalize(IMMND_CB *cb, 
    SaUint32T ccbId)
{
    return ImmModel::instance(&cb->immModel)->ccbTerminate(ccbId);
}

SaAisErrorT
immModel_searchInitialize(IMMND_CB *cb, struct ImmsvOmSearchInit* req, 
    void** searchOp)
{
    ImmSearchOp* op = new ImmSearchOp();
    *searchOp = op;
    
    return ImmModel::instance(&cb->immModel)->searchInitialize(req, *op);
}

SaAisErrorT
immModel_nextResult(IMMND_CB *cb, void* searchOp, 
    IMMSV_OM_RSP_SEARCH_NEXT** rsp,
    SaUint32T* implConn, SaUint32T* implNodeId,
    struct ImmsvAttrNameList** rtAttrsToFetch,
    MDS_DEST* implDest)
{
    //assert(searchOp && rsp && implConn && implNodeId && rtAttrsToFetch);
    assert(searchOp && rsp);
    if(rtAttrsToFetch) {assert(implConn && implNodeId && implDest);}
    ImmSearchOp* op = (ImmSearchOp *) searchOp;
    
    AttributeList* rtAttrs = NULL;
    SaAisErrorT err = op->nextResult(rsp, implConn, implNodeId, 
        (rtAttrsToFetch)?(&rtAttrs):NULL,
        (SaUint64T*) implDest);
    if(err != SA_AIS_OK) { return err; }
    
    if(rtAttrs && (*implNodeId)) {
        AttributeList::iterator ai;
        for(ai = rtAttrs->begin();ai != rtAttrs->end(); ++ai) {
            ImmsvAttrNameList* p = (ImmsvAttrNameList *) 
                malloc(sizeof(ImmsvAttrNameList));  /*alloc-1*/
            p->name.size = (SaUint32T) (*ai).name.length() +1;
            p->name.buf = (char *) malloc(p->name.size); /*alloc-2*/
            strncpy(p->name.buf, (*ai).name.c_str(), p->name.size);
            
            p->next = *rtAttrsToFetch;
            *rtAttrsToFetch = p;
        }
    }
    return SA_AIS_OK;
}

void
immModel_deleteSearchOp(void* searchOp)
{
    ImmSearchOp* op = (ImmSearchOp *) searchOp;
    delete op;
}

void
immModel_fetchLastResult(void* searchOp, IMMSV_OM_RSP_SEARCH_NEXT** rsp)
{
    ImmSearchOp* op = (ImmSearchOp *) searchOp;
    *rsp = op->fetchLastResult();
}

void
immModel_clearLastResult(void* searchOp)
{
    ImmSearchOp* op = (ImmSearchOp *) searchOp;
    op->clearLastResult();
}

void
immModel_setSearchReqContinuation(IMMND_CB *cb, SaInvocationT invoc, 
    SaUint32T reqConn)
{
    ImmModel::instance(&cb->immModel)->setSearchReqContinuation(invoc, reqConn);
}

void
immModel_setSearchImplContinuation(IMMND_CB *cb, SaUint32T searchId,
    SaUint32T reqNodeId, MDS_DEST reply_dest)
{
    SaInvocationT tmp_hdl = m_IMMSV_PACK_HANDLE(searchId, reqNodeId);
    ImmModel::instance(&cb->immModel)->
        setSearchImplContinuation(tmp_hdl, (SaUint64T) reply_dest);
}

SaAisErrorT
immModel_implementerSet(IMMND_CB *cb, const IMMSV_OCTET_STRING* implName,
    SaUint32T implConn, SaUint32T implNodeId,
    SaUint32T implId, MDS_DEST mds_dest)
{
    return 
        ImmModel::instance(&cb->immModel)->implementerSet(implName, 
            implConn,
            implNodeId,
            implId,
            (SaUint64T) mds_dest);
}

SaUint32T
immModel_getImplementerId(IMMND_CB* cb, SaUint32T deadConn)
{
    return ImmModel::instance(&cb->immModel)->getImplementerId(deadConn);
}

void
immModel_discardImplementer(IMMND_CB* cb, SaUint32T implId, 
    SaBoolT reallyDiscard)
{
    ImmModel::instance(&cb->immModel)->discardImplementer(implId, reallyDiscard);
}

SaAisErrorT
immModel_implementerClear(IMMND_CB *cb, const struct ImmsvOiImplSetReq* req,
    SaUint32T implConn, SaUint32T implNodeId)
{
    return ImmModel::instance(&cb->immModel)->implementerClear(req, implConn,
        implNodeId);
}

SaAisErrorT
immModel_classImplementerSet(IMMND_CB *cb, const struct ImmsvOiImplSetReq* req,
    SaUint32T implConn, SaUint32T implNodeId)
{
    return ImmModel::instance(&cb->immModel)->classImplementerSet(req, implConn,
        implNodeId);
}

SaAisErrorT
immModel_classImplementerRelease(IMMND_CB *cb, 
    const struct ImmsvOiImplSetReq* req,
    SaUint32T implConn, SaUint32T implNodeId)
{
    return ImmModel::instance(&cb->immModel)->classImplementerRelease(req,
        implConn,
        implNodeId);
}

SaAisErrorT
immModel_objectImplementerSet(IMMND_CB *cb, 
    const struct ImmsvOiImplSetReq* req,
    SaUint32T implConn, SaUint32T implNodeId)
{
    return ImmModel::instance(&cb->immModel)->objectImplementerSet(req, implConn,
        implNodeId);
}

SaAisErrorT
immModel_objectImplementerRelease(IMMND_CB *cb, 
    const struct ImmsvOiImplSetReq* req,
    SaUint32T implConn, SaUint32T implNodeId)
{
    return ImmModel::instance(&cb->immModel)->objectImplementerRelease(req,
        implConn,
        implNodeId);
}

void
immModel_ccbCompletedContinuation(IMMND_CB *cb, 
    const struct immsv_oi_ccb_upcall_rsp* rsp,
    SaUint32T* reqConn)
{
    ImmModel::instance(&cb->immModel)->ccbCompletedContinuation(rsp, reqConn);
}

void
immModel_ccbObjDelContinuation(IMMND_CB *cb, 
    const struct immsv_oi_ccb_upcall_rsp* rsp,
    SaUint32T* reqConn)
{
    ImmModel::instance(&cb->immModel)->ccbObjDelContinuation(rsp, reqConn);
}

void
immModel_ccbObjCreateContinuation(IMMND_CB *cb,
    SaUint32T ccbId, 
    SaUint32T invocation,
    SaAisErrorT error,
    SaUint32T* reqConn)
{
    ImmModel::instance(&cb->immModel)->ccbObjCreateContinuation(ccbId,
        invocation,
        error,
        reqConn);
}

void immModel_pbePrtObjCreateContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaAisErrorT err,
        SaClmNodeIdT nodeId, SaUint32T *reqConn)
{
    ImmModel::instance(&cb->immModel)->pbePrtObjCreateContinuation(
        invocation, err, nodeId, reqConn);
}

void immModel_pbeClassCreateContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaClmNodeIdT nodeId, SaUint32T *reqConn)
{
    ImmModel::instance(&cb->immModel)->pbeClassCreateContinuation(
        invocation, nodeId, reqConn);
}

void immModel_pbeClassDeleteContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaClmNodeIdT nodeId, SaUint32T *reqConn)
{
    ImmModel::instance(&cb->immModel)->pbeClassDeleteContinuation(
        invocation, nodeId, reqConn);
}

void immModel_pbeUpdateEpochContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaClmNodeIdT nodeId)
{
    ImmModel::instance(&cb->immModel)->pbeUpdateEpochContinuation(
        invocation, nodeId);
}

void immModel_pbePrtObjDeletesContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaAisErrorT err,
        SaClmNodeIdT nodeId, SaUint32T *reqConn)
{
    ImmModel::instance(&cb->immModel)->pbePrtObjDeletesContinuation(
        invocation, err, nodeId, reqConn);
}

void immModel_pbePrtAttrUpdateContinuation(IMMND_CB *cb,
        SaUint32T invocation, SaAisErrorT err,
        SaClmNodeIdT nodeId, SaUint32T *reqConn)
{
    ImmModel::instance(&cb->immModel)->pbePrtAttrUpdateContinuation(
        invocation, err, nodeId, reqConn);
}

void
immModel_ccbObjModifyContinuation(IMMND_CB *cb,
    SaUint32T ccbId, 
    SaUint32T invocation,
    SaAisErrorT error,
    SaUint32T* reqConn)
{
    ImmModel::instance(&cb->immModel)->ccbObjModifyContinuation(ccbId,
        invocation,
        error,
        reqConn);
}

SaAisErrorT
immModel_classDescriptionGet(IMMND_CB *cb, const IMMSV_OCTET_STRING* clName,
    struct ImmsvOmClassDescr* res)
{
    return ImmModel::instance(&cb->immModel)->classDescriptionGet(clName, res);
}


void
immModel_fetchAdmOpContinuations(IMMND_CB *cb, SaInvocationT inv, 
    SaBoolT local, SaUint32T* implConn, 
    SaUint32T* reqConn, SaUint64T* reply_dest)
{
    if(local) {
        ImmModel::instance(&cb->immModel)->fetchAdmImplContinuation(inv, implConn,
            reply_dest);
    }
    
    ImmModel::instance(&cb->immModel)->fetchAdmReqContinuation(inv, reqConn);
}

void
immModel_fetchSearchReqContinuation(IMMND_CB *cb, SaInvocationT inv, 
    SaUint32T* reqConn)
{
    ImmModel::instance(&cb->immModel)->fetchSearchReqContinuation(inv, reqConn);
}

void
immModel_fetchSearchImplContinuation(IMMND_CB *cb, SaUint32T searchId,
    SaUint32T reqNodeId,
    MDS_DEST* reply_dest)
{
    SaInvocationT tmp_hdl = m_IMMSV_PACK_HANDLE(searchId, reqNodeId);
    ImmModel::instance(&cb->immModel)->
        fetchSearchImplContinuation(tmp_hdl, (SaUint64T *) reply_dest);
}

void
immModel_prepareForLoading(IMMND_CB *cb)
{
    ImmModel::instance(&cb->immModel)->prepareForLoading();
}


SaBoolT 
immModel_readyForLoading(IMMND_CB *cb)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->readyForLoading();
}

SaInt32T
immModel_getLoader(IMMND_CB *cb)
{
    return ImmModel::instance(&cb->immModel)->getLoader();
}

void
immModel_setLoader(IMMND_CB *cb, SaInt32T loaderPid)
{
    ImmModel::instance(&cb->immModel)->setLoader(loaderPid);
}

void
immModel_recognizedIsolated(IMMND_CB *cb)
{
    ImmModel::instance(&cb->immModel)->recognizedIsolated();
}

SaBoolT 
immModel_syncComplete(IMMND_CB *cb)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->syncComplete(0);
}

SaBoolT 
immModel_ccbsTerminated(IMMND_CB *cb)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->ccbsTerminated();
}

SaBoolT
immModel_immNotWritable(IMMND_CB *cb)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->immNotWritable();
}

SaBoolT
immModel_pbeIsInSync(IMMND_CB *cb)
{
    return (SaBoolT) ImmModel::instance(&cb->immModel)->pbeIsInSync();
}

SaImmRepositoryInitModeT
immModel_getRepositoryInitMode(IMMND_CB *cb)
{
    return (SaImmRepositoryInitModeT)
        ImmModel::instance(&cb->immModel)->getRepositoryInitMode();
}

void
immModel_prepareForSync(IMMND_CB *cb, SaBoolT isJoining)
{
    ImmModel::instance(&cb->immModel)->prepareForSync(isJoining);
}

void
immModel_discardContinuations(IMMND_CB *cb, SaUint32T deadConn)
{
    ImmModel::instance(&cb->immModel)->discardContinuations(deadConn);
}

SaAisErrorT
immModel_objectSync(IMMND_CB *cb, 
    const struct ImmsvOmObjectSync* req)
{
    return ImmModel::instance(&cb->immModel)->objectSync(req);
}

SaAisErrorT
immModel_finalizeSync(IMMND_CB *cb, 
    struct ImmsvOmFinalizeSync* req,
    SaBoolT isCoord, SaBoolT isSyncClient)
{
    return ImmModel::instance(&cb->immModel)->finalizeSync(req, 
        isCoord,
        isSyncClient);
}

SaUint32T
immModel_adjustEpoch(IMMND_CB *cb, SaUint32T suggestedEpoch,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId, SaBoolT increment)
{
    return ImmModel::instance(&cb->immModel)->adjustEpoch(suggestedEpoch,
        continuationId, pbeConn, pbeNodeId, increment);
}

SaUint32T
immModel_adminOwnerChange(IMMND_CB *cb, 
    const struct immsv_a2nd_admown_set* req,
    SaBoolT isRelease)
{
    return ImmModel::instance(&cb->immModel)->adminOwnerChange(req, isRelease);
}

SaAisErrorT
immModel_rtObjectCreate(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectCreate* req,
    SaUint32T implConn,
    SaClmNodeIdT implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId)
{
    return ImmModel::instance(&cb->immModel)->
        rtObjectCreate(req, implConn, (unsigned int) implNodeId, continuationId, 
            pbeConn, pbeNodeId);

}

SaAisErrorT
immModel_rtObjectDelete(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectDelete* req,
    SaUint32T implConn,
    SaClmNodeIdT implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConn,
    SaClmNodeIdT* pbeNodeId,
    SaStringT **objNameArr,
    SaUint32T* arrSizePtr)
{
    ObjectNameVector ov;
    ObjectNameVector::iterator oni;
    unsigned int ix = 0;
    assert(arrSizePtr);

    SaAisErrorT err = ImmModel::instance(&cb->immModel)->
        rtObjectDelete(req, implConn, (unsigned int) implNodeId,
            continuationId, pbeConn, pbeNodeId, ov);

    (*arrSizePtr) = ov.size();
    if((err == SA_AIS_OK) && (*arrSizePtr)) {
       *objNameArr = (SaStringT *) malloc((*arrSizePtr)* sizeof(SaStringT));

       for(oni=ov.begin(); oni != ov.end(); ++oni, ++ix) {
           std::string delObjName = (*oni);
           (*objNameArr)[ix] = (SaStringT) malloc(delObjName.size() + 1);
           strncpy((*objNameArr)[ix], delObjName.c_str(), delObjName.size()+1);
       }
    }
    assert(ix==(*arrSizePtr));
    return err;
}

SaAisErrorT
immModel_rtObjectUpdate(IMMND_CB *cb, 
    const struct ImmsvOmCcbObjectModify* req,
    SaUint32T implConn,
    SaClmNodeIdT implNodeId,
    unsigned int* isPureLocal,
    SaUint32T *continuationId,
    SaUint32T *pbeConn,
    SaClmNodeIdT *pbeNodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    bool isPl = *isPureLocal;
    TRACE_5("on enter isPl:%u", isPl);
    err =  ImmModel::instance(&cb->immModel)->
        rtObjectUpdate(req, implConn, (unsigned int) implNodeId, &isPl,
            continuationId, pbeConn, pbeNodeId);
    TRACE_5("on leave isPl:%u", isPl);
    *isPureLocal = isPl;
    TRACE_LEAVE();
    return err;
}

/*====================================================================*/

ImmModel::ImmModel() : 
    loaderPid(-1)
{
}

bool
ImmModel::immNotWritable()
{
    switch(sImmNodeState){ 
        case IMM_NODE_R_AVAILABLE:
        case IMM_NODE_UNKNOWN:
        case IMM_NODE_ISOLATED:
            return true;
            
        case IMM_NODE_W_AVAILABLE:
        case IMM_NODE_FULLY_AVAILABLE:
        case IMM_NODE_LOADING:
            return false;
            
        default:
            LOG_ER( "Impossible node state, will terminate");
            assert(0);
            
    }  
}

/* immNotPbeWritable returning true means:
   (1) immNotWriteable is true OR... 
   (2) immNotWritable is false (imm service is writable), but according to
   configuration there should be a persistent back-end (Pbe) and the Pbe is
   currently not operational.

   Pbe is dynamically disabled by setting immInitMode to SA_IMM_INIT_FROM_FILE.
*/
bool
ImmModel::immNotPbeWritable()
{
    SaUint32T dummyCon;
    unsigned int dummyNode;
    /* Not writable => Not persitent writable. */
    if(immNotWritable()) {return true;}

    /* INIT_FROM_FILE => no PBE => Writable => PersistentWritable */
    if(immInitMode == SA_IMM_INIT_FROM_FILE) {return false;}

    if(immInitMode != SA_IMM_KEEP_REPOSITORY) {
        LOG_ER("Illegal value on RepositoryInitMode:%u", immInitMode);
        immInitMode = SA_IMM_INIT_FROM_FILE; 
        return false;
    }

    /* immInitMode == SA_IMM_KEEP_REPOSITORY */
    /* Check if PBE OI is available. */

    
    if(getPbeOi(&dummyCon, &dummyNode)) {
        return false; /* Pbe IS present => writable. */
    }

    /* Pbe SHOULD be present but is NOT */
    return true; 
}


void
ImmModel::prepareForLoading()
{
    switch(sImmNodeState){ 
        case  IMM_NODE_UNKNOWN:
            sImmNodeState = IMM_NODE_LOADING;
            LOG_IN("NODE STATE-> IMM_NODE_LOADING");
            break;
            
        case IMM_NODE_ISOLATED: 
            LOG_IN("Node is not ready to participate in loading, "
                "will wait and be synced instead.");    
            break;
        case IMM_NODE_LOADING:
        case IMM_NODE_FULLY_AVAILABLE:
        case IMM_NODE_W_AVAILABLE:
        case IMM_NODE_R_AVAILABLE:
            LOG_ER( "Node is in a state that cannot accept start "
                "of loading, will terminate");
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
            
    }
}

bool
ImmModel::readyForLoading()
{
    return sImmNodeState == IMM_NODE_LOADING;
}

void
ImmModel::recognizedIsolated()
{
    switch(sImmNodeState) {
        case  IMM_NODE_UNKNOWN:
            sImmNodeState = IMM_NODE_ISOLATED;
            LOG_IN("NODE STATE-> IMM_NODE_ISOLATED");
            break;
            
        case IMM_NODE_ISOLATED: 
            LOG_IN("Redundant sync request, when IMM_NODE_ISOLATED");
            break;
        case IMM_NODE_W_AVAILABLE: 
            LOG_WA("Redundant sync request, when IMM_NODE_W_AVAILABLE");
            break;
            
        case IMM_NODE_FULLY_AVAILABLE:
        case IMM_NODE_LOADING:
        case IMM_NODE_R_AVAILABLE:
            LOG_ER("Node is in a state that cannot request sync, will terminate");
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
    }
}

void
ImmModel::prepareForSync(bool isJoining)
{
    switch(sImmNodeState){ 
        
        case IMM_NODE_ISOLATED: 
            assert(isJoining);
            sImmNodeState = IMM_NODE_W_AVAILABLE; //accept the sync messages
            LOG_IN("NODE STATE-> IMM_NODE_W_AVAILABLE");
            break;
            
        case IMM_NODE_FULLY_AVAILABLE:
            assert(!isJoining);
            sImmNodeState = IMM_NODE_R_AVAILABLE; //Stop mutations.
            LOG_IN("NODE STATE-> IMM_NODE_R_AVAILABLE");
            break;
            
        case  IMM_NODE_UNKNOWN:
            LOG_IN("Node is not ready to participate in sync, "
                "will wait and be synced later.");    
            break;

        case IMM_NODE_LOADING:
        case IMM_NODE_W_AVAILABLE:
        case IMM_NODE_R_AVAILABLE:
            LOG_ER("Node is in a state that cannot accept start of sync, "
                "will terminate");
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
            
    }
}

bool
ImmModel::syncComplete(bool isJoining)
{
    //Dont need the isJoining argument for now.
    
    return sImmNodeState == IMM_NODE_FULLY_AVAILABLE;
}

void
ImmModel::pbePrtoPurgeMutations(unsigned int nodeId, ConnVector& connVector)
{
    ObjectMutationMap::iterator omuti;
    ObjectMap::iterator oi;
    SaInvocationT inv;
    ContinuationMap2::iterator ci;
    ImmAttrValueMap::iterator oavi;
    ObjectInfo *afim = NULL;
    TRACE_ENTER();
    bool dummy=false;

    for(omuti=sPbeRtMutations.begin(); 
        omuti!=sPbeRtMutations.end(); ++omuti) {
        ObjectMutation* oMut = omuti->second;
        oi = sObjectMap.find(omuti->first);
        assert(oi != sObjectMap.end() ||
            (oMut->mOpType == IMM_CREATE_CLASS)||
            (oMut->mOpType == IMM_DELETE_CLASS)||
            (oMut->mOpType == IMM_UPDATE_EPOCH));

        inv = m_IMMSV_PACK_HANDLE(oMut->mContinuationId, nodeId);
        ci = sPbeRtReqContinuationMap.find(inv);

        if(ci != sPbeRtReqContinuationMap.end()) {
            if(oi != sObjectMap.end()) {
                /* Only reply the default reply of TRY_AGAIN on failed
                   RTO ops, not failed class ops. The RTO ops are
                   reverted on fauilure here, which is consistent with
                   a reply of TRY_AGAIN. The class ops are NOT reverted,
                   so a reply of TRY_AGAIN would be incorrect and a
                   reply of OK would be premature. For class ops
                   that have trouble with the PBE, ERR_TIMEOUT would
                   be the only proper reply, so we let the client
                   timeout by not replying.
                */
                connVector.push_back(ci->second.mConn);
            }
            sPbeRtReqContinuationMap.erase(ci);
        }

        /* Fall back. */
        switch(oMut->mOpType) {
            case IMM_CREATE:
                assert(oi->second == oMut->mAfterImage);

                oMut->mAfterImage->mObjFlags &= ~IMM_CREATE_LOCK;
                oMut->mAfterImage = NULL;
                LOG_WA("Create of PERSISTENT runtime object '%s' REVERTED ",
                    omuti->first.c_str());
                assert(deleteRtObject(oi, true, NULL, dummy) == SA_AIS_OK);
            break;

            case IMM_DELETE:
                    assert(oi->second == oMut->mAfterImage);

                    oMut->mAfterImage->mObjFlags &= ~IMM_DELETE_LOCK;
                    oMut->mAfterImage = NULL;
                    /* A delete may delete a tree including both persistent
                       and non persistent RTOs. The roll back here has to
                       revert all the deletes, not just the persistent RTOs.
                    */
                    LOG_WA("Delete of runtime object '%s' REVERTED ",
                        omuti->first.c_str());
                break;

            case IMM_MODIFY:
                    assert(oi->second != oMut->mAfterImage);/* NOT equal */
                    oi->second->mObjFlags &= ~IMM_RT_UPDATE_LOCK;
                    afim =  oMut->mAfterImage;
                    for(oavi =  afim->mAttrValueMap.begin(); 
                        oavi != afim->mAttrValueMap.end(); ++oavi) {
                            delete oavi->second;
                    }
                    afim->mAttrValueMap.clear(); 
                    afim->mAdminOwnerAttrVal=0;
                    afim->mClassInfo=0;
                    afim->mImplementer=0;
                    delete afim;
                    oMut->mAfterImage = NULL;
                    LOG_WA("update of PERSISTENT runtime attributes in object '%s' REVERTED.",
                        omuti->first.c_str());
                break;

            case IMM_CREATE_CLASS:
                    LOG_WA("PBE failed in persistification of class create %s",
                        omuti->first.c_str());
                    /* The PBE should have aborted when this happened.
                       The existence of this mutation would prevent a restart
                       with --recover. Instead a restart will regenerate the
                       PBE file, forcing it in line with the imms runtime
                       state.
                    */
                break;

            case IMM_DELETE_CLASS:
                    LOG_WA("PBE failed in persistification of class delete %s",
                        omuti->first.c_str());
                    /* The PBE should have aborted when this happened.
                       The existence of this mutation would prevent a restart
                       with --recover. Instead a restart will regenerate the
                       PBE file, forcing it in line with the imms runtime
                       state.
                    */

                break;

            case IMM_UPDATE_EPOCH:
                    LOG_WA("PBE failed in persistification of update epoch");
                    /* The PBE should have aborted when this happened.
                       The existence of this mutation would prevent a restart
                       with --recover. Instead a restart will regenerate the
                       PBE file, forcing it in line with the imms runtime
                       state.
                    */

                break;


            default:
                assert(0);
        }
        delete oMut;
    }

    sPbeRtMutations.clear();
    sPbeRtReqContinuationMap.clear();
    TRACE_LEAVE();
}

void
ImmModel::abortSync()
{
    switch(sImmNodeState){ 
        
        case IMM_NODE_R_AVAILABLE:
            sImmNodeState = IMM_NODE_FULLY_AVAILABLE; 
            LOG_IN("NODE STATE-> IMM_NODE_FULLY_AVAILABLE (%u)", 
                __LINE__);
            break;
            
        case IMM_NODE_UNKNOWN:
        case IMM_NODE_ISOLATED: 
            break;

        case IMM_NODE_FULLY_AVAILABLE:
            LOG_WA("Abort sync received while being fully available, "
                "should not happen.");
            break;
            
        case IMM_NODE_W_AVAILABLE:
            sImmNodeState = IMM_NODE_UNKNOWN;
            LOG_IN("NODE STATE-> IMM_NODE_UNKNOW %u", __LINE__);
            /* Aborting a started but not completed sync. */
            LOG_NO("Abort sync: Discarding synced objects");
            while(sObjectMap.size()) {
                ObjectMap::iterator oi = sObjectMap.begin();
                TRACE("sObjectmap.size:%u delete: %s", 
                    (unsigned int) sObjectMap.size(), oi->first.c_str());
                commitDelete(oi->first);
            }

            LOG_NO("Abort sync: Discarding synced classes");
            while(sClassMap.size()) {
                ClassMap::iterator ci = sClassMap.begin();
                TRACE("Removing Class:%s", ci->first.c_str());
                assert(ci->second->mRefCount == 0);
                while(ci->second->mAttrMap.size()) {
                    AttrMap::iterator ai = ci->second->mAttrMap.begin();
                    TRACE("Remove Attr:%s", ai->first.c_str());
                    AttrInfo* ainfo = ai->second;
                    assert(ainfo);
                    delete(ainfo);
                    ci->second->mAttrMap.erase(ai);
                }
                delete ci->second;
                updateImmObject(ci->first, true);
                sClassMap.erase(ci);
            }

            assert(!sOwnerVector.size());
            assert(!sCcbVector.size());
            assert(!sImplementerVector.size());
            assert(!sMissingParents.size());
            break;

        case IMM_NODE_LOADING:
            
            LOG_ER("Node is in a state %u that cannot accept abort "
                " of sync, will terminate", sImmNodeState);
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
            
    }
}

/**
 * Sets the pid for the loader process. Bars other processes
 * from accessing the IMM model during loading. 
 *
 */
void
ImmModel::setLoader(int pid) 
{
    loaderPid = pid;
    if(pid == 0) {
        sImmNodeState = IMM_NODE_FULLY_AVAILABLE;
        LOG_IN("NODE STATE-> IMM_NODE_FULLY_AVAILABLE %u", 
            __LINE__);

        immInitMode = getRepositoryInitMode();
        if(immInitMode == SA_IMM_KEEP_REPOSITORY) {
            LOG_IN("RepositoryInitModeT is SA_IMM_KEEP_REPOSITORY");
        } else {
            LOG_IN("RepositoryInitModeT is SA_IMM_INIT_FROM_FILE");
            immInitMode = SA_IMM_INIT_FROM_FILE; /* Ensure valid value*/
        }
    } else {
        TRACE_5("Loading starts, pid:%u", pid);
    }
}

/**
 * Returns the pid for the loader process. Used to check if loading is
 * currently in progress. 
 *
 */
int
ImmModel::getLoader() 
{
    return loaderPid;
}

/**
 * Changes the current epoch to either suggestedEpoch or to
 * the epoch fetched from the imm-object.
 * The former case is expected to be after a sync. The latter
 * case is expected after a reload.
 * Normally the epoch is incremented when suggestedEpoch is <=
 * than current epoch. But if the 'inrement' arg is false then
 * no incrementing is done here. This is used when only the side
 * effects of adjustEpoch are desired (pushing epoch to IMMD and PBE).
 */
int
ImmModel::adjustEpoch(int suggestedEpoch,
    SaUint32T* continuationIdPtr,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr,
    bool increment)
{
    int restoredEpoch = 0;
    ImmAttrValueMap::iterator avi;
    ObjectInfo* immObject = NULL;
    ObjectMap::iterator oi = sObjectMap.find(immObjectDn);
    if(oi != sObjectMap.end()) {
        immObject = oi->second;
        avi = immObject->mAttrValueMap.find(immAttrEpoch);
        
        if(avi != immObject->mAttrValueMap.end()) {
            assert(!avi->second->isMultiValued());
            restoredEpoch = avi->second->getValue_int();
        }
    }
    
    if(restoredEpoch < 1) {
        restoredEpoch = 1;
    }
    
    if(suggestedEpoch <= restoredEpoch) {
        suggestedEpoch = restoredEpoch;
        if(increment) {
            ++suggestedEpoch;
        }
    }
    
    if(increment && immObject && avi != immObject->mAttrValueMap.end()) {
        avi->second->setValue_int(suggestedEpoch);
        LOG_NO("Epoch set to %u in ImmModel", suggestedEpoch);
    }

    if(pbeNodeIdPtr && getPbeOi(pbeConnPtr, pbeNodeIdPtr)) {
        *continuationIdPtr = ++sLastContinuationId;
        if(sLastContinuationId >= 0xfffffffe)
        {sLastContinuationId = 1;}
        TRACE("continuation generated: %u", *continuationIdPtr);

        if(sPbeRtMutations.find(immObjectDn) == sPbeRtMutations.end()) {
           ObjectMutation* oMut = new ObjectMutation(IMM_UPDATE_EPOCH);
           oMut->mContinuationId = (*continuationIdPtr);
           oMut->mAfterImage = NULL;
           sPbeRtMutations[immObjectDn] = oMut;
        } else {
            LOG_WA("Continuation for Pbe mutation on %s already exists", immObjectDn.c_str());
        }
    }
    
    return suggestedEpoch;
}

/**
 * Fetches the SaImmRepositoryInitT value of the attribute
 * 'saImmRepositoryInit' in the object immManagementDn.
 * If not found then return SA_IMM_INIT_FROM_FILE as default.
 */
SaImmRepositoryInitModeT 
ImmModel::getRepositoryInitMode() 
{
    ImmAttrValueMap::iterator avi;
    ObjectInfo* immMgObject = NULL;
    ObjectMap::iterator oi = sObjectMap.find(immManagementDn);
    if(oi != sObjectMap.end()) {
        immMgObject = oi->second;
        avi = immMgObject->mAttrValueMap.find(saImmRepositoryInit);
        
        if(avi != immMgObject->mAttrValueMap.end()) {
            assert(!avi->second->isMultiValued());
            return (SaImmRepositoryInitModeT) avi->second->getValue_int();
        }
    }

    TRACE_2("%s not found or %s not found, returning INIT_FROM_FILE",
        immManagementDn.c_str(), saImmRepositoryInit.c_str());
    return SA_IMM_INIT_FROM_FILE;
}

/**
 * Fetches the nodeId and possibly connection id for the
 * implementer connected to the class OPENSAF_IMM_CLASS_NAME.
 * This is used both when testing for the presence of the PBE
 * and for locating the PBE when a message/upcall is to be sent 
 * to it.
 */
void *
ImmModel::getPbeOi(SaUint32T* pbeConn, unsigned int* pbeNode)
{
    std::string opensafClass(OPENSAF_IMM_CLASS_NAME);
    ClassMap::iterator ci = sClassMap.find(opensafClass);
    assert(ci!=sClassMap.end());
    ClassInfo* classInfo = ci->second;
    if((classInfo->mImplementer && 
       classInfo->mImplementer->mId &&
       !(classInfo->mImplementer->mDying))) {
        *pbeConn = classInfo->mImplementer->mConn;
        *pbeNode = classInfo->mImplementer->mNodeId;
        return classInfo->mImplementer;
    }
    *pbeConn = 0;
    *pbeNode = 0;
    return NULL;
}

/** 
 * Returns the only instance of ImmModel.
 */
ImmModel*
ImmModel::instance(void** sInstancep)
{
    //static ImmModel* sInstance = 0;
    
    if (*sInstancep == NULL) {
        *sInstancep = new ImmModel();
    }
    
    return (ImmModel*) *sInstancep;
}

/** 
 * Creates a class. 
 */
SaAisErrorT
ImmModel::classCreate(const ImmsvOmClassDescr* req,
        SaUint32T reqConn,
        unsigned int nodeId,
        SaUint32T* continuationIdPtr,
        SaUint32T* pbeConnPtr,
        unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER2("cont:%p connp:%p nodep:%p", continuationIdPtr, pbeConnPtr, pbeNodeIdPtr);
    size_t sz = strnlen((char *) req->className.buf, (size_t)req->className.size);
    
    std::string className((const char*)req->className.buf, sz);
    SaAisErrorT err = SA_AIS_OK;
    
    if(immNotPbeWritable()) {
        return SA_AIS_ERR_TRY_AGAIN;
    }

    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ClassMap::iterator i = sClassMap.find(className);
        if (i != sClassMap.end()) {
            TRACE_7("ERR_EXIST: class '%s' exist", className.c_str());
            err = SA_AIS_ERR_EXIST;
        } else {
            TRACE_5("CREATE CLASS '%s' category:%u", className.c_str(),
                req->classCategory);
            ClassInfo* classInfo = new ClassInfo(req->classCategory);
            
            int rdns=0;
            int illegal=0;
            bool persistentRt = false;
            bool persistentRdn = false;
            ImmsvAttrDefList* list = req->attrDefinitions;
            //ImmsvAttrDefinition* attr = req->attrDefinitions.list.array[j];
            //for (int j = 0; 
            //   j < req->attrDefinitions.list.count && err == SA_AIS_OK; 
            //   j++) {
            while(list) {
                ImmsvAttrDefinition* attr = &list->d;
                sz = strnlen((char *) attr->attrName.buf, 
                    (size_t)attr->attrName.size);
                std::string attrName((const char*)attr->attrName.buf, sz);
                const char* attNm = attrName.c_str();
                
                if((attr->attrFlags & SA_IMM_ATTR_CONFIG) &&
                    (attr->attrFlags & SA_IMM_ATTR_RUNTIME)) {
                    LOG_NO("ERR_INVALID_PARAM: Attribute '%s' can not be both SA_IMM_ATTR_CONFIG "
                        "and SA_IMM_ATTR_RUNTIME", attNm);
                    illegal = 1;
                }
                
                if(attr->attrFlags & SA_IMM_ATTR_CONFIG) {
                    
                    if(req->classCategory == SA_IMM_CLASS_RUNTIME) {
                        LOG_NO("ERR_INVALID_PARAM: Runtime objects can not have config attributes '%s'",
                            attNm);
                        illegal = 1;
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_PERSISTENT) {
                        LOG_NO("ERR_INVALID_PARAM: Config attribute '%s' can not be declared "
                            "SA_IMM_ATTR_PERSISTENT", attNm);
                        illegal = 1;
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_CACHED) {
                        LOG_NO("ERR_INVALID_PARAM: Config attribute '%s' can not be declared "
                            "SA_IMM_ATTR_CACHED", attNm);
                        illegal = 1;
                    }
                    
                } else if(attr->attrFlags & SA_IMM_ATTR_RUNTIME) {
                    
                    if((attr->attrFlags & SA_IMM_ATTR_PERSISTENT) &&
                        !(attr->attrFlags & SA_IMM_ATTR_CACHED)){
                        attr->attrFlags |= SA_IMM_ATTR_CACHED;
                        LOG_NO("persistent implies cached as of SAI-AIS-IMM-A.02.01 "
                            "class:%s atrName:%s - corrected", className.c_str(), attNm);
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_WRITABLE) {
                        LOG_NO("ERR_INVALID_PARAM: Runtime attribute '%s' can not be declared "
                            "SA_IMM_ATTR_WRITABLE", attNm);
                        illegal = 1;
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_INITIALIZED) {
                        LOG_NO("ERR_INVALID_PARAM: Runtime attribute '%s' can not be declared "
                            "SA_IMM_ATTR_INITIALIZED", attNm);
                        illegal = 1;
                    }
                    
                    if((req->classCategory == SA_IMM_CLASS_RUNTIME) &&
                        (attr->attrFlags & SA_IMM_ATTR_PERSISTENT)) {
                        TRACE_5("PERSISTENT RT %s:%s", className.c_str(), attNm);
                        persistentRt = true;
                        if(attr->attrFlags & SA_IMM_ATTR_RDN) {
                            TRACE_5("PERSISTENT RT && PERSISTENT RDN %s:%s", 
                                className.c_str(), attNm);
                            persistentRdn = true;
                        }
                    }
                } else {
                    LOG_NO("ERR_INVALID_PARAM: Attribute '%s' is neither SA_IMM_ATTR_CONFIG nor "
                        "SA_IMM_ATTR_RUNTIME", attNm);
                    illegal = 1;
                }
                
                if(attr->attrFlags & SA_IMM_ATTR_RDN) {
                    ++rdns;
                    
                    if((req->classCategory == SA_IMM_CLASS_CONFIG) &&
                        (attr->attrFlags & SA_IMM_ATTR_RUNTIME)) {
                        LOG_NO("ERR_INVALID_PARAM: RDN '%s' of a configuration object "
                            "can not be a runtime attribute", attNm);
                        illegal = 1;
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_WRITABLE) {
                        LOG_NO("ERR_INVALID_PARAM: RDN attribute '%s' can not be SA_IMM_ATTR_WRITABLE",
                            attNm);
                        illegal = 1;
                    }
                    
                    if((req->classCategory == SA_IMM_CLASS_RUNTIME) &&
                        !(attr->attrFlags & SA_IMM_ATTR_CACHED)) {
                        LOG_NO("ERR_INVALID_PARAM: RDN '%s' of a runtime object must be declared cached",
                            attNm);
                        illegal = 1;
                    }
                }
                
                if(attr->attrDefaultValue) {
                    if(attr->attrFlags & SA_IMM_ATTR_RDN) {
                        LOG_NO("ERR_INVALID_PARAM: RDN '%s' can not have a default", attNm);
                        illegal = 1;
                    }
                    
                    if((attr->attrFlags & SA_IMM_ATTR_RUNTIME) &&
                        !(attr->attrFlags & SA_IMM_ATTR_CACHED) && 
                        !(attr->attrFlags & SA_IMM_ATTR_PERSISTENT))
                    {
                        LOG_NO("ERR_INVALID_PARAM: Runtime attribute '%s' is neither cached nor "
                            "persistent => can not have a default", attNm);
                        illegal = 1;
                    }
                    
                    if(attr->attrFlags & SA_IMM_ATTR_INITIALIZED) {
                        LOG_NO("ERR_INVALID_PARAM: Attribute %s declared as SA_IMM_ATTR_INITIALIZED "
                            "inconsistent with having default", attNm);
                        illegal = 1;
                    }
                }
                err = attrCreate(classInfo, attr);
                if(err != SA_AIS_OK) {
                    illegal = 1;
                }
                list = list->next;
            }
            
            if(persistentRt && !persistentRdn) {
                LOG_NO("ERR_INVALID_PARAM: Class for persistent runtime object requires "
                    "persistent RDN");
                illegal = 1;
            }
            
            if(rdns != 1) {
                LOG_NO("ERR_INVALID_PARAM: ONE and only ONE RDN attribute must be defined!");
                illegal = 1;
            }
            
            if(illegal) {
                
                LOG_NO("ERR_INVALID_PARAM: Problem with class '%s'", className.c_str());
                
                err = SA_AIS_ERR_INVALID_PARAM;

                while(classInfo->mAttrMap.size()) {
                    AttrMap::iterator ai = classInfo->mAttrMap.begin();
                    AttrInfo* ainfo = ai->second;
                    assert(ainfo);
                    delete(ainfo);
                    classInfo->mAttrMap.erase(ai);
                }
                delete classInfo;
            } else {
                sClassMap[className] = classInfo;
                updateImmObject(className);
                if(pbeNodeIdPtr) {
                    if(!getPbeOi(pbeConnPtr, pbeNodeIdPtr)) {
                        LOG_ER("Pbe is not available, can not happen here");
                        abort();
                    }
                    ++sLastContinuationId;
                    if(sLastContinuationId >= 0xfffffffe) {
                            sLastContinuationId = 1;
                    }
                    (*continuationIdPtr) = sLastContinuationId;
                    /* There is a tiny risk here that there exists a PRTO with DN
                       identical to the classname and that a PRTO operation on THIS
                       object is performed concurrently with this class create!
                    */
                    ObjectMutationMap::iterator i2 = sPbeRtMutations.find(className);
                    if(i2 == sPbeRtMutations.end()) {
                        /* Object mutation to bar pbe restart --recover
                           This is actually a class mutation, but lets keep the name.
                        */
                        ObjectMutation* oMut = new ObjectMutation(IMM_CREATE_CLASS);
                        oMut->mContinuationId = (*continuationIdPtr);
                        oMut->mAfterImage = NULL;
                        sPbeRtMutations[className] = oMut;

                        if(reqConn) {
                            SaInvocationT tmp_hdl =
                                m_IMMSV_PACK_HANDLE((*continuationIdPtr), nodeId);
                            sPbeRtReqContinuationMap[tmp_hdl] =
                                ContinuationInfo2(reqConn, DEFAULT_TIMEOUT_SEC);
                        }
                    } else {
                        LOG_WA("PBE class create unprotected because of concurrent "
                           "conflicting PRTO operation on same name '%s'",
                           className.c_str());
                    }
                }
            }
        }
    }
    TRACE_LEAVE();
    return err;
}

/** 
 * Deletes a class. 
 */
SaAisErrorT
ImmModel::classDelete(const ImmsvOmClassDescr* req,
        SaUint32T reqConn,
        unsigned int nodeId,
        SaUint32T* continuationIdPtr,
        SaUint32T* pbeConnPtr,
        unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER();
    
    size_t sz = strnlen((char *) req->className.buf, 
        (size_t)req->className.size);
    std::string className((const char*)req->className.buf, sz);
    
    SaAisErrorT err = SA_AIS_OK;
    
    if(immNotPbeWritable()) {
        err = SA_AIS_ERR_TRY_AGAIN;
    } else if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ClassMap::iterator i = sClassMap.find(className);
        if (i == sClassMap.end()) {
            TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", 
                className.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
        } else if (i->second->mRefCount > 0) {
            LOG_IN("ERR_BUSY: class '%s' busy, refCount:%u", 
                className.c_str(), i->second->mRefCount);
            err = SA_AIS_ERR_BUSY;
        } else {
            while(i->second->mAttrMap.size()) {
                AttrMap::iterator ai = i->second->mAttrMap.begin();
                AttrInfo* ainfo = ai->second;
                assert(ainfo);
                delete(ainfo);
                i->second->mAttrMap.erase(ai);
            }
            delete i->second;
            sClassMap.erase(i);
            updateImmObject(className, true);
            TRACE_5("class %s deleted", className.c_str());

            if(pbeNodeIdPtr) {
                if(!getPbeOi(pbeConnPtr, pbeNodeIdPtr)) {
                    LOG_ER("Pbe is not available, can not happen here");
                    abort();
                }
                ++sLastContinuationId;
                if(sLastContinuationId >= 0xfffffffe) {
                        sLastContinuationId = 1;
                }
                (*continuationIdPtr) = sLastContinuationId;
                /* There is a tiny risk here that there exists a PRTO with DN
                   identical to the classname and that a PRTO operation on THIS
                   object is performed concurrently with this class delete!
                */
                ObjectMutationMap::iterator i2 = sPbeRtMutations.find(className);
                if(i2 == sPbeRtMutations.end()) {
                    /* Object mutation to bar pbe restart --recover
                       This is actually a class mutation, but lets keep the name.
                    */
                        ObjectMutation* oMut = new ObjectMutation(IMM_DELETE_CLASS);
                        oMut->mContinuationId = (*continuationIdPtr);
                        oMut->mAfterImage = NULL;
                        sPbeRtMutations[className] = oMut;

                        if(reqConn) {
                            SaInvocationT tmp_hdl =
                                m_IMMSV_PACK_HANDLE((*continuationIdPtr), nodeId);
                            sPbeRtReqContinuationMap[tmp_hdl] =
                                ContinuationInfo2(reqConn, DEFAULT_TIMEOUT_SEC);
                        }
                    } else {
                        LOG_WA("PBE class delete unprotected because of concurrent "
                           "conflicting PRTO operation on same name '%s'",
                           className.c_str());
                    }
                }
        }
    }
    TRACE_LEAVE();
    return err;
}

/** 
 * Creates an attribute.
 */
SaAisErrorT
ImmModel::attrCreate(ClassInfo* classInfo, const ImmsvAttrDefinition* attr)
{
    SaAisErrorT err = SA_AIS_OK;
    //TRACE_ENTER();
    
    size_t sz = strnlen((char *) attr->attrName.buf,
        (size_t)attr->attrName.size);
    std::string attrName((const char*)attr->attrName.buf, sz);
    
    
    if(!schemaNameCheck(attrName)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper attribute name: %s", 
            attrName.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        AttrMap::iterator i = classInfo->mAttrMap.find(attrName);
        if (i != classInfo->mAttrMap.end()) {
            LOG_NO("ERR_INVALID_PARAM: attr def for '%s' is duplicated",
                attrName.c_str());
            err = SA_AIS_ERR_INVALID_PARAM;
        } else {
            TRACE_5("create attribute '%s'", attrName.c_str());
            
            AttrInfo* attrInfo = new AttrInfo;
            attrInfo->mValueType = attr->attrValueType;
            attrInfo->mFlags = attr->attrFlags;
            attrInfo->mNtfId = attr->attrNtfId;
            if(attr->attrDefaultValue) {
                IMMSV_OCTET_STRING tmpos; //temporary octet string
                eduAtValToOs(&tmpos, attr->attrDefaultValue,
                    (SaImmValueTypeT) attr->attrValueType);
                attrInfo->mDefaultValue.setValue(tmpos);
            } 
            classInfo->mAttrMap[attrName] = attrInfo;
        }
    }
    //TRACE_LEAVE();
    return err;
}

struct AttrDescriptionGet
{
    AttrDescriptionGet(ImmsvOmClassDescr*& s) : classDescription(s) { }
    
    AttrMap::value_type operator() (const AttrMap::value_type& item) {
        ImmsvAttrDefList* p = (ImmsvAttrDefList*)
            calloc(1, sizeof(ImmsvAttrDefList)); /*Alloc-X*/
        const std::string& attrName = item.first;
        p->d.attrName.size = (int)attrName.length()+1;          /*Alloc-Y*/
        p->d.attrName.buf = (char *)  malloc(p->d.attrName.size);
        strncpy(p->d.attrName.buf, attrName.c_str(), p->d.attrName.size);
        p->d.attrValueType = item.second->mValueType;
        p->d.attrFlags = item.second->mFlags;
        p->d.attrNtfId = item.second->mNtfId;
        if(item.second->mDefaultValue.empty()) {
            p->d.attrDefaultValue=NULL;
        } else {
            p->d.attrDefaultValue = (IMMSV_EDU_ATTR_VAL *)/*alloc-Z*/
                calloc(1, sizeof(IMMSV_EDU_ATTR_VAL));  
            
            item.second->mDefaultValue.
                copyValueToEdu(p->d.attrDefaultValue, (SaImmValueTypeT)/*alloc-ZZ*/
                    p->d.attrValueType);
        }
        p->next=classDescription->attrDefinitions;
        classDescription->attrDefinitions=p;
        return item;
    }
    
    ImmsvOmClassDescr*& classDescription; //Data member
};

/** 
 * Returns a class description.
 */
SaAisErrorT
ImmModel::classDescriptionGet(const IMMSV_OCTET_STRING* clName, 
    ImmsvOmClassDescr* res)
{
    TRACE_ENTER();
    size_t sz = strnlen((char *) clName->buf, 
        (size_t) clName->size);
    std::string className((const char*)clName->buf, sz);
    
    SaAisErrorT err = SA_AIS_OK;
    
    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ClassMap::iterator i = sClassMap.find(className);
        if (i == sClassMap.end()) {
            TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", className.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
        } else {
            ClassInfo* classInfo = i->second;
            res->classCategory = classInfo->mCategory;
            
            std::for_each(classInfo->mAttrMap.begin(), classInfo->mAttrMap.end(), 
                AttrDescriptionGet(res));
        }
    }
    TRACE_LEAVE();
    return err;
}

struct AttrFlagIncludes
{
    AttrFlagIncludes(SaUint32T attrFlag) : mFlag(attrFlag) { }
    
    bool operator() (AttrMap::value_type& item) const {
        return (item.second->mFlags & mFlag) != 0;
    }
    
    SaUint32T mFlag;
};

struct IdIs
{
    IdIs(SaUint32T id) : mId(id) { }
    
    bool operator() (AdminOwnerInfo*& item) const {
        return item->mId == mId;
    }
    
    SaUint32T mId;
};

struct CcbIdIs
{
    CcbIdIs(SaUint32T id) : mId(id) { }
    
    bool operator() (CcbInfo*& item) const {
        return item->mId == mId;
    }
    
    SaUint32T mId;
};

/** 
 * Creates an admin owner. 
 */
SaAisErrorT
ImmModel::adminOwnerCreate(const ImmsvOmAdminOwnerInitialize* req,
    SaUint32T ownerId, SaUint32T conn, 
    unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    if(strncmp("IMMLOADER", (const char *) req->adminOwnerName.value,
        (size_t) req->adminOwnerName.length) == 0) {
        if(sImmNodeState != IMM_NODE_LOADING) {
            LOG_NO("ERR_INVALID_PARAM: Admin Owner 'IMMLOADER' only allowed for loading");
            TRACE_LEAVE();
            return SA_AIS_ERR_INVALID_PARAM; 
        }
    }
    
    AdminOwnerInfo* info = new AdminOwnerInfo;
    
    info->mId = ownerId;
    
    info->mAdminOwnerName.append((const char*)req->adminOwnerName.value,
        (size_t)req->adminOwnerName.length);
    info->mReleaseOnFinalize = req->releaseOwnershipOnFinalize;
    info->mConn = conn;
    info->mNodeId = nodeId;
    info->mDying = false;
    sOwnerVector.push_back(info);
    TRACE_5("admin owner '%s' %u created", info->mAdminOwnerName.c_str(),
        info->mId);
    
    TRACE_LEAVE();
    return err;
}


/** 
 * Deletes and admin owner.
 */
SaAisErrorT
ImmModel::adminOwnerDelete(SaUint32T ownerId, bool hard)
{
    SaAisErrorT err = SA_AIS_OK;
    
    //Even if the finalize is hard (client crash/detach), we assume
    //the order for hard finalize arrives over fevs, i.e. in the same
    //order position relative other fevs messages, for all IMMNDs.
    //This means that a hard finalize of admin owner can not cause
    //differences in behavior at diferent IMMNDs.
    
    TRACE_ENTER();
    
    AdminOwnerVector::iterator i;
    
    i = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), IdIs(ownerId));
    if (i == sOwnerVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: Admin owner %u does not exist", ownerId);
        err = SA_AIS_ERR_BAD_HANDLE;
    } else {
        unsigned int loopCount=0;
        
        if((*i)->mAdminOwnerName == std::string("IMMLOADER")) {
            //Loader-pid set to zero both in immnd-coord and in other immnds.
            //In coord the loader-pid was positive. 
            //In non-coords the loader-pid was negative.
            if(hard) {
                LOG_WA("Hard close of admin owner IMMLOADER "
                    "=> Loading must have failed");
            } else {
                if(this->getLoader() && (sImmNodeState == IMM_NODE_LOADING)) {
                    LOG_IN("Closing admin owner IMMLOADER, "
                        "loading of IMM done");
                    this->setLoader(0);
                }
            }
        }
        
        if(immNotWritable()) {
            if(hard) {
                LOG_WA("Postponing hard delete of admin owner with id:%u "
                    "when imm is not writable", ownerId);
                (*i)->mDying = true;
                err = SA_AIS_ERR_BUSY;
            } else {
                err = SA_AIS_ERR_TRY_AGAIN;
            }
            goto done;
        }
        
        if((*i)->mDying) {
            LOG_IN("Removing zombie Admin Owner %u %s", ownerId,
                (*i)->mAdminOwnerName.c_str());
        } else {
            TRACE_5("Delete admin owner %u '%s'", ownerId,
                (*i)->mAdminOwnerName.c_str());
        }
        
        if((*i)->mReleaseOnFinalize) {
            ObjectSet::iterator oi = (*i)->mTouchedObjects.begin();
            while(oi!=(*i)->mTouchedObjects.end()) {
                std::string dummyName;
                err = adminOwnerRelease(dummyName, *oi, *i, true);
                if(err != SA_AIS_OK) {
                    //This should never happen.
                    TRACE_5("Internal error when trying to release owner on "
                        "finalize. Application may need to do an "
                        "AdminOwnerClear");
                    //Ignore the error, assume brutal adminOwnerClear was done
                    err = SA_AIS_OK;
                    
                }
                if(++loopCount > 500) {
                    if(loopCount%1000==0) {
                        LOG_ER("Stuck in adminOwnerDelete? %u", loopCount);
                    }
                    if(loopCount > 500000) {
                        LOG_ER("Stuck in adminOwnerDelete!!!!!");
                        err = SA_AIS_ERR_TRY_AGAIN;
                        goto done;
                    }
                }
                //adminOwnerRelease deletes elements from the collection we
                //are iterating over here, which is why this is not a for-loop.
                //Instead, it is a while-loop, where we always fetch the first
                //element, until the collection is empty.
                //We are assuming here that adminOwnerRelease always removes
                //each element from mTouchedObjects. If it does not, 
                //we would get an endless loop here!
                //The above check is an extra precaution to obtain a clear
                //error indication should this happen.
                oi = (*i)->mTouchedObjects.begin();
            }
        }
        (*i)->mId = 0;
        (*i)->mAdminOwnerName.clear();
        (*i)->mConn = 0;
        (*i)->mNodeId = 0;
        (*i)->mTouchedObjects.clear();
        (*i)->mReleaseOnFinalize = false;
        (*i)->mDying = false;
        delete *i;
        sOwnerVector.erase(i);
    }
 done:
    TRACE_LEAVE();
    return err;
}

/** 
 * Admin owner set and release
 */
SaAisErrorT
ImmModel::adminOwnerChange(const struct immsv_a2nd_admown_set* req, 
    bool release)
{
    //This method handles three om-api-calls: 
    //(a) AdminOwnerSet (release is false and req->adminOwnerId is non-zero; 
    //(b) AdminOwnerRelease (release is true and req->adminOwnerId is non-zero;
    //(c) AdminOwnerClear (release is true and req->adminOwnerId is zero.
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    SaUint32T ownerId = req->adm_owner_id;
    
    assert(release || ownerId);  
    SaImmScopeT scope = (SaImmScopeT) req->scope;
    AdminOwnerVector::iterator i = sOwnerVector.end();
    if(ownerId) {
        i = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
            IdIs(ownerId));
        if((i != sOwnerVector.end()) && (*i)->mDying) {
            i = sOwnerVector.end(); //Pretend it does not exist.
            TRACE_5("Attempt to use dead admin owner in adminOwnerChange");
        }
    }
    
    if(ownerId && (i == sOwnerVector.end())) {
        LOG_IN("ERR_BAD_HANDLE: admin owner %u does not exist", ownerId);
        err = SA_AIS_ERR_BAD_HANDLE;
    } else {
        TRACE_5("%s admin owner '%s'", 
            release?(ownerId?"Release":"Clear"):"Set", 
            (ownerId)?(*i)->mAdminOwnerName.c_str():"ALL");
        
        AdminOwnerInfo* adm = ownerId?(*i):NULL;
        
        for(int doIt=0; (doIt < 2) && (err == SA_AIS_OK); ++doIt) {
            IMMSV_OBJ_NAME_LIST* ol = req->objectNames;
            while(ol && err == SA_AIS_OK) {
                std::string objectName((const char *) ol->name.buf,
                    (size_t) strnlen((const char *) ol->name.buf, 
                        (size_t) ol->name.size));
                if(doIt) {
                    TRACE_5("%s Admin Owner for object %s\n", 
                        release?(ownerId?"Release":"Clear"):"Set", 
                        objectName.c_str());
                }

                if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
                    LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
                    assert(!doIt);
                    return SA_AIS_ERR_INVALID_PARAM;
                }

                ObjectMap::iterator i1 = sObjectMap.find(objectName);
                if (i1 == sObjectMap.end()) {
                    TRACE_7("ERR_NOT_EXIST: object '%s' does not exist",
                        objectName.c_str());
                    return SA_AIS_ERR_NOT_EXIST;
                } else {
                    SaUint32T ccbIdOfObj;
                    CcbVector::iterator i2;
                    ObjectInfo* objectInfo = i1->second;
                    ccbIdOfObj = objectInfo->mCcbId;
                    if(!doIt && ccbIdOfObj && release) {
                        //check for ccb interference
                        //Note: interference only possible fore release.
                        //For set the adminowner is either same == noop;
                        //or different => will be caught in adminOwnerSet()
                        i2 = std::find_if(sCcbVector.begin(), sCcbVector.end(),
                            CcbIdIs(ccbIdOfObj));
                        if (i2 != sCcbVector.end() && (*i2)->isActive()) {
                            LOG_IN("ERR_BUSY: ccb id %u active on object %s", 
                                ccbIdOfObj, objectName.c_str());
                            TRACE_LEAVE();
                            return SA_AIS_ERR_BUSY;
                        }
                    }
                    if(release) {
                        err = adminOwnerRelease(objectName, objectInfo, adm, 
                            doIt);
                    } else {
                        err = adminOwnerSet(objectName, objectInfo, adm, doIt);
                    }
                    
                    if(err == SA_AIS_OK && scope != SA_IMM_ONE) {
                        // Find all sub objects to the root object
                        for (i1 = sObjectMap.begin(); 
                             i1 != sObjectMap.end() && err == SA_AIS_OK; i1++){
                            std::string subObjName = i1->first;
                            if (subObjName.length() > objectName.length()) {
                                size_t pos = 
                                    subObjName.length() - objectName.length();
                                if((subObjName.rfind(objectName,pos) == pos) &&
                                    (subObjName[pos-1] == ',')){
                                    if(scope==SA_IMM_SUBTREE || 
                                        checkSubLevel(subObjName, pos)){
                                        ObjectInfo* subObj = i1->second;
                                        ccbIdOfObj = subObj->mCcbId;
                                        if(!doIt && ccbIdOfObj) { 
                                            //check for ccb interference
                                            i2=std::find_if(sCcbVector.begin(),
                                                sCcbVector.end(),
                                                CcbIdIs(ccbIdOfObj));
                                            if (i2 != sCcbVector.end() && (*i2)->isActive()) {
                                                LOG_IN("ERR_BUSY: ccb id %u active on"
                                                    "object %s", ccbIdOfObj,
                                                    subObjName.c_str());
                                                TRACE_LEAVE();
                                                return SA_AIS_ERR_BUSY;
                                            }
                                        }
                                        if(release) {
                                            err = adminOwnerRelease(subObjName,
                                                subObj, adm, doIt);
                                        } else {
                                            err = adminOwnerSet(subObjName,
                                                subObj, adm, doIt);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                ol = ol->next;
            }
        }
    }
    
    TRACE_LEAVE();
    return err;
}

/** 
 * Creates a CCB.
 */
SaAisErrorT
ImmModel::ccbCreate(SaUint32T adminOwnerId,
    SaUint32T ccbFlags,
    SaUint32T ccbId, 
    unsigned int originatingNodeId,
    SaUint32T originatingConn)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    CcbInfo* info = new CcbInfo;
    info->mId = ccbId;
    info->mAdminOwnerId = adminOwnerId;
    info->mCcbFlags = ccbFlags;
    info->mOriginatingNode = originatingNodeId; 
    info->mOriginatingConn = originatingConn;
    info->mVeto = SA_AIS_OK;
    info->mState = IMM_CCB_EMPTY;
    info->mWaitStartTime = 0;
    info->mOpCount = 0;
    info->mPbeRestartId = 0;
    sCcbVector.push_back(info);
    
    TRACE_5("CCB %u created with admo %u", info->mId, adminOwnerId);
    
    TRACE_LEAVE();
    return err;
}

SaAisErrorT
ImmModel::ccbResult(SaUint32T ccbId)
{
    SaAisErrorT err = SA_AIS_OK;
    CcbVector::iterator i;
    i = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i == sCcbVector.end()) {
        LOG_WA("CCB %u does not exist- probably removed already", ccbId);
        err = SA_AIS_ERR_NO_RESOURCES;
    } else {
        switch ((*i)->mState) {

            case IMM_CCB_ABORTED:
            case IMM_CCB_PBE_ABORT:
                TRACE_5("Fetch ccb result: CCB %u was aborted", ccbId);
                err = SA_AIS_ERR_FAILED_OPERATION;
                break; //Normal

            case IMM_CCB_COMMITTED:
                TRACE_5("Fetch ccb result: CCB %u was committed", ccbId);
                err = SA_AIS_OK;
                break; //Normal
 
            case IMM_CCB_EMPTY:
            case IMM_CCB_READY:
            case IMM_CCB_CREATE_OP:
            case IMM_CCB_MODIFY_OP:
            case IMM_CCB_DELETE_OP:
                LOG_WA("ccbResult: CCB %u is active! state:%u.", ccbId, (*i)->mState);
                    err = SA_AIS_ERR_TRY_AGAIN;
                break; //Unusual

            case IMM_CCB_PREPARE:
                LOG_WA("ccbResult: CCB %u in prepare! Commit/abort in progress?", 
                    ccbId);
                err = SA_AIS_ERR_TRY_AGAIN;
                break; //Unusual

            case IMM_CCB_CRITICAL:
                LOG_WA("ccbResult: CCB %u in critical state! Commit/apply in progress?", ccbId);
                err = SA_AIS_ERR_TRY_AGAIN;
                break; //Can happen if PBE crashes.

            default:
                LOG_ER("ccbResult: Illegal state %u in ccb %u", (*i)->mState, ccbId);
                assert(0);
        }
    }
    return err;
}
/** 
 * Commits a CCB
 */
SaAisErrorT
ImmModel::ccbApply(SaUint32T ccbId, 
    SaUint32T reqConn,
    ConnVector& connVector,
    IdVector& implIds,
    IdVector& continuations)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    TRACE_5("APPLYING CCB ID:%u", ccbId);
    
    CcbVector::iterator i;
    AdminOwnerVector::iterator i2;
    i = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if (i == sCcbVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: CCB %u does not exist", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
    } else {
        CcbInfo* ccb = (*i);
        i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
            IdIs(ccb->mAdminOwnerId));
        if((i2 != sOwnerVector.end()) && (*i2)->mDying) {
            i2 = sOwnerVector.end(); //Pretend it does not exist.
            TRACE_5("Attempt to use dead admin owner in ccbApply");
        }
        if (i2 == sOwnerVector.end()) {
            LOG_IN("ERR_BAD_HANDLE: Admin owner id %u does not exist", 
                ccb->mAdminOwnerId);
            ccb->mVeto = SA_AIS_ERR_BAD_HANDLE;
        } else if(ccb->mState > IMM_CCB_READY) {
            LOG_IN("ERR_FAILED_OPERATION: Ccb not in correct state (%u) for Apply", ccb->mState);
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        } else if(reqConn && (ccb->mOriginatingConn != reqConn)) {
            LOG_IN("ERR_BAD_HANDLE: Missmatch on connection for ccb id %u", ccbId);
            ccb->mVeto = SA_AIS_ERR_BAD_HANDLE;
        } 
        
        if(!ccb->isOk()) {
            err = SA_AIS_ERR_FAILED_OPERATION;
        } else if((sImmNodeState == IMM_NODE_LOADING) && 
            sMissingParents.size()) {
            ObjectNameSet::iterator oni;
            LOG_ER("Can not apply because there are %u missing parents", 
                (unsigned int) sMissingParents.size());
            for(oni=sMissingParents.begin(); oni != sMissingParents.end(); 
                ++oni) {
                LOG_ER("Missing Parent DN: %s", oni->c_str());
            }
            err = SA_AIS_ERR_FAILED_OPERATION;
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        } else {
            /* Remove assert after component test */
            assert(sMissingParents.empty());

            TRACE_5("Apply CCB %u", ccb->mId);
            ccb->mState = IMM_CCB_PREPARE;
            CcbImplementerMap::iterator isi;
            for(isi = ccb->mImplementers.begin();
                isi != ccb->mImplementers.end();
                ++isi) {
                ImplementerCcbAssociation* implAssoc = isi->second;
                ImplementerInfo* impInfo = implAssoc->mImplementer;
                assert(impInfo);
                ++sLastContinuationId; 
                if(sLastContinuationId >= 0xfffffffe) {
                    sLastContinuationId = 1;
                }
                /* incremented sLastContinuationId unconditionally before
                   check on impInfo->mDying because mDying is not purely
                   fevs regulated => we could get discrepancy on sLastContinuationId.
                   The locally lost implementer connection below may seem dangerous
                   because we do an early abort of the apply attempt only on this node,
                   but not globally. But the other nodes will sitll wait for a reply
                   from the dead implementer, which they will never get, i.e. they
                   will abort also. 
                   Important though that the sLastContinuationId is in step.
                 */
                if(impInfo->mDying) {
                    LOG_WA("Lost connection with implementer %s, "
                        "refusing apply", impInfo->mImplementerName.c_str());
                    err = SA_AIS_ERR_FAILED_OPERATION;
                    ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
                    break;
                }
                //Wait for ack, possibly remote
                implAssoc->mWaitForImplAck = true; 
                implAssoc->mContinuationId = sLastContinuationId;/* incremented above */
                if(ccb->mWaitStartTime == 0) {
                    ccb->mWaitStartTime = time(NULL);
                    TRACE("Wait timer for completed started for ccb:%u", 
                        ccb->mId);
                }
                SaUint32T implConn = impInfo->mConn;
                if(implConn) {
                    connVector.push_back(implConn);
                    implIds.push_back(impInfo->mId);
                    continuations.push_back(sLastContinuationId);
                }                
            }
        }
    }
    
    TRACE_LEAVE();
    return err;
}

void
ImmModel::commitCreate(ObjectInfo* obj)
{
    //obj->mCreateLock = false;
    obj->mObjFlags &= ~IMM_CREATE_LOCK;
    /*TRACE_5("Flags after remove create lock:%u", obj->mObjFlags);*/
}

bool
ImmModel::commitModify(const std::string& dn, ObjectInfo* afterImage)
{
    TRACE_ENTER();
    TRACE_5("COMMITING MODIFY of %s", dn.c_str());
    ObjectMap::iterator oi = sObjectMap.find(dn);
    assert(oi != sObjectMap.end());
    ObjectInfo* beforeImage = oi->second;
    
    //  sObjectMap.erase(oi);
    //sObjectMap[dn] = afterImage;
    //instead of switching ObjectInfo record, I move the attribute values
    //from the after-image tothe before-image. This to avoid having to 
    //update stuff such as AdminOwnerInfo->mTouchedObjects
    
    ImmAttrValueMap::iterator oavi;
    for(oavi = beforeImage->mAttrValueMap.begin();
        oavi != beforeImage->mAttrValueMap.end(); ++oavi) {
        delete oavi->second; 
    }
    beforeImage->mAttrValueMap.clear(); 
    
    for(oavi = afterImage->mAttrValueMap.begin();
        oavi != afterImage->mAttrValueMap.end(); ++oavi) {
        beforeImage->mAttrValueMap[oavi->first] = oavi->second;
        if(oavi->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
            beforeImage->mAdminOwnerAttrVal = oavi->second;
        }
    }
    afterImage->mAttrValueMap.clear();
    delete afterImage;
    if(dn == immManagementDn) {
        /* clumsy solution to check every modify for this. 
           TODO: catch this in the modify op OI handler to
           prevent datatype error. ? Problem is that seems not
           tight enough.
        */
        SaImmRepositoryInitModeT oldMode = immInitMode;
        immInitMode = getRepositoryInitMode();
        if(oldMode != immInitMode) {
            if(immInitMode == SA_IMM_KEEP_REPOSITORY) {
                LOG_IN("SaImmRepositoryInitModeT changed to: SA_IMM_KEEP_REPOSITORY");
            } else {
                LOG_IN("SaImmRepositoryInitModeT changed to: SA_IMM_INIT_FROM_FILE");
            }
            TRACE_LEAVE();
            return true;
        }
    }
    TRACE_LEAVE();
    return false;
}

void
ImmModel::commitDelete(const std::string& dn)
{
    TRACE_ENTER();
    TRACE_5("COMMITING DELETE of %s", dn.c_str());
    ObjectMap::iterator oi = sObjectMap.find(dn);
    assert(oi != sObjectMap.end());
    
    ImmAttrValueMap::iterator oavi;
    for(oavi = oi->second->mAttrValueMap.begin();
        oavi != oi->second->mAttrValueMap.end(); ++oavi) {
        delete oavi->second;  
    }
    
    //Empty the collection, probably not necessary (as the ObjectInfo
    //record is deleted below), but does not hurt.
    oi->second->mAttrValueMap.clear(); 
    
    assert(oi->second->mClassInfo->mRefCount > 0);
    oi->second->mClassInfo->mRefCount--;
    
    oi->second->mAdminOwnerAttrVal=0;
    oi->second->mClassInfo=0;
    oi->second->mImplementer=0;
    oi->second->mObjFlags = 0;
    
    TRACE_5("delete object '%s'", oi->first.c_str());
    AdminOwnerVector::iterator i2;
    
    //adminOwner->mTouchedObjects.erase(obj);
    //TODO This is not so efficient. Correct with Ccbs.
    for(i2=sOwnerVector.begin(); i2!=sOwnerVector.end();++i2) {
        (*i2)->mTouchedObjects.erase(oi->second);
        //Note that on STL sets, the erase operation is polymorphic.
        //Here we are erasing based on value, not iterator position. 
    }
    
    delete oi->second;
    sObjectMap.erase(oi);
    TRACE_LEAVE();
}

bool
ImmModel::ccbCommit(SaUint32T ccbId, ConnVector& connVector)
{
    TRACE_ENTER();
    CcbVector::iterator i;
    bool pbeModeChange = false;
    bool ccbNotEmpty = false;

    i = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    assert(i != sCcbVector.end());
    TRACE_5("Commit CCB %u", (*i)->mId);
    CcbInfo* ccb = (*i);
    assert(ccb->isOk());
    if(ccb->mState == IMM_CCB_PREPARE) {
        ccb->mState = IMM_CCB_CRITICAL;
    } else {
        assert(ccb->mState == IMM_CCB_CRITICAL);
        TRACE_5("Ccb %u comitted by persistent back end", ccbId);
    }
    ccb->mWaitStartTime = 0;

    //Do the actual commit!
    ObjectMutationMap::iterator omit;
    for(omit=ccb->mMutations.begin(); omit!=ccb->mMutations.end(); ++omit){
        ccbNotEmpty=true;
        ObjectMutation* omut = omit->second;
        assert(!omut->mWaitForImplAck);
        switch(omut->mOpType){
            case IMM_CREATE:
                TRACE_5("COMMITING CREATE of %s", omit->first.c_str());
                assert(omut->mAfterImage);
                commitCreate(omut->mAfterImage);
                omut->mAfterImage=NULL;
                break;
            case IMM_MODIFY:
                assert(omut->mAfterImage);
                pbeModeChange = commitModify(omit->first, omut->mAfterImage) || pbeModeChange;
                omut->mAfterImage=NULL;
                break;
            case IMM_DELETE:
                assert(omut->mAfterImage==NULL);
                commitDelete(omit->first);
                break;
            default:
                assert(0);
        }//switch
        delete omut;
    }//for
    
    ccb->mMutations.clear();
    
    //If there are implementers involved then send the final apply callback 
    //to them and remove the implementers from the Ccb.
    
    CcbImplementerMap::iterator isi;
    for(isi = ccb->mImplementers.begin();
        isi != ccb->mImplementers.end();
        ++isi) {
        ccbNotEmpty = true;
        ImplementerCcbAssociation* implAssoc = isi->second;
        assert(!(implAssoc->mWaitForImplAck));
        ImplementerInfo* impInfo = implAssoc->mImplementer;
        assert(impInfo);
        SaUint32T implConn = impInfo->mConn;
        if(implConn) {
            if(impInfo->mDying) {
                LOG_ER("LOST CONNECTION WITH IMPLEMENTER %s AFTER COMPLETED OK BUT "
                    "BEFORE APPLY UC. CCB %u will apply ayway. "
                    "Discrepancy between Immsv and this implementer may be "
                    "the result", impInfo->mImplementerName.c_str(), ccbId);
                
            } else {
                connVector.push_back(implConn);
            }
        }
        delete isi->second; //Should not affect the iteration
    }
    
    ccb->mImplementers.clear();
    
    //With FEVS and no PBE the critical phase is trivially completed.
    //We do not wait for replies from peer imm-server-nodes or from
    //implementers on the apply callback (no return code).
    ccb->mState = IMM_CCB_COMMITTED;
    if(ccbNotEmpty) {
        LOG_IN("Ccb %u COMMITTED", ccb->mId);
    }
    return pbeModeChange;
}

void
ImmModel::ccbAbort(SaUint32T ccbId, ConnVector& connVector, SaUint32T* client,
    unsigned int* pbeNodeIdPtr)
{
    SaUint32T pbeConn=0;
    TRACE_ENTER();
    CcbVector::iterator i;
    
    i = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i == sCcbVector.end()) {
        LOG_WA("Could not find ccb %u ignoring abort",  ccbId);
        TRACE_LEAVE();
        return;
    }
    TRACE_5("ABORT CCB %u", ccbId);
    CcbInfo* ccb = (*i);
    
    switch(ccb->mState) {
        case IMM_CCB_EMPTY:
        case IMM_CCB_READY:
            break; //OK
        case IMM_CCB_CREATE_OP:
            LOG_WA("Aborting ccb %u while waiting for "
                "reply from implementer on CREATE-OP", ccbId);
            *client = ccb->mOriginatingConn;
            break;
            
        case IMM_CCB_MODIFY_OP:
            LOG_WA("Aborting ccb %u while waiting for "
                "reply from implementers on MODIFY-OP", ccbId);
            *client = ccb->mOriginatingConn;
            break;
            
        case IMM_CCB_DELETE_OP:
            LOG_WA("Aborting ccb %u while waiting for "
                "replies from implementers on DELETE-OP", ccbId);
            *client = ccb->mOriginatingConn;
            break;
            
        case IMM_CCB_PREPARE:
            LOG_WA("Aborting ccb %u while waiting for "
                "replies from implementers on COMPLETED", ccbId);
            *client = ccb->mOriginatingConn;
            break;
            
        case IMM_CCB_CRITICAL:
            LOG_WA("CCB %u is in critical state, can not abort", ccbId);
            TRACE_LEAVE();
            return;
            
        case IMM_CCB_PBE_ABORT:
            TRACE_5("Aborting ccb %u because PBE decided ABORT", ccbId);
            *client = ccb->mOriginatingConn;
            break;
            
        case IMM_CCB_COMMITTED:
            TRACE_5("CCB %u was already committed", ccbId);
            TRACE_LEAVE();
            return;
            
        case IMM_CCB_ABORTED:
            TRACE_5("CCB %u was already aborted", ccbId);
            TRACE_LEAVE();
            return;

        default:
            LOG_ER("Illegal state %u in ccb %u", ccb->mState, ccbId);
            assert(0);
    }
    
    
    TRACE_5("Ccb %u ABORTED", ccb->mId);
    ccb->mState = IMM_CCB_ABORTED;
    ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
    ccb->mWaitStartTime = 0;
    
    CcbImplementerMap::iterator isi;
    for(isi = ccb->mImplementers.begin();
        isi != ccb->mImplementers.end();
        ++isi) {
        ImplementerCcbAssociation* implAssoc = isi->second;
        ImplementerInfo* impInfo = implAssoc->mImplementer;
        assert(impInfo);
        SaUint32T implConn = impInfo->mConn;
        if(implConn) {
            if(impInfo->mDying) {
                LOG_WA("Lost connection with implementer %s, cannot send "
                    "abort upcall. Ccb %u is aborted anyway.",
                    impInfo->mImplementerName.c_str(), ccbId);
            } else {
                connVector.push_back(implConn);
                TRACE_5("Abort upcall for implementer %u/%s", 
                    impInfo->mId, impInfo->mImplementerName.c_str());
            }
        }
        delete isi->second; //Should not affect the iteration
    }
    
    ccb->mImplementers.clear();

    if(ccb->mMutations.empty() && pbeNodeIdPtr) {
        TRACE_5("Ccb %u being aborted is empty, avoid involving PBE", ccbId);
        pbeNodeIdPtr = NULL;
    }

    if(pbeNodeIdPtr && getPbeOi(&pbeConn, pbeNodeIdPtr)) {
        /* There is a PBE and it is registered at this node.
           Send abort also to pbe. */
        connVector.push_back(pbeConn);
        TRACE_5("Ccb abort upcall for ccb %u for local PBE ", ccbId);
    }
}

/** 
 * Deletes a CCB
 */
SaAisErrorT
ImmModel::ccbTerminate(SaUint32T ccbId) 
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    CcbVector::iterator i;
    ObjectMap::iterator oi;
    i = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if (i == sCcbVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: CCB %u does not exist", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
    } else {
        TRACE_5("terminate the CCB %u", (*i)->mId);
        
        //Delete ObjectMutations and afims.
        CcbInfo* ccb = *i;
        switch(ccb->mState) {
            case IMM_CCB_EMPTY:
            case IMM_CCB_READY:
            case IMM_CCB_COMMITTED:
            case IMM_CCB_ABORTED:
                break; //OK
            case IMM_CCB_CREATE_OP:
                LOG_WA("Will not terminate ccb %u while waiting for "
                    "reply from implementer on create-op", ccbId);
                TRACE_LEAVE();
                return SA_AIS_ERR_TRY_AGAIN;
                
            case IMM_CCB_MODIFY_OP:
                LOG_WA("Will not terminate ccb %u while waiting for "
                    "reply from implementers on modify-op", ccbId);
                TRACE_LEAVE();
                return SA_AIS_ERR_TRY_AGAIN;
                
            case IMM_CCB_DELETE_OP:
                LOG_WA("Will not terminate ccb %u while waiting for "
                    "replies from implementers on delete-op", ccbId);
                TRACE_LEAVE();
                return SA_AIS_ERR_TRY_AGAIN;
                
            case IMM_CCB_PREPARE:
            case IMM_CCB_PBE_ABORT:
                LOG_WA("Will not terminate ccb %u while waiting for "
                    "replies from implementers on completed ack", ccbId);
                TRACE_LEAVE();
                return SA_AIS_ERR_TRY_AGAIN;
                
            case IMM_CCB_CRITICAL:
                LOG_WA("Will not terminate ccb %u in critical state ",  ccbId);
                TRACE_LEAVE();
                return SA_AIS_ERR_TRY_AGAIN;
            default:
                LOG_ER("Illegal state %u in ccb %u", ccb->mState, ccbId);
                assert(0);
        }
        
        ObjectMutationMap::iterator omit;
        for(omit=ccb->mMutations.begin(); omit!=ccb->mMutations.end(); ++omit){
            ObjectMutation *omut = omit->second;
            ObjectInfo* afim = omut->mAfterImage;
            omut->mAfterImage = NULL;
            AdminOwnerVector::iterator i2;
            switch(omut->mOpType) {
                case IMM_DELETE:
                    TRACE_2("Aborting Delete of %s", omit->first.c_str());
                    oi = sObjectMap.find(omit->first);
                    assert(oi != sObjectMap.end());
                    oi->second->mObjFlags &= ~IMM_DELETE_LOCK;//Remove delete lock
                    TRACE_5("Flags after remove delete lock:%u", 
                        oi->second->mObjFlags);
                    break;
                    
                case IMM_CREATE: {
                    const std::string& dn = omit->first;
                    ObjectMap::iterator oi = sObjectMap.find(dn);
                    assert(oi != sObjectMap.end());
                    sObjectMap.erase(oi);
                    assert(afim);
                    ImmAttrValueMap::iterator oavi;
                    for(oavi = afim->mAttrValueMap.begin();
                        oavi != afim->mAttrValueMap.end(); ++oavi) {
                        delete oavi->second;
                    }
                    afim->mAttrValueMap.clear(); 
                    
                    assert(afim->mClassInfo->mRefCount > 0);
                    afim->mClassInfo->mRefCount--;
                    
                    //Aborting create => ensure no dangling references to
                    //object. Only possible dangling reference for a create is 
                    //from the admin owner of the ccb being terminated, in case
                    //this is an abort.
                    i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(),
                        IdIs(ccb->mAdminOwnerId));
                    if(i2 == sOwnerVector.end()) {
                        LOG_WA("Admin owner id %u does not exist", 
                            ccb->mAdminOwnerId);
                    } else {
                        (*i2)->mTouchedObjects.erase(afim);
                        //Note that on STL sets, the erase operation is
                        //polymorphic. Here we are erasing based on value, not
                        //iterator position. See commitDelete, which is similar
                        //in nature to abortCreate. One difference is that
                        //abortCreate can only have references in from the
                        //adminOwner of the cccb that creaed it. No other
                        //ccb/admin-owner can have seen the object. 
                    }
                    afim->mAdminOwnerAttrVal=0;
                    afim->mClassInfo=0;
                    afim->mImplementer=0;
                    afim->mObjFlags=0;
                    delete afim;
                }
                    break;
                    
                case IMM_MODIFY: {
                    assert(afim);
                    ImmAttrValueMap::iterator oavi;
                    for(oavi = afim->mAttrValueMap.begin();
                        oavi != afim->mAttrValueMap.end(); ++oavi) {
                        delete oavi->second;
                    }
                    //Empty the collection, probably not necessary (as the
                    //ObjectInfo record is deleted below), but does not hurt.
                    afim->mAttrValueMap.clear(); 
                    afim->mAdminOwnerAttrVal=0;
                    afim->mClassInfo=0;
                    afim->mImplementer=0;
                    delete afim;
                }
                    break;
                default: 
                    assert(0);
            }//switch
            delete omut;
        }//for each mutation
        ccb->mMutations.clear();
        if(ccb->mImplementers.size()) {
            LOG_WA("Ccb destroyed without notifying some implementers from IMMND.");
            CcbImplementerMap::iterator ix;
            for(ix=ccb->mImplementers.begin(); 
                ix != ccb->mImplementers.end(); ++ix) {
                struct ImplementerCcbAssociation* impla = ix->second;
                delete impla;
            }
            ccb->mImplementers.clear();
        }
        /*  ABT OCT 2009 Retain the ccb info to allow ccb result recovery. */

        if(ccb->mWaitStartTime == 0)  {
            ccb->mWaitStartTime = time(NULL); 
            TRACE_5("Ccb Wait-time for GC set. State: %u/%s", ccb->mState,
                (ccb->mState == IMM_CCB_COMMITTED)?"COMMITTED":
                ((ccb->mState == IMM_CCB_ABORTED)?"ABORTED":"OTHER"));
        }
        //TODO(?) Would be neat to store ccb outcomes in the OpenSafImm object.
    }
    
    TRACE_LEAVE();
    return err;
}


void
ImmModel::eduAtValToOs(immsv_octet_string* tmpos, 
    immsv_edu_attr_val* v,
    SaImmValueTypeT t)
{
    //This function simply transforms an IMM value type to a temporary
    //octet string, simplifying the storage model of the internal IMM repository.
    
    switch(t) {
        
        case SA_IMM_ATTR_SANAMET:
        case SA_IMM_ATTR_SASTRINGT:
        case SA_IMM_ATTR_SAANYT:
            tmpos->size = v->val.x.size;
            tmpos->buf = v->val.x.buf;   //Warning, only borowing the pointer.
            return;
            
        case SA_IMM_ATTR_SAINT32T:
            tmpos->size = 4;
            tmpos->buf = (char *) &(v->val.saint32);
            return;
            
        case SA_IMM_ATTR_SAUINT32T:
            tmpos->size = 4;
            tmpos->buf = (char *) &(v->val.sauint32);
            return;
            
        case SA_IMM_ATTR_SAINT64T:
            tmpos->size = 8;
            tmpos->buf = (char *) &(v->val.saint64);
            return;
            
        case SA_IMM_ATTR_SAUINT64T:
            tmpos->size = 8;
            tmpos->buf = (char *) &(v->val.sauint64);
            return;
            
        case SA_IMM_ATTR_SAFLOATT:
            tmpos->size = 4;
            tmpos->buf = (char *) &(v->val.safloat);
            return;
            
        case SA_IMM_ATTR_SATIMET:
            tmpos->size = 8;
            tmpos->buf = (char *) &(v->val.satime);
            return;
            
        case SA_IMM_ATTR_SADOUBLET:
            tmpos->size = 8;
            tmpos->buf = (char *) &(v->val.sadouble);
            return;
    }
}

/** 
 * Creates an object
 */
SaAisErrorT ImmModel::ccbObjectCreate(const ImmsvOmCcbObjectCreate* req,
    SaUint32T* implConn,
    unsigned int* implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER();
    SaAisErrorT err = SA_AIS_OK;
    //assert(!immNotWritable()); 
    //It should be safe to allow old ccbs to continue to mutate the IMM.
    //The sync does not realy start until all ccb's are completed.
    
    size_t sz = strnlen((char *) req->className.buf, 
        (size_t)req->className.size);
    std::string className((const char*)req->className.buf, sz);
    
    sz = strnlen((char *) req->parentName.buf, (size_t)req->parentName.size);
    std::string parentName((const char*)req->parentName.buf, sz);
    
    TRACE_2("parentName:%s\n", parentName.c_str());
    std::string objectName;
    SaUint32T ccbId = req->ccbId;  //Warning: SaImmOiCcbIdT is 64-bits
    SaUint32T adminOwnerId = req->adminOwnerId;
    
    CcbInfo* ccb = 0;
    AdminOwnerInfo* adminOwner = 0;
    ClassInfo* classInfo = 0;
    ObjectInfo* parent = 0;
    
    CcbVector::iterator i1;
    AdminOwnerVector::iterator i2;
    ClassMap::iterator i3 = sClassMap.find(className);
    AttrMap::iterator i4;
    ObjectMutation* oMut = 0;
    ObjectMap::iterator i5;
    ImmAttrValueMap::iterator i6;
    
    immsv_attr_values_list* attrValues = NULL;
    
    bool nameCorrected = false;

    //int isLoading = this->getLoader() > 0;
    int isLoading = (sImmNodeState == IMM_NODE_LOADING);
    
    if(!nameCheck(parentName)) {
        if(nameToInternal(parentName)) {
            nameCorrected = true;
        } else {
            LOG_NO("ERR_INVALID_PARAM: Not a proper parent name:%s size:%u", 
                parentName.c_str(), (unsigned int) parentName.size());
            err = SA_AIS_ERR_INVALID_PARAM;
            goto ccbObjectCreateExit;
        }
    }
    
    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name:%s", className.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
        goto ccbObjectCreateExit;
    }
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if (i1 == sCcbVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: ccb id %u does not exist", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectCreateExit;
    }
    ccb = *i1;
    
    if(!ccb->isOk()) {
        LOG_IN("ERR_FAILED_OPERATION: ccb %u is in an error state "
            "rejecting ccbObjectCreate operation ", ccbId);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectCreateExit;
    }
    
    if(ccb->mState > IMM_CCB_READY) {
        LOG_ER("ERR_FAILED_OPERATION: ccb %u is not in an expected state:%u "
            "rejecting ccbObjectCreate operation ", ccbId, ccb->mState);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectCreateExit;
    }
    
    i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
        IdIs(adminOwnerId));
    if (i2 == sOwnerVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: admin owner id %u does not exist", 
            adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectCreateExit;
    }
    adminOwner = *i2;
    
    assert(!adminOwner->mDying);
    
    if(adminOwner->mId !=  ccb->mAdminOwnerId) {
        LOG_WA("ERR_FAILED_OPERATION: Inconsistency between Ccb admoId:%u and AdminOwner-id:%u",
            adminOwner->mId, ccb->mAdminOwnerId);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectCreateExit;
    }
    
    if (i3 == sClassMap.end()) {
        TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", className.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto ccbObjectCreateExit;
    }
    
    classInfo = i3->second;
    
    if((classInfo->mCategory != SA_IMM_CLASS_CONFIG) && !isLoading) {
        //If class is not config and isloading==true, then a check that only 
        //accepts persistent RT attributes is enforced below. Se ***.
        LOG_NO("ERR_INVALID_PARAM: class '%s' is not a configuration class", 
            className.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
        goto ccbObjectCreateExit;
    }
    
    if(parentName.length() > 0) {
        i5 = sObjectMap.find(parentName);
        if (i5 == sObjectMap.end()) {
            if(isLoading) {
                TRACE_7("Parent object '%s' does not exist..yet", 
                    parentName.c_str());
                //Save parentName in sMissingParents. Remove when loaded.
                //Check at end of loading that sMissingParents is empty.
                sMissingParents.insert(parentName);
            } else {
                TRACE_7("ERR_NOT_EXIST: parent object '%s' does not exist", 
                    parentName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
                goto ccbObjectCreateExit;
            }
        } else {
            parent = i5->second;
            if((parent->mObjFlags & IMM_CREATE_LOCK) && 
                (parent->mCcbId != ccbId)) {
                TRACE_7("ERR_NOT_EXIST: Parent object '%s' is being created in a different "
                    "ccb %u from the ccb for this create %u", 
                    parentName.c_str(), parent->mCcbId, ccbId);
                err = SA_AIS_ERR_NOT_EXIST;
                goto ccbObjectCreateExit;
            }
            if(parent->mObjFlags & IMM_DELETE_LOCK) {
                TRACE_7("ERR_NOT_EXIST: Parent object '%s' is locked for delete in ccb: %u. "
                    "Will not allow create of subobject.",
                    parentName.c_str(), parent->mCcbId);
                err = SA_AIS_ERR_NOT_EXIST;
                goto ccbObjectCreateExit;
            }
            if(parent->mClassInfo->mCategory == SA_IMM_CLASS_RUNTIME) {
                if(classInfo->mCategory == SA_IMM_CLASS_CONFIG) {
                    LOG_IN("ERR_NOT_EXIST: Parent object '%s' is a runtime object."
                        "Will not allow create of config sub-object.",
                        parentName.c_str());
                    err = SA_AIS_ERR_NOT_EXIST;
                    goto ccbObjectCreateExit;
                } else {
                    assert(isLoading); 
                    /* See check above, only loader allowed to create 
                       (persistent) rtos this way. Should really check
                       that parent and child are persistent. Since 
                       we know we are loading, we assume we are 
                       loading a dump that only contains persistent
                       runtime objects.
                    */
                }
            }
        }
    }
    
    if (parent) {
        std::string parentAdminOwnerName;
        parent->getAdminOwnerName(&parentAdminOwnerName);
        if((parentAdminOwnerName != adminOwner->mAdminOwnerName) && 
            !isLoading) {
            LOG_IN("ERR_BAD_OPERATION: parent object not owned by '%s'", 
                adminOwner->mAdminOwnerName.c_str());
            err = SA_AIS_ERR_BAD_OPERATION;
            goto ccbObjectCreateExit;
        }
    }
    
    i4 = std::find_if(classInfo->mAttrMap.begin(), classInfo->mAttrMap.end(),
        AttrFlagIncludes(SA_IMM_ATTR_RDN));
    if (i4 == classInfo->mAttrMap.end()) {
        LOG_ER("ERR_FAILED_OPERATION: No RDN attribute found in class!");
        err = SA_AIS_ERR_FAILED_OPERATION;     //Should never happen!
        goto ccbObjectCreateExit;
    }
    
    attrValues = req->attrValues;
    
    while(attrValues) {
        sz = strnlen((char *) attrValues->n.attrName.buf, 
            (size_t)attrValues->n.attrName.size);
        std::string attrName((const char*)attrValues->n.attrName.buf, sz);
        
        if (attrName == i4->first) { //Match on name for RDN attribute
            if((attrValues->n.attrValueType != SA_IMM_ATTR_SANAMET) &&
                (attrValues->n.attrValueType != SA_IMM_ATTR_SASTRINGT)) {
                LOG_NO("ERR_INVALID_PARAM: Value type for RDN attribute is "
                    "neither SaNameT nor SaStringT");
                err = SA_AIS_ERR_INVALID_PARAM;
                goto ccbObjectCreateExit;
            }
            
            if(((size_t)attrValues->n.attrValue.val.x.size > 64) &&
                (i4->second->mValueType == SA_IMM_ATTR_SASTRINGT))
            {
                LOG_NO("ERR_INVALID_PARAM: RDN attribute is too large: %u. Max length is 64 "
                    "for SaStringT", attrValues->n.attrValue.val.x.size);
                err = SA_AIS_ERR_INVALID_PARAM;     
                goto ccbObjectCreateExit;
            }

            if(attrValues->n.attrValueType != (int) i4->second->mValueType) {
                if(isLoading) {
                    //Be lenient on the loader. It assumes RDN is always SaNameT.
                    TRACE_7("COMPONENT TEST*** IF I SEE THIS THEN DONT REMOVE THE CODE BRANCH");
                    //I dont think we ever get this case.
                    assert(i4->second->mValueType == SA_IMM_ATTR_SANAMET ||
                        i4->second->mValueType == SA_IMM_ATTR_SASTRINGT);
                    attrValues->n.attrValueType = i4->second->mValueType;
                }
            }
            
            
            objectName.append((const char*)attrValues->n.attrValue.val.x.buf, 
                strnlen((const char*)attrValues->n.attrValue.val.x.buf,
                    (size_t)attrValues->n.attrValue.val.x.size));
            break; //out of for-loop.
        }
        attrValues = attrValues->next;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: no RDN attribute found or value is empty");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto ccbObjectCreateExit;
    }
    
    if(!nameCheck(objectName)) {
        if(nameToInternal(objectName)) {
            nameCorrected = true;
        } else {
            LOG_NO("ERR_INVALID_PARAM: Not a proper RDN (%s, %u)", objectName.c_str(),
                (unsigned int) objectName.length());
            err = SA_AIS_ERR_INVALID_PARAM;
            goto ccbObjectCreateExit;
        }
    }
    
    if(objectName.find(',') != std::string::npos) {
        LOG_NO("ERR_INVALID_PARAM: Can not tolerate ',' in RDN");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto ccbObjectCreateExit;
    }
    
    if(parentName.size()) {
        objectName.append(",");
        objectName.append(parentName);
    }
    
    if (objectName.size() >= SA_MAX_NAME_LENGTH) {
        TRACE_7("ERR_NAME_TOO_LONG: DN is too long, size:%u, max size is:%u",
            (unsigned int) objectName.size(), SA_MAX_NAME_LENGTH);
        err = SA_AIS_ERR_NAME_TOO_LONG;
        goto ccbObjectCreateExit;
    }
    
    if ((i5 = sObjectMap.find(objectName)) != sObjectMap.end()) {
        if(i5->second->mObjFlags & IMM_CREATE_LOCK) {
            TRACE_7("ERR_EXIST: object '%s' is already registered "
                "for creation in a ccb, but not applied yet", 
                objectName.c_str());
        } else {
            TRACE_7("ERR_EXIST: object '%s' exists", objectName.c_str());
        }
        err = SA_AIS_ERR_EXIST;
        goto ccbObjectCreateExit;
    }
    
    if(err == SA_AIS_OK) {
        TRACE_5("create object '%s'", objectName.c_str());
        ObjectInfo* object = new ObjectInfo();
        object->mCcbId = req->ccbId;
        object->mClassInfo = classInfo;
        object->mImplementer = classInfo->mImplementer;
        //Note: mObjFlags is both initialized and assigned below
        if(nameCorrected) {
            object->mObjFlags = IMM_CREATE_LOCK | IMM_DN_INTERNAL_REP;
        } else {
            object->mObjFlags = IMM_CREATE_LOCK;
        }
        //TRACE_5("Flags after insert create lock:%u", object->mObjFlags);
        
        
        // Add attributes to object
        for (i4 = classInfo->mAttrMap.begin(); 
             i4 != classInfo->mAttrMap.end();
             i4++) {
            AttrInfo* attr = i4->second;
            
            ImmAttrValue* attrValue = NULL;
            if(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE) {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrMultiValue();
                } else {
                    attrValue = new 
                        ImmAttrMultiValue(attr->mDefaultValue);
                }
            } else {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrValue();
                } else {
                    attrValue = new 
                        ImmAttrValue(attr->mDefaultValue);
                }
            }
            
            //Set admin owner as a regular attribute and then 
            //also a pointer to the attrValue for efficient access.
            if(i4->first == 
                std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                attrValue->
                    setValueC_str(adminOwner->mAdminOwnerName.c_str());
                object->mAdminOwnerAttrVal = attrValue;
            }
            
            object->mAttrValueMap[i4->first] = attrValue;
        }
        
        // Set attribute values
        immsv_attr_values_list* p = req->attrValues;
        while(p) {
            sz = strnlen((char *) p->n.attrName.buf, 
                (size_t)p->n.attrName.size);
            std::string attrName((const char*)p->n.attrName.buf, sz);
            
            i6 = object->mAttrValueMap.find(attrName);
            
            if (i6 == object->mAttrValueMap.end()) {
                TRACE_7("ERR_NOT_EXIST: attr '%s' not defined", attrName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
                break; //out of for-loop
            }
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if(attr->mValueType != 
                (unsigned int) p->n.attrValueType) {

                LOG_NO("ERR_INVALID_PARAM: attr '%s' type missmatch", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            }
            
            if((attr->mFlags & SA_IMM_ATTR_RUNTIME)&& 
                !(attr->mFlags & SA_IMM_ATTR_PERSISTENT)) {
                // *** Reject non-persistent rt attributes from created over OM API.
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is a runtime attribute => "
                    "can not be assigned over OM-API.", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            }
            
            ImmAttrValue* attrValue = i6->second;
            IMMSV_OCTET_STRING tmpos; //temporary octet string
            eduAtValToOs(&tmpos, &(p->n.attrValue), 
                (SaImmValueTypeT) p->n.attrValueType);
            attrValue->setValue(tmpos);
            if(p->n.attrValuesNumber > 1) {
                /*
                  int extraValues = p->n.attrValuesNumber - 1;
                */
                if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                    LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued yet "
                        "multiple values provided in create call", 
                        attrName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    break; //out of for loop
                } 
                /*
                  else if(extraValues != 
                  p->attrMoreValues.list.count) {
                  LOG_ER("Multivalued attr '%s' should have "
                  "%u extra values but %u extra values tranmitted",
                  attrName.c_str(), extraValues, 
                  p->attrMoreValues.list.count);
                  //Should not happen, error in packing.
                  err = SA_AIS_ERR_LIBRARY;
                  break; //out of for-loop
                  } */
                else {
                    assert(attrValue->isMultiValued());
                    if(className != immClassName) { 
                        //Avoid restoring imm object
                        ImmAttrMultiValue* multiattr = 
                            (ImmAttrMultiValue *) attrValue;
                        
                        IMMSV_EDU_ATTR_VAL_LIST* al= 
                            p->n.attrMoreValues;
                        while(al) {
                            eduAtValToOs(&tmpos, &(al->n), 
                                (SaImmValueTypeT) p->n.attrValueType);
                            multiattr->setExtraValue(tmpos);
                            al = al->next;
                        }
                        //TODO: Verify that attrValuesNumber was correct.
                    }
                }
            } //if(p->n.attrValuesNumber > 1)
            //If reloading (cluster restart), revive implementers as needed.
            if(isLoading && i4->first == 
                std::string(SA_IMM_ATTR_IMPLEMENTER_NAME)) {
                if(!attrValue->empty()) {
                    std::string implName(attrValue->getValueC_str());
                    ImplementerInfo* impl = findImplementer(implName);
                    if(!impl) {
                        TRACE_5("Reviving implementor %s", implName.c_str());
                        //Create implementer
                        impl = new ImplementerInfo; 
                        impl->mImplementerName = implName;
                        sImplementerVector.push_back(impl);
                        impl->mId = 0;
                        impl->mConn = 0;
                        impl->mNodeId = 0;
                        impl->mMds_dest = 0LL;
                        impl->mAdminOpBusy = 0; 
                        impl->mDying = false;
                    }
                    //Reconenct implementer.
                    object->mImplementer = impl;
                }
            }
            
            p = p->next;
        } //while(p)
        
        //Check that all attributes with INITIALIZED flag have been set.
        
        for(i6=object->mAttrValueMap.begin(); 
            i6!=object->mAttrValueMap.end() && err==SA_AIS_OK;
            ++i6) {
            ImmAttrValue* attrValue = i6->second;
            std::string attrName(i6->first);
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if((attr->mFlags & SA_IMM_ATTR_INITIALIZED) && 
                attrValue->empty()) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' must be initialized "
                    "yet no value provided in the object create call", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
            }
        }

         // Prepare for call on PersistentBackEnd
        if((err == SA_AIS_OK) && pbeNodeIdPtr) {
            void* pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
            if(!pbe) {
                if(ccb->mMutations.size()) {
                    /* ongoing ccb interrupted by PBE down */
                    err = SA_AIS_ERR_FAILED_OPERATION;
                    ccb->mVeto = err;
                    LOG_WA("ERR_FAILED_OPERATION: Persistent back end is down "
                           "ccb %u is aborted", ccbId);
                } else {
                    /* Pristine ccb can not start because PBE down */
                    TRACE_5("ERR_TRY_AGAIN: Persistent back end is down");
                    err = SA_AIS_ERR_TRY_AGAIN;
                }
            }
        }

        if(err == SA_AIS_OK) {
            oMut = new ObjectMutation(IMM_CREATE);
            oMut->mAfterImage = object;
            
            // Prepare for call on object implementor 
            // and add implementer to ccb.
            if(object->mImplementer && 
                object->mImplementer->mNodeId) {
                ccb->mState = IMM_CCB_CREATE_OP;
                *implConn = object->mImplementer->mConn;
                *implNodeId = object->mImplementer->mNodeId;
                
                CcbImplementerMap::iterator ccbi = 
                    ccb->mImplementers.find(object->mImplementer->mId);
                if(ccbi == ccb->mImplementers.end()) {
                    ImplementerCcbAssociation* impla = new 
                        ImplementerCcbAssociation(object->mImplementer);
                    ccb->mImplementers[object->mImplementer->mId] = impla;
                }
                //Create an implementer continuation to ensure wait
                //can be aborted. 
                oMut->mWaitForImplAck = true;
                
                //Increment even if we dont invoke locally
                oMut->mContinuationId = (++sLastContinuationId);
                if(sLastContinuationId >= 0xfffffffe) 
                {sLastContinuationId = 1;}

                if(*implConn) {
                    if(object->mImplementer->mDying) {
                        LOG_WA("Lost connection with implementer %s in "
                            "CcbObjectCreate.", 
                            object->mImplementer->mImplementerName.c_str());
                        *continuationId = 0;
                        *implConn = 0;
                        //err = SA_AIS_ERR_FAILED_OPERATION;
                        //Let the timeout handling take care of it.
                        //This really needs to be tested! But how ?
                        
                    } else {
                        *continuationId = sLastContinuationId;
                    }
                }
                
                TRACE_5("THERE IS AN IMPLEMENTER %u conn:%u node:%x name:%s\n",
                    object->mImplementer->mId, *implConn, *implNodeId,
                    object->mImplementer->mImplementerName.c_str());
                ccb->mWaitStartTime = time(NULL);
            } else if(ccb->mCcbFlags & 
                SA_IMM_CCB_REGISTERED_OI) {
                TRACE_7("ERR_NOT_EXIST: object '%s' does not have an implementer yet "
                    "flag SA_IMM_CCB_REGISTERED_OI is set", 
                    objectName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
            }

        }
        
        if(err == SA_AIS_OK) {
            assert(oMut);
            //This is a create => No need to check if there already is a 
            //mutation on this object.
            ccb->mMutations[objectName] = oMut;
            ccb->mOpCount++;
            
            //Object placed in map before apply/commit as a place-holder.
            //The mCreateLock on the object should prevent premature 
            //read access.
            sObjectMap[objectName] = object; 
            classInfo->mRefCount++;
            
            unsigned int sze = sObjectMap.size();
            if(sze >= 5000) {
                if(sze%1000 == 0) {
                    LOG_WA("Number of objects in IMM is:%u", sze);
                }
            }
            
            if(adminOwner->mReleaseOnFinalize) {
                adminOwner->mTouchedObjects.insert(object);
            }
            
            //Move this to commitCreate ?
            if(className == immClassName) {
                if(objectName == immObjectDn) {
                    updateImmObject(immClassName);
                } else {
                    LOG_ER("IMM OBJECT DN:%s should be:%s", 
                        objectName.c_str(), immObjectDn.c_str());
                }
            }

            if(isLoading && sMissingParents.size() && 
                sMissingParents.erase(objectName)) {
                TRACE_5("Missing parent %s found", objectName.c_str());
            }
        } else {
            //err != SA_AIS_OK
            //Delete object and its attributes
            if(ccb->mState == IMM_CCB_CREATE_OP) {
                ccb->mState = IMM_CCB_READY;
            }
            ImmAttrValueMap::iterator oavi;
            for(oavi = object->mAttrValueMap.begin();
                oavi != object->mAttrValueMap.end(); ++oavi) {
                delete oavi->second;  
            }
            
            //Empty the collection, probably not necessary 
            //(as the ObjectInfo record is deleted below), 
            //but does not hurt.
            object->mAttrValueMap.clear(); 
            
            object->mImplementer=0;
            object->mAdminOwnerAttrVal=0;
            object->mClassInfo=0;
            delete object; 
            
            if(oMut) {
                oMut->mAfterImage = NULL;
                delete oMut;
            }
        }
    }
 ccbObjectCreateExit:
    TRACE_LEAVE(); 
    return err;
}

/** 
 * Modifies an object
 */
SaAisErrorT
ImmModel::ccbObjectModify(const ImmsvOmCcbObjectModify* req,
    SaUint32T* implConn,
    unsigned int* implNodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER();
    SaAisErrorT err = SA_AIS_OK;
    
    //assert(!immNotWritable());
    //It should be safe to allow old ccbs to continue to mutate the IMM.
    //The sync does not realy start until all ccb's are completed.
    
    size_t sz = strnlen((char *) req->objectName.buf, 
        (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    SaUint32T ccbId = req->ccbId;
    SaUint32T ccbIdOfObj = 0;
    SaUint32T adminOwnerId = req->adminOwnerId;

    TRACE_2("Modify objectName:%s ccbId:%u admoId:%u\n", 
        objectName.c_str(), ccbId, adminOwnerId);
    
    CcbInfo* ccb = 0;
    AdminOwnerInfo* adminOwner = 0;
    ClassInfo* classInfo = 0;
    ObjectInfo* object = 0;
    ObjectInfo* afim = 0;
    
    CcbVector::iterator i1;
    AdminOwnerVector::iterator i2;
    AttrMap::iterator i4;
    ObjectMap::iterator oi;
    ImmAttrValueMap::iterator oavi;
    std::string objAdminOwnerName;
    ObjectMutationMap::iterator omuti;
    ObjectMutation* oMut = 0;
    bool chainedOp = false;
    immsv_attr_mods_list* p = req->attrMods;
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto ccbObjectModifyExit;
    }
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if (i1 == sCcbVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: ccb id %u does not exist", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectModifyExit;
    }
    ccb = *i1;
    
    if(!ccb->isOk()) {
        LOG_IN("ERR_FAILED_OPERATION: ccb %u is in an error state "
            "rejecting ccbObjectModify operation ", ccbId);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectModifyExit;
    }
    
    if(ccb->mState > IMM_CCB_READY) {
        LOG_ER("ERR_FAILED_OPERATION: ccb %u is not in an expected state: %u "
            "rejecting ccbObjectModify operation ", ccbId, ccb->mState);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectModifyExit;
    }
    
    i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
        IdIs(adminOwnerId));
    if (i2 == sOwnerVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: admin owner id %u does not exist", adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectModifyExit;
    }
    adminOwner = *i2;
    
    assert(!adminOwner->mDying);
    
    if(adminOwner->mId !=  ccb->mAdminOwnerId) {
        LOG_WA("ERR_FAILED_OPERATION: Inconsistency between Ccb-admoId:%u and "
            "AdminOwnerId:%u", adminOwner->mId, ccb->mAdminOwnerId);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectModifyExit;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN attribute value");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto ccbObjectModifyExit;
    }
    
    oi = sObjectMap.find(objectName);
    if (oi == sObjectMap.end()) {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto ccbObjectModifyExit;
    }
    
    object = oi->second;
    
    object->getAdminOwnerName(&objAdminOwnerName);
    if(objAdminOwnerName != adminOwner->mAdminOwnerName)
    {
        LOG_IN("ERR_BAD_OPERATION: Mismatch on administrative owner %s != %s", 
            objAdminOwnerName.c_str(), adminOwner->mAdminOwnerName.c_str());
        err = SA_AIS_ERR_BAD_OPERATION;
        goto ccbObjectModifyExit;
    }
    
    ccbIdOfObj = object->mCcbId;
    if(ccbIdOfObj != ccbId) {
        i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), 
            CcbIdIs(ccbIdOfObj));
        if ((i1 != sCcbVector.end()) && ((*i1)->isActive())) {
            TRACE_7("ERR_BUSY: ccb id %u differs from active ccb id on object %u", 
                ccbId, ccbIdOfObj);
            err = SA_AIS_ERR_BUSY;
            goto ccbObjectModifyExit;
        }
    }

    if(object->mObjFlags & IMM_RT_UPDATE_LOCK) {
        /* Sorry but any ongoing PRTO update will interfere with additional
           ccb ops on the object because the PRTO update has to be reversible.
        */
        LOG_IN("ERR_TRY_AGAIN: Object '%s' already subject of a persistent runtime "
                 "attribute update", objectName.c_str());    
        err = SA_AIS_ERR_TRY_AGAIN;
        goto ccbObjectModifyExit;
    }

    classInfo = object->mClassInfo;
    assert(classInfo);
    
    if(classInfo->mCategory != SA_IMM_CLASS_CONFIG) {
        LOG_NO("ERR_INVALID_PARAM: object '%s' is not a configuration object", 
            objectName.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
        goto ccbObjectModifyExit;
    }
    
    omuti = ccb->mMutations.find(objectName);
    if(omuti != ccb->mMutations.end()) {
        oMut = omuti->second;
        if(oMut->mOpType == IMM_DELETE) {
            TRACE_7("ERR_NOT_EXIST: object '%s' is scheduled for delete", objectName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
            goto ccbObjectModifyExit;
        } else if(oMut->mOpType == IMM_CREATE) {
            chainedOp = true;
            afim = object;
        } else {
            assert(oMut->mOpType == IMM_MODIFY);
            chainedOp = true;
            afim = oMut->mAfterImage;
        }
    }

    //Find each attribute to be modified.
    //Check that attribute is a config attribute.
    //Check that attribute is writable.
    //If multivalue implied check that it is multivalued.
    //Assign the attribute as intended.
    
    TRACE_5("modify object '%s'", objectName.c_str());
    
    if(!chainedOp) {
        afim = new ObjectInfo();
        afim->mCcbId = ccbId;
        afim->mClassInfo = classInfo;
        afim->mImplementer = object->mImplementer;
        afim->mObjFlags = object->mObjFlags;
        
        
        // Copy attribute values from existing object version to afim
        for(oavi = object->mAttrValueMap.begin(); 
            oavi != object->mAttrValueMap.end();
            oavi++) {
            ImmAttrValue* oldValue = oavi->second;
            ImmAttrValue* newValue = NULL;
            
            if(oldValue->isMultiValued()) {
                newValue = new ImmAttrMultiValue(*((ImmAttrMultiValue *) 
                                                     oldValue));
            } else {
                newValue = new ImmAttrValue(*oldValue);
            }
            
            //Set admin owner as a regular attribute and then also a pointer
            //to the attrValue for efficient access.
            if(oavi->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                afim->mAdminOwnerAttrVal = newValue;
            }
            afim->mAttrValueMap[oavi->first] = newValue;
        }
    }
    
    for (p = req->attrMods; p; p=p->next) {
        sz = strnlen((char *) p->attrValue.attrName.buf,
            (size_t) p->attrValue.attrName.size);
        std::string attrName((const char *) p->attrValue.attrName.buf, sz);
        
        i4 = classInfo->mAttrMap.find(attrName);
        if(i4==classInfo->mAttrMap.end()) {
            TRACE_7("ERR_NOT_EXIST: attr '%s' does not exist in object %s",
                attrName.c_str(), objectName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
            break; //out of for-loop
        }
        AttrInfo* attr = i4->second;
        
        if(attr->mValueType != (unsigned int) p->attrValue.attrValueType) {
            LOG_NO("ERR_INVALID_PARAM: attr '%s' type missmatch", 
                attrName.c_str());
            err = SA_AIS_ERR_INVALID_PARAM;
            break; //out of for-loop
        }
        
        if(attr->mFlags & SA_IMM_ATTR_RUNTIME) {
            LOG_NO("ERR_INVALID_PARAM: attr '%s' is a runtime attribute => "
                "can not be modified over OM-API.", 
                attrName.c_str());
            err = SA_AIS_ERR_INVALID_PARAM;
            break; //out of for-loop
        }
        
        if(!(attr->mFlags & SA_IMM_ATTR_WRITABLE)) {
            LOG_NO("ERR_INVALID_PARAM: attr '%s' is not a writable attribute",
                attrName.c_str());
            err = SA_AIS_ERR_INVALID_PARAM;
            break; //out of for-loop
        }
        
        ImmAttrValueMap::iterator i5 = 
            afim->mAttrValueMap.find(attrName);
        
        if (i5 == afim->mAttrValueMap.end()) {
            LOG_ER("Attr '%s' defined in class yet missing from existing object %s", 
                attrName.c_str(), objectName.c_str());
            assert(0); 
        }
        
        ImmAttrValue* attrValue = i5->second;
        IMMSV_OCTET_STRING tmpos; //temporary octet string
        
        switch(p->attrModType) {
            case SA_IMM_ATTR_VALUES_REPLACE:
                //Remove existing values and then fall through to the add case.
                attrValue->discardValues();
                if(p->attrValue.attrValuesNumber == 0) {
                    if(attr->mFlags & SA_IMM_ATTR_INITIALIZED) {
                        LOG_NO("ERR_INVALID_PARAM: attr '%s' has flag SA_IMM_ATTR_INITIALIZED "
                            " cannot modify to zero values", attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break; //out of switch
                    }
                    continue; //Ok to replace with nothing.
                }
                //else intentional fall-through
                
            case SA_IMM_ATTR_VALUES_ADD:
                
                if(p->attrValue.attrValuesNumber == 0) {
                    LOG_NO("ERR_INVALID_PARAM: Empty value used for adding to attribute %s",
                        attrName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    break;
                }
                
                eduAtValToOs(&tmpos, &(p->attrValue.attrValue),
                    (SaImmValueTypeT) p->attrValue.attrValueType);
                
                if(attrValue->empty()) {
                    attrValue->setValue(tmpos);
                } else {
                    if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                        LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued, yet "
                            "multiple values attempted", attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break; //out of switch
                    }
                    assert(attrValue->isMultiValued());
                    ImmAttrMultiValue* multiattr = 
                        (ImmAttrMultiValue *) attrValue;
                    multiattr->setExtraValue(tmpos);
                }
                
                if(p->attrValue.attrValuesNumber > 1) {
                    if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                        LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued, yet "
                            "multiple values provided in modify call", 
                            attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break; //out of switch
                    } else {
                        assert(attrValue->isMultiValued());
                        ImmAttrMultiValue* multiattr = 
                            (ImmAttrMultiValue *) attrValue;
                        
                        IMMSV_EDU_ATTR_VAL_LIST* al = 
                            p->attrValue.attrMoreValues;
                        while(al) {
                            eduAtValToOs(&tmpos, &(al->n),
                                (SaImmValueTypeT) p->attrValue.attrValueType);
                            multiattr->setExtraValue(tmpos);
                            al = al->next;
                        }
                    }
                }
                break;
                
            case SA_IMM_ATTR_VALUES_DELETE:
                //Delete all values that match that provided by invoker.
                if(p->attrValue.attrValuesNumber == 0) {
                    LOG_NO("ERR_INVALID_PARAM: Empty value used for deleting from attribute %s",
                        attrName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    break;
                }
                
                if(!attrValue->empty()) {
                    eduAtValToOs(&tmpos, &(p->attrValue.attrValue),
                        (SaImmValueTypeT) p->attrValue.attrValueType);
                    
                    attrValue->removeValue(tmpos);
                    //Note: We allow the delete value to be multivalued even
                    //ifthe attribute is single valued. The semantics is that
                    //we delete ALL matchings of ANY delete value.
                    if(p->attrValue.attrValuesNumber > 1) {
                        IMMSV_EDU_ATTR_VAL_LIST* al = 
                            p->attrValue.attrMoreValues;
                        while(al) {
                            eduAtValToOs(&tmpos, &(al->n),
                                (SaImmValueTypeT) p->attrValue.attrValueType);
                            attrValue->removeValue(tmpos);
                            al = al->next;
                        }
                    }

                    if(attrValue->empty() && (attr->mFlags & SA_IMM_ATTR_INITIALIZED)) {
                        LOG_NO("ERR_INVALID_PARAM: attr '%s' has flag SA_IMM_ATTR_INITIALIZED "
                            " cannot modify to zero values", attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break; //out of switch
                    }
                }

                break; //out of switch
                
            default:
                err = SA_AIS_ERR_INVALID_PARAM;
                LOG_NO("ERR_INVALID_PARAM: Illegal value for SaImmAttrModificationTypeT: %u", 
                    p->attrModType);
                break;
        }
        
        if(err != SA_AIS_OK) {
            break; //out of for-loop
        }
    }//for (p = ....)
    
    // Prepare for call on PersistentBackEnd

    if((err == SA_AIS_OK) && pbeNodeIdPtr) {
        void* pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
        if(!pbe) {
            if(ccb->mMutations.size()) {
                /* ongoing ccb interrupted by PBE down */
                err = SA_AIS_ERR_FAILED_OPERATION;
                ccb->mVeto = err;
                LOG_WA("ERR_FAILED_OPERATION: Persistent back end is down "
                       "ccb %u is aborted", ccbId);
            } else {
                /* Pristine ccb can not start because PBE down */
                TRACE_5("ERR_TRY_AGAIN: Persistent back end is down");
                err = SA_AIS_ERR_TRY_AGAIN;
            }
        }
    }

    // Prepare for call on object implementer 
    // and add implementer to ccb.

    if(err == SA_AIS_OK) {
        if(chainedOp) {
            assert(oMut);
        } else {
            oMut = new ObjectMutation(IMM_MODIFY);
            oMut->mAfterImage = afim;
        }
        
        // Prepare for call on object implementor 
        // and add implementer to ccb.
        if(object->mImplementer && object->mImplementer->mNodeId) {
            ccb->mState = IMM_CCB_MODIFY_OP;
            *implConn = object->mImplementer->mConn;
            *implNodeId = object->mImplementer->mNodeId;
            
            CcbImplementerMap::iterator ccbi = 
                ccb->mImplementers.find(object->mImplementer->mId);
            if(ccbi == ccb->mImplementers.end()) {
                ImplementerCcbAssociation* impla = new 
                    ImplementerCcbAssociation(object->mImplementer);
                ccb->mImplementers[object->mImplementer->mId] = impla;
            }
            //Create an implementer continuation to ensure wait
            //can be aborted. 
            oMut->mWaitForImplAck = true;
            
            //Increment even if we dont invoke locally
            oMut->mContinuationId = (++sLastContinuationId);
            if(sLastContinuationId >= 0xfffffffe) {sLastContinuationId = 1;}

            if(*implConn) {
                if(object->mImplementer->mDying) {
                    LOG_WA("Lost connection with implementer %s in "
                        "CcbObjectModify.", 
                        object->mImplementer->mImplementerName.c_str());
                    *continuationId = 0;
                    *implConn = 0;
                    err = SA_AIS_ERR_FAILED_OPERATION;
                    //Let the timeout handling take care of it on other nodes.
                    //This really needs to be tested! But how ?
                } else {
                    *continuationId = sLastContinuationId;
                }
            }
            
            TRACE_5("THERE IS AN IMPLEMENTER %u conn:%u node:%x name:%s\n",
                object->mImplementer->mId, *implConn, *implNodeId,
                object->mImplementer->mImplementerName.c_str());
            
            ccb->mWaitStartTime = time(NULL);
        } else if(ccb->mCcbFlags & SA_IMM_CCB_REGISTERED_OI) {
            TRACE_7("ERR_NOT_EXIST: object '%s' does not have an implementer yet "
                "flag SA_IMM_CCB_REGISTERED_OI is set", 
                objectName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
        }
    }
    
    if(err == SA_AIS_OK) {
        object->mCcbId = ccbId; //Overwrite any old obsolete ccb id.
        assert(oMut);
        if(!chainedOp) {     
            ccb->mMutations[objectName] = oMut;
            if(adminOwner->mReleaseOnFinalize) {
                //NOTE: this may not be good enough. For a modify the
                //object should already be "owned" by the admin.
                adminOwner->mTouchedObjects.insert(object);
            }
        }
        ccb->mOpCount++;
    } else { 
        //err != SA_AIS_OK
        if(ccb->mState == IMM_CCB_MODIFY_OP) {ccb->mState = IMM_CCB_READY;}
        if(chainedOp) {
            err = SA_AIS_ERR_FAILED_OPERATION;
            ccb->mVeto = err; //Corrupted chain => corrupted ccb
        } else { //First op on this object => ccb can survive
            if(afim) {
                //Delete the cloned afim.
                for(oavi = afim->mAttrValueMap.begin(); 
                    oavi != afim->mAttrValueMap.end(); ++oavi) {
                    delete oavi->second;
                }
                afim->mAttrValueMap.clear();
                
                afim->mImplementer = 0;
                afim->mAdminOwnerAttrVal=0;
                afim->mClassInfo=0;
                delete afim;
            }
            if(oMut) {
                oMut->mAfterImage = NULL;
                delete oMut;
            }
        }
    }

 ccbObjectModifyExit:
    TRACE_LEAVE(); 
    return err;
}

/** 
 * Deletes an object
 */
SaAisErrorT
ImmModel::ccbObjectDelete(const ImmsvOmCcbObjectDelete* req,
    SaUint32T reqConn,
    ObjectNameVector& objNameVector,
    ConnVector& connVector, 
    IdVector& continuations,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER();
    SaAisErrorT err = SA_AIS_OK;
    
    //assert(!immNotWritable()); 
    //It should be safe to allow old ccbs to continue to mutate the IMM.
    //The sync does not realy start until all ccb's are completed.
    
    size_t sz = strnlen((char *) req->objectName.buf, 
        (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    SaUint32T ccbId = req->ccbId;
    SaUint32T adminOwnerId = req->adminOwnerId;
    
    CcbInfo* ccb = 0;
    AdminOwnerInfo* adminOwner = 0;
    CcbVector::iterator i1;
    AdminOwnerVector::iterator i2;
    ObjectMap::iterator oi, oi2;
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto ccbObjectDeleteExit;
    } 
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if (i1 == sCcbVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: ccb id %u does not exist", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectDeleteExit;
    }
    ccb = *i1;
    
    if(!ccb->isOk()) {
        LOG_IN("ERR_FAILED_OPERATION: ccb %u is in an error state "
            "rejecting ccbObjectDelete operation ", ccbId);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectDeleteExit;
    }
    
    if(ccb->mState > IMM_CCB_READY) {
        LOG_ER("ERR_FAILED_OPERATION: ccb %u is not in an expected state:%u "
            "rejecting ccbObjectDelete operation ", ccbId, ccb->mState);
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectDeleteExit;
    }
    
    if(reqConn && (ccb->mOriginatingConn != reqConn)) {
        LOG_IN("ERR_BAD_HANDLE: Missmatch on connection for ccb id %u", ccbId);
        err = SA_AIS_ERR_BAD_HANDLE;
        //Abort the CCB ??
        goto ccbObjectDeleteExit;
    }
    
    i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
        IdIs(adminOwnerId));
    if (i2 == sOwnerVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: admin owner id %u does not exist", adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto ccbObjectDeleteExit;
    }
    adminOwner = *i2;
    assert(!adminOwner->mDying);
    
    if(adminOwner->mId !=  ccb->mAdminOwnerId) {
        LOG_WA("ERR_FAILED_OPERATION: Inconsistency between Ccb and AdminOwner");
        err = SA_AIS_ERR_FAILED_OPERATION;
        goto ccbObjectDeleteExit;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN attribute value");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto ccbObjectDeleteExit;
    }
    
    oi = sObjectMap.find(objectName);
    if (oi == sObjectMap.end()) {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto ccbObjectDeleteExit;
    }

    // Prepare for call on PersistentBackEnd

    if((err == SA_AIS_OK) && pbeNodeIdPtr) {
        void* pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
        if(!pbe) {
            if(ccb->mMutations.size()) {
                /* ongoing ccb interrupted by PBE down */
                err = SA_AIS_ERR_FAILED_OPERATION;
                ccb->mVeto = err;
                LOG_WA("ERR_FAILED_OPERATION: Persistent back end is down "
                       "ccb %u is aborted", ccbId);
            } else {
                /* Pristine ccb can not start because PBE down */
                TRACE_5("ERR_TRY_AGAIN: Persistent back end is down");
                err = SA_AIS_ERR_TRY_AGAIN;
            }
        } else {
                TRACE_7("PbeConn: %u PbeNode:%u assigned", *pbeConnPtr, *pbeNodeIdPtr);
        }
    }
    
    for(int doIt=0; (doIt < 2) && (err == SA_AIS_OK); ++doIt) {
        
        err = deleteObject(oi, reqConn, adminOwner, ccb, doIt, objNameVector,
            connVector, continuations, pbeConnPtr?(*pbeConnPtr):0);
        
        // Find all sub objects to the deleted object and delete them
        for (oi2 = sObjectMap.begin(); 
             oi2 != sObjectMap.end() && err == SA_AIS_OK; oi2++) {
            std::string subObjName = oi2->first;
            if (subObjName.length() > objectName.length()) {
                size_t pos = subObjName.length() - objectName.length();
                if ((subObjName.rfind(objectName, pos) == pos) &&
                    (subObjName[pos-1] == ',')){
                    err = deleteObject(oi2, reqConn, adminOwner, ccb, doIt, 
                        objNameVector, connVector, continuations, 
                        pbeConnPtr?(*pbeConnPtr):0);
                }
            }
        }
        
        //B) AdminOwner linkage ? Not relevant if delete succeeded
        //   But could be relevant if Process or Ccb terminated.
        //   Depends on implementation. is new admin owner stored in 
        //   after image or in object header ?
        //   if( err == SA_AIS_OK && adminOwner->mReleaseOnFinalize) {
        //      adminOwner->mTouchedObjects.insert(object);
        //   }
        //  NOTE: The deleted object should already be owned.
    }
 ccbObjectDeleteExit:
    TRACE_LEAVE(); 
    return err;
}

SaAisErrorT
ImmModel::deleteObject(ObjectMap::iterator& oi, 
    SaUint32T reqConn,
    AdminOwnerInfo* adminOwner, 
    CcbInfo* ccb, 
    bool doIt,
    ObjectNameVector& objNameVector,
    ConnVector& connVector, 
    IdVector& continuations,
    unsigned int pbeIsLocal)
{
    /*TRACE_ENTER();*/
    bool configObj = true;    
    std::string objAdminOwnerName;
    SaUint32T ccbIdOfObj = 0;
    CcbVector::iterator i1;
    
    oi->second->getAdminOwnerName(&objAdminOwnerName);
    if(objAdminOwnerName != adminOwner->mAdminOwnerName) {
        LOG_IN("ERR_BAD_OPERATION: Mismatch on administrative owner %s != %s", 
            objAdminOwnerName.c_str(),
            adminOwner->mAdminOwnerName.c_str());
        return SA_AIS_ERR_BAD_OPERATION;
    }
    
    ccbIdOfObj = oi->second->mCcbId;
    if(ccbIdOfObj != ccb->mId) {
        i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), 
            CcbIdIs(ccbIdOfObj));
        if ((i1 != sCcbVector.end()) && ((*i1)->isActive())) {
            LOG_IN("ERR_BUSY: ccb id %u diff from active ccb id on object %u",
                ccb->mId, ccbIdOfObj);
            return SA_AIS_ERR_BUSY;
        }
    }
    
    if(oi->second->mClassInfo->mCategory != SA_IMM_CLASS_CONFIG) {
        if(doIt) {
            LOG_IN("Runtime object '%s' will be deleted as a side effect, "
                "if ccb %u is applied", oi->first.c_str(), ccb->mId);

            if(!(oi->second->mObjFlags & IMM_PRTO_FLAG)) {
                AttrMap::iterator i4 =
                    std::find_if(oi->second->mClassInfo->mAttrMap.begin(),
                            oi->second->mClassInfo->mAttrMap.end(),
                            AttrFlagIncludes(SA_IMM_ATTR_PERSISTENT));
                if(i4 != oi->second->mClassInfo->mAttrMap.end()) {
                    oi->second->mObjFlags |= IMM_PRTO_FLAG;
                }
            }
        }
        configObj = false;


        /* Check for possible interference with RTO create or delete */
        if(!doIt && !ccbIdOfObj) {
            if(oi->second->mObjFlags & IMM_CREATE_LOCK) {
                TRACE_7("ERR_TRY_AGAIN: sub-object '%s' registered for creation "
                    "but not yet applied by PRTO PBE ?", oi->first.c_str());
                return SA_AIS_ERR_TRY_AGAIN;
            } else if(oi->second->mObjFlags & IMM_DELETE_LOCK) {
                TRACE_7("ERR_TRY_AGAIN: sub-object '%s' already registered for delete "
                        "but not yet applied by PRTO PBE ?", oi->first.c_str());
                return SA_AIS_ERR_TRY_AGAIN;
            }
        }
    }
    
    if(!(oi->second->mImplementer && oi->second->mImplementer->mNodeId) &&
        (ccb->mCcbFlags & SA_IMM_CCB_REGISTERED_OI) && configObj) {

        TRACE_7("ERR_NOT_EXIST: object '%s' has no implementer yet flag "
            "SA_IMM_CCB_REGISTERED_OI is set in ccb", 
            oi->first.c_str());
        return SA_AIS_ERR_NOT_EXIST;
    }
    
    ObjectMutationMap::iterator omuti =
        ccb->mMutations.find(oi->first);
    if(omuti != ccb->mMutations.end()) {
        /*
          TODO: fix to allow delete on top of modify. Possibly also delete on 
          create. Delete on create is dangerous, must check if there are
          children creates.
        */
        LOG_IN("ERR_BAD_OPERATION: Object '%s' already subject of another "
            "operation in same ccb. can not handle chained delete in same ccb "
            "currently", oi->first.c_str());
        return SA_AIS_ERR_BAD_OPERATION;
    }

    if(oi->second->mObjFlags & IMM_RT_UPDATE_LOCK) {
        LOG_IN("ERR_TRY_AGAIN: Object '%s' already subject of a persistent runtime "
            "attribute update", oi->first.c_str());
        return SA_AIS_ERR_TRY_AGAIN;
    }

    if(doIt) {
        bool localImpl = false;
        oi->second->mCcbId = ccb->mId; //Overwrite any old obsolete ccb id.
        oi->second->mObjFlags |= IMM_DELETE_LOCK;//Prevents creates of subobjects.
        TRACE_5("Flags after insert delete lock:%u", 
            oi->second->mObjFlags);
        
        ObjectMutation* oMut = 
            new ObjectMutation(IMM_DELETE);
        ccb->mMutations[oi->first] = oMut;
        //oMut->mBeforeImage = oi->second;
        ccb->mOpCount++;

        if(oi->second->mImplementer && oi->second->mImplementer->mNodeId && configObj) {
            /* Not sending implementer ccb-delete upcalls for RTOs persistent or not. */
            ccb->mState = IMM_CCB_DELETE_OP;
            
            CcbImplementerMap::iterator ccbi = 
                ccb->mImplementers.find(oi->second->mImplementer->mId);
            if(ccbi == ccb->mImplementers.end()) {
                ImplementerCcbAssociation* impla = new 
                    ImplementerCcbAssociation(oi->second->mImplementer);
                ccb->mImplementers[oi->second->mImplementer->mId] = impla;
            }
            
            oMut->mWaitForImplAck = true; //Wait for an ack from implementer
            //Increment even if we dont invoke locally
            oMut->mContinuationId = (++sLastContinuationId);
            if(sLastContinuationId >= 0xfffffffe) {sLastContinuationId = 1;}

            SaUint32T implConn = oi->second->mImplementer->mConn;
            
            ccb->mWaitStartTime = time(NULL);
            /* TODO: Resetting the ccb timer for each deleted object here. 
               Not so efficient. Should set it only when all objects
               are processed.
            */
            
            if(implConn) { //implementer is on THIS node.
                if(oi->second->mImplementer->mDying) {
                    LOG_WA("Lost connection with implementer %s in "
                        "CcbObjectDelete.", 
                        oi->second->mImplementer->mImplementerName.c_str());
                    //Let the timeout take care of it.
                } else {
                    if(oi->second->mObjFlags & IMM_DN_INTERNAL_REP) {
                        std::string objectName(oi->first);
                        nameToExternal(objectName);
                        objNameVector.push_back(objectName);
                    } else {
                        objNameVector.push_back(oi->first);
                    }
                    connVector.push_back(implConn);
                    continuations.push_back(sLastContinuationId);
                    localImpl = true;
                }
            } 
        }

        if(pbeIsLocal && !localImpl && 
          (configObj || (oi->second->mObjFlags |= IMM_PRTO_FLAG))) {
            /* No regular and local implementer, but we have a PBE. 
               The object is a config object or a persistent RT object.
            */
            if(oi->second->mObjFlags & IMM_DN_INTERNAL_REP) {
                std::string objectName(oi->first);
                nameToExternal(objectName);
                objNameVector.push_back(objectName);
            } else {
                objNameVector.push_back(oi->first);
            }
            connVector.push_back(0);
            continuations.push_back(0);
        }
    }
    /*TRACE_LEAVE();*/
    return SA_AIS_OK;
}

bool 
ImmModel::ccbWaitForDeleteImplAck(SaUint32T ccbId, SaAisErrorT* err) 
{
    TRACE_ENTER();
    CcbVector::iterator i1;
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("CCb %u terminated during ccbCompleted processing, "
            "ccb must be aborted", ccbId);
        return false;
    }
    CcbInfo* ccb = *i1;
    
    if(ccb->mVeto != SA_AIS_OK) {
        *err = ccb->mVeto;
    } else {
        *err = SA_AIS_OK;
        ObjectMutationMap::iterator i2;
        for(i2=ccb->mMutations.begin(); i2!=ccb->mMutations.end(); ++i2) {
            if(i2->second->mWaitForImplAck) {
                assert(ccb->mState == IMM_CCB_DELETE_OP);
                TRACE_LEAVE();
                return true;
            }
        }
    }
    
    ccb->mWaitStartTime = 0;
    ccb->mState = IMM_CCB_READY;
    TRACE_LEAVE();
    return false;
}

bool
ImmModel::ccbWaitForCompletedAck(SaUint32T ccbId, SaAisErrorT* err,
                                 SaUint32T* pbeConnPtr, unsigned int* pbeNodeIdPtr,
                                 SaUint32T* pbeIdPtr, SaUint32T* pbeCtnPtr)
{
    TRACE_ENTER();
    if(pbeNodeIdPtr) {
        TRACE("We expect there to be a Pbe");
        assert(pbeConnPtr);
        assert(pbeIdPtr);
        assert(pbeCtnPtr);
        *pbeNodeIdPtr = 0;
        *pbeConnPtr = 0;
        *pbeIdPtr = 0;
        *pbeCtnPtr = 0;
    }

    CcbVector::iterator i1;
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("CCb %u terminated during ccbCompleted processing, "
            "ccb must be aborted", ccbId);
        *err = SA_AIS_ERR_FAILED_OPERATION;
        return false;
    }
    CcbInfo* ccb = *i1;
    
    if(ccb->mVeto != SA_AIS_OK) {
        *err = ccb->mVeto;
    } else {
        *err = SA_AIS_OK;
        CcbImplementerMap::iterator i2;
        for(i2=ccb->mImplementers.begin(); i2!=ccb->mImplementers.end(); ++i2){
            if(i2->second->mWaitForImplAck) {
                TRACE_LEAVE();
                return true; /* Continue waiting for some regular implementers */
            }
        }
    }

    /* No more implementers to wait for. 
       If PBE exists then we need to wait for pbe commit decission,
       but there can be no ccb timeout on that wait because we must
       obtain the decision from the PBE. 
       If the PBE crashes we could possibly force it to presumed abort.
       But that requires the current repository on disk to be scrapped,
       followed by a fresh PBE dump.
       Instead we reset the timer so that a timeout on a ccb in critical
       will force attempts to recover ccb-outcome from the current PBE.
    */

    if(ccb->mState == IMM_CCB_CRITICAL) {
        /* This must be the PBE reply. Stop waiting. */
        TRACE_5("PBE replied with rc:%u for ccb:%u", *err, ccbId);
        if((*err) != SA_AIS_OK) {
            ccb->mState = IMM_CCB_PBE_ABORT;
        }
        ccb->mWaitStartTime = 0;
        return false;
    }

    if(ccb->mMutations.empty() && pbeNodeIdPtr) {
        TRACE_5("Ccb %u being applied is empty, avoid involving PBE", ccbId);
        pbeNodeIdPtr = NULL;
    }

    if(((*err) == SA_AIS_OK) && pbeNodeIdPtr) {
        /* There should be a PBE */
        ImplementerInfo* pbeImpl = (ImplementerInfo *) getPbeOi(pbeConnPtr, pbeNodeIdPtr);
        if(pbeImpl) {
            /* There is in fact a PBE (up) */
            assert(ccb->mState == IMM_CCB_PREPARE);
            ccb->mState = IMM_CCB_CRITICAL;
            *pbeIdPtr = pbeImpl->mId;
            /* Add pbe implementer to the ccb implementer collection. 
               Note that this is done here AFTER all normal implementers
               have replied on the ccb-completed upcall.
             */
            ImplementerCcbAssociation* impla = new 
                        ImplementerCcbAssociation(pbeImpl);
            ccb->mImplementers[pbeImpl->mId] = impla;
            impla->mWaitForImplAck = true;
            impla->mContinuationId = ccb->mOpCount;

            *pbeCtnPtr = impla->mContinuationId;

            /* With PBE enabled, the critical phase is non-trivial. 
               The commit/abort decision is now delegated to the PBE.
               The CCB is in critical phase until we know the outcome.
               This means we can not terminate the ccb (release the
               objects write locked by the ccb) until we know the outcome.
               Restart the timer to catch ccbs hung waiting on PBE.
            */
             ccb->mWaitStartTime = time(NULL);
            return true; /* Wait for PBE commit*/
        } else {
            /* But there is not any PBE up currently => abort.  */
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
            *err = ccb->mVeto;
            LOG_WA("ERR_FAILED_OPERATION: Persistent back end is down "
                           "ccb %u is aborted", ccbId);
        }
    }

    TRACE_LEAVE();
    return false; /* nobody to wait for */
}

void
ImmModel::ccbObjDelContinuation(const immsv_oi_ccb_upcall_rsp* rsp,
    SaUint32T* reqConn)
{
    TRACE_ENTER();
    size_t sz = strnlen((char *) rsp->name.value, 
        (size_t)rsp->name.length);
    std::string objectName((const char*)rsp->name.value, sz);

    SaUint32T ccbId = rsp->ccbId;
    CcbInfo* ccb = 0;
    CcbVector::iterator i1;

    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("ccb id %u missing or terminated", ccbId);
        TRACE_LEAVE();
        return;
    }
    ccb = *i1;

    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_ER("Not a proper object name: %s", objectName.c_str());
        if(ccb->mVeto == SA_AIS_OK) {
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        }
        return;
    }

    ObjectMutationMap::iterator omuti =
        ccb->mMutations.find(objectName);
    if(omuti == ccb->mMutations.end()) {
        LOG_WA("object '%s' Not found in ccb - aborting ccb", 
            objectName.c_str());
        if(ccb->mVeto == SA_AIS_OK) {
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        }
    } else {
        assert(omuti->second->mWaitForImplAck);
        omuti->second->mWaitForImplAck = false;
        assert(/*(omuti->second->mContinuationId == 0) ||*/
               (omuti->second->mContinuationId == (SaUint32T) rsp->inv));
        *reqConn = ccb->mOriginatingConn;
        if((ccb->mVeto == SA_AIS_OK) && (rsp->result != SA_AIS_OK)) {
            LOG_IN("ImmModel::ccbObjDelContinuation: "
                "implementer returned error, Ccb aborted with error: %u",
                rsp->result);
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
            //TODO: This is perhaps more drastic than the specification
            //demands. We are here aborting the entire Ccb, whereas the spec
            //seems to allow for a non-ok returnvalue from implementer 
            //(in this callback) to simply reject the delete request, 
            //leaving the ccb otherwise intact.
            //On the other hand it is not *against* the specification as the
            //imm implementation has the right to veto any ccb at any time.
        }
    }
    TRACE_LEAVE();  
}

void
ImmModel::ccbCompletedContinuation(const immsv_oi_ccb_upcall_rsp* rsp,
    SaUint32T* reqConn)
{
    TRACE_ENTER();
    SaUint32T ccbId = rsp->ccbId;
    CcbInfo* ccb = 0;
    CcbVector::iterator i1;
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("ccb id %u missing or terminated", ccbId);
        TRACE_LEAVE();
        return;
    }
    ccb = *i1;
    
    CcbImplementerMap::iterator ix =
        ccb->mImplementers.find(rsp->implId);
    if(ix == ccb->mImplementers.end()) {
        if((ccb->mVeto == SA_AIS_OK) && (ccb->mState < IMM_CCB_CRITICAL)) {
            LOG_WA("Completed continuation: implementer '%u' Not found "
                "in ccb %u aborting ccb", rsp->implId, ccbId);
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        } else {
            LOG_WA("Completed continuation: implementer '%u' Not found "
                "in ccb %u in state(%u), can not abort", rsp->implId, 
                    ccbId, ccb->mState);
        }
    } else {
        if(ccb->mState == IMM_CCB_CRITICAL) {
            /* This is the commit/abort decision from the PBE.*/
            /* Verify that it is the PBE that is replying. */
            SaUint32T dummyConn;
            unsigned int dummyNodeId;
            ImplementerInfo* pbeImpl = (ImplementerInfo *) getPbeOi(&dummyConn, &dummyNodeId);
            if(!pbeImpl || (pbeImpl->mId != rsp->implId)) {
                LOG_WA("Received commit/abort decision on ccb %u from terminated PBE", ccbId);
                TRACE_LEAVE();
                return; /* Intentionally drop the missmatched response.*/
                /* If the PBE responds corrrectly, but crashes and gets deregistered
                   as implementer before this reply reaches us, the reply should be
                   seen as valid, but gets discard here. This could be fixed by
                   replacing the getPbeOi call with code that checks against the 
                   implementer info even when the implementer is marked as dead.
                   getPbeOi returns NULL for the valid but dead case.
                */
            }
            if(rsp->result == SA_AIS_OK) {
                TRACE("COMMIT decision on ccb %u received from PBE", ccbId);
            } else {
                TRACE("ABORT decision on ccb %u received from PBE", ccbId);
            }
        }

        ix->second->mWaitForImplAck = false;
        if(/*(ix->second->mContinuationId == 0) ||*/
               (ix->second->mContinuationId != rsp->inv)) {
            TRACE("ix->second->mContinuationId(%u) != rsp->inv(%u)",
                ix->second->mContinuationId, rsp->inv);
        }

        *reqConn = ccb->mOriginatingConn;

        if((ccb->mVeto == SA_AIS_OK) && (rsp->result != SA_AIS_OK)) {
            LOG_IN("implementer returned error, Ccb aborted with error: %u",
                rsp->result);
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        }
    }
    TRACE_LEAVE();  
}

void
ImmModel::ccbObjCreateContinuation(SaUint32T ccbId, SaUint32T invocation,
    SaAisErrorT error,
    SaUint32T* reqConn)
{
    TRACE_ENTER();
    CcbInfo* ccb = 0;
    CcbVector::iterator i1;
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("ccb id %u missing or terminated", ccbId);
        TRACE_LEAVE();
        return;
    }
    ccb = *i1;
    
    ObjectMutationMap::iterator omuti;
    for(omuti=ccb->mMutations.begin();omuti != ccb->mMutations.end();++omuti) {
        if(omuti->second->mContinuationId == invocation) {break;}
    }
    
    if(omuti == ccb->mMutations.end()) {
        LOG_WA("create invocation '%u' Not found in ccb - aborting ccb %u", 
            invocation, ccbId);
        if(ccb->mVeto == SA_AIS_OK) {
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        }
    } else {
        assert(omuti->second->mWaitForImplAck);
        omuti->second->mWaitForImplAck = false;
        ccb->mWaitStartTime = 0;
    }
    
    *reqConn = ccb->mOriginatingConn;
    if((ccb->mVeto == SA_AIS_OK) && (error != SA_AIS_OK)) {
        LOG_IN("ImmModel::ccbObjCreateContinuation: "
            "implementer returned error, Ccb aborted with error: %u",
            error);
        ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        //TODO: This is perhaps more drastic than the specification demands.
        //We are here aborting the entire Ccb, whereas the spec seems to allow
        //for a non-ok returnvalue from implementer (in this callback) to
        //simply reject the create request, leaving the ccb otherwise intact.
        //On the other hand it is not *against* the specification as the
        //imm implementation has the right to veto any ccb at any time.
    }
    if(ccb->mState != IMM_CCB_CREATE_OP) {
        LOG_ER("Unexpected state(%u) for ccb(%u) found in "
            "ccbObjectCreateContinuation ", ccb->mState, ccbId);
        assert(ccb->mState == IMM_CCB_CREATE_OP);
    }
    
    ccb->mState = IMM_CCB_READY;
    TRACE_LEAVE();  
}

void
ImmModel::ccbObjModifyContinuation(SaUint32T ccbId, SaUint32T invocation,
    SaAisErrorT error,
    SaUint32T* reqConn)
{
    TRACE_ENTER();
    CcbInfo* ccb = 0;
    CcbVector::iterator i1;
    
    i1 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbId));
    if(i1 == sCcbVector.end() || (!(*i1)->isActive()) ) {
        LOG_WA("ccb id %u missing or terminated", ccbId);
        TRACE_LEAVE();
        return;
    }
    ccb = *i1;
    
    ObjectMutationMap::iterator omuti;
    for(omuti=ccb->mMutations.begin();omuti != ccb->mMutations.end();++omuti) {
        if(omuti->second->mContinuationId == invocation) {break;}
    }
    
    if(omuti == ccb->mMutations.end()) {
        LOG_WA("modify invocation '%u' Not found in ccb - aborting ccb %u", 
            invocation, ccbId);
        if(ccb->mVeto == SA_AIS_OK) {
            ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        }
    } else {
        assert(omuti->second->mWaitForImplAck);
        omuti->second->mWaitForImplAck = false;
        ccb->mWaitStartTime = 0;
    }
    
    *reqConn = ccb->mOriginatingConn;
    if((ccb->mVeto == SA_AIS_OK) && (error != SA_AIS_OK)) {
        LOG_IN("ImmModel::ccbObjModifyContinuation: "
            "implementer returned error, Ccb aborted with error: %u", error);
        ccb->mVeto = SA_AIS_ERR_FAILED_OPERATION;
        //TODO: This is perhaps more drastic than the specification demands.
        //We are here aborting the entire Ccb, whereas the spec seems to allow
        //for a non-ok returnvalue from implementer (in this callback) to
        //simply reject the delete request, leaving the ccb otherwise intact.
        //On the other hand it is not *against* the specification as the
        //imm implementation has the right to veto any ccb at any time.
    }
    if(ccb->mState != IMM_CCB_MODIFY_OP) {
        LOG_ER("Unexpected state(%u) for ccb(%u) found in "
            "ccbObjectModifyContinuation ", ccb->mState, ccbId);
        assert(ccb->mState == IMM_CCB_MODIFY_OP);
    }
    ccb->mState = IMM_CCB_READY;
    TRACE_LEAVE();  
}


SaAisErrorT
ImmModel::accessorGet(const ImmsvOmSearchInit* req, ImmSearchOp& op)
{
    TRACE_ENTER();
    SaAisErrorT err = SA_AIS_OK;
    
    size_t sz = strnlen((char *) req->rootName.buf,
        (size_t)req->rootName.size);
    std::string objectName((const char*)req->rootName.buf, sz);
    
    SaImmScopeT scope = (SaImmScopeT)req->scope;
    SaImmSearchOptionsT searchOptions = 
        (SaImmSearchOptionsT)req->searchOptions;
    ObjectMap::iterator i;
    ObjectInfo* obj = NULL;
    bool implNotSet = true;
    ImmAttrValueMap::iterator j;
    int matchedAttributes=0;
    int soughtAttributes=0;
    
    if(immNotWritable()) {
        op.setImmNotWritable();
    }
    
    // Validate object name
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto accessorExit;
    }
    
    if (objectName.length() > 0) {
        i = sObjectMap.find(objectName);
        if (i == sObjectMap.end()) {
            TRACE_7("ERR_NOT_EXIST: Object '%s' does not exist", objectName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
            goto accessorExit;
        } else if(i->second->mObjFlags & IMM_CREATE_LOCK) {
            TRACE_7("ERR_NOT_EXIST: Object '%s' is being created, but ccb "
                    "or PRTO PBE, not yet applied", objectName.c_str());  
            err = SA_AIS_ERR_NOT_EXIST;
            goto accessorExit;
        } 
    }
    
    // Validate scope
    if (scope != SA_IMM_ONE) {
        LOG_NO("ERR_INVALID_PARAM: invalid search scope");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto accessorExit;
    }
    
    // Validate searchOptions
    if ((searchOptions & SA_IMM_SEARCH_ONE_ATTR) == 0) {
        //This should never happen as this value is set by imm-code and 
        //not user.
        LOG_ER("ERR_LIBRARY: Invalid search criteria - library problem ?");
        err = SA_AIS_ERR_LIBRARY;
        goto accessorExit;
    }
    
    switch(searchOptions & (SA_IMM_SEARCH_GET_ALL_ATTR | 
               SA_IMM_SEARCH_GET_NO_ATTR | 
               SA_IMM_SEARCH_GET_SOME_ATTR)) {
        case SA_IMM_SEARCH_GET_ALL_ATTR: break;
        case SA_IMM_SEARCH_GET_SOME_ATTR: break;
        default:
            LOG_WA("ERR_LIBRARY: Invalid search option - library problem ?");
            err = SA_AIS_ERR_LIBRARY;
            goto accessorExit;
    }
    
    //TODO: Reverse the order of matching attribute names.
    //The class should be searched first since we must return error
    //if the attribute is not defined in the class of the object. 
    
    obj = i->second;
    if(obj->mObjFlags & IMM_DN_INTERNAL_REP) {
        nameToExternal(objectName);
    }
    op.addObject(objectName);
    for(j = obj->mAttrValueMap.begin(); j != obj->mAttrValueMap.end(); j++) {
        if(searchOptions & SA_IMM_SEARCH_GET_SOME_ATTR) {
            bool notFound = true;
            ImmsvAttrNameList* list = req->attributeNames;
            
            if(!soughtAttributes) {
                while(list) {
                    ++soughtAttributes;
                    list = list->next;
                }
            }
            
            list = req->attributeNames;
            while(list) {
                size_t sz = strnlen((char *) list->name.buf, 
                    (size_t) list->name.size);
                std::string attName((const char *) list->name.buf, sz);
                if(j->first == attName) {
                    notFound = false;
                    ++matchedAttributes;
                    break;
                }
                list = list->next;
            }//while(list)
            if(notFound) {continue;}
        }
        AttrMap::iterator k =
            obj->mClassInfo->mAttrMap.find(j->first);
        assert(k != obj->mClassInfo->mAttrMap.end());
        op.addAttribute(j->first, k->second->mValueType, k->second->mFlags);
        //Check if attribute is the implementername attribute.
        //If so add the value artificially
        if(j->first.c_str() == std::string(SA_IMM_ATTR_IMPLEMENTER_NAME)) {
            if(obj->mImplementer) {
                j->second->
                    setValueC_str(obj->mImplementer->mImplementerName.c_str());
            } //else if(!j->second->empty()){ 
            //Discard obsolete implementer name
            //j->second->setValueC_str(NULL);
            //BUGFIX 081112
            //}
        }
        if(k->second->mFlags & SA_IMM_ATTR_CONFIG) {
            if(!j->second->empty()) {
                //Config attributes always accessible
                op.addAttrValue(*j->second);
            }
        } else { //Runtime attributes
            if(obj->mImplementer && obj->mImplementer->mNodeId ) {
                //There is an implementer
                if(!(k->second->mFlags & SA_IMM_ATTR_CACHED && 
                       j->second->empty())) {
                    op.addAttrValue(*j->second);  
                    //If the attribute is cached and empty then there 
                    //is truly no value and we are not supposed to fetch
                    //any. Thus we get here only if we are not cached
                    //or if we are not empty. 
                    //This means we can be empty and non cached.
                    //Should we really add a value then ? Yes because
                    //we are to fetch one from the implementer.
                }
                
                if(implNotSet && !(k->second->mFlags & SA_IMM_ATTR_CACHED)) {
                    //Non-cached rtattr and implementer exists => fetch it
                    op.setImplementer(obj->mImplementer->mConn,
                        obj->mImplementer->mNodeId,
                        obj->mImplementer->mMds_dest);
                    implNotSet = false;
                }
            } else {
                //There is no implementer
                if((k->second->mFlags & SA_IMM_ATTR_PERSISTENT) &&
                    !(j->second->empty())) {
                    op.addAttrValue(*j->second);
                    //Persistent rt attributes still accessible
                    //If they have been given any value
                }
                //No implementer and the rt attribute is not persistent 
                //then attribute name, but no value is to be returned.
                //Se spec p 42.
            }//No-impl
        }//Rt-attr
    }//for
    
    if((searchOptions & SA_IMM_SEARCH_GET_SOME_ATTR) &&
        (matchedAttributes != soughtAttributes)) {
        LOG_IN("ERR_NOT_EXIST: Some attributeNames did not exist in Object '%s' "
            "(nrof names:%u matched:%u)", objectName.c_str(),
            soughtAttributes, matchedAttributes);
        err = SA_AIS_ERR_NOT_EXIST;
    }
    
 accessorExit:
    TRACE_LEAVE(); 
    return err; 
}


bool
ImmModel::filterMatch(ObjectInfo* obj, ImmsvOmSearchOneAttr* filter, 
    SaAisErrorT& err, const char* objName)
{
    //TRACE_ENTER();
    std::string attrName((const char *) filter->attrName.buf);
    //(size_t) filter->attrName.size);
    
    //First check in the class of the object if the attribute should exist.
    AttrMap::iterator k = obj->mClassInfo->mAttrMap.find(attrName);
    if(k == obj->mClassInfo->mAttrMap.end()) {
        //TRACE_LEAVE();
        return false;
    }
    
    //Attribute exists in object.
    //If there is no value to match, then the filter is only on attribute name
    //Note that "no value" is encoded as zero sized string for all types

    //TODO: This branch really needs component testing!!!
    //All cases of: No value, string value, other type of value
    //Problem: This representation of null value eliminates possibility to
    //match on the zero length string!! Conclusion: Need different rep for
    //filter. OR: Is it even posible to assign a zero sized string value ?
    //The shortest possible string should have length 1.
    if(filter->attrValueType==SA_IMM_ATTR_SASTRINGT && 
        !filter->attrValue.val.x.size) {
        //TRACE_LEAVE();
        return true;
    }
    
    //There is a value to match => Check that the types match. 
    if(k->second->mValueType != 
        (unsigned int) filter->attrValueType) {
        //TRACE_LEAVE();
        return false;
    }
    
    if((k->second->mFlags & SA_IMM_ATTR_RUNTIME) &&
        !(k->second->mFlags & SA_IMM_ATTR_CACHED)){
        //TODO ABT We dont yet handle the case of attribute matching on 
        //non-cached runtime attributes. => Warn for this case.
        //To implement this, we should return true here, then fetch the
        //attribute as part of getNext, then check if the fetched attribute
        //does match.
        //If it does not match then simply discard the search object.
        LOG_WA("ERR_NO_RESOURCES: Attribute %s is a non-cached runtime "
            "attribute in object %s => can not handle search with match "
             "on such attributes currently.", attrName.c_str(), objName);
        err = SA_AIS_ERR_NO_RESOURCES;
        //TRACE_LEAVE();
        return false;
    }
    
    //Finally, check that the values match.
    ImmAttrValueMap::iterator j = obj->mAttrValueMap.find(attrName);
    assert(j != obj->mAttrValueMap.end());
    
    //If the attribute is the implementer name attribute, then assign it now.
    if((j->first.c_str() == std::string(SA_IMM_ATTR_IMPLEMENTER_NAME)) && 
        obj->mImplementer) {
        j->second->setValueC_str(obj->mImplementer->mImplementerName.c_str());
    }
    
    if(j->second->empty()) {
        //TRACE_LEAVE();
        return false;
    }
    
    //Convert match-value to octet string
    IMMSV_OCTET_STRING tmpos;
    eduAtValToOs(&tmpos, &(filter->attrValue), 
        (SaImmValueTypeT) filter->attrValueType);
    
    //TRACE_LEAVE();
    return j->second->hasMatchingValue(tmpos);
}

SaAisErrorT
ImmModel::searchInitialize(const ImmsvOmSearchInit* req, ImmSearchOp& op)
{
    TRACE_ENTER();
    
    SaAisErrorT err = SA_AIS_OK;
    bool filter=false;
    bool isDumper=false;
    bool isSyncer=false;
    
    if(immNotWritable()) {
        op.setImmNotWritable();
    }
    
    SaImmScopeT scope = (SaImmScopeT)req->scope;
    
    if(scope == SA_IMM_ONE) {
        return this->accessorGet(req, op);
    }
    
    size_t sz = strnlen((char *) req->rootName.buf, 
        (size_t)req->rootName.size);
    std::string rootName((const char*)req->rootName.buf, sz);
    
    SaImmSearchOptionsT searchOptions = 
        (SaImmSearchOptionsT)req->searchOptions;
    ObjectMap::iterator i;
    
    // Validate root name
    if(! (nameCheck(rootName)||nameToInternal(rootName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper root name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto searchInitializeExit;
    }
    
    if (rootName.length() > 0) {
        i = sObjectMap.find(rootName);
        if (i == sObjectMap.end()) {
            TRACE_7("ERR_NOT_EXIST: root object '%s' does not exist",
                rootName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
            goto searchInitializeExit;
        } else if(i->second->mObjFlags & IMM_CREATE_LOCK) {
            TRACE_7("ERR_NOT_EXIST: Root object '%s' is being created, "
                     "but ccb or PRTO PBE not yet applied",
                     rootName.c_str()); 
            err = SA_AIS_ERR_NOT_EXIST;
            goto searchInitializeExit;
        }
    }
    
    // Validate scope
    if((scope != SA_IMM_SUBLEVEL) && (scope != SA_IMM_SUBTREE)) {
        LOG_NO("ERR_INVALID_PARAM: invalid search scope");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto searchInitializeExit;
    }
    
    filter = (req->searchParam.choice.oneAttrParam.attrName.size != 0);
    
    // Validate searchOptions
    if (filter && ((searchOptions & SA_IMM_SEARCH_ONE_ATTR) == 0)) {
        LOG_NO("ERR_INVALID_PARAM: The SA_IMM_SEARCH_ONE_ATTR flag "
            "must be set in the searchOptions parameter");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto searchInitializeExit;
    }
    
    switch (searchOptions & (SA_IMM_SEARCH_GET_ALL_ATTR | 
                SA_IMM_SEARCH_GET_NO_ATTR | 
                SA_IMM_SEARCH_GET_SOME_ATTR)) {
        case SA_IMM_SEARCH_GET_ALL_ATTR: break;
        case SA_IMM_SEARCH_GET_NO_ATTR:  break;
        case SA_IMM_SEARCH_GET_SOME_ATTR: break;
        default:
            LOG_NO("ERR_INVALID_PARAM: invalid search option");
            err = SA_AIS_ERR_INVALID_PARAM;
            goto searchInitializeExit;
    }
    
    isDumper = (searchOptions & SA_IMM_SEARCH_PERSISTENT_ATTRS);
    isSyncer= (searchOptions & SA_IMM_SEARCH_SYNC_CACHED_ATTRS);
    
    // Find root object and all sub objects to the root object.
    for (i = sObjectMap.begin(); 
         err==SA_AIS_OK && i != sObjectMap.end(); i++) {
        ObjectInfo* obj = i->second;
        if(obj->mObjFlags & IMM_CREATE_LOCK) {continue;} /*Skip pending creates.*/
        std::string objectName = i->first;
        size_t rootlen = rootName.length();
        if (objectName.length() >= rootlen) {
            size_t pos = objectName.length() - rootName.length();
            if((objectName.rfind(rootName, pos) == pos)&&
                (!pos //Object IS the current root
                    || !rootlen //Empty root => all objects are sub-rootd.
                    || (objectName[pos-1] == ',') //Root with subobject
                 )){
                if(scope==SA_IMM_SUBTREE || checkSubLevel(objectName, pos)){
                    //Before adding the object to the result, check if there
                    //is any attribute match filter. If so check if there is a 
                    //match.
                    if(filter && 
                        !filterMatch(obj, (ImmsvOmSearchOneAttr *)
                            &(req->searchParam.choice.oneAttrParam), 
                            err, objectName.c_str())) {
                        continue; //filter missmatch
                    }
                    if(obj->mObjFlags & IMM_DN_INTERNAL_REP) {
                        nameToExternal(objectName);
                    }
                    /*TRACE_7("Add object:%s flags:%u", objectName.c_str(), obj->mObjFlags);*/
                    op.addObject(objectName);

                    if(searchOptions & (SA_IMM_SEARCH_GET_ALL_ATTR |
                           SA_IMM_SEARCH_GET_SOME_ATTR)) {
                        ImmAttrValueMap::iterator j;
                        bool implNotSet = true;
                        for (j = obj->mAttrValueMap.begin(); 
                             j != obj->mAttrValueMap.end(); 
                             j++) {
                            if(searchOptions & SA_IMM_SEARCH_GET_SOME_ATTR) {
                                bool notFound = true;
                                ImmsvAttrNameList* list = req->attributeNames;
                                while(list) {
                                    size_t sz = strnlen((char *) 
                                        list->name.buf, 
                                        (size_t) list->name.size);
                                    std::string attName((const char *) 
                                        list->name.buf, sz);
                                    if(j->first == attName) {
                                        notFound = false;
                                        break;
                                    }
                                    list = list->next;
                                }//while(list)
                                if(notFound) {continue;}
                            }
                            AttrMap::iterator k =
                                obj->mClassInfo->mAttrMap.find(j->first);
                            assert(k != obj->mClassInfo->mAttrMap.end());
                            
                            if(isDumper && 
                                !(k->second->mFlags & SA_IMM_ATTR_CONFIG) &&
                                !(k->second->mFlags & SA_IMM_ATTR_PERSISTENT)){
                                continue;
                                //If we are the dumper, then exclude
                                //non-persistent rt attributes, even when there
                                //is an implementer. For normal applications, 
                                //the above if evaluates to false, because
                                //isDumper is false. 
                            }
                            
                            op.addAttribute(j->first, k->second->mValueType, 
                                k->second->mFlags);
                            //Check if attribute is the implementername
                            //attribute. if so add the value artificially
                            if(j->first.c_str() == 
                                std::string(SA_IMM_ATTR_IMPLEMENTER_NAME)) {
                                if(obj->mImplementer) {
                                    j->second->setValueC_str(obj->
                                        mImplementer->
                                        mImplementerName.c_str());
                                } //else if(!j->second->empty()) {
                                //Discard obsolete implementer name
                                //j->second->setValueC_str(NULL);
                                //BUGFIX 081112
                                //}
                            }
                            
                            if(k->second->mFlags & SA_IMM_ATTR_CONFIG) {
                                if(!j->second->empty()) {
                                    //Config attributes always accessible
                                    op.addAttrValue(*j->second);
                                }
                            } else { //Runtime attributes
                                if(obj->mImplementer && 
                                    obj->mImplementer->mNodeId) {
                                    //There is an implementer
                                    if(!(
                                         k->second->mFlags & SA_IMM_ATTR_CACHED
                                         && j->second->empty())) {
                                        op.addAttrValue(*j->second);
                                        //If the attribute is cached and empty
                                        //then there is truly no value and we
                                        //are not supposed to fetch any. Thus
                                        //we get here only if we are not cached
                                        //or if we are not empty. 
                                        //This means we can be empty and non
                                        //cached. Should we really add a value
                                        //then ? Yes because we are to fetch
                                        //one from the implementer.
                                    }
                                    
                                    if(implNotSet && 
                                        !(k->second->mFlags & 
                                            SA_IMM_ATTR_CACHED)) {
                                        //Non cached rtattr and implementer
                                        //exists => fetch it
                                        op.setImplementer(obj->mImplementer->
                                            mConn,
                                            obj->mImplementer->mNodeId,
                                            obj->mImplementer->mMds_dest);
                                        implNotSet = false;
                                    }
                                } else {
                                    //There is no implementer
                                    if(!(j->second->empty()) 
                                        && (k->second->mFlags & 
                                            SA_IMM_ATTR_PERSISTENT ||
                                            (isSyncer && k->second->mFlags & 
                                                SA_IMM_ATTR_CACHED))) {
                                        //There is a value AND (
                                        //the attribute is persistent OR 
                                        //this is a syncer and the attribute is
                                        //cached. The special case for the
                                        //sync'er is to ensure that we still
                                        //sync cached values even when there
                                        //is no implementer currently. 
                                        op.addAttrValue(*j->second);
                                    }
                                    //No implementer and the rt attribute is
                                    //not persistent, then attribute name, but
                                    //no value is to be returned. 
                                    //Se spec p 42.
                                }//No-imp
                            }//Runtime
                        }//for(..
                    }
                }
            }
        } 
    }
    
 searchInitializeExit:
    TRACE_LEAVE(); 
    return err; 
}

/** 
 * Administrative oepration invoke.
 */
SaAisErrorT ImmModel::adminOperationInvoke(
                                           const ImmsvOmAdminOperationInvoke* req,
                                           SaUint32T reqConn,
                                           SaUint64T reply_dest,
                                           SaInvocationT& saInv,
                                           SaUint32T* implConn,
                                           unsigned int* implNodeId)
{
    TRACE_ENTER();
    SaAisErrorT err = SA_AIS_OK;
    
    size_t sz = strnlen((char *) req->objectName.buf, 
        (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    TRACE_5("Admin op on objectName:%s\n", objectName.c_str());
    
    SaUint32T adminOwnerId = req->adminOwnerId;
    
    AdminOwnerInfo* adminOwner = 0;
    //ClassInfo* classInfo = 0;
    ObjectInfo* object = 0;
    
    AdminOwnerVector::iterator i2;
    ObjectMap::iterator oi;
    std::string objAdminOwnerName;
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        TRACE_LEAVE();
        return SA_AIS_ERR_INVALID_PARAM;
    }
    
    i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
        IdIs(adminOwnerId));
    if ((i2 != sOwnerVector.end()) && (*i2)->mDying) {
        i2 = sOwnerVector.end();//Pretend that it does not exist.
        TRACE_5("Attempt to use dead admin owner in adminOperationInvoke");
    }
    if (i2 == sOwnerVector.end()) {
        LOG_IN("ERR_BAD_HANDLE: admin owner id %u does not exist", adminOwnerId);
        TRACE_LEAVE();
        return SA_AIS_ERR_BAD_HANDLE;
    }
    adminOwner = *i2;
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN attribute value");
        TRACE_LEAVE();
        return SA_AIS_ERR_INVALID_PARAM;     
    }
    
    oi = sObjectMap.find(objectName);
    if (oi == sObjectMap.end()) {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
        TRACE_LEAVE();
        return SA_AIS_ERR_NOT_EXIST;
    }
    
    object = oi->second;
    object->getAdminOwnerName(&objAdminOwnerName);
    
    if(objAdminOwnerName != adminOwner->mAdminOwnerName)
    {
        LOG_IN("ERR_BAD_OPERATION: Mismatch on administrative owner %s/%u != %s/%u", 
            objAdminOwnerName.c_str(), 
            (unsigned int) objAdminOwnerName.size(),
            adminOwner->mAdminOwnerName.c_str(),
            (unsigned int) adminOwner->mAdminOwnerName.size());
        TRACE_LEAVE();
        return SA_AIS_ERR_BAD_OPERATION;
    }
    
    CcbVector::iterator i3;
    SaUint32T ccbIdOfObj = object->mCcbId;
    if(ccbIdOfObj) {//check for ccb interference
        i3 = std::find_if(sCcbVector.begin(), sCcbVector.end(), CcbIdIs(ccbIdOfObj));
        if (i3 != sCcbVector.end() && (*i3)->isActive()) {
            LOG_IN("ERR_BUSY: ccb id %u is active on object %s", 
                ccbIdOfObj, objectName.c_str());
            TRACE_LEAVE();
            return SA_AIS_ERR_BUSY;
        }
    }
    
    
    // Check for call on object implementor
    if(object->mImplementer && object->mImplementer->mNodeId) {
        *implConn = object->mImplementer->mConn;
        *implNodeId = object->mImplementer->mNodeId;
        
        TRACE_5("IMPLEMENTER FOR ADMIN OPERATION INVOKE %u "
            "conn:%u node:%x name:%s", object->mImplementer->mId, *implConn,
            *implNodeId, object->mImplementer->mImplementerName.c_str());
        
        //If request originated on this node => register request continuation.
        if(reqConn) {
            SaUint32T timeout = 
                (req->timeout)?((SaUint32T) (req->timeout)/100 + 1):0;
            TRACE_5("Storing req invocation %u for inv: %llu timeout:%u", reqConn, saInv,
                timeout);
            sAdmReqContinuationMap[saInv] = ContinuationInfo2(reqConn, timeout);
        }
        
        if(*implConn) {
            if(object->mImplementer->mDying) {
                LOG_WA("Lost connection with implementer %s %u in admin op",
                    object->mImplementer->mImplementerName.c_str(),
                    object->mImplementer->mId);
                err = SA_AIS_ERR_NOT_EXIST;
                *implConn = 0;
            } else {
                if(object->mImplementer->mAdminOpBusy) {
                    LOG_WA("Implementer for object: %s is already busy with admop",
                        objAdminOwnerName.c_str());
                    //TODO: This should generate SA_AIS_ERR_BUSY for ccb ops, 
                    //but we dont have reliable cleanup yet.
                    //It is the "object" which is bussy not the "implementer".
                }

                TRACE_5("Storing impl invocation %u for inv: %llu", 
                    *implConn, saInv);

                ++(object->mImplementer->mAdminOpBusy);
                sAdmImplContinuationMap[saInv] =
                    ContinuationInfo3(*implConn, reply_dest, 
                        object->mImplementer);
            }
        }
    } else {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not have an implementer", 
            objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
    }
    TRACE_LEAVE(); 
    return err;
}

void
ImmModel::setSearchReqContinuation(SaInvocationT& saInv, SaUint32T reqConn)
{
    TRACE_ENTER();
    TRACE_5("setSearchReqContinuation <%llu, %u>", saInv, reqConn);
    sSearchReqContinuationMap[saInv] = ContinuationInfo2(reqConn, DEFAULT_TIMEOUT_SEC);
    TRACE_LEAVE();
}

void
ImmModel::setSearchImplContinuation(SaInvocationT& saInv, SaUint64T reply_dest)
{
    sSearchImplContinuationMap[saInv] = ContinuationInfo3(0, reply_dest, NULL);
}

void
ImmModel::fetchSearchReqContinuation(SaInvocationT& inv, SaUint32T* reqConn)
{
    TRACE_ENTER();
    ContinuationMap2::iterator ci = sSearchReqContinuationMap.find(inv);
    if(ci == sSearchReqContinuationMap.end()) { 
        *reqConn=0;
        TRACE_5("Failed to find continuation for inv:%llu", inv);
        return;
    }
    TRACE_5("REQ SEARCH CONTINUATION %u FOUND FOR %llu", 
        ci->second.mConn, inv);
    *reqConn = ci->second.mConn;
    sSearchReqContinuationMap.erase(ci);
    TRACE_LEAVE();
}

void
ImmModel::fetchSearchImplContinuation(SaInvocationT& inv, 
    SaUint64T* reply_dest)
{
    ContinuationMap3::iterator ci = sSearchImplContinuationMap.find(inv);
    if(ci == sSearchImplContinuationMap.end()) { 
        *reply_dest=0LL;
        return;
    }
    TRACE_5("IMPL SEARCH CONTINUATION FOUND FOR %llu", inv);
    *reply_dest = ci->second.mReply_dest;
    sSearchImplContinuationMap.erase(ci);
}

void
ImmModel::fetchAdmImplContinuation(SaInvocationT& inv, 
    SaUint32T* implConn,
    SaUint64T* reply_dest)
{
    TRACE_5("Fetch implCon for invocation:%llu", inv);
    ContinuationMap3::iterator ci3 = sAdmImplContinuationMap.find(inv);
    if(ci3 == sAdmImplContinuationMap.end()) {
        *implConn = 0;
        *reply_dest = 0LL;
        return;
    }
    if((*implConn) && (*implConn != ci3->second.mConn)) {
        LOG_ER("Collision on invocation identity %llu", inv);
    } 
    TRACE_5("IMPL ADM CONTINUATION %u FOUND FOR %llu", ci3->second.mConn, inv);
    *implConn = ci3->second.mConn;
    *reply_dest = ci3->second.mReply_dest;
    (ci3->second.mImplementer->mAdminOpBusy)--;
    if(ci3->second.mImplementer->mAdminOpBusy) {
        LOG_WA("Waiting for %u additional admop replies after admop return", 
            ci3->second.mImplementer->mAdminOpBusy);
    }
    sAdmImplContinuationMap.erase(ci3);
}

void
ImmModel::fetchAdmReqContinuation(SaInvocationT& inv, SaUint32T* reqConn)
{
    ContinuationMap2::iterator ci = sAdmReqContinuationMap.find(inv);
    if(ci == sAdmReqContinuationMap.end()) { 
        *reqConn=0;
        return;
    }
    
    TRACE_5("REQ ADM CONTINUATION %u FOUND FOR %llu", ci->second.mConn, inv);
    *reqConn = ci->second.mConn;
    sAdmReqContinuationMap.erase(ci);
}

bool
ImmModel::checkSubLevel(const std::string& objectName, 
    size_t rootStart)
{
    std::string sublevel = objectName.substr(0, rootStart?(rootStart - 1):0);
    return sublevel.find(',') == std::string::npos;
}

bool
ImmModel::schemaNameCheck(const std::string& name) const
{
    /* Dont allow some chars in class & attribute names that cause
       problems in sqlite. Each imm-class is mapped to several tables,
       but one table is named usingthe classname. 
    */
    unsigned char chr;
    size_t pos;
    size_t len = name.length();

    if(!nameCheck(name)) {
        return false;
    }

    for(pos=0; pos < len; ++pos) {
        chr = name.at(pos);
        if(isalnum(chr)||(chr == 95)) {
            continue;      /* _ */
        } else {
            LOG_IN("Bad class/attribute name: '%s' (%c): pos=%zu", 
                name.c_str(), chr, pos);
            return false;
        }
    }

    chr = name.at(0);

    if(isdigit(chr)) {
        LOG_IN("Bad class/attribute name starts with number: '%s' (%c): pos=%u", 
            name.c_str(), chr, 0);
        return false;
    }

    return true;
}


bool
ImmModel::nameCheck(const std::string& name, bool strict) const
{
    size_t pos;
    size_t len = name.length();
    unsigned char prev_chr = '\0';
    
    for(pos=0; pos < len; ++pos) {
        unsigned char chr = name.at(pos);
        
        if((((chr == ',') || (strict && (chr == '#'))) && 
               (prev_chr == '\\')) || 
                (!isgraph(chr) && !(chr == '\0' && pos == len - 1)))
        {
            /*TRACE_5("bad name size:%u '%s'", (unsigned int) len, name.c_str());*/
            TRACE_5("Bad name. string size:%zu isgraph(%c):%u, pos=%zu", 
                len, chr, isgraph(chr), pos);
            return false;
        }
        prev_chr = chr;
    }
    return true;
}

bool
ImmModel::nameToInternal(std::string& name)
{
    bool effective = false;
    size_t pos;
    size_t len = name.length();
    unsigned char prev_chr = '\0';
    
    for(pos=0; pos < len; ++pos) {
        unsigned char chr = name.at(pos);
        //TRACE("nameToInternal: %u %u %c", len, pos, chr);
        if(prev_chr == '\\') {
            if(chr == ',') 
            {
                name.replace(pos, 1, 1, '#');
                effective = true;
            } else if(chr == '#') {
                LOG_WA("Can not accept external DN with escaped hash '\\#' "
                    "pos:%zu in %s", pos, name.c_str());
                return false;
            }
        }
        prev_chr = chr;
    }

    /*
    if(effective) {
        TRACE_5("Replaced occurences of '\\,' with '\\#' result:%s",
            name.c_str());
    }
    */

    return nameCheck(name, false);
}

void
ImmModel::nameToExternal(std::string& name)
{
    bool effective = false;
    size_t pos;
    size_t len = name.length();
    unsigned char prev_chr = '\0';
    
    for(pos=0; pos < len; ++pos) {
        unsigned char chr = name.at(pos);
        //TRACE("nameToInternal: %u %u %c", len, pos, chr);
        if((chr == '#') && (prev_chr == '\\'))
        {
            name.replace(pos, 1, 1, ',');
            effective = true;
        }
        prev_chr = chr;
    }

    /*
    if(effective) {
        TRACE_5("Replaced occurences of '\\#' with '\\,' result:%s",
            name.c_str());
    }
    */
}

void
ImmModel::updateImmObject(std::string newClassName, 
    bool remove) //default is remove == false
{
    TRACE_ENTER();
    ObjectMap::iterator oi = sObjectMap.find(immObjectDn);
    
    if(oi == sObjectMap.end()) {
        TRACE_LEAVE();
        return;  //ImmObject not created yet.
    }
    
    ObjectInfo* immObject = oi->second;
    
    ImmAttrValueMap::iterator avi = 
        immObject->mAttrValueMap.find(immAttrClasses);
    
    assert(avi != immObject->mAttrValueMap.end());
    
    assert(avi->second->isMultiValued());
    ImmAttrMultiValue* valuep = (ImmAttrMultiValue *) avi->second;
    
    if(remove) {
        assert(valuep->removeExtraValueC_str(newClassName.c_str()));
    } else if(newClassName == immClassName) {
        TRACE_5("Imm meta object class %s added", immClassName.c_str());
        //First time, include all current classes
        //Set first value to the class name of the Imm class itself
        valuep->setValueC_str(immClassName.c_str());
        
        ClassMap::iterator ci;
        for(ci = sClassMap.begin(); ci != sClassMap.end(); ++ci) {
            //Check that it does not already exist.
            //This should be done in a better way. 
            //An attribute in the imm object that
            //allows us to simply test if this is 
            //intial start or a reload.
            if((ci->first !=immClassName) &&
                (!valuep->hasExtraValueC_str(ci->first.c_str()))) {
                TRACE_5("Adding existing class %s", ci->first.c_str());
                valuep->setExtraValueC_str(ci->first.c_str());
            }
        }
    } else {
        //Just add the newly added class.
        //Check that it does not already exist.
        //This should be done in a better way. 
        //An attribute in the imm object that
        //allows us to simply test if this is intial start or a reload.
        if(!valuep->hasExtraValueC_str(newClassName.c_str())) {
            TRACE_5("Adding new class %s", newClassName.c_str());
            valuep->setExtraValueC_str(newClassName.c_str());
        } else {
            TRACE_5("Class %s already existed", newClassName.c_str());
        }
    }
    TRACE_LEAVE();
}

SaUint32T
ImmModel::findConnForImplementerOfObject(std::string objectDn)
{
    ObjectInfo* object;
    ObjectMap::iterator oi;

    if(! (nameCheck(objectDn)||nameToInternal(objectDn)) ) {
        LOG_ER("Invalid object name %s sent internally", objectDn.c_str());
        assert(0);
    }

    oi = sObjectMap.find(objectDn);
    if (oi == sObjectMap.end()) {
        TRACE_7("Object '%s' %u does not exist", objectDn.c_str(), 
            (unsigned int) objectDn.size());
        return 0;
    }
    
    object = oi->second;
    if(!object->mImplementer) {
        return 0;
    }
    
    return object->mImplementer->mConn;
}

ImplementerInfo*
ImmModel::findImplementer(SaUint32T id)
{
    //TODO change rep to handle a large number of implementers ? 
    //Current is not efficient with vector/linear
    ImplementerVector::iterator i;
    for(i = sImplementerVector.begin(); i != sImplementerVector.end(); ++i) {
        ImplementerInfo* info = (*i);
        if(info->mId == id) return info;
    }
    
    return NULL;
}

ImplementerInfo*
ImmModel::findImplementer(std::string& iname)
{
    //TODO: use a map <name>->impl instead.
    ImplementerVector::iterator i;
    for(i = sImplementerVector.begin(); i != sImplementerVector.end(); ++i) {
        ImplementerInfo* info = (*i);
        if(info->mImplementerName == iname) return info;
    }
    
    return NULL;
}

SaUint32T
ImmModel::getImplementerId(SaUint32T localConn)
{
    assert(localConn);
    ImplementerVector::iterator i;
    for(i = sImplementerVector.begin(); i != sImplementerVector.end(); ++i) {
        ImplementerInfo* info = (*i);
        if(info->mConn == localConn) return info->mId;
    }
    
    return 0;
}

void
ImmModel::discardNode(unsigned int deadNode, IdVector& cv)
{
    ImplementerVector::iterator i;
    AdminOwnerVector::iterator i2;
    CcbVector::iterator i3;
    TRACE_ENTER();

    if(sImmNodeState == IMM_NODE_W_AVAILABLE) {
        /* Sync is ongoing and we are a sync client.
           Remember the death of the node. 
        */
        TRACE_7("Sync client notes death of node %x", deadNode);
        sNodesDeadDuringSync.push_back(deadNode);
        goto done; /* Postpone discardNode untill after finalizeSync. */
    }

    if(sImmNodeState == IMM_NODE_R_AVAILABLE) {
        /* Sync is ongoing but we are not a sync client.
           Remember the death of the node for verify in finalizeSync
        */
        TRACE_7("Death of node %x noted during on-going sync", deadNode);
        sNodesDeadDuringSync.push_back(deadNode);
    }

    //Discard implementers from that node.
    for(i = sImplementerVector.begin(); i != sImplementerVector.end(); ++i) {
        ImplementerInfo* info = (*i);
        if(info->mNodeId == deadNode) {
            //discardImplementer(info->mId); 
            //Doing it directly here for efficiency.
            //But watch out for changes in discardImplementer
            LOG_IN("DISCARDING IMPLEMENTER %u <%u, %x>", 
                info->mId, info->mConn, info->mNodeId);
            info->mId = 0;
            info->mConn = 0;
            info->mNodeId = 0;
            info->mMds_dest = 0LL;
            info->mAdminOpBusy = 0; 
            info->mDying = false; //We are so dead now we dont need to remember
            //We dont need the dying phase for implementer when an entire node
            //is gone. Thew dying phase is only needed to handle trailing evs
            //messages that depend on the implementer. We DO NOT want different
            //behavior at different nodes, only because an implementor is gone.
            
            //We keep the ImplementerInfo entry with the implementer name, 
            //for now.
            //The ImplementerInfo is pointed to by ClassInfo or ObjectInfo.
            //The point is to allow quick re-attachmenet by a process to the 
            //implementer.
        }
    }
    
    //Discard AdminOwners
    i2 = sOwnerVector.begin();
    while(i2 != sOwnerVector.end()) {
        AdminOwnerInfo* ainfo = (*i2);
        if(ainfo->mNodeId == deadNode && (!ainfo->mDying)) {
            //Inefficient: lookup of admin owner again.
            if(adminOwnerDelete(ainfo->mId, true) == SA_AIS_OK) {
                i2 = sOwnerVector.begin();//restart of iteration again.
            } else { 
                LOG_WA("Could not delete admin owner %u %s", ainfo->mId,
                    ainfo->mAdminOwnerName.c_str());
                ++i2;
            }
        } else { ++i2;}
    }
    
    //Fetch CcbIds for Ccbs originating at the departed node.
    for(i3=sCcbVector.begin(); i3!=sCcbVector.end(); ++i3) {
        if(((*i3)->mOriginatingNode == deadNode) && ((*i3)->isActive())) {
            cv.push_back((*i3)->mId);
            assert((*i3)->mOriginatingConn == 0); //Dead node can not be us!!
        }
    }
 done:    
    TRACE_LEAVE();
}

void
ImmModel::discardImplementer(unsigned int implHandle, bool reallyDiscard)
{
    //Note: If this function is altered, then you may need to make
    //changes in ImmModel::discardNode, since that function also deletes
    //implementers.
    TRACE_ENTER();
    ImplementerInfo* info = findImplementer(implHandle);
    if(info) {
        if(reallyDiscard) {
            LOG_IN("DISCARDING IMPLEMENTER %u <%u, %x> (%s)", 
                info->mId, info->mConn, info->mNodeId,
                info->mImplementerName.c_str());
            info->mId = 0;
            info->mConn = 0;
            info->mNodeId = 0;
            info->mMds_dest = 0LL;
            info->mAdminOpBusy = 0; 
            info->mDying = false;//We are so dead now, we dont need to remember
            //We keep the ImplementerInfo entry with the implementer name, 
            //for now. The ImplementerInfo is pointed to by ClassInfo or 
            //ObjectInfo. The point is to allow quick re-attachmenet by a 
            //process to the implementer.
            //But we have a problem of ever growing number of distinct
            //implementers. Possible solution is to here iterate through all
            //classes and objects. If none point to this object then we can
            //actually remove it.
           if(sImmNodeState == IMM_NODE_R_AVAILABLE) {
                /* Sync is ongoing but we are not a sync client. 
                   Remember the death of the implementer. */
                sImplsDeadDuringSync.push_back(implHandle);
            }
        } else {
            if(!info->mDying) {
                LOG_IN("Implementer %u disconnected. Marking it as doomed "
                    "<Conn:%u, Node:%x>", info->mId, info->mConn, 
                    info->mNodeId);
                info->mDying = true;
            }
        }
    } else { /* implementer not found */
        if(sImmNodeState == IMM_NODE_W_AVAILABLE) {
            /* Sync is ongoing and we are a sync client.
               Remember the death of the implementer. 
            */
            TRACE_7("Sync client notes death of implementer %u", implHandle);
            sImplsDeadDuringSync.push_back(implHandle);
        } else {
            LOG_WA("discardImplementer: Implementer %u is missing - ignoring", implHandle);
            /* Should not happen */
        }
    }
    
    TRACE_LEAVE();
}

void
ImmModel::discardContinuations(SaUint32T dead)
{
    TRACE_ENTER();
    //ContinuationMap::iterator ci;
    ContinuationMap2::iterator ci2;
    ContinuationMap3::iterator ci3;
    
    for(ci2=sAdmReqContinuationMap.begin(); 
        ci2!=sAdmReqContinuationMap.end();) {
        if(ci2->second.mConn == dead) {
            TRACE_5("Discarding Adm Req continuation %llu", ci2->first);
            sAdmReqContinuationMap.erase(ci2);
            ci2=sAdmReqContinuationMap.begin(); 
        } else { ++ci2;}
    }
    
    for(ci3=sAdmImplContinuationMap.begin(); 
        ci3!=sAdmImplContinuationMap.end();) {
        if(ci3->second.mConn == dead) {
            TRACE_5("Discarding Adm Impl continuation %llu", ci3->first);
            //TODO Should send fevs message to terminate call that caused
            //the implementer continuation to be created.
            //I can only do this if I check that IMMD is up first. 
            sAdmImplContinuationMap.erase(ci3);
            ci3=sAdmImplContinuationMap.begin();
        } else {++ci3;}
    }
    
    for(ci3=sSearchImplContinuationMap.begin(); 
        ci3!=sSearchImplContinuationMap.end();) {
        if(ci3->second.mConn == dead) {
            TRACE_5("Discarding Adm Impl continuation %llu", ci3->first);
            //TODO: Should send fevs message to terminate call that caused
            //the implementer continuation to be created.
            //I can only do this if I check that IMMD is up first. 
            sSearchImplContinuationMap.erase(ci3);
            ci3=sSearchImplContinuationMap.begin();
        } else {++ci3;}
    }
    
    for(ci2=sSearchReqContinuationMap.begin(); 
        ci2!=sSearchReqContinuationMap.end();) {
        if(ci2->second.mConn == dead) {
            TRACE_5("Discarding Search Req continuation %llu", ci2->first);
            sSearchReqContinuationMap.erase(ci2);
            ci2=sSearchReqContinuationMap.begin();
        } else {++ci2;}
    }
    
    //TODO: implementer still potentially refered to from Ccbs (mImplementers)
    
    TRACE_LEAVE();
    return;
}

void
ImmModel::getCcbIdsForOrigCon(SaUint32T dead, IdVector& cv)
{
    CcbVector::iterator i;
    for(i=sCcbVector.begin(); i!=sCcbVector.end(); ++i) {
        if(((*i)->mOriginatingConn == dead) && ((*i)->isActive())) {
            cv.push_back((*i)->mId);
            /* Do not set: (*i)->mOriginatingConn = 0; 
               because stale client handling requires repeated invocation
               of this function. */
        }
    }
}


/* Are all ccbs terminated ? */
bool
ImmModel::ccbsTerminated()
{
    CcbVector::iterator i;

    if(sCcbVector.empty()) {return true;}

    for(i=sCcbVector.begin(); i!=sCcbVector.end(); ++i) {
        if((*i)->mState < IMM_CCB_COMMITTED) {
            TRACE("Waiting for CCB:%u in state %u", 
                (*i)->mId, (*i)->mState);
            return false;
        }
    }

    return true;
}

void
ImmModel::getNonCriticalCcbs(IdVector& cv)
{
    CcbVector::iterator i;
    for(i=sCcbVector.begin(); i!=sCcbVector.end(); ++i) {
        if((*i)->mState < IMM_CCB_CRITICAL) {
            cv.push_back((*i)->mId);
        }
    }
}

void
ImmModel::getOldCriticalCcbs(IdVector& cv, SaUint32T *pbeConnPtr,
    unsigned int *pbeNodeIdPtr, SaUint32T* pbeIdPtr)
{
    assert(pbeConnPtr);
    assert(pbeNodeIdPtr);
    assert(pbeIdPtr);
    *pbeNodeIdPtr = 0;
    *pbeConnPtr = 0;
    *pbeIdPtr = 0;
    CcbVector::iterator i;
    time_t now = time(NULL);
    for(i=sCcbVector.begin(); i!=sCcbVector.end(); ++i) {
        if((*i)->mState == IMM_CCB_CRITICAL && 
           (((*i)->mWaitStartTime && 
             now - (*i)->mWaitStartTime >= DEFAULT_TIMEOUT_SEC)||/* Should be saImmOiTimeout*/
             (*i)->mPbeRestartId))
        {
            /* We have a critical ccb that has: timed out OR lived through a PBE restart.
               This means the PBE is slow in processing (hung?) OR the PBE has crashed.
               Find the ccb-implementer association and verify which case it is.
             */
            CcbInfo* ccb = (*i);
            CcbImplementerMap::iterator isi;
            ImplementerCcbAssociation* implAssoc = NULL;

            for(isi = ccb->mImplementers.begin(); isi != ccb->mImplementers.end(); ++isi) {
                implAssoc = isi->second;
                if(implAssoc->mWaitForImplAck) {
                    break;
                }
                implAssoc = NULL;
            }

            if(!implAssoc) {
                LOG_ER("CCB %u seems blocked in CRITICAL, yet NOT waiting on implementer!!",
                    ccb->mId);
                continue;
            }
            
            assert(implAssoc->mImplementer);

            bool isPbe = (implAssoc->mImplementer->mImplementerName ==
                std::string(OPENSAF_IMM_PBE_IMPL_NAME));


            if(!isPbe) {
                LOG_ER("CCB %u is blocked in CRITICAL, waiting on implementer %u/%s.",
                    ccb->mId, implAssoc->mImplementer->mId,
                    implAssoc->mImplementer->mImplementerName.c_str());
                continue;
            }

            TRACE("CCB %u is waiting on PBE commit", ccb->mId);
            ImplementerInfo* impInfo = (ImplementerInfo *)
                getPbeOi(pbeConnPtr, pbeNodeIdPtr);

            if(!impInfo) {
                LOG_IN("No PBE implementer registered at this time");
                /* TODO: Should check rim if it was switched to  FROM_FILE
                   FROM_FILE indicates the pathological case of PBE switched off
                   accidentally leaving a ccb in critical state. 
                */
                continue;
            } 

            if(impInfo->mId != ccb->mPbeRestartId) {
                LOG_WA("Missmatch between pbe-Id %u and pbe-restart-id %u", 
                    impInfo->mId, ccb->mPbeRestartId);
                if(ccb->mPbeRestartId == 0 &&
                   (implAssoc->mImplementer->mId == impInfo->mId)) {
                    LOG_ER("PBE implementer %u seems hung!", impInfo->mId);
                    /*TODO return true => signalling need to kill pbe*/
                } 
            } 

            /*This is the perfect match recovery case. **/
            LOG_IN("PBE implementer %s restarted, check ccb %u",
                impInfo->mImplementerName.c_str(), ccb->mId);
            (*pbeIdPtr) = impInfo->mId;
            cv.push_back((*i)->mId);
        }
    }
}

void
ImmModel::getAdminOwnerIdsForCon(SaUint32T dead, IdVector& cv)
{
    AdminOwnerVector::iterator i;
    for(i=sOwnerVector.begin(); i!=sOwnerVector.end(); ++i) {
        if((*i)->mConn == dead) {
            cv.push_back((*i)->mId);
            /* Do not set: (*i)->mConn = 0; 
               because stale client handling requires repeated invocation
               of this function. */
        }
    }
}

SaUint32T
ImmModel::cleanTheBasement(unsigned int seconds, InvocVector& admReqs,
    InvocVector& searchReqs, IdVector& ccbs, IdVector& pbePrtoReqs,
    bool iAmCoord)
{
    time_t now = time(NULL);
    ContinuationMap2::iterator ci2;
    CcbVector::iterator i3;
    CcbVector ccbsToGc;
    SaUint32T ccbsStuck=0; /* 0 or 1 */
    SaUint32T pbeRtRegress=0; /* 0 or 2 */
    
    for(ci2=sAdmReqContinuationMap.begin(); 
        ci2!=sAdmReqContinuationMap.end();
        ++ci2) {
        if(ci2->second.mTimeout && 
            (now - ci2->second.mCreateTime >= (int) ci2->second.mTimeout)) {
            TRACE_5("Timeout on AdministrativeOp continuation %llu  tmout:%u", 
                ci2->first, ci2->second.mTimeout);
            admReqs.push_back(ci2->first);
        } else {
            TRACE_5("Did not timeout now - start < %u(%lu)", ci2->second.mTimeout,
                now - ci2->second.mCreateTime);
        }
    }
    
    //sAdmImplContinuationMap is cleaned up when implementer detaches.
    //TODO: problem is implementer may stay connected for weeks and
    //there is no guarantee of explicit reply from implementer.
    //Possibly use the implicit reply of the return to cleanup contiuation.
    //Conclusion: I should add cleanup logic here anyway, since it is easy to 
    //do and solves the problem. 
    
    for(ci2=sSearchReqContinuationMap.begin(); 
        ci2!=sSearchReqContinuationMap.end();
        ++ci2) {
        //TODO the timeout should not be hardwired, but for now it is.
        if(now - ci2->second.mCreateTime >= DEFAULT_TIMEOUT_SEC) {
            TRACE_5("Timeout on Search continuation %llu", ci2->first);
            searchReqs.push_back(ci2->first);
        } 
    }
    
    //sSearchImplContinationMap is cleaned up when implementer detaches.
    //TODO See above problem description for sAdmImplContinuationMap.
    //Conclusion: I should add cleanup logic here anyway, since it is easy to
    //do and solves the problem. 

    
    for(i3=sCcbVector.begin(); i3!=sCcbVector.end(); ++i3) {
        if((*i3)->mState > IMM_CCB_CRITICAL) {
            /* Garbage Collect ccbInfo more than one minute old */
            if((*i3)->mWaitStartTime && (now - (*i3)->mWaitStartTime >= 60)) {
                TRACE_5("Removing CCB %u terminated more than 60 secs ago", 
                    (*i3)->mId);
                (*i3)->mState = IMM_CCB_ILLEGAL;
                ccbsToGc.push_back(*i3);
            }
        } else if(iAmCoord) {
            //Fetch CcbIds for Ccbs that have waited too long on an implementer
            //AND ccbIds for ccbs in critical and marked with PbeRestartedId.
            //Restarted PBE => try to recover outcome BEFORE timeout, making
            //recovery transparent to user!
            //TODO the timeout should not be hardwired, but for now it is.
            TRACE("Checking active ccb %u for deadlock or blocked implementer",
                (*i3)->mId);
            TRACE("state:%u waitsart:%u PberestartId:%u",(*i3)->mState, 
                (unsigned int) (*i3)->mWaitStartTime, (*i3)->mPbeRestartId);
            if(((*i3)->mWaitStartTime &&
                (now - (*i3)->mWaitStartTime >= DEFAULT_TIMEOUT_SEC)) ||
                ((*i3)->mPbeRestartId)) {
                //TODO Timeout value should be fetched from IMM service object.
                if((*i3)->mPbeRestartId) { 
                    TRACE_5("PBE restarted id:%u with ccb:%u in critical",
                        (*i3)->mPbeRestartId, (*i3)->mId);
                } else {
                    TRACE_5("CCB %u timeout while waiting on implementer reply",
                        (*i3)->mId);
                }
                if((*i3)->mState == IMM_CCB_CRITICAL) {
                    LOG_NO("Critical transaction backloged! ccb:%u",
                        (*i3)->mId);
                    ccbsStuck=1;
                } else {
                    ccbs.push_back((*i3)->mId); /*Non critical ccb to abort.*/
                }
            }
        }
    }

    while((i3 = ccbsToGc.begin()) != ccbsToGc.end()) {
        CcbInfo* ccb = (*i3);
        ccbsToGc.erase(i3);
        i3 = std::find_if(sCcbVector.begin(), sCcbVector.end(),
            CcbIdIs(ccb->mId));
        assert(i3 != sCcbVector.end());
        sCcbVector.erase(i3);
        delete (ccb);
    }

    ci2=sPbeRtReqContinuationMap.begin(); 
    while(ci2!=sPbeRtReqContinuationMap.end()) {
        //TODO the timeout should not be hardwired, but for now it is.
        if(now - ci2->second.mCreateTime >= DEFAULT_TIMEOUT_SEC) {
            TRACE_5("Timeout on PbeRtReqContinuation %llu", ci2->first);
            pbePrtoReqs.push_back(ci2->second.mConn);
            sPbeRtReqContinuationMap.erase(ci2);
            ci2=sPbeRtReqContinuationMap.begin();
        } else {
            ++ci2;
        }
    }

    /* Check for progress on PbeRtMutations. */
    if(sPbeRtMutations.empty()) {
        if(sPbeRtMinContId) {
            TRACE_5("Progress: sPbeMinRtContId reset from %u to zero", sPbeRtMinContId);
            sPbeRtMinContId = 0;
        }

        if(sPbeRtBacklog) {
            TRACE_5("Progress: sPbeBacklog reset from %u to zero", sPbeRtBacklog);
            sPbeRtBacklog = 0;
        }
    } else {
        /* Check for blockage. */
        SaUint32T pbeMinContinuationId=0xffffffff;
        for(ObjectMutationMap::iterator omuti=sPbeRtMutations.begin(); 
            omuti!=sPbeRtMutations.end(); ++omuti) {
            if(omuti->second->mContinuationId < pbeMinContinuationId) {
                pbeMinContinuationId = omuti->second->mContinuationId;
            }
        }

        assert(pbeMinContinuationId);
        assert(pbeMinContinuationId < 0xffffffff);
        if(sPbeRtMinContId == pbeMinContinuationId) {
            pbeRtRegress = 2; /* No progress on oldest continuation. */
        } else {
            TRACE_5("Progress: sPbeMinRtContId raised from %u to %u", sPbeRtMinContId,
                pbeMinContinuationId);
            sPbeRtMinContId = pbeMinContinuationId;

            /* We are making progress but is backlog increasing? */
            SaUint32T newBacklog = (SaUint32T) sPbeRtMutations.size();
            if(newBacklog > sPbeRtBacklog) {
                /* Backlog increased. */
                TRACE_5("Backlog increased from %u to %u", sPbeRtBacklog, newBacklog);
                sPbeRtBacklog = newBacklog;
                pbeRtRegress = 2;
            }
        }
    }


    if(pbeRtRegress) {
        if(++sPbeRegressPeriods < 15) {
            /* Have some patience. */
            pbeRtRegress = 0;
            TRACE_5("sPbeRegressPeriods:%u", sPbeRegressPeriods);
        } 
    } else {
        if(sPbeRegressPeriods) {
            TRACE_5("Progress: sPbeRegressPeriods reset from %u to zero", 
                sPbeRegressPeriods);
            sPbeRegressPeriods = 0;
        }
    }

    return ccbsStuck + pbeRtRegress;
}


/** 
 * Creates an implementer name and registers an implementer.
 */
SaAisErrorT
ImmModel::implementerSet(const IMMSV_OCTET_STRING* implementerName,
    SaUint32T conn,
    SaUint32T nodeId,
    SaUint32T implementerId,
    SaUint64T mds_dest)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    //Check first if implementer name already exists.
    //If so check if occupied, if not re-use.
    size_t sz = strnlen((char *) implementerName->buf,
        (size_t)implementerName->size);
    std::string implName((const char*)implementerName->buf, sz);
    
    if(!nameCheck(implName)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper implementer name");
        err = SA_AIS_ERR_INVALID_PARAM; 
        return err;
        //This return code not formally allowed here according to IMM standard.
    }
    
    ImplementerInfo* info = findImplementer(implName);
    if(info) {
        if(info->mId) { 
            TRACE_7("ERR_EXIST: Registered implementer already exists: %s", 
                implName.c_str());
            err = SA_AIS_ERR_EXIST;
            TRACE_LEAVE();
            return err;
        }
        //Implies we re-use ImplementerInfo. The previous 
        //implementer-connection must have been cleared or crashed.
        assert(!info->mConn);
        assert(!info->mNodeId);
    } else {
        info = new ImplementerInfo;
        info->mImplementerName = implName;
        sImplementerVector.push_back(info);
    }
    
    if((sImplementerVector.size() > 300) && 
        (sImplementerVector.size()%30 == 0)) {
        LOG_WA("More than 300 distinct implementer names used:%u",
            (unsigned int) sImplementerVector.size());
    }
    
    info->mId = implementerId;
    info->mConn = conn;               //conn!=NULL only on primary.
    info->mNodeId = nodeId;
    info->mAdminOpBusy = 0;
    info->mMds_dest = mds_dest;
    info->mDying = false;
    
    
    LOG_IN("Create implementer:<name:%s, id:%u, conn:%u, node:%x dead:%u>",
        info->mImplementerName.c_str(), info->mId, info->mConn, info->mNodeId,
        info->mDying);

    if(implName == std::string(OPENSAF_IMM_PBE_IMPL_NAME)) {
        TRACE_7("Implementer %s for PBE created - "
            "check for ccbs stuck in critical", implName.c_str());
       /* If we find any, then surgically replace the implId of the implAssoc with
           the new implId of the newly reincarnated PBE implementer.*/

        CcbVector::iterator i;
        for(i=sCcbVector.begin(); i!=sCcbVector.end(); ++i) {
            CcbInfo* ccb = (*i);
            if(ccb->mState == IMM_CCB_CRITICAL) {
                CcbImplementerMap::iterator isi;
                for(isi = ccb->mImplementers.begin();
                    isi != ccb->mImplementers.end(); ++isi) {
                    ImplementerCcbAssociation* oldImplAssoc = isi->second;
                    if(oldImplAssoc->mWaitForImplAck && oldImplAssoc->mImplementer) {
                        ImplementerInfo* impInfo = oldImplAssoc->mImplementer;
                        if(impInfo->mImplementerName == implName) {
                            SaUint32T oldImplId = isi->first;
                            if(oldImplId == info->mId) { continue; }/*already replaced*/
                                ccb->mImplementers.erase(isi);
                                ccb->mImplementers[info->mId] = oldImplAssoc;
                                TRACE_7("Replaced implid %u with %u", oldImplId, info->mId);
                                ccb->mPbeRestartId = info->mId;
                                /* Can only be one PBE impl asoc*/
                                break;  /* out of for(isi = ccb->mImplementers....*/
                        }
                    }
                }
            }
        }
    }
    /*
      ImplementerVector::iterator i;
      for(i = sImplementerVector.begin(); i != sImplementerVector.end(); ++i) {
      info = (*i);
      TRACE_7("Implementer id:%u name:%s node:%x conn:%u mDying:%s "
      "mMds_dest:%llu AdminOpBusy:%u",
      info->mId,
      info->mImplementerName.c_str(),
      info->mNodeId,
      info->mConn,
      (info->mDying)?"TRUE":"FALSE",
      info->mMds_dest,
      info->mAdminOpBusy);
      }
    */
    TRACE_LEAVE();
    return err;
}

/** 
 * Registers a specific implementer for a class and all its instances
 */
SaAisErrorT
ImmModel::classImplementerSet(const struct ImmsvOiImplSetReq* req,
    SaUint32T conn,
    unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((const char *)req->impl_name.buf, req->impl_name.size);
    std::string className((const char *)req->impl_name.buf, sz);
    
    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ImplementerInfo* info = findImplementer(req->impl_id);
        if(!info) {
            LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->impl_id);
            err = SA_AIS_ERR_BAD_HANDLE;
        } else {
            if((info->mConn != conn) || (info->mNodeId != nodeId)) {
                LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle "
                    "conn:%u nodeId:%x", conn, nodeId);
                err = SA_AIS_ERR_BAD_HANDLE;
            } else {
                //conn is NULL on all nodes except primary.
                //At these other nodes the only info on implementer existence
                //is that the nodeId is non-zero. The nodeId is the nodeId of
                //the node where the implementer resides.
                
                ClassMap::iterator i1 = sClassMap.find(className);
                if (i1 == sClassMap.end()) {
                    TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", 
                        className.c_str());
                    err = SA_AIS_ERR_NOT_EXIST;
                } else {
                    ClassInfo* classInfo = i1->second;
                    if(classInfo->mCategory == SA_IMM_CLASS_RUNTIME) {
                        LOG_IN("ERR_BAD_OPERATION: Class '%s' is a runtime "
                            "class, not allowed to set class implementer.", 
                            className.c_str());
                        err = SA_AIS_ERR_BAD_OPERATION;
                    } else {
                        if(classInfo->mImplementer && 
                            (classInfo->mImplementer != info)) {
                            TRACE_7("ERR_EXIST: Class '%s' already has class implementer "
                                "%s != %s", className.c_str(), 
                                classInfo->mImplementer->
                                mImplementerName.c_str(),
                                info->mImplementerName.c_str());
                            err = SA_AIS_ERR_EXIST;
                        } else {
                            ObjectMap::iterator oi;
                            for(oi=sObjectMap.begin(); oi!=sObjectMap.end();
                                ++oi) {
                                ObjectInfo* obj = oi->second;
                                if(obj->mClassInfo == classInfo) {
                                    if(obj->mImplementer &&
                                        obj->mImplementer != info) {
                                        TRACE_7("ERR_EXIST: Object '%s' already has implementer "
                                            "%s != %s", oi->first.c_str(), 
                                            obj->mImplementer->mImplementerName.c_str(),
                                            info->mImplementerName.c_str());
                                        err = SA_AIS_ERR_EXIST;
                                    }
                                }
                            }

                            if(err == SA_AIS_OK) {
                                classInfo->mImplementer = info;
                                TRACE_5("implementer for class '%s' is %s",
                                    className.c_str(),
                                    info->mImplementerName.c_str());
                                //ABT090225
                                //classImplementerSet and objectImplementerSet
                                //are mutually exclusive. This has not been 
                                //explicitly clear in the IMM standard until 
                                //release A.03.01.
                                //This implementation is release A.02.01 and
                                //has up until now provided the semantics that
                                //classImplementerSet overrides objectImplementerSet.
                                //We realize that this change is not backwards 
                                //compatible, but (1) the risk for problems should
                                //be minimal (backwards incompatibility on an 
                                //error case), and (2) the old solution was not
                                //good because it removed the object-implementorship
                                //without notifying that object-impleemntor.
                                for(oi=sObjectMap.begin(); oi!=sObjectMap.end();
                                    ++oi) {
                                    ObjectInfo* obj = oi->second;
                                    if(obj->mClassInfo == classInfo) {
                                        obj->mImplementer = 
                                            classInfo->mImplementer;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    TRACE_LEAVE();
    return err;
}


/** 
 * Releases a specific implementer for a class and all its instances
 */
SaAisErrorT
ImmModel::classImplementerRelease(const struct ImmsvOiImplSetReq* req,
    SaUint32T conn,
    unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((const char *)req->impl_name.buf, req->impl_name.size);
    std::string className((const char *)req->impl_name.buf, sz);
    
    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ImplementerInfo* info = findImplementer(req->impl_id);
        if(!info) {
            LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->impl_id);
            err = SA_AIS_ERR_BAD_HANDLE;
        } else {
            if((info->mConn != conn) || (info->mNodeId != nodeId)) {
                LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle conn:%u nodeId:%x", 
                    conn, nodeId);
                err = SA_AIS_ERR_BAD_HANDLE;
            } else {
                //conn is NULL on all nodes except primary.
                //At these other nodes the only info on implementer existence
                //is that the nodeId is non-zero. The nodeId is the nodeId of
                //the node where the implementer resides.
                
                ClassMap::iterator i1 = sClassMap.find(className);
                if (i1 == sClassMap.end()) {
                    TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", className.c_str());
                    err = SA_AIS_ERR_NOT_EXIST;
                } else {
                    ClassInfo* classInfo = i1->second;
                    if(classInfo->mCategory == SA_IMM_CLASS_RUNTIME) {
                        LOG_IN("ERR_BAD_OPERATION: Class '%s' is a runtime class.", 
                            className.c_str());
                        err = SA_AIS_ERR_BAD_OPERATION;
                    } else {
                        if(!classInfo->mImplementer) {
                            TRACE_7("ERR_NOT_EXIST: Class '%s' has no implementer", 
                                className.c_str());
                            err = SA_AIS_ERR_NOT_EXIST;
                        } else if(classInfo->mImplementer != info) {
                            TRACE_7("ERR_NOT_EXIST: Class '%s' has implementer %s != %s",
                                className.c_str(), 
                                classInfo->mImplementer->
                                mImplementerName.c_str(),
                                info->mImplementerName.c_str());
                            err = SA_AIS_ERR_NOT_EXIST;
                        } else {
                            TRACE_5("implementer for class '%s' is released", 
                                className.c_str());
                            ObjectMap::iterator oi;
                            for(oi=sObjectMap.begin(); oi!=sObjectMap.end();
                                ++oi) {
                                //No need for two passes since class
                                //implementer always overrides object
                                //implementer.
                                //TODO ABT0811: Incorrect semanics, opposite in standard.
                                ObjectInfo* obj = oi->second;
                                if(obj->mClassInfo == classInfo) {
                                    if(obj->mImplementer == 
                                        classInfo->mImplementer) {
                                        //The above if-statement should
                                        //possibly be an assert. Since 
                                        //implementer always overrides object 
                                        //implementer, I dont see how an object
                                        //of the class *could* have an
                                        //implementer different from the class
                                        //implementer, as long as there *is* a
                                        //class implementer for the class. 
                                        if(obj->mCcbId) {
                                            CcbVector::iterator i1 = 
                                                std::find_if(sCcbVector.begin(), sCcbVector.end(),
                                                    CcbIdIs(obj->mCcbId));
                                            if(i1 != sCcbVector.end() && (*i1)->isActive()) {
                                                LOG_IN("ERR_BUSY: ccb %u is active on object %s",
                                                    obj->mCcbId, oi->first.c_str());
                                                TRACE_LEAVE();
                                                return SA_AIS_ERR_BUSY;
                                            }
                                        }
                                        obj->mImplementer = 0;
                                    }
                                }
                            }//for
                            classInfo->mImplementer = 0;
                        }
                    }
                }
            }
        }
    }
    
    TRACE_LEAVE();
    return err;
}

/** 
 * Registers a specific implementer for an object and possibly sub-objects
 */
SaAisErrorT ImmModel::objectImplementerSet(const struct ImmsvOiImplSetReq* req,
    SaUint32T conn,
    unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((const char *)req->impl_name.buf, req->impl_name.size);
    std::string objectName((const char *)req->impl_name.buf, sz);
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object DN");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ImplementerInfo* info = findImplementer(req->impl_id);
        if(!info) {
            LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->impl_id);
            err = SA_AIS_ERR_BAD_HANDLE;
        } else {
            if((info->mConn != conn) || (info->mNodeId != nodeId)) {
                LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle conn:%u nodeId:%x", 
                    conn, nodeId);
                err = SA_AIS_ERR_BAD_HANDLE;
            } else {
                //conn is NULL on all nodes except primary.
                //At these other nodes the only info on implementer existence 
                //is that the nodeId is non-zero. The nodeId is the nodeId of
                //the node where the implementer resides.
                
                //I need to run two passes if scope != ONE
                //This because if I fail on ONE object then I have to revert
                //the implementer on all previously set in this op.
                
                ObjectMap::iterator i1 = sObjectMap.find(objectName);
                if (i1 == sObjectMap.end()) {
                    TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
                    err = SA_AIS_ERR_NOT_EXIST;
                } else {
                    ObjectInfo* rootObj = i1->second;
                    for(int doIt=0; doIt<2 && err == SA_AIS_OK; ++doIt) {
                        //ObjectInfo* objectInfo = i1->second;
                        err = setImplementer(objectName, rootObj, info, doIt);
                        SaImmScopeT scope = (SaImmScopeT) req->scope;
                        if(err == SA_AIS_OK && scope != SA_IMM_ONE) {
                            // Find all sub objects to the root object
                            //Warning re-using iterator i1 inside this loop!!!
                            for (i1 = sObjectMap.begin(); 
                                 i1 != sObjectMap.end() && err == SA_AIS_OK; 
                                 i1++) {
                                std::string subObjName = i1->first;
                                if (subObjName.length() > objectName.length()){
                                    size_t pos = subObjName.length() - 
                                        objectName.length();
                                    if ((subObjName.rfind(objectName, pos) == 
                                            pos)&&(subObjName[pos-1] == ',')){
                                        if(scope==SA_IMM_SUBTREE || 
                                            checkSubLevel(subObjName, pos)){
                                            ObjectInfo* subObj = i1->second;
                                            err = setImplementer(subObjName, 
                                                subObj, info, doIt);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    TRACE_LEAVE();
    return err;
}


/** 
 * Releases a specific implementer for an object and possibly sub-objects
 */
SaAisErrorT ImmModel::objectImplementerRelease(
                                               const struct ImmsvOiImplSetReq* req,
                                               SaUint32T conn,
                                               unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    if(immNotWritable()) {
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((const char *)req->impl_name.buf, req->impl_name.size);
    std::string objectName((const char *)req->impl_name.buf, sz);
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object DN");
        err = SA_AIS_ERR_INVALID_PARAM;
    } else {
        ImplementerInfo* info = findImplementer(req->impl_id);
        if(!info) {
            LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->impl_id);
            err = SA_AIS_ERR_BAD_HANDLE;
        } else {
            if((info->mConn != conn) || (info->mNodeId != nodeId)) {
                LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle conn:%u nodeId:%x", 
                    conn, nodeId);
                err = SA_AIS_ERR_BAD_HANDLE;
            } else {
                //conn is NULL on all nodes except primary.
                //At these other nodes the only info on implementer existence 
                //is that the nodeId is non-zero. The nodeId is the nodeId of
                //the node where the implementer resides.
                
                //I need to run two passes if scope != ONE
                //This because if I fail on ONE object then I have to revert
                //the implementer on all previously set in this op.
                
                ObjectMap::iterator i1 = sObjectMap.find(objectName);
                if (i1 == sObjectMap.end()) {
                    TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
                    err = SA_AIS_ERR_NOT_EXIST;
                } else {
                    ObjectInfo* rootObj = i1->second;
                    for(int doIt=0; doIt<2 && err == SA_AIS_OK; ++doIt) {
                        //ObjectInfo* objectInfo = i1->second;
                        err = releaseImplementer(objectName, rootObj, info, 
                            doIt);
                        SaImmScopeT scope = (SaImmScopeT) req->scope;
                        if(err == SA_AIS_OK && scope != SA_IMM_ONE) {
                            // Find all sub objects to the root object
                            // Warning re-using iterator i1 inside this loop
                            for (i1 = sObjectMap.begin(); 
                                 i1 != sObjectMap.end() && err == SA_AIS_OK; 
                                 i1++) {
                                std::string subObjName = i1->first;
                                if (subObjName.length() > objectName.length()){
                                    size_t pos = subObjName.length() - 
                                        objectName.length();
                                    if ((subObjName.rfind(objectName, pos) == 
                                            pos)&&(subObjName[pos-1] == ',')){
                                        if(scope==SA_IMM_SUBTREE || 
                                            checkSubLevel(subObjName, pos)){
                                            ObjectInfo* subObj = i1->second;
                                            err =releaseImplementer(subObjName,
                                                subObj, info, doIt);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    TRACE_LEAVE();
    return err;
}

/**
 * Helper function to set object implementer.
 */
SaAisErrorT ImmModel::setImplementer(std::string objectName, 
    ObjectInfo* obj, 
    ImplementerInfo* info,
    bool doIt)
{
    SaAisErrorT err = SA_AIS_OK;
    /*TRACE_ENTER();*/
    ClassInfo* classInfo = obj->mClassInfo;
    if(classInfo->mCategory == SA_IMM_CLASS_RUNTIME) {
        assert(!doIt);
        LOG_IN("ERR_BAD_OPERATION: Object '%s' is a runtime object, "
            "not allowed to set object implementer.", 
            objectName.c_str());
        err = SA_AIS_ERR_BAD_OPERATION;
    } else {
        if(obj->mImplementer && 
            (obj->mImplementer != info)) {
            assert(!doIt);
            TRACE_7("ERR_EXIST: Object '%s' already has implementer %s != %s",
                objectName.c_str(), 
                obj->mImplementer->mImplementerName.c_str(),
                info->mImplementerName.c_str());
            err = SA_AIS_ERR_EXIST;
        } else if(doIt) {
            obj->mImplementer = info;
            TRACE_5("implementer for object '%s' is %s", objectName.c_str(),
                info->mImplementerName.c_str());
            //Class implementer always overrides object implementer,
            //according to spec.
        }
    }
    /*TRACE_LEAVE();*/
    return err;
}

/**
 * Helper function to release object implementer.
 */
SaAisErrorT ImmModel::releaseImplementer(std::string objectName, 
    ObjectInfo* obj, 
    ImplementerInfo* info, 
    bool doIt)
{
    SaAisErrorT err = SA_AIS_OK;
    /*TRACE_ENTER();*/
    ClassInfo* classInfo = obj->mClassInfo;
    if(classInfo->mCategory == SA_IMM_CLASS_RUNTIME) {
        assert(!doIt);
        LOG_IN("ERR_BAD_OPERATION: Object '%s' is a runtime object, "
            "not allowed to release object implementer.", 
            objectName.c_str());
        err = SA_AIS_ERR_BAD_OPERATION;
    } else {
        if(obj->mImplementer && 
            (obj->mImplementer != info)) {
            assert(!doIt);
            TRACE_7("ERR_NOT_EXIST: Object '%s' has different implementer %s != %s",
                objectName.c_str(), 
                obj->mImplementer->mImplementerName.c_str(),
                info->mImplementerName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
        } else {
            if(obj->mCcbId) {
                CcbVector::iterator i1 = std::find_if(sCcbVector.begin(),
                    sCcbVector.end(), CcbIdIs(obj->mCcbId));
                if(i1 != sCcbVector.end() && (*i1)->isActive()) {
                    assert(!doIt);
                    LOG_IN("ERR_BUSY: ccb %u is active on object %s",
                        obj->mCcbId, objectName.c_str());
                    return SA_AIS_ERR_BUSY;
                }
            }

            if(doIt) {
                obj->mImplementer = 0;
                TRACE_5("implementer for object '%s' is released", 
                    objectName.c_str());
            }
        }
    }
    /*TRACE_LEAVE();*/
    return err;
}


/**
 * Helper function to set admin owner.
 */
SaAisErrorT 
ImmModel::adminOwnerSet(std::string objectName, ObjectInfo* obj, 
    AdminOwnerInfo* adm, bool doIt)
{
    /*TRACE_ENTER();*/
    SaAisErrorT err = SA_AIS_OK;
    std::string oldOwner;
    assert(adm);
    assert(obj);
    obj->getAdminOwnerName(&oldOwner);
    
    //TODO: "IMMLOADER" should be environment variable.
    std::string loader("IMMLOADER");  
    
    //No need to check for interference with Ccb
    //If the object points to a CCb and the state of that ccb is
    //active, then adminowner must be set already for that object.
    
    if(oldOwner.empty() || oldOwner == loader) {
        if(doIt) {
            obj->mAdminOwnerAttrVal->setValueC_str(adm->
                mAdminOwnerName.c_str());
            if(adm->mReleaseOnFinalize) {
                adm->mTouchedObjects.insert(obj);
            }
        }
    } else if(oldOwner != adm->mAdminOwnerName) {
        TRACE_7("ERR_EXIST: Object '%s' has different administative owner %s != %s",
            objectName.c_str(), 
            oldOwner.c_str(), 
            adm->mAdminOwnerName.c_str());
        err = SA_AIS_ERR_EXIST;
    }
    
    /*TRACE_LEAVE();*/
    return err;
}

/**
 * Helper function to release admin owner.
 */
SaAisErrorT ImmModel::adminOwnerRelease(std::string objectName, 
    ObjectInfo* obj, AdminOwnerInfo* adm, bool doIt)
{
    //If adm is NULL then this is assumed to be a clear and not just release.
    /*TRACE_ENTER();*/
    SaAisErrorT err = SA_AIS_OK;
    std::string oldOwner;
    assert(obj);
    obj->getAdminOwnerName(&oldOwner);
    
    std::string loader("IMMLOADER");
    
    //TODO: Need to check for interference with Ccb!!
    //If the object points to a CCb and the state of that ccb is
    //active, then ERROR.
    
    if(oldOwner.empty()) {
        //Actually empty => ERROR acording to spec
        //But we assume that clear ignores this and that 
        //release on finalize ignores this 
        //(problem assumed to be caused by a previous clear).
        if(adm) {
            //Not a clear=> error
            if(adm->mReleaseOnFinalize) {
                adm->mTouchedObjects.erase(obj);
            } 
            err = SA_AIS_ERR_NOT_EXIST;
        }
    } else {    
        if(!adm || oldOwner == adm->mAdminOwnerName || oldOwner == loader) {
            //This is the normal explicit IMM API use case (clear or release)
            //where there is an old-owner that is actually to be removed.
            if(doIt) {
                //We may be pulling the rug out from under another admin owner
                obj->mAdminOwnerAttrVal->setValueC_str(NULL);
                if(adm && adm->mReleaseOnFinalize) {
                    adm->mTouchedObjects.erase(obj);
                }
            }
        } else {
            //This is the internal use case (release on finalize)
            //Or the error case when a release conflicts with a
            //different admin owner (probably after clear + set)
            //Empty name => internal-use, ignore
            if(objectName.empty()) {
                //internal use case
                TRACE_7("Release on finalize, an object has changed "
                    "administative owner %s != %s", oldOwner.c_str(), 
                    adm->mAdminOwnerName.c_str());
            } else {
                //Error case
                TRACE_7("ERR_NOT_EXIST: Object '%s' has different administative "
                    "owner %s != %s", objectName.c_str(), 
                    oldOwner.c_str(), adm->mAdminOwnerName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
            }
            
            //For both error case and internal use, remove from touchedObjects
            if(adm && adm->mReleaseOnFinalize) {
                adm->mTouchedObjects.erase(obj);
            }
        }
    }
    /*TRACE_LEAVE();*/
    return err;
}

/** 
 * Clears the implementer name associated with the handle
 */
SaAisErrorT
ImmModel::implementerClear(const struct ImmsvOiImplSetReq* req,
    SaUint32T conn,
    unsigned int nodeId)
{
    SaAisErrorT err = SA_AIS_OK;
    TRACE_ENTER();
    
    //TODO This is a mutating op.
    //Potential interference problem with sync!
    
    ImplementerInfo* info = findImplementer(req->impl_id);
    if(!info) {
        if(sImmNodeState == IMM_NODE_W_AVAILABLE) {
            /* Sync is ongoing and we are a sync client.
               Remember the death of the implementer. 
            */
            discardImplementer(req->impl_id, true);
            goto done;
        }
        LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle? %llu id:%u", 
            req->client_hdl,
            req->impl_id);
        err = SA_AIS_ERR_BAD_HANDLE;
    } else {
        if((info->mConn != conn) || (info->mNodeId != nodeId)) {
            LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle conn:%u nodeId:%x", 
                conn, nodeId);
            err = SA_AIS_ERR_BAD_HANDLE;
        } else {
            discardImplementer(req->impl_id, true);
        }
    }
    
 done:
    TRACE_LEAVE();
    return err;
}


/** 
 * Creates a runtime object
 */
SaAisErrorT
ImmModel::rtObjectCreate(const struct ImmsvOmCcbObjectCreate* req,
    SaUint32T reqConn,
    unsigned int nodeId,
    SaUint32T* continuationId,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER2("cont:%p connp:%p nodep:%p", continuationId, pbeConnPtr, pbeNodeIdPtr);
    SaAisErrorT err = SA_AIS_OK;
    
    if(immNotWritable()) { /*Check for persistent RTO further down. */
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((char *) req->className.buf,  
        (size_t)req->className.size);
    std::string className((const char*)req->className.buf, sz);
    
    sz = strnlen((char *) req->parentName.buf, 
        (size_t)req->parentName.size);
    std::string parentName((const char*)req->parentName.buf, sz);
    
    TRACE_2("parentName:%s\n", parentName.c_str());
    std::string objectName;
    
    ClassInfo* classInfo = NULL;
    ObjectInfo* parent = NULL;
    ObjectInfo* object = NULL;
    
    ClassMap::iterator i3;
    AttrMap::iterator i4;
    ObjectMap::iterator i5;
    ImmAttrValueMap::iterator i6;
    
    immsv_attr_values_list* attrValues = NULL;
    bool isPersistent = false;
    bool nameCorrected = false;
    
    /*Should rename member adminOwnerId. Used to store implid here.*/
    ImplementerInfo* info = findImplementer(req->adminOwnerId);
    if(!info) {
        LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto rtObjectCreateExit;
    } 
    
    if((info->mConn != reqConn) || (info->mNodeId != nodeId)) {
        LOG_IN("ERR_BAD_HANDLE: Not a correct implementer handle conn:%u nodeId:%x", 
            reqConn, nodeId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto rtObjectCreateExit;
    }
    //conn is NULL on all nodes except primary.
    //At these other nodes the only info on implementer existence is that
    //the nodeId is non-zero. The nodeId is the nodeId of the node where
    //the implementer resides.
    
    //We rely on FEVS to guarantee that the same handleId is produced at
    //all nodes for the same implementer. 
    
    if(!nameCheck(parentName)) {
        if(nameToInternal(parentName)) {
            nameCorrected = true;
        } else {
            LOG_NO("ERR_INVALID_PARAM: Not a proper parent name:%s size:%u",
                parentName.c_str(), (unsigned int) parentName.size());
            err = SA_AIS_ERR_INVALID_PARAM;
            goto rtObjectCreateExit;
        }
    }
    
    if(!schemaNameCheck(className)) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper class name:%s", className.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
        goto rtObjectCreateExit;
    }
    
    i3 = sClassMap.find(className);
    if (i3 == sClassMap.end()) {
        TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", className.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto rtObjectCreateExit;
    }
    classInfo = i3->second;
    
    if(classInfo->mCategory != SA_IMM_CLASS_RUNTIME) {
        LOG_NO("ERR_INVALID_PARAM: class '%s' is not a runtime class", className.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;
        goto rtObjectCreateExit;
    }
    
    if (parentName.length() > 0) {
        ObjectMap::iterator i = sObjectMap.find(parentName);
        if (i == sObjectMap.end()) {
            TRACE_7("ERR_NOT_EXIST: parent object '%s' does not exist", parentName.c_str());
            err = SA_AIS_ERR_NOT_EXIST;
            goto rtObjectCreateExit;
        } else {
            parent = i->second;
            if(parent->mObjFlags & IMM_CREATE_LOCK) {
                TRACE_7("ERR_NOT_EXIST: parent object '%s' is being created "
                    "in a ccb %u or PRTO PBE that has not yet been applied.", 
                    parentName.c_str(), parent->mCcbId);
                err = SA_AIS_ERR_NOT_EXIST;
                goto rtObjectCreateExit;
            }
            
            if(parent->mObjFlags & IMM_DELETE_LOCK) {
                TRACE_7("ERR_NOT_EXIST: parent object '%s' is registered for delete in ccb: "
                    "%u. Will not allow create of subobject.",
                    parentName.c_str(), parent->mCcbId);
                err = SA_AIS_ERR_NOT_EXIST; /* object is not deleted yet */
                goto rtObjectCreateExit;
            }
            
        }
    }
    
    i4 = std::find_if(classInfo->mAttrMap.begin(), classInfo->mAttrMap.end(),
        AttrFlagIncludes(SA_IMM_ATTR_RDN));
    if (i4 == classInfo->mAttrMap.end()) {
        LOG_ER("No RDN attribute found in class!");
        assert(0); 
    }
    
    attrValues = req->attrValues;
    
    while(attrValues) {
        sz = strnlen((char *) attrValues->n.attrName.buf, 
            (size_t)attrValues->n.attrName.size);
        std::string attrName((const char*)attrValues->n.attrName.buf, sz);
        
        if (attrName == i4->first) { //Match on name for RDN attribute
            /* Sloppy type check, supplied value should EQUAL type in class */
            if((attrValues->n.attrValueType != SA_IMM_ATTR_SANAMET) &&
                (attrValues->n.attrValueType != SA_IMM_ATTR_SASTRINGT)) {
                LOG_NO("ERR_INVALID_PARAM:  value type for RDN attribute is "
                    "neither SaNameT nor SaStringT");
                err = SA_AIS_ERR_INVALID_PARAM;
                goto rtObjectCreateExit;
            }
            
            if(((size_t)attrValues->n.attrValue.val.x.size > 64) &&
                (attrValues->n.attrValueType == SA_IMM_ATTR_SASTRINGT))
            {
                LOG_NO("ERR_INVALID_PARAM: RDN attribute is too large: %u. Max length is 64 "
                    "for SaStringT", attrValues->n.attrValue.val.x.size);
                err = SA_AIS_ERR_INVALID_PARAM;     
                goto rtObjectCreateExit;
            }

            if(i4->second->mFlags & SA_IMM_ATTR_PERSISTENT) {
                isPersistent = true;

                if(immNotPbeWritable()) {
                    err = SA_AIS_ERR_TRY_AGAIN;
                    TRACE_7("TRY_AGAIN: Can not create persistent RTO when PBE is unavailable");
                    goto rtObjectCreateExit;
                }
            }
            
            if(parent && 
                (parent->mClassInfo->mCategory == SA_IMM_CLASS_RUNTIME) &&
                (i4->second->mFlags & SA_IMM_ATTR_PERSISTENT)) {
                /*Parent is a runtime obj & new obj is persistent => verify that
                  parent is persistent. */
                AttrMap::iterator i44 =
                    std::find_if(parent->mClassInfo->mAttrMap.begin(), 
                        parent->mClassInfo->mAttrMap.end(),
                        AttrFlagIncludes(SA_IMM_ATTR_RDN));
                
                assert(i44 != parent->mClassInfo->mAttrMap.end());
                
                if(!(i44->second->mFlags & SA_IMM_ATTR_PERSISTENT)) {
                    LOG_NO("ERR_INVALID_PARAM: Parent object '%s' is a "
                        "non-persistent runtime object. Will not allow "
                        "create of persistent sub-object.",parentName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    goto rtObjectCreateExit;
                }
            }

            objectName.append((const char*)attrValues->n.attrValue.val.x.buf, 
                strnlen((const char*)attrValues->n.attrValue.val.x.buf,
                    (size_t)attrValues->n.attrValue.val.x.size));
            break; //out of loop
        }
        attrValues = attrValues->next;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: No RDN attribute found or value is empty");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto rtObjectCreateExit;
    }
    
    if(!nameCheck(objectName)) {
        if(nameToInternal(objectName)) {
            nameCorrected = true;
        } else {
            LOG_NO("ERR_INVALID_PARAM: Not a proper RDN (%s, %u)", objectName.c_str(),
                (unsigned int) objectName.length());
            err = SA_AIS_ERR_INVALID_PARAM;
            goto rtObjectCreateExit;
        }
    }

    if(objectName.find(',') != std::string::npos) {
        LOG_NO("ERR_INVALID_PARAM: Can not tolerate ',' in RDN: '%s'", 
            objectName.c_str());
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto rtObjectCreateExit;
    }

    if (parent) {
        objectName.append(",");
        objectName.append(parentName);
    }
    
    if (objectName.size() >= SA_MAX_NAME_LENGTH) {
        TRACE_7("ERR_NAME_TOO_LONG: DN is too long, size:%u, max size is:%u", 
            (unsigned int) objectName.size(), SA_MAX_NAME_LENGTH);
        err = SA_AIS_ERR_NAME_TOO_LONG;     
        goto rtObjectCreateExit;
    }
    
    if((i5 = sObjectMap.find(objectName)) != sObjectMap.end()) {
        if(i5->second->mObjFlags & IMM_CREATE_LOCK) {
            TRACE_7("ERR_EXIST: object '%s' is already registered "
                "for creation in a ccb or PRTO PBE, but not applied yet", 
                objectName.c_str());
        } else {
            TRACE_7("ERR_EXIST: Object '%s' already exists", objectName.c_str());
        }
        err = SA_AIS_ERR_EXIST;
        goto rtObjectCreateExit;
    }
    
    if(err == SA_AIS_OK) {
        void* pbe = NULL;
        object = new ObjectInfo();
        object->mCcbId = 0;
        object->mClassInfo = classInfo;
        object->mImplementer = info;
        if(nameCorrected) {
            object->mObjFlags = IMM_DN_INTERNAL_REP;
        } else {
            object->mObjFlags = 0;
        }
        
        // Add attributes to object
        for (i4 = classInfo->mAttrMap.begin(); 
             i4 != classInfo->mAttrMap.end();
             i4++) {
            
            AttrInfo* attr = i4->second;
            ImmAttrValue* attrValue = NULL;
            
            if(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE) {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrMultiValue();
                } else {
                    attrValue = new ImmAttrMultiValue(attr->mDefaultValue);
                }
            } else {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrValue();
                } else {
                    attrValue = new ImmAttrValue(attr->mDefaultValue);
                }
            }
            
            //Set admin owner as a regular attribute and then also a pointer
            //to the attrValue for efficient access.
            if(i4->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                //attrValue->setValueC_str(adminOwner->mAdminOwnerName.c_str());
                object->mAdminOwnerAttrVal = attrValue;
            }
            
            object->mAttrValueMap[i4->first] = attrValue;
        }
        
        // Set attribute values
        immsv_attr_values_list* p = req->attrValues;
        while(p) {
            sz = strnlen((char *) p->n.attrName.buf, 
                (size_t)p->n.attrName.size);
            std::string attrName((const char*)p->n.attrName.buf, sz);
            
            i6 = object->mAttrValueMap.find(attrName);
            
            if (i6 == object->mAttrValueMap.end()) {
                TRACE_7("ERR_NOT_EXIST: attr '%s' not defined", attrName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
                break; //out of for-loop
            }
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if(attr->mValueType != (unsigned int) p->n.attrValueType) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' type missmatch", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            }
            
            if(attr->mFlags & SA_IMM_ATTR_CONFIG) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is a config attribute => "
                    "can not be assigned over OI-API.", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            } else if(!(attr->mFlags & SA_IMM_ATTR_CACHED)) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is a non-cached runtime attribute => "
                    "can not be assigned in rtObjectCreate (see spec).", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            }
            
            ImmAttrValue* attrValue = i6->second;
            IMMSV_OCTET_STRING tmpos; //temporary octet string
            eduAtValToOs(&tmpos, &(p->n.attrValue), 
                (SaImmValueTypeT) p->n.attrValueType);
            attrValue->setValue(tmpos);
            if(p->n.attrValuesNumber > 1) {
                /*
                  int extraValues = p->attrValuesNumber - 1;
                */
                if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                    LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued yet "
                        "multiple values provided in create call", 
                        attrName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    break; //out of for loop
                } 
                /*
                  else if(extraValues != p->attrMoreValues.list.count) {
                  LOG_ER("Multivalued attr '%s' should have "
                  "%u extra values but %u extra values tranmitted",
                  attrName.c_str(), extraValues, 
                  p->attrMoreValues.list.count);
                  //Should not happen, error in packing.
                  err = SA_AIS_ERR_LIBRARY;
                  break; //out of for-loop
                  } */
                else {
                    assert(attrValue->isMultiValued());
                    if(className != immClassName) {//Avoid restoring imm object
                        ImmAttrMultiValue* multiattr = 
                            (ImmAttrMultiValue *) attrValue;
                        
                        IMMSV_EDU_ATTR_VAL_LIST* al= p->n.attrMoreValues;
                        while(al) {
                            eduAtValToOs(&tmpos, &(al->n), 
                                (SaImmValueTypeT) p->n.attrValueType);
                            multiattr->setExtraValue(tmpos);
                            al = al->next;
                        }
                        //TODO: Verify that attrValuesNumber was correct.
                    }
                }
            } //if(p->n.attrValuesNumber > 1)
            p = p->next;
        } //while(p)
        
        //Check that all attributes with CACHED flag have been set.
        
        for(i6=object->mAttrValueMap.begin(); 
            i6!=object->mAttrValueMap.end() && err==SA_AIS_OK;
            ++i6) {
            ImmAttrValue* attrValue = i6->second;
            std::string attrName(i6->first);
            
            if(attrName == std::string(SA_IMM_ATTR_IMPLEMENTER_NAME)) {
                continue;
            }
            
            if(attrName == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                continue;
            }
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if((attr->mFlags & SA_IMM_ATTR_CACHED) && attrValue->empty()) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is cached "
                    "yet no value provided in the object create call", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
            }
        }
        
        if(isPersistent && (err == SA_AIS_OK) && pbeNodeIdPtr) {
            /* PBE expected. */
            pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
            if(!pbe) {
                LOG_ER("Pbe is not available, can not happen here");
                abort();
            } 
        }

        if (err != SA_AIS_OK) {
            //Delete object and its attributes
            ImmAttrValueMap::iterator oavi;
            for(oavi = object->mAttrValueMap.begin();
                oavi != object->mAttrValueMap.end(); ++oavi) {
                delete oavi->second;  
            }
            
            //Empty the collection, probably not necessary (as the ObjectInfo
            //record is deleted below), but does not hurt.
            object->mAttrValueMap.clear(); 
            
            object->mImplementer=0;
            object->mClassInfo=0;
            object->mCcbId=0;
            delete object; 
            goto rtObjectCreateExit;
        }

        if(isPersistent) {
            if(pbe) { /* Persistent back end is there. */

                object->mObjFlags |= IMM_CREATE_LOCK;
                /* Dont overwrite IMM_DN_INTERNAL_REP*/

                *continuationId = ++sLastContinuationId;
                if(sLastContinuationId >= 0xfffffffe)
                {sLastContinuationId = 1;}

                ObjectMutation* oMut = new ObjectMutation(IMM_CREATE);
                oMut->mContinuationId = (*continuationId);
                oMut->mAfterImage = object;
                sPbeRtMutations[objectName] = oMut;
                if(reqConn) {
                    SaInvocationT tmp_hdl =
                        m_IMMSV_PACK_HANDLE((*continuationId), nodeId);
                    sPbeRtReqContinuationMap[tmp_hdl] = 
                        ContinuationInfo2(reqConn, DEFAULT_TIMEOUT_SEC);
                }
                TRACE_5("Tentative create of PERSISTENT runtime object '%s' "
                    "by Impl %s pending PBE ack", objectName.c_str(),
                         info->mImplementerName.c_str());
            } else { /* No pbe and no pbe expected. */
                       LOG_IN("Create of PERSISTENT runtime object '%s' by Impl %s",
                           objectName.c_str(), info->mImplementerName.c_str());
            }
        } else {/* !isPersistent i.e. normal RTO */
            TRACE_7("Create runtime object '%s' by Impl id: %u",
                objectName.c_str(), info->mId);
        }
        
        sObjectMap[objectName] = object;
        classInfo->mRefCount++;
        
        if(className == immClassName) {
            updateImmObject(immClassName);
        }
    }
 rtObjectCreateExit:
    TRACE_LEAVE(); 
    return err;
}

void ImmModel::pbePrtObjCreateContinuation(SaUint32T invocation,
    SaAisErrorT error, unsigned int nodeId, SaUint32T *reqConn)
{
    TRACE_ENTER();

    SaInvocationT inv = m_IMMSV_PACK_HANDLE(invocation, nodeId);
    ContinuationMap2::iterator ci = sPbeRtReqContinuationMap.find(inv);
    if(ci != sPbeRtReqContinuationMap.end()) {
        /* The client was local. */
        TRACE("CLIENT WAS LOCAL continuation found");
        *reqConn = ci->second.mConn;
        sPbeRtReqContinuationMap.erase(ci);
    } 

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ++i2) {
        if(i2->second->mContinuationId == invocation) {break;}
    }

    if(i2 == sPbeRtMutations.end()) {
        LOG_ER("PBE PRTO Create continuation missing! invoc:%u",
            invocation);
        return;
    }

    ObjectMutation* oMut = i2->second;

    assert(oMut->mOpType == IMM_CREATE);

    oMut->mAfterImage->mObjFlags &= ~IMM_CREATE_LOCK;
    oMut->mAfterImage = NULL;

    if(error == SA_AIS_OK) {
        LOG_IN("Create of PERSISTENT runtime object '%s'.", i2->first.c_str());
    } else {
        bool dummy=false;
        ObjectMap::iterator oi = sObjectMap.find(i2->first);
        assert(oi != sObjectMap.end());
        assert(deleteRtObject(oi, true, NULL, dummy) == SA_AIS_OK);
        LOG_WA("Create of PERSISTENT runtime object '%s' REVERTED. PBE rc:%u", 
            i2->first.c_str(), error);
    }

    sPbeRtMutations.erase(i2);
    delete oMut;
    TRACE_LEAVE();
}

void ImmModel::pbePrtObjDeletesContinuation(SaUint32T invocation,
    SaAisErrorT error, unsigned int nodeId, SaUint32T *reqConn)
{
    TRACE_ENTER();
    unsigned int nrofDeletes=0;
    SaInvocationT inv = m_IMMSV_PACK_HANDLE(invocation, nodeId);
    ContinuationMap2::iterator ci = sPbeRtReqContinuationMap.find(inv);
    if(ci != sPbeRtReqContinuationMap.end()) {
        /* The client was local. */
        TRACE("CLIENT WAS LOCAL continuation found");
        *reqConn = ci->second.mConn;
        sPbeRtReqContinuationMap.erase(ci);
    } 

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ) {
        if(i2->second->mContinuationId != invocation) {
            ++i2;
            continue;
        }

        ++nrofDeletes;
        ObjectMutation* oMut = i2->second;
        assert(oMut->mOpType == IMM_DELETE);

        oMut->mAfterImage->mObjFlags &= ~IMM_DELETE_LOCK;

        if(error == SA_AIS_OK) {
            if(oMut->mAfterImage->mObjFlags & IMM_PRTO_FLAG) {
                LOG_IN("Delete of PERSISTENT runtime object '%s'.", i2->first.c_str());
            } else {
                LOG_IN("Delete of runtime object '%s'.", i2->first.c_str());
            }
            bool dummy=false;
            ObjectMap::iterator oi = sObjectMap.find(i2->first);
            assert(oi != sObjectMap.end());
            assert(deleteRtObject(oi, true, NULL, dummy) == SA_AIS_OK);
        } else {
            if(oMut->mAfterImage->mObjFlags & IMM_PRTO_FLAG) {
                LOG_WA("Delete of PERSISTENT runtime object '%s' REVERTED. PBE rc:%u", 
                    i2->first.c_str(), error);
            } else {
                LOG_WA("Delete of cached runtime object '%s' REVERTED. PBE rc:%u", 
                    i2->first.c_str(), error);
            }
        }

        oMut->mAfterImage = NULL;
        delete oMut;
        sPbeRtMutations.erase(i2);
        i2 = sPbeRtMutations.begin();
    } //for

    if(nrofDeletes == 0) {
        LOG_ER("PBE PRTO Deletes continuation missing! invoc:%u",
            invocation);
    } else {
        TRACE("PBE PRTO Deleted %u RT Objects", nrofDeletes);    
    }
    TRACE_LEAVE();
}

void ImmModel::pbePrtAttrUpdateContinuation(SaUint32T invocation,
    SaAisErrorT error, unsigned int nodeId, SaUint32T *reqConn)
{
    TRACE_ENTER();
    SaInvocationT inv = m_IMMSV_PACK_HANDLE(invocation, nodeId);
    ContinuationMap2::iterator ci = sPbeRtReqContinuationMap.find(inv);
    if(ci != sPbeRtReqContinuationMap.end()) {
        /* The client was local. */
        TRACE("CLIENT WAS LOCAL continuation found");
        *reqConn = ci->second.mConn;
        sPbeRtReqContinuationMap.erase(ci);
    } 

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ++i2) {
        if(i2->second->mContinuationId == invocation) {break;}
    }

    if(i2 == sPbeRtMutations.end()) {
        LOG_ER("PBE PRTAttrs Update continuation missing! invoc:%u",
            invocation);
        return;
    }

    ObjectMutation* oMut = i2->second;
    std::string objName(i2->first);

    assert(oMut->mOpType == IMM_MODIFY);
    ObjectInfo *afim =  oMut->mAfterImage;
    assert(afim);
    oMut->mAfterImage = NULL;

    sPbeRtMutations.erase(i2);
    delete oMut; oMut=NULL;

    ObjectMap::iterator oi = sObjectMap.find(objName);
    assert(oi != sObjectMap.end());
    ObjectInfo* beforeImage = oi->second;
    beforeImage->mObjFlags &= ~IMM_RT_UPDATE_LOCK;

    ImmAttrValueMap::iterator oavi;

    if(error == SA_AIS_OK) {
        LOG_IN("Update of PERSISTENT runtime attributes in object '%s'.", 
            objName.c_str());

        /* Discard beforeimage attr values. */
        for(oavi =  beforeImage->mAttrValueMap.begin();
            oavi != beforeImage->mAttrValueMap.end(); ++oavi) {
            delete oavi->second;
        }
        beforeImage->mAttrValueMap.clear(); 

        for(oavi = afim->mAttrValueMap.begin(); oavi != afim->mAttrValueMap.end(); ++oavi) {
            beforeImage->mAttrValueMap[oavi->first] = oavi->second;
            if(oavi->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                beforeImage->mAdminOwnerAttrVal = oavi->second;
            }
        }
        afim->mAttrValueMap.clear();
        delete afim;
    } else {
        LOG_WA("update of PERSISTENT runtime attributes in object '%s' REVERTED. "
            "PBE rc:%u", objName.c_str(), error);

        /*Discard afterimage */
        for(oavi =  afim->mAttrValueMap.begin(); oavi != afim->mAttrValueMap.end(); ++oavi) {
            delete oavi->second;
        }
        //Empty the collection, probably not necessary (as the
        //ObjectInfo record is deleted below), but does not hurt.
        afim->mAttrValueMap.clear(); 
        afim->mAdminOwnerAttrVal=0;
        afim->mClassInfo=0;
        afim->mImplementer=0;
        delete afim;
    }

    TRACE_LEAVE();
}

void ImmModel::pbeClassCreateContinuation(SaUint32T invocation,
    unsigned int nodeId, SaUint32T *reqConn)
{
    TRACE_ENTER();

    SaInvocationT inv = m_IMMSV_PACK_HANDLE(invocation, nodeId);
    ContinuationMap2::iterator ci = sPbeRtReqContinuationMap.find(inv);
    if(ci != sPbeRtReqContinuationMap.end()) {
        /* The client was local. */
        TRACE("CLIENT WAS LOCAL continuation found");
        *reqConn = ci->second.mConn;
        sPbeRtReqContinuationMap.erase(ci);
    }

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ++i2) {
        if(i2->second->mContinuationId == invocation) {break;}
    }

    if(i2 == sPbeRtMutations.end()) {
        LOG_ER("PBE Class Create continuation missing! invoc:%u",
            invocation);
        return;
    }

    ObjectMutation* oMut = i2->second;

    assert(oMut->mOpType == IMM_CREATE_CLASS);

    LOG_IN("Create of class %s is PERSISTENT.", i2->first.c_str());

    sPbeRtMutations.erase(i2);
    delete oMut;
    TRACE_LEAVE();
}

void ImmModel::pbeClassDeleteContinuation(SaUint32T invocation,
    unsigned int nodeId, SaUint32T *reqConn)
{
    TRACE_ENTER();

    SaInvocationT inv = m_IMMSV_PACK_HANDLE(invocation, nodeId);
    ContinuationMap2::iterator ci = sPbeRtReqContinuationMap.find(inv);
    if(ci != sPbeRtReqContinuationMap.end()) {
        /* The client was local. */
        TRACE("CLIENT WAS LOCAL continuation found");
        *reqConn = ci->second.mConn;
        sPbeRtReqContinuationMap.erase(ci);
    }

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ++i2) {
        if(i2->second->mContinuationId == invocation) {break;}
    }

    if(i2 == sPbeRtMutations.end()) {
        LOG_ER("PBE Class Delete continuation missing! invoc:%u",
            invocation);
        return;
    }

    ObjectMutation* oMut = i2->second;

    assert(oMut->mOpType == IMM_DELETE_CLASS);

    LOG_IN("Delete of class %s is PERSISTENT.", i2->first.c_str());

    sPbeRtMutations.erase(i2);
    delete oMut;
    TRACE_LEAVE();
}

void ImmModel::pbeUpdateEpochContinuation(SaUint32T invocation,
    unsigned int nodeId)
{
    TRACE_ENTER();

    ObjectMutationMap::iterator i2;
    for(i2=sPbeRtMutations.begin(); i2!=sPbeRtMutations.end(); ++i2) {
        if(i2->second->mContinuationId == invocation) {break;}
    }

    if(i2 == sPbeRtMutations.end()) {
        LOG_WA("PBE update epoch continuation missing! invoc:%u",
            invocation);
        return;
    }

    ObjectMutation* oMut = i2->second;

    assert(oMut->mOpType == IMM_UPDATE_EPOCH);

    LOG_IN("Update of epoch is PERSISTENT.");

    sPbeRtMutations.erase(i2);
    delete oMut;
    TRACE_LEAVE();
}

bool ImmModel::pbeIsInSync()
{
    return sPbeRtMutations.empty();
}

/** 
 * Update a runtime attributes in an object (runtime or config)
 */
SaAisErrorT
ImmModel::rtObjectUpdate(const ImmsvOmCcbObjectModify* req,
    SaUint32T conn,
    unsigned int nodeId,
    bool* isPureLocal,
    SaUint32T* continuationIdPtr,
    SaUint32T* pbeConnPtr,
    unsigned int* pbeNodeIdPtr)
{
    TRACE_ENTER2("cont:%p connp:%p nodep:%p", continuationIdPtr, pbeConnPtr, pbeNodeIdPtr);
    SaAisErrorT err = SA_AIS_OK;
    
    //Even if Imm is not writable (sync is on-going) we must allow
    //updates of non-cached and non-persistent attributes.
    
    size_t sz = strnlen((char *) req->objectName.buf, 
        (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    ClassInfo* classInfo = 0;
    ObjectInfo* object = 0;
    bool isPersistent = false;
    AttrMap::iterator i4;
    ObjectMap::iterator oi;
    ImmAttrValueMap::iterator oavi;
    ImplementerInfo* info = NULL;
    bool wasLocal = *isPureLocal;
    if(wasLocal) {assert(conn);}  //Assert may be a bit too strong here
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto rtObjectUpdateExit;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN value");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto rtObjectUpdateExit;
    }
    
    oi = sObjectMap.find(objectName);
    if (oi == sObjectMap.end()) {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto rtObjectUpdateExit;
    } else if(oi->second->mObjFlags & IMM_CREATE_LOCK) {
        TRACE_7("ERR_NOT_EXIST: object '%s' registered for creation "
            "but not yet applied (ccb or PRTO PBE)", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto rtObjectUpdateExit;
    } else if(oi->second->mObjFlags & IMM_DELETE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: object '%s' registered for delete "
            "but not yet applied (PRTO PBE)", objectName.c_str());
        err = SA_AIS_ERR_TRY_AGAIN;
        goto rtObjectUpdateExit;
    } 

    /*Prevent abuse from wrong implementer.*/
    /*Should rename member adminOwnerId. Used to store implid here.*/
    info = findImplementer(req->adminOwnerId);
    if(!info) {
        LOG_NO("ERR_BAD_HANDLE: Not a correct implementer handle %u", req->adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto rtObjectUpdateExit;
    } 
    
    if((info->mConn != conn) || (info->mNodeId != nodeId)) {
        LOG_NO("ERR_BAD_OPERATION: The provided implementer handle %u does "
            "not correspond to the actual connection <%u, %x> != <%u, %x>",
            req->adminOwnerId, info->mConn, info->mNodeId, conn, nodeId);
        err = SA_AIS_ERR_BAD_OPERATION;
        goto rtObjectUpdateExit;
    }
    //conn is NULL on all nodes except primary.
    //At these other nodes the only info on implementer existence is that
    //the nodeId is non-zero. The nodeId is the nodeId of the node where
    //the implementer resides.
    
    object = oi->second;
    
    if(!object->mImplementer ||
        (object->mImplementer->mConn != conn) || 
        (object->mImplementer->mNodeId != nodeId)) {
        LOG_NO("ERR_BAD_OPERATION: Not a correct implementer handle or object "
            "not handled by the implementer conn:%u nodeId:%x "
            "object->mImplementer:%p", conn, nodeId, object->mImplementer);
        err = SA_AIS_ERR_BAD_OPERATION;
        goto rtObjectUpdateExit;
    }
    //conn is NULL on all nodes except primary.
    
    //We rely on FEVS to guarantee that the same handleId is produced at
    //all nodes for the same implementer. 
    
    classInfo = object->mClassInfo;
    assert(classInfo);
    
    //if(classInfo->mCategory != SA_IMM_CLASS_RUNTIME) {
    //LOG_WA("object '%s' is not a runtime object", objectName.c_str());
    // return SA_AIS_ERR_BAD_OPERATION;
    //}
    //Note: above check was incorrect because this call SHOULD be 
    //allowed to update config objects, but only runtime attributes..
    
    //Find each attribute to be modified.
    //Check that attribute is a runtime attribute.
    //If multivalue implied check that it is multivalued.
    //Assign the attribute as intended.
    
    TRACE_5("Update runtime attributes in object '%s'", objectName.c_str());
    
    for(int doIt=0; (doIt < 2) && (err == SA_AIS_OK); ++doIt) {
        TRACE_5("update rt attributes doit: %u", doIt);
        void* pbe = NULL;
        ImmAttrValueMap::iterator oavi;
        if(doIt && pbeNodeIdPtr && isPersistent && !wasLocal)
        {
            ObjectInfo* afim = 0;
            ImmAttrValueMap::iterator oavi;
            ObjectMutation* oMut = 0;

            TRACE("PRT ATTRs UPDATE case, defer updates of cached attrs, until ACK from PBE");
            /* 
               We expect a PBE and the list of cached attributes to update includes
               some PERSISTENT ATTRs, then dont update any cached RTattrs now,
               instead flag the object (can be a config object) and create a 
               an afterimage mutation to hold the updates. Postpone the updates
               until we get an ack from PBE. Note that even non persistent but
               cached rtattr updates included are not updated untile after the ack,
               because we may need to revert the entire operation. 
            */
            pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
            if(!pbe) {
                LOG_ER("ERR_TRY_AGAIN: Persistent back end is down - unexpected here");
                err = SA_AIS_ERR_TRY_AGAIN;
                goto rtObjectUpdateExit;
                /* We have already checked for PbeWritable with success inside
                   deleteRtObject. Why do we fail in obtaining the PBE data now??
                   We could have an assert here, but since we have not done any
                   mutations yet (doit just turned true), we can get away with a
                   TRY_AGAIN.
                */
            }
            *continuationIdPtr = ++sLastContinuationId;
            if(sLastContinuationId >= 0xfffffffe)
            {sLastContinuationId = 1;}
            TRACE("continuation generated: %u", *continuationIdPtr);

            if(conn) {
                SaInvocationT tmp_hdl = m_IMMSV_PACK_HANDLE((*continuationIdPtr), nodeId);
                sPbeRtReqContinuationMap[tmp_hdl] =
                    ContinuationInfo2(conn, DEFAULT_TIMEOUT_SEC);
            }

            TRACE_5("Tentative update of cached runtime attribute(s) for "
                "object '%s' by Impl %s pending PBE ack", oi->first.c_str(),
                info->mImplementerName.c_str());

            object->mObjFlags |= IMM_RT_UPDATE_LOCK;
            
            afim = new ObjectInfo();
            afim->mCcbId = 0;
            afim->mClassInfo = object->mClassInfo;
            afim->mImplementer = object->mImplementer;
            afim->mObjFlags = object->mObjFlags;
            // Copy attribute values from existing object version to afim
            for(oavi = object->mAttrValueMap.begin(); 
                oavi != object->mAttrValueMap.end();
                oavi++) {
                ImmAttrValue* oldValue = oavi->second;
                ImmAttrValue* newValue = NULL;

                if(oldValue->isMultiValued()) {
                    newValue = new ImmAttrMultiValue(*((ImmAttrMultiValue *) oldValue));
                } else {
                    newValue = new ImmAttrValue(*oldValue);
                }

                //Set admin owner as a regular attribute and then also a pointer
                //to the attrValue for efficient access.
                if(oavi->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                    afim->mAdminOwnerAttrVal = newValue;
                }
                afim->mAttrValueMap[oavi->first] = newValue;
            }

            oMut = new ObjectMutation(IMM_MODIFY);
            oMut->mAfterImage = afim;
            oMut->mContinuationId = (*continuationIdPtr);
            sPbeRtMutations[objectName] = oMut;

            object = afim; /* Rest of the code below updates the afim and not 
                              the master object.*/
        }

        immsv_attr_mods_list* p = req->attrMods;
        while(p && (err == SA_AIS_OK)) {
            sz = strnlen((char *) p->attrValue.attrName.buf,
                (size_t) p->attrValue.attrName.size);
            std::string attrName((const char *) p->attrValue.attrName.buf, sz);
            
            TRACE_5("update rt attribute '%s'", attrName.c_str());
            
            ImmAttrValueMap::iterator i5 = 
                object->mAttrValueMap.find(attrName);
            
            if (i5 == object->mAttrValueMap.end()) {
                TRACE_7("ERR_NOT_EXIST: attr '%s' not defined", attrName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
                assert(!doIt);
                break; //out of while-loop
            }
            
            ImmAttrValue* attrValue = i5->second;
            
            i4 = classInfo->mAttrMap.find(attrName);
            if(i4==classInfo->mAttrMap.end()) {
                LOG_ER("i4==classInfo->mAttrMap.end())");
                assert(i4!=classInfo->mAttrMap.end());
            }

            AttrInfo* attr = i4->second;
            
            if(attr->mValueType != (unsigned int) p->attrValue.attrValueType) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' type missmatch", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                assert(!doIt);
                break; //out of while-loop
            }
            
            if(attr->mFlags & SA_IMM_ATTR_CONFIG) {
                assert(!doIt);
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is a config attribute => "
                    "can not be modified over OI-API.", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of while-loop
            }
            
            if(attr->mFlags & SA_IMM_ATTR_RDN) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' is the RDN attribute => "
                    "can not be modified over OI-API.", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                assert(!doIt);
                break; //out of while-loop
            }
            
            if(attr->mFlags & SA_IMM_ATTR_CACHED || 
                attr->mFlags & SA_IMM_ATTR_PERSISTENT) {
                //A non-local runtime attribute
                TRACE_5("A non local runtime attribute '%s'", 
                    attrName.c_str());
                
                if(immNotWritable()) {
                    //Dont allow writes to cached or persistent rt-attrs 
                    //during sync
                    assert(!doIt);
                    err = SA_AIS_ERR_TRY_AGAIN;
                    TRACE_5("IMM not writable => Cant update cahced rtattrs");
                    break;//out of while-loop
                }

                if(!doIt && (object->mObjFlags & IMM_RT_UPDATE_LOCK)) {
                    err = SA_AIS_ERR_TRY_AGAIN;
                   /* Sorry but any ongoing PRTO update will interfere with additional
                       updates of cached RTOs because the PRTO update has to be reversible.
                    */
                    LOG_IN("ERR_TRY_AGAIN: Object '%s' already subject of a persistent runtime "
                        "attribute update", objectName.c_str());
                    break;//out of while-loop
                }

                if(attr->mFlags & SA_IMM_ATTR_PERSISTENT) {
                    isPersistent = true;
                    if(immNotPbeWritable()) {
                        err = SA_AIS_ERR_TRY_AGAIN;
                        TRACE_5("ERR_TRY_AGAIN: IMM not persistent writable => Cant update persistent RTO");
                        break;//out of while-loop
                    }

                    /* Check for ccb interference if it is a config object */
                    if(classInfo->mCategory == SA_IMM_CLASS_CONFIG &&
                        object->mCcbId) {
                        CcbVector::iterator ci = std::find_if(sCcbVector.begin(),
                            sCcbVector.end(), CcbIdIs(object->mCcbId));
                        if((ci != sCcbVector.end()) && ((*ci)->isActive())) {
                            assert(!doIt);
                            LOG_IN("ERR_TRY_AGAIN: Object %s is part of active ccb %u, can not "
                                "allow PRT attr updates", objectName.c_str(), object->mCcbId);
                            err = SA_AIS_ERR_TRY_AGAIN; 
                            break;//out of while-loop
                            /* ERR_BAD_OPERATION would be more appropriate, but standard does
                               not allow it.
                            */
                        }
                    }

                    if(doIt && !wasLocal) {
                        TRACE_5("Update of PERSISTENT runtime attr %s in object %s",
                            attrName.c_str(), objectName.c_str());
                    }
                }

                *isPureLocal = false;
                if(wasLocal) {
                    p = p->next;
                    continue;
                } //skip non-locals (cached), on first and local invocation.
                //will be set over fevs.
            } else {
                // A local runtime attribute.
                if(!wasLocal) {
                    p = p->next;
                    continue;
                } //skip pure locals when invocation was not local.
            }
            
            IMMSV_OCTET_STRING tmpos; //temporary octet string
            
            switch(p->attrModType) {
                case SA_IMM_ATTR_VALUES_REPLACE:
                    
                    TRACE_3("SA_IMM_ATTR_VALUES_REPLACE");
                    //Remove existing values, then fall through to the add case
                    if(doIt) {
                        attrValue->discardValues();
                    }
                    if(p->attrValue.attrValuesNumber == 0) {
                        p = p->next;
                        continue; //Ok to replace with nothing.
                    }
                    //else intentional fall-through
                    
                case SA_IMM_ATTR_VALUES_ADD:
                    TRACE_3("SA_IMM_ATTR_VALUES_ADD");
                    if(p->attrValue.attrValuesNumber == 0) {
                        assert(!doIt);
                        LOG_NO("ERR_INVALID_PARAM: Empty value used for adding to attribute %s",
                            attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break;//out of switch
                    }
                    
                    eduAtValToOs(&tmpos, &(p->attrValue.attrValue),
                        (SaImmValueTypeT) p->attrValue.attrValueType);
                    
                    if(p->attrValue.attrValueType == SA_IMM_ATTR_SASTRINGT) {
                        TRACE("Update attribute of type string:%s  %s",
                            tmpos.buf, p->attrValue.attrValue.val.x.buf);
                    }
                    
                    if(p->attrValue.attrValueType == SA_IMM_ATTR_SAUINT32T) {
                        TRACE("Update attribute of type uint:%u",
                            p->attrValue.attrValue.val.sauint32);
                    }
                    
                    if(p->attrModType == SA_IMM_ATTR_VALUES_REPLACE ||
                        attrValue->empty()) {
                        if(doIt) {
                            assert(attrValue->empty());
                            attrValue->setValue(tmpos);
                        }
                    } else {
                        if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                            assert(!doIt);
                            LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued, yet "
                                "multiple values attempted", attrName.c_str());
                            err = SA_AIS_ERR_INVALID_PARAM;
                            break; //out of switch
                        }
                        if(doIt) {
                            assert(attrValue->isMultiValued());
                            ImmAttrMultiValue* multiattr = 
                                (ImmAttrMultiValue *) attrValue;
                            multiattr->setExtraValue(tmpos);
                        }
                    }
                    
                    if(p->attrValue.attrValuesNumber > 1) {
                        /*
                          int extraValues = p->attrValue.attrValuesNumber - 1;
                        */
                        if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                            assert(!doIt);
                            LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued, yet "
                                "multiple values provided in modify call", 
                                attrName.c_str());
                            err = SA_AIS_ERR_INVALID_PARAM;
                            break; //out of switch
                        } else {
                            if(doIt) {
                                assert(attrValue->isMultiValued());
                                ImmAttrMultiValue* multiattr = 
                                    (ImmAttrMultiValue *) attrValue;
                                
                                IMMSV_EDU_ATTR_VAL_LIST* al = 
                                    p->attrValue.attrMoreValues;
                                while(al) {
                                    eduAtValToOs(&tmpos, &(al->n),
                                        (SaImmValueTypeT) 
                                        p->attrValue.attrValueType);
                                    multiattr->setExtraValue(tmpos);
                                    al = al->next;
                                }
                            }
                        }
                    }
                    break;
                    
                case SA_IMM_ATTR_VALUES_DELETE:
                    TRACE_3("SA_IMM_ATTR_VALUES_DELETE");
                    //Delete all values that match that provided by invoker.
                    if(p->attrValue.attrValuesNumber == 0) {
                        assert(!doIt);
                        LOG_NO("ERR_INVALID_PARAM: Empty value used for deleting from "
                            "attribute %s", attrName.c_str());
                        err = SA_AIS_ERR_INVALID_PARAM;
                        break; //out of switch
                    }
                    
                    if(!attrValue->empty()) {
                        if(doIt) {
                            eduAtValToOs(&tmpos, &(p->attrValue.attrValue),
                                (SaImmValueTypeT) p->attrValue.attrValueType);
                            
                            attrValue->removeValue(tmpos);
                        }
                        //Note: We allow the delete value to be multivalued
                        //even if the attribute is single valued. 
                        //The semantics is that we delete ALL matchings of ANY
                        //delete value.
                        if(p->attrValue.attrValuesNumber > 1) {
                            IMMSV_EDU_ATTR_VAL_LIST* al = 
                                p->attrValue.attrMoreValues;
                            if(doIt) {
                                while(al) {
                                    eduAtValToOs(&tmpos, &(al->n),
                                        (SaImmValueTypeT) 
                                        p->attrValue.attrValueType);
                                    attrValue->removeValue(tmpos);
                                    al = al->next;
                                }
                            }
                        }
                    }
                    break; //out of switch
                    
                default:
                    err = SA_AIS_ERR_INVALID_PARAM;
                    LOG_NO("ERR_INVALID_PARAM: Illegal value for SaImmAttrModificationTypeT: %u",
                        p->attrModType);
                    break;
            }
            
            if(err != SA_AIS_OK) {
                break; //out of while-loop
            }
            p = p->next;
        }//while(p)
        //err!=OK => breaks out of for loop
    }//for(int doIt...
 rtObjectUpdateExit:
    TRACE_5("isPureLocal: %u when leaving", *isPureLocal);
    TRACE_LEAVE();
    return err;
}

/** 
 * Delete a runtime object
 */
SaAisErrorT
ImmModel::rtObjectDelete(const ImmsvOmCcbObjectDelete* req,
        SaUint32T reqConn, 
        unsigned int nodeId,
        SaUint32T* continuationIdPtr,
        SaUint32T* pbeConnPtr,
        unsigned int* pbeNodeIdPtr,
        ObjectNameVector& objNameVector)
{
    bool subTreeHasPersistent = false;

    TRACE_ENTER2("cont:%p connp:%p nodep:%p", continuationIdPtr, pbeConnPtr,
        pbeNodeIdPtr);
    SaAisErrorT err = SA_AIS_OK;
    ImplementerInfo* info = NULL;
    if(pbeNodeIdPtr) {
        (*pbeNodeIdPtr) = 0;
        assert(continuationIdPtr);
        (*continuationIdPtr) = 0;
        assert(pbeConnPtr);
        (*pbeConnPtr) = 0;
    }

    if(immNotWritable()) { /*Check for persistent RTOs further down. */
        TRACE_LEAVE();
        return SA_AIS_ERR_TRY_AGAIN;
    }
    
    size_t sz = strnlen((char *) req->objectName.buf, 
        (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    TRACE_2("Delete runtime object '%s' and all subobjects\n", 
        objectName.c_str());
    
    ObjectMap::iterator oi, oi2;
    
    if(! (nameCheck(objectName)||nameToInternal(objectName)) ) {
        LOG_NO("ERR_INVALID_PARAM: Not a proper object name");
        err = SA_AIS_ERR_INVALID_PARAM;
        goto rtObjectDeleteExit;
    }
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN attribute value");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto rtObjectDeleteExit;
    }
    
    oi = sObjectMap.find(objectName);
    if (oi == sObjectMap.end()) {
        TRACE_7("ERR_NOT_EXIST: object '%s' does not exist", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto rtObjectDeleteExit;
    } else if(oi->second->mObjFlags & IMM_CREATE_LOCK) {
        TRACE_7("ERR_NOT_EXIST: object '%s' registered for creation "
            "but not yet applied PRTO PBE ?", objectName.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto rtObjectDeleteExit;
    } else if(oi->second->mObjFlags & IMM_DELETE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: object '%s' registered for deletion "
            "but not yet applied PRTO PBE ?", objectName.c_str());
        err = SA_AIS_ERR_TRY_AGAIN;
        goto rtObjectDeleteExit;
    } else if(oi->second->mObjFlags & IMM_RT_UPDATE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: Object '%s' already subject of a persistent runtime "
                "attribute update", objectName.c_str());
        err = SA_AIS_ERR_TRY_AGAIN;
        goto rtObjectDeleteExit;
    }
    
    /*Prevent abuse from wrong implementer.*/
    /*Should rename member adminOwnerId. Used to store implid here.*/
    info = findImplementer(req->adminOwnerId);
    if(!info) {
        LOG_NO("ERR_BAD_HANDLE: Not a correct implementer handle %u",
            req->adminOwnerId);
        err = SA_AIS_ERR_BAD_HANDLE;
        goto rtObjectDeleteExit;
    } 
    
    if((info->mConn != reqConn) || (info->mNodeId != nodeId)) {
        LOG_NO("ERR_BAD_OPERATION: The provided implementer handle %u does "
            "not correspond to the actual connection <%u, %x> != <%u, %x>",
            req->adminOwnerId, info->mConn, info->mNodeId, reqConn, nodeId);
        err = SA_AIS_ERR_BAD_OPERATION;
        goto rtObjectDeleteExit;
    }
    //reqConn is NULL on all nodes except primary.
    //At these other nodes the only info on implementer existence is that
    //the nodeId is non-zero. The nodeId is the nodeId of the node where
    //the implementer resides.
    
    for(int doIt=0; (doIt < 2) && (err == SA_AIS_OK); ++doIt) {
        void* pbe = NULL;
        if(doIt && pbeNodeIdPtr && subTreeHasPersistent) {
            TRACE("PRTO DELETE case, deferred deletes until ACK from PBE");
            /* We expect a PBE and the recursive RTO delete includes
               some PERSISETENT RTOs, then dont delete the RTOs now,
               instead flag each object with a DELETE_LOCK and postpone
               the actual deletes pending an ack from PBE. Note that
               even non persistent RTOs that happen to be included
               in the subtree to be deleted, will be flag'ed with
               DELETE_LOCK and wait for the PBE ack. The reason is that
               the *entire* subtree delete must be atomic. If the PBE
               has problems, the recursive delete may get reverted. This
               reversion has to include non persistent RTOs. 
            */
            pbe = getPbeOi(pbeConnPtr, pbeNodeIdPtr);
            if(!pbe) {
                LOG_ER("ERR_TRY_AGAIN: Persistent back end is down - unexpected here");
                err = SA_AIS_ERR_TRY_AGAIN;
                goto rtObjectDeleteExit;
                /* We have already checked for PbeWritable with success inside
                   deleteRtObject. Why do we fail in obtaining the PBE data now??
                   We could have an assert here, but since we have not done any
                   mutations yet (doit just turned true), we can get away with a
                   TRY_AGAIN.
                */
            }

            /* If the subtree to delete includes PRTOs then we use a continuationId
               as a common pseudo ccbId for all the RTOs in the subtree.
            */
            *continuationIdPtr = ++sLastContinuationId;
            if(sLastContinuationId >= 0xfffffffe)
            {sLastContinuationId = 1;}
            TRACE("continuation generated: %u", *continuationIdPtr);

            if(reqConn) {
                SaInvocationT tmp_hdl = m_IMMSV_PACK_HANDLE((*continuationIdPtr), nodeId);
                sPbeRtReqContinuationMap[tmp_hdl] =
                    ContinuationInfo2(reqConn, DEFAULT_TIMEOUT_SEC);
            }

            TRACE_5("Tentative delete of runtime object '%s' "
                "by Impl %s pending PBE ack", oi->first.c_str(),
                info->mImplementerName.c_str());

            oi->second->mObjFlags |= IMM_DELETE_LOCK;
            /* Dont overwrite IMM_DN_INTERNAL_REP */
            ObjectMutation* oMut = new ObjectMutation(IMM_DELETE);
            oMut->mContinuationId = (*continuationIdPtr);
            oMut->mAfterImage = oi->second;
            sPbeRtMutations[oi->first] = oMut;
           if(oi->second->mObjFlags & IMM_PRTO_FLAG) {
                TRACE("PRTO flag was set for root of subtree to delete");
                if(oi->second->mObjFlags & IMM_DN_INTERNAL_REP) {
                    std::string tmpName(oi->first);
                    nameToExternal(tmpName);
                    objNameVector.push_back(tmpName);
                } else {
                    objNameVector.push_back(oi->first);
                }
            }
        } else {
            err = deleteRtObject(oi, doIt, info, subTreeHasPersistent);
        }

        //Find all sub objects to the deleted object and delete them 
        for(oi2 = sObjectMap.begin(); 
            oi2 != sObjectMap.end() && err == SA_AIS_OK;) {
            bool deleted=false;
            std::string subObjName = oi2->first;
            if(subObjName.length() > objectName.length()) {
                size_t pos = subObjName.length() - objectName.length();
                if((subObjName.rfind(objectName, pos) == pos) &&
                    (subObjName[pos-1] == ',')) {
                    if(doIt && pbeNodeIdPtr && subTreeHasPersistent) {
                        TRACE_5("Tentative delete of runtime object '%s' "
                            "by Impl %s pending PBE ack", subObjName.c_str(),
                            info->mImplementerName.c_str());

                        oi2->second->mObjFlags |= IMM_DELETE_LOCK;
                        /* Dont overwrite IMM_DN_INTERNAL_REP */
                        ObjectMutation* oMut = new ObjectMutation(IMM_DELETE);
                        oMut->mContinuationId = (*continuationIdPtr);
                        oMut->mAfterImage = oi2->second;
                        sPbeRtMutations[subObjName] = oMut;
                        if(oi2->second->mObjFlags & IMM_PRTO_FLAG) {
                            TRACE("PRTO flag was set for subobj");
                            if(oi2->second->mObjFlags & IMM_DN_INTERNAL_REP) {
                                std::string tmpName(subObjName);
                                nameToExternal(tmpName);
                                objNameVector.push_back(tmpName);
                            } else {
                                objNameVector.push_back(subObjName);
                            }
                        }
                    } else {
                        /* Normal non persistent RTO delete case */
                        err = deleteRtObject(oi2, doIt, info, subTreeHasPersistent);
                        if(doIt && err == SA_AIS_OK) {
                            deleted = true;
                        }
                    }//else
                }//if
            }//if
            if(deleted) {
                //Restart iterator (really inefficient)
                oi2 = sObjectMap.begin(); 
            } else {
                oi2++;
            }
        }//for
    }
    
 rtObjectDeleteExit:
    TRACE_LEAVE();
    return err;
}


SaAisErrorT
ImmModel::deleteRtObject(ObjectMap::iterator& oi, bool doIt, 
    ImplementerInfo* info, bool& subTreeHasPersistent)
{
    TRACE_ENTER();
    /* If param info!=NULL then this is a normal RTO delete.
       If param info==NULL then this is an immsv internal delete.
       Either a revert of PRTO PBE create due to NACK from PBE.
       OR materialization of a delete duw to ACK from PBE.
       See: ImmModel::pbePrtObjCreateContinuation and
       ImmModl:pbePrtoObjDeletesContinuation.
     */
    ObjectInfo* object = oi->second;
    assert(object);
    ClassInfo* classInfo = object->mClassInfo;
    assert(classInfo);

    AttrMap::iterator i4;
    
    if(info && (!object->mImplementer || object->mImplementer != info)) {
        LOG_NO("ERR_BAD_OPERATION: Not a correct implementer handle "
            "or object %s is not handled by the provided implementer " 
            "named %s (!=%p <name:'%s' conn:%u, node:%x>)", 
            oi->first.c_str(), info->mImplementerName.c_str(),
            object->mImplementer, 
            (object->mImplementer)?
            (object->mImplementer->mImplementerName.c_str()):"NIL",
            (object->mImplementer)?(object->mImplementer->mConn):0,
            (object->mImplementer)?(object->mImplementer->mNodeId):0);
        return SA_AIS_ERR_BAD_OPERATION;
    }
    
    if(classInfo->mCategory != SA_IMM_CLASS_RUNTIME) {
        LOG_NO("ERR_BAD_OPERATION: object '%s' is not a runtime object",
            oi->first.c_str());
        return SA_AIS_ERR_BAD_OPERATION;
    }

    i4 = std::find_if(classInfo->mAttrMap.begin(), classInfo->mAttrMap.end(),
        AttrFlagIncludes(SA_IMM_ATTR_PERSISTENT));

    bool isPersistent = i4 != classInfo->mAttrMap.end();

    if(isPersistent) {
        object->mObjFlags |= IMM_PRTO_FLAG;
    }

    if(!info) {
        if(isPersistent) {
            TRACE_7("Exotic deleteRtObj for PERSISTENT rto, "
                "revert on create or ack on delete from PBE");
        } else {
            TRACE_7("Exotic deleteRtObj for regular rto, "
                "revert on create or ack on delete from PBE");
        }
    } else if(isPersistent && !doIt) {
        if(!subTreeHasPersistent && immNotPbeWritable()) {
            /* Clarification of if statement: We are about to set 
               subTreeHasPersistent to true. 
               Why the condition "!subTreeHasPersistent" ? 
               It is just to avoid repeated execution of immNotPbeWritable
               for every object in the subtree. We only need to check
               immNotPbeWritable once in this job, so we do it the first time
               when we are about to set subTreeHasPersistent,
            */
            TRACE_7("TRY_AGAIN: Can not delete persistent RTO when PBE is unavailable");
            return SA_AIS_ERR_TRY_AGAIN;
        }

        subTreeHasPersistent = true;
        TRACE("Detected persistent object %s in subtree to be deleted",
            oi->first.c_str());
    }

    if(object->mObjFlags & IMM_CREATE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: sub-object '%s' registered for creation "
            "but not yet applied by PRTO PBE ?", oi->first.c_str());
        /* Possibly we should return NOT_EXIST here, but the delete may
           be recursive over a tree where only some subobject is being 
           created. Returning NOT_EXIST could be very confusing here.
         */
        TRACE("ERR_TRY_AGAIN CREATE LOCK WAS SET");
        return SA_AIS_ERR_TRY_AGAIN; 
    }

    if(object->mObjFlags & IMM_DELETE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: sub-object '%s' already registered for delete "
            "but not yet applied by PRTO PBE ?", oi->first.c_str());
        /* Possibly we should return NOT_EXIST here, but the delete may
           be recursive over a tree where only some subobject is being 
           deleted. Returning NOT_EXIST could be very confusing here.
         */
        TRACE("ERR_TRY_AGAIN DELETE LOCK WAS SET");
        return SA_AIS_ERR_TRY_AGAIN; 
    }

    if(object->mObjFlags & IMM_RT_UPDATE_LOCK) {
        TRACE_7("ERR_TRY_AGAIN: Object '%s' already subject of a persistent runtime "
                "attribute update", oi->first.c_str());
        return SA_AIS_ERR_TRY_AGAIN;
    }

    if(doIt) {
        ImmAttrValueMap::iterator oavi;
        for(oavi = object->mAttrValueMap.begin();
            oavi != object->mAttrValueMap.end(); ++oavi) {
            delete oavi->second;
        }
        object->mAttrValueMap.clear();
        
        assert(classInfo->mRefCount > 0);
        classInfo->mRefCount--;
        object->mAdminOwnerAttrVal=0;
        object->mClassInfo=0;
        object->mImplementer = 0;
        object->mObjFlags = 0;

        if(isPersistent) {
            if(info) {
                LOG_IN("Delete of PERSISTENT runtime object '%s' by Impl: %s", 
                oi->first.c_str(), info->mImplementerName.c_str());
            } else {
                TRACE_7("REVERT of create OR ack from PBE on delete of PERSISTENT "
                    "runtime object '%s'", oi->first.c_str());
            }
        } else {
            TRACE_7("Delete runtime object '%s' by Impl-id: %u", 
                oi->first.c_str(), info?(info->mId):0);
        }

        AdminOwnerVector::iterator i2;
        
        //Delete rt-object from touched objects in admin-owners
        //TODO: This is not so efficient. 
        for(i2=sOwnerVector.begin(); i2!=sOwnerVector.end();++i2) {
            (*i2)->mTouchedObjects.erase(oi->second);
            //Note that on STL sets, the erase operation is polymorphic.
            //Here we are erasing based on value, not iterator position. 
        }
        
        delete object;
        sObjectMap.erase(oi);
    }
    
    return SA_AIS_OK;
}

SaAisErrorT
ImmModel::objectSync(const ImmsvOmObjectSync* req)
{
    TRACE_ENTER();
    SaAisErrorT err=SA_AIS_OK;
    immsv_attr_values_list* p = NULL;
    bool nameCorrected = false;
    
    switch(sImmNodeState){ 
        case IMM_NODE_W_AVAILABLE:
            break;
        case IMM_NODE_UNKNOWN:
        case IMM_NODE_ISOLATED: 
        case IMM_NODE_LOADING:
        case IMM_NODE_FULLY_AVAILABLE:
        case IMM_NODE_R_AVAILABLE:
            LOG_ER("Node is in a state %u that cannot accept"
                "sync message, will terminate", sImmNodeState);
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
    }
    
    size_t sz = strnlen((char *) req->className.buf, 
        (size_t)req->className.size);
    std::string className((const char*)req->className.buf, sz);
    
    sz = strnlen((char *) req->objectName.buf, (size_t)req->objectName.size);
    std::string objectName((const char*)req->objectName.buf, sz);
    
    TRACE_5("Syncing objectName:%s", objectName.c_str());
    
    ClassInfo* classInfo = 0;
    
    ClassMap::iterator i3 = sClassMap.find(className);
    AttrMap::iterator i4;
    ObjectMap::iterator i5;
    
    if(!nameCheck(objectName)) {
        if(nameToInternal(objectName)) {
            nameCorrected = true;
        } else {
            LOG_NO("ERR_INVALID_PARAM: Not a proper object name:%s size:%u", 
                objectName.c_str(), (unsigned int) objectName.size());
            err = SA_AIS_ERR_INVALID_PARAM;
            goto objectSyncExit;
        }
    }
    
    if (i3 == sClassMap.end()) {
        TRACE_7("ERR_NOT_EXIST: class '%s' does not exist", className.c_str());
        err = SA_AIS_ERR_NOT_EXIST;
        goto objectSyncExit;
    }
    
    classInfo = i3->second;
    
    if (objectName.empty()) {
        LOG_NO("ERR_INVALID_PARAM: Empty DN is not allowed");
        err = SA_AIS_ERR_INVALID_PARAM;     
        goto objectSyncExit;
    }
    
    if ((i5 = sObjectMap.find(objectName)) != sObjectMap.end()) {
        if(i5->second->mObjFlags & IMM_CREATE_LOCK) {
            LOG_ER("ERR_EXIST: synced object '%s' is already registered for "
            "creation in a ccb, but not applied yet", objectName.c_str());
        } else {
            LOG_ER("ERR_EXIST: synced object '%s' exists", objectName.c_str());
        }
        err = SA_AIS_ERR_EXIST;
        goto objectSyncExit;
    }
    
    if(err == SA_AIS_OK) {
        TRACE_5("sync inserting object '%s'", objectName.c_str());
        
        ObjectInfo* object = new ObjectInfo();
        object->mCcbId = 0;
        object->mClassInfo = classInfo;
        object->mImplementer = 0; //Implementer will be corrected later.
        if(nameCorrected) {
            object->mObjFlags = IMM_DN_INTERNAL_REP;
        } else {
            object->mObjFlags = 0;
        }
        
        // Add attributes to object
        for (i4 = classInfo->mAttrMap.begin(); i4 != classInfo->mAttrMap.end();
             i4++) {
            AttrInfo* attr = i4->second;
            
            ImmAttrValue* attrValue = NULL;
            if(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE) {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrMultiValue();
                } else {
                    attrValue = new ImmAttrMultiValue(attr->mDefaultValue);
                }
            } else {
                if(attr->mDefaultValue.empty()) {
                    attrValue = new ImmAttrValue();
                } else {
                    attrValue = new ImmAttrValue(attr->mDefaultValue);
                }
            }
            
            //Set admin owner as a regular attribute and then also a pointer
            //to the attrValue for efficient access.
            if(i4->first == std::string(SA_IMM_ATTR_ADMIN_OWNER_NAME)) {
                //attrValue->setValueC_str(adminOwner->mAdminOwnerName.c_str());
                object->mAdminOwnerAttrVal = attrValue;
            }
            
            object->mAttrValueMap[i4->first] = attrValue;
        }
        
        // Set attribute values
        p = req->attrValues;
        while(p) {
            //for (int j = 0; j < req->attrValues.list.count; j++) {
            //ImmAttrValues* p = req->attrValues.list.array[j];
            
            sz = strnlen((char *) p->n.attrName.buf, 
                (size_t)p->n.attrName.size);
            std::string attrName((const char*)p->n.attrName.buf, sz);
            
            ImmAttrValueMap::iterator i5 = 
                object->mAttrValueMap.find(attrName);
            
            if (i5 == object->mAttrValueMap.end()) {
                TRACE_7("ERR_NOT_EXIST: attr '%s' not defined", attrName.c_str());
                err = SA_AIS_ERR_NOT_EXIST;
                break; //out of for-loop
            }
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if(attr->mValueType != (unsigned int) p->n.attrValueType) {
                LOG_NO("ERR_INVALID_PARAM: attr '%s' type missmatch", attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
                break; //out of for-loop
            }
            
            assert(p->n.attrValuesNumber > 0);
            ImmAttrValue* attrValue = i5->second;
            IMMSV_OCTET_STRING tmpos; //temporary octet string
            eduAtValToOs(&tmpos, &(p->n.attrValue), 
                (SaImmValueTypeT) p->n.attrValueType);
            attrValue->setValue(tmpos);
            
            if(p->n.attrValuesNumber > 1) {
                /*
                  int extraValues = p->n.attrValuesNumber - 1;
                */
                if(!(attr->mFlags & SA_IMM_ATTR_MULTI_VALUE)) {
                    LOG_NO("ERR_INVALID_PARAM: attr '%s' is not multivalued yet "
                        "multiple values provided in create call", 
                        attrName.c_str());
                    err = SA_AIS_ERR_INVALID_PARAM;
                    break; //out of for loop
                } 
                /*
                  else if(extraValues != p->attrMoreValues.list.count) {
                  LOG_ER( "Multivalued attr '%s' should have "
                  "%u extra values but %u extra values tranmitted",
                  attrName.c_str(), extraValues, 
                  p->attrMoreValues.list.count);
                  //Should not happen, error in packing.
                  err = SA_AIS_ERR_LIBRARY;
                  break; //out of for-loop
                  } */
                else {
                    assert(attrValue->isMultiValued());
                    ImmAttrMultiValue* multiattr = 
                        (ImmAttrMultiValue *) attrValue;
                    
                    IMMSV_EDU_ATTR_VAL_LIST* al= p->n.attrMoreValues;
                    while(al) {
                        eduAtValToOs(&tmpos, &(al->n), 
                            (SaImmValueTypeT) p->n.attrValueType);
                        multiattr->setExtraValue(tmpos);
                        al = al->next;
                    }
                    //TODO: Verify that attrValuesNumber was correct.
                }
            } //if(p->attrValuesNumber > 1)
            p = p->next;
        } //while(p)
        
        //Check that all attributes with INITIALIZED flag have been set.
        ImmAttrValueMap::iterator i6;
        for(i6=object->mAttrValueMap.begin(); 
            i6!=object->mAttrValueMap.end() && err==SA_AIS_OK;
            ++i6) {
            ImmAttrValue* attrValue = i6->second;
            std::string attrName(i6->first);
            
            i4 = classInfo->mAttrMap.find(attrName);
            assert(i4!=classInfo->mAttrMap.end());
            AttrInfo* attr = i4->second;
            
            if((attr->mFlags & SA_IMM_ATTR_INITIALIZED) && attrValue->empty()){
                LOG_NO("ERR_INVALID_PARAM: attr '%s' must be initialized "
                    "yet no value provided in the object create call", 
                    attrName.c_str());
                err = SA_AIS_ERR_INVALID_PARAM;
            }
        }
        
        if(err == SA_AIS_OK) {
            sObjectMap[objectName] = object; 
            classInfo->mRefCount++;
            TRACE_7("Object '%s' was synced ", objectName.c_str());
        } else {
            //err != SA_AIS_OK
            //Delete object and its attributes
            
            ImmAttrValueMap::iterator oavi;
            for(oavi = object->mAttrValueMap.begin();
                oavi != object->mAttrValueMap.end(); ++oavi) {
                delete oavi->second;  
            }
            
            //Empty the collection, probably not necessary (as the ObjectInfo
            //record is deleted below), but does not hurt.
            object->mAttrValueMap.clear(); 
            
            object->mImplementer=0;
            object->mAdminOwnerAttrVal=0;
            object->mClassInfo=0;
            delete object; 
        }
    }
    
 objectSyncExit:
    sImplsDeadDuringSync.clear();
    sNodesDeadDuringSync.clear();
    /* Clear the "tombstones" for Implementers and Nodes for each sync message
       received. The tiny hole that we need to plug only exists after the
       last sync message, when coord sends the finalizeSync message until
       everyone receives it. Nodes may receive discardImplementer or
       discardNode messages over fevs before receiving finalizeSync.
       These nodes/implementers will still be part of the finalizeSync 
       message, causing apparent verification failure in veterans and 
       incorrectly installed implementers in sync clients. 
     */
    TRACE_LEAVE(); 
    return err;
}

void
ImmModel::getObjectName(ObjectInfo* info, std::string& objName)
{
    ObjectMap::iterator oi;
    for(oi=sObjectMap.begin(); oi!=sObjectMap.end(); ++oi) {
        if(oi->second == info) {
            objName = oi->first;
            return;
        }
    }
    assert(0);
}

SaAisErrorT
ImmModel::finalizeSync(ImmsvOmFinalizeSync* req, bool isCoord, 
    bool isSyncClient)
{
    TRACE_ENTER();
    SaAisErrorT err=SA_AIS_OK;
    assert(!(isCoord && isSyncClient));
    
    switch(sImmNodeState){ 
        case IMM_NODE_W_AVAILABLE:
            assert(isSyncClient);
            break;
        case IMM_NODE_R_AVAILABLE:
            assert(!isSyncClient);
            break;
        case IMM_NODE_UNKNOWN:
        case IMM_NODE_ISOLATED: 
        case IMM_NODE_LOADING:
        case IMM_NODE_FULLY_AVAILABLE:
            LOG_ER("Node is in a state %u that cannot accept"
                "finalize of sync, will terminate %u %u", sImmNodeState, 
                isCoord,
                isSyncClient);
            assert(0);
        default:
            LOG_ER("Impossible node state, will terminate");
            assert(0);
    }
    
    if(isCoord) {//Produce the checkpoint 
        CcbVector::iterator ccbItr;

        sImmNodeState = IMM_NODE_FULLY_AVAILABLE;
        LOG_IN("NODE STATE-> IMM_NODE_FULLY_AVAILABLE %u", __LINE__);
        /*WARNING the controller node here goes to writable state
          based on a NON FEVS message, directly from the sync-process. 
          Other nodes go to writable based on the reception of the 
          finalizeSync message over FEVS. This means that the controller
          actually opens up for writes before having received the 
          finalize sync message. 
          
          This should still be safe based on the following reasoning:
          Any writable operation now allowed at this node will be
          sent over fevs AFTER the finalizeSync message that this node
          just sent. This at least is the case for messages arriving at
          this node. So we know that ALL nodes, including this coord node,
          will receive any mutating messages AFTER ALL nodes, including this
          coord node, have received the finalizeSync message. This at least
          for messages arriving from clients at this node.
          
          At the other nodes. Any mutating message does a local check on writability.
          If currently not writable, the request is rejected locally. 
          Shift to writability is only by finalize sync or abortSync. 
        */
        
        req->lastContinuationId = sLastContinuationId;
        req->adminOwners = NULL;
        AdminOwnerVector::iterator i;
        
        for(i=sOwnerVector.begin(); i!=sOwnerVector.end();) {
            if((*i)->mDying) {
                LOG_WA("Removing admin owner %u %s which is in demise, before "
                    "generating sync message", (*i)->mId,
                    (*i)->mAdminOwnerName.c_str());
                assert(adminOwnerDelete((*i)->mId, true) == SA_AIS_OK);
                //Above does a lookup of admin owner again.
                i=sOwnerVector.begin();//Restart of iteration.
            } else {
                ++i;
            }
        }
        
        for(i=sOwnerVector.begin(); i!=sOwnerVector.end(); ++i) {
            assert(!(*i)->mDying);
            ImmsvAdmoList* ai = (ImmsvAdmoList *) 
                calloc(1, sizeof(ImmsvAdmoList));
            ai->id = (*i)->mId;
            ai->releaseOnFinalize = (SaBoolT) (*i)->mReleaseOnFinalize;
            ai->nodeId = (*i)->mNodeId;
            ai->adminOwnerName.size = (*i)->mAdminOwnerName.size();
            ai->adminOwnerName.buf = 
                strndup((char *) (*i)->mAdminOwnerName.c_str(),
                    ai->adminOwnerName.size);
            
            ai->touchedObjects = NULL;
            
            ObjectSet::iterator oi;
            for(oi = (*i)->mTouchedObjects.begin();
                oi!=(*i)->mTouchedObjects.end();
                ++oi) {
                ImmsvObjNameList* nl = (ImmsvObjNameList *)
                    calloc(1, sizeof(ImmsvObjNameList));
                std::string objName;
                //TODO: re-implement, this is really ineficient.
                getObjectName(*oi, objName); 
                nl->name.size = objName.size();
                nl->name.buf =strndup((char *) objName.c_str(), 
                    nl->name.size);
                nl->next = ai->touchedObjects;
                ai->touchedObjects = nl;
            }
            ai->next = req->adminOwners;
            req->adminOwners = ai;
        }
        
        LOG_NO("finalizeSync message contains %u admin-owners", 
            (unsigned int) sOwnerVector.size());
        
        req->implementers = NULL;
        ImplementerVector::iterator i2;
        for(i2=sImplementerVector.begin(); i2!=sImplementerVector.end(); 
            ++i2) {
            ImmsvImplList* ii = (ImmsvImplList *)
                calloc(1, sizeof(ImmsvImplList));
            if((*i2)->mDying) {
                ii->id = 0;
                ii->nodeId = 0;
                ii->mds_dest = 0LL;

                /* Note that here the finalize sync message is bypassing
                   any discardImplementer over fevs. Old clients receiving
                   this message will need to discard implementer on the 
                   basis of the incoming 'id' being zero.
                 */
            } else {
                ii->id = (*i2)->mId; /* id is zero if dead. */
                ii->nodeId = (*i2)->mNodeId;
                ii->mds_dest = (*i2)->mMds_dest;
            }
            ii->implementerName.size = (*i2)->mImplementerName.size();
            ii->implementerName.buf = 
                strndup((char *) (*i2)->mImplementerName.c_str(), 
                    ii->implementerName.size);
            
            ii->next = req->implementers;
            req->implementers = ii;
        }

        LOG_NO("finalizeSync message contains %u implementers", 
            (unsigned int) sImplementerVector.size());
        
        req->classes = NULL;
        ClassMap::iterator i3;
        for(i3= sClassMap.begin(); i3 != sClassMap.end();++i3) {
            ClassInfo* ci = i3->second;
            ImmsvClassList* ioci = (ImmsvClassList *) 
                calloc(1, sizeof(ImmsvClassList));
            ioci->className.size = i3->first.size();
            ioci->className.buf = 
                strndup((char *) i3->first.c_str(), ioci->className.size);
            if(ci->mImplementer) {
                ioci->classImplName.size = 
                    ci->mImplementer->mImplementerName.size();
                ioci->classImplName.buf = 
                    strndup((char *)ci->mImplementer->mImplementerName.c_str(),
                        ioci->classImplName.size);
            }
            ioci->nrofInstances = ci->mRefCount;
            ioci->next = req->classes;
            req->classes = ioci;
        }

        LOG_IN("finalizeSync message contains %u class info records", 
            (unsigned int) sClassMap.size());

        for(ccbItr=sCcbVector.begin(); ccbItr!=sCcbVector.end(); ++ccbItr){
            if((*ccbItr)->isActive()) {
                LOG_ER("FinalizeSync: Found active transaction %u, "
                    "sync should not have started!", (*ccbItr)->mId);
                err = SA_AIS_ERR_BAD_OPERATION;
                goto done;
            }

            /*
              TRACE("Encode CCB-ID:%u outcome:(%u)%s", (*ccbItr)->mId,
               (*ccbItr)->mState,
               ((*ccbItr)->mState == IMM_CCB_COMMITTED)?"COMMITTED":
               (((*ccbItr)->mState == IMM_CCB_ABORTED)?"ABORTED":"OTHER"));
            */

            ImmsvCcbOutcomeList* ol = (ImmsvCcbOutcomeList *)
                calloc(1, sizeof(ImmsvCcbOutcomeList));
            ol->ccbId = (*ccbItr)->mId;
            ol->ccbState = (*ccbItr)->mState;
            ol->next = req->ccbResults;
            req->ccbResults = ol;
        }
        
    } else {
        //SyncFinalize received by all cluster members, old and new-joining.
        //Joiners will copy the finalize state.
        //old members (except coordinator) will verify equivalent state.
        if(isSyncClient) {
            //syncronize with the checkpoint
            //sLastAdminOwnerId = req->lastAdminOwnerId;
            //sLastCcbId = req->lastCcbId;
            //sLastImplementerId = req->lastImplementerId;
            sLastContinuationId = req->lastContinuationId;
            
            //Sync currently existing AdminOwners.
            ImmsvAdmoList* ai = req->adminOwners;
            while(ai) {
                AdminOwnerInfo* info = new AdminOwnerInfo;
                info->mId = ai->id;
                info->mNodeId = ai->nodeId;
                info->mConn = 0;
                info->mAdminOwnerName.append((const char *) 
                    ai->adminOwnerName.buf, (size_t) ai->adminOwnerName.size);
                info->mReleaseOnFinalize = ai->releaseOnFinalize;
                info->mDying = false;
                
                ImmsvObjNameList* nl = ai->touchedObjects;
                TRACE_5("Synced admin owner %s",info->mAdminOwnerName.c_str());
                while(nl) {
                    std::string objectName((const char *) nl->name.buf, 
                        (size_t) strnlen((const char *) nl->name.buf,
                            (size_t) nl->name.size));
                    
                    ObjectMap::iterator oi = sObjectMap.find(objectName);
                    if(oi == sObjectMap.end()) {
                        LOG_ER("Sync client failed to locate object: "
                            "%s, will restart.", objectName.c_str());
                        assert(0);
                    }
                    info->mTouchedObjects.insert(oi->second);
                    nl = nl->next;
                }
                sOwnerVector.push_back(info);
                ai = ai->next;
            }
            
            //Sync currently existing implementers
            ImmsvImplList* ii = req->implementers;
            while(ii) {
                ImplementerInfo* info = new ImplementerInfo;
                IdVector::iterator ivi = sImplsDeadDuringSync.begin();
                for(;ivi != sImplsDeadDuringSync.end(); ++ivi) {
                    if((*ivi) == ii->id) {
                        LOG_NO("Avoiding re-create of implementer %u in finalizeSync",
                            ii->id);
                        /* The coord generated finalizeSync just before a discardImplementer
                           arrived. Zero parts of incoming data, making it appear as if
                           discardImplementer arrived before generation of finalizeSync.
                        */
                        ii->id = 0; 
                        ii->nodeId = 0;
                        ii->mds_dest = 0LL;
                        break;/* out of inner for-loop */
                    }
                }

                info->mId = ii->id;
                info->mNodeId = ii->nodeId;
                info->mConn = 0;
                info->mMds_dest = ii->mds_dest;
                info->mDying = false; //only relevant on the implementers node.
                info->mImplementerName.append((const char *) 
                    ii->implementerName.buf,
                    (size_t) ii->implementerName.size);
                info->mAdminOpBusy=0;
                sImplementerVector.push_back(info);
                ii = ii->next;
            }

            //Attach object implementers using the implementer-name attribute.
            ObjectMap::iterator oi;
            std::string implAttr(SA_IMM_ATTR_IMPLEMENTER_NAME);
            for(oi = sObjectMap.begin(); oi != sObjectMap.end(); oi++) {
                ObjectInfo* obj = oi->second;
                //ImmAttrValueMap::iterator j;
                ImmAttrValue* att = obj->mAttrValueMap[implAttr];
                assert(att);
                if(!(att->empty())) {
                    std::string implName(att->getValueC_str());
                    /*
                      TRACE_5("Attaching implementer %s to object %s",
                      implName.c_str(), oi->first.c_str());
                    */
                    ImplementerInfo* impl = findImplementer(implName);
                    //assert(impl);
                    if(impl) {
                        //Implementer name may be assigned but none attached 
                        obj->mImplementer = impl;
                    }
                }
            }
            
            unsigned int classCount = 0;
            ImmsvClassList* ioci = req->classes;
            while(ioci) {
                size_t sz = strnlen((char *) ioci->className.buf, 
                    (size_t) ioci->className.size);
                std::string className((char *) ioci->className.buf, sz);
                
                ClassMap::iterator i3 = sClassMap.find(className);
                assert(i3 != sClassMap.end());
                ClassInfo* ci = i3->second;
                //Is the assert below really true if the class is a runtime 
                //class ?
                //Yes, because the existence of a rt object is noted at 
                //all nodes.
                assert(ci->mRefCount == (int) ioci->nrofInstances);
                ++classCount;
                LOG_IN("Synced class %s has %u instances", 
                    className.c_str(), ioci->nrofInstances);
                if(ioci->classImplName.size) {
                    sz = strnlen((char *) ioci->classImplName.buf, 
                        (size_t) ioci->classImplName.size);
                    std::string clImplName((char *) ioci->classImplName.buf, 
                        sz);
                    
                    ImplementerInfo* impl = findImplementer(clImplName);
                    assert(impl);
                    ci->mImplementer = impl;
                }
                ioci = ioci->next;
            }
            LOG_IN("Synced %u classes", classCount);
            assert(sClassMap.size() == classCount);

            ImmsvCcbOutcomeList* ol = req->ccbResults;
            while(ol) {
                CcbInfo* newCcb = new CcbInfo;
                newCcb->mId = ol->ccbId;
                newCcb->mAdminOwnerId = 0;
                newCcb->mCcbFlags = 0;
                newCcb->mOriginatingNode = 0;
                newCcb->mOriginatingConn = 0;
                newCcb->mVeto = SA_AIS_OK;
                newCcb->mState = (ImmCcbState) ol->ccbState;
                newCcb->mWaitStartTime = time(NULL);
                sCcbVector.push_back(newCcb);
    
                TRACE_5("CCB %u state %s", newCcb->mId, 
                    (newCcb->mState == IMM_CCB_COMMITTED)?"COMMITTED":
                    ((newCcb->mState == IMM_CCB_ABORTED)?"ABORTED":"OTHER"));
                assert(!(newCcb->isActive()));
                ol = ol->next;
            }

            TRACE_5("Synced %u CCB-outcomes",(unsigned int) sCcbVector.size());
            
            this->setLoader(0); 
            //Opens for OM and OI connections to this node.
            //immnd must also set cb->mAccepted=SA_TRUE

            if(!sNodesDeadDuringSync.empty()) {
                IdVector::iterator ivi = sNodesDeadDuringSync.begin();
                for(;ivi != sNodesDeadDuringSync.end(); ++ivi) {
                    ConnVector cv;
                    LOG_NO("Sync client re-executing discardNode for node %x", (*ivi));
                    this->discardNode((*ivi), cv);
                    assert(cv.empty());
                }
            }
        } else {
            CcbVector::iterator ccbItr;
            //verify the checkpoint at veteran nodes (not coord & not sync-client)

            
            sImmNodeState = IMM_NODE_FULLY_AVAILABLE;
            LOG_IN("NODE STATE-> IMM_NODE_FULLY_AVAILABLE %u", __LINE__);
            
            if(sLastContinuationId != (unsigned int) req->lastContinuationId) {
                LOG_ER("Sync-verify: Established node has "
                    "different Continuation count (%u) than expected (%u)",
                    sLastContinuationId, req->lastContinuationId);
                if(sLastContinuationId < 
                    (unsigned int) req->lastContinuationId) {
                    sLastContinuationId = req->lastContinuationId;
                } else {
                    LOG_ER("Continuation count is LARGER than coord - exiting");
                    /* Could be a case of counter wrap arround on top of the 
                       basic error of divergence. */
                    exit(1);
                }
            }
            
            //verify currently existing AdminOwners.

            AdminOwnerVector::iterator i2;
            for(i2=sOwnerVector.begin(); i2!=sOwnerVector.end();) {
                if((*i2)->mDying) {
                    LOG_WA("Removing admin owner %u %s which is in demise, "
                        "before receiving sync/verify message", 
                        (*i2)->mId,
                        (*i2)->mAdminOwnerName.c_str());
                    assert(adminOwnerDelete((*i2)->mId, true) == SA_AIS_OK);
                    //lookup of admin owner again.

                    //restart of iteration again.
                    i2=sOwnerVector.begin();
                } else {
                    ++i2;
                }
            }
            
            ImmsvAdmoList* ai = req->adminOwners;
            while(ai) {
                int nrofTouchedObjs=0;
                i2 = std::find_if(sOwnerVector.begin(), sOwnerVector.end(), 
                    IdIs(ai->id));
                assert(!(i2==sOwnerVector.end()));
                AdminOwnerInfo* info = *i2;
                assert(!info->mDying);
                std::string ownerName((const char *) ai->adminOwnerName.buf,
                    (size_t) strnlen((const char *) ai->adminOwnerName.buf,
                        (size_t) ai->adminOwnerName.size));
                if(info->mAdminOwnerName != ownerName) {
                    LOG_ER("Sync-verify: Established node has different "
                        "AdminOwner-name %s for id %u, should be %s.",
                        info->mAdminOwnerName.c_str(), ai->id, 
                        ownerName.c_str());
                    assert(0);
                }
                if(info->mReleaseOnFinalize != ai->releaseOnFinalize) {
                    LOG_ER("Sync-verify: Established node has "
                        "different release-on-verify flag (%u) for AdminOwner "
                        "%s, should be %u.", info->mReleaseOnFinalize, 
                        ownerName.c_str(), ai->releaseOnFinalize);
                    assert(0);
                }
                if(info->mNodeId != (unsigned int) ai->nodeId) {
                    LOG_ER("Sync-verify: Established node has "
                        "different nodeId (%x) for AdminOwner "
                        "%s, should be %x.", info->mNodeId, 
                        ownerName.c_str(), ai->nodeId);
                    assert(0);
                }
                
                ImmsvObjNameList* nl = ai->touchedObjects;
                while(nl) {
                    ++nrofTouchedObjs;
                    nl = nl->next;
                }
                
                if(nrofTouchedObjs != (int)info->mTouchedObjects.size()){
                    LOG_ER("Sync-verify: Missmatch on number of "
                        "touched objects %u for AdminOwner %s, should be %u.",
                        (unsigned int) info->mTouchedObjects.size(), 
                        ownerName.c_str(),
                        nrofTouchedObjs);
                    assert(0);
                }
                ai = ai->next;
            }
            
            //Verify currently existing implementers
            ImmsvImplList* ii = req->implementers;
            while(ii) {
                std::string implName((const char *) ii->implementerName.buf, 
                    (size_t) strnlen((const char *)
                        ii->implementerName.buf,
                        (size_t) ii->implementerName.size));
                
                ImplementerInfo* info = findImplementer(implName);
                assert(info!=NULL);
                
                if(info->mId != (unsigned int) ii->id) {
                    bool explained = false;
                    if(ii->id == 0) {
                        LOG_NO("Sync-verify: Veteran node has different "
                            "Implementer-id %u for name%s, should be 0 "
                            "according to sync-verify. Discarding it.",
                            info->mId, implName.c_str());
                        /*Does not delete struct that info points to.*/
                        discardImplementer(info->mId, true);
                        explained = true;
                    } else if(info->mId == 0) {
                        /* Here info->mid == 0 i.e. veteran claims dead implementer 
                           but coord claims non-dead implementer. This can happen
                           when implementer is deleted just after coord sends 
                           finalizeSync. Checking for this.
                        */
                        IdVector::iterator ivi = sImplsDeadDuringSync.begin();
                        for(;ivi != sImplsDeadDuringSync.end(); ++ivi) {
                            if((*ivi) == ii->id) {
                                LOG_NO("Detected dead implementer %u in "
                                      "finalizeSync message - ignoring.", ii->id);
                                explained = true;
                                break;
                            }
                        }

                        if(ivi == sImplsDeadDuringSync.end() && 
                           !(sNodesDeadDuringSync.empty()) && ii->nodeId) {
                            /* Could be dead node instead. */
                            ivi = sNodesDeadDuringSync.begin();
                            for(;ivi != sNodesDeadDuringSync.end(); ++ivi) {
                                if((*ivi) == ii->nodeId) {
                                    LOG_NO("Detected implementer from dead node %u in "
                                        "finalizeSync message - ignoring", ii->nodeId);
                                    explained = true;
                                    break;
                                }
                            }
                        }
                    }

                    if(!explained) {
                        LOG_ER("Sync-verify: Established node has different "
                               "Implementer-id: %u for name%s, sync says %u. ",
                               info->mId, implName.c_str(), ii->id);
                        abort();
                    }
                }
                
                if(info->mNodeId != ii->nodeId) {
                    LOG_ER("Sync-verify: Missmatch on node-id "
                        "%x for implementer %s, sync says %x",
                        info->mNodeId, implName.c_str(), ii->nodeId);
                    abort();
                }
                ii = ii->next;
            }
            
            //Verify classes
            int classCount = 0;
            ImmsvClassList* ioci = req->classes;
            while(ioci) {
                size_t sz = strnlen((char *) ioci->className.buf, 
                    (size_t) ioci->className.size);
                std::string className((char *) ioci->className.buf, sz);
                ClassMap::iterator i3 = sClassMap.find(className);
                assert(i3 != sClassMap.end());
                ClassInfo* ci = i3->second;
                assert(ci->mRefCount == (int) ioci->nrofInstances);
                if(ioci->classImplName.size) {
                    sz = strnlen((char *) ioci->classImplName.buf, 
                        (size_t) ioci->classImplName.size);
                    std::string clImplName((char *) ioci->classImplName.buf, 
                        sz);
                    ImplementerInfo* impl = findImplementer(clImplName);
                    assert(impl);
                    assert(ci->mImplementer);
                    assert(ci->mImplementer == impl);
                }
                ++classCount;
                ioci = ioci->next;
            }

            //Verify CCB outcomes.
            ImmsvCcbOutcomeList* ol = req->ccbResults;
            unsigned int verified = 0;
            unsigned int gone = 0;
            while(ol) {
                CcbVector::iterator i1 = std::find_if(sCcbVector.begin(),
                    sCcbVector.end(), CcbIdIs(ol->ccbId));
                if(i1 == sCcbVector.end()) {
                    ++gone;
                } else {
                    CcbInfo* ccb = *i1;
                    assert(ccb->mState == (ImmCcbState) ol->ccbState);
                    ++verified;
                    /*
                        TRACE_5("CCB %u verified with state %s", ccb->mId, 
                         (ccb->mState == IMM_CCB_COMMITTED)?"COMMITTED":
                         ((ccb->mState == IMM_CCB_ABORTED)?"ABORTED":"OTHER"));
                    */
                }
                ol = ol->next;
            }
            TRACE_5("Verified %u CCBs, %u gone", verified, gone);
            if(sCcbVector.size() != verified + gone) {
                LOG_WA("sCcbVector.size()/%u != verified/%u + gone/%u",
                    (unsigned int) sCcbVector.size(), verified, gone);
            }
            //Old member passed verification.
        }
    }

    sImplsDeadDuringSync.clear(); /* for coord, sync-client & veterans. */
    sNodesDeadDuringSync.clear(); /* should only be relevant for sync-client. */
 done:
    TRACE_LEAVE();
    return err;
}

