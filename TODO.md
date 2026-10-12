# TODO.md — Task board

**Deadline:** Thu 15 Oct, midnight. **Demo:** right after submission.
**Owners:** replace A–E with names. Tick `[x]` only when the acceptance check passes **and** the work is committed.
**Rule:** nothing is "done" unless `make test` is green.

```
Mon night   Phase 0–1   (everyone)        spec + contract + tests
Tue         Phase 2     (A,B,C,D) ‖ Phase 5 (E)
Wed         Phase 3–4   integrate, benchmark, Doxygen
Thu         Phase 4–6   report, polish, rehearse, submit
```

---

## Phase 0 — Setup (everyone, ~1 h)

- [ ] **0.1** Every member: install `g++`, `make`, `git`, `doxygen`, VS Code + Lean 4 extension (elan). Check `g++ --version`, `doxygen --version`.
- [ ] **0.2** (A) Create a GitHub/GitLab private repo, add everyone, and push the skeleton from GETTING_STARTED.md.
- [x] **0.3** (A) Add `SPEC.md`, `TODO.md`, `CLAUDE.md`, `.gitignore` (`*.o`, `logic_a1`, `tests/run_tests`, `bench/bench`, `docs/html/`, `.lake/`, `data/*.cnf`).
- [x] **0.4** (B) Download the SAT 2002 benchmark → `data/`. Pick ≥6 files of increasing size and list them in `data/README.md`, with vars/clauses from each header.

## Phase 1 — Contract and tests first (everyone, Mon night)

- [x] **1.1** Group read-through of SPEC.md. Resolve §12 open questions, or record "assumed".
- [x] **1.2** (A) `include/formula.h`: all types + signatures from SPEC §3–4, with full Doxygen blocks and **no implementations**.
  *Accept:* header compiles standalone (`g++ -std=c++17 -fsyntax-only include/formula.h`).
- [x] **1.3** (C) `tests/golden/A.txt … I.txt` copied from SPEC Appendix A. **Each person re-derives one case by hand** to verify it.
- [x] **1.4** (C) `tests/run_tests.cpp`: loads every golden file and checks every key; also checks the invariants (rec == iter, preorder == prefix, heightRec == heightIter, evaluateRec == evaluateIter on all rows). Prints PASS/FAIL per check, and exits non-zero on failure.
- [x] **1.5** (A) `Makefile` with targets `all test bench docs clean` (SPEC §6). Build with stub implementations that throw `"not implemented"`.
  *Accept:* `make test` builds and fails cleanly (red is expected now). Keep the base flags in a separate variable (e.g. `BASEFLAGS`) that is always used alongside `CXXFLAGS`: a command-line `CXXFLAGS+=...` replaces the Makefile's own value, so the ASan run would otherwise drop `-std=c++17 -Wall -Wextra -O2`.

## Phase 2 — Implementation (Tue, in parallel)

- [x] **2.1** (A) `src/tokenizer.cpp` — `tokenize`, `joinTokens`.
  *Accept:* tokens correct for all golden inputs; multi-character atoms intact; illegal char → `invalid_argument`.
- [x] **2.2** (A) `src/infix_prefix.cpp` — T1, using the reverse method with the ≥/> pop rule (SPEC §4.1).
  *Accept:* `prefix:` matches for A–I. **H must give `> p > q r`.**
- [x] **2.3** (B) `src/tree_build.cpp` — T2 `prefixToTree` + iterative `freeTree`.
  *Accept:* node invariants hold (NOT child on the right); bad prefix → exception; no leaks (`valgrind` or `-fsanitize=address`).
- [x] **2.4** (B) `src/cnf_reader.cpp` — `readDimacs`, `cnfToInfix` (LEFT_DEEP + BALANCED).
  *Accept:* `tests/data/tiny.cnf` → case E string; handles `c`, `%`, multi-line clauses; balanced height ≈ log₂(#clauses) + clause depth.
- [x] **2.5** (C) `src/traversal.cpp` — T3, all 6 functions.
  *Accept:* `inorder/preorder/postorder` match; rec == iter on all cases.
- [x] **2.6** (D) `src/height.cpp` — T4 rec + BFS.
  *Accept:* `height:` matches; rec == iter.
- [x] **2.7** (D) `src/eval.cpp` — T5 `collectAtoms` (natural sort), `evaluateRec`, `evaluateIter`, `printTruthTable`, `truthTableColumn`.
  *Accept:* `atoms:` and `table:` match; case I orders `x2 x10`; k > maxAtoms prints the refusal message.
- [x] **2.8** (A) `src/main.cpp` — CLI per SPEC §5.
  *Accept:* `./logic_a1 --formula "((p+(~q))>r)" --table` prints all sections; `--cnf tests/data/tiny.cnf` works.

## Phase 3 — Integration and stress (Wed morning)

- [x] **3.1** (C) `make test` is fully green with `-Wall -Wextra` and zero warnings.
- [x] **3.2** (B) Run every SAT file in `data/` through the CLI (LEFT_DEEP and BALANCED). Note which recursive functions crash, and at what height.
- [x] **3.3** (D) Add a random formula generator (`tests/gen.cpp` or in bench) and a property test: for 1000 random formulas, generated in canonical form (SPEC §4.3), `inorder(prefixToTree(infixToPrefix(tokenize(s)))) == s`, and rec == iter everywhere.
- [x] **3.4** (A) Run AddressSanitizer: `make CXXFLAGS+="-fsanitize=address -g" test` must be clean.

## Phase 4 — Analysis and docs (Wed → Thu)

- [x] **4.1** (D) `bench/bench.cpp` per SPEC §8.2 (chrono median of 5, aux-peak counters, `ru_maxrss`, `fork()` for recursive crash detection) → `bench/results.csv` with the exact columns from §8.3.
- [x] **4.2** (D) Run the benchmarks on the synthetic sizes plus ≥6 SAT files, and commit `results.csv`.
- [x] **4.3** (C) `docs/Doxyfile` per SPEC §9; `make docs` produces `docs/html/index.html` with every function documented (no Doxygen warnings).
- [x] **4.4** (A) `README.md`: build, run, test, file map, team, and the SAT files used.
- [ ] **4.5** (C+D) `docs/report/` — `index`, `algorithms` (worked trace of case A), `results` (tables + plots from the CSV), `analysis` (all of §8.4), `lean`, plus a shared `style.css`. Open each page on desktop and phone width.

## Phase 5 — Lean, Part B (E leads from Tue; everyone practices Thu)

- [x] **5.1** (E) Set up Lean: open `lean/Examples.lean` in VS Code and confirm the Infoview shows goals.
- [x] **5.2** (E) Copy Examples 1.13–1.18 **exactly** from the textbook into theorem stubs ending in `sorry`. Have a second person verify each statement against the book.
- [x] **5.3** (E) Prove each one. Mark which ones need `Classical`, and comment each step with its natural-deduction rule.
  *Accept:* the file compiles; `grep sorry lean/Examples.lean` returns nothing.
- [x] **5.4** (E) `lean/RULES.md` — the rule → tactic cheat sheet (SPEC §10).
- [ ] **5.5** (everyone) Each member proves ≥3 sequents in `lean/Practice.lean` **without help**, for the 7-mark live demo. Suggested: ¬¬p ⊢ p, p→q ⊢ ¬q→¬p, ⊢ p∨¬p, p∧q→r ⊢ p→(q→r), (p∨q)∧(p∨r) ⊢ p∨(q∧r).

## Phase 6 — Submit and demo (Thu)

- [ ] **6.1** Fresh clone into an empty folder → `make && make test && make docs` must all pass.
- [ ] **6.2** Build the zip per SPEC §11, named `<BITSID>_LOGIC_A1.zip`, then unzip it and rebuild once to confirm.
- [ ] **6.3** Upload to LMS (one person) and confirm the upload in the group chat.
- [ ] **6.4** Demo rehearsal: every member can (a) trace case A by hand through T1→T5, (b) explain recursive vs iterative stack use, (c) explain why the truth table is infeasible for SAT files, and (d) do one live Lean proof.

---

## Blockers / notes log

| Date | Who | Note |
|---|---|---|
| | | |
