#ifndef ADS127X_H
#define ADS127X_H

#define ADS_BLOCKSIZE 4096

int ads_read(unsigned char *blocks, int num_blocks);
int ads_read_file(const char *filename, unsigned char *blocks, int num_blocks);
int ads_start();
int ads_stop();
void ads_dump_stats();

#endif /* ifndef ADS127X_H */
