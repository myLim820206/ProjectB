// 실행: node prototype/tetris-defense/mergetest.js "$(pwd)/prototype/tetris-defense/index.html"
// 줄 지우기 포탑 규칙 검증
//  1) 같은 종류 2개 → 합체해 판에 남음 (레벨 합산)
//  2) I + Z → 증기(V)로 합성
//  3) 규칙 없는 I + O → 둘 다 판에 남음
//  4) 세로줄에 블록이 없으면 1칸 포탑 블록으로 떨어짐
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api;
    const T = (kind, lvl = 1) => ({ kind, lvl, cd: 1, hp: 65, maxhp: 65 });
    function run(setup) {
      A.newGame(); const s = window.__td(); s.started = true;
      for (let x = 0; x < 8; x++) s.board[19][x] = { c: 'J', iron: false, turret: null };
      for (let x = 0; x < 10; x++) s.board[18][x] = x < 8 ? { c: 'S', iron: false, turret: null } : null; // 지운 줄 위 블록 (8, 9열은 비움)
      setup(s);
      s.piece = { t: 'O', c: 'O', turrets: [], iron: false, r: 0, x: 7, y: 0 }; // 8, 9열의 18, 19행을 채움 → 19행 지움
      A.hardDrop(); A.draw();
      const st = window.__td(), list = [];
      st.board.forEach((row, y) => row.forEach((c, x) => { if (c && c.turret) list.push(`${c.turret.kind}${c.turret.lvl}@${x},${y}`); }));
      return { lines: st.lines, turrets: list.sort() };
    }
    return {
      same: run(s => { s.board[19][1].turret = T('I'); s.board[19][5].turret = T('I', 2); }),
      fusion: run(s => { s.board[19][2].turret = T('I'); s.board[19][6].turret = T('Z'); }),
      noRule: run(s => { s.board[19][2].turret = T('I'); s.board[19][6].turret = T('O'); }),
      emptyColumn: run(s => { s.board[18][3] = null; s.board[19][3].turret = T('L'); }),
    };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
