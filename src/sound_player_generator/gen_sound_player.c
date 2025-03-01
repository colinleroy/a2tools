#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "sound.h"

static int emit_instruction(char *instr, int cycles, int cost) {
  printf("%s    ; %d - rem %d\n", instr, cost, cycles - cost);
  return cycles - cost;
}

static unsigned char fast_steps[] = {46, 44, 42, 40, 38, 36, 34, 32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12};

static int emit_wait(int l, int cycles) {
  int i;

  while (l > 0) {
    again:
    for (i = 0; i < sizeof(fast_steps); i++) {
      if (l > fast_steps[i]+1) {
        cycles -= fast_steps[i];
        printf("         jsr waste_%d     ; %d - rem %d\n", fast_steps[i], fast_steps[i], cycles);
        l -= fast_steps[i];
        if (l > fast_steps[i]+1) {
          goto again;
        }
      }
      if (l == fast_steps[i]) {
        cycles -= fast_steps[i];
        printf("         jsr waste_%d     ; %d - rem %d\n", fast_steps[i], fast_steps[i], cycles);
        l -= fast_steps[i];
      }
    }

    if (l > 5) {
      cycles -= 2;
      printf("         nop              ; 2 - rem %d\n", cycles);
      cycles -= 2;
      printf("         nop              ; 2 - rem %d\n", cycles);
      l -= 4;
      if (l > 5) {
        continue;
      }
    }
    if (l == 4) {
      cycles -= 2;
      printf("         nop              ; 2 - rem %d\n", cycles);
      cycles -= 2;
      printf("         nop              ; 2 - rem %d\n", cycles);
      l -= 4;
    }
    if (l > 3) {
      cycles -= 3;
      printf("         bit $FF          ; 3 - rem %d\n", cycles);
      l -= 3;
      if (l > 3) {
        continue;
      }
    } 
    if (l == 3) {
      cycles -= 3;
      printf("         bit $FF          ; 3 - rem %d\n", cycles);
      l -= 3;
    }
    if (l == 2) {
      cycles -= 2;
      printf("         nop              ; 2 - rem %d\n", cycles);
      l -= 2;
    } else if (l == 1) {
      printf("Just one cycle left :( %d\n", l);
      exit(1);
    }
  }
  return cycles;
}

#define BYTE_LOAD_CYCLES 13
static int emit_byte_loading(int cycles) {
  if (cycles < BYTE_LOAD_CYCLES+6) {
    fprintf(stderr, "Error - not enough cycles to load byte\n");
    exit(1);
  }
  cycles -= BYTE_LOAD_CYCLES+6;
  printf("         jsr load_byte    ; 6+%d - rem %d\n", BYTE_LOAD_CYCLES, cycles);
  return cycles;
}

#define POINTER_INCR_CYCLES 15
static int emit_pointer_increment(int cycles) {
  if (cycles < POINTER_INCR_CYCLES+6) {
    fprintf(stderr, "Error - not enough cycles to increment pointer\n");
    exit(1);
  }
  cycles -= POINTER_INCR_CYCLES+6;
  printf("         jsr incr_pointer ; 6+%d - rem %d\n", POINTER_INCR_CYCLES, cycles);
  return cycles;
}

static void sub_level(int l, int repeat) {
  int cycles = AVAIL_CYCLES;
  /* X is l*2 + 32 so compute C030 offset */
  int C030_OFFSET = 0xC030 - ((l * 2) + PAGE_CROSSER);
  char sta_C030_OFFSET[32];
  int i, orig_cycles;
  int byte_loaded = 0, incremented_pointer = 0;

  snprintf(sta_C030_OFFSET, sizeof(sta_C030_OFFSET),
           "         sta $%04X,x  ", C030_OFFSET);

  orig_cycles = cycles;
  for (i = 0; i < repeat; i++) {
    cycles = orig_cycles;
    printf("sound_level_%d_%d:\n", l, i);
    cycles = emit_instruction("         sta $C030    ", cycles, 4);
    if (l == 0) {
      cycles = emit_instruction("         sta $C030    ", cycles, 4);
    } else if (l == 1) {
      cycles = emit_instruction(sta_C030_OFFSET, cycles, 5);
    } else if (l == 2) {
      cycles = emit_instruction("         nop          ", cycles, 2);
      cycles = emit_instruction("         sta $C030    ", cycles, 4);
    } else {
      if (!incremented_pointer && l > POINTER_INCR_CYCLES+6+3) {
        cycles = emit_pointer_increment(cycles);
        cycles = emit_wait(l-(POINTER_INCR_CYCLES+6), cycles);
        incremented_pointer = 1;
      } else if (incremented_pointer && !byte_loaded && l > BYTE_LOAD_CYCLES+6+3) {
        cycles = emit_byte_loading(cycles);
        cycles = emit_wait(l-(BYTE_LOAD_CYCLES+6), cycles);
        byte_loaded = 1;
      } else {
        cycles = emit_wait(l, cycles);
      }
      cycles = emit_instruction("         sta $C030    ", cycles, 4);
    }

    if (!incremented_pointer && cycles > POINTER_INCR_CYCLES+6+2) {
      cycles = emit_pointer_increment(cycles);
      incremented_pointer = 1;
    }
    if (incremented_pointer && !byte_loaded && cycles > BYTE_LOAD_CYCLES+6+2) {
      cycles = emit_byte_loading(cycles);
      byte_loaded = 1;
    }

    if (i < repeat - 1) {
      cycles = emit_wait(cycles, cycles);
      if (cycles != 0) {
        fprintf(stderr, "bug. Cycles left %d\n", cycles);
        exit(1);
      }
      printf("         nop             ; Inter-duty waster equals to jmp overhead\n");
      printf("         nop\n");
      printf("         nop\n");
    } else {
      cycles = emit_wait(cycles, cycles);
      if (cycles != 0) {
        fprintf(stderr, "bug. Cycles left %d\n", cycles);
        exit(1);
      }
    }
  }
  if (!byte_loaded) {
    fprintf(stderr, "Error: not enough cycles to load byte\n");
    exit(1);
  }
  if (!incremented_pointer) {
    fprintf(stderr, "Error: not enough cycles to increment pointer\n");
    exit(1);
  }
  printf("         jmp (sound_levels-$%02X,x)\n\n", PAGE_CROSSER);
}

static void level(int l) {
  int repeat = CARRIER_HZ/SAMPLING_HZ;
  if (CARRIER_HZ % SAMPLING_HZ != 0) {
    fprintf(stderr, "CARRIER_HZ %d is not a multiple of SAMPLING_HZ %d\n",
            CARRIER_HZ, SAMPLING_HZ);
    exit(1);
  }
  sub_level(l, repeat);
}

int main(int argc, char *argv[]) {
  int c;

  printf("; Settings for this sound generator:\n"
         "; CYCLES_PER_SEC = %d\n"
         "; CARRIER_HZ = %d\n"
         "; SAMPLING_HZ = %d\n"
         "; DUTY_CYCLE_LENGTH = %d\n"
         "; AVAIL_CYCLES = %d\n"
         "; NUM_LEVELS = %d\n"
         "; STEP = %d\n"
         "; PAGE_CROSSER = %d\n\n",
         CYCLES_PER_SEC,
         CARRIER_HZ,
         SAMPLING_HZ,
         DUTY_CYCLE_LENGTH,
         AVAIL_CYCLES,
         NUM_LEVELS,
         STEP,
         PAGE_CROSSER);

  printf("         .export   _play_sample\n"
         "         .importzp ptr1\n"
         "\n"
         "         .rodata\n\n");

  /* Jump table */
  printf("sound_levels:\n");
  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf("         .addr sound_level_%d_0\n", c);
  }
  printf("         .addr play_done\n");

  /* Wasters */
  printf("\n"
         "         .code\n"
         "\n"
         "waste_46: nop\n"
         "waste_44: nop\n"
         "waste_42: nop\n"
         "waste_40: nop\n"
         "waste_38: nop\n"
         "waste_36: nop\n"
         "waste_34: nop\n"
         "waste_32: nop\n"
         "waste_30: nop\n"
         "waste_28: nop\n"
         "waste_26: nop\n"
         "waste_24: nop\n"
         "waste_22: nop\n"
         "waste_20: nop\n"
         "waste_18: nop\n"
         "waste_16: nop\n"
         "waste_14: nop\n"
         "waste_12: rts\n\n");

  /* Levels */
  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    level(c);
  }

  /* Player */
  printf("         .data\n\n"
         ".align $100\n");
#ifdef CPU_65c02
  printf("load_byte:\n"
         "         lda (ptr1),y                ; 5\n"
         "         tax                         ; 7\n"
         "         rts                         ; 13\n");
#if BYTE_LOAD_CYCLES != 13
#error Recount byte load cycles
#endif

  printf("incr_pointer:\n"
         "         iny                         ; 2\n"
         "         bne :+                      ; 4   (5)\n"
         "         inc ptr1+1                  ; 9\n"
         "         rts                         ; 15\n"
         ":        nop                         ; 7\n"
         "         nop                         ; 9\n"
         "         rts                         ; 15\n");
#if POINTER_INCR_CYCLES != 15
#error Recount pointer increment cycles
#endif

  printf("_play_sample:\n"
         "         sta ptr1\n"
         "         stx ptr1+1\n"
         "         php\n"
         "         sei\n"
         "         ldy #$00\n"
         "         lda (ptr1),y\n"
         "         tax\n"
         "         jmp (sound_levels-$%02X,x)\n"
         "play_done:\n"
         "         plp\n"
         "         rts\n",
         PAGE_CROSSER);
#else
  printf("_play_sample:\n"
         "         sta ptr1\n"
         "         stx ptr1+1\n"
         "         php\n"
         "         sei\n"
         "         ldy #$00\n"
         "                                     ; Byte read loop\n"
         "                                     ; Y=0    Y!=0\n"
         ":        nop                         ; 2             Compensate when we don't inc ptr1+1\n"
         "         nop                         ; 4\n"
         "         bit $FF                     ; 7\n"
         ":        lda (ptr1),y                ; 12     5\n"
         "         bmi play_out                ; 14     7      Considered not taken\n"
         "\n"
         "         tax                         ; 16     9\n"
         "         lda sound_levels-$%02X,x      ; 20     13\n"
         "         sta jump_target+1           ; 24     17\n"
         "         lda sound_levels+1-$%02X,x    ; 28     21\n"
         "         sta jump_target+2           ; 32     25\n"
         "jump_target:\n"
         "         jmp $FFFF                   ; 38     31 (3 + jmp back)\n"
         "play_next_sample:\n"
         "         iny                         ; 40     33\n"
         "         bne :--                     ; 43(t)  35\n"
         "         inc ptr1+1                  ;        40\n"
         "         jmp :-                      ;        43\n"
         "play_out:\n"
         "         plp\n"
         "         rts\n",
         PAGE_CROSSER,
         PAGE_CROSSER);
#endif
}
