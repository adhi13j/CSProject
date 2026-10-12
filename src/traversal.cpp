/**
 * @file traversal.cpp
 * @brief Task 3: in-order, pre-order and post-order traversals, recursive and iterative.
 *
 * Output formats (SPEC 4.3): in-order is canonical infix with no spaces
 * (`p`, `(~X)`, `(L op R)`); pre-order and post-order are single-space separated.
 *
 * The recursive versions append to one shared buffer through a helper. Returning
 * and concatenating a std::string at every level would copy the text once per
 * level (O(n*h) time, O(n^2) on a left-deep tree).
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <utility>

namespace {

/**
 * @brief Append `token` to `out`, preceded by a single space unless `out` is empty.
 * @param out   Output buffer.
 * @param token Text to append.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(|token|) amortised / O(1) extra space.
 */
void emitSpaced(std::string& out, const std::string& token) {
    if (!out.empty()) {
        out += ' ';
    }
    out += token;
}

/**
 * @brief Recursive helper for inorderRec.
 * @param n   Current node (may be nullptr).
 * @param out Output buffer, appended to.
 * @return Nothing.
 * @throws Nothing (but may exhaust the call stack on very deep trees; that is the point of the iterative twin).
 * @note Complexity: O(subtree size) time / O(h) call stack.
 */
void inorderAppend(const Node* n, std::string& out) {
    if (n == nullptr) {
        return;
    }
    if (n->type == NodeType::ATOM) {
        out += n->value;
        return;
    }
    out += '(';
    if (n->type == NodeType::NOT) {
        out += n->value;                 // "~" comes before its operand, which lives in `right`
        inorderAppend(n->right, out);
    } else {
        inorderAppend(n->left, out);
        out += n->value;
        inorderAppend(n->right, out);
    }
    out += ')';
}

/** @brief Recursive helper for preorderRec: node, left, right. @note O(subtree) time / O(h) stack. */
void preorderAppend(const Node* n, std::string& out) {
    if (n == nullptr) {
        return;
    }
    emitSpaced(out, n->value);
    preorderAppend(n->left, out);
    preorderAppend(n->right, out);
}

/** @brief Recursive helper for postorderRec: left, right, node. @note O(subtree) time / O(h) stack. */
void postorderAppend(const Node* n, std::string& out) {
    if (n == nullptr) {
        return;
    }
    postorderAppend(n->left, out);
    postorderAppend(n->right, out);
    emitSpaced(out, n->value);
}

/** @brief What an in-order stack frame still has to do for its node. */
enum class State { OPEN, MIDDLE, CLOSE };

} // namespace

/** @copydoc inorderRec */
std::string inorderRec(const Node* root) {
    std::string out;
    inorderAppend(root, out);
    return out;
}

/**
 * @copydoc inorderIter
 *
 * @details A plain "push the left chain, pop, visit, go right" loop is not
 * enough: parentheses must be printed before the left subtree and after the right
 * one, and a NOT node has no left child at all (its operand is in `right`). So each
 * stack entry is a (node, state) frame: OPEN prints `(` and schedules the left
 * subtree, MIDDLE prints the operator and schedules the right subtree, CLOSE
 * prints `)`.
 */
std::string inorderIter(const Node* root) {
    std::string out;
    if (root == nullptr) {
        return out;
    }
    std::vector<std::pair<const Node*, State>> stack;
    stack.emplace_back(root, State::OPEN);
    while (!stack.empty()) {
        const std::pair<const Node*, State> frame = stack.back();
        stack.pop_back();
        const Node* n = frame.first;
        switch (frame.second) {
        case State::OPEN:
            if (n->type == NodeType::ATOM) {
                out += n->value;
            } else {
                out += '(';
                stack.emplace_back(n, State::MIDDLE);          // runs after the left subtree
                if (n->left != nullptr) {
                    stack.emplace_back(n->left, State::OPEN);  // pushed last, so it runs first
                }
            }
            break;
        case State::MIDDLE:
            out += n->value;
            stack.emplace_back(n, State::CLOSE);
            stack.emplace_back(n->right, State::OPEN);
            break;
        case State::CLOSE:
            out += ')';
            break;
        }
    }
    return out;
}

/** @copydoc preorderRec */
std::string preorderRec(const Node* root) {
    std::string out;
    preorderAppend(root, out);
    return out;
}

/**
 * @copydoc preorderIter
 *
 * @details The right child is pushed before the left one so that the left child
 * is popped (and therefore visited) first.
 */
std::string preorderIter(const Node* root) {
    std::string out;
    if (root == nullptr) {
        return out;
    }
    std::vector<const Node*> stack;
    stack.push_back(root);
    while (!stack.empty()) {
        const Node* n = stack.back();
        stack.pop_back();
        emitSpaced(out, n->value);
        if (n->right != nullptr) {
            stack.push_back(n->right);
        }
        if (n->left != nullptr) {
            stack.push_back(n->left);
        }
    }
    return out;
}

/** @copydoc postorderRec */
std::string postorderRec(const Node* root) {
    std::string out;
    postorderAppend(root, out);
    return out;
}

/**
 * @copydoc postorderIter
 *
 * @details Version used: ONE stack plus a last-visited pointer. A node may be
 * visited only after its right subtree is done; `last` tells us whether we are
 * coming back up from that right subtree (visit now) or have not been there yet
 * (descend into it first). This keeps the stack at O(h), where the two-stack
 * variant would need O(n).
 */
std::string postorderIter(const Node* root) {
    std::string out;
    std::vector<const Node*> stack;
    const Node* cur = root;
    const Node* last = nullptr;
    while (cur != nullptr || !stack.empty()) {
        if (cur != nullptr) {
            stack.push_back(cur);
            cur = cur->left;          // null for ATOM and NOT: falls through to the else branch
        } else {
            const Node* top = stack.back();
            if (top->right != nullptr && last != top->right) {
                cur = top->right;     // right subtree not finished yet
            } else {
                emitSpaced(out, top->value);
                last = top;
                stack.pop_back();
            }
        }
    }
    return out;
}
