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

#ifndef _ImmSearchOp_hh_
#define _ImmSearchOp_hh_ 1

#include "saImmOm.h"
#include "ImmAttrValue.hh"
#include <string>
#include <list>


struct SearchAttribute
{
    SearchAttribute(const std::string& attributeName) : name(attributeName),
                                                        valuep(NULL), flags(0)
    { valueType = (SaImmValueTypeT) 0;}
    std::string name;
    ImmAttrValue*     valuep;
    SaImmValueTypeT  valueType;
    SaImmAttrFlagsT flags;

    ~SearchAttribute();
};
typedef std::list<SearchAttribute> AttributeList;

struct SearchObject
{
    SearchObject(const std::string& objectName) : name(objectName),
                                                  implConn(0), implNodeId(0), implDest(0LL) { }
    std::string name;
    AttributeList attributeList;
    SaUint32T implConn;
    unsigned int implNodeId;
    SaUint64T implDest;

    ~SearchObject();
};
typedef std::list<SearchObject> ResultList;


class ImmSearchOp
{
public:
    ImmSearchOp();
    ~ImmSearchOp();
    
    void          addObject(const std::string& objectName);
    void          addAttribute(
                               const std::string& attributeName, 
                               SaUint32T valueType,
                               SaImmAttrFlagsT flags);
    void          addAttrValue(const ImmAttrValue& value);
    void          setImplementer(
                                 SaUint32T conn, 
                                 unsigned int nodeId,
                                 SaUint64T mds_dest);

    SaAisErrorT   testTopResult(
                                unsigned int* nodeIdp,
                                SaBoolT* bRtsToFetch);

    SaAisErrorT   nextResult(
                             IMMSV_OM_RSP_SEARCH_NEXT** rsp,
                             SaUint32T* connp,
                             unsigned int* nodeIdp,
                             AttributeList** rtsToFetch,
                             SaUint64T* implDest);
    
    IMMSV_OM_RSP_SEARCH_NEXT*
    fetchLastResult() {return mLastResult;}
    void          clearLastResult() {mLastResult = NULL;}
    void          setIsSync() {mIsSync = true;}
    void          setIsAccessor() {mIsAccessor = true;}
    bool          isSync() {return mIsSync;}
    bool          isAccessor() {return mIsAccessor;}
    void*         syncOsi;
    void*         attrNameList;
    void*         classInfo;
private:
    ResultList    mResultList;
    IMMSV_OM_RSP_SEARCH_NEXT* mLastResult;//only used to save result during
    //fetching of runtime attribute values.
    AttributeList mRtsToFetch;
    bool mIsSync;
    bool mIsAccessor;
};

#endif
