// 실행: node prototype/poker-defense/test.js "$(pwd)/prototype/poker-defense/index.html" [봇 판 수]
// 족보 판정, 줄 시너지(가로·세로 겹침, 큰 족보 우선), 설치·교체, 봇 시험
// 봇: 손패를 전부, 길을 많이 덮고 시너지가 커지는 자리에 하나씩 놓는다 (아끼지 않음)
// 4번째 인자 dumb: 족보를 생각하지 않고 길 근처 빈칸에 아무렇게나 놓는 봇 (비교용)
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(([runs, dumb]) => {
    const A = window.__api, S = () => window.__pd(); A.setMuted(true);
    const c = (r, s = '♠') => ({ r, s });
    const cases = [];
    const t = (label, cards, want) => { const got = A.windowHands(cards).sort().join(',') || 'none'; cases.push({ label, got, ok: got === want }); };
    t('원 페어', [c(7), c(7, '♥')], 'pair');
    t('떨어진 숫자 2장', [c(7), c(9, '♥')], 'none');
    t('이어진 숫자 2장 = 스트레이트', [c(4, '♥'), c(3, '♠')], 'straight');
    t('K-A도 이어짐', [c(13, '♥'), c(14, '♠')], 'straight');
    t('같은 무늬 2장은 아직 아님', [c(7, '♥'), c(9, '♥')], 'none');
    t('플러시 3장', [c(7, '♥'), c(9, '♥'), c(2, '♥')], 'flush');
    t('스트레이트 3장', [c(7, '♥'), c(9, '♠'), c(8, '♦')], 'straight');
    t('A-2-3 스트레이트', [c(14, '♥'), c(2, '♠'), c(3, '♦')], 'straight');
    t('Q-K-A 스트레이트', [c(12, '♥'), c(13, '♠'), c(14, '♦')], 'straight');
    t('트리플', [c(5), c(5, '♥'), c(5, '♦')], 'triple');
    t('투 페어', [c(5), c(9, '♥'), c(5, '♦'), c(9)], 'twoPair');
    t('포카드', [c(5), c(5, '♥'), c(5, '♦'), c(5, '♣')], 'four');
    t('풀하우스', [c(5), c(9, '♥'), c(5, '♦'), c(9), c(5, '♣')], 'fullHouse');
    t('스트레이트 플러시', [c(5), c(6), c(7), c(8), c(9)], 'flush,straight,straightFlush');
    // 줄 시너지: 가로 페어 + 세로 플러시가 한 타워에서 겹친다
    const tw = new Map(); const put = (x, y, card) => tw.set(x + ',' + y, { card, x, y });
    put(0, 0, c(7, '♥')); put(1, 0, c(7, '♠')); put(0, 1, c(2, '♥')); put(0, 2, c(11, '♥'));
    const syn = A.applyEffects(tw); const corner = tw.get('0,0');
    const crossOk = syn.length === 2 && corner.eff.hands.sort().join() === 'flush,pair' && corner.eff.dmg === 1.5 && corner.eff.splash === 1.25;
    // 큰 족보 우선: 7 7 7 이 한 줄이면 페어가 아니라 트리플
    const tw2 = new Map(); [0, 1, 2].forEach(x => tw2.set(x + ',5', { card: c(7, ['♠', '♥', '♦'][x]), x, y: 5 }));
    const tripleOk = A.applyEffects(tw2).map(s => s.key).join() === 'triple';
    // 설치와 교체 (길 위에는 못 놓음)
    A.newGame(true); const g = S(); g.hand = [c(9), c(9, '♥'), c(4, '♦')];
    const [px, py] = [...A.pathCells()][3].split(',').map(Number); const onPath = A.place(0, px, py); const p1 = A.place(0, 0, 0); const p2 = A.place(0, 1, 0);
    const placeOk = !onPath && p1 && p2 && g.syn.length === 1 && g.syn[0].key === 'pair' && g.hand.length === 1;
    A.place(0, 1, 0); const replaceOk = g.towers.get('1,0').card.r === 4 && g.syn.length === 0 && g.discarded === 1;
    // 무작위 길: 판마다 다르고, 길이가 범위 안이고, 왼쪽 밖에서 오른쪽 밖으로
    const paths = Array.from({ length: 200 }, () => A.genPath());
    const lens = paths.map(pp => pp.slice(1).reduce((a, q, i) => a + Math.abs(q[0] - pp[i][0]) + Math.abs(q[1] - pp[i][1]), 0));
    const pathOk = new Set(paths.map(pp => JSON.stringify(pp))).size > 150 && lens.every(l => l >= 34 && l <= 46) && paths.every(pp => pp[0][0] === -1 && pp.at(-1)[0] === A.COLS);
    // 봇
    let cells = [], pathPts = [];
    const coverage = (x, y, range) => { const [cx, cy] = A.center(x, y); return pathPts.filter(([px, py]) => Math.hypot(px - cx, py - cy) <= range * A.CS).length; };
    function power(towers) { A.applyEffects(towers); let sum = 0; for (const t of towers.values()) { const st = A.towerStats(t); sum += st.dmg * st.rate * (1 + st.splash * 0.8) * coverage(t.x, t.y, st.range); } return sum; }
    const res = [];
    for (let r = 0; r < runs; r++) {
      A.newGame(true); const gg = S(); let guard = 0;
      cells = []; for (let y = 0; y < A.ROWS; y++) for (let x = 0; x < A.COLS; x++) if (A.buildable(x, y)) cells.push([x, y]);
      pathPts = []; for (let d = 0; d < A.pathLen(); d += A.CS / 2) pathPts.push(A.posAt(d));
      while (gg.phase !== 'over' && gg.phase !== 'win' && guard++ < 200) {
        if (dumb) { // 족보를 생각 안 하는 봇: 길을 덮는 빈칸에 아무렇게나
          while (gg.hand.length) { const free = cells.filter(([x, y]) => !gg.towers.has(x + ',' + y) && coverage(x, y, 2.2) > 4); if (!free.length) break; const [x, y] = free[Math.random() * free.length | 0]; A.place(0, x, y); }
        }
        while (gg.hand.length) {
          let best = null;
          for (let i = 0; i < gg.hand.length; i++) for (const [x, y] of cells) { if (gg.towers.has(x + ',' + y)) continue;
            const tw3 = new Map([...gg.towers].map(([k, v]) => [k, { ...v }])); tw3.set(x + ',' + y, { card: gg.hand[i], x, y }); const v = power(tw3); if (!best || v > best.v) best = { v, i, x, y }; }
          if (!best) break; A.place(best.i, best.x, best.y);
        }
        A.startWave(); let k = 0; while (gg.phase === 'wave' && k++ < 20000) A.update(0.05);
      }
      res.push({ wave: gg.phase === 'win' ? 16 : gg.wave, lives: gg.lives, towers: gg.towers.size, hands: gg.syn.map(s => s.key) });
    }
    const hist = {}; res.forEach(x => hist[x.wave] = (hist[x.wave] || 0) + 1);
    return { cases, allOk: cases.every(x => x.ok), pathOk, crossOk, tripleOk, placeOk, replaceOk, bot: { reachedWave: hist, avgLives: Math.round(res.reduce((a, x) => a + x.lives, 0) / res.length * 10) / 10, sampleHands: res[0].hands } };
  }, [+(process.argv[3] || 10), process.argv[4] === 'dumb']);
  console.log(JSON.stringify({ ...out, cases: out.cases.filter(x => !x.ok) }, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
