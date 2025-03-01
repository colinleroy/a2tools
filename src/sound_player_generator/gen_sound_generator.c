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
  unsigned char cycles = AVAIL_CYCLES-12; /* Account for jsr/rts */
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
        printf("bug. Cycles left %d\n", cycles);
      }
#ifdef CPU_65c02
      printf("         jsr waste_22\n"); /* Overhead of the outer loop */
#else
      printf("         jsr waste_35\n"); /* Overhead of the outer loop */
#endif
    } else {
      cycles = emit_wait(cycles-8, cycles);
      if (cycles != 8) {
        printf("bug. Cycles left %d\n", cycles);
      }
    }
  }
  printf("         jmp play_next_sample\n\n");
}

static void level(unsigned char l) {
  unsigned char repeat;
  switch(SAMPLING_HZ) {
    case 4000:
      repeat = 4;
      break;
    case 8000:
      repeat = 2;
      break;
    case 16000:
      repeat = 1;
      break;
  }
  sub_level(l, repeat);
}

int main(int argc, char *argv[]) {
  int c;

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
         "waste_35: bit $FF\n"
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
         "\n"
         ":        nop                         ; 2 Compensate when we don't inc ptr1+1\n"
         "         nop                         ; 4\n"
         "         bit $FF                     ; 7\n"
         ":        lda (ptr1),y                ; 13\n"
         "         bmi play_out                ; 15\n"
         "\n"
         "         tax                         ; 17\n"
         "         jmp (sound_levels-$%02X,x)    ; 23 + Duty cycles + jmp back, overhead now 26 cycles\n"
         "play_next_sample:\n"
         "         iny                         ; 28\n"
         "         bne :--                     ; 31\n"
         "         inc ptr1+1\n"
         "         bra :-\n"
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
         "\n"
         ":        nop                         ; 2 Compensate when we don't inc ptr1+1\n"
         "         nop                         ; 4\n"
         "         bit $FF                     ; 7\n"
         ":        lda (ptr1),y                ; 13\n"
         "         bmi play_out                ; 15\n"
         "\n"
         "         tax                         ; 17\n"
         "         lda sound_levels-$%02X,x      ; 21\n"
         "         sta jump_target+1           ; 25\n"
         "         lda sound_levels+1-$%02X,x    ; 27\n"
         "         sta jump_target+2           ; 31\n"
         "jump_target:\n"
         "         jmp $FFFF                   ; 34 + Duty cycles + jmp back, overhead now 26 cycles\n"
         "play_next_sample:\n"
         "         iny                         ; 28\n"
         "         bne :--                     ; 31\n"
         "         inc ptr1+1\n"
         "         jmp :-\n"
         "play_out:\n"
         "         plp\n"
         "         rts\n",
         PAGE_CROSSER,
         PAGE_CROSSER);
#endif
}
