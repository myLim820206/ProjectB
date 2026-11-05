# 상세 기획 명세

[01 게임 기획서](../01-game-design.md)의 시스템별 규칙, 상태 흐름, 수식입니다.
수치의 기준은 [`data/`](../../data/README.md)의 CSV이고, 이 문서의 숫자와 다르면 CSV가 우선합니다.
경제 수치는 [`tools/balance/sim.py`](../../tools/balance/sim.py)로 검증합니다.

| 문서 | 내용 |
|---|---|
| [01-day-cycle.md](01-day-cycle.md) | 시간, 하루 상태 흐름, 달력, 날씨, 저장 |
| [02-service.md](02-service.md) | 영업: 손님 흐름, 조작, 인내심, 만족도, 일꾼 |
| [03-cooking.md](03-cooking.md) | 조리 시설, 레시피, 품질, 신선도, 술과 장 숙성 |
| [04-customers.md](04-customers.md) | 손님 유형, 등장 규칙, 단골, 특별 손님 |
| [05-night.md](05-night.md) | 밤 주막과 요괴 손님 |
| [06-progression-economy.md](06-progression-economy.md) | 자원, 빚, 평판, 시설, 경제 밸런스 |
| [07-ui.md](07-ui.md) | 화면 흐름, HUD, 입력, 접근성 |
| [08-onboarding-endings.md](08-onboarding-endings.md) | 튜토리얼, 결말, 칭호, 지역 시나리오 |
| [09-scenario.md](09-scenario.md) | 시나리오: 인물, 이야기 구조, 장면 목록 |

## 표기 규칙

- 시간: 실시간은 "초", 게임 안 시각은 "시"로 적는다
- 화폐: 푼. 100푼 = 10전 = 1냥. 시설 비용만 냥
- `[조정]`: 프로토타입에서 체감으로 다시 맞출 수치
