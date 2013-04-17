/*      -*- OpenSAF  -*-
 *
 * (C) Copyright 2010 The OpenSAF Foundation
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 * under the GNU Lesser General Public License Version 2.1, February 1999.
 * The complete license can be accessed from the following location:
 * http://opensource.org/licenses/lgpl-license.php
 * See the Copying file included with the OpenSAF distribution for full
 * licensing terms.z
 *
 * Author(s): GoAhead Software
 *
 */

#include <net/if.h>
#include <arpa/inet.h>
#include <configmake.h>
#include <ifaddrs.h>
#include "ncs_main_papi.h"
#include "dtm.h"
#include "dtm_socket.h"
#include "dtm_node.h"

char match_ip[INET6_ADDRSTRLEN];

/* Socket timeout values */
#define SOCK_KEEPALIVE  1
#define KEEPIDLE_TIME   7200
#define KEEPALIVE_INTVL 75
#define KEEPALIVE_PROBES 9
#define DIS_TIME_OUT 5
#define BCAST_FRE 250

/* These are the numerical values of the tags found in the dtmd.conf */
/* configuration file.                                               */
typedef enum dtm_config_tags {
	DTM_CLUSTER_ID = 1,
	DTM_NODE_IP,
	DTM_MCAST_ADDR,
	DTM_TCP_LISTENING_PORT,
	DTM_UDP_BCAST_SND_PORT,
	DTM_UDP_BCAST_REV_PORT,
	DTM_BCAST_FRE_MSECS,
	DTM_INI_DIS_TIMEOUT_SECS,
	DTM_SKEEPALIVE,
	DTM_TCP_KEEPIDLE_TIME,
	DTM_TCP_KEEPALIVE_INTVL,
	DTM_TCP_KEEPALIVE_PROBES
} DTM_CONFIG_TAGS;


/**
 * print the configuration
 *
 * @param config
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void dtm_print_config(DTM_INTERNODE_CB * config)
{
 	TRACE("DTM :dtm.conf: config file contains:");
 	TRACE("  IP_V4_OR_V6: ");
 	TRACE("  %d", config->i_addr_family);
 	TRACE("  DTM_CLUSTER_ID: ");
 	TRACE("  %d", config->cluster_id);
 	TRACE("  NODE_ID: ");
 	TRACE("  %d", config->node_id);
 	TRACE("  IP_ADDR: ");
 	TRACE("  %s", config->ip_addr);
 	TRACE("  STREAM_PORT: ");
 	TRACE("  %u", config->stream_port);
 	TRACE("  DGRAM_PORT_SNDR: ");
 	TRACE("  %u", config->dgram_port_sndr);
 	TRACE("  DGRAM_PORT_REV: ");
 	TRACE("  %u", config->dgram_port_rcvr);
 	TRACE("  MCAST_ADDR: ");
 	TRACE("  %s", config->mcast_addr);
 	TRACE("  NODE_NAME: ");
 	TRACE("  %s", config->node_name);
 	TRACE("  DTM_SKEEPALIVE: ");
 	TRACE("  %d", config->so_keepalive);
 	TRACE("  COMM_KEEPIDLE_TIME: ");
 	TRACE("  %d", config->comm_keepidle_time);
 	TRACE("  COMM_KEEPALIVE_INTVL: ");
 	TRACE("  %d", config->comm_keepalive_intvl);
 	TRACE("  COMM_KEEPALIVE_PROBES: ");
 	TRACE("  %d", config->comm_keepalive_probes);
 	TRACE("  DTM_INI_DIS_TIMEOUT_SECS: ");
 	TRACE("  %d", config->initial_dis_timeout);
 	TRACE("  DTM_BCAST_FRE_MSECS: ");
	TRACE("  %d", config->bcast_msg_freq);
 	TRACE("DTM : ");
}


bool in6_islinklocal(struct sockaddr_in6 *sin6)
{
	if (IN6_IS_ADDR_LINKLOCAL(&sin6->sin6_addr)) 
		return true;
	else
		return false;
}


/**
 * validate the ip address
 *
 * @param config
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
char *dtm_validate_listening_ip_addr(DTM_INTERNODE_CB * config)
{
	struct ifaddrs *if_addrs = NULL;
	struct ifaddrs *if_addr = NULL;
	void *tmp = NULL;
	char buf[INET6_ADDRSTRLEN];
	memset(match_ip, 0, INET6_ADDRSTRLEN);

	if (0 == getifaddrs(&if_addrs)) {
		for (if_addr = if_addrs; if_addr != NULL; if_addr = if_addr->ifa_next) {

			TRACE("Interface name : %s", if_addr->ifa_name);

			// Address
			if (if_addr->ifa_addr->sa_family == AF_INET) {
				tmp = &((struct sockaddr_in *)if_addr->ifa_addr)->sin_addr;
			} else if (if_addr->ifa_addr->sa_family == AF_INET6)  {
				tmp = &((struct sockaddr_in6 *)if_addr->ifa_addr)->sin6_addr;
			}
			TRACE("IP addr : %s",
					inet_ntop(if_addr->ifa_addr->sa_family,
						tmp,
						match_ip,
						sizeof(match_ip)));

			if (strcmp(match_ip, config->ip_addr) == 0) {

				config->i_addr_family  = if_addr->ifa_addr->sa_family;
				strncpy(config->ifname, if_addr->ifa_name, IFNAMSIZ);

				//Bcast  Address
				if (if_addr->ifa_addr->sa_family == AF_INET) {
					tmp = &((struct sockaddr_in *)if_addr->ifa_broadaddr)->sin_addr;
					TRACE("Bcast addr : %s",
							inet_ntop(if_addr->ifa_addr->sa_family,
								tmp,
								config->bcast_addr,
								sizeof(buf)));
				} else if (if_addr->ifa_addr->sa_family == AF_INET6) {
					struct sockaddr_in6 *addr = (struct sockaddr_in6 *)if_addr->ifa_addr;
					if (in6_islinklocal(addr) == true ) {
						config->scope_link = true;	
						TRACE ("DTM:  %s scope_link : %d    IP address : %s  sa_family : %d ",
								if_addr->ifa_name, config->scope_link, match_ip, config->i_addr_family);
					}			
					TRACE("Bcast addr : %s",
							inet_ntop(if_addr->ifa_addr->sa_family,
								addr,
								config->bcast_addr,
								sizeof(buf)));

				}
				TRACE ("DTM:  %s Validate  IP address : %s  Bcast address : %s sa_family : %d ",
						if_addr->ifa_name, match_ip, config->bcast_addr, config->i_addr_family);

				freeifaddrs(if_addrs);
				if_addrs = NULL;
				return (match_ip);
			}

		}
		freeifaddrs(if_addrs);
		if_addrs = NULL;
	} else {
		TRACE("getifaddrs() failed with errno =  %i %s", errno, strerror(errno));
	}
	LOG_ER("DTM:  Validation of  IP address failed : %s ", config->ip_addr);
	return (NULL);
}

/**
 * read the configuartion file
 *
 * @param config dtm_config_file
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int dtm_read_config(DTM_INTERNODE_CB * config, char *dtm_config_file)
{
	FILE *dtm_conf_file;
	char line[DTM_MAX_TAG_LEN];
	int i, n, comment_line, fieldmissing = 0, tag = 0, tag_len = 0;
	/* Location for storing the matched IP address */
	char *local_match_ip = NULL;
	FILE *fp;

	TRACE_ENTER();

	/*
	 * Initialize the socket timeout values in case they are not
	 * set in the dtm.conf file
	 */

	memset(line, 0, DTM_MAX_TAG_LEN);

	config->so_keepalive = SOCK_KEEPALIVE;
	config->comm_keepidle_time = KEEPIDLE_TIME;
	config->comm_keepalive_intvl = KEEPALIVE_INTVL;
	config->comm_keepalive_probes = KEEPALIVE_PROBES;
	config->i_addr_family = DTM_IP_ADDR_TYPE_IPV4;
	config->bcast_msg_freq = BCAST_FRE;
	config->initial_dis_timeout = DIS_TIME_OUT;
	config->mcast_flag = false;
	config->scope_link = false;
	config->node_id = m_NCS_GET_NODE_ID;
	fp = fopen(PKGSYSCONFDIR "/node_name", "r");
	if (fp == NULL) {
		LOG_ER("DTM: Could not open file  node_name ");
		return errno;
	}
	if (EOF == fscanf(fp, "%s", config->node_name)) {
		fclose(fp);
		LOG_ER("DTM: Could not get node name ");
		return errno;
	}
	fclose(fp);
	TRACE("DTM :config->node_nam : %s", config->node_name);
	config->cluster_id = -1;

	/* 
	 * Set timeout to a negative value so we can detect if the value 
	 * is missing in the conf file
	 */

	/* Open dtm.conf config file. */
	dtm_conf_file = fopen(dtm_config_file, "r");
	if (dtm_conf_file == NULL) {
		/* Problem with conf file - return errno value */
		LOG_ER("DTM: dtm_read_cofig: there was a problem opening the dtm.conf file");
		return errno;
	}

	/* Read file. */
	while (fgets(line, DTM_MAX_TAG_LEN, dtm_conf_file)) {
		/* If blank line, skip it - and set tag back to 0. */
		if (strcmp(line, "\n") == 0) {
			tag = 0;
			continue;
		}

		/* If a comment line, skip it. */
		n = strlen(line);
		comment_line = 0;
		for (i = 0; i < n; i++) {
			if ((line[i] == ' ') || (line[i] == '\t'))
				continue;
			else if (line[i] == '#') {
				comment_line = 1;
				break;
			} else
				break;
		}
		if (comment_line)
			continue;

		/* Remove the new-line char at the end of the line. */
		line[n - 1] = 0;

		/* Set tag if we are not in a tag. */
		if ((tag == 0) || (strncmp(line, "DTM_", 4) == 0)) {

			if (strncmp(line, "DTM_CLUSTER_ID=", strlen("DTM_CLUSTER_ID=")) == 0) {
				tag_len = strlen("DTM_CLUSTER_ID=");
				config->cluster_id = atoi(&line[tag_len]);
				if (config->cluster_id < 1) {
					LOG_ER("DTM:cluster_id must be a positive integer");
					return -1;
				}
				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_NODE_IP=", strlen("DTM_NODE_IP=")) == 0) {
				tag_len = strlen("DTM_NODE_IP=");
				strncpy(config->ip_addr, &line[tag_len], INET6_ADDRSTRLEN - 1);	/* ipv4 ipv6 addrBuffer */
				if (strlen(config->ip_addr) == 0) {
					LOG_ER("DTM:ip_addr Shouldn't  be NULL");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}

			if (strncmp(line, "DTM_MCAST_ADDR=", strlen("DTM_MCAST_ADDR=")) == 0) {
				tag_len = strlen("DTM_MCAST_ADDR=");
				strncpy(config->mcast_addr, &line[tag_len], INET6_ADDRSTRLEN - 1);	/* ipv4 ipv6 addrBuffer */
				if (strlen(config->mcast_addr) != 0) {
					config->mcast_flag = true;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_TCP_LISTENING_PORT=", strlen("DTM_TCP_LISTENING_PORT=")) == 0) {
				tag_len = strlen("DTM_TCP_LISTENING_PORT=");
				config->stream_port = (in_port_t)atoi(&line[tag_len]);

				if (config->stream_port < 1) {
					LOG_ER("DTM:stream_port  must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_UDP_BCAST_SND_PORT=", strlen("DTM_UDP_BCAST_SND_PORT=")) == 0) {
				tag_len = strlen("DTM_UDP_BCAST_SND_PORT=");
				config->dgram_port_sndr = ((in_port_t)atoi(&line[tag_len]));
				if (config->dgram_port_sndr < 1) {
					LOG_ER("DTM:dgram_port_sndr  must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_UDP_BCAST_REV_PORT=", strlen("DTM_UDP_BCAST_REV_PORT=")) == 0) {
				tag_len = strlen("DTM_UDP_BCAST_REV_PORT=");
				config->dgram_port_rcvr = ((in_port_t)atoi(&line[tag_len]));
				TRACE("DTM:dgram_port_rcvr  :%d", config->dgram_port_rcvr);
				if (config->dgram_port_rcvr < 1) {
					LOG_ER("DTM:dgram_port_rcvr t must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_BCAST_FRE_MSECS=", strlen("DTM_BCAST_FRE_MSECS=")) == 0) {
				tag_len = strlen("DTM_BCAST_FRE_MSECS=");
				config->bcast_msg_freq = atoi(&line[tag_len]);
				if (config->bcast_msg_freq < 1) {
					LOG_ER("DTM:bcast_msg_freq  must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_INI_DIS_TIMEOUT_SECS=", strlen("DTM_INI_DIS_TIMEOUT_SECS=")) == 0) {
				tag_len = strlen("DTM_INI_DIS_TIMEOUT_SECS=");
				config->initial_dis_timeout = atoi(&line[tag_len]);
				if (config->initial_dis_timeout < 1) {
					LOG_ER("DTM:initial_dis_timeout must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_SKEEPALIVE=", strlen("DTM_SKEEPALIVE=")) == 0) {
				tag_len = strlen("DTM_SKEEPALIVE=");
				config->so_keepalive = atoi(&line[tag_len]);
				if (config->so_keepalive < 0 || config->so_keepalive > 1) {
					LOG_ER("DTM: so_keepalive needs to be 0 or 1");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_TCP_KEEPIDLE_TIME=", strlen("DTM_TCP_KEEPIDLE_TIME=")) == 0) {
				tag_len = strlen("DTM_TCP_KEEPIDLE_TIME=");
				config->comm_keepidle_time = atoi(&line[tag_len]);
				if (config->comm_keepidle_time < 1) {
					LOG_ER("DTM:comm_keepidle_time must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_TCP_KEEPALIVE_INTVL=", strlen("DTM_TCP_KEEPALIVE_INTVL=")) == 0) {
				tag_len = strlen("DTM_TCP_KEEPALIVE_INTVL=");
				config->comm_keepalive_intvl = atoi(&line[tag_len]);
				if (config->comm_keepalive_intvl < 1) {
					LOG_ER("DTM:comm_keepalive_intvl must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
			if (strncmp(line, "DTM_TCP_KEEPALIVE_PROBES=", strlen("DTM_TCP_KEEPALIVE_PROBES=")) == 0) {
				tag_len = strlen("DTM_TCP_KEEPALIVE_PROBES=");
				config->comm_keepalive_probes = atoi(&line[tag_len]);
				if (config->comm_keepalive_probes < 1) {
					LOG_ER("DTM:comm_keepalive_probes must be a positive integer");
					return -1;
				}

				tag = 0;
				tag_len = 0;

			}
		}

		memset(line, 0, DTM_MAX_TAG_LEN);
	}

	/* End-of-file or error? */
	int err = feof(dtm_conf_file) ? 0 : errno;

	/* Close file. */
	fclose(dtm_conf_file);

	/*************************************************************/
	/* Set up validate the IP & sa_family stuff  */
	/*************************************************************/
	local_match_ip = dtm_validate_listening_ip_addr(config);
	if (local_match_ip == NULL) {
		LOG_ER("DTM: ip_addr cannot match available network interfaces with IPs of node specified in the dtm.conf file");
		return -1;
	}

	/* Test so we have all mandatory fields */
	if ((config->cluster_id) == 0) {
		LOG_ER("DTM: dtm_read_config: cluster_id is missing in conf file");
		fieldmissing = 1;
	} else if ((config->node_id) == 0) {
		LOG_ER("DTM: dtm_read_config: node_id is missing in configuration");
		fieldmissing = 1;
	} else if ((config->dgram_port_sndr) == 0) {
		LOG_ER("DTM: dtm_read_config: dgram_port_sndr is missing in conf file");
		fieldmissing = 1;
	} else if ((config->dgram_port_rcvr) == 0) {
		LOG_ER("DTM: dtm_read_config: dgram_port_rcvr is missing in conf file");
		fieldmissing = 1;
	} else if ((config->stream_port) == 0) {
		LOG_ER("DTM: dtm_read_config: stream_port is missing in conf file");
		fieldmissing = 1;
	} else if (strlen(config->ip_addr) == 0) {
		LOG_ER("DTM: dtm_read_config: ip_addr is missing in conf file");
		fieldmissing = 1;
	} else if (strlen(config->node_name) == 0) {
		LOG_ER("DTM: dtm_read_config: node_name is missing in conf file");
		fieldmissing = 1;
	}

	if (fieldmissing == 1)
		return -1;
	/* All done. */

	dtm_print_config(config);

	TRACE_LEAVE();
	return (err);
}

#if 0
static int checkfile(char *buf)
{
	struct stat statbuf;
	int ii;
	char cmd[DTM_MAX_TAG_LEN];

	strncpy(cmd, buf, DTM_MAX_TAG_LEN - 1);
	for (ii = 0; ii < strlen(cmd); ii++)
		if (cmd[ii] == ' ')
			cmd[ii] = '\0';

	if (stat(cmd, &statbuf) == -1) {
		LOG_ER("DTM: dtm_read_config: File does not exsist: %s", cmd);
		return -1;
	}

	return 0;
}

#endif
