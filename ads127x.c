#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

#include "ads127x.h"

int ads_read(unsigned char *blocks, int num_blocks)
{
	static char byte = 'A';

	for (int i = 0; i < num_blocks; i++) {
		memset(blocks + (i * ADS_BLOCKSIZE), byte, ADS_BLOCKSIZE);
		byte++;

		if (byte > 'Z')
			byte = 'A';
	}

	return num_blocks;
}

/*
  Not trying to be robust here. If the file doesn't have enough data
  to fulfill the request we fail.
*/
int ads_read_file(const char *filename, unsigned char *blocks, int num_blocks)
{
	int ret = -1;

	if (!filename || !*filename)
		return -1;

	int fd = open(filename, O_RDONLY);

	if (fd < 0) {
		syslog(LOG_WARNING, "ads_read_file: open: %m\n");
		return -1;
	}

	int len = read(fd, blocks, num_blocks * ADS_BLOCKSIZE);

	if (len == (num_blocks * ADS_BLOCKSIZE))
		ret = num_blocks;

	close(fd);

	return ret;
}
