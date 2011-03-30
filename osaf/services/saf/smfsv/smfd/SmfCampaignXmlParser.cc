/*
 *
 * (C) Copyright 2009 The OpenSAF Foundation
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

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */

#include <new>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <assert.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpathInternals.h>
#include <libxml/xmlmemory.h>
#include <libxml/globals.h>

#include "logtrace.h"
#include "saImm.h"

#include "SmfUtils.hh"

#include "SmfImmOperation.hh"
#include "SmfUpgradeAction.hh"

#include "SmfCampaign.hh"
#include "SmfCampaignThread.hh"

#include "SmfCampaignXmlParser.hh"
#include "SmfUpgradeCampaign.hh"
#include "SmfUpgradeProcedure.hh"
#include "SmfUpgradeMethod.hh"
#include "SmfTargetTemplate.hh"
#include "SmfUpgradeStep.hh"
//#include "singlestepupgrade.h"
//#include "node.h"
//#include "upgradecomp.h"

#if defined(LIBXML_XPATH_ENABLED) && defined(LIBXML_SAX1_ENABLED)

#endif

/* ========================================================================
 *   DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   TYPE DEFINITIONS
 * ========================================================================
 */

/* ========================================================================
 *   DATA DECLARATIONS
 * ========================================================================
 */

/* ========================================================================
 *   FUNCTION PROTOTYPES
 * ========================================================================
 */

// ------------------------------------------------------------------------------
// SmfCampaignXmlParser()
// ------------------------------------------------------------------------------
 SmfCampaignXmlParser::SmfCampaignXmlParser():
    m_doc(0), 
    m_xpathCtx(0), 
    m_xpathObj(0)
{
	xmlInitParser();
}

// ------------------------------------------------------------------------------
// ~SmfCampaignXmlParser()
// ------------------------------------------------------------------------------
SmfCampaignXmlParser::~SmfCampaignXmlParser()
{
	xmlXPathFreeObject(m_xpathObj);
	xmlXPathFreeContext(m_xpathCtx);
	xmlFreeDoc(m_doc);
	xmlCleanupParser();	//Shutdown libxml
	xmlMemoryDump();	//this is to debug memory for regression tests
}

// ------------------------------------------------------------------------------
// getClassName()
// ------------------------------------------------------------------------------
std::string 
SmfCampaignXmlParser::getClassName()const
{
	return "SmfCampaignXmlParser";
}

// ------------------------------------------------------------------------------
// toString()
// ------------------------------------------------------------------------------
std::string 
SmfCampaignXmlParser::toString()const
{
	return getClassName();
}

// ------------------------------------------------------------------------------
// parseCampaignXml()
// ------------------------------------------------------------------------------
SmfUpgradeCampaign *
SmfCampaignXmlParser::parseCampaignXml(std::string i_file)
{
	TRACE_ENTER();

	xmlNodeSetPtr nodes = 0;
	int size;
	const xmlChar *xpathExpr = (const xmlChar *)"/upgradeCampaign";
	xmlNsPtr ns = 0;
	xmlNode *upgradeCampaignNode;
	xmlNode *cur;

	SmfUpgradeCampaign *campaign = new(std::nothrow) SmfUpgradeCampaign;
	assert(campaign != NULL);

	/* Load XML document */
	m_doc = xmlParseFile(i_file.c_str());
	if (m_doc == NULL) {
		LOG_ER("SmfCampaignXmlParser::parseCampaignXml, Unable to parse file \"%s\"", i_file.c_str());
		goto error_exit;
	}

	/* Create xpath evaluation context */
	m_xpathCtx = xmlXPathNewContext(m_doc);
	if (m_xpathCtx == NULL) {
		LOG_ER("SmfCampaignXmlParser::parseCampaignXml, Unable to create new XPath context");
		xmlFreeDoc(m_doc);
		goto error_exit;
	}
#if 0
	/* Register namespaces from list (if any) */
	if ((m_nsList != NULL)
	    && (register_namespaces(m_xpathCtx, m_nsList) < 0)) {
		fprintf(stderr, "Error: failed to register namespaces list \"%s\"\n", m_nsList);
		xmlXPathFreeContext(m_xpathCtx);
		xmlFreeDoc(m_doc);
		goto error_exit;
	}
#endif

	m_xpathObj = xmlXPathEvalExpression(xpathExpr, m_xpathCtx);	// Evaluate xpath expression

	if (m_xpathObj == NULL) {
		LOG_ER("SmfCampaignXmlParser::parseCampaignXml, Unable to evaluate xpath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(m_xpathCtx);
		xmlFreeDoc(m_doc);
		goto error_exit;
	}
	//Check that the file contain just one upgrade campaign
	nodes = m_xpathObj->nodesetval;
	size = (nodes) ? nodes->nodeNr : 0;
	if (size != 1) {
		LOG_ER("SmfCampaignXmlParser::parseCampaignXml, UpgradeCampaign tag counter != 1, counter = %d", size);
		goto error_exit;
	}

	upgradeCampaignNode = nodes->nodeTab[0];

	///////////////////////////
	//Parse campaign level tags
	///////////////////////////
	if (!parseCampaignProperties(campaign, upgradeCampaignNode)) {
		LOG_ER("SmfCampaignXmlParser::parseCampaignXml, parseCampaignProperties failed");
		goto error_exit;
	}
	//Get all second level tags and parse the content
	cur = upgradeCampaignNode->xmlChildrenNode;
	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "campaignInfo"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag campaignInfo found");
			if (!parseCampaignInfo(campaign, cur))
				goto error_exit;
		} else if ((!strcmp((char *)cur->name, "campaignInitialization"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag campaignInitialization found");
			parseCampaignInitialization(campaign, cur);
		} else if ((!strcmp((char *)cur->name, "upgradeProcedure"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag upgradeProcedure found\n");
			SmfUpgradeProcedure *up = new(std::nothrow) SmfUpgradeProcedure;
			assert(up != NULL);

			parseUpgradeProcedure(up, cur);
			if(!campaign->addUpgradeProcedure(up))
			{
				LOG_ER("SmfCampaignXmlParser::parseCampaignXml, addUpgradeProcedure failed");
				goto error_exit;
			}
		} else if ((!strcmp((char *)cur->name, "campaignWrapup"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag campaignWrapup found\n");
			parseCampaignWrapup(campaign, cur);
		}

		cur = cur->next;
	}

	campaign->sortProceduresInExecLevelOrder();

	TRACE_LEAVE();
	return campaign;

 error_exit:
	delete campaign;
	TRACE_LEAVE();
	return static_cast < SmfUpgradeCampaign * >(0);
}

// ------------------------------------------------------------------------------
// parseCampaignProperties()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseCampaignProperties(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();

	char *s;
	std::string str;

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"safSmfCampaign"))) {
		TRACE("Tag safSmfCampaign found : %s\n", s);
		str = s;
		i_campaign->setCampaignName(str);
		xmlFree(s);
	}
//TBD "xmlns:xsi" does not work, don't know why
	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"xmlns:xsi"))) {
		TRACE("Tag xmlns:xsi found : %s\n", s);
		str = s;
		i_campaign->setXsi(str);
		xmlFree(s);
	}

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"noNamespaceSchemaLocation"))) {
		TRACE("Tag noNamespaceSchemaLocation found : %s\n", s);
		str = s;
		i_campaign->setNameSpaceSchemaLocation(str);
		xmlFree(s);
	}

	TRACE_LEAVE();
	return true;
}

// ------------------------------------------------------------------------------
// parseCampaignInfo()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseCampaignInfo(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();

	char *s;
	std::string str;
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "campaignPeriod"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag campaignPeriod found\n");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfCmpgExpectedTime"))) {
				TRACE("saSmfCmpgExpectedTime = %s\n", s);
				str = s;
				i_campaign->setCampaignPeriod(str);
				xmlFree(s);
			}
		}

		if ((!strcmp((char *)cur->name, "configurationBase"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag configurationBase found\n");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfCmpgConfigBase"))) {
				TRACE("saSmfCmpConfigBase = %s\n", s);
				str = s;
				i_campaign->setConfigurationBase(str);
				xmlFree(s);
			}
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
	return true;
}

// ------------------------------------------------------------------------------
// parseUpgradeProcedure()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseUpgradeProcedure(SmfUpgradeProcedure * io_up, xmlNode * i_node)
{
	TRACE_ENTER();
	char *s;
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"safSmfProcedure"))) {
		io_up->setProcName(s);
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"saSmfExecLevel"))) {
		io_up->setExecLevel(s);
		xmlFree(s);
	}

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "outageInfo"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag outageInfo found\n");
                        parseOutageInfo(io_up, cur);
		}
		if ((!strcmp((char *)cur->name, "procInitAction"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag procInitAction found\n");
			parseProcInitAction(io_up, cur);
		}
		if ((!strcmp((char *)cur->name, "procWrapupAction"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag procWrapupAction found\n");
			parseProcWrapupAction(io_up, cur);
		}
		if ((!strcmp((char *)cur->name, "upgradeMethod"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag upgradeMethod found");
			//Parse upgrade methods
			xmlNode *cur2 = cur->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "rollingUpgrade"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag rollingUpgrade found\n");
					SmfRollingUpgrade *ru = new(std::nothrow) SmfRollingUpgrade;
					assert(ru != NULL);
					parseRollingUpgrade(ru, cur2);
					io_up->setUpgradeMethod(ru);
					break;
				}
				if ((!strcmp((char *)cur2->name, "singleStepUpgrade"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag singleStepUpgrade found\n");
					SmfSinglestepUpgrade *su = new(std::nothrow) SmfSinglestepUpgrade;
					assert(su != NULL);
					parseSinglestepUpgrade(su, cur2);
					io_up->setUpgradeMethod(su);
					break;
				}
				cur2 = cur2->next;
			}	//End while
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseOutageInfo()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseOutageInfo(SmfUpgradeProcedure * i_proc, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	//Choice of do/UndoAdminOper, immCCB, do/UndoCliCmd and callback

	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "acceptableServiceOutage")) && (cur->ns == ns)) {
			TRACE("xmlTag acceptableServiceOutage found, parsing not implemented");
		}
		if ((!strcmp((char *)cur->name, "procedurePeriod")) && (cur->ns == ns)) {
			TRACE("xmlTag procedurePeriod found");
                        s = (char *)xmlGetProp(cur, (const xmlChar *) "time");
                        if (s != NULL) {
                                i_proc->setProcedurePeriod((SaTimeT)strtoll(s, NULL, 0));
                                xmlFree(s);
                        }
		}

		cur = cur->next;
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseProcInitAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseProcInitAction(SmfUpgradeProcedure * i_proc, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	//Choice of do/UndoAdminOper, immCCB, do/UndoCliCmd and callback

	int sequenceNumber = 1;
	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(sequenceNumber++);
			assert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_proc->addProcInitAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(sequenceNumber++);
			assert(iccb != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"ccbFlags"))) {
				TRACE("ccbFlagss = %s\n", s);
				iccb->setCcbFlag(s);
			}

			parseImmCcb(iccb, cur);
			i_proc->addProcInitAction(iccb);
		}
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(sequenceNumber++);
			assert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_proc->addProcInitAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found, not implemented");
		}

		cur = cur->next;
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseProcWrapupAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseProcWrapupAction(SmfUpgradeProcedure * i_proc, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	//Choice of do/UndoAdminOper, immCCB, do/UndoCliCmd and callback

	int sequenceNumber = 1;
	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(sequenceNumber++);
			assert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_proc->addProcWrapupAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(sequenceNumber++);
			assert(iccb != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"ccbFlags"))) {
				TRACE("ccbFlagss = %s\n", s);
				iccb->setCcbFlag(s);
			}

			parseImmCcb(iccb, cur);
			i_proc->addProcWrapupAction(iccb);
		}
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(sequenceNumber++);
			assert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_proc->addProcWrapupAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found, not implemented");
		}

		cur = cur->next;
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseRollingUpgrade()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseRollingUpgrade(SmfRollingUpgrade * io_rolling, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "upgradeScope"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag upgradeScope found");

			xmlNode *cur2 = cur->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "byTemplate"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag byTemplate found");

					SmfByTemplate *templ = new(std::nothrow) SmfByTemplate;
					assert(templ != NULL);
					parseByTemplate(templ, cur2);
					io_rolling->setUpgradeScope(templ);
				}
				cur2 = cur2->next;
			}
		}
		if ((!strcmp((char *)cur->name, "upgradeStep"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag upgradeStep found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfStepRestartOption"))) {
				TRACE("saSmfStepRestartOption =  %lu", strtoul(s, NULL, 0));
				io_rolling->setStepRestartOption(strtoul(s, NULL, 0));
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfStepMaxRetry"))) {
				TRACE("saSmfStepMaxRetryCount = %lu", strtoul(s, NULL, 0));
				io_rolling->setStepMaxRetryCount(strtoul(s, NULL, 0));
				xmlFree(s);
			}

			parseCallback(io_rolling, cur->xmlChildrenNode);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseByTemplate()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseByTemplate(SmfByTemplate * io_templ, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "targetNodeTemplate"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag targetNodeTemplate found");

			SmfTargetNodeTemplate *tnt = new(std::nothrow) SmfTargetNodeTemplate;
			assert(tnt != NULL);

			parseTargetNodeTemplate(tnt, cur);
			io_templ->setTargetNodeTemplate(tnt);

		}
		if ((!strcmp((char *)cur->name, "targetEntityTemplate"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag targetEntityTemplate found");

			SmfTargetEntityTemplate *tet = new(std::nothrow) SmfTargetEntityTemplate;
			assert(tet != NULL);

			parseTargetEntityTemplate(tet, cur);
			io_templ->addTargetEntityTemplate(tet);
		}

		cur = cur->next;
	}
}

// ------------------------------------------------------------------------------
// parseTargetNodeTemplate()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseTargetNodeTemplate(SmfTargetNodeTemplate * io_templ, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"objectDN"))) {
		TRACE("objectDN = %s", s);
		io_templ->setObjectDn(s);
		xmlFree(s);
	}

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "activationUnitTemplate"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag activationUnitTemplate found");
			parseParentTypeElements(io_templ, cur);
		}
		if ((!strcmp((char *)cur->name, "swRemove")) && (cur->ns == ns)) {
			TRACE("xmlTag swRemove found");
			SmfBundleRef *br = new(std::nothrow) SmfBundleRef;
			assert(br != NULL);
			parseBundleRef(br, cur);
			io_templ->addSwRemove(br);	//Add the bundle ref to the TargetNodeTemplate
		}
		if ((!strcmp((char *)cur->name, "swAdd")) && (cur->ns == ns)) {
			TRACE("xmlTag swAdd found");
			SmfBundleRef *br = new(std::nothrow) SmfBundleRef;
			assert(br != NULL);
			parseBundleRef(br, cur);
			io_templ->addSwInstall(br);	//Add the bundle ref to the TargetNodeTemplate
		}

		cur = cur->next;
	}
}

// ------------------------------------------------------------------------------
// parseTargetEntityTemplate()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseTargetEntityTemplate(SmfTargetEntityTemplate * io_templ, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	SmfParentType *pt = new(std::nothrow) SmfParentType;
	assert(pt != NULL);

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "parent")) && (cur->ns == ns)) {
			TRACE("xmlTag parent found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s\n", s);
				pt->setParentDn(s);
				xmlFree(s);
			}
		}

		if ((!strcmp((char *)cur->name, "type")) && (cur->ns == ns)) {
			TRACE("xmlTag type found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s\n", s);
				pt->setTypeDn(s);
				xmlFree(s);
			}
		}

		if ((!strcmp((char *)cur->name, "modifyOperation"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag modifyOperation found");
			SmfImmModifyOperation *mo = new(std::nothrow) SmfImmModifyOperation;
			assert(mo != NULL);
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectRDN"))) {
				TRACE("objectRDN = %s\n", s);
				mo->setRdn(s);
				xmlFree(s);
			}

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"operation"))) {
				TRACE("operation = %s\n", s);
				mo->setOp(s);
				xmlFree(s);
			}

			parseAttribute(mo, cur);
			io_templ->addModifyOperation(mo);
		}

		cur = cur->next;
	}

	io_templ->setEntityTemplate(pt);

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseParentTypeElements()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseParentTypeElements(SmfTargetNodeTemplate * io_templ, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;
	std::string str;
        SmfParentType *pt = 0;

        bool parentFound = false;
        bool typeFound   = false;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "parent")) && (cur->ns == ns))	//Fetch parent/type pair
		{
			TRACE("xmlTag parent found");
                        if ( parentFound == true ) { //Two parent tag was in sequence
                                io_templ->addActivationUnitTemplate(pt); //Save the previous found parent
                        }

                        pt = new(std::nothrow) SmfParentType;    //Create a new parent/type pair for the new parent
                        assert(pt != NULL);
                        parentFound = true;

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s\n", s);
				str = s;
				pt->setParentDn(str);
				xmlFree(s);
			}

		} else if ((!strcmp((char *)cur->name, "type")) && (cur->ns == ns)) {	//Fetch parent/type pair with only type element
                        if ( parentFound == true ) {
                                TRACE("xmlTag type found, in pair with previous parent");
                                if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
                                        TRACE("objectDN = %s\n", s);
                                        str = s;
                                        pt->setTypeDn(str);
                                        xmlFree(s);
                                }
                                parentFound = false;
                                typeFound   = true;

                        } else {
                                TRACE("xmlTag type found, single");
                                if ( typeFound == true ) { //Two type tag was in sequence
                                        io_templ->addActivationUnitTemplate(pt); //Save the previous found parent
                                }

                                pt = new(std::nothrow) SmfParentType;
                                assert(pt != NULL);
                                typeFound = true;

                                if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
                                        TRACE("objectDN = %s\n", s);
                                        str = s;
                                        pt->setTypeDn(str);
                                        xmlFree(s);
                                }
                        }
		}

		cur = cur->next;
	}

	if(pt != 0) {
		io_templ->addActivationUnitTemplate(pt);
	}

        TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseBundleRef()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseBundleRef(SmfBundleRef * io_br, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNode *cur = i_node;
	char *s;
	xmlNsPtr ns = 0;

	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"bundleDN"))) {
		TRACE("bundleDN = %s\n", s);
		io_br->setBundleDn(s);
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"pathnamePrefix"))) {
		TRACE("pathnamePrefix = %s\n", s);
		io_br->setPathNamePrefix(s);
		xmlFree(s);
	}
	for (cur = i_node->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (strcmp((char *)cur->name, "plmExecEnv") == 0 && cur->ns == ns) {
			parsePlmExecEnv(io_br->m_plmExecEnvList, cur);
		}
	}
}

// ------------------------------------------------------------------------------
// parsePlmExecEnv()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parsePlmExecEnv(
	std::list<SmfPlmExecEnv>& plmexecenv_list, xmlNode* node)
{
	SmfPlmExecEnv env;
	char* s;
	s = (char *)xmlGetProp(node, (const xmlChar *)"plmExecEnviron");
	if (s != NULL) {
		env.m_plmExecEnviron = s;
		xmlFree(s);
	}
	s = (char *)xmlGetProp(node, (const xmlChar *)"clmNode");
	if (s != NULL) {
		env.m_clmNode = s;
		xmlFree(s);
	}
	s = (char *)xmlGetProp(node, (const xmlChar *)"amfNode");
	if (s != NULL) {
		env.m_amfNode = s;
		xmlFree(s);
	}
	plmexecenv_list.push_back(env);
	
}

// ------------------------------------------------------------------------------
// parseAttribute()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseAttribute(SmfImmOperation * io_immo, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "attribute"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag attribute found");

			SmfImmAttribute ia;

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"name"))) {
				TRACE("name = %s\n", s);
				ia.setName(s);
				xmlFree(s);
			}

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"type"))) {
				TRACE("type = %s\n", s);
				ia.setType(s);
				xmlFree(s);
			}
			//Get the values
			xmlNode *cur2 = cur->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "value"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag value found");
					if ((s = (char *)xmlNodeListGetString(m_doc, cur2->xmlChildrenNode, 1))) {
						TRACE("value = %s", s);
						ia.addValue(s);
						xmlFree(s);
					}
				}

				cur2 = cur2->next;
			}

			io_immo->addValue(ia);
		}

		cur = cur->next;
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCallback()
//  (plus some help functions)
// ------------------------------------------------------------------------------

static SmfCallback::StepCountT parseStepCount(xmlNode* node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "onEveryStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onEveryStep found");
			return SmfCallback::onEveryStep;
		} else if (strcmp((char *)n->name, "onFirstStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onFirstStep found");
			return SmfCallback::onFirstStep;
		} else if (strcmp((char *)n->name, "onLastStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onLastStep found");
			return SmfCallback::onLastStep;
		} else if (strcmp((char *)n->name, "halfWay") == 0 && node->ns == ns) {
			TRACE("xmlTag halfWay found");
			return SmfCallback::halfWay;
		}
	}
	assert(0);
	return SmfCallback::onEveryStep;
}

static SmfCallback::AtActionT parseAtAction(xmlNode* node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "beforeLock") == 0 && node->ns == ns) {
			TRACE("xmlTag beforeLock found");
			return SmfCallback::beforeLock;
		} else if (strcmp((char *)n->name, "beforeTermination") == 0 && node->ns == ns) {
			TRACE("xmlTag beforeTermination found");
			return SmfCallback::beforeTermination;
		} else if (strcmp((char *)n->name, "afterImmModification") == 0 && node->ns == ns) {
			TRACE("xmlTag afterImmModification found");
			return SmfCallback::afterImmModification;
		} else if (strcmp((char *)n->name, "afterInstantiation") == 0 && node->ns == ns) {
			TRACE("xmlTag afterInstantiation found");
			return SmfCallback::afterInstantiation;
		} else if (strcmp((char *)n->name, "afterUnlock") == 0 && node->ns == ns) {
			TRACE("xmlTag afterUnlock found");
			return SmfCallback::afterUnlock;
		}
	}
	assert(0);
	return SmfCallback::beforeLock;
}

void 
SmfCampaignXmlParser::parseCallback(
	SmfUpgradeMethod* upgrade, xmlNode* node)
{
	xmlNsPtr ns = 0;
	while (node != NULL) {
		SmfCallback cb;
		// First scan for customizationTime/atAction tags
		while (node != NULL) {
			if (strcmp((char *)node->name, "customizationTime") == 0 && node->ns == ns) {
				TRACE("xmlTag customizationTime found");
				for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
					if (strcmp((char *)n->name, "onStep") == 0 && n->ns == ns) {
						TRACE("xmlTag onStep found");
						cb.m_stepCount = parseStepCount(n);
					} else if (strcmp((char *)n->name, "atAction") == 0 && n->ns == ns) {
						TRACE("xmlTag atAction found");
						cb.m_atAction = parseAtAction(n);
					}
				}
				break;
			} else if (strcmp((char *)node->name, "atAction") == 0 && node->ns == ns) {
				TRACE("xmlTag atAction found");
				cb.m_atAction = parseAtAction(node);
				break;
			}
			node = node->next;
		}

		if (node == NULL) break; // END OF LIST

		// We have found a customizationTime/atAction tag, now scan for callback
		while (node != NULL) {
			if (strcmp((char *)node->name, "callback") == 0 && node->ns == ns) {
				TRACE("xmlTag callback found");
				char *s;
				s = (char *)xmlGetProp(node, (const xmlChar *) "callbackLabel");
				if (s != NULL) {
					cb.m_callbackLabel = s;
					xmlFree(s);
				}
				s = (char *)xmlGetProp(node, (const xmlChar *) "stringToPass");
				if (s != NULL) {
					cb.m_stringToPass = s;
					xmlFree(s);
				}
				s = (char *)xmlGetProp(node, (const xmlChar *) "time");
				if (s != NULL) {
					cb.m_time = strtoll(s, NULL, 0);
					xmlFree(s);
				}
				break;
			}
			node = node->next;
		}
		assert(node != NULL); // A callback tag must have been found
		upgrade->addCallback(cb);
		node = node->next;
	}
}


// ------------------------------------------------------------------------------
// parseSinglestepUpgrade()
// ------------------------------------------------------------------------------
void
SmfCampaignXmlParser::parseSinglestepUpgrade(
	SmfSinglestepUpgrade* single, xmlNode* node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur;

	for (cur = node->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if ((!strcmp((char *)cur->name, "upgradeScope")) && (cur->ns == ns)) {
			TRACE("xmlTag upgradeScope found");
			for (xmlNode*n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if ((!strcmp((char *)n->name, "forAddRemove")) && (n->ns == ns)) {
					TRACE("xmlTag forAddRemove found");
					SmfForAddRemove* scope = new(std::nothrow) SmfForAddRemove;
					assert(scope != NULL);
					parseForAddRemove(scope, n);
					single->setUpgradeScope(scope);
				}
				if ((!strcmp((char *)n->name, "forModify")) && (n->ns == ns)) {
					TRACE("xmlTag forModify found");
					SmfForModify* scope = new(std::nothrow) SmfForModify;
					assert(scope != NULL);
					parseForModify(scope, n);
					single->setUpgradeScope(scope);
				}
			}
		}
		if ((!strcmp((char *)cur->name, "upgradeStep")) && (cur->ns == ns)) {
			TRACE("xmlTag upgradeStep found");
			char *s;
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfStepRestartOption"))) {
				TRACE("saSmfStepRestartOption =  %lu", strtoul(s, NULL, 0));
				single->setStepRestartOption(strtoul(s, NULL, 0));
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfStepMaxRetry"))) {
				TRACE("saSmfStepMaxRetryCount = %lu", strtoul(s, NULL, 0));
				single->setStepMaxRetryCount(strtoul(s, NULL, 0));
				xmlFree(s);
			}

			parseCallback(single, cur->xmlChildrenNode);
		}
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseForAddRemove()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseForAddRemove(SmfForAddRemove *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "deactivationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag deactivationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			parseActivationUnit(act, n);
			scope->setDeactivationUnit(act);

		} else if ((!strcmp((char *)n->name, "activationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag activationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			parseActivationUnit(act, n);
			scope->setActivationUnit(act);
		}
	}
}

// ------------------------------------------------------------------------------
// parseForModify()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseForModify(SmfForModify *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "targetEntityTemplate")) && (n->ns == ns)) {
			TRACE("xmlTag targetEntityTemplate found");
			SmfTargetEntityTemplate *tet = new(std::nothrow) SmfTargetEntityTemplate;
			assert(tet != NULL);
			parseTargetEntityTemplate(tet, n);
			scope->addTargetEntityTemplate(tet);

		} else if ((!strcmp((char *)n->name, "activationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag activationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			parseActivationUnit(act, n);
			scope->setActivationUnit(act);
		}
	}
}

// ------------------------------------------------------------------------------
// parseActivationUnit()
// ------------------------------------------------------------------------------
void
SmfCampaignXmlParser::parseActivationUnit(SmfActivationUnitType *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "actedOn")) && (n->ns == ns)) {
			TRACE("xmlTag actedOn found");
			parseEntityList(scope->m_actedOn, n);

		} else if ((!strcmp((char *)n->name, "removed")) && (n->ns == ns)) {
			TRACE("xmlTag removed found");
			parseEntityList(scope->m_removed, n);

		} else if ((!strcmp((char *)n->name, "added")) && (n->ns == ns)) {
			TRACE("xmlTag added found");
			SmfImmCreateOperation createop;
			parseImmCreate(&createop, n);
			scope->m_added.push_back(createop);

		} else if ((!strcmp((char *)n->name, "swRemove")) && (n->ns == ns)) {
			TRACE("xmlTag swRemove found");
			SmfBundleRef br;
			parseBundleRef(&br, n);
			scope->m_swRemowe.push_back(br);

		} else if ((!strcmp((char *)n->name, "swAdd")) && (n->ns == ns)) {
			TRACE("xmlTag swAdd found");
			SmfBundleRef br;
			parseBundleRef(&br, n);
			scope->m_swAdd.push_back(br);

		}
	}
}

// ------------------------------------------------------------------------------
// parseEntityList()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseEntityList(
	std::list<SmfEntity>& entityList, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "byName") == 0 && n->ns == ns) {
			SmfEntity entity;
			char* s = (char *)xmlGetProp(n, (const xmlChar *)"objectDN");
			assert(s != NULL);
			entity.m_name = s;
			entityList.push_back(entity);
			xmlFree(s);
		} else if (strcmp((char *)n->name, "byTemplate") == 0 && n->ns == ns) {
			SmfEntity entity;
			for (xmlNode* n2 = n->xmlChildrenNode; n2 != NULL; n2 = n2->next) {
				if ((!strcmp((char *)n2->name, "parent")) && (n->ns == ns)) {
					assert(entity.m_parent.length() == 0);
					char* s = (char *)xmlGetProp(n2, (const xmlChar *)"objectDN");
					assert(s != NULL);
					entity.m_parent = s;
					xmlFree(s);
				} else if ((!strcmp((char *)n2->name, "type")) && (n->ns == ns)) {
					assert(entity.m_type.length() == 0);
					char* s = (char *)xmlGetProp(n2, (const xmlChar *)"objectDN");
					assert(s != NULL);
					entity.m_type = s;
					xmlFree(s);
				}
			}
			entityList.push_back(entity);
		}
	}
}

// ------------------------------------------------------------------------------
// parseCampaignInitialization()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCampaignInitialization(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "addToImm")) && (cur->ns == ns)) {
			TRACE("xmlTag addToImm found");
			parseAddToImm(i_campaign, cur);
		}
		if ((!strcmp((char *)cur->name, "callbackAtInit"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtInit found, no parsing implemented");

		}
		if ((!strcmp((char *)cur->name, "callbackAtBackup"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtBackup found, no parsing implemented");

		}
		if ((!strcmp((char *)cur->name, "callbackAtRollback"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtRollback found, no parsing implemented");

		}
		if ((!strcmp((char *)cur->name, "campInitAction"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag campInitAction found");
			parseCampInitAction(i_campaign, cur);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCampaignWrapup()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCampaignWrapup(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();

	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "campCompleteAction")) && (cur->ns == ns)) {
			TRACE("xmlTag campCompleteAction found");
			parseCampCompleteAction(i_campaign, cur);
		} else if ((!strcmp((char *)cur->name, "waitToCommit")) && (cur->ns == ns)) {
			TRACE("xmlTag waitToCommit found");
                        s = (char *)xmlGetProp(cur, (const xmlChar *) "time");
                        if (s != NULL) {
                           i_campaign->setWaitToCommit((SaTimeT)strtoll(s, NULL, 0));
                           xmlFree(s);
                        }
		} else if ((!strcmp((char *)cur->name, "callbackAtCommit")) && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtCommit found, no parsing implemented yet");
		} else if ((!strcmp((char *)cur->name, "campWrapupAction")) && (cur->ns == ns)) {
			TRACE("xmlTag campWrapupAction found");
			parseCampWrapupAction(i_campaign, cur);
		} else if ((!strcmp((char *)cur->name, "waitToAllowNewCampaign")) && (cur->ns == ns)) {
			TRACE("xmlTag waitToAllowNewCampaign found");
                        s = (char *)xmlGetProp(cur, (const xmlChar *) "time");
                        if (s != NULL) {
                           i_campaign->setWaitToAllowNewCampaign((SaTimeT)strtoll(s, NULL, 0));
                           xmlFree(s);
                        }
		} else if ((!strcmp((char *)cur->name, "removeFromImm")) && (cur->ns == ns)) {
			TRACE("xmlTag removeFromImm found");
			parseRemoveFromImm(i_campaign, cur);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseAddToImm()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseAddToImm(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "softwareBundle"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag softwareBundle found");
			SmfImmCreateOperation *ico = new(std::nothrow) SmfImmCreateOperation;
			assert(ico != 0);
			parseSoftwareBundle(ico, cur);

			i_campaign->addCampInitAddToImm(ico);
		} else if ((!strcmp((char *)cur->name, "amfEntityTypes"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag amfEntityTypes found");
			parseAmfEntityTypes(i_campaign, cur);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseRemoveFromImm()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseRemoveFromImm(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "softwareBundleDN"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag softwareBundleDN found");
			SmfImmDeleteOperation *ico = new(std::nothrow) SmfImmDeleteOperation;
			assert(ico != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"bundleDN"))) {
				TRACE("bundleDN = %s", s);
				ico->setDn(s);
				xmlFree(s);
			}

			i_campaign->addCampWrapupRemoveFromImm(ico);
		}
		if ((!strcmp((char *)cur->name, "amfEntityTypeDN"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag amfEntityTypes found, no parsing implemented");
			SmfImmDeleteOperation *ico = new(std::nothrow) SmfImmDeleteOperation;
			assert(ico != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s", s);
				ico->setDn(s);
				xmlFree(s);
			}

			i_campaign->addCampWrapupRemoveFromImm(ico);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseSoftwareBundle()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseSoftwareBundle(SmfImmCreateOperation * i_createOper, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	char *s;

	//Specify classname and objects parent
	i_createOper->setClassName(SMF_BUNDLE_CLASS_NAME);

	//Set the bundle name
	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"name"))) {
		TRACE("name = %s\n", s);
		//Create a attribute SmfImmAttribute object
		SmfImmAttribute attr;
                
                //Find out parent etc from DN
                std::string dn = s;
		xmlFree(s);
                std::string parent;
                std::string rdn = dn;
		if (dn.find(",") != std::string::npos) { 
			parent = dn.substr(dn.find(",") + 1);
			rdn    = dn.substr(0, dn.find(","));
		}
 		TRACE("SoftwareBundle: parent = %s ,rdn = %s\n", parent.c_str(), rdn.c_str());
                i_createOper->setParentDn(parent);

		attr.setName("safSmfBundle");
		attr.setType("SA_IMM_ATTR_SASTRINGT");
		attr.addValue(rdn);
		i_createOper->addValue(attr);
	}
	//Find childrens
	cur = i_node->xmlChildrenNode;

	//Set the rest of the attributes
	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "removal")) && (cur->ns == ns)) {
			TRACE("xmlTag removal found");
			xmlNode *cur2 = cur->xmlChildrenNode;
			while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "offline")) && (cur2->ns == ns)) {
					TRACE("xmlTag offline found");
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"command"))) {
						TRACE("command = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleRemoveOfflineCmdUri");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"args"))) {
						TRACE("args = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleRemoveOfflineCmdArgs");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)
								    "saSmfBundleRemoveOfflineScope"))) {
						TRACE("saSmfBundleRemoveOfflineScope = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleRemoveOfflineScope");
						attr.setType("SA_IMM_ATTR_SAUINT32T");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
				}
				if ((!strcmp((char *)cur2->name, "online"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag online found");
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"command"))) {
						TRACE("command = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleRemoveOnlineCmdUri");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"args"))) {
						TRACE("args = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleRemoveOnlineCmdArgs");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
				}

				cur2 = cur2->next;
			}
		} else if ((!strcmp((char *)cur->name, "installation")) && (cur->ns == ns)) {
			TRACE("xmlTag installation found");
			xmlNode *cur2 = cur->xmlChildrenNode;
			while (cur2 != NULL) { if ((!strcmp((char *)cur2->name, "offline")) && (cur2->ns == ns)) {
                                        TRACE("xmlTag offline found");
                                        if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"command"))) {
                                                TRACE("command = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleInstallOfflineCmdUri");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"args"))) {
						TRACE("args = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleInstallOfflineCmdArgs");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"saSmfBundleInstallOfflineScope"))) {
						TRACE("saSmfBundleInstallOfflineScope = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleInstallOfflineScope");
						attr.setType("SA_IMM_ATTR_SAUINT32T");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
				}
				if ((!strcmp((char *)cur2->name, "online")) && (cur2->ns == ns)) {
					TRACE("xmlTag online found");
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"command"))) {
						TRACE("command = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleInstallOnlineCmdUri");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"args"))) {
						TRACE("args = %s", s);
						//Create a attribute SmfImmAttribute object
						SmfImmAttribute attr;

						attr.setName("saSmfBundleInstallOnlineCmdArgs");
						attr.setType("SA_IMM_ATTR_SASTRINGT");
						attr.addValue(s);
						xmlFree(s);
						i_createOper->addValue(attr);
					}
				}

				cur2 = cur2->next;
			}
		} else if ((!strcmp((char *)cur->name, "defaultCliTimeout"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag defaultCliTimeout found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)
						    "saSmfBundleDefaultCmdTimeout"))) {
				TRACE("saSmfBundleDefaultCmdTimeout = %s\n", s);

				//Create a attribute SmfImmAttribute object
				SmfImmAttribute attr;

				attr.setName("saSmfBundleDefaultCmdTimeout");
				attr.setType("SA_IMM_ATTR_SATIMET");
				attr.addValue(s);
				xmlFree(s);
				i_createOper->addValue(attr);
			}
		}

		cur = cur->next;
	}
}

// ------------------------------------------------------------------------------
// prepareCreateOperation()
// ------------------------------------------------------------------------------

SmfImmCreateOperation* 
SmfCampaignXmlParser::prepareCreateOperation(
	char const* parent, char const* className, xmlNode* node, char const* rdnAttribute,
	std::string& dn, bool isnamet)
{
	SmfImmCreateOperation *ico = new(std::nothrow) SmfImmCreateOperation;
	assert(ico != 0);
	ico->setClassName(className);
        if (parent != NULL)
            ico->setParentDn(parent);
	char* s = (char *)xmlGetProp(node, (const xmlChar*)rdnAttribute);
	assert(s != NULL);
	SmfImmAttribute attr;
	attr.setName(rdnAttribute);
	if (isnamet)
		attr.setType("SA_IMM_ATTR_SANAMET");
	else
		attr.setType("SA_IMM_ATTR_SASTRINGT");
	attr.addValue(s);
        ico->addValue(attr);

	dn += s;
	if (parent != NULL) {
		dn += ",";
		dn += parent;
	}

	xmlFree(s);
	return ico;
}


// ------------------------------------------------------------------------------
// parseAmfEntityTypes()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseAmfEntityTypes(SmfUpgradeCampaign* i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur;
	SmfImmCreateOperation *ico;

	for (cur = i_node->xmlChildrenNode; cur != NULL; cur = cur->next) {
		if (strcmp((char*)cur->name, "AppBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag AppBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfAppBaseType", cur, "safAppType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "AppType") == 0 && n->ns == ns) {
					TRACE("xmlTag AppType found");
					parseAppType(i_campaign, n, dn.c_str());
				}
			}

		} else if (strcmp((char*)cur->name, "SUBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag SUBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSUBaseType", cur, "safSuType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "SUType") == 0 && n->ns == ns) {
					TRACE("xmlTag SUType found");
					parseSUType(i_campaign, n, dn.c_str());
				}
			}

		} else if (strcmp((char*)cur->name, "SGBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag SGBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSGBaseType", cur, "safSgType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "SGType") == 0 && n->ns == ns) {
					TRACE("xmlTag SGType found");
					parseSGType(i_campaign, n, dn.c_str());
				}
			}

		} else if (strcmp((char*)cur->name, "CompBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag CompBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfCompBaseType", cur, "safCompType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "CompType") == 0 && n->ns == ns) {
					TRACE("xmlTag CompType found");
					parseCompType(i_campaign, n, dn.c_str());
				}
			}

		} else if (strcmp((char*)cur->name, "CSBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag CSBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfCSBaseType", cur, "safCSType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "CSType") == 0 && n->ns == ns) {
					TRACE("xmlTag CSType found");
					parseCSType(i_campaign, n, dn.c_str());
				}
			}

		} else if (strcmp((char*)cur->name, "ServiceBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag ServiceBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSvcBaseType", cur, "safSvcType", dn);
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "ServiceType") == 0 && n->ns == ns) {
					TRACE("xmlTag ServiceType found");
					parseServiceType(i_campaign, n, dn.c_str());
				}
			}
		}
	}
}

// ------------------------------------------------------------------------------
// parseAppType()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseAppType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfAppType", i_node, "safVersion", dn);
	SmfImmAttribute attr;
	attr.setName("saAmfApptSGTypes");
	attr.setType("SA_IMM_ATTR_SANAMET");

	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "serviceGroupType") == 0 && n->ns == ns) {
			TRACE("xmlTag serviceGroupType found");
			char* s = (char *)xmlGetProp(n, (const xmlChar*)"saAmfApptSGTypes");
			assert(s != NULL);
			attr.addValue(s);
			xmlFree(s);
		}
	}
	ico->addValue(attr);
	i_campaign->addCampInitAddToImm(ico);
}

// ------------------------------------------------------------------------------
// addAttribute()
// ------------------------------------------------------------------------------

void SmfCampaignXmlParser::addAttribute(
	SmfImmCreateOperation* ico, xmlNode* n, char const* attrname, char const* attrtype,
	bool optional, char const* objattr)
{
	char* s = (char *)xmlGetProp(n, (const xmlChar*)attrname);
	if (s == NULL && optional) return;
	assert(s != NULL);
	SmfImmAttribute a;
	if (objattr != NULL)
		a.setName(objattr);
	else
		a.setName(attrname);
	a.setType(attrtype);
	a.addValue(s);
	ico->addValue(a);
	xmlFree(s);
}

// ------------------------------------------------------------------------------
// parseSGType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseSGType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfSGType", i_node, "safVersion", dn);

	// saAmfSgtValidSuTypes is SA_MULTI_VALUE
	SmfImmAttribute attr;
	attr.setName("saAmfSgtValidSuTypes");
	attr.setType("SA_IMM_ATTR_SANAMET");

	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "suType") == 0 && n->ns == ns) {
			TRACE("xmlTag suType found");
			char* s = (char *)xmlGetProp(n, (const xmlChar*)"saAmfSgtValidSuTypes");
			assert(s != NULL);
			attr.addValue(s);
			xmlFree(s);

		} else if (strcmp((char*)n->name, "redundancy") == 0 && n->ns == ns) {
			TRACE("xmlTag redundancy found");
			addAttribute(
				ico, n, "saAmfSgtRedundancyModel", "SA_IMM_ATTR_SAUINT32T");
			
		} else if (strcmp((char*)n->name, "compRestart") == 0 && n->ns == ns) {
			TRACE("xmlTag compRestart found");
			addAttribute(
				ico, n, "saAmfSgtDefCompRestartProb", "SA_IMM_ATTR_SATIMET");
			addAttribute(
				ico, n, "saAmfSgtDefCompRestartMax", "SA_IMM_ATTR_SAUINT32T");

		} else if (strcmp((char*)n->name, "suRestart") == 0 && n->ns == ns) {
			TRACE("xmlTag suRestart found");
			addAttribute(
				ico, n, "saAmfSgtDefSuRestartProb", "SA_IMM_ATTR_SATIMET");
			addAttribute(
				ico, n, "saAmfSgtDefSuRestartMax", "SA_IMM_ATTR_SAUINT32T");
			
		} else if (strcmp((char*)n->name, "autoAttrs") == 0 && n->ns == ns) {
			TRACE("xmlTag autoAttrs found");
			addAttribute(
				ico, n, "saAmfSgtDefAutoAdjustProb", "SA_IMM_ATTR_SATIMET");
			addAttribute(
				ico, n, "safAmfSgtDefAutoAdjust", "SA_IMM_ATTR_SAUINT32T", true,
				"saAmfSgtDefAutoAdjust");
			addAttribute(
				ico, n, "safAmfSgtDefAutoRepair", "SA_IMM_ATTR_SAUINT32T", true,
				"saAmfSgtDefAutoRepair");
		}
	}

	ico->addValue(attr);
	i_campaign->addCampInitAddToImm(ico);
}

// ------------------------------------------------------------------------------
// parseSUType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseSUType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfSUType", i_node, "safVersion", dn);
	i_campaign->addCampInitAddToImm(ico);

	// saAmfSutProvidesSvcTypes is SA_MULTI_VALUE
	SmfImmAttribute attr;
	attr.setName("saAmfSutProvidesSvcTypes");
	attr.setType("SA_IMM_ATTR_SANAMET");

	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "supportedSvcType") == 0 && n->ns == ns) {
			TRACE("xmlTag supportedSvcType found");
			char* s = (char *)xmlGetProp(n, (const xmlChar*)"saAmfSutProvidesSvcType");
			assert(s != NULL);
			attr.addValue(s);
			xmlFree(s);

		} else if (strcmp((char*)n->name, "mandatoryAttrs") == 0 && n->ns == ns) {
			TRACE("xmlTag mandatoryAttrs found");
			addAttribute(
				ico, n, "saAmfSutIsExternal", "SA_IMM_ATTR_SAUINT32T");
			addAttribute(
				ico, n, "saAmfSutDefSUFailover", "SA_IMM_ATTR_SAUINT32T");

		} else if (strcmp((char*)n->name, "componentType") == 0 && n->ns == ns) {
			TRACE("xmlTag componentType found");
			parseComponentType(i_campaign, n, dn.c_str());
		}
	}

	ico->addValue(attr);
}

// ------------------------------------------------------------------------------
// parseComponentType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseComponentType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, char const* parent)
{
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfSutCompType", i_node, "safMemberCompType", dn, true);
	addAttribute(
		ico, i_node, "saAmfSutMinNumComponents", "SA_IMM_ATTR_SAUINT32T", true);
	addAttribute(
		ico, i_node, "saAmfSutMaxNumComponents", "SA_IMM_ATTR_SAUINT32T", true);
	i_campaign->addCampInitAddToImm(ico);
}

// ------------------------------------------------------------------------------
// elementToAttr()
// ------------------------------------------------------------------------------
void
SmfCampaignXmlParser::elementToAttr(
	SmfImmCreateOperation* ico, xmlNode* node, 
	char const* tag, char const* attrname, char const* attrtype, bool optional)
{
	xmlNsPtr ns = 0;
	unsigned int nElements = 0;
	SmfImmAttribute attr;
	attr.setName(attrname);
	attr.setType(attrtype);
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, tag) == 0 && n->ns == ns) {
			TRACE("xmlTag %s found", tag);
			char* s = (char*)xmlNodeListGetString(
				m_doc, n->xmlChildrenNode, 1);
			assert(s != NULL);
			attr.addValue(s);
			xmlFree(s);
			nElements++;
		}
	}
	if (nElements > 0) {
		ico->addValue(attr);
	} else {
		assert(optional);
	}
}

// ------------------------------------------------------------------------------
// parseCompType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseCompType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfCompType", i_node, "safVersion", dn);
	i_campaign->addCampInitAddToImm(ico);

	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "providesCSType") == 0 && n->ns == ns) {
			TRACE("xmlTag providesCSType found");
			parseProvidesCSType(i_campaign, n, dn.c_str());

		} else if (strcmp((char*)n->name, "compTypeDefaults") == 0 && n->ns == ns) {
			TRACE("xmlTag compTypeDefaults found");
			addAttribute(
				ico, n, "saAmfCtCompCategory", "SA_IMM_ATTR_SAUINT32T");
			addAttribute(
				ico, n, "saAmfCtDefRecoveryOnError", "SA_IMM_ATTR_SAUINT32T");
			addAttribute(
				ico, n, "saAmfCtDefClcCliTimeout", "SA_IMM_ATTR_SATIMET", true);
			addAttribute(
				ico, n, "saAmfCtDefCallbackTimeout", "SA_IMM_ATTR_SATIMET", true);
			addAttribute(
				ico, n, "saAmfCtDefInstantiationLevel", "SA_IMM_ATTR_SAUINT32T", true);
			addAttribute(
				ico, n, "saAmfCtDefQuiescingCompleteTimeout", "SA_IMM_ATTR_SATIMET", true);
			addAttribute(
				ico, n, "saAmfCtDefDisableRestart", "SA_IMM_ATTR_SAUINT32T", true);
			elementToAttr(ico, n, "cmdEnv", "saAmfCtDefCmdEnv", "SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "instantiateCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag instantiateCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathInstantiateCmd", "SA_IMM_ATTR_SASTRINGT");
			elementToAttr(
				ico, n, "cmdArgv", "saAmfCtDefInstantiateCmdArgv",
				"SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "terminateCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag terminateCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathTerminateCmd", "SA_IMM_ATTR_SASTRINGT");
			elementToAttr(
				ico, n, "cmdArgv", "saAmfCtDefTerminateCmdArgv",
				"SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "cleanupCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag cleanupCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathCleanupCmd", "SA_IMM_ATTR_SASTRINGT");
			elementToAttr(
				ico, n, "cmdArgv", "saAmfCtDefCleanupCmdArgv",
				"SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "amStartCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag amStartCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathAmStartCmd", "SA_IMM_ATTR_SASTRINGT");
			elementToAttr(
				ico, n, "cmdArgv", "saAmfCtDefAmStartCmdArgv",
				"SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "amStopCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag amStopCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathAmStopCmd", "SA_IMM_ATTR_SASTRINGT");
			elementToAttr(
				ico, n, "cmdArgv", "saAmfCtDefAmStopCmdArgv",
				"SA_IMM_ATTR_SASTRINGT");

		} else if (strcmp((char*)n->name, "healthCheck") == 0 && n->ns == ns) {
			TRACE("xmlTag healthCheck found");
			parseHealthCheck(i_campaign, n, dn.c_str());

		} else if (strcmp((char*)n->name, "swBundle") == 0 && n->ns == ns) {
			TRACE("xmlTag swBundle found");
			addAttribute(
				ico, n, "saAmfCtSwBundle", "SA_IMM_ATTR_SANAMET");

		}
	}

}

// ------------------------------------------------------------------------------
// parseProvidesCSType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseProvidesCSType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfCtCsType", i_node, "safSupportedCsType", dn, true);
	i_campaign->addCampInitAddToImm(ico);

	addAttribute(
		ico, i_node, "saAmfCtCompCapability", "SA_IMM_ATTR_SAUINT32T");
	addAttribute(
		ico, i_node, "saAmfCtDefNumMaxActiveCsi", "SA_IMM_ATTR_SAUINT32T", true,
		"saAmfCtDefNumMaxActiveCSIs");
	addAttribute(
		ico, i_node, "saAmfCtDefNumMaxStandbyCsi", "SA_IMM_ATTR_SAUINT32T", true,
		"saAmfCtDefNumMaxStandbyCSIs");
}

// ------------------------------------------------------------------------------
// parseHealthCheck()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseHealthCheck(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfHealthcheckType", i_node, "safHealthcheckKey", dn);
	i_campaign->addCampInitAddToImm(ico);
	addAttribute(
		ico, i_node, "saAmfHealthcheckPeriod", "SA_IMM_ATTR_SATIMET", false, "saAmfHctDefPeriod");
	addAttribute(
		ico, i_node, "saAmfHealthcheckMaxDuration", "SA_IMM_ATTR_SATIMET", false, "saAmfHctDefMaxDuration");	
}

// ------------------------------------------------------------------------------
// parseServiceType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseServiceType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfSvcType", i_node, "safVersion", dn);
	i_campaign->addCampInitAddToImm(ico);

	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "csType") == 0 && n->ns == ns) {
			TRACE("xmlTag csType found");
			parseCsType(i_campaign, n, dn.c_str());
		}
		else if (strcmp((char*)n->name, "defWeights") == 0 && n->ns == ns) {
			TRACE("xmlTag defWeights found");
			addAttribute(
				ico, n, "saAmfSvcDefActiveWeight", "SA_IMM_ATTR_SASTRINGT");	
			addAttribute(
				ico, n, "saAmfSvcDefStandbyWeight", "SA_IMM_ATTR_SASTRINGT");	
		}
	}
}

// ------------------------------------------------------------------------------
// parseCsType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseCsType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	std::string dn;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfSvcTypeCSTypes", i_node, "safMemberCSType", dn, true);
	i_campaign->addCampInitAddToImm(ico);
	addAttribute(
		ico, i_node, "saAmfSvctMaxNumCSIs", "SA_IMM_ATTR_SAUINT32T", true);	
}

// ------------------------------------------------------------------------------
// parseCSType()
// ------------------------------------------------------------------------------

void 
SmfCampaignXmlParser::parseCSType(
	SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
	char const* parent)
{
	xmlNsPtr ns = 0;
	std::string dn;
	unsigned int cnt = 0;
	SmfImmCreateOperation* ico = prepareCreateOperation(
		parent, "SaAmfCSType", i_node, "safVersion", dn);
	i_campaign->addCampInitAddToImm(ico);

	SmfImmAttribute attr;
	attr.setName("saAmfCSAttrName");
	attr.setType("SA_IMM_ATTR_SASTRINGT");
	for (xmlNode* n = i_node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char*)n->name, "csAttribute") == 0 && n->ns == ns) {
			TRACE("xmlTag csAttribute found");
			char* s = (char *)xmlGetProp(n, (const xmlChar*)"saAmfCSAttrName");
			assert(s != NULL);
			attr.addValue(s);
			xmlFree(s);
			cnt++;
		}
	}
	if (cnt > 0) {
		ico->addValue(attr);
	}
}

// ------------------------------------------------------------------------------
// parseCampInitAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCampInitAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	int sequenceNumber = 1;
	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(sequenceNumber++);
			assert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampInitAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(sequenceNumber++);
			assert(iccb != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"ccbFlags"))) {
				TRACE("ccbFlagss = %s\n", s);
				iccb->setCcbFlag(s);
			}

			parseImmCcb(iccb, cur);
			i_campaign->addCampInitAction(iccb);
		}
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(sequenceNumber++);
			assert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampInitAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found, not implemented");
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCampCompleteAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCampCompleteAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	//Choice of doUndoAdminOper, immCCB, doUndoCliCmd and callback

	int sequenceNumber = 1;
	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(sequenceNumber++);
			assert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampCompleteAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(sequenceNumber++);
			assert(iccb != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"ccbFlags"))) {
				TRACE("ccbFlagss = %s\n", s);
				iccb->setCcbFlag(s);
			}

			parseImmCcb(iccb, cur);
			i_campaign->addCampCompleteAction(iccb);
		}
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(sequenceNumber++);
			assert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampCompleteAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found, not implemented");
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCampWrapupAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCampWrapupAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	int sequenceNumber = 1;
	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
                if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(sequenceNumber++);
			assert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampWrapupAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(sequenceNumber++);
			assert(iccb != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"ccbFlags"))) {
				TRACE("ccbFlagss = %s\n", s);
				iccb->setCcbFlag(s);
			}

			parseImmCcb(iccb, cur);
			i_campaign->addCampWrapupAction(iccb);
		}
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(sequenceNumber++);
			assert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampWrapupAction(opa);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCliCommandAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCliCommandAction(SmfCliCommandAction * i_cmdAction, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"command"))) {
				TRACE("command = %s", s);
				std::string cmd(s);
				if(CAMPAIGN_ROOT_TAG == cmd.substr(0, cmd.find_first_of('/'))) {
					TRACE("Keyword %s found, add dir path", CAMPAIGN_ROOT_TAG);
					std::string dirpath = SmfCampaignThread::instance()->campaign()->getCampaignXmlDir();
					cmd.replace(cmd.find(CAMPAIGN_ROOT_TAG), sizeof(CAMPAIGN_ROOT_TAG), dirpath);
					TRACE("command = %s", cmd.c_str());
				}

				i_cmdAction->setDoCmd(cmd);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"args"))) {
				TRACE("args = %s", s);
				i_cmdAction->setDoCmdArgs(s);
				xmlFree(s);
			}
		}
		if ((!strcmp((char *)cur->name, "undoCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag undoCliCommand found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"command"))) {
				TRACE("command = %s", s);
				std::string cmd(s);
				if(CAMPAIGN_ROOT_TAG == cmd.substr(0, cmd.find_first_of('/'))) {
					TRACE("Keyword %s found, add dir path", CAMPAIGN_ROOT_TAG);
					std::string dirpath = SmfCampaignThread::instance()->campaign()->getCampaignXmlDir();
					cmd.replace(cmd.find(CAMPAIGN_ROOT_TAG), sizeof(CAMPAIGN_ROOT_TAG), dirpath);
					TRACE("command = %s", cmd.c_str());
				}

				i_cmdAction->setUndoCmd(cmd);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"args"))) {
				TRACE("args = %s", s);
				i_cmdAction->setUndoCmdArgs(s);
				xmlFree(s);
			}
		}
		if ((!strcmp((char *)cur->name, "plmExecEnv")) && (cur->ns == ns)) {
			TRACE("xmlTag plmExecEnv found");
			parsePlmExecEnv(i_cmdAction->m_plmExecEnvList, cur);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseImmCreate()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseImmCreate(SmfImmCreateOperation* i_createop, xmlNode* i_node)
{
	char *s;

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)
				    "objectClassName"))) {
		TRACE("objectClassName = %s", s);
		i_createop->setClassName(s);
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)
				    "parentObjectDN"))) {
		TRACE("parentObjectDN = %s", s);
		// "=" means top-level (the silly interpretation is
		// imposed by the schema)
		if (strcmp(s, "=") != 0)
			i_createop->setParentDn(s);
		xmlFree(s);
	}
	//Fetch and add the attributes and values
	parseAttribute(i_createop, i_node);
}

// ------------------------------------------------------------------------------
// parseImmCcb()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseImmCcb(SmfImmCcbAction * i_ccbAction, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;
	char *s;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "create")) && (cur->ns == ns)) {
			TRACE("xmlTag create found");
			SmfImmCreateOperation *ico = new(std::nothrow) SmfImmCreateOperation;
			assert(ico != 0);
			parseImmCreate(ico, cur);

			//Add the create operation to the CCB
			i_ccbAction->addOperation(ico);	//Add an operation to the CCB
		}

		if ((!strcmp((char *)cur->name, "delete")) && (cur->ns == ns)) {
			TRACE("xmlTag delete found");
			SmfImmDeleteOperation *ido = new(std::nothrow) SmfImmDeleteOperation;
			assert(ido != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s", s);
				ido->setDn(s);
				xmlFree(s);
			}
			//Add the create operation to the CCB
			i_ccbAction->addOperation(ido);	//Add an operation to the CCB
		}

		if ((!strcmp((char *)cur->name, "modify")) && (cur->ns == ns)) {
			TRACE("xmlTag modify found");
			SmfImmModifyOperation *imo = new(std::nothrow) SmfImmModifyOperation;
			assert(imo != 0);

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s", s);
				imo->setDn(s);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"operation"))) {
				TRACE("operation = %s", s);
				imo->setOp(s);
				xmlFree(s);
			}
			//Fetch and add the attributes and values
			parseAttribute(imo, cur);

			//Add the create operation to the CCB
			i_ccbAction->addOperation(imo);	//Add an operation to the CCB
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseAdminOpAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseAdminOpAction(SmfAdminOperationAction * i_admOpAction, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	char *s;

	while (cur != NULL) {
                //The doAdminOperation part of the do/undo pair
		if ((!strcmp((char *)cur->name, "doAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag doAdminOperation found");

			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s", s);
				i_admOpAction->setDoDn(s);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"operationID"))) {
				TRACE("operationID = %s", s);
                                //Translate Operation ID string to int
				i_admOpAction->setDoId(smf_opStringToInt(s));
				xmlFree(s);
			}

                        //Fetch the parameters
			xmlNode *cur2 = cur->xmlChildrenNode;
                        char *name = NULL;
                        char *type = NULL;
                        char *value = NULL;
                        while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "param")) && (cur2->ns == ns)) {
					TRACE("xmlTag param found");
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"name"))) {
						TRACE("name = %s", s);
                                                name = strdup(s);
                                                xmlFree(s);
                                        }
					if ((s = (char *)xmlGetProp(cur2, (const xmlChar *)"type"))) {
						TRACE("type = %s", s);
                                                type = strdup(s);
                                                xmlFree(s);
                                        }

                                        //Fetch the parameter value
                                        xmlNode *cur3 = cur2->xmlChildrenNode;
                                        while (cur3 != NULL) {
                                                if ((!strcmp((char *)cur3->name, "value")) && (cur3->ns == ns)) {
                                                        TRACE("xmlTag value found");
                                                        if ((s = (char *)xmlNodeListGetString(m_doc, cur3->xmlChildrenNode, 1))) {
                                                                TRACE("value = %s", s);
                                                                value = strdup(s);
                                                                xmlFree(s);
                                                        }
                                                }

                                                cur3 = cur3->next;
                                        }

                                        i_admOpAction->addDoParameter(name, type, value);
                                }

                                cur2 = cur2->next;
                        }
		}

                //The undoAdminOperation part of the do/undo pair
		if ((!strcmp((char *)cur->name, "undoAdminOperation")) && (cur->ns == ns)) {
			TRACE("xmlTag undoAdminOperation found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"objectDN"))) {
				TRACE("objectDN = %s", s);
				i_admOpAction->setUndoDn(s);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"operationID"))) {
				TRACE("operationID = %s", s);
                                //Translate Operation ID string to int
				i_admOpAction->setUndoId(smf_opStringToInt(s));
				xmlFree(s);
			}

                        //Fetch the parameters
			xmlNode *cur2 = cur->xmlChildrenNode;
                        char *name    = NULL;
                        char *type    = NULL;
                        char *value   = NULL;

                        while (cur2 != NULL) {
				if ((!strcmp((char *)cur2->name, "param")) && (cur2->ns == ns)) {
					TRACE("xmlTag param found");
					if ((name = (char *)xmlGetProp(cur2, (const xmlChar *)"name"))) {
						TRACE("name = %s", name);
                                        }
					if ((type = (char *)xmlGetProp(cur2, (const xmlChar *)"type"))) {
						TRACE("type = %s", type);
                                        }

                                        //Fetch the parameter value
                                        xmlNode *cur3 = cur2->xmlChildrenNode;
                                        while (cur3 != NULL) {
                                                if ((!strcmp((char *)cur3->name, "value")) && (cur3->ns == ns)) {
                                                        TRACE("xmlTag value found");
                                                        if ((value = (char *)xmlNodeListGetString(m_doc, cur3->xmlChildrenNode, 1))) {
                                                                TRACE("value = %s", value);
                                                        }
                                                }

                                                cur3 = cur3->next;
                                        }

                                        i_admOpAction->addUndoParameter(name, type, value);
					if(name != 0)
						xmlFree(name);
					if(type != 0)
						xmlFree(type);
					if(value != 0)
						xmlFree(value);
				}

                                cur2 = cur2->next;
                        }
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
}

