// 실행: node prototype/stack-poker/shot.js "$(pwd)/prototype/stack-poker/index.html" 출력.png
// 30초쯤 진행한 뒤 족보 하나를 골라 둔 장면을 캡처한다
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 780 }, deviceScaleFactor: 2 });
  await p.goto('file://' + process.argv[2]);
  await p.evaluate(() => {
    const A = window.__api; A.setMuted(true);
    const step = sec => { for (let t = 0; t < sec - 1e-6; t += 0.05) A.update(Math.min(0.05, sec - t)); };
    A.newGame(); const g = window.__sp(); g.phase = 'play';
    function subsets(arr, k, start = 0, cur = [], out = []) { if (cur.length === k) { out.push(cur.slice()); return out; } for (let i = start; i < arr.length; i++) { cur.push(arr[i]); subsets(arr, k, i + 1, cur, out); cur.pop(); } return out; }
    const find = (ks) => { let best = null; const cand = g.cols.flatMap(c => c.slice(-6)); for (const k of ks) for (const sub of subsets(cand, k)) { const h = A.evalHand(sub); if (h && (!best || h.score > best.h.score)) best = { h, sub }; } return best; };
    for (let k = 0; k < 6; k++) { step(4); const b = find([4, 3]); if (b) { b.sub.forEach(c => A.toggle(c)); A.playHand(); } }
    for (let k = 0; k < 40; k++) A.update(0.05);
    g.fx = []; g.texts = [];
    const b = find([5, 4]) || find([3]); if (b) b.sub.forEach(c => A.toggle(c));
  });
  await p.waitForTimeout(260); await p.screenshot({ path: process.argv[3] }); await b.close();
})();
