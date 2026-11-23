# What is ours, what is base

The sensing pipeline (CSI frames from three boards and a router, per-link features, k-nearest-neighbour posture model, camera-taught training) comes from the open-source
**wifi-sensing-kit** (MIT licence; credit and licence text in `third_party/`). Everything below is added on top of it.

| Area | Base kit | This project |
|---|---|---|
| Classes | camera-defined movements (no lying) | nobody / standing / sitting / **lying** / walking (`care/web/care_pose_rules.js`) |
| Unknown situations | always returns the nearest class | **"unfamiliar" state**: refuses to guess when the live signal is far from everything learned (moved furniture, nudged board) — `kit/server.py`, switch `KIT_UNFAMILIAR` |
| Stale data | silently reuses a link's feature up to 3 s old | counts it per recording, A/B switch `KIT_STRICT_FRESH=1` |
| Teacher camera | movement page | **fixed-view teacher**: view lock + shift warning, guided recording, camera-vs-instruction agreement report (`care/web/care_teacher.html`) |
| Caretaker UI | engineering dashboards | plain-language wellbeing page: posture, confidence, 10-minute timeline, link health, non-medical notices (`care/web/care.html`) |
| 3D | none | **sensor-mesh scene**: 3 boards + router, packets on all links, per-link evidence, unfamiliar ring, link-down warning, fixed camera presets (`care/web/nodes3d.html`) |
| Testing without hardware | none | synthetic 3-board simulator, scripted camera, evaluation driver (`care/sim/`) |
| Future work | – | UNO Q supervisor plan and STM32 proof of concept (`docs/future/`, `uno_q_system/`, `stm32_poc/`) |

Changes to base files are limited to `kit/server.py` (diff in `care/server_patch/server_changes.patch`): stale counter, unfamiliar status, `source` field in `/api/config`, and serving `care/web` under `/care/`.
Design reasoning for the unfamiliar state and why a language model was not added: `docs/DESIGN_NOTES_unfamiliar_and_rag.md`.
