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
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <conio.h>
#include <apple2enh.h>
#include <tgi.h>
#endif
#include "array_sort.h"
#include "slist.h"

#if 0
//TEST
long ROWNUM = 10;
long search_x_min = 0, search_x_max = 20;
long search_y_min = 0, search_y_max = 20;
#define DATASET "IN15E"

#else

long ROWNUM = 2000000;
long search_x_min = 0, search_x_max = 4000000;
long search_y_min = 0, search_y_max = 4000000;
#define DATASET "IN15"


#endif

#define BUFSIZE 300

static void read_file(FILE *fp);

int main(void) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  fp = fopen(DATASET, "r");
  if (fp == NULL) {
    printf("Error %d\n", errno);
    exit(1);
  }

  read_file(fp);

  exit (0);
}

typedef struct _report {
  long sx;
  long sy;
  long bx;
  long by;
} report;

typedef struct _range {
  long start;
  long end;
} range;

slist *reports = NULL;
slist *beacons = NULL;

int num_ranges = 0;

static int range_compare(void *a, void *b) {
  range *ra = (range *)a;
  range *rb = (range *)b;
  if (ra-> start < rb->start)
    return -1;
  else if (ra-> start > rb->start)
    return 1;
  else
    return 0;
}

static int count_beacons_in_range(slist *beacons, long y, long start_x, long end_x) {
  slist *w;
  int count = 0;
  for (w = beacons; w; w = w->next) {
    report *r = (report *)w->data;
    if (r->by != y) {
      continue;
    }
    if (start_x <= r->bx && r->bx <= end_x) {
      count ++;
    }
  }
  return count;
}

static slist *beacon_add_unique(slist *beacons, report *r) {
  slist *w;
  for (w = beacons; w; w = w->next) {
    report *o = (report *)w->data;
    if (o->bx == r->bx && o->by == r->by) {
      return beacons;
    }
  }
  return slist_prepend(beacons, r);
}

static int out_of_sensor_ranges(slist *reports, long x, long y) {
  slist *w;
  if (x < search_x_min || x > search_x_max
   || y < search_y_min || y > search_y_max) {
   return 0;
  }

  for (w = reports; w; w = w->next) {
    report *r = (report *)w->data;
    long x_dist   = labs(r->sx - r->bx);
    long y_dist   = labs(r->sy - r->by);
    long known_man_dist = x_dist + y_dist;
    long test_x_dist = labs(r->sx - x);
    long test_y_dist = labs(r->sy - y);
    long test_man_dist = test_x_dist + test_y_dist;
    
    if (known_man_dist >= test_man_dist) {
      return 0;
    }
  }
  
  return 1;
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  long sum1 = 0, last_end, last_end_set = 0;
  char *wxs, *wys, *wxb, *wyb;
  range **ranges = malloc(30 * sizeof(range *));
  slist *w;
  int i, n_beacons;
  long tuning = 0;

  do {
    report *r;

    if (fgets(buf, BUFSIZE-1, fp) == NULL)
      break;

    r = malloc(sizeof(struct _report));

    /* Skip to coords */
    wxs = strchr(buf, '=') + 1;
    wys = strchr(wxs, '=') + 1;
    wxb = strchr(wys, '=') + 1;
    wyb = strchr(wxb, '=') + 1;
    
    /* zero out after numbers */
    *(strchr(wxs,',')) = '\0' ;
    *(strchr(wys,':')) = '\0' ;
    *(strchr(wxb,',')) = '\0' ;
    *(strchr(wyb,'\n')) = '\0' ;

    /* parse ints */
    r->sx = atol(wxs);
    r->sy = atol(wys);
    r->bx = atol(wxb);
    r->by = atol(wyb);
    
    reports = slist_prepend(reports, r);
    beacons = beacon_add_unique(beacons, r);

    printf("Sensor at %ld,%ld, beacon at %ld,%ld\n", r->sx, r->sy, r->bx, r->by);
  } while (1);
  printf("\n");

  num_ranges = 0;

/* Part 1*/
  for (w = reports; w; w = w->next) {
    report *r = (report *)w->data;
    long x_dist   = labs(r->sx - r->bx);
    long y_dist   = labs(r->sy - r->by);
    long man_dist = x_dist + y_dist;
    long delta_y, row_dist;
    long sx_left, sx_right;
    long x, y;

    printf("Doing report: Sensor at %ld,%ld, beacon at %ld,%ld\n",
          r->sx, r->sy, r->bx, r->by);

    delta_y = labs(r->sy - ROWNUM);
    if (delta_y > man_dist) {
      printf("  Row %ld too far.\n", ROWNUM);
      continue;
    }

    row_dist = labs(man_dist - delta_y);

    printf(" X distance is %lu, Y distance %ld, Manhattan distance %ld, delta y %ld, row_dist %ld\n",
          x_dist, y_dist, man_dist, delta_y, row_dist);

    if (row_dist > 0) {
      ranges[num_ranges] = malloc(sizeof(struct _range));
      ranges[num_ranges]->start = r->sx - row_dist;
      ranges[num_ranges]->end = r->sx + row_dist;

      printf(" added range %ld => %ld\n", ranges[num_ranges]->start, ranges[num_ranges]->end);
      num_ranges++;
    }

    /* Part 2 */
#ifndef __CC65__
    if (tuning == 0) {
      sx_left = r->sx - man_dist - 1;
      sx_right = r->sx + man_dist + 1;
      for (x = sx_left; x <= sx_right; x++) {
        long y_up, y_down;
        if (x <= r->sx) {
          y_up = r->sy - (sx_left - x);
          y_down = r->sy + (sx_left - x);
        } else if (x > r->sx) {
          y_up = r->sy - (x - sx_right);
          y_down = r->sy + (x - sx_right);
        }
        
        if (out_of_sensor_ranges(reports, x, y_up)) {
          tuning = (4000000*x) + y_up;
        } else if (out_of_sensor_ranges(reports, x, y_down)) {
          tuning = (4000000*x) + y_down;
        }
      }
    }
  #endif
}

  bubble_sort_array((void **)ranges, num_ranges, range_compare);
  
  for (i = 0; i < num_ranges; i++) {
    printf("Range %ld => %ld", ranges[i]->start, ranges[i]->end);
    if (last_end_set == 0) {
      last_end_set = 1;
    } else {
      if (ranges[i]->start <= last_end) {
        ranges[i]->start = last_end +1;
        printf(" (Shifting to %ld => %ld)\n", ranges[i]->start, ranges[i]->end);
        if (ranges[i]->start > ranges[i]->end) {
          printf("    Skipping altogether\n");
          free(ranges[i]);
          continue;
        }
      } else {
        printf("\n");
      }
    }
    printf(" Range length %ld\n",labs(ranges[i]->end - ranges[i]->start) + (long)1);
    n_beacons = count_beacons_in_range(beacons, ROWNUM, ranges[i]->start, ranges[i]->end);
    printf(" Substracting %d beacon(s)\n", n_beacons);
    sum1 += labs(ranges[i]->end - ranges[i]->start) + 1 - n_beacons;
    last_end = ranges[i]->end;
    free(ranges[i]);
  }

  printf("Part 1: %ld\n", sum1);
  printf("Part 2: %ld\n", tuning);

/* All done */
  for (w = reports; w; w = w->next) {
    free(w->data);
  }
  slist_free(reports);

  free(ranges);
  free(buf);
  fclose(fp);
}
