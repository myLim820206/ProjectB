// 포커 디펜스 규칙 코어 시험 (언리얼 없이 실행)
//   ./Tests/run.sh [봇 판 수]
// 족보·연쇄·길·설치·적·부적·발사체·합성·조커·스킬 트리·골드 강화를 확인하고, 봇으로 밸런스를 본다.
//   ./Tests/run.sh 0 은 봇 없이, ./Tests/run.sh N base|tree 는 그 봇만
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
	const EPDSuit He = EPDSuit::Heart, Di = EPDSuit::Diamond, Cl = EPDSuit::Club;
	auto Case = [](const char* Label, std::vector<FPDCard> Cards, const char* Want) {
		const std::string Got = Names(ClassifyHand(Cards));
		Expect(Got == Want, std::string(Label) + " -> " + Got);
	};
	Case("원 페어", {C(7), C(7, He)}, "pair");
	Case("떨어진 숫자 2장", {C(7), C(9, He)}, "none");
	Case("이어진 숫자 2장 = 스트레이트", {C(4, He), C(3)}, "straight");
	Case("K-A도 이어짐", {C(13, He), C(14)}, "straight");
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
	{ // 연쇄: 스트레이트(7 8) + 트리플(5 5 5) 맞닿음
		FPDTowerMap M;
		Put(M, 0, 3, C(7)); Put(M, 1, 3, C(8, He)); Put(M, 2, 3, C(5)); Put(M, 3, 3, C(5, He)); Put(M, 4, 3, C(5, Di));
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
		Expect(HasStraight(B) == 2, "5 7 6 은 3장 스트레이트가 아님 (붙은 7 6 만 2장 스트레이트)");
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
			G.Gold = 1234; G.Points = 3; G.Upg[2] = 7; G.Charms = {(EPDCharm)1}; G.TreeOwned.set(5); G.Wave = 9;
			G.Towers[CellKey(EX, 0)].Joker = 2; G.Towers[CellKey(EX, 0)].JLevel = 4; G.Towers[CellKey(EX, 0)].JXp = 12.5f;
			const std::string Sv = G.SaveRun();
			FPDGame H(77);
			H.NewGame(true);
			const bool bLoad = H.LoadRun(Sv);
			bool bSame = bLoad && H.SaveRun() == Sv && H.Path.Points.size() == G.Path.Points.size() && H.Towers.size() == G.Towers.size() && H.Phase == EPDPhase::Build;
			bSame = bSame && H.Towers[CellKey(EX, 0)].JLevel == 4 && H.Gold == 1234 && H.Swaps == G.Swaps && H.Hand.size() == G.Hand.size() && H.Deck.size() == G.Deck.size();
			FPDGame Bad(5);
			Bad.NewGame(true);
			const int BadGold = Bad.Gold;
			const bool bReject = !Bad.LoadRun("pdrun 1\nwave 3\nnonsense 1\n") && Bad.Gold == BadGold && !Bad.LoadRun("hello");
			Expect(bSame && bReject, "중단 저장: 저장한 판을 그대로 불러오고, 깨진 글은 거절");
		}
		std::map<int, int> Seen;
		for (int I = 0; I < 400; ++I) for (const FPDReward& R : G.RollRewards()) if (!R.bCharm) Seen[R.Id]++;
		Expect(Seen[(int)EPDInstant::Swap] > 60, "부적 창에 자리바꿈이 자주 나온다 (" + std::to_string(Seen[(int)EPDInstant::Swap]) + "/400)");
	}
	{ // 와일드 카드
		const FPDCard W{0, EPDSuit::Wild};
		auto HasH = [](std::vector<EPDHand> V, EPDHand H) { return std::find(V.begin(), V.end(), H) != V.end(); };
		Expect(HasH(ClassifyHand({C(7), W}), EPDHand::Pair) && HasH(ClassifyHand({C(2, He), C(9, He), W}), EPDHand::Flush) && HasH(ClassifyHand({C(4), C(5, Di), W}), EPDHand::Straight),
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
		G.Rewards = {{false, (int)EPDInstant::Gold}};
		G.Wave = 10;
		const int G0 = G.Gold;
		G.TakeReward(0);
		Expect(G.Gold - G0 == 50, "골드 보상 10 + 웨이브 × 4");
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
		Expect(B1 == 5 && B2 == 5 && B3 == 10 && FPDGame::BossOfWave(20) == BossOrder[3], "다음 보스 웨이브: 3 → 5, 5 진행 중 → 5, 5 끝 → 10");
	}
	{ // 조커가 든 족보 (D-128): 조커를 빼고도 플러시면 그 칸들만 합성, 조커는 남는다
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
		const bool bCan = Fl >= 0 && G.CanFuse(Fl) && G.FuseSynergy(Fl, FS) && FS.Cells.size() == 3 && FS.Kind == EPDHand::Flush;
		const bool bFused = bCan && G.Fuse(Fl);
		int Lv2 = 0;
		for (const auto& KV : G.Towers) Lv2 += KV.second.Level == 2 && !KV.second.IsJoker();
		Expect(bFused && G.Towers.count(CellKey(3, 2)) && G.Towers.at(CellKey(3, 2)).IsJoker() && Lv2 == 1 && G.Towers.size() == 2,
			"조커 + ♦ 셋 = 플러시4 → 조커 빼고 ♦ 셋 플러시 합성 (조커는 그대로)");
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
		Expect(BossOf(5) == EPDEnemy::ShieldBoss && BossOf(20) == EPDEnemy::Charger && BossOf(65) == EPDEnemy::Breaker && BossOf(70) == EPDEnemy::ShieldBoss,
			"보스 순서: 5 방패 거인, 20 돌진 기사, 65 파괴자, 70 다시 방패 거인");
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
			FPDEnemy Far = G.SpawnTestEnemy(EPDEnemy::Normal, 10.f, 20); Far.D = 9.f;
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
		// 스트레이트 넷 (2 3 · 5 6 · 8 9 · J Q, 숫자가 안 이어지게): 지휘봉 3개 레시피 하나 + 남는 스트레이트 하나
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(3, He)); Put(G.Towers, 2, Row, C(5)); Put(G.Towers, 3, Row, C(6, He));
		Put(G.Towers, 4, Row, C(8)); Put(G.Towers, 5, Row, C(9, He)); Put(G.Towers, 6, Row, C(11)); Put(G.Towers, 7, Row, C(12, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Gs = G.JokerGroups();
		const bool bThree = !Gs.empty() && Gs[0].Result == EPDJoker::Baton && Gs[0].Cells.size() == 6;
		const bool bFused = bThree && G.JokerFuse(0);
		Expect(bFused && G.Towers.size() == 3, "스트레이트 넷: 2개 레시피보다 3개 레시피(지휘봉 6장) 먼저, 남는 스트레이트는 그대로");
		// 2개 레시피: 원 페어 + 스트레이트 → 희비극 (카드 4장 덤 ×1.4)
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(9)); Put(G.Towers, 1, Row, C(9, He)); Put(G.Towers, 2, Row, C(4, Di)); Put(G.Towers, 3, Row, C(5));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto G2 = G.JokerGroups();
		const bool b2 = G2.size() == 1 && G2[0].Result == EPDJoker::Comedy && G2[0].Cells.size() == 4 && G.JokerFuse(0);
		const FPDTower& Jc = G.Towers.begin()->second;
		Expect(b2 && G.Towers.size() == 1 && std::fabs(Jc.Mult - 4.f * JokerBonus * JokerTier(EPDJoker::Comedy) * (1.f + JokerCardBonus * 4.f)) < 1e-4f,
			"원 페어 + 스트레이트 → 희비극 2개 레시피 (카드 4장 덤 ×1.4)");
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
		for (const auto& Ev : G.Events) if (Ev.Kind == FPDEvent::EKind::WaveStart && Ev.A == 20) bBoss20 = Ev.B == 1 && Ev.Enemy == EPDEnemy::Charger;
		Expect(bNoWin && bBoss20, "15웨이브 뒤에도 계속, 20웨이브에 새 보스 돌진 기사");
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
		// 스트레이트 셋(4 5 · 7 8 · J Q, 숫자가 안 이어지게) = 지휘봉 3개 레시피 (D-118)
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(5, He)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(8, He)); Put(G.Towers, 4, Row, C(11)); Put(G.Towers, 5, Row, C(12, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Groups = G.JokerGroups();
		const bool bGroup = Groups.size() == 1 && Groups[0].Result == EPDJoker::Baton && Groups[0].Cells.size() == 6;
		const bool bFused = G.JokerFuse(0);
		const FPDTower& J = G.Towers.begin()->second;
		Expect(bGroup && bFused && G.Towers.size() == 1 && J.IsJoker() && J.Joker == (int)EPDJoker::Baton && std::fabs(J.Mult - 6.f * JokerBonus * JokerTier(EPDJoker::Baton) * (1.f + JokerCardBonus * 6.f)) < 1e-4f && (G.Discovered & (1u << (int)EPDJoker::Baton)),
			"맞닿은 스트레이트 셋 → 지휘봉 3개 레시피 (위력 6 × 조커 덤 × 등급 ×2 × 카드 6장 덤 1.6), 도감에 기록");
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
		Put(G.Towers, 0, Row, C(13), 3); Put(G.Towers, 1, Row, C(7)); Put(G.Towers, 2, Row, C(8, He));
		G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Four;
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Cheshire && Groups[0].FusedCells.size() == 1, "포카드를 기억한 합성 타워 + 스트레이트 → 체셔 (2개 레시피)");
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
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(4, He)); Put(G.Towers, 2, Row, C(4, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(DefaultFuseCell(G.Synergies[0].Cells) == CellKey(1, Row) && G.Fuse(0) && G.Towers.count(CellKey(1, Row)) == 1, "합성 위치를 안 고르면 가운데 칸 (예전과 같음)");
		// 조커도 고른 칸에 (스트레이트 셋 → 지휘봉을 맨 왼쪽에)
		G.Towers.clear();
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(5, He)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(8, He)); Put(G.Towers, 4, Row, C(11)); Put(G.Towers, 5, Row, C(12, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bJBad = G.JokerFuse(0, CellKey(6, Row));
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
		Expect(Jokers == 10 && bLeaf && Keys == 8 && bKeyLeaf, "조커 강화 10·키스톤 8, 모두 막다른 끝");
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
		for (int I = 0; I < N; ++I) bPos = bPos && TreeNode(I).X > 20.f && TreeNode(I).X < 1260.f && TreeNode(I).Y > 90.f && TreeNode(I).Y < 700.f;
		float MinGap = 1e9f;
		for (int I = 0; I < N; ++I) for (int J = I + 1; J < N; ++J) MinGap = std::min(MinGap, std::hypot(TreeNode(I).X - TreeNode(J).X, TreeNode(I).Y - TreeNode(J).Y));
		Expect(bPos && MinGap >= 24.f, "노드가 화면 안, 서로 24px 이상 떨어짐 (최소 " + std::to_string((int)MinGap) + ")");
	}
	{ // 포인트 구매
		FPDGame G(5);
		G.NewGame(true);
		const int First = TreeLinks(0)[0];
		Expect(G.PointPrice() == PointBasePrice, "첫 포인트 값 = 기본값");
		Expect(G.CanBuy(First) == FPDGame::EBuy::NoGold, "골드가 없으면 못 찍음");
		G.Gold = 1000;
		Expect(G.CanBuy(PathTo(FindKey(EPDKeystone::Lone)).back()) == FPDGame::EBuy::NotLinked, "이어지지 않은 노드는 못 찍음");
		Expect(G.BuyNode(First) && G.Gold == 1000 - PointBasePrice && G.Points == 1, "시작점 옆 노드 찍기");
		Expect(G.PointPrice() == (int)std::lround(PointBasePrice * PointPriceGrowth), "다음 포인트 값은 성장 배율만큼");
		Expect(G.CanBuy(First) == FPDGame::EBuy::Owned, "이미 찍은 노드");
		G.Points = 20;
		Expect(G.PointPrice() == (int)std::lround(PointBasePrice * std::pow(PointPriceGrowth, 20.f)), "21번째 포인트 값: " + std::to_string(G.PointPrice()));
		G.Phase = EPDPhase::Reward;
		Expect(G.CanBuy(TreeLinks(0)[1]) == FPDGame::EBuy::Closed, "보상 고르는 중에는 못 찍음");
	}
	{ // 키스톤: 짝은 봉인, 판마다 3개
		FPDGame G(6);
		G.NewGame(true);
		G.Gold = 10000000;
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
		Expect(bNoPair, "골드가 무한이어도 짝 키스톤을 둘 다 가질 수 없다");
	}
	{ // 골드: 처치 골드 (마리 수 배율만큼 나눔), 보스 25, 골드 노드
		FPDGame G(7);
		G.NewGame(true);
		G.Phase = EPDPhase::Wave;
		for (int I = 0; I < 16; ++I) { FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(E, 100.f); }
		Expect(G.Gold == 10, "1웨이브 보통 적 16마리 = 예전 10마리 몫 10골드");
		FPDEnemy B = G.SpawnTestEnemy(EPDEnemy::ShieldBoss, 1.f, 5);
		G.HitTest(B, 1000.f);
		Expect(G.Gold == 35, "보스 처치 25골드 (나누지 않음)");
		Expect(G.KillGold(EPDEnemy::Tank, 11) == 5, "탱크 11웨이브 = 3 + 2");
		G.TreeOwned.set(FindStat(EPDStat::GoldPct, 0));
		G.RebuildMods();
		const int Before = G.Gold;
		for (int I = 0; I < 32; ++I) { FPDEnemy N2 = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(N2, 100.f); }
		Expect(G.Gold - Before == 23, "골드 +15% 노드: 32마리 (예전 20마리 몫) → 23골드");
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
		Put(G.Towers, 0, Row, C(4)); Put(G.Towers, 1, Row, C(5, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(8, EPDSuit::Heart)); Put(G.Towers, 4, Row, C(11)); Put(G.Towers, 5, Row, C(12, EPDSuit::Heart));
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		Expect(!G.JokerGroups().empty(), "조커 재료 있음");
		G.TreeOwned = Bits({FindKey(EPDKeystone::Smith)});
		G.RebuildMods();
		Expect(G.JokerGroups().empty() && std::abs(G.FuseMult() - 2.f) < 1e-4f, "대장장이: 조커 못 만듦, 합성 배율 2.0");
		G.TreeOwned = Bits({FindKey(EPDKeystone::ClownPath)});
		for (auto& KV : G.Towers) KV.second.Level = 2;
		G.RebuildMods();
		Expect(!G.CanFuse(0) && std::abs(G.JokerMult() - 1.6f) < 1e-4f, "광대의 길: 레벨 2는 더 합성 못 함, 조커 배율 1.6");
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
	{ // 골드 강화: 레벨 9999까지, 값은 10 + 2 × 레벨
		FPDGame G(9);
		G.NewGame(true);
		G.Gold = 10 + 12 + 14;
		Expect(G.Upgrade(EPDUpgrade::Attack, 100) == 3 && G.Gold == 0 && G.Upg[(int)EPDUpgrade::Attack] == 3, "공격 강화 3레벨 (10 + 12 + 14 골드)");
		FPDTower T;
		T.Card = C(9);
		const float D0 = TowerStats(T, 0).Dmg;
		Expect(std::abs(TowerStats(T, 0, &G.Mods).Dmg / D0 - 1.03f) < 1e-4f, "공격 +1%/레벨");
		G.Upg[(int)EPDUpgrade::Attack] = UpgradeMax - 1;
		G.Gold = 1000000;
		Expect(G.Upgrade(EPDUpgrade::Attack, 10) == 1 && G.Upg[(int)EPDUpgrade::Attack] == UpgradeMax && G.Upgrade(EPDUpgrade::Attack, 1) == 0, "최대 9999레벨");
		G.Upg[(int)EPDUpgrade::Crit] = 1450; // 5% + 145% = 150%
		G.Upg[(int)EPDUpgrade::Spade] = 10;
		G.RebuildMods();
		const FPDTowerStats S = TowerStats(T, 0, &G.Mods);
		Expect(std::abs(S.Crit - 1.5f) < 1e-3f && std::abs(S.CritFactor() - (1.f + 1.5f * 0.5f)) < 1e-3f, "치명타 확률 150% = 치명타 1~2번 (기대 배율 1.75)");
		Expect(G.UpgradeBatchCost(EPDUpgrade::Heart, 3) == 36 && G.UpgradeBatchCost(EPDUpgrade::Attack, 5) == 0, "여러 레벨 값, 최대치면 0");
		long long Total = 0;
		for (int L = 0; L < UpgradeMax; ++L) Total += UpgradeCost(L);
		Expect(Total > 90000000LL, "9999레벨까지 약 " + std::to_string(Total / 1000000) + "백만 골드");
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
	const uint32_t M = G.CharmMask();
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
				if (F.Stat == EPDStat::GoldPct) V += Before * 0.04;
				if (F.Stat == EPDStat::HandMax) V += Before * 0.03;
				if (F.Stat == EPDStat::Draw) V += Before * 0.08;
				if (F.Stat == EPDStat::Lives) V += Before * 0.02;
			}
	return V;
}
// 골드 쓰기: 트리 노드와 골드 강화 중 골드당 위력이 가장 많이 오르는 쪽 (bUpg 면 강화도 씀)
static std::map<int, int> GUpgLv;
static void BotSpend(FPDGame& G, bool bTree, bool bUpg)
{
	for (int Guard = 0; Guard < 24; ++Guard)
	{
		FPDTowerMap Cur = G.Towers;
		const double Before = std::max(1.0, Power(G, Cur));
		int BestNode = -1, BestUpg = -1, BestN = 0;
		double BestRatio = 0.0;
		if (bTree && G.Gold >= G.PointPrice())
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
				const double R = V / G.PointPrice();
				if (R > BestRatio) { BestRatio = R; BestNode = Id; BestUpg = -1; }
			}
		if (bUpg)
			for (int U = 0; U < (int)EPDUpgrade::Count; ++U)
			{
				int N = 0, Cost = 0;
				for (int L = G.Upg[U]; L < UpgradeMax && Cost + UpgradeCost(L) <= std::max(G.Gold / 2, UpgradeCost(G.Upg[U])) && Cost + UpgradeCost(L) <= G.Gold; ++L) { Cost += UpgradeCost(L); ++N; }
				if (N == 0) continue;
				FPDGame Copy = G;
				Copy.Upg[U] += N;
				Copy.RebuildMods();
				FPDTowerMap Tw = Copy.Towers;
				const double R = (Power(Copy, Tw) - Before) / Cost;
				if (R > BestRatio) { BestRatio = R; BestUpg = U; BestN = N; BestNode = -1; }
			}
		if (BestNode >= 0)
		{
			G.BuyNode(BestNode);
			if (TreeNode(BestNode).Kind == EPDNode::Keystone) GKeysTaken[TreeNode(BestNode).Fx[0].Param]++;
		}
		else if (BestUpg >= 0) G.Upgrade((EPDUpgrade)BestUpg, BestN);
		else break;
	}
}

static void BotTest(int Runs, bool bDumb, bool bNoReward = false, bool bFuse = false, bool bJoker = false, bool bTree = false, bool bUpg = false)
{
	double PointSum = 0, GoldSum = 0, UpgSum = 0;
	GKeysTaken.clear();
	GUpgLv.clear();
	std::map<int, int> Hist;
	int WaveSum = 0;
	std::map<int, int> JokersMade, KindsMade;
	long Feeds = 0, JLvSum = 0;
	for (int Run = 0; Run < Runs; ++Run)
	{
		FPDGame G(1000 + Run);
		G.NewGame(true);
		G.bMuted = true;
		std::mt19937 R(77 + Run);
		for (int Guard = 0; Guard < 200 && G.Phase != EPDPhase::Over; ++Guard)
		{
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
				for (int I = 0; I < (int)Groups.size(); ++I)
				{
					FPDGame Copy = G;
					Copy.JokerFuse(I);
					FPDTowerMap After = Copy.Towers;
					const double Gain = Power(Copy, After) - Before;
					if (Gain > BestGain) { BestGain = Gain; BestI = I; }
				}
				if (BestI < 0) break;
				G.JokerFuse(BestI);
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
			if (bTree || bUpg) BotSpend(G, bTree, bUpg);
			G.StartWave();
			for (int K = 0; K < 20000 && G.Phase == EPDPhase::Wave; ++K) G.Update(0.05f);
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
		WaveSum += G.Wave;
		PointSum += G.Points;
		GoldSum += G.GoldEarned;
		UpgSum += G.GoldUpg;
		for (int U = 0; U < (int)EPDUpgrade::Count; ++U) GUpgLv[U] += G.Upg[U];
	}
	std::printf("\n%s%s%s%s%s 봇 %d판, 무너진 웨이브:", bDumb ? "족보 무시" : "족보 생각", bNoReward ? " · 보상 안 받음" : " · 부적 먼저", bFuse ? " · 합성" : "", bJoker ? " · 조커" : "", bTree ? " · 트리" : "", Runs);
	if (bUpg) std::printf(" (+ 강화)");
	for (auto& KV : Hist) std::printf("  %d:%d", KV.first, KV.second);
	std::printf("   평균 %.1f웨이브", (double)WaveSum / Runs);
	if (bJoker) std::printf("   조커 판당 %.1f개 (종류:", (double)JokersMade[0] / Runs);
	if (bJoker) { for (auto& KV : KindsMade) std::printf(" %d=%d", KV.first + 1, KV.second); std::printf(")"); }
	if (bJoker) std::printf("   먹이기 판당 %.1f번 · 최고 조커 레벨 평균 %.1f", (double)Feeds / Runs, (double)JLvSum / Runs);
	std::printf("   번 골드 %.0f · 포인트 %.1f", GoldSum / Runs, PointSum / Runs);
	if (bTree) { std::printf("   키스톤:"); for (auto& KV : GKeysTaken) std::printf(" %d=%d", KV.first, KV.second); }
	if (bUpg) { std::printf("\n      강화에 쓴 골드 %.0f, 평균 레벨:", UpgSum / Runs); for (auto& KV : GUpgLv) std::printf(" %d=%.0f", KV.first, (double)KV.second / Runs); }
	std::printf("\n");
}

int main(int Argc, char** Argv)
{
	std::printf("[규칙 단위 시험]\n");
	UnitTests();
	const int Runs = Argc > 1 ? std::atoi(Argv[1]) : 10;
	if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "base") BotTest(Runs, false, false, true, true);
	else if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "tree") { BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, false, true, true, true, true); }
	else if (Runs > 0) { BotTest(Runs, false); BotTest(Runs, false, false, true); BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, false, true, true, true, true); BotTest(Runs, false, true); BotTest(Runs, true); }
	std::printf("\n%s (실패 %d)\n", GFails ? "실패 있음" : "모두 통과", GFails);
	return GFails ? 1 : 0;
}
