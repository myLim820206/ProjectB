// 실행: node prototype/tetris-poker/handtest.js "$(pwd)/prototype/tetris-poker/index.html"
// v3 족보 판정과 연쇄 단위 시험 (8 × 12 판에 카드를 직접 배치)
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api, B = A.ROWS - 1; // 맨 아래 줄
    const empty = () => Array.from({ length: A.ROWS }, () => Array(A.COLS).fill(null));
    const put = (b, x, y, r, s) => { b[y][x] = { r, s }; };
    const F = ['♠', '♥', '♦', '♣'];
    const cases = [];
    const test = (label, build, want) => { const b = empty(); build(b); const keys = A.detect(b).map(h => h.key).sort(); cases.push({ label, got: keys.join(',') || '없음', ok: JSON.stringify(keys) === JSON.stringify(want.slice().sort()) }); };
    test('세로 트리플', b => { put(b, 2, B, 5, '♠'); put(b, 2, B - 1, 5, '♥'); put(b, 2, B - 2, 5, '♦'); }, ['triple']);
    test('ㄱ자 포카드', b => { put(b, 2, B, 3, '♠'); put(b, 3, B, 3, '♥'); put(b, 4, B, 3, '♦'); put(b, 4, B - 1, 3, '♣'); }, ['four']);
    test('파이브 카드', b => { [0, 1, 2, 3, 4].forEach((x, i) => put(b, x, B, 6, F[i % 4])); }, ['five']);
    test('플러시 덩어리', b => { [[0,B,1],[1,B,3],[2,B,6],[1,B-1,2],[1,B-2,7]].forEach(([x, y, r]) => put(b, x, y, r, '♥')); }, ['flush']);
    test('가로 스트레이트', b => { [1, 2, 3, 4, 5].forEach((r, i) => put(b, 2 + i, B, r, F[i % 4])); }, ['straight']);
    test('꺾인 스트레이트 (ㄴ자)', b => { put(b, 0, B - 2, 3, '♠'); put(b, 0, B - 1, 4, '♥'); put(b, 0, B, 5, '♦'); put(b, 1, B, 6, '♣'); put(b, 2, B, 7, '♠'); }, ['straight']);
    test('계단 스트레이트', b => { put(b, 0, B, 1, '♠'); put(b, 1, B, 2, '♥'); put(b, 1, B - 1, 3, '♦'); put(b, 2, B - 1, 4, '♣'); put(b, 2, B - 2, 5, '♠'); }, ['straight']);
    test('스트레이트 플러시', b => { [2, 3, 4, 5, 6].forEach((r, i) => put(b, i, B, r, '♣')); }, ['flush', 'straightFlush']);
    test('풀하우스', b => { put(b, 0, B, 4, '♠'); put(b, 1, B, 4, '♥'); put(b, 2, B, 4, '♦'); put(b, 3, B, 7, '♣'); put(b, 3, B - 1, 7, '♠'); }, ['fullHouse']);
    test('페어만으로는 안 터짐', b => { put(b, 0, B, 4, '♠'); put(b, 1, B, 4, '♥'); }, []);
    test('떨어진 같은 숫자는 안 터짐', b => { put(b, 0, B, 4, '♠'); put(b, 2, B, 4, '♥'); put(b, 4, B, 4, '♦'); }, []);
    test('끊긴 연속 숫자는 스트레이트 아님', b => { [1, 2, 3].forEach((r, i) => put(b, i, B, r, F[i])); [4, 5].forEach((r, i) => put(b, 5 + i, B, r, F[i])); }, []);
    // 연쇄: 가운데 세로 2 트리플이 터지면 그 위 7이 내려와 양옆 7과 가로 트리플
    const cb = empty();
    put(cb, 1, B, 2, '♠'); put(cb, 1, B - 1, 2, '♥'); put(cb, 1, B - 2, 2, '♦');
    put(cb, 0, B, 7, '♣'); put(cb, 2, B, 7, '♥'); put(cb, 1, B - 3, 7, '♠');
    const sim = A.simulate(cb, {});
    // 블록 착지: 가로로 놓인 2장 중 받침 없는 카드는 따로 떨어진다
    const s = window.__td ? null : window.__tp(); A.newRun(); const st = window.__tp();
    st.board[B][0] = { r: 1, s: '♠' }; // 0열에만 받침
    st.piece = { cards: [{ r: 2, s: '♥' }, { r: 3, s: '♦' }], x: 0, y: 1, o: 1 }; // 가로 (0열, 1열)
    const L = A.landing(st.piece);
    return { cases, allOk: cases.every(c => c.ok), chain: { chain: sim.chain, steps: sim.steps.map(x => `${x.chain}단계 ${x.hands.join('+')} +${x.score}`) }, split: { first: L.pos[0], second: L.pos[1] } };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
