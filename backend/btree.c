#include "filesystem.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Simple B-tree node structure (for metadata indexing)
#define BTREE_ORDER 5

typedef struct BTreeNode {
    uint32_t keys[BTREE_ORDER - 1];
    void *values[BTREE_ORDER - 1];
    struct BTreeNode *children[BTREE_ORDER];
    uint32_t num_keys;
    bool is_leaf;
} BTreeNode;

typedef struct {
    BTreeNode *root;
    uint32_t size;
} BTree;

// Create a new B-tree node
BTreeNode* btree_create_node(bool is_leaf) {
    BTreeNode *node = (BTreeNode *)calloc(1, sizeof(BTreeNode));
    node->is_leaf = is_leaf;
    node->num_keys = 0;
    return node;
}

// Create a new B-tree
BTree* btree_create() {
    BTree *tree = (BTree *)malloc(sizeof(BTree));
    tree->root = btree_create_node(true);
    tree->size = 0;
    return tree;
}

// Search for a key in the B-tree
void* btree_search(BTreeNode *node, uint32_t key) {
    if (node == NULL) return NULL;
    
    uint32_t i = 0;
    while (i < node->num_keys && key > node->keys[i]) {
        i++;
    }
    
    if (i < node->num_keys && key == node->keys[i]) {
        return node->values[i];
    }
    
    if (node->is_leaf) {
        return NULL;
    }
    
    return btree_search(node->children[i], key);
}

// Split a full child node
void btree_split_child(BTreeNode *parent, uint32_t index) {
    BTreeNode *full_child = parent->children[index];
    BTreeNode *new_child = btree_create_node(full_child->is_leaf);
    
    uint32_t mid = (BTREE_ORDER - 1) / 2;
    new_child->num_keys = BTREE_ORDER - 1 - mid - 1;
    
    // Copy second half of keys to new child
    for (uint32_t i = 0; i < new_child->num_keys; i++) {
        new_child->keys[i] = full_child->keys[i + mid + 1];
        new_child->values[i] = full_child->values[i + mid + 1];
    }
    
    // Copy children if not leaf
    if (!full_child->is_leaf) {
        for (uint32_t i = 0; i <= new_child->num_keys; i++) {
            new_child->children[i] = full_child->children[i + mid + 1];
        }
    }
    
    full_child->num_keys = mid;
    
    // Insert middle key into parent
    for (int32_t i = parent->num_keys; i > (int32_t)index; i--) {
        parent->children[i + 1] = parent->children[i];
    }
    parent->children[index + 1] = new_child;
    
    for (int32_t i = parent->num_keys - 1; i >= (int32_t)index; i--) {
        parent->keys[i + 1] = parent->keys[i];
        parent->values[i + 1] = parent->values[i];
    }
    parent->keys[index] = full_child->keys[mid];
    parent->values[index] = full_child->values[mid];
    parent->num_keys++;
}

// Insert into a non-full node
void btree_insert_non_full(BTreeNode *node, uint32_t key, void *value) {
    int32_t i = node->num_keys - 1;
    
    if (node->is_leaf) {
        // Find position and shift keys
        while (i >= 0 && key < node->keys[i]) {
            node->keys[i + 1] = node->keys[i];
            node->values[i + 1] = node->values[i];
            i--;
        }
        
        node->keys[i + 1] = key;
        node->values[i + 1] = value;
        node->num_keys++;
    } else {
        // Find child to insert into
        while (i >= 0 && key < node->keys[i]) {
            i--;
        }
        i++;
        
        if (node->children[i]->num_keys == BTREE_ORDER - 1) {
            btree_split_child(node, i);
            
            if (key > node->keys[i]) {
                i++;
            }
        }
        
        btree_insert_non_full(node->children[i], key, value);
    }
}

// Insert a key-value pair into the B-tree
void btree_insert(BTree *tree, uint32_t key, void *value) {
    BTreeNode *root = tree->root;
    
    if (root->num_keys == BTREE_ORDER - 1) {
        // Root is full, create new root
        BTreeNode *new_root = btree_create_node(false);
        new_root->children[0] = root;
        tree->root = new_root;
        
        btree_split_child(new_root, 0);
        btree_insert_non_full(new_root, key, value);
    } else {
        btree_insert_non_full(root, key, value);
    }
    
    tree->size++;
}

// Free B-tree
void btree_free(BTreeNode *node) {
    if (node == NULL) return;
    
    if (!node->is_leaf) {
        for (uint32_t i = 0; i <= node->num_keys; i++) {
            btree_free(node->children[i]);
        }
    }
    
    free(node);
}

void btree_destroy(BTree *tree) {
    if (tree) {
        btree_free(tree->root);
        free(tree);
    }
}
