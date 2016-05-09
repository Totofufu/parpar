#include <set>

#ifndef SAT_H
#define SAT_H

struct work {
  std::map<int, std::set<int> > clauses;
  std::map<int, std::pair<std::set<int>, std::set<int> > > vars;
};
typedef struct work* work_t;

#endif
