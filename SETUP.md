# 🛠️ Setup Guide: From Download to Running

**Contents:** [1. Get the project](#1-get-the-project) · [2. Try without hardware](#2-try-it-without-hardware-simulator-demo) · [3. Run with real boards](#3-run-with-the-real-boards) · [4. Check results](#4-check-results-and-tests) · [5. Upload changes to GitHub](#5-upload-changes-to-github) · [Troubleshooting](#troubleshooting)

Commands are for Ubuntu (WSL2 on Windows 11, or Linux). Run them in the project folder unless stated otherwise.

## 1. Get the project

Clone the repository:

```bash
git clone https://github.com/devakumar18dk-spec/elderly-care-wifi-sensing.git
cd elderly-care-wifi-sensing
```

## 2. Try it without hardware (simulator demo)

**Terminal 1:**

```bash
sudo apt update && sudo apt install -y python3 python3-venv python3-pip git
python3 -m venv .venv && source .venv/bin/activate
pip install numpy pyyaml
cp config.example.yaml config.yaml
KIT_SOURCE=SYNTHETIC python kit/server.py
```

**Terminal 2:**

```bash
cd ~/elderly-care-wifi-sensing
source .venv/bin/activate
python care/sim/kit_sim.py
```

Open `http://127.0.0.1:8932/care/` in your browser. The pages show a **SYNTHETIC** badge because the simulator sends fake data.

Until a model is trained, the pages will indicate that it is not trained yet.

Train the simulated model with the scripted test (about 8 minutes).

**Terminal 3:**

```bash
cd ~/elderly-care-wifi-sensing
source .venv/bin/activate
python care/sim/teacher_sim.py &
python care/sim/drive_eval.py
```

Results are written to `care/results/results.json`.

## 3. Run with the real boards

1. Edit `config.yaml`:

   - Wi-Fi SSID and `password_file`.
   - `host.ip` (your PC's address on the router).
   - `router_bssid`.
   - The three boards' MAC addresses and IDs.

   Put your Wi-Fi password alone in `~/.wifi-sensing-pass`, then run:

   ```bash
   chmod 600 ~/.wifi-sensing-pass
   ```

2. Install packages and download the camera pose model (about 28 MB):

   ```bash
   source .venv/bin/activate
   python install.py
   ```

3. Build and flash the boards. This requires ESP-IDF v5.4 at the path specified in `config.yaml`. Connect the boards using USB data cables.

   ```bash
   python firmware/discover.py
   python firmware/build.py
   python firmware/flash.py
   ```

   **WSL note:** WSL cannot access USB devices by default. Flash from Windows, or share the boards with WSL using `usbipd`.

4. Allow incoming **UDP port 5005** through the Windows firewall. Give the PC a fixed IP address using a DHCP reservation on your router.

5. Start the server:

   ```bash
   python kit/server.py
   ```

6. Teach the model with the camera fixed so the whole room, including the bed, is visible:

   - Open `http://127.0.0.1:8932/care/care_teacher.html`.
   - Press **Lock this view** while the room is empty.
   - Record three rounds as **train**, then one round as **test**.

7. Open the dashboards:

   - `http://127.0.0.1:8932/care/care.html` — caretaker page.
   - `http://127.0.0.1:8932/care/nodes3d.html` — 3D sensor mesh.

**Note:** Links must reach 5 packets per second to be used. If the dashboard reports that links are too slow, fix that before training.

## 4. Check results and tests

```bash
python kit/evaluate.py
node care/tests/test_pose_rules.mjs
```

The evaluation script checks accuracy using your recorded data. The pose-rule tests require Node.js.

## 5. Upload changes to GitHub

If you are updating the existing repository, run these commands from the project folder.

Check your changes:

```bash
git status
```

Stage the README, setup guide, and documentation images:

```bash
git add README.md SETUP.md docs
git status
```

Review what will be committed:

```bash
git diff --cached --stat
```

Commit and push:

```bash
git commit -m "Add animated README, setup guide and dashboard images"
git push origin main
```

**Important:** Do not upload `config.yaml`, Wi-Fi credentials, passwords, or private data. Do not add the downloaded `care_room_readme_images.zip` file to the repository.

## Troubleshooting

- **No boards found:** Try another USB cable. Charge-only cables do not carry data.
- **Boards flashed but not visible on the dashboard:** Check the SSID, password, 5 GHz-only network setting, firewall, and whether the PC's IP address changed. Re-run `python firmware/flash.py` if necessary.
- **Page says "model not trained":** Record the train and test rounds first.
- **Teacher page shows a camera or pose-model error:** Run `python install.py` and allow camera access in your browser.
