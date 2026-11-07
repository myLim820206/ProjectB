// 실행: node prototype/tetris-poker/handtest.js "$(pwd)/prototype/tetris-poker/index.html"
// 족보 계산 단위 시험
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api;
    const row = str => str.split(' ').map(t => ({ r: ({ J: 11, Q: 12, K: 13, A: 14 })[t.slice(0, -1)] || +t.slice(0, -1), s: t.slice(-1) }));
    const cases = [
      ['스트레이트 플러시', '9♥ 10♥ J♥ Q♥ K♥ 2♣ 3♦ 7♠ 4♣ 8♦', 'straightFlush'],
      ['포카드', '7♠ 7♥ 2♣ 7♦ 9♣ 7♣ 3♦ K♠ 4♣ 5♦', 'four'],
      ['풀하우스', '5♠ 5♥ 5♦ 9♣ 9♦ 2♣ K♦ 7♠ 4♣ J♦', 'fullHouse'],
      ['플러시 (흩어져 있어도 인정)', '2♠ 9♥ 5♠ K♣ 8♠ 3♦ J♠ 7♥ Q♠ 4♦', 'flush'],
      ['스트레이트 (순서대로)', '3♠ 4♥ 5♦ 6♣ 7♠ K♦ 2♣ 9♠ J♣ Q♦', 'straight'],
      ['내림차순 스트레이트', 'K♠ Q♥ J♦ 10♣ 9♠ 2♦ 4♣ 6♠ 8♣ 3♦', 'straight'],
      ['A-2-3-4-5', 'A♠ 2♥ 3♦ 4♣ 5♠ K♦ 9♣ J♠ 7♣ Q♦', 'straight'],
      ['흩어진 연속 숫자는 스트레이트 아님', '3♠ K♥ 4♦ Q♣ 5♠ 9♦ 6♣ 2♠ 7♣ J♦', 'high'],
      ['트리플', '8♠ 8♥ 8♦ 2♣ 4♦ 6♣ K♦ J♠ 3♣ Q♦', 'three'],
      ['투 페어', '8♠ 8♥ 2♦ 2♣ 4♦ 6♣ K♦ J♠ 3♣ Q♦', 'twoPair'],
      ['원 페어', '8♠ 8♥ 2♦ 5♣ 4♦ 6♣ K♦ J♠ 3♣ 10♦', 'pair'],
    ];
    const res = cases.map(([label, str, want]) => { const r = A.evaluateRow(row(str)); return { label, got: r.key, ok: r.key === want, score: `${r.chips}×${r.mult}=${r.score}` }; });
    const multi = A.scoreRows([row('8♠ 8♥ 2♦ 5♣ 4♦ 6♣ K♦ J♠ 3♣ 10♦'), row('8♠ 8♥ 2♦ 2♣ 4♦ 6♣ K♦ J♠ 3♣ Q♦')]);
    const noRed = A.evaluateRow(row('9♥ 10♥ J♥ Q♥ K♥ 2♣ 3♦ 7♠ 4♣ 8♦'), { noRed: true });
    return { res, allOk: res.every(r => r.ok), twoLines: { total: multi.total, lineMult: multi.lineMult }, noRedSF: `${noRed.chips}×${noRed.mult}` };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
