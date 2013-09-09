/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2013 The OpenSAF Foundation
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
#ifndef TEST_NTF_IMCN_H
#define	TEST_NTF_IMCN_H

#ifdef	__cplusplus
extern "C" {
#endif

	typedef struct {
		SaBoolT populated;
		SaNtfEventTypeT evType;
		SaNtfObjectCreateDeleteNotificationT* c_d_notif_ptr;
		SaNtfAttributeChangeNotificationT* a_c_notif_ptr;
		SaNtfStateChangeNotificationT* o_s_c_notif_ptr;
		SaNtfNotificationHandleT nHandle;
		SaNtfElementIdT ccbInfoId;
	} NotifData;

	typedef struct {
		const char* attrName;
		SaImmValueTypeT attrType;
		void** attrValues;
	} TestAttributeValue;

	const SaUint8T BUF1[] = {
		'0', '9', '0', '8', '0', '7', '0', '6', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '9', '0', '8', '0', '7', '0', '6', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '9', '0', '8', '0', '7', '0', '6', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1'
	};
	const SaUint8T BUF2[] = {
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '7', '0', '7', '0', '7', '0', '6', '0', '6', '0', '4', '0', '3', '0', '2', '0', '2', '0', '1'
	};
	const SaUint8T BUF3[] = {
		'0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1', '0', '1'
	};
	const SaUint8T NAME1[] = {
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1'
	};
	const SaUint8T NAME2[] = {
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '7', '0', '7', '0', '7', '0', '6', '0', '6', '0', '4', '0', '3', '0', '2', '0', '2', '0', '1'
	};
	const SaUint8T NAME3[] = {
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1',
		'0', '9', '0', '8', '0', '1', '0', '1', '0', '5', '0', '4', '0', '3', '0', '2', '0', '1', '0', '1'
	};

	// Never-Used-Anywhere-But-Must-Be-Declared-Anyway-Beats-Me-Why
	const SaUint8T NUABMBDABMW[] = {0};

	const SaInt32T INT32VAR1 = -32;
	const SaUint32T UINT32VAR1 = 32;
	const SaInt64T INT64VAR1 = -64;
	const SaUint64T UINT64VAR1 = 64;
	const SaTimeT TIMEVAR1 = 12345;
	const SaFloatT FLOATVAR1 = 3.140000;
	const SaDoubleT DOUBLEVAR1 = 2.712200;
	const SaStringT STRINGVAR1 = "firstString";

	const SaInt32T INT32VAR2 = -232;
	const SaUint32T UINT32VAR2 = 232;
	const SaInt64T INT64VAR2 = -264;
	const SaUint64T UINT64VAR2 = 264;
	const SaTimeT TIMEVAR2 = 67890;
	const SaFloatT FLOATVAR2 = 23.14359;
	const SaDoubleT DOUBLEVAR2 = 22.712223;
	const SaStringT STRINGVAR2 = "secondStringMultivalue";

	const SaInt32T INT32VAR3 = -332;
	const SaUint32T UINT32VAR3 = 332;
	const SaInt64T INT64VAR3 = -364;
	const SaUint64T UINT64VAR3 = 364;
	const SaTimeT TIMEVAR3 = 54545454;
	const SaFloatT FLOATVAR3 = 2333.14359;
	const SaDoubleT DOUBLEVAR3 = 2244.712223;
	const SaStringT STRINGVAR3 = "thirdStringMultivalue";

	const int POLLWAIT = 100;

	static const SaImmOiImplementerNameT IMPLEMENTERNAME_RT = "Runtime_implementer";
	static SaVersionT immVersion = {'A', 0x02, 0x0c};

	static const char* const DNTESTRT = "stringRdn=TestObject";
	static const char* const DNTESTRT1 = "stringRdn1=TestObject";
	static const char* const DNTESTRT2 = "stringRdn2=TestObject";
	static const char* const DNTESTCFG = "stringRdnCfg=TestObject";

#ifdef	__cplusplus
}
#endif

#endif	/* TEST_NTF_IMCN_H */
