#ifndef __qt_serial_h
#define __qt_serial_h

#include <stdio.h>
#include <time.h>
#include <sys/types.h>

#define QT_MODEL_UNKNOWN 0
#define QT_MODEL_100     1
#define QT_MODEL_150     2
#define QT_MODEL_FUJI    3
#define QT_MODEL_DC50    4
#define QT_MODEL_SIERRA  5

#define CAM_FEATURES        0
#define CAM_WAKEUP          1
#define CAM_SET_SPEED       2
#define CAM_SET_CAMERA_NAME 3
#define CAM_SET_CAMERA_TIME 4
#define CAM_GET_INFORMATION 5
#define CAM_SET_QUALITY     6
#define CAM_SET_FLASH       7
#define CAM_TAKE_PICTURE    8
#define CAM_GET_PICTURE     9
#define CAM_GET_THUMBNAIL   10
#define CAM_DELETE_PICTURES 11
#define CAM_GET_FILENAME    12
#define CAM_THUMB_HISTOGRAM 13
#define CAM_THUMB_LOAD_DATA 14
#define CAM_GET_QUALITY_STR 15
#define CAM_GET_FLASH_STR   16

#define CAM_CAN_SET_CAMERA_NAME 0x01
#define CAM_CAN_SET_CAMERA_TIME 0x02
#define CAM_CAN_SET_QUALITY     0x04
#define CAM_CAN_SET_FLASH       0x08
#define CAM_CAN_TAKE_PICTURE    0x10
#define CAM_CAN_GET_THUMBNAIL   0x20
#define CAM_CAN_DELETE_PICTURES 0x40

#ifdef __CC65__
#define CamSpeed uint8
#else
#define CamSpeed int
#define SER_BAUD_9600   B9600
#define SER_BAUD_19200  B19200
#define SER_BAUD_57600  B57600
#define SER_BAUD_115200 B115200
#define SER_PAR_EVEN    PARENB
#define SER_PAR_NONE    0
#endif

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
  char name[32];
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
#if (BLOCK_SIZE*BUFFER_BLOCKS) != BUFFER_SIZE
#error "Wrong buffer size defined"
#endif
extern unsigned char buffer[BUFFER_SIZE];

extern uint8 serial_model;
extern char *cam_file_extension[];
/* Camera interface functions, protocol-agnostic */
uint8 cam_serial_connect(CamSpeed speed);

/* Helper functions */
const char *cam_get_quality_str(uint8 is_pic, uint8 mode);
const char *cam_get_flash_str(uint8 is_pic, uint8 mode);

/* Callbacks */
extern uint16 cam_features;
uint8 cam_wakeup(CamSpeed speed);
uint8 cam_set_speed(CamSpeed speed);
uint8 cam_set_camera_name(const char *name);
uint8 cam_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
uint8 cam_get_information(void);
uint8 cam_set_quality(uint8 quality);
uint8 cam_set_flash(uint8 mode);
uint8 cam_take_picture(void);
uint8 cam_get_picture(uint8 n_pic, int fd, off_t avail);
uint8 cam_get_thumbnail(uint8 n_pic, int fd);
uint8 cam_delete_pictures(void);
void cam_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Camera thumbnail functions */
void cam_thumb_histogram(void);
void cam_load_thumb_data(uint8 line);

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
