/**
 * @file cnf_reader.cpp
 * @brief DIMACS CNF input and conversion of a CNF into a fully parenthesized infix string.
 *
 * Format reference: https://jix.github.io/varisat/manual/0.2.0/formats/dimacs.html
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <cerrno>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace {

/**
 * @brief Parse a whole token as a base-10 integer that fits in `int`.
 * @param tok  The token text.
 * @param what Description used in the error message.
 * @return The integer value.
 * @throws std::invalid_argument if the token is not entirely an integer or is out of range.
 * @note Complexity: O(|tok|) time / O(1) space.
 */
int parseInt(const std::string& tok, const char* what) {
    char* end = nullptr;
    errno = 0;
    const long v = std::strtol(tok.c_str(), &end, 10);
    if (tok.empty() || *end != '\0' || errno == ERANGE || v > 2147483647L || v < -2147483647L) {
        throw std::invalid_argument(std::string("malformed DIMACS: ") + what + " '" + tok + "' is not an integer");
    }
    return static_cast<int>(v);
}

/**
 * @brief Append the infix text of a literal: `k` -> `xk`, `-k` -> `(~xk)`.
 * @param lit  A non-zero literal.
 * @param out  Output buffer, appended to.
 * @throws std::invalid_argument if `lit == 0`.
 * @note Complexity: O(digits) time / O(1) extra space.
 */
void appendLiteral(int lit, std::string& out) {
    if (lit == 0) {
        throw std::invalid_argument("malformed CNF: literal 0 inside a clause");
    }
    if (lit > 0) {
        out += 'x';
        out += std::to_string(lit);
    } else {
        out += "(~x";
        out += std::to_string(-static_cast<long>(lit));
        out += ')';
    }
}

/**
 * @brief Append one clause as a left-deep disjunction `((a+b)+c)`; a single literal stays bare.
 * @param clause The literals, non-empty.
 * @param out    Output buffer, appended to.
 * @throws std::invalid_argument for an empty clause.
 * @note Complexity: O(clause length) time / O(1) extra space.
 */
void appendClause(const std::vector<int>& clause, std::string& out) {
    if (clause.empty()) {
        throw std::invalid_argument("malformed CNF: empty clause");
    }
    // All opening parentheses first, then each further literal closes one:
    // this builds the left-deep shape with one pass and no string copying.
    out.append(clause.size() - 1, '(');
    appendLiteral(clause[0], out);
    for (size_t i = 1; i < clause.size(); ++i) {
        out += '+';
        appendLiteral(clause[i], out);
        out += ')';
    }
}

/**
 * @brief Append clauses [lo, hi) joined by `*`, split recursively at the midpoint.
 * @param cnf The formula.
 * @param lo  First clause index.
 * @param hi  One past the last clause index; `hi > lo`.
 * @param out Output buffer, appended to.
 * @throws std::invalid_argument for an empty clause.
 * @note Complexity: O(literals in range) time / O(log(hi - lo)) call stack.
 */
void appendBalanced(const CNF& cnf, size_t lo, size_t hi, std::string& out) {
    if (hi - lo == 1) {
        appendClause(cnf.clauses[lo], out);
        return;
    }
    // Midpoint split: the left half gets floor(size / 2) clauses, so the
    // recursion depth is only ceil(log2(#clauses)) -- safe to recurse here.
    const size_t mid = lo + (hi - lo) / 2;
    out += '(';
    appendBalanced(cnf, lo, mid, out);
    out += '*';
    appendBalanced(cnf, mid, hi, out);
    out += ')';
}

} // namespace

CNF readDimacs(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::invalid_argument("cannot open DIMACS file '" + path + "'");
    }
    CNF cnf;
    int headerClauses = 0;
    bool haveHeader = false;
    std::vector<int> current;
    std::string line;
    while (std::getline(in, line)) {
        const size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos) {
            continue;   // blank line
        }
        const char c = line[first];
        if (c == 'c') {
            continue;   // comment
        }
        if (c == '%') {
            break;      // some SATLIB files end with "%\n0\n"
        }
        std::istringstream ls(line.substr(first));
        if (c == 'p') {
            std::string p, format, vars, clauses;
            ls >> p >> format >> vars >> clauses;
            if (haveHeader) {
                throw std::invalid_argument("malformed DIMACS: more than one 'p cnf' header");
            }
            if (p != "p" || format != "cnf" || clauses.empty()) {
                throw std::invalid_argument("malformed DIMACS: header must be 'p cnf <vars> <clauses>'");
            }
            cnf.numVars = parseInt(vars, "variable count");
            headerClauses = parseInt(clauses, "clause count");
            haveHeader = true;
            continue;
        }
        if (!haveHeader) {
            throw std::invalid_argument("malformed DIMACS: clause data before the 'p cnf' header");
        }
        std::string tok;
        while (ls >> tok) {
            const int lit = parseInt(tok, "literal");
            if (lit == 0) {
                if (current.empty()) {
                    throw std::invalid_argument("malformed DIMACS: empty clause");
                }
                cnf.clauses.push_back(std::move(current));
                current.clear();
            } else {
                current.push_back(lit);
            }
        }
    }
    if (!haveHeader) {
        throw std::invalid_argument("malformed DIMACS: missing 'p cnf' header");
    }
    if (!current.empty()) {
        throw std::invalid_argument("malformed DIMACS: last clause is not terminated by 0");
    }
    cnf.numClauses = static_cast<int>(cnf.clauses.size());
    if (cnf.numClauses != headerClauses) {
        std::cerr << "Warning: header declares " << headerClauses << " clauses but " << cnf.numClauses
                  << " were read\n";
    }
    return cnf;
}

std::string cnfToInfix(const CNF& cnf, TreeShape shape) {
    if (cnf.clauses.empty()) {
        throw std::invalid_argument("malformed CNF: empty formula (no clauses)");
    }
    size_t literals = 0;
    for (const std::vector<int>& cl : cnf.clauses) {
        literals += cl.size();
    }
    std::string out;
    out.reserve(literals * 12 + cnf.clauses.size() * 2);   // rough: "(~x123)+" per literal

    if (shape == TreeShape::BALANCED) {
        appendBalanced(cnf, 0, cnf.clauses.size(), out);
        return out;
    }
    // LEFT_DEEP: ((C1*C2)*C3)... built iteratively -- the formula may have
    // millions of clauses, so no recursion here.
    out.append(cnf.clauses.size() - 1, '(');
    appendClause(cnf.clauses[0], out);
    for (size_t i = 1; i < cnf.clauses.size(); ++i) {
        out += '*';
        appendClause(cnf.clauses[i], out);
        out += ')';
    }
    return out;
}
