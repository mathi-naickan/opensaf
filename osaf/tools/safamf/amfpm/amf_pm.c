/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2011 The OpenSAF Foundation
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
 * Author(s): Ericsson
 */

/*****************************************************************************

  DESCRIPTION:

  This file contains a utility that can be used to start and stop AMF
  passive monitoring.

******************************************************************************
*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <syslog.h>
#include <getopt.h>
#include <libgen.h>
#include <string.h>
#include <assert.h>
#include <saAmf.h>

bool usesyslog = false;

void logerr(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);

	if (usesyslog)
		vsyslog(LOG_ERR, format, ap);
	else
		vfprintf(stderr, format, ap);

	va_end(ap);
}

void usage(const char *prog)
{
	fprintf(stderr, "\nusage: %s --start | --stop [OPTIONS] [DN]\n\n", prog);
	fprintf(stderr, "Start/stop AMF passive monitoring of an AMF component.\n\n");
	fprintf(stderr, "-a, --start          Start passive monitoring\n");
	fprintf(stderr, "-f, --file=FILE      Path to pidfile\n");
	fprintf(stderr, "-o, --stop           Stop passive monitoring\n");
	fprintf(stderr, "-p, --pid=PID        PID\n");
	fprintf(stderr, "-r, --recrec=RECREC  Recommended recovery\n\n");
	fprintf(stderr, "Valid recommended recovery names: \n");
	fprintf(stderr, "   norec (default), comprestart, compfailover, nodeswitchover\n");
	fprintf(stderr, "   nodefailover, nodefailfast, clusterreset, apprestart, containerrestart\n\n");
	fprintf(stderr, "If no DN is specified, the environment variable SA_AMF_COMPONENT_NAME is used.\n");
	fprintf(stderr, "If no pidfile or PID is specified, the pidfile name is /var/run/<basename>.pid\n");
	fprintf(stderr, "basename is the RDN attribute value of the DN.\n");
	fprintf(stderr, "for example DN \"safComp=snmpd,safSu=1,safSg=2N,safApp=net-snmp\" gives basename snmpd\n");
	fprintf(stderr, "The command will wait forever for the pidfile to appear in the file system.\n");
	fprintf(stderr, "AMF timeouts (e.g. saAmfCtDefClcCliTimeout) solves the \"forever\" problem.\n\n");
}

SaAmfRecommendedRecoveryT recrec2value(const char *recrec)
{
	if (!strcmp(recrec, "norec"))
		return SA_AMF_NO_RECOMMENDATION;
	else if (!strcmp(recrec, "comprestart"))
		return SA_AMF_COMPONENT_RESTART;
	else if (!strcmp(recrec, "compfailover"))
		return SA_AMF_COMPONENT_FAILOVER;
	else if (!strcmp(recrec, "nodeswitchover"))
		return SA_AMF_NODE_SWITCHOVER;
	else if (!strcmp(recrec, "nodefailover"))
		return SA_AMF_NODE_FAILOVER;
	else if (!strcmp(recrec, "nodefailfast"))
		return SA_AMF_NODE_FAILFAST;
	else if (!strcmp(recrec, "clusterreset"))
		return SA_AMF_CLUSTER_RESET;
	else if (!strcmp(recrec, "apprestart"))
		return SA_AMF_APPLICATION_RESTART;
	else if (!strcmp(recrec, "containerrestart"))
		return SA_AMF_CONTAINER_RESTART;
	else
	{
		logerr("unknown recommended recovery: %s\n", recrec);
		exit(EXIT_FAILURE);
	}
}

pid_t getpidfromfile(const char *pidfile, bool waitforfile)
{
	FILE *f;
	pid_t pid;

	assert(pidfile);

retry:
	f = fopen(pidfile, "r");
	if (f == NULL) {
		if ((errno == ENOENT) && waitforfile) {
			sleep(1);
			goto retry;
		}

		logerr("could not open file %s - %s\n", pidfile, strerror(errno));
		exit(EXIT_FAILURE);
	}

	if (fscanf(f, "%d", &pid) == 0) {
		logerr("could not read PID from file %s\n", pidfile);
		exit(EXIT_FAILURE);
	}

	if (fclose(f) != 0) {
		logerr("could not close file\n");
		exit(EXIT_FAILURE);
	}

	return pid;
}

int main(int argc, char **argv)
{
	SaVersionT ver = {.releaseCode = 'B', ver.majorVersion = 0x04, ver.minorVersion = 0x01};
	SaAisErrorT rc;
	SaAmfHandleT amf_hdl;
	SaNameT compName = {0};
	char *prog = basename(argv[0]);
	SaUint64T processId = 0;
	bool start = false;
	bool stop = false;
	int c;
	struct option long_options[] = {
		{"help", no_argument, 0, 'h'},
		{"file", required_argument, 0, 'f'},
		{"pid", required_argument, 0, 'p'},
		{"recrec", required_argument, 0, 'r'},
		{"start", no_argument, 0, 'a'},
		{"stop", no_argument, 0, 'o'},
		{0, 0, 0, 0}
	};
	SaAmfPmErrorsT pmErr = SA_AMF_PM_ZERO_EXIT | SA_AMF_PM_NON_ZERO_EXIT;
        SaInt32T descendentsTreeDepth = 0;
	char *pidfile = NULL;
	char *dn;
	SaAmfRecommendedRecoveryT recrec = SA_AMF_NO_RECOMMENDATION;

	while (1) {
		c = getopt_long(argc, argv, "af:p:hor:", long_options, NULL);

		if (c == -1)	/* have all command-line options have been parsed? */
			break;

		switch (c) {
		case 'a':
			start = true;
			break;
		case 'f':
			pidfile = strdup(optarg);
			break;
		case 'p':
			processId = strtoll(optarg, NULL, 0);
			break;
		case 'h':
			usage(prog);
			exit(EXIT_SUCCESS);
			break;
		case 'o':
			stop = true;
			break;
		case 'r':
			recrec = recrec2value(optarg);
			break;
		default:
			logerr("Try '%s --help' for more information\n", argv[0]);
			exit(EXIT_FAILURE);
			break;
		}
	}

	/* DN specified as argument has precendence of env var */
	if ((argc - optind) == 1) {
		compName.length = snprintf((char*) compName.value, sizeof(compName.value), "%s", argv[optind]);
		if (compName.length >=  sizeof(compName.value)) {
			logerr("too long DN\n");
			exit(EXIT_FAILURE);
		}
	} else if ((dn = getenv("SA_AMF_COMPONENT_NAME")) != NULL) {
		compName.length = snprintf((char*) compName.value,
								   sizeof(compName.value), "%s", dn);

		/* If AMF component use syslog for errors. */
		usesyslog = true;
	} else {
		logerr("missing DN argument\n");
		exit(EXIT_FAILURE);
	}

	if (start && stop) {
		logerr("specify either --start or --stop\n");
		exit(EXIT_FAILURE);
	}

	if (!start && !stop) {
		logerr("neither --start or --stop specified\n");
		exit(EXIT_FAILURE);
	}

	if ((pidfile != NULL) && (processId > 0)) {
		logerr("specify either --file or --pid\n");
		exit(EXIT_FAILURE);
	}

	/* If no pidfile option is specified, create pidfile name as specified
	** by LSB start_daemon (and friends) using basename from DN
	*/
	if ((processId == 0) && (pidfile == NULL)) {
		char basename[64];
		char *start, *stop, *p;
		int i;
	
		start = strchr((char*)compName.value, '=');
		if (start == NULL) {
			logerr("invalid component DN\n");
			exit(EXIT_FAILURE);
		}

		stop = strchr((char*)compName.value, ',');
		if (stop == NULL) {
			logerr("invalid component DN\n");
			exit(EXIT_FAILURE);
		}

		for (p = start + 1, i = 0; p < stop; p++, i++)
			basename[i] = *p;

		basename[i] = '\0';

		pidfile = malloc(64);
		snprintf(pidfile, 64, "/var/run/%s.pid", basename);
	}

	if (processId == 0) {
		processId = getpidfromfile(pidfile, true);
	}

	rc = saAmfInitialize_4(&amf_hdl, NULL, &ver);
	if (SA_AIS_OK != rc) {
		logerr("saAmfInitialize FAILED %u\n", rc);
		exit(EXIT_FAILURE);
	}

	if (start) {
		rc = saAmfPmStart_3(amf_hdl, &compName, processId, descendentsTreeDepth,
							pmErr, recrec);
		if (SA_AIS_OK != rc) {
			logerr("saAmfPmStart FAILED %u\n", rc);
			exit(EXIT_FAILURE);
		}
	} else {
		rc = saAmfPmStop(amf_hdl, &compName, SA_AMF_PM_PROC, processId, pmErr);
		if ((SA_AIS_OK != rc) && (SA_AIS_ERR_NOT_EXIST != rc)) {
			logerr("saAmfPmStop FAILED %u\n", rc);
			exit(EXIT_FAILURE);
		}
	}

	(void) saAmfFinalize(amf_hdl);

	return 0;
}

