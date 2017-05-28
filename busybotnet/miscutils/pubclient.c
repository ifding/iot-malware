/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/


#include <signal.h>

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include "../include/mosquitto.h"
#include "../include/client_shared.h"

#define STATUS_CONNECTING 0
#define STATUS_CONNACK_RECVD 1
#define STATUS_WAITING 2

/* Global variables for use in callbacks. See sub_client.c for an example of
 * using a struct to hold variables for use in callbacks. */
static char *topic = NULL;
static char *message = NULL;
static long msglen = 0;
static int qos = 0;
static int retain = 0;
static int mode = MSGMODE_NONE;
static int status = STATUS_CONNECTING;
static int mid_sent = 0;
static int last_mid = -1;
static int last_mid_sent = -1;
static bool connected = true;
static char *username = NULL;
static char *password = NULL;
static bool disconnect_sent = false;
static bool quiet = false;

void pub_my_connect_callback(struct mosquitto *mosq, void *obj, int result)
{
	int rc = MOSQ_ERR_SUCCESS;

	if(!result){
		switch(mode){
			case MSGMODE_CMD:
			case MSGMODE_FILE:
			case MSGMODE_STDIN_FILE:
				rc = mosquitto_publish(mosq, &mid_sent, topic, msglen, message, qos, retain);
				break;
			case MSGMODE_NULL:
				rc = mosquitto_publish(mosq, &mid_sent, topic, 0, NULL, qos, retain);
				break;
			case MSGMODE_STDIN_LINE:
				status = STATUS_CONNACK_RECVD;
				break;
		}
		if(rc){
			if(!quiet){
				switch(rc){
					case MOSQ_ERR_INVAL:
						fprintf(stderr, "Error: Invalid input. Does your topic contain '+' or '#'?\n");
						break;
					case MOSQ_ERR_NOMEM:
						fprintf(stderr, "Error: Out of memory when trying to publish message.\n");
						break;
					case MOSQ_ERR_NO_CONN:
						fprintf(stderr, "Error: Client not connected when trying to publish.\n");
						break;
					case MOSQ_ERR_PROTOCOL:
						fprintf(stderr, "Error: Protocol error when communicating with broker.\n");
						break;
					case MOSQ_ERR_PAYLOAD_SIZE:
						fprintf(stderr, "Error: Message payload is too large.\n");
						break;
				}
			}
			mosquitto_disconnect(mosq);
		}
	}else{
		if(result && !quiet){
			fprintf(stderr, "%s\n", mosquitto_connack_string(result));
		}
	}
}

void pub_my_disconnect_callback(struct mosquitto *mosq, void *obj, int rc)
{
	connected = false;
}

void pub_my_publish_callback(struct mosquitto *mosq, void *obj, int mid)
{
	last_mid_sent = mid;
	if(mode == MSGMODE_STDIN_LINE){
		if(mid == last_mid){
			mosquitto_disconnect(mosq);
			disconnect_sent = true;
		}
	}else if(disconnect_sent == false){
		mosquitto_disconnect(mosq);
		disconnect_sent = true;
	}
}

void pub_my_log_callback(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}

int load_stdin(void)
{
	long pos = 0, rlen;
	char buf[1024];
	char *aux_message = NULL;

	mode = MSGMODE_STDIN_FILE;

	while(!feof(stdin)){
		rlen = fread(buf, 1, 1024, stdin);
		aux_message = realloc(message, pos+rlen);
		if(!aux_message){
			if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
			free(message);
			return 1;
		} else
		{
			message = aux_message;
		}
		memcpy(&(message[pos]), buf, rlen);
		pos += rlen;
	}
	msglen = pos;

	if(!msglen){
		if(!quiet) fprintf(stderr, "Error: Zero length input.\n");
		return 1;
	}

	return 0;
}

int load_file(const char *filename)
{
	long pos, rlen;
	FILE *fptr = NULL;

	fptr = fopen(filename, "rb");
	if(!fptr){
		if(!quiet) fprintf(stderr, "Error: Unable to open file \"%s\".\n", filename);
		return 1;
	}
	mode = MSGMODE_FILE;
	fseek(fptr, 0, SEEK_END);
	msglen = ftell(fptr);
	if(msglen > 268435455){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is too large (>268,435,455 bytes).\n", filename);
		return 1;
	}else if(msglen == 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: File \"%s\" is empty.\n", filename);
		return 1;
	}else if(msglen < 0){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Unable to determine size of file \"%s\".\n", filename);
		return 1;
	}
	fseek(fptr, 0, SEEK_SET);
	message = malloc(msglen);
	if(!message){
		fclose(fptr);
		if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}
	pos = 0;
	while(pos < msglen){
		rlen = fread(&(message[pos]), sizeof(char), msglen-pos, fptr);
		pos += rlen;
	}
	fclose(fptr);
	return 0;
}

void pub_print_usage(void)
{
	int major, minor, revision, VERSION;

	mosquitto_lib_version(&major, &minor, &revision);
	printf("mosquitto_pub is a simple mqtt client that will publish a message on a single topic and exit.\n");
	printf("mosquitto_pub version  running on libmosquitto %d,%d.%d.\n\n", major, minor, revision);
	printf("Usage: mosquitto_pub [-h host] [-k keepalive] [-p port] [-q qos] [-r] {-f file | -l | -n | -m message} -t topic\n");
#ifdef WITH_SRV
	printf("                     [-A bind_address] [-S]\n");
#else
	printf("                     [-A bind_address]\n");
#endif
	printf("                     [-i id] [-I id_prefix]\n");
	printf("                     [-d] [--quiet]\n");
	printf("                     [-M max_inflight]\n");
	printf("                     [-u username [-P password]]\n");
	printf("                     [--will-topic [--will-payload payload] [--will-qos qos] [--will-retain]]\n");
#ifdef WITH_TLS
	printf("                     [{--cafile file | --capath dir} [--cert file] [--key file]\n");
	printf("                      [--ciphers ciphers] [--insecure]]\n");
#ifdef WITH_TLS_PSK
	printf("                     [--psk hex-key --psk-identity identity [--ciphers ciphers]]\n");
#endif
#endif
#ifdef WITH_SOCKS
	printf("                     [--proxy socks-url]\n");
#endif
	printf("       mosquitto_pub --help\n\n");
	printf(" -A : bind the outgoing socket to this host/ip address. Use to control which interface\n");
	printf("      the client communicates over.\n");
	printf(" -d : enable debug messages.\n");
	printf(" -f : send the contents of a file as the message.\n");
	printf(" -h : mqtt host to connect to. Defaults to localhost.\n");
	printf(" -i : id to use for this client. Defaults to mosquitto_pub_ appended with the process id.\n");
	printf(" -I : define the client id as id_prefix appended with the process id. Useful for when the\n");
	printf("      broker is using the clientid_prefixes option.\n");
	printf(" -k : keep alive in seconds for this client. Defaults to 60.\n");
	printf(" -l : read messages from stdin, sending a separate message for each line.\n");
	printf(" -m : message payload to send.\n");
	printf(" -M : the maximum inflight messages for QoS 1/2..\n");
	printf(" -n : send a null (zero length) message.\n");
	printf(" -p : network port to connect to. Defaults to 1883.\n");
	printf(" -P : provide a password (requires MQTT 3.1 broker)\n");
	printf(" -q : quality of service level to use for all messages. Defaults to 0.\n");
	printf(" -r : message should be retained.\n");
	printf(" -s : read message from stdin, sending the entire input as a message.\n");
#ifdef WITH_SRV
	printf(" -S : use SRV lookups to determine which host to connect to.\n");
#endif
	printf(" -t : mqtt topic to publish to.\n");
	printf(" -u : provide a username (requires MQTT 3.1 broker)\n");
	printf(" -V : specify the version of the MQTT protocol to use when connecting.\n");
	printf("      Can be mqttv31 or mqttv311. Defaults to mqttv31.\n");
	printf(" --help : display this message.\n");
	printf(" --quiet : don't print error messages.\n");
	printf(" --will-payload : payload for the client Will, which is sent by the broker in case of\n");
	printf("                  unexpected disconnection. If not given and will-topic is set, a zero\n");
	printf("                  length message will be sent.\n");
	printf(" --will-qos : QoS level for the client Will.\n");
	printf(" --will-retain : if given, make the client Will retained.\n");
	printf(" --will-topic : the topic on which to publish the client Will.\n");
#ifdef WITH_TLS
	printf(" --cafile : path to a file containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --capath : path to a directory containing trusted CA certificates to enable encrypted\n");
	printf("            communication.\n");
	printf(" --cert : client certificate for authentication, if required by server.\n");
	printf(" --key : client private key for authentication, if required by server.\n");
	printf(" --ciphers : openssl compatible list of TLS ciphers to support.\n");
	printf(" --tls-version : TLS protocol version, can be one of tlsv1.2 tlsv1.1 or tlsv1.\n");
	printf("                 Defaults to tlsv1.2 if available.\n");
	printf(" --insecure : do not check that the server certificate hostname matches the remote\n");
	printf("              hostname. Using this option means that you cannot be sure that the\n");
	printf("              remote host is the server you wish to connect to and so is insecure.\n");
	printf("              Do not use this option in a production environment.\n");
#  ifdef WITH_TLS_PSK
	printf(" --psk : pre-shared-key in hexadecimal (no leading 0x) to enable TLS-PSK mode.\n");
	printf(" --psk-identity : client identity string for TLS-PSK mode.\n");
#  endif
#endif
#ifdef WITH_SOCKS
	printf(" --proxy : SOCKS5 proxy URL of the form:\n");
	printf("           socks5h://[username[:password]@]hostname[:port]\n");
	printf("           Only \"none\" and \"username\" authentication is supported.\n");
#endif
	printf("\nSee http://mosquitto.org/ for more information.\n\n");
}

int pubclient_main(int argc, char *argv[])
{
	struct mosq_config cfg;
	struct mosquitto *mosq = NULL;
	int rc;
	int rc2;
	char *buf;
	int buf_len = 1024;
	int buf_len_actual;
	int read_len;
	int pos;

	buf = malloc(buf_len);
	if(!buf){
		fprintf(stderr, "Error: Out of memory.\n");
		return 1;
	}

	memset(&cfg, 0, sizeof(struct mosq_config));
	rc = client_config_load(&cfg, CLIENT_PUB, argc, argv);
	if(rc){
		client_config_cleanup(&cfg);
		if(rc == 2){
			/* --help */
			pub_print_usage();
		}else{
			fprintf(stderr, "\nUse 'mosquitto_pub --help' to see usage.\n");
		}
		return 1;
	}

	topic = cfg.topic;
	message = cfg.message;
	msglen = cfg.msglen;
	qos = cfg.qos;
	retain = cfg.retain;
	mode = cfg.pub_mode;
	username = cfg.username;
	password = cfg.password;
	quiet = cfg.quiet;

	if(cfg.pub_mode == MSGMODE_STDIN_FILE){
		if(load_stdin()){
			fprintf(stderr, "Error loading input from stdin.\n");
			return 1;
		}
	}else if(cfg.file_input){
		if(load_file(cfg.file_input)){
			fprintf(stderr, "Error loading input file \"%s\".\n", cfg.file_input);
			return 1;
		}
	}

	if(!topic || mode == MSGMODE_NONE){
		fprintf(stderr, "Error: Both topic and message must be supplied.\n");
		pub_print_usage();
		return 1;
	}


	mosquitto_lib_init();

	if(client_id_generate(&cfg, "mosqpub")){
		return 1;
	}

	mosq = mosquitto_new(cfg.id, true, NULL);
	if(!mosq){
		switch(errno){
			case ENOMEM:
				if(!quiet) fprintf(stderr, "Error: Out of memory.\n");
				break;
			case EINVAL:
				if(!quiet) fprintf(stderr, "Error: Invalid id.\n");
				break;
		}
		mosquitto_lib_cleanup();
		return 1;
	}
	if(cfg.debug){
		mosquitto_log_callback_set(mosq, pub_my_log_callback);
	}
	mosquitto_connect_callback_set(mosq, pub_my_connect_callback);
	mosquitto_disconnect_callback_set(mosq, pub_my_disconnect_callback);
	mosquitto_publish_callback_set(mosq, pub_my_publish_callback);

	if(client_opts_set(mosq, &cfg)){
		return 1;
	}
	rc = client_connect(mosq, &cfg);
	if(rc) return rc;

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_start(mosq);
	}

	do{
		if(mode == MSGMODE_STDIN_LINE){
			if(status == STATUS_CONNACK_RECVD){
				pos = 0;
				read_len = buf_len;
				while(fgets(&buf[pos], read_len, stdin)){
					buf_len_actual = strlen(buf);
					if(buf[buf_len_actual-1] == '\n'){
						buf[buf_len_actual-1] = '\0';
						rc2 = mosquitto_publish(mosq, &mid_sent, topic, buf_len_actual-1, buf, qos, retain);
						if(rc2){
							if(!quiet) fprintf(stderr, "Error: Publish returned %d, disconnecting.\n", rc2);
							mosquitto_disconnect(mosq);
						}
						break;
					}else{
						buf_len += 1024;
						pos += 1023;
						read_len = 1024;
						buf = realloc(buf, buf_len);
						if(!buf){
							fprintf(stderr, "Error: Out of memory.\n");
							return 1;
						}
					}
				}
				if(feof(stdin)){
					last_mid = mid_sent;
					status = STATUS_WAITING;
				}
			}else if(status == STATUS_WAITING){
				if(last_mid_sent == last_mid && disconnect_sent == false){
					mosquitto_disconnect(mosq);
					disconnect_sent = true;
				}
#ifdef WIN32
				Sleep(100);
#else
				usleep(100000);
#endif
			}
			rc = MOSQ_ERR_SUCCESS;
		}else{
			rc = mosquitto_loop(mosq, -1, 1);
		}
	}while(rc == MOSQ_ERR_SUCCESS && connected);

	if(mode == MSGMODE_STDIN_LINE){
		mosquitto_loop_stop(mosq, false);
	}

	if(message && mode == MSGMODE_FILE){
		free(message);
	}
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();

	if(rc){
		fprintf(stderr, "Error: %s\n", mosquitto_strerror(rc));
	}
	return rc;
}







// oh shit

#include "../include/mosquitto.h"
#include "../include/mosquitto_internal.h"
#include "../include/logging_mosq.h"
#include "../include/messages_mosq.h"
#include "../include/memory_mosq.h"
#include "../include/mqtt3_protocol.h"
#include "../include/net_mosq.h"
#include "../include/read_handle.h"
#include "../include/send_mosq.h"
#include "../include/socks_mosq.h"
#include "../include/time_mosq.h"
#include "../include/tls_mosq.h"
#include "../include/util_mosq.h"
#include "../include/will_mosq.h"


void _mosquitto_destroy(struct mosquitto *mosq);
static int _mosquitto_reconnect(struct mosquitto *mosq, bool blocking);
static int _mosquitto_connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address);

int mosquitto_lib_version(int *major, int *minor, int *revision)
{
	if(major) *major = LIBMOSQUITTO_MAJOR;
	if(minor) *minor = LIBMOSQUITTO_MINOR;
	if(revision) *revision = LIBMOSQUITTO_REVISION;
	return LIBMOSQUITTO_VERSION_NUMBER;
}

int mosquitto_lib_init(void)
{
#ifdef WIN32
	srand(GetTickCount());
#else
	struct timeval tv;

	gettimeofday(&tv, NULL);
	srand(tv.tv_sec*1000 + tv.tv_usec/1000);
#endif

	_mosquitto_net_init();

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_lib_cleanup(void)
{
	_mosquitto_net_cleanup();

	return MOSQ_ERR_SUCCESS;
}

struct mosquitto *mosquitto_new(const char *id, bool clean_session, void *userdata)
{
	struct mosquitto *mosq = NULL;
	int rc;

	if(clean_session == false && id == NULL){
		errno = EINVAL;
		return NULL;
	}

#ifndef WIN32
	signal(SIGPIPE, SIG_IGN);
#endif

	mosq = (struct mosquitto *)_mosquitto_calloc(1, sizeof(struct mosquitto));
	if(mosq){
		mosq->sock = INVALID_SOCKET;
		mosq->sockpairR = INVALID_SOCKET;
		mosq->sockpairW = INVALID_SOCKET;
#ifdef WITH_THREADING
		mosq->thread_id = pthread_self();
#endif
		rc = mosquitto_reinitialise(mosq, id, clean_session, userdata);
		if(rc){
			mosquitto_destroy(mosq);
			if(rc == MOSQ_ERR_INVAL){
				errno = EINVAL;
			}else if(rc == MOSQ_ERR_NOMEM){
				errno = ENOMEM;
			}
			return NULL;
		}
	}else{
		errno = ENOMEM;
	}
	return mosq;
}

int mosquitto_reinitialise(struct mosquitto *mosq, const char *id, bool clean_session, void *userdata)
{
	int i;

	if(!mosq) return MOSQ_ERR_INVAL;

	if(clean_session == false && id == NULL){
		return MOSQ_ERR_INVAL;
	}

	_mosquitto_destroy(mosq);
	memset(mosq, 0, sizeof(struct mosquitto));

	if(userdata){
		mosq->userdata = userdata;
	}else{
		mosq->userdata = mosq;
	}
	mosq->protocol = mosq_p_mqtt31;
	mosq->sock = INVALID_SOCKET;
	mosq->sockpairR = INVALID_SOCKET;
	mosq->sockpairW = INVALID_SOCKET;
	mosq->keepalive = 60;
	mosq->message_retry = 20;
	mosq->last_retry_check = 0;
	mosq->clean_session = clean_session;
	if(id){
		if(STREMPTY(id)){
			return MOSQ_ERR_INVAL;
		}
		mosq->id = _mosquitto_strdup(id);
	}else{
		mosq->id = (char *)_mosquitto_calloc(24, sizeof(char));
		if(!mosq->id){
			return MOSQ_ERR_NOMEM;
		}
		mosq->id[0] = 'm';
		mosq->id[1] = 'o';
		mosq->id[2] = 's';
		mosq->id[3] = 'q';
		mosq->id[4] = '/';

		for(i=5; i<23; i++){
			mosq->id[i] = (rand()%73)+48;
		}
	}
	mosq->in_packet.payload = NULL;
	_mosquitto_packet_cleanup(&mosq->in_packet);
	mosq->out_packet = NULL;
	mosq->current_out_packet = NULL;
	mosq->last_msg_in = mosquitto_time();
	mosq->last_msg_out = mosquitto_time();
	mosq->ping_t = 0;
	mosq->last_mid = 0;
	mosq->state = mosq_cs_new;
	mosq->in_messages = NULL;
	mosq->in_messages_last = NULL;
	mosq->out_messages = NULL;
	mosq->out_messages_last = NULL;
	mosq->max_inflight_messages = 20;
	mosq->will = NULL;
	mosq->on_connect = NULL;
	mosq->on_publish = NULL;
	mosq->on_message = NULL;
	mosq->on_subscribe = NULL;
	mosq->on_unsubscribe = NULL;
	mosq->host = NULL;
	mosq->port = 1883;
	mosq->in_callback = false;
	mosq->in_queue_len = 0;
	mosq->out_queue_len = 0;
	mosq->reconnect_delay = 1;
	mosq->reconnect_delay_max = 1;
	mosq->reconnect_exponential_backoff = false;
	mosq->threaded = false;
#ifdef WITH_TLS
	mosq->ssl = NULL;
	mosq->tls_cert_reqs = SSL_VERIFY_PEER;
	mosq->tls_insecure = false;
	mosq->want_write = false;
#endif
#ifdef WITH_THREADING
	pthread_mutex_init(&mosq->callback_mutex, NULL);
	pthread_mutex_init(&mosq->log_callback_mutex, NULL);
	pthread_mutex_init(&mosq->state_mutex, NULL);
	pthread_mutex_init(&mosq->out_packet_mutex, NULL);
	pthread_mutex_init(&mosq->current_out_packet_mutex, NULL);
	pthread_mutex_init(&mosq->msgtime_mutex, NULL);
	pthread_mutex_init(&mosq->in_message_mutex, NULL);
	pthread_mutex_init(&mosq->out_message_mutex, NULL);
	pthread_mutex_init(&mosq->mid_mutex, NULL);
	mosq->thread_id = pthread_self();
#endif

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	return _mosquitto_will_set(mosq, topic, payloadlen, payload, qos, retain);
}

int mosquitto_will_clear(struct mosquitto *mosq)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	return _mosquitto_will_clear(mosq);
}

int mosquitto_username_pw_set(struct mosquitto *mosq, const char *username, const char *password)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	if(mosq->username){
		_mosquitto_free(mosq->username);
		mosq->username = NULL;
	}
	if(mosq->password){
		_mosquitto_free(mosq->password);
		mosq->password = NULL;
	}

	if(username){
		mosq->username = _mosquitto_strdup(username);
		if(!mosq->username) return MOSQ_ERR_NOMEM;
		if(password){
			mosq->password = _mosquitto_strdup(password);
			if(!mosq->password){
				_mosquitto_free(mosq->username);
				mosq->username = NULL;
				return MOSQ_ERR_NOMEM;
			}
		}
	}
	return MOSQ_ERR_SUCCESS;
}

int mosquitto_reconnect_delay_set(struct mosquitto *mosq, unsigned int reconnect_delay, unsigned int reconnect_delay_max, bool reconnect_exponential_backoff)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	
	mosq->reconnect_delay = reconnect_delay;
	mosq->reconnect_delay_max = reconnect_delay_max;
	mosq->reconnect_exponential_backoff = reconnect_exponential_backoff;
	
	return MOSQ_ERR_SUCCESS;
	
}

void _mosquitto_destroy(struct mosquitto *mosq)
{
	struct _mosquitto_packet *packet;
	if(!mosq) return;

#ifdef WITH_THREADING
	if(mosq->threaded && !pthread_equal(mosq->thread_id, pthread_self())){
		pthread_cancel(mosq->thread_id);
		pthread_join(mosq->thread_id, NULL);
		mosq->threaded = false;
	}

	if(mosq->id){
		/* If mosq->id is not NULL then the client has already been initialised
		 * and so the mutexes need destroying. If mosq->id is NULL, the mutexes
		 * haven't been initialised. */
		pthread_mutex_destroy(&mosq->callback_mutex);
		pthread_mutex_destroy(&mosq->log_callback_mutex);
		pthread_mutex_destroy(&mosq->state_mutex);
		pthread_mutex_destroy(&mosq->out_packet_mutex);
		pthread_mutex_destroy(&mosq->current_out_packet_mutex);
		pthread_mutex_destroy(&mosq->msgtime_mutex);
		pthread_mutex_destroy(&mosq->in_message_mutex);
		pthread_mutex_destroy(&mosq->out_message_mutex);
		pthread_mutex_destroy(&mosq->mid_mutex);
	}
#endif
	if(mosq->sock != INVALID_SOCKET){
		_mosquitto_socket_close(mosq);
	}
	_mosquitto_message_cleanup_all(mosq);
	_mosquitto_will_clear(mosq);
#ifdef WITH_TLS
	if(mosq->ssl){
		SSL_free(mosq->ssl);
	}
	if(mosq->ssl_ctx){
		SSL_CTX_free(mosq->ssl_ctx);
	}
	if(mosq->tls_cafile) _mosquitto_free(mosq->tls_cafile);
	if(mosq->tls_capath) _mosquitto_free(mosq->tls_capath);
	if(mosq->tls_certfile) _mosquitto_free(mosq->tls_certfile);
	if(mosq->tls_keyfile) _mosquitto_free(mosq->tls_keyfile);
	if(mosq->tls_pw_callback) mosq->tls_pw_callback = NULL;
	if(mosq->tls_version) _mosquitto_free(mosq->tls_version);
	if(mosq->tls_ciphers) _mosquitto_free(mosq->tls_ciphers);
	if(mosq->tls_psk) _mosquitto_free(mosq->tls_psk);
	if(mosq->tls_psk_identity) _mosquitto_free(mosq->tls_psk_identity);
#endif

	if(mosq->address){
		_mosquitto_free(mosq->address);
		mosq->address = NULL;
	}
	if(mosq->id){
		_mosquitto_free(mosq->id);
		mosq->id = NULL;
	}
	if(mosq->username){
		_mosquitto_free(mosq->username);
		mosq->username = NULL;
	}
	if(mosq->password){
		_mosquitto_free(mosq->password);
		mosq->password = NULL;
	}
	if(mosq->host){
		_mosquitto_free(mosq->host);
		mosq->host = NULL;
	}
	if(mosq->bind_address){
		_mosquitto_free(mosq->bind_address);
		mosq->bind_address = NULL;
	}

	/* Out packet cleanup */
	if(mosq->out_packet && !mosq->current_out_packet){
		mosq->current_out_packet = mosq->out_packet;
		mosq->out_packet = mosq->out_packet->next;
	}
	while(mosq->current_out_packet){
		packet = mosq->current_out_packet;
		/* Free data and reset values */
		mosq->current_out_packet = mosq->out_packet;
		if(mosq->out_packet){
			mosq->out_packet = mosq->out_packet->next;
		}

		_mosquitto_packet_cleanup(packet);
		_mosquitto_free(packet);
	}

	_mosquitto_packet_cleanup(&mosq->in_packet);
	if(mosq->sockpairR != INVALID_SOCKET){
		COMPAT_CLOSE(mosq->sockpairR);
		mosq->sockpairR = INVALID_SOCKET;
	}
	if(mosq->sockpairW != INVALID_SOCKET){
		COMPAT_CLOSE(mosq->sockpairW);
		mosq->sockpairW = INVALID_SOCKET;
	}
}

void mosquitto_destroy(struct mosquitto *mosq)
{
	if(!mosq) return;

	_mosquitto_destroy(mosq);
	_mosquitto_free(mosq);
}

int mosquitto_socket(struct mosquitto *mosq)
{
	if(!mosq) return INVALID_SOCKET;
	return mosq->sock;
}

static int _mosquitto_connect_init(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	if(!host || port <= 0) return MOSQ_ERR_INVAL;

	if(mosq->host) _mosquitto_free(mosq->host);
	mosq->host = _mosquitto_strdup(host);
	if(!mosq->host) return MOSQ_ERR_NOMEM;
	mosq->port = port;

	if(mosq->bind_address) _mosquitto_free(mosq->bind_address);
	if(bind_address){
		mosq->bind_address = _mosquitto_strdup(bind_address);
		if(!mosq->bind_address) return MOSQ_ERR_NOMEM;
	}

	mosq->keepalive = keepalive;

	if(mosq->sockpairR != INVALID_SOCKET){
		COMPAT_CLOSE(mosq->sockpairR);
		mosq->sockpairR = INVALID_SOCKET;
	}
	if(mosq->sockpairW != INVALID_SOCKET){
		COMPAT_CLOSE(mosq->sockpairW);
		mosq->sockpairW = INVALID_SOCKET;
	}

	if(_mosquitto_socketpair(&mosq->sockpairR, &mosq->sockpairW)){
		_mosquitto_log_printf(mosq, MOSQ_LOG_WARNING,
				"Warning: Unable to open socket pair, outgoing publish commands may be delayed.");
	}

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_connect(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
	return mosquitto_connect_bind(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	int rc;
	rc = _mosquitto_connect_init(mosq, host, port, keepalive, bind_address);
	if(rc) return rc;

	pthread_mutex_lock(&mosq->state_mutex);
	mosq->state = mosq_cs_new;
	pthread_mutex_unlock(&mosq->state_mutex);

	return _mosquitto_reconnect(mosq, true);
}

int mosquitto_connect_async(struct mosquitto *mosq, const char *host, int port, int keepalive)
{
	return mosquitto_connect_bind_async(mosq, host, port, keepalive, NULL);
}

int mosquitto_connect_bind_async(struct mosquitto *mosq, const char *host, int port, int keepalive, const char *bind_address)
{
	int rc = _mosquitto_connect_init(mosq, host, port, keepalive, bind_address);
	if(rc) return rc;

	pthread_mutex_lock(&mosq->state_mutex);
	mosq->state = mosq_cs_connect_async;
	pthread_mutex_unlock(&mosq->state_mutex);

	return _mosquitto_reconnect(mosq, false);
}

int mosquitto_reconnect_async(struct mosquitto *mosq)
{
	return _mosquitto_reconnect(mosq, false);
}

int mosquitto_reconnect(struct mosquitto *mosq)
{
	return _mosquitto_reconnect(mosq, true);
}

static int _mosquitto_reconnect(struct mosquitto *mosq, bool blocking)
{
	int rc;
	struct _mosquitto_packet *packet;
	if(!mosq) return MOSQ_ERR_INVAL;
	if(!mosq->host || mosq->port <= 0) return MOSQ_ERR_INVAL;

	pthread_mutex_lock(&mosq->state_mutex);
#ifdef WITH_SOCKS
	if(mosq->socks5_host){
		mosq->state = mosq_cs_socks5_new;
	}else
#endif
	{
		mosq->state = mosq_cs_new;
	}
	pthread_mutex_unlock(&mosq->state_mutex);

	pthread_mutex_lock(&mosq->msgtime_mutex);
	mosq->last_msg_in = mosquitto_time();
	mosq->last_msg_out = mosquitto_time();
	pthread_mutex_unlock(&mosq->msgtime_mutex);

	mosq->ping_t = 0;

	_mosquitto_packet_cleanup(&mosq->in_packet);
		
	pthread_mutex_lock(&mosq->current_out_packet_mutex);
	pthread_mutex_lock(&mosq->out_packet_mutex);

	if(mosq->out_packet && !mosq->current_out_packet){
		mosq->current_out_packet = mosq->out_packet;
		mosq->out_packet = mosq->out_packet->next;
	}

	while(mosq->current_out_packet){
		packet = mosq->current_out_packet;
		/* Free data and reset values */
		mosq->current_out_packet = mosq->out_packet;
		if(mosq->out_packet){
			mosq->out_packet = mosq->out_packet->next;
		}

		_mosquitto_packet_cleanup(packet);
		_mosquitto_free(packet);
	}
	pthread_mutex_unlock(&mosq->out_packet_mutex);
	pthread_mutex_unlock(&mosq->current_out_packet_mutex);

	_mosquitto_messages_reconnect_reset(mosq);

#ifdef WITH_SOCKS
	if(mosq->socks5_host){
		rc = _mosquitto_socket_connect(mosq, mosq->socks5_host, mosq->socks5_port, mosq->bind_address, blocking);
	}else
#endif
	{
		rc = _mosquitto_socket_connect(mosq, mosq->host, mosq->port, mosq->bind_address, blocking);
	}
	if(rc>0){
		return rc;
	}

#ifdef WITH_SOCKS
	if(mosq->socks5_host){
		return mosquitto__socks5_send(mosq);
	}else
#endif
	{
		return _mosquitto_send_connect(mosq, mosq->keepalive, mosq->clean_session);
	}
}

int mosquitto_disconnect(struct mosquitto *mosq)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	pthread_mutex_lock(&mosq->state_mutex);
	mosq->state = mosq_cs_disconnecting;
	pthread_mutex_unlock(&mosq->state_mutex);

	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;
	return _mosquitto_send_disconnect(mosq);
}

int mosquitto_publish(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	struct mosquitto_message_all *message;
	uint16_t local_mid;
	int queue_status;

	if(!mosq || !topic || qos<0 || qos>2) return MOSQ_ERR_INVAL;
	if(STREMPTY(topic)) return MOSQ_ERR_INVAL;
	if(payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD) return MOSQ_ERR_PAYLOAD_SIZE;

	if(mosquitto_pub_topic_check(topic) != MOSQ_ERR_SUCCESS){
		return MOSQ_ERR_INVAL;
	}

	local_mid = _mosquitto_mid_generate(mosq);
	if(mid){
		*mid = local_mid;
	}

	if(qos == 0){
		return _mosquitto_send_publish(mosq, local_mid, topic, payloadlen, payload, qos, retain, false);
	}else{
		message = _mosquitto_calloc(1, sizeof(struct mosquitto_message_all));
		if(!message) return MOSQ_ERR_NOMEM;

		message->next = NULL;
		message->timestamp = mosquitto_time();
		message->msg.mid = local_mid;
		message->msg.topic = _mosquitto_strdup(topic);
		if(!message->msg.topic){
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_NOMEM;
		}
		if(payloadlen){
			message->msg.payloadlen = payloadlen;
			message->msg.payload = _mosquitto_malloc(payloadlen*sizeof(uint8_t));
			if(!message->msg.payload){
				_mosquitto_message_cleanup(&message);
				return MOSQ_ERR_NOMEM;
			}
			memcpy(message->msg.payload, payload, payloadlen*sizeof(uint8_t));
		}else{
			message->msg.payloadlen = 0;
			message->msg.payload = NULL;
		}
		message->msg.qos = qos;
		message->msg.retain = retain;
		message->dup = false;

		pthread_mutex_lock(&mosq->out_message_mutex);
		queue_status = _mosquitto_message_queue(mosq, message, mosq_md_out);
		if(queue_status == 0){
			if(qos == 1){
				message->state = mosq_ms_wait_for_puback;
			}else if(qos == 2){
				message->state = mosq_ms_wait_for_pubrec;
			}
			pthread_mutex_unlock(&mosq->out_message_mutex);
			return _mosquitto_send_publish(mosq, message->msg.mid, message->msg.topic, message->msg.payloadlen, message->msg.payload, message->msg.qos, message->msg.retain, message->dup);
		}else{
			message->state = mosq_ms_invalid;
			pthread_mutex_unlock(&mosq->out_message_mutex);
			return MOSQ_ERR_SUCCESS;
		}
	}
}

int mosquitto_subscribe(struct mosquitto *mosq, int *mid, const char *sub, int qos)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	if(mosquitto_sub_topic_check(sub)) return MOSQ_ERR_INVAL;

	return _mosquitto_send_subscribe(mosq, mid, sub, qos);
}

int mosquitto_unsubscribe(struct mosquitto *mosq, int *mid, const char *sub)
{
	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	if(mosquitto_sub_topic_check(sub)) return MOSQ_ERR_INVAL;

	return _mosquitto_send_unsubscribe(mosq, mid, sub);
}

int mosquitto_tls_set(struct mosquitto *mosq, const char *cafile, const char *capath, const char *certfile, const char *keyfile, int (*pw_callback)(char *buf, int size, int rwflag, void *userdata))
{
#ifdef WITH_TLS
	FILE *fptr;

	if(!mosq || (!cafile && !capath) || (certfile && !keyfile) || (!certfile && keyfile)) return MOSQ_ERR_INVAL;

	if(cafile){
		fptr = _mosquitto_fopen(cafile, "rt");
		if(fptr){
			fclose(fptr);
		}else{
			return MOSQ_ERR_INVAL;
		}
		mosq->tls_cafile = _mosquitto_strdup(cafile);

		if(!mosq->tls_cafile){
			return MOSQ_ERR_NOMEM;
		}
	}else if(mosq->tls_cafile){
		_mosquitto_free(mosq->tls_cafile);
		mosq->tls_cafile = NULL;
	}

	if(capath){
		mosq->tls_capath = _mosquitto_strdup(capath);
		if(!mosq->tls_capath){
			return MOSQ_ERR_NOMEM;
		}
	}else if(mosq->tls_capath){
		_mosquitto_free(mosq->tls_capath);
		mosq->tls_capath = NULL;
	}

	if(certfile){
		fptr = _mosquitto_fopen(certfile, "rt");
		if(fptr){
			fclose(fptr);
		}else{
			if(mosq->tls_cafile){
				_mosquitto_free(mosq->tls_cafile);
				mosq->tls_cafile = NULL;
			}
			if(mosq->tls_capath){
				_mosquitto_free(mosq->tls_capath);
				mosq->tls_capath = NULL;
			}
			return MOSQ_ERR_INVAL;
		}
		mosq->tls_certfile = _mosquitto_strdup(certfile);
		if(!mosq->tls_certfile){
			return MOSQ_ERR_NOMEM;
		}
	}else{
		if(mosq->tls_certfile) _mosquitto_free(mosq->tls_certfile);
		mosq->tls_certfile = NULL;
	}

	if(keyfile){
		fptr = _mosquitto_fopen(keyfile, "rt");
		if(fptr){
			fclose(fptr);
		}else{
			if(mosq->tls_cafile){
				_mosquitto_free(mosq->tls_cafile);
				mosq->tls_cafile = NULL;
			}
			if(mosq->tls_capath){
				_mosquitto_free(mosq->tls_capath);
				mosq->tls_capath = NULL;
			}
			if(mosq->tls_certfile){
				_mosquitto_free(mosq->tls_certfile);
				mosq->tls_certfile = NULL;
			}
			return MOSQ_ERR_INVAL;
		}
		mosq->tls_keyfile = _mosquitto_strdup(keyfile);
		if(!mosq->tls_keyfile){
			return MOSQ_ERR_NOMEM;
		}
	}else{
		if(mosq->tls_keyfile) _mosquitto_free(mosq->tls_keyfile);
		mosq->tls_keyfile = NULL;
	}

	mosq->tls_pw_callback = pw_callback;


	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;

#endif
}

int mosquitto_tls_opts_set(struct mosquitto *mosq, int cert_reqs, const char *tls_version, const char *ciphers)
{
#ifdef WITH_TLS
	if(!mosq) return MOSQ_ERR_INVAL;

	mosq->tls_cert_reqs = cert_reqs;
	if(tls_version){
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
		if(!strcasecmp(tls_version, "tlsv1.2")
				|| !strcasecmp(tls_version, "tlsv1.1")
				|| !strcasecmp(tls_version, "tlsv1")){

			mosq->tls_version = _mosquitto_strdup(tls_version);
			if(!mosq->tls_version) return MOSQ_ERR_NOMEM;
		}else{
			return MOSQ_ERR_INVAL;
		}
#else
		if(!strcasecmp(tls_version, "tlsv1")){
			mosq->tls_version = _mosquitto_strdup(tls_version);
			if(!mosq->tls_version) return MOSQ_ERR_NOMEM;
		}else{
			return MOSQ_ERR_INVAL;
		}
#endif
	}else{
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
		mosq->tls_version = _mosquitto_strdup("tlsv1.2");
#else
		mosq->tls_version = _mosquitto_strdup("tlsv1");
#endif
		if(!mosq->tls_version) return MOSQ_ERR_NOMEM;
	}
	if(ciphers){
		mosq->tls_ciphers = _mosquitto_strdup(ciphers);
		if(!mosq->tls_ciphers) return MOSQ_ERR_NOMEM;
	}else{
		mosq->tls_ciphers = NULL;
	}


	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;

#endif
}


int mosquitto_tls_insecure_set(struct mosquitto *mosq, bool value)
{
#ifdef WITH_TLS
	if(!mosq) return MOSQ_ERR_INVAL;
	mosq->tls_insecure = value;
	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}


int mosquitto_tls_psk_set(struct mosquitto *mosq, const char *psk, const char *identity, const char *ciphers)
{
#ifdef REAL_WITH_TLS_PSK
	if(!mosq || !psk || !identity) return MOSQ_ERR_INVAL;

	/* Check for hex only digits */
	if(strspn(psk, "0123456789abcdefABCDEF") < strlen(psk)){
		return MOSQ_ERR_INVAL;
	}
	mosq->tls_psk = _mosquitto_strdup(psk);
	if(!mosq->tls_psk) return MOSQ_ERR_NOMEM;

	mosq->tls_psk_identity = _mosquitto_strdup(identity);
	if(!mosq->tls_psk_identity){
		_mosquitto_free(mosq->tls_psk);
		return MOSQ_ERR_NOMEM;
	}
	if(ciphers){
		mosq->tls_ciphers = _mosquitto_strdup(ciphers);
		if(!mosq->tls_ciphers) return MOSQ_ERR_NOMEM;
	}else{
		mosq->tls_ciphers = NULL;
	}

	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}


int mosquitto_loop(struct mosquitto *mosq, int timeout, int max_packets)
{
#ifdef HAVE_PSELECT
	struct timespec local_timeout;
#else
	struct timeval local_timeout;
#endif
	fd_set readfds, writefds;
	int fdcount;
	int rc;
	char pairbuf;
	int maxfd = 0;

	if(!mosq || max_packets < 1) return MOSQ_ERR_INVAL;
#ifndef WIN32
	if(mosq->sock >= FD_SETSIZE || mosq->sockpairR >= FD_SETSIZE){
		return MOSQ_ERR_INVAL;
	}
#endif

	FD_ZERO(&readfds);
	FD_ZERO(&writefds);
	if(mosq->sock != INVALID_SOCKET){
		maxfd = mosq->sock;
		FD_SET(mosq->sock, &readfds);
		pthread_mutex_lock(&mosq->current_out_packet_mutex);
		pthread_mutex_lock(&mosq->out_packet_mutex);
		if(mosq->out_packet || mosq->current_out_packet){
			FD_SET(mosq->sock, &writefds);
		}
#ifdef WITH_TLS
		if(mosq->ssl){
			if(mosq->want_write){
				FD_SET(mosq->sock, &writefds);
			}else if(mosq->want_connect){
				/* Remove possible FD_SET from above, we don't want to check
				 * for writing if we are still connecting, unless want_write is
				 * definitely set. The presence of outgoing packets does not
				 * matter yet. */
				FD_CLR(mosq->sock, &writefds);
			}
		}
#endif
		pthread_mutex_unlock(&mosq->out_packet_mutex);
		pthread_mutex_unlock(&mosq->current_out_packet_mutex);
	}else{
#ifdef WITH_SRV
		if(mosq->achan){
			pthread_mutex_lock(&mosq->state_mutex);
			if(mosq->state == mosq_cs_connect_srv){
				rc = ares_fds(mosq->achan, &readfds, &writefds);
				if(rc > maxfd){
					maxfd = rc;
				}
			}else{
				pthread_mutex_unlock(&mosq->state_mutex);
				return MOSQ_ERR_NO_CONN;
			}
			pthread_mutex_unlock(&mosq->state_mutex);
		}
#else
		return MOSQ_ERR_NO_CONN;
#endif
	}
	if(mosq->sockpairR != INVALID_SOCKET){
		/* sockpairR is used to break out of select() before the timeout, on a
		 * call to publish() etc. */
		FD_SET(mosq->sockpairR, &readfds);
		if(mosq->sockpairR > maxfd){
			maxfd = mosq->sockpairR;
		}
	}

	if(timeout >= 0){
		local_timeout.tv_sec = timeout/1000;
#ifdef HAVE_PSELECT
		local_timeout.tv_nsec = (timeout-local_timeout.tv_sec*1000)*1e6;
#else
		local_timeout.tv_usec = (timeout-local_timeout.tv_sec*1000)*1000;
#endif
	}else{
		local_timeout.tv_sec = 1;
#ifdef HAVE_PSELECT
		local_timeout.tv_nsec = 0;
#else
		local_timeout.tv_usec = 0;
#endif
	}

#ifdef HAVE_PSELECT
	fdcount = pselect(maxfd+1, &readfds, &writefds, NULL, &local_timeout, NULL);
#else
	fdcount = select(maxfd+1, &readfds, &writefds, NULL, &local_timeout);
#endif
	if(fdcount == -1){
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(errno == EINTR){
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_ERRNO;
		}
	}else{
		if(mosq->sock != INVALID_SOCKET){
			if(FD_ISSET(mosq->sock, &readfds)){
#ifdef WITH_TLS
				if(mosq->want_connect){
					rc = mosquitto__socket_connect_tls(mosq);
					if(rc) return rc;
				}else
#endif
				{
					do{
						rc = mosquitto_loop_read(mosq, max_packets);
						if(rc || mosq->sock == INVALID_SOCKET){
							return rc;
						}
					}while(SSL_DATA_PENDING(mosq));
				}
			}
			if(mosq->sockpairR != INVALID_SOCKET && FD_ISSET(mosq->sockpairR, &readfds)){
#ifndef WIN32
				if(read(mosq->sockpairR, &pairbuf, 1) == 0){
				}
#else
				recv(mosq->sockpairR, &pairbuf, 1, 0);
#endif
				/* Fake write possible, to stimulate output write even though
				 * we didn't ask for it, because at that point the publish or
				 * other command wasn't present. */
				FD_SET(mosq->sock, &writefds);
			}
			if(FD_ISSET(mosq->sock, &writefds)){
#ifdef WITH_TLS
				if(mosq->want_connect){
					rc = mosquitto__socket_connect_tls(mosq);
					if(rc) return rc;
				}else
#endif
				{
					rc = mosquitto_loop_write(mosq, max_packets);
					if(rc || mosq->sock == INVALID_SOCKET){
						return rc;
					}
				}
			}
		}
#ifdef WITH_SRV
		if(mosq->achan){
			ares_process(mosq->achan, &readfds, &writefds);
		}
#endif
	}
	return mosquitto_loop_misc(mosq);
}

int mosquitto_loop_forever(struct mosquitto *mosq, int timeout, int max_packets)
{
	int run = 1;
	int rc;
	unsigned int reconnects = 0;
	unsigned long reconnect_delay;

	if(!mosq) return MOSQ_ERR_INVAL;

	if(mosq->state == mosq_cs_connect_async){
		mosquitto_reconnect(mosq);
	}

	while(run){
		do{
			rc = mosquitto_loop(mosq, timeout, max_packets);
			if (reconnects !=0 && rc == MOSQ_ERR_SUCCESS){
				reconnects = 0;
			}
		}while(run && rc == MOSQ_ERR_SUCCESS);
		/* Quit after fatal errors. */
		switch(rc){
			case MOSQ_ERR_NOMEM:
			case MOSQ_ERR_PROTOCOL:
			case MOSQ_ERR_INVAL:
			case MOSQ_ERR_NOT_FOUND:
			case MOSQ_ERR_TLS:
			case MOSQ_ERR_PAYLOAD_SIZE:
			case MOSQ_ERR_NOT_SUPPORTED:
			case MOSQ_ERR_AUTH:
			case MOSQ_ERR_ACL_DENIED:
			case MOSQ_ERR_UNKNOWN:
			case MOSQ_ERR_EAI:
			case MOSQ_ERR_PROXY:
				return rc;
			case MOSQ_ERR_ERRNO:
				break;
		}
		if(errno == EPROTO){
			return rc;
		}
		do{
			rc = MOSQ_ERR_SUCCESS;
			pthread_mutex_lock(&mosq->state_mutex);
			if(mosq->state == mosq_cs_disconnecting){
				run = 0;
				pthread_mutex_unlock(&mosq->state_mutex);
			}else{
				pthread_mutex_unlock(&mosq->state_mutex);

				if(mosq->reconnect_delay > 0 && mosq->reconnect_exponential_backoff){
					reconnect_delay = mosq->reconnect_delay*reconnects*reconnects;
				}else{
					reconnect_delay = mosq->reconnect_delay;
				}

				if(reconnect_delay > mosq->reconnect_delay_max){
					reconnect_delay = mosq->reconnect_delay_max;
				}else{
					reconnects++;
				}

#ifdef WIN32
				Sleep(reconnect_delay*1000);
#else
				sleep(reconnect_delay);
#endif

				pthread_mutex_lock(&mosq->state_mutex);
				if(mosq->state == mosq_cs_disconnecting){
					run = 0;
					pthread_mutex_unlock(&mosq->state_mutex);
				}else{
					pthread_mutex_unlock(&mosq->state_mutex);
					rc = mosquitto_reconnect(mosq);
				}
			}
		}while(run && rc != MOSQ_ERR_SUCCESS);
	}
	return rc;
}

int mosquitto_loop_misc(struct mosquitto *mosq)
{
	time_t now;
	int rc;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	_mosquitto_check_keepalive(mosq);
	now = mosquitto_time();
	if(mosq->last_retry_check+1 < now){
		_mosquitto_message_retry_check(mosq);
		mosq->last_retry_check = now;
	}
	if(mosq->ping_t && now - mosq->ping_t >= mosq->keepalive){
		/* mosq->ping_t != 0 means we are waiting for a pingresp.
		 * This hasn't happened in the keepalive time so we should disconnect.
		 */
		_mosquitto_socket_close(mosq);
		pthread_mutex_lock(&mosq->state_mutex);
		if(mosq->state == mosq_cs_disconnecting){
			rc = MOSQ_ERR_SUCCESS;
		}else{
			rc = 1;
		}
		pthread_mutex_unlock(&mosq->state_mutex);
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_disconnect){
			mosq->in_callback = true;
			mosq->on_disconnect(mosq, mosq->userdata, rc);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
		return MOSQ_ERR_CONN_LOST;
	}
	return MOSQ_ERR_SUCCESS;
}

static int _mosquitto_loop_rc_handle(struct mosquitto *mosq, int rc)
{
	if(rc){
		_mosquitto_socket_close(mosq);
		pthread_mutex_lock(&mosq->state_mutex);
		if(mosq->state == mosq_cs_disconnecting){
			rc = MOSQ_ERR_SUCCESS;
		}
		pthread_mutex_unlock(&mosq->state_mutex);
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_disconnect){
			mosq->in_callback = true;
			mosq->on_disconnect(mosq, mosq->userdata, rc);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
		return rc;
	}
	return rc;
}

int mosquitto_loop_read(struct mosquitto *mosq, int max_packets)
{
	int rc;
	int i;
	if(max_packets < 1) return MOSQ_ERR_INVAL;

	pthread_mutex_lock(&mosq->out_message_mutex);
	max_packets = mosq->out_queue_len;
	pthread_mutex_unlock(&mosq->out_message_mutex);

	pthread_mutex_lock(&mosq->in_message_mutex);
	max_packets += mosq->in_queue_len;
	pthread_mutex_unlock(&mosq->in_message_mutex);

	if(max_packets < 1) max_packets = 1;
	/* Queue len here tells us how many messages are awaiting processing and
	 * have QoS > 0. We should try to deal with that many in this loop in order
	 * to keep up. */
	for(i=0; i<max_packets; i++){
#ifdef WITH_SOCKS
		if(mosq->socks5_host){
			rc = mosquitto__socks5_read(mosq);
		}else
#endif
		{
			rc = _mosquitto_packet_read(mosq);
		}
		if(rc || errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
			return _mosquitto_loop_rc_handle(mosq, rc);
		}
	}
	return rc;
}

int mosquitto_loop_write(struct mosquitto *mosq, int max_packets)
{
	int rc;
	int i;
	if(max_packets < 1) return MOSQ_ERR_INVAL;

	pthread_mutex_lock(&mosq->out_message_mutex);
	max_packets = mosq->out_queue_len;
	pthread_mutex_unlock(&mosq->out_message_mutex);

	pthread_mutex_lock(&mosq->in_message_mutex);
	max_packets += mosq->in_queue_len;
	pthread_mutex_unlock(&mosq->in_message_mutex);

	if(max_packets < 1) max_packets = 1;
	/* Queue len here tells us how many messages are awaiting processing and
	 * have QoS > 0. We should try to deal with that many in this loop in order
	 * to keep up. */
	for(i=0; i<max_packets; i++){
		rc = _mosquitto_packet_write(mosq);
		if(rc || errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
			return _mosquitto_loop_rc_handle(mosq, rc);
		}
	}
	return rc;
}

bool mosquitto_want_write(struct mosquitto *mosq)
{
	if(mosq->out_packet || mosq->current_out_packet){
		return true;
#ifdef WITH_TLS
	}else if(mosq->ssl && mosq->want_write){
		return true;
#endif
	}else{
		return false;
	}
}

int mosquitto_opts_set(struct mosquitto *mosq, enum mosq_opt_t option, void *value)
{
	int ival;

	if(!mosq || !value) return MOSQ_ERR_INVAL;

	switch(option){
		case MOSQ_OPT_PROTOCOL_VERSION:
			ival = *((int *)value);
			if(ival == MQTT_PROTOCOL_V31){
				mosq->protocol = mosq_p_mqtt31;
			}else if(ival == MQTT_PROTOCOL_V311){
				mosq->protocol = mosq_p_mqtt311;
			}else{
				return MOSQ_ERR_INVAL;
			}
			break;
		default:
			return MOSQ_ERR_INVAL;
	}
	return MOSQ_ERR_SUCCESS;
}


void mosquitto_connect_callback_set(struct mosquitto *mosq, void (*on_connect)(struct mosquitto *, void *, int))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_connect = on_connect;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_disconnect_callback_set(struct mosquitto *mosq, void (*on_disconnect)(struct mosquitto *, void *, int))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_disconnect = on_disconnect;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_publish_callback_set(struct mosquitto *mosq, void (*on_publish)(struct mosquitto *, void *, int))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_publish = on_publish;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_message_callback_set(struct mosquitto *mosq, void (*on_message)(struct mosquitto *, void *, const struct mosquitto_message *))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_message = on_message;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_subscribe_callback_set(struct mosquitto *mosq, void (*on_subscribe)(struct mosquitto *, void *, int, int, const int *))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_subscribe = on_subscribe;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_unsubscribe_callback_set(struct mosquitto *mosq, void (*on_unsubscribe)(struct mosquitto *, void *, int))
{
	pthread_mutex_lock(&mosq->callback_mutex);
	mosq->on_unsubscribe = on_unsubscribe;
	pthread_mutex_unlock(&mosq->callback_mutex);
}

void mosquitto_log_callback_set(struct mosquitto *mosq, void (*on_log)(struct mosquitto *, void *, int, const char *))
{
	pthread_mutex_lock(&mosq->log_callback_mutex);
	mosq->on_log = on_log;
	pthread_mutex_unlock(&mosq->log_callback_mutex);
}

void mosquitto_user_data_set(struct mosquitto *mosq, void *userdata)
{
	if(mosq){
		mosq->userdata = userdata;
	}
}

const char *mosquitto_strerror(int mosq_errno)
{
	switch(mosq_errno){
		case MOSQ_ERR_CONN_PENDING:
			return "Connection pending.";
		case MOSQ_ERR_SUCCESS:
			return "No error.";
		case MOSQ_ERR_NOMEM:
			return "Out of memory.";
		case MOSQ_ERR_PROTOCOL:
			return "A network protocol error occurred when communicating with the broker.";
		case MOSQ_ERR_INVAL:
			return "Invalid function arguments provided.";
		case MOSQ_ERR_NO_CONN:
			return "The client is not currently connected.";
		case MOSQ_ERR_CONN_REFUSED:
			return "The connection was refused.";
		case MOSQ_ERR_NOT_FOUND:
			return "Message not found (internal error).";
		case MOSQ_ERR_CONN_LOST:
			return "The connection was lost.";
		case MOSQ_ERR_TLS:
			return "A TLS error occurred.";
		case MOSQ_ERR_PAYLOAD_SIZE:
			return "Payload too large.";
		case MOSQ_ERR_NOT_SUPPORTED:
			return "This feature is not supported.";
		case MOSQ_ERR_AUTH:
			return "Authorisation failed.";
		case MOSQ_ERR_ACL_DENIED:
			return "Access denied by ACL.";
		case MOSQ_ERR_UNKNOWN:
			return "Unknown error.";
		case MOSQ_ERR_ERRNO:
			return strerror(errno);
		case MOSQ_ERR_EAI:
			return "Lookup error.";
		case MOSQ_ERR_PROXY:
			return "Proxy error.";
		default:
			return "Unknown error.";
	}
}

const char *mosquitto_connack_string(int connack_code)
{
	switch(connack_code){
		case 0:
			return "Connection Accepted.";
		case 1:
			return "Connection Refused: unacceptable protocol version.";
		case 2:
			return "Connection Refused: identifier rejected.";
		case 3:
			return "Connection Refused: broker unavailable.";
		case 4:
			return "Connection Refused: bad user name or password.";
		case 5:
			return "Connection Refused: not authorised.";
		default:
			return "Connection Refused: unknown reason.";
	}
}

int mosquitto_sub_topic_tokenise(const char *subtopic, char ***topics, int *count)
{
	int len;
	int hier_count = 1;
	int start, stop;
	int hier;
	int tlen;
	int i, j;

	if(!subtopic || !topics || !count) return MOSQ_ERR_INVAL;

	len = strlen(subtopic);

	for(i=0; i<len; i++){
		if(subtopic[i] == '/'){
			if(i > len-1){
				/* Separator at end of line */
			}else{
				hier_count++;
			}
		}
	}

	(*topics) = _mosquitto_calloc(hier_count, sizeof(char *));
	if(!(*topics)) return MOSQ_ERR_NOMEM;

	start = 0;
	stop = 0;
	hier = 0;

	for(i=0; i<len+1; i++){
		if(subtopic[i] == '/' || subtopic[i] == '\0'){
			stop = i;
			if(start != stop){
				tlen = stop-start + 1;
				(*topics)[hier] = _mosquitto_calloc(tlen, sizeof(char));
				if(!(*topics)[hier]){
					for(i=0; i<hier_count; i++){
						if((*topics)[hier]){
							_mosquitto_free((*topics)[hier]);
						}
					}
					_mosquitto_free((*topics));
					return MOSQ_ERR_NOMEM;
				}
				for(j=start; j<stop; j++){
					(*topics)[hier][j-start] = subtopic[j];
				}
			}
			start = i+1;
			hier++;
		}
	}

	*count = hier_count;

	return MOSQ_ERR_SUCCESS;
}

int mosquitto_sub_topic_tokens_free(char ***topics, int count)
{
	int i;

	if(!topics || !(*topics) || count<1) return MOSQ_ERR_INVAL;

	for(i=0; i<count; i++){
		if((*topics)[i]) _mosquitto_free((*topics)[i]);
	}
	_mosquitto_free(*topics);

	return MOSQ_ERR_SUCCESS;
}


/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/
#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <mosquitto_internal.h>
#include <mosquitto.h>
#include <memory_mosq.h>

int _mosquitto_log_printf(struct mosquitto *mosq, int priority, const char *fmt, ...)
{
	va_list va;
	char *s;
	int len;

	assert(mosq);
	assert(fmt);

	pthread_mutex_lock(&mosq->log_callback_mutex);
	if(mosq->on_log){
		len = strlen(fmt) + 500;
		s = _mosquitto_malloc(len*sizeof(char));
		if(!s){
			pthread_mutex_unlock(&mosq->log_callback_mutex);
			return MOSQ_ERR_NOMEM;
		}

		va_start(va, fmt);
		vsnprintf(s, len, fmt, va);
		va_end(va);
		s[len-1] = '\0'; /* Ensure string is null terminated. */

		mosq->on_log(mosq, mosq->userdata, priority, s);

		_mosquitto_free(s);
	}
	pthread_mutex_unlock(&mosq->log_callback_mutex);

	return MOSQ_ERR_SUCCESS;
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <config.h>

#include <stdlib.h>
#include <string.h>

#include <memory_mosq.h>

#ifdef REAL_WITH_MEMORY_TRACKING
#  if defined(__APPLE__)
#    include <malloc/malloc.h>
#    define malloc_usable_size malloc_size
#  elif defined(__FreeBSD__)
#    include <malloc_np.h>
#  else
#    include <malloc.h>
#  endif
#endif

#ifdef REAL_WITH_MEMORY_TRACKING
static unsigned long memcount = 0;
static unsigned long max_memcount = 0;
#endif

void *_mosquitto_calloc(size_t nmemb, size_t size)
{
	void *mem = calloc(nmemb, size);

#ifdef REAL_WITH_MEMORY_TRACKING
	memcount += malloc_usable_size(mem);
	if(memcount > max_memcount){
		max_memcount = memcount;
	}
#endif

	return mem;
}

void _mosquitto_free(void *mem)
{
#ifdef REAL_WITH_MEMORY_TRACKING
	if(!mem){
		return;
	}
	memcount -= malloc_usable_size(mem);
#endif
	free(mem);
}

void *_mosquitto_malloc(size_t size)
{
	void *mem = malloc(size);

#ifdef REAL_WITH_MEMORY_TRACKING
	memcount += malloc_usable_size(mem);
	if(memcount > max_memcount){
		max_memcount = memcount;
	}
#endif

	return mem;
}

#ifdef REAL_WITH_MEMORY_TRACKING
unsigned long _mosquitto_memory_used(void)
{
	return memcount;
}

unsigned long _mosquitto_max_memory_used(void)
{
	return max_memcount;
}
#endif

void *_mosquitto_realloc(void *ptr, size_t size)
{
	void *mem;
#ifdef REAL_WITH_MEMORY_TRACKING
	if(ptr){
		memcount -= malloc_usable_size(ptr);
	}
#endif
	mem = realloc(ptr, size);

#ifdef REAL_WITH_MEMORY_TRACKING
	memcount += malloc_usable_size(mem);
	if(memcount > max_memcount){
		max_memcount = memcount;
	}
#endif

	return mem;
}

char *_mosquitto_strdup(const char *s)
{
	char *str = strdup(s);

#ifdef REAL_WITH_MEMORY_TRACKING
	memcount += malloc_usable_size(str);
	if(memcount > max_memcount){
		max_memcount = memcount;
	}
#endif

	return str;
}

/*
Copyright (c) 2010-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <mosquitto_internal.h>
#include <mosquitto.h>
#include <memory_mosq.h>
#include <messages_mosq.h>
#include <send_mosq.h>
#include <time_mosq.h>

void _mosquitto_message_cleanup(struct mosquitto_message_all **message)
{
	struct mosquitto_message_all *msg;

	if(!message || !*message) return;

	msg = *message;

	if(msg->msg.topic) _mosquitto_free(msg->msg.topic);
	if(msg->msg.payload) _mosquitto_free(msg->msg.payload);
	_mosquitto_free(msg);
}

void _mosquitto_message_cleanup_all(struct mosquitto *mosq)
{
	struct mosquitto_message_all *tmp;

	assert(mosq);

	while(mosq->in_messages){
		tmp = mosq->in_messages->next;
		_mosquitto_message_cleanup(&mosq->in_messages);
		mosq->in_messages = tmp;
	}
	while(mosq->out_messages){
		tmp = mosq->out_messages->next;
		_mosquitto_message_cleanup(&mosq->out_messages);
		mosq->out_messages = tmp;
	}
}

int mosquitto_message_copy(struct mosquitto_message *dst, const struct mosquitto_message *src)
{
	if(!dst || !src) return MOSQ_ERR_INVAL;

	dst->mid = src->mid;
	dst->topic = _mosquitto_strdup(src->topic);
	if(!dst->topic) return MOSQ_ERR_NOMEM;
	dst->qos = src->qos;
	dst->retain = src->retain;
	if(src->payloadlen){
		dst->payload = _mosquitto_malloc(src->payloadlen);
		if(!dst->payload){
			_mosquitto_free(dst->topic);
			return MOSQ_ERR_NOMEM;
		}
		memcpy(dst->payload, src->payload, src->payloadlen);
		dst->payloadlen = src->payloadlen;
	}else{
		dst->payloadlen = 0;
		dst->payload = NULL;
	}
	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_message_delete(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir)
{
	struct mosquitto_message_all *message;
	int rc;
	assert(mosq);

	rc = _mosquitto_message_remove(mosq, mid, dir, &message);
	if(rc == MOSQ_ERR_SUCCESS){
		_mosquitto_message_cleanup(&message);
	}
	return rc;
}

void mosquitto_message_free(struct mosquitto_message **message)
{
	struct mosquitto_message *msg;

	if(!message || !*message) return;

	msg = *message;

	if(msg->topic) _mosquitto_free(msg->topic);
	if(msg->payload) _mosquitto_free(msg->payload);
	_mosquitto_free(msg);
}


/*
 * Function: _mosquitto_message_queue
 *
 * Returns:
 *	0 - to indicate an outgoing message can be started
 *	1 - to indicate that the outgoing message queue is full (inflight limit has been reached)
 */
int _mosquitto_message_queue(struct mosquitto *mosq, struct mosquitto_message_all *message, enum mosquitto_msg_direction dir)
{
	int rc = 0;

	/* mosq->*_message_mutex should be locked before entering this function */
	assert(mosq);
	assert(message);

	if(dir == mosq_md_out){
		mosq->out_queue_len++;
		message->next = NULL;
		if(mosq->out_messages_last){
			mosq->out_messages_last->next = message;
		}else{
			mosq->out_messages = message;
		}
		mosq->out_messages_last = message;
		if(message->msg.qos > 0){
			if(mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages){
				mosq->inflight_messages++;
			}else{
				rc = 1;
			}
		}
	}else{
		mosq->in_queue_len++;
		message->next = NULL;
		if(mosq->in_messages_last){
			mosq->in_messages_last->next = message;
		}else{
			mosq->in_messages = message;
		}
		mosq->in_messages_last = message;
	}
	return rc;
}

void _mosquitto_messages_reconnect_reset(struct mosquitto *mosq)
{
	struct mosquitto_message_all *message;
	struct mosquitto_message_all *prev = NULL;
	assert(mosq);

	pthread_mutex_lock(&mosq->in_message_mutex);
	message = mosq->in_messages;
	mosq->in_queue_len = 0;
	while(message){
		mosq->in_queue_len++;
		message->timestamp = 0;
		if(message->msg.qos != 2){
			if(prev){
				prev->next = message->next;
				_mosquitto_message_cleanup(&message);
				message = prev;
			}else{
				mosq->in_messages = message->next;
				_mosquitto_message_cleanup(&message);
				message = mosq->in_messages;
			}
		}else{
			/* Message state can be preserved here because it should match
			* whatever the client has got. */
		}
		prev = message;
		message = message->next;
	}
	mosq->in_messages_last = prev;
	pthread_mutex_unlock(&mosq->in_message_mutex);


	pthread_mutex_lock(&mosq->out_message_mutex);
	mosq->inflight_messages = 0;
	message = mosq->out_messages;
	mosq->out_queue_len = 0;
	while(message){
		mosq->out_queue_len++;
		message->timestamp = 0;

		if(mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages){
			if(message->msg.qos > 0){
				mosq->inflight_messages++;
			}
			if(message->msg.qos == 1){
				message->state = mosq_ms_wait_for_puback;
			}else if(message->msg.qos == 2){
				/* Should be able to preserve state. */
			}
		}else{
			message->state = mosq_ms_invalid;
		}
		prev = message;
		message = message->next;
	}
	mosq->out_messages_last = prev;
	pthread_mutex_unlock(&mosq->out_message_mutex);
}

int _mosquitto_message_remove(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_direction dir, struct mosquitto_message_all **message)
{
	struct mosquitto_message_all *cur, *prev = NULL;
	bool found = false;
	int rc;
	assert(mosq);
	assert(message);

	if(dir == mosq_md_out){
		pthread_mutex_lock(&mosq->out_message_mutex);
		cur = mosq->out_messages;
		while(cur){
			if(cur->msg.mid == mid){
				if(prev){
					prev->next = cur->next;
				}else{
					mosq->out_messages = cur->next;
				}
				*message = cur;
				mosq->out_queue_len--;
				if(cur->next == NULL){
					mosq->out_messages_last = prev;
				}else if(!mosq->out_messages){
					mosq->out_messages_last = NULL;
				}
				if(cur->msg.qos > 0){
					mosq->inflight_messages--;
				}
				found = true;
				break;
			}
			prev = cur;
			cur = cur->next;
		}

		if(found){
			cur = mosq->out_messages;
			while(cur){
				if(mosq->max_inflight_messages == 0 || mosq->inflight_messages < mosq->max_inflight_messages){
					if(cur->msg.qos > 0 && cur->state == mosq_ms_invalid){
						mosq->inflight_messages++;
						if(cur->msg.qos == 1){
							cur->state = mosq_ms_wait_for_puback;
						}else if(cur->msg.qos == 2){
							cur->state = mosq_ms_wait_for_pubrec;
						}
						rc = _mosquitto_send_publish(mosq, cur->msg.mid, cur->msg.topic, cur->msg.payloadlen, cur->msg.payload, cur->msg.qos, cur->msg.retain, cur->dup);
						if(rc){
							pthread_mutex_unlock(&mosq->out_message_mutex);
							return rc;
						}
					}
				}else{
					pthread_mutex_unlock(&mosq->out_message_mutex);
					return MOSQ_ERR_SUCCESS;
				}
				cur = cur->next;
			}
			pthread_mutex_unlock(&mosq->out_message_mutex);
			return MOSQ_ERR_SUCCESS;
		}else{
			pthread_mutex_unlock(&mosq->out_message_mutex);
			return MOSQ_ERR_NOT_FOUND;
		}
	}else{
		pthread_mutex_lock(&mosq->in_message_mutex);
		cur = mosq->in_messages;
		while(cur){
			if(cur->msg.mid == mid){
				if(prev){
					prev->next = cur->next;
				}else{
					mosq->in_messages = cur->next;
				}
				*message = cur;
				mosq->in_queue_len--;
				if(cur->next == NULL){
					mosq->in_messages_last = prev;
				}else if(!mosq->in_messages){
					mosq->in_messages_last = NULL;
				}
				found = true;
				break;
			}
			prev = cur;
			cur = cur->next;
		}

		pthread_mutex_unlock(&mosq->in_message_mutex);
		if(found){
			return MOSQ_ERR_SUCCESS;
		}else{
			return MOSQ_ERR_NOT_FOUND;
		}
	}
}

#ifdef WITH_THREADING
void _mosquitto_message_retry_check_actual(struct mosquitto *mosq, struct mosquitto_message_all *messages, pthread_mutex_t *mutex)
#else
void _mosquitto_message_retry_check_actual(struct mosquitto *mosq, struct mosquitto_message_all *messages)
#endif
{
	time_t now = mosquitto_time();
	assert(mosq);

#ifdef WITH_THREADING
	pthread_mutex_lock(mutex);
#endif

	while(messages){
		if(messages->timestamp + mosq->message_retry < now){
			switch(messages->state){
				case mosq_ms_wait_for_puback:
				case mosq_ms_wait_for_pubrec:
					messages->timestamp = now;
					messages->dup = true;
					_mosquitto_send_publish(mosq, messages->msg.mid, messages->msg.topic, messages->msg.payloadlen, messages->msg.payload, messages->msg.qos, messages->msg.retain, messages->dup);
					break;
				case mosq_ms_wait_for_pubrel:
					messages->timestamp = now;
					messages->dup = true;
					_mosquitto_send_pubrec(mosq, messages->msg.mid);
					break;
				case mosq_ms_wait_for_pubcomp:
					messages->timestamp = now;
					messages->dup = true;
					_mosquitto_send_pubrel(mosq, messages->msg.mid);
					break;
				default:
					break;
			}
		}
		messages = messages->next;
	}
#ifdef WITH_THREADING
	pthread_mutex_unlock(mutex);
#endif
}

void _mosquitto_message_retry_check(struct mosquitto *mosq)
{
#ifdef WITH_THREADING
	_mosquitto_message_retry_check_actual(mosq, mosq->out_messages, &mosq->out_message_mutex);
	_mosquitto_message_retry_check_actual(mosq, mosq->in_messages, &mosq->in_message_mutex);
#else
	_mosquitto_message_retry_check_actual(mosq, mosq->out_messages);
	_mosquitto_message_retry_check_actual(mosq, mosq->in_messages);
#endif
}

void mosquitto_message_retry_set(struct mosquitto *mosq, unsigned int message_retry)
{
	assert(mosq);
	if(mosq) mosq->message_retry = message_retry;
}

int _mosquitto_message_out_update(struct mosquitto *mosq, uint16_t mid, enum mosquitto_msg_state state)
{
	struct mosquitto_message_all *message;
	assert(mosq);

	pthread_mutex_lock(&mosq->out_message_mutex);
	message = mosq->out_messages;
	while(message){
		if(message->msg.mid == mid){
			message->state = state;
			message->timestamp = mosquitto_time();
			pthread_mutex_unlock(&mosq->out_message_mutex);
			return MOSQ_ERR_SUCCESS;
		}
		message = message->next;
	}
	pthread_mutex_unlock(&mosq->out_message_mutex);
	return MOSQ_ERR_NOT_FOUND;
}

int mosquitto_max_inflight_messages_set(struct mosquitto *mosq, unsigned int max_inflight_messages)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	mosq->max_inflight_messages = max_inflight_messages;

	return MOSQ_ERR_SUCCESS;
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef __ANDROID__
#include <linux/in.h>
#include <linux/in6.h>
#include <sys/endian.h>
#endif

#ifdef __FreeBSD__
#  include <netinet/in.h>
#endif

#ifdef __SYMBIAN32__
#include <netinet/in.h>
#endif

#ifdef __QNX__
#ifndef AI_ADDRCONFIG
#define AI_ADDRCONFIG 0
#endif
#include <net/netbyte.h>
#include <netinet/in.h>
#endif

#ifdef WITH_TLS
#include <openssl/conf.h>
#include <openssl/engine.h>
#include <openssl/err.h>
#include <tls_mosq.h>
#endif

#ifdef WITH_BROKER
#  include <mosquitto_broker.h>
#  ifdef WITH_SYS_TREE
   extern uint64_t g_bytes_received;
   extern uint64_t g_bytes_sent;
   extern unsigned long g_msgs_received;
   extern unsigned long g_msgs_sent;
   extern unsigned long g_pub_msgs_received;
   extern unsigned long g_pub_msgs_sent;
#  endif
#  ifdef WITH_WEBSOCKETS
#    include <libwebsockets.h>
#  endif
#else
#  include <read_handle.h>
#endif

#include <logging_mosq.h>
#include <memory_mosq.h>
#include <mqtt3_protocol.h>
#include <net_mosq.h>
#include <time_mosq.h>
#include <util_mosq.h>

#include "config.h"

#ifdef WITH_TLS
int tls_ex_index_mosq = -1;
#endif

void _mosquitto_net_init(void)
{
#ifdef WIN32
	WSADATA wsaData;
	WSAStartup(MAKEWORD(2,2), &wsaData);
#endif

#ifdef WITH_SRV
	ares_library_init(ARES_LIB_INIT_ALL);
#endif

#ifdef WITH_TLS
	SSL_load_error_strings();
	SSL_library_init();
	OpenSSL_add_all_algorithms();
	if(tls_ex_index_mosq == -1){
		tls_ex_index_mosq = SSL_get_ex_new_index(0, "client context", NULL, NULL, NULL);
	}
#endif
}

void _mosquitto_net_cleanup(void)
{
#ifdef WITH_TLS
	ERR_remove_state(0);
	ENGINE_cleanup();
	CONF_modules_unload(1);
	ERR_free_strings();
	EVP_cleanup();
	CRYPTO_cleanup_all_ex_data();
#endif

#ifdef WITH_SRV
	ares_library_cleanup();
#endif

#ifdef WIN32
	WSACleanup();
#endif
}

void _mosquitto_packet_cleanup(struct _mosquitto_packet *packet)
{
	if(!packet) return;

	/* Free data and reset values */
	packet->command = 0;
	packet->remaining_count = 0;
	packet->remaining_mult = 1;
	packet->remaining_length = 0;
	if(packet->payload) _mosquitto_free(packet->payload);
	packet->payload = NULL;
	packet->to_process = 0;
	packet->pos = 0;
}

int _mosquitto_packet_queue(struct mosquitto *mosq, struct _mosquitto_packet *packet)
{
#ifndef WITH_BROKER
	char sockpair_data = 0;
#endif
	assert(mosq);
	assert(packet);

	packet->pos = 0;
	packet->to_process = packet->packet_length;

	packet->next = NULL;
	pthread_mutex_lock(&mosq->out_packet_mutex);
	if(mosq->out_packet){
		mosq->out_packet_last->next = packet;
	}else{
		mosq->out_packet = packet;
	}
	mosq->out_packet_last = packet;
	pthread_mutex_unlock(&mosq->out_packet_mutex);
#ifdef WITH_BROKER
#  ifdef WITH_WEBSOCKETS
	if(mosq->wsi){
		libwebsocket_callback_on_writable(mosq->ws_context, mosq->wsi);
		return 0;
	}else{
		return _mosquitto_packet_write(mosq);
	}
#  else
	return _mosquitto_packet_write(mosq);
#  endif
#else

	/* Write a single byte to sockpairW (connected to sockpairR) to break out
	 * of select() if in threaded mode. */
	if(mosq->sockpairW != INVALID_SOCKET){
#ifndef WIN32
		if(write(mosq->sockpairW, &sockpair_data, 1)){
		}
#else
		send(mosq->sockpairW, &sockpair_data, 1, 0);
#endif
	}

	if(mosq->in_callback == false && mosq->threaded == false){
		return _mosquitto_packet_write(mosq);
	}else{
		return MOSQ_ERR_SUCCESS;
	}
#endif
}

/* Close a socket associated with a context and set it to -1.
 * Returns 1 on failure (context is NULL)
 * Returns 0 on success.
 */
#ifdef WITH_BROKER
int _mosquitto_socket_close(struct mosquitto_db *db, struct mosquitto *mosq)
#else
int _mosquitto_socket_close(struct mosquitto *mosq)
#endif
{
	int rc = 0;

	assert(mosq);
#ifdef WITH_TLS
	if(mosq->ssl){
		SSL_shutdown(mosq->ssl);
		SSL_free(mosq->ssl);
		mosq->ssl = NULL;
	}
	if(mosq->ssl_ctx){
		SSL_CTX_free(mosq->ssl_ctx);
		mosq->ssl_ctx = NULL;
	}
#endif

	if((int)mosq->sock >= 0){
#ifdef WITH_BROKER
		HASH_DELETE(hh_sock, db->contexts_by_sock, mosq);
#endif
		rc = COMPAT_CLOSE(mosq->sock);
		mosq->sock = INVALID_SOCKET;
#ifdef WITH_WEBSOCKETS
	}else if(mosq->sock == WEBSOCKET_CLIENT){
		if(mosq->state != mosq_cs_disconnecting){
			mosq->state = mosq_cs_disconnect_ws;
		}
		if(mosq->wsi){
			libwebsocket_callback_on_writable(mosq->ws_context, mosq->wsi);
		}
		mosq->sock = INVALID_SOCKET;
#endif
	}

#ifdef WITH_BROKER
	if(mosq->listener){
		mosq->listener->client_count--;
		assert(mosq->listener->client_count >= 0);
		mosq->listener = NULL;
	}
#endif

	return rc;
}

#ifdef REAL_WITH_TLS_PSK
static unsigned int psk_client_callback(SSL *ssl, const char *hint,
		char *identity, unsigned int max_identity_len,
		unsigned char *psk, unsigned int max_psk_len)
{
	struct mosquitto *mosq;
	int len;

	mosq = SSL_get_ex_data(ssl, tls_ex_index_mosq);
	if(!mosq) return 0;

	snprintf(identity, max_identity_len, "%s", mosq->tls_psk_identity);

	len = _mosquitto_hex2bin(mosq->tls_psk, psk, max_psk_len);
	if (len < 0) return 0;
	return len;
}
#endif

int _mosquitto_try_connect(struct mosquitto *mosq, const char *host, uint16_t port, mosq_sock_t *sock, const char *bind_address, bool blocking)
{
	struct addrinfo hints;
	struct addrinfo *ainfo, *rp;
	struct addrinfo *ainfo_bind, *rp_bind;
	int s;
	int rc = MOSQ_ERR_SUCCESS;
#ifdef WIN32
	uint32_t val = 1;
#endif

	*sock = INVALID_SOCKET;
	memset(&hints, 0, sizeof(struct addrinfo));
#ifdef WITH_TLS
	if(mosq->tls_cafile || mosq->tls_capath || mosq->tls_psk){
		hints.ai_family = PF_INET;
	}else
#endif
	{
		hints.ai_family = PF_UNSPEC;
	}
	hints.ai_flags = AI_ADDRCONFIG;
	hints.ai_socktype = SOCK_STREAM;

	s = getaddrinfo(host, NULL, &hints, &ainfo);
	if(s){
		errno = s;
		return MOSQ_ERR_EAI;
	}

	if(bind_address){
		s = getaddrinfo(bind_address, NULL, &hints, &ainfo_bind);
		if(s){
			freeaddrinfo(ainfo);
			errno = s;
			return MOSQ_ERR_EAI;
		}
	}

	for(rp = ainfo; rp != NULL; rp = rp->ai_next){
		*sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if(*sock == INVALID_SOCKET) continue;
		
		if(rp->ai_family == PF_INET){
			((struct sockaddr_in *)rp->ai_addr)->sin_port = htons(port);
		}else if(rp->ai_family == PF_INET6){
			((struct sockaddr_in6 *)rp->ai_addr)->sin6_port = htons(port);
		}else{
			COMPAT_CLOSE(*sock);
			continue;
		}

		if(bind_address){
			for(rp_bind = ainfo_bind; rp_bind != NULL; rp_bind = rp_bind->ai_next){
				if(bind(*sock, rp_bind->ai_addr, rp_bind->ai_addrlen) == 0){
					break;
				}
			}
			if(!rp_bind){
				COMPAT_CLOSE(*sock);
				continue;
			}
		}

		if(!blocking){
			/* Set non-blocking */
			if(_mosquitto_socket_nonblock(*sock)){
				COMPAT_CLOSE(*sock);
				continue;
			}
		}

		rc = connect(*sock, rp->ai_addr, rp->ai_addrlen);
#ifdef WIN32
		errno = WSAGetLastError();
#endif
		if(rc == 0 || errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK){
			if(rc < 0 && (errno == EINPROGRESS || errno == COMPAT_EWOULDBLOCK)){
				rc = MOSQ_ERR_CONN_PENDING;
			}

			if(blocking){
				/* Set non-blocking */
				if(_mosquitto_socket_nonblock(*sock)){
					COMPAT_CLOSE(*sock);
					continue;
				}
			}
			break;
		}

		COMPAT_CLOSE(*sock);
		*sock = INVALID_SOCKET;
	}
	freeaddrinfo(ainfo);
	if(bind_address){
		freeaddrinfo(ainfo_bind);
	}
	if(!rp){
		return MOSQ_ERR_ERRNO;
	}
	return rc;
}

#ifdef WITH_TLS
int mosquitto__socket_connect_tls(struct mosquitto *mosq)
{
	int ret;

	ret = SSL_connect(mosq->ssl);
	if(ret != 1){
		ret = SSL_get_error(mosq->ssl, ret);
		if(ret == SSL_ERROR_WANT_READ){
			mosq->want_connect = true;
			/* We always try to read anyway */
		}else if(ret == SSL_ERROR_WANT_WRITE){
			mosq->want_write = true;
			mosq->want_connect = true;
		}else{
			COMPAT_CLOSE(mosq->sock);
			mosq->sock = INVALID_SOCKET;
			return MOSQ_ERR_TLS;
		}
	}else{
		mosq->want_connect = false;
	}
	return MOSQ_ERR_SUCCESS;
}
#endif

/* Create a socket and connect it to 'ip' on port 'port'.
 * Returns -1 on failure (ip is NULL, socket creation/connection error)
 * Returns sock number on success.
 */
int _mosquitto_socket_connect(struct mosquitto *mosq, const char *host, uint16_t port, const char *bind_address, bool blocking)
{
	mosq_sock_t sock = INVALID_SOCKET;
	int rc;
#ifdef WITH_TLS
	int ret;
	BIO *bio;
#endif

	if(!mosq || !host || !port) return MOSQ_ERR_INVAL;

	rc = _mosquitto_try_connect(mosq, host, port, &sock, bind_address, blocking);
	if(rc > 0) return rc;

#ifdef WITH_TLS
	if(mosq->tls_cafile || mosq->tls_capath || mosq->tls_psk){
#if OPENSSL_VERSION_NUMBER >= 0x10001000L
		if(!mosq->tls_version || !strcmp(mosq->tls_version, "tlsv1.2")){
			mosq->ssl_ctx = SSL_CTX_new(TLSv1_2_client_method());
		}else if(!strcmp(mosq->tls_version, "tlsv1.1")){
			mosq->ssl_ctx = SSL_CTX_new(TLSv1_1_client_method());
		}else if(!strcmp(mosq->tls_version, "tlsv1")){
			mosq->ssl_ctx = SSL_CTX_new(TLSv1_client_method());
		}else{
			_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Protocol %s not supported.", mosq->tls_version);
			COMPAT_CLOSE(sock);
			return MOSQ_ERR_INVAL;
		}
#else
		if(!mosq->tls_version || !strcmp(mosq->tls_version, "tlsv1")){
			mosq->ssl_ctx = SSL_CTX_new(TLSv1_client_method());
		}else{
			_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Protocol %s not supported.", mosq->tls_version);
			COMPAT_CLOSE(sock);
			return MOSQ_ERR_INVAL;
		}
#endif
		if(!mosq->ssl_ctx){
			_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to create TLS context.");
			COMPAT_CLOSE(sock);
			return MOSQ_ERR_TLS;
		}

#if OPENSSL_VERSION_NUMBER >= 0x10000000
		/* Disable compression */
		SSL_CTX_set_options(mosq->ssl_ctx, SSL_OP_NO_COMPRESSION);
#endif
#ifdef SSL_MODE_RELEASE_BUFFERS
			/* Use even less memory per SSL connection. */
			SSL_CTX_set_mode(mosq->ssl_ctx, SSL_MODE_RELEASE_BUFFERS);
#endif

		if(mosq->tls_ciphers){
			ret = SSL_CTX_set_cipher_list(mosq->ssl_ctx, mosq->tls_ciphers);
			if(ret == 0){
				_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to set TLS ciphers. Check cipher list \"%s\".", mosq->tls_ciphers);
				COMPAT_CLOSE(sock);
				return MOSQ_ERR_TLS;
			}
		}
		if(mosq->tls_cafile || mosq->tls_capath){
			ret = SSL_CTX_load_verify_locations(mosq->ssl_ctx, mosq->tls_cafile, mosq->tls_capath);
			if(ret == 0){
#ifdef WITH_BROKER
				if(mosq->tls_cafile && mosq->tls_capath){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check bridge_cafile \"%s\" and bridge_capath \"%s\".", mosq->tls_cafile, mosq->tls_capath);
				}else if(mosq->tls_cafile){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check bridge_cafile \"%s\".", mosq->tls_cafile);
				}else{
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check bridge_capath \"%s\".", mosq->tls_capath);
				}
#else
				if(mosq->tls_cafile && mosq->tls_capath){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\" and capath \"%s\".", mosq->tls_cafile, mosq->tls_capath);
				}else if(mosq->tls_cafile){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check cafile \"%s\".", mosq->tls_cafile);
				}else{
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load CA certificates, check capath \"%s\".", mosq->tls_capath);
				}
#endif
				COMPAT_CLOSE(sock);
				return MOSQ_ERR_TLS;
			}
			if(mosq->tls_cert_reqs == 0){
				SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_NONE, NULL);
			}else{
				SSL_CTX_set_verify(mosq->ssl_ctx, SSL_VERIFY_PEER, _mosquitto_server_certificate_verify);
			}

			if(mosq->tls_pw_callback){
				SSL_CTX_set_default_passwd_cb(mosq->ssl_ctx, mosq->tls_pw_callback);
				SSL_CTX_set_default_passwd_cb_userdata(mosq->ssl_ctx, mosq);
			}

			if(mosq->tls_certfile){
				ret = SSL_CTX_use_certificate_chain_file(mosq->ssl_ctx, mosq->tls_certfile);
				if(ret != 1){
#ifdef WITH_BROKER
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client certificate, check bridge_certfile \"%s\".", mosq->tls_certfile);
#else
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client certificate \"%s\".", mosq->tls_certfile);
#endif
					COMPAT_CLOSE(sock);
					return MOSQ_ERR_TLS;
				}
			}
			if(mosq->tls_keyfile){
				ret = SSL_CTX_use_PrivateKey_file(mosq->ssl_ctx, mosq->tls_keyfile, SSL_FILETYPE_PEM);
				if(ret != 1){
#ifdef WITH_BROKER
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client key file, check bridge_keyfile \"%s\".", mosq->tls_keyfile);
#else
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unable to load client key file \"%s\".", mosq->tls_keyfile);
#endif
					COMPAT_CLOSE(sock);
					return MOSQ_ERR_TLS;
				}
				ret = SSL_CTX_check_private_key(mosq->ssl_ctx);
				if(ret != 1){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Client certificate/key are inconsistent.");
					COMPAT_CLOSE(sock);
					return MOSQ_ERR_TLS;
				}
			}
#ifdef REAL_WITH_TLS_PSK
		}else if(mosq->tls_psk){
			SSL_CTX_set_psk_client_callback(mosq->ssl_ctx, psk_client_callback);
#endif
		}

		mosq->ssl = SSL_new(mosq->ssl_ctx);
		if(!mosq->ssl){
			COMPAT_CLOSE(sock);
			return MOSQ_ERR_TLS;
		}
		SSL_set_ex_data(mosq->ssl, tls_ex_index_mosq, mosq);
		bio = BIO_new_socket(sock, BIO_NOCLOSE);
		if(!bio){
			COMPAT_CLOSE(sock);
			return MOSQ_ERR_TLS;
		}
		SSL_set_bio(mosq->ssl, bio, bio);

		mosq->sock = sock;
		if(mosquitto__socket_connect_tls(mosq)){
			return MOSQ_ERR_TLS;
		}

	}
#endif

	mosq->sock = sock;

	return rc;
}

int _mosquitto_read_byte(struct _mosquitto_packet *packet, uint8_t *byte)
{
	assert(packet);
	if(packet->pos+1 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	*byte = packet->payload[packet->pos];
	packet->pos++;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_byte(struct _mosquitto_packet *packet, uint8_t byte)
{
	assert(packet);
	assert(packet->pos+1 <= packet->packet_length);

	packet->payload[packet->pos] = byte;
	packet->pos++;
}

int _mosquitto_read_bytes(struct _mosquitto_packet *packet, void *bytes, uint32_t count)
{
	assert(packet);
	if(packet->pos+count > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	memcpy(bytes, &(packet->payload[packet->pos]), count);
	packet->pos += count;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_bytes(struct _mosquitto_packet *packet, const void *bytes, uint32_t count)
{
	assert(packet);
	assert(packet->pos+count <= packet->packet_length);

	memcpy(&(packet->payload[packet->pos]), bytes, count);
	packet->pos += count;
}

int _mosquitto_read_string(struct _mosquitto_packet *packet, char **str)
{
	uint16_t len;
	int rc;

	assert(packet);
	rc = _mosquitto_read_uint16(packet, &len);
	if(rc) return rc;

	if(packet->pos+len > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	*str = _mosquitto_malloc(len+1);
	if(*str){
		memcpy(*str, &(packet->payload[packet->pos]), len);
		(*str)[len] = '\0';
		packet->pos += len;
	}else{
		return MOSQ_ERR_NOMEM;
	}

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_string(struct _mosquitto_packet *packet, const char *str, uint16_t length)
{
	assert(packet);
	_mosquitto_write_uint16(packet, length);
	_mosquitto_write_bytes(packet, str, length);
}

int _mosquitto_read_uint16(struct _mosquitto_packet *packet, uint16_t *word)
{
	uint8_t msb, lsb;

	assert(packet);
	if(packet->pos+2 > packet->remaining_length) return MOSQ_ERR_PROTOCOL;

	msb = packet->payload[packet->pos];
	packet->pos++;
	lsb = packet->payload[packet->pos];
	packet->pos++;

	*word = (msb<<8) + lsb;

	return MOSQ_ERR_SUCCESS;
}

void _mosquitto_write_uint16(struct _mosquitto_packet *packet, uint16_t word)
{
	_mosquitto_write_byte(packet, MOSQ_MSB(word));
	_mosquitto_write_byte(packet, MOSQ_LSB(word));
}

ssize_t _mosquitto_net_read(struct mosquitto *mosq, void *buf, size_t count)
{
#ifdef WITH_TLS
	int ret;
	int err;
	char ebuf[256];
	unsigned long e;
#endif
	assert(mosq);
	errno = 0;
#ifdef WITH_TLS
	if(mosq->ssl){
		ret = SSL_read(mosq->ssl, buf, count);
		if(ret <= 0){
			err = SSL_get_error(mosq->ssl, ret);
			if(err == SSL_ERROR_WANT_READ){
				ret = -1;
				errno = EAGAIN;
			}else if(err == SSL_ERROR_WANT_WRITE){
				ret = -1;
				mosq->want_write = true;
				errno = EAGAIN;
			}else{
				e = ERR_get_error();
				while(e){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "OpenSSL Error: %s", ERR_error_string(e, ebuf));
					e = ERR_get_error();
				}
				errno = EPROTO;
			}
		}
		return (ssize_t )ret;
	}else{
		/* Call normal read/recv */

#endif

#ifndef WIN32
	return read(mosq->sock, buf, count);
#else
	return recv(mosq->sock, buf, count, 0);
#endif

#ifdef WITH_TLS
	}
#endif
}

ssize_t _mosquitto_net_write(struct mosquitto *mosq, void *buf, size_t count)
{
#ifdef WITH_TLS
	int ret;
	int err;
	char ebuf[256];
	unsigned long e;
#endif
	assert(mosq);

	errno = 0;
#ifdef WITH_TLS
	if(mosq->ssl){
		mosq->want_write = false;
		ret = SSL_write(mosq->ssl, buf, count);
		if(ret < 0){
			err = SSL_get_error(mosq->ssl, ret);
			if(err == SSL_ERROR_WANT_READ){
				ret = -1;
				errno = EAGAIN;
			}else if(err == SSL_ERROR_WANT_WRITE){
				ret = -1;
				mosq->want_write = true;
				errno = EAGAIN;
			}else{
				e = ERR_get_error();
				while(e){
					_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "OpenSSL Error: %s", ERR_error_string(e, ebuf));
					e = ERR_get_error();
				}
				errno = EPROTO;
			}
		}
		return (ssize_t )ret;
	}else{
		/* Call normal write/send */
#endif

#ifndef WIN32
	return write(mosq->sock, buf, count);
#else
	return send(mosq->sock, buf, count, 0);
#endif

#ifdef WITH_TLS
	}
#endif
}

int _mosquitto_packet_write(struct mosquitto *mosq)
{
	ssize_t write_length;
	struct _mosquitto_packet *packet;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;

	pthread_mutex_lock(&mosq->current_out_packet_mutex);
	pthread_mutex_lock(&mosq->out_packet_mutex);
	if(mosq->out_packet && !mosq->current_out_packet){
		mosq->current_out_packet = mosq->out_packet;
		mosq->out_packet = mosq->out_packet->next;
		if(!mosq->out_packet){
			mosq->out_packet_last = NULL;
		}
	}
	pthread_mutex_unlock(&mosq->out_packet_mutex);

	if(mosq->state == mosq_cs_connect_pending){
		pthread_mutex_unlock(&mosq->current_out_packet_mutex);
		return MOSQ_ERR_SUCCESS;
	}

	while(mosq->current_out_packet){
		packet = mosq->current_out_packet;

		while(packet->to_process > 0){
			write_length = _mosquitto_net_write(mosq, &(packet->payload[packet->pos]), packet->to_process);
			if(write_length > 0){
#if defined(WITH_BROKER) && defined(WITH_SYS_TREE)
				g_bytes_sent += write_length;
#endif
				packet->to_process -= write_length;
				packet->pos += write_length;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
					return MOSQ_ERR_SUCCESS;
				}else{
					pthread_mutex_unlock(&mosq->current_out_packet_mutex);
					switch(errno){
						case COMPAT_ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}

#ifdef WITH_BROKER
#  ifdef WITH_SYS_TREE
		g_msgs_sent++;
		if(((packet->command)&0xF6) == PUBLISH){
			g_pub_msgs_sent++;
		}
#  endif
#else
		if(((packet->command)&0xF6) == PUBLISH){
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_publish){
				/* This is a QoS=0 message */
				mosq->in_callback = true;
				mosq->on_publish(mosq, mosq->userdata, packet->mid);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
		}else if(((packet->command)&0xF0) == DISCONNECT){
			/* FIXME what cleanup needs doing here? 
			 * incoming/outgoing messages? */
			_mosquitto_socket_close(mosq);

			/* Start of duplicate, possibly unnecessary code.
			 * This does leave things in a consistent state at least. */
			/* Free data and reset values */
			pthread_mutex_lock(&mosq->out_packet_mutex);
			mosq->current_out_packet = mosq->out_packet;
			if(mosq->out_packet){
				mosq->out_packet = mosq->out_packet->next;
				if(!mosq->out_packet){
					mosq->out_packet_last = NULL;
				}
			}
			pthread_mutex_unlock(&mosq->out_packet_mutex);

			_mosquitto_packet_cleanup(packet);
			_mosquitto_free(packet);

			pthread_mutex_lock(&mosq->msgtime_mutex);
			mosq->last_msg_out = mosquitto_time();
			pthread_mutex_unlock(&mosq->msgtime_mutex);
			/* End of duplicate, possibly unnecessary code */

			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_disconnect){
				mosq->in_callback = true;
				mosq->on_disconnect(mosq, mosq->userdata, 0);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
			pthread_mutex_unlock(&mosq->current_out_packet_mutex);
			return MOSQ_ERR_SUCCESS;
		}
#endif

		/* Free data and reset values */
		pthread_mutex_lock(&mosq->out_packet_mutex);
		mosq->current_out_packet = mosq->out_packet;
		if(mosq->out_packet){
			mosq->out_packet = mosq->out_packet->next;
			if(!mosq->out_packet){
				mosq->out_packet_last = NULL;
			}
		}
		pthread_mutex_unlock(&mosq->out_packet_mutex);

		_mosquitto_packet_cleanup(packet);
		_mosquitto_free(packet);

		pthread_mutex_lock(&mosq->msgtime_mutex);
		mosq->last_msg_out = mosquitto_time();
		pthread_mutex_unlock(&mosq->msgtime_mutex);
	}
	pthread_mutex_unlock(&mosq->current_out_packet_mutex);
	return MOSQ_ERR_SUCCESS;
}

#ifdef WITH_BROKER
int _mosquitto_packet_read(struct mosquitto_db *db, struct mosquitto *mosq)
#else
int _mosquitto_packet_read(struct mosquitto *mosq)
#endif
{
	uint8_t byte;
	ssize_t read_length;
	int rc = 0;

	if(!mosq) return MOSQ_ERR_INVAL;
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;
	if(mosq->state == mosq_cs_connect_pending){
		return MOSQ_ERR_SUCCESS;
	}

	/* This gets called if pselect() indicates that there is network data
	 * available - ie. at least one byte.  What we do depends on what data we
	 * already have.
	 * If we've not got a command, attempt to read one and save it. This should
	 * always work because it's only a single byte.
	 * Then try to read the remaining length. This may fail because it is may
	 * be more than one byte - will need to save data pending next read if it
	 * does fail.
	 * Then try to read the remaining payload, where 'payload' here means the
	 * combined variable header and actual payload. This is the most likely to
	 * fail due to longer length, so save current data and current position.
	 * After all data is read, send to _mosquitto_handle_packet() to deal with.
	 * Finally, free the memory and reset everything to starting conditions.
	 */
	if(!mosq->in_packet.command){
		read_length = _mosquitto_net_read(mosq, &byte, 1);
		if(read_length == 1){
			mosq->in_packet.command = byte;
#ifdef WITH_BROKER
#  ifdef WITH_SYS_TREE
			g_bytes_received++;
#  endif
			/* Clients must send CONNECT as their first command. */
			if(!(mosq->bridge) && mosq->state == mosq_cs_new && (byte&0xF0) != CONNECT) return MOSQ_ERR_PROTOCOL;
#endif
		}else{
			if(read_length == 0) return MOSQ_ERR_CONN_LOST; /* EOF */
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
				return MOSQ_ERR_SUCCESS;
			}else{
				switch(errno){
					case COMPAT_ECONNRESET:
						return MOSQ_ERR_CONN_LOST;
					default:
						return MOSQ_ERR_ERRNO;
				}
			}
		}
	}
	/* remaining_count is the number of bytes that the remaining_length
	 * parameter occupied in this incoming packet. We don't use it here as such
	 * (it is used when allocating an outgoing packet), but we must be able to
	 * determine whether all of the remaining_length parameter has been read.
	 * remaining_count has three states here:
	 *   0 means that we haven't read any remaining_length bytes
	 *   <0 means we have read some remaining_length bytes but haven't finished
	 *   >0 means we have finished reading the remaining_length bytes.
	 */
	if(mosq->in_packet.remaining_count <= 0){
		do{
			read_length = _mosquitto_net_read(mosq, &byte, 1);
			if(read_length == 1){
				mosq->in_packet.remaining_count--;
				/* Max 4 bytes length for remaining length as defined by protocol.
				 * Anything more likely means a broken/malicious client.
				 */
				if(mosq->in_packet.remaining_count < -4) return MOSQ_ERR_PROTOCOL;

#if defined(WITH_BROKER) && defined(WITH_SYS_TREE)
				g_bytes_received++;
#endif
				mosq->in_packet.remaining_length += (byte & 127) * mosq->in_packet.remaining_mult;
				mosq->in_packet.remaining_mult *= 128;
			}else{
				if(read_length == 0) return MOSQ_ERR_CONN_LOST; /* EOF */
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return MOSQ_ERR_SUCCESS;
				}else{
					switch(errno){
						case COMPAT_ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}while((byte & 128) != 0);
		/* We have finished reading remaining_length, so make remaining_count
		 * positive. */
		mosq->in_packet.remaining_count *= -1;

		if(mosq->in_packet.remaining_length > 0){
			mosq->in_packet.payload = _mosquitto_malloc(mosq->in_packet.remaining_length*sizeof(uint8_t));
			if(!mosq->in_packet.payload) return MOSQ_ERR_NOMEM;
			mosq->in_packet.to_process = mosq->in_packet.remaining_length;
		}
	}
	while(mosq->in_packet.to_process>0){
		read_length = _mosquitto_net_read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
		if(read_length > 0){
#if defined(WITH_BROKER) && defined(WITH_SYS_TREE)
			g_bytes_received += read_length;
#endif
			mosq->in_packet.to_process -= read_length;
			mosq->in_packet.pos += read_length;
		}else{
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
				if(mosq->in_packet.to_process > 1000){
					/* Update last_msg_in time if more than 1000 bytes left to
					 * receive. Helps when receiving large messages.
					 * This is an arbitrary limit, but with some consideration.
					 * If a client can't send 1000 bytes in a second it
					 * probably shouldn't be using a 1 second keep alive. */
					pthread_mutex_lock(&mosq->msgtime_mutex);
					mosq->last_msg_in = mosquitto_time();
					pthread_mutex_unlock(&mosq->msgtime_mutex);
				}
				return MOSQ_ERR_SUCCESS;
			}else{
				switch(errno){
					case COMPAT_ECONNRESET:
						return MOSQ_ERR_CONN_LOST;
					default:
						return MOSQ_ERR_ERRNO;
				}
			}
		}
	}

	/* All data for this packet is read. */
	mosq->in_packet.pos = 0;
#ifdef WITH_BROKER
#  ifdef WITH_SYS_TREE
	g_msgs_received++;
	if(((mosq->in_packet.command)&0xF5) == PUBLISH){
		g_pub_msgs_received++;
	}
#  endif
	rc = mqtt3_packet_handle(db, mosq);
#else
	rc = _mosquitto_packet_handle(mosq);
#endif

	/* Free data and reset values */
	_mosquitto_packet_cleanup(&mosq->in_packet);

	pthread_mutex_lock(&mosq->msgtime_mutex);
	mosq->last_msg_in = mosquitto_time();
	pthread_mutex_unlock(&mosq->msgtime_mutex);
	return rc;
}

int _mosquitto_socket_nonblock(mosq_sock_t sock)
{
#ifndef WIN32
	int opt;
	/* Set non-blocking */
	opt = fcntl(sock, F_GETFL, 0);
	if(opt == -1){
		COMPAT_CLOSE(sock);
		return 1;
	}
	if(fcntl(sock, F_SETFL, opt | O_NONBLOCK) == -1){
		/* If either fcntl fails, don't want to allow this client to connect. */
		COMPAT_CLOSE(sock);
		return 1;
	}
#else
	unsigned long opt = 1;
	if(ioctlsocket(sock, FIONBIO, &opt)){
		COMPAT_CLOSE(sock);
		return 1;
	}
#endif
	return 0;
}


#ifndef WITH_BROKER
int _mosquitto_socketpair(mosq_sock_t *pairR, mosq_sock_t *pairW)
{
#ifdef WIN32
	int family[2] = {AF_INET, AF_INET6};
	int i;
	struct sockaddr_storage ss;
	struct sockaddr_in *sa = (struct sockaddr_in *)&ss;
	struct sockaddr_in6 *sa6 = (struct sockaddr_in6 *)&ss;
	socklen_t ss_len;
	mosq_sock_t spR, spW;

	mosq_sock_t listensock;

	*pairR = INVALID_SOCKET;
	*pairW = INVALID_SOCKET;

	for(i=0; i<2; i++){
		memset(&ss, 0, sizeof(ss));
		if(family[i] == AF_INET){
			sa->sin_family = family[i];
			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			sa->sin_port = 0;
			ss_len = sizeof(struct sockaddr_in);
		}else if(family[i] == AF_INET6){
			sa6->sin6_family = family[i];
			sa6->sin6_addr = in6addr_loopback;
			sa6->sin6_port = 0;
			ss_len = sizeof(struct sockaddr_in6);
		}else{
			return MOSQ_ERR_INVAL;
		}

		listensock = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
		if(listensock == -1){
			continue;
		}

		if(bind(listensock, (struct sockaddr *)&ss, ss_len) == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}

		if(listen(listensock, 1) == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}
		memset(&ss, 0, sizeof(ss));
		ss_len = sizeof(ss);
		if(getsockname(listensock, (struct sockaddr *)&ss, &ss_len) < 0){
			COMPAT_CLOSE(listensock);
			continue;
		}

		if(family[i] == AF_INET){
			sa->sin_family = family[i];
			sa->sin_addr.s_addr = htonl(INADDR_LOOPBACK);
			ss_len = sizeof(struct sockaddr_in);
		}else if(family[i] == AF_INET6){
			sa6->sin6_family = family[i];
			sa6->sin6_addr = in6addr_loopback;
			ss_len = sizeof(struct sockaddr_in6);
		}

		spR = socket(family[i], SOCK_STREAM, IPPROTO_TCP);
		if(spR == -1){
			COMPAT_CLOSE(listensock);
			continue;
		}
		if(_mosquitto_socket_nonblock(spR)){
			COMPAT_CLOSE(spR);
			COMPAT_CLOSE(listensock);
			continue;
		}
		if(connect(spR, (struct sockaddr *)&ss, ss_len) < 0){
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno != EINPROGRESS && errno != COMPAT_EWOULDBLOCK){
				COMPAT_CLOSE(spR);
				COMPAT_CLOSE(listensock);
				continue;
			}
		}
		spW = accept(listensock, NULL, 0);
		if(spW == -1){
#ifdef WIN32
			errno = WSAGetLastError();
#endif
			if(errno != EINPROGRESS && errno != COMPAT_EWOULDBLOCK){
				COMPAT_CLOSE(spR);
				COMPAT_CLOSE(listensock);
				continue;
			}
		}

		if(_mosquitto_socket_nonblock(spW)){
			COMPAT_CLOSE(spR);
			COMPAT_CLOSE(spW);
			COMPAT_CLOSE(listensock);
			continue;
		}
		COMPAT_CLOSE(listensock);

		*pairR = spR;
		*pairW = spW;
		return MOSQ_ERR_SUCCESS;
	}
	return MOSQ_ERR_UNKNOWN;
#else
	int sv[2];

	if(socketpair(AF_UNIX, SOCK_STREAM, 0, sv) == -1){
		return MOSQ_ERR_ERRNO;
	}
	if(_mosquitto_socket_nonblock(sv[0])){
		COMPAT_CLOSE(sv[0]);
		COMPAT_CLOSE(sv[1]);
		return MOSQ_ERR_ERRNO;
	}
	if(_mosquitto_socket_nonblock(sv[1])){
		COMPAT_CLOSE(sv[0]);
		COMPAT_CLOSE(sv[1]);
		return MOSQ_ERR_ERRNO;
	}
	*pairR = sv[0];
	*pairW = sv[1];
	return MOSQ_ERR_SUCCESS;
#endif
}
#endif
/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <mosquitto.h>
#include <logging_mosq.h>
#include <memory_mosq.h>
#include <messages_mosq.h>
#include <mqtt3_protocol.h>
#include <net_mosq.h>
#include <read_handle.h>
#include <send_mosq.h>
#include <time_mosq.h>
#include <util_mosq.h>

int _mosquitto_packet_handle(struct mosquitto *mosq)
{
	assert(mosq);

	switch((mosq->in_packet.command)&0xF0){
		case PINGREQ:
			return _mosquitto_handle_pingreq(mosq);
		case PINGRESP:
			return _mosquitto_handle_pingresp(mosq);
		case PUBACK:
			return _mosquitto_handle_pubackcomp(mosq, "PUBACK");
		case PUBCOMP:
			return _mosquitto_handle_pubackcomp(mosq, "PUBCOMP");
		case PUBLISH:
			return _mosquitto_handle_publish(mosq);
		case PUBREC:
			return _mosquitto_handle_pubrec(mosq);
		case PUBREL:
			return _mosquitto_handle_pubrel(NULL, mosq);
		case CONNACK:
			return _mosquitto_handle_connack(mosq);
		case SUBACK:
			return _mosquitto_handle_suback(mosq);
		case UNSUBACK:
			return _mosquitto_handle_unsuback(mosq);
		default:
			/* If we don't recognise the command, return an error straight away. */
			_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: Unrecognised command %d\n", (mosq->in_packet.command)&0xF0);
			return MOSQ_ERR_PROTOCOL;
	}
}

int _mosquitto_handle_publish(struct mosquitto *mosq)
{
	uint8_t header;
	struct mosquitto_message_all *message;
	int rc = 0;
	uint16_t mid;

	assert(mosq);

	message = _mosquitto_calloc(1, sizeof(struct mosquitto_message_all));
	if(!message) return MOSQ_ERR_NOMEM;

	header = mosq->in_packet.command;

	message->dup = (header & 0x08)>>3;
	message->msg.qos = (header & 0x06)>>1;
	message->msg.retain = (header & 0x01);

	rc = _mosquitto_read_string(&mosq->in_packet, &message->msg.topic);
	if(rc){
		_mosquitto_message_cleanup(&message);
		return rc;
	}
	if(!strlen(message->msg.topic)){
		_mosquitto_message_cleanup(&message);
		return MOSQ_ERR_PROTOCOL;
	}

	if(message->msg.qos > 0){
		rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
		if(rc){
			_mosquitto_message_cleanup(&message);
			return rc;
		}
		message->msg.mid = (int)mid;
	}

	message->msg.payloadlen = mosq->in_packet.remaining_length - mosq->in_packet.pos;
	if(message->msg.payloadlen){
		message->msg.payload = _mosquitto_calloc(message->msg.payloadlen+1, sizeof(uint8_t));
		if(!message->msg.payload){
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_NOMEM;
		}
		rc = _mosquitto_read_bytes(&mosq->in_packet, message->msg.payload, message->msg.payloadlen);
		if(rc){
			_mosquitto_message_cleanup(&message);
			return rc;
		}
	}
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG,
			"Client %s received PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))",
			mosq->id, message->dup, message->msg.qos, message->msg.retain,
			message->msg.mid, message->msg.topic,
			(long)message->msg.payloadlen);

	message->timestamp = mosquitto_time();
	switch(message->msg.qos){
		case 0:
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_message){
				mosq->in_callback = true;
				mosq->on_message(mosq, mosq->userdata, &message->msg);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_SUCCESS;
		case 1:
			rc = _mosquitto_send_puback(mosq, message->msg.mid);
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_message){
				mosq->in_callback = true;
				mosq->on_message(mosq, mosq->userdata, &message->msg);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
			_mosquitto_message_cleanup(&message);
			return rc;
		case 2:
			rc = _mosquitto_send_pubrec(mosq, message->msg.mid);
			pthread_mutex_lock(&mosq->in_message_mutex);
			message->state = mosq_ms_wait_for_pubrel;
			_mosquitto_message_queue(mosq, message, mosq_md_in);
			pthread_mutex_unlock(&mosq->in_message_mutex);
			return rc;
		default:
			_mosquitto_message_cleanup(&message);
			return MOSQ_ERR_PROTOCOL;
	}
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>

#include <mosquitto.h>
#include <logging_mosq.h>
#include <memory_mosq.h>
#include <net_mosq.h>
#include <read_handle.h>

int _mosquitto_handle_connack(struct mosquitto *mosq)
{
	uint8_t byte;
	uint8_t result;
	int rc;

	assert(mosq);
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received CONNACK", mosq->id);
	rc = _mosquitto_read_byte(&mosq->in_packet, &byte); // Reserved byte, not used
	if(rc) return rc;
	rc = _mosquitto_read_byte(&mosq->in_packet, &result);
	if(rc) return rc;
	pthread_mutex_lock(&mosq->callback_mutex);
	if(mosq->on_connect){
		mosq->in_callback = true;
		mosq->on_connect(mosq, mosq->userdata, result);
		mosq->in_callback = false;
	}
	pthread_mutex_unlock(&mosq->callback_mutex);
	switch(result){
		case 0:
			if(mosq->state != mosq_cs_disconnecting){
				mosq->state = mosq_cs_connected;
			}
			return MOSQ_ERR_SUCCESS;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			return MOSQ_ERR_CONN_REFUSED;
		default:
			return MOSQ_ERR_PROTOCOL;
	}
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <mosquitto.h>
#include <logging_mosq.h>
#include <memory_mosq.h>
#include <messages_mosq.h>
#include <mqtt3_protocol.h>
#include <net_mosq.h>
#include <read_handle.h>
#include <send_mosq.h>
#include <util_mosq.h>
#ifdef WITH_BROKER
#include <mosquitto_broker.h>
#endif

int _mosquitto_handle_pingreq(struct mosquitto *mosq)
{
	assert(mosq);
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received PINGREQ from %s", mosq->id);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGREQ", mosq->id);
#endif
	return _mosquitto_send_pingresp(mosq);
}

int _mosquitto_handle_pingresp(struct mosquitto *mosq)
{
	assert(mosq);
	mosq->ping_t = 0; /* No longer waiting for a PINGRESP. */
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received PINGRESP from %s", mosq->id);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PINGRESP", mosq->id);
#endif
	return MOSQ_ERR_SUCCESS;
}

#ifdef WITH_BROKER
int _mosquitto_handle_pubackcomp(struct mosquitto_db *db, struct mosquitto *mosq, const char *type)
#else
int _mosquitto_handle_pubackcomp(struct mosquitto *mosq, const char *type)
#endif
{
	uint16_t mid;
	int rc;

	assert(mosq);
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received %s from %s (Mid: %d)", type, mosq->id, mid);

	if(mid){
		rc = mqtt3_db_message_delete(db, mosq, mid, mosq_md_out);
		if(rc) return rc;
	}
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received %s (Mid: %d)", mosq->id, type, mid);

	if(!_mosquitto_message_delete(mosq, mid, mosq_md_out)){
		/* Only inform the client the message has been sent once. */
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_publish){
			mosq->in_callback = true;
			mosq->on_publish(mosq, mosq->userdata, mid);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
	}
#endif

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_pubrec(struct mosquitto *mosq)
{
	uint16_t mid;
	int rc;

	assert(mosq);
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received PUBREC from %s (Mid: %d)", mosq->id, mid);

	rc = mqtt3_db_message_update(mosq, mid, mosq_md_out, mosq_ms_wait_for_pubcomp);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREC (Mid: %d)", mosq->id, mid);

	rc = _mosquitto_message_out_update(mosq, mid, mosq_ms_wait_for_pubcomp);
#endif
	if(rc) return rc;
	rc = _mosquitto_send_pubrel(mosq, mid);
	if(rc) return rc;

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_pubrel(struct mosquitto_db *db, struct mosquitto *mosq)
{
	uint16_t mid;
#ifndef WITH_BROKER
	struct mosquitto_message_all *message = NULL;
#endif
	int rc;

	assert(mosq);
	if(mosq->protocol == mosq_p_mqtt311){
		if((mosq->in_packet.command&0x0F) != 0x02){
			return MOSQ_ERR_PROTOCOL;
		}
	}
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received PUBREL from %s (Mid: %d)", mosq->id, mid);

	if(mqtt3_db_message_release(db, mosq, mid, mosq_md_in)){
		/* Message not found. Still send a PUBCOMP anyway because this could be
		 * due to a repeated PUBREL after a client has reconnected. */
	}
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received PUBREL (Mid: %d)", mosq->id, mid);

	if(!_mosquitto_message_remove(mosq, mid, mosq_md_in, &message)){
		/* Only pass the message on if we have removed it from the queue - this
		 * prevents multiple callbacks for the same message. */
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_message){
			mosq->in_callback = true;
			mosq->on_message(mosq, mosq->userdata, &message->msg);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
		_mosquitto_message_cleanup(&message);
	}
#endif
	rc = _mosquitto_send_pubcomp(mosq, mid);
	if(rc) return rc;

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_suback(struct mosquitto *mosq)
{
	uint16_t mid;
	uint8_t qos;
	int *granted_qos;
	int qos_count;
	int i = 0;
	int rc;

	assert(mosq);
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received SUBACK from %s", mosq->id);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received SUBACK", mosq->id);
#endif
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;

	qos_count = mosq->in_packet.remaining_length - mosq->in_packet.pos;
	granted_qos = _mosquitto_malloc(qos_count*sizeof(int));
	if(!granted_qos) return MOSQ_ERR_NOMEM;
	while(mosq->in_packet.pos < mosq->in_packet.remaining_length){
		rc = _mosquitto_read_byte(&mosq->in_packet, &qos);
		if(rc){
			_mosquitto_free(granted_qos);
			return rc;
		}
		granted_qos[i] = (int)qos;
		i++;
	}
#ifndef WITH_BROKER
	pthread_mutex_lock(&mosq->callback_mutex);
	if(mosq->on_subscribe){
		mosq->in_callback = true;
		mosq->on_subscribe(mosq, mosq->userdata, mid, qos_count, granted_qos);
		mosq->in_callback = false;
	}
	pthread_mutex_unlock(&mosq->callback_mutex);
#endif
	_mosquitto_free(granted_qos);

	return MOSQ_ERR_SUCCESS;
}

int _mosquitto_handle_unsuback(struct mosquitto *mosq)
{
	uint16_t mid;
	int rc;

	assert(mosq);
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Received UNSUBACK from %s", mosq->id);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s received UNSUBACK", mosq->id);
#endif
	rc = _mosquitto_read_uint16(&mosq->in_packet, &mid);
	if(rc) return rc;
#ifndef WITH_BROKER
	pthread_mutex_lock(&mosq->callback_mutex);
	if(mosq->on_unsubscribe){
		mosq->in_callback = true;
	   	mosq->on_unsubscribe(mosq, mosq->userdata, mid);
		mosq->in_callback = false;
	}
	pthread_mutex_unlock(&mosq->callback_mutex);
#endif

	return MOSQ_ERR_SUCCESS;
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <string.h>

#include <mosquitto.h>
#include <logging_mosq.h>
#include <memory_mosq.h>
#include <mqtt3_protocol.h>
#include <net_mosq.h>
#include <send_mosq.h>
#include <util_mosq.h>

#ifdef WITH_BROKER
#include <mosquitto_broker.h>
#endif

int _mosquitto_send_connect(struct mosquitto *mosq, uint16_t keepalive, bool clean_session)
{
	struct _mosquitto_packet *packet = NULL;
	int payloadlen;
	uint8_t will = 0;
	uint8_t byte;
	int rc;
	uint8_t version;
	char *clientid, *username, *password;
	int headerlen;

	assert(mosq);
	assert(mosq->id);

#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	if(mosq->bridge){
		clientid = mosq->bridge->remote_clientid;
		username = mosq->bridge->remote_username;
		password = mosq->bridge->remote_password;
	}else{
		clientid = mosq->id;
		username = mosq->username;
		password = mosq->password;
	}
#else
	clientid = mosq->id;
	username = mosq->username;
	password = mosq->password;
#endif

	if(mosq->protocol == mosq_p_mqtt31){
		version = MQTT_PROTOCOL_V31;
		headerlen = 12;
	}else if(mosq->protocol == mosq_p_mqtt311){
		version = MQTT_PROTOCOL_V311;
		headerlen = 10;
	}else{
		return MOSQ_ERR_INVAL;
	}

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	payloadlen = 2+strlen(clientid);
	if(mosq->will){
		will = 1;
		assert(mosq->will->topic);

		payloadlen += 2+strlen(mosq->will->topic) + 2+mosq->will->payloadlen;
	}
	if(username){
		payloadlen += 2+strlen(username);
		if(password){
			payloadlen += 2+strlen(password);
		}
	}

	packet->command = CONNECT;
	packet->remaining_length = headerlen+payloadlen;
	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	if(version == MQTT_PROTOCOL_V31){
		_mosquitto_write_string(packet, PROTOCOL_NAME_v31, strlen(PROTOCOL_NAME_v31));
	}else if(version == MQTT_PROTOCOL_V311){
		_mosquitto_write_string(packet, PROTOCOL_NAME_v311, strlen(PROTOCOL_NAME_v311));
	}
#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	if(mosq->bridge && mosq->bridge->try_private && mosq->bridge->try_private_accepted){
		version |= 0x80;
	}else{
	}
#endif
	_mosquitto_write_byte(packet, version);
	byte = (clean_session&0x1)<<1;
	if(will){
		byte = byte | ((mosq->will->retain&0x1)<<5) | ((mosq->will->qos&0x3)<<3) | ((will&0x1)<<2);
	}
	if(username){
		byte = byte | 0x1<<7;
		if(mosq->password){
			byte = byte | 0x1<<6;
		}
	}
	_mosquitto_write_byte(packet, byte);
	_mosquitto_write_uint16(packet, keepalive);

	/* Payload */
	_mosquitto_write_string(packet, clientid, strlen(clientid));
	if(will){
		_mosquitto_write_string(packet, mosq->will->topic, strlen(mosq->will->topic));
		_mosquitto_write_string(packet, (const char *)mosq->will->payload, mosq->will->payloadlen);
	}
	if(username){
		_mosquitto_write_string(packet, username, strlen(username));
		if(password){
			_mosquitto_write_string(packet, password, strlen(password));
		}
	}

	mosq->keepalive = keepalive;
#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending CONNECT", clientid);
# endif
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending CONNECT", clientid);
#endif
	return _mosquitto_packet_queue(mosq, packet);
}

int _mosquitto_send_disconnect(struct mosquitto *mosq)
{
	assert(mosq);
#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending DISCONNECT", mosq->id);
# endif
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending DISCONNECT", mosq->id);
#endif
	return _mosquitto_send_simple_command(mosq, DISCONNECT);
}

int _mosquitto_send_subscribe(struct mosquitto *mosq, int *mid, const char *topic, uint8_t topic_qos)
{
	/* FIXME - only deals with a single topic */
	struct _mosquitto_packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;

	assert(mosq);
	assert(topic);

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packetlen = 2 + 2+strlen(topic) + 1;

	packet->command = SUBSCRIBE | (1<<1);
	packet->remaining_length = packetlen;
	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = _mosquitto_mid_generate(mosq);
	if(mid) *mid = (int)local_mid;
	_mosquitto_write_uint16(packet, local_mid);

	/* Payload */
	_mosquitto_write_string(packet, topic, strlen(topic));
	_mosquitto_write_byte(packet, topic_qos);

#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d)", mosq->id, local_mid, topic, topic_qos);
# endif
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending SUBSCRIBE (Mid: %d, Topic: %s, QoS: %d)", mosq->id, local_mid, topic, topic_qos);
#endif

	return _mosquitto_packet_queue(mosq, packet);
}


int _mosquitto_send_unsubscribe(struct mosquitto *mosq, int *mid, const char *topic)
{
	/* FIXME - only deals with a single topic */
	struct _mosquitto_packet *packet = NULL;
	uint32_t packetlen;
	uint16_t local_mid;
	int rc;

	assert(mosq);
	assert(topic);

	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packetlen = 2 + 2+strlen(topic);

	packet->command = UNSUBSCRIBE | (1<<1);
	packet->remaining_length = packetlen;
	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}

	/* Variable header */
	local_mid = _mosquitto_mid_generate(mosq);
	if(mid) *mid = (int)local_mid;
	_mosquitto_write_uint16(packet, local_mid);

	/* Payload */
	_mosquitto_write_string(packet, topic, strlen(topic));

#ifdef WITH_BROKER
# ifdef WITH_BRIDGE
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Bridge %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)", mosq->id, local_mid, topic);
# endif
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending UNSUBSCRIBE (Mid: %d, Topic: %s)", mosq->id, local_mid, topic);
#endif
	return _mosquitto_packet_queue(mosq, packet);
}

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include <mosquitto.h>
#include <mosquitto_internal.h>
#include <logging_mosq.h>
#include <mqtt3_protocol.h>
#include <memory_mosq.h>
#include <net_mosq.h>
#include <send_mosq.h>
#include <time_mosq.h>
#include <util_mosq.h>

#ifdef WITH_BROKER
#include <mosquitto_broker.h>
#  ifdef WITH_SYS_TREE
extern uint64_t g_pub_bytes_sent;
#  endif
#endif

int _mosquitto_send_pingreq(struct mosquitto *mosq)
{
	int rc;
	assert(mosq);
#ifdef WITH_BROKER
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PINGREQ to %s", mosq->id);
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGREQ", mosq->id);
#endif
	rc = _mosquitto_send_simple_command(mosq, PINGREQ);
	if(rc == MOSQ_ERR_SUCCESS){
		mosq->ping_t = mosquitto_time();
	}
	return rc;
}

int _mosquitto_send_pingresp(struct mosquitto *mosq)
{
#ifdef WITH_BROKER
	if(mosq) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PINGRESP to %s", mosq->id);
#else
	if(mosq) _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PINGRESP", mosq->id);
#endif
	return _mosquitto_send_simple_command(mosq, PINGRESP);
}

int _mosquitto_send_puback(struct mosquitto *mosq, uint16_t mid)
{
#ifdef WITH_BROKER
	if(mosq) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBACK to %s (Mid: %d)", mosq->id, mid);
#else
	if(mosq) _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBACK (Mid: %d)", mosq->id, mid);
#endif
	return _mosquitto_send_command_with_mid(mosq, PUBACK, mid, false);
}

int _mosquitto_send_pubcomp(struct mosquitto *mosq, uint16_t mid)
{
#ifdef WITH_BROKER
	if(mosq) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBCOMP to %s (Mid: %d)", mosq->id, mid);
#else
	if(mosq) _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBCOMP (Mid: %d)", mosq->id, mid);
#endif
	return _mosquitto_send_command_with_mid(mosq, PUBCOMP, mid, false);
}

int _mosquitto_send_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup)
{
#ifdef WITH_BROKER
	size_t len;
#ifdef WITH_BRIDGE
	int i;
	struct _mqtt3_bridge_topic *cur_topic;
	bool match;
	int rc;
	char *mapped_topic = NULL;
	char *topic_temp = NULL;
#endif
#endif
	assert(mosq);
	assert(topic);

#if defined(WITH_BROKER) && defined(WITH_WEBSOCKETS)
	if(mosq->sock == INVALID_SOCKET && !mosq->wsi) return MOSQ_ERR_NO_CONN;
#else
	if(mosq->sock == INVALID_SOCKET) return MOSQ_ERR_NO_CONN;
#endif

#ifdef WITH_BROKER
	if(mosq->listener && mosq->listener->mount_point){
		len = strlen(mosq->listener->mount_point);
		if(len < strlen(topic)){
			topic += len;
		}else{
			/* Invalid topic string. Should never happen, but silently swallow the message anyway. */
			return MOSQ_ERR_SUCCESS;
		}
	}
#ifdef WITH_BRIDGE
	if(mosq->bridge && mosq->bridge->topics && mosq->bridge->topic_remapping){
		for(i=0; i<mosq->bridge->topic_count; i++){
			cur_topic = &mosq->bridge->topics[i];
			if((cur_topic->direction == bd_both || cur_topic->direction == bd_out) 
					&& (cur_topic->remote_prefix || cur_topic->local_prefix)){
				/* Topic mapping required on this topic if the message matches */

				rc = mosquitto_topic_matches_sub(cur_topic->local_topic, topic, &match);
				if(rc){
					return rc;
				}
				if(match){
					mapped_topic = _mosquitto_strdup(topic);
					if(!mapped_topic) return MOSQ_ERR_NOMEM;
					if(cur_topic->local_prefix){
						/* This prefix needs removing. */
						if(!strncmp(cur_topic->local_prefix, mapped_topic, strlen(cur_topic->local_prefix))){
							topic_temp = _mosquitto_strdup(mapped_topic+strlen(cur_topic->local_prefix));
							_mosquitto_free(mapped_topic);
							if(!topic_temp){
								return MOSQ_ERR_NOMEM;
							}
							mapped_topic = topic_temp;
						}
					}

					if(cur_topic->remote_prefix){
						/* This prefix needs adding. */
						len = strlen(mapped_topic) + strlen(cur_topic->remote_prefix)+1;
						topic_temp = _mosquitto_malloc(len+1);
						if(!topic_temp){
							_mosquitto_free(mapped_topic);
							return MOSQ_ERR_NOMEM;
						}
						snprintf(topic_temp, len, "%s%s", cur_topic->remote_prefix, mapped_topic);
						topic_temp[len] = '\0';
						_mosquitto_free(mapped_topic);
						mapped_topic = topic_temp;
					}
					_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBLISH to %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, mapped_topic, (long)payloadlen);
#ifdef WITH_SYS_TREE
					g_pub_bytes_sent += payloadlen;
#endif
					rc =  _mosquitto_send_real_publish(mosq, mid, mapped_topic, payloadlen, payload, qos, retain, dup);
					_mosquitto_free(mapped_topic);
					return rc;
				}
			}
		}
	}
#endif
	_mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBLISH to %s (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);
#  ifdef WITH_SYS_TREE
	g_pub_bytes_sent += payloadlen;
#  endif
#else
	_mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBLISH (d%d, q%d, r%d, m%d, '%s', ... (%ld bytes))", mosq->id, dup, qos, retain, mid, topic, (long)payloadlen);
#endif

	return _mosquitto_send_real_publish(mosq, mid, topic, payloadlen, payload, qos, retain, dup);
}

int _mosquitto_send_pubrec(struct mosquitto *mosq, uint16_t mid)
{
#ifdef WITH_BROKER
	if(mosq) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBREC to %s (Mid: %d)", mosq->id, mid);
#else
	if(mosq) _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREC (Mid: %d)", mosq->id, mid);
#endif
	return _mosquitto_send_command_with_mid(mosq, PUBREC, mid, false);
}

int _mosquitto_send_pubrel(struct mosquitto *mosq, uint16_t mid)
{
#ifdef WITH_BROKER
	if(mosq) _mosquitto_log_printf(NULL, MOSQ_LOG_DEBUG, "Sending PUBREL to %s (Mid: %d)", mosq->id, mid);
#else
	if(mosq) _mosquitto_log_printf(mosq, MOSQ_LOG_DEBUG, "Client %s sending PUBREL (Mid: %d)", mosq->id, mid);
#endif
	return _mosquitto_send_command_with_mid(mosq, PUBREL|2, mid, false);
}

/* For PUBACK, PUBCOMP, PUBREC, and PUBREL */
int _mosquitto_send_command_with_mid(struct mosquitto *mosq, uint8_t command, uint16_t mid, bool dup)
{
	struct _mosquitto_packet *packet = NULL;
	int rc;

	assert(mosq);
	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = command;
	if(dup){
		packet->command |= 8;
	}
	packet->remaining_length = 2;
	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}

	packet->payload[packet->pos+0] = MOSQ_MSB(mid);
	packet->payload[packet->pos+1] = MOSQ_LSB(mid);

	return _mosquitto_packet_queue(mosq, packet);
}

/* For DISCONNECT, PINGREQ and PINGRESP */
int _mosquitto_send_simple_command(struct mosquitto *mosq, uint8_t command)
{
	struct _mosquitto_packet *packet = NULL;
	int rc;

	assert(mosq);
	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->command = command;
	packet->remaining_length = 0;

	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}

	return _mosquitto_packet_queue(mosq, packet);
}

int _mosquitto_send_real_publish(struct mosquitto *mosq, uint16_t mid, const char *topic, uint32_t payloadlen, const void *payload, int qos, bool retain, bool dup)
{
	struct _mosquitto_packet *packet = NULL;
	int packetlen;
	int rc;

	assert(mosq);
	assert(topic);

	packetlen = 2+strlen(topic) + payloadlen;
	if(qos > 0) packetlen += 2; /* For message id */
	packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
	if(!packet) return MOSQ_ERR_NOMEM;

	packet->mid = mid;
	packet->command = PUBLISH | ((dup&0x1)<<3) | (qos<<1) | retain;
	packet->remaining_length = packetlen;
	rc = _mosquitto_packet_alloc(packet);
	if(rc){
		_mosquitto_free(packet);
		return rc;
	}
	/* Variable header (topic string) */
	_mosquitto_write_string(packet, topic, strlen(topic));
	if(qos > 0){
		_mosquitto_write_uint16(packet, mid);
	}

	/* Payload */
	if(payloadlen){
		_mosquitto_write_bytes(packet, payload, payloadlen);
	}

	return _mosquitto_packet_queue(mosq, packet);
}
/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.

The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.

Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <errno.h>
#include <string.h>

#include "mosquitto_internal.h"
#include "memory_mosq.h"
#include "net_mosq.h"
#include "send_mosq.h"

#define SOCKS_AUTH_NONE 0x00
#define SOCKS_AUTH_GSS 0x01
#define SOCKS_AUTH_USERPASS 0x02
#define SOCKS_AUTH_NO_ACCEPTABLE 0xFF

#define SOCKS_ATYPE_IP_V4 1 /* four bytes */
#define SOCKS_ATYPE_DOMAINNAME 3 /* one byte length, followed by fqdn no null, 256 max chars */
#define SOCKS_ATYPE_IP_V6 4 /* 16 bytes */

#define SOCKS_REPLY_SUCCEEDED 0x00
#define SOCKS_REPLY_GENERAL_FAILURE 0x01
#define SOCKS_REPLY_CONNECTION_NOT_ALLOWED 0x02
#define SOCKS_REPLY_NETWORK_UNREACHABLE 0x03
#define SOCKS_REPLY_HOST_UNREACHABLE 0x04
#define SOCKS_REPLY_CONNECTION_REFUSED 0x05
#define SOCKS_REPLY_TTL_EXPIRED 0x06
#define SOCKS_REPLY_COMMAND_NOT_SUPPORTED 0x07
#define SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED 0x08

int mosquitto_socks5_set(struct mosquitto *mosq, const char *host, int port, const char *username, const char *password)
{
#ifdef WITH_SOCKS
	if(!mosq) return MOSQ_ERR_INVAL;
	if(!host || strlen(host) > 256) return MOSQ_ERR_INVAL;
	if(port < 1 || port > 65535) return MOSQ_ERR_INVAL;

	if(mosq->socks5_host){
		_mosquitto_free(mosq->socks5_host);
	}

	mosq->socks5_host = _mosquitto_strdup(host);
	if(!mosq->socks5_host){
		return MOSQ_ERR_NOMEM;
	}

	mosq->socks5_port = port;

	if(mosq->socks5_username){
		_mosquitto_free(mosq->socks5_username);
	}
	if(mosq->socks5_password){
		_mosquitto_free(mosq->socks5_password);
	}

	if(username){
		mosq->socks5_username = _mosquitto_strdup(username);
		if(!mosq->socks5_username){
			return MOSQ_ERR_NOMEM;
		}

		if(password){
			mosq->socks5_password = _mosquitto_strdup(password);
			if(!mosq->socks5_password){
				_mosquitto_free(mosq->socks5_username);
				return MOSQ_ERR_NOMEM;
			}
		}
	}

	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}

#ifdef WITH_SOCKS
int mosquitto__socks5_send(struct mosquitto *mosq)
{
	struct _mosquitto_packet *packet;
	int slen;
	int ulen, plen;

	if(mosq->state == mosq_cs_socks5_new){
		packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
		if(!packet) return MOSQ_ERR_NOMEM;

		if(mosq->socks5_username){
			packet->packet_length = 4;
		}else{
			packet->packet_length = 3;
		}
		packet->payload = _mosquitto_malloc(sizeof(uint8_t)*packet->packet_length);

		packet->payload[0] = 0x05;
		if(mosq->socks5_username){
			packet->payload[1] = 2;
			packet->payload[2] = SOCKS_AUTH_NONE;
			packet->payload[3] = SOCKS_AUTH_USERPASS;
		}else{
			packet->payload[1] = 1;
			packet->payload[2] = SOCKS_AUTH_NONE;
		}

		pthread_mutex_lock(&mosq->state_mutex);
		mosq->state = mosq_cs_socks5_start;
		pthread_mutex_unlock(&mosq->state_mutex);

		mosq->in_packet.pos = 0;
		mosq->in_packet.packet_length = 2;
		mosq->in_packet.to_process = 2;
		mosq->in_packet.payload = _mosquitto_malloc(sizeof(uint8_t)*2);
		if(!mosq->in_packet.payload){
			_mosquitto_free(packet->payload);
			_mosquitto_free(packet);
			return MOSQ_ERR_NOMEM;
		}

		return _mosquitto_packet_queue(mosq, packet);
	}else if(mosq->state == mosq_cs_socks5_auth_ok){
		packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
		if(!packet) return MOSQ_ERR_NOMEM;

		packet->packet_length = 7+strlen(mosq->host);
		packet->payload = _mosquitto_malloc(sizeof(uint8_t)*packet->packet_length);

		slen = strlen(mosq->host);

		packet->payload[0] = 0x05;
		packet->payload[1] = 1;
		packet->payload[2] = 0;
		packet->payload[3] = SOCKS_ATYPE_DOMAINNAME;
		packet->payload[4] = slen;
		memcpy(&(packet->payload[5]), mosq->host, slen);
		packet->payload[5+slen] = MOSQ_MSB(mosq->port);
		packet->payload[6+slen] = MOSQ_LSB(mosq->port);

		pthread_mutex_lock(&mosq->state_mutex);
		mosq->state = mosq_cs_socks5_request;
		pthread_mutex_unlock(&mosq->state_mutex);

		mosq->in_packet.pos = 0;
		mosq->in_packet.packet_length = 5;
		mosq->in_packet.to_process = 5;
		mosq->in_packet.payload = _mosquitto_malloc(sizeof(uint8_t)*5);
		if(!mosq->in_packet.payload){
			_mosquitto_free(packet->payload);
			_mosquitto_free(packet);
			return MOSQ_ERR_NOMEM;
		}

		return _mosquitto_packet_queue(mosq, packet);
	}else if(mosq->state == mosq_cs_socks5_send_userpass){
		packet = _mosquitto_calloc(1, sizeof(struct _mosquitto_packet));
		if(!packet) return MOSQ_ERR_NOMEM;

		ulen = strlen(mosq->socks5_username);
		plen = strlen(mosq->socks5_password);
		packet->packet_length = 3 + ulen + plen;
		packet->payload = _mosquitto_malloc(sizeof(uint8_t)*packet->packet_length);


		packet->payload[0] = 0x01;
		packet->payload[1] = ulen;
		memcpy(&(packet->payload[2]), mosq->socks5_username, ulen);
		packet->payload[2+ulen] = plen;
		memcpy(&(packet->payload[3+ulen]), mosq->socks5_password, plen);

		pthread_mutex_lock(&mosq->state_mutex);
		mosq->state = mosq_cs_socks5_userpass_reply;
		pthread_mutex_unlock(&mosq->state_mutex);

		mosq->in_packet.pos = 0;
		mosq->in_packet.packet_length = 2;
		mosq->in_packet.to_process = 2;
		mosq->in_packet.payload = _mosquitto_malloc(sizeof(uint8_t)*2);
		if(!mosq->in_packet.payload){
			_mosquitto_free(packet->payload);
			_mosquitto_free(packet);
			return MOSQ_ERR_NOMEM;
		}

		return _mosquitto_packet_queue(mosq, packet);
	}
	return MOSQ_ERR_SUCCESS;
}

int mosquitto__socks5_read(struct mosquitto *mosq)
{
	ssize_t len;
	uint8_t *payload;
	uint8_t i;

	if(mosq->state == mosq_cs_socks5_start){
		while(mosq->in_packet.to_process > 0){
			len = _mosquitto_net_read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
			if(len > 0){
				mosq->in_packet.pos += len;
				mosq->in_packet.to_process -= len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return MOSQ_ERR_SUCCESS;
				}else{
					_mosquitto_packet_cleanup(&mosq->in_packet);
					switch(errno){
						case 0:
							return MOSQ_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}
		if(mosq->in_packet.payload[0] != 5){
			_mosquitto_packet_cleanup(&mosq->in_packet);
			return MOSQ_ERR_PROXY;
		}
		switch(mosq->in_packet.payload[1]){
			case SOCKS_AUTH_NONE:
				_mosquitto_packet_cleanup(&mosq->in_packet);
				mosq->state = mosq_cs_socks5_auth_ok;
				return mosquitto__socks5_send(mosq);
			case SOCKS_AUTH_USERPASS:
				_mosquitto_packet_cleanup(&mosq->in_packet);
				mosq->state = mosq_cs_socks5_send_userpass;
				return mosquitto__socks5_send(mosq);
			default:
				_mosquitto_packet_cleanup(&mosq->in_packet);
				return MOSQ_ERR_AUTH;
		}
	}else if(mosq->state == mosq_cs_socks5_userpass_reply){
		while(mosq->in_packet.to_process > 0){
			len = _mosquitto_net_read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
			if(len > 0){
				mosq->in_packet.pos += len;
				mosq->in_packet.to_process -= len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return MOSQ_ERR_SUCCESS;
				}else{
					_mosquitto_packet_cleanup(&mosq->in_packet);
					switch(errno){
						case 0:
							return MOSQ_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}
		if(mosq->in_packet.payload[0] != 1){
			_mosquitto_packet_cleanup(&mosq->in_packet);
			return MOSQ_ERR_PROXY;
		}
		if(mosq->in_packet.payload[1] == 0){
			_mosquitto_packet_cleanup(&mosq->in_packet);
			mosq->state = mosq_cs_socks5_auth_ok;
			return mosquitto__socks5_send(mosq);
		}else{
			i = mosq->in_packet.payload[1];
			_mosquitto_packet_cleanup(&mosq->in_packet);
			switch(i){
				case SOCKS_REPLY_CONNECTION_NOT_ALLOWED:
					return MOSQ_ERR_AUTH;

				case SOCKS_REPLY_NETWORK_UNREACHABLE:
				case SOCKS_REPLY_HOST_UNREACHABLE:
				case SOCKS_REPLY_CONNECTION_REFUSED:
					return MOSQ_ERR_NO_CONN;

				case SOCKS_REPLY_GENERAL_FAILURE:
				case SOCKS_REPLY_TTL_EXPIRED:
				case SOCKS_REPLY_COMMAND_NOT_SUPPORTED:
				case SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED:
					return MOSQ_ERR_PROXY;

				default:
					return MOSQ_ERR_INVAL;
			}
			return MOSQ_ERR_PROXY;
		}
	}else if(mosq->state == mosq_cs_socks5_request){
		while(mosq->in_packet.to_process > 0){
			len = _mosquitto_net_read(mosq, &(mosq->in_packet.payload[mosq->in_packet.pos]), mosq->in_packet.to_process);
			if(len > 0){
				mosq->in_packet.pos += len;
				mosq->in_packet.to_process -= len;
			}else{
#ifdef WIN32
				errno = WSAGetLastError();
#endif
				if(errno == EAGAIN || errno == COMPAT_EWOULDBLOCK){
					return MOSQ_ERR_SUCCESS;
				}else{
					_mosquitto_packet_cleanup(&mosq->in_packet);
					switch(errno){
						case 0:
							return MOSQ_ERR_PROXY;
						case COMPAT_ECONNRESET:
							return MOSQ_ERR_CONN_LOST;
						default:
							return MOSQ_ERR_ERRNO;
					}
				}
			}
		}

		if(mosq->in_packet.packet_length == 5){
			/* First part of the packet has been received, we now know what else to expect. */
			if(mosq->in_packet.payload[3] == SOCKS_ATYPE_IP_V4){
				mosq->in_packet.to_process += 4+2-1; /* 4 bytes IPv4, 2 bytes port, -1 byte because we've already read the first byte */
				mosq->in_packet.packet_length += 4+2-1;
			}else if(mosq->in_packet.payload[3] == SOCKS_ATYPE_IP_V6){
				mosq->in_packet.to_process += 16+2-1; /* 16 bytes IPv6, 2 bytes port, -1 byte because we've already read the first byte */
				mosq->in_packet.packet_length += 16+2-1;
			}else if(mosq->in_packet.payload[3] == SOCKS_ATYPE_DOMAINNAME){
				if(mosq->in_packet.payload[4] > 0 && mosq->in_packet.payload[4] <= 255){
					mosq->in_packet.to_process += mosq->in_packet.payload[4];
					mosq->in_packet.packet_length += mosq->in_packet.payload[4];
				}
			}else{
				_mosquitto_packet_cleanup(&mosq->in_packet);
				return MOSQ_ERR_PROTOCOL;
			}
			payload = _mosquitto_realloc(mosq->in_packet.payload, mosq->in_packet.packet_length);
			if(payload){
				mosq->in_packet.payload = payload;
			}else{
				_mosquitto_packet_cleanup(&mosq->in_packet);
				return MOSQ_ERR_NOMEM;
			}
			return MOSQ_ERR_SUCCESS;
		}

		/* Entire packet is now read. */
		if(mosq->in_packet.payload[0] != 5){
			_mosquitto_packet_cleanup(&mosq->in_packet);
			return MOSQ_ERR_PROXY;
		}
		if(mosq->in_packet.payload[1] == 0){
			/* Auth passed */
			_mosquitto_packet_cleanup(&mosq->in_packet);
			mosq->state = mosq_cs_new;
			return _mosquitto_send_connect(mosq, mosq->keepalive, mosq->clean_session);
		}else{
			i = mosq->in_packet.payload[1];
			_mosquitto_packet_cleanup(&mosq->in_packet);
			mosq->state = mosq_cs_socks5_new;
			switch(i){
				case SOCKS_REPLY_CONNECTION_NOT_ALLOWED:
					return MOSQ_ERR_AUTH;

				case SOCKS_REPLY_NETWORK_UNREACHABLE:
				case SOCKS_REPLY_HOST_UNREACHABLE:
				case SOCKS_REPLY_CONNECTION_REFUSED:
					return MOSQ_ERR_NO_CONN;

				case SOCKS_REPLY_GENERAL_FAILURE:
				case SOCKS_REPLY_TTL_EXPIRED:
				case SOCKS_REPLY_COMMAND_NOT_SUPPORTED:
				case SOCKS_REPLY_ADDRESS_TYPE_NOT_SUPPORTED:
					return MOSQ_ERR_PROXY;

				default:
					return MOSQ_ERR_INVAL;
			}
		}
	}else{
		return _mosquitto_packet_read(mosq);
	}
	return MOSQ_ERR_SUCCESS;
}
#endif
/*
Copyright (c) 2013,2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifdef WITH_SRV
#  include <ares.h>

#  include <arpa/nameser.h>
#  include <stdio.h>
#  include <string.h>
#endif

#include "logging_mosq.h"
#include "memory_mosq.h"
#include "mosquitto_internal.h"
#include "mosquitto.h"

#ifdef WITH_SRV
static void srv_callback(void *arg, int status, int timeouts, unsigned char *abuf, int alen)
{   
	struct mosquitto *mosq = arg;
	struct ares_srv_reply *reply = NULL;
	if(status == ARES_SUCCESS){
		status = ares_parse_srv_reply(abuf, alen, &reply);
		if(status == ARES_SUCCESS){
			// FIXME - choose which answer to use based on rfc2782 page 3. */
			mosquitto_connect(mosq, reply->host, reply->port, mosq->keepalive);
		}
	}else{
		_mosquitto_log_printf(mosq, MOSQ_LOG_ERR, "Error: SRV lookup failed (%d).", status);
		/* FIXME - calling on_disconnect here isn't correct. */
		pthread_mutex_lock(&mosq->callback_mutex);
		if(mosq->on_disconnect){
			mosq->in_callback = true;
			mosq->on_disconnect(mosq, mosq->userdata, 2);
			mosq->in_callback = false;
		}
		pthread_mutex_unlock(&mosq->callback_mutex);
	}
}
#endif

int mosquitto_connect_srv(struct mosquitto *mosq, const char *host, int keepalive, const char *bind_address)
{
#ifdef WITH_SRV
	char *h;
	int rc;
	if(!mosq) return MOSQ_ERR_INVAL;

	rc = ares_init(&mosq->achan);
	if(rc != ARES_SUCCESS){
		return MOSQ_ERR_UNKNOWN;
	}

	if(!host){
		// get local domain
	}else{
#ifdef WITH_TLS
		if(mosq->tls_cafile || mosq->tls_capath || mosq->tls_psk){
			h = _mosquitto_malloc(strlen(host) + strlen("_secure-mqtt._tcp.") + 1);
			if(!h) return MOSQ_ERR_NOMEM;
			sprintf(h, "_secure-mqtt._tcp.%s", host);
		}else{
#endif
			h = _mosquitto_malloc(strlen(host) + strlen("_mqtt._tcp.") + 1);
			if(!h) return MOSQ_ERR_NOMEM;
			sprintf(h, "_mqtt._tcp.%s", host);
#ifdef WITH_TLS
		}
#endif
		ares_search(mosq->achan, h, ns_c_in, ns_t_srv, srv_callback, mosq);
		_mosquitto_free(h);
	}

	pthread_mutex_lock(&mosq->state_mutex);
	mosq->state = mosq_cs_connect_srv;
	pthread_mutex_unlock(&mosq->state_mutex);

	mosq->keepalive = keepalive;

	return MOSQ_ERR_SUCCESS;

#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}


/*
Copyright (c) 2011-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <config.h>

#ifndef WIN32
#include <unistd.h>
#endif

#include <mosquitto_internal.h>
#include <net_mosq.h>

void *_mosquitto_thread_main(void *obj);

int mosquitto_loop_start(struct mosquitto *mosq)
{
#ifdef WITH_THREADING
	if(!mosq || mosq->threaded) return MOSQ_ERR_INVAL;

	mosq->threaded = true;
	pthread_create(&mosq->thread_id, NULL, _mosquitto_thread_main, mosq);
	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}

int mosquitto_loop_stop(struct mosquitto *mosq, bool force)
{
#ifdef WITH_THREADING
#  ifndef WITH_BROKER
	char sockpair_data = 0;
#  endif

	if(!mosq || !mosq->threaded) return MOSQ_ERR_INVAL;


	/* Write a single byte to sockpairW (connected to sockpairR) to break out
	 * of select() if in threaded mode. */
	if(mosq->sockpairW != INVALID_SOCKET){
#ifndef WIN32
		if(write(mosq->sockpairW, &sockpair_data, 1)){
		}
#else
		send(mosq->sockpairW, &sockpair_data, 1, 0);
#endif
	}
	
	if(force){
		pthread_cancel(mosq->thread_id);
	}
	pthread_join(mosq->thread_id, NULL);
	mosq->thread_id = pthread_self();
	mosq->threaded = false;

	return MOSQ_ERR_SUCCESS;
#else
	return MOSQ_ERR_NOT_SUPPORTED;
#endif
}

#ifdef WITH_THREADING
void *_mosquitto_thread_main(void *obj)
{
	struct mosquitto *mosq = obj;

	if(!mosq) return NULL;

	pthread_mutex_lock(&mosq->state_mutex);
	if(mosq->state == mosq_cs_connect_async){
		pthread_mutex_unlock(&mosq->state_mutex);
		mosquitto_reconnect(mosq);
	}else{
		pthread_mutex_unlock(&mosq->state_mutex);
	}

	if(!mosq->keepalive){
		/* Sleep for a day if keepalive disabled. */
		mosquitto_loop_forever(mosq, 1000*86400, 1);
	}else{
		/* Sleep for our keepalive value. publish() etc. will wake us up. */
		mosquitto_loop_forever(mosq, mosq->keepalive*1000, 1);
	}

	return obj;
}
#endif

int mosquitto_threaded_set(struct mosquitto *mosq, bool threaded)
{
	if(!mosq) return MOSQ_ERR_INVAL;

	mosq->threaded = threaded;

	return MOSQ_ERR_SUCCESS;
}
/*
Copyright (c) 2013,2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifdef __APPLE__
#include <mach/mach.h>
#include <mach/mach_time.h>
#endif

#ifdef WIN32
#  define _WIN32_WINNT _WIN32_WINNT_VISTA
#  include <windows.h>
#else
#  include <unistd.h>
#endif
#include <time.h>

#include "mosquitto.h"
#include "time_mosq.h"

#ifdef WIN32
static bool tick64 = false;

void _windows_time_version_check(void)
{
	OSVERSIONINFO vi;

	tick64 = false;

	memset(&vi, 0, sizeof(OSVERSIONINFO));
	vi.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	if(GetVersionEx(&vi)){
		if(vi.dwMajorVersion > 5){
			tick64 = true;
		}
	}
}
#endif

time_t mosquitto_time(void)
{
#ifdef WIN32
	if(tick64){
		return GetTickCount64()/1000;
	}else{
		return GetTickCount()/1000; /* FIXME - need to deal with overflow. */
	}
#elif _POSIX_TIMERS>0 && defined(_POSIX_MONOTONIC_CLOCK)
	struct timespec tp;

	clock_gettime(CLOCK_MONOTONIC, &tp);
	return tp.tv_sec;
#elif defined(__APPLE__)
	static mach_timebase_info_data_t tb;
    uint64_t ticks;
	uint64_t sec;

	ticks = mach_absolute_time();

	if(tb.denom == 0){
		mach_timebase_info(&tb);
	}
	sec = ticks*tb.numer/tb.denom/1000000000;

	return (time_t)sec;
#else
	return time(NULL);
#endif
}

/*
Copyright (c) 2013,2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#ifdef WITH_TLS

#ifdef WIN32
#  include <winsock2.h>
#  include <ws2tcpip.h>
#else
#  include <arpa/inet.h>
#  include <sys/socket.h>
#endif

#include <string.h>
#include <openssl/conf.h>
#include <openssl/x509v3.h>
#include <openssl/ssl.h>

#ifdef WITH_BROKER
#  include "mosquitto_broker.h"
#endif
#include "mosquitto_internal.h"
#include "tls_mosq.h"

extern int tls_ex_index_mosq;

int _mosquitto_server_certificate_verify(int preverify_ok, X509_STORE_CTX *ctx)
{
	/* Preverify should have already checked expiry, revocation.
	 * We need to verify the hostname. */
	struct mosquitto *mosq;
	SSL *ssl;
	X509 *cert;

	/* Always reject if preverify_ok has failed. */
	if(!preverify_ok) return 0;

	ssl = X509_STORE_CTX_get_ex_data(ctx, SSL_get_ex_data_X509_STORE_CTX_idx());
	mosq = SSL_get_ex_data(ssl, tls_ex_index_mosq);
	if(!mosq) return 0;

	if(mosq->tls_insecure == false){
		if(X509_STORE_CTX_get_error_depth(ctx) == 0){
			/* FIXME - use X509_check_host() etc. for sufficiently new openssl (>=1.1.x) */
			cert = X509_STORE_CTX_get_current_cert(ctx);
			/* This is the peer certificate, all others are upwards in the chain. */
#if defined(WITH_BROKER)
			return _mosquitto_verify_certificate_hostname(cert, mosq->bridge->addresses[mosq->bridge->cur_address].address);
#else
			return _mosquitto_verify_certificate_hostname(cert, mosq->host);
#endif
		}else{
			return preverify_ok;
		}
	}else{
		return preverify_ok;
	}
}

int mosquitto__cmp_hostname_wildcard(char *certname, const char *hostname)
{
	int i;
	int len;

	if(!certname || !hostname){
		return 1;
	}

	if(certname[0] == '*'){
		if(certname[1] != '.'){
			return 1;
		}
		certname += 2;
		len = strlen(hostname);
		for(i=0; i<len-1; i++){
			if(hostname[i] == '.'){
				hostname += i+1;
				break;
			}
		}
		return strcasecmp(certname, hostname);
	}else{
		return strcasecmp(certname, hostname);
	}
}

/* This code is based heavily on the example provided in "Secure Programming
 * Cookbook for C and C++".
 */
int _mosquitto_verify_certificate_hostname(X509 *cert, const char *hostname)
{
	int i;
	char name[256];
	X509_NAME *subj;
	bool have_san_dns = false;
	STACK_OF(GENERAL_NAME) *san;
	const GENERAL_NAME *nval;
	const unsigned char *data;
	unsigned char ipv6_addr[16];
	unsigned char ipv4_addr[4];
	int ipv6_ok;
	int ipv4_ok;

#ifdef WIN32
	ipv6_ok = InetPton(AF_INET6, hostname, &ipv6_addr);
	ipv4_ok = InetPton(AF_INET, hostname, &ipv4_addr);
#else
	ipv6_ok = inet_pton(AF_INET6, hostname, &ipv6_addr);
	ipv4_ok = inet_pton(AF_INET, hostname, &ipv4_addr);
#endif

	san = X509_get_ext_d2i(cert, NID_subject_alt_name, NULL, NULL);
	if(san){
		for(i=0; i<sk_GENERAL_NAME_num(san); i++){
			nval = sk_GENERAL_NAME_value(san, i);
			if(nval->type == GEN_DNS){
				data = ASN1_STRING_data(nval->d.dNSName);
				if(data && !mosquitto__cmp_hostname_wildcard((char *)data, hostname)){
					return 1;
				}
				have_san_dns = true;
			}else if(nval->type == GEN_IPADD){
				data = ASN1_STRING_data(nval->d.iPAddress);
				if(nval->d.iPAddress->length == 4 && ipv4_ok){
					if(!memcmp(ipv4_addr, data, 4)){
						return 1;
					}
				}else if(nval->d.iPAddress->length == 16 && ipv6_ok){
					if(!memcmp(ipv6_addr, data, 16)){
						return 1;
					}
				}
			}
		}
		if(have_san_dns){
			/* Only check CN if subjectAltName DNS entry does not exist. */
			return 0;
		}
	}
	subj = X509_get_subject_name(cert);
	if(X509_NAME_get_text_by_NID(subj, NID_commonName, name, sizeof(name)) > 0){
		name[sizeof(name) - 1] = '\0';
		if (!mosquitto__cmp_hostname_wildcard(name, hostname)) return 1;
	}
	return 0;
}

#endif

/*
Copyright (c) 2009-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <assert.h>
#include <string.h>

#ifdef WIN32
#include <winsock2.h>
#endif


#include <mosquitto.h>
#include <memory_mosq.h>
#include <net_mosq.h>
#include <send_mosq.h>
#include <time_mosq.h>
#include <tls_mosq.h>
#include <util_mosq.h>

#ifdef WITH_BROKER
#include <mosquitto_broker.h>
#endif

#ifdef WITH_WEBSOCKETS
#include <libwebsockets.h>
#endif

int _mosquitto_packet_alloc(struct _mosquitto_packet *packet)
{
	uint8_t remaining_bytes[5], byte;
	uint32_t remaining_length;
	int i;

	assert(packet);

	remaining_length = packet->remaining_length;
	packet->payload = NULL;
	packet->remaining_count = 0;
	do{
		byte = remaining_length % 128;
		remaining_length = remaining_length / 128;
		/* If there are more digits to encode, set the top bit of this digit */
		if(remaining_length > 0){
			byte = byte | 0x80;
		}
		remaining_bytes[packet->remaining_count] = byte;
		packet->remaining_count++;
	}while(remaining_length > 0 && packet->remaining_count < 5);
	if(packet->remaining_count == 5) return MOSQ_ERR_PAYLOAD_SIZE;
	packet->packet_length = packet->remaining_length + 1 + packet->remaining_count;
#ifdef WITH_WEBSOCKETS
	packet->payload = _mosquitto_malloc(sizeof(uint8_t)*packet->packet_length + LWS_SEND_BUFFER_PRE_PADDING + LWS_SEND_BUFFER_POST_PADDING);
#else
	packet->payload = _mosquitto_malloc(sizeof(uint8_t)*packet->packet_length);
#endif
	if(!packet->payload) return MOSQ_ERR_NOMEM;

	packet->payload[0] = packet->command;
	for(i=0; i<packet->remaining_count; i++){
		packet->payload[i+1] = remaining_bytes[i];
	}
	packet->pos = 1 + packet->remaining_count;

	return MOSQ_ERR_SUCCESS;
}

#ifdef WITH_BROKER
void _mosquitto_check_keepalive(struct mosquitto_db *db, struct mosquitto *mosq)
#else
void _mosquitto_check_keepalive(struct mosquitto *mosq)
#endif
{
	time_t last_msg_out;
	time_t last_msg_in;
	time_t now = mosquitto_time();
#ifndef WITH_BROKER
	int rc;
#endif

	assert(mosq);
#if defined(WITH_BROKER) && defined(WITH_BRIDGE)
	/* Check if a lazy bridge should be timed out due to idle. */
	if(mosq->bridge && mosq->bridge->start_type == bst_lazy
				&& mosq->sock != INVALID_SOCKET
				&& now - mosq->last_msg_out >= mosq->bridge->idle_timeout){

		_mosquitto_log_printf(NULL, MOSQ_LOG_NOTICE, "Bridge connection %s has exceeded idle timeout, disconnecting.", mosq->id);
		_mosquitto_socket_close(db, mosq);
		return;
	}
#endif
	pthread_mutex_lock(&mosq->msgtime_mutex);
	last_msg_out = mosq->last_msg_out;
	last_msg_in = mosq->last_msg_in;
	pthread_mutex_unlock(&mosq->msgtime_mutex);
	if(mosq->keepalive && mosq->sock != INVALID_SOCKET &&
			(now - last_msg_out >= mosq->keepalive || now - last_msg_in >= mosq->keepalive)){

		if(mosq->state == mosq_cs_connected && mosq->ping_t == 0){
			_mosquitto_send_pingreq(mosq);
			/* Reset last msg times to give the server time to send a pingresp */
			pthread_mutex_lock(&mosq->msgtime_mutex);
			mosq->last_msg_in = now;
			mosq->last_msg_out = now;
			pthread_mutex_unlock(&mosq->msgtime_mutex);
		}else{
#ifdef WITH_BROKER
			if(mosq->listener){
				mosq->listener->client_count--;
				assert(mosq->listener->client_count >= 0);
			}
			mosq->listener = NULL;
			_mosquitto_socket_close(db, mosq);
#else
			_mosquitto_socket_close(mosq);
			pthread_mutex_lock(&mosq->state_mutex);
			if(mosq->state == mosq_cs_disconnecting){
				rc = MOSQ_ERR_SUCCESS;
			}else{
				rc = 1;
			}
			pthread_mutex_unlock(&mosq->state_mutex);
			pthread_mutex_lock(&mosq->callback_mutex);
			if(mosq->on_disconnect){
				mosq->in_callback = true;
				mosq->on_disconnect(mosq, mosq->userdata, rc);
				mosq->in_callback = false;
			}
			pthread_mutex_unlock(&mosq->callback_mutex);
#endif
		}
	}
}

uint16_t _mosquitto_mid_generate(struct mosquitto *mosq)
{
	/* FIXME - this would be better with atomic increment, but this is safer
	 * for now for a bug fix release.
	 *
	 * If this is changed to use atomic increment, callers of this function
	 * will have to be aware that they may receive a 0 result, which may not be
	 * used as a mid.
	 */
	uint16_t mid;
	assert(mosq);

	pthread_mutex_lock(&mosq->mid_mutex);
	mosq->last_mid++;
	if(mosq->last_mid == 0) mosq->last_mid++;
	mid = mosq->last_mid;
	pthread_mutex_unlock(&mosq->mid_mutex);
	
	return mid;
}

/* Check that a topic used for publishing is valid.
 * Search for + or # in a topic. Return MOSQ_ERR_INVAL if found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_pub_topic_check(const char *str)
{
	int len = 0;
	while(str && str[0]){
		if(str[0] == '+' || str[0] == '#'){
			return MOSQ_ERR_INVAL;
		}
		len++;
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;

	return MOSQ_ERR_SUCCESS;
}

/* Check that a topic used for subscriptions is valid.
 * Search for + or # in a topic, check they aren't in invalid positions such as
 * foo/#/bar, foo/+bar or foo/bar#.
 * Return MOSQ_ERR_INVAL if invalid position found.
 * Also returns MOSQ_ERR_INVAL if the topic string is too long.
 * Returns MOSQ_ERR_SUCCESS if everything is fine.
 */
int mosquitto_sub_topic_check(const char *str)
{
	char c = '\0';
	int len = 0;
	while(str && str[0]){
		if(str[0] == '+'){
			if((c != '\0' && c != '/') || (str[1] != '\0' && str[1] != '/')){
				return MOSQ_ERR_INVAL;
			}
		}else if(str[0] == '#'){
			if((c != '\0' && c != '/')  || str[1] != '\0'){
				return MOSQ_ERR_INVAL;
			}
		}
		len++;
		c = str[0];
		str = &str[1];
	}
	if(len > 65535) return MOSQ_ERR_INVAL;

	return MOSQ_ERR_SUCCESS;
}

/* Does a topic match a subscription? */
int mosquitto_topic_matches_sub(const char *sub, const char *topic, bool *result)
{
	int slen, tlen;
	int spos, tpos;
	bool multilevel_wildcard = false;

	if(!sub || !topic || !result) return MOSQ_ERR_INVAL;

	slen = strlen(sub);
	tlen = strlen(topic);

	if(slen && tlen){
		if((sub[0] == '$' && topic[0] != '$')
				|| (topic[0] == '$' && sub[0] != '$')){

			*result = false;
			return MOSQ_ERR_SUCCESS;
		}
	}

	spos = 0;
	tpos = 0;

	while(spos < slen && tpos < tlen){
		if(sub[spos] == topic[tpos]){
			if(tpos == tlen-1){
				/* Check for e.g. foo matching foo/# */
				if(spos == slen-3 
						&& sub[spos+1] == '/'
						&& sub[spos+2] == '#'){
					*result = true;
					multilevel_wildcard = true;
					return MOSQ_ERR_SUCCESS;
				}
			}
			spos++;
			tpos++;
			if(spos == slen && tpos == tlen){
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}else if(tpos == tlen && spos == slen-1 && sub[spos] == '+'){
				spos++;
				*result = true;
				return MOSQ_ERR_SUCCESS;
			}
		}else{
			if(sub[spos] == '+'){
				spos++;
				while(tpos < tlen && topic[tpos] != '/'){
					tpos++;
				}
				if(tpos == tlen && spos == slen){
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else if(sub[spos] == '#'){
				multilevel_wildcard = true;
				if(spos+1 != slen){
					*result = false;
					return MOSQ_ERR_SUCCESS;
				}else{
					*result = true;
					return MOSQ_ERR_SUCCESS;
				}
			}else{
				*result = false;
				return MOSQ_ERR_SUCCESS;
			}
		}
	}
	if(multilevel_wildcard == false && (tpos < tlen || spos < slen)){
		*result = false;
	}

	return MOSQ_ERR_SUCCESS;
}

#ifdef REAL_WITH_TLS_PSK
int _mosquitto_hex2bin(const char *hex, unsigned char *bin, int bin_max_len)
{
	BIGNUM *bn = NULL;
	int len;

	if(BN_hex2bn(&bn, hex) == 0){
		if(bn) BN_free(bn);
		return 0;
	}
	if(BN_num_bytes(bn) > bin_max_len){
		BN_free(bn);
		return 0;
	}

	len = BN_bn2bin(bn, bin);
	BN_free(bn);
	return len;
}
#endif

FILE *_mosquitto_fopen(const char *path, const char *mode)
{
#ifdef WIN32
	char buf[4096];
	int rc;
	rc = ExpandEnvironmentStrings(path, buf, 4096);
	if(rc == 0 || rc > 4096){
		return NULL;
	}else{
		return fopen(buf, mode);
	}
#else
	return fopen(path, mode);
#endif
}

/*
Copyright (c) 2010-2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/

#include <stdio.h>
#include <string.h>

#include <mosquitto_internal.h>
#include <memory_mosq.h>
#include <mqtt3_protocol.h>

int _mosquitto_will_set(struct mosquitto *mosq, const char *topic, int payloadlen, const void *payload, int qos, bool retain)
{
	int rc = MOSQ_ERR_SUCCESS;

	if(!mosq || !topic) return MOSQ_ERR_INVAL;
	if(payloadlen < 0 || payloadlen > MQTT_MAX_PAYLOAD) return MOSQ_ERR_PAYLOAD_SIZE;
	if(payloadlen > 0 && !payload) return MOSQ_ERR_INVAL;

	if(mosquitto_pub_topic_check(topic)) return MOSQ_ERR_INVAL;

	if(mosq->will){
		if(mosq->will->topic){
			_mosquitto_free(mosq->will->topic);
			mosq->will->topic = NULL;
		}
		if(mosq->will->payload){
			_mosquitto_free(mosq->will->payload);
			mosq->will->payload = NULL;
		}
		_mosquitto_free(mosq->will);
		mosq->will = NULL;
	}

	mosq->will = _mosquitto_calloc(1, sizeof(struct mosquitto_message));
	if(!mosq->will) return MOSQ_ERR_NOMEM;
	mosq->will->topic = _mosquitto_strdup(topic);
	if(!mosq->will->topic){
		rc = MOSQ_ERR_NOMEM;
		goto cleanup;
	}
	mosq->will->payloadlen = payloadlen;
	if(mosq->will->payloadlen > 0){
		if(!payload){
			rc = MOSQ_ERR_INVAL;
			goto cleanup;
		}
		mosq->will->payload = _mosquitto_malloc(sizeof(char)*mosq->will->payloadlen);
		if(!mosq->will->payload){
			rc = MOSQ_ERR_NOMEM;
			goto cleanup;
		}

		memcpy(mosq->will->payload, payload, payloadlen);
	}
	mosq->will->qos = qos;
	mosq->will->retain = retain;

	return MOSQ_ERR_SUCCESS;

cleanup:
	if(mosq->will){
		if(mosq->will->topic) _mosquitto_free(mosq->will->topic);
		if(mosq->will->payload) _mosquitto_free(mosq->will->payload);
	}
	_mosquitto_free(mosq->will);
	mosq->will = NULL;

	return rc;
}

int _mosquitto_will_clear(struct mosquitto *mosq)
{
	if(!mosq->will) return MOSQ_ERR_SUCCESS;

	if(mosq->will->topic){
		_mosquitto_free(mosq->will->topic);
		mosq->will->topic = NULL;
	}
	if(mosq->will->payload){
		_mosquitto_free(mosq->will->payload);
		mosq->will->payload = NULL;
	}
	_mosquitto_free(mosq->will);
	mosq->will = NULL;

	return MOSQ_ERR_SUCCESS;
}


/*
Copyright (c) 2014 Roger Light <roger@atchoo.org>

All rights reserved. This program and the accompanying materials
are made available under the terms of the Eclipse Public License v1.0
and Eclipse Distribution License v1.0 which accompany this distribution.
 
The Eclipse Public License is available at
   http://www.eclipse.org/legal/epl-v10.html
and the Eclipse Distribution License is available at
  http://www.eclipse.org/org/documents/edl-v10.php.
 
Contributors:
   Roger Light - initial implementation and documentation.
*/


#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#else
#include <process.h>
#include <winsock2.h>
#define snprintf sprintf_s
#endif

#include <mosquitto.h>
#include "client_shared.h"

static int mosquitto__parse_socks_url(struct mosq_config *cfg, char *url);
static int client_config_line_proc(struct mosq_config *cfg, int pub_or_sub, int argc, char *argv[]);

void init_config(struct mosq_config *cfg)
{
	memset(cfg, 0, sizeof(*cfg));
	cfg->port = 1883;
	cfg->max_inflight = 20;
	cfg->keepalive = 60;
	cfg->clean_session = true;
	cfg->eol = true;
	cfg->protocol_version = MQTT_PROTOCOL_V31;
}

void client_config_cleanup(struct mosq_config *cfg)
{
	int i;
	free(cfg->id);
	free(cfg->id_prefix);
	free(cfg->host);
	free(cfg->file_input);
	free(cfg->message);
	free(cfg->topic);
	free(cfg->bind_address);
	free(cfg->username);
	free(cfg->password);
	free(cfg->will_topic);
	free(cfg->will_payload);
#ifdef WITH_TLS
	free(cfg->cafile);
	free(cfg->capath);
	free(cfg->certfile);
	free(cfg->keyfile);
	free(cfg->ciphers);
	free(cfg->tls_version);
#  ifdef WITH_TLS_PSK
	free(cfg->psk);
	free(cfg->psk_identity);
#  endif
#endif
	if(cfg->topics){
		for(i=0; i<cfg->topic_count; i++){
			free(cfg->topics[i]);
		}
		free(cfg->topics);
	}
	if(cfg->filter_outs){
		for(i=0; i<cfg->filter_out_count; i++){
			free(cfg->filter_outs[i]);
		}
		free(cfg->filter_outs);
	}
#ifdef WITH_SOCKS
	free(cfg->socks5_host);
	free(cfg->socks5_username);
	free(cfg->socks5_password);
#endif
}

int client_config_load(struct mosq_config *cfg, int pub_or_sub, int argc, char *argv[])
{
	int rc;
	FILE *fptr;
	char line[1024];
	int count;
	char *loc = NULL;
	int len;
	char *args[3];

#ifndef WIN32
	char *env;
#else
	char env[1024];
#endif
	args[0] = NULL;

	init_config(cfg);

	/* Default config file */
#ifndef WIN32
	env = getenv("XDG_CONFIG_HOME");
	if(env){
		len = strlen(env) + strlen("/mosquitto_pub") + 1;
		loc = malloc(len);
		if(pub_or_sub == CLIENT_PUB){
			snprintf(loc, len, "%s/mosquitto_pub", env);
		}else{
			snprintf(loc, len, "%s/mosquitto_sub", env);
		}
		loc[len-1] = '\0';
	}else{
		env = getenv("HOME");
		if(env){
			len = strlen(env) + strlen("/.config/mosquitto_pub") + 1;
			loc = malloc(len);
			if(pub_or_sub == CLIENT_PUB){
				snprintf(loc, len, "%s/.config/mosquitto_pub", env);
			}else{
				snprintf(loc, len, "%s/.config/mosquitto_sub", env);
			}
			loc[len-1] = '\0';
		}else{
			fprintf(stderr, "Warning: Unable to locate configuration directory, default config not loaded.\n");
		}
	}

#else
	rc = GetEnvironmentVariable("USERPROFILE", env, 1024);
	if(rc > 0 && rc < 1024){
		len = strlen(env) + strlen("\\mosquitto_pub.conf") + 1;
		loc = malloc(len);
		if(pub_or_sub == CLIENT_PUB){
			snprintf(loc, len, "%s\\mosquitto_pub.conf", env);
		}else{
			snprintf(loc, len, "%s\\mosquitto_sub.conf", env);
		}
		loc[len-1] = '\0';
	}else{
		fprintf(stderr, "Warning: Unable to locate configuration directory, default config not loaded.\n");
	}
#endif

	if(loc){
		fptr = fopen(loc, "rt");
		if(fptr){
			while(fgets(line, 1024, fptr)){
				if(line[0] == '#') continue; /* Comments */

				while(line[strlen(line)-1] == 10 || line[strlen(line)-1] == 13){
					line[strlen(line)-1] = 0;
				}
				/* All offset by one "args" here, because real argc/argv has
				 * program name as the first entry. */
				args[1] = strtok(line, " ");
				if(args[1]){
					args[2] = strtok(NULL, " ");
					if(args[2]){
						count = 3;
					}else{
						count = 2;
					}
					rc = client_config_line_proc(cfg, pub_or_sub, count, args);
					if(rc){
						fclose(fptr);
						free(loc);
						return rc;
					}
				}
			}
			fclose(fptr);
		}
		free(loc);
	}

	/* Deal with real argc/argv */
	rc = client_config_line_proc(cfg, pub_or_sub, argc, argv);
	if(rc) return rc;

	if(cfg->will_payload && !cfg->will_topic){
		fprintf(stderr, "Error: Will payload given, but no will topic given.\n");
		return 1;
	}
	if(cfg->will_retain && !cfg->will_topic){
		fprintf(stderr, "Error: Will retain given, but no will topic given.\n");
		return 1;
	}
	if(cfg->password && !cfg->username){
		if(!cfg->quiet) fprintf(stderr, "Warning: Not using password since username not set.\n");
	}
#ifdef WITH_TLS
	if((cfg->certfile && !cfg->keyfile) || (cfg->keyfile && !cfg->certfile)){
		fprintf(stderr, "Error: Both certfile and keyfile must be provided if one of them is.\n");
		return 1;
	}
#endif
#ifdef WITH_TLS_PSK
	if((cfg->cafile || cfg->capath) && cfg->psk){
		if(!cfg->quiet) fprintf(stderr, "Error: Only one of --psk or --cafile/--capath may be used at once.\n");
		return 1;
	}
	if(cfg->psk && !cfg->psk_identity){
		if(!cfg->quiet) fprintf(stderr, "Error: --psk-identity required if --psk used.\n");
		return 1;
	}
#endif

	if(pub_or_sub == CLIENT_SUB){
		if(cfg->clean_session == false && (cfg->id_prefix || !cfg->id)){
			if(!cfg->quiet) fprintf(stderr, "Error: You must provide a client id if you are using the -c option.\n");
			return 1;
		}
		if(cfg->topic_count == 0){
			if(!cfg->quiet) fprintf(stderr, "Error: You must specify a topic to subscribe to.\n");
			return 1;
		}
	}

	if(!cfg->host){
		cfg->host = "localhost";
	}
	return MOSQ_ERR_SUCCESS;
}

/* Process a tokenised single line from a file or set of real argc/argv */
int client_config_line_proc(struct mosq_config *cfg, int pub_or_sub, int argc, char *argv[])
{
	int i;

	for(i=1; i<argc; i++){
		if(!strcmp(argv[i], "-p") || !strcmp(argv[i], "--port")){
			if(i==argc-1){
				fprintf(stderr, "Error: -p argument given but no port specified.\n\n");
				return 1;
			}else{
				cfg->port = atoi(argv[i+1]);
				if(cfg->port<1 || cfg->port>65535){
					fprintf(stderr, "Error: Invalid port given: %d\n", cfg->port);
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "-A")){
			if(i==argc-1){
				fprintf(stderr, "Error: -A argument given but no address specified.\n\n");
				return 1;
			}else{
				cfg->bind_address = strdup(argv[i+1]);
			}
			i++;
#ifdef WITH_TLS
		}else if(!strcmp(argv[i], "--cafile")){
			if(i==argc-1){
				fprintf(stderr, "Error: --cafile argument given but no file specified.\n\n");
				return 1;
			}else{
				cfg->cafile = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--capath")){
			if(i==argc-1){
				fprintf(stderr, "Error: --capath argument given but no directory specified.\n\n");
				return 1;
			}else{
				cfg->capath = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--cert")){
			if(i==argc-1){
				fprintf(stderr, "Error: --cert argument given but no file specified.\n\n");
				return 1;
			}else{
				cfg->certfile = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--ciphers")){
			if(i==argc-1){
				fprintf(stderr, "Error: --ciphers argument given but no ciphers specified.\n\n");
				return 1;
			}else{
				cfg->ciphers = strdup(argv[i+1]);
			}
			i++;
#endif
		}else if(!strcmp(argv[i], "-C")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}else{
				if(i==argc-1){
					fprintf(stderr, "Error: -C argument given but no count specified.\n\n");
					return 1;
				}else{
					cfg->msg_count = atoi(argv[i+1]);
					if(cfg->msg_count < 1){
						fprintf(stderr, "Error: Invalid message count \"%d\".\n\n", cfg->msg_count);
						return 1;
					}
				}
				i++;
			}
		}else if(!strcmp(argv[i], "-d") || !strcmp(argv[i], "--debug")){
			cfg->debug = true;
		}else if(!strcmp(argv[i], "-f") || !strcmp(argv[i], "--file")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			if(cfg->pub_mode != MSGMODE_NONE){
				fprintf(stderr, "Error: Only one type of message can be sent at once.\n\n");
				return 1;
			}else if(i==argc-1){
				fprintf(stderr, "Error: -f argument given but no file specified.\n\n");
				return 1;
			}else{
				cfg->pub_mode = MSGMODE_FILE;
				cfg->file_input = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--help")){
			return 2;
		}else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--host")){
			if(i==argc-1){
				fprintf(stderr, "Error: -h argument given but no host specified.\n\n");
				return 1;
			}else{
				cfg->host = strdup(argv[i+1]);
			}
			i++;
#ifdef WITH_TLS
		}else if(!strcmp(argv[i], "--insecure")){
			cfg->insecure = true;
#endif
		}else if(!strcmp(argv[i], "-i") || !strcmp(argv[i], "--id")){
			if(cfg->id_prefix){
				fprintf(stderr, "Error: -i and -I argument cannot be used together.\n\n");
				return 1;
			}
			if(i==argc-1){
				fprintf(stderr, "Error: -i argument given but no id specified.\n\n");
				return 1;
			}else{
				cfg->id = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "-I") || !strcmp(argv[i], "--id-prefix")){
			if(cfg->id){
				fprintf(stderr, "Error: -i and -I argument cannot be used together.\n\n");
				return 1;
			}
			if(i==argc-1){
				fprintf(stderr, "Error: -I argument given but no id prefix specified.\n\n");
				return 1;
			}else{
				cfg->id_prefix = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "-k") || !strcmp(argv[i], "--keepalive")){
			if(i==argc-1){
				fprintf(stderr, "Error: -k argument given but no keepalive specified.\n\n");
				return 1;
			}else{
				cfg->keepalive = atoi(argv[i+1]);
				if(cfg->keepalive>65535){
					fprintf(stderr, "Error: Invalid keepalive given: %d\n", cfg->keepalive);
					return 1;
				}
			}
			i++;
#ifdef WITH_TLS
		}else if(!strcmp(argv[i], "--key")){
			if(i==argc-1){
				fprintf(stderr, "Error: --key argument given but no file specified.\n\n");
				return 1;
			}else{
				cfg->keyfile = strdup(argv[i+1]);
			}
			i++;
#endif
		}else if(!strcmp(argv[i], "-l") || !strcmp(argv[i], "--stdin-line")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			if(cfg->pub_mode != MSGMODE_NONE){
				fprintf(stderr, "Error: Only one type of message can be sent at once.\n\n");
				return 1;
			}else{
				cfg->pub_mode = MSGMODE_STDIN_LINE;
			}
		}else if(!strcmp(argv[i], "-m") || !strcmp(argv[i], "--message")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			if(cfg->pub_mode != MSGMODE_NONE){
				fprintf(stderr, "Error: Only one type of message can be sent at once.\n\n");
				return 1;
			}else if(i==argc-1){
				fprintf(stderr, "Error: -m argument given but no message specified.\n\n");
				return 1;
			}else{
				cfg->message = strdup(argv[i+1]);
				cfg->msglen = strlen(cfg->message);
				cfg->pub_mode = MSGMODE_CMD;
			}
			i++;
		}else if(!strcmp(argv[i], "-M")){
			if(i==argc-1){
				fprintf(stderr, "Error: -M argument given but max_inflight not specified.\n\n");
				return 1;
			}else{
				cfg->max_inflight = atoi(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "-n") || !strcmp(argv[i], "--null-message")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			if(cfg->pub_mode != MSGMODE_NONE){
				fprintf(stderr, "Error: Only one type of message can be sent at once.\n\n");
				return 1;
			}else{
				cfg->pub_mode = MSGMODE_NULL;
			}
		}else if(!strcmp(argv[i], "-V") || !strcmp(argv[i], "--protocol-version")){
			if(i==argc-1){
				fprintf(stderr, "Error: --protocol-version argument given but no version specified.\n\n");
				return 1;
			}else{
				if(!strcmp(argv[i+1], "mqttv31")){
					cfg->protocol_version = MQTT_PROTOCOL_V31;
				}else if(!strcmp(argv[i+1], "mqttv311")){
					cfg->protocol_version = MQTT_PROTOCOL_V311;
				}else{
					fprintf(stderr, "Error: Invalid protocol version argument given.\n\n");
					return 1;
				}
				i++;
			}
#ifdef WITH_SOCKS
		}else if(!strcmp(argv[i], "--proxy")){
			if(i==argc-1){
				fprintf(stderr, "Error: --proxy argument given but no proxy url specified.\n\n");
				return 1;
			}else{
				if(mosquitto__parse_socks_url(cfg, argv[i+1])){
					return 1;
				}
				i++;
			}
#endif
#ifdef WITH_TLS_PSK
		}else if(!strcmp(argv[i], "--psk")){
			if(i==argc-1){
				fprintf(stderr, "Error: --psk argument given but no key specified.\n\n");
				return 1;
			}else{
				cfg->psk = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--psk-identity")){
			if(i==argc-1){
				fprintf(stderr, "Error: --psk-identity argument given but no identity specified.\n\n");
				return 1;
			}else{
				cfg->psk_identity = strdup(argv[i+1]);
			}
			i++;
#endif
		}else if(!strcmp(argv[i], "-q") || !strcmp(argv[i], "--qos")){
			if(i==argc-1){
				fprintf(stderr, "Error: -q argument given but no QoS specified.\n\n");
				return 1;
			}else{
				cfg->qos = atoi(argv[i+1]);
				if(cfg->qos<0 || cfg->qos>2){
					fprintf(stderr, "Error: Invalid QoS given: %d\n", cfg->qos);
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "--quiet")){
			cfg->quiet = true;
		}else if(!strcmp(argv[i], "-r") || !strcmp(argv[i], "--retain")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			cfg->retain = 1;
		}else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--stdin-file")){
			if(pub_or_sub == CLIENT_SUB){
				goto unknown_option;
			}
			if(cfg->pub_mode != MSGMODE_NONE){
				fprintf(stderr, "Error: Only one type of message can be sent at once.\n\n");
				return 1;
			}else{ 
				cfg->pub_mode = MSGMODE_STDIN_FILE;
			}
#ifdef WITH_SRV
		}else if(!strcmp(argv[i], "-S")){
			cfg->use_srv = true;
#endif
		}else if(!strcmp(argv[i], "-t") || !strcmp(argv[i], "--topic")){
			if(i==argc-1){
				fprintf(stderr, "Error: -t argument given but no topic specified.\n\n");
				return 1;
			}else{
				if(pub_or_sub == CLIENT_PUB){
					if(mosquitto_pub_topic_check(argv[i+1]) == MOSQ_ERR_INVAL){
						fprintf(stderr, "Error: Invalid publish topic '%s', does it contain '+' or '#'?\n", argv[i+1]);
						return 1;
					}
					cfg->topic = strdup(argv[i+1]);
				}else{
					if(mosquitto_sub_topic_check(argv[i+1]) == MOSQ_ERR_INVAL){
						fprintf(stderr, "Error: Invalid subscription topic '%s', are all '+' and '#' wildcards correct?\n", argv[i+1]);
						return 1;
					}
					cfg->topic_count++;
					cfg->topics = realloc(cfg->topics, cfg->topic_count*sizeof(char *));
					cfg->topics[cfg->topic_count-1] = strdup(argv[i+1]);
				}
				i++;
			}
		}else if(!strcmp(argv[i], "-T") || !strcmp(argv[i], "--filter-out")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}
			if(i==argc-1){
				fprintf(stderr, "Error: -T argument given but no topic filter specified.\n\n");
				return 1;
			}else{
				if(mosquitto_sub_topic_check(argv[i+1]) == MOSQ_ERR_INVAL){
					fprintf(stderr, "Error: Invalid filter topic '%s', are all '+' and '#' wildcards correct?\n", argv[i+1]);
					return 1;
				}
				cfg->filter_out_count++;
				cfg->filter_outs = realloc(cfg->filter_outs, cfg->filter_out_count*sizeof(char *));
				cfg->filter_outs[cfg->filter_out_count-1] = strdup(argv[i+1]);
			}
			i++;
#ifdef WITH_TLS
		}else if(!strcmp(argv[i], "--tls-version")){
			if(i==argc-1){
				fprintf(stderr, "Error: --tls-version argument given but no version specified.\n\n");
				return 1;
			}else{
				cfg->tls_version = strdup(argv[i+1]);
			}
			i++;
#endif
		}else if(!strcmp(argv[i], "-u") || !strcmp(argv[i], "--username")){
			if(i==argc-1){
				fprintf(stderr, "Error: -u argument given but no username specified.\n\n");
				return 1;
			}else{
				cfg->username = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "-P") || !strcmp(argv[i], "--pw")){
			if(i==argc-1){
				fprintf(stderr, "Error: -P argument given but no password specified.\n\n");
				return 1;
			}else{
				cfg->password = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "--will-payload")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-payload argument given but no will payload specified.\n\n");
				return 1;
			}else{
				cfg->will_payload = strdup(argv[i+1]);
				cfg->will_payloadlen = strlen(cfg->will_payload);
			}
			i++;
		}else if(!strcmp(argv[i], "--will-qos")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-qos argument given but no will QoS specified.\n\n");
				return 1;
			}else{
				cfg->will_qos = atoi(argv[i+1]);
				if(cfg->will_qos < 0 || cfg->will_qos > 2){
					fprintf(stderr, "Error: Invalid will QoS %d.\n\n", cfg->will_qos);
					return 1;
				}
			}
			i++;
		}else if(!strcmp(argv[i], "--will-retain")){
			cfg->will_retain = true;
		}else if(!strcmp(argv[i], "--will-topic")){
			if(i==argc-1){
				fprintf(stderr, "Error: --will-topic argument given but no will topic specified.\n\n");
				return 1;
			}else{
				if(mosquitto_pub_topic_check(argv[i+1]) == MOSQ_ERR_INVAL){
					fprintf(stderr, "Error: Invalid will topic '%s', does it contain '+' or '#'?\n", argv[i+1]);
					return 1;
				}
				cfg->will_topic = strdup(argv[i+1]);
			}
			i++;
		}else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--disable-clean-session")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}
			cfg->clean_session = false;
		}else if(!strcmp(argv[i], "-N")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}
			cfg->eol = false;
		}else if(!strcmp(argv[i], "-R")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}
			cfg->no_retain = true;
		}else if(!strcmp(argv[i], "-v") || !strcmp(argv[i], "--verbose")){
			if(pub_or_sub == CLIENT_PUB){
				goto unknown_option;
			}
			cfg->verbose = 1;
		}else{
			goto unknown_option;
		}
	}

	return MOSQ_ERR_SUCCESS;

unknown_option:
	fprintf(stderr, "Error: Unknown option '%s'.\n",argv[i]);
	return 1;
}

int client_opts_set(struct mosquitto *mosq, struct mosq_config *cfg)
{
	int rc;

	if(cfg->will_topic && mosquitto_will_set(mosq, cfg->will_topic,
				cfg->will_payloadlen, cfg->will_payload, cfg->will_qos,
				cfg->will_retain)){

		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting will.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(cfg->username && mosquitto_username_pw_set(mosq, cfg->username, cfg->password)){
		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting username and password.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
#ifdef WITH_TLS
	if((cfg->cafile || cfg->capath)
			&& mosquitto_tls_set(mosq, cfg->cafile, cfg->capath, cfg->certfile, cfg->keyfile, NULL)){

		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
	if(cfg->insecure && mosquitto_tls_insecure_set(mosq, true)){
		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting TLS insecure option.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
#  ifdef WITH_TLS_PSK
	if(cfg->psk && mosquitto_tls_psk_set(mosq, cfg->psk, cfg->psk_identity, NULL)){
		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting TLS-PSK options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
#  endif
	if(cfg->tls_version && mosquitto_tls_opts_set(mosq, 1, cfg->tls_version, cfg->ciphers)){
		if(!cfg->quiet) fprintf(stderr, "Error: Problem setting TLS options.\n");
		mosquitto_lib_cleanup();
		return 1;
	}
#endif
	mosquitto_max_inflight_messages_set(mosq, cfg->max_inflight);
#ifdef WITH_SOCKS
	if(cfg->socks5_host){
		rc = mosquitto_socks5_set(mosq, cfg->socks5_host, cfg->socks5_port, cfg->socks5_username, cfg->socks5_password);
		if(rc){
			mosquitto_lib_cleanup();
			return rc;
		}
	}
#endif
	mosquitto_opts_set(mosq, MOSQ_OPT_PROTOCOL_VERSION, &(cfg->protocol_version));
	return MOSQ_ERR_SUCCESS;
}

int client_id_generate(struct mosq_config *cfg, const char *id_base)
{
	int len;
	char hostname[256];

	if(cfg->id_prefix){
		cfg->id = malloc(strlen(cfg->id_prefix)+10);
		if(!cfg->id){
			if(!cfg->quiet) fprintf(stderr, "Error: Out of memory.\n");
			mosquitto_lib_cleanup();
			return 1;
		}
		snprintf(cfg->id, strlen(cfg->id_prefix)+10, "%s%d", cfg->id_prefix, getpid());
	}else if(!cfg->id){
		hostname[0] = '\0';
		gethostname(hostname, 256);
		hostname[255] = '\0';
		len = strlen(id_base) + strlen("/-") + 6 + strlen(hostname);
		cfg->id = malloc(len);
		if(!cfg->id){
			if(!cfg->quiet) fprintf(stderr, "Error: Out of memory.\n");
			mosquitto_lib_cleanup();
			return 1;
		}
		snprintf(cfg->id, len, "%s/%d-%s", id_base, getpid(), hostname);
		if(strlen(cfg->id) > MOSQ_MQTT_ID_MAX_LENGTH){
			/* Enforce maximum client id length of 23 characters */
			cfg->id[MOSQ_MQTT_ID_MAX_LENGTH] = '\0';
		}
	}
	return MOSQ_ERR_SUCCESS;
}

int client_connect(struct mosquitto *mosq, struct mosq_config *cfg)
{
	char err[1024];
	int rc;

#ifdef WITH_SRV
	if(cfg->use_srv){
		rc = mosquitto_connect_srv(mosq, cfg->host, cfg->keepalive, cfg->bind_address);
	}else{
		rc = mosquitto_connect_bind(mosq, cfg->host, cfg->port, cfg->keepalive, cfg->bind_address);
	}
#else
	rc = mosquitto_connect_bind(mosq, cfg->host, cfg->port, cfg->keepalive, cfg->bind_address);
#endif
	if(rc>0){
		if(!cfg->quiet){
			if(rc == MOSQ_ERR_ERRNO){
#ifndef WIN32
				strerror_r(errno, err, 1024);
#else
				FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errno, 0, (LPTSTR)&err, 1024, NULL);
#endif
				fprintf(stderr, "Error: %s\n", err);
			}else{
				fprintf(stderr, "Unable to connect (%s).\n", mosquitto_strerror(rc));
			}
		}
		mosquitto_lib_cleanup();
		return rc;
	}
	return MOSQ_ERR_SUCCESS;
}

#ifdef WITH_SOCKS
/* Convert %25 -> %, %3a, %3A -> :, %40 -> @ */
static int mosquitto__urldecode(char *str)
{
	int i, j;
	int len;
	if(!str) return 0;

	if(!strchr(str, '%')) return 0;

	len = strlen(str);
	for(i=0; i<len; i++){
		if(str[i] == '%'){
			if(i+2 >= len){
				return 1;
			}
			if(str[i+1] == '2' && str[i+2] == '5'){
				str[i] = '%';
				len -= 2;
				for(j=i+1; j<len; j++){
					str[j] = str[j+2];
				}
				str[j] = '\0';
			}else if(str[i+1] == '3' && (str[i+2] == 'A' || str[i+2] == 'a')){
				str[i] = ':';
				len -= 2;
				for(j=i+1; j<len; j++){
					str[j] = str[j+2];
				}
				str[j] = '\0';
			}else if(str[i+1] == '4' && str[i+2] == '0'){
				str[i] = ':';
				len -= 2;
				for(j=i+1; j<len; j++){
					str[j] = str[j+2];
				}
				str[j] = '\0';
			}else{
				return 1;
			}
		}
	}
	return 0;
}

static int mosquitto__parse_socks_url(struct mosq_config *cfg, char *url)
{
	char *str;
	int i;
	char *username = NULL, *password = NULL, *host = NULL, *port = NULL;
	char *username_or_host = NULL;
	int start;
	int len;
	bool have_auth = false;
	int port_int;

	if(!strncmp(url, "socks5h://", strlen("socks5h://"))){
		str = url + strlen("socks5h://");
	}else{
		fprintf(stderr, "Error: Unsupported proxy protocol: %s\n", url);
		return 1;
	}

	// socks5h://username:password@host:1883
	// socks5h://username:password@host
	// socks5h://username@host:1883
	// socks5h://username@host
	// socks5h://host:1883
	// socks5h://host

	start = 0;
	for(i=0; i<strlen(str); i++){
		if(str[i] == ':'){
			if(i == start){
				goto cleanup;
			}
			if(have_auth){
				/* Have already seen a @ , so this must be of form
				 * socks5h://username[:password]@host:port */
				if(host){
					/* Already seen a host, must be malformed. */
					goto cleanup;
				}
				len = i-start;
				host = malloc(len + 1);
				memcpy(host, &(str[start]), len);
				host[len] = '\0';
				start = i+1;
			}else if(!username_or_host){
				/* Haven't seen a @ before, so must be of form
				 * socks5h://host:port or
				 * socks5h://username:password@host[:port] */
				len = i-start;
				username_or_host = malloc(len + 1);
				memcpy(username_or_host, &(str[start]), len);
				username_or_host[len] = '\0';
				start = i+1;
			}
		}else if(str[i] == '@'){
			if(i == start){
				goto cleanup;
			}
			have_auth = true;
			if(username_or_host){
				/* Must be of form socks5h://username:password@... */
				username = username_or_host;
				username_or_host = NULL;

				len = i-start;
				password = malloc(len + 1);
				memcpy(password, &(str[start]), len);
				password[len] = '\0';
				start = i+1;
			}else{
				/* Haven't seen a : yet, so must be of form
				 * socks5h://username@... */
				if(username){
					/* Already got a username, must be malformed. */
					goto cleanup;
				}
				len = i-start;
				username = malloc(len + 1);
				memcpy(username, &(str[start]), len);
				username[len] = '\0';
				start = i+1;
			}
		}
	}

	/* Deal with remainder */
	if(i > start){
		len = i-start;
		if(host){
			/* Have already seen a @ , so this must be of form
			 * socks5h://username[:password]@host:port */
			port = malloc(len + 1);
			memcpy(port, &(str[start]), len);
			port[len] = '\0';
		}else if(username_or_host){
			/* Haven't seen a @ before, so must be of form
			 * socks5h://host:port */
			host = username_or_host;
			username_or_host = NULL;
			port = malloc(len + 1);
			memcpy(port, &(str[start]), len);
			port[len] = '\0';
		}else{
			host = malloc(len + 1);
			memcpy(host, &(str[start]), len);
			host[len] = '\0';
		}
	}

	if(!host){
		fprintf(stderr, "Error: Invalid proxy.\n");
		goto cleanup;
	}

	if(mosquitto__urldecode(username)){
		goto cleanup;
	}
	if(mosquitto__urldecode(password)){
		goto cleanup;
	}
	if(port){
		port_int = atoi(port);
		if(port_int < 1 || port_int > 65535){
			fprintf(stderr, "Error: Invalid proxy port %d\n", port_int);
			goto cleanup;
		}
		free(port);
	}else{
		port_int = 1080;
	}

	cfg->socks5_username = username;
	cfg->socks5_password = password;
	cfg->socks5_host = host;
	cfg->socks5_port = port_int;

	return 0;
cleanup:
	if(username_or_host) free(username_or_host);
	if(username) free(username);
	if(password) free(password);
	if(host) free(host);
	if(port) free(port);
	return 1;
}

#endif
