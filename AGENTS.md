# ProjectB — 작업 안내 (목차)

게임: **바 앨리스: 늦은 밤의 포커** (포커 디펜스). 언리얼 5.8 C++, Windows · macOS, Steam.
이 파일은 짧은 목차다. 필요한 문서만 찾아 읽는다.

## 꼭 지킬 것
- 한국어 반말로 편하게 대화한다. git 커밋과 푸시는 묻지 않고 바로 한다.
- 일감은 GitHub 이슈 단위. PR 에 이슈를 연결하고, 1인 개발이라 리뷰 없이 main 에 머지한다.
- 웹 프로토타입 작업은 하지 않는다 (D-062). 새 기능은 언리얼(`Source/ProjectB`)에만.
- 화면 글자는 언어팩을 거친다 (D-159), 그림은 화풍 규칙과 스프라이트 규칙을 따른다 (D-146) → 아래 역할별 규칙.
- 결정은 `DOC/adr/` 에 한 건당 한 장으로 남긴다 (규칙은 [DOC/adr/README.md](DOC/adr/README.md)).

## 역할별 문서
| 역할 | 시작점 | 내용 |
|---|---|---|
| 기획 | [DOC/design/README.md](DOC/design/README.md) | 게임 규칙, 모드, 조커 · 묘약 · 보스, 출시 · 로드맵 |
| 개발 | [DOC/dev/README.md](DOC/dev/README.md) | 개발 · 빌드 · 시험 규칙, 언어팩, 언리얼 구조 |
| 디자인 | [DOC/art/README.md](DOC/art/README.md) | 화풍(필수), 스프라이트 · 아틀라스 규칙 |
| 공용 | [DOC/glossary.md](DOC/glossary.md) · [DOC/adr/](DOC/adr/README.md) · [DOC/archive/](DOC/archive/README.md) | 용어집, 결정 기록, 보관 |

## 저장소 구성
| 경로 | 내용 |
|---|---|
| `DOC/` | AI 가 읽는 md 문서 (`design` · `dev` · `art` · `glossary.md` · `adr` · `archive`), 문서용 그림 · 소리는 `DOC/assets/` |
| `Source/ProjectB/` | 게임 C++ (규칙 코어 `PDCore`, 화면 `PDBoardWidget`) |
| `ProjectB.uproject`, `Config/`, `Content/` | 언리얼 프로젝트 (위치를 옮기지 않는다) |
| `Tests/` | 언리얼 없이 도는 규칙 시험 (`./Tests/run.sh`), 가짜 헤더 문법 검사 (`Tests/syntax.sh`) |
| `tools/` | 언어팩 · 네온 미리보기 · 아틀라스 · 패키징 도구 |
