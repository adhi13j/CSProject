/**
 * @file tokenizer.cpp
 * @brief Task 1, step 1: splitting formula text into tokens.
 *
 * Every later algorithm works on a std::vector<std::string> of tokens and never
 * on raw characters, because reversing characters would turn `x12` into `21x`.
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <stdexcept>

namespace {

/** @brief True for A-Z / a-z (ASCII only, so the result never depends on the locale). */
bool isAlpha(char c) {
    return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
}

/** @brief True for 0-9. */
bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

/** @brief True for the whitespace characters that are skipped everywhere. */
bool isSpace(char c) {
    return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\v' || c == '\f';
}

} // namespace

/** @copydoc tokenize */
std::vector<std::string> tokenize(const std::string& infix) {
    std::vector<std::string> tokens;
    const size_t n = infix.size();
    size_t i = 0;
    while (i < n) {
        const char c = infix[i];
        if (isSpace(c)) {
            ++i;
        } else if (c == '~' || c == '*' || c == '+' || c == '>' || c == '(' || c == ')') {
            tokens.emplace_back(1, c);
            ++i;
        } else if (isAlpha(c)) {
            // ATOM ::= [A-Za-z][A-Za-z0-9_]*  -- take the longest match so that
            // "x12" stays one token.
            size_t j = i + 1;
            while (j < n && (isAlpha(infix[j]) || isDigit(infix[j]) || infix[j] == '_')) {
                ++j;
            }
            tokens.emplace_back(infix, i, j - i);
            i = j;
        } else {
            throw std::invalid_argument(std::string("illegal character '") + c + "' at position " +
                                        std::to_string(i));
        }
    }
    return tokens;
}

/** @copydoc joinTokens */
std::string joinTokens(const std::vector<std::string>& tokens, const std::string& sep) {
    std::string out;
    for (size_t i = 0; i < tokens.size(); ++i) {
        if (i != 0) {
            out += sep;
        }
        out += tokens[i];
    }
    return out;
}
