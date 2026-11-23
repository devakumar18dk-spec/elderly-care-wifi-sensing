# Care Room — privacy-preserving elderly-care posture monitoring with Wi-Fi CSI

Three ESP32-S3 boards and a router sense the room through Wi-Fi signal shape (no camera in normal use). The system shows a caretaker whether someone is
nobody / standing / sitting / lying down / walking, and says "unfamiliar" instead of guessing when the room no longer looks like what it learned.
Wellbeing view only: **not a medical device**, no fall, sleep or vital-sign detection.

Built on the open-source *wifi-sensing-kit* (MIT; see `third_party/`). What we added is listed in [`docs/ENHANCEMENTS.md`](docs/ENHANCEMENTS.md).

## Our contribution
- **Unfamiliar-room detection** (refuses to guess after moved furniture or boards) — [design notes](docs/DESIGN_NOTES_unfamiliar_and_rag.md)
- **Lying class + fixed teacher camera** with view lock and agreement report
- **Caretaker wellbeing page** and **3D sensor-mesh page** (3 boards + router, 9 links)
- **Synthetic simulator and evaluation** so everything can be tested without hardware — [statistics](care/results/STATISTICS.md) (synthetic only)
- **Future work:** Arduino UNO Q supervisor ([plan](docs/future/ARDUINO_UNO_Q_PLAN.md), [gaps](docs/future/UNO_Q_GAPS_TO_FILL.md)); STM32F407VET6 proof of concept ([`stm32_poc/`](stm32_poc/))

## Layout
```
kit/            sensing pipeline: server, evaluation, base dashboards      (base; server.py has our changes)
firmware/       board firmware build/flash tools                           (base)
care/web/       caretaker page, 3D mesh, teacher camera, hub               (ours)
care/sim/       synthetic boards, scripted teacher, evaluation driver      (ours)
care/tests/     pose-rule tests            care/results/  statistics, raw results, screenshots
care/server_patch/  exact diff of our changes to kit/server.py
docs/           enhancements, design notes, future/ (UNO Q)
uno_q_system/   planned (empty)        stm32_poc/  to be completed
third_party/    licence and notices for the base project and components
```

## Run
```
python install.py                         # packages + pose model (once)
cp config.example.yaml config.yaml        # fill in router BSSID, board MACs, PC address
python firmware/build.py && python firmware/flash.py      # boards (see HARDWARE_REQUIREMENTS.md)
python kit/server.py                      # then open http://127.0.0.1:8932/care/
```
Without hardware: `KIT_SOURCE=SYNTHETIC python kit/server.py`, and in a second terminal `python care/sim/kit_sim.py`; pages show a SYNTHETIC badge.
Teach the model with `/care/care_teacher.html` (3 train rounds, 1 test round), then watch `/care/care.html` and `/care/nodes3d.html`.

## Honest status
Works end to end on synthetic data. Real-board accuracy is **not measured**; the boards currently stream below the kit's 5 packets/s per link gate, which must be fixed before real training.
The pose model for the teacher camera is downloaded by `install.py` and was not exercised in our sandbox (tested with a stub). Requirements: [HARDWARE_REQUIREMENTS.md](HARDWARE_REQUIREMENTS.md).
