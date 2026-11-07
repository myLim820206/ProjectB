// 실행: node prototype/tetris-defense/bottest.js "$(pwd)/prototype/tetris-defense/index.html" [all]
// all을 주면 시작할 때 적 8종을 한꺼번에 소환한다.
// 휴리스틱 봇이 쌓기를 하고, 게임 시간을 빠르게 돌려 웨이브와 적, 포탑 로직을 검증한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1000, height: 700 } });
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]); if (process.argv[3]==='all') await p.evaluate(()=>window.__spawnAll=true);
  const res = await p.evaluate(async () => {
    const A = window.__api; const S = () => window.__td();
    A.newGame(); S().started = true; if (window.__spawnAll) { ["shield","spider","ghost","boss","bomb","digger","chute","grunt"].forEach((t,i)=>A.spawnEnemy(t, i)); }
    const log = []; let t = 0, nextMove = 0, crushes = 0, maxWave = 1, turretsMax = 0, enemiesSeen = new Set();
    function evalBoard(board) { let holes = 0, agg = 0, bump = 0, hs = []; for (let x = 0; x < A.COLS; x++) { let h = 0, seen = false; for (let y = 0; y < A.ROWS; y++) { if (board[y][x]) { if (!seen) { h = A.ROWS - y; seen = true; } } else if (seen) holes++; } hs.push(h); agg += h; } for (let x = 1; x < A.COLS; x++) bump += Math.abs(hs[x] - hs[x - 1]); let lines = board.filter(r => r.every(c => c)).length; return -0.51 * agg + 0.76 * lines - 0.36 * holes - 0.18 * bump; }
    function bestPlacement() {
      const s = S(), pc = s.piece; let best = null;
      for (let r = 0; r < 4; r++) for (let x = -2; x < A.COLS; x++) {
        if (A.collide(pc, x, pc.y, r)) continue; let y = pc.y; while (!A.collide(pc, x, y + 1, r)) y++;
        const bd = s.board.map(row => row.slice()); let ok = true;
        A.cellsOf(pc, x, y, r).forEach(([cx, cy]) => { if (cy < 0) ok = false; else bd[cy][cx] = 1; }); if (!ok) continue;
        const sc = evalBoard(bd); if (!best || sc > best.sc) best = { r, x, sc };
      }
      return best;
    }
    const dt = 1 / 60;
    for (let i = 0; i < 60 * 240; i++) { // 게임 시간 240초
      t += dt;
      if (t >= nextMove && !S().over) {
        const bp = bestPlacement();
        if (bp) { const pc = S().piece; while (pc.r !== bp.r) { const r0 = pc.r; A.rotate(1); if (pc.r === r0) break; } let guard = 20; while (pc.x !== bp.x && guard--) A.move(bp.x > pc.x ? 1 : -1); }
        A.hardDrop(); nextMove = t + 0.45;
      }
      A.update(dt);
      const s = S(); maxWave = Math.max(maxWave, s.wave); s.enemies.forEach(e => enemiesSeen.add(e.type));
      let tc = 0; s.board.forEach(r => r.forEach(c => { if (c && c.turret) tc++; })); turretsMax = Math.max(turretsMax, tc);
      if (i % (60 * 30) === 0) log.push({ sec: Math.round(t), wave: s.wave, core: s.core, lines: s.lines, kills: s.kills, enemies: s.enemies.length, turrets: tc, merges: s.merges, crushes: s.crushes });
      if (s.over) { log.push({ over: s.overWhy, sec: Math.round(t), wave: s.wave }); break; }
    }
    const s = S();
    return { log, final: { wave: s.wave, core: s.core, lines: s.lines, kills: s.kills, score: s.score, over: s.over }, maxWave, turretsMax, enemiesSeen: [...enemiesSeen] };
  });
  console.log(JSON.stringify(res, null, 1)); console.log('errors:', errors.length ? errors.slice(0, 5) : 'none');
  await b.close();
})();
