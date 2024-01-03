#ifndef __QT_STATE_H__
#define __QT_STATE_H__

#include "platform.h"

typedef enum _qt_state_kind {
  STATE_GET      = 0,
  STATE_PREVIEW,
  STATE_EDIT
} qt_state_kind;

uint8 state_load(uint8 state_num, uint16 *num, char **edit_name);
uint8 state_set(uint8 state_num, uint16 num, const char *edit_name);

#endif
