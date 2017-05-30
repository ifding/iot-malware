#ifndef __HEADERS_H_
#define __HEADERS_H_

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdarg.h>
#include <wait.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/ip.h>
#include <linux/tcp.h>

#include "main.h"
#include "config.h"
#include "utils.h"
#include "irc.h"
#include "requests.h"
#include "scan.h"
#include "attacks.h"

#undef EXIT_FAILURE
#define EXIT_FAILURE -1

#endif
