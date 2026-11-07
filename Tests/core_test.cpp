// 규칙 코어 시험 (언리얼 없이 실행)
//   ./Tests/run.sh          단위 시험 + 봇 40판
// 웹 프로토타입의 handtest.js, bottest.js 와 같은 경우를 확인한다.
#include "../Source/ProjectB/TPCore.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <string>

using namespace TP;

static int GFails = 0;
static void Expect(bool bOk, const std::string& Label)
{
	std::printf("%s %s\n", bOk ? "  ok " : "FAIL ", Label.c_str());
	if (!bOk) ++GFails;
}

struct FBuilder
{
	FTPGrid G = EmptyGrid();
	std::vector<FTPCard> Pool{FTPCard{}};
	int Add(int Rank, ETPSuit S) { Pool.push_back({Rank, S}); return (int)Pool.size() - 1; }
	// 세로 카드: (x, y) 와 (x, y-1)
	int V(int X, int Y, int Rank, ETPSuit S) { const int Id = Add(Rank, S); G[Y][X] = Id; G[Y - 1][X] = Id; return Id; }
	// 가로 카드: (x, y) 와 (x+1, y)
	int H(int X, int Y, int Rank, ETPSuit S) { const int Id = Add(Rank, S); G[Y][X] = Id; G[Y][X + 1] = Id; return Id; }
};

static std::string Kinds(const std::vector<FTPHand>& Hs)
{
	static const char* Names[] = {"five", "four", "fullHouse", "triple", "twoPair"};
	std::vector<std::string> V;
	for (const FTPHand& H : Hs) V.push_back(Names[(int)H.Kind]);
	std::sort(V.begin(), V.end());
	std::string S;
	for (auto& X : V) S += (S.empty() ? "" : ",") + X;
	return S.empty() ? "none" : S;
}

static void HandTests()
{
	const int B = Rows - 1;
	const ETPSuit Sp = ETPSuit::Spade, He = ETPSuit::Heart, Di = ETPSuit::Diamond, Cl = ETPSuit::Club;
	const ETPSuit Su[4] = {Sp, He, Di, Cl};
	auto Case = [](const char* Label, FBuilder& F, const char* Want, bool bExtra = true) {
		const auto Hs = Detect(F.G, F.Pool, FTPRules{});
		const std::string Got = Kinds(Hs);
		Expect(Got == Want && bExtra, std::string(Label) + " -> " + Got);
		return Hs;
	};
	{ FBuilder F; F.V(0, B, 5, Sp); F.V(1, B, 5, He); F.V(2, B, 5, Di);
	  auto Hs = Detect(F.G, F.Pool, {}); Case("세로 카드 3장 나란히 = 트리플", F, "triple", !Hs[0].bFlush && Hs[0].Cells.size() == 6); }
	{ FBuilder F; F.H(0, B, 5, Sp); F.H(0, B - 1, 5, He); F.H(1, B - 2, 5, Di); Case("가로 카드 3장 쌓기 = 트리플", F, "triple"); }
	{ FBuilder F; F.H(0, B, 3, Sp); F.H(1, B - 1, 3, He); F.V(3, B, 3, Di); Case("한 칸만 맞닿아도 붙음", F, "triple"); }
	{ FBuilder F; for (int I = 0; I < 4; ++I) F.V(I, B, 3, Su[I]); Case("포카드", F, "four"); }
	{ FBuilder F; for (int I = 0; I < 5; ++I) F.V(I, B, 6, Su[I % 4]); Case("파이브 카드", F, "five"); }
	{ FBuilder F; F.V(0, B, 4, Sp); F.V(1, B, 4, He); F.V(2, B, 4, Di); F.H(3, B, 7, Cl); F.H(3, B - 1, 7, Sp);
	  auto Hs = Detect(F.G, F.Pool, {}); Case("풀하우스", F, "fullHouse", Hs.size() == 1 && Hs[0].CardIds.size() == 5 && Hs[0].Cells.size() == 10); }
	{ FBuilder F; F.V(0, B, 4, Sp); F.V(1, B, 4, He); F.V(2, B, 7, Di); F.V(3, B, 7, Cl); Case("투 페어", F, "twoPair"); }
	{ FBuilder F; F.V(0, B, 4, Sp); F.V(1, B, 4, He); Case("원 페어는 안 터짐", F, "none"); }
	{ FBuilder F; F.V(0, B, 4, Sp); F.V(1, B, 4, He); F.V(5, B, 7, Di); F.V(6, B, 7, Cl); Case("떨어진 페어 두 쌍은 안 터짐", F, "none"); }
	{ FBuilder F; F.H(0, B, 2, Sp); F.H(2, B - 1, 2, He); F.H(4, B - 2, 2, Di); Case("대각선은 안 붙음", F, "none"); }
	{ FBuilder F; F.V(2, B, 5, He); F.V(3, B, 5, He); F.V(4, B, 5, He);
	  auto Hs = Detect(F.G, F.Pool, {}); Case("같은 무늬 트리플 = 플러시 x2", F, "triple", Hs[0].bFlush && Hs[0].Mult == 4); }
	{ FBuilder F; F.V(2, B, 5, He); F.V(3, B, 5, He); F.V(4, B, 5, He); FTPRules R; R.bNoFlush = true; R.bNoRed = true;
	  auto Hs = Detect(F.G, F.Pool, R); Expect(Hs.size() == 1 && !Hs[0].bFlush && Hs[0].Chips == 20, "보스 규칙: 플러시 없음, 빨간 카드 칩 0"); }

	auto St = [](std::vector<std::vector<int>> A) { std::vector<std::set<int>> S; for (auto& X : A) S.emplace_back(X.begin(), X.end()); return ChainStraight(S); };
	Expect(St({{3}, {4}, {5}}) == 3, "연쇄 스트레이트 3-4-5 = 3");
	Expect(St({{5}, {4}, {3}}) == 3, "연쇄 스트레이트 5-4-3 = 3");
	Expect(St({{3}, {5}, {4}}) == 0, "3-5-4 는 아님");
	Expect(St({{2, 6}, {3}, {4}, {5}}) == 4, "2-3-4-5 = 4");
	Expect(St({{3}, {4}}) == 0, "2단계만은 아님");

	{ // 받침 없는 가로 카드는 떨어지고, 한 칸만 받쳐도 걸린다
		FBuilder F; F.V(0, B, 1, Sp); const int Fl = F.H(4, B - 5, 2, He); const int Hd = F.H(0, B - 2, 3, Di);
		Settle(F.G); const auto Cs = CardsOf(F.G);
		const auto* A = FindCard(Cs, Fl); const auto* C = FindCard(Cs, Hd);
		Expect(A && A->Cells[0] == FTPCell{4, B} && A->Cells[1] == FTPCell{5, B} && C && C->Cells[0] == FTPCell{0, B - 2}, "받침 없는 카드 낙하");
	}
	{ // 2단계 연쇄: 가로 2 트리플이 터지면 위의 가로 7 이 내려와 양옆 7 과 트리플
		FBuilder F; F.H(1, B, 2, Sp); F.H(1, B - 1, 2, He); F.H(1, B - 2, 2, Di);
		F.V(0, B, 7, Cl); F.V(3, B, 7, He); F.H(1, B - 3, 7, Sp);
		const FTPSimResult R = Simulate(F.G, F.Pool, {});
		Expect(R.Chain == 2 && R.Steps[0].Score == 52 && R.Steps[1].Score == 164, "2단계 연쇄 52 -> 164 (웹과 같음), 실제 " + std::to_string(R.Steps.empty() ? 0 : R.Steps[0].Score) + " -> " + std::to_string(R.Steps.size() > 1 ? R.Steps[1].Score : 0));
	}
	{ // 모든 회전에서 카드 두 장이 각각 이웃한 칸 2개
		bool bOk = true;
		for (int T = 0; T < (int)ETPPieceType::Count; ++T)
			for (int R = 0; R < 4; ++R)
			{
				const auto& C = ShapeCells((ETPPieceType)T, R);
				auto Adj = [](FTPCell A, FTPCell D) { return std::abs(A.X - D.X) + std::abs(A.Y - D.Y) == 1; };
				std::set<std::pair<int, int>> U;
				for (auto& X : C) U.insert({X.X, X.Y});
				bOk = bOk && Adj(C[0], C[1]) && Adj(C[2], C[3]) && U.size() == 4;
			}
		Expect(bOk, "회전 모양 검사");
	}
	{ // 실제 진행: 떨어뜨리면 카드 2장, 칸 4개
		FTPGame G(7); G.bStarted = true; const FTPPreview Pv = G.Preview(); G.HardDrop();
		while (G.Phase == ETPPhase::Chain) G.ChainStep();
		const auto Cs = CardsOf(G.Board);
		Expect(Pv.CardCells[0].size() == 2 && Pv.CardCells[1].size() == 2 && Cs.size() == 2 && Cs[0].Cells.size() == 2, "블록 떨어뜨리기");
	}
}

// ---------------------------------------------------------------- 봇 (bottest.js 와 같은 평가)
static double BoardValue(const FTPGrid& Bd, const std::vector<FTPCard>& Pool)
{
	double Agg = 0, Bump = 0, Adj = 0;
	int Hs[Cols], MaxH = 0;
	for (int X = 0; X < Cols; ++X)
	{
		int H = 0;
		for (int Y = 0; Y < Rows; ++Y) if (Bd[Y][X]) { H = Rows - Y; break; }
		Hs[X] = H; Agg += H; MaxH = std::max(MaxH, H);
	}
	for (int X = 1; X < Cols; ++X) Bump += std::abs(Hs[X] - Hs[X - 1]);
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			const int C = Bd[Y][X];
			if (!C) continue;
			const int Ns[2][2] = {{X + 1, Y}, {X, Y + 1}};
			for (auto& N : Ns)
			{
				if (N[0] >= Cols || N[1] >= Rows) continue;
				const int O = Bd[N[1]][N[0]];
				if (!O) continue;
				if (Pool[O].Rank == Pool[C].Rank) Adj += 1;
				if (Pool[O].Suit == Pool[C].Suit) Adj += 0.3;
			}
		}
	return -0.6 * Agg - 0.4 * Bump + 1.2 * Adj - (MaxH > Rows - 3 ? 40 : 0);
}

static void BotTest(int Runs)
{
	std::map<int, int> Hist;
	std::map<int, std::vector<int>> PerRound;
	int MaxChainSeen = 0;
	for (int Run = 0; Run < Runs; ++Run)
	{
		FTPGame G(1000 + Run);
		G.bStarted = true;
		for (int Guard = 0; Guard < 6000; ++Guard)
		{
			if (G.Phase == ETPPhase::Chain) { G.ChainStep(); continue; }
			if (G.Phase == ETPPhase::Clear) { PerRound[G.Round + 1].push_back(G.Score); MaxChainSeen = std::max(MaxChainSeen, G.MaxChain); G.NextRound(); continue; }
			if (G.Phase != ETPPhase::Play) break;
			bool bFound = false;
			double BestSc = 0;
			int BX = 0, BR = 0;
			const FTPPiece Saved = G.Piece;
			for (int R = 0; R < 4; ++R)
				for (int X = -2; X <= Cols; ++X)
				{
					if (G.Collide(G.Piece, X, G.Piece.Y, R)) continue;
					G.Piece.X = X; G.Piece.Rot = R;
					const FTPPreview Pv = G.Preview();
					const double Sc = Pv.Sim.Total * 0.06 + BoardValue(Pv.Sim.Board, G.Pool);
					G.Piece = Saved;
					if (!bFound || Sc > BestSc) { bFound = true; BestSc = Sc; BX = X; BR = R; }
				}
			if (!bFound) break;
			G.Piece.X = BX; G.Piece.Rot = BR;
			G.HardDrop();
		}
		MaxChainSeen = std::max(MaxChainSeen, G.MaxChain);
		Hist[G.Phase == ETPPhase::Win ? 9 : G.Round + 1]++;
	}
	std::printf("\n봇 %d판 도달 라운드 (9 = 전부 통과):", Runs);
	for (auto& [K, V] : Hist) std::printf("  %d:%d", K, V);
	std::printf("\n라운드별 통과 평균 점수:");
	for (auto& [K, V] : PerRound) { long S = 0; for (int X : V) S += X; std::printf("  %d:%ld", K, S / (long)V.size()); }
	std::printf("\n최대 연쇄: %d\n", MaxChainSeen);
	// 웹 봇 40판: 6라운드 이상이 대부분. 크게 어긋나면 규칙 이식이 틀린 것
	int Reached6 = 0;
	for (auto& [K, V] : Hist) if (K >= 6) Reached6 += V;
	Expect(Reached6 * 10 >= Runs * 8, "봇 80% 이상이 6라운드 도달 (웹 프로토타입과 같은 수준)");
}

int main(int Argc, char** Argv)
{
	std::printf("[족보·낙하 단위 시험]\n");
	HandTests();
	BotTest(Argc > 1 ? std::atoi(Argv[1]) : 40);
	std::printf("\n%s (실패 %d)\n", GFails ? "실패 있음" : "모두 통과", GFails);
	return GFails ? 1 : 0;
}
