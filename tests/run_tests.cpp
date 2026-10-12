/**
 * @file run_tests.cpp
 * @brief Test runner: golden files (SPEC Appendix A), invariants (SPEC 4.3-4.5),
 *        error paths, DIMACS parsing and deep-tree stress.
 *
 * Run from the repository root (`make test` does). Prints PASS/FAIL per check
 * and exits non-zero if any check fails. An exception inside a check counts as
 * a failure of that check only.
 */
#include "formula.h"
#include "gen.h"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>

namespace {

int g_pass = 0;
int g_fail = 0;

/**
 * @brief Record one check result and print a PASS/FAIL line.
 * @param name   Check label.
 * @param ok     Whether the check passed.
 * @param detail Extra text shown on failure (expected vs actual).
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(1).
 */
void report(const std::string& name, bool ok, const std::string& detail = "") {
    if (ok) {
        ++g_pass;
        std::cout << "PASS  " << name << "\n";
    } else {
        ++g_fail;
        std::cout << "FAIL  " << name;
        if (!detail.empty()) {
            std::cout << "  -- " << detail;
        }
        std::cout << "\n";
    }
}

/**
 * @brief Run `body`, turning any exception into a failed check.
 * @param name Check label.
 * @param body Callable that returns true when the check passes.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: that of `body`.
 */
void check(const std::string& name, const std::function<bool()>& body) {
    try {
        report(name, body());
    } catch (const std::exception& e) {
        report(name, false, std::string("exception: ") + e.what());
    }
}

/**
 * @brief Compare two values, printing both on mismatch.
 * @param name     Check label.
 * @param actual   Value computed by the code under test (a lambda, so a throw is caught).
 * @param expected Value from the golden file.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: that of `actual`.
 */
void checkEq(const std::string& name, const std::function<std::string()>& actual, const std::string& expected) {
    try {
        const std::string got = actual();
        report(name, got == expected, "expected [" + expected + "] got [" + got + "]");
    } catch (const std::exception& e) {
        report(name, false, std::string("exception: ") + e.what());
    }
}

/**
 * @brief Check that `body` throws std::invalid_argument.
 * @param name Check label.
 * @param body Callable expected to throw.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: that of `body`.
 */
void checkThrows(const std::string& name, const std::function<void()>& body) {
    try {
        body();
        report(name, false, "no exception thrown");
    } catch (const std::invalid_argument&) {
        report(name, true);
    } catch (const std::exception& e) {
        report(name, false, std::string("wrong exception type: ") + e.what());
    }
}

/** @brief RAII owner for a tree; swallows exceptions so a stub freeTree cannot abort the run. */
struct Tree {
    Node* root = nullptr;
    Tree() = default;
    explicit Tree(Node* r) : root(r) {}
    Tree(const Tree&) = delete;
    Tree& operator=(const Tree&) = delete;
    ~Tree() {
        try {
            freeTree(root);
        } catch (...) {
        }
    }
};

/**
 * @brief Full pipeline: infix text to tree.
 * @param infix Formula text.
 * @return Newly built tree (caller owns).
 * @throws std::invalid_argument from any stage.
 * @note Complexity: O(n) time / O(n) space.
 */
Node* parse(const std::string& infix) {
    return prefixToTree(infixToPrefix(tokenize(infix)));
}

/**
 * @brief Join strings with single spaces.
 * @param v Strings to join.
 * @return The joined string.
 * @throws Nothing.
 * @note Complexity: O(total length).
 */
std::string spaced(const std::vector<std::string>& v) {
    std::string s;
    for (size_t i = 0; i < v.size(); ++i) {
        if (i != 0) {
            s += ' ';
        }
        s += v[i];
    }
    return s;
}

/**
 * @brief Read a golden file into key -> value.
 * @param path File path.
 * @return Map of keys to values (split at the first ": ", trailing CR removed).
 * @throws std::runtime_error if the file cannot be opened.
 * @note Complexity: O(file size).
 */
std::map<std::string, std::string> loadGolden(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("cannot open " + path);
    }
    std::map<std::string, std::string> kv;
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') {
            line.pop_back();
        }
        const size_t pos = line.find(": ");
        if (pos != std::string::npos) {
            kv[line.substr(0, pos)] = line.substr(pos + 2);
        }
    }
    return kv;
}

/**
 * @brief Evaluate a tree on every row of its truth table with both evaluators.
 * @param root Tree to evaluate.
 * @return The F column, or "MISMATCH" if evaluateRec and evaluateIter ever disagree.
 * @throws std::exception from the evaluators.
 * @note Complexity: O(2^k * n) time.
 */
std::string columnByBothEvaluators(const Node* root) {
    const std::vector<std::string> atoms = collectAtoms(root);
    const size_t k = atoms.size();
    std::string col;
    for (size_t i = 0; i < (size_t{1} << k); ++i) {
        Assignment a;
        for (size_t j = 0; j < k; ++j) {
            a[atoms[j]] = ((i >> (k - 1 - j)) & 1u) != 0;
        }
        const bool r = evaluateRec(root, a);
        const bool it = evaluateIter(root, a);
        if (r != it) {
            return "MISMATCH";
        }
        col += r ? '1' : '0';
    }
    return col;
}

/**
 * @brief Run every check for one golden file.
 * @param path Golden file path.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(2^k * n) for the formula in the file.
 */
void runGolden(const std::string& path) {
    const std::string name = std::filesystem::path(path).stem().string();
    std::map<std::string, std::string> g;
    try {
        g = loadGolden(path);
    } catch (const std::exception& e) {
        report("golden " + name + " loads", false, e.what());
        return;
    }
    const std::string in = g["input"];
    const std::string pre = "golden " + name + ": ";

    checkEq(pre + "prefix", [&] { return joinTokens(infixToPrefix(tokenize(in))); }, g["prefix"]);

    Tree t;
    try {
        t.root = parse(in);
    } catch (const std::exception& e) {
        report(pre + "build tree", false, e.what());
        return;
    }
    const Node* r = t.root;

    checkEq(pre + "inorderRec", [&] { return inorderRec(r); }, g["inorder"]);
    checkEq(pre + "inorderIter", [&] { return inorderIter(r); }, g["inorder"]);
    checkEq(pre + "preorderRec", [&] { return preorderRec(r); }, g["preorder"]);
    checkEq(pre + "preorderIter", [&] { return preorderIter(r); }, g["preorder"]);
    checkEq(pre + "postorderRec", [&] { return postorderRec(r); }, g["postorder"]);
    checkEq(pre + "postorderIter", [&] { return postorderIter(r); }, g["postorder"]);
    checkEq(pre + "heightRec", [&] { return std::to_string(heightRec(r)); }, g["height"]);
    checkEq(pre + "heightIter", [&] { return std::to_string(heightIter(r)); }, g["height"]);
    checkEq(pre + "atoms", [&] { return spaced(collectAtoms(r)); }, g["atoms"]);
    checkEq(pre + "truthTableColumn", [&] { return truthTableColumn(r); }, g["table"]);
    checkEq(pre + "evaluateRec == evaluateIter on all rows", [&] { return columnByBothEvaluators(r); }, g["table"]);
    // Invariants of SPEC 4.3.
    checkEq(pre + "preorder == prefix", [&] { return preorderRec(r); }, g["prefix"]);
    check(pre + "inorder(canonical) re-parses to the same tree", [&] {
        Tree again(parse(inorderRec(r)));
        return inorderRec(again.root) == inorderRec(r) && preorderRec(again.root) == preorderRec(r);
    });
}

/** @brief Tokenizer, infix -> prefix and tree-building unit checks. */
void runPipelineUnits() {
    check("tokenize keeps multi-character atoms", [] {
        const std::vector<std::string> expected = {"x12", "+", "~", "y_3", ">", "(", "p", ")"};
        return tokenize("x12 + ~y_3>(p)") == expected;
    });
    checkThrows("tokenize rejects illegal character", [] { tokenize("p&q"); });
    checkThrows("tokenize rejects a digit-initial atom", [] { tokenize("1p"); });
    check("joinTokens", [] {
        return joinTokens({"a", "b", "c"}) == "a b c" && joinTokens({"a", "b"}, "") == "ab" && joinTokens({}) == "";
    });

    // Precedence and associativity beyond Appendix A (SPEC 2.3).
    const std::vector<std::pair<std::string, std::string>> cases = {
        {"p+q*r", "+ p * q r"},       {"p+q+r", "+ + p q r"},     {"p*q*r", "* * p q r"},
        {"p>q>r", "> p > q r"},       {"p>q+r", "> p + q r"},     {"p*~q", "* p ~ q"},
        {"~p*q", "* ~ p q"},          {"~~p", "~ ~ p"},           {"~(p+q)", "~ + p q"},
        {"(p>q)>r", "> > p q r"},     {"p+q>r", "> + p q r"},     {"((p))", "p"},
        {"p*q+r*s", "+ * p q * r s"}, {"p+q*r+s", "+ + p * q r s"},
    };
    for (const auto& c : cases) {
        checkEq("infixToPrefix \"" + c.first + "\"", [&] { return joinTokens(infixToPrefix(tokenize(c.first))); }, c.second);
    }
    checkThrows("infixToPrefix rejects missing close paren", [] { infixToPrefix(tokenize("(p+q")); });
    checkThrows("infixToPrefix rejects extra close paren", [] { infixToPrefix(tokenize("p+q)")); });
    checkThrows("infixToPrefix rejects operator missing operand", [] { infixToPrefix(tokenize("p+")); });
    checkThrows("infixToPrefix rejects leading binary operator", [] { infixToPrefix(tokenize("*p")); });
    checkThrows("infixToPrefix rejects adjacent operands", [] { infixToPrefix(tokenize("p q")); });
    checkThrows("infixToPrefix rejects empty input", [] { infixToPrefix(tokenize("")); });

    check("tree invariant: NOT keeps its operand in right", [] {
        Tree t(prefixToTree({"~", "p"}));
        return t.root->type == NodeType::NOT && t.root->left == nullptr && t.root->right != nullptr &&
               t.root->right->type == NodeType::ATOM && t.root->right->value == "p";
    });
    check("tree invariant: binary node children and values", [] {
        Tree t(prefixToTree({">", "p", "q"}));
        return t.root->type == NodeType::IMPLIES && t.root->value == ">" && t.root->left->value == "p" &&
               t.root->right->value == "q";
    });
    check("tree invariant: atom is a leaf", [] {
        Tree t(prefixToTree({"x7"}));
        return t.root->type == NodeType::ATOM && t.root->left == nullptr && t.root->right == nullptr;
    });
    checkThrows("prefixToTree rejects empty input", [] { prefixToTree({}); });
    checkThrows("prefixToTree rejects missing operand", [] { prefixToTree({"+", "p"}); });
    checkThrows("prefixToTree rejects leftover nodes", [] { prefixToTree({"p", "q"}); });
    checkThrows("prefixToTree rejects extra operand", [] { prefixToTree({"+", "p", "q", "r"}); });
    checkThrows("prefixToTree rejects a parenthesis token", [] { prefixToTree({"(", "p"}); });
    check("freeTree accepts nullptr", [] {
        freeTree(nullptr);
        return true;
    });
}

/** @brief Evaluation and truth-table unit checks. */
void runEvalUnits() {
    check("evaluate: implication semantics", [] {
        Tree t(parse("(p>q)"));
        Assignment a;
        bool ok = true;
        for (int p = 0; p < 2; ++p) {
            for (int q = 0; q < 2; ++q) {
                a["p"] = p != 0;
                a["q"] = q != 0;
                const bool expected = (p == 0) || (q != 0);
                ok = ok && evaluateRec(t.root, a) == expected && evaluateIter(t.root, a) == expected;
            }
        }
        return ok;
    });
    checkThrows("evaluateRec rejects a missing atom", [] {
        Tree t(parse("(p*q)"));
        evaluateRec(t.root, Assignment{{"p", true}});
    });
    checkThrows("evaluateIter rejects a missing atom", [] {
        Tree t(parse("(p*q)"));
        evaluateIter(t.root, Assignment{{"p", true}});
    });
    check("collectAtoms natural sort: x < x1 < x2 < x10 < y", [] {
        Tree t(parse("((y+x10)*((x2+x)+x1))"));
        const std::vector<std::string> expected = {"x", "x1", "x2", "x10", "y"};
        return collectAtoms(t.root) == expected;
    });
    check("collectAtoms: leading zeros, then plain lexicographic tie-break", [] {
        Tree t(parse("((x01+x1)+x2)"));
        const std::vector<std::string> expected = {"x01", "x1", "x2"};
        return collectAtoms(t.root) == expected;
    });
    check("collectAtoms: distinct only", [] {
        Tree t(parse("((p+p)*(p+q))"));
        return collectAtoms(t.root).size() == 2;
    });
    checkEq("printTruthTable layout", [] {
        Tree t(parse("(p*q)"));
        std::ostringstream os;
        printTruthTable(t.root, os);
        return os.str();
    }, "p q | F\n0 0 | 0\n0 1 | 0\n1 0 | 0\n1 1 | 1\n");
    check("printTruthTable refuses k > maxAtoms", [] {
        Tree t(parse("((p*q)+r)"));
        std::ostringstream os;
        printTruthTable(t.root, os, 2);
        return os.str().find("Truth table skipped: k=3 atoms, 2^3 rows (limit 2)") != std::string::npos &&
               truthTableColumn(t.root, 2).empty();
    });
    checkEq("truthTableColumn at exactly maxAtoms", [] {
        Tree t(parse("(p+q)"));
        return truthTableColumn(t.root, 2);
    }, "0111");
}

/**
 * @brief Write `text` to a scratch .cnf file, run `body` with its path, and remove the file.
 * @param text Contents of the file.
 * @param body Callable that receives the path.
 * @return Nothing.
 * @throws Whatever `body` throws (the file is removed first).
 * @note Complexity: O(size of text).
 */
void withCnfFile(const std::string& text, const std::function<void(const std::string&)>& body) {
    const std::string path = "tests/data/_scratch.cnf";
    {
        std::ofstream out(path);
        out << text;
    }
    try {
        body(path);
    } catch (...) {
        std::remove(path.c_str());
        throw;
    }
    std::remove(path.c_str());
}

/** @brief DIMACS reader and CNF conversion checks (SPEC 4.6). */
void runCnfUnits() {
    check("readDimacs tiny.cnf", [] {
        const CNF c = readDimacs("tests/data/tiny.cnf");
        return c.numVars == 3 && c.numClauses == 2 && c.clauses.size() == 2 &&
               c.clauses[0] == std::vector<int>({1, -2}) && c.clauses[1] == std::vector<int>({2, 3});
    });
    checkEq("tiny.cnf LEFT_DEEP equals golden E input", [] {
        return cnfToInfix(readDimacs("tests/data/tiny.cnf"), TreeShape::LEFT_DEEP);
    }, loadGolden("tests/golden/E.txt")["input"]);
    checkEq("tiny.cnf through the full pipeline gives golden E table", [] {
        Tree t(parse(cnfToInfix(readDimacs("tests/data/tiny.cnf"), TreeShape::LEFT_DEEP)));
        return truthTableColumn(t.root);
    }, loadGolden("tests/golden/E.txt")["table"]);
    check("readDimacs: comments, multi-line clause, shared line, % terminator", [] {
        bool ok = false;
        withCnfFile("c hello\np cnf 3 3\n1 -2\n0 2 3 0 -1 0\n%\n0\n", [&](const std::string& p) {
            const CNF c = readDimacs(p);
            ok = c.numClauses == 3 && c.clauses.size() == 3 && c.clauses[0] == std::vector<int>({1, -2}) &&
                 c.clauses[1] == std::vector<int>({2, 3}) && c.clauses[2] == std::vector<int>({-1});
        });
        return ok;
    });
    checkEq("single-literal clause is the bare literal", [] {
        std::string s;
        withCnfFile("p cnf 2 2\n-1 0\n2 0\n", [&](const std::string& p) {
            s = cnfToInfix(readDimacs(p), TreeShape::LEFT_DEEP);
        });
        return s;
    }, "((~x1)*x2)");
    check("clause count mismatch warns but continues", [] {
        bool ok = false;
        withCnfFile("p cnf 2 5\n1 0\n", [&](const std::string& p) {
            const CNF c = readDimacs(p);
            ok = c.numClauses == 1 && c.clauses.size() == 1;
        });
        return ok;
    });
    checkThrows("readDimacs rejects a missing file", [] { readDimacs("tests/data/does_not_exist.cnf"); });
    checkThrows("readDimacs rejects a missing header", [] {
        withCnfFile("1 2 0\n", [](const std::string& p) { readDimacs(p); });
    });
    checkThrows("readDimacs rejects an empty clause", [] {
        withCnfFile("p cnf 2 1\n0\n", [](const std::string& p) { readDimacs(p); });
    });
    checkThrows("readDimacs rejects a non-integer token", [] {
        withCnfFile("p cnf 2 1\n1 a 0\n", [](const std::string& p) { readDimacs(p); });
    });
    checkThrows("cnfToInfix rejects an empty formula", [] { cnfToInfix(CNF{}, TreeShape::LEFT_DEEP); });
    checkThrows("cnfToInfix rejects an empty clause", [] {
        CNF c;
        c.numClauses = 1;
        c.clauses = {{}};
        cnfToInfix(c, TreeShape::BALANCED);
    });
    check("LEFT_DEEP vs BALANCED heights for 4 unit clauses", [] {
        bool ok = false;
        withCnfFile("p cnf 4 4\n1 0\n2 0\n3 0\n4 0\n", [&](const std::string& p) {
            const CNF c = readDimacs(p);
            Tree l(parse(cnfToInfix(c, TreeShape::LEFT_DEEP)));
            Tree b(parse(cnfToInfix(c, TreeShape::BALANCED)));
            ok = heightRec(l.root) == 3 && heightRec(b.root) == 2 && inorderRec(b.root) == "((x1*x2)*(x3*x4))";
        });
        return ok;
    });
    check("BALANCED and LEFT_DEEP are logically equivalent", [] {
        bool ok = false;
        withCnfFile("p cnf 4 5\n1 -2 3 0\n-1 4 0\n2 0\n-3 -4 0\n1 2 3 4 0\n", [&](const std::string& p) {
            const CNF c = readDimacs(p);
            Tree l(parse(cnfToInfix(c, TreeShape::LEFT_DEEP)));
            Tree b(parse(cnfToInfix(c, TreeShape::BALANCED)));
            ok = truthTableColumn(l.root) == truthTableColumn(b.root) && collectAtoms(l.root).size() == 4;
        });
        return ok;
    });
    check("BALANCED height is logarithmic in the clause count", [] {
        CNF c;
        c.numVars = 1000;
        for (int i = 1; i <= 1000; ++i) {
            c.clauses.push_back({i});
        }
        c.numClauses = 1000;
        Tree b(parse(cnfToInfix(c, TreeShape::BALANCED)));
        const int h = heightIter(b.root);
        return h >= 9 && h <= 11;   // ceil(log2 1000) = 10
    });
}

/** @brief Deep (left-deep) trees: iterative code must survive, freeTree must not recurse. */
void runDeepTreeStress() {
    const long leaves = 100000;   // height 99999: far beyond a safe recursion depth
    const std::string infix = leftDeepChain(leaves, '+');
    check("deep chain: infix -> prefix is iterative and correct", [&] {
        const std::vector<std::string> p = infixToPrefix(tokenize(infix));
        if (p.size() != static_cast<size_t>(2 * leaves - 1)) {
            return false;
        }
        for (long i = 0; i < leaves - 1; ++i) {
            if (p[static_cast<size_t>(i)] != "+") {
                return false;
            }
        }
        return p[static_cast<size_t>(leaves - 1)] == "x1" && p.back() == "x" + std::to_string(leaves);
    });
    Tree t;
    check("deep chain: prefixToTree", [&] {
        t.root = parse(infix);
        return t.root != nullptr;
    });
    if (t.root == nullptr) {
        return;
    }
    check("deep chain: heightIter", [&] { return heightIter(t.root) == leaves - 1; });
    check("deep chain: inorderIter reproduces the input", [&] { return inorderIter(t.root) == infix; });
    check("deep chain: preorderIter equals prefix", [&] {
        return preorderIter(t.root) == joinTokens(infixToPrefix(tokenize(infix)));
    });
    check("deep chain: postorderIter ends with the root operator", [&] {
        const std::string s = postorderIter(t.root);
        return s.size() > 2 && s.compare(s.size() - 2, 2, " +") == 0 && s.compare(0, 6, "x1 x2 ") == 0;
    });
    check("deep chain: evaluateIter, all atoms false then one true", [&] {
        Assignment a;
        for (long i = 1; i <= leaves; ++i) {
            a["x" + std::to_string(i)] = false;
        }
        const bool allFalse = evaluateIter(t.root, a);
        a["x" + std::to_string(leaves / 2)] = true;
        return !allFalse && evaluateIter(t.root, a);
    });
    check("deep chain: collectAtoms is sorted and complete", [&] {
        const std::vector<std::string> atoms = collectAtoms(t.root);
        return atoms.size() == static_cast<size_t>(leaves) && atoms.front() == "x1" && atoms[1] == "x2" &&
               atoms[9] == "x10" && atoms.back() == "x" + std::to_string(leaves);
    });
    // freeTree runs in Tree's destructor; reaching the end of the run proves it did not overflow the stack.
}

/**
 * @brief Property test (TODO 3.3): random canonical formulas must survive the whole pipeline.
 * @return Nothing.
 * @throws Nothing.
 * @note Complexity: O(sum of formula sizes).
 */
void runProperties() {
    std::mt19937_64 rng(20261012);
    int bad = 0;
    std::string firstBad;
    const int trials = 1000;
    for (int i = 0; i < trials; ++i) {
        const long nodes = 1 + static_cast<long>(rng() % 60);
        const int atoms = 1 + static_cast<int>(rng() % 6);
        const std::string s = randomFormula(rng, nodes, atoms);
        bool ok = false;
        try {
            const std::vector<std::string> prefix = infixToPrefix(tokenize(s));
            Tree t(prefixToTree(prefix));
            const Node* r = t.root;
            ok = inorderRec(r) == s && inorderIter(r) == s && preorderRec(r) == preorderIter(r) &&
                 preorderRec(r) == joinTokens(prefix) && postorderRec(r) == postorderIter(r) &&
                 heightRec(r) == heightIter(r) && columnByBothEvaluators(r) == truthTableColumn(r);
        } catch (const std::exception&) {
            ok = false;
        }
        if (!ok) {
            ++bad;
            if (firstBad.empty()) {
                firstBad = s;
            }
        }
    }
    report("property: 1000 random formulas (round trip, rec == iter everywhere)", bad == 0,
           std::to_string(bad) + " failures, first: " + firstBad);
}

} // namespace

int main(int argc, char** argv) {
    const std::string goldenDir = argc > 1 ? argv[1] : "tests/golden";

    std::vector<std::string> files;
    if (std::filesystem::is_directory(goldenDir)) {
        for (const auto& e : std::filesystem::directory_iterator(goldenDir)) {
            if (e.path().extension() == ".txt") {
                files.push_back(e.path().string());
            }
        }
    }
    std::sort(files.begin(), files.end());
    report("golden directory has files", !files.empty(), goldenDir);
    for (const std::string& f : files) {
        runGolden(f);
    }
    runPipelineUnits();
    runEvalUnits();
    runCnfUnits();
    runDeepTreeStress();
    runProperties();

    std::cout << "\n" << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
