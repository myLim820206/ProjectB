#include "PDCore.h"

#include <unordered_map>
#include <sstream>

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
	{10.f, 0.6f, 0.f, 0, false, true},     // 보스: 돌진 기사
	{11.f, 0.5f, 0.f, 0, false, true},     // 보스: 소환사
	{10.f, 0.6f, 0.f, 0, false, true},     // 보스: 유령
	{12.f, 0.55f, 0.f, 0, false, true},    // 보스: 군단장
	{12.f, 0.5f, 0.f, 0, false, true},     // 보스: 분노 거인
	{10.f, 0.55f, 0.f, 0, false, true},    // 보스: 공간 도약자
	{13.f, 0.5f, 0.f, 0, false, true},     // 보스: 흡수자
	{12.f, 0.55f, 0.f, 0, false, true},    // 보스: 수호자
	{11.f, 0.55f, 0.f, 0, false, true},    // 보스: 저주술사
	{13.f, 0.5f, 0.f, 0, false, true},     // 보스: 파괴자
	{11.f, 0.55f, 0.f, 0, false, true},    // 보스: 블러퍼 (분신 둘)
	{10.f, 0.6f, 0.f, 0, false, true},     // 보스: 빅 블라인드 (판돈)
	{11.f, 0.55f, 0.f, 0, false, true},    // 보스: 딜러 (무늬 바꾸기)
	{12.f, 0.5f, 0.f, 0, false, true},     // 보스: 카드 카운터 (무늬 내성)
	{13.f, 0.5f, 0.f, 0, false, true},     // 보스: 하우스 룰 (연쇄 끔)
	{12.f, 0.55f, 0.f, 0, false, true},    // 보스: 쌍둥이 (둘이 한 묶음)
	{36.f, 0.45f, 0.f, 0, true, true},     // 최종 보스: 바운서 (30)
	{40.f, 0.5f, 0.f, 0, false, true},     // 최종 보스: 하이롤러 (60)
	{45.f, 0.45f, 0.f, 0, false, true},    // 최종 보스: 더 하우스 (100)
	{0.01f, 0.55f, 0.f, 0, false, false},  // 블러퍼 분신 (체력 1 로 고쳐 만든다)
};
const EPDEnemy BossOrder[13] = {EPDEnemy::ShieldBoss, EPDEnemy::SplitBoss, EPDEnemy::RegenBoss, EPDEnemy::Charger, EPDEnemy::Summoner,
	EPDEnemy::Phantom, EPDEnemy::Warlord, EPDEnemy::Berserker, EPDEnemy::Blinker, EPDEnemy::Devourer, EPDEnemy::Warden, EPDEnemy::Hexer, EPDEnemy::Breaker};
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
			case EPDStat::UpperPrep: M.UpperPrep += F.Value; break;
			case EPDStat::XpPct: M.XpPct += F.Value; break;
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

bool FPDTreeMods::Up(const FPDTower& T) const { return T.IsJoker() && (IsUpperJoker(T.Joker) || Up(T.Joker) || T.JLevel >= JokerUpLevel); } // 상위 조커는 늘 강화 상태

// 상위 조커 30종 (D-231, docs/18 4절 순서)
static const FPDUpperDef GPDUpper[UpperJokerCount] = {
	{EPDJoker::Clown, EPDJoker::Hatter, EPDUpperSpecial::None},        // 장미 칠하는 정원사
	{EPDJoker::Clown, EPDJoker::Comedy, EPDUpperSpecial::Tweedle},     // 트위들덤과 트위들디: 둘째 적은 다음 무늬로
	{EPDJoker::Clown, EPDJoker::Baton, EPDUpperSpecial::None},         // 도도새
	{EPDJoker::Clown, EPDJoker::Cheshire, EPDUpperSpecial::None},      // 애벌레
	{EPDJoker::Clown, EPDJoker::Bell, EPDUpperSpecial::None},          // 공작부인
	{EPDJoker::Clown, EPDJoker::Star, EPDUpperSpecial::Mirror},        // 거울 나라: 판 전체 피해 무늬가 번갈아 (♥ ×2, ♣ 판 전체 둔화)
	{EPDJoker::Hatter, EPDJoker::Mask, EPDUpperSpecial::None},         // 3월 토끼
	{EPDJoker::Hatter, EPDJoker::Harlequin, EPDUpperSpecial::None},    // 하트 왕
	{EPDJoker::Hatter, EPDJoker::Jack, EPDUpperSpecial::None},         // 하트 잭
	{EPDJoker::Hatter, EPDJoker::Cheshire, EPDUpperSpecial::MadTea},   // 미친 다과회: 주변 8칸 사거리 무한
	{EPDJoker::Hatter, EPDJoker::Bell, EPDUpperSpecial::WhiteRabbit},  // 흰 토끼: 10초마다 판 전체 1초 멈춤
	{EPDJoker::Mask, EPDJoker::Comedy, EPDUpperSpecial::None},         // 험프티 덤프티
	{EPDJoker::Mask, EPDJoker::Harlequin, EPDUpperSpecial::RedQueen},  // 붉은 여왕: 판 위 같은 무늬 중 가장 강한 타워를 따라 함
	{EPDJoker::Mask, EPDJoker::Baton, EPDUpperSpecial::MockTurtle},    // 가짜 거북: 주변에서 가장 빠른 타워를 따라 함
	{EPDJoker::Mask, EPDJoker::Jack, EPDUpperSpecial::None},           // 그리핀
	{EPDJoker::Mask, EPDJoker::Cheshire, EPDUpperSpecial::GrinOnly},   // 웃음만 남은 고양이: 판 전체에서 가장 강한 타워를 따라 함
	{EPDJoker::Comedy, EPDJoker::Harlequin, EPDUpperSpecial::None},    // 하얀 여왕
	{EPDJoker::Comedy, EPDJoker::Baton, EPDUpperSpecial::Walrus},      // 바다코끼리와 목수: 굴 잔치 — 처치할 때마다 주변 연사 +10% (최대 +50%, 웨이브마다 초기화, D-482)
	{EPDJoker::Comedy, EPDJoker::Jack, EPDUpperSpecial::None},         // 하얀 기사
	{EPDJoker::Comedy, EPDJoker::Star, EPDUpperSpecial::None},         // 말하는 꽃들
	{EPDJoker::Harlequin, EPDJoker::Jack, EPDUpperSpecial::QueenHearts}, // 하트 여왕: 체력 15% 이하 즉시 처치 (보스 제외)
	{EPDJoker::Harlequin, EPDJoker::Bell, EPDUpperSpecial::None},      // 홍학 크로케
	{EPDJoker::Harlequin, EPDJoker::Star, EPDUpperSpecial::None},      // 트럼프 근위대
	{EPDJoker::Baton, EPDJoker::Cheshire, EPDUpperSpecial::None},      // 체스판
	{EPDJoker::Baton, EPDJoker::Bell, EPDUpperSpecial::Dormouse},      // 겨울잠쥐: 주변 연사 ×1.8
	{EPDJoker::Baton, EPDJoker::Star, EPDUpperSpecial::None},          // 유니콘
	{EPDJoker::Jack, EPDJoker::Cheshire, EPDUpperSpecial::Jabberwock}, // 재버워키: 보스를 먼저 노리고 보스에게 +50%
	{EPDJoker::Jack, EPDJoker::Bell, EPDUpperSpecial::None},           // 사자
	{EPDJoker::Cheshire, EPDJoker::Star, EPDUpperSpecial::None},       // 꿈꾸는 붉은 왕
	{EPDJoker::Bell, EPDJoker::Star, EPDUpperSpecial::None},           // 반짝반짝 작은 박쥐
};
const FPDUpperDef& UpperDef(int U) { return GPDUpper[std::clamp(U, 0, UpperJokerCount - 1)]; }
bool GoalMet(EPDGoal G, const FPDGoalStats& S)
{
	switch (G)
	{
	case EPDGoal::Clear30: return (S.ModesCleared >> (int)EPDMode::W30) & 1u;
	case EPDGoal::Clear60: return (S.ModesCleared >> (int)EPDMode::W60) & 1u;
	case EPDGoal::Clear100: return (S.ModesCleared >> (int)EPDMode::W100) & 1u;
	case EPDGoal::FirstJoker: return S.Discovered != 0;
	case EPDGoal::FirstUpper: return S.UpperFound != 0;
	case EPDGoal::AllJokers: return (S.Discovered & 0x3FFu) == 0x3FFu;
	case EPDGoal::Level30: return S.BestLevel >= 30;
	case EPDGoal::Kills1000: return S.BestKills >= 1000;
	case EPDGoal::Bosses9: { int N = 0; for (uint32_t V = S.BossKilled; V; V >>= 1) N += V & 1u; return N >= 9; } // 보스 9종 처치 (D-340)
	case EPDGoal::Runs10: return S.RunsPlayed >= 10;
	default: return false;
	}
}

bool AchMet(EPDAch A, const FPDAchStats& S)
{
	if ((int)A < (int)EPDGoal::Count) return GoalMet((EPDGoal)A, S);
	auto Bits = [](uint32_t V) { int N = 0; while (V) { N += V & 1u; V >>= 1; } return N; };
	switch (A)
	{
	case EPDAch::Endless150: return S.EndlessBest >= 150;
	case EPDAch::Endless200: return S.EndlessBest >= 200;
	case EPDAch::RoyalFlush: return (S.HandsEver >> (int)EPDHand::RoyalFlush) & 1u;
	case EPDAch::FiveCard: return (S.HandsEver >> (int)EPDHand::Five) & 1u;
	case EPDAch::AllHands: return (S.HandsEver & ((1u << (int)EPDHand::Count) - 1u)) == (1u << (int)EPDHand::Count) - 1u;
	case EPDAch::FuseMax: return S.MaxFuseLevel >= MaxLevel;
	case EPDAch::Uppers10: return Bits(S.UpperFound) >= 10;
	case EPDAch::AllUppers: return Bits(S.UpperFound) >= UpperJokerCount;
	case EPDAch::Charms30: return S.CharmsOpen >= 30;
	case EPDAch::AllCharms: return S.CharmsOpen >= (int)EPDCharm::Count;
	case EPDAch::UpgradesAll: return S.bUpgradesMax;
	case EPDAch::SecretChannel: return S.bSecretChannel;
	case EPDAch::Runs50: return S.RunsPlayed >= 50;
	case EPDAch::TotalKills100k: return S.TotalKills >= 100000;
	case EPDAch::AllBosses: { const uint32_t M = (1u << (BossKinds - 3)) - 1u; return (S.BossKilled & M) == M; } // 일반 보스 19종
	case EPDAch::AllFinals: { const uint32_t M = 7u << (BossKinds - 3); return (S.BossKilled & M) == M; } // 최종 보스 셋
	case EPDAch::Mut3Boss: return S.bMut3Kill;
	default: return false;
	}
}

const char* AchApiName(EPDAch A)
{
	static const char* N[(int)EPDAch::Count] = {"ACH_CLEAR_30", "ACH_CLEAR_60", "ACH_CLEAR_100", "ACH_FIRST_JOKER", "ACH_FIRST_UPPER", "ACH_ALL_JOKERS", "ACH_LEVEL_30", "ACH_KILLS_1000",
		"ACH_BOSSES_9", "ACH_RUNS_10", "ACH_ENDLESS_150", "ACH_ENDLESS_200", "ACH_ROYAL_FLUSH", "ACH_FIVE_CARD", "ACH_ALL_HANDS", "ACH_FUSE_MAX", "ACH_UPPERS_10", "ACH_ALL_UPPERS",
		"ACH_CHARMS_30", "ACH_ALL_CHARMS", "ACH_UPGRADE_MAX", "ACH_SECRET_CHANNEL", "ACH_RUNS_50", "ACH_TOTAL_KILLS_100K", "ACH_ALL_BOSSES", "ACH_ALL_FINAL_BOSSES", "ACH_MUTANT_BOSS"};
	return N[std::clamp((int)A, 0, (int)EPDAch::Count - 1)];
}

int UpperPrice(int U) { const FPDUpperDef& D = UpperDef(U); const int Avg = (JokerPrice((int)D.A) + JokerPrice((int)D.B)) / 2; return (300 + Avg + 50) / 100 * 50; } // 150~400 (D-320)
int UpperOf(EPDJoker A, EPDJoker B)
{
	for (int U = 0; U < UpperJokerCount; ++U)
		if ((GPDUpper[U].A == A && GPDUpper[U].B == B) || (GPDUpper[U].A == B && GPDUpper[U].B == A)) return U;
	return -1;
}
bool JokerHas(int Joker, EPDJoker K)
{
	if (Joker == (int)K) return true;
	if (!IsUpperJoker(Joker)) return false;
	const FPDUpperDef& D = UpperDef(Joker - UpperBase);
	return D.A == K || D.B == K;
}
EPDUpperSpecial JokerSpecial(int Joker) { return IsUpperJoker(Joker) ? UpperDef(Joker - UpperBase).Special : EPDUpperSpecial::None; }

int UpgradeCost(int Level) { return Level < UpgradeMax ? 5 + 5 * Level : 105 + 2 * (Level - UpgradeMax); } // 칩 5, 10, 15 … 20레벨까지 한 종류 1,050 (D-230). 그 뒤는 천천히 (D-238)
float UpgradeStep(EPDUpgrade U)
{
	switch (U)
	{
	case EPDUpgrade::Attack: return 0.03f;    // 공격력 +3% (D-230 영구 강화, 20레벨 +60%)
	case EPDUpgrade::Speed: return 0.02f;     // 공격 속도 +2%
	case EPDUpgrade::Crit: return 0.01f;      // 치명타 확률 +1%p (100% 넘으면 한 방에 여러 번)
	case EPDUpgrade::CritMult: return 0.05f;  // 치명타 배율 +0.05
	default: return 0.04f;                    // 무늬 강화: 그 무늬 공격력 +4%
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

static std::vector<EPDHand> PDClassifyHandRaw(const std::vector<FPDCard>& Cards);
// 족보 판정은 카드 순서와 무관하다 (스트레이트 순서는 PDOrderedStraight 가 따로 본다). 정렬한 카드 묶음을 키로 결과를 기억한다 (D-480):
// 와일드(조커)가 W 장이면 숫자·무늬를 다 넣어 보는 재귀가 (13 × 무늬)^W 번이라, 조커 4장 덩어리에서 족보 찾기가 127ms 걸리던 것
std::vector<EPDHand> ClassifyHand(const std::vector<FPDCard>& Cards)
{
	if (Cards.size() > 8) return PDClassifyHandRaw(Cards);
	uint8_t Code[8];
	const int N = (int)Cards.size();
	for (int I = 0; I < N; ++I) Code[I] = (uint8_t)((Cards[I].IsWild() ? 15 : Cards[I].Rank) << 3 | (Cards[I].IsWild() ? 7 : (int)Cards[I].Suit));
	std::sort(Code, Code + N);
	uint64_t Key = (uint64_t)N;
	for (int I = 0; I < N; ++I) Key = Key << 7 | Code[I];
	static std::unordered_map<uint64_t, std::vector<EPDHand>> GHandMemo;
	if (auto It = GHandMemo.find(Key); It != GHandMemo.end()) return It->second;
	std::vector<EPDHand> Out = PDClassifyHandRaw(Cards);
	if (GHandMemo.size() > 400000) GHandMemo.clear();
	GHandMemo.emplace(Key, Out);
	return Out;
}

static std::vector<EPDHand> PDClassifyHandRaw(const std::vector<FPDCard>& Cards)
{
	// 와일드 카드 (D-480): 예전엔 와일드마다 숫자 2~14 · 판의 무늬를 다 넣어 보고 나온 족보를 모았다. 같은 결과를 바로 구한다
	//  와일드 무늬는 와일드 아닌 카드의 무늬 중에서만 고르므로(없으면 스페이드) 플러시 = 나머지 무늬가 모두 같음
	int W = 0;
	for (const FPDCard& C : Cards) W += C.IsWild();
	if (W > 0)
	{
		const int N = (int)Cards.size(), M = N - W;
		std::map<int, int> Cnt;
		bool bSuit = true;
		int Suit = -1, MaxCnt = 0, HiMin = 99, HiMax = 0, LoMin = 99, LoMax = 0;
		for (const FPDCard& C : Cards)
		{
			if (C.IsWild()) continue;
			MaxCnt = std::max(MaxCnt, ++Cnt[C.Rank]);
			if (Suit < 0) Suit = (int)C.Suit; else bSuit = bSuit && Suit == (int)C.Suit;
			const int Lo = C.Rank == 14 ? 1 : C.Rank;
			HiMin = std::min(HiMin, C.Rank); HiMax = std::max(HiMax, C.Rank);
			LoMin = std::min(LoMin, Lo); LoMax = std::max(LoMax, Lo);
		}
		const int D = (int)Cnt.size();
		// 스트레이트: 3장부터 (D-418), 숫자가 모두 다르고, N 칸 창(2~14, 또는 A = 1 로 1~13)에 다 들어간다
		bool bStraight = false;
		if (N >= 3 && D == M)
		{
			if (M == 0) bStraight = N <= 13;
			else bStraight = (HiMax - HiMin < N && std::max(2, HiMax - N + 1) + N - 1 <= 14) || (LoMax - LoMin < N && std::max(1, LoMax - N + 1) + N - 1 <= 13);
		}
		const bool bRoyal = N == 5 && bSuit && D == M && (M == 0 || HiMin >= 10);
		std::vector<EPDHand> Out; // 예전처럼 족보 번호 순서 (std::set)
		if (bRoyal) Out.push_back(EPDHand::RoyalFlush);
		if (N == 5 && D <= 1) Out.push_back(EPDHand::Five);
		if (N == 5 && bSuit && bStraight) Out.push_back(EPDHand::StraightFlush);
		if (N == 4 && D <= 1) Out.push_back(EPDHand::Four);
		if (N == 5 && D <= 2 && MaxCnt <= 3) Out.push_back(EPDHand::FullHouse);
		if (N >= 3 && bSuit) Out.push_back(EPDHand::Flush);
		if (bStraight) Out.push_back(EPDHand::Straight);
		if (N == 3 && D <= 1) Out.push_back(EPDHand::Triple);
		if (N == 4 && D <= 2 && MaxCnt <= 2) Out.push_back(EPDHand::TwoPair);
		if (N == 2 && D <= 1) Out.push_back(EPDHand::Pair);
		return Out;
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
	if (N >= 3 && bStraight) Out.push_back(EPDHand::Straight); // 이어진 숫자는 3장부터 (D-418: 2장은 플러시와 구분이 안 가서 뺐다, 예전 2장부터)
	return Out;
}

// ---------------------------------------------------------------- 시너지
static const int GPDN4[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
// 칸 K 의 상하좌우 이웃 칸 키마다 Fn (판 밖은 건너뜀)
template <class F> static void PDForNeighbors(int K, F&& Fn)
{
	const int X = K % Cols, Y = K / Cols;
	for (const auto& D : GPDN4)
	{
		const int NX = X + D[0], NY = Y + D[1];
		if (NX >= 0 && NY >= 0 && NX < Cols && NY < Rows) Fn(CellKey(NX, NY));
	}
}
// 두 칸의 맨해튼 거리 (1 = 맞닿음)
static int PDCellDist(int A, int B) { return std::abs(A % Cols - B % Cols) + std::abs(A / Cols - B / Cols); }

// ESU 방식으로 연결 묶음(2~KMax칸)을 한 번씩만 만든다: 시작점보다 번호가 큰 칸으로만 넓힌다
// Viable(Sub) 가 false 면 그 묶음과 그것을 품는 더 큰 묶음은 족보가 될 수 없어 더 넓히지 않는다 (D-134: 큰 판에서 61ms → 1ms 남짓)
// 찾은 묶음은 저장하지 않고 바로 OnSub 으로 넘긴다 (D-142: 묶음마다 벡터를 새로 잡던 비용)
template <class FViable, class FOnSub>
static void PDExtend(const std::vector<std::vector<int>>& Nb, std::vector<int>& Sub, const std::vector<int>& Ext, int Root, int KMax, const FViable& Viable, const FOnSub& OnSub)
{
	if (!Viable(Sub)) return;
	if (Sub.size() >= 2) OnSub(Sub);
	if ((int)Sub.size() == KMax) return;
	std::vector<int> E = Ext;
	while (!E.empty())
	{
		const int W = E.back();
		E.pop_back();
		int Near[32], NNear = 0; // 지금 묶음의 이웃 (묶음 5칸 × 이웃 4 = 최대 20)
		for (int U : Sub) for (int Z : Nb[U]) if (NNear < 32) Near[NNear++] = Z;
		auto IsNear = [&](int U) { for (int I = 0; I < NNear; ++I) if (Near[I] == U) return true; return false; };
		std::vector<int> Next = E;
		for (int U : Nb[W])
			if (U > Root && std::find(Sub.begin(), Sub.end(), U) == Sub.end() && U != W && !IsNear(U) && std::find(E.begin(), E.end(), U) == E.end())
				Next.push_back(U);
		Sub.push_back(W);
		PDExtend(Nb, Sub, Next, Root, KMax, Viable, OnSub);
		Sub.pop_back();
	}
}

// 스트레이트는 순서대로 (D-108): 칸들이 맞닿아 한 줄기로 이어지고, 그 줄기를 따라 숫자가 1씩 오른다 (꺾여도 됨, 거꾸로 읽어도 됨).
// 조커·와일드는 아무 숫자, A 는 1 또는 14. 5칸이면 120가지 순서를 다 본다
// NeedBase: 가장 낮은 숫자가 꼭 이것이어야 함 (로열 = 10, D-190). -1 이면 아무 숫자
static bool PDOrderedStraight(const std::vector<int>& Cells, const std::vector<FPDCard>& Cards, int NeedBase = -1)
{
	const int N = (int)Cells.size();
	std::vector<int> P(N);
	for (int I = 0; I < N; ++I) P[I] = I;
	do
	{
		bool bPath = true;
		for (int I = 1; I < N && bPath; ++I) bPath = PDCellDist(Cells[P[I - 1]], Cells[P[I]]) == 1;
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
			if (!bOk) continue;
			if (NeedBase >= 0) { if (!bHave || Base == NeedBase) return true; } // 와일드만이면 어디든 맞출 수 있다
			else if (!bHave || (Base >= 1 && Base + N - 1 <= 14)) return true;
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
			PDForNeighbors(K, [&](int NK) {
				if (Towers.count(NK) && !Seen.count(NK) && (Towers.at(NK).Level == Towers.at(K).Level || Towers.at(NK).IsJoker() || Towers.at(K).IsJoker())) { Seen.insert(NK); Stack.push_back(NK); } // 같은 레벨끼리만 이어짐 (조커는 아무 레벨과)
			});
		}
		if (Comp.size() < 2) continue;
		std::sort(Comp.begin(), Comp.end());
		// 덩어리 캐시 (D-142): 덩어리마다 족보 찾기는 서로 무관하다. 칸·카드·레벨이 같으면 지난 결과를 그대로 쓴다
		// (카드를 놓거나 미리보기하면 그 덩어리만, 조커 레벨업은 아무 덩어리도 다시 찾지 않는다). 연쇄는 아래에서 판 전체로 다시 이음
		uint64_t CompKey = 1469598103934665603ull;
		for (int K : Comp)
		{
			const FPDTower& T = Towers.at(K);
			const uint64_t V = (uint64_t)K << 20 ^ (uint64_t)(T.IsJoker() ? 0 : T.Card.Rank) << 12 ^ (uint64_t)(T.IsJoker() ? 4 : (int)T.Card.Suit) << 8 ^ (uint64_t)(T.IsJoker() ? 15 : T.Level);
			CompKey = (CompKey ^ V) * 1099511628211ull;
		}
		CompKey ^= (uint64_t)Comp.size() << 56;
		static std::unordered_map<uint64_t, std::vector<FPDSynergy>> GCompCache;
		if (auto Hit = GCompCache.find(CompKey); Hit != GCompCache.end()) { Picked.insert(Picked.end(), Hit->second.begin(), Hit->second.end()); continue; }
		const size_t PickedBefore = Picked.size();
		std::map<int, int> Idx;
		for (int I = 0; I < (int)Comp.size(); ++I) Idx[Comp[I]] = I;
		std::vector<std::vector<int>> Nb(Comp.size());
		for (int I = 0; I < (int)Comp.size(); ++I)
			PDForNeighbors(Comp[I], [&](int NK) {
				auto It = Idx.find(NK);
				if (It != Idx.end()) Nb[I].push_back(It->second);
			});
		// 가지치기 (D-134): 와일드·조커를 뺀 카드가 ① 무늬가 모두 같거나 ② 숫자가 모두 다르고 폭이 4 이하(A 는 1 또는 14)거나
		// ③ 숫자 종류가 2개 이하일 때만 족보(또는 더 큰 족보의 일부)가 될 수 있다. 레벨이 섞여도 안 됨. 큰 묶음으로 갈수록 조건이 줄기만 하므로 여기서 끊어도 빠지는 족보가 없다
		std::vector<FPDCard> CardOf(Comp.size());
		std::vector<int> LvOf(Comp.size());
		for (int I = 0; I < (int)Comp.size(); ++I)
		{
			const FPDTower& T = Towers.at(Comp[I]);
			CardOf[I] = T.IsJoker() ? FPDCard{0, EPDSuit::Wild} : T.Card;
			LvOf[I] = T.IsJoker() ? -1 : T.Level;
		}
		auto Viable = [&](const std::vector<int>& Sub) {
			int Lv = -1, Suit = -1, NR = 0, Ranks[5], HiMin = 99, HiMax = 0, LoMin = 99, LoMax = 0;
			bool bSameSuit = true, bDistinct = true;
			for (int I : Sub)
			{
				if (LvOf[I] >= 0) { if (Lv < 0) Lv = LvOf[I]; else if (Lv != LvOf[I]) return false; }
				const FPDCard& C = CardOf[I];
				if (C.IsWild()) continue;
				if (Suit < 0) Suit = (int)C.Suit; else if (Suit != (int)C.Suit) bSameSuit = false;
				bool bSeen = false;
				for (int J = 0; J < NR; ++J) bSeen = bSeen || Ranks[J] == C.Rank;
				if (bSeen) bDistinct = false; else Ranks[NR++] = C.Rank;
				const int Lo = C.Rank == 14 ? 1 : C.Rank;
				HiMin = std::min(HiMin, C.Rank); HiMax = std::max(HiMax, C.Rank);
				LoMin = std::min(LoMin, Lo); LoMax = std::max(LoMax, Lo);
			}
			if (NR <= 2 || bSameSuit) return true;
			return bDistinct && (HiMax - HiMin <= 4 || LoMax - LoMin <= 4);
		};
		struct FCand { EPDHand Kind; int N, Value; std::vector<int> Cells; };
		std::vector<FCand> Cands;
		std::vector<FPDCard> Cards; // 묶음마다 다시 쓰는 그릇 (레벨 섞임은 Viable 에서 이미 걸렀다)
		std::vector<int> Cells;
		Cards.reserve(5);
		Cells.reserve(5);
		auto OnSub = [&](const std::vector<int>& Sub) {
			Cards.clear();
			Cells.clear();
			for (int I : Sub) { Cells.push_back(Comp[I]); Cards.push_back(CardOf[I]); } // 조커는 와일드
			int Ordered = -1; // 스트레이트 순서 검사 (필요할 때 한 번만)
			for (EPDHand H : ClassifyHand(Cards))
			{
				if (H == EPDHand::RoyalFlush) // 로열은 순서대로 놓았을 때 10 부터 시작해야 (D-190): 와일드 10 J Q K 에서 와일드가 10 쪽 끝이면 9 라서 스트레이트 플러시
				{
					if (!PDOrderedStraight(Cells, Cards, 10)) continue;
				}
				else if (H == EPDHand::Straight || H == EPDHand::StraightFlush)
				{
					if (Ordered < 0) Ordered = PDOrderedStraight(Cells, Cards) ? 1 : 0;
					if (!Ordered) continue;
				}
				Cands.push_back({H, (int)Sub.size(), HandValue(H, (int)Sub.size()), Cells});
			}
		};
		std::vector<int> Sub, Ext;
		for (int V = 0; V < (int)Comp.size(); ++V)
		{
			Sub.assign(1, V);
			Ext.clear();
			for (int U : Nb[V]) if (U > V) Ext.push_back(U);
			PDExtend(Nb, Sub, Ext, V, 5, Viable, OnSub);
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
		if (GCompCache.size() > 512) GCompCache.clear();
		GCompCache[CompKey].assign(Picked.begin() + PickedBefore, Picked.end());
	}
	// 연쇄: 족보끼리 맞닿아 있거나 카드를 나눠 쓰면 하나의 연쇄
	std::map<int, std::vector<int>> Owner;
	for (int I = 0; I < (int)Picked.size(); ++I) for (int K : Picked[I].Cells) Owner[K].push_back(I);
	std::vector<std::set<int>> Adj(Picked.size());
	for (int I = 0; I < (int)Picked.size(); ++I)
		for (int K : Picked[I].Cells)
		{
			for (int J : Owner[K]) if (J != I) Adj[I].insert(J);
			PDForNeighbors(K, [&](int NK) {
				auto It = Owner.find(NK);
				if (It != Owner.end()) for (int J : It->second) if (J != I) Adj[I].insert(J);
			});
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

// 족보 효과 (공격력·연사·사거리 배율, 범위). 합성 타워는 부적 없이(Charms = 0) 구한 값을 특성으로 물려받는다
struct FPDTrait
{
	float Dmg = 1.f, Rate = 1.f, Range = 1.f, Splash = 0.f;
};
static FPDTrait HandTrait(EPDHand H, int N, FPDCharmBits Charms = 0)
{
	const float PairK = HasCharm(Charms, EPDCharm::Twins) ? 2.f : 1.5f;
	FPDTrait T;
	switch (H)
	{
	case EPDHand::Pair: T.Dmg = PairK; break;
	case EPDHand::TwoPair: T.Dmg = PairK; T.Range = 1.25f; break;
	case EPDHand::Triple: T.Dmg = 2.f; break;
	case EPDHand::Straight: T.Rate = 1.f + (HasCharm(Charms, EPDCharm::LineLover) ? 0.5f : 0.25f) * (N - 1); break;
	case EPDHand::Flush: T.Splash = 0.5f + 0.25f * N + (HasCharm(Charms, EPDCharm::WideFlush) ? 0.75f : 0.f); break;
	case EPDHand::FullHouse: T.Dmg = HasCharm(Charms, EPDCharm::FullHouseFan) ? 2.25f : 1.5f; T.Rate = 1.5f; T.Splash = 1.5f; break;
	case EPDHand::Four: T.Dmg = HasCharm(Charms, EPDCharm::FourLeaf) ? 5.25f : 3.5f; break;
	case EPDHand::StraightFlush: T.Dmg = HasCharm(Charms, EPDCharm::RoyalGuard) ? 3.f : 2.f; T.Rate = 2.f; T.Splash = 2.f; break;
	case EPDHand::Five: T.Dmg = 5.f; break;
	case EPDHand::RoyalFlush: T.Dmg = HasCharm(Charms, EPDCharm::RoyalGuard) ? 3.75f : 2.5f; T.Rate = 2.5f; T.Splash = 2.5f; break;
	default: break;
	}
	return T;
}

std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers, FPDCharmBits Charms, const FPDTreeMods* Tree)
{
	static const FPDTreeMods NoTree;
	const FPDTreeMods& Tm = Tree ? *Tree : NoTree;
	std::vector<FPDSynergy> Syn = ComputeSynergies(Towers);
	const float Step = (HasCharm(Charms, EPDCharm::ChainLink) ? 0.45f : ChainStep) + Tm.ChainAdd + (Tm.Key(EPDKeystone::Crowd) ? 0.15f : 0.f);
	for (auto& KV : Towers) KV.second.Eff = FPDEffects{};
	for (int I = 0; I < (int)Syn.size(); ++I)
		for (int K : Syn[I].Cells)
		{
			FPDEffects& E = Towers[K].Eff;
			const int N = Syn[I].N;
			if (E.Hand < 0) { E.Hand = I; E.Kind = (int)Syn[I].Kind; E.N = N; } // 대표 족보: 가장 큰 것 (Syn 은 큰 족보부터)
			E.Kinds |= 1u << (int)Syn[I].Kind;
			const bool bSkip = Tm.Key(EPDKeystone::BigTable) && N == 2; // 큰 판: 2장 족보는 효과 없음
			const bool bOmniFlush = Tm.Key(EPDKeystone::Omni) && Syn[I].Kind == EPDHand::Flush; // 잡식가: 플러시 효과 없음
			if (!bSkip && !bOmniFlush)
			{
				const FPDTrait H = HandTrait(Syn[I].Kind, N, Charms);
				E.Dmg *= H.Dmg;
				E.Rate *= H.Rate;
				E.Range *= H.Range;
				E.Splash = std::max(E.Splash, H.Splash);
			}
			const bool bNoChain = bSkip || Tm.Key(EPDKeystone::Lone) || (Tm.Key(EPDKeystone::SmallTable) && N == 5); // 고독한 저격수·작은 판(5장)은 연쇄 덤 없음
			if (Syn[I].Chain >= 2 && E.Chain < 2) { if (!bNoChain) E.Dmg *= 1.f + Step * (Syn[I].Chain - 1); E.Chain = Syn[I].Chain; } // 연쇄 덤은 타워마다 한 번 (족보를 나눠 쓰면 같은 연쇄)
		}
	// 대장 지휘: 판 위 자기 족보 타워(족보에 든 타워, 또는 그 족보를 기억하는 합성 타워) 위력 +30%씩, 겹친다
	int Captains[(int)EPDHand::Count] = {};
	for (auto& KV : Towers)
	{
		if (!KV.second.IsJoker()) continue;
		const int Jk = KV.second.Joker;
		if (IsUpperJoker(Jk)) { Captains[(int)CaptainHand(UpperDef(Jk - UpperBase).A)]++; Captains[(int)CaptainHand(UpperDef(Jk - UpperBase).B)]++; } // 상위 조커는 재료 둘의 지휘 (D-231)
		else if (Jk >= 0 && Jk < (int)EPDJoker::Count && !Tm.Key(EPDKeystone::Crown)) Captains[(int)CaptainHand((EPDJoker)Jk)]++; // 왕관의 무게: 기본 조커 지휘 없음 (D-232)
	}
	for (auto& KV : Towers)
	{
		FPDTower& O = KV.second;
		if (O.IsJoker()) continue;
		const uint32_t Bits = O.Eff.Kinds ? O.Eff.Kinds : (O.FuseHand >= 0 ? 1u << O.FuseHand : 0u);
		int Cmd = 0;
		for (int H = 0; H < (int)EPDHand::Count; ++H) if ((Bits >> H) & 1u) Cmd += Captains[H]; // 여러 족보에 든 타워는 대장마다 지휘를 받는다
		if (Cmd > 0) O.Eff.Dmg *= 1.f + CaptainCommand * Cmd;
	}
	// 서커스 (D-232): 판 위 기본 조커 하나마다 모든 조커 위력 +15%
	if (Tm.Key(EPDKeystone::Circus))
	{
		int NBase = 0;
		for (auto& KV : Towers) NBase += KV.second.IsJoker() && !IsUpperJoker(KV.second.Joker);
		for (auto& KV : Towers) if (KV.second.IsJoker()) KV.second.Eff.Dmg *= 1.f + 0.15f * NBase;
	}
	// 조커 오라
	for (auto& KV : Towers)
	{
		const FPDTower& J = KV.second;
		if (!J.IsJoker()) continue;
		const int Kind = J.Joker;
		const EPDUpperSpecial Sp = JokerSpecial(Kind);
		float BestPow = -1.f;
		for (auto& OV : Towers)
		{
			FPDTower& O = OV.second;
			if (&O == &J || O.IsJoker()) continue;
			const bool bUp = Tm.Up(J); // 트리 조커 강화 또는 조커 레벨 10+
			const int Reach = (JokerHas(Kind, EPDJoker::Hatter) && bUp) ? 2 : 1;
			const bool bNear = std::abs(O.X - J.X) <= Reach && std::abs(O.Y - J.Y) <= Reach; // 주변 8칸 (강화한 모자 장수는 24칸)
			if (JokerHas(Kind, EPDJoker::Baton) && bNear) O.Eff.Rate *= (Sp == EPDUpperSpecial::Dormouse ? 1.8f : bUp ? 1.7f : 1.4f) * (Sp == EPDUpperSpecial::Walrus ? 1.f + WalrusFeastStep * J.Feast : 1.f); // 굴 잔치 (D-482)
			if (JokerHas(Kind, EPDJoker::Hatter) && bNear) { O.Eff.Dmg *= bUp ? 1.5f : 1.25f; O.Eff.Range *= Sp == EPDUpperSpecial::MadTea ? 30.f : 1.1f; } // 미친 다과회: 주변 사거리 무한
			if (JokerHas(Kind, EPDJoker::Harlequin) && !O.Card.IsWild() && O.Card.Suit == J.JSuit) O.Eff.Dmg *= bUp ? 1.6f : 1.3f;
			const bool bCopyable = Sp == EPDUpperSpecial::RedQueen ? (!O.Card.IsWild() && O.Card.Suit == J.JSuit) : (bNear || Sp == EPDUpperSpecial::GrinOnly); // 붉은 여왕: 판 위 같은 무늬, 웃음만 남은 고양이: 판 전체
			if (JokerHas(Kind, EPDJoker::Mask) && bCopyable)
			{
				const float Pow = Sp == EPDUpperSpecial::MockTurtle ? SuitDef(O.Card.Suit).Rate * O.TRate * O.Eff.Rate : (3.f + O.Card.Power()) * SuitDef(O.Card.Suit).Dmg * O.Mult * O.TDmg; // 가짜 거북: 가장 빠른 타워
				if (Pow > BestPow) { BestPow = Pow; Towers[KV.first].Eff.CopyKey = OV.first; }
			}
		}
	}
	// 무지개 칵테일 (D-239): 판에 ♠♥♦♣ 가 다 있으면 모든 타워 공격력 +25%
	if (HasCharm(Charms, EPDCharm::Rainbow))
	{
		uint32_t Suits = 0;
		for (const auto& KV : Towers) if (!KV.second.Card.IsWild() && !KV.second.IsJoker()) Suits |= 1u << (int)KV.second.Card.Suit;
		if (Suits == 0xFu) for (auto& KV : Towers) KV.second.Eff.Dmg *= 1.25f;
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
// 레시피 (D-118). EPDJoker 순서: 광대, 모자 장수, 웃는 가면, 희비극, 할리퀸, 지휘봉, 잭, 체셔, 방울, 스타
using H3 = std::array<EPDHand, 3>;
using H2 = std::array<EPDHand, 2>;
static const H3 GPDRecipe[(int)EPDJoker::Count] = {
	{EPDHand::Five, EPDHand::Straight, EPDHand::Flush},                // 광대
	{EPDHand::StraightFlush, EPDHand::Straight, EPDHand::Flush},       // 모자 장수
	{EPDHand::TwoPair, EPDHand::Pair, EPDHand::Straight},              // 웃는 가면
	{EPDHand::Pair, EPDHand::Straight, EPDHand::Flush},                // 희비극
	{EPDHand::Flush, EPDHand::Flush, EPDHand::Flush},                  // 할리퀸 (무늬가 다르게)
	{EPDHand::Straight, EPDHand::Straight, EPDHand::Straight},         // 지휘봉 (숫자가 안 이어지게)
	{EPDHand::Triple, EPDHand::Straight, EPDHand::Flush},              // 잭
	{EPDHand::Four, EPDHand::Straight, EPDHand::Flush},                // 체셔
	{EPDHand::FullHouse, EPDHand::Straight, EPDHand::Flush},           // 방울
	{EPDHand::RoyalFlush, EPDHand::StraightFlush, EPDHand::Flush},     // 스타
};
static const H2 GPDRecipe2[(int)EPDJoker::Count] = {
	{EPDHand::Five, EPDHand::Straight},           // 광대
	{EPDHand::StraightFlush, EPDHand::Flush},     // 모자 장수
	{EPDHand::TwoPair, EPDHand::Pair},            // 웃는 가면
	{EPDHand::Pair, EPDHand::Straight},           // 희비극
	{EPDHand::Flush, EPDHand::Flush},             // 할리퀸
	{EPDHand::Straight, EPDHand::Straight},       // 지휘봉
	{EPDHand::Triple, EPDHand::Straight},         // 잭
	{EPDHand::Four, EPDHand::Straight},           // 체셔
	{EPDHand::FullHouse, EPDHand::Flush},         // 방울
	{EPDHand::RoyalFlush, EPDHand::StraightFlush}, // 스타
};
const std::array<EPDHand, 3>& JokerRecipe(EPDJoker J) { return GPDRecipe[(int)J]; }
const std::array<EPDHand, 2>& JokerRecipe2(EPDJoker J) { return GPDRecipe2[(int)J]; }
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands)
{
	if (Hands.size() != 2 && Hands.size() != 3) return EPDJoker::Count;
	std::vector<int> A;
	for (EPDHand H : Hands) A.push_back((int)H);
	std::sort(A.begin(), A.end());
	for (int J = 0; J < (int)EPDJoker::Count; ++J)
	{
		std::vector<int> B;
		if (Hands.size() == 3) for (EPDHand H : GPDRecipe[J]) B.push_back((int)H);
		else for (EPDHand H : GPDRecipe2[J]) B.push_back((int)H);
		std::sort(B.begin(), B.end());
		if (A == B) return (EPDJoker)J;
	}
	return EPDJoker::Count;
}
float JokerTier(EPDJoker J)
{
	switch (J)
	{
	case EPDJoker::Comedy: case EPDJoker::Mask: case EPDJoker::Jack: case EPDJoker::Baton: return 2.f;
	case EPDJoker::Harlequin: case EPDJoker::Bell: case EPDJoker::Cheshire: return 2.5f;
	default: return 3.f; // 모자 장수, 광대, 스타
	}
}

EPDShot ShotKind(const FPDTower& T)
{
	if (T.IsJoker()) return T.Joker >= 0 && T.Joker < (int)EPDJoker::Count ? (EPDShot)((int)EPDShot::Juggle + T.Joker) : IsUpperJoker(T.Joker) ? (EPDShot)((int)EPDShot::Juggle + (int)UpperDef(T.Joker - UpperBase).A) : EPDShot::JokerLaser;
	// 발사체는 카드 무늬로만 (D-170): 족보에 들어가도 무늬 공격 방식 그대로. 족보는 위력·연사·범위 같은 효과만 준다
	// (예전엔 스트레이트 이상이면 족보 발사체로 바뀌어 같은 ♠ 가 화살·번개를 오갔다. 족보 발사체 종류는 남겨 두지만 쓰지 않는다)
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

FPDTowerStats TowerStats(const FPDTower& T, FPDCharmBits Charms, const FPDTreeMods* Tree)
{
	static const FPDTreeMods NoTree;
	const FPDTreeMods& Tm = Tree ? *Tree : NoTree;
	// 광대 조커는 무늬 4종 공격을 번갈아 쓴다
	const FPDSuitDef& S = (T.IsJoker() && JokerHas(T.Joker, EPDJoker::Clown)) ? SuitDef((EPDSuit)(T.Cycle % 4)) : SuitDef(T.Card.Suit);
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
	// 부적 상자 (D-233)
	const bool bNum = !T.Card.IsWild() && !T.IsJoker();
	if (HasCharm(Charms, EPDCharm::Aces) && bNum && R == 14) St.Dmg *= 3.f;
	if (HasCharm(Charms, EPDCharm::EvenNums) && bNum && R <= 10 && R % 2 == 0) St.Dmg *= 1.4f;
	if (HasCharm(Charms, EPDCharm::OddNums) && bNum && R <= 9 && R % 2 == 1) St.Rate *= 1.3f;
	if (HasCharm(Charms, EPDCharm::GlassCannon)) St.Dmg *= 1.3f;
	if (HasCharm(Charms, EPDCharm::LongShot) && bNum && T.Card.Suit == EPDSuit::Spade) St.Range *= 1.3f;
	if (HasCharm(Charms, EPDCharm::BigBang) && bNum && T.Card.Suit == EPDSuit::Heart) St.Splash += 0.5f;
	if (HasCharm(Charms, EPDCharm::Overclock) && bNum && T.Card.Suit == EPDSuit::Diamond) St.Rate *= 1.3f;
	if (HasCharm(Charms, EPDCharm::JokerFan) && T.IsJoker()) St.Dmg *= 1.3f;
	if (HasCharm(Charms, EPDCharm::Forge) && !T.IsJoker() && T.Level >= 2) St.Dmg *= 1.3f;
	if (HasCharm(Charms, EPDCharm::QueenGuard) && bNum && R == 12) St.Rate *= 1.8f;          // 상자 IV (D-234)
	if (HasCharm(Charms, EPDCharm::KingsReach) && bNum && R == 13) { St.Range *= 1.4f; St.Dmg *= 1.3f; }
	if (HasCharm(Charms, EPDCharm::ChainFever) && T.Eff.Chain >= 3) St.Dmg *= 1.3f;
	if (HasCharm(Charms, EPDCharm::LongHand) && T.Eff.N == 5) St.Dmg *= 1.35f;
	if (HasCharm(Charms, EPDCharm::Triad) && T.Eff.N == 3) St.Dmg *= 1.4f;
	if (HasCharm(Charms, EPDCharm::Loner) && T.Eff.Kind < 0 && !T.IsJoker()) St.Dmg *= 1.6f;
	if (HasCharm(Charms, EPDCharm::WildHeart) && T.Card.IsWild() && !T.IsJoker()) St.Dmg *= 2.f;
	// 칵테일 60종 (D-239)
	auto InHand = [&T](EPDHand H) { return ((T.Eff.Kinds >> (int)H) & 1u) != 0; };
	if (HasCharm(Charms, EPDCharm::JackGuard) && bNum && R == 11) St.Dmg *= 1.8f;
	if (HasCharm(Charms, EPDCharm::TwoRebel) && bNum && R == 2) St.Dmg *= 3.f;
	if (HasCharm(Charms, EPDCharm::StraightFlushFan) && (InHand(EPDHand::StraightFlush) || InHand(EPDHand::RoyalFlush))) St.Dmg *= 1.6f;
	if (HasCharm(Charms, EPDCharm::TwoPairFan) && InHand(EPDHand::TwoPair)) St.Dmg *= 1.6f;
	if (HasCharm(Charms, EPDCharm::TripleFan) && InHand(EPDHand::Triple)) St.Dmg *= 1.3f;
	if (HasCharm(Charms, EPDCharm::FourFan) && InHand(EPDHand::Four)) St.Dmg *= 1.5f;
	if (HasCharm(Charms, EPDCharm::TwoCards) && T.Eff.N == 2) St.Dmg *= 1.6f;
	if (HasCharm(Charms, EPDCharm::FourCards) && T.Eff.N == 4) St.Dmg *= 1.4f;
	if (HasCharm(Charms, EPDCharm::QuickFuse) && !T.IsJoker() && T.Level >= 2) St.Rate *= 1.3f;
	if (HasCharm(Charms, EPDCharm::BigJoker) && T.IsJoker()) St.Range *= 1.3f;
	if (HasCharm(Charms, EPDCharm::SniperScope) && bNum && T.Card.Suit == EPDSuit::Spade) St.Range *= 1.5f;
	if (HasCharm(Charms, EPDCharm::IceCrystal) && bNum && T.Card.Suit == EPDSuit::Club) St.Slow = std::min(0.75f, St.Slow + 0.15f);
	if (HasCharm(Charms, EPDCharm::Rapid) && bNum && T.Card.Suit == EPDSuit::Diamond) St.Rate *= 1.4f;
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
		St.Dmg *= 1.f + JokerLevelDmg * (T.JLevel - 1); // 조커 레벨: 레벨마다 위력 +50% (D-196)
		const bool bUp = Tm.Up(T);
		if (JokerHas(T.Joker, EPDJoker::Clown) && bUp) St.Rate *= 1.5f;                                          // 강화: 연사 +50%
		if (JokerHas(T.Joker, EPDJoker::Jack)) { St.Dmg *= bUp ? 5.f : 3.f; St.Rate *= 0.5f; }                    // 아주 강한 한 방
		if (JokerHas(T.Joker, EPDJoker::Cheshire)) { St.Range = 99.f; if (bUp) St.Dmg *= 1.5f; }                  // 사거리 무한
		if (JokerHas(T.Joker, EPDJoker::Star)) { St.Dmg *= bUp ? 0.8f : 0.5f; St.Rate = 1.f; St.Range = 99.f; }   // 판 전체 레이저 (모든 적)
	}
	return St;
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
	Xp = Points = XpEarned = 0;
	Level = 1;
	Swaps = 0;
	XpCarry = 0.f;
	TreeOwned = FPDTreeBits(1);
	Mods = BuildTreeMods(TreeOwned);
	ApplyUpgrades(Mods, Upg); // 영구 강화 (D-230)
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
	SuitSwaps.clear();
	ChipPiles.clear();
	AddFreshDeck();
	Path.Build(GeneratePath(Rng));
	Draw(StartHand);
}

// 중단 저장 (D-162). 한 줄에 하나: 이름 값... (공백 구분). 판 상태만 (도감·최고 기록은 따로 저장)
std::string FPDGame::SaveRun() const
{
	std::ostringstream O;
	O.precision(9);
	O << "pdrun 1\n";
	O << "mode " << (int)Mode << "\nwave " << Wave << "\nlives " << Lives << "\nkills " << Kills << "\nleaks " << Leaks << "\ndiscarded " << Discarded << "\nrefills " << DeckRefills << "\n";
	O << "xp " << Xp << "\nlevel " << Level << "\npoints " << Points << "\nearned " << XpEarned << "\ncarry " << XpCarry << "\nswaps " << Swaps << "\nspeed " << Speed << "\n";
	O << "upg"; for (int U : Upg) O << ' ' << U; O << "\n";
	O << "tree " << TreeOwned.to_string() << "\n";
	O << "path " << Path.Corners.size(); for (const auto& C : Path.Corners) O << ' ' << C[0] << ' ' << C[1]; O << "\n";
	auto Cards = [&](const char* Name, const std::vector<FPDCard>& V) { O << Name << ' ' << V.size(); for (const FPDCard& C : V) O << ' ' << C.Rank << ' ' << (int)C.Suit; O << "\n"; };
	Cards("deck", Deck);
	Cards("hand", Hand);
	O << "charms " << Charms.size(); for (EPDCharm C : Charms) O << ' ' << (int)C; O << "\n";
	for (const auto& KV : Towers)
	{
		const FPDTower& T = KV.second;
		O << "tower " << KV.first << ' ' << T.Card.Rank << ' ' << (int)T.Card.Suit << ' ' << T.Level << ' ' << T.Mult << ' ' << T.TDmg << ' ' << T.TRate << ' ' << T.TRange << ' ' << T.TSplash
		  << ' ' << T.FuseHand << ' ' << T.Joker << ' ' << (int)T.JSuit << ' ' << T.JLevel << ' ' << T.JXp << ' ' << T.Cycle << "\n";
	}
	O << "rng " << Rng << "\n";
	O << "end\n";
	return O.str();
}

bool FPDGame::LoadRun(const std::string& S)
{
	std::istringstream In(S);
	std::string Tag;
	int Ver = 0;
	if (!(In >> Tag >> Ver) || Tag != "pdrun" || Ver != 1) return false;
	FPDGame N(*this); // 실패하면 그대로 두려고 사본에 읽는다
	N.Towers.clear(); N.Deck.clear(); N.Hand.clear(); N.Charms.clear();
	std::vector<std::array<int, 2>> Corners;
	bool bEnd = false;
	auto ReadCards = [&](std::vector<FPDCard>& V) { size_t K = 0; In >> K; for (size_t I = 0; I < K && In; ++I) { int R = 0, Su = 0; In >> R >> Su; V.push_back({R, (EPDSuit)Su}); } };
	while (In >> Tag)
	{
		if (Tag == "mode") { int M = 0; In >> M; N.Mode = (EPDMode)std::clamp(M, 0, (int)EPDMode::Count - 1); }
		else if (Tag == "wave") In >> N.Wave;
		else if (Tag == "lives") In >> N.Lives;
		else if (Tag == "kills") In >> N.Kills;
		else if (Tag == "leaks") In >> N.Leaks;
		else if (Tag == "discarded") In >> N.Discarded;
		else if (Tag == "refills") In >> N.DeckRefills;
		else if (Tag == "xp" || Tag == "gold") In >> N.Xp; // 예전 저장의 골드는 경험치로
		else if (Tag == "level") In >> N.Level;
		else if (Tag == "points") In >> N.Points;
		else if (Tag == "earned") In >> N.XpEarned;
		else if (Tag == "carry") In >> N.XpCarry;
		else if (Tag == "swaps") In >> N.Swaps;
		else if (Tag == "goldupg") { int Skip = 0; In >> Skip; } // 예전 판 안 강화 (D-228 에서 없앰)
		else if (Tag == "speed") In >> N.Speed;
		else if (Tag == "upg") { int Skip = 0; for (size_t I = 0; I < N.Upg.size(); ++I) In >> Skip; } // 강화는 판 밖 영구 강화가 정한다 (D-228)
		else if (Tag == "tree") { std::string B; In >> B; N.TreeOwned = FPDTreeBits(B); }
		else if (Tag == "path") { size_t K = 0; In >> K; for (size_t I = 0; I < K && In; ++I) { std::array<int, 2> C{}; In >> C[0] >> C[1]; Corners.push_back(C); } }
		else if (Tag == "deck") ReadCards(N.Deck);
		else if (Tag == "hand") ReadCards(N.Hand);
		else if (Tag == "charms") { size_t K = 0; In >> K; for (size_t I = 0; I < K && In; ++I) { int C = 0; In >> C; N.Charms.push_back((EPDCharm)C); } }
		else if (Tag == "tower")
		{
			int Key = 0, Su = 0, JS = 0;
			FPDTower T;
			In >> Key >> T.Card.Rank >> Su >> T.Level >> T.Mult >> T.TDmg >> T.TRate >> T.TRange >> T.TSplash >> T.FuseHand >> T.Joker >> JS >> T.JLevel >> T.JXp >> T.Cycle;
			T.Card.Suit = (EPDSuit)Su;
			T.JSuit = (EPDSuit)JS;
			T.X = Key % Cols;
			T.Y = Key / Cols;
			T.Age = 10.f; // 연출 없이
			if (Key >= 0 && Key < Cols * Rows) N.Towers[Key] = T;
		}
		else if (Tag == "rng") In >> N.Rng;
		else if (Tag == "end") { bEnd = true; break; }
		else return false; // 모르는 줄
		if (!In) return false;
	}
	if (!bEnd || Corners.size() < 2) return false;
	N.Path.Build(Corners);
	N.Phase = EPDPhase::Build;
	N.AutoStartT = -1.f;
	N.Time = 0.f;
	N.Enemies.clear(); N.Pending.clear(); N.Born.clear(); N.Projectiles.clear(); N.Rewards.clear(); N.Events.clear(); N.Sounds.clear(); N.SuitSwaps.clear(); N.ChipPiles.clear();
	N.RebuildMods();
	*this = N;
	return true;
}

int FPDGame::AddXp(float Amount, bool bBonus)
{
	XpCarry += Amount * (bBonus ? 1.f + Mods.XpPct : 1.f);
	const int Got = (int)(XpCarry + 1e-4f); // 0.15 × 20 이 2.9999 가 되지 않게
	XpCarry -= Got;
	Xp += Got;
	XpEarned += Got;
	int Ups = 0;
	while (XpToNext() > 0 && Xp >= XpToNext())
	{
		Xp -= XpToNext();
		Level++;
		Ups++;
	}
	if (Ups > 0)
	{
		if (Has(EPDCharm::Herald)) Lives += Ups; // 전령 (D-234)
		FPDEvent E;
		E.Kind = FPDEvent::EKind::LevelUp;
		E.A = Level;
		E.B = Ups;
		Ev(E);
		Sfx(EPDSound::Rotate, 1);
	}
	return Got;
}

int FPDGame::KillXp(EPDEnemy T, int W) const
{
	const FPDEnemyDef& D = EnemyDef(T);
	if (D.bBoss) return 25;
	if (T == EPDEnemy::Mini) return 1;
	const int Base = T == EPDEnemy::Tank ? 3 : (T == EPDEnemy::Normal || T == EPDEnemy::Fast) ? 1 : 2;
	return Base + (W - 1) / 5; // 5웨이브마다 +1
}

int FPDGame::XpToNext() const { return (int)std::min(1.0e9, std::round(PointBasePrice * std::pow((double)PointPriceGrowth, (double)(Level - 1)))); } // 레벨 94 넘으면 int 가 넘쳐 음수 → AddXp 무한 반복이던 것 (D-268): 10억에서 멈춘다
float FPDGame::FuseMult() const { return FuseBonus + Mods.FuseAdd + (Mods.Key(EPDKeystone::Smith) ? 0.5f : 0.f); }
float FPDGame::JokerMult() const { return JokerBonus + Mods.JokerAdd + (Mods.Key(EPDKeystone::ClownPath) ? 0.4f : 0.f); }

void FPDGame::RebuildMods()
{
	Mods = BuildTreeMods(TreeOwned);
	ApplyUpgrades(Mods, Upg);
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
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
	if (!InPlay()) return EBuy::Closed;
	if (Id < 0 || Id >= TreeNodeCount()) return EBuy::Closed;
	if (TreeOwnedNode(Id)) return EBuy::Owned;
	if (TreeSealed(Id)) return EBuy::Sealed;
	bool bLinked = false;
	for (int O : TreeLinks(Id)) bLinked = bLinked || TreeOwnedNode(O);
	if (!bLinked) return EBuy::NotLinked;
	if (TreeNode(Id).Kind == EPDNode::Keystone && KeystoneCount() >= KeystoneMax) return EBuy::KeyCap;
	if (PointsLeft() <= 0) return EBuy::NoPoint;
	return EBuy::Ok;
}

bool FPDGame::BuyNode(int Id)
{
	if (CanBuy(Id) != EBuy::Ok) return false;
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

FPDCharmBits FPDGame::CharmMask() const
{
	FPDCharmBits M = 0;
	for (EPDCharm J : Charms) M |= FPDCharmBits(1) << (int)J;
	return M;
}

void FPDGame::AddFreshDeck()
{
	for (int S = 0; S < 4; ++S) for (int R = 2; R <= 14; ++R) Deck.push_back({R, (EPDSuit)S});
	std::shuffle(Deck.begin(), Deck.end(), Rng);
}

void FPDGame::Draw(int N, int* OutGot)
{
	int Got = 0;
	while (Got < N && (int)Hand.size() < HandMax())
	{
		if (Deck.empty()) // 무한 진행: 덱이 비면 새 덱 한 벌을 섞어 채운다
		{
			AddFreshDeck();
			DeckRefills++;
		}
		Hand.push_back(Deck.back());
		Deck.pop_back();
		Got++;
	}
	if (OutGot) *OutGot = Got;
}

bool FPDGame::Buildable(int X, int Y) const { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && !Path.IsPath(X, Y); }
bool FPDGame::CanPlace(int X, int Y) const
{
	if (!Buildable(X, Y)) return false;
	auto It = Towers.find(CellKey(X, Y));
	return It == Towers.end() || (!It->second.IsJoker() && It->second.Level <= 1);
}

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

// ---------------------------------------------------------------- 보스 차례 (D-335): 30 · 60 · 100 모드는 고정 (25웨이브까지 셋이 같다), 무한은 100 뒤 판마다 뽑기
namespace
{
struct FPDFixedBoss { int W; EPDBossForm Form; EPDEnemy A, B, C; uint8_t Mut; };
constexpr EPDEnemy NoB = EPDEnemy::Count;
const FPDFixedBoss GPDFixedBoss[] = {
	{5, EPDBossForm::Solo, EPDEnemy::ShieldBoss, NoB, NoB, 0},
	{10, EPDBossForm::Solo, EPDEnemy::BigBlind, NoB, NoB, 0},
	{15, EPDBossForm::Duo, EPDEnemy::Charger, EPDEnemy::Bluffer, NoB, 0},
	{20, EPDBossForm::Escort, EPDEnemy::SplitBoss, NoB, NoB, 0},
	{25, EPDBossForm::Duo, EPDEnemy::Summoner, EPDEnemy::RegenBoss, NoB, 0},
	{30, EPDBossForm::Solo, EPDEnemy::Phantom, NoB, NoB, 0},
	{35, EPDBossForm::Duo, EPDEnemy::Dealer, EPDEnemy::Counter, NoB, 0},
	{40, EPDBossForm::Escort, EPDEnemy::Berserker, NoB, NoB, 0},
	{45, EPDBossForm::Duo, EPDEnemy::Warlord, EPDEnemy::Blinker, NoB, 0},
	{50, EPDBossForm::Trio, EPDEnemy::Devourer, EPDEnemy::Summoner, EPDEnemy::Warlord, 0},
	{55, EPDBossForm::Escort, EPDEnemy::Dealer, NoB, NoB, 0},
	{60, EPDBossForm::Duo, EPDEnemy::Warden, EPDEnemy::Hexer, NoB, 0},
	{65, EPDBossForm::Solo, EPDEnemy::HouseRule, NoB, NoB, 0},
	{70, EPDBossForm::Duo, EPDEnemy::Twins, NoB, NoB, 0},
	{75, EPDBossForm::Trio, EPDEnemy::Breaker, EPDEnemy::Warlord, EPDEnemy::Devourer, 0},
	{80, EPDBossForm::Escort, EPDEnemy::HouseRule, NoB, NoB, 0},
	{85, EPDBossForm::Trio, EPDEnemy::Warden, EPDEnemy::Hexer, EPDEnemy::Dealer, MutFast},
	{90, EPDBossForm::Duo, EPDEnemy::Twins, NoB, NoB, MutHard},
	{95, EPDBossForm::Trio, EPDEnemy::Breaker, EPDEnemy::HouseRule, EPDEnemy::Counter, MutRegen},
	{100, EPDBossForm::Trio, EPDEnemy::Warden, EPDEnemy::Hexer, EPDEnemy::Devourer, MutFast}, // 무한 모드 100 (100 모드는 최종 보스)
};
// 무한 뽑기 주머니 (일반 보스 19종)
const EPDEnemy GPDBossPool[] = {EPDEnemy::ShieldBoss, EPDEnemy::SplitBoss, EPDEnemy::RegenBoss, EPDEnemy::Charger, EPDEnemy::Summoner, EPDEnemy::Phantom, EPDEnemy::Warlord,
	EPDEnemy::Berserker, EPDEnemy::Blinker, EPDEnemy::Devourer, EPDEnemy::Warden, EPDEnemy::Hexer, EPDEnemy::Breaker,
	EPDEnemy::Bluffer, EPDEnemy::BigBlind, EPDEnemy::Dealer, EPDEnemy::Counter, EPDEnemy::HouseRule, EPDEnemy::Twins};
uint32_t PDBossHash(uint32_t X) { X ^= X >> 16; X *= 0x7feb352du; X ^= X >> 15; X *= 0x846ca68bu; X ^= X >> 16; return X; }
}

std::vector<FPDBossSlot> FPDGame::BossWave(int W, EPDBossForm* OutForm) const
{
	std::vector<FPDBossSlot> Out;
	EPDBossForm Form = EPDBossForm::Solo;
	if (W <= 0 || W % 5 != 0) { if (OutForm) *OutForm = Form; return Out; }
	if (IsFinalWave(W)) // 모드 전용 최종 보스 + 동료 (30 호위 · 60 짝 · 100 셋)
	{
		Form = EPDBossForm::Final;
		if (Mode == EPDMode::W30) Out = {{EPDEnemy::Bouncer, 0}};
		else if (Mode == EPDMode::W60) Out = {{EPDEnemy::HighRoller, 0}, {EPDEnemy::Berserker, 0}, {EPDEnemy::Blinker, 0}};
		else Out = {{EPDEnemy::House, 0}, {EPDEnemy::Warden, 0}, {EPDEnemy::Hexer, 0}, {EPDEnemy::Devourer, 0}};
	}
	else if (W <= 100)
	{
		for (const FPDFixedBoss& F : GPDFixedBoss)
			if (F.W == W)
			{
				Form = F.Form;
				for (EPDEnemy T : {F.A, F.B, F.C}) if (T != NoB) Out.push_back({T, F.Mut});
			}
	}
	else // 무한 100 뒤 (D-335): 판 씨앗(길 모양)으로 뽑기, 짜임은 돌아가며, 변이는 100웨이브마다 하나씩 늘어남
	{
		uint32_t Seed = 2166136261u;
		for (const auto& Cn : Path.Corners) Seed = (Seed ^ (uint32_t)(Cn[0] * 31 + Cn[1])) * 16777619u;
		uint32_t R = PDBossHash(Seed ^ (uint32_t)W * 2654435761u);
		static const EPDBossForm Rot[4] = {EPDBossForm::Duo, EPDBossForm::Trio, EPDBossForm::Escort, EPDBossForm::Duo};
		Form = Rot[(W / 5) % 4];
		const int N = Form == EPDBossForm::Trio ? 3 : Form == EPDBossForm::Duo ? 2 : 1;
		const int NMut = std::min(6, 1 + (W - 101) / 100);
		for (int I = 0; I < N; ++I)
		{
			EPDEnemy T = EPDEnemy::ShieldBoss;
			for (int Try = 0; Try < 20; ++Try)
			{
				R = PDBossHash(R + 0x9e3779b9u);
				T = GPDBossPool[R % (sizeof(GPDBossPool) / sizeof(GPDBossPool[0]))];
				bool bDup = T == EPDEnemy::Twins && N > 2; // 쌍둥이는 짝 · 호위에서만
				for (const FPDBossSlot& S : Out) bDup = bDup || S.T == T;
				if (!bDup) break;
			}
			uint8_t Mut = 0;
			for (int M = 0; M < NMut; ++M) { R = PDBossHash(R + 77u); Mut |= (uint8_t)(1u << (R % 6)); }
			Out.push_back({T, Mut});
		}
	}
	if (OutForm) *OutForm = Form;
	return Out;
}

std::vector<FPDEnemy> FPDGame::WaveSpec(int W)
{
	std::vector<FPDEnemy> List;
	// NumWaves(15)웨이브까지 ×1.45, 그 뒤로는 모드마다 다른 배율 (무한 ×1.3, D-229)
	const float BossHp = 26.f * std::pow(1.45f, (float)(std::min(W, NumWaves) - 1)) * std::pow(ModeGrowth(Mode), (float)std::max(0, W - NumWaves)) * EarlyHpK(W); // 초반 완화 (D-106)
	const float Hp = BossHp * WaveScale(W) * MobHpBonus * GPDDebugHpMul; // 마리 수를 늘린 만큼 한 마리 체력을 낮춘다 (범위 공격이 세지는 몫은 MobHpBonus 로 되돌림)
	const int N = WaveCount(W) * GPDDebugCountMul;
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
	// 보스 (D-335): 5웨이브마다 BossWave 의 짜임대로. 짝 · 셋은 한 마리 체력을 나누고(각 ×0.6 · ×0.45), 호위는 보스 ×0.8 + 정예 넷 (D-334)
	if (W % 5 == 0)
	{
		EPDBossForm Form = EPDBossForm::Solo;
		const std::vector<FPDBossSlot> Slots = BossWave(W, &Form);
		const int NComp = (int)Slots.size() - (Form == EPDBossForm::Final ? 1 : 0); // 최종 보스 옆 동료 수
		const float CompK = Form == EPDBossForm::Duo ? 0.6f : Form == EPDBossForm::Trio ? 0.45f : Form == EPDBossForm::Escort ? 0.8f : Form == EPDBossForm::Final ? (NComp >= 3 ? 0.45f : NComp == 2 ? 0.6f : 0.8f) : 1.f;
		for (const FPDBossSlot& S : Slots)
		{
			const bool bFin = IsFinalBoss(S.T);
			const float K = bFin ? 1.f : CompK;
			const int Copies = S.T == EPDEnemy::Twins ? 2 : 1; // 쌍둥이는 둘이 한 묶음
			int FirstId = 0;
			for (int Cp = 0; Cp < Copies; ++Cp)
			{
				FPDEnemy B = MakeEnemy(S.T, BossHp * GPDDebugHpMul * K, W);
				B.Mut = S.Mut;
				B.XpK = bFin ? 1.f : K;
				if (S.Mut & MutHard) B.Armor += Boss::MutArmor;
				if (S.Mut & MutFast) B.Speed *= Boss::MutSpeed;
				if (S.Mut & MutShield) B.Shield = B.MaxHp * Boss::MutShield;
				if (Cp == 0) FirstId = B.Id; else { B.Link = FirstId; for (FPDEnemy& Q : List) if (Q.Id == FirstId) Q.Link = B.Id; }
				List.push_back(B);
			}
			if (S.T == EPDEnemy::Bluffer) // 분신 둘: 체력 1, 맞기 전엔 진짜와 똑같이 보인다 (위젯)
				for (int I = 0; I < Boss::DecoyN; ++I)
				{
					FPDEnemy Dc = MakeEnemy(EPDEnemy::Decoy, 1.f, W);
					Dc.Hp = Dc.MaxHp = 1.f;
					Dc.XpK = 0.f;
					List.push_back(Dc);
				}
		}
		const bool bEscort = Form == EPDBossForm::Escort || (Form == EPDBossForm::Final && NComp == 0);
		if (bEscort)
			for (int I = 0; I < 4; ++I) // 정예 넷: 탱크 · 갑옷 병정 체력 ×3
			{
				FPDEnemy El = MakeEnemy(I % 2 ? EPDEnemy::Armored : EPDEnemy::Tank, Hp * 3.f, W);
				El.bElite = true;
				List.push_back(El);
			}
	}
	return List;
}

void FPDGame::StartWave()
{
	if (Phase != EPDPhase::Build) return;
	PreWaveSave = SaveRun(); // 중단 저장 (D-162): 웨이브 중에 그만두면 여기로
	++SaveSerial;
	AutoStartT = -1.f;
	WaveStartT = Time;
	Wave++;
	Phase = EPDPhase::Wave;
	Pending = WaveSpec(Wave);
	WaveTotal = (int)Pending.size();
	SpawnT = 0.f;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::WaveStart;
	E.A = Wave;
	for (const FPDEnemy& P : Pending) if (EnemyDef(P.Type).bBoss) { E.B = 1; E.Enemy = P.Type; break; } // 짝 · 셋이면 첫째 (D-334)
	Ev(E);
	Sfx(EPDSound::Start);
}

void FPDGame::EndWave()
{
	Projectiles.clear(); // 남은 발사체는 맞힐 적이 없다
	RestoreSuits(); // 딜러가 바꾼 무늬 · 칩 더미는 웨이브가 끝나면 원래대로 (D-334)
	bool bFeast = false; // 굴 잔치는 웨이브마다 처음부터 (D-482)
	for (auto& KV : Towers) if (KV.second.Feast) { KV.second.Feast = 0; bFeast = true; }
	if (bFeast) Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	int Got = 0;
	Draw(DrawPerWave + (Has(EPDCharm::Supply) ? 1 : 0) + (Has(EPDCharm::Dexterity) ? 1 : 0) + Mods.DrawAdd, &Got);
	AddXp((5.f + Wave) * (Has(EPDCharm::Sweeper) ? 2.f : 1.f)); // 웨이브 통과 덤 (청소부 ×2, D-234)
	if (Goal() > 0 && Wave >= Goal()) // 모드 목표 달성 (D-229)
	{
		Phase = EPDPhase::Clear;
		BestWave = std::max(BestWave, Wave);
		FPDEvent C;
		C.Kind = FPDEvent::EKind::RunClear;
		C.A = Wave;
		C.B = (int)Mode;
		Ev(C);
		Sfx(EPDSound::Clear);
		return;
	}
	if (Wave % CharmEvery == 0) // 부적 창은 5웨이브마다 (D-101)
	{
		Phase = EPDPhase::Reward;
		Rewards = RollRewards();
	}
	else // 그 사이는 창 없이 AutoStartDelayFor 초(1분, D-185) 뒤 다음 웨이브
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
	for (int J = 0; J < (int)EPDCharm::Count; ++J) if (((CharmUnlocked >> J) & 1u) && std::find(Charms.begin(), Charms.end(), (EPDCharm)J) == Charms.end()) Pool.push_back(J); // 잠긴 부적은 안 나옴 (D-233)
	std::shuffle(Pool.begin(), Pool.end(), Rng);
	const bool bWild = std::uniform_real_distribution<float>(0.f, 1.f)(Rng) < WildChance;
	const bool bSwap = !bWild && std::uniform_real_distribution<float>(0.f, 1.f)(Rng) < SwapChance; // 자리바꿈 (D-149)
	const int NCharm = (int)Charms.size() < CharmMax ? std::min(bWild || bSwap ? 2 : 3, (int)Pool.size()) : 0;
	for (int I = 0; I < NCharm; ++I) Out.push_back({true, Pool[I]});
	std::vector<int> Inst{(int)EPDInstant::Draw2, (int)EPDInstant::Repair, (int)EPDInstant::Xp};
	if (!bSwap) Inst.push_back((int)EPDInstant::Swap);
	std::shuffle(Inst.begin(), Inst.end(), Rng);
	if (bWild) Inst.insert(Inst.begin(), (int)EPDInstant::Wild);
	if (bSwap) Inst.insert(Inst.begin(), (int)EPDInstant::Swap);
	for (int I = 0; Out.size() < 3 && I < (int)Inst.size(); ++I) Out.push_back({false, Inst[I]});
	return Out;
}

void FPDGame::TakeReward(int Index)
{
	if (Phase != EPDPhase::Reward) return;
	if (Index >= 0 && Index < (int)Rewards.size())
	{
		const FPDReward R = Rewards[Index];
		if (R.bCharm) { if ((int)Charms.size() < CharmMax) { Charms.push_back((EPDCharm)R.Id); if ((EPDCharm)R.Id == EPDCharm::GlassCannon) Lives = std::max(1, Lives - 3); } } // 유리 대포: 목숨 −3
		else if (R.Id == (int)EPDInstant::Wild)
		{
			const FPDCard W{0, EPDSuit::Wild};
			if ((int)Hand.size() < HandMax()) Hand.push_back(W);
			else Deck.push_back(W);
		}
		else if (R.Id == (int)EPDInstant::Draw2) Draw(2);
		else if (R.Id == (int)EPDInstant::Repair) Lives += 3;
		else if (R.Id == (int)EPDInstant::Xp) AddXp(10.f + Wave * 4.f, false);
		else if (R.Id == (int)EPDInstant::Swap) Swaps += SwapPerReward;
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
	AutoStartT = AutoStartDelayFor(Wave + 1); // 부적을 고르면 1분(D-185) 뒤 다음 웨이브
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

// 자리바꿈 (D-149): From 타워를 To 로. To 에 타워가 있으면 서로 바꾸고, 빈 칸이면 옮긴다 (길 위·판 밖은 안 됨).
// 레벨·조커·경험치 등 타워 상태는 그대로 따라가고, 족보는 다시 찾는다
bool FPDGame::SwapTowers(int From, int To)
{
	if (!InPlay() || Swaps <= 0 || From == To || !Towers.count(From)) return false;
	const int TX = To % Cols, TY = To / Cols;
	const bool bOther = Towers.count(To) > 0;
	if (!bOther && !Buildable(TX, TY)) return false;
	FPDTower A = Towers[From];
	if (bOther)
	{
		FPDTower B = Towers[To];
		B.X = From % Cols; B.Y = From / Cols;
		Towers[From] = B;
	}
	else Towers.erase(From);
	A.X = TX; A.Y = TY;
	Towers[To] = A;
	--Swaps;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	for (int K : {From, To})
	{
		if (!Towers.count(K)) continue;
		FPDEvent P;
		P.Kind = FPDEvent::EKind::Place;
		P.X = K % Cols + 0.5f;
		P.Y = K / Cols + 0.5f;
		P.Suit = Towers[K].Card.Suit;
		Ev(P);
	}
	return true;
}

bool FPDGame::Place(int HandIdx, int X, int Y)
{
	if (!InPlay()) return false;
	if (HandIdx < 0 || HandIdx >= (int)Hand.size() || !CanPlace(X, Y)) return false;
	const int Key = CellKey(X, Y);
	auto Sig = [](const FPDSynergy& S) { std::vector<int> V = S.Cells; std::sort(V.begin(), V.end()); V.push_back((int)S.Kind); return V; };
	std::set<std::vector<int>> Before;
	for (const FPDSynergy& S : Synergies) Before.insert(Sig(S));
	FPDTower T;
	T.Card = Hand[HandIdx];
	T.X = X;
	T.Y = Y;
	Hand.erase(Hand.begin() + HandIdx);
	if (auto Old = Towers.find(Key); Old != Towers.end()) Hand.insert(Hand.begin() + HandIdx, Old->second.Card); // 교체 (D-192): 덮인 Lv1 카드는 놓은 카드 자리로 손에 돌아온다 (손패 수는 그대로)
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

double TowerPower(const FPDTowerStats& S) { return (double)(S.Dmg * S.Rate * S.CritFactor()) * (1 + S.Splash * 0.8); }

double OneTowerPower(const FPDTowerMap& Towers, const FPDTower& T, FPDCharmBits Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight)
{
	auto W = [&](const FPDTower& At, float Range) { return Weight ? Weight(At, Range) : 1.0; };
	{
		const FPDTowerStats S = TowerStats(T, Charms, Tree);
		double V = TowerPower(S) * W(T, S.Range);
		if (T.IsJoker())
		{
			if (JokerHas(T.Joker, EPDJoker::Mask) && T.Eff.CopyKey >= 0 && Towers.count(T.Eff.CopyKey)) // 옆 타워 따라 함 (가면 자리에서 그 타워의 사거리로)
			{
				const FPDTowerStats O = TowerStats(Towers.at(T.Eff.CopyKey), Charms, Tree);
				V = std::max(V, TowerPower(O) * W(T, O.Range));
			}
			if (JokerHas(T.Joker, EPDJoker::Comedy)) V *= 1.8; // 두 발
			if (JokerHas(T.Joker, EPDJoker::Star)) V *= 4.0;   // 모든 적 (한 번에 적 여럿)
			if (JokerHas(T.Joker, EPDJoker::Bell)) V *= 1.3;   // 멈춤
		}
		return V;
	}
}

double SumBoardPower(const FPDTowerMap& Towers, FPDCharmBits Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight)
{
	double Sum = 0;
	for (const auto& KV : Towers) Sum += OneTowerPower(Towers, KV.second, Charms, Tree, Weight);
	return Sum;
}

// 판의 타워는 바뀔 때마다 ApplyEffects 를 마쳐 두므로 족보를 다시 찾지 않는다 (D-142: 큰 판에서 카드 놓을 때마다 17ms × 2 가 더 들던 것)
std::vector<std::pair<int, float>> FPDGame::TowerPowers() const
{
	std::vector<std::pair<int, float>> Out;
	for (const auto& KV : Towers) Out.push_back({KV.first, (float)OneTowerPower(Towers, KV.second, CharmMask(), &Mods)});
	std::stable_sort(Out.begin(), Out.end(), [](const std::pair<int, float>& A, const std::pair<int, float>& B) { return A.second > B.second; });
	return Out;
}

float FPDGame::SynergyGain() const
{
	const double With = SumBoardPower(Towers, CharmMask(), &Mods); // 이미 효과가 걸린 판
	FPDTowerMap T = Towers;
	for (auto& KV : T) KV.second.Eff = FPDEffects(); // 족보·연쇄·조커 오라 없이 (합성 배율·특성은 그대로)
	const double Without = SumBoardPower(T, CharmMask(), &Mods);
	return Without > 0 ? (float)(With / Without) : 1.f;
}

float BoardPower(FPDTowerMap Towers, FPDCharmBits Charms, const FPDTreeMods* Tree)
{
	ApplyEffects(Towers, Charms, Tree);
	return (float)SumBoardPower(Towers, Charms, Tree);
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
	case EPowerAction::JokerMerge: bOk = Copy.MergeJokers(Idx); break;
	}
	if (bOk) P.After = Copy.Power();
	return P;
}

bool FPDGame::CanFuse(int SynergyIdx) const
{
	if (SynergyIdx < 0 || SynergyIdx >= (int)Synergies.size()) return false;
	return FuseBlock(SynergyIdx) == EFuseBlock::None;
}

FPDGame::EFuseBlock FPDGame::FuseBlock(int SynergyIdx) const
{
	if (!InPlay()) return EFuseBlock::NotInPlay;
	if (SynergyIdx < 0 || SynergyIdx >= (int)Synergies.size()) return EFuseBlock::NotInPlay;
	FPDSynergy S;
	if (!FuseSynergy(SynergyIdx, S)) return EFuseBlock::Joker;
	const int Lv = Towers.at(S.Cells[0]).Level;
	if (Lv >= MaxLevel) return EFuseBlock::TopLevel;
	if (Mods.Key(EPDKeystone::ClownPath) && Lv >= 2) return EFuseBlock::ClownPath; // 광대의 길: 합성은 레벨 2까지
	return EFuseBlock::None;
}

bool FPDGame::FuseSynergy(int SynergyIdx, FPDSynergy& Out) const
{
	if (SynergyIdx < 0 || SynergyIdx >= (int)Synergies.size()) return false;
	const FPDSynergy& S = Synergies[SynergyIdx];
	for (int K : S.Cells) if (Towers.at(K).IsJoker()) return false; // 조커가 낀 족보는 합성 불가 (D-200, 예전 D-128 은 조커를 뺀 칸만 합성)
	Out = S;
	return true;
}

std::vector<FPDJokerGroup> FPDGame::JokerGroups() const
{
	std::vector<FPDJokerGroup> Out;
	const std::vector<FPDJokerNode> Nodes = JokerNodes();
	// 3개 조합과 2개 조합 중 레시피와 정확히 맞는 것 (D-115, D-118). 거리는 상관없다 (D-195)
	struct FCand { int N[3]; int Count; EPDJoker J; int Value; };
	std::vector<FCand> Cands;
	const int NN = (int)Nodes.size();
	std::vector<int> Kind(NN);
	for (int I = 0; I < NN; ++I) Kind[I] = Nodes[I].Kind == EPDHand::Count ? -1 : (int)Nodes[I].Kind;
	for (int I = 0; I < NN; ++I)
		for (int J = I + 1; J < NN; ++J)
		{
			if (Kind[I] < 0 || Kind[J] < 0 || JokerNodesClash(Nodes[I], Nodes[J])) continue;
			{
				const EPDJoker R = MatchJokerRecipe({(EPDHand)Kind[I], (EPDHand)Kind[J]});
				if (R != EPDJoker::Count && JokerOpen((int)R)) Cands.push_back({{I, J, -1}, 2, R, HandBaseValue((EPDHand)Kind[I]) + HandBaseValue((EPDHand)Kind[J])});
			}
			for (int K = J + 1; K < NN; ++K)
			{
				if (Kind[K] < 0 || JokerNodesClash(Nodes[I], Nodes[K]) || JokerNodesClash(Nodes[J], Nodes[K])) continue;
				const EPDJoker R = MatchJokerRecipe({(EPDHand)Kind[I], (EPDHand)Kind[J], (EPDHand)Kind[K]});
				if (R != EPDJoker::Count && JokerOpen((int)R)) Cands.push_back({{I, J, K}, 3, R, HandBaseValue((EPDHand)Kind[I]) + HandBaseValue((EPDHand)Kind[J]) + HandBaseValue((EPDHand)Kind[K])});
			}
		}
	// 3개 레시피 먼저 (카드가 많아 세다), 같은 개수끼리는 낮은 족보부터, 칸이 겹치지 않게
	std::stable_sort(Cands.begin(), Cands.end(), [](const FCand& X, const FCand& Y) { return X.Count != Y.Count ? X.Count > Y.Count : X.Value < Y.Value; });
	std::set<int> UsedCells;
	for (const FCand& Cd : Cands)
	{
		FPDJokerGroup G;
		bool bFree = true;
		for (int Q = 0; Q < Cd.Count; ++Q)
		{
			const int N = Cd.N[Q];
			for (int K : Nodes[N].Cells) bFree = bFree && !UsedCells.count(K);
			if (Nodes[N].Syn >= 0) G.SynergyIdx.push_back(Nodes[N].Syn);
			else G.FusedCells.push_back(Nodes[N].Fused);
			G.Cells.insert(G.Cells.end(), Nodes[N].Cells.begin(), Nodes[N].Cells.end());
		}
		if (!bFree) continue;
		std::sort(G.Cells.begin(), G.Cells.end());
		G.Cells.erase(std::unique(G.Cells.begin(), G.Cells.end()), G.Cells.end()); // 나눠 쓴 카드는 한 번만
		for (int K : G.Cells) UsedCells.insert(K);
		G.Result = Cd.J;
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

std::vector<FPDJokerNode> FPDGame::JokerNodes() const
{
	std::vector<FPDJokerNode> Nodes;
	if (!InPlay()) return Nodes;
	if (Mods.Key(EPDKeystone::Smith)) return Nodes; // 대장장이: 조커 포탑을 만들 수 없다
	// 재료 후보: 조커가 없는 족보 + 족보에 안 든 합성 타워
	std::set<int> InSyn;
	for (int I = 0; I < (int)Synergies.size(); ++I)
	{
		bool bJoker = false;
		for (int K : Synergies[I].Cells) { InSyn.insert(K); bJoker = bJoker || Towers.at(K).IsJoker(); }
		if (!bJoker) { FPDJokerNode N; N.Syn = I; N.Cells = Synergies[I].Cells; N.Kind = Synergies[I].Kind; Nodes.push_back(N); }
	}
	for (const auto& KV : Towers)
		if (KV.second.Level >= 2 && !KV.second.IsJoker() && KV.second.FuseHand >= 0) // 족보에 들어 있어도 기억한 족보로 재료 (D-220)
		{
			FPDJokerNode N;
			N.Fused = KV.first;
			N.Cells = {KV.first};
			N.Kind = (EPDHand)KV.second.FuseHand;
			Nodes.push_back(N);
		}
	return Nodes;
}

bool FPDGame::JokerNodesClash(const FPDJokerNode& A, const FPDJokerNode& B)
{
	auto In = [](const FPDJokerNode& S, int Cell) { return std::find(S.Cells.begin(), S.Cells.end(), Cell) != S.Cells.end(); };
	return (A.Fused >= 0 && In(B, A.Fused)) || (B.Fused >= 0 && In(A, B.Fused));
}

EPDJoker FPDGame::JokerCraftResult(const std::vector<int>& Picks) const
{
	if (Picks.size() != 2 && Picks.size() != 3) return EPDJoker::Count;
	const std::vector<FPDJokerNode> Nodes = JokerNodes();
	std::vector<EPDHand> Hands;
	for (size_t I = 0; I < Picks.size(); ++I)
	{
		if (Picks[I] < 0 || Picks[I] >= (int)Nodes.size()) return EPDJoker::Count;
		for (size_t J = 0; J < I; ++J) if (Picks[J] == Picks[I] || JokerNodesClash(Nodes[Picks[J]], Nodes[Picks[I]])) return EPDJoker::Count; // 같은 재료 두 번, 합성 타워와 그 족보 (D-220)
		Hands.push_back(Nodes[Picks[I]].Kind);
	}
	const EPDJoker R = MatchJokerRecipe(Hands);
	return JokerOpen((int)R) ? R : EPDJoker::Count; // 잠긴 조커는 못 만든다 (D-239)
}

bool FPDGame::JokerCraft(const std::vector<int>& Picks, int TargetCell)
{
	const EPDJoker R = JokerCraftResult(Picks);
	if (R == EPDJoker::Count) return false;
	const std::vector<FPDJokerNode> Nodes = JokerNodes();
	FPDJokerGroup G;
	for (int P : Picks)
	{
		const FPDJokerNode& N = Nodes[P];
		if (N.Syn >= 0) G.SynergyIdx.push_back(N.Syn);
		else G.FusedCells.push_back(N.Fused);
		G.Cells.insert(G.Cells.end(), N.Cells.begin(), N.Cells.end());
	}
	std::sort(G.Cells.begin(), G.Cells.end());
	G.Cells.erase(std::unique(G.Cells.begin(), G.Cells.end()), G.Cells.end()); // 나눠 쓴 카드는 한 번만
	G.Result = R;
	return JokerFuseGroup(G, TargetCell);
}

std::vector<int> FPDGame::BestJokerPicks(EPDJoker J) const
{
	const std::vector<FPDJokerNode> Ns = JokerNodes();
	auto MultOf = [&](int N) { float M = 0.f; for (int K : Ns[N].Cells) M += Towers.at(K).Mult; return M; };
	for (const int Count : {3, 2})
	{
		std::vector<EPDHand> Kinds;
		if (Count == 3) for (EPDHand H : JokerRecipe(J)) Kinds.push_back(H);
		else for (EPDHand H : JokerRecipe2(J)) Kinds.push_back(H);
		std::sort(Kinds.begin(), Kinds.end());
		std::vector<std::vector<int>> Cand(Count);
		for (int S = 0; S < Count; ++S)
		{
			for (int I = 0; I < (int)Ns.size(); ++I) if (Ns[I].Kind == Kinds[S]) Cand[S].push_back(I);
			std::stable_sort(Cand[S].begin(), Cand[S].end(), [&](int A, int B) { return MultOf(A) > MultOf(B); });
			if (Cand[S].size() > 4) Cand[S].resize(4);
		}
		std::vector<int> Pick(Count, -1), Best;
		double BestScore = -1.0;
		std::function<void(int)> Go = [&](int S) {
			if (S == Count)
			{
				FPDGame Copy = *this;
				Copy.bMuted = true;
				if (!Copy.JokerCraft(Pick, -1)) return;
				const double Score = Copy.Power();
				if (Score > BestScore) { BestScore = Score; Best = Pick; }
				return;
			}
			for (int N : Cand[S])
			{
				if (S > 0 && Kinds[S] == Kinds[S - 1] && N <= Pick[S - 1]) continue; // 같은 족보 자리는 순서 하나만
				if (std::find(Pick.begin(), Pick.begin() + S, N) != Pick.begin() + S) continue;
				Pick[S] = N;
				Go(S + 1);
			}
		};
		Go(0);
		if (!Best.empty()) return Best;
	}
	return {};
}

int FPDGame::JokerMissing(EPDJoker J, EPDHand* Missing) const
{
	const std::vector<FPDJokerNode> Ns = JokerNodes();
	std::map<int, int> Need, Have;
	for (EPDHand H : JokerRecipe(J)) Need[(int)H]++;
	for (const FPDJokerNode& N : Ns) Have[(int)N.Kind]++;
	int Miss = 0;
	if (Missing) *Missing = EPDHand::Count;
	for (const auto& KV : Need)
	{
		const int M = std::max(0, KV.second - Have[KV.first]);
		if (M > 0 && Missing) *Missing = (EPDHand)KV.first;
		Miss += M;
	}
	return Miss;
}

bool FPDGame::JokerFuse(int GroupIdx, int TargetCell)
{
	const std::vector<FPDJokerGroup> Groups = JokerGroups();
	if (GroupIdx < 0 || GroupIdx >= (int)Groups.size()) return false;
	return JokerFuseGroup(Groups[GroupIdx], TargetCell);
}

bool FPDGame::JokerFuseGroup(const FPDJokerGroup& G, int TargetCell)
{
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
	J.Mult = Mult * JokerMult() * JokerTier(G.Result) * (1.f + JokerCardBonus * (float)G.Cells.size()); // 등급 배율 (D-116), 재료 카드가 많을수록 세다 (D-114)
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

std::vector<FPDJokerMerge> FPDGame::JokerMerges() const
{
	std::vector<FPDJokerMerge> Out;
	if (!InPlay()) return Out;
	std::set<int> Seen;
	for (const auto& KV : Towers)
	{
		if (!KV.second.IsJoker() || Seen.count(KV.first)) continue;
		FPDJokerMerge M;
		M.Joker = KV.second.Joker;
		std::vector<int> St{KV.first};
		Seen.insert(KV.first);
		while (!St.empty())
		{
			const int K = St.back();
			St.pop_back();
			M.Cells.push_back(K);
			PDForNeighbors(K, [&](int NK) {
				auto It = Towers.find(NK);
				if (It != Towers.end() && It->second.IsJoker() && It->second.Joker == M.Joker && !Seen.count(NK)) { Seen.insert(NK); St.push_back(NK); }
			});
		}
		if (M.Cells.size() >= 2)
		{
			std::sort(M.Cells.begin(), M.Cells.end());
			Out.push_back(M);
		}
	}
	// 상위 조커 (D-231): 맞닿은 서로 다른 기본 조커 둘, 레시피가 있고 둘 다 조건 레벨 이상. 서커스 키스톤이면 못 만듦 (D-232)
	for (const auto& KV : Towers)
	{
		const FPDTower& A = KV.second;
		if (Mods.Key(EPDKeystone::Circus) || !A.IsJoker() || A.Joker < 0 || A.Joker >= (int)EPDJoker::Count) continue;
		PDForNeighbors(KV.first, [&](int NK) {
			if (NK <= KV.first) return; // 쌍은 한 번만
			auto It = Towers.find(NK);
			if (It == Towers.end()) return;
			const FPDTower& B = It->second;
			if (!B.IsJoker() || B.Joker < 0 || B.Joker >= (int)EPDJoker::Count || B.Joker == A.Joker) return;
			const int U = UpperOf((EPDJoker)A.Joker, (EPDJoker)B.Joker);
			if (U < 0 || !UpperOpen(U)) return; // 잠긴 상위 조커는 못 만든다 (D-241)
			if (A.JLevel < UpperNeedFor(U, A) || B.JLevel < UpperNeedFor(U, B)) return; // 레벨 조건은 대표 11종(★)만 (D-333)
			FPDJokerMerge M;
			M.Joker = UpperBase + U;
			M.Cells = {KV.first, NK};
			M.Upper = U;
			Out.push_back(M);
		});
	}
	return Out;
}

int FPDGame::UpperNeedFor(int U, const FPDTower& T) const { return UpperDef(U).Special == EPDUpperSpecial::None ? 0 : UpperNeed(T); } // 일반 19종은 맞닿기만 하면, ★ 11종은 레벨 조건 (D-333)
int FPDGame::UpperNeed(const FPDTower& T) const { return T.Joker >= 0 && T.Joker < (int)EPDJoker::Count && Mods.Up(T.Joker) ? UpperNeedLevelTip : Mods.UpperPrep > 0.f ? 7 : UpperNeedLevel; } // 트리 끝자락 5, 다과회 초대장 7 (D-232)

bool FPDGame::MergeJokers(int MergeIdx, int TargetCell)
{
	const std::vector<FPDJokerMerge> List = JokerMerges();
	if (MergeIdx < 0 || MergeIdx >= (int)List.size()) return false;
	const FPDJokerMerge& M = List[MergeIdx];
	const int Target = PDFuseTarget(M.Cells, TargetCell);
	if (Target < 0) return false;
	FPDTower J = Towers.at(Target);
	if (M.Upper >= 0) // 상위 조커 (D-231, D-333): 위력 합 × 1.5 (★) / 1.3 (일반), 트리 끝자락 재료 하나마다 +25%, 조커 레벨은 높은 쪽
	{
		float Sum = 0.f;
		int Tips = 0, Lv = 1;
		EPDSuit JS = J.JSuit;
		for (int K : M.Cells)
		{
			const FPDTower& T = Towers.at(K);
			Sum += T.Mult;
			Tips += Mods.Up(T.Joker) ? 1 : 0;
			Lv = std::max(Lv, T.JLevel);
			if (T.Joker == (int)EPDJoker::Harlequin) JS = T.JSuit; // 할리퀸의 무늬를 이어받음
			J.Level = std::max(J.Level, T.Level); // 타워 레벨(사거리)은 큰 쪽 — 같은 조커 합치기 · 조커 만들기와 같게 (D-481: 예전엔 고른 칸의 레벨이라 칸에 따라 사거리가 달랐다)
			J.TDmg = std::max(J.TDmg, T.TDmg);
			J.TRate = std::max(J.TRate, T.TRate);
			J.TRange = std::max(J.TRange, T.TRange);
			J.TSplash = std::max(J.TSplash, T.TSplash);
		}
		const bool bFirst = !(UpperFound & (1u << M.Upper));
		J.Joker = UpperBase + M.Upper;
		J.JSuit = JS;
		J.JLevel = Lv;
		J.JXp = 0.f;
		const float Bonus = Mods.Key(EPDKeystone::Crown) ? 2.2f : UpperDef(M.Upper).Special == EPDUpperSpecial::None ? UpperBonusPlain : UpperBonus; // 왕관의 무게 ×2.2 (D-232), 일반 19종 ×1.3 · ★ ×1.5 (D-333)
		J.Mult = Sum * Bonus * (1.f + UpperTipBonus * Tips + Mods.UpperPrep);
		J.Cd = 0.f;
		J.StunT = 0.f;
		J.Timer = 0.f;
		for (int K : M.Cells) Towers.erase(K);
		Towers[Target] = J;
		UpperFound |= 1u << M.Upper;
		Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
		FPDEvent E;
		E.Kind = FPDEvent::EKind::JokerFuse;
		E.X = J.X + 0.5f;
		E.Y = J.Y + 0.5f;
		E.A = J.Joker;
		E.B = bFirst ? 1 : 0;
		Ev(E);
		Sfx(EPDSound::Win);
		return true;
	}
	float Mult = 0.f;
	for (int K : M.Cells)
	{
		const FPDTower& T = Towers.at(K);
		Mult += T.Mult;
		if (T.JLevel > J.JLevel || (T.JLevel == J.JLevel && T.JXp > J.JXp)) { J.JLevel = T.JLevel; J.JXp = T.JXp; }
		J.Level = std::max(J.Level, T.Level);
		J.TDmg = std::max(J.TDmg, T.TDmg);
		J.TRate = std::max(J.TRate, T.TRate);
		J.TRange = std::max(J.TRange, T.TRange);
		J.TSplash = std::max(J.TSplash, T.TSplash);
	}
	J.Mult = Mult * JokerMergeBonus;
	J.Cd = 0.f;
	J.StunT = 0.f;
	for (int K : M.Cells) Towers.erase(K);
	Towers[Target] = J;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::JokerMerge;
	E.X = J.X + 0.5f;
	E.Y = J.Y + 0.5f;
	E.A = J.Joker;
	E.B = (int)M.Cells.size();
	Ev(E);
	Sfx(EPDSound::Win);
	return true;
}

std::vector<FPDFeed> FPDGame::FeedTargets(int OnlyJoker) const
{
	std::vector<FPDFeed> Out;
	if (!InPlay()) return Out;
	for (int I = 0; I < (int)Synergies.size(); ++I)
	{
		const FPDSynergy& S = Synergies[I];
		// 먹을 조커: 족보 안에 든 조커(와일드로 든 것) 먼저, 없으면 맞닿은 조커 중 레벨이 가장 높은 것
		int Best = -1, Inside = -1;
		float MultSum = 0.f;
		int Fed = 0;
		bool bOnly = false; // OnlyJoker 가 이 족보에 들었거나 맞닿았다
		for (int K : S.Cells)
		{
			const FPDTower& T = Towers.at(K);
			if (T.IsJoker()) { bOnly = bOnly || K == OnlyJoker; if (Inside < 0 || T.JLevel > Towers.at(Inside).JLevel) Inside = K; continue; }
			MultSum += T.Mult;
			Fed++;
			PDForNeighbors(K, [&](int NK) {
				auto It = Towers.find(NK);
				if (It == Towers.end() || !It->second.IsJoker()) return;
				bOnly = bOnly || NK == OnlyJoker;
				if (Best < 0 || It->second.JLevel > Towers.at(Best).JLevel) Best = It->first;
			});
		}
		if (Inside >= 0) Best = Inside;
		if (OnlyJoker >= 0) { if (!bOnly) continue; Best = OnlyJoker; } // 고른 조커가 먹는다 (D-213)
		if (Best < 0 || Fed == 0 || Towers.at(Best).JLevel >= JokerMaxLevel) continue;
		// 경험치 = 족보 점수 × 먹인 타워 평균 위력 (합성 타워일수록 많이)
		// 흡수 (D-191): 먹인 타워의 실제 세기((3+숫자)·배율·특성)를 조커 단위로 바꿔 60% 를 더한다. 조커 카드는 와일드라 숫자 10
		const FPDTower& Jt = Towers.at(Best);
		float Val = 0.f;
		for (int K : S.Cells) { const FPDTower& T = Towers.at(K); if (!T.IsJoker()) Val += (3.f + T.Card.Power()) * T.Mult * T.TDmg * T.TRate; }
		const float Gain = (Has(EPDCharm::FoodChain) ? 0.9f : JokerFeedAbsorb) * Val / (13.f * std::max(0.01f, Jt.TDmg * Jt.TRate)); // 먹이 사슬 칵테일 90% (D-239)
		Out.push_back({I, Best, S.Value * MultSum / Fed, Gain});
	}
	return Out;
}

bool FPDGame::Feed(int SynergyIdx, int JokerKey)
{
	const std::vector<FPDFeed> List = FeedTargets(JokerKey);
	const FPDFeed* F = nullptr;
	for (const FPDFeed& X : List) if (X.Syn == SynergyIdx) F = &X;
	if (!F) return false;
	const int JKey = F->Joker;
	const float FeedXp = F->Xp, Gain = F->Gain;
	for (int K : Synergies[SynergyIdx].Cells) if (!Towers.at(K).IsJoker()) Towers.erase(K);
	FPDTower& J = Towers.at(JKey);
	const int Old = J.JLevel;
	J.Mult += Gain; // 먹인 위력 흡수 (D-191)
	J.JXp += FeedXp * (Has(EPDCharm::Feeder) ? 1.5f : 1.f); // 먹이 상자 부적 (D-233)
	while (J.JLevel < JokerMaxLevel && J.JXp >= JokerXpNeed(J.JLevel)) { J.JXp -= JokerXpNeed(J.JLevel); J.JLevel++; }
	if (J.JLevel >= JokerMaxLevel) J.JXp = 0.f;
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Feed;
	E.X = J.X + 0.5f;
	E.Y = J.Y + 0.5f;
	E.A = J.JLevel;
	E.B = J.JLevel - Old;
	E.R = FeedXp;
	Ev(E);
	Sfx(EPDSound::Burst, std::min(8, 2 + E.B), E.B > 0 ? 1 : 0);
	if (E.B > 0) Sfx(EPDSound::Clear);
	return true;
}

bool FPDGame::Fuse(int SynergyIdx, int TargetCell)
{
	if (!CanFuse(SynergyIdx)) return false;
	FPDSynergy S;
	FuseSynergy(SynergyIdx, S); // 조커가 낀 족보는 CanFuse 에서 이미 걸렀다 (D-200)
	// 합쳐질 칸: 고른 칸 (재료 칸 중 하나), 안 골랐으면 족보 가운데에 가장 가까운 칸
	const int Target = PDFuseTarget(S.Cells, TargetCell);
	if (Target < 0) return false;
	// 대표 카드 (D-193): 합쳐질 칸(고른 칸)의 숫자·무늬를 그대로 쓴다.
	// 그 칸이 와일드면 예전처럼 표결: 가장 많은 숫자(같으면 높은 쪽) + 가장 많은 무늬. 와일드는 표를 던지지 않는다
	std::map<int, int> RankCnt;
	std::map<int, int> SuitCnt;
	FPDTrait Old;
	float Mult = 0.f;
	int TopLevel = 1;
	for (int K : S.Cells)
	{
		const FPDTower& T = Towers.at(K);
		if (!T.Card.IsWild()) { RankCnt[T.Card.Rank]++; SuitCnt[(int)T.Card.Suit]++; }
		Mult += T.Mult;
		TopLevel = std::max(TopLevel, T.Level);
		Old.Dmg = std::max(Old.Dmg, T.TDmg);
		Old.Rate = std::max(Old.Rate, T.TRate);
		Old.Range = std::max(Old.Range, T.TRange);
		Old.Splash = std::max(Old.Splash, T.TSplash);
	}
	FPDTower F;
	F.X = Target % Cols;
	F.Y = Target / Cols;
	if (!Towers.at(Target).Card.IsWild()) F.Card = Towers.at(Target).Card;
	else if (RankCnt.empty()) F.Card = {0, EPDSuit::Wild};
	else
	{
		int BestR = 0, BestC = -1, BestS = 0, BestSC = -1;
		for (auto& KV : RankCnt) if (KV.second >= BestC) { BestC = KV.second; BestR = KV.first; }
		for (auto& KV : SuitCnt) if (KV.second > BestSC) { BestSC = KV.second; BestS = KV.first; }
		F.Card = {BestR, (EPDSuit)BestS};
	}
	const FPDTrait H = HandTrait(S.Kind, S.N);
	F.Level = std::min(MaxLevel, TopLevel + 1);
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

int FPDGame::SellPrice(int Cell) const
{
	if (!InPlay()) return 0;
	auto It = Towers.find(Cell);
	if (It == Towers.end()) return 0;
	const int B = 2 + Wave / 5;
	const FPDTower& T = It->second;
	return T.IsJoker() ? B * 10 : B * T.Level * T.Level;
}

bool FPDGame::SellTower(int Cell)
{
	const int Price = SellPrice(Cell);
	if (Price <= 0) return false;
	const FPDTower T = Towers.at(Cell);
	Towers.erase(Cell);
	AddXp((float)Price, false); // 경험치 덤(트리)은 안 붙는다
	Synergies = ApplyEffects(Towers, CharmMask(), &Mods);
	FPDEvent E;
	E.Kind = FPDEvent::EKind::Discard;
	E.X = T.X + 0.5f;
	E.Y = T.Y + 0.5f;
	E.A = Price;
	Ev(E);
	Sfx(EPDSound::Rotate);
	return true;
}

bool FPDGame::GainJokerXp(int Key, float Amount)
{
	auto It = Towers.find(Key);
	if (It == Towers.end() || !It->second.IsJoker() || It->second.JLevel >= JokerMaxLevel) return false;
	FPDTower& J = It->second;
	const int Old = J.JLevel;
	J.JXp += Amount;
	while (J.JLevel < JokerMaxLevel && J.JXp >= JokerXpNeed(J.JLevel)) { J.JXp -= JokerXpNeed(J.JLevel); J.JLevel++; }
	if (J.JLevel >= JokerMaxLevel) J.JXp = 0.f;
	FPDEvent E;
	E.Kind = FPDEvent::EKind::JokerXp;
	E.X = J.X + 0.5f;
	E.Y = J.Y + 0.5f;
	E.A = J.JLevel;
	E.B = J.JLevel - Old;
	E.R = Amount;
	Ev(E);
	if (E.B > 0)
	{
		bReapply = true; // 레벨 10 강화 등 효과는 다음 갱신 처음에 다시 (타워를 도는 중일 수 있어서)
		Sfx(EPDSound::Clear);
	}
	return E.B > 0;
}

void FPDGame::Hit(FPDEnemy& E, float Dmg, float Slow)
{
	if (E.bDead || E.Untargetable()) return; // 유령: 무적인 동안은 안 맞음
	const float Armor = E.Armor + (InWarlordAura(E) ? Boss::AuraArmor : 0.f);
	if (Has(EPDCharm::BossHunter) && EnemyDef(E.Type).bBoss) Dmg *= 1.4f; // 보스 사냥꾼 (D-234)
	if (Has(EPDCharm::OldWatch) && Phase == EPDPhase::Wave && NextBossWave() == Wave) Dmg *= 1.3f; // 낡은 회중시계 (D-239)
	if (Has(EPDCharm::FirstBeat) && Phase == EPDPhase::Wave && Time - WaveStartT < 10.f) Dmg *= 1.5f; // 첫 박자 (D-239)
	if (Armor > 0.f && !Has(EPDCharm::Pierce)) Dmg = std::max(Dmg * 0.25f, Dmg - Armor); // 방패: 한 방 피해를 깎는다 (최소 25%)
	if (E.Type == EPDEnemy::Counter && HitSrc >= 0) // 카드 카운터 (D-334): 맞은 무늬 내성이 쌓이고 다른 무늬는 풀린다
	{
		auto It = Towers.find(HitSrc);
		if (It != Towers.end() && !It->second.IsJoker() && !It->second.Card.IsWild())
		{
			const int S = (int)It->second.Card.Suit;
			if (S >= 0 && S < 4)
			{
				Dmg *= 1.f - E.Res[S];
				for (int K = 0; K < 4; ++K) E.Res[K] = K == S ? std::min(Boss::CounterMax, E.Res[K] + Boss::CounterGain) : std::max(0.f, E.Res[K] - Boss::CounterDecay);
			}
		}
	}
	if (E.Shield > 0.f) { const float A = std::min(E.Shield, Dmg); E.Shield -= A; Dmg -= A; } // 수호자 보호막 · 보호막 변이가 먼저 받는다
	E.Hp -= Dmg;
	if (Slow > 0.f) { E.SlowT = 1.2f; E.SlowAmt = std::max(E.SlowAmt, Slow); }
	if (E.Hp > 0.f) return;
	E.bDead = true;
	Kills++;
	if (BossBit(E.Type) >= 0) // 도감: 잡은 보스, 변이 셋 (D-337)
	{
		BossKilled |= 1u << BossBit(E.Type);
		int Mb = 0;
		for (int K = 0; K < 6; ++K) Mb += (E.Mut >> K) & 1;
		if (Mb >= 3) bMut3Kill = true;
	}
	if (HitSrc >= 0) // 조커가 처치하면 경험치 (D-125)
	{
		auto It = Towers.find(HitSrc);
		if (It != Towers.end() && It->second.IsJoker()) GainJokerXp(HitSrc, EnemyDef(E.Type).bBoss ? JokerBossXp : JokerKillXp);
		It = Towers.find(HitSrc); // 굴 잔치 (D-482): 바다코끼리와 목수가 처치하면 주변 연사가 쌓인다
		if (It != Towers.end() && JokerSpecial(It->second.Joker) == EPDUpperSpecial::Walrus && It->second.Feast < WalrusFeastMax)
		{
			It->second.Feast++;
			bReapply = true; // 효과는 다음 갱신 처음에 (타워를 도는 중일 수 있어서, 조커 레벨업과 같게)
		}
	}
	const FPDPoint P = Path.At(E.D);
	FPDEvent K;
	K.Kind = FPDEvent::EKind::Kill;
	K.X = P.X;
	K.Y = P.Y;
	K.Enemy = E.Type;
	K.A = AddXp(KillXp(E.Type, E.Wave) * E.XpK * (EnemyDef(E.Type).bBoss ? 1.f : WaveScale(E.Wave)) * (Has(EPDCharm::Bounty) ? 1.25f : 1.f)); // 마리 수가 늘어도 웨이브당 골드는 그대로, 짝 · 셋 보스는 나눠 받음
	if (E.Type == EPDEnemy::BigBlind && E.Pot > 0.f) { K.A += AddXp(E.Pot * 2.f, false); E.Pot = 0.f; } // 빅 블라인드: 20초 안에 잡으면 판돈 ×2
	Ev(K);
	if (E.Type == EPDEnemy::Twins && E.Link) // 쌍둥이: 남은 쪽이 회복 + 분노
		for (FPDEnemy& O : Enemies)
			if (O.Id == E.Link && !O.bDead) { O.Hp = std::min(O.MaxHp, O.Hp + O.MaxHp * Boss::TwinHeal); O.bRage = true; BossEvent(O); }
	for (FPDEnemy& O : Enemies) // 흡수자: 주변에서 적이 죽으면 회복
		if (&O != &E && !O.bDead && (O.Type == EPDEnemy::Devourer || (O.Type == EPDEnemy::House && O.Phase >= 2))) // 더 하우스 3단계도 흡수
		{
			const FPDPoint Q = Path.At(O.D);
			if (std::hypot(Q.X - P.X, Q.Y - P.Y) <= Boss::DevourRange) O.Hp = std::min(O.MaxHp, O.Hp + E.MaxHp * Boss::DevourHeal);
		}
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

// 쏘기: 즉시 발사체(ShotSpeed 0: 레이저·번개·로열·체셔)는 바로 맞히고, 나머지는 발사체를 띄운다
void FPDGame::Fire(const FPDTower& T, FPDEnemy& Target, float Dmg, float Splash, float Slow, int Crits)
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
	Pr.Src = CellKey(T.X, T.Y);
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
	const float Spd = ShotSpeed(Pr.Kind);
	Sh.Dmg = Spd <= 0.f ? Dmg : 0.f;
	Sh.R = (float)(int)Pr.Kind;
	Ev(Sh);
	if (Sounds.size() < 256) Sfx(EPDSound::Shot, (int)Pr.Suit, Pr.bHand ? 1 : 0); // 발사 소리 (D-155). 쌓이지 않게 (화면이 프레임마다 가져간다). 전기 소리는 뺐다 (D-171)
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
}

// 닿음: 목표(살아 있으면)에 피해, 범위 공격은 닿은 자리 둘레에
void FPDGame::Explode(const FPDProjectile& P, FPDEnemy* Target)
{
	FPDPoint At{P.TX, P.TY};
	const bool bHitTarget = Target && !Target->bDead;
	struct FSrcScope { int& R; int Old; ~FSrcScope() { R = Old; } } Scope{HitSrc, HitSrc};
	HitSrc = P.Src;
	if (bHitTarget)
	{
		At = Path.At(Target->D);
		Hit(*Target, P.Dmg, P.Slow);
		if (Sounds.size() < 256) Sfx(EPDSound::Hit, (int)P.Suit, P.Crits > 0 ? 1 : 0); // 타격 소리 (D-155)
	}
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

// ---------------------------------------------------------------- 보스 능력 (D-112)
bool FPDGame::InWarlordAura(const FPDEnemy& E) const
{
	if (E.Type == EPDEnemy::Warlord || (E.Mut & MutLead)) return false;
	const FPDPoint P = Path.At(E.D);
	for (const FPDEnemy& O : Enemies)
		if (!O.bDead && (O.Type == EPDEnemy::Warlord || (O.Mut & MutLead))) // 이끄는 변이도 군단장 오라
		{
			const FPDPoint Q = Path.At(O.D);
			if (std::hypot(Q.X - P.X, Q.Y - P.Y) <= Boss::AuraRange) return true;
		}
	return false;
}

bool FPDGame::Hexed(const FPDTower& T) const
{
	for (const FPDEnemy& O : Enemies)
		if (!O.bDead && O.Type == EPDEnemy::Hexer)
		{
			const FPDPoint Q = Path.At(O.D);
			if (std::hypot(Q.X - (T.X + 0.5f), Q.Y - (T.Y + 0.5f)) <= Boss::HexRange) return true;
		}
	return false;
}

float FPDGame::HexFactor(const FPDTower& T) const
{
	float K = 1.f;
	for (const FPDEnemy& O : Enemies)
	{
		if (O.bDead || (O.Type != EPDEnemy::Hexer && !(O.Mut & MutHex))) continue;
		const FPDPoint Q = Path.At(O.D);
		if (std::hypot(Q.X - (T.X + 0.5f), Q.Y - (T.Y + 0.5f)) <= Boss::HexRange) K = std::min(K, O.Type == EPDEnemy::Hexer ? Boss::HexDmg : Boss::MutHex);
	}
	return K;
}

bool FPDGame::HouseRuled(const FPDTower& T) const
{
	for (const FPDEnemy& O : Enemies)
	{
		if (O.bDead) continue;
		const float R = O.Type == EPDEnemy::HouseRule ? Boss::HouseRange : (O.Type == EPDEnemy::House && O.Phase >= 1) ? Boss::HouseRangeBig : 0.f;
		if (R <= 0.f) continue;
		const FPDPoint Q = Path.At(O.D);
		if (std::hypot(Q.X - (T.X + 0.5f), Q.Y - (T.Y + 0.5f)) <= R) return true;
	}
	return false;
}

void FPDGame::DealSuits(int N)
{
	std::vector<int> Cand;
	for (const auto& KV : Towers)
	{
		const FPDTower& T = KV.second;
		if (T.IsJoker() || T.Card.IsWild() || T.Level >= MaxLevel) continue; // 조커 · 와일드 · Lv4 는 안 바꾼다
		bool bAlready = false;
		for (const FPDSuitSwap& S : SuitSwaps) bAlready = bAlready || S.Cell == KV.first;
		if (!bAlready) Cand.push_back(KV.first);
	}
	for (int I = 0; I < N && !Cand.empty() && (int)SuitSwaps.size() < Boss::DealMax; ++I)
	{
		const int Pick = std::uniform_int_distribution<int>(0, (int)Cand.size() - 1)(Rng);
		const int Cell = Cand[Pick];
		Cand.erase(Cand.begin() + Pick);
		FPDTower& T = Towers.at(Cell);
		FPDSuitSwap Sw;
		Sw.Cell = Cell; Sw.Orig = T.Card.Suit; Sw.T = Boss::DealTime;
		T.Card.Suit = (EPDSuit)(((int)T.Card.Suit + 1 + std::uniform_int_distribution<int>(0, 2)(Rng)) % 4);
		SuitSwaps.push_back(Sw);
		bReapply = true;
	}
}

void FPDGame::RestoreSuits()
{
	for (const FPDSuitSwap& Sw : SuitSwaps)
	{
		auto It = Towers.find(Sw.Cell);
		if (It != Towers.end() && !It->second.IsJoker()) It->second.Card.Suit = Sw.Orig;
	}
	if (!SuitSwaps.empty()) { SuitSwaps.clear(); Synergies = ApplyEffects(Towers, CharmMask(), &Mods); }
	ChipPiles.clear();
}

void FPDGame::BossEvent(const FPDEnemy& E, float X2, float Y2)
{
	const FPDPoint P = Path.At(E.D);
	FPDEvent B;
	B.Kind = FPDEvent::EKind::BossSkill;
	B.Enemy = E.Type;
	B.X = P.X;
	B.Y = P.Y;
	B.X2 = X2;
	B.Y2 = Y2;
	Ev(B);
}

float FPDGame::BossTick(FPDEnemy& E, float Dt)
{
	if (!EnemyDef(E.Type).bBoss) return 1.f;
	E.AbilT += Dt;
	E.StateT = std::max(0.f, E.StateT - Dt);
	auto Every = [&](float Period) { if (E.AbilT < Period) return false; E.AbilT -= Period; return true; };
	switch (E.Type)
	{
	case EPDEnemy::Charger:
		if (Every(Boss::ChargeEvery)) { E.StateT = Boss::ChargeTime; BossEvent(E); }
		return E.StateT > 0.f ? Boss::ChargeSpeed : 1.f;
	case EPDEnemy::Summoner:
		if (Every(Boss::SummonEvery))
		{
			for (int I = 0; I < Boss::SummonCount; ++I)
			{
				FPDEnemy M = MakeEnemy(EPDEnemy::Mini, E.BaseHp * Boss::SummonHp, E.Wave);
				M.D = std::max(0.f, E.D - 0.3f - 0.25f * I);
				Born.push_back(M);
			}
			BossEvent(E);
		}
		return 1.f;
	case EPDEnemy::Phantom:
		if (Every(Boss::PhaseEvery)) { E.StateT = Boss::PhaseTime; BossEvent(E); }
		return 1.f;
	case EPDEnemy::Berserker:
		return 1.f + Boss::RageSpeed * (1.f - std::max(0.f, E.Hp / E.MaxHp));
	case EPDEnemy::Blinker:
		if (Every(Boss::BlinkEvery)) { BossEvent(E); E.D = std::min(Path.Length - 0.5f, E.D + Boss::BlinkDist); BossEvent(E); }
		return 1.f;
	case EPDEnemy::Warden:
		if (Every(Boss::WardEvery)) { E.StateT = Boss::WardTime; E.Shield = E.MaxHp * Boss::WardShield; BossEvent(E); }
		if (E.StateT <= 0.f) E.Shield = 0.f;
		return 1.f;
	case EPDEnemy::Breaker:
		if (Every(Boss::BreakEvery))
		{
			const FPDPoint P = Path.At(E.D);
			FPDTower* Best = nullptr;
			float BestD = Boss::BreakRange;
			for (auto& KV : Towers)
			{
				const float Dd = std::hypot(KV.second.X + 0.5f - P.X, KV.second.Y + 0.5f - P.Y);
				if (Dd <= BestD) { BestD = Dd; Best = &KV.second; }
			}
			if (Best) { Best->StunT = Boss::BreakStun; BossEvent(E, Best->X + 0.5f, Best->Y + 0.5f); }
		}
		return 1.f;
	case EPDEnemy::BigBlind: // 판돈 시간이 다 되면 올인 (D-334)
		if (E.Pot > 0.f && E.StateT <= 0.f) { E.Pot = 0.f; E.bRage = true; E.Armor += Boss::AllInArmor; BossEvent(E); }
		return E.bRage ? Boss::AllInSpeed : 1.f;
	case EPDEnemy::Dealer:
		if (Every(Boss::DealEvery)) { DealSuits(Boss::DealN); BossEvent(E); }
		return 1.f;
	case EPDEnemy::Twins:
		return E.bRage ? Boss::TwinRage : 1.f;
	case EPDEnemy::Bouncer:
	case EPDEnemy::HighRoller:
	case EPDEnemy::House:
	{
		const float Fr = std::max(0.f, E.Hp / std::max(1.f, E.MaxHp));
		const int Ph = Fr < 1.f / 3.f ? 2 : Fr < 2.f / 3.f ? 1 : 0;
		if (Ph > E.Phase) { E.Phase = Ph; E.AbilT = 0.f; E.StateT = 0.f; E.Borrow = -1; FPDEvent Pe; const FPDPoint P = Path.At(E.D); Pe.Kind = FPDEvent::EKind::BossSkill; Pe.Enemy = E.Type; Pe.X = P.X; Pe.Y = P.Y; Pe.A = 100 + Ph; Ev(Pe); } // 단계가 바뀜 (A = 100 + 단계, 화면 연출)
		if (E.Type == EPDEnemy::Bouncer)
		{
			if (E.Phase == 1) { if (Every(Boss::ChargeEvery)) { E.StateT = Boss::ChargeTime; BossEvent(E); } return E.StateT > 0.f ? Boss::ChargeSpeed : 1.f; }
			if (E.Phase == 2 && Every(Boss::BouncerStunEvery)) // 주변 타워 2초 기절 (D-335: 밀어내기 대신)
			{
				const FPDPoint P = Path.At(E.D);
				for (auto& KV : Towers) if (std::hypot(KV.second.X + 0.5f - P.X, KV.second.Y + 0.5f - P.Y) <= Boss::BouncerStunRange) KV.second.StunT = std::max(KV.second.StunT, Boss::BouncerStun);
				BossEvent(E);
			}
			return 1.f;
		}
		if (E.Type == EPDEnemy::HighRoller)
		{
			if (E.Phase == 0 && Every(Boss::PileEvery)) { ChipPiles.push_back({E.D, Boss::PileTime}); BossEvent(E); }
			if (E.Phase == 1 && Every(Boss::DealEvery)) { DealSuits(Boss::DealN); BossEvent(E); }
			if (E.Phase == 2)
			{
				if (Every(Boss::AllInShieldEvery)) { E.Shield = std::max(E.Shield, E.MaxHp * Boss::AllInShield); BossEvent(E); }
				return 1.f + Boss::AllInRage * (1.f - Fr);
			}
			return 1.f;
		}
		// 더 하우스: 1단계 남의 능력을 6초씩 빌림 (돌진 · 무적 · 기절), 2단계 연쇄 끄기 판 절반 (HouseRuled), 3단계 졸개 + 흡수 (Hit)
		if (E.Phase == 0)
		{
			if (Every(Boss::HouseCycle))
			{
				static const EPDEnemy Lend[3] = {EPDEnemy::Charger, EPDEnemy::Phantom, EPDEnemy::Breaker};
				E.Borrow = (int)Lend[(E.Borrow < 0 ? 0 : (E.Borrow == (int)EPDEnemy::Charger ? 1 : E.Borrow == (int)EPDEnemy::Phantom ? 2 : 0))];
				if (E.Borrow == (int)EPDEnemy::Charger) E.StateT = Boss::ChargeTime * 2.f;
				else if (E.Borrow == (int)EPDEnemy::Phantom) E.StateT = Boss::PhaseTime;
				else
				{
					const FPDPoint P = Path.At(E.D);
					FPDTower* Best = nullptr;
					float BestD = Boss::BreakRange;
					for (auto& KV : Towers) { const float Dd = std::hypot(KV.second.X + 0.5f - P.X, KV.second.Y + 0.5f - P.Y); if (Dd <= BestD) { BestD = Dd; Best = &KV.second; } }
					if (Best) Best->StunT = Boss::BreakStun;
				}
				BossEvent(E);
			}
			return E.Borrow == (int)EPDEnemy::Charger && E.StateT > 0.f ? Boss::ChargeSpeed : 1.f;
		}
		if (E.Phase == 2 && Every(Boss::HouseSummonEvery))
		{
			for (int I = 0; I < Boss::SummonCount; ++I)
			{
				FPDEnemy M = MakeEnemy(EPDEnemy::Mini, E.BaseHp * Boss::SummonHp, E.Wave);
				M.D = std::max(0.f, E.D - 0.3f - 0.25f * I);
				Born.push_back(M);
			}
			BossEvent(E);
		}
		return 1.f;
	}
	default:
		return 1.f;
	}
}

void FPDGame::Update(float Dt)
{
	Time += Dt;
	if (bReapply) { bReapply = false; Synergies = ApplyEffects(Towers, CharmMask(), &Mods); }
	for (auto& KV : Towers) KV.second.Age += Dt;
	KillSfxT -= Dt;
	if (Phase != EPDPhase::Wave) return;
	SpawnT -= Dt;
	if (!Pending.empty() && SpawnT <= 0.f)
	{
		FPDEnemy E = Pending.front();
		Pending.erase(Pending.begin());
		if (E.Type == EPDEnemy::BigBlind) // 빅 블라인드 (D-334): 나오면 판돈을 건다 (가진 경험치에서, 다음 레벨까지의 30% 까지)
		{
			const int PotI = std::min(Xp, (int)(XpToNext() * Boss::BlindPotK));
			Xp -= PotI;
			E.Pot = (float)PotI;
			E.StateT = Boss::BlindTime;
		}
		Enemies.push_back(E);
		if (BossBit(E.Type) >= 0) BossSeen |= 1u << BossBit(E.Type); // 도감: 만난 보스 (D-337)
		if (E.Type == EPDEnemy::BigBlind) BossEvent(E);
		SpawnT = EnemyDef(E.Type).bBoss ? 1.2f : 0.65f * WaveScale(Wave); // 마리 수가 늘어도 웨이브 길이는 그대로
	}
	// 딜러가 바꾼 무늬는 6초 뒤 되돌린다, 하이롤러 칩 더미는 6초 (D-334)
	for (FPDSuitSwap& Sw : SuitSwaps)
	{
		Sw.T -= Dt;
		if (Sw.T > 0.f) continue;
		auto It = Towers.find(Sw.Cell);
		if (It != Towers.end() && !It->second.IsJoker()) { It->second.Card.Suit = Sw.Orig; bReapply = true; }
	}
	SuitSwaps.erase(std::remove_if(SuitSwaps.begin(), SuitSwaps.end(), [](const FPDSuitSwap& S) { return S.T <= 0.f; }), SuitSwaps.end());
	for (FPDChipPile& Pl : ChipPiles) Pl.T -= Dt;
	ChipPiles.erase(std::remove_if(ChipPiles.begin(), ChipPiles.end(), [](const FPDChipPile& P) { return P.T <= 0.f; }), ChipPiles.end());
	for (FPDEnemy& E : Enemies)
	{
		E.SlowT = std::max(0.f, E.SlowT - Dt);
		const float Rg = EnemyDef(E.Type).Regen + ((E.Mut & MutRegen) ? Boss::MutRegen : 0.f); // 재생하는 변이
		if (Rg > 0.f && !E.bDead) E.Hp = std::min(E.MaxHp, E.Hp + E.MaxHp * Rg * Dt);
		float PileK = 1.f;
		for (const FPDChipPile& Pl : ChipPiles) if (std::fabs(Pl.D - E.D) <= Boss::PileRange) PileK = Boss::PileSpeed;
		const float SpeedK = E.bDead ? 1.f : BossTick(E, Dt) * (InWarlordAura(E) ? Boss::AuraSpeed : 1.f) * PileK;
		E.D += E.Speed * SpeedK * (E.SlowT > 0.f ? 1.f - E.SlowAmt : 1.f) * Dt;
		if (E.D >= Path.Length && !E.bDead)
		{
			E.bDead = true;
			const int Loss = EnemyDef(E.Type).bBoss ? 5 : 1;
			Lives -= Loss;
			Leaks++;
			FPDEvent L;
			L.Kind = FPDEvent::EKind::Leak;
			L.A = Loss;
			{ const FPDPoint Ex = Path.At(Path.Length); L.X = Ex.X; L.Y = Ex.Y; } // 빠져나간 자리 (예전엔 비어 있어 판 왼쪽 위에서 터졌다, D-180)
			Ev(L);
			Sfx(EPDSound::Denied);
		}
	}
	const FPDCharmBits Mask = CharmMask();
	const FPDTreeMods* Tm = &Mods;
	for (auto& KV : Towers)
	{
		FPDTower& T = KV.second;
		if (T.StunT > 0.f) { T.StunT -= Dt; continue; } // 파괴자에게 기절
		T.Cd -= Dt;
		if (T.Cd > 0.f) continue;
		FPDTowerStats St = TowerStats(T, Mask, Tm);
		St.Dmg *= HexFactor(T); // 저주술사 근처 · 저주하는 변이
		if (T.Eff.Chain >= 2 && HouseRuled(T)) // 하우스 룰 (D-334): 연쇄 배율을 뺀다 (족보 배율은 그대로)
		{
			const float Step = (Has(EPDCharm::ChainLink) ? 0.45f : ChainStep) + Mods.ChainAdd + (Mods.Key(EPDKeystone::Crowd) ? 0.15f : 0.f);
			if (!Mods.Key(EPDKeystone::Lone)) St.Dmg /= 1.f + Step * (T.Eff.Chain - 1);
		}
		const bool bUp = T.IsJoker() && Mods.Up(T);
		const float TX = T.X + 0.5f, TY = T.Y + 0.5f;
		const int JK = T.IsJoker() ? T.Joker : -1;
		const EPDUpperSpecial JSp = JokerSpecial(JK);
		if (JokerHas(JK, EPDJoker::Mask) && T.Eff.CopyKey >= 0 && Towers.count(T.Eff.CopyKey)) // 웃는 가면: 옆 타워가 더 세면 그대로 따라 함
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
		if (JokerHas(JK, EPDJoker::Bell)) // 방울: 8초마다 사거리 안 적을 1.5초 멈춘다
		{
			const bool bRabbit = JSp == EPDUpperSpecial::WhiteRabbit; // 흰 토끼: 10초마다 판 전체 1초
			const float BellR = bRabbit ? 99.f : St.Range;
			T.Timer += Dt;
			if (T.Timer >= (bRabbit ? 10.f : bUp ? 5.f : 8.f)) // 강화: 5초마다 2초
			{
				T.Timer = 0.f;
				for (FPDEnemy& E : Enemies)
				{
					const FPDPoint P = Path.At(E.D);
					if (!E.bDead && std::hypot(P.X - TX, P.Y - TY) <= BellR) { E.SlowT = bRabbit ? 1.f : bUp ? 2.f : 1.5f; E.SlowAmt = 1.f; }
				}
				FPDEvent B;
				B.Kind = FPDEvent::EKind::Bell;
				B.X = TX;
				B.Y = TY;
				B.R = std::min(BellR, 30.f);
				Ev(B);
			}
		}
		if (JokerHas(JK, EPDJoker::Star)) // 스타: 판 전체 레이저, 모든 적에게
		{
			bool bAny = false;
			float SD = St.Dmg * (1.f + RollCrit() * (St.CritMult - 1.f));
			const bool bMirror = JSp == EPDUpperSpecial::Mirror; // 거울 나라: 무늬 차례 ♥ 위력 ×2, ♣ 판 전체 둔화
			const int MirrorSuit = T.Cycle % 4;
			if (bMirror && MirrorSuit == (int)EPDSuit::Heart) SD *= 2.f;
			HitSrc = CellKey(T.X, T.Y);
			for (FPDEnemy& E : Enemies) if (!E.bDead) { Hit(E, SD, 0.f); if (bMirror && MirrorSuit == (int)EPDSuit::Club) { E.SlowT = std::max(E.SlowT, 1.5f); E.SlowAmt = std::max(E.SlowAmt, 0.35f); } bAny = true; }
			if (bMirror) T.Cycle++;
			HitSrc = -1;
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
			if (E.bDead || E.Untargetable()) continue;
			const FPDPoint P = Path.At(E.D);
			if (std::hypot(P.X - TX, P.Y - TY) > St.Range) continue;
			const bool bDoomed = E.Incoming >= E.Hp;
			if (JSp == EPDUpperSpecial::Jabberwock && Target && EnemyDef(Target->Type).bBoss != EnemyDef(E.Type).bBoss) { if (EnemyDef(E.Type).bBoss) { Target = &E; bTargetDoomed = bDoomed; } continue; } // 재버워키: 보스 먼저
			if (!Target || (bTargetDoomed && !bDoomed) || (bDoomed == bTargetDoomed && E.D > Target->D)) { Target = &E; bTargetDoomed = bDoomed; }
		}
		if (!Target) continue;
		if (JSp == EPDUpperSpecial::Jabberwock && EnemyDef(Target->Type).bBoss) St.Dmg *= 1.5f;
		T.Cd = 1.f / St.Rate;
		const int Crits = RollCrit();
		St.Dmg *= 1.f + Crits * (St.CritMult - 1.f);
		if (JokerHas(JK, EPDJoker::Clown)) T.Cycle++;
		if (JSp == EPDUpperSpecial::QueenHearts && !EnemyDef(Target->Type).bBoss && Target->Hp - St.Dmg <= Target->MaxHp * 0.15f) St.Dmg = std::max(St.Dmg, Target->Hp + 1.f); // 하트 여왕: "목을 쳐라"
		FPDEnemy* Shot2[2] = {nullptr, nullptr};
		for (int Extra = 0; JokerHas(JK, EPDJoker::Comedy) && Extra < (bUp ? 2 : 1); ++Extra) // 희비극: 둘째 적도 함께 공격 (강화: 셋째까지)
		{
			FPDEnemy* Second = nullptr;
			for (FPDEnemy& E : Enemies)
			{
				if (E.bDead || E.Untargetable() || &E == Target || &E == Shot2[0]) continue;
				const FPDPoint P = Path.At(E.D);
				if (std::hypot(P.X - TX, P.Y - TY) <= St.Range && (!Second || E.D > Second->D)) Second = &E;
			}
			if (Second)
			{
				Shot2[Extra] = Second;
				if (JSp == EPDUpperSpecial::Tweedle) // 트위들덤과 트위들디: 둘째 적은 다음 무늬 공격으로
				{
					FPDTower T2 = T;
					T2.Cycle = T.Cycle + 1 + Extra;
					const FPDTowerStats S2 = TowerStats(T2, Mask, Tm);
					Fire(T, *Second, S2.Dmg * std::max(1.f, S2.Rate * Dt) * (1.f + Crits * (S2.CritMult - 1.f)), S2.Splash, S2.Slow, Crits);
				}
				else Fire(T, *Second, St.Dmg, 0.f, St.Slow, Crits);
			}
		}
		Fire(T, *Target, St.Dmg, St.Splash, St.Slow, Crits);
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
