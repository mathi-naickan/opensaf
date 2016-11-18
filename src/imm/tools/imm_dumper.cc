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


#include "imm/tools/imm_dumper.h"
#include "base/osaf_extended_name.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <limits.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>
#include <getopt.h>
#include <assert.h>
#include <libgen.h>
#include <unistd.h>
#include <stdlib.h>

#define XML_VERSION "1.0"

/* Prototypes */
static std::map<std::string, std::string> cacheRDNs(SaImmHandleT, std::list<std::string>& selectedClassList);
static int checkClassNames(SaImmHandleT immHandle, std::list<std::string>& inputList);

static void usage(const char *progname)
{
    printf("\nNAME\n");
    printf("\t%s - dump IMM model to file\n", progname);

    printf("\nSYNOPSIS\n");
    printf("\t%s [ <file name> ]\n", progname);

    printf("\nDESCRIPTION\n");
    printf("\t%s is an IMM OM client used to dump, write the IMM model to file\n", progname);

    printf("\nOPTIONS\n");
    printf("\t-h, --help\n");
    printf("\t\tthis help\n\n");

    printf("\t-x, --xmlwriter   {<file name>}\n");
    printf("\t\tOption kept only for backward compatibility\n\n");

    printf("\t-p, --pbe   {<file name>}\n");
    printf("\t\tCreate an IMM database file from the current IMM state\n\n");

    printf("\t-c, --class   {<class name>}\n");
    printf("\t\tOnly dump objects of this class\n\n");

    printf("\t-a, --audit   {<pbe file name>}\n");
    printf("\t\tAudit PBE database\n\n");

    printf("\nEXAMPLE\n");
    printf("\t%s /tmp/imm.xml\n", progname);
    printf("\t%s /tmp/imm.xml -c ClassA -c ClassB\n", progname);
}


static void saImmOmAdminOperationInvokeCallback(SaInvocationT invocation,
    SaAisErrorT operationReturnValue,
    SaAisErrorT)
{
    LOG_ER("Unexpected async admin-op callback invocation:%llx", invocation);
}

static const SaImmCallbacksT callbacks = {
    saImmOmAdminOperationInvokeCallback
};


/* Functions */
int main(int argc, char* argv[])
{
    int c;
    struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"pbe", required_argument, 0, 'p'},
        {"xmlwriter", required_argument, 0, 'x'},
        {"class", required_argument, 0, 'c'},
        {"audit", required_argument, 0, 'a'},
        {0, 0, 0, 0}
    };
    SaImmHandleT           immHandle;
    SaAisErrorT            errorCode;
    SaVersionT             version;

    /*
    SaNameT immApp;
    immApp.length = strlen(OPENSAF_IMM_OBJECT_PARENT);
    strncpy((char *) immApp.value, OPENSAF_IMM_OBJECT_PARENT, 
        immApp.length + 1);

    immApp.length = strlen(OPENSAF_IMM_OBJECT_DN);
    strncpy((char *) immApp.value, OPENSAF_IMM_OBJECT_DN, 
        immApp.length + 1);
    const SaNameT* objectNames[] = {&immApp, NULL};
    */

    std::map<std::string, std::string> classRDNMap;
    std::string filename;
    std::string localTmpFilename;
    const char* defaultLog = "immdump_trace";
    const char* logPath;
    unsigned int category_mask = 0;
    bool pbeDumpCase = false;
    bool auditPbe = false;
    void* dbHandle=NULL;
    const char* dump_trace_label = "immdump";
    const char* trace_label = dump_trace_label;
    ClassMap classIdMap;
    int objCount=0;
    std::list<std::string> selectedClassList;

	/* Support for long DN */
	setenv("SA_ENABLE_EXTENDED_NAMES", "1", 1);
	/* osaf_extended_name_init() is added to prevent future safe use of
	 * osaf_extended_name_* before saImmOmInitialize and saImmOiInitialize */
	osaf_extended_name_init();

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

    while (1) {
            if ((c = getopt_long(argc, argv, "hp:x:c:a:", long_options, NULL)) == -1)
                    break;

            switch (c) {
                case 'h':
                    usage(basename(argv[0]));
                    exit(EXIT_SUCCESS);
                    break;

                case 'p':
                    pbeDumpCase = true;
                    filename.append(optarg);
                    break;

                case 'x':
                    filename.append(optarg);
                    break;

                case 'c':
                    selectedClassList.push_back(std::string(optarg));
                    break;

                case 'a':
                    auditPbe = true;
                    filename.append(optarg);
                    break;

                default:
                    fprintf(stderr, "Try '%s --help' for more information\n", 
                        argv[0]);
                    exit(EXIT_FAILURE);
                    break;
        }
    }

    if(pbeDumpCase && auditPbe) {
    	usage(basename(argv[0]));
    	exit(EXIT_FAILURE);
    }

    if(auditPbe) {
        int rc;

        rc = pbeAuditFile(filename.c_str());
        if(!rc) {
            std::cout << "Audit successful" << std::endl;
        } else if(rc == 2) {
            std::cerr << "Option --enable-imm-pbe must be enabled in the build to be able to audit PBE file"
                    << std::endl;
        } else {
            std::cerr << "Audit failed. Check syslog for more details."
                    << std::endl;
        }
        exit(rc);
    }

    version.releaseCode = RELEASE_CODE;
    version.majorVersion = MAJOR_VERSION;
    version.minorVersion = MINOR_VERSION;

    /* Initialize immOm */
    errorCode = saImmOmInitialize(&immHandle, &callbacks, &version);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed to initialize the imm om interface - exiting"
            << errorCode 
            <<  std::endl;
        exit(1);
    }

    if (!selectedClassList.empty() && !checkClassNames(immHandle, selectedClassList)) {
        /* selectedClassList has no valid class */
        std::cerr << "No valid class - exiting" << std::endl;
        exit(1);
    }

    if(pbeDumpCase) {
    	/* Generate PBE database file from current IMM state */

    	std::cout <<
            "Generating DB file from current IMM state. File: " << filename <<
             std::endl;

        /* Initialize access to PBE database. */
        dbHandle = pbeRepositoryInit(filename.c_str(), true, localTmpFilename);


        if(dbHandle) {
            TRACE_1("Opened persistent repository %s", filename.c_str());
        } else {
            std::cerr << "immdump: intialize failed - exiting, check syslog for details"
                << std::endl;
            exit(1);
        }

        if(dumpClassesToPbe(immHandle, &classIdMap, dbHandle, selectedClassList)) {
            TRACE("Dump classes OK");
	} else {
            std::cerr << "immdump: dumpClassesToPbe failed - exiting, check syslog for details"
                << std::endl;
            if(!localTmpFilename.empty()) {
                pbeCleanTmpFiles(localTmpFilename);
            }
            exit(1);
	}

        if (selectedClassList.empty()) {
            objCount = dumpObjectsToPbe(immHandle, &classIdMap, dbHandle);
        } else {
            objCount = dumpObjectsToPbe(immHandle, &classIdMap, dbHandle, selectedClassList);
        }
	if(objCount > 0) {
            TRACE("Dump %u objects OK", objCount);
	} else if (selectedClassList.empty()) {
            std::cerr << "immdump: dumpObjectsToPbe failed - exiting, check syslog for details"
                << std::endl;
            if(!localTmpFilename.empty()) {
                pbeCleanTmpFiles(localTmpFilename);
            }
            exit(1);
	}

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
	filename.append(".tmp-journal");
	unlink(filename.c_str()); /* Dont bother checking if it succeeded. */
    } else {
        /* Generate IMM XML file from current IMM state */
        /* xmlWriter dump case */
    	if(filename.empty() && argc == optind) {
    		filename.append("/proc/self/fd/1");
    	} else {
    		if(filename.empty())
                /* getopt rearranges the position of arguments
                 * argv[1] before getopting is now argv[optind] */
    			filename.append(argv[optind]);
    		std::cout << "Dumping current IMM state to XML file " << filename <<
    				" using XMLWriter" << std::endl;
    	}

        xmlTextWriterPtr writer=xmlNewTextWriterFilename(filename.c_str(),0);

        if(writer == NULL)  {
        	std::cout << "Error at xmlNewTextWriterFilename" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterSetIndent(writer,1) < 0)  {
        	std::cout << "Error at xmlTextWriterSetIndent" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterSetIndentString(writer,(xmlChar *)"  ") < 0)  {
        	std::cout << "Error at xmlTextWriterSetIndentString" << std::endl;
        	exit(1);
        }

        /* Create a new xml document */
        if(xmlTextWriterStartDocument(writer,NULL,NULL,NULL) < 0) {
        	std::cout << "Error at xmlTextWriterStartDocument" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterStartElementNS(writer, (xmlChar*) "imm",(xmlChar *)"IMM-contents"
                ,(xmlChar *)"http://www.saforum.org/IMMSchema") < 0) {
        	std::cout <<"Error at xmlTextWriterStartElementNS (IMM-contents)" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterWriteAttributeNS(writer, (xmlChar*) "xmlns",(xmlChar *)"xsi", NULL,
                (xmlChar *)"http://www.w3.org/2001/XMLSchema-instance") < 0) {
        	std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 1)" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterWriteAttributeNS(writer,
                (xmlChar*)"xsi",(xmlChar *)"noNamespaceSchemaLocation", NULL,
                (xmlChar*)"SAI-AIS-IMM-XSD-A.02.13.xsd") < 0) {
        	std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 2)" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterWriteAttributeNS(writer, (xmlChar*) "xmlns",(xmlChar *)"xs", NULL,
                (xmlChar *)"http://www.w3.org/2001/XMLSchema") < 0) {
        	std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 3)" << std::endl;
        	exit(1);
        }

        classRDNMap = cacheRDNs(immHandle, selectedClassList);

        dumpClassesXMLw(immHandle, writer, selectedClassList);

        if (selectedClassList.empty()) {
            dumpObjectsXMLw(immHandle, classRDNMap, writer);
        } else {
            dumpObjectsXMLw(immHandle, classRDNMap, writer, selectedClassList);
        }

        /* Close element named imm:IMM-contents */
        if( xmlTextWriterEndElement(writer) < 0) {
            std::cout << "Error at xmlTextWriterEndElement" << std::endl;
            exit(1);
        }

        xmlFreeTextWriter(writer);
        xmlCleanupParser();
        xmlMemoryDump();
	}

    /* Finalize immOm */
    errorCode = saImmOmFinalize(immHandle);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed to finalize the imm om interface"
            << errorCode
            <<  std::endl;
    }

    return 0;
}

static std::map<std::string, std::string>
    cacheRDNs(SaImmHandleT immHandle, std::list<std::string>& selectedClassList)
{
    std::list<std::string>             classNamesList;
    SaImmClassCategoryT                classCategory;
    SaImmAttrDefinitionT_2**           attrs;
    std::map<std::string, std::string> classRDNMap;

    if (selectedClassList.empty()) {
        classNamesList = getClassNames(immHandle);
    } else {
        classNamesList = selectedClassList;
    }

    std::list<std::string>::iterator it = classNamesList.begin();

    while (it != classNamesList.end())
    {

        saImmOmClassDescriptionGet_2(immHandle,
                                     (char*)it->c_str(),
                                     &classCategory,
                                     &attrs);

        for (SaImmAttrDefinitionT_2** p = attrs; *p != NULL; p++)
        {
            if ((*p)->attrFlags & SA_IMM_ATTR_RDN)
            {
                classRDNMap[*it] = std::string((*p)->attrName);
                break;
            }
        }

        /* Avoid memory leaking */
        saImmOmClassDescriptionMemoryFree_2(immHandle, attrs);

        ++it;
    }

    return classRDNMap;
}

int checkClassNames(SaImmHandleT immHandle, std::list<std::string>& inputList)
{
    std::list<std::string> classNameList = getClassNames(immHandle);
    std::list<std::string>::iterator it = inputList.begin();
    while (it != inputList.end()) {
        std::list<std::string>::iterator sub_it = classNameList.begin();
        bool found = false;
        while (sub_it != classNameList.end()) {
            if (*it == *sub_it) {
                found = true;
                break;
            }
            ++sub_it;
        }
        if (found) {
            ++it;
        } else {
            printf("Warning: Class '%s' doesn't exist\n", (*it).c_str());
            /* Remove invalid class from list */
            it = inputList.erase(it);
        }
    }

    return inputList.size();
}
