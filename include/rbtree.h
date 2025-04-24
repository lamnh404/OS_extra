#ifndef RBTREE_H
#define RBTREE_H

#include <stdlib.h>

typedef enum { RED, BLACK } Color;

// Forward declarations
typedef struct RBNode RBNode;
typedef struct RBTree RBTree;

// Function pointer types
typedef int (*CmpOp)(void*, void*);
typedef void* (*CloneFunc)(void*);
typedef void (*FreeFunc)(void*);
typedef void (*PrintFunc)(void*);

// Node structure
struct RBNode {
    void* data;
    Color color;
    RBNode* left;
    RBNode* right;
    RBNode* parent;
};

struct RBTree {
    RBNode* root;
    CmpOp cmpop;
    CloneFunc clone_data;
    FreeFunc free_data;
};

// Public API
RBTree* new_rbtree(CmpOp cmpop, CloneFunc clone_data, FreeFunc free_data);
void destroy_rbtree(RBTree* tree);
void rbtree_insert(RBTree* tree, void* data);
void rbtree_delete(RBTree* tree, void* data);
void rbtree_print(RBTree* tree, PrintFunc print);
void* rbtree_search(RBTree* tree, void* key);

#endif // RBTREE_H
