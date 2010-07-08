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

#ifndef _ImmModel_hh_
#define _ImmModel_hh_ 1

#include "saImmOm.h"
#include <sys/types.h>
#include <string>
#include <vector>
#include <map>

struct ClassInfo;
struct CcbInfo;
struct ObjectInfo;
struct AdminOwnerInfo;
struct ImplementerInfo;

//TODO: Some of the struct forward declarations below are obsolete.

struct ImmsvOmSearchInit;
struct ImmsvOmCcbObjectCreate;
struct ImmsvOmCcbObjectModify;
struct ImmsvOmCcbObjectDelete;
struct ImmsvOmClassDescr; 
struct ImmsvOmAdminOperationInvoke;
struct ImmsvOmAdminOwnerInitialize;
struct ImmsvAttrDefinition;
struct immsv_octet_string;
struct immsv_edu_attr_val;
struct ImmsvOiImplSetReq;
struct ImmsvOmObjectSync;
struct immsv_a2nd_admown_set;
struct ImmsvOmSearchOneAttr;
struct ImmsvOmFinalizeSync;

struct ImmOmClassDelete;
struct ImmOmCcbObjectDelete;
struct ImmOmCcbApply;
struct ImmOiObjectImplementerSet;
struct ImmOiRtObjectCreate;

struct immsv_oi_ccb_upcall_rsp;

class ImmSearchOp;

typedef std::vector<SaUint32T> ConnVector;
typedef std::vector<unsigned int> NodeIdVector;
typedef std::vector<std::string> ObjectNameVector;
typedef std::vector<SaUint32T> IdVector;
typedef std::map<std::string, ObjectInfo*> ObjectMap;
typedef std::vector<SaInvocationT> InvocVector;

struct ImmOiImplementerClear;

class ImmModel
{
public:
    ImmModel();
    static ImmModel*    instance(void** sInstancep);
    // Classes
    
    SaAisErrorT         classCreate(
                                    const ImmsvOmClassDescr* req,
                                    SaUint32T reqConn,
                                    SaUint32T nodeId,
                                    SaUint32T* continuationId,
                                    SaUint32T* pbeConn,
                                    unsigned int* pbeNodeId);

    
    SaAisErrorT         classDelete(
                                    const ImmsvOmClassDescr* req,
                                    SaUint32T reqConn,
                                    SaUint32T nodeId,
                                    SaUint32T* continuationId,
                                    SaUint32T* pbeConn,
                                    unsigned int* pbeNodeId);
    
    SaAisErrorT         classDescriptionGet(
                                            const immsv_octet_string* clName,
                                            ImmsvOmClassDescr* res);
    
    SaAisErrorT         classSerialize(
                                       const char* className, 
                                       char** data, 
                                       size_t* size);
    
    
    SaAisErrorT         attrCreate(
                                   ClassInfo* classInfo,
                                   const ImmsvAttrDefinition* attr);
    
    // Admin ownership
    
    SaAisErrorT         adminOwnerCreate(
                                         const ImmsvOmAdminOwnerInitialize* req,
                                         SaUint32T ownerId,
                                         SaUint32T conn,
                                         unsigned int nodeId);
    
    SaAisErrorT         adminOwnerDelete(SaUint32T ownerId, bool hard);
    
    SaAisErrorT         adminOwnerChange(
                                         const immsv_a2nd_admown_set* req, 
                                         bool release=false);
    
    // CCB
    SaAisErrorT         ccbCreate(
                                  SaUint32T adminOwnerId, 
                                  SaUint32T ccbFlags, 
                                  SaUint32T ccbId,
                                  unsigned int originatingNode,
                                  SaUint32T originatingConn);
    
    SaAisErrorT         ccbApply(
                                 SaUint32T ccbId,
                                 SaUint32T reqConn,
                                 ConnVector& connVector,
                                 IdVector& implIds,
                                 IdVector& continuations);
    
    SaAisErrorT         ccbTerminate(SaUint32T ccbId);
    
    SaAisErrorT         ccbObjectCreate(
                                        const ImmsvOmCcbObjectCreate* req,
                                        SaUint32T* implConn,
                                        unsigned int* implNodeId,
                                        SaUint32T* continuationId,
                                        SaUint32T* pbeConn,
                                        unsigned int* pbeNodeId);
    
    SaAisErrorT         ccbObjectModify(
                                        const ImmsvOmCcbObjectModify* req,
                                        SaUint32T* implConn,
                                        unsigned int* implNodeId,
                                        SaUint32T* continuationId,
                                        SaUint32T* pbeConn,
                                        unsigned int* pbeNodeId);
    
    SaAisErrorT         ccbObjectDelete(
                                        const ImmsvOmCcbObjectDelete* req,
                                        SaUint32T reqConn,
                                        ObjectNameVector& objNameVector,
                                        ConnVector& connVector,
                                        IdVector& continuations,
                                        SaUint32T* pbeConn,
                                        unsigned int* pbeNodeId);

    SaAisErrorT         deleteObject(
                                     ObjectMap::iterator& oi,
                                     SaUint32T reqConn,
                                     AdminOwnerInfo* adminOwner,
                                     CcbInfo* ccb, 
                                     bool doIt,
                                     ObjectNameVector& objNameVector,
                                     ConnVector& connVector,
                                     IdVector& continuations,
                                     unsigned int pbeIsLocal);
    
    bool                ccbWaitForDeleteImplAck(
                                                SaUint32T ccbId, 
                                                SaAisErrorT* err);

    bool                ccbWaitForCompletedAck(
                                               SaUint32T ccbId, 
                                               SaAisErrorT* err,
                                               SaUint32T* pbeConn,
                                               unsigned int* pbeNodeId,
                                               SaUint32T* pbeId,
                                               SaUint32T* pbeCtn);
    
    void                ccbObjDelContinuation(
                                              const immsv_oi_ccb_upcall_rsp* rsp,
                                              SaUint32T* reqConn);
    
    void                ccbCompletedContinuation(
                                                 const immsv_oi_ccb_upcall_rsp* rsp,
                                                 SaUint32T* reqConn);
    
    void                ccbObjCreateContinuation(
                                                 SaUint32T ccbId, 
                                                 SaUint32T invocation,
                                                 SaAisErrorT err,
                                                 SaUint32T* reqConn);
    
    void                ccbObjModifyContinuation(
                                                 SaUint32T ccbId, 
                                                 SaUint32T invocation,
                                                 SaAisErrorT err,
                                                 SaUint32T* reqConn);


    void                 pbePrtObjCreateContinuation(
                                                     SaUint32T invocation,
                                                     SaAisErrorT error,
                                                     unsigned int nodeId, 
                                                     SaUint32T *reqConn);
    
    void                 pbePrtObjDeletesContinuation(
                                                     SaUint32T invocation,
                                                     SaAisErrorT error,
                                                     unsigned int nodeId, 
                                                     SaUint32T *reqConn);
    
    void                 pbePrtAttrUpdateContinuation(
                                                     SaUint32T invocation,
                                                     SaAisErrorT error,
                                                     unsigned int nodeId, 
                                                     SaUint32T *reqConn);

    void                 pbeClassCreateContinuation(
                                                     SaUint32T invocation,
                                                     unsigned int nodeId, 
                                                     SaUint32T *reqConn);
    
    void                 pbeClassDeleteContinuation(
                                                     SaUint32T invocation,
                                                     unsigned int nodeId, 
                                                     SaUint32T *reqConn);
    
    void                 pbeUpdateEpochContinuation(
                                                     SaUint32T invocation,
                                                     unsigned int nodeId);
    
    // Administrative operations
    SaAisErrorT         adminOperationInvoke(
                                             const ImmsvOmAdminOperationInvoke* req,
                                             SaUint32T reqConn,
                                             SaUint64T reply_dest,
                                             SaInvocationT& inv,
                                             SaUint32T* implConn,
                                             unsigned int* implNodeId);
    
    // Objects
    
    SaAisErrorT         searchInitialize(
                                         const ImmsvOmSearchInit* req,
                                         ImmSearchOp& op);
    
    SaAisErrorT         accessorGet(
                                    const ImmsvOmSearchInit* req,
                                    ImmSearchOp& op);
    
    bool                filterMatch(
                                    ObjectInfo* obj, 
                                    ImmsvOmSearchOneAttr* filter,
                                    SaAisErrorT& err,
                                    const char* objName);
    
    
    // Implementer API
    SaAisErrorT         implementerSet(
                                       const immsv_octet_string* implName,
                                       SaUint32T con,
                                       SaUint32T nodeId,
                                       SaUint32T ownerId,
                                       SaUint64T mds_dest);
    
    SaAisErrorT         classImplementerSet(
                                            const struct ImmsvOiImplSetReq* req,
                                            SaUint32T con,
                                            unsigned int nodeId);
    
    SaAisErrorT         classImplementerRelease(
                                                const struct ImmsvOiImplSetReq* req,
                                                SaUint32T con,
                                                unsigned int nodeId);
    
    SaAisErrorT         objectImplementerSet(
                                             const struct ImmsvOiImplSetReq* req,
                                             SaUint32T con,
                                             unsigned int nodeId);
    
    SaAisErrorT         objectImplementerRelease(
                                                 const struct ImmsvOiImplSetReq* req,
                                                 SaUint32T con,
                                                 unsigned int nodeId);
    
    SaAisErrorT         implementerClear(
                                         const struct ImmsvOiImplSetReq* req,
                                         SaUint32T con,
                                         unsigned int nodeId);
    
    SaAisErrorT         rtObjectCreate(
                                       const struct ImmsvOmCcbObjectCreate* req,
                                       SaUint32T conn,
                                       unsigned int nodeId,
                                       SaUint32T* continuationId,
                                        SaUint32T* pbeConn,
                                        unsigned int* pbeNodeId);

    
    SaAisErrorT         rtObjectUpdate(
                                       const ImmsvOmCcbObjectModify* req, //re-used struct
                                       SaUint32T conn,
                                       unsigned int nodeId,
                                       bool* isPureLocal,
                                       SaUint32T* continuationId,
                                       SaUint32T* pbeConn,
                                       unsigned int* pbeNodeId);
    
    SaAisErrorT         rtObjectDelete(
                                       const ImmsvOmCcbObjectDelete* req, //re-used struct
                                       SaUint32T conn,
                                       unsigned int nodeId,
                                       SaUint32T* continuationId,
                                       SaUint32T* pbeConnPtr,
                                       unsigned int* pbeNodeIdPtr,
                                       ObjectNameVector& objNameVector);
    
    SaAisErrorT         deleteRtObject(
                                       ObjectMap::iterator& oi,
                                       bool doIt,
                                       ImplementerInfo* info,
                                       bool& subTreeHasPersistent);
    
    SaAisErrorT       objectSync(const ImmsvOmObjectSync* req);

    SaAisErrorT       finalizeSync(
                                   ImmsvOmFinalizeSync* req,
                                   bool isCoord, bool isSyncClient);
    
    void              eduAtValToOs(
                                   immsv_octet_string* tmpos,
                                   immsv_edu_attr_val* v,
                                   SaImmValueTypeT t);

    void              getObjectName(ObjectInfo* obj, std::string& name);
    void              setLoader(int pid);
    int               getLoader();
    int               adjustEpoch(int suggestedEpoch, 
                                  SaUint32T* continuationId,
                                  SaUint32T* pbeConnPtr,
                                  unsigned int* pbeNodeIdPtr,
	                          bool increment);

    SaImmRepositoryInitModeT getRepositoryInitMode();
    void              prepareForLoading();
    bool              readyForLoading();
    void              prepareForSync(bool isJoining);
    void              recognizedIsolated();
    bool              syncComplete(bool isJoining);
    void              abortSync();
    void              pbePrtoPurgeMutations(unsigned int nodeId, ConnVector& connVector);
    SaAisErrorT       ccbResult(SaUint32T ccbId);
    bool              ccbsTerminated();
    bool              pbeIsInSync();
    void              getNonCriticalCcbs(IdVector& cv);
    void              getOldCriticalCcbs(
                                         IdVector& cv, 
                                         SaUint32T* pbeConn,
                                         unsigned int* pbeNodeId,
                                         SaUint32T* pbeId);
    bool              immNotWritable();
    bool              immNotPbeWritable();
    void*             getPbeOi(SaUint32T* pbeConn, unsigned int* pbeNode);
    SaUint32T         findConnForImplementerOfObject(std::string objectDn);
    ImplementerInfo*  findImplementer(SaUint32T);
    ImplementerInfo*  findImplementer(std::string&);
    SaUint32T         getImplementerId(SaUint32T localConn);
    void              discardImplementer(
                                         unsigned int implHandle, 
                                         bool reallyDiscard);
    void              discardContinuations(SaUint32T dead);
    void              discardNode(unsigned int nodeId, IdVector& cv);
    void              getCcbIdsForOrigCon(SaUint32T dead, IdVector& cv);
    void              getAdminOwnerIdsForCon(SaUint32T dead, IdVector& cv);
    bool              ccbCommit(SaUint32T ccbId, ConnVector& connVector);
    void              ccbAbort(
                               SaUint32T ccbId, 
                               ConnVector& connVector,
                               SaUint32T* client,
			       unsigned int* pbeNodeIdPtr);
    bool              cleanTheBasement(
                                       unsigned int seconds, 
                                       InvocVector& admReqs,
                                       InvocVector& searchReqs,
                                       IdVector& ccbs,
                                       IdVector& pbePrtoReqs,
                                       bool iAmCoord);
    
    void              fetchAdmImplContinuation(
                                               SaInvocationT& inv, 
                                               SaUint32T* implConn, //in-out!
                                               SaUint64T* reply_dest); //out
    void              fetchAdmReqContinuation(
                                              SaInvocationT& inv,
                                              SaUint32T* reqConn); //in-out?
    void              fetchSearchReqContinuation(
                                                 SaInvocationT& inv,
                                                 SaUint32T* reqConn); //in-out?
    void              setSearchReqContinuation(
                                               SaInvocationT& inv,
                                               SaUint32T conn);
    
    void              fetchSearchImplContinuation(
                                                  SaInvocationT& inv,
                                                  SaUint64T* reply_dest);//out
    void              setSearchImplContinuation(
                                                SaInvocationT& inv,
                                                SaUint64T reply_dest);
    
    void              fetchCcbObjDelImplContinuation(
                                                     SaInvocationT& inv, 
                                                     SaUint32T* implConn,
                                                     SaUint64T* reply_dest);
    /*
      void              setCcbCompletedImplContinuation(
                                                        SaInvocationT& inv,
                                                        SaUint32T conn,
                                                        SaUint64T reply_dest);
      void              fetchCcbCompletedContinuation(
                                                      SaInvocationT& inv, 
                                                      SaUint32T* implConn,
                                                      SaUint64T* reply_dest);
    */

    void              setCcbObjCreateImplContinuation(
                                                      SaInvocationT& inv,
                                                      SaUint32T conn,
                                                      SaUint64T reply_dest);
    void              setCcbObjDelImplContinuation(
                                                   SaInvocationT& inv,
                                                   SaUint32T conn,
                                                   SaUint64T reply_dest);
    void              fetchCcbObjCreateImplContinuation(
                                                        SaInvocationT& inv, 
                                                        SaUint32T* implConn, //in-out!
                                                        SaUint64T* reply_dest);//out
    void              setCcbObjModifyImplContinuation(
                                                      SaInvocationT& inv,
                                                      SaUint32T conn);
    void              fetchCcbObjModifyImplContinuation(
                                                        SaInvocationT& inv, 
                                                        SaUint32T* implConn); //in-out!
    
    SaUint32T          fetchSearchReqContinuation(SaInvocationT& inv);
    
    
    SaAisErrorT         setImplementer(
                                       std::string objectName,
                                       ObjectInfo* obj, 
                                       ImplementerInfo* info,
                                       bool doIt);
    
    SaAisErrorT         releaseImplementer(
                                           std::string objectName,
                                           ObjectInfo* obj, 
                                           ImplementerInfo* info,
                                           bool doIt);
    
    SaAisErrorT         adminOwnerSet(
                                      std::string objectName,
                                      ObjectInfo* obj, 
                                      AdminOwnerInfo* adm,
                                      bool doIt);
    
    SaAisErrorT         adminOwnerRelease(
                                          std::string objectName,
                                          ObjectInfo* obj, 
                                          AdminOwnerInfo* adm,
                                          bool doIt);
    
    
private:
    bool               checkSubLevel(
                                     const std::string& objectName, 
                                     size_t rootStart);

    bool            nameCheck(const std::string& name, bool strict=true) const;
    bool            schemaNameCheck(const std::string& name) const;
    bool            nameToInternal(std::string& name);
    void            nameToExternal(std::string& name);
    
    void               updateImmObject(
                                       std::string newClassName, 
                                       bool remove=false);
    
    void               commitCreate(ObjectInfo* afim);
    bool               commitModify(const std::string& dn, ObjectInfo* afim);
    void               commitDelete(const std::string& dn);
    
    int loaderPid; //(-1) => loading not started or loading partiticpant.
                   // >0  => loading in progress here at coordinator.
                   //  0  => loading completed at coordinator and participants.
    
};

#endif
