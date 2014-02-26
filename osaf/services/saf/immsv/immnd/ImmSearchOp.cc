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

#include "ImmSearchOp.hh"
#include "immnd.h"


ImmSearchOp::ImmSearchOp()
{
    mLastResult=NULL;
    mIsSync=false;
    mIsAccessor=false;
    syncOsi=NULL;
    attrNameList=NULL;
    classInfo=NULL;
}

ImmSearchOp::~ImmSearchOp()
{
    mResultList.clear();
    mRtsToFetch.clear();
    //Do NOT try to delete mlastResult, it is not owned by this object.
    mLastResult=NULL;
    osafassert(syncOsi == NULL);
    osafassert(attrNameList == NULL);
    classInfo = NULL;
}

void
ImmSearchOp::addObject(
                       const std::string& objectName)
{
    //TRACE_ENTER();
    mResultList.push_back(SearchObject(objectName));
    //TRACE_LEAVE();
}

void
ImmSearchOp::addAttribute(const std::string& attributeName, 
    SaUint32T valueType,
    SaImmAttrFlagsT flags)
    
{
    SearchObject& obj = mResultList.back();
    obj.attributeList.push_back(SearchAttribute(attributeName));    
    SearchAttribute& attr = obj.attributeList.back();  
    attr.valueType = (SaImmValueTypeT) valueType;
    attr.flags = flags;
}

void
ImmSearchOp::addAttrValue(const ImmAttrValue& value)
{
    SearchObject& obj = mResultList.back();
    SearchAttribute& attr = obj.attributeList.back();
    if(value.isMultiValued() && value.extraValues()) {
        attr.valuep = new ImmAttrMultiValue(*((ImmAttrMultiValue  *) &value));
    } else {
        attr.valuep = new ImmAttrValue(value);
    }
}

void
ImmSearchOp::setImplementer(SaUint32T conn, unsigned int nodeId,
    SaUint64T mds_dest)
{
     //TRACE_ENTER();
    SearchObject& obj = mResultList.back();
    obj.implConn = conn;
    obj.implNodeId = nodeId;
    obj.implDest = mds_dest;
    //TRACE_LEAVE();
}

SaAisErrorT
ImmSearchOp::testTopResult(unsigned int* nodeIdp, SaBoolT* bRtsToFetch)
{
    SaAisErrorT err = SA_AIS_ERR_NOT_EXIST;

    if (!mResultList.empty()) {
        SearchObject& obj = mResultList.front();
        err = SA_AIS_OK;
        *bRtsToFetch = SA_FALSE;

        // Check for pure runtime attribute
        AttributeList::iterator i;
        for (i = obj.attributeList.begin(); i != obj.attributeList.end(); i++) {
            if(bRtsToFetch && ((*i).flags & SA_IMM_ATTR_RUNTIME) &&
                            ! ((*i).flags & SA_IMM_ATTR_CACHED)) {
                *bRtsToFetch = SA_TRUE;
                *nodeIdp = obj.implNodeId;
                break;
            }
        }
    }

    return err;
}

SaAisErrorT
ImmSearchOp::nextResult(IMMSV_OM_RSP_SEARCH_NEXT** rsp, SaUint32T* connp, 
    unsigned int* nodeIdp,
    AttributeList** rtsToFetch,
    SaUint64T* implDest)
{
    SaAisErrorT err = SA_AIS_ERR_NOT_EXIST;
    if(!mRtsToFetch.empty()) {mRtsToFetch.clear();}
    
    if (!mResultList.empty()) {
        SearchObject& obj = mResultList.front();
        IMMSV_OM_RSP_SEARCH_NEXT* p =   (IMMSV_OM_RSP_SEARCH_NEXT*)
            calloc(1, sizeof(IMMSV_OM_RSP_SEARCH_NEXT));
        osafassert(p);
        *rsp = p;
        mLastResult = p; //Only used if there are runtime attributes to fetch.
        //the partially finished result then has to be picked-
        //up in a continuation and the rt attributes appended
        
        // Get object name
        p->objectName.size = (int)obj.name.length()+1;
        p->objectName.buf = strdup(obj.name.c_str());
        p->attrValuesList = NULL;
        
        // Get attribute values
        AttributeList::iterator i;
        for (i = obj.attributeList.begin(); i != obj.attributeList.end(); i++) {
            IMMSV_ATTR_VALUES_LIST* attrl = (IMMSV_ATTR_VALUES_LIST *)
                calloc(1, sizeof(IMMSV_ATTR_VALUES_LIST));
            IMMSV_ATTR_VALUES* attr = &(attrl->n);
            attr->attrName.size = (int)(*i).name.length()+1;
            attr->attrName.buf = strdup((*i).name.c_str());
            attr->attrValueType = (*i).valueType;
            
            if(rtsToFetch && ((*i).flags & SA_IMM_ATTR_RUNTIME) &&
                ! ((*i).flags & SA_IMM_ATTR_CACHED)) {
                //The non-cached rt-attr must in general be fetched
                mRtsToFetch.push_back(*i);
                mRtsToFetch.back().valuep=NULL;/*Unused & crashes destructor.*/
                *rtsToFetch = &mRtsToFetch;
                *connp = obj.implConn;
                *nodeIdp = obj.implNodeId;
                *implDest = obj.implDest;
            }
            
            if((*i).valuep) {
                //There is possibly a value for the attribute in the OI
                if(rtsToFetch && ((*i).flags & SA_IMM_ATTR_RUNTIME) &&
                    ! ((*i).flags & SA_IMM_ATTR_CACHED) &&
                    ! ((*i).flags & SA_IMM_ATTR_PERSISTENT)) {
                    //Dont set any value for non-cached and non-persistent
                    //runtime attributes, unless this is the local fetch
                    //of just those runtime attributes
                    attr->attrValuesNumber=0;
                } else if((*i).valuep->empty()) {
                    //Value fetched from the OI for the attribute is empty.
                    attr->attrValuesNumber=0;
                } else {
                    attr->attrValuesNumber = (*i).valuep->extraValues() + 1;
                    (*i).valuep->copyValueToEdu(&(attr->attrValue),
                        (SaImmValueTypeT) attr->attrValueType);
                    if(attr->attrValuesNumber > 1) {
                        osafassert((*i).valuep->isMultiValued());
                        ((ImmAttrMultiValue *)(*i).valuep)->
                            copyExtraValuesToEdu(&(attr->attrMoreValues),
                                (SaImmValueTypeT) attr->attrValueType);
                    }
                }
                delete (*i).valuep;
                (*i).valuep=NULL;
            } else {
                //There is no current value for the attribute
                attr->attrValuesNumber=0;
            }
            attrl->next = p->attrValuesList;
            p->attrValuesList = attrl;
        }
        
        mResultList.pop_front();
        err = SA_AIS_OK;
    } else {
        mLastResult=NULL;
    }
    return err;
}


SearchObject::~SearchObject()
{
    /* Not strictly necessary, but does not hurt. */
    attributeList.clear(); 
}

SearchAttribute::~SearchAttribute()
{
    delete valuep;
    valuep = NULL;
}
