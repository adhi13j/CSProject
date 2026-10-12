# CS F214 Assignment 1 — Propositional-logic parse trees (C++17) and Lean 4 proofs

BITS Pilani, Hyderabad Campus · Logic in CS · Assignment 1 (30 marks).

Part A stores a propositional formula as a binary parse tree and runs the course
algorithms on it: infix → prefix, prefix → tree, in/pre/post-order traversals
(recursive and iterative), height (recursive and iterative), evaluation and truth
tables. Part B proves Examples 1.13–1.18 in Lean 4.

`SPEC.md` is the source of truth for every behaviour described here.

**New here? Read `GUIDE.md`**: install, first run, command-line cookbook, tests, benchmarks, Lean, daily workflow, troubleshooting and the demo cheat sheet.

## Team

| Name | BITS ID |
|---|---|
| *replace with member A* | |
| *replace with member B* | |
| *replace with member C* | |
| *replace with member D* | |
| *replace with member E* | |

## Requirements

`g++` with C++17, `make`, `doxygen` (only for `make docs`), POSIX (`fork`, `getrusage`; on Windows use WSL).
Part B needs Lean 4 (via `elan`) — see `lean/`.

## Build, run, test

```bash
make            # builds ./logic_a1   (g++ -std=c++17 -Wall -Wextra -O2, zero warnings)
make test       # builds and runs tests/run_tests; non-zero exit on any failure
make bench      # builds bench/bench and bench/crash_search, writes bench/results.csv
make docs       # doxygen docs/Doxyfile  ->  docs/html/index.html
make dist BITSID=<your BITS ID>   # builds <BITSID>_LOGIC_A1.zip (SPEC section 11)
make clean
```

AddressSanitizer / UBSan run: `make clean && make CXXFLAGS+="-fsanitize=address,undefined -g" test`.

### Command line

```bash
./logic_a1 --formula "((p+(~q))>r)" --table
./logic_a1 --formula "(p>q>r)" --task 3
./logic_a1 --formula "((p+(~q))>r)" --assign "p=1,q=0,r=1" --task 5
./logic_a1 --cnf data/ca002.shuffled.cnf --shape balanced --task 4
./logic_a1 --help
```

Connectives: `~` not, `*` and, `+` or, `>` implies. Atoms: `[A-Za-z][A-Za-z0-9_]*`.
Precedence: `~` > `*` > `+` > `>`; `>` is right-associative, `*` and `+` left-associative,
so `p>q>r` is `p>(q>r)`. Default `--task all` prints `TOKENS`, `PREFIX`, `TREE`,
`INORDER`/`PREORDER`/`POSTORDER` (rec/iter), `HEIGHT` (rec/iter), and with `--assign` / `--table`
the evaluation and truth table. With `--cnf`, recursive variants are skipped above height
10 000 (they would overflow the stack on left-deep CNF trees) and long strings are truncated.
Errors print `Error: <message>` to stderr and exit 1.

## How the tasks map to the code

| Task | Function(s) | File |
|---|---|---|
| T1 infix → prefix | `tokenize`, `infixToPrefix`, `joinTokens` | `src/tokenizer.cpp`, `src/infix_prefix.cpp` |
| T2 prefix → tree | `prefixToTree`, `freeTree` (iterative) | `src/tree_build.cpp` |
| T3 traversals | `inorder/preorder/postorder` × `Rec/Iter` | `src/traversal.cpp` |
| T4 height | `heightRec`, `heightIter` (BFS) | `src/height.cpp` |
| T5 evaluation | `collectAtoms`, `evaluateRec/Iter`, `printTruthTable`, `truthTableColumn` | `src/eval.cpp` |
| DIMACS input | `readDimacs`, `cnfToInfix` | `src/cnf_reader.cpp` |
| CLI | `main` | `src/main.cpp` |

Contracts and Doxygen comments for all public functions are in `include/formula.h`.

### Algorithms in one paragraph each

* **Infix → prefix (T1).** Tokenize, reverse the token list, swap `(` and `)`, run the
  operator-stack infix → postfix algorithm, reverse the output. Because the string is
  reversed, the pop rule flips: left-associative `*`/`+` pop operators of *higher*
  precedence, right-associative `>`/`~` pop operators of *higher or equal* precedence.
* **Prefix → tree (T2).** Scan the tokens right to left with a stack of nodes. For a binary
  operator the first pop is the left child. A `~` node keeps its operand in `right`.
* **In-order (T3), iterative.** A stack of `(node, state)` frames with states
  OPEN / MIDDLE / CLOSE prints the parentheses; pre-order uses a stack; post-order uses one
  stack and a last-visited pointer.
* **Height (T4).** Recursive `1 + max(...)` with `null = -1`, and a level-order BFS
  that returns `levels - 1`.
* **Evaluation (T5).** Bottom-up (post-order) with a value stack; the truth table counts rows
  with the first atom as the most significant bit.

## Project layout

```
include/formula.h      public types and function contracts
src/                   implementation (one module per task) and the CLI
src/instrument.h       aux-size counters, compiled in only for the benchmark
tests/                 run_tests.cpp, golden/*.txt (SPEC Appendix A), gen.{h,cpp}, data/tiny.cnf
bench/                 bench.cpp, crash_search.cpp, results.csv
data/                  SAT 2002 files used (not committed; see data/README.md)
docs/                  Doxyfile, report/ (HTML), html/ (generated)
lean/                  Part B: Examples.lean, Practice.lean, RULES.md
SPEC.md  TODO.md  CLAUDE.md  GETTING_STARTED.md
```

## SAT 2002 files used

Eight files from the SAT 2002 competition archive, from 70 to 879,754 clauses
(`ca002`, `unif-c900-v450`, `3col260_5_7`, `rand_net70-30-1`, `too_large_gr_rcs_w7`,
`ip36`, `fifo8_200`, `cache_10`). Sizes, source path and the measured tree heights are in
`data/README.md`. The raw archive is not part of the submission.

## Results and report

`make bench` writes `bench/results.csv`; the HTML report in `docs/report/`
(`index.html`, `algorithms.html`, `results.html`, `analysis.html`, `lean.html`) tabulates and
plots it and discusses the findings (linear time, recursion vs. iteration crash depth,
tree shape, truth-table blow-up, memory per node).

## Part B (Lean 4)

`lean/Examples.lean` holds Examples 1.13–1.18 and `lean/Practice.lean` extra sequents for
the demo; `lean/RULES.md` maps each natural-deduction rule to its Lean tactic or term.
