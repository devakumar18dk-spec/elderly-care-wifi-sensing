// Care-posture rules for a FIXED camera: nobody | standing | sitting | lying | walking | unsure.
// Pure functions (no DOM) so they can be unit-tested. Landmarks are MediaPipe Pose (33 points, x/y normalised 0..1).
export const P = { LS: 11, RS: 12, LH: 23, RH: 24, LK: 25, RK: 26, LA: 27, RA: 28 };
const vis = (l, i, th = .4) => (l[i].visibility ?? 1) > th;
const mid = (a, b) => ({ x: (a.x + b.x) / 2, y: (a.y + b.y) / 2 });
const span = a => a.length ? Math.max(...a) - Math.min(...a) : 0;
const median = a => { const b = [...a].sort((x, y) => x - y); return b.length ? b[b.length >> 1] : 0; };

/** One sample (8/s) from landmarks, or null when no person. */
export function sample(lm, t) {
  if (!lm) return { t, present: false };
  const ok = (vis(lm, P.LS) || vis(lm, P.RS)) && (vis(lm, P.LH) || vis(lm, P.RH));
  if (!ok) return { t, present: false };
  const sh = mid(lm[P.LS], lm[P.RS]), hip = mid(lm[P.LH], lm[P.RH]);
  const dx = hip.x - sh.x, dy = hip.y - sh.y, torso = Math.hypot(dx, dy) || 1e-3;
  const legs = [[P.LH, P.LK], [P.RH, P.RK]].filter(([h, k]) => vis(lm, h) && vis(lm, k, .25));
  return { t, present: true, hip, torso, horiz: Math.abs(dx) / torso,           // cosine: 1 = torso horizontal in the image, 0 = upright
           thigh: legs.length ? Math.min(...legs.map(([h, k]) => Math.abs(lm[k].y - lm[h].y))) / torso : null,
           };
}

/** Stateful classifier. feed(sample) then label(now). Learns the person's standing thigh length while they stand. */
export function makeClassifier() {
  const hist = []; let thighBase = null, last = "unsure", streak = 0, cand = "unsure";
  function feed(s) { hist.push(s); while (hist.length > 40) hist.shift(); }
  function raw(now) {
    const H = hist.filter(h => now - h.t < 1600);
    if (H.length < 8) return "unsure";
    const seen = H.filter(h => h.present);
    if (seen.length < H.length * .3) return "nobody";
    if (seen.length < H.length * .7) return "unsure";                          // walking in/out of view
    const torso = median(seen.map(h => h.torso)), xs = seen.map(h => h.hip.x);
    const dt = (seen[seen.length - 1].t - seen[0].t) / 1000 || 1.5;
    const speed = Math.abs(xs[xs.length - 1] - xs[0]) / torso / dt + span(xs) / torso * .5;
    const horiz = median(seen.map(h => h.horiz));
    const th = seen.map(h => h.thigh).filter(v => v != null), thigh = th.length ? median(th) : null;
    if (horiz > 0.8 && span(seen.map(h => h.hip.y)) / torso < .6) return "lying";   // torso lies across the image and the person is not mid-fall/transition
    if (speed > .9) return "walking";
    const comp = thigh != null && thighBase ? thigh / thighBase : null;
    if (comp != null && comp < .7) return "sitting";
    if (thigh != null && thigh < .45 && !thighBase) return "sitting";
    if (horiz < .6 && thigh != null) { thighBase = thighBase == null ? thigh : .95 * thighBase + .05 * thigh; return "standing"; }
    return "unsure";
  }
  // a new label must be seen 3 times in a row before it is reported (avoids flicker; 'unsure' is reported at once)
  function label(now) {
    const r = raw(now);
    if (r === cand) streak++; else { cand = r; streak = 1; }
    if (r === "unsure" || streak >= 3) last = r;
    return last;
  }
  return { feed, label, get thighBase() { return thighBase; }, reset() { hist.length = 0; thighBase = null; last = "unsure"; } };
}

/** Fixed-view check: mean absolute difference between two gray thumbnails (0..255). */
export function viewShift(ref, cur) { let s = 0; for (let i = 0; i < ref.length; i++) s += Math.abs(ref[i] - cur[i]); return s / ref.length; }
