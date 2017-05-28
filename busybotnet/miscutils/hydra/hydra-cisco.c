#ifdef PALM
#include "palm/hydra-mod.h"
#else
#include "hydra-mod.h"
#endif

extern char *HYDRA_EXIT;
char *hybuf = NULL;

int start_cisco(int s, char *ip, int port, unsigned char options, char *miscptr, FILE * fp) {
  char *empty = "";
  char *pass, hybuffer[300];

  if (strlen(pass = hydra_get_next_password()) == 0)
    pass = empty;

#ifdef PALM
  sprintf(hybuffer, "%s\r\n", pass);
#else
  sprintf(hybuffer, "%.250s\r\n", pass);
#endif

  if (hydra_send(s, hybuffer, strlen(hybuffer), 0) < 0) {
    return 1;
  }
  sleep(1);
  hybuf = NULL;
  do {
    if (hybuf != NULL)
      free(hybuf);
    if ((hybuf = hydra_receive_line(s)) == NULL)
      return 3;
    if (hybuf[strlen(hybuf) - 1] == '\n')
      hybuf[strlen(hybuf) - 1] = 0;
    if (hybuf[strlen(hybuf) - 1] == '\r')
      hybuf[strlen(hybuf) - 1] = 0;
  } while (strlen(hybuf) <= 1);
  if (strstr(hybuf, "assw") != NULL) {
    hydra_completed_pair();
    free(hybuf);
    if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0)
      return 3;
    if (strlen(pass = hydra_get_next_password()) == 0)
      pass = empty;

#ifdef PALM
    sprintf(hybuffer, "%s\r\n", pass);
#else
    sprintf(hybuffer, "%.250s\r\n", pass);
#endif

    if (hydra_send(s, hybuffer, strlen(hybuffer), 0) < 0) {
      return 1;
    }
    
    hybuf = NULL;
    do {
      if (hybuf != NULL)
        free(hybuf);
      if ((hybuf = hydra_receive_line(s)) == NULL)
        return 3;
      if (hybuf[strlen(hybuf) - 1] == '\n')
        hybuf[strlen(hybuf) - 1] = 0;
      if (hybuf[strlen(hybuf) - 1] == '\r')
        hybuf[strlen(hybuf) - 1] = 0;
    } while (strlen(hybuf) <= 1);
    if (hybuf != NULL && strstr(hybuf, "assw") != NULL) {
      hydra_completed_pair();
      free(hybuf);
      hybuf = NULL;
      if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0)
        return 3;
      if (strlen(pass = hydra_get_next_password()) == 0)
        pass = empty;

#ifdef PALM
      sprintf(hybuffer, "%s\r\n", pass);
#else
      sprintf(hybuffer, "%.250s\r\n", pass);
#endif

      if (hydra_send(s, hybuffer, strlen(hybuffer), 0) < 0) {
        return 1;
      }
      hybuf = NULL;
      do {
        if (hybuf != NULL)
          free(hybuf);
        hybuf = hydra_receive_line(s);
        if (hybuf != NULL) {
          if (hybuf[strlen(hybuf) - 1] == '\n')
            hybuf[strlen(hybuf) - 1] = 0;
          if (hybuf[strlen(hybuf) - 1] == '\r')
            hybuf[strlen(hybuf) - 1] = 0;
        }
      } while (hybuf != NULL && strlen(hybuf) <= 1);
    }

  }

  if (hybuf != NULL && (strstr(hybuf, "assw") != NULL || strstr(hybuf, "ad ") != NULL || strstr(hybuf, "attempt") != NULL || strstr(hybuf, "ailur") != NULL)) {
    free(hybuf);
    hydra_completed_pair();
    if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0)
      return 3;
    return 1;
  }

  hydra_report_found_host(port, ip, "cisco", fp);
  hydra_completed_pair_found();
  if (hybuf != NULL)
    free(hybuf);
  if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0)
    return 3;
  return 1;
}

void service_cisco(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port) {
  int run = 1, failc = 0, retry = 1, next_run = 1, sock = -1;
  int myport = PORT_TELNET, mysslport = PORT_TELNET_SSL;

  hydra_register_socket(sp);
  if (memcmp(hydra_get_next_pair(), &HYDRA_EXIT, sizeof(HYDRA_EXIT)) == 0)
    return;
  while (1) {
    next_run = 0;
    switch (run) {
    case 1:                    /* connect and service init function */
      {
        unsigned char *hybuf2;
        int f = 0;

        if (sock >= 0)
          sock = hydra_disconnect(sock);
//        sleepn(275);
        if ((options & OPTION_SSL) == 0) {
          if (port != 0)
            myport = port;
          sock = hydra_connect_tcp(ip, myport);
          port = myport;
          if (miscptr != NULL && hydra_strcasestr(miscptr, "enter") != NULL)
            hydra_send(sock, "\r\n", 2, 0);
        } else {
          if (port != 0)
            mysslport = port;
          sock = hydra_connect_ssl(ip, mysslport);
          port = mysslport;
        }
        if (sock < 0) {
          hydra_report(stderr, "[ERROR] Child with pid %d terminating, can not connect\n", (int) getpid());
          hydra_child_exit(1);
        }
        do {
          if (f != 0)
            free(hybuf2);
          else
            f = 1;
          if ((hybuf2 = (unsigned char *) hydra_receive_line(sock)) == NULL) {
            if (failc < retry) {
              next_run = 1;
              failc++;
              if (quiet != 1) hydra_report(stderr, "[ERROR] Child with pid %d was disconnected - retrying (%d of %d retries)\n", (int) getpid(), failc, retry);
              sleep(3);
              break;
            } else {
              if (quiet != 1) hydra_report(stderr, "[ERROR] Child with pid %d was disconnected - exiting\n", (int) getpid());
              hydra_child_exit(0);
            }
          }
          if (hybuf2 != NULL && hydra_strcasestr((char*)hybuf2, "ress ENTER") != NULL)
            hydra_send(sock, "\r\n", 2, 0);
        } while (strstr((char *) hybuf2, "assw") == NULL);
        free(hybuf2);
        if (next_run != 0)
          break;
        failc = 0;
        next_run = 2;
        break;
      }
    case 2:                    /* run the cracking function */
      next_run = start_cisco(sock, ip, port, options, miscptr, fp);
      break;
    case 3:                    /* clean exit */
      if (sock >= 0)
        sock = hydra_disconnect(sock);
      hydra_child_exit(0);
      return;
    default:
      hydra_report(stderr, "[ERROR] Caught unknown return code, exiting!\n");
      hydra_child_exit(0);
#ifdef PALM
      return;
#else
      hydra_child_exit(2);
#endif
    }
    run = next_run;
  }
}

int service_cisco_init(char *ip, int sp, unsigned char options, char *miscptr, FILE * fp, int port) {
  // called before the childrens are forked off, so this is the function
  // which should be filled if initial connections and service setup has to be
  // performed once only.
  //
  // fill if needed.
  // 
  // return codes:
  //   0 all OK
  //   -1  error, hydra will exit, so print a good error message here

  return 0;
}
