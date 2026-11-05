// 헤드리스 Chromium으로 HD-2D 미리보기를 PNG로 렌더링한다.
// 사용법: node render.mjs [출력 경로] [가로] [세로] [페이지 파일, 기본 index.html]
import { createRequire } from 'node:module';
const { chromium } = createRequire(import.meta.url)('playwright');
import { spawn } from 'node:child_process';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const here = path.dirname(fileURLToPath(import.meta.url));
const out = path.resolve(process.argv[2] || path.join(here, '../../docs/images/hd2d-preview.png'));
const w = process.argv[3] || 1280, h = process.argv[4] || 720;
const pageFile = process.argv[5] || 'index.html';
const port = 8765;

const server = spawn('python3', ['-m', 'http.server', String(port), '--bind', '127.0.0.1'], { cwd: path.join(here, '..'), stdio: 'ignore' });
await new Promise(r => setTimeout(r, 800));
const browser = await chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
try {
  const page = await browser.newPage({ viewport: { width: +w, height: +h } });
  page.on('console', m => console.log('[page]', m.text()));
  page.on('pageerror', e => console.log('[error]', e.message));
  await page.goto(`http://127.0.0.1:${port}/preview/${pageFile}?w=${w}&h=${h}`);
  await page.waitForFunction(() => window.__done === true, null, { timeout: 120000 });
  await page.waitForTimeout(300);
  await page.screenshot({ path: out });
  console.log('saved', out);
} finally {
  await browser.close();
  server.kill();
}
