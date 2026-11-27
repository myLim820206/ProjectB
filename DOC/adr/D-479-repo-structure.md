# D-479. 저장소를 DOC/ 구조로 정리

- 날짜: 2026-11-27
- 상태: 확정
- 이슈: #1

## 결정
- 문서는 `DOC/` 아래 역할별 폴더 `design/`(기획) · `dev/`(프로그래밍) · `art/`(디자인)로 나눈다. 공용은 `glossary.md`, `adr/`, `archive/`.
- 루트 `AGENTS.md` 는 짧은 목차와 꼭 지킬 규칙만, `CLAUDE.md` 는 `@AGENTS.md` 한 줄. 자세한 규칙은 `DOC/dev/README.md`, `DOC/art/README.md` 로 옮겼다.
- 의사결정 기록(`docs/05-decision-log.md`, D-001 ~ D-478)은 `DOC/adr/D-001-478-legacy-log.md` 로 옮겨 읽기 전용으로 두고, D-479 부터 한 건당 한 장으로 쓴다.
- 지난 기획(06 · 07 · 08 · 09 · 10 · 11 · 13, 퇴마청), 달빛주막, 웹 프로토타입은 `DOC/archive/` 로 옮겼다.
- 문서용 그림 · 소리 · 악보(`docs/images`, `docs/audio`, `docs/music`)는 `DOC/assets/` 로 옮겼다.
- 언리얼이 쓰는 `ProjectB.uproject` · `Config/` · `Content/` · `Source/` 와 `Tests/` · `tools/` 는 그대로 둔다.

## 이유
프로젝트 운영 규칙의 목표 구조. 역할 스레드가 필요한 문서만 찾아 읽게 하고, 옛 결정 478건을 한 장씩 쪼개는 비용은 들이지 않는다.

## 대안
- 옛 결정 478건을 한 장씩 나누기: 얻는 것보다 비용이 크다.
- `tools/` · `Tests/` 도 옮기기: 스크립트 경로와 빌드 습관이 깨져서 하지 않음.
