/*
 * Support functions
 */

#ifndef UTILITY_H
#define UTILITY_H

#include <time.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>

int msleep(int ms);
int read_cmd(int sock, char *buff, int maxlen, int timeout);
int send_response(int sock, const char *str);
int send_binary(int sock, unsigned char *data, int size);
int start_listener(int port);

#endif /* UTILITY_H */
