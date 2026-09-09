#ifndef PS350_READ_DIR_LIST_H
#define PS350_READ_DIR_LIST_H

#define DATABUF_SIZE 8192

#define FIRST_HEADER_SIZE 41
#define NEXT_HEADERS_SIZE 5
#define FOOTER_SIZE 3
#define DATA_SIZE_FIRST_BLOCK (PS350_PKT_LEN-FIRST_HEADER_SIZE-FOOTER_SIZE)
#define DATA_SIZE_NEXT_BLOCKS  (PS350_PKT_LEN-NEXT_HEADERS_SIZE-FOOTER_SIZE)
#define NUM_MULTIPACKETS ((DATABUF_SIZE-256)/292)
#define READ_BLOCK_MAX_SIZE (DATA_SIZE_FIRST_BLOCK + (NUM_MULTIPACKETS-1)*DATA_SIZE_NEXT_BLOCKS)
/* Camera sends in batches of 256 bytes anyway: the last packet is not full */
#define READ_BLOCK_SIZE (READ_BLOCK_MAX_SIZE-(READ_BLOCK_MAX_SIZE%256))


#include "platform.h"

uint8 ps350_read_dir_list(void);
uint8 ps350_read_file(char *databuf);

#endif
