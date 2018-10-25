#include <stdlib.h>

#include "utility.h"
#include "commands.h"

int start(int c_sock)
{
	return send_response(c_sock, "*** processing start");
}

int stop(int c_sock)
{
	return send_response(c_sock, "*** processing stop");
}

int status(int c_sock)
{
	return send_response(c_sock, "*** processing status");
}

int dispatch_command(int c_sock, int cmd, char *args)
{
	int ret = -1;

	switch(cmd) {
	case CMD_STATUS:
		ret = status(c_sock);
		break;

	case CMD_START:
		ret = start(c_sock);
		break;

	case CMD_STOP:
		ret = stop(c_sock);
		break;
	}

	return ret;
}

int command_id(const char *cmdstr)
{
	if (!strcmp(cmdstr, "status"))
		return CMD_STATUS;

	if (!strcmp(cmdstr, "start"))
		return CMD_START;

	if (!strcmp(cmdstr, "stop"))
		return CMD_STOP;

	return -1;
}

