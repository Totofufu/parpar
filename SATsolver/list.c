#include "list.h"
#include <stdlib.h>
#include <stdio.h>

int_list* list_new() {
    int_list* list = malloc(sizeof(int_list));
    list->head = NULL;
    return list;
}

void list_add(int_list* list, int x) {
    node* new = malloc(sizeof(node));
    new->elem = x;
    new->next = list->head;
    new->prev = NULL;
    if (list->head != NULL) {
        list->head->prev = new;
    }
    list->head = new;
}

void list_remove(int_list* list, int x) {
    node* curr;
    node* next;
    for (curr=list->head; curr != NULL; curr = next) {
        next = curr->next;
        if (curr->elem == x) {
            if (curr == list->head) {
                list->head = curr->next;
            } else {
                curr->prev->next = curr->next;
                if (curr->next != NULL) {
                    curr->next->prev = curr->prev;
                }
            }
            free(curr);
        }
    }
}

node* remove_front(int_list* list) {
    node* ret = list->head;
    if (ret == NULL) return ret;
    list->head = ret->next;
    if (ret->next == NULL) return ret;
    list->head->prev = NULL;
    return ret;
}

void list_free(int_list* list) {
    node* curr;
    node* next;
    for (curr=list->head; curr != NULL; curr = next) {
        next = curr->next;
        free(curr);
    }
    free(list);
}

void list_print(int_list* list) {
    node* curr;
    for (curr=list->head; curr != NULL; curr = curr->next) {
        printf("%d, ", curr->elem);
    }
    printf("\n");
}

int_list* list_copy(int_list* list) {
    int_list* new = malloc(sizeof(int_list));
    new->head = NULL;
    node* curr;
    for (curr = list->head; curr != NULL; curr = curr->next) {
        list_add(new, curr->elem);
    }
    return new;
}
