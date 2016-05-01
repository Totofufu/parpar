#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <vector>
#include <string.h>

#include "lib/parse.cpp"

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

bool is_satisfiable(std::vector <std::vector<int> > expr, std::vector<int> sat_vals) {
  bool satis = true;
  for (std::vector<std::vector<int> >::iterator outer = expr.begin(); outer != expr.end(); ++outer) {
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

// input form of SAT expression: 2,1 -2,3,-4
int main(int argc, char** argv) {
  int* num_vars_ptr = new int;
  std::vector <std::vector<int> > expr = parse(argc, argv, num_vars_ptr); // assume this is initialized
  int num_vars = *num_vars_ptr;

  // contains current clause true/false values
  std::vector<int> sat_vals;
  for (int i = 0; i < num_vars; i++) {
    sat_vals.push_back(-1);
  }

  for (int rep_num = 0; rep_num < pow(2, num_vars); rep_num++) {
    int rep_temp = rep_num;
    // fill in the SAT expr with a brute force attempt
    for (int i = 0; i < num_vars; i++) {
      int bit = rep_temp & 0x1;
      sat_vals.at(i) = bit;
      rep_temp >>= 1;
    }

    // check to see if the current SAT expression is satisfiable
    if (is_satisfiable(expr, sat_vals)) {
      printf("Solution found!\n");
      print_solution(sat_vals);
      delete num_vars_ptr;
      return 0;
    }
  }

  printf("No solution found.\n");
  delete num_vars_ptr;
  return 1;
}
