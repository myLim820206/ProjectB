# ProjectB: 달빛주막 (가제)

조선 길목의 주막을 키우는 한국형 HD-2D 경영 시뮬레이션입니다. 언리얼 엔진 기반 PC 게임 프로젝트입니다. Windows와 macOS를 지원하며 Steam으로 출시합니다.

## 문서

| 문서 | 내용 |
|---|---|
| [docs/00-project-charter.md](docs/00-project-charter.md) | 프로젝트 개요, 확정 사항, 미결정 사항 |
| [docs/01-game-design.md](docs/01-game-design.md) | 게임 기획서: 주막 경영 (개요) |
| [docs/design/](docs/design/README.md) | 상세 기획 명세 (시스템별 규칙과 수식) |
| [docs/scenario/](docs/scenario/README.md) | 시나리오 대본 (샘플) |
| [data/](data/README.md) | 기획 데이터 CSV (언리얼 DataTable 형식) |
| [docs/02-technical-plan.md](docs/02-technical-plan.md) | 기술 계획: 엔진, 플랫폼, 빌드, 협업 규칙 |
| [docs/03-steam-release.md](docs/03-steam-release.md) | Steam 출시 계획과 체크리스트 |
| [docs/04-roadmap.md](docs/04-roadmap.md) | 마일스톤과 일정 |
| [docs/05-decision-log.md](docs/05-decision-log.md) | 의사결정 기록 |
| [docs/06-concept-candidates.md](docs/06-concept-candidates.md) | 콘셉트 검토 기록 |
| [docs/concepts/A-toemacheong.md](docs/concepts/A-toemacheong.md) | 제외된 기획안: 퇴마청 (보관) |
| [docs/07-art-pipeline.md](docs/07-art-pipeline.md) | 아트 파이프라인과 픽셀 아트 생성 방식 |

## 도구

| 경로 | 내용 |
|---|---|
| `tools/pixelart/` | 픽셀 아트 생성 스크립트 (Python, Pillow) |
| `tools/preview/` | HD-2D 3D 미리보기 렌더러 (three.js, 헤드리스 Chromium) |
| `tools/balance/` | 경제 밸런스 시뮬레이터 (`python3 tools/balance/sim.py`) |

![HD-2D 미리보기](docs/images/hd2d-preview.png)

## 상태

기획 단계 (상세 기획 v1.0 완료). 다음은 언리얼 프로젝트 골격 생성과 프로토타입입니다.
