// 실행: node prototype/poker-defense/test.js "$(pwd)/prototype/poker-defense/index.html" [봇 판 수]
// 족보 판정, 붙은 모양 시너지(ㄴ자), 연쇄, 큰 족보 우선, 설치·교체, 무작위 길, 봇 시험
// 봇: 손패를 전부, 길을 많이 덮고 시너지가 커지는 자리에 하나씩 놓는다 (아끼지 않음)
// 4번째 인자 dumb: 족보를 생각하지 않고 길 근처 빈칸에 아무렇게나 놓는 봇 (비교용), 5번째 인자 보상 고르기: joker(기본) / random / none
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(([runs, dumb, rewardMode]) => {
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
    // ㄴ자 플러시: 하트 3장이 꺾여 붙어도 플러시
    const tw = new Map(); const put = (m, x, y, card) => m.set(x + ',' + y, { card, x, y });
    put(tw, 0, 0, c(7, '♥')); put(tw, 0, 1, c(2, '♥')); put(tw, 1, 1, c(11, '♥'));
    const lOk = A.applyEffects(tw).map(s => s.key + s.n).join() === 'flush3';
    // 연쇄: 스트레이트(7 8)와 트리플(5 5 5)이 맞닿으면 둘 다 연쇄 2 → 공격력 ×1.25 추가
    // (7 7 5 5 5 였다면 풀하우스 하나가 더 크므로 그것을 고른다)
    const tc = new Map(); put(tc, 0, 3, c(7)); put(tc, 1, 3, c(8, '♥')); put(tc, 2, 3, c(5)); put(tc, 3, 3, c(5, '♥')); put(tc, 4, 3, c(5, '♦'));
    const syn2 = A.applyEffects(tc);
    const fh = new Map(); put(fh, 0, 3, c(7)); put(fh, 1, 3, c(7, '♥')); put(fh, 2, 3, c(5)); put(fh, 3, 3, c(5, '♥')); put(fh, 4, 3, c(5, '♦'));
    const crossOk = syn2.length === 2 && syn2.every(s => s.chain === 2) && tc.get('0,3').eff.dmg === 1.25 && tc.get('4,3').eff.dmg === 2 * 1.25
      && A.applyEffects(fh).map(s => s.key).join() === 'fullHouse';
    // 떨어져 있으면 연쇄 아님
    const td = new Map(); put(td, 0, 6, c(7)); put(td, 1, 6, c(7, '♥')); put(td, 3, 6, c(5)); put(td, 4, 6, c(5, '♥'));
    const noChainOk = A.applyEffects(td).every(s => s.chain === 1);
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
    // 와일드 카드: 7 + 와일드 = 페어, 하트 둘 + 와일드 = 플러시, 4 5 + 와일드 = 스트레이트(3장 포함)
    const W = { r: 0, s: '★', wild: true };
    const wildOk = A.windowHands([c(7), W]).includes('pair') && A.windowHands([c(2, '♥'), c(9, '♥'), W]).includes('flush') && A.windowHands([c(4), c(5, '♦'), W]).includes('straight');
    // 방패: 한 방 피해를 깎는다 (최소 25%), 관통탄 조커면 무시
    A.newGame(true); const ge = S(); const ar = A.makeEnemy('armored', 100, 10); ar.maxHp = ar.hp; ar.d = 0;
    ge.enemies = [ar]; A.hit(ar, 20, 0); const afterArmor = 130 - ar.hp; ge.jokers = ['pierce']; A.hit(ar, 20, 0); const afterPierce = 130 - ar.hp - afterArmor;
    const armorOk = Math.abs(afterArmor - (20 - 10)) < 1e-6 && afterPierce === 20;
    // 분열: 죽으면 작은 적 둘
    A.newGame(true); const gs = S(); const sp = A.makeEnemy('splitter', 50, 6); sp.maxHp = sp.hp; sp.d = 100; gs.enemies = [sp]; A.hit(sp, 999, 0);
    const splitOk = gs.newEnemies && gs.newEnemies.length === 2 && gs.newEnemies.every(m => m.type === 'mini');
    // 재생: 체력이 찬다
    A.newGame(true); const gr = S(); gr.phase = 'wave'; gr.spawn = [{ type: 'normal', hp: 1, speed: 0 }]; const rgE = A.makeEnemy('regen', 100, 8); rgE.maxHp = rgE.hp; rgE.hp = 50; rgE.d = 0; rgE.speed = 0; gr.enemies = [rgE];
    A.update(1); const regenOk = rgE.hp > 50 && rgE.hp < 60;
    // 조커: 하트 애호가 공격력 ×1.4, 큰 손 손패 +2, 보상 받기
    A.newGame(true); const gj = S(); const tH = { card: c(9, '♥'), x: 0, y: 0, eff: null }; const base = A.towerStats(tH).dmg;
    gj.phase = 'reward'; gj.rewards = [{ kind: 'joker', id: 'heartLover' }]; A.takeReward(0);
    const jokerOk = Math.abs(A.towerStats(tH).dmg - base * 1.4) < 1e-9 && gj.phase === 'build' && gj.jokers.length === 1;
    // 보상 후보: 조커 2 + 즉시 1, 조커가 5칸이면 즉시만
    A.newGame(true); const gq = S(); const r1 = A.rollRewards(); gq.jokers = ['twins', 'scope', 'supply', 'lucky7', 'frost']; const r2 = A.rollRewards();
    const rewardOk = r1.filter(r => r.kind === 'joker').length === 2 && r1.length === 3 && r2.every(r => r.kind === 'instant');
    // 웨이브 5·10·15 보스
    const bossOk = A.waveSpec(5).some(e => e.type === 'shieldBoss') && A.waveSpec(10).some(e => e.type === 'splitBoss') && A.waveSpec(15).some(e => e.type === 'regenBoss') && !A.waveSpec(7).some(e => A.ENEMY[e.type].boss);
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
          const free = cells.filter(([x, y]) => !gg.towers.has(x + ',' + y));
          const nearT = free.filter(([x, y]) => [[1, 0], [-1, 0], [0, 1], [0, -1]].some(([dx, dy]) => gg.towers.has((x + dx) + ',' + (y + dy))));
          const topCov = free.slice().sort((a, b) => coverage(b[0], b[1], 2.4) - coverage(a[0], a[1], 2.4)).slice(0, 14);
          const cand = [...new Set([...nearT, ...topCov])];
          for (let i = 0; i < gg.hand.length; i++) for (const [x, y] of cand) {
            const tw3 = new Map([...gg.towers].map(([k, v]) => [k, { ...v }])); tw3.set(x + ',' + y, { card: gg.hand[i], x, y }); const v = power(tw3); if (!best || v > best.v) best = { v, i, x, y }; }
          if (!best) break; A.place(best.i, best.x, best.y);
        }
        A.startWave(); let k = 0; while (gg.phase === 'wave' && k++ < 20000) A.update(0.05);
        if (gg.phase === 'reward') { // 보상: rewards 인자에 따라 고르기 (none = 건너뛰기, random = 아무거나, joker = 조커 먼저)
          const rs = gg.rewards; let pick = -1;
          if (rewardMode === 'random') pick = Math.random() * rs.length | 0;
          else if (rewardMode === 'joker') { pick = rs.findIndex(r => r.kind === 'joker'); if (pick < 0) pick = rs.findIndex(r => r.id === 'repair'); }
          A.takeReward(pick);
        }
      }
      res.push({ wave: gg.phase === 'win' ? 16 : gg.wave, lives: gg.lives, towers: gg.towers.size, hands: gg.syn.map(s => s.key) });
    }
    const hist = {}; res.forEach(x => hist[x.wave] = (hist[x.wave] || 0) + 1);
    return { cases, allOk: cases.every(x => x.ok), wildOk, armorOk, splitOk, regenOk, jokerOk, rewardOk, bossOk, pathOk, lOk, crossOk, noChainOk, tripleOk, placeOk, replaceOk, bot: { reachedWave: hist, avgLives: Math.round(res.reduce((a, x) => a + x.lives, 0) / res.length * 10) / 10, sampleHands: res[0].hands } };
  }, [+(process.argv[3] || 10), process.argv[4] === 'dumb', process.argv[5] || 'joker']);
  console.log(JSON.stringify({ ...out, cases: out.cases.filter(x => !x.ok) }, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
