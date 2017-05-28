#!/bin/sh
# ./socatchk remote-host remote-port
# crudely shutsdown socat (if running) and then restarts it for new host/port
orport=9050
orlisadr=127.0.0.1

case $1 in
-K|--kill)
[ "$(pidof socat >/dev/null 2>&1 && echo $?)" = 0 ] && kill $(pidof socat); [ "$(pidof socat && echo $?)" != 0 ]
;;
-c|--connect)
socat TCP4-LISTEN:$2,fork SOCKS4A:$orlisadr:$3:$4,socksport=$orport &
;;
-h|--help)
echo "Usage: $0 -c <local port> <onion addr> <port>"
;;
*)
echo "Invalid. -h or --help for Usage"
;;
esac
exit
