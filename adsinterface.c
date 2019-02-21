#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>
#include <syslog.h>
#include <stdint.h>

#include "utility.h"
#include "adsinterface.h"

int device_fd;

int ads_open_device()
{
	int fd = open("/dev/ads127x", O_RDWR);

	if (fd < 0)
		syslog(LOG_WARNING, "Error opening driver: %m\n");

	return fd;
}

int get_ioc_value(int fd, int ioc)
{
	long val;
	long result = ioctl(fd, ioc, &val);

	if (result < 0) {
		syslog(LOG_WARNING, "Driver ioctl error: %m\n");
		return -1;
	}

	return (int) val;
}

int ads_start()
{
	if (!device_fd) {
		device_fd = ads_open_device();

		if (device_fd < 0) {
			device_fd = 0;
			return -1;
		}
	}

	return (5 == write(device_fd, "start", 5));
}

int ads_stop()
{
	if (!device_fd) {
		device_fd = ads_open_device();

		if (device_fd < 0) {
			device_fd = 0;
			return -1;
		}
	}

	return (4 == write(device_fd, "stop", 4));
}

int ads_read(unsigned char *blocks, int num_blocks)
{
	int retries, len;
	int blocks_read;

	if (!device_fd) {
		device_fd = ads_open_device();

		if (device_fd < 0) {
			device_fd = 0;
			return -1;
		}
	}

	retries = 0;
	blocks_read = 0;

	while (retries < 5 && blocks_read < (num_blocks + 1)) {
		// leave room for timestamp header block at front
		len = read(device_fd,
				blocks + ((blocks_read + 1) * ADS_BLOCKSIZE),
				((num_blocks + 1) - blocks_read) * ADS_BLOCKSIZE);

		if (len < 0) {
			syslog(LOG_WARNING, "Driver read error ret = %d: %m\n", len);
			return len;
		}

		if (len > 0)
			blocks_read += len / ADS_BLOCKSIZE;

		retries++;
		msleep(50);
	}

	// move header block to front
	if (blocks_read == (num_blocks + 1))
		memcpy(blocks, blocks + (num_blocks * ADS_BLOCKSIZE), ADS_BLOCKSIZE);

	return blocks_read;
}

/*
 * ================================================================
 * Code below this point is used only when data comes from a file.
 *
 * Primarily used for testing clients with known/repeatable data.
 * ================================================================
 */

static int data_block_pos;
static int data_num_blocks;
#define MAX_TIMESTAMPS (ADS_BLOCKSIZE / 8)
#define SAMPLE_RATE_NS 32000
static uint64_t data_timestamps[MAX_TIMESTAMPS];
static uint64_t data_last_timestamp;
unsigned char *file_data;
char *loaded_filename;

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
	data_last_timestamp = 0;
	memset(data_timestamps, 0, sizeof(data_timestamps));

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

		// leave room at the front for the timestamp header block
		memcpy(blocks + ((copied + 1) * ADS_BLOCKSIZE),
			file_data + (data_block_pos * ADS_BLOCKSIZE),
			count * ADS_BLOCKSIZE);

		copied += count;
		data_block_pos += count;

		if (data_block_pos >= data_num_blocks)
			data_block_pos = 0;
	}

	for (int i = 0; i < num_blocks; i++) {
		data_timestamps[i] = data_last_timestamp;
		data_last_timestamp += SAMPLE_RATE_NS;
	}

	// timestamp header block at the front
	memcpy(blocks, data_timestamps, ADS_BLOCKSIZE);

	// at 8 MHz (DEFAULT_CLKDIV = 12)
	// 32 blocks = 31.25 us * 128 samples/block * 32 = 128 ms
	// fake a delay the real driver will incur
	msleep(128);

	return num_blocks + 1;
}
