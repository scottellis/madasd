/*
 * madasd daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <getopt.h>

#include "utility.h"

#define DEFAULT_CONTROL_PORT 6000

static void control_loop(int control_sock);
static void client_handler(int c_sock);
static int do_start(int c_sock);
static int do_stop(int c_sock);
static int do_status(int c_sock);
static void sig_handler(int sig);
static int add_sig_handlers();

volatile int shutdown_event;

int control_port;
int debug_mode;


int state;

void usage(char *argv_0)
{
	printf("Usage: %s -p<port>\n", argv_0);
	printf("  -p     control listener port, data will be port + 1\n");
	printf("  -d     debug mode, do NOT daemonize\n");
	exit(1);
}

void parse_args(int argc, char **argv)
{
	int opt;

	control_port = DEFAULT_CONTROL_PORT;
	debug_mode = 0;

	while ((opt = getopt(argc, argv, "p:d")) != -1) {
		switch (opt) {
		case 'p':
			control_port = strtoul(optarg, NULL, 0);

			if (control_port < 1 || control_port > 65534) {
				printf("Invalid control port: %s\n", optarg);
				usage(argv[0]);
			}

			break;

		case 'd':
			debug_mode = 1;
			break;

		default:
			printf("Unknown option\n");
			usage(argv[0]);
			break;
		}
	}
}

int main(int argc, char **argv)
{
	parse_args(argc, argv);

	openlog(argv[0], LOG_CONS | LOG_PERROR, LOG_DAEMON);

	add_sig_handlers();

	int control_sock = start_listener(control_port);

	if (control_sock < 0) {
		syslog(LOG_ERR, "Failed to open control socket");
		exit(1);
	}

	syslog(LOG_INFO, "control listening on port %d\n", control_port);

	control_loop(control_sock);

	syslog(LOG_INFO, "control listener closed\n");

	closelog();

	return 0;
}

void control_loop(int control_sock)
{
	struct sockaddr_in c_addr_in;
	socklen_t c_len;
	int c_sock;
	char c_ip[INET_ADDRSTRLEN + 8];

	while (!shutdown_event) {
		c_len = sizeof(c_addr_in);
		c_sock = accept(control_sock, (struct sockaddr *) &c_addr_in, &c_len);

		if (c_sock < 0) {
			if (!shutdown_event)
				syslog(LOG_WARNING, "accept: %m\n");
		}
		else {
			memset(c_ip, 0, sizeof(c_ip));
			inet_ntop(AF_INET, &c_addr_in.sin_addr, c_ip, INET_ADDRSTRLEN);
			client_handler(c_sock);
			close(c_sock);
		}
	}
}

void client_handler(int c_sock)
{
	char rx[128];

	memset(rx, 0, sizeof(rx));

	if (read_cmd(c_sock, rx, sizeof(rx) - 1, 2000) < 0)
		return;

	if (!strncmp("start", rx, 5))
		do_start(c_sock);
	else if (!strncmp("stop", rx, 4))
		do_stop(c_sock);
	else if (!strncmp("status", rx, 6))
		do_status(c_sock);
	else
		send_response(c_sock, "invalid command");
}

int do_start(int c_sock)
{
	return send_response(c_sock, "ok");
}

int do_stop(int c_sock)
{
	return send_response(c_sock, "ok");
}

int do_status(int c_sock)
{
	return send_response(c_sock, "ok");
}

void sig_handler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM)
		shutdown_event = 1;
}

int add_sig_handlers()
{
	struct sigaction sia;

	memset(&sia, 0, sizeof(sia));

	sia.sa_handler = sig_handler;

	if (sigaction(SIGINT, &sia, NULL) < 0) {
		syslog(LOG_ERR, "sigaction(SIGINT): %m\n");
		return -1;
	}

	if (sigaction(SIGTERM, &sia, NULL) < 0) {
		syslog(LOG_ERR, "sigaction(SIGTERM): %m\n");
		return -1;
	}

	return 0;
}
