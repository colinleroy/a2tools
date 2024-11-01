#ifndef __vsdrive_h
#define __vsdrive_h

typedef enum {
  VSDRIVE_WRITE      = 0,
  VSDRIVE_READ       = 1,
  
  VSDRIVE_PRODOS_24  = 24,
  VSDRIVE_PRODOS_25  = 25,

  VSDRIVE_BLOCK_SIZE = 512
} VSDriveValues;

void handle_vsdrive_request(void);
void vsdrive_read_config(void);

#endif
