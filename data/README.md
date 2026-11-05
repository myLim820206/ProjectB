# 기획 데이터

언리얼 DataTable로 가져올 수 있는 CSV 형식입니다. 첫 열 `Name`이 행 키입니다.
수치는 상세 기획 초안이며 `tools/balance/sim.py`로 검증하고 조정합니다.

| 파일 | 내용 | 명세 |
|---|---|---|
| ingredients.csv | 재료 | docs/design/03-cooking.md |
| recipes.csv | 레시피 | docs/design/03-cooking.md |
| drinks_aging.csv | 술 숙성 곡선 | docs/design/03-cooking.md |
| customers.csv | 낮 손님 유형 | docs/design/04-customers.md |
| yokai.csv | 요괴 손님 | docs/design/05-night.md |
| facilities.csv | 시설과 단계 | docs/design/06-progression-economy.md |
| reputation.csv | 평판 단계 | docs/design/06-progression-economy.md |
| debt.csv | 빚 상환 일정 | docs/design/06-progression-economy.md |

화폐 단위: 푼. 100푼 = 10전 = 1냥. 시설 비용만 냥 단위로 적습니다.
