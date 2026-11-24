<div align="center">

# 🛰️ Arduino UNO Q: the Room Supervisor

**ESP32-S3 nodes sense · KNN recognises · the UNO Q judges the room · the caretaker approves changes**

![Board](https://img.shields.io/badge/Board-Arduino%20UNO%20Q-00979D)
![MPU](https://img.shields.io/badge/Linux-QRB2210%20Debian-2F74C0)
![MCU](https://img.shields.io/badge/MCU-STM32U585-03234B)
![Storage](https://img.shields.io/badge/Storage-SQLite-003B57)
![Status](https://img.shields.io/badge/status-planned%20design-orange)

</div>

> ⚠️ **Planned design, not yet implemented.** The UNO Q supervises the existing Wi-Fi Sensing Kit and its KNN model. It does not replace them.

---

## 🏠 The setup

![Setup](docs/images/uno-q-setup.svg)

The three ESP32-S3 boards sense the room. The Python server turns CSI into KNN predictions. The UNO Q is **not** a sensing node. It receives compact feature windows from the server over the local network.

---

## 🔄 How data flows

![Flow](docs/images/uno-q-flow-animated.svg)

---

## 🧩 Two processors, one board

| Unit | Role |
|---|---|
| **Qualcomm QRB2210** (Debian, Python) | Baselines, change detection, KNN health monitoring, caretaker alerts, recalibration, SQLite storage. Phase 1 uses this only. |
| **STM32U585** | Optional hardware-level and real-time control, added later through the Arduino Bridge. |

## 🎯 What the UNO Q does

1. **Keeps a baseline** of the normal room as `room_v1`.
2. **Watches for persistent change** across several healthy CSI links, ignoring one-off spikes and normal movement.
3. **Asks the caretaker** to confirm what changed. CSI alone cannot say which object moved.
4. **Manages recalibration**: saves the old state, waits for a stable room, collects a new baseline and validates it.
5. **Versions the room** as `room_v2`, with `room_v1` archived and never overwritten.
6. **Validates the KNN** on caretaker-labelled samples. It never retrains on the model's own unverified predictions.

## 🗺️ Build order

- [ ] 1. Server to UNO Q data interface
- [ ] 2. Baseline manager and versions
- [ ] 3. Persistent-change detection
- [ ] 4. Caretaker alerts on the dashboard
- [ ] 5. Controlled recalibration
- [ ] 6. KNN reliability monitoring
- [ ] 7. Daily-routine monitoring

## ⚖️ Honest limits

- Thresholds come from real measurements, not guesses.
- Not a medical device. No fall or sleep detection claims.
- Everything stays local. No video is stored.

<div align="center">

Part of the [Wi-Fi Sensing Kit for Elderly Care](README.md)

</div>
