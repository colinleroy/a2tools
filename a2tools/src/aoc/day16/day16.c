#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#ifdef __CC65__
#include <apple2.h>
#endif
#include "array_sort.h"
#include "extended_string.h"
#include "slist.h"

#define BUFSIZE 255
static void read_file(FILE *fp);

#define DATASET "IN16E"

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

static char **valve_name = NULL;
static short **valve_destinations = NULL;
static short *valve_num_destinations = NULL;
static short *valve_flow;
static short num_valves;
static short **distances;
static char *visited;

static int get_valve_by_name(const char *name) {
  int i;
  for (i = 0; i < num_valves; i++) {
    if (!strcmp(name, valve_name[i]))
      return i;
  }
  return -1;
}

static short find_closest_valve(short start_valve) {
  int min_dist = -1;
  int i;
  short closest = -1;

  for (i = 0; i < num_valves; i++) {
    if (visited[i]) {
      continue;
    }
    if (min_dist < distances[start_valve][i] || min_dist == -1) {
      min_dist = distances[start_valve][i];
      closest = i;
    }
  }
  return closest;
}
static void bfs_distances(short start_valve) {
  
  memset(visited, 0, num_valves * sizeof(char));
  distances[start_valve][start_valve] = 0;
  
  while (1) {
    short next_valve = find_closest_valve(start_valve);
    int i;
    short cur_len = -1;
    if (next_valve < 0) {
      return;
    }

    cur_len = distances[start_valve][next_valve];
    visited[next_valve] = 1;

    for (i = 0; i < valve_num_destinations[next_valve]; i++) {
      short dest_valve = valve_destinations[next_valve][i];
      distances[next_valve][dest_valve] = 1;
      printf("distance %s => %s = 1\n", valve_name[next_valve], valve_name[dest_valve]);
      
      if (distances[start_valve][dest_valve] == -1
       || distances[start_valve][dest_valve] > cur_len + 1) {
         distances[start_valve][dest_valve] = cur_len + 1;
         printf("distance %s => %s = %d\n", valve_name[start_valve], valve_name[dest_valve], cur_len + 1);
       }
    }
  }
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  char **valve_destinations_str = NULL;
  short count = 0;

  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    char *name = strchr(buf, ' ') + 1;
    char *flow_rate = strchr(buf, '=') + 1;
    char *paths_str = strstr(buf, "to valve") + strlen("to valve");
    char **paths = NULL;
    int i, num_paths;

    *strchr(name, ' ') ='\0';
    *strchr(flow_rate, ';') = '\0';
    paths_str = strchr(paths_str, ' ') +1;

    valve_name             = realloc(valve_name, (count + 1) * sizeof(char *));
    valve_destinations_str = realloc(valve_destinations_str, (count + 1) * sizeof(char *));
    valve_destinations     = realloc(valve_destinations, (count + 1) * sizeof(short *));
    valve_num_destinations = realloc(valve_num_destinations, (count + 1) * sizeof(short));
    valve_flow             = realloc(valve_flow, (count + 1) * sizeof(short));

    valve_name[count] = strdup(name);
    valve_destinations_str[count] = strdup(paths_str);
    valve_flow[count] = atoi(flow_rate);

    count++;
  }
  free(buf);
  fclose(fp);

  num_valves = count;
  for (count = 0; count < num_valves; count++) {
    int num_dest, i;
    char **dests;

    num_dest = strsplit(valve_destinations_str[count], ' ', &dests);
    valve_destinations[count] = malloc(num_dest*sizeof(int));
    valve_num_destinations[count] = num_dest;
    for (i = 0; i < num_dest; i++) {
      if (strchr(dests[i],','))
        *strchr(dests[i],',') = '\0';
      if (strchr(dests[i],'\n'))
        *strchr(dests[i],'\n') = '\0';

      valve_destinations[count][i] = get_valve_by_name(dests[i]);
      free(dests[i]);
    }
    free(valve_destinations_str[count]);
    free(dests);
  }
  free(valve_destinations_str);

  distances = malloc(num_valves * sizeof(short *));
  visited = malloc(num_valves * sizeof(char));

  for (count = 0; count < num_valves; count++) {
    int num_dest, i;
    printf("valve %d: %s, flow %d, destinations: ", count, valve_name[count], valve_flow[count]);
    for (i = 0; i < valve_num_destinations[count]; i++) {
      printf("%d (%s),", valve_destinations[count][i], valve_name[valve_destinations[count][i]]);
    }
    printf("\n");
    distances[count] = malloc(num_valves * sizeof(short));
    for (i = 0; i < num_valves; i++) {
      distances[count][i] = -1;
    }
  }
  
  /* Do the thing */
  bfs_distances(0);
  
  /* free; keep Valgrind happy */
  for (count = 0; count < num_valves; count++) {
    free(distances[count]);
    free(valve_name[count]);
    free(valve_destinations[count]);
  }
  free(distances);
  free(valve_name);
  free(valve_destinations);
  free(valve_num_destinations);
  free(valve_flow);
}
