/-
  Practice sequents for the live demo (Part B, Task 2).  Core Lean only, no imports.

  Study each proof, then DELETE the proof of one and rebuild it from memory using lean/RULES.md.
  Every member should be able to prove at least three of these without help.
  Each step names the natural-deduction rule it implements.
-/

-- ¬¬p ⊢ p                                   (¬¬e: needs classical reasoning)
theorem dne (p : Prop) : ¬¬p → p := by
  intro h                                    -- premise  h : ¬¬p
  apply Classical.byContradiction            -- PBC: to show p, assume ¬p and derive ⊥
  intro hnp                                  --   assume hnp : ¬p
  exact h hnp                                --   ¬e: h : ¬¬p applied to ¬p gives ⊥

-- ⊢ p ∨ ¬p                                  (LEM)
theorem lem (p : Prop) : p ∨ ¬p := by
  exact Classical.em p                       -- LEM

-- p → q ⊢ ¬q → ¬p                           (MT: modus tollens)
theorem mt_demo (p q : Prop) : (p → q) → (¬q → ¬p) := by
  intro hpq                                  -- premise  hpq : p → q
  intro hnq                                  -- →i: assume ¬q
  intro hp                                   -- ¬i: assume p, derive ⊥
  exact hnq (hpq hp)                         --   →e gives q, then ¬e with ¬q gives ⊥

-- ¬q → ¬p ⊢ p → q                           (contrapositive; PBC)
theorem contra (p q : Prop) : (¬q → ¬p) → (p → q) := by
  intro h                                    -- premise  h : ¬q → ¬p
  intro hp                                   -- →i: assume p, goal is q
  apply Classical.byContradiction            -- PBC: assume ¬q and derive ⊥
  intro hnq                                  --   hnq : ¬q
  exact h hnq hp                             --   →e gives ¬p, then ¬e with p gives ⊥

-- p → q, q → r ⊢ p → r                      (chain of →e)
theorem syllogism (p q r : Prop) : (p → q) → (q → r) → (p → r) := by
  intro hpq hqr                              -- premises
  intro hp                                   -- →i: assume p
  exact hqr (hpq hp)                         -- →e twice

-- (p ∨ q) ∧ (p ∨ r) ⊢ p ∨ (q ∧ r)
theorem distrib_or (p q r : Prop) : (p ∨ q) ∧ (p ∨ r) → p ∨ (q ∧ r) := by
  intro h                                    -- premise
  have hpq : p ∨ q := h.left                 -- ∧e₁
  have hpr : p ∨ r := h.right                -- ∧e₂
  rcases hpq with hp | hq                    -- ∨e on p ∨ q
  · exact Or.inl hp                          --   case p: ∨i₁
  · rcases hpr with hp | hr                  --   case q: ∨e on p ∨ r
    · exact Or.inl hp                        --     case p: ∨i₁
    · exact Or.inr (And.intro hq hr)         --     case r: ∧i gives q ∧ r, ∨i₂ gives p ∨ (q ∧ r)

-- p ∧ (q ∨ r) ⊢ (p ∧ q) ∨ (p ∧ r)
theorem distrib_and (p q r : Prop) : p ∧ (q ∨ r) → (p ∧ q) ∨ (p ∧ r) := by
  intro h                                    -- premise
  have hp : p := h.left                      -- ∧e₁
  have hqr : q ∨ r := h.right                -- ∧e₂
  rcases hqr with hq | hr                    -- ∨e on q ∨ r
  · exact Or.inl (And.intro hp hq)           --   case q: ∧i, then ∨i₁
  · exact Or.inr (And.intro hp hr)           --   case r: ∧i, then ∨i₂

-- ¬(p ∧ q) ⊢ ¬p ∨ ¬q                        (classical: LEM)
theorem de_morgan (p q : Prop) : ¬(p ∧ q) → ¬p ∨ ¬q := by
  intro h                                    -- premise  h : ¬(p ∧ q)
  rcases Classical.em p with hp | hnp        -- LEM, then ∨e
  · apply Or.inr                             --   case p: aim for ¬q (∨i₂)
    intro hq                                 --     ¬i: assume q, derive ⊥
    exact h (And.intro hp hq)                --       ∧i gives p ∧ q, ¬e with h gives ⊥
  · exact Or.inl hnp                         --   case ¬p: ∨i₁

-- p ∧ ¬p ⊢ q                                (⊥e after ¬e)
theorem explosion (p q : Prop) : p ∧ ¬p → q := by
  intro h                                    -- premise  h : p ∧ ¬p
  have hbot : False := h.right h.left        -- ¬e: ¬p applied to p gives ⊥
  exact False.elim hbot                      -- ⊥e

-- p ∧ q ⊢ q ∧ p                             (∧e, ∧i)
theorem and_comm' (p q : Prop) : p ∧ q → q ∧ p := by
  intro h                                    -- premise
  constructor                                -- ∧i
  · exact h.right                            --   ∧e₂ gives q
  · exact h.left                             --   ∧e₁ gives p
