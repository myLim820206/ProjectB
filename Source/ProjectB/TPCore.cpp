#include "TPCore.h"

#include <algorithm>
#include <map>

namespace TP
{
// ---------------------------------------------------------------- 표
static const FTPHandDef GHandDefs[(int)ETPHandKind::Count] = {
	{80, 8}, // 파이브 카드
	{50, 5}, // 포카드
	{40, 4}, // 풀하우스
	{20, 2}, // 트리플
	{15, 2}, // 투 페어
};
const FTPHandDef& HandDef(ETPHandKind Kind) { return GHandDefs[(int)Kind]; }

static const FTPRoundDef GRounds[NumRounds] = {
	{400, ETPBoss::None},  {700, ETPBoss::None},  {1000, ETPBoss::NoHold}, {1400, ETPBoss::None},
	{1850, ETPBoss::None}, {2150, ETPBoss::NoRed}, {2650, ETPBoss::None},  {3200, ETPBoss::NoFlush},
};
const FTPRoundDef& RoundDef(int InRound) { return GRounds[std::min(std::max(InRound, 0), NumRounds - 1)]; }

FTPGrid EmptyGrid()
{
	FTPGrid G;
	for (auto& Row : G) Row.fill(0);
	return G;
}

// 블록 모양. 회전은 좌표 회전으로 만들고 칸 순서(카드 구분)를 유지한다
struct FShapeTable
{
	std::array<std::array<std::array<FTPCell, 4>, 4>, (int)ETPPieceType::Count> Shapes;
	FShapeTable()
	{
		const int Base[(int)ETPPieceType::Count][4][2] = {
			{{0, 1}, {1, 1}, {2, 1}, {3, 1}}, // I
			{{1, 0}, {1, 1}, {2, 0}, {2, 1}}, // O
			{{2, 0}, {2, 1}, {0, 1}, {1, 1}}, // L
			{{0, 0}, {0, 1}, {1, 1}, {2, 1}}, // J
			{{1, 0}, {2, 0}, {0, 1}, {1, 1}}, // S
			{{0, 0}, {1, 0}, {1, 1}, {2, 1}}, // Z
		};
		for (int T = 0; T < (int)ETPPieceType::Count; ++T)
		{
			const int N = T == (int)ETPPieceType::I ? 4 : 3;
			for (int K = 0; K < 4; ++K) Shapes[T][0][K] = {Base[T][K][0], Base[T][K][1]};
			for (int R = 1; R < 4; ++R)
				for (int K = 0; K < 4; ++K)
				{
					const FTPCell C = Shapes[T][R - 1][K];
					Shapes[T][R][K] = T == (int)ETPPieceType::O ? FTPCell{2 - C.Y, C.X - 1} : FTPCell{N - 1 - C.Y, C.X};
				}
		}
	}
};
static const FShapeTable GShapeTable;
const std::array<FTPCell, 4>& ShapeCells(ETPPieceType Type, int Rot) { return GShapeTable.Shapes[(int)Type][Rot & 3]; }

static const int GKicks[7][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {-2, 0}, {2, 0}, {0, 1}};
static const int GN4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
static bool InBoard(int X, int Y) { return X >= 0 && Y >= 0 && X < Cols && Y < Rows; }

// ---------------------------------------------------------------- 카드 단위 판
std::vector<FTPCardCells> CardsOf(const FTPGrid& Board)
{
	std::vector<FTPCardCells> Out;
	std::map<int, size_t> Index;
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			const int Id = Board[Y][X];
			if (!Id) continue;
			auto It = Index.find(Id);
			if (It == Index.end())
			{
				Index[Id] = Out.size();
				Out.push_back({Id, {}});
				Out.back().Cells.push_back({X, Y});
			}
			else
				Out[It->second].Cells.push_back({X, Y});
		}
	return Out;
}

const FTPCardCells* FindCard(const std::vector<FTPCardCells>& Cards, int Id)
{
	for (const FTPCardCells& C : Cards)
		if (C.Id == Id) return &C;
	return nullptr;
}

static int MaxY(const std::vector<FTPCell>& Cells)
{
	int M = -1;
	for (const FTPCell& C : Cells) M = std::max(M, C.Y);
	return M;
}

bool Settle(FTPGrid& Board)
{
	bool bMoved = true, bAny = false;
	while (bMoved)
	{
		bMoved = false;
		std::vector<FTPCardCells> Cards = CardsOf(Board);
		std::stable_sort(Cards.begin(), Cards.end(), [](const FTPCardCells& A, const FTPCardCells& B) { return MaxY(A.Cells) > MaxY(B.Cells); });
		for (const FTPCardCells& CC : Cards)
		{
			bool bFree = true;
			for (const FTPCell& C : CC.Cells)
			{
				if (C.Y + 1 >= Rows) { bFree = false; break; }
				const int Below = Board[C.Y + 1][C.X];
				if (Below && Below != CC.Id) { bFree = false; break; }
			}
			if (!bFree) continue;
			for (const FTPCell& C : CC.Cells) Board[C.Y][C.X] = 0;
			for (const FTPCell& C : CC.Cells) Board[C.Y + 1][C.X] = CC.Id;
			bMoved = bAny = true;
		}
	}
	return bAny;
}

static std::vector<int> Neighbors(const FTPGrid& Board, int Id, const std::vector<FTPCell>& Cells)
{
	std::vector<int> Out;
	for (const FTPCell& C : Cells)
		for (const auto& D : GN4)
		{
			const int NX = C.X + D[0], NY = C.Y + D[1];
			if (!InBoard(NX, NY)) continue;
			const int N = Board[NY][NX];
			if (N && N != Id && std::find(Out.begin(), Out.end(), N) == Out.end()) Out.push_back(N);
		}
	return Out;
}

// ---------------------------------------------------------------- 족보
std::vector<FTPCluster> RankClusters(const FTPGrid& Board, const std::vector<FTPCard>& Pool)
{
	const std::vector<FTPCardCells> Cards = CardsOf(Board);
	std::set<int> Seen;
	std::vector<FTPCluster> Out;
	for (const FTPCardCells& CC : Cards)
	{
		if (Seen.count(CC.Id)) continue;
		const int Rank = Pool[CC.Id].Rank;
		FTPCluster Cl{Rank, {}};
		std::vector<int> Stack{CC.Id};
		Seen.insert(CC.Id);
		while (!Stack.empty())
		{
			const int Cur = Stack.back();
			Stack.pop_back();
			Cl.Ids.push_back(Cur);
			for (int N : Neighbors(Board, Cur, FindCard(Cards, Cur)->Cells))
				if (!Seen.count(N) && Pool[N].Rank == Rank)
				{
					Seen.insert(N);
					Stack.push_back(N);
				}
		}
		Out.push_back(std::move(Cl));
	}
	return Out;
}

std::vector<FTPHand> Detect(const FTPGrid& Board, const std::vector<FTPCard>& Pool, const FTPRules& Rules)
{
	const std::vector<FTPCardCells> Cards = CardsOf(Board);
	const std::vector<FTPCluster> Clusters = RankClusters(Board, Pool);
	std::map<int, int> ClusterOf;
	for (int I = 0; I < (int)Clusters.size(); ++I)
		for (int Id : Clusters[I].Ids) ClusterOf[Id] = I;
	auto Touching = [&](int A, int O) {
		for (int Id : Clusters[A].Ids)
			for (int N : Neighbors(Board, Id, FindCard(Cards, Id)->Cells))
				if (ClusterOf[N] == O) return true;
		return false;
	};
	std::vector<FTPHand> Hands;
	std::set<int> Used;
	auto Add = [&](ETPHandKind Kind, std::vector<int> Ids, std::vector<int> Ranks) {
		FTPHand H;
		H.Kind = Kind;
		H.CardIds = std::move(Ids);
		H.Ranks = std::move(Ranks);
		Hands.push_back(std::move(H));
	};
	for (int I = 0; I < (int)Clusters.size(); ++I)
	{
		const FTPCluster& Cl = Clusters[I];
		const int N = (int)Cl.Ids.size();
		if (N < 3) continue;
		if (N >= 5) { Add(ETPHandKind::Five, Cl.Ids, {Cl.Rank}); continue; }
		if (N == 4) { Add(ETPHandKind::Four, Cl.Ids, {Cl.Rank}); continue; }
		int Pair = -1;
		for (int O = 0; O < (int)Clusters.size(); ++O)
			if (O != I && Clusters[O].Ids.size() == 2 && !Used.count(O) && Touching(I, O)) { Pair = O; break; }
		if (Pair >= 0)
		{
			Used.insert(Pair);
			std::vector<int> Ids = Cl.Ids;
			Ids.insert(Ids.end(), Clusters[Pair].Ids.begin(), Clusters[Pair].Ids.end());
			Add(ETPHandKind::FullHouse, Ids, {Cl.Rank});
		}
		else
			Add(ETPHandKind::Triple, Cl.Ids, {Cl.Rank});
	}
	std::vector<int> Pairs;
	for (int I = 0; I < (int)Clusters.size(); ++I)
		if (Clusters[I].Ids.size() == 2 && !Used.count(I)) Pairs.push_back(I);
	for (int A : Pairs)
	{
		if (Used.count(A)) continue;
		for (int P : Pairs)
			if (P != A && !Used.count(P) && Clusters[P].Rank != Clusters[A].Rank && Touching(A, P))
			{
				Used.insert(A);
				Used.insert(P);
				std::vector<int> Ids = Clusters[A].Ids;
				Ids.insert(Ids.end(), Clusters[P].Ids.begin(), Clusters[P].Ids.end());
				Add(ETPHandKind::TwoPair, Ids, {Clusters[A].Rank, Clusters[P].Rank});
				break;
			}
	}
	for (FTPHand& H : Hands)
	{
		const FTPHandDef& D = HandDef(H.Kind);
		H.Mult = D.Mult;
		H.bFlush = !Rules.bNoFlush;
		for (int Id : H.CardIds)
			if (Pool[Id].Suit != Pool[H.CardIds[0]].Suit) H.bFlush = false;
		if (H.bFlush) H.Mult *= 2;
		H.Chips = D.Chips;
		for (int Id : H.CardIds) H.Chips += (Rules.bNoRed && IsRed(Pool[Id].Suit)) ? 0 : ChipOf(Pool[Id].Rank);
		H.Score = H.Chips * H.Mult;
		for (int Id : H.CardIds)
		{
			const FTPCardCells* CC = FindCard(Cards, Id);
			H.Cells.insert(H.Cells.end(), CC->Cells.begin(), CC->Cells.end());
		}
	}
	return Hands;
}

int ChainStraight(const std::vector<std::set<int>>& Steps)
{
	if (Steps.size() < 3) return 0;
	std::map<int, int> Up, Down;
	int BestRun = 0;
	for (size_t I = 0; I < Steps.size(); ++I)
	{
		std::map<int, int> NU, ND;
		for (int R : Steps[I])
		{
			const int U = (I && Up.count(R - 1)) ? Up[R - 1] + 1 : 1;
			const int D = (I && Down.count(R + 1)) ? Down[R + 1] + 1 : 1;
			NU[R] = U;
			ND[R] = D;
			if (I == Steps.size() - 1) BestRun = std::max({BestRun, U, D});
		}
		Up = std::move(NU);
		Down = std::move(ND);
	}
	return BestRun >= 3 ? BestRun : 0;
}

static void RemoveHands(FTPGrid& Board, const std::vector<FTPHand>& Hands)
{
	for (const FTPHand& H : Hands)
		for (const FTPCell& C : H.Cells) Board[C.Y][C.X] = 0;
}

static std::set<int> RanksOf(const std::vector<FTPHand>& Hands)
{
	std::set<int> S;
	for (const FTPHand& H : Hands) S.insert(H.Ranks.begin(), H.Ranks.end());
	return S;
}

FTPSimResult Simulate(const FTPGrid& Start, const std::vector<FTPCard>& Pool, const FTPRules& Rules)
{
	FTPSimResult R;
	R.Board = Start;
	Settle(R.Board);
	std::vector<std::set<int>> Steps;
	while (true)
	{
		const std::vector<FTPHand> Hands = Detect(R.Board, Pool, Rules);
		if (Hands.empty()) break;
		R.Chain++;
		Steps.push_back(RanksOf(Hands));
		const int St = ChainStraight(Steps);
		int Sum = 0;
		for (const FTPHand& H : Hands) Sum += H.Score;
		const int Sc = Sum * R.Chain * (St ? St : 1);
		R.Total += Sc;
		FTPSimStep S;
		S.Chain = R.Chain;
		S.Score = Sc;
		S.Straight = St;
		for (const FTPHand& H : Hands)
		{
			S.Kinds.push_back(H.Kind);
			S.Flushes.push_back(H.bFlush);
			S.Cells.insert(S.Cells.end(), H.Cells.begin(), H.Cells.end());
		}
		R.Steps.push_back(std::move(S));
		RemoveHands(R.Board, Hands);
		Settle(R.Board);
	}
	return R;
}

// ---------------------------------------------------------------- 진행
FTPGame::FTPGame(uint32_t Seed) : Rng(Seed ? Seed : std::random_device{}())
{
	NewRun();
	bStarted = false;
}

int FTPGame::RandInt(int N) { return std::uniform_int_distribution<int>(0, N - 1)(Rng); }

FTPRules FTPGame::CurrentRules() const
{
	const ETPBoss B = RoundDef(Round).Boss;
	FTPRules R;
	R.bNoRed = B == ETPBoss::NoRed;
	R.bNoFlush = B == ETPBoss::NoFlush;
	R.bNoHold = B == ETPBoss::NoHold;
	return R;
}

void FTPGame::NewRun()
{
	Round = 0;
	TotalScore = 0;
	bHasBest = false;
	Time = 0.f;
	Pool.assign(1, FTPCard{});
	StartRound();
}

void FTPGame::StartRound()
{
	Board = EmptyGrid();
	Deck.clear();
	Bag.clear();
	Next.clear();
	bHasHold = false;
	bCanHold = true;
	bHasPiece = false;
	Score = 0;
	PiecesLeft = PiecesPerRound;
	Bursts = 0;
	MaxChain = 0;
	bHasLast = false;
	DropT = LockT = 0.f;
	LockMoves = 0;
	Phase = ETPPhase::Play;
	OverReason = ETPOverReason::None;
	ChainT = 0.f;
	Chain = ChainScore = 0;
	RankSteps.clear();
	Fx.clear();
	Texts.clear();
	bPaused = false;
	for (int I = 0; I < 3; ++I) Next.push_back(MakePiece());
	SpawnPiece();
}

int FTPGame::DrawCard()
{
	if (Deck.empty())
	{
		for (int K = 0; K < DeckCopies; ++K)
			for (int S = 0; S < 4; ++S)
				for (int R = 1; R <= NumRanks; ++R) Deck.push_back({R, (ETPSuit)S});
		for (int I = (int)Deck.size() - 1; I > 0; --I) std::swap(Deck[I], Deck[RandInt(I + 1)]);
	}
	Pool.push_back(Deck.back()); // 판 위에서 카드마다 다른 번호
	Deck.pop_back();
	return (int)Pool.size() - 1;
}

ETPPieceType FTPGame::FromBag()
{
	if (Bag.empty())
	{
		for (int T = 0; T < (int)ETPPieceType::Count; ++T) Bag.push_back((ETPPieceType)T);
		for (int I = (int)Bag.size() - 1; I > 0; --I) std::swap(Bag[I], Bag[RandInt(I + 1)]);
	}
	const ETPPieceType T = Bag.back();
	Bag.pop_back();
	return T;
}

FTPPiece FTPGame::MakePiece()
{
	FTPPiece P;
	P.Type = FromBag();
	P.Cards[0] = DrawCard();
	P.Cards[1] = DrawCard();
	return P;
}

void FTPGame::SpawnPiece()
{
	if (PiecesLeft <= 0) { EndRound(); return; }
	Piece = Next.front();
	Next.erase(Next.begin());
	while (Next.size() < 3) Next.push_back(MakePiece());
	Piece.Rot = 0;
	Piece.X = 3;
	Piece.Y = Piece.Type == ETPPieceType::I ? -1 : 0;
	bHasPiece = true;
	DropT = LockT = 0.f;
	LockMoves = 0;
	if (Collide(Piece, Piece.X, Piece.Y, Piece.Rot))
	{
		Phase = ETPPhase::Over;
		OverReason = ETPOverReason::Ceiling;
	}
}

std::array<FTPCell, 4> FTPGame::CellsOf(const FTPPiece& P, int X, int Y, int Rot) const
{
	std::array<FTPCell, 4> Out = ShapeCells(P.Type, Rot);
	for (FTPCell& C : Out) { C.X += X; C.Y += Y; }
	return Out;
}

bool FTPGame::Collide(const FTPPiece& P, int X, int Y, int Rot) const
{
	for (const FTPCell& C : CellsOf(P, X, Y, Rot))
		if (C.X < 0 || C.X >= Cols || C.Y >= Rows || C.Y < 0 || Board[C.Y][C.X]) return true;
	return false;
}

FTPPreview FTPGame::Preview() const
{
	FTPPreview Pv;
	if (!bHasPiece) return Pv;
	int Y = Piece.Y;
	while (!Collide(Piece, Piece.X, Y + 1, Piece.Rot)) ++Y;
	Pv.LandY = Y;
	FTPGrid B = Board;
	const auto Cells = CellsOf(Piece, Piece.X, Y, Piece.Rot);
	for (int I = 0; I < 4; ++I) B[Cells[I].Y][Cells[I].X] = Piece.Cards[CardIndexOfCell(I)];
	Settle(B);
	const std::vector<FTPCardCells> Cards = CardsOf(B);
	for (int K = 0; K < 2; ++K)
		if (const FTPCardCells* CC = FindCard(Cards, Piece.Cards[K])) Pv.CardCells[K] = CC->Cells;
	Pv.Sim = Simulate(B, Pool, CurrentRules());
	return Pv;
}

float FTPGame::Gravity() const { return std::max(0.22f, 0.9f - Round * 0.08f); }
bool FTPGame::OnGround() const { return Collide(Piece, Piece.X, Piece.Y + 1, Piece.Rot); }
void FTPGame::TouchLock()
{
	if (bHasPiece && OnGround() && LockMoves < MaxLockMoves)
	{
		LockT = 0.f;
		LockMoves++;
	}
}

void FTPGame::Move(int Dx)
{
	if (Phase == ETPPhase::Play && bHasPiece && !Collide(Piece, Piece.X + Dx, Piece.Y, Piece.Rot))
	{
		Piece.X += Dx;
		TouchLock();
	}
}

void FTPGame::SoftDrop()
{
	if (Phase == ETPPhase::Play && bHasPiece && !OnGround())
	{
		Piece.Y++;
		DropT = 0.f;
	}
}

void FTPGame::Rotate(int Dir)
{
	if (Phase != ETPPhase::Play || !bHasPiece) return;
	const int NR = (Piece.Rot + Dir + 4) % 4;
	for (const auto& K : GKicks)
		if (!Collide(Piece, Piece.X + K[0], Piece.Y + K[1], NR))
		{
			Piece.X += K[0];
			Piece.Y += K[1];
			Piece.Rot = NR;
			TouchLock();
			return;
		}
}

void FTPGame::HoldPiece()
{
	if (Phase != ETPPhase::Play || !bCanHold || !bHasPiece) return;
	if (CurrentRules().bNoHold)
	{
		FTPPopText T;
		T.Kind = FTPPopText::EKind::NoHoldMsg;
		T.X = 4.5f;
		T.Y = 2.f;
		Pop(T);
		return;
	}
	FTPPiece Cur = Piece;
	if (bHasHold)
	{
		Piece = Hold;
		Piece.Rot = 0;
		Piece.X = 3;
		Piece.Y = Piece.Type == ETPPieceType::I ? -1 : 0;
		Hold = Cur;
	}
	else
	{
		Hold = Cur;
		bHasHold = true;
		const int Left = PiecesLeft;
		PiecesLeft++;
		SpawnPiece();
		PiecesLeft = Left;
	}
	bCanHold = false;
}

void FTPGame::HardDrop()
{
	if (Phase != ETPPhase::Play || !bHasPiece) return;
	while (!OnGround()) Piece.Y++;
	LockPiece();
}

void FTPGame::LockPiece()
{
	const auto Cells = CellsOf(Piece);
	for (int I = 0; I < 4; ++I) Board[Cells[I].Y][Cells[I].X] = Piece.Cards[CardIndexOfCell(I)];
	bHasPiece = false;
	bCanHold = true;
	PiecesLeft--;
	Settle(Board);
	Chain = ChainScore = 0;
	ChainNow = FTPChainSummary{};
	RankSteps.clear();
	ChainStep();
}

void FTPGame::ChainStep()
{
	const std::vector<FTPHand> Hands = Detect(Board, Pool, CurrentRules());
	if (Hands.empty())
	{
		if (Chain > 0)
		{
			ChainNow.Chain = Chain;
			ChainNow.Total = ChainScore;
			Last = ChainNow;
			bHasLast = true;
			if (!bHasBest || ChainScore > Best.Total)
			{
				Best = ChainNow;
				bHasBest = true;
			}
		}
		Phase = ETPPhase::Play;
		AfterTurn();
		return;
	}
	Chain++;
	MaxChain = std::max(MaxChain, Chain);
	RankSteps.push_back(RanksOf(Hands));
	const int St = ChainStraight(RankSteps);
	int Sum = 0;
	for (const FTPHand& H : Hands) Sum += H.Score;
	const int Sc = Sum * Chain * (St ? St : 1);
	if (St)
	{
		FTPPopText T;
		T.Kind = FTPPopText::EKind::Straight;
		T.A = St;
		T.X = Cols / 2.f - 0.5f;
		T.Y = 1.5f;
		Pop(T);
	}
	Score += Sc;
	TotalScore += Sc;
	ChainScore += Sc;
	Bursts += (int)Hands.size();
	const std::vector<FTPCardCells> Cards = CardsOf(Board);
	FTPBurstFx F;
	for (const FTPHand& H : Hands)
	{
		ChainNow.Kinds.push_back(H.Kind);
		ChainNow.Flushes.push_back(H.bFlush);
		for (int Id : H.CardIds) F.Items.push_back({Pool[Id], FindCard(Cards, Id)->Cells});
		float CX = 0.f, CY = 0.f;
		for (const FTPCell& C : H.Cells) { CX += C.X; CY += C.Y; }
		FTPPopText T;
		T.Kind = FTPPopText::EKind::Hand;
		T.Hand = H.Kind;
		T.bFlush = H.bFlush;
		T.A = H.Chips;
		T.B = H.Mult;
		T.X = CX / H.Cells.size();
		T.Y = CY / H.Cells.size() - 0.3f;
		Pop(T);
	}
	Fx.push_back(std::move(F));
	Texts.erase(std::remove_if(Texts.begin(), Texts.end(), [](const FTPPopText& T) { return T.bBig; }), Texts.end());
	FTPPopText Big;
	Big.Kind = FTPPopText::EKind::Score;
	Big.A = Sc;
	Big.B = Chain;
	Big.X = Cols / 2.f - 0.5f;
	Big.Y = -0.4f;
	Big.T = 1.1f;
	Big.bBig = true;
	Pop(Big);
	RemoveHands(Board, Hands);
	Settle(Board);
	Phase = ETPPhase::Chain;
	ChainT = ChainStepTime;
}

void FTPGame::AfterTurn()
{
	if (Score >= RoundDef(Round).Target)
	{
		Phase = ETPPhase::Clear;
		Bonus = PiecesLeft;
		return;
	}
	if (PiecesLeft <= 0) { EndRound(); return; }
	SpawnPiece();
}

void FTPGame::EndRound()
{
	bHasPiece = false;
	if (Score >= RoundDef(Round).Target)
	{
		Phase = ETPPhase::Clear;
		Bonus = 0;
	}
	else
	{
		Phase = ETPPhase::Over;
		OverReason = ETPOverReason::OutOfPieces;
	}
}

void FTPGame::NextRound()
{
	if (Phase != ETPPhase::Clear) return;
	Round++;
	if (Round >= NumRounds)
	{
		Phase = ETPPhase::Win;
		return;
	}
	StartRound();
}

void FTPGame::Update(float Dt)
{
	if (bPaused) return;
	if (Phase == ETPPhase::Chain)
	{
		ChainT -= Dt;
		if (ChainT <= 0.f) ChainStep();
		return;
	}
	if (Phase != ETPPhase::Play || !bHasPiece) return;
	for (int K = 0; K < (int)ETPHeldKey::Count; ++K)
	{
		if (!Held[K]) continue;
		Rep[K] -= Dt;
		if (Rep[K] > 0.f) continue;
		const bool bDown = K == (int)ETPHeldKey::Down;
		Rep[K] = bDown ? 0.04f : 0.05f;
		if (bDown) SoftDrop();
		else Move(K == (int)ETPHeldKey::Left ? -1 : 1);
		if (!bHasPiece) return;
	}
	DropT += Dt;
	if (DropT >= Gravity())
	{
		DropT = 0.f;
		if (!OnGround()) Piece.Y++;
	}
	if (OnGround())
	{
		LockT += Dt;
		if (LockT >= LockDelay) LockPiece();
	}
	else
		LockT = 0.f;
}

void FTPGame::Tick(float Dt)
{
	Dt = std::min(Dt, 0.05f);
	Time += Dt;
	if (bStarted) Update(Dt);
	for (FTPPopText& T : Texts) T.T -= Dt;
	Texts.erase(std::remove_if(Texts.begin(), Texts.end(), [](const FTPPopText& T) { return T.T <= 0.f; }), Texts.end());
	for (FTPBurstFx& F : Fx) F.T -= Dt;
	Fx.erase(std::remove_if(Fx.begin(), Fx.end(), [](const FTPBurstFx& F) { return F.T <= 0.f; }), Fx.end());
}

void FTPGame::SetHeld(ETPHeldKey Key, bool bDown)
{
	Held[(int)Key] = bDown;
	if (bDown) Rep[(int)Key] = Key == ETPHeldKey::Down ? 0.04f : 0.16f;
}

void FTPGame::Act(ETPAction A)
{
	if (!bStarted) { bStarted = true; return; }
	if (Phase == ETPPhase::Over || Phase == ETPPhase::Win)
	{
		if (A == ETPAction::Restart || A == ETPAction::Tap || A == ETPAction::Next) NewRun();
		return;
	}
	if (Phase == ETPPhase::Clear)
	{
		if (A == ETPAction::Next || A == ETPAction::Tap) NextRound();
		return;
	}
	if (A == ETPAction::Pause) { bPaused = !bPaused; return; }
	if (bPaused) return;
	switch (A)
	{
	case ETPAction::Left: Move(-1); break;
	case ETPAction::Right: Move(1); break;
	case ETPAction::Rotate: Rotate(1); break;
	case ETPAction::RotateCcw: Rotate(-1); break;
	case ETPAction::SoftDrop: SoftDrop(); break;
	case ETPAction::HardDrop: HardDrop(); break;
	case ETPAction::Hold: HoldPiece(); break;
	default: break;
	}
}
} // namespace TP
