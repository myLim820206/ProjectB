// 실행: node prototype/tetris-poker/bottest.js "$(pwd)/prototype/tetris-poker/index.html" [판 수]
// 봇이 2장 블록의 모든 자리(방향 4 × 열)를 따져 "예상 점수(연쇄 포함) + 판 상태"가 가장 좋은 곳에 놓으며 진행한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const runs = +(process.argv[3] || 20);
  const out = await p.evaluate((runs) => {
    const A = window.__api, S = () => window.__tp();
    // 판 상태: 낮을수록, 평평할수록, 같은 숫자·무늬가 이웃할수록 좋다
    function boardValue(bd) { let agg = 0, bump = 0, hs = [], adj = 0;
      for (let x = 0; x < A.COLS; x++) { let h = 0; for (let y = 0; y < A.ROWS; y++) if (bd[y][x]) { h = A.ROWS - y; break; } hs.push(h); agg += h; }
      for (let x = 1; x < A.COLS; x++) bump += Math.abs(hs[x] - hs[x - 1]);
      for (let y = 0; y < A.ROWS; y++) for (let x = 0; x < A.COLS; x++) { const c = bd[y][x]; if (!c) continue; for (const [dx, dy] of [[1, 0], [0, 1]]) { const n = bd[y + dy]?.[x + dx]; if (n && (n.r === c.r)) adj += 1; if (n && n.s === c.s) adj += .3; } }
      return -0.6 * agg - 0.4 * bump + 1.2 * adj - (Math.max(...hs) > A.ROWS - 3 ? 40 : 0); }
    const results = [];
    for (let run = 0; run < runs; run++) {
      A.newRun(); S().started = true; const rounds = []; let guard = 0;
      while (guard++ < 6000) {
        const s = S();
        if (s.phase === 'chain') { A.chainStep(); continue; }
        if (s.phase === 'clear') { rounds.push({ r: s.round + 1, score: s.score, left: s.piecesLeft, bursts: s.bursts, maxChain: s.maxChain }); A.nextRound(); continue; }
        if (s.phase === 'over' || s.phase === 'win') break;
        const pc = s.piece; let best = null;
        for (let o = 0; o < 4; o++) for (let x = -1; x <= A.COLS; x++) {
          if (A.collide(pc, x, pc.y, o)) continue;
          const sv = { x: pc.x, o: pc.o }; pc.x = x; pc.o = o;
          const pv = A.preview(); const sc = pv.total * 0.06 + boardValue(pv.board);
          pc.x = sv.x; pc.o = sv.o;
          if (!best || sc > best.sc) best = { x, o, sc };
        }
        if (!best) break;
        pc.x = best.x; pc.o = best.o; A.drop(); if (guard % 40 === 0) A.draw();
      }
      const s = S();
      results.push({ reached: s.phase === 'win' ? 9 : s.round + 1, phase: s.phase, why: s.overWhy || '', rounds, best: s.best && `${s.best.total}점 ${s.best.chain}연쇄`, lastMaxChain: s.maxChain });
    }
    const hist = {}; results.forEach(r => hist[r.reached] = (hist[r.reached] || 0) + 1);
    const perRound = {}; results.forEach(r => r.rounds.forEach(x => (perRound[x.r] ||= []).push(x.score)));
    const avg = Object.fromEntries(Object.entries(perRound).map(([k, v]) => [k, Math.round(v.reduce((a, b) => a + b, 0) / v.length)]));
    const fails = {}; results.filter(r => r.phase === 'over').forEach(r => { const k = r.why.slice(0, 12); fails[k] = (fails[k] || 0) + 1; });
    const allRounds = results.flatMap(r => r.rounds);
    return { reachedHistogram: hist, avgClearScoreByRound: avg, failReasons: fails,
      burstsPerClearedRound: Math.round(allRounds.reduce((a, x) => a + x.bursts, 0) / Math.max(1, allRounds.length) * 10) / 10,
      maxChainSeen: Math.max(0, ...results.map(r => Math.max(r.lastMaxChain || 0, ...r.rounds.map(x => x.maxChain)))), sampleBest: results.slice(0, 5).map(r => r.best) };
  }, runs);
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
