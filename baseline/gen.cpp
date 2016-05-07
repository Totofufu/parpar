#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cassert>
#include <vector>
#include <algorithm>
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

// gets myvector[j_index] and returns it as a string
std::string get_idx_str(std::vector<int> myvector, int j_index) {
  std::stringstream out;
  out << myvector.at(j_index);
  return out.str();
}

std::string gen_sat(int num_vars, int num_clauses, int max_clause_len) {
  assert(num_vars <= num_clauses);
  std::string expr;

  // firstly, shuffle a vector containing all values from 1 to 2^num_vars
  std::vector<int> myvector;
  for (int i = 1; i <= num_vars; i++) {
    int var = i;
    if (rand() % 2) var *= -1; // randomly choose pos or negation of var
    myvector.push_back(var);
  }
  std::random_shuffle(myvector.begin(), myvector.end());

  // fill up expr with the first 2^num_vars, shuffled
  int idx = 0;
  int curr_num_clauses = 0;
  while (idx < num_vars) {
    int clause_len = rand() % max_clause_len + 1;
    if (idx+clause_len <= num_vars) {
      for (int j = idx; j < idx+clause_len-1; j++) {
        expr += get_idx_str(myvector, j);
        expr += ",";
      }
      expr += get_idx_str(myvector, idx+clause_len-1);
    }
    else {
      for (int j = idx; j < num_vars-1; j++) {
        expr += get_idx_str(myvector, j);
        expr += ",";
      }
      expr += get_idx_str(myvector, num_vars-1);
    }
    curr_num_clauses++;
    expr += "\n";
    idx += clause_len;
  }

  // fill up the remaining part of expr (after 2^num_vars)
  for (int i = curr_num_clauses; i < num_clauses; i++) {
    int clause_len = rand() % max_clause_len + 1;
    for (int j = 1; j < clause_len; j++) {
      expr += get_rand_var(num_vars);
      expr += ",";
    }
    expr += get_rand_var(num_vars);
    expr += "\n";
  }
  return expr;
}

// input form of SAT expression: 2,1  -2,3,-4
// NUM_CLAUSES MUST BE AT LEAST NUM_VARS
int main(int argc, char** argv) {
  // num_vars, num_clauses, max_clause_length
  std::string result = gen_sat(100, 1000, 5);
  std::cout << result << "\n";
  return 1;
}
