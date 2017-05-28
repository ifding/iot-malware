# BusyBotNet

<pre>
`7MM"""Yp,                              `7MM"""Yp,           mm   `7MN.   `7MF'         mm    
  MM    Yb                                MM    Yb           MM     MMN.    M           MM    
  MM    dP `7MM  `7MM  ,pP"Ybd `7M'   `MF'MM    dP  ,pW"Wq.mmMMmm   M YMb   M  .gP"Ya mmMMmm  
  MM"""bg.   MM    MM  8I   `"   VA   ,V  MM"""bg. 6W'   `Wb MM     M  `MN. M ,M'   Yb  MM    
  MM    `Y   MM    MM  `YMMMa.    VA ,V   MM    `Y 8M     M8 MM     M   `MM.M 8M""""""  MM    
  MM    ,9   MM    MM  L.   I8     VVV    MM    ,9 YA.   ,A9 MM     M     YMM YM.    ,  MM    
.JMMmmmd9    `Mbod"YML.M9mmmP'     ,V   .JMMmmmd9   `Ybmd9'  `Mbmo.JML.    YM  `Mbmmd'  `Mbmo 
                                  ,V                                                          
                               OOb"                                                           
=============================================================================================
  01000010 01110101 01110011 01111001 01000010 01101111 01110100 01001110 01100101 01110100                            

</pre>
## Offensive & Defensive Security for Embedded Systems

### One Bin to Rule Them All...


*IT DOESN'T COMPILE IT SELF.*<br>
*We won't help you DDOS people*<br>
*If you have build errors, GOOGLE them!*<br>
*Do you own research and ask **intelligent** questions.*<br>
*Now with ssh bruteforce support*<br>

Busybotnet is a (deviously named) fork of [busybox](https://busybox.net) that aims to make many of the security tools that
are often only found on full systems available their resource lacking counterparts we call embedded devices. With the 
recent surge in popularity of such devices (aka, the explosion of the 'internet of things'), came many, *many* security 
issues. Part of the problem is that it's difficult to implement cryptography tools on systems with limited resources, 
and the rest is caused by incompetent OEM's that never issue updates or bother to patch any of the gaping security holes 
in their systems. This inevitably leads to the devices being repurpoused by hackers, visa vi botnets... The 
point of this project is to provide all of the security tools a system admin needs to administer embedded devices in one 
static binary, hence the term, "Busybotnet".

### What's New?
-- Our brilliant developer, @kerneldayzero ported in libssh! Busybotnet<br>
    now has ssh and ssh brute force support via hydra!<br>
-- Cowroot - Get root on anything > October 18, 2016 (something like that) This is the  <br>
   poc that actually works. <br>
-- Honeydoor - Fine, we've open sourced it. A backdoor and honeypot in one, with <br>
   shadow parsing, hardcoded passwords, and some other hillarious stuff. Specifically <br>
   tailored toward messing with Mirai. Enjoy. <br>
-- Banscan -- Some banner scanner from packetstorm <br>
-- BusyBotNet now has *MASSCAN* !!!! <br>
-- You can now actually call ./busybotnet and it will work! <br>
-- fenc (encrypt stuff with salsa algo) <br>
-- tsh (needs work, backdoor shell aes enc) <br>
-- rathole (backdoor shell, blowfish enc) <br>
-- ssyn2 (deadly ddos tool) <br>
-- sudp (deadly udp ddos tool) <br>
-- jshon (sh wrapper for json) <br>
-- hydra (yes, buybotnet now has hydra!) <br>
-- prism (userspace icmp triggered reverse shell backdoor) <br>
-- Many other gems, you must figure out the power yourself. <br>

### Currently Defined Functions:
As you can see, we have added many new features to busybox. Particulary interesting are the cryptography applets. This is an 
incomplete list of the applets enabled during my last build:

<pre>
evil@devbox:~/busybotnet$ ./busybotnet hydra
Hydra v8.2-dev (c) 2016 by van Hauser/THC - Please do not use in military or secret service organizations, or for illegal purposes.

Syntax: hydra [[[-l LOGIN|-L FILE] [-p PASS|-P FILE]] | [-C FILE]] [-e nsr] [-o FILE] [-t TASKS] [-M FILE [-T TASKS]] [-w TIME] [-W TIME] [-f] [-s PORT] [-SOuvVd46] [service://server[:PORT][/OPT]]

Options:
  -l LOGIN or -L FILE  login with LOGIN name, or load several logins from FILE
  -p PASS  or -P FILE  try password PASS, or load several passwords from FILE
  -C FILE   colon separated "login:pass" format, instead of -L/-P options
  -M FILE   list of servers to attack, one entry per line, ':' to specify port
  -t TASKS  run TASKS number of connects in parallel (per host, default: 16)
  -U        service module usage details
  -h        more command line options (COMPLETE hyhelp)
  server    the target: DNS, IP or 192.168.0.0/24 (this OR the -M option)
  service   the service to crack (see below for supported protocols)
  OPT       some service modules support additional input (-U for module hyhelp)

Supported services: asterisk cisco cisco-enable cvs ftp http-{head|get} http-{get|post}-form http-proxy http-proxy-urlenum icq imap irc ldap2 ldap3[s] mssql mysql(v4) nntp pcanywhere pcnfs pop3 redis rexec rlogin rsh rtsp s7-300 smb smtp smtp-enum snmp socks5 teamspeak telnet vmauthd vnc xmpp
</pre>

<pre>
evil@devbox:~/busybotnet_masscan/binaries$ ./busybotnet-x64 masscan --echo
rate =     100.00
randomize-hosts = true
seed = 5335182937496124102
shard = 1/1
# ADAPTER SETTINGS
adapter = 
adapter-ip = 0.0.0.0
adapter-mac = 00:00:00:00:00:00
router-mac = 00:00:00:00:00:00
# OUTPUT/REPORTING SETTINGS
output-format = unknown(0)
show = open,,
output-filename = 
rotate = 0
rotate-dir = .
rotate-offset = 0
rotate-filesize = 0
pcap = 
# TARGET SELECTION (IP, PORTS, EXCLUDES)
retries = 0
ports = 

capture = cert
nocapture = html
nocapture = heartbleed

min-packet = 60


evil@devbox:~/busybotnet$ ./busybotnet
BusyBox v1.24.1 (2016-03-15 22:49:48 CDT) multi-call binary.
BusyBox is copyrighted by many authors between 1998-2015.
Licensed under GPLv2. See source distribution for detailed
copyright notices.

Usage: busybox [function [arguments]...]
   or: busybox --list[-full]
   or: busybox --install [-s] [DIR]
   or: function [arguments]...

	BusyBox is a multi-call binary that combines many common Unix
	utilities into a single executable.  Most people will create a
	link to busybox for each function they wish to use and BusyBox
	will act like whatever it was invoked as.

Currently defined functions:
	[, [[, acpid, add-shell, addgroup, adduser, adjtimex, aescrypt, arp,
	arping, ash, awk, bangrab, base64, basename, bd, beep, beer, bindtty,
	blacknurse, blkid, blockdev, boink, bonk, bootchartd, brctl, bunzip2,
	bzcat, bzip2, cal, cat, catv, chat, chattr, chgrp, chmod, chown,
	chpasswd, chpst, chroot, chrt, chvt, cksum, clear, cmp, coke, comm,
	conseal, conspy, cowroot, cp, cpio, crond, crontab, crunch, crypthash,
	cryptpw, cttyhack, cut, date, dc, dcd3c, dd, deallocvt, delgroup,
	deluser, depmod, devmem, df, dhclient, dhcprelay, dhgenprime, diff,
	dirname, dmesg, dnsamp, dnsd, dnsdomainname, dos2unix, dpsc, dpss, du,
	dumpkmap, dumpleases, ecdsa, echo, ed, egrep, eject, env, envdir,
	envuidgid, ether-wake, expand, expr, fakeidentd, false, fatattr, fbset,
	fbsplash, fdflush, fdformat, fdisk, fenc, fgconsole, fgrep, find,
	findfs, flock, fold, free, freeramdisk, fsck, fsck.minix, fstrim,
	fsync, ftpd, ftpget, ftpput, fuser, genericsum, genkey, getopt, getty,
	gewse, gewse5, grep, groups, gunzip, gzip, halt, hd, hdparm, head,
	hexdump, hole, hostid, hostname, httpd, hush, hwclock, *hydra*,
	i2cdetect, i2cdump, i2cget, i2cset, id, ifconfig, ifdown, ifenslave,
	ifplugd, ifup, inetd, init, insmod, install, ionice, iostat, ip,
	ipaddr, ipcalc, ipcrm, ipcs, iplink, iproute, iprule, iptunnel, jolt,
	jshon, kbd_mode, kill, killall, killall5, kissofdeath, kkill, klogd,
	knbot, land, last, latierra, less, linux32, linux64, linuxrc, lizbot,
	lizserv, ln, loadfont, loadkmap, logger, login, logname, logread,
	losetup, lpd, lpq, lpr, ls, lsattr, lsmod, lsof, lspci, lsusb, lzcat,
	lzma, lzop, lzopcat, makedevs, makemime, man, *masscan*, md5sum, mdev,
	mesg, microcom, mkdir, mkdosfs, mke2fs, mkfifo, mkfs.ext2, mkfs.minix,
	mkfs.vfat, mknod, mkpasswd, mkswap, mktemp, modinfo, modprobe, more,
	mount, mountpoint, mpstat, mq, mqsh, mqtte, mt, mv, nameif, nanddump,
	nandwrite, nbd-client, nc, nestea, netscan, netstat, newtear, nice,
	nmeter, nohup, nslookup, ntpd, ntpdos, od, openvt, orgasm, ottf,
	passwd, patator, patch, pgrep, pidof, ping, ping6, pipe_progress,
	pivot_root, pkdecrypt, pkencrypt, pkill, pksign, pmap, pong,
	popmaildir, poweroff, powertop, printenv, printf, prism, proxcat, ps,
	pscan, pstree, pubclient, pud, pwd, pwdx, raidautorun, randip, raped,
	rdate, rdev, readahead, readlink, readprofile, realpath, reboot,
	reformime, remove-shell, renice, reset, resize, rev, rm, rmdir, rmmod,
	route, rpm, rpm2cpio, rsadecrypt, rsaencrypt, rsagenkey, rsasign,
	rsaverify, rtcwake, run-parts, runlevel, runsv, runsvdir, rx, scp,
	script, scriptreplay, sed, sendmail, seq, setarch, setconsole, setfont,
	setkeycodes, setlogcons, setserial, setsid, setuidgid, sftp, sh,
	sha1sum, sha256sum, sha3sum, sha512sum, showkey, shuf, slattach, sleep,
	smemcap, snmpdos, sockstress, softlimit, sort, spiffit, sping, split,
	*ssh*, ssyn2, start-stop-daemon, stat, stream, strings, stty, su,
	subclient, sudp, sulogin, sum, sv, svlogd, swapoff, swapon,
	switch_root, sync, synk4, synscan, sysctl, syslogd, tac, tail, tar,
	tcpsvd, teardrop, tee, telnet, telnetd, test, tftp, tftpd, time,
	timeout, top, torloris, touch, tr, traceroute, traceroute6, true,
	truncate, tty, ttysize, tunctl, ubiattach, ubidetach, ubimkvol,
	ubirmvol, ubirsvol, ubiupdatevol, udhcpc, udhcpd, udpdata, udpspoof,
	udpsvd, uevent, umount, uname, unexpand, uniq, unix2dos, unlink,
	unlzma, unlzop, unxz, unzip, uptime, users, usleep, uudecode, uuencode,
	vconfig, vi, vlock, volname, wall, watch, watchdog, wc, wget, which,
	who, whoami, whois, wingatecrash, xargs, xersex, xersextcp, xorpipe,
	xxd, xz, xzcat, yes, zcat, zcip

</pre>

### Demo

<pre>
evil@devbox:~/busybotnet$ ./busybotnet rsaencrypt -h

  . Seeding the random number generator...
  . Reading public key from rsa_pub.txt
  . Generating the RSA encrypted value
  . Done (created "result-enc.txt")

evil@devbox:~/busybotnet$ ./busybotnet ecdsa

  . Seeding the random number generator... ok
  . Generating key pair... ok (key size: 192 bits)
  + Public key: 042B22958EAEABB744D2B0C7F3BA71133400D498725FFB86B2B4C3EDE4EB188741DBC1777779C3B20914F7E96AB4FB359E
  . Signing message... ok (signature length = 55)
  + Hash: 546869732073686F756C64206265207468652068617368206F662061206D6573736167652E00
  + Signature: 30350218788C84CAE1B3A4D4E297FDC517889D1C102B899E202A6C09021900CA152006C9526719C901203AA037E8CD5FC29E1D2A9CEDAF
  . Preparing verification context... ok
  . Verifying signature... ok
evil@devbox:~/busybotnet$ ./busybox ecdsa -h
usage: ecdsa
evil@devbox:~/busybotnet$ ./busybox ecdsa --help
BusyBox v1.24.1 (2016-03-15 22:49:48 CDT) multi-call binary.

Usage: ecdsa NoneNone
evil@devbox:~/busybotnet$ ./busybotnet crypthash -h

  crypt_and_hash <mode> <input filename> <output filename> <cipher> <mbedtls_md> <key>

   <mode>: 0 = encrypt, 1 = decrypt

  example: crypt_and_hash 0 file file.aes AES-128-CBC SHA1 hex:E76B2413958B00E193

Available ciphers:
  AES-128-ECB
  AES-192-ECB
  AES-256-ECB
  AES-128-CBC
  AES-192-CBC
  AES-256-CBC
  AES-128-CFB128
  AES-192-CFB128
  AES-256-CFB128
  AES-128-CTR
  AES-192-CTR
  AES-256-CTR
  AES-128-GCM
  AES-192-GCM
  AES-256-GCM
  AES-128-CCM
  AES-192-CCM
  AES-256-CCM
  ARC4-128
  BLOWFISH-ECB
  BLOWFISH-CBC
  BLOWFISH-CFB64
  BLOWFISH-CTR
  CAMELLIA-128-ECB
  CAMELLIA-192-ECB
  CAMELLIA-256-ECB
  CAMELLIA-128-CBC
  CAMELLIA-192-CBC
  CAMELLIA-256-CBC
  CAMELLIA-128-CFB128
  CAMELLIA-192-CFB128
  CAMELLIA-256-CFB128
  CAMELLIA-128-CTR
  CAMELLIA-192-CTR
  CAMELLIA-256-CTR
  CAMELLIA-128-GCM
  CAMELLIA-192-GCM
  CAMELLIA-256-GCM
  CAMELLIA-128-CCM
  CAMELLIA-192-CCM
  CAMELLIA-256-CCM
  DES-ECB
  DES-EDE-ECB
  DES-EDE3-ECB
  DES-CBC
  DES-EDE-CBC
  DES-EDE3-CBC

Available message digests:
  SHA512
  SHA384
  SHA256
  SHA224
  SHA1
  RIPEMD160
  MD5

evil@devbox:~/busybotnet$ ./busybotnet aescrypt -h

  aescrypt2 <mode> <input filename> <output filename> <key>

   <mode>: 0 = encrypt, 1 = decrypt

  example: aescrypt2 0 file file.aes hex:E76B2413958B00E193

Usage: xersex NoneNone
evil@devbox:~/busybotnet$ ./busybotnet xersex fags.com 80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
[Connecting -> fags.com:80
^C
evil@devbox:~/busybotnet$ ./busybox wget
BusyBox v1.24.1 (2016-03-15 22:49:48 CDT) multi-call binary.

Usage: wget [-c|--continue] [-s|--spider] [-q|--quiet] [-O|--output-document FILE]
	[--header 'header: value'] [-Y|--proxy on/off] [-P DIR]
	[-U|--user-agent AGENT] [-T SEC] URL...

Retrieve files via HTTP or FTP

	-s	Spider mode - only check file existence
	-c	Continue retrieval of aborted transfer
	-q	Quiet
	-P DIR	Save to DIR (default .)
	-T SEC	Network read timeout is SEC seconds
	-O FILE	Save to FILE ('-' for stdout)
	-U STR	Use STR for User-Agent header
	-Y	Use proxy ('on' or 'off')
evil@devbox:~/busybotnet$ ./busybox wget https://google.com
Connecting to google.com (216.58.216.238:443)
Connecting to www.google.de (216.58.216.227:443)
index.html           100% |****************************************************| 19570   0:00:00 ETA

evil@devbox:~/busybotnet$ ./busybotnet proxcat 
connect --- simple relaying command via proxy.
Version 1.97
usage: proxcat [-dnhst45N] [-p local-port][-R resolve] [-w timeout] 
          [-H proxy-server[:port]] [-S [user@]socks-server[:port]] 
          [-T proxy-server[:port]]
          [-c telnet-proxy-command]
          host port
          
evil@devbox:~/busybotnet$ ./busybotnet netscan -h
[*] Network Scanner v1.0 starting at 22:51:11 Mar 15 2016 [*]
  -c | --connect	Tcp protocol
  -s | --syn		Syn packet scanner
  -t | --tor		Tor scanner default 127.0.0.1:9050
  -u | --udp		Udp protocol
  -b | --banner		Parse service banner
  -p | --port		Port method A, A-B, A,B,C,D
  -d | --delay		Delay synpack in ms [min: 50000]
  -v | --verbose	Verbose output
  -h | --help		Print help menu

  Example: scan -s google.it
           scan -c google.it
           scan -t google.it
           scan -c -b google.it
           scan -c -p1-100 google.it
           scan -c -p1,2,3,4 google.it

</pre>
<br>

### Compiling & Installation
Building is litterally 3 commands <br>
**Step 1:** <br>
$ make clean <br>
**Step 2:** <br>
$ make menuconfig 
  -- Configure your build <br>
  -- Choose applets to include
  -- If we're cross compiling see below... <br>
**Step 3:** <br>
$ make <br>

To install, run ./busybox --install -s /path/to/wherever

#### Cross Compiling
Cross-compiling busybo* is easy. First, you need a toolchain. <br>
**Step 1:** <br>
Grab the latest [buildroot](https://download.buildroot.org) and build it (same as above, $ make clean;make menuconfig;make) <br>
**Step 2** <br>
Configure with *make menuconfig* -- Specifically, tell busybotnet where your *toolchain and sysroot* are located. <br>
**Step 3** <br>
$ make <br>

If you want your resulting binary to be conspiciously called "busybotnet" than rename it like so: <br>
$ mv busybox busybotnet <br>
The libbb.h library has been changed to allow prefixes of busybo\* instead of busybox\* , way cooler, in my opinion. 

That's it!

### Want to help?

Great! We've even included a shell script (add.sh) that simplifies the process of adding applets to busybotnet (or just plain busybox). If you want to improve busybotnet, fork our code and submit a pull request.


### License 
BusyBotNet is licensed under the GPL. You should have received a copy of the GPL with the source code. You are permitted to use, modify, copy and redistribute so long as you keep the source open & available and credit the authors.

#### Credits
Authors: Kod & Shellz <br>
Conceptualized by [Shellz](https://github.com/isdrupter). <br>
Brought to life by [Kod](https://github.com/kernelzeroday) <br>
Busybox GPL source code forked from [busybox.net](https://busybox.net) <br>
Authors of any applets included are in the source. I will add them here when I get around to it. <br>
If you add an applet, please do credit the original author (even if it's you). <br>
If you one your programs ended up in busybotnet and we have not credited you for it, please do comment <br>
and I will fix that!

#### Contact
Don't be shy! Feel free to write us at: ---- never mind, our email was suspended. You can message us here on github.
