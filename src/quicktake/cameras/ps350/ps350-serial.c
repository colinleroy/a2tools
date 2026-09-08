#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "a2_features.h"
#include "platform.h"
#include "extended_conio.h"
#include "progress_bar.h"
#include "simple_serial.h"
#include "ps350.h"
#include "ps350-read-dir-list.h"
#include "../qt-serial.h"
#include "../../decoders/qt-conv.h"
#include "../../ui/ui.h"

#pragma code-name(push, "PS350")
#pragma rodata-name(push, "PS350")
#pragma data-name(push, "PS350")
#pragma bss-name(push, "PS350")

uint8 command_packet[300];

/* Camera features */
#define ps350_features 0b0000000010000000
//                               ||||||||_ SET_CAMERA_NAME
//                               |||||||__ SET_CAMERA_TIME
//                               ||||||___ SET_QUALITY,
//                               |||||____ SET_FLASH,
//                               ||||_____ TAKE_PICTURE,
//                               |||______ GET_THUMBNAIL,
//                               ||_______ DELETE_PICTURES,
//                               |________ RESERVED,

/* Camera callbacks definitions */
static uint8 ps350_wakeup(CamSpeed speed);
static uint8 ps350_set_speed(CamSpeed speed);

/* Camera settings functions */
static uint8 ps350_get_information(void);

/* Camera pictures functions */
static uint8 ps350_get_picture(uint8 n_pic, int fd, off_t avail);
static uint8 ps350_get_thumbnail(uint8 n_pic, int fd);
static void ps350_get_filename(uint8 n_pic, char *dirname, char *filename);

/* Other functions, that this driver doesn't implement
 * but must exist and return -1
 */
static uint8 ps350_set_camera_name(const char *name);
static uint8 ps350_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second);
static uint8 ps350_set_quality(uint8 quality);
static uint8 ps350_set_flash(uint8 mode);
static uint8 ps350_take_picture(void);
static uint8 ps350_delete_pictures(void);

/* Camera thumbnail functions */
void ps350_thumb_histogram(void);
void ps350_load_thumb_data(uint8 line);

/* Modes strings */
static const char *ps350_get_quality_str(uint8 is_pic, uint8 mode);
static const char *ps350_get_flash_str(uint8 is_pic, uint8 mode);

/* Camera callbacks */
void *ps350_callbacks[] = {
  /* FEATURES */        (void *)ps350_features,
  /* WAKEUP */          ps350_wakeup,
  /* SET_SPEED */       ps350_set_speed,
  /* SET_CAMERA_NAME */ ps350_set_camera_name,
  /* SET_CAMERA_TIME */ ps350_set_camera_time,
  /* GET_INFORMATION */ ps350_get_information,
  /* SET_QUALITY */     ps350_set_quality,
  /* SET_FLASH */       ps350_set_flash,
  /* TAKE_PICTURE */    ps350_take_picture,
  /* GET_PICTURE */     ps350_get_picture,
  /* GET_THUMBNAIL */   ps350_get_thumbnail,
  /* DELETE_PICTURES */ ps350_delete_pictures,
  /* GET_FILENAME */    ps350_get_filename,
  /* THUMB_HISTOGRAM */ ps350_thumb_histogram,
  /* THUMB_LOAD_DATA */ ps350_load_thumb_data,
  /* GET_QUALITY_STR */ ps350_get_quality_str,
  /* GET_FLASH_STR */   ps350_get_flash_str,
};

#ifdef __CC65__
#define PC_DEBUG_BUFFER(op, str, len)
#define PC_DEBUG_PRINTF(...)
#else
#define PC_DEBUG_PRINTF(...) do { if (do_debug) printf(__VA_ARGS__); } while (0)

static void PC_DEBUG_BUFFER(char *op, const char *str, int len) {
  if (do_debug) {
    printf("%s:", op);
    for (int i = 0; i < len; i++) {
      printf("%s %02X", i%16 == 0 ? "\n":"", (uint8)str[i]);
    }
    printf("\n");
  }
}
#endif

extern camera_info cam_info;
extern thumb_info th_info;

uint8 count = 0;

/* We don't support sending packets longer than 255 */
static void ps350_prepare_packet(uint8 len) {
  bzero(command_packet, sizeof command_packet);
  command_packet[0]   = 0xC0;

  if (command_packet[PS350_TYPE_IDX] == CMD_ACK) {
    command_packet[1] = count;
  } else {
    command_packet[17] = count;
  }
  /* Length and total length */
  command_packet[3] = command_packet[13] = len & 0xFF;
  command_packet[4] = command_packet[14] = len >> 8;

  /* 0x01 = computer to camera, 0x02 vice versa ? */
  /* 0x60 = ?? */
  command_packet[11]             = 0x01;
  command_packet[12]             = 0x60;

  command_packet[299] = 0xC1;
}

uint8 ps350_send_packet(void) {
  unsigned short chksum = 0;
  unsigned short i;
  unsigned char j;
  unsigned char lastbit, xorbit;

  simple_serial_putc(0xC0);
  /* Compute checksum, excluding header, checksum and trailer */
  for (i = 1; i < PS350_PKT_LEN-3; i++) {
    unsigned char cur = command_packet[i];
    simple_serial_putc(cur);
    for (j = 8; j; j--) {
      lastbit = ((uint8)chksum) & 0x01;      /* remember if last bit was 1 */
      chksum >>= 1;                          /* shift 1 place */

      xorbit = (cur & 0x01)^lastbit;
      cur >>= 1;
      if (xorbit) {                          /* XOR if needed */
        chksum ^= 0x8408;
      }
    }
  }
  simple_serial_putc((uint8)chksum);
  simple_serial_putc((uint8)((chksum>>8)));
  simple_serial_putc(0xC1);
#ifndef __CC65__
  command_packet[PS350_CHK_IDX]   = chksum & 0xFF;
  command_packet[PS350_CHK_IDX+1] = chksum >> 8;
#endif
  PC_DEBUG_BUFFER("Sent: ", command_packet, PS350_PKT_LEN);
  return 0;
}

static uint8 ps350_send_ack(void) {
  PC_DEBUG_PRINTF("Sending ACK\n");
  ps350_prepare_packet(4);
  command_packet[PS350_TYPE_IDX] = CMD_ACK;
  count++;
  PC_DEBUG_PRINTF("ACK - count now %d\n", count);
  return ps350_send_packet();
}

static void ps350_send_ping(void) {
  ps350_prepare_packet(12);
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[9]              = CMD_CODE_PING;
  ps350_send_packet();
}

#ifndef __CC65__
/* On CC65, we'll just read and ignore the packet, checking nothing
 * more than we got 300 bytes. */
static uint8 ps350_get_ping_reply(void) {
  if (simple_serial_read_no_irq((char *)buffer+512, PS350_PKT_LEN)) {
    PC_DEBUG_BUFFER("Ping reply short read: ", buffer+512, PS350_PKT_LEN);
    return -1;
  }
  if (buffer[PS350_CMD_IDX+512] != CMD_CODE_PING_REPLY) {
    PC_DEBUG_BUFFER("Not Ping reply: ", buffer+512, PS350_PKT_LEN);
    return -1;
  }
  PC_DEBUG_BUFFER("Got Ping reply: ", buffer+512, PS350_PKT_LEN);
  return 0;
}

static uint8 ps350_get_eot(void) {
  PC_DEBUG_PRINTF("Getting EOT\n");
  /* EOTs are read at +512 to preserve the previous command's
   * output */
  if (simple_serial_read_no_irq((char *)buffer+512, PS350_PKT_LEN)) {
    PC_DEBUG_BUFFER("EOT short read: ", buffer+512, PS350_PKT_LEN);
    return -1;
  }
  if (buffer[PS350_TYPE_IDX+512] != CMD_EOT) {
    PC_DEBUG_BUFFER("Not EOT: ", buffer+512, PS350_PKT_LEN);
    return -1;
  }
  return 0;
}
#else
static uint8 ps350_read_ignore(void) {
  return simple_serial_read_no_irq((char *)buffer+512, PS350_PKT_LEN);
}
#define ps350_get_eot        ps350_read_ignore
#define ps350_get_ping_reply ps350_read_ignore
#endif

static uint8 ps350_read_packet(void) {
  return simple_serial_read_no_irq((char *)buffer, PS350_PKT_LEN);
}

uint8 ps350_get_eot_and_ack(void) {
  uint8 i = 0;
  if (ps350_get_eot() != 0) {
    return -1;
  }
  return ps350_send_ack();
}

#pragma warn(unused-param, push, off)
/* Wakeup and detect a Canon Powershot 350 camera
 * Returns 0 if successful, -1 otherwise
 */
static uint8 ps350_wakeup(CamSpeed speed) {
  uint8 c;
  cputs("Pinging Canon Powershot 350... ");

  simple_serial_set_speed(SER_BAUD_9600);
  simple_serial_set_parity(SER_PAR_NONE);

  /* Flush shit */
  simple_serial_send_break(200);
  while (!simple_serial_read_no_irq((char *)&c, 1));

  /* Tighter than building a packet and sending it */
  simple_serial_putc(0x00);
  simple_serial_putc(0x53);
#ifdef __CC65__
  __A__ = 0x55;
  __asm__("jsr %v", simple_serial_putc);
  __asm__("jsr %v", simple_serial_putc);
  __asm__("jsr %v", simple_serial_putc);
  __asm__("jsr %v", simple_serial_putc);
#else
  simple_serial_putc(0x55);
  simple_serial_putc(0x55);
  simple_serial_putc(0x55);
  simple_serial_putc(0x55);
#endif

  PC_DEBUG_PRINTF("Sending ping\n");

  if (ps350_read_packet() == 0) {
    PC_DEBUG_BUFFER("Initial reply: ", buffer, PS350_PKT_LEN);
  } else {
    PC_DEBUG_BUFFER("Initial reply shorter than expected: ", buffer, PS350_PKT_LEN);
    goto no_cam;
  }

  if (memcmp(buffer+INIT_REPLY_NAME_IDX, "ModelName=PowerShot 350", 23/* strlen("ModelName=PowerShot 350") */)) {
    goto no_cam;
  }

  if (ps350_get_eot_and_ack() != 0) {
    goto no_cam;
  }
  return QT_MODEL_PS350;

no_cam:
  return QT_MODEL_UNKNOWN;
}

#pragma warn(unused-param, pop)

static CamSpeed my_speed = SER_BAUD_9600;

/* Send the speed upgrade command */
static uint8 ps350_set_speed(CamSpeed speed) {
  uint8 tries = 3, ps350_speed, c;

  // speed = SER_BAUD_19200;
  if (speed == my_speed) {
    return 0;
  }

  switch (speed) {
  case SER_BAUD_9600:   ps350_speed = PS350_SPEED_9600;   break;
  case SER_BAUD_19200:  ps350_speed = PS350_SPEED_19200;  break;
  case SER_BAUD_57600:  ps350_speed = PS350_SPEED_57600;  break;
  case SER_BAUD_115200: ps350_speed = PS350_SPEED_115200; break;
  }

again:
  simple_serial_send_break(200);
  simple_serial_set_speed(SER_BAUD_9600);
  PC_DEBUG_PRINTF("Waiting for break\n");

  PC_DEBUG_PRINTF("Sending %02X %02X\n", 0x42, ps350_speed);
  simple_serial_putc(0x42);
  platform_msleep(500);
  simple_serial_putc(ps350_speed);
  if (simple_serial_read_no_irq((char*)&c, 1)) {
    if (tries--) {
      goto again;
    }
    cputs("No reply.\r\n");
    goto err_out;
  }

  PC_DEBUG_PRINTF("Upgrading speed\n");
  simple_serial_set_speed(speed);

  PC_DEBUG_PRINTF("Sending %02X %02X\n", 0x00, 0x53);
  simple_serial_putc(0x00);
  simple_serial_putc(0x53);
  platform_msleep(500);

  ps350_send_ping();
  if (ps350_get_ping_reply() != 0) {
    cputs("No reply\r\n");
    goto err_out;
  }

  return ps350_get_eot_and_ack();
err_out:
  return -1;
}

uint8 is_multi = 0;
static uint8 ps350_get_result(void) {
  if (ps350_read_packet() != 0) {
err_out:
    return -1;
  }
#ifndef __CC65__
  if (!is_multi) {
    if (buffer[PS350_TYPE_IDX] != command_packet[PS350_TYPE_IDX]
     || buffer[PS350_CMD_IDX] != command_packet[PS350_CMD_IDX]) {
      goto err_out;
    }
  }
  PC_DEBUG_BUFFER("Got reply: ", buffer, PS350_PKT_LEN);
#endif
  is_multi = buffer[PS350_LEN_IDX+1] & 0x80;
  buffer[PS350_LEN_IDX+1] &= ~0x80;
  if (is_multi) {
    PC_DEBUG_PRINTF("Multi-packet response\n");
  }
  return 0;
}

static uint8 ps350_send_command_and_get_result(void) {
  ps350_send_packet();

  if (ps350_get_result() != 0) {
    return -1;
  }
#ifndef __CC65__
  if (is_multi) {
    printf("Multi-packet not handled via ps350_send_command_and_get_result()\n");
    exit(1);
  }
#endif
  return ps350_get_eot_and_ack();
}

static char root_dir_name[64];
static char cur_dir_id[4];

static uint8 ps350_select_disk(void) {
  char *ptr;
  /* GET DISKS */
  ps350_prepare_packet(20);
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[PS350_CMD_IDX]  = CMD_CODE_GET_DISKS;
  command_packet[21]             = 0x1E;

  if (ps350_send_command_and_get_result() != 0) {
    return -1;
  }
  strcpy(root_dir_name, (char *)buffer+25);

  if (IS_NOT_NULL(ptr = strchr(root_dir_name, '/'))) {
    *ptr = '\0';
  }
  PC_DEBUG_PRINTF("Disk name %s\n", root_dir_name);

  /* USE DISK */
  ps350_prepare_packet(21+strlen(root_dir_name));
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[PS350_CMD_IDX]  = CMD_CODE_USE_DISK;
  command_packet[11]             = 0x21;
  command_packet[12]             = 0xA0;
  memcpy(command_packet+PS350_DATA_IDX, root_dir_name, strlen(root_dir_name));

  return ps350_send_command_and_get_result();
}

static uint8 get_dir_id(char *dir) {
  uint8 pkt_len;

  PC_DEBUG_PRINTF("Checking for %s\n", dir);
  pkt_len = strlen(dir) + 17;
  ps350_prepare_packet(pkt_len);
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[PS350_CMD_IDX]  = CMD_CODE_GET_DIR_PTR;
  command_packet[11]             = 0x21;
  command_packet[12]             = 0xA0;
  memcpy(command_packet+PS350_DATA_IDX, dir, strlen(dir));

  if (ps350_send_command_and_get_result() != 0) {
    goto err_out;
  }
  if (buffer[21] == 0x00 && buffer[24] == 0x00) {
    /* found directory */
    PC_DEBUG_PRINTF("Directory %s exists\n", dir);
    memcpy(cur_dir_id, buffer+25, 4);
    return 0;
  }
err_out:
  PC_DEBUG_PRINTF("Directory %s does not exist\n", dir);
  return -1;
}

uint8 num_subdirs = 0;
char ent_name[13];
uint32 ent_size;

/* Streaming directory list, as we have no time to split read/handling */
uint8 found_ent;
uint8 cur_ent;
extern uint8 read_to_get_ent;
extern uint8 ent_to_get;

static uint8 ps350_list_dir(uint8 *num_entries, uint8 get_ent) {
  uint8 had_multi = 0;

  if (!get_ent) {
    *num_entries = 0;
  }
  cur_ent = 0;
  read_to_get_ent = get_ent;
  ent_to_get = *num_entries;

  PC_DEBUG_PRINTF("Listing subdirectory %02X%02X%02X%02X\n",
                  cur_dir_id[0],cur_dir_id[1],cur_dir_id[2],cur_dir_id[3]);
  /* Prepare for dir download */
  ps350_prepare_packet(20);
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[PS350_CMD_IDX]  = CMD_CODE_PREPARE_GET_DIR_LIST;
  memcpy(command_packet+PS350_DATA_IDX, cur_dir_id, 4);
  if (ps350_send_command_and_get_result() != 0) {
err_out:
    return -1;
  }

  /* Dir download */
  ps350_prepare_packet(28);
  command_packet[PS350_TYPE_IDX] = CMD_PACKET;
  command_packet[PS350_CMD_IDX]  = CMD_CODE_GET_DIR_LIST;
  command_packet[PS350_CMD_IDX+2]= 0x81;
  command_packet[PS350_CMD_IDX+3]= 0xA0;
  memcpy(command_packet+PS350_DATA_IDX, cur_dir_id, 4);
  command_packet[25]             = 0xE8;
  command_packet[26]             = 0x03;

  /* So this is where the camera sends us multiple packets
   * without waiting for an ACK or anything. */
  if (ps350_read_dir_list() != 0) {
    goto err_out;
  }

  if (!get_ent) {
    (*num_entries) = cur_ent;
    return 0;
  } else {
    PC_DEBUG_PRINTF("Returning %d\n", found_ent == 0);
    if (!found_ent) {
      ent_name[0] = '\0';
    }
    return found_ent == 0;
  }
}

#define NUM_DIRS 2
static char *directories[NUM_DIRS] = {"DC97", "PWRSHOT"};
static uint8 last_subdir;

static uint8 ps350_get_root_directory(void) {
  char *p;
  uint8 i;

  last_subdir = 0xFF; /* Reset subdir cache */

  if (ps350_select_disk() != 0) {
err_out:
    return -1;
  }

  /* Figure out root directory */
  for (i = 0; i < NUM_DIRS; i++) {
    if (IS_NOT_NULL(p = strchr(root_dir_name, '\\'))) {
      *(p+1) = '\0';
    } else {
      strcat(root_dir_name, "\\");
    }
    strcat(root_dir_name, directories[i]);

    if (get_dir_id(root_dir_name) != 0) {
      goto err_out;
    }
    if (ps350_list_dir(&num_subdirs, 0) == 0) {
      break;
    }
  }
  PC_DEBUG_PRINTF("Root directory: %s (%d entries)\n", root_dir_name, num_subdirs);

  return (i == NUM_DIRS); /* Failure if we finished the loop */
}

static char subdir_path[35];

static uint8 get_subdir_path(uint8 subdir_idx) {
  if (last_subdir == subdir_idx) {
    /* Already set, spare time */
    return 0;
  }
  strcpy(subdir_path, root_dir_name);
  if (get_dir_id(root_dir_name) != 0) {
    goto err_out;
  }
  if (ps350_list_dir(&subdir_idx, 1) != 0) {
err_out:
    return -1;
  }
  PC_DEBUG_PRINTF("Got entity: %s (size %d)\n", ent_name, ent_size);
  strcat(subdir_path, "\\");
  strcat(subdir_path, ent_name);
  last_subdir = subdir_idx;

  return get_dir_id(subdir_path);
}

/* Get information from the camera */
static uint8 ps350_get_information(void) {
  uint8 i, total_pics = 0;
  if (ps350_get_root_directory() != 0) {
err_out:
    return -1;
  }
  /* Now count files in subdirectories */
  for (i = 0; i < num_subdirs; i++) {
    uint8 num_pics_in_dir;

    if (get_subdir_path(i) != 0) {
      goto err_out;
    }

    if (ps350_list_dir(&num_pics_in_dir, 0) != 0) {
      goto err_out;
    }
    total_pics += num_pics_in_dir;
  }

  /* WIP: Get a picture info (from last used subdir_path) */
  cam_info.num_pics = total_pics/2; /* Ignore THM files */
  strcpy(cam_info.name, "Canon PowerShot 350");
  return 0;
}

static void ps350_get_filename(uint8 n_pic, char *dirname, char *filename) {
  uint8 idx_dir = n_pic/100;
  uint8 idx_img = n_pic-1; /* Counted from 0 */
 
  if (get_subdir_path(idx_dir) != 0) {
    goto err_out;
  }
  if (ps350_list_dir(&idx_img, 1) != 0) {
err_out:
    sprintf(filename, "%s%sIMAGE%d.JPG",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", n_pic);
  } else {
    sprintf(filename, "%s%s%s",
          IS_NOT_NULL(dirname)?dirname:"",
          IS_NOT_NULL(dirname)?"/":"", ent_name);
  }
}

static uint8 ps350_get_picture(uint8 n_pic, int fd, off_t avail) {
  ui_get_image_header_str();
  ui_get_image_str(640, 480, 0UL);

  return -1;
}

static uint8 ps350_get_thumbnail(uint8 n_pic, int fd) {
  ui_get_thumbnail_str(n_pic);
  th_info.flash_mode   = 0xFF;
  th_info.quality_mode = 0xFF;
  return -1;
}

#pragma warn(unused-param, push, off)
static uint8 ps350_set_camera_name(const char *name) {
  return -1;
}

static uint8 ps350_set_camera_time(uint8 day, uint8 month, uint8 year, uint8 hour, uint8 minute, uint8 second) {
  return -1;
}

static uint8 ps350_set_quality(uint8 quality) {
  return -1;
}

static uint8 ps350_set_flash(uint8 mode) {
  return -1;
}

static uint8 ps350_take_picture(void) {
  return -1;
}

static uint8 ps350_delete_pictures(void) {
  return -1;
}

static const char *ps350_get_quality_str(uint8 is_pic, uint8 mode) {
  return "unknown";
}

static const char *ps350_get_flash_str(uint8 is_pic, uint8 mode) {
  return "unknown";
}

#pragma warn(unused-param, pop)
