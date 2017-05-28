#!/bin/sh

host=127.0.0.1
key=/tmp/.key
echo $@ |fenc e $key |base64|tr --delete "\n" > /tmp/send.tmp
mosquitto_pub -h $host -t shell/bot -u admin -P pass -f /tmp/send.tmp

exit
