#include <string.h>

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
