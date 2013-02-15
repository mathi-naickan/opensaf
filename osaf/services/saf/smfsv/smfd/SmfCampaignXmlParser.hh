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

#ifndef __SMFCAMPAIGNXMLPARSER_HH
#define __SMFCAMPAIGNXMLPARSER_HH

#define SMF_BUNDLE_CLASS_NAME "SaSmfSwBundle"
#define SMF_BUNDLE_PARENT_DN  "safRepository=smfRepository,safApp=safSmfService"
#define CAMPAIGN_ROOT_TAG     "$OSAFCAMPAIGNROOT"

/* ========================================================================
 *   INCLUDE FILES
 * ========================================================================
 */
#include <vector>
#include <string>
#include <libxml/xpath.h>
#include "SmfCallback.hh"

class SmfUpgradeCampaign;
class SmfUpgradeProcedure;
class SmfRollingUpgrade;
class SmfByTemplate;
class SmfTargetNodeTemplate;
class SmfTargetEntityTemplate;
class SmfTargetNodeTemplate;
class SmfBundleRef;
class SmfSinglestepUpgrade;
class SmfForAddRemove;
class SmfForModify;
class SmfActivationUnitType;
class SmfImmOperation;
class SmfImmModifyOperation;
class SmfImmCreateOperation;
class SmfCliCommandAction;
class SmfImmCcbAction;
class SmfAdminOperationAction;
class SmfEntity;
class SmfCallback;
class SmfUpgradeMethod;
class SmfPlmExecEnv;
class SmfCallbackAction;

///
/// Parses the campaign xml file and build a structure of objects found in the file.
///

class SmfCampaignXmlParser {
 public:

///
/// The constructor
/// @param    None
/// @return   None
///
	SmfCampaignXmlParser();

///
/// The dDestructor
/// @param    None
/// @return   None
///
	~SmfCampaignXmlParser();

///
/// Gets the current class name.
/// @return   A std::string containing the name of this class.
///
	std::string getClassName() const;

///
/// Gets a string representation of this object.
/// @return   The state of this class members expressed by means of a string representation.
///
	 std::string toString() const;

///
/// Convert a string fetched from the campaign xml to a IMM SaImmValueTypeT enum 
/// @return     A SaImmValueTypeT enum
///    
	const SaImmValueTypeT stringToImmType(std::string i_type) const;

///
/// Prepare a CreateOperation for a node.
/// It assumes that the RDN attribute is an attribute in the node with the correct name.
/// @param parent Parent DN or NULL.
/// @param className The IMM class-name for the object.
/// @param node The XML node.
/// @param rdnAttribute The name of the attribute to use as RDN.
/// @param dn Out-parameter. The DN in the form; rdn ',' parent_dn
/// @return A SmfImmCreateOperation with parent, class and RDN-attribute set.
///

	SmfImmCreateOperation* prepareCreateOperation(
		char const* parent, char const* className, xmlNode* node, char const* rdnAttribute,
                std::string& dn, bool isnamet = false);

///
/// Read an attribute from the XML node and add it to a newly created object.
/// @param ico An ongoing create operation
/// @param n The node
/// @param attrname The attribute name
/// @param attrtype The attribute type
/// @param optional True if the attribute is optional
/// @param objattr Non-NULL if the attribute name differs in the node and
///   the object.
///
	bool addAttribute(
		SmfImmCreateOperation* ico, xmlNode* n, char const* attrname, 
                char const* attrtype, bool optional = false,
                char const* objattr = NULL);

///
/// Scan for XML elements with the passed tag. The contents of the elements
/// are added to the multi-value attribute passed. This function is used to
/// parse "cmdEnv" and "cmdArgv" and alike.
/// @param ico An ongoing create operation
/// @param node The node to scan
/// @param tag The element tag to scan for
/// @param attrname The attribute name
/// @param attrtype The attribute type
///

	bool elementToAttr(
		SmfImmCreateOperation* ico, xmlNode* node, 
		char const* tag, char const* attrname, char const* attrtype,
		bool optional = true);

///
/// Purpose: Parse the campaignXML file and put the content in the upgrade campaign
/// @param   i_file A string containing the name of the file including full path.
/// @return  A pointer to a campaign object.
///
	SmfUpgradeCampaign *parseCampaignXml(std::string i_file);

///
/// Purpose: Parse the campaign properties part of the campaign.
/// @param   i_campaign A pointer to the campaign object.
/// @param   i_node The xml node where the campaignproperties tag was found.
/// @return  True if successful, otherwise false
///
	bool parseCampaignProperties(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse the campaigninfo part of the campaign.
/// @param   i_campaign A pointer to the campaign object.
/// @param   i_node The xml node where the campaigninfo tag was found.
/// @return  True if successful, otherwise false.
///
	bool parseCampaignInfo(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse the opgradeProcedure part of the campaign   
/// @param   io_up  A pointer to the procedure object.
/// @param   i_node The xml node where the campaigninfo tag was found.
/// @return  Bool true if successful, otherwise false
///
	bool parseUpgradeProcedure(SmfUpgradeProcedure * io_up, xmlNode * i_node);

///
/// Purpose: Parse the procedure estimated time   
/// @param   io_proc  A pointer to the procedure object.
/// @param   i_node The xml node where the outageinfo tag (parent) was found.
/// @return  None
///
	void parseOutageInfo(SmfUpgradeProcedure * i_proc, xmlNode * i_node);

///
/// Purpose: Parse the procedure init actions part of the campaign   
/// @param   io_proc  A pointer to the procedure object.
/// @param   i_node The xml node where the campaigninfo tag was found.
/// @return  None
///
	void parseProcInitAction(SmfUpgradeProcedure * i_proc, xmlNode * i_node);

///
/// Purpose: Parse the procedure wrapup actions part of the campaign   
/// @param   i_proc  A pointer to the procedure object.
/// @param   i_node The xml node where the campaigninfo tag was found.
/// @return  None
///
	void parseProcWrapupAction(SmfUpgradeProcedure * i_proc, xmlNode * i_node);

///
/// Purpose: Parse the rollingUpgrade part of the campaign   
/// @param   a_rolling  A pointer to the SmfRollingUpgrade object.
/// @param   i_node The xml node where the rollingUpgrade tag was found.
/// @return  Bool true if successful, otherwise false
///
	bool parseRollingUpgrade(SmfRollingUpgrade * a_rolling, xmlNode * a_node);

///
/// Purpose: Parse the byTemplate part of the campaign   
/// @param   a_templ  A pointer to the SmfByTemplate object.
/// @param   a_node The xml node where the byTemplate tag was found.
/// @return  Bool true if successful, otherwise false
///
	bool parseByTemplate(SmfByTemplate * a_templ, xmlNode * a_node);

///
/// Purpose: Parse the targetNodeTemplate part of the campaign   
/// @param   a_templ  A pointer to the SmfTargetNodeTemplate object.
/// @param   a_node The xml node where the targetNodeTemplate tag was found.
/// @return  None
///
	void parseTargetNodeTemplate(SmfTargetNodeTemplate * a_templ, xmlNode * a_node);

///
/// Purpose: Parse the targetNodeTemplate part of the campaign   
/// @param   a_templ  A pointer to the SmfTargetEntityTemplate object.
/// @param   a_node The xml node where the targetEntityTemplate tag was found.
/// @return  None
///
	void parseTargetEntityTemplate(SmfTargetEntityTemplate * a_templ, xmlNode * a_node);

///
/// Purpose: Parse the parent/type elements under targetNodeTemplate
/// @param   io_templ  A pointer to the SmfTargetNodeTemplate object.
/// @param   i_node The xml node where the targetNodeTemplate tag was found.
/// @return  None
///
	void parseParentTypeElements(SmfTargetNodeTemplate * io_templ, xmlNode * i_node);

///
/// Purpose: Parse a bundle reference
/// @param   a_br A pointer to a SmfBundleRef object.
/// @param   a_node The xml node where the bundle reference tag was found.
/// @return  None
///
	void parseBundleRef(SmfBundleRef * a_br, xmlNode * a_node);

///
/// Purpose: Parse PlmExecEnv information
/// @param   plmexecenv_list The parsed object is added to this list.
/// @param   node The xml node where the softwareBundle tag was found.
/// @return  None
///
	void parsePlmExecEnv(std::list<SmfPlmExecEnv>& plmexecenv_list, xmlNode* node);

///
/// Purpose: Parse an attribute for an IMM modify operation
/// @param   io_immo A pointer to a SmfImmOperation object.
/// @param   i_node The xml node where the attribute tag was found.
/// @return  None
///
	void parseAttribute(SmfImmOperation * io_immo, xmlNode * i_node);

///
/// Purpose: Parse the step counter
/// @param   i_node The xml node
/// @param   o_stepcount, the value of onEveryStep/onFirstStep/onLastStep/halfWay
/// @return  Bool true if successful, otherwise false.
///
	bool parseStepCount(xmlNode* node, SmfCallback::StepCountT& o_stepcount);

///
/// Purpose: Parse the at action tag
/// @param   i_node The xml node
/// @param   o_ataction, the value of beforeTermination/afterImmModification/afterInstantiation/afterUnlock
/// @return  Bool true if successful, otherwise false.
///
	bool parseAtAction(xmlNode* node,  SmfCallback::AtActionT& o_ataction);

///
/// Purpose: Parse Callback items. In the campaign these are pairs; customizationTime/callback
///   for rolling-upgrade and atAction/callback for singlestep-upgrade.
/// @param   upgrade The Upgrade object that holds the callback list
/// @param   node The first xml node to search for callbacks. Note; the sibling to this node
///   are searched, not the children.
/// @return  Bool true if successful, otherwise false.
///
	bool parseCallback(SmfUpgradeMethod* upgrade, xmlNode* node);

///
/// Purpose: Parse the singlestepUpgrade part of the campaign   
/// @param   a_single  A pointer to the SmfSinglestepUpgrade object.
/// @param   i_node The xml node where the singleStepUpgrade tag was found.
/// @return  Bool true if successful, otherwise false
///
	bool parseSinglestepUpgrade(SmfSinglestepUpgrade* a_single, xmlNode* i_node);

///
/// Purpose: Parse the ForAddRemove part of a single-step upgrade.
/// @param   scope  A pointer to the SmfForAddRemove object.
/// @param   a_node The xml node where the forAddRemove tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseForAddRemove(SmfForAddRemove *scope, xmlNode *node);

///
/// Purpose: Parse the ForModify part of a single-step upgrade.
/// @param   scope  A pointer to the SmfModify object.
/// @param   a_node The xml node where the forModify tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseForModify(SmfForModify *scope, xmlNode *node);

///
/// Purpose: Parse the (De-)ActivationUnit part of a single-step upgrade.
/// @param   scope  A pointer to the SmfActivationUnit object.
/// @param   node The xml node where the tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseActivationUnit(SmfActivationUnitType *scope, xmlNode *node);

///
/// Purpose: Parse a Entity List (part of a single-step upgrade).
/// @param   entityList  A reference to a list of Entities.
/// @param   node The xml node where the tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseEntityList(std::list<SmfEntity>& entityList, xmlNode *node);

///
/// Purpose: Parse the campaign init section
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the campaignInitialization tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseCampaignInitialization(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse the campaign wrapup section
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the campaignWrapup tag was found.
/// @return  None
///
	void parseCampaignWrapup(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse addToImm in the campaignInitialization section
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the addToImm tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseAddToImm(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse removeFromImm in the campaign campaignWrapup section
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the removeFromImm tag was found.
/// @return  None
///
	void parseRemoveFromImm(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse software bundle information
/// @param   i_createOper A pointer to a SmfImmCreateOperation object.
/// @param   i_node The xml node where the softwareBundle tag was found.
/// @return  None
///
	void parseSoftwareBundle(SmfImmCreateOperation * i_createOper, xmlNode * i_node);

/// Purpose: Parse Amf Entity Types
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @return  Bool true if successful, otherwise false.
///
	bool parseAmfEntityTypes(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node);

/// Purpose: Parse AppType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  Bool true if successful, otherwise false.
///
	bool parseAppType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse SGType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  Bool true if successful, otherwise false.
///
	bool parseSGType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse SUType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  Bool true if successful, otherwise false.
///
	bool parseSUType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse componentType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  None
///
	void parseComponentType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse CompType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  Bool true if successful, otherwise false.
///
	bool parseCompType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse ProvidesCSType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  None
///
	void parseProvidesCSType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse HealthCheck
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  None
///
	void parseHealthCheck(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse ServiceType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  None
///
	void parseServiceType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse CsType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  None
///
	void parseCsType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

/// Purpose: Parse CSType
/// @param   i_campaign A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the tag was found.
/// @param   parent The Parent DN
/// @return  Bool true if successful, otherwise false.
///
	bool parseCSType(
		SmfUpgradeCampaign* i_campaign, xmlNode * i_node, 
		char const* parent);

///
/// Purpose: Parse campaign init actions
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the campInitAction tag was found.
/// @return  None
///
	void parseCampInitAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse campaign complete actions.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the campCommitAction tag was found.
/// @return  None
///
	void parseCampCompleteAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse campaign wrapup actions.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node where the campWrapupAction tag was found.
/// @return  None
///
	void parseCampWrapupAction(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

///
/// Purpose: Parse a cli command action.
/// @param   i_campaign  A pointer to a SmfCliCommandAction object.
/// @param   i_node The xml node where the doCliCommand tag was found.
/// @return  None
///
	void parseCliCommandAction(SmfCliCommandAction * i_cmdAction, xmlNode * i_node);

///
/// Purpose: Parse an immCcb.
/// @param   i_ccbAction  A pointer to a SmfImmCcbAction object.
/// @param   i_node The xml node where an immCCB tag was found.
/// @return  None
///
	void parseImmCcb(SmfImmCcbAction * i_ccbAction, xmlNode * i_node);

///
/// Purpose: Parse an ImmCreate.
/// @param   i_createop  A pointer to a SmfImmCreateOperation object.
/// @param   i_node The xml node
/// @return  None
///
	void parseImmCreate(SmfImmCreateOperation* i_createop, xmlNode* i_node);


///
/// Purpose: Parse an admin operation.
/// @param   i_admOpAction  A pointer to a SmfAdmOpAction object.
/// @param   i_node The xml node where an immCCB tag was found.
/// @return  None
///
	void parseAdminOpAction(SmfAdminOperationAction * i_admOpAction, xmlNode * i_node);

///
/// Purpose: Parse callback action.
/// @param   i_callbackAction  A pointer to a SmfCallbackAction object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackAction(SmfCallbackAction * i_callbackAction, xmlNode * i_node);

///
/// Purpose: Parse parseCallbackAtInit.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackAtInit(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

/// Purpose: Parse parseCallbackAtBackup.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackAtBackup(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

/// Purpose: Parse parseCallbackAtRollback.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackAtRollback(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

/// Purpose: Parse parseCallbackAtCommit.
/// @param   i_campaign  A pointer to a SmfUpgradeCampaign object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackAtCommit(SmfUpgradeCampaign * i_campaign, xmlNode * i_node);

/// Purpose: Parse parseCallbackOptions.
/// @param   i_callback  A pointer to a SmfCallback object.
/// @param   i_node The xml node 
/// @return  None
///
	void parseCallbackOptions(SmfCallback * i_callback, xmlNode * i_node);
 private:
///
/// Purpose:  Disables copy constructor.
///
	 SmfCampaignXmlParser(const SmfCampaignXmlParser &);

///
/// Purpose:  Disables assignment operator.
///
	 SmfCampaignXmlParser & operator=(const SmfCampaignXmlParser &);

	xmlDocPtr m_doc;
	xmlXPathContextPtr m_xpathCtx;
	xmlXPathObjectPtr m_xpathObj;
        unsigned int m_actionId;
};

#endif				// __SMFCAMPAIGNXMLREADER_H
