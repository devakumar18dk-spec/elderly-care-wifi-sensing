# Arduino UNO Q supervisor — gaps to fill

Your plan is in `ARDUINO_UNO_Q_PLAN.md` (stored unchanged). I read it against the code in this repository. **Status of the UNO Q work: nothing implemented yet.**
For each gap: why it matters, what the repository does today, and a line for your answer. Write under **Your answer**; delete what you decide against.

## A. What already exists vs. what the plan assumes
| Plan item | Today in the repo | Gap |
|---|---|---|
| §4 server sends feature windows, links, kNN result | `/stream` (about 15 Hz) carries per-link packet rate, signal strength, motion, and the kNN result (`best`, `confidence`, `status`, `novelty`). It does **not** carry the 156-number feature vector per link. | Add a small endpoint for compact feature windows, or decide the supervisor only needs summaries. |
| §6 deviation score `z = |x-μ|/(σ+ε)` | Server already computes a per-window **novelty** (distance to nearest learned windows / normal distance between rounds). | Decide the split (see A1). |
| §7 router channel change | Boards report the channel in each frame header; the server does not expose it. | Expose it, or detect from the boards' reports. |
| §9/§12 room versions, never overwrite | One dataset file `data/movement.npz`; a "Clear all" button deletes it; samples carry session and round but no room-version tag. | Versioned datasets and a guard on Clear (A4). |
| §10 kNN reliability | `confidence` (vote share) and `novelty` exist per window. | Define "prediction stability" (A5). |
| §3 link health | Server ignores links under 5 packets/s. Our real boards currently reach about 1.5–4/s. | Hardware problem to fix first, otherwise health checks always fail. |

## B. Decisions I could not make for you
**A1. Who does what in change detection?** Suggested split: the server scores every window (novelty); the UNO Q decides whether a *persistent, multi-link* change happened (rolling statistics / CUSUM), keeps versions, and talks to the caretaker. This avoids two parts raising the same alert.
Your answer:

**A2. What is the baseline?** A person in the room changes the signal as much as moved furniture. Compare against an *empty-room* baseline only when the model says "nobody", or keep a baseline per posture? Without this, normal activity looks like an environment change.
Your answer:

**A3. How do you tell furniture from a moved board?** CSI alone cannot (the plan correctly asks the caretaker). Possible cue to test, unproven: board-to-board links change while router links do not. Do you want to test it, or always ask?
Your answer:

**A4. One model or one per room version?** Adding new-layout samples to the same dataset gives one model that knows both layouts (it grows, and may blur); a model per version is clean but needs a full re-teach each time. Also: dataset files must be copied, not overwritten, when a version is created.
Your answer:

**A5. Define "prediction stability".** For example: label changes per minute while the person is known to be still, or share of low-confidence windows in the last N minutes.
Your answer:

**A6. Who labels the validation samples after recalibration (§9 step 7)?** The base kit labels with a camera. Options: bring the teacher camera back for 5 minutes (best labels); a guided script where the caretaker follows instructions without a camera (weaker labels, no camera needed); caretaker confirms a few spot checks.
Your answer:

**A7. Thresholds.** CUSUM drift/limit, number of links that must agree, how many minutes of "stable room" before collecting, target false alarms per day. These need several days of real empty-room and normal-use recordings, which do not exist yet.
Your answer:

## C. Platform questions (I could not verify these)
- UNO Q memory, storage, OS image, Python version; do `numpy`/`pyyaml` install for it? Can it run the kit server (about 9 links x 20 packets/s plus the dashboard stream) or only the supervisor?
- Does the UNO Q join the same 2.4 GHz network as the boards? Any limit on listening to UDP on port 5005?
- How the Linux side and the STM32U585 talk (Bridge), and what the MCU does in version 1 (buzzer, LED, watchdog).
- **Two different microcontrollers appear in the project:** STM32U585 (inside UNO Q, I/O only) and STM32F407VET6 (statistical-model proof of concept). Say clearly in the pitch which does what, so they are not confused.
Your answer:

## D. Safety, privacy, operations
- The dashboard will let a caretaker approve recalibration, which changes what the system treats as normal. Needs at least a login or token; on a home LAN it is otherwise open to anyone.
- What happens when nobody answers a verification request? Suggested: keep monitoring, show "unverified change" and mark predictions as lower trust; never auto-accept a new baseline.
- Alert lifecycle: who is told, how, repeat interval, acknowledgement, quiet hours.
- Routine monitoring (§11): kNN mistakes flow into the routine profile, so count with confidence weighting. "Preserve the historical routine" needs a concrete rule (for example a frozen reference period plus a rolling one) and a minimum number of days before any notice.
- Power loss, clock/time sync (NTP) between laptop, UNO Q and boards, database migration between versions.
Your answer:

## E. How to show it works at the demo
Suggested measures: detection delay after a layout change, false alarms per day with no change, kNN accuracy before / after supervised recalibration. The synthetic simulator in `care/sim/` can script a layout change repeatably; a real furniture move is the honest version.
Your answer:

## F. Suggested build order (from your plan, adjusted)
1. Fix real-board packet rate (blocks everything with real data). 2. Compact feature endpoint on the server. 3. Baseline store with versions (SQLite). 4. Persistence/multi-link change detector, tested on the simulator, then on real recordings. 5. Dashboard section and caretaker response. 6. Recalibration and validation. 7. Routine monitoring.
Your changes:
