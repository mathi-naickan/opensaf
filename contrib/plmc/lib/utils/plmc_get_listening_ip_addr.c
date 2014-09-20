/*      -*- OpenSAF  -*-
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
 * Author(s): Hewlett-Packard Development Company, L.P.
 *
 */

/*
 * plmc_get_listening_ip_addr() utility function.
 *
 * plmc_get_listening_ip_addr() reads the network interfaces that are
 * available and compares their IP addresses to the controller IP
 * addresses found in the plmcd.conf file.  If a match is found, then
 * the matching IP address is returned as a character string,
 * otherwise a NULL string is returned.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>

#include <plmc.h>

extern int plmc_read_config(char *plmc_config_file, PLMC_config_data *config);

/* Location for storing the matched IP address */
static char match_ip[100];

/*
 * plmc_get_iface_list()
 *
 * A function to return a list of available network interfaces.
 *
 * Inputs:
 *   ifconf : A network interface structure.
 *
 * Returns: result of ioctl call.
 * Returns: -1 on error.
 *
 */
int plmc_get_iface_list(struct ifconf *ifconf)
{
  int sock, rval;

  sock = socket(AF_INET,SOCK_STREAM,0);
  if(sock < 0)
  {
    perror("socket");
    return (-1);
  }

  if((rval = ioctl(sock, SIOCGIFCONF , (char*) ifconf  )) < 0 )
    perror("ioctl(SIOGIFCONF)");

  close(sock);

  return rval;
}

/*
 * plmc_get_listening_ip_addr()
 *
 * A function to match available IP addresses to one that is found
 * in the plmcd.conf file.
 *
 * Inputs:
 *   plmc_config_file : A file pathname.
 *
 * Returns: matched IP address on success.
 * Returns: NULL string if no match is found or for any other error.
 *
 */
char * plmc_get_listening_ip_addr(char *plmc_config_file)
{
  static struct ifreq ifreqs[20];
  struct ifconf ifconf;
  int  nifaces, i;
  int s, retval;
  PLMC_config_data config;

  retval = plmc_read_config(plmc_config_file, &config);
  match_ip[0] = 0;

  /* If there is no config file, then we are done. */
  if (retval != 0) {
    printf("error: plmc_get_listening_ip_addr() encountered error reading %s.  errno = %d\n", plmc_config_file, retval);
    syslog(LOG_ERR, "error: plmc_get_listening_ip_addr() encountered error reading %s.  errno = %d", plmc_config_file, retval);
    exit(PLMC_EXIT_FAILURE);
  }

  memset(&ifconf,0,sizeof(ifconf));
  ifconf.ifc_buf = (char*) (ifreqs);
  ifconf.ifc_len = sizeof(ifreqs);

  if (plmc_get_iface_list(&ifconf) < 0)
    exit(-1);

  nifaces =  ifconf.ifc_len/sizeof(struct ifreq);

#ifdef PLMC_DEBUG
  printf("Interfaces (count = %d):\n", nifaces);
#endif
  s = socket(PF_INET, SOCK_DGRAM, 0);
  for(i = 0; i < nifaces; i++)
  {
    if (ioctl(s, SIOCGIFADDR, &ifreqs[i]) >= 0) {
      struct sockaddr_in addr;

      memcpy(&addr, &ifreqs[i].ifr_addr, sizeof(struct sockaddr_in));
#ifdef PLMC_DEBUG
      printf("  %s IP address: %s\n", ifreqs[i].ifr_name,
		      inet_ntoa(addr.sin_addr));
#endif
      if (strcmp(inet_ntoa(addr.sin_addr), config.controller_1_ip) == 0) {
        strncpy(match_ip, inet_ntoa(addr.sin_addr), INET_ADDRSTRLEN);
        break;
      }
      if (strcmp(inet_ntoa(addr.sin_addr), config.controller_2_ip) == 0) {
        strncpy(match_ip, inet_ntoa(addr.sin_addr), INET_ADDRSTRLEN);
        break;
      }
    }
  }

  return(match_ip);
}

