// 실행: node prototype/tetris-poker/flowtest.js "$(pwd)/prototype/tetris-poker/index.html"
// 1) 줄을 지워도 멈추지 않고 다음 블록이 바로 나온다  2) 연달아 지우면 콤보 배수  3) 블록이 저절로 떨어진다
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 760 } });
  const errors = []; p.on('pageerror', e => errors.push(e.message)); p.on('console', m => { if (m.type() === 'error') errors.push(m.text()); });
  await p.goto('file://' + process.argv[2]);
  await p.keyboard.press('Enter'); await p.waitForTimeout(200);
  const r = await p.evaluate(() => {
    const A = window.__api, s = window.__tp();
    A.ROUNDS[0].target = 1e9; // 라운드가 끝나지 않게
    const card = (r, su) => ({ r, s: su });
    const fillRow = (y, base) => { for (let x = 0; x < 9; x++) s.board[y][x] = { t: 'J', card: card(2 + ((x + base) % 12), ['♠','♥','♦','♣'][x % 4]) }; };
    fillRow(15, 0); fillRow(14, 3);
    s.piece = { t: 'I', cards: [card(5,'♠'), card(6,'♥'), card(7,'♦'), card(8,'♣')], r: 1, x: 7, y: 0 };
    const y0 = s.piece.y; for (let i = 0; i < 60; i++) A.update(1 / 60); const fell = s.piece.y > y0;
    s.piece.x = 7; s.piece.r = 1; A.drop();
    const after1 = { phase: s.phase, nextPieceReady: !!s.piece, combo: s.combo, lines: s.lines, fxRows: s.fx.filter(f => f.kind === 'row').length };
    // 바로 이어서 두 번째로 지우기: 남은 9열 I 조각 옆을 채우고 아무 블록이나 고정시킨다
    fillRow(15, 5); fillRow(14, 7);
    s.piece.x = 0; s.piece.r = 0; A.drop();
    const after2 = { phase: s.phase, combo: s.combo, comboMult: s.lastResult.comboMult, lines: s.lines };
    return { fell, after1, after2 };
  });
  await p.waitForTimeout(400); await p.screenshot({ path: process.argv[3] || '/dev/null' });
  console.log(JSON.stringify(r)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
