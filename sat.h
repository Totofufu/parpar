#ifndef SAT_H
#define SAT_H

struct poss_soln {
  std::vector <std::vector<int> > expr;
  int rep_temp;
};
typedef struct poss_soln poss_soln;
typedef struct poss_soln* poss_soln_t;


#endif
