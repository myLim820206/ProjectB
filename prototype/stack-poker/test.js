// 실행: node prototype/stack-poker/test.js "$(pwd)/prototype/stack-poker/index.html" [봇 판 수] [클릭 1번당 초] [족보 찾는 시간 초]
// 족보 판정(조커 포함), 고르기 규칙, 족보 내기·버리기·줄 폭발, 봇 시험.
// 봇: 각 줄 아래쪽 6장에서 점수가 가장 큰 족보를 찾아 낸다. 없으면 기다리고, 줄이 위험하면 맨 아래 1장을 버린다.
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(([runs, clickSec, thinkSec]) => {
    const A = window.__api, S = () => window.__sp(); A.setMuted(true);
    const step = sec => { for (let t = 0; t < sec - 1e-6; t += 0.05) A.update(Math.min(0.05, sec - t)); }; // 큰 시간을 잘게 나눠 진행
    const c = (r, s = '♠') => ({ r, s, y: 0 }), J = { joker: true, y: 0 };
    const cases = [];
    const t = (label, cards, want) => { const got = A.evalHand(cards)?.key || 'none'; cases.push({ label, got, ok: got === want }); };
    t('트리플', [c(5, '♥'), c(5, '♦'), c(5)], 'triple');
    t('원 페어', [c(4), c(4, '♥')], 'onePair');
    t('다른 숫자 2장은 안 됨', [c(4), c(5, '♥')], 'none');
    t('트리플 + 남는 카드는 안 됨', [c(5, '♥'), c(5, '♦'), c(5), c(2)], 'none');
    t('투 페어', [c(3, '♥'), c(6), c(3, '♦'), c(6, '♣')], 'twoPair');
    t('스트레이트 4장', [c(5, '♥'), c(3), c(6, '♦'), c(4, '♣')], 'straight');
    t('스트레이트 5장', [c(5, '♥'), c(3), c(6, '♦'), c(4, '♣'), c(7)], 'straight');
    t('포카드', [c(7), c(7, '♥'), c(7, '♦'), c(7, '♣')], 'four');
    t('풀하우스', [c(2), c(2, '♥'), c(6, '♦'), c(6, '♣'), c(2, '♦')], 'fullHouse');
    t('플러시', [c(1, '♥'), c(3, '♥'), c(5, '♥'), c(7, '♥'), c(2, '♥')], 'flush');
    t('스트레이트 플러시', [c(3, '♣'), c(4, '♣'), c(5, '♣'), c(6, '♣'), c(7, '♣')], 'straightFlush');
    t('조커 + 페어 = 트리플', [c(4), J, c(4, '♥')], 'triple');
    t('A는 1 (6 7 A 2 아님)', [c(6), c(7, '♥'), c(1, '♦'), c(2, '♣')], 'none');
    // 고르기: 고른 카드 내면 사라지고 아래 카드가 위로 붙음
    A.newGame(); const s = S(); s.phase = 'play';
    s.cols = [[c(1), c(2), c(6, '♥'), c(3), c(4)], [c(6, '♦'), c(5)], [c(6, '♣')], []];
    A.toggle(s.cols[0][0]); const picked = s.sel.length === 1; A.toggle(s.cols[0][0]); const topBlocked = picked && s.sel.length === 0; // 위쪽 카드도 고를 수 있고, 다시 누르면 빠짐
    A.toggle(s.cols[0][2]); A.toggle(s.cols[1][0]); A.toggle(s.cols[2][0]); A.playHand();
    const playOk = topBlocked && s.hands === 1 && s.cols[0].length === 4 && s.cols[0][2].r === 3 && s.score === (20 + 18) * 2 && s.combo === 1;
    A.update(1); A.toggle(s.cols[0][3]); const t0 = s.time; A.discard(); const discardOk = s.cols[0].length === 3 && s.time <= t0 - 3 && s.combo === 0;
    // 원 페어: 콤보 시간은 이어주고 단계는 안 올림
    A.newGame(); const s3 = S(); s3.phase = 'play'; s3.cols = [[c(2), c(2, '♥'), c(2, '♦'), c(5), c(5, '♥')], [], [], []];
    [0, 1, 2].forEach(k => A.toggle(s3.cols[0][k])); A.playHand(); A.update(3); const left = s3.comboT;
    A.toggle(s3.cols[0][0]); A.toggle(s3.cols[0][1]); A.playHand();
    const pairOk = s3.combo === 1 && left < 3 && s3.comboT > 4.9 && s3.hands === 2;
    // 줄 폭발
    A.newGame(); const s2 = S(); s2.phase = 'play'; s2.cols[1] = Array.from({ length: A.MAXH - 1 }, (_, i) => c(i % 7 + 1)); const t1 = s2.time; for (let k = 0; k < 300 && !s2.explodes; k++) A.push(); // 무작위 줄에 밀어 넣으니 가득 찬 줄에 닿을 때까지
    const explodeOk = s2.explodes >= 1 && s2.time <= t1 - 8;
    // 봇
    function subsets(arr, k, start = 0, cur = [], out = []) { if (cur.length === k) { out.push(cur.slice()); return out; } for (let i = start; i < arr.length; i++) { cur.push(arr[i]); subsets(arr, k, i + 1, cur, out); cur.pop(); } return out; }
    const res = []; const THINK = thinkSec;
    for (let r = 0; r < runs; r++) {
      A.newGame(); const g = S(); g.phase = 'play'; let guard = 0;
      while (g.phase === 'play' && guard++ < 2000) {
        const cand = g.cols.flatMap(col => col.slice(-6)); // 사람처럼 아래쪽 6장씩만 본다
        let best = null;
        for (const k of [5, 4, 3, 2]) for (const sub of subsets(cand, k)) { const h = A.evalHand(sub); if (h && (!best || h.score > best.h.score)) best = { h, sub }; }
        if (best) { step(THINK + clickSec * best.sub.length); if (g.phase !== 'play') break; if (best.sub.every(cc => A.pickable(cc))) { best.sub.forEach(cc => A.toggle(cc)); A.playHand(); } }
        else { step(0.5); if (g.phase !== 'play') break; const tall = g.cols.reduce((a, col, i) => col.length > g.cols[a].length ? i : a, 0); if (g.cols[tall].length >= A.MAXH - 2) { A.toggle(g.cols[tall].at(-1)); A.discard(); } }
        if (g.bomb >= 100 && g.cols.some(col => col.length > A.MAXH - 4)) A.bomb();
      }
      res.push({ score: g.score, hands: g.hands, maxCombo: g.maxCombo, explodes: g.explodes, discards: g.discards });
    }
    const avg = k => Math.round(res.reduce((a, x) => a + x[k], 0) / res.length * 10) / 10;
    return { cases, allOk: cases.every(x => x.ok), pairOk, playOk, discardOk, explodeOk,
      bot: { thinkSec, clickSec, avgScore: avg('score'), min: Math.min(...res.map(x => x.score)), max: Math.max(...res.map(x => x.score)), hands: avg('hands'), maxCombo: avg('maxCombo'), explodes: avg('explodes'), discards: avg('discards') } };
  }, [+(process.argv[3] || 10), +(process.argv[4] || 0.3), +(process.argv[5] || 2)]);
  console.log(JSON.stringify({ ...out, cases: out.cases.filter(x => !x.ok) }, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
