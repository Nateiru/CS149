#include "bfs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cstddef>
#include <omp.h>

#include "../common/CycleTimer.h"
#include "../common/graph.h"

#define ROOT_NODE_ID 0
#define NOT_VISITED_MARKER -1

void vertex_set_clear(vertex_set *list) {
  list->count = 0;
}

void vertex_set_init(vertex_set *list, int count) {
  list->max_vertices = count;
  list->vertices = (int *)malloc(sizeof(int) * list->max_vertices);
  vertex_set_clear(list);
}

// Take one step of "top-down" BFS.  For each vertex on the frontier,
// follow all outgoing edges, and add all neighboring vertices to the
// new_frontier.
void top_down_step(
  Graph g,
  vertex_set *frontier,
  vertex_set *localList,
  int* count,
  int *distances) {
  
  // 需要 for 循环更新每一个点，此过程是可以并行的
  #pragma omp parallel for
  for (int i = 0; i < frontier->count; i++) {

    int node = frontier->vertices[i];

    int start_edge = g->outgoing_starts[node];
    int end_edge = (node == g->num_nodes - 1)
                   ? g->num_edges
                   : g->outgoing_starts[node + 1];

    // attempt to add all neighbors to the new frontier
    for (int neighbor = start_edge; neighbor < end_edge; neighbor++) {
      int outgoing = g->outgoing_edges[neighbor];

      if (distances[outgoing] == NOT_VISITED_MARKER &&
          __sync_bool_compare_and_swap(&distances[outgoing], NOT_VISITED_MARKER, distances[node] + 1)) {

        int index = localList[omp_get_thread_num()].count++;
        localList[omp_get_thread_num()].vertices[index] = outgoing;
      }
    }
  }
  int totalCount = 0;
  for (int i = 0; i < omp_get_max_threads(); ++i) {
    count[i] = totalCount;
    totalCount += localList[i].count;
  }
  frontier->count = totalCount;

  // Parallel copy the data from `localList` to `frontier`
  #pragma omp parallel for
  for (int i = 0; i < omp_get_max_threads(); ++i) {
    memcpy(frontier->vertices + count[i], localList[i].vertices, localList[i].count * sizeof(int));
  }
}

// Implements top-down BFS.
//
// Result of execution is that, for each node in the graph, the
// distance to the root is stored in sol.distances.
void bfs_top_down(Graph graph, solution *sol) {

  // We only need one list to represent the current frontier.
  // And when we handle things concurrently, we should write the results back to list.
  // list -> localList[omp_get_max_threads()] -> list
  vertex_set list;
  vertex_set_init(&list, graph->num_nodes);

  // For each thread, in order not to use any locks. We use local storage.
  int maxThreadNum = omp_get_max_threads();
  vertex_set localList[maxThreadNum];
  // We need to store the count, this is a state which is necessary
  int count[maxThreadNum];

  #pragma omp parallel for
  for(int i = 0; i < maxThreadNum; ++i) {
    vertex_set_init(&localList[i], graph->num_nodes);
  }

  vertex_set* frontier = &list;
	#pragma omp parallel for
  // initialize all nodes to NOT_VISITED
  for (int i = 0; i < graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;

  // setup frontier with the root node
  frontier->vertices[frontier->count++] = ROOT_NODE_ID;
  sol->distances[ROOT_NODE_ID] = 0;

  while (frontier->count != 0) {

#ifdef VERBOSE
    double start_time = CycleTimer::currentSeconds();
#endif

  // We need to reset the `count` for `localList` every time
    #pragma omp parallel for
    for (int i = 0; i < maxThreadNum; ++i) {
      vertex_set_clear(&localList[i]);
    }

    top_down_step(graph, frontier, localList, count, sol->distances);

#ifdef VERBOSE
    double end_time = CycleTimer::currentSeconds();
    printf("frontier=%-10d %.4f sec\n", frontier->count, end_time - start_time);
#endif

  }
}

uint bottom_up_step(Graph g, bool* frontier, bool* new_frontier, int* distances) {
  uint count = 0;
  #pragma omp parallel for reduction(+: count)
  for (int i = 0; i < g->num_nodes; ++i) {
    if (distances[i] == NOT_VISITED_MARKER) {
      for (const Vertex* incoming = incoming_begin(g, i); incoming != incoming_end(g, i); ++incoming) {
        if (frontier[*incoming]) {
          distances[i] = distances[*incoming] + 1;
          new_frontier[i] = true;
          count += 1;
          break;
        }
      }
    }
  }
  return count;
}

void bfs_bottom_up(Graph graph, solution *sol) {
  bool *frontier = new bool[graph->num_nodes];
  bool *new_frontier = new bool[graph->num_nodes];

  memset(frontier, 0, sizeof(bool) * graph->num_nodes);

  // Set for the root node
  frontier[ROOT_NODE_ID] = true;

  #pragma omp parallel for
  // initialize all nodes to NOT_VISITED
  for (int i=0; i<graph->num_nodes; i++)
    sol->distances[i] = NOT_VISITED_MARKER;
  sol->distances[ROOT_NODE_ID] = 0;

  uint count = 1;

  while (count != 0) {
    memset(new_frontier, 0, sizeof(bool) * graph->num_nodes);
    count = bottom_up_step(graph, frontier, new_frontier, sol->distances);
    // Swap the pointer
    bool * temp = frontier;
    frontier = new_frontier;
    new_frontier = temp;
  }

  delete [] frontier;
  delete [] new_frontier;
}

void bfs_hybrid(Graph graph, solution *sol) {
  // CS149 students:
  //
  // You will need to implement the "hybrid" BFS here as
  // described in the handout.
}