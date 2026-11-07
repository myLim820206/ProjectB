# ProjectB

기본 도형만으로 만드는 언리얼 엔진 PC 게임 프로젝트입니다. Windows와 macOS를 지원하며 Steam으로 출시합니다.

## 문서

| 문서 | 내용 |
|---|---|
| [docs/00-project-charter.md](docs/00-project-charter.md) | 프로젝트 개요, 확정 사항, 미결정 사항 |
| [docs/09-tetris-poker.md](docs/09-tetris-poker.md) | 테트리스 포커 (현재 진행) |
| [docs/10-balatro-reverse-design.md](docs/10-balatro-reverse-design.md) | 발라트로 역기획 (성장 구조 참고) |
| [docs/08-shape-concepts.md](docs/08-shape-concepts.md) | 도형 기반 게임 콘셉트 검토, 테트리스 디펜스 기록 |
| [docs/02-technical-plan.md](docs/02-technical-plan.md) | 기술 계획: 엔진, 플랫폼, 빌드, 협업 규칙 |
| [docs/03-steam-release.md](docs/03-steam-release.md) | Steam 출시 계획과 체크리스트 |
| [docs/04-roadmap.md](docs/04-roadmap.md) | 마일스톤과 일정 |
| [docs/05-decision-log.md](docs/05-decision-log.md) | 의사결정 기록 |
| [docs/06-concept-candidates.md](docs/06-concept-candidates.md) | 이전 콘셉트 검토 기록 |
| [docs/07-art-pipeline.md](docs/07-art-pipeline.md) | 픽셀 아트 파이프라인 (보류) |
| [archive/](archive/README.md) | 보관 기획: 달빛주막, 퇴마청 |

## 도구

| 경로 | 내용 |
|---|---|
| `prototype/tetris-poker/` | 테트리스 포커 웹 프로토타입 (현재) |
| `prototype/tetris-defense/` | 테트리스 디펜스 웹 프로토타입 (보류) |
| `tools/pixelart/` | 픽셀 아트 생성 스크립트 (Python, Pillow) |
| `tools/preview/` | 3D 미리보기 렌더러 (three.js, 헤드리스 Chromium) |
| `archive/dalbit-jumak/tools/balance/` | 달빛주막 경제 밸런스 시뮬레이터 |

## 상태

프로토타입 단계. 테트리스 포커의 핵심 루프를 시험하는 중입니다.
