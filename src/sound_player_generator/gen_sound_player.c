/*
 * Copyright (C) 2025 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "sound.h"

int sampling_hz = DEFAULT_SAMPLING_HZ;

static unsigned char fast_steps[] = {37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23,
                                     22, 21, 20, 19, 18, 17, 16, 15, 14, 12};

static int emit_instruction(const char *instr, int cycles, int cycles_avail) {
  cycles_avail -= cycles;
  printf("%s; %d cycles, %d remain\n", instr, cycles, cycles_avail);
  return cycles_avail;
}

static int byte_loaded = 0;
static int pointer_incremented = 0;
static int target_set[2] = {0, 0};

#define BYTE_LOAD_CYCLES 7
static int emit_byte_loading(int cycles) {
  if (cycles < BYTE_LOAD_CYCLES) {
    fprintf(stderr, "Error - not enough cycles to load byte\n");
    exit(1);
  }
  cycles -= 5;
  printf("         lda (ptr1),y              ; %d cycles, %d remain\n", 5, cycles);
  cycles -= 2;
  printf("         tax                       ; %d cycles, %d remain\n", 2, cycles);

  byte_loaded = 1;
  return BYTE_LOAD_CYCLES;
}

#define POINTER_INCR_CYCLES 21
static int emit_pointer_increment(int cycles) {
  if (cycles < POINTER_INCR_CYCLES) {
    fprintf(stderr, "Error - not enough cycles to increment pointer\n");
    exit(1);
  }
  cycles -= POINTER_INCR_CYCLES;
  printf("         jsr incr_pointer          ; %d cycles, %d remain\n", POINTER_INCR_CYCLES, cycles);
  pointer_incremented = 1;
  return POINTER_INCR_CYCLES;
}

#define FAST_POINTER_INCR_CYCLES 12
static int emit_fast_pointer_increment(int cycles) {
  if (cycles < FAST_POINTER_INCR_CYCLES) {
    fprintf(stderr, "Error - not enough cycles to increment pointer (fast)\n");
    exit(1);
  }
  cycles -= FAST_POINTER_INCR_CYCLES;
  printf("         iny                         ; 2\n"
         "         bne :+                      ; 4   (5)\n"
         "         inc ptr1+1                  ; 9\n"
         "         bne :++                     ; 12\n"
         ":        nop                         ; 7\n"
         "         nop                         ; 9\n"
         "         bit $FF                     ; 12\n"
         ":        \n");
  pointer_incremented = 1;
  return FAST_POINTER_INCR_CYCLES;
}

#define HALF_TARGET_BYTE_SET 7
static int emit_half_target_set(int offset, int cycles) {
  if (cycles < HALF_TARGET_BYTE_SET) {
    fprintf(stderr, "Error - not enough cycles to set target low\n");
    exit(1);
  }
  printf("         lda sound_levels+%d,x      ; 4 cycles, %d remain\n"
         "         sta target+%d              ; 3 cycles, %d remain\n",
         offset, cycles-4, offset, cycles-HALF_TARGET_BYTE_SET);
  target_set[offset] = 1;
  return HALF_TARGET_BYTE_SET;
}

#define FULL_TARGET_SET_CYCLES 26
static int emit_full_target_set(int cycles) {
  cycles -= FULL_TARGET_SET_CYCLES;
  target_set[0] = 1;
  target_set[1] = 1;
  printf("         jsr set_jump_target       ; %d cycles, %d remain\n",
    FULL_TARGET_SET_CYCLES, cycles);
  return FULL_TARGET_SET_CYCLES;
}


static void emit_wait_steps(int cycles, int avail) {
  int i;

  for (i = 0; i < sizeof(fast_steps); i++) {
    if (cycles > fast_steps[i]+1 || cycles == fast_steps[i]) {
      cycles -= fast_steps[i];
      printf("         jsr waste_%d              ; %d cycles, %d remain\n", fast_steps[i], fast_steps[i], avail - cycles);
      return emit_wait_steps(cycles, avail);
    }
  }

  while (cycles > 0) {
    if (cycles % 2 != 0) {
      cycles = emit_instruction("         bit $FF                   ", 3, cycles);
    } else {
      cycles = emit_instruction("         nop                       ", 2, cycles);
    }
  }

  if (cycles) {
    fprintf(stderr, "Error - still %d cycles to waste\n", cycles);
    exit(1);
  }
}

static int emit_wait(int waste, int cycles_avail, int allow_half_target) {
  int spent = 0;
  /* While wasting cycles, try to do what we need to.
   * First useful thing to do is to load the next sample.
   */
  if (!byte_loaded && (waste == BYTE_LOAD_CYCLES || waste > BYTE_LOAD_CYCLES+1)) {
    spent = emit_byte_loading(cycles_avail);
    waste -= spent;
    cycles_avail -= spent;
  /* After loading the sample, if we have enough cycles, increment the
   * pointer in the sample stream.
   */
  } 
  if (byte_loaded && !pointer_incremented && (waste == POINTER_INCR_CYCLES || waste > POINTER_INCR_CYCLES+1)) {
    spent = emit_pointer_increment(cycles_avail);
    waste -= spent;
    cycles_avail -= spent;
  /* Or quickly?
   */
  }
#ifdef ENABLE_SLOWER
  else if (byte_loaded && !pointer_incremented && (waste == FAST_POINTER_INCR_CYCLES || waste > FAST_POINTER_INCR_CYCLES+1)) {
    spent = emit_fast_pointer_increment(cycles_avail);
    waste -= spent;
    cycles_avail -= spent;
  /* If we have enough cycles, update both bytes of the jump target.
   * This can be done *before* incrementing the pointer.
   */
  }
#endif
  if (byte_loaded && !target_set[0] && !target_set[1]
          && (waste == FULL_TARGET_SET_CYCLES || waste > FULL_TARGET_SET_CYCLES+1)) {
    spent = emit_full_target_set(cycles_avail);
    waste -= spent;
    cycles_avail -= spent;
  /* Otherwise, update the other byte of the jump target. But not at first repeat,
   * as the next one(s) will have more cycles and we could do a full target set,
   * sparing code size.
   */
  } 
  if (byte_loaded && allow_half_target && (!target_set[0] || !target_set[1])
          && (waste == HALF_TARGET_BYTE_SET || waste > HALF_TARGET_BYTE_SET+1)) {
    if (!target_set[0]) {
      spent = emit_half_target_set(0, cycles_avail);
      waste -= spent;
      cycles_avail -= spent;
    } else if (!target_set[1]) {
      spent = emit_half_target_set(1, cycles_avail);
      waste -= spent;
      cycles_avail -= spent;
    }
  /* If we had nothing useful to do, waste the cycles.
   */
  } else if (waste > 1) {
    cycles_avail -= waste;
    printf("                                   ; Wasting %d cycles\n", waste);
    emit_wait_steps(waste, cycles_avail);
    return cycles_avail;
  } else if (waste == 1){
    printf("                                   ; Nothing to waste %d cycles\n", waste);
    return cycles_avail;
  }

  /* If we did something useful, count those cycles as wasted,
   * and re-call ourselves with the remaining cycles to be
   * wasted/invested.
   */
  if (waste > 1) {
    return emit_wait(waste, cycles_avail, allow_half_target);
  }
  return cycles_avail;
}

static int emit_slow(int cycles_avail) {
#ifdef ENABLE_SLOWER
  cycles_avail -= SLOWER_OVERHEAD;
  printf("         jsr slow_sound            ; %d cycles, remain %d\n", SLOWER_OVERHEAD, cycles_avail);
#endif
  return cycles_avail;
}

static int emit_jump(int cycles_avail) {
  cycles_avail -= JUMP_OVERHEAD;
  printf("         jmp (target)              ; %d cycles, remain %d\n", JUMP_OVERHEAD, cycles_avail);
  return cycles_avail;
}

static void sub_level(int l, int repeat) {
  int cycles = AVAIL_CYCLES;
  /* X is l*2 + 32 so compute C030 offset */
  char sta_C030_OFFSET[32];
  int i, orig_cycles;
  int byte_loaded = 0, incremented_pointer = 0;

  orig_cycles = cycles;
  for (i = 0; i < repeat; i++) {
    cycles = orig_cycles;
    printf(".proc sound_level_%d_%d\n", l, i);
    cycles = emit_instruction("         sta SPKR                  ", 4, cycles);

#ifdef CPU_65c02
    if (l == 1) {
      cycles = emit_instruction("         sta (ispkr)               ", 5, cycles);
    } else
#endif
    {
      cycles = emit_wait(l, cycles, SLOWER_OVERHEAD != 0);
      cycles = emit_instruction("         sta SPKR                  ", 4, cycles);
    }

    if (i != repeat - 1) {
      cycles = emit_slow(cycles);
      cycles = emit_wait(cycles, cycles, SLOWER_OVERHEAD != 0);
    } else {
      cycles = emit_wait(cycles - (JUMP_OVERHEAD + SLOWER_OVERHEAD), cycles, 1);
      cycles = emit_slow(cycles);
      emit_jump(cycles);
    }
    printf(".endproc\n");
  }

  printf("\n\n");
}

static void level(int l) {
  int repeat = CARRIER_HZ/sampling_hz;
  if (CARRIER_HZ % sampling_hz != 0) {
    fprintf(stderr, "CARRIER_HZ %d is not a multiple of SAMPLING_HZ %d\n",
            CARRIER_HZ, sampling_hz);
    exit(1);
  }
  byte_loaded = 0;
  pointer_incremented = 0;
  target_set[0] = 0;
  target_set[1] = 0;
  sub_level(l, repeat);
  if (!target_set[0] || !target_set[1] ||
      !pointer_incremented || !byte_loaded) {
        fprintf(stderr, "Error: not enough cycles to do everything at level %d: \n%s%s%s%s\n", l,
                !byte_loaded ? "Could not load byte\n":"",
                !pointer_incremented ? "Could not increment pointer\n":"",
                !target_set[0] ? "Could not set low target\n":"",
                !target_set[1] ? "Could not set high target\n":"");
        exit(1);
  }
}

static char *segment = "DATA";

int main(int argc, char *argv[]) {
  int c;

  if (argc < 2) {
    fprintf(stderr, "Usage: %s [sampling hz]\n", argv[0]);
    exit(1);
  }

  if (argc == 3) {
    segment = argv[2];
  }

  sampling_hz = atoi(argv[1]);

  printf("; Settings for this sound generator:\n"
         "; CYCLES_PER_SEC = %d\n"
         "; CARRIER_HZ = %d\n"
         "; SAMPLING_HZ = %d\n"
         "; DUTY_CYCLE_LENGTH = %d\n"
         "; AVAIL_CYCLES = %d\n"
         "; NUM_LEVELS = %d\n"
         "; STEP = %d\n\n",
         CYCLES_PER_SEC,
         CARRIER_HZ,
         sampling_hz,
         DUTY_CYCLE_LENGTH,
         AVAIL_CYCLES,
         NUM_LEVELS,
         STEP);

  printf("         .export   _play_sample\n"
         "         .importzp ptr1, ptr2, ptr3, tmp3, tmp4\n"
         "\n"
         "SPKR     = $C030\n"
         "ispkr    = ptr2\n"
         "target   = ptr3\n"
         "snd_slow = tmp4\n"
         "\n\n");

  printf(".segment \"%s\"\n", segment);
  printf(".align $100\n");
  printf("START_SOUND_PLAYER = *\n");

  printf("\n"
         ".proc slow_sound\n"
         "         sty tmp3                  ; 3\n"
         "         ldy snd_slow              ; 6\n"
         ":        dey                       ; 8\n"
         "         bpl :-                    ; 10\n"
         "         ldy tmp3                  ; 13\n"
         "         rts\n"
         ".endproc\n\n");

  /* Jump table, aligned on a page to make sure we don't get extra cycles */
  printf(".proc sound_levels\n");
  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf("         .addr sound_level_%d_0\n", c);
  }
  printf("         .addr play_done\n");
  printf(".endproc\n\n");

  printf(".proc incr_pointer\n");
  printf("         iny                         ; 2\n"
         "         bne :+                      ; 4   (5)\n"
         "         inc ptr1+1                  ; 9\n"
         "         rts                         ; 15\n"
         ":        nop                         ; 7\n"
         "         nop                         ; 9\n");
  printf("         rts                         ; 15\n\n");
  printf(".endproc\n\n");
#if POINTER_INCR_CYCLES != 21
#error Recount pointer increment cycles
#endif

  printf(".proc set_jump_target\n");
  emit_half_target_set(0, HALF_TARGET_BYTE_SET);
  emit_half_target_set(1, HALF_TARGET_BYTE_SET);
  printf("         rts\n");
  printf(".endproc\n\n");
#if FULL_TARGET_SET_CYCLES != 26
#error Recount target setting cycles
#endif

  /* Wasters */
  printf("\n"
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

  printf("\n"
         "waste_37: nop\n"
         "waste_35: nop\n"
         "waste_33: nop\n"
         "waste_31: nop\n"
         "waste_29: nop\n"
         "waste_27: nop\n"
         "waste_25: nop\n"
         "waste_23: nop\n"
         "waste_21: nop\n"
         "waste_19: nop\n"
         "waste_17: nop\n"
         "waste_15: bit $FF\n"
         "          rts\n\n");

  printf(".proc _play_sample\n"
         "         sty snd_slow\n"
         "         sta ptr1\n"
         "         stx ptr1+1\n"
         "         lda #<SPKR\n"
         "         sta ispkr\n"
         "         lda #>SPKR\n"
         "         sta ispkr+1\n"
         "         php\n"
         "         sei\n"
         "         ldy #$00\n"
         "         lda (ptr1),y\n"
         "         tax\n\n");
  emit_half_target_set(0, HALF_TARGET_BYTE_SET);
  emit_half_target_set(1, HALF_TARGET_BYTE_SET);
  emit_jump(JUMP_OVERHEAD+SLOWER_OVERHEAD);
  printf(".endproc\n\n");
  printf(".proc play_done\n"
  "         plp\n"
  "         rts\n");
  printf(".endproc\n\n");


  printf(".assert >START_SOUND_PLAYER = >*, error\n");

  /* Levels */
  for (c = 0; c < NUM_LEVELS; c+=STEP) {
#ifdef ENABLE_SLOWER
    if (c % (sampling_hz/2000) == 0) {
      printf(".align $100\n");
    }
#endif
    level(c);
#ifdef ENABLE_SLOWER
  printf(".assert >sound_level_%d_0 = >*, error\n\n", c);
#endif
  }
}
