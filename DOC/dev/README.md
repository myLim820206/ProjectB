# 개발 문서와 규칙

| 문서 | 내용 |
|---|---|
| [12-unreal-port.md](12-unreal-port.md) | 언리얼 프로젝트: 열어 보는 법, 코드 구조, 이펙트, 검증 상태 |
| [02-technical-plan.md](02-technical-plan.md) | 기술 계획: 엔진, 플랫폼, 빌드, 저장소 규칙 |

## 개발 규칙
- 엔진은 언리얼 5.8 C++. **웹 프로토타입 작업은 하지 않는다** (토큰 낭비, D-062). 새 기능은 언리얼(`Source/ProjectB`)에만. 옛 웹 프로토타입은 `DOC/archive/prototype/` 에 보관만 한다.
- 규칙 코어 `PDCore` 는 언리얼 의존성 없는 C++ 로 유지하고 `./Tests/run.sh` 로 시험한다. 화면은 `PDBoardWidget` (Slate).
- 작업이 끝나면 따로 말이 없는 한 **언리얼을 직접 빌드하고 에디터를 자동 재시작**한다. 빌드에 필요하면(DLL 잠김 등) 에디터를 끄고 빌드한다. 빌드 전에 g++/cl 시험과 가짜 헤더 문법 검사(`Tests/syntax.sh`)를 한다.
- 결정은 `DOC/adr/` 에 한 건당 한 장 ([작성 규칙](../adr/README.md)).

## 언어팩 (D-159)
화면 글자는 언어팩을 거친다. 한국어 원문으로 쓰고, 새 글자를 넣으면
1. `tools/lang/extract.py --tsv tools/lang/strings.tsv`
2. 영어 · 일본어 · 중국어 간체 · 번체 번역을 채운다
3. `tools/lang/gen_lang.py`

printf 형식 글자는 `TrF`, 형식 인자로 넘기는 한국어는 `Tr(...)`.
