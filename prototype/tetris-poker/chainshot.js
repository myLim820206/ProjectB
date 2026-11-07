// 실행: node prototype/tetris-poker/chainshot.js "$(pwd)/prototype/tetris-poker/index.html" 출력경로접두어
// 봇으로 진행하다 2연쇄 이상 자리를 찾아 미리보기와 터지는 순간을 캡처한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 760 } });
  await p.goto('file://' + process.argv[2]);
  // 봇으로 진행하다가 2연쇄 이상이 나는 자리가 생기면 거기에 둔 채 멈춰 미리보기를 찍고, 떨어뜨려 터지는 순간을 찍는다
  const ok = await p.evaluate(() => {
    const A = window.__api, S = () => window.__tp(); A.newRun(); S().started = true;
    function shape(board) { let holes = 0, agg = 0, bump = 0, hs = []; for (let x = 0; x < 10; x++) { let h = 0, seen = false; for (let y = 0; y < 16; y++) { if (board[y][x]) { if (!seen) { h = 16 - y; seen = true; } } else if (seen) holes++; } hs.push(h); agg += h; } for (let x = 1; x < 10; x++) bump += Math.abs(hs[x] - hs[x - 1]); return -0.5 * agg - 4 * holes - 0.3 * bump; }
    for (let k = 0; k < 200; k++) {
      const s = S(); while (s.phase === 'chain') A.chainStep(); if (s.phase === 'clear') { A.nextRound(); continue; } if (s.phase !== 'play') { A.newRun(); S().started = true; continue; }
      const pc = s.piece; let best = null, chainSpot = null;
      for (let r = 0; r < 4; r++) for (let x = -2; x < 10; x++) { if (A.collide(pc, x, pc.y, r)) continue; const sv = { x: pc.x, r: pc.r }; pc.x = x; pc.r = r; const pv = A.preview();
        const sc = (pv.total || 0) * 0.05 + shape(pv.board); if (!best || sc > best.sc) best = { x, r, sc }; if (k > 8 && pv.chain >= 2 && (!chainSpot || pv.total > chainSpot.total)) chainSpot = { x, r, total: pv.total }; pc.x = sv.x; pc.r = sv.r; }
      if (chainSpot) { pc.x = chainSpot.x; pc.r = chainSpot.r; return true; }
      pc.x = best.x; pc.r = best.r; A.drop();
    }
    return false;
  });
  await p.waitForTimeout(300); await p.screenshot({ path: process.argv[3] + '-preview.png' });
  if (ok) { await p.keyboard.press(' '); await p.waitForTimeout(120); await p.screenshot({ path: process.argv[3] + '-burst.png' }); await p.waitForTimeout(300); await p.screenshot({ path: process.argv[3] + '-chain2.png' }); }
  console.log('ok', ok); await b.close();
})();
