/**
 * @file formula.h
 * @brief Public contract for CS F214 Assignment 1: propositional-logic parse trees.
 *
 * Declares the tree node, the five Part A tasks (infix -> prefix, prefix -> tree,
 * traversals, height, evaluation / truth table) and the DIMACS CNF reader.
 * SPEC.md is the source of truth for every behaviour described here.
 *
 * @authors Replace with group member names.
 */
#ifndef FORMULA_H
#define FORMULA_H

#include <iosfwd>
#include <string>
#include <unordered_map>
#include <vector>

/** @brief Kind of a parse-tree node. */
enum class NodeType { ATOM, NOT, AND, OR, IMPLIES };

/**
 * @brief One node of the binary parse tree.
 *
 * Invariants:
 *  - ATOM: `left == nullptr && right == nullptr`.
 *  - NOT: `left == nullptr`; the operand is stored in `right`, so in-order
 *    traversal prints `~` before its operand with no special case.
 *  - AND / OR / IMPLIES: both children are non-null; `left` is the left operand.
 *
 * Ownership: there is deliberately no destructor that deletes children (a
 * recursive delete overflows the stack on deep trees). Use freeTree().
 */
struct Node {
    NodeType    type;
    std::string value;          ///< atom name, or the operator symbol "~", "*", "+", ">"
    Node*       left  = nullptr;
    Node*       right = nullptr;
};

/** @brief Truth values of the atoms, keyed by atom name. */
using Assignment = std::unordered_map<std::string, bool>;

/** @brief A CNF formula read from a DIMACS file. */
struct CNF {
    int numVars    = 0;                          ///< variable count from the `p cnf` header
    int numClauses = 0;                          ///< clauses actually read
    std::vector<std::vector<int>> clauses;       ///< each clause is a list of non-zero literals
};

/** @brief Shape of the tree built for a CNF formula (see cnfToInfix()). */
enum class TreeShape { LEFT_DEEP, BALANCED };

// ---------------------------------------------------------------------------
// Task 1 -- tokenize and infix -> prefix
// ---------------------------------------------------------------------------

/**
 * @brief Split an infix formula into tokens.
 *
 * Tokens are ATOM (`[A-Za-z][A-Za-z0-9_]*`), `~`, `*`, `+`, `>`, `(` and `)`.
 * Whitespace is ignored everywhere.
 *
 * @param infix The formula text.
 * @return The token list, in input order.
 * @throws std::invalid_argument on an illegal character.
 * @note Complexity: O(n) time / O(n) space, n = number of characters.
 */
std::vector<std::string> tokenize(const std::string& infix);

/**
 * @brief Convert infix tokens to prefix tokens.
 *
 * Reverses the tokens, swaps parentheses, runs infix -> postfix with an operator
 * stack, and reverses the result (SPEC 4.1). Precedence: `~`(4, right) > `*`(3, left)
 * > `+`(2, left) > `>`(1, right).
 *
 * @param infixTokens Tokens from tokenize().
 * @return The prefix token list.
 * @throws std::invalid_argument on unbalanced parentheses, an operator missing
 *         operands, or two adjacent operands.
 * @note Complexity: O(n) time / O(n) space, n = number of tokens.
 */
std::vector<std::string> infixToPrefix(const std::vector<std::string>& infixTokens);

/**
 * @brief Join tokens into one string.
 *
 * @param tokens The tokens.
 * @param sep    Separator placed between tokens (no leading or trailing separator).
 * @return The joined string; empty if `tokens` is empty.
 * @throws Nothing.
 * @note Complexity: O(total length) time / O(total length) space.
 */
std::string joinTokens(const std::vector<std::string>& tokens, const std::string& sep = " ");

// ---------------------------------------------------------------------------
// Task 2 -- prefix -> parse tree
// ---------------------------------------------------------------------------

/**
 * @brief Build a parse tree from prefix tokens.
 *
 * Scans right to left with a stack of nodes. The caller owns the result and
 * must release it with freeTree().
 *
 * @param prefixTokens Prefix tokens from infixToPrefix().
 * @return The root of a newly allocated tree.
 * @throws std::invalid_argument if an operator lacks operands, or if the stack
 *         does not end with exactly one node.
 * @note Complexity: O(n) time / O(n) space, n = number of tokens.
 */
Node* prefixToTree(const std::vector<std::string>& prefixTokens);

/**
 * @brief Release a whole tree. Iterative, so deep trees cannot overflow the call stack.
 *
 * @param root Root of the tree; may be nullptr.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) space.
 */
void freeTree(Node* root);

// ---------------------------------------------------------------------------
// Task 3 -- traversals
// ---------------------------------------------------------------------------

/**
 * @brief Recursive in-order traversal, returning canonical infix.
 *
 * Canonical form: atom `p`, negation `(~X)`, binary `(L op R)`, no spaces.
 *
 * @param root Root of the tree (may be nullptr).
 * @return The canonical infix string; empty for a null tree.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) call stack.
 */
std::string inorderRec(const Node* root);

/**
 * @brief Iterative in-order traversal (explicit stack of node/state frames).
 *
 * @param root Root of the tree (may be nullptr).
 * @return The canonical infix string; equal to inorderRec(root).
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) heap stack.
 */
std::string inorderIter(const Node* root);

/**
 * @brief Recursive pre-order traversal.
 *
 * @param root Root of the tree (may be nullptr).
 * @return Node values separated by single spaces, no leading or trailing space.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) call stack.
 */
std::string preorderRec(const Node* root);

/**
 * @brief Iterative pre-order traversal (stack: pop and visit, push right, push left).
 *
 * @param root Root of the tree (may be nullptr).
 * @return Same string as preorderRec(root).
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) heap stack.
 */
std::string preorderIter(const Node* root);

/**
 * @brief Recursive post-order traversal.
 *
 * @param root Root of the tree (may be nullptr).
 * @return Node values separated by single spaces, no leading or trailing space.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) call stack.
 */
std::string postorderRec(const Node* root);

/**
 * @brief Iterative post-order traversal (one stack plus a last-visited pointer).
 *
 * @param root Root of the tree (may be nullptr).
 * @return Same string as postorderRec(root).
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) heap stack.
 */
std::string postorderIter(const Node* root);

// ---------------------------------------------------------------------------
// Task 4 -- height
// ---------------------------------------------------------------------------

/**
 * @brief Recursive height. `height(nullptr) = -1`, `height(leaf) = 0`.
 *
 * @param root Root of the tree (may be nullptr).
 * @return The height in edges.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) call stack.
 */
int heightRec(const Node* root);

/**
 * @brief Iterative height by level-order BFS with std::queue.
 *
 * Counts the levels and returns `levels - 1`.
 *
 * @param root Root of the tree (may be nullptr).
 * @return The height in edges; equal to heightRec(root).
 * @throws Nothing.
 * @note Complexity: O(n) time / O(w) queue, w = maximum level width.
 */
int heightIter(const Node* root);

// ---------------------------------------------------------------------------
// Task 5 -- evaluation and truth table
// ---------------------------------------------------------------------------

/**
 * @brief Collect the distinct atoms of a tree, in natural sort order.
 *
 * Natural sort (SPEC 4.5): prefix, then trailing digits as a number, then the
 * whole name, so `x2` comes before `x10`.
 *
 * @param root Root of the tree (may be nullptr).
 * @return The sorted, distinct atom names.
 * @throws Nothing.
 * @note Complexity: O(n + k log k) time / O(k) space, k = number of distinct atoms.
 */
std::vector<std::string> collectAtoms(const Node* root);

/**
 * @brief Evaluate a formula recursively (post-order, bottom-up).
 *
 * `a > b` is `(!a) || b`.
 *
 * @param root Root of the tree; must be non-null.
 * @param a    Truth value of every atom in the tree.
 * @return The truth value of the formula.
 * @throws std::invalid_argument if an atom is missing from `a`.
 * @note Complexity: O(n) time / O(h) call stack.
 */
bool evaluateRec(const Node* root, const Assignment& a);

/**
 * @brief Evaluate a formula iteratively (post-order with a value stack).
 *
 * @param root Root of the tree; must be non-null.
 * @param a    Truth value of every atom in the tree.
 * @return The truth value; equal to evaluateRec(root, a).
 * @throws std::invalid_argument if an atom is missing from `a`.
 * @note Complexity: O(n) time / O(h) heap space.
 */
bool evaluateIter(const Node* root, const Assignment& a);

/**
 * @brief Print the complete truth table.
 *
 * Row i (0 .. 2^k - 1): atom j in sorted order has value `(i >> (k-1-j)) & 1`,
 * so the first atom is the most significant bit and row 0 is all-false. Columns
 * are separated by one space and values are printed as 0/1:
 * @code
 * p q r | F
 * 0 0 0 | 0
 * @endcode
 * If k > maxAtoms, writes `Truth table skipped: k=<k> atoms, 2^<k> rows (limit <maxAtoms>)`
 * and returns.
 *
 * @param root     Root of the tree.
 * @param out      Destination stream.
 * @param maxAtoms Largest atom count for which a table is generated.
 * @throws Nothing (evaluation cannot fail: every atom is assigned).
 * @note Complexity: O(2^k * n) time / O(h) extra space.
 */
void printTruthTable(const Node* root, std::ostream& out, int maxAtoms = 20);

/**
 * @brief The F column of the truth table only, e.g. `"01110101"`.
 *
 * @param root     Root of the tree.
 * @param maxAtoms Largest atom count for which the column is generated.
 * @return One `0`/`1` character per row in row order; empty if k > maxAtoms.
 * @throws Nothing.
 * @note Complexity: O(2^k * n) time / O(2^k) space for the result.
 */
std::string truthTableColumn(const Node* root, int maxAtoms = 20);

// ---------------------------------------------------------------------------
// DIMACS CNF input
// ---------------------------------------------------------------------------

/**
 * @brief Read a DIMACS CNF file.
 *
 * Skips `c` comment lines, stops at a line starting with `%`, accepts clauses
 * spanning lines or sharing a line, and warns on `stderr` (without failing) when
 * the clause count differs from the header.
 *
 * @param path Path of the `.cnf` file.
 * @return The parsed formula; `numClauses` is the number of clauses actually read.
 * @throws std::invalid_argument if the file cannot be opened, the `p cnf`
 *         header is missing, a token is not an integer, or a clause is empty.
 * @note Complexity: O(L) time / O(L) space, L = total number of literals.
 */
CNF readDimacs(const std::string& path);

/**
 * @brief Convert a CNF formula into a fully parenthesized infix string.
 *
 * Literal `k` becomes `xk`, `-k` becomes `(~xk)`. A clause is its literals joined
 * by `+`, left-deep: `((a+b)+c)`; a one-literal clause is the literal itself.
 * The formula is the clauses joined by `*`: LEFT_DEEP gives `((C1*C2)*C3)...`
 * with height Theta(number of clauses); BALANCED splits recursively at the midpoint.
 *
 * @param cnf   The formula.
 * @param shape How the clauses are combined.
 * @return The infix string, ready for tokenize().
 * @throws std::invalid_argument for an empty clause or an empty formula.
 * @note Complexity: O(L) time / O(L) space (BALANCED recursion depth is O(log(number of clauses))).
 */
std::string cnfToInfix(const CNF& cnf, TreeShape shape);

#endif // FORMULA_H
