#include "page_rank.h"

#include <stdlib.h>
#include <cmath>
#include <omp.h>
#include <utility>

#include "../common/CycleTimer.h"
#include "../common/graph.h"


// pageRank --
//
// g:           graph to process (see common/graph.h)
// solution:    array of per-vertex vertex scores (length of array is num_nodes(g))
// damping:     page-rank algorithm's damping parameter
// convergence: page-rank algorithm's convergence threshold
//
void pageRank(Graph g, double* solution, double damping, double convergence)
{


  // initialize vertex weights to uniform probability. Double
  // precision scores are used to avoid underflow for large graphs

  int numNodes = num_nodes(g);
  double equal_prob = 1.0 / numNodes;
  #pragma omp parallel for
  for (int i = 0; i < numNodes; ++i) {
    solution[i] = equal_prob;
  }

  double* tempArray = new double[g->num_nodes];

  bool converged = false;
  while (!converged) {
    #pragma omp parallel for default(none) shared(tempArray, g)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      tempArray[v] = 0.0;
    }

    #pragma omp parallel for default(none) shared(tempArray, g, solution)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      for (const Vertex* iv = incoming_begin(g, v); iv != incoming_end(g, v); ++iv) {
        tempArray[v] += solution[*iv] / static_cast<double>(outgoing_size(g, *iv));
      }
    }

    #pragma omp parallel for default(none) shared(tempArray, g, damping, numNodes)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      tempArray[v] = tempArray[v] * damping + (1.0 - damping) / static_cast<double>(numNodes);
    }

    double t = 0.0;
    #pragma omp parallel for default(none) reduction(+: t) shared(solution, g, numNodes, damping)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      if(outgoing_size(g, v) == 0) {
        t += damping * solution[v] / static_cast<double>(numNodes);
      }
    }

    #pragma omp parallel for default(none) shared(tempArray, g, t)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      tempArray[v] += t;
    }

    double globalDiff = 0.0;
    #pragma omp parallel for default(none) reduction(+: globalDiff) shared(tempArray, solution, g)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      globalDiff += abs(tempArray[v] - solution[v]);
    }

    #pragma omp parallel for default(none) shared(tempArray, solution, g)
    for (Vertex v = 0; v < g->num_nodes; ++v) {
      solution[v] = tempArray[v];
    }
    converged = globalDiff < convergence;
  }

  delete[] tempArray;
  
  /*
     CS149 students: Implement the page rank algorithm here.  You
     are expected to parallelize the algorithm using openMP.  Your
     solution may need to allocate (and free) temporary arrays.

     Basic page rank pseudocode is provided below to get you started:

     // initialization: see example code above
     score_old[vi] = 1/numNodes;

     while (!converged) {

       // compute score_new[vi] for all nodes vi:
       score_new[vi] = sum over all nodes vj reachable from incoming edges
                          { score_old[vj] / number of edges leaving vj  }
       score_new[vi] = (damping * score_new[vi]) + (1.0-damping) / numNodes;

       score_new[vi] += sum over all nodes v in graph with no outgoing edges
                          { damping * score_old[v] / numNodes }

       // compute how much per-node scores have changed
       // quit once algorithm has converged

       global_diff = sum over all nodes vi { abs(score_new[vi] - score_old[vi]) };
       converged = (global_diff < convergence)
     }

   */
}
