#include "platform.h"

int8 mul_362(int8 w)
{
  int16 x;
  x = (int16)(w * 362);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_473(int8 w)
{
  int16 x;
  x = (int16)(w * 473);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_277(int8 w)
{
  int16 x;
  x = (int16)(w * 277);
  x >>= 8;
  return (int8)x & 0xFF;
}

int8 mul_669(int8 w)
{
  int16 x;
  x = (int16)(w * 669);
  x >>= 8;
  return (int8)x & 0xFF;
}
