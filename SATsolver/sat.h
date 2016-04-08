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
    int ident;
    int pos;
    int neg;
    int assign;
} variable;

