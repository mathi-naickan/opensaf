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

#include <netdb.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include "ncsencdec_pub.h"
#include "usrbuf.h"
#include "dtm.h"
#include "dtm_socket.h"
#include "dtm_node.h"

#define MYPORT "6900"
#define MAXBUFLEN 100
#define DTM_INTERNODE_SOCK_SIZE 64000

struct addrinfo *mcast_sender_addr;	/* Holder for mcast_sender_addr address */

struct sockaddr_storage bcast_dest_storage;
struct sockaddr *bcast_dest_address = (struct sockaddr *)&bcast_dest_storage;

size_t bcast_sen_addr_size;	/* Holder for bcast_dest_address size ip v4 or v6 address */

/**
 * Close the socketr descriptors
 *
 * @param sock_desc
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_sockdesc_close(int sock_desc)
{
	TRACE_ENTER();

	if (close(sock_desc) != 0) {
		LOG_ER("DTM : dtm_sockdesc_close err :%s", strerror(errno));
		return NCSCC_RC_FAILURE;
	}
	sock_desc = -1;

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Set the keep alive
 *
 * @param dtms_cb sock_desc
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t set_keepalive(DTM_INTERNODE_CB * dtms_cb, int sock_desc)
{

	TRACE_ENTER();
	socklen_t optlen;
	int comm_keepidle_time, comm_keepalive_intvl, comm_keepalive_probes;
	int so_keepalive;
	int smode = 1;

	/* Start with setting the tcp keepalive stuff */
	so_keepalive = dtms_cb->so_keepalive;
	comm_keepidle_time = dtms_cb->comm_keepidle_time;
	comm_keepalive_intvl = dtms_cb->comm_keepalive_intvl;
	comm_keepalive_probes = dtms_cb->comm_keepalive_probes;

	if (so_keepalive == true) {
		/* Set SO_KEEPALIVE */
		optlen = sizeof(so_keepalive);
		if (setsockopt(sock_desc, SOL_SOCKET, SO_KEEPALIVE, &so_keepalive, optlen) < 0) {
			LOG_ER("DTM :setsockopt SO_KEEPALIVE failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}

		/* Set TCP_KEEPIDLE */
		optlen = sizeof(comm_keepidle_time);
		if (setsockopt(sock_desc, SOL_TCP, TCP_KEEPIDLE, &comm_keepidle_time, optlen) < 0) {
			LOG_ER("DTM :setsockopt TCP_KEEPIDLE failed err :%s ", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}

		/* Set TCP_KEEPINTVL */
		optlen = sizeof(comm_keepalive_intvl);
		if (setsockopt(sock_desc, SOL_TCP, TCP_KEEPINTVL, &comm_keepalive_intvl, optlen) < 0) {
			LOG_ER("DTM :setsockopt TCP_KEEPINTVL failed err :%s ", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}

		/* Set TCP_KEEPCNT */
		optlen = sizeof(comm_keepalive_probes);
		if (setsockopt(sock_desc, SOL_TCP, TCP_KEEPCNT, &comm_keepalive_probes, optlen) < 0) {
			LOG_ER("DTM :setsockopt TCP_KEEPCNT  failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
	}

	if ((setsockopt(sock_desc, SOL_SOCKET, SO_REUSEADDR, (void *)&smode, sizeof(smode)) == -1)) {
		LOG_ER("DTM : Error setsockpot: err :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Set the non blocking for socket
 *
 * @param sock_desc bNb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint8_t set_nonblocking(int sock_desc, uint8_t bNb)
{
	TRACE_ENTER();
	int flags;
	if ((flags = fcntl(sock_desc, F_GETFL, NULL)) < 0) {
		LOG_ER("DTM :fcntl(F_SETFL, O_NONBLOCK) err :%s ", strerror(errno));
		return false;
	}
	if (bNb) {
		flags |= O_NONBLOCK;
		if (fcntl(sock_desc, F_SETFL, flags) < 0) {
			LOG_ER("DTM :fcntl(F_SETFL, O_NONBLOCK) err :%s ", strerror(errno));
			return false;
		}
	} else {
		flags &= (~O_NONBLOCK);
		if (fcntl(sock_desc, F_SETFL, flags) < 0) {
			LOG_ER("DTM :fcntl(F_SETFL, 0) err :%s", strerror(errno));
			return false;
		}
	}
	TRACE_LEAVE();
	return true;

}

/**
 * Enable the dgram bcast
 *
 * @param sock_desc
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dgram_enable_bcast(int sock_desc)
{

	TRACE_ENTER();
	/* If this fails, we'll hear about it when we try to send.  This will allow */
	/* system that cannot bcast to continue if they don't plan to bcast */
	int bcast_permission = 1;
	if (setsockopt(sock_desc, SOL_SOCKET, SO_BROADCAST, (raw_type *) & bcast_permission, sizeof(bcast_permission)) <
	    0) {
		LOG_ER("DTM :setsockopt(SO_BROADCAST) failed err :%s ", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/**
 * Join the mcast group
 *
 * @param dtms_cb mcast_receiver_addr
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dgram_join_mcast_group(DTM_INTERNODE_CB * dtms_cb, struct addrinfo *mcast_receiver_addr)
{

	TRACE_ENTER();

	/*  we need some address-family-specific pieces */
	if (mcast_receiver_addr->ai_family == AF_INET6) {
		/* Now join the mcast "group" (address) */
		struct ipv6_mreq join_request;
		memcpy(&join_request.ipv6mr_multiaddr,
		       &((struct sockaddr_in6 *)mcast_receiver_addr->ai_addr)->sin6_addr, sizeof(struct in6_addr));
		join_request.ipv6mr_interface = if_nametoindex(dtms_cb->ifname);
		TRACE("DTM :Joining IPv6 mcast group...");
		if (setsockopt
		    (dtms_cb->dgram_sock_rcvr, IPPROTO_IPV6, IPV6_JOIN_GROUP, &join_request,
		     sizeof(join_request)) < 0) {
			LOG_ER("DTM :setsockopt(IPV6_JOIN_GROUP) failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
	} else if (mcast_receiver_addr->ai_family == AF_INET) {

		/* Now join the mcast "group" */
		struct ip_mreq join_request;
		join_request.imr_multiaddr = ((struct sockaddr_in *)mcast_receiver_addr->ai_addr)->sin_addr;
		join_request.imr_interface.s_addr =  inet_addr(dtms_cb->ip_addr);
		TRACE("DTM :Joining IPv4 mcast group...");
		if (setsockopt
		    (dtms_cb->dgram_sock_rcvr, IPPROTO_IP, IP_ADD_MEMBERSHIP, &join_request,
		     sizeof(join_request)) < 0) {
			LOG_ER("DTM :setsockopt(IP_ADD_MEMBERSHIP) failed err :%s ", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}

	} else {
		LOG_ER("DTM: AF not supported :%d", mcast_receiver_addr->ai_family);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Send the dgram mcast message 
 *
 * @param dtms_cb buffer buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_sendto_mcast(DTM_INTERNODE_CB * dtms_cb, const void *buffer, int buffer_len)
{
	TRACE_ENTER();
	/* Multicast the string to all who have joined the group */
	ssize_t num_bytes = sendto(dtms_cb->dgram_sock_sndr, (raw_type *) buffer, buffer_len, 0,
				   mcast_sender_addr->ai_addr, mcast_sender_addr->ai_addrlen);
	if (num_bytes < 0) {
		LOG_ER("DTM : sendto() failed err :%s ", strerror(errno));
		TRACE_LEAVE2("rc::%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	} else if (num_bytes != buffer_len) {
		LOG_ER("DTM :sendto() sent unexpected number of bytes err :%s ", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Bcast send function
 *
 * @param dtms_cb buffer buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_sendto_bcast(DTM_INTERNODE_CB * dtms_cb, const void *buffer, int buffer_len)
{
	TRACE_ENTER();

	/* Multicast the string to all who have joined the group */
	ssize_t num_bytes = sendto(dtms_cb->dgram_sock_sndr, (raw_type *) buffer, buffer_len, 0,
				   (struct sockaddr *)bcast_dest_address, bcast_sen_addr_size);
	if (num_bytes < 0) {
		LOG_ER("DTM :sendto() failed err :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	} else if (num_bytes != buffer_len) {
		LOG_ER("DTM :sendto() sent unexpected number of bytes err :%s ", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Set the mcast ttl
 *
 * @param dtms_cb mcast_ttl
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
static uint32_t dgram_set_mcast_ttl(DTM_INTERNODE_CB * dtms_cb, int mcast_ttl)
{

	TRACE_ENTER();
	/* Set TTL of mcast packet. Unfortunately this requires */
	/* address-family-specific code */
	if (mcast_sender_addr->ai_family == AF_INET6) {	// v6-specific
		/* The v6 mcast TTL socket option requires that the value be */
		/* passed in as an integer */
		if (setsockopt
		    (dtms_cb->dgram_sock_sndr, IPPROTO_IPV6, IPV6_MULTICAST_HOPS, &mcast_ttl, sizeof(mcast_ttl)) < 0) {
			LOG_ER("DTM : setsockopt(IPV6_MULTICAST_HOPS) failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
	} else if (mcast_sender_addr->ai_family == AF_INET) {	/* v4 specific */
		/* The v4 mcast TTL socket option requires that the value be */
		/* passed in an unsigned char */
		u_char mcTTL = (u_char)mcast_ttl;
		if (setsockopt(dtms_cb->dgram_sock_sndr, IPPROTO_IP, IP_MULTICAST_TTL, &mcTTL, sizeof(mcTTL)) < 0) {
			LOG_ER("DTM :setsockopt(IP_MULTICAST_TTL) failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}
	} else {
		
		LOG_ER("DTM: AF not supported :%d", mcast_sender_addr->ai_family);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Close the socket 
 *
 * @param comm_socket
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_comm_socket_close(int *comm_socket)
{

	DTM_NODE_DB *node;
	TRACE_ENTER();
	int rc = NCSCC_RC_SUCCESS;
	int err = 0;

	node = dtm_node_get_by_comm_socket(*comm_socket);

	if (node != NULL) {
		TRACE("DTM: node deleting  enty ");
		if (true == node->comm_status) {
			if (dtm_process_node_up_down(node->node_id, node->node_name, false) != NCSCC_RC_SUCCESS) {
				LOG_ER(" dtm_process_node_up_down() failed rc : %d ", rc);
				rc = NCSCC_RC_FAILURE;
				goto done;
			}
		}

		if (dtm_node_delete(node, 0) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed ");
		}

		if (dtm_node_delete(node, 1) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed");
		}

		if (dtm_node_delete(node, 2) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM :dtm_node_delete failed ");
		}

		free(node);

	} else
		TRACE("DTM :comm_socket_not exist ");

	if (close(*comm_socket) != 0) {
		err = errno;
		if (!IS_BLOCKIN_ERROR(err)) {

			LOG_ER("DTM : dtm_sockdesc_close err :%s ", strerror(err));
			rc = NCSCC_RC_FAILURE;
			goto done;
		}

	}
	*comm_socket = -1;

 done:
	TRACE_LEAVE2("rc :%d", rc);
	return rc;

}

/**
 * Send the message
 *
 * @param sock_desc buffer buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_comm_socket_send(int sock_desc, const void *buffer, int buffer_len)
{
	TRACE_ENTER();
	int rtn = 0;
	int err = 0;
	int rc = NCSCC_RC_SUCCESS;
	rtn = send(sock_desc, (raw_type *) buffer, buffer_len, 0);
	err = errno;
	if (rtn < 0) {
		if (!IS_BLOCKIN_ERROR(err)) {
			LOG_ER("DTM :dtm_comm_socket_send failed  err :%s", strerror(err));
			rc = NCSCC_RC_FAILURE;
		}

	}
	TRACE_LEAVE2("rc :%d", rc);
	return rc;
}

/**
 * Rcv the message from the socket
 *
 * @param sock_desc buffer buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_comm_socket_recv(int sock_desc, void *buffer, int buffer_len)
{
	int rtn = 0, err = 0;
	int rc = NCSCC_RC_SUCCESS;
	TRACE_ENTER();
	if ((rtn = recv(sock_desc, (raw_type *) buffer, buffer_len, 0)) < 0) {
		err = errno;
		if (!IS_BLOCKIN_ERROR(err)) {
			LOG_ER("DTM :dtm_comm_socket_recv failed err :%s", strerror(err));
			rc = NCSCC_RC_FAILURE;
		}

	}
	TRACE_LEAVE2("rc : %d", rc);
	return rc;
}

#if 0
/*********************************************************

  Function NAME: 
  DESCRIPTION:

  ARGUMENTS:

  RETURNS: 

*********************************************************/
static char *comm_get_foreign_address(int sock_desc)
{
	struct addrinfo addr;
	unsigned int addr_len = sizeof(addr);
	TRACE_ENTER();

	if (getpeername(sock_desc, (sockaddr *) & addr, (socklen_t *)&addr_len) < 0) {
		LOG_ER("DTM :Fetch of foreign address failed (getpeername()) err :%s", strerror(errno));
		return NULL;
	}
	TRACE_LEAVE();
	return inet_ntoa(addr.ai_addr);
}
#endif

/**
 * Setup the new communication socket
 *
 * @param dtms_cb foreign_address foreign_port ip_addr_type
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int comm_socket_setup_new(DTM_INTERNODE_CB * dtms_cb, const char *foreign_address, in_port_t foreign_port,
			  DTM_IP_ADDR_TYPE ip_addr_type)
{
	int sock_desc = -1, size = DTM_INTERNODE_SOCK_SIZE;
	int err = 0, rv;
	char local_port_str[INET6_ADDRSTRLEN];
	struct addrinfo *addr_list;
	struct addrinfo addr_criteria, *p;	/* Criteria for address match */

	TRACE_ENTER();

	/* Construct the serv address structure */
	TRACE("DTM:dgram_port_rcvr :%d", dtms_cb->dgram_port_rcvr);
	snprintf(local_port_str, sizeof(local_port_str), "%d", foreign_port);

	/* Construct the serv address structure */
	memset(&addr_criteria, 0, sizeof(addr_criteria));	/* Zero out structure */
	addr_criteria.ai_family = AF_UNSPEC;	/* v4 or v6 is OK */
	addr_criteria.ai_socktype = SOCK_STREAM;	/* Only streaming sockets */
	addr_criteria.ai_protocol = IPPROTO_TCP;	/* Only TCP protocol */
	addr_criteria.ai_flags |= AI_NUMERICHOST;

	TRACE("DTM:foreign_address : %s local_port_str :%s", foreign_address, local_port_str);
	if ((rv = getaddrinfo(foreign_address, local_port_str, &addr_criteria, &addr_list)) != 0) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));		
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;

	}

	if (addr_list == NULL) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));		
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* results and bind to the first we can */
	p = addr_list;

	TRACE("DTM : family : %d, socktype : %d, protocol :%d", p->ai_family, p->ai_socktype, p->ai_protocol);
	/* Create socket for sending multicast datagrams */
	if ((sock_desc = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == SOCKET_ERROR()) {
		LOG_ER("DTM:Socket creation failed (socket()) err :%s", strerror(errno));
		goto done;
	}

	if (set_nonblocking(sock_desc, true) != true) {
		LOG_ER("DTM :set_nonblocking failed ");
		dtm_comm_socket_close(&sock_desc);
		goto done;
	}

	if (setsockopt(sock_desc, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM:Socket rcv buf size set failed err :%s", strerror(errno));
		dtm_comm_socket_close(&sock_desc);
		goto done;
	}

	if (setsockopt(sock_desc, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM:Socket snd buf size set failed err :%s", strerror(errno));
		dtm_comm_socket_close(&sock_desc);
		goto done;
	}
	if (NCSCC_RC_SUCCESS != set_keepalive(dtms_cb, sock_desc)) {
		LOG_ER("DTM :set_keepalive failed ");
		dtm_comm_socket_close(&sock_desc);
		goto done;
	}

	/* Try to connect to the given port */
	if (connect(sock_desc, addr_list->ai_addr, addr_list->ai_addrlen) < 0) {
		err = errno;
		if (!IS_BLOCKIN_ERROR(err)) {
			LOG_ER("DTM :Connect failed (connect()) err :%s", strerror(err));
			dtm_comm_socket_close(&sock_desc);

		}

	}

	/* Free address structure(s) allocated by getaddrinfo() */
	freeaddrinfo(addr_list);

 done:
	TRACE_LEAVE2("sock_desc : %d", sock_desc);
	return sock_desc;
}

/**
 * Bind the socket
 *
 * @param dtms_cb stream_addr
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
#define MAXPENDING  20

static uint32_t stream_sock_bind(DTM_INTERNODE_CB * dtms_cb, struct addrinfo *stream_addr)
{

	/* Bind to the local address and set socket to list */
	TRACE_ENTER();
	if ((bind(dtms_cb->stream_sock, stream_addr->ai_addr, stream_addr->ai_addrlen) == 0) &&
	    (listen(dtms_cb->stream_sock, MAXPENDING) == 0)) {
		/* get local address of socket */
		struct sockaddr_storage local_addr;
		socklen_t addr_size = sizeof(local_addr);
		if (getsockname(dtms_cb->stream_sock, (struct sockaddr *)&local_addr, &addr_size) < 0) {
			LOG_ER("DTM : getsockname() failed err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			return NCSCC_RC_FAILURE;
		}

		TRACE("DTM : Binding done for : %d", dtms_cb->stream_sock);

	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Stream the non blocking listner
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_stream_nonblocking_listener(DTM_INTERNODE_CB * dtms_cb)
{
	struct addrinfo addr_criteria;	/* Criteria for address match */
	char local_port_str[6];
	struct addrinfo *addr_list = NULL, *p;;	/* List of serv addresses */
	int size = DTM_INTERNODE_SOCK_SIZE;
	int rv;

	dtms_cb->stream_sock = -1;
	TRACE_ENTER();
	/* Construct the serv address structure */

	memset(&addr_criteria, 0, sizeof(addr_criteria));	/* Zero out structure */
	addr_criteria.ai_family = AF_UNSPEC;	/* Any address family */
	addr_criteria.ai_socktype = SOCK_STREAM;	/* Only stream sockets */
	addr_criteria.ai_protocol = IPPROTO_TCP;	/* Only TCP protocol */
	addr_criteria.ai_flags |= AI_NUMERICHOST;

	TRACE("DTM :stream_port :%d", dtms_cb->stream_port);
	/* snprintf(local_port_str, sizeof(local_port_str), "%d", htons(dtms_cb->stream_port)); */
	snprintf(local_port_str, sizeof(local_port_str), "%d", dtms_cb->stream_port);

	TRACE("DTM :ip_addr : %s local_port_str -%s", dtms_cb->ip_addr, local_port_str);
	if ((rv = getaddrinfo(dtms_cb->ip_addr, local_port_str, &addr_criteria, &addr_list)) != 0) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));		
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;

	}

	if (addr_list == NULL) {
		TRACE("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));		
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* results and bind to the first we can */
	p = addr_list;

	TRACE("DTM :family : %d, socktype : %d, protocol :%d", p->ai_family, p->ai_socktype, p->ai_protocol);
	/* Create socket for sending multicast datagrams */
	if ((dtms_cb->stream_sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == SOCKET_ERROR()) {
		LOG_ER("DTM:Socket creation failed (socket()) err :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}


	if (set_nonblocking(dtms_cb->stream_sock, true) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : set_nonblocking() failed");
		dtm_sockdesc_close(dtms_cb->stream_sock);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (setsockopt(dtms_cb->stream_sock, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM:Socket rcv buf size set failed err :%s", strerror(errno));
	}

	if (setsockopt(dtms_cb->stream_sock, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM:Socket snd buf size set failed err :%s", strerror(errno));
	}

	if (set_keepalive(dtms_cb, dtms_cb->stream_sock) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : set_keepalive() failed");
		dtm_sockdesc_close(dtms_cb->stream_sock);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (stream_sock_bind(dtms_cb, p) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : stream_sock_bind() failed");
		dtm_sockdesc_close(dtms_cb->stream_sock);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Free address structure(s) allocated by getaddrinfo() */
	freeaddrinfo(addr_list);

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/**
 * Function to listen to mcast message
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_mcast_listener(DTM_INTERNODE_CB * dtms_cb)
{

	/* Construct the serv address structure */
	struct addrinfo addr_criteria, *p;	/* Criteria for address match */
	char local_port_str[INET6_ADDRSTRLEN];
	int rv;
	struct addrinfo *addr_list;	/* Holder serv address */
	TRACE_ENTER();

	TRACE("DTM :dgram_port_rcvr :%d", dtms_cb->dgram_port_rcvr);
	snprintf(local_port_str, sizeof(local_port_str), "%d", (dtms_cb->dgram_port_rcvr));

	dtms_cb->dgram_sock_rcvr = -1;

	memset(&addr_criteria, 0, sizeof(addr_criteria));	/* Zero out structure */
	addr_criteria.ai_family = AF_UNSPEC;	/* v4 or v6 is OK */
	addr_criteria.ai_socktype = SOCK_DGRAM;	/* Only datagram sockets */
	addr_criteria.ai_protocol = IPPROTO_UDP;	/* Only UDP protocol */
	addr_criteria.ai_flags |= AI_NUMERICHOST;	/* Don't try to resolve address */

	TRACE("DTM :mcast_addr : %s local_port_str :%s", dtms_cb->mcast_addr, local_port_str);
	if ((rv = getaddrinfo(dtms_cb->mcast_addr, local_port_str, &addr_criteria, &addr_list)) != 0) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));		
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;

	}

	if (addr_list == NULL) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));	
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* results and bind to the first we can */
	p = addr_list;

	TRACE("DTM :family : %d, socktype : %d, protocol :%d", p->ai_family, p->ai_socktype, p->ai_protocol);
	/* Create socket for sending multicast datagrams */
	if ((dtms_cb->dgram_sock_rcvr = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==  SOCKET_ERROR()) {
		LOG_ER("DTM:Socket creation failed (socket()) err :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (dtms_cb->dgram_sock_rcvr == -1) {
		LOG_ER("DTM:Socket creation failed (socket())");
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (bind(dtms_cb->dgram_sock_rcvr, p->ai_addr, p->ai_addrlen) < 0) {
		LOG_ER("DTM : bind() failed err :%s ", strerror(errno));
		dtm_sockdesc_close(dtms_cb->dgram_sock_rcvr);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (dgram_join_mcast_group(dtms_cb, p) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : dgram_join_mcast_group() failed");
		dtm_sockdesc_close(dtms_cb->dgram_sock_rcvr);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Free address structure(s) allocated by getaddrinfo() */
	freeaddrinfo(addr_list);
	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Function to send the mcast message
 *
 * @param dtms_cb mcast_ttl
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_mcast_sender(DTM_INTERNODE_CB * dtms_cb, int mcast_ttl)
{

	/* Construct the serv address structure */
	struct addrinfo addr_criteria, *p;	// Criteria for address match
	char local_port_str[INET6_ADDRSTRLEN];
	int rv;
	TRACE_ENTER();

	dtms_cb->dgram_sock_sndr = -1;

	TRACE("DTM :dgram_port_rcvr :%d", dtms_cb->dgram_port_rcvr);
	snprintf(local_port_str, sizeof(local_port_str), "%d", (dtms_cb->dgram_port_rcvr));

	memset(&addr_criteria, 0, sizeof(addr_criteria));	/* Zero out structure */
	addr_criteria.ai_family = AF_UNSPEC;	/* v4 or v6 is OK */
	addr_criteria.ai_socktype = SOCK_DGRAM;	/* Only datagram sockets */
	addr_criteria.ai_protocol = IPPROTO_UDP;	/* Only UDP please */
	addr_criteria.ai_flags |= AI_NUMERICHOST;	/* Don't try to resolve address */

	TRACE("DTM :mcast_addr : %s local_port_str :%s", dtms_cb->mcast_addr, local_port_str);
	if ((rv = getaddrinfo(dtms_cb->mcast_addr, local_port_str, &addr_criteria, &mcast_sender_addr)) != 0) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;

	}

	if (mcast_sender_addr == NULL) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));	
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);		
		return NCSCC_RC_FAILURE;
	}

	/* results and bind to the first we can */
	p = mcast_sender_addr;

	TRACE("DTM :family : %d, socktype : %d, protocol :%d", p->ai_family, p->ai_socktype, p->ai_protocol);
	/* Create socket for sending multicast datagrams */
	if ((dtms_cb->dgram_sock_sndr = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==  SOCKET_ERROR()) {
		LOG_ER("DTM:Socket creation failed (socket()) err :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}


	if (dgram_set_mcast_ttl(dtms_cb, mcast_ttl) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : dgram_set_mcast_ttl() failed");
		dtm_sockdesc_close(dtms_cb->dgram_sock_rcvr);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Free address structure(s) allocated by getaddrinfo() */
	/*freeaddrinfo(mcast_sender_addr); */

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;

}

/**
 * Function to get the address
 *
 * @param sa
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in *)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

/**
 * Function to listen to the bcast message
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_bcast_listener(DTM_INTERNODE_CB * dtms_cb)
{

	struct addrinfo addr_criteria, *addr_list, *p;	// Criteria for address
	char local_port_str[INET6_ADDRSTRLEN];
	int rv;
	TRACE_ENTER();

	TRACE("DTM :dgram_port_rcvr :%d", dtms_cb->dgram_port_rcvr);
	snprintf(local_port_str, sizeof(local_port_str), "%d", (dtms_cb->dgram_port_rcvr));

	dtms_cb->dgram_sock_rcvr = -1;
	/*  Construct the server address structure */
	memset(&addr_criteria, 0, sizeof(addr_criteria));	// Zero out structure
	addr_criteria.ai_family = AF_UNSPEC;	// Any address family
	addr_criteria.ai_flags = AI_PASSIVE;	// Accept on any address/port
	addr_criteria.ai_socktype = SOCK_DGRAM;	// Only datagram socket
	addr_criteria.ai_protocol = IPPROTO_UDP;	// Only UDP socket
	addr_criteria.ai_flags |= AI_NUMERICHOST;

	TRACE("DTM :ip_addr : %s local_port_str :%s", dtms_cb->ip_addr, local_port_str);

	/* FIX ME */
	/*if ((rv = getaddrinfo(dtms_cb->ip_addr, local_port_str, &addr_criteria, &addr_list)) != 0)  */
	if ((rv = getaddrinfo(NULL, local_port_str, &addr_criteria, &addr_list)) != 0) {
		LOG_ER("DTM:Unable to getaddrinfo() rtn_val :%d err :%s", rv, strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;

	}

	if (addr_list == NULL) {
		LOG_ER("DTM:Unable to get addr_list ");
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* results and bind to the first we can */
	p = addr_list;

	for (; p; p = p->ai_next) {

		TRACE("DTM :family : %d, socktype : %d, protocol :%d", p->ai_family, p->ai_socktype, p->ai_protocol);
		if (dtms_cb->i_addr_family != p->ai_family) {
			continue;
		}

		if ((dtms_cb->dgram_sock_rcvr = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) ==  SOCKET_ERROR()) {
			LOG_ER("DTM:Socket creation failed (socket()) err :%s", strerror(errno));
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			continue;
		}

		if (bind(dtms_cb->dgram_sock_rcvr, p->ai_addr, p->ai_addrlen) == -1) {
			LOG_ER("DTM:Socket bind failed  err :%s", strerror(errno));
			close(dtms_cb->dgram_sock_rcvr);
			TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
			perror("listener: bind");
			return NCSCC_RC_FAILURE;
		} else {

			break;
		}

	}

	/* Free address structure(s) allocated by getaddrinfo() */
	freeaddrinfo(addr_list);

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/**
 * Function for sending the bcast
 *
 * @param dtms_cb
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
uint32_t dtm_dgram_bcast_sender(DTM_INTERNODE_CB * dtms_cb)
{

	const char *IN6ADDR_ALLNODES = "FF02::1";	/* v6 addr not built in */
	TRACE_ENTER();

	dtms_cb->dgram_sock_sndr = -1;
	bcast_sen_addr_size = 0;
	memset(&bcast_dest_storage, 0, sizeof(bcast_dest_storage));

	if (dtms_cb->i_addr_family == DTM_IP_ADDR_TYPE_IPV4) {

		/* Holder for bcast_dest_address address */
		uint32_t rc = 0;
		struct sockaddr_in *bcast_sender_addr_in = (struct sockaddr_in *)&bcast_dest_storage;
		bcast_sender_addr_in->sin_family = AF_INET;
		bcast_sender_addr_in->sin_port = htons((dtms_cb->dgram_port_rcvr));
		rc = inet_pton(AF_INET, dtms_cb->bcast_addr, &bcast_sender_addr_in->sin_addr);
		if (1 != rc) {
			LOG_ER("DTM : inet_pton failed");
			TRACE_LEAVE2("rc :%d", rc);
			return NCSCC_RC_FAILURE;
		}
		
		memset(bcast_sender_addr_in->sin_zero, '\0', sizeof bcast_sender_addr_in->sin_zero);
		bcast_sen_addr_size = sizeof(struct sockaddr_in);

	} else if (dtms_cb->i_addr_family == DTM_IP_ADDR_TYPE_IPV6) {

		/* Holder for bcast_dest_address address */
		struct sockaddr_in6 *bcast_sender_addr_in6 = (struct sockaddr_in6 *)&bcast_dest_storage;
		bcast_sender_addr_in6->sin6_family = AF_INET6;
		bcast_sender_addr_in6->sin6_port = htons((dtms_cb->dgram_port_rcvr));
		inet_pton(AF_INET6, IN6ADDR_ALLNODES, &bcast_sender_addr_in6->sin6_addr);
		bcast_sen_addr_size = sizeof(struct sockaddr_in6);

	} else {
		LOG_ER("DTM : dgram_enable_bcast failed");
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	/* Create socket for sending/receiving datagrams */
	dtms_cb->dgram_sock_sndr = socket(bcast_dest_address->sa_family, SOCK_DGRAM, IPPROTO_UDP);
	if (dtms_cb->dgram_sock_sndr ==  SOCKET_ERROR()) {
		LOG_ER("DTM :socket create  failederr :%s", strerror(errno));
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	if (dgram_enable_bcast(dtms_cb->dgram_sock_sndr) != NCSCC_RC_SUCCESS) {
		LOG_ER("DTM : dgram_enable_bcast failed");
		dtm_sockdesc_close(dtms_cb->dgram_sock_sndr);
		TRACE_LEAVE2("rc :%d", NCSCC_RC_FAILURE);
		return NCSCC_RC_FAILURE;
	}

	TRACE_LEAVE2("rc :%d", NCSCC_RC_SUCCESS);
	return NCSCC_RC_SUCCESS;
}

/**
 * Function for dtm connect process
 *
 * @param dtms_cb node_ip data len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int dtm_process_connect(DTM_INTERNODE_CB * dtms_cb, char *node_ip, uint8_t *data, uint16_t len)
{

	in_port_t foreign_port;
	int sock_desc = -1;
	DTM_NODE_DB node = { 0 };
	DTM_NODE_DB *new_node = NULL;
	uint8_t *buffer = data, mcast_flag;
	DTM_IP_ADDR_TYPE ip_addr_type = 0;
	TRACE_ENTER();

	memset(&node, 0, sizeof(DTM_NODE_DB));

	/* Decode start */
	node.cluster_id = ncs_decode_32bit(&buffer);
	node.node_id = ncs_decode_32bit(&buffer);
	if (dtms_cb->node_id == node.node_id) {
		if (dtms_cb->mcast_flag != true) {
			TRACE("DTM: received the self node_id bcast  message,  droping message");
		} else {
			TRACE("DTM: received the self node_id mcast message,  droping message");
		}
		TRACE_LEAVE2("sock_desc :%d", sock_desc);
		return sock_desc;
	}

	/* Decode end */
	if (node.cluster_id != dtms_cb->cluster_id) {
		TRACE("DTM:cluster_id  mis match  droping message");
		TRACE_LEAVE2("sock_desc :%d", sock_desc);
		return sock_desc;
	}

	mcast_flag = ncs_decode_8bit(&buffer);
	TRACE("mcast flag: %" PRIu8, mcast_flag);

	/* foreign_port = htons((in_port_t)(ncs_decode_16bit(&buffer))); */
	foreign_port = ((in_port_t)(ncs_decode_16bit(&buffer)));
	ip_addr_type = ncs_decode_8bit(&buffer);
	memcpy(node.node_ip, buffer, INET6_ADDRSTRLEN);

	if (initial_discovery_phase == true) {
		if (node.node_id < dtms_cb->node_id) {
			TRACE("DTM: received node_id is less than local node_id  droping message");
			return sock_desc;
		}

	}

	new_node = dtm_node_get_by_id(node.node_id); 
	/*new_node = dtm_node_get_by_node_ip((uint8_t *)node.node_ip);*/

	if (new_node != NULL) {
		if (((new_node->node_id == 0) || (new_node->node_id == node.node_id)) &&
		    (strncmp(node.node_ip, new_node->node_ip, INET6_ADDRSTRLEN) == 0) &&
		    (new_node->comm_status == false)) {
			TRACE("DTM:new_node  discovery in progress droping message");
			TRACE_LEAVE2("sock_desc :%d", sock_desc);
			return sock_desc;
		} else if ((new_node->comm_status == true) && (new_node->node_id == node.node_id) &&
			   (strncmp(node.node_ip, new_node->node_ip, INET6_ADDRSTRLEN) == 0)) {
			TRACE("DTM:new_node node already discovered droping message");
			TRACE_LEAVE2("sock_desc :%d", sock_desc);
			return sock_desc;
		} else if ((new_node->comm_status == false) &&
			   ((new_node->node_id != node.node_id) ||
			    (strncmp(node.node_ip, new_node->node_ip, INET6_ADDRSTRLEN) != 0))) {
			TRACE("DTM:new_node deleting stale enty ");
			if (dtm_node_delete(new_node, 0) != NCSCC_RC_SUCCESS) {
				LOG_ER("DTM :dtm_node_delete failed (recv())");
			}
			if (dtm_node_delete(new_node, 1) != NCSCC_RC_SUCCESS) {
				LOG_ER("DTM :dtm_node_delete failed (recv())");
			}
			if (dtm_node_delete(new_node, 2) != NCSCC_RC_SUCCESS) {
				LOG_ER("DTM :dtm_node_delete failed (recv())");
			}
			free(new_node);
		}
	}

	new_node = dtm_node_new(&node);

	if (new_node == NULL) {
		LOG_ER(" dtm_node_new failed .node_ip : %s ", node.node_ip);
		sock_desc = -1;
		goto node_fail;
	}

	sock_desc = comm_socket_setup_new(dtms_cb, (char *)&node.node_ip, foreign_port, ip_addr_type);

	new_node->comm_socket = sock_desc;
	new_node->node_id = node.node_id;
	memcpy(new_node->node_ip, node.node_ip, INET6_ADDRSTRLEN);

	if (sock_desc != -1) {

		if (dtm_node_add(new_node, 0) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM: dtm_node_add failed .node_ip : %s ", new_node->node_ip);
			dtm_comm_socket_close(&sock_desc);
			sock_desc = -1;
			free(new_node);
			goto node_fail;

		}

		if (dtm_node_add(new_node, 1) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM:dtm_node_add failed .node_ip : %s ", new_node->node_ip);
			dtm_comm_socket_close(&sock_desc);
			sock_desc = -1;
			free(new_node);
			goto node_fail;

		}

		if (dtm_node_add(new_node, 2) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM:dtm_node_add  failed .node_ip : %s ", new_node->node_ip);
			dtm_comm_socket_close(&sock_desc);
			sock_desc = -1;
			free(new_node);
			goto node_fail;

		}
	}

 node_fail:
	TRACE_LEAVE2("sock_desc :%d", sock_desc);
	return sock_desc;

}

#define DTM_INTERNODE_SOCK_SIZE 64000

/**
 * Function for dtm accept the connection
 *
 * @param dtms_cb stream_sock
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int dtm_process_accept(DTM_INTERNODE_CB * dtms_cb, int stream_sock)
{
	struct sockaddr_storage clnt_addr;	/* Client address */
	/* Set length of client address structure (in-out parameter) */
	socklen_t clnt_addrLen = sizeof(clnt_addr);
	void *numericAddress = NULL;	/* Pointer to binary address */
	char addrBuffer[INET6_ADDRSTRLEN];
	int err = 0;
	DTM_NODE_DB node;
	DTM_NODE_DB *new_node;
	int new_conn_sd, size = DTM_INTERNODE_SOCK_SIZE;
	const struct sockaddr *clnt_addr1 = (struct sockaddr *)&clnt_addr;
	TRACE_ENTER();

	memset(&node, 0, sizeof(DTM_NODE_DB));

	if ((new_conn_sd = accept(stream_sock, (struct sockaddr *)&clnt_addr, &clnt_addrLen)) < 0) {

		err = errno;
		if (!IS_BLOCKIN_ERROR(err)) {
			LOG_ER("DTM:Accept failed (accept()) err :%s", strerror(err));
			new_conn_sd = -1;
			goto done;
		}

	}

	if (setsockopt(new_conn_sd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM: Unable to set the SO_RCVBUF ");
		dtm_comm_socket_close(&new_conn_sd);
		goto done;
	}
	if (setsockopt(new_conn_sd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size)) != 0) {
		LOG_ER("DTM: Unable to set the SO_SNDBUF ");
		dtm_comm_socket_close(&new_conn_sd);
		goto done;
	}
	if (NCSCC_RC_SUCCESS != set_keepalive(dtms_cb, new_conn_sd)) {
		LOG_ER("DTM:set_keepalive failed ");
		dtm_comm_socket_close(&new_conn_sd);
		goto done;
	}

	if (clnt_addr1->sa_family == AF_INET) {
		numericAddress = &((struct sockaddr_in *)clnt_addr1)->sin_addr;
	} else if (clnt_addr1->sa_family == AF_INET6) {
		numericAddress = &((struct sockaddr_in6 *)clnt_addr1)->sin6_addr;
	} else {
	     	LOG_ER("DTM: AF not supported=%d", clnt_addr1->sa_family);
		dtm_comm_socket_close(&new_conn_sd);
		osafassert(0);
	}

	/* Convert binary to printable address */
	if (inet_ntop(clnt_addr1->sa_family, numericAddress, addrBuffer, sizeof(addrBuffer)) == NULL) {
		LOG_ER("DTM:[invalid address] %s ", addrBuffer);
		dtm_comm_socket_close(&new_conn_sd);
		goto done;
	} else {
		memcpy(node.node_ip, (uint8_t *)addrBuffer, INET6_ADDRSTRLEN);
	}

	if (new_conn_sd != -1) {

		node.cluster_id = dtms_cb->cluster_id;
		node.comm_socket = new_conn_sd;

		new_node = dtm_node_new(&node);

		if (new_node == NULL) {
			LOG_ER(" DTM: dtm_node_new failed .node_ip : %s ", node.node_ip);
			dtm_comm_socket_close(&new_conn_sd);
			goto node_fail;
		}

		if (dtm_node_add(new_node, 1) != NCSCC_RC_SUCCESS) {
			LOG_ER("DTM: dtm_node_add failed .node_ip : %s ", node.node_ip);
			dtm_comm_socket_close(&new_conn_sd);
			goto node_fail;
		}

		if (dtm_node_add(new_node, 2) != NCSCC_RC_SUCCESS) {

			LOG_ER("DTM: dtm_node_add failed .node_ip : %s ", node.node_ip);
			dtm_comm_socket_close(&new_conn_sd);
			goto node_fail;
		}
	}

 done:
 node_fail:
	TRACE_LEAVE2("DTM: new_conn_sd :%d", new_conn_sd);
	return new_conn_sd;
}

/**
 * Function to rcv the bcast message
 *
 * @param dtms_cb node_ip buffer buffer_len
 *
 * @return NCSCC_RC_SUCCESS
 * @return NCSCC_RC_FAILURE
 *
 */
int dtm_dgram_recvfrom_bmcast(DTM_INTERNODE_CB * dtms_cb, char *node_ip, void *buffer, int buffer_len)
{
	struct sockaddr_storage clnt_addr;
	void *numericAddress = NULL;	/* Pointer to binary address */
	char addrBuffer[INET6_ADDRSTRLEN];
	socklen_t addrLen = sizeof(clnt_addr);
	int rtn;
	TRACE_ENTER();

	memset(node_ip, 0, INET6_ADDRSTRLEN);

	if ((rtn = recvfrom(dtms_cb->dgram_sock_rcvr, (raw_type *) buffer, buffer_len, 0,
			    (struct sockaddr *)&clnt_addr, (socklen_t *)&addrLen)) < 0) {
	
		LOG_ER("DTM:Receive failed (recvfrom()) err :%s", strerror(errno));

	} else {
		const struct sockaddr *clnt_addr1 = (struct sockaddr *)&clnt_addr;

		if (clnt_addr1->sa_family == AF_INET) {
			numericAddress = &((struct sockaddr_in *)clnt_addr1)->sin_addr;
		}

		else if (clnt_addr1->sa_family == AF_INET6) {

			numericAddress = &((struct sockaddr_in6 *)clnt_addr1)->sin6_addr;
		} else {
			LOG_ER("DTM:AF not supported=%d", clnt_addr1->sa_family);
			osafassert(0);
		}

		/* Convert binary to printable address */
		if (inet_ntop(clnt_addr1->sa_family, numericAddress, addrBuffer, sizeof(addrBuffer)) == NULL) {
			TRACE("DTM: invalid address :%s", addrBuffer);
		} else {
			memcpy(node_ip, (char *)addrBuffer, INET6_ADDRSTRLEN);
		}
	}

	TRACE_LEAVE2("rc :%d", rtn);
	return rtn;
}

#if 0
/*********************************************************

  Function NAME: 
DESCRIPTION:

ARGUMENTS:

RETURNS: 

 *********************************************************/

uint32_t dtm_get_sa_family(DTM_INTERNODE_CB * dtms_cb)
{

	uint32_t rc = NCSCC_RC_SUCCESS;
	char local_port_str[INET6_ADDRSTRLEN];
	struct addrinfo addr_criteria;	/* Criteria for address match */

	TRACE_ENTER();

	char *addr_string = dtms_cb->ip_addr;	/* Server address/name */

	TRACE("DTM :stream_port :%d", dtms_cb->stream_port);
	snprintf(local_port_str, sizeof(local_port_str), "%d", (dtms_cb->stream_port));

	/* Tell the system what kind(s) of address info we want */
	memset(&addr_criteria, 0, sizeof(addr_criteria));	/* Zero out structure */
	addr_criteria.ai_family = AF_UNSPEC;	/* Any address family */
	addr_criteria.ai_socktype = SOCK_STREAM;	/* Only stream sockets */
	addr_criteria.ai_protocol = IPPROTO_TCP;	/* Only TCP protocol */
	addr_criteria.ai_flags |= AI_NUMERICHOST;

	/* Get address(es) associated with the specified name/service */
	struct addrinfo *addr_list;	/* Holder for list of addresses returned */
	/* Modify servAddr contents to reference linked list of addresses */
	TRACE("DTM :addr_string : %s local_port_str :%s", addr_string, local_port_str);
	int rtn_val = getaddrinfo(addr_string, local_port_str, &addr_criteria, &addr_list);
	if (rtn_val != 0)
		LOG_ER("getaddrinfo() failed %d", rtn_val);

	/* Display returned addresses */
	struct addrinfo *addr;
	for (addr = addr_list; addr != NULL; addr = addr->ai_next) {

		if (addr->ai_addr->sa_family == AF_INET) {
			dtms_cb->i_addr_family = DTM_IP_ADDR_TYPE_IPV4;
			TRACE("DTM :dtms_cb->i_addr_family :%d", dtms_cb->i_addr_family);
			goto done;
		} else if (addr->ai_addr->sa_family == AF_INET6) {
			dtms_cb->i_addr_family = DTM_IP_ADDR_TYPE_IPV6;
			TRACE("DTM :dtms_cb->i_addr_family :%d", dtms_cb->i_addr_family);
			goto done;
		}

		else
			continue;

	}

	rc = NCSCC_RC_FAILURE;
 done:
	/* Free address structure(s) allocated by getaddrinfo() */
	freeaddrinfo(addr_list);
	TRACE_LEAVE2("rc :%d", rc);
	return rc;

}

/*********************************************************

  Function NAME: 
DESCRIPTION:

ARGUMENTS:

RETURNS: 

 *********************************************************/

char *get_local_address(int sock_desc)
{
	TRACE_ENTER();
	struct addrinfo addr;
	unsigned int addr_len = sizeof(addr);

	if (getsockname(sock_desc, (sockaddr *) & addr, (socklen_t *)&addr_len) < 0) {
		LOG_ER("DTM :Fetch of local address failed (getsockname())");
		return NULL;
	}
	TRACE_LEAVE();
	return inet_ntoa(addr.sin_addr);
}

unsigned short get_local_port(int sock_desc)
{
	TRACE_ENTER();
	struct addrinfo addr;
	unsigned int addr_len = sizeof(addr);

	if (getsockname(sock_desc, (sockaddr *) & addr, (socklen_t *)&addr_len) < 0) {
		LOG_ER("DTM :Fetch of local port failed (getsockname())");
		return 0;
	}
	TRACE_LEAVE();
	return ntohs(addr.sin_port);
}

/*********************************************************

  Function NAME: 
DESCRIPTION:

ARGUMENTS:

RETURNS: 

 *********************************************************/
uint32_t dgram_set_mcastloop(bool x)
{

	TRACE_ENTER();
	if (mcast_sender_addr->ai_family == AF_INET6) {
		int val = x ? 1 : 0;
		if (setsockopt(dtms_cb->dgram_sock_rcvr, IPPROTO_IPV6, IPV6_MULTICAST_LOOP, (char *)&val, sizeof(int))
		    == -1) {
			LOG_ER("DTM:SetMulticastLoop(ipv6)");
		}
		return;
	} else if (mcast_sender_addr->ai_family == AF_INET) {

		int val = x ? 1 : 0;
		if (setsockopt(dtms_cb->dgram_sock_rcvr, SOL_IP, IP_MULTICAST_LOOP, (char *)&val, sizeof(int)) == -1) {
			LOG_ER("DTM:SetMulticastLoop(ipv4)");
		}

	} else {
		LOG_ER("DTM:Unable to set set_mcastloop invalid address family");
	}

	TRACE_LEAVE();
	return NCSCC_RC_SUCCESS;

}

#endif
