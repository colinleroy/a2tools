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

#define DATASET "IN16"

int main(int argc, char **argv) {
  FILE *fp;

#ifdef PRODOS_T_TXT
  _filetype = PRODOS_T_TXT;
#endif
  if (argc > 1)
    fp = fopen(argv[1],"r");
  else
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

static int get_valve_by_name(const char *name) {
  int i;
  for (i = 0; i < num_valves; i++) {
    if (!strcmp(name, valve_name[i]))
      return i;
  }
  return -1;
}

static short find_closest_valve(short start_valve, char *visited) {
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
  char *visited = malloc(num_valves * sizeof(char));
  int i;
  short cur_len = 0;
  slist *queue = NULL, *w;

  memset(visited, 0, num_valves * sizeof(char));
  queue = slist_append(queue, (void *)(long)start_valve);
  distances[start_valve][start_valve] = 0;
  visited[start_valve] = 1;

  while (queue) {
    short next_valve = (short)(queue->data);
    queue = slist_remove(queue, queue);
    cur_len = distances[start_valve][next_valve] + 1;
    for (i = 0; i < valve_num_destinations[next_valve]; i++) {
      short dest_valve = valve_destinations[next_valve][i];
      if (!visited[dest_valve]) {
        distances[start_valve][dest_valve] = cur_len;
        printf("distance %s => %s = %d\n", valve_name[start_valve], valve_name[dest_valve], cur_len);
        visited[dest_valve] = 1;
        queue = slist_append(queue, (void *)(long)dest_valve);
      }
    }
    cur_len++;
  }
  // memset(visited, 0, num_valves * sizeof(char));
  // distances[start_valve][start_valve] = 0;
  // 
  // while (1) {
  //   short next_valve = find_closest_valve(start_valve, visited);
  //   if (next_valve < 0) {
  //     break;
  //   }
  //   cur_len = distances[start_valve][next_valve];
  //   visited[next_valve] = 1;
  // 
  //   for (i = 0; i < valve_num_destinations[next_valve]; i++) {
  //     short dest_valve = valve_destinations[next_valve][i];
  //     distances[next_valve][dest_valve] = 1;
  //     if (distances[start_valve][dest_valve] == -1
  //      || distances[start_valve][dest_valve] > cur_len + 1) {
  //        distances[start_valve][dest_valve] = cur_len + 1;
  //        printf("distance %s => %s = %d\n", valve_name[start_valve], valve_name[dest_valve], cur_len + 1);
  //      }
  //   }
  // }
  free(visited);
  visited = NULL;
}

static int find_optimal_flow(short start_valve, short time, char *targets, int depth) {
  int optimal_flow = 0, i;
  char *sub_targets;
  char *prefix = malloc(depth+2);
  int time_rem;
  for (i = 0; i < depth; i++) {
    prefix[i] = ' ';
  }
  prefix[i] = '\0';

  printf("%stargets from %s: [", prefix, valve_name[start_valve]);
  for (i = 0; i < num_valves; i++) {
    if (targets[i])
      printf("%s,", valve_name[i]);
  }
  printf("\n");

  sub_targets = malloc(num_valves * sizeof(char));
  memcpy(sub_targets, targets, num_valves * sizeof(char));
  sub_targets[start_valve] = 0;

  for (i = 0; i < num_valves; i++) {
    if (sub_targets[i] == 0) {
      /* we don't care */
      continue;
    }

    /* time to travel + time to open */
    time_rem = time - distances[start_valve][i] - 1;
    if (time_rem > 0) {
      int path_flow = valve_flow[i] * time_rem;
      printf("%sopen valve %s (%d * %d = %d) + \n", prefix, valve_name[i], valve_flow[i], time_rem, valve_flow[i] * time_rem);
      path_flow += find_optimal_flow(i, time_rem, sub_targets, depth + 1);
      printf("%stotal = %d\n", prefix, path_flow);
      if (path_flow > optimal_flow) {
        optimal_flow = path_flow;
      }
    }
  }
  free(sub_targets);

  printf("%soptimum found = %d\n", prefix, optimal_flow);
  free(prefix);
  return optimal_flow;
}

static void read_file(FILE *fp) {
  char *buf = malloc(BUFSIZE);
  char **valve_destinations_str = NULL;
  short count = 0;
  char *targets;
  short start_valve;

  while (fgets(buf, BUFSIZE-1, fp) != NULL) {
    char *name = strchr(buf, ' ') + 1;
    char *flow_rate = strchr(buf, '=') + 1;
    char *paths_str = strstr(buf, "to valve") + strlen("to valve");
    char **paths = NULL;
    int i, num_paths;
    char *targets;

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
  targets = malloc(num_valves * sizeof(char));
  memset(targets, 0, num_valves * sizeof(char));

  start_valve = get_valve_by_name("AA");
  bfs_distances(start_valve);
  for (count = 0; count < num_valves; count++) {
    if (valve_flow[count] > 0) {
      targets[count] = 1;
    }
    if (count != start_valve) {
      bfs_distances(count);
    }
  }
  int num_c = 0;
  for (count = 0; count < num_valves; count++) {
    int i;
    for (i = 0; i < num_valves; i++) {
      if (distances[count][i] < 0) {
        printf("Graph not fully connected ! (%s => %s missing)\n", valve_name[count], valve_name[i]);
      }
      else if (count != i && distances[count][i] == 0) {
        printf("Graph not fully connected ! (%s => %s is 0)\n", valve_name[count], valve_name[i]);
      } else {
        num_c++;
      }
    }
  }
  printf("Graph fully connected with %d (num_valves %d) connections\n", num_c, num_valves);
  targets[start_valve] = 1;
  printf("best flow in part 1: %d\n", find_optimal_flow(start_valve, 30, targets, 0));
  
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
