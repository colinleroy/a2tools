#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <conio.h>
#include <apple2.h>
#include <tgi.h>
#endif
#include "array_sort.h"
#include "slist.h"

#define DATASET "IN15"
#define BUFSIZE 300
long ROWNUM = 2000000;

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

  printf("Ready.\n");

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

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  long sum = 0, last_end, last_end_set = 0;
  char *wxs, *wys, *wxb, *wyb;
  range **ranges = malloc(30 * sizeof(range *));
  slist *w;
  int i, n_beacons;

  do {
    report *r = malloc(sizeof(struct _report));

    if (fgets(buf, BUFSIZE-1, fp) == NULL)
      break;
    
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
    r->sx = atoi(wxs);
    r->sy = atoi(wys);
    r->bx = atoi(wxb);
    r->by = atoi(wyb);
    
    reports = slist_prepend(reports, r);
    beacons = beacon_add_unique(beacons, r);

    printf("Sensor at %s,%s, beacon at %s,%s\n", wxs, wys, wxb, wyb);
  } while (1);
  printf("\n");

  num_ranges = 0;

  for (w = reports; w; w = w->next) {
    report *r = (report *)w->data;
    long x_dist   = abs(r->sx - r->bx);
    long y_dist   = abs(r->sy - r->by);
    long man_dist = x_dist + y_dist;
    long delta_y, row_dist;

    printf("Doing report: Sensor at %ld,%ld, beacon at %ld,%ld\n",
          r->sx, r->sy, r->bx, r->by);

    delta_y = abs(r->sy - ROWNUM);
    if (delta_y > man_dist) {
      printf("  Row %ld too far.\n", ROWNUM);
      continue;
    }

    row_dist = abs(man_dist - delta_y);

    printf(" X distance is %lu, Y distance %ld, Manhattan distance %ld, delta y %ld, row_dist %ld\n",
          x_dist, y_dist, man_dist, delta_y, row_dist);

    if (row_dist > 0) {
      ranges[num_ranges] = malloc(sizeof(struct _range));
      ranges[num_ranges]->start = r->sx - row_dist;
      ranges[num_ranges]->end = r->sx + row_dist;

      printf(" added range %ld => %ld\n", ranges[num_ranges]->start, ranges[num_ranges]->end);
      num_ranges++;
    }
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
          continue;
        }
      } else {
        printf("\n");
      }
    }
    printf(" Range length %ld\n",abs(ranges[i]->end - ranges[i]->start) + (long)1);
    n_beacons = count_beacons_in_range(beacons, ROWNUM, ranges[i]->start, ranges[i]->end);
    printf(" Substracting %d beacon(s)\n", n_beacons);
    sum += abs(ranges[i]->end - ranges[i]->start) + 1 - n_beacons;
    last_end = ranges[i]->end;
    free(ranges[i]);
  }
  printf("Total: %ld\n", sum);
  free(ranges);
  free(buf);
  fclose(fp);
}
