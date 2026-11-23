# Hardware and software requirements

## Sensing set-up (what this repository runs on today)
| Item | Qty | Notes |
|---|---|---|
| ESP32-S3 development board | 3 | Must be ESP32-S3 (the firmware targets `esp32s3`). Ours: DevKitC-style, 8 MB flash. Each board needs its own USB data cable (charge-only cables do not work for flashing) and a 5 V USB power source in the room. |
| 2.4 GHz Wi-Fi router | 1 | Boards join it on 2.4 GHz and send UDP to the PC. Use a fixed channel and give the PC a DHCP reservation (a changed PC address stops all boards). |
| PC / laptop | 1 | Runs `kit/server.py` and the dashboards. Windows 11 with WSL2 Ubuntu, or Linux. Must be on the same network. Allow incoming UDP on port 5005 in the firewall. |
| USB webcam (or laptop camera) | 1 | Set-up only: teaches the model. Mount it fixed so the whole room including the bed is visible. Not needed in normal use. |
| Browser | 1 | Chromium-based (Chrome/Edge) recommended; WebGL needed for the 3D page. |

## Software
- Python 3.10+ and the packages in `requirements.txt` (`python install.py` installs them and fetches the in-browser pose model, about 28 MB, once).
- ESP-IDF v5.4 to build the board firmware (a Docker `espressif/idf:v5.4` container works), plus `esptool` to flash.
- Node.js (optional): only for `care/tests/test_pose_rules.mjs`.

## Planned / proof-of-concept hardware (not required to run the repository)
| Item | Role | Status |
|---|---|---|
| Arduino UNO Q | Future supervisor: room baselines, environment-change detection, caretaker-confirmed recalibration (see `docs/future/`) | Planned, not implemented |
| STM32F407VET6 board | Proof of concept: statistical models running on a microcontroller (see `stm32_poc/`) | Done by the team; source and results to be added |

## Not provided
Router, power supplies, USB cables, mounting hardware, and any patient-facing equipment. Every board must stay powered and in range of the router for its links to stream.
