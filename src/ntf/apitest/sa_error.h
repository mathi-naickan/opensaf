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

#ifndef NTF_APITEST_SA_ERROR_H_
#define NTF_APITEST_SA_ERROR_H_

#include <saAis.h>
#include <saNtf.h>

extern char *get_sa_error_b(SaAisErrorT error);

extern char *get_test_output(SaAisErrorT result, SaAisErrorT expected);

extern void print_severity(SaNtfSeverityT input);

extern void print_probable_cause(SaNtfProbableCauseT input);

extern void print_event_type(SaNtfEventTypeT input,
                             SaNtfNotificationTypeT notificationType);

extern void print_change_states(SaNtfStateChangeT *input);

extern void print_object_attributes(SaNtfAttributeT *input);

extern void print_changed_attributes(SaNtfAttributeChangeT *input);

extern void print_security_alarm_types(SaNtfSecurityAlarmNotificationT *input);

extern void print_source_indicator(SaNtfSourceIndicatorT input);

#endif  // NTF_APITEST_SA_ERROR_H_
