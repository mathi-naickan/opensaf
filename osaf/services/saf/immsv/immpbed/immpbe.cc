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


#include "immpbe.hh"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <libxml/parser.h>
#include <getopt.h>
#include <assert.h>
#include <libgen.h>
#include <unistd.h>

#define XML_VERSION "1.0"


static void saImmOmAdminOperationInvokeCallback(SaInvocationT invocation,
	SaAisErrorT operationReturnValue,
	SaAisErrorT)
{
	LOG_ER("Unexpected async admin-op callback invocation:%llx", invocation);
}

static const SaImmCallbacksT callbacks = {
	saImmOmAdminOperationInvokeCallback
};


/**
 * Check if osafimmpbe is executed by osafimmnd
 */
static void checkParentProcess() {
	char cmd[1024];
	char *base;
	pid_t ppid = getppid();

	sprintf(cmd, "cat /proc/%u/cmdline", ppid);
	FILE *f = popen(cmd, "r");
	if(!f)
		goto fail;

	if(!fgets(cmd, 1024, f)) {
		pclose(f);
		goto fail;
	}

	pclose(f);

	base = basename(cmd);

	if(strncmp(base, "osafimmnd", strlen("osafimmnd")))
		goto fail;

	return;

fail:
	fprintf(stderr, "osafimmpbe is not started by osafimmnd\n");
	LOG_ER("osafimmpbe is not started by osafimmnd");
	exit(1);
}

static void usage(const char *progname)
{
	printf("\t%s - Persistent Back End process was not exec'ed correctly\n", progname);
}

/* Functions */
int main(int argc, char* argv[])
{
	int c;
	struct option long_options[] = {
		{"recover", no_argument, 0, 'r'},
		{"pbe", required_argument, 0, 'p'},
		{"pbe2A", required_argument, 0, 'A'},
		{"pbe2B", required_argument, 0, 'B'},
		{0, 0, 0, 0}
	};
	SaImmHandleT		immHandle;
	SaAisErrorT			errorCode;
	SaVersionT			version;
	SaImmAdminOwnerHandleT ownerHandle;


	std::map<std::string, std::string> classRDNMap;
	std::string filename;
	std::string localTmpFilename;
	const char* defaultLog = "osafimmpbed_trace";
	const char* logPath;
	unsigned int category_mask = 0;
	bool pbeDumpCase = true;
	bool pbeRecoverFile = false;
	bool pbe2 = false;
	bool pbe2BCase = false;
	void* dbHandle=NULL;
	const char* dump_trace_label = "osafimmpbed";
	const char* trace_label = dump_trace_label;
	ClassMap classIdMap;
	int objCount=0;
	bool fileReOpened=false;

	unsigned int		retryInterval = 1000000;	/* 1 sec */
	unsigned int		maxTries = 70;				/* 70 times == max 70 secs */
	unsigned int		tryCount=0;

	if ((logPath = getenv("IMMSV_TRACE_PATHNAME")))
	{
		category_mask = 0xffffffff; /* TODO: set using -t flag ? */
	} else {
		logPath = defaultLog;
	}

	if (logtrace_init(trace_label, logPath, category_mask) == -1)
	{
		printf("logtrace_init FAILED\n");
		syslog(LOG_ERR, "logtrace_init FAILED");
		/* We allow the dump to execute anyway. */
	}

	if (argc < 2 || argc > 4)
	{
		usage(basename(argv[0]));
		exit(EXIT_FAILURE);
	}

	int i=0;
	while(i < argc) {
		LOG_IN("arg[%u] == '%s'",i, argv[i]);
		++i;
	}

	while (1) {
		if ((c = getopt_long(argc, argv, "rp:", long_options, NULL)) == -1)
			break;

		switch (c) {
			case 'r':
				pbeDumpCase = false;
				pbeRecoverFile = true;
				break;

			case 'B':
				pbe2 = true;
				pbe2BCase = true;
				// Intentional fall through. 

			case 'A':
				pbe2 = true;
				// Intentional fall through. 

			case 'p':
				/* ABT This if statement is redundant. */
				if(!pbeRecoverFile) {
					pbeDumpCase = true;
				}
				break;


			default:
				LOG_ER("Invalid argument to %s.", argv[0]);
				exit(EXIT_FAILURE);
				break;
		}
	}

	checkParentProcess();

	if(pbeRecoverFile && argc == 4)
		filename.append(argv[3]);
	else if(!pbeRecoverFile && argc == 3)
		filename.append(argv[2]);
	else {
		LOG_WA("File name is empty argc:%u recover?:%u", argc, pbeRecoverFile);
		exit(1);
	}

	version.releaseCode = RELEASE_CODE;
	version.majorVersion = MAJOR_VERSION;
	version.minorVersion = MINOR_VERSION;

	/* Initialize immOm */
	errorCode = saImmOmInitialize(&immHandle, &callbacks, &version);
	if (SA_AIS_OK != errorCode)
	{
		LOG_WA("Failed to initialize imm om handle - exiting");
		exit(1);
	}

 re_open:
	if(!pbeDumpCase && pbeRecoverFile) {
		/* This is the re-attachement case.
		   The PBE has crashed,
		   or the IMMND-coord has crashed which will force the PBE to terminate,
		   or the entire node on which immnd-coord and pbe was running has crashed.
		   Try to re-attach to the db file and avoid regenerating it.
		*/
		dbHandle = pbeRepositoryInit(filename.c_str(), false, localTmpFilename);
		/* getClassIdMap */
		if(dbHandle) {
			objCount = verifyPbeState(immHandle, &classIdMap, dbHandle);
			TRACE("Classes Verified?:%d", objCount);
			if(objCount <= 0) {dbHandle = NULL;} 
		}

		if(!dbHandle) {
			LOG_WA("Pbe: Failed to re-attach to db file %s - regenerating db file",
					filename.c_str());
			pbeDumpCase = true;
		}
	}

	if(pbeDumpCase) {
		LOG_IN("Generating DB file from current IMM state. DB file: %s", filename.c_str());

		/* Initialize access to PBE database. */
		dbHandle = pbeRepositoryInit(filename.c_str(), true, localTmpFilename);

		if(dbHandle) {
			TRACE_1("Opened persistent repository %s", filename.c_str());
		} else {
			/* Any localTmpFile was removed in pbeRepositoryInit */
			LOG_ER("immpbe.cc: pbe intialize failed - exiting");
			exit(1);
		}

		if(!dumpClassesToPbe(immHandle, &classIdMap, dbHandle)) {
			if(!localTmpFilename.empty()) {
				std::string localTmpJournalFileName(localTmpFilename);
				localTmpJournalFileName.append("-journal");
				unlink(localTmpJournalFileName.c_str());
				unlink(localTmpFilename.c_str());
				localTmpFilename.clear();
			}
			LOG_ER("immpbe.cc: dumpClassesToPbe failed - exiting (line:%u)", __LINE__);
			exit(1);
		}
		TRACE("Dump classes OK");

		objCount = dumpObjectsToPbe(immHandle, &classIdMap, dbHandle);
		if(objCount <= 0) {
			if(!localTmpFilename.empty()) {
				std::string localTmpJournalFileName(localTmpFilename);
				localTmpJournalFileName.append("-journal");
				unlink(localTmpJournalFileName.c_str());
				unlink(localTmpFilename.c_str());
				localTmpFilename.clear();
			}
			LOG_ER("immpbe.cc dumpObjectsToPbe failed - exiting (line:%u)", __LINE__);
			exit(1);
		}
		TRACE("Dump objects OK");

		/* Discard the old classIdMap, will otherwise contain invalid
		   pointer/member 'sqlStmt' after handle close below. */
		ClassMap::iterator itr;
		for(itr = classIdMap.begin(); itr != classIdMap.end(); ++itr) {
			ClassInfo* ci = itr->second;
			delete(ci);
		}
		classIdMap.clear();

		pbeRepositoryClose(dbHandle);
		dbHandle = NULL;
		LOG_NO("Successfully dumped to file %s", localTmpFilename.c_str());

		pbeAtomicSwitchFile(filename.c_str(), localTmpFilename);

		/* Else the pbe dump was needed to get the initial pbe-file
		   to be used by the pbeDaemon.
		 */
		if(fileReOpened) {
			LOG_ER("immpbe.cc: will not re-open twice. immpbe.cc exiting (line:%u)", __LINE__);
			exit(1);
		}

		fileReOpened=true;
		pbeDumpCase=false;
		pbeRecoverFile=true;
		LOG_IN("Re-attaching to the new version of %s", filename.c_str());
		goto re_open;
	}


	if(!dbHandle) {
		dbHandle = pbeRepositoryInit(filename.c_str(), false, localTmpFilename);
		if(!dbHandle) {
			LOG_WA("immpbe.cc: pbe intialize failed - exiting");
			exit(1);
		}
	}

	/* Creating an admin owner with release on finalize set.
	   This makes the handle non resurrectable, allowing us
	   to detect loss of immnd. If immnd goes down we also exit.
	   A new immnd coord will be elected which should start a new pbe process.
	 */
	do {
		if(tryCount) {
			usleep(retryInterval);
		}
		++tryCount;
		errorCode = saImmOmAdminOwnerInitialize(immHandle,
			(char *) OPENSAF_IMM_SERVICE_NAME, SA_TRUE, &ownerHandle);
	} while ((errorCode == SA_AIS_ERR_TRY_AGAIN) &&
			 (tryCount < maxTries));

	if(SA_AIS_OK != errorCode)
	{
		LOG_WA("Failed to initialize admin owner handle: err:%u - exiting",
			errorCode);
		pbeRepositoryClose(dbHandle);
		exit(1);
	}

	/*
	errorCode = saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE);
	*/

	/* If we allow pbe without prior dump we need to fix classIdMap. */
	assert(classIdMap.size());
	pbeDaemon(immHandle, dbHandle, ownerHandle, &classIdMap, objCount, pbe2, pbe2BCase);
	TRACE("Exit from pbeDaemon");

	return 0;
}


