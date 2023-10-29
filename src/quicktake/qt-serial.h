#ifndef __qt_serial_h
#define __qt_serial_h

#include <time.h>

uint8 qt_serial_connect(void);
void qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *mode, char **name, struct tm *time);
void qt_get_picture(uint8 n_pic, const char *filename, uint8 full);
void qt_delete_pictures(void);

void qt_set_camera_name(const char *name);
void qt_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);

const char *qt_get_mode_str(uint8 mode);

#endif