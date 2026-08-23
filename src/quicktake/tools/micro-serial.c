#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>

#include "platform.h"
#include "simple_serial.h"
#include "cameras/qt-serial.h"
#include "cameras/qt-thumbs.h"

extern void *qt1x0_callbacks[];
extern void *fuji_callbacks[];
extern void *dc50_callbacks[];
extern void *sierra_callbacks[];
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
typedef void  (*impl_get_filename_func)(uint8 n_pic, char *dirname, char *filename);
typedef void  (*impl_thumb_histogram_func)(void);
typedef void  (*impl_thumb_load_data_func)(uint8 line);
typedef const char * (*impl_get_quality_str_func)(uint8 mode);
typedef const char * (*impl_get_flash_str_func)(uint8 mode);

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
impl_get_filename_func    impl_get_filename;
impl_thumb_histogram_func impl_thumb_histogram;
impl_thumb_load_data_func impl_thumb_load_data;
impl_get_quality_str_func impl_get_quality_str;
impl_get_flash_str_func   impl_get_flash_str;

camera_info info;

unsigned char buffer[BUFFER_SIZE];
int scrw, scrh;
uint8 do_debug = 0;
int is_iigs = 1;

uint16 histogram[256];
uint8 opt_histogram[256];
uint8 thumb_buf[THUMB_WIDTH * 2];
int ifd;

static void setup_pointers(void *callbacks[]) {
  cam_features         = (unsigned long)callbacks[CAM_FEATURES];
  impl_wakeup          = callbacks[CAM_WAKEUP];
  impl_set_speed       = callbacks[CAM_SET_SPEED];
  impl_set_camera_name = callbacks[CAM_SET_CAMERA_NAME];
  impl_set_camera_time = callbacks[CAM_SET_CAMERA_TIME];
  impl_get_information = callbacks[CAM_GET_INFORMATION];
  impl_set_quality     = callbacks[CAM_SET_QUALITY];
  impl_set_flash       = callbacks[CAM_SET_FLASH];
  impl_take_picture    = callbacks[CAM_TAKE_PICTURE];
  impl_get_picture     = callbacks[CAM_GET_PICTURE];
  impl_get_thumbnail   = callbacks[CAM_GET_THUMBNAIL];
  impl_delete_pictures = callbacks[CAM_DELETE_PICTURES];
  impl_get_quality_str = callbacks[CAM_GET_QUALITY_STR];
  impl_get_flash_str   = callbacks[CAM_GET_FLASH_STR];
  impl_get_filename    = callbacks[CAM_GET_FILENAME];
}

static void list_images(void) {
  uint8 i;
  char filename[64];

  for (i = 1; i <= info.num_pics; i++) {
    cam_get_filename(i, NULL, filename);
    printf("%d: %s\n", i, filename);
  }
}

int main(int argc, char *argv[]) {
  char filename[64];

  do_debug = getenv("DEBUG") != NULL;

  if (argc < 3) {
    printf("Usage: %s camera_model tty_path [command] [command parameter]\n"
           "\n"
           "       camera_models: qt1x0, fuji, dc50, sierra\n"
           "       tty_path:      /dev/ttyUSB0 for example\n"
           "       command:       get (parameter: picture number)\n\n"
           "                      thumb (parameter: picture number)\n\n"
           "                      set_name (parameter: new name)\n\n",
           argv[0]);
    exit(1);
  }
  if (!strcmp(argv[1], "qt1x0")) {
    setup_pointers(qt1x0_callbacks);
  } else if (!strcmp(argv[1], "fuji")) {
    setup_pointers(fuji_callbacks);
  } else if (!strcmp(argv[1], "dc50")) {
    setup_pointers(dc50_callbacks);
  } else if (!strcmp(argv[1], "sierra")) {
    setup_pointers(sierra_callbacks);
  } else {
    printf("Unknown model '%s'\n", argv[1]);
    exit(1);
  }
  opt_tty_path = strdup(argv[2]);
  opt_tty_speed = B9600;

  printf("cam features: %0b\n", cam_features);

  printf("Connecting...\n");
  if (cam_serial_connect(SER_BAUD_115200) != 0) {
    printf("Failed.\n");
    exit(1);
  }

  printf("Connected. Getting information...\n");
get_info:
  if (cam_get_information(&info) != 0) {
    printf("Failure.\n");
  }
  printf("  Pictures taken: %d\n"
         "  Pictures left:  %d\n"
         "  Quality mode:   %d (%s)\n"
         "  Flash mode:     %d (%s)\n"
         "  Battery level:  %d%%\n"
         "  Is charging:    %d\n"
         "  Name:           '%s'\n"
         "  Date:           %02d/%02d/%04d %02d:%02d\n",
         info.num_pics,
         info.left_pics,
         info.quality_mode, cam_get_quality_str(info.quality_mode),
         info.flash_mode, cam_get_flash_str(info.flash_mode),
         info.battery_level,
         info.charging,
         info.name,
         info.date.day, info.date.month, info.date.year,
         info.date.hour, info.date.minute);

  if (argc > 3) {
    /* handle command */
    if ((!strcmp(argv[3], "get") || !strcmp(argv[3], "thumb")) && argc > 4) {
      char filename[64];
      uint8 n_pic = atoi(argv[4]);
      int fd;
      int thumb = !strcmp(argv[3], "thumb");

      cam_get_filename(n_pic, NULL, filename);
      if (thumb) {
        strcat(filename, ".thumb");
      }

      printf("Saving picture %d to %s\n", n_pic, filename);

      fd = open(filename, O_WRONLY|O_CREAT, 00644);
      if (fd < 0) {
        printf("Can not open %s: %s\n", filename, strerror(errno));
      } else {
        int r;
        if (thumb) {
          r = cam_get_thumbnail(n_pic, fd, NULL);
        } else {
          r = cam_get_picture(n_pic, fd, (off_t)1024*1024*1024UL);
        }
        if (r != 0) {
          printf("Can not get picture: %s\n", strerror(errno));
        }
        close(fd);
      }
    }
    if (!strcmp(argv[3], "set_name") && argc > 4) {
      cam_set_camera_name(argv[4]);
      argc = 2;
      goto get_info;
    }
    if (!strcmp(argv[3], "list")) {
      list_images();
    }
    if (!strcmp(argv[3], "delete")) {
      cam_delete_pictures();
      argc = 2;
      goto get_info;
    }
    if (!strcmp(argv[3], "set_flash") && argc > 4) {
      cam_set_flash(atoi(argv[4]));
      argc = 2;
      goto get_info;
    }
    if (!strcmp(argv[3], "set_quality") && argc > 4) {
      cam_set_quality(atoi(argv[4]));
      argc = 2;
      goto get_info;
    }
    if (!strcmp(argv[3], "snap")) {
      cam_take_picture();
      argc = 2;
      goto get_info;
    }
    if (!strcmp(argv[3], "set_time")) {
      time_t now = time(NULL);
      struct tm *date = localtime(&now);

      cam_set_camera_time(date->tm_mday, date->tm_mon+1, (date->tm_year+1900)%100, 
                          date->tm_hour, date->tm_min, 00);
      argc = 2;
      goto get_info;
    }
  }
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

uint8 cam_get_information(camera_info *info) {
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

uint8 cam_get_picture(uint8 n_pic, int fd, off_t avail) {
  return impl_get_picture(n_pic, fd, avail);
}

uint8 cam_delete_pictures(void) {
  return impl_delete_pictures();
}

uint8 cam_get_thumbnail(uint8 n_pic, int fd, thumb_info *info) {
  return impl_get_thumbnail(n_pic, fd, info);
}

void cam_get_filename(uint8 n_pic, char *dirname, char *filename) {
  return impl_get_filename(n_pic, dirname, filename);
}

void cam_thumb_histogram(void) {
  return impl_thumb_histogram();
}

void cam_thumb_load_data (uint8 line) {
  return impl_thumb_load_data(line);
}

const char *cam_get_quality_str (uint8 mode) {
  return impl_get_quality_str(mode);
}

const char *cam_get_flash_str (uint8 mode) {
  return impl_get_flash_str(mode);
}
