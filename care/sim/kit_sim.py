#!/usr/bin/env python3
"""SYNTHETIC board simulator for wifi-sensing-kit (3 boards + router = 9 links). Everything it sends is fake.

Sends real kit-format UDP frames (0xC5110001 header, int8 I/Q, 6-byte transmitter MAC tag) to the server, one stream per
link. The "person" state, room layout and noise epoch are read from a small JSON file so a script can change them:
    {"state": "lying", "layout": "A", "epoch": 1, "stall": ""}     state: nobody|standing|sitting|lying|walking
It exists to exercise the real server/UI/classifier code without hardware. It is NOT a physical model of a person."""
import argparse, json, os, socket, struct, sys, time
import numpy as np
HERE = os.path.dirname(os.path.abspath(__file__)); sys.path.insert(0, os.path.join(HERE, "..", ".."))
import kitconfig

STATES = ("nobody", "standing", "sitting", "lying", "walking")
K = 64

def smooth(rng, n=K, w=9):
    v = np.convolve(rng.normal(0, 1, n + 2 * w), np.ones(w) / w, mode="same")[w:-w]
    return v / (np.abs(v).max() + 1e-9)

def main():
    ap = argparse.ArgumentParser(); ap.add_argument("--state-file", default="/tmp/care_state.json")
    ap.add_argument("--rate", type=float, default=10.0, help="frames/s per link"); ap.add_argument("--seed", type=int, default=7)
    a = ap.parse_args()
    cfg = kitconfig.load(); port = int(cfg["host"]["udp_port"])
    macs = {str(b["id"]): bytes(int(x, 16) for x in b["mac"].split(":")) for b in cfg["boards"]}
    macs["R"] = bytes(int(x, 16) for x in cfg["router_bssid"].split(":"))
    ids = [str(b["id"]) for b in cfg["boards"]]
    links = [(tx, rx) for rx in ids for tx in ["R"] + ids if tx != rx]
    rng = np.random.default_rng(a.seed)                       # HARDWARE: fixed for the whole session
    base = {l: 22 + 6 * np.sin(2 * np.pi * np.arange(K) / K * (1 + i % 3) + i) + rng.normal(0, .4, K) for i, l in enumerate(links)}
    shape = {s: smooth(rng) for s in STATES}                   # how each posture bends the channel shape
    coup = {(l, s): rng.uniform(.35, 1.25) for l in links for s in STATES}
    lay_b = {l: smooth(rng) * (4.5 if i % 2 == 0 else 0.0) for i, l in enumerate(links)}   # "furniture moved": 5 of 9 links change
    phi = {l: rng.uniform(0, 2 * np.pi, K) for l in links}
    seg_scale, seg_id = 1.0, None
    sock = socket.socket(2, 2); seq = {l: int(rng.integers(0, 1000)) for l in links}
    st = {"state": "nobody", "layout": "A", "epoch": 0, "stall": ""}; last_read = 0; epoch_rng = None; drift = {}
    print(f"SYNTHETIC sim -> udp 127.0.0.1:{port}, {len(links)} links @ {a.rate} Hz, state file {a.state_file}", flush=True)
    nxt = time.time(); period = 1.0 / a.rate
    while True:
        now = time.time()
        if now - last_read > .2:
            try: st = json.load(open(a.state_file))
            except Exception: pass
            last_read = now
            key = (st.get("epoch"), st.get("state"))
            if key != seg_id:                                   # new epoch or new posture instance: new nuisance draw
                if st.get("epoch") != (seg_id or (None,))[0]:
                    er = np.random.default_rng([a.seed, int(st.get("epoch", 0))]); drift = {l: smooth(er) * 1.2 for l in links}
                seg_scale = float(np.random.default_rng([a.seed, int(st.get("epoch", 0)), hash(st.get("state")) & 0xFFFF, int(now) // 7]).uniform(.8, 1.2))
                seg_id = key
        s = st.get("state", "nobody"); lay = st.get("layout", "A"); stall = st.get("stall", "")
        for l in links:
            if stall and f"{l[0]}>{l[1]}" == stall: continue
            tx, rx = l
            amp = base[l] + drift.get(l, 0)
            if s != "nobody":
                amp = amp + shape[s] * coup[(l, s)] * 2.8 * seg_scale
                if s == "walking":   amp = amp + 3.5 * coup[(l, s)] * np.sin(2 * np.pi * 1.8 * now + phi[l])
                elif s == "standing": amp = amp + .45 * np.sin(2 * np.pi * .7 * now + phi[l])
                elif s == "sitting":  amp = amp + .6 * np.sin(2 * np.pi * .35 * now + phi[l]) * (1 + np.random.random())
                elif s == "lying":    amp = amp + .35 * coup[(l, s)] * np.sin(2 * np.pi * .25 * now)
            if lay == "B": amp = amp + lay_b[l]
            amp = np.clip(amp + np.random.normal(0, .55, K), 0.5, 100)
            ph = phi[l] + np.random.normal(0, .05, K)
            iq = np.empty(2 * K, np.int8); iq[0::2] = np.clip(np.round(amp * np.sin(ph)), -127, 127); iq[1::2] = np.clip(np.round(amp * np.cos(ph)), -127, 127)
            seq[l] += 1
            rssi = int(-45 - 3 * (hash(l) % 5) + np.random.normal(0, .7))
            hdr = struct.pack("<IBBHIIbbBB", 0xC5110001, int(rx), 1, K, 2412, seq[l], rssi, -95, 0, 0x80)
            sock.sendto(hdr + iq.tobytes() + macs[tx], ("127.0.0.1", port))
        nxt += period; time.sleep(max(0, nxt - time.time()))

if __name__ == "__main__": main()
