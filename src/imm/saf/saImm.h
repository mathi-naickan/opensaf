/******************************************************************************
**
** FILE:
**   saImm.h
**
** DESCRIPTION:
**   This file provides the C language binding for the Service
**   Availability(TM) Forum Information Model Management Service (IMM).
**   It contains all the prototypes and type definitions required
**   by the IMM Object Management and Object Implementer APIs.
**
** SPECIFICATION VERSION:
**   SAI-AIS-IMM-A.02.01
**
** DATE:
**   Thurs  June   28  2007
**
** LEGAL:
**   OWNERSHIP OF SPECIFICATION AND COPYRIGHTS.
**   The Specification and all worldwide copyrights therein are
**   the exclusive property of Licensor.  You may not remove, obscure, or
**   alter any copyright or other proprietary rights notices that are in or
**   on the copy of the Specification you download.  You must reproduce all
**   such notices on all copies of the Specification you make.  Licensor
**   may make changes to the Specification, or to items referenced therein,
**   at any time without notice.  Licensor is not obligated to support or
**   update the Specification.
**
**   Copyright(c) 2007, Service Availability(TM) Forum. All rights
**   reserved.
**
******************************************************************************/
#include <saAis.h>

#ifndef _SAIMM_H
#define _SAIMM_H

#ifdef  __cplusplus
extern "C" {
#endif

	/* 4.2.2 Various IMM Service Names */
	typedef SaStringT SaImmClassNameT;
	typedef SaStringT SaImmAttrNameT;
	typedef SaStringT SaImmAdminOwnerNameT;

	/* 4.2.3 SaImmValueTypeT */
	typedef enum {
		SA_IMM_ATTR_SAINT32T = 1,	/* SaInt32T */
		SA_IMM_ATTR_SAUINT32T = 2,	/* SaUint32T */
		SA_IMM_ATTR_SAINT64T = 3,	/* SaInt64T */
		SA_IMM_ATTR_SAUINT64T = 4,	/* SaUint64T */
		SA_IMM_ATTR_SATIMET = 5,	/* SaTimeT */
		SA_IMM_ATTR_SANAMET = 6,	/* SaNameT */
		SA_IMM_ATTR_SAFLOATT = 7,	/* SaFloatT */
		SA_IMM_ATTR_SADOUBLET = 8,	/* SaDoubleT */
		SA_IMM_ATTR_SASTRINGT = 9,	/* SaStringT */
		SA_IMM_ATTR_SAANYT = 10	/* SaAnyT */
	} SaImmValueTypeT;

	/* 4.2.4 SaImmClassCategoryT */
	typedef enum {
		SA_IMM_CLASS_CONFIG = 1,
		SA_IMM_CLASS_RUNTIME = 2
	} SaImmClassCategoryT;

	/* 4.2.5 SaImmAttrFlagsT */
#define SA_IMM_ATTR_MULTI_VALUE   0x00000001
#define SA_IMM_ATTR_RDN           0x00000002
#define SA_IMM_ATTR_CONFIG        0x00000100
#define SA_IMM_ATTR_WRITABLE      0x00000200
#define SA_IMM_ATTR_INITIALIZED   0x00000400
#define SA_IMM_ATTR_RUNTIME       0x00010000
#define SA_IMM_ATTR_PERSISTENT    0x00020000
#define SA_IMM_ATTR_CACHED        0x00040000
	typedef SaUint64T SaImmAttrFlagsT;

	/* 4.2.6 SaImmAttrValueT */
	typedef void *SaImmAttrValueT;

	/* 4.2.7 SaImmAttrDefinitionT */
	typedef struct {
		SaImmAttrNameT attrName;
		SaImmValueTypeT attrValueType;
		SaImmAttrFlagsT attrFlags;
		SaImmAttrValueT attrDefaultValue;
	} SaImmAttrDefinitionT_2;

	/* 4.2.8 SaImmAttrValuesT */
	typedef struct {
		SaImmAttrNameT attrName;
		SaImmValueTypeT attrValueType;
		SaUint32T attrValuesNumber;
		SaImmAttrValueT *attrValues;
	} SaImmAttrValuesT_2;

	/* 4.2.9 SaImmAttrModificationTypeT */
	typedef enum {
		SA_IMM_ATTR_VALUES_ADD = 1,
		SA_IMM_ATTR_VALUES_DELETE = 2,
		SA_IMM_ATTR_VALUES_REPLACE = 3
	} SaImmAttrModificationTypeT;

	/* 4.2.10 SaImmAttrModificationT */
	typedef struct {
		SaImmAttrModificationTypeT modType;
		SaImmAttrValuesT_2 modAttr;
	} SaImmAttrModificationT_2;

	/* 4.2.11 SaImmScopeT */
	typedef enum {
		SA_IMM_ONE = 1,
		SA_IMM_SUBLEVEL = 2,
		SA_IMM_SUBTREE = 3
	} SaImmScopeT;

	/* 4.2.12 SaImmSearchOptionsT */
#define SA_IMM_SEARCH_ONE_ATTR       0x0001
#define SA_IMM_SEARCH_GET_ALL_ATTR   0x0100
#define SA_IMM_SEARCH_GET_NO_ATTR    0x0200
#define SA_IMM_SEARCH_GET_SOME_ATTR  0x0400
	typedef SaUint64T SaImmSearchOptionsT;

	/* 4.2.13 SaImmSearchParametersT */
	typedef struct {
		SaImmAttrNameT attrName;
		SaImmValueTypeT attrValueType;
		SaImmAttrValueT attrValue;
	} SaImmSearchOneAttrT_2;

	typedef union {
		SaImmSearchOneAttrT_2 searchOneAttr;
	} SaImmSearchParametersT_2;

	/* 4.2.14 SaImmCcbFlagsT */
#define SA_IMM_CCB_REGISTERED_OI 0x00000001
	typedef SaUint64T SaImmCcbFlagsT;

	/* 4.2.15  SaImmContinuationIdT */

	typedef SaUint64T SaImmContinuationIdT;

	/* 4.2.16 SaImmAdminOperationIdT */
	typedef SaUint64T SaImmAdminOperationIdT;

	/* 4.2.17 SaImmAdminOperationParamsT */

	typedef struct {
		SaStringT paramName;
		SaImmValueTypeT paramType;
		SaImmAttrValueT paramBuffer;
	} SaImmAdminOperationParamsT_2;

#ifdef  __cplusplus
}
#endif

#endif   /* _SA_IMM_H */
