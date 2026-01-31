#ifndef __smartport_h
#define __smartport_h

#define SP_STATUS_NORMAL 0x00
#define SP_STATUS_DIB    0x03

unsigned char __fastcall__ is_slot_smartport(unsigned char slot);
unsigned char * __fastcall__ smartport_get_status(unsigned char slot, unsigned char unit, unsigned char status_type);

unsigned long smartport_dev_size(unsigned char *dib_buffer);
unsigned char *smartport_dev_name(unsigned char *dib_buffer);

#endif
