// 포커 디펜스 규칙 코어 시험 (언리얼 없이 실행)
//   ./Tests/run.sh [봇 판 수]
// 족보·연쇄·길·설치에 더해 콘텐츠 1차(와일드 카드, 방패·분열·재생, 부적, 보상)를 확인하고, 봇으로 밸런스를 본다.
#include "../Source/ProjectB/PDCore.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

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
	static const char* N[] = {"straightFlush", "four", "fullHouse", "flush", "straight", "triple", "twoPair", "pair"};
	std::vector<std::string> S;
	for (EPDHand H : V) S.push_back(N[(int)H]);
	std::sort(S.begin(), S.end());
	std::string O;
	for (auto& X : S) O += (O.empty() ? "" : ",") + X;
	return O.empty() ? "none" : O;
}
static void Put(FPDTowerMap& M, int X, int Y, FPDCard Card)
{
	FPDTower T;
	T.Card = Card;
	T.X = X;
	T.Y = Y;
	M[CellKey(X, Y)] = T;
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
		Expect(G.Towers[CellKey(1, 0)].Card.Rank == 4 && G.Synergies.empty() && G.Discarded == 1, "타워 위에 놓으면 교체");
	}
	{ // 와일드 카드
		const FPDCard W{0, EPDSuit::Wild};
		auto HasH = [](std::vector<EPDHand> V, EPDHand H) { return std::find(V.begin(), V.end(), H) != V.end(); };
		Expect(HasH(ClassifyHand({C(7), W}), EPDHand::Pair) && HasH(ClassifyHand({C(2, He), C(9, He), W}), EPDHand::Flush) && HasH(ClassifyHand({C(4), C(5, Di), W}), EPDHand::Straight),
			"와일드 카드: 페어·플러시·스트레이트를 이어 줌");
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
		int J1 = 0;
		for (auto& R : R1) J1 += R.bCharm;
		G.Charms = {EPDCharm::Twins, EPDCharm::Scope, EPDCharm::Supply, EPDCharm::Lucky7, EPDCharm::Frost};
		auto R2 = G.RollRewards();
		bool bAllInst = true;
		for (auto& R : R2) bAllInst = bAllInst && !R.bCharm;
		Expect(R1.size() == 3 && J1 == 2 && bAllInst && R2.size() == 3, "보상 후보: 부적 2 + 즉시 1, 부적 칸이 차면 즉시만");
		G.Charms = {EPDCharm::BigHand};
		Expect(G.HandMax() == 9, "큰 손: 손패 최대 9");
		G.Phase = EPDPhase::Reward;
		G.Rewards = {{false, (int)EPDInstant::Wild}};
		G.TakeReward(0);
		Expect(!G.Hand.empty() && G.Hand.back().IsWild(), "와일드 카드 보상");
	}
	{ // 합성: 페어 → 레벨 2 타워 하나, 같은 레벨끼리만 족보, 레벨 2 페어 → 레벨 3
		FPDGame G(8);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		auto PutT = [&](int X, int Y, FPDCard Cd) { FPDTower T; T.Card = Cd; T.X = X; T.Y = Y; G.Towers[CellKey(X, Y)] = T; };
		int Row = -1;
		for (int Y = 0; Y < Rows && Row < 0; ++Y) { bool bFree = true; for (int X = 0; X < 6; ++X) bFree = bFree && G.Buildable(X, Y); if (bFree) Row = Y; }
		PutT(0, Row, C(9)); PutT(1, Row, C(9, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bFused = G.Fuse(0);
		const FPDTower& F = G.Towers.begin()->second;
		Expect(bFused && G.Towers.size() == 1 && F.Level == 2 && std::fabs(F.Mult - 2.f * FuseBonus) < 1e-4f && F.TDmg == 1.5f && F.Card.Rank == 9 && G.Synergies.empty(),
			"합성: 9 페어 → 레벨 2 타워 하나 (위력 3배, 공격력 특성 ×1.5)");
		// 레벨 2 옆에 레벨 1 같은 숫자 → 족보 아님
		const int FX0 = F.X;
		PutT(FX0 + 1, Row, C(9, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.empty(), "레벨이 다르면 족보가 안 됨");
		// 레벨 1 두 장을 또 합쳐 레벨 2 를 만들고, 레벨 2 둘이 붙으면 페어 → 레벨 3
		PutT(FX0 + 2, Row, C(9, Di));
		PutT(FX0 + 1, Row, C(9, Cl));
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
		for (const auto& Ev : G.Events) if (Ev.Kind == FPDEvent::EKind::WaveStart && Ev.A == 20) bBoss20 = Ev.B == 1 && Ev.Enemy == EPDEnemy::ShieldBoss;
		Expect(bNoWin && bBoss20, "15웨이브 뒤에도 계속, 20웨이브에 방패 거인이 다시");
		FPDGame G2(11);
		G2.NewGame(true);
		G2.Deck.clear();
		G2.Hand.clear();
		G2.Phase = EPDPhase::Reward;
		G2.Rewards = {{false, (int)EPDInstant::Draw2}};
		G2.TakeReward(0);
		Expect(G2.Hand.size() == 2 && G2.DeckRefills == 1 && G2.Deck.size() == 50, "덱이 비면 새 덱 한 벌");
	}
	{ // 조커: 레시피
		using H = EPDHand;
		Expect(MatchJokerRecipe({H::Pair, H::Pair, H::Pair}, 0) == EPDJoker::Clown && MatchJokerRecipe({H::TwoPair, H::TwoPair, H::Pair}, 0) == EPDJoker::Comedy &&
			MatchJokerRecipe({H::Straight, H::Straight, H::Straight}, 0) == EPDJoker::Baton && MatchJokerRecipe({H::Flush, H::Flush, H::Flush}, 0) == EPDJoker::Harlequin &&
			MatchJokerRecipe({H::Triple, H::Triple, H::Pair}, 0) == EPDJoker::Jack && MatchJokerRecipe({H::FullHouse, H::FullHouse, H::Pair}, 0) == EPDJoker::Bell &&
			MatchJokerRecipe({H::Four, H::Pair, H::Pair}, 0) == EPDJoker::Cheshire && 
			MatchJokerRecipe({}, 3) == EPDJoker::Mask && MatchJokerRecipe({H::Pair, H::Pair}, 1, 3) == EPDJoker::Hatter && MatchJokerRecipe({H::Pair, H::Pair}, 1, 2) == EPDJoker::Clown && MatchJokerRecipe({H::StraightFlush, H::Four, H::FullHouse}, 0) == EPDJoker::Star,
			"조커 레시피 10종");
	}
	{ // 조커 합성: 연쇄된 페어 셋 → 광대, 합성 타워 + 족보 둘 → 모자 장수
		FPDGame G(12);
		G.NewGame(true);
		G.bMuted = true;
		G.Towers.clear();
		auto PutT = [&](int X, int Y, FPDCard Cd, int Lv = 1) { FPDTower T; T.Card = Cd; T.X = X; T.Y = Y; T.Level = Lv; G.Towers[CellKey(X, Y)] = T; };
		int Row = -1;
		for (int Y = 0; Y < Rows && Row < 0; ++Y) { bool bFree = true; for (int X = 0; X < 7; ++X) bFree = bFree && G.Buildable(X, Y); if (bFree) Row = Y; }
		// 페어 + 스트레이트(7 8) + 페어 (페어 둘이 바로 붙으면 투 페어 하나가 되므로 사이에 스트레이트)
		PutT(0, Row, C(2)); PutT(1, Row, C(2, He)); PutT(2, Row, C(7)); PutT(3, Row, C(8, He)); PutT(4, Row, C(5)); PutT(5, Row, C(5, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		auto Groups = G.JokerGroups();
		const bool bGroup = Groups.size() == 1 && Groups[0].Result == EPDJoker::Clown && Groups[0].Cells.size() == 6;
		const bool bFused = G.JokerFuse(0);
		const FPDTower& J = G.Towers.begin()->second;
		Expect(bGroup && bFused && G.Towers.size() == 1 && J.IsJoker() && J.Joker == (int)EPDJoker::Clown && std::fabs(J.Mult - 6.f * JokerBonus) < 1e-4f && (G.Discovered & 1u),
			"연쇄된 족보 셋(페어·스트레이트·페어) → 광대 조커 (위력 6 × 조커 덤), 도감에 기록");
		// 조커는 와일드: 레벨 2 타워 옆에 붙어도 족보
		G.Towers.clear();
		FPDTower Jk;
		Jk.Card = {0, EPDSuit::Wild};
		Jk.X = 0;
		Jk.Y = Row;
		Jk.Joker = (int)EPDJoker::Clown;
		G.Towers[CellKey(0, Row)] = Jk;
		PutT(1, Row, C(9), 2);
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(G.Synergies.size() == 1 && !G.CanFuse(0), "조커는 레벨 2 타워와도 족보 (와일드), 조커가 든 족보는 합성 불가");
		// 합성 타워(따로 선 레벨 2) + 페어 둘 → 모자 장수
		G.Towers.clear();
		PutT(0, Row, C(13), 3); PutT(1, Row, C(2)); PutT(2, Row, C(2, He)); PutT(3, Row, C(7)); PutT(4, Row, C(8, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Hatter && Groups[0].FusedCells.size() == 1, "레벨 3 합성 타워 + 족보 둘 → 모자 장수");
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
		for (int Y = 0; Y < Rows && JX < 0; ++Y) for (int X = 0; X < Cols; ++X) if (G.Buildable(X, Y) && ((X > 0 && G.Path.IsPath(X - 1, Y)) || (X + 1 < Cols && G.Path.IsPath(X + 1, Y)))) { JX = X; JY = Y; break; }
		float NearD = 0.f, BestDist = 1e9f;
		for (float D = 0.f; D < G.Path.Length; D += 0.1f) { const FPDPoint P = G.Path.At(D); const float Dist = std::hypot(P.X - JX - 0.5f, P.Y - JY - 0.5f); if (Dist < BestDist) { BestDist = Dist; NearD = D; } }
		auto Setup = [&](EPDJoker K) {
			G.Towers.clear();
			G.Towers[CellKey(JX, JY)] = MakeJ(JX, JY, K);
			G.Synergies = ApplyEffects(G.Towers, 0);
			G.Phase = EPDPhase::Wave;
			G.Enemies.clear();
			for (int I = 0; I < 3; ++I) { FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 10000.f, 1); E.Speed = 0.f; E.D = NearD + I * 0.15f; G.Enemies.push_back(E); }
		};
		Setup(EPDJoker::Comedy);
		G.Update(0.01f);
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
	{ // 스킬 트리 구조: 모두 이어져 있고, 조커 강화는 막다른 끝이라 조커끼리는 안쪽으로 돌아가야 한다
		const int N = TreeNodeCount();
		std::vector<int> Dist(N, -1);
		std::vector<int> Q{0};
		Dist[0] = 0;
		for (size_t I = 0; I < Q.size(); ++I) for (int M : TreeLinks(Q[I])) if (Dist[M] < 0) { Dist[M] = Dist[Q[I]] + 1; Q.push_back(M); }
		bool bAll = true, bLeaf = true, bKeyLeaf = true;
		int Jokers = 0, Keys = 0;
		for (int I = 0; I < N; ++I)
		{
			bAll = bAll && Dist[I] >= 0;
			if (TreeNode(I).Kind == EPDNode::Joker) { Jokers++; bLeaf = bLeaf && TreeLinks(I).size() == 1; }
			if (TreeNode(I).Kind == EPDNode::Keystone) { Keys++; bKeyLeaf = bKeyLeaf && TreeLinks(I).size() == 1; }
		}
		Expect(N == 61 && bAll, "트리 61노드 모두 시작점과 이어짐");
		Expect(Jokers == 10 && bLeaf, "조커 강화 10개, 모두 막다른 끝 (끝자락끼리 안 이어짐)");
		Expect(Keys == 8 && bKeyLeaf, "키스톤 8개 (4쌍), 모두 막다른 끝");
		// 조커 끝에서 다른 조커 끝까지 최소 3걸음 (갈림길 → 뿌리 → 갈림길 → 조커)
		int MinJJ = 99;
		for (int A = 0; A < N; ++A)
		{
			if (TreeNode(A).Kind != EPDNode::Joker) continue;
			std::vector<int> D(N, -1);
			std::vector<int> Q2{A};
			D[A] = 0;
			for (size_t I = 0; I < Q2.size(); ++I) for (int M : TreeLinks(Q2[I])) if (D[M] < 0) { D[M] = D[Q2[I]] + 1; Q2.push_back(M); }
			for (int B = 0; B < N; ++B) if (B != A && TreeNode(B).Kind == EPDNode::Joker) MinJJ = std::min(MinJJ, D[B]);
		}
		Expect(MinJJ >= 4, "조커 강화끼리 최소 4걸음 (다시 올라와야 함): " + std::to_string(MinJJ));
		int MaxD = 0;
		for (int I = 0; I < N; ++I) MaxD = std::max(MaxD, Dist[I]);
		Expect(MaxD <= 7, "가장 먼 노드도 7포인트 안: " + std::to_string(MaxD));
	}
	{ // 포인트 구매: 값은 ×1.15, 이어진 노드만, 골드 부족
		FPDGame G(5);
		G.NewGame(true);
		Expect(G.PointPrice() == PointBasePrice, "첫 포인트 값 = 기본값");
		Expect(G.CanBuy(1) == FPDGame::EBuy::NoGold, "골드가 없으면 못 찍음");
		G.Gold = 1000;
		Expect(G.CanBuy(2) == FPDGame::EBuy::NotLinked, "이어지지 않은 노드는 못 찍음");
		Expect(G.BuyNode(1) && G.Gold == 1000 - PointBasePrice && G.Points == 1, "시작점 옆 노드 찍기");
		Expect(G.PointPrice() == (int)std::lround(PointBasePrice * PointPriceGrowth), "다음 포인트 값은 성장 배율만큼");
		Expect(G.CanBuy(1) == FPDGame::EBuy::Owned, "이미 찍은 노드");
		G.Points = 20;
		Expect(G.PointPrice() == (int)std::lround(PointBasePrice * std::pow(PointPriceGrowth, 20.f)), "21번째 포인트 값: " + std::to_string(G.PointPrice()));
		G.Phase = EPDPhase::Reward;
		Expect(G.CanBuy(2) == FPDGame::EBuy::Closed, "보상 고르는 중에는 못 찍음");
	}
	{ // 키스톤: 짝은 봉인, 판마다 3개
		FPDGame G(6);
		G.NewGame(true);
		G.Gold = 100000;
		for (int Id : {1, 2, 3, 4, 5}) G.BuyNode(Id); // ♠ 팔 끝 키스톤 고독한 저격수
		Expect(G.Mods.Key(EPDKeystone::Lone), "고독한 저격수 찍음");
		for (int Id : {6, 7, 8, 9}) G.BuyNode(Id);
		Expect(G.TreeSealed(10) && G.CanBuy(10) == FPDGame::EBuy::Sealed && !G.BuyNode(10), "짝 키스톤 군중은 봉인");
		uint64_t All = 0;
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind != EPDNode::Keystone) All |= 1ull << I;
		G.TreeOwned |= All;
		G.Mods = BuildTreeMods(G.TreeOwned);
		int Got = 1;
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && G.CanBuy(I) == FPDGame::EBuy::Ok && G.BuyNode(I)) Got++;
		bool bCap = false;
		for (int I = 0; I < TreeNodeCount(); ++I) if (TreeNode(I).Kind == EPDNode::Keystone && !G.TreeOwnedNode(I) && !G.TreeSealed(I)) bCap = bCap || G.CanBuy(I) == FPDGame::EBuy::KeyCap;
		Expect(Got == 3 && G.KeystoneCount() == 3 && bCap, "키스톤은 3개까지, 남은 건 막힘");
		bool bNoPair = true;
		for (int K = 0; K < (int)EPDKeystone::Count; K += 2) bNoPair = bNoPair && !(G.Mods.Key((EPDKeystone)K) && G.Mods.Key((EPDKeystone)(K + 1)));
		Expect(bNoPair, "골드가 무한이어도 짝 키스톤을 둘 다 가질 수 없다");
	}
	{ // 골드: 처치 골드, 웨이브 덤, 골드 노드
		FPDGame G(7);
		G.NewGame(true);
		G.Phase = EPDPhase::Wave;
		FPDEnemy E = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1);
		G.HitTest(E, 100.f);
		Expect(G.Gold == 1, "보통 적 처치 1골드");
		FPDEnemy B = G.SpawnTestEnemy(EPDEnemy::ShieldBoss, 1.f, 5);
		G.HitTest(B, 1000.f);
		Expect(G.Gold == 26, "보스 처치 25골드");
		Expect(G.KillGold(EPDEnemy::Tank, 11) == 5, "탱크 11웨이브 = 3 + 2");
		G.TreeOwned |= 1ull << 37;
		G.Mods = BuildTreeMods(G.TreeOwned);
		int Before = G.Gold;
		for (int I = 0; I < 20; ++I) { FPDEnemy N2 = G.SpawnTestEnemy(EPDEnemy::Normal, 1.f, 1); G.HitTest(N2, 100.f); }
		Expect(G.Gold - Before == 23, "골드 +15% 노드: 20마리 → 23골드");
	}
	{ // 트리 효과: 무늬 노드, 외골수, 고독한 저격수, 대장장이, 광대의 길, 조커 강화
		FPDTowerMap M;
		Put(M, 0, 0, C(9, EPDSuit::Spade));
		FPDTreeMods Tm = BuildTreeMods(1ull | 1ull << 1);
		ApplyEffects(M, 0, &Tm);
		const float Base = TowerStats(M.begin()->second, 0).Dmg, Up = TowerStats(M.begin()->second, 0, &Tm).Dmg;
		Expect(std::abs(Up / Base - 1.12f) < 1e-3f, "♠ 위력 +12% 노드");
		Tm = BuildTreeMods(1ull | 1ull << 5);
		ApplyEffects(M, 0, &Tm);
		Expect(std::abs(TowerStats(M.begin()->second, 0, &Tm).Dmg / Base - 1.6f) < 1e-3f, "고독한 저격수: 혼자 선 타워 ×1.6");
		FPDTowerMap F; // 플러시 3장 + 떨어진 원 페어
		Put(F, 0, 0, C(2, EPDSuit::Heart)); Put(F, 1, 0, C(6, EPDSuit::Heart)); Put(F, 2, 0, C(11, EPDSuit::Heart));
		Put(F, 0, 5, C(8, EPDSuit::Club)); Put(F, 1, 5, C(8, EPDSuit::Diamond));
		ApplyEffects(F, 0);
		const float Fl = TowerStats(F.at(CellKey(0, 0)), 0).Dmg, Pr = TowerStats(F.at(CellKey(0, 5)), 0).Dmg;
		Tm = BuildTreeMods(1ull | 1ull << 28); // 외골수
		ApplyEffects(F, 0, &Tm);
		Expect(std::abs(TowerStats(F.at(CellKey(0, 0)), 0, &Tm).Dmg / Fl - 2.f) < 1e-3f && std::abs(TowerStats(F.at(CellKey(0, 5)), 0, &Tm).Dmg / Pr - 0.5f) < 1e-3f, "외골수: 플러시 ×2, 다른 족보 ×0.5");
		Tm = BuildTreeMods(1ull | 1ull << 15); // 잡식가
		ApplyEffects(F, 0, &Tm);
		Expect(F.at(CellKey(0, 0)).Eff.Splash == 0.f, "잡식가: 플러시 범위 효과 없음");
		FPDGame G(8);
		G.NewGame(true);
		G.Towers.clear();
		int Row = -1;
		for (int Y = 0; Y < Rows && Row < 0; ++Y) { bool bFree = true; for (int X = 0; X < 7; ++X) bFree = bFree && G.Buildable(X, Y); if (bFree) Row = Y; }
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(2, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(8, EPDSuit::Heart)); Put(G.Towers, 4, Row, C(5)); Put(G.Towers, 5, Row, C(5, EPDSuit::Heart));
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		Expect(!G.JokerGroups().empty(), "조커 재료 있음");
		G.TreeOwned |= 1ull << 36;
		G.Mods = BuildTreeMods(G.TreeOwned);
		Expect(G.JokerGroups().empty() && std::abs(G.FuseMult() - 2.f) < 1e-4f, "대장장이: 조커 못 만듦, 합성 배율 2.0");
		G.TreeOwned = 1ull | 1ull << 20;
		G.Mods = BuildTreeMods(G.TreeOwned);
		for (auto& KV : G.Towers) KV.second.Level = 2;
		G.Synergies = ApplyEffects(G.Towers, 0, &G.Mods);
		Expect(!G.CanFuse(0) && std::abs(G.JokerMult() - 1.6f) < 1e-4f, "광대의 길: 레벨 2는 더 합성 못 함, 조커 배율 1.6");
		FPDTower J;
		J.Card = {0, EPDSuit::Wild};
		J.Joker = (int)EPDJoker::Jack;
		const float J0 = TowerStats(J, 0).Dmg;
		Tm = BuildTreeMods(1ull | 1ull << (41 + (int)EPDJoker::Jack));
		Expect(std::abs(TowerStats(J, 0, &Tm).Dmg / J0 - 5.f / 3.f) < 1e-3f, "잭 강화: ×3 → ×5");
	}
}

// ---------------------------------------------------------------- 봇 (test.js 와 같은 평가)
static int Coverage(const FPDGame& G, int X, int Y, float Range)
{
	int N = 0;
	for (float D = 0.f; D < G.Path.Length; D += 0.5f) { const FPDPoint P = G.Path.At(D); if (std::hypot(P.X - X - 0.5f, P.Y - Y - 0.5f) <= Range) N++; }
	return N;
}
static double Power(FPDGame& G, FPDTowerMap& Tw)
{
	const uint32_t M = G.CharmMask();
	ApplyEffects(Tw, M, &G.Mods);
	double Sum = 0;
	for (auto& KV : Tw)
	{
		const FPDTower& T = KV.second;
		const FPDTowerStats S = TowerStats(T, M, &G.Mods);
		double V = S.Dmg * S.Rate * (1 + S.Splash * 0.8) * Coverage(G, T.X, T.Y, S.Range);
		if (T.IsJoker()) // 조커 능력 어림값
		{
			switch ((EPDJoker)T.Joker)
			{
			case EPDJoker::Comedy: V *= 1.8; break;                       // 두 발
			case EPDJoker::Star: V *= 4.0; break;                         // 모든 적 (한 번에 적 여럿)
			case EPDJoker::Bell: V *= 1.3; break;                         // 멈춤
			case EPDJoker::Mask:                                          // 옆 타워 따라 함
				if (T.Eff.CopyKey >= 0 && Tw.count(T.Eff.CopyKey)) { const FPDTowerStats O = TowerStats(Tw.at(T.Eff.CopyKey), M, &G.Mods); V = std::max(V, O.Dmg * O.Rate * (1 + O.Splash * 0.8) * Coverage(G, T.X, T.Y, O.Range)); }
				break;
			default: break;
			}
		}
		Sum += V;
	}
	return Sum;
}
// 트리: 골드가 되는 만큼 찍는다. 지금 판 위력이 가장 많이 오르는 노드 (한 걸음 더 내다봄, 살림 노드는 어림값)
static std::map<int, int> GKeysTaken;
static double TreeGain(FPDGame& G, uint64_t Owned, double Before)
{
	FPDGame Copy = G;
	Copy.TreeOwned = Owned;
	Copy.Mods = BuildTreeMods(Owned);
	FPDTowerMap Tw = Copy.Towers;
	double V = Power(Copy, Tw) - Before;
	for (int Id = 0; Id < TreeNodeCount(); ++Id)
		if (((Owned >> Id) & 1ull) && !((G.TreeOwned >> Id) & 1ull))
			for (const FPDNodeFx& F : TreeNode(Id).Fx)
			{
				if (F.Stat == EPDStat::GoldPct) V += Before * 0.04;
				if (F.Stat == EPDStat::HandMax) V += Before * 0.03;
				if (F.Stat == EPDStat::Draw) V += Before * 0.08;
				if (F.Stat == EPDStat::Lives) V += Before * 0.02;
			}
	return V;
}
static void BotBuyTree(FPDGame& G)
{
	for (int Guard = 0; Guard < 30 && G.Gold >= G.PointPrice(); ++Guard)
	{
		FPDTowerMap Cur = G.Towers;
		const double Before = std::max(1.0, Power(G, Cur));
		int Best = -1;
		double BestV = -1e18;
		for (int Id = 0; Id < TreeNodeCount(); ++Id)
		{
			if (G.CanBuy(Id) != FPDGame::EBuy::Ok) continue;
			const uint64_t O1 = G.TreeOwned | 1ull << Id;
			double V = TreeGain(G, O1, Before);
			for (int M : TreeLinks(Id)) // 한 걸음 더 (두 포인트로 나눔)
			{
				if ((O1 >> M) & 1ull) continue;
				if (TreeNode(M).Kind == EPDNode::Keystone && (G.KeystoneCount() >= KeystoneMax || G.Mods.Key((EPDKeystone)(TreeNode(M).Fx[0].Param ^ 1)))) continue;
				V = std::max(V, TreeGain(G, O1 | 1ull << M, Before) / 2.0);
			}
			if (V > BestV) { BestV = V; Best = Id; }
		}
		if (Best < 0) break;
		G.BuyNode(Best);
		if (TreeNode(Best).Kind == EPDNode::Keystone) GKeysTaken[TreeNode(Best).Fx[0].Param]++;
	}
}

static void BotTest(int Runs, bool bDumb, bool bNoReward = false, bool bFuse = false, bool bJoker = false, bool bTree = false)
{
	double PointSum = 0, GoldSum = 0;
	GKeysTaken.clear();
	std::map<int, int> Hist;
	int WaveSum = 0;
	std::map<int, int> JokersMade, KindsMade;
	for (int Run = 0; Run < Runs; ++Run)
	{
		FPDGame G(1000 + Run);
		G.NewGame(true);
		G.bMuted = true;
		std::mt19937 R(77 + Run);
		for (int Guard = 0; Guard < 200 && G.Phase != EPDPhase::Over; ++Guard)
		{
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
			if (bTree) BotBuyTree(G);
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
		Hist[G.Wave]++;
		WaveSum += G.Wave;
		PointSum += G.Points;
		GoldSum += G.GoldEarned;
	}
	std::printf("\n%s%s%s%s%s 봇 %d판, 무너진 웨이브:", bDumb ? "족보 무시" : "족보 생각", bNoReward ? " · 보상 안 받음" : " · 부적 먼저", bFuse ? " · 합성" : "", bJoker ? " · 조커" : "", bTree ? " · 트리" : "", Runs);
	for (auto& KV : Hist) std::printf("  %d:%d", KV.first, KV.second);
	std::printf("   평균 %.1f웨이브", (double)WaveSum / Runs);
	if (bJoker) std::printf("   조커 판당 %.1f개 (종류:", (double)JokersMade[0] / Runs);
	if (bJoker) { for (auto& KV : KindsMade) std::printf(" %d=%d", KV.first + 1, KV.second); std::printf(")"); }
	std::printf("   번 골드 %.0f · 포인트 %.1f", GoldSum / Runs, PointSum / Runs);
	if (bTree) { std::printf("   키스톤:"); for (auto& KV : GKeysTaken) std::printf(" %d=%d", KV.first, KV.second); }
	std::printf("\n");
}

int main(int Argc, char** Argv)
{
	std::printf("[규칙 단위 시험]\n");
	UnitTests();
	const int Runs = Argc > 1 ? std::atoi(Argv[1]) : 10;
	if (Runs > 0 && Argc > 2 && std::string(Argv[2]) == "tree") { BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); }
	else if (Runs > 0) { BotTest(Runs, false); BotTest(Runs, false, false, true); BotTest(Runs, false, false, true, true); BotTest(Runs, false, false, true, true, true); BotTest(Runs, false, true); BotTest(Runs, true); }
	std::printf("\n%s (실패 %d)\n", GFails ? "실패 있음" : "모두 통과", GFails);
	return GFails ? 1 : 0;
}
