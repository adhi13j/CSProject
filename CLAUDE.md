# CLAUDE.md — instructions for Claude Code

## Project
CS F214 Assignment 1: propositional-logic parse trees in C++ (Part A), plus Lean 4 proofs (Part B).
**SPEC.md is the source of truth. TODO.md is the task board.** Read both at the start of every session.

## Hard rules
- C++17, standard library only. Code must compile with `g++ -std=c++17 -Wall -Wextra -O2`, with zero warnings.
- Do NOT change any signature or type in `include/formula.h`, any golden file in `tests/golden/`, or SPEC.md without asking first and explaining why.
- Never edit a test just to make it pass. If you think a test is wrong, stop and say so.
- Work on ONE TODO item at a time. When it's done, run `make test`, show the result, and suggest a commit message.
- Every function needs a Doxygen block: `@brief @param @return @throws`, and `@note Complexity: time / space`.
- Comment the *why* in the algorithm code (e.g. why the pop rule is ≥ for right-associative operators in the reversed scan).
- `freeTree`, and any function expected to run on CNF-sized trees, must be iterative (deep left-deep trees overflow the call stack). The `*Rec` functions are deliberately recursive (SPEC §4); the CLI skips them above `kRecursionLimit` and `bench` runs them in a `fork()`ed child.
- Tokens are strings; never reverse or index characters of the formula directly.

## Conventions (from SPEC)
- Precedence: `~`(4, right) > `*`(3, left) > `+`(2, left) > `>`(1, right).
- The NOT node's operand goes in `right`; `left` is null.
- Height: null = -1, leaf = 0.
- Canonical infix: atom `p`, negation `(~X)`, binary `(L op R)`, no spaces.
- Pre-/post-order output is single-space-separated.
- Truth table: atoms are natural-sorted, the first atom is the MSB, row 0 is all-false, and values are printed as 0/1.

## Commands
- `make` · `make test` · `make bench` · `make docs` · `make clean`
- ASan check: `make clean && make CXXFLAGS+="-fsanitize=address -g" test`
- Lean: run `lake build` inside `lean/` if it's a Lake project, otherwise `lean lean/Examples.lean`

## Lean (Part B)
- Theorem statements must match the textbook exactly. Never change a statement to make a proof easier.
- Prefer natural-deduction-style tactics, and comment each step with its rule name (∧i, →e, ¬¬e, …).
- When helping with the proofs, explain them step by step; the students must be able to reproduce them live at the demo.

## Style
- 4-space indent, braces on the same line, `camelCase` functions, `PascalCase` types.
- One module per file, as listed in SPEC/TODO.
