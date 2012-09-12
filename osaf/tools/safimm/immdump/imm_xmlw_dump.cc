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
#include <libxml/encoding.h>
#include <unistd.h>
#include <osaf_unicode.h>



/* Functions */

void dumpObjectsXMLw(SaImmHandleT immHandle, 
                        std::map<std::string, std::string> classRDNMap,
                        xmlTextWriterPtr writer)
{
    SaNameT                root;
    SaImmSearchHandleT     searchHandle;
    SaAisErrorT            errorCode;
    SaNameT                objectName;
    SaImmAttrValuesT_2**   attrs;
    unsigned int           retryInterval = 1000000; /* 1 sec */
    unsigned int           maxTries = 15;          /* 15 times == max 15 secs */
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

        objectToXMLw(std::string((char*)objectName.value, objectName.length),
                    attrs,
                    immHandle,
                    classRDNMap,
                    writer);
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

void dumpClassesXMLw(SaImmHandleT immHandle, xmlTextWriterPtr writer)
{
    std::list<std::string> classNameList;
    std::list<std::string>::iterator it;
    TRACE_ENTER();

    classNameList = getClassNames(immHandle);

    it = classNameList.begin();

    while (it != classNameList.end())
    {
        classToXMLw(*it, immHandle, writer);

        it++;
    }
    TRACE_LEAVE();
}

void classToXMLw(std::string classNameString,
                       SaImmHandleT immHandle,
                       xmlTextWriterPtr writer)
{
    SaImmClassCategoryT classCategory;
    SaImmAttrDefinitionT_2 **attrDefinitions;
    SaAisErrorT errorCode;

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
    //std::cout << "Dumping class:" << classNameString<< std::endl;

    if(xmlTextWriterStartElement(writer, (xmlChar*) "class") < 0) {
        std::cout << "Error at xmlTextWriterStartElement" << std::endl;
        exit(1);
    }
    if( xmlTextWriterWriteAttribute(writer, 
                                    (xmlChar*) "name",
                                    (xmlChar *) classNameString.c_str()) < 0) {
    std::cout << "Error at xmlTextWriterWriteAttribute" << std::endl;
      exit(1);
    }

    /* Start an element named "category" as child of class. */
    if(xmlTextWriterWriteElement(writer,
                                 (xmlChar*) "category",
                                 (xmlChar*)((classCategory == SA_IMM_CLASS_CONFIG)?
                                     "SA_CONFIG":"SA_RUNTIME")) < 0) {
    std::cout << "Error at xmlTextWriterStartElement (category)" << std::endl;
      exit(1);
    }

    /* List the attributes*/
    for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
    {
        if ((*p)->attrFlags & SA_IMM_ATTR_RDN)
        {
            if(xmlTextWriterStartElement(writer, (xmlChar*) "rdn") < 0) {
                std::cout << "Error at xmlTextWriterStartElement" << std::endl;
                exit(1);
            }
        }
        else
	    continue;

        if(xmlTextWriterWriteElement(writer,
                                     (xmlChar*) "name",
                                     (xmlChar*)(*p)->attrName) < 0) {
            std::cout << "Error at xmlTextWriterWriteElement (name)" << std::endl;
            exit(1);
        }
        if ((*p)->attrDefaultValue != NULL)
        {
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "default-value",
               (xmlChar*)valueToString((*p)->attrDefaultValue,
               (*p)->attrValueType).c_str()) < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement (default-value)" << std::endl;
                exit(1);
            }
        }

        typeToXMLw(*p, writer);

        flagsToXMLw(*p, writer);

        /* Close element named rdn */
        if(xmlTextWriterEndElement(writer) < 0 ) {
            std::cout << "Error at xmlTextWriterWriteEndElement (rdn)" << std::endl;
            exit(1);
        }

        xmlTextWriterFlush(writer);   // Maybe unnecssary
    }

    for (SaImmAttrDefinitionT_2** p = attrDefinitions; *p != NULL; p++)
    {
        if ((*p)->attrFlags & SA_IMM_ATTR_RDN)
		continue;
        else
        {
            if(xmlTextWriterStartElement(writer, (xmlChar*) "attr") < 0) {
                std::cout << "Error at xmlTextWriterStartElement" << std::endl;
                exit(1);
            }
        }

        if(xmlTextWriterWriteElement(writer,
                                     (xmlChar*) "name",
                                     (xmlChar*)(*p)->attrName) < 0) {
            std::cout << "Error at xmlTextWriterWriteElement (name)" << std::endl;
            exit(1);
        }
        if ((*p)->attrDefaultValue != NULL)
        {
            std::string str = valueToString((*p)->attrDefaultValue, (*p)->attrValueType);
            if(xmlTextWriterStartElement(writer, (xmlChar*) "default-value") < 0) {
                std::cout << "Error at xmlTextWriterStartElement (default-value)" << std::endl;
                exit(1);
            }

            if(osaf_is_valid_xml_utf8(str.c_str())) {
                if(xmlTextWriterWriteString(writer, (xmlChar *) str.c_str()) < 0) {
            		std::cout << "Error at xmlTextWriterWriteString (default-value)" << std::endl;
                    exit(1);
                }
            } else {
                if(xmlTextWriterWriteAttribute(writer, (xmlChar*)"encoding", (xmlChar*)"base64") < 0) {
                	std::cout << "Error at xmlTextWriterWriteAttribute (default-value)" << std::endl;
                    exit(1);
                }

                if(xmlTextWriterWriteBase64(writer, str.c_str(), 0, str.size()) < 0) {
                    std::cout << "Error at xmlTextWriterWriteBase64 (default-value)" << std::endl;
                    exit(1);
                }
            }

            if(xmlTextWriterEndElement(writer) < 0) {
                std::cout << "Error at xmlTextWriterWriteEndElement (default-value)" << std::endl;
                exit(1);
            }
        }

        typeToXMLw(*p, writer);

        flagsToXMLw(*p, writer);

        /* Close element named attr */
        if(xmlTextWriterEndElement(writer) < 0 ) {
	    std::cout << "Error at xmlTextWriterWriteEndElement (attr)" << std::endl;
            exit(1);
        }
	xmlTextWriterFlush(writer);   // Maybe unnecssary
    }


    /* Close element named class */
    if(xmlTextWriterEndElement(writer) < 0 ) {
      std::cout << "Error at xmlTextWriterWriteEndElement (class)" << std::endl;
      exit(1);
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

void objectToXMLw(std::string objectNameString,
                        SaImmAttrValuesT_2** attrs,
                        SaImmHandleT immHandle,
                        std::map<std::string, std::string> classRDNMap,
                        xmlTextWriterPtr writer)
{
    std::string valueString;
    std::string classNameString;
    TRACE_ENTER();

    //std::cout << "Dumping object " << objectNameString << std::endl;
    classNameString = getClassName((const SaImmAttrValuesT_2**) attrs);


    /* Create the object tag */
    if(xmlTextWriterStartElement(writer, (xmlChar*) "object") < 0) {
      std::cout << "Error at xmlTextWriterStartElement" << std::endl;
      exit(1);
    }
    if(xmlTextWriterWriteAttribute(writer, (xmlChar*) "class",
       (xmlChar *) classNameString.c_str()) < 0) {
        std::cout << "Error at xmlTextWriterWriteAttribute" << std::endl;
        exit(1);
    }
    if(xmlTextWriterWriteElement(writer, (xmlChar*) "dn",
       (xmlChar*)objectNameString.c_str()) < 0 )  {
        std::cout << "Error at xmlTextWriterWriteElement (dn)" << std::endl;
        exit(1);
    }
    for (SaImmAttrValuesT_2** p = attrs; *p != NULL; p++)
    {
        /* Skip attributes with attrValues = NULL */
        if ((*p)->attrValues == NULL)
        {
            continue;
        }


        if (classRDNMap.find(classNameString) != classRDNMap.end() &&
            classRDNMap[classNameString] == std::string((*p)->attrName))
        {
            continue;
        }
        else
        {
            if(xmlTextWriterStartElement(writer,(xmlChar*) "attr")  < 0 )  {
                std::cout << "Error at xmlTextWriterStartElement (attr-object)" << std::endl;
                exit(1);
            }
        }
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "name",
           (xmlChar*)(*p)->attrName) < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement(name)" << std::endl;
            exit(1);
        }

        valuesToXMLw(*p, writer);

        /* Close element named attr */
        if(xmlTextWriterEndElement(writer) < 0) {
            std::cout << "Error at xmlTextWriterEndElement (attr - object)" << std::endl;
            exit(1);
        }

    }
    /* Close element named object */
    if(xmlTextWriterEndElement(writer) < 0 ) {
    std::cout << "Error at xmlTextWriterEndElement (object)" << std::endl;
      exit(1);
    }
    TRACE_LEAVE();

}

void valuesToXMLw(SaImmAttrValuesT_2* p, xmlTextWriterPtr writer)
{
    if (!p->attrValues)
    {
       //std::cout << "No values!" << std::endl;
        return;
    }

    for (unsigned int i = 0; i < p->attrValuesNumber; i++)
    {
        std::string str = valueToString(p->attrValues[i], p->attrValueType);
        if(xmlTextWriterStartElement(writer, (xmlChar*) "value") < 0) {
            std::cout << "Error at xmlTextWriterStartElement (value)" << std::endl;
            exit(1);
        }

        if(osaf_is_valid_xml_utf8(str.c_str())) {
            if(xmlTextWriterWriteString(writer, (xmlChar *) str.c_str()) < 0) {
        		std::cout << "Error at xmlTextWriterWriteString (value)" << std::endl;
                exit(1);
            }
        } else {
            if(xmlTextWriterWriteAttribute(writer, (xmlChar*)"encoding", (xmlChar*)"base64") < 0) {
            	std::cout << "Error at xmlTextWriterWriteAttribute (value)" << std::endl;
                exit(1);
            }

            if(xmlTextWriterWriteBase64(writer, str.c_str(), 0, str.size()) < 0) {
                std::cout << "Error at xmlTextWriterWriteBase64 (value)" << std::endl;
                exit(1);
            }
        }

        if(xmlTextWriterEndElement(writer) < 0) {
            std::cout << "Error at xmlTextWriterWriteEndElement (value)" << std::endl;
            exit(1);
        }
    }
}

void flagsToXMLw(SaImmAttrDefinitionT_2* p, xmlTextWriterPtr writer)
{
    SaImmAttrFlagsT flags;

    flags = p->attrFlags;

    if (flags & SA_IMM_ATTR_RUNTIME)
    {
        if(xmlTextWriterWriteElement(writer,
           (xmlChar*) "category",    ///// OBS  category -> flag ??//////
           (xmlChar*) "SA_RUNTIME") < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement (category)" << std::endl;
            exit(1);
        }
    }
    
    if (flags & SA_IMM_ATTR_CONFIG)
    {
        if(xmlTextWriterWriteElement(writer,
           (xmlChar*) "category",    ///// OBS  category -> flag ??//////
           (xmlChar*) "SA_CONFIG") < 0) {
            std::cout << "Error at xmlTextWriterWriteElement (category)" << std::endl;
            exit(1);
        }
    }
    
    if (flags & SA_IMM_ATTR_MULTI_VALUE)
    {
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "flag",
           (xmlChar*) "SA_MULTI_VALUE") < 0) {
            std::cout << "Error at xmlTextWriterWriteElement (flag)" << std::endl;
            exit(1);
        }
    }

    if (flags & SA_IMM_ATTR_WRITABLE)
    {
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "flag",
           (xmlChar*) "SA_WRITABLE")  < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement (flag)" << std::endl;
            exit(1);
        }
    }

    if (flags & SA_IMM_ATTR_INITIALIZED)
    {
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "flag",
           (xmlChar*) "SA_INITIALIZED") < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement(flag)" << std::endl;
            exit(1);
        }
    }
    
    if (flags & SA_IMM_ATTR_PERSISTENT)
    {
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "flag",
           (xmlChar*) "SA_PERSISTENT") < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement (flag SA-PERSISTENT)" << std::endl;
            exit(1);
        }
    }
    
    if (flags & SA_IMM_ATTR_CACHED)
    {
        if(xmlTextWriterWriteElement(writer, (xmlChar*) "flag",
           (xmlChar*) "SA_CACHED") < 0 ) {
            std::cout << "Error at xmlTextWriterWriteElement (flag - SA_CACHED)" << std::endl;
            exit(1);
        }
    }
    
}

void typeToXMLw(SaImmAttrDefinitionT_2* p, xmlTextWriterPtr writer)
{

    switch (p->attrValueType)
    {
        case SA_IMM_ATTR_SAINT32T: 
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_INT32_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement (type)" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SAUINT32T: 
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_UINT32_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement (type)" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SAINT64T:  
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_INT64_T") < 0 )  {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SAUINT64T: 
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_UINT64_T") < 0) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SATIMET:   
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_TIME_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SAFLOATT:  
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_FLOAT_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SADOUBLET: 
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_DOUBLE_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SANAMET:
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_NAME_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SASTRINGT:
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_STRING_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        case SA_IMM_ATTR_SAANYT:
            if(xmlTextWriterWriteElement(writer, (xmlChar*) "type",
               (xmlChar*) "SA_ANY_T") < 0 ) {
                std::cout << "Error at xmlTextWriterWriteElement" << std::endl;
                exit(1);
            }
            break;

        default:
            std::cerr << "Unknown value type" << std::endl;
            exit(1);
    }

}
