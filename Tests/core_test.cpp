// 포커 디펜스 규칙 코어 시험 (언리얼 없이 실행)
//   ./Tests/run.sh [봇 판 수]
// 족보·연쇄·길·설치·적·부적·발사체·합성·조커·스킬 트리·골드 강화를 확인하고, 봇으로 밸런스를 본다.
//   ./Tests/run.sh 0 은 봇 없이, ./Tests/run.sh N base|tree 는 그 봇만
#include "../Source/ProjectB/PDCore.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <random>
#include <set>
#include <string>
#include <tuple>

using namespace PD;

static int GFails = 0;
static void Expect(bool bOk, const std::string& Label)
{
	std::printf("%s %s\n", bOk ? "  ok " : "FAIL ", Label.c_str());
	if (!bOk) ++GFails;
}
static FPDCard C(int R, EPDSuit S = EPDSuit::Spade) { return {R, S}; }
static std::string Names(std::vector<EPDHand> V)
{
	static const char* N[] = {"royal", "five", "straightFlush", "four", "fullHouse", "flush", "straight", "triple", "twoPair", "pair"};
	std::vector<std::string> S;
	for (EPDHand H : V) S.push_back(N[(int)H]);
	std::sort(S.begin(), S.end());
	std::string O;
	for (auto& X : S) O += (O.empty() ? "" : ",") + X;
	return O.empty() ? "none" : O;
}
// 예전 ClassifyHand 의 와일드 처리 (와일드마다 숫자 · 무늬를 다 넣어 봄): 새 셈과 같은 답인지 비교용 (QA 29 문제 2)
static std::vector<EPDHand> RefClassify(const std::vector<FPDCard>& Cards)
{
	for (size_t W = 0; W < Cards.size(); ++W)
	{
		if (!Cards[W].IsWild()) continue;
		std::vector<EPDSuit> Suits;
		for (const FPDCard& Cd : Cards) if (!Cd.IsWild() && std::find(Suits.begin(), Suits.end(), Cd.Suit) == Suits.end()) Suits.push_back(Cd.Suit);
		if (Suits.empty()) Suits.push_back(EPDSuit::Spade);
		std::set<EPDHand> All;
		std::vector<FPDCard> Try = Cards;
		for (int R = 2; R <= 14; ++R)
			for (EPDSuit Su : Suits)
			{
				Try[W] = {R, Su};
				for (EPDHand H : RefClassify(Try)) All.insert(H);
			}
		return std::vector<EPDHand>(All.begin(), All.end());
	}
	return ClassifyHand(Cards); // 와일드 없는 패는 그대로
}

static void Put(FPDTowerMap& M, int X, int Y, FPDCard Card, int Level = 1)
{
	FPDTower T;
	T.Card = Card;
	T.X = X;
	T.Y = Y;
	T.Level = Level;
	M[CellKey(X, Y)] = T;
}
// 지휘봉 레시피를 진짜 카드로 한 줄에 (D-487): 스트레이트 2♠3♥4♠ · 원 페어 9♠9♥ · 원 페어 K♠K♥ · 플러시 5♦8♦J♦, 사이는 빈 칸 (0~12열, 카드 10장)
static void PutBaton(FPDTowerMap& M, int Row)
{
	const EPDSuit H = EPDSuit::Heart, D = EPDSuit::Diamond;
	Put(M, 0, Row, C(2)); Put(M, 1, Row, C(3, H)); Put(M, 2, Row, C(4));
	Put(M, 4, Row, C(9)); Put(M, 5, Row, C(9, H));
	Put(M, 7, Row, C(13)); Put(M, 8, Row, C(13, H));
	Put(M, 10, Row, C(5, D)); Put(M, 11, Row, C(8, D)); Put(M, 12, Row, C(11, D));
}
// 따로 선 합성 타워 하나 (레벨 Lv, 기억한 족보 H): 조커 재료 하나가 된다 (D-220)
static void PutFused(FPDTowerMap& M, int X, int Y, FPDCard Card, EPDHand H, int Lv = 2, float Mult = 1.f)
{
	Put(M, X, Y, Card, Lv);
	M[CellKey(X, Y)].FuseHand = (int)H;
	M[CellKey(X, Y)].Mult = Mult;
}

// 판 왼쪽 N칸이 모두 비어 설치할 수 있는 첫 줄
static int FreeRow(const FPDGame& G, int N)
{
	for (int Y = 0; Y < Rows; ++Y)
	{
		bool bFree = true;
		for (int X = 0; X < N; ++X) bFree = bFree && G.Buildable(X, Y);
		if (bFree) return Y;
	}
	return -1;
}
// 길 바로 옆(왼쪽·오른쪽)에 설치할 수 있는 첫 칸과, 그 칸에 가장 가까운 길 위 거리
static void NearPathCell(const FPDGame& G, int& JX, int& JY, float& NearD)
{
	JX = JY = -1;
	NearD = 0.f;
	for (int Y = 0; Y < Rows && JX < 0; ++Y) for (int X = 0; X < Cols; ++X) if (G.Buildable(X, Y) && ((X > 0 && G.Path.IsPath(X - 1, Y)) || (X + 1 < Cols && G.Path.IsPath(X + 1, Y)))) { JX = X; JY = Y; break; }
	float BestDist = 1e9f;
	for (float D = 0.f; D < G.Path.Length; D += 0.1f) { const FPDPoint P = G.Path.At(D); const float Dist = std::hypot(P.X - JX - 0.5f, P.Y - JY - 0.5f); if (Dist < BestDist) { BestDist = Dist; NearD = D; } }
}
static void UnitTests()
{
	{ // 높은 레벨에서 경험치가 무한 반복에 빠지지 않는다 (D-268): 레벨 300 이어하기 판이 멈추던 것
		FPDGame G;
		G.NewGame(false);
		G.Level = 300;
		G.Xp = 0;
		Expect(G.XpToNext() > 0, "레벨 300: 다음 레벨 경험치가 양수 (AddXp 반복이 끝난다)");
		G.Level = 94; G.Xp = 0;
		Expect(G.XpToNext() > 0, "레벨 94: 다음 레벨 경험치가 넘치지 않음");
	}
	const EPDSuit He = EPDSuit::Heart, Di = EPDSuit::Diamond, Cl = EPDSuit::Club;
	auto Case = [](const char* Label, std::vector<FPDCard> Cards, const char* Want) {
		const std::string Got = Names(ClassifyHand(Cards));
		Expect(Got == Want, std::string(Label) + " -> " + Got);
	};
	Case("원 페어", {C(7), C(7, He)}, "pair");
	Case("떨어진 숫자 2장", {C(7), C(9, He)}, "none");
	Case("이어진 숫자 2장은 아직 아님 (D-418)", {C(4, He), C(3)}, "none");
	Case("Q-K-A도 이어짐", {C(12), C(13, He), C(14)}, "straight");
	Case("같은 무늬 2장은 아직 아님", {C(7, He), C(9, He)}, "none");
	Case("플러시 3장", {C(7, He), C(9, He), C(2, He)}, "flush");
	Case("스트레이트 3장", {C(7, He), C(9), C(8, Di)}, "straight");
	Case("A-2-3 스트레이트", {C(14, He), C(2), C(3, Di)}, "straight");
	Case("트리플", {C(5), C(5, He), C(5, Di)}, "triple");
	Case("투 페어", {C(5), C(9, He), C(5, Di), C(9)}, "twoPair");
	Case("포카드", {C(5), C(5, He), C(5, Di), C(5, Cl)}, "four");
	Case("풀하우스", {C(5), C(9, He), C(5, Di), C(9), C(5, Cl)}, "fullHouse");
	Case("스트레이트 플러시", {C(5), C(6), C(7), C(8), C(9)}, "flush,straight,straightFlush");
	Case("로열 스트레이트 플러시", {C(10), C(11), C(12), C(13), C(14)}, "flush,royal,straight,straightFlush");
	Case("파이브 카드 (와일드)", {C(9), C(9, EPDSuit::Heart), C(9, EPDSuit::Club), C(9, EPDSuit::Diamond), {0, EPDSuit::Wild}}, "five");
	Case("파이브 카드 (다시 채운 덱의 같은 카드)", {C(4), C(4), C(4, EPDSuit::Heart), C(4, EPDSuit::Club), C(4, EPDSuit::Diamond)}, "five");

	{ // ㄴ자 플러시
		FPDTowerMap M;
		Put(M, 0, 0, C(7, He)); Put(M, 0, 1, C(2, He)); Put(M, 1, 1, C(11, He));
		auto S = ApplyEffects(M);
		Expect(S.size() == 1 && S[0].Kind == EPDHand::Flush && S[0].N == 3 && M[CellKey(0, 0)].Eff.Splash == 1.25f, "ㄴ자 하트 3장 = 플러시3");
	}
	{ // 연쇄: 스트레이트(7 8 9) + 트리플(5 5 5) 맞닿음 (D-418: 스트레이트는 3장부터)
		FPDTowerMap M;
		Put(M, 0, 3, C(7)); Put(M, 1, 3, C(8, He)); Put(M, 2, 3, C(9, Di)); Put(M, 3, 3, C(5)); Put(M, 4, 3, C(5, He)); Put(M, 5, 3, C(5, Di));
		auto S = ApplyEffects(M);
		Expect(S.size() == 2 && S[0].Chain == 2 && S[1].Chain == 2 && M[CellKey(0, 3)].Eff.Dmg == 1.25f && M[CellKey(4, 3)].Eff.Dmg == 2.5f, "맞닿은 족보 = 연쇄 2 (공격력 ×1.25)");
		FPDTowerMap F;
		Put(F, 0, 3, C(7)); Put(F, 1, 3, C(7, He)); Put(F, 2, 3, C(5)); Put(F, 3, 3, C(5, He)); Put(F, 4, 3, C(5, Di));
		auto SF = ApplyEffects(F);
		Expect(SF.size() == 1 && SF[0].Kind == EPDHand::FullHouse, "7 7 5 5 5 는 풀하우스 하나");
		FPDTowerMap D;
		Put(D, 0, 6, C(7)); Put(D, 1, 6, C(7, He)); Put(D, 3, 6, C(5)); Put(D, 4, 6, C(5, He));
		auto SD = ApplyEffects(D);
		Expect(SD.size() == 2 && SD[0].Chain == 1 && SD[1].Chain == 1, "떨어진 족보는 연쇄 아님");
		FPDTowerMap T3;
		Put(T3, 0, 5, C(7)); Put(T3, 1, 5, C(7, He)); Put(T3, 2, 5, C(7, Di));
		Expect(ApplyEffects(T3).size() == 1 && ApplyEffects(T3)[0].Kind == EPDHand::Triple, "7 7 7 은 페어가 아니라 트리플");
	}
	{ // 스트레이트는 순서대로 (D-108): 맞닿은 줄기를 따라 1씩, 꺾여도 되고 거꾸로도 됨
		auto HasStraight = [](FPDTowerMap M) { for (const FPDSynergy& S : ApplyEffects(M)) if (S.Kind == EPDHand::Straight) return (int)S.N; return 0; };
		FPDTowerMap A, B, L, R, W;
		Put(A, 0, 0, C(5)); Put(A, 1, 0, C(6, He)); Put(A, 2, 0, C(7, Di));
		Put(B, 0, 0, C(5)); Put(B, 1, 0, C(7, He)); Put(B, 2, 0, C(6, Di));
		Put(L, 0, 0, C(5)); Put(L, 1, 0, C(6, He)); Put(L, 1, 1, C(7, Di));
		Put(R, 0, 0, C(7)); Put(R, 1, 0, C(6, He)); Put(R, 2, 0, C(5, Di));
		Put(W, 0, 0, C(14)); Put(W, 1, 0, C(2, He)); Put(W, 2, 0, C(3, Di));
		Expect(HasStraight(A) == 3 && HasStraight(L) == 3 && HasStraight(R) == 3 && HasStraight(W) == 3, "순서대로: 5 6 7, ㄱ자 5→6→7, 거꾸로 7 6 5, A 2 3 은 스트레이트");
		Expect(HasStraight(B) == 0, "5 7 6 은 스트레이트가 아님 (붙은 7 6 은 2장이라 족보 아님, D-418)");
	}
	{ // 카드 공유 (D-097): 한 장이 여러 족보에 든다
		FPDTowerMap M;
		Put(M, 0, 0, C(5)); Put(M, 1, 0, C(6, He)); Put(M, 2, 0, C(7, Di)); Put(M, 3, 0, C(7, Cl));
		auto S = ApplyEffects(M);
		const FPDEffects& E7 = M[CellKey(2, 0)].Eff;
		Expect(S.size() == 2 && S[0].Kind == EPDHand::Straight && S[1].Kind == EPDHand::Pair && S[0].Chain == 2,
			"5 6 7 + 7 = 스트레이트와 원 페어가 7 을 나눠 씀 (연쇄 2)");
		Expect(E7.Kind == (int)EPDHand::Straight && E7.Kinds == ((1u << (int)EPDHand::Straight) | (1u << (int)EPDHand::Pair)) && std::fabs(E7.Dmg - 1.5f * 1.25f) < 1e-4f && std::fabs(E7.Rate - 1.5f) < 1e-4f,
			"나눠 쓴 카드는 두 족보 효과를 다 받고 연쇄 덤은 한 번");
		FPDTowerMap M8;
		Put(M8, 0, 0, C(5)); Put(M8, 1, 0, C(6, He)); Put(M8, 2, 0, C(7, Di)); Put(M8, 3, 0, C(8, Cl)); Put(M8, 3, 1, C(8, He));
		auto S8 = ApplyEffects(M8);
		Expect(S8.size() == 2 && S8[0].Kind == EPDHand::Straight && S8[0].N == 4 && S8[1].Kind == EPDHand::Pair, "5 6 7 8 + 8 = 스트레이트4 와 원 페어");
		FPDTowerMap MS;
		Put(MS, 0, 0, C(5)); Put(MS, 1, 0, C(6, He)); Put(MS, 2, 0, C(7, Di)); Put(MS, 1, 1, C(7, Cl));
		auto SS = ApplyEffects(MS);
		Expect(SS.size() == 1, "같은 종류 족보(스트레이트 5 6 7 과 6 7)는 카드를 나눠 쓰지 않음");
		FPDTowerMap MH;
		Put(MH, 0, 0, C(9)); Put(MH, 1, 0, C(9, He)); Put(MH, 0, 1, C(5)); Put(MH, 1, 1, C(5, He));
		auto SH = ApplyEffects(MH);
		Expect(SH.size() == 1 && SH[0].Kind == EPDHand::TwoPair, "9 9 / 5 5 는 투 페어 하나 (안의 페어는 따로 안 셈)");
	}
	{ // 무작위 길
		std::mt19937 R(5);
		std::set<std::vector<std::array<int, 2>>> Uniq;
		bool bOk = true;
		for (int I = 0; I < 200; ++I)
		{
			auto P = GeneratePath(R);
			Uniq.insert(P);
			FPDPath Pa;
			Pa.Build(P);
			bOk = bOk && Pa.CellCount() >= PathMin && Pa.CellCount() <= PathMax && P.front()[0] == -1 && P.back()[0] == Cols;
		}
		Expect(bOk && Uniq.size() > 150, "무작위 길 200개: 서로 다르고 길이 34~46칸");
	}
	{ // 설치와 교체
		FPDGame G(3);
		G.Swaps = 0;
		G.NewGame(true);
		Expect(G.Swaps == 2 && StartSwaps == 2, "새 판은 자리바꿈 2번으로 시작 (D-489)");
		G.Hand = {C(9), C(9, He), C(4, Di)};
		int PX = -1, PY = -1;
		for (int Y = 0; Y < Rows && PX < 0; ++Y) for (int X = 0; X < Cols; ++X) if (G.Path.IsPath(X, Y)) { PX = X; PY = Y; break; }
		const bool bOnPath = G.Place(0, PX, PY);
		const bool bP1 = G.Place(0, 0, 0), bP2 = G.Place(0, 1, 0);
		Expect(!bOnPath && bP1 && bP2 && G.Synergies.size() == 1 && G.Synergies[0].Kind == EPDHand::Pair && G.Hand.size() == 1, "설치 (길 위는 불가), 페어 생김");
		G.Place(0, 1, 0);
		Expect(G.Towers[CellKey(1, 0)].Card.Rank == 4 && G.Synergies.empty() && G.Discarded == 0 && G.Hand.size() == 1 && G.Hand[0].Rank == 9 && G.Hand[0].Suit == He, "Lv1 타워 위에 놓으면 교체, 덮인 카드는 손으로 (D-192)");
		{ // 합성 타워·조커 위에는 못 놓는다 (D-192)
			FPDGame H(3);
			H.NewGame(true);
			H.Hand = {C(5), C(6)};
			Put(H.Towers, 0, 0, C(9), 2);
			Put(H.Towers, 1, 0, C(2));
			H.Towers[CellKey(1, 0)].Joker = (int)EPDJoker::Star;
			const bool bFused = H.Place(0, 0, 0), bJoker = H.Place(0, 1, 0);
			Expect(!bFused && !bJoker && !H.CanPlace(0, 0) && !H.CanPlace(1, 0) && H.Hand.size() == 2 && H.Towers.at(CellKey(0, 0)).Level == 2, "합성 타워·조커 위에는 못 놓는다 (D-192)");
		}
		// 자리바꿈 (D-149): 남은 횟수가 있어야, 타워끼리 바꾸기 / 빈 칸으로 옮기기 / 길 위는 안 됨
		G.Hand = {C(4, He)};
		G.Towers[CellKey(0, 0)].Level = 2;
		G.Swaps = 0;
		const bool bNone = G.SwapTowers(CellKey(0, 0), CellKey(1, 0));
		G.Swaps = 3;
		const bool bSw = G.SwapTowers(CellKey(0, 0), CellKey(1, 0));
		const bool bSwOk = bSw && G.Towers[CellKey(1, 0)].Card.Rank == 9 && G.Towers[CellKey(1, 0)].Level == 2 && G.Towers[CellKey(1, 0)].X == 1 && G.Towers[CellKey(0, 0)].Card.Rank == 4;
		const bool bPath = G.SwapTowers(CellKey(1, 0), CellKey(PX, PY));
		int EX = -1;
		for (int X = 2; X < Cols && EX < 0; ++X) if (G.Buildable(X, 0) && !G.Towers.count(CellKey(X, 0))) EX = X;
		const bool bMove = EX >= 0 && G.SwapTowers(CellKey(1, 0), CellKey(EX, 0)) && !G.Towers.count(CellKey(1, 0)) && G.Towers[CellKey(EX, 0)].Level == 2;
		Expect(!bNone && bSwOk && !bPath && bMove && G.Swaps == 1, "자리바꿈: 횟수 필요, 바꾸기·옮기기(레벨 따라감), 길 위 불가");
		// 중단 저장 (D-162): 저장 → 다른 판에 불러오기 → 같은 상태, 같은 다음 카드
		{
			G.Xp = 12; G.Level = 6; G.Points = 3; G.Charms = {(EPDCharm)1}; G.TreeOwned.set(5); G.Wave = 9;
			G.Towers[CellKey(EX, 0)].Joker = 2; G.Towers[CellKey(EX, 0)].JLevel = 4; G.Towers[CellKey(EX, 0)].JXp = 12.5f;
			const std::string Sv = G.SaveRun();
			FPDGame H(77);
			H.NewGame(true);
			const bool bLoad = H.LoadRun(Sv);
			bool bSame = bLoad && H.SaveRun() == Sv && H.Path.Points.size() == G.Path.Points.size() && H.Towers.size() == G.Towers.size() && H.Phase == EPDPhase::Build;
			bSame = bSame && H.Towers[CellKey(EX, 0)].JLevel == 4 && H.Xp == 12 && H.Level == 6 && H.Swaps == G.Swaps && H.Hand.size() == G.Hand.size() && H.Deck.size() == G.Deck.size();
			FPDGame Bad(5);
			Bad.NewGame(true);
			const int BadLevel = Bad.Level;
			const bool bReject = !Bad.LoadRun("pdrun 1\nwave 3\nnonsense 1\n") && Bad.Level == BadLevel && !Bad.LoadRun("hello");
			Expect(bSame && bReject, "중단 저장: 저장한 판을 그대로 불러오고, 깨진 글은 거절");
		}
		std::map<int, int> Seen;
		for (int I = 0; I < 400; ++I) for (const FPDReward& R : G.RollRewards()) if (!R.bCharm) Seen[R.Id]++;
		Expect(Seen[(int)EPDInstant::Swap] > 60, "부적 창에 자리바꿈이 자주 나온다 (" + std::to_string(Seen[(int)EPDInstant::Swap]) + "/400)");
	}
	{ // 와일드 카드
		const FPDCard W{0, EPDSuit::Wild};
		auto HasH = [](std::vector<EPDHand> V, EPDHand H) { return std::find(V.begin(), V.end(), H) != V.end(); };
		Expect(HasH(ClassifyHand({C(7), W}), EPDHand::Pair) && HasH(ClassifyHand({C(2, He), C(9, He), W}), EPDHand::Flush) && HasH(ClassifyHand({C(4), C(5, Di), W}), EPDHand::Straight) && !HasH(ClassifyHand({C(4), W}), EPDHand::Straight),
			"와일드 카드: 페어·플러시·스트레이트를 이어 줌");
	}
	{ // 와일드가 든 로열 스트레이트 플러시 (판 위, 순서대로)
		const FPDCard W{0, EPDSuit::Wild};
		auto HasRoyal = [](FPDTowerMap M) { for (const FPDSynergy& S : ApplyEffects(M)) if (S.Kind == EPDHand::RoyalFlush) return true; return false; };
		FPDTowerMap A, B, Cc;
		Put(A, 0, 0, C(10)); Put(A, 1, 0, C(11)); Put(A, 2, 0, W); Put(A, 3, 0, C(13)); Put(A, 4, 0, C(14));
		Put(B, 0, 0, W); Put(B, 1, 0, C(11)); Put(B, 2, 0, C(12)); Put(B, 3, 0, C(13)); Put(B, 4, 0, C(14));
		Put(Cc, 0, 0, C(10)); Put(Cc, 1, 0, C(11)); Put(Cc, 2, 0, C(12)); Put(Cc, 3, 0, C(13)); Put(Cc, 4, 0, W);
		Expect(HasRoyal(A) && HasRoyal(B) && HasRoyal(Cc), "와일드가 가운데·10 자리·A 자리에 있어도 로열");
		FPDTowerMap Lv2, Jk;
		Put(Lv2, 0, 0, C(10), 2); Put(Lv2, 1, 0, C(11), 2); Put(Lv2, 2, 0, W, 2); Put(Lv2, 3, 0, C(13), 2); Put(Lv2, 4, 0, C(14), 2);
		Put(Jk, 0, 0, C(10)); Put(Jk, 1, 0, C(11)); Put(Jk, 2, 0, C(12)); Put(Jk, 3, 0, C(13)); Put(Jk, 4, 0, C(2, He));
		Jk[CellKey(4, 0)].Joker = (int)EPDJoker::Star; // A 자리에 조커
		FPDTowerMap Lo9;
		Put(Lo9, 0, 0, W); Put(Lo9, 1, 0, C(10)); Put(Lo9, 2, 0, C(11)); Put(Lo9, 3, 0, C(12)); Put(Lo9, 4, 0, C(13));
		auto HasSF = [](FPDTowerMap M) { for (const FPDSynergy& S : ApplyEffects(M)) if (S.Kind == EPDHand::StraightFlush) return true; return false; };
		Expect(!HasRoyal(Lo9) && HasSF(Lo9), "와일드 10 J Q K 에서 와일드가 10 쪽 끝이면 9 라서 로열 아님, 스트레이트 플러시 (D-190)");
		Expect(HasRoyal(Lv2), "Lv2 와일드가 끼어도 로열");
		Expect(!HasRoyal(Jk) && !HasSF(Jk), "A 자리 조커는 로열을 메우지 않는다: 10 J Q K 만 남아 로열·스트레이트 플러시 아님 (D-489)");
	}
	{ // 와일드가 든 패 셈 (QA 29 문제 2): 예전처럼 다 넣어 본 답과 같아야 하고, 와일드 3 · 5장 맞닿은 판도 빨라야
		std::mt19937 Rr(4242);
		int Bad = 0;
		for (int T = 0; T < 6000; ++T)
		{
			const int N = 2 + (int)(Rr() % 4), NW = 1 + (int)(Rr() % std::min(N, 3));
			std::vector<FPDCard> Cards;
			const int SuitSpread = 1 + (int)(Rr() % 3); // 무늬가 같은 패도 자주 나오게
			const int Base = 2 + (int)(Rr() % 13);
			for (int I = 0; I < N - NW; ++I) Cards.push_back({Rr() % 2 ? 2 + (int)(Rr() % 13) : std::min(14, Base + (int)(Rr() % 4)), (EPDSuit)(Rr() % SuitSpread)});
			for (int I = 0; I < NW; ++I) Cards.insert(Cards.begin() + Rr() % (Cards.size() + 1), FPDCard{0, EPDSuit::Wild});
			if (ClassifyHand(Cards) != RefClassify(Cards)) Bad++;
		}
		Expect(Bad == 0, "와일드가 든 패: 셈으로 낸 족보가 예전(다 넣어 보기)과 같음 (6000패, 다름 " + std::to_string(Bad) + ")");
		FPDTowerMap Wb;
		for (int Y = 0; Y < 4; ++Y) for (int X = 0; X < 8; ++X) Put(Wb, X, Y, C(2 + (X * 5 + Y * 3) % 13, (EPDSuit)((X + Y) % 4)));
		for (int X = 2; X < 7; ++X) Put(Wb, X, 1, FPDCard{0, EPDSuit::Wild}); // 와일드 5장 맞닿음 (예전 357ms 남짓)
		const auto T0 = std::chrono::steady_clock::now();
		const std::vector<FPDSynergy> SW = ComputeSynergies(Wb);
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
		char Buf[48];
		std::snprintf(Buf, sizeof Buf, "%.2fms, 족보 %d", Ms, (int)SW.size());
		Expect(Ms < 10.0 && !SW.empty(), std::string("와일드 5장 맞닿은 32칸 판 족보 계산 10ms 아래: ") + Buf);
	}
	{ // 조커는 족보에 안 든다 (D-489): 빈 칸처럼 줄기를 끊는다. 와일드 카드는 그대로 만능
		auto Joker = [](FPDTowerMap& M, int X, int Y, EPDJoker K) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Y; T.Joker = (int)K; T.Mult = 4.f; M[CellKey(X, Y)] = T; };
		auto Has = [](const std::vector<FPDSynergy>& S, EPDHand H, int N) { for (const FPDSynergy& X : S) if (X.Kind == H && (int)X.Cells.size() == N) return true; return false; };
		auto NoJokerCell = [](const FPDTowerMap& M, const std::vector<FPDSynergy>& S) { for (const FPDSynergy& X : S) for (int K : X.Cells) if (M.at(K).IsJoker()) return false; return true; };
		FPDTowerMap Cut, Side, Wi, Jm;
		// 5 6 [조커] 7 8: 줄기가 끊겨 스트레이트 없음 (조커를 건너뛰지 않음)
		Put(Cut, 0, 0, C(5)); Put(Cut, 1, 0, C(6, EPDSuit::Heart)); Joker(Cut, 2, 0, EPDJoker::Baton); Put(Cut, 3, 0, C(7, EPDSuit::Club)); Put(Cut, 4, 0, C(8, EPDSuit::Diamond));
		const std::vector<FPDSynergy> SCut = ComputeSynergies(Cut);
		bool bStraight = false;
		for (const FPDSynergy& X : SCut) bStraight = bStraight || X.Kind == EPDHand::Straight;
		Expect(!bStraight && NoJokerCell(Cut, SCut), "5 6 [조커] 7 8 = 스트레이트 아님: 조커가 줄기를 끊는다 (D-489)");
		// 5 6 7 8 9 옆에 조커가 붙어 있어도 스트레이트 5장 그대로, 조커는 안 든다
		for (int X = 0; X < 5; ++X) Put(Side, X, 0, C(5 + X, X % 2 ? EPDSuit::Heart : EPDSuit::Spade));
		Joker(Side, 2, 1, EPDJoker::Clown);
		const std::vector<FPDSynergy> SSide = ComputeSynergies(Side);
		Expect(Has(SSide, EPDHand::Straight, 5) && NoJokerCell(Side, SSide), "스트레이트 옆 조커: 스트레이트 5장 그대로, 조커 칸은 족보 밖 (D-489)");
		// 9 [와일드] 9 = 트리플 (와일드는 만능), 9 [조커] 9 = 족보 없음
		Put(Wi, 0, 0, C(9)); Put(Wi, 1, 0, FPDCard{0, EPDSuit::Wild}); Put(Wi, 2, 0, C(9, EPDSuit::Heart));
		Put(Jm, 0, 0, C(9)); Joker(Jm, 1, 0, EPDJoker::Jack); Put(Jm, 2, 0, C(9, EPDSuit::Heart));
		Expect(Has(ComputeSynergies(Wi), EPDHand::Triple, 3) && ComputeSynergies(Jm).empty(), "9 와일드 9 = 트리플, 9 조커 9 = 족보 없음 (와일드만 만능, D-489)");
		// 조커 셋이 맞닿은 판 (QA 26 부록 A, 예전 135ms): 족보 계산 3ms 아래, 조커는 어느 족보에도 없음
		FPDTowerMap A;
		const int Rows26[][6] = {{8, 1, 11, 0, 2, -1}, {2, 2, 11, 2, 1, -1}, {4, 2, 13, 3, 1, -1}, {5, 2, 10, 1, 1, -1}, {4, 3, 4, 2, 1, -1}, {5, 3, 0, 4, 2, 2},
			{3, 4, 14, 1, 1, -1}, {4, 4, 12, 3, 1, -1}, {5, 4, 0, 4, 1, 3}, {6, 4, 0, 4, 2, 5}, {7, 4, 9, 0, 1, -1}, {8, 4, 8, 0, 1, -1}, {11, 4, 3, 1, 1, -1},
			{5, 5, 7, 3, 1, -1}, {6, 5, 2, 2, 1, -1}, {7, 5, 9, 3, 1, -1}, {11, 5, 6, 3, 1, -1}, {13, 5, 13, 1, 1, -1}, {14, 5, 8, 1, 1, -1}, {3, 6, 12, 3, 1, -1},
			{11, 6, 13, 0, 1, -1}, {13, 6, 5, 0, 1, -1}, {11, 7, 12, 1, 1, -1}};
		for (const auto& R : Rows26)
		{
			FPDTower T; T.X = R[0]; T.Y = R[1]; T.Card = {R[2], (EPDSuit)R[3]}; T.Level = R[4]; T.Joker = R[5];
			if (R[0] == 8 && R[1] == 1) T.FuseHand = 9;
			A[CellKey(R[0], R[1])] = T;
		}
		const auto T0 = std::chrono::steady_clock::now();
		const std::vector<FPDSynergy> SA = ComputeSynergies(A); // 처음 보는 판이라 덩어리 캐시 없이 다 찾는다
		const double Ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - T0).count();
		char Buf[64];
		std::snprintf(Buf, sizeof Buf, "%.2fms, 족보 %d", Ms, (int)SA.size());
		Expect(Ms < 3.0 && NoJokerCell(A, SA), std::string("조커 셋 맞닿은 판 (QA 26 부록 A) 족보 계산 3ms 아래, 조커 칸은 족보 밖: ") + Buf);
	}
	{ // 적: 방패, 관통탄, 분열, 재생, 보스
		FPDGame G(4);
		G.NewGame(true);
		G.bMuted = true;
		FPDEnemy A = G.SpawnTestEnemy(EPDEnemy::Armored, 100.f, 10); // 방패 4 + 10*0.6 = 10
		G.HitTest(A, 20.f);
		const float AfterArmor = 130.f - A.Hp;
		G.Charms = {EPDCharm::Pierce};
		G.HitTest(A, 20.f);
		Expect(std::fabs(AfterArmor - 10.f) < 1e-4f && std::fabs(130.f - A.Hp - AfterArmor - 20.f) < 1e-4f, "방패는 한 방 피해를 깎고, 관통탄은 무시");
		G.Charms.clear();
		G.Phase = EPDPhase::Wave;
		G.Enemies = {G.SpawnTestEnemy(EPDEnemy::Splitter, 50.f, 6)};
		G.Enemies[0].D = 3.f;
		G.HitTest(G.Enemies[0], 9999.f);
		G.Update(0.f);
		int Minis = 0;
		for (auto& E : G.Enemies) Minis += E.Type == EPDEnemy::Mini;
		Expect(Minis == 2, "분열체는 죽으면 작은 적 둘");
		FPDEnemy R = G.SpawnTestEnemy(EPDEnemy::Regen, 100.f, 8);
		R.Hp = 50.f;
		R.Speed = 0.f;
		G.Enemies = {R};
		G.Update(1.f);
		Expect(!G.Enemies.empty() && G.Enemies[0].Hp > 50.f && G.Enemies[0].Hp < 60.f, "재생자는 체력이 찬다");
	}
	{ // 부적과 보상
		FPDGame G(6);
		G.NewGame(true);
		G.bMuted = true;
		FPDTower T;
		T.Card = C(9, He);
		const float Base = TowerStats(T, G.CharmMask()).Dmg;
		G.Phase = EPDPhase::Reward;
		G.Rewards = {{true, (int)EPDCharm::HeartLover}};
		G.TakeReward(0);
		Expect(std::fabs(TowerStats(T, G.CharmMask()).Dmg - Base * 1.4f) < 1e-3f && G.Phase == EPDPhase::Build && G.Charms.size() == 1, "하트 애호가 부적: ♥ 공격력 ×1.4");
		auto R1 = G.RollRewards();
		int J1 = 0, W1 = 0;
		for (auto& R : R1) { J1 += R.bCharm; W1 += !R.bCharm && R.Id == (int)EPDInstant::Wild; }
		G.Charms = {EPDCharm::Twins, EPDCharm::Scope, EPDCharm::Supply, EPDCharm::Lucky7, EPDCharm::Frost};
		auto R2 = G.RollRewards();
		bool bAllInst = true;
		for (auto& R : R2) bAllInst = bAllInst && !R.bCharm;
		Expect(R1.size() == 3 && (J1 == 3 || (J1 == 2 && W1 == 1)) && bAllInst && R2.size() == 3, "부적 창: 부적 3 (와일드면 부적 2 + 와일드), 부적 칸이 차면 즉시만");
		G.Charms = {EPDCharm::BigHand};
		Expect(G.HandMax() == 9, "큰 손: 손패 최대 9");
		{
			FPDGame H = G;
			H.Charms = {EPDCharm::BigHand, EPDCharm::Dexterity};
			H.Mods.HandAdd = 1;
			Expect(H.HandMax() == 10, "손패 덤을 다 더해도 최대 10장 (7+2+1+1 = 11 → 10, D-397)");
		}
		G.Phase = EPDPhase::Reward;
		G.Rewards = {{false, (int)EPDInstant::Wild}};
		G.TakeReward(0);
		Expect(!G.Hand.empty() && G.Hand.back().IsWild(), "와일드 카드 보상");
	}
	{ // 와일드 카드: 부적 창(5웨이브마다)의 절반쯤에만
		FPDGame G(31);
		G.NewGame(true);
		int Wild = 0;
		for (int W = CharmEvery; W <= 300; W += CharmEvery)
		{
			G.Wave = W;
			bool bW = false;
			for (const FPDReward& R : G.RollRewards()) bW = bW || (!R.bCharm && R.Id == (int)EPDInstant::Wild);
			Wild += bW;
		}
		Expect(Wild >= 18 && Wild <= 42, "와일드 보상 300웨이브(부적 창 60번)에 " + std::to_string(Wild) + "번");
		G.Phase = EPDPhase::Reward;
		G.Rewards = {{false, (int)EPDInstant::Xp}};
		G.Wave = 10;
		const int G0 = G.XpEarned;
		G.TakeReward(0);
		Expect(G.XpEarned - G0 == 50, "경험치 보상 10 + 웨이브 × 4 (D-228)");
	}
	{ // 판 위력 비교 (D-107): 합성 버튼을 누르기 전 지금 / 누른 뒤
		FPDGame G(14);
		G.NewGame(true);
		G.bMuted = true;
		G.Phase = EPDPhase::Build;
		G.Towers.clear();
		Put(G.Towers, 2, 0, C(9, He));
		Put(G.Towers, 3, 0, C(9, EPDSuit::Spade));
		G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
		const auto P = G.PreviewPower(FPDGame::EPowerAction::Fuse, 0);
		const auto Bad = G.PreviewPower(FPDGame::EPowerAction::Fuse, 99);
		Expect(G.Synergies.size() == 1 && P.Before > 0.f && P.After > 0.f && std::fabs(P.Before - G.Power()) < 1e-3f && Bad.After < 0.f && G.Towers.size() == 2,
			"합성 전 판 위력 비교: " + std::to_string((int)P.Before) + " → " + std::to_string((int)P.After) + " (원본 판은 그대로)");
		// 화면 위력 정보 (D-119): 타워마다 몫의 합 = 판 위력, 큰 것부터
		Put(G.Towers, 8, 5, C(13, EPDSuit::Diamond));
		G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
		const auto TP = G.TowerPowers();
		float Sum = 0.f;
		for (const auto& KV : TP) Sum += KV.second;
		Expect(TP.size() == 3 && std::fabs(Sum - G.Power()) < G.Power() * 1e-4f && TP[0].second >= TP[1].second && TP[1].second >= TP[2].second,
			"타워별 위력 합 = 판 위력 (" + std::to_string((int)Sum) + ")");
		G.Wave = 3; G.Phase = EPDPhase::Build;
		const int B1 = G.NextBossWave();
		G.Wave = 5; G.Phase = EPDPhase::Wave;
		const int B2 = G.NextBossWave();
		G.Phase = EPDPhase::Build;
		const int B3 = G.NextBossWave();
		Expect(B1 == 5 && B2 == 5 && B3 == 10 && G.BossOfWave(20) == EPDEnemy::SplitBoss, "다음 보스 웨이브: 3 → 5, 5 진행 중 → 5, 5 끝 → 10, 20웨이브 = 분열 군주 (D-335 고정 차례)");
	}
	{ // 보스 확장 (D-334, D-335): 고정 차례 · 짜임 · 최종 보스 · 무한 뽑기 · 변이 · 새 보스 능력
		FPDGame G(31);
		G.Mode = EPDMode::W30;
		G.NewGame(true);
		G.bMuted = true;
		EPDBossForm F15, F20, F30;
		const auto S15 = G.BossWave(15, &F15), S20 = G.BossWave(20, &F20), S30 = G.BossWave(30, &F30);
		G.Mode = EPDMode::W60;
		const auto S30b = G.BossWave(30), S60 = G.BossWave(60);
		G.Mode = EPDMode::W100;
		EPDBossForm F100;
		const auto S100 = G.BossWave(100, &F100), S95 = G.BossWave(95), S60c = G.BossWave(60);
		const bool bFixed = F15 == EPDBossForm::Duo && S15.size() == 2 && F20 == EPDBossForm::Escort && S20[0].T == EPDEnemy::SplitBoss
			&& F30 == EPDBossForm::Final && S30[0].T == EPDEnemy::Bouncer && S30b[0].T == EPDEnemy::Phantom && S60[0].T == EPDEnemy::HighRoller && S60.size() == 3 && S60c[0].T == EPDEnemy::Warden
			&& F100 == EPDBossForm::Final && S100[0].T == EPDEnemy::House && S100.size() == 4 && S95.size() == 3 && (S95[0].Mut & MutRegen);
		bool bAll19 = true; // 100 모드 안에서 새 보스까지 모두 나온다
		for (EPDEnemy T : {EPDEnemy::Bluffer, EPDEnemy::BigBlind, EPDEnemy::Dealer, EPDEnemy::Counter, EPDEnemy::HouseRule, EPDEnemy::Twins, EPDEnemy::Breaker, EPDEnemy::Hexer})
		{
			bool bSeen = false;
			for (int W = 5; W <= 100; W += 5) for (const FPDBossSlot& Sl : G.BossWave(W)) bSeen = bSeen || Sl.T == T;
			bAll19 = bAll19 && bSeen;
		}
		G.Mode = EPDMode::Endless;
		const auto E205a = G.BossWave(205), E205b = G.BossWave(205);
		bool bSame = E205a.size() == E205b.size(), bMut = true;
		for (size_t I = 0; I < E205a.size() && I < E205b.size(); ++I) { bSame = bSame && E205a[I].T == E205b[I].T; bMut = bMut && E205a[I].Mut != 0; }
		// 웨이브 짜임: 30 모드 30웨이브 = 바운서 + 정예 넷, 15 = 둘 (블러퍼 분신 둘 더)
		G.Mode = EPDMode::W30;
		int Boss15 = 0, Decoys = 0;
		for (const FPDEnemy& E : G.PreviewWaveSpec(15)) { Boss15 += EnemyDef(E.Type).bBoss; Decoys += E.Type == EPDEnemy::Decoy; }
		int Elites = 0, Bouncers = 0;
		for (const FPDEnemy& E : G.PreviewWaveSpec(30)) { Elites += E.bElite; Bouncers += E.Type == EPDEnemy::Bouncer; }
		Expect(bFixed && bAll19 && bSame && bMut && Boss15 == 2 && Decoys == 2 && Elites == 4 && Bouncers == 1, "보스 차례 고정 · 짜임(짝 · 호위 · 최종) · 새 보스 모두 등장 · 무한 100 뒤 같은 씨앗이면 같은 뽑기 + 변이 (D-335)");
		// 딜러: 한꺼번에 최대 3장, 조커 · 와일드는 안 바꿈, 웨이브 끝나면 되돌림
		G.Enemies.clear(); G.Phase = EPDPhase::Wave; G.Towers.clear();
		for (int K = 0; K < 6; ++K) Put(G.Towers, K * 2, 1, C(2 + K, EPDSuit::Spade));
		FPDTower Jk; Jk.Card = {0, EPDSuit::Wild}; Jk.X = 0; Jk.Y = 3; Jk.Joker = (int)EPDJoker::Jack; G.Towers[CellKey(0, 3)] = Jk;
		G.DealSuits(2); G.DealSuits(2);
		int Changed = 0;
		for (const auto& KV : G.Towers) Changed += !KV.second.IsJoker() && KV.second.Card.Suit != EPDSuit::Spade;
		const bool bJokerSame = G.Towers.at(CellKey(0, 3)).Card.Suit == EPDSuit::Wild;
		G.RestoreSuits();
		int After = 0;
		for (const auto& KV : G.Towers) After += !KV.second.IsJoker() && KV.second.Card.Suit != EPDSuit::Spade;
		Expect(Changed == 3 && G.SuitSwaps.empty() && After == 0 && bJokerSame, "딜러: 무작위로 바꾸되 한꺼번에 최대 3장, 조커 빼고, 되돌림 (D-334)");
		// 카드 카운터 내성 · 빅 블라인드 판돈 · 쌍둥이 분노
		FPDEnemy Co = G.SpawnTestEnemy(EPDEnemy::Counter, 1000.f, 10);
		G.Enemies = {Co};
		for (int K = 0; K < 40; ++K) G.HitTestFrom(CellKey(2, 1), G.Enemies[0], 1.f);
		const float Res = G.Enemies[0].Res[(int)EPDSuit::Spade];
		FPDEnemy Bb = G.SpawnTestEnemy(EPDEnemy::BigBlind, 1.f, 10);
		Bb.Pot = 10.f;
		G.Enemies = {Bb};
		const int Xp0 = G.XpEarned;
		G.HitTest(G.Enemies[0], 1e9f);
		const int XpGot = G.XpEarned - Xp0;
		FPDEnemy T1 = G.SpawnTestEnemy(EPDEnemy::Twins, 10.f, 10), T2 = G.SpawnTestEnemy(EPDEnemy::Twins, 10.f, 10);
		T1.Link = T2.Id; T2.Link = T1.Id; T2.Hp = T2.MaxHp * 0.3f;
		G.Enemies = {T1, T2};
		G.HitTest(G.Enemies[0], 1e9f);
		Expect(std::fabs(Res - Boss::CounterMax) < 1e-3f && XpGot >= 20 && G.Enemies[1].bRage && G.Enemies[1].Hp > G.Enemies[1].MaxHp * 0.75f, "카드 카운터 내성 최대 60% · 빅 블라인드 판돈 ×2 · 쌍둥이 회복 + 분노 (D-334)");
	}
	{ // 조커가 든 족보는 합성 불가 (D-200, 예전 D-128 은 조커를 뺀 칸만 합성)
		FPDGame G(22);
		G.NewGame(true);
		G.bMuted = true;
		G.Phase = EPDPhase::Build;
		G.Towers.clear();
		Put(G.Towers, 3, 2, C(13, Di));
		G.Towers.at(CellKey(3, 2)).Joker = (int)EPDJoker::Jack;
		Put(G.Towers, 3, 3, C(3, Di));
		Put(G.Towers, 3, 4, C(8, Di));
		Put(G.Towers, 3, 5, C(11, Di));
		G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
		int Fl = -1;
		for (int I = 0; I < (int)G.Synergies.size(); ++I) if (G.Synergies[I].Kind == EPDHand::Flush) Fl = I;
		const bool bThree = Fl >= 0 && G.Synergies.size() == 1 && G.Synergies[Fl].Cells.size() == 3 && std::find(G.Synergies[Fl].Cells.begin(), G.Synergies[Fl].Cells.end(), CellKey(3, 2)) == G.Synergies[Fl].Cells.end();
		const bool bFuse = bThree && G.CanFuse(Fl) && G.Fuse(Fl, CellKey(3, 3));
		Expect(bThree && bFuse && G.Towers.size() == 2 && G.Towers.at(CellKey(3, 2)).IsJoker(), "조커 + ♦ 셋 = 조커 빼고 플러시3, 합성되고 조커는 남는다 (D-489)");
	}
	{ // 조커 처치 경험치 (D-125): 조커가 잡으면 +1 (보스 +15), 다른 타워가 잡으면 없음, 레벨업 사건
		FPDGame G(21);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		Put(G.Towers, 4, 4, C(9, He));
		FPDTower& J = G.Towers.at(CellKey(4, 4));
		J.Joker = (int)EPDJoker::Jack;
		Put(G.Towers, 6, 4, C(5, EPDSuit::Spade));
		G.Events.clear();
		FPDEnemy A = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 3);
		G.HitTestFrom(CellKey(4, 4), A, 1e6f);
		FPDEnemy B = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 3);
		G.HitTestFrom(CellKey(6, 4), B, 1e6f);
		const float Xp1 = G.Towers.at(CellKey(4, 4)).JXp;
		G.Towers.at(CellKey(4, 4)).JXp = JokerXpNeed(1) - 1.f;
		FPDEnemy Bs = G.SpawnTestEnemy(EPDEnemy::ShieldBoss, 10.f, 5);
		G.HitTestFrom(CellKey(4, 4), Bs, 1e6f);
		const FPDTower& J2 = G.Towers.at(CellKey(4, 4));
		bool bUp = false;
		for (const FPDEvent& E : G.Events) bUp = bUp || (E.Kind == FPDEvent::EKind::JokerXp && E.B == 1 && E.A == 2);
		Expect(Xp1 == JokerKillXp && J2.JLevel == 2 && std::fabs(J2.JXp - (JokerBossXp - 1.f)) < 1e-4f && bUp,
			"조커 처치 경험치: 일반 +" + std::to_string(JokerKillXp).substr(0, 4) + ", 보스 +" + std::to_string((int)JokerBossXp) + " → 레벨 2 (남은 경험치 " + std::to_string((int)J2.JXp) + "), 다른 타워 처치는 없음");
	}
	{ // 처치 경험치 주인은 쏠 때 기준 (#67): 발사체가 날아가는 사이 쏜 칸이 바뀌어도
		FPDGame G(23);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		G.Enemies.clear();
		G.Projectiles.clear();
		Put(G.Towers, 1, 1, C(9, He));
		Put(G.Towers, 7, 5, C(9, He));
		G.Towers.at(CellKey(7, 5)).Joker = (int)EPDJoker::Bell;
		auto Fly = [&](bool bJoker) {
			FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 3);
			E.Hp = 1.f;
			G.Enemies.push_back(E);
			FPDProjectile P;
			P.Target = E.Id;
			P.Dmg = 1e6f;
			P.Src = CellKey(1, 1);
			P.bJokerSrc = bJoker;
			P.Life = 10.f; // 다음 갱신에 바로 닿는다
			G.Projectiles.push_back(P);
		};
		// B: 카드 타워가 쏜 뒤 그 칸이 조커가 됨 → 카드가 잡은 것, 조커 경험치 없음
		Fly(false);
		G.Towers.at(CellKey(1, 1)).Joker = (int)EPDJoker::Jack;
		G.UpdateProjectilesForTest(0.01f);
		const bool bB = G.Enemies.back().bDead && G.Towers.at(CellKey(7, 5)).JXp == 0.f && G.Towers.at(CellKey(1, 1)).JXp == 0.f;
		// C: 조커가 쏜 뒤 그 조커가 사라짐 (상위 조커 재료 등) → 조커가 잡은 것, 남은 조커가 경험치
		Fly(true);
		G.Towers.erase(CellKey(1, 1));
		G.UpdateProjectilesForTest(0.01f);
		const bool bC = G.Enemies.back().bDead && std::fabs(G.Towers.at(CellKey(7, 5)).JXp - JokerKillXp) < 1e-4f;
		Expect(bB && bC, "처치 경험치는 쏠 때 조커였는지로: 쏜 뒤 칸이 조커가 돼도 없음, 쏜 조커가 사라져도 남은 조커가 받음 (#67)");
	}
	{ // 처치 경험치 공유 (D-497): 조커가 잡으면 판 위 모든 조커가 각자 다 받는다. 사건은 처치한 조커만 (다른 조커는 레벨이 오를 때만)
		FPDGame G(22);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		Put(G.Towers, 1, 1, C(9, He));
		Put(G.Towers, 7, 5, C(9, He));
		Put(G.Towers, 4, 3, C(5, EPDSuit::Spade));
		G.Towers.at(CellKey(1, 1)).Joker = (int)EPDJoker::Jack;
		G.Towers.at(CellKey(7, 5)).Joker = (int)EPDJoker::Bell;
		G.Events.clear();
		FPDEnemy A = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 3);
		G.HitTestFrom(CellKey(1, 1), A, 1e6f);
		int XpEv = 0;
		for (const FPDEvent& E : G.Events) XpEv += E.Kind == FPDEvent::EKind::JokerXp;
		const bool bBoth = std::fabs(G.Towers.at(CellKey(1, 1)).JXp - JokerKillXp) < 1e-4f && std::fabs(G.Towers.at(CellKey(7, 5)).JXp - JokerKillXp) < 1e-4f;
		FPDEnemy B = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 3);
		G.HitTestFrom(CellKey(4, 3), B, 1e6f);
		const bool bCard = std::fabs(G.Towers.at(CellKey(7, 5)).JXp - JokerKillXp) < 1e-4f;
		Expect(bBoth && XpEv == 1 && bCard && std::fabs(JokerKillXp - 2.f / 3.f) < 1e-6f && JokerBossXp == 10.f,
			"처치 경험치 공유 (D-497): 두 조커 모두 +2/3, 사건은 처치한 조커 하나, 카드가 잡으면 없음, 보스 10");
	}
	{ // 보스 13종 (D-112): 순서와 능력
		auto BossOf = [](int W) {
			FPDGame G(40);
			G.NewGame(true);
			G.bMuted = true;
			G.Wave = W - 1;
			G.Phase = EPDPhase::Build;
			G.StartWave();
			for (const auto& Ev : G.Events) if (Ev.Kind == FPDEvent::EKind::WaveStart && Ev.B == 1) return Ev.Enemy;
			return EPDEnemy::Count;
		};
		Expect(BossOf(5) == EPDEnemy::ShieldBoss && BossOf(20) == EPDEnemy::SplitBoss && BossOf(65) == EPDEnemy::HouseRule && BossOf(70) == EPDEnemy::Twins,
			"보스 차례 (D-335 고정): 5 방패 거인, 20 분열 군주(호위), 65 하우스 룰, 70 쌍둥이");
		auto Fresh = [](EPDEnemy T, float D) {
			FPDGame G(41);
			G.NewGame(true);
			G.bMuted = true;
			G.Towers.clear();
			G.Phase = EPDPhase::Wave;
			FPDEnemy E = G.SpawnTestEnemy(T, 1000.f, 20);
			E.D = D;
			G.Enemies = {E};
			return G;
		};
		{ // 돌진 기사: 4초마다 0.8초 동안 3배
			FPDGame G = Fresh(EPDEnemy::Charger, 0.5f);
			float D0 = G.Enemies[0].D; G.Update(0.05f); const float Step1 = G.Enemies[0].D - D0;
			for (int K = 0; K < 79; ++K) G.Update(0.05f);
			D0 = G.Enemies[0].D; G.Update(0.05f); const float Step2 = G.Enemies[0].D - D0;
			Expect(Step2 > Step1 * 2.5f, "돌진 기사: 4초 뒤 한 틱 이동 " + std::to_string(Step2 / Step1) + "배");
		}
		{ // 소환사: 3초마다 졸개 둘
			FPDGame G = Fresh(EPDEnemy::Summoner, 3.f);
			G.Enemies[0].AbilT = 2.99f;
			G.Update(0.05f);
			int Minis = 0;
			for (const FPDEnemy& E : G.Enemies) Minis += E.Type == EPDEnemy::Mini;
			Expect(Minis == 2, "소환사: 3초에 졸개 둘");
		}
		{ // 유령: 무적 1.2초
			FPDGame G = Fresh(EPDEnemy::Phantom, 1.f);
			G.Enemies[0].AbilT = 3.99f;
			G.Update(0.05f);
			const float Hp0 = G.Enemies[0].Hp;
			G.HitTest(G.Enemies[0], 100.f);
			const bool bImmune = G.Enemies[0].Hp == Hp0 && G.Enemies[0].Untargetable();
			for (int K = 0; K < 26; ++K) G.Update(0.05f);
			G.HitTest(G.Enemies[0], 100.f);
			Expect(bImmune && G.Enemies[0].Hp < Hp0, "유령: 무적인 동안 안 맞고, 1.2초 뒤엔 맞음");
		}
		{ // 군단장: 주변 적만
			FPDGame G = Fresh(EPDEnemy::Warlord, 3.f);
			FPDEnemy Near = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 20); Near.D = 4.f;
			// 먼 적: 길이 지그재그라 길로 6칸이어도 직선으론 가까울 수 있다. 오라는 직선거리라 직선으로 4칸 넘게 떨어진 자리를 고른다
			FPDEnemy Far = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 20); Far.D = 9.f;
			const FPDPoint WQ = G.Path.At(3.f);
			while (Far.D < G.Path.Length - 1.f && std::hypot(G.Path.At(Far.D).X - WQ.X, G.Path.At(Far.D).Y - WQ.Y) <= Boss::AuraRange + 1.5f) Far.D += 0.5f;
			G.Enemies.push_back(Near);
			G.Enemies.push_back(Far);
			Expect(G.InWarlordAura(G.Enemies[1]) && !G.InWarlordAura(G.Enemies[2]) && !G.InWarlordAura(G.Enemies[0]), "군단장: 1칸 옆 적은 오라 안, 6칸 떨어진 적과 자신은 밖");
		}
		{ // 분노 거인: 체력이 줄수록 빨라짐
			FPDGame G = Fresh(EPDEnemy::Berserker, 1.f);
			float D0 = G.Enemies[0].D; G.Update(0.05f); const float Full = G.Enemies[0].D - D0;
			G.Enemies[0].Hp = G.Enemies[0].MaxHp * 0.2f;
			D0 = G.Enemies[0].D; G.Update(0.05f); const float Low = G.Enemies[0].D - D0;
			Expect(std::fabs(Low / Full - 2.2f) < 0.05f, "분노 거인: 체력 20%면 2.2배 빠름 (" + std::to_string(Low / Full) + ")");
		}
		{ // 공간 도약자: 5초마다 2.5칸 앞으로
			FPDGame G = Fresh(EPDEnemy::Blinker, 1.f);
			G.Enemies[0].AbilT = 4.99f;
			G.Update(0.05f);
			Expect(G.Enemies[0].D > 3.4f, "공간 도약자: 순간이동 (D " + std::to_string(G.Enemies[0].D) + ")");
		}
		{ // 흡수자: 주변 적이 죽으면 그 적 최대 체력의 20% 회복
			FPDGame G = Fresh(EPDEnemy::Devourer, 3.f);
			G.Enemies[0].Hp = G.Enemies[0].MaxHp * 0.5f;
			FPDEnemy N = G.SpawnTestEnemy(EPDEnemy::Tank, 100.f, 20); N.D = 3.5f;
			G.Enemies.push_back(N);
			const float Hp0 = G.Enemies[0].Hp, Heal = G.Enemies[1].MaxHp * 0.2f;
			G.HitTest(G.Enemies[1], 1e9f);
			Expect(std::fabs(G.Enemies[0].Hp - (Hp0 + Heal)) < 1e-2f, "흡수자: 옆 적이 죽으면 회복");
		}
		{ // 수호자: 보호막이 먼저 받음
			FPDGame G = Fresh(EPDEnemy::Warden, 1.f);
			G.Enemies[0].AbilT = 5.99f;
			G.Update(0.05f);
			const float Sh = G.Enemies[0].Shield, Hp0 = G.Enemies[0].Hp;
			G.HitTest(G.Enemies[0], Sh * 0.5f);
			Expect(std::fabs(Sh - G.Enemies[0].MaxHp * 0.15f) < 1e-2f && G.Enemies[0].Hp == Hp0, "수호자: 최대 체력 15% 보호막이 피해를 먼저 받음");
		}
		{ // 저주술사·파괴자: 근처 타워
			FPDGame G = Fresh(EPDEnemy::Hexer, 3.f);
			const FPDPoint P = G.Path.At(3.f);
			Put(G.Towers, (int)P.X, (int)P.Y, C(9));
			Put(G.Towers, 0, 0, C(9));
			const FPDPoint Q0 = G.Path.At(3.f);
			const bool bFarOk = std::hypot(0.5f - Q0.X, 0.5f - Q0.Y) > Boss::HexRange;
			Expect(G.Hexed(G.Towers.at(CellKey((int)P.X, (int)P.Y))) && (!bFarOk || !G.Hexed(G.Towers.at(CellKey(0, 0)))), "저주술사: 3칸 안 타워만 약해짐");
			FPDGame B = Fresh(EPDEnemy::Breaker, 3.f);
			Put(B.Towers, (int)P.X, (int)P.Y, C(9));
			B.Enemies[0].AbilT = 5.99f;
			B.Update(0.05f);
			Expect(B.Towers.at(CellKey((int)P.X, (int)P.Y)).StunT > 2.f, "파괴자: 가까운 타워 기절");
		}
	}
	{ // 조커 재료: 레시피와 정확히 맞는 것만, 재료가 많은 레시피 먼저 (D-487)
		FPDGame G(16);
		G.NewGame(true);
		G.bMuted = true;
		G.Phase = EPDPhase::Build;
		G.Towers.clear();
		const int Row = FreeRow(G, 13);
		// 따로 선 합성 타워: 원 페어 둘 · 플러시 둘 · 포카드. 희비극(4개) 과 체셔(포카드 + 플러시, 2개) 가 플러시를 다툰다
		PutFused(G.Towers, 0, Row, C(4), EPDHand::Pair); PutFused(G.Towers, 2, Row, C(6, He), EPDHand::Pair);
		PutFused(G.Towers, 4, Row, C(8, Di), EPDHand::Flush); PutFused(G.Towers, 6, Row, C(10, Cl), EPDHand::Flush);
		PutFused(G.Towers, 8, Row, C(13), EPDHand::Four, 3);
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Gs = G.JokerGroups();
		const bool bFour = Gs.size() == 1 && Gs[0].Result == EPDJoker::Comedy && Gs[0].Cells.size() == 4;
		const bool bFused = bFour && G.JokerFuse(0);
		Expect(bFused && G.Towers.size() == 2 && G.JokerGroups().empty(), "원 페어 둘 · 플러시 둘 · 포카드: 희비극(재료 4) 먼저, 체셔는 플러시를 뺏겨 못 만듦");
		// 2개 레시피: 포카드 합성 타워 + 진짜 카드 플러시 4♦ 9♦ Q♦ → 체셔 (카드 4장 덤 ×1.4, 등급 ×3.5)
		G.Towers.clear();
		PutFused(G.Towers, 0, Row, C(13), EPDHand::Four, 3);
		Put(G.Towers, 3, Row, C(4, Di)); Put(G.Towers, 4, Row, C(9, Di)); Put(G.Towers, 5, Row, C(12, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto G2 = G.JokerGroups();
		const bool b2 = G2.size() == 1 && G2[0].Result == EPDJoker::Cheshire && G2[0].Cells.size() == 4 && G.JokerFuse(0);
		const FPDTower& Jc = G.Towers.begin()->second;
		Expect(b2 && G.Towers.size() == 1 && std::fabs(Jc.Mult - 4.f * JokerBonus * 3.5f * (1.f + JokerCardBonus * 4.f)) < 1e-4f,
			"포카드 + 플러시 → 체셔 (재료 2, 등급 ×3.5, 카드 4장 덤)");
		// 조커 덤 (D-492): 10, QA 갈래로 바꾸면 조커 위력이 그 비율로
		G.Towers.clear();
		PutFused(G.Towers, 0, Row, C(13), EPDHand::Four, 3);
		Put(G.Towers, 3, Row, C(4, Di)); Put(G.Towers, 4, Row, C(9, Di)); Put(G.Towers, 5, Row, C(12, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		G.JokerBonusK = 2.f;
		const bool b3 = G.JokerFuse(0) && G.Towers.size() == 1;
		Expect(JokerBonus == 10.f && b3 && std::fabs(G.Towers.begin()->second.Mult - 4.f * 2.f * 3.5f * (1.f + JokerCardBonus * 4.f)) < 1e-4f && std::fabs(G.JokerMult() - 2.f) < 1e-6f,
			"조커 덤 10 (D-492), JokerBonusK 갈래 2.0 이면 위력도 2.0 배로");
	}
	{ // 같은 조커끼리 합치기 (D-114), 조커는 먹이가 되지 않음
		FPDGame G(15);
		G.NewGame(true);
		G.bMuted = true;
		G.Phase = EPDPhase::Build;
		G.Towers.clear();
		const int Row = FreeRow(G, 5);
		auto MakeJ = [&](int X, EPDJoker K, float M, int Lv) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Row; T.Joker = (int)K; T.Mult = M; T.JLevel = Lv; G.Towers[CellKey(X, Row)] = T; };
		MakeJ(0, EPDJoker::Clown, 4.f, 3);
		MakeJ(1, EPDJoker::Clown, 6.f, 5);
		MakeJ(3, EPDJoker::Hatter, 4.f, 1); // 떨어져 있는 다른 조커
		G.Towers.at(CellKey(0, Row)).JXp = 7.f; G.Towers.at(CellKey(1, Row)).JXp = 7.f; // 합치면 경험치는 0 (D-500)
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Ms = G.JokerMerges();
		const auto Pw = Ms.size() == 1 ? G.PreviewPower(FPDGame::EPowerAction::JokerMerge, 0) : FPDGame::FPowerPreview{};
		const bool bMerged = Ms.size() == 1 && Ms[0].Cells.size() == 2 && G.MergeJokers(0);
		int Clowns = 0;
		float M = 0.f;
		int Lv = 0;
		float MXp = -1.f;
		for (const auto& KV : G.Towers) if (KV.second.Joker == (int)EPDJoker::Clown) { Clowns++; M = KV.second.Mult; Lv = KV.second.JLevel; MXp = KV.second.JXp; }
		Expect(bMerged && Clowns == 1 && std::fabs(M - 10.f * JokerMergeBonus) < 1e-4f && Lv == 5 && MXp == 0.f && G.Towers.size() == 2 && Pw.After > 0.f,
			"같은 광대 둘이 맞닿으면 하나로: 위력 (4+6) × 1.3, 레벨은 높은 쪽 5, 경험치는 0 부터 (D-500)");
		// 조커 옆 족보를 먹여도 조커는 사라지지 않는다 (조커는 족보에 안 든다, D-489)
		G.Towers.clear();
		MakeJ(0, EPDJoker::Clown, 4.f, 1);
		MakeJ(1, EPDJoker::Hatter, 4.f, 1);
		Put(G.Towers, 2, Row, C(9)); Put(G.Towers, 3, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		bool bFedJoker = false;
		for (const FPDFeed& Fd : G.FeedTargets()) { G.Feed(Fd.Syn); break; }
		int Jokers = 0;
		for (const auto& KV : G.Towers) Jokers += KV.second.IsJoker();
		bFedJoker = Jokers < 2;
		Expect(!bFedJoker, "조커는 먹이가 되지 않음 (먹이기 뒤에도 조커 둘 그대로)");
		// 판 아무 족보나 먹인다 (D-494, 예전 맞닿은 족보만 D-489): 한 칸 떨어진 원 페어도 먹일 수 있다
		G.Towers.clear();
		MakeJ(0, EPDJoker::Clown, 4.f, 1);
		Put(G.Towers, 2, Row, C(9)); Put(G.Towers, 3, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const auto FarFd = G.FeedTargets();
		const bool bFar = G.Synergies.size() == 1 && FarFd.size() == 1 && FarFd[0].Joker == CellKey(0, Row) && FarFd[0].Jokers == 1 && G.Feed(0);
		Expect(bFar && G.Towers.size() == 1 && G.Towers.at(CellKey(0, Row)).JXp + G.Towers.at(CellKey(0, Row)).JLevel > 1.f,
			"먹이기는 판 아무 족보나: 조커에 안 닿은 원 페어도 먹인다 (D-494)");
		// 조커가 없으면 먹이기 없음
		G.Towers.clear();
		Put(G.Towers, 2, Row, C(9)); Put(G.Towers, 3, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.size() == 1 && G.FeedTargets().empty() && !G.Feed(0) && G.Towers.size() == 2, "조커가 없으면 먹일 족보도 없다 (D-494)");
		// 조커 둘 (멀리, 레벨 다름): 둘 다 같은 경험치 + 각자 흡수, 사건도 조커마다 (D-494)
		G.Towers.clear();
		MakeJ(0, EPDJoker::Clown, 4.f, 3);
		MakeJ(Cols - 1, EPDJoker::Hatter, 9.f, 1);
		Put(G.Towers, 3, Row, C(9)); Put(G.Towers, 4, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		{
			const int JA = CellKey(0, Row), JB = CellKey(Cols - 1, Row);
			const auto Fd = G.FeedTargets();
			const float XA = G.Towers.at(JA).JXp, XB = G.Towers.at(JB).JXp, MA = G.Towers.at(JA).Mult, MB = G.Towers.at(JB).Mult;
			const int LA = G.Towers.at(JA).JLevel, LB = G.Towers.at(JB).JLevel;
			float Val = 0.f;
			for (int K : G.Synergies[0].Cells) { const FPDTower& T = G.Towers.at(K); Val += (3.f + T.Card.Power()) * T.Mult * T.TDmg * T.TRate; }
			const float GA = G.FeedGain(Val, G.Towers.at(JA)), GB = G.FeedGain(Val, G.Towers.at(JB));
			G.Events.clear();
			const bool bOk = Fd.size() == 1 && Fd[0].Joker == JA && Fd[0].Jokers == 2 && G.Feed(Fd[0].Syn);
			auto Total = [](int L, float X) { float S = X; for (int I = 1; I < L; ++I) S += JokerXpNeed(I); return S; };
			const FPDTower &A = G.Towers.at(JA), &B = G.Towers.at(JB);
			int NEv = 0;
			for (const FPDEvent& E : G.Events) NEv += E.Kind == FPDEvent::EKind::Feed && E.Lv == 2;
			Expect(bOk && G.Towers.size() == 2 && std::fabs(Total(A.JLevel, A.JXp) - Total(LA, XA) - Fd[0].Xp) < 1e-3f && std::fabs(Total(B.JLevel, B.JXp) - Total(LB, XB) - Fd[0].Xp) < 1e-3f
				&& GA > 0.f && GB > 0.f && std::fabs(A.Mult - (MA + GA)) < 1e-3f && std::fabs(B.Mult - (MB + GB)) < 1e-3f && NEv == 2,
				"조커 둘: 둘 다 같은 경험치 전량 + 각자 흡수 60%, 대표는 레벨 높은 쪽, 사건 둘 (D-494)");
		}
		// 먹보 · 먹이 사슬 부적도 조커마다 (D-494)
		G.Towers.clear();
		MakeJ(0, EPDJoker::Clown, 4.f, 1);
		MakeJ(Cols - 1, EPDJoker::Hatter, 4.f, 1);
		Put(G.Towers, 3, Row, C(2)); Put(G.Towers, 4, Row, C(2, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		G.Charms.push_back(EPDCharm::Feeder); G.Charms.push_back(EPDCharm::FoodChain);
		{
			const int JA = CellKey(0, Row), JB = CellKey(Cols - 1, Row);
			const auto Fd = G.FeedTargets();
			float Val = 0.f;
			for (int K : G.Synergies[0].Cells) { const FPDTower& T = G.Towers.at(K); Val += (3.f + T.Card.Power()) * T.Mult * T.TDmg * T.TRate; }
			const float MB = G.Towers.at(JB).Mult, GB = 0.9f * Val / (13.f * std::max(0.01f, G.Towers.at(JB).TDmg * G.Towers.at(JB).TRate));
			const float Xp = Fd.empty() ? 0.f : Fd[0].Xp * 1.5f;
			const bool bOk = Fd.size() == 1 && G.Feed(Fd[0].Syn);
			auto Total = [](const FPDTower& T) { float S = T.JXp; for (int I = 1; I < T.JLevel; ++I) S += JokerXpNeed(I); return S; };
			Expect(bOk && std::fabs(Total(G.Towers.at(JA)) - Xp) < 1e-3f && std::fabs(Total(G.Towers.at(JB)) - Xp) < 1e-3f && std::fabs(G.Towers.at(JB).Mult - (MB + GB)) < 1e-3f,
				"먹보 경험치 ×1.5 · 먹이 사슬 흡수 90% 가 조커마다 (D-494)");
		}
		G.Charms.clear();
	}
	{ // 부적 창은 5웨이브마다 (D-101): 그 사이 웨이브는 창 없이 5초 뒤 다음 웨이브
		FPDGame G(12);
		G.NewGame(true);
		G.bMuted = true;
		auto EndAt = [&](int W) { G.Wave = W; G.Phase = EPDPhase::Wave; G.Enemies.clear(); G.Update(0.f); }; // 새 판이라 남은 적(Pending)도 없음
		EndAt(3);
		const bool bNoWindow = G.Phase == EPDPhase::Build && G.Rewards.empty() && std::fabs(G.AutoStartT - AutoStartDelayFor(4)) < 1e-4f;
		EndAt(5);
		int NC = 0;
		for (const FPDReward& R : G.Rewards) NC += R.bCharm;
		Expect(bNoWindow && G.Phase == EPDPhase::Reward && G.Rewards.size() == 3 && NC >= 2, "3웨이브는 창 없이 (초반이라 12초) 뒤 다음, 5웨이브에 부적 창");
	}
	{ // 발사체: ♥ 미사일은 날아가서 닿을 때 피해, ♠ 레이저는 즉시
		FPDGame G(32);
		G.NewGame(true);
		G.bMuted = true;
		int JX = -1, JY = -1;
		float NearD = 0.f;
		NearPathCell(G, JX, JY, NearD);
		auto Try = [&](EPDSuit Su) {
			G.Towers.clear();
			G.Projectiles.clear();
			Put(G.Towers, JX, JY, C(9, Su));
			G.Synergies = ApplyEffects(G.Towers, 0);
			G.Phase = EPDPhase::Wave;
			G.Enemies.clear();
			FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 10000.f, 1);
			E.Speed = 0.f;
			E.D = NearD + 0.5f;
			G.Enemies.push_back(E);
			G.Update(0.01f);
			const bool bNow = G.Enemies[0].Hp < G.Enemies[0].MaxHp;
			const int Flying = (int)G.Projectiles.size();
			for (int K = 0; K < 60; ++K) G.Update(0.01f);
			return std::make_tuple(bNow, Flying, G.Enemies[0].Hp < G.Enemies[0].MaxHp);
		};
		auto [bNowL, FlyL, bLaterL] = Try(EPDSuit::Spade);
		auto [bNowM, FlyM, bLaterM] = Try(EPDSuit::Heart);
		auto [bNowB, FlyB, bLaterB] = Try(EPDSuit::Diamond);
		Expect(bNowL && FlyL == 0, "♠ 레이저: 즉시 맞음");
		Expect(!bNowM && FlyM == 1 && bLaterM, "♥ 미사일: 날아가서 맞음");
		Expect(!bNowB && FlyB == 1 && bLaterB, "♦ 탄환: 날아가서 맞음");
		Expect(ShotKind(G.Towers.begin()->second) == EPDShot::Bullet && ShotSpeed(EPDShot::Bullet) > ShotSpeed(EPDShot::Missile), "탄환이 미사일보다 빠름");
	}
	{ // 자동 시작: 보상을 고르면 5초(실제 시간, 속도와 상관없이) 뒤 다음 웨이브, 첫 웨이브는 직접
		FPDGame G(5);
		G.NewGame(true);
		G.bMuted = true;
		for (int K = 0; K < 200; ++K) G.Tick(0.05f);
		const bool bFirstWaits = G.Phase == EPDPhase::Build && G.Wave == 0;
		G.Wave = EasyWaves; // 초반 완화가 끝난 뒤 (다음 웨이브는 5초)
		G.Phase = EPDPhase::Reward;
		G.TakeReward(-1);
		G.Speed = 3;
		for (int K = 0; K < (int)(AutoStartDelay / 0.05f) - 2; ++K) G.Tick(0.05f); // 대기 시간 0.1초 전
		const bool bNotYet = G.Phase == EPDPhase::Build;
		for (int K = 0; K < 4; ++K) G.Tick(0.05f);   // 대기 시간 0.1초 뒤
		Expect(bFirstWaits && bNotYet && G.Phase == EPDPhase::Wave && G.Wave == EasyWaves + 1 && G.AutoStartT < 0.f, "자동 시작: 첫 웨이브는 기다림, 보상 뒤 대기 시간(1분)(속도 ×3 이어도)에 다음 웨이브");
		Expect(AutoStartDelayFor(2) == EasyAutoStartDelay && AutoStartDelayFor(EasyWaves + 1) == AutoStartDelay, "다음 웨이브까지 대기 시간 (D-185: 모두 1분)");
		Expect(std::fabs(EarlyHpK(1) - 0.55f) < 1e-4f && EarlyHpK(5) < 1.f && EarlyHpK(6) == 1.f, "초반 적 체력: 1웨이브 ×0.55, 6웨이브부터 ×1");
	}
	{ // 발사체 고르기: 조커 → 무늬 (족보는 발사체를 바꾸지 않는다, D-170)
		FPDTower T;
		T.Card = C(9, He);
		const EPDShot Base = ShotKind(T);
		T.Eff.Kind = (int)EPDHand::Pair;
		const EPDShot PairShot = ShotKind(T);
		T.Eff.Kind = (int)EPDHand::Flush;
		const EPDShot FlushShot = ShotKind(T);
		T.Eff.Kind = (int)EPDHand::RoyalFlush;
		const EPDShot RoyalShot = ShotKind(T);
		T.Joker = (int)EPDJoker::Jack;
		const EPDShot JackShot = ShotKind(T);
		T.Joker = (int)EPDJoker::Star;
		const EPDShot StarShot = ShotKind(T);
		Expect(Base == EPDShot::Missile && PairShot == EPDShot::Missile && FlushShot == EPDShot::Missile && RoyalShot == EPDShot::Missile && JackShot == EPDShot::Blade && StarShot == EPDShot::StarComet,
			"발사체: ♥ 는 족보와 상관없이 미사일, 조커는 저마다 (잭 칼날, 스타 별 혜성)");
		bool bAllOk = true;
		for (int K = 0; K < (int)EPDShot::Count; ++K)
		{
			const EPDShot S = (EPDShot)K;
			const bool bInstant = S == EPDShot::Laser || S == EPDShot::JokerLaser || S == EPDShot::Arc || S == EPDShot::Crown || S == EPDShot::Grin;
			bAllOk = bAllOk && (bInstant ? ShotSpeed(S) == 0.f : ShotSpeed(S) >= 8.f);
		}
		Expect(bAllOk, "즉시 발사체(레이저·번개·로열·체셔)만 속도 0, 나머지는 8칸/초 이상");
	}
	{ // 합성: 페어 → 레벨 2 타워 하나, 같은 레벨끼리만 족보, 레벨 2 페어 → 레벨 3
		FPDGame G(8);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bFused = G.Fuse(0);
		const FPDTower& F = G.Towers.begin()->second;
		Expect(bFused && G.Towers.size() == 1 && F.Level == 2 && std::fabs(F.Mult - 2.f * FuseBonus) < 1e-4f && F.TDmg == 1.5f && F.Card.Rank == 9 && G.Synergies.empty(),
			"합성: 9 페어 → 레벨 2 타워 하나 (위력 3배, 공격력 특성 ×1.5)");
		// 레벨 2 옆에 레벨 1 같은 숫자 → 족보 아님
		const int FX0 = F.X;
		Put(G.Towers, FX0 + 1, Row, C(9, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.empty(), "레벨이 다르면 족보가 안 됨");
		// 레벨 1 두 장을 또 합쳐 레벨 2 를 만들고, 레벨 2 둘이 붙으면 페어 → 레벨 3
		Put(G.Towers, FX0 + 2, Row, C(9, Di));
		Put(G.Towers, FX0 + 1, Row, C(9, Cl));
		G.Synergies = ApplyEffects(G.Towers, 0);
		int Lv1Pair = -1;
		for (int I = 0; I < (int)G.Synergies.size(); ++I) if (G.Towers[G.Synergies[I].Cells[0]].Level == 1) Lv1Pair = I;
		G.Fuse(Lv1Pair);
		int Lv2 = 0;
		for (auto& KV : G.Towers) Lv2 += KV.second.Level == 2;
		bool bAdj = false;
		G.Synergies = ApplyEffects(G.Towers, 0);
		for (auto& S2 : G.Synergies) bAdj = bAdj || (G.Towers[S2.Cells[0]].Level == 2 && S2.Kind == EPDHand::Pair);
		const bool bFused3 = bAdj && G.Fuse(0);
		int Lv3 = 0;
		for (auto& KV : G.Towers) Lv3 += KV.second.Level == 3;
		Expect(Lv2 == 2 && bFused3 && Lv3 == 1 && G.Towers.size() == 1, "레벨 2 페어를 합성하면 레벨 3");
	}
	{ // 무한 진행: 15웨이브 뒤에도 계속, 보스가 돌아오고, 덱이 비면 새 덱
		FPDGame G(10);
		G.NewGame(true);
		G.bMuted = true;
		G.Wave = 15;
		G.Phase = EPDPhase::Wave;
		G.Enemies.clear();
		G.Update(0.f); // 적이 없으니 웨이브 끝 → 보상
		const bool bNoWin = G.Phase == EPDPhase::Reward;
		G.Deck.clear();
		G.Hand.clear();
		G.TakeReward(-1);
		G.Phase = EPDPhase::Build;
		G.StartWave();
		bool bBoss20 = false;
		G.Wave = 19;
		G.Phase = EPDPhase::Build;
		G.StartWave();
		for (const auto& Ev : G.Events) if (Ev.Kind == FPDEvent::EKind::WaveStart && Ev.A == 20) bBoss20 = Ev.B == 1 && Ev.Enemy == EPDEnemy::SplitBoss;
		Expect(bNoWin && bBoss20, "15웨이브 뒤에도 계속, 20웨이브 보스 분열 군주 (D-335)");
		FPDGame G2(11);
		G2.NewGame(true);
		G2.Deck.clear();
		G2.Hand.clear();
		G2.Phase = EPDPhase::Reward;
		G2.Rewards = {{false, (int)EPDInstant::Draw2}};
		G2.TakeReward(0);
		Expect(G2.Hand.size() == 2 && G2.DeckRefills == 1 && G2.Deck.size() == 50, "덱이 비면 새 덱 한 벌");
	}
	{ // 조커: 대장(지휘)과 레시피 (D-115)
		using H = EPDHand;
		bool bMap = true;
		std::set<int> Seen;
		for (int K = 0; K < (int)H::Count; ++K) { bMap = bMap && CaptainHand(CaptainOf((H)K)) == (H)K; Seen.insert((int)CaptainOf((H)K)); }
		Expect(bMap && Seen.size() == 10, "족보 10개 ↔ 대장 조커 10개 (한 쌍씩)");
		// 기획 25절 표 그대로 (D-487), 할리퀸은 투 페어 (D-490)
		const std::vector<H> Want[(int)EPDJoker::Count] = {
			{H::Five, H::Pair},                         // 광대
			{H::StraightFlush, H::Pair},                // 모자 장수
			{H::TwoPair, H::Pair, H::Flush, H::Straight}, // 웃는 가면
			{H::Pair, H::Pair, H::Flush, H::Flush},     // 희비극
			{H::Flush, H::Flush, H::Flush, H::TwoPair}, // 할리퀸 (D-490)
			{H::Straight, H::Pair, H::Pair, H::Flush},  // 지휘봉
			{H::Triple, H::Pair, H::Flush},             // 잭
			{H::Four, H::Flush},                        // 체셔
			{H::FullHouse, H::Flush, H::Straight},      // 방울
			{H::RoyalFlush, H::Pair},                   // 스타
		};
		bool bTable = true, bRecipes = true, bCaptainIn = true, bSize = true, bCheap = true, bOneStraight = true, bTier = true;
		std::set<std::vector<int>> Uniq;
		for (int J = 0; J < (int)EPDJoker::Count; ++J)
		{
			std::vector<H> R = JokerRecipe((EPDJoker)J);
			std::vector<H> W = Want[J];
			std::vector<H> Rs = R;
			std::sort(Rs.begin(), Rs.end());
			std::sort(W.begin(), W.end());
			bTable = bTable && Rs == W;
			std::vector<H> Rev(R.rbegin(), R.rend());
			bRecipes = bRecipes && MatchJokerRecipe(Rev) == (EPDJoker)J; // 순서 무관
			const H Cap = CaptainHand((EPDJoker)J);
			bCaptainIn = bCaptainIn && std::find(R.begin(), R.end(), Cap) != R.end();
			const bool bCommon = Cap == H::Pair || Cap == H::Flush || Cap == H::Straight || Cap == H::TwoPair;
			const bool bMid = Cap == H::Triple || Cap == H::FullHouse;
			const size_t Need = bCommon ? 4 : bMid ? 3 : 2;
			bSize = bSize && R.size() == Need;
			const float WantTier = J == (int)EPDJoker::Hatter ? HatterTier : J == (int)EPDJoker::Star ? StarTier : Need == 4 ? 1.6f : Need == 3 ? 2.5f : 3.5f;
			bTier = bTier && std::fabs(JokerTier((EPDJoker)J) - WantTier) < 1e-6f;
			std::vector<H> Rest = R;
			Rest.erase(std::find(Rest.begin(), Rest.end(), Cap));
			for (H X : Rest) bCheap = bCheap && (X == H::Pair || X == H::Flush || X == H::Straight || X == H::TwoPair);
			bOneStraight = bOneStraight && std::count(R.begin(), R.end(), H::Straight) <= 1;
			std::vector<int> V;
			for (H X : Rs) V.push_back((int)X);
			Uniq.insert(V);
		}
		Expect(bTable, "조커 레시피가 기획 25절 표와 같음 (D-487, 할리퀸 D-490)");
		Expect(bRecipes && bCaptainIn && Uniq.size() == 10, "조커 레시피 10개: 서로 다르고, 순서 무관, 자기 대장 족보가 들어 있음 (D-115)");
		Expect(bSize && bCheap && bOneStraight, "재료 수 = 대장 난이도 (흔함 4 · 중간 3 · 드묾 2), 짝 재료는 흔한 족보만, 스트레이트는 최대 하나 (D-487)");
		Expect(bTier && JokerTier(EPDJoker::Comedy) == 1.6f && JokerTier(EPDJoker::Bell) == 2.5f && JokerTier(EPDJoker::Cheshire) == 3.5f && JokerTier(EPDJoker::Hatter) == 5.f && JokerTier(EPDJoker::Star) == 6.f, "조커 등급 배율: 재료 4개 ×1.6, 3개 ×2.5, 2개 ×3.5, 모자 장수 ×5 · 스타 ×6 (D-490)");
		Expect(MatchJokerRecipe({H::Flush, H::Flush, H::Flush, H::TwoPair}) == EPDJoker::Harlequin && MatchJokerRecipe({H::Flush, H::Flush, H::Flush, H::Pair}) == EPDJoker::Count, "할리퀸: 플러시 셋 + 투 페어, 옛 레시피(플러시 셋 + 원 페어)는 안 됨 (D-490)");
		Expect(MatchJokerRecipe({H::Pair, H::Pair, H::Flush}) == EPDJoker::Count && MatchJokerRecipe({H::Pair, H::Pair}) == EPDJoker::Count &&
			MatchJokerRecipe({H::Straight, H::Straight, H::Straight}) == EPDJoker::Count && MatchJokerRecipe({H::Four, H::Straight}) == EPDJoker::Count &&
			MatchJokerRecipe({H::Flush, H::Pair, H::Pair, H::Straight}) == EPDJoker::Baton && MatchJokerRecipe({H::Flush, H::Four}) == EPDJoker::Cheshire &&
			MatchJokerRecipe({H::Pair, H::RoyalFlush}) == EPDJoker::Star && MatchJokerRecipe({H::Pair, H::Pair, H::Flush, H::Flush, H::Pair}) == EPDJoker::Count,
			"레시피와 정확히 맞아야 조커: 옛 레시피(스트레이트 셋 · 포카드 + 스트레이트)와 덜 고른 · 더 고른 것은 안 됨");
	}
	{ // 모자 장수 오라 · 스타 위력 (D-490)
		FPDTowerMap M;
		auto PutJ = [&](int X, int Y, EPDJoker K, int Lv) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Y; T.Joker = (int)K; T.JLevel = Lv; T.Mult = 4.f; M[CellKey(X, Y)] = T; };
		PutJ(1, 1, EPDJoker::Hatter, 1);
		Put(M, 2, 1, C(5)); Put(M, 3, 1, C(9, EPDSuit::Club)); // 주변 8칸 / 2칸 떨어짐
		ApplyEffects(M);
		const FPDEffects& Near = M.at(CellKey(2, 1)).Eff;
		const FPDEffects& Far = M.at(CellKey(3, 1)).Eff;
		const bool bBase = std::fabs(Near.Dmg - 1.5f) < 1e-4f && std::fabs(Near.Rate - 1.2f) < 1e-4f && std::fabs(Near.Range - 1.2f) < 1e-4f && std::fabs(Far.Dmg - 1.f) < 1e-4f;
		M.at(CellKey(1, 1)).JLevel = JokerUpLevel; // 강화: 24칸 · ×1.8 · ×1.3 · ×1.2
		ApplyEffects(M);
		const bool bUp = std::fabs(M.at(CellKey(2, 1)).Eff.Dmg - 1.8f) < 1e-4f && std::fabs(M.at(CellKey(2, 1)).Eff.Rate - 1.3f) < 1e-4f && std::fabs(M.at(CellKey(3, 1)).Eff.Dmg - 1.8f) < 1e-4f && std::fabs(M.at(CellKey(3, 1)).Eff.Range - 1.2f) < 1e-4f;
		Expect(bBase && bUp, "모자 장수 오라: 주변 8칸 공격력 ×1.5 · 연사 ×1.2 · 사거리 ×1.2, 강화 24칸 ×1.8 · ×1.3 · ×1.2 (D-490)");
		FPDTower St, Co;
		St.Card = Co.Card = {0, EPDSuit::Wild};
		St.Joker = (int)EPDJoker::Star; Co.Joker = (int)EPDJoker::Comedy;
		St.Mult = Co.Mult = 4.f;
		const float R1 = TowerStats(St).Dmg / TowerStats(Co).Dmg;
		St.JLevel = Co.JLevel = JokerUpLevel;
		const float R2 = TowerStats(St).Dmg / TowerStats(Co).Dmg;
		Expect(std::fabs(R1 - 0.8f) < 1e-4f && std::fabs(R2 - 1.2f) < 1e-4f, "스타 위력 ×0.8, 강화 ×1.2 (D-490)");
	}
	{ // 행운의 한 장 (D-490)
		FPDGame G(31);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		const int Row = FreeRow(G, 7);
		const EPDSuit Hs = EPDSuit::Heart;
		for (int I = 0; I < 4; ++I) Put(G.Towers, 1 + I, Row, C(9 + I, Hs)); // ♥9 10 J Q
		const FPDCard W8{8, Hs}, WK{13, Hs};
		auto Has = [](const FPDLucky& L, FPDCard Cd) { for (int I = 0; I < L.NWant; ++I) if (L.Want[I] == Cd) return true; return false; };
		FPDLucky L = G.LuckyLine();
		Expect(L.Len == 4 && L.NWant == 2 && Has(L, W8) && Has(L, WK) && std::fabs(L.P() - LuckyP4) < 1e-6f, "4장 줄 ♥9 10 J Q: ♥8 · ♥K 를 기다림, 확률 40%");
		// 웨이브에 받는 카드 중 한 장만, 확률은 40% 근처 (씨앗 고정)
		const std::vector<FPDCard> Deck0 = G.Deck;
		int Hits = 0, Many = 0;
		for (int T = 0; T < 1000; ++T)
		{
			G.Deck = Deck0;
			G.Hand = {C(2, EPDSuit::Club), C(3, EPDSuit::Club), C(4, EPDSuit::Club)};
			const int H0 = G.LuckyHits;
			G.LuckyDraw(0, 3);
			int N = 0;
			for (const FPDCard& Cd : G.Hand) N += Cd == W8 || Cd == WK;
			Hits += G.LuckyHits - H0;
			Many += N > 1;
			if (N == 1 && (G.Deck.size() != Deck0.size() || std::find(G.Deck.begin(), G.Deck.end(), C(4, EPDSuit::Club)) == G.Deck.end())) Many += 1000; // 바뀐 카드는 덱으로
		}
		Expect(Hits >= 340 && Hits <= 460 && Many == 0, "행운의 한 장: 4장 줄이면 40% 로 한 장만 온다, 원래 카드는 덱으로 (" + std::to_string(Hits) + "/1000)");
		// 덱에 없으면 안 온다
		G.Deck = Deck0;
		G.Deck.erase(std::remove_if(G.Deck.begin(), G.Deck.end(), [&](const FPDCard& Cd) { return Cd == W8 || Cd == WK; }), G.Deck.end());
		bool bNone = G.LuckyLine().NWant == 0;
		for (int T = 0; T < 50; ++T) { G.Hand = {C(2, EPDSuit::Club)}; G.LuckyDraw(0, 1); bNone = bNone && G.Hand[0] == C(2, EPDSuit::Club); }
		Expect(bNone, "줄 끝 카드가 덱에 없으면 행운의 한 장 없음");
		G.Deck = Deck0;
		// 끝이 막히면 (조커 · 길 · 판 밖 = 빈 칸 아님) 그 끝은 기다리지 않는다
		const FPDLucky OnlyLow = FindLuckyLine(G.Towers, [&](int K) { return K == CellKey(0, Row); });
		std::vector<int> Blk;
		for (const auto& D : {std::array<int, 2>{1, 0}, std::array<int, 2>{-1, 0}, std::array<int, 2>{0, 1}, std::array<int, 2>{0, -1}}) { const int NX = 4 + D[0], NY = Row + D[1]; if (G.Buildable(NX, NY) && !G.Towers.count(CellKey(NX, NY))) Blk.push_back(CellKey(NX, NY)); }
		FPDGame J = G;
		for (int K : Blk) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = K % Cols; T.Y = K / Cols; T.Joker = (int)EPDJoker::Comedy; J.Towers[K] = T; }
		const FPDLucky JL = J.LuckyLine();
		Expect(OnlyLow.NWant == 1 && OnlyLow.Want[0] == W8 && JL.NWant == 1 && JL.Want[0] == W8, "끝 옆이 막히면 (조커 · 길 · 판 밖) 그 끝 카드는 안 기다림");
		// 3장 줄은 15%, 5장(이미 스트레이트 플러시)의 일부는 줄로 안 친다, 와일드는 가운데 숫자 하나
		FPDGame G3 = G;
		G3.Towers.erase(CellKey(4, Row)); // ♥9 10 J
		const FPDLucky L3 = G3.LuckyLine();
		FPDGame G5 = G;
		Put(G5.Towers, 5, Row, C(13, Hs)); // ♥9 10 J Q K
		FPDGame Gw = G3;
		Put(Gw.Towers, 2, Row, FPDCard{0, EPDSuit::Wild}); // ♥9 W J
		const FPDLucky Lw = Gw.LuckyLine();
		Expect(L3.Len == 3 && std::fabs(L3.P() - LuckyP3) < 1e-6f && G5.LuckyLine().Len == 0 && Lw.Len == 3 && Has(Lw, W8) && Has(Lw, FPDCard{12, Hs}),
			"3장 줄 15%, 이미 5장이면 없음, 와일드는 가운데 빈 숫자 하나를 채움");
		// 같은 레벨만, 무늬가 다르면 줄이 아님
		FPDGame Gl = G3;
		Gl.Towers.at(CellKey(2, Row)).Level = 2;
		FPDGame Gs = G3;
		Gs.Towers.at(CellKey(2, Row)).Card.Suit = EPDSuit::Spade;
		Expect(Gl.LuckyLine().Len == 0 && Gs.LuckyLine().Len == 0, "레벨이 섞이거나 무늬가 다르면 줄이 아님");
		// 보충 보상(2장)에는 안 붙는다
		FPDGame Gr = G;
		Gr.LuckyHits = 0;
		int RewardHits = 0;
		for (int T = 0; T < 40; ++T)
		{
			Gr.Phase = EPDPhase::Reward;
			Gr.Hand.clear();
			Gr.Rewards = {{false, (int)EPDInstant::Draw2}};
			Gr.TakeReward(0);
			RewardHits += Gr.LuckyHits;
		}
		Expect(RewardHits == 0, "보충 보상(2장)에는 행운의 한 장이 안 붙는다");
		// 끈 판 (QA 비교용)
		FPDGame Go = G;
		Go.LuckyHits = 0;
		Go.bLucky = false;
		for (int T = 0; T < 50; ++T) { Go.Deck = Deck0; Go.Hand = {C(2, EPDSuit::Club)}; Go.LuckyDraw(0, 1); }
		Expect(Go.LuckyHits == 0, "bLucky = false 면 행운의 한 장 없음");
	}
	{ // 조커 합성: 연쇄된 페어 셋 → 광대, 합성 타워 + 족보 둘 → 모자 장수
		FPDGame G(12);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		const int Row = FreeRow(G, 13);
		// 스트레이트 · 원 페어 둘 · 플러시 (진짜 카드 10장) = 지휘봉 (D-487)
		PutBaton(G.Towers, Row);
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Groups = G.JokerGroups();
		const bool bGroup = Groups.size() == 1 && Groups[0].Result == EPDJoker::Baton && Groups[0].Cells.size() == 10;
		const bool bFused = G.JokerFuse(0);
		const FPDTower& J = G.Towers.begin()->second;
		Expect(bGroup && bFused && G.Towers.size() == 1 && J.IsJoker() && J.Joker == (int)EPDJoker::Baton && std::fabs(J.Mult - 10.f * JokerBonus * 1.6f * (1.f + JokerCardBonus * 10.f)) < 1e-4f && (G.Discovered & (1u << (int)EPDJoker::Baton)),
			"스트레이트 + 원 페어 둘 + 플러시 → 지휘봉 (위력 10 × 조커 덤 × 등급 ×1.6 × 카드 10장 덤), 도감에 기록");
		// 족보 먹이기 (D-098, D-494): 족보를 먹이면 경험치(족보 점수 × 평균 위력) → 레벨 업, 먹인 칸은 사라짐
		{
			const int JKey = G.Towers.begin()->first, JX = JKey % Cols;
			const float D0 = TowerStats(G.Towers.at(JKey)).Dmg; // 족보에 안 든 조커 혼자
			Put(G.Towers, JX + 1, Row, C(9)); Put(G.Towers, JX + 2, Row, C(9, He)); // 조커 옆 원 페어 (조커는 족보에 안 들어 트리플이 아니다, D-489)
			G.Synergies = ApplyEffects(G.Towers, 0);
			Expect(G.Synergies.size() == 1 && G.Synergies[0].Kind == EPDHand::Pair && G.Synergies[0].Cells.size() == 2, "조커 옆 9 9 는 원 페어 (트리플 아님, D-489)");
			auto Fd = G.FeedTargets();
			const int SV = Fd.empty() ? 0 : G.Synergies[Fd[0].Syn].Value;
			const float M0 = G.Towers.at(JKey).Mult, Gain = Fd.empty() ? 0.f : Fd[0].Gain; // 흡수 (D-191)
			const bool bOk = Fd.size() == 1 && Fd[0].Joker == JKey && std::fabs(Fd[0].Xp - SV) < 1e-4f && G.Feed(Fd[0].Syn);
			int L = 1;
			float X = (float)SV;
			while (X >= JokerXpNeed(L)) { X -= JokerXpNeed(L); ++L; }
			const FPDTower& Jt = G.Towers.at(JKey);
			Expect(bOk && G.Towers.size() == 1 && L >= 2 && Jt.JLevel == L && std::fabs(Jt.JXp - X) < 1e-3f && Gain > 0.f && std::fabs(Jt.Mult - (M0 + Gain)) < 1e-3f && std::fabs(TowerStats(Jt).Dmg / D0 - (M0 + Gain) / M0 * (1.f + JokerLevelDmg * (L - 1))) < 1e-3f,
				"조커 옆 족보 먹이기: 경험치만큼 레벨 업 (레벨마다 위력 +15%) + 먹인 위력 60% 흡수 (D-191), 먹인 칸은 사라짐");
			G.Towers.at(JKey).JLevel = JokerUpLevel - 1;
			G.Towers.at(JKey).JXp = JokerXpNeed(JokerUpLevel - 1) - 1.f;
			Put(G.Towers, JX + 1, Row, C(4)); Put(G.Towers, JX + 2, Row, C(4, He));
			G.Synergies = ApplyEffects(G.Towers, 0);
			auto Fd2 = G.FeedTargets();
			Expect(!Fd2.empty() && G.Feed(Fd2[0].Syn) && G.Towers.at(JKey).JLevel >= JokerUpLevel && G.Mods.Up(G.Towers.at(JKey)), "레벨 10 이 되면 강화 능력");
			// 원 페어 양쪽에 조커 둘 (오른쪽이 레벨 낮음): 고르는 단계 없이 둘 다 먹는다 (D-494, 예전 고른 조커 D-213)
			const int J2 = CellKey(JX + 3, Row);
			FPDTower Jk2 = G.Towers.at(JKey);
			Jk2.X = JX + 3;
			Jk2.JLevel = 1;
			Jk2.JXp = 0.f;
			G.Towers[J2] = Jk2;
			Put(G.Towers, JX + 1, Row, C(6), 2); Put(G.Towers, JX + 2, Row, C(6, He), 2); // 레벨 2 원 페어
			G.Synergies = ApplyEffects(G.Towers, 0);
			int PairSyn = -1;
			for (int I = 0; I < (int)G.Synergies.size(); ++I)
			{
				const std::vector<int>& Cs = G.Synergies[I].Cells;
				if (std::find(Cs.begin(), Cs.end(), CellKey(JX + 1, Row)) != Cs.end()) PairSyn = I;
			}
			bool bRepHigh = false;
			for (const FPDFeed& F : G.FeedTargets()) if (F.Syn == PairSyn) bRepHigh = F.Joker == JKey && F.Jokers == 2;
			const float X1 = G.Towers.at(JKey).JXp;
			const int L1 = G.Towers.at(JKey).JLevel;
			const bool bFed = PairSyn >= 0 && G.Feed(PairSyn);
			Expect(bRepHigh && bFed && G.Towers.at(J2).JXp + G.Towers.at(J2).JLevel > 1.f && (G.Towers.at(JKey).JLevel > L1 || G.Towers.at(JKey).JXp > X1) && !G.Towers.count(CellKey(JX + 1, Row)),
				"족보 양쪽에 조커 둘: 대표는 레벨 높은 쪽, 둘 다 경험치를 받는다 (D-494)");
		}
		// 조커는 족보에 안 든다 (D-489): 레벨 2 타워 옆에 붙어도 족보 없음
		G.Towers.clear();
		FPDTower Jk;
		Jk.Card = {0, EPDSuit::Wild};
		Jk.X = 0;
		Jk.Y = Row;
		Jk.Joker = (int)EPDJoker::Clown;
		G.Towers[CellKey(0, Row)] = Jk;
		Put(G.Towers, 1, Row, C(9), 2);
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.empty(), "조커 + 레벨 2 타워 하나 = 족보 없음 (D-489, 예전엔 와일드로 원 페어)");
		// 포카드를 기억하는 합성 타워(따로 선 레벨 3) + 플러시(4♦ 9♦ Q♦) → 체셔 (D-487)
		G.Towers.clear();
		PutFused(G.Towers, 0, Row, C(13), EPDHand::Four, 3);
		Put(G.Towers, 1, Row, C(4, Di)); Put(G.Towers, 2, Row, C(9, Di)); Put(G.Towers, 3, Row, C(12, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Cheshire && Groups[0].FusedCells.size() == 1, "포카드를 기억한 합성 타워 + 플러시 → 체셔 (재료 2)");
		// 따로 선 합성 타워 셋: 원 페어 4♠ · 로열 A♦ · 스트레이트 플러시 3♥ 한 줄. 스타(로열 + 원 페어)와 모자 장수(스트플 + 원 페어)가 원 페어를 다툰다
		G.Towers.clear();
		PutFused(G.Towers, 0, Row, C(4), EPDHand::Pair);
		PutFused(G.Towers, 1, Row, C(14, Di), EPDHand::RoyalFlush);
		PutFused(G.Towers, 2, Row, C(3, He), EPDHand::StraightFlush);
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Hatter && Groups[0].Cells.size() == 2, "합성 타워 원 페어·로열·스트플: 원 페어 하나를 두 조커가 노리면 낮은 족보 쪽(모자 장수) 하나만");
		// 조커 만들기 (D-195): 멀리 떨어진 재료를 직접 고른다
		G.Towers.clear();
		PutFused(G.Towers, 0, Row, C(4), EPDHand::Pair);
		PutFused(G.Towers, 7, Row, C(14, Di), EPDHand::RoyalFlush);
		PutFused(G.Towers, 12, Row, C(3, He), EPDHand::StraightFlush);
		G.Synergies = ApplyEffects(G.Towers, 0);
		{
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			int NP = -1, NR = -1, NS = -1;
			for (int I = 0; I < (int)Ns.size(); ++I) { if (Ns[I].Kind == EPDHand::Pair) NP = I; if (Ns[I].Kind == EPDHand::RoyalFlush) NR = I; if (Ns[I].Kind == EPDHand::StraightFlush) NS = I; }
			const bool bNodes = Ns.size() == 3 && NP >= 0 && NR >= 0 && NS >= 0;
			const bool bTwo = bNodes && G.JokerCraftResult({NR, NP}) == EPDJoker::Star;
			const bool bBad = bNodes && G.JokerCraftResult({NP, NS}) == EPDJoker::Hatter && G.JokerCraftResult({NR, NS}) == EPDJoker::Count && G.JokerCraftResult({NP, NP}) == EPDJoker::Count &&
				G.JokerCraftResult({NP}) == EPDJoker::Count && G.JokerCraftResult({NP, NR, NS}) == EPDJoker::Count;
			const bool bOutside = bNodes && !G.JokerCraft({NP, NR}, CellKey(3, Row));
			const bool bMade = bNodes && G.JokerCraft({NP, NR}, CellKey(7, Row));
			Expect(bTwo && bBad && bOutside && bMade && G.Towers.size() == 2 && G.Towers.at(CellKey(7, Row)).Joker == (int)EPDJoker::Star,
				"조커 만들기: 떨어진 원 페어·로열을 골라 스타, 옛 레시피(로열 + 스트플)·더 고른 것·재료 밖 칸·같은 재료 두 번은 거절");
		}
		{ // 판매 (D-221): 카드 B, 합성 타워 B × 레벨², 조커 B × 10 (B = 2 + 웨이브/5)
			G.Towers.clear();
			G.Wave = 12;
			Put(G.Towers, 0, Row, C(7));
			Put(G.Towers, 2, Row, C(7), 3);
			FPDTower SellJk;
			SellJk.Card = {0, EPDSuit::Wild}; SellJk.X = 4; SellJk.Y = Row; SellJk.Joker = (int)EPDJoker::Clown;
			G.Towers[CellKey(4, Row)] = SellJk;
			G.Synergies = ApplyEffects(G.Towers, 0);
			const int B = 2 + 12 / 5, G0 = G.XpEarned;
			const bool bPrice = G.SellPrice(CellKey(0, Row)) == B && G.SellPrice(CellKey(2, Row)) == B * 9 && G.SellPrice(CellKey(4, Row)) == B * 10 && G.SellPrice(CellKey(9, Row)) == 0;
			const bool bSold = G.SellTower(CellKey(2, Row)) && !G.Towers.count(CellKey(2, Row)) && G.XpEarned == G0 + B * 9 && !G.SellTower(CellKey(2, Row));
			Expect(bPrice && bSold, "판매: 카드 4, Lv3 합성 36, 조커 40 골드 (웨이브 12), 판 타워만 팔림");
		}
		{ // 잠금 해제 (D-239): 잠긴 조커는 못 만든다, 새 묘약 효과
			G.Towers.clear();
			PutFused(G.Towers, 0, Row, C(14, Di), EPDHand::RoyalFlush);
			PutFused(G.Towers, 4, Row, C(5, Cl), EPDHand::Pair);
			G.Synergies = ApplyEffects(G.Towers, 0);
			const uint32_t Keep = G.JokerUnlocked;
			G.JokerUnlocked = JokerStarterMask;
			const bool bLocked = G.JokerGroups().empty() && G.BestJokerPicks(EPDJoker::Star).empty();
			G.JokerUnlocked |= 1u << (int)EPDJoker::Star;
			const bool bOpen = !G.JokerGroups().empty() && G.BestJokerPicks(EPDJoker::Star).size() == 2;
			G.JokerUnlocked = Keep;
			FPDTower T2; T2.Card = C(2); FPDTower T7; T7.Card = C(7);
			const FPDCharmBits Rebel = FPDCharmBits(1) << (int)EPDCharm::TwoRebel;
			const bool bRebel = std::fabs(TowerStats(T2, Rebel).Dmg / TowerStats(T2).Dmg - 3.f) < 1e-3f && std::fabs(TowerStats(T7, Rebel).Dmg - TowerStats(T7).Dmg) < 1e-3f;
			FPDTowerMap Four;
			Put(Four, 0, 0, C(5)); Put(Four, 3, 0, C(5, He)); Put(Four, 6, 0, C(5, Di)); Put(Four, 9, 0, C(5, Cl));
			ApplyEffects(Four, FPDCharmBits(1) << (int)EPDCharm::Rainbow);
			const bool bRainbow = std::fabs(Four.begin()->second.Eff.Dmg - 1.25f) < 1e-3f;
			Expect(bLocked && bOpen && bRebel && bRainbow && (int)EPDCharm::Count == 60 && CharmPrice(59) > 0 && JokerPrice((int)EPDJoker::Star) == 500,
				"잠긴 스타는 못 만들고 열면 된다, 2의 반란 ×3, 무지개 +25%, 묘약 60종");
			// 상위 조커 잠금 (D-241): 잭 + 체셔 = 재버워키
			G.Towers.clear();
			FPDTower Ja, Ch;
			Ja.Card = Ch.Card = {0, EPDSuit::Wild};
			Ja.X = 0; Ja.Y = Row; Ja.Joker = (int)EPDJoker::Jack; Ja.JLevel = 10;
			Ch.X = 1; Ch.Y = Row; Ch.Joker = (int)EPDJoker::Cheshire; Ch.JLevel = 10;
			G.Towers[CellKey(0, Row)] = Ja; G.Towers[CellKey(1, Row)] = Ch;
			G.Synergies = ApplyEffects(G.Towers, 0);
			auto HasUpper = [&G]() { for (const FPDJokerMerge& M : G.JokerMerges()) if (M.Upper >= 0) return true; return false; };
			const uint32_t KeepU = G.UpperUnlocked;
			G.UpperUnlocked = 0;
			const bool bULocked = !HasUpper();
			G.UpperUnlocked = KeepU;
			Expect(bULocked && HasUpper() && UpperPrice(26) == 200 && UpperPrice(11) == 150, "잠긴 상위 조커는 못 합치고 열면 된다, 값 (300 + 재료 평균) / 2 (재버워키 200, 험프티 덤프티 150, D-320)");
		}
		{ // 족보에 든 합성 타워도 기억한 족보로 재료 (D-220): 로열 A♠ Lv2 가 Lv2 A♦ 와 원 페어
			G.Towers.clear();
			Put(G.Towers, 0, Row, C(14), 2); G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::RoyalFlush;
			Put(G.Towers, 1, Row, C(14, Di), 2);
			PutFused(G.Towers, 6, Row, C(7, Cl), EPDHand::Pair); // 따로 선 원 페어 합성 타워
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			int NR = -1, NP = -1, NP2 = -1;
			for (int I = 0; I < (int)Ns.size(); ++I) { if (Ns[I].Kind == EPDHand::RoyalFlush) NR = I; if (Ns[I].Kind == EPDHand::Pair) { if (Ns[I].Syn >= 0) NP = I; else NP2 = I; } }
			const bool bStar = NR >= 0 && NP2 >= 0 && G.JokerCraftResult({NR, NP2}) == EPDJoker::Star;
			const bool bClash = NR >= 0 && NP >= 0 && FPDGame::JokerNodesClash(Ns[NR], Ns[NP]) && G.JokerCraftResult({NR, NP}) == EPDJoker::Count;
			Expect(NP >= 0 && bStar && bClash, "원 페어에 든 로열 합성 타워도 로열로 재료 (따로 선 원 페어와 스타), 자기가 든 원 페어와는 같이 못 넣음");
		}
		{ // 조커 목록 (D-210): 가장 좋은 재료, 모자란 족보
			G.Towers.clear();
			PutFused(G.Towers, 0, Row, C(4), EPDHand::Pair);
			PutFused(G.Towers, 3, Row, C(5), EPDHand::Pair, 2, 9.f);
			PutFused(G.Towers, 7, Row, C(14, Di), EPDHand::RoyalFlush);
			PutFused(G.Towers, 9, Row, C(8, He), EPDHand::Flush);
			PutFused(G.Towers, 12, Row, C(3, He), EPDHand::StraightFlush);
			PutFused(G.Towers, 14, Row, C(11, Di), EPDHand::Flush);
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			const std::vector<int> B = G.BestJokerPicks(EPDJoker::Star);
			bool bHasBig = false;
			for (int N : B) bHasBig = bHasBig || (Ns[N].Fused == CellKey(3, Row));
			EPDHand Miss = EPDHand::Count;
			const int MissBell = G.JokerMissing(EPDJoker::Bell, &Miss);
			const std::vector<int> B4 = G.BestJokerPicks(EPDJoker::Comedy);
			Expect(B.size() == 2 && G.JokerCraftResult(B) == EPDJoker::Star && bHasBig && G.JokerMissing(EPDJoker::Star) == 0 && MissBell == 2 &&
				B4.size() == 4 && G.JokerCraftResult(B4) == EPDJoker::Comedy,
				"조커 목록: 스타는 배율 큰 원 페어(5♠ ×9)와 로열, 희비극은 재료 4개, 방울은 풀하우스·스트레이트가 모자람");
			G.Towers.erase(CellKey(0, Row)); G.Towers.erase(CellKey(3, Row));
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<int> B2 = G.BestJokerPicks(EPDJoker::Star);
			const int MissStar = G.JokerMissing(EPDJoker::Star, &Miss);
			const int MissComedy = G.JokerMissing(EPDJoker::Comedy);
			Expect(B2.empty() && MissStar == 1 && Miss == EPDHand::Pair && MissComedy == 2, "원 페어가 없으면 스타는 못 만들고 원 페어 하나가 모자람 (OR 레시피 없음), 희비극은 둘");
		}
		// 합성 타워는 족보를 기억한다
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(6)); Put(G.Towers, 1, Row, C(6, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		G.Fuse(0);
		Expect(G.Towers.size() == 1 && G.Towers.begin()->second.FuseHand == (int)EPDHand::Pair, "합성 타워가 원 페어를 기억");
		// 합성 위치 고르기: 재료 칸 중 고른 칸으로, 재료 밖 칸이면 실패하고 판은 그대로
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(4, He)); Put(G.Towers, 2, Row, C(4, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bBad = G.Fuse(0, CellKey(5, Row));
		const size_t AfterBad = G.Towers.size();
		const bool bPicked = G.Fuse(0, CellKey(2, Row));
		Expect(!bBad && AfterBad == 3 && bPicked && G.Towers.size() == 1 && G.Towers.count(CellKey(2, Row)) == 1 && G.Towers.at(CellKey(2, Row)).X == 2,
			"합성 위치: 트리플을 오른쪽 끝 칸에 모음, 재료 밖 칸은 거절");
		Expect(bPicked && G.Towers.begin()->second.Card == C(4, Di), "합성 대표 카드는 고른 칸의 숫자·무늬 (D-193): 4 다이아");
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(7)); Put(G.Towers, 1, Row, C(7, He)); Put(G.Towers, 2, Row, C(13)); Put(G.Towers, 3, Row, C(13, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		{
			int Tp = -1;
			for (int I = 0; I < (int)G.Synergies.size(); ++I) if (G.Synergies[I].Kind == EPDHand::TwoPair) Tp = I;
			const bool bTp = Tp >= 0 && G.Fuse(Tp, CellKey(1, Row));
			Expect(bTp && G.Towers.size() == 1 && G.Towers.at(CellKey(1, Row)).Card == C(7, He), "투페어를 7 하트 칸에 합성하면 7 하트 (예전엔 K)");
		}
		// 고른 칸이 와일드면 예전처럼 표결
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, FPDCard{0, EPDSuit::Wild}); Put(G.Towers, 2, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		{
			const bool bW = !G.Synergies.empty() && G.CanFuse(0) && G.Fuse(0, CellKey(1, Row));
			Expect(bW && G.Towers.size() == 1 && G.Towers.at(CellKey(1, Row)).Card.Rank == 9 && !G.Towers.at(CellKey(1, Row)).Card.IsWild(), "와일드 칸에 합성하면 나머지 카드 표결로 9");
		}
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(4, He)); Put(G.Towers, 2, Row, C(4, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(DefaultFuseCell(G.Synergies[0].Cells) == CellKey(1, Row) && G.Fuse(0) && G.Towers.count(CellKey(1, Row)) == 1, "합성 위치를 안 고르면 가운데 칸 (예전과 같음)");
		// 조커도 고른 칸에 (지휘봉 재료 → 맨 왼쪽에, 재료 사이 빈 칸은 거절)
		G.Towers.clear();
		PutBaton(G.Towers, Row);
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bJBad = G.JokerFuse(0, CellKey(3, Row));
		const bool bJOk = G.JokerFuse(0, CellKey(0, Row));
		Expect(!bJBad && bJOk && G.Towers.size() == 1 && G.Towers.count(CellKey(0, Row)) == 1 && G.Towers.at(CellKey(0, Row)).IsJoker(), "조커 합성 위치: 고른 재료 칸에, 재료 밖은 거절");
		// 대장 지휘: 자기 족보 타워 +30%, 겹친다
		G.Towers.clear();
		Put(G.Towers, 3, Row, C(9)); Put(G.Towers, 4, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const float P0 = G.Towers[CellKey(3, Row)].Eff.Dmg;
		FPDTower Cm;
		Cm.Card = {0, EPDSuit::Wild};
		Cm.Joker = (int)EPDJoker::Comedy;
		Cm.X = 0; Cm.Y = Row;
		G.Towers[CellKey(0, Row)] = Cm;
		G.Synergies = ApplyEffects(G.Towers, 0);
		const float P1 = G.Towers[CellKey(3, Row)].Eff.Dmg;
		Cm.X = 6;
		G.Towers[CellKey(6, Row)] = Cm;
		G.Synergies = ApplyEffects(G.Towers, 0);
		const float P2 = G.Towers[CellKey(3, Row)].Eff.Dmg;
		Expect(std::fabs(P1 / P0 - 1.3f) < 1e-4f && std::fabs(P2 / P0 - 1.6f) < 1e-4f, "원 페어 대장 희비극: 원 페어 타워 +30%, 둘이면 +60% (겹침)");
	}
	{ // 조커 능력: 지휘봉 오라, 희비극 두 발, 방울 멈춤, 스타 전체 피해
		FPDGame G(13);
		G.NewGame(true);
		G.bMuted = true;
		auto MakeJ = [](int X, int Y, EPDJoker K) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Y; T.Joker = (int)K; T.Mult = 4.f; return T; };
		FPDTowerMap M;
		M[CellKey(5, 5)] = MakeJ(5, 5, EPDJoker::Baton);
		FPDTower N;
		N.Card = C(9);
		N.X = 6;
		N.Y = 6;
		M[CellKey(6, 6)] = N;
		ApplyEffects(M, 0);
		Expect(std::fabs(M[CellKey(6, 6)].Eff.Rate - 1.4f) < 1e-4f, "지휘봉: 주변 타워 연사 +40%");
		// 길 옆 칸에 조커 하나만 두고 적을 넣어 본다
		int JX = -1, JY = -1;
		float NearD = 0.f;
		NearPathCell(G, JX, JY, NearD);
		auto Setup = [&](EPDJoker K) {
			G.Towers.clear();
			G.Towers[CellKey(JX, JY)] = MakeJ(JX, JY, K);
			G.Synergies = ApplyEffects(G.Towers, 0);
			G.Phase = EPDPhase::Wave;
			G.Enemies.clear();
			for (int I = 0; I < 3; ++I) { FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 10000.f, 1); E.Speed = 0.f; E.D = NearD + I * 0.15f; G.Enemies.push_back(E); }
		};
		Setup(EPDJoker::Comedy);
		for (int K = 0; K < 30; ++K) G.Update(0.01f); // 희비극 구슬은 날아가서 맞는다
		int Hurt = 0;
		for (auto& E : G.Enemies) Hurt += E.Hp < E.MaxHp;
		Expect(Hurt == 2, "희비극: 두 적을 동시에 공격");
		Setup(EPDJoker::Bell);
		G.Towers.begin()->second.Timer = 7.99f;
		G.Update(0.02f);
		bool bStop = true;
		for (auto& E : G.Enemies) bStop = bStop && E.StopT > 0.f;
		Expect(bStop, "방울: 사거리 안 적을 멈춘다");
		{ // 이슈 #5: 멈춤이 끝난 뒤 다른 둔화를 맞아도 계속 멈춰 있으면 안 된다 (예전: 둔화 100%가 남아 웨이브가 끝나지 않음)
			FPDEnemy& E = G.Enemies[0];
			E.Speed = 1.f;
			for (int K = 0; K < 150; ++K) { E.SlowT = 1.2f; E.SlowAmt = std::max(E.SlowAmt, 0.2f); G.Update(0.02f); } // 3초 동안 약한 둔화를 계속 맞음 (방울 다음 울림 전)
			const float D0 = E.D;
			E.SlowT = 1.2f; E.SlowAmt = std::max(E.SlowAmt, 0.2f);
			G.Update(0.02f);
			Expect(E.D > D0 && E.StopT == 0.f && E.SlowAmt < 0.5f, "방울: 멈춤이 끝나면 약한 둔화를 맞아도 다시 움직인다");
		}
		Setup(EPDJoker::Star);
		for (auto& E : G.Enemies) E.D = NearD + 5.f; // 멀리 있어도
		G.Update(0.01f);
		int Hurt2 = 0;
		for (auto& E : G.Enemies) Hurt2 += E.Hp < E.MaxHp;
		Expect(Hurt2 == 3, "스타: 판 전체 모든 적에게 피해");
	}
	{ // 덱: 52장, 중복 없음
		FPDGame G(9);
		G.NewGame(true);
		std::set<int> U;
		for (auto& Cd : G.Deck) U.insert(Cd.Rank * 4 + (int)Cd.Suit);
		for (auto& Cd : G.Hand) U.insert(Cd.Rank * 4 + (int)Cd.Suit);
		Expect(U.size() == 52 && G.Deck.size() + G.Hand.size() == 52, "덱 52장 중복 없음");
	}
	// ---- 스킬 트리 (노드 번호는 생성기가 정하므로 찾아서 쓴다)
	auto FindKey = [](EPDKeystone K) { for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && TreeNode(I).Fx[0].Param == (int)K) return I; return -1; };
	auto FindJoker = [](EPDJoker J) { for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Joker && TreeNode(I).Fx[0].Param == (int)J) return I; return -1; };
	auto FindStat = [](EPDStat S, int Param) { for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Fx[0].Stat == S && TreeNode(I).Fx[0].Param == Param && TreeNode(I).Fx[1].Stat == EPDStat::None) return I; return -1; };
	auto PathTo = [](int Target) { // 시작점에서 Target 까지 (시작점 제외)
		std::vector<int> Prev(TreeNodeCount(), -2), Q{0};
		Prev[0] = -1;
		for (size_t I = 0; I < Q.size(); ++I) for (int M : TreeLinks(Q[I])) if (Prev[M] == -2) { Prev[M] = Q[I]; Q.push_back(M); }
		std::vector<int> P;
		for (int C = Target; C > 0; C = Prev[C]) P.insert(P.begin(), C);
		return P;
	};
	auto Bits = [](std::initializer_list<int> Ids) { FPDTreeBits B(1); for (int I : Ids) B.set(I); return B; };
	{ // 구조: 모두 이어짐, 키스톤·조커는 막다른 끝, 고리는 4개 (이웃 큰길을 잇는 길)
		const int N = TreeNodeCount();
		int Links = 0, Jokers = 0, Keys = 0, Notables = 0;
		bool bLeaf = true, bKeyLeaf = true;
		for (int I = 0; I < N; ++I)
		{
			Links += (int)TreeLinks(I).size();
			if (TreeNode(I).Kind == EPDNode::Joker) { Jokers++; bLeaf = bLeaf && TreeLinks(I).size() == 1; }
			if (TreeNode(I).Kind == EPDNode::Keystone) { Keys++; bKeyLeaf = bKeyLeaf && TreeLinks(I).size() == 1; }
			Notables += TreeNode(I).Kind == EPDNode::Notable;
		}
		Links /= 2;
		std::vector<int> D(N, -1), Q{0};
		D[0] = 0;
		for (size_t I = 0; I < Q.size(); ++I) for (int M : TreeLinks(Q[I])) if (D[M] < 0) { D[M] = D[Q[I]] + 1; Q.push_back(M); }
		bool bAll = true;
		int MaxD = 0;
		for (int I = 0; I < N; ++I) { bAll = bAll && D[I] >= 0; MaxD = std::max(MaxD, D[I]); }
		Expect(N >= 120 && bAll, "트리 " + std::to_string(N) + "노드 (2배), 모두 시작점과 이어짐");
		Expect(Links - N + 1 == 4, "고리는 4개뿐 (주머니는 막다른 길)");
		Expect(Jokers == 10 && bLeaf && Keys == (int)EPDKeystone::Count && bKeyLeaf, "조커 강화 10·키스톤 " + std::to_string(Keys) + " (상위 조커 짝 D-232 포함), 모두 막다른 끝");
		Expect(Notables >= 24, "특급 노드 " + std::to_string(Notables) + "개");
		int MinJJ = 99;
		for (int A = 0; A < N; ++A)
		{
			if (TreeNode(A).Kind != EPDNode::Joker) continue;
			std::vector<int> E(N, -1), Q2{A};
			E[A] = 0;
			for (size_t I = 0; I < Q2.size(); ++I) for (int M : TreeLinks(Q2[I])) if (E[M] < 0) { E[M] = E[Q2[I]] + 1; Q2.push_back(M); }
			for (int B = 0; B < N; ++B) if (B != A && TreeNode(B).Kind == EPDNode::Joker) MinJJ = std::min(MinJJ, E[B]);
		}
		Expect(MinJJ >= 4, "조커 강화끼리 최소 4걸음 (다시 올라와야 함): " + std::to_string(MinJJ));
		Expect(MaxD <= 9, "가장 먼 노드 " + std::to_string(MaxD) + "포인트");
		bool bPos = true;
		// D-378: 간격을 1.5배로 벌려 화면(1280x720)보다 커졌다 — 0.55배로 줄이면 다 보이는 범위 안 (휠 축소 · 끌기, D-376)
		for (int I = 0; I < N; ++I) bPos = bPos && std::abs(TreeNode(I).X - 640.f) < 640.f / 0.55f - 20.f && std::abs(TreeNode(I).Y - 392.f) < 328.f / 0.55f - 20.f;
		float MinGap = 1e9f;
		for (int I = 0; I < N; ++I) for (int J = I + 1; J < N; ++J) MinGap = std::min(MinGap, std::hypot(TreeNode(I).X - TreeNode(J).X, TreeNode(I).Y - TreeNode(J).Y));
		Expect(bPos && MinGap >= 36.f, "노드가 0.55배 화면 안, 서로 36px 이상 떨어짐 (최소 " + std::to_string((int)MinGap) + ")");
	}
	{ // 15웨이브 뒤 체력 증가율 (D-497, QA 38): 30 1.37 · 60 1.19 · 100 1.1225 · 무한 1.19 (#65), ModeGrowthK 갈래로 바꾸면 16웨이브 적 체력이 그 비율로
		auto Wave16 = [](float K) {
			FPDGame G(11);
			G.Mode = EPDMode::W30;
			G.NewGame(true);
			G.bMuted = true;
			G.ModeGrowthK = K;
			G.Wave = 15;
			G.StartWave();
			float Hp = 0.f;
			for (const FPDEnemy& E : G.PendingForTest()) Hp += E.MaxHp;
			return Hp;
		};
		const float H0 = Wave16(0.f), H2 = Wave16(2.9f);
		Expect(ModeGrowth(EPDMode::W30) == 1.37f && ModeGrowth(EPDMode::W60) == 1.19f && ModeGrowth(EPDMode::W100) == 1.1225f && ModeGrowth(EPDMode::Endless) == 1.19f && H0 > 0.f && std::abs(H2 / H0 - 2.9f / 1.37f) < 0.01f,
			"체력 증가 30 1.37 · 60 1.19 · 100 1.1225 · 무한 1.19 (D-497 · QA 38, 무한은 QA 39), ModeGrowthK 2.9 면 16웨이브 적 체력이 2.9 / 1.37 배");
	}
	{ // 모드 (D-229): 목표 웨이브, 최종 보스 체력 ×3, 목표 웨이브를 넘기면 클리어
		Expect(ModeGoal(EPDMode::W30) == 30 && ModeGoal(EPDMode::W60) == 60 && ModeGoal(EPDMode::W100) == 100 && ModeGoal(EPDMode::Endless) == 0, "모드 목표 30 · 60 · 100 · 무한");
		FPDGame G(11);
		G.Mode = EPDMode::W30;
		G.NewGame(true);
		G.bMuted = true;
		G.Wave = 29;
		G.StartWave();
		float BossHp = 0.f, RefHp = 0.f;
		for (const FPDEnemy& E : G.PendingForTest()) if (EnemyDef(E.Type).bBoss) BossHp = E.MaxHp;
		FPDGame R(11);
		R.Mode = EPDMode::W60;
		R.NewGame(true);
		const float K = std::pow(ModeGrowth(EPDMode::W30) / ModeGrowth(EPDMode::W60), 15.f);
		R.Wave = 29;
		R.StartWave();
		for (const FPDEnemy& E : R.PendingForTest()) if (EnemyDef(E.Type).bBoss) RefHp = E.MaxHp;
		Expect(BossHp > 0.f && RefHp > 0.f && std::abs(BossHp / (RefHp * K) - EnemyDef(EPDEnemy::Bouncer).Hp / EnemyDef(EPDEnemy::Phantom).Hp) < 0.01f, "30 모드 30웨이브 = 최종 보스 바운서 (60 모드 30웨이브 유령 대비 체력, D-335)");
		G.ClearWaveForTest();
		G.Update(0.05f);
		bool bEv = false;
		for (const FPDEvent& E : G.Events) bEv = bEv || E.Kind == FPDEvent::EKind::RunClear;
		Expect(G.Phase == EPDPhase::Clear && bEv, "30웨이브를 넘기면 클리어 (사건 RunClear)");
		FPDGame H(12);
		H.NewGame(true);
		const bool bLoad = H.LoadRun(G.SaveRun());
		Expect(bLoad && H.Mode == EPDMode::W30, "중단 저장에 모드가 남는다");
	}
	{ // 상위 조커 (D-231): 레시피 30개 (조커마다 6개), 조건 레벨, 합성 결과, 트리 끝자락, 능력
		int Deg[(int)EPDJoker::Count] = {};
		bool bSym = true;
		for (int A = 0; A < (int)EPDJoker::Count; ++A)
			for (int B = 0; B < (int)EPDJoker::Count; ++B)
			{
				const int U = A == B ? -1 : UpperOf((EPDJoker)A, (EPDJoker)B);
				bSym = bSym && U == (A == B ? -1 : UpperOf((EPDJoker)B, (EPDJoker)A));
				if (U >= 0 && A < B) { Deg[A]++; Deg[B]++; }
			}
		bool bSix = true;
		for (int D : Deg) bSix = bSix && D == 6;
		Expect(bSym && bSix && UpperOf(EPDJoker::Hatter, EPDJoker::Baton) < 0, "상위 조커 레시피 30개, 조커마다 6개, 모자 장수 + 지휘봉은 없음");
		FPDGame G(21);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		int Row = 0;
		for (int Y = 0; Y < Rows; ++Y) { bool bOk = true; for (int X = 0; X < 4; ++X) bOk = bOk && G.Buildable(X, Y); if (bOk) { Row = Y; break; } }
		auto PutJ = [&](int X, EPDJoker K, int Lv) { FPDTower T; T.Card = {0, EPDSuit::Wild}; T.X = X; T.Y = Row; T.Joker = (int)K; T.JLevel = Lv; T.Mult = 2.f; G.Towers[CellKey(X, Row)] = T; };
		PutJ(0, EPDJoker::Hatter, 9);
		PutJ(1, EPDJoker::Cheshire, 10);
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		auto Uppers = [&]() { int N = 0; for (const FPDJokerMerge& M : G.JokerMerges()) N += M.Upper >= 0; return N; };
		Expect(Uppers() == 0, "조커 레벨 9 이면 상위 조커 재료가 아니다");
		G.Mods.JokerUp |= 1u << (int)EPDJoker::Hatter; // 트리 끝자락을 찍은 셈
		Expect(Uppers() == 1 && G.UpperNeed(G.Towers.at(CellKey(0, Row))) == UpperNeedLevelTip, "트리 끝자락을 찍은 조커는 레벨 5부터 재료");
		G.Mods.JokerUp = 0;
		G.Towers.at(CellKey(0, Row)).JLevel = 10;
		int Idx = -1;
		const std::vector<FPDJokerMerge> Ms = G.JokerMerges();
		for (int I = 0; I < (int)Ms.size(); ++I) if (Ms[I].Upper == 9) Idx = I;
		const bool bFused = Idx >= 0 && G.MergeJokers(Idx, CellKey(1, Row));
		const bool bOne = G.Towers.size() == 1 && G.Towers.count(CellKey(1, Row)) && G.Towers.at(CellKey(1, Row)).Joker == UpperBase + 9;
		Expect(bFused && bOne && std::abs(G.Towers.at(CellKey(1, Row)).Mult - 4.f * UpperBonus) < 1e-4f && (G.UpperFound & (1u << 9)), "모자 장수 + 체셔 → 미친 다과회 (위력 합 × 1.5, 도감에 남음)");
		const int UK = CellKey(1, Row);
		Expect(JokerHas(G.Towers.at(UK).Joker, EPDJoker::Hatter) && JokerHas(G.Towers.at(UK).Joker, EPDJoker::Cheshire) && !JokerHas(G.Towers.at(UK).Joker, EPDJoker::Star), "상위 조커는 재료 둘의 능력을 가진다");
		FPDTower N;
		N.Card = C(9);
		N.X = 2;
		N.Y = Row;
		G.Towers[CellKey(2, Row)] = N;
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		const FPDTowerStats S0 = TowerStats(N, 0, &G.Mods), S1 = TowerStats(G.Towers.at(CellKey(2, Row)), 0, &G.Mods);
		Expect(S1.Range > S0.Range * 10.f && TowerStats(G.Towers.at(UK), 0, &G.Mods).Range >= 99.f, "미친 다과회: 주변 타워 사거리 무한, 자신도 사거리 무한");
		// 새 짝 키스톤 (D-232): 서커스 = 상위 조커 못 만듦, 왕관의 무게 = 위력 배율 ×2.2
		G.Towers.clear();
		PutJ(0, EPDJoker::Jack, 10);
		PutJ(1, EPDJoker::Cheshire, 10);
		G.Mods.Keys = 1u << (int)EPDKeystone::Circus;
		Expect(Uppers() == 0, "서커스 키스톤이면 상위 조커를 만들 수 없다");
		G.Mods.Keys = 1u << (int)EPDKeystone::Crown;
		int Ci = -1;
		const std::vector<FPDJokerMerge> M2 = G.JokerMerges();
		for (int I = 0; I < (int)M2.size(); ++I) if (M2[I].Upper >= 0) Ci = I;
		Expect(Ci >= 0 && G.MergeJokers(Ci) && G.Towers.size() == 1 && std::abs(G.Towers.begin()->second.Mult - 4.f * 2.2f) < 1e-3f && G.Towers.begin()->second.Joker == UpperBase + 26, "왕관의 무게: 잭 + 체셔 → 재버워키, 위력 합 × 2.2");
	}
	{ // 부적 상자 (D-233): 잠긴 부적은 안 나오고, 열면 나온다. 에이스 ×3, 유리 대포 목숨 −3
		FPDGame G(31);
		G.NewGame(true);
		bool bLocked = true, bSeen = false;
		for (int K = 0; K < 200; ++K) for (const FPDReward& R : G.RollRewards()) if (R.bCharm) bLocked = bLocked && R.Id < CharmBaseCount;
		G.CharmUnlocked = ~FPDCharmBits(0);
		for (int K = 0; K < 200 && !bSeen; ++K) for (const FPDReward& R : G.RollRewards()) if (R.bCharm) bSeen = bSeen || R.Id >= CharmBaseCount;
		Expect((int)EPDCharm::Count == 60 && bLocked && bSeen, "묘약 60개: 열기 전엔 새 묘약이 안 나오고, 열면 나온다 (D-239)");
		FPDTower A;
		A.Card = C(14);
		const float D0 = TowerStats(A, 0).Dmg;
		Expect(std::abs(TowerStats(A, 1u << (int)EPDCharm::Aces).Dmg / D0 - 3.f) < 1e-4f, "에이스: A 공격력 ×3");
		G.Phase = EPDPhase::Reward;
		G.Lives = 10;
		G.Rewards = {{true, (int)EPDCharm::GlassCannon}};
		G.TakeReward(0);
		Expect(G.Lives == 7 && G.Has(EPDCharm::GlassCannon), "유리 대포: 고르면 목숨 −3");
	}
	{ // 포인트 구매
		FPDGame G(5);
		G.NewGame(true);
		const int First = TreeLinks(0)[0];
		Expect(G.Level == 1 && G.PointsLeft() == 0 && G.XpToNext() == PointBasePrice, "레벨 1, 포인트 0, 다음 레벨까지 = 기본값 (D-228)");
		Expect(G.CanBuy(First) == FPDGame::EBuy::NoPoint, "포인트가 없으면 못 찍음");
		G.Phase = EPDPhase::Wave;
		for (int I = 0; I < 40; ++I) { FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(E, 100.f); }
		G.Phase = EPDPhase::Build;
		bool bLvEv = false;
		for (const FPDEvent& E : G.Events) bLvEv = bLvEv || E.Kind == FPDEvent::EKind::LevelUp;
		Expect(G.Level == 2 && G.PointsLeft() == 1 && bLvEv, "경험치 20 → 레벨 2, 포인트 1, 레벨업 사건 (레벨 " + std::to_string(G.Level) + ")");
		Expect(G.CanBuy(PathTo(FindKey(EPDKeystone::Lone)).back()) == FPDGame::EBuy::NotLinked, "이어지지 않은 노드는 못 찍음");
		Expect(G.BuyNode(First) && G.PointsLeft() == 0 && G.Points == 1, "시작점 옆 노드 찍기");
		Expect(G.XpToNext() == (int)std::lround(PointBasePrice * PointPriceGrowth), "다음 레벨까지는 성장 배율만큼");
		Expect(G.CanBuy(First) == FPDGame::EBuy::Owned, "이미 찍은 노드");
		G.Level = 21;
		Expect(G.XpToNext() == (int)std::lround(PointBasePrice * std::pow(PointPriceGrowth, 20.f)), "레벨 21 → 22 경험치: " + std::to_string(G.XpToNext()));
		G.Phase = EPDPhase::Reward;
		Expect(G.CanBuy(TreeLinks(0)[1]) == FPDGame::EBuy::Closed, "보상 고르는 중에는 못 찍음");
	}
	{ // 키스톤: 짝은 봉인, 판마다 3개
		FPDGame G(6);
		G.NewGame(true);
		G.Level = 500;
		for (int Id : PathTo(FindKey(EPDKeystone::Lone))) G.BuyNode(Id);
		Expect(G.Mods.Key(EPDKeystone::Lone), "고독한 저격수 찍음");
		const auto CrowdPath = PathTo(FindKey(EPDKeystone::Crowd));
		for (size_t I = 0; I + 1 < CrowdPath.size(); ++I) G.BuyNode(CrowdPath[I]);
		const int Crowd = FindKey(EPDKeystone::Crowd);
		Expect(G.TreeSealed(Crowd) && G.CanBuy(Crowd) == FPDGame::EBuy::Sealed && !G.BuyNode(Crowd), "짝 키스톤 군중은 봉인");
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind != EPDNode::Keystone) G.TreeOwned.set(I);
		G.RebuildMods();
		int Got = 1;
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && G.CanBuy(I) == FPDGame::EBuy::Ok && G.BuyNode(I)) Got++;
		bool bCap = false;
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && !G.TreeOwnedNode(I) && !G.TreeSealed(I)) bCap = bCap || G.CanBuy(I) == FPDGame::EBuy::KeyCap;
		Expect(Got == 3 && G.KeystoneCount() == 3 && bCap, "키스톤은 3개까지, 남은 건 막힘");
		bool bNoPair = true;
		for (int K = 0; K < (int)EPDKeystone::Count; K += 2) bNoPair = bNoPair && !(G.Mods.Key((EPDKeystone)K) && G.Mods.Key((EPDKeystone)(K + 1)));
		Expect(bNoPair, "포인트가 무한이어도 짝 키스톤을 둘 다 가질 수 없다");
	}
	{ // 경험치 (D-228, 예전 골드): 처치 경험치 (마리 수 배율만큼 나눔), 보스 25, 경험치 노드
		FPDGame G(7);
		G.NewGame(true);
		G.Phase = EPDPhase::Wave;
		for (int I = 0; I < 16; ++I) { FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(E, 100.f); }
		Expect(G.XpEarned == 10, "1웨이브 보통 적 16마리 = 예전 10마리 몫 경험치 10");
		FPDEnemy B = G.SpawnTestEnemy(EPDEnemy::ShieldBoss, 1.f, 5);
		G.HitTest(B, 1000.f);
		Expect(G.XpEarned == 35, "보스 처치 경험치 25 (나누지 않음)");
		Expect(G.KillXp(EPDEnemy::Tank, 11) == 5, "탱크 11웨이브 = 3 + 2");
		G.TreeOwned.set(FindStat(EPDStat::XpPct, 0));
		G.RebuildMods();
		const int Before = G.XpEarned;
		for (int I = 0; I < 32; ++I) { FPDEnemy N2 = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(N2, 100.f); }
		Expect(G.XpEarned - Before == 23, "경험치 +15% 노드: 32마리 (예전 20마리 몫) → 23");
	}
	{ // 마리 수: 12 + 4W, 웨이브 전체 체력은 예전(8 + 2W)과 같음
		bool bSame = true;
		for (int W = 1; W <= 40; ++W) bSame = bSame && std::abs(WaveCount(W) * WaveScale(W) - (8.f + 2.f * W)) < 1e-3f;
		Expect(WaveCount(1) == 16 && WaveCount(20) == 92 && bSame, "마리 수 1웨이브 16·20웨이브 92, 전체 체력은 그대로");
	}
	{ // 트리 효과
		FPDTowerMap M;
		Put(M, 0, 0, C(9, EPDSuit::Spade));
		FPDTreeMods Tm = BuildTreeMods(Bits({FindStat(EPDStat::SuitDmg, 0)}));
		ApplyEffects(M, 0, &Tm);
		const float Base = TowerStats(M.begin()->second, 0).Dmg;
		Expect(std::abs(TowerStats(M.begin()->second, 0, &Tm).Dmg / Base - 1.1f) < 1e-3f, "♠ 위력 +10% 노드");
		Tm = BuildTreeMods(Bits({FindKey(EPDKeystone::Lone)}));
		ApplyEffects(M, 0, &Tm);
		Expect(std::abs(TowerStats(M.begin()->second, 0, &Tm).Dmg / Base - 1.6f) < 1e-3f, "고독한 저격수: 혼자 선 타워 ×1.6");
		Tm = BuildTreeMods(Bits({FindStat(EPDStat::CritChance, 0), FindStat(EPDStat::AllRate, 0)}));
		const FPDTowerStats Cs = TowerStats(M.begin()->second, 0, &Tm), C0 = TowerStats(M.begin()->second, 0);
		Expect(Cs.Crit > C0.Crit + 0.01f && Cs.Rate > C0.Rate * 1.02f && std::abs(C0.Crit - BaseCrit) < 1e-5f, "치명타 확률·공격 속도 노드");
		FPDTowerMap F; // 플러시 3장 + 떨어진 원 페어
		Put(F, 0, 0, C(2, EPDSuit::Heart)); Put(F, 1, 0, C(6, EPDSuit::Heart)); Put(F, 2, 0, C(11, EPDSuit::Heart));
		Put(F, 0, 5, C(8, EPDSuit::Club)); Put(F, 1, 5, C(8, EPDSuit::Diamond));
		ApplyEffects(F, 0);
		const float Fl = TowerStats(F.at(CellKey(0, 0)), 0).Dmg, Pr = TowerStats(F.at(CellKey(0, 5)), 0).Dmg;
		Tm = BuildTreeMods(Bits({FindKey(EPDKeystone::Purist)}));
		ApplyEffects(F, 0, &Tm);
		Expect(std::abs(TowerStats(F.at(CellKey(0, 0)), 0, &Tm).Dmg / Fl - 2.f) < 1e-3f && std::abs(TowerStats(F.at(CellKey(0, 5)), 0, &Tm).Dmg / Pr - 0.5f) < 1e-3f, "외골수: 플러시 ×2, 다른 족보 ×0.5");
		Tm = BuildTreeMods(Bits({FindKey(EPDKeystone::Omni)}));
		ApplyEffects(F, 0, &Tm);
		Expect(F.at(CellKey(0, 0)).Eff.Splash == 0.f, "잡식가: 플러시 범위 효과 없음");
		FPDGame G(8);
		G.NewGame(true);
		G.Towers.clear();
		const int Row = FreeRow(G, 7);
		PutBaton(G.Towers, Row); // 지휘봉 재료 (D-487)
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		Expect(!G.JokerGroups().empty(), "조커 재료 있음");
		G.TreeOwned = Bits({FindKey(EPDKeystone::Smith)});
		G.RebuildMods();
		Expect(G.JokerGroups().empty() && std::abs(G.FuseMult() - 2.f) < 1e-4f, "대장장이: 조커 못 만듦, 합성 배율 2.0");
		G.TreeOwned = Bits({FindKey(EPDKeystone::ClownPath)});
		for (auto& KV : G.Towers) KV.second.Level = 2;
		G.RebuildMods();
		Expect(!G.CanFuse(0) && std::abs(G.JokerMult() - (JokerBonus + 0.4f)) < 1e-4f, "광대의 길: 레벨 2는 더 합성 못 함, 조커 배율 +0.4");
		Expect(G.FuseBlock(0) == FPDGame::EFuseBlock::ClownPath, "합성 막힌 까닭 = 광대의 길 (D-371)");
		for (auto& KV : G.Towers) KV.second.Level = MaxLevel;
		Expect(G.FuseBlock(0) == FPDGame::EFuseBlock::TopLevel, "합성 막힌 까닭 = 최고 레벨 (D-371)");
		for (auto& KV : G.Towers) KV.second.Level = 1;
		Expect(G.FuseBlock(0) == FPDGame::EFuseBlock::None && G.CanFuse(0), "레벨 1 은 광대의 길이어도 합성 (D-371)");
		for (auto& KV : G.Towers) KV.second.Level = 2; // 뒤 시험은 레벨 2 그대로
		FPDTower J;
		J.Card = {0, EPDSuit::Wild};
		J.Joker = (int)EPDJoker::Jack;
		const float J0 = TowerStats(J, 0).Dmg;
		Tm = BuildTreeMods(Bits({FindJoker(EPDJoker::Jack)}));
		Expect(std::abs(TowerStats(J, 0, &Tm).Dmg / J0 - 5.f / 3.f) < 1e-3f, "잭 강화: ×3 → ×5");
		bool bNew = false;
		for (int I = 0; I < TreeNodeCount(); ++I) for (const FPDNodeFx& Fx : TreeNode(I).Fx) bNew = bNew || (Fx.Stat == EPDStat::HandDmg && (Fx.Param & (1 << (int)EPDHand::Five)) && TreeNode(I).Kind == EPDNode::Notable);
		Expect(bNew, "새 족보(파이브 카드)를 키우는 특급 노드가 있음");
	}
	{ // 영구 강화 효과 (D-228, D-230): 레벨 0~20, 칩 값 5 + 5 × 레벨
		FPDGame G(9);
		G.NewGame(true);
		G.Upg[(int)EPDUpgrade::Attack] = 3;
		G.RebuildMods();
		FPDTower T;
		T.Card = C(9);
		const float D0 = TowerStats(T, 0).Dmg;
		Expect(std::abs(TowerStats(T, 0, &G.Mods).Dmg / D0 - 1.09f) < 1e-4f, "공격 +3%/레벨");
		G.Upg[(int)EPDUpgrade::Crit] = 145; // 5% + 145% = 150% (상한 20을 넘긴 값은 시험용)
		G.Upg[(int)EPDUpgrade::Spade] = 10;
		G.RebuildMods();
		const FPDTowerStats S = TowerStats(T, 0, &G.Mods);
		Expect(std::abs(S.Crit - 1.5f) < 1e-3f && std::abs(S.CritFactor() - (1.f + 1.5f * 0.5f)) < 1e-3f, "치명타 확률 150% = 치명타 1~2번 (기대 배율 1.75)");
		FPDGame N(9);
		N.Upg[(int)EPDUpgrade::Attack] = 5;
		N.NewGame(true);
		Expect(std::abs(N.Mods.UpgDmg - 0.15f) < 1e-4f, "새 판을 시작하면 영구 강화가 바로 걸린다");
		N.Wave = 12; N.Level = 9;
		Expect(N.RunChips() == 2 * (12 + 16), "칩 = 2 × (웨이브 + 2 × 오른 레벨) (D-323)");
		{
			// D-333: 일반 상위 조커(험프티 덤프티 = 웃는 가면 + 희비극)는 레벨 조건 없이 맞닿기만 하면, 위력 합 × 1.3. ★ 는 레벨 10 그대로
			FPDGame U;
			U.NewGame(true);
			U.Phase = EPDPhase::Build;
			U.Towers.clear();
			FPDTower Ma, Co;
			Ma.Card = Co.Card = {0, EPDSuit::Wild};
			Ma.X = 0; Ma.Y = 0; Ma.Joker = (int)EPDJoker::Mask; Ma.JLevel = 1; Ma.Mult = 1.f;
			Co.X = 1; Co.Y = 0; Co.Joker = (int)EPDJoker::Comedy; Co.JLevel = 1; Co.Mult = 1.f;
			U.Towers[CellKey(0, 0)] = Ma; U.Towers[CellKey(1, 0)] = Co;
			U.Synergies = ApplyEffects(U.Towers, 0);
			int Mi = -1;
			const std::vector<FPDJokerMerge> Ms = U.JokerMerges();
			for (int I = 0; I < (int)Ms.size(); ++I) if (Ms[I].Upper == 11) Mi = I;
			const bool bMade = Mi >= 0 && U.MergeJokers(Mi, CellKey(0, 0)) && U.Towers.size() == 1;
			const float Mult = bMade ? U.Towers.begin()->second.Mult : 0.f;
			FPDTower Ha = Ma, Ch = Co; // ★ 미친 다과회 (모자 장수 + 체셔) 레벨 1 은 안 됨
			Ha.Joker = (int)EPDJoker::Hatter; Ch.Joker = (int)EPDJoker::Cheshire;
			U.Towers.clear(); U.Towers[CellKey(0, 0)] = Ha; U.Towers[CellKey(1, 0)] = Ch;
			bool bStarLocked = true;
			for (const FPDJokerMerge& M : U.JokerMerges()) bStarLocked &= M.Upper != 9;
			Expect(bMade && std::abs(Mult - 2.f * UpperBonusPlain) < 1e-3f && bStarLocked, "일반 상위 조커는 레벨 1 이어도 맞닿으면 만들고 ×1.3, ★ 는 레벨 10 필요 (D-333)");
			// 바다코끼리와 목수 (희비극 + 지휘봉, 상위 17번)는 고유 능력은 없어도 ★ 대표: 레벨 10 조건, ×1.5 (#66)
			FPDTower Cm = Ma, Bt = Co;
			Cm.Joker = (int)EPDJoker::Comedy; Bt.Joker = (int)EPDJoker::Baton;
			U.Towers.clear(); U.Towers[CellKey(0, 0)] = Cm; U.Towers[CellKey(1, 0)] = Bt;
			bool bWalrusLocked = true;
			for (const FPDJokerMerge& M : U.JokerMerges()) bWalrusLocked &= M.Upper != 17;
			Cm.JLevel = Bt.JLevel = JokerUpLevel;
			U.Towers.clear(); U.Towers[CellKey(0, 0)] = Cm; U.Towers[CellKey(1, 0)] = Bt;
			int Wi = -1;
			const std::vector<FPDJokerMerge> Ws = U.JokerMerges();
			for (int I = 0; I < (int)Ws.size(); ++I) if (Ws[I].Upper == 17) Wi = I;
			const bool bWalrus = Wi >= 0 && U.MergeJokers(Wi, CellKey(0, 0)) && U.Towers.size() == 1;
			int NStar = 0;
			for (int K = 0; K < UpperJokerCount; ++K) NStar += UpperStar(K);
			Expect(UpperStar(17) && NStar == 11 && bWalrusLocked && bWalrus && std::abs(U.Towers.begin()->second.Mult - 2.f * UpperBonus) < 1e-3f,
				"바다코끼리와 목수는 ★ 대표: 레벨 1 이면 안 되고, 둘 다 레벨 10 이면 위력 합 ×1.5. ★ 는 모두 11종 (#66)");
		}
		Expect(FirstClearBonus(EPDMode::W30) == 400 && FirstClearBonus(EPDMode::W100) == 1600 && FirstClearBonus(EPDMode::Endless) == 0, "첫 클리어 덤 2 × 200/400/800, 무한은 없음 (D-321, D-323)");
		{
			FPDGoalStats St;
			bool bNone = true;
			for (int I = 0; I < (int)EPDGoal::Count; ++I) bNone &= !GoalMet((EPDGoal)I, St);
			St.ModesCleared = 1u; St.BossKilled = 0x1FFu; St.RunsPlayed = 10;
			const bool bSome = GoalMet(EPDGoal::Clear30, St) && !GoalMet(EPDGoal::Clear60, St) && GoalMet(EPDGoal::Bosses9, St) && GoalMet(EPDGoal::Runs10, St);
			bool bUniq = true, bPriced = true;
			for (int I = 0; I < (int)EPDGoal::Count; ++I) { bPriced &= (int)GoalDef(I).Reward >= CharmBaseCount; for (int K = 0; K < I; ++K) bUniq &= GoalDef(I).Reward != GoalDef(K).Reward; }
			Expect(bNone && bSome && bUniq && bPriced && CharmGoal((int)EPDCharm::JokerFan) == (int)EPDGoal::FirstJoker && CharmGoal(0) < 0, "목표 해금 10종: 처음엔 없음, 조건 맞으면 열림, 보상 칵테일은 겹치지 않고 처음 열린 18종이 아님 (D-321)");
		}
		{
			FPDAchStats A;
			bool bNone = true;
			for (int I = 0; I < (int)EPDAch::Count; ++I) bNone &= !AchMet((EPDAch)I, A);
			A.HandsEver = (1u << (int)EPDHand::Count) - 1u; A.MaxFuseLevel = MaxLevel; A.ModesCleared = 1u; A.bSecretChannel = true;
			const bool bSome = AchMet(EPDAch::AllHands, A) && AchMet(EPDAch::RoyalFlush, A) && AchMet(EPDAch::FuseMax, A) && AchMet(EPDAch::Clear30, A) && AchMet(EPDAch::SecretChannel, A) && !AchMet(EPDAch::Endless150, A);
			bool bNames = true;
			for (int I = 0; I < (int)EPDAch::Count; ++I) for (int K = 0; K < I; ++K) bNames &= std::string(AchApiName((EPDAch)I)) != AchApiName((EPDAch)K);
			FPDAchStats B2;
			B2.BossKilled = (1u << BossKinds) - 1u; B2.bMut3Kill = true;
			const bool bBoss = AchMet(EPDAch::AllBosses, B2) && AchMet(EPDAch::AllFinals, B2) && AchMet(EPDAch::Mut3Boss, B2) && !AchMet(EPDAch::AllBosses, A);
			Expect(bNone && bSome && bNames && bBoss && (int)EPDAch::Count == 27, "도전 과제 27종: 처음엔 없음, 조건 맞으면 이룸, 스팀 이름 겹치지 않음, 보스 셋 (B8, D-337)");
		}
		long long Total = 0;
		for (int L = 0; L < UpgradeMax; ++L) Total += UpgradeCost(L);
		Expect(UpgradeMax == 20 && Total == 1050, "20레벨까지 한 종류 칩 " + std::to_string(Total));
		Expect(UpgradeCost(20) == 105 && UpgradeCost(21) == 107 && UpgradeCost(120) == 305 && UpgradeMaxCleared == 9999, "21레벨부터 105 + 2씩 (100 클리어 뒤 9999까지, D-238)");
	}
}

// ---------------------------------------------------------------- 봇 (웹 프로토타입 test.js 의 평가에서 시작)
static int Coverage(const FPDGame& G, int X, int Y, float Range)
{
	int N = 0;
	for (float D = 0.f; D < G.Path.Length; D += 0.5f) { const FPDPoint P = G.Path.At(D); if (std::hypot(P.X - X - 0.5f, P.Y - Y - 0.5f) <= Range) N++; }
	return N;
}
// 판 위력: 코어의 판 위력 어림 × 타워마다 사거리 안 길 칸 수
static double Power(FPDGame& G, FPDTowerMap& Tw, std::vector<FPDSynergy>* OutSyn = nullptr)
{
	const FPDCharmBits M = G.CharmMask();
	if (OutSyn) *OutSyn = ApplyEffects(Tw, M, &G.Mods);
	else ApplyEffects(Tw, M, &G.Mods);
	return SumBoardPower(Tw, M, &G.Mods, [&G](const FPDTower& T, float Range) { return (double)Coverage(G, T.X, T.Y, Range); });
}
// 트리: 골드가 되는 만큼 찍는다. 지금 판 위력이 가장 많이 오르는 노드 (한 걸음 더 내다봄, 살림 노드는 어림값)
static std::map<int, int> GKeysTaken;
static double TreeGain(FPDGame& G, const FPDTreeBits& Owned, double Before)
{
	FPDGame Copy = G;
	Copy.TreeOwned = Owned;
	Copy.RebuildMods();
	FPDTowerMap Tw = Copy.Towers;
	double V = Power(Copy, Tw) - Before;
	for (int Id = 0; Id < TreeNodeCount(); ++Id)
		if (Owned.test(Id) && !G.TreeOwned.test(Id))
			for (const FPDNodeFx& F : TreeNode(Id).Fx)
			{
				if (F.Stat == EPDStat::XpPct) V += Before * 0.04;
				if (F.Stat == EPDStat::HandMax) V += Before * 0.03;
				if (F.Stat == EPDStat::Draw) V += Before * 0.08;
				if (F.Stat == EPDStat::Lives) V += Before * 0.02;
			}
	return V;
}
// 트리 포인트 쓰기: 포인트당 위력이 가장 많이 오르는 노드 (D-228: 강화는 판 밖이라 여기서 안 씀)
static std::map<int, int> GUpgLv;
static void BotSpend(FPDGame& G, bool bTree)
{
	for (int Guard = 0; Guard < 24; ++Guard)
	{
		FPDTowerMap Cur = G.Towers;
		const double Before = std::max(1.0, Power(G, Cur));
		int BestNode = -1;
		double BestRatio = 0.0;
		if (bTree && G.PointsLeft() > 0)
			for (int Id = 0; Id < TreeNodeCount(); ++Id)
			{
				if (G.CanBuy(Id) != FPDGame::EBuy::Ok) continue;
				FPDTreeBits O1 = G.TreeOwned;
				O1.set(Id);
				double V = TreeGain(G, O1, Before);
				for (int M : TreeLinks(Id)) // 한 걸음 더 (두 포인트로 나눔)
				{
					if (O1.test(M)) continue;
					if (TreeNode(M).Kind == EPDNode::Keystone && (G.KeystoneCount() >= KeystoneMax || G.Mods.Key((EPDKeystone)(TreeNode(M).Fx[0].Param ^ 1)))) continue;
					FPDTreeBits O2 = O1;
					O2.set(M);
					V = std::max(V, TreeGain(G, O2, Before) / 2.0);
				}
				const double R = V;
				if (R > BestRatio) { BestRatio = R; BestNode = Id; }
			}
		if (BestNode >= 0)
		{
			G.BuyNode(BestNode);
			if (TreeNode(BestNode).Kind == EPDNode::Keystone) GKeysTaken[TreeNode(BestNode).Fx[0].Param]++;
		}
		else break;
	}
}

static EPDMode GBotMode = EPDMode::Endless; // 봇 판의 모드 (D-229)
static int GBotClears = 0;
static int GBotUpgLv = 10;   // bUpg 일 때 영구 강화 레벨 (D-322 판당 칩 표)
static int GBotContent = 0;  // 0 = 규칙 기본(기본 조커 전부 · 칵테일 기본 18), 1 = 처음 상태(조커 4종 · 상위 조커 맛보기 2종 · 칵테일 18), 2 = 전부 열림
// 조커 연구 (이슈 #5): study 명령에서만 켠다. 기본값이면 예전 봇과 똑같이 돈다
static bool GStudy = false;      // 판마다 STUDY 한 줄 (tools/sim/joker_study.py 가 모은다)
static bool GSmartJoker = false; // 조커: 열린 조커마다 BestJokerPicks 로 가장 좋은 재료를 따져 만든다 (예전: JokerGroups 첫 묶음들만)
static bool GPursue = false;     // 놓기: 새로 만들 수 있게 되는 조커 레시피마다 판 위력 덤 (레시피를 노림)
static float GJokerSlack = 0.f;  // 조커를 만드는 문턱: 판 위력이 이만큼(비율) 깎여도 만든다
static long GJokerMask = -1;     // 열 조커 (1 << EPDJoker), -1 = 그대로
static int GSeedBase = 0;        // 판 씨앗 시작 (여러 프로세스로 나눠 돌리기)
static bool GAimSF = false;      // 스트레이트 플러시 노림 (D-490): 같은 무늬 줄을 합성 · 먹이기 · 조커 재료로 쓰지 않고 지키며, 줄을 늘리는 자리에 덤
static bool GLuckyOn = true;     // 행운의 한 장 켬 (QA: 끈 판과 비교)
static float GJokerBonus = JokerBonus; // 조커 덤 갈래 (D-492)
static float GModeGrowth = 0.f; // 15웨이브 뒤 체력 증가율 갈래 (D-492), 0 이면 모드 기본값
// 노림 봇이 지키는 칸: 행운의 한 장 줄(3 ~ 4장) + 판 위 스트레이트 플러시 · 로열 (모자 장수 · 스타 재료)
static FPDLucky BotLine(const FPDGame& G, const FPDTowerMap& Tw)
{
	return FindLuckyLine(Tw, [&](int K) { return G.Buildable(K % Cols, K / Cols) && !Tw.count(K); });
}
static bool HasSFHand(const std::vector<FPDSynergy>& Syn) { for (const FPDSynergy& S : Syn) if (S.Kind == EPDHand::StraightFlush || S.Kind == EPDHand::RoyalFlush) return true; return false; }
static std::set<int> SFKeep(const FPDGame& G)
{
	std::set<int> Out;
	if (!GAimSF) return Out;
	const FPDLucky L = BotLine(G, G.Towers);
	Out.insert(L.Line.begin(), L.Line.end());
	for (const FPDSynergy& S : G.Synergies) if (S.Kind == EPDHand::StraightFlush || S.Kind == EPDHand::RoyalFlush) Out.insert(S.Cells.begin(), S.Cells.end());
	return Out;
}
// 줄의 씨앗: 맞닿은 같은 무늬 · 같은 레벨 카드 중 숫자가 1 차이인 짝 수
static int SFLinks(const FPDTowerMap& Tw)
{
	int N = 0;
	for (const auto& KV : Tw)
	{
		const FPDTower& A = KV.second;
		if (A.IsJoker() || A.Card.IsWild()) continue;
		for (const int NK : {KV.first + 1, KV.first + Cols})
		{
			if ((NK == KV.first + 1 && KV.first % Cols == Cols - 1) || !Tw.count(NK)) continue;
			const FPDTower& B = Tw.at(NK);
			if (B.IsJoker() || B.Card.IsWild() || B.Card.Suit != A.Card.Suit || B.Level != A.Level) continue;
			const int D = std::abs(A.Card.Rank - B.Card.Rank);
			N += D == 1 || D == 12;
		}
	}
	return N;
}
static bool Touches(const std::set<int>& Keep, const std::vector<int>& Cells) { for (int K : Cells) if (Keep.count(K)) return true; return false; }
// 판 위 재료로 지금 만들 수 있는 조커 (비트). 칸이 겹치는지는 안 본다 (어림)
static uint32_t CraftableMask(const FPDTowerMap& Tw, const std::vector<FPDSynergy>& Syn, uint32_t Open)
{
	int Have[(int)EPDHand::Count] = {};
	for (const FPDSynergy& S : Syn)
	{
		bool bJ = false;
		for (int K : S.Cells) bJ = bJ || Tw.at(K).IsJoker();
		if (!bJ) Have[(int)S.Kind]++;
	}
	for (const auto& KV : Tw) if (KV.second.Level >= 2 && !KV.second.IsJoker() && KV.second.FuseHand >= 0) Have[KV.second.FuseHand]++;
	uint32_t Out = 0;
	for (int J = 0; J < (int)EPDJoker::Count; ++J)
	{
		if (!((Open >> J) & 1u)) continue;
		int Need[(int)EPDHand::Count] = {};
		for (EPDHand H : JokerRecipe((EPDJoker)J)) Need[(int)H]++; // 레시피 하나 (D-487)
		bool bOk = true;
		for (int H = 0; H < (int)EPDHand::Count; ++H) bOk = bOk && Have[H] >= Need[H];
		if (bOk) Out |= 1u << J;
	}
	return Out;
}
// 레시피 노림 (D-487: 재료 4개 레시피는 한 번에 다 차지 않는다): 조커마다 판에 모자란 재료 수 (칸 겹침은 안 봄, 어림)
static std::array<int, (int)EPDJoker::Count> RecipeMissing(const FPDTowerMap& Tw, const std::vector<FPDSynergy>& Syn)
{
	int Have[(int)EPDHand::Count] = {};
	for (const FPDSynergy& S : Syn)
	{
		bool bJ = false;
		for (int K : S.Cells) bJ = bJ || Tw.at(K).IsJoker();
		if (!bJ) Have[(int)S.Kind]++;
	}
	for (const auto& KV : Tw) if (KV.second.Level >= 2 && !KV.second.IsJoker() && KV.second.FuseHand >= 0) Have[KV.second.FuseHand]++;
	std::array<int, (int)EPDJoker::Count> Out{};
	for (int J = 0; J < (int)EPDJoker::Count; ++J)
	{
		int Need[(int)EPDHand::Count] = {};
		for (EPDHand H : JokerRecipe((EPDJoker)J)) Need[(int)H]++;
		for (int H = 0; H < (int)EPDHand::Count; ++H) Out[J] += std::max(0, Need[H] - Have[H]);
	}
	return Out;
}
// 놓기 덤 (레시피 노림): 새로 만들 수 있게 된 조커마다 +4%, 아직 안 되지만 모자란 재료가 줄어든 조커는 하나당 +1% (열린 조커만)
static double PursueBonus(const std::array<int, (int)EPDJoker::Count>& Before, const std::array<int, (int)EPDJoker::Count>& After, uint32_t Open)
{
	double B = 0.0;
	for (int J = 0; J < (int)EPDJoker::Count; ++J)
	{
		if (!((Open >> J) & 1u) || Before[J] == 0 || After[J] >= Before[J]) continue;
		B += After[J] == 0 ? 0.04 : 0.01 * (Before[J] - After[J]);
	}
	return B;
}
static uint32_t CraftableNow(const FPDGame& G)
{
	if (G.Mods.Key(EPDKeystone::Smith)) return 0;
	return CraftableMask(G.Towers, G.Synergies, G.JokerUnlocked);
}
static void BotTest(int Runs, bool bDumb, bool bNoReward = false, bool bFuse = false, bool bJoker = false, bool bTree = false, bool bUpg = false)
{
	double PointSum = 0, GoldSum = 0, ChipSum = 0, TimeSum = 0;
	GKeysTaken.clear();
	GUpgLv.clear();
	std::map<int, int> Hist;
	int WaveSum = 0;
	std::map<int, int> JokersMade, KindsMade;
	long Feeds = 0, JLvSum = 0, UppersMade = 0;
	for (int Run = GSeedBase; Run < GSeedBase + Runs; ++Run)
	{
		FPDGame G(1000 + Run);
		// 조커 연구 기록 (판마다)
		int Built[(int)EPDJoker::Count] = {}, FirstBuilt[(int)EPDJoker::Count] = {}, CanWaves[(int)EPDJoker::Count] = {}, FirstCan[(int)EPDJoker::Count] = {};
		int HandWaves[(int)EPDHand::Count] = {}, FirstHand[(int)EPDHand::Count] = {}; // 족보 등장 (이슈 #33): 배치를 마친 판에 그 족보가 있던 웨이브 수 · 처음 웨이브
		int Line3Waves = 0, Line4Waves = 0, FirstLine4 = 0; // 행운의 한 장 줄 (D-490)
		std::string LuckyLog; // 행운의 한 장이 온 웨이브
		std::string UpperLog; // 상위 조커: 웨이브:번호
		int LuckySeen = 0;
		int RunUppers = 0, Phases = 0;
		const long FeedsAtStart = Feeds;
		bool bStuck = false;
		std::string BuildLog;
		G.Mode = GBotMode;
		if (bUpg) for (int& U : G.Upg) U = GBotUpgLv; // 영구 강화 흉내: 8종 모두 같은 레벨 (기본 10)
		if (GBotContent == 1) { G.JokerUnlocked = JokerStarterMask; G.UpperUnlocked = (1u << 11) | (1u << 18); }
		if (GBotContent == 2) G.CharmUnlocked = ~FPDCharmBits(0);
		if (GJokerMask >= 0) { G.JokerUnlocked = (uint32_t)GJokerMask; G.UpperUnlocked = 0; }
		G.NewGame(true);
		G.RebuildMods();
		G.bMuted = true;
		G.bLucky = GLuckyOn;
		G.JokerBonusK = GJokerBonus;
		G.ModeGrowthK = GModeGrowth;
		std::mt19937 R(77 + Run);
		double TyShare[40] = {}; int TyPh[40] = {}, TyMaxLv[40] = {}; double LvAt[3] = {}; int LvN[3] = {}; // QA 조커 스펙: 종류별 몫 · 레벨
		double JsShare = 0, JsDmg = 0, JsRate = 0, JsSplash = 0, JsKinds = 0, JsMult = 0; int JsW = 0, JsN = 0; double PowAt[4] = {}, JPowAt[4] = {}; int JCntAt[4] = {};
		for (int Guard = 0; Guard < 200 && G.Phase != EPDPhase::Over && G.Phase != EPDPhase::Clear; ++Guard)
		{
			// 상위 조커 (D-231): 판 위력이 오르면 합친다
			for (int Tries = 0; bJoker && Tries < 2; ++Tries)
			{
				const std::vector<FPDJokerMerge> Ms = G.JokerMerges();
				FPDTowerMap Cur = G.Towers;
				const double Base = Power(G, Cur);
				int BestI = -1;
				double BestP = Base;
				for (int I = 0; I < (int)Ms.size(); ++I)
				{
					if (Ms[I].Upper < 0) continue;
					FPDGame Copy = G;
					if (!Copy.MergeJokers(I)) continue;
					FPDTowerMap Tw = Copy.Towers;
					const double P = Power(Copy, Tw);
					if (P > BestP) { BestP = P; BestI = I; }
				}
				if (BestI < 0) break;
				const int UpId = Ms[BestI].Upper;
				if (!G.MergeJokers(BestI)) break;
				UppersMade++;
				RunUppers++;
				UpperLog += (UpperLog.empty() ? "" : ",") + std::to_string(G.Wave + 1) + ":" + std::to_string(UpId); // 상위 조커 종류 (이슈 #33)
			}
			// 조커 먹이기: 판 위 위력을 15% 넘게 깎지 않는 족보 중 경험치가 가장 많은 것 (비운 칸은 바로 아래에서 다시 채움)
			for (int Tries = 0; bJoker && Tries < 3 && !G.Hand.empty(); ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				int BestS = -1;
				float BestXp = 0.f;
				const std::set<int> Keep = SFKeep(G);
				for (const FPDFeed& Fd : G.FeedTargets())
				{
					if (Touches(Keep, G.Synergies[Fd.Syn].Cells)) continue; // 노림 봇: 줄은 먹이지 않는다
					FPDGame Copy = G;
					Copy.Feed(Fd.Syn);
					FPDTowerMap After = Copy.Towers;
					if (Power(Copy, After) >= Before * 0.85 && Fd.Xp > BestXp) { BestXp = Fd.Xp; BestS = Fd.Syn; }
				}
				if (BestS < 0) break;
				G.Feed(BestS);
				Feeds++;
			}
			std::vector<std::array<int, 2>> Free;
			for (int Y = 0; Y < Rows; ++Y) for (int X = 0; X < Cols; ++X) if (G.Buildable(X, Y) && !G.Towers.count(CellKey(X, Y))) Free.push_back({X, Y});
			if (bDumb)
			{
				while (!G.Hand.empty())
				{
					std::vector<std::array<int, 2>> Ok;
					for (auto& F : Free) if (!G.Towers.count(CellKey(F[0], F[1])) && Coverage(G, F[0], F[1], 2.2f) > 4) Ok.push_back(F);
					if (Ok.empty()) break;
					auto F = Ok[std::uniform_int_distribution<int>(0, (int)Ok.size() - 1)(R)];
					G.Place(0, F[0], F[1]);
				}
			}
			while (!G.Hand.empty())
			{
				std::vector<std::array<int, 2>> Cand;
				std::vector<std::pair<int, std::array<int, 2>>> Cov;
				for (auto& F : Free)
				{
					if (G.Towers.count(CellKey(F[0], F[1]))) continue;
					bool bNear = false;
					for (int K = 0; K < 4; ++K) { static const int D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; bNear = bNear || G.Towers.count(CellKey(std::clamp(F[0] + D[K][0], 0, Cols - 1), std::clamp(F[1] + D[K][1], 0, Rows - 1))); }
					if (bNear) Cand.push_back(F);
					Cov.push_back({Coverage(G, F[0], F[1], 2.4f), F});
				}
				std::sort(Cov.begin(), Cov.end(), [](auto& A, auto& B) { return A.first > B.first; });
				for (int I = 0; I < 14 && I < (int)Cov.size(); ++I) if (std::find(Cand.begin(), Cand.end(), Cov[I].second) == Cand.end()) Cand.push_back(Cov[I].second);
				double BestV = -1;
				int BI = -1, BX = 0, BY = 0;
				const bool bPursue = GPursue && bJoker && !G.Mods.Key(EPDKeystone::Smith);
				const std::array<int, (int)EPDJoker::Count> MissBefore = bPursue ? RecipeMissing(G.Towers, G.Synergies) : std::array<int, (int)EPDJoker::Count>{};
				const bool bAim = GAimSF && bJoker;
				const int LineBefore = bAim ? BotLine(G, G.Towers).Len : 0, LinksBefore = bAim ? SFLinks(G.Towers) : 0;
				const bool bSFBefore = bAim && HasSFHand(G.Synergies);
				for (int I = 0; I < (int)G.Hand.size(); ++I)
					for (auto& F : Cand)
					{
						FPDTowerMap Tw = G.Towers;
						FPDTower T;
						T.Card = G.Hand[I];
						T.X = F[0];
						T.Y = F[1];
						Tw[CellKey(F[0], F[1])] = T;
						std::vector<FPDSynergy> Syn;
						double V = Power(G, Tw, bPursue || bAim ? &Syn : nullptr);
						if (bPursue) V *= 1.0 + PursueBonus(MissBefore, RecipeMissing(Tw, Syn), G.JokerUnlocked);
						if (bAim) // 스트레이트 플러시 노림 (D-490): 완성하면 크게, 줄이 길어지면 조금
						{
							if (!bSFBefore && HasSFHand(Syn)) V *= 1.5;
							else
							{
								const int After = BotLine(G, Tw).Len;
								if (After > LineBefore) V *= After >= 4 ? 1.3 : 1.15;
								else if (SFLinks(Tw) > LinksBefore) V *= 1.04;
							}
						}
						if (V > BestV) { BestV = V; BI = I; BX = F[0]; BY = F[1]; }
					}
				if (BI < 0) break;
				G.Place(BI, BX, BY);
			}
			Phases++;
			{ // QA 조커 몫 (이슈 #37): JSTAT 줄 = 조커가 판 위력에서 차지하는 몫 · 조커가 받은 효과 · 5/10/15/20웨이브 판 위력
				FPDTowerMap Tw = G.Towers;
				const FPDCharmBits Cm = G.CharmMask();
				ApplyEffects(Tw, Cm, &G.Mods);
				auto Wt = [&G](const FPDTower& T, float Range) { return (double)Coverage(G, T.X, T.Y, Range); };
				double Tot = 0, Jp = 0; int Jc = 0;
				for (const auto& KV : Tw)
				{
					const double P = OneTowerPower(Tw, KV.second, Cm, &G.Mods, Wt);
					Tot += P;
					if (!KV.second.IsJoker()) continue;
					Jp += P; Jc++;
					const FPDEffects& E = KV.second.Eff;
					JsDmg += E.Dmg; JsRate += E.Rate; JsSplash += E.Splash; JsMult += KV.second.Mult;
					int Kc = 0; for (int H = 0; H < (int)EPDHand::Count; ++H) Kc += (E.Kinds >> H) & 1u;
					JsKinds += Kc; JsN++;
				}
				if (Jc > 0 && Tot > 0) { JsShare += Jp / Tot; JsW++; }
				if (Tot > 0)
					for (const auto& KV : Tw)
					{
						const int Ty = KV.second.Joker;
						if (Ty < 0 || Ty >= 40) continue;
						TyShare[Ty] += OneTowerPower(Tw, KV.second, Cm, &G.Mods, Wt) / Tot; TyPh[Ty]++;
						TyMaxLv[Ty] = std::max(TyMaxLv[Ty], KV.second.JLevel);
						static const int LW[3] = {10, 20, 30};
						for (int Q = 0; Q < 3; ++Q) if (G.Wave + 1 == LW[Q]) { LvAt[Q] += KV.second.JLevel; LvN[Q]++; }
					}
				static const int At[4] = {5, 10, 15, 20};
				for (int Q = 0; Q < 4; ++Q) if (G.Wave + 1 == At[Q]) { PowAt[Q] = Tot; JPowAt[Q] = Jp; JCntAt[Q] = Jc; }
			}
			if (bJoker)
			{
				const uint32_t Can = CraftableNow(G);
				for (int J = 0; J < (int)EPDJoker::Count; ++J) if ((Can >> J) & 1u) { CanWaves[J]++; if (!FirstCan[J]) FirstCan[J] = G.Wave + 1; }
				bool HSeen[(int)EPDHand::Count] = {};
				for (const FPDSynergy& S : G.Synergies) HSeen[(int)S.Kind] = true;
				for (int H = 0; H < (int)EPDHand::Count; ++H) if (HSeen[H]) { HandWaves[H]++; if (!FirstHand[H]) FirstHand[H] = G.Wave + 1; }
				const FPDLucky Ln = BotLine(G, G.Towers);
				if (Ln.Len >= 4) { Line4Waves++; if (!FirstLine4) FirstLine4 = G.Wave + 1; } else if (Ln.Len == 3) Line3Waves++;
				if (G.LuckyHits > LuckySeen) { LuckyLog += (LuckyLog.empty() ? "" : ",") + std::to_string(G.Wave); LuckySeen = G.LuckyHits; }
			}
			// 조커 (똑똑한 봇): 열린 조커마다 가장 좋은 재료로 만들어 보고, 판 위력이 가장 많이 오르는 것
			for (int Tries = 0; bJoker && GSmartJoker && Tries < 5; ++Tries)
			{
				const uint32_t Can = CraftableNow(G);
				if (!Can) break;
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				double BestGain = -GJokerSlack * Before - 1e-9;
				std::vector<int> BestPicks;
				int BestCell = -1;
				for (int J = 0; J < (int)EPDJoker::Count; ++J)
				{
					if (!((Can >> J) & 1u)) continue;
					const std::vector<int> Picks = G.BestJokerPicks((EPDJoker)J);
					if (Picks.empty()) continue;
					if (GAimSF && J != (int)EPDJoker::Hatter && J != (int)EPDJoker::Star) // 노림 봇: 줄을 다른 조커 재료로 쓰지 않는다
					{
						const std::set<int> Keep = SFKeep(G);
						const std::vector<FPDJokerNode> Ns0 = G.JokerNodes();
						bool bHit = false;
						for (int P : Picks) bHit = bHit || Touches(Keep, Ns0[P].Cells);
						if (bHit) continue;
					}
					FPDGame Copy = G;
					if (!Copy.JokerCraft(Picks, -1)) continue;
					FPDTowerMap After = Copy.Towers;
					const double Gain = Power(Copy, After) - Before;
					if (Gain > BestGain) { BestGain = Gain; BestPicks = Picks; BestCell = -1; }
					// 상위 조커 짝과 맞닿는 재료 칸에 두면 덤 (다음에 합칠 수 있음)
					const std::vector<FPDJokerNode> Ns = G.JokerNodes();
					std::vector<int> Cells;
					for (int P : Picks) Cells.insert(Cells.end(), Ns[P].Cells.begin(), Ns[P].Cells.end());
					for (int K : Cells)
					{
						bool bNext = false;
						for (const auto& OV : G.Towers)
						{
							const FPDTower& O = OV.second;
							if (!O.IsJoker() || IsUpperJoker(O.Joker) || O.Joker == J || std::abs(O.X - K % Cols) + std::abs(O.Y - K / Cols) != 1) continue;
							const int U = UpperOf((EPDJoker)J, (EPDJoker)O.Joker);
							if (U >= 0 && G.UpperOpen(U) && G.UpperNeedFor(U, O) <= O.JLevel) bNext = true;
						}
						if (!bNext) continue;
						FPDGame C2 = G;
						if (!C2.JokerCraft(Picks, K)) continue;
						FPDTowerMap A2 = C2.Towers;
						const double G2 = Power(C2, A2) - Before + 0.05 * Before;
						if (G2 > BestGain) { BestGain = G2; BestPicks = Picks; BestCell = K; }
					}
				}
				std::set<int> OldJ; // 새로 생긴 조커만 센다 (같은 정리 때 만든 조커도 Age 0 이라 Age 로는 못 가른다, 이슈 #25)
				for (auto& KV2 : G.Towers) if (KV2.second.IsJoker()) OldJ.insert(KV2.first);
				if (BestPicks.empty() || !G.JokerCraft(BestPicks, BestCell)) break;
				JokersMade[0]++;
				for (auto& KV2 : G.Towers) if (KV2.second.IsJoker() && !OldJ.count(KV2.first) && !IsUpperJoker(KV2.second.Joker))
				{
					const int J = KV2.second.Joker;
					KindsMade[J]++;
					Built[J]++;
					if (!FirstBuilt[J]) FirstBuilt[J] = G.Wave + 1;
					BuildLog += (BuildLog.empty() ? "" : ",") + std::to_string(G.Wave + 1) + ":" + std::to_string(J);
					break;
				}
			}
			// 조커: 만들면 전체 위력이 오르는 묶음을 만든다
			for (int Tries = 0; bJoker && !GSmartJoker && Tries < 5; ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				const auto Groups = G.JokerGroups();
				int BestI = -1;
				double BestGain = 0.0;
				int BestCell = -1;
				const std::set<int> KeepG = SFKeep(G);
				for (int I = 0; I < (int)Groups.size(); ++I)
				{
					if (Groups[I].Result != EPDJoker::Hatter && Groups[I].Result != EPDJoker::Star && Touches(KeepG, Groups[I].Cells)) continue;
					FPDGame Copy = G;
					Copy.JokerFuse(I);
					FPDTowerMap After = Copy.Towers;
					const double Gain = Power(Copy, After) - Before;
					if (Gain > BestGain) { BestGain = Gain; BestI = I; BestCell = -1; }
					// 상위 조커 짝 옆 칸 (D-333: 일반 상위 조커는 맞닿기만 하면 된다): 짝 조커와 맞닿은 묶음 칸만 더 따져 본다 (싸게)
					const int NewJ = (int)Groups[I].Result;
					for (int K : Groups[I].Cells)
					{
						bool bNextToPartner = false;
						for (const auto& OV : G.Towers)
						{
							const FPDTower& O = OV.second;
							if (!O.IsJoker() || IsUpperJoker(O.Joker) || O.Joker == NewJ || std::abs(O.X - K % Cols) + std::abs(O.Y - K / Cols) != 1) continue;
							const int U = UpperOf((EPDJoker)NewJ, (EPDJoker)O.Joker);
							if (U >= 0 && G.UpperOpen(U) && !UpperStar(U)) bNextToPartner = true;
						}
						if (!bNextToPartner) continue;
						FPDGame C2 = G;
						if (!C2.JokerFuse(I, K)) continue;
						FPDTowerMap A2 = C2.Towers;
						const double G2 = Power(C2, A2) - Before + 0.05 * Before; // 다음에 상위 조커로 합칠 수 있어 덤
						if (G2 > BestGain) { BestGain = G2; BestI = I; BestCell = K; }
					}
				}
				if (BestI < 0) break;
				std::set<int> OldJ;
				for (auto& KV2 : G.Towers) if (KV2.second.IsJoker()) OldJ.insert(KV2.first);
				G.JokerFuse(BestI, BestCell);
				JokersMade[0]++;
				for (auto& KV2 : G.Towers) if (KV2.second.IsJoker() && !OldJ.count(KV2.first))
				{
					KindsMade[KV2.second.Joker]++;
					const int J = KV2.second.Joker;
					if (J >= 0 && J < (int)EPDJoker::Count) { Built[J]++; if (!FirstBuilt[J]) FirstBuilt[J] = G.Wave + 1; BuildLog += (BuildLog.empty() ? "" : ",") + std::to_string(G.Wave + 1) + ":" + std::to_string(J); }
				}
			}
			for (int Tries = 0; bFuse && Tries < 20; ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				int BestI = -1;
				double BestGain = 0.0;
				const std::set<int> KeepF = SFKeep(G);
				for (int I = 0; I < (int)G.Synergies.size(); ++I)
				{
					if (!G.CanFuse(I) || Touches(KeepF, G.Synergies[I].Cells)) continue; // 노림 봇: 줄은 합성하지 않는다 (레벨이 달라지면 끊김)
					FPDGame Copy = G;
					Copy.Fuse(I);
					FPDTowerMap After = Copy.Towers;
					const double Gain = Power(Copy, After) - Before;
					if (Gain > BestGain) { BestGain = Gain; BestI = I; }
				}
				if (BestI < 0) break;
				G.Fuse(BestI);
			}
			if (bTree) BotSpend(G, bTree);
			G.StartWave();
			for (int K = 0; K < 20000 && G.Phase == EPDPhase::Wave; ++K) G.Update(0.05f);
			if (GStudy && G.Phase == EPDPhase::Wave) // 웨이브가 1000초 넘게 안 끝남: 남은 적을 적고 판을 멈춘 것으로 친다
			{
				std::printf("STUCK seed=%d wave=%d pending=%d enemies=%d:", Run, G.Wave, (int)G.PendingForTest().size(), (int)G.Enemies.size());
				for (const FPDEnemy& E : G.Enemies) std::printf(" [type=%d hp=%.0f/%.0f d=%.2f/%.2f spd=%.2f st=%.1f sh=%.0f ph=%d dead=%d borrow=%d slowT=%.2f slowA=%.2f]", (int)E.Type, E.Hp, E.MaxHp, E.D, G.Path.Length, E.Speed, E.StateT, E.Shield, E.Phase, E.bDead, E.Borrow, E.SlowT, E.SlowAmt);
				std::printf("\n");
				for (const auto& KV : G.Towers) if (KV.second.IsJoker()) std::printf(" J%d@%d,%d lv%d", KV.second.Joker, KV.second.X, KV.second.Y, KV.second.JLevel);
				std::printf("\n");
				{ FPDGame Probe = G; float D0 = Probe.Enemies.empty() ? 0.f : Probe.Enemies[0].D; for (int K = 0; K < 400; ++K) { Probe.Update(0.05f); if (K % 40 == 0 && !Probe.Enemies.empty()) std::printf("  t=%.1f d=%.3f slowT=%.2f hp=%.0f\n", K * 0.05f, Probe.Enemies[0].D - D0, Probe.Enemies[0].SlowT, Probe.Enemies[0].Hp); } }
				bStuck = true;
				break;
			}
			if (G.Phase == EPDPhase::Reward) // 보상: 부적 먼저, 없으면 수리 (bNoReward면 건너뛰기)
			{
				int Pick = -1;
				for (int I = 0; I < (int)G.Rewards.size() && !bNoReward; ++I) if (G.Rewards[I].bCharm) { Pick = I; break; }
				for (int I = 0; I < (int)G.Rewards.size() && Pick < 0 && !bNoReward; ++I) if (!G.Rewards[I].bCharm && G.Rewards[I].Id == (int)EPDInstant::Repair) Pick = I;
				G.TakeReward(Pick);
			}
			G.Events.clear();
		}
		{ int Mx = 0; for (auto& KV : G.Towers) if (KV.second.IsJoker()) Mx = std::max(Mx, KV.second.JLevel); JLvSum += Mx; }
		Hist[G.Wave]++;
		GBotClears += G.Phase == EPDPhase::Clear;
		WaveSum += G.Wave;
		PointSum += G.Points;
		GoldSum += G.XpEarned;
		ChipSum += G.RunChips(); // 판당 칩 (D-320 해금 속도 계산)
		TimeSum += G.Time;     // 웨이브 진행 시간 합 (D-322 플레이 시간 어림)
		for (int U = 0; U < (int)EPDUpgrade::Count; ++U) GUpgLv[U] += G.Upg[U];
		if (GStudy)
		{
			int Mx = 0, JNow = 0;
			for (auto& KV : G.Towers) if (KV.second.IsJoker()) { Mx = std::max(Mx, KV.second.JLevel); JNow++; }
			{ // JTYPE 줄: 종류:웨이브 평균 몫(판 위력 중) / 있던 웨이브 수 / 가장 높은 레벨. 조커 레벨 평균 10·20·30웨이브
				std::string Ty;
				for (int T = 0; T < 40; ++T) if (TyPh[T]) { char B[64]; std::snprintf(B, sizeof B, "%s%d:%.3f/%d/%d", Ty.empty() ? "" : ",", T, TyShare[T] / std::max(1, Phases), TyPh[T], TyMaxLv[T]); Ty += B; }
				std::printf("JTYPE seed=%d mode=%d clear=%d wave=%d phases=%d lv10=%.1f lv20=%.1f lv30=%.1f types=%s\n", Run, (int)GBotMode, G.Phase == EPDPhase::Clear, G.Wave, Phases, LvN[0] ? LvAt[0] / LvN[0] : 0., LvN[1] ? LvAt[1] / LvN[1] : 0., LvN[2] ? LvAt[2] / LvN[2] : 0., Ty.empty() ? "-" : Ty.c_str());
			}
			std::printf("JSTAT seed=%d wave=%d clear=%d jw=%d share=%.3f jdmg=%.2f jrate=%.2f jsplash=%.2f jkinds=%.2f jmult=%.2f pow5=%.0f pow10=%.0f pow15=%.0f pow20=%.0f jpow5=%.0f jpow10=%.0f jpow15=%.0f jpow20=%.0f jc10=%d jc15=%d jc20=%d\n", Run, G.Wave, G.Phase == EPDPhase::Clear, JsW, JsW ? JsShare / JsW : 0., JsN ? JsDmg / JsN : 0., JsN ? JsRate / JsN : 0., JsN ? JsSplash / JsN : 0., JsN ? JsKinds / JsN : 0., JsN ? JsMult / JsN : 0., PowAt[0], PowAt[1], PowAt[2], PowAt[3], JPowAt[0], JPowAt[1], JPowAt[2], JPowAt[3], JCntAt[1], JCntAt[2], JCntAt[3]);
			std::printf("STUDY mode=%d upg=%d content=%d mask=%ld smart=%d pursue=%d slack=%.2f aimsf=%d lucky=%d luckyhits=%d jbonus=%.2f growth=%.3f seed=%d stuck=%d wave=%d clear=%d lives=%d phases=%d uppers=%d jnow=%d jmaxlv=%d feeds=%ld time=%.0f built=", (int)GBotMode, bUpg ? GBotUpgLv : 0, GBotContent, GJokerMask, GSmartJoker, GPursue, GJokerSlack, (int)GAimSF, (int)GLuckyOn, G.LuckyHits, GJokerBonus, G.HpGrowth(), Run, (int)bStuck, G.Wave, G.Phase == EPDPhase::Clear, G.Lives, Phases, RunUppers, JNow, Mx, Feeds - FeedsAtStart, G.Time);
			for (int J = 0; J < (int)EPDJoker::Count; ++J) std::printf("%s%d", J ? "," : "", Built[J]);
			std::printf(" first=");
			for (int J = 0; J < (int)EPDJoker::Count; ++J) std::printf("%s%d", J ? "," : "", FirstBuilt[J]);
			std::printf(" can=");
			for (int J = 0; J < (int)EPDJoker::Count; ++J) std::printf("%s%d", J ? "," : "", CanWaves[J]);
			std::printf(" firstcan=");
			for (int J = 0; J < (int)EPDJoker::Count; ++J) std::printf("%s%d", J ? "," : "", FirstCan[J]);
			std::printf(" hands=");
			for (int H = 0; H < (int)EPDHand::Count; ++H) std::printf("%s%d", H ? "," : "", HandWaves[H]);
			std::printf(" firsthand=");
			for (int H = 0; H < (int)EPDHand::Count; ++H) std::printf("%s%d", H ? "," : "", FirstHand[H]);
			std::printf(" line3=%d line4=%d firstline4=%d luckylog=%s keys=%u upperlog=%s", Line3Waves, Line4Waves, FirstLine4, LuckyLog.empty() ? "-" : LuckyLog.c_str(), G.Mods.Keys, UpperLog.empty() ? "-" : UpperLog.c_str());
			std::printf(" log=%s\n", BuildLog.empty() ? "-" : BuildLog.c_str());
			std::fflush(stdout);
		}
	}
	std::printf("\n%s%s%s%s%s 봇 %d판, 무너진 웨이브:", bDumb ? "족보 무시" : "족보 생각", bNoReward ? " · 보상 안 받음" : " · 부적 먼저", bFuse ? " · 합성" : "", bJoker ? " · 조커" : "", bTree ? " · 트리" : "", Runs);
	if (bUpg) std::printf(" (+ 영구 강화 8종 10레벨)");
	for (auto& KV : Hist) std::printf("  %d:%d", KV.first, KV.second);
	std::printf("   평균 %.1f웨이브", (double)WaveSum / Runs);
	if (bJoker) std::printf("   조커 판당 %.1f개 (종류:", (double)JokersMade[0] / Runs);
	if (bJoker) { for (auto& KV : KindsMade) std::printf(" %d=%d", KV.first + 1, KV.second); std::printf(")"); }
	if (bJoker) std::printf("   먹이기 판당 %.1f번 · 최고 조커 레벨 평균 %.1f · 상위 조커 판당 %.1f개", (double)Feeds / Runs, (double)JLvSum / Runs, (double)UppersMade / Runs);
	std::printf("   번 경험치 %.0f · 레벨 포인트 %.1f · 칩 %.0f · 웨이브 시간 %.1f분", GoldSum / Runs, PointSum / Runs, ChipSum / Runs, TimeSum / Runs / 60.0);
	if (bTree) { std::printf("   키스톤:"); for (auto& KV : GKeysTaken) std::printf(" %d=%d", KV.first, KV.second); }
	std::printf("\n");
}

int main(int Argc, char** Argv)
{
	std::printf("[규칙 단위 시험]\n");
	UnitTests();
	const int Runs = Argc > 1 ? std::atoi(Argv[1]) : 10;
	if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "base") BotTest(Runs, false, false, true, true);
	else if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "tree") { BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, false, true, true, true, true); }
	else if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "modes") // 모드별 클리어율 (D-229): 트리까지 쓰는 봇, 강화 없음 / 영구 강화 8종 10레벨
	{
		static const char* Names[3] = {"30", "60", "100"};
		for (int M = 0; M < 3; ++M)
			for (int U = 0; U < 2; ++U)
			{
				GBotMode = (EPDMode)M;
				GBotClears = 0;
				BotTest(Runs, false, false, true, true, true, U == 1);
				std::printf("   → %s 모드%s 클리어 %d / %d판\n", Names[M], U ? " (영구 강화 10레벨)" : "", GBotClears, Runs);
			}
		GBotMode = EPDMode::Endless;
	}
	else if (Runs > 0 && Argc > 5 && std::string(Argv[2]) == "chips") // 판당 칩 표 (D-322): chips <모드 0=30 1=60 2=100 3=무한> <강화 레벨> <해금 0/1/2>
	{
		GBotMode = (EPDMode)std::atoi(Argv[3]); GBotUpgLv = std::atoi(Argv[4]); GBotContent = std::atoi(Argv[5]); GBotClears = 0;
		BotTest(Runs, false, false, true, true, true, GBotUpgLv > 0);
		std::printf("   → CHIPS mode=%s upg=%d content=%d clear=%d/%d\n", Argv[3], GBotUpgLv, GBotContent, GBotClears, Runs);
		GBotMode = EPDMode::Endless; GBotUpgLv = 10; GBotContent = 0;
	}
	else if (Runs > 0 && Argc > 9 && std::string(Argv[2]) == "study") // 조커 연구 (이슈 #5): study <모드> <강화> <해금> <조커 비트, -1 그대로> <씨앗 시작> <똑똑한 조커 0/1> <레시피 노림 0/1> [문턱] [스트레이트 플러시 노림 0/1] [행운의 한 장 1/0] [조커 덤, 기본 JokerBonus] [체력 증가율, 0 = 모드 기본]
	{
		GStudy = true;
		GBotMode = (EPDMode)std::atoi(Argv[3]); GBotUpgLv = std::atoi(Argv[4]); GBotContent = std::atoi(Argv[5]);
		GJokerMask = std::atol(Argv[6]); GSeedBase = std::atoi(Argv[7]); GSmartJoker = std::atoi(Argv[8]) != 0; GPursue = std::atoi(Argv[9]) != 0;
		GJokerSlack = Argc > 10 ? (float)std::atof(Argv[10]) : 0.f;
		GAimSF = Argc > 11 && std::atoi(Argv[11]) != 0; // D-490
		GLuckyOn = Argc > 12 ? std::atoi(Argv[12]) != 0 : true;
		GJokerBonus = Argc > 13 ? (float)std::atof(Argv[13]) : JokerBonus; // D-492 갈래
		GModeGrowth = Argc > 14 ? (float)std::atof(Argv[14]) : 0.f; // D-492 갈래
		BotTest(Runs, false, false, true, true, true, GBotUpgLv > 0);
	}
	else if (Runs > 0) { BotTest(Runs, false); BotTest(Runs, false, false, true); BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, false, true, true, true, true); BotTest(Runs, false, true); BotTest(Runs, true); }
	std::printf("\n%s (실패 %d)\n", GFails ? "실패 있음" : "모두 통과", GFails);
	return GFails ? 1 : 0;
}
