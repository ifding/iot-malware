/*
MQTT is licensed under Creative Commons 4 -- http://creativecommons.org/licenses/by/4.0/
http://mqtt.org
*/
#include <err.h>
#include <getopt.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <mosquitto.h>


struct userdata {
	char **username;
	char **topics;
	size_t topic_count;
	int command_argc;
	int verbose;
	char **command_argv;
	int qos;
};

void log_cb(struct mosquitto *mosq, void *obj, int level, const char *str)
{
	printf("%s\n", str);
}


//void hax_message_cb(struct mosquitto *mosq, void *obj,
//					const struct mosquitto_message *msg)
//{
//	struct userdata *ud = (struct userdata *) obj;
//
//	if (msg->payloadlen || ud->verbose) {
//		if (ud->command_argv && fork() == 0) {
//			if (ud->verbose)
//				ud->command_argv[ud->command_argc - 2] = msg->topic;
//			ud->command_argv[ud->command_argc - 1] =
//				msg->payloadlen ? msg->payload : NULL;
//
//			FILE *pubme = popen(ud->command_argv + " 2>&1", "r");
//
//			mosquitto_publish(mosq, mid, pubtopic, sizeof(pubme), pubme, qos,
//							  retain);
//			`perror(ud->command_argv[0]);
//			_exit(1);
//		}
//	}
//}




void message_cb(struct mosquitto *mosq, void *obj,
				const struct mosquitto_message *msg)
{
	struct userdata *ud = (struct userdata *) obj;

	if (msg->payloadlen || ud->verbose) {
		if (ud->command_argv && fork() == 0) {
			if (ud->verbose)
				ud->command_argv[ud->command_argc - 2] = msg->topic;
			ud->command_argv[ud->command_argc - 1] =
				msg->payloadlen ? msg->payload : NULL;
			execv(ud->command_argv[0], ud->command_argv);
			perror(ud->command_argv[0]);
			_exit(1);
		}
	}
}

void connect_cb(struct mosquitto *mosq, void *obj, int result)
{
	struct userdata *ud = (struct userdata *) obj;

	fflush(stderr);
	if (result == 0) {
		size_t i;

		for (i = 0; i < ud->topic_count; i++)
			mosquitto_subscribe(mosq, NULL, ud->topics[i], ud->qos);
	} else {
		fprintf(stderr, "%s\n", mosquitto_connack_string(result));
	}
}

int mqtte_usage(int retcode)
{
	int major, minor, rev;

	mosquitto_lib_version(&major, &minor, &rev);
	printf("mqtt-exec - execute command on mqtt messages\n"
		   "libmosquitto version: %d.%d.%d\n"
		   "\n"
		   "mqtte_usage: mqtt-exec [ARGS...] -t TOPIC ... -- CMD [CMD ARGS...]\n"
		   "\n"
		   "options:\n"
		   " -c,--disable-clean-session  Disable the 'clean session' flag\n"
		   " -d,--debug                  Enable debugging\n"
		   " -h,--host HOST              Connect to HOST. Default is localhost\n"
		   " -i,--id ID                  The id to use for this client\n"
		   " -u,--username USERNAME      The username for the client\n"
		   " -x,--password PASSWORD      The password for the client\n"
		   " -k,--keepalive SEC          Set keepalive to SEC. Default is 60\n"
		   " -p,--port PORT              Set TCP port to PORT. Default is 1883\n"
		   " -q,--qos QOS                Set Quality of Serive to level. Default is 0\n"
		   " -t,--topic TOPIC            Set MQTT topic to TOPIC. May be repeated\n"
		   " -v,--verbose                Pass over the topic to application as firs arg\n"
		   " --will-topic TOPIC          Set the client Will topic to TOPIC\n"
		   " --will-payload MSG          Set the client Will message to MSG\n"
		   " --will-qos QOS              Set the QoS level for client Will message\n"
		   " --will-retain               Make the client Will retained\n"
		   "\n", major, minor, rev);
	return retcode;
}

static int perror_ret(const char *msg)
{
	perror(msg);
	return 1;
}

static int valid_qos_range(int qos, const char *type)
{
	if (qos >= 0 && qos <= 2)
		return 1;

	fprintf(stderr, "%d: %s out of range\n", qos, type);
	return 0;
}

int mqtte_main(int argc, char *argv[])
{
	static struct option opts[] = {
		{"disable-clean-session", no_argument, 0, 'c'},
		{"debug", no_argument, 0, 'd'},
		{"host", required_argument, 0, 'h'},
		{"id", required_argument, 0, 'i'},
		{"keepalive", required_argument, 0, 'k'},
		{"port", required_argument, 0, 'p'},
		{"qos", required_argument, 0, 'q'},
		{"topic", required_argument, 0, 't'},
		{"verbose", no_argument, 0, 'v'},
		{"username", required_argument, 0, 'u'},
		{"password", required_argument, 0, 'x'},
		{"will-topic", required_argument, 0, 0x1001},
		{"will-payload", required_argument, 0, 0x1002},
		{"will-qos", required_argument, 0, 0x1003},
		{"will-retain", no_argument, 0, 0x1004},
		{0, 0, 0, 0}
	};
	int debug = 0;
	bool clean_session = true;
	const char *host = "localhost";
	int port = 1883;
	int keepalive = 60;
	int i, c, rc = 1;
	struct userdata ud;
	char hostname[256];
	static char id[MOSQ_MQTT_ID_MAX_LENGTH + 1];
	static char username[MOSQ_MQTT_ID_MAX_LENGTH + 1];
	static char password[MOSQ_MQTT_ID_MAX_LENGTH + 1];
	struct mosquitto *mosq = NULL;

	char *will_payload = NULL;
	int will_qos = 0;
	bool will_retain = false;
	char *will_topic = NULL;

	memset(&ud, 0, sizeof(ud));

	memset(hostname, 0, sizeof(hostname));
	memset(id, 0, sizeof(id));
	memset(username, 0, sizeof(username));
	memset(password, 0, sizeof(password));

	while ((c =
			getopt_long(argc, argv, "cdh:i:u:x:k:p:q:t:v", opts, &i)) != -1) {
		switch (c) {
		case 'c':
			clean_session = false;
			break;
		case 'd':
			debug = 1;
			break;
		case 'h':
			host = optarg;
			break;
		case 'i':
			if (strlen(optarg) > MOSQ_MQTT_ID_MAX_LENGTH) {
				fprintf(stderr, "specified id is longer than %d chars\n",
						MOSQ_MQTT_ID_MAX_LENGTH);
				return 1;
			}
			strncpy(id, optarg, sizeof(id) - 1);
			break;
		case 'k':
			keepalive = atoi(optarg);
			break;
		case 'p':
			port = atoi(optarg);
			break;
		case 'q':
			ud.qos = atoi(optarg);
			if (!valid_qos_range(ud.qos, "QoS"))
				return 1;
			break;

		case 'u':
			if (strlen(optarg) > MOSQ_MQTT_ID_MAX_LENGTH) {
				fprintf(stderr,
						"specified username is longer than %d chars\n",
						MOSQ_MQTT_ID_MAX_LENGTH);
				return 1;
			}
			strncpy(username, optarg, sizeof(username) - 1);
			break;
		case 'x':
			if (strlen(optarg) > MOSQ_MQTT_ID_MAX_LENGTH) {
				fprintf(stderr,
						"specified password is longer than %d chars\n",
						MOSQ_MQTT_ID_MAX_LENGTH);
				return 1;
			}
			strncpy(password, optarg, sizeof(password) - 1);
			break;
		case 't':
			ud.topic_count++;
			ud.topics = realloc(ud.topics, sizeof(char *) * ud.topic_count);
			ud.topics[ud.topic_count - 1] = optarg;
			break;
		case 'v':
			ud.verbose = 1;
			break;
		case 0x1001:
			will_topic = optarg;
			break;
		case 0x1002:
			will_payload = optarg;
			break;
		case 0x1003:
			will_qos = atoi(optarg);
			if (!valid_qos_range(will_qos, "will QoS"))
				return 1;
			break;
		case 0x1004:
			will_retain = 1;
			break;
		case '?':
			return mqtte_usage(1);
		}
	}

	if ((ud.topics == NULL) || (optind == argc))
		return mqtte_usage(1);

	ud.command_argc = (argc - optind) + 1 + ud.verbose;
	ud.command_argv = malloc((ud.command_argc + 1) * sizeof(char *));
	if (ud.command_argv == NULL)
		return perror_ret("malloc");

	for (i = 0; i <= ud.command_argc; i++)
		ud.command_argv[i] = optind + i < argc ? argv[optind + i] : NULL;

	if (id[0] == '\0') {
		/* generate an id */
		gethostname(hostname, sizeof(hostname) - 1);
		snprintf(id, sizeof(id), "mqttexe/%x-%s", getpid(), hostname);
	}

	mosquitto_lib_init();
	mosq = mosquitto_new(id, clean_session, &ud);
	if (mosq == NULL)
		return perror_ret("mosquitto_new");

	if (debug) {
		printf
			("username=%\nshost=%s:%d\nid=%s\ntopic_count=%zu\ncommand=%s\n",
			 username, host, port, id, ud.topic_count, ud.command_argv[0]);
		mosquitto_log_callback_set(mosq, log_cb);
	}

	if (will_topic && mosquitto_will_set(mosq, will_topic,
										 will_payload ? strlen(will_payload) :
										 0, will_payload, will_qos,
										 will_retain)) {
		fprintf(stderr, "Failed to set will\n");
		goto cleanup;
	}

	mosquitto_username_pw_set(mosq, username, password);

	mosquitto_connect_callback_set(mosq, connect_cb);
	mosquitto_message_callback_set(mosq, message_cb);

	/* let kernel reap the children */
	signal(SIGCHLD, SIG_IGN);

	rc = mosquitto_connect(mosq, host, port, keepalive);
	if (rc != MOSQ_ERR_SUCCESS) {
		if (rc == MOSQ_ERR_ERRNO)
			return perror_ret("mosquitto_connect_bind");
		fprintf(stderr, "Unable to connect (%d)\n", rc);
		goto cleanup;
	}

	rc = mosquitto_loop_forever(mosq, -1, 1);

  cleanup:
	mosquitto_destroy(mosq);
	mosquitto_lib_cleanup();
	return rc;

}
