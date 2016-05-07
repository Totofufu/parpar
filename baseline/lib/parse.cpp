#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

#ifndef PARSELIB_H
#define PARSELIB_H

// Input: 1,2,3 4,-5 -6,7
// Output: [[1,2,3],[4,-5],[-6,7]]
std::vector< std::vector<int> > parse(int argc, char** argv, int* max_var) {
  std::vector< std::vector<int> > full_expr; // Formatted output
  std::vector< std::vector<int> >::iterator full_it;
  full_it = full_expr.begin();
  *max_var = 0;
  std::string line;
  std::ifstream input_expr (argv[1]);

  if (input_expr.is_open()) {
    while (std::getline(input_expr, line)) {
      std::vector<int> sub_expr;
      std::vector<int>::iterator sub_it;
      sub_it = sub_expr.begin();

      std::string delimiter = ",";
      size_t pos = 0;
      std::string token;

      while ((pos = line.find(delimiter)) != std::string::npos) {
        token = line.substr(0, pos);
        int new_var = atoi(token.c_str());
        if (abs(new_var) > max_var[0]) {
          *max_var = abs(new_var);
        }

        // Insert each variable into the subexpression.
        sub_expr.insert(sub_it, new_var);
        //std::cout << new_var << "\n";
        sub_it = sub_expr.begin();

        line.erase(0, pos + delimiter.length());
      }

      // Don't forget the last argument
      token = line.substr(0, pos);
      int last_arg = atoi(token.c_str());
      sub_expr.insert(sub_it, last_arg);
      //std::cout << token << "\n";
      if (abs(last_arg) > max_var[0]) {
        *max_var = abs(last_arg);
      }

      // Reset full expression iterator.
      full_expr.insert(full_it, sub_expr);
      full_it = full_expr.begin();
    }
    input_expr.close();
  }
  return full_expr;
}

#endif
