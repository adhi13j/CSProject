/-
  CS F214 Assignment 1, Part B, Task 1: Examples 1.13 - 1.18.

  Each sequent  P1, ..., Pn ⊢ C  is written as the curried theorem  P1 → ... → Pn → C
  (as in the assignment PDF: `theorem and_intro (P Q : Prop) : P → Q → P ∧ Q`).

  NOTE (SPEC §12.5): the group decided that these six sequents are self-made / taken from the
  group's own question list, because the textbook text was not available when this file was written.
  Before the demo, compare each statement below with the printed Examples 1.13 - 1.18 and change it
  ONLY if the textbook differs.  The statements are never changed to make a proof easier.

  Every step is annotated with the natural-deduction rule it implements
  (∧i ∧e₁ ∧e₂ ∨i₁ ∨i₂ ∨e →i →e ¬i ¬e ⊥e LEM).  Lean tactic <-> rule table: lean/RULES.md.
  Only Example 1.17 needs classical reasoning (LEM); the rest are constructive.
-/

-- Example 1.13:  p ∧ q → r  ⊢  p → (q → r)
theorem ex1_13 (p q r : Prop) : (p ∧ q → r) → (p → (q → r)) := by
  intro h                      -- premise  h : p ∧ q → r
  intro hp                     -- →i (outer): assume p
  intro hq                     -- →i (inner): assume q
  apply h                      -- →e: h turns a proof of p ∧ q into r, so it is enough to prove p ∧ q
  constructor                  -- ∧i: prove both halves of p ∧ q
  · exact hp                   --   left half: the assumption p
  · exact hq                   --   right half: the assumption q

-- Example 1.14:  p → (q → r)  ⊢  p ∧ q → r
theorem ex1_14 (p q r : Prop) : (p → (q → r)) → (p ∧ q → r) := by
  intro h                      -- premise  h : p → (q → r)
  intro hpq                    -- →i: assume p ∧ q
  have hp : p := hpq.left      -- ∧e₁: first component of p ∧ q
  have hq : q := hpq.right     -- ∧e₂: second component of p ∧ q
  have hqr : q → r := h hp     -- →e: from p → (q → r) and p
  exact hqr hq                 -- →e: from q → r and q we get r

-- Example 1.15:  p ∨ q  ⊢  q ∨ p
theorem ex1_15 (p q : Prop) : p ∨ q → q ∨ p := by
  intro h                      -- premise  h : p ∨ q
  rcases h with hp | hq        -- ∨e: split into the case p and the case q
  · exact Or.inr hp            --   case p: ∨i₂ gives q ∨ p
  · exact Or.inl hq            --   case q: ∨i₁ gives q ∨ p

-- Example 1.16:  ¬p ∨ q  ⊢  p → q
theorem ex1_16 (p q : Prop) : ¬p ∨ q → (p → q) := by
  intro h                      -- premise  h : ¬p ∨ q
  intro hp                     -- →i: assume p, goal is q
  rcases h with hnp | hq       -- ∨e: split into the case ¬p and the case q
  · have hbot : False := hnp hp  --   case ¬p: ¬e applied to p and ¬p gives ⊥
    exact False.elim hbot        --   ⊥e: from ⊥ anything follows, in particular q
  · exact hq                   --   case q: the goal itself

-- Example 1.17:  p → q  ⊢  ¬p ∨ q          (classical: uses LEM)
theorem ex1_17 (p q : Prop) : (p → q) → (¬p ∨ q) := by
  intro h                      -- premise  h : p → q
  rcases Classical.em p with hp | hnp   -- LEM: p ∨ ¬p, then ∨e on it
  · exact Or.inr (h hp)        --   case p: →e gives q, ∨i₂ gives ¬p ∨ q
  · exact Or.inl hnp           --   case ¬p: ∨i₁ gives ¬p ∨ q

-- Example 1.18:  ¬(p ∨ q)  ⊢  ¬p ∧ ¬q
theorem ex1_18 (p q : Prop) : ¬(p ∨ q) → (¬p ∧ ¬q) := by
  intro h                      -- premise  h : ¬(p ∨ q)
  constructor                  -- ∧i: prove ¬p and ¬q separately
  · intro hp                   --   ¬i: assume p, derive ⊥
    exact h (Or.inl hp)        --     ∨i₁ gives p ∨ q, then ¬e with h gives ⊥
  · intro hq                   --   ¬i: assume q, derive ⊥
    exact h (Or.inr hq)        --     ∨i₂ gives p ∨ q, then ¬e with h gives ⊥
