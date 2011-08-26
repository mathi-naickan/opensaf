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

#include <iostream>
#include <list>
#include <map>
#include <set>
#include <string>
#include <libxml/parser.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <configmake.h>

//#include <logtrace.h>  // Will not use of log trace for this command line tool!
#include <saAis.h>
#include <saImmOm.h>
#include <immutil.h>

#define MAX_DEPTH 10
#define MAX_CHAR_BUFFER_SIZE 8192  //8k

// redefine logtrace macros local for this file (instead of logtrace.h) because this is a command line tool
#define LOG_IN(format, args...) log_stderr(LOG_INFO, format, ##args)
#define LOG_WA(format, args...) log_stderr(LOG_WARNING, format, ##args)
#define LOG_ER(format, args...) log_stderr(LOG_ERR, format, ##args)
// Trace statements not that informative, lets skip them...
#define TRACE_8(format, args...)


extern "C"
{
	int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ignore_duplicates);
}

extern ImmutilErrorFnT immutilError;
char* imm_import_adminOwnerName;
static bool imm_import_verbose = false;
static bool imm_import_ignore_duplicates = false;

/* The possible states of the parser */
typedef enum
{
    STARTED,
    DONE,
    CLASS,
    OBJECT,
    DN,
    ATTRIBUTE,
    NAME,
    VALUE,
    CATEGORY,
    RDN,
    DEFAULT_VALUE,
    TYPE,
    FLAG,
    NTFID,
    IMM_CONTENTS
} StatesEnum;

/* The state struct for the parser */
typedef struct ParserStateStruct
{
    int                  immInit;
    int                  adminInit;
    int                  ccbInit;

    int                  doneParsingClasses;
    int                  depth;
    StatesEnum           state[MAX_DEPTH];

    char*                className;
    SaImmClassCategoryT  classCategory;
    int                  classCategorySet;

    /* AttrDefinition parameters */
    char*                attrName;
    SaImmValueTypeT      attrValueType;
    SaImmAttrFlagsT      attrFlags;
    SaUint32T            attrNtfId;
    char*                attrDefaultValueBuffer;

    int                  attrValueTypeSet;
    int                  attrNtfIdSet;
    int                  attrDefaultValueSet;
    int                  attrValueSet;

    std::list<SaImmAttrDefinitionT_2> attrDefinitions;
    std::map<std::string, std::map<std::string, SaImmValueTypeT> > classAttrTypeMap;
    std::set<std::string> adminOwnerSetSet;

    /* Object parameters */
    char*                objectClass;
    char*                objectName;

    std::list<SaImmAttrValuesT_2> attrValues;
    std::list<char*>            attrValueBuffers;
    int                         valueContinue;

    std::map<std::string, SaImmAttrValuesT_2>    classRDNMap;

    SaImmHandleT         immHandle;
    SaImmHandleT         ownerHandle;
    SaImmHandleT         ccbHandle;
} ParserState;

SaImmCallbacksT immCallbacks =
{
};

/* Prototypes */

/* Helper functions */
static void addClassAttributeDefinition(ParserState* state);
static void addObjectAttributeDefinition(ParserState* state);
static void addToAttrTypeCache(ParserState*, SaImmValueTypeT);
static SaImmValueTypeT getClassAttrValueType(ParserState*,
                                             const char*,
                                             const char*);
static void saveRDNAttribute(ParserState* parserState);
static void getDNForClass(ParserState*,
                          const SaImmClassNameT,
                          SaImmAttrValuesT_2*);
static void charsToValueHelper(SaImmAttrValueT*,
                               SaImmValueTypeT,
                               const char*);
static SaImmValueTypeT charsToTypeHelper(const xmlChar* str, size_t len);
static SaImmAttrFlagsT charsToFlagsHelper(const xmlChar* str, size_t len);

static void createImmClass(ParserState*);
static void createImmObject(ParserState*);


/* SAX callback handlers */
static char* getAttributeValue(const char* attr, const xmlChar** const attrArray);

static void errorHandler(void*, const char*, ...);

static void warningHandler(void*, const char*, ...);

static void startElementHandler(void*, const xmlChar*, const xmlChar**);

static void endElementHandler(void*, const xmlChar*);

static void startDocumentHandler(void* userData);

static void endDocumentHandler(void* userData);

static void attributeDeclHandler(void*,
                                 const xmlChar*,
                                 const xmlChar*,
                                 int,
                                 int,
                                 const xmlChar*,
                                 xmlEnumerationPtr);

static void charactersHandler(void*, const xmlChar*, int);

static xmlEntityPtr getEntityHandler(void*, const xmlChar*);

/* Data declarations */
xmlSAXHandler my_handler = {
    NULL,                  //   internalSubsetSAXFunc internalSubset;
    NULL,                  //   isStandaloneSAXFunc isStandalone,
    NULL,                  //   hasInternalSubsetSAXFunc hasInternalSubset,
    NULL,                  //   hasExternalSubsetSAXFunc hasExternalSubset,
    NULL,                  //   resolveEntitySAXFunc resolveEntity,
    getEntityHandler,
    NULL,                  //   entityDeclSAXFunc entityDecl,
    NULL,                  //   notationDeclSAXFunc notationDecl,
    attributeDeclHandler,
    NULL,                  //   elementDeclSAXFunc elementDecl,
    NULL,                  //   unparsedEntityDeclSAXFunc unparsedEntityDecl,
    NULL,                  //   setDocumentLocatorSAXFunc setDocumentLocator,
    startDocumentHandler,
    endDocumentHandler,
    startElementHandler,
    endElementHandler,
    NULL,                  //   referenceSAXFunc reference,
    charactersHandler,
    NULL,                  //   ignorableWhitespaceSAXFunc ignorableWhitespace,
    NULL,                  //   processingInstructionSAXFunc processingInstruction,
    NULL,                  //   commentSAXFunc comment,
    warningHandler,
    errorHandler,
    NULL                   //   fatalErrorSAXFunc fatalError;
};


/* Function bodies */

static void log_stderr_int(int priority, const char *format, va_list args)
{
	char log_string[1024];

	if (priority == LOG_INFO && !imm_import_verbose)
	{
		// only printout INFO statements if -v (verbose) flag is set
		return;
	}

	/* Add line feed if not there already */
	if (format[strlen(format) - 1] != '\n')
	{
		sprintf(log_string, "%s\n", format);
		format = log_string;
	}

	vfprintf(stderr, format, args);
}

static void log_stderr(int priority, const char *format, ...)
{
	va_list args;
	va_start(args, format);
	log_stderr_int(priority, format, args);
	va_end(args);
}


static void imm_importImmutilError(char const *fmt, ...)
    __attribute__ ((format(printf, 1, 2)));

/**
 * Modified version of immutil error handler
 * Report to stderr
 * For command line usage there should be no call to
 * abort()....
 */
static void imm_importImmutilError(char const *fmt, ...)
{
	return;
	/*
	Not relevant to use immutil internal error logging 
	since code below will log if (and only if) call is considered failing

	va_list ap;
	va_list ap2;

	va_start(ap, fmt);
	va_copy(ap2, ap);
	log_stderr_int(LOG_ERR, fmt, ap);
	//vsyslog(LOG_ERR, fmt, ap2);
	va_end(ap);
	//abort();
	*/
}

void setAdminOwnerHelper(ParserState* state, SaNameT *parentOfObject)
{
	/* Function: setAdminOwnerHelper()
	 * This function checks if there is a need to call
	   saImmOmAdminOwnerSet on the parent or if a call is redundant */

	// safe conversion of SaNameT to  char*
	int len = parentOfObject->length;
	char tmpStr[len+1];
	strncpy(tmpStr, (char*) parentOfObject->value, len);
	tmpStr[len] = '\0';

	if (len < 1)
	{
		// this is a root object, no need to set AdminOwner
	    LOG_IN("  This is a root object, no need to call saImmOmAdminOwnerSet");
	    // state->adminOwnerSetSet.insert(state->objectName);
		return;
	}

	std::set<std::string>::iterator set_it = state->adminOwnerSetSet.find(tmpStr);
	if (set_it != state->adminOwnerSetSet.end())
	{
	    LOG_IN("  Already called saImmOmAdminOwnerSet on parent (or parent of parent) '%s'", tmpStr);
	    // lets also store this objectName as "done"
	    state->adminOwnerSetSet.insert(state->objectName);
	    return;
	}

	// if parent does not exist in map then do actual call
    SaNameT* objectNames[2];
    objectNames[0] = parentOfObject;
    objectNames[1] = NULL;

    LOG_IN("  Calling saImmOmAdminOwnerSet on parent '%s'", tmpStr);
    int errorCode = saImmOmAdminOwnerSet(state->ownerHandle,
				     (const SaNameT**) objectNames,
				     SA_IMM_ONE);

    if (SA_AIS_OK != errorCode)
    {
        LOG_ER("Failed to call saImmOmAdminOwnerSet on parent: '%s' , rc =  %d", tmpStr, errorCode);
        exit(1);
    }

    // Store parent and dn of object in set
    state->adminOwnerSetSet.insert(tmpStr);
}


/*
 * This function is inherited from an existing utility to import additional classes/objects
 * If the class definition has not been read from the input we try to
 * read the class and it's RDN from the IMM.
 */
static void getClassFromImm(
    ParserState* state,
    const SaImmClassNameT className)
{
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2 **attrDefinitions;

    SaAisErrorT err = saImmOmClassDescriptionGet_2(
        state->immHandle, className, &classCategory, &attrDefinitions);
    if(err != SA_AIS_OK)
    {
    	LOG_ER("Failed to fetch class description for %s loaded in IMM, "
                   "error code:%u", className, err);
        return;
    }

    LOG_IN("Read class description from IMM: %s",
               className);

    for (SaImmAttrDefinitionT_2** ap = attrDefinitions; *ap != NULL; ap++)
    {
    	SaImmAttrDefinitionT_2* attr = *ap;

        std::string classString = std::string(className);
        std::string attrNameString = std::string(attr->attrName);
        state->classAttrTypeMap[classString][attrNameString] = attr->attrValueType;

        if (attr->attrFlags & SA_IMM_ATTR_RDN)
        {

            SaImmAttrValuesT_2 values;
	    memset(&values, 0, sizeof values);

            size_t len = strlen(attr->attrName);
            values.attrName = (char*)malloc(len + 1);
            if (values.attrName == NULL)
            {
            	LOG_ER("Failed to malloc values.attrName");
                return;
            }
            strncpy(values.attrName, attr->attrName, len);
            values.attrName[len] = '\0';

            values.attrValueType = attr->attrValueType;
            values.attrValuesNumber = 1;

            TRACE_8("ADDED CLASS TO RDN MAP");
            state->classRDNMap[classString] = values;

        }
    }

    saImmOmClassDescriptionMemoryFree_2(state->immHandle, attrDefinitions);
}

/**
 * Creates an Imm Object through the ImmOm interface
 */
static void createImmObject(ParserState* state)
{
    SaImmClassNameT className;
    SaNameT parentName;
    SaImmAttrValuesT_2** attrValues;
    SaAisErrorT errorCode;
    int i;
    size_t DNlen;

    TRACE_8("CREATE IMM OBJECT %s, %s",
            state->objectClass,
            state->objectName);

    LOG_IN("CREATE IMM OBJECT %s, %s",
            state->objectClass,
            state->objectName);

    /* Set the class name */
    className = state->objectClass;

    /* Set the parent name */
    parentName.length = 0;
    if (state->objectName != NULL)
    {
        char* parent;

        /* ',' is the delimeter */
        /* but '\' is the escape character, used for association objects */
        parent = state->objectName;
        do {
            parent = strchr(parent, ',');
            TRACE_8("PARENT: %s", parent);
        } while (parent && (*((++parent)-2)) == '\\');

        if (parent && strlen(parent) <= 1)
        {
            parent = NULL;
        }

        if (parent != NULL)
        {
            parentName.length = (SaUint16T)strlen(parent);
            strncpy((char*)parentName.value, parent, parentName.length);
        }
    } else {
        LOG_ER("Empty DN for object");
        exit(1);
    }

    // checks and sets AdminOwer if needed
    // Note! it must be called before state->objectName is "null truncated"!!
    setAdminOwnerHelper(state, &parentName);

#ifdef TRACE_8
    /* Get the length of the DN and truncate state->objectName */
    if (parentName.length > 0)
    {
        DNlen = strlen(state->objectName) - (parentName.length + 1);
    }
    else
    {
        DNlen = strlen(state->objectName);
    }

    state->objectName[DNlen] = '\0';
    TRACE_8("OBJECT NAME: %s", state->objectName);
#endif

    /* Set the attribute values array, add space for the rdn attribute
     * and a NULL terminator */

    /* Freed at the bottom of the function */
    attrValues = (SaImmAttrValuesT_2**)
                 malloc((state->attrValues.size() + 2)
                        * sizeof(SaImmAttrValuesT_2*));
    if (attrValues == NULL)
    {
        LOG_ER("Failed to malloc attrValues");
        exit(1);
    }

    /* Add the NULL termination */
    attrValues[state->attrValues.size() + 1] = NULL; /* Adjust for RDN */

    std::list<SaImmAttrValuesT_2>::iterator it =
        state->attrValues.begin();

    i = 0;
    while (it != state->attrValues.end())
    {
        attrValues[i] = &(*it);

        i++;
        it++;
    }

    attrValues[i] = (SaImmAttrValuesT_2*)malloc(sizeof(SaImmAttrValuesT_2));
    getDNForClass(state, className, attrValues[i]);


    /* Do the actual creation */
    errorCode = immutil_saImmOmCcbObjectCreate_2(state->ccbHandle,
				       className,
				       &parentName,
				       (const SaImmAttrValuesT_2**)
				       attrValues);

    if (SA_AIS_OK != errorCode)
    {
        if (imm_import_ignore_duplicates && (SA_AIS_ERR_EXIST == errorCode))
        {
            LOG_IN("IGNORE EXISTING OBJECT %s", state->objectName);
        }
        else
        {
            LOG_ER("Failed to create the imm object %s, rc =  %d", state->objectName, errorCode);
            exit(1);
        }
    } else {
	state->adminOwnerSetSet.insert(state->objectName);
    }

    TRACE_8("CREATE DONE");

    /* Free used parameters */
    free(state->objectClass);
    state->objectClass = NULL;
    free(state->objectName);
    state->objectName = NULL;

    /* Free the DN attrName later since it's re-used */
    /*free(attrValues[i]->attrValues);*/
    free(attrValues[i]);
    free(attrValues);


    for (it = state->attrValues.begin();
        it != state->attrValues.end();
        it++)
    {
        free(it->attrName);
        free(it->attrValues);
    }
    state->attrValues.clear();
}

/**
 * Creates an ImmClass through the ImmOm interface
 */
static void createImmClass(ParserState* state)
{
    SaImmClassNameT          className;
    SaImmClassCategoryT      classCategory;
    SaImmAttrDefinitionT_2** attrDefinition;
    SaAisErrorT              errorCode;
    int i;

    TRACE_8("CREATING IMM CLASS %s", state->className);
    LOG_IN("CREATING IMM CLASS %s",
	       state->className);

    /* Set the name */
    className = state->className;

    /* Set the category */
    if (state->classCategorySet)
    {
        classCategory = state->classCategory;
    }
    else
    {
        LOG_ER("NO CLASS CATEGORY");
        exit(1);
    }

    /* Set the attrDefinition array */
    attrDefinition = (SaImmAttrDefinitionT_2**)
                     calloc((state->attrDefinitions.size() + 1),
                            sizeof(SaImmAttrDefinitionT_2*));
    if (attrDefinition == NULL)
    {
        LOG_ER("Failed to malloc attrDefinition");
        exit(1);
    }

    attrDefinition[state->attrDefinitions.size()] = NULL;

    std::list<SaImmAttrDefinitionT_2>::iterator it =
        state->attrDefinitions.begin();

    i = 0;
    while (it != state->attrDefinitions.end())
    {
        attrDefinition[i] = &(*it);

        i++;
        it++;
    }

    errorCode = immutil_saImmOmClassCreate_2(state->immHandle,
                                     className,
                                     classCategory,
                                     (const SaImmAttrDefinitionT_2**)
                                     attrDefinition);

    if (SA_AIS_OK != errorCode)
    {
        if (imm_import_ignore_duplicates && (SA_AIS_ERR_EXIST == errorCode))
        {
            LOG_IN("IGNORE EXISTING CLASS %s", state->className);
        } 
        else
        {
            LOG_ER("FAILED to create IMM class %s, rc = %d", state->className, errorCode);
            exit(1);
        }
    }


    TRACE_8("CREATED IMM CLASS %s", className);

    /* Free all each attrDefinition */
    it = state->attrDefinitions.begin();

    while (it != state->attrDefinitions.end())
    {
        free(it->attrName);
        it->attrName = NULL;
        it++;
    }

    /* Free the attrDefinition array and empty the list */
    free(attrDefinition);
    state->attrDefinitions.clear();

    TRACE_8("<CREATE IMM CLASS");
}

/**
 * Returns an SaImmAttrValueT struct representing the DN for an object
 */
static void getDNForClass(ParserState* state,
                          const SaImmClassNameT className,
                          SaImmAttrValuesT_2* values)
{
    std::string classNameString;

    classNameString = std::string(className);

    if (state->classRDNMap.find(classNameString) ==
        state->classRDNMap.end())
    {
        /* Attempt to read the class from the IMM */
        getClassFromImm(state, className);
        if (state->classRDNMap.find(classNameString) ==
            state->classRDNMap.end())
        {
            LOG_ER("Cannot find CLASS %s loaded in IMM", className);
            exit(1);
        }
    }

    *values = state->classRDNMap[classNameString];

    values->attrValues = (SaImmAttrValueT*)malloc(sizeof(SaImmAttrValueT));

    values->attrValuesNumber = 1;

    charsToValueHelper(values->attrValues,
                       values->attrValueType,
                       state->objectName);
}


static void errorHandler(void* userData,
                         const char* msg,
                         ...)
{
    LOG_ER("Error occured during parsing: %s", msg);
    exit(1);
}

static void warningHandler(void* userData,
                           const char* msg,
                           ...)
{
    LOG_WA("Warning occured during parsing: %s", msg);
}

/**
 * This is the handler for start tags
 */
static void startElementHandler(void* userData,
                                const xmlChar*  name,
                                const xmlChar** attrs)
{
    ParserState* state;

    TRACE_8("TAG %s", name);

    state = (ParserState*) userData;
    state->depth++;

    if (state->depth >= MAX_DEPTH)
    {
        LOG_ER( "The document is too deply nested");
        exit(1);
    }

    /* <class ...> */
    if (strcmp((const char*)name, "class") == 0)
    {
        char* nameAttr;

        if (state->doneParsingClasses)
        {
            LOG_ER("CLASS TAG AT INVALID PLACE IN XML");
            exit(1);
        }

        state->state[state->depth] = CLASS;
        nameAttr = getAttributeValue("name", attrs);

        if (nameAttr != NULL)
        {
            size_t len;

            len = strlen(nameAttr);

            state->className = (char*)malloc(len + 1);
            strcpy(state->className, nameAttr);
            state->className[len] = '\0';

            TRACE_8("CLASS NAME: %s", state->className);
        }
        else
        {
            LOG_ER( "NAME ATTRIBUTE IS NULL");
            exit(1);
        }
        /* <object ...> */
    }
    else if (strcmp((const char*)name, "object") == 0)
    {
        char* classAttr;

        state->attrFlags = 0;

        state->state[state->depth] = OBJECT;

        /* Get the class attribute */
        classAttr = getAttributeValue("class", attrs);

        if (classAttr != NULL)
        {
            size_t len;

            len = strlen(classAttr);

            state->objectClass = (char*)malloc(len + 1);
            strncpy(state->objectClass, classAttr, len);
            state->objectClass[len] = '\0';

            TRACE_8("OBJECT CLASS NAME: %s",
                    state->objectClass);
        }
        else
        {
            LOG_ER("OBJECT %s HAS NO CLASS ATTRIBUTE", state->objectName);
            exit(1);
        }

        /* <dn> */
    }
    else if (strcmp((const char*)name, "dn") == 0)
    {
        state->state[state->depth] = DN;
        /* <attr> */
    }
    else if (strcmp((const char*)name, "attr") == 0)
    {
        state->state[state->depth] = ATTRIBUTE;

        state->attrFlags = 0x0;
        state->attrName  = NULL;
        state->attrDefaultValueBuffer = NULL;

        state->attrValueTypeSet    = 0;
        state->attrNtfIdSet        = 0;
        state->attrDefaultValueSet = 0;
        /* <name> */
    }
    else if (strcmp((const char*)name, "name") == 0)
    {
        state->state[state->depth] = NAME;
        /* <value> */
    }
    else if (strcmp((const char*)name, "value") == 0)
    {
        state->state[state->depth] = VALUE;
        state->valueContinue = 0;
        /* <category> */
    }
    else if (strcmp((const char*)name, "category") == 0)
    {
        state->state[state->depth] = CATEGORY;
        /* <rdn> */
    }
    else if (strcmp((const char*)name, "rdn") == 0)
    {
        state->attrFlags        = SA_IMM_ATTR_RDN;

        state->attrName = NULL;

        state->attrDefaultValueBuffer = NULL;

        state->attrValueTypeSet    = 0;
        state->attrDefaultValueSet = 0;
        state->state[state->depth] = RDN;
        /* <default-value> */
    }
    else if (strcmp((const char*)name, "default-value") == 0)
    {
        state->state[state->depth] = DEFAULT_VALUE;
        /* <type> */
    }
    else if (strcmp((const char*)name, "type") == 0)
    {
        state->state[state->depth] = TYPE;
        /* <flag> */
    }
    else if (strcmp((const char*)name, "flag") == 0)
    {
        state->state[state->depth] = FLAG;
        /* <ntfid> */
    }
    else if (strcmp((const char*)name, "ntfid") == 0)
    {
        state->state[state->depth] = NTFID;
        /* <imm:IMM-contents> */
    }
    else if (strcmp((const char*)name, "imm:IMM-contents") == 0)
    {
        state->state[state->depth] = IMM_CONTENTS;
    }
    else
    {
        LOG_ER("UNKNOWN TAG! (%s)", name);
        exit(1);
    }
}

/**
 * Called when an end-tag is reached
 */
static void endElementHandler(void* userData,
                              const xmlChar* name)
{
    ParserState* state;

    state = (ParserState*)userData;

    TRACE_8("END %s", name);

    /* </value> */
    if (strcmp((const char*)name, "value") == 0)
    {
        if (state->attrValueBuffers.size() == 0)
        {
            char* str = (char*)malloc(1);

            str[0] = '\0';
            state->attrValueBuffers.push_front(str);
        }

        state->valueContinue = 0; /* Actually redundant. */

        /* </default-value> */
    }
    else if (strcmp((const char*)name, "default-value") == 0)
    {
        if (state->attrDefaultValueBuffer == NULL ||
            !state->attrDefaultValueSet)
        {
            state->attrDefaultValueBuffer = (char*)malloc(1);

            state->attrDefaultValueBuffer[0] = '\0';
            state->attrDefaultValueSet = 1;
            TRACE_8("EMPTY DEFAULT-VALUE TAG");
            TRACE_8("Attribute: %s", state->attrName);
        }
        /* </class> */
    }
    else if (strcmp((const char*)name, "class") == 0)
    {
        if (state->doneParsingClasses)
        {
            LOG_ER("INVALID CLASS PLACEMENT");
            exit(1);
        }
        else
        {
            createImmClass(state);
            state->attrFlags = 0;

            state->attrValueTypeSet    = 0;
            state->attrNtfIdSet        = 0;
            state->attrDefaultValueSet = 0;
        }
        /* </attr> or </rdn> */
    }
    else if (strcmp((const char*)name, "attr") == 0 ||
             strcmp((const char*)name, "rdn") == 0)
    {
        if (state->state[state->depth - 1] == CLASS)
        {
            addClassAttributeDefinition(state);
        }
        else
        {
            addObjectAttributeDefinition(state);
        }
        /* </object> */
    }
    else if (strcmp((const char*)name, "object") == 0)
    {
        TRACE_8("END OBJECT");
        if (!state->doneParsingClasses)
        {
            TRACE_8("CCB INIT");
            SaAisErrorT errorCode;

            state->doneParsingClasses = 1;

            /* First time, initialize the imm object api */
            TRACE_8("\n AdminOwner: %s \n", imm_import_adminOwnerName);
            errorCode = saImmOmAdminOwnerInitialize(state->immHandle,
                                                    imm_import_adminOwnerName,
                                                    SA_TRUE,
                                                    &state->ownerHandle);
            if (errorCode != SA_AIS_OK)
            {
                LOG_ER("Failed on saImmOmAdminOwnerInitialize %d",
                       errorCode);
                exit(1);
            }
            state->adminInit = 1;

            /* ... and initialize the imm ccb api  */
            errorCode = saImmOmCcbInitialize(state->ownerHandle,
                                             0
                                             /*(SaImmCcbFlagsT)
                                               SA_IMM_CCB_REGISTERED_OI*/,
                                             &state->ccbHandle);
            if (errorCode != SA_AIS_OK)
            {
                LOG_ER("Failed to initialize ImmOmCcb %d", errorCode);
                exit(1);
            }
            state->ccbInit = 1;

        }

        /* Create the object */
        createImmObject(state);
        /* </imm:IMM-contents> */
    }
    else if (strcmp((const char*)name, "imm:IMM-contents") == 0)
    {
        SaAisErrorT errorCode;

        /* Apply the object creations */
        if (state->ccbInit)
        {
            errorCode = saImmOmCcbApply(state->ccbHandle);
            if (SA_AIS_OK != errorCode)
            {
                LOG_ER("Failed to apply object creations %d", errorCode);
                exit(1);
            }
        }

        /* Finalize the ccb connection*/
        if (state->ccbInit)
        {
            errorCode = saImmOmCcbFinalize(state->ccbHandle);
            if (SA_AIS_OK != errorCode)
            {
                LOG_WA("Failed to finalize the ccb object connection %d",
                       errorCode);
            }
            else
            {
                state->ccbInit = 0;
            }
        }

        /* Finalize the owner connection */
        if (state->adminInit)
        {
            errorCode = saImmOmAdminOwnerFinalize(state->ownerHandle);
            if (SA_AIS_OK != errorCode)
            {
                LOG_WA("Failed on saImmOmAdminOwnerFinalize (%d)",
                       errorCode);
            }
            else
            {
                state->adminInit = 0;
            }
        }

        /* Finalize the imm connection */
        if (state->immInit)
        {
            errorCode = saImmOmFinalize(state->immHandle);
            if (SA_AIS_OK != errorCode)
            {
                LOG_WA("Failed on saImmOmFinalize (%d)", errorCode);
            }

            state->immInit = 0;
        }
    }

    ((ParserState*)userData)->depth--;
}

static void startDocumentHandler(void* userData)
{
    ParserState* state;
    state = (ParserState*)userData;

    state->depth = 0;
    state->state[0] = STARTED;
}

static void endDocumentHandler(void* userData)
{
    if (((ParserState*)userData)->depth != 0)
    {
        LOG_ER( "Document ends too early\n");
        exit(1);
    }
    TRACE_8("endDocument occured\n");
}

static void attributeDeclHandler(void *ctx,
                                 const xmlChar *elem,
                                 const xmlChar *fullname,
                                 int type,
                                 int def,
                                 const xmlChar *defaultValue,
                                 xmlEnumerationPtr tree)
{
    TRACE_8("attributeDecl occured\n");
}

static void charactersHandler(void* userData,
                              const xmlChar* chars,
                              int len)
{
    ParserState* state;
    std::list<char*>::iterator it;

    state = (ParserState*)userData;

    if (len > MAX_CHAR_BUFFER_SIZE)
    {
        LOG_ER("The character field is too big len(%d) > max(%d)",
            len, MAX_CHAR_BUFFER_SIZE);
        exit(1);
    }
    else if (len < 0)
    {
        LOG_ER("The character array length is negative %d", len);
        exit(1);
    }

    /* Treat each tag type separately */
    switch (state->state[state->depth])
    {
        case IMM_CONTENTS:
            break;
        case STARTED:
            break;
        case DONE:
            break;
        case CLASS:
            break;
        case OBJECT:
            break;
        case ATTRIBUTE:
            break;
        case DN:
            /* Copy the distinguished name */
            assert(len < SA_MAX_NAME_LENGTH);
            state->objectName = (char*)malloc((size_t)len + 1);

            strncpy(state->objectName, (const char*)chars, (size_t)len);

            state->objectName[len] = '\0';

            break;
        case NAME:
            /* The attrName must be NULL */
            assert(!state->attrName);

            if (state->state[state->depth - 1] == ATTRIBUTE ||
                state->state[state->depth - 1] == RDN)
            {
                state->attrName = (char*)malloc((size_t)len + 1);
                if (state->attrName == NULL)
                {
                    LOG_ER("Failed to malloc state->attrName");
                    exit(1);
                }

                strncpy(state->attrName, (const char*)chars, (size_t)len);
                state->attrName[len] = '\0';
            }
            else
            {
                LOG_ER("Name not immediately inside an attribute tag");
                exit(1);
            }
            break;
        case VALUE:
            if (state->state[state->depth - 1] == ATTRIBUTE)
            {
                char* str;

                if(!state->valueContinue)
                {
                    /* Start of value, also end of value for 99.999% of cases */
                    state->valueContinue = 1;
                    str = (char*)malloc((size_t)len + 1);
                    if (str == NULL)
                    {
                        LOG_ER("Failed to malloc value");
                        exit(1);
                    }

                    strncpy(str, (const char*)chars, (size_t)len);
                    str[len] = '\0';

                    state->attrValueBuffers.push_front(str);
                } else {
                     /* CONTINUATION of CURRENT value, typically only happens for loooong strings. */
                    TRACE_8("APPEND TO CURRENT VALUE");

                    size_t oldsize = strlen(state->attrValueBuffers.front());
                    TRACE_8("APPEND VALUE newsize:%u", oldsize + len + 1);

                    str = (char *) malloc(oldsize + len + 1);
                    if (str == NULL)
                    {
                        LOG_ER("Failed to malloc value");
                        exit(1);
                    }
                    strncpy(str, state->attrValueBuffers.front(), oldsize + 1);
                    TRACE_8("COPIED OLD VALUE %u %s", oldsize, str);

                    strncpy(str + oldsize, (const char*)chars, (size_t)len + 1);
                    str[oldsize + len] = '\0';
                    LOG_IN("APPENDED NEW VALUE newsize %u %s", oldsize + len + 1, str);

                    /* Remove the old string */
                    free(state->attrValueBuffers.front());
                    state->attrValueBuffers.pop_front();
                    /* state->attrValueBuffers.clear();
                       clear not appropriate since we could ALSO have several values!
                       We are here only operating on the front value in the list.
                    */
                    state->attrValueBuffers.push_front(str);
                }

            }
            else
            {
                LOG_ER("UNKNOWN PLACEMENT OF VALUE");
                exit(1);
            }
            break;
        case CATEGORY:
            if (state->state[state->depth - 1] == CLASS)
            {
                SaImmClassCategoryT category;
                if (strncmp((const char*)chars, "SA_CONFIG", (size_t)len) == 0)
                {
                    category = SA_IMM_CLASS_CONFIG;
                }
                else
                {
                    category = SA_IMM_CLASS_RUNTIME;
                }

                state->classCategorySet = 1;
                state->classCategory = category;
            }
            else if (state->state[state->depth - 1] == ATTRIBUTE ||
                     state->state[state->depth - 1] == RDN)
            {
                SaImmAttrFlagsT category;
                if (strncmp((const char*)chars, "SA_CONFIG", (size_t)len) == 0)
                {
                    category = SA_IMM_ATTR_CONFIG;
                }
                else
                {
                    category = SA_IMM_ATTR_RUNTIME;
                }
                state->attrFlags = state->attrFlags | category;
            }

            break;
        case DEFAULT_VALUE:
            if (state->state[state->depth - 1] == ATTRIBUTE)
            {
                if (state->attrDefaultValueBuffer == NULL){
                    state->attrDefaultValueBuffer = (char*)malloc((size_t)len + 1);
                    strncpy(state->attrDefaultValueBuffer,
                        (const char*)chars,
                        (size_t)len);
                    state->attrDefaultValueBuffer[len] = '\0';
                    state->attrDefaultValueSet = 1;
                } else {
                    /* The defaultValueBuffer contains data from previous
                     * call for same value */
                    assert(state->attrDefaultValueSet);
                    int newlen = strlen(state->attrDefaultValueBuffer)+len;
                    state->attrDefaultValueBuffer = (char*) realloc((void*)state->attrDefaultValueBuffer, (size_t) newlen+1);
                    strncat(state->attrDefaultValueBuffer,
                	                        (const char*)chars,
                	                        (size_t)len);
                    state->attrDefaultValueBuffer[newlen] = '\0';
                }
            }
            else
            {
                LOG_ER("UNKNOWN PLACEMENT OF DEFAULT VALUE");
                exit(1);
            }
            break;
        case TYPE:
            state->attrValueType = charsToTypeHelper(chars, (size_t)len);
            state->attrValueTypeSet = 1;

            addToAttrTypeCache(state, state->attrValueType);

            break;
        case RDN:

            break;
        case FLAG:
            state->attrFlags =
                state->attrFlags | charsToFlagsHelper(chars, (size_t)len);
            break;
        case NTFID:
            if (atoi((const char*)chars) < 0)
            {
                LOG_ER("NtfId can not be negative");
                exit(1);
            }

            state->attrNtfId = (SaUint32T)atoi((const char*)chars);
            state->attrNtfIdSet = 1;
            break;
        default:
            LOG_ER("Unknown state");
            exit(1);
    }

}

static xmlEntityPtr
    getEntityHandler(void *user_data, const xmlChar *name)
{
    return xmlGetPredefinedEntity(name);
}

/**
 * Takes a string and returns the corresponding flag
 */
static SaImmAttrFlagsT charsToFlagsHelper(const xmlChar* str, size_t len)
{
    if (strncmp((const char*)str, "SA_MULTI_VALUE", len) == 0)
    {
        return SA_IMM_ATTR_MULTI_VALUE;
    }
    else if (strncmp((const char*)str, "SA_RDN", len) == 0)
    {
        return SA_IMM_ATTR_RDN;
    }
    else if (strncmp((const char*)str, "SA_CONFIG", len) == 0)
    {
        return SA_IMM_ATTR_CONFIG;
    }
    else if (strncmp((const char*)str, "SA_WRITABLE", len ) == 0)
    {
        return SA_IMM_ATTR_WRITABLE;
    }
    else if (strncmp((const char*)str, "SA_INITIALIZED", len) == 0)
    {
        return SA_IMM_ATTR_INITIALIZED;
    }
    else if (strncmp((const char*)str, "SA_RUNTIME", len ) == 0)
    {
        return SA_IMM_ATTR_RUNTIME;
    }
    else if (strncmp((const char*)str, "SA_PERSISTENT", len ) == 0)
    {
        return SA_IMM_ATTR_PERSISTENT;
    }
    else if (strncmp((const char*)str, "SA_CACHED", len) == 0)
    {
        return SA_IMM_ATTR_CACHED;
    }

    LOG_ER("UNKNOWN FLAGS, %s", str);

    exit(1);
}

/**
 * Takes a string and returns the corresponding valueType
 */
static SaImmValueTypeT charsToTypeHelper(const xmlChar* str, size_t len)
{
    if (strncmp((const char*)str, "SA_NAME_T", len) == 0)
    {
        return SA_IMM_ATTR_SANAMET;
    }
    else if (strncmp((const char*)str, "SA_INT32_T", len ) == 0)
    {
        return SA_IMM_ATTR_SAINT32T;
    }
    else if (strncmp((const char*)str, "SA_UINT32_T", len) == 0)
    {
        return SA_IMM_ATTR_SAUINT32T;
    }
    else if (strncmp((const char*)str, "SA_INT64_T", len ) == 0)
    {
        return SA_IMM_ATTR_SAINT64T;
    }
    else if (strncmp((const char*)str, "SA_UINT64_T", len) == 0)
    {
        return SA_IMM_ATTR_SAUINT64T;
    }
    else if (strncmp((const char*)str, "SA_TIME_T", len  ) == 0)
    {
        return SA_IMM_ATTR_SATIMET;
    }
    else if (strncmp((const char*)str, "SA_NAME_T", len  ) == 0)
    {
        return SA_IMM_ATTR_SANAMET;
    }
    else if (strncmp((const char*)str, "SA_FLOAT_T", len ) == 0)
    {
        return SA_IMM_ATTR_SAFLOATT;
    }
    else if (strncmp((const char*)str, "SA_DOUBLE_T", len) == 0)
    {
        return SA_IMM_ATTR_SADOUBLET;
    }
    else if (strncmp((const char*)str, "SA_STRING_T", len) == 0)
    {
        return SA_IMM_ATTR_SASTRINGT;
    }
    else if (strncmp((const char*)str, "SA_ANY_T", len   ) == 0)
    {
        return SA_IMM_ATTR_SAANYT;
    }

    LOG_ER("UNKNOWN VALUE TYPE, %s", str);

    exit(1);
}

/**
 * Returns the value for a given attribute name in the xml attribute array
 */
static char* getAttributeValue(const char* attr,
                               const xmlChar** const attrArray)
{
    int i;

    if (attrArray == NULL)
    {
        LOG_ER("The document is TOO DEEPLY NESTED");
        exit(1);
    }

    for (i = 0; attrArray != NULL && attrArray[i*2] != NULL; i++)
    {
        if (strcmp(attr, (const char*)attrArray[i*2]) == 0)
        {
            return(char*)attrArray[i*2 + 1];
        }
    }

    LOG_WA( "RETURNING NULL");
    return NULL;
}

/**
 * Adds an object attr definition to the state->attrValues list
 */
static void addObjectAttributeDefinition(ParserState* state)
{
    std::list<char*>::iterator it;
    SaImmAttrValuesT_2 attrValues;
    int i;
    size_t len;

    /* The attrName must be set */
    assert(state->attrName);

    /* The value array can not be empty */
    assert(state->attrValueBuffers.size() != 0);

    /* The object class must be set */
    assert(state->objectClass);

    /* Set the valueType */
    attrValues.attrValueType = getClassAttrValueType(state,
                                                     state->objectClass,
                                                     state->attrName);

    TRACE_8("addObjectAttributeDefinition %s, %s, %d",
            state->className,
            state->attrName,
            attrValues.attrValueType);

    /* For each value, convert from char* to SaImmAttrValuesT_2 and
       store an array pointing to all in attrValues */
    attrValues.attrValuesNumber = state->attrValueBuffers.size();
    attrValues.attrValues = (SaImmAttrValueT*)
                            malloc(sizeof(SaImmAttrValuesT_2) *
                                   attrValues.attrValuesNumber + 1);

    attrValues.attrValues[attrValues.attrValuesNumber] = NULL;

    it = state->attrValueBuffers.begin();
    i = 0;
    while (it != state->attrValueBuffers.end())
    {
        TRACE_8("NAME: %s", state->attrName);

        charsToValueHelper(&attrValues.attrValues[i],
                           attrValues.attrValueType,
                           *it);

        i++;
        it++;
    }

    /* Assign the name */
    len = strlen(state->attrName);
    attrValues.attrName = (char*) malloc(len + 1);
    if (attrValues.attrName == NULL)
    {
        LOG_ER("Failed to malloc attrValues.attrName");
        exit(1);
    }
    strncpy(attrValues.attrName,
            state->attrName,
            len);
    attrValues.attrName[len] = '\0';

    /* Add attrValues to the list */
    state->attrValues.push_front(attrValues);

    /* Free unneeded data */
    for (it = state->attrValueBuffers.begin();
        it != state->attrValueBuffers.end();
        it++)
    {
        free(*it);
    }

    state->attrValueBuffers.clear();
    assert(state->attrValueBuffers.size() == 0);
}

/**
 * Saves the rdn attribute for a class in a map
 */
static void saveRDNAttribute(ParserState* state)
{
    if (state->attrFlags & SA_IMM_ATTR_RDN &&
        state->classRDNMap.find(std::string(state->className)) ==
        state->classRDNMap.end())
    {
        SaImmAttrValuesT_2 values;
        size_t len;

        /* Set the RDN name */
        len = strlen(state->attrName);
        values.attrName = (char*)malloc(len + 1);
        if (values.attrName == NULL)
        {
            LOG_ER( "Failed to malloc values.attrName");
            exit(1);
        }

        strncpy(values.attrName, state->attrName, len);
        values.attrName[len] = '\0';

        /* Set the valueType */
        values.attrValueType = state->attrValueType;

        /* Set the number of attr values */
        values.attrValuesNumber = 1;

        values.attrValues = NULL;

        TRACE_8("ADDED CLASS TO RDN MAP");
        state->classRDNMap[std::string(state->className)] =
            values;
    }
}

/**
 * Adds the given valueType to the a mapping in the state
 *
 * Mapping: class -> attribute name -> type
 */
static void addToAttrTypeCache(ParserState* state,
                               SaImmValueTypeT valueType)
{
    /* className and attrName can not be NULL */
    assert(state->className && state->attrName);

    std::string classString;
    std::string attrNameString;

    classString = std::string(state->className);
    attrNameString = std::string(state->attrName);
    state->classAttrTypeMap[classString][attrNameString] = valueType;
}

/**
 * Returns the valueType for a given state, classname and attribute name
 */
static SaImmValueTypeT getClassAttrValueType(ParserState* state,
                                             const char* className,
                                             const char* attrName)
{
    std::string classNameString;
    std::string attrNameString;

    std::map<std::string, std::map<std::string, std::string> >::iterator classIt;
    std::map<std::string, std::string>::iterator attrIt;

    classNameString = std::string(className);
    attrNameString = std::string(attrName);

    if (state->classAttrTypeMap.find(classNameString) ==
        state->classAttrTypeMap.end())
    {
        getClassFromImm(state, (const SaImmClassNameT)className);
        if (state->classAttrTypeMap.find(classNameString) ==
            state->classAttrTypeMap.end())
        {
            LOG_ER("Cannot find CLASS %s loaded in IMM", className);
            exit(1);
        }
    }


    if (state->classAttrTypeMap[classNameString].find(attrNameString) ==
        state->classAttrTypeMap[classNameString].end())
    {
        LOG_ER("NO CORRESPONDING ATTRIBUTE %s in class %s", attrName,
               className);
        exit(1);
    }

    return state->classAttrTypeMap[classNameString][attrNameString];
}

/**
 * Adds an class attribute definition to the list
 */
static void addClassAttributeDefinition(ParserState* state)
{
    SaImmAttrDefinitionT_2 attrDefinition;

    /* Set the name */
    if (state->attrName != NULL)
    {
        attrDefinition.attrName = state->attrName;
    }
    else
    {
        LOG_ER( "NO ATTR NAME");
        exit(1);
    }

    /* Save the attribute definition in classRDNMap if the RDN flag is
     * set */
    saveRDNAttribute(state);

    /* Set attrValueType */
    assert(state->attrValueTypeSet);
    attrDefinition.attrValueType = state->attrValueType;
    if (state->state[state->depth] == RDN)
    {
        TRACE_8("ADDING RDN!");
        /*
        // Work-around since the IMM-server only accepts SaNameT RDN
        if (attrDefinition.attrValueType != SA_IMM_ATTR_SANAMET)
        {
            LOG_IN("RDN of type SA_STRING_T is not supported. "
                   "Changed to SA_NAME_T for class %s",
                   state->className);
        }

        attrDefinition.attrValueType = SA_IMM_ATTR_SANAMET;
        */
    }
    else if (state->attrValueTypeSet)
    {
        attrDefinition.attrValueType = state->attrValueType;
        TRACE_8("ATTR %s, %d",
                attrDefinition.attrName,
                attrDefinition.attrValueType);
    }
    else
    {
        LOG_ER("NO ATTR VALUE TYPE");
        exit(1);
    }

    /* Set the flags */
    attrDefinition.attrFlags = state->attrFlags;

    /* Set the NtfId */
    if (state->attrNtfIdSet)
    {
        LOG_WA("IGNORING NTF-ID FOR CLASS CREATE");
        //attrDefinition.attrNtfId = state->attrNtfId;
    }
    else
    {
        //TRACE_8("NO ATTR NTF ID");
        //attrDefinition.attrNtfId = 0;
    }

    /* Set the default value */
    if (state->attrDefaultValueSet)
    {
        charsToValueHelper(&attrDefinition.attrDefaultValue,
                           state->attrValueType,
                           state->attrDefaultValueBuffer);
    }
    else
    {
        attrDefinition.attrDefaultValue = NULL;
    }

    /* Add to the list of attrDefinitions */
    state->attrDefinitions.push_front(attrDefinition);

    /* Free the default value */
    free(state->attrDefaultValueBuffer);
    state->attrDefaultValueBuffer = NULL;
}

/**
 * Converts an array of chars to an SaImmAttrValueT
 */
static void charsToValueHelper(SaImmAttrValueT* value,
                               SaImmValueTypeT type,
                               const char* str)
{
    size_t len;
    unsigned int i;
    char byte[5];
    char* endMark;

    TRACE_8("CHARS TO VALUE HELPER");

    switch (type)
    {
        case SA_IMM_ATTR_SAINT32T:
            *value = malloc(sizeof(SaInt32T));
            *((SaInt32T*)*value) = (SaInt32T)strtol(str, NULL, 0);
            break;
        case SA_IMM_ATTR_SAUINT32T:
            *value = malloc(sizeof(SaUint32T));
            *((SaUint32T*)*value) = (SaUint32T)strtoul(str, NULL, 0);
            break;
        case SA_IMM_ATTR_SAINT64T:
            *value = malloc(sizeof(SaInt64T));
            *((SaInt64T*)*value) = (SaInt64T)strtoll(str, NULL, 0);
            break;
        case SA_IMM_ATTR_SAUINT64T:
            *value = malloc(sizeof(SaUint64T));
            *((SaUint64T*)*value) = (SaUint64T)strtoull(str, NULL, 0);
            break;
        case SA_IMM_ATTR_SATIMET: // Int64T
            *value = malloc(sizeof(SaInt64T));
            *((SaTimeT*)*value) = (SaTimeT)strtoll(str, NULL, 0);
            break;
        case SA_IMM_ATTR_SANAMET:
            len = strlen(str);
            assert(len < SA_MAX_NAME_LENGTH);
            *value = malloc(sizeof(SaNameT));
            ((SaNameT*)*value)->length = (SaUint16T)len;
            strncpy((char*)((SaNameT*)*value)->value, str, len);
            ((SaNameT*)*value)->value[len] = '\0';
            break;
        case SA_IMM_ATTR_SAFLOATT:
            *value = malloc(sizeof(SaFloatT));
            *((SaFloatT*)*value) = (SaFloatT)atof(str);
            break;
        case SA_IMM_ATTR_SADOUBLET:
            *value = malloc(sizeof(SaDoubleT));
            *((SaDoubleT*)*value) = (SaDoubleT)atof(str);
            break;
        case SA_IMM_ATTR_SASTRINGT:
            len = strlen(str);
            *value = malloc(sizeof(SaStringT));
            *((SaStringT*)*value) = (SaStringT)malloc(len + 1);
            strncpy(*((SaStringT*)*value), str, len);
            (*((SaStringT*)*value))[len] = '\0';
            break;
        case SA_IMM_ATTR_SAANYT:
            len = strlen(str) / 2;
            *value = malloc(sizeof(SaAnyT));
            ((SaAnyT*)*value)->bufferAddr =
                (SaUint8T*)malloc(sizeof(SaUint8T) * len);
            ((SaAnyT*)*value)->bufferSize = len;

            byte[0] = '0';
            byte[1] = 'x';
            byte[4] = '\0';

            endMark = byte + 4;

            for (i = 0; i < len; i++)
            {
                byte[2] = str[2*i];
                byte[3] = str[2*i + 1];

                ((SaAnyT*)*value)->bufferAddr[i] =
                    (SaUint8T)strtod(byte, &endMark);
            }

            TRACE_8("SIZE: %d", (int) ((SaAnyT*)*value)->bufferSize);
            break;
        default:
            LOG_ER("BAD VALUE TYPE");
            exit(1);
    }
}

int loadImmXML(std::string xmlfile)
{
    ParserState state;
    SaVersionT version;
    SaAisErrorT errorCode;
    int result;

    version.releaseCode   = 'A';
    version.majorVersion  = 2;
    version.minorVersion  = 1;

    TRACE_8("Loading from %s", xmlfile.c_str());

    errorCode = saImmOmInitialize(&(state.immHandle),
                                  &immCallbacks,
                                  &version);
    if (SA_AIS_OK != errorCode)
    {
        LOG_ER("Failed to initialize the IMM OM interface (%d)", errorCode);
        exit(1);
    }
    state.immInit = 1;

    /* Initialize the state */
    state.attrDefaultValueBuffer = NULL;
    state.objectClass            = NULL;
    state.objectName             = NULL;
    state.className              = NULL;
    state.attrName               = NULL;
    state.classCategorySet    = 0;
    state.attrValueTypeSet    = 0;
    state.attrNtfIdSet        = 0;
    state.attrDefaultValueSet = 0;
    state.attrValueSet        = 0;
    state.doneParsingClasses  = 0;
    state.depth               = 0;
    state.attrFlags           = 0;

    state.immInit   = 0;
    state.adminInit = 0;
    state.ccbInit   = 0;


    //std::cout << "Loading " << xmlfile << std::endl;

    result = xmlSAXUserParseFile(&my_handler, &state, xmlfile.c_str());

    /* Make sure to finalize the imm connections */
    /* Finalize the ccb connection*/
    if (state.ccbInit)
    {
        errorCode = saImmOmCcbFinalize(state.ccbHandle);
        if (SA_AIS_OK != errorCode)
        {
            LOG_WA("Failed to finalize the ccb object connection %d",
                   errorCode);
        }
    }

    /* Finalize the owner connection */
    if (state.adminInit)
    {
        errorCode = saImmOmAdminOwnerFinalize(state.ownerHandle);
        if (SA_AIS_OK != errorCode)
        {
            LOG_WA("Failed on saImmOmAdminOwnerFinalize (%d)", errorCode);
        }
    }

    /* Finalize the imm connection */
    if (state.immInit)
    {
        errorCode = saImmOmFinalize(state.immHandle);
        if (SA_AIS_OK != errorCode)
        {
            LOG_WA("Failed on saImmOmFinalize (%d)", errorCode);
        }
    }

    return result;
}


// C and c++ caller wrapper
//  The objective is to keep the code copied from imm_load.cc as close to original as possible
//  to ease a future refactoring towards common codebase
int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ignore_duplicates)
{
	std::string xmlfile(xmlfileC);
	imm_import_adminOwnerName = adminOwnerName;
	imm_import_verbose = verbose;
        imm_import_ignore_duplicates = ignore_duplicates;
	LOG_IN("file: %s adminOwner: %s", xmlfileC, adminOwnerName);

	// assign own immutil errorhandler (no call to abort())
	immutilError = imm_importImmutilError;

	return loadImmXML(xmlfile);
}
