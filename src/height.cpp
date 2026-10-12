/**
 * @file height.cpp
 * @brief Task 4: tree height, recursive and iterative (BFS).
 *
 * Convention (SPEC 4.4): height(nullptr) = -1, height(leaf) = 0.
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <algorithm>
#include <queue>

/** @copydoc heightRec */
int heightRec(const Node* root) {
    if (root == nullptr) {
        return -1;
    }
    return 1 + std::max(heightRec(root->left), heightRec(root->right));
}

/**
 * @copydoc heightIter
 *
 * @details Level-order traversal: each pass of the outer loop empties exactly one
 * level of the queue. The number of levels minus one is the height, which also
 * gives -1 for the empty tree (0 levels).
 */
int heightIter(const Node* root) {
    if (root == nullptr) {
        return -1;
    }
    std::queue<const Node*> level;
    level.push(root);
    int levels = 0;
    while (!level.empty()) {
        ++levels;
        for (size_t remaining = level.size(); remaining > 0; --remaining) {
            const Node* n = level.front();
            level.pop();
            if (n->left != nullptr) {
                level.push(n->left);
            }
            if (n->right != nullptr) {
                level.push(n->right);
            }
        }
    }
    return levels - 1;
}
