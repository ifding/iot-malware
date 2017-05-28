/*
 * synscan.c - a simple TCP SYN Scanner for Linux
 *
 * Copyright (c) 2002, 2010, Felix Opatz
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 *  * Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *  * Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH
 * DAMAGE.
 *
 * This simple port scanner emits TCP SYN datagrams and waits for replies.
 * If a TCP SYN/ACK datagram is received, the port is considered as open. If
 * either a TCP RST datagram is received or a timeout elapses, the port is
 * considered as closed. There can be multiple outstanding SYN packets
 * travelling around, i.e. the tool doesn't wait for one packet to return until
 * the next one is sent. The number of parallel packets can be defined by a
 * preprocessor constant.
 *
 * Please note that this program works on Linux only. Although it is compiling
 * on FreeBSD it will not work, because the *BSD raw sockets for TCP don't
 * receive packets.
 *
 * Usage: synscan [-h] [-n] [-p port(s)] <source> <destination>
 *
 * -h      Print help and exit
 * -n      Do not resolve port names in report
 * -p      Ports to scan (single port or range in format 'a-b')
 *
 * Destination can be an IP address or a hostname. Source address must be the
 * IP address of the interface from which the packets are sent. If no ports
 * are specified the well-known ports from 1-1023 are scanned.
 *
 *
 * Date        Change  
 * ---------------------------------------------------------------------------
 * 2010-05-21  Redesign with improved packet handling.
 * 2002-05-12  Initial implementation.
 */

//kbuild:lib-$(CONFIG_SYNSCAN) += synscan.o
//config:config SYNSCAN
//config:	bool "synscan"
//config:	default y
//config:	help
//config:	  Returns an indeterminate value.

#include "libbb.h"
//#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#define __FAVOR_BSD
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <unistd.h>

// Default values for options
#define DEF_PORT_BEGIN  1
#define DEF_PORT_END    1023

// Minimum and maximum values
#define MIN_PORT        1     // Lowest port number
#define MAX_PORT        65535 // Highest port number
#define MAX_PACKET_LEN  65536 // Maximum packet length to be received
#define MAX_PENDING     10    // Maximum count of pending packets
#define PACKET_TIMEOUT  5     // Timeout for pending packets, in seconds

// Configuration from command line
struct config
{
	const char     *dest_name;  // Destination as specified by user
	const char     *src_name;   // Source as specified by user
	unsigned short  port_begin; // Begin of port range
	unsigned short  port_end;   // End of port range
	int             help;       // Print help and exit
	int             numeric;    // Output in numeric form
};

// State information of a single port.
// Used in an array where the port number is index.
struct port_info
{
	unsigned char  flags;   // State of port as flags
#define FLAG_OPEN    0x01       // Port is observed open
#define FLAG_CLOSED  0x02       // Port is observed closed
#define FLAG_PENDING 0x04       // Packet is sent and pending
#define FLAG_LOST    0x08       // Packet has timed out
	time_t         tx_time; // Timestamp of transmission
};

// Runtime information of a scan session
struct scan_session
{
	int               sockfd;     // Socket descriptor
	struct in_addr    src_addr;   // IP address of source interface
	const char       *dest_name;  // Destination as specified by user
	struct in_addr    dest_addr;  // IP address of destiantion
	unsigned short    port_begin; // Begin of port range
	unsigned short    port_end;   // End of port range
	int               numeric;    // Output in numeric form
	struct port_info  ports[MAX_PORT+1]; // State of ports
};

// TCP Pseudo Header used for checksum calculation
struct pseudohdr
{
	uint32_t  saddr;   // Source address
	uint32_t  daddr;   // Destination address
	uint8_t   zero;    // Must be zero
	uint8_t   proto;   // Protocol number
	uint16_t  tcp_len; // Length of TCP datagram
};

/* Reads the port specification and fills the data into the config struct.
 * Returns 0 if successful, 1 otherwise.
 *
 * Valid input formats are:
 * "a" -> begin = a, end = a
 * "a-b" -> begin = a, end = b
 * Both a and b must be in the range 1..65535
 * a > b is accepted and reversed
 */
int insert_ports(const char *spec, struct config *cfg)
{
	int failed = 0;
	int begin, end;

	if (sscanf(spec, "%d-%d", &begin, &end) == 2)
	{
		if (begin > end)
		{
			int tmp = end;
			end = begin;
			begin = tmp;
		}
	}
	else if (sscanf(spec, "%d", &begin) == 1)
		end = begin;
	else
		failed = 1;

	if ((begin < MIN_PORT) || (begin > MAX_PORT))
		failed = 1;
	if ((end < MIN_PORT) || (end > MAX_PORT))
		failed = 1;

	if (!failed)
	{
		cfg->port_begin = begin;
		cfg->port_end = end;
	}

	return failed;
}

// Gets options from command line. Returns 0 if successful, 1 otherwise.
int get_opts(int argc, char **argv, struct config *cfg)
{
	int failed = 0;
	int c;
	const char *getopt_str = "hnp:";

	while (!failed && (c = getopt(argc, argv, getopt_str)) != -1)
	{
		switch (c)
		{
		case 'h':
			cfg->help = 1;
			break;
		case 'n':
			cfg->numeric = 1;
			break;
		case 'p':
			if (insert_ports(optarg, cfg) != 0)
			{
				fprintf(stderr,
					"Invalid port specification.\n");
				failed = 1;
			}
			break;
		case '?':
			failed = 1;
			break;
		default:
			fprintf(stderr, "Unknown error in getopt().\n");
			abort();
		}
	}

	// First trailing argument is source address
	if (optind < argc)
		cfg->src_name = argv[optind++];

	// Second trailing argument is destination
	if (optind < argc)
		cfg->dest_name = argv[optind++];

	// More trailing arguments are not accepted
	if (optind < argc)
		cfg->help = 1;

	return failed;
}

/* Calculation of checksum for TCP header. Must be used on the TCP datagram
 * with prepended pseudo header. For details of checksum algorithm refer to
 * RFC1071 Computing the Internet Checksum.
 */
static uint16_t in_cksum(const void *addr, size_t len)
{
	uint32_t sum = 0;
	uint16_t *curr_word = (uint16_t*) addr;

	while (len > 1)
	{
		sum += *curr_word++;
		len -= 2;
	}

	if (len > 0)
		sum += *(uint8_t*)curr_word;

	while (sum >> 16)
		sum = (sum & 0xFFFF) + (sum >> 16);

	return ~sum;
}

// Sends a TCP SYN packet for the specified port
int send_packet(struct scan_session *ss, unsigned short port)
{
	unsigned char packet[MAX_PACKET_LEN];
	struct pseudohdr *ph;
	struct tcphdr *tcp;
	struct sockaddr_in addr;
	int bytes;

	memset(packet, 0, sizeof(packet));
	ph = (struct pseudohdr*) packet;
	tcp = (struct tcphdr*) (ph + 1);

	ph->saddr = ss->src_addr.s_addr;   // source address
	ph->daddr = ss->dest_addr.s_addr;  // destination address
	ph->proto = IPPROTO_TCP;           // protocol number from IP header
	ph->tcp_len = htons(sizeof(*tcp)); // length = TCP header, no payload

	tcp->th_sport = htons(getpid());   // use process ID as source port
	tcp->th_dport = htons(port);       // destination port as requested
	tcp->th_seq = rand();              // random sequence number
	tcp->th_off = sizeof(*tcp) / 4;    // size of header in 32-bit-chunks
	tcp->th_flags = TH_SYN;            // send SYN datagrams
	tcp->th_win = htons(1024);         // usual window size

	// Calculate internet checksum over pseudo header and TCP header
	tcp->th_sum = in_cksum(packet, sizeof(*ph) + sizeof(*tcp));

	addr.sin_addr = ss->dest_addr;
	addr.sin_port = IPPROTO_TCP; // irrelevant since Linux 2.2
	addr.sin_family = AF_INET;

	bytes = sendto(ss->sockfd, tcp, sizeof(*tcp), 0,
		(struct sockaddr*) &addr, sizeof(addr));

	if (bytes < 0)
	{
		perror("Sending packet failed");
		return -1;
	}

	// Update port states
	ss->ports[port].flags |= FLAG_PENDING;
	ss->ports[port].tx_time = time(NULL);

	return bytes;
}

// Receive a TCP packet
// Return values:
// -1  error
//  0  packet of somebody else was received, or packet is timed out
// >0  one of our packets was received
int recv_packet(struct scan_session *ss)
{
	unsigned char packet[MAX_PACKET_LEN];
	struct sockaddr_in addr;
	socklen_t addr_len;
	int bytes;
	struct ip *ip;
	struct tcphdr *tcp;
	unsigned short sport;

	addr_len = sizeof(addr);
	bytes = recvfrom(ss->sockfd, packet, sizeof(packet), 0,
		(struct sockaddr*) &addr, &addr_len);

	if (bytes < 0)
	{
		perror("Receiving packet failed");
		return -1;
	}

	// Different source address -> not ours
	if (addr.sin_addr.s_addr != ss->dest_addr.s_addr)
		return 0;

	ip = (struct ip*) packet;
	tcp = (struct tcphdr*) (packet + 4 * ip->ip_hl);

	// Different destination port -> not ours
	if (ntohs(tcp->th_dport) != getpid())
		return 0;

	// Not pending, perhaps already timed out -> not ours
	sport = ntohs(tcp->th_sport);
	if (!(ss->ports[sport].flags & FLAG_PENDING))
		return 0;

	// It is our packet and not timed out. Remove pending flag
	// and interpret the response.
	ss->ports[sport].flags &= ~FLAG_PENDING;
	if (tcp->th_flags & TH_SYN)
		ss->ports[sport].flags |= FLAG_OPEN;
	else
		ss->ports[sport].flags |= FLAG_CLOSED;

	return bytes;
}

// Makes an address out of name. Returns 0 if successful, -1 on error.
int insert_addr(struct in_addr *addr, const char *name)
{
	if (inet_aton(name, addr) == 0)
	{
		struct hostent *host;
		host = gethostbyname(name);
		if (!host)
		{
			fprintf(stderr, "Unable to resolve hostname\n");
			return -1;
		}
		*addr = *(struct in_addr*)host->h_addr;
	}

	return 0;
}

// Prepares a scan by initializing session.
// Returns 0 if successful, -1 on error.
int scan_init(struct scan_session *ss, struct config *cfg)
{
	memset(ss, 0, sizeof(*ss));

	ss->dest_name  = cfg->dest_name;
	ss->port_begin = cfg->port_begin;
	ss->port_end   = cfg->port_end;
	ss->numeric    = cfg->numeric;

	if (insert_addr(&ss->src_addr, cfg->src_name) == -1)
		return -1;
	if (insert_addr(&ss->dest_addr, cfg->dest_name) == -1)
		return -1;

	ss->sockfd = socket(PF_INET, SOCK_RAW, IPPROTO_TCP);
	if (ss->sockfd == -1)
	{
		perror("Unable to create socket");
		return -1;
	}

	return 0;
}

// Begin scan process
int scan_begin(struct scan_session *ss)
{
	printf("Going to scan %s (%s), ",
		inet_ntoa(ss->dest_addr), ss->dest_name);
	if (ss->port_begin != ss->port_end)
		printf("ports %d to %d\n", ss->port_begin, ss->port_end);
	else
		printf("port %d\n", ss->port_begin);

	return 0;
}

// Main loop for sending and receiving packets
int scan_main(struct scan_session *ss)
{
	int port;
	int pend_cnt = 0;
	struct timeval tv;
	time_t now;
	struct port_info *info;
	fd_set fds;
	int result;

	// Loop until all packets are sent and received
	port = ss->port_begin;
	while (port <= ss->port_end || pend_cnt > 0)
	{
		// If it is possible to send more packets, do so
		if (pend_cnt < MAX_PENDING && port <= ss->port_end)
		{
			double ratio;

			if (send_packet(ss, port) < 0)
				return -1;
			pend_cnt++;
			port++;
			ratio = (port - ss->port_begin) * 100.0 /
				(ss->port_end - ss->port_begin + 1);
			printf("\rscanning... %.1f%% complete", ratio);
			fflush(stdout);
			continue;
		}
		
		// Wait for incoming packets, wake up after 1 second
		FD_ZERO(&fds);
		FD_SET(ss->sockfd, &fds);
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		result = select(ss->sockfd + 1, &fds, NULL, NULL, &tv);
		if (result == -1)
			return -1;

		// Find timed out packets and remove pending flag
		now = time(NULL);
		for (info = &ss->ports[ss->port_begin];
			info <= &ss->ports[ss->port_end]; info++)
		{
			if ((info->flags & FLAG_PENDING) &&
				info->tx_time + PACKET_TIMEOUT <= now)
			{
				info->flags &= ~FLAG_PENDING;
				info->flags |= FLAG_LOST;
				pend_cnt--;
			}
		}

		// Receive available packets
		if (FD_ISSET(ss->sockfd, &fds))
		{
			result = recv_packet(ss);
			if (result == -1)
				return -1;
			if (result > 0)
				pend_cnt--;
		}
	}

	return 0;
}

// End of scan process, print out summary
int scan_end(struct scan_session *ss)
{
	int port;
	int open = 0;
	int closed = 0;
	int lost = 0;

	putchar('\n');

	for (port = ss->port_begin; port <= ss->port_end; port++)
	{
		if (ss->ports[port].flags & FLAG_OPEN)
		{
			printf("%d open\n", port);
			open++;
		}

		if (ss->ports[port].flags & FLAG_CLOSED)
			closed++;
		if (ss->ports[port].flags & FLAG_LOST)
			lost++;
	}

	printf("%d ports open, %d ports closed, %d packets lost\n",
		open, closed, lost);

	return 0;
}

int synscan_main(int argc, char *argv[])
{
	struct config cfg =
	{
		.port_begin = DEF_PORT_BEGIN,
		.port_end   = DEF_PORT_END,
	};
	struct scan_session ss;

	// Get options from command line
	if (get_opts(argc, argv, &cfg) != 0)
		return EXIT_FAILURE;

	if (cfg.help)
	{
 		printf("Usage: synscan [-h] [-n] [-p port(s)]"
			"<source> <destination>\n");
		return EXIT_SUCCESS;
	}

	if (!cfg.src_name)
	{
		fprintf(stderr, "Missing source interface.\n");
		return EXIT_FAILURE;
	}

	if (!cfg.dest_name)
	{
		fprintf(stderr, "Missing destination.\n");
		return EXIT_FAILURE;
	}

	// Initialize scan session and create socket
	if (scan_init(&ss, &cfg) != 0)
		return EXIT_FAILURE;

	// Process scan session
	scan_begin(&ss);
	scan_main(&ss);
	scan_end(&ss);

	return EXIT_SUCCESS;
}

