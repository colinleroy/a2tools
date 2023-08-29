#include <string.h>
#include <stddef.h>

/* Remove the most annoying MAME symbols 
 * and put back the address instead 
 */
const char * fix_mame_param(const char *param) {
  if (!param)
    return NULL;
  if (param[0] == '$'
   || param[0] == '#'
   || param[0] == '<'
   || param[0] == '>')
    return param;
  if (!strcmp(param, "TXTCLR"))
    return "$c050";
  if (!strcmp(param, "TXTSET"))
    return "$c051";
  if (!strcmp(param, "MIXCLR"))
    return "$c052";
  if (!strcmp(param, "MIXSET"))
    return "$c053";
  if (!strcmp(param, "TXTPAGE1"))
    return "$c054";
  if (!strcmp(param, "TXTPAGE2"))
    return "$c055";
  if (!strcmp(param, "LORES"))
    return "$c056";
  if (!strcmp(param, "HIRES"))
    return "$c057";
  if (!strcmp(param, "DHIRESON"))
    return "$c05e";
  if (!strcmp(param, "DHIRESOFF"))
    return "$c05f";
  if (!strcmp(param, "ROMIN"))
    return "$c085";
  if (!strcmp(param, "LCBANK2"))
    return "$c087";
  if (!strcmp(param, "($03fe)"))
    return "$befb";
  return param;
}
