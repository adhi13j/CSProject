/**
 * @file gen.cpp
 * @brief Implementation of the formula generators declared in gen.h.
 */
#include "gen.h"
#include <stdexcept>

namespace {

/**
 * @brief Append a random formula of exactly `budget` nodes to `out`.
 *
 * @param rng      Random engine.
 * @param budget   Nodes to generate, at least 1.
 * @param numAtoms Size of the atom pool.
 * @param out      Output buffer, appended to.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(budget) time / O(depth) call stack; the depth is O(log budget)
 *       in expectation because the budget is split uniformly at random.
 */
void build(std::mt19937_64& rng, long budget, int numAtoms, std::string& out) {
    if (budget == 1) {
        out += 'x';
        out += std::to_string(1 + static_cast<int>(rng() % static_cast<unsigned>(numAtoms)));
        return;
    }
    // A NOT takes 1 node plus its operand; use it about 1 time in 5, and always
    // when only 2 nodes remain (a binary node needs at least 3).
    if (budget == 2 || rng() % 5 == 0) {
        out += "(~";
        build(rng, budget - 1, numAtoms, out);
        out += ')';
        return;
    }
    const long leftSize = 1 + static_cast<long>(rng() % static_cast<unsigned long>(budget - 2));
    static const char ops[] = {'*', '+', '>'};
    out += '(';
    build(rng, leftSize, numAtoms, out);
    out += ops[rng() % 3];
    build(rng, budget - 1 - leftSize, numAtoms, out);
    out += ')';
}

} // namespace

std::string randomFormula(std::mt19937_64& rng, long nodes, int numAtoms) {
    if (nodes < 1 || numAtoms < 1) {
        throw std::invalid_argument("randomFormula: nodes and numAtoms must be >= 1");
    }
    std::string out;
    out.reserve(static_cast<size_t>(nodes) * 5);
    build(rng, nodes, numAtoms, out);
    return out;
}

std::string leftDeepChain(long leaves, char op) {
    if (leaves < 1) {
        throw std::invalid_argument("leftDeepChain: leaves must be >= 1");
    }
    // Emit all the opening parentheses first, then "x1", then "op x_i )" for each
    // further leaf. Wrapping the string once per leaf would copy it each time (O(n^2)).
    std::string out(static_cast<size_t>(leaves - 1), '(');
    out += "x1";
    for (long i = 2; i <= leaves; ++i) {
        out += op;
        out += 'x';
        out += std::to_string(i);
        out += ')';
    }
    return out;
}
