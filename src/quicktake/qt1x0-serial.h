#ifndef __qt_1x0_serial_h
#define __qt_1x0_serial_h

/* Connection functions */
uint8 qt1x0_wakeup(uint16 speed);
uint8 qt1x0_set_speed(uint16 speed);

/* Camera settings functions */
uint8 qt1x0_set_camera_name(const char *name);
uint8 qt1x0_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
uint8 qt1x0_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, char **name, struct tm *time);
uint8 qt1x0_set_quality(uint8 quality);
uint8 qt1x0_set_flash(uint8 mode);

/* Camera pictures functions */
uint8 qt1x0_take_picture(void);
uint8 qt1x0_get_picture(uint8 n_pic, const char *filename);
uint8 qt1x0_get_thumbnail(uint8 n_pic, uint8 *quality, uint8 *flash, uint8 *year, uint8 *month, uint8 *day, uint8 *hour, uint8 *minute);
uint8 qt1x0_delete_pictures(void);

#endif
