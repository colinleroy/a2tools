#ifndef __qt_edit_image_h
#define __qt_edit_image_h

#include "platform.h"

void qt_convert_image(const char *filename);
uint8 qt_view_image(const char *filename);
void finish_img_view(void);
void get_program_disk(void);

void qt_edit_image(const char *ofname, uint16 p_width, uint8 serial_model);

void hgr_print(void);
#endif
