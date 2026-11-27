# ProjectB — 바 앨리스: 늦은 밤의 포커

포커 카드로 타워를 세우는 디펜스 게임(포커 디펜스). 언리얼 엔진 5.8 C++, Windows · macOS, Steam 출시.

- 작업 안내와 문서 목차: [AGENTS.md](AGENTS.md)
- 기획: [DOC/design](DOC/design/README.md) · 개발: [DOC/dev](DOC/dev/README.md) · 디자인: [DOC/art](DOC/art/README.md)
- 용어집: [DOC/glossary.md](DOC/glossary.md) · 결정 기록: [DOC/adr](DOC/adr/README.md) · 보관: [DOC/archive](DOC/archive/README.md)

## 구성

| 경로 | 내용 |
|---|---|
| `ProjectB.uproject`, `Config/`, `Content/`, `Source/` | 언리얼 프로젝트 (포커 디펜스 C++) |
| `Tests/` | 규칙 코어 시험 (`./Tests/run.sh`, 언리얼 없이 실행) |
| `tools/` | 언어팩, 네온 미리보기, 아틀라스 굽기, 패키징 도구 |
| `DOC/` | 문서 (`DOC/assets/` 는 문서용 그림 · 소리) |
