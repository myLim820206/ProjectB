// HD-2D 미리보기 장면.
// 1 유닛 = 16 픽셀. 블록은 픽셀 텍스처를 입힌 큐브, 캐릭터와 소품은 세워 둔 스프라이트 평면.
// 언리얼에서 구현할 구성(Lit 마스크 스프라이트, 동적 광원, 틸트 시프트 DOF, 블룸)을 그대로 흉내 낸다.
import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { HorizontalTiltShiftShader } from 'three/addons/shaders/HorizontalTiltShiftShader.js';
import { VerticalTiltShiftShader } from 'three/addons/shaders/VerticalTiltShiftShader.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';

const params = new URLSearchParams(location.search);
const W = +(params.get('w') || 1280), H = +(params.get('h') || 720);
const ASSET = '../pixelart/out/';
const PX = 16;

const renderer = new THREE.WebGLRenderer({ antialias: false, preserveDrawingBuffer: true });
renderer.setSize(W, H);
renderer.setPixelRatio(1);
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.05;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
scene.fog = new THREE.Fog(0x5a4678, 24, 46);

const camera = new THREE.PerspectiveCamera(24, W / H, 0.5, 200);
camera.position.set(0, 9.0, 19);
camera.lookAt(0, 1.6, -1.5);

// ---------------------------------------------------------------- 텍스처
const loader = new THREE.TextureLoader();
const cache = {};
function tex(name) {
  if (!cache[name]) {
    const t = loader.load(ASSET + name + '.png');
    t.magFilter = THREE.NearestFilter;
    t.minFilter = THREE.NearestFilter;
    t.generateMipmaps = false;
    t.colorSpace = THREE.SRGBColorSpace;
    cache[name] = t;
  }
  return cache[name];
}
const matCache = {};
function mat(name, extra = {}) {
  const key = name + JSON.stringify(extra);
  if (!matCache[key]) matCache[key] = new THREE.MeshStandardMaterial({ map: tex(name), roughness: 0.95, metalness: 0, ...extra });
  return matCache[key];
}

// ---------------------------------------------------------------- 하늘과 원경
function skyTexture() {
  const c = document.createElement('canvas'); c.width = 4; c.height = 256;
  const g = c.getContext('2d');
  const grd = g.createLinearGradient(0, 0, 0, 256);
  grd.addColorStop(0.0, '#2a2350'); grd.addColorStop(0.45, '#7a4c86');
  grd.addColorStop(0.75, '#e2876a'); grd.addColorStop(1.0, '#f7c27a');
  g.fillStyle = grd; g.fillRect(0, 0, 4, 256);
  const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
}
scene.background = skyTexture();

// 원경 산: 들쭉날쭉한 윗선을 가진 평면 도형 (텍스처 없이 단색)
function ridge(z, color, seed, width, base, amp, step) {
  let s = seed; const rnd = () => (s = (s * 9301 + 49297) % 233280) / 233280;
  const shape = new THREE.Shape();
  shape.moveTo(-width / 2, -6);
  let h = base;
  for (let x = -width / 2; x <= width / 2; x += step) {
    h = Math.max(base - amp, Math.min(base + amp, h + (rnd() - 0.5) * amp));
    shape.lineTo(x, h); shape.lineTo(x + step, h);
  }
  shape.lineTo(width / 2, -6);
  const m = new THREE.Mesh(new THREE.ShapeGeometry(shape), new THREE.MeshBasicMaterial({ color, fog: false }));
  m.position.set(0, 0, z); scene.add(m);
}
ridge(-48, 0xc07a8a, 9, 140, 12, 4, 2.0);
ridge(-38, 0x8a5a84, 3, 120, 8.5, 3, 1.5);
ridge(-28, 0x5a4274, 5, 100, 6, 2, 1.0);
ridge(-20, 0x3e3260, 11, 80, 4.2, 1.2, 1.0);

// ---------------------------------------------------------------- 지형 블록
const box = new THREE.BoxGeometry(1, 1, 1);
function block(x, y, z, side, top, bottom = 'tile_dirt') {
  const s = mat(side), t = mat(top), b = mat(bottom);
  const m = new THREE.Mesh(box, [s, s, t, b, s, s]);
  m.position.set(x + 0.5, y - 0.5, z + 0.5);
  m.castShadow = true; m.receiveShadow = true;
  scene.add(m);
}
const X0 = -12, X1 = 12, Z0 = -12, Z1 = 7;
const path = new Set();
for (let x = X0; x < X1; x++) { const z = Math.round(1.2 - x * 0.18); path.add(x + ',' + z); path.add(x + ',' + (z + 1)); }
for (let x = X0; x < X1; x++) {
  for (let z = Z0; z < Z1; z++) {
    let hgt = 0;
    if (z <= -3 && x < -1) hgt = 1;                      // 뒤쪽 언덕
    if (z <= -4 && x >= 4) hgt = 1;
    if (z <= -8) hgt = 2;
    const top = path.has(x + ',' + z) && hgt === 0 ? 'tile_path' : 'tile_grass_top';
    block(x, hgt, z, 'tile_grass_side', top);
    if (hgt > 0) block(x, hgt - 1, z, 'tile_dirt', 'tile_dirt');
  }
}
// 무너진 돌담
const wall = [3, 4, 4, 3, 2, 3, 1];
wall.forEach((hgt, i) => {
  for (let y = 0; y < hgt; y++) {
    const side = (y + i) % 3 === 0 ? 'tile_stone_brick_moss' : 'tile_stone_brick';
    block(-8 + i, 2 + y, -5, side, 'tile_stone_brick');
  }
});
// 광석 바위
[[5, 2, -5, 'tile_ore_blue'], [6, 2, -5, 'tile_stone_rough'], [6, 3, -5, 'tile_ore_red'], [7, 2, -5, 'tile_ore_green'], [7, 2, -4, 'tile_stone_rough']]
  .forEach(([x, y, z, t]) => block(x, y, z, t, 'tile_stone_rough', 'tile_stone_rough'));
// 상자 (3D 큐브)
[[3, 1, -1], [4, 1, -1], [3.5, 2, -1]].forEach(([x, y, z]) => block(x, y, z, 'tile_crate', 'tile_crate', 'tile_crate'));

// ---------------------------------------------------------------- 스프라이트
function sprite(name, wpx, hpx, x, z, y = 0, emissive = false) {
  const t = tex(name);
  const m = new THREE.MeshStandardMaterial({
    map: t, alphaTest: 0.5, side: THREE.DoubleSide, roughness: 1, metalness: 0,
    emissive: 0xffffff, emissiveMap: t, emissiveIntensity: emissive ? 2.2 : 0.28,
  });
  const mesh = new THREE.Mesh(new THREE.PlaneGeometry(wpx / PX, hpx / PX), m);
  mesh.position.set(x, y + hpx / PX / 2, z);
  mesh.castShadow = true; mesh.receiveShadow = false;
  mesh.customDepthMaterial = new THREE.MeshDepthMaterial({ depthPacking: THREE.RGBADepthPacking, map: t, alphaTest: 0.5 });
  mesh.customDistanceMaterial = new THREE.MeshDistanceMaterial({ map: t, alphaTest: 0.5 });
  scene.add(mesh);
  return mesh;
}
sprite('prop_tree_large', 64, 88, -6.2, -3.2, 1);
sprite('prop_tree_large', 64, 88, 7.6, -2.6, 0);
sprite('prop_tree', 32, 44, -8.2, 0.6, 0);
sprite('prop_bush', 24, 16, -3.4, 2.4);
sprite('prop_bush', 24, 16, 6.2, 2.0);
sprite('prop_bush', 24, 16, 1.6, -2.4);
sprite('hero', 24, 32, 0.4, 1.4);
sprite('prop_bush', 24, 16, -5.5, 5.2);
sprite('prop_bush', 24, 16, 4.2, 5.6);
sprite('prop_tree', 32, 44, 9.5, 4.2);
sprite('prop_tree_large', 64, 88, -10.5, -6.5, 2);
sprite('prop_tree_large', 64, 88, 3.5, -8.5, 2);
[-8, -5, -1.5, 1, 6.5, 9.5].forEach((x, i) => sprite(i % 2 ? 'prop_tree' : 'prop_tree_large', i % 2 ? 32 : 64, i % 2 ? 44 : 88, x, -10.5 - (i % 3) * 0.6, 2));
sprite('prop_bush', 24, 16, -2.5, -6.2, 1);
const torchA = sprite('prop_torch', 8, 18, -1.6, 1.0);
const torchB = sprite('prop_torch', 8, 18, 2.4, -1.6);


// ---------------------------------------------------------------- 조명
scene.add(new THREE.HemisphereLight(0x9a8ad8, 0x4a3a40, 0.9));
const sun = new THREE.DirectionalLight(0xffc89a, 2.1);
sun.position.set(-14, 9, 6);
sun.castShadow = true;
sun.shadow.mapSize.set(2048, 2048);
Object.assign(sun.shadow.camera, { left: -14, right: 14, top: 10, bottom: -10, near: 1, far: 50 });
sun.shadow.bias = -0.0008;
sun.shadow.normalBias = 0.02;
scene.add(sun);
function torchLight(x, z) {
  const l = new THREE.PointLight(0xff9a4a, 9, 7, 1.6);
  l.position.set(x, 18 / PX - 0.15, z + 0.25);
  scene.add(l);
}
torchLight(-1.6, 1.0);
torchLight(2.4, -1.6);

// 반딧불과 불티
const pts = [];
let s = 7; const rnd = () => (s = (s * 9301 + 49297) % 233280) / 233280;
for (let i = 0; i < 70; i++) pts.push((rnd() - 0.5) * 18, 0.3 + rnd() * 2.4, (rnd() - 0.5) * 8);
const pg = new THREE.BufferGeometry(); pg.setAttribute('position', new THREE.Float32BufferAttribute(pts, 3));
const dot = (() => { const c = document.createElement('canvas'); c.width = c.height = 4; const g = c.getContext('2d'); g.fillStyle = '#fff'; g.fillRect(1, 1, 2, 2); const t = new THREE.CanvasTexture(c); t.magFilter = THREE.NearestFilter; return t; })();
scene.add(new THREE.Points(pg, new THREE.PointsMaterial({ color: 0xfff0a0, size: 0.18, map: dot, transparent: true, alphaTest: 0.3, fog: false })));

// ---------------------------------------------------------------- 후처리
const composer = new EffectComposer(renderer);
composer.setSize(W, H);
composer.addPass(new RenderPass(scene, camera));
// 틸트 시프트: 초점 띠(r) 밖으로 갈수록 흐려진다. 스프라이트의 투명 영역 문제를 피하려고 깊이 대신 화면 높이를 쓴다.
for (let i = 0; i < 2; i++) {
  const hb = new ShaderPass(HorizontalTiltShiftShader); hb.uniforms.h.value = 2.2 / W; hb.uniforms.r.value = 0.44;
  const vb = new ShaderPass(VerticalTiltShiftShader); vb.uniforms.v.value = 2.2 / H; vb.uniforms.r.value = 0.44;
  composer.addPass(hb); composer.addPass(vb);
}
composer.addPass(new UnrealBloomPass(new THREE.Vector2(W, H), 0.55, 0.6, 0.82));
const grade = new ShaderPass({
  uniforms: { tDiffuse: { value: null } },
  vertexShader: 'varying vec2 vUv; void main(){ vUv = uv; gl_Position = projectionMatrix * modelViewMatrix * vec4(position,1.0); }',
  fragmentShader: `uniform sampler2D tDiffuse; varying vec2 vUv;
    void main(){
      vec4 c = texture2D(tDiffuse, vUv);
      float l = dot(c.rgb, vec3(0.299,0.587,0.114));
      c.rgb = mix(c.rgb, c.rgb * vec3(0.92,0.9,1.12), 1.0 - smoothstep(0.0, 0.5, l)); // 그림자는 푸르게
      c.rgb = mix(c.rgb, c.rgb * vec3(1.06,1.0,0.9), smoothstep(0.5, 1.0, l));        // 밝은 곳은 따뜻하게
      vec2 d = vUv - 0.5; c.rgb *= 1.0 - dot(d, d) * 1.1;                              // 비네팅
      gl_FragColor = c;
    }`,
});
composer.addPass(grade);
composer.addPass(new OutputPass());

THREE.DefaultLoadingManager.onLoad = () => {
  composer.render();
  window.__done = true;
};
