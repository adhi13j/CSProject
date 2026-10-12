/**
 * @file infix_prefix.cpp
 * @brief Task 1: infix tokens to prefix tokens (reverse, swap parentheses, shunting-yard, reverse).
 *
 * Reference: http://scanftree.com/Data_Structure/infix-to-prefix (see SPEC 4.1).
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <algorithm>
#include <stdexcept>

namespace {

/** @brief True if `t` is one of the operator tokens `~ * + >`. */
bool isOperator(const std::string& t) {
    return t == "~" || t == "*" || t == "+" || t == ">";
}

/** @brief True if `t` is a binary operator token. */
bool isBinary(const std::string& t) {
    return t == "*" || t == "+" || t == ">";
}

/**
 * @brief Precedence of an operator: `~` 4 > `*` 3 > `+` 2 > `>` 1 (SPEC 2.3).
 * @param op An operator token.
 * @return Its precedence; higher binds tighter.
 * @throws Nothing.
 * @note Complexity: O(1) / O(1).
 */
int precedence(const std::string& op) {
    if (op == "~") {
        return 4;
    }
    if (op == "*") {
        return 3;
    }
    if (op == "+") {
        return 2;
    }
    return 1;   // ">"
}

/**
 * @brief Associativity in the ORIGINAL (un-reversed) formula: `~` and `>` are right-associative.
 * @param op An operator token.
 * @return True for right-associative operators.
 * @throws Nothing.
 * @note Complexity: O(1) / O(1).
 */
bool isRightAssoc(const std::string& op) {
    return op == "~" || op == ">";
}

/**
 * @brief Reject malformed token sequences before converting.
 *
 * A two-state machine (expecting an operand vs. expecting an operator or `)`)
 * catches every shape error with a precise message, so the converter itself can
 * assume a well-formed input.
 *
 * @param tokens The infix tokens.
 * @return Nothing.
 * @throws std::invalid_argument on unbalanced parentheses, an operator missing
 *         operands, two adjacent operands, or an empty formula.
 * @note Complexity: O(n) time / O(1) space.
 */
void validate(const std::vector<std::string>& tokens) {
    bool expectOperand = true;
    long depth = 0;
    for (size_t i = 0; i < tokens.size(); ++i) {
        const std::string& t = tokens[i];
        const std::string where = " (token " + std::to_string(i) + ": '" + t + "')";
        if (t == "(") {
            if (!expectOperand) {
                throw std::invalid_argument("missing operator before '('" + where);
            }
            ++depth;
        } else if (t == ")") {
            if (expectOperand) {
                throw std::invalid_argument("operator missing an operand before ')'" + where);
            }
            if (--depth < 0) {
                throw std::invalid_argument("unbalanced parentheses: extra ')'" + where);
            }
        } else if (t == "~") {
            if (!expectOperand) {
                throw std::invalid_argument("'~' cannot follow an operand" + where);
            }
        } else if (isBinary(t)) {
            if (expectOperand) {
                throw std::invalid_argument("binary operator missing its left operand" + where);
            }
            expectOperand = true;
        } else {   // an atom
            if (!expectOperand) {
                throw std::invalid_argument("two adjacent operands (missing operator)" + where);
            }
            expectOperand = false;
        }
    }
    if (depth != 0) {
        throw std::invalid_argument("unbalanced parentheses: missing ')'");
    }
    if (expectOperand) {
        throw std::invalid_argument(tokens.empty() ? "empty formula" : "operator missing its right operand");
    }
}

} // namespace

/**
 * @copydoc infixToPrefix
 *
 * @details Why reversal works: a prefix expression read right to left looks like
 * a postfix expression. So reverse the infix tokens (swapping `(` and `)` so
 * groups stay groups), run the usual infix -> postfix algorithm, and reverse the
 * postfix result.
 */
std::vector<std::string> infixToPrefix(const std::vector<std::string>& infixTokens) {
    validate(infixTokens);

    // Steps 1 and 2: reverse the tokens and swap the parentheses.
    std::vector<std::string> scan(infixTokens.rbegin(), infixTokens.rend());
    for (std::string& t : scan) {
        if (t == "(") {
            t = ")";
        } else if (t == ")") {
            t = "(";
        }
    }

    // Step 3: infix -> postfix of the reversed string, with an operator stack.
    std::vector<std::string> output;
    std::vector<std::string> stack;
    output.reserve(scan.size());
    for (const std::string& t : scan) {
        if (t == "(") {
            stack.push_back(t);
        } else if (t == ")") {
            while (!stack.empty() && stack.back() != "(") {
                output.push_back(stack.back());
                stack.pop_back();
            }
            stack.pop_back();   // discard the "(" (validate() guarantees it exists)
        } else if (isOperator(t)) {
            // Pop rule. Because the string is reversed, the usual associativity
            // test flips:
            //  - left-assoc ('*', '+'): pop only while top has HIGHER precedence (>).
            //  - right-assoc ('>', '~'): pop while top has precedence >= ours.
            // Example p>q>r reads r>q>p. On meeting the second '>' we must first
            // emit the first '>' ("r q >"), which finally reverses to "> p > q r",
            // i.e. p>(q>r). Popping only on ">" would give "r q p > >", which
            // reverses to "> > p q r" = (p>q)>r -- the wrong, left-associative parse.
            const bool right = isRightAssoc(t);
            while (!stack.empty() && stack.back() != "(" &&
                   (right ? precedence(stack.back()) >= precedence(t) : precedence(stack.back()) > precedence(t))) {
                output.push_back(stack.back());
                stack.pop_back();
            }
            stack.push_back(t);
        } else {
            output.push_back(t);   // an atom goes straight to the output
        }
    }
    while (!stack.empty()) {
        output.push_back(stack.back());
        stack.pop_back();
    }

    // Step 4: reverse the postfix of the reversed string to get the prefix.
    std::reverse(output.begin(), output.end());
    return output;
}
