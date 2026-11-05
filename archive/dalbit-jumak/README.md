# 달빛주막 (보관)

조선 길목의 주막을 이어받아 낮에는 길손에게, 밤에는 요괴에게 밥과 술을 내는 한국형 HD-2D 경영 시뮬레이션.
2026-11-05 보관 (D-027).

| 경로 | 내용 |
|---|---|
| [docs/01-game-design.md](docs/01-game-design.md) | 게임 기획서 (상세 기획 v1.0 개요) |
| [docs/design/](docs/design/README.md) | 시스템별 상세 명세 9편 |
| [docs/scenario/](docs/scenario/README.md) | 샘플 대본 3편 |
| [data/](data/README.md) | 기획 데이터 CSV |
| tools/balance/sim.py | 경제 밸런스 시뮬레이터 |

```bash
python3 archive/dalbit-jumak/tools/balance/sim.py --seeds 200
```

## 다시 꺼낼 때 볼 것
- 아트 물량 줄이기 방안: 그림 두루마리를 엔진 연출로, 주인공 비표시(1인칭), 사람 체형 2종 + 부위 조합, 걷기는 좌우 반전으로 2방향, 초상화 대신 감정 말풍선
- 캐릭터 애니메이션 시험(기본 체형 걷기, 앉기, 먹기 + 부위 교체)을 먼저 해서 가능성 확인
