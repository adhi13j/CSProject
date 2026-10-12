# Natural deduction rules → Lean 4

A sequent `P, Q ⊢ R` is the theorem `P → Q → R`. Always start with `intro` for the premises.
Everything below is core Lean 4 (no Mathlib). Check a file with `lean lean/Examples.lean`.

| Rule | Meaning | Lean tactic | Lean term |
|---|---|---|---|
| **∧i** | from `p` and `q` get `p ∧ q` | `constructor` (then one bullet `·` per half) | `And.intro hp hq` or `⟨hp, hq⟩` |
| **∧e₁** | from `p ∧ q` get `p` | `have hp := h.left` | `h.left` (`And.left h`) |
| **∧e₂** | from `p ∧ q` get `q` | `have hq := h.right` | `h.right` (`And.right h`) |
| **∨i₁** | from `p` get `p ∨ q` | `apply Or.inl` | `Or.inl hp` |
| **∨i₂** | from `q` get `p ∨ q` | `apply Or.inr` | `Or.inr hq` |
| **∨e** | from `p ∨ q`, a proof from `p` and a proof from `q`, get the goal | `rcases h with hp \| hq` (two bullets) | `Or.elim h (fun hp => …) (fun hq => …)` |
| **→i** | assume `p`, prove `q`, get `p → q` | `intro hp` | `fun hp => …` |
| **→e** (modus ponens) | from `p → q` and `p` get `q` | `apply h` (goal becomes `p`), or `have := h hp` | `h hp` |
| **¬i** | assume `p`, derive `⊥`, get `¬p` | `intro hp` (`¬p` unfolds to `p → False`) | `fun hp => …` |
| **¬e** | from `p` and `¬p` get `⊥` | `have hbot : False := hnp hp` | `hnp hp` |
| **⊥e** | from `⊥` get anything | `exact False.elim hbot` (or `contradiction`) | `False.elim hbot`, `absurd hp hnp` |
| **¬¬e** | from `¬¬p` get `p` (classical) | `apply Classical.byContradiction` | `Classical.byContradiction (fun hnp => h hnp)` |
| **MT** | from `p → q` and `¬q` get `¬p` | `intro hp; exact hnq (hpq hp)` | `fun hp => hnq (hpq hp)` |
| **PBC** | assume `¬p`, derive `⊥`, get `p` (classical) | `apply Classical.byContradiction; intro hnp` | `Classical.byContradiction (fun hnp => …)` |
| **LEM** | `p ∨ ¬p` (classical) | `rcases Classical.em p with hp \| hnp` | `Classical.em p` |
| premise / assumption | use a hypothesis | `exact h` or `assumption` | `h` |

## Reading a Lean proof as natural deduction

* `intro` introduces an assumption (**→i**, **¬i**, or just taking a premise).
* `exact t` closes the goal with the term `t`; the term is the proof tree written inline.
* `apply h` works backwards: it replaces the goal `q` by the goal `p` when `h : p → q` (**→e**).
* `constructor` on a goal `p ∧ q` is **∧i** and creates two goals; on `p ↔ q` it creates two implications.
* `rcases h with a | b` is a proof by cases (**∨e**); `rcases h with ⟨a, b⟩` splits an `∧` (**∧e₁, ∧e₂**).
* Bullets `·` focus on one goal at a time, one bullet per case or conjunct.

## Constructive or classical?

Lean's core logic is constructive. These rules need the classical axioms and are written with
`Classical.*`: **¬¬e**, **PBC**, **LEM**. All the others hold constructively.

| Theorem | Rules used | Classical? |
|---|---|---|
| `ex1_13` p∧q→r ⊢ p→(q→r) | →i, →e, ∧i | no |
| `ex1_14` p→(q→r) ⊢ p∧q→r | →i, ∧e₁, ∧e₂, →e | no |
| `ex1_15` p∨q ⊢ q∨p | ∨e, ∨i₁, ∨i₂ | no |
| `ex1_16` ¬p∨q ⊢ p→q | →i, ∨e, ¬e, ⊥e | no |
| `ex1_17` p→q ⊢ ¬p∨q | LEM, ∨e, →e, ∨i₁, ∨i₂ | **yes (LEM)** |
| `ex1_18` ¬(p∨q) ⊢ ¬p∧¬q | ∧i, ¬i, ∨i₁, ∨i₂, ¬e | no |

## Template for the live demo

```lean
theorem demo (p q : Prop) : <premise 1> → <premise 2> → <conclusion> := by
  intro h1 h2          -- take the premises
  -- look at the shape of the goal:
  --   p → q    : intro                 p ∧ q : constructor
  --   p ∨ q    : apply Or.inl / Or.inr ¬p     : intro (then derive False)
  -- look at the premises you have:
  --   h : p ∧ q : h.left, h.right      h : p ∨ q : rcases h with a | b
  --   h : p → q : h hp                 h : ¬p    : h hp : False
  -- stuck on a negation or a disjunction? use Classical.em p or Classical.byContradiction
```
