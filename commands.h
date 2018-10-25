#ifndef COMMANDS_H
#define COMMANDS_H

#define CMD_STATUS 0
#define CMD_START 1
#define CMD_STOP 2

int command_id(const char *cmdstr);
int dispatch_command(int c_sock, int cmd, char *args);

#endif /* COMMANDS_H */
