#include <ctype.h>
#include "extended_conio.h"
#include "simple_serial.h"
#include "platform.h"
#include "../qt-serial.h"
#include "ps350.h"

#pragma code-name(push, "PS350")
#pragma rodata-name(push, "PS350")
#pragma data-name(push, "PS350")
#pragma bss-name(push, "PS350")

#ifdef __CC65__
#define PC_DEBUG_BUFFER(op, str, len)
#define PC_DEBUG_PRINTF(...)
#else
extern uint8 do_debug;
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

extern uint8 is_multi;
extern uint8 found_ent;
extern char ent_name[13];
extern uint32 ent_size;
extern uint8 cur_ent;

static uint16 rem_bytes_in_pack;
static uint8 offset_in_pack;

static void get_packet_length(void) {
  /* Discard header and low byte of size */
  simple_serial_read_no_irq(buffer, 4);
  /* Get packet len high byte to check for multi */
  simple_serial_read_no_irq(&is_multi, 1);
  is_multi &= 0x80;
}

uint8 read_to_get_ent;
uint8 ent_to_get;

uint8 ps350_read_dir_list(void) {
  ps350_send_packet();
  get_packet_length();

  PC_DEBUG_PRINTF("Packet is_multi %d\n", is_multi);
  /* Discard 22 bytes */
  if (simple_serial_read_no_irq(buffer, 22) != 0) {
    goto err_out;
  }
  offset_in_pack = 27;
  found_ent = 0;
packet_loop:
  rem_bytes_in_pack = 299-offset_in_pack;
  PC_DEBUG_PRINTF("Packet loop start: Offset %d, remainaing %d\n", offset_in_pack, rem_bytes_in_pack);
file_loop:
  if (simple_serial_read_no_irq(buffer, 1) != 0) {
    goto err_out;
  }
  PC_DEBUG_PRINTF("file type %d\n", buffer[0]);
  if (buffer[0] == 0x00) {
    goto file_loop_done;
  }
  /* Get size */
  if (read_to_get_ent && !found_ent) {
    if (simple_serial_read_no_irq((char *)&ent_size, 4) != 0) {
      goto err_out;
    }
  } else {
    if (simple_serial_read_no_irq(buffer, 4) != 0) {
      goto err_out;
    }
  }
  /* Skip date */
  if (simple_serial_read_no_irq(buffer, 4) != 0) {
    goto err_out;
  }
  if (read_to_get_ent && !found_ent) {
    if (simple_serial_read_no_irq(ent_name, 12) != 0) {
    goto err_out;
    }
    ent_name[12] = '\0';
    /* Skip T*, the thumbnails */
    if (ent_name[0] >= 'A' && ent_name[0] <= 'Z' && ent_name[0] != 'T') {
      if (cur_ent == ent_to_get) {
        PC_DEBUG_PRINTF("found file %s (%d bytes)\n", ent_name, ent_size);
        found_ent = 1;
      }
      cur_ent++;
    }
  } else {
    if (simple_serial_read_no_irq(buffer, 12) != 0) {
      goto err_out;
    }
    /* Skip T*, the thumbnails */
    if (buffer[0] >= 'A' && buffer[0] <= 'Z' && ent_name[0] != 'T') {
      cur_ent++;
    }
  }

  rem_bytes_in_pack -= 21;
  PC_DEBUG_PRINTF("file loop cont: remainaing %d\n", rem_bytes_in_pack);
  goto file_loop;

file_loop_done:
  PC_DEBUG_PRINTF("file loop done: remainaing %d\n", rem_bytes_in_pack);
  if (simple_serial_read_no_irq(buffer, rem_bytes_in_pack) != 0) {
    goto err_out;
  }
  PC_DEBUG_BUFFER("Discarded: ", buffer, rem_bytes_in_pack);
  if (!is_multi) {
    goto packet_loop_done;
  }
  get_packet_length();

  PC_DEBUG_PRINTF("Next packet is_multi %d\n", is_multi);
  offset_in_pack = 5;
  goto packet_loop;
packet_loop_done:

  return ps350_get_eot_and_ack();
err_out:
  return -1;
}
