/**
 * @file eval.cpp
 * @brief Task 5: atom collection (natural sort), evaluation, and truth tables.
 *
 * Semantics: `~`, `*`, `+` are the usual connectives and `a > b` is `(!a) || b`.
 * Both children of a binary node are always evaluated (no short-circuit), so a
 * missing atom is reported no matter which branch it sits in, and the recursive
 * and iterative evaluators fail in exactly the same situations.
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <unordered_set>

namespace {

/** @brief An atom name split into its non-digit prefix and its maximal trailing digit run. */
struct SplitName {
    std::string prefix;
    std::string digits;   ///< trailing digits, leading zeros stripped
    bool hasDigits;       ///< distinguishes "x" (no digits) from "x0" (digits "0", stripped to "")
};

/**
 * @brief Split `name` into prefix and trailing digits for natural sorting.
 * @param name An atom name.
 * @return The prefix, the digit run without leading zeros, and whether any digit was present.
 * @throws Nothing.
 * @note Complexity: O(|name|) time / O(|name|) space.
 */
SplitName splitName(const std::string& name) {
    size_t cut = name.size();
    while (cut > 0 && name[cut - 1] >= '0' && name[cut - 1] <= '9') {
        --cut;
    }
    SplitName s;
    s.prefix = name.substr(0, cut);
    s.hasDigits = cut < name.size();
    size_t z = cut;
    while (z < name.size() && name[z] == '0') {
        ++z;
    }
    s.digits = name.substr(z);
    return s;
}

/**
 * @brief Natural-sort ordering of atom names (SPEC 4.5): prefix, then number, then whole name.
 *
 * Digit runs are compared as strings by (length, then text) after stripping
 * leading zeros, which equals numeric comparison without ever converting to an
 * integer, so a 40-digit suffix cannot overflow.
 *
 * @param a First name.
 * @param b Second name.
 * @return True if `a` sorts strictly before `b`.
 * @throws Nothing.
 * @note Complexity: O(|a| + |b|) time / O(|a| + |b|) space.
 */
bool naturalLess(const std::string& a, const std::string& b) {
    const SplitName sa = splitName(a);
    const SplitName sb = splitName(b);
    if (sa.prefix != sb.prefix) {
        return sa.prefix < sb.prefix;
    }
    if (sa.hasDigits != sb.hasDigits) {
        return !sa.hasDigits;   // "x" before "x1"
    }
    if (sa.digits.size() != sb.digits.size()) {
        return sa.digits.size() < sb.digits.size();
    }
    if (sa.digits != sb.digits) {
        return sa.digits < sb.digits;
    }
    return a < b;   // same number, e.g. "x01" vs "x1": fall back to plain order
}

/**
 * @brief Look up an atom's truth value.
 * @param name Atom name.
 * @param a    The assignment.
 * @return Its value.
 * @throws std::invalid_argument if the atom is not in `a`.
 * @note Complexity: O(1) expected / O(1) space.
 */
bool lookup(const std::string& name, const Assignment& a) {
    const auto it = a.find(name);
    if (it == a.end()) {
        throw std::invalid_argument("atom '" + name + "' has no value in the assignment");
    }
    return it->second;
}

/**
 * @brief Apply a binary connective.
 * @param type Node type (AND, OR or IMPLIES).
 * @param l    Left operand value.
 * @param r    Right operand value.
 * @return The result.
 * @throws Nothing.
 * @note Complexity: O(1) / O(1).
 */
bool applyBinary(NodeType type, bool l, bool r) {
    switch (type) {
    case NodeType::AND:
        return l && r;
    case NodeType::OR:
        return l || r;
    default:   // IMPLIES: a > b  ==  (!a) || b
        return !l || r;
    }
}

/**
 * @brief Enumerate all 2^k rows and call `visit(rowValue)` with the formula's value.
 *
 * Row i gives atom j (0-based, in natural order) the bit `(i >> (k-1-j)) & 1`, so
 * the first atom is the most significant bit and row 0 is all-false.
 *
 * @param root  Formula root.
 * @param atoms Sorted atoms of the formula.
 * @param visit Called once per row, in row order, with the row's input bits and result.
 * @return Nothing.
 * @throws std::invalid_argument from evaluateIter (cannot happen: every atom is assigned).
 * @note Complexity: O(2^k * n) time / O(k) space.
 */
template <typename Visit>
void forEachRow(const Node* root, const std::vector<std::string>& atoms, Visit visit) {
    const size_t k = atoms.size();
    Assignment a;
    std::vector<bool*> slot(k);
    for (size_t j = 0; j < k; ++j) {
        slot[j] = &(a[atoms[j]] = false);   // element addresses stay valid: unordered_map is node-based
    }
    const unsigned long long rows = 1ULL << k;
    for (unsigned long long i = 0; i < rows; ++i) {
        for (size_t j = 0; j < k; ++j) {
            *slot[j] = ((i >> (k - 1 - j)) & 1ULL) != 0;
        }
        visit(i, evaluateIter(root, a));   // iterative: safe for any tree depth
    }
}

/** @brief Largest atom count that a 64-bit row counter can enumerate. */
constexpr int kHardAtomLimit = 62;

} // namespace

/**
 * @copydoc collectAtoms
 *
 * @details Uses an explicit stack (not recursion) so it also works on the deepest trees.
 */
std::vector<std::string> collectAtoms(const Node* root) {
    std::unordered_set<std::string> seen;
    std::vector<const Node*> stack;
    if (root != nullptr) {
        stack.push_back(root);
    }
    while (!stack.empty()) {
        const Node* n = stack.back();
        stack.pop_back();
        if (n->type == NodeType::ATOM) {
            seen.insert(n->value);
        }
        if (n->left != nullptr) {
            stack.push_back(n->left);
        }
        if (n->right != nullptr) {
            stack.push_back(n->right);
        }
    }
    std::vector<std::string> atoms(seen.begin(), seen.end());
    std::sort(atoms.begin(), atoms.end(), naturalLess);
    return atoms;
}

/** @copydoc evaluateRec */
bool evaluateRec(const Node* root, const Assignment& a) {
    if (root == nullptr) {
        throw std::invalid_argument("cannot evaluate an empty tree");
    }
    switch (root->type) {
    case NodeType::ATOM:
        return lookup(root->value, a);
    case NodeType::NOT:
        return !evaluateRec(root->right, a);   // the operand of NOT lives in `right`
    default: {
        const bool l = evaluateRec(root->left, a);    // post-order: children first
        const bool r = evaluateRec(root->right, a);
        return applyBinary(root->type, l, r);
    }
    }
}

/**
 * @copydoc evaluateIter
 *
 * @details Post-order with one node stack and a last-visited pointer (the same
 * scheme as postorderIter). A value stack holds finished subtrees: when an
 * operator is visited its operand values are the top entries, the right operand
 * being the very top because it was finished last.
 */
bool evaluateIter(const Node* root, const Assignment& a) {
    if (root == nullptr) {
        throw std::invalid_argument("cannot evaluate an empty tree");
    }
    std::vector<const Node*> stack;
    std::vector<char> values;
    const Node* cur = root;
    const Node* last = nullptr;
    while (cur != nullptr || !stack.empty()) {
        if (cur != nullptr) {
            stack.push_back(cur);
            cur = cur->left;
        } else {
            const Node* top = stack.back();
            if (top->right != nullptr && last != top->right) {
                cur = top->right;
            } else {
                if (top->type == NodeType::ATOM) {
                    values.push_back(lookup(top->value, a) ? 1 : 0);
                } else if (top->type == NodeType::NOT) {
                    values.back() = values.back() ? 0 : 1;
                } else {
                    const bool r = values.back() != 0;
                    values.pop_back();
                    const bool l = values.back() != 0;
                    values.back() = applyBinary(top->type, l, r) ? 1 : 0;
                }
                last = top;
                stack.pop_back();
            }
        }
    }
    return values.back() != 0;
}

/** @copydoc printTruthTable */
void printTruthTable(const Node* root, std::ostream& out, int maxAtoms) {
    const std::vector<std::string> atoms = collectAtoms(root);
    const int k = static_cast<int>(atoms.size());
    const int limit = std::min(maxAtoms, kHardAtomLimit);
    if (k > limit) {
        out << "Truth table skipped: k=" << k << " atoms, 2^" << k << " rows (limit " << limit << ")\n";
        return;
    }
    for (const std::string& name : atoms) {
        out << name << ' ';
    }
    out << "| F\n";
    forEachRow(root, atoms, [&](unsigned long long i, bool value) {
        for (int j = 0; j < k; ++j) {
            out << (((i >> (k - 1 - j)) & 1ULL) != 0 ? '1' : '0') << ' ';
        }
        out << "| " << (value ? '1' : '0') << '\n';
    });
}

/** @copydoc truthTableColumn */
std::string truthTableColumn(const Node* root, int maxAtoms) {
    const std::vector<std::string> atoms = collectAtoms(root);
    if (static_cast<int>(atoms.size()) > std::min(maxAtoms, kHardAtomLimit)) {
        return std::string();
    }
    std::string column;
    column.reserve(size_t{1} << atoms.size());
    forEachRow(root, atoms, [&](unsigned long long, bool value) { column += value ? '1' : '0'; });
    return column;
}
