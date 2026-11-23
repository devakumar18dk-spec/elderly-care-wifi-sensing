import { sample, makeClassifier, viewShift, P } from "../web/care_pose_rules.js";
const blank = () => Array.from({length:33}, () => ({x:.5,y:.5,visibility:0}));
function person(kind, t, cx=.5) {
  const l = blank(), set=(i,x,y)=>l[i]={x,y,visibility:.9};
  if (kind==="standing"||kind==="walking") { set(P.LS,cx-.05,.3); set(P.RS,cx+.05,.3); set(P.LH,cx-.04,.55); set(P.RH,cx+.04,.55); set(P.LK,cx-.04,.75); set(P.RK,cx+.04,.75); set(P.LA,cx-.04,.95); set(P.RA,cx+.04,.95); }
  if (kind==="sitting") { set(P.LS,cx-.05,.35); set(P.RS,cx+.05,.35); set(P.LH,cx-.04,.6); set(P.RH,cx+.04,.6); set(P.LK,cx+.12,.63); set(P.RK,cx+.14,.63); set(P.LA,cx+.12,.85); set(P.RA,cx+.14,.85); }
  if (kind==="lying") { set(P.LS,.3,.62); set(P.RS,.3,.68); set(P.LH,.55,.63); set(P.RH,.55,.69); set(P.LK,.72,.63); set(P.RK,.72,.69); set(P.LA,.9,.64); set(P.RA,.9,.7); }
  return l;
}
function run(seq, seconds=4) { const c = makeClassifier(); let out; for (let i=0;i<seconds*8;i++){ const t=i*125; const kind=seq(t); const lm = kind==="nobody"?null:person(kind==="walking"?"walking":kind,t, kind==="walking"?.2+ .6*((t/1000)%2)/2 : .5); c.feed(sample(lm,t)); out=c.label(t);} return out; }
const results = {};
const c = makeClassifier(); // teach standing first so sitting has a baseline, like in real use
for (let i=0;i<24;i++){c.feed(sample(person("standing",i*125),i*125));c.label(i*125)}
console.log("standing thighBase learned:", c.thighBase.toFixed(2));
for (const k of ["nobody","standing","lying","walking"]) results[k]=run(()=>k);
// sitting after standing baseline
const c2 = makeClassifier(); let t=0; for(;t<24*125;t+=125){c2.feed(sample(person("standing",t),t));c2.label(t)} let lab; for(let i=0;i<24;i++,t+=125){c2.feed(sample(person("sitting",t),t));lab=c2.label(t)} results.sitting_after_standing=lab;
// first-time sitting without baseline
results.sitting_cold = run(()=>"sitting");
// transition lying -> standing flips within 1.5 s
console.log(results);
console.log("viewShift same", viewShift(new Uint8Array(100).fill(100), new Uint8Array(100).fill(101)), "moved", viewShift(new Uint8Array(100).fill(100), new Uint8Array(100).fill(140)));
