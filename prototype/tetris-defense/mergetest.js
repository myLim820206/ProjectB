// 실행: node prototype/tetris-defense/mergetest.js "$(pwd)/prototype/tetris-defense/index.html"
// v2 규칙 검증: 줄을 지우면 그 줄의 포탑이 전장에 배치된다
//  1) 같은 종류 2개 → 합체해 전장에 1개 (레벨 합산)
//  2) 빙결 + 화염 → 증기로 합성
//  3) 규칙 없는 빙결 + 번개 → 둘 다 전장에
//  4) 전장이 꽉 차면 같은 종류 중 가장 약한 포탑을 강화
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api;
    function run(turrets, prefill) {
      A.newGame(); const s = window.__td(); s.started = true;
      if (prefill) prefill(s);
      for (let x = 0; x < 8; x++) s.board[19][x] = { c: 'J', turret: null };
      turrets.forEach(([x, kind, lvl]) => s.board[19][x].turret = { kind, lvl });
      s.board[18][8] = { c: 'S', turret: null }; s.board[18][9] = { c: 'S', turret: null };
      s.piece = { t: 'O', turrets: [], r: 0, x: 7, y: 0 }; // 8, 9열의 17, 18행... O는 18행 위에 얹힘
      s.board[18][8] = null; s.board[18][9] = null;      // O가 18, 19행의 8, 9열을 채워 19행만 지움
      A.hardDrop(); A.draw();
      const f = []; window.__td().field.forEach((row, y) => row.forEach((t, x) => { if (t && !(t.kind === 'S' && t.lvl === 1)) f.push(`${t.kind}${t.lvl}`); }));
      return { lines: window.__td().lines, field: f.sort() };
    }
    return {
      same: run([[1, 'I', 1], [5, 'I', 2]]),
      fusion: run([[2, 'I', 1], [6, 'Z', 1]]),
      noRule: run([[2, 'I', 1], [6, 'O', 1]]),
      fullField: run([[3, 'L', 2]], s => { let first = true; for (let y = 0; y < 20; y++) for (let x = 0; x < 9; x++) if (A.SLOT_D[y][x] >= 0) { s.field[y][x] = { kind: first ? 'L' : 'S', lvl: 1, cd: 1, hp: 65, maxhp: 65 }; first = false; } }),
    };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
