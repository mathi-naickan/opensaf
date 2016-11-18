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

#include <stdio.h>
#include <stdlib.h>
#include "osaf/saf/saAis.h"
#include "ntf/saf/saNtf.h"

void exitIfFalse(int expression) /* instead of assert */
{
  if (!expression) {
    printf("INVALID PARAM - exit\n");
    exit(1);
  }
}

const char *sa_error_list[] = {
    "OUT_OF_RANGE",
    "SA_AIS_OK",
    "SA_AIS_ERR_LIBRARY",
    "SA_AIS_ERR_VERSION",
    "SA_AIS_ERR_INIT",
    "SA_AIS_ERR_TIMEOUT",
    "SA_AIS_ERR_TRY_AGAIN",
    "SA_AIS_ERR_INVALID_PARAM",
    "SA_AIS_ERR_NO_MEMORY",
    "SA_AIS_ERR_BAD_HANDLE",
    "SA_AIS_ERR_BUSY",
    "SA_AIS_ERR_ACCESS",
    "SA_AIS_ERR_NOT_EXIST",
    "SA_AIS_ERR_NAME_TOO_LONG",
    "SA_AIS_ERR_EXIST",
    "SA_AIS_ERR_NO_SPACE",
    "SA_AIS_ERR_INTERRUPT",
    "SA_AIS_ERR_NAME_NOT_FOUND",
    "SA_AIS_ERR_NO_RESOURCES",
    "SA_AIS_ERR_NOT_SUPPORTED",
    "SA_AIS_ERR_BAD_OPERATION",
    "SA_AIS_ERR_FAILED_OPERATION",
    "SA_AIS_ERR_MESSAGE_ERROR",
    "SA_AIS_ERR_QUEUE_FULL",
    "SA_AIS_ERR_QUEUE_NOT_AVAILABLE",
    "SA_AIS_ERR_BAD_CHECKPOINT",
    "SA_AIS_ERR_BAD_FLAGS",
    "SA_AIS_ERR_NO_SECTIONS",
};

const char *sa_probable_cause_list[] = {
    "SA_NTF_ADAPTER_ERROR",
    "SA_NTF_APPLICATION_SUBSYSTEM_FAILURE",
    "SA_NTF_BANDWIDTH_REDUCED",
    "SA_NTF_CALL_ESTABLISHMENT_ERROR",
    "SA_NTF_COMMUNICATIONS_PROTOCOL_ERROR",
    "SA_NTF_COMMUNICATIONS_SUBSYSTEM_FAILURE",
    "SA_NTF_CONFIGURATION_OR_CUSTOMIZATION_ERROR",
    "SA_NTF_CONGESTION",
    "SA_NTF_CORRUPT_DATA",
    "SA_NTF_CPU_CYCLES_LIMIT_EXCEEDED",
    "SA_NTF_DATASET_OR_MODEM_ERROR",
    "SA_NTF_DEGRADED_SIGNAL",
    "SA_NTF_D_T_E",
    "SA_NTF_ENCLOSURE_DOOR_OPEN",
    "SA_NTF_EQUIPMENT_MALFUNCTION",
    "SA_NTF_EXCESSIVE_VIBRATION",
    "SA_NTF_FILE_ERROR",
    "SA_NTF_FIRE_DETECTED",
    "SA_NTF_FLOOD_DETECTED",
    "SA_NTF_FRAMING_ERROR",
    "SA_NTF_HEATING_OR_VENTILATION_OR_COOLING_SYSTEM_PROBLEM",
    "SA_NTF_HUMIDITY_UNACCEPTABLE",
    "SA_NTF_INPUT_OUTPUT_DEVICE_ERROR",
    "SA_NTF_INPUT_DEVICE_ERROR",
    "SA_NTF_L_A_N_ERROR",
    "SA_NTF_LEAK_DETECTED",
    "SA_NTF_LOCAL_NODE_TRANSMISSION_ERROR",
    "SA_NTF_LOSS_OF_FRAME",
    "SA_NTF_LOSS_OF_SIGNAL",
    "SA_NTF_MATERIAL_SUPPLY_EXHAUSTED",
    "SA_NTF_MULTIPLEXER_PROBLEM",
    "SA_NTF_OUT_OF_MEMORY",
    "SA_NTF_OUTPUT_DEVICE_ERROR",
    "SA_NTF_PERFORMANCE_DEGRADED",
    "SA_NTF_POWER_PROBLEM",
    "SA_NTF_PRESSURE_UNACCEPTABLE",
    "SA_NTF_PROCESSOR_PROBLEM",
    "SA_NTF_PUMP_FAILURE",
    "SA_NTF_QUEUE_SIZE_EXCEEDED",
    "SA_NTF_RECEIVE_FAILURE",
    "SA_NTF_RECEIVER_FAILURE",
    "SA_NTF_REMOTE_NODE_TRANSMISSION_ERROR",
    "SA_NTF_RESOURCE_AT_OR_NEARING_CAPACITY",
    "SA_NTF_RESPONSE_TIME_EXCESSIVE",
    "SA_NTF_RETRANSMISSION_RATE_EXCESSIVE",
    "SA_NTF_SOFTWARE_ERROR",
    "SA_NTF_SOFTWARE_PROGRAM_ABNORMALLY_TERMINATED",
    "SA_NTF_SOFTWARE_PROGRAM_ERROR",
    "SA_NTF_STORAGE_CAPACITY_PROBLEM",
    "SA_NTF_TEMPERATURE_UNACCEPTABLE",
    "SA_NTF_THRESHOLD_CROSSED",
    "SA_NTF_TIMING_PROBLEM",
    "SA_NTF_TOXIC_LEAK_DETECTED",
    "SA_NTF_TRANSMIT_FAILURE",
    "SA_NTF_TRANSMITTER_FAILURE",
    "SA_NTF_UNDERLYING_RESOURCE_UNAVAILABLE",
    "SA_NTF_VERSION_MISMATCH",
    "SA_NTF_AUTHENTICATION_FAILURE",
    "SA_NTF_BREACH_OF_CONFIDENTIALITY",
    "SA_NTF_CABLE_TAMPER",
    "SA_NTF_DELAYED_INFORMATION",
    "SA_NTF_DENIAL_OF_SERVICE",
    "SA_NTF_DUPLICATE_INFORMATION",
    "SA_NTF_INFORMATION_MISSING",
    "SA_NTF_INFORMATION_MODIFICATION_DETECTED",
    "SA_NTF_INFORMATION_OUT_OF_SEQUENCE",
    "SA_NTF_INTRUSION_DETECTION",
    "SA_NTF_KEY_EXPIRED",
    "SA_NTF_NON_REPUDIATION_FAILURE",
    "SA_NTF_OUT_OF_HOURS_ACTIVITY",
    "SA_NTF_OUT_OF_SERVICE",
    "SA_NTF_PROCEDURAL_ERROR",
    "SA_NTF_UNAUTHORIZED_ACCESS_ATTEMPT",
    "SA_NTF_UNEXPECTED_INFORMATION",
    "SA_NTF_UNSPECIFIED_REASON",
};

const char *sa_severity_list[] = {
    "SA_NTF_SEVERITY_CLEARED",
    "SA_NTF_SEVERITY_INDETERMINATE",
    "SA_NTF_SEVERITY_WARNING",
    "SA_NTF_SEVERITY_MINOR",
    "SA_NTF_SEVERITY_MAJOR",
    "SA_NTF_SEVERITY_CRITICAL",
};

const char *sa_alarm_event_type_list[] = {
    "SA_NTF_ALARM_NOTIFICATIONS_START",
    "SA_NTF_ALARM_COMMUNICATION",
    "SA_NTF_ALARM_QOS",
    "SA_NTF_ALARM_PROCESSING",
    "SA_NTF_ALARM_EQUIPMENT",
    "SA_NTF_ALARM_ENVIRONMENT",
};

const char *sa_state_change_event_type_list[] = {
    "SA_NTF_STATE_CHANGE_NOTIFICATIONS_START",
    "SA_NTF_OBJECT_STATE_CHANGE",
};

const char *sa_object_create_delete_event_type_list[] = {
    "SA_NTF_OBJECT_NOTIFICATIONS_START",
    "SA_NTF_OBJECT_CREATION",
    "SA_NTF_OBJECT_DELETION",
};

const char *sa_attribute_change_event_type_list[] = {
    "SA_NTF_ATTRIBUTE_NOTIFICATIONS_START",
    "SA_NTF_ATTRIBUTE_ADDED",
    "SA_NTF_ATTRIBUTE_REMOVED",
    "SA_NTF_ATTRIBUTE_CHANGED",
    "SA_NTF_ATTRIBUTE_RESET",
};

const char *sa_security_alarm_event_type_list[] = {
    "SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START",
    "SA_NTF_INTEGRITY_VIOLATION",
    "SA_NTF_OPERATION_VIOLATION",
    "SA_NTF_PHYSICAL_VIOLATION",
    "SA_NTF_SECURITY_SERVICE_VIOLATION",
    "SA_NTF_TIME_VIOLATION",
};

const char *sa_source_indicator_list[] = {
    "SA_NTF_OBJECT_OPERATION",
    "SA_NTF_UNKNOWN_OPERATION",
    "SA_NTF_MANAGEMENT_OPERATION",
};

char *get_sa_error_b(SaAisErrorT error)
{
    exitIfFalse(error >= SA_AIS_OK);
    exitIfFalse(error <= SA_AIS_ERR_NO_SECTIONS);

    return ((char *)sa_error_list[error]);
}

char *get_test_output(SaAisErrorT result, SaAisErrorT expected)
{
    static char test_result[256];

    if (result == expected) {
	return ("PASSED");
    } else {
	(void)sprintf(test_result,
		"FAILED expected %s got %s",
		get_sa_error_b(expected),
		get_sa_error_b(result));
	return (test_result);
    }
}

void print_severity(SaNtfSeverityT input)
{
    exitIfFalse(input >= SA_NTF_SEVERITY_CLEARED);
    exitIfFalse(input <= SA_NTF_SEVERITY_CRITICAL);

    (void)printf("%s\n", (char *)sa_severity_list[input]);
}

void print_probable_cause(SaNtfProbableCauseT input)
{
    exitIfFalse(input >= SA_NTF_ADAPTER_ERROR);
    exitIfFalse(input <= SA_NTF_UNSPECIFIED_REASON);

    (void)printf("%s\n", (char *)sa_probable_cause_list[input]);
}

void print_event_type(SaNtfEventTypeT input,
                      SaNtfNotificationTypeT notificationType)
{
    int listIndex;


    switch (notificationType) {
    case SA_NTF_TYPE_STATE_CHANGE:
        if(input >= (int)SA_NTF_STATE_CHANGE_NOTIFICATIONS_START) {
            listIndex = (int)input - (int)SA_NTF_TYPE_STATE_CHANGE;

	    exitIfFalse(input >= SA_NTF_STATE_CHANGE_NOTIFICATIONS_START);
	    exitIfFalse(input <= SA_NTF_OBJECT_STATE_CHANGE);

            (void)printf("%s\n",
                         (char *)sa_state_change_event_type_list[listIndex]);

        }
        break;

    case SA_NTF_TYPE_ALARM:
	if(input >= (int)SA_NTF_ALARM_NOTIFICATIONS_START) {
	    listIndex = (int)input - (int)SA_NTF_TYPE_ALARM;

	    exitIfFalse(input >= SA_NTF_ALARM_NOTIFICATIONS_START);
	    exitIfFalse(input <= SA_NTF_ALARM_ENVIRONMENT);

	    (void)printf("%s\n",
			 (char *)sa_alarm_event_type_list[listIndex]);
	}
        break;

    case SA_NTF_TYPE_OBJECT_CREATE_DELETE:
	if(input >= (int)SA_NTF_OBJECT_NOTIFICATIONS_START) {
	    listIndex = (int)input - (int)SA_NTF_TYPE_OBJECT_CREATE_DELETE;

	    exitIfFalse(input >= SA_NTF_OBJECT_NOTIFICATIONS_START);
	    exitIfFalse(input <= SA_NTF_OBJECT_DELETION);

	    (void)printf(
		"%s\n",
		(char *)sa_object_create_delete_event_type_list[listIndex]);
	}
        break;

    case SA_NTF_TYPE_ATTRIBUTE_CHANGE:
	if(input >= (int)SA_NTF_ATTRIBUTE_NOTIFICATIONS_START) {
	    listIndex = (int)input - (int)SA_NTF_TYPE_ATTRIBUTE_CHANGE;

	    exitIfFalse(input >= SA_NTF_ATTRIBUTE_NOTIFICATIONS_START);
	    exitIfFalse(input <= SA_NTF_ATTRIBUTE_RESET);

	    (void)printf(
		"%s\n",
		(char *)sa_attribute_change_event_type_list[listIndex]);
	}
	break;

    case SA_NTF_TYPE_SECURITY_ALARM:
	if(input >= (int)SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START) {
	    listIndex = (int)input - (int)SA_NTF_TYPE_SECURITY_ALARM;

	    exitIfFalse(input >= SA_NTF_SECURITY_ALARM_NOTIFICATIONS_START);
	    exitIfFalse(input <= SA_NTF_TIME_VIOLATION);

	    (void)printf("%s\n",
			 (char *)sa_security_alarm_event_type_list[listIndex]);
	}
	break;

    default:
        (void)printf("Unknown Notification Type!!");
        exit(1);
        break;
    }
}

void print_change_states(SaNtfStateChangeT *input)
{
    (void)printf("State ID: %d\n", input->stateId);

    if (input->oldStatePresent == SA_TRUE) {
        (void)printf("Old State Present: Yes\n");
        (void)printf("Old State: %d\n", input->oldState);
    } else {
        (void)printf("Old State Present: No\n");
    }
    (void)printf("New State: %d\n", input->newState);

}

void print_object_attributes(SaNtfAttributeT *input)
{
    (void)printf("Attribute ID: %d\n", input->attributeId);
    (void)printf("Attribute Type: %d\n", input->attributeType);
    (void)printf("Attribute Value: %d\n", input->attributeValue.int32Val);
}

void print_changed_attributes(SaNtfAttributeChangeT *input)
{
    (void)printf("Attribute ID: %d\n", input->attributeId);

    (void)printf("Attribute Type: %d\n", input->attributeType);
    if (input->oldAttributePresent == SA_TRUE) {
        (void)printf("Old Attribute Present: Yes\n");
        (void)printf("Old Attribute: %d\n", input->oldAttributeValue.int16Val);
    } else {
        (void)printf("Old Attribute Present: No\n");
    }
    (void)printf("New Attribute Value: %d\n", input->newAttributeValue.int32Val);

}

void print_security_alarm_types(SaNtfSecurityAlarmNotificationT *input)
{
    (void)printf("Security Alarm Detector Type: %d\n",
		 input->securityAlarmDetector->valueType);
    (void)printf("Security Alarm Detector Value: %d\n",
		 input->securityAlarmDetector->value.int32Val);

    (void)printf("Service User Type: %d\n",
		 input->serviceUser->valueType);
    (void)printf("Service User Value: %d\n",
		 input->serviceUser->value.int32Val);

    (void)printf("Service Provider Type: %d\n",
		 input->serviceProvider->valueType);
    (void)printf("Service Provider Value: %d\n",
		 input->serviceProvider->value.int32Val);
}

void print_source_indicator(SaNtfSourceIndicatorT input)
{
    exitIfFalse(input >= SA_NTF_OBJECT_OPERATION);
    exitIfFalse(input <= SA_NTF_MANAGEMENT_OPERATION);

    (void)printf("%s\n", (char *)sa_source_indicator_list[input]);
}
