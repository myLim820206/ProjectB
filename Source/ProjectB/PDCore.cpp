#include "PDCore.h"

#include <algorithm>
#include <cmath>
#include <set>

namespace PD
{
// ---------------------------------------------------------------- 표
static const FPDSuitDef GPDSuits[5] = {
	{1.6f, 0.8f, 3.0f, 0.f, 0.f},  // ♠ 저격
	{0.9f, 0.9f, 2.2f, 0.8f, 0.f}, // ♥ 폭발
	{0.45f, 2.6f, 2.2f, 0.f, 0.f}, // ♦ 연사
	{0.7f, 1.0f, 2.4f, 0.f, 0.4f}, // ♣ 둔화
	{1.0f, 1.2f, 2.6f, 0.f, 0.f},  // ★ 와일드
};
static const FPDEnemyDef GPDEnemies[(int)EPDEnemy::Count] = {
	{1.f, 1.1f, 0.f, 0, false, false},     // 보통
	{0.6f, 1.9f, 0.f, 0, false, false},    // 빠름
	{2.6f, 0.7f, 0.f, 0, false, false},    // 탱크
	{1.3f, 0.9f, 0.f, 0, true, false},     // 방패병
	{1.2f, 1.0f, 0.f, 2, false, false},    // 분열체
	{1.4f, 0.9f, 0.03f, 0, false, false},  // 재생자 (초당 최대 체력 3%)
	{0.35f, 1.5f, 0.f, 0, false, false},   // 분열 조각
	{12.f, 0.55f, 0.f, 0, true, true},     // 보스: 방패 거인
	{12.f, 0.55f, 0.f, 4, false, true},    // 보스: 분열 군주 (탱크 넷으로)
	{16.f, 0.5f, 0.015f, 0, false, true},  // 보스: 재생 왕
};
const FPDEnemyDef& EnemyDef(EPDEnemy T) { return GPDEnemies[(int)T]; }
const FPDSuitDef& SuitDef(EPDSuit S) { return GPDSuits[(int)S]; }

static const int GPDHandVal[(int)EPDHand::Count] = {120, 110, 100, 90, 80, 30, 28, 60, 45, 20};
int HandBaseValue(EPDHand H) { return GPDHandVal[(int)H]; }
int HandValue(EPDHand H, int N) { return GPDHandVal[(int)H] + ((H == EPDHand::Flush || H == EPDHand::Straight) ? (N - 3) * 22 : 0); }


// ---------------------------------------------------------------- 스킬 트리 표 (tools/tree/gen_tree.py 가 만든 PDTreeData.inl)
#include "PDTreeData.inl"
namespace
{
const std::vector<std::vector<int>>& PDTreeAdj()
{
	static const std::vector<std::vector<int>> Adj = [] {
		std::vector<std::vector<int>> A(sizeof(GPDTreeNodes) / sizeof(GPDTreeNodes[0]));
		for (const auto& L : GPDTreeLinks) { A[L[0]].push_back(L[1]); A[L[1]].push_back(L[0]); }
		return A;
	}();
	return Adj;
}
} // namespace
static_assert(sizeof(GPDTreeNodes) / sizeof(GPDTreeNodes[0]) <= TreeMaxNodes, "TreeMaxNodes 를 늘릴 것");

int TreeNodeCount() { return (int)(sizeof(GPDTreeNodes) / sizeof(GPDTreeNodes[0])); }
const FPDTreeNode& TreeNode(int Id) { return GPDTreeNodes[Id]; }
const std::vector<int>& TreeLinks(int Id) { return PDTreeAdj()[Id]; }

FPDTreeMods BuildTreeMods(const FPDTreeBits& Owned)
{
	FPDTreeMods M;
	for (int Id = 0; Id < TreeNodeCount(); ++Id)
	{
		if (!Owned.test(Id)) continue;
		for (const FPDNodeFx& F : TreeNode(Id).Fx)
		{
			auto Hands = [&](float* Arr) { for (int H = 0; H < (int)EPDHand::Count; ++H) if (F.Param & (1 << H)) Arr[H] += F.Value; };
			switch (F.Stat)
			{
			case EPDStat::SuitDmg: M.SuitDmg[F.Param] += F.Value; break;
			case EPDStat::SuitRate: M.SuitRate[F.Param] += F.Value; break;
			case EPDStat::SuitRange: M.SuitRange[F.Param] += F.Value; break;
			case EPDStat::SuitSplash: M.SuitSplash[F.Param] += F.Value; break;
			case EPDStat::SuitSlow: M.SuitSlow[F.Param] += F.Value; break;
			case EPDStat::HandDmg: Hands(M.HandDmg); break;
			case EPDStat::HandRate: Hands(M.HandRate); break;
			case EPDStat::HandSplash: Hands(M.HandSplash); break;
			case EPDStat::AllDmg: M.AllDmg += F.Value; break;
			case EPDStat::ChainAdd: M.ChainAdd += F.Value; break;
			case EPDStat::FuseAdd: M.FuseAdd += F.Value; break;
			case EPDStat::FusedDmg: M.FusedDmg += F.Value; break;
			case EPDStat::JokerDmg: M.JokerDmg += F.Value; break;
			case EPDStat::JokerAdd: M.JokerAdd += F.Value; break;
			case EPDStat::AllRate: M.AllRate += F.Value; break;
			case EPDStat::CritChance: M.CritChance += F.Value; break;
			case EPDStat::CritMult: M.CritMult += F.Value; break;
			case EPDStat::GoldPct: M.GoldPct += F.Value; break;
			case EPDStat::HandMax: M.HandAdd += (int)F.Value; break;
			case EPDStat::Draw: M.DrawAdd += (int)F.Value; break;
			case EPDStat::Keystone: M.Keys |= 1u << F.Param; break;
			case EPDStat::Joker: M.JokerUp |= 1u << F.Param; break;
			default: break; // Lives 는 살 때 한 번
			}
		}
	}
	return M;
}

bool FPDTreeMods::Up(const FPDTower& T) const { return T.IsJoker() && (Up(T.Joker) || T.JLevel >= JokerUpLevel); }

int UpgradeCost(int Level) { return 10 + 2 * Level; } // 10, 12, 14 … 9999레벨까지 약 1억
float UpgradeStep(EPDUpgrade U)
{
	switch (U)
	{
	case EPDUpgrade::Attack: return 0.01f;    // 공격력 +1%
	case EPDUpgrade::Speed: return 0.01f;     // 공격 속도 +1%
	case EPDUpgrade::Crit: return 0.001f;     // 치명타 확률 +0.1%p (100% 넘으면 한 방에 여러 번)
	case EPDUpgrade::CritMult: return 0.01f;  // 치명타 배율 +0.01
	default: return 0.02f;                    // 무늬 강화: 그 무늬 공격력 +2%
	}
}
void ApplyUpgrades(FPDTreeMods& M, const std::array<int, (int)EPDUpgrade::Count>& Lv)
{
	M.UpgDmg = UpgradeStep(EPDUpgrade::Attack) * Lv[(int)EPDUpgrade::Attack];
	M.UpgRate = UpgradeStep(EPDUpgrade::Speed) * Lv[(int)EPDUpgrade::Speed];
	M.UpgCrit = UpgradeStep(EPDUpgrade::Crit) * Lv[(int)EPDUpgrade::Crit];
	M.UpgCritMult = UpgradeStep(EPDUpgrade::CritMult) * Lv[(int)EPDUpgrade::CritMult];
	for (int S = 0; S < 4; ++S) M.UpgSuit[S] = UpgradeStep((EPDUpgrade)((int)EPDUpgrade::Spade + S)) * Lv[(int)EPDUpgrade::Spade + S];
}

std::vector<EPDHand> ClassifyHand(const std::vector<FPDCard>& Cards)
{
	// 와일드 카드: 가능한 숫자·무늬를 다 넣어 보고 나오는 족보를 모두 모은다
	for (size_t W = 0; W < Cards.size(); ++W)
	{
		if (!Cards[W].IsWild()) continue;
		std::vector<EPDSuit> Suits;
		for (const FPDCard& C : Cards) if (!C.IsWild() && std::find(Suits.begin(), Suits.end(), C.Suit) == Suits.end()) Suits.push_back(C.Suit);
		if (Suits.empty()) Suits.push_back(EPDSuit::Spade);
		std::set<EPDHand> All;
		std::vector<FPDCard> Try = Cards;
		for (int R = 2; R <= 14; ++R)
			for (EPDSuit Su : Suits)
			{
				Try[W] = {R, Su};
				for (EPDHand H : ClassifyHand(Try)) All.insert(H);
			}
		return std::vector<EPDHand>(All.begin(), All.end());
	}
	const int N = (int)Cards.size();
	std::map<int, int> Cnt;
	for (const FPDCard& C : Cards) Cnt[C.Rank]++;
	std::vector<int> Vals;
	for (auto& KV : Cnt) Vals.push_back(KV.second);
	std::sort(Vals.rbegin(), Vals.rend());
	bool bSameSuit = true;
	for (const FPDCard& C : Cards) bSameSuit = bSameSuit && C.Suit == Cards[0].Suit;
	bool bStraight = false;
	if ((int)Vals.size() == N && N >= 2)
	{
		std::vector<int> Hi, Lo;
		for (const FPDCard& C : Cards) { Hi.push_back(C.Rank); Lo.push_back(C.Rank == 14 ? 1 : C.Rank); }
		std::sort(Hi.begin(), Hi.end());
		std::sort(Lo.begin(), Lo.end());
		auto Consec = [](const std::vector<int>& V) { for (size_t I = 1; I < V.size(); ++I) if (V[I] != V[I - 1] + 1) return false; return true; };
		bStraight = Consec(Hi) || Consec(Lo);
	}
	std::vector<EPDHand> Out;
	if (N == 5 && Vals[0] == 5) Out.push_back(EPDHand::Five);
	if (N == 5 && bSameSuit && bStraight)
	{
		int Lo = 99;
		for (const FPDCard& C : Cards) Lo = std::min(Lo, C.Rank);
		if (Lo == 10) Out.push_back(EPDHand::RoyalFlush);
	}
	if (N == 2 && Vals[0] == 2) Out.push_back(EPDHand::Pair);
	if (N == 3 && Vals[0] == 3) Out.push_back(EPDHand::Triple);
	if (N == 4 && Vals[0] == 4) Out.push_back(EPDHand::Four);
	if (N == 4 && Vals.size() == 2 && Vals[0] == 2 && Vals[1] == 2) Out.push_back(EPDHand::TwoPair);
	if (N == 5 && Vals.size() == 2 && Vals[0] == 3 && Vals[1] == 2) Out.push_back(EPDHand::FullHouse);
	if (N == 5 && bSameSuit && bStraight) Out.push_back(EPDHand::StraightFlush);
	if (N >= 3 && bSameSuit) Out.push_back(EPDHand::Flush);
	if (N >= 2 && bStraight) Out.push_back(EPDHand::Straight); // 이어진 숫자는 2장부터 (페어와 맞춤)
	return Out;
}

// ---------------------------------------------------------------- 시너지
static const int GPDN4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};

// ESU 방식으로 연결 묶음(2~KMax칸)을 한 번씩만 만든다: 시작점보다 번호가 큰 칸으로만 넓힌다
static void PDExtend(const std::vector<std::vector<int>>& Nb, std::vector<int>& Sub, const std::vector<int>& Ext, int Root, int KMax, std::vector<std::vector<int>>& Out)
{
	if (Sub.size() >= 2) Out.push_back(Sub);
	if ((int)Sub.size() == KMax) return;
	std::vector<int> E = Ext;
	while (!E.empty())
	{
		const int W = E.back();
		E.pop_back();
		std::set<int> Near;
		for (int U : Sub) for (int Z : Nb[U]) Near.insert(Z);
		std::vector<int> Next = E;
		for (int U : Nb[W])
			if (U > Root && std::find(Sub.begin(), Sub.end(), U) == Sub.end() && U != W && !Near.count(U) && std::find(E.begin(), E.end(), U) == E.end())
				Next.push_back(U);
		Sub.push_back(W);
		PDExtend(Nb, Sub, Next, Root, KMax, Out);
		Sub.pop_back();
	}
}

// 스트레이트는 순서대로 (D-108): 칸들이 맞닿아 한 줄기로 이어지고, 그 줄기를 따라 숫자가 1씩 오른다 (꺾여도 됨, 거꾸로 읽어도 됨).
// 조커·와일드는 아무 숫자, A 는 1 또는 14. 5칸이면 120가지 순서를 다 본다
static bool PDOrderedStraight(const std::vector<int>& Cells, const std::vector<FPDCard>& Cards)
{
	const int N = (int)Cells.size();
	std::vector<int> P(N);
	for (int I = 0; I < N; ++I) P[I] = I;
	auto Adjacent = [](int A, int B) { return std::abs(A % Cols - B % Cols) + std::abs(A / Cols - B / Cols) == 1; };
	do
	{
		bool bPath = true;
		for (int I = 1; I < N && bPath; ++I) bPath = Adjacent(Cells[P[I - 1]], Cells[P[I]]);
		if (!bPath) continue;
		for (const bool bAceLow : {false, true})
		{
			bool bHave = false, bOk = true;
			int Base = 0;
			for (int I = 0; I < N && bOk; ++I)
			{
				const FPDCard& C = Cards[P[I]];
				if (C.IsWild()) continue;
				const int B = ((C.Rank == 14 && bAceLow) ? 1 : C.Rank) - I;
				if (!bHave) { Base = B; bHave = true; }
				else bOk = B == Base;
			}
			if (bOk && (!bHave || (Base >= 1 && Base + N - 1 <= 14))) return true;
		}
	} while (std::next_permutation(P.begin(), P.end()));
	return false;
}

std::vector<FPDSynergy> ComputeSynergies(const FPDTowerMap& Towers)
{
	std::vector<FPDSynergy> Picked;
	std::set<int> Seen;
	for (const auto& KV : Towers)
	{
		if (Seen.count(KV.first)) continue;
		// 덩어리 찾기
		std::vector<int> Comp, Stack{KV.first};
		Seen.insert(KV.first);
		while (!Stack.empty())
		{
			const int K = Stack.back();
			Stack.pop_back();
			Comp.push_back(K);
			const int X = K % Cols, Y = K / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				const int NK = CellKey(NX, NY);
				if (Towers.count(NK) && !Seen.count(NK) && (Towers.at(NK).Level == Towers.at(K).Level || Towers.at(NK).IsJoker() || Towers.at(K).IsJoker())) { Seen.insert(NK); Stack.push_back(NK); } // 같은 레벨끼리만 이어짐 (조커는 아무 레벨과)
			}
		}
		if (Comp.size() < 2) continue;
		std::sort(Comp.begin(), Comp.end());
		std::map<int, int> Idx;
		for (int I = 0; I < (int)Comp.size(); ++I) Idx[Comp[I]] = I;
		std::vector<std::vector<int>> Nb(Comp.size());
		for (int I = 0; I < (int)Comp.size(); ++I)
		{
			const int X = Comp[I] % Cols, Y = Comp[I] / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				auto It = Idx.find(CellKey(NX, NY));
				if (It != Idx.end()) Nb[I].push_back(It->second);
			}
		}
		std::vector<std::vector<int>> Subs;
		for (int V = 0; V < (int)Comp.size(); ++V)
		{
			std::vector<int> Sub{V}, Ext;
			for (int U : Nb[V]) if (U > V) Ext.push_back(U);
			PDExtend(Nb, Sub, Ext, V, 5, Subs);
		}
		struct FCand { EPDHand Kind; int N, Value; std::vector<int> Cells; };
		std::vector<FCand> Cands;
		for (const auto& Sub : Subs)
		{
			std::vector<FPDCard> Cards;
			std::vector<int> Cells;
			int Lv = -1;
			bool bMixed = false;
			for (int I : Sub)
			{
				const FPDTower& T = Towers.at(Comp[I]);
				Cells.push_back(Comp[I]);
				Cards.push_back(T.IsJoker() ? FPDCard{0, EPDSuit::Wild} : T.Card); // 조커는 와일드
				if (!T.IsJoker()) { if (Lv < 0) Lv = T.Level; else if (Lv != T.Level) bMixed = true; }
			}
			if (bMixed) continue; // 조커가 아닌 타워는 레벨이 같아야 족보
			int Ordered = -1; // 스트레이트 순서 검사 (필요할 때 한 번만)
			for (EPDHand H : ClassifyHand(Cards))
			{
				if (H == EPDHand::Straight || H == EPDHand::StraightFlush || H == EPDHand::RoyalFlush)
				{
					if (Ordered < 0) Ordered = PDOrderedStraight(Cells, Cards) ? 1 : 0;
					if (!Ordered) continue;
				}
				Cands.push_back({H, (int)Sub.size(), HandValue(H, (int)Sub.size()), Cells});
			}
		}
		std::stable_sort(Cands.begin(), Cands.end(), [](const FCand& A, const FCand& B) { return A.Value != B.Value ? A.Value > B.Value : A.N > B.N; });
		// 카드 한 장이 여러 족보에 같이 들 수 있다 (D-097). 큰 족보부터 고르되
		//  1) 아직 어느 족보에도 안 든 카드가 한 장 이상 있어야 한다 (트리플 안의 페어 같은 부분 족보는 따로 안 셈)
		//  2) 같은 종류 족보끼리는 카드를 나눠 쓰지 않는다
		std::set<int> Used;
		std::map<int, std::set<int>> UsedBy; // 족보 종류 → 쓰인 칸
		for (const FCand& C : Cands)
		{
			bool bNew = false, bClash = false;
			std::set<int>& Same = UsedBy[(int)C.Kind];
			for (int K : C.Cells) { bNew = bNew || !Used.count(K); bClash = bClash || Same.count(K); }
			if (!bNew || bClash) continue;
			for (int K : C.Cells) { Used.insert(K); Same.insert(K); }
			FPDSynergy S;
			S.Kind = C.Kind;
			S.N = C.N;
			S.Value = C.Value;
			S.Cells = C.Cells;
			Picked.push_back(S);
		}
	}
	// 연쇄: 족보끼리 맞닿아 있거나 카드를 나눠 쓰면 하나의 연쇄
	std::map<int, std::vector<int>> Owner;
	for (int I = 0; I < (int)Picked.size(); ++I) for (int K : Picked[I].Cells) Owner[K].push_back(I);
	std::vector<std::set<int>> Adj(Picked.size());
	for (int I = 0; I < (int)Picked.size(); ++I)
		for (int K : Picked[I].Cells)
		{
			for (int J : Owner[K]) if (J != I) Adj[I].insert(J);
			const int X = K % Cols, Y = K / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				auto It = Owner.find(CellKey(NX, NY));
				if (It != Owner.end()) for (int J : It->second) if (J != I) Adj[I].insert(J);
			}
		}
	int Cid = 0;
	for (int I = 0; I < (int)Picked.size(); ++I)
	{
		if (Picked[I].ChainId >= 0) continue;
		std::vector<int> Mem, St{I};
		Picked[I].ChainId = Cid;
		while (!St.empty())
		{
			const int K = St.back();
			St.pop_back();
			Mem.push_back(K);
			for (int J : Adj[K]) if (Picked[J].ChainId < 0) { Picked[J].ChainId = Cid; St.push_back(J); }
		}
		for (int K : Mem) Picked[K].Chain = (int)Mem.size();
		Cid++;
	}
	for (int I = 0; I < (int)Picked.size(); ++I) Picked[I].Links.assign(Adj[I].begin(), Adj[I].end());
	return Picked;
}

std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers, uint32_t Charms, const FPDTreeMods* Tree)
{
	static const FPDTreeMods NoTree;
	const FPDTreeMods& Tm = Tree ? *Tree : NoTree;
	std::vector<FPDSynergy> Syn = ComputeSynergies(Towers);
	const float Step = (HasCharm(Charms, EPDCharm::ChainLink) ? 0.45f : ChainStep) + Tm.ChainAdd + (Tm.Key(EPDKeystone::Crowd) ? 0.15f : 0.f);
	const float PairK = HasCharm(Charms, EPDCharm::Twins) ? 2.f : 1.5f;
	for (auto& KV : Towers) KV.second.Eff = FPDEffects{};
	for (int I = 0; I < (int)Syn.size(); ++I)
		for (int K : Syn[I].Cells)
		{
			FPDEffects& E = Towers[K].Eff;
			const int N = Syn[I].N;
			if (E.Hand < 0) { E.Hand = I; E.Kind = (int)Syn[I].Kind; E.N = N; } // 대표 족보: 가장 큰 것 (Syn 은 큰 족보부터)
			E.Kinds |= 1u << (int)Syn[I].Kind;
			const bool bSkip = Tm.Key(EPDKeystone::BigTable) && N == 2; // 큰 판: 2장 족보는 효과 없음
			if (!bSkip) switch (Syn[I].Kind)
			{
			case EPDHand::Pair: E.Dmg *= PairK; break;
			case EPDHand::TwoPair: E.Dmg *= PairK; E.Range *= 1.25f; break;
			case EPDHand::Triple: E.Dmg *= 2.f; break;
			case EPDHand::Straight: E.Rate *= 1.f + (HasCharm(Charms, EPDCharm::LineLover) ? 0.5f : 0.25f) * (N - 1); break;
			case EPDHand::Flush: if (!Tm.Key(EPDKeystone::Omni)) E.Splash = std::max(E.Splash, 0.5f + 0.25f * N + (HasCharm(Charms, EPDCharm::WideFlush) ? 0.75f : 0.f)); break; // 잡식가: 플러시 효과 없음
			case EPDHand::FullHouse: E.Dmg *= 1.5f; E.Rate *= 1.5f; E.Splash = std::max(E.Splash, 1.5f); break;
			case EPDHand::Four: E.Dmg *= 3.5f; break;
			case EPDHand::StraightFlush: E.Dmg *= 2.f; E.Rate *= 2.f; E.Splash = std::max(E.Splash, 2.f); break;
			case EPDHand::Five: E.Dmg *= 5.f; break;
			case EPDHand::RoyalFlush: E.Dmg *= 2.5f; E.Rate *= 2.5f; E.Splash = std::max(E.Splash, 2.5f); break;
			default: break;
			}
			const bool bNoChain = bSkip || Tm.Key(EPDKeystone::Lone) || (Tm.Key(EPDKeystone::SmallTable) && N == 5); // 고독한 저격수·작은 판(5장)은 연쇄 덤 없음
			if (Syn[I].Chain >= 2 && E.Chain < 2) { if (!bNoChain) E.Dmg *= 1.f + Step * (Syn[I].Chain - 1); E.Chain = Syn[I].Chain; } // 연쇄 덤은 타워마다 한 번 (족보를 나눠 쓰면 같은 연쇄)
		}
	// 대장 지휘: 판 위 자기 족보 타워(족보에 든 타워, 또는 그 족보를 기억하는 합성 타워) 위력 +30%씩, 겹친다
	int Captains[(int)EPDHand::Count] = {};
	for (auto& KV : Towers) if (KV.second.IsJoker()) Captains[(int)CaptainHand((EPDJoker)KV.second.Joker)]++;
	for (auto& KV : Towers)
	{
		FPDTower& O = KV.second;
		if (O.IsJoker()) continue;
		const uint32_t Bits = O.Eff.Kinds ? O.Eff.Kinds : (O.FuseHand >= 0 ? 1u << O.FuseHand : 0u);
		int Cmd = 0;
		for (int H = 0; H < (int)EPDHand::Count; ++H) if ((Bits >> H) & 1u) Cmd += Captains[H]; // 여러 족보에 든 타워는 대장마다 지휘를 받는다
		if (Cmd > 0) O.Eff.Dmg *= 1.f + CaptainCommand * Cmd;
	}
	// 조커 오라
	for (auto& KV : Towers)
	{
		const FPDTower& J = KV.second;
		if (!J.IsJoker()) continue;
		const EPDJoker Kind = (EPDJoker)J.Joker;
		float BestPow = -1.f;
		for (auto& OV : Towers)
		{
			FPDTower& O = OV.second;
			if (&O == &J || O.IsJoker()) continue;
			const bool bUp = Tm.Up(J); // 트리 조커 강화 또는 조커 레벨 10+
			const int Reach = (Kind == EPDJoker::Hatter && bUp) ? 2 : 1;
			const bool bNear = std::abs(O.X - J.X) <= Reach && std::abs(O.Y - J.Y) <= Reach; // 주변 8칸 (강화한 모자 장수는 24칸)
			if (Kind == EPDJoker::Baton && bNear) O.Eff.Rate *= bUp ? 1.7f : 1.4f;
			if (Kind == EPDJoker::Hatter && bNear) { O.Eff.Dmg *= bUp ? 1.5f : 1.25f; O.Eff.Range *= 1.1f; }
			if (Kind == EPDJoker::Harlequin && !O.Card.IsWild() && O.Card.Suit == J.JSuit) O.Eff.Dmg *= bUp ? 1.6f : 1.3f;
			if (Kind == EPDJoker::Mask && bNear)
			{
				const float Pow = (3.f + O.Card.Power()) * SuitDef(O.Card.Suit).Dmg * O.Mult * O.TDmg;
				if (Pow > BestPow) { BestPow = Pow; Towers[KV.first].Eff.CopyKey = OV.first; }
			}
		}
	}
	return Syn;
}

// 대장 표: 족보 → 조커 (그림 번호는 docs/15 9절)
static const EPDJoker GPDCaptain[(int)EPDHand::Count] = {
	EPDJoker::Star,      // 로열 스트레이트 플러시
	EPDJoker::Clown,     // 파이브 카드 (와일드의 상징)
	EPDJoker::Hatter,    // 스트레이트 플러시
	EPDJoker::Cheshire,  // 포카드
	EPDJoker::Bell,      // 풀하우스
	EPDJoker::Harlequin, // 플러시
	EPDJoker::Baton,     // 스트레이트
	EPDJoker::Jack,      // 트리플
	EPDJoker::Mask,      // 투 페어
	EPDJoker::Comedy,    // 원 페어
};
EPDJoker CaptainOf(EPDHand H) { return GPDCaptain[(int)H]; }
EPDHand CaptainHand(EPDJoker J)
{
	for (int H = 0; H < (int)EPDHand::Count; ++H) if (GPDCaptain[H] == J) return (EPDHand)H;
	return EPDHand::Pair;
}
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands)
{
	if (Hands.empty()) return EPDJoker::Comedy;
	EPDHand Best = Hands[0];
	for (EPDHand H : Hands) if (HandBaseValue(H) > HandBaseValue(Best)) Best = H;
	return CaptainOf(Best);
}

EPDShot ShotKind(const FPDTower& T)
{
	if (T.IsJoker()) return T.Joker >= 0 && T.Joker < (int)EPDJoker::Count ? (EPDShot)((int)EPDShot::Juggle + T.Joker) : EPDShot::JokerLaser;
	if (T.Eff.Kind >= 0) // 족보 타워: 스트레이트 이상은 족보 발사체 (원 페어·투 페어·트리플은 무늬 그대로)
	{
		switch ((EPDHand)T.Eff.Kind)
		{
		case EPDHand::Straight: return EPDShot::Arrow;
		case EPDHand::Flush: return EPDShot::Arc;
		case EPDHand::FullHouse: return EPDShot::Cluster;
		case EPDHand::Four: return EPDShot::Shell;
		case EPDHand::StraightFlush: return EPDShot::Comet;
		case EPDHand::Five: return EPDShot::Nova;
		case EPDHand::RoyalFlush: return EPDShot::Crown;
		default: break;
		}
	}
	switch (T.Card.Suit)
	{
	case EPDSuit::Spade: return EPDShot::Laser;
	case EPDSuit::Heart: return EPDShot::Missile;
	case EPDSuit::Diamond: return EPDShot::Bullet;
	case EPDSuit::Club: return EPDShot::Orb;
	default: return EPDShot::Star;
	}
}
float ShotSpeed(EPDShot K)
{
	switch (K)
	{
	case EPDShot::Missile: return 9.f;
	case EPDShot::Bullet: return 22.f;
	case EPDShot::Orb: return 10.f;
	case EPDShot::Star: return 15.f;
	case EPDShot::Arrow: return 20.f;
	case EPDShot::Cluster: return 11.f;
	case EPDShot::Shell: return 8.f;
	case EPDShot::Comet: return 15.f;
	case EPDShot::Nova: return 12.f;
	case EPDShot::Juggle: return 10.f;
	case EPDShot::Hat: return 11.f;
	case EPDShot::Crescent: return 13.f;
	case EPDShot::Twin: return 12.f;
	case EPDShot::Shards: return 17.f;
	case EPDShot::Note: return 10.f;
	case EPDShot::Blade: return 9.f;
	case EPDShot::Wave: return 12.f;
	case EPDShot::StarComet: return 15.f;
	default: return 0.f; // 레이저, 번개, 로열 광선, 체셔 광선은 즉시
	}
}

FPDTowerStats TowerStats(const FPDTower& T, uint32_t Charms, const FPDTreeMods* Tree)
{
	static const FPDTreeMods NoTree;
	const FPDTreeMods& Tm = Tree ? *Tree : NoTree;
	// 광대 조커는 무늬 4종 공격을 번갈아 쓴다
	const FPDSuitDef& S = (T.IsJoker() && T.Joker == (int)EPDJoker::Clown) ? SuitDef((EPDSuit)(T.Cycle % 4)) : SuitDef(T.Card.Suit);
	const int R = T.Card.Power();
	FPDTowerStats St{(3.f + R) * S.Dmg * T.Eff.Dmg * T.Mult * T.TDmg, S.Rate * T.Eff.Rate * T.TRate, S.Range * T.Eff.Range * T.TRange * (1.f + LevelRange * (T.Level - 1)),
		std::max({S.Splash, T.Eff.Splash, T.TSplash}), S.Slow};
	static const EPDCharm SuitJ[4] = {EPDCharm::SpadeLover, EPDCharm::HeartLover, EPDCharm::DiaLover, EPDCharm::ClubLover};
	if (!T.Card.IsWild() && HasCharm(Charms, SuitJ[(int)T.Card.Suit])) St.Dmg *= 1.4f;
	if (HasCharm(Charms, EPDCharm::FaceCards) && R >= 11 && R <= 13) St.Dmg *= 1.6f;
	if (HasCharm(Charms, EPDCharm::SmallNums) && !T.Card.IsWild() && R <= 6) St.Rate *= 1.6f;
	if (HasCharm(Charms, EPDCharm::Lucky7) && R == 7) St.Dmg *= 2.5f;
	if (HasCharm(Charms, EPDCharm::Scope)) St.Range *= 1.15f;
	if (HasCharm(Charms, EPDCharm::Frost) && T.Card.Suit == EPDSuit::Club) { St.Slow = std::min(0.75f, St.Slow + 0.2f); St.Splash = std::max(St.Splash, 0.6f); }
	// 스킬 트리: 작은·특급 노드는 같은 종류끼리 더해서 한 번에 곱한다 (increased)
	const int Si = T.Card.IsWild() ? 4 : (int)T.Card.Suit;
	const int K = T.Eff.Kind;
	float HD = 0.f, HR = 0.f, HS = 0.f; // 트리 족보 노드: 타워가 든 족보마다 더한다
	for (int H = 0; H < (int)EPDHand::Count; ++H) if ((T.Eff.Kinds >> H) & 1u) { HD += Tm.HandDmg[H]; HR += Tm.HandRate[H]; HS += Tm.HandSplash[H]; }
	St.Dmg *= 1.f + Tm.AllDmg + Tm.SuitDmg[Si] + HD + (T.Level >= 2 && !T.IsJoker() ? Tm.FusedDmg : 0.f) + (T.IsJoker() ? Tm.JokerDmg : 0.f);
	St.Rate *= 1.f + Tm.AllRate + Tm.SuitRate[Si] + HR;
	St.Crit = BaseCrit + Tm.CritChance + Tm.UpgCrit;
	St.CritMult = BaseCritMult + Tm.CritMult + Tm.UpgCritMult;
	// 골드 강화 (따로 곱함)
	St.Dmg *= (1.f + Tm.UpgDmg) * (1.f + (Si < 4 ? Tm.UpgSuit[Si] : 0.f));
	St.Rate *= 1.f + Tm.UpgRate;
	St.Range *= 1.f + Tm.SuitRange[Si];
	if (St.Splash > 0.f) St.Splash += Tm.SuitSplash[Si] + HS;
	if (St.Slow > 0.f) St.Slow = std::min(0.75f, St.Slow + Tm.SuitSlow[Si]);
	// 키스톤 (more)
	const bool bFlushK = K == (int)EPDHand::Flush || K == (int)EPDHand::StraightFlush;
	const bool bAlone = K < 0 && !T.IsJoker();
	if (Tm.Key(EPDKeystone::Purist) && K >= 0) St.Dmg *= bFlushK ? 2.f : 0.5f;
	if (Tm.Key(EPDKeystone::Omni) && K >= 0 && K != (int)EPDHand::Flush) St.Dmg *= 1.3f;
	if (Tm.Key(EPDKeystone::BigTable) && T.Eff.N == 5) St.Dmg *= 1.6f;
	if (Tm.Key(EPDKeystone::SmallTable) && (K == (int)EPDHand::Pair || K == (int)EPDHand::TwoPair)) St.Dmg *= 1.8f;
	if (Tm.Key(EPDKeystone::Lone) && bAlone) St.Dmg *= 1.6f;
	if (Tm.Key(EPDKeystone::Crowd) && bAlone) St.Dmg *= 0.5f;
	if (T.IsJoker())
	{
		St.Dmg *= 1.f + JokerLevelDmg * (T.JLevel - 1); // 조커 레벨: 레벨마다 위력 +10%
		const bool bUp = Tm.Up(T);
		switch ((EPDJoker)T.Joker)
		{
		case EPDJoker::Clown: if (bUp) St.Rate *= 1.5f; break;                                     // 강화: 연사 +50%
		case EPDJoker::Jack: St.Dmg *= bUp ? 5.f : 3.f; St.Rate *= 0.5f; break;                    // 아주 강한 한 방
		case EPDJoker::Cheshire: St.Range = 99.f; if (bUp) St.Dmg *= 1.5f; break;                  // 사거리 무한
		case EPDJoker::Star: St.Dmg *= bUp ? 0.8f : 0.5f; St.Rate = 1.f; St.Range = 99.f; break;   // 판 전체 레이저 (모든 적)
		default: break;
		}
	}
	return St;
}

// ---------------------------------------------------------------- 합성
// 족보 효과를 특성으로 (부적과 무관한 기본값)
struct FPDTrait
{
	float Dmg = 1.f, Rate = 1.f, Range = 1.f, Splash = 0.f;
};
static FPDTrait HandTrait(EPDHand H, int N)
{
	FPDTrait T;
	switch (H)
	{
	case EPDHand::Pair: T.Dmg = 1.5f; break;
	case EPDHand::TwoPair: T.Dmg = 1.5f; T.Range = 1.25f; break;
	case EPDHand::Triple: T.Dmg = 2.f; break;
	case EPDHand::Straight: T.Rate = 1.f + 0.25f * (N - 1); break;
	case EPDHand::Flush: T.Splash = 0.5f + 0.25f * N; break;
	case EPDHand::FullHouse: T.Dmg = 1.5f; T.Rate = 1.5f; T.Splash = 1.5f; break;
	case EPDHand::Four: T.Dmg = 3.5f; break;
	case EPDHand::StraightFlush: T.Dmg = 2.f; T.Rate = 2.f; T.Splash = 2.f; break;
	case EPDHand::Five: T.Dmg = 5.f; break;
	case EPDHand::RoyalFlush: T.Dmg = 2.5f; T.Rate = 2.5f; T.Splash = 2.5f; break;
	default: break;
	}
	return T;
}

// ---------------------------------------------------------------- 길
void FPDPath::Build(const std::vector<std::array<int, 2>>& InCorners)
{
	Corners = InCorners;
	Points.clear();
	SegStart.clear();
	Length = 0.f;
	OnPath.assign(Cols * Rows, 0);
	for (const auto& C : Corners) Points.push_back({C[0] + 0.5f, C[1] + 0.5f});
	for (size_t I = 1; I < Points.size(); ++I)
	{
		SegStart.push_back(Length);
		Length += std::hypot(Points[I].X - Points[I - 1].X, Points[I].Y - Points[I - 1].Y);
	}
	for (size_t I = 1; I < Corners.size(); ++I)
	{
		int X = Corners[I - 1][0], Y = Corners[I - 1][1];
		const int BX = Corners[I][0], BY = Corners[I][1];
		const int DX = (BX > X) - (BX < X), DY = (BY > Y) - (BY < Y);
		while (true)
		{
			if (X >= 0 && X < Cols && Y >= 0 && Y < Rows) OnPath[CellKey(X, Y)] = 1;
			if (X == BX && Y == BY) break;
			X += DX;
			Y += DY;
		}
	}
}

FPDPoint FPDPath::At(float D) const
{
	for (size_t I = 1; I < Points.size(); ++I)
	{
		const float L = std::hypot(Points[I].X - Points[I - 1].X, Points[I].Y - Points[I - 1].Y);
		if (D <= SegStart[I - 1] + L)
		{
			const float U = L > 0.f ? (D - SegStart[I - 1]) / L : 0.f;
			return {Points[I - 1].X + (Points[I].X - Points[I - 1].X) * U, Points[I - 1].Y + (Points[I].Y - Points[I - 1].Y) * U};
		}
	}
	return Points.empty() ? FPDPoint{} : Points.back();
}

int FPDPath::CellCount() const
{
	int N = 0;
	for (size_t I = 1; I < Corners.size(); ++I) N += std::abs(Corners[I][0] - Corners[I - 1][0]) + std::abs(Corners[I][1] - Corners[I - 1][1]);
	return N;
}

std::vector<std::array<int, 2>> GeneratePath(std::mt19937& Rng)
{
	auto Ri = [&](int A, int B) { return std::uniform_int_distribution<int>(A, B)(Rng); };
	for (int Tries = 0; Tries < 500; ++Tries)
	{
		int X = -1, Y = Ri(1, Rows - 2);
		std::vector<std::array<int, 2>> Pts{{X, Y}};
		while (true)
		{
			const int NX = X + Ri(2, 4);
			if (NX >= Cols - 2) { Pts.push_back({Cols, Y}); break; }
			Pts.push_back({NX, Y});
			X = NX;
			int NY;
			do NY = Ri(1, Rows - 2); while (std::abs(NY - Y) < 2);
			Pts.push_back({X, NY});
			Y = NY;
		}
		int Cells = 0;
		for (size_t I = 1; I < Pts.size(); ++I) Cells += std::abs(Pts[I][0] - Pts[I - 1][0]) + std::abs(Pts[I][1] - Pts[I - 1][1]);
		if (Cells >= PathMin && Cells <= PathMax) return Pts;
	}
	return {{-1, 1}, {3, 1}, {3, 7}, {7, 7}, {7, 2}, {11, 2}, {11, 8}, {14, 8}, {14, 4}, {16, 4}};
}

// ---------------------------------------------------------------- 진행
FPDGame::FPDGame(uint32_t Seed) : Rng(Seed ? Seed : std::random_device{}()) { NewGame(false); }

void FPDGame::NewGame(bool bStart)
{
	Phase = bStart ? EPDPhase::Build : EPDPhase::Title;
	Wave = 0;
	AutoStartT = -1.f;
	Lives = StartLives;
	Kills = Leaks = Discarded = DeckRefills = 0;
	Gold = Points = GoldEarned = 0;
	GoldCarry = 0.f;
	TreeOwned = FPDTreeBits(1);
	Upg = {};
	GoldUpg = 0;
	Mods = FPDTreeMods{};
	Time = 0.f;
	Towers.clear();
	Synergies.clear();
	Enemies.clear();
	Pending.clear();
	Born.clear();
	Hand.clear();
	Deck.clear();
	Events.clear();
	Charms.clear();
	Rewards.clear();
	Projectiles.clear();
	for (int S = 0; S < 4; ++S) for (int R = 2; R <= 14; ++R) Deck.push_back({R, (EPDSuit)S});
	std::shuffle(Deck.begin(), Deck.end(), Rng);
	Path.Build(GeneratePath(Rng));
	Draw(StartHand);
}

int FPDGame::AddGold(float Amount)
{
	GoldCarry += Amount * (1.f + Mods.GoldPct);
	const int Got = (int)(GoldCarry + 1e-4f); // 0.15 × 20 이 2.9999 가 되지 않게
	GoldCarry -= Got;
	Gold += Got;
	GoldEarned += Got;
	return Got;
}

int FPDGame::KillGold(EPDEnemy T, int W) const
{
	const FPDEnemyDef& D = EnemyDef(T);
	if (D.bBoss) return 25;
	if (T == EPDEnemy::Mini) return 1;
	const int Base = T == EPDEnemy::Tank ? 3 : (T == EPDEnemy::Normal || T == EPDEnemy::Fast) ? 1 : 2;
	return Base + (W - 1) / 5; // 5웨이브마다 +1
}

int FPDGame::PointPrice() const { return (int)std::lround(PointBasePrice * std::pow(PointPriceGrowth, (float)Points)); }
float FPDGame::FuseMult() const { return FuseBonus + Mods.FuseAdd + (Mods.Key(EPDKeystone::Smith) ? 0.5f : 0.f); }
float FPDGame::JokerMult() const { return JokerBonus + Mods.JokerAdd + (Mods.Key(EPDKeystone::ClownPath) ? 0.4f : 0.f); }

void FPDGame::RebuildMods()
{
	Mods = BuildTreeMods(TreeOwned);
	ApplyUpgrades(Mods, Upg);
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
}

int FPDGame::UpgradeBatchCost(EPDUpgrade U, int N) const
{
	const int L = Upg[(int)U];
	const int Count = std::max(0, std::min(N, UpgradeMax - L));
	long long Sum = 0;
	for (int I = 0; I < Count; ++I) Sum += UpgradeCost(L + I);
	return (int)std::min<long long>(Sum, 2000000000LL);
}

int FPDGame::Upgrade(EPDUpgrade U, int N)
{
	if (Phase != EPDPhase::Build && Phase != EPDPhase::Wave) return 0;
	int Done = 0;
	int& L = Upg[(int)U];
	while (Done < N && L < UpgradeMax && Gold >= UpgradeCost(L))
	{
		Gold -= UpgradeCost(L);
		GoldUpg += UpgradeCost(L);
		L++;
		Done++;
	}
	if (Done > 0)
	{
		RebuildMods();
		Sfx(EPDSound::Rotate);
	}
	return Done;
}

int FPDGame::KeystoneCount() const
{
	int N = 0;
	for (int Id = 0; Id < TreeNodeCount(); ++Id) N += TreeOwnedNode(Id) && TreeNode(Id).Kind == EPDNode::Keystone;
	return N;
}

bool FPDGame::TreeSealed(int Id) const
{
	const FPDTreeNode& N = TreeNode(Id);
	return N.Kind == EPDNode::Keystone && !TreeOwnedNode(Id) && Mods.Key((EPDKeystone)(N.Fx[0].Param ^ 1));
}

FPDGame::EBuy FPDGame::CanBuy(int Id) const
{
	if (Phase != EPDPhase::Build && Phase != EPDPhase::Wave) return EBuy::Closed;
	if (Id < 0 || Id >= TreeNodeCount()) return EBuy::Closed;
	if (TreeOwnedNode(Id)) return EBuy::Owned;
	if (TreeSealed(Id)) return EBuy::Sealed;
	bool bLinked = false;
	for (int O : TreeLinks(Id)) bLinked = bLinked || TreeOwnedNode(O);
	if (!bLinked) return EBuy::NotLinked;
	if (TreeNode(Id).Kind == EPDNode::Keystone && KeystoneCount() >= KeystoneMax) return EBuy::KeyCap;
	if (Gold < PointPrice()) return EBuy::NoGold;
	return EBuy::Ok;
}

bool FPDGame::BuyNode(int Id)
{
	if (CanBuy(Id) != EBuy::Ok) return false;
	Gold -= PointPrice();
	Points++;
	TreeOwned.set(Id);
	for (const FPDNodeFx& F : TreeNode(Id).Fx) if (F.Stat == EPDStat::Lives) Lives += (int)F.Value;
	RebuildMods();
	FPDEvent E;
	E.Kind = FPDEvent::EKind::TreeBuy;
	E.A = Id;
	Ev(E);
	Sfx(TreeNode(Id).Kind == EPDNode::Keystone || TreeNode(Id).Kind == EPDNode::Joker ? EPDSound::Win : EPDSound::Rotate);
	return true;
}

uint32_t FPDGame::CharmMask() const
{
	uint32_t M = 0;
	for (EPDCharm J : Charms) M |= 1u << (int)J;
	return M;
}

void FPDGame::Draw(int N, int* OutGot)
{
	int Got = 0;
	while (Got < N && (int)Hand.size() < HandMax())
	{
		if (Deck.empty()) // 무한 진행: 덱이 비면 새 덱 한 벌을 섞어 채운다
		{
			for (int S = 0; S < 4; ++S) for (int R = 2; R <= 14; ++R) Deck.push_back({R, (EPDSuit)S});
			std::shuffle(Deck.begin(), Deck.end(), Rng);
			DeckRefills++;
		}
		Hand.push_back(Deck.back());
		Deck.pop_back();
		Got++;
	}
	if (OutGot) *OutGot = Got;
}

bool FPDGame::Buildable(int X, int Y) const { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && !Path.IsPath(X, Y); }

FPDEnemy FPDGame::MakeEnemy(EPDEnemy T, float BaseHp, int W)
{
	const FPDEnemyDef& D = EnemyDef(T);
	FPDEnemy E;
	E.Type = T;
	E.Hp = E.MaxHp = BaseHp * D.Hp;
	E.Speed = D.Speed;
	E.Armor = D.bArmor ? (D.bBoss ? 15.f : 4.f + W * 0.6f) : 0.f;
	E.BaseHp = BaseHp;
	E.Wave = W;
	E.Id = NextId++;
	return E;
}

int WaveCount(int W) { return 12 + 4 * W; }
float WaveScale(int W) { return (8.f + 2.f * W) / (float)WaveCount(W); }

std::vector<FPDEnemy> FPDGame::WaveSpec(int W)
{
	std::vector<FPDEnemy> List;
	// 무한 진행: 15웨이브까지 ×1.45, 그 뒤로는 ×1.3씩
	const float BossHp = 26.f * std::pow(1.45f, (float)(std::min(W, 15) - 1)) * std::pow(1.3f, (float)std::max(0, W - 15)) * EarlyHpK(W); // 초반 완화 (D-106)
	const float Hp = BossHp * WaveScale(W) * MobHpBonus; // 마리 수를 늘린 만큼 한 마리 체력을 낮춘다 (범위 공격이 세지는 몫은 MobHpBonus 로 되돌림)
	const int N = WaveCount(W);
	for (int I = 0; I < N; ++I)
	{
		EPDEnemy T = EPDEnemy::Normal;
		if (W >= 3 && I % 4 == 3) T = EPDEnemy::Fast;
		if (W >= 4 && I % 5 == 4) T = EPDEnemy::Armored;
		if (W >= 5 && I % 6 == 5) T = EPDEnemy::Tank;
		if (W >= 6 && I % 7 == 6) T = EPDEnemy::Splitter;
		if (W >= 8 && I % 6 == 2) T = EPDEnemy::Regen;
		List.push_back(MakeEnemy(T, Hp, W));
	}
	// 보스: 5웨이브마다 방패 거인 → 분열 군주 → 재생 왕 순서로 돌아온다
	if (W % 5 == 0)
	{
		static const EPDEnemy Bosses[3] = {EPDEnemy::ShieldBoss, EPDEnemy::SplitBoss, EPDEnemy::RegenBoss};
		List.push_back(MakeEnemy(Bosses[(W / 5 - 1) % 3], BossHp, W));
	}
	return List;
}

void FPDGame::StartWave()
{
	if (Phase != EPDPhase::Build) return;
	AutoStartT = -1.f;
	Wave++;
	Phase = EPDPhase::Wave;
	Pending = WaveSpec(Wave);
	SpawnT = 0.f;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::WaveStart;
	E.A = Wave;
	for (const FPDEnemy& P : Pending) if (EnemyDef(P.Type).bBoss) { E.B = 1; E.Enemy = P.Type; }
	Ev(E);
	Sfx(EPDSound::Start);
}

void FPDGame::EndWave()
{
	Projectiles.clear(); // 남은 발사체는 맞힐 적이 없다
	int Got = 0;
	Draw(DrawPerWave + (Has(EPDCharm::Supply) ? 1 : 0) + Mods.DrawAdd, &Got);
	AddGold(5.f + Wave); // 웨이브 통과 덤
	if (Wave % CharmEvery == 0) // 부적 창은 5웨이브마다 (D-101)
	{
		Phase = EPDPhase::Reward;
		Rewards = RollRewards();
	}
	else // 그 사이는 창 없이 5초 뒤 다음 웨이브
	{
		Rewards.clear();
		Phase = EPDPhase::Build;
		AutoStartT = AutoStartDelayFor(Wave + 1);
	}
	FPDEvent E;
	E.Kind = FPDEvent::EKind::WaveClear;
	E.A = Got;
	Ev(E);
	Sfx(EPDSound::Clear);
}

// 부적 창 (5웨이브마다, D-101): 부적 3개(가진 것 제외). 와일드가 나오면(WildChance) 셋째 칸이 와일드.
// 부적 칸이 가득이거나 남은 부적이 모자라면 보충·수리·골드로 채운다
std::vector<FPDReward> FPDGame::RollRewards()
{
	std::vector<FPDReward> Out;
	std::vector<int> Pool;
	for (int J = 0; J < (int)EPDCharm::Count; ++J) if (std::find(Charms.begin(), Charms.end(), (EPDCharm)J) == Charms.end()) Pool.push_back(J);
	std::shuffle(Pool.begin(), Pool.end(), Rng);
	const bool bWild = std::uniform_real_distribution<float>(0.f, 1.f)(Rng) < WildChance;
	const int NCharm = (int)Charms.size() < CharmMax ? std::min(bWild ? 2 : 3, (int)Pool.size()) : 0;
	for (int I = 0; I < NCharm; ++I) Out.push_back({true, Pool[I]});
	std::vector<int> Inst{(int)EPDInstant::Draw2, (int)EPDInstant::Repair, (int)EPDInstant::Gold};
	std::shuffle(Inst.begin(), Inst.end(), Rng);
	if (bWild) Inst.insert(Inst.begin(), (int)EPDInstant::Wild);
	for (int I = 0; Out.size() < 3 && I < (int)Inst.size(); ++I) Out.push_back({false, Inst[I]});
	return Out;
}

void FPDGame::TakeReward(int Index)
{
	if (Phase != EPDPhase::Reward) return;
	if (Index >= 0 && Index < (int)Rewards.size())
	{
		const FPDReward R = Rewards[Index];
		if (R.bCharm) { if ((int)Charms.size() < CharmMax) Charms.push_back((EPDCharm)R.Id); }
		else if (R.Id == (int)EPDInstant::Wild)
		{
			const FPDCard W{0, EPDSuit::Wild};
			if ((int)Hand.size() < HandMax()) Hand.push_back(W);
			else Deck.push_back(W);
		}
		else if (R.Id == (int)EPDInstant::Draw2) Draw(2);
		else if (R.Id == (int)EPDInstant::Repair) Lives += 3;
		else if (R.Id == (int)EPDInstant::Gold) { Gold += 10 + Wave * 4; GoldEarned += 10 + Wave * 4; }
		Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
		FPDEvent E;
		E.Kind = FPDEvent::EKind::Reward;
		E.A = R.bCharm;
		E.B = R.Id;
		Ev(E);
		Sfx(EPDSound::Burst, 4, 1);
	}
	Rewards.clear();
	Phase = EPDPhase::Build;
	AutoStartT = AutoStartDelayFor(Wave + 1); // 부적을 고르면 5초(초반은 12초) 뒤 다음 웨이브
}

std::vector<FPDSynergy> FPDGame::PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers) const
{
	std::vector<FPDSynergy> Out;
	if (HandIdx < 0 || HandIdx >= (int)Hand.size() || !Buildable(X, Y)) return Out;
	FPDTowerMap Copy = Towers;
	FPDTower T;
	T.Card = Hand[HandIdx];
	T.X = X;
	T.Y = Y;
	Copy[CellKey(X, Y)] = T;
	std::vector<FPDSynergy> All = ApplyEffects(Copy, CharmMask(), &Mods);
	std::set<int> Chains;
	for (const FPDSynergy& S : All) if (std::find(S.Cells.begin(), S.Cells.end(), CellKey(X, Y)) != S.Cells.end()) Chains.insert(S.ChainId);
	for (const FPDSynergy& S : All) if (Chains.count(S.ChainId)) Out.push_back(S);
	if (OutTowers) *OutTowers = std::move(Copy);
	return Out;
}

bool FPDGame::Place(int HandIdx, int X, int Y)
{
	if (Phase == EPDPhase::Title || Phase == EPDPhase::Over || Phase == EPDPhase::Win || Phase == EPDPhase::Reward) return false;
	if (HandIdx < 0 || HandIdx >= (int)Hand.size() || !Buildable(X, Y)) return false;
	const int Key = CellKey(X, Y);
	auto Sig = [](const FPDSynergy& S) { std::vector<int> V = S.Cells; std::sort(V.begin(), V.end()); V.push_back((int)S.Kind); return V; };
	std::set<std::vector<int>> Before;
	for (const FPDSynergy& S : Synergies) Before.insert(Sig(S));
	if (Towers.count(Key)) Discarded++;
	FPDTower T;
	T.Card = Hand[HandIdx];
	T.X = X;
	T.Y = Y;
	Hand.erase(Hand.begin() + HandIdx);
	Towers[Key] = T;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent P;
	P.Kind = FPDEvent::EKind::Place;
	P.X = X + 0.5f;
	P.Y = Y + 0.5f;
	P.Suit = T.Card.Suit;
	Ev(P);
	// 새로 생긴 족보 중 가장 큰 것
	const FPDSynergy* Best = nullptr;
	for (const FPDSynergy& S : Synergies)
		if (!Before.count(Sig(S)) && std::find(S.Cells.begin(), S.Cells.end(), Key) != S.Cells.end() && (!Best || S.Value > Best->Value)) Best = &S;
	if (Best)
	{
		FPDEvent E;
		E.Kind = FPDEvent::EKind::Synergy;
		E.Hand = Best->Kind;
		E.A = Best->N;
		E.B = Best->Chain;
		float CX = 0.f, CY = 0.f;
		for (int K : Best->Cells) { CX += K % Cols + 0.5f; CY += K / Cols + 0.5f; }
		E.X = CX / Best->Cells.size();
		E.Y = CY / Best->Cells.size();
		Ev(E);
		Sfx(EPDSound::Burst, std::min(8, std::max(1, Best->Value / 15 + Best->Chain - 1)), Best->Value >= 80 ? 2 : Best->Value >= 50 ? 1 : 0);
		if (Best->Kind == EPDHand::Flush || Best->Kind == EPDHand::StraightFlush) Sfx(EPDSound::Flush);
		if (Best->Kind == EPDHand::Straight && Best->N >= 3) Sfx(EPDSound::Straight, Best->N);
	}
	else
		Sfx(EPDSound::Lock);
	return true;
}

float BoardPower(FPDTowerMap Towers, uint32_t Charms, const FPDTreeMods* Tree)
{
	ApplyEffects(Towers, Charms, Tree);
	auto Base = [&](const FPDTower& T) { const FPDTowerStats S = TowerStats(T, Charms, Tree); return S.Dmg * S.Rate * S.CritFactor() * (1.f + S.Splash * 0.8f); };
	float Sum = 0.f;
	for (const auto& KV : Towers)
	{
		const FPDTower& T = KV.second;
		float V = Base(T);
		if (T.IsJoker())
		{
			switch ((EPDJoker)T.Joker)
			{
			case EPDJoker::Comedy: V *= 1.8f; break; // 두 발
			case EPDJoker::Star: V *= 4.f; break;    // 모든 적
			case EPDJoker::Bell: V *= 1.3f; break;   // 멈춤
			case EPDJoker::Mask: if (T.Eff.CopyKey >= 0 && Towers.count(T.Eff.CopyKey)) V = std::max(V, Base(Towers.at(T.Eff.CopyKey))); break; // 옆 타워 따라 함
			default: break;
			}
		}
		Sum += V;
	}
	return Sum;
}

FPDGame::FPowerPreview FPDGame::PreviewPower(EPowerAction Action, int Idx) const
{
	FPowerPreview P;
	P.Before = Power();
	FPDGame Copy = *this; // 사본에서 눌러 본다 (소리·사건은 버려짐)
	Copy.bMuted = true;
	bool bOk = false;
	switch (Action)
	{
	case EPowerAction::Fuse: bOk = Copy.Fuse(Idx); break;
	case EPowerAction::JokerFuse: bOk = Copy.JokerFuse(Idx); break;
	case EPowerAction::Feed: bOk = Copy.Feed(Idx); break;
	}
	if (bOk) P.After = Copy.Power();
	return P;
}

bool FPDGame::CanFuse(int SynergyIdx) const
{
	if (Phase == EPDPhase::Title || Phase == EPDPhase::Over || Phase == EPDPhase::Win || Phase == EPDPhase::Reward) return false;
	if (SynergyIdx < 0 || SynergyIdx >= (int)Synergies.size()) return false;
	for (int K : Synergies[SynergyIdx].Cells) if (Towers.at(K).IsJoker()) return false; // 조커가 든 족보는 합성하지 않는다
	return Towers.at(Synergies[SynergyIdx].Cells[0]).Level < (Mods.Key(EPDKeystone::ClownPath) ? 2 : MaxLevel); // 광대의 길: 합성은 레벨 2까지
}

std::vector<FPDJokerGroup> FPDGame::JokerGroups() const
{
	std::vector<FPDJokerGroup> Out;
	if (Phase == EPDPhase::Title || Phase == EPDPhase::Over || Phase == EPDPhase::Win || Phase == EPDPhase::Reward) return Out;
	if (Mods.Key(EPDKeystone::Smith)) return Out; // 대장장이: 조커 포탑을 만들 수 없다
	// 재료 후보: 조커가 없는 족보 + 족보에 안 든 합성 타워
	struct FNode
	{
		int Syn;
		int Fused;
		std::vector<int> Cells;
	};
	std::vector<FNode> Nodes;
	std::set<int> InSyn;
	for (int I = 0; I < (int)Synergies.size(); ++I)
	{
		bool bJoker = false;
		for (int K : Synergies[I].Cells) { InSyn.insert(K); bJoker = bJoker || Towers.at(K).IsJoker(); }
		if (!bJoker) Nodes.push_back({I, -1, Synergies[I].Cells});
	}
	for (const auto& KV : Towers)
		if (KV.second.Level >= 2 && !KV.second.IsJoker() && !InSyn.count(KV.first)) Nodes.push_back({-1, KV.first, {KV.first}});
	auto Touch = [](const FNode& A, const FNode& B) {
		for (int P : A.Cells) for (int Q : B.Cells) if (std::abs(P % Cols - Q % Cols) + std::abs(P / Cols - Q / Cols) <= 1) return true; // 맞닿거나 카드를 나눠 씀
		return false;
	};
	std::vector<int> GroupOf(Nodes.size(), -1);
	int Gid = 0;
	for (int I = 0; I < (int)Nodes.size(); ++I)
	{
		if (GroupOf[I] >= 0) continue;
		std::vector<int> Mem, St{I};
		GroupOf[I] = Gid;
		while (!St.empty())
		{
			const int K = St.back();
			St.pop_back();
			Mem.push_back(K);
			for (int J = 0; J < (int)Nodes.size(); ++J) if (GroupOf[J] < 0 && Touch(Nodes[K], Nodes[J])) { GroupOf[J] = Gid; St.push_back(J); }
		}
		Gid++;
		if (Mem.size() < 3) continue;
		FPDJokerGroup G;
		std::vector<EPDHand> Kinds;
		for (int K : Mem)
		{
			if (Nodes[K].Syn >= 0) { G.SynergyIdx.push_back(Nodes[K].Syn); Kinds.push_back(Synergies[Nodes[K].Syn].Kind); }
			else
			{
				G.FusedCells.push_back(Nodes[K].Fused);
				const int FH = Towers.at(Nodes[K].Fused).FuseHand;
				if (FH >= 0) Kinds.push_back((EPDHand)FH);
			}
			G.Cells.insert(G.Cells.end(), Nodes[K].Cells.begin(), Nodes[K].Cells.end());
		}
		std::sort(G.Cells.begin(), G.Cells.end());
		G.Cells.erase(std::unique(G.Cells.begin(), G.Cells.end()), G.Cells.end()); // 나눠 쓴 카드는 한 번만
		G.Result = MatchJokerRecipe(Kinds);
		Out.push_back(G);
	}
	return Out;
}

int DefaultFuseCell(const std::vector<int>& Cells)
{
	if (Cells.empty()) return -1;
	float CX = 0.f, CY = 0.f;
	for (int K : Cells) { CX += K % Cols; CY += K / Cols; }
	CX /= Cells.size();
	CY /= Cells.size();
	int Target = Cells[0];
	float BestD = 1e9f;
	for (int K : Cells)
	{
		const float D = std::hypot(K % Cols - CX, K / Cols - CY);
		if (D < BestD - 1e-4f) { BestD = D; Target = K; }
	}
	return Target;
}
// 고른 칸이 재료 칸 안이면 그 칸, -1 이면 기본 칸, 그 밖이면 -1 (실패)
static int PDFuseTarget(const std::vector<int>& Cells, int Wanted)
{
	if (Wanted < 0) return DefaultFuseCell(Cells);
	return std::find(Cells.begin(), Cells.end(), Wanted) != Cells.end() ? Wanted : -1;
}

bool FPDGame::JokerFuse(int GroupIdx, int TargetCell)
{
	const std::vector<FPDJokerGroup> Groups = JokerGroups();
	if (GroupIdx < 0 || GroupIdx >= (int)Groups.size()) return false;
	const FPDJokerGroup& G = Groups[GroupIdx];
	const int Target = PDFuseTarget(G.Cells, TargetCell);
	if (Target < 0) return false;
	FPDTower J;
	J.X = Target % Cols;
	J.Y = Target / Cols;
	J.Card = {0, EPDSuit::Wild};
	J.Joker = (int)G.Result;
	float Mult = 0.f;
	std::map<int, int> SuitCnt;
	for (int K : G.Cells)
	{
		const FPDTower& T = Towers.at(K);
		Mult += T.Mult;
		J.Level = std::max(J.Level, T.Level);
		J.TDmg = std::max(J.TDmg, T.TDmg);
		J.TRate = std::max(J.TRate, T.TRate);
		J.TRange = std::max(J.TRange, T.TRange);
		J.TSplash = std::max(J.TSplash, T.TSplash);
		if (!T.Card.IsWild()) SuitCnt[(int)T.Card.Suit]++;
	}
	int BestSC = -1;
	for (auto& KV : SuitCnt) if (KV.second > BestSC) { BestSC = KV.second; J.JSuit = (EPDSuit)KV.first; }
	J.Mult = Mult * JokerMult();
	for (int K : G.Cells) Towers.erase(K);
	Towers[Target] = J;
	const bool bFirst = !((Discovered >> J.Joker) & 1u);
	Discovered |= 1u << J.Joker;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::JokerFuse;
	E.X = J.X + 0.5f;
	E.Y = J.Y + 0.5f;
	E.A = J.Joker;
	E.B = bFirst;
	Ev(E);
	Sfx(EPDSound::Win);
	return true;
}

std::vector<FPDFeed> FPDGame::FeedTargets() const
{
	std::vector<FPDFeed> Out;
	if (Phase == EPDPhase::Title || Phase == EPDPhase::Over || Phase == EPDPhase::Win || Phase == EPDPhase::Reward) return Out;
	for (int I = 0; I < (int)Synergies.size(); ++I)
	{
		const FPDSynergy& S = Synergies[I];
		// 먹을 조커: 족보 안에 든 조커(와일드로 든 것) 먼저, 없으면 맞닿은 조커 중 레벨이 가장 높은 것
		int Best = -1, Inside = -1;
		float MultSum = 0.f;
		int Fed = 0;
		for (int K : S.Cells)
		{
			const FPDTower& T = Towers.at(K);
			if (T.IsJoker()) { if (Inside < 0 || T.JLevel > Towers.at(Inside).JLevel) Inside = K; continue; }
			MultSum += T.Mult;
			Fed++;
			const int X = K % Cols, Y = K / Cols;
			for (const auto& D : GPDN4)
			{
				const int NX = X + D[0], NY = Y + D[1];
				if (NX < 0 || NY < 0 || NX >= Cols || NY >= Rows) continue;
				auto It = Towers.find(CellKey(NX, NY));
				if (It != Towers.end() && It->second.IsJoker() && (Best < 0 || It->second.JLevel > Towers.at(Best).JLevel)) Best = It->first;
			}
		}
		if (Inside >= 0) Best = Inside;
		if (Best < 0 || Fed == 0 || Towers.at(Best).JLevel >= JokerMaxLevel) continue;
		// 경험치 = 족보 점수 × 먹인 타워 평균 위력 (합성 타워일수록 많이)
		Out.push_back({I, Best, S.Value * MultSum / Fed});
	}
	return Out;
}

bool FPDGame::Feed(int SynergyIdx)
{
	const std::vector<FPDFeed> List = FeedTargets();
	const FPDFeed* F = nullptr;
	for (const FPDFeed& X : List) if (X.Syn == SynergyIdx) F = &X;
	if (!F) return false;
	const int JKey = F->Joker;
	const float Xp = F->Xp;
	for (int K : Synergies[SynergyIdx].Cells) if (!Towers.at(K).IsJoker()) Towers.erase(K);
	FPDTower& J = Towers.at(JKey);
	J.JXp += Xp;
	const int Old = J.JLevel;
	while (J.JLevel < JokerMaxLevel && J.JXp >= JokerXpNeed(J.JLevel)) { J.JXp -= JokerXpNeed(J.JLevel); J.JLevel++; }
	if (J.JLevel >= JokerMaxLevel) J.JXp = 0.f;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Feed;
	E.X = J.X + 0.5f;
	E.Y = J.Y + 0.5f;
	E.A = J.JLevel;
	E.B = J.JLevel - Old;
	E.R = Xp;
	Ev(E);
	Sfx(EPDSound::Burst, std::min(8, 2 + E.B), E.B > 0 ? 1 : 0);
	if (E.B > 0) Sfx(EPDSound::Clear);
	return true;
}

bool FPDGame::Fuse(int SynergyIdx, int TargetCell)
{
	if (!CanFuse(SynergyIdx)) return false;
	const FPDSynergy S = Synergies[SynergyIdx];
	// 합쳐질 칸: 고른 칸 (재료 칸 중 하나), 안 골랐으면 족보 가운데에 가장 가까운 칸
	const int Target = PDFuseTarget(S.Cells, TargetCell);
	if (Target < 0) return false;
	// 대표 카드: 가장 많은 숫자(같으면 높은 쪽) + 가장 많은 무늬. 와일드는 표를 던지지 않는다
	std::map<int, int> RankCnt;
	std::map<int, int> SuitCnt;
	FPDTrait Old;
	float Mult = 0.f;
	int Level = 1;
	for (int K : S.Cells)
	{
		const FPDTower& T = Towers.at(K);
		if (!T.Card.IsWild()) { RankCnt[T.Card.Rank]++; SuitCnt[(int)T.Card.Suit]++; }
		Mult += T.Mult;
		Level = std::max(Level, T.Level);
		Old.Dmg = std::max(Old.Dmg, T.TDmg);
		Old.Rate = std::max(Old.Rate, T.TRate);
		Old.Range = std::max(Old.Range, T.TRange);
		Old.Splash = std::max(Old.Splash, T.TSplash);
	}
	FPDTower F;
	F.X = Target % Cols;
	F.Y = Target / Cols;
	if (RankCnt.empty()) F.Card = {0, EPDSuit::Wild};
	else
	{
		int BestR = 0, BestC = -1, BestS = 0, BestSC = -1;
		for (auto& KV : RankCnt) if (KV.second >= BestC) { BestC = KV.second; BestR = KV.first; }
		for (auto& KV : SuitCnt) if (KV.second > BestSC) { BestSC = KV.second; BestS = KV.first; }
		F.Card = {BestR, (EPDSuit)BestS};
	}
	const FPDTrait H = HandTrait(S.Kind, S.N);
	F.Level = std::min(MaxLevel, Level + 1);
	F.FuseHand = (int)S.Kind;
	for (int K : S.Cells) { const int FH = Towers.at(K).FuseHand; if (FH >= 0 && HandBaseValue((EPDHand)FH) > HandBaseValue((EPDHand)F.FuseHand)) F.FuseHand = FH; }
	F.Mult = Mult * FuseMult();
	F.TDmg = Old.Dmg * H.Dmg;
	F.TRate = Old.Rate * H.Rate;
	F.TRange = Old.Range * H.Range;
	F.TSplash = std::max(Old.Splash, H.Splash);
	for (int K : S.Cells) Towers.erase(K);
	Towers[Target] = F;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Fuse;
	E.X = F.X + 0.5f;
	E.Y = F.Y + 0.5f;
	E.A = F.Level;
	E.B = (int)S.Cells.size();
	E.Hand = S.Kind;
	E.Suit = F.Card.Suit;
	Ev(E);
	Sfx(EPDSound::Burst, std::min(8, 3 + F.Level), 2);
	Sfx(EPDSound::Clear);
	return true;
}

void FPDGame::Discard(int HandIdx)
{
	if (HandIdx < 0 || HandIdx >= (int)Hand.size()) return;
	Hand.erase(Hand.begin() + HandIdx);
	Discarded++;
	if (Has(EPDCharm::Recycle)) Draw(1);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Discard;
	Ev(E);
	Sfx(EPDSound::HardDrop);
}

void FPDGame::Hit(FPDEnemy& E, float Dmg, float Slow)
{
	if (E.bDead) return;
	if (E.Armor > 0.f && !Has(EPDCharm::Pierce)) Dmg = std::max(Dmg * 0.25f, Dmg - E.Armor); // 방패: 한 방 피해를 깎는다 (최소 25%)
	E.Hp -= Dmg;
	if (Slow > 0.f) { E.SlowT = 1.2f; E.SlowAmt = std::max(E.SlowAmt, Slow); }
	if (E.Hp > 0.f) return;
	E.bDead = true;
	Kills++;
	const FPDPoint P = Path.At(E.D);
	FPDEvent K;
	K.Kind = FPDEvent::EKind::Kill;
	K.X = P.X;
	K.Y = P.Y;
	K.Enemy = E.Type;
	K.A = AddGold(KillGold(E.Type, E.Wave) * (EnemyDef(E.Type).bBoss ? 1.f : WaveScale(E.Wave))); // 마리 수가 늘어도 웨이브당 골드는 그대로
	Ev(K);
	const FPDEnemyDef& D = EnemyDef(E.Type);
	if (KillSfxT <= 0.f || D.bBoss) { Sfx(D.bBoss ? EPDSound::Clear : EPDSound::Rotate); KillSfxT = 0.08f; }
	// 분열: 작은 적(보스는 탱크)으로 갈라진다
	for (int I = 0; I < D.Split; ++I)
	{
		FPDEnemy Child = MakeEnemy(D.bBoss ? EPDEnemy::Tank : EPDEnemy::Mini, E.BaseHp * (D.bBoss ? 0.6f : 1.f), E.Wave);
		Child.D = std::max(0.f, E.D - I * 0.2f);
		Born.push_back(Child);
	}
	if (Has(EPDCharm::ChainBlast))
		for (FPDEnemy& O : Enemies)
		{
			if (&O == &E || O.bDead) continue;
			const FPDPoint Q = Path.At(O.D);
			if (std::hypot(Q.X - P.X, Q.Y - P.Y) <= 1.2f) { O.Hp -= E.MaxHp * 0.15f; if (O.Hp <= 0.f) Hit(O, 0.f, 0.f); }
		}
}

// 쏘기: 레이저는 바로 맞히고, 나머지는 발사체를 띄운다
void FPDGame::Fire(const FPDTower& T, FPDEnemy& Target, float Dmg, float Splash, float Slow, int Crits, bool bMain)
{
	const float TX = T.X + 0.5f, TY = T.Y + 0.5f;
	const FPDPoint P = Path.At(Target.D);
	FPDProjectile Pr;
	Pr.Kind = ShotKind(T);
	Pr.X = TX;
	Pr.Y = TY;
	Pr.TX = P.X;
	Pr.TY = P.Y;
	Pr.Target = Target.Id;
	Pr.Dmg = Dmg;
	Pr.Splash = Splash;
	Pr.Slow = Slow;
	Pr.Crits = Crits;
	Pr.Level = T.Level;
	Pr.bHand = T.Eff.Hand >= 0 || T.IsJoker();
	Pr.Suit = T.IsJoker() ? EPDSuit::Wild : T.Card.Suit;
	FPDEvent Sh;
	Sh.Kind = FPDEvent::EKind::Shot;
	Sh.X = TX;
	Sh.Y = TY;
	Sh.X2 = P.X;
	Sh.Y2 = P.Y;
	Sh.Suit = Pr.Suit;
	Sh.A = Pr.bHand;
	Sh.B = Crits;
	Sh.Lv = T.Level;
	Sh.Dmg = ShotSpeed(Pr.Kind) <= 0.f ? Dmg : 0.f;
	Sh.R = (float)(int)Pr.Kind;
	Ev(Sh);
	const float Spd = ShotSpeed(Pr.Kind);
	if (Spd <= 0.f) { Explode(Pr, &Target); return; }
	const float DX = P.X - TX, DY = P.Y - TY, Len = std::max(0.01f, std::hypot(DX, DY));
	if (Pr.Kind == EPDShot::Missile) // 미사일: 옆으로 튀어 나갔다가 휘어 들어간다
	{
		const float Side = std::uniform_real_distribution<float>(0.f, 1.f)(Rng) < 0.5f ? -1.f : 1.f;
		Pr.VX = (DX / Len * 0.3f - DY / Len * Side) * Spd * 0.7f;
		Pr.VY = (DY / Len * 0.3f + DX / Len * Side) * Spd * 0.7f;
	}
	else
	{
		Pr.VX = DX / Len * Spd;
		Pr.VY = DY / Len * Spd;
	}
	Target.Incoming += Dmg;
	Projectiles.push_back(Pr);
	(void)bMain;
}

// 닿음: 목표(살아 있으면)에 피해, 범위 공격은 닿은 자리 둘레에
void FPDGame::Explode(const FPDProjectile& P, FPDEnemy* Target)
{
	FPDPoint At{P.TX, P.TY};
	const bool bHitTarget = Target && !Target->bDead;
	if (bHitTarget) { At = Path.At(Target->D); Hit(*Target, P.Dmg, P.Slow); }
	if (P.Splash > 0.f)
	{
		for (FPDEnemy& O : Enemies)
		{
			if (&O == Target || O.bDead) continue;
			const FPDPoint Q = Path.At(O.D);
			if (std::hypot(Q.X - At.X, Q.Y - At.Y) <= P.Splash) Hit(O, P.Dmg * 0.6f, P.Slow);
		}
		FPDEvent Sp;
		Sp.Kind = FPDEvent::EKind::Splash;
		Sp.X = At.X;
		Sp.Y = At.Y;
		Sp.R = P.Splash;
		Sp.Suit = P.Suit;
		Ev(Sp);
	}
	if (ShotSpeed(P.Kind) > 0.f)
	{
		FPDEvent Im;
		Im.Kind = FPDEvent::EKind::Impact;
		Im.X = At.X;
		Im.Y = At.Y;
		Im.R = (float)(int)P.Kind;
		Im.B = P.Crits;
		Im.A = P.bHand;
		Im.Lv = P.Level;
		Im.Dmg = bHitTarget ? P.Dmg : 0.f;
		Im.X2 = P.VX; // 날아온 방향 (화면의 튀는 효과 방향)
		Im.Y2 = P.VY;
		Im.Suit = P.Suit;
		Ev(Im);
	}
}

void FPDGame::UpdateProjectiles(float Dt)
{
	for (FPDProjectile& P : Projectiles)
	{
		FPDEnemy* T = nullptr;
		for (FPDEnemy& E : Enemies) if (E.Id == P.Target && !E.bDead) { T = &E; break; }
		if (T) { const FPDPoint Q = Path.At(T->D); P.TX = Q.X; P.TY = Q.Y; }
		const float DX = P.TX - P.X, DY = P.TY - P.Y, Dist = std::hypot(DX, DY);
		const float Spd = ShotSpeed(P.Kind);
		P.Life += Dt;
		if (Dist <= Spd * Dt + 0.12f || P.Life > 3.f)
		{
			if (T) T->Incoming = std::max(0.f, T->Incoming - P.Dmg);
			P.X = P.TX;
			P.Y = P.TY;
			Explode(P, T);
			P.Life = -1.f; // 지움 표시
			continue;
		}
		if (P.Kind == EPDShot::Missile) // 유도: 속도를 목표 쪽으로 돌린다 (갈수록 세게)
		{
			const float K = std::min(1.f, (3.f + P.Life * 12.f) * Dt);
			P.VX += (DX / Dist * Spd - P.VX) * K;
			P.VY += (DY / Dist * Spd - P.VY) * K;
		}
		else
		{
			P.VX = DX / Dist * Spd;
			P.VY = DY / Dist * Spd;
		}
		P.X += P.VX * Dt;
		P.Y += P.VY * Dt;
	}
	Projectiles.erase(std::remove_if(Projectiles.begin(), Projectiles.end(), [](const FPDProjectile& P) { return P.Life < 0.f; }), Projectiles.end());
}

void FPDGame::Update(float Dt)
{
	Time += Dt;
	for (auto& KV : Towers) KV.second.Age += Dt;
	KillSfxT -= Dt;
	if (Phase != EPDPhase::Wave) return;
	SpawnT -= Dt;
	if (!Pending.empty() && SpawnT <= 0.f)
	{
		const FPDEnemy E = Pending.front();
		Pending.erase(Pending.begin());
		Enemies.push_back(E);
		SpawnT = EnemyDef(E.Type).bBoss ? 1.2f : 0.65f * WaveScale(Wave); // 마리 수가 늘어도 웨이브 길이는 그대로
	}
	for (FPDEnemy& E : Enemies)
	{
		E.SlowT = std::max(0.f, E.SlowT - Dt);
		const float Rg = EnemyDef(E.Type).Regen;
		if (Rg > 0.f && !E.bDead) E.Hp = std::min(E.MaxHp, E.Hp + E.MaxHp * Rg * Dt);
		E.D += E.Speed * (E.SlowT > 0.f ? 1.f - E.SlowAmt : 1.f) * Dt;
		if (E.D >= Path.Length && !E.bDead)
		{
			E.bDead = true;
			const int Loss = EnemyDef(E.Type).bBoss ? 5 : 1;
			Lives -= Loss;
			Leaks++;
			FPDEvent L;
			L.Kind = FPDEvent::EKind::Leak;
			L.A = Loss;
			Ev(L);
			Sfx(EPDSound::Denied);
		}
	}
	const uint32_t Mask = CharmMask();
	const FPDTreeMods* Tm = &Mods;
	for (auto& KV : Towers)
	{
		FPDTower& T = KV.second;
		T.Cd -= Dt;
		if (T.Cd > 0.f) continue;
		FPDTowerStats St = TowerStats(T, Mask, Tm);
		const bool bUp = T.IsJoker() && Mods.Up(T);
		const float TX = T.X + 0.5f, TY = T.Y + 0.5f;
		const EPDJoker JK = T.IsJoker() ? (EPDJoker)T.Joker : EPDJoker::Count;
		if (JK == EPDJoker::Mask && T.Eff.CopyKey >= 0 && Towers.count(T.Eff.CopyKey)) // 웃는 가면: 옆 타워가 더 세면 그대로 따라 함
		{
			const FPDTowerStats O = TowerStats(Towers.at(T.Eff.CopyKey), Mask, Tm);
			St.Dmg = std::max(St.Dmg, O.Dmg * (bUp ? 1.3f : 1.f)); // 강화: 따라 한 공격 ×1.3
			St.Rate = std::max(St.Rate, O.Rate);
			St.Range = std::max(St.Range, O.Range);
			St.Splash = std::max(St.Splash, O.Splash);
			St.Slow = std::max(St.Slow, O.Slow);
		}
		St.Dmg *= std::max(1.f, St.Rate * Dt); // 한 틱(0.05초)보다 빨리 쏘면 남는 만큼 피해로 몰아준다
		auto RollCrit = [&]() { // 치명타 횟수: 100%를 넘은 만큼 확정 + 나머지는 확률
			int N = (int)St.Crit;
			if (std::uniform_real_distribution<float>(0.f, 1.f)(Rng) < St.Crit - N) N++;
			return N;
		};
		if (JK == EPDJoker::Bell) // 방울: 8초마다 사거리 안 적을 1.5초 멈춘다
		{
			T.Timer += Dt;
			if (T.Timer >= (bUp ? 5.f : 8.f)) // 강화: 5초마다 2초
			{
				T.Timer = 0.f;
				for (FPDEnemy& E : Enemies)
				{
					const FPDPoint P = Path.At(E.D);
					if (!E.bDead && std::hypot(P.X - TX, P.Y - TY) <= St.Range) { E.SlowT = bUp ? 2.f : 1.5f; E.SlowAmt = 1.f; }
				}
				FPDEvent B;
				B.Kind = FPDEvent::EKind::Bell;
				B.X = TX;
				B.Y = TY;
				B.R = St.Range;
				Ev(B);
			}
		}
		if (JK == EPDJoker::Star) // 스타: 판 전체 레이저, 모든 적에게
		{
			bool bAny = false;
			const float SD = St.Dmg * (1.f + RollCrit() * (St.CritMult - 1.f));
			for (FPDEnemy& E : Enemies) if (!E.bDead) { Hit(E, SD, 0.f); bAny = true; }
			if (bAny)
			{
				T.Cd = 1.f / St.Rate;
				FPDEvent B;
				B.Kind = FPDEvent::EKind::Star;
				B.X = TX;
				B.Y = TY;
				Ev(B);
			}
			continue;
		}
		// 가장 앞선 적. 날아가는 발사체로 이미 죽을 적은 뒤로 미룬다 (겹쳐 쏘지 않게)
		FPDEnemy* Target = nullptr;
		bool bTargetDoomed = true;
		for (FPDEnemy& E : Enemies)
		{
			if (E.bDead) continue;
			const FPDPoint P = Path.At(E.D);
			if (std::hypot(P.X - TX, P.Y - TY) > St.Range) continue;
			const bool bDoomed = E.Incoming >= E.Hp;
			if (!Target || (bTargetDoomed && !bDoomed) || (bDoomed == bTargetDoomed && E.D > Target->D)) { Target = &E; bTargetDoomed = bDoomed; }
		}
		if (!Target) continue;
		T.Cd = 1.f / St.Rate;
		const int Crits = RollCrit();
		St.Dmg *= 1.f + Crits * (St.CritMult - 1.f);
		if (JK == EPDJoker::Clown) T.Cycle++;
		FPDEnemy* Shot2[2] = {nullptr, nullptr};
		for (int Extra = 0; JK == EPDJoker::Comedy && Extra < (bUp ? 2 : 1); ++Extra) // 희비극: 둘째 적도 함께 공격 (강화: 셋째까지)
		{
			FPDEnemy* Second = nullptr;
			for (FPDEnemy& E : Enemies)
			{
				if (E.bDead || &E == Target || &E == Shot2[0]) continue;
				const FPDPoint P = Path.At(E.D);
				if (std::hypot(P.X - TX, P.Y - TY) <= St.Range && (!Second || E.D > Second->D)) Second = &E;
			}
			if (Second)
			{
				Shot2[Extra] = Second;
				Fire(T, *Second, St.Dmg, 0.f, St.Slow, Crits, false);
			}
		}
		Fire(T, *Target, St.Dmg, St.Splash, St.Slow, Crits, true);
	}
	UpdateProjectiles(Dt);
	Enemies.erase(std::remove_if(Enemies.begin(), Enemies.end(), [](const FPDEnemy& E) { return E.bDead; }), Enemies.end());
	Enemies.insert(Enemies.end(), Born.begin(), Born.end());
	Born.clear();
	if (Lives <= 0)
	{
		Lives = 0;
		Phase = EPDPhase::Over;
		BestWave = std::max(BestWave, Wave);
		FPDEvent G;
		G.Kind = FPDEvent::EKind::GameOver;
		Ev(G);
		Sfx(EPDSound::GameOver);
		return;
	}
	if (Pending.empty() && Enemies.empty()) EndWave();
}

void FPDGame::Tick(float Dt)
{
	Dt = std::min(Dt, 0.05f);
	// 자동 시작: 실제 시간으로 센다 (속도 ×2·×3 과 상관없이 5초)
	if (Phase == EPDPhase::Build && AutoStartT >= 0.f)
	{
		AutoStartT -= Dt;
		if (AutoStartT <= 0.f) StartWave();
	}
	for (int I = 0; I < Speed; ++I) Update(Dt);
}
} // namespace PD
