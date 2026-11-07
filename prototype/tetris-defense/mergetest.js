// 실행: node prototype/tetris-defense/mergetest.js "$(pwd)/prototype/tetris-defense/index.html"
// v3 규칙 검증
//  wall: 줄을 지우면 전장의 같은 자리에 벽, 포탑도 같은 칸
//  stack: 같은 자리에 또 생기면 같은 세로줄 바로 위 빈칸에 쌓임
//  merge / fusion: 같은 줄의 같은 종류 합체, 빙결+화염 → 증기
//  gap: 벽에 틈이 있으면 부수지 않고 틈으로 지나감
//  weakest: 막혔을 때 주변 8칸 중 체력이 가장 낮은 블록부터 부숨
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api, S = () => window.__td();
    function clearOne(turrets) {
      const s = S();
      for (let x = 0; x < 8; x++) s.board[19][x] = { c: 'J', turret: null };
      turrets.forEach(([x, kind, lvl]) => s.board[19][x].turret = { kind, lvl });
      s.piece = { t: 'O', turrets: [], r: 0, x: 7, y: 0 }; // 8, 9열의 18, 19행을 채워 19행만 지움
      A.hardDrop();
    }
    const fieldRow = y => S().field[y].map(c => c ? (c.turret ? c.turret.kind + c.turret.lvl : '#') : '.').join('');
    const res = {};
    A.newGame(); S().started = true; S().breakT = 999;
    clearOne([[1, 'I', 1], [5, 'I', 2], [7, 'Z', 1]]);
    res.wall = { row19: fieldRow(19) };
    clearOne([[2, 'L', 1], [6, 'S', 1]]);
    res.stack = { row18: fieldRow(18), row19: fieldRow(19) };
    A.newGame(); S().started = true; S().breakT = 999;
    clearOne([[2, 'I', 1], [6, 'Z', 1]]);
    res.fusion = { row19: fieldRow(19) };
    // gap: 10행에 9칸 벽, 9열만 비움. 적은 부수지 않고 돌아가야 한다
    A.newGame(); let s = S(); s.started = true; s.breakT = 999;
    for (let x = 0; x < 9; x++) s.field[10][x] = { c: 'T', turret: null, hp: 60, maxhp: 60 };
    A.spawnEnemy('grunt'); s.enemies[0].gx = 0; s.enemies[0].gy = 5; s.enemies[0].px = 0; s.enemies[0].py = 5; s.flowDirty = true;
    for (let i = 0; i < 60 * 15 && s.enemies.length; i++) { A.update(1 / 60); s.piece.y = 0; }
    res.gap = { wallIntact: S().field[10].slice(0, 9).every(c => c && c.hp === 60), coreAfter: S().core };
    // weakest: 10행 꽉 찬 벽. 적 바로 아래 세 칸의 체력 80, 25, 90 → 체력 25 칸부터 부숴야 한다
    A.newGame(); s = S(); s.started = true; s.breakT = 999;
    for (let x = 0; x < 10; x++) s.field[10][x] = { c: 'T', turret: null, hp: 200, maxhp: 200 };
    s.field[10][3].hp = 80; s.field[10][4].hp = 90; s.field[10][5].hp = 25;
    A.spawnEnemy('grunt'); const e = s.enemies[0]; e.gx = 4; e.gy = 9; e.px = 4; e.py = 9; s.flowDirty = true;
    let firstBroken = null;
    for (let i = 0; i < 60 * 30 && !firstBroken; i++) { A.update(1 / 60); s.piece.y = 0; for (let x = 0; x < 10; x++) if (!S().field[10][x]) { firstBroken = x; break; } }
    res.weakest = { firstBroken, expected: 5 };
    A.draw();
    return res;
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
