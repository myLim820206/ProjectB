// 실행: node prototype/stack-poker/test.js "$(pwd)/prototype/stack-poker/index.html" [봇 판 수]
// 족보 판정(조커 포함)과 봇 시험: 봇은 쏘면 터지는 줄 중 점수가 큰 곳, 없으면 가장 짧은 줄에 쏜다
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate((runs) => {
    const A = window.__api, S = () => window.__sp(); A.setMuted(true);
    const c = (r, s = '♠') => ({ r, s }), J = { joker: true };
    const cases = [];
    const t = (label, cards, want) => { const got = A.bestHand(cards)?.key || 'none'; cases.push({ label, got, ok: got === want }); };
    t('트리플', [c(2), c(5, '♥'), c(5, '♦'), c(5)], 'triple');
    t('원 페어는 안 터짐', [c(3), c(4, '♥'), c(4)], 'none');
    t('투 페어 (순서 상관없음)', [c(3, '♥'), c(6), c(3, '♦'), c(6, '♣')], 'twoPair');
    t('스트레이트 4장', [c(5, '♥'), c(3), c(6, '♦'), c(4, '♣')], 'straight');
    t('3장 연속은 안 터짐', [c(1, '♥'), c(3), c(4, '♦'), c(5, '♣')], 'none');
    t('포카드', [c(7), c(7, '♥'), c(7, '♦'), c(7, '♣')], 'four');
    t('풀하우스', [c(2), c(2, '♥'), c(6, '♦'), c(6, '♣'), c(2, '♦')], 'fullHouse');
    t('플러시 5장', [c(1, '♥'), c(3, '♥'), c(5, '♥'), c(7, '♥'), c(2, '♥')], 'flush');
    t('스트레이트 플러시', [c(3, '♣'), c(4, '♣'), c(5, '♣'), c(6, '♣'), c(7, '♣')], 'straightFlush');
    t('조커 + 페어 = 트리플', [c(4), J, c(4, '♥')], 'triple');
    t('조커로 스트레이트 빈칸', [c(2), c(3, '♥'), J, c(5, '♦')], 'straight');
    t('A는 1 (7-A 이어지지 않음)', [c(6), c(7, '♥'), c(1, '♦'), c(2, '♣')], 'none');
    // 실제 진행: 줄에 페어 두 장 뒤 같은 숫자 쏘기 → 트리플로 터지고 콤보 1
    A.newGame(); const s = S(); s.phase = 'play'; s.cols = [[c(1), c(6, '♥'), c(6, '♦')], [], []]; s.cur = c(6, '♣'); A.shoot(0);
    const shotOk = s.cols[0].length === 1 && s.combo === 1 && s.score === (20 + 18) * 2;
    // 위험: 줄이 가득 차면 폭발, 시간 -8
    A.newGame(); const s2 = S(); s2.phase = 'play'; s2.cols[1] = Array.from({ length: A.MAXH - 1 }, (_, i) => c(i % 7 + 1, ['♠', '♥'][i % 2])); const t0 = s2.time; A.push(); A.push(); A.push(); A.push(); A.push(); A.push();
    const dangerOk = s2.explodes >= 1 && s2.time <= t0 - 8;
    // 봇
    const res = [];
    for (let r = 0; r < runs; r++) {
      A.newGame(); const g = S(); g.phase = 'play'; let shots = 0;
      while (g.phase === 'play') {
        A.update(0.45); // 사람이 대략 0.45초마다 한 번 쏜다고 가정
        if (g.phase !== 'play') break;
        let best = null;
        for (const cand of [0, 1]) { // 0 지금 카드, 1 바꾼 카드
          const cardNow = cand ? g.next[0] : g.cur;
          for (let i = 0; i < A.NCOL; i++) { const h = A.bestHand(g.cols[i].concat([cardNow])); const v = h ? h.score + 1000 : -g.cols[i].length * 10 + (g.cols[i].length && g.cols[i].at(-1).r === cardNow.r ? 15 : 0);
            if (!best || v > best.v) best = { v, i, cand }; }
        }
        if (best.cand) A.swap(); A.shoot(best.i); shots++;
        if (g.bomb >= 100 && g.cols.some(cc => cc.length > A.MAXH - 4)) A.bomb();
      }
      res.push({ score: g.score, bursts: g.bursts, maxCombo: g.maxCombo, explodes: g.explodes, shots });
    }
    const avg = k => Math.round(res.reduce((a, x) => a + x[k], 0) / res.length);
    return { cases, allOk: cases.every(x => x.ok), shotOk, dangerOk, bot: { avgScore: avg('score'), min: Math.min(...res.map(x => x.score)), max: Math.max(...res.map(x => x.score)), avgBursts: avg('bursts'), avgShots: avg('shots'), avgMaxCombo: avg('maxCombo'), avgExplodes: Math.round(res.reduce((a, x) => a + x.explodes, 0) / res.length * 10) / 10 } };
  }, +(process.argv[3] || 30));
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
