# uno_q_system — planned, not implemented

Placeholder for the Arduino UNO Q supervisor. No code has been written. The plan is in `docs/future/ARDUINO_UNO_Q_PLAN.md`; open questions are in `docs/future/UNO_Q_GAPS_TO_FILL.md`.

Planned layout (from the plan):
```
uno_q_system/
  main.py
  monitoring/   link_health.py  environment_monitor.py  change_detector.py  knn_monitor.py
  calibration/  baseline_collector.py  baseline_validator.py  version_manager.py
  caretaker/    alert_manager.py  verification_handler.py
  database/     room_baselines.db  events.db
  config/       room_config.json
  logs/
```
| Module group | Status |
|---|---|
| Data interface to the server | not started |
| Baseline manager and versions | not started |
| Environment-change detection | not started |
| Caretaker workflow and dashboard section | not started |
| Recalibration | not started |
| kNN supervision | not started |
| Routine monitoring | not started |
