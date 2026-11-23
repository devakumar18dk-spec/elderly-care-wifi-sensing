#!/usr/bin/env python3
"""Drives the SYNTHETIC story against the real patched kit server and writes care/results/results.json.
phases: 3 train rounds -> test session -> live in-layout -> live layout-shifted (furniture moved) -> live in-layout again."""
import json, os, time, urllib.request, threading, sys, random
PORT, SF = 8932, "/tmp/care_state.json"
STATES = ["nobody", "standing", "sitting", "lying", "walking"]
HOLD_TRAIN, HOLD_TEST, HOLD_LIVE = 20, 14, 10
def post(p, b):
    try: return json.loads(urllib.request.urlopen(urllib.request.Request(f"http://127.0.0.1:{PORT}{p}", json.dumps(b).encode(), {"Content-Type": "application/json"}), timeout=5).read())
    except Exception as e: return {"ok": False, "error": str(e)}
def setst(**k):
    d = json.load(open(SF)); d.update(k); json.dump(d, open(SF, "w"))
log = {"samples": [], "rec": {}}; phase = ["init"]; cur = ["nobody"]; stop = [False]
def watcher():
    r = urllib.request.urlopen(f"http://127.0.0.1:{PORT}/stream", timeout=10)
    while not stop[0]:
        l = r.readline().decode()
        if not l.startswith("data:"): continue
        m = json.loads(l[6:]); e = m.get("move_est"); rec = (m.get("move") or {}).get("recording")
        if rec: log["rec"][phase[0]] = rec
        if e and e.get("best") and phase[0].startswith("live"):
            log["samples"].append({"t": m["t"], "phase": phase[0], "truth": cur[0], "best": e["best"], "conf": e["confidence"], "status": e["status"], "nov": e["novelty"], "stale": e["stale"]})
threading.Thread(target=watcher, daemon=True).start()
def hold(state, secs, **k):
    cur[0] = state; setst(state=state, **k); time.sleep(secs)
rng = random.Random(3)
for rnd in (1, 2, 3):
    phase[0] = f"train{rnd}"; order = STATES[:]; rng.shuffle(order); setst(layout="A", epoch=rnd); hold("nobody", 3)
    print(post("/api/move/rec", {"on": True, "session": "train", "round": rnd}), flush=True)
    for s in order: hold(s, HOLD_TRAIN)
    post("/api/move/rec", {"on": False}); print("round", rnd, "done", flush=True)
phase[0] = "test"; setst(layout="A", epoch=7); hold("nobody", 3)
print(post("/api/move/rec", {"on": True, "session": "test", "round": 1}), flush=True)
for s in rng.sample(STATES, 5): hold(s, HOLD_TEST)
post("/api/move/rec", {"on": False}); print("test done", flush=True)
for ph, lay in (("live_A", "A"), ("live_B", "B"), ("live_A2", "A")):
    phase[0] = ph; setst(layout=lay, epoch=7); hold("nobody", 3)
    for s in rng.sample(STATES, 5): hold(s, HOLD_LIVE)
    print(ph, "done", flush=True)
phase[0] = "end"; stop[0] = True
r = urllib.request.urlopen(f"http://127.0.0.1:{PORT}/stream", timeout=10); l = r.readline(); r.readline()
m = json.loads(r.readline().decode()[6:] if False else l.decode()[6:]); log["summary"] = m["move"]
json.dump(log, open(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "results", "results.json"), "w")); print("RESULTS WRITTEN", flush=True)
