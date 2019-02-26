#ifndef ADS127X_H
#define ADS127X_H

#include <stdint.h>

#define ADS_BLOCKSIZE 4096
#define ADS_HEADER_MAGIC 0x7ff77ff7

struct ads_block_header {
	uint32_t magic;
	uint32_t num_blocks;
        uint64_t timestamps[1];
};

int ads_read(unsigned char *blocks, int num_blocks);
int ads_read_file(const char *filename, unsigned char *blocks, int num_blocks);
int ads_start();
int ads_stop();
void ads_dump_stats();

#endif /* ifndef ADS127X_H */
