// 실행: node prototype/poker-defense/shot.js "$(pwd)/prototype/poker-defense/index.html" 출력.png
// 봇으로 6웨이브까지 진행하고, 7웨이브 중간 장면 + 카드를 고른 미리보기를 캡처한다
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 2 });
  await p.goto('file://' + process.argv[2]);
  const hover = await p.evaluate(() => {
    const A = window.__api, S = () => window.__pd(); A.setMuted(true); A.newGame(true); const g = S();
    const cells = []; for (let y = 0; y < A.ROWS; y++) for (let x = 0; x < A.COLS; x++) if (A.buildable(x, y)) cells.push([x, y]);
    const pathPts = []; for (let d = 0; d < A.pathLen(); d += A.CS / 2) pathPts.push(A.posAt(d));
    const coverage = (x, y, range) => { const [cx, cy] = A.center(x, y); return pathPts.filter(([px, py]) => Math.hypot(px - cx, py - cy) <= range * A.CS).length; };
    const power = towers => { A.applyEffects(towers); let s = 0; for (const t of towers.values()) { const st = A.towerStats(t); s += st.dmg * st.rate * (1 + st.splash * .8) * coverage(t.x, t.y, st.range); } return s; };
    const bestMove = () => { let best = null; for (let i = 0; i < g.hand.length; i++) for (const [x, y] of cells) { if (g.towers.has(x + ',' + y)) continue; const tw = new Map([...g.towers].map(([k, v]) => [k, { ...v }])); tw.set(x + ',' + y, { card: g.hand[i], x, y }); const v = power(tw); if (!best || v > best.v) best = { v, i, x, y }; } return best; };
    for (let w = 0; w < 6; w++) { while (g.hand.length > (w === 5 ? 2 : 0)) { const m = bestMove(); if (!m) break; A.place(m.i, m.x, m.y); } A.startWave(); let k = 0; while (g.phase === 'wave' && k++ < 20000) A.update(0.05); }
    A.startWave(); for (let k = 0; k < 160; k++) A.update(0.05);
    const m = bestMove(); if (!m) return null; g.sel = m.i; g.hover = { x: m.x, y: m.y }; g.texts = []; return m;
  });
  await p.waitForTimeout(300); await p.screenshot({ path: process.argv[3] }); console.log('hover', JSON.stringify(hover)); await b.close();
})();
