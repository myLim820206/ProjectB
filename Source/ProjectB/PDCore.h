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
constexpr int HandMax = 7;
constexpr int NumWaves = 15;
constexpr int StartLives = 20;
constexpr int StartHand = 5;
constexpr int DrawPerWave = 3;
constexpr float ChainStep = 0.25f; // 연쇄: 맞닿은 족보 하나마다 공격력 +25%

enum class EPDSuit : uint8_t { Spade, Heart, Diamond, Club };
struct FPDCard
{
	int Rank = 2; // 2~14 (J Q K A = 11 12 13 14)
	EPDSuit Suit = EPDSuit::Spade;
	bool operator==(const FPDCard& O) const { return Rank == O.Rank && Suit == O.Suit; }
};

// 무늬 = 공격 방식
struct FPDSuitDef
{
	float Dmg, Rate, Range, Splash, Slow;
};
const FPDSuitDef& SuitDef(EPDSuit S);

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
};

struct FPDTower
{
	FPDCard Card;
	int X = 0, Y = 0;
	float Cd = 0.f;
	float Age = 0.f; // 설치 후 시간 (연출용)
	FPDEffects Eff;
};
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
std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers);

struct FPDTowerStats
{
	float Dmg, Rate, Range, Splash, Slow;
};
FPDTowerStats TowerStats(const FPDTower& T);

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
enum class EPDEnemy : uint8_t { Normal, Fast, Tank, Boss };
struct FPDEnemy
{
	EPDEnemy Type = EPDEnemy::Normal;
	float D = 0.f, Hp = 1.f, MaxHp = 1.f, Speed = 1.f, SlowT = 0.f, SlowAmt = 0.f;
	bool bDead = false;
	int Id = 0;
};

// ---------------------------------------------------------------- 화면 연출용 사건 (위젯이 꺼내 파티클을 만든다)
struct FPDEvent
{
	enum class EKind : uint8_t { Shot, Splash, Kill, Leak, Place, Synergy, WaveStart, WaveClear, GameOver, Win, Discard } Kind = EKind::Shot;
	float X = 0.f, Y = 0.f, X2 = 0.f, Y2 = 0.f, R = 0.f;
	EPDSuit Suit = EPDSuit::Spade;
	EPDEnemy Enemy = EPDEnemy::Normal;
	EPDHand Hand = EPDHand::Pair;
	int A = 0, B = 0; // Synergy: A=장수, B=연쇄 / WaveStart: A=웨이브, B=보스 여부 / Leak: A=잃은 목숨 / WaveClear: A=받은 카드
};

enum class EPDPhase : uint8_t { Title, Build, Wave, Over, Win };

class FPDGame
{
public:
	explicit FPDGame(uint32_t Seed = 0);
	void NewGame(bool bStart);
	void Tick(float Dt);                     // 0.05초로 잘라 Speed 배만큼 진행
	bool Place(int HandIdx, int X, int Y);   // 카드 설치 (타워 위면 교체)
	void Discard(int HandIdx);
	void StartWave();
	bool Buildable(int X, int Y) const;
	// 고른 카드를 이 칸에 놓으면 생길 족보 (놓은 칸이 든 연쇄 전체)
	std::vector<FPDSynergy> PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers = nullptr) const;
	void Update(float Dt);

	EPDPhase Phase = EPDPhase::Title;
	int Wave = 0, Lives = StartLives, Kills = 0, Leaks = 0, Discarded = 0;
	int Speed = 1;
	float Time = 0.f;
	std::vector<FPDCard> Deck, Hand;
	FPDTowerMap Towers;
	std::vector<FPDSynergy> Synergies;
	std::vector<FPDEnemy> Enemies;
	FPDPath Path;
	std::vector<FPDEvent> Events;
	std::vector<FPDSoundEvent> Sounds;
	bool bMuted = false;

private:
	struct FSpawn
	{
		EPDEnemy Type;
		float Hp, Speed;
	};
	std::vector<FSpawn> WaveSpec(int W) const;
	void Draw(int N, int* OutGot = nullptr);
	void EndWave();
	void Hit(FPDEnemy& E, float Dmg, float Slow);
	void Ev(const FPDEvent& E) { Events.push_back(E); }
	void Sfx(EPDSound S, int A = 0, int B = 0) { if (!bMuted) Sounds.push_back({S, A, B, 1.f}); }

	std::mt19937 Rng;
	std::vector<FSpawn> Pending;
	float SpawnT = 0.f;
	int NextId = 1;
	float KillSfxT = 0.f;
};
} // namespace PD
