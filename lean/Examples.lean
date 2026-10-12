/-
  CS F214 Assignment 1, Part B, Task 1: Examples 1.13 - 1.18.

  Each sequent  P1, ..., Pn ⊢ C  is written as the curried theorem  P1 → ... → Pn → C
  (as in the assignment PDF: `theorem and_intro (P Q : Prop) : P → Q → P ∧ Q`).

  NOTE (SPEC §12.5): the group decided that these six sequents are self-made / taken from the
  group's own question list, because the textbook text was not available when this file was written.
  Before the demo, compare each statement below with the printed Examples 1.13 - 1.18 and change it
  ONLY if the textbook differs.  The statements are never changed to make a proof easier.

  Spec-first: the statements below are committed with `sorry`; proofs are filled in afterwards.
-/

-- Example 1.13:  p ∧ q → r  ⊢  p → (q → r)
theorem ex1_13 (p q r : Prop) : (p ∧ q → r) → (p → (q → r)) := by
  sorry

-- Example 1.14:  p → (q → r)  ⊢  p ∧ q → r
theorem ex1_14 (p q r : Prop) : (p → (q → r)) → (p ∧ q → r) := by
  sorry

-- Example 1.15:  p ∨ q  ⊢  q ∨ p
theorem ex1_15 (p q : Prop) : p ∨ q → q ∨ p := by
  sorry

-- Example 1.16:  ¬p ∨ q  ⊢  p → q
theorem ex1_16 (p q : Prop) : ¬p ∨ q → (p → q) := by
  sorry

-- Example 1.17:  p → q  ⊢  ¬p ∨ q          (needs classical reasoning: LEM)
theorem ex1_17 (p q : Prop) : (p → q) → (¬p ∨ q) := by
  sorry

-- Example 1.18:  ¬(p ∨ q)  ⊢  ¬p ∧ ¬q
theorem ex1_18 (p q : Prop) : ¬(p ∨ q) → (¬p ∧ ¬q) := by
  sorry
