// TODO: this is not thread safe, make it so

#include <assert.h>
#include <kernel.h>
#include <slab.h>
#include <string.h>
#include <kprintf.h>
#include <stdint.h>
#include <rbtree.h>

typedef enum {
    LEFT  = 0,
    RIGHT = 1,
    THIS  = 2,
} Direction;

typedef enum {
    BLACK, RED,
} Colour;

// check whether a key would belong to the right or left of a node
Direction check_node_direction(Node *node, uint64_t key) {
    assert(node);
    if (key > node->val)
        return RIGHT;
    else if (key < node->val)
        return LEFT;
    else return THIS;
}

#define FLIP_DIR(dir) (assert(dir != THIS), (dir == RIGHT) ? LEFT : RIGHT)

// these assume the pointer of the coloured pointer is of type Node*
#define IS_BYTE_ALIGNED(x) (!((size_t)x & 0b1))
#define MK_COLOURED_POINTER(parent, tag) (assert(IS_BYTE_ALIGNED(parent)), ((size_t)parent | (uint8_t)tag))
#define POINTER_FROM_COLOURED_POINTER(x) ((Node*)((size_t)x & ~1))
#define TAG_FROM_COLOURED_POINTER(x) ((size_t)x & 1)
#define COLOURED_POINTER_SET_TAG(cptr, tag) (cptr = (Node*)(((size_t)cptr & ~1) | (uint8_t)tag))
#define COLOURED_POINTER_SET_PTR(cptr, ptr) (assert(IS_BYTE_ALIGNED(ptr)), cptr = (Node*)(((size_t)cptr & 1) | (size_t)ptr))

/* rotates from a specific node and returns the new root node which takes the place of
 * the previous *node in the tree.
 * I tried to do it myself but I was looking at the wikipedia samples as I was
 * implementing this so it is probably quite similar, therefore here is some credit for ya:
 *
 * https://en.wikipedia.org/wiki/Red-black_tree */
Node *rbtree_rotate(Tree *tree, Node *node, Direction dir) {
    assert(dir != THIS && "invalid direction for rbtree_rotate");

    Node *parent    = POINTER_FROM_COLOURED_POINTER(node->parent_and_colour);
    Node *new_root  = node->children[FLIP_DIR(dir)];
    Node *new_child = new_root->children[dir];

    node->children[FLIP_DIR(dir)] = new_child;
    
    if (new_child) COLOURED_POINTER_SET_PTR(new_child->parent_and_colour, node);
    
    new_root->children[dir] = node;
    COLOURED_POINTER_SET_PTR(new_root->parent_and_colour, parent);
    COLOURED_POINTER_SET_PTR(node->parent_and_colour, new_root);

    if (parent) {
        Direction new_dir = (node == parent->children[RIGHT]) ? RIGHT : LEFT;
        parent->children[new_dir] = new_root;
    } else tree->root = new_root;

    return new_root;
}

// if it doesnt exist then it should
// return the parent of what the node would be.
Node *rbtree_search(Tree *tree, uint64_t key) {
    if (!tree) {
        klogf(LOG_ERROR, "got nullptr for tree in rbtree_search\n");
        return NULL;
    }

    Node *node = tree->root;

    if (!node) {
        klogf(LOG_ERROR, "search empty tree\n");
        return 0;
    }

    for (;;) {
        Direction direction = check_node_direction(node, key);
        switch (direction) {
        case RIGHT:
        case LEFT:
            if (node->children[direction] == NULL) 
                return node; // if it doesnt exist return parent
            node = node->children[direction];
            continue;
        case THIS:
            return node;
        }
    }
    kpanic("unreachable\n");
    return NULL;
}

// like rbtree_search, but if it doesn't exist then it will return NULL instead of the parent
// of what it wouldve been. also instead of a node to start searching from it takes a Tree*
Node *rbtree_search_err(Tree *tree, uint64_t key) {
    Node *ret = rbtree_search(tree, key);

    if (!ret) return NULL; // couldn't find it
    Direction dir = check_node_direction(ret, key);
    if (dir != THIS) return NULL; // couldn't find it

    return ret;
}

int rbtree_insert_first_node(Tree *tree, uint64_t key, Node *nodebuf) {
    nodebuf->val = key;
    nodebuf->parent_and_colour = (Node*)MK_COLOURED_POINTER(NULL, BLACK);
    memset(nodebuf->children, 0, sizeof(nodebuf->children));

    tree->root = nodebuf;
    return 0;
}

// return 0 on success, -1 on error. will not rebalance the tree.
// both *_buf args can be NULL if you don't care about them, otherwise they
// will point to the parent of the inserted node and the inserted node.
int rbtree_insert_unbalanced(Tree *tree, uint64_t key, Node **parent_buf, Node *nodebuf) {
//    printf("Try insert key %u...\n", key);
    if (!tree->root) {
        if (parent_buf) *parent_buf = NULL;
        return rbtree_insert_first_node(tree, key, nodebuf);
    }

    Node *parent = rbtree_search(tree, key);

    Direction direction = check_node_direction(parent, key);
    if (direction == THIS) {
        klogf(LOG_ERROR, "A node with key %u already exists in the red/black tree\n", key);
        return -1;
    }

    nodebuf->val    = key;
    nodebuf->parent_and_colour = (Node*)MK_COLOURED_POINTER(parent, RED);
    memset(nodebuf->children, 0, sizeof(nodebuf->children));

    if (parent_buf) *parent_buf = parent;

    parent->children[direction] = nodebuf;
    return 0;
}

// this is a chonky macro name but idc its descriptive. 
#define DIR_OF_CHILD_IN_PARENT(node) (((POINTER_FROM_COLOURED_POINTER((node)->parent_and_colour))->children[RIGHT] == (node)) ? RIGHT : LEFT)
// assumes node was just inserted and is still RED as it has not been modified.
// this is also pretty damn based on the wikipedia one so credit is as given in
// the link above.
void rbtree_rebalance(Tree *tree, Node *parent, Node *node) {
    do {
        // no need to rebalance, as it is black->red, not red->red
        if (TAG_FROM_COLOURED_POINTER(parent->parent_and_colour) == BLACK) return;

        Node *grandparent = POINTER_FROM_COLOURED_POINTER(parent->parent_and_colour);
        if (!grandparent) {
            /* if the parent is the root node, we can really easily fix the
             * red->red issue by just making the parent black, then we can be
             * sure it won't cause issues further up the tree. */
            COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
            return;
        }

        Direction dir = DIR_OF_CHILD_IN_PARENT(parent);
        Node *uncle = grandparent->children[FLIP_DIR(dir)];
        if (!uncle || TAG_FROM_COLOURED_POINTER(uncle->parent_and_colour) == BLACK) {
            if (node == parent->children[FLIP_DIR(dir)]) {
                // parent is red but the uncle, its sibling, is black. we want
                // to rotate so that the parent becomes the grandparent
                rbtree_rotate(tree, parent, dir);
                node = parent;
                parent = grandparent->children[dir];
            }

            // the node is now an outer node (left->left or right->right), rotate
            // so that the parent replaces the grandparent, parent becomes the parent
            // of both the node and the grandparent.
            rbtree_rotate(tree, grandparent, FLIP_DIR(dir));
            COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
            COLOURED_POINTER_SET_TAG(grandparent->parent_and_colour, RED);
            return;
        }

        // parent and uncle are both red, they can become black while the
        // grandparent (their parent) becomes red, to ensure that the number of
        // black nodes from any node to its leaf nodes are the same.
        assert(POINTER_FROM_COLOURED_POINTER(uncle->parent_and_colour) == grandparent &&
                POINTER_FROM_COLOURED_POINTER(parent->parent_and_colour) == grandparent);
        COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
        COLOURED_POINTER_SET_TAG(uncle->parent_and_colour, BLACK);
        COLOURED_POINTER_SET_TAG(grandparent->parent_and_colour, RED);
        node = grandparent;
    } while((parent = POINTER_FROM_COLOURED_POINTER(node->parent_and_colour)));
}

// ret inserted node on success (literally nodebuf), NULL on error. inserts then ensures the tree is balanced.
Node *rbtree_insert(Tree *tree, Node *nodebuf, uint64_t key) {
    Node *parent;
    if (rbtree_insert_unbalanced(tree, key, &parent, nodebuf) < 0) {
        klogf(LOG_ERROR, "Failed insertion of key %u\n", key);
        return NULL;
    }
   
    /* we don't wanna rebalance if it was the root node (aka the first node)
     * we just inserted */
    if (parent)
        rbtree_rebalance(tree, parent, nodebuf);

    return nodebuf;
}

// balanced removal of a specific node from a tree where the node is black, non-root, and a leaf
// this one is also pretty wikipedia-esque so credit is as above.
int rbtree_remove_node_complex(Tree *tree, Node *node) {
    assert(node && tree);
    Node *sibling, *close_nephew, *far_nephew;
    Node *parent = POINTER_FROM_COLOURED_POINTER(node->parent_and_colour);
    assert(parent);
    Direction dir = DIR_OF_CHILD_IN_PARENT(node);

    slab_free(kernel_info.rbtree_cache, node);
    parent->children[dir] = NULL;
    goto start_balance;
    do {
        dir = DIR_OF_CHILD_IN_PARENT(node);
start_balance:
        sibling = parent->children[FLIP_DIR(dir)];
        if (!sibling) return 0;
        far_nephew = sibling->children[FLIP_DIR(dir)];
        close_nephew = sibling->children[dir];
        if (TAG_FROM_COLOURED_POINTER(sibling->parent_and_colour) == RED) {
            rbtree_rotate(tree, parent, dir);
            COLOURED_POINTER_SET_TAG(parent->parent_and_colour, RED);
            COLOURED_POINTER_SET_TAG(sibling->parent_and_colour, BLACK);
            sibling = close_nephew;

            far_nephew = sibling->children[FLIP_DIR(dir)];
            if (far_nephew &&
                    TAG_FROM_COLOURED_POINTER(far_nephew->parent_and_colour) == RED) {
                goto case6;
            }

            close_nephew = sibling->children[dir];
            if (close_nephew &&
                    TAG_FROM_COLOURED_POINTER(close_nephew->parent_and_colour) == RED) {
                goto case5;
            }

            COLOURED_POINTER_SET_TAG(sibling->parent_and_colour, RED);
            COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
            return 0;
        }

        if (far_nephew &&
                TAG_FROM_COLOURED_POINTER(far_nephew->parent_and_colour) == RED) {
            goto case6;
        }

        if (close_nephew &&
                TAG_FROM_COLOURED_POINTER(close_nephew->parent_and_colour) == RED) {
            goto case5;
        }

        if (TAG_FROM_COLOURED_POINTER(parent->parent_and_colour) == RED) {
            COLOURED_POINTER_SET_TAG(sibling->parent_and_colour, RED);
            COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
            return 0;
        }

        COLOURED_POINTER_SET_TAG(sibling->parent_and_colour, RED);
        node = parent;
    } while ((parent = POINTER_FROM_COLOURED_POINTER(node->parent_and_colour)));
    return 0;

case5:
    rbtree_rotate(tree, sibling, FLIP_DIR(dir));
    COLOURED_POINTER_SET_TAG(sibling->parent_and_colour, RED);
    COLOURED_POINTER_SET_TAG(close_nephew->parent_and_colour, BLACK);
    far_nephew = sibling;
    sibling = close_nephew;

case6:
    rbtree_rotate(tree, parent, dir);
    COLOURED_POINTER_SET_TAG(sibling->parent_and_colour,
            TAG_FROM_COLOURED_POINTER(parent->parent_and_colour)
        );
    COLOURED_POINTER_SET_TAG(parent->parent_and_colour, BLACK);
    COLOURED_POINTER_SET_TAG(far_nephew->parent_and_colour, BLACK);

    return 0;
}

#define SWAP(Type, x, y) do { \
        Type temp = x; \
        x = y; \
        y = temp; \
    } while(0)

// returns -1 on error and 0 on success. if you want to delete by key youll
// have to search for it first.
int rbtree_remove(Tree *tree, Node *node) {
    assert(tree && node);

    if (node->children[LEFT] && node->children[RIGHT]) {
        /* the successor will be found by taking the right child of the node
         * then going down left until it hits the leaf */
        Node *successor = node->children[RIGHT];
        while (successor->children[LEFT])
            successor = successor->children[LEFT];

        SWAP(uint64_t, node->val, successor->val);

        assert(successor != node);
        rbtree_remove(tree, successor);
        return 0;
    } else if (node->children[LEFT] || node->children[RIGHT]) {
        // only one child
        Direction child_dir = node->children[LEFT] ? LEFT : RIGHT;
        Node *child = node->children[child_dir];

        node->val = child->val;
        COLOURED_POINTER_SET_TAG(node->parent_and_colour, BLACK);
       
        slab_free(kernel_info.rbtree_cache, node->children[child_dir]);
        node->children[child_dir] = NULL;
        return 0;
    } else if (!POINTER_FROM_COLOURED_POINTER(node->parent_and_colour)) {
        // this node is the root value and has no children, just get rid of it
        slab_free(kernel_info.rbtree_cache, tree->root);
        tree->root = NULL;
        return 0;
    } else if (TAG_FROM_COLOURED_POINTER(node->parent_and_colour) == RED) {
        // no children, not root, and is red, can just remove it
        Node *parent = POINTER_FROM_COLOURED_POINTER(node->parent_and_colour);
        Direction dir = DIR_OF_CHILD_IN_PARENT(node);

        slab_free(kernel_info.rbtree_cache, parent->children[dir]);
        parent->children[dir] = NULL;
        return 0;
    } else {
        // only case left is a black leaf non-root node, which is the complex case
        return rbtree_remove_node_complex(tree, node);
    }

    kpanic("unexpected removal type\n");
    return -1;
}

#define NUM_NUMS 10
void rbtree_init(void) {
    kernel_info.rbtree_cache = cache_create(sizeof(Node));
    if (!kernel_info.rbtree_cache) kpanic("failed to create rbtree cache");

    // basic testing (these might be removed later idk it doesnt really matter)
    // insert some numbers then delete half of them
    Tree rbtree = {0};
    for (size_t i = 0; i < NUM_NUMS; i++) {
        Node *node = slab_alloc(kernel_info.rbtree_cache);
        assert(rbtree_insert(&rbtree, node, i));
        if (i < 5) continue;
        assert(!rbtree_remove(&rbtree, node));
    }

    // try find them
    for (size_t i = 0; i < NUM_NUMS; i++) {
        Node *n = rbtree_search_err(&rbtree, i);
        if (!n) klogf(LOG_ERROR, "Failed to find node of key %u\n", i);
        else    klogf(LOG_DEBUG, "Found node of key %u\n", i);
    }
}
