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

#include <string>
#include <list>
#include <map>
#include <cstring>
#include <saImmOm.h>
#include <immsv_api.h>
#include <logtrace.h>

#define RELEASE_CODE 'A'
#define MAJOR_VERSION 2
#define MINOR_VERSION 1

/* Prototypes */
typedef std::map<std::string, SaImmAttrFlagsT> AttrMap;
struct ClassInfo
{
	ClassInfo(unsigned int class_id) : mClassId(class_id) { }
	~ClassInfo() { }
    
	unsigned int mClassId;
	AttrMap mAttrMap;
};
typedef std::map<std::string, ClassInfo*> ClassMap;

std::list<std::string> getClassNames(SaImmHandleT handle);
std::string getClassName(const SaImmAttrValuesT_2** attrs);
std::string valueToString(SaImmAttrValueT, SaImmValueTypeT);

void* pbeRepositoryInit(const char* filePath, bool create);
void pbeAtomicSwitchFile(const char* filePath);
void pbeRepositoryClose(void* dbHandle);
void dumpClassesToPbe(SaImmHandleT immHandle, ClassMap *classIdMap,
	void* db_handle);

ClassInfo* classToPBE(std::string classNameString, SaImmHandleT immHandle,
	void* db_handle, unsigned int class_id);

void deleteClassToPBE(std::string classNameString, void* db_handle, 
	ClassInfo* theClass);

unsigned int verifyPbeState(SaImmHandleT immHandle, ClassMap *classIdMap,
	void* db_handle);

unsigned int dumpObjectsToPbe(SaImmHandleT immHandle, ClassMap* classIdMap,
	void* db_handle);
void objectToPBE(std::string objectNameString, 
	const SaImmAttrValuesT_2** attrs,
	ClassMap* classIdMap, void* db_handle, unsigned int object_id,
	SaImmClassNameT className,
	SaUint64T ccbId);

void objectDeleteToPBE(std::string objectNameString, void* db_handle);

void pbeDaemon(SaImmHandleT immHandle, void* dbHandle, ClassMap* classIdMap,
	unsigned int objCount);

SaAisErrorT pbeBeginTrans(void* db_handle);
SaAisErrorT pbeCommitTrans(void* db_handle, SaUint64T ccbId, SaUint32T epoch);
void pbeAbortTrans(void* db_handle);

void purgeCcbCommitsFromPbe(void* sDbHandle, SaUint32T currentEpoch);

void objectModifyDiscardAllValuesOfAttrToPBE(void* dbHandle, 
	std::string objName, const SaImmAttrValuesT_2* modAttr,
	SaUint64T ccb_id);

void objectModifyAddValuesOfAttrToPBE(void* dbHandle, 
	std::string objName, const SaImmAttrValuesT_2* modAttr,
	SaUint64T ccb_id);
	
void objectModifyDiscardMatchingValuesOfAttrToPBE(void* dbHandle, 
	std::string objName, const SaImmAttrValuesT_2* modAttr,
	SaUint64T ccb_id);
	
void stampObjectWithCcbId(void* db_handle, const char* object_id, 
	SaUint64T ccb_id);
	
SaAisErrorT getCcbOutcomeFromPbe(void* db_handle, SaUint64T ccbId, SaUint32T epoch);
