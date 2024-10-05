#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "ut.h"
#include "strsplit.h"


char *lines[32];
int n_lines;
int i;

/* Full of leaks, but we want a new pointer on each test */
int main(void) {
  n_lines = strnsplit_in_place(strdup(""), '\n', lines, 3);
  ASSERT_EQU(n_lines, 0, "wrong number of lines");

  n_lines = strnsplit_in_place(strdup("abc"), '\n', lines, 3);
  ASSERT_EQU(n_lines, 1, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\n"), '\n', lines, 3);
  ASSERT_EQU(n_lines, 1, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\ndef\nghi\n"), '\n', lines, 3);
  ASSERT_EQU(n_lines, 3, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");
  ASSERT_STR_EQU(lines[1], "def", "wrong value");
  ASSERT_STR_EQU(lines[2], "ghi", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\ndef\nghi\n\njkl"), '\n', lines, 5);
  ASSERT_EQU(n_lines, 5, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");
  ASSERT_STR_EQU(lines[1], "def", "wrong value");
  ASSERT_STR_EQU(lines[2], "ghi", "wrong value");
  ASSERT_STR_EQU(lines[3], "", "wrong value");
  ASSERT_STR_EQU(lines[4], "jkl", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\ndef\nghi\n\n\n"), '\n', lines, 5);
  ASSERT_EQU(n_lines, 5, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");
  ASSERT_STR_EQU(lines[1], "def", "wrong value");
  ASSERT_STR_EQU(lines[2], "ghi", "wrong value");
  ASSERT_STR_EQU(lines[3], "", "wrong value");
  ASSERT_STR_EQU(lines[4], "", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\ndef\nghi\n\n\njkl"), '\n', lines, 6);
  ASSERT_EQU(n_lines, 6, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");
  ASSERT_STR_EQU(lines[1], "def", "wrong value");
  ASSERT_STR_EQU(lines[2], "ghi", "wrong value");
  ASSERT_STR_EQU(lines[3], "", "wrong value");
  ASSERT_STR_EQU(lines[4], "", "wrong value");
  ASSERT_STR_EQU(lines[5], "jkl", "wrong value");

  n_lines = strnsplit_in_place(strdup("abc\ndef\nghi\n\njkl\n"), '\n', lines, 2);
  ASSERT_EQU(n_lines, 2, "wrong number of lines");
  ASSERT_STR_EQU(lines[0], "abc", "wrong value");
  ASSERT_STR_EQU(lines[1], "def", "wrong value");

  exit(0);
}
