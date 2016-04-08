/* Greg Kownacki SAT Solver */

#include "list.h"
#include <stdlib.h>
#include <stdio.h>
#define NUM_HEURISTICS 1

typedef struct clause {
    int_list* posvars;
    int_list* negvars;
    int watch1;
    int watch2;
    int true_num;
    int false_num;
    int var_num;
} clause;

typedef struct variable {
    int_list *watched;
    int pos;
    int neg;
    int assign;
} variable;

int num_vars;
int num_clauses;
variable* assignments;
int_list* unassigned;
clause* clauses;
int_list* unsatisfied;
int_list* uce_queue;
int flag;
int verbose = 0;
int heuristic = 0;

/* Free all global variables */
void global_free() {
    int i;
    for (i=0; i<num_vars; i++) {
        list_free(assignments[i].watched);
    }
    free(assignments);
    list_free(unassigned);
    for (i=0; i<num_clauses; i++) {
        list_free(clauses[i].posvars);
        list_free(clauses[i].negvars);
    }
    free(clauses);
    list_free(unsatisfied);
    list_free(uce_queue);
}

/* Print all assignments to all variables */
void print_assignments() {
    int i;
    for (i=0; i<num_vars; i++) {
        if (assignments[i].assign == -1) {
            printf("%d: UNASSIGNED\n", i);
        } else if (assignments[i].assign == 0) {
            printf("%d: F\n", i);
        } else {
            printf("%d: T\n", i);
        }
    }
    printf("\n");
}

/* Print all clauses, and whether they are satisfied, with literals which are
 * still relevant (this may not be totally accurate, as we only coalesce watched
 * literals, unless we have to do otherwise) */
void print_clauses() {
    int i;
    printf("{\n");
    for (i=0; i<num_clauses; i++) {
        clause to_print = clauses[i];
        printf("(");
        node* j;
        for (j = to_print.posvars->head; j != NULL; j = j->next) {
            printf("%d, ", j->elem);
        }
        for (j = to_print.negvars->head; j != NULL; j = j->next) {
            printf("!%d, ", j->elem);
        }
        printf(")");
        if (to_print.true_num > 0) printf(" (satisfied)");
        printf("\n");
    }
    printf("}\n\n");
}

/* Inspect a clause, updating all variable info and checking if satisfied */
void update(int index) {
    clause* unsat = &clauses[index];

    // Check if watched variables have changed
    int to_add = 0;
    if (unsat->watch1 != -1) {
        int varindex = unsat->watch1/2;
        if (assignments[varindex].assign != -1) {
            unsat->watch1 = -1;
            to_add++;
            list_remove(assignments[varindex].watched,index);
        }
    }
    if (unsat->watch2 != -1) {
        int varindex = unsat->watch2/2;
        if (assignments[varindex].assign != -1) {
            unsat->watch2 = -1;
            to_add++;
            list_remove(assignments[varindex].watched,index);
        }
    }

    // Loop through all variables in clause, removing as necessary and updating
    // relevant invariants
    node* curr;
    node* next;
    variable var;
    int elem;
    for (curr = unsat->posvars->head; curr != NULL; curr = next) {
        next = curr->next;
        elem = curr->elem;
        var = assignments[elem];
        int val = var.assign;
        if (val != -1) {
            if (val == 1) {
                unsat->true_num++;
            } else if (val == 0) {
                unsat->false_num++;
            }
            var.pos--;
            list_remove(unsat->posvars,elem);
        } else if (to_add > 0) {
            if (unsat->watch1 == -1 && unsat->watch2/2 != elem) {
                unsat->watch1 = 2*elem+1;
                list_add(var.watched,index);
                to_add--;
            } else if (unsat->watch1/2 != elem && unsat->watch2 == -1) {
                unsat->watch2 = 2*elem+1;
                list_add(var.watched,index);
                to_add--;
            }
        }
    }

    for (curr = unsat->negvars->head; curr != NULL; curr = next) {
        next = curr->next;
        elem = curr->elem;
        var = assignments[elem];
        int val = var.assign;
        if (val != -1) {
            if (val == 0) {
                unsat->true_num++;
            } else if (val == 1) {
                unsat->false_num++;
            }
            var.neg--;
            list_remove(unsat->negvars,elem);
        } else if (to_add > 0) {
            if (unsat->watch1 == -1 && unsat->watch2/2 != elem) {
                unsat->watch1 = 2*elem;
                list_add(var.watched,index);
                to_add--;
            } else if (unsat->watch1/2 != elem && unsat->watch2 == 1) {
                unsat->watch2 = 2*elem;
                list_add(var.watched,index);
                to_add--;
            }
        }
    }

    // Global updates
    if (unsat->false_num == unsat->var_num) {
        flag = 1;
    }
    if (to_add > 0 && unsat->true_num == 0) {
        if (unsat->watch1 != -1) {
            list_add(uce_queue, unsat->watch1);
        } else {
            list_add(uce_queue, unsat->watch2);
        }
    }
    if (unsat->true_num > 0) {
        list_remove(unsatisfied, index);
    }
}

/* Force all clauses to update */
void coalesce() {
    node* curr;
    node* next;
    for (curr = unsatisfied->head; curr != NULL; curr = next) {
        next = curr->next;
        update(curr->elem);
    }
}

/* Assign value b to variable v */
void assign(int v, int b) {
    int_list* watched = assignments[v].watched;
    assignments[v].assign = b;
    list_remove(unassigned, v);
    node* curr;
    node* next;
    for (curr = watched->head; curr != NULL; curr = next) {
        next = curr->next;
        update(curr->elem);
    }
}

/* Check if any variables have PLE */
int ple() {
    node* curr;
    int result;
    for (curr=unassigned->head; curr != NULL; curr = curr->next) {
        int v = curr->elem;
        int pos = assignments[v].pos;
        int neg = assignments[v].neg;
        if (assignments[v].assign < 0) {
            if (pos == 0) {
                return 2*v;
            }
            if (neg == 0) {
                return 2*v+1;
            }
        }
    }
    return -1;
}

/* Pick a variable and value to split on, where result/2 is var and
 * result%2 is value */
int split() {
    int unsat;
    int max = 0;
    int result;
    node *curr;
    switch (heuristic) {
        case 0:
            unsat = unsatisfied->head->elem;
            if (clauses[unsat].watch1 == -1) {
                return clauses[unsat].watch2;
            }
            return clauses[unsat].watch1;
        case 1:
            for (curr = unassigned->head; curr != NULL; curr = curr->next) {
                if (assignments[curr->elem].pos > max) {
                    max = assignments[curr->elem].pos;
                    result = 2*curr->elem+1;
                }
                if (assignments[curr->elem].neg > max) {
                    max = assignments[curr->elem].neg;
                    result = 2*curr->elem;
                }
            }
            return result;
        default:
            printf("Invalid heuristic???\n");
            exit(-1);
    }
}

/* Check if satisfiable */
int sat() {
    while (unsatisfied->head != NULL) {
        if (verbose) {
            printf("Current:\n");
            print_clauses();
        }
        if (flag) {
            // :(
            return 0;
        }

        // Check for UCE
        node* uce = remove_front(uce_queue);
        if (uce != NULL) {
            int elem = uce->elem;
            int v = elem/2;
            int b = elem%2;
            assign(v,b);
            if (verbose) {
                printf("UCE on %d\n\n", v);
            }
            free(uce);
            continue;
        }

        // Check for PLE
        int ple_check = ple();
        if (ple_check != -1) {
            int v = ple_check/2;
            int b = ple_check%2;
            assign(v,b);
            if (verbose) {
                printf("PLE on %d\n\n", v);
            }
            continue;
        }

        if (verbose) {
            printf("Coalescing.\n\n");
        }

        coalesce();

        // Coalescing can only introduce new PLE, watched deals with UCE
        ple_check = ple();
        if (ple_check != -1) {
            int v = ple_check/2;
            int b = ple_check%2;
            assign(v,b);
            if (verbose) {
                printf("PLE on %d\n\n", v);
            }
            continue;
        }

        // We must split, so save current state
        int splitasgn = split();
        int v = splitasgn / 2;
        int b = splitasgn % 2;

        if (verbose) {
            printf("Splitting on %d\n\n",v);
        }

        variable* saved_assgn = assignments;
        int_list* saved_unassgn = unassigned;
        clause* saved_clauses = clauses;
        int_list* saved_unsat = unsatisfied;
        int_list* saved_uce = uce_queue;

        assignments = calloc(sizeof(variable), num_vars);
        unassigned = list_copy(saved_unassgn);
        clauses = calloc(sizeof(clause), num_clauses);
        unsatisfied = list_copy(saved_unsat);
        uce_queue = list_copy(saved_uce);

        int i;
        for (i=0; i<num_vars; i++) {
            assignments[i].watched = list_copy(saved_assgn[i].watched);
            assignments[i].pos = saved_assgn[i].pos;
            assignments[i].neg = saved_assgn[i].neg;
            assignments[i].assign = saved_assgn[i].assign;
        }
        for (i=0; i<num_clauses; i++) {
            clauses[i].posvars = list_copy(saved_clauses[i].posvars);
            clauses[i].negvars = list_copy(saved_clauses[i].negvars);
            clauses[i].watch1 = saved_clauses[i].watch1;
            clauses[i].watch2 = saved_clauses[i].watch2;
            clauses[i].true_num = saved_clauses[i].true_num;
            clauses[i].false_num = saved_clauses[i].false_num;
            clauses[i].var_num = saved_clauses[i].var_num;
        }

        assign(v,b);
        int result = sat();
        if (result) {
            int i;
            for (i=0; i<num_vars; i++) {
                list_free(saved_assgn[i].watched);
            }
            free(saved_assgn);
            list_free(saved_unassgn);
            for (i=0; i<num_clauses; i++) {
                list_free(saved_clauses[i].posvars);
                list_free(saved_clauses[i].negvars);
            }
            free(saved_clauses);
            list_free(saved_unsat);
            list_free(saved_uce);
            return 1;
        }
        global_free();

        if (verbose) {
            printf("Trying other split on %d\n\n", v);
        }

        assignments = saved_assgn;
        unassigned = saved_unassgn;
        clauses = saved_clauses;
        unsatisfied = saved_unsat;
        uce_queue = saved_uce;
        flag = 0;
        assign(v,1-b);

    }
    if (verbose) {
        printf("Final:\n");
        print_clauses();
    }
    return 1;
}

int main(int argc, char** argv) {
    // I'm not commenting this part. Parsing input is lame.
    FILE *fp;

    char* input = NULL;
    int h;

    while (argc > 1) {
        switch (argv[1][0])
        {
            case '-':
                switch (argv[1][1]) {
                    case 'v':
                        verbose = 1;
                        break;
                    case 'h':
                        printf("Use -v for verbose mode.\n");
                        printf("Any non-flag argument is assumed to be input file.\n");
                        printf("See README for input format details.\n");
                        break;
                    case 's':
                        h = atoi(&(argv[1][2]));
                        if (h < 0 || h >= NUM_HEURISTICS) {
                            printf("Invalid heuristic %s\n", argv[1]);
                        } else {
                            heuristic = h;
                        }
                        break;
                    default:
                        printf("Invalid Argument %s\n", argv[1]);
                        break;
                }
                break;
            default:
                input = argv[1];
                break;
        }
        argv++;
        argc--;
    }

    if (input == NULL) {
        printf("No argument file given.\n");
        return -1;
    }

    fp = fopen(input, "r");
    char buf[1024];
    fgets(buf, sizeof(buf), fp);
    sscanf(buf, "%d,%d\n", &num_vars, &num_clauses);
    assignments = calloc(sizeof(variable), num_vars);
    unassigned = list_new();
    clauses = calloc(sizeof(clause), num_clauses);
    unsatisfied = list_new();
    uce_queue = list_new();
    flag = 0;
    int i;
    for (i=0; i<num_vars; i++) {
        list_add(unassigned, i);
        assignments[i].watched = list_new();
        assignments[i].assign = -1;
    }

    int cindex = 0;
    clause* currclause;
    while (fgets(buf, sizeof(buf), fp) != NULL) {
        currclause = &clauses[cindex];
        list_add(unsatisfied, cindex);
        int total = 0;
        currclause->posvars = list_new();
        currclause->negvars = list_new();
        currclause->watch1 = -1;
        currclause->watch2 = -1;
        int to_watch = 2;
        int var = 0;
        int negflag = 0;
        int index = 0;
        char curr = buf[index];
        do {
            curr = buf[index];
            switch(curr){
                case '!' :
                    negflag = 1;
                    break;
                case ',' :
                case '\n' :
                    if (to_watch == 2) {
                        currclause->watch1 = 2*var+(1-negflag);
                        list_add(assignments[var].watched, cindex);
                        to_watch--;
                    } else if (to_watch == 1) {
                        currclause->watch2 = 2*var+(1-negflag);
                        list_add(assignments[var].watched, cindex);
                        to_watch--;
                    }
                    total++;
                    if (negflag) {
                        list_add(currclause->negvars,var);
                        assignments[var].neg++;
                    } else {
                        list_add(currclause->posvars,var);
                        assignments[var].pos++;
                    }
                    negflag = 0;
                    var = 0;
                    break;
                default :
                    var = 10*var+(curr-48);
                    break;
            }
            index++;
        } while (curr != '\n');

        currclause->var_num = total;
        if (to_watch == 1) {
            list_add(uce_queue, currclause->watch1);
        }

        negflag = 0;
        var = 0;
        cindex++;
    }
    fclose(fp);
    printf("Input:\n");
    print_clauses();

    int result = sat();
    if (result == 0) {
        printf("UNSATISFIABLE\n");
    } else {
        printf("SATISFIABLE\n");
        print_assignments();
    }
    global_free();
    return 0;
}

