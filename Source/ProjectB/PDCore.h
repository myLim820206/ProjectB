// 포커 디펜스 규칙 코어
// 언리얼 의존성이 없는 순수 C++17. 언리얼 모듈 안에서 그대로 컴파일되고,
// Tests/core_test.cpp 가 이 파일과 PDSynth.cpp 만 컴파일해 단위 시험과 봇 시험을 돌린다 (Tests/run.sh).
// 처음 규칙은 웹 프로토타입(DOC/archive/prototype/poker-defense/index.html, v1.2)에서 옮겼고, 그 뒤 기능은 여기에만 있다.
// 좌표는 모두 칸 단위 (칸 중심 = x + 0.5). 화면 픽셀 변환은 위젯이 한다.
#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <functional>
#include <map>
#include <string>
#include <random>
#include <vector>

#include "PDSynth.h"

namespace PD
{
constexpr int Cols = 16;
constexpr int Rows = 10;
constexpr int HandMaxBase = 7;
constexpr int HandMaxCap = 10; // D-397: 칵테일 · 스킬 덤을 다 더해도 손패는 10장까지 (예전 최대 11)
constexpr int CharmMax = 5;
constexpr int NumWaves = 15; // 1회차 (보스 3종이 한 바퀴). 진행은 무한이고 목숨이 0이 되면 끝. 이 뒤로는 적 체력이 더 천천히 오른다
constexpr int StartLives = 20;
constexpr int StartHand = 5;
constexpr int DrawPerWave = 5; // 웨이브를 막으면 받는 카드 (D-216, 예전 3)
constexpr float AutoStartDelay = 60.f; // D-185: 모든 웨이브 1분. 웨이브를 막고 보상을 고르면 이 시간(실제 초) 뒤 다음 웨이브가 저절로 시작 (첫 웨이브는 직접)
// 초반 완화 (D-106): EasyWaves 웨이브까지는 적 체력을 낮추고, 다음 웨이브까지 덱을 짤 시간을 더 준다
constexpr int EasyWaves = 5;
constexpr float EasyAutoStartDelay = 60.f; // D-185: 초반도 똑같이 1분 (예전 12초, 그 뒤 5초)
inline float AutoStartDelayFor(int NextWave) { return NextWave <= EasyWaves ? EasyAutoStartDelay : AutoStartDelay; }
inline float EarlyHpK(int W) { return W <= EasyWaves ? 0.55f + 0.09f * (float)(W < 1 ? 0 : W - 1) : 1.f; } // 1웨이브 ×0.55 → 5웨이브 ×0.91
constexpr float ChainStep = 0.25f; // 연쇄: 맞닿은 족보 하나마다 공격력 +25% (연결고리 부적이면 +45%)

enum class EPDSuit : uint8_t { Spade, Heart, Diamond, Club, Wild }; // Wild = 와일드 카드 (아무 숫자·무늬)
struct FPDCard
{
	int Rank = 2; // 2~14 (J Q K A = 11 12 13 14)
	EPDSuit Suit = EPDSuit::Spade;
	bool IsWild() const { return Suit == EPDSuit::Wild; }
	int Power() const { return IsWild() ? 10 : Rank; } // 공격력 계산용 숫자
	bool operator==(const FPDCard& O) const { return Rank == O.Rank && Suit == O.Suit; }
};

// 무늬 = 공격 방식
struct FPDSuitDef
{
	float Dmg, Rate, Range, Splash, Slow;
};
const FPDSuitDef& SuitDef(EPDSuit S);

// 부적: 웨이브 보상, 최대 5칸, 판 끝까지 규칙을 바꾼다. 비트 묶음(uint32)으로 넘긴다
enum class EPDCharm : uint8_t
{
	HeartLover, SpadeLover, DiaLover, ClubLover, // 그 무늬 타워 공격력 +40%
	ChainLink,  // 연쇄 +25% → +45%
	Twins,      // 원 페어·투 페어 ×1.5 → ×2
	LineLover,  // 스트레이트 연사 한 장마다 +0.25 더
	WideFlush,  // 플러시 범위 +0.75
	FaceCards,  // J·Q·K 공격력 +60%
	SmallNums,  // 2~6 연사 +60%
	Lucky7,     // 7 공격력 ×2.5
	BigHand,    // 손패 최대 +2
	Supply,     // 웨이브마다 카드 +1
	Recycle,    // 버리면 1장 뽑기
	Scope,      // 사거리 +15%
	Frost,      // 둔화 강화, ♣ 주변도 둔화
	ChainBlast, // 처치 시 주변 적에게 최대 체력 15% 피해
	Pierce,     // 방패 무시
	// 부적 상자 (D-233): 처음엔 잠겨 있고, 영구 강화 창에서 칩으로 상자를 사면 열린다
	Aces,         // A 공격력 ×3
	EvenNums,     // 짝수 카드 공격력 +40%
	OddNums,      // 홀수 카드 연사 +30%
	FullHouseFan, // 풀하우스 위력 ×1.5
	FourLeaf,     // 포카드 위력 ×1.5
	RoyalGuard,   // 로열·스트레이트 플러시 위력 ×1.5
	GlassCannon,  // 모든 타워 공격력 +30%, 고를 때 목숨 −3
	LongShot,     // ♠ 사거리 +30%
	BigBang,      // ♥ 폭발 범위 +0.5칸
	Overclock,    // ♦ 연사 +30%
	JokerFan,     // 조커 위력 +30%
	Feeder,       // 먹이기 경험치 +50%
	Forge,        // 합성 타워 위력 +30%
	Bounty,       // 처치 경험치 +25%
	// 부적 상자 IV (D-234)
	QueenGuard,   // Q 연사 +80%
	KingsReach,   // K 사거리 +40%, 공격력 +30%
	ChainFever,   // 연쇄 3 이상에 든 타워 공격력 +30%
	LongHand,     // 5장 족보 타워 공격력 +35%
	Triad,        // 3장 족보 타워 공격력 +40%
	Loner,        // 족보에 안 든 타워 공격력 +60%
	WildHeart,    // 와일드 카드 타워 공격력 ×2
	Herald,       // 레벨업할 때마다 목숨 +1
	BossHunter,   // 보스에게 주는 피해 +40%
	Sweeper,      // 웨이브 통과 경험치 ×2
	// 칵테일 60종 (D-239): 잠금 해제 메뉴에서 하나씩
	JackGuard,    // J 타워 공격력 +80%
	TwoRebel,     // 숫자 2 타워 공격력 ×3
	Rainbow,      // 판에 무늬 4종이 다 있으면 모든 타워 공격력 +25%
	StraightFlushFan, // 스트레이트 플러시·로열 위력 +60%
	TwoPairFan,   // 투 페어 위력 +60%
	TripleFan,    // 트리플 공격 ×2 → ×2.6
	FourFan,      // 포카드 위력 +50%
	TwoCards,     // 2장 족보 타워 공격력 +60%
	FourCards,    // 4장 족보 타워 공격력 +40%
	QuickFuse,    // 합성 타워 연사 +30%
	BigJoker,     // 조커 사거리 +30%
	SniperScope,  // ♠ 사거리 +50%
	IceCrystal,   // ♣ 둔화 +15%p
	Rapid,        // ♦ 연사 +40%
	OldWatch,     // 보스 웨이브 동안 모든 피해 +30%
	FirstBeat,    // 웨이브 시작 10초 동안 모든 피해 +50%
	FoodChain,    // 먹이기 흡수 60% → 90%
	Dexterity,    // 손패 최대 +1, 웨이브마다 카드 +1
	Count
};
constexpr int CharmBaseCount = 18;                            // 처음부터 열린 부적
using FPDCharmBits = uint64_t; // 부적 비트 (D-233: 32개를 넘기려고 64비트)
constexpr FPDCharmBits CharmBaseMask = (FPDCharmBits(1) << CharmBaseCount) - 1u;
// 부적 상자 3개 (D-233): 상자마다 열리는 부적 범위와 칩 값
// 잠금 해제 (D-239): 칵테일·조커를 하나씩 칩으로 연다 (예전 칵테일 상자 D-233 은 없앴다)
inline int CharmPrice(int J) { return J < 18 ? 0 : J < 23 ? 60 : J < 28 ? 120 : J < 32 ? 180 : J < 42 ? 250 : 200; } // 마지막 18종 320 → 200 (D-320)
inline bool HasCharm(FPDCharmBits Mask, EPDCharm J) { return (Mask >> (int)J) & 1u; }
// 보상 창은 CharmEvery 웨이브마다만 뜬다 (D-101). 그 사이 웨이브는 창 없이 AutoStartDelay 뒤 다음 웨이브
constexpr int CharmEvery = 5;
// 즉시 보상 (부적 칸을 쓰지 않음): 부적 창에서 와일드가 셋째 칸에 들거나, 부적 칸이 가득일 때 채움
enum class EPDInstant : uint8_t { Wild, Draw2, Repair, Xp, Swap, Count }; // 경험치 = 10 + 웨이브 × 4 (D-228)
constexpr float WildChance = 0.5f; // 부적 창마다 와일드가 셋째 칸에 들 확률
constexpr float SwapChance = 0.4f; // 와일드가 없을 때 자리바꿈이 셋째 칸에 들 확률 (D-149)
constexpr int SwapPerReward = 2;    // 자리바꿈 보상 한 장 = 2번
constexpr int StartSwaps = 2;       // 새 판은 자리바꿈 2번으로 시작 (D-489): 조커를 판 구석으로 옮기라고
struct FPDReward
{
	bool bCharm = true;
	int Id = 0; // EPDCharm 또는 EPDInstant
};

// 족보 = 시너지
// 로열 스트레이트 플러시(같은 무늬 10 J Q K A)와 파이브 카드(같은 숫자 5장: 와일드·조커나 다시 채운 덱의 같은 카드가 있어야 나옴)를 더해 10가지
enum class EPDHand : uint8_t { RoyalFlush, Five, StraightFlush, Four, FullHouse, Flush, Straight, Triple, TwoPair, Pair, Count };
int HandBaseValue(EPDHand H);
int HandValue(EPDHand H, int N); // 플러시·스트레이트는 장수가 많을수록 큼
// 카드 묶음(순서 무관)이 이루는 족보 전부
std::vector<EPDHand> ClassifyHand(const std::vector<FPDCard>& Cards);

struct FPDEffects
{
	float Dmg = 1.f, Rate = 1.f, Range = 1.f, Splash = 0.f;
	int Hand = -1;   // 들어간 족보 번호 (Synergies 안), 없으면 -1
	int Chain = 1;
	int CopyKey = -1; // 웃는 가면 조커: 따라 할 옆 타워 칸
	int Kind = -1;    // 들어간 족보 종류 (EPDHand), 없으면 -1
	int N = 0;        // 들어간 족보 장수
	uint32_t Kinds = 0; // 들어간 족보 종류 전부 (1 << EPDHand). 카드 한 장이 여러 족보에 들 수 있다
};


// ---------------------------------------------------------------- 스킬 트리 (판 안에서만, DOC/design/16-skill-tree.md)
// 적을 잡아 번 골드로 포인트를 사서 노드를 찍는다. 이미 찍은 노드와 이어진 노드만 살 수 있고,
// 포인트 값은 살 때마다 ×1.15 로 올라 무한 진행에서도 트리를 다 채우지 못한다.
// 키스톤은 둘씩 짝이라 하나를 찍으면 짝은 그 판 동안 봉인되고, 판마다 최대 3개.
enum class EPDNode : uint8_t { Start, Small, Notable, Keystone, Joker };
enum class EPDStat : uint8_t { None, SuitDmg, SuitRate, SuitRange, SuitSplash, SuitSlow, HandDmg, HandRate, HandSplash, AllDmg, ChainAdd, FuseAdd, FusedDmg, XpPct, HandMax, Draw, Lives, Keystone, Joker, JokerDmg, JokerAdd, AllRate, CritChance, CritMult, UpperPrep };
// 키스톤: (외골수, 잡식가) (큰 판, 작은 판) (대장장이, 광대의 길) (고독한 저격수, 군중) (왕관의 무게, 서커스, D-232) 이 짝. 짝 번호 = K ^ 1
enum class EPDKeystone : uint8_t { Purist, Omni, BigTable, SmallTable, Smith, ClownPath, Lone, Crowd, Crown, Circus, Count };
constexpr int KeystoneMax = 3;
constexpr int PointBasePrice = 20;
constexpr float PointPriceGrowth = 1.22f;
struct FPDNodeFx
{
	EPDStat Stat = EPDStat::None;
	int Param = 0;   // 무늬 번호, 족보 비트 묶음(1 << EPDHand), 키스톤·조커 번호
	float Value = 0.f;
};
struct FPDTreeNode
{
	EPDNode Kind = EPDNode::Small;
	uint8_t Theme = 0;    // 색 묶음: 0~3 무늬, 4 족보, 5 살림, 6 조커, 7 시작
	float X = 0.f, Y = 0.f; // 트리 창(1280x720) 위치
	FPDNodeFx Fx[2];
};
// 노드 표는 tools/tree/gen_tree.py 가 만든 PDTreeData.inl
constexpr int TreeMaxNodes = 160;
using FPDTreeBits = std::bitset<TreeMaxNodes>;
int TreeNodeCount();
const FPDTreeNode& TreeNode(int Id);
const std::vector<int>& TreeLinks(int Id);
// 찍은 노드들을 모은 효과 (increased 식 덧셈, 키스톤·조커 강화만 곱)
struct FPDTower;
struct FPDTreeMods
{
	float SuitDmg[5] = {}, SuitRate[5] = {}, SuitRange[5] = {}, SuitSplash[5] = {}, SuitSlow[5] = {};
	float HandDmg[(int)EPDHand::Count] = {}, HandRate[(int)EPDHand::Count] = {}, HandSplash[(int)EPDHand::Count] = {};
	float AllDmg = 0.f, ChainAdd = 0.f, FuseAdd = 0.f, FusedDmg = 0.f, XpPct = 0.f, JokerDmg = 0.f, JokerAdd = 0.f;
	float AllRate = 0.f, CritChance = 0.f, CritMult = 0.f; // 공격 속도, 치명타 확률(+%p), 치명타 배율(+)
	// 골드 강화 (트리와 따로 곱한다)
	float UpgDmg = 0.f, UpgRate = 0.f, UpgSuit[4] = {}, UpgCrit = 0.f, UpgCritMult = 0.f;
	float UpperPrep = 0.f; // 다과회 초대장 (D-232): 상위 조커 위력 +, 조건 레벨 7
	int HandAdd = 0, DrawAdd = 0;
	uint32_t Keys = 0;    // 찍은 키스톤 (비트)
	uint32_t JokerUp = 0; // 강화한 조커 (비트)
	bool Key(EPDKeystone K) const { return (Keys >> (int)K) & 1u; }
	bool Up(int Joker) const { return Joker >= 0 && Joker < 10 && ((JokerUp >> Joker) & 1u); } // 상위 조커 번호는 트리 강화 없음
	bool Up(const FPDTower& T) const; // 트리 강화 또는 조커 레벨 10 이상
};
FPDTreeMods BuildTreeMods(const FPDTreeBits& Owned);

// ---------------------------------------------------------------- 강화 (D-228: 판 안 골드 강화 → 판 밖 영구 강화로 옮기는 중)
enum class EPDUpgrade : uint8_t { Attack, Speed, Crit, CritMult, Spade, Heart, Diamond, Club, Count };
constexpr int UpgradeMax = 20; // 영구 강화 레벨 상한 (D-230)
constexpr int UpgradeMaxCleared = 9999; // 100 웨이브 모드를 클리어하면 (D-238): 무한 모드용
constexpr float BaseCrit = 0.05f, BaseCritMult = 1.5f; // 기본 치명타 5%, ×1.5. 100%를 넘으면 한 방에 치명타가 여러 번
int UpgradeCost(int Level);           // Level → Level+1 칩 값 (D-230: 5 + 5 × 레벨, 21부터 105 + 2 × (레벨 − 20), D-238)
float UpgradeStep(EPDUpgrade U);      // 레벨당 효과
void ApplyUpgrades(FPDTreeMods& M, const std::array<int, (int)EPDUpgrade::Count>& Lv);

struct FPDTower
{
	FPDCard Card;
	int X = 0, Y = 0;
	float Cd = 0.f;
	float Age = 0.f; // 설치 후 시간 (연출용)
	FPDEffects Eff;
	// 합성: 족보를 합치면 레벨이 오르고, 합친 타워들의 위력과 족보 효과(특성)를 물려받는다
	int Level = 1;            // 1~4. 같은 레벨끼리만 족보가 된다
	float Mult = 1.f;         // 위력 배율 (합친 타워들 배율 합 × FuseBonus)
	float TDmg = 1.f, TRate = 1.f, TRange = 1.f, TSplash = 0.f; // 물려받은 특성
	// 조커 포탑 (-1 = 일반 타워). 최종형: 더 합성되지 않고, 맞닿은 족보에서는 와일드(레벨 무관)
	int FuseHand = -1;       // 합성 타워가 기억하는 족보 (합친 족보 중 가장 높은 것). 조커 재료가 될 때 이 족보로 친다
	int Joker = -1;
	EPDSuit JSuit = EPDSuit::Spade; // 할리퀸: 강화할 무늬 (재료에 가장 많던 무늬)
	float Timer = 0.f;              // 방울: 종 주기
	int Cycle = 0;                  // 광대: 공격 무늬 순환
	int JLevel = 1;                 // 조커 레벨 (족보 먹이기·처치 경험치로 오름, 최대 JokerMaxLevel)
	float StunT = 0.f;              // 파괴자 보스에게 기절한 남은 시간 (D-112)
	float JXp = 0.f;                // 다음 레벨까지 쌓인 경험치
	bool IsJoker() const { return Joker >= 0; }
};
// 조커 레벨 (D-098): 족보를 먹이면 판 위 모든 조커가 경험치 (D-494). 레벨마다 위력 +15% (D-191), 레벨 10부터 강화 능력 (트리 조커 강화와 같음)
constexpr int JokerMaxLevel = 9999;
constexpr float JokerLevelDmg = 0.5f; // 레벨마다 위력 +50% (D-196, 예전 15%)
constexpr float JokerFeedAbsorb = 0.6f; // 먹이기: 먹인 타워 위력의 60% 를 조커마다 위력에 바로 더한다 (D-191, D-494)
constexpr int JokerUpLevel = 10;
inline float JokerXpNeed(int Level) { return 20.f + 10.f * (Level - 1); }
constexpr float JokerKillXp = 2.f / 3.f, JokerBossXp = 10.f; // 조커가 적을 처치하면 판 위 모든 조커가 각자 받는 경험치 (D-125, D-497: 예전 1 · 15 를 2/3 로)
struct FPDFeed
{
	int Syn = -1;   // 먹일 족보 번호
	int Joker = -1; // 대표 조커 칸 (레벨 가장 높은 것: 툴팁 · 연출 기준점). 먹는 건 판 위 모든 조커 (D-494)
	int Jokers = 0; // 먹는 조커 수 (판 위 조커 전부)
	float Xp = 0.f; // 조커 한 마리가 얻는 경험치 (조커마다 같은 양, 먹보 부적 전)
	float Gain = 0.f; // 대표 조커 위력 배율(Mult)에 더해지는 양 (D-191). 조커마다 자기 단위로 따로 계산
};
// 조커 10종 (그림 번호 1~10 순서: DOC/art/15-art-style.md 9절)
enum class EPDJoker : uint8_t { Clown, Hatter, Mask, Comedy, Harlequin, Baton, Jack, Cheshire, Bell, Star, Count };
// 조커 잠금 (D-239): 처음엔 희비극·웃는 가면·잭·지휘봉만. 나머지는 잠금 해제 메뉴에서 칩으로
constexpr uint32_t JokerStarterMask = (1u << 3) | (1u << 2) | (1u << 6) | (1u << 5);
constexpr int ChipMult = 2; // 판당 칩 배율 (D-323: 다 여는 데 약 60시간)
constexpr uint32_t UpperStarterMask = (1u << 11) | (1u << 18); // 상위 조커 맛보기 2종: 험프티 덤프티 · 하얀 기사, 처음부터 열림 (D-320)
inline int JokerPrice(int J) { static const int P[10] = {350, 350, 0, 0, 150, 0, 0, 250, 200, 500}; return J >= 0 && J < 10 ? P[J] : 0; }
constexpr float JokerBonus = 10.0f; // 조커 합성: 재료 위력 합 × 10 (D-492, 예전 3.0 D-491 · 1.2: 만능 제거로 사라진 족보 효과를 덤으로 되돌림)
constexpr float JokerCardBonus = 0.1f; // 재료 카드 한 장마다 조커 위력 +10% (4장 ×1.4, 10장 ×2) (D-114, D-118)
constexpr float JokerMergeBonus = 1.3f; // 같은 조커끼리 합치기: 위력 합 × 1.3 (D-114)
// 같은 조커끼리 맞닿은 묶음 (D-114). 합치면 한 칸에 하나로
struct FPDJokerMerge
{
	int Joker = -1;
	std::vector<int> Cells;
	int Upper = -1; // 상위 조커 합성 (D-231): 서로 다른 두 조커 → 상위 조커 번호 (0~29). -1 = 같은 조커 합치기
};
// ---------------------------------------------------------------- 상위 조커 (D-231, DOC/design/18)
// 서로 다른 기본 조커 둘(맞닿음, 둘 다 조커 레벨 10 이상, 트리 끝자락을 찍은 조커는 5)을 합친 최종형. 타워의 Joker 값 10~39.
// 능력 = 두 재료 조커의 능력 (강화된 상태) + 대표 조커 몇은 고유 능력. 위력 = 재료 합 × 1.5 (트리 끝자락 재료 하나마다 +25%)
constexpr int UpperBase = 10, UpperJokerCount = 30;
constexpr int UpperNeedLevel = 10, UpperNeedLevelTip = 5;
constexpr float UpperBonus = 1.5f, UpperBonusPlain = 1.3f, UpperTipBonus = 0.25f; // ★ 대표 11종 ×1.5, 일반 19종 ×1.3 (D-333)
enum class EPDUpperSpecial : uint8_t { None, MadTea, WhiteRabbit, GrinOnly, QueenHearts, Dormouse, Jabberwock, Tweedle, Mirror, RedQueen, MockTurtle };
struct FPDUpperDef { EPDJoker A, B; EPDUpperSpecial Special; };
const FPDUpperDef& UpperDef(int U);          // U = 0~29
int UpperPrice(int U);                       // 잠금 해제 값 (D-241 → D-320): (300 + 재료 둘 값의 평균) / 2, 50 단위 → 150~400. 재료 조커가 둘 다 열려 있어야 살 수 있다
int UpperOf(EPDJoker A, EPDJoker B);         // 레시피가 있으면 상위 조커 번호, 없으면 -1 (순서 무관)
inline bool IsUpperJoker(int Joker) { return Joker >= UpperBase && Joker < UpperBase + UpperJokerCount; }
bool JokerHas(int Joker, EPDJoker K);        // 그 조커이거나, 그 조커를 재료로 한 상위 조커
EPDUpperSpecial JokerSpecial(int Joker);
// 조커 재료 묶음 (D-487): 족보·합성 타워(레벨 2+) 2~4개가 한 조커의 레시피와 정확히 맞을 때.
// 판 어디에 있어도 된다 (D-195, 예전엔 서로 맞닿아야 했다). 재료가 많은 레시피를 먼저, 같은 개수끼리는 족보 기본값 합이 낮은 것부터
struct FPDJokerGroup
{
	std::vector<int> SynergyIdx; // 족보 번호
	std::vector<int> FusedCells; // 따로 선 합성 타워 칸
	std::vector<int> Cells;      // 쓰이는 칸 전부
	EPDJoker Result = EPDJoker::Clown;
};
// 조커 재료 하나 (D-195): 조커가 없는 족보 하나, 또는 합성 타워(레벨 2+, 기억한 족보로) 하나. 합성 타워는 족보에 들어 있어도 따로 재료가 된다 (D-220)
struct FPDJokerNode
{
	int Syn = -1;            // 족보 번호 (-1 이면 합성 타워)
	int Fused = -1;          // 합성 타워 칸
	std::vector<int> Cells;  // 재료 칸
	EPDHand Kind = EPDHand::Count; // 족보 (합성 타워는 기억하는 족보)
};
// 대장: 족보마다 조커 하나 (지휘 능력과 도감 표시). 조커마다 레시피에 자기 대장 족보가 들어 있다
EPDJoker CaptainOf(EPDHand H);
EPDHand CaptainHand(EPDJoker J);
// 레시피 (D-487, 예전 D-115 · D-118): 조커마다 레시피 하나 (순서 무관). 재료 수는 대장 족보의 뽑기 난이도로 정한다:
// 흔한 대장(원 페어 · 플러시 · 스트레이트 · 투 페어) 4개, 중간(트리플 · 풀하우스) 3개, 드문 대장 2개. 짝 재료는 흔한 족보만, 스트레이트는 레시피에 최대 하나
constexpr int JokerRecipeMax = 4;
const std::vector<EPDHand>& JokerRecipe(EPDJoker J);
// 조커 등급 배율 (D-487, 예전 D-116): 재료 수로 정한다. 4개 ×1.6, 3개 ×2.5, 2개 ×3.5.
// 대장 족보가 판 위에서 아주 드문 둘은 따로 (D-490): 모자 장수(스트레이트 플러시) ×5, 스타(로열) ×6
constexpr float HatterTier = 5.f, StarTier = 6.f;
float JokerTier(EPDJoker J);
// 행운의 한 장 (D-490): 판에 같은 무늬 · 같은 레벨로 숫자가 이어진 줄(3 ~ 4장, 스트레이트 모양 D-108)을 지켜 두면
// 웨이브를 막고 받는 카드 중 한 장이 확률로 그 줄 끝을 늘리는 카드로 온다 (덱에 남은 카드만, 웨이브에 한 번)
constexpr float LuckyP4 = 0.4f, LuckyP3 = 0.15f;
struct FPDLucky
{
	std::vector<int> Line;          // 줄 칸 (숫자 오름차순)
	int Len = 0;                    // 0 = 줄 없음
	int NWant = 0;                  // 기다리는 카드 수 (끝 둘까지)
	FPDCard Want[2];                // 줄 끝을 늘리는 카드
	int Cell[2] = {-1, -1};         // 그 카드를 놓을 빈 칸 (끝에 맞닿은 칸)
	float P() const { return NWant == 0 ? 0.f : Len >= 4 ? LuckyP4 : LuckyP3; }
};
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands); // 레시피와 정확히 맞는 조커 (족보 묶음이 같아야, 개수도), 없으면 EPDJoker::Count
constexpr float CaptainCommand = 0.3f; // 지휘: 판 위 자기 족보 타워 위력 +30%, 대장이 여럿이면 겹친다 (+60%, +90% …)
constexpr int MaxLevel = 4;
constexpr float FuseBonus = 1.5f;      // 합성 덤: 합친 위력 × 1.5
constexpr float LevelRange = 0.1f;     // 레벨마다 사거리 +10%
inline int CellKey(int X, int Y) { return Y * Cols + X; }
// 합성 결과가 놓일 기본 칸: 재료 칸들 가운데에 가장 가까운 칸 (같으면 먼저 나온 칸)
int DefaultFuseCell(const std::vector<int>& Cells);
using FPDTowerMap = std::map<int, FPDTower>;
// 판에서 가장 긴 줄 (같으면 숫자가 높은 줄). Open(칸) = 그 칸이 비어 있고 지을 수 있음. 덱은 보지 않는다 (봇 어림용)
FPDLucky FindLuckyLine(const FPDTowerMap& Towers, const std::function<bool(int)>& Open);

struct FPDSynergy
{
	EPDHand Kind = EPDHand::Pair;
	int N = 2;
	int Value = 0;
	std::vector<int> Cells;   // 칸 키
	int Chain = 1;            // 이 족보가 든 연쇄의 족보 수
	int ChainId = -1;
	std::vector<int> Links;   // 맞닿은 다른 족보 번호
};
// 상하좌우로 붙은 타워 덩어리 안의 2~5장 연결 묶음을 (스트레이트 · 플러시는 3장부터, D-418) 모두 따져 큰 족보부터 고르고(카드 공유 가능, 새 카드 한 장 이상, 같은 종류끼리는 공유 안 함), 연쇄를 묶는다
std::vector<FPDSynergy> ComputeSynergies(const FPDTowerMap& Towers);
// 족보 효과를 타워에 반영하고 족보 목록을 돌려준다
std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers, FPDCharmBits Charms = 0, const FPDTreeMods* Tree = nullptr);

struct FPDTowerStats
{
	float Dmg, Rate, Range, Splash, Slow;
	float Crit = BaseCrit, CritMult = BaseCritMult; // 치명타 확률 (1 넘으면 여러 번), 배율
	float CritFactor() const { return 1.f + Crit * (CritMult - 1.f); } // 기대 배율
};
FPDTowerStats TowerStats(const FPDTower& T, FPDCharmBits Charms = 0, const FPDTreeMods* Tree = nullptr);
// 타워 하나의 어림 위력: 피해 × 연사 × 치명타 기대 × (1 + 범위 × 0.8)
double TowerPower(const FPDTowerStats& S);
// 판 위력 어림의 합: 타워마다 TowerPower × Weight(타워, 사거리). 조커 능력은 어림 배율
// (희비극 ×1.8, 스타 ×4, 방울 ×1.3, 웃는 가면은 따라 하는 타워). Towers 는 ApplyEffects 를 마친 판. Weight 가 없으면 1
// 봇(Tests/core_test.cpp)은 Weight 로 사거리 안 길 칸 수를 곱한다
using FPDPowerWeight = std::function<double(const FPDTower& T, float Range)>;
double SumBoardPower(const FPDTowerMap& Towers, FPDCharmBits Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight = nullptr);
// 타워 하나의 몫 (SumBoardPower 의 한 항): 조커 어림 배율까지
double OneTowerPower(const FPDTowerMap& Towers, const FPDTower& T, FPDCharmBits Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight = nullptr);
// 판 위력 어림 (D-107, 화면 비교용): 사거리·적 위치는 보지 않는다
float BoardPower(FPDTowerMap Towers, FPDCharmBits Charms = 0, const FPDTreeMods* Tree = nullptr);

// ---------------------------------------------------------------- 길
struct FPDPoint
{
	float X = 0.f, Y = 0.f;
};
struct FPDPath
{
	std::vector<std::array<int, 2>> Corners; // 격자 좌표 (화면 밖 -1, Cols 포함)
	std::vector<FPDPoint> Points;            // 칸 중심
	std::vector<float> SegStart;             // 각 구간 시작 거리
	float Length = 0.f;
	std::vector<uint8_t> OnPath;             // Cols*Rows, 길 칸이면 1
	void Build(const std::vector<std::array<int, 2>>& InCorners);
	FPDPoint At(float D) const;
	bool IsPath(int X, int Y) const { return OnPath[CellKey(X, Y)] != 0; }
	int CellCount() const;
};
constexpr int PathMin = 34, PathMax = 46;
std::vector<std::array<int, 2>> GeneratePath(std::mt19937& Rng);

// ---------------------------------------------------------------- 적
// 보통, 빠름, 탱크, 방패병(한 방 피해 감소), 분열체(죽으면 둘), 재생자(체력 회복), 분열 조각, 보스 3종
// 보스 13종 (D-112): 5웨이브마다 BossOrder 순서로 돌아온다 (65웨이브에 한 바퀴)
enum class EPDEnemy : uint8_t { Normal, Fast, Tank, Armored, Splitter, Regen, Mini, ShieldBoss, SplitBoss, RegenBoss,
	Charger, Summoner, Phantom, Warlord, Berserker, Blinker, Devourer, Warden, Hexer, Breaker,
	Bluffer, BigBlind, Dealer, Counter, HouseRule, Twins, // 새 보스 6종 (D-334, D-335)
	Bouncer, HighRoller, House,                             // 모드 전용 최종 보스 (30 · 60 · 100)
	Decoy,                                                  // 블러퍼 분신 (체력 1)
	Count };
constexpr int BossKinds = 22; // 보스 19 + 최종 보스 3 (방패 거인 ~ 더 하우스)
inline int BossBit(EPDEnemy T) { const int B = (int)T - (int)EPDEnemy::ShieldBoss; return B >= 0 && B < BossKinds ? B : -1; }
inline bool IsFinalBoss(EPDEnemy T) { return T == EPDEnemy::Bouncer || T == EPDEnemy::HighRoller || T == EPDEnemy::House; }
// 보스 능력 수치 (D-112)
namespace Boss
{
constexpr float ChargeEvery = 4.f, ChargeTime = 0.8f, ChargeSpeed = 3.f;   // 돌진 기사
constexpr float SummonEvery = 3.f; constexpr int SummonCount = 2;           // 소환사: 졸개(분열 조각) 둘
constexpr float SummonHp = 0.12f;                                          //   졸개 체력 = 보스 기준 체력 × 이 값
constexpr float PhaseEvery = 4.f, PhaseTime = 1.2f;                        // 유령: 무적·조준 안 됨
constexpr float AuraRange = 2.5f, AuraSpeed = 1.35f, AuraArmor = 4.f;       // 군단장: 주변 적 빠르고 단단하게
constexpr float RageSpeed = 1.5f;                                          // 분노 거인: 속도 × (1 + 1.5 × 잃은 체력 비율)
constexpr float BlinkEvery = 5.f, BlinkDist = 2.5f;                        // 공간 도약자
constexpr float DevourRange = 2.5f, DevourHeal = 0.2f;                     // 흡수자: 죽은 적 최대 체력의 20% 회복
constexpr float WardEvery = 6.f, WardTime = 3.f, WardShield = 0.15f;       // 수호자: 최대 체력 15% 보호막
constexpr float HexRange = 3.f, HexDmg = 0.65f;                            // 저주술사: 주변 타워 공격력 ×0.65
constexpr float BreakEvery = 6.f, BreakRange = 2.5f, BreakStun = 2.5f;     // 파괴자: 가까운 타워 기절
// 새 보스 (D-334)
constexpr int DecoyN = 2;                                                  // 블러퍼: 분신 둘 (체력 1)
constexpr float BlindTime = 20.f, BlindPotK = 0.3f, AllInSpeed = 1.5f, AllInArmor = 8.f; // 빅 블라인드: 판돈 = 다음 레벨까지 경험치의 30% (가진 만큼), 20초 안에 잡으면 ×2, 넘기면 올인
constexpr float DealEvery = 8.f, DealTime = 6.f; constexpr int DealN = 2, DealMax = 3; // 딜러: 8초마다 무작위 2장(조커 · 와일드 · Lv4 빼고) 6초, 한꺼번에 최대 3장
constexpr float CounterGain = 0.02f, CounterDecay = 0.01f, CounterMax = 0.6f; // 카드 카운터: 맞은 무늬 피해 내성 (최대 60%), 다른 무늬로 맞으면 풀림
constexpr float HouseRange = 3.f, HouseRangeBig = 8.f;                     // 하우스 룰: 주변 타워 연쇄 배율 끔 (더 하우스 2단계는 판 절반)
constexpr float TwinHeal = 0.5f, TwinRage = 1.6f;                          // 쌍둥이: 하나가 죽으면 남은 쪽 체력 50% 회복 + 속도 ×1.6
// 최종 보스 (D-334, D-335): 체력 2/3 · 1/3 에서 단계가 바뀐다
constexpr float BouncerStunEvery = 5.f, BouncerStunRange = 2.5f, BouncerStun = 2.f; // 바운서 3단계: 주변 타워 2초 기절
constexpr float PileEvery = 6.f, PileTime = 6.f, PileRange = 0.6f, PileSpeed = 1.3f;  // 하이롤러 1단계: 칩 더미 (지나가는 적 빠르게)
constexpr float AllInRage = 1.5f, AllInShieldEvery = 6.f, AllInShield = 0.1f;          // 하이롤러 3단계: 올인
constexpr float HouseCycle = 6.f, HouseSummonEvery = 3.f;                  // 더 하우스: 1단계 남의 능력을 6초씩 빌림, 3단계 졸개 + 흡수
// 변이 (D-334): 60 · 100 뒤쪽 · 무한 뒤쪽 보스
constexpr float MutArmor = 10.f, MutSpeed = 1.3f, MutRegen = 0.01f, MutShield = 0.25f, MutHex = 0.8f;
}
enum EPDMut : uint8_t { MutHard = 1, MutFast = 2, MutRegen = 4, MutShield = 8, MutLead = 16, MutHex = 32, MutAll = 63 }; // 변이 비트
enum class EPDBossForm : uint8_t { Solo, Duo, Trio, Escort, Final };  // 보스 웨이브 짜임 (D-334): 단독 · 짝 · 셋 · 호위(보스 + 정예 넷) · 최종
struct FPDBossSlot { EPDEnemy T = EPDEnemy::ShieldBoss; uint8_t Mut = 0; };
extern const EPDEnemy BossOrder[13];
struct FPDEnemyDef
{
	float Hp, Speed, Regen;
	int Split;      // 죽으면 갈라지는 수
	bool bArmor, bBoss;
};
const FPDEnemyDef& EnemyDef(EPDEnemy T);
// 웨이브 마리 수 (보스 빼고) = 12 + 4W. 예전 8 + 2W 와 웨이브 전체 체력·골드·길이를 맞추는 배율 = (8 + 2W) / (12 + 4W)
int WaveCount(int W);
float WaveScale(int W);
constexpr float MobHpBonus = 2.5f; // 마리 수가 늘면 범위 공격·연쇄가 여러 마리를 맞혀 쉬워지므로 일반 적 체력에 곱한다 (보스는 그대로). 봇으로 맞춤: 1.6 → 21.6, 2.5 → 19.1웨이브
struct FPDEnemy
{
	EPDEnemy Type = EPDEnemy::Normal;
	float D = 0.f, Hp = 1.f, MaxHp = 1.f, Speed = 1.f, SlowT = 0.f, SlowAmt = 0.f, Armor = 0.f, BaseHp = 1.f;
	int Wave = 1;
	bool bDead = false;
	int Id = 0;
	float Incoming = 0.f; // 날아가는 중인 발사체 피해 합 (겹쳐 쏘지 않게)
	float StopT = 0.f;    // 방울 멈춤 남은 시간 (둔화와 따로: 예전엔 둔화 100%로 넣어 이어진 둔화가 멈춤을 끝없이 늘렸다, 이슈 #5)
	// 보스 능력 (D-112)
	float AbilT = 0.f;  // 능력 주기 타이머
	float StateT = 0.f; // 능력이 켜져 있는 남은 시간 (돌진·무적·보호막)
	float Shield = 0.f; // 수호자 보호막 남은 양
	uint8_t Mut = 0;     // 변이 (EPDMut)
	bool bElite = false; // 호위 정예 (체력 ×3)
	bool bRage = false;  // 쌍둥이 분노 · 빅 블라인드 올인
	float Pot = 0.f;     // 빅 블라인드 판돈 (경험치)
	float XpK = 1.f;     // 처치 경험치 배율 (짝 · 셋은 나눠 받음, 분신 0)
	int Phase = 0;       // 최종 보스 단계 (0 · 1 · 2)
	int Link = 0;        // 쌍둥이 짝 Id
	int Borrow = -1;     // 더 하우스가 빌린 능력 (EPDEnemy)
	float Res[4] = {0.f, 0.f, 0.f, 0.f}; // 카드 카운터 무늬 내성
	bool Untargetable() const { return (Type == EPDEnemy::Phantom || Borrow == (int)EPDEnemy::Phantom) && StateT > 0.f; }
};

// ---------------------------------------------------------------- 발사체 (미리보기: tools/neon/neon.py shots / shots_level / shots_hand / shots_joker)
// 고르는 순서: 조커 포탑 → 족보 타워(스트레이트 이상) → 무늬 기본형. 레벨(합성)은 그림만 진화시킨다 (위젯)
// 무늬: ♠ 레이저 (즉시), ♥ 미사일 (휘어 날아가는 유도탄), ♦ 탄환 (빠름), ♣ 서리 구체 (느림), ★ 별 조각
// 족보: 스트레이트 화살, 플러시 번개 (즉시), 풀하우스 분열탄, 포카드 포탄, 스트레이트 플러시 혜성, 파이브 카드 파동, 로열 금빛 광선 (즉시)
// 조커: 광대 공, 모자 장수 실크해트, 웃는 가면 초승달, 희비극 두 구슬, 할리퀸 마름모, 지휘봉 음표, 잭 칼날, 체셔 웃음 광선 (즉시), 방울 소리 고리, 스타 별 혜성
// 즉시가 아니면 날아가서 닿을 때 피해를 준다 (범위 피해도 닿은 자리에서). 피해 계산은 종류와 상관없다
enum class EPDShot : uint8_t
{
	Laser, Missile, Bullet, Orb, Star, JokerLaser,
	Arrow, Arc, Cluster, Shell, Comet, Nova, Crown,                          // 족보
	Juggle, Hat, Crescent, Twin, Shards, Note, Blade, Grin, Wave, StarComet, // 조커 (EPDJoker 순서)
	Count
};
EPDShot ShotKind(const FPDTower& T);
float ShotSpeed(EPDShot K); // 칸 / 초, 0 = 즉시
inline bool IsJokerShot(EPDShot K) { return K == EPDShot::JokerLaser || K >= EPDShot::Juggle; }
inline bool IsHandShot(EPDShot K) { return K >= EPDShot::Arrow && K <= EPDShot::Crown; }
struct FPDProjectile
{
	EPDShot Kind = EPDShot::Bullet;
	float X = 0.f, Y = 0.f, VX = 0.f, VY = 0.f; // 칸 단위 위치와 속도
	float TX = 0.f, TY = 0.f;                   // 마지막으로 본 목표 위치
	int Target = 0;                             // 목표 적 Id (죽으면 마지막 위치로 날아가 터짐)
	float Dmg = 0.f, Splash = 0.f, Slow = 0.f, Life = 0.f;
	int Crits = 0, Level = 1;
	int Src = -1;               // 쏜 타워 칸 키 (처치 경험치, D-125)
	bool bHand = false;
	EPDSuit Suit = EPDSuit::Spade;
};

// ---------------------------------------------------------------- 화면 연출용 사건 (위젯이 꺼내 파티클을 만든다)
struct FPDEvent
{
	enum class EKind : uint8_t { Shot, Splash, Kill, Leak, Place, Synergy, WaveStart, WaveClear, GameOver, Discard, Reward, Fuse, JokerFuse, Feed, Bell, Star, TreeBuy, Impact, BossSkill, JokerMerge, JokerXp, LevelUp, RunClear, Lucky } Kind = EKind::Shot; // BossSkill: Enemy=보스, X·Y=보스 위치, X2·Y2=대상(파괴자: 기절한 타워 칸)
	float X = 0.f, Y = 0.f, X2 = 0.f, Y2 = 0.f, R = 0.f;
	EPDSuit Suit = EPDSuit::Spade;
	EPDEnemy Enemy = EPDEnemy::Normal;
	EPDHand Hand = EPDHand::Pair;
	int Lv = 1;       // Shot·Impact: 쏜 타워 레벨 (그림 진화)
	float Dmg = 0.f;  // Shot(즉시 발사체)·Impact: 목표에 준 피해 (화면의 피해 숫자)
	int A = 0, B = 0; // Synergy: A=장수, B=연쇄 / WaveStart: A=웨이브, B=보스 여부, Enemy=보스 종류 / Leak: A=잃은 목숨 / WaveClear: A=받은 카드 / Reward: A=조커면 1, B=번호 / Fuse: A=새 레벨, B=합친 장수, X·Y=합쳐진 칸 / JokerFuse: A=조커 종류, B=처음 만듦 여부 / Feed: 먹은 조커마다 하나, X·Y=조커 칸, A=새 레벨, B=오른 레벨 수, R=얻은 경험치, Lv=먹은 조커 수, Dmg=그중 몇 번째 (D-494) / Bell·Star: X·Y=조커 칸, R=범위 / Kill: A=번 골드 / TreeBuy: A=노드 / Shot: A=족보 타워, B=치명타 횟수, R=발사체 종류(EPDShot) / Impact: 발사체가 닿음, R=종류, B=치명타 횟수, X2·Y2=날아온 속도 / Lucky: X·Y=기다리던 칸, A=숫자, Suit=무늬 (D-490)
};

enum class EPDPhase : uint8_t { Title, Build, Wave, Reward, Over, Clear }; // Reward: 부적 창에서 고르는 중, Clear: 모드 목표 웨이브를 넘김 (D-229)
// 모드 (D-225, D-229): 30 · 60 · 100웨이브를 버티면 클리어, 무한은 끝없음. 15웨이브 뒤 적 체력 성장 배율이 모드마다 다르다
enum class EPDMode : uint8_t { W30, W60, W100, Endless, Count };
inline int ModeGoal(EPDMode M) { return M == EPDMode::W30 ? 30 : M == EPDMode::W60 ? 60 : M == EPDMode::W100 ? 100 : 0; } // 0 = 끝없음
inline int FirstClearBonus(EPDMode M) { return ChipMult * (M == EPDMode::W30 ? 200 : M == EPDMode::W60 ? 400 : M == EPDMode::W100 ? 800 : 0); } // 모드 첫 클리어 덤 (D-321, D-323)
constexpr int FirstRunMinChips = 50; // 첫 판은 칩 최소 50: 끝나자마자 하나는 살 수 있게 (D-321)

// 목표로 여는 것 (D-321 ④): 칩 대신 목표를 이루면 열리는 칵테일 10종. 스팀 도전 과제(B8)와 같이 쓴다
struct FPDGoalStats
{
	int RunsPlayed = 0;        // 끝낸 판 수
	long long TotalKills = 0;  // 모든 판 처치 합
	int BestLevel = 0;         // 한 판 최고 레벨
	int BestKills = 0;         // 한 판 최고 처치
	uint32_t ModesCleared = 0; // 1 << EPDMode
	uint32_t Discovered = 0;   // 만들어 본 기본 조커 (1 << EPDJoker)
	uint32_t UpperFound = 0;   // 만들어 본 상위 조커
	uint32_t BossKilled = 0;   // 잡은 보스 (1 << BossBit, D-337)
};
enum class EPDGoal : uint8_t { Clear30, Clear60, Clear100, FirstJoker, FirstUpper, AllJokers, Level30, Kills1000, Bosses9, Runs10, Count }; // Bosses9: 보스 9종 처치 (D-340, 예전 누적 10,000마리)
struct FPDGoalDef { EPDGoal Goal; EPDCharm Reward; };
inline const FPDGoalDef& GoalDef(int I)
{
	static const FPDGoalDef D[(int)EPDGoal::Count] = {
		{EPDGoal::Clear30, EPDCharm::FullHouseFan}, {EPDGoal::Clear60, EPDCharm::RoyalGuard}, {EPDGoal::Clear100, EPDCharm::StraightFlushFan},
		{EPDGoal::FirstJoker, EPDCharm::JokerFan}, {EPDGoal::FirstUpper, EPDCharm::BigJoker}, {EPDGoal::AllJokers, EPDCharm::Forge},
		{EPDGoal::Level30, EPDCharm::Herald}, {EPDGoal::Kills1000, EPDCharm::Bounty}, {EPDGoal::Bosses9, EPDCharm::BossHunter}, {EPDGoal::Runs10, EPDCharm::Dexterity}};
	return D[I];
}
bool GoalMet(EPDGoal G, const FPDGoalStats& S);
// 도전 과제 (B8, DOC/design/17 4절): 목표 해금 10종(EPDGoal 과 같은 순서) + 14종. 스팀 도전 과제 API 이름은 AchApiName
enum class EPDAch : uint8_t
{
	Clear30, Clear60, Clear100, FirstJoker, FirstUpper, AllJokers, Level30, Kills1000, Bosses9, Runs10, // 목표 해금과 같다
	Endless150, Endless200, RoyalFlush, FiveCard, AllHands, FuseMax, Uppers10, AllUppers, Charms30, AllCharms, UpgradesAll, SecretChannel, Runs50, TotalKills100k,
	AllBosses, AllFinals, Mut3Boss, // 보스 (D-337)
	Count
};
struct FPDAchStats : FPDGoalStats
{
	uint32_t HandsEver = 0;   // 만들어 본 족보 (1 << EPDHand)
	int MaxFuseLevel = 0;     // 만들어 본 가장 높은 합성 레벨
	int CharmsOpen = 0;       // 연 칵테일 수
	int EndlessBest = 0;      // 무한 모드 최고 웨이브
	bool bUpgradesMax = false; // 영구 강화 8종 모두 20
	bool bSecretChannel = false; // 채널 TV 숨은 채널 (D-316)
	bool bMut3Kill = false;   // 변이 셋 붙은 보스를 잡음
};
bool AchMet(EPDAch A, const FPDAchStats& S);
const char* AchApiName(EPDAch A); // 스팀 도전 과제 이름 (Steamworks 에 같은 이름으로 등록)
inline int CharmGoal(int J) { for (int I = 0; I < (int)EPDGoal::Count; ++I) if ((int)GoalDef(I).Reward == J) return I; return -1; } // 목표로 여는 칵테일이면 목표 번호, 아니면 -1 (칩으로 못 산다)
inline float ModeGrowth(EPDMode M) { return M == EPDMode::W30 ? 1.28f : M == EPDMode::W60 ? 1.165f : M == EPDMode::W100 ? 1.105f : 1.18f; } // D-493 · QA 34 (덤 10, 목표 30 95% · 60 70% · 100 30%): 30 96% · 60 66% · 100 29%, 무한 1.18 은 제안값. 60 은 1.1625 = 75% · 1.17 = 61% 로 아주 예민하다, 조커 · 부적 · 트리 숫자를 바꾸면 다시 잰다
constexpr float FinalBossHpK = 3.f; // 목표 웨이브의 보스 = 최종 보스 (체력 ×3)

// 부하 시험 배율 (D-135, F4 치트): 웨이브 적 체력·마리 수. 평소 1
inline float GPDDebugHpMul = 1.f;
inline int GPDDebugCountMul = 1;

class FPDGame
{
public:
	explicit FPDGame(uint32_t Seed = 0);
	FPDEnemy SpawnTestEnemy(EPDEnemy T, float BaseHp, int W) { return MakeEnemy(T, BaseHp, W); } // 시험용
	const std::vector<FPDEnemy>& PendingForTest() const { return Pending; } // 시험용
	void ClearWaveForTest() { Pending.clear(); Enemies.clear(); } // 시험용: 이번 웨이브 적을 모두 없앰
	void HitTest(FPDEnemy& E, float Dmg) { Hit(E, Dmg, 0.f); } // 시험용
	void HitTestFrom(int SrcKey, FPDEnemy& E, float Dmg) { HitSrc = SrcKey; Hit(E, Dmg, 0.f); HitSrc = -1; } // 시험용: 이 칸 타워가 때림
	bool GainJokerXp(int Key, float Amount, bool bQuiet = false);   // 조커 경험치 (레벨이 오르면 true). 사건 JokerXp: A=레벨, B=오른 레벨 수, R=경험치. bQuiet 면 레벨이 오를 때만 사건
	void NewGame(bool bStart);
	// 중단 저장 (D-162): 준비 단계(Build)의 판을 글자로. 웨이브 중에 그만두면 그 웨이브 시작 직전(PreWaveSave)으로 돌아간다
	std::string SaveRun() const;
	bool LoadRun(const std::string& S); // 성공하면 준비 단계로 (자동 시작은 꺼 둠)
	std::string PreWaveSave; // 웨이브를 시작하기 직전의 저장 (StartWave 가 만든다)
	int SaveSerial = 0;      // PreWaveSave 가 새로 만들어질 때마다 +1
	void Tick(float Dt);                     // 0.05초로 잘라 Speed 배만큼 진행
	bool Place(int HandIdx, int X, int Y);   // 카드 설치 (타워 위면 교체)
	bool SwapTowers(int From, int To);       // 자리바꿈 (D-149): 타워 둘의 자리를 바꾸거나 빈 칸으로 옮김, Swaps 하나 씀
	void Discard(int HandIdx);
	// 판매 (D-221): 판 위 타워를 팔아 조금의 골드. 기본 B = 2 + 웨이브/5, 카드 B · 합성 타워 B × 레벨² · 조커 B × 10
	int SellPrice(int Cell) const; // 팔 수 없으면 0
	bool SellTower(int Cell);
	void StartWave();
	void TakeReward(int Index); // -1 = 건너뛰기
	bool CanFuse(int SynergyIdx) const;
	// 합성이 막힌 까닭 (D-371): 화면이 '합성 불가' 줄에 이유를 보여 준다
	enum class EFuseBlock : uint8_t { None, NotInPlay, ClownPath, TopLevel }; // 조커가 낀 족보(D-200)는 없어졌다 (D-489)
	EFuseBlock FuseBlock(int SynergyIdx) const;
	// 합성: 결과 타워가 놓일 칸을 재료 칸 중에서 고를 수 있다 (-1 = 기본 칸). 재료 밖 칸이면 실패
	bool Fuse(int SynergyIdx, int TargetCell = -1);  // 족보를 타워 하나로 합성 (버튼). 성공하면 true
	std::vector<FPDJokerGroup> JokerGroups() const; // 조커 합성 가능한 묶음
	bool JokerFuse(int GroupIdx, int TargetCell = -1); // 묶음을 조커 포탑으로
	// 조커 만들기 (D-195): 플레이어가 재료를 직접 고른다 (JokerNodes 번호 2~4개, 레시피 길이만큼, 거리 상관없음)
	std::vector<FPDJokerNode> JokerNodes() const;
	// 한 조커에 같이 못 넣는 재료 둘 (D-220): 합성 타워와 그 타워가 든 족보 (같은 칸을 두 번 쓰게 된다)
	static bool JokerNodesClash(const FPDJokerNode& A, const FPDJokerNode& B);
	EPDJoker JokerCraftResult(const std::vector<int>& Nodes) const; // 레시피와 맞으면 그 조커, 아니면 Count
	bool JokerCraft(const std::vector<int>& Nodes, int TargetCell); // TargetCell = 재료 칸 중 하나 (-1 = 기본 칸)
	bool JokerFuseGroup(const FPDJokerGroup& G, int TargetCell);
	// 조커 목록 (D-210): 그 조커를 만들 가장 좋은 재료 (JokerNodes 번호, 레시피 길이만큼). 안 되면 빈 것.
	// 레시피 족보마다 위력 배율이 큰 후보 4개까지 중에서, 만든 뒤 판 위력이 가장 높은 조합
	std::vector<int> BestJokerPicks(EPDJoker J) const;
	// 레시피에서 판에 모자란 족보 수 (같은 족보가 둘 필요하면 둘 다 세야). Missing = 모자란 족보 하나 (없으면 Count)
	int JokerMissing(EPDJoker J, EPDHand* Missing = nullptr) const;
	// 먹일 수 있는 족보 (족보마다 하나). 판에 조커가 한 마리라도 있으면 판 위 모든 족보 (D-494, 예전 맞닿은 족보만 D-489)
	std::vector<FPDFeed> FeedTargets() const;
	// 족보를 먹인다 (족보 칸은 사라짐). 판 위 모든 조커가 각자 경험치 전량 + 흡수 60% 를 받는다 (D-494). 조커마다 Feed 사건 하나
	bool Feed(int SynergyIdx);
	float FeedGain(float Val, const FPDTower& Joker) const; // 먹인 세기 Val 을 이 조커 단위로 바꾼 흡수량 (D-191)
	std::vector<FPDJokerMerge> JokerMerges() const;  // 같은 조커끼리 맞닿은 묶음 (2개 이상)
	bool MergeJokers(int MergeIdx, int TargetCell = -1); // 묶음을 조커 하나로 (위력 합 × JokerMergeBonus, 레벨은 가장 높은 것)
	// 버튼을 누르기 전 판 위력 비교 (D-107): 지금 / 누른 뒤 (기본 칸에 합칠 때). 누를 수 없으면 After < 0
	enum class EPowerAction : uint8_t { Fuse, JokerFuse, Feed, JokerMerge };
	struct FPowerPreview { float Before = 0.f, After = -1.f; };
	FPowerPreview PreviewPower(EPowerAction Action, int Idx) const;
	float Power() const { return BoardPower(Towers, CharmMask(), &Mods); }
	// 화면 위력 정보 (D-119): 타워마다 위력(초당 피해 어림), 큰 것부터. first = 칸 키
	std::vector<std::pair<int, float>> TowerPowers() const;
	float SynergyGain() const; // 족보 덤: 지금 판 위력 / 족보·연쇄·조커 효과가 없을 때 위력
	int WaveLeft() const { return (int)(Enemies.size() + Pending.size()); } // 이번 웨이브 남은 적 (나올 적 포함)
	int WaveTotal = 0;                                                    // 이번 웨이브 적 수 (분열·소환 제외)
	int NextBossWave() const { const int W = Phase == EPDPhase::Wave ? Wave : Wave + 1; return (W + 4) / 5 * 5; } // 지금 또는 다음 보스 웨이브
	std::vector<FPDBossSlot> BossWave(int W, EPDBossForm* OutForm = nullptr) const; // 그 웨이브 보스들 (D-335: 30 · 60 · 100 고정 차례, 무한은 100 뒤 판마다 뽑기)
	EPDEnemy BossOfWave(int W) const { const std::vector<FPDBossSlot> S = BossWave(W); return S.empty() ? EPDEnemy::ShieldBoss : S[0].T; } // 대표 보스 (첫째)
	struct FPDSuitSwap { int Cell = -1; EPDSuit Orig = EPDSuit::Spade; float T = 0.f; };
	std::vector<FPDSuitSwap> SuitSwaps; // 딜러가 잠깐 바꾼 무늬 (D-334)
	struct FPDChipPile { float D = 0.f, T = 0.f; };
	std::vector<FPDChipPile> ChipPiles; // 하이롤러 칩 더미
	uint32_t Discovered = 0;                         // 도감: 만들어 본 조커 (새 판에도 유지)
	uint32_t UpperFound = 0;                         // 도감: 만들어 본 상위 조커 (1 << 번호, D-231)
	uint32_t BossSeen = 0, BossKilled = 0;           // 도감 보스 판 (D-337): 만난 · 잡은 보스 (1 << BossBit), 새 판에도 유지
	bool bMut3Kill = false;                          // 변이 셋 붙은 보스를 잡은 적 있음 (도전 과제)
	FPDCharmBits CharmUnlocked = CharmBaseMask;          // 나올 수 있는 칵테일 (잠금 해제 메뉴로 늘어남, D-233, D-239)
	uint32_t JokerUnlocked = 0x3FFu;                    // 만들 수 있는 기본 조커 (1 << EPDJoker, D-239). 규칙·봇은 모두 열림, 게임은 진행 파일로
	float WaveStartT = 0.f;                             // 웨이브가 시작된 Time (첫 박자 칵테일, D-239)
	bool JokerOpen(int J) const { return J < 0 || J >= 32 || ((JokerUnlocked >> J) & 1u); }
	uint32_t UpperUnlocked = 0x3FFFFFFFu;               // 만들 수 있는 상위 조커 (1 << 번호, D-241). 규칙·봇은 모두 열림, 게임은 진행 파일로
	bool UpperOpen(int U) const { return U >= 0 && U < 30 && ((UpperUnlocked >> U) & 1u); }
	int UpperNeed(const FPDTower& T) const;          // 이 조커가 ★ 상위 조커 재료가 되는 조커 레벨 (10, 트리 끝자락이면 5)
	int UpperNeedFor(int U, const FPDTower& T) const; // 상위 조커 U 의 재료 레벨 조건: 일반 19종 0, ★ 11종 UpperNeed (D-333)
	std::vector<FPDReward> RollRewards();
	FPDCharmBits CharmMask() const;
	bool Has(EPDCharm J) const { return HasCharm(CharmMask(), J); }
	int HandMax() const { const int N = HandMaxBase + (Has(EPDCharm::BigHand) ? 2 : 0) + (Has(EPDCharm::Dexterity) ? 1 : 0) + Mods.HandAdd; return N < HandMaxCap ? N : HandMaxCap; }
	// 스킬 트리
	enum class EBuy : uint8_t { Ok, Owned, NotLinked, Sealed, KeyCap, NoPoint, Closed };
	// 레벨 (D-228): 적을 잡아 경험치 → 레벨업마다 트리 포인트 1. 다음 레벨까지 = 예전 n번째 포인트 골드 값 (20 × 1.22^(레벨-1))
	int XpToNext() const;
	int PointsLeft() const { return Level - 1 - Points; } // 남은 트리 포인트
	int KeystoneCount() const;
	bool TreeOwnedNode(int Id) const { return TreeOwned.test(Id); }
	bool TreeSealed(int Id) const;       // 짝 키스톤을 찍어서 막힌 노드
	EBuy CanBuy(int Id) const;
	bool BuyNode(int Id);
	int KillXp(EPDEnemy T, int W) const; // 경험치 덤 적용 전
	// 보스 능력 (D-112)
	bool InWarlordAura(const FPDEnemy& E) const; // 군단장 주변 (군단장 자신은 아님)
	bool Hexed(const FPDTower& T) const;          // 저주술사 근처 타워
	float HexFactor(const FPDTower& T) const;     // 저주 배율 (저주술사 ×0.65, 저주하는 변이 ×0.8)
	bool HouseRuled(const FPDTower& T) const;     // 하우스 룰 · 더 하우스 2단계 근처 (연쇄 배율 끔)
	void DealSuits(int N);                        // 딜러: 무작위 타워 N 장 무늬 바꾸기 (한꺼번에 최대 DealMax)
	void RestoreSuits();                          // 딜러가 바꾼 무늬 되돌리기
	float FuseMult() const;              // 합성 배율 (기본 1.5)
	float JokerMult() const;             // 조커 배율 (기본 JokerBonusK = 10, D-492)
	void RebuildMods();
	bool Buildable(int X, int Y) const;
	// 손패를 놓을 수 있나 (D-192): 빈 칸이거나 Lv1 카드 타워(교체하면 그 카드는 손으로). 합성 타워·조커 위에는 못 놓는다
	bool CanPlace(int X, int Y) const;
	// 고른 카드를 이 칸에 놓으면 생길 족보 (놓은 칸이 든 연쇄 전체)
	std::vector<FPDSynergy> PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers = nullptr) const;
	void Update(float Dt);

	EPDPhase Phase = EPDPhase::Title;
	EPDMode Mode = EPDMode::Endless; // NewGame 전에 정한다 (D-229)
	int Goal() const { return ModeGoal(Mode); }
	int RunChips() const { return ChipMult * (Wave + 2 * (Level - 1) + (Phase == EPDPhase::Clear ? Goal() : 0)); } // 판이 끝나면 받는 칩 (D-230 → D-323): 2 × (웨이브 + 2 × 오른 레벨 + 클리어 덤(목표 웨이브))
	bool IsFinalWave(int W) const { return Goal() > 0 && W == Goal(); }
	int Wave = 0, Lives = StartLives, Kills = 0, Leaks = 0, Discarded = 0, DeckRefills = 0;
	float AutoStartT = -1.f; // 다음 웨이브 자동 시작까지 남은 시간 (음수 = 없음). Tick(화면) 에서만 줄어든다
	int BestWave = 0; // 이번 실행에서 가장 멀리 간 웨이브
	int Xp = 0, Level = 1, Points = 0, XpEarned = 0; // 다음 레벨까지 모은 경험치, 레벨, 쓴 트리 포인트, 판에서 번 경험치 합
	int Swaps = StartSwaps;                   // 남은 자리바꿈 (시작 2번 D-489 + 보상 D-149)
	bool bLucky = true;                       // 행운의 한 장 (D-490). QA 가 끈 판과 비교할 때만 false
	float JokerBonusK = JokerBonus;           // 조커 덤 (D-492). QA 갈래만 바꾼다, 게임은 늘 JokerBonus
	float ModeGrowthK = 0.f;                  // 15웨이브 뒤 체력 증가율 (D-492). 0 이면 ModeGrowth(Mode). QA 갈래만 바꾼다
	float HpGrowth() const { return ModeGrowthK > 0.f ? ModeGrowthK : ModeGrowth(Mode); }
	int LuckyHits = 0;                        // 이번 판에 행운의 한 장이 온 횟수 (봇 기록)
	FPDLucky LuckyLine() const;               // 지금 판의 행운의 한 장 줄 (덱에 남은 카드만 기다림)
	void LuckyDraw(int First, int Got);      // 행운의 한 장: 방금 받은 카드(Hand[First..]) 중 한 장을 확률로 줄 끝 카드로 (D-490)
	float XpCarry = 0.f;                      // 경험치 덤의 소수 부분
	FPDTreeBits TreeOwned = FPDTreeBits(1);   // 찍은 노드 (0번 시작점은 처음부터)
	std::array<int, (int)EPDUpgrade::Count> Upg = {}; // 강화 레벨 (판 밖 영구 강화가 채운다, D-228. 새 판에도 유지)
	FPDTreeMods Mods;
	int Speed = 1;
	float Time = 0.f;
	std::vector<FPDCard> Deck, Hand;
	std::vector<EPDCharm> Charms;
	std::vector<FPDReward> Rewards;
	FPDTowerMap Towers;
	std::vector<FPDSynergy> Synergies;
	std::vector<FPDEnemy> Enemies;
	std::vector<FPDProjectile> Projectiles;
	FPDPath Path;
	std::vector<FPDEvent> Events;
	std::vector<FPDSoundEvent> Sounds;
	bool bMuted = false;
	std::vector<FPDEnemy> PreviewWaveSpec(int W) const { FPDGame C = *this; C.bMuted = true; return C.WaveSpec(W); } // 그 웨이브에 나올 적 (사본에서, 채널 TV CH1 D-316)

private:
	FPDEnemy MakeEnemy(EPDEnemy T, float BaseHp, int W);
	std::vector<FPDEnemy> WaveSpec(int W);
	void Draw(int N, int* OutGot = nullptr);
	void EndWave();
	void Hit(FPDEnemy& E, float Dmg, float Slow);
	int AddXp(float Amount, bool bBonus = true); // 경험치를 더하고 레벨업 (bBonus = 트리 경험치 덤 적용)
	void Fire(const FPDTower& T, FPDEnemy& Target, float Dmg, float Splash, float Slow, int Crits);
	bool InPlay() const { return Phase == EPDPhase::Build || Phase == EPDPhase::Wave; } // 설치·합성·구매를 할 수 있는 단계
	void AddFreshDeck(); // 52장 한 벌을 덱에 넣고 섞는다
	void Explode(const FPDProjectile& P, FPDEnemy* Target);
	void UpdateProjectiles(float Dt);
	void Ev(const FPDEvent& E) { Events.push_back(E); }
	void Sfx(EPDSound S, int A = 0, int B = 0) { if (!bMuted) Sounds.push_back({S, A, B, 1.f}); }

	std::mt19937 Rng;
	std::vector<FPDEnemy> Pending;
	std::vector<FPDEnemy> Born; // 분열로 새로 생긴 적 (갱신 끝에 합친다)
	float BossTick(FPDEnemy& E, float Dt);       // 보스 능력 진행, 이번 틱 속도 배율을 돌려준다
	void BossEvent(const FPDEnemy& E, float X2 = 0.f, float Y2 = 0.f);
	float SpawnT = 0.f;
	int NextId = 1;
	int HitSrc = -1; // 지금 피해를 주는 타워 칸 키 (처치 경험치를 줄 조커, -1 = 모름)
	bool bReapply = false; // 조커 레벨업으로 족보 효과를 다시 계산해야 함
	float KillSfxT = 0.f;
};
} // namespace PD
