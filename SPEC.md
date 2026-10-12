# SPEC.md — CS F214 Logic in CS, Assignment 1

**Status:** v1.0 — the answers assumed in §12 apply (and are encoded in Appendix A) until the group confirms or changes them.
**Assignment:** CS F214 Assignment 1 PDF — 30 marks, 10% weightage, groups of at most 5, Part A in C or C++ only.
**Source of truth:** If code and this spec disagree, the code is wrong. Changes to this spec are made by the group, committed separately, and announced.

---

## 1. Scope

| Part | Task | Marks | Section |
|---|---|---|---|
| A | T1 Infix → prefix | 2 | §4.1 |
| A | T2 Prefix → parse tree | 3 | §4.2 |
| A | T3 Traversals (in/pre/post × recursive/iterative) | 3 | §4.3 |
| A | T4 Height (recursive + iterative) | 2 | §4.4 |
| A | T5 Evaluation + truth table | 3 | §4.5 |
| A | T6 Analysis, Doxygen, HTML report | 5 | §8, §9 |
| B | T1 Lean proofs of Examples 1.13–1.18 | 5 | §10 |
| B | T2 Live sequent proof at demo | 7 | §10 |

Total: 18 (A) + 12 (B) = 30. The demo is mandatory: absence means zero credit (assignment PDF, instruction 6).

Language for Part A: **C++17**, standard library only. Must compile with `g++ -std=c++17 -Wall -Wextra -O2` with zero warnings.

---

## 2. Input language

### 2.1 Tokens

| Token | Lexeme | Meaning |
|---|---|---|
| ATOM | `[A-Za-z][A-Za-z0-9_]*` | propositional atom (multi-character allowed: `p`, `x1`, `x23`) |
| NOT | `~` | negation (unary, prefix) |
| AND | `*` | conjunction |
| OR | `+` | disjunction |
| IMPLIES | `>` | implication |
| LPAREN / RPAREN | `(` `)` | grouping |

Whitespace is ignored everywhere. Any other character is an error (§7).

**Rule:** every algorithm operates on a `std::vector<std::string>` of tokens, never on raw characters. (Reversing characters would turn `x12` into `21x`.)

### 2.2 Grammar (EBNF)

```
formula  ::= ATOM
           | "~" formula
           | "(" formula ")"
           | formula BINOP formula        (disambiguated by §2.3)
BINOP    ::= "*" | "+" | ">"
```

The assignment guarantees input is well-formed and fully parenthesized. We still implement precedence (§2.3) so that the code is correct on less-parenthesized input — this is a robustness bonus, not a requirement.

### 2.3 Precedence and associativity

| Operator | Precedence (higher binds tighter) | Associativity |
|---|---|---|
| `~` | 4 | right (unary) |
| `*` | 3 | left |
| `+` | 2 | left |
| `>` | 1 | **right** |

So `p>q>r` means `p>(q>r)`, and `~p+q` means `(~p)+q`.

---

## 3. Data structures

### 3.1 Node

```cpp
enum class NodeType { ATOM, NOT, AND, OR, IMPLIES };

struct Node {
    NodeType    type;
    std::string value;          // atom name, or the operator symbol "~","*","+",">"
    Node*       left  = nullptr;
    Node*       right = nullptr;
};
```

**Invariants**

- ATOM: `left == nullptr && right == nullptr`.
- NOT: `left == nullptr`, operand stored in **`right`**. (This makes in-order traversal print `~` before its operand with no special case.)
- AND / OR / IMPLIES: both children non-null; `left` is the left operand.

### 3.2 Ownership

- `prefixToTree` allocates with `new`; the caller owns the tree.
- `freeTree(Node*)` releases it and **must be iterative** (a recursive free overflows the stack on deep CNF trees).
- No destructor recursion in `Node` (no `~Node()` that deletes children).

---

## 4. Function contracts

All declarations live in `include/formula.h`. Every function gets a Doxygen block (§9). "n" = number of tokens / nodes, "h" = tree height, "k" = number of distinct atoms.

### 4.1 Task 1 — Tokenize and infix → prefix

```cpp
std::vector<std::string> tokenize(const std::string& infix);
std::vector<std::string> infixToPrefix(const std::vector<std::string>& infixTokens);
std::string joinTokens(const std::vector<std::string>& tokens, const std::string& sep = " ");
```

**Algorithm (required, matches course reference — http://scanftree.com/Data_Structure/infix-to-prefix):**

1. Reverse the token list.
2. Swap every `(` with `)` and vice versa.
3. Run infix → postfix with an operator stack, using this pop rule for an incoming operator `o`:
   - `o` left-associative (`*`, `+`): pop while top is an operator with precedence **>** prec(`o`).
   - `o` right-associative (`>`, `~`): pop while top is an operator with precedence **≥** prec(`o`).
   (This is flipped relative to normal infix→postfix because the string is reversed. Getting it wrong makes `p>q>r` parse as `(p>q)>r`.)
4. Reverse the output → prefix.

**Output:** prefix token list, printed space-separated, e.g. `> + p ~ q r`.
**Complexity:** O(n) time, O(n) space.

### 4.2 Task 2 — Prefix → parse tree

```cpp
Node* prefixToTree(const std::vector<std::string>& prefixTokens);
void  freeTree(Node* root);   // iterative
```

**Algorithm:** scan tokens **right to left** with a stack of `Node*`.

- ATOM → push a leaf.
- `~` → pop 1 → becomes `right`; push.
- binary op → pop first → `left`, pop second → `right`; push.
- At the end the stack holds exactly 1 node: the root. Otherwise → error (§7).

**Complexity:** O(n) time, O(n) space.

### 4.3 Task 3 — Traversals (6 functions)

```cpp
std::string inorderRec   (const Node* root);
std::string inorderIter  (const Node* root);
std::string preorderRec  (const Node* root);
std::string preorderIter (const Node* root);
std::string postorderRec (const Node* root);
std::string postorderIter(const Node* root);
```

**Output formats**

- **In-order** returns the *canonical infix* string, no spaces:
  - ATOM → `p`
  - NOT → `(~X)`
  - binary → `(L op R)`
- **Pre-order / post-order** return tokens separated by single spaces, no leading or trailing space.

**Iterative algorithms (required approach)**

- In-order: explicit stack of `(node, state)` frames, state ∈ {OPEN, MIDDLE, CLOSE}. OPEN emits `(` (binary/NOT) and pushes the node as MIDDLE followed by its left child (binary only); MIDDLE emits the operator and pushes CLOSE followed by the right child; CLOSE emits `)`. ATOM emits its name. A plain push-left-chain loop is *not* used: a NOT node has `left == nullptr` and its operand in `right`, so it needs the frame form.
- Pre-order: stack; pop+visit, push right, push left.
- Post-order: two stacks, or one stack + last-visited pointer (document which).

**Invariants (tested):**

- `xxxRec(t) == xxxIter(t)` for all three.
- `preorder(t) == joinTokens(infixToPrefix(tokenize(s)))`.
- `inorder(t) == canonical(s)`. For fully parenthesized input with `(~X)` negations, `canonical(s)` is `s` with whitespace removed.

**Complexity:** O(n) time. Recursive: O(h) call stack. Iterative: O(h) heap stack.
The recursive versions must append to one shared output buffer through a private helper (the public signature is unchanged). Concatenating returned `std::string`s at every level would cost O(n·h), i.e. O(n²) on a left-deep tree.

### 4.4 Task 4 — Height

```cpp
int heightRec (const Node* root);
int heightIter(const Node* root);   // level-order BFS with std::queue
```

**Convention:** `height(nullptr) = -1`, `height(leaf) = 0`, `height(n) = 1 + max(height(left), height(right))`.
`heightIter` counts BFS levels and returns `levels - 1`.

**Complexity:** O(n) time. Recursive: O(h) stack. Iterative: O(w) queue, where w = max level width.

### 4.5 Task 5 — Evaluation and truth table

```cpp
using Assignment = std::unordered_map<std::string, bool>;

std::vector<std::string> collectAtoms(const Node* root);          // distinct, natural-sorted
bool evaluateRec (const Node* root, const Assignment& a);         // recursive post-order
bool evaluateIter(const Node* root, const Assignment& a);         // iterative post-order (bottom-up)
void printTruthTable(const Node* root, std::ostream& out, int maxAtoms = 20);
std::string truthTableColumn(const Node* root, int maxAtoms = 20); // results only, e.g. "01110101"
```

**Semantics:** `~`, `*`, `+` standard. `a > b` ≡ `(!a) || b`.

**Natural sort for atoms:** split each name into a prefix and a maximal trailing run of digits (possibly empty). Compare the prefixes (case-sensitive, by `char` value); then the digit runs as unsigned integers (an empty run sorts before any number; compare by length after stripping leading zeros, then lexicographically, so there is no overflow); then the whole names lexicographically. So `x < x1 < x2 < x10`, and `x01` sorts after `x1`.

**Truth table**

- k = `collectAtoms(root).size()`. If k > `maxAtoms` → write `Truth table skipped: k=<k> atoms, 2^<k> rows (limit <maxAtoms>)` to `out` (§8.4 explains why) and return. `truthTableColumn` returns an empty string in that case.
- Row i, for i = 0 … 2^k − 1: atom j (0-based in sorted order) has value `(i >> (k-1-j)) & 1`. So the **first atom is the most significant bit** and row 0 is all-false.
- Output format, columns separated by a single space:

```
p q r | F
0 0 0 | 0
0 0 1 | 1
...
```

- `truthTableColumn` returns just the F column as a string of `0`/`1` in row order (used by golden tests).

**Complexity:** evaluation O(n). Truth table O(2^k · n) time, O(h) extra space.

### 4.6 DIMACS CNF input

Format reference (from the assignment PDF): https://jix.github.io/varisat/manual/0.2.0/formats/dimacs.html

```cpp
struct CNF { int numVars = 0; int numClauses = 0; std::vector<std::vector<int>> clauses; };
enum class TreeShape { LEFT_DEEP, BALANCED };

CNF         readDimacs(const std::string& path);
std::string cnfToInfix(const CNF& cnf, TreeShape shape);
```

**Parsing rules**

- Lines starting with `c` are comments → skip.
- Header `p cnf <vars> <clauses>` → fill `numVars`, `numClauses`.
- Clauses are whitespace-separated integers terminated by `0`. A clause may span lines, and several clauses may share a line.
- A line starting with `%` ends the input (some SATLIB/SAT 2002 files have this).
- If the clause count read ≠ the header count → print a warning to `stderr`, but continue. `CNF::numClauses` holds the count actually read.
- A missing `p cnf` header before the first clause is an error (§7).

**Conversion rules**

- Literal `k > 0` → `xk`. Literal `-k` → `(~xk)`.
- Clause = literals joined by `+`, fully parenthesized, left-deep: `((a+b)+c)`. A single-literal clause is just the literal.
- Formula = clauses joined by `*`:
  - LEFT_DEEP: `((C1*C2)*C3)…` → height Θ(#clauses). This is the stress case for recursion.
  - BALANCED: recursive midpoint split → height Θ(log #clauses + max clause length).
- An empty clause or empty formula → error (§7).

---

## 5. Command-line interface

Binary: `./logic_a1`

```
./logic_a1 --formula "<infix>"   [--task all|1|2|3|4|5] [--assign "p=1,q=0"] [--table]
./logic_a1 --cnf <file.cnf>      [--shape left|balanced] [--task ...] [--assign ...] [--table]
./logic_a1 --help
```

- Default `--task all` prints labelled sections: `TOKENS`, `PREFIX`, `INORDER (rec/iter)`, `PREORDER (rec/iter)`, `POSTORDER (rec/iter)`, `HEIGHT (rec/iter)`, `EVAL` (if `--assign` is given), `TRUTH TABLE` (if `--table` is given).
- `--task n` prints only the sections of that task: 1 = `TOKENS`, `PREFIX`; 2 = `TREE` (node count); 3 = the three traversals; 4 = `HEIGHT`; 5 = `EVAL` and `TRUTH TABLE`. `--assign` and `--table` have no effect for tasks 1–4.
- `--formula` and `--cnf` are mutually exclusive; giving both, or neither, is an error.
- `--assign` is a comma-separated list of `atom=0|1` (whitespace tolerated) and must cover every atom of the formula, otherwise `evaluate*` throws (§7).
- For `--cnf`, traversal strings longer than 200 characters are truncated in the display as `<first 100>…<last 100> (N chars)`.
- A recursive variant is run only if the tree height is ≤ `kRecursionLimit` (10 000). Otherwise its line prints `SKIPPED (height <h> > 10000)` and only the iterative result is shown. This keeps the CLI from crashing on left-deep CNF trees; the deliberate crash experiments live in `bench` (§8.2), which uses `fork()`.
- Exit code 0 on success, 1 on any error.

---

## 6. Build

- `make` → `./logic_a1`
- `make test` → builds and runs `tests/run_tests`; non-zero exit on any failure.
- `make bench` → builds `bench/bench` and writes `bench/results.csv`.
- `make docs` → runs `doxygen docs/Doxyfile`; output goes to `docs/html/`.
- `make clean`

---

## 7. Error handling

Input is assumed well-formed, but failures must be loud, never silent:

- Throw `std::invalid_argument` with a clear message for: an illegal character, unbalanced parentheses, an operator missing operands, two adjacent operands with no operator (e.g. `p q`), leftover nodes after tree building, an atom missing from the `Assignment`, or a malformed DIMACS file (cannot be opened, missing `p cnf` header, non-integer token, empty clause or formula).
- Command-line errors (unknown flag, bad `--assign` syntax, `--formula` together with `--cnf`) are reported the same way.
- `main` catches the exception, prints `Error: <message>` to `stderr`, and exits 1.

---

## 8. Analysis requirements (Task 6)

### 8.1 Datasets

1. **Golden formulas** (Appendix A).
2. **Synthetic scaling:** randomly generated formulas of n = 10, 100, 1k, 10k, 100k, 1M nodes, plus left-deep chains of the same sizes.
3. **SAT 2002 benchmark** (from https://satcompetition.github.io/): at least 6 `.cnf` files spanning small → large clause counts, each built both LEFT_DEEP and BALANCED. Record file name, vars, clauses, nodes, and height.

### 8.2 Metrics per function

- Wall time: `std::chrono::steady_clock`, median of 5 runs.
- Node count and `sizeof(Node)` → tree memory estimate. Also note `std::string` small-string-optimisation (SSO) behaviour.
- Peak auxiliary structure size (max stack/queue length), instrumented via a counter.
- Process peak RSS via `getrusage(RUSAGE_SELF).ru_maxrss` (POSIX).
- Recursive-function survival: run each recursive function in a `fork()`ed child, and record OK / CRASH (SIGSEGV) and the height at which it crashed. On Windows, skip the recursive function when h > a set threshold, and mark it `SKIPPED`.

### 8.3 Output

`bench/results.csv` columns:
`dataset,shape,nodes,height,atoms,function,median_ms,aux_peak,rss_kb,status`

### 8.4 Required discussion points in the report

- Linear time for T1–T4 and for single evaluation, confirmed by the plots.
- Recursive vs iterative: same Big-O, but call stack vs heap stack. Show the crash depth.
- Effect of tree shape (left-deep vs balanced) on height, stack use, and the BFS queue width.
- Truth table: O(2^k · n). Explain why it is infeasible for SAT instances (e.g., k = 50 → ~10^15 rows), and what the course says this implies (SAT is NP-complete).
- Memory: the cost per node and the overhead of pointers vs an index-based node pool (a discussion point, implementation optional).

---

## 9. Documentation

- Every public function in `formula.h` has a Doxygen block: `@brief`, `@param`, `@return`, `@throws`, `@note Complexity: …`. Each source file has a `@file` header with authors.
- `docs/Doxyfile`: `EXTRACT_ALL=YES`, `GENERATE_LATEX=NO`, `INPUT = include src README.md`, `USE_MDFILE_AS_MAINPAGE = README.md`.
- Report under `docs/report/`, as plain HTML+CSS (charts as inline SVG, or Chart.js loaded from a CDN). Required pages:
  - `index.html` — overview, team, how to build and run.
  - `algorithms.html` — each algorithm with a worked trace (Appendix A, case A).
  - `results.html` — tables and plots from `results.csv`.
  - `analysis.html` — the discussion points from §8.4.
  - `lean.html` — the Part B proofs, with a rule → tactic mapping.
- Consistent styling: one shared `style.css`, readable typography, and a light/dark-friendly palette.

---

## 10. Part B — Lean 4

- Files: `lean/Examples.lean` (Ex 1.13–1.18) and `lean/Practice.lean` (extra sequents for demo prep).
- Each example is written as `theorem ex1_13 (p q r : Prop) : <premises> → <conclusion> := by …`. Premises are curried (`P → Q → R`), mirroring the sequent `P, Q ⊢ R`.
- **Spec-first workflow:** commit all statements ending in `sorry` first, and check that they match the textbook exactly. Then fill in the proofs.
- Prefer tactics that mirror natural deduction: `intro`, `exact`, `apply`, `constructor`, `cases`/`rcases`, `Or.inl`/`Or.inr`, `absurd`. Where a proof needs classical reasoning (¬¬-elimination, LEM, PBC), use `Classical.byContradiction` / `Classical.em`, and add a comment naming the textbook rule.
- **Done means:** the file compiles with no `sorry` and no errors.
- Maintain `lean/RULES.md`: a mapping from each natural-deduction rule (∧i, ∧e₁, ∧e₂, ∨i₁, ∨i₂, ∨e, →i, →e, ¬i, ¬e, ¬¬e, ⊥e, MT, PBC, LEM) to its Lean tactic or term.

---

## 11. Submission

- File name: `<BITSID>_LOGIC_A1.zip`, where `<BITSID>` is the BITS ID of the member who submits. One submission per group (max 5 students), by Oct 15 midnight. The demo takes place right after submission and every member must attend.
- Build the zip from the working tree, not with `git archive`: `docs/html/` is git-ignored but must be included.
- Contents: `include/ src/ tests/ bench/ docs/ (incl. generated html) lean/ Makefile README.md SPEC.md`.
- Exclude: binaries, `.o` files, the raw SAT benchmark archive (list the files used in the README instead), and `.lake/`.

---

## 12. Open questions (resolve and record answers here)

1. Is `~p` (without parentheses) valid input, or always `(~p)`? → *Current spec: accept both, and emit `(~p)`.*
2. Height convention: does a leaf have height 0 or 1? → *Current spec: 0.* Confirm with the instructor. If it's 1, change `heightRec`/`heightIter`, the `height:` line of every golden file in Appendix A, and the null/leaf values in §4.4 and CLAUDE.md.
3. Truth table format expected by the evaluator (0/1 vs T/F). → *Current spec: 0/1.*
4. Is the use of AI coding tools permitted? The assignment PDF only says "discuss with friends but refrain from copying the code" and is silent on AI tools. Ask the instructor before the demo.
5. Which textbook edition are Examples 1.13–1.18 from? The PDF just says "the textbook". Record the edition and the exact sequents here before `lean/Examples.lean` is written.
6. How does `bench` read peak stack/queue sizes (`aux_peak`) without changing the signatures in `formula.h`? → *Proposal:* a global counter in a private header, compiled in only with `-DBENCH_INSTRUMENT`.
7. The PDF header says the due date is 15 October **2025**, but TODO.md and §11 assume Thu 15 Oct 2026. Confirm the deadline on the LMS.

## Answers : 
1. current implementation is good
2. current spec is good
3. 0/1 should be used
4. AI is permitted
5. Examples should self made or extracted from a          questions file
6. proposal is good
7. submission is on 15 Oct 2026

---

## Appendix A — Golden test cases

The test file format (`tests/golden/<name>.txt`) is one `key: value` per line. Keys: `input, prefix, inorder, preorder, postorder, height, atoms, table`.
`inorder` = canonical form. `table` = the F column in row order (§4.5).

**A. Worked example**
```
input: ((p+(~q))>r)
prefix: > + p ~ q r
inorder: ((p+(~q))>r)
preorder: > + p ~ q r
postorder: p q ~ + r >
height: 3
atoms: p q r
table: 01110101
```

**B. Single atom**
```
input: p
prefix: p
inorder: p
preorder: p
postorder: p
height: 0
atoms: p
table: 01
```

**C. Double negation**
```
input: (~(~p))
prefix: ~ ~ p
inorder: (~(~p))
preorder: ~ ~ p
postorder: p ~ ~
height: 2
atoms: p
table: 01
```

**D. Right-nested implication**
```
input: (p>(q>r))
prefix: > p > q r
inorder: (p>(q>r))
preorder: > p > q r
postorder: p q r > >
height: 2
atoms: p q r
table: 11111101
```

**E. Multi-character atoms (CNF style)**
```
input: ((x1+(~x2))*(x2+x3))
prefix: * + x1 ~ x2 + x2 x3
inorder: ((x1+(~x2))*(x2+x3))
preorder: * + x1 ~ x2 + x2 x3
postorder: x1 x2 ~ + x2 x3 + *
height: 3
atoms: x1 x2 x3
table: 01000111
```

**F. Whitespace tolerance**
```
input: ( p * q )
prefix: * p q
inorder: (p*q)
preorder: * p q
postorder: p q *
height: 1
atoms: p q
table: 0001
```

**G. Robustness — precedence (not fully parenthesized)**
```
input: (~p+q)
prefix: + ~ p q
inorder: ((~p)+q)
preorder: + ~ p q
postorder: p ~ q +
height: 2
atoms: p q
table: 1101
```

**H. Robustness — right associativity of `>`**
```
input: (p>q>r)
prefix: > p > q r
inorder: (p>(q>r))
preorder: > p > q r
postorder: p q r > >
height: 2
atoms: p q r
table: 11111101
```

**I. Natural sort of atoms**
```
input: (x10*x2)
prefix: * x10 x2
inorder: (x10*x2)
preorder: * x10 x2
postorder: x10 x2 *
height: 1
atoms: x2 x10
table: 0001
```

**DIMACS fixture** — `tests/data/tiny.cnf`:
```
c tiny example
p cnf 3 2
1 -2 0
2 3 0
```
Expected `cnfToInfix(..., LEFT_DEEP)` = `((x1+(~x2))*(x2+x3))` → then identical to case E.
