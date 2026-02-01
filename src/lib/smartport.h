#ifndef __smartport_h
#define __smartport_h

#define SP_STATUS_NORMAL 0x00
#define SP_STATUS_DIB    0x03


/* Returns the first slot number that is a Smartport controller,
 * or INVALID_DEVICE if none is found. */
unsigned char getfirstsmartportslot(void);

/* Returns the next slot number that is a Smartport controller,
 * or INVALID_DEVICE if no more are found. */
unsigned char __fastcall__ getnextsmartportslot(unsigned char slot);

/* Returns the number of units controlled by the Smartport controller
 * in the given slot, or zero on failure. */
unsigned char __fastcall__ smartportgetunitcount(unsigned char slot);

/* Gets the status for a given Smartport slot/unit. 
 * Returns the raw response's buffer address, or NULL on failure. */ 
unsigned char * __fastcall__ smartport_get_status(unsigned char slot, unsigned char unit, unsigned char status_type);

unsigned long smartport_dev_size(unsigned char *dib_buffer);
char *smartport_dev_name(unsigned char *dib_buffer);

#endif
