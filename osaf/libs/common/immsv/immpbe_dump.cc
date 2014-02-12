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

#include "immpbe_dump.hh"
#include <errno.h>
#include <fcntl.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <wait.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdint.h>
#include <sys/stat.h>


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifdef HAVE_IMM_PBE

/* Spinlock for sqlite access see pbeBeginTrans.
   The lock will only be aquired in pbeBeginTrans().
   It is relased in either pbeCommitTrans() or pbeAbortTrans().
   PbeCommitTrans() is only accepted after pbeClosePrepareTrans()
*/
static volatile unsigned int sqliteTransLock=0;

bool pbeTransStarted()
{
	return sqliteTransLock!=0;
}

bool pbeTransIsPrepared()
{
	return sqliteTransLock==2;
}

void pbeClosePrepareTrans()
{
    if(sqliteTransLock != 1) {
        LOG_ER("pbePrepareTrans was called when sqliteTransLock(%u)!=1",
               sqliteTransLock);
        abort();
    }
    assert((++sqliteTransLock) == 2);
}

#include <sqlite3.h> 
#define STRINT_BSZ 32

static std::string sPbeFileName;

#define SQL_STMT_SIZE		31

enum {
/* INSERT */
	SQL_INS_OBJECTS_INT_MULTI = 0,
	SQL_INS_OBJECTS_REAL_MULTI,
	SQL_INS_OBJECTS_TEXT_MULTI,
	SQL_INS_CLASSES,
	SQL_INS_ATTR_DEF,
	SQL_INS_ATTR_DFLT_INT,
	SQL_INS_ATTR_DFLT_REAL,
	SQL_INS_ATTR_DFLT_TEXT,
	SQL_INS_OBJECTS,
	SQL_INS_CCB_COMMITS,
/* SELECT */
	SQL_SEL_OBJECTS_ID,
	SQL_SEL_OBJECTS_DN,
	SQL_SEL_CLASSES_ID,
	SQL_SEL_CLASSES_NAME,
	SQL_SEL_ATTR_DEF,
	SQL_SEL_CCB_COMMITS,
/* DELETE */
	SQL_DEL_CLASSES,
	SQL_DEL_OBJECTS,
	SQL_DEL_ATTR_DEF,
	SQL_DEL_ATTR_DFLT,
	SQL_DEL_OBJ_INT_MULTI_ID,
	SQL_DEL_OBJ_REAL_MULTI_ID,
	SQL_DEL_OBJ_TEXT_MULTI_ID,
	SQL_DEL_OBJ_INT_MULTI_NAME,
	SQL_DEL_OBJ_REAL_MULTI_NAME,
	SQL_DEL_OBJ_TEXT_MULTI_NAME,
	SQL_DEL_OBJ_INT_MULTI_VAL,
	SQL_DEL_OBJ_REAL_MULTI_VAL,
	SQL_DEL_OBJ_TEXT_MULTI_VAL,
	SQL_DEL_CCB_COMMITS,
/* UPDATE */
	SQL_UPD_OBJECTS
};

static const char *preparedSql[] = {
/* INSERT */
	"INSERT INTO objects_int_multi (obj_id, attr_name, int_val) VALUES (?, ?, ?)",
	"INSERT INTO objects_real_multi (obj_id, attr_name, real_val) VALUES (?, ?, ?)",
	"INSERT INTO objects_text_multi (obj_id, attr_name, text_val) VALUES (?, ?, ?)",
	"INSERT INTO classes (class_id, class_name, class_category) VALUES (?, ?, ?)",
	"INSERT INTO attr_def (class_id, attr_name, attr_type, attr_flags) VALUES (?, ?, ?, ?)",
	"INSERT INTO attr_dflt (class_id, attr_name, int_dflt) VALUES (?, ?, ?)",
	"INSERT INTO attr_dflt (class_id, attr_name, real_dflt) VALUES (?, ?, ?)",
	"INSERT INTO attr_dflt (class_id, attr_name, text_dflt) VALUES (?, ?, ?)",
	"INSERT INTO objects (obj_id, class_id, dn, last_ccb) VALUES (?, ?, ?, ?)",
	"INSERT INTO ccb_commits (ccb_id, epoch, commit_time) VALUES (?, ?, ?)",
/* SELECT */
	"SELECT obj_id FROM objects WHERE class_id = ?",
	"SELECT obj_id,class_id FROM objects WHERE dn = ?",
	"SELECT class_name FROM classes WHERE class_id = ?",
	"SELECT class_id FROM classes WHERE class_name = ?",
	"SELECT attr_type,attr_flags FROM attr_def WHERE class_id = ? AND attr_name = ?",
	"SELECT epoch, commit_time FROM ccb_commits WHERE ccb_id = ?",
/* DELETE */
	"DELETE FROM classes WHERE class_id = ?",
	"DELETE FROM objects WHERE obj_id = ?",
	"DELETE FROM attr_def WHERE class_id = ?",
	"DELETE FROM attr_dflt WHERE class_id = ?",
	"DELETE FROM objects_int_multi WHERE obj_id = ?",
	"DELETE FROM objects_real_multi WHERE obj_id = ?",
	"DELETE FROM objects_text_multi WHERE obj_id = ?",
	"DELETE FROM objects_int_multi WHERE obj_id = ? AND attr_name = ?",
	"DELETE FROM objects_real_multi WHERE obj_id = ? AND attr_name = ?",
	"DELETE FROM objects_text_multi WHERE obj_id = ? AND attr_name = ?",
	"DELETE FROM objects_int_multi WHERE obj_id = ? AND attr_name = ? AND int_val = ?",
	"DELETE FROM objects_real_multi WHERE obj_id = ? AND attr_name = ? AND real_val = ?",
	"DELETE FROM objects_text_multi WHERE obj_id = ? AND attr_name = ? AND text_val = ?",
	"DELETE FROM ccb_commits WHERE epoch <= ?",
/* UPDATE */
	"UPDATE objects SET last_ccb = ? WHERE obj_id = ?"
};


static sqlite3_stmt *preparedStmt[SQL_STMT_SIZE] = { NULL };


static int prepareSqlStatements(sqlite3 *dbHandle) {
	int i;
	int rc;

	for(i=0; i<SQL_STMT_SIZE; i++) {
		rc = sqlite3_prepare_v2(dbHandle, preparedSql[i], -1, &(preparedStmt[i]), NULL);
		if(rc != SQLITE_OK) {
			LOG_ER("Failed to prepare SQL statement for: %s", preparedSql[i]);
			return -1;
		}
	}

	return 0;
}

int finalizeSqlStatement(void *stmt) {
	int rc = SQLITE_OK;

	if(stmt) {
		rc = sqlite3_finalize((sqlite3_stmt *)stmt);
		if(rc != SQLITE_OK)
			LOG_WA("Failed to finalize SQL statement");
	}

	return rc;
}

static int finalizeSqlStatements() {
	int i;
	int rc = SQLITE_OK;
	int retCode = SQLITE_OK;

	for(i=0; i<SQL_STMT_SIZE; i++) {
		rc = sqlite3_finalize(preparedStmt[i]);
		if(rc != SQLITE_OK) {
			retCode = rc;
			LOG_WA("Failed to finalize SQL statement for: %s", preparedSql[i]);
		}
	}

	return retCode;
}

static int bindValue(sqlite3_stmt *stmt, int position, SaImmAttrValueT value, SaImmValueTypeT type) {
	SaNameT *name;
	char *str;
	SaAnyT *anyp;
	std::ostringstream ost;

    switch (type)
    {
        case SA_IMM_ATTR_SAINT32T:
        case SA_IMM_ATTR_SAUINT32T:
        	return sqlite3_bind_int(stmt, position, *((int *) value));
        case SA_IMM_ATTR_SAINT64T:
        case SA_IMM_ATTR_SAUINT64T:
        case SA_IMM_ATTR_SATIMET:
        	return sqlite3_bind_int64(stmt, position, *((long long *) value));
        case SA_IMM_ATTR_SAFLOATT:
        	return sqlite3_bind_double(stmt, position, (double)*((float *) value));
        case SA_IMM_ATTR_SADOUBLET:
        	return sqlite3_bind_double(stmt, position, *((double *) value));
        case SA_IMM_ATTR_SANAMET:
            name = (SaNameT *)value;
        	return sqlite3_bind_text(stmt, position, (char *)name->value, name->length, NULL);
        case SA_IMM_ATTR_SASTRINGT:
            str = *((SaStringT *) value);
        	return sqlite3_bind_text(stmt, position, str, -1, NULL);
        case SA_IMM_ATTR_SAANYT:
            anyp = (SaAnyT *) value;

            for (unsigned int i = 0; i < anyp->bufferSize; i++) {
                ost << std::hex
                    << (((int)anyp->bufferAddr[i] < 0x10)? "0":"")
                << (int)anyp->bufferAddr[i];
            }

            return sqlite3_bind_text(stmt, position, ost.str().c_str(), ost.str().length(), SQLITE_TRANSIENT);
    }

	return SQLITE_ERROR;
}

static int prepareClassInsertStmt(sqlite3 *dbHandle, const char *className, bool isClassRuntime, SaImmAttrDefinitionT_2 **attrDefinitions, sqlite3_stmt **stmt) {
	std::string sql;
	std::string sqlParam;
	bool attr_is_pure_rt = false;
	bool attr_is_multi = false;

	sql.append("INSERT INTO \"");
	sql.append(className);
	sql.append("\" (obj_id");

	sqlParam.append("@obj_id");

	for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
	{
		attr_is_pure_rt=false; /* This attribute is pure runtime?*/
		attr_is_multi=false;

		if ((*p)->attrFlags & SA_IMM_ATTR_RUNTIME) {
			if((*p)->attrFlags & SA_IMM_ATTR_PERSISTENT) {
				TRACE_2("Persistent runtime attribute %s", (*p)->attrName);
				isClassRuntime = false;
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
			sql.append(", \"");
			sql.append((*p)->attrName);
			sql.append("\"");
			sqlParam.append(", @");
			sqlParam.append((*p)->attrName);
		}
	}

	if(isClassRuntime) {
		*stmt = NULL;
		return SQLITE_OK;
	} else {
		sql.append(") VALUES (");
		sql.append(sqlParam);
		sql.append(")");

		return sqlite3_prepare_v2(dbHandle, sql.c_str(), -1, stmt, NULL);
	}
}

/**
 * @brief Append a string, escaping any quote characters in it
 * @param i_quote : [in] quote character to escape when appending the i_suffix
 *                       string
 * @param io_base : [in:out] string to be appended to
 * @param i_suffix : [in] string to append at the end of io_base
 *
 * Append the string i_suffix to the string io_base, replacing any occurrences
 * of the character i_quote with pairs of i_quote characters.
 *
 * @b Example:
 * @code
 * std::string io_base("Time is now ");
 * append_quoted_string('\'', io_base, "eight o'clock");
 * // io_base now contains the string "Time is now eight o''clock"
 * @endcode
 */
/*
static void append_quoted_string(char i_quote,
	std::string& io_base,
	const std::string& i_suffix)
{
	const char two_quotes[] = { i_quote, i_quote };
	std::string::size_type start = 0;
	std::string::size_type end;
	while ((end = i_suffix.find(i_quote, start)) != i_suffix.npos) {
		io_base.append(i_suffix.substr(start, end - start));
		io_base.append(two_quotes, sizeof(two_quotes));
		start = end + 1;
	}
	io_base.append(i_suffix.substr(start));
}
*/

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
			LOG_ER("Unknown value type %u. Exiting (line:%u)",
				p->attrValueType, __LINE__);
			sqlite3_close((sqlite3 *) dbHandle);
			exit(1);
	}
}

static void valuesToPBE(const SaImmAttrValuesT_2* p, 
	SaImmAttrFlagsT attrFlags, 
	int objId, void* db_handle)
{
	int rc = 0;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	int sqlIndex;
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
			stmt = preparedStmt[SQL_INS_OBJECTS_INT_MULTI];
			sqlIndex = SQL_INS_OBJECTS_INT_MULTI;
			break;

		case SA_IMM_ATTR_SAFLOATT:  
		case SA_IMM_ATTR_SADOUBLET: 
			stmt = preparedStmt[SQL_INS_OBJECTS_REAL_MULTI];
			sqlIndex = SQL_INS_OBJECTS_REAL_MULTI;
			break;

		case SA_IMM_ATTR_SANAMET:
		case SA_IMM_ATTR_SASTRINGT:
		case SA_IMM_ATTR_SAANYT:
			stmt = preparedStmt[SQL_INS_OBJECTS_TEXT_MULTI];
			sqlIndex = SQL_INS_OBJECTS_TEXT_MULTI;
			break;

		default:
			LOG_ER("Unknown value type: %u (line:%u)", p->attrValueType, __LINE__);
			goto bailout;
	}

	for (unsigned int i = 0; i < p->attrValuesNumber; i++)
	{
		rc = sqlite3_bind_int(stmt, 1, objId);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed to bind obj_id with error(%d): %s\n", preparedSql[sqlIndex], rc, sqlite3_errmsg(dbHandle));
			goto bailout;
		}
		rc = sqlite3_bind_text(stmt, 2, p->attrName, -1, NULL);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed to bind attr_name with error(%d): %s\n", preparedSql[sqlIndex], rc, sqlite3_errmsg(dbHandle));
			goto bailout;
		}
		rc = bindValue(stmt, 3, p->attrValues[i], p->attrValueType);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed to bind third parameter with error(%d)\n", preparedSql[sqlIndex], rc);
			goto bailout;
		}

		rc = sqlite3_step(stmt);
		if(rc != SQLITE_DONE) {
			LOG_ER("SQL statement ('%s') failed with error code: %d\n", preparedSql[sqlIndex], rc);
			goto bailout;
		}
		sqlite3_reset(stmt);
	}

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);	
}


/* 
   Replace the existing DB-file on global/shared FS with the newly generated
   DB-file. If the new DB file was generated on local FS (for performance 
   reasons) then first move it to global FS. 

   (1) Copy local-tmp-file from local FS to global-tmp-file on global FS.
       Remove local-tmp-file.

   (2) On global FS, unlink (remove) any old .prev link/file.

   (3) On global FS, Create a ".prev" link to the current DB file (i-node).
       The current DB file now has two links, the official and the .prev link.

   (4) On global FS, rename/move global-tmp-file onto the official DB file link.
       This will switch the file (i-node) pointed at by the official DB file link
       to the newly generated file.
       This is a file system local operation and the link switch is guaranteed
       atomic by the OS. The previous DB-file is still accessible via .prev link.
*/
void pbeAtomicSwitchFile(const char* filePath, std::string localTmpFilename)
{
	std::string globalTmpFilename;
	std::string globalJournalFilename;
	globalTmpFilename.append(filePath);
	globalTmpFilename.append(".tmp"); 
	globalJournalFilename.append(filePath);
	globalJournalFilename.append("-journal"); 
	//	int fd=(-1);
	std::string oldFilename;
	oldFilename.append(filePath);
	oldFilename.append(".prev"); 

	if(!localTmpFilename.empty()) {
		int rc=(-1);
		std::string shellCommand("/bin/cp ");
		shellCommand.append(localTmpFilename);
		shellCommand.append(" ");
		shellCommand.append(globalTmpFilename);
		rc = system(shellCommand.c_str());
		if(rc) {
			if(rc == (-1)) {
				LOG_ER("Invocation of system(%s) failed, "
					"cause:%s", shellCommand.c_str(), 
					strerror(errno));
			} else {
				LOG_ER("Exit status for '%s' is %u != 0, "
					"cause:%s", shellCommand.c_str(), rc,
					strerror(WEXITSTATUS(rc)));
			}
			unlink(localTmpFilename.c_str());
			unlink(globalTmpFilename.c_str());
			exit(1);
		}
		unlink(localTmpFilename.c_str());
		LOG_NO("Moved %s to %s", localTmpFilename.c_str(),
			globalTmpFilename.c_str());
	}

	/* A new globalTmp database file now exists on the shared directory. 
	   Now remove any old-old database file to make way for the double link
	   to the current (old) database file.
	*/

	if(unlink(oldFilename.c_str()) != 0) {
		TRACE_2("Failed to unlink %s  error:%s", 
			oldFilename.c_str(), strerror(errno));
	}

	/* Make an additional link to the curent (old) db file using
	   the oldFilename link.
	*/
	if(link(filePath, oldFilename.c_str()) != 0) {
		LOG_IN("Could not move %s to %s error:%s - ok for initial pbe-enable", 
			filePath, oldFilename.c_str(), strerror(errno));
	}

	/* Switch the official DB file link to the new file. */
	if(rename(globalTmpFilename.c_str(), filePath)!=0) {
		LOG_ER("Failed to move %s to %s error:%s",
			globalTmpFilename.c_str(), filePath, strerror(errno));
		exit(1);
	} 

	LOG_NO("Moved %s to %s", globalTmpFilename.c_str(), filePath);

	if(access(globalJournalFilename.c_str(), F_OK) != (-1)) {
		/* Remove -journal file */
		if(unlink(globalJournalFilename.c_str()) != 0) {
			LOG_ER("Failed to remove EXISTING obsolete journal file: %s ",
				globalJournalFilename.c_str());
		} else {
			LOG_NO("Removed obsolete journal file: %s ", globalJournalFilename.c_str());
			/* and remove corresponding imm.db.prev since it depends on the journal file */
			if(unlink(oldFilename.c_str()) != 0) {
				LOG_WA("Failed to remove %s ", oldFilename.c_str());
			} else {
				LOG_NO("Removed obsolete db file: %s ", oldFilename.c_str());
			}
		}
	}

}


void* pbeRepositoryInit(const char* filePath, bool create, std::string& localTmpFilename)
{
	int fd=(-1);
	sqlite3* dbHandle=NULL;
	std::string globalTmpFilename;
	localTmpFilename.clear();
	char * localTmpDir = NULL;
	int rc=0;
	SaImmRepositoryInitModeT rpi = (SaImmRepositoryInitModeT) 0;
	char **result=NULL;
	char *zErr=NULL;
	int nrows=0;
	int ncols=0;
	const char * sql = "select saImmRepositoryInit from SaImmMngt";
	bool badfile = false;

	const char * sql_tr[] = 
		{"PRAGMA journal_mode=TRUNCATE", /* sql_tr[0] also invoked by re_attach below. */
		 "BEGIN EXCLUSIVE TRANSACTION",
		 "CREATE TABLE pbe_rep_version (major integer, minor integer)",
		 "INSERT INTO pbe_rep_version (major, minor) values('1', '2')",
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

	/* Create a fresh Pbe-repository by dumping current imm contents to (local or global)
	   Tmp-File. 
	 */

	globalTmpFilename.append(filePath);
	globalTmpFilename.append(".tmp"); 
	localTmpDir = getenv("IMMSV_PBE_TMP_DIR");
	TRACE("TMP DIR:%s", localTmpDir);
	if(localTmpDir) {
		TRACE("IMMSV_PBE_TMP_DIR:%s", localTmpDir);
		localTmpFilename.append(localTmpDir);
		localTmpFilename.append("/imm.db.XXXXXX");
		unsigned int sz = localTmpFilename.size() +1;
		TRACE("localTmpFile:%s", localTmpFilename.c_str());
		if(1) {
			char buf[sz];
			strncpy(buf, localTmpFilename.c_str(), sz);
			fd = mkstemp(buf);
			localTmpFilename.clear();
			localTmpFilename.append(buf);
			if(fd != (-1)) {
				TRACE("Successfully created local tmp file %s", buf);
				close(fd);
				fd=(-1);
			} else {
				LOG_WA("Could not create local tmp file '%s' cause:%s - "
					"reverting to global tmp file", 
					buf, strerror(errno));
				localTmpDir = NULL;
				localTmpFilename.clear();
			}

		}
	}

	/* Existing new file indicates a previous failure.
	   We should possibly have some escalation mechanism here. */
	if(unlink(globalTmpFilename.c_str()) != 0) {
		TRACE_2("Failed to unlink %s  error:%s", 
			globalTmpFilename.c_str(), strerror(errno));
	}

	if(localTmpDir) {
		rc = sqlite3_open(localTmpFilename.c_str(), &dbHandle);
		if(rc) {
			LOG_ER("Can't open sqlite pbe local Tmp file '%s', cause:%s", 
				localTmpFilename.c_str(), sqlite3_errmsg(dbHandle));
			LOG_WA("Reverting to generate on global tmpFile:%s  ..may take time", 
				globalTmpFilename.c_str());
			localTmpDir = NULL;
			localTmpFilename.clear();
			rc = 0;
		} else {
			LOG_NO("Successfully opened empty local sqlite pbe file %s", localTmpFilename.c_str());
		}
	} 

	if(!localTmpDir)
	{ /* No local tmp file = > generate directly to global tmp file, likely to be slooow.. */
		rc = sqlite3_open(globalTmpFilename.c_str(), &dbHandle);
		if(rc) {
			LOG_ER("Can't open sqlite pbe file '%s', cause:%s", 
				globalTmpFilename.c_str(), sqlite3_errmsg(dbHandle));
			exit(1);
		}
		LOG_NO("Successfully opened empty global sqlite pbe file %s", globalTmpFilename.c_str());
	}

	/* Creating the schema. */
	for (int ix=0; sql_tr[ix]!=NULL; ++ix) {
		rc = sqlite3_exec(dbHandle, sql_tr[ix], NULL, NULL, &zErr);
		if (rc != SQLITE_OK) {
			LOG_ER("SQL statement %u/('%s') failed because:\n %s", ix, sql_tr[ix], zErr);
			sqlite3_free(zErr);
			goto bailout;
		}
		TRACE("Successfully executed %s", sql_tr[ix]);
	}

	prepareSqlStatements(dbHandle);

	sPbeFileName = std::string(filePath); 
	TRACE_LEAVE();
	return (void *) dbHandle;

 re_attach:

	/* Re-attach to an already created pbe file.
	   Before trying to open with sqlite3, check if the db file exists
	   and is writable. This avoids the sqlite3 default behavior of simply
	   succeeding with open and creating an empty db, when there is no db
	   file.
	*/

	if(access(filePath, R_OK | W_OK) == (-1)) {
		LOG_ER("File '%s' is not accessible for read/write, cause:%s - exiting", 
			filePath, strerror(errno));
		goto bailout;
	} else {
		std::string journalFile(filePath);
		journalFile.append("-journal");
		if(access(journalFile.c_str(), F_OK) != (-1)) {
			struct stat stat_buf;
			memset(&stat_buf, 0, sizeof(struct stat));
			if(stat(journalFile.c_str(), &stat_buf)==0) {
				if(stat_buf.st_size) {
					LOG_WA("Journal file %s of non zero size exists at "
						"start of PBE/immdump => sqlite recovery",
						journalFile.c_str());
				} else {
					TRACE_2("Journal file %s exists and is empty at "
						"start of PBE/immdump", journalFile.c_str());
				}
			} else {
				LOG_WA("Journal file %s exists but could not stat() at "
					"start of PBE/immdump",	journalFile.c_str());
			}
		}
	}

	rc = sqlite3_open(filePath, &dbHandle);
	if(rc) {
		LOG_ER("Can't open sqlite pbe file '%s', cause:%s", 
			filePath, sqlite3_errmsg(dbHandle));
		badfile = true;
		goto bailout;
	} 
	LOG_NO("Successfully opened pre-existing sqlite pbe file %s", filePath);

	rc = sqlite3_get_table((sqlite3 *) dbHandle, sql, &result, &nrows, 
		&ncols, &zErr);
	if(rc) {
		LOG_IN("Could not access table SaImmMngt, error:%s", zErr);
		sqlite3_free(zErr);
		badfile = true;
		goto bailout;
	}
	TRACE_2("Successfully accessed SaImmMngt table rows:%u cols:%u", 
		nrows, ncols);

	if(nrows == 0) {
		LOG_ER("SaImmMngt exists but is empty");
		badfile = true;
		goto bailout;
	} else if(nrows > 1) {
		LOG_WA("SaImmMngt has %u tuples, should only be one - using first tuple", nrows);
	}

	rpi = (SaImmRepositoryInitModeT) strtoul(result[1], NULL, 0);

	sqlite3_free_table(result);

	if( rpi == SA_IMM_KEEP_REPOSITORY) {
		LOG_IN("saImmRepositoryInit: SA_IMM_KEEP_REPOSITORY - attaching to repository");
	} else if(rpi == SA_IMM_INIT_FROM_FILE) {
		LOG_WA("saImmRepositoryInit: SA_IMM_INIT_FROM_FILE - will not attach!");
		goto bailout;
	} else {
		LOG_ER("saImmRepositoryInit: Not a valid value (%u) - can not attach", rpi);
		badfile=true;
		goto bailout;
	}

	rc = sqlite3_exec(dbHandle, sql_tr[0], NULL, NULL, &zErr);
	if (rc != SQLITE_OK) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql_tr[0], zErr);
		sqlite3_free(zErr);
		goto bailout;
	}
	TRACE("Successfully executed %s", sql_tr[0]);
	

	sPbeFileName = std::string(filePath); /* Avoid apend to presumed empty string */

	prepareSqlStatements(dbHandle);

	TRACE_LEAVE();
	return dbHandle;

 bailout:
	if(dbHandle) {
		sqlite3_close(dbHandle);
	}
	if(badfile) {
		discardPbeFile(std::string(filePath));
	}
	TRACE_LEAVE();
	return NULL;
}

void pbeRepositoryClose(void* dbHandle)
{
	finalizeSqlStatements();
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
	int rc=0;
	char *execErr=NULL;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	ClassInfo* classInfo = new ClassInfo(class_id);
	TRACE_ENTER();

	std::string sql;

	/* Get the class description */
	errorCode = saImmOmClassDescriptionGet_2(immHandle,
		(char*)classNameString.c_str(),
		&classCategory,
		&attrDefinitions);

	if (SA_AIS_OK != errorCode)
	{
		TRACE_4("Failed to get the description for the %s class error:%u - exiting",
			classNameString.c_str(), errorCode);
		goto bailout;
	}

	stmt = preparedStmt[SQL_INS_CLASSES];
	if(sqlite3_bind_int(stmt, 1, class_id) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id");
		goto bailout;
	}
	if(sqlite3_bind_text(stmt, 2, classNameString.c_str(), -1, NULL) != SQLITE_OK) {
		LOG_ER("Failed to bind class_name");
		goto bailout;
	}

	if(classCategory == SA_IMM_CLASS_CONFIG) {
		if(sqlite3_bind_int(stmt, 3, 1) != SQLITE_OK) {
			LOG_ER("Failed to bind class_category");
			goto bailout;
		}
		class_is_pure_runtime=false; /* true => no persistent rtattrs. */
	} else if (classCategory == SA_IMM_CLASS_RUNTIME) {
		TRACE("RUNTIME CLASS %s", (char*)classNameString.c_str());
		if(sqlite3_bind_int(stmt, 3, 2) != SQLITE_OK) {
			LOG_ER("Failed to bind class_category");
			goto bailout;
		}
	} else {
		LOG_ER("Class category %u for class %s is neither CONFIG nor RUNTIME",
			classCategory, classNameString.c_str());
		goto bailout;
	}

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed. Error code: %d", preparedSql[SQL_INS_CLASSES], rc);
		goto bailout;
	}
	sqlite3_reset(stmt);

	sql.append("CREATE TABLE \"");
	sql.append(classNameString);
	sql.append("\" (obj_id integer primary key");

	/* Attributes needed both in class def tuple and in instance relations. */
	for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
	{
		bool attr_is_pure_rt=false; /* This attribute is pure runtime?*/
		bool attr_is_multi=false;

		classInfo->mAttrMap[(*p)->attrName] = (*p)->attrFlags;
		TRACE("DUMPED Class %s Attr %s Flags %llx", classNameString.c_str(),
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
			sql.append(", \"");
			sql.append((*p)->attrName);
			sql.append("\"");
			typeToPBE(*p, dbHandle, &sql);
		}

		stmt = preparedStmt[SQL_INS_ATTR_DEF];
		if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
			LOG_ER("Failed to bind class_id with error code: %d", rc);
			goto bailout;
		}
		if((rc = sqlite3_bind_text(stmt, 2, (*p)->attrName, -1, NULL)) != SQLITE_OK) {
			LOG_ER("Failed to bind attr_name with error code: %d", rc);
			goto bailout;
		}
		if((rc = sqlite3_bind_int(stmt, 3, (*p)->attrValueType)) != SQLITE_OK) {
			LOG_ER("Failed to bind attr_type with error code: %d", rc);
			goto bailout;
		}
		if((rc = sqlite3_bind_int(stmt, 4, (*p)->attrFlags)) != SQLITE_OK) {
			LOG_ER("Failed to bind attr_flags with error code: %d", rc);
			goto bailout;
		}
		rc = sqlite3_step(stmt);
		if(rc != SQLITE_DONE) {
			LOG_ER("SQL statement('%s') failed with error code: %d",
					preparedSql[SQL_INS_ATTR_DEF], rc);
			goto bailout;
		}
		sqlite3_reset(stmt);

		if ((*p)->attrDefaultValue != NULL)
		{
			int stmt_id;
			switch ((*p)->attrValueType) {
				case SA_IMM_ATTR_SAINT32T:
				case SA_IMM_ATTR_SAUINT32T:
				case SA_IMM_ATTR_SAINT64T: 
				case SA_IMM_ATTR_SAUINT64T:
				case SA_IMM_ATTR_SATIMET:
					stmt_id = SQL_INS_ATTR_DFLT_INT;
					stmt = preparedStmt[SQL_INS_ATTR_DFLT_INT];
					break;

				case SA_IMM_ATTR_SAFLOATT:  
				case SA_IMM_ATTR_SADOUBLET: 
					stmt_id = SQL_INS_ATTR_DFLT_REAL;
					stmt = preparedStmt[SQL_INS_ATTR_DFLT_REAL];
					break;

				case SA_IMM_ATTR_SANAMET:
				case SA_IMM_ATTR_SASTRINGT:
				case SA_IMM_ATTR_SAANYT:
					stmt_id = SQL_INS_ATTR_DFLT_TEXT;
					stmt = preparedStmt[SQL_INS_ATTR_DFLT_TEXT];
					break;

				default:
					LOG_ER("Invalid attribte type %u", 
						(*p)->attrValueType);
					goto bailout;
			}

			if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
				LOG_ER("Failed to bind class_id with error code: %d", rc);
				goto bailout;
			}
			if((rc = sqlite3_bind_text(stmt, 2, (*p)->attrName, -1, NULL)) != SQLITE_OK) {
				LOG_ER("Failed to bind attr_name with error code: %d", rc);
				goto bailout;
			}
			if((rc = bindValue(stmt, 3, (*p)->attrDefaultValue, (*p)->attrValueType)) != SQLITE_OK) {
				LOG_ER("Failed to bind third parameter with error code: %d", rc);
				goto bailout;
			}
			rc = sqlite3_step(stmt);
			if(rc != SQLITE_DONE) {
				LOG_ER("SQL statement ('%s') failed because with error code: %d",
						preparedSql[stmt_id], rc);
				goto bailout;
			}
			sqlite3_reset(stmt);
		}
	}

	sql.append(")");
	if(class_is_pure_runtime) {
		TRACE_2("Class %s is pure runtime, no create of instance table",
			classNameString.c_str());
	} else {
		TRACE("GENERATED B: (%s)", sql.c_str());
		rc = sqlite3_exec(dbHandle, sql.c_str(), NULL, NULL, &execErr);
		if(rc != SQLITE_OK) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql.c_str(),
				execErr);
			sqlite3_free(execErr);
			goto bailout;
		}

		rc = prepareClassInsertStmt(dbHandle, classNameString.c_str(), class_is_pure_runtime, attrDefinitions, (sqlite3_stmt **)&(classInfo->sqlStmt));
		if(rc != SQLITE_OK) {
			TRACE_4("Failed to prepare SQL statement for '%s' table. Error: %u", classNameString.c_str(), rc);
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
	/* Dont remove imm.db file here, this could be a failed schema upgrade. */
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

void deleteClassToPBE(std::string classNameString, void* db_handle, 
	ClassInfo* theClass)
{
	TRACE_ENTER();
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	std::string sqlB("drop table \"");
	sqlite3_stmt *stmt;

	int rc=0;
	char *execErr=NULL;
	unsigned int rowsModified=0;

	/* 1. Verify zero instances of the class in objects relation. */
	stmt = preparedStmt[SQL_SEL_OBJECTS_ID];
	if((rc = sqlite3_bind_int(stmt, 1, theClass->mClassId)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW) {
		LOG_ER("Can not delete class %s when instances exist(%d):\n %s",
			classNameString.c_str(), rc, sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed with error(%d):\n %s",
				preparedSql[SQL_SEL_OBJECTS_ID], rc, sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	sqlite3_reset(stmt);

	/* 2. Remove classes tuple. */
	stmt = preparedStmt[SQL_DEL_CLASSES];
	if((rc = sqlite3_bind_int(stmt, 1, theClass->mClassId)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", preparedSql[SQL_DEL_CLASSES], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from classes", rowsModified);
	if(rowsModified != 1) {
		LOG_ER("Could not delete class tuple for class %s from relation 'classes'",
			classNameString.c_str());
		goto bailout;
	}
	sqlite3_reset(stmt);

	/* 3. Remove attr_def tuples. */
	stmt = preparedStmt[SQL_DEL_ATTR_DEF];
	if((rc = sqlite3_bind_int(stmt, 1, theClass->mClassId)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", preparedSql[SQL_DEL_ATTR_DEF], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from attr_def", rowsModified);
	if(rowsModified == 0) {
		LOG_WA("Could not delete attr_def tuples for class %s",
			classNameString.c_str());
		/* Dont bailout on this one. Could possibly be a degenerate class.*/
	}
	sqlite3_reset(stmt);

	/* 4. Remove attr_dflt tuples. */
	stmt = preparedStmt[SQL_DEL_ATTR_DFLT];
	if((rc = sqlite3_bind_int(stmt, 1, theClass->mClassId)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", preparedSql[SQL_DEL_ATTR_DFLT], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	rowsModified = sqlite3_changes(dbHandle);
	TRACE("Deleted %u tuples from attr_dflt", rowsModified);
	sqlite3_reset(stmt);

	/* 5. Drop 'classname' base relation. */
	sqlB.append(classNameString);
	sqlB.append("\"");
	TRACE("GENERATED B:%s", sqlB.c_str());
	rc = sqlite3_exec(dbHandle, sqlB.c_str(), NULL, NULL, &execErr);
	if(rc) {
		TRACE("SQL statement ('%s') failed because:\n %s", sqlB.c_str(), execErr);
		TRACE("Class apparently defined non-persistent runtime objects");
		sqlite3_free(execErr);
	} else {
		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Dropped table %s rows:%u", classNameString.c_str(), rowsModified);
	}

	TRACE_LEAVE();
	return;

bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

static ClassInfo* verifyClassPBE(std::string classNameString,
	SaImmHandleT immHandle,
	void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	int rc=0;
	unsigned int class_id = 0;
	ClassInfo* classInfo = NULL;
	sqlite3_stmt *stmt;

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

	stmt = preparedStmt[SQL_SEL_CLASSES_NAME];
	if((rc = sqlite3_bind_text(stmt, 1, classNameString.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_name with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_CLASSES_NAME], sqlite3_errmsg(dbHandle));
		goto bailout;
	}

	class_id = sqlite3_column_int(stmt, 0);
	/* check if there are more rows */
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		if(rc == SQLITE_ROW) {
			LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
			goto bailout;
		} else {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[SQL_SEL_CLASSES_NAME], sqlite3_errmsg(dbHandle));
			goto bailout;
		}
	}
	sqlite3_reset(stmt);

	classInfo = new ClassInfo(class_id);
	TRACE("ClassId:%u", class_id);

	/* Get the class description */
	errorCode = saImmOmClassDescriptionGet_2(immHandle,
		(char*)classNameString.c_str(),
		&classCategory,
		&attrDefinitions);

	if(errorCode != SA_AIS_OK) {
		TRACE_4("Failed to get class description for class %s from imm",
			classNameString.c_str());
		goto bailout;
	}


	for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
	{
		classInfo->mAttrMap[(*p)->attrName] = (*p)->attrFlags;
		TRACE("VERIFIED Class %s Attr%s Flags %llx", classNameString.c_str(),
			(*p)->attrName, (*p)->attrFlags);
	}

	if((rc = prepareClassInsertStmt(dbHandle, classNameString.c_str(),
			classCategory != SA_IMM_CLASS_CONFIG, attrDefinitions,
			(sqlite3_stmt **)&(classInfo->sqlStmt))) != SQLITE_OK)
	{
		LOG_ER("Failed to prepare class insert statement. Error code: %d", rc);
		saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
		goto bailout;
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
	sqlite3_reset(stmt);
	/*sqlite3_close((sqlite3 *) dbHandle);*/
	delete classInfo;
	LOG_WA("Verify class %s failed!", classNameString.c_str());
	return NULL;
}

void stampObjectWithCcbId(void* db_handle, const char* object_id,  SaUint64T ccb_id)
{
	int rc=0;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;

	TRACE_ENTER();

	stmt = preparedStmt[SQL_UPD_OBJECTS];
	if((rc = sqlite3_bind_int64(stmt, 1, ccb_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind last_ccb with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_int(stmt, 2, atoi(object_id))) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_UPD_OBJECTS], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_LEAVE();
	return;

bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	exit(1);
}

void objectModifyDiscardAllValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;

	int rc=0;
	char *zErr=NULL;
	int object_id;
	std::string object_id_str;
	int class_id;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	unsigned int rowsModified=0;
	sqlite3_stmt *stmt;
	bool badfile=false;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	stmt = preparedStmt[SQL_SEL_OBJECTS_DN];
	if((rc = sqlite3_bind_text(stmt, 1, objName.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind dn with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("Could not access object '%s' for modify, error:%s",
			objName.c_str(), sqlite3_errmsg(dbHandle));
		badfile=true;
		goto bailout;
	}

	object_id = sqlite3_column_int(stmt, 0);
	object_id_str.append((char *)sqlite3_column_text(stmt, 0));
	class_id = sqlite3_column_int(stmt, 1);

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed object '%s'", objName.c_str());
	TRACE_2("object_id:%d class_id:%d", object_id, class_id);

	/* Second, obtain the class description for the attribute. */
	stmt = preparedStmt[SQL_SEL_ATTR_DEF];
	if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_text(stmt, 2, attrValue->attrName, -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind attr_name with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	} else if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_ATTR_DEF], sqlite3_errmsg(dbHandle));
		goto bailout;
	}

	attr_type = (SaImmValueTypeT)sqlite3_column_int(stmt, 0);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = sqlite3_column_int(stmt, 1);

	rc = sqlite3_step(stmt);
	if(rc == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed attr_def for class_id:%d attr_name:%s. (T:%d, F:%lld)",
		class_id, attrValue->attrName, attr_type, attr_flags);

	if(ccb_id > 0x100000000LL) { /* PRTAttr case */
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
		int sqlIndex;

		TRACE_3("COMPONENT TEST BRANCH 1");
		/* Remove all values. */
		switch(attr_type) {
			case SA_IMM_ATTR_SAINT32T: 
			case SA_IMM_ATTR_SAUINT32T: 
			case SA_IMM_ATTR_SAINT64T:  
			case SA_IMM_ATTR_SAUINT64T: 
			case SA_IMM_ATTR_SATIMET:
				sqlIndex = SQL_DEL_OBJ_INT_MULTI_NAME;
				stmt = preparedStmt[SQL_DEL_OBJ_INT_MULTI_NAME];
				break;

			case SA_IMM_ATTR_SAFLOATT:
			case SA_IMM_ATTR_SADOUBLET:
				sqlIndex = SQL_DEL_OBJ_REAL_MULTI_NAME;
				stmt = preparedStmt[SQL_DEL_OBJ_REAL_MULTI_NAME];
				break;

			case SA_IMM_ATTR_SANAMET:
			case SA_IMM_ATTR_SASTRINGT:
			case SA_IMM_ATTR_SAANYT:
				sqlIndex = SQL_DEL_OBJ_TEXT_MULTI_NAME;
				stmt = preparedStmt[SQL_DEL_OBJ_TEXT_MULTI_NAME];
				break;

			default:
				LOG_ER("Unknown value type: %u (line:%u)", attr_type, __LINE__);
				goto bailout;
		}

		if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
			LOG_ER("Failed to bind obj_id with error code: %d", rc);
			goto bailout;
		}
		if((rc = sqlite3_bind_text(stmt, 2, attrValue->attrName, -1, NULL)) != SQLITE_OK) {
			LOG_ER("Failed to bind attr_name with error code: %d", rc);
			goto bailout;
		}
		rc = sqlite3_step(stmt);
		if(rc != SQLITE_DONE) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[sqlIndex], sqlite3_errmsg(dbHandle));
			goto bailout;
		}
		sqlite3_reset(stmt);

		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Deleted %u values", rowsModified);
	} else {
		/* Assign the null value to the single valued attribute. */
		std::string sql22("update \"");
		const char *class_name;

		/* Get the class-name for the object */
		stmt = preparedStmt[SQL_SEL_CLASSES_ID];
		if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
			LOG_ER("Failed to bind class_id with error code: %d", rc);
			goto bailout;
		}
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_DONE) {
			LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		} else if(rc != SQLITE_ROW) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[SQL_SEL_CLASSES_ID], sqlite3_errmsg(dbHandle));
			goto bailout;
		}

		class_name = (char *)sqlite3_column_text(stmt, 0);

		/* Update the relevant attribute in the class_name table */
		sql22.append(class_name);

		rc = sqlite3_step(stmt);
		if(rc == SQLITE_ROW) {
			LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		sqlite3_reset(stmt);

		sql22.append("\" set \"");
		sql22.append(attrValue->attrName);
		sql22.append("\" = NULL where obj_id =");
		sql22.append(object_id_str);

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
	if(rowsModified)
		stampObjectWithCcbId(db_handle, object_id_str.c_str(), ccb_id);

	TRACE_LEAVE();
	return;

 bailout:
	TRACE_LEAVE();
	sqlite3_close((sqlite3 *) dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

void objectModifyDiscardMatchingValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;

	int rc=0;
	std::string object_id_str;
	int object_id;
	int class_id;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	unsigned int rowsModified=0;
	bool badfile=false;

	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	stmt = preparedStmt[SQL_SEL_OBJECTS_DN];
	if((rc = sqlite3_bind_text(stmt, 1, objName.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind dn with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("Could not access object '%s' for modify, error:%s",
			objName.c_str(), sqlite3_errmsg(dbHandle));
		badfile=true;
		goto bailout;
	}

	object_id = sqlite3_column_int(stmt, 0);
	object_id_str.append((char *)sqlite3_column_text(stmt, 0));
	class_id = sqlite3_column_int(stmt, 1);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("object_id:%d class_id:%d", object_id, class_id);

	/* Second, obtain the class description for the attribute. */
	stmt = preparedStmt[SQL_SEL_ATTR_DEF];
	if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_text(stmt, 2, attrValue->attrName, -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind attr_name with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_ATTR_DEF], sqlite3_errmsg(dbHandle));
		goto bailout;
	}

	attr_type = (SaImmValueTypeT)sqlite3_column_int(stmt, 0);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = (SaImmAttrFlagsT)sqlite3_column_int64(stmt, 1);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed attr_def for class_id:%d attr_name:%s. (T:%d, F:%lld)",
		class_id, attrValue->attrName, attr_type, attr_flags);

	if(ccb_id > 0x100000000LL) { /* PRTAttr case */
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
		unsigned int ix;
		int sqlIndex;

		switch(attr_type) {
			case SA_IMM_ATTR_SAINT32T: 
			case SA_IMM_ATTR_SAUINT32T: 
			case SA_IMM_ATTR_SAINT64T:  
			case SA_IMM_ATTR_SAUINT64T: 
			case SA_IMM_ATTR_SATIMET:
				sqlIndex = SQL_DEL_OBJ_INT_MULTI_VAL;
				stmt = preparedStmt[SQL_DEL_OBJ_INT_MULTI_VAL];
				break;

			case SA_IMM_ATTR_SAFLOATT:
			case SA_IMM_ATTR_SADOUBLET:
				sqlIndex = SQL_DEL_OBJ_REAL_MULTI_VAL;
				stmt = preparedStmt[SQL_DEL_OBJ_REAL_MULTI_VAL];
				break;

			case SA_IMM_ATTR_SANAMET:
			case SA_IMM_ATTR_SASTRINGT:
			case SA_IMM_ATTR_SAANYT:
				sqlIndex = SQL_DEL_OBJ_TEXT_MULTI_VAL;
				stmt = preparedStmt[SQL_DEL_OBJ_TEXT_MULTI_VAL];
				break;

		default:
			LOG_ER("Unknown value type: %u (line:%u)", attr_type, __LINE__);
			goto bailout;
		}

		for(ix=0; ix < attrValue->attrValuesNumber; ++ix) {
			if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
				LOG_ER("Failed to bind obj_id with error code: %d", rc);
				goto bailout;
			}
			if((rc = sqlite3_bind_text(stmt, 2, attrValue->attrName, -1, NULL)) != SQLITE_OK) {
				LOG_ER("Failed to bind attr_name with error code: %d", rc);
				goto bailout;
			}
			if((rc = bindValue(stmt, 3, attrValue->attrValues[ix], attr_type)) != SQLITE_OK) {
				LOG_ER("Failed to bind third parameter in '%s' with error code: %d", preparedSql[sqlIndex], rc);
				goto bailout;
			}

			rc = sqlite3_step(stmt);
			if(rc != SQLITE_DONE) {
				LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[sqlIndex], sqlite3_errmsg(dbHandle));
				goto bailout;
			}

			rowsModified=sqlite3_changes(dbHandle);
			TRACE("Deleted %u values", rowsModified);

			sqlite3_reset(stmt);
		}
	} else {
		/* Assign the null value to the single valued attribute IFF
		   current value matches.
		 */
		unsigned int ix;
		std::string sql23("update \"");

		/* Get the class-name for the object */
		stmt = preparedStmt[SQL_SEL_CLASSES_ID];
		if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
			LOG_ER("Failed to bind class_id with error code: %d", rc);
			goto bailout;
		}
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_DONE) {
			LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		if(rc != SQLITE_ROW) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[SQL_SEL_CLASSES_ID], sqlite3_errmsg(dbHandle));
			goto bailout;
		}

		sql23.append((char *)sqlite3_column_text(stmt, 0));

		if(sqlite3_step(stmt) == SQLITE_ROW) {
			LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		sqlite3_reset(stmt);

		TRACE_2("Successfully accessed classes class_id:%d.", class_id);

		sql23.append("\" set \"");
		sql23.append(attrValue->attrName);
		sql23.append("\" = NULL where obj_id = ");
		sql23.append(object_id_str.c_str());
		sql23.append(" and \"");
		sql23.append(attrValue->attrName);
		sql23.append("\" = ?");

		rc = sqlite3_prepare_v2(dbHandle, sql23.c_str(), -1, &stmt, NULL);
		if(rc != SQLITE_OK) {
			LOG_ER("Failed to prepare SQL statement '%s' with erroc code: %d", sql23.c_str(), rc);
			goto bailout;
		}

		for(ix=0; ix < attrValue->attrValuesNumber; ++ix) {
			rc = bindValue(stmt, 1, attrValue->attrValues[ix], attr_type);
			if(rc != SQLITE_OK) {
				LOG_ER("Failed to bind '%s' parameter with error code: %d", attrValue->attrName, rc);
				goto bailout;
			}

			rc = sqlite3_step(stmt);
			if(rc != SQLITE_DONE) {
				LOG_ER("SQL statement ('%s') failed because:\n %s", sql23.c_str(), sqlite3_errmsg(dbHandle));
				goto bailout;
			}

			rowsModified=sqlite3_changes(dbHandle);
			TRACE("Update %u values", rowsModified);

			sqlite3_reset(stmt);
		}

		rc = sqlite3_finalize(stmt);
		if(rc != SQLITE_OK)
			LOG_NO("Failed to delete prepared SQL statement '%s' with error code: %d", sql23.c_str(), rc);
	}
 done:
	if(rowsModified) {
		stampObjectWithCcbId(db_handle, object_id_str.c_str(), ccb_id);
	}
	TRACE_LEAVE();
	return;

 bailout:
	TRACE_LEAVE();
	sqlite3_close((sqlite3 *) dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
 
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

void objectModifyAddValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;

	int rc=0;
	std::string object_id_str;
	int object_id;
	int class_id;
	SaImmValueTypeT attr_type;
	SaImmAttrFlagsT attr_flags;
	unsigned int rowsModified=0;
	bool badfile = false;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	stmt = preparedStmt[SQL_SEL_OBJECTS_DN];
	if((rc = sqlite3_bind_text(stmt, 1, objName.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind dn with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("Could not access object '%s' for modify, error:%s",
				objName.c_str(), sqlite3_errmsg(dbHandle));
		badfile=true;
		goto bailout;
	}

	object_id = sqlite3_column_int(stmt, 0);
	object_id_str.append((char *)sqlite3_column_text(stmt, 0));
	class_id = sqlite3_column_int(stmt, 1);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed object '%s'.", objName.c_str());
	TRACE_2("object_id:%d class_id:%d", object_id, class_id);


	/* Second, obtain the class description for the attribute. */
	stmt = preparedStmt[SQL_SEL_ATTR_DEF];
	if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_text(stmt, 2, attrValue->attrName, -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind attr_name with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_ATTR_DEF], sqlite3_errmsg(dbHandle));
		goto bailout;
	}

	attr_type = (SaImmValueTypeT)sqlite3_column_int(stmt, 0);
	assert(attr_type == attrValue->attrValueType);
	attr_flags = (SaImmAttrFlagsT)sqlite3_column_int64(stmt, 1);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed attr_def for class_id:%d attr_name:%s. (T:%d, F:%lld)",
		class_id, attrValue->attrName, attr_type, attr_flags);

	if(ccb_id > 0x100000000LL) { /* PRTAttr case */
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
		std::string class_name;
		std::string sql22("update \"");

		assert(attrValue->attrValuesNumber == 1);
		/* Get the class-name for the object */
		stmt = preparedStmt[SQL_SEL_CLASSES_ID];
		if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
			LOG_ER("Failed to bind class_id with error code: %d", rc);
			goto bailout;
		}
		rc = sqlite3_step(stmt);
		if(rc == SQLITE_DONE) {
			LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		if(rc != SQLITE_ROW) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
					preparedSql[SQL_SEL_CLASSES_ID], sqlite3_errmsg(dbHandle));
			goto bailout;
		}

		class_name.append((char *)sqlite3_column_text(stmt, 0));

		if(sqlite3_step(stmt) == SQLITE_ROW) {
			LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		sqlite3_reset(stmt);

		TRACE_2("Successfully accessed classes class_id:%d class_name:'%s'",
			class_id, class_name.c_str());

		/* Update the relevant attribute in the class_name table */
		/* We should check that the current value is NULL, but we assume instead
		   that the ImmModel has done this check. */
		sql22.append(class_name);
		sql22.append("\" set \"");
		sql22.append(attrValue->attrName);
		sql22.append("\" = ? where obj_id = ");
		sql22.append(object_id_str);

		rc = sqlite3_prepare_v2(dbHandle, sql22.c_str(), -1, &stmt, NULL);
		if(rc != SQLITE_OK) {
			LOG_ER("Failed to prepare SQL statement '%s' with error code: %d", sql22.c_str(), rc);
			goto bailout;
		}

		rc = bindValue(stmt, 1, attrValue->attrValues[0], attrValue->attrValueType);
		if(rc != SQLITE_OK) {
			LOG_ER("Failed to bind attr_name parameter with error code: %d", rc);
			sqlite3_finalize(stmt);
			goto bailout;
		}

		rc = sqlite3_step(stmt);
		if(rc != SQLITE_DONE) {
			LOG_ER("SQL statement ('%s') failed because:\n %s", sql22.c_str(), sqlite3_errmsg(dbHandle));
			sqlite3_finalize(stmt);
			goto bailout;
		}

		rowsModified = sqlite3_changes(dbHandle);
		TRACE("Updated %u values", rowsModified);

		sqlite3_finalize(stmt);
	}
 done:
	if(rowsModified) {
		stampObjectWithCcbId(db_handle, object_id_str.c_str(), ccb_id);
		/* Warning function call on line above refers to >>result<< via object_id */
	}

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

unsigned int purgeInstancesOfClassToPBE(SaImmHandleT immHandle, std::string className, void* db_handle)
{
	SaImmSearchHandleT     searchHandle;
	SaAisErrorT            errorCode;
	SaNameT                objectName;
	SaImmSearchParametersT_2 searchParam;
	SaImmAttrValuesT_2**   attrs=NULL;
	unsigned int           retryInterval = 300000; /* 0.3 sec */
	unsigned int           maxTries = 10;          /* 9 times =~ max 3 secs */
	unsigned int           tryCount=0;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	const char * classNamePar = className.c_str();
	unsigned int nrofDeletes=0;
	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = (SaImmAttrNameT) SA_IMM_ATTR_CLASS_NAME;
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &classNamePar;

	do {
		if(tryCount) {
			usleep(retryInterval);
		}
		++tryCount;

		errorCode = saImmOmSearchInitialize_2(immHandle, 
			NULL, 
			SA_IMM_SUBTREE,
			(SaImmSearchOptionsT)
			(SA_IMM_SEARCH_ONE_ATTR | SA_IMM_SEARCH_GET_NO_ATTR),
			&searchParam,
			NULL, 
			&searchHandle);

		TRACE_5("Search initialize returned %u", errorCode);
 
	} while ((errorCode == SA_AIS_ERR_TRY_AGAIN) &&
		(tryCount < maxTries)); 

	if (SA_AIS_OK != errorCode)
	{
		LOG_ER("Failed on saImmOmSearchInitialize:%u - exiting ", errorCode);
		goto bailout;
	}

	do
	{
		errorCode = saImmOmSearchNext_2(searchHandle, &objectName, &attrs);
		TRACE_5("Search next returned %u", errorCode);
		if (SA_AIS_OK != errorCode)
		{
			break;
		}

		//assert(attrs[0] == NULL);

		objectDeleteToPBE(std::string((const char *) objectName.value), db_handle);
		++nrofDeletes;
	} while (true);

	if (SA_AIS_ERR_NOT_EXIST != errorCode)
	{
		LOG_ER("Failed in saImmOmSearchNext_2:%u - exiting", errorCode);
		goto bailout;
	}

	saImmOmSearchFinalize(searchHandle);

	TRACE_LEAVE();
	return nrofDeletes;
 bailout:
	sqlite3_close(dbHandle);
	/* Nothing wrong with imm.db file */
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);	
}

unsigned int dumpInstancesOfClassToPBE(SaImmHandleT immHandle, ClassMap *classIdMap,
	std::string className, unsigned int* objIdCount, void* db_handle)
{
	unsigned int obj_count=0;
	SaImmSearchHandleT     searchHandle;
	SaAisErrorT            errorCode;
	SaNameT                objectName;
	SaImmSearchParametersT_2 searchParam;
	SaImmAttrValuesT_2**   attrs;
	unsigned int           retryInterval = 300000; /* 0.3 sec */
	unsigned int           maxTries = 10;          /* 9 times =~ max 3 secs */
	unsigned int           tryCount=0;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	const char * classNamePar = className.c_str();
	TRACE_ENTER();

	searchParam.searchOneAttr.attrName = (SaImmAttrNameT) SA_IMM_ATTR_CLASS_NAME;
	searchParam.searchOneAttr.attrValueType = SA_IMM_ATTR_SASTRINGT;
	searchParam.searchOneAttr.attrValue = &classNamePar;

	do {
		if(tryCount) {
			usleep(retryInterval);
		}
		++tryCount;

		errorCode = saImmOmSearchInitialize_2(immHandle, 
			NULL,
			SA_IMM_SUBTREE,
			(SaImmSearchOptionsT)
			(SA_IMM_SEARCH_ONE_ATTR | 
				SA_IMM_SEARCH_GET_ALL_ATTR |
				SA_IMM_SEARCH_PERSISTENT_ATTRS),//Special & nonstandard
			&searchParam,
			NULL, 
			&searchHandle);
	} while ((errorCode == SA_AIS_ERR_TRY_AGAIN) &&
		(tryCount < maxTries)); 

	if (SA_AIS_OK != errorCode)
	{
		LOG_ER("Failed on saImmOmSearchInitialize:%u - exiting ", errorCode);
		goto bailout;
	}

	do
	{
		errorCode = saImmOmSearchNext_2(searchHandle, &objectName, &attrs);

		if (SA_AIS_OK != errorCode)
		{
			break;
		}

		assert(attrs[0] != NULL);

		objectToPBE(std::string((const char*)objectName.value),
			(const SaImmAttrValuesT_2**) attrs, classIdMap, dbHandle, 
			++(*objIdCount), (SaImmClassNameT) className.c_str(), 0);

		++obj_count;

	} while (true);

	if (SA_AIS_ERR_NOT_EXIST != errorCode)
	{
		LOG_ER("Failed in saImmOmSearchNext_2:%u - exiting", errorCode);
		goto bailout;
	}

	saImmOmSearchFinalize(searchHandle);

	TRACE_LEAVE();
	return obj_count;
 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}


void objectDeleteToPBE(std::string objectNameString, void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	std::string sql("delete from \"");

	int rc=0;
	char *zErr=NULL;
	std::string object_id_str;
	int object_id;
	int class_id;
	std::string class_name;
	bool badfile=false;
	TRACE_ENTER();
	assert(dbHandle);

	/* First, look up obj_id and class_id  from objects where dn == objname. */
	stmt = preparedStmt[SQL_SEL_OBJECTS_DN];
	if((rc = sqlite3_bind_text(stmt, 1, objectNameString.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind dn with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("Could not access object '%s' for delete, error:%s",
				objectNameString.c_str(), sqlite3_errmsg(dbHandle));
		badfile=true;
		goto bailout;
	}

	object_id = sqlite3_column_int(stmt, 0);
	object_id_str.append((char *)sqlite3_column_text(stmt, 0));
	class_id = sqlite3_column_int(stmt, 1);

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed object '%s'.", objectNameString.c_str());
	TRACE_2("object_id:%d class_id:%d", object_id, class_id);

	/*
	  Second, delete the root object tuple in objects for obj_id. 
	*/
	stmt = preparedStmt[SQL_DEL_OBJECTS];
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_DEL_OBJECTS], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	sqlite3_reset(stmt);
	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	/* Third get the class-name for the object */
	stmt = preparedStmt[SQL_SEL_CLASSES_ID];
	if((rc = sqlite3_bind_int(stmt, 1, class_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		LOG_ER("Expected 1 row got 0 rows (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_CLASSES_ID], sqlite3_errmsg(dbHandle));
		goto bailout;
	}

	class_name.append((char *)sqlite3_column_text(stmt, 0));

	if(sqlite3_step(stmt) == SQLITE_ROW) {
		LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
		badfile=true;
		goto bailout;
	}
	sqlite3_reset(stmt);

	TRACE_2("Successfully accessed classes class_id:%d class_name:'%s'",
		class_id, class_name.c_str());

	/* Fourth delete the base attribute tuple from table 'classname' for obj_id.
	 */
	sql.append(class_name);
	sql.append("\" where obj_id = ");
	sql.append(object_id_str);

	TRACE("GENERATED 4:%s", sql.c_str());
	rc = sqlite3_exec(dbHandle, sql.c_str(), NULL, NULL, &zErr);
	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sql.c_str(), zErr);
		sqlite3_free(zErr);
		goto bailout;
	}

	TRACE("Deleted %u values", sqlite3_changes(dbHandle));

	/*
	  Fifth delete from objects_int_multi, objects_real_multi, objects_text_multi where
	  obj_id ==OBJ_ID
	 */
	stmt = preparedStmt[SQL_DEL_OBJ_INT_MULTI_ID];
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_DEL_OBJ_INT_MULTI_ID], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	TRACE("Deleted %u values", sqlite3_changes(dbHandle));
	sqlite3_reset(stmt);

	stmt = preparedStmt[SQL_DEL_OBJ_REAL_MULTI_ID];
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_DEL_OBJ_REAL_MULTI_ID], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	TRACE("Deleted %u values", sqlite3_changes(dbHandle));
	sqlite3_reset(stmt);

	stmt = preparedStmt[SQL_DEL_OBJ_TEXT_MULTI_ID];
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_DEL_OBJ_TEXT_MULTI_ID], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	TRACE("Deleted %u values", sqlite3_changes(dbHandle));
	sqlite3_reset(stmt);

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close((sqlite3 *) dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
	LOG_ER("Exiting (line:%u)", __LINE__);
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
	unsigned int class_id=0;
	ClassInfo* classInfo=NULL;
	int rc=0;
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	bool rdnFound=false;

	std::string sqlAttr;

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

	if(!classInfo->sqlStmt) {
		LOG_ER("Insert statement for class '%s' is not prepared", classNameString.c_str());
		goto bailout;
	}

	stmt = preparedStmt[SQL_INS_OBJECTS];
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind obj_id with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_int(stmt, 2, class_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind class_id with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_text(stmt, 3, objectNameString.c_str(), -1, NULL)) != SQLITE_OK) {
		LOG_ER("Failed to bind dn with error code: %d", rc);
		goto bailout;
	}
	if((rc = sqlite3_bind_int64(stmt, 4, ccb_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind last_ccb with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement('%s') failed with error code: %d",
				preparedSql[SQL_INS_OBJECTS], rc);
		goto bailout;
	}
	sqlite3_reset(stmt);

	stmt = (sqlite3_stmt *)classInfo->sqlStmt;
	if((rc = sqlite3_bind_int(stmt, 1, object_id)) != SQLITE_OK) {
		LOG_ER("Failed to bind last_ccb with error code: %d", rc);
		goto bailout;
	}

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

		if(attrFlags & SA_IMM_ATTR_RDN) {
			if(rdnFound) {
				LOG_ER("Duplicate RDN attribute");
				goto bailout;
			}
			rdnFound = true;
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
			valuesToPBE(*p, attrFlags, object_id, db_handle);
		} else {
			if((*p)->attrValuesNumber != 1) {
				LOG_ER("attrValuesNumber not 1: %u",
					(*p)->attrValuesNumber);
				assert((*p)->attrValuesNumber == 1);
			}

			sqlAttr.clear();
			sqlAttr.append("@").append(attName);

			rc = bindValue(stmt,
					sqlite3_bind_parameter_index(stmt, sqlAttr.c_str()),
					*((*p)->attrValues),
					(*p)->attrValueType);
			if(rc != SQLITE_OK) {
				LOG_ER("Cannot bind '%s' parameter. Error code: %u",
						attName.c_str(), rc);
				goto bailout;
			}
		}
	}

	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL object statement for table '%s' failed with error code: %d\n", classNameString.c_str(), rc);
		goto bailout;
	}
	sqlite3_reset(stmt);
	sqlite3_clear_bindings(stmt);

	TRACE_LEAVE();
	return;
 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
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

	rc = sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
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

	rc = sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	fsyncPbeJournalFile();

	TRACE_LEAVE();
	return;

 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
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
	bool badfile=false;
	TRACE_ENTER();

	classNameList = getClassNames(immHandle);
	it = classNameList.begin();

	rc = sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
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
			badfile=true;
			goto bailout;
		}
	}

	rc = sqlite3_get_table(dbHandle, sqlQ.c_str(), &result, &nrows, &ncols, &qErr);

	if(rc) {
		LOG_ER("SQL statement ('%s') failed because:\n %s", sqlQ.c_str(), qErr);
		sqlite3_free(qErr);
		badfile=true;
		goto bailout;
	}

	if(nrows != 1) {
		LOG_ER("Expected 1 row got %u rows (line: %u)", nrows, __LINE__);
		badfile = true;
		goto bailout;
	}

	obj_count = strtoul(result[ncols], NULL, 0);
	TRACE("verifPbeState: obj_count:%u", obj_count);

	rc = sqlite3_exec(dbHandle, "ROLLBACK", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('ROLLBACK') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}
	sqlite3_free_table(result);

	TRACE_LEAVE();
	return obj_count;

 bailout:
	sqlite3_free_table(result);
	sqlite3_close(dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
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

	rc = sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
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
			(const SaImmAttrValuesT_2**) attrs, classIdMap, dbHandle, ++object_id, 
			NULL, 0);
	} while (true);

	if (SA_AIS_ERR_NOT_EXIST != errorCode)
	{
		LOG_ER("Failed in saImmOmSearchNext_2:%u - exiting", errorCode);
		goto bailout;
	}

	rc = sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		goto bailout;
	}

	fsyncPbeJournalFile();

	/* End the search */
	saImmOmSearchFinalize(searchHandle);
	TRACE_LEAVE();
	return object_id; /* == number of dumped objects */
 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

SaAisErrorT pbeBeginTrans(void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;
	unsigned int try_count=0;
	unsigned int max_tries=20;

	/* Sqlite is supposedly threadsafe, but we dont want to rely on that.

	   With 2PBE, the single sqlite handle is used from both the main thread
           and the runtime thread, but only in in the *slave* PBE. The main thread
	   is also called the applier-thread in the slave because the slave receives
	   all regular ccb-oi-callbacks in the applier role. The other thread called
	   the runtime thread is used by the slave to receives PRT operations and 
	   requests from the primary to begin the commit protocol for ccbs.

	   PRTO create and PRTA update are the simplest since they consist of just one
	   operation request to be immediately commited. They are handled as one callback
	   in the primary and in the slave done in paralell. Imm-ram counts in the two
	   replies before committing the operations in the cluster. If replies dont
	   arrive in time, the imm-ram will abort the operation and initiate restart
	   of the PBE that failed to reply, (with regeneration of imm.db file).

	   Regular ccbs are more complex since they in general consist of several
	   operations and several callbacks. PRTO deletes are also complex because
	   they can be cascading deletes and can thus involve many callbacks to complete.
	   PRTO deletes are handled internally by the imm similarly to regular ccbs.
	   
	   The primary PBE still only uses the sqlite handle from the main thread,
           just as in regular 1PBE. 

	   The distributed protocol between primary PBE and slave PBE has been designed
	   so that the two threads (in the slave) will never deadlock. The thread that 
	   ocasionally has to wait for a lock should normally not need to wait for long
	   because the sqlite lock is only obtained when the PBE has all information it needs
	   to build and commit the complete ccb. Thus the thread that has the lock would 
	   only itself block on problems with sqlite or below (i.e. the file system).
	   Worst case for the waiting thread is that it times out and has to abort the ccb.

	   For CCBs the entry into the commit protocol at the slave is guarded not just by
	   sqliteTransLock (which is obtained only when sqlite processing really starts)
	   but also by the volatile variables. s2PbeBCcbToCompleteAtB, s2PbeBCcbOpCountNowAtB
	   and s2PbeBCcbUtilCcbData in immpbe_daemon.cc. 

	   This sqliteTranslock also catches logical errors not necessarily related to
	   multiple threads. Attempts to commit a non started transaction, or attempts to
	   start a transaction when the previous has not yet been terminated by commit or
	   abort, are caught.

	   If there is a threading issue, it would be in reltion to the ccbUitls access in
	   the slave. The ccbUtils may be mutating by operation calls received by the applier
	   thread (for several ccbs developing concurently). At the same time the runtime
	   thread in the applier may initiate the processing of a ccb-commit (at most one 
	   at a time). That ccb-commit has to pick up the ccbUtils pointer to its CCB.
	   The lookup (search for the root structure for the ccb) into the ccbUtils is
	   done without any locking. The lookup, if apparently succeeding will get a pointer
	   to the ccb root structure. A check is made here that the ccb-id member matches.

	   This can go wrong, but it is (a) unlikely and (b) will result in failure or crash,
	   not inconsistent commit. 
	   Failure here means that the ccb will get aborted in the system. But if the lookup
	   succeeds and the object pointed to is identifiable as being for the correct ccb,
	   then all the rest should be safe. The ccb structure is only deallocated much later
	   after slite commit, by the applier thread when it receives the apply callback. 
	
	   The sensitive pointer lookup is done only once per ccb, and the pointer is saved 
	   in s2PbeBCcbUtilCcbData. Only one CCB can be committing at a time, even if several
	   can be building up. 

	   PRTO deletes are handled similarly to regular CCBs. The same structures and 
	   callbacks are used as for regular CCBS. BUT, it is the runtime thread in the 
	   gets all the callbacks. 
	   
	*/
	while(sqliteTransLock) {
		LOG_WA("Sqlite db locked by other thread.");
		if(try_count < max_tries) {
			usleep(500000);
			++try_count;
		} else {
			LOG_ER("Sqlite db appears blocked on other transaction");
			return SA_AIS_ERR_FAILED_OPERATION;
		}
	}

	++sqliteTransLock; /* Lock is set. */
        if(sqliteTransLock != 1) { /* i.e. not 2 or 3 */
            LOG_ER("Failure in obtaining sqliteTransLock: %u", sqliteTransLock);
            return SA_AIS_ERR_FAILED_OPERATION;
        }

	rc = sqlite3_exec(dbHandle, "BEGIN EXCLUSIVE TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('BEGIN EXCLUSIVE TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		return SA_AIS_ERR_FAILED_OPERATION;
	}
	return SA_AIS_OK;
}

SaAisErrorT pbeCommitTrans(void* db_handle, SaUint64T ccbId, SaUint32T currentEpoch, SaTimeT *externCommitTime)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;
	unsigned int commitTime=0;
	time_t now = time(NULL);
	SaAisErrorT err = SA_AIS_OK;

	if(sqliteTransLock != 2) {
		LOG_ER("pbeCommitTrans was called when sqliteTransLock(%u)!=2", sqliteTransLock);
		abort();
	}

	assert((++sqliteTransLock) == 3);


	if(ccbId) {
		sqlite3_stmt *stmt = preparedStmt[SQL_INS_CCB_COMMITS];

		commitTime = (unsigned int) now;
		*externCommitTime = commitTime * SA_TIME_ONE_SECOND;

		if((rc = sqlite3_bind_int64(stmt, 1, ccbId)) != SQLITE_OK) {
			LOG_ER("Failed to bind ccb_id with error code: %d", rc);
			err = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
		if((rc = sqlite3_bind_int(stmt, 2, currentEpoch)) != SQLITE_OK) {
			LOG_ER("Failed to bind epoch with error code: %d", rc);
			err = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
		if((rc = sqlite3_bind_int(stmt, 3, commitTime)) != SQLITE_OK) {
			LOG_ER("Failed to bind commit_time with error code: %d", rc);
			err = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
		rc = sqlite3_step(stmt);
		if(rc != SQLITE_DONE) {
			LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_INS_CCB_COMMITS], sqlite3_errmsg(dbHandle));
			pbeAbortTrans(db_handle);
			err = SA_AIS_ERR_FAILED_OPERATION;
			goto done;
		}
		sqlite3_reset(stmt);
	}

	rc = sqlite3_exec(dbHandle, "COMMIT TRANSACTION", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('COMMIT TRANSACTION') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		err = SA_AIS_ERR_FAILED_OPERATION;
		goto done;
	}

 done:
	--sqliteTransLock; 
	--sqliteTransLock; 
	--sqliteTransLock; /* Lock is released. */
	fsyncPbeJournalFile(); /* This should not be needed. sqlite does double fsync itself */
	return err;
}

void purgeCcbCommitsFromPbe(void* db_handle, SaUint32T currentEpoch)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	int rc=0;

	if(currentEpoch < 11) return;

	stmt = preparedStmt[SQL_DEL_CCB_COMMITS];
	if((rc = sqlite3_bind_int(stmt, 1, currentEpoch - 10)) != SQLITE_OK) {
		LOG_ER("Failed to bind epoch with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc != SQLITE_DONE) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_DEL_CCB_COMMITS], sqlite3_errmsg(dbHandle));
		goto bailout;
	}
	TRACE("Deleted %u ccb commits", sqlite3_changes(dbHandle));
	sqlite3_reset(stmt);

	return;

 bailout:
	sqlite3_close(dbHandle);
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

void pbeAbortTrans(void* db_handle)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	char *execErr=NULL;
	int rc=0;

	rc = sqlite3_exec(dbHandle, "ROLLBACK", NULL, NULL, &execErr);
	if(rc != SQLITE_OK) {
		LOG_ER("SQL statement ('ROLLBACK') failed because:\n %s",
			execErr);
		sqlite3_free(execErr);
		sqlite3_close(dbHandle);
		LOG_ER("Exiting (line:%u)", __LINE__);
		exit(1);
	}

	if(sqliteTransLock == 0) {
		LOG_WA("pbeAbortTrans was called when sqliteTransLock==0");
	}

        switch(sqliteTransLock) {
            case 3:
                --sqliteTransLock;
            case 2:
                --sqliteTransLock;
            case 1:
                --sqliteTransLock;
                break;

            default:
                LOG_ER("Illegal value on sqliteTransLock:%u", sqliteTransLock);
                abort();

        }
}

SaAisErrorT getCcbOutcomeFromPbe(void* db_handle, SaUint64T ccbId, SaUint32T currentEpoch)
{
	sqlite3* dbHandle = (sqlite3 *) db_handle;
	sqlite3_stmt *stmt;
	int rc=0;
	SaAisErrorT err = SA_AIS_ERR_BAD_OPERATION;
	bool badfile=false;

	TRACE_ENTER2("get Outcome for ccb:%llu", ccbId);

	stmt = preparedStmt[SQL_SEL_CCB_COMMITS];
	if((rc = sqlite3_bind_int64(stmt, 1, ccbId)) != SQLITE_OK) {
		LOG_ER("Failed to bind ccb_id with error code: %d", rc);
		goto bailout;
	}
	rc = sqlite3_step(stmt);
	if(rc == SQLITE_DONE) {
		sqlite3_reset(stmt);
		LOG_NO("getCcbOutcomeFromPbe: Could not find ccb %llu presume ABORT", ccbId);
		err = SA_AIS_ERR_BAD_OPERATION;
	} else if(rc != SQLITE_ROW) {
		LOG_ER("SQL statement ('%s') failed because:\n %s",
				preparedSql[SQL_SEL_CCB_COMMITS], sqlite3_errmsg(dbHandle));
		badfile=true;
		goto bailout;
	} else {
		unsigned int commitTime;
		unsigned int ccbEpoch;

		ccbEpoch = (unsigned int)sqlite3_column_int64(stmt, 0);
		commitTime = (unsigned int)sqlite3_column_int64(stmt, 1);
		if(sqlite3_step(stmt) == SQLITE_ROW) {
			LOG_ER("Expected 1 row got more then 1 row (line: %u)", __LINE__);
			badfile=true;
			goto bailout;
		}
		sqlite3_reset(stmt);

		LOG_NO("getCcbOutcomeFromPbe: Found ccb %llu epoch %u time:%u => COMMIT",
				ccbId, ccbEpoch, commitTime);
		if(ccbEpoch > currentEpoch) {
			LOG_ER("Recovered CCB has higher epoch (%u) than current epoch (%u) not allowed.",
				ccbEpoch, currentEpoch);
			badfile = true;
			goto bailout;
		}
		err = SA_AIS_OK;
	}

	TRACE_LEAVE();
	return err;
 bailout:
	sqlite3_close(dbHandle);
	if(badfile) {
		discardPbeFile(sPbeFileName);
	}
	LOG_ER("Exiting (line:%u)", __LINE__);
	exit(1);
}

void fsyncPbeJournalFile()
{
	int fd=(-1);
	std::string globalJournalFilename(sPbeFileName);
	globalJournalFilename.append("-journal");
	fd = open(globalJournalFilename.c_str(), O_RDWR);
	if(fd != (-1)) {
		/* Sync the -journal file */
		if(fdatasync(fd)==(-1)) {
			LOG_WA("Failed to fdatasync %s ", globalJournalFilename.c_str());
		}
		close(fd);
	}
}

#else

bool pbeTransStarted()
{
	return false;
}

bool pbeTransIsPrepared()
{
	return false;
}


void* pbeRepositoryInit(const char* filePath, bool create, std::string& localTmpFilename)
{
	LOG_WA("immdump/osafimmpbed not built with the --enable-imm-pbe option.");
	return NULL;
}

void pbeRepositoryClose(void* dbHandle) 
{
	/* Dont abort, can be invoked from sigterm_handler */
}

void pbeAtomicSwitchFile(const char* filePath, std::string localTmpFilename)
{
	abort();
}

void dumpClassesToPbe(SaImmHandleT immHandle, ClassMap *classIdMap,
	void* db_handle)
{
	abort();
}

unsigned int purgeInstancesOfClassToPBE(SaImmHandleT immHandle, std::string className, void* db_handle)
{
	abort();
	return 0;
}

unsigned int dumpInstancesOfClassToPBE(SaImmHandleT immHandle, ClassMap *classIdMap,
	std::string className, unsigned int* objIdCount, void* db_handle)
{
	abort();
	return 0;
}

int finalizeSqlStatement(void *stmt) {
	abort();
	return 0;
}

ClassInfo* classToPBE(std::string classNameString,
	SaImmHandleT immHandle,
	void* db_handle,
	unsigned int class_id)
{
	abort();
	return NULL;
}

void deleteClassToPBE(std::string classNameString, void* db_handle, 
	ClassInfo* theClass)
{
	abort();
}

unsigned int dumpObjectsToPbe(SaImmHandleT immHandle, ClassMap* classIdMap,
	void* db_handle)
{
	abort();
	return 0;
}

SaAisErrorT pbeBeginTrans(void* db_handle)
{
	return SA_AIS_ERR_NO_RESOURCES;
}

SaAisErrorT pbeCommitTrans(void* db_handle, SaUint64T ccbId, SaUint32T epoch, SaTimeT *externCommitTime)
{
	return SA_AIS_ERR_NO_RESOURCES;
}

void pbeAbortTrans(void* db_handle)
{
	abort();
}

void pbeClosePrepareTrans()
{
    abort();
}

void objectDeleteToPBE(std::string objectNameString, void* db_handle)
{
	abort();
}

void objectModifyDiscardAllValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	abort();
}

void objectModifyAddValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	abort();
}

void objectModifyDiscardMatchingValuesOfAttrToPBE(void* db_handle, std::string objName, 
	const SaImmAttrValuesT_2* attrValue, SaUint64T ccb_id)
{
	abort();
}

void objectToPBE(std::string objectNameString,
	const SaImmAttrValuesT_2** attrs,
	ClassMap *classIdMap,
	void* db_handle,
	unsigned int object_id,
	SaImmClassNameT className,
	SaUint64T ccb_id)
{
	abort();
}

SaAisErrorT getCcbOutcomeFromPbe(void* db_handle, SaUint64T ccbId, SaUint32T epoch)
{
	return SA_AIS_ERR_LIBRARY;
}

unsigned int verifyPbeState(SaImmHandleT immHandle, ClassMap *classIdMap, void* db_handle)
{
	abort();
	return 0;
}

void purgeCcbCommitsFromPbe(void* sDbHandle, SaUint32T currentEpoch)
{
	abort();
}

void fsyncPbeJournalFile()
{
	abort();
}

#endif

/* Note: a version of this function exists as 'escalatePbe()' in 
   imm_pbe_load.cc */
void discardPbeFile(std::string filename)
{
	if(filename.empty()) {return;}
	std::string newFilename(filename);
	newFilename.append(".failed_immdump");
	std::string globalJournalFilename(filename);
	globalJournalFilename.append("-journal");
	

	if(rename(filename.c_str(), newFilename.c_str())!=0) {
		LOG_ER("Failed to rename %s to %s error:%s", 
			filename.c_str(), newFilename.c_str(),
			strerror(errno));
		return;
	} else {
		LOG_NO("Renamed %s to %s because it has been detected to be corrupt.", 
			filename.c_str(), newFilename.c_str());
	}

	if(access(globalJournalFilename.c_str(), F_OK) != (-1)) {
		/* Remove -journal file */
		if(unlink(globalJournalFilename.c_str()) != 0) {
			LOG_ER("Failed to remove EXISTING obsolete journal file: %s ",
				globalJournalFilename.c_str());
		} else {
			LOG_NO("Removed obsolete journal file: %s ", globalJournalFilename.c_str());
		}
	}

}


std::string getClassName(const SaImmAttrValuesT_2** attrs)
{
	int i;
	std::string className;
	TRACE_ENTER();

	for (i = 0; attrs[i] != NULL; i++)
	{
		if (strcmp(attrs[i]->attrName,
				   "SaImmAttrClassName") == 0)
		{
			if (attrs[i]->attrValueType == SA_IMM_ATTR_SANAMET)
			{
				className =
					std::string((char*)
								((SaNameT*)*attrs[i]->attrValues)->value,
								(size_t) ((SaNameT*)*attrs[i]->attrValues)->length);
				TRACE_LEAVE();
				return className;
			}
			else if (attrs[i]->attrValueType == SA_IMM_ATTR_SASTRINGT)
			{
				className =
					std::string(*((SaStringT*)*(attrs[i]->attrValues)));
				TRACE_LEAVE();
				return className;
			}
			else
			{
				std::cerr << "Invalid type for class name exiting"
					<< attrs[i]->attrValueType
					<< std::endl;
				exit(1);
			}
		}
	}
	std::cerr << "Could not find classname attribute -  exiting"
		<< std::endl;
	exit(1);
}

std::string valueToString(SaImmAttrValueT value, SaImmValueTypeT type)
{
	SaNameT* namep;
	char* str;
	SaAnyT* anyp;
	std::ostringstream ost;

	switch (type)
	{
		case SA_IMM_ATTR_SAINT32T:
			ost << *((int *) value);
			break;
		case SA_IMM_ATTR_SAUINT32T:
			ost << *((unsigned int *) value);
			break;
		case SA_IMM_ATTR_SAINT64T:
			ost << *((long long *) value);
			break;
		case SA_IMM_ATTR_SAUINT64T:
			ost << *((unsigned long long *) value);
			break;
		case SA_IMM_ATTR_SATIMET:
			ost << *((unsigned long long *) value);
			break;
		case SA_IMM_ATTR_SAFLOATT:
			ost << *((float *) value);
			break;
		case SA_IMM_ATTR_SADOUBLET:
			ost << *((double *) value);
			break;
		case SA_IMM_ATTR_SANAMET:
			namep = (SaNameT *) value;

			if (namep->length > 0)
			{
				namep->value[namep->length] = 0;
				ost << (char*) namep->value;
			}
			break;
		case SA_IMM_ATTR_SASTRINGT:
			str = *((SaStringT *) value);
			ost << str;
			break;
		case SA_IMM_ATTR_SAANYT:
			anyp = (SaAnyT *) value;

			for (unsigned int i = 0; i < anyp->bufferSize; i++)
			{
				ost << std::hex
					<< (((int)anyp->bufferAddr[i] < 0x10)? "0":"")
				<< (int)anyp->bufferAddr[i];
			}

			break;
		default:
			std::cerr << "Unknown value type - exiting" << std::endl;
			exit(1);
	}

	return ost.str().c_str();
}

std::list<std::string> getClassNames(SaImmHandleT immHandle)
{
	SaImmAccessorHandleT accessorHandle;
	SaImmAttrValuesT_2** attributes;
	SaImmAttrValuesT_2** p;
	SaNameT opensafObjectName;
	SaAisErrorT errorCode;
	std::list<std::string> classNamesList;
	TRACE_ENTER();

	strcpy((char*)opensafObjectName.value, OPENSAF_IMM_OBJECT_DN);
	opensafObjectName.length = strlen(OPENSAF_IMM_OBJECT_DN);

	/* Initialize immOmSearch */
	errorCode = saImmOmAccessorInitialize(immHandle,
										  &accessorHandle);

	if (SA_AIS_OK != errorCode)
	{
		std::cerr << "Failed on saImmOmAccessorInitialize - exiting "
			<< errorCode
			<< std::endl;
		exit(1);
	}

	/* Get the first match */

	errorCode = saImmOmAccessorGet_2(accessorHandle,
									 &opensafObjectName,
									 NULL,
									 &attributes);

	if (SA_AIS_OK != errorCode)
	{
		std::cerr << "Failed in saImmOmAccessorGet - exiting "
			<< errorCode
			<< std::endl;
		exit(1);
	}

	/* Find the classes attribute */
	for (p = attributes; (*p) != NULL; p++)
	{
		if (strcmp((*p)->attrName, OPENSAF_IMM_ATTR_CLASSES) == 0)
		{
			attributes = p;
			break;
		}
	}

	if (NULL == (*p))
	{
		std::cerr << "Failed to get the classes attribute" << std::endl;
		exit(1);
	}

	/* Iterate through the class names */
	for (SaUint32T i = 0; i < (*attributes)->attrValuesNumber; i++)
	{
		if ((*attributes)->attrValueType == SA_IMM_ATTR_SASTRINGT)
		{
			std::string classNameString =
				std::string(*(SaStringT*)(*attributes)->attrValues[i]);

			classNamesList.push_front(classNameString);
		}
		else if ((*attributes)->attrValueType == SA_IMM_ATTR_SANAMET)
		{
		//std::cout << "SANAMET" << std::endl;
			std::string classNameString =
				std::string((char*)((SaNameT*)(*attributes)->attrValues + i)->value,
							((SaNameT*)(*attributes)->attrValues + i)->length);

			classNamesList.push_front(classNameString);
		}
		else
		{
			std::cerr << "Invalid class name value type for "
				<< (*attributes)->attrName
				<< std::endl;
			exit(1);
		}
	}

	errorCode = saImmOmAccessorFinalize(accessorHandle);
	if (SA_AIS_OK != errorCode)
	{
		std::cerr << "Failed to finalize the accessor handle "
			<< errorCode
			<< std::endl;
		exit(1);
	}

	TRACE_LEAVE();
	return classNamesList;
}


