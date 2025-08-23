#ifndef __qt_serial_h
#define __qt_serial_h

#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#define QT_MODEL_UNKNOWN 0
#define QT_MODEL_100 100
#define QT_MODEL_150 150
#define QT_MODEL_200 200

#define QUALITY_STANDARD 0x20
#define QUALITY_HIGH 0x10
#define FLASH_AUTO 0
#define FLASH_OFF 1
#define FLASH_ON 2

extern uint8 camera_connected;
typedef struct _camera_date {
  uint16 year;
  uint8 month;
  uint8 day;
  uint8 hour;
  uint8 minute;
} camera_date;

typedef struct _camera_info {
  uint8 num_pics;
  uint8 left_pics;
  uint8 quality_mode;
  uint8 flash_mode;
  uint8 battery_level;
  uint8 charging;
  char *name;
  camera_date date;
} camera_info;

typedef struct _thumb_info {
  uint8 quality_mode;
  uint8 flash_mode;
  camera_date date;
} thumb_info;

/* Communication buffer */
#define BLOCK_SIZE 512
#define BUFFER_BLOCKS 4
#define BUFFER_SIZE (BLOCK_SIZE*BUFFER_BLOCKS)
extern unsigned char buffer[BUFFER_SIZE];
extern uint8 serial_model;

/* Camera interface functions, protocol-agnostic */
uint8 qt_serial_connect(uint16 speed);
uint8 qt_get_information(camera_info *info);
uint8 qt_get_picture(uint8 n_pic, int fd, off_t avail);

uint8 qt_get_thumbnail(uint8 n_pic, int fd, thumb_info *info);
uint8 qt_delete_pictures(void);
uint8 qt_take_picture(void);

uint8 qt_set_camera_name(const char *name);
uint8 qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
uint8 qt_set_quality(uint8 quality);
uint8 qt_set_flash(uint8 mode);

/* Helper functions */
const char *qt_get_quality_str(uint8 mode);
const char *qt_get_flash_str(uint8 mode);

/* Debug helpers */
#ifndef __CC65__
extern FILE *dbgfp;

  #define DUMP_START(name) do {       \
    if (dbgfp) fclose(dbgfp);         \
    dbgfp = fopen(name".dump", "wb"); \
  } while (0)
  #define DUMP_DATA(buf,size) do {    \
    fwrite(buf, 1, size, dbgfp);      \
  } while (0)
  #define DUMP_END() do {             \
    fclose(dbgfp);                    \
    dbgfp = NULL;                     \
  } while (0)

#else
  /* Debug disabled on target apple2 */
  #if 0
  uint16 dump_counter;
    #define DUMP_START(name) printf("\n%s :", name)
    #define DUMP_DATA(buf,size)  do {    \
      for (dump_counter = 0; dump_counter < size; dump_counter++) \
        printf("%02x ", (unsigned char) buf[dump_counter]);       \
    } while (0)
    #define DUMP_END() printf("\n")
  #else
    #define DUMP_START(name)
    #define DUMP_DATA(buf,size)
    #define DUMP_END()
  #endif
#endif

#endif
