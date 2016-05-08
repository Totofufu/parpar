#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include <set>

#ifndef PARSELIB_H
#define PARSELIB_H

// Input: 1,2,3 4,-5 -6,7
// Output: [[1,2,3],[4,-5],[-6,7]]
void parse(int argc, char** argv, std::map<int,std::set<int> >* clauses, std::map<int,std::set<int> >* vars) {
  std::string line;
  std::ifstream input_expr (argv[1]);

  if (input_expr.is_open()) {
    int counter = 0;
    while (std::getline(input_expr, line)) {
      std::set<int> new_clause;

      std::string delimiter = ",";
      size_t pos = 0;
      std::string token;

      while ((pos = line.find(delimiter)) != std::string::npos) {
        token = line.substr(0, pos);
        int new_var = atoi(token.c_str());
        new_clause.insert (new_var); // Insert line elements into clause.
        (*vars)[abs(new_var)].insert (counter);
        line.erase(0, pos + delimiter.length());
      }

      // Don't forget the last argument
      token = line.substr(0, pos);
      int last_arg = atoi(token.c_str());
      new_clause.insert (last_arg);

      (*clauses)[counter] = new_clause;
      counter++;

    }
    input_expr.close();
  }
}

#endif
