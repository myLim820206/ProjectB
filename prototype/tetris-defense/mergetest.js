// 실행: node prototype/tetris-defense/mergetest.js "$(pwd)/prototype/tetris-defense/index.html"
// 같은 모양 포탑 합체 규칙 검증: I 포탑 2개 + Z 포탑 1개가 있는 줄을 지우면
// 다음 I 블록이 Lv2 빙결 포탑, 다음 Z 블록이 Lv1 화염 포탑을 싣고 나와야 한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const r = await p.evaluate(() => {
    const A = window.__api; A.newGame(); const s = window.__td(); s.started = true;
    const T = (kind, lvl) => ({ kind, lvl, cd: 1, hp: 65, maxhp: 65 });
    for (let x = 0; x < 9; x++) s.board[19][x] = { c: 'I', iron: false, turret: null };
    s.board[19][0].turret = T('I', 1); s.board[19][4].turret = T('I', 1); s.board[19][7].turret = T('Z', 1);
    s.board[18][5] = { c: 'O', iron: false, turret: T('O', 1) };
    s.next = [{ t: 'I', c: 'I', turrets: [{ kind: 'I', lvl: 1, idx: 2 }], iron: false }, { t: 'O', c: 'O', turrets: [{ kind: 'O', lvl: 1, idx: 0 }], iron: false }, { t: 'T', c: 'T', turrets: [{ kind: 'T', lvl: 1, idx: 0 }], iron: false }];
    s.piece = { t: 'O', c: 'O', turrets: [], iron: false, r: 0, x: 7, y: 0 }; // O 블록은 (8,9)열을 채움 → (9,19)와 (9,18) 채움
    s.board[19][8] = null; // O 블록이 8, 9열 아래 두 칸을 채우도록
    A.hardDrop();
    const st = window.__td();
    return { lines: st.lines, merges: st.merges, poolAfter: { ...st.pool }, spawned: st.piece.t, spawnedTurret: st.piece.turrets, keptO: st.board[19][5]?.turret?.lvl ?? null };
  });
  console.log(JSON.stringify(r)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
