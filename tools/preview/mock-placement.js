// 배치 점수 퍼즐 목업: 산을 등지고 바다를 바라보는 로우폴리 섬, 도형으로 만든 한옥 마을.
import * as THREE from 'three';
import { EffectComposer } from 'three/addons/postprocessing/EffectComposer.js';
import { RenderPass } from 'three/addons/postprocessing/RenderPass.js';
import { UnrealBloomPass } from 'three/addons/postprocessing/UnrealBloomPass.js';
import { ShaderPass } from 'three/addons/postprocessing/ShaderPass.js';
import { OutputPass } from 'three/addons/postprocessing/OutputPass.js';
import { HorizontalTiltShiftShader } from 'three/addons/shaders/HorizontalTiltShiftShader.js';
import { VerticalTiltShiftShader } from 'three/addons/shaders/VerticalTiltShiftShader.js';

const W = 1280, H = 720;
const renderer = new THREE.WebGLRenderer({ antialias: true, preserveDrawingBuffer: true });
renderer.setSize(W, H); renderer.setPixelRatio(1);
renderer.shadowMap.enabled = true; renderer.shadowMap.type = THREE.PCFSoftShadowMap;
renderer.toneMapping = THREE.ACESFilmicToneMapping; renderer.toneMappingExposure = 1.1;
document.body.prepend(renderer.domElement);

const scene = new THREE.Scene();
scene.background = new THREE.Color(0xf2c9a0);
scene.fog = new THREE.Fog(0xf2c9a0, 34, 70);
const camera = new THREE.PerspectiveCamera(30, W / H, 0.5, 200);
camera.position.set(9, 15, 21);
camera.lookAt(0, 0.5, -0.5);

const flat = (c, o = {}) => new THREE.MeshStandardMaterial({ color: c, flatShading: true, roughness: 0.9, ...o });

// ---------------------------------------------------------------- 지형
function height(x, z) {
  const r = Math.hypot(x * 0.95, z * 1.1);
  let h = 1.4 * (1 - THREE.MathUtils.smoothstep(r, 7.5, 11)) - 0.6;            // 섬 바탕
  h += 6.5 * Math.exp(-((x + 0.5) ** 2 / 14 + (z + 7) ** 2 / 5));               // 뒷산
  h += 3.2 * Math.exp(-((x - 5) ** 2 / 6 + (z + 5.5) ** 2 / 4));
  h += 2.6 * Math.exp(-((x + 6) ** 2 / 5 + (z + 4.5) ** 2 / 4));
  h += 0.25 * Math.sin(x * 1.3) * Math.cos(z * 1.1);
  const village = Math.exp(-((x - 0.5) ** 2 / 18 + (z - 1.8) ** 2 / 8));          // 마을 평지
  h = THREE.MathUtils.lerp(h, 0.8, village * 0.85);
  return h;
}
const geo = new THREE.PlaneGeometry(30, 30, 72, 72);
geo.rotateX(-Math.PI / 2);
const pos = geo.attributes.position, colors = [];
const sand = new THREE.Color(0xe8d29a), grass = new THREE.Color(0x8cbf5a), grass2 = new THREE.Color(0x5e9a4a),
      forest = new THREE.Color(0x3f7a44), rock = new THREE.Color(0x9a9088);
for (let i = 0; i < pos.count; i++) {
  const x = pos.getX(i), z = pos.getZ(i), y = height(x, z);
  pos.setY(i, y);
  let c = y < 0.25 ? sand : y < 1.2 ? grass.clone().lerp(grass2, (x + z + 10) % 1 * 0.3) : y < 3.5 ? forest : rock;
  colors.push(c.r, c.g, c.b);
}
geo.setAttribute('color', new THREE.Float32BufferAttribute(colors, 3));
geo.computeVertexNormals();
const ground = new THREE.Mesh(geo.toNonIndexed(), new THREE.MeshStandardMaterial({ vertexColors: true, flatShading: true, roughness: 1 }));
ground.receiveShadow = true; ground.castShadow = true; scene.add(ground);

const water = new THREE.Mesh(new THREE.CircleGeometry(60, 64), new THREE.MeshStandardMaterial({ color: 0x4aa3c8, roughness: 0.25, metalness: 0.1, transparent: true, opacity: 0.88 }));
water.rotation.x = -Math.PI / 2; water.position.y = 0.05; water.receiveShadow = true; scene.add(water);

// ---------------------------------------------------------------- 도형 건물
const yAt = (x, z) => height(x, z);
function add(mesh, x, z, rot = 0, yOff = 0) {
  mesh.position.set(x, yAt(x, z) + yOff, z); mesh.rotation.y = rot;
  mesh.traverse(o => { if (o.isMesh) { o.castShadow = true; o.receiveShadow = true; } });
  scene.add(mesh); return mesh;
}
function prism(w, h, d, mat) { // 삼각기둥 지붕
  const s = new THREE.Shape(); s.moveTo(-w / 2, 0); s.lineTo(0, h); s.lineTo(w / 2, 0); s.lineTo(-w / 2, 0);
  const g = new THREE.ExtrudeGeometry(s, { depth: d, bevelEnabled: false }); g.translate(0, 0, -d / 2);
  return new THREE.Mesh(g, mat);
}
const wallM = flat(0xf3e6c8), woodM = flat(0x8a5a3a), giwaM = flat(0x3c4664), thatchM = flat(0xd8b46a), redM = flat(0xa8402e);
function giwa() {
  const g = new THREE.Group();
  const b = new THREE.Mesh(new THREE.BoxGeometry(1.5, 0.6, 1.0), wallM); b.position.y = 0.3; g.add(b);
  const r = prism(1.9, 0.6, 1.35, giwaM); r.position.y = 0.6; g.add(r);
  for (const sx of [-1, 1]) { const e = new THREE.Mesh(new THREE.BoxGeometry(0.12, 0.12, 1.4), giwaM); e.position.set(sx * 0.98, 0.68, 0); e.rotation.z = sx * 0.5; g.add(e); }
  return g;
}
function choga() {
  const g = new THREE.Group();
  const b = new THREE.Mesh(new THREE.BoxGeometry(1.1, 0.5, 0.8), wallM); b.position.y = 0.25; g.add(b);
  const r = new THREE.Mesh(new THREE.SphereGeometry(0.75, 8, 5, 0, Math.PI * 2, 0, Math.PI / 2), thatchM); r.scale.set(1, 0.6, 0.8); r.position.y = 0.48; g.add(r);
  return g;
}
function well() {
  const g = new THREE.Group();
  const ringM = new THREE.Mesh(new THREE.CylinderGeometry(0.32, 0.36, 0.3, 10), flat(0xb8b4bc)); ringM.position.y = 0.15; g.add(ringM);
  const w = new THREE.Mesh(new THREE.CylinderGeometry(0.22, 0.22, 0.05, 10), flat(0x3a6ea5)); w.position.y = 0.29; g.add(w);
  return g;
}
function jeongja() {
  const g = new THREE.Group();
  for (const [x, z] of [[-.4, -.4], [.4, -.4], [-.4, .4], [.4, .4]]) { const p = new THREE.Mesh(new THREE.CylinderGeometry(0.05, 0.05, 0.7, 6), redM); p.position.set(x, 0.35, z); g.add(p); }
  const f = new THREE.Mesh(new THREE.BoxGeometry(1, 0.08, 1), woodM); f.position.y = 0.12; g.add(f);
  const r = new THREE.Mesh(new THREE.ConeGeometry(0.72, 0.35, 4), giwaM); r.position.y = 0.88; r.rotation.y = Math.PI / 4; g.add(r);
  return g;
}
function jangdok() {
  const g = new THREE.Group();
  [[0, 0, .2], [.3, .1, .16], [-.25, .15, .17], [.05, -.28, .14], [.3, -.2, .12]].forEach(([x, z, r]) => {
    const m = new THREE.Mesh(new THREE.SphereGeometry(r, 8, 6), flat(0x6e3e30)); m.scale.y = 1.2; m.position.set(x, r, z); g.add(m);
  });
  const base = new THREE.Mesh(new THREE.BoxGeometry(1, 0.08, 0.8), flat(0xb0a898)); base.position.y = 0.04; g.add(base);
  return g;
}
function pine(s = 1) {
  const g = new THREE.Group();
  const t = new THREE.Mesh(new THREE.CylinderGeometry(0.06 * s, 0.09 * s, 0.6 * s, 5), woodM); t.position.y = 0.3 * s; g.add(t);
  [[0.55, 0.7], [0.42, 1.0], [0.28, 1.25]].forEach(([r, y]) => { const c = new THREE.Mesh(new THREE.ConeGeometry(r * s, 0.5 * s, 6), flat(0x2f6a40)); c.position.y = y * s; g.add(c); });
  return g;
}
function stoneTower() {
  const g = new THREE.Group();
  [[0.35, 0.18], [0.28, 0.45], [0.22, 0.68], [0.15, 0.86]].forEach(([r, y]) => { const m = new THREE.Mesh(new THREE.DodecahedronGeometry(r, 0), flat(0xa8a4a0)); m.scale.y = 0.6; m.position.y = y; g.add(m); });
  return g;
}
function forge() {
  const g = new THREE.Group();
  const b = new THREE.Mesh(new THREE.BoxGeometry(1, 0.55, 0.8), flat(0x7a6a5e)); b.position.y = 0.27; g.add(b);
  const r = prism(1.2, 0.35, 0.95, flat(0x4a3a34)); r.position.y = 0.55; g.add(r);
  const c = new THREE.Mesh(new THREE.CylinderGeometry(0.09, 0.11, 0.6, 6), flat(0x5a4a44)); c.position.set(0.3, 0.9, 0); g.add(c);
  return g;
}

// 마을 배치
add(giwa(), -1.4, 0.4, 0.05);
add(giwa(), 0.6, -0.2, -0.05);
add(choga(), -2.9, 1.9, 0.3); add(choga(), -1.6, 2.4, -0.2); add(choga(), 3.0, 0.9, 0.4); add(choga(), 3.6, 2.3, -0.1);
add(well(), -2.4, 3.2); add(jangdok(), -0.2, 1.5, 0.2);
add(jeongja(), 4.6, 4.3, 0.3);
add(forge(), 5.2, -0.3, -0.3);
add(stoneTower(), -4.6, 3.4);
[[-5.5, -1.5, 1.2], [-6.5, 0.5, 1], [-4.3, -2.8, 1.3], [6.8, -2, 1.1], [7.5, 0.5, 0.9], [2.5, -3.2, 1.3], [-1.5, -3.6, 1.4], [0.8, -4.4, 1.2], [4.2, -3.9, 1.2],
 [-3, -4.6, 1.5], [-7, -2.5, 1.2], [5.8, -4.5, 1.3]].forEach(([x, z, s]) => add(pine(s), x, z));

// 배치 중인 건물 (반투명 미리보기)
const ghostPos = [1.6, 3.4];
const ghost = giwa();
ghost.traverse(o => { if (o.isMesh) { o.material = o.material.clone(); o.material.transparent = true; o.material.opacity = 0.6; o.material.emissive = new THREE.Color(0x3a8a3a); o.material.emissiveIntensity = 0.5; } });
add(ghost, ...ghostPos, -0.05, 0.25);
const ring = new THREE.Mesh(new THREE.RingGeometry(1.75, 1.85, 48), new THREE.MeshBasicMaterial({ color: 0xffffff, transparent: true, opacity: 0.8 }));
ring.rotation.x = -Math.PI / 2; ring.position.set(ghostPos[0], yAt(...ghostPos) + 0.32, ghostPos[1]); scene.add(ring);
const disc = new THREE.Mesh(new THREE.CircleGeometry(1.75, 48), new THREE.MeshBasicMaterial({ color: 0x6ad06a, transparent: true, opacity: 0.18 }));
disc.rotation.x = -Math.PI / 2; disc.position.copy(ring.position); disc.position.y -= 0.01; scene.add(disc);
// 영향선: 미리보기 건물에서 점수 주는 이웃으로
function link(a, b, color) {
  const pa = new THREE.Vector3(a[0], yAt(...a) + 0.5, a[1]), pb = new THREE.Vector3(b[0], yAt(...b) + 0.5, b[1]);
  const mid = pa.clone().add(pb).multiplyScalar(0.5); mid.y += 0.8;
  const curve = new THREE.QuadraticBezierCurve3(pa, mid, pb);
  scene.add(new THREE.Mesh(new THREE.TubeGeometry(curve, 20, 0.035, 6), new THREE.MeshBasicMaterial({ color })));
}
link(ghostPos, [-0.2, 1.5], 0x2f8a3a); link(ghostPos, [3.0, 0.9], 0x2f8a3a);

// ---------------------------------------------------------------- 조명
scene.add(new THREE.HemisphereLight(0xcfe0ff, 0x8a6a4a, 0.9));
const sun = new THREE.DirectionalLight(0xffd2a0, 2.6);
sun.position.set(14, 12, 6); sun.castShadow = true; sun.shadow.mapSize.set(2048, 2048);
Object.assign(sun.shadow.camera, { left: -14, right: 14, top: 14, bottom: -14, near: 1, far: 60 });
sun.shadow.bias = -0.0006; sun.shadow.normalBias = 0.03; scene.add(sun);

// ---------------------------------------------------------------- 후처리
const composer = new EffectComposer(renderer); composer.setSize(W, H);
composer.addPass(new RenderPass(scene, camera));
const hb = new ShaderPass(HorizontalTiltShiftShader); hb.uniforms.h.value = 1.4 / W; hb.uniforms.r.value = 0.42;
const vb = new ShaderPass(VerticalTiltShiftShader); vb.uniforms.v.value = 1.4 / H; vb.uniforms.r.value = 0.42;
composer.addPass(hb); composer.addPass(vb);
composer.addPass(new UnrealBloomPass(new THREE.Vector2(W, H), 0.25, 0.5, 0.9));
composer.addPass(new OutputPass());
composer.render();

// ---------------------------------------------------------------- 점수 팝업 (HTML)
function pop(text, cls, x, z, dy = 1.4) {
  const v = new THREE.Vector3(x, yAt(x, z) + dy, z).project(camera);
  const el = document.createElement('div'); el.className = 'pop ' + cls; el.textContent = text;
  el.style.left = ((v.x + 1) / 2 * W) + 'px'; el.style.top = ((1 - v.y) / 2 * H) + 'px';
  document.getElementById('ui').appendChild(el);
}
pop('+7', 'main', ghostPos[0], ghostPos[1], 2.2);
pop('+4 배산임수', 'good', ghostPos[0], ghostPos[1] + 1.2, 0.2);
pop('+2', 'good', -0.2, 1.5, 1.0); pop('+1', 'good', 3.0, 0.9, 1.3);
window.__done = true;
