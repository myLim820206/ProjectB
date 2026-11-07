// 실행: node prototype/tetris-poker/chainshot.js "$(pwd)/prototype/tetris-poker/index.html" 출력경로접두어
// 봇으로 진행하다 족보가 터지는 자리(가능하면 2연쇄)를 찾아 미리보기와 터지는 순간을 캡처한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 780 } });
  await p.goto('file://' + process.argv[2]);
  const ok = await p.evaluate(() => {
    const A = window.__api, S = () => window.__tp(); A.newRun(); S().started = true;
    function boardValue(bd) { let agg = 0, adj = 0; for (let x = 0; x < A.COLS; x++) for (let y = 0; y < A.ROWS; y++) if (bd[y][x]) { agg += A.ROWS - y; break; }
      for (let y = 0; y < A.ROWS; y++) for (let x = 0; x < A.COLS; x++) { const c = bd[y][x]; if (!c) continue; for (const [dx, dy] of [[1, 0], [0, 1]]) { const n = bd[y + dy]?.[x + dx]; if (n && n.r === c.r) adj++; } } return -0.6 * agg + 1.2 * adj; }
    for (let k = 0; k < 300; k++) {
      const s = S(); while (s.phase === 'chain') A.chainStep(); if (s.phase === 'clear') { A.nextRound(); continue; } if (s.phase !== 'play') { A.newRun(); S().started = true; continue; }
      const pc = s.piece; let best = null, spot = null;
      for (let o = 0; o < 4; o++) for (let x = -1; x <= A.COLS; x++) { if (A.collide(pc, x, pc.y, o)) continue; const sv = { x: pc.x, o: pc.o }; pc.x = x; pc.o = o; const pv = A.preview();
        const sc = pv.total * .06 + boardValue(pv.board); if (!best || sc > best.sc) best = { x, o, sc };
        if (k > 8 && pv.chain >= 1 && S().board.flat().filter(Boolean).length > 9 && (!spot || pv.chain > spot.chain || (pv.chain === spot.chain && pv.total > spot.total))) spot = { x, o, total: pv.total, chain: pv.chain }; pc.x = sv.x; pc.o = sv.o; }
      if (spot) { pc.x = spot.x; pc.o = spot.o; return true; }
      pc.x = best.x; pc.o = best.o; A.drop();
    }
    return false;
  });
  await p.waitForTimeout(300); await p.screenshot({ path: process.argv[3] + '-preview.png' });
  if (ok) { await p.keyboard.press(' '); await p.waitForTimeout(130); await p.screenshot({ path: process.argv[3] + '-burst.png' }); }
  console.log('ok', ok); await b.close();
})();
