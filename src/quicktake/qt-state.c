#include <errno.h>
#include <stdio.h>
#include <string.h>
#include "qt-state.h"
#include "extended_conio.h"
#include "platform.h"
#include "path_helper.h"


typedef struct _qt_state {
  uint16 num[3];
  char edit_name[FILENAME_MAX];
} qt_state;

static qt_state state;

#define STATE_FILE "/RAM/QTSTATE"

static uint8 state_io(char *mode) {
  FILE *fp;
  int c;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif
  fp = fopen(STATE_FILE, mode);
  if (!fp) {
    return -1;
  }
  
  if (mode[0] == 'r') {
    c = fread(&state, 1, sizeof(state), fp);
  } else {
    c = fwrite(&state, 1, sizeof(state), fp);
  }
  fclose(fp);
  if (c != sizeof(state)) {
    return -1;
  }

  return 0;
}

uint8 state_load(uint8 state_num, uint16 *num, char **edit_name) {
  uint8 n;

  if (state_io("r") != 0) {
    return -1;
  }
  n = state.num[state_num];
  if (n != 0) {
    if (state_num == STATE_EDIT) {
      *edit_name = state.edit_name;
    }
    *num = n;
    return 0;
  }
  return -1;
}

#pragma code-name(push, "LC")

uint8 state_set(uint8 state_num, uint16 num, const char *edit_name) {
  if (state_io("r") != 0) {
    memset(&state, 0, sizeof(state));
  }
  state.num[state_num] = num;
  if (state_num == STATE_EDIT) {
    strcpy(state.edit_name, edit_name);
  }
  return state_io("w");
}
#pragma code-name(pop)
