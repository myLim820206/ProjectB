# 23. 도전 과제 (B8)

최종 수정: 2026-11-16
관련: D-321 ④ · D-329 (목표 해금 10종), D-316 (숨은 채널), D-331

게임은 진행 파일(`Progress.txt` 의 `Ach=`)에 따로 남기고, 이룰 때 `PDSteam::UnlockAchievement` 로 스팀에 알린다. 지금 `PDSteam.cpp` 는 빈 구현이라 App ID 가 생기면 그 파일만 채운다. 앞 10개는 칩 대신 칵테일을 여는 목표 해금과 같은 조건이다.

| # | 스팀 이름 | 이름 | 조건 | 목표 해금 칵테일 |
|---|---|---|---|---|
| 1 | ACH_CLEAR_30 | 첫 손님 배웅 | 30 모드 클리어 | 풀하우스 펀치 |
| 2 | ACH_CLEAR_60 | 두 번째 잔 | 60 모드 클리어 | 로열 가드 |
| 3 | ACH_CLEAR_100 | 마감까지 | 100 모드 클리어 | 스트레이트 플러시 팬 |
| 4 | ACH_FIRST_JOKER | 첫 조커 | 조커 처음 만들기 | 조커 팬 |
| 5 | ACH_FIRST_UPPER | 왕관의 무게 | 상위 조커 처음 만들기 | 빅 조커 |
| 6 | ACH_ALL_JOKERS | 조커 수집가 | 기본 조커 10종 모두 만들기 | 포지 |
| 7 | ACH_LEVEL_30 | 늦은 밤의 고수 | 한 판에 레벨 30 | 헤럴드 |
| 8 | ACH_KILLS_1000 | 천 명의 병정 | 한 판에 1,000마리 | 바운티 |
| 9 | ACH_TOTAL_KILLS_10K | 만 명의 병정 | 모든 판 합쳐 10,000마리 | 보스 헌터 |
| 10 | ACH_RUNS_10 | 단골손님 | 10판 끝내기 | 덱스터리티 |
| 11 | ACH_ENDLESS_150 | 끝나지 않는 밤 | 무한 모드 150웨이브 | |
| 12 | ACH_ENDLESS_200 | 새벽이 올 때까지 | 무한 모드 200웨이브 | |
| 13 | ACH_ROYAL_FLUSH | 로열 플러시 | 로열 스트레이트 플러시 만들기 | |
| 14 | ACH_FIVE_CARD | 파이브 카드 | 파이브 카드 만들기 | |
| 15 | ACH_ALL_HANDS | 족보 완성 | 족보 10종 모두 만들어 보기 | |
| 16 | ACH_FUSE_MAX | 최고 합성 | 합성 Lv4 | |
| 17 | ACH_UPPERS_10 | 상위 조커 열 장 | 상위 조커 10종 발견 | |
| 18 | ACH_ALL_UPPERS | 상위 조커 도감 완성 | 상위 조커 30종 발견 | |
| 19 | ACH_CHARMS_30 | 칵테일 서른 잔 | 칵테일 30종 열기 | |
| 20 | ACH_ALL_CHARMS | 모든 칵테일 | 칵테일 60종 열기 | |
| 21 | ACH_UPGRADE_MAX | 영구 강화 끝 | 영구 강화 8종 모두 20레벨 | |
| 22 | ACH_SECRET_CHANNEL | 숨은 채널 | 채널 TV 다이얼을 빠르게 7번 | |
| 23 | ACH_RUNS_50 | 바의 터줏대감 | 50판 끝내기 | |
| 24 | ACH_TOTAL_KILLS_100K | 십만 명의 병정 | 모든 판 합쳐 100,000마리 | |

보스 확장(docs/22, 확인 뒤)이 들어가면 보스 22종 · 최종 보스 셋 · 변이 셋 붙은 보스 과제를 더한다.

## 클라우드 저장

코드 없이 Steamworks 의 **Steam Auto-Cloud** 로 한다. 올릴 파일: `Saved/SaveGames/` 아래 진행(`Progress.txt`) · 중단 저장 · 랭킹 파일 (윈도우 `%LOCALAPPDATA%/ProjectB/Saved/SaveGames`, 맥 `~/Library/Application Support/Epic/ProjectB/Saved/SaveGames` 쪽 — 패키지 뒤 실제 경로를 확인해서 Auto-Cloud 루트에 등록). 설정은 위의 `GameUserSettings.ini`(그래픽 · 음악 · 채널 · 곡)는 올리지 않는다(컴퓨터마다 다름).
