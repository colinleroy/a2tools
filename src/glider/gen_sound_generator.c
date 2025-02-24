#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sound.h"

static unsigned char emit_instruction(char *instr, unsigned char cycles, unsigned char cost) {
  printf("%s    ; %d - rem %d\n", instr, cost, cycles - cost);
  return cycles - cost;
}

static unsigned char emit_wait(unsigned char l, unsigned char cycles) {
/*
2+N*5 cycles+2
  ldy #XX     ; 2
: dey         ; 4
  bne :-      ; 7 if branch is taken

2+N*8 cycles+2
  ldy #XX     ; 
: lda $00     ; 3
  dey         ; 5
  bne :-      ; 8 if branch is taken

2+N*9 cycles+2
  ldy #XX     ; 
: lda $FFFF   ; 4
  dey         ; 6
  bne :-      ; 9 if branch is taken
*/
  while (l > 0) {
    if (l > 13) {
      cycles -= 12;
      printf("jsr waste_12      ; 12 - rem %d\n", cycles);
      l -= 12;
      if (l > 13) {
        continue;
      }
    }
    if (l == 12) {
      cycles -= 12;
      printf("jsr waste_12      ; 12 - rem %d\n", cycles);
      l -= 12;
    }
    if (l > 5) {
      cycles -= 4;
      printf("lda $FFFF        ; 4 - rem %d\n", cycles);
      l -= 4;
      if (l > 5) {
        continue;
      }
    }
    if (l == 4) {
      cycles -= 4;
      printf("lda $FFFF        ; 4 - rem %d\n", cycles);
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
      printf("lda #$FF          ; 2 - rem %d\n", cycles);
      l -= 2;
    } else if (l == 1) {
      printf("Just one cycle left :( %d\n", l);
      exit(1);
    }
  }
  return cycles;
}

static void level(unsigned char l) {
  unsigned char cycles = 128-6;
  printf("; Level %d\n", l);
  printf("sound_level_%d:\n", l);
  cycles = emit_instruction("ldx #$32     ", cycles, 2);
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
    if (cycles == 6+5) {
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

int main(int argc, char *argv[]) {
  int c;

  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf(".export sound_level_%d\n", c);
  }

  printf("waste_12: rts\n\n");

  for (c = 0; c < 106; c++) {
    level(c);
  }
}
