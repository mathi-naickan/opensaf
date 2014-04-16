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

#define SQL_STMT_SIZE		7

enum {
	SQL_SEL_ATTR_DEF = 0,
	SQL_SEL_ATTR_DFLT_INT,
	SQL_SEL_ATTR_DFLT_TEXT,
	SQL_SEL_ATTR_DFLT_REAL,
	SQL_SEL_OBJECT_INT_MULTI,
	SQL_SEL_OBJECT_TEXT_MULTI,
	SQL_SEL_OBJECT_REAL_MULTI
};

static const char *preparedSql[] = {
		"SELECT attr_type, attr_flags, attr_name FROM attr_def WHERE class_id = ?",
		"SELECT int_dflt FROM attr_dflt WHERE class_id = ? AND attr_name = ?",
		"SELECT text_dflt FROM attr_dflt WHERE class_id = ? AND attr_name = ?",
		"SELECT real_dflt FROM attr_dflt WHERE class_id = ? AND attr_name = ?",
		"SELECT int_val FROM objects_int_multi WHERE obj_id = ? AND attr_name = ?",
		"SELECT text_val FROM objects_text_multi WHERE obj_id = ? AND attr_name = ?",
		"SELECT real_val FROM objects_real_multi WHERE obj_id = ? AND attr_name = ?"
};

static sqlite3_stmt *preparedStmt[SQL_STMT_SIZE] = { NULL };


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

	LOG_IN("PBE repository of rep-version <%u, %u>",
		pbe_ver_major, pbe_ver_minor);

	if(pbe_ver_major > 1) {
		LOG_ER("Can not handle rep-version with major version %u greater than 1", pbe_ver_major);
		goto load_from_xml_file;
	}

	sqlite3_free_table(result);
	
	TRACE_2("Prepare SQL statements");
	for(int i=0; i<SQL_STMT_SIZE; i++) {
		rc = sqlite3_prepare_v2((sqlite3 *)dbHandle, preparedSql[i], -1, &(preparedStmt[i]), NULL);
		if(rc != SQLITE_OK) {
			LOG_ER("Failed to prepare SQL statement: %s", preparedSql[i]);
			goto load_from_xml_file;
		}
	}

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
	sqlite3_stmt *stmt;
	sqlite3_stmt *stmtDflt = NULL;
	int rc=0;
	int ncols=0;
	int r,c;
	std::list<SaImmAttrDefinitionT_2> attrDefs;
	std::string attrName;
	std::list<SaImmAttrDefinitionT_2>::iterator it;
	TRACE_ENTER2("Loading class %s from PBE", className);

	stmt = preparedStmt[SQL_SEL_ATTR_DEF];
	rc = sqlite3_bind_int(stmt, 1, atoi(class_id));
	if(rc != SQLITE_OK) {
		LOG_ER("Failed to bind class_id");
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_ROW && rc != SQLITE_DONE) {
		LOG_IN("Could not access table 'attr_def', error:%s", sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	TRACE_2("Successfully accessed 'attr_def' table.");

	r = 0;
	ncols = sqlite3_column_count(stmt);
	while(rc == SQLITE_ROW) {
		attrName.clear();
		SaImmValueTypeT attrValueType= (SaImmValueTypeT) 0;
		SaImmAttrFlagsT attrFlags = 0LL;
		SaImmAttrValueT attDflt = NULL;
		char buf[32];
		snprintf(buf, 32, "Row(%u): <", ++r);
		std::string rowStr(buf);
		AttrInfo * attr_info=NULL;
		char *val;
		for(c=0;c<ncols;c++) {
			if((val = (char *)sqlite3_column_text(stmt, c))) {
				rowStr.append("'");
				rowStr.append(val);
				rowStr.append("' ");
			} else
				rowStr.append("(null) ");
		}
		rowStr.append(">");
		TRACE_1("           ABT: %s", rowStr.c_str());
		attrValueType = (SaImmValueTypeT)sqlite3_column_int(stmt, 0);
		attrFlags = (SaImmAttrFlagsT)sqlite3_column_int64(stmt, 1);
		attrName.append((char *)sqlite3_column_text(stmt, 2));


		if(!(attrFlags & SA_IMM_ATTR_INITIALIZED)) {
			/*Get any attribute default.*/
			switch(attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T:
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET: // Int64T
					stmtDflt = preparedStmt[SQL_SEL_ATTR_DFLT_INT];
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					stmtDflt = preparedStmt[SQL_SEL_ATTR_DFLT_TEXT];
					break;

				case SA_IMM_ATTR_SAFLOATT:
				case SA_IMM_ATTR_SADOUBLET:
					stmtDflt = preparedStmt[SQL_SEL_ATTR_DFLT_REAL];
					break;
				default:
					LOG_ER("BAD VALUE TYPE");
					goto bailout;
			}

			if(sqlite3_bind_int(stmtDflt, 1, atoi(class_id)) != SQLITE_OK) {
				LOG_ER("Failed to bind class_id");
				goto bailout;
			}
			if(sqlite3_bind_text(stmtDflt, 2, attrName.c_str(), -1, NULL) != SQLITE_OK) {
				LOG_ER("Failed to bind attr_name");
				goto bailout;
			}

			rc = sqlite3_step(stmtDflt);
			if(rc == SQLITE_DONE) {
				TRACE("ABT No default for %s", attrName.c_str());
			} else if(rc != SQLITE_ROW) {
				LOG_IN("Could not access table 'attr_dflt', error:%s", sqlite3_errmsg(dbHandle));
				goto bailout;
			} else {
				val = (char *)sqlite3_column_text(stmtDflt, 0);
				if(val) {
					attDflt = malloc(strlen(val) + 1);
					strcpy((char *)attDflt, (char *)val);

					rc = sqlite3_step(stmtDflt);
					if(rc == SQLITE_ROW) {
						free(attDflt);
						LOG_ER("Expected 1 row got more then 1 row");
						goto bailout;
					} else {
						TRACE("ABT Default value for %s is %s",
								attrName.c_str(), (char *) attDflt);
					}
				} else {
					TRACE("ABT No default for %s", attrName.c_str());
				}
			}

			sqlite3_reset(stmtDflt);
			stmtDflt = NULL;
		}

		attr_info = new AttrInfo;
		attr_info->attrName = attrName;
		attr_info->attrValueType = attrValueType;
		attr_info->attrFlags = attrFlags;
		class_info->attrInfoVector.push_back(attr_info);

		addClassAttributeDefinition((SaImmAttrNameT)strdup(attrName.c_str()), attrValueType, attrFlags,
			attDflt, &attrDefs);

		if(attDflt) {
			free(attDflt);
			attDflt = NULL;
		}

		/* move to the next database result */
		rc = sqlite3_step(stmt);
	}

	if (!createImmClass(immHandle, (char *) className, classCategory, &attrDefs)) {
		LOG_ER("Failed to create IMM class");
		goto bailout;
	}

	sqlite3_reset(stmt);

	TRACE_LEAVE();
	return true;

bailout:
	if(stmtDflt)
		sqlite3_reset(stmtDflt);
	sqlite3_reset(stmt);
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
		class_info->class_category = class_category;
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
	sqlite3_stmt *stmt = NULL;
	int rc=0;
	char **resultF=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	int c;
	std::string sqlF("select \"");
	bool attr_appended = false;
	std::list<SaImmAttrValuesT_2> attrValuesList;
	AttrInfoVector::iterator it;
	int obj_id;

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
			if((strcmp(resultF[c], "SaImmAttrImplementerName") == 0) && 
					(class_info->class_category == SA_IMM_CLASS_CONFIG))
				continue;
					
			addObjectAttributeDefinition((char *) 
				class_info->className.c_str(), 
				resultF[c], &attrValueBuffers, 
				attrType, &attrValuesList);
		}
	}
	sqlite3_free_table(resultF);

	/* Now add any multivalued attributes. */

	obj_id = atoi(object_id);
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
			switch((*it)->attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T:
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET: // Int64T
					stmt = preparedStmt[SQL_SEL_OBJECT_INT_MULTI];
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					stmt = preparedStmt[SQL_SEL_OBJECT_TEXT_MULTI];
					break;

				case SA_IMM_ATTR_SAFLOATT:
				case SA_IMM_ATTR_SADOUBLET:
					stmt = preparedStmt[SQL_SEL_OBJECT_REAL_MULTI];
					break;
				default:
					LOG_ER("BAD VALUE TYPE");
					goto bailout;
			}

			if(sqlite3_bind_int(stmt, 1, obj_id) != SQLITE_OK) {
				LOG_ER("Failed to bind obj_id, %d", __LINE__);
				goto bailout;
			}
			if(sqlite3_bind_text(stmt, 2, (*it)->attrName.c_str(), -1, NULL) != SQLITE_OK) {
				LOG_ER("Failed to bind attr_name at: %d", __LINE__);
				goto bailout;
			}

			rc = sqlite3_step(stmt);
			if(rc != SQLITE_DONE && rc != SQLITE_ROW) {
				LOG_IN("Could not access table multi table, error:%s", sqlite3_errmsg(dbHandle));
				goto bailout;
			}

			std::list<char*> attrValueBuffers;
			char *val;
			while(rc == SQLITE_ROW) {
				val = (char *)sqlite3_column_text(stmt, 0);
				if(val) {
					/* Guard for NULL values. */
					char *str = strdup(val);
					attrValueBuffers.push_front(str);
					TRACE("ABT pushed value:%s", str);
				}

				rc = sqlite3_step(stmt);
			}

			if(attrValueBuffers.size() > 0)
				addObjectAttributeDefinition((char *)
						class_info->className.c_str(),
						(char *) (*it)->attrName.c_str(),
						&attrValueBuffers,
						(*it)->attrValueType,
						&attrValuesList);

			sqlite3_reset(stmt);
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
	if(stmt)
		sqlite3_reset(stmt);
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


int loadImmFromPbe(void* pbeHandle, bool preload)
{
	SaVersionT             version = {'A', 2, 13};
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

	errorCode = saImmOmAdminOwnerInitialize(immHandle, (char *) "IMMLOADER", 
		SA_FALSE, &ownerHandle);
	if (errorCode != SA_AIS_OK) {
		LOG_ER("Failed on saImmOmAdminOwnerInitialize %d", errorCode);
		exit(1);
	}

        if(preload) { /* Break out this into a separate function. */
               SaUint32T epoch=0;
               SaUint32T maxCcbId=0;
               SaUint32T maxCommitTime=0;
               SaUint64T maxWeakCcbId=0;
               SaUint32T maxWeakCommitTime=0;
               std::string sqlEpoch("SELECT opensafImmEpoch FROM OpensafImm");
               std::string sqlLowCommits("SELECT * FROM ccb_commits WHERE ccb_id < 4294967297");
               std::string sqlCcbId("SELECT MAX(ccb_id) FROM ccb_commits WHERE ccb_id < 4294967297");
               std::string sqlCommitTime("SELECT MAX(commit_time) FROM ccb_commits WHERE ccb_id < 4294967297");	
               std::string sqlHighCommits("SELECT * FROM ccb_commits WHERE ccb_id > 4294967297");
               std::string sqlWeakCcbId("SELECT MAX(ccb_id) FROM ccb_commits WHERE ccb_id > 4294967296");
               std::string sqlWeakCommitTime("SELECT MAX(commit_time) FROM ccb_commits WHERE ccb_id > 4294967296");	

               char **result=NULL;
               char *qErr=NULL;
               int nrows=0;
               int ncols=0;

	       /* Get latest epoch from local imm.db */
	       rc = sqlite3_get_table(dbHandle, sqlEpoch.c_str(), &result, &nrows, &ncols, &qErr);
	       if(rc) {
		       LOG_ER("SQL statement ('%s') failed because:\n %s", sqlEpoch.c_str(), qErr);
		       sqlite3_free(qErr);
		       goto bailout;
	       }

	       if(nrows != 1) {
		       LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
		       goto bailout;
	       }

	       epoch = strtoul(result[ncols], NULL, 0);
	       TRACE("Preload: epoch:%u", epoch);

	       sqlite3_free_table(result);

	       /* Check if any low (regular) commit records exist */
	       rc = sqlite3_get_table(dbHandle, sqlLowCommits.c_str(), &result, &nrows, &ncols, &qErr);
	       if(rc) {
		       LOG_ER("SQL statement ('%s') failed because:\n %s", sqlLowCommits.c_str(), qErr);
		       sqlite3_free(qErr);
		       goto bailout;
	       }

	       sqlite3_free_table(result);

	       if(nrows == 0) {
		       LOG_NO("No regular ccb commit records found");
	       } else {
		       TRACE("Preload: We have %u low (regular) ccb commit records", nrows);
		       /* Get max ccb-id from local imm.db */
		       rc = sqlite3_get_table(dbHandle, sqlCcbId.c_str(), &result, &nrows, &ncols, &qErr);
		       if(rc) {
			       LOG_ER("SQL statement ('%s') failed because:\n %s", sqlCcbId.c_str(), qErr);
			       sqlite3_free(qErr);
			       goto bailout;
		       }

		       if(nrows != 1) {
			       LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
			       goto bailout;
		       }

		       maxCcbId = strtoul(result[ncols], NULL, 0);
		       LOG_NO("Preload: maxCcbId:%u", maxCcbId);
		       sqlite3_free_table(result);
		       sleep(1);

		       /* Get max commit-time from local imm.db */
		       rc = sqlite3_get_table(dbHandle, sqlCommitTime.c_str(), &result, &nrows, 
			       &ncols, &qErr);
		       if(rc) {
			       LOG_ER("SQL statement ('%s') failed because:\n %s", 
				       sqlCommitTime.c_str(), qErr);
			       sqlite3_free(qErr);
			       goto bailout;
		       }

		       if(nrows != 1) {
			       LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
			       goto bailout;
		       }

		       maxCommitTime = strtoul(result[ncols], NULL, 0);
		       TRACE("Preload: max commit-time:%u", maxCommitTime);
		       sqlite3_free_table(result);
	       }

	       /* Check if any high (prto/class) commit records exist */
	       rc = sqlite3_get_table(dbHandle, sqlHighCommits.c_str(), &result, &nrows, &ncols, &qErr);
	       if(rc) {
		       LOG_ER("SQL statement ('%s') failed because:\n %s", sqlHighCommits.c_str(), qErr);
		       sqlite3_free(qErr);
		       goto bailout;
	       }

	       sqlite3_free_table(result);

	       if(nrows == 0) {
		       LOG_NO("No high commit records found");
	       } else {
		       /* Get max weak ccb-id from local imm.db (PRTops, class-create/delete epoch-update) */
		       rc = sqlite3_get_table(dbHandle, sqlWeakCcbId.c_str(), &result, &nrows, &ncols, &qErr);
		       if(rc) {
			       LOG_ER("SQL statement ('%s') failed because:\n %s", sqlWeakCcbId.c_str(), qErr);
			       sqlite3_free(qErr);
			       goto bailout;
		       }

		       if(nrows != 1) {
			       LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
			       goto bailout;
		       }

		       maxWeakCcbId = strtoull(result[ncols], NULL, 0);
		       TRACE("Preload: maxWeakCcbId:%llu", maxWeakCcbId);
		       sqlite3_free_table(result);

		       /* Get max weak commit-time from local imm.db (PRTops, class-create/delete epoch-update) */
		       rc = sqlite3_get_table(dbHandle, sqlWeakCommitTime.c_str(), &result, &nrows, 
			       &ncols, &qErr);
		       if(rc) {
			       LOG_ER("SQL statement ('%s') failed because:\n %s", 
				       sqlWeakCommitTime.c_str(), qErr);
			       sqlite3_free(qErr);
			       goto bailout;
		       }

		       if(nrows != 1) {
			       LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
			       goto bailout;
		       }
		       maxWeakCommitTime = strtoul(result[ncols], NULL, 0);
		       TRACE("Preload: max weak commit-time:%u", maxWeakCommitTime);
		       sqlite3_free_table(result);
		       sleep(1);
	       }

               rc = sqlite3_exec(dbHandle, commitT, NULL, NULL, &execErr);

               if (rc != SQLITE_OK) {
                       LOG_ER("SQL statement ('%s') failed because:\n %s",
                               commitT, execErr);
                       sqlite3_free(execErr);
                       goto bailout;
               }
               TRACE_2("Successfully executed %s", commitT);

               sqlite3_close(dbHandle);

	       sendPreloadParams(immHandle, ownerHandle, epoch, maxCcbId, maxCommitTime, 
		       maxWeakCcbId, maxWeakCommitTime);
	       goto done;
        }

	/*Fetch classes from PBE and create in IMM */
	if(!loadClassesFromPbe(pbeHandle, immHandle, &classInfoMap)) {
		goto bailout;
	}

	/*Fetch objects from PBE and create in IMM */
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

	errorCode = saImmOmClassDelete(immHandle, (char *) OPENSAF_IMM_PBE_RT_CLASS_NAME);
	if((errorCode != SA_AIS_OK) && (errorCode != SA_AIS_ERR_NOT_EXIST)) {
		int retryCount = 0;
		do {
			if(errorCode != SA_AIS_OK) {
				LOG_WA("Delete of class %s returned errorCode:%u - unexpected",
					(char *) OPENSAF_IMM_PBE_RT_CLASS_NAME, errorCode);
			}
			errorCode = saImmOmClassDelete(immHandle, (char *) OPENSAF_IMM_PBE_RT_CLASS_NAME);
			if((errorCode != SA_AIS_OK) && (errorCode != SA_AIS_ERR_NOT_EXIST)) {
				LOG_WA("Delete of class %s returned errorCode:%u - unexpected",
					(char *) OPENSAF_IMM_PBE_RT_CLASS_NAME, errorCode);
				usleep(200000);
				++retryCount;
			}
		} while((errorCode == SA_AIS_ERR_TRY_AGAIN) && (retryCount < 32));

		if((errorCode != SA_AIS_OK) && (errorCode!=SA_AIS_ERR_NOT_EXIST)) {
			LOG_ER("Delete of class %s FAILED with errorCode:%u",
				(char *) OPENSAF_IMM_PBE_RT_CLASS_NAME, errorCode);
			goto bailout;
		}
	}

	if(errorCode == SA_AIS_OK) {
		LOG_IN("Removed class %s", (char *) OPENSAF_IMM_PBE_RT_CLASS_NAME);
	} else if(errorCode == SA_AIS_ERR_NOT_EXIST) {
		LOG_IN("Class %s did not exist", (char *) OPENSAF_IMM_PBE_RT_CLASS_NAME);
	}

	if(!opensafPbeRtClassCreate(immHandle)) {
		/* Error already logged. */
		goto bailout;
	}

 done:
	sqlite3_close(dbHandle);
	if(preload) {exit(0);} /* dont delay termination of preloader.*/
	saImmOmAdminOwnerFinalize(ownerHandle);
	saImmOmFinalize(immHandle);
	TRACE_LEAVE();
	return 1;

 bailout:
TRACE("ABT bailout reached");
	sqlite3_close(dbHandle);
	return 0;
}

#else

void* checkPbeRepositoryInit(std::string dir, std::string file)
{
	TRACE_ENTER2("Not enabled");
	LOG_WA("osafimmloadd not built with the --enable-imm-pbe option");
	TRACE_LEAVE();
	return NULL;
}

int loadImmFromPbe(void* pbeHandle, bool preload)
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
