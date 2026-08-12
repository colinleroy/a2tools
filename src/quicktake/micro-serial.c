#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include "platform.h"
#include "simple_serial.h"
#include "qt-serial.h"

extern void *qt1x0_callbacks[];
extern void *qt200_callbacks[];
extern char *opt_tty_path;
extern int opt_tty_speed;

typedef uint8 (*impl_wakeup_func)(CamSpeed speed);
typedef uint8 (*impl_set_speed_func)(CamSpeed speed);
typedef uint8 (*impl_set_camera_name_func)(const char *name);
typedef uint8 (*impl_set_camera_time_func)(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
typedef uint8 (*impl_get_information_func)(camera_info *info);
typedef uint8 (*impl_set_quality_func)(uint8 quality);
typedef uint8 (*impl_set_flash_func)(uint8 mode);
typedef uint8 (*impl_take_picture_func)(void);
typedef uint8 (*impl_get_picture_func)(uint8 n_pic, int fd, off_t avail);
typedef uint8 (*impl_get_thumbnail_func)(uint8 n_pic, int fd, thumb_info *info);
typedef uint8 (*impl_delete_pictures_func)(void);

uint8                     camera_connected;
uint16                    cam_features;
impl_wakeup_func          impl_wakeup;
impl_set_speed_func       impl_set_speed;
impl_set_camera_name_func impl_set_camera_name;
impl_set_camera_time_func impl_set_camera_time;
impl_get_information_func impl_get_information;
impl_set_quality_func     impl_set_quality;
impl_set_flash_func       impl_set_flash;
impl_take_picture_func    impl_take_picture;
impl_get_picture_func     impl_get_picture;
impl_get_thumbnail_func   impl_get_thumbnail;
impl_delete_pictures_func impl_delete_pictures;

unsigned char buffer[BUFFER_SIZE];
int scrw, scrh;
uint8 do_debug = 0;

static void setup_pointers(void *callbacks[]) {
  cam_features          = (unsigned long)callbacks[CAM_FEATURES];
  impl_wakeup           = callbacks[CAM_WAKEUP];
  impl_set_speed        = callbacks[CAM_SET_SPEED];
  impl_set_camera_name  = callbacks[CAM_SET_CAMERA_NAME];
  impl_set_camera_time  = callbacks[CAM_SET_CAMERA_TIME];
  impl_get_information  = callbacks[CAM_GET_INFORMATION];
  impl_set_quality      = callbacks[CAM_SET_QUALITY];
  impl_set_flash        = callbacks[CAM_SET_FLASH];
  impl_take_picture     = callbacks[CAM_TAKE_PICTURE];
  impl_get_picture      = callbacks[CAM_GET_PICTURE];
  impl_get_thumbnail    = callbacks[CAM_GET_THUMBNAIL];
  impl_delete_pictures  = callbacks[CAM_DELETE_PICTURES];
}

int main(int argc, char *argv[]) {
  if (argc < 4) {
    printf("Usage: %s [camera_model] [tty_path] [tty_speed]\n"
           "\n"
           "       camera_models: qt1x0, qt200\n\n",
           argv[0]);
    exit(1);
  }
  if (!strcmp(argv[1], "qt1x0")) {
    setup_pointers(qt1x0_callbacks);
  } else if (!strcmp(argv[1], "qt200")) {
    setup_pointers(qt200_callbacks);
  } else {
    printf("Unknown model '%s'\n", argv[1]);
    exit(1);
  }
  opt_tty_path = strdup(argv[2]);
  opt_tty_speed = B9600;

  printf("cam features: %0b\n", cam_features);

  printf("Connecting...\n");
  if (qt_serial_connect(SER_BAUD_9600) != 0) {
    printf("Failed.\n");
    exit(1);
  }

  printf("Connected.\n");

  return 0;
}

uint8 cam_wakeup(CamSpeed speed) {
  return impl_wakeup(speed);
}

uint8 cam_set_speed(CamSpeed speed) {
  return impl_set_speed(speed);
}

uint8 cam_set_camera_name(const char *name) {
  return impl_set_camera_name(name);
}

uint8 cam_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return impl_set_camera_time(day, month, year, hour, minute, second);
}

uint8 qt_get_information(camera_info *info) {
  return impl_get_information(info);
}

uint8 cam_set_quality(uint8 quality) {
  return impl_set_quality(quality);
}

uint8 cam_set_flash(uint8 mode) {
  return impl_set_flash(mode);
}

uint8 cam_take_picture(void) {
  return impl_take_picture();
}

uint8 qt_get_picture(uint8 n_pic, int fd, off_t avail) {
  return impl_get_picture(n_pic, fd, avail);
}

uint8 cam_delete_pictures(void) {
  return impl_delete_pictures();
}

uint8 cam_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  return impl_get_thumbnail(n_pic, fd, info);
}
