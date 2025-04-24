#include "rbtree.h"
#include <stdlib.h>
#include <stdio.h>

// Create a new node, cloning data if requested
static RBNode* create_node(RBTree* tree, void* data) {
    RBNode* node = malloc(sizeof(RBNode));
    node->data = tree->clone_data ? tree->clone_data(data) : data;
    node->color = RED;
    node->left = node->right = node->parent = NULL;
    return node;
}

// Left rotate around x
static void left_rotate(RBTree* tree, RBNode* x) {
    RBNode* y = x->right;
    x->right = y->left;
    if (y->left) y->left->parent = x;
    y->parent = x->parent;
    if (!x->parent)
        tree->root = y;
    else if (x == x->parent->left)
        x->parent->left = y;
    else
        x->parent->right = y;
    y->left = x;
    x->parent = y;
}

// Right rotate around y
static void right_rotate(RBTree* tree, RBNode* y) {
    RBNode* x = y->left;
    y->left = x->right;
    if (x->right) x->right->parent = y;
    x->parent = y->parent;
    if (!y->parent)
        tree->root = x;
    else if (y == y->parent->left)
        y->parent->left = x;
    else
        y->parent->right = x;
    x->right = y;
    y->parent = x;
}

// Replace subtree u with v
static void transplant(RBTree* tree, RBNode* u, RBNode* v) {
    if (!u->parent)
        tree->root = v;
    else if (u == u->parent->left)
        u->parent->left = v;
    else
        u->parent->right = v;
    if (v) v->parent = u->parent;
}

// Find minimum in subtree
static RBNode* minimum(RBNode* node) {
    while (node->left)
        node = node->left;
    return node;
}

// Fix-up after deletion to restore red-black properties
static void fix_delete(RBTree* tree, RBNode* x) {
    while (x && x != tree->root && x->color == BLACK) {
        if (x == x->parent->left) {
            RBNode* w = x->parent->right;
            if (w && w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                left_rotate(tree, x->parent);
                w = x->parent->right;
            }
            if ((!w->left || w->left->color == BLACK) &&
                (!w->right || w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (!w->right || w->right->color == BLACK) {
                    if (w->left) w->left->color = BLACK;
                    w->color = RED;
                    right_rotate(tree, w);
                    w = x->parent->right;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->right) w->right->color = BLACK;
                left_rotate(tree, x->parent);
                x = tree->root;
            }
        } else {
            RBNode* w = x->parent->left;
            if (w && w->color == RED) {
                w->color = BLACK;
                x->parent->color = RED;
                right_rotate(tree, x->parent);
                w = x->parent->left;
            }
            if ((!w->left || w->left->color == BLACK) &&
                (!w->right || w->right->color == BLACK)) {
                w->color = RED;
                x = x->parent;
            } else {
                if (!w->left || w->left->color == BLACK) {
                    if (w->right) w->right->color = BLACK;
                    w->color = RED;
                    left_rotate(tree, w);
                    w = x->parent->left;
                }
                w->color = x->parent->color;
                x->parent->color = BLACK;
                if (w->left) w->left->color = BLACK;
                right_rotate(tree, x->parent);
                x = tree->root;
            }
        }
    }
    if (x) x->color = BLACK;
}

// Public delete: removes data if found
void rbtree_delete(RBTree* tree, void* data) {
    RBNode* z = tree->root;
    while (z && tree->cmpop(data, z->data) != 0)
        z = (tree->cmpop(data, z->data) < 0) ? z->left : z->right;
    if (!z) return;

    RBNode* y = z;
    Color y_color = y->color;
    RBNode* x = NULL;

    if (!z->left) {
        x = z->right;
        transplant(tree, z, z->right);
    } else if (!z->right) {
        x = z->left;
        transplant(tree, z, z->left);
    } else {
        y = minimum(z->right);
        y_color = y->color;
        x = y->right;
        if (y->parent == z) {
            if (x) x->parent = y;
        } else {
            transplant(tree, y, y->right);
            y->right = z->right;
            if (y->right) y->right->parent = y;
        }
        transplant(tree, z, y);
        y->left = z->left;
        if (y->left) y->left->parent = y;
        y->color = z->color;
    }

    if (tree->free_data)
        tree->free_data(z->data);
    free(z);
    if (y_color == BLACK)
        fix_delete(tree, x);
}

// Fix-up after insertion
static void fix_insert(RBTree* tree, RBNode* z) {
    while (z->parent && z->parent->color == RED) {
        if (z->parent == z->parent->parent->left) {
            RBNode* y = z->parent->parent->right;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->right) {
                    z = z->parent;
                    left_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                right_rotate(tree, z->parent->parent);
            }
        } else {
            RBNode* y = z->parent->parent->left;
            if (y && y->color == RED) {
                z->parent->color = BLACK;
                y->color = BLACK;
                z->parent->parent->color = RED;
                z = z->parent->parent;
            } else {
                if (z == z->parent->left) {
                    z = z->parent;
                    right_rotate(tree, z);
                }
                z->parent->color = BLACK;
                z->parent->parent->color = RED;
                left_rotate(tree, z->parent->parent);
            }
        }
    }
    tree->root->color = BLACK;
}

// Public insert
void rbtree_insert(RBTree* tree, void* data) {
    RBNode* z = create_node(tree, data);
    RBNode* y = NULL;
    RBNode* x = tree->root;
    while (x) {
        y = x;
        x = (tree->cmpop(z->data, x->data) < 0) ? x->left : x->right;
    }
    z->parent = y;
    if (!y)
        tree->root = z;
    else if (tree->cmpop(z->data, y->data) < 0)
        y->left = z;
    else
        y->right = z;
    fix_insert(tree, z);
}

// In-order traversal
static void inorder(RBNode* node, PrintFunc print) {
    if (!node) return;
    inorder(node->left, print);
    print(node->data);
    inorder(node->right, print);
}

// Public print
void rbtree_print(RBTree* tree, PrintFunc print) {
    inorder(tree->root, print);
}

// Public search: retrieves data if found
void* rbtree_search(RBTree* tree, void* key) {
    RBNode* cur = tree->root;
    while (cur) {
        int cmp = tree->cmpop(key, cur->data);
        if (cmp == 0)
            return cur->data;
        cur = (cmp < 0) ? cur->left : cur->right;
    }
    return NULL;
}

// Constructor
RBTree* new_rbtree(CmpOp cmpop, CloneFunc clone_data, FreeFunc free_data) {
    RBTree* tree = malloc(sizeof(RBTree));
    tree->root = NULL;
    tree->cmpop = cmpop;
    tree->clone_data = clone_data;
    tree->free_data = free_data;
    return tree;
}

// Recursively free nodes
static void free_node(RBNode* node, FreeFunc free_data) {
    if (!node) return;
    free_node(node->left, free_data);
    free_node(node->right, free_data);
    if (free_data) free_data(node->data);
    free(node);
}

// Destructor
void destroy_rbtree(RBTree* tree) {
    if (!tree) return;
    free_node(tree->root, tree->free_data);
    free(tree);
}
