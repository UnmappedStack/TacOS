#pragma once

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

#define list_init(list) llist_init(list)
#define list_insert(list, element) llist_insert(list, element)
#define list_remove(element) llist_remove(element)
#define list_empty(list) ((list)->next == list)
