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
	{ // 카드 공유 (D-092): 한 장이 여러 족보에 든다
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
	{ // 와일드 카드 보상은 드물게: 20% 쯤, 3웨이브에 한 번까지
		FPDGame G(31);
		G.NewGame(true);
		for (int K = 0; K < (int)EPDCharm::Count && (int)G.Charms.size() < CharmMax; ++K) G.Charms.push_back((EPDCharm)K); // 부적 칸 가득 (즉시 보상만 나옴)
		int Wild = 0, Last = -99, MinGap = 99;
		for (int W = 1; W <= 300; ++W)
		{
			G.Wave = W;
			bool bW = false;
			for (const FPDReward& R : G.RollRewards()) bW = bW || (!R.bCharm && R.Id == (int)EPDInstant::Wild);
			if (bW) { Wild++; MinGap = std::min(MinGap, W - Last); Last = W; }
		}
		Expect(Wild >= 20 && Wild <= 70 && MinGap >= WildCooldown, "와일드 보상 300웨이브에 " + std::to_string(Wild) + "번, 간격 최소 " + std::to_string(MinGap));
		G.Phase = EPDPhase::Reward;
		G.Rewards = {{false, (int)EPDInstant::Gold}};
		G.Wave = 10;
		const int G0 = G.Gold;
		G.TakeReward(0);
		Expect(G.Gold - G0 == 50, "골드 보상 10 + 웨이브 × 4");
	}
	{ // 발사체: ♥ 미사일은 날아가서 닿을 때 피해, ♠ 레이저는 즉시
		FPDGame G(32);
		G.NewGame(true);
		G.bMuted = true;
		int JX = -1, JY = -1;
		for (int Y = 0; Y < Rows && JX < 0; ++Y) for (int X = 0; X < Cols; ++X) if (G.Buildable(X, Y) && ((X > 0 && G.Path.IsPath(X - 1, Y)) || (X + 1 < Cols && G.Path.IsPath(X + 1, Y)))) { JX = X; JY = Y; break; }
		float NearD = 0.f, BestDist = 1e9f;
		for (float D = 0.f; D < G.Path.Length; D += 0.1f) { const FPDPoint P = G.Path.At(D); const float Dist = std::hypot(P.X - JX - 0.5f, P.Y - JY - 0.5f); if (Dist < BestDist) { BestDist = Dist; NearD = D; } }
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
		G.Phase = EPDPhase::Reward;
		G.TakeReward(-1);
		G.Speed = 3;
		for (int K = 0; K < 98; ++K) G.Tick(0.05f); // 4.9초
		const bool bNotYet = G.Phase == EPDPhase::Build;
		for (int K = 0; K < 4; ++K) G.Tick(0.05f);   // 5.1초
		Expect(bFirstWaits && bNotYet && G.Phase == EPDPhase::Wave && G.Wave == 1 && G.AutoStartT < 0.f, "자동 시작: 첫 웨이브는 기다림, 보상 뒤 5초(속도 ×3 이어도)에 다음 웨이브");
	}
	{ // 발사체 고르기: 조커 → 족보(스트레이트 이상) → 무늬
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
		Expect(Base == EPDShot::Missile && PairShot == EPDShot::Missile && FlushShot == EPDShot::Arc && RoyalShot == EPDShot::Crown && JackShot == EPDShot::Blade && StarShot == EPDShot::StarComet,
			"발사체: ♥ 기본 미사일, 원 페어는 그대로, 플러시 번개, 로열 광선, 잭 칼날, 스타 별 혜성");
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
	{ // 조커: 대장 = 재료 중 가장 높은 족보
		using H = EPDHand;
		bool bMap = true;
		std::set<int> Seen;
		for (int K = 0; K < (int)H::Count; ++K) { bMap = bMap && CaptainHand(CaptainOf((H)K)) == (H)K; Seen.insert((int)CaptainOf((H)K)); }
		Expect(bMap && Seen.size() == 10, "족보 10개 ↔ 대장 조커 10개 (한 쌍씩)");
		Expect(MatchJokerRecipe({H::Pair, H::Pair, H::Pair}) == EPDJoker::Comedy && MatchJokerRecipe({H::Pair, H::TwoPair, H::Pair}) == EPDJoker::Mask &&
			MatchJokerRecipe({H::Pair, H::Four, H::Straight}) == EPDJoker::Cheshire && MatchJokerRecipe({H::RoyalFlush, H::Pair, H::Pair}) == EPDJoker::Star &&
			MatchJokerRecipe({H::Five, H::StraightFlush, H::Pair}) == EPDJoker::Clown && MatchJokerRecipe({H::Flush, H::Straight, H::Pair}) == EPDJoker::Harlequin,
			"재료 중 가장 높은 족보의 대장");
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
		const bool bGroup = Groups.size() == 1 && Groups[0].Result == EPDJoker::Baton && Groups[0].Cells.size() == 6;
		const bool bFused = G.JokerFuse(0);
		const FPDTower& J = G.Towers.begin()->second;
		Expect(bGroup && bFused && G.Towers.size() == 1 && J.IsJoker() && J.Joker == (int)EPDJoker::Baton && std::fabs(J.Mult - 6.f * JokerBonus) < 1e-4f && (G.Discovered & (1u << (int)EPDJoker::Baton)),
			"연쇄된 족보 셋(페어·스트레이트·페어) → 가장 높은 스트레이트의 대장 지휘봉 (위력 6 × 조커 덤), 도감에 기록");
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
		// 포카드를 기억하는 합성 타워(따로 선 레벨 3) + 족보 둘 → 체셔
		G.Towers.clear();
		PutT(0, Row, C(13), 3); PutT(1, Row, C(2)); PutT(2, Row, C(2, He)); PutT(3, Row, C(7)); PutT(4, Row, C(8, He));
		G.Towers[CellKey(0, Row)].FuseHand = (int)EPDHand::Four;
		G.Synergies = ApplyEffects(G.Towers, 0);
		Groups = G.JokerGroups();
		Expect(Groups.size() == 1 && Groups[0].Result == EPDJoker::Cheshire && Groups[0].FusedCells.size() == 1, "포카드를 기억한 합성 타워 + 족보 둘 → 포카드 대장 체셔");
		// 합성 타워는 족보를 기억한다
		G.Towers.clear();
		PutT(0, Row, C(6)); PutT(1, Row, C(6, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		G.Fuse(0);
		Expect(G.Towers.size() == 1 && G.Towers.begin()->second.FuseHand == (int)EPDHand::Pair, "합성 타워가 원 페어를 기억");
		// 합성 위치 고르기: 재료 칸 중 고른 칸으로, 재료 밖 칸이면 실패하고 판은 그대로
		G.Towers.clear();
		PutT(0, Row, C(4)); PutT(1, Row, C(4, He)); PutT(2, Row, C(4, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bBad = G.Fuse(0, CellKey(5, Row));
		const size_t AfterBad = G.Towers.size();
		const bool bPicked = G.Fuse(0, CellKey(2, Row));
		Expect(!bBad && AfterBad == 3 && bPicked && G.Towers.size() == 1 && G.Towers.count(CellKey(2, Row)) == 1 && G.Towers.at(CellKey(2, Row)).X == 2,
			"합성 위치: 트리플을 오른쪽 끝 칸에 모음, 재료 밖 칸은 거절");
		G.Towers.clear();
		PutT(0, Row, C(4)); PutT(1, Row, C(4, He)); PutT(2, Row, C(4, Di));
		G.Synergies = ApplyEffects(G.Towers, 0);
		Expect(DefaultFuseCell(G.Synergies[0].Cells) == CellKey(1, Row) && G.Fuse(0) && G.Towers.count(CellKey(1, Row)) == 1, "합성 위치를 안 고르면 가운데 칸 (예전과 같음)");
		// 조커도 고른 칸에 (페어 + 스트레이트 + 페어 → 지휘봉을 맨 왼쪽에)
		G.Towers.clear();
		PutT(0, Row, C(2)); PutT(1, Row, C(2, He)); PutT(2, Row, C(7)); PutT(3, Row, C(8, He)); PutT(4, Row, C(5)); PutT(5, Row, C(5, He));
		G.Synergies = ApplyEffects(G.Towers, 0);
		const bool bJBad = G.JokerFuse(0, CellKey(6, Row));
		const bool bJOk = G.JokerFuse(0, CellKey(0, Row));
		Expect(!bJBad && bJOk && G.Towers.size() == 1 && G.Towers.count(CellKey(0, Row)) == 1 && G.Towers.at(CellKey(0, Row)).IsJoker(), "조커 합성 위치: 고른 재료 칸에, 재료 밖은 거절");
		// 대장 지휘: 자기 족보 타워 +30%, 겹친다
		G.Towers.clear();
		PutT(3, Row, C(9)); PutT(4, Row, C(9, He));
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
		int Row = -1;
		for (int Y = 0; Y < Rows && Row < 0; ++Y) { bool bFree = true; for (int X = 0; X < 7; ++X) bFree = bFree && G.Buildable(X, Y); if (bFree) Row = Y; }
		Put(G.Towers, 0, Row, C(2)); Put(G.Towers, 1, Row, C(2, EPDSuit::Heart)); Put(G.Towers, 2, Row, C(7)); Put(G.Towers, 3, Row, C(8, EPDSuit::Heart)); Put(G.Towers, 4, Row, C(5)); Put(G.Towers, 5, Row, C(5, EPDSuit::Heart));
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
		double V = S.Dmg * S.Rate * S.CritFactor() * (1 + S.Splash * 0.8) * Coverage(G, T.X, T.Y, S.Range);
		if (T.IsJoker()) // 조커 능력 어림값
		{
			switch ((EPDJoker)T.Joker)
			{
			case EPDJoker::Comedy: V *= 1.8; break;                       // 두 발
			case EPDJoker::Star: V *= 4.0; break;                         // 모든 적 (한 번에 적 여럿)
			case EPDJoker::Bell: V *= 1.3; break;                         // 멈춤
			case EPDJoker::Mask:                                          // 옆 타워 따라 함
				if (T.Eff.CopyKey >= 0 && Tw.count(T.Eff.CopyKey)) { const FPDTowerStats O = TowerStats(Tw.at(T.Eff.CopyKey), M, &G.Mods); V = std::max(V, O.Dmg * O.Rate * O.CritFactor() * (1 + O.Splash * 0.8) * Coverage(G, T.X, T.Y, O.Range)); }
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
