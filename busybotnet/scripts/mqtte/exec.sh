# vim syntax=bash
# SH POC for mqtte transition - first run:
# mqtte -h localhost -t shell/bot -i 127.0.0.1 -u bot -x x -- /bin/bash ./exec.sh
pubclient="mosquitto_pub"

if which bash >/dev/null 2>&1 ; then 
  export shell='bash'
elif which ash >/dev/null 2>&1; then
  export shell='ash'
elif which dash >/dev/null 2>&1; then
  export shell='dash'
elif which ksh >/dev/null 2>&1; then
  export shell='ksh'
else
  export shell='sh'
fi

export mq_host="localhost"
export mq_pass="x"
export mq_path="/usr/sbin:/bin:/usr/bin:/sbin:/var/bin"
export mq_debug="0"
export mq_intf="lo"
export mq_subtop="shell/bot"
export mq_pubtop="data/jsh"
export mq_binpath="/var/bin"
mq_ip=$(/sbin/ifconfig $mq_intf | grep Mask | cut -d ':' -f2 | cut -d " " -f1)
export mq_ip="$mq_ip"
errorlog=error.log
>$error.log

if [[ -w /tmp ]] ; then workdir=/tmp;elif [[ -w /var/tmp ]] ; then workdir=/var/tmp ; elif [[ -w /var ]] ; then workdir=/var;else workdir=`pwd`;fi
tempf=`mktemp cmd.XXXXXXX`
tempout="$tempf.out"


jsonify(){
jtemp=$(mktemp json.XXXXXX)
jtempf="$workdir/$jtemp"
input="$1"
echo "$2" > $jtempf
bbj="$(jshon -Qs 2>/dev/null 1>/dev/null)"
getip=$bbj"$(echo $mq_ip)"
unixtime=$bbj"$(date +%s)"
getdate=$bbj"$(date)"
getuptime=$bbj"$(uptime | sed s/\,//)"
kernelcmdline=$bbj"$(cat /proc/cmdline)"
getid=$bbj"$(id)"
kcrypto=$bbj"$(cat /proc/crypto | grep name | cut -d':' -f2 | uniq | tr -s '\n' ' '|sed s/\,//)"
getversion=$bbj"$(cat /proc/version)"
memstat=$bbj"$(cat /proc/meminfo | head -n 3 | tr -s '\n' ' ')"
getcwd=$bbj"$(pwd)"
defshell=$bbj"$(echo $SHELL)"
currentshell=$bbj"$(which $shell)"
getstty=$bbj"$(stty 2>/dev/null||echo 'n/a')"
term=$bbj"$(echo $TERM)"
cpuname=$bbj"$(cat /proc/cpuinfo | grep name || echo 'n/a')"
status=$bbj"$(if ([ -s /tmp/.status ] && [ -f /tmp/.status ]); then echo "SYSTEM_BUSY" ; else echo "SYSTEM_READY";fi)"
hashcmd=$bbj"$(md5sum $jtempf |head -c 32)"
uuid=$bbj"'$unixtime.$hashcmd'"
botversion=$bbj"'$(echo $mq_version)'"
output=$bbj"'$($shell $jtempf)'"
cmdline=$bbj"$(echo $(cat $input))"

echo '{
"ip" : "'$getip'",
"unixtime": "'$unixtime'",
"date": "'$getdate'",
"uptime" : "'$getuptime'",
"cpuname": "'$cpuname'",
"memstat" : "'$memstat'",
"id" : "'$getid'",
"version" : "'$getversion'",
"kernel_cmdline": "'$kernelcmdline'",
"kernel_crypto": "'$kcrypto'",
"default shell": "'$defshell'",
"current shell": "'$currentshell'",
"shell level": "'$SHLVL'",
"term": "'$term'",
"stty": "'$getstty'",
"cwd": "'$getcwd'",
"uuid": "'$uuid'",
"status": "'$status'",
"bot version": "'$botversion'",
"cmdline": "'$cmdline'",
"output": "'$output'"
}' 2>>$errorlog
rm -f $jtemp 2>>$errorlog
}

echo "$@"|base64 -d >"$workdir/$tempf"
$shell "$workdir/$tempf" >"$workdir/$tempout" 2>&1 & echo "$!" > "$workdir/$tempf.pid"
wait;[[ "$?" -eq "0" ]] && jsonify "$workdir/$tempf" "cat $workdir/$tempout"|base64|$pubclient -h $mq_host -t data/jsh -s
#base64 "$workdir/$tempout" | mosquitto_pub -h localhost -t data/jsh -s
[[ "$?" -eq "0" ]] &&\
echo ok 
rm -f $tempf $workdir/$tempf $workdir/$tempout $workdir/$tempf.pid
exit
