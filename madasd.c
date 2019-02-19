/*
 * madasd daemon
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <errno.h>
#include <syslog.h>
#include <signal.h>
#include <time.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <getopt.h>
#include <pthread.h>

#include "adsinterface.h"
#include "utility.h"

#define DEFAULT_CONTROL_PORT 6000
#define BLOCKS_PER_READ 32

static void control_loop(int control_sock);
static void client_handler(int c_sock);
static int do_connect(int c_sock);
static int do_disconnect(int c_sock);
static int do_start(int c_sock);
static int do_stop(int c_sock);
static int do_status(int c_sock);
static void * data_thread_handler(void *param);
static void data_loop(int data_sock);
static void data_client_handler(int c_sock);
static void sig_handler(int sig);
static int add_sig_handlers();

volatile int shutdown_event;
volatile int disconnect_event;
volatile int running;

int control_port;
int daemon_mode;
int verbose;
int file_mode;
char data_file[512];
pthread_t data_thread;

void usage(char *argv_0)
{
	printf("Usage: %s [-p<port>][-f<file>][-d]\n", argv_0);
	printf("  -p     control listener port, data will be port + 1\n");
	printf("  -d     daemonize\n");
	printf("  -f     data file from real a capture\n");
	printf("  -v     verbose mode, enable some extra logging\n");
	exit(1);
}

void parse_args(int argc, char **argv)
{
	int opt;
	struct stat sb;

	control_port = DEFAULT_CONTROL_PORT;
	daemon_mode = 0;
	verbose = 0;
	memset(data_file, 0, sizeof(data_file));

	while ((opt = getopt(argc, argv, "p:f:dvh")) != -1) {
		switch (opt) {
		case 'p':
			control_port = strtoul(optarg, NULL, 0);

			if (control_port < 1 || control_port > 65534) {
				printf("Invalid control port: %s\n", optarg);
				usage(argv[0]);
			}

			break;

		case 'f':
			if (strlen(optarg) > 500) {
				printf("Path to data file too long\n");
				usage(argv[0]);
			}

			strcpy(data_file, optarg);
			stat(data_file, &sb);

			if (!S_ISREG(sb.st_mode)) {
				printf("Data file arg is not a regular file\n");
				usage(argv[0]);
			}

			if (sb.st_size < (ADS_BLOCKSIZE * BLOCKS_PER_READ)) {
				printf("Data file is not big enough\n");
				usage(argv[0]);
			}

			file_mode = 1;
			break;

		case 'd':
			daemon_mode = 1;
			break;

		case 'v':
			verbose = 1;
			break;

		case 'h':
			usage(argv[0]);
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

	if (daemon_mode) {
		if (daemon(0, 0)) {
			perror("daemon");
			exit(1);
		}
	}

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

			if (verbose) {
				inet_ntop(AF_INET, &c_addr_in.sin_addr, c_ip, INET_ADDRSTRLEN);
				syslog(LOG_INFO, "new client: %s\n", c_ip);
			}

			client_handler(c_sock);
			syslog(LOG_INFO, "client disconnect: %s\n", c_ip);
			close(c_sock);
		}
	}

	if (data_thread) {
		disconnect_event = 1;
		pthread_join(data_thread, NULL);
	}
}

void client_handler(int c_sock)
{
	char rx[128];
	int len;

	do_connect(c_sock);

	if (!data_thread)
		return;

	while (!shutdown_event) {
		memset(rx, 0, sizeof(rx));

		len = read_cmd(c_sock, rx, 32, 2000);

		if (len	< 0)
			return;

		if (len > 0) {
			if (!strcmp("start", rx)) {
				syslog(LOG_INFO, "start\n");
				do_start(c_sock);
			}
			else if (!strcmp("stop", rx)) {
				syslog(LOG_INFO, "stop\n");
				do_stop(c_sock);
			}
			else if (!strcmp("disconnect", rx)) {
				syslog(LOG_INFO, "disconnect\n");
				do_disconnect(c_sock);
				break;
			}
			else if (!strcmp("status", rx)) {
				syslog(LOG_INFO, "status\n");
				do_status(c_sock);
			}
			else {
				send_response(c_sock, "invalid command");
			}
		}
	}
}

int do_connect(int c_sock)
{
	disconnect_event = 0;
	running = 0;

	if (pthread_create(&data_thread, NULL, data_thread_handler, NULL)) {
		syslog(LOG_WARNING, "pthread_create: %m\n");
		data_thread = 0;
		return send_response(c_sock, "fail");
	}

	return send_response(c_sock, "ok");
}

int do_disconnect(int c_sock)
{
	running = 0;

	disconnect_event = 1;

	if (data_thread) {
		pthread_join(data_thread, NULL);
		data_thread = 0;
	}

	return send_response(c_sock, "ok");
}

int do_start(int c_sock)
{
	running = 1;

	return send_response(c_sock, "ok");
}

int do_stop(int c_sock)
{
	running = 0;

	return send_response(c_sock, "ok");
}

int do_status(int c_sock)
{
	if (running)
		return send_response(c_sock, "running");
	else
		return send_response(c_sock, "idle");
}

static void * data_thread_handler(void *param)
{
	int data_sock = start_listener(control_port + 1);

	if (data_sock < 0) {
		syslog(LOG_ERR, "Failed to open data socket");
		return NULL;
	}

	syslog(LOG_INFO, "data thread started: port: %d\n", control_port + 1);

	data_loop(data_sock);

	close(data_sock);

	syslog(LOG_INFO, "data thread stopped\n");

	return NULL;
}

void data_loop(int data_sock)
{
	struct sockaddr_in c_addr_in;
	socklen_t c_len;
	int c_sock;
	fd_set rset;
	struct timespec timeout;
	char c_ip[INET_ADDRSTRLEN + 8];

	timeout.tv_sec = 2;
	timeout.tv_nsec = 0;

	while (!disconnect_event) {
		FD_ZERO(&rset);
		FD_SET(data_sock, &rset);

		if (pselect(data_sock + 1, &rset, NULL, NULL, &timeout, NULL) < 0) {
			if (errno != EINTR)
				syslog(LOG_WARNING, "pselect: %m");
		}
		else if (FD_ISSET(data_sock, &rset)) {
			c_len = sizeof(c_addr_in);
			c_sock = accept(data_sock, (struct sockaddr *) &c_addr_in, &c_len);

			if (c_sock < 0) {
				if (!disconnect_event)
					syslog(LOG_WARNING, "accept: %m\n");
			}
			else {
				memset(c_ip, 0, sizeof(c_ip));

				if (verbose) {
					inet_ntop(AF_INET, &c_addr_in.sin_addr, c_ip, INET_ADDRSTRLEN);
					syslog(LOG_INFO, "new data client: %s\n", c_ip);
				}

				data_client_handler(c_sock);
				syslog(LOG_INFO, "data client disconnect: %s\n", c_ip);
				close(c_sock);
			}
		}
	}
}

void data_client_handler(int c_sock)
{
	int num_blocks;

	unsigned char *blocks = (unsigned char *) malloc(BLOCKS_PER_READ * ADS_BLOCKSIZE);

	if (!blocks)
		return;

	while (!disconnect_event) {
		if (running) {
			memset(blocks, 0, BLOCKS_PER_READ * ADS_BLOCKSIZE);

			if (file_mode)
				num_blocks = ads_read_file(data_file, blocks, BLOCKS_PER_READ);
			else
				num_blocks = ads_read(blocks, BLOCKS_PER_READ);

			if (num_blocks < 0) {
				syslog(LOG_WARNING, "error reading block data\n");
				break;
			}

			if (num_blocks > 0) {
				int sent = send_binary(c_sock, blocks, num_blocks * ADS_BLOCKSIZE);

				if (sent != (num_blocks * ADS_BLOCKSIZE)) {
					syslog(LOG_WARNING, "error sending binary data\n");
					break;
				}
			}

			msleep(100);
		}
		else {
			msleep(500);
		}
	}

	free(blocks);
}

void sig_handler(int sig)
{
	if (sig == SIGINT || sig == SIGTERM) {
		disconnect_event = 1;
		shutdown_event = 1;
	}
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
