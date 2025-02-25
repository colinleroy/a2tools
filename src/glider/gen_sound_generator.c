#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

static unsigned char emit_instruction(char *instr, unsigned char cycles, unsigned char cost) {
  printf("%s    ; %d - rem %d\n", instr, cost, cycles - cost);
  return cycles - cost;
}

static unsigned char fast_steps[5] = {48, 24, 20, 16, 12};

static unsigned char emit_wait(unsigned char l, unsigned char cycles) {
  int i;

  while (l > 0) {
    again:
    for (i = 0; i < sizeof(fast_steps); i++) {
      if (l > fast_steps[i]+1) {
        cycles -= fast_steps[i];
        printf("jsr waste_%d      ; %d - rem %d\n", fast_steps[i], fast_steps[i], cycles);
        l -= fast_steps[i];
        if (l > fast_steps[i]+1) {
          goto again;
        }
      }
      if (l == fast_steps[i]) {
        cycles -= fast_steps[i];
        printf("jsr waste_%d      ; %d - rem %d\n", fast_steps[i], fast_steps[i], cycles);
        l -= fast_steps[i];
      }
    }

    if (l > 5) {
      cycles -= 2;
      printf("nop              ; 2 - rem %d\n", cycles);
      cycles -= 2;
      printf("nop              ; 2 - rem %d\n", cycles);
      l -= 4;
      if (l > 5) {
        continue;
      }
    }
    if (l == 4) {
      cycles -= 2;
      printf("nop              ; 2 - rem %d\n", cycles);
      cycles -= 2;
      printf("nop              ; 2 - rem %d\n", cycles);
      l -= 4;
    }
    if (l > 3) {
      cycles -= 3;
      printf("lda $FF          ; 3 - rem %d\n", cycles);
      l -= 3;
      if (l > 3) {
        continue;
      }
    } 
    if (l == 3) {
      cycles -= 3;
      printf("lda $FF          ; 3 - rem %d\n", cycles);
      l -= 3;
    }
    if (l == 2) {
      cycles -= 2;
      printf("nop              ; 2 - rem %d\n", cycles);
      l -= 2;
    } else if (l == 1) {
      printf("Just one cycle left :( %d\n", l);
      exit(1);
    }
  }
  return cycles;
}

static void sub_level(unsigned char l) {
  unsigned char cycles = (AVAIL_CYCLES/2)-12; /* Account for jsr/rts */
  printf("; SubLevel %d\n", l);
  printf("sub_sound_level_%d:\n", l);
  cycles = emit_instruction("ldx #$32     ", cycles, 2);
  printf("toggle_on_%d:\n", l);
  cycles = emit_instruction("sta $C030    ", cycles, 4);
  
  if (l == 0) {
    cycles = emit_instruction("sta $C030  ", cycles, 4);
  } else if (l == 1) {
    cycles = emit_instruction("sta $BFFE,x", cycles, 5);
  } else if (l == 2) {
    cycles = emit_instruction("nop        ", cycles, 2);
    cycles = emit_instruction("sta $C030  ", cycles, 4);
  } else {
    cycles = emit_wait(l, cycles);
    if (cycles % 2) {
      cycles = emit_instruction("sta $BFFE,x", cycles, 5);
    } else {
      cycles = emit_instruction("sta $C030  ", cycles, 4);
    }
  }

  cycles = emit_wait(cycles-6, cycles);
  if (cycles != 6) {
    printf("bug. Cycles left %d\n", cycles);
  }
  cycles = emit_instruction("rts  ", cycles, 6);
  printf("\n\n");
}

static void level(unsigned char l) {
  printf("; Level %d, %d cycles\n", l, AVAIL_CYCLES);
  printf("sound_level_%d:\n", l);
  printf("jsr sub_sound_level_%d\n", l);
  printf("jsr waste_12\n");
  sub_level(l);
}

int main(int argc, char *argv[]) {
  int c;

  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf(".export sound_level_%d\n", c);
  }

  printf("waste_48:\n"
         "jsr waste_24\n");

  printf("waste_24: lda $FFFF\n\n");
  printf("waste_20: lda $FFFF\n\n");
  printf("waste_16: lda $FFFF\n\n");
  printf("waste_12: rts\n\n");

  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    level(c);
  }
}
