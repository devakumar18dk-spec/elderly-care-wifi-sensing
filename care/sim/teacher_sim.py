#!/usr/bin/env python3
"""SYNTHETIC stand-in for the webcam teacher: posts the scripted posture to /api/pose at 10 Hz, like movement.html does,
with 'unsure' for 1.2 s after each change (the real camera is unsure while you move). No video, no camera."""
import argparse, json, time, urllib.request
ap = argparse.ArgumentParser(); ap.add_argument("--state-file", default="/tmp/care_state.json"); ap.add_argument("--port", type=int, default=8932); a = ap.parse_args()
cur, since = None, 0
while True:
    try: st = json.load(open(a.state_file)).get("state", "nobody")
    except Exception: st = "nobody"
    if st != cur: cur, since = st, time.time()
    lab = "unsure" if time.time() - since < 1.2 else cur
    try:
        urllib.request.urlopen(urllib.request.Request(f"http://127.0.0.1:{a.port}/api/pose", json.dumps({"label": lab, "tab": "sim"}).encode(), {"Content-Type": "application/json"}), timeout=2).read()
    except Exception: pass
    time.sleep(.1)
