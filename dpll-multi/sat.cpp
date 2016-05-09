#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <vector>
#include <string.h>
#include <string>
#include <map>
#include <set>
#include <utility>
#include <algorithm>
#include <thread>
#include <mutex>

#include "lib/parse.cpp"
#include "lib/my_queue.h"
#include "lib/my_stack.h"
#include "sat.h"

#define NUM_THREADS 128
#define UNUSED(x) (void)(x)

static struct Global_state {
  MQueue<int>* success_queue;
  MStack<work_t>* wstack;
  bool success;
  bool done;
  std::mutex print_sol_mutex;
} gstate;


// BEGIN DEBUGGERS //

// Prints out everything in the vars map.
void debug_vars(std::map<int, std::pair<std::set<int>, std::set<int> > > vars) {
  for (std::map<int, std::pair<std::set<int>, std::set<int> > >::iterator it = vars.begin(); it != vars.end(); ++it) {
    std::cout << "\n\nVAR " << it->first << ": ";
    for (std::set<int>::iterator pos=(it->second).first.begin(); pos != (it->second).first.end(); ++pos) {
      std::cout << *pos << " ";
    }
    std::cout << "\n\n~VAR " << it->first << ": ";
    for (std::set<int>::iterator neg=(it->second).second.begin(); neg != (it->second).second.end(); ++neg) {
      std::cout << *neg << " ";
    }
  }
  std::cout << "\n\n";
}

void scratch_maps(std::map<int, std::set<int> > clauses,
    std::map<int, std::pair<std::set<int>, std::set<int> > > map) {
  std::map<int, std::set<int> > new_clauses = clauses;
  clauses.erase (1);

  std::cout << "Original:\n";
  for (std::map<int, std::set<int> >::iterator it = clauses.begin(); it != clauses.end(); ++it) {
    std::set<int> elems = it->second;
    std::cout << "elems is size " << elems.size() << "\n";
    for (std::set<int>::iterator elem = elems.begin(); elem != elems.end(); ++elem) {
      std::cout << *elem << " ";
    }
    std::cout << "\n";
  }
  std::cout << "\n\n";
  std::cout << "Copy:\n";
  for (std::map<int, std::set<int> >::iterator it = new_clauses.begin(); it != new_clauses.end(); ++it) {
    std::set<int> elems = it->second;
    std::cout << "copy elems size " << elems.size() << "\n";
    for (std::set<int>::iterator elem = elems.begin(); elem != elems.end(); ++elem) {
      std::cout << *elem << " ";
    }
    std::cout << "\n";
  }
  std::cout << "\n\n";
}

// END DEBUGGERS //



// Print out the solution (multi-threading compatible - need to fix for current rep though)
void print_solution(std::vector<int> sat_vals) {
  std::string str = "Solution found!\n";
  int i = 1;
  for (std::vector<int>::iterator it = sat_vals.begin(); it != sat_vals.end(); ++it) {
    int exp = *it;
    str.append("x" + std::to_string(i) + " = ");
    if (exp == 1) str.append("T");
    else str.append("F");

    // deal with trailing comma at the very end
    if (i < sat_vals.size()) str.append(", ");
    i++;
  }
  gstate.print_sol_mutex.lock();
  std::cout << str << std::endl;
  gstate.print_sol_mutex.unlock();
}

// Clauses are empty when they are unsatisfied (all set to false).
bool contains_empty(std::map<int, std::set<int> > clauses) {
  for (std::map<int, std::set<int> >::iterator it=clauses.begin(); it != clauses.end(); ++it) {
    std::set<int> clause = it->second;
    if (clause.empty()) return true;
  }
  return false;
}

// Sets a given variable x to true by removing all clauses where x is a member,
// and removes -x from clauses where -x is a member.
// Also removes the variable entirely from consideration.
void assign_truth(int var_id, std::map<int, std::set<int> >* clauses,
    std::map<int, std::pair<std::set<int>, std::set<int> > >* vars) {

  // Remove the variable from clauses first.
  std::set<int> var_clauses = (*vars)[abs(var_id)].first;
  std::set<int> opp_clauses = (*vars)[abs(var_id)].second;
  if (var_id < 0) { // Assigning x to false is assigning -x to true.
    var_clauses = (*vars)[abs(var_id)].second;
    opp_clauses = (*vars)[abs(var_id)].first;
  }
  for (std::set<int>::iterator it = var_clauses.begin(); it != var_clauses.end(); ++it) {
    // Remove the clauses where x is a member and update variable membership.
    int clause_id = *it;
    std::set<int> clause = (*clauses)[clause_id];
    for (std::set<int>::iterator elem = clause.begin(); elem != clause.end(); ++elem) {
      if ((*elem) > 0) (*vars)[abs(*elem)].first.erase (clause_id);
      else (*vars)[abs(*elem)].second.erase (clause_id);
    }
    (*clauses).erase (clause_id);
  }
  for (std::set<int>::iterator it = opp_clauses.begin(); it != opp_clauses.end(); ++it) {
    // Remove -x from clauses where it is a member.
    int opp_var_id = -1 * var_id;
    (*clauses)[*it].erase (opp_var_id);
  }

  // Remove the variable itself.
  //(*vars).erase (var_id);
}

// Searches for unit clauses, and sets those variables to true.
void assign_unit_clauses(std::map<int, std::set<int> >* clauses,
    std::map<int, std::pair<std::set<int>, std::set<int> > >* vars) {
  for (std::map<int, std::set<int> >::iterator it = (*clauses).begin(); it != (*clauses).end(); ++it) {
    std::set<int> clause = it->second;
    if (clause.size() == 1) {
      // Found a unit clause, set the variable to true.
      for (std::set<int>::iterator elem = clause.begin(); elem != clause.end(); ++elem) {
        assign_truth(*elem, clauses, vars);
      }
    }
  }
}

// Searches for pure literals, and sets those variables to true.
void assign_pure_literals(std::map<int, std::set<int> >* clauses,
    std::map<int, std::pair<std::set<int>, std::set<int> > >* vars) {
  for (std::map<int, std::pair<std::set<int>, std::set<int> > >::iterator it = (*vars).begin(); it != (*vars).end(); ++it) {
    int var_id = it->first;
    std::set<int> pos_clauses = (it->second).first; // Clauses that contain x.
    std::set<int> neg_clauses = (it->second).second; // Clauses that contain -x.
    if (pos_clauses.size() == 0) {
      int opp_var_id = var_id * -1;
      assign_truth(opp_var_id, clauses, vars);
    }
    else if (neg_clauses.size() == 0) {
      assign_truth(var_id, clauses, vars);
    }
  }
}

// Checks if the clauses are already all satisfiable.
bool check_satisfied(std::map<int, std::set<int> > clauses) {
  if (clauses.empty()) return true;
  for (std::map<int, std::set<int> >::iterator it = clauses.begin(); it != clauses.end(); ++it) {
    bool has_pos = false;
    std::set<int> clause = it->second;
    for (std::set<int>::iterator elem = clause.begin(); elem != clause.end(); ++elem) {
      if (*elem > 0) {
        has_pos = true;
        break;
      }
    }
    if (!has_pos) return false;
  }
  for (std::map<int, std::set<int> >::iterator it = clauses.begin(); it != clauses.end(); ++it) {
    bool has_neg = false;
    std::set<int> clause = it->second;
    for (std::set<int>::iterator elem = clause.begin(); elem != clause.end(); ++elem) {
      if (*elem < 0) {
        has_neg = true;
        break;
      }
    }
    if (!has_neg) return false;
  }
  return true;
}

// Randomly picks the next available variable to assign.
int choose_literal(std::map<int, std::pair<std::set<int>, std::set<int> > > vars) {
  std::map<int, std::pair<std::set<int>, std::set<int> > >::iterator it = vars.begin();
  std::advance(it, rand() & vars.size());
  return it->first;
}


// Implements the DPLL algorithm.
void dpll(std::map<int, std::set<int> > clauses, std::map<int, std::pair<std::set<int>, std::set<int> > > vars) {
  if (check_satisfied(clauses)) {
    // found a viable solution!
    gstate.success_queue->enq(1);
    gstate.success = true;
    gstate.done = true;
    return;
  }
  if (contains_empty(clauses)) {
    // no solution found here
    return;
  }
  assign_unit_clauses(&clauses, &vars);
  assign_pure_literals(&clauses, &vars);

  // Try setting a random variable to true.
  int lit = choose_literal(vars);
  std::map<int, std::set<int> > pos_clauses = clauses;
  std::map<int, std::pair<std::set<int>, std::set<int> > > pos_vars = vars;
  assign_truth(lit, &pos_clauses, &pos_vars);

  // Try setting it to false.
  int not_lit = -1 * lit; // lol
  std::map<int, std::set<int> > neg_clauses = clauses;
  std::map<int, std::pair<std::set<int>, std::set<int> > > neg_vars = vars;
  assign_truth(not_lit, &neg_clauses, &neg_vars);

  work_t pos_work = new struct work;
  pos_work->clauses = pos_clauses;
  pos_work->vars = pos_vars;

  work_t neg_work = new struct work;
  neg_work->clauses = neg_clauses;
  neg_work->vars = neg_vars;

  gstate.wstack->push(pos_work);
  gstate.wstack->push(neg_work);

  /*std::thread neg_thread(dpll, neg_clauses, neg_vars);
  dpll(pos_clauses, pos_azvars);
  neg_thread.join();*/
  //return (dpll(pos_clauses, pos_vars) || dpll(neg_clauses, neg_vars));
}

// function that each thread calls and stays in
void* attempt_single_solution(void* args) {

  UNUSED(args);

  while (!gstate.done) {
    work_t work_struct = gstate.wstack->pop();

    dpll(work_struct->clauses, work_struct->vars);

    // delete work_struct? no need for struct memory anymore
  }

  return NULL;
}


// input form of SAT expression: 2,1 -2,3,-4
int main(int argc, char** argv) {
  std::map<int,std::set<int> > clauses;
  std::map<int,std::pair<std::set<int>, std::set<int> > > vars;
  parse(argc, argv, &clauses, &vars);

  //debug_vars(vars);
  //scratch_maps(clauses, vars);

  gstate.success_queue = new MQueue<int>();
  gstate.wstack = new MStack<work_t>();
  gstate.success = false;
  gstate.done = false;

  // start the initial call
  dpll(clauses, vars);

  // start all the threads
  pthread_t threads[NUM_THREADS];
  for (int t = 0; t < NUM_THREADS; t++) {
    pthread_create(&threads[t], NULL, attempt_single_solution, NULL);
    pthread_detach(threads[t]);
  }

  if (gstate.success) printf("There's a solution here somewhere...\n");
  else printf("No solution.\n");


  return 1;
}
