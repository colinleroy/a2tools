#ifndef __qt_200_serial_h
#define __qt_200_serial_h

/* Connection functions */
uint8 qt200_wakeup(void);
uint8 qt200_set_speed(uint16 speed);

/* Camera settings functions */
uint8 qt200_get_information(uint8 *num_pics, uint8 *left_pics, uint8 *quality_mode, uint8 *flash_mode, uint8 *battery_level, uint8 *charging, char **name, struct tm *time);

/* Camera pictures functions */
uint8 qt200_get_picture(uint8 n_pic, const char *filename);

#endif
