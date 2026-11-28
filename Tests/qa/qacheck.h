// QA 불변식 검사 (QA 39, DOC/design/39-qa-endless-and-bugs.md 2절). 평소 빌드에는 안 들어간다.
// 쓰는 법: git apply Tests/qa/core_test-qacheck.patch 뒤 study 봇을 돌리면 어긋날 때 QAERR 줄, 끝에 QACOUNT 줄. 다 보면 git checkout Tests/core_test.cpp
#pragma once
#include <cmath>
#include <cstdio>
#include <set>
using namespace PD;
static int GQaErr = 0;
static int GQaN[5] = {0,0,0,0,0};
struct QaPrint { ~QaPrint() { std::printf("QACOUNT phase=%d feed=%d fuse=%d merge=%d craft=%d err=%d\n", GQaN[0], GQaN[1], GQaN[2], GQaN[3], GQaN[4], GQaErr); } };
static QaPrint GQaPrint;
#define QAERR(...) do { if (GQaErr++ < 400) { std::printf("QAERR seed=%d wave=%d ", Seed, G.Wave + 1); std::printf(__VA_ARGS__); std::printf("\n"); } } while (0)
static double QaCum(int L, float Xp) { double S = 0; for (int l = 1; l < L; ++l) S += JokerXpNeed(l); return S + Xp; }
static bool QaNear(double A, double B, double Rel = 2e-3) { return std::fabs(A - B) <= Rel * std::max(1.0, std::max(std::fabs(A), std::fabs(B))); }
struct QaJ { int Type, Lv; float Xp, Mult, TDmg, TRate; };
static std::map<int, QaJ> QaJokers(const FPDGame& G)
{
	std::map<int, QaJ> M;
	for (const auto& KV : G.Towers) if (KV.second.IsJoker()) M[KV.first] = {KV.second.Joker, KV.second.JLevel, KV.second.JXp, KV.second.Mult, KV.second.TDmg, KV.second.TRate};
	return M;
}
static void QaPhase(const FPDGame& G, int Seed)
{
	++GQaN[0];
	for (const auto& KV : G.Towers)
	{
		const FPDTower& T = KV.second;
		if (KV.first != CellKey(T.X, T.Y)) QAERR("key mismatch key=%d x=%d y=%d", KV.first, T.X, T.Y);
		if (!std::isfinite(T.Mult) || T.Mult <= 0.f) QAERR("bad mult key=%d mult=%g joker=%d", KV.first, T.Mult, T.Joker);
		if (T.IsJoker())
		{
			if (T.JLevel < 1) QAERR("joker lv<1 key=%d", KV.first);
			if (!std::isfinite(T.JXp) || T.JXp < -1e-3f || (T.JLevel < JokerMaxLevel && T.JXp >= JokerXpNeed(T.JLevel) + 1e-3f)) QAERR("joker xp out of range key=%d lv=%d xp=%g need=%g", KV.first, T.JLevel, T.JXp, JokerXpNeed(T.JLevel));
			if (T.Joker >= (int)EPDJoker::Count && !IsUpperJoker(T.Joker)) QAERR("bad joker type %d", T.Joker);
		}
	}
	for (int I = 0; I < (int)G.Synergies.size(); ++I)
		for (int K : G.Synergies[I].Cells)
		{
			auto It = G.Towers.find(K);
			if (It == G.Towers.end()) QAERR("synergy %d cell %d missing", I, K);
			else if (It->second.IsJoker()) QAERR("synergy %d has joker cell %d", I, K);
		}
	for (const FPDFeed& F : G.FeedTargets()) if (F.Syn < 0 || F.Syn >= (int)G.Synergies.size()) QAERR("feed target bad syn %d", F.Syn);
	for (const FPDJokerMerge& M : G.JokerMerges())
	{
		for (int K : M.Cells) if (!G.Towers.count(K) || !G.Towers.at(K).IsJoker()) QAERR("merge cell %d not joker", K);
		if (M.Upper >= 0)
		{
			if (M.Cells.size() != 2) QAERR("upper merge cells=%d", (int)M.Cells.size());
			else
			{
				const int A = G.Towers.at(M.Cells[0]).Joker, B = G.Towers.at(M.Cells[1]).Joker;
				const FPDUpperDef& D = UpperDef(M.Upper);
				if (!((A == (int)D.A && B == (int)D.B) || (A == (int)D.B && B == (int)D.A))) QAERR("upper %d recipe mismatch %d+%d", M.Upper, A, B);
			}
		}
	}
}
static bool QaFeed(FPDGame& G, int Syn, int Seed)
{
	++GQaN[1];
	const auto Before = QaJokers(G);
	const size_t NBefore = G.Towers.size();
	float Xp = -1.f, Val = 0.f;
	for (const FPDFeed& F : G.FeedTargets()) if (F.Syn == Syn) Xp = F.Xp;
	const std::vector<int> Cells = G.Synergies[Syn].Cells;
	for (int K : Cells) { const FPDTower& T = G.Towers.at(K); Val += (3.f + T.Card.Power()) * T.Mult * T.TDmg * T.TRate; }
	const float Mul = G.Has(EPDCharm::Feeder) ? 1.5f : 1.f;
	const bool Ok = G.Feed(Syn);
	if (!Ok) { QAERR("feed failed syn=%d", Syn); return false; }
	if (G.Towers.size() != NBefore - Cells.size()) QAERR("feed tower count %d -> %d (cells %d)", (int)NBefore, (int)G.Towers.size(), (int)Cells.size());
	for (int K : Cells) if (G.Towers.count(K)) QAERR("feed cell %d still there", K);
	for (const auto& KV : Before)
	{
		auto It = G.Towers.find(KV.first);
		if (It == G.Towers.end()) { QAERR("joker %d vanished by feed", KV.first); continue; }
		const double D = QaCum(It->second.JLevel, It->second.JXp) - QaCum(KV.second.Lv, KV.second.Xp);
		if (!QaNear(D, Xp * Mul, 1e-3)) QAERR("feed xp joker=%d type=%d got=%g want=%g lv %d->%d", KV.first, KV.second.Type, D, Xp * Mul, KV.second.Lv, It->second.JLevel);
		const float WantGain = G.FeedGain(Val, It->second);
		if (!QaNear(It->second.Mult - KV.second.Mult, WantGain, 1e-3)) QAERR("feed gain joker=%d got=%g want=%g", KV.first, It->second.Mult - KV.second.Mult, WantGain);
	}
	return true;
}
static bool QaFuse(FPDGame& G, int Idx, int Cell, int Seed)
{
	++GQaN[2];
	const std::vector<FPDJokerGroup> Gs = G.JokerGroups();
	if (Idx < 0 || Idx >= (int)Gs.size()) { QAERR("fuse bad idx"); return false; }
	const FPDJokerGroup Gr = Gs[Idx];
	const auto Before = QaJokers(G);
	const size_t NBefore = G.Towers.size();
	float Sum = 0.f;
	for (int K : Gr.Cells) { if (!G.Towers.count(K)) QAERR("fuse cell %d missing", K); else { Sum += G.Towers.at(K).Mult; if (G.Towers.at(K).IsJoker()) QAERR("fuse uses joker cell %d", K); } }
	// 같은 칸을 두 번 쓰지 않는지
	std::set<int> U(Gr.Cells.begin(), Gr.Cells.end());
	if (U.size() != Gr.Cells.size()) QAERR("fuse duplicate cells");
	// 레시피와 재료 족보 일치
	std::vector<int> Need; for (EPDHand H : JokerRecipe(Gr.Result)) Need.push_back((int)H);
	const float Want = Sum * G.JokerMult() * JokerTier(Gr.Result) * (1.f + JokerCardBonus * (float)Gr.Cells.size());
	if (!G.JokerFuse(Idx, Cell)) { QAERR("fuse failed"); return false; }
	if (G.Towers.size() != NBefore - Gr.Cells.size() + 1) QAERR("fuse tower count %d -> %d (cells %d)", (int)NBefore, (int)G.Towers.size(), (int)Gr.Cells.size());
	int NewK = -1;
	for (const auto& KV : G.Towers) if (KV.second.IsJoker() && !Before.count(KV.first)) NewK = KV.first;
	if (NewK < 0) { QAERR("fuse no new joker"); return true; }
	const FPDTower& J = G.Towers.at(NewK);
	if (J.Joker != (int)Gr.Result) QAERR("fuse type %d want %d", J.Joker, (int)Gr.Result);
	if (!U.count(NewK)) QAERR("fuse target %d outside cells", NewK);
	if (!QaNear(J.Mult, Want)) QAERR("fuse mult %g want %g", J.Mult, Want);
	if (J.JLevel != 1 || J.JXp != 0.f) QAERR("fuse new joker lv=%d xp=%g", J.JLevel, J.JXp);
	for (int K : Gr.Cells) if (K != NewK && G.Towers.count(K)) QAERR("fuse material %d left", K);
	(void)Need;
	return true;
}
static bool QaMerge(FPDGame& G, int Idx, int Seed)
{
	++GQaN[3];
	const std::vector<FPDJokerMerge> Ms = G.JokerMerges();
	if (Idx < 0 || Idx >= (int)Ms.size()) { QAERR("merge bad idx"); return false; }
	const FPDJokerMerge M = Ms[Idx];
	float Sum = 0.f; int Lv = 1, Tips = 0;
	for (int K : M.Cells) { const FPDTower& T = G.Towers.at(K); Sum += T.Mult; Lv = std::max(Lv, T.JLevel); Tips += G.Mods.Up(T.Joker) ? 1 : 0; }
	const size_t NBefore = G.Towers.size();
	if (!G.MergeJokers(Idx)) { QAERR("merge failed"); return false; }
	if (G.Towers.size() != NBefore - M.Cells.size() + 1) QAERR("merge tower count");
	int K2 = -1;
	for (int K : M.Cells) if (G.Towers.count(K)) { if (K2 >= 0) QAERR("merge left two"); K2 = K; }
	if (K2 < 0) { QAERR("merge no result"); return true; }
	const FPDTower& J = G.Towers.at(K2);
	if (J.JLevel != Lv) QAERR("merge lv %d want %d", J.JLevel, Lv);
	if (M.Upper >= 0)
	{
		const float Bonus = G.Mods.Key(EPDKeystone::Crown) ? 2.2f : UpperDef(M.Upper).Special == EPDUpperSpecial::None ? UpperBonusPlain : UpperBonus;
		const float Want = Sum * Bonus * (1.f + UpperTipBonus * Tips + G.Mods.UpperPrep);
		if (J.Joker != UpperBase + M.Upper) QAERR("upper type %d want %d", J.Joker, UpperBase + M.Upper);
		if (!QaNear(J.Mult, Want)) QAERR("upper mult %g want %g", J.Mult, Want);
		if (!JokerHas(J.Joker, UpperDef(M.Upper).A) || !JokerHas(J.Joker, UpperDef(M.Upper).B)) QAERR("upper lacks parent ability");
	}
	else
	{
		if (J.Joker != M.Joker) QAERR("merge type changed");
		if (!QaNear(J.Mult, Sum * JokerMergeBonus)) QAERR("merge mult %g want %g", J.Mult, Sum * JokerMergeBonus);
	}
	return true;
}

static bool QaCraft(FPDGame& G, const std::vector<int>& Picks, int Cell, int Seed)
{
	++GQaN[4];
	const std::vector<FPDJokerNode> Ns = G.JokerNodes();
	const EPDJoker R = G.JokerCraftResult(Picks);
	const auto Before = QaJokers(G);
	const size_t NBefore = G.Towers.size();
	std::set<int> U; float Sum = 0.f;
	std::vector<int> Hands;
	for (int P : Picks)
	{
		if (P < 0 || P >= (int)Ns.size()) { QAERR("craft bad pick"); return G.JokerCraft(Picks, Cell); }
		Hands.push_back((int)Ns[P].Kind);
		for (int K : Ns[P].Cells) { U.insert(K); if (G.Towers.count(K) && G.Towers.at(K).IsJoker()) QAERR("craft uses joker cell %d", K); }
	}
	for (int K : U) Sum += G.Towers.at(K).Mult;
	// 레시피 확인 (정렬해서 비교)
	std::vector<int> Need; for (EPDHand H : JokerRecipe(R)) Need.push_back((int)H);
	std::sort(Need.begin(), Need.end()); std::sort(Hands.begin(), Hands.end());
	if (R != EPDJoker::Count && Need != Hands) QAERR("craft recipe mismatch joker=%d", (int)R);
	if (R != EPDJoker::Count && !G.JokerOpen((int)R)) QAERR("craft locked joker %d", (int)R);
	const float Want = Sum * G.JokerMult() * JokerTier(R) * (1.f + JokerCardBonus * (float)U.size());
	if (!G.JokerCraft(Picks, Cell)) return false;
	if (R == EPDJoker::Count) QAERR("craft succeeded with no recipe");
	if (G.Towers.size() != NBefore - U.size() + 1) QAERR("craft tower count %d -> %d (cells %d)", (int)NBefore, (int)G.Towers.size(), (int)U.size());
	int NewK = -1;
	for (const auto& KV : G.Towers) if (KV.second.IsJoker() && (!Before.count(KV.first))) NewK = KV.first;
	if (NewK < 0) { QAERR("craft no new joker"); return true; }
	const FPDTower& J = G.Towers.at(NewK);
	if (J.Joker != (int)R) QAERR("craft type %d want %d", J.Joker, (int)R);
	if (!U.count(NewK)) QAERR("craft target outside");
	if (!QaNear(J.Mult, Want)) QAERR("craft mult %g want %g", J.Mult, Want);
	if (J.JLevel != 1 || J.JXp != 0.f) QAERR("craft new joker lv=%d xp=%g", J.JLevel, J.JXp);
	for (const auto& KV : Before) if (!G.Towers.count(KV.first)) QAERR("craft removed joker %d", KV.first);
	return true;
}
