// 실행: node prototype/tetris-poker/handtest.js "$(pwd)/prototype/tetris-poker/index.html"
// v5 규칙 단위 시험: 카드 한 장 = 칸 2개. 터짐 조건(같은 숫자 3장+, 투 페어), 원 페어 안 터짐,
// 플러시 보너스, 연쇄 스트레이트, 연쇄, 받침 없는 카드 따로 떨어짐, 블록 회전 시 카드 유지
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api, B = A.ROWS - 1, SU = ['♠', '♥', '♦', '♣'];
    const empty = () => Array.from({ length: A.ROWS }, () => Array(A.COLS).fill(null));
    // v: 세로 카드 (x, y) 와 (x, y-1) / h: 가로 카드 (x, y) 와 (x+1, y)
    const v = (b, x, y, r, s) => { const c = { r, s }; b[y][x] = c; b[y - 1][x] = c; return c; };
    const h = (b, x, y, r, s) => { const c = { r, s }; b[y][x] = c; b[y][x + 1] = c; return c; };
    const cases = [];
    const test = (label, build, want, check) => { const b = empty(); build(b); const hs = A.detect(b); const keys = hs.map(x => x.key).sort();
      let ok = JSON.stringify(keys) === JSON.stringify(want.slice().sort()); if (check) ok = ok && check(hs, b); cases.push({ label, got: keys.join(',') || '없음', ok }); };
    test('세로 카드 3장 나란히 → 트리플', b => { v(b, 0, B, 5, '♠'); v(b, 1, B, 5, '♥'); v(b, 2, B, 5, '♦'); }, ['triple'], hs => !hs[0].flush && hs[0].cells.length === 6);
    test('가로 카드 3장 쌓기 → 트리플', b => { h(b, 0, B, 5, '♠'); h(b, 0, B - 1, 5, '♥'); h(b, 1, B - 2, 5, '♦'); }, ['triple']);
    test('한 칸만 맞닿아도 붙은 것', b => { h(b, 0, B, 3, '♠'); h(b, 1, B - 1, 3, '♥'); v(b, 3, B, 3, '♦'); }, ['triple']);
    test('포카드', b => { [0, 1, 2, 3].forEach(i => v(b, i, B, 3, SU[i])); }, ['four']);
    test('파이브 카드', b => { [0, 1, 2, 3, 4].forEach(i => v(b, i, B, 6, SU[i % 4])); }, ['five']);
    test('풀하우스 (트리플 + 맞닿은 페어)', b => { v(b, 0, B, 4, '♠'); v(b, 1, B, 4, '♥'); v(b, 2, B, 4, '♦'); h(b, 3, B, 7, '♣'); h(b, 3, B - 1, 7, '♠'); }, ['fullHouse'], hs => hs[0].cards.length === 5 && hs[0].cells.length === 10);
    test('투 페어 (맞닿은 두 페어)', b => { v(b, 0, B, 4, '♠'); v(b, 1, B, 4, '♥'); v(b, 2, B, 7, '♦'); v(b, 3, B, 7, '♣'); }, ['twoPair'], hs => hs[0].cards.length === 4);
    test('원 페어는 안 터짐', b => { v(b, 0, B, 4, '♠'); v(b, 1, B, 4, '♥'); }, []);
    test('떨어진 페어 두 쌍은 안 터짐', b => { v(b, 0, B, 4, '♠'); v(b, 1, B, 4, '♥'); v(b, 5, B, 7, '♦'); v(b, 6, B, 7, '♣'); }, []);
    test('대각선은 안 붙음', b => { h(b, 0, B, 2, '♠'); h(b, 2, B - 1, 2, '♥'); h(b, 4, B - 2, 2, '♦'); }, []);
    test('같은 무늬 트리플 → 플러시 ×2', b => { v(b, 2, B, 5, '♥'); v(b, 3, B, 5, '♥'); v(b, 4, B, 5, '♥'); }, ['triple'], hs => hs[0].flush && hs[0].mult === 4);
    const st = (arr) => A.chainStraight(arr.map(a => new Set(a)));
    const straight = { '3→4→5': st([[3], [4], [5]]), '5→4→3': st([[5], [4], [3]]), '3→5→4': st([[3], [5], [4]]), '2→3→4→5': st([[2, 6], [3], [4], [5]]), '2단계만': st([[3], [4]]) };
    // 받침 없는 가로 카드가 떨어진다: 아래 세로 카드 한쪽만 받치는 건 받침 있음
    const sb = empty(); v(sb, 0, B, 1, '♠'); const fl = h(sb, 4, B - 5, 2, '♥'); const held = h(sb, 0, B - 2, 3, '♦'); A.settle(sb);
    const m = A.cardsOf(sb); const settleOk = JSON.stringify(m.get(fl)) === JSON.stringify([[4, B], [5, B]]) && JSON.stringify(m.get(held)) === JSON.stringify([[0, B - 2], [1, B - 2]]);
    // 연쇄: 가로 2 카드 3장 트리플이 터지면 위의 가로 7 카드가 내려와 양옆 7과 트리플
    const cb = empty();
    h(cb, 1, B, 2, '♠'); h(cb, 1, B - 1, 2, '♥'); h(cb, 1, B - 2, 2, '♦');
    v(cb, 0, B, 7, '♣'); v(cb, 3, B, 7, '♥'); h(cb, 1, B - 3, 7, '♠');
    const sim = A.simulate(cb, {});
    // 블록 회전해도 카드 두 장이 각각 칸 2개 그대로
    const rotOk = Object.entries(A.SHAPES).every(([t, rs]) => rs.every(cells => { const k = (a, c) => Math.abs(a[0] - c[0]) + Math.abs(a[1] - c[1]) === 1; return k(cells[0], cells[1]) && k(cells[2], cells[3]) && new Set(cells.map(c => c + '')).size === 4; }));
    // 실제 진행: 블록 떨어뜨리고 미리보기
    A.newRun(); const s = window.__tp(); s.started = true; const pv = A.preview(); A.drop(); while (s.phase === 'chain') A.chainStep();
    const placed = A.cardsOf(s.board);
    return { cases, allOk: cases.every(c => c.ok), chainStraight: straight, settleOk, rotOk,
      chain: sim.steps.map(x => `${x.chain}단계 ${x.hands.join('+')} +${x.score}`), previewCards: pv.pos.map(c => c.length), placedCards: placed.size, placedCells: [...placed.values()].map(c => c.length) };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
