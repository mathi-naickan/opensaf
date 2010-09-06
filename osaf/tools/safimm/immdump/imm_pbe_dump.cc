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

#include "imm_dumper.hh"
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IMM_PBE

#include <sqlite3.h> 
#define STRINT_BSZ 32



static void typeToPBE(SaImmAttrDefinitionT_2* p, 
	void* dbHandle, std::string * table_def)
{
	switch (p->attrValueType)
	{
		case SA_IMM_ATTR_SAINT32T:
		case SA_IMM_ATTR_SAUINT32T:
		case SA_IMM_ATTR_SAINT64T:
		case SA_IMM_ATTR_SAUINT64T:
		case SA_IMM_ATTR_SATIMET:
			table_def->append(" integer");
			break;

		case SA_IMM_ATTR_SAFLOATT:
		case SA_IMM_ATTR_SADOUBLET:
			table_def->append(" real");
			break;

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
			table_def->append(" text");
			break;

		default:
			TRACE_4("Unknown value type %u", p->attrValueType);
			exit(1);
	}
}

static void valuesToPBE(const SaImmAttrValuesT_2* p, 
	SaImmAttrFlagsT attrFlags, 
	const char *objIdStr, void* db_handle)
{
	/* Handle the multivalued case. */	
	char *execErr=NULL;
	int rc=0;
	std::string sqlG("INSERT INTO ");
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	TRACE_ENTER();
	assert(p->attrValues);
	assert(p->attrValuesNumber);
	assert(attrFlags & SA_IMM_ATTR_MULTI_VALUE);

	switch(p->attrValueType) {
		case SA_IMM_ATTR_SAINT32T: 
		case SA_IMM_ATTR_SAUINT32T: 
		case SA_IMM_ATTR_SAINT64T:  
		case SA_IMM_ATTR_SAUINT64T: 
		case SA_IMM_ATTR_SATIMET:   
			sqlG.append("objects_int_multi (obj_id, attr_name, int_val) values('");
			break;

		case SA_IMM_ATTR_SAFLOATT:  
		case SA_IMM_ATTR_SADOUBLET: 
			sqlG.append("objects_real_multi (obj_id, attr_name, real_val) values('");
			break;

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
			sqlG.append("objects_text_multi (obj_id, attr_name, text_val) values('");
			break;

		default:
			LOG_ER("Unknown value type %u", p->attrValueType);
			assert(0);
	}
	sqlG.append(objIdStr);
	sqlG.append("', '");
	sqlG.append(p->attrName);
	sqlG.append("', '");
	for (unsigned int i = 0; i < p->attrValuesNumber; i++)
	{
		std::string sqlG1(sqlG);
		sqlG1.append(valueToString(p->attrValues[i], p->attrValueType));
		sqlG1.append("')");
		TRACE("GENERATED G:%s", sqlG1.c_str());
		rc = sqlite3_exec(dbHandle, sqlG1.c_str(), NULL, NULL, &execErr);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sqlG1.c_str(),
				execErr);
			sqlite3_free(execErr);
			exit(1);
		}
	}
	TRACE_LEAVE();
}


/* mv the tempfile on to the regular db file name link.
   This will simultaneously/atomically remove the link for the
   old file. 
*/
void pbeAtomicSwitchFile(const char* filePath)
{
	std::string tmpFilename;
	tmpFilename.append(filePath);
	tmpFilename.append(".tmp"); 

	if(rename(tmpFilename.c_str(), filePath)!=0) {
		LOG_ER("Failed to rename %s to %s error:%s", 
			filePath, tmpFilename.c_str(), strerror(errno));
		exit(1);
	} else {
		TRACE("Renamed %s to %s.", tmpFilename.c_str(), filePath);
	}
}


void* pbeRepositoryInit(const char* filePath, bool create)
{
	int fd=(-1);
	sqlite3* dbHandle=NULL;
	std::string oldFilename;
	std::string tmpFilename;
	int rc=0;
	SaImmRepositoryInitModeT rpi = (SaImmRepositoryInitModeT) 0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char * sql = "select saImmRepositoryInit from SaImmMngt";

	const char * sql_tr[] = 
		{"BEGIN EXCLUSIVE TRANSACTION",
		 "CREATE TABLE classes (class_id integer primary key, class_category integer, class_name text)",
		 "CREATE UNIQUE INDEX classes_idx on classes (class_name)",

		 "CREATE TABLE attr_def(class_id integer, attr_type integer, attr_flags integer, attr_name text)",
		 "CREATE UNIQUE INDEX attr_def_idx on attr_def (class_id, attr_name)",

		 "CREATE TABLE attr_dflt(attr_name text, class_id integer, int_dflt integer, text_dflt text, "
		 "real_dflt real, blob_dflt blob)",
		 "CREATE UNIQUE INDEX attr_dflt_idx on attr_dflt (class_id, attr_name)",

		 "CREATE TABLE objects (obj_id integer primary key, class_id integer, dn text, last_ccb integer)",
		 "CREATE UNIQUE INDEX objects_idx on objects (dn)",

		 "CREATE TABLE objects_int_multi (obj_id integer, attr_name text, int_val integer, tuple_id integer primary key)",
		 "CREATE INDEX objects_int_multi_idx on objects_int_multi (obj_id, attr_name)",

		 "CREATE TABLE objects_text_multi (obj_id integer, attr_name text, text_val text, tuple_id integer primary key)",
		 "CREATE INDEX objects_text_multi_idx on objects_text_multi (obj_id, attr_name)",

		 "CREATE TABLE objects_real_multi (obj_id integer, attr_name text, real_val real, tuple_id integer primary key)",
		 "CREATE INDEX objects_real_multi_idx on objects_real_multi (obj_id, attr_name)",

		 "CREATE TABLE objects_blob_multi (obj_id integer, attr_name text, blob_val blob, tuple_id integer primary key)",
		 "CREATE INDEX objects_blob_multi_idx on objects_blob_multi (obj_id, attr_name)",

		 "CREATE TABLE ccb_commits(ccb_id integer, epoch integer, commit_time integer)",
		 "CREATE INDEX ccb_commits_idx on ccb_commits (ccb_id, epoch)",

		 "COMMIT TRANSACTION",
		 NULL
		};
	TRACE_ENTER();

	if(!create) {goto re_attach;}

	/* Create a fresh Pbe-repository by dumping current imm contents to tmpFilename. 
	   If old repository exists, then "save it" under additional oldFileName link.
	 */

	oldFilename.append(filePath);
	oldFilename.append(".prev"); 
	tmpFilename.append(filePath);
	tmpFilename.append(".tmp"); 


	/* Existing new file indicates a previous failure.
	   We should possible have some excalation mechanism here. */
	if(unlink(tmpFilename.c_str()) != 0) {
		TRACE_2("Failed to unlink %s  error:%s", 
			tmpFilename.c_str(), strerror(errno));
	}

	/* Also remove old old file. */
	if(unlink(oldFilename.c_str()) != 0) {
		TRACE_2("Failed to unlink %s  error:%s", 
			oldFilename.c_str(), strerror(errno));
	}

	/* Make an additional link to the current db file.
	   Instead of removing or overwriting the current db file
	   we create the tmpFilename on the side, then mv the
	   new file to the current file, automatically unlinking
	   the current file from the regular file name. The current
	   file will still have the oldFilename link.
	   Could mark with date, but that requires rotation. */
	if(link(filePath, oldFilename.c_str()) != 0) {
		LOG_WA("Failed to link %s to %s error:%s", 
			filePath, oldFilename.c_str(), strerror(errno));
	}

	rc = sqlite3_open(tmpFilename.c_str(), &dbHandle);
	if(rc) {
		LOG_ER("Can't open sqlite pbe file '%s', cause:%s", 
			tmpFilename.c_str(), sqlite3_errmsg(dbHandle));
		exit(1);
	} 
	TRACE_2("Successfully opened sqlite pbe file %s", tmpFilename.c_str());
		 

	/* Creating the schema. */
	for (int ix=0; sql_tr[ix]!=NULL; ++ix) {
		rc = sqlite3_exec(dbHandle, sql_tr[ix], NULL, NULL, &zErr);
		if (rc != SQLITE_OK) {
			LOG_ER("SQL statement %u/('%s') failed because:\n %s", ix, sql_tr[ix], zErr);
			sqlite3_free(zErr);
			goto bailout;
		}
		TRACE_2("Successfully executed %s", sql_tr[ix]);
	}

	TRACE_LEAVE();
	return (void *) dbHandle;

 re_attach:

	/* Re-attach to an already created pbe file.
	   Before trying to open with sqlite3, check if the db file exists
	   and is writable. This avoids the sqlite3 default behavior of simply
	   succeeding with open and creating an empty db, when there is no db
	   file.
	*/
	fd = open(filePath, O_RDWR);
	if(fd == (-1)) {
		LOG_ER("File '%s' is not accessible for read/write, cause:%s - exiting", 
			filePath, strerror(errno));
		goto bailout;
	}

	close(fd);
	fd=(-1);

	rc = sqlite3_open(filePath, &dbHandle);
	if(rc) {
		LOG_ER("Can't open sqlite pbe file '%s', cause:%s", 
			filePath, sqlite3_errmsg(dbHandle));
		goto bailout;
	} 
	TRACE_2("Successfully opened sqlite pbe file %s", filePath);

	rc = sqlite3_get_table((sqlite3 *) dbHandle, sql, &result, &nrows, 
		&ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table SaImmMngt, error:%s", zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	TRACE_2("Successfully accessed SaImmMngt table rows:%u cols:%u", 
		nrows, ncols);

	if(nrows == 0) {
		LOG_ER("SaImmMngt exists but is empty");
		goto bailout;
	} else if(nrows > 1) {
		LOG_WA("SaImmMngt has %u tuples, should only be one - using first tuple", nrows);
	}

	rpi = (SaImmRepositoryInitModeT) strtoul(result[1], NULL, 0);

	if( rpi == SA_IMM_KEEP_REPOSITORY) {
		LOG_IN("saImmRepositoryInit: SA_IMM_KEEP_REPOSITORY - attaching to repository");
	} else if(rpi == SA_IMM_INIT_FROM_FILE) {
		LOG_WA("saImmRepositoryInit: SA_IMM_INIT_FROM_FILE - will not attach!");
		goto bailout;
	} else {
		LOG_ER("saImmRepositoryInit: Not a valid value (%u) - can not attach", rpi);
		goto bailout;
	}

	sqlite3_free_table(result);
	TRACE_LEAVE();
	return dbHandle;

 bailout:
	/* TODO: remove imm.db file */
	if(dbHandle) {
		sqlite3_close(dbHandle);
	}
	TRACE_LEAVE();
	return NULL;
}

void pbeRepositoryClose(void* dbHandle)
{
	sqlite3_close((sqlite3 *) dbHandle);
}

ClassInfo* classToPBE(std::string classNameString,
	SaImmHandleT immHandle,
	void* db_handle,
	unsigned int class_id)
{
	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;
	SaAisErrorT errorCode;
	bool class_is_pure_runtime=true; /* true => no persistent rtattrs. */
	char classIdStr[STRINT_BSZ];
	char attrPropStr[STRINT_BSZ];
	int rc=0;
	char *execErr=NULL;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	ClassInfo* classInfo = new ClassInfo(class_id);
	TRACE_ENTER();

	std::string sqlA("INSERT INTO classes (class_id, class_name, class_category) values('");
	std::string sqlB;
	unsigned int sqlBsize = 2048;  /* There appears to be a bug in the version of 
					  std:string I am using that is provoked (segv)
					  when the string expands beyond aproximately
					  1K in size.
					  Preallocating large space to avoid this. 
					  This is also more efficient.
				       */
	std::string sqlC("INSERT INTO attr_def (class_id, attr_name, attr_type, attr_flags) values('");

	std::string sqlD("INSERT INTO attr_dflt (class_id, attr_name, ");

	/* Get the class description */
	errorCode = saImmOmClassDescriptionGet_2(immHandle,
		(char*)classNameString.c_str(),
		&classCategory,
		&attrDefinitions);

	if (SA_AIS_OK != errorCode)
	{
		TRACE_4("Failed to get the description for the %s class error:%u - exiting",
			classNameString.c_str(), errorCode);
		exit(1);
	}

	snprintf(classIdStr, STRINT_BSZ, "%u", class_id);
	sqlA.append(classIdStr);
	sqlA.append("', '");
	sqlA.append(classNameString);
	sqlA.append("', '");
	if(classCategory == SA_IMM_CLASS_CONFIG) {
		sqlA.append("1');");
		class_is_pure_runtime=false; /* true => no persistent rtattrs. */
	} else if (classCategory == SA_IMM_CLASS_RUNTIME) {
		TRACE("RUNTIME CLASS %s", (char*)classNameString.c_str());
		sqlA.append("2');");
	} else {
		LOG_ER("Class category %u for class %s is neither CONFIG nor RUNTIME",
			classCategory, classNameString.c_str());
		goto bailout;
	}

	TRACE("GENERATED A class tuple: (%s)", sqlA.c_str());

	rc = sqlite3_exec(dbHandle, sqlA.c_str(), NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlA.c_str(),
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	sqlA.resize(0); /* sqlA not used any more. */
	sqlB.reserve(sqlBsize);
	sqlB.append("CREATE TABLE ");
	sqlB.append(classNameString);
	sqlB.append("(obj_id integer primary key");
	sqlC.append(classIdStr);
	sqlC.append("', '");

	/* Attributes needed both in class def tuple and in instance relations. */
	for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
	{
		bool attr_is_pure_rt=false; /* This attribute is pure runtime?*/
		bool attr_is_multi=false;
		std::string sqlC2(sqlC);

		classInfo->mAttrMap[(*p)->attrName] = (*p)->attrFlags;
		TRACE("DUMPED Class %s Attr%s Flags %llx", classNameString.c_str(),
			(*p)->attrName, (*p)->attrFlags);

		if ((*p)->attrFlags & SA_IMM_ATTR_RUNTIME) {
			if((*p)->attrFlags & SA_IMM_ATTR_PERSISTENT) {
				TRACE_2("Persistent runtime attribute %s", (*p)->attrName);
				class_is_pure_runtime=false;
			} else {
				TRACE_2("PURE runtime attribute %s", (*p)->attrName);
				attr_is_pure_rt=true;
			}
		}

		if ((*p)->attrFlags & SA_IMM_ATTR_MULTI_VALUE) {
			TRACE_2("Multivalued attribute %s", (*p)->attrName);
			attr_is_multi=true;
		}

		if(!(attr_is_pure_rt || attr_is_multi)) {
			sqlB.append(", ");
			sqlB.append((*p)->attrName);
			typeToPBE(*p, dbHandle, &sqlB);
		}

		sqlC2.append((*p)->attrName);
		sqlC2.append("', '");

		snprintf(attrPropStr, STRINT_BSZ, "%u", (*p)->attrValueType);
		sqlC2.append(attrPropStr);
		sqlC2.append("', '");
		snprintf(attrPropStr, STRINT_BSZ, "%u", (unsigned int) (*p)->attrFlags);
		sqlC2.append(attrPropStr);		
		sqlC2.append("' )");
		TRACE("GENERATED C2: (%s)", sqlC2.c_str());
		rc = sqlite3_exec(dbHandle, sqlC2.c_str(), NULL, NULL, &execErr);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
				sqlC2.c_str(), execErr);
			sqlite3_free(execErr);
			goto bailout;
		}

		if ((*p)->attrDefaultValue != NULL)
		{
			std::string sqlD2(sqlD);
			switch ((*p)->attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T: 
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET:
					sqlD2.append(" int_dflt) values ('");
					break;

				case SA_IMM_ATTR_SAFLOATT:  
				case SA_IMM_ATTR_SADOUBLET: 
					sqlD2.append(" real_dflt) values ('");
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					sqlD2.append(" text_dflt) values ('");
					break;

				default:
					LOG_ER("Invalid attribte type %u", 
						(*p)->attrValueType);
					goto bailout;
			}

			sqlD2.append(classIdStr);
			sqlD2.append("', '");
			sqlD2.append((*p)->attrName);
			sqlD2.append("', '");
			sqlD2.append(valueToString((*p)->attrDefaultValue, 
					     (*p)->attrValueType));
			sqlD2.append("')");
			TRACE("GENERATED D2: (%s)", sqlD2.c_str());
			rc = sqlite3_exec(dbHandle, sqlD2.c_str(), NULL, NULL, &execErr);
			if(rc != SQLITE_OK) {
				LOG_ER("SQL statement ('%s') failed because:\n %s",
					sqlD2.c_str(), execErr);
				sqlite3_free(execErr);
				goto bailout;
			}
		}

		if (sqlB.size() > sqlBsize) {
			LOG_ER("SQL statement too long:%zu max length:%u", 
				sqlB.size(), sqlBsize);
			goto bailout;
		}
	}

	sqlB.append(")");
	if(class_is_pure_runtime) {
		TRACE_2("Class %s is pure runtime, no create of instance table",
			classNameString.c_str());
	} else {
		TRACE("GENERATED B: (%s)", sqlB.c_str());
		rc = sqlite3_exec(dbHandle, sqlB.c_str(), NULL, NULL, &execErr);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sqlB.c_str(),
				execErr);
			sqlite3_free(execErr);
			goto bailout;
		}
	}

	errorCode = saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
	if (SA_AIS_OK != errorCode)
	{
		TRACE_4("Failed to free the description of class %s error:%u",
			classNameString.c_str(), errorCode);
		goto bailout;
	}
	TRACE_LEAVE();
	return classInfo;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	delete classInfo;
	/* TODO remove imm.db file */
	LOG_ER("Exiting");
	exit(1);
}

void deleteClassToPBE(std::string classNameString, void* db_handle, 
	ClassInfo* theClass)
{

	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sqlZ("select obj_id from objects where class_id = ");

	std::string sqlA("delete from classes where class_id = ");
	std::string sqlB("drop table ");
	std::string sqlC("delete from attr_def where class_id = ");
	std::string sqlD("delete from attr_dflt where class_id = ");

	int rc=0;
	char **result=NULL;
	char *execErr=NULL;
	int nrows=0;
	int ncols=0;
	unsigned int rowsModified=0;
	char classIdStr[STRINT_BSZ];
	snprintf(classIdStr, STRINT_BSZ, "%u", theClass->mClassId);

	/* 1. Verify zero instances of the class in objects relation. */
	sqlZ.append(classIdStr);

	TRACE("GENERATED Z:%s", sqlZ.c_str());
	rc = sqlite3_get_table(dbHandle, sqlZ.c_str(), &result, &nrows, &ncols, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlZ.c_str(),
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	sqlite3_free_table(result);
	if(nrows) {
		LOG_ER("Can not delete class %s when instances exist, rows:%u", 
			classNameString.c_str(), nrows);
		goto bailout;
	}

	/* 2. Remove classes tuple. */
	sqlA.append(classIdStr);
	TRACE("GENERATED A:%s", sqlA.c_str());
	rc = sqlite3_exec(dbHandle, sqlA.c_str(), NULL, NULL, &execErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlA.c_str(), execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from classes", rowsModified);
	if(rowsModified != 1) {
		LOG_ER("Could not delete class tuple for class %s from relation 'classes'",
			classNameString.c_str());
		goto bailout;
	}
	
	/* 3. Remove attr_def tuples. */
	sqlC.append(classIdStr);
	TRACE("GENERATED C:%s", sqlC.c_str());
	rc = sqlite3_exec(dbHandle, sqlC.c_str(), NULL, NULL, &execErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlC.c_str(), execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from attr_def", rowsModified);
	if(rowsModified == 0) {
		LOG_WA("Could not delete attr_def tuples for class %s",
			classNameString.c_str());
		/* Dont bailout on this one. Could possibly be a degenerate class.*/
	}

	/* 4. Remove attr_dflt tuples. */
	sqlD.append(classIdStr);
	TRACE("GENERATED D:%s", sqlD.c_str());
	rc = sqlite3_exec(dbHandle, sqlD.c_str(), NULL, NULL, &execErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlD.c_str(), execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from attr_dflt", rowsModified);

	/* 5. Drop 'classname' base relation. */
	sqlB.append(classNameString);
	TRACE("GENERATED B:%s", sqlB.c_str());
	rc = sqlite3_exec(dbHandle, sqlB.c_str(), NULL, NULL, &execErr);
	if(rc) {
		TRACE("SQL statement ('%s') failed because:\n %s", sqlB.c_str(), execErr);
		TRACE("Class apparently defined non-persistent runtime objects");
		sqlite3_free(execErr);
		/**/
	} else {
		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Dropped table %s rows:%u", classNameString.c_str(), rowsModified);
	}

	TRACE_LEAVE();
	return;

bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting");
	exit(1);
}

static ClassInfo* verifyClassPBE(std::string classNameString,
	SaImmHandleT immHandle,
	void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	int rc=0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	unsigned int class_id=0;
	ClassInfo* classInfo = NULL;

	SaImmClassCategoryT classCategory;
	SaImmAttrDefinitionT_2 **attrDefinitions;
	SaAisErrorT errorCode;
	/*
	  This function does very little verification right now. It should do more.
	  Reason is we are re-attaching to a DB file after a gap during which we do not know
	  what has happened to that file. We should at least verify that the number of classes 
	  and number of objects match. Each object should really have a checksum.
	  Verification of objects should be a separate function verifyObjectPBE.
	*/

	TRACE_ENTER();

	std::string sqlZ("SELECT class_id FROM classes WHERE class_name = '");
	sqlZ.append(classNameString.c_str());
	sqlZ.append("'");

	TRACE("GENERATED Z:%s", sqlZ.c_str());

	rc = sqlite3_get_table(dbHandle, sqlZ.c_str(), &result, &nrows, &ncols, &zErr);

	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlZ.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}

	
        class_id = strtoul(result[ncols], NULL, 0);
	classInfo = new ClassInfo(class_id);
	sqlite3_free_table(result);
	TRACE("ClassId:%u", class_id);

	/* Get the class description */
	errorCode = saImmOmClassDescriptionGet_2(immHandle,
		(char*)classNameString.c_str(),
		&classCategory,
		&attrDefinitions);

	for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
	{
	
		classInfo->mAttrMap[(*p)->attrName] = (*p)->attrFlags;
		TRACE("VERIFIED Class %s Attr%s Flags %llx", classNameString.c_str(),
			(*p)->attrName, (*p)->attrFlags);
	}

	errorCode = saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
	if (SA_AIS_OK != errorCode)
	{
		TRACE_4("Failed to free the description of class %s error:%u",
			classNameString.c_str(), errorCode);
		goto bailout;
	}

	TRACE_LEAVE();
	return classInfo;

 bailout:
	/*sqlite3_close((sqlite3 *) dbHandle);*/
	delete classInfo;
	LOG_WA("Verify class %s failed!", classNameString.c_str());
	return NULL;
}

void stampObjectWithCcbId(void* db_handle, const char* object_id,  SaUint64T ccb_id)
{
	int rc=0;
	char *execErr=NULL;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string stampObj("UPDATE objects SET last_ccb = ");
	char ccbIdStr[STRINT_BSZ];

	TRACE_ENTER();
	snprintf(ccbIdStr, STRINT_BSZ, "%llu", ccb_id);
	stampObj.append(ccbIdStr);
	stampObj.append(" WHERE obj_id = ");
	stampObj.append(object_id);

	TRACE("GENERATED STAMP-OBJ:%s", stampObj.c_str());

	rc = sqlite3_exec(dbHandle, stampObj.c_str(), NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", 
			stampObj.c_str(), execErr);
		sqlite3_free(execErr);
		exit(1);
	}
	TRACE_LEAVE();
}

void objectModifyDiscardAllValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sql1("select obj_id,class_id from objects where dn = '");
	std::string sql21("select attr_type,attr_flags from attr_def where class_id = ");

	int rc=0;
	char **result=NULL;
	char **result2=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char* object_id=NULL;
	const char* class_id=NULL;
	const char* class_name=NULL;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	unsigned int rowsModified=0;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	sql1.append(objName);
	sql1.append("'");
	
	TRACE("GENERATED 1:%s", sql1.c_str());

	rc = sqlite3_get_table(dbHandle, sql1.c_str(), &result, &nrows,
		&ncols, &zErr);

	if(rc) {
		LOG_ER("Could not access object '%s' for delete, error:%s",
			objName.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	TRACE_2("Successfully accessed object '%s'. cols:%u", 
		objName.c_str(), ncols);

	object_id = result[ncols];
	class_id  = result[ncols+1];

	TRACE_2("object_id:%s class_id:%s", object_id, class_id);

	/* Second, obtain the class description for the attribute. */

	sql21.append(class_id);
	sql21.append(" and attr_name = '");
	sql21.append(attrValue->attrName);
	sql21.append("'");

	TRACE("GENERATED 21:%s", sql21.c_str());

	rc = sqlite3_get_table(dbHandle, sql21.c_str(), &result2, &nrows,
		&ncols, &zErr);

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}

	TRACE_2("Successfully accessed attr_def for class_id:%s attr_name:%s. cols:%u (T:%s, F:%s)", 
		class_id, attrValue->attrName, ncols, result2[ncols], result2[ncols+1]);

	attr_type = (SaImmValueTypeT) atol(result2[ncols]);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = atol(result2[ncols+1]);
	sqlite3_free_table(result2);

	if(ccb_id == 0) { /* PRTAttr case */
		if(!(attr_flags & SA_IMM_ATTR_PERSISTENT)) {
			TRACE_2("PRTAttr updates skipping non persistent RTAttr %s",
				attrValue->attrName);
			goto done;
		} else {
			TRACE_2("PERSISTENT RUNTIME ATTRIBUTE %s UPDATE",
				attrValue->attrName);
		}
	}

	if(attr_flags & SA_IMM_ATTR_MULTI_VALUE) {
		TRACE_3("COMPONENT TEST BRANCH 1");
		/* Remove all values. */
		std::string sql211("delete from objects_");
		switch(attr_type) {
			case SA_IMM_ATTR_SAINT32T: 
			case SA_IMM_ATTR_SAUINT32T: 
			case SA_IMM_ATTR_SAINT64T:  
			case SA_IMM_ATTR_SAUINT64T: 
			case SA_IMM_ATTR_SATIMET:
				sql211.append("int_multi where obj_id = ");
			break;

		case SA_IMM_ATTR_SAFLOATT:  
		case SA_IMM_ATTR_SADOUBLET: 
				sql211.append("real_multi where obj_id = ");
			break;

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
				sql211.append("text_multi where obj_id = ");
			break;

		default:
			LOG_ER("Unknown value type %u", attr_type);
			goto bailout;
		}

		sql211.append(object_id);
		sql211.append(" and attr_name = '");
		sql211.append(attrValue->attrName);
		sql211.append("'");

		TRACE("GENERATED 211:%s", sql211.c_str());
		rc = sqlite3_exec(dbHandle, sql211.c_str(), NULL, NULL, &zErr);
		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql211.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}
		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Deleted %u values", rowsModified);
	} else {
		/* Assign the null value to the single valued attribute. */
		std::string sql22("update ");
		std::string sql3("select class_name from classes where class_id = ");

		/* Get the class-name for the object */
		sql3.append(class_id);
		TRACE("GENERATED 3:%s", sql3.c_str());
		rc = sqlite3_get_table(dbHandle, sql3.c_str(), &result2, &nrows,
			&ncols, &zErr);

		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql3.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}

		if(nrows != 1) {
			LOG_ER("Expected 1 row got %u rows", nrows);
			goto bailout;
		}

		class_name = result2[ncols];
		TRACE_2("Successfully accessed classes class_id:%s class_name:'%s'. cols:%u", 
			class_id, class_name, ncols);

		/* Update the relevant attribute in the class_name table */
		sql22.append(class_name);
		sql22.append(" set ");
		sql22.append(attrValue->attrName);
		sql22.append(" = NULL where obj_id =");
		sql22.append(object_id);
		sqlite3_free_table(result2);

		TRACE("GENERATED 22:%s", sql22.c_str());
		rc = sqlite3_exec(dbHandle, sql22.c_str(), NULL, NULL, &zErr);
		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql22.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}
		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Update %u values", rowsModified);
	}

 done:
	if(rowsModified) stampObjectWithCcbId(db_handle, object_id, ccb_id);
	/* Warning function call on line above refers to >>result<< via object_id */
	sqlite3_free_table(result);

	TRACE_LEAVE();
	return;

 bailout:
	TRACE_LEAVE();
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting");
	exit(1);
}



void objectModifyDiscardMatchingValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sql1("select obj_id,class_id from objects where dn = '");
	std::string sql21("select attr_type,attr_flags from attr_def where class_id = ");

	int rc=0;
	char **result=NULL;
	char **result2=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char* object_id=NULL;
	const char* class_id=NULL;
	const char* class_name=NULL;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	bool text_val = false;
	unsigned int rowsModified=0;

	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	sql1.append(objName);
	sql1.append("'");
	
	TRACE("GENERATED 1:%s", sql1.c_str());

	rc = sqlite3_get_table(dbHandle, sql1.c_str(), &result, &nrows,
		&ncols, &zErr);

	if(rc) {
		LOG_ER("Could not access object '%s' for delete, error:%s",
			objName.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	TRACE_2("Successfully accessed object '%s'. cols:%u", 
		objName.c_str(), ncols);

	object_id = result[ncols];
	class_id  = result[ncols+1];
	TRACE_2("object_id:%s class_id:%s", object_id, class_id);

	/* Second, obtain the class description for the attribute. */

	sql21.append(class_id);
	sql21.append(" and attr_name = '");
	sql21.append(attrValue->attrName);
	sql21.append("'");

	TRACE("GENERATED 21:%s", sql21.c_str());

	rc = sqlite3_get_table(dbHandle, sql21.c_str(), &result2, &nrows,
		&ncols, &zErr);

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}

	TRACE_2("Successfully accessed attr_def for class_id:%s attr_name:%s. cols:%u (T:%s, F:%s)", 
		class_id, attrValue->attrName, ncols, result2[ncols], result2[ncols+1]);

	attr_type = (SaImmValueTypeT) atol(result2[ncols]);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = atol(result2[ncols+1]);
	sqlite3_free_table(result2);

	if(ccb_id == 0) { /* PRTAttr case */
		if(!(attr_flags & SA_IMM_ATTR_PERSISTENT)) {
			TRACE_2("PRTAttr updates skipping non persistent RTAttr %s",
				attrValue->attrName);
			goto done;
		} else {
			TRACE_2("PERSISTENT RUNTIME ATTRIBUTE %s UPDATE",
				attrValue->attrName);
		}
	}

	if(attr_flags & SA_IMM_ATTR_MULTI_VALUE) {
		/* Remove all matching values. */
		std::string sql212("delete from objects_");
		std::string val_attr;
		unsigned int ix;

		switch(attr_type) {
			case SA_IMM_ATTR_SAINT32T: 
			case SA_IMM_ATTR_SAUINT32T: 
			case SA_IMM_ATTR_SAINT64T:  
			case SA_IMM_ATTR_SAUINT64T: 
			case SA_IMM_ATTR_SATIMET:
				sql212.append("int_multi where obj_id = ");
				val_attr.append("' and int_val = ");
			break;

		case SA_IMM_ATTR_SAFLOATT:  
		case SA_IMM_ATTR_SADOUBLET: 
				sql212.append("real_multi where obj_id = ");
				val_attr.append("' and real_val =");
			break;

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
				sql212.append("text_multi where obj_id = ");
				val_attr.append("' and text_val = '");
				text_val = true;
			break;

		default:
			LOG_ER("Unknown value type %u", attr_type);
			goto bailout;
		}

		sql212.append(object_id);
		sql212.append(" and attr_name = '");
		sql212.append(attrValue->attrName);
		sql212.append(val_attr);
		for(ix=0; ix < attrValue->attrValuesNumber; ++ix) {
			sql212.append(valueToString(attrValue->attrValues[ix], attr_type));
			if(text_val) {
				sql212.append("'");
			}
			TRACE("GENERATED 212:%s", sql212.c_str());
			rc = sqlite3_exec(dbHandle, sql212.c_str(), NULL, NULL, &zErr);
			if(rc) {
				LOG_ER("SQL statement ('%s') failed because:\n %s", 
					sql212.c_str(), zErr);
				sqlite3_free(zErr);
				goto bailout;
			}
			rowsModified=sqlite3_changes(dbHandle);
			TRACE("Deleted %u values", rowsModified);
		}
	} else {
		/* Assign the null value to the single valued attribute IFF
		   current value matches.
		 */
		unsigned int ix;
		std::string sql23("update ");
		std::string sql3("select class_name from classes where class_id = ");
		bool text_val = ((attr_type == SA_IMM_ATTR_SANAMET) ||
			(attr_type == SA_IMM_ATTR_SASTRINGT) ||
			(attr_type == SA_IMM_ATTR_SAANYT));
		TRACE_3("COMPONENT TEST BRANCH 5");

		/* Get the class-name for the object */
		sql3.append(class_id);
		TRACE("GENERATED 3:%s", sql3.c_str());
		rc = sqlite3_get_table(dbHandle, sql3.c_str(), &result2, &nrows,
			&ncols, &zErr);

		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql3.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}

		if(nrows != 1) {
			LOG_ER("Expected 1 row got %u rows", nrows);
			goto bailout;
		}
		TRACE_2("Successfully accessed classes class_id:%s class_name:'%s'. cols:%u", 
			class_id, class_name, ncols);

		class_name = result2[ncols];


		/* Update the relevant attribute in the class_name table IFF value matches. */
		sql23.append(class_name);
		sql23.append(" set ");
		sql23.append(attrValue->attrName);
		sql23.append(" = NULL where obj_id =");
		sql23.append(object_id);
		sql23.append(" and ");
		sql23.append(attrValue->attrName);
		sql23.append(" = ");
		if(text_val) {sql23.append("'");}
		sqlite3_free_table(result2);
		for(ix=0; ix < attrValue->attrValuesNumber; ++ix) {
		TRACE_3("COMPONENT TEST BRANCH 6");
			sql23.append(valueToString(attrValue->attrValues[ix], attr_type));
			if(text_val) {sql23.append("'");}
			TRACE("GENERATED 23:%s", sql23.c_str());
			rc = sqlite3_exec(dbHandle, sql23.c_str(), NULL, NULL, &zErr);
			if(rc) {
				LOG_ER("SQL statement ('%s') failed because:\n %s",
					sql23.c_str(), zErr);
				sqlite3_free(zErr);
				goto bailout;
			}
			rowsModified=sqlite3_changes(dbHandle);
			TRACE("Update %u values", rowsModified);
		}
	}
 done:
	if(rowsModified) stampObjectWithCcbId(db_handle, object_id, ccb_id);
	/* Warning function call on line above refers to >>result<< via object_id */
	sqlite3_free_table(result); 
	TRACE_LEAVE();
	return;

 bailout:
	TRACE_LEAVE();
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting");
	exit(1);
}

void objectModifyAddValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sql1("select obj_id,class_id from objects where dn = '");
	std::string sql21("select attr_type,attr_flags from attr_def where class_id = ");

	int rc=0;
	char **result=NULL;
	char **result2=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char* object_id=NULL;
	const char* class_id=NULL;
	const char* class_name=NULL;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	unsigned int rowsModified=0;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	sql1.append(objName);
	sql1.append("'");
	
	TRACE("GENERATED 1:%s", sql1.c_str());

	rc = sqlite3_get_table(dbHandle, sql1.c_str(), &result, &nrows,
		&ncols, &zErr);

	if(rc) {
		LOG_ER("Could not access object '%s' for delete, error:%s",
			objName.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	TRACE_2("Successfully accessed object '%s'. cols:%u", 
		objName.c_str(), ncols);

	object_id = result[ncols];
	class_id  = result[ncols+1];

	TRACE_2("object_id:%s class_id:%s", object_id, class_id);

	/* Second, obtain the class description for the attribute. */

	sql21.append(class_id);
	sql21.append(" and attr_name = '");
	sql21.append(attrValue->attrName);
	sql21.append("'");

	TRACE("GENERATED 21:%s", sql21.c_str());

	rc = sqlite3_get_table(dbHandle, sql21.c_str(), &result2, &nrows,
		&ncols, &zErr);

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}

	TRACE_2("Successfully accessed attr_def for class_id:%s attr_name:%s. cols:%u (T:%s, F:%s)", 
		class_id, attrValue->attrName, ncols, result2[ncols], result2[ncols+1]);

	attr_type = (SaImmValueTypeT) atol(result2[ncols]);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = atol(result2[ncols+1]);
	sqlite3_free_table(result2);

	if(ccb_id == 0) { /* PRTAttr case */
		if(!(attr_flags & SA_IMM_ATTR_PERSISTENT)) {
			TRACE_2("PRTAttr updates skipping non persistent RTAttr %s",
				attrValue->attrName);
			goto done;
		} else {
			TRACE_2("PERSISTENT RUNTIME ATTRIBUTE %s UPDATE",
				attrValue->attrName);
		}
	}

	if(attr_flags & SA_IMM_ATTR_MULTI_VALUE) {
		TRACE("Add to multivalued attribute.");
	        valuesToPBE(attrValue, attr_flags, object_id, dbHandle);
		++rowsModified;/* Not a correct count, just for stampObjectWithCcbId */
	} else {
		/* Add value to single valued */
		std::string sql22("update ");
		std::string sql3("select class_name from classes where class_id = ");

		assert(attrValue->attrValuesNumber == 1);
		/* Get the class-name for the object */
		sql3.append(class_id);
		TRACE("GENERATED 3:%s", sql3.c_str());
		rc = sqlite3_get_table(dbHandle, sql3.c_str(), &result2, &nrows, &ncols, &zErr);

		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql3.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}

		if(nrows != 1) {
			LOG_ER("Expected 1 row got %u rows", nrows);
			goto bailout;
		}

		class_name = result2[ncols];
		TRACE_2("Successfully accessed classes class_id:%s class_name:'%s'. cols:%u", 
			class_id, class_name, ncols);

		/* Update the relevant attribute in the class_name table */
		/* We should check that the current value is NULL, but we assume instead
		   that the ImmModel has done this check. */
		sql22.append(class_name);
		sql22.append(" set ");
		sql22.append(attrValue->attrName);
		sql22.append(" = ");
		sql22.append(valueToString(attrValue->attrValues[0], attrValue->attrValueType));
		sql22.append(" where obj_id = ");
		sql22.append(object_id);
		sqlite3_free_table(result2);

		TRACE("GENERATED 22:%s", sql22.c_str());
		rc = sqlite3_exec(dbHandle, sql22.c_str(), NULL, NULL, &zErr);
		if(rc) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql22.c_str(), zErr);
			sqlite3_free(zErr);
			goto bailout;
		}
		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Updated %u values", rowsModified);
	}
 done:
	if(rowsModified) stampObjectWithCcbId(db_handle, object_id, ccb_id);
	/* Warning function call on line above refers to >>result<< via object_id */
	sqlite3_free_table(result);

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting");
	exit(1);
}

void objectDeleteToPBE(std::string objectNameString, void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sql1("select obj_id,class_id from objects where dn = '");
	std::string sql2("delete from objects where obj_id = ");
	std::string sql3("select class_name from classes where class_id = ");
	std::string sql4("delete from ");
	std::string sql5("delete from objects_int_multi where obj_id = ");
	std::string sql6("delete from objects_real_multi where obj_id = ");
	std::string sql7("delete from objects_text_multi where obj_id = ");

	int rc=0;
	char **result=NULL;
	char **result2=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char* object_id=NULL;
	const char* class_id=NULL;
	const char* class_name=NULL;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	sql1.append(objectNameString);
	sql1.append("'");
	
	TRACE("GENERATED 1:%s", sql1.c_str());

	rc = sqlite3_get_table(dbHandle, sql1.c_str(), &result, &nrows,
		&ncols, &zErr);

	if(rc) {
		LOG_ER("Could not access object '%s' for delete, error:%s",
			objectNameString.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	TRACE_2("Successfully accessed object '%s'. cols:%u", 
		objectNameString.c_str(), ncols);

	object_id = result[ncols];
	class_id  = result[ncols+1];

	TRACE_2("object_id:%s class_id:%s", object_id, class_id);

	/*
	  Second, delete the root object tuple in objects for obj_id. 
	*/

	sql2.append(object_id);
	TRACE("GENERATED 2:%s", sql2.c_str());

	rc = sqlite3_exec(dbHandle, sql2.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql2.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	/* Third get the class-name for the object */

	sql3.append(class_id);
	TRACE("GENERATED 3:%s", sql3.c_str());
	rc = sqlite3_get_table(dbHandle, sql3.c_str(), &result2, &nrows,
		&ncols, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql3.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}
	class_name = result2[ncols];

	TRACE_2("Successfully accessed classes class_id:%s class_name:'%s'. cols:%u", 
		class_id, class_name, ncols);

	/* Fourth delete the base attribute tuple from table 'classname' for obj_id.
	 */
	sql4.append(class_name);
	sql4.append(" where obj_id = ");
	sql4.append(object_id);
	sqlite3_free_table(result2);

	TRACE("GENERATED 4:%s", sql4.c_str());
	rc = sqlite3_exec(dbHandle, sql4.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql4.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	/*
	  Fifth delete from objects_int_multi, objects_real_multi, objects_text_multi where
	  obj_id ==OBJ_ID
	 */

	sql5.append(object_id);
	TRACE("GENERATED 5:%s", sql5.c_str());

	rc = sqlite3_exec(dbHandle, sql5.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql5.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	sql6.append(object_id);
	TRACE("GENERATED 6:%s", sql6.c_str());

	rc = sqlite3_exec(dbHandle, sql6.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql6.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	sql7.append(object_id);
	TRACE("GENERATED 7:%s", sql7.c_str());

	rc = sqlite3_exec(dbHandle, sql7.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql7.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	sqlite3_free_table(result);
	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting");
	exit(1);
}



void objectToPBE(std::string objectNameString,
	const SaImmAttrValuesT_2** attrs,
	ClassMap *classIdMap,
	void* db_handle,
	unsigned int object_id,
	SaImmClassNameT className,
	SaUint64T ccb_id)
{
	std::string valueString;
	std::string classNameString;
	char objIdStr[STRINT_BSZ];
	char classIdStr[STRINT_BSZ];
	unsigned int class_id=0;
	char ccbIdStr[STRINT_BSZ];
	ClassInfo* classInfo=NULL;
	int rc=0;
	char *execErr=NULL;
	sqlite3* dbHandle = (sqlite3 *) db_handle;

	std::string sqlE("INSERT INTO objects (obj_id, class_id, dn, last_ccb) values('");
	std::string sqlF("INSERT INTO ");
	std::string sqlF1(" (obj_id ");
	std::string sqlF2(" values('");

	TRACE_ENTER();
	TRACE_1("Dumping object %s", objectNameString.c_str());
	if(className) {
		classNameString = std::string(className);
	} else {
		classNameString = getClassName(attrs);
	}

	classInfo = (*classIdMap)[classNameString];
	if(!classInfo) {
		LOG_ER("Class '%s' not found in classIdMap", 
			classNameString.c_str());
		goto bailout;
	}
	class_id = classInfo->mClassId;
	snprintf(classIdStr, STRINT_BSZ, "%u", class_id);
	snprintf(objIdStr, STRINT_BSZ, "%u", object_id);
	snprintf(ccbIdStr, STRINT_BSZ, "%llu", ccb_id);

	sqlE.append(objIdStr);
	sqlE.append("', '");
	sqlE.append(classIdStr);
	sqlE.append("', '");
	sqlE.append(objectNameString);
	sqlE.append("', '");
	sqlE.append(ccbIdStr);
	sqlE.append("')");
	TRACE("GENERATED E:%s", sqlE.c_str());
	rc = sqlite3_exec(dbHandle, sqlE.c_str(), NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlE.c_str(),
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	sqlF.append(classNameString.c_str());
	sqlF2.append(objIdStr);

	for (const SaImmAttrValuesT_2** p = attrs; *p != NULL; p++)
	{
		std::string attName((*p)->attrName);
		SaImmAttrFlagsT attrFlags = classInfo->mAttrMap[attName];
		if(attrFlags == 0) {
			LOG_ER("Missing attrInfo %s in classinfo %s", 
				(*p)->attrName, classNameString.c_str());
			goto bailout;
		}

		/* Skip attributes that have no values. */
		if ((*p)->attrValues == NULL || ((*p)->attrValuesNumber == 0))
		{
			continue;
		}

		if(attrFlags & SA_IMM_ATTR_RUNTIME) {
			if(attrFlags & SA_IMM_ATTR_PERSISTENT) {
				TRACE_2("Persistent runtime attribute %s", (*p)->attrName);
			} else {
				TRACE_2("PURE runtime attribute %s - ignoring",
					(*p)->attrName);
				continue;
			}
		}

		if (attrFlags & SA_IMM_ATTR_MULTI_VALUE) {
			TRACE_2("Attr %s is MULTIVALUED", (*p)->attrName);
			valuesToPBE(*p, attrFlags, objIdStr, db_handle);
		} else {
			if((*p)->attrValuesNumber != 1) {
				LOG_ER("attrValuesNumber not 1 :%u",
					(*p)->attrValuesNumber);
				assert((*p)->attrValuesNumber == 1);
			}
			sqlF1.append(", ");
			sqlF1.append(attName);

			sqlF2.append("', '");
			sqlF2.append(valueToString(*((*p)->attrValues),
					     (*p)->attrValueType));
		}
	}

	sqlF1.append(")");
	sqlF2.append("')");
	sqlF.append(sqlF1);
	sqlF.append(sqlF2);

	TRACE("GENERATED F:%s", sqlF.c_str());
	rc = sqlite3_exec(dbHandle, sqlF.c_str(), NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlF.c_str(),
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	TRACE_LEAVE();
	return;
 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting");
	/* TODO remove imm.db file */
	exit(1);
}


void dumpClassesToPbe(SaImmHandleT immHandle, ClassMap *classIdMap,
	void* db_handle)
{
	std::list<std::string> classNameList;
	std::list<std::string>::iterator it;
	int rc=0;
	unsigned int class_id=0;
	char *execErr=NULL;	sqlite3* dbHandle = (sqlite3 *) db_handle;
	TRACE_ENTER();

	classNameList = getClassNames(immHandle);
	it = classNameList.begin();

	sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('BEGIN EXCLUSIVE TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	while (it != classNameList.end())
	{
		(*classIdMap)[(*it)] = 
			classToPBE((*it), immHandle, dbHandle, ++class_id);
		it++;
	}

	sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close(dbHandle);
	/* TODO remove imm.db file */
	LOG_ER("Exiting");
	exit(1);	
}

unsigned int verifyPbeState(SaImmHandleT immHandle, ClassMap *classIdMap, void* db_handle)
{
	/* Function used only when re-connecting to an already existing DB file. */
	std::list<std::string> classNameList;
	std::list<std::string>::iterator it;
	int rc=0;
	char *execErr=NULL;	
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sqlQ("SELECT MAX(obj_id) FROM objects");
	unsigned int obj_count;
	char **result=NULL;
	char *qErr=NULL;
	int nrows=0;
	int ncols=0;
	TRACE_ENTER();

	classNameList = getClassNames(immHandle);
	it = classNameList.begin();

	sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('BEGIN EXCLUSIVE TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	while (it != classNameList.end())
	{
		ClassInfo* cl_info = verifyClassPBE((*it), immHandle, dbHandle);
		if(cl_info) { 
			(*classIdMap)[(*it)] = cl_info;
			it++;
		} else {
			goto bailout;
		}
	}

	rc = sqlite3_get_table(dbHandle, sqlQ.c_str(), &result, &nrows, &ncols, &qErr);

	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlQ.c_str(), qErr);
		sqlite3_free(qErr);
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows", nrows);
		goto bailout;
	}

	obj_count = strtoul(result[ncols], NULL, 0);
	TRACE("verifPbeState: obj_count:%u", obj_count);

	sqlite3_exec(dbHandle, "ABORT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('ABORT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	TRACE_LEAVE();
	return obj_count;

 bailout:
	sqlite3_close(dbHandle);
	LOG_WA("verifyPbeState failed!");
	return 0;
}

unsigned int dumpObjectsToPbe(SaImmHandleT immHandle, ClassMap* classIdMap,
	void* db_handle)
{
	int                    rc=0;
	SaNameT                root;
	SaImmSearchHandleT     searchHandle;
	SaAisErrorT            errorCode;
	SaNameT                objectName;
	SaImmAttrValuesT_2**   attrs;
	unsigned int           retryInterval = 1000000; /* 1 sec */
	unsigned int           maxTries = 15;          /* 15 times == max 15 secs */
	unsigned int           tryCount=0;
	char *execErr=NULL;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	TRACE_ENTER();
	unsigned int object_id=0;
	root.length = 0;
	strncpy((char*)root.value, "", 3);

	/* Initialize immOmSearch */

	TRACE_1("Before searchInitialize");
	do {
		if(tryCount) {
			usleep(retryInterval);
		}
		++tryCount;

		errorCode = saImmOmSearchInitialize_2(immHandle, 
			&root, 
			SA_IMM_SUBTREE,
			(SaImmSearchOptionsT)
			(SA_IMM_SEARCH_ONE_ATTR | 
				SA_IMM_SEARCH_GET_ALL_ATTR |
				SA_IMM_SEARCH_PERSISTENT_ATTRS),//Special & nonstandard
			NULL/*&params*/, 
			NULL, 
			&searchHandle);
 
	} while ((errorCode == SA_AIS_ERR_TRY_AGAIN) &&
		(tryCount < maxTries)); /* Can happen if imm is syncing. */

	TRACE_1("After searchInitialize rc:%u", errorCode);
	if (SA_AIS_OK != errorCode)
	{
		LOG_ER("Failed on saImmOmSearchInitialize:%u - exiting ", errorCode);
		goto bailout;
	}

	sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('BEGIN EXCLUSIVE TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	/* Iterate through the object space */
	do
	{
		errorCode = saImmOmSearchNext_2(searchHandle, &objectName, &attrs);

		if (SA_AIS_OK != errorCode)
		{
			break;
		}

		if (attrs[0] == NULL)
		{
			TRACE_2("Skipping object %s because no attributes from searchNext",
				(char *) objectName.value);
			continue;
		}

		objectToPBE(std::string((char*)objectName.value, objectName.length),
			(const SaImmAttrValuesT_2**) attrs, classIdMap, dbHandle, ++object_id, NULL, 0);
	} while (true);

	if (SA_AIS_ERR_NOT_EXIST != errorCode)
	{
		LOG_ER("Failed in saImmOmSearchNext_2:%u - exiting", errorCode);
		goto bailout;
	}

	sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	/* End the search */
	saImmOmSearchFinalize(searchHandle);
	TRACE_LEAVE();
	return object_id; /* == number of dumped objects */
 bailout:
	sqlite3_close(dbHandle);
	/* TODO remove imm.db file */
	LOG_ER("Exiting");
	exit(1);	
}

SaAisErrorT pbeBeginTrans(void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;

	sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('BEGIN EXCLUSIVE TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	return SA_AIS_OK;
}

SaAisErrorT pbeCommitTrans(void* db_handle, SaUint64T ccbId, SaUint32T currentEpoch)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;
	unsigned int commitTime=0;
	char ccbIdStr[STRINT_BSZ];
	char epochStr[STRINT_BSZ];
	char commitTimeStr[STRINT_BSZ];
	time_t now = time(NULL);

	if(ccbId) {
		commitTime = (unsigned int) now;

		std::string sqlCcb("INSERT INTO ccb_commits (ccb_id, epoch, commit_time) VALUES('");

		snprintf(ccbIdStr, STRINT_BSZ, "%llu", ccbId);
		snprintf(epochStr, STRINT_BSZ, "%u", currentEpoch);
		snprintf(commitTimeStr, STRINT_BSZ, "%u", commitTime);

		sqlCcb.append(ccbIdStr);
		sqlCcb.append("','");
		sqlCcb.append(epochStr);
		sqlCcb.append("','");
		sqlCcb.append(commitTimeStr);
		sqlCcb.append("')");

		TRACE("GENERATED CCB:%s", sqlCcb.c_str());

		rc = sqlite3_exec(dbHandle, sqlCcb.c_str(), NULL, NULL, &execErr);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sqlCcb.c_str(),
				execErr);
			sqlite3_free(execErr);
			pbeAbortTrans(db_handle);
			return SA_AIS_ERR_FAILED_OPERATION;
		}
	}

	sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	return SA_AIS_OK;
}

void purgeCcbCommitsFromPbe(void* db_handle, SaUint32T currentEpoch)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;
	std::string sqlPurge("delete from ccb_commits where epoch <= ");
	char epochStr[STRINT_BSZ];

	if(currentEpoch < 11) return;

	snprintf(epochStr, STRINT_BSZ, "%u", currentEpoch - 10);
	sqlPurge.append(epochStr);

	TRACE("GENERATED sqlPurge:%s", sqlPurge.c_str());
	rc = sqlite3_exec(dbHandle, sqlPurge.c_str(), NULL, NULL, &execErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlPurge.c_str(), execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	TRACE("Deleted %u ccb commits", sqlite3_changes(dbHandle));
	return;

 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting");
	exit(1);
}

void pbeAbortTrans(void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;

	sqlite3_exec(dbHandle, "ABORT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('ABORT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
	}
}

SaAisErrorT getCcbOutcomeFromPbe(void* db_handle, SaUint64T ccbId, SaUint32T currentEpoch)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	int rc=0;
	char **result=NULL;
	char *yErr=NULL;
	int nrows=0;
	int ncols=0;
	SaAisErrorT err = SA_AIS_ERR_BAD_OPERATION;
	char ccbIdStr[STRINT_BSZ];
	std::string sqlY("SELECT  epoch, commit_time FROM ccb_commits WHERE ccb_id = ");

	TRACE_ENTER2("get Outcome for ccb:%llu", ccbId);
	snprintf(ccbIdStr, STRINT_BSZ, "%llu", ccbId);
	sqlY.append(ccbIdStr);
	TRACE("GENERATED Y:%s", sqlY.c_str());

	rc = sqlite3_get_table(dbHandle, sqlY.c_str(), &result, &nrows, &ncols, &yErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlY.c_str(), yErr);
		sqlite3_free(yErr);
		goto bailout;
	}

	if(nrows != 1) {
		if(nrows > 1) {
			LOG_ER("Expected 1 row got %u rows", nrows);
			goto bailout;
		}
		LOG_NO("getCcbOutcomeFromPbe: Could not find ccb %llu presume ABORT", ccbId);
		err = SA_AIS_ERR_BAD_OPERATION;
	} else {
		unsigned int commitTime=0;
		unsigned int ccbEpoch=0;
		ccbEpoch = strtoul(result[ncols], NULL, 0);
		commitTime = strtoul(result[ncols+1], NULL, 0);
		LOG_NO("getCcbOutcomeFromPbe: Found ccb %llu epoch %u time:%u => COMMIT",
			ccbId, ccbEpoch, commitTime);
		if(ccbEpoch > currentEpoch) {
			LOG_ER("Recovered CCB has higher epoch (%u) than current epoch (%u) not allowed.",
				ccbEpoch, currentEpoch);
			exit(1);
		}
		err = SA_AIS_OK;
	}

	sqlite3_free_table(result);
	TRACE_LEAVE();
	return err;
 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting");
	exit(1);
}


#else
void* pbeRepositoryInit(const char* filePath, bool create)
{
	LOG_WA("immdump not built with the --enable-imm-pbe option.");
	return NULL;
}

void pbeRepositoryClose(void* dbHandle) 
{
	assert(0);
}

void pbeAtomicSwitchFile(const char* filePath)
{
	assert(0);
}

void dumpClassesToPbe(SaImmHandleT immHandle, ClassMap *classIdMap,
	void* db_handle)
{
	assert(0);
}

ClassInfo* classToPBE(std::string classNameString,
	SaImmHandleT immHandle,
	void* db_handle,
	unsigned int class_id)
{
	assert(0);
	return NULL;
}

void deleteClassToPBE(std::string classNameString, void* db_handle, 
	ClassInfo* theClass)
{
	assert(0);
}

unsigned int dumpObjectsToPbe(SaImmHandleT immHandle, ClassMap* classIdMap,
	void* db_handle)
{
	assert(0);
	return 0;
}

SaAisErrorT pbeBeginTrans(void* db_handle)
{
	return SA_AIS_ERR_NO_RESOURCES;
}

SaAisErrorT pbeCommitTrans(void* db_handle, SaUint64T ccbId, SaUint32T epoch)
{
	return SA_AIS_ERR_NO_RESOURCES;
}

void pbeAbortTrans(void* db_handle)
{
	assert(0);
}

void objectDeleteToPBE(std::string objectNameString, void* db_handle)
{
	assert(0);
}

void objectModifyDiscardAllValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	assert(0);
}

void objectModifyAddValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	assert(0);
}

void objectModifyDiscardMatchingValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	assert(0);
}

void objectToPBE(std::string objectNameString,
	const SaImmAttrValuesT_2** attrs,
	ClassMap *classIdMap,
	void* db_handle,
	unsigned int object_id,
	SaImmClassNameT className,
	SaUint64T ccb_id)
{
	assert(0);
}

SaAisErrorT getCcbOutcomeFromPbe(void* db_handle, SaUint64T ccbId, SaUint32T epoch)
{
	return SA_AIS_ERR_LIBRARY;
}

unsigned int verifyPbeState(SaImmHandleT immHandle, ClassMap *classIdMap, void* db_handle)
{
	assert(0);
	return 0;
}

void purgeCcbCommitsFromPbe(void* sDbHandle, SaUint32T currentEpoch)
{
	assert(0);
}

#endif
