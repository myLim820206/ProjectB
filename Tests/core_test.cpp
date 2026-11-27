// 포커 디펜스 규칙 코어 시험 (언리얼 없이 실행)
//   ./Tests/run.sh [봇 판 수]
// 족보·연쇄·길·설치·적·부적·발사체·합성·조커·스킬 트리·골드 강화를 확인하고, 봇으로 밸런스를 본다.
//   ./Tests/run.sh 0 은 봇 없이, ./Tests/run.sh N base|tree 는 그 봇만, ./Tests/run.sh N level [모드] 는 레벨 곡선, ./Tests/run.sh 0 jokerlv [upper] 는 재료 레벨별 조커 세기 표
#include <chrono>
#include <cstdlib>
#include "../Source/ProjectB/PDCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <map>
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
static void Put(FPDTowerMap& M, int X, int Y, FPDCard Card, int Level = 1)
{
	FPDTower T;
	T.Card = Card;
	T.X = X;
	T.Y = Y;
	T.Level = Level;
	M[CellKey(X, Y)] = T;
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
		G.NewGame(true);
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
		Expect(HasRoyal(Lv2) && HasRoyal(Jk), "Lv2 와일드·A 자리 조커가 끼어도 로열");
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
		for (int I = 0; I < (int)G.Synergies.size(); ++I) if (G.Synergies[I].Kind == EPDHand::Flush && G.Synergies[I].Cells.size() == 4) Fl = I;
		FPDSynergy FS;
		const bool bNo = Fl >= 0 && !G.CanFuse(Fl) && !G.FuseSynergy(Fl, FS) && !G.Fuse(Fl, CellKey(3, 3));
		Expect(bNo && G.Towers.size() == 4, "조커 + ♦ 셋 = 플러시4 → 조커가 껴서 합성 불가, 판 그대로");
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
			"조커 처치 경험치: 일반 +1, 보스 +15 → 레벨 2 (남은 경험치 " + std::to_string((int)J2.JXp) + "), 다른 타워 처치는 없음");
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
	{ // 조커 재료: 맞는 것만 쓰고 3개 레시피 먼저 (D-115, D-118)
		FPDGame G(16);
		G.NewGame(true);
		G.bMuted = true;
		G.Phase = EPDPhase::Build;
		G.Towers.clear();
		const int Row = FreeRow(G, 8);
		// 스트레이트 넷 (2 3 4 · 6 7 8 · 10 J Q · A 2 3, 숫자가 안 이어지게, D-418: 3장씩): 지휘봉 3개 레시피 하나 + 남는 스트레이트 하나
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, He)); Put(G.Towers, 2, Row, C(4)); Put(G.Towers, 3, Row, C(6, He)); Put(G.Towers, 4, Row, C(7)); Put(G.Towers, 5, Row, C(8, He)); Put(G.Towers, 6, Row, C(10)); Put(G.Towers, 7, Row, C(11, He)); Put(G.Towers, 8, Row, C(12));
		Put(G.Towers, 9, Row, C(14, He)); Put(G.Towers, 10, Row, C(2)); Put(G.Towers, 11, Row, C(3, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Gs = G.JokerGroups();
		const bool bThree = !Gs.empty() && Gs[0].Result == EPDJoker::Baton && Gs[0].Cells.size() == 9;
		const bool bFused = bThree && G.JokerFuse(0);
		Expect(bFused && G.Towers.size() == 4, "스트레이트 넷: 2개 레시피보다 3개 레시피(지휘봉 9장) 먼저, 남는 스트레이트는 그대로");
		// 2개 레시피: 원 페어 + 스트레이트 → 희비극 (카드 4장 덤 ×1.4)
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, He)); Put(G.Towers, 2, Row, C(4, Di)); Put(G.Towers, 3, Row, C(5)); Put(G.Towers, 4, Row, C(6, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto G2 = G.JokerGroups();
		const bool b2 = G2.size() == 1 && G2[0].Result == EPDJoker::Comedy && G2[0].Cells.size() == 5 && G.JokerFuse(0);
		const FPDTower& Jc = G.Towers.begin()->second;
		Expect(b2 && G.Towers.size() == 1 && std::fabs(Jc.Mult - 5.f * JokerBonus * JokerTier(EPDJoker::Comedy) * (1.f + JokerCardBonus * 5.f)) < 1e-4f,
			"원 페어 + 스트레이트 → 희비극 2개 레시피 (카드 5장 덤)");
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
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Ms = G.JokerMerges();
		const auto Pw = Ms.size() == 1 ? G.PreviewPower(FPDGame::EPowerAction::JokerMerge, 0) : FPDGame::FPowerPreview{};
		const bool bMerged = Ms.size() == 1 && Ms[0].Cells.size() == 2 && G.MergeJokers(0);
		int Clowns = 0;
		float M = 0.f;
		int Lv = 0;
		for (const auto& KV : G.Towers) if (KV.second.Joker == (int)EPDJoker::Clown) { Clowns++; M = KV.second.Mult; Lv = KV.second.JLevel; }
		Expect(bMerged && Clowns == 1 && std::fabs(M - 10.f * JokerMergeBonus) < 1e-4f && Lv == 5 && G.Towers.size() == 2 && Pw.After > 0.f,
			"같은 광대 둘이 맞닿으면 하나로: 위력 (4+6) × 1.3, 레벨은 높은 쪽 5");
		// 조커가 족보에 와일드로 끼어도 먹이기에서 조커는 사라지지 않는다
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
		bool bRecipes = true, bCaptainIn = true;
		std::set<std::vector<int>> Uniq, Uniq2;
		for (int J = 0; J < (int)EPDJoker::Count; ++J)
		{
			const auto& R = JokerRecipe((EPDJoker)J);
			const auto& R2 = JokerRecipe2((EPDJoker)J);
			bRecipes = bRecipes && MatchJokerRecipe({R[2], R[0], R[1]}) == (EPDJoker)J && MatchJokerRecipe({R2[1], R2[0]}) == (EPDJoker)J; // 순서 무관
			bCaptainIn = bCaptainIn && std::find(R.begin(), R.end(), CaptainHand((EPDJoker)J)) != R.end() && std::find(R2.begin(), R2.end(), CaptainHand((EPDJoker)J)) != R2.end();
			std::vector<int> V{(int)R[0], (int)R[1], (int)R[2]}, V2{(int)R2[0], (int)R2[1]};
			std::sort(V.begin(), V.end());
			std::sort(V2.begin(), V2.end());
			Uniq.insert(V);
			Uniq2.insert(V2);
		}
		Expect(Uniq2.size() == 10, "2개 레시피도 10개 모두 다름 (D-118)");
		Expect(JokerTier(EPDJoker::Comedy) == 2.f && JokerTier(EPDJoker::Bell) == 2.5f && JokerTier(EPDJoker::Star) == 3.f, "조커 등급 배율: 쉬움 ×2, 중간 ×2.5, 어려움 ×3 (D-116)");
		Expect(bRecipes && bCaptainIn && Uniq.size() == 10, "조커 레시피 10개: 서로 다르고, 순서 무관, 자기 대장 족보가 들어 있음 (D-115)");
		Expect(MatchJokerRecipe({H::Pair, H::Pair, H::Straight}) == EPDJoker::Count && MatchJokerRecipe({H::Pair, H::Pair}) == EPDJoker::Count &&
			MatchJokerRecipe({H::Straight, H::Straight, H::Straight}) == EPDJoker::Baton && MatchJokerRecipe({H::Flush, H::Four, H::Straight}) == EPDJoker::Cheshire &&
			MatchJokerRecipe({H::Straight, H::Pair}) == EPDJoker::Comedy && MatchJokerRecipe({H::Pair, H::TwoPair}) == EPDJoker::Mask,
			"레시피와 정확히 맞아야 조커: 3개(스트레이트 셋 = 지휘봉), 2개(원 페어 + 스트레이트 = 희비극), 페어 둘은 없음");
	}
	{ // 조커 합성: 연쇄된 페어 셋 → 광대, 합성 타워 + 족보 둘 → 모자 장수
		FPDGame G(12);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		const int Row = FreeRow(G, 7);
		// 스트레이트 셋(2 3 4 · 6 7 8 · 10 J Q, 숫자가 안 이어지게) = 지휘봉 3개 레시피 (D-118, D-418: 3장씩)
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, He)); Put(G.Towers, 2, Row, C(4)); Put(G.Towers, 3, Row, C(6, He)); Put(G.Towers, 4, Row, C(7)); Put(G.Towers, 5, Row, C(8, He)); Put(G.Towers, 6, Row, C(10)); Put(G.Towers, 7, Row, C(11, He)); Put(G.Towers, 8, Row, C(12));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Groups = G.JokerGroups();
		const bool bGroup = Groups.size() == 1 && Groups[0].Result == EPDJoker::Baton && Groups[0].Cells.size() == 9;
		const bool bFused = G.JokerFuse(0);
		const FPDTower& J = G.Towers.begin()->second;
		Expect(bGroup && bFused && G.Towers.size() == 1 && J.IsJoker() && J.Joker == (int)EPDJoker::Baton && std::fabs(J.Mult - 9.f * JokerBonus * JokerTier(EPDJoker::Baton) * (1.f + JokerCardBonus * 9.f)) < 1e-4f && (G.Discovered & (1u << (int)EPDJoker::Baton)),
			"맞닿은 스트레이트 셋 → 지휘봉 3개 레시피 (위력 9 × 조커 덤 × 등급 ×2 × 카드 9장 덤), 도감에 기록");
		// 족보 먹이기 (D-098): 조커 옆 족보를 먹이면 경험치(족보 점수 × 평균 위력) → 레벨 업, 먹인 칸은 사라짐
		{
			const int JKey = G.Towers.begin()->first, JX = JKey % Cols;
			const float D0 = TowerStats(G.Towers.at(JKey)).Dmg; // 족보에 안 든 조커 혼자
			Put(G.Towers, JX + 1, Row, C(9)); Put(G.Towers, JX + 2, Row, C(9, He)); // 조커(와일드)가 끼어 트리플
			G.Synergies = ApplyEffects(G.Towers, 0);
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
			Put(G.Towers, JX + 1, Row, C(4)); Put(G.Towers, JX + 2, Row, C(5, He));
			G.Synergies = ApplyEffects(G.Towers, 0);
			auto Fd2 = G.FeedTargets();
			Expect(!Fd2.empty() && G.Feed(Fd2[0].Syn) && G.Towers.at(JKey).JLevel >= JokerUpLevel && G.Mods.Up(G.Towers.at(JKey)), "레벨 10 이 되면 강화 능력");
			// 고른 조커가 먹는다 (D-213): 원 페어 양쪽에 조커 둘 (오른쪽이 레벨 낮음)
			const int J2 = CellKey(JX + 3, Row);
			FPDTower Jk2 = G.Towers.at(JKey);
			Jk2.X = JX + 3;
			Jk2.JLevel = 1;
			Jk2.JXp = 0.f;
			G.Towers[J2] = Jk2;
			Put(G.Towers, JX + 1, Row, C(6), 2); Put(G.Towers, JX + 2, Row, C(6, He), 2); // 레벨 2 원 페어 (조커는 와일드라 낄 수 있지만 레벨 2 둘이 따로 족보)
			G.Synergies = ApplyEffects(G.Towers, 0);
			int PairSyn = -1;
			for (int I = 0; I < (int)G.Synergies.size(); ++I)
			{
				const std::vector<int>& Cs = G.Synergies[I].Cells; // 조커가 와일드로 껴도 된다
				if (std::find(Cs.begin(), Cs.end(), CellKey(JX + 1, Row)) != Cs.end()) PairSyn = I;
			}
			bool bDefaultHigh = false, bOnly2 = false;
			for (const FPDFeed& F : G.FeedTargets()) if (F.Syn == PairSyn) bDefaultHigh = F.Joker == JKey;
			for (const FPDFeed& F : G.FeedTargets(J2)) if (F.Syn == PairSyn) bOnly2 = F.Joker == J2;
			const bool bFed = PairSyn >= 0 && G.Feed(PairSyn, J2);
			Expect(bDefaultHigh && bOnly2 && bFed && G.Towers.at(J2).JXp + G.Towers.at(J2).JLevel > 1.f && !G.Towers.count(CellKey(JX + 1, Row)),
				"족보 양쪽에 조커 둘: 기본은 레벨 높은 쪽, 고르면 고른 조커(레벨 1)가 먹는다");
		}
		// 조커는 와일드: 레벨 2 타워 옆에 붙어도 족보
		G.Towers.clear();
		FPDTower Jk;
		Jk.Card = {0, EPDSuit::Wild};
		Jk.X = 0;
		Jk.Y = Row;
		Jk.Joker = (int)EPDJoker::Clown;
		G.Towers[CellKey(0, Row)] = Jk;
		Put(G.Towers, 1, Row, C(9), 2);
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.size() == 1 && !G.CanFuse(0), "조커는 레벨 2 타워와도 족보 (와일드), 조커가 든 족보는 합성 불가");
		// 포카드를 기억하는 합성 타워(따로 선 레벨 3) + 스트레이트 → 체셔 2개 레시피 (D-118)
		G.Towers.clear();
		const int R1 = Row >= 2 ? Row - 1 : Row + 1, R2 = Row >= 2 ? Row - 2 : Row + 2;
		(void)R1; (void)R2;
		Put(G.Towers, 0, Row, C(13), 3); Put(G.Towers, 1, Row, C(7)); Put(G.Towers, 2, Row, C(8, He)); Put(G.Towers, 3, Row, C(9));
		G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Four;
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Cheshire && Groups[0].FusedCells.size() == 1, "포카드를 기억한 합성 타워 + 스트레이트 → 체셔 (2개 레시피)");
		// 따로 선 합성 타워 셋: 플러시 4♠ · 로열 A♦ · 스트레이트 플러시 3♥ 한 줄 → 스타 (사용자 판)
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4), 2); G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Flush;
		Put(G.Towers, 1, Row, C(14, Di), 2); G.Towers[CellKey(1, Row)].FuseHand = (int)EPDHand::RoyalFlush;
		Put(G.Towers, 2, Row, C(3, He), 2); G.Towers[CellKey(2, Row)].FuseHand = (int)EPDHand::StraightFlush;
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Star && Groups[0].Cells.size() == 3, "합성 타워 플러시·로열·스트플 한 줄 → 스타");
		// 조커 만들기 (D-195): 멀리 떨어진 재료를 직접 고른다
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4), 2); G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Flush;
		Put(G.Towers, 7, Row, C(14, Di), 2); G.Towers[CellKey(7, Row)].FuseHand = (int)EPDHand::RoyalFlush;
		Put(G.Towers, 12, Row, C(3, He), 2); G.Towers[CellKey(12, Row)].FuseHand = (int)EPDHand::StraightFlush;
		G.Synergies = ApplyEffects(G.Towers, 0);
		{
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			int NF = -1, NR = -1, NS = -1;
			for (int I = 0; I < (int)Ns.size(); ++I) { if (Ns[I].Kind == EPDHand::Flush) NF = I; if (Ns[I].Kind == EPDHand::RoyalFlush) NR = I; if (Ns[I].Kind == EPDHand::StraightFlush) NS = I; }
			const bool bNodes = Ns.size() == 3 && NF >= 0 && NR >= 0 && NS >= 0;
			const bool bTwo = bNodes && G.JokerCraftResult({NR, NS}) == EPDJoker::Star;
			const bool bBad = bNodes && G.JokerCraftResult({NF, NS}) == EPDJoker::Hatter && G.JokerCraftResult({NF, NF}) == EPDJoker::Count && G.JokerCraftResult({NF}) == EPDJoker::Count;
			const bool bOutside = bNodes && !G.JokerCraft({NF, NR, NS}, CellKey(3, Row));
			const bool bMade = bNodes && G.JokerCraft({NF, NR, NS}, CellKey(7, Row));
			Expect(bTwo && bBad && bOutside && bMade && G.Towers.size() == 1 && G.Towers.at(CellKey(7, Row)).Joker == (int)EPDJoker::Star,
				"조커 만들기: 떨어진 플러시·로열·스트플을 골라 스타, 재료 밖 칸·같은 재료 두 번은 거절");
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
			Put(G.Towers, 0, Row, C(14, Di), 2); G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::RoyalFlush;
			Put(G.Towers, 4, Row, C(3, He), 2); G.Towers[CellKey(4, Row)].FuseHand = (int)EPDHand::StraightFlush;
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
			Put(G.Towers, 6, Row, C(3, He), 2); G.Towers[CellKey(6, Row)].FuseHand = (int)EPDHand::StraightFlush;
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			int NR = -1, NP = -1, NS = -1;
			for (int I = 0; I < (int)Ns.size(); ++I) { if (Ns[I].Kind == EPDHand::RoyalFlush) NR = I; if (Ns[I].Kind == EPDHand::Pair) NP = I; if (Ns[I].Kind == EPDHand::StraightFlush) NS = I; }
			const bool bStar = NR >= 0 && NS >= 0 && G.JokerCraftResult({NR, NS}) == EPDJoker::Star;
			const bool bClash = NR >= 0 && NP >= 0 && FPDGame::JokerNodesClash(Ns[NR], Ns[NP]);
			Expect(NP >= 0 && bStar && bClash, "원 페어에 든 로열 합성 타워도 로열로 재료 (스타), 그 원 페어와는 같이 못 넣음");
		}
		{ // 조커 목록 (D-210): 가장 좋은 재료, 모자란 족보
			G.Towers.clear();
			Put(G.Towers, 0, Row, C(4), 2); G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Flush;
			Put(G.Towers, 3, Row, C(5), 2); G.Towers[CellKey(3, Row)].FuseHand = (int)EPDHand::Flush; G.Towers[CellKey(3, Row)].Mult = 9.f;
			Put(G.Towers, 7, Row, C(14, Di), 2); G.Towers[CellKey(7, Row)].FuseHand = (int)EPDHand::RoyalFlush;
			Put(G.Towers, 12, Row, C(3, He), 2); G.Towers[CellKey(12, Row)].FuseHand = (int)EPDHand::StraightFlush;
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<FPDJokerNode> Ns = G.JokerNodes();
			const std::vector<int> B = G.BestJokerPicks(EPDJoker::Star);
			bool bHasBig = false;
			for (int N : B) bHasBig = bHasBig || (Ns[N].Fused == CellKey(3, Row));
			EPDHand Miss = EPDHand::Count;
			const int MissBell = G.JokerMissing(EPDJoker::Bell, &Miss);
			Expect(B.size() == 3 && G.JokerCraftResult(B) == EPDJoker::Star && bHasBig && G.JokerMissing(EPDJoker::Star) == 0 && MissBell == 2,
				"조커 목록: 스타 재료 3개를 고르고 배율 큰 플러시(5♠ ×9)를 쓴다, 방울은 풀하우스·스트레이트가 모자람");
			G.Towers.erase(CellKey(0, Row)); G.Towers.erase(CellKey(3, Row));
			G.Synergies = ApplyEffects(G.Towers, 0);
			const std::vector<int> B2 = G.BestJokerPicks(EPDJoker::Star);
			const int MissStar = G.JokerMissing(EPDJoker::Star, &Miss);
			Expect(B2.size() == 2 && MissStar == 1 && Miss == EPDHand::Flush, "플러시가 없으면 스타는 2개 레시피, 3개 레시피엔 플러시 하나가 모자람");
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
		// 조커도 고른 칸에 (스트레이트 셋 → 지휘봉을 맨 왼쪽에)
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, He)); Put(G.Towers, 2, Row, C(4)); Put(G.Towers, 3, Row, C(6, He)); Put(G.Towers, 4, Row, C(7)); Put(G.Towers, 5, Row, C(8, He)); Put(G.Towers, 6, Row, C(10)); Put(G.Towers, 7, Row, C(11, He)); Put(G.Towers, 8, Row, C(12));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bJBad = G.JokerFuse(0, CellKey(9, Row));
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
		for (auto& E : G.Enemies) bStop = bStop && E.SlowAmt == 1.f && E.SlowT > 0.f;
		Expect(bStop, "방울: 사거리 안 적을 멈춘다");
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
		{ Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, He)); Put(G.Towers, 2, Row, C(4)); Put(G.Towers, 3, Row, C(6, He)); Put(G.Towers, 4, Row, C(7)); Put(G.Towers, 5, Row, C(8, He)); Put(G.Towers, 6, Row, C(10)); Put(G.Towers, 7, Row, C(11, He)); Put(G.Towers, 8, Row, C(12)); }
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		Expect(!G.JokerGroups().empty(), "조커 재료 있음");
		G.TreeOwned = Bits({FindKey(EPDKeystone::Smith)});
		G.RebuildMods();
		Expect(G.JokerGroups().empty() && std::abs(G.FuseMult() - 2.f) < 1e-4f, "대장장이: 조커 못 만듦, 합성 배율 2.0");
		G.TreeOwned = Bits({FindKey(EPDKeystone::ClownPath)});
		for (auto& KV : G.Towers) KV.second.Level = 2;
		G.RebuildMods();
		Expect(!G.CanFuse(0) && std::abs(G.JokerMult() - 1.6f) < 1e-4f, "광대의 길: 레벨 2는 더 합성 못 함, 조커 배율 1.6");
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

// ---------------------------------------------------------------- 합성 규칙 상황별 (D-480)
static FPDGame FuseBoard()
{
	FPDGame G(42);
	G.NewGame(true);
	G.bMuted = true;
	G.Towers.clear();
	G.Synergies.clear();
	return G;
}
static void Refresh(FPDGame& G) { G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods); }
static int FindSyn(const FPDGame& G, EPDHand Kind, int Level = -1)
{
	for (int I = 0; I < (int)G.Synergies.size(); ++I)
		if (G.Synergies[I].Kind == Kind && (Level < 0 || G.Towers.at(G.Synergies[I].Cells[0]).Level == Level)) return I;
	return -1;
}
static bool Near(float A, float B) { return std::fabs(A - B) < 1e-4f; }
static int PDCellDistT(int A, int B) { return std::abs(A % Cols - B % Cols) + std::abs(A / Cols - B / Cols); }
static void FuseRuleTests()
{
	auto FindKey = [](EPDKeystone K) { for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && TreeNode(I).Fx[0].Param == (int)K) return I; return -1; };
	auto Bits = [](std::initializer_list<int> Ids) { FPDTreeBits B(1); for (int I : Ids) B.set(I); return B; };
	std::printf("\n[합성 규칙 상황별]\n");
	{ // 1. 다시 합성하면 특성이 곱해지고 위력 배율도 (합 × 1.5)
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, EPDSuit::Heart));
		Refresh(G);
		const bool bA = G.Fuse(FindSyn(G, EPDHand::Pair, 1), CellKey(1, Row));
		// Lv2 9 옆에 Lv1 9 두 장: 레벨이 달라 Lv1 페어만 잡힌다 → Lv2 9 옆 칸으로 합성
		Put(G.Towers, 2, Row, C(9, EPDSuit::Diamond)); Put(G.Towers, 3, Row, C(9, EPDSuit::Club));
		Refresh(G);
		const bool bB = G.Synergies.size() == 1 && G.Fuse(FindSyn(G, EPDHand::Pair, 1), CellKey(2, Row));
		const int P2 = FindSyn(G, EPDHand::Pair, 2);
		const bool bC = P2 >= 0 && G.Fuse(P2, CellKey(1, Row));
		const FPDTower& F = G.Towers.at(CellKey(1, Row));
		Expect(bA && bB && bC && G.Towers.size() == 1 && F.Level == 3 && Near(F.Mult, (3.f + 3.f) * FuseBonus) && Near(F.TDmg, 1.5f * 1.5f),
			"Lv2 페어 둘 → Lv3: 위력 (3+3)×1.5 = 9, 공격력 특성 1.5×1.5");
	}
	{ // 2. 서로 다른 특성을 가진 Lv2 둘을 합성: 특성은 종류마다 큰 쪽 × 새 족보, 범위는 큰 쪽, 기억 족보는 높은 쪽
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 8);
		FPDTower A; A.Card = C(7); A.Level = 2; A.Mult = 4.5f; A.TSplash = 1.25f; A.FuseHand = (int)EPDHand::Flush; A.X = 0; A.Y = Row;
		FPDTower B; B.Card = C(7, EPDSuit::Heart); B.Level = 2; B.Mult = 4.5f; B.TRate = 1.5f; B.FuseHand = (int)EPDHand::Straight; B.X = 1; B.Y = Row;
		G.Towers[CellKey(0, Row)] = A; G.Towers[CellKey(1, Row)] = B;
		Refresh(G);
		const bool bOk = G.Fuse(FindSyn(G, EPDHand::Pair, 2), CellKey(0, Row));
		const FPDTower& F = G.Towers.at(CellKey(0, Row));
		Expect(bOk && F.Level == 3 && Near(F.TDmg, 1.5f) && Near(F.TRate, 1.5f) && Near(F.TSplash, 1.25f) && F.FuseHand == (int)EPDHand::Flush && Near(F.Mult, 9.f * FuseBonus),
			"플러시 Lv2 + 스트레이트 Lv2 페어 → Lv3: 연사·범위 물려받고 공격력 ×1.5, 기억은 플러시");
	}
	{ // 3. Lv3 → Lv4, Lv4 끼리는 족보는 되지만 합성은 막힘 (최고 레벨)
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(5), 3); Put(G.Towers, 1, Row, C(5, EPDSuit::Heart), 3);
		Refresh(G);
		const bool b4 = G.Fuse(0, CellKey(0, Row));
		const bool bLv4 = b4 && G.Towers.at(CellKey(0, Row)).Level == MaxLevel;
		Put(G.Towers, 1, Row, C(5, EPDSuit::Diamond), MaxLevel);
		Refresh(G);
		const int P = FindSyn(G, EPDHand::Pair, MaxLevel);
		Expect(bLv4 && P >= 0 && G.FuseBlock(P) == FPDGame::EFuseBlock::TopLevel && !G.Fuse(P) && G.Towers.size() == 2,
			"Lv3 페어 → Lv4, Lv4 페어는 족보로 남고 합성은 '최고 레벨'로 막힘");
	}
	{ // 4. 레벨이 섞인 줄: Lv1 · Lv2 · Lv1 은 족보 없음, Lv2 옆 Lv1 둘은 Lv1 페어만
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(8)); Put(G.Towers, 1, Row, C(8, EPDSuit::Heart), 2); Put(G.Towers, 2, Row, C(8, EPDSuit::Diamond));
		Refresh(G);
		const bool bNone = G.Synergies.empty();
		Put(G.Towers, 3, Row, C(8, EPDSuit::Club));
		Refresh(G);
		bool bOnlyLv1 = G.Synergies.size() == 1 && G.Synergies[0].Kind == EPDHand::Pair;
		for (int K : G.Synergies.empty() ? std::vector<int>{} : G.Synergies[0].Cells) bOnlyLv1 = bOnlyLv1 && G.Towers.at(K).Level == 1;
		Expect(bNone && bOnlyLv1, "Lv1·Lv2·Lv1 줄은 족보 없음, 오른쪽에 Lv1 하나 더 → Lv1 페어만 (Lv2 는 안 낌)");
	}
	{ // 5. 같은 칸·카드에서 레벨만 바뀌어도 족보를 다시 찾는다 (덩어리 캐시 D-142)
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(10)); Put(G.Towers, 1, Row, C(10, EPDSuit::Heart));
		Refresh(G);
		const bool bLv1 = G.Synergies.size() == 1;
		G.Towers.at(CellKey(1, Row)).Level = 2;
		Refresh(G);
		const bool bMixed = G.Synergies.empty();
		G.Towers.at(CellKey(0, Row)).Level = 2;
		Refresh(G);
		const bool bLv2 = G.Synergies.size() == 1 && G.Towers.at(G.Synergies[0].Cells[0]).Level == 2;
		Expect(bLv1 && bMixed && bLv2, "캐시: 같은 칸·카드라도 Lv1 페어 → 레벨 섞임 없음 → Lv2 페어");
	}
	{ // 6. 조커가 서로 다른 레벨을 잇는다: 레벨이 섞인 족보는 생기지 않고, 조커 족보는 합성 불가
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(6)); Put(G.Towers, 2, Row, C(6, EPDSuit::Heart), 2);
		FPDTower J; J.Joker = (int)EPDJoker::Clown; J.X = 1; J.Y = Row; J.Card = {0, EPDSuit::Wild};
		G.Towers[CellKey(1, Row)] = J;
		Refresh(G);
		bool bNoMix = !G.Synergies.empty(), bNoFuse = true;
		for (int I = 0; I < (int)G.Synergies.size(); ++I)
		{
			int L = -1;
			for (int K : G.Synergies[I].Cells)
			{
				const FPDTower& T = G.Towers.at(K);
				if (T.IsJoker()) continue;
				if (L < 0) L = T.Level; else bNoMix = bNoMix && L == T.Level;
			}
			bNoFuse = bNoFuse && !G.CanFuse(I) && G.FuseBlock(I) == FPDGame::EFuseBlock::Joker;
		}
		Expect(bNoMix && bNoFuse && !G.Fuse(0) && G.Towers.size() == 3, "Lv1 6 · 조커 · Lv2 6: 족보마다 레벨 하나, 조커 족보는 합성 막힘 (까닭 = 조커)");
	}
	{ // 7. 카드를 나눠 쓰는 족보 (D-097): 5 6 7 + 7 → 스트레이트를 합성하면 남은 7 은 혼자 Lv1
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(5)); Put(G.Towers, 1, Row, C(6, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(7, EPDSuit::Heart));
		Refresh(G);
		const int St = FindSyn(G, EPDHand::Straight), Pr = FindSyn(G, EPDHand::Pair);
		const bool bBoth = St >= 0 && Pr >= 0;
		const bool bOk = bBoth && G.Fuse(St, CellKey(2, Row));
		const FPDTower& F = G.Towers.at(CellKey(2, Row));
		Expect(bOk && G.Towers.size() == 2 && F.Level == 2 && F.Card == C(7) && Near(F.TRate, 1.5f) && G.Towers.at(CellKey(3, Row)).Level == 1 && G.Synergies.empty(),
			"5 6 7 + 7: 스트레이트를 7 칸에 합성 → Lv2 7(연사 ×1.5), 옆 Lv1 7 과는 족보 아님");
		FPDGame H = FuseBoard();
		Put(H.Towers, 0, Row, C(5)); Put(H.Towers, 1, Row, C(6, EPDSuit::Heart)); Put(H.Towers, 2, Row, C(7)); Put(H.Towers, 3, Row, C(7, EPDSuit::Heart));
		Refresh(H);
		const bool bP = H.Fuse(FindSyn(H, EPDHand::Pair), CellKey(2, Row));
		Expect(bP && H.Towers.size() == 3 && H.Towers.at(CellKey(2, Row)).Level == 2 && H.Synergies.empty(),
			"5 6 7 + 7: 페어를 7 칸에 합성 → 5 6 은 스트레이트를 잃고(3장부터, D-418) Lv2 7 은 안 낌");
	}
	{ // 8. 와일드끼리 합성하면 와일드 Lv2, 그 와일드 Lv2 는 Lv2 와 족보가 되고 합치면 숫자는 표결
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, {0, EPDSuit::Wild}); Put(G.Towers, 1, Row, {0, EPDSuit::Wild});
		Refresh(G);
		const bool bSyn = !G.Synergies.empty();
		const bool bW = bSyn && G.Fuse(0, CellKey(0, Row));
		const bool bWild = bW && G.Towers.at(CellKey(0, Row)).Card.IsWild() && G.Towers.at(CellKey(0, Row)).Level == 2;
		Put(G.Towers, 1, Row, C(12, EPDSuit::Club), 2);
		Refresh(G);
		const bool bPair = FindSyn(G, EPDHand::Pair, 2) >= 0;
		const bool bF = bPair && G.Fuse(FindSyn(G, EPDHand::Pair, 2), CellKey(0, Row));
		const FPDCard Got = bF ? G.Towers.at(CellKey(0, Row)).Card : FPDCard{};
		Expect(bWild && bF && Got == C(12, EPDSuit::Club) && G.Towers.at(CellKey(0, Row)).Level == 3, "와일드 둘 → 와일드 Lv2, + Q♣ Lv2 를 와일드 칸에 합성 → Q♣ Lv3");
	}
	{ // 9. 웨이브 중에도 합성, 게임 오버 · 제목에서는 안 됨
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		auto Setup = [&](FPDGame& X) { X.Towers.clear(); Put(X.Towers, 0, Row, C(4)); Put(X.Towers, 1, Row, C(4, EPDSuit::Heart)); Refresh(X); };
		Setup(G); G.Phase = EPDPhase::Wave;
		const bool bWave = G.Fuse(0);
		Setup(G); G.Phase = EPDPhase::Over;
		const bool bOver = G.Fuse(0) || G.CanFuse(0) || G.FuseBlock(0) != FPDGame::EFuseBlock::NotInPlay;
		Setup(G); G.Phase = EPDPhase::Reward;
		const bool bReward = G.Fuse(0);
		Expect(bWave && !bOver && !bReward, "웨이브 중 합성 됨, 게임 오버·보상 창에선 안 됨");
	}
	{ // 10. 이상한 번호 · 칸
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(4, EPDSuit::Heart));
		Refresh(G);
		FPDSynergy S;
		const bool bBad = G.Fuse(-1) || G.Fuse(1) || G.Fuse(99) || G.CanFuse(-1) || G.FuseSynergy(5, S) || G.Fuse(0, CellKey(4, Row));
		Expect(!bBad && G.Towers.size() == 2 && G.FuseBlock(-1) == FPDGame::EFuseBlock::NotInPlay, "음수·범위 밖 번호, 재료 밖 칸은 모두 실패하고 판 그대로");
	}
	{ // 11. 저장 · 불러오기에 합성 타워(레벨·위력·특성·기억 족보)가 그대로
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(3)); Put(G.Towers, 1, Row, C(4)); Put(G.Towers, 2, Row, C(5)); Put(G.Towers, 3, Row, C(6)); Put(G.Towers, 4, Row, C(7));
		Refresh(G);
		const int Fl = FindSyn(G, EPDHand::StraightFlush); // 스트레이트 플러시는 5장
		const bool bF = Fl >= 0 && G.Fuse(Fl, CellKey(1, Row));
		const FPDTower Before = G.Towers.at(CellKey(1, Row));
		FPDGame L(1);
		const bool bLoad = bF && L.LoadRun(G.SaveRun());
		const auto It = L.Towers.find(CellKey(1, Row));
		const bool bSame = bLoad && It != L.Towers.end() && It->second.Level == Before.Level && Near(It->second.Mult, Before.Mult) && Near(It->second.TDmg, Before.TDmg)
			&& Near(It->second.TRate, Before.TRate) && Near(It->second.TSplash, Before.TSplash) && It->second.FuseHand == Before.FuseHand && It->second.Card == Before.Card;
		Expect(bSame && Before.FuseHand == (int)EPDHand::StraightFlush, "스트레이트 플러시 합성 타워 저장 → 불러오기: 레벨·위력·특성·기억 족보 같음");
	}
	{ // 12. 자리바꿈으로 옮긴 합성 타워는 레벨 그대로, 같은 레벨 옆으로 가면 족보
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 8);
		Put(G.Towers, 0, Row, C(11), 2); Put(G.Towers, 4, Row, C(11, EPDSuit::Heart), 2); Put(G.Towers, 6, Row, C(11, EPDSuit::Club));
		G.Towers.at(CellKey(0, Row)).Mult = 3.f;
		Refresh(G);
		G.Swaps = 1;
		const bool bEmpty = G.Synergies.empty();
		const bool bSw = G.SwapTowers(CellKey(0, Row), CellKey(5, Row));
		const auto It = G.Towers.find(CellKey(5, Row));
		Expect(bEmpty && bSw && It != G.Towers.end() && It->second.Level == 2 && Near(It->second.Mult, 3.f) && FindSyn(G, EPDHand::Pair, 2) >= 0 && FindSyn(G, EPDHand::Pair, 1) < 0,
			"자리바꿈: Lv2 J 를 Lv2 J 옆으로 → Lv2 페어 (옆 Lv1 J 는 안 낌)");
	}
	{ // 13. 합성 타워 위에는 카드를 못 놓고, 보스 무늬 바꾸기는 Lv4 를 건너뛴다
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(2), 2); Put(G.Towers, 1, Row, C(2), MaxLevel);
		Refresh(G);
		G.SuitSwaps.clear();
		G.DealSuits(5);
		bool bLv4Kept = true;
		for (const FPDGame::FPDSuitSwap& Sw : G.SuitSwaps) bLv4Kept = bLv4Kept && Sw.Cell != CellKey(1, Row);
		Expect(!G.CanPlace(0, Row) && !G.CanPlace(1, Row) && bLv4Kept, "Lv2·Lv4 위에는 카드 못 놓음, 보스 무늬 바꾸기는 Lv4 제외");
	}
	{ // 14. 다섯 장 족보 합성: 스트레이트 5장 → 연사 ×2, 위력 5 × 1.5 / 플러시 5장 → 범위 1.75
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(4)); Put(G.Towers, 3, Row, C(5, EPDSuit::Heart)); Put(G.Towers, 4, Row, C(6));
		Refresh(G);
		const int St = FindSyn(G, EPDHand::Straight);
		const bool bS = St >= 0 && G.Synergies[St].N == 5 && G.Fuse(St, CellKey(2, Row));
		const FPDTower FS = bS ? G.Towers.at(CellKey(2, Row)) : FPDTower{};
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(2, EPDSuit::Club)); Put(G.Towers, 1, Row, C(9, EPDSuit::Club)); Put(G.Towers, 2, Row, C(4, EPDSuit::Club)); Put(G.Towers, 3, Row, C(12, EPDSuit::Club)); Put(G.Towers, 4, Row, C(6, EPDSuit::Club));
		Refresh(G);
		const int Fl = FindSyn(G, EPDHand::Flush);
		const bool bF = Fl >= 0 && G.Synergies[Fl].N == 5 && G.Fuse(Fl, CellKey(2, Row));
		const FPDTower FF = bF ? G.Towers.at(CellKey(2, Row)) : FPDTower{};
		Expect(bS && Near(FS.TRate, 2.f) && Near(FS.Mult, 5.f * FuseBonus) && bF && Near(FF.TSplash, 1.75f) && FF.Card == C(4, EPDSuit::Club),
			"스트레이트 5장 → 연사 ×2 · 위력 7.5, 플러시 5장 → 범위 1.75 · 대표 카드 4♣");
	}
	{ // 15. 대장장이 합성 배율 2.0 이 실제 합성에 걸린다, 광대의 길은 Lv2 → Lv3 를 막는다
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		G.TreeOwned = Bits({FindKey(EPDKeystone::Smith)});
		G.RebuildMods();
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(4, EPDSuit::Heart));
		Refresh(G);
		const bool bSm = G.Fuse(0) && Near(G.Towers.begin()->second.Mult, 2.f * 2.f);
		FPDGame H = FuseBoard();
		H.TreeOwned = Bits({FindKey(EPDKeystone::ClownPath)});
		H.RebuildMods();
		Put(H.Towers, 0, Row, C(4)); Put(H.Towers, 1, Row, C(4, EPDSuit::Heart));
		Refresh(H);
		const bool bL1 = H.Fuse(0, CellKey(0, Row));
		Put(H.Towers, 1, Row, C(4, EPDSuit::Diamond), 2);
		Refresh(H);
		const int P2 = FindSyn(H, EPDHand::Pair, 2);
		Expect(bSm && bL1 && P2 >= 0 && !H.Fuse(P2) && H.FuseBlock(P2) == FPDGame::EFuseBlock::ClownPath, "대장장이: 페어 합성 위력 2×2.0, 광대의 길: Lv1 → Lv2 는 되고 Lv2 → Lv3 는 막힘");
	}
	{ // 16. 합성 타워의 실제 세기: 사거리는 레벨마다 +10%, 공격력에 위력 배율과 특성이 곱해진다
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, EPDSuit::Heart));
		Refresh(G);
		FPDTower Lone = G.Towers.at(CellKey(0, Row));
		Lone.Eff = FPDEffects();
		const FPDTowerStats S1 = TowerStats(Lone);
		G.Fuse(0, CellKey(0, Row));
		FPDTower F = G.Towers.at(CellKey(0, Row));
		F.Eff = FPDEffects();
		const FPDTowerStats S2 = TowerStats(F);
		Expect(Near(S2.Range, S1.Range * (1.f + LevelRange)) && Near(S2.Dmg, S1.Dmg * 2.f * FuseBonus * 1.5f), "Lv2 9: 사거리 ×1.1, 공격력 = 카드 × 위력 3 × 특성 1.5");
	}
	{ // 17. 합성 타워가 같은 레벨 카드와 더 큰 족보를 만들고, 그걸 다시 합성: 위력·특성이 모두 이어진다
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 8);
		Put(G.Towers, 0, Row, C(8)); Put(G.Towers, 1, Row, C(8, EPDSuit::Heart));
		Put(G.Towers, 3, Row, C(2), 2); Put(G.Towers, 4, Row, C(2, EPDSuit::Heart), 2);
		Refresh(G);
		const bool bTwo = G.Synergies.size() == 2;
		const bool bF = G.Fuse(FindSyn(G, EPDHand::Pair, 1), CellKey(1, Row));
		Put(G.Towers, 2, Row, C(8, EPDSuit::Club), 2);
		Refresh(G);
		const int Tp = FindSyn(G, EPDHand::TwoPair, 2);
		const bool bTp = Tp >= 0 && G.Synergies[Tp].N == 4 && G.Fuse(Tp, CellKey(2, Row));
		const FPDTower F = bTp ? G.Towers.at(CellKey(2, Row)) : FPDTower{};
		Expect(bTwo && bF && bTp && G.Towers.size() == 1 && F.Level == 3 && Near(F.Mult, (3.f + 1.f + 1.f + 1.f) * FuseBonus) && Near(F.TDmg, 1.5f * 1.5f) && Near(F.TRange, 1.25f) && F.FuseHand == (int)EPDHand::TwoPair && F.Card == C(8, EPDSuit::Club),
			"Lv2 8(합성) · Lv2 8 · Lv2 2 2 → 투 페어 → Lv3 8♣: 위력 6×1.5, 공격력 1.5×1.5, 사거리 1.25, 기억 투 페어");
	}
	{ // 원스 모어 (D-484): 타워를 팔면 1장 뽑기 (예전 버리면 1장), 손패가 가득이면 안 뽑고, 칵테일이 없으면 안 뽑음
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		auto Try = [&](bool bCharm, bool bFull) {
			G.Towers.clear(); G.Hand.clear();
			G.Charms.clear(); if (bCharm) G.Charms.push_back(EPDCharm::Recycle);
			while (bFull && (int)G.Hand.size() < G.HandMax()) G.Hand.push_back(C(2));
			Put(G.Towers, 0, Row, C(9)); Refresh(G);
			const size_t Before = G.Hand.size();
			const bool bSold = G.SellTower(CellKey(0, Row));
			return bSold ? (int)G.Hand.size() - (int)Before : -99;
		};
		const int With = Try(true, false), Without = Try(false, false), Full = Try(true, true);
		G.Hand = {C(5)}; G.Charms = {EPDCharm::Recycle};
		G.Discard(0);
		Expect(With == 1 && Without == 0 && Full == 0 && G.Hand.empty(), "원스 모어: 팔면 손패 +1, 칵테일 없으면 0, 손패 가득이면 0, 버리기로는 안 뽑음");
	}
	{ // 개발자 모드 (D-485): 칵테일 켜기/끄기, 조커 놓기 (위력은 실제로 만든 것과 같은 식)
		FPDGame G = FuseBoard();
		const int Row = FreeRow(G, 6);
		const int L0 = G.Lives;
		const bool bOn = G.DevToggleCharm((int)EPDCharm::GlassCannon) && G.Has(EPDCharm::GlassCannon) && G.Lives == std::max(1, L0 - 3);
		const bool bOff = !G.DevToggleCharm((int)EPDCharm::GlassCannon) && !G.Has(EPDCharm::GlassCannon);
		for (int K = 0; K < 8; ++K) G.DevToggleCharm(K); // 칸 수(5) 제한 없이
		const bool bMany = (int)G.Charms.size() == 8 && !G.DevToggleCharm(-1) && !G.DevToggleCharm((int)EPDCharm::Count);
		// 재료 레벨 1~4 로 만든 조커 (D-486): 실제로 카드를 놓고 합성 · 조커 만들기를 한 결과와 같아야 한다 (희비극 · 잭)
		struct FReal
		{
			static FPDCard Card(EPDHand H, int X) { if (H == EPDHand::Flush) { const int R[3] = {2, 7, 11}; return {R[X], EPDSuit::Club}; } return H == EPDHand::Straight ? FPDCard{5 + X, (EPDSuit)(X % 2)} : FPDCard{9, (EPDSuit)X}; }
			static int N(EPDHand H) { return H == EPDHand::Pair ? 2 : 3; }
			static FPDTower Make(EPDHand H, int L) // 족보 H 를 실제 Fuse 로 레벨 L 까지 (같은 족보로 거듭)
			{
				FPDGame F = FuseBoard();
				for (int X = 0; X < N(H); ++X) { FPDTower T; if (L > 2) T = Make(H, L - 1); T.X = X; T.Y = 9; T.Card = Card(H, X); F.Towers[CellKey(X, 9)] = T; }
				Refresh(F);
				for (int I = 0; I < (int)F.Synergies.size(); ++I) if (F.Synergies[I].Kind == H && (int)F.Synergies[I].Cells.size() == N(H)) { F.Fuse(I, CellKey(0, 9)); return F.Towers.at(CellKey(0, 9)); }
				return FPDTower{};
			}
			static FPDTower Joker(EPDHand A, EPDHand B, EPDHand C, int L)
			{
				FPDGame G2 = FuseBoard();
				const EPDHand Hs[3] = {A, B, C}; const int X0[3] = {0, 5, 10};
				for (int I = 0; I < 3; ++I)
				{
					if (L == 1) for (int X = 0; X < N(Hs[I]); ++X) Put(G2.Towers, X0[I] + X, 9, Card(Hs[I], X));
					else { FPDTower T = Make(Hs[I], L); T.X = X0[I]; T.Y = 9; T.Card = {2 + 3 * I, (EPDSuit)I}; G2.Towers[CellKey(X0[I], 9)] = T; }
				}
				Refresh(G2);
				const auto Gs = G2.JokerGroups();
				for (int I = 0; I < (int)Gs.size(); ++I) if (Gs[I].SynergyIdx.size() + Gs[I].FusedCells.size() == 3 && G2.JokerFuse(I)) for (auto& KV : G2.Towers) if (KV.second.IsJoker()) return KV.second;
				return FPDTower{};
			}
		};
		bool bSame = true;
		std::string Why;
		for (int L = 1; L <= MaxLevel; ++L)
			for (int Pick = 0; Pick < 2; ++Pick)
			{
				const FPDTower R = Pick == 0 ? FReal::Joker(EPDHand::Pair, EPDHand::Straight, EPDHand::Flush, L) : FReal::Joker(EPDHand::Triple, EPDHand::Straight, EPDHand::Flush, L);
				const int JK = Pick == 0 ? (int)EPDJoker::Comedy : (int)EPDJoker::Jack;
				const FPDTower D = G.DevJokerTower(JK, L);
				const bool bOk = R.Joker == JK && std::fabs(R.Mult - D.Mult) < 1e-3f * std::max(1.f, R.Mult) && Near(R.TDmg, D.TDmg) && Near(R.TRate, D.TRate) && Near(R.TSplash, D.TSplash) && R.Level == D.Level;
				if (!bOk && Why.empty()) Why = " (어긋남: " + std::string(Pick ? "잭" : "희비극") + " 재료 Lv" + std::to_string(L) + " 실제 " + std::to_string(R.Mult) + " / 계산 " + std::to_string(D.Mult) + ")";
				bSame = bSame && bOk;
			}
		const bool bCards = G.DevJokerCards((int)EPDJoker::Comedy, 1) == 8 && G.DevJokerCards((int)EPDJoker::Comedy, 2) == 8 && G.DevJokerCards((int)EPDJoker::Comedy, 4) == 8 + 27 + 27;
		Expect(bSame && bCards, "재료 Lv1~4 로 만든 희비극 · 잭: 계산 = 실제 카드 놓고 합성 · 조커 만들기 결과, 카드 수 8 · 8 · 62" + Why);
		const bool bPlace = G.DevPlaceJoker(CellKey(0, Row), (int)EPDJoker::Bell, false) && G.Towers.at(CellKey(0, Row)).JLevel == 1 && G.DevPlaceJoker(CellKey(1, Row), (int)EPDJoker::Bell, true) && G.Towers.at(CellKey(1, Row)).JLevel == JokerUpLevel;
		const int U = UpperOf(EPDJoker::Hatter, EPDJoker::Cheshire);
		const bool bUpper = G.DevPlaceJoker(CellKey(3, Row), UpperBase + U, false, 3) && G.Towers.at(CellKey(3, Row)).JLevel == JokerUpLevel && G.Towers.at(CellKey(3, Row)).Level == 3
			&& Near(G.Towers.at(CellKey(3, Row)).Mult, (G.DevJokerTower((int)EPDJoker::Hatter, 3).Mult + G.DevJokerTower((int)EPDJoker::Cheshire, 3).Mult) * UpperBonus);
		Put(G.Towers, 4, Row, C(7));
		const bool bReplace = G.DevPlaceJoker(CellKey(4, Row), (int)EPDJoker::Jack, false) && G.Towers.at(CellKey(4, Row)).IsJoker();
		int JX, JY; float ND; NearPathCell(G, JX, JY, ND);
		const bool bPath = !G.DevPlaceJoker(CellKey(JX - 1 >= 0 && G.Path.IsPath(JX - 1, JY) ? JX - 1 : JX + 1, JY), (int)EPDJoker::Jack, false) && !G.DevPlaceJoker(CellKey(5, Row), 99, false);
		G.Phase = EPDPhase::Over;
		const bool bOver = !G.DevPlaceJoker(CellKey(5, Row), (int)EPDJoker::Jack, false);
		Expect(bOn && bOff && bMany && bPlace && bUpper && bReplace && bPath && bOver,
			"개발자 모드: 유리 대포 켜면 목숨 −3 · 끄기 · 칸 수 제한 없음, 조커 Lv1/Lv10, 재료 Lv3 다과회 = 재료 조커 둘 ×1.5, 타워 바꿈, 길 · 없는 번호 · 게임 오버는 안 됨");
	}
	{ // 18. 무작위 판 2,000개: 아무 족보나 아무 칸에 합성해도 규칙이 깨지지 않는다
		std::mt19937 Rng(2026);
		int Tried = 0, Done = 0, Bad = 0;
		std::string FirstBad;
		auto Fail = [&](const std::string& Why) { if (Bad++ == 0) FirstBad = Why; };
		for (int Board = 0; Board < 2000; ++Board)
		{
			FPDGame G = FuseBoard();
			if (Board % 3 == 1) { G.TreeOwned = Bits({FindKey(EPDKeystone::Smith)}); G.RebuildMods(); }
			if (Board % 3 == 2) { G.TreeOwned = Bits({FindKey(EPDKeystone::ClownPath)}); G.RebuildMods(); }
			// 5×4 칸에 카드 (작은 숫자 범위 · 무늬 2개로 족보가 잘 나오게), 레벨 1~4 섞어서, 가끔 와일드 · 조커
			for (int Y = 0; Y < 4; ++Y)
				for (int X = 0; X < 5; ++X)
				{
					if (!G.Buildable(X, Y) || Rng() % 5 == 0) continue;
					const int Roll = (int)(Rng() % 40);
					FPDCard Cd{2 + (int)(Rng() % 5), (EPDSuit)(Rng() % 2)};
					if (Roll == 0) Cd = {0, EPDSuit::Wild};
					const int Lv = Rng() % 3 == 0 ? 1 + (int)(Rng() % MaxLevel) : 1;
					Put(G.Towers, X, Y, Cd, Lv);
					if (Roll == 1) { FPDTower& T = G.Towers.at(CellKey(X, Y)); T.Joker = (int)EPDJoker::Clown; T.Card = {0, EPDSuit::Wild}; T.Level = 1; }
				}
			Refresh(G);
			for (int Step = 0; Step < 6 && !G.Synergies.empty(); ++Step)
			{
				// 족보마다: 조커 아닌 칸은 레벨이 하나
				for (const FPDSynergy& S : G.Synergies)
				{
					int L = -1;
					for (int K : S.Cells) { const FPDTower& T = G.Towers.at(K); if (T.IsJoker()) continue; if (L < 0) L = T.Level; else if (L != T.Level) Fail("레벨이 섞인 족보"); }
				}
				const int I = (int)(Rng() % G.Synergies.size());
				const FPDSynergy S = G.Synergies[I];
				const FPDGame::EFuseBlock Why = G.FuseBlock(I);
				bool bJoker = false;
				for (int K : S.Cells) bJoker = bJoker || G.Towers.at(K).IsJoker();
				const int Lv = bJoker ? -1 : G.Towers.at(S.Cells[0]).Level;
				const FPDGame::EFuseBlock Want = bJoker ? FPDGame::EFuseBlock::Joker : Lv >= MaxLevel ? FPDGame::EFuseBlock::TopLevel
					: (G.Mods.Key(EPDKeystone::ClownPath) && Lv >= 2) ? FPDGame::EFuseBlock::ClownPath : FPDGame::EFuseBlock::None;
				if (Why != Want) Fail("막힌 까닭이 다름 (족보 " + std::to_string((int)S.Kind) + ")");
				const int Target = Rng() % 4 == 0 ? -1 : S.Cells[Rng() % S.Cells.size()];
				float SumMult = 0.f;
				int MaxFH = (int)S.Kind;
				for (int K : S.Cells) { SumMult += G.Towers.at(K).Mult; const int FH = G.Towers.at(K).FuseHand; if (FH >= 0 && HandBaseValue((EPDHand)FH) > HandBaseValue((EPDHand)MaxFH)) MaxFH = FH; }
				const size_t Before = G.Towers.size();
				const FPDTowerMap Snapshot = G.Towers;
				++Tried;
				const bool bOk = G.Fuse(I, Target);
				if (bOk != (Want == FPDGame::EFuseBlock::None)) { Fail("합성 됨/안 됨이 까닭과 다름"); break; }
				if (!bOk)
				{
					if (G.Towers.size() != Before) Fail("실패한 합성이 판을 바꿈");
					break;
				}
				++Done;
				const int At = Target >= 0 ? Target : DefaultFuseCell(S.Cells);
				auto It = G.Towers.find(At);
				if (G.Towers.size() != Before - S.Cells.size() + 1) Fail("타워 수가 맞지 않음");
				if (It == G.Towers.end()) { Fail("합쳐질 칸에 타워가 없음"); break; }
				const FPDTower& F = It->second;
				if (F.Level != Lv + 1 || F.Level > MaxLevel) Fail("레벨이 +1 이 아님");
				if (!Near(F.Mult, SumMult * G.FuseMult())) Fail("위력 배율 ≠ 합 × 합성 배율");
				if (F.FuseHand != MaxFH) Fail("기억 족보가 가장 높은 족보가 아님");
				if (F.IsJoker()) Fail("합성 결과가 조커");
				const FPDTower& Src = Snapshot.at(At);
				if (!Src.Card.IsWild() && !(F.Card == Src.Card)) Fail("대표 카드가 고른 칸 카드가 아님");
				for (const auto& KV : Snapshot)
				{
					const bool bMat = std::find(S.Cells.begin(), S.Cells.end(), KV.first) != S.Cells.end();
					if (!bMat && (!G.Towers.count(KV.first) || G.Towers.at(KV.first).Level != KV.second.Level || !(G.Towers.at(KV.first).Card == KV.second.Card))) Fail("재료 아닌 타워가 바뀜");
					if (bMat && KV.first != At && G.Towers.count(KV.first)) Fail("재료 칸이 안 비워짐");
				}
				const std::vector<FPDSynergy> Fresh = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
				if (Fresh.size() != G.Synergies.size()) Fail("합성 뒤 족보 목록이 다시 찾은 것과 다름");
			}
		}
		Expect(Bad == 0 && Done > 1000, "무작위 판 2,000개 · 합성 " + std::to_string(Done) + "번 성공 / " + std::to_string(Tried) + "번 시도: 규칙 어긋남 " + std::to_string(Bad) + (Bad ? " (처음: " + FirstBad + ")" : ""));
	}
	{ // 19. 와일드 족보 판정 (D-480): 바로 구한 값 = 예전처럼 와일드마다 숫자·무늬를 다 넣어 본 값 (무작위 20,000 묶음)
		struct FOracle
		{
			static std::vector<EPDHand> Run(const std::vector<FPDCard>& Cards)
			{
				for (size_t W = 0; W < Cards.size(); ++W)
				{
					if (!Cards[W].IsWild()) continue;
					std::vector<EPDSuit> Suits;
					for (const FPDCard& Cd : Cards) if (!Cd.IsWild() && std::find(Suits.begin(), Suits.end(), Cd.Suit) == Suits.end()) Suits.push_back(Cd.Suit);
					if (Suits.empty()) Suits.push_back(EPDSuit::Spade);
					std::set<EPDHand> All;
					std::vector<FPDCard> Try = Cards;
					for (int R = 2; R <= 14; ++R) for (EPDSuit Su : Suits) { Try[W] = {R, Su}; for (EPDHand H : Run(Try)) All.insert(H); }
					return std::vector<EPDHand>(All.begin(), All.end());
				}
				return ClassifyHand(Cards);
			}
		};
		std::mt19937 Rng(398);
		int Diff = 0;
		for (int I = 0; I < 20000; ++I)
		{
			const int N = 2 + (int)(Rng() % 4), W = 1 + (int)(Rng() % std::min(N, 3));
			std::vector<FPDCard> Cards;
			const int Lo = 2 + (int)(Rng() % 9); // 숫자를 좁게 몰아 족보가 잘 나오게
			for (int K = 0; K < N - W; ++K) Cards.push_back({Rng() % 6 == 0 ? 14 : Lo + (int)(Rng() % 5), (EPDSuit)(Rng() % (Rng() % 2 ? 1 : 4))});
			for (int K = 0; K < W; ++K) Cards.insert(Cards.begin() + Rng() % (Cards.size() + 1), FPDCard{0, EPDSuit::Wild});
			Diff += ClassifyHand(Cards) != FOracle::Run(Cards);
		}
		Expect(Diff == 0, "와일드 든 족보 판정 20,000 묶음: 예전 방식과 다름 " + std::to_string(Diff));
	}
}

// ---------------------------------------------------------------- 조커 합치기 · 상위 조커 상황별 (D-481)
static void PutJ(FPDTowerMap& M, int X, int Y, EPDJoker J, int JLevel = 1, float Mult = 1.f, int Level = 1)
{
	FPDTower T;
	T.X = X; T.Y = Y;
	T.Card = {0, EPDSuit::Wild};
	T.Joker = (int)J;
	T.JLevel = JLevel;
	T.Mult = Mult;
	T.Level = Level;
	M[CellKey(X, Y)] = T;
}
static int FindMerge(const std::vector<FPDJokerMerge>& L, int Joker)
{
	for (int I = 0; I < (int)L.size(); ++I) if (L[I].Joker == Joker) return I;
	return -1;
}
static void JokerMergeTests()
{
	std::printf("\n[조커 합치기 · 상위 조커 상황별]\n");
	auto Board = []() { FPDGame G = FuseBoard(); G.UpperUnlocked = 0x3FFFFFFFu; return G; };
	{ // 1. 레시피 표: 30개, 서로 다른 기본 조커 둘, 겹치는 짝 없음, 조커마다 6개, 순서 무관
		std::set<std::pair<int, int>> Pairs;
		int Per[(int)EPDJoker::Count] = {}, Star = 0;
		bool bOk = true;
		for (int U = 0; U < UpperJokerCount; ++U)
		{
			const FPDUpperDef& D = UpperDef(U);
			bOk = bOk && D.A != D.B && (int)D.A < (int)EPDJoker::Count && (int)D.B < (int)EPDJoker::Count;
			Pairs.insert({std::min((int)D.A, (int)D.B), std::max((int)D.A, (int)D.B)});
			Per[(int)D.A]++; Per[(int)D.B]++;
			bOk = bOk && UpperOf(D.A, D.B) == U && UpperOf(D.B, D.A) == U;
			Star += D.Special != EPDUpperSpecial::None;
		}
		for (int J = 0; J < (int)EPDJoker::Count; ++J) bOk = bOk && Per[J] == 6;
		const bool bStarter = UpperDef(11).Special == EPDUpperSpecial::None && UpperDef(18).Special == EPDUpperSpecial::None; // 맛보기 2종은 일반 (D-333)
		Expect(bOk && Pairs.size() == 30 && bStarter && Star == 11 && UpperOf(EPDJoker::Clown, EPDJoker::Mask) < 0 && UpperOf(EPDJoker::Clown, EPDJoker::Clown) < 0,
			"레시피 30개: 짝 겹침 없음 · 조커마다 6개 · 순서 무관 · 맛보기 2종은 일반 (★ " + std::to_string(Star) + "종)");
	}
	{ // 2. 같은 조커 둘: 위력 합 × 1.3, 조커 레벨은 높은 쪽(경험치도 그쪽), 타워 레벨 · 특성은 큰 쪽
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Jack, 4, 3.f, 1); PutJ(G.Towers, 1, Row, EPDJoker::Jack, 7, 5.f, 3);
		G.Towers.at(CellKey(1, Row)).JXp = 12.f; G.Towers.at(CellKey(0, Row)).TRate = 1.5f;
		Refresh(G);
		const auto L = G.JokerMerges();
		const int I = FindMerge(L, (int)EPDJoker::Jack);
		const bool bOk = I >= 0 && L[I].Upper < 0 && G.MergeJokers(I, CellKey(0, Row));
		const FPDTower F = bOk ? G.Towers.at(CellKey(0, Row)) : FPDTower{};
		Expect(bOk && G.Towers.size() == 1 && Near(F.Mult, 8.f * JokerMergeBonus) && F.JLevel == 7 && Near(F.JXp, 12.f) && F.Level == 3 && Near(F.TRate, 1.5f) && F.Joker == (int)EPDJoker::Jack,
			"같은 잭 둘: 위력 (3+5)×1.3, 조커 레벨 7·경험치 12, 타워 레벨 3, 연사 특성 1.5");
	}
	{ // 3. 같은 조커 셋 ㄱ자 → 한 묶음, 대각선은 안 이어짐
		FPDGame G = Board();
		const int Row = FreeRow(G, 6), Row2 = Row + 1 < Rows && G.Buildable(1, Row + 1) && G.Buildable(3, Row + 1) ? Row + 1 : -1;
		PutJ(G.Towers, 0, Row, EPDJoker::Bell); PutJ(G.Towers, 1, Row, EPDJoker::Bell);
		if (Row2 >= 0) { PutJ(G.Towers, 1, Row2, EPDJoker::Bell); PutJ(G.Towers, 3, Row, EPDJoker::Bell); PutJ(G.Towers, 4, Row2, EPDJoker::Bell); }
		Refresh(G);
		const auto L = G.JokerMerges();
		size_t Three = 0; int Groups = 0;
		for (const auto& M : L) if (M.Upper < 0) { Groups++; Three = std::max(Three, M.Cells.size()); }
		const bool bMerge = Row2 >= 0 && G.MergeJokers(FindMerge(L, (int)EPDJoker::Bell));
		Expect(Row2 >= 0 && Groups == 1 && Three == 3 && bMerge && G.Towers.size() == 3 && G.JokerMerges().empty(), "방울 ㄱ자 셋은 한 묶음 (위력 ×1.3 한 번), 대각선 방울 둘은 안 합쳐짐");
	}
	{ // 4. 레시피 없는 짝 · 같은 칸 아닌 곳 · 이상한 번호
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Clown); PutJ(G.Towers, 1, Row, EPDJoker::Mask);
		PutJ(G.Towers, 3, Row, EPDJoker::Jack); PutJ(G.Towers, 4, Row, EPDJoker::Jack);
		Refresh(G);
		const auto L = G.JokerMerges();
		const bool bBad = G.MergeJokers(-1) || G.MergeJokers((int)L.size()) || G.MergeJokers(0, CellKey(0, Row));
		Expect(L.size() == 1 && L[0].Joker == (int)EPDJoker::Jack && !bBad && G.Towers.size() == 4, "광대 + 웃는 가면은 레시피 없음, 범위 밖 번호 · 재료 밖 칸은 실패");
	}
	{ // 5. 일반 상위 조커(험프티 덤프티 = 가면 + 희비극): 레벨 조건 없이, 위력 합 × 1.3, 조커 레벨 높은 쪽, 경험치 0, 도감에 남음
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Mask, 1, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Comedy, 3, 4.f);
		G.Towers.at(CellKey(1, Row)).JXp = 9.f;
		G.UpperFound = 0;
		Refresh(G);
		const auto L = G.JokerMerges();
		const int U = UpperOf(EPDJoker::Mask, EPDJoker::Comedy);
		const int I = FindMerge(L, UpperBase + U);
		const bool bOk = I >= 0 && L[I].Upper == U && G.MergeJokers(I, CellKey(1, Row));
		const FPDTower F = bOk ? G.Towers.at(CellKey(1, Row)) : FPDTower{};
		Expect(bOk && U == 11 && F.Joker == UpperBase + U && Near(F.Mult, 6.f * UpperBonusPlain) && F.JLevel == 3 && F.JXp == 0.f && (G.UpperFound & (1u << U)) && IsUpperJoker(F.Joker),
			"험프티 덤프티: 레벨 1·3 그대로 합쳐짐, 위력 (2+4)×1.3, 조커 레벨 3, 도감에 남음");
	}
	{ // 6. ★ 상위 조커(미친 다과회 = 모자 장수 + 체셔): 둘 다 조커 레벨 10, 위력 × 1.5
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		const int U = UpperOf(EPDJoker::Hatter, EPDJoker::Cheshire);
		PutJ(G.Towers, 0, Row, EPDJoker::Hatter, 10, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Cheshire, 9, 2.f);
		Refresh(G);
		const bool b9 = FindMerge(G.JokerMerges(), UpperBase + U) < 0;
		G.Towers.at(CellKey(1, Row)).JLevel = 10;
		const int I = FindMerge(G.JokerMerges(), UpperBase + U);
		const bool bOk = I >= 0 && G.MergeJokers(I);
		const FPDTower F = bOk ? G.Towers.begin()->second : FPDTower{};
		Expect(UpperDef(U).Special == EPDUpperSpecial::MadTea && b9 && bOk && Near(F.Mult, 4.f * UpperBonus), "미친 다과회(★): 레벨 10·9 는 안 되고 10·10 이면 위력 (2+2)×1.5");
	}
	{ // 7. 트리 끝자락(조커 강화) 재료는 조건 레벨 5 · 하나마다 +25%, 다과회 초대장은 조건 7 · +15%
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		const int U = UpperOf(EPDJoker::Hatter, EPDJoker::Cheshire);
		PutJ(G.Towers, 0, Row, EPDJoker::Hatter, 5, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Cheshire, 10, 2.f);
		Refresh(G);
		const bool bNo = FindMerge(G.JokerMerges(), UpperBase + U) < 0;
		G.Mods.JokerUp = 1u << (int)EPDJoker::Hatter;
		const int I = FindMerge(G.JokerMerges(), UpperBase + U);
		const bool bTip = I >= 0 && G.MergeJokers(I) && Near(G.Towers.begin()->second.Mult, 4.f * UpperBonus * (1.f + UpperTipBonus));
		FPDGame H = Board();
		PutJ(H.Towers, 0, Row, EPDJoker::Hatter, 7, 2.f); PutJ(H.Towers, 1, Row, EPDJoker::Cheshire, 7, 2.f);
		Refresh(H);
		const bool bNo7 = FindMerge(H.JokerMerges(), UpperBase + U) < 0;
		H.Mods.UpperPrep = 0.15f;
		const int I7 = FindMerge(H.JokerMerges(), UpperBase + U);
		const bool bPrep = I7 >= 0 && H.MergeJokers(I7) && Near(H.Towers.begin()->second.Mult, 4.f * UpperBonus * 1.15f);
		Expect(bNo && bTip && bNo7 && bPrep, "끝자락 찍은 모자 장수 Lv5 + 체셔 Lv10 → 다과회 (+25%), 다과회 초대장이면 Lv7 둘로 (+15%)");
	}
	{ // 8. 왕관의 무게 ×2.2 (일반 · ★ 모두), 서커스면 상위 조커 못 만들고 같은 조커 합치기는 됨, 잠긴 상위 조커는 안 뜸
		FPDGame G = Board();
		const int Row = FreeRow(G, 8);
		PutJ(G.Towers, 0, Row, EPDJoker::Mask, 1, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Comedy, 1, 2.f);
		PutJ(G.Towers, 4, Row, EPDJoker::Jack); PutJ(G.Towers, 5, Row, EPDJoker::Jack);
		Refresh(G);
		G.Mods.Keys = 1u << (int)EPDKeystone::Crown;
		const int I = FindMerge(G.JokerMerges(), UpperBase + 11);
		const bool bCrown = I >= 0 && G.MergeJokers(I, CellKey(0, Row)) && Near(G.Towers.at(CellKey(0, Row)).Mult, 4.f * 2.2f);
		FPDGame H = Board();
		PutJ(H.Towers, 0, Row, EPDJoker::Mask); PutJ(H.Towers, 1, Row, EPDJoker::Comedy);
		PutJ(H.Towers, 4, Row, EPDJoker::Jack); PutJ(H.Towers, 5, Row, EPDJoker::Jack);
		Refresh(H);
		H.Mods.Keys = 1u << (int)EPDKeystone::Circus;
		const auto LC = H.JokerMerges();
		const bool bCircus = LC.size() == 1 && LC[0].Upper < 0;
		H.Mods.Keys = 0;
		H.UpperUnlocked = ~(1u << 11);
		const auto LL = H.JokerMerges();
		const bool bLocked = LL.size() == 1 && LL[0].Upper < 0;
		Expect(bCrown && bCircus && bLocked, "왕관의 무게: 험프티 위력 4×2.2, 서커스: 같은 잭 합치기만, 잠긴 험프티는 안 뜸");
	}
	{ // 9. 상위 조커는 최종형: 기본 조커와 맞닿아도 상위 조커 안 됨, 할리퀸이 들면 할리퀸 무늬를 이어받음
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Hatter); PutJ(G.Towers, 1, Row, EPDJoker::Harlequin);
		G.Towers.at(CellKey(1, Row)).JSuit = EPDSuit::Club; G.Towers.at(CellKey(0, Row)).JSuit = EPDSuit::Heart;
		Refresh(G);
		const int U = UpperOf(EPDJoker::Hatter, EPDJoker::Harlequin);
		const int I = FindMerge(G.JokerMerges(), UpperBase + U);
		const bool bOk = I >= 0 && G.MergeJokers(I, CellKey(0, Row));
		const bool bSuit = bOk && G.Towers.at(CellKey(0, Row)).JSuit == EPDSuit::Club;
		PutJ(G.Towers, 1, Row, EPDJoker::Jack); // 하트 왕(모자 장수+할리퀸) 옆에 잭: 모자 장수 + 잭 = 하트 잭 레시피가 있어도 안 됨
		Refresh(G);
		Expect(bSuit && G.JokerMerges().empty(), "하트 왕: 할리퀸 ♣ 무늬를 이어받음, 상위 조커 옆 잭은 합칠 것 없음");
	}
	{ // 10. 상위 조커 결과 칸에 따라 타워 레벨(사거리)이 달라지면 안 된다 — 같은 조커 합치기 · 조커 만들기는 큰 쪽
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Mask, 1, 1.f, 3); PutJ(G.Towers, 1, Row, EPDJoker::Comedy, 1, 1.f, 1);
		Refresh(G);
		FPDGame H = G;
		const bool bA = G.MergeJokers(FindMerge(G.JokerMerges(), UpperBase + 11), CellKey(0, Row));
		const bool bB = H.MergeJokers(FindMerge(H.JokerMerges(), UpperBase + 11), CellKey(1, Row));
		const int LA = bA ? G.Towers.at(CellKey(0, Row)).Level : -1, LB = bB ? H.Towers.at(CellKey(1, Row)).Level : -1;
		Expect(bA && bB && LA == LB && LA == 3, "상위 조커 타워 레벨: 어느 칸에 합쳐도 큰 쪽 (가면 Lv3 + 희비극 Lv1 → " + std::to_string(LA) + " / " + std::to_string(LB) + ")");
	}
	{ // 11. 같은 조커 합치기와 상위 조커가 한 조커를 같이 노릴 때: 하나를 하면 목록이 새로 짜인다
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Mask, 1, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Mask, 1, 2.f); PutJ(G.Towers, 2, Row, EPDJoker::Comedy, 1, 2.f);
		Refresh(G);
		const auto L = G.JokerMerges();
		const bool bBoth = L.size() == 2 && FindMerge(L, (int)EPDJoker::Mask) >= 0 && FindMerge(L, UpperBase + 11) >= 0;
		const bool bSame = G.MergeJokers(FindMerge(L, (int)EPDJoker::Mask), CellKey(1, Row));
		const auto L2 = G.JokerMerges();
		const int I = FindMerge(L2, UpperBase + 11);
		const bool bUp = L2.size() == 1 && I >= 0 && G.MergeJokers(I, CellKey(1, Row));
		Expect(bBoth && bSame && bUp && G.Towers.size() == 1 && Near(G.Towers.begin()->second.Mult, (2.f * 2.f * JokerMergeBonus + 2.f) * UpperBonusPlain),
			"가면 · 가면 · 희비극: 가면 둘 합치고 → 희비극 옆이면 험프티, 위력 ((2+2)×1.3 + 2)×1.3");
	}
	{ // 12. 단계: 준비 · 웨이브 중엔 되고 게임 오버 · 보상 창에선 목록이 비고 합치기 실패
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		auto Setup = [&](FPDGame& X) { X.Towers.clear(); PutJ(X.Towers, 0, Row, EPDJoker::Jack); PutJ(X.Towers, 1, Row, EPDJoker::Jack); Refresh(X); };
		Setup(G); G.Phase = EPDPhase::Wave;
		const bool bWave = G.MergeJokers(0);
		Setup(G); G.Phase = EPDPhase::Over;
		const bool bOver = !G.JokerMerges().empty() || G.MergeJokers(0);
		Setup(G); G.Phase = EPDPhase::Reward;
		const bool bReward = !G.JokerMerges().empty() || G.MergeJokers(0);
		Expect(bWave && !bOver && !bReward, "웨이브 중 합치기 됨, 게임 오버 · 보상 창에선 안 됨");
	}
	{ // 13. 저장 · 불러오기: 상위 조커 번호 · 위력 · 조커 레벨 · 무늬가 그대로, 같은 상위 조커 둘은 맞닿으면 같은 조커 합치기
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Harlequin, 12, 3.f); PutJ(G.Towers, 1, Row, EPDJoker::Jack, 11, 2.f);
		G.Towers.at(CellKey(0, Row)).JSuit = EPDSuit::Diamond;
		Refresh(G);
		const int U = UpperOf(EPDJoker::Harlequin, EPDJoker::Jack);
		const bool bQ = G.MergeJokers(FindMerge(G.JokerMerges(), UpperBase + U), CellKey(0, Row));
		const FPDTower Before = G.Towers.at(CellKey(0, Row));
		FPDGame L(1);
		const bool bLoad = bQ && L.LoadRun(G.SaveRun());
		const auto It = L.Towers.find(CellKey(0, Row));
		const bool bSame = bLoad && It != L.Towers.end() && It->second.Joker == Before.Joker && Near(It->second.Mult, Before.Mult) && It->second.JLevel == 12 && It->second.JSuit == EPDSuit::Diamond;
		Expect(UpperDef(U).Special == EPDUpperSpecial::QueenHearts && bSame, "하트 여왕 저장 → 불러오기: 번호 · 위력 · 조커 레벨 12 · ♦ 그대로");
	}
	{ // 15. 바다코끼리와 목수는 ★ (D-482): 둘 다 조커 레벨 10, 위력 × 1.5
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		const int U = UpperOf(EPDJoker::Comedy, EPDJoker::Baton);
		PutJ(G.Towers, 0, Row, EPDJoker::Comedy, 9, 2.f); PutJ(G.Towers, 1, Row, EPDJoker::Baton, 10, 2.f);
		Refresh(G);
		const bool b9 = FindMerge(G.JokerMerges(), UpperBase + U) < 0;
		G.Towers.at(CellKey(0, Row)).JLevel = 10;
		const int I = FindMerge(G.JokerMerges(), UpperBase + U);
		const bool bOk = I >= 0 && G.MergeJokers(I);
		Expect(UpperDef(U).Special == EPDUpperSpecial::Walrus && b9 && bOk && Near(G.Towers.begin()->second.Mult, 4.f * UpperBonus), "바다코끼리와 목수(★): 레벨 9·10 은 안 되고 10·10 이면 위력 (2+2)×1.5");
	}
	{ // 16. 굴 잔치: 처치 수만큼 주변 8칸 연사 +10% (최대 5번), 주변 밖은 그대로
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 1, Row, EPDJoker::Comedy); G.Towers.at(CellKey(1, Row)).Joker = UpperBase + UpperOf(EPDJoker::Comedy, EPDJoker::Baton);
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 4, Row, C(3, EPDSuit::Heart));
		auto Rate = [&](int X) { Refresh(G); return G.Towers.at(CellKey(X, Row)).Eff.Rate; };
		const float R0 = Rate(0), Far0 = Rate(4);
		bool bSteps = true;
		for (int F = 1; F <= 7; ++F) { G.Towers.at(CellKey(1, Row)).Feast = std::min(F, WalrusFeastMax); bSteps = bSteps && Near(Rate(0), R0 * (1.f + WalrusFeastStep * std::min(F, WalrusFeastMax))); }
		Expect(bSteps && Near(Rate(4), Far0) && Near(R0, 1.7f), "굴 잔치: 옆 타워 연사 ×1.7 → 처치 5번에 ×1.7×1.5, 더 처치해도 그대로, 주변 밖은 안 바뀜");
	}
	{ // 17. 실제 웨이브: 바다코끼리와 목수가 처치하면 쌓이고(최대 5), 웨이브가 끝나면 0 으로
		FPDGame G = Board();
		int JX, JY; float NearD;
		NearPathCell(G, JX, JY, NearD);
		PutJ(G.Towers, JX, JY, EPDJoker::Comedy, 10, 400.f); G.Towers.at(CellKey(JX, JY)).Joker = UpperBase + UpperOf(EPDJoker::Comedy, EPDJoker::Baton);
		Refresh(G);
		G.StartWave();
		int MaxFeast = 0; bool bRateOk = true;
		for (int K = 0; K < 20000 && G.Phase == EPDPhase::Wave; ++K)
		{
			G.Update(0.05f);
			const auto It = G.Towers.find(CellKey(JX, JY));
			if (It != G.Towers.end()) MaxFeast = std::max(MaxFeast, It->second.Feast);
			bRateOk = bRateOk && (It == G.Towers.end() || It->second.Feast <= WalrusFeastMax);
		}
		const auto It = G.Towers.find(CellKey(JX, JY));
		Expect(G.Phase != EPDPhase::Wave && MaxFeast == WalrusFeastMax && bRateOk && It != G.Towers.end() && It->second.Feast == 0, "웨이브 1: 굴 잔치가 " + std::to_string(MaxFeast) + "까지 쌓이고 웨이브가 끝나면 0");
	}
	{ // 14. 무작위 판 3,000개: 조커만 놓고 아무 합치기나 해도 규칙이 깨지지 않는다
		std::mt19937 Rng(399);
		int Done = 0, Ups = 0, Bad = 0;
		std::string FirstBad;
		auto Fail = [&](const std::string& Why) { if (Bad++ == 0) FirstBad = Why; };
		for (int Board_ = 0; Board_ < 3000; ++Board_)
		{
			FPDGame G = Board();
			const int Key = (int)(Rng() % 6);
			if (Key == 1) G.Mods.Keys = 1u << (int)EPDKeystone::Crown;
			if (Key == 2) G.Mods.Keys = 1u << (int)EPDKeystone::Circus;
			if (Key == 3) G.Mods.JokerUp = (uint32_t)(Rng() & 0x3FF);
			if (Key == 4) G.Mods.UpperPrep = 0.15f;
			if (Rng() % 4 == 0) G.UpperUnlocked = (uint32_t)Rng() & 0x3FFFFFFFu;
			for (int Y = 0; Y < 3; ++Y)
				for (int X = 0; X < 4; ++X)
				{
					if (!G.Buildable(X, Y) || Rng() % 4 == 0) continue;
					PutJ(G.Towers, X, Y, (EPDJoker)(Rng() % 4 == 0 ? 6 : Rng() % (int)EPDJoker::Count), Rng() % 3 == 0 ? 10 + (int)(Rng() % 5) : 1 + (int)(Rng() % 10), 1.f + (float)(Rng() % 8), 1 + (int)(Rng() % 3));
				}
			Refresh(G);
			for (int Step = 0; Step < 8; ++Step)
			{
				const auto L = G.JokerMerges();
				// 목록이 규칙대로인지
				for (const auto& M : L)
				{
					if (M.Upper < 0)
					{
						if (M.Cells.size() < 2) Fail("같은 조커 묶음이 하나뿐");
						for (int K : M.Cells) if (!G.Towers.count(K) || G.Towers.at(K).Joker != M.Joker) Fail("같은 조커 묶음에 다른 조커");
					}
					else
					{
						if (M.Cells.size() != 2 || PDCellDistT(M.Cells[0], M.Cells[1]) != 1) Fail("상위 조커 재료가 맞닿은 둘이 아님");
						const FPDTower& A = G.Towers.at(M.Cells[0]); const FPDTower& B = G.Towers.at(M.Cells[1]);
						if (IsUpperJoker(A.Joker) || IsUpperJoker(B.Joker) || A.Joker == B.Joker) Fail("상위 조커 재료가 기본 조커 둘이 아님");
						if (UpperOf((EPDJoker)A.Joker, (EPDJoker)B.Joker) != M.Upper || M.Joker != UpperBase + M.Upper) Fail("레시피와 다른 상위 조커");
						if (!G.UpperOpen(M.Upper)) Fail("잠긴 상위 조커가 뜸");
						if (G.Mods.Key(EPDKeystone::Circus)) Fail("서커스인데 상위 조커가 뜸");
						if (A.JLevel < G.UpperNeedFor(M.Upper, A) || B.JLevel < G.UpperNeedFor(M.Upper, B)) Fail("조건 레벨 미달인데 뜸");
					}
				}
				if (L.empty()) break;
				const int I = (int)(Rng() % L.size());
				const FPDJokerMerge M = L[I];
				const int Target = Rng() % 3 == 0 ? -1 : M.Cells[Rng() % M.Cells.size()];
				float Sum = 0.f; int Tips = 0, Lv = 0;
				for (int K : M.Cells) { Sum += G.Towers.at(K).Mult; Tips += G.Mods.Up(G.Towers.at(K).Joker); Lv = std::max(Lv, G.Towers.at(K).JLevel); }
				const size_t Before = G.Towers.size();
				const FPDTowerMap Snap = G.Towers;
				if (!G.MergeJokers(I, Target)) { Fail("목록에 있는 합치기가 실패"); break; }
				++Done;
				const int At = Target >= 0 ? Target : DefaultFuseCell(M.Cells);
				if (G.Towers.size() != Before - M.Cells.size() + 1 || !G.Towers.count(At)) { Fail("타워 수 · 결과 칸이 맞지 않음"); break; }
				const FPDTower& F = G.Towers.at(At);
				float Want = Sum * JokerMergeBonus;
				if (M.Upper >= 0)
				{
					++Ups;
					const float Bonus = G.Mods.Key(EPDKeystone::Crown) ? 2.2f : UpperDef(M.Upper).Special == EPDUpperSpecial::None ? UpperBonusPlain : UpperBonus;
					Want = Sum * Bonus * (1.f + UpperTipBonus * Tips + G.Mods.UpperPrep);
					if (F.Joker != UpperBase + M.Upper || F.JXp != 0.f) Fail("상위 조커 번호 · 경험치");
				}
				else if (F.Joker != M.Joker) Fail("같은 조커 합치기 결과 종류가 바뀜");
				if (std::fabs(F.Mult - Want) > 1e-3f * std::max(1.f, Want)) Fail("위력이 식과 다름");
				if (F.JLevel != Lv) Fail("조커 레벨이 가장 높은 것이 아님");
				int TopLv = 0; for (int K : M.Cells) TopLv = std::max(TopLv, Snap.at(K).Level);
				if (F.Level != TopLv) Fail("타워 레벨이 가장 높은 것이 아님");
				for (const auto& KV : Snap)
				{
					const bool bMat = std::find(M.Cells.begin(), M.Cells.end(), KV.first) != M.Cells.end();
					if (!bMat && (!G.Towers.count(KV.first) || G.Towers.at(KV.first).Joker != KV.second.Joker || !Near(G.Towers.at(KV.first).Mult, KV.second.Mult))) Fail("재료 아닌 조커가 바뀜");
					if (bMat && KV.first != At && G.Towers.count(KV.first)) Fail("재료 칸이 안 비워짐");
				}
			}
		}
		Expect(Bad == 0 && Done > 3000 && Ups > 300, "무작위 조커 판 3,000개 · 합치기 " + std::to_string(Done) + "번 (상위 조커 " + std::to_string(Ups) + "번): 규칙 어긋남 " + std::to_string(Bad) + (Bad ? " (처음: " + FirstBad + ")" : ""));
	}
}

// ---------------------------------------------------------------- 먹이기 상황별 (D-483)
static int FindFeed(const std::vector<FPDFeed>& L, int Syn)
{
	for (int I = 0; I < (int)L.size(); ++I) if (L[I].Syn == Syn) return I;
	return -1;
}
static double XpTotal(int Level, float Xp) { double T = Xp; for (int L = 1; L < Level; ++L) T += JokerXpNeed(L); return T; } // 레벨 1 부터 모은 경험치 전체
static void FeedTests()
{
	std::printf("\n[먹이기 상황별]\n");
	auto Board = []() { FPDGame G = FuseBoard(); return G; };
	const float Unit = 13.f; // 흡수 단위: 조커 카드 숫자 10 + 3
	// 조커는 와일드라 카드 옆에 붙으면 족보 안에 든다 (잭 + 9 + 9 = 트리플). '맞닿았지만 안 든' 조커는 5장 족보처럼 더 낄 수 없을 때 생긴다
	// 줄 하나(가로 5칸) + 가운데 위아래 칸 + 오른쪽으로 둘 떨어진 칸이 비어 있는 곳. 칸은 (X0 + dx, Y)
	int X0 = 0;
	auto Spot = [&X0](const FPDGame& G, int& Y) {
		for (Y = 1; Y + 1 < Rows; ++Y)
			for (X0 = 0; X0 + 6 < Cols; ++X0)
			{
				bool bOk = G.Buildable(X0 + 2, Y - 1) && G.Buildable(X0 + 2, Y + 1) && G.Buildable(X0 + 6, Y);
				for (int X = 0; X < 5; ++X) bOk = bOk && G.Buildable(X0 + X, Y);
				if (bOk) return true;
			}
		return false;
	};
	auto Row5 = [&X0](FPDGame& G, int Y) { for (int X = 0; X < 5; ++X) Put(G.Towers, X0 + X, Y, C(2 + X, X % 2 ? EPDSuit::Heart : EPDSuit::Spade)); }; // 2 3 4 5 6 스트레이트
	auto Find5 = [](const FPDGame& G) { for (int I = 0; I < (int)G.Synergies.size(); ++I) if (G.Synergies[I].Kind == EPDHand::Straight && G.Synergies[I].N == 5) return I; return -1; };
	{ // 1. 기본: 스트레이트 5장 가운데 아래에 잭 — 조커는 못 끼고(5장) 맞닿아서 먹는다. 경험치 72 → Lv3 + 22, 카드는 사라지고 위력에 흡수 60%
		FPDGame G = Board();
		int Y; const bool bSpot = Spot(G, Y);
		Row5(G, Y); PutJ(G.Towers, X0 + 2, Y + 1, EPDJoker::Jack, 1, 2.f);
		Refresh(G);
		const int S5 = Find5(G);
		bool bNoJ = S5 >= 0;
		for (int K : S5 >= 0 ? G.Synergies[S5].Cells : std::vector<int>{}) bNoJ = bNoJ && !G.Towers.at(K).IsJoker();
		const auto L = G.FeedTargets();
		const int I = FindFeed(L, S5);
		const float Gain = JokerFeedAbsorb * (5.f + 6.f + 7.f + 8.f + 9.f) / Unit;
		const bool bList = I >= 0 && L[I].Joker == CellKey(X0 + 2, Y + 1) && Near(L[I].Xp, 72.f) && Near(L[I].Gain, Gain);
		const bool bFed = bList && G.Feed(S5);
		const auto It = G.Towers.find(CellKey(X0 + 2, Y + 1));
		Expect(bSpot && bNoJ && bFed && G.Towers.size() == 1 && It != G.Towers.end() && It->second.JLevel == 3 && Near(It->second.JXp, 22.f) && Near(It->second.Mult, 2.f + Gain),
			"2~6 스트레이트(조커 안 듦) 아래 잭: 경험치 72 → Lv3 + 22, 카드 다섯 사라짐, 위력 2 + 0.6×35/13");
	}
	{ // 3. 든 조커가 맞닿은 더 높은 조커보다 먼저, 고른 조커가 있으면 그 조커가 먹는다 (D-213)
		FPDGame G = Board();
		int Y; const bool bSpot = Spot(G, Y);
		Row5(G, Y);
		PutJ(G.Towers, X0 + 0, Y, EPDJoker::Clown, 2); // 2 대신 광대 (와일드) — 광대 3 4 5 6 스트레이트
		PutJ(G.Towers, X0 + 2, Y + 1, EPDJoker::Star, 50);
		Refresh(G);
		int S5 = -1;
		for (int K = 0; K < (int)G.Synergies.size(); ++K) if (G.Synergies[K].N == 5 && std::find(G.Synergies[K].Cells.begin(), G.Synergies[K].Cells.end(), CellKey(X0 + 0, Y)) != G.Synergies[K].Cells.end()) S5 = K;
		const auto L = G.FeedTargets();
		const int I = FindFeed(L, S5);
		const bool bInside = I >= 0 && L[I].Joker == CellKey(X0 + 0, Y);
		const auto L2 = G.FeedTargets(CellKey(X0 + 2, Y + 1));
		const int I2 = FindFeed(L2, S5);
		const bool bPick = I2 >= 0 && L2[I2].Joker == CellKey(X0 + 2, Y + 1) && G.Feed(S5, CellKey(X0 + 2, Y + 1)) && G.Towers.at(CellKey(X0 + 2, Y + 1)).JLevel == 50 && G.Towers.count(CellKey(X0 + 0, Y)) && G.Towers.at(CellKey(X0 + 0, Y)).JLevel == 2;
		Expect(bSpot && bInside && bPick, "광대 3 4 5 6 (광대가 듦, Lv2) + 아래 스타 Lv50: 기본은 광대, 스타를 고르면 스타가 먹고 광대는 그대로");
	}
	{ // 4. 맞닿은 조커 둘: 레벨 높은 쪽이 기본, 낮은 쪽을 고를 수 있음, 안 닿은 조커는 못 고름
		FPDGame G = Board();
		int Y; const bool bSpot = Spot(G, Y);
		Row5(G, Y);
		PutJ(G.Towers, X0 + 2, Y - 1, EPDJoker::Jack, 3); PutJ(G.Towers, X0 + 2, Y + 1, EPDJoker::Bell, 8); PutJ(G.Towers, X0 + 6, Y, EPDJoker::Star, 1);
		Refresh(G);
		const int S5 = Find5(G);
		const auto L = G.FeedTargets();
		const int I = FindFeed(L, S5);
		const bool bHigh = I >= 0 && L[I].Joker == CellKey(X0 + 2, Y + 1);
		const bool bFar = FindFeed(G.FeedTargets(CellKey(X0 + 6, Y)), S5) < 0 && !G.Feed(S5, CellKey(X0 + 6, Y));
		const bool bLow = G.Feed(S5, CellKey(X0 + 2, Y - 1)) && G.Towers.at(CellKey(X0 + 2, Y - 1)).JLevel > 3 && G.Towers.at(CellKey(X0 + 2, Y + 1)).JLevel == 8;
		Expect(bSpot && S5 >= 0 && bHigh && bFar && bLow, "스트레이트 위 잭 Lv3 · 아래 방울 Lv8: 기본은 방울, 잭을 고르면 잭, 떨어진 스타는 못 먹음");
	}
	{ // 5. 합성 타워를 먹이면 경험치(평균 위력) · 흡수(특성까지)가 크다, 조커 특성이 크면 흡수는 줄어든다
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(7), 2); Put(G.Towers, 1, Row, C(7, EPDSuit::Heart), 2); PutJ(G.Towers, 2, Row, EPDJoker::Jack);
		G.Towers.at(CellKey(0, Row)).Mult = 3.f; G.Towers.at(CellKey(0, Row)).TDmg = 1.5f;
		G.Towers.at(CellKey(1, Row)).Mult = 5.f; G.Towers.at(CellKey(1, Row)).TRate = 2.f;
		G.Towers.at(CellKey(2, Row)).TDmg = 2.f;
		Refresh(G);
		const auto L = G.FeedTargets();
		const int T = FindSyn(G, EPDHand::Triple);
		const int I = FindFeed(L, T);
		const float Val = (3.f + 7.f) * 3.f * 1.5f + (3.f + 7.f) * 5.f * 2.f;
		Expect(T >= 0 && I >= 0 && Near(L[I].Xp, 60.f * (3.f + 5.f) / 2.f) && Near(L[I].Gain, JokerFeedAbsorb * Val / (Unit * 2.f)), "잭 + Lv2 7 둘(위력 3·5, 특성 1.5·2) 트리플 → 경험치 60×4 = 240, 흡수 0.6×145/(13×조커 공격 특성 2)");
	}
	auto Trip = [](FPDGame& G, int Row, int JLevel = 1, int Lv = 1, float M = 1.f) {
		Put(G.Towers, 0, Row, C(9), Lv); Put(G.Towers, 1, Row, C(9, EPDSuit::Heart), Lv); PutJ(G.Towers, 2, Row, EPDJoker::Jack, JLevel);
		G.Towers.at(CellKey(0, Row)).Mult = M; G.Towers.at(CellKey(1, Row)).Mult = M;
		Refresh(G);
		return FindSyn(G, EPDHand::Triple);
	};
	{ // 6. 먹이 상자 부적: 경험치 ×1.5, 먹이 사슬 칵테일: 흡수 90%
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		G.Charms = {EPDCharm::Feeder, EPDCharm::FoodChain};
		const int T = Trip(G, Row);
		const auto L = G.FeedTargets();
		const int I = FindFeed(L, T);
		const bool bGain = I >= 0 && Near(L[I].Gain, 0.9f * 24.f / Unit);
		const bool bFed = G.Feed(T);
		const FPDTower& J = G.Towers.at(CellKey(2, Row));
		Expect(bGain && bFed && Near((float)XpTotal(J.JLevel, J.JXp), 90.f), "먹이 상자: 경험치 60×1.5 = 90, 먹이 사슬: 흡수 0.9×24/13");
	}
	{ // 7. 한 번에 여러 레벨, 최대 레벨 9999 에서 멈추고 경험치 0, 최대 레벨 조커는 먹이기 목록에서 빠짐
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		const int T = Trip(G, Row, 1, 2, 10.f); // 경험치 60 × 10 = 600 → 20+30+…+100 = 540 → Lv10, 남은 60
		const bool bFed = G.Feed(T);
		const FPDTower& J = G.Towers.at(CellKey(2, Row));
		bool bEv = false;
		for (const FPDEvent& E : G.Events) bEv = bEv || (E.Kind == FPDEvent::EKind::Feed && E.A == 10 && E.B == 9);
		const bool bMulti = bFed && J.JLevel == 10 && Near(J.JXp, 60.f) && bEv && G.Mods.Up(J);
		FPDGame H = Board();
		const int T2 = Trip(H, Row, JokerMaxLevel - 1);
		H.Towers.at(CellKey(2, Row)).JXp = JokerXpNeed(JokerMaxLevel - 1) - 5.f;
		const bool bCap = H.Feed(T2) && H.Towers.at(CellKey(2, Row)).JLevel == JokerMaxLevel && H.Towers.at(CellKey(2, Row)).JXp == 0.f;
		Put(H.Towers, 0, Row, C(4)); Put(H.Towers, 1, Row, C(4, EPDSuit::Heart));
		Refresh(H);
		const bool bGone = H.FeedTargets().empty() && !H.Feed(FindSyn(H, EPDHand::Triple));
		Expect(bMulti && bCap && bGone, "경험치 600 → Lv1 에서 Lv10 + 60 (사건 +9, 강화 능력), Lv9998 → 9999 에서 멈추고 경험치 0, Lv9999 는 못 먹음");
	}
	{ // 8. 조커만 있는 족보(조커 둘 = 와일드 페어)는 먹일 것이 없다, 단계(오버 · 보상)에서는 안 됨
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		PutJ(G.Towers, 0, Row, EPDJoker::Jack); PutJ(G.Towers, 1, Row, EPDJoker::Bell);
		Refresh(G);
		const bool bNone = !G.Synergies.empty() && G.FeedTargets().empty() && !G.Feed(0);
		G.Towers.clear();
		const int T = Trip(G, Row);
		G.Phase = EPDPhase::Over;
		const bool bOver = G.FeedTargets().empty() && !G.Feed(T);
		G.Phase = EPDPhase::Reward;
		const bool bReward = G.FeedTargets().empty() && !G.Feed(T);
		G.Phase = EPDPhase::Wave;
		const bool bWave = G.Feed(T);
		Expect(bNone && bOver && bReward && bWave && G.Towers.size() == 1, "조커 둘 와일드 페어는 먹이 없음, 게임 오버 · 보상 창에선 안 되고 웨이브 중엔 됨");
	}
	{ // 9. 카드를 나눠 쓰는 족보: 5 6 7 + 7 의 스트레이트를 먹이면 남은 7 은 그대로, 페어는 사라짐
		FPDGame G = Board();
		const int Row = FreeRow(G, 6), Row2 = Row + 1 < Rows && G.Buildable(0, Row + 1) ? Row + 1 : -1;
		Put(G.Towers, 0, Row, C(5)); Put(G.Towers, 1, Row, C(6, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(7, EPDSuit::Heart));
		if (Row2 >= 0) PutJ(G.Towers, 0, Row2, EPDJoker::Jack);
		Refresh(G);
		const int St = FindSyn(G, EPDHand::Straight);
		const bool bFed = Row2 >= 0 && St >= 0 && G.Feed(St);
		Expect(bFed && G.Towers.size() == 2 && G.Towers.count(CellKey(3, Row)) && G.Synergies.empty(), "5 6 7 + 7: 스트레이트를 먹이면 7♥ 하나만 남고 페어는 사라짐");
	}
	{ // 10. 상위 조커도 먹는다, 고른 칸이 조커가 아니면 아무것도 없음
		FPDGame G = Board();
		const int Row = FreeRow(G, 6);
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, EPDSuit::Heart)); PutJ(G.Towers, 2, Row, EPDJoker::Jack, 12);
		G.Towers.at(CellKey(2, Row)).Joker = UpperBase + UpperOf(EPDJoker::Harlequin, EPDJoker::Jack);
		Refresh(G);
		const bool bCard = G.FeedTargets(CellKey(0, Row)).empty();
		const bool bUp = G.Feed(FindSyn(G, EPDHand::Triple)) && G.Towers.size() == 1 && Near((float)XpTotal(G.Towers.begin()->second.JLevel, G.Towers.begin()->second.JXp), (float)XpTotal(12, 0.f) + 60.f);
		Expect(bCard && bUp, "하트 여왕 Lv12 + 9 + 9 트리플: 상위 조커도 먹음 (경험치 +60), 카드 칸을 고르면 목록이 빔");
	}
	{ // 11. 무작위 판 3,000개: 아무 족보를 아무 조커에 먹여도 규칙이 깨지지 않는다
		std::mt19937 Rng(401);
		int Done = 0, Bad = 0;
		std::string FirstBad;
		auto Fail = [&](const std::string& Why) { if (Bad++ == 0) FirstBad = Why; };
		for (int B = 0; B < 3000; ++B)
		{
			FPDGame G = Board();
			if (B % 4 == 1) G.Charms = {EPDCharm::Feeder};
			if (B % 4 == 2) G.Charms = {EPDCharm::FoodChain};
			for (int Y = 0; Y < 4; ++Y)
				for (int X = 0; X < 5; ++X)
				{
					if (!G.Buildable(X, Y) || Rng() % 5 == 0) continue;
					if (Rng() % 5 == 0) { PutJ(G.Towers, X, Y, (EPDJoker)(Rng() % (int)EPDJoker::Count), Rng() % 20 == 0 ? JokerMaxLevel : 1 + (int)(Rng() % 30), 1.f + (float)(Rng() % 6)); G.Towers.at(CellKey(X, Y)).JXp = (float)(Rng() % 15); G.Towers.at(CellKey(X, Y)).TDmg = Rng() % 3 == 0 ? 1.5f : 1.f; continue; }
					Put(G.Towers, X, Y, {2 + (int)(Rng() % 5), (EPDSuit)(Rng() % 2)}, Rng() % 4 == 0 ? 2 : 1);
					FPDTower& T = G.Towers.at(CellKey(X, Y));
					if (T.Level == 2) { T.Mult = 3.f; T.TDmg = 1.5f; }
				}
			Refresh(G);
			std::vector<int> Jokers;
			for (const auto& KV : G.Towers) if (KV.second.IsJoker()) Jokers.push_back(KV.first);
			for (int Step = 0; Step < 6; ++Step)
			{
				const int Only = (!Jokers.empty() && Rng() % 2) ? Jokers[Rng() % Jokers.size()] : -1;
				const auto L = G.FeedTargets(Only);
				for (const FPDFeed& F : L)
				{
					const FPDSynergy& S = G.Synergies[F.Syn];
					if (!G.Towers.count(F.Joker) || !G.Towers.at(F.Joker).IsJoker()) { Fail("먹는 칸이 조커가 아님"); continue; }
					if (Only >= 0 && F.Joker != Only) Fail("고른 조커가 아닌 조커가 먹음");
					if (G.Towers.at(F.Joker).JLevel >= JokerMaxLevel) Fail("최대 레벨 조커가 목록에 있음");
					bool bInside = false, bTouch = false; int Fed = 0; float MultSum = 0.f;
					for (int K : S.Cells)
					{
						if (K == F.Joker) bInside = true;
						if (G.Towers.at(K).IsJoker()) continue;
						Fed++; MultSum += G.Towers.at(K).Mult;
						bTouch = bTouch || PDCellDistT(K, F.Joker) == 1;
					}
					if (!bInside && !bTouch) Fail("먹는 조커가 족보에 없고 맞닿지도 않음");
					if (Fed == 0) Fail("먹일 카드가 없는 족보가 목록에 있음");
					if (Fed > 0 && !Near(F.Xp, S.Value * MultSum / Fed)) Fail("경험치 ≠ 족보 점수 × 평균 위력");
					if (F.Gain <= 0.f) Fail("흡수가 0 이하");
				}
				if (L.empty()) break;
				const FPDFeed F = L[Rng() % L.size()];
				const FPDSynergy S = G.Synergies[F.Syn];
				const FPDTowerMap Snap = G.Towers;
				const FPDTower J0 = Snap.at(F.Joker);
				size_t JokerCount = 0; for (const auto& KV : Snap) JokerCount += KV.second.IsJoker();
				if (!G.Feed(F.Syn, Only)) { Fail("목록에 있는 먹이기가 실패"); break; }
				++Done;
				size_t After = 0; for (const auto& KV : G.Towers) After += KV.second.IsJoker();
				if (After != JokerCount) Fail("조커가 먹이로 사라짐");
				if (!G.Towers.count(F.Joker)) { Fail("먹은 조커가 사라짐"); break; }
				const FPDTower& J = G.Towers.at(F.Joker);
				if (!Near(J.Mult, J0.Mult + F.Gain)) Fail("위력 ≠ 전 위력 + 흡수");
				const double Want = std::min(XpTotal(J0.JLevel, J0.JXp) + F.Xp * (G.Has(EPDCharm::Feeder) ? 1.5 : 1.0), XpTotal(JokerMaxLevel, 0.f));
				const double Got = J.JLevel >= JokerMaxLevel ? XpTotal(JokerMaxLevel, 0.f) : XpTotal(J.JLevel, J.JXp);
				if (std::fabs(Got - Want) > 1e-2 * std::max(1.0, Want)) Fail("경험치가 사라지거나 늘어남");
				if (J.JXp < 0.f || (J.JLevel < JokerMaxLevel && J.JXp >= JokerXpNeed(J.JLevel))) Fail("남은 경험치가 다음 레벨보다 큼");
				for (const auto& KV : Snap)
				{
					const bool bFood = !KV.second.IsJoker() && std::find(S.Cells.begin(), S.Cells.end(), KV.first) != S.Cells.end();
					if (bFood && G.Towers.count(KV.first)) Fail("먹인 카드가 남음");
					if (!bFood && !G.Towers.count(KV.first)) Fail("먹이 아닌 타워가 사라짐");
					if (!bFood && KV.first != F.Joker && G.Towers.count(KV.first) && (G.Towers.at(KV.first).JLevel != KV.second.JLevel || !Near(G.Towers.at(KV.first).Mult, KV.second.Mult))) Fail("다른 타워가 바뀜");
				}
			}
		}
		Expect(Bad == 0 && Done > 3000, "무작위 판 3,000개 · 먹이기 " + std::to_string(Done) + "번: 규칙 어긋남 " + std::to_string(Bad) + (Bad ? " (처음: " + FirstBad + ")" : ""));
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
static double Power(FPDGame& G, FPDTowerMap& Tw)
{
	const FPDCharmBits M = G.CharmMask();
	ApplyEffects(Tw, M, &G.Mods);
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
// 레벨 곡선 (D-479, ./Tests/run.sh N level [모드]): 웨이브를 막은 뒤마다 레벨 · 번 경험치 · 시간을 모은다
struct FLvRow { int N = 0; double Lv = 0, Xp = 0, Time = 0, Ups = 0; };
static bool GLvTrack = false;
static std::map<int, FLvRow> GLvRows;
static int GBotContent = 0;  // 0 = 규칙 기본(기본 조커 전부 · 칵테일 기본 18), 1 = 처음 상태(조커 4종 · 상위 조커 맛보기 2종 · 칵테일 18), 2 = 전부 열림
static void BotTest(int Runs, bool bDumb, bool bNoReward = false, bool bFuse = false, bool bJoker = false, bool bTree = false, bool bUpg = false)
{
	double PointSum = 0, GoldSum = 0, ChipSum = 0, TimeSum = 0;
	GKeysTaken.clear();
	GUpgLv.clear();
	std::map<int, int> Hist;
	int WaveSum = 0;
	std::map<int, int> JokersMade, KindsMade;
	long Feeds = 0, JLvSum = 0, UppersMade = 0;
	for (int Run = 0; Run < Runs; ++Run)
	{
		const auto RunT0 = std::chrono::steady_clock::now();
		FPDGame G(1000 + Run);
		G.Mode = GBotMode;
		if (bUpg) for (int& U : G.Upg) U = GBotUpgLv; // 영구 강화 흉내: 8종 모두 같은 레벨 (기본 10)
		if (GBotContent == 1) { G.JokerUnlocked = JokerStarterMask; G.UpperUnlocked = (1u << 11) | (1u << 18); }
		if (GBotContent == 2) G.CharmUnlocked = ~FPDCharmBits(0);
		G.NewGame(true);
		G.RebuildMods();
		G.bMuted = true;
		std::mt19937 R(77 + Run);
		int LvBefore = G.Level;
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
				if (BestI < 0 || !G.MergeJokers(BestI)) break;
				UppersMade++;
			}
			// 조커 먹이기: 판 위 위력을 15% 넘게 깎지 않는 족보 중 경험치가 가장 많은 것 (비운 칸은 바로 아래에서 다시 채움)
			for (int Tries = 0; bJoker && Tries < 3 && !G.Hand.empty(); ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				int BestS = -1;
				float BestXp = 0.f;
				for (const FPDFeed& Fd : G.FeedTargets())
				{
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
				for (int I = 0; I < (int)G.Hand.size(); ++I)
					for (auto& F : Cand)
					{
						FPDTowerMap Tw = G.Towers;
						FPDTower T;
						T.Card = G.Hand[I];
						T.X = F[0];
						T.Y = F[1];
						Tw[CellKey(F[0], F[1])] = T;
						const double V = Power(G, Tw);
						if (V > BestV) { BestV = V; BI = I; BX = F[0]; BY = F[1]; }
					}
				if (BI < 0) break;
				G.Place(BI, BX, BY);
			}
			// 조커: 만들면 전체 위력이 오르는 묶음을 만든다
			for (int Tries = 0; bJoker && Tries < 5; ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				const auto Groups = G.JokerGroups();
				int BestI = -1;
				double BestGain = 0.0;
				int BestCell = -1;
				for (int I = 0; I < (int)Groups.size(); ++I)
				{
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
							if (U >= 0 && G.UpperOpen(U) && UpperDef(U).Special == EPDUpperSpecial::None) bNextToPartner = true;
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
				G.JokerFuse(BestI, BestCell);
				JokersMade[0]++;
				for (auto& KV2 : G.Towers) if (KV2.second.IsJoker() && KV2.second.Age == 0.f) KindsMade[KV2.second.Joker]++;
			}
			for (int Tries = 0; bFuse && Tries < 20; ++Tries)
			{
				FPDTowerMap Cur = G.Towers;
				const double Before = Power(G, Cur);
				int BestI = -1;
				double BestGain = 0.0;
				for (int I = 0; I < (int)G.Synergies.size(); ++I)
				{
					if (!G.CanFuse(I)) continue;
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
			if (G.Phase == EPDPhase::Reward) // 보상: 부적 먼저, 없으면 수리 (bNoReward면 건너뛰기)
			{
				int Pick = -1;
				for (int I = 0; I < (int)G.Rewards.size() && !bNoReward; ++I) if (G.Rewards[I].bCharm) { Pick = I; break; }
				for (int I = 0; I < (int)G.Rewards.size() && Pick < 0 && !bNoReward; ++I) if (!G.Rewards[I].bCharm && G.Rewards[I].Id == (int)EPDInstant::Repair) Pick = I;
				G.TakeReward(Pick);
			}
			if (GLvTrack && G.Phase != EPDPhase::Over)
			{
				FLvRow& Row = GLvRows[G.Wave];
				Row.N++; Row.Lv += G.Level; Row.Xp += G.XpEarned; Row.Time += G.Time; Row.Ups += G.Level - LvBefore;
				LvBefore = G.Level;
			}
			G.Events.clear();
		}
		if (std::getenv("PD_BOT_VERBOSE")) std::fprintf(stderr, "  [판 %d] 시드 %d · 웨이브 %d · %s · %.1f초\n", Run, 1000 + Run, G.Wave, G.Phase == EPDPhase::Clear ? "클리어" : G.Phase == EPDPhase::Over ? "패배" : "멈춤(Guard)", std::chrono::duration<double>(std::chrono::steady_clock::now() - RunT0).count());
		{ int Mx = 0; for (auto& KV : G.Towers) if (KV.second.IsJoker()) Mx = std::max(Mx, KV.second.JLevel); JLvSum += Mx; }
		Hist[G.Wave]++;
		GBotClears += G.Phase == EPDPhase::Clear;
		WaveSum += G.Wave;
		PointSum += G.Points;
		GoldSum += G.XpEarned;
		ChipSum += G.RunChips(); // 판당 칩 (D-320 해금 속도 계산)
		TimeSum += G.Time;     // 웨이브 진행 시간 합 (D-322 플레이 시간 어림)
		for (int U = 0; U < (int)EPDUpgrade::Count; ++U) GUpgLv[U] += G.Upg[U];
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
	FuseRuleTests();
	JokerMergeTests();
	FeedTests();
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
	else if (Argc > 2 && std::string(Argv[2]) == "jokerlv") // 재료 레벨별 조커 세기 표 (D-486): ./Tests/run.sh 0 jokerlv [upper]
	{
		static const char* JN[] = {"광대", "모자 장수", "웃는 가면", "희비극", "할리퀸", "지휘봉", "잭", "체셔", "방울", "스타"};
		FPDGame G(42);
		G.NewGame(true);
		const bool bUpper = Argc > 3 && std::string(Argv[3]) == "upper";
		std::printf("\n[재료 레벨별 조커 세기] 조커 하나만 (판 족보 · 오라 · 칵테일 · 트리 없음), 단일 대상 DPS = 공격력 × 초당 발\n");
		std::printf("재료는 3개 레시피 족보마다 가장 적은 장수, Lv2+ 는 그 족보로만 거듭 합성. 카드 = 든 Lv1 카드 수\n");
		std::printf(" %-22s 재료  카드 |   위력  공격특성 연사특성 범위 사거리 |    공격력  초당발        DPS   Lv1대비  카드당(Lv1대비)\n", "조커");
		const int Count = bUpper ? UpperJokerCount : (int)EPDJoker::Count;
		for (int K = 0; K < Count; ++K)
		{
			const int JK = bUpper ? UpperBase + K : K;
			std::string Name = bUpper ? std::string(JN[(int)UpperDef(K).A]) + "+" + JN[(int)UpperDef(K).B] + (UpperDef(K).Special != EPDUpperSpecial::None ? " ★" : "") : JN[K];
			double Dps1 = 0, PerCard1 = 0;
			for (int L = 1; L <= MaxLevel; ++L)
			{
				FPDTower T = G.DevJokerTower(JK, L);
				T.JLevel = IsUpperJoker(JK) ? JokerUpLevel : 1;
				const FPDTowerStats St = TowerStats(T, 0, &G.Mods);
				const double Dps = (double)St.Dmg * St.Rate, PerCard = Dps / std::max(1, G.DevJokerCards(JK, L));
				if (L == 1) { Dps1 = Dps; PerCard1 = PerCard; }
				std::printf(" %-22s  Lv%d %5d | %7.1f  %7.2f  %7.2f  %4.2f  %5.2f | %9.0f  %6.2f  %10.0f  %7.1f배  %7.1f배\n", L == 1 ? Name.c_str() : "", L, G.DevJokerCards(JK, L), T.Mult, T.TDmg, T.TRate, T.TSplash, St.Range, St.Dmg, St.Rate, Dps, Dps / Dps1, PerCard / PerCard1);
			}
		}
	}
	else if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "level") // 레벨 곡선 (D-479): level [모드 0=30 1=60 2=100 3=무한, 기본 0] — 트리까지 쓰는 봇
	{
		GBotMode = (EPDMode)(Argc > 3 ? std::atoi(Argv[3]) : 0);
		GLvTrack = true; GLvRows.clear(); GBotClears = 0;
		BotTest(Runs, false, false, true, true, true);
		std::printf("\n[레벨 곡선] 웨이브를 막은 직후 평균 (살아남은 판만)\n 웨이브  판수   레벨  이번 웨이브 레벨업  번 경험치  다음 레벨까지   시간(분)\n");
		for (auto& KV : GLvRows)
		{
			const FLvRow& Row = KV.second;
			if (KV.first > 30 && KV.first % 5 != 0) continue; // 30 넘으면 5웨이브마다
			FPDGame Probe(1);
			Probe.Level = (int)std::lround(Row.Lv / Row.N);
			std::printf(" %5d  %4d  %5.1f  %15.2f  %9.0f  %12d  %8.1f\n", KV.first, Row.N, Row.Lv / Row.N, Row.Ups / Row.N, Row.Xp / Row.N, Probe.XpToNext(), Row.Time / Row.N / 60.0);
		}
		std::printf("   → 클리어 %d / %d판\n", GBotClears, Runs);
		GLvTrack = false; GBotMode = EPDMode::Endless;
	}
	else if (Runs > 0 && Argc > 5 && std::string(Argv[2]) == "chips") // 판당 칩 표 (D-322): chips <모드 0=30 1=60 2=100 3=무한> <강화 레벨> <해금 0/1/2>
	{
		GBotMode = (EPDMode)std::atoi(Argv[3]); GBotUpgLv = std::atoi(Argv[4]); GBotContent = std::atoi(Argv[5]); GBotClears = 0;
		BotTest(Runs, false, false, true, true, true, GBotUpgLv > 0);
		std::printf("   → CHIPS mode=%s upg=%d content=%d clear=%d/%d\n", Argv[3], GBotUpgLv, GBotContent, GBotClears, Runs);
		GBotMode = EPDMode::Endless; GBotUpgLv = 10; GBotContent = 0;
	}
	else if (Runs > 0) { BotTest(Runs, false); BotTest(Runs, false, false, true); BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, false, true, true, true, true); BotTest(Runs, false, true); BotTest(Runs, true); }
	std::printf("\n%s (실패 %d)\n", GFails ? "실패 있음" : "모두 통과", GFails);
	return GFails ? 1 : 0;
}
