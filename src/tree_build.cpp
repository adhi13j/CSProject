/**
 * @file tree_build.cpp
 * @brief Task 2: prefix tokens to a binary parse tree, and iterative tree release.
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <stdexcept>

namespace {

/**
 * @brief True if `t` is a syntactically valid atom `[A-Za-z][A-Za-z0-9_]*`.
 * @param t A token.
 * @return Whether it is an atom.
 * @throws Nothing.
 * @note Complexity: O(|t|) time / O(1) space.
 */
bool isAtom(const std::string& t) {
    if (t.empty()) {
        return false;
    }
    const char c0 = t[0];
    if (!((c0 >= 'A' && c0 <= 'Z') || (c0 >= 'a' && c0 <= 'z'))) {
        return false;
    }
    for (char c : t) {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_';
        if (!ok) {
            return false;
        }
    }
    return true;
}

/**
 * @brief Free every tree whose root is in `roots`, then clear the vector.
 * @param roots Roots still owned by the caller.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(total nodes) time / O(h) space.
 */
void freeAll(std::vector<Node*>& roots) {
    for (Node* r : roots) {
        freeTree(r);
    }
    roots.clear();
}

} // namespace

/**
 * @copydoc prefixToTree
 *
 * @details Scanning right to left means that when an operator is reached, its
 * operands are already built and sitting on the stack: the top is the FIRST
 * operand (it was pushed last), so it becomes `left`; the next one becomes `right`.
 */
Node* prefixToTree(const std::vector<std::string>& prefixTokens) {
    std::vector<Node*> stack;
    stack.reserve(prefixTokens.size());
    try {
        for (size_t k = prefixTokens.size(); k-- > 0;) {
            const std::string& t = prefixTokens[k];
            if (t == "~") {
                if (stack.empty()) {
                    throw std::invalid_argument("'~' is missing its operand (token " + std::to_string(k) + ")");
                }
                // NOT keeps its operand in `right`, with `left` null (SPEC 3.1).
                Node* operand = stack.back();
                stack.pop_back();
                stack.push_back(new Node{NodeType::NOT, t, nullptr, operand});
            } else if (t == "*" || t == "+" || t == ">") {
                if (stack.size() < 2) {
                    throw std::invalid_argument("operator '" + t + "' is missing operands (token " +
                                                std::to_string(k) + ")");
                }
                Node* first = stack.back();
                stack.pop_back();
                Node* second = stack.back();
                stack.pop_back();
                const NodeType type = t == "*" ? NodeType::AND : (t == "+" ? NodeType::OR : NodeType::IMPLIES);
                stack.push_back(new Node{type, t, first, second});
            } else if (isAtom(t)) {
                stack.push_back(new Node{NodeType::ATOM, t, nullptr, nullptr});
            } else {
                throw std::invalid_argument("illegal prefix token '" + t + "' (token " + std::to_string(k) + ")");
            }
        }
        if (stack.size() != 1) {
            throw std::invalid_argument(stack.empty() ? "empty prefix expression"
                                                      : "malformed prefix expression: " +
                                                            std::to_string(stack.size()) +
                                                            " subtrees left after building (expected 1)");
        }
    } catch (...) {
        freeAll(stack);   // no leak on a malformed input
        throw;
    }
    return stack.back();
}

/**
 * @copydoc freeTree
 *
 * @details A recursive delete would use one call frame per tree level, and a
 * left-deep CNF tree has hundreds of thousands of levels. An explicit stack on
 * the heap has no such limit.
 */
void freeTree(Node* root) {
    std::vector<Node*> pending;
    if (root != nullptr) {
        pending.push_back(root);
    }
    while (!pending.empty()) {
        Node* n = pending.back();
        pending.pop_back();
        if (n->left != nullptr) {
            pending.push_back(n->left);
        }
        if (n->right != nullptr) {
            pending.push_back(n->right);
        }
        delete n;
    }
}
