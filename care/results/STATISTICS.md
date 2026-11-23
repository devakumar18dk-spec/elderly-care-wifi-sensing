# Statistics — SYNTHETIC data only

**Read this first.** Every number below comes from the synthetic simulator (`care/sim/`), not from real boards or people. They show that the software chain works end to end.
They are **not** evidence of real-world accuracy, which has not been measured yet.

## Set-up
Real `kit/server.py` (with our changes) fed by `care/sim/kit_sim.py`: 3 boards + router = 9 links, 10 frames/s per link, 5 postures. Camera teacher replaced by `teacher_sim.py`.
Protocol (`care/sim/drive_eval.py`): 3 training rounds (5 postures x 20 s each, shuffled), 1 separate test session (5 x 14 s), then 3 live phases (5 x 10 s each):
layout A (learned) -> layout B (5 of 9 links altered, "furniture moved") -> layout A again. First 3 s after every posture change are excluded.

## Results
| Measure | Value |
|---|---|
| Held-out test session accuracy (kNN, k=7) | 100% (all 5 classes 100%) |
| Leave-one-round-out accuracy | 100% |
| Live, learned layout A: windows flagged "unfamiliar" | 0 of 450 (0.0%) |
| Live, layout B (moved): windows flagged "unfamiliar" | 490 of 490 (100%) |
| Live, layout A again: windows flagged "unfamiliar" | 0 of 488 (0.0%) |
| Novelty score, layout A (median, 5th–95th pct) | 0.99 (0.96–1.03) |
| Novelty score, layout B | 2.06 (2.02–2.09) |
| Novelty score, layout A again | 0.98 (0.96–1.03) |
| Threshold (`KIT_UNFAMILIAR`) | 1.5 x the normal distance between different recording rounds |
| Stale-feature reuse | 0% (simulator sends 10/s per link; expected to matter at the 1.5–4/s seen on real boards) |
| Pose rules, synthetic landmark test (`care/tests`) | 6 of 6 cases (nobody, standing, lying, walking, sitting with and without a standing baseline) |

## What these numbers do and do not show
- They show the unfamiliar detector fires on a layout shift and stays quiet in normal use **in the simulator**, and that the recording, training, live and UI chain works.
- 100% accuracy reflects an easy simulator. In it, the class vote stayed correct even in the moved layout, so this run does **not** show that the flag prevents wrong answers. Real data is needed.
- Not yet measured: accuracy with real people, real-board packet rate (currently below the kit's 5/s per link gate), false "unfamiliar" alarms per day, detection delay, camera lying-rule accuracy with the real pose model.
- Raw samples: `results.json`. Screenshots: `screenshots/`.
