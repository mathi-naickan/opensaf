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

#define XML_VERSION "1.0"

/* Prototypes */
static std::map<std::string, std::string> cacheRDNs(SaImmHandleT);

static void usage(const char *progname)
{
    printf("\nNAME\n");
    printf("\t%s - dump IMM model to file\n", progname);

    printf("\nSYNOPSIS\n");
    printf("\t%s <file name>\n", progname);

    printf("\nDESCRIPTION\n");
    printf("\t%s is an IMM OM client used to dump, write the IMM model to file\n", progname);

    printf("\nOPTIONS\n");
    printf("\t-h, --help\n");
    printf("\t\tthis help\n\n");

    printf("\t-x, --xmlwriter   {<file name>}\n");
    printf("\t\tOption kept only for backward compatibility\n\n");

    printf("\t-p, --pbe   {<file name>}\n");
    printf("\t\tCreate an IMM database file from the current IMM state\n\n");

    printf("\nEXAMPLE\n");
    printf("\t%s /tmp/imm.xml\n", progname);
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
        {"xmlwriter", no_argument, 0, 'x'},
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
    void* dbHandle=NULL;
    const char* dump_trace_label = "immdump";
    const char* trace_label = dump_trace_label;
    ClassMap classIdMap;
    int objCount=0;

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

    if ((argc < 2) || (argc > 5))
    {
        printf("Usage: %s <xmldumpfile>\n", basename(argv[0]));
	usage(basename(argv[0]));
        exit(1);
    }

    while (1) {
    if ((c = getopt_long(argc, argv, "hp:x:", long_options, NULL)) == -1)
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

                default:
                    fprintf(stderr, "Try '%s --help' for more information\n", 
                        argv[0]);
                    exit(EXIT_FAILURE);
                    break;
        }
    }

    if(filename.empty()) {
	    filename.append(argv[1]);
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

        if(dumpClassesToPbe(immHandle, &classIdMap, dbHandle)) {
            TRACE("Dump classes OK");
	} else {
            std::cerr << "immdump: dumpClassesToPbe failed - exiting, check syslog for details"
                << std::endl;
            exit(1);
	}

        objCount = dumpObjectsToPbe(immHandle, &classIdMap, dbHandle);
	if(objCount > 0) {
            TRACE("Dump %u objects OK", objCount);
	} else {
            std::cerr << "immdump: dumpObjectsToPbe failed - exiting, check syslog for details"
                << std::endl;
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
        std::cout << "Dumping current IMM state to XML file " << filename <<
	        " using XMLWriter" << std::endl;

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
                (xmlChar*)"SAI-AIS-IMM-XSD-A.02.12.xsd") < 0) {
        	std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 2)" << std::endl;
        	exit(1);
        }

        if(xmlTextWriterWriteAttributeNS(writer, (xmlChar*) "xmlns",(xmlChar *)"xs", NULL,
                (xmlChar *)"http://www.w3.org/2001/XMLSchema") < 0) {
        	std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 3)" << std::endl;
        	exit(1);
        }

        classRDNMap = cacheRDNs(immHandle);

        dumpClassesXMLw(immHandle, writer);

        dumpObjectsXMLw(immHandle, classRDNMap, writer);

        /* Close element named imm:IMM-contents */
        if( xmlTextWriterEndElement(writer) < 0) {
            std::cout << "Error at xmlTextWriterEndElement" << std::endl;
            exit(1);
        }

        xmlFreeTextWriter(writer);
        xmlCleanupParser();
        xmlMemoryDump();
	}

    return 0;
}

static std::map<std::string, std::string>
    cacheRDNs(SaImmHandleT immHandle)
{
    std::list<std::string>             classNamesList;
    SaImmClassCategoryT                classCategory;
    SaImmAttrDefinitionT_2**           attrs;
    std::map<std::string, std::string> classRDNMap;

    classNamesList = getClassNames(immHandle);

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

        it++;
    }

    return classRDNMap;
}

