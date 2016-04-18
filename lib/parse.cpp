#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <vector>

// Input: 1,2,3 4,-5 -6,7
// Output: [[1,2,3],[4,-5],[-6,7]]
std::vector< std::vector<int> > parse(int argc, char** argv, int* max_var) {
  std::vector< std::vector<int> > full_expr;
  std::vector< std::vector<int> >::iterator full_it;
  full_it = full_expr.begin();
  max_var[0] = 0;

  // Iterate through cmd line arguments.
  for (int i = 1; i < argc; i++) {
    std::vector<int> sub_expr;
    std::vector<int>::iterator sub_it;
    sub_it = sub_expr.begin();

    char *sub = argv[i];
    std::string full_sub(sub);
    std::string delimiter = ",";
    size_t pos = 0;
    std::string token;

    // Delimit the subexpression by commas and insert into full expression.
    while ((pos = full_sub.find(delimiter)) != std::string::npos) {
      token = full_sub.substr(0, pos);
      int new_var = atoi(token.c_str());
      if (abs(new_var) > max_var[0]) {
        max_var[0] = abs(new_var);
      }

      // Insert each variable into the subexpression.
      sub_expr.insert(sub_it, new_var);
      std::cout << new_var << "\n";
      sub_it = sub_expr.begin();

      full_sub.erase(0, pos + delimiter.length());
    }
    // Don't forget the last argument
    token = full_sub.substr(0, pos);
    int last_arg = atoi(token.c_str());
    sub_expr.insert(sub_it, last_arg);
    std::cout << token << "\n";
    if (abs(last_arg) > max_var[0]) {
      max_var[0] = abs(last_arg);
    }

    // Reset full expression iterator.
    full_expr.insert(full_it, sub_expr);
    full_it = full_expr.begin();

  }
  return full_expr;
}

// input form of SAT expression: 0,2 | -2,3,-4
int main(int argc, char** argv) {
  int foo[1] = {};
  std::vector< std::vector <int> > exp = parse(argc, argv, foo);
  std::cout << foo[0] << "\n";
  return 0;
}
