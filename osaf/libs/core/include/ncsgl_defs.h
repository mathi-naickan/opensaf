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
 * Author(s): Emerson Network Power
 *
 */

/*****************************************************************************
..............................................................................

    @@@@@  @               @@@@@@  @@@@@@@ @@@@@@@  @@@@@          @     @
   @     @ @               @     @ @       @       @     @         @     @
   @       @               @     @ @       @       @               @     @
   @  @@@@ @               @     @ @@@@@   @@@@@    @@@@@          @@@@@@@
   @     @ @               @     @ @       @             @   @@@   @     @
   @     @ @               @     @ @       @       @     @   @@@   @     @
    @@@@@  @@@@@@@ @@@@@@@ @@@@@@  @@@@@@@ @        @@@@@    @@@   @     @

..............................................................................

  DESCRIPTION:

  This module contains data type defs usable throughout the system.
  Inclusion of this module into all C-source modules is strongly
  recommended.

******************************************************************************
*/

/*
 * Module Inclusion Control...
 */
#ifndef NCSGL_DEFS_H
#define NCSGL_DEFS_H

#ifdef  __cplusplus
extern "C" {
#endif

/*****************************************************************************
 
                             BASIC DECLARATIONS

 ****************************************************************************/

/*
 * The following typedefs are dependent upon the target system processor and the
 * C-compiler. A change in the processor or the compiler *may* entail changes
 * to the following sections.
 */


/* Native typedef declarations.... */

	typedef unsigned char uns8;	/*  8-bit */
	typedef unsigned short uns16;	/* 16-bit */
	typedef unsigned int uns32;	/* 32-bit */
	typedef unsigned long long uns64;	/* 64-bit */

	typedef signed char int8;
	typedef signed short int16;
	typedef signed int int32;
	typedef long long int64;

	typedef float ncsfloat32;

	/* 
	 * support for double is dependent upon the target system and/or C-compiler.
	 * If your system does NOT support double, change this to uns32.
	 */
	typedef double DOUBLE;

	typedef uns16 NCS_VRID;	/* Virtual Router ID */

	typedef unsigned short cfgflag;	/* Usually a YES/NO or T/F boolean */
	typedef unsigned short cfgenum;	/* An enumerated value */

	typedef unsigned int NCS_BOOL;	/* move to this solves BOOLEAN problem */
	typedef unsigned char bcd;	/* Binary-Coded-Decimal */

	typedef void* NCSCONTEXT;	/* opaque context between svc-usr/svc-provider */

	typedef uns32 IE_DESC[2];	/* IE Descriptor for ATM Signallng */
	typedef uns32 FIE_DESC;	/* IE Descriptor for FR Signalling. */

	typedef uns32 ncs_oid;	/* Basic data type for ILMI Object Sub-ids... */

#define NCS_PTR_TO_INT32_CAST(x)   ((int32)(long)(x))
#define NCS_PTR_TO_UNS64_CAST(x)   ((uns64)(long)(x))
#define NCS_PTR_TO_UNS32_CAST(x)   ((uns32)(long)(x))
#define NCS_INT32_TO_PTR_CAST(x)   ((void*)(long)(x))
#define NCS_INT64_TO_PTR_CAST(x)   ((void*)(long)(x))
#define NCS_UNS32_TO_PTR_CAST(x)   ((void*)(long)(x))

	typedef uns32 NCS_IPV4_ADDR;
#if (NCS_IPV6 == 1)

#define NCS_IPV6_ADDR_UNS8_CNT    16	/* Number of uns8's in an IPv6 address */
#define NCS_IPV6_ADDR_UNS16_CNT   8	/* Number of uns16's in an IPv6 address */
#define NCS_IPV6_ADDR_UNS32_CNT   4	/* Number of uns32's in an IPv6 address */

	typedef struct ncs_ipv6_addr_tag {
		union {
			uns8 ipv6_addr8[NCS_IPV6_ADDR_UNS8_CNT];
			uns16 ipv6_addr16[NCS_IPV6_ADDR_UNS16_CNT];
			uns32 ipv6_addr32[NCS_IPV6_ADDR_UNS32_CNT];
		} ipv6;
	} NCS_IPV6_ADDR;

#define m_ipv6_addr              ipv6.ipv6_addr8
#define m_ipv6_addr16            ipv6.ipv6_addr16
#define m_ipv6_addr32            ipv6.ipv6_addr32
#define m_IPV6_ADDR_SIZE         sizeof (NCS_IPV6_ADDR)
#endif

/* prototype for registered function to 'Probe' protocol values */

	typedef void (*PROBER) (uns32 dir_flag, void *, const void *);

/* 
 * Define to allow .h files to be processed by C++ compiler.
 */
#if (!defined EXTERN_C)

#if (defined _cplusplus) || (defined __cplusplus)	/* C++ compilers define this */
#define EXTERN_C extern "C"
#else				/* not a C++ compiler */
#define EXTERN_C extern
#endif   /* _cplusplus */
#endif   /* EXTERN_C */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

        Manifest Constants

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#ifndef TRUE
#define TRUE    1
#endif

#ifndef FALSE
#define FALSE   0
#endif

#ifndef NULL
#define NULL    0
#endif

#ifndef EOF
#define EOF  (-1)
#endif

#ifndef YES
#define YES     1
#endif

#ifndef NO
#define NO      0
#endif

#ifndef ON
#define ON      1
#endif

#ifndef OFF
#define OFF     0
#endif

#ifndef ENABLE
#define ENABLE  1
#endif

#ifndef DISABLE
#define DISABLE 0
#endif

#ifndef RX
#define RX      1
#endif

#ifndef TX
#define TX      2
#endif

/* H&J Constants aligned with  Admin and Oper STATUS values */

	typedef enum ncs_status {
		NCS_STATUS_UP = 1,
		NCS_STATUS_DOWN = 2,
		NCS_STATUS_TESTING = 3,
		NCS_STATUS_UNKNOWN = 4,
		NCS_STATUS_DORMANT = 5,
		NCS_STATUS_NOT_PRESENT = 6,
		NCS_STATUS_LL_DOWN = 7
	} NCS_STATUS;

/* X.731 admin state values */
	typedef enum ncs_admin_state_tag {
		NCS_ADMIN_STATE_LOCK = 1,
		NCS_ADMIN_STATE_UNLOCK,
		NCS_ADMIN_STATE_SHUTDOWN
	} NCS_ADMIN_STATE;

/* X.731 operational state values */
	typedef enum ncs_oper_state_tag {
		NCS_OPER_STATE_ENABLE = 1,
		NCS_OPER_STATE_DISABLE
	} NCS_OPER_STATE;

/* X.731 usage state values */
	typedef enum ncs_usage_state_tag {
		NCS_USAGE_STATE_IDLE = 1,
		NCS_USAGE_STATE_ACTIVE,
		NCS_USAGE_STATE_BUSY
	} NCS_USAGE_STATE;

/* Format types, proprietory to NCS), 
   used in EDU */
	typedef enum ncs_edu_fmat_tag {
		NCS_EDU_FMAT_8BIT,	/* 8 bit    */
		NCS_EDU_FMAT_16BIT,	/* 16 bit   */
		NCS_EDU_FMAT_32BIT,	/* 32 bit   */
		NCS_EDU_FMAT_64BIT,	/* 64 bit   */
		NCS_EDU_FMAT_OCT,	/* Octet */
		NCS_EDU_FMAT_CNT	/* Count of instances */
	} NCS_EDU_FMAT;

/* type definition for SNMP-like IFINDEX, an abstraction for an interface */

	typedef unsigned int NCS_IFINDEX;

/* type definition for SNMP-like TUNNEL ID, an abstraction for a tunnel */

	typedef unsigned int NCS_TUNNEL_ID;

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                      Generic Function Return Codes                     

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

/** Return Codes shared across the entire H&J product line...
    Reserved Area: 0-thru-1023
 **/

#define NCSCC_RC_SUCCESS               1	/* All went well -success           */
#define NCSCC_RC_FAILURE               2	/* Unspecified failure              */

#define NCSCC_RC_UNK_CONTEXT           3
#define NCSCC_RC_BUSY_PROF             4	/* Profile is in use                */
#define NCSCC_RC_UNK_PROF              5	/* Unknown User Profile             */
#define NCSCC_RC_UNK_ATTR              6	/* Unknown profile attribute type   */
#define NCSCC_RC_BAD_ATTR              7	/* Attribute has bad format         */
#define NCSCC_RC_NO_ATTR               8	/* Attribute not present            */
#define NCSCC_RC_UNK_OP                9	/* Operation type unknown           */

#define NCSCC_RC_CALL_ACCEPT          11
#define NCSCC_RC_CALL_RELEASE         12
#define NCSCC_RC_CALL_PEND            13

#define NCSCC_RC_SKIP_MSG             14
#define NCSCC_RC_CONT_MSG             15

#define NCSCC_RC_INV_VAL              16
#define NCSCC_RC_INV_SPECIFIC_VAL     17

#define NCSCC_RC_NO_OBJECT            18
#define NCSCC_RC_NO_INSTANCE          19
#define NCSCC_RC_NOSUCHNAME           20

#define NCSCC_RC_OUT_OF_MEM           21	/* Out of memory                    */
#define NCSCC_RC_CALL_GATED           22	/* NCSCC_CALL_DATA passed on         */
#define NCSCC_RC_SIG_FREE_CD          23	/* Signalling alloc'ed the call data */
						/* Signalling should free it        */

#define NCSCC_RC_SIG_CLEARS_ALL       24	/* Signalling drives iface clearing */
#define NCSCC_RC_USR_CLEARS_ALL       25	/* Application drives iface clearing */
#define NCSCC_RC_EACH_CLEARS_OWN      26	/* each (sig/app) clears own world  */

#define NCSCC_RC_CAPD_NOT_CHANGED     31	/* Attachment point not changed     */
#define NCSCC_RC_CAPD_CHANGED         32	/* Attachment point changed         */
#define NCSCC_RC_NOT_SUPPORTED        33	/* Request not supported            */

#define NCSCC_RC_INTF_NOT_UP          34
#define NCSCC_RC_DUPLICATE_FLTR       35
#define NCSCC_RC_INVALID_PING_TIMEOUT 36

#define NCSCC_RC_ENQUEUED            125
#define NCSCC_RC_INOPPORTUNE_REQUEST 126
#define NCSCC_RC_DISABLED  127

#define NCSCC_RC_TMR_STOPPED         128
#define NCSCC_RC_TMR_DESTROYED       129

#define NCSCC_RC_NO_SUCH_TBL         130
#define NCSCC_RC_REQ_TIMOUT          131

#define NCSCC_RC_NO_TO_SVC           132

#define NCSCC_RC_NO_ACCESS           133
#define NCSCC_RC_NO_CREATION         134
#define NCSCC_RC_NO_MAS              135
#define NCSCC_RC_PLBK_IN_PROGRESS    136
#define NCSCC_RC_INVALID_INPUT       137	/* Input is invalid */
#define NCSCC_RC_NOT_WRITABLE        138
/* NCSCC_RC_NOT_WRITABLE is mapped to SNMP_ERR_NOTWRITABLE. If the variable binding's name specifies a variable
   which exists but can not be modified no matter what new value is specified, then the value of the Response-PDU's 
   error-status field is set to `notWritable' (set on read-only, not-accessible objects returns this error)
*/

#define NCSCC_RC_CONTINUE           1023

/* MPOA Return Codes */

#define NCSCC_RC_DUPLICATE_ENTRY                2011
#define NCSCC_RC_TOO_MANY_MPS                   2012
#define NCSCC_RC_BAD_LEC                        2013
#define NCSCC_RC_ENCAP_TOO_LONG                 2014
#define NCSCC_RC_TOO_MANY_PROTOS                2015
#define NCSCC_RC_VCC_RCV_BUSY                   2016
#define NCSCC_RC_PDU_RUNT                       2017
#define NCSCC_RC_PDU_TOO_BIG                    2018
#define NCSCC_RC_UNSUPPORTED_PROTOCOL           2019
#define NCSCC_RC_DROP_PACKET                    2020
#define NCSCC_RC_SEND_VIA_LANE                  2021
#define NCSCC_RC_PKT_TOO_SHORT                  2022
#define NCSCC_RC_PKT_NOT_NHRP                   2023
#define NCSCC_RC_NHRP_BAD_PKT_TYPE              2024

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                      Product Specific Function Return Code Bases

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define NCSCC_RC_SSS_BASE      1025	/* up to  2048 */
#define NCSCC_RC_ILMI_BASE     2049	/* up to  2399 */
#define NCSCC_RC_FRS_BASE      2400	/* up to  3072 */
#define NCSCC_RC_LEC_BASE      3073	/* up to  4096 */
#define NCSCC_RC_IPOA_BASE     4097	/* up to  5120 */
#define NCSCC_RC_NCSLES_BASE    5121	/* up to  6144 */
#define NCSCC_RC_FRF5_BASE     6145	/* up to  7168 */
#define NCSCC_RC_PNNI_BASE     7169	/* up to  8192 */
#define NCSCC_RC_FRF8_BASE     8193	/* up to  9216 */
	/* unexplained gap here! */
#define NCSCC_RC_CMS_BASE     11265	/* up to 12288 */
#define NCSCC_RC_LTCS_BASE    12289	/* up to 13312 */
#define NCSCC_RC_RMS_BASE     13313	/* up to 14336 */
#define NCSCC_RC_MDS_BASE     14337	/* up to 15360 */
#define NCSCC_RC_BGP_BASE     15361	/* up to 15450 */
#define NCSCC_RC_VPN_BASE     15451	/* up to 15500 */
#define NCSCC_RC_IGMP_BASE    15501	/* up to 15550 */
#define NCSCC_RC_PIM_BASE     15551	/* up to 15600 */

/*@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

                    MACROS

@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@*/

#define NP_MAX_FILENAME_LEN         32
#define NP_MAX_PATHNAME_LEN         128

#define m_tst(v,f)  ((v) & (f))
#define m_set(v,f)  ((v) |= (f))
#define m_rst(v,f)  ((v) &= ~(f))
#define m_tog(v,f)  ((v) ^= (f))

/*************************************************
 * Maximum Slots (Including sub slots) supported
 * 16 slots x 8 subslots
 *************************************************/
#define NCS_SLOT_MAX 16

#define NCS_SUB_SLOT_MAX 16

#define NCS_MAX_SLOTS ((NCS_SLOT_MAX *  NCS_SUB_SLOT_MAX) + NCS_SLOT_MAX)

/***************************************************************************
 * The IP address definations are removed from this file, and moved into the 
 * followng include files.
 * Including this files here is a work around to avoide compilation errors in 
 * all subsytems that are using IP address definations. Going forward all 
 * subsystems need to include the following include files seperately along with
 * gl_defs.h, and remove the include defenations from this file 
 *****************************************************************************/

/* NEEDS_TO_BE_REMOVED_FROM_LEAP */
#if (NCS_IP_SERVICES==1)
#include "ncs_iplib.h"

	/* The below two includes were added to overcome the issues
	   rely upon LEAP to include the files they need. i.e. below
	 */
#include "ncs_ipprm.h"		/* Req'd for primitive interfaces */
#include "ncs_ip.h"
#endif

#ifndef NCS_SAF
#define NCS_SAF  0
#endif

#define NCS_MDS  1
#define NCS_CLI  1

#define NCS_IPV6 0
#define NCS_MTREE_PAT_STYLE_SUPPORTED 1

#include "ncs_osprm.h"

	typedef uns64 MDS_DEST;

	typedef uns32 NCS_NODE_ID;
	typedef uns8 NCS_CHASSIS_ID;
	typedef uns8 NCS_PHY_SLOT_ID;
	typedef uns8 NCS_SUB_SLOT_ID;

/* m_NCS_NODE_ID_FROM_MDS_DEST: Returns node-id if the MDS_DEST provided
                                is an absolute destination. Returns 0
                                if the MDS_DEST provided is a virtual
                                destination.
*/
#define m_NCS_NODE_ID_FROM_MDS_DEST(mdsdest) ((uns32) (((uns64)(mdsdest))>>32))

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

#ifndef HOST_NAME_MAX
#define HOST_NAME_MAX 64
#endif

#ifndef PTHREAD_MUTEX_RECURSIVE
#define PTHREAD_MUTEX_RECURSIVE 1
#endif

#ifdef  __cplusplus
}
#endif

#endif   /* ifndef NCSGL_DEFS_H */
