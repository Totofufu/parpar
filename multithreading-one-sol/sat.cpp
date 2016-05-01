#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <vector>
#include <string.h>
#include <pthread.h>
#include <thread>
#include <mutex>

#include "lib/work_queue.h"
#include "lib/parse.cpp"
#include "sat.h"

#define NUM_THREADS 24
#define UNUSED(x) (void)(x)

static struct Global_state {
  WorkQueue<int>* wqueue;
  bool done; // set to true once we attempt all possible solns or find a satisfiable soln
  bool success; // indicating whether current expression is satisfiable or not.
  std::vector <std::vector<int> > expr;
  int num_vars;
  std::mutex print_sol_mutex;
} gstate;


void print_solution(std::vector<int> sat_vals) {
  int i = 1;
  for (std::vector<int>::iterator it = sat_vals.begin(); it != sat_vals.end(); ++it) {
    int exp = *it;
    printf("x%d = ", i);
    if (exp == 1) printf("T");
    else printf("F");

    // deal with trailing comma at the very end
    if (i < sat_vals.size()) printf(", ");
    i++;
  }
  printf("\n");
}

bool is_satisfiable(std::vector<int> sat_vals) {
  bool satis = true;
  for (std::vector<std::vector<int> >::iterator outer = gstate.expr.begin(); outer != gstate.expr.end(); ++outer) {
    bool clause = false;
    for (std::vector<int>::iterator inner = outer->begin(); inner != outer->end(); ++inner) {
      // find index into sat_vals by taking abs value and then subtracting 1 to account for array zero-indexing
      int exp_index = abs(*inner)-1;
      bool exp_value = sat_vals.at(exp_index);
      if (*inner > 0)
        clause |= exp_value;
      else
        clause |= !exp_value; // account for literals that are negations of a variable
    }
    satis &= clause;
    if (!satis) return false;
  }
  return satis;
}

void* attempt_single_solution(void* args) {

  UNUSED(args);

  while (!gstate.done) {
    int rep_temp = gstate.wqueue->get_work();

    std::vector<int> sat_vals;

    // fill in the SAT expr with a brute force attempt
    for (int i = 0; i < gstate.num_vars; i++) {
      int bit = rep_temp & 0x1;
      sat_vals.push_back(bit);
      rep_temp >>= 1;
    }

    // check to see if the current SAT expression is satisfiable
    if (is_satisfiable(sat_vals)) {
      gstate.print_sol_mutex.lock();
      printf("Solution found!\n");
      print_solution(sat_vals);
      gstate.print_sol_mutex.unlock();
      gstate.done = true;
      gstate.success = true;
    }
  }

  return NULL;

}

// input form of SAT expression: 2,1 -2,3,-4
int main(int argc, char** argv) {
  // init all the gstate values first
  int* num_vars_ptr = new int;
  gstate.expr = parse(argc, argv, num_vars_ptr); // assume this is initialized
  gstate.num_vars = *num_vars_ptr; // set global num_vars here
  gstate.wqueue = new WorkQueue<int>();
  gstate.done = false;
  gstate.success = false;

  // contains values from 0 to 2^num_vars that represent all possible solutions
  std::vector<int> rep_nums;
  for (int i = 0; i < pow(2, gstate.num_vars); i++) {
    rep_nums.push_back(i);
  }

  // rep_temp represents the solution we're attempting
  for (int rep_temp = 0; rep_temp < pow(2, gstate.num_vars); rep_temp++) {
    gstate.wqueue->put_work(rep_temp);
  }

  pthread_t threads[NUM_THREADS];
  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, attempt_single_solution, NULL);
    pthread_detach(threads[t]);
  }

  /*for (int rep_temp = 0; rep_temp < pow(2, num_vars); rep_temp++) {
    std::vector<int> sat_vals;

    // fill in the SAT expr with a brute force attempt
    for (int i = 0; i < num_vars; i++) {
      int bit = rep_temp & 0x1;
      sat_vals.push_back(bit);
      rep_temp >>= 1;
    }

    // check to see if the current SAT expression is satisfiable
    if (is_satisfiable(expr, sat_vals)) {
      printf("Solution found!\n");
      print_solution(sat_vals);
      delete num_vars_ptr;
      return 0;
    }
  }*/

  if (!gstate.success) printf("No solution found.\n");
  delete num_vars_ptr;
  return 1;
}
