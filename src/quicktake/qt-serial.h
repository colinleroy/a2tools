#ifndef __qt_serial_h
#define __qt_serial_h

void qt_serial_connect(void);
void qt_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *mode, char **name);
void qt_get_picture(uint8 n_pic, const char *filename);
const char *qt_get_mode_str(uint8 mode);

#endif
