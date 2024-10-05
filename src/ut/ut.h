#ifndef __ut_h
#define __ut_h

#define ASSERT_EQU(n, e, str) do {                    \
  if (n != e) {                                       \
    printf("%s:%d: "str" (expected %04x, got %04x)\n", \
           __FILE__, __LINE__, e, n);                 \
    exit(1);                                          \
  }                                                   \
} while (0)

#define ASSERT_STR_EQU(n, e, str) do {                \
  if (strcmp(n, e)) {                                 \
    printf("%s:%d: "str" (expected '%s', got '%s')\n",\
           __FILE__, __LINE__, e, n);                 \
    exit(1);                                          \
  }                                                   \
} while (0)

#endif
