#pragma once
#include <stddef.h>

typedef struct LList LList;
struct LList {
    LList *next, *prev;
};

static inline void llist_init(LList *list) {
    list->next = list;
    list->prev = list;
}

static inline void llist_insert(LList *list, LList *element) {
    LList *last_element = list->prev;

    last_element->next = element;
    list->prev = element;

    element->next = list;
    element->prev = last_element;
}

static inline void llist_remove(LList *element) {
    LList *prev = element->prev;
    LList *next = element->next;

    prev->next = next;
    next->prev = prev;
}

// tries to pop first element from list and return it, or null if its empty
static inline LList *llist_pop(LList *list) {
    LList *ret = list->next;
    if (ret == list) return NULL;
    llist_remove(ret);
    return ret;
}

#define list_init(list) llist_init(list)
#define list_insert(list, element) llist_insert(list, element)
#define list_remove(element) llist_remove(element)
#define list_empty(list) ((list)->next == list)
#define llist_iter(list, i) for (LList *i = (list)->next; i != (list); i = i->next)

#define LList(type) LList
