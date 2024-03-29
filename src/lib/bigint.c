/*
 * Copyright (C) 2022 Colin Leroy-Mira <colin@colino.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "bigint.h"

bigint *bigint_new(const char *a) {
  return strdup(a);
}

bigint *bigint_new_from_long(long a) {
  char *buf = malloc(255);
  if (buf == NULL) {
    printf("bigint_new_from_long: cannot malloc\n");
    return NULL;
  }
  sprintf(buf, "%ld", a);
  return buf;
}

char *trim_leading_zeroes(char *a) {
  int i = 0;
  char *out;
  while (a[i] == '0')
    i++;
  if (i == 0)
    return a;
  
  out = strdup(a + i);
  if (out == NULL) {
    printf("trim_leading_zeroes: cannot malloc\n");
    return NULL;
  }
  free(a);
  return out;
}

char *bigint_add(const char *a, const char *b) {
  int i = strlen(a) - 1;
  int j = strlen(b) - 1;
  int len = (i > j ? i : j) + 3; /* extra digit + nul */
  int l = len - 1;
  char *result = malloc(len);
  int carry = 0;
  
  if (result == NULL) {
    printf("bigint_add: cannot malloc\n");
    return NULL;
  }

  result[l] = '\0';

  for (; i >= 0 || j>= 0; i--, j--) {
    int a_digit = (i >= 0 ? a[i] : '0') - '0';
    int b_digit = (j >= 0 ? b[j] : '0') - '0';
    int sum = (a_digit + b_digit + carry);
    l--;
    result[l] = (sum % 10) + '0';
    carry = (sum > 9) ? 1 : 0;
  }
  l--;
  result[l] = carry + '0';
  if (l != 0) {
    printf("cannot add %s+%s, have l %d\n", a, b, l);
    free(result);
    return NULL;
  }

  return trim_leading_zeroes(result);
}

int bigint_bigger(const char *a, const char *b){
  int i = strlen(a);
  int j = strlen(b);
  int c, result = 0;
  int both_negs = 0;

  /* a neg and b pos */
  if (a[0] == '-' && b[0] != '-') {
    result = 0; goto out_bigger;
  }
  
  /* a pos and b neg */
  if (a[0] != '-' && b[0] == '-') {
    result = 1; goto out_bigger;
  }
  /* they have the same sign. */
  both_negs = (a[0] == '-');

  /* a longer */
  if (i > j) {
    result = !both_negs; goto out_bigger;
  }
  /* a shorter */
  if (i < j) {
    result = both_negs; goto out_bigger;
  }
  
  /* same length. compare by digit */
  for (c = 0; c < i; c++) {
    if (a[c] == b[c]) {
      continue;
    }

    if (a[c] > b[c]) {
      result = !both_negs; goto out_bigger;
    } else {
      result = both_negs; goto out_bigger;
    }
  }
out_bigger:
  return result;
}

char *bigint_sub(const char *a, const char *b) {
  int i = strlen(a) - 1;
  int j = strlen(b) - 1;
  int len = (i > j ? i : j) + 3; /* extra digit + nul */
  int l = len - 1;
  char *result;
  int carry = 0;

  if (!strcmp(a, b)) {
    return bigint_new("0");
  }

  if (!bigint_bigger(a, b)) {
    char *final;

    result = bigint_sub(b, a);
    if (result == NULL) return NULL;
    final = malloc(strlen(result) + 2);

    if (final == NULL) {
      printf("bigint_sub: cannot malloc\n");
      return NULL;
    }


    final[0] = '-';
    strcpy(final + 1, result);
    
    free(result);
    return final;
  }

  result = malloc(len);
  if (result == NULL) {
    printf("bigint_sub: cannot malloc\n");
    return NULL;
  }
  result[l] = '\0';

  for (; i >= 0 || j>= 0; i--, j--) {
    int a_digit = (i >= 0 ? a[i] : '0') - '0';
    int b_digit = (j >= 0 ? b[j] : '0') - '0';
    int sum = (a_digit - b_digit - carry);
    if (sum < 0) {
      sum = sum + 10;
      carry = 1;
    } else {
      carry = 0;
    }
    l--;
    result[l] = sum + '0';
  }
  l--;
  result[l] = carry + '0';
  if (l != 0) {
    printf("cannot sub %s-%s, have l %d\n", a, b, l);
    free(result);
    return NULL;
  }
  i = 0;
  
  return trim_leading_zeroes(result);
}

char *bigint_mul(const char *a, const char *b) {
  int i = strlen(a) - 1;
  int j = strlen(b) - 1;
  int ini_i = i;
  int len = (i + 1) + 2; /* extra digit + nul */
  int l, l2;
  char *result;
  int carry = 0;
  int num_done = 0;
  char *total = NULL, *tmp;
  int is_neg = 0;

  while (j >= 0) {
    if (j == 0 && b[j] == '-') {
      is_neg = 1 - is_neg;
      goto end_mul;
    }
    l = len - 1;
    result = malloc(len);

    if (result == NULL) {
      printf("bigint_mul: cannot malloc\n");
      return NULL;
    }
    
    result[l] = '\0';

    for (; i >= 0; i--) {
      int a_digit = a[i] - '0';
      int b_digit = b[j] - '0';
      int mul = (a_digit * b_digit) + carry;
      if (i == 0 && a[i] == '-') {
        is_neg = 1;
      } else {
        l--;
        result[l] = (mul % 10) + '0';
        carry = (mul / 10);
      }
    }
    l--;
    result[l] = carry + '0';
    carry = 0;
    while(l > 0) {
      l--;
      result[l] = '0';
    }
    
    l2 = strlen(result);
    tmp = malloc(l2 + num_done + 1);
    
    if (tmp == NULL) {
      printf("bigint_mul: cannot malloc tmp\n");
      free(total);
      return NULL;
    }

    strcpy(tmp, result);
    for (i = 0; i < num_done; i++) {
      tmp[i+l2]='0';
    }
    tmp[l2+num_done] = '\0';
    num_done ++;

    free(result);
    result = trim_leading_zeroes(tmp);

    if (total == NULL) {
      total = strdup(result);
      if (total == NULL) {
        printf("cannot strdup total\n");
        free(result);
        return NULL;
      }
    } else {
      tmp = bigint_add(total, result);
      if (tmp == NULL) return NULL;
      free(total);
      total = tmp;
    }
    free(result);

    j--;
    i = ini_i;
  }
end_mul:
  if (is_neg && total) {
    tmp = trim_leading_zeroes(total);
    result = malloc(strlen(tmp)+2);
    if (result == NULL) {
      free(tmp);
      printf("bigint_mul: cannot malloc\n");
      return NULL;
    }
    result[0] = '-';
    strcpy(result + 1, tmp);
    free(tmp);
    return result;
  }
  return trim_leading_zeroes(total);
}

char *bigint_div(char *a, char *b) {
  char *num;
  char *counter;
  char *tmp;
  int a_len, b_len;
  char *tmp_a = a, *tmp_b = b;
  int num_neg = 0;

  if (a[0] == '-') {
    tmp_a = (a + 1);
    num_neg++;
  }
  if (b[0] == '-') {
    tmp_b = (b + 1);
    num_neg++;
  }
  if (num_neg > 0) {
    bigint *pos_res = bigint_div(tmp_a, tmp_b);
    if (pos_res == NULL) return NULL;
    if (num_neg == 1) {
      tmp = malloc(strlen(pos_res) + 2);

      if (tmp == NULL) {
        printf("bigint_div: cannot malloc\n");
        return NULL;
      }

      tmp[0] = '-';
      strcpy(tmp + 1, pos_res);
      free(pos_res);
      pos_res = tmp;
    }
    return pos_res;
  }

  /* Approximate */
  a_len = strlen(a);
  b_len = strlen(b);
  
  if (a_len < b_len) {
    return strdup("0");
  } else if (a_len - b_len < 2) {
    goto div_precise;
  } else {
    int len_diff = a_len - b_len, i;
    char *len_order;
    char *div_order = malloc(1 + len_diff + 1);
    bigint *order_div, *remainder, *removed, *tmp, *quotient, *result;
    bigint *sub_div;

    if (div_order == NULL) {
      printf("bigint_div: cannot malloc\n");
      return NULL;
    }

    div_order[0] = '1';
    for (i = 1; i < len_diff; i++) {
      div_order[i] = '0';
    }
    div_order[i] = '\0';

    len_order = bigint_mul(div_order, b);
    if (len_order == NULL) return NULL;
    order_div = bigint_div(a, len_order);
    if (order_div == NULL) return NULL;
    free(len_order);
    tmp = bigint_mul(order_div, b);
    if (tmp == NULL) return NULL;
    removed = bigint_mul(tmp, div_order);
    free(tmp);
    if (removed == NULL) return NULL;
    quotient = bigint_mul(order_div, div_order);
    free(div_order);
    free(order_div);
    if (quotient == NULL) return NULL;
    remainder = bigint_sub(a, removed);
    free(removed);
    if (remainder == NULL) return NULL;
    
    sub_div = bigint_div(remainder, b);
    free(remainder);
    if (sub_div == NULL) return NULL;
    result = bigint_add(quotient, sub_div);

    free(sub_div);
    free(quotient);
    return result;
  }

div_precise:
  counter = bigint_new("0");
  if (counter == NULL) return NULL;
  num = bigint_new(a);
  if (num == NULL) return NULL;
  while(1) {
    tmp = bigint_sub(num, b);
    free(num);
    if (tmp == NULL) return NULL;
    num = tmp;

    if (!strcmp(num, "0") || bigint_bigger(num, "0")) {
      tmp = bigint_add(counter, "1");
      free(counter);
      if (tmp == NULL) return NULL;
      counter = tmp;
    } else {
      break;
    }
  }
  free(num);
  return counter;
}

char *bigint_mod(char *a, char *b) {
  char *q;
  char *round;
  char *r;
  q = bigint_div(a, b);
  if (q == NULL) return NULL;
  round = bigint_mul(q, b);
  free(q);
  if (round == NULL) return NULL;
  r = bigint_sub(a, round);
  free(round);
  if (r == NULL) return NULL;
  
  return r;
}
