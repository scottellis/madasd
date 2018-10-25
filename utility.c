/*
 * Support functions
 */

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

int msleep(int ms)
{
	struct timespec ts;

	if (ms < 1)
		return -1;

	ts.tv_sec = ms / 1000;
	ts.tv_nsec = 1000000 * (ms % 1000);

	return nanosleep(&ts, NULL);
}

int read_cmd(int sock, char *buff, int maxlen, int timeout)
{
	int ret = -1;
	int pos = 0;
	int elapsed = 0;

	do {
		// a byte at a time, our commands are short
		int len = read(sock, buff + pos, 1);

		if (len < 0) {
			// client closed socket
			syslog(LOG_WARNING, "cmd read: read: %m\n");
			break;
		}
		else if (len == 0) {
			ret = msleep(50);

			if (ret < 0) {
				syslog(LOG_WARNING, "cmd read: nanosleep: %m");
				break;
			}

			elapsed += 50;

			if (elapsed > timeout)
				syslog(LOG_WARNING, "cmd read: timed out\n");
		}
		else {
			if (buff[pos] == '\n') {
				buff[pos] = 0;
				ret = pos;
				break;
			}

			if (pos++ >= (maxlen - 1)) {
				syslog(LOG_WARNING, "cmd read: %d bytes without newline\n", pos);
				break;
			}
		}
	} while (elapsed <= timeout);

	return ret;
}

int send_response(int sock, const char *str)
{
	int len;
	int sent;
	int total;

	if (!str || !*str)
		return 0;

	total = strlen(str);

	sent = 0;

	while (sent < total) {
		len = write(sock, str + sent, total - sent);

		if (len <= 0) {
			syslog(LOG_ERR, "send_response write: %m\n");
			return -1;
		}

		sent += len;
	}

	if (str[total - 1] != '\n') {
		if (write(sock, "\n", 1) != 1) {
			syslog(LOG_WARNING, "send_response write(2): %m\n");
			return -1;
		}
	}

	return 0;
}

int start_listener(int port)
{
	int s, flags;
	struct sockaddr_in addr;

	if ((s = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		syslog(LOG_ERR, "socket: %m\n");
		return -1;
	}

	flags = 1;

	if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &flags, sizeof(flags)) < 0) {
		syslog(LOG_ERR, "setsockopt(SO_REUSEADDR): %m\n");
		close(s);
		return -1;
	}
/*
	if (setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, &flags, sizeof(flags)) < 0) {
		syslog(LOG_ERR, "setsockopt(SO_KEEPALIVE): %m\n");
		close(s);
		return -1;
	}
*/
	memset(&addr, 0, sizeof(addr));

	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);

	if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
		syslog(LOG_ERR, "bind: %m\n");
		close(s);
		return -1;
	}

	// only one client at a time
	if (listen(s, 1) < 0) {
		syslog(LOG_ERR, "listen: %m\n");
		close(s);
		return -1;
	}

	return s;
}
