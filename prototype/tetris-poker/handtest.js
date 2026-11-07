// 실행: node prototype/tetris-poker/handtest.js "$(pwd)/prototype/tetris-poker/index.html"
// 족보 판정과 연쇄 단위 시험 (판 위에 카드를 직접 배치)
const { chromium } = require('playwright');
(async () => {
  const b = await chromium.launch(); const p = await b.newPage();
  const errors = []; p.on('pageerror', e => errors.push(e.message));
  await p.goto('file://' + process.argv[2]);
  const out = await p.evaluate(() => {
    const A = window.__api;
    const empty = () => Array.from({ length: A.ROWS }, () => Array(A.COLS).fill(null));
    const put = (b, x, y, r, s) => { b[y][x] = { t: 'T', card: { r, s } }; };
    // 무늬가 서로 다르게 돌아가는 깔개 (같은 무늬 5장이 이어지지 않게)
    const filler = ['♠', '♥', '♦', '♣'];
    const cases = [];
    const test = (label, build, want) => { const b = empty(); build(b); const keys = A.detect(b).map(h => h.key).sort(); cases.push({ label, got: keys.join(',') || '없음', ok: JSON.stringify(keys) === JSON.stringify(want.slice().sort()) }); };
    test('세로 트리플', b => { put(b, 2, 15, 5, '♠'); put(b, 2, 14, 5, '♥'); put(b, 2, 13, 5, '♦'); }, ['triple']);
    test('ㄱ자 포카드', b => { put(b, 2, 15, 3, '♠'); put(b, 3, 15, 3, '♥'); put(b, 4, 15, 3, '♦'); put(b, 4, 14, 3, '♣'); }, ['four']);
    test('파이브 카드', b => { [0, 1, 2, 3, 4].forEach((x, i) => put(b, x, 15, 6, filler[i % 4])); }, ['five']);
    test('플러시 (같은 무늬 5장 덩어리)', b => { [[0,15,1],[1,15,3],[2,15,6],[1,14,2],[1,13,7]].forEach(([x, y, r]) => put(b, x, y, r, '♥')); }, ['flush']);
    test('가로 스트레이트', b => { [1, 2, 3, 4, 5].forEach((r, i) => put(b, 3 + i, 15, r, filler[i % 4])); }, ['straight']);
    test('세로 내림 스트레이트', b => { [7, 6, 5, 4, 3].forEach((r, i) => put(b, 0, 11 + i, r, filler[i % 4])); }, ['straight']);
    test('스트레이트 플러시', b => { [2, 3, 4, 5, 6].forEach((r, i) => put(b, i, 15, r, '♣')); }, ['flush', 'straightFlush']);
    test('풀하우스 (트리플 + 맞닿은 페어)', b => { put(b, 0, 15, 4, '♠'); put(b, 1, 15, 4, '♥'); put(b, 2, 15, 4, '♦'); put(b, 3, 15, 7, '♣'); put(b, 3, 14, 7, '♠'); }, ['fullHouse']);
    test('페어만으로는 안 터짐', b => { put(b, 0, 15, 4, '♠'); put(b, 1, 15, 4, '♥'); }, []);
    test('떨어진 같은 숫자는 안 터짐', b => { put(b, 0, 15, 4, '♠'); put(b, 2, 15, 4, '♥'); put(b, 4, 15, 4, '♦'); }, []);
    test('대각선 연속 숫자는 스트레이트 아님', b => { [1, 2, 3, 4, 5].forEach((r, i) => put(b, i, 15 - i, r, filler[i % 4])); }, []);
    // 연쇄: 가운데 세로 2 트리플이 터지면 그 위의 7이 바닥까지 내려와 양옆 7과 가로 트리플이 된다
    const cb = empty();
    put(cb, 1, 15, 2, '♠'); put(cb, 1, 14, 2, '♥'); put(cb, 1, 13, 2, '♦');        // 1단계: 세로 2 트리플
    put(cb, 0, 15, 7, '♣'); put(cb, 2, 15, 7, '♥'); put(cb, 1, 12, 7, '♠');        // 처음엔 7들이 서로 떨어져 있음
    const sim = A.simulate(cb, {});
    return { cases, allOk: cases.every(c => c.ok), chain: { chain: sim.chain, steps: sim.steps.map(s => `${s.chain}단계 ${s.hands.join('+')} +${s.score}`), total: sim.total } };
  });
  console.log(JSON.stringify(out, null, 1)); console.log('errors:', errors.length ? errors : 'none'); await b.close();
})();
