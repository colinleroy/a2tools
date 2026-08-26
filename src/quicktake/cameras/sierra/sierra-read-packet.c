#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "platform.h"
#include "extended_conio.h"
#include "simple_serial.h"
#include "sierra.h"
#include "../qt-serial.h"

extern uint16 sierra_response_len;
extern uint8 sierra_response_continues;
extern uint8 sierra_packet_type;
extern uint8 resetting;
extern uint8 header[3], footer[2];

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

uint8 sierra_read_packet(void) {
  uint8 tries = 0;

  /* Set to stupid value */
  sierra_packet_type = 0xEE;
  bzero(header, 3);
  bzero(footer, 2);
  bzero(buffer, 16);

  /* either one byte or longer. length in bytes 2-3 */
  if (simple_serial_read_no_irq((char *)&sierra_packet_type, 1) != 0) {
    PC_DEBUG_PRINTF("Timeout\r\n");
    return EOF;
  }

  if (sierra_packet_type == SIERRA_PACKET_DATA ||
      sierra_packet_type == SIERRA_PACKET_DATA_END ||
      sierra_packet_type == SIERRA_PACKET_COMMAND) {
    sierra_response_continues = (sierra_packet_type == SIERRA_PACKET_DATA);
    /* Read subtype and length */
    if (simple_serial_read_no_irq((char *)header, 3) != 0) {
      PC_DEBUG_PRINTF("header %02X %02X %02X\r\n", header[0], header[1], header[2]);
      return EOF;
    }
    PC_DEBUG_BUFFER("HEADER ", header, 3);
    /* Ignore subtype for now */
    sierra_response_len = header[1] | (header[2] << 8);
    /* Read actual data */
    if (simple_serial_read_no_irq((char *)buffer, sierra_response_len) != 0) {
      PC_DEBUG_PRINTF("Timeout packet\r\n");
      return EOF;
    }
    PC_DEBUG_BUFFER("RESPONSE ", buffer, sierra_response_len);

    /* Read two more bytes for the checksum */
    if (simple_serial_read_no_irq((char *)footer, 2) != 0) {
      PC_DEBUG_PRINTF("Error checksum\r\n");
      return -1;
    } else {
      return 0;
    }
  } else if (sierra_packet_type == SIERRA_PACKET_SESSION_END) {
    PC_DEBUG_PRINTF("session end %02X\n", sierra_packet_type);
    cputs("got session end\r\n");
    if (!resetting && tries++ < 3) {
      sierra_reset();
      sierra_packet_type = SIERRA_PACKET_RETRY_INTERNAL;
      return -1;
    } else {
      /* Abandon and reset tries */
      tries = 0;
      return -1;
    }
  } else {
    PC_DEBUG_PRINTF("OK %02X\n", sierra_packet_type);
    cputs("single byte OK\r\n");
    /* We read the single-byte "packet" */
    tries = 0;
    return 0;
  }
}
