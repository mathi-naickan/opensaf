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
#include <libxml/tree.h>
#include <libxml/xpath.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <assert.h>
#include <syslog.h>
#include <configmake.h>
#include <sys/stat.h>
#include <errno.h>

#include <saAis.h>
#include <saImmOm.h>
#include <immutil.h>

#define MAX_DEPTH 10
#define MAX_CHAR_BUFFER_SIZE 8192  //8k

static char base64_dec_table[] = {
		62,																										/* + 			*/
		-1, -1, -1,																								/* 0x2c-0x2e 	*/
		63,																										/* / 			*/
		52, 53, 54, 55, 56, 57, 58, 59, 60, 61,																	/* 0-9 			*/
		-1, -1, -1, -1, -1, -1, -1,																				/* 0x3a-0x40	*/
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25,			/* A-Z 			*/
		-1, -1, -1, -1, -1, -1,																					/* 0x5b-0x60 	*/
		26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51,	/* a-z 			*/
};

// redefine logtrace macros local for this file (instead of logtrace.h) because this is a command line tool
#define LOG_ER(format, args...) log_stderr(LOG_ERR, format, ##args)
#define LOG_WA(format, args...) log_stderr(LOG_WARNING, format, ##args)
#define LOG_NO(format, args...) log_stderr(LOG_NOTICE, format, ##args)
#define LOG_IN(format, args...) log_stderr(LOG_INFO, format, ##args)

// Trace statements not that informative, lets skip them...
#define TRACE_8(format, args...)

extern "C"
{
	int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ccb_safe,
			SaImmHandleT *immHandle, SaImmAdminOwnerHandleT *ownerHandle,
			SaImmCcbHandleT *ccbHandle, int mode, const char *xsdPath);
	int validateImmXML(const char *xmlfile, int verbose, int mode);
}

extern ImmutilErrorFnT immutilError;
static char* imm_import_adminOwnerName;
static bool imm_import_verbose = false;
static bool imm_import_ccb_safe = true;

static int transaction_mode;
static SaImmHandleT *imm_import_immHandle;
static SaImmAdminOwnerHandleT *imm_import_ownerHandle;
static SaImmCcbHandleT *imm_import_ccbHandle;


/* The possible states of the parser */
typedef enum {
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
typedef struct ParserStateStruct {
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
	int                         isBase64Encoded;

	std::map<std::string, SaImmAttrValuesT_2>    classRDNMap;

	SaImmHandleT         immHandle;
	SaImmAdminOwnerHandleT ownerHandle;
	SaImmCcbHandleT      ccbHandle;

	bool validation;
	xmlParserCtxtPtr ctxt;
	int parsingStatus;		/* 0 = ok */
} ParserState;

bool isXsdLoaded = false;
static const char *imm_xsd_file;
typedef std::set<std::string> AttrFlagSet;
AttrFlagSet attrFlagSet;

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
static int charsToValueHelper(SaImmAttrValueT*,
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
	NULL,				   //   internalSubsetSAXFunc internalSubset;
	NULL,				   //   isStandaloneSAXFunc isStandalone,
	NULL,				   //   hasInternalSubsetSAXFunc hasInternalSubset,
	NULL,				   //   hasExternalSubsetSAXFunc hasExternalSubset,
	NULL,				   //   resolveEntitySAXFunc resolveEntity,
	getEntityHandler,
	NULL,				   //   entityDeclSAXFunc entityDecl,
	NULL,				   //   notationDeclSAXFunc notationDecl,
	attributeDeclHandler,
	NULL,				   //   elementDeclSAXFunc elementDecl,
	NULL,				   //   unparsedEntityDeclSAXFunc unparsedEntityDecl,
	NULL,				   //   setDocumentLocatorSAXFunc setDocumentLocator,
	startDocumentHandler,
	endDocumentHandler,
	startElementHandler,
	endElementHandler,
	NULL,				   //   referenceSAXFunc reference,
	charactersHandler,
	NULL,				   //   ignorableWhitespaceSAXFunc ignorableWhitespace,
	NULL,				   //   processingInstructionSAXFunc processingInstruction,
	NULL,				   //   commentSAXFunc comment,
	warningHandler,
	errorHandler,
	NULL				   //   fatalErrorSAXFunc fatalError;
};


/* Function bodies */

static void free_attr_value(SaImmValueTypeT attrValueType, SaImmAttrValueT attrValue) {
	if(attrValue) {
		if(attrValueType == SA_IMM_ATTR_SASTRINGT)
			free(*((SaStringT *)attrValue));
		else if(attrValueType == SA_IMM_ATTR_SAANYT)
			free(((SaAnyT*)attrValue)->bufferAddr);
		free(attrValue);
	}
}

static void free_attr_values(SaImmAttrValuesT_2 *attrValues) {
	if(attrValues) {
		SaUint32T i;
		for(i=0; i<attrValues->attrValuesNumber; i++)
			free_attr_value(attrValues->attrValueType, attrValues->attrValues[i]);

		free(attrValues->attrName);
		free(attrValues->attrValues);
		free(attrValues);
	}
}
/*
typedef struct ParserStateStruct1 {

	char*                className;

	char*                attrName;
	char*                attrDefaultValueBuffer;

	std::list<SaImmAttrDefinitionT_2> attrDefinitions;
	std::map<std::string, std::map<std::string, SaImmValueTypeT> > classAttrTypeMap;
	std::set<std::string> adminOwnerSetSet;

	char*                objectClass;
	char*                objectName;

	std::list<SaImmAttrValuesT_2> attrValues;
	std::list<char*>            attrValueBuffers;

	std::map<std::string, SaImmAttrValuesT_2>    classRDNMap;

} ;
*/
static void free_parserState(ParserState *state) {
	free(state->className);
	state->className = NULL;

	free(state->attrName);
	state->attrName = NULL;

	free(state->objectClass);
	state->objectClass = NULL;

	free(state->objectName);
	state->objectName = NULL;

	state->classAttrTypeMap.clear();
	state->adminOwnerSetSet.clear();

	free(state->attrDefaultValueBuffer);
	state->attrDefaultValueBuffer = NULL;

	{
		std::list<SaImmAttrDefinitionT_2>::iterator it;
		it = state->attrDefinitions.begin();

		while (it != state->attrDefinitions.end()) {
			free(it->attrName);
			it->attrName = NULL;
			if(it->attrDefaultValue) {
				if(it->attrValueType == SA_IMM_ATTR_SASTRINGT && *(void **)(it->attrDefaultValue))
					free(*(void **)(it->attrDefaultValue));
				else if(it->attrValueType == SA_IMM_ATTR_SAANYT && (SaAnyT *)(it->attrDefaultValue))
					free(((SaAnyT *)(it->attrDefaultValue))->bufferAddr);
				free(it->attrDefaultValue);
				it->attrDefaultValue = NULL;
			}
			it++;
		}

		/* Free the attrDefinition array and empty the list */
		state->attrDefinitions.clear();
	}

	{
		std::list<SaImmAttrValuesT_2>::iterator it;
		int i;

		for(it = state->attrValues.begin(); it != state->attrValues.end(); it++) {
			free(it->attrName);
			for(i=0; it->attrValues[i]; i++) {
				if(it->attrValueType == SA_IMM_ATTR_SASTRINGT
						|| it->attrValueType == SA_IMM_ATTR_SAANYT) {
					free(*(void **)(it->attrValues[i]));
				}
				free(it->attrValues[i]);
			}
			free(it->attrValues);
		}
		state->attrValues.clear();
	}

	{
		std::list<char*>::iterator it;
		for(it = state->attrValueBuffers.begin(); it != state->attrValueBuffers.end(); it++)
			free(*it);
		state->attrValueBuffers.clear();
	}

	{
		std::map<std::string, SaImmAttrValuesT_2>::iterator it;
		for(it = state->classRDNMap.begin(); it != state->classRDNMap.end(); it++)
			free(it->second.attrName);
		state->classRDNMap.clear();
	}
}

static void stopParser(ParserState *state) {
	xmlStopParser(state->ctxt);
	free_parserState(state);
}

static int base64_decode(char *in, char *out) {
	char ch1, ch2, ch3, ch4;
	int len = 0;

	/* skip spaces */
	while(*in == 10 || *in == 13 || *in == 32)
		in++;

	while(*in && in[3] != '=') {
		if(*in < '+' || *in > 'z' || in[1] < '+' || in[1] > 'z'
				|| in[2] < '+' || in[2] > 'z' || in[3] < '+' || in[3] > 'z')
			return -1;

		ch1 = base64_dec_table[*in - '+'];
		ch2 = base64_dec_table[in[1] - '+'];
		ch3 = base64_dec_table[in[2] - '+'];
		ch4 = base64_dec_table[in[3] - '+'];

		if(((ch1 | ch2 | ch3 | ch4) & 0xc0) > 0)
			return -1;

		*out = (ch1 << 2) | (ch2 >> 4);
		out[1] = (ch2 << 4) | (ch3 >> 2);
		out[2] = (ch3 << 6) | ch4;

		in += 4;
		out += 3;
		len += 3;

		while(*in == 10 || *in == 13 || *in == 32)
			in++;
	}

	if(*in && in[3] == '=') {
		if(*in < '+' || *in > 'z' || in[1] < '+' || in[1] > 'z')
			return -1;

		ch1 = base64_dec_table[*in - '+'];
		ch2 = base64_dec_table[in[1] - '+'];

		if(in[2] == '=') {
			if(((ch1 | ch2) & 0xc0) > 0)
				return -1;

			*out = (ch1 << 2) | (ch2 >> 4);

			len++;
		} else {
			if(in[2] < '+' || in[2] > 'z')
				return -1;

			ch3 = base64_dec_table[in[2] - '+'];

			if(((ch1 | ch2 | ch3) & 0xc0) > 0)
				return -1;

			*out = (ch1 << 2) | (ch2 >> 4);
			out[1] = (ch2 << 4) | (ch3 >> 2);

			len += 2;
		}

		in += 4;

		/* Skip spaces from the end of the string */
		while(*in == 10 || *in == 13 || *in == 32)
			in++;
	}

	return (*in) ? -1 : len;
}

static void log_stderr_int(int priority, const char *format, va_list args)
{
	char log_string[1024];

	if (priority == LOG_INFO && !imm_import_verbose) {
		// only printout INFO statements if -v (verbose) flag is set
		return;
	}

	/* Add line feed if not there already */
	if (format[strlen(format) - 1] != '\n') {
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

	if (len < 1) {
		// this is a root object, no need to set AdminOwner
		LOG_IN("  This is a root object, no need to call saImmOmAdminOwnerSet");
		// state->adminOwnerSetSet.insert(state->objectName);
		return;
	}

	std::set<std::string>::iterator set_it = state->adminOwnerSetSet.find(tmpStr);
	if (set_it != state->adminOwnerSetSet.end()) {
		LOG_IN("  Already called saImmOmAdminOwnerSet on parent (or parent of parent) '%s'", tmpStr);
		return;
	}

	// if parent does not exist in map then do actual call
	SaNameT* objectNames[2];
	objectNames[0] = parentOfObject;
	objectNames[1] = NULL;

	if(!state->validation) {
		LOG_IN("  Calling saImmOmAdminOwnerSet on parent '%s'", tmpStr);
		int errorCode = immutil_saImmOmAdminOwnerSet(state->ownerHandle,
											 (const SaNameT**) objectNames,
											 SA_IMM_ONE);

		if (SA_AIS_OK != errorCode) {
			LOG_ER("Failed to call saImmOmAdminOwnerSet on parent: '%s' , rc =  %d", tmpStr, errorCode);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
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

	SaAisErrorT err = immutil_saImmOmClassDescriptionGet_2(
												  state->immHandle, className, &classCategory, &attrDefinitions);
	if (err != SA_AIS_OK) {
		LOG_ER("Failed to fetch class description for %s loaded in IMM, "
			   "error code:%u", className, err);
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	LOG_IN("Read class description from IMM: %s",
		   className);

	for (SaImmAttrDefinitionT_2** ap = attrDefinitions; *ap != NULL; ap++) {
		SaImmAttrDefinitionT_2* attr = *ap;

		std::string classString = std::string(className);
		std::string attrNameString = std::string(attr->attrName);
		state->classAttrTypeMap[classString][attrNameString] = attr->attrValueType;

		if (attr->attrFlags & SA_IMM_ATTR_RDN) {

			SaImmAttrValuesT_2 values;
			memset(&values, 0, sizeof values);

			size_t len = strlen(attr->attrName);
			values.attrName = (char*)malloc(len + 1);
			if (values.attrName == NULL) {
				LOG_ER("Failed to malloc values.attrName");
				stopParser(state);
				state->parsingStatus = 1;
				break;
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
 * Return attribute value for named attribute
 * @param attrDefinitions
 * @param attrName
 * 
 * @return SaImmAttrValuesT_2*
 */
static SaImmAttrValuesT_2 *attrvalue_get(SaImmAttrValuesT_2 **attrValues,
										 const char *attrName)
{
	int i;

	for (i = 0; attrValues[i] != NULL; i++)
		if (strcmp(attrValues[i]->attrName, attrName) == 0)
		    return attrValues[i];

	return NULL;
}

/**
 * Creates an Imm Object through the ImmOm interface
 */
static void createImmObject(ParserState* state)
{
	SaImmClassNameT className;
	SaNameT parentName;
	SaImmAttrValuesT_2** attrValues;
	SaAisErrorT errorCode = SA_AIS_OK;
	int i = 0;
	size_t DNlen;
	SaNameT objectName;
	std::list<SaImmAttrValuesT_2>::iterator it;

	TRACE_8("CREATE IMM OBJECT %s, %s",
			state->objectClass,
			state->objectName);

	LOG_IN("CREATE IMM OBJECT %s, %s",
		   state->objectClass,
		   state->objectName);

	/* Set the class name */
	className = state->objectClass;

	objectName.length = snprintf((char*) objectName.value,
								 sizeof(objectName.value),
								 "%s", state->objectName);

	/* Set the parent name */
	parentName.length = 0;
	if (state->objectName != NULL) {
		char* parent;

		/* ',' is the delimeter */
		/* but '\' is the escape character, used for association objects */
		parent = state->objectName;
		do {
			parent = strchr(parent, ',');
			TRACE_8("PARENT: %s", parent);
		} while (parent && (*((++parent)-2)) == '\\');

		if (parent && strlen(parent) <= 1) {
			parent = NULL;
		}

		if (parent != NULL) {
			parentName.length = (SaUint16T)strlen(parent);
			strncpy((char*)parentName.value, parent, parentName.length);
			parentName.value[parentName.length] = 0;
		}
	} else {
		LOG_ER("Empty DN for object");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	// checks and sets AdminOwer if needed
	// Note! it must be called before state->objectName is "null truncated"!!
	setAdminOwnerHelper(state, &parentName);

	if(state->ctxt->instate == XML_PARSER_EOF)
		return;

#ifdef TRACE_8
	/* Get the length of the DN and truncate state->objectName */
	if (parentName.length > 0) {
		DNlen = strlen(state->objectName) - (parentName.length + 1);
	} else {
		DNlen = strlen(state->objectName);
	}

	state->objectName[DNlen] = '\0';
	TRACE_8("OBJECT NAME: %s", state->objectName);
#endif

	if(!state->validation) {
		/* Set the attribute values array, add space for the rdn attribute
		 * and a NULL terminator */

		/* Freed at the bottom of the function */
		attrValues = (SaImmAttrValuesT_2**) malloc((state->attrValues.size() + 2) *
												   sizeof(SaImmAttrValuesT_2*));
		if (attrValues == NULL) {
			LOG_ER("Failed to malloc attrValues");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		/* Add the NULL termination */
		attrValues[state->attrValues.size() + 1] = NULL; /* Adjust for RDN */

		it = state->attrValues.begin();

		i = 0;
		while (it != state->attrValues.end()) {
			attrValues[i] = &(*it);

			i++;
			it++;
		}

		/* Do the actual creation */
		attrValues[i] = (SaImmAttrValuesT_2*)calloc(1, sizeof(SaImmAttrValuesT_2));
		getDNForClass(state, className, attrValues[i]);

		if(state->ctxt->instate != XML_PARSER_EOF)
			errorCode = immutil_saImmOmCcbObjectCreate_2(state->ccbHandle,
														 className,
														 &parentName,
														 (const SaImmAttrValuesT_2**)
														 attrValues);
	} else
		attrValues = NULL;

	if (SA_AIS_OK != errorCode) {
		if ((errorCode == SA_AIS_ERR_NOT_EXIST) && imm_import_ccb_safe) {
			fprintf(stderr,
					"NOTEXIST(%u): missing parent, class, implementer "
					"or invalid attribute names (see also immcfg -h under '--unsafe')\n",
					SA_AIS_ERR_NOT_EXIST);
			stopParser(state);
			state->parsingStatus = 1;
			goto done;
		}

		if (SA_AIS_ERR_EXIST == errorCode) {
			SaImmAccessorHandleT accessorHandle;
			SaImmAttrValuesT_2 **existing_attributes;
			SaImmAttrValuesT_2 *attr;
			const char *existing_className;

			LOG_IN("OBJECT %s already exist, verifying...", state->objectName);

			errorCode = immutil_saImmOmAccessorInitialize(state->immHandle, &accessorHandle);
			if (SA_AIS_OK != errorCode) {
				fprintf(stderr, "FAILED: saImmOmAccessorInitialize failed: %u\n", errorCode);
				stopParser(state);
				state->parsingStatus = 1;
				goto done;
			}

			errorCode = immutil_saImmOmAccessorGet_2(accessorHandle, &objectName,
													 NULL, // get all attributes
													 &existing_attributes);
			if (SA_AIS_OK != errorCode) {
				fprintf(stderr, "FAILED: saImmOmAccessorGet_2 failed: %u\n", errorCode);
				stopParser(state);
				state->parsingStatus = 1;
				goto done;
			}

			attr = attrvalue_get(existing_attributes, "SaImmAttrClassName");
			if (attr == NULL) {
				fprintf(stderr, "FAILED: Attribute 'SaImmAttrClassName' missing\n");
				stopParser(state);
				state->parsingStatus = 1;
			} else {
				if(attr->attrValueType == SA_IMM_ATTR_SASTRINGT) {
					existing_className = *((char**) attr->attrValues[0]);

					if (strcmp(existing_className, className) != 0) {
						fprintf(stderr, "FAILED: '%s' of class '%s' "
								"is not of the same class as an existing object of class '%s'\n",
								state->objectName, className, existing_className);
						stopParser(state);
						state->parsingStatus = 1;
					}
				} else {
					LOG_ER("Attribute value type is not SA_IMM_ATTR_SASTRINGT");
					stopParser(state);
					state->parsingStatus = 1;
				}
			}

			LOG_IN("OBJECT '%s' OK", state->objectName);

			errorCode = immutil_saImmOmAccessorFinalize(accessorHandle);
			if (SA_AIS_OK != errorCode) {
				fprintf(stderr, "FAILED: saImmOmAccessorFinalize failed: %u\n", errorCode);
				stopParser(state);
				state->parsingStatus = 1;
			}

		} else {
			fprintf(stderr, "FAILED to create object of class '%s', rc = %d\n",
					className, errorCode);
			stopParser(state);
			state->parsingStatus = 1;
		}
	} else {
		state->adminOwnerSetSet.insert(state->objectName);
	}

done:
	TRACE_8("CREATE DONE");

	/* Free used parameters */
	free(state->objectClass);
	state->objectClass = NULL;
	free(state->objectName);
	state->objectName = NULL;

	/* Free the DN attrName later since it's re-used */
	/*free(attrValues[i]->attrValues);*/
	if(attrValues) {
		free_attr_values(attrValues[i]);
		free(attrValues);
	}


	for (it = state->attrValues.begin();
		it != state->attrValues.end();
		it++) {
		free(it->attrName);
		for(i=0; it->attrValues[i]; i++) {
			if(it->attrValueType == SA_IMM_ATTR_SASTRINGT
					|| it->attrValueType == SA_IMM_ATTR_SAANYT) {
				free(*(void **)(it->attrValues[i]));
			}
			free(it->attrValues[i]);
		}
		free(it->attrValues);
	}
	state->attrValues.clear();
}

/**
 * Determines number of SaImmAttrDefinitionT_2 in the NULL
 * terminated array attrDefinitions.
 * Does not count IMM added attributes (SaImmAttr*).
 * @param attrDefinitions
 * 
 * @return unsigned int
 */
static unsigned int attrdef_array_size(SaImmAttrDefinitionT_2 **attrDefinitions)
{
	int i, imm_added = 0;

	for (i = 0; attrDefinitions[i] != NULL; i++) {
		if (strncmp(attrDefinitions[i]->attrName, "SaImmAttr", 9) == 0)
			imm_added++;
	}

	return (i - imm_added);
}

/**
 * Return attribute definition for named attribute
 * @param attrDefinitions
 * @param attrName
 * 
 * @return SaImmAttrDefinitionT_2*
 */
static SaImmAttrDefinitionT_2 *attrdef_get(SaImmAttrDefinitionT_2 **attrDefinitions,
										   SaImmAttrNameT attrName)
{
	int i;

	for (i = 0; attrDefinitions[i] != NULL; i++)
		if (strcmp(attrDefinitions[i]->attrName, attrName) == 0)
		    return attrDefinitions[i];

	return NULL;
}

static bool attrvalue_is_equal(SaImmValueTypeT valueType, SaImmAttrValueT val1, SaImmAttrValueT val2)
{
	switch (valueType) {
	case SA_IMM_ATTR_SAINT32T:
		return *((SaInt32T*) val1) == *((SaInt32T*) val2);
		break;
	case SA_IMM_ATTR_SAUINT32T:
		return *((SaUint32T*) val1) == *((SaUint32T*) val2);
		break;
	case SA_IMM_ATTR_SAINT64T:
		return *((SaInt64T*) val1) == *((SaInt64T*) val2);
		break;
	case SA_IMM_ATTR_SAUINT64T:
		return *((SaUint64T*) val1) == *((SaUint64T*) val2);
		break;
	case SA_IMM_ATTR_SATIMET:
		return *((SaTimeT*) val1) == *((SaTimeT*) val2);
		break;
	case SA_IMM_ATTR_SANAMET:
		return (memcmp(val1, val2, sizeof(SaNameT)) == 0);
		break;
	case SA_IMM_ATTR_SAFLOATT:
		return *((SaFloatT*) val1) == *((SaFloatT*) val2);
		break;
	case SA_IMM_ATTR_SADOUBLET:
		return *((SaDoubleT*) val1) == *((SaDoubleT*) val2);
		break;
	case SA_IMM_ATTR_SASTRINGT: {
			char *s1 = *((char**) val1);
			char *s2 = *((char**) val2);
			return (strcmp(s1, s2) == 0);
			break;
		}
	case SA_IMM_ATTR_SAANYT: {
			SaAnyT *at1 = (SaAnyT *) val1;
			SaAnyT *at2 = (SaAnyT *) val2;

			if (at1->bufferSize != at2->bufferSize)
				return false;

			return (memcmp(at1->bufferAddr, at2->bufferAddr, at1->bufferSize) == 0);
			break;
		}
	default:
		fprintf(stderr, "FAILED: unknown valuetype %u\n", valueType);
		return false; // keep compiler happy
		break; // keep compiler happy
	}
}

/**
 * Returns true if attribute definitions are equal
 * @param attrDef1
 * @param attrDef2
 * 
 * @return bool
 */
static bool attrdefs_are_equal(SaImmAttrDefinitionT_2 *attrDef1,
							   SaImmAttrDefinitionT_2 *attrDef2)
{
	if(strcmp(attrDef1->attrName, attrDef2->attrName)) {
		fprintf(stderr, "FAILED: attribute names are not same: '%s', '%s'\n", attrDef1->attrName, attrDef2->attrName);
		return false;
	}

	if (attrDef1->attrFlags != attrDef2->attrFlags) {
		fprintf(stderr, "FAILED: attrFlags mismatch for attribute '%s'\n", attrDef1->attrName);
		return false;
	}

	if (((attrDef1->attrDefaultValue == NULL) && (attrDef2->attrDefaultValue != NULL)) ||
		((attrDef1->attrDefaultValue != NULL) && (attrDef2->attrDefaultValue == NULL))) {
		fprintf(stderr, "FAILED: attrDefaultValue mismatch for attribute '%s'\n", attrDef1->attrName);
		return false;
	}

	if (attrDef1->attrValueType != attrDef2->attrValueType) {
		fprintf(stderr, "FAILED: attrValueType mismatch for attribute '%s'\n", attrDef1->attrName);
		return false;
	}

	if ((attrDef1->attrDefaultValue != NULL) &&
		!attrvalue_is_equal(attrDef1->attrValueType, attrDef1->attrDefaultValue, attrDef2->attrDefaultValue)) {
		fprintf(stderr, "FAILED: attrDefaultValue mismatch for attribute '%s'\n", attrDef1->attrName);
		return false;
	}

	return true;
}

/**
 * Creates an ImmClass through the ImmOm interface
 */
static void createImmClass(ParserState* state)
{
	SaImmClassNameT          className;
	SaImmClassCategoryT      classCategory;
	SaImmAttrDefinitionT_2** new_attrDefinitions = NULL;
	SaAisErrorT              errorCode;
	int i;
	std::list<SaImmAttrDefinitionT_2>::iterator it;

	TRACE_8("CREATING IMM CLASS %s", state->className);
	LOG_IN("CREATING IMM CLASS %s",
		   state->className);

	if(!state->validation) {
		/* Set the name */
		className = state->className;

		/* Set the category */
		if (state->classCategorySet) {
			classCategory = state->classCategory;
		} else {
			LOG_ER("NO CLASS CATEGORY");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		/* Set the attrDefinition array */
		new_attrDefinitions = (SaImmAttrDefinitionT_2**)
						 calloc((state->attrDefinitions.size() + 1),
								sizeof(SaImmAttrDefinitionT_2*));
		if (new_attrDefinitions == NULL) {
			LOG_ER("Failed to allocate attrDefinition");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		new_attrDefinitions[state->attrDefinitions.size()] = NULL;

		it = state->attrDefinitions.begin();

		i = 0;
		while (it != state->attrDefinitions.end()) {
			new_attrDefinitions[i] = &(*it);

			i++;
			it++;
		}

		errorCode = immutil_saImmOmClassCreate_2(state->immHandle,
												 className,
												 classCategory,
												 (const SaImmAttrDefinitionT_2**)
												 new_attrDefinitions);

		if (SA_AIS_OK != errorCode) {
			if (SA_AIS_ERR_EXIST == errorCode) {

				// Class with same name already exist
				// If class descriptions are equal ignore the error
				LOG_IN("Class %s already exist, verifying...", state->className);

				SaImmClassCategoryT existing_classCategory;
				SaImmAttrDefinitionT_2 **existing_attrDefinitions;
				SaImmAttrDefinitionT_2 *new_attrDef, *existing_attrDef;

				errorCode = immutil_saImmOmClassDescriptionGet_2(state->immHandle,
																 className,
																 &existing_classCategory,
																 &existing_attrDefinitions);
				if (SA_AIS_OK != errorCode) {
					fprintf(stderr, "FAILED to get IMM class description for '%s', rc = %d\n",
							className, errorCode);
					stopParser(state);
					state->parsingStatus = 1;
				} else {
					if (existing_classCategory != classCategory) {
						fprintf(stderr, "FAILED: Class category differ for '%s'\n", className);
						stopParser(state);
						state->parsingStatus = 1;
					} else if (attrdef_array_size(existing_attrDefinitions) !=
								attrdef_array_size(new_attrDefinitions)) {
						// If there are more (or less) attributes in the new class description
						// than in the existing one, this check will catch it.
						fprintf(stderr, "FAILED: Number of attribute definitions differ for '%s'\n", className);
						stopParser(state);
						state->parsingStatus = 1;
					} else {
						for (i=0; existing_attrDefinitions[i] != NULL; i++) {
							existing_attrDef = existing_attrDefinitions[i];

							// Skip IMM added attributes
							if (strncmp(existing_attrDef->attrName, "SaImmAttr", 9) == 0)
								continue;

							new_attrDef = attrdef_get(new_attrDefinitions, existing_attrDef->attrName);
							if (new_attrDef == NULL) {
								fprintf(stderr, "FAILED: Attribute '%s' missing in loaded class '%s'\n",
										existing_attrDef->attrName, className);
								stopParser(state);
								state->parsingStatus = 1;
								break;
							}

							if (!attrdefs_are_equal(existing_attrDef, new_attrDef)) {
								stopParser(state);
								state->parsingStatus = 1;
								break;
							}
						}
					}

					if(state->ctxt->instate != XML_PARSER_EOF && !existing_attrDefinitions[i])
						LOG_IN("Class '%s' OK", state->className);

					(void) saImmOmClassDescriptionMemoryFree_2(state->immHandle,
															   existing_attrDefinitions);
				}
			} else {
				fprintf(stderr, "FAILED to create class '%s', rc = %d\n", className, errorCode);
				stopParser(state);
				state->parsingStatus = 1;
			}
		}

		TRACE_8("CREATED IMM CLASS %s", className);

		free(new_attrDefinitions);
	}

	/* Free all each attrDefinition */
	it = state->attrDefinitions.begin();

	while (it != state->attrDefinitions.end()) {
		free(it->attrName);
		it->attrName = NULL;
		if(it->attrDefaultValue) {
			if(it->attrValueType == SA_IMM_ATTR_SASTRINGT && *(void **)(it->attrDefaultValue))
				free(*(void **)(it->attrDefaultValue));
			else if(it->attrValueType == SA_IMM_ATTR_SAANYT && (SaAnyT *)(it->attrDefaultValue))
				free(((SaAnyT *)(it->attrDefaultValue))->bufferAddr);
			free(it->attrDefaultValue);
			it->attrDefaultValue = NULL;
		}
		it++;
	}

	/* Free the attrDefinition array and empty the list */
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
		state->classRDNMap.end()) {
		/* Attempt to read the class from the IMM */
		getClassFromImm(state, className);
		if(state->ctxt->instate == XML_PARSER_EOF)
			return;
		if (state->classRDNMap.find(classNameString) ==
			state->classRDNMap.end()) {
			LOG_ER("Cannot find CLASS %s loaded in IMM", className);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
	}

	*values = state->classRDNMap[classNameString];

	/* Make a copy of an attribute name. Coping a pointer is not enough */
	values->attrName = strdup(values->attrName);

	values->attrValues = (SaImmAttrValueT*)calloc(1, sizeof(SaImmAttrValueT));

	values->attrValuesNumber = 1;

	if(charsToValueHelper(values->attrValues,
						values->attrValueType,
						state->objectName)) {
		free(values->attrValues);
		values->attrValues = NULL;

		LOG_ER("Failed to parse DN value of class %s", className);
		stopParser(state);
		state->parsingStatus = 1;
	}
}

static void errorHandler(void* userData,
						 const char* format,
						 ...)
{
	ParserState* state = (ParserState *)userData;
    va_list ap;
    char *errMsg=NULL;
    va_start(ap, format);
    if(*(++format) == 's') {
        errMsg=va_arg(ap, char *);
        LOG_ER("Error occured during XML parsing libxml2: %s", errMsg);
    } else {
        LOG_ER("Error (unknown) occured during XML parsing");
    }
    va_end(ap);

    stopParser(state);
    state->parsingStatus = 1;
}

static void warningHandler(void* userData,
						   const char* format,
						   ...)
{
    va_list ap;
    char *errMsg=NULL;
    va_start(ap, format);
    if(*(++format) == 's') {
        errMsg=va_arg(ap, char *);
        LOG_WA("Warning occured during XML parsing libxml2: %s", errMsg);
    } else {
        LOG_WA("Warning (unknonw) occured during XML parsing");
    }
    va_end(ap);
}

static const xmlChar *getAttributeValue(const xmlChar **attrs, const xmlChar *attr) {
	if(!attrs)
		return NULL;

	while(*attrs) {
		if(!strcmp((char *)*attrs, (char *)attr))
			return (xmlChar *)*(++attrs);

		attrs++;
		attrs++;
	}

	return NULL;
}

static inline bool isBase64Encoded(const xmlChar **attrs) {
	char *encoding;
	bool isB64 = false;

	if((encoding = (char *)getAttributeValue(attrs, (xmlChar *)"xsi:type")))
		isB64 = !strcmp(encoding, "xs:base64Binary");

	/* This verification has been left for backward compatibility */
	if(!isB64 && (encoding = (char *)getAttributeValue(attrs, (xmlChar *)"encoding")))
		isB64 = !strcmp(encoding, "base64");

	return isB64;
}

static inline char *getAttrValue(xmlAttributePtr attr) {
    if(!attr || !attr->children) {
        return NULL;
    }

    return (char *)attr->children->content;
}

static bool loadXsd(const xmlChar** attrs) {
    if(!imm_xsd_file) {
        return true;
    }

    // Check if schema path exist
    struct stat st;
    if(stat(imm_xsd_file, &st)) {
        if(errno == ENOENT) {
            LOG_ER("%s does not exist", imm_xsd_file);
        } else {
            LOG_ER("stat of %s return error: %d", imm_xsd_file, errno);
        }

        return false;
    }
    // It should be a file or a directory
    if(!S_ISREG(st.st_mode) && !S_ISDIR(st.st_mode)) {
        LOG_ER("%s is not a file or directory", imm_xsd_file);
        return false;
    }

    std::string xsdFile = imm_xsd_file;
    if(S_ISDIR(st.st_mode)) {
        // Schema path is a directory, so we shall add a schema file from XML file
        if(xsdFile.at(xsdFile.size() - 1) != '/') {
            xsdFile.append("/");
        }
        char *xsd = (char *)getAttributeValue(attrs, (xmlChar *)"noNamespaceSchemaLocation");
        if(!xsd) {
            // try with a namespace
            xsd = (char *)getAttributeValue(attrs, (xmlChar *)"xsi:noNamespaceSchemaLocation");
            if(!xsd) {
                LOG_ER("Schema is not defined in XML file");
                return false;
            }
        }
        xsdFile.append(xsd);

        // Check if schema file exists and that it's a file
        if(stat(xsdFile.c_str(), &st)) {
            if(errno == ENOENT) {
                LOG_ER("XSD file %s does not exist", imm_xsd_file);
            } else {
                LOG_ER("Stat of XSD file %s return error: %d", imm_xsd_file, errno);
            }

            return false;
        }
        if(!S_ISREG(st.st_mode)) {
            LOG_ER("Schema %s is not a file", xsdFile.c_str());
            return false;
        }
    }

    xmlNodePtr xsdDocRoot;
    xmlDocPtr xsdDoc = xmlParseFile(xsdFile.c_str());
    if(!xsdDoc) {
        return false;
    }

    bool rc = true;
    xmlXPathContextPtr ctx = xmlXPathNewContext(xsdDoc);
    if(!ctx) {
        rc = false;
        goto freedoc;
    }

    //    Add namespace of the first element
    xsdDocRoot = xmlDocGetRootElement(xsdDoc);
    if(xsdDocRoot->ns) {
        ctx->namespaces = (xmlNsPtr *)malloc(sizeof(xmlNsPtr));
        ctx->namespaces[0] = xsdDocRoot->ns;
        ctx->nsNr = 1;
    }

    xmlXPathObjectPtr xpathObj;
    xpathObj = xmlXPathEval((const xmlChar*)"/xs:schema/xs:simpleType[@name=\"attr-flags\"]/xs:restriction/xs:enumeration", ctx);
    if(!xpathObj || !xpathObj->nodesetval) {
        rc = false;
        goto freectx;
    }

    xmlElementPtr element;
    xmlAttributePtr attr;
    char *value;
    int size;

    size = xpathObj->nodesetval->nodeNr;
    for(int i=0; i<size; i++) {
        value = NULL;
        element = (xmlElementPtr)xpathObj->nodesetval->nodeTab[i];
        attr = element->attributes;
        while(attr) {
            if(!strcmp((char *)attr->name, "value")) {
                value = getAttrValue(attr);
            }

            if(value) {
                break;
            }

            attr = (xmlAttributePtr)attr->next;
        }

        if(value) {
            if(strcmp(value, "SA_RUNTIME") && strcmp(value, "SA_CONFIG") &&
                    strcmp(value, "SA_MULTI_VALUE") && strcmp(value, "SA_WRITABLE") &&
                    strcmp(value, "SA_INITIALIZED") && strcmp(value, "SA_PERSISTENT") &&
                    strcmp(value, "SA_CACHED") && strcmp(value, "SA_NOTIFY") &&
                    strcmp(value, "SA_NO_DUPLICATES") && strcmp(value, "SA_NO_DANGLING")) {
                attrFlagSet.insert(value);
            }
        }
    }

    isXsdLoaded = true;

    xmlXPathFreeObject(xpathObj);
freectx:
    if(ctx->nsNr) {
        free(ctx->namespaces);
    }
    xmlXPathFreeContext(ctx);
freedoc:
    xmlFreeDoc(xsdDoc);

    return rc;
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

	if (state->depth >= MAX_DEPTH) {
		LOG_ER( "The document is too deeply nested");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* <class ...> */
	if (strcmp((const char*)name, "class") == 0) {
		char* nameAttr;

		if (state->doneParsingClasses) {
			LOG_ER("CLASS TAG AT INVALID PLACE IN XML");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		state->state[state->depth] = CLASS;
		nameAttr = getAttributeValue("name", attrs);

		if (nameAttr != NULL) {
			size_t len;

			len = strlen(nameAttr);

			state->className = (char*)malloc(len + 1);
			strcpy(state->className, nameAttr);
			state->className[len] = '\0';

			TRACE_8("CLASS NAME: %s", state->className);
		} else {
			LOG_ER( "NAME ATTRIBUTE IS NULL");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		/* <object ...> */
	} else if (strcmp((const char*)name, "object") == 0) {
		char* classAttr;

		state->attrFlags = 0;

		state->state[state->depth] = OBJECT;

		/* Get the class attribute */
		classAttr = getAttributeValue("class", attrs);

		if (classAttr != NULL) {
			size_t len;

			len = strlen(classAttr);

			state->objectClass = (char*)malloc(len + 1);
			strncpy(state->objectClass, classAttr, len);
			state->objectClass[len] = '\0';

			TRACE_8("OBJECT CLASS NAME: %s",
					state->objectClass);
		} else {
			LOG_ER("OBJECT %s HAS NO CLASS ATTRIBUTE", state->objectName);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		/* <dn> */
	} else if (strcmp((const char*)name, "dn") == 0) {
		state->state[state->depth] = DN;
		/* <attr> */
	} else if (strcmp((const char*)name, "attr") == 0) {
		state->state[state->depth] = ATTRIBUTE;

		state->attrFlags = 0x0;
		state->attrName  = NULL;
		state->attrDefaultValueBuffer = NULL;

		state->attrValueTypeSet    = 0;
		state->attrNtfIdSet        = 0;
		state->attrDefaultValueSet = 0;
		/* <name> */
	} else if (strcmp((const char*)name, "name") == 0) {
		state->state[state->depth] = NAME;
		/* <value> */
	} else if (strcmp((const char*)name, "value") == 0) {
		state->state[state->depth] = VALUE;
		state->valueContinue = 0;
		state->isBase64Encoded = isBase64Encoded(attrs);
		/* <category> */
	} else if (strcmp((const char*)name, "category") == 0) {
		state->state[state->depth] = CATEGORY;
		/* <rdn> */
	} else if (strcmp((const char*)name, "rdn") == 0) {
		state->attrFlags        = SA_IMM_ATTR_RDN;

		state->attrName = NULL;

		state->attrDefaultValueBuffer = NULL;

		state->attrValueTypeSet    = 0;
		state->attrDefaultValueSet = 0;
		state->state[state->depth] = RDN;
		/* <default-value> */
	} else if (strcmp((const char*)name, "default-value") == 0) {
		state->state[state->depth] = DEFAULT_VALUE;
		state->isBase64Encoded = isBase64Encoded(attrs);
		/* <type> */
	} else if (strcmp((const char*)name, "type") == 0) {
		state->state[state->depth] = TYPE;
		/* <flag> */
	} else if (strcmp((const char*)name, "flag") == 0) {
		state->state[state->depth] = FLAG;
		/* <ntfid> */
	} else if (strcmp((const char*)name, "ntfid") == 0) {
		state->state[state->depth] = NTFID;
		/* <imm:IMM-contents> */
	} else if (strcmp((const char*)name, "imm:IMM-contents") == 0) {
		state->state[state->depth] = IMM_CONTENTS;
		if(imm_xsd_file) {
			if(!loadXsd(attrs)) {
				LOG_ER("Failed to load XML schema", name);
				stopParser(state);
				state->parsingStatus = 1;
			}
		}
	} else {
		LOG_ER("UNKNOWN TAG! (%s)", name);
		stopParser(state);
		state->parsingStatus = 1;
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
	if (strcmp((const char*)name, "value") == 0) {
		if (state->attrValueBuffers.size() == 0) {
			char* str = (char*)malloc(1);

			str[0] = '\0';
			state->attrValueBuffers.push_front(str);
		} else if(state->isBase64Encoded) {
        	char *value = state->attrValueBuffers.front();
        	int len = strlen(value);

        	/* count the length of the decoded string */
        	int newlen = (len / 4) * 3
        			- (value[len - 1] == '=' ? 1 : 0)
        			- (value[len - 2] == '=' ? 1 : 0);
        	char *newvalue = (char *)malloc(newlen + 1);

        	newlen = base64_decode(value, newvalue);
        	if(newlen == -1) {
        		free(newvalue);
        		LOG_ER("Failed to decode base64 value in attribute %s (value)", state->attrName);
        		stopParser(state);
        		state->parsingStatus = 1;
        		return;
        	}
        	newvalue[newlen] = 0;

        	state->attrValueBuffers.pop_front();
        	state->attrValueBuffers.push_front(newvalue);
        	free(value);
		}

		state->valueContinue = 0; /* Actually redundant. */
		state->isBase64Encoded = 0;

		/* </default-value> */
	} else if (strcmp((const char*)name, "default-value") == 0) {
		if (state->attrDefaultValueBuffer == NULL ||
			!state->attrDefaultValueSet) {
			state->attrDefaultValueBuffer = (char*)malloc(1);

			state->attrDefaultValueBuffer[0] = '\0';
			state->attrDefaultValueSet = 1;
			TRACE_8("EMPTY DEFAULT-VALUE TAG");
			TRACE_8("Attribute: %s", state->attrName);
		} else if(state->isBase64Encoded) {
        	char *value = state->attrDefaultValueBuffer;
        	int len = strlen(value);

        	int newlen = (len / 4) * 3
        			- (value[len - 1] == '=' ? 1 : 0)
        			- (value[len - 2] == '=' ? 1 : 0);
        	char *newvalue = (char *)malloc(newlen + 1);

        	newlen = base64_decode(value, newvalue);
        	if(newlen == -1) {
        		free(newvalue);
        		LOG_ER("Failed to decode base64 default value in attribute %s (default-value)", state->attrName);
        		stopParser(state);
        		state->parsingStatus = 1;
        		return;
        	}
        	newvalue[newlen] = 0;

        	free(state->attrDefaultValueBuffer);
        	state->attrDefaultValueBuffer = newvalue;
		}
		state->isBase64Encoded = 0;
		/* </class> */
	} else if (strcmp((const char*)name, "class") == 0) {
		if (state->doneParsingClasses) {
			LOG_ER("INVALID CLASS PLACEMENT");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		} else {
			createImmClass(state);

			state->attrFlags = 0;
			state->attrValueTypeSet    = 0;
			state->attrNtfIdSet        = 0;
			state->attrDefaultValueSet = 0;
			free(state->className);
			state->className = NULL;
		}
		/* </attr> or </rdn> */
	} else if (strcmp((const char*)name, "attr") == 0 ||
			   strcmp((const char*)name, "rdn") == 0) {
		if (state->state[state->depth - 1] == CLASS) {
			addClassAttributeDefinition(state);
		} else {
			addObjectAttributeDefinition(state);
		}
		/* </object> */
	} else if (strcmp((const char*)name, "object") == 0) {
		TRACE_8("END OBJECT");
		if (!state->doneParsingClasses && !state->validation) {
			TRACE_8("CCB INIT");
			SaAisErrorT errorCode;

			state->doneParsingClasses = 1;

			/* First time, initialize the imm object api */
			TRACE_8("\n AdminOwner: %s \n", imm_import_adminOwnerName);
			errorCode = immutil_saImmOmAdminOwnerInitialize(state->immHandle,
				imm_import_adminOwnerName,
				SA_TRUE,
				&state->ownerHandle);
			if (errorCode != SA_AIS_OK) {
				LOG_ER("Failed on saImmOmAdminOwnerInitialize %d",
					   errorCode);
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}
			state->adminInit = 1;

			/* ... and initialize the imm ccb api  */
			errorCode = immutil_saImmOmCcbInitialize(state->ownerHandle,
				imm_import_ccb_safe?(SA_IMM_CCB_REGISTERED_OI|SA_IMM_CCB_ALLOW_NULL_OI):0x0,
				&state->ccbHandle);
			if (errorCode != SA_AIS_OK) {
				LOG_ER("Failed to initialize ImmOmCcb %d", errorCode);
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}
			state->ccbInit = 1;

		}

		/* Create the object */
		createImmObject(state);
		/* </imm:IMM-contents> */
	} else if (strcmp((const char*)name, "imm:IMM-contents") == 0) {
		if(!transaction_mode) {
			SaAisErrorT errorCode;

			/* Apply the object creations */
			if (state->ccbInit) {
				errorCode = immutil_saImmOmCcbApply(state->ccbHandle);
				if (SA_AIS_OK != errorCode) {
					LOG_ER("Failed to apply object creations %d", errorCode);
					stopParser(state);
					state->parsingStatus = 1;
					return;
				}
			}
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
	if (((ParserState*)userData)->depth != 0) {
		LOG_ER( "Document ends too early\n");
		stopParser((ParserState *)userData);
		((ParserState*)userData)->parsingStatus = 1;
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

	if (len > MAX_CHAR_BUFFER_SIZE) {
		LOG_ER("The character field is too big len(%d) > max(%d)",
			   len, MAX_CHAR_BUFFER_SIZE);
		stopParser(state);
		state->parsingStatus = 1;
		return;
	} else if (len < 0) {
		LOG_ER("The character array length is negative %d", len);
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* Treat each tag type separately */
	switch (state->state[state->depth]) {
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
		if(len >= SA_MAX_NAME_LENGTH) {
			LOG_ER("DN is too long (%d characters)", len);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		state->objectName = (char*)malloc((size_t)len + 1);

		strncpy(state->objectName, (const char*)chars, (size_t)len);

		state->objectName[len] = '\0';

		break;
	case NAME:
		/* The attrName must be NULL */
		assert(!state->attrName);

		if (state->state[state->depth - 1] == ATTRIBUTE ||
			state->state[state->depth - 1] == RDN) {
			state->attrName = (char*)malloc((size_t)len + 1);
			if (state->attrName == NULL) {
				LOG_ER("Failed to malloc state->attrName");
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}

			strncpy(state->attrName, (const char*)chars, (size_t)len);
			state->attrName[len] = '\0';
		} else {
			LOG_ER("Name not immediately inside an attribute tag");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		break;
	case VALUE:
		if (state->state[state->depth - 1] == ATTRIBUTE) {
			char* str;

			if (!state->valueContinue) {
				/* Start of value, also end of value for 99.999% of cases */
				state->valueContinue = 1;
				str = (char*)malloc((size_t)len + 1);
				if (str == NULL) {
					LOG_ER("Failed to malloc value");
					stopParser(state);
					state->parsingStatus = 1;
					return;
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
				if (str == NULL) {
					LOG_ER("Failed to malloc value");
					stopParser(state);
					state->parsingStatus = 1;
					return;
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

		} else {
			LOG_ER("UNKNOWN PLACEMENT OF VALUE");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		break;
	case CATEGORY:
		if (state->state[state->depth - 1] == CLASS) {
			SaImmClassCategoryT category;

			/* strlen("SA_CONFIG") == 9, strlen("SA_RUNTIME") == 10 */
			if (len == 9 && strncmp((const char*)chars, "SA_CONFIG", (size_t)len) == 0) {
				category = SA_IMM_CLASS_CONFIG;
			} else if (len == 10 && strncmp((const char*)chars, "SA_RUNTIME", (size_t)len) == 0) {
				category = SA_IMM_CLASS_RUNTIME;
			} else {
				LOG_ER("Unknown class category");
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}

			state->classCategorySet = 1;
			state->classCategory = category;
		} else if (state->state[state->depth - 1] == ATTRIBUTE ||
				   state->state[state->depth - 1] == RDN) {
			SaImmAttrFlagsT category;

			/* strlen("SA_CONFIG") == 9, strlen("SA_RUNTIME") == 10 */
			if (len == 9 && strncmp((const char*)chars, "SA_CONFIG", (size_t)len) == 0) {
				category = SA_IMM_ATTR_CONFIG;
			} else if (len == 10 && strncmp((const char*)chars, "SA_RUNTIME", (size_t)len) == 0) {
				category = SA_IMM_ATTR_RUNTIME;
			} else {
				LOG_ER("Unknown attribute category");
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}

			state->attrFlags = state->attrFlags | category;
		}

		break;
	case DEFAULT_VALUE:
		if (state->state[state->depth - 1] == ATTRIBUTE) {
			if (state->attrDefaultValueBuffer == NULL) {
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
		} else {
			LOG_ER("UNKNOWN PLACEMENT OF DEFAULT VALUE");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		break;
	case TYPE:
		if(!(state->attrValueType = charsToTypeHelper(chars, (size_t)len))) {
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
		state->attrValueTypeSet = 1;

		addToAttrTypeCache(state, state->attrValueType);

		break;
	case RDN:

		break;
	case FLAG:
		{
			SaImmAttrFlagsT flg = charsToFlagsHelper(chars, (size_t)len);
			if(flg == 0xffffffffffffffffULL) {
				stopParser(state);
				state->parsingStatus = 1;
			} else {
				state->attrFlags |= flg;
			}
		}
		break;
	case NTFID:
		if (atoi((const char*)chars) < 0) {
			LOG_ER("NtfId can not be negative");
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}

		state->attrNtfId = (SaUint32T)atoi((const char*)chars);
		state->attrNtfIdSet = 1;
		break;
	default:
		LOG_ER("Unknown state");
		stopParser(state);
		state->parsingStatus = 1;
		return;
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
	if (len == strlen("SA_MULTI_VALUE") && strncmp((const char*)str, "SA_MULTI_VALUE", len) == 0) {
		return SA_IMM_ATTR_MULTI_VALUE;
	} else if (len == strlen("SA_RDN") && strncmp((const char*)str, "SA_RDN", len) == 0) {
		return SA_IMM_ATTR_RDN;
	} else if (len == strlen("SA_CONFIG") && strncmp((const char*)str, "SA_CONFIG", len) == 0) {
		return SA_IMM_ATTR_CONFIG;
	} else if (len == strlen("SA_WRITABLE") && strncmp((const char*)str, "SA_WRITABLE", len ) == 0) {
		return SA_IMM_ATTR_WRITABLE;
	} else if (len == strlen("SA_INITIALIZED") && strncmp((const char*)str, "SA_INITIALIZED", len) == 0) {
		return SA_IMM_ATTR_INITIALIZED;
	} else if (len == strlen("SA_RUNTIME") && strncmp((const char*)str, "SA_RUNTIME", len ) == 0) {
		return SA_IMM_ATTR_RUNTIME;
	} else if (len == strlen("SA_PERSISTENT") && strncmp((const char*)str, "SA_PERSISTENT", len ) == 0) {
		return SA_IMM_ATTR_PERSISTENT;
	} else if (len == strlen("SA_CACHED") && strncmp((const char*)str, "SA_CACHED", len) == 0) {
		return SA_IMM_ATTR_CACHED;
	} else if (len == strlen("SA_NOTIFY") && strncmp((const char*)str, "SA_NOTIFY", len) == 0) {
		return SA_IMM_ATTR_NOTIFY;
	} else if (len == strlen("SA_NO_DUPLICATES") && strncmp((const char*)str, "SA_NO_DUPLICATES", len) == 0) {
		return SA_IMM_ATTR_NO_DUPLICATES;
	} else if (len == strlen("SA_NO_DANGLING") && strncmp((const char*)str, "SA_NO_DANGLING", len) == 0) {
		return SA_IMM_ATTR_NO_DANGLING;
	}

	std::string flag((char *)str, len);
	if(isXsdLoaded) {
		AttrFlagSet::iterator it = attrFlagSet.find(flag);
		if(it != attrFlagSet.end()) {
			return 0;
		}
	}

	LOG_ER("UNKNOWN FLAGS, %s", flag.c_str());

	return 0xffffffffffffffffULL;
}

/**
 * Takes a string and returns the corresponding valueType
 */
static SaImmValueTypeT charsToTypeHelper(const xmlChar* str, size_t len)
{
	if (len == strlen("SA_NAME_T") && strncmp((const char*)str, "SA_NAME_T", len) == 0) {
		return SA_IMM_ATTR_SANAMET;
	} else if (len == strlen("SA_INT32_T") && strncmp((const char*)str, "SA_INT32_T", len ) == 0) {
		return SA_IMM_ATTR_SAINT32T;
	} else if (len == strlen("SA_UINT32_T") && strncmp((const char*)str, "SA_UINT32_T", len) == 0) {
		return SA_IMM_ATTR_SAUINT32T;
	} else if (len == strlen("SA_INT64_T") && strncmp((const char*)str, "SA_INT64_T", len ) == 0) {
		return SA_IMM_ATTR_SAINT64T;
	} else if (len == strlen("SA_UINT64_T") && strncmp((const char*)str, "SA_UINT64_T", len) == 0) {
		return SA_IMM_ATTR_SAUINT64T;
	} else if (len == strlen("SA_TIME_T") && strncmp((const char*)str, "SA_TIME_T", len  ) == 0) {
		return SA_IMM_ATTR_SATIMET;
	} else if (len == strlen("SA_FLOAT_T") && strncmp((const char*)str, "SA_FLOAT_T", len ) == 0) {
		return SA_IMM_ATTR_SAFLOATT;
	} else if (len == strlen("SA_DOUBLE_T") && strncmp((const char*)str, "SA_DOUBLE_T", len) == 0) {
		return SA_IMM_ATTR_SADOUBLET;
	} else if (len == strlen("SA_STRING_T") && strncmp((const char*)str, "SA_STRING_T", len) == 0) {
		return SA_IMM_ATTR_SASTRINGT;
	} else if (len == strlen("SA_ANY_T") && strncmp((const char*)str, "SA_ANY_T", len   ) == 0) {
		return SA_IMM_ATTR_SAANYT;
	}

	/* strlen("SA_DOUBLE_T") == 11 (the longest type name)
	 * A bit longer string is taken to log, max 20 characters.
	 */
	char unknown[21];
	if(len > 20)
		len = 20;
	memcpy(unknown, str, len);
	unknown[len] = 0;

	LOG_ER("UNKNOWN VALUE TYPE, %s", unknown);

	return (SaImmValueTypeT)0;
}

/**
 * Returns the value for a given attribute name in the xml attribute array
 */
static char* getAttributeValue(const char* attr,
							   const xmlChar** const attrArray)
{
	int i;

	if (attrArray == NULL) {
		LOG_ER("The document is TOO DEEPLY NESTED");
		return NULL;
	}

	for (i = 0; attrArray != NULL && attrArray[i*2] != NULL; i++) {
		if (strcmp(attr, (const char*)attrArray[i*2]) == 0) {
			return(char*)attrArray[i*2 + 1];
		}
	}

	LOG_WA("RETURNING NULL");
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
	if(!state->attrName) {
		LOG_ER("The attrName must be set");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* The value array can not be empty */
	if(!state->attrValueBuffers.size()) {
		LOG_ER("The value array can not be empty");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* The object class must be set */
	if(!state->objectClass) {
		LOG_ER("The object class must be set");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	std::list<SaImmAttrValuesT_2>::iterator iter;
	for(iter=state->attrValues.begin(); iter != state->attrValues.end(); iter++) {
		if(!strcmp(state->attrName, (*iter).attrName)) {
			LOG_ER("Attribute '%s' is defined more than once in object '%s'", state->attrName, state->objectName);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
	}

	/* Set the valueType */
	if(state->validation) {
		/* Set fake value type for validation */
		attrValues.attrValueType = SA_IMM_ATTR_SASTRINGT;
	} else {
		attrValues.attrValueType = getClassAttrValueType(state,
														 state->objectClass,
														 state->attrName);
		if(state->ctxt->instate == XML_PARSER_EOF)
			return;
	}

	TRACE_8("addObjectAttributeDefinition %s, %s, %d",
			state->className,
			state->attrName,
			attrValues.attrValueType);

	/* For each value, convert from char* to SaImmAttrValuesT_2 and
	   store an array pointing to all in attrValues */
	attrValues.attrValuesNumber = state->attrValueBuffers.size();
	attrValues.attrValues = (SaImmAttrValueT*)
							calloc(1, sizeof(SaImmAttrValuesT_2) *
								   attrValues.attrValuesNumber + 1);

	attrValues.attrValues[attrValues.attrValuesNumber] = NULL;

	it = state->attrValueBuffers.begin();
	i = 0;
	while (it != state->attrValueBuffers.end()) {
		TRACE_8("NAME: %s", state->attrName);

		if(charsToValueHelper(&attrValues.attrValues[i],
						   attrValues.attrValueType,
						   *it)) {
			LOG_ER("Failed to parse a value of attribute %s", state->attrName);
			stopParser(state);
			state->parsingStatus = 1;
			break;
		}

		i++;
		it++;
	}

	/* Assign the name */
	if(state->ctxt->instate != XML_PARSER_EOF) {
		len = strlen(state->attrName);
		attrValues.attrName = (char*) malloc(len + 1);
		if (attrValues.attrName == NULL) {
			LOG_ER("Failed to malloc attrValues.attrName");
			stopParser(state);
			state->parsingStatus = 1;
		} else {
			strncpy(attrValues.attrName,
					state->attrName,
					len);
			attrValues.attrName[len] = '\0';

			/* Add attrValues to the list */
			state->attrValues.push_front(attrValues);
		}
	} else {
		i = 0;
		while(attrValues.attrValues[i]) {
			if(attrValues.attrValueType == SA_IMM_ATTR_SASTRINGT)
				free(*((SaStringT *)attrValues.attrValues[i]));
			else if(attrValues.attrValueType == SA_IMM_ATTR_SAANYT)
				free(((SaAnyT*)attrValues.attrValues[i])->bufferAddr);
			free(attrValues.attrValues[i]);
			i++;
		}
		free(attrValues.attrValues);
	}

	/* Free unneeded data */
	for (it = state->attrValueBuffers.begin();
		it != state->attrValueBuffers.end();
		it++) {
		free(*it);
	}

	free(state->attrName);
	state->attrName = NULL;
	state->attrValueBuffers.clear();
	assert(state->attrValueBuffers.size() == 0);
}

/**
 * Saves the rdn attribute for a class in a map
 */
static void saveRDNAttribute(ParserState* state)
{
	if ((state->attrFlags & SA_IMM_ATTR_RDN) &&
		state->classRDNMap.find(std::string(state->className)) ==
		state->classRDNMap.end()) {
		SaImmAttrValuesT_2 values;
		size_t len;

		/* Set the RDN name */
		len = strlen(state->attrName);
		values.attrName = (char*)malloc(len + 1);
		if (values.attrName == NULL) {
			LOG_ER( "Failed to malloc values.attrName");
			stopParser(state);
			state->parsingStatus = 1;
			return;
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
	if(!state->className) {
		LOG_ER("Class name is missing");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	if(!state->attrName) {
		LOG_ER("Attribute name is missing");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

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
		state->classAttrTypeMap.end()) {
		getClassFromImm(state, (const SaImmClassNameT)className);
		if(state->ctxt->instate == XML_PARSER_EOF)
			return (SaImmValueTypeT)-1;
		if (state->classAttrTypeMap.find(classNameString) ==
			state->classAttrTypeMap.end()) {
			stopParser(state);
			state->parsingStatus = 1;
			LOG_ER("Cannot find CLASS %s loaded in IMM", className);
			return (SaImmValueTypeT)-1;
		}
	}


	if (state->classAttrTypeMap[classNameString].find(attrNameString) ==
			state->classAttrTypeMap[classNameString].end()) {
		stopParser(state);
		state->parsingStatus = 1;
		LOG_ER("NO CORRESPONDING ATTRIBUTE %s in class %s", attrName,
			   className);
		return (SaImmValueTypeT)-1;
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
	if (state->attrName != NULL) {
		std::list<SaImmAttrDefinitionT_2>::iterator it;
		for(it=state->attrDefinitions.begin(); it != state->attrDefinitions.end(); it++) {
			if(!strcmp(state->attrName, (*it).attrName)) {
				LOG_ER("Attribute '%s' is defined more than once in class '%s'", state->attrName, state->className);
				stopParser(state);
				state->parsingStatus = 1;
				return;
			}
		}

		attrDefinition.attrName = state->attrName;
	} else {
		LOG_ER( "NO ATTR NAME");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* Save the attribute definition in classRDNMap if the RDN flag is
	 * set */
	saveRDNAttribute(state);
	if(state->ctxt->instate == XML_PARSER_EOF)
		return;

	/* Set attrValueType */
	if(!state->attrValueTypeSet) {
		LOG_ER("Attribute value type is not set");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	attrDefinition.attrValueType = state->attrValueType;
	if (state->state[state->depth] == RDN) {
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
	} else if (state->attrValueTypeSet) {
		attrDefinition.attrValueType = state->attrValueType;
		TRACE_8("ATTR %s, %d",
				attrDefinition.attrName,
				attrDefinition.attrValueType);
	} else {
		LOG_ER("NO ATTR VALUE TYPE");
		stopParser(state);
		state->parsingStatus = 1;
		return;
	}

	/* Set the flags */
	attrDefinition.attrFlags = (state->attrFlags & SA_IMM_ATTR_PERSISTENT) ?
					(state->attrFlags | SA_IMM_ATTR_CACHED) :
					state->attrFlags;

	/* Set the NtfId */
	if (state->attrNtfIdSet) {
		LOG_WA("IGNORING NTF-ID FOR CLASS CREATE");
		//attrDefinition.attrNtfId = state->attrNtfId;
	} else {
		//TRACE_8("NO ATTR NTF ID");
		//attrDefinition.attrNtfId = 0;
	}

	/* Set the default value */
	if (state->attrDefaultValueSet) {
		if(charsToValueHelper(&attrDefinition.attrDefaultValue,
						   state->attrValueType,
						   state->attrDefaultValueBuffer)) {
			LOG_ER("Failed to parse default value of attribute %s", state->attrName);
			stopParser(state);
			state->parsingStatus = 1;
			return;
		}
	} else {
		attrDefinition.attrDefaultValue = NULL;
	}

	/* Add to the list of attrDefinitions */
	state->attrDefinitions.push_front(attrDefinition);

	/* Free the default value */
	free(state->attrDefaultValueBuffer);
	state->attrDefaultValueBuffer = NULL;

	state->attrName = NULL;
}

/**
 * Converts an array of chars to an SaImmAttrValueT
 */
static int charsToValueHelper(SaImmAttrValueT* value,
							   SaImmValueTypeT type,
							   const char* str)
{
	size_t len;
	unsigned int i;
	char byte[5];
	char* endMark;
	int rc = 0;

	TRACE_8("CHARS TO VALUE HELPER");

	switch (type) {
	case SA_IMM_ATTR_SAINT32T:
		*value = malloc(sizeof(SaInt32T));
		*((SaInt32T*)*value) = (SaInt32T)strtol(str, &endMark, 0);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SAUINT32T:
		*value = malloc(sizeof(SaUint32T));
		*((SaUint32T*)*value) = (SaUint32T)strtoul(str, &endMark, 0);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SAINT64T:
		*value = malloc(sizeof(SaInt64T));
		*((SaInt64T*)*value) = (SaInt64T)strtoll(str, &endMark, 0);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SAUINT64T:
		*value = malloc(sizeof(SaUint64T));
		*((SaUint64T*)*value) = (SaUint64T)strtoull(str, &endMark, 0);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SATIMET: // Int64T
		*value = malloc(sizeof(SaInt64T));
		*((SaTimeT*)*value) = (SaTimeT)strtoll(str, &endMark, 0);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SANAMET:
		len = strlen(str);
		if(len >= SA_MAX_NAME_LENGTH) {
			LOG_ER("SaNameT value is too long: %d characters", len);
			return 1;
		}
		*value = malloc(sizeof(SaNameT));
		((SaNameT*)*value)->length = (SaUint16T)len;
		strncpy((char*)((SaNameT*)*value)->value, str, len);
		((SaNameT*)*value)->value[len] = '\0';
		break;
	case SA_IMM_ATTR_SAFLOATT:
		*value = malloc(sizeof(SaFloatT));
		*((SaFloatT*)*value) = (SaFloatT)strtof(str, &endMark);
		rc = *endMark != 0;
		break;
	case SA_IMM_ATTR_SADOUBLET:
		*value = malloc(sizeof(SaDoubleT));
		*((SaDoubleT*)*value) = (SaDoubleT)strtod(str, &endMark);
		rc = *endMark != 0;
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

		for (i = 0; i < len; i++) {
			byte[2] = str[2*i];
			byte[3] = str[2*i + 1];

			((SaAnyT*)*value)->bufferAddr[i] =
			(SaUint8T)strtod(byte, &endMark);
		}

		TRACE_8("SIZE: %d", (int) ((SaAnyT*)*value)->bufferSize);
		break;
	default:
		LOG_ER("BAD VALUE TYPE");
		return -1;
	}

	if(rc) {
		free(*value);
		*value = NULL;
	}

	return rc;
}

int loadImmXML(const char *xmlfile)
{
	ParserState state;
	SaVersionT version;
	SaAisErrorT errorCode;
	int result = 1;

	version.releaseCode   = 'A';
	version.majorVersion  = 2;
	version.minorVersion  = 13;

	TRACE_8("Loading from %s", xmlfile);

	if(!transaction_mode) {
		errorCode = immutil_saImmOmInitialize(&(state.immHandle), NULL, &version);
		if (SA_AIS_OK != errorCode) {
			LOG_ER("Failed to initialize the IMM OM interface (%d)", errorCode);
			return 1;
		}
	} else {
		if(!(*imm_import_immHandle)) {
			errorCode = immutil_saImmOmInitialize(&state.immHandle, NULL, &version);
			if (SA_AIS_OK != errorCode) {
				fprintf(stderr, "Failed to initialize the IMM OM interface (%d)", errorCode);
				return 1;
			}
		} else
			state.immHandle = *imm_import_immHandle;
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

	state.adminInit = *imm_import_ownerHandle != 0;
	state.ownerHandle = *imm_import_ownerHandle;
	state.ccbInit   = *imm_import_ccbHandle != (SaImmCcbHandleT)-1;
	state.ccbHandle = *imm_import_ccbHandle;

	state.validation = 0;

	state.parsingStatus = 0;

	//std::cout << "Loading " << xmlfile << std::endl;

	FILE *file = fopen(xmlfile, "rb");
	if(file) {
		char buf[1024];
		int rd;

		rd = fread(buf, 1, 4, file);
		if(rd > 0) {
			state.ctxt = xmlCreatePushParserCtxt(&my_handler, &state, buf, rd, xmlfile);
			while(state.ctxt->instate != XML_PARSER_EOF && (rd = fread(buf, 1, 1024, file)) > 0) {
				if(xmlParseChunk(state.ctxt, buf, rd, 0))
					break;
			}

			result = state.parsingStatus;

			xmlParseChunk(state.ctxt, buf, 0, 1);

			xmlFreeParserCtxt(state.ctxt);
		}

		fclose(file);
	} else {
		result = 1;
		LOG_ER("Cannot open '%s'", xmlfile);
	}

	if(!transaction_mode) {
		/* Make sure to finalize the imm connections */
		/* Finalize the ccb connection*/
		if (state.ccbInit) {
			errorCode = immutil_saImmOmCcbFinalize(state.ccbHandle);
			if (SA_AIS_OK != errorCode) {
				LOG_WA("Failed to finalize the ccb object connection %d",
						errorCode);
			}
		}

		/* Finalize the owner connection */
		if (state.adminInit) {
			errorCode = immutil_saImmOmAdminOwnerFinalize(state.ownerHandle);
			if (SA_AIS_OK != errorCode) {
				LOG_WA("Failed on saImmOmAdminOwnerFinalize (%d)", errorCode);
			}
		}

		/* Finalize the imm connection */
		if (state.immInit) {
			errorCode = immutil_saImmOmFinalize(state.immHandle);
			if (SA_AIS_OK != errorCode) {
				LOG_WA("Failed on saImmOmFinalize (%d)", errorCode);
			}
		}
	} else {
		/* Save handles */
		if(state.immInit)
			*imm_import_immHandle = state.immHandle;
		if(state.adminInit)
			*imm_import_ownerHandle = state.ownerHandle;
		if(state.ccbInit)
			*imm_import_ccbHandle = state.ccbHandle;
	}

	if(state.attrName)
		free(state.attrName);
	if(state.className)
		free(state.className);

	std::map<std::string, SaImmAttrValuesT_2>::iterator it;
	for(it = state.classRDNMap.begin(); it != state.classRDNMap.end(); it++)
		free(it->second.attrName);
	state.classRDNMap.clear();

	return result;
}

// C and c++ caller wrapper
//  The objective is to keep the code copied from imm_load.cc as close to original as possible
//  to ease a future refactoring towards common codebase
int importImmXML(char* xmlfileC, char* adminOwnerName, int verbose, int ccb_safe,
		SaImmHandleT *immHandle, SaImmAdminOwnerHandleT *ownerHandle,
		SaImmCcbHandleT *ccbHandle, int mode, const char *xsdPath)
{
	imm_import_adminOwnerName = adminOwnerName;
	imm_import_verbose = verbose;
	imm_import_ccb_safe = ccb_safe;

	imm_import_immHandle = immHandle;
	imm_import_ownerHandle = ownerHandle;
	imm_import_ccbHandle = ccbHandle;

	transaction_mode = mode;
	imm_xsd_file = xsdPath;
	isXsdLoaded = false;
	attrFlagSet.clear();

	LOG_IN("file: %s adminOwner: %s", xmlfileC, adminOwnerName);

	// assign own immutil errorhandler (no call to abort())
	immutilError = imm_importImmutilError;

	return loadImmXML(xmlfileC);
}

int validateImmXML(const char *xmlfile, int verbose, int mode)
{
	ParserState state;
	int result = 1;

	TRACE_8("Loading from %s", xmlfile);

	imm_import_verbose = verbose;

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

	state.validation = 1;

	state.parsingStatus = 0;

	transaction_mode = mode;

	FILE *file = fopen(xmlfile, "rb");
	if(file) {
		char buf[1024];
		int rd;

		rd = fread(buf, 1, 4, file);
		if(rd > 0) {
			state.ctxt = xmlCreatePushParserCtxt(&my_handler, &state, buf, rd, xmlfile);
			while(state.ctxt->instate != XML_PARSER_EOF && (rd = fread(buf, 1, 1024, file)) > 0) {
				if(xmlParseChunk(state.ctxt, buf, rd, 0))
					break;
			}

			result = state.parsingStatus;

			xmlParseChunk(state.ctxt, buf, 0, 1);

			xmlFreeParserCtxt(state.ctxt);
		}

		fclose(file);
	} else {
		LOG_ER("Cannot open '%s'", xmlfile);
		if(!transaction_mode)
			exit(EXIT_FAILURE);
	}

	std::map<std::string, SaImmAttrValuesT_2>::iterator it;
	for(it = state.classRDNMap.begin(); it != state.classRDNMap.end(); it++)
		free(it->second.attrName);
	state.classRDNMap.clear();

	return result;
}


