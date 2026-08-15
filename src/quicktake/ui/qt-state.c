#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

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

static uint8 state_io(int mode) {
  int fd, c;

#ifdef __APPLE2__
  _filetype = PRODOS_T_BIN;
#endif
  fd = open(STATE_FILE, mode);
  if (fd <= 0) {
    return -1;
  }
  
  if (mode == O_RDONLY) {
    c = read(fd, &state, sizeof(state));
  } else {
    c = write(fd, &state, sizeof(state));
  }
  close(fd);
  if (c != sizeof(state)) {
    return -1;
  }

  return 0;
}

uint8 state_load(uint8 state_num, uint16 *num, char **edit_name) {
  uint8 n;

  if (state_io(O_RDONLY) != 0) {
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
  if (state_io(O_RDONLY) != 0) {
    bzero(&state, sizeof(state));
  }
  state.num[state_num] = num;
  if (state_num == STATE_EDIT) {
    strcpy(state.edit_name, edit_name);
  }
  return state_io(O_WRONLY|O_CREAT);
}

void state_unlink(void) {
  unlink(STATE_FILE);
}
#pragma code-name(pop)
