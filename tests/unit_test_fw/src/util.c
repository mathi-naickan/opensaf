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
#include <string.h>
#include <sys/time.h>
#include <unistd.h>
#include <assert.h>
#include <saAis.h>

static const char *saf_error[] =
{
    "SA_AIS_NOT_VALID",
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
    "SA_AIS_ERR_BAD_FLAGS",
    "SA_AIS_ERR_TOO_BIG",
    "SA_AIS_ERR_NO_SECTIONS"
};

SaTimeT getSaTimeT(void) 
{ 
    struct timeval tp; 
 
    gettimeofday(&tp, NULL);
 
    return (tp.tv_sec * SA_TIME_ONE_SECOND) +
        (tp.tv_usec * SA_TIME_ONE_MICROSECOND);
} 

void create_dn(char *rdn, char *parent, SaNameT *dn)
{
    dn->length = sprintf((char*)dn->value, "%s,%s", rdn, parent);
    assert(dn->length < SA_MAX_NAME_LENGTH);
}

void sa_namet_init(char *value, SaNameT *namet)
{
    namet->length = sprintf((char*)namet->value, "%s", value);
}

const char *get_saf_error(SaAisErrorT rc)
{
    if (rc <= SA_AIS_ERR_UNAVAILABLE)
        return saf_error[rc];
    else
        return "UNKNOWN";
}

void safassert_impl(const char* file, unsigned int line, SaAisErrorT rc, SaAisErrorT expected)
{
    if (rc != expected)
    {
        fprintf(stderr, "error: in %s at %u: %s, expected %s - exiting\n",
                file, line, saf_error[rc], saf_error[expected]);
        exit(1);
    }
}

