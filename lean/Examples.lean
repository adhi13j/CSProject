/-
  CS F214 Assignment 1, Part B: practice sequents for the live demo (Task 2).

  Same conventions as Examples.lean:
    * a sequent  P1, ..., Pn ⊢ C  is the curried theorem  P1 → ... → Pn → C
    * every step is annotated with its natural-deduction rule
    * (classical) marks proofs that need LEM / PBC / ¬¬e

  Grouped by the rule each one mainly exercises, so you can drill one rule at a time.
-/

/-! ## Group 1: ∧ and → (all constructive) -/

-- P01:  p ∧ q, r  ⊢  q ∧ r
theorem pr01 (p q r : Prop) : p ∧ q → r → q ∧ r := by
  intro hpq hr                 -- premises  hpq : p ∧ q,  hr : r
  constructor                  -- ∧i
  · exact hpq.right            --   ∧e₂ on p ∧ q gives q
  · exact hr                   --   premise r

-- P02:  (p ∧ q) ∧ r, s ∧ t  ⊢  q ∧ s
theorem pr02 (p q r s t : Prop) : (p ∧ q) ∧ r → s ∧ t → q ∧ s := by
  intro h1 h2                  -- premises
  constructor                  -- ∧i
  · exact h1.left.right        --   ∧e₁ gives p ∧ q, then ∧e₂ gives q
  · exact h2.left              --   ∧e₁ on s ∧ t gives s

-- P03:  p → (q → r), p → q, p  ⊢  r
theorem pr03 (p q r : Prop) : (p → (q → r)) → (p → q) → p → r := by
  intro h1 h2 hp               -- premises
  have hq : q := h2 hp         -- →e on p → q and p
  have hqr : q → r := h1 hp    -- →e on p → (q → r) and p
  exact hqr hq                 -- →e on q → r and q

-- P04:  ⊢  p → p
theorem pr04 (p : Prop) : p → p := by
  intro hp                     -- →i: assume p
  exact hp                     -- the assumption closes the box

-- P05:  q → r  ⊢  (p → q) → (p → r)
theorem pr05 (p q r : Prop) : (q → r) → (p → q) → (p → r) := by
  intro hqr                    -- premise
  intro hpq                    -- →i (outer): assume p → q
  intro hp                     -- →i (inner): assume p
  exact hqr (hpq hp)           -- →e twice: p gives q, q gives r

-- P06:  (p → q) ∧ (p → r)  ⊢  p → q ∧ r
theorem pr06 (p q r : Prop) : ((p → q) ∧ (p → r)) → (p → q ∧ r) := by
  intro h                      -- premise
  intro hp                     -- →i: assume p
  constructor                  -- ∧i
  · exact h.left hp            --   ∧e₁ then →e gives q
  · exact h.right hp           --   ∧e₂ then →e gives r

/-! ## Group 2: ∨ (all constructive) -/

-- P07:  p ∧ (q ∨ r)  ⊢  (p ∧ q) ∨ (p ∧ r)
theorem pr07 (p q r : Prop) : p ∧ (q ∨ r) → (p ∧ q) ∨ (p ∧ r) := by
  intro h                      -- premise
  have hp : p := h.left        -- ∧e₁
  rcases h.right with hq | hr  -- ∧e₂ gives q ∨ r, then ∨e
  · exact Or.inl ⟨hp, hq⟩      --   case q: ∧i, then ∨i₁
  · exact Or.inr ⟨hp, hr⟩      --   case r: ∧i, then ∨i₂

-- P08:  (p ∧ q) ∨ (p ∧ r)  ⊢  p ∧ (q ∨ r)
theorem pr08 (p q r : Prop) : (p ∧ q) ∨ (p ∧ r) → p ∧ (q ∨ r) := by
  intro h                      -- premise
  rcases h with hpq | hpr      -- ∨e
  · exact ⟨hpq.left, Or.inl hpq.right⟩   -- case p ∧ q: ∧e₁, ∧e₂, ∨i₁, ∧i
  · exact ⟨hpr.left, Or.inr hpr.right⟩   -- case p ∧ r: ∧e₁, ∧e₂, ∨i₂, ∧i

-- P09:  p ∨ (q ∧ r)  ⊢  (p ∨ q) ∧ (p ∨ r)
theorem pr09 (p q r : Prop) : p ∨ (q ∧ r) → (p ∨ q) ∧ (p ∨ r) := by
  intro h                      -- premise
  rcases h with hp | hqr       -- ∨e
  · exact ⟨Or.inl hp, Or.inl hp⟩              -- case p: ∨i₁ twice, ∧i
  · exact ⟨Or.inr hqr.left, Or.inr hqr.right⟩ -- case q ∧ r: ∧e₁/∧e₂, ∨i₂, ∧i

-- P10:  (p ∨ q) ∧ (p ∨ r)  ⊢  p ∨ (q ∧ r)
theorem pr10 (p q r : Prop) : (p ∨ q) ∧ (p ∨ r) → p ∨ (q ∧ r) := by
  intro h                      -- premise
  rcases h.left with hp | hq   -- ∧e₁, then ∨e on p ∨ q
  · exact Or.inl hp            --   case p: ∨i₁
  · rcases h.right with hp | hr  -- case q: ∧e₂, then ∨e on p ∨ r
    · exact Or.inl hp          --     case p: ∨i₁
    · exact Or.inr ⟨hq, hr⟩    --     case r: ∧i with q, then ∨i₂

/-! ## Group 3: ¬ and ⊥ (constructive) -/

-- P11 (Modus Tollens):  p → q, ¬q  ⊢  ¬p
theorem pr11 (p q : Prop) : (p → q) → ¬q → ¬p := by
  intro h hnq                  -- premises
  intro hp                     -- ¬i: assume p, goal ⊥
  exact hnq (h hp)             -- →e gives q, ¬e with ¬q gives ⊥

-- P12:  p → ¬q, q  ⊢  ¬p
theorem pr12 (p q : Prop) : (p → ¬q) → q → ¬p := by
  intro h hq                   -- premises
  intro hp                     -- ¬i: assume p
  exact (h hp) hq              -- →e gives ¬q, ¬e with q gives ⊥

-- P13 (¬¬i):  p  ⊢  ¬¬p
theorem pr13 (p : Prop) : p → ¬¬p := by
  intro hp                     -- premise
  intro hnp                    -- ¬i: assume ¬p
  exact hnp hp                 -- ¬e on p and ¬p gives ⊥

-- P14 (⊥e):  ⊥  ⊢  p
theorem pr14 (p : Prop) : False → p := by
  intro hbot                   -- premise ⊥
  exact False.elim hbot        -- ⊥e

-- P15:  ¬p ∨ ¬q  ⊢  ¬(p ∧ q)
theorem pr15 (p q : Prop) : ¬p ∨ ¬q → ¬(p ∧ q) := by
  intro h                      -- premise
  intro hpq                    -- ¬i: assume p ∧ q
  rcases h with hnp | hnq      -- ∨e
  · exact hnp hpq.left         --   case ¬p: ∧e₁, ¬e
  · exact hnq hpq.right        --   case ¬q: ∧e₂, ¬e

-- P16:  ¬p ∧ ¬q  ⊢  ¬(p ∨ q)
theorem pr16 (p q : Prop) : ¬p ∧ ¬q → ¬(p ∨ q) := by
  intro h                      -- premise
  intro hpq                    -- ¬i: assume p ∨ q
  rcases hpq with hp | hq      -- ∨e
  · exact h.left hp            --   case p: ∧e₁ gives ¬p, ¬e
  · exact h.right hq           --   case q: ∧e₂ gives ¬q, ¬e

/-! ## Group 4: classical (LEM, PBC, ¬¬e) -/

-- P17 (¬¬e):  ¬¬p  ⊢  p                                  (classical)
theorem pr17 (p : Prop) : ¬¬p → p := by
  intro hnnp                   -- premise
  apply Classical.byContradiction  -- PBC: assume ¬p, goal ⊥
  intro hnp
  exact hnnp hnp               -- ¬e on ¬p and ¬¬p

-- P18 (LEM, proved the textbook way via PBC):  ⊢  p ∨ ¬p   (classical)
theorem pr18 (p : Prop) : p ∨ ¬p := by
  apply Classical.byContradiction  -- PBC: assume ¬(p ∨ ¬p)
  intro h
  have hnp : ¬p := by
    intro hp                   -- ¬i: assume p
    exact h (Or.inl hp)        -- ∨i₁ gives p ∨ ¬p, ¬e with h
  exact h (Or.inr hnp)         -- ∨i₂ gives p ∨ ¬p, ¬e with h

-- P19:  p, ¬¬(q ∧ r)  ⊢  ¬¬p ∧ r                          (classical)
theorem pr19 (p q r : Prop) : p → ¬¬(q ∧ r) → ¬¬p ∧ r := by
  intro hp h                   -- premises
  constructor                  -- ∧i
  · intro hnp                  --   ¬¬i: assume ¬p
    exact hnp hp               --     ¬e
  · have hqr : q ∧ r := Classical.byContradiction h  -- ¬¬e on ¬¬(q ∧ r)
    exact hqr.right            --   ∧e₂

-- P20 (De Morgan):  ¬(p ∧ q)  ⊢  ¬p ∨ ¬q                  (classical)
theorem pr20 (p q : Prop) : ¬(p ∧ q) → ¬p ∨ ¬q := by
  intro h                      -- premise
  rcases Classical.em p with hp | hnp  -- LEM on p, then ∨e
  · apply Or.inr               --   case p: aim for ¬q (∨i₂)
    intro hq                   --     ¬i: assume q
    exact h ⟨hp, hq⟩           --     ∧i, then ¬e with h
  · exact Or.inl hnp           --   case ¬p: ∨i₁

-- P21 (converse of contrapositive):  ¬q → ¬p  ⊢  p → q    (classical)
theorem pr21 (p q : Prop) : (¬q → ¬p) → (p → q) := by
  intro h                      -- premise
  intro hp                     -- →i: assume p
  apply Classical.byContradiction  -- PBC: assume ¬q
  intro hnq
  exact (h hnq) hp             -- →e gives ¬p, ¬e with p

-- P22 (case analysis):  p → q, ¬p → q  ⊢  q              (classical)
theorem pr22 (p q : Prop) : (p → q) → (¬p → q) → q := by
  intro h1 h2                  -- premises
  rcases Classical.em p with hp | hnp  -- LEM, ∨e
  · exact h1 hp                --   case p: →e
  · exact h2 hnp               --   case ¬p: →e

-- P23 (Peirce's law):  ⊢  ((p → q) → p) → p              (classical)
theorem pr23 (p q : Prop) : ((p → q) → p) → p := by
  intro h                      -- →i: assume (p → q) → p
  rcases Classical.em p with hp | hnp  -- LEM, ∨e
  · exact hp                   --   case p: done
  · apply h                    --   case ¬p: →e, so it suffices to prove p → q
    intro hp                   --     →i: assume p
    exact absurd hp hnp        --     ¬e gives ⊥, then ⊥e gives q