// 실행: node prototype/stack-poker/shot.js "$(pwd)/prototype/stack-poker/index.html" 출력.png
// 봇으로 20초쯤 진행한 장면을 캡처한다
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage({ viewport: { width: 1280, height: 780 }, deviceScaleFactor: 2 });
  await p.goto('file://' + process.argv[2]);
  await p.evaluate(() => {
    const A = window.__api, g = window.__sp(); A.setMuted(true); g.phase = 'play';
    for (let k = 0; k < 26; k++) { A.update(0.8); let best = null;
      for (let i = 0; i < A.NCOL; i++) { const h = A.bestHand(g.cols[i].concat([g.cur])); const v = h ? h.score + 1000 : -g.cols[i].length * 10; if (!best || v > best.v) best = { v, i }; }
      if (k < 25) A.shoot(best.i); }
    // 지금 카드가 어딘가에서 터지는 순간까지 더 진행
    for (let k = 0; k < 80 && ![0, 1, 2].some(i => A.bestHand(g.cols[i].concat([g.cur]))); k++) { const i = g.cols.map(c => c.length).indexOf(Math.min(...g.cols.map(c => c.length))); A.shoot(i); A.update(0.3); }
    g.fx = []; g.texts = [];
    // 미리보기가 보이도록 지금 카드가 터지는 줄을 고른다
    for (let i = 0; i < A.NCOL; i++) if (A.bestHand(g.cols[i].concat([g.cur]))) g.sel = i;
  });
  await p.waitForTimeout(250); await p.screenshot({ path: process.argv[3] }); await b.close();
})();
