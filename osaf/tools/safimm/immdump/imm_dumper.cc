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
#include <getopt.h>
#include <assert.h>
#include <libgen.h>
#include <unistd.h>

#define XML_VERSION "1.0"

/* Prototypes */
static void dumpClasses(SaImmHandleT, xmlNodePtr);
static void classToXML(std::string, SaImmHandleT, xmlNodePtr);
static void objectToXML(std::string, 
                        SaImmAttrValuesT_2**, 
                        SaImmHandleT, 
                        std::map<std::string, std::string>,
                     	xmlNodePtr);
static void valuesToXML(SaImmAttrValuesT_2* p, xmlNodePtr parent);
static void dumpObjects(SaImmHandleT, 
                        std::map<std::string, std::string>,
                        xmlNodePtr);
static void flagsToXML(SaImmAttrDefinitionT_2*, xmlNodePtr);
static void typeToXML(SaImmAttrDefinitionT_2*, xmlNodePtr);
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
    printf("\t\tGenerate xml file using xmlWriter instead of xmlDoc (saves memory)\n\n");
    printf("\t-y, --yyy   {<file name>}\n");
    printf("\t\tGenerate xml file using the old xmlDoc instead of xmlWriter\n\n");

    printf("\t-p, --pbe   {<file name>}\n");
    printf("\t\tInstead of xml file, generate/populate persistent back-end database/file\n");

    printf("\nEXAMPLE\n");
    printf("\timmdump /tmp/imm.xml\n");
}
/*
  Hidden arguments, only intended to be used when immdump is forked by immnd.
   --daemon
   Tells immdump that it will be running as a slave daemon to the coord immnd.
   The immdump process will try to connect back to the coord immnd.

   --recover
   If immdump is running as daemon then the recover argument tells immdump
   NOT to initialize the db file from scratch. Instead it will try to
   recover based on an existing db file. 
   The --recover argument overrides the --pbe argument as far as generating
   an initial dump, but the --pbe <file> argument still needs to be supplied
   by immnd because the name for the db file is obtained this way. 
*/


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
        {"daemon", no_argument, 0, 'd'},
        {"pbe", required_argument, 0, 'p'},
        {"recover", no_argument, 0, 'r'},
        {"xmlwriter", no_argument, 0, 'x'},
        {"yyy", no_argument, 0, 'y'},
        {0, 0, 0, 0}
    };
    SaImmHandleT           immHandle;
    SaAisErrorT            errorCode;
    SaVersionT             version;

    SaImmAdminOwnerHandleT ownerHandle;
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
    bool pbeDaemonCase = false;
    bool pbeRecoverFile = false;
    bool xmlwDumpCase = true;
    void* dbHandle=NULL;
    const char* dump_trace_label = "immdump";
    const char* pbe_daemon_trace_label = "immpbe";
    const char* trace_label = dump_trace_label;
    ClassMap classIdMap;
    unsigned int objCount=0;

    unsigned int           retryInterval = 1000000; /* 1 sec */
    unsigned int           maxTries = 70;          /* 70 times == max 70 secs */
    unsigned int           tryCount=0;

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
        printf("Usage: %s <xmldumpfile>\n", argv[0]);
	usage(argv[0]);
        exit(1);
    }

    while (1) {
    if ((c = getopt_long(argc, argv, "hdrp:x:y:", long_options, NULL)) == -1)
            break;

            switch (c) {
                case 'h':
                    usage(basename(argv[0]));
                    exit(EXIT_SUCCESS);
                    break;

                case 'd':
                    pbeDaemonCase = true;
                    trace_label = pbe_daemon_trace_label;
                    break;

                case 'p':
		    if(!pbeRecoverFile) {
			    pbeDumpCase = true;
		    }

		    filename.append(optarg);
                    break;

                case 'r':
		    pbeDumpCase = false;
		    pbeRecoverFile = true;
                    break;

                case 'x':
                    xmlwDumpCase = true;
                    filename.append(optarg);
                    break;

                case 'y':
                    xmlwDumpCase = false;
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
        if(pbeDaemonCase) {
            LOG_WA("Failed to initialize imm om handle - exiting");
        } else {
            std::cerr << "Failed to initialize the imm om interface - exiting" 
            << errorCode 
            <<  std::endl;
        }
        exit(1);
    }

    if(pbeDaemonCase && !pbeDumpCase && pbeRecoverFile) {
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
		TRACE("Classes Verified");
		if(!objCount) {dbHandle = NULL;}
        } 

	if(!dbHandle) {
		LOG_WA("Pbe: Failed to re-attach to db file %s - regenerating db file",
			filename.c_str());
		pbeDumpCase = true;
	}
    }    

    if(pbeDumpCase) {
        if(pbeDaemonCase) {
            LOG_IN("Generating DB file from current IMM state. DB file: %s", filename.c_str());
        } else {
            std::cout << 
                "Generating DB file from current IMM state. File: " << filename << 
                 std::endl;
        }

	/* Initialize access to PBE database. */
	dbHandle = pbeRepositoryInit(filename.c_str(), true, localTmpFilename);


        if(dbHandle) {
            TRACE_1("Opened persistent repository %s", filename.c_str());
        } else {
            if(pbeDaemonCase) {
                LOG_WA("immdump: pbe intialize failed - exiting");
            } else {
                std::cerr << "immdump: intialize failed - exiting, check syslog for details"
                    << std::endl;
            }
            exit(1);
        }

        dumpClassesToPbe(immHandle, &classIdMap, dbHandle);
        TRACE("Dump classes OK");

        objCount = dumpObjectsToPbe(immHandle, &classIdMap, dbHandle);
        TRACE("Dump objects OK");

	pbeRepositoryClose(dbHandle);
	dbHandle = NULL;
	LOG_NO("Successfully dumped to file %s", localTmpFilename.c_str());

	pbeAtomicSwitchFile(filename.c_str(), localTmpFilename);
        if(!pbeDaemonCase) {
            exit(0);
        }
        /* Else the pbe dump was needed to get the initial pbe-file
           to be used by the pbeDaemon.
         */
    }


    if(pbeDaemonCase) {

        if(!dbHandle) {
            dbHandle = pbeRepositoryInit(filename.c_str(), false, localTmpFilename);
            if(!dbHandle) {
                LOG_WA("immdump: pbe intialize failed - exiting");
                exit(1);
                /* TODO SYNC with pbe-file AND with IMMSv */
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
            if(pbeDaemonCase) {
                LOG_WA("Failed to initialize admin owner handle: err:%u - exiting",
                    errorCode);
            } else {
                std::cerr << "Failed to initialize admin owner handle; err:" 
                    << errorCode << std::endl;
            }
            exit(1);
        }

        /*
        errorCode = saImmOmAdminOwnerSet(ownerHandle, objectNames, SA_IMM_ONE);
        */

	
	/* If we allow pbe without prior dump we need to fix classIdMap. */
	assert(classIdMap.size());
        pbeDaemon(immHandle, dbHandle, &classIdMap, objCount);
        TRACE("Exit from pbeDaemon");
        exit(0);
    }


    /* NOT pbeDaemonCase, i.e. plain imm.xml dump */

    assert(!pbeDumpCase && !pbeDaemonCase);



    if(xmlwDumpCase) {
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
           (xmlChar*)"SAI-AIS-IMM-XSD-A.01.01.xsd") < 0) {
            std::cout <<"Error at xmlTextWriterWriteAttribute (attribute 2)" << std::endl;
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

        exit(0);
    }

    /* Normal dump/export case to XML file. */
    xmlDocPtr xmlDoc;
    xmlNodePtr xmlImmRoot;

    filename.append(argv[1]);
    std::cout << "Dumping the current IMM state to XML" << std::endl;

    /* Create a new xml document */
    xmlDoc = xmlNewDoc((xmlChar*)XML_VERSION);
    xmlImmRoot = xmlNewNode(NULL, (const xmlChar*)"imm:IMM-contents");

    xmlDocSetRootElement(xmlDoc, xmlImmRoot);

    classRDNMap = cacheRDNs(immHandle);

    dumpClasses(immHandle, xmlImmRoot);

    dumpObjects(immHandle, classRDNMap, xmlImmRoot);

    std::cout << "Wrote " 
        << xmlSaveFormatFile (filename.c_str(), xmlDoc, 1) 
            << std::endl;

    return 0;
}

static void dumpObjects(SaImmHandleT immHandle, 
                        std::map<std::string, std::string> classRDNMap,
                        xmlNodePtr parent)
{
    SaNameT                root;
    SaImmSearchHandleT     searchHandle;
    SaAisErrorT            errorCode;
    SaNameT                objectName;
    SaImmAttrValuesT_2**   attrs;
    unsigned int           retryInterval = 1000000; /* 1 sec */
    unsigned int           maxTries = 70;          /* 70 times == max 70 secs */
    unsigned int           tryCount=0;
    TRACE_ENTER();

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
                SA_IMM_SEARCH_PERSISTENT_ATTRS),/*only persistent rtattrs*/
            NULL/*&params*/, 
            NULL, 
            &searchHandle);

    } while ((errorCode == SA_AIS_ERR_TRY_AGAIN) &&
              (tryCount < maxTries)); /* Can happen if imm is syncing. */

    TRACE_1("After searchInitialize rc:%u", errorCode);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed on saImmOmSearchInitialize - exiting " 
            << errorCode 
            << std::endl;

        exit(1);
    }

    /* Iterate through the object space */
    do
    {
        errorCode = saImmOmSearchNext_2(searchHandle, 
                                        &objectName, 
                                        &attrs);

        if (SA_AIS_OK != errorCode)
        {
            break;
        }

        if (attrs[0] == NULL)
        {
            continue;
        }

        objectToXML(std::string((char*)objectName.value, objectName.length),
                    attrs,
                    immHandle,
                    classRDNMap,
                    parent);
    } while (SA_AIS_OK == errorCode);

    if (SA_AIS_ERR_NOT_EXIST != errorCode)
    {
        std::cerr << "Failed in saImmOmSearchNext_2 - exiting"
            << errorCode
            << std::endl;
        exit(1);
    }

    /* End the search */
    errorCode = saImmOmSearchFinalize(searchHandle);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed to finalize the search connection - exiting"
            << errorCode
            << std::endl;
        exit(1);
    }
    TRACE_LEAVE();
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

static void dumpClasses(SaImmHandleT immHandle, xmlNodePtr parent)
{
    std::list<std::string> classNameList;
    std::list<std::string>::iterator it;
    TRACE_ENTER();

    classNameList = getClassNames(immHandle);

    it = classNameList.begin();

    while (it != classNameList.end())
    {
        classToXML(*it, immHandle, parent);

        it++;
    }
    TRACE_LEAVE();
}

static void classToXML(std::string classNameString,
                       SaImmHandleT immHandle,
                       xmlNodePtr parent)
{
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2 **attrDefinitions;
    SaAisErrorT errorCode;
    xmlNodePtr  classNode;
    TRACE_ENTER();
    /* Get the class description */
    errorCode = saImmOmClassDescriptionGet_2(immHandle,
                                             (char*)classNameString.c_str(),
                                             &classCategory,
                                             &attrDefinitions);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed to get the description for the " 
            << classNameString
            << " class - exiting, "
            << errorCode
            << std::endl;
        exit(1);
    }

    classNode = xmlNewTextChild(parent, 
                                NULL,
                                (xmlChar*)"class",
                                NULL);

    xmlNewTextChild(classNode,
                    NULL,
                    (xmlChar*)"category",
                    (xmlChar*)((classCategory == SA_IMM_CLASS_CONFIG)?
                               "SA_CONFIG":"SA_RUNTIME"));

    xmlSetNsProp(classNode,
                 NULL,
                 (xmlChar*)"name",
                 (xmlChar*)classNameString.c_str());

    /* List the attributes*/
    for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
    {
        xmlNodePtr attrNode;
        if ((*p)->attrFlags & SA_IMM_ATTR_RDN)
        {
            attrNode = xmlNewTextChild(classNode,
                                       NULL,
                                       (xmlChar*)"rdn",
                                       NULL);
        }
        else
	    continue;

        xmlNewTextChild(attrNode,
                        NULL,
                        (xmlChar*)"name",
                        (xmlChar*)(*p)->attrName);

        if ((*p)->attrDefaultValue != NULL)
        {
            xmlNewTextChild(attrNode,
                            NULL,
                            (xmlChar*)"default-value",
                            (xmlChar*)valueToString((*p)->attrDefaultValue,
                                                    (*p)->attrValueType).c_str());
        }

        typeToXML(*p, attrNode);

        flagsToXML(*p, attrNode);
    }

    for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
    {
        xmlNodePtr attrNode;
        if ((*p)->attrFlags & SA_IMM_ATTR_RDN)
		continue;
        else
        {
            attrNode = xmlNewTextChild(classNode,
                                       NULL,
                                       (xmlChar*)"attr",
                                       NULL);
        }
    
        xmlNewTextChild(attrNode,
                        NULL,
                        (xmlChar*)"name", 
                        (xmlChar*)(*p)->attrName);
    
        if ((*p)->attrDefaultValue != NULL)
        {
            xmlNewTextChild(attrNode,
                            NULL,
                            (xmlChar*)"default-value",
                            (xmlChar*)valueToString((*p)->attrDefaultValue,
                                                    (*p)->attrValueType).c_str());
        }

        typeToXML(*p, attrNode);

        flagsToXML(*p, attrNode); 
    }


    errorCode = 
        saImmOmClassDescriptionMemoryFree_2(immHandle, attrDefinitions);
    if (SA_AIS_OK != errorCode)
    {
        std::cerr << "Failed to free the description of class "
            << classNameString
            << std::endl;
        exit(1);
    }
    TRACE_LEAVE();
}

static void objectToXML(std::string objectNameString,
                        SaImmAttrValuesT_2** attrs,
                        SaImmHandleT immHandle,
                        std::map<std::string, std::string> classRDNMap,
                       	xmlNodePtr parent)
{
    std::string valueString;
    std::string classNameString;
    xmlNodePtr objectNode;
    TRACE_ENTER();

    //std::cout << "Dumping object " << objectNameString << std::endl;
    classNameString = getClassName((const SaImmAttrValuesT_2**) attrs);
    /* Create the object tag */
    objectNode = xmlNewTextChild(parent, 
                                 NULL, 
                                 (xmlChar*)"object", 
                                 NULL);
    xmlSetNsProp(objectNode, 
                 NULL, 
                 (xmlChar*)"class", 
                 (xmlChar*)classNameString.c_str());
    xmlNewTextChild(objectNode,
                    NULL,
                    (xmlChar*)"dn",
                    (xmlChar*)objectNameString.c_str());
    for (SaImmAttrValuesT_2** p = attrs; *p != NULL; p++)
    {
        /* Skip attributes with attrValues = NULL */
        if ((*p)->attrValues == NULL)
        {
            continue;
        }

        xmlNodePtr attrNode;
        if (classRDNMap.find(classNameString) != classRDNMap.end() &&
            classRDNMap[classNameString] == std::string((*p)->attrName))
        {
            continue;
        }
        else
        {
            attrNode = xmlNewTextChild(objectNode,
                                       NULL,
                                       (xmlChar*)"attr",
                                       NULL);
        }
        xmlNewTextChild(attrNode,
                        NULL,
                        (xmlChar*)"name",
                        (xmlChar*)(*p)->attrName);

        valuesToXML(*p, attrNode);
    }
    TRACE_LEAVE();
}

static void valuesToXML(SaImmAttrValuesT_2* p, xmlNodePtr parent)
{
    if (!p->attrValues)
    {
       //std::cout << "No values!" << std::endl;
        return;
    }

    for (unsigned int i = 0; i < p->attrValuesNumber; i++)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"value",
                        (xmlChar*)valueToString(p->attrValues[i], 
                                                p->attrValueType).c_str());
    }
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

static void flagsToXML(SaImmAttrDefinitionT_2* p, xmlNodePtr parent)
{
    SaImmAttrFlagsT flags;

    flags = p->attrFlags;

    if (flags & SA_IMM_ATTR_CONFIG)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"category",
                        (xmlChar*)"SA_CONFIG");
    }

    if (flags & SA_IMM_ATTR_RUNTIME)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"category",
                        (xmlChar*)"SA_RUNTIME");
    }

    if (flags & SA_IMM_ATTR_MULTI_VALUE)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"flag",
                        (xmlChar*)"SA_MULTI_VALUE");
    }


    if (flags & SA_IMM_ATTR_WRITABLE)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"flag",
                        (xmlChar*)"SA_WRITABLE");
    }

    if (flags & SA_IMM_ATTR_INITIALIZED)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"flag",
                        (xmlChar*)"SA_INITIALIZED");
    }

    if (flags & SA_IMM_ATTR_PERSISTENT)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"flag",
                        (xmlChar*)"SA_PERSISTENT");
    }

    if (flags & SA_IMM_ATTR_CACHED)
    {
        xmlNewTextChild(parent,
                        NULL,
                        (xmlChar*)"flag",
                        (xmlChar*)"SA_CACHED");
    }
}

static void typeToXML(SaImmAttrDefinitionT_2* p, xmlNodePtr parent)
{

    switch (p->attrValueType)
    {
        case SA_IMM_ATTR_SAINT32T: 
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_INT32_T");
            break;
        case SA_IMM_ATTR_SAUINT32T: 
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_UINT32_T");
            break;
        case SA_IMM_ATTR_SAINT64T:  
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_INT64_T");
            break;
        case SA_IMM_ATTR_SAUINT64T: 
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_UINT64_T");
            break;
        case SA_IMM_ATTR_SATIMET:   
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_TIME_T");
            break;
        case SA_IMM_ATTR_SAFLOATT:  
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_FLOAT_T");
            break;
        case SA_IMM_ATTR_SADOUBLET: 
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_DOUBLE_T");
            break;
        case SA_IMM_ATTR_SANAMET:
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_NAME_T");
            break;
        case SA_IMM_ATTR_SASTRINGT:
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_STRING_T");
            break;
        case SA_IMM_ATTR_SAANYT:
            xmlNewTextChild(parent,
                            NULL,
                            (xmlChar*)"type",
                            (xmlChar*)"SA_ANY_T");
            break;
        default:
            std::cerr << "Unknown value type" << std::endl;
            exit(1);
    }
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

    errorCode = saImmOmAccessorGet_2(
                                     accessorHandle,
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

