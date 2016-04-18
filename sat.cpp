#include <stdlib.h>
#include <stdio.h>
#include <math.h>

void print_solution(std::vector <std::vector<int>> sat_vals) {
  for (std::vector<std::vector<int>>::iterator outer = sat_vals.begin(); outer != sat_vals.end(); ++outer) {
    for (std::vector<int>::iterator inner = *outer.begin(); inner != *outer.end(); ++inner) {
      int exp = *inner;
      if (exp == 1) printf("T,");
      else printf("F,");
    }
    printf("\n");
  }
}

bool is_satisfiable(std::vector <std::vector<int>> sat_vals) {
  bool satis = false;
  for (std::vector<std::vector<int>>::iterator outer = sat_vals.begin(); outer != sat_vals.end(); ++outer) {
    bool clause = false;
    for (std::vector<int>::iterator inner = *outer; inner != *outer.end(); ++inner) {
      int exp = *inner;
      clause |= exp;
    }
    satis &= clause;
    if (satis) return true;
  }
  return false;
}

// input form of SAT expression: 0,2 -2,3,-4
int main(int argc, char** argv) {
  int num_vars = 0;
  std::vector <std::vector<int>> expr = file_read(&max_var); // assume this is initialized

  // contains current clause true/false values
  std::vector<int> sat_vals;
  for (int i = 0; i < num_vars; i++) {
    sat_vals.push_back(-1);
  }

  for (int rep_num = 0; rep_num < pow(2, num_vars); rep_num++) {
    int rep_temp = rep_num;
    for (int i = 0; i < num_vars; i++) {
      int bit = rep_temp & 0x1;
      sat_vals[i] = bit;
      rep_temp >>= 1;
    }

    if (is_satisfiable(sat_vals)) {
      printf("Solution found!\n");
      print_solution(sat_vals);
      return 0;
    }
  }

  printf("No solution found.\n");
  return 1;
}
