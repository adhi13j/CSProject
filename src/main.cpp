/**
 * @file main.cpp
 * @brief Command-line interface `logic_a1` (SPEC section 5).
 *
 * Usage:
 *   logic_a1 --formula "<infix>" [--task all|1|2|3|4|5] [--assign "p=1,q=0"] [--table]
 *   logic_a1 --cnf <file.cnf>    [--shape left|balanced] [--task ...] [--assign ...] [--table]
 *   logic_a1 --help
 *
 * Errors are written as `Error: <message>` to stderr with exit code 1.
 *
 * @authors Replace with group member names.
 */
#include "formula.h"

#include <iostream>
#include <stdexcept>

namespace {

/** @brief Recursive variants are skipped above this height (SPEC 5): a left-deep CNF tree would overflow the stack. */
constexpr int kRecursionLimit = 10000;

/** @brief Parsed command line. */
struct Options {
    std::string formula;
    std::string cnfPath;
    bool haveFormula = false;
    bool haveCnf = false;
    TreeShape shape = TreeShape::LEFT_DEEP;
    std::string task = "all";
    std::string assign;
    bool haveAssign = false;
    bool table = false;
    bool help = false;
};

/** @brief RAII owner of a tree, so every exit path releases it with the iterative freeTree. */
struct TreeOwner {
    Node* root = nullptr;
    TreeOwner() = default;
    TreeOwner(const TreeOwner&) = delete;
    TreeOwner& operator=(const TreeOwner&) = delete;
    ~TreeOwner() { freeTree(root); }
};

/**
 * @brief Print the usage text.
 * @param out Destination stream.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(1).
 */
void printUsage(std::ostream& out) {
    out << "Usage:\n"
           "  logic_a1 --formula \"<infix>\" [--task all|1|2|3|4|5] [--assign \"p=1,q=0\"] [--table]\n"
           "  logic_a1 --cnf <file.cnf>    [--shape left|balanced] [--task ...] [--assign ...] [--table]\n"
           "  logic_a1 --help\n"
           "\n"
           "Connectives: ~ not, * and, + or, > implies.   Atoms: [A-Za-z][A-Za-z0-9_]*\n"
           "Tasks: 1 tokens+prefix, 2 tree, 3 traversals, 4 height, 5 evaluation / truth table.\n";
}

/**
 * @brief Parse the command line.
 * @param argc Argument count.
 * @param argv Arguments.
 * @return The parsed options.
 * @throws std::invalid_argument on an unknown flag, a missing value, a bad value, or a wrong flag combination.
 * @note Complexity: O(argc) time / O(1) space.
 */
Options parseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        auto value = [&]() -> std::string {
            if (i + 1 >= argc) {
                throw std::invalid_argument("missing value after " + a);
            }
            return argv[++i];
        };
        if (a == "--formula") {
            o.formula = value();
            o.haveFormula = true;
        } else if (a == "--cnf") {
            o.cnfPath = value();
            o.haveCnf = true;
        } else if (a == "--shape") {
            const std::string s = value();
            if (s == "left") {
                o.shape = TreeShape::LEFT_DEEP;
            } else if (s == "balanced") {
                o.shape = TreeShape::BALANCED;
            } else {
                throw std::invalid_argument("--shape must be 'left' or 'balanced', got '" + s + "'");
            }
        } else if (a == "--task") {
            o.task = value();
            if (o.task != "all" && (o.task.size() != 1 || o.task[0] < '1' || o.task[0] > '5')) {
                throw std::invalid_argument("--task must be all, 1, 2, 3, 4 or 5, got '" + o.task + "'");
            }
        } else if (a == "--assign") {
            o.assign = value();
            o.haveAssign = true;
        } else if (a == "--table") {
            o.table = true;
        } else if (a == "--help" || a == "-h") {
            o.help = true;
        } else {
            throw std::invalid_argument("unknown argument '" + a + "' (try --help)");
        }
    }
    if (!o.help) {
        if (o.haveFormula && o.haveCnf) {
            throw std::invalid_argument("--formula and --cnf are mutually exclusive");
        }
        if (!o.haveFormula && !o.haveCnf) {
            throw std::invalid_argument("give either --formula or --cnf (try --help)");
        }
    }
    return o;
}

/**
 * @brief Parse `p=1,q=0` into an Assignment.
 * @param text The `--assign` argument.
 * @return The assignment.
 * @throws std::invalid_argument on bad syntax or a value other than 0/1.
 * @note Complexity: O(|text|) time / O(|text|) space.
 */
Assignment parseAssignment(const std::string& text) {
    Assignment a;
    size_t pos = 0;
    while (pos <= text.size()) {
        size_t comma = text.find(',', pos);
        if (comma == std::string::npos) {
            comma = text.size();
        }
        std::string item = text.substr(pos, comma - pos);
        std::string compact;
        for (char c : item) {
            if (c != ' ' && c != '\t') {
                compact += c;
            }
        }
        const size_t eq = compact.find('=');
        if (eq == std::string::npos || eq == 0 || eq + 2 != compact.size() ||
            (compact[eq + 1] != '0' && compact[eq + 1] != '1')) {
            throw std::invalid_argument("bad --assign item '" + item + "' (expected atom=0 or atom=1)");
        }
        a[compact.substr(0, eq)] = compact[eq + 1] == '1';
        pos = comma + 1;
    }
    return a;
}

/**
 * @brief Count the nodes of a tree iteratively.
 * @param root Tree root (may be nullptr).
 * @return Number of nodes.
 * @throws Nothing.
 * @note Complexity: O(n) time / O(h) space.
 */
size_t countNodes(const Node* root) {
    size_t count = 0;
    std::vector<const Node*> stack;
    if (root != nullptr) {
        stack.push_back(root);
    }
    while (!stack.empty()) {
        const Node* n = stack.back();
        stack.pop_back();
        ++count;
        if (n->left != nullptr) {
            stack.push_back(n->left);
        }
        if (n->right != nullptr) {
            stack.push_back(n->right);
        }
    }
    return count;
}

/**
 * @brief Shorten a long string for display: `<first 100>…<last 100> (N chars)` above 200 characters.
 * @param s        The full text.
 * @param truncate Whether truncation applies (it does in --cnf mode).
 * @return The text to print.
 * @throws Nothing.
 * @note Complexity: O(1) beyond the copy / O(min(|s|, 200)) space.
 */
std::string display(const std::string& s, bool truncate) {
    if (!truncate || s.size() <= 200) {
        return s;
    }
    return s.substr(0, 100) + "…" + s.substr(s.size() - 100) + " (" + std::to_string(s.size()) + " chars)";
}

/**
 * @brief Print `LABEL (rec): ...` and `LABEL (iter): ...` for a function pair.
 * @param label     Section label, e.g. "INORDER".
 * @param rec       Recursive variant.
 * @param iter      Iterative variant.
 * @param tallRoot  True if the tree is too tall for the recursive variant.
 * @param height    Tree height, shown in the SKIPPED message.
 * @param truncate  Whether long strings are shortened.
 * @return Nothing.
 * @throws Whatever `iter` throws.
 * @note Complexity: that of the two variants.
 */
void printPair(const std::string& label, std::string (*rec)(const Node*), std::string (*iter)(const Node*),
               const Node* root, bool tallRoot, int height, bool truncate) {
    if (tallRoot) {
        std::cout << label << " (rec): SKIPPED (height " << height << " > " << kRecursionLimit << ")\n";
    } else {
        std::cout << label << " (rec): " << display(rec(root), truncate) << "\n";
    }
    std::cout << label << " (iter): " << display(iter(root), truncate) << "\n";
}

/**
 * @brief Run the whole pipeline for one formula and print the requested sections.
 * @param o     Parsed options.
 * @param infix The infix formula (typed by the user, or generated from the CNF).
 * @return Nothing.
 * @throws std::invalid_argument from any stage of the pipeline.
 * @note Complexity: O(n) for tasks 1-4; O(2^k * n) with --table.
 */
void run(const Options& o, const std::string& infix) {
    const bool all = o.task == "all";
    const bool truncate = o.haveCnf;
    const auto wants = [&](char t) { return all || o.task[0] == t; };

    const std::vector<std::string> tokens = tokenize(infix);
    const std::vector<std::string> prefix = infixToPrefix(tokens);
    TreeOwner tree;
    tree.root = prefixToTree(prefix);
    const Node* root = tree.root;

    if (wants('1')) {
        std::cout << "TOKENS: " << display(joinTokens(tokens), truncate) << "\n";
        std::cout << "PREFIX: " << display(joinTokens(prefix), truncate) << "\n";
    }
    if (wants('2')) {
        std::cout << "TREE: " << countNodes(root) << " nodes\n";
    }
    const int h = heightIter(root);   // iterative, so safe to call before deciding about recursion
    const bool tall = h > kRecursionLimit;
    if (wants('3')) {
        printPair("INORDER", inorderRec, inorderIter, root, tall, h, truncate);
        printPair("PREORDER", preorderRec, preorderIter, root, tall, h, truncate);
        printPair("POSTORDER", postorderRec, postorderIter, root, tall, h, truncate);
    }
    if (wants('4')) {
        if (tall) {
            std::cout << "HEIGHT (rec): SKIPPED (height " << h << " > " << kRecursionLimit << ")\n";
        } else {
            std::cout << "HEIGHT (rec): " << heightRec(root) << "\n";
        }
        std::cout << "HEIGHT (iter): " << h << "\n";
    }
    if (wants('5')) {
        if (o.haveAssign) {
            const Assignment a = parseAssignment(o.assign);
            if (tall) {
                std::cout << "EVAL (rec): SKIPPED (height " << h << " > " << kRecursionLimit << ")\n";
            } else {
                std::cout << "EVAL (rec): " << (evaluateRec(root, a) ? 1 : 0) << "\n";
            }
            std::cout << "EVAL (iter): " << (evaluateIter(root, a) ? 1 : 0) << "\n";
        }
        if (o.table) {
            std::cout << "TRUTH TABLE\n";
            printTruthTable(root, std::cout);
        }
        if (!o.haveAssign && !o.table && !all) {
            std::cout << "EVAL: give --assign \"p=1,q=0\" and/or --table\n";
        }
    }
}

} // namespace

/**
 * @brief Program entry point.
 * @param argc Argument count.
 * @param argv Arguments.
 * @return 0 on success, 1 on any error.
 * @throws Nothing (all exceptions are caught and reported).
 * @note Complexity: see run().
 */
int main(int argc, char** argv) {
    try {
        const Options o = parseArgs(argc, argv);
        if (o.help) {
            printUsage(std::cout);
            return 0;
        }
        const std::string infix = o.haveFormula ? o.formula : cnfToInfix(readDimacs(o.cnfPath), o.shape);
        run(o, infix);
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
}
