#pragma once
#include <stdint.h>

typedef struct Node Node;
struct Node {
    Node *parent_and_colour;
    uint64_t val;
    Node *children[2];

    void *data;
};

typedef struct {
    Node *root;
} Tree;

int rbtree_remove(Tree *tree, uint64_t key);
Node *rbtree_insert(Tree *tree, uint64_t key);
Node *rbtree_search(Tree *tree, uint64_t key);
