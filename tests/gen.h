/**
 * @file gen.h
 * @brief Formula generators shared by the property tests and the benchmark.
 *
 * Every generator returns a formula in canonical infix form (SPEC 4.3), so
 * `inorder(prefixToTree(infixToPrefix(tokenize(s)))) == s` must hold.
 */
#ifndef GEN_H
#define GEN_H

#include <cstdint>
#include <random>
#include <string>

/**
 * @brief Generate a random formula with exactly `nodes` parse-tree nodes.
 *
 * Splits the node budget at a random point, so the expected height is
 * O(log n) rather than Theta(n).
 *
 * @param rng      Random engine (the caller seeds it, so runs are reproducible).
 * @param nodes    Exact number of tree nodes, at least 1.
 * @param numAtoms Number of distinct atom names `x1 .. x<numAtoms>` to draw from, at least 1.
 * @return The canonical infix string.
 * @throws std::invalid_argument if `nodes < 1` or `numAtoms < 1`.
 * @note Complexity: O(n) time / O(n) space.
 */
std::string randomFormula(std::mt19937_64& rng, long nodes, int numAtoms);

/**
 * @brief Generate a left-deep chain `((x1 op x2) op x3) ...` with `leaves` atoms.
 *
 * The tree has height `leaves - 1`, the worst case for recursion.
 *
 * @param leaves Number of distinct atoms `x1 .. x<leaves>`, at least 1.
 * @param op     Binary operator character: '*', '+' or '>'.
 * @return The canonical infix string (2 * leaves - 1 nodes).
 * @throws std::invalid_argument if `leaves < 1`.
 * @note Complexity: O(n) time / O(n) space (no repeated string copying).
 */
std::string leftDeepChain(long leaves, char op = '+');

#endif // GEN_H
