#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

static unsigned char emit_instruction(char *instr, unsigned char cycles, unsigned char cost) {
  printf("%s    ; %d - rem %d\n", instr, cost, cycles - cost);
  return cycles - cost;
}

static unsigned char fast_steps[] = {32, 30, 28, 26, 24, 22, 20, 18, 16, 14, 12};

static unsigned char emit_wait(unsigned char l, unsigned char cycles) {
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

static void sub_level(unsigned char l, unsigned char repeat) {
  unsigned char cycles = AVAIL_CYCLES;
  /* X is l*2 + 32 so compute C030 offset */
  int C030_OFFSET = 0xC030 - ((l * 2) + PAGE_CROSSER);
  char sta_C030_OFFSET[32];
  int i, orig_cycles;

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
      cycles = emit_wait(l, cycles);
      if (cycles % 2) {
        cycles = emit_instruction(sta_C030_OFFSET, cycles, 5);
      } else {
        cycles = emit_instruction("         sta $C030    ", cycles, 4);
      }
    }

    if (i < repeat - 1) {
      cycles = emit_wait(cycles, cycles);
      if (cycles != 0) {
        fprintf(stderr, "bug. Cycles left %d\n", cycles);
        exit(1);
      }
      printf("         jsr waste_%d     ; Inter-duty waster equals to byte read overhead\n", READ_OVERHEAD);
    } else {
      cycles = emit_wait(cycles, cycles);
      if (cycles != 0) {
        fprintf(stderr, "bug. Cycles left %d\n", cycles);
        exit(1);
      }
    }
  }
  printf("         jmp play_next_sample\n\n");
}

static void level(unsigned char l) {
  unsigned char repeat = CARRIER_HZ/SAMPLING_HZ;
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
         "; READ_OVERHEAD = %d\n"
         "; DUTY_CYCLE_LENGTH = %d\n"
         "; AVAIL_CYCLES = %d\n"
         "; NUM_LEVELS = %d\n"
         "; STEP = %d\n"
         "; PAGE_CROSSER = %d\n\n",
         CYCLES_PER_SEC,
         CARRIER_HZ,
         SAMPLING_HZ,
         READ_OVERHEAD,
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

  /* Wasters */
  printf("\n"
         "         .code\n"
         "\n"
         "waste_43: bit $FF\n"
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
         "         jmp (sound_levels-$%02X,x)    ; 25     18      (6 + jmp back)\n"
         "play_next_sample:\n"
         "         iny                         ; 27     20\n"
         "         bne :--                     ; 30(t)  22\n"
         "         inc ptr1+1                  ;        27\n"
         "         bra :-                      ;        30(t)\n"
         "play_out:\n"
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
