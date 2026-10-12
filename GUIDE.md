# GUIDE.md — run the project and work with it

A hands-on walkthrough. Every command below is meant to be run from the **repository root**
(the folder that contains `Makefile`). `README.md` is the short reference, `SPEC.md` the rules,
`TODO.md` the task board; this file is the "how do I actually use it" page.

Contents: [1 Install](#1-install) · [2 First run](#2-first-run-5-minutes) · [3 Command-line cookbook](#3-command-line-cookbook) ·
[4 Tests](#4-tests) · [5 Benchmarks](#5-benchmarks) · [6 Docs and report](#6-docs-and-report) · [7 Lean](#7-lean-part-b) ·
[8 Working on the code](#8-working-on-the-code) · [9 Troubleshooting](#9-troubleshooting) · [10 Demo cheat sheet](#10-demo-cheat-sheet) · [11 Submitting](#11-submitting)

---

## 1. Install

You need: `g++` (C++17), `make`, `git`, `zip`, and optionally `doxygen` and Lean 4 (via `elan`).

| System | Command |
|---|---|
| Arch / Manjaro | `sudo pacman -S gcc make git zip doxygen` |
| Ubuntu / Debian / WSL | `sudo apt install g++ make git zip doxygen` |
| macOS | `brew install gcc make git zip doxygen` (use `g++-14` or similar via `CXX=...`) |

Check: `g++ --version` (needs C++17, i.e. GCC 7+), `make --version`, `doxygen --version`.

**Lean 4** (Part B only). The standard installer is `elan`:

```bash
curl -sSfL https://elan.lean-lang.org/elan-init.sh | sh -s -- -y --default-toolchain stable
```

If you installed it with `--no-modify-path` (as was done on this machine), `lean` is **not** on your `PATH`
until you add it. Either run `export PATH="$HOME/.elan/bin:$PATH"` in each terminal, or put that line in
your `~/.zshrc` / `~/.bashrc`. Check: `lean --version`.

VS Code: install the extension **Lean 4** (publisher *leanprover*), open `lean/Practice.lean`, and click inside a proof;
the Infoview panel (`Ctrl+Shift+Enter`) shows the goals.

---

## 2. First run (5 minutes)

```bash
make            # builds ./logic_a1   (zero warnings expected)
make test       # runs 193 checks; last line should be "193 passed, 0 failed"
./logic_a1 --formula "((p+(~q))>r)" --table
```

The last command prints every task for the worked example from the spec:

```
TOKENS: ( ( p + ( ~ q ) ) > r )
PREFIX: > + p ~ q r
TREE: 6 nodes
INORDER (rec): ((p+(~q))>r)
...
HEIGHT (rec): 3
HEIGHT (iter): 3
TRUTH TABLE
p q r | F
0 0 0 | 0
...
```

If that works, the toolchain is fine. Targets at a glance:

| Command | What it does |
|---|---|
| `make` | build `./logic_a1` |
| `make test` | build and run `tests/run_tests` (non-zero exit on any failure) |
| `make bench` | build and run the benchmarks → `bench/results.csv` (see §5 before running) |
| `make docs` | Doxygen → `docs/html/index.html` (needs `doxygen`) |
| `make dist BITSID=<id>` | build the submission zip `<id>_LOGIC_A1.zip` |
| `make clean` | delete binaries, objects, `docs/html` |

---

## 3. Command-line cookbook

Connectives: `~` not · `*` and · `+` or · `>` implies. Atoms: letters, digits, `_`, starting with a letter (`p`, `x12`).
Precedence: `~` > `*` > `+` > `>`; `>` is right-associative, so `p>q>r` means `p>(q>r)`.
Whitespace is ignored. Put the formula in quotes (`>` and `*` mean something to the shell).

```bash
./logic_a1 --help

# Everything for one formula
./logic_a1 --formula "((p+(~q))>r)"

# One task at a time:  1 tokens+prefix · 2 tree · 3 traversals · 4 height · 5 evaluation/truth table
./logic_a1 --formula "(p>q>r)" --task 1        # PREFIX: > p > q r  (right-associative)
./logic_a1 --formula "(p+q*r)" --task 3        # in/pre/post-order, rec and iter

# Evaluate under an assignment (must list every atom)
./logic_a1 --formula "((p+(~q))>r)" --assign "p=1,q=0,r=0" --task 5

# Truth table (refused above 20 atoms, with an explanation)
./logic_a1 --formula "((x1+(~x2))*(x2+x3))" --table --task 5

# A SAT benchmark file, as a left-deep or a balanced tree
./logic_a1 --cnf tests/data/tiny.cnf --task 1
./logic_a1 --cnf data/ca002.shuffled.cnf --shape balanced --task 4
```

Notes:

* **Errors** print `Error: <message>` to stderr and exit with code 1, for example `./logic_a1 --formula "(p+"` → `unbalanced parentheses: missing ')'`.
* With `--cnf`, long strings are shortened to `<first 100>…<last 100> (N chars)`, and the recursive variants
  are shown as `SKIPPED (height h > 10000)` for tall trees, because they would overflow the stack. The iterative
  results are always printed.
* `data/*.cnf` (the SAT 2002 files) are **not** in git. See `data/README.md` for where to get the 8 files used.
  `tests/data/tiny.cnf` is always there.

---

## 4. Tests

```bash
make test
```

Each check prints `PASS` or `FAIL`; the exit code is non-zero if any fail. What is covered:

* the nine **golden files** `tests/golden/A.txt … I.txt` (every key: prefix, in/pre/post-order, height, atoms, truth table),
* invariants: recursive result == iterative result, pre-order == prefix, `evaluateRec` == `evaluateIter` on every row,
* error paths (illegal characters, unbalanced parentheses, malformed prefix, bad DIMACS),
* precedence and associativity beyond the golden cases,
* 100,000-level-deep trees (iterative code and `freeTree` must not overflow the stack),
* 1000 random formulas through the whole pipeline.

Memory-error check (recommended before every merge):

```bash
make clean && make CXXFLAGS+="-fsanitize=address,undefined -g" test
make clean && make          # go back to the normal build
```

**Rules** (also in `CLAUDE.md`): never edit a golden file or a test to make it pass. If you believe a test is wrong, say so in the group chat.

To add a golden case: create `tests/golden/J.txt` with the same `key: value` lines as the others
(`input, prefix, inorder, preorder, postorder, height, atoms, table`). `make test` picks up every `.txt` in that folder.

---

## 5. Benchmarks

```bash
make bench
```

This writes `bench/results.csv` (the committed one has ~640 rows) and **appends** the crash-depth rows. Before you run it:

* It takes a few minutes and needs about **1.5 GB of free RAM** for the biggest SAT file (`cache_10`, 5.5 M nodes). Close other programs.
* The SAT rows need the eight `.cnf` files in `data/`. Without them the benchmark prints `no data/*.cnf found: SAT rows skipped` and carries on.
* It **overwrites** `bench/results.csv`, which is a committed deliverable. To experiment without touching it:

```bash
make bench/bench bench/crash_search
./bench/bench /tmp/test.csv --quick     # skips the 1M-node, large-SAT and big truth-table runs (a few seconds)
./bench/crash_search /tmp/test.csv      # appends the stack-overflow depth rows
```

Reading the CSV: `dataset,shape,nodes,height,atoms,function,median_ms,aux_peak,rss_kb,status`.
`status` is `OK`, `CRASH` (recursive function killed by SIGSEGV in its `fork()`ed child) or `SKIPPED`.
`aux_peak` is the largest explicit stack/queue (iterative) or recursion depth (recursive).
Quick look: `column -s, -t < bench/results.csv | less -S`.

---

## 6. Docs and report

* **API docs:** `make docs`, then open `docs/html/index.html`. Zero warnings is the goal; a new function needs a Doxygen block
  (`@brief @param @return @throws @note Complexity`). The block in `include/formula.h` documents the public function; in the
  `.cpp` file only add an `@details` block for the *why* of the algorithm (Doxygen merges the two).
* **Report:** open `docs/report/index.html` in a browser (it is plain HTML + one `style.css`; no server needed).
  Pages: index, algorithms (worked trace of case A), results (tables and plots), analysis, lean.
  The pages are **static**: if you re-run `make bench` and the numbers change, the report text and charts do not update by
  themselves. The script that generated them was a throw-away helper and is not part of the repository, so after a re-run
  either keep the committed `results.csv` or edit the affected numbers in `analysis.html` by hand.

---

## 7. Lean (Part B)

```bash
export PATH="$HOME/.elan/bin:$PATH"     # if `lean` is not found
lean lean/Examples.lean                  # no output = every proof is accepted
lean lean/Practice.lean
grep -n sorry lean/*.lean                # must print nothing when you are done
```

* A sequent `P, Q ⊢ R` is written as `theorem name (p q r : Prop) : P → Q → R := by …`.
* `lean/RULES.md` is the cheat sheet: each natural-deduction rule (∧i, ∨e, →i, ¬e, LEM, …) and the tactic that does it.
  The proofs annotate every step with its rule name; keep doing that, the demo audience will ask.
* `lean/Practice.lean` has extra sequents for drilling. Method: read a proof, delete it down to `sorry`, rebuild it from `RULES.md`.
* The statements of Examples 1.13–1.18 must match the textbook exactly, and never be changed to make a proof easier.
  See TODO 5.2 and the note in `TODO.md`: the current statements are self-made and still need to be compared with the book.
* **Check before committing:** `lean/Examples.lean` is the Task 1 file and should contain the six theorems
  `ex1_13 … ex1_18`. If it does not, compare with the last commit: `git show HEAD:lean/Examples.lean | head -40`
  (`git diff lean/Examples.lean` shows what is currently different).

---

## 8. Working on the code

**Where things live**

| I want to change… | File |
|---|---|
| tokens / infix → prefix (T1) | `src/tokenizer.cpp`, `src/infix_prefix.cpp` |
| prefix → tree, `freeTree` (T2) | `src/tree_build.cpp` |
| traversals (T3) | `src/traversal.cpp` |
| height (T4) | `src/height.cpp` |
| evaluation, atoms, truth table (T5) | `src/eval.cpp` |
| DIMACS reading, CNF → infix | `src/cnf_reader.cpp` |
| command-line options | `src/main.cpp` |
| the public contract (types, signatures) | `include/formula.h` — **ask the group before changing a signature** |
| tests / random generators | `tests/run_tests.cpp`, `tests/gen.{h,cpp}` |
| benchmarks | `bench/bench.cpp`, `bench/crash_search.cpp`, counters in `src/instrument.h` |

**The loop for every change**

1. Pick **one** item from `TODO.md`; create a branch: `git checkout -b t2.3-tree-build`.
2. Edit. Rebuild and test: `make test`. Fix until it reads `0 failed`; the build must have **zero warnings**
   (`-Wall -Wextra`).
3. Run the sanitizer build once (§4).
4. Tick the item in `TODO.md` **in the same commit**, then commit: `git add -A && git commit -m "TODO 2.x: short description"`.
5. Merge to `main`, or open a pull request for someone to glance over.

**Rules that bite** (from `CLAUDE.md`)

* C++17, standard library only; keep the code warning-free. 4-space indent, braces on the same line, `camelCase` functions, `PascalCase` types.
* Tokens are strings; never reverse or index characters of the formula.
* `freeTree` and anything that may run on CNF-sized trees must be **iterative** (the `*Rec` versions are deliberately recursive
  and are guarded by the CLI and the benchmark).
* Comment the *why* in algorithm code, not the what.
* `SPEC.md` changes go in a separate commit titled `SPEC: …`, after the group has agreed.

**Using Claude Code** (optional): run `claude` in the repo root. It reads `CLAUDE.md` automatically.
Good prompts name one TODO item: *"Do TODO 3.x. Run make test and show me the result."*

---

## 9. Troubleshooting

| Symptom | Cause / fix |
|---|---|
| `lean: command not found` | `export PATH="$HOME/.elan/bin:$PATH"` (§1) |
| `doxygen: command not found` / `make docs` fails | install `doxygen` (§1) |
| `make: *** No rule to make target 'tests/gen.o'` | run `make` from the repository root, not from a sub-folder |
| `fatal error: formula.h: No such file` | you compiled by hand; use `make`, or add `-Iinclude -Itests` |
| `zip: command not found` on `make dist` | install `zip` (§1) |
| `Error: atom 'x' has no value in the assignment` | `--assign` must give a value for **every** atom in the formula |
| `Error: illegal character '&' …` | only `~ * + >` and parentheses are allowed |
| `Error: two adjacent operands (missing operator)` | `p q` is not a formula; write `p*q` |
| CLI prints `SKIPPED (height … > 10000)` | expected for tall CNF trees: the recursive variants would crash. Iterative results are shown |
| A recursive function segfaults when you call it directly on a huge tree | expected stack overflow (~175 000 levels at 8 MB). Use the `*Iter` variant, or `ulimit -s unlimited` for an experiment |
| `make bench` is killed / the machine freezes | out of memory on the biggest SAT file; close programs, or remove `data/cache_10.shuffled.cnf` and `data/fifo8_200.shuffled.cnf` |
| Tests pass but the sanitizer build complains | read the report: ASan prints the exact allocation and use site |
| Stale object files after switching flags | `make clean && make` |

---

## 10. Demo cheat sheet

Everyone should be able to do these without notes:

1. **Trace case A by hand** (`((p+(~q))>r)`), T1 → T5: tokens → reverse and swap → stack table → prefix `> + p ~ q r` → tree (height 3) →
   in/pre/post-order → truth table `01110101`. The worked traces are in `docs/report/algorithms.html`.
2. **Explain why the pop rule is `≥` for `>` and `~` but `>` for `*` and `+`** in the reversed scan (right- vs left-associativity; `p>q>r` must give `> p > q r`).
3. **Recursive vs iterative:** same O(n) time; the recursive version uses the call stack (about 48 bytes per level, dies near 175 000 levels on 8 MB);
   the iterative version uses a heap `std::vector`. Show it: `./logic_a1 --cnf data/<big>.cnf` prints `SKIPPED` for the recursive lines.
4. **Why no truth table for SAT files:** 2^k rows × O(n) per row; 2^50 ≈ 10^15 rows; SAT is NP-complete.
5. **One live Lean proof**, using `lean/RULES.md`: `intro` the premises, look at the shape of the goal, apply the matching rule.

---

## 11. Submitting

1. `make test` green, zero warnings, `grep -n sorry lean/*.lean` empty, names and BITS IDs filled in (`README.md`, `@authors` in the sources, `docs/report/index.html`).
2. `make dist BITSID=<the submitting member's BITS ID>` → creates `<BITSID>_LOGIC_A1.zip` containing `include src tests bench docs lean Makefile README.md SPEC.md`
   plus `data/README.md`; it excludes binaries, object files, the raw SAT files and `.lake`.
3. Test it: unzip into an empty folder, then `make && make test`.
4. Upload one zip per group to the LMS, and confirm in the group chat. The demo follows straight after; absence means zero credit.
