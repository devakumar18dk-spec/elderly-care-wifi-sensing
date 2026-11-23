# Your RAG idea, and the upgrades that are actually worth building

## 1. Is your RAG idea right?
Your instinct is correct, and the kit already does it. "Remember every Wi-Fi change together with its action label, then answer a new
movement by finding the most similar remembered ones and reporting how confident that is" is exactly what the kit's kNN does:
it stores labelled 156-number windows (movement.npz) and votes among the 7 nearest.
That is retrieval-based prediction. What it is NOT is RAG in the usual sense. RAG = retrieve text, then an LLM writes an answer.
An LLM adds nothing to classifying a 156-number vector, and would be slower and less reliable than kNN.

Where an LLM + retrieval genuinely helps: caregiver questions over the event log ("how long was she in bed last night?",
"was today different from the last 7 days?"). That is a summary layer on top of the classifier. Optional, and it must only quote the log.

## 2. What to build instead (purposeful, in priority order)
1. "Unfamiliar" state (BUILT in care/server_patch/server_changes.patch). Retrieval tells you how close the nearest memories are.
   If the live window is far from everything learned (distance > 1.5x the normal distance between different recording rounds),
   say "unfamiliar" instead of forcing a class. This is the part of your RAG idea that adds real value: knowing when NOT to answer.
   This is also your furniture/board-moved detector (gap G1).
2. Recalibration prompt. When "unfamiliar" persists > 1 min, ask for a 2-minute re-calibration and ADD it as a new round
   (never overwrite movement.npz). Over time memory covers several layouts (G4).
3. Fix stale-feature reuse (patched: counter + KIT_STRICT_FRESH=1 switch). The kit reuses a link's feature up to 3 s old when a link
   is slow. On your real boards (1.5-4 PPS) this may pair old data with new labels. Measure it on hardware before training.
4. Evaluate by session, not random windows. Train rounds on different days/times, test on another. Report that number.
5. Persistence-based notices (UI has them, wording non-medical): lying > N min, nobody > N min, unfamiliar > 1 min.
6. Link-quality gate: do not trust any classification when fewer than the trained links stream (UI shows per-link rate).
7. Optional: LLM caregiver summary over the event log only.

## 3. What the synthetic run shows, and does not show
Run on the real patched kit server with a SYNTHETIC simulator (fake frames, no hardware):
- Held-out test session 100%, leave-one-round-out 100%. The simulator is easy, so this only proves the plumbing works. It says NOTHING about real accuracy.
- Layout shifted (5 of 9 links changed): 100% of windows flagged "unfamiliar" (median novelty 2.06 vs 0.99 in the learned layout),
  0% flagged in the learned layout, and back to 0% after the layout returned. So the detector triggers on a shift and not on normal use, in this simulation.
- In this simulator, the shift did not break the class vote itself, so this run does not show that "unfamiliar" prevents wrong answers. Needs real data.
- Stale reuse was 0% because the simulator sends 10 PPS per link; it will matter at your real 1.5-4 PPS.
- A dropped link makes the page show "Checking..." (kit waits for the trained links).
