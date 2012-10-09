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

#include "imm_loader.hh"
#include <stdio.h>
#include <logtrace.h>
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <cstdlib>
#include <unistd.h>
#include <stdint.h>
#include <sys/stat.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IMM_PBE

#include <sqlite3.h>

static unsigned int pbe_ver_major = 0;
static unsigned int pbe_ver_minor = 0;

void* checkPbeRepositoryInit(std::string dir, std::string file)
{
	SaImmRepositoryInitModeT rpi = (SaImmRepositoryInitModeT) 0;
	std::string filename;
	void* dbHandle=NULL;
	int rc=0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char * sql = "select saImmRepositoryInit from SaImmMngt";
	const char * sql2 = "select major,minor from pbe_rep_version";
	TRACE_ENTER();
	/* Build the filename */
	filename = dir;
	filename.append("/");
	filename.append(file);

	/* Before trying top open with sqlite3, check if the db file exists
	   and is writable. This avoids the sqlite3 default behavior of simply
	   succeeding with open and creating an empty db, when there is no db
	   file.
	*/

	if(access(filename.c_str(), R_OK | W_OK) == (-1)) {
		LOG_IN("File '%s' is not accessible for read/write, cause:%s", 
			filename.c_str(), strerror(errno));
		goto load_from_xml_file;
	} else {
		std::string journalFile(filename);
		journalFile.append("-journal");
		if(access(journalFile.c_str(), F_OK) != (-1)) {
			struct stat stat_buf;
			memset(&stat_buf, 0, sizeof(struct stat));
			if(stat(journalFile.c_str(), &stat_buf)==0) {
				if(stat_buf.st_size) {
					LOG_WA("Journal file %s of non zero size exists at "
						"open for loading => sqlite recovery",
						journalFile.c_str());
				} else {
					TRACE_2("Journal file %s exists and is empty at "
						"open for loading", journalFile.c_str());
				}
			} else {
				LOG_WA("Journal file %s exists but could not stat() at "
					"open for loading", journalFile.c_str());
			}
		}
	}

	rc = sqlite3_open(filename.c_str(), ((sqlite3 **) &dbHandle));
	if(rc) {
		LOG_ER("Can't open sqlite pbe file '%s', cause:%s", 
			filename.c_str(), sqlite3_errmsg((sqlite3 *) dbHandle));
		goto load_from_xml_file;
	} 
	TRACE_2("Successfully opened sqlite pbe file %s", filename.c_str());

	rc = sqlite3_get_table((sqlite3 *) dbHandle, sql, &result, &nrows, 
		&ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table SaImmMngt, error:%s", zErr);
		sqlite3_free(zErr);
		goto load_from_xml_file;
	}
	TRACE_2("Successfully accessed SaImmMngt table rows:%u cols:%u", 
		nrows, ncols);

	if(nrows == 0) {
		LOG_ER("SaImmMngt exists but is empty");
		goto load_from_xml_file;
	} else if(nrows > 1) {
		LOG_WA("SaImmMngt has %u tuples, should only be one - using first tuple", nrows);
	}

	rpi = (SaImmRepositoryInitModeT) atoi(result[1]);

	if(rpi == SA_IMM_INIT_FROM_FILE) {
		LOG_NO("saImmRepositoryInit: SA_IMM_INIT_FROM_FILE - falling back to loading from imm.xml");
		goto load_from_xml_file;
	} else if( rpi == SA_IMM_KEEP_REPOSITORY) {
		LOG_IN("saImmRepositoryInit: SA_IMM_KEEP_REPOSITORY - loading from repository");
	} else {
		LOG_ER("saImmRepositoryInit: Not a valid value (%u) - falling back to loading from imm.xml", rpi);
		goto load_from_xml_file;
	}

	sqlite3_free_table(result);

	/* Get PBE rep-version */

	rc = sqlite3_get_table((sqlite3 *) dbHandle, sql2, &result, &nrows, 
		&ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table 'pbe_rep_version', error:%s. Assuming version <0,0>", zErr);
		sqlite3_free(zErr);

	} else {
		TRACE_2("Successfully accessed table 'pbe_rep_version', rows:%u cols:%u", 
			nrows, ncols);
		if(nrows == 0) {
			LOG_ER("pbe_rep_version exists but is empty");
		} else {
			if(nrows > 1) {
				LOG_WA("pbe_rep_version has %u tuples, should "
					"only be one - using first tuple",
					nrows);
			}

			pbe_ver_major = atoi(result[2]);
			pbe_ver_minor = atoi(result[3]);
		}
	}

	sqlite3_free_table(result);
	
	LOG_IN("PBE repository of rep-version <%u, %u>",
		pbe_ver_major, pbe_ver_minor);

	TRACE_LEAVE();
	return dbHandle;

 load_from_xml_file:
	sqlite3_close((sqlite3 *) dbHandle);
	TRACE_LEAVE();
	return NULL;
}



bool loadClassFromPbe(void* pbeHandle, 
	SaImmHandleT immHandle,
	const char* className, 
	const char* class_id, 
	SaImmClassCategoryT classCategory,
	ClassInfo* class_info)
{
	sqlite3* dbHandle = (sqlite3 *) pbeHandle;
	std::string sql("select attr_type,attr_flags,attr_name from attr_def where class_id = ");
	int rc=0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	int r,c;
	std::list<SaImmAttrDefinitionT_2> attrDefs;
	TRACE_ENTER2("Loading class %s from PBE", className);
	sql.append(class_id);

	rc = sqlite3_get_table(dbHandle, sql.c_str(), &result, &nrows, &ncols,
		&zErr);
	if(rc) {
		LOG_IN("Could not access table 'attr_def', error:%s", zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	TRACE_2("Successfully accessed 'attr_def' table. Rows:%u cols:%u", 
		nrows, ncols);
	for(r=0; r<=nrows; ++r) {
		SaImmAttrNameT attrName=NULL;
		SaImmValueTypeT attrValueType= (SaImmValueTypeT) 0;
		SaImmAttrFlagsT attrFlags = 0LL;
		SaImmAttrValueT attDflt = NULL;
		char buf[32];
		snprintf(buf, 32, "Row(%u): <", r);
		std::string rowStr(buf);
		AttrInfo * attr_info=NULL;
		for(c=0;c<ncols;++c) {
			rowStr.append("'");
			rowStr.append(result[r*ncols+c]);
			rowStr.append("' ");
		}
		rowStr.append(">");
		TRACE_1("           ABT: %s", rowStr.c_str());
		if(r==0) {continue;}
		attrValueType = (SaImmValueTypeT) atoi(result[r*ncols]);
		attrFlags = atoll(result[r*ncols+1]);
		attrName = strdup(result[r*ncols+2]);

		if(!(attrFlags & SA_IMM_ATTR_INITIALIZED)) {
			/*Get any attribute default.*/
			char **result2=NULL;
			int nrows2=0;
			int ncols2=0;
			std::string sqlDflt("select ");
			size_t len = 0;
			switch(attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T:
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET: // Int64T
					sqlDflt.append("int_dflt from ");
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					sqlDflt.append("text_dflt from ");
					break;

				case SA_IMM_ATTR_SAFLOATT:
				case SA_IMM_ATTR_SADOUBLET:
					sqlDflt.append("real_dflt from ");
					break;
				default:
					LOG_ER("BAD VALUE TYPE");
					goto bailout;
			}

			sqlDflt.append("attr_dflt where class_id = ");
			sqlDflt.append(class_id);
			sqlDflt.append(" and attr_name = '");
			sqlDflt.append(attrName);
			sqlDflt.append("'");
			TRACE("GENERATED sqlDflt:%s", sqlDflt.c_str());

			rc = sqlite3_get_table(dbHandle, sqlDflt.c_str(), 
				&result2, &nrows2, &ncols2, &zErr);
			if(rc) {
				LOG_IN("Could not access table 'attr_dflt', error:%s", zErr);
				sqlite3_free(zErr);
				goto bailout;
			}
			TRACE_2("Successfully accessed 'attr_dflt' table. Rows:%u cols:%u", nrows2, ncols2);
			if(nrows2 && result2[1]) { 
				/*There is a default and it is not null */
				if(nrows2 > 1) {
					LOG_ER("Expected 1 row got %u rows", 
						nrows2);
					goto bailout;
				}
				if(ncols2 != 1) {
					LOG_ER("Expected 1 col got %u cols",
						ncols2);
					goto bailout;
				}
				len = strlen(result2[1]);
				attDflt = malloc(len + 1);
				strncpy((char *) attDflt, 
					(const char*) result2[1], len);
				((char *) attDflt)[len] = '\0';
				sqlite3_free_table(result2);
				TRACE("ABT Default value for %s is %s", 
					attrName, (char *) attDflt);
			} else {
				TRACE("ABT No default for %s", attrName);
			}
		}

		attr_info = new AttrInfo;
		attr_info->attrName = std::string(attrName);
		attr_info->attrValueType = attrValueType;
		attr_info->attrFlags = attrFlags;
		class_info->attrInfoVector.push_back(attr_info);

		addClassAttributeDefinition(attrName, attrValueType, attrFlags, 
			attDflt, &attrDefs);
		/*Free attrDefs, default*/
	}

	if (!createImmClass(immHandle, (char *) className, classCategory, &attrDefs)) {
		goto bailout;
	}

	sqlite3_free_table(result);
	TRACE_LEAVE();
	return true;

 bailout:
	sqlite3_free_table(result);
	TRACE_LEAVE();
	return false;
}

bool loadClassesFromPbe(void* pbeHandle, SaImmHandleT immHandle, ClassInfoMap* classInfoMap)
{
	sqlite3* dbHandle = (sqlite3 *) pbeHandle;	
	const char * sql = "select * from classes";
	int rc=0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	int r,c;
	TRACE_ENTER();
	assert(dbHandle);

	rc = sqlite3_get_table(dbHandle, sql, &result, &nrows, &ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table 'classes', error:%s", zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	TRACE_2("Successfully accessed 'classes' table. Rows:%u cols:%u",
		nrows, ncols);
	for(r=0; r<=nrows; ++r) {
		const char* class_id=NULL;
		SaImmClassCategoryT class_category = (SaImmClassCategoryT) 0;
		const char* class_name=NULL;
		ClassInfo* class_info =NULL;

		char buf[32];
		snprintf(buf, 32, "Row(%u): <", r);
		std::string rowStr(buf);
		for(c=0;c<ncols;++c) {
			rowStr.append("'");
			rowStr.append(result[r*ncols+c]);
			rowStr.append("' ");
		}
		rowStr.append(">");
		TRACE_1("ABT: %s", rowStr.c_str());

		if(r==0) {continue;}
		class_id = result[r*ncols];
		class_category = (SaImmClassCategoryT) atoi(result[r*ncols+1]);
		class_name = result[r*ncols+2];
		class_info = new ClassInfo;
		class_info->className = std::string(class_name);
		if(!loadClassFromPbe(pbeHandle, immHandle, class_name,
			   class_id, class_category, class_info))
		{
			sqlite3_free_table(result);
			goto bailout;
		}
		
		(*classInfoMap)[std::string(class_id)] = class_info;
	}

	sqlite3_free_table(result);

	TRACE_LEAVE();
	return true;

 bailout:
	sqlite3_close(dbHandle);
	return false;
}

bool loadObjectFromPbe(void* pbeHandle, SaImmHandleT immHandle, SaImmCcbHandleT ccbHandle,
	const char* object_id, ClassInfo* class_info, const char* dn)
{
	sqlite3* dbHandle = (sqlite3 *) pbeHandle;	
	int rc=0;
	char **resultF=NULL;
	char **resultG=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	int c;
	std::string sqlF("select \"");
	bool attr_appended = false;
	std::list<SaImmAttrValuesT_2> attrValuesList;
	AttrInfoVector::iterator it;

	TRACE_ENTER2("Loading object id(%s) dn(%s) class(%s)(#atts %zu) from PBE",
		object_id, dn, class_info->className.c_str(),
		class_info->attrInfoVector.size());

	/* First take care of the base tuple (single valued attributes). */
	it = class_info->attrInfoVector.begin();
	while(it != class_info->attrInfoVector.end()) {
		bool attr_is_multi=false;
		bool attr_is_pure_rt=false;

		if((*it)->attrFlags & SA_IMM_ATTR_MULTI_VALUE) { 
			attr_is_multi=true;
		}

		if(((*it)->attrFlags & SA_IMM_ATTR_RUNTIME) &&
			!((*it)->attrFlags & SA_IMM_ATTR_PERSISTENT)) { 
			attr_is_pure_rt=true;
		}

		if(!attr_is_multi && !attr_is_pure_rt) { 
			if(attr_appended) {
				sqlF.append(", \"");
			}

			sqlF.append((*it)->attrName);
			sqlF.append("\"");
			attr_appended = true;
		}

		++it;
	}

	sqlF.append(" from \"");
	sqlF.append(class_info->className);
	sqlF.append("\" where obj_id = ");
	sqlF.append(object_id);

	TRACE("GENERATED F:%s", sqlF.c_str());

	rc = sqlite3_get_table(dbHandle, sqlF.c_str(), &resultF, &nrows,
		&ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table '%s', error:%s",
			class_info->className.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	TRACE_2("Successfully accessed '%s' table. cols:%u",
		class_info->className.c_str(), ncols);

	for(c=0; c<ncols; ++c) {
		if(resultF[ncols+c]) {
			std::list<char*> attrValueBuffers;
			SaImmValueTypeT attrType = (SaImmValueTypeT) 0;
			size_t len = strlen(resultF[ncols+c]);
			char * str = (char *) malloc(len+1);
			strncpy(str, (const char *) resultF[ncols+c], len);
			str[len] = '\0';
			attrValueBuffers.push_front(str);
			it = class_info->attrInfoVector.begin();
			while(it != class_info->attrInfoVector.end()) {
				if((*it)->attrName == std::string(resultF[c]))
				{
					attrType = (*it)->attrValueType;
					break;
				}
				++it;
			}
			assert(it != class_info->attrInfoVector.end());
			addObjectAttributeDefinition((char *) 
				class_info->className.c_str(), 
				resultF[c], &attrValueBuffers, 
				attrType, &attrValuesList);
		}
	}
	sqlite3_free_table(resultF);

	/* Now add any multivalued attributes. */

	it = class_info->attrInfoVector.begin();
	while(it != class_info->attrInfoVector.end()) {
		bool attr_is_multi=false;
		bool attr_is_pure_rt=false;

		if((*it)->attrFlags & SA_IMM_ATTR_MULTI_VALUE) { 
			attr_is_multi=true;
		}

		if(((*it)->attrFlags & SA_IMM_ATTR_RUNTIME) &&
			!((*it)->attrFlags & SA_IMM_ATTR_PERSISTENT)) { 
			attr_is_pure_rt=true;
		}

		if(attr_is_multi && !attr_is_pure_rt) {
			std::string sqlG("select ");

			switch((*it)->attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T:
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET: // Int64T
					sqlG.append("int_val");
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					sqlG.append("text_val");
					break;

				case SA_IMM_ATTR_SAFLOATT:
				case SA_IMM_ATTR_SADOUBLET:
					sqlG.append("real_val");
					break;
				default:
					LOG_ER("BAD VALUE TYPE");
					goto bailout;
			}

			sqlG.append(" from ");

			switch((*it)->attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T:
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET: // Int64T
					sqlG.append("objects_int_multi ");
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					sqlG.append("objects_text_multi ");
					break;

				case SA_IMM_ATTR_SAFLOATT:
				case SA_IMM_ATTR_SADOUBLET:
					sqlG.append("objects_real_multi ");
					break;
				default:
					LOG_ER("BAD VALUE TYPE");
					goto bailout;
			}
			sqlG.append("where obj_id = ");
			sqlG.append(object_id);
			sqlG.append(" and attr_name = '");
			sqlG.append((*it)->attrName);
			sqlG.append("'");

			TRACE("GENERATED G:%s", sqlG.c_str());

			rc = sqlite3_get_table(dbHandle, sqlG.c_str(),
				&resultG, &nrows, &ncols, &zErr);
			if(rc) {
				LOG_IN("Could not access table multi table, error:%s", zErr);
				sqlite3_free(zErr);
				goto bailout;
			}
			if(ncols > 1) {
				LOG_ER("Expected 1 collumn, got %u", ncols);
				goto bailout;
			}
			TRACE_2("Successfully accessed multi table. rows:%u",
				nrows);
			if(nrows) {
				int r;
				std::list<char*> attrValueBuffers;
				for(r=1; r<=nrows; ++r) {
					if(resultG[r]) {
						/* Guard for NULL values. */
						size_t len = 
							strlen(resultG[r]);
						char * str =
							(char *) malloc(len+1);
						strncpy(str, (const char *) resultG[r], len);
						str[len] = '\0';
						attrValueBuffers.push_front(str);
						TRACE("ABT pushed value:%s", str);
					}
				}
				addObjectAttributeDefinition((char *) 
					class_info->className.c_str(), 
					(char *) (*it)->attrName.c_str(), 
					&attrValueBuffers, 
					(*it)->attrValueType,
					&attrValuesList);
			}
			sqlite3_free_table(resultG);
		}/*if(attr_is_multi && !attr_is_pure_rt)*/
		++it;
	}/*while*/


	if(!createImmObject((char *) class_info->className.c_str(), (char *) dn, 
		   &attrValuesList, ccbHandle, NULL))
	{
		LOG_NO("Failed to create object - exiting");
		goto bailout;
	}

	return true;

 bailout:
	//sqlite3_close(dbHandle);
	return false;
}

bool loadObjectsFromPbe(void* pbeHandle, SaImmHandleT immHandle,
	SaImmCcbHandleT ccbHandle, ClassInfoMap *classInfoMap)
{
	sqlite3* dbHandle = (sqlite3 *) pbeHandle;	
	const char * sql = "select * from objects";
	int rc=0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	int r,c;
	TRACE_ENTER();
	assert(dbHandle);

	rc = sqlite3_get_table(dbHandle, sql, &result, &nrows, &ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table 'objects', error:%s", zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	TRACE_2("Successfully accessed 'objects' table. Rows:%u cols:%u",
		nrows, ncols);

	for(r=0; r<=nrows; ++r) {
		const char* object_id=NULL;
		const char* class_id=NULL;
		const char* dn=NULL;
		ClassInfo* class_info=NULL;

		char buf[32];
		snprintf(buf, 32, "Row(%u): <", r);
		std::string rowStr(buf);
		for(c=0;c<ncols;++c) {
			rowStr.append("'");
			rowStr.append(result[r*ncols+c]);
			rowStr.append("' ");
		}
		rowStr.append(">");
		TRACE_1("ABT: %s", rowStr.c_str());
		
		if(r==0) {continue;}
		object_id = result[r*ncols];
		class_id =  result[r*ncols + 1];
		dn =        result[r*ncols + 2];
		class_info = (*classInfoMap)[std::string(class_id)];

		if(!loadObjectFromPbe(pbeHandle, immHandle, ccbHandle, object_id, 
			   class_info, dn)) {
			sqlite3_free_table(result);
			goto bailout;
		}
	}

	sqlite3_free_table(result);
	TRACE_LEAVE();
	return true;

 bailout:
	sqlite3_close(dbHandle);
	return false;
}


int loadImmFromPbe(void* pbeHandle)
{
	SaVersionT             version = {'A', 2, 1};
	SaImmHandleT           immHandle=0LL;
	SaImmAdminOwnerHandleT ownerHandle=0LL;
	SaImmCcbHandleT        ccbHandle=0LL;
	SaAisErrorT            errorCode;
	sqlite3* dbHandle = (sqlite3 *) pbeHandle;
	const char * beginT = "BEGIN EXCLUSIVE TRANSACTION";
	const char * commitT =   "COMMIT TRANSACTION";
	char *execErr=NULL;
	int rc=0;
	ClassInfoMap  classInfoMap;
	unsigned int retries=0;
	TRACE_ENTER();
	assert(dbHandle);

	do {
		rc = sqlite3_exec(dbHandle, beginT, NULL, NULL, &execErr);
		if (rc == SQLITE_BUSY) {
			LOG_WA("SQL statement ('%s') failed because:(%u)\n %s\n retrying",
				beginT, rc, execErr);
			sqlite3_free(execErr);
			usleep(500000);
		} else if(rc != SQLITE_OK) {
			LOG_WA("SQL statement ('%s') failed because:(%u)\n %s\n escalating",
				beginT, rc, execErr);
			sqlite3_free(execErr);
			goto bailout;
		}
	} while (rc == SQLITE_BUSY && ++retries < 40);

	if(rc != SQLITE_OK) {
		LOG_ER("Repeated tries to start sqlite loading transaction failed, database locked? rc(%d)", rc);
		/* In this case do not escalate the error, because that will assume the imm.db is corrupt.
		   The problem here is more likely a locking conflict on imm.db, meaning that this
		   loading attempt is being made while another user is concurrently claiming exclusive
		   use of the imm.db. This is likely to be caused by a poorly coordinated cluster restart.
		   The "other user" is likely to be a lingering PBE (on the other SC) or a lingering POSIX 
		   file lock in an NFS client cache. 
		 */
		sqlite3_close(dbHandle);
		exit(1);
	}

	errorCode = saImmOmInitialize(&immHandle, NULL, &version);
	if (SA_AIS_OK != errorCode) {
		LOG_ER("Failed to initialize the IMM OM interface (%d)",
			errorCode);
		goto bailout;
	}

	/*Fetch classes from PBE and create in IMM */
	if(!loadClassesFromPbe(pbeHandle, immHandle, &classInfoMap)) {
		goto bailout;
	}

	/*Fetch objects from PBE and create in IMM */
	errorCode = saImmOmAdminOwnerInitialize(immHandle, (char *) "IMMLOADER", 
		SA_FALSE, &ownerHandle);
	if (errorCode != SA_AIS_OK) {
		LOG_ER("Failed on saImmOmAdminOwnerInitialize %d", errorCode);
		exit(1);
	}

	errorCode = saImmOmCcbInitialize(ownerHandle, 0, &ccbHandle);
	if (errorCode != SA_AIS_OK) {
		LOG_ER("Failed to initialize ImmOmCcb %d", errorCode);
		exit(1);
	}

	if(!loadObjectsFromPbe(pbeHandle, immHandle, ccbHandle, &classInfoMap))
	{
		goto bailout;
	}

	rc = sqlite3_exec(dbHandle, commitT, NULL, NULL, &execErr);
	if (rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", 
			commitT, execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	TRACE_2("Successfully executed %s", commitT);

	errorCode = saImmOmCcbApply(ccbHandle);
	if (errorCode != SA_AIS_OK) {
		LOG_ER("Failed to APPLY ImmOmCcb %d", errorCode);
		exit(1);
	}

	TRACE_2("Successfully Applied the CCB ");
	saImmOmCcbFinalize(ccbHandle);
	saImmOmAdminOwnerFinalize(ownerHandle);
	saImmOmFinalize(immHandle);

	TRACE_LEAVE();
	return 1;

 bailout:
	sqlite3_close(dbHandle);
	return 0;
}

#else

void* checkPbeRepositoryInit(std::string dir, std::string file)
{
	TRACE_ENTER2("Not enabled");
	LOG_WA("immload not built with the --enable-imm-pbe option");
	TRACE_LEAVE();
	return NULL;
}

int loadImmFromPbe(void* pbeHandle)
{
	TRACE_ENTER2("Not enabled");
	TRACE_LEAVE();
	return 0;
}

#endif

/* Note: a version of this function exists as 'discardPbeFile()' in 
   imm_pbe_dump.cc */
void escalatePbe(std::string dir, std::string file)
{
	std::string filename;
	std::string newFilename;
	filename = dir;
	filename.append("/");
	filename.append(file);

	newFilename = filename;
	newFilename.append(".failed");

	if(rename(filename.c_str(), newFilename.c_str())!=0) {
		LOG_ER("Failed to rename %s to %s error:%s", 
			filename.c_str(), newFilename.c_str(),
			strerror(errno));
	} else {
		LOG_NO("Renamed %s to %s to prevent cyclic reload.", 
			filename.c_str(), newFilename.c_str());
	}

	/* No need to remove any journal file here. 
	 This is the loader. It must have resolved any old journal
	 file recovery at sqlite_open. The loader does not write
	 new transactions to the sqlite file and thus can not 
	 itself generate any new journal file. 
	*/
}
