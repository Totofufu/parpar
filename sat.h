#ifndef SAT_H
#define SAT_H

struct poss_ans {
  std::vector<int> sat_vals;
  int rep_temp;
  bool success;
};
typedef struct poss_ans poss_ans_t;

#endif
