const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const r = await p.evaluate(() => {
    const A = window.__api, S = window.__td(); A.newGame(); const s = window.__td(); s.started = true;
    for (let x = 0; x < 9; x++) s.board[19][x] = { c: 'I', iron: false, turret: null };
    s.board[19][0].turret = { kind: 'basic', lvl: 1, cd: 1, hp: 65, maxhp: 65 };
    s.board[19][3].turret = { kind: 'spin', lvl: 2, cd: 1, hp: 90, maxhp: 90 };
    s.board[18][5] = { c: 'O', iron: false, turret: { kind: 'basic', lvl: 1, cd: 1, hp: 65, maxhp: 65 } }; // 남는 줄의 포탑
    s.piece = { t: 'P', c: 'Z', turrets: [], iron: false, r: 0, x: 8, y: 0 };
    A.hardDrop();
    const nxt = window.__td().next[0], cur = window.__td().piece;
    const kept = window.__td().board[19][5];
    return { lines: window.__td().lines, merges: window.__td().merges, nextPiece: cur.t, nextTurrets: cur.turrets, keptTurret: kept && kept.turret ? kept.turret.lvl : null };
  });
  console.log(JSON.stringify(r)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
