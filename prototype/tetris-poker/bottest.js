// 실행: node prototype/tetris-poker/bottest.js "$(pwd)/prototype/tetris-poker/index.html" [판 수]
// 봇이 모든 자리(회전 × 열)를 따져 "예상 점수 + 쌓인 모양"이 가장 좋은 곳에 놓으며 라운드를 진행한다.
// 몇 라운드까지 가는지, 라운드별 점수와 목표를 기록해 밸런스를 본다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const runs = +(process.argv[3] || 20);
  const out = await p.evaluate((runs) => {
    const A = window.__api, S = () => window.__tp();
    function shape(board) { let holes = 0, agg = 0, bump = 0, hs = [];
      for (let x = 0; x < A.COLS; x++) { let h = 0, seen = false; for (let y = 0; y < A.ROWS; y++) { if (board[y][x]) { if (!seen) { h = A.ROWS - y; seen = true; } } else if (seen) holes++; } hs.push(h); agg += h; }
      for (let x = 1; x < A.COLS; x++) bump += Math.abs(hs[x] - hs[x - 1]); return -0.5 * agg - 4 * holes - 0.3 * bump; }
    const results = [];
    for (let run = 0; run < runs; run++) {
      A.newRun(); S().started = true; const rounds = [];
      let guard = 0;
      while (guard++ < 5000) {
        const s = S();
        if (s.phase === 'scoring') { A.finishScoring(); continue; }
        if (s.phase === 'clear') { rounds.push({ r: s.round + 1, score: s.score, target: A.ROUNDS[s.round].target, left: s.piecesLeft }); A.nextRound(); continue; }
        if (s.phase === 'over' || s.phase === 'win') break;
        const pc = s.piece; let best = null;
        const noRot = A.ROUNDS[s.round].boss === 'noRotate';
        for (let r = 0; r < (noRot ? 1 : 4); r++) for (let x = -2; x < A.COLS; x++) {
          if (A.collide(pc, x, pc.y, r)) continue;
          const save = { x: pc.x, r: pc.r }; pc.x = x; pc.r = r;
          const pv = A.preview(); const y = pv.y;
          const bd = s.board.map(row => row.slice()); A.cellsOf(pc, x, y, r).forEach(([cx, cy]) => bd[cy][cx] = 1);
          const remaining = bd.filter(row => !row.every(c => c));
          while (remaining.length < A.ROWS) remaining.unshift(Array(A.COLS).fill(null));
          const sc = (pv.total || 0) * 0.08 + shape(remaining);
          pc.x = save.x; pc.r = save.r;
          if (!best || sc > best.sc) best = { x, r, sc };
        }
        if (!best) { break; }
        pc.x = best.x; pc.r = best.r; A.drop();
      }
      const s = S();
      results.push({ reached: s.phase === 'win' ? 9 : s.round + 1, phase: s.phase, why: s.overWhy || '', rounds, lastScore: s.score, best: s.best && s.best.name });
    }
    const hist = {}; results.forEach(r => hist[r.reached] = (hist[r.reached] || 0) + 1);
    const perRound = {}; results.forEach(r => r.rounds.forEach(x => (perRound[x.r] ||= []).push(x.score)));
    const avg = Object.fromEntries(Object.entries(perRound).map(([k, v]) => [k, Math.round(v.reduce((a, b) => a + b, 0) / v.length)]));
    const fails = {}; results.filter(r => r.phase === 'over').forEach(r => { const k = r.why.slice(0, 12); fails[k] = (fails[k] || 0) + 1; });
    return { reachedHistogram: hist, avgClearScoreByRound: avg, failReasons: fails, sampleBest: results.slice(0, 5).map(r => r.best) };
  }, runs);
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
