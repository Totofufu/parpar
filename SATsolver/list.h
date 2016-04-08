typedef struct node {
    int elem;
    struct node* prev;
    struct node* next;
} node;

typedef struct int_list {
    node* head;
} int_list;

int_list* list_new();
void list_add(int_list* list, int x);
void list_remove(int_list* list, int x);
node* remove_front(int_list* list);
void list_free(int_list* list);
void list_print(int_list* list);
int_list* list_copy(int_list* list);
