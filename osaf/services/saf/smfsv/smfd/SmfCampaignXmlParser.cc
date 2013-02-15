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
#include "SmfCbkUtil.hh"

#define OSAF_MAX_RDN_LENGTH 64

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
        m_xpathObj(0),
        m_actionId(1)
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
	osafassert(campaign != NULL);

	/* Load XML document */
	m_doc = xmlParseFile(i_file.c_str());
	if (m_doc == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Unable to parse file \"%s\"", i_file.c_str());
		goto error_exit;
	}

	/* Create xpath evaluation context */
	m_xpathCtx = xmlXPathNewContext(m_doc);
	if (m_xpathCtx == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Unable to create new XPath context");
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
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Unable to evaluate xpath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(m_xpathCtx);
		xmlFreeDoc(m_doc);
		goto error_exit;
	}
	//Check that the file contain just one upgrade campaign
	nodes = m_xpathObj->nodesetval;
	size = (nodes) ? nodes->nodeNr : 0;
	if (size != 1) {
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: UpgradeCampaign tag counter != 1, counter = %d", size);
		goto error_exit;
	}

	upgradeCampaignNode = nodes->nodeTab[0];

	///////////////////////////
	//Parse campaign level tags
	///////////////////////////
	if (!parseCampaignProperties(campaign, upgradeCampaignNode)) {
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: parseCampaignProperties failed");
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
			if (parseCampaignInitialization(campaign, cur) == false){
				LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Parse of campaignInitialization failed");
				goto error_exit;
			}
		} else if ((!strcmp((char *)cur->name, "upgradeProcedure"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag upgradeProcedure found\n");
			SmfUpgradeProcedure *up = new(std::nothrow) SmfUpgradeProcedure;
			osafassert(up != NULL);

			if (parseUpgradeProcedure(up, cur) == false){
				delete up;
				LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Parse of upgradeProcedure failed");
				goto error_exit;
			}

			//For the procedure, check if the same SwBudle DN exist in both swAdd and swRemove lists
			//In that case the SwBundle shall not be touched, remove the SwBundle DN from the lists.

			SmfUpgradeMethod *upgradeMethod = up->getUpgradeMethod();
			switch (upgradeMethod->getUpgradeMethod()) {
			case SA_SMF_ROLLING:
			{
				SmfRollingUpgrade *rollingUpgrade = (SmfRollingUpgrade *) upgradeMethod;
				const SmfByTemplate *byTemplate = (const SmfByTemplate *)rollingUpgrade->getUpgradeScope();
				const SmfTargetNodeTemplate *nodeTemplate = byTemplate->getTargetNodeTemplate();

	                        const_cast<SmfTargetNodeTemplate *>(nodeTemplate)->removeSwAddRemoveDuplicates();

				break;
			}
			case SA_SMF_SINGLE_STEP:  //No action for single step procedures
			default:
			{
				break;
			}
			} //End switch

			if(!campaign->addUpgradeProcedure(up))
			{
				LOG_NO("SmfCampaignXmlParser::parseCampaignXml: addUpgradeProcedure failed");
				goto error_exit;
			}
		} else if ((!strcmp((char *)cur->name, "campaignWrapup"))
			   && (cur->ns == ns)) {
			TRACE("xmlTag campaignWrapup found\n");
			parseCampaignWrapup(campaign, cur);
		}

		cur = cur->next;
	}

	if (campaign->getUpgradeProcedures().size() == 0) {
		LOG_NO("SmfCampaignXmlParser::parseCampaignXml: Campaign contain no procedure");
		goto error_exit;
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
bool 
SmfCampaignXmlParser::parseUpgradeProcedure(SmfUpgradeProcedure * io_up, xmlNode * i_node)
{
	TRACE_ENTER();
	char *s;
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"safSmfProcedure"))) {
                int procLen = strlen (s);
                if (procLen > OSAF_MAX_RDN_LENGTH) {
                        LOG_NO("SmfCampaignXmlParser::parseUpgradeProcedure: Procedure name too long %d (max %d), %s", procLen, OSAF_MAX_RDN_LENGTH, s);
                        TRACE_LEAVE();
                        return false;
                }

		io_up->setProcName(s);
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(i_node, (const xmlChar *)"saSmfExecLevel"))) {
		io_up->setExecLevel(s);
		xmlFree(s);
	}

	m_actionId = 1; // reset action id for init actions

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
					osafassert(ru != NULL);
					if (parseRollingUpgrade(ru, cur2) == false){
						delete ru;
						LOG_NO("SmfCampaignXmlParser::parseUpgradeProcedure: Parsing of rolling upgrade failed");
						TRACE_LEAVE();
						return false;
					}
					io_up->setUpgradeMethod(ru);
					break;
				}
				if ((!strcmp((char *)cur2->name, "singleStepUpgrade"))
				    && (cur2->ns == ns)) {
					TRACE("xmlTag singleStepUpgrade found\n");
					SmfSinglestepUpgrade *su = new(std::nothrow) SmfSinglestepUpgrade;
					osafassert(su != NULL);
					if (parseSinglestepUpgrade(su, cur2) == false){
						delete su;
						LOG_NO("SmfCampaignXmlParser::parseUpgradeProcedure: Parsing of single step upgrade failed");
						TRACE_LEAVE();						
						return false;
					}
					io_up->setUpgradeMethod(su);
					break;
				}
				cur2 = cur2->next;
			}	//End while
                        m_actionId = 1; // reset action id for wrapup actions
		}

		cur = cur->next;
	}

	if (io_up->getUpgradeMethod() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseUpgradeProcedure: No upgrade method found");
		TRACE_LEAVE();
		return false;
	}

	TRACE_LEAVE();
	return true;
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

	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(m_actionId++);
			osafassert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_proc->addProcInitAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(m_actionId++);
			osafassert(iccb != 0);

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
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(m_actionId++);
			osafassert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_proc->addProcInitAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found");
			SmfCallbackAction *cba = new (std::nothrow) SmfCallbackAction(m_actionId++);
			osafassert(cba != 0);
			parseCallbackAction(cba, cur);
			SmfCallback & cbk = cba->getCallback();
			cbk.m_atAction = SmfCallback::atProcInitAction;
			cbk.m_procedure = i_proc;
			i_proc->addProcInitAction(cba);
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

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(m_actionId++);
			osafassert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_proc->addProcWrapupAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(m_actionId++);
			osafassert(iccb != 0);

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
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(m_actionId++);
			osafassert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_proc->addProcWrapupAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found");
			SmfCallbackAction *cba = new (std::nothrow) SmfCallbackAction(m_actionId++);
			osafassert(cba != 0);
			parseCallbackAction(cba, cur);
			SmfCallback & cbk = cba->getCallback();
			cbk.m_atAction = SmfCallback::atProcWrapupAction;
			cbk.m_procedure = i_proc;
			i_proc->addProcWrapupAction(cba);
		}

		cur = cur->next;
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseRollingUpgrade()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseRollingUpgrade(SmfRollingUpgrade * io_rolling, xmlNode * i_node)
{
	TRACE_ENTER();
	bool rc = true;
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
					osafassert(templ != NULL);
					if (parseByTemplate(templ, cur2) == false) {
						LOG_NO("SmfCampaignXmlParser::parseRollingUpgrade: Parse of byTemplate failed");
						TRACE_LEAVE();
						return false;
					}
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

			if (parseCallback(io_rolling, cur->xmlChildrenNode) == false){
				LOG_NO("SmfCampaignXmlParser::parseRollingUpgrade: Parse of upgrade step callback failed");
				TRACE_LEAVE();
				return false;
			}
		}

		cur = cur->next;
	}

	if (io_rolling->getUpgradeScope() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseRollingUpgrade: No upgrade scope found in rolling procedure");
		rc = false;
	}

	TRACE_LEAVE();
	return rc;
}

// ------------------------------------------------------------------------------
// parseByTemplate()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseByTemplate(SmfByTemplate * io_templ, xmlNode * i_node)
{
	TRACE_ENTER();
	bool rc = true;
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "targetNodeTemplate"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag targetNodeTemplate found");

			SmfTargetNodeTemplate *tnt = new(std::nothrow) SmfTargetNodeTemplate;
			osafassert(tnt != NULL);

			parseTargetNodeTemplate(tnt, cur);
			io_templ->setTargetNodeTemplate(tnt);

		}
		if ((!strcmp((char *)cur->name, "targetEntityTemplate"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag targetEntityTemplate found");

			SmfTargetEntityTemplate *tet = new(std::nothrow) SmfTargetEntityTemplate;
			osafassert(tet != NULL);

			parseTargetEntityTemplate(tet, cur);
			io_templ->addTargetEntityTemplate(tet);
		}

		cur = cur->next;
	}

	if (io_templ->getTargetNodeTemplate() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseByTemplate: No target node template found in procedure");
		rc = false;
	}

	TRACE_LEAVE();
	return rc;
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
			osafassert(br != NULL);
			parseBundleRef(br, cur);
			io_templ->addSwRemove(br);	//Add the bundle ref to the TargetNodeTemplate
		}
		if ((!strcmp((char *)cur->name, "swAdd")) && (cur->ns == ns)) {
			TRACE("xmlTag swAdd found");
			SmfBundleRef *br = new(std::nothrow) SmfBundleRef;
			osafassert(br != NULL);
			parseBundleRef(br, cur);
			io_templ->addSwInstall(br);	//Add the bundle ref to the TargetNodeTemplate
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
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
	osafassert(pt != NULL);

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
			osafassert(mo != NULL);
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
                        osafassert(pt != NULL);
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
                                osafassert(pt != NULL);
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
// parseStepCount()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseStepCount(xmlNode* node, SmfCallback::StepCountT& o_stepcount )
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "onEveryStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onEveryStep found");
			o_stepcount = SmfCallback::onEveryStep;
			return true;
		} else if (strcmp((char *)n->name, "onFirstStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onFirstStep found");
			o_stepcount = SmfCallback::onFirstStep;
			return true;
		} else if (strcmp((char *)n->name, "onLastStep") == 0 && node->ns == ns) {
			TRACE("xmlTag onLastStep found");
			o_stepcount = SmfCallback::onLastStep;
			return true;
		} else if (strcmp((char *)n->name, "halfWay") == 0 && node->ns == ns) {
			TRACE("xmlTag halfWay found");
			o_stepcount = SmfCallback::halfWay;
			return true;
		}
	}
        LOG_NO("SmfCampaignXmlParser::parseStepCount: No stepCount onEveryStep/onFirstStep/onLastStep/halfWay tags found");
	return false;
}

// ------------------------------------------------------------------------------
// parseAtAction()
// ------------------------------------------------------------------------------
bool
SmfCampaignXmlParser::parseAtAction(xmlNode* node, SmfCallback::AtActionT& o_ataction)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "beforeLock") == 0 && node->ns == ns) {
			TRACE("xmlTag beforeLock found");
			o_ataction = SmfCallback::beforeLock;
			return true;
		} else if (strcmp((char *)n->name, "beforeTermination") == 0 && node->ns == ns) {
			TRACE("xmlTag beforeTermination found");
			o_ataction = SmfCallback::beforeTermination;
			return true;
		} else if (strcmp((char *)n->name, "afterImmModification") == 0 && node->ns == ns) {
			TRACE("xmlTag afterImmModification found");
			o_ataction = SmfCallback::afterImmModification;
			return true;
		} else if (strcmp((char *)n->name, "afterInstantiation") == 0 && node->ns == ns) {
			TRACE("xmlTag afterInstantiation found");
			o_ataction = SmfCallback::afterInstantiation;
			return true;
		} else if (strcmp((char *)n->name, "afterUnlock") == 0 && node->ns == ns) {
			TRACE("xmlTag afterUnlock found");
			o_ataction = SmfCallback::afterUnlock;
			return true;
		}
	}

        LOG_NO("SmfCampaignXmlParser::parseAtAction: No beforeTermination/afterImmModification/afterInstantiation/afterUnlock found");
	return false;
}

// ------------------------------------------------------------------------------
// parseCallback()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseCallback(SmfUpgradeMethod* upgrade, xmlNode* node)
{
	xmlNsPtr ns = 0;
	while (node != NULL) {
		SmfCallback *cb = new SmfCallback();
		// First scan for customizationTime/atAction tags
		while (node != NULL) {
			if (strcmp((char *)node->name, "customizationTime") == 0 && node->ns == ns) {
				TRACE("xmlTag customizationTime found");
				for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
					if (strcmp((char *)n->name, "onStep") == 0 && n->ns == ns) {
						TRACE("xmlTag onStep found");
						if (parseStepCount(n, cb->m_stepCount) == false){
							LOG_NO("SmfCampaignXmlParser::parseCallback: Parse of customizationTime/onStep step counter fails");
							return false;
						}
					} else if (strcmp((char *)n->name, "atAction") == 0 && n->ns == ns) {
						TRACE("xmlTag atAction found");
						if (parseAtAction(n, cb->m_atAction) == false){
							LOG_NO("SmfCampaignXmlParser::parseCallback: Parse of customizationTime/atAction fails");
							return false;
						}
					}
				}
				break;
			} else if (strcmp((char *)node->name, "atAction") == 0 && node->ns == ns) {
				TRACE("xmlTag atAction found");
				if (parseAtAction(node, cb->m_atAction) == false){
					LOG_NO("SmfCampaignXmlParser::parseCallback: Parse of atAction fails");
					return false;
				}
				break;
			}
			node = node->next;
		}

		if (node == NULL) break; // END OF LIST

		// We have found a customizationTime/atAction tag, now scan for callback
		while (node != NULL) {
			if (strcmp((char *)node->name, "callback") == 0 && node->ns == ns) {
				TRACE("xmlTag callback found");
				parseCallbackOptions(cb, node);
				break;
			}
			node = node->next;
		}

		if (node == NULL){ // A callback tag must have been found
			LOG_NO("SmfCampaignXmlParser::parseCallback: No callback found for customizationTime/atAction tag");
			return false;
		}

		upgrade->addCallback(cb);
		node = node->next;
	}

	return true;
}

// ------------------------------------------------------------------------------
// parseSinglestepUpgrade()
// ------------------------------------------------------------------------------
bool
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
					osafassert(scope != NULL);
					if (parseForAddRemove(scope, n) == false ){
						delete scope;
						LOG_NO("SmfCampaignXmlParser::parseSinglestepUpgrade: Parse of upgradeScope/forAddRemove fails");
						TRACE_LEAVE();
						return false;
					}
					single->setUpgradeScope(scope);
				}
				if ((!strcmp((char *)n->name, "forModify")) && (n->ns == ns)) {
					TRACE("xmlTag forModify found");
					SmfForModify* scope = new(std::nothrow) SmfForModify;
					osafassert(scope != NULL);
					if (parseForModify(scope, n) == false ){
						delete scope;
						LOG_NO("SmfCampaignXmlParser::parseSinglestepUpgrade: Parse of upgradeScope/forModify fails");
						TRACE_LEAVE();
						return false;
					}
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

			if (parseCallback(single, cur->xmlChildrenNode) == false) {
				TRACE_LEAVE();
				LOG_NO("SmfCampaignXmlParser::parseSinglestepUpgrade: Parse of upgradeStep callback fails");
				return false;
			}
		}
	}

	if (single->getUpgradeScope() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseSinglestepUpgrade: No upgrade scope/forAddRemove/forModify found in single step procedure");
		TRACE_LEAVE();
		return false;
	}

	TRACE_LEAVE();
	return true;
}

// ------------------------------------------------------------------------------
// parseForAddRemove()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseForAddRemove(SmfForAddRemove *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "deactivationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag deactivationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			osafassert(act != NULL);
			if (parseActivationUnit(act, n) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Parsing of deactivationUnit failed");
				return false;
			}
			if (scope->setDeactivationUnit(act) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Fail to set deactivationUnit");
				return false;
			}

		} else if ((!strcmp((char *)n->name, "activationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag activationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			osafassert(act != NULL);
			if (parseActivationUnit(act, n) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Parsing of activationUnit failed");
				return false;
			}
			if (scope->setActivationUnit(act) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Fail to set activationUnit");
				return false;
			}
		}
	}

	if (scope->getDeactivationUnit() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Deactivation unit is missing in single step procedure"); 
		return false;
	}
	if (scope->getActivationUnit() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseForAddRemove: Activation unit is missing in single step procedure"); 
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// parseForModify()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseForModify(SmfForModify *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "targetEntityTemplate")) && (n->ns == ns)) {
			TRACE("xmlTag targetEntityTemplate found");
			SmfTargetEntityTemplate *tet = new(std::nothrow) SmfTargetEntityTemplate;
			osafassert(tet != NULL);
			parseTargetEntityTemplate(tet, n);
			scope->addTargetEntityTemplate(tet);

		} else if ((!strcmp((char *)n->name, "activationUnit")) && (n->ns == ns)) {
			TRACE("xmlTag activationUnit found");
			SmfActivationUnitType* act = new(std::nothrow) SmfActivationUnitType;
			osafassert(act != NULL);
			if (parseActivationUnit(act, n) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForModify: Fail to parse activationUnit");
				return false;
			}
			if (scope->setActivationUnit(act) == false){
				delete act;
				LOG_NO("SmfCampaignXmlParser::parseForModify: Fail to set activationUnit");
				return false;
			}
		}
	}

	const std::list < SmfTargetEntityTemplate * >& tmpTemplate = scope->getTargetEntityTemplate();
	if (tmpTemplate.size() == 0) {
		LOG_NO("SmfCampaignXmlParser::parseForModify: Target entity template is missing in single step procedure");
		return false;
	}
	if (scope->getActivationUnit() == NULL) {
		LOG_NO("SmfCampaignXmlParser::parseForModify: Activation unit is missing in single step procedure");
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// parseActivationUnit()
// ------------------------------------------------------------------------------
bool
SmfCampaignXmlParser::parseActivationUnit(SmfActivationUnitType *scope, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if ((!strcmp((char *)n->name, "actedOn")) && (n->ns == ns)) {
			TRACE("xmlTag actedOn found");
			if (parseEntityList(scope->m_actedOn, n) == false){
				LOG_NO("SmfCampaignXmlParser::parseActivationUnit: parseEntityList actedOn fails");
				return false;
			}
		} else if ((!strcmp((char *)n->name, "removed")) && (n->ns == ns)) {
			TRACE("xmlTag removed found");
			if (parseEntityList(scope->m_removed, n)== false){
				LOG_NO("SmfCampaignXmlParser::parseActivationUnit: parseEntityList removed fails");
				return false;
			}
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

	return true;
}

// ------------------------------------------------------------------------------
// parseEntityList()
// ------------------------------------------------------------------------------

bool 
SmfCampaignXmlParser::parseEntityList(std::list<SmfEntity>& entityList, xmlNode *node)
{
	xmlNsPtr ns = 0;
	for (xmlNode* n = node->xmlChildrenNode; n != NULL; n = n->next) {
		if (strcmp((char *)n->name, "byName") == 0 && n->ns == ns) {
			SmfEntity entity;
			char* s = (char *)xmlGetProp(n, (const xmlChar *)"objectDN");
			if (s == NULL){
				LOG_NO("SmfCampaignXmlParser::parseEntityList: Could not find property of byName/objectDN");
				return false;
			}
			entity.m_name = s;
			entityList.push_back(entity);
			xmlFree(s);
		} else if (strcmp((char *)n->name, "byTemplate") == 0 && n->ns == ns) {
			SmfEntity entity;
			for (xmlNode* n2 = n->xmlChildrenNode; n2 != NULL; n2 = n2->next) {
				if ((!strcmp((char *)n2->name, "parent")) && (n->ns == ns)) {
					if (entity.m_parent.length() != 0) {
						LOG_NO("SmfCampaignXmlParser::parseEntityList: Only one parent allowed in parent/type pair. Create additional byTemplate sections for more parents.");
						return false;
					}
					char* s = (char *)xmlGetProp(n2, (const xmlChar *)"objectDN");
					if (s == NULL){
						LOG_NO("SmfCampaignXmlParser::parseEntityList: Could not find property of byTemplate/parent objectDN");
						return false;
					}
					entity.m_parent = s;
					xmlFree(s);
				} else if ((!strcmp((char *)n2->name, "type")) && (n->ns == ns)) {
					if (entity.m_type.length() != 0) {
						LOG_NO("SmfCampaignXmlParser::parseEntityList: Only one type allowed in parent/type pair. Create additional byTemplate sections for more types.");
						return false;
					}
					char* s = (char *)xmlGetProp(n2, (const xmlChar *)"objectDN");
					if (s == NULL){
						LOG_NO("SmfCampaignXmlParser::parseEntityList: Could not find property of byTemplate/type objectDN");
						return false;
					}
					entity.m_type = s;
					xmlFree(s);
				}
			}
			entityList.push_back(entity);
		}
	}

	return true;
}

// ------------------------------------------------------------------------------
// parseCampaignInitialization()
// ------------------------------------------------------------------------------
bool 
SmfCampaignXmlParser::parseCampaignInitialization(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	TRACE_ENTER();
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node->xmlChildrenNode;

        m_actionId = 1; // reset action id for init actions

	while (cur != NULL) {
		if ((!strcmp((char *)cur->name, "addToImm")) && (cur->ns == ns)) {
			TRACE("xmlTag addToImm found");
			if (parseAddToImm(i_campaign, cur) == false){
				LOG_NO("SmfCampaignXmlParser::parseCampaignInitialization: Parsing of addToImm faild");
				TRACE_LEAVE();
				return false;
			}
		}
		if ((!strcmp((char *)cur->name, "callbackAtInit"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtInit found");
			parseCallbackAtInit(i_campaign, cur);

		}
		if ((!strcmp((char *)cur->name, "callbackAtBackup"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtBackup found");
			parseCallbackAtBackup(i_campaign, cur);

		}
		if ((!strcmp((char *)cur->name, "callbackAtRollback"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtRollback found");
			parseCallbackAtRollback(i_campaign, cur);

		}
		if ((!strcmp((char *)cur->name, "campInitAction"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag campInitAction found");
			parseCampInitAction(i_campaign, cur);
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
	return true;
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

        m_actionId = 1; // reset action id for complete actions

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
                        m_actionId = 1; // reset action id for wrapup actions
		} else if ((!strcmp((char *)cur->name, "callbackAtCommit")) && (cur->ns == ns)) {
			TRACE("xmlTag callbackAtCommit found");
			parseCallbackAtCommit(i_campaign, cur);

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
bool 
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
			osafassert(ico != 0);
			parseSoftwareBundle(ico, cur);

			i_campaign->addCampInitAddToImm(ico);
		} else if ((!strcmp((char *)cur->name, "amfEntityTypes"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag amfEntityTypes found");
			if (parseAmfEntityTypes(i_campaign, cur) == false){
				LOG_NO("SmfCampaignXmlParser::parseAddToImm: Failed to parse amfEntityTypes");
				TRACE_LEAVE();
				return false;
			}
		}

		cur = cur->next;
	}

	TRACE_LEAVE();
	return true;
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
			osafassert(ico != 0);

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
			osafassert(ico != 0);

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
	char* s = (char *)xmlGetProp(node, (const xmlChar*)rdnAttribute);
	if (s == NULL){
		LOG_NO("SmfCampaignXmlParser::prepareCreateOperation: Fail to get rdnAttribute property for create operation");
		return NULL;
	}

	dn += s;
	if (parent != NULL) {
		dn += ",";
		dn += parent;
	}

        SmfImmCreateOperation *ico = new(std::nothrow) SmfImmCreateOperation;
        osafassert(ico != 0);
        ico->setClassName(className);
        if (parent != NULL)
            ico->setParentDn(parent);
        SmfImmAttribute attr;
        attr.setName(rdnAttribute);
        if (isnamet)
                attr.setType("SA_IMM_ATTR_SANAMET");
        else
                attr.setType("SA_IMM_ATTR_SASTRINGT");
        attr.addValue(s);
        ico->addValue(attr);

	xmlFree(s);
	return ico;
}


// ------------------------------------------------------------------------------
// parseAmfEntityTypes()
// ------------------------------------------------------------------------------
bool 
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
			if (ico == NULL) {
				LOG_NO("Fail prepare create operations for AMF entity types SaAmfAppBaseType/safAppType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "AppType") == 0 && n->ns == ns) {
					TRACE("xmlTag AppType found");
					if (parseAppType(i_campaign, n, dn.c_str()) == false){
						LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to parse AMF entity type AppType");
						TRACE_LEAVE();
						return false;
					}
				}
			}

		} else if (strcmp((char*)cur->name, "SUBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag SUBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSUBaseType", cur, "safSuType", dn);
			if (ico == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail prepare create operation for AMF entity type SaAmfSUBaseType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "SUType") == 0 && n->ns == ns) {
					TRACE("xmlTag SUType found");
					if (parseSUType(i_campaign, n, dn.c_str()) == false){
						LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to parse AMF entity type SUType");
						TRACE_LEAVE();
						return false;
					}
				}
			}

		} else if (strcmp((char*)cur->name, "SGBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag SGBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSGBaseType", cur, "safSgType", dn);
			if (ico == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail prepare create operation for AMF entity type SaAmfSGBaseType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "SGType") == 0 && n->ns == ns) {
					TRACE("xmlTag SGType found");
					if (parseSGType(i_campaign, n, dn.c_str()) == false){
						LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to parse AMF entity type SGType");
						TRACE_LEAVE();
						return false;
					}
				}
			}

		} else if (strcmp((char*)cur->name, "CompBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag CompBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfCompBaseType", cur, "safCompType", dn);
			if (ico == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to prepare create operation for AMF entity type SaAmfCompBaseType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "CompType") == 0 && n->ns == ns) {
					TRACE("xmlTag CompType found");
					if (parseCompType(i_campaign, n, dn.c_str()) == false){
						LOG_NO("Fail to parse parse AMF entity type CompType");
						TRACE_LEAVE();
						return false;
					}
				}
			}

		} else if (strcmp((char*)cur->name, "CSBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag CSBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfCSBaseType", cur, "safCSType", dn);
			if (ico == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to prepare create operation for AMF entity type SaAmfCSBaseType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "CSType") == 0 && n->ns == ns) {
					TRACE("xmlTag CSType found");
					if (parseCSType(i_campaign, n, dn.c_str()) == false){
						LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to parse parse AMF entity type CompType");
						TRACE_LEAVE();
						return false;
					}
				}
			}

		} else if (strcmp((char*)cur->name, "ServiceBaseType") == 0 && cur->ns == ns) {
			TRACE("xmlTag ServiceBaseType found");
			std::string dn;
			ico = prepareCreateOperation(NULL, "SaAmfSvcBaseType", cur, "safSvcType", dn);
			if (ico == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAmfEntityTypes: Fail to prepare create operation SaAmfSvcBaseType/safSvcType");
				TRACE_LEAVE();
				return false;
			}
			i_campaign->addCampInitAddToImm(ico);
			for (xmlNode* n = cur->xmlChildrenNode; n != NULL; n = n->next) {
				if (strcmp((char*)n->name, "ServiceType") == 0 && n->ns == ns) {
					TRACE("xmlTag ServiceType found");
					parseServiceType(i_campaign, n, dn.c_str());
				}
			}
		}
	}

	TRACE_LEAVE();
	return true;
}

// ------------------------------------------------------------------------------
// parseAppType()
// ------------------------------------------------------------------------------
bool 
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
			if (s == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseAppType: Fail to parse saAmfApptSGTypes");
				return false;
			}
			attr.addValue(s);
			xmlFree(s);
		}
	}
	ico->addValue(attr);
	i_campaign->addCampInitAddToImm(ico);

	return true;
}

// ------------------------------------------------------------------------------
// addAttribute()
// ------------------------------------------------------------------------------

bool 
SmfCampaignXmlParser::addAttribute(
	SmfImmCreateOperation* ico, xmlNode* n, char const* attrname, char const* attrtype,
	bool optional, char const* objattr)
{
	char* s = (char *)xmlGetProp(n, (const xmlChar*)attrname);
	if (s == NULL && optional) return true;
	if (s == NULL) {
		LOG_NO("SmfCampaignXmlParser::addAttribute: Fail to get property of non optional attribute %s, no value", attrname);
		return false;
	}

	SmfImmAttribute a;
	if (objattr != NULL)
		a.setName(objattr);
	else
		a.setName(attrname);
	a.setType(attrtype);
	a.addValue(s);
	ico->addValue(a);
	xmlFree(s);
	return true;
}

// ------------------------------------------------------------------------------
// parseSGType()
// ------------------------------------------------------------------------------

bool 
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
			if (s == NULL) {
				LOG_NO("SmfCampaignXmlParser::parseSGType: Parsing of saAmfSgtValidSuTypes fails, no value");
				return false;
			}
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

	return true;
}

// ------------------------------------------------------------------------------
// parseSUType()
// ------------------------------------------------------------------------------

bool 
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
			if (s == NULL){
				LOG_NO("Parsing of saAmfSutProvidesSvcType fails, no value");
				return false;
			}
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

	return true;
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
bool
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
			char* s = (char*)xmlNodeListGetString(m_doc, n->xmlChildrenNode, 1);
			if (s == NULL){
				LOG_NO("SmfCampaignXmlParser::elementToAttr: xmlTag %s found but no value", tag);
				return false;
			}
			attr.addValue(s);
			xmlFree(s);
			nElements++;
		}
	}
	if (nElements > 0) {
		ico->addValue(attr);
	} else if (optional != true) {
		LOG_NO("SmfCampaignXmlParser::elementToAttr: No elements in non optional attribute (%s)",attrname);
		return false;
	}

	return true;
}

// ------------------------------------------------------------------------------
// parseCompType()
// ------------------------------------------------------------------------------

bool 
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
			if (elementToAttr(ico, n, "cmdEnv", "saAmfCtDefCmdEnv", "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "instantiateCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag instantiateCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathInstantiateCmd", "SA_IMM_ATTR_SASTRINGT");
			if (elementToAttr(
				    ico, n, "cmdArgv", "saAmfCtDefInstantiateCmdArgv", 
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;			
			}

		} else if (strcmp((char*)n->name, "terminateCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag terminateCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathTerminateCmd", "SA_IMM_ATTR_SASTRINGT");
			if (elementToAttr(
				    ico, n, "cmdArgv", "saAmfCtDefTerminateCmdArgv",
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "cleanupCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag cleanupCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathCleanupCmd", "SA_IMM_ATTR_SASTRINGT");
			if (elementToAttr(
				    ico, n, "cmdArgv", "saAmfCtDefCleanupCmdArgv",
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "amStartCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag amStartCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathAmStartCmd", "SA_IMM_ATTR_SASTRINGT");
			if (elementToAttr(
				    ico, n, "cmdArgv", "saAmfCtDefAmStartCmdArgv",
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "amStopCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag amStopCmd found");
			addAttribute(
				ico, n, "saAmfCtRelPathAmStopCmd", "SA_IMM_ATTR_SASTRINGT");
			if (elementToAttr(
				    ico, n, "cmdArgv", "saAmfCtDefAmStopCmdArgv",
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "osafHcCmd") == 0 && n->ns == ns) {
			TRACE("xmlTag osafHcCmd found");
			addAttribute(
				ico, n, "osafAmfCtRelPathHcCmd", "SA_IMM_ATTR_SASTRINGT", true);
			if (elementToAttr(
				    ico, n, "cmdArgv", "osafAmfCtDefHcCmdArgv",
				    "SA_IMM_ATTR_SASTRINGT") == false){
				return false;
			}

		} else if (strcmp((char*)n->name, "healthCheck") == 0 && n->ns == ns) {
			TRACE("xmlTag healthCheck found");
			parseHealthCheck(i_campaign, n, dn.c_str());

		} else if (strcmp((char*)n->name, "swBundle") == 0 && n->ns == ns) {
			TRACE("xmlTag swBundle found");
			addAttribute(
				ico, n, "saAmfCtSwBundle", "SA_IMM_ATTR_SANAMET");

		}
	}

	return true;
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

bool 
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
			if (s == NULL){
				LOG_NO("SmfCampaignXmlParser::parseCSType: xmlTag csAttribute found but no value");
				return false;
			}
			attr.addValue(s);
			xmlFree(s);
			cnt++;
		}
	}
	if (cnt > 0) {
		ico->addValue(attr);
	}

	return true;
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

	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(m_actionId++);
			osafassert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampInitAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(m_actionId++);
			osafassert(iccb != 0);

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
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(m_actionId++);
			osafassert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampInitAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found");
			SmfCallbackAction *cba = new (std::nothrow) SmfCallbackAction(m_actionId++);
			osafassert(cba != 0);
			parseCallbackAction(cba, cur);
			SmfCallback & cbk = cba->getCallback();
			cbk.m_atAction = SmfCallback::atCampInitAction;
			i_campaign->addCampInitAction(cba);
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

	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
		if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(m_actionId++);
			osafassert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampCompleteAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(m_actionId++);
			osafassert(iccb != 0);

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
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(m_actionId++);
			osafassert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampCompleteAction(opa);
		}
		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found");
			SmfCallbackAction *cba = new (std::nothrow) SmfCallbackAction(m_actionId++);
			osafassert(cba != 0);
			parseCallbackAction(cba, cur);
			SmfCallback & cbk = cba->getCallback();
			cbk.m_atAction = SmfCallback::atCampCompleteAction;
			i_campaign->addCampCompleteAction(cba);
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

	while (cur != NULL) {
		//If this tag is found a do/undo pair is expected to be found
                if ((!strcmp((char *)cur->name, "doCliCommand")) && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			SmfCliCommandAction *cci = new(std::nothrow) SmfCliCommandAction(m_actionId++);
			osafassert(cci != 0);
			parseCliCommandAction(cci, cur);
			i_campaign->addCampWrapupAction(cci);
		}
		if ((!strcmp((char *)cur->name, "immCCB")) && (cur->ns == ns)) {
			TRACE("xmlTag immCCB found");
			SmfImmCcbAction *iccb = new(std::nothrow) SmfImmCcbAction(m_actionId++);
			osafassert(iccb != 0);

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
			SmfAdminOperationAction *opa = new(std::nothrow) SmfAdminOperationAction(m_actionId++);
			osafassert(opa != 0);
			parseAdminOpAction(opa, cur);
			i_campaign->addCampWrapupAction(opa);
		}

		if ((!strcmp((char *)cur->name, "callback")) && (cur->ns == ns)) {
			TRACE("xmlTag callback found");
			SmfCallbackAction *cba = new (std::nothrow) SmfCallbackAction(m_actionId++);
			osafassert(cba != 0);
			parseCallbackAction(cba, cur);
			SmfCallback & cbk = cba->getCallback();
			cbk.m_atAction = SmfCallback::atCampWrapupAction;
			i_campaign->addCampWrapupAction(cba);
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
		const std::string &dirpath = SmfCampaignThread::instance()->campaign()->getCampaignXmlDir();
		if ((!strcmp((char *)cur->name, "doCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag doCliCommand found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"command"))) {
				TRACE("command = %s", s);
				std::string cmd = replaceAllCopy(s, CAMPAIGN_ROOT_TAG, dirpath);
				TRACE("Modified command = %s", cmd.c_str());
				i_cmdAction->setDoCmd(cmd);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"args"))) {
				TRACE("args = %s", s);
				std::string str = replaceAllCopy(s, CAMPAIGN_ROOT_TAG, dirpath);
				TRACE("Modified args = %s", str.c_str());
				i_cmdAction->setDoCmdArgs(str);
				xmlFree(s);
			}
		}
		if ((!strcmp((char *)cur->name, "undoCliCommand"))
		    && (cur->ns == ns)) {
			TRACE("xmlTag undoCliCommand found");
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"command"))) {
				TRACE("command = %s", s);
				std::string cmd = replaceAllCopy(s, CAMPAIGN_ROOT_TAG, dirpath);
				TRACE("Modified command = %s", cmd.c_str());
				i_cmdAction->setUndoCmd(cmd);
				xmlFree(s);
			}
			if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"args"))) {
				TRACE("args = %s", s);
				std::string str = replaceAllCopy(s, CAMPAIGN_ROOT_TAG, dirpath);
				TRACE("Modified args = %s", str.c_str());
				i_cmdAction->setUndoCmdArgs(str);
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
			osafassert(ico != 0);
			parseImmCreate(ico, cur);

			//Add the create operation to the CCB
			i_ccbAction->addOperation(ico);	//Add an operation to the CCB
		}

		if ((!strcmp((char *)cur->name, "delete")) && (cur->ns == ns)) {
			TRACE("xmlTag delete found");
			SmfImmDeleteOperation *ido = new(std::nothrow) SmfImmDeleteOperation;
			osafassert(ido != 0);

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
			osafassert(imo != 0);

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

// ------------------------------------------------------------------------------
// parseCallbackOptions()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackOptions(SmfCallback* i_cbk, xmlNode * i_node)
{
	xmlNode *cur = i_node;
	char *s;

//#define SMF_UTIL_LABEL "OsafSmfCbkUtil-"

	TRACE_ENTER();
	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"callbackLabel"))) {
		TRACE("callback label = %s", s);
		i_cbk->m_callbackLabel = s;
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"stringToPass"))) {
		TRACE("args = %s", s);

		/* 
		   The reason to modify the "stringToPass" content is to be able to easy call a command
		   located in the sw bundle directory.If the "$OSAFCAMPAIGNROOT" token is found in the string 
		   it is replaced with the directory of the campaign xml file.
		   Normally the "$OSAFCAMPAIGNROOT" token is used in the "stringToPass" only when used to pass a string to
		   the OpenSAF built in SMF API client, which listenen to to callback labels starting with "OsafSmfCbkUtil-".
		   
		   Since the "stringToPass" is always modified it is also possible to use the "$OSAFCAMPAIGNROOT" tag for all 
		   "stringToPass" in any callbackLabel.
		*/
		const std::string &dirpath = SmfCampaignThread::instance()->campaign()->getCampaignXmlDir();
		std::string str = replaceAllCopy(s, CAMPAIGN_ROOT_TAG, dirpath);
		TRACE("Modified arg = %s", str.c_str());

		i_cbk->m_stringToPass = str.c_str();
		xmlFree(s);
	}
	if ((s = (char *)xmlGetProp(cur, (const xmlChar *)"time"))) {
		TRACE("args = %s", s);
		i_cbk->m_time = strtoll(s, NULL, 0);
		xmlFree(s);
	}
	TRACE_LEAVE();
}

// ------------------------------------------------------------------------------
// parseCallbackAction()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackAction(SmfCallbackAction * i_callbackAction, xmlNode * i_node)
{
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	TRACE_ENTER();
	SmfCallback& cbk = i_callbackAction->getCallback();
	
	while (cur != NULL) {
		if ((!strcmp((char *)i_node->name, "callback"))
		    && (i_node->ns == ns)) {
			
			TRACE("xmlTag callback found");
			parseCallbackOptions (&cbk, i_node);
		}
		cur = cur->next;
	}
	TRACE_LEAVE();
}
// ------------------------------------------------------------------------------
// parseCallbackAtInit()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackAtInit(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	SmfCallback *cbk = new SmfCallback();
	
	TRACE_ENTER();
	while (cur != NULL) {
		if ((!strcmp((char *)i_node->name, "callbackAtInit"))
		    && (i_node->ns == ns)) {
			
			TRACE("xmlTag callbackAtInit found");
			parseCallbackOptions (cbk, i_node);
		}
		cur = cur->next;
	}
	cbk->m_atAction = SmfCallback::atCampInit;
	i_campaign->getCampaignInit().addCallbackAtInit(cbk);
	TRACE_LEAVE();
}
// ------------------------------------------------------------------------------
// parseCallbackAtBackup()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackAtBackup(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	SmfCallback *cbk = new SmfCallback();
	
	TRACE_ENTER();
	while (cur != NULL) {
		if ((!strcmp((char *)i_node->name, "callbackAtBackup"))
		    && (i_node->ns == ns)) {
			
			TRACE("xmlTag callbackAtBackup found");
			parseCallbackOptions (cbk, i_node);
		}
		cur = cur->next;
	}
	cbk->m_atAction = SmfCallback::atCampBackup;
	i_campaign->getCampaignInit().addCallbackAtBackup(cbk);
	TRACE_LEAVE();
}
// ------------------------------------------------------------------------------
// parseCallbackAtRollback()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackAtRollback(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	SmfCallback *cbk = new SmfCallback();
	
	TRACE_ENTER();
	while (cur != NULL) {
		if ((!strcmp((char *)i_node->name, "callbackAtRollback"))
		    && (i_node->ns == ns)) {
			
			TRACE("xmlTag callbackAtRollback found");
			parseCallbackOptions (cbk, i_node);
		}
		cur = cur->next;
	}
	cbk->m_atAction = SmfCallback::atCampRollback;
	i_campaign->getCampaignInit().addCallbackAtRollback(cbk);
	TRACE_LEAVE();
}
// ------------------------------------------------------------------------------
// parseCallbackAtCommit()
// ------------------------------------------------------------------------------
void 
SmfCampaignXmlParser::parseCallbackAtCommit(SmfUpgradeCampaign * i_campaign, xmlNode * i_node)
{
	xmlNsPtr ns = 0;
	xmlNode *cur = i_node;
	SmfCallback *cbk = new SmfCallback();
	
	TRACE_ENTER();
	while (cur != NULL) {
		if ((!strcmp((char *)i_node->name, "callbackAtCommit"))
		    && (i_node->ns == ns)) {
			
			TRACE("xmlTag callbackAtCommit found");
			parseCallbackOptions (cbk, i_node);
		}
		cur = cur->next;
	}
	cbk->m_atAction = SmfCallback::atCampCommit;
	i_campaign->getCampaignWrapup().addCallbackAtCommit(cbk);
	TRACE_LEAVE();
}
