#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>

#include "ads127x.h"

static int data_block_pos;
static int data_num_blocks;
unsigned char *file_data;
char *loaded_filename;

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

int ads_file_loaded(const char *filename)
{
	if (loaded_filename
			&& !strcmp(loaded_filename, filename)
			&& data_num_blocks > 0
			&& data_block_pos < data_num_blocks) {
		return 1;
	}

	return 0;
}

int ads_init_file(const char *filename)
{
	struct stat st;

	if (ads_file_loaded(filename))
		return 0;

	if (file_data) {
		free(file_data);
		file_data = NULL;
		free(loaded_filename);
		loaded_filename = NULL;
		data_num_blocks = 0;
		data_block_pos = 0;
	}

	if (stat(filename, &st) == -1)
		return -1;

	if (st.st_size < (ADS_BLOCKSIZE * 32))
		return -1;

	if (st.st_size > (ADS_BLOCKSIZE * 1000))
		return -1;

	if ((st.st_size % ADS_BLOCKSIZE) != 0)
		return -1;

	file_data = malloc(st.st_size);

	if (!file_data)
		return -1;

	int fd = open(filename, O_RDONLY);

	int len = read(fd, file_data, st.st_size);

	close(fd);

	if (len != st.st_size) {
		free(file_data);
		file_data = NULL;
		return -1;
	}

	loaded_filename = strdup(filename);

	data_block_pos = 0;
	data_num_blocks = len / ADS_BLOCKSIZE;

	return 1;
}

void ads_dump_stats()
{
	static int once = 0;

	if (loaded_filename && !once) {
		syslog(LOG_WARNING, "loaded_filename: %s\n", loaded_filename);
		once = 1;
	}

	syslog(LOG_WARNING, "data_num_blocks: %d  data_block_pos: %d\n",
		data_num_blocks, data_block_pos);
}

int ads_read_file(const char *filename, unsigned char *blocks, int num_blocks)
{
	if (!filename || !*filename)
		return -1;

	if (ads_init_file(filename) < 0) {
		syslog(LOG_WARNING, "ads_read_file: init failed");
		return -1;
	}

	int copied = 0;

	while (copied < num_blocks) {
		int count = data_num_blocks - data_block_pos;

		if (count > (num_blocks - copied))
			count = num_blocks - copied;

		memcpy(blocks + (copied * ADS_BLOCKSIZE),
			file_data + (data_block_pos * ADS_BLOCKSIZE),
			count * ADS_BLOCKSIZE);

		copied += count;
		data_block_pos += count;

		if (data_block_pos >= data_num_blocks)
			data_block_pos = 0;
	}

	return num_blocks;
}
