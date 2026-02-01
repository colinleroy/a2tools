#ifndef __smartport_h
#define __smartport_h

/* Documentation from
 * Calls to the Protocol Converter, http://www.apple-iigs.info/doc/fichiers/iicrom35b.pdf,
 * Smartport TechNote #4, https://prodos8.com/docs/technote/smartport/04/
 */

typedef enum {
  SP_DEVICE_STATUS         = 0x00,
  SP_DEVICE_CONTROL_BLOCK,
  SP_NEWLINE_STATUS,
  SP_DEVICE_INFORMATION_BLOCK,

  SP_UNIDISK_35_STATUS    = 0x05
} smartport_status_code;

typedef enum {
  SP_OK         = 0x00,
  SP_BADCMD     = 0x01,
  SP_BADPCNT    = 0x04,
  SP_BUSERR     = 0x06,
  SP_BADUNIT    = 0x11,
  SP_BADCTL     = 0x21,
  SP_BADCTLPARM = 0x22,
  SP_IOERROR    = 0x27,
  SP_NODRIVE    = 0x28,
  SP_NOWRITE    = 0x2B,
  SP_BADBLOCK   = 0x2D,
  SP_OFFLINE    = 0x2F
} smartport_error;

typedef enum {
  SP_RAMDISK    = 0x00,
  SP_35DISK     = 0x01,
  SP_PROFILEHD  = 0x02,
  SP_GENSCSI    = 0x03,
  SP_ROMDISK    = 0x04,
  SP_SCSICDROM  = 0x05,
  SP_SCSITAPE   = 0x06,
  SP_SCSIHD     = 0x07,

  SP_SCSIPRINTER= 0x09,
  SP_525DISK    = 0x0A,
  SP_PRINTER    = 0x0D,
  SP_CLOCK      = 0x0E,
  SP_MODEM      = 0x0F
} smartport_device_type;

typedef enum {
  SP_IS_BLOCK_DEVICE = 1<<7,
  SP_WRITE_ALLOWED   = 1<<6,
  SP_READ_ALLOWED    = 1<<5,
  SP_IS_ONLINE       = 1<<4,
  SP_FORMAT_FORBIDDEN= 1<<3,
  SP_WRITE_UNPROT    = 1<<2,
  SP_INTERRUPTING    = 1<<1,
  SP_IS_OPEN         = 1<<0
} smartport_device_flags;

typedef struct {
  union {
    struct {
      smartport_device_flags flags;
      unsigned char nblocks_low;
      unsigned char nblocks_mid;
      unsigned char nblocks_hi;
    } device_status;
    struct {
      unsigned char unit_count;
      unsigned char interrupt_status;
      unsigned char reserved[6];
    } controller_status;
    struct {
      unsigned char block_len;
      unsigned char control_block[1]; /* Maximum 255 */
    } device_control_block;
    struct {
      smartport_device_flags flags;
      unsigned char nblocks_low;
      unsigned char nblocks_mid;
      unsigned char nblocks_hi;
      unsigned char name_len;
      unsigned char name[16];
      smartport_device_type type;
      unsigned char subtype;
      unsigned int version;
    } device_information_block;
    struct {
      unsigned char reserved;
      unsigned char error;
      unsigned char retries;
      unsigned char reserved2;
      unsigned char a;
      unsigned char x;
      unsigned char y;
      unsigned char p;
    } unidisk35_status;
  };
} smartport_status_response;

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
smartport_error __fastcall__ smartport_get_status(unsigned char slot, unsigned char unit,
                                                  unsigned char status_type,
                                                  smartport_status_response *buffer);

/* Returns the unit's storage size in bytes, from a STATUS DIB response buffer */
unsigned long smartport_unit_size(smartport_status_response *buffer);

/* Returns the unit's storage name, from a STATUS DIB response buffer */
const char *smartport_unit_name(smartport_status_response *buffer);

#endif
