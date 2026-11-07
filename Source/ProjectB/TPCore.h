// 테트리스 포커 규칙 코어
// 언리얼 의존성이 없는 순수 C++17 코드다. 언리얼 모듈 안에서 그대로 컴파일되고,
// Tests/ 의 단위 시험과 봇 시험은 이 파일만 g++ 로 컴파일해 돌린다.
// 웹 프로토타입(prototype/tetris-poker/index.html, v5)의 규칙을 그대로 옮겼다.
#pragma once

#include <array>
#include <cstdint>
#include <random>
#include <set>
#include <vector>

#include "TPSynth.h"

namespace TP
{
constexpr int Cols = 10;
constexpr int Rows = 18;
constexpr int PiecesPerRound = 40;
constexpr int DeckCopies = 2;
constexpr int NumRanks = 7;        // A(1) ~ 7
constexpr float ChainStepTime = 0.3f;
constexpr float LockDelay = 0.5f;
constexpr int MaxLockMoves = 15;

enum class ETPSuit : uint8_t { Spade, Heart, Diamond, Club };
inline bool IsRed(ETPSuit S) { return S == ETPSuit::Heart || S == ETPSuit::Diamond; }

struct FTPCard
{
	int Rank = 1;
	ETPSuit Suit = ETPSuit::Spade;
};

struct FTPCell
{
	int X = 0;
	int Y = 0;
	bool operator==(const FTPCell& O) const { return X == O.X && Y == O.Y; }
};

// 판: 칸마다 카드 번호(0 = 빈칸). 카드 한 장은 칸 두 개를 같은 번호로 차지한다.
using FTPGrid = std::array<std::array<int, Cols>, Rows>;
FTPGrid EmptyGrid();

enum class ETPHandKind : uint8_t { Five, Four, FullHouse, Triple, TwoPair, Count };
struct FTPHandDef
{
	int Chips;
	int Mult;
};
const FTPHandDef& HandDef(ETPHandKind Kind);
inline int ChipOf(int Rank) { return Rank == 1 ? 11 : Rank; }

struct FTPRules
{
	bool bNoRed = false;
	bool bNoFlush = false;
	bool bNoHold = false;
};

struct FTPHand
{
	ETPHandKind Kind = ETPHandKind::Triple;
	std::vector<int> CardIds;
	std::vector<int> Ranks;
	bool bFlush = false;
	int Chips = 0;
	int Mult = 0;
	int Score = 0;
	std::vector<FTPCell> Cells;
};

struct FTPCardCells
{
	int Id = 0;
	std::vector<FTPCell> Cells;
};

struct FTPCluster
{
	int Rank = 0;
	std::vector<int> Ids;
};

// 판 위 카드 목록 (행 우선으로 처음 만난 순서)
std::vector<FTPCardCells> CardsOf(const FTPGrid& Board);
const FTPCardCells* FindCard(const std::vector<FTPCardCells>& Cards, int Id);
// 받침 없는 카드를 한 장씩 떨어뜨린다. 움직였으면 true
bool Settle(FTPGrid& Board);
// 같은 숫자로 맞닿은 카드 덩어리
std::vector<FTPCluster> RankClusters(const FTPGrid& Board, const std::vector<FTPCard>& Pool);
// 터지는 족보 판정
std::vector<FTPHand> Detect(const FTPGrid& Board, const std::vector<FTPCard>& Pool, const FTPRules& Rules);
// 연쇄로 터진 숫자가 차례로 이어진 단계 수 (3 이상일 때만, 아니면 0)
int ChainStraight(const std::vector<std::set<int>>& RankSteps);

struct FTPSimStep
{
	int Chain = 0;
	std::vector<ETPHandKind> Kinds;
	std::vector<bool> Flushes;
	int Score = 0;
	int Straight = 0;
	std::vector<FTPCell> Cells;
};
struct FTPSimResult
{
	int Chain = 0;
	int Total = 0;
	std::vector<FTPSimStep> Steps;
	FTPGrid Board{};
};
FTPSimResult Simulate(const FTPGrid& Board, const std::vector<FTPCard>& Pool, const FTPRules& Rules);

// ---------------------------------------------------------------- 블록
enum class ETPPieceType : uint8_t { I, O, L, J, S, Z, Count };
// 칸 4개. 앞의 두 칸이 첫 번째 카드, 뒤의 두 칸이 두 번째 카드
const std::array<FTPCell, 4>& ShapeCells(ETPPieceType Type, int Rot);

struct FTPPiece
{
	ETPPieceType Type = ETPPieceType::I;
	int Cards[2] = {0, 0};
	int X = 3;
	int Y = 0;
	int Rot = 0;
};
inline int CardIndexOfCell(int CellIndex) { return CellIndex < 2 ? 0 : 1; }

// ---------------------------------------------------------------- 진행
struct FTPRoundDef
{
	int Target;
	enum class ETPBoss : uint8_t { None, NoHold, NoRed, NoFlush } Boss;
};
using ETPBoss = FTPRoundDef::ETPBoss;
constexpr int NumRounds = 8;
const FTPRoundDef& RoundDef(int Round);

enum class ETPPhase : uint8_t { Play, Chain, Clear, Over, Win };
enum class ETPOverReason : uint8_t { None, Ceiling, OutOfPieces };
enum class ETPAction : uint8_t { Left, Right, Rotate, RotateCcw, SoftDrop, HardDrop, Hold, Next, Restart, Pause, Tap, Mute };
enum class ETPHeldKey : uint8_t { Left, Right, Down, Count };

// 화면에 띄울 글자 (문구는 화면 쪽에서 만든다)
struct FTPPopText
{
	enum class EKind : uint8_t { Hand, Straight, Score, NoHoldMsg } Kind = EKind::Hand;
	ETPHandKind Hand = ETPHandKind::Triple;
	bool bFlush = false;
	int A = 0;          // Hand: 칩, Straight: 단계 수, Score: 점수
	int B = 0;          // Hand: 배수, Score: 연쇄 단계
	float X = 0.f;      // 판 칸 좌표
	float Y = 0.f;
	float T = 1.2f;     // 남은 시간
	bool bBig = false;
};

struct FTPBurstFx
{
	struct FItem
	{
		FTPCard Card;
		std::vector<FTPCell> Cells;
	};
	std::vector<FItem> Items;
	float T = 0.5f;
};

struct FTPChainSummary
{
	int Chain = 0;
	int Total = 0;
	std::vector<ETPHandKind> Kinds;
	std::vector<bool> Flushes;
};

struct FTPPreview
{
	int LandY = 0;
	std::vector<FTPCell> CardCells[2];
	FTPSimResult Sim;
};

class FTPGame
{
public:
	explicit FTPGame(uint32_t Seed = 0);

	// 매 프레임 호출 (초 단위, 0.05초로 자름)
	void Tick(float Dt);
	// 키를 눌렀을 때
	void Act(ETPAction Action);
	// 이동·빠르게 키를 누르고 있는 상태 (자동 반복)
	void SetHeld(ETPHeldKey Key, bool bDown);

	void NewRun();
	void NextRound();
	void Move(int Dx);
	void Rotate(int Dir);
	void SoftDrop();
	void HardDrop();
	void HoldPiece();
	void ChainStep();

	bool Collide(const FTPPiece& P, int X, int Y, int Rot) const;
	std::array<FTPCell, 4> CellsOf(const FTPPiece& P, int X, int Y, int Rot) const;
	std::array<FTPCell, 4> CellsOf(const FTPPiece& P) const { return CellsOf(P, P.X, P.Y, P.Rot); }
	FTPPreview Preview() const;
	FTPRules CurrentRules() const;

	// 상태 (화면과 시험에서 읽는다)
	bool bStarted = false;
	bool bPaused = false;
	ETPPhase Phase = ETPPhase::Play;
	ETPOverReason OverReason = ETPOverReason::None;
	int Round = 0;
	int Score = 0;
	int TotalScore = 0;
	int PiecesLeft = PiecesPerRound;
	int Bursts = 0;
	int MaxChain = 0;
	int Bonus = 0;
	float Time = 0.f;
	bool bHasPiece = false;
	FTPPiece Piece;
	bool bHasHold = false;
	FTPPiece Hold;
	bool bCanHold = true;
	std::vector<FTPPiece> Next;
	FTPGrid Board{};
	std::vector<FTPCard> Pool;      // 카드 번호 → 카드 (0번은 비움)
	std::vector<FTPCard> Deck;
	bool bHasLast = false;
	FTPChainSummary Last;
	bool bHasBest = false;
	FTPChainSummary Best;
	std::vector<FTPPopText> Texts;
	std::vector<FTPBurstFx> Fx;
	// 이번 프레임에 낼 소리 (화면 쪽이 꺼내서 재생하고 비운다)
	std::vector<FTPSoundEvent> Sounds;
	bool bMuted = false;

private:
	void StartRound();
	void SpawnPiece();
	FTPPiece MakePiece();
	int DrawCard();
	ETPPieceType FromBag();
	void LockPiece();
	void AfterTurn();
	void EndRound();
	void Update(float Dt);
	bool OnGround() const;
	void TouchLock();
	float Gravity() const;
	void Pop(const FTPPopText& T) { Texts.push_back(T); }
	void Sfx(ETPSound S, int A = 0, int B = 0) { if (!bMuted) Sounds.push_back({S, A, B, 1.f}); }
	int RandInt(int N); // 0 ~ N-1

	std::mt19937 Rng;
	std::vector<ETPPieceType> Bag;
	float DropT = 0.f;
	float LockT = 0.f;
	int LockMoves = 0;
	float ChainT = 0.f;
	int Chain = 0;
	int ChainScore = 0;
	FTPChainSummary ChainNow;
	std::vector<std::set<int>> RankSteps;
	bool Held[(int)ETPHeldKey::Count] = {false, false, false};
	float Rep[(int)ETPHeldKey::Count] = {0.f, 0.f, 0.f};
};
} // namespace TP
