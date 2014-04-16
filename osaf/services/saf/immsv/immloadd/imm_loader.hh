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
#include <list>
#include <map>
#include <vector>
#include <string>
#include <string.h>
#include <saImmOm.h>
#include <immsv_api.h>
#include <saAis.h>

struct AttrInfo
{
	std::string attrName;
	SaImmValueTypeT attrValueType;
	SaImmAttrFlagsT attrFlags;
};

typedef std::vector<AttrInfo*> AttrInfoVector;

struct ClassInfo
{
	std::string className;
	AttrInfoVector attrInfoVector;
	SaImmClassCategoryT class_category;
};

typedef	std::map<std::string, ClassInfo*>  ClassInfoMap;

/* Prototypes */

/* Helper functions */
void addClassAttributeDefinition(SaImmAttrNameT attrName, 
	SaImmValueTypeT attrValueType,
	SaImmAttrFlagsT attrFlags,
	SaImmAttrValueT attrDefaultValueBuffer,
	std::list<SaImmAttrDefinitionT_2> *attrDefinitions);

void addObjectAttributeDefinition(SaImmClassNameT objectClass,
	SaImmAttrNameT attrName, std::list<char*> *attrValueBuffers,
	SaImmValueTypeT attrType,
	std::list<SaImmAttrValuesT_2> *attrValuesList);

bool createImmClass(SaImmHandleT immHandle,
	const SaImmClassNameT className, 
	SaImmClassCategoryT classCategory,
	std::list<SaImmAttrDefinitionT_2>* attrDefinitions);

bool createImmObject(SaImmClassNameT className,
	char * objectName,
	std::list<SaImmAttrValuesT_2> *attrValuesList,
	SaImmCcbHandleT ccbHandle,
	std::map<std::string, SaImmAttrValuesT_2> *classRDNMap);


void escalatePbe(std::string dir, std::string file);

void* checkPbeRepositoryInit(std::string dir, std::string file);

int loadImmFromPbe(void* pbeHandle, bool preload);

void sendPreloadParams(SaImmHandleT immHandle, SaImmAdminOwnerHandleT ownerHandle, 
	SaUint32T epoch, SaUint32T maxCcbId, SaUint32T maxCommitTime,
	SaUint64T maxWeakCcbId, SaUint32T maxWeakCommitTime);

bool opensafPbeRtClassCreate(SaImmHandleT immHandle);
