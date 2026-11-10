// 포커 디펜스 규칙 코어
// 언리얼 의존성이 없는 순수 C++17. 언리얼 모듈 안에서 그대로 컴파일되고,
// Tests/core_test.cpp 가 이 파일만 g++ 로 컴파일해 단위 시험과 봇 시험을 돌린다.
// 웹 프로토타입(prototype/poker-defense/index.html, v1.2)의 규칙을 옮겼다.
// 좌표는 모두 칸 단위 (칸 중심 = x + 0.5). 화면 픽셀 변환은 위젯이 한다.
#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <random>
#include <vector>

#include "PDSynth.h"

namespace PD
{
constexpr int Cols = 16;
constexpr int Rows = 10;
constexpr int HandMaxBase = 7;
constexpr int CharmMax = 5;
constexpr int NumWaves = 15; // 1회차 기준 (보스 3종이 한 바퀴). 진행은 무한, 목숨이 0이 되면 끝
constexpr int StartLives = 20;
constexpr int StartHand = 5;
constexpr int DrawPerWave = 3;
constexpr float ChainStep = 0.25f; // 연쇄: 맞닿은 족보 하나마다 공격력 +25% (연결고리 부적이면 +45%)

enum class EPDSuit : uint8_t { Spade, Heart, Diamond, Club, Wild }; // Wild = 와일드 카드 (아무 숫자·무늬)
struct FPDCard
{
	int Rank = 2; // 2~14 (J Q K A = 11 12 13 14)
	EPDSuit Suit = EPDSuit::Spade;
	bool IsWild() const { return Suit == EPDSuit::Wild; }
	int Power() const { return IsWild() ? 10 : Rank; } // 공격력 계산용 숫자
	bool operator==(const FPDCard& O) const { return Rank == O.Rank && Suit == O.Suit; }
};

// 무늬 = 공격 방식
struct FPDSuitDef
{
	float Dmg, Rate, Range, Splash, Slow;
};
const FPDSuitDef& SuitDef(EPDSuit S);

// 부적: 웨이브 보상, 최대 5칸, 판 끝까지 규칙을 바꾼다. 비트 묶음(uint32)으로 넘긴다
enum class EPDCharm : uint8_t
{
	HeartLover, SpadeLover, DiaLover, ClubLover, // 그 무늬 타워 공격력 +40%
	ChainLink,  // 연쇄 +25% → +45%
	Twins,      // 원 페어·투 페어 ×1.5 → ×2
	LineLover,  // 스트레이트 연사 한 장마다 +0.25 더
	WideFlush,  // 플러시 범위 +0.75
	FaceCards,  // J·Q·K 공격력 +60%
	SmallNums,  // 2~6 연사 +60%
	Lucky7,     // 7 공격력 ×2.5
	BigHand,    // 손패 최대 +2
	Supply,     // 웨이브마다 카드 +1
	Recycle,    // 버리면 1장 뽑기
	Scope,      // 사거리 +15%
	Frost,      // 둔화 강화, ♣ 주변도 둔화
	ChainBlast, // 처치 시 주변 적에게 최대 체력 15% 피해
	Pierce,     // 방패 무시
	Count
};
inline bool HasCharm(uint32_t Mask, EPDCharm J) { return (Mask >> (int)J) & 1u; }
// 즉시 보상 (부적 칸을 쓰지 않음)
enum class EPDInstant : uint8_t { Wild, Draw2, Repair, Count };
struct FPDReward
{
	bool bCharm = true;
	int Id = 0; // EPDCharm 또는 EPDInstant
};

// 족보 = 시너지
enum class EPDHand : uint8_t { StraightFlush, Four, FullHouse, Flush, Straight, Triple, TwoPair, Pair, Count };
int HandBaseValue(EPDHand H);
int HandValue(EPDHand H, int N); // 플러시·스트레이트는 장수가 많을수록 큼
// 카드 묶음(순서 무관)이 이루는 족보 전부
std::vector<EPDHand> ClassifyHand(const std::vector<FPDCard>& Cards);

struct FPDEffects
{
	float Dmg = 1.f, Rate = 1.f, Range = 1.f, Splash = 0.f;
	int Hand = -1;   // 들어간 족보 번호 (Synergies 안), 없으면 -1
	int Chain = 1;
	int CopyKey = -1; // 웃는 가면 조커: 따라 할 옆 타워 칸
};

struct FPDTower
{
	FPDCard Card;
	int X = 0, Y = 0;
	float Cd = 0.f;
	float Age = 0.f; // 설치 후 시간 (연출용)
	FPDEffects Eff;
	// 합성: 족보를 합치면 레벨이 오르고, 합친 타워들의 위력과 족보 효과(특성)를 물려받는다
	int Level = 1;            // 1~4. 같은 레벨끼리만 족보가 된다
	float Mult = 1.f;         // 위력 배율 (합친 타워들 배율 합 × FuseBonus)
	float TDmg = 1.f, TRate = 1.f, TRange = 1.f, TSplash = 0.f; // 물려받은 특성
	// 조커 포탑 (-1 = 일반 타워). 최종형: 더 합성되지 않고, 맞닿은 족보에서는 와일드(레벨 무관)
	int Joker = -1;
	EPDSuit JSuit = EPDSuit::Spade; // 할리퀸: 강화할 무늬 (재료에 가장 많던 무늬)
	float Timer = 0.f;              // 방울: 종 주기
	int Cycle = 0;                  // 광대: 공격 무늬 순환
	bool IsJoker() const { return Joker >= 0; }
};
// 조커 10종 (그림 번호 1~10 순서: docs/15-art-style.md 9절)
enum class EPDJoker : uint8_t { Clown, Hatter, Mask, Comedy, Harlequin, Baton, Jack, Cheshire, Bell, Star, Count };
constexpr float JokerBonus = 2.f; // 조커 합성: 재료 위력 합 × 2
// 조커 재료 묶음: 연쇄로 이어진 족보와 합성 타워(레벨 2+, 족보에 안 든 것)가 합쳐서 3개 이상
struct FPDJokerGroup
{
	std::vector<int> SynergyIdx; // 족보 번호
	std::vector<int> FusedCells; // 따로 선 합성 타워 칸
	std::vector<int> Cells;      // 쓰이는 칸 전부
	EPDJoker Result = EPDJoker::Clown;
};
// 레시피: 족보 종류 목록 + 합성 타워 수로 어떤 조커가 나오는지 (맞는 게 없으면 광대)
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands, int FusedCount);
constexpr int MaxLevel = 4;
constexpr float FuseBonus = 1.5f;      // 합성 덤: 합친 위력 × 1.5
constexpr float LevelRange = 0.1f;     // 레벨마다 사거리 +10%
inline int CellKey(int X, int Y) { return Y * Cols + X; }
using FPDTowerMap = std::map<int, FPDTower>;

struct FPDSynergy
{
	EPDHand Kind = EPDHand::Pair;
	int N = 2;
	int Value = 0;
	std::vector<int> Cells;   // 칸 키
	int Chain = 1;            // 이 족보가 든 연쇄의 족보 수
	int ChainId = -1;
	std::vector<int> Links;   // 맞닿은 다른 족보 번호
};
// 상하좌우로 붙은 타워 덩어리 안의 2~5장 연결 묶음을 모두 따져 큰 족보부터 겹치지 않게 고르고, 연쇄를 묶는다
std::vector<FPDSynergy> ComputeSynergies(const FPDTowerMap& Towers);
// 족보 효과를 타워에 반영하고 족보 목록을 돌려준다
std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers, uint32_t Charms = 0);

struct FPDTowerStats
{
	float Dmg, Rate, Range, Splash, Slow;
};
FPDTowerStats TowerStats(const FPDTower& T, uint32_t Charms = 0);

// ---------------------------------------------------------------- 길
struct FPDPoint
{
	float X = 0.f, Y = 0.f;
};
struct FPDPath
{
	std::vector<std::array<int, 2>> Corners; // 격자 좌표 (화면 밖 -1, Cols 포함)
	std::vector<FPDPoint> Points;            // 칸 중심
	std::vector<float> SegStart;             // 각 구간 시작 거리
	float Length = 0.f;
	std::vector<uint8_t> OnPath;             // Cols*Rows, 길 칸이면 1
	void Build(const std::vector<std::array<int, 2>>& InCorners);
	FPDPoint At(float D) const;
	bool IsPath(int X, int Y) const { return OnPath[CellKey(X, Y)] != 0; }
	int CellCount() const;
};
constexpr int PathMin = 34, PathMax = 46;
std::vector<std::array<int, 2>> GeneratePath(std::mt19937& Rng);

// ---------------------------------------------------------------- 적
// 보통, 빠름, 탱크, 방패병(한 방 피해 감소), 분열체(죽으면 둘), 재생자(체력 회복), 분열 조각, 보스 3종
enum class EPDEnemy : uint8_t { Normal, Fast, Tank, Armored, Splitter, Regen, Mini, ShieldBoss, SplitBoss, RegenBoss, Count };
struct FPDEnemyDef
{
	float Hp, Speed, Regen;
	int Split;      // 죽으면 갈라지는 수
	bool bArmor, bBoss;
};
const FPDEnemyDef& EnemyDef(EPDEnemy T);
struct FPDEnemy
{
	EPDEnemy Type = EPDEnemy::Normal;
	float D = 0.f, Hp = 1.f, MaxHp = 1.f, Speed = 1.f, SlowT = 0.f, SlowAmt = 0.f, Armor = 0.f, BaseHp = 1.f;
	int Wave = 1;
	bool bDead = false;
	int Id = 0;
};

// ---------------------------------------------------------------- 화면 연출용 사건 (위젯이 꺼내 파티클을 만든다)
struct FPDEvent
{
	enum class EKind : uint8_t { Shot, Splash, Kill, Leak, Place, Synergy, WaveStart, WaveClear, GameOver, Win, Discard, Reward, Fuse, JokerFuse, Bell, Star } Kind = EKind::Shot;
	float X = 0.f, Y = 0.f, X2 = 0.f, Y2 = 0.f, R = 0.f;
	EPDSuit Suit = EPDSuit::Spade;
	EPDEnemy Enemy = EPDEnemy::Normal;
	EPDHand Hand = EPDHand::Pair;
	int A = 0, B = 0; // Synergy: A=장수, B=연쇄 / WaveStart: A=웨이브, B=보스 여부, Enemy=보스 종류 / Leak: A=잃은 목숨 / WaveClear: A=받은 카드 / Reward: A=조커면 1, B=번호 / Fuse: A=새 레벨, B=합친 장수, X·Y=합쳐진 칸 / JokerFuse: A=조커 종류, B=처음 만듦 여부 / Bell·Star: X·Y=조커 칸, R=범위
};

enum class EPDPhase : uint8_t { Title, Build, Wave, Reward, Over, Win }; // Reward: 웨이브 보상 고르는 중

class FPDGame
{
public:
	explicit FPDGame(uint32_t Seed = 0);
	FPDEnemy SpawnTestEnemy(EPDEnemy T, float BaseHp, int W) { return MakeEnemy(T, BaseHp, W); } // 시험용
	void HitTest(FPDEnemy& E, float Dmg) { Hit(E, Dmg, 0.f); } // 시험용
	void NewGame(bool bStart);
	void Tick(float Dt);                     // 0.05초로 잘라 Speed 배만큼 진행
	bool Place(int HandIdx, int X, int Y);   // 카드 설치 (타워 위면 교체)
	void Discard(int HandIdx);
	void StartWave();
	void TakeReward(int Index); // -1 = 건너뛰기
	bool CanFuse(int SynergyIdx) const;
	bool Fuse(int SynergyIdx);  // 족보를 타워 하나로 합성 (버튼). 성공하면 true
	std::vector<FPDJokerGroup> JokerGroups() const; // 조커 합성 가능한 묶음
	bool JokerFuse(int GroupIdx);                    // 묶음을 조커 포탑으로
	uint32_t Discovered = 0;                         // 도감: 만들어 본 조커 (새 판에도 유지)
	std::vector<FPDReward> RollRewards();
	uint32_t CharmMask() const;
	bool Has(EPDCharm J) const { return HasCharm(CharmMask(), J); }
	int HandMax() const { return HandMaxBase + (Has(EPDCharm::BigHand) ? 2 : 0); }
	bool Buildable(int X, int Y) const;
	// 고른 카드를 이 칸에 놓으면 생길 족보 (놓은 칸이 든 연쇄 전체)
	std::vector<FPDSynergy> PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers = nullptr) const;
	void Update(float Dt);

	EPDPhase Phase = EPDPhase::Title;
	int Wave = 0, Lives = StartLives, Kills = 0, Leaks = 0, Discarded = 0, DeckRefills = 0;
	int BestWave = 0; // 이번 실행에서 가장 멀리 간 웨이브
	int Speed = 1;
	float Time = 0.f;
	std::vector<FPDCard> Deck, Hand;
	std::vector<EPDCharm> Charms;
	std::vector<FPDReward> Rewards;
	FPDTowerMap Towers;
	std::vector<FPDSynergy> Synergies;
	std::vector<FPDEnemy> Enemies;
	FPDPath Path;
	std::vector<FPDEvent> Events;
	std::vector<FPDSoundEvent> Sounds;
	bool bMuted = false;

private:
	FPDEnemy MakeEnemy(EPDEnemy T, float BaseHp, int W);
	std::vector<FPDEnemy> WaveSpec(int W);
	void Draw(int N, int* OutGot = nullptr);
	void EndWave();
	void Hit(FPDEnemy& E, float Dmg, float Slow);
	void Ev(const FPDEvent& E) { Events.push_back(E); }
	void Sfx(EPDSound S, int A = 0, int B = 0) { if (!bMuted) Sounds.push_back({S, A, B, 1.f}); }

	std::mt19937 Rng;
	std::vector<FPDEnemy> Pending;
	std::vector<FPDEnemy> Born; // 분열로 새로 생긴 적 (갱신 끝에 합친다)
	float SpawnT = 0.f;
	int NextId = 1;
	float KillSfxT = 0.f;
};
} // namespace PD
