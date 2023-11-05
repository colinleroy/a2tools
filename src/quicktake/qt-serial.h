#ifndef __qt_serial_h
#define __qt_serial_h

#include <time.h>

#define QT_MODEL_100 100
#define QT_MODEL_150 150
#define QT_MODEL_200 200

uint8 qt_serial_connect(uint16 speed);
uint8 qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, char **name, struct tm *time);
uint8 qt_get_picture(uint8 n_pic, const char *filename, uint8 full);
uint8 qt_delete_pictures(void);
uint8 qt_take_picture(void);

void qt_set_camera_name(const char *name);
void qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);

void write_qtk_header(FILE *fp, const char *pic_format);

const char *qt_get_mode_str(uint8 mode);
const char *qt_get_flash_str(uint8 mode);

#define BLOCK_SIZE 512
extern char buffer[BLOCK_SIZE];

#ifndef __CC65__
extern FILE *dbgfp;

  #define DUMP_START(name) do {       \
    dbgfp = fopen(name".dump", "wb"); \
  } while (0)
  #define DUMP_DATA(buf,size) do {    \
    fwrite(buf, 1, size, dbgfp);      \
  } while (0)
  #define DUMP_END() do {             \
    fclose(dbgfp);                    \
  } while (0)

#else

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
