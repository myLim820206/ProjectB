// 실행: node prototype/tetris-defense/linefreeze-test.js "$(pwd)/prototype/tetris-defense/index.html"
// 실제 키 입력으로 한 줄을 지운 뒤 게임 시간이 계속 흐르는지, 그리기 오류가 없는지 확인한다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1000, height: 700 } });
  const errors = []; p.on('pageerror', e => errors.push(e.stack || e.message));
  await p.goto('file://' + process.argv[2]);
  await p.keyboard.press('Enter');
  await p.waitForTimeout(300);
  await p.evaluate(() => { const s = window.__td(); for (let x = 0; x < 9; x++) s.board[19][x] = { c: "J", turret: x === 2 ? { kind: 'J', lvl: 1, cd: 1, hp: 65, maxhp: 65 } : null };
    s.piece = { t: 'I', c: 'I', turrets: [{ kind: "I", lvl: 1, idx: 0 }], r: 1, x: 7, y: 0 }; });
  const t0 = await p.evaluate(() => window.__td().time);
  await p.keyboard.press(' ');
  await p.waitForTimeout(1500);
  const st = await p.evaluate(() => ({ time: window.__td().time, lines: window.__td().lines, over: window.__td().over }));
  console.log('t0', t0.toFixed(2), JSON.stringify(st)); console.log('errors:', errors.length ? errors : 'none');
  await b.close();
})();
