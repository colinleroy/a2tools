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
  FILE *fp;
  char *filename;
  int c;

  if (argc < 2) {
    printf("Usage: %s [input.raw]\n", argv[0]);
    exit(1);
  }

  fp = fopen(argv[1], "rb");
  if (fp == NULL) {
    printf("Can not open %s\n", argv[1]);
    exit(1);
  }
  filename = argv[1];
  if (strchr(filename, '.')) {
    *strchr(filename, '.') = 0;
  }
  if (strchr(filename, '/')) {
    filename = strrchr(filename, '/')+1;
  }

  for (c = 0; c < NUM_LEVELS; c+=STEP) {
    printf(".import sound_level_%d\n", c);
  }
  printf(".export _play_%s\n", filename);
  printf("_play_%s:", filename);

  while ((c = fgetc(fp)) != EOF) {
    unsigned char r = (c*105)/255;
    r = (r/STEP)*STEP;
    printf("jsr sound_level_%d\n", r);
  }
  printf("rts");

  fclose(fp);
}
