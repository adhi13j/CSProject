# GETTING_STARTED.md — Project kickoff

Do these steps once, in order. One person (the "repo owner") does steps 2–4; everyone does steps 1 and 5.

## 1. Install tools (everyone)

| Tool | Check | Notes |
|---|---|---|
| g++ (C++17) | `g++ --version` | Windows: use WSL (Ubuntu) — it also makes `fork()`/`getrusage` work for benchmarks |
| make | `make --version` | |
| git | `git --version` | |
| doxygen | `doxygen --version` | Linux: `sudo apt install doxygen` · macOS: `brew install doxygen` |
| VS Code + Lean 4 extension | ∀ icon appears | Follow the install steps in the assignment PDF (elan) |
| Claude Code (optional) | `claude --version` | Install guide: https://docs.claude.com/en/docs/claude-code/overview |

## 2. Create the skeleton (repo owner)

```bash
mkdir logic_a1 && cd logic_a1
git init
mkdir -p include src tests/golden tests/data bench docs/report lean data
touch include/formula.h src/{tokenizer,infix_prefix,tree_build,traversal,height,eval,cnf_reader,main}.cpp
touch tests/run_tests.cpp bench/bench.cpp README.md
printf 'c tiny example\np cnf 3 2\n1 -2 0\n2 3 0\n' > tests/data/tiny.cnf
printf '*.o\nlogic_a1\ntests/run_tests\nbench/bench\ndocs/html/\n.lake/\ndata/*.cnf\n' > .gitignore
```

Now copy `SPEC.md`, `TODO.md`, `CLAUDE.md`, and this file into the folder root.

## 3. Create the golden test files (repo owner)

Make one file per case in SPEC Appendix A: `tests/golden/A.txt` … `I.txt`. Copy each code block exactly, in `key: value` lines.

## 4. First commit and share

```bash
git add . && git commit -m "Project skeleton, spec v1.0, golden tests"
git branch -M main
git remote add origin <your-private-repo-url>
git push -u origin main
```

Then add all group members as collaborators.

## 5. Team workflow (everyone)

- Clone the repo, then **read SPEC.md fully before writing any code.**
- Work on a branch named after your TODO item: `git checkout -b t2.3-tree-build`.
- Before you push: `make test` must not break any test that already passed.
- Open a pull request (or simply merge, if you're short on time); someone else glances over it.
- Tick the item in TODO.md in the same commit that completes it.
- Spec change needed? Raise it in the group chat **first**, then edit SPEC.md in a separate commit titled `SPEC: …`.

## 6. Kickoff with Claude Code (optional)

From the repo root, run `claude`. Then use these prompts in order; let each one finish and review the result before moving on.

**Prompt 1 (plan mode — press Shift+Tab first):**
> Read SPEC.md, TODO.md and CLAUDE.md. Summarise the plan in 10 lines and list any contradictions or gaps in the spec. Don't edit anything.

**Prompt 2 (TODO 1.2):**
> Do TODO 1.2: write include/formula.h exactly per SPEC §3–4, with full Doxygen blocks and no implementations. Check that it compiles with -fsyntax-only.

**Prompt 3 (TODO 1.4 + 1.5):**
> Do TODO 1.4 and 1.5: write tests/run_tests.cpp to load tests/golden/*.txt and check every key plus the invariants in SPEC §4.3–4.5, and write the Makefile from SPEC §6. Stub each function in src/ so it throws "not implemented". Run make test; it should build and report failures.

**Prompt 4 onward:**
> Do TODO 2.x. Run make test and show me the result.

Use one prompt per item, and commit after each green run.

## 7. Kickoff meeting agenda (30 min, Mon night)

1. Walk through SPEC.md together, and agree on the §12 answers (10 min).
2. Each person hand-traces one golden case (10 min). This is also demo practice.
3. Assign the A–E owners in TODO.md and commit them (5 min).
4. Agree on a check-in time for Tue night and Wed night (5 min).
