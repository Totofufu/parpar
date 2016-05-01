#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <vector>
#include <string>
#include <iostream>
#include <sstream>

#include "lib/parse.cpp"

std::string get_rand_var(int num_vars) {
  int var = rand() % num_vars + 1;
  int flip = rand() % 2;
  if (flip) {
    var = var * -1;
  }
  std::string s;
  std::stringstream out;
  out << var;
  s = out.str();
  return s;
}

std::string gen_sat(int num_vars, int num_clauses, int max_clause_len) {
  std::string expr;
  for (int i = 0; i < num_clauses; i++) {
    int clause_len = rand() % max_clause_len + 1;
    for (int j = 1; j < clause_len; j++) {
      expr += get_rand_var(num_vars);
      expr += ",";
    }
    expr += get_rand_var(num_vars);
    expr += "  ";
  }
  return expr;
}

// input form of SAT expression: 2,1 -2,3,-4
int main(int argc, char** argv) {
  std::string result = gen_sat(100, 10, 5);
  std::cout << result << "\n";
  return 1;
}
