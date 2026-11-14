// 포커 디펜스 규칙 코어
// 언리얼 의존성이 없는 순수 C++17. 언리얼 모듈 안에서 그대로 컴파일되고,
// Tests/core_test.cpp 가 이 파일과 PDSynth.cpp 만 컴파일해 단위 시험과 봇 시험을 돌린다 (Tests/run.sh).
// 처음 규칙은 웹 프로토타입(prototype/poker-defense/index.html, v1.2)에서 옮겼고, 그 뒤 기능은 여기에만 있다.
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
constexpr int CharmMax = 5;
constexpr int NumWaves = 15; // 1회차 (보스 3종이 한 바퀴). 진행은 무한이고 목숨이 0이 되면 끝. 이 뒤로는 적 체력이 더 천천히 오른다
constexpr int StartLives = 20;
constexpr int StartHand = 5;
constexpr int DrawPerWave = 3;
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
	Count
};
inline bool HasCharm(uint32_t Mask, EPDCharm J) { return (Mask >> (int)J) & 1u; }
// 보상 창은 CharmEvery 웨이브마다만 뜬다 (D-101). 그 사이 웨이브는 창 없이 AutoStartDelay 뒤 다음 웨이브
constexpr int CharmEvery = 5;
// 즉시 보상 (부적 칸을 쓰지 않음): 부적 창에서 와일드가 셋째 칸에 들거나, 부적 칸이 가득일 때 채움
enum class EPDInstant : uint8_t { Wild, Draw2, Repair, Gold, Swap, Count }; // 골드 = 10 + 웨이브 × 4
constexpr float WildChance = 0.5f; // 부적 창마다 와일드가 셋째 칸에 들 확률
constexpr float SwapChance = 0.4f; // 와일드가 없을 때 자리바꿈이 셋째 칸에 들 확률 (D-149)
constexpr int SwapPerReward = 2;    // 자리바꿈 보상 한 장 = 2번
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


// ---------------------------------------------------------------- 스킬 트리 (판 안에서만, docs/16-skill-tree.md)
// 적을 잡아 번 골드로 포인트를 사서 노드를 찍는다. 이미 찍은 노드와 이어진 노드만 살 수 있고,
// 포인트 값은 살 때마다 ×1.15 로 올라 무한 진행에서도 트리를 다 채우지 못한다.
// 키스톤은 둘씩 짝이라 하나를 찍으면 짝은 그 판 동안 봉인되고, 판마다 최대 3개.
enum class EPDNode : uint8_t { Start, Small, Notable, Keystone, Joker };
enum class EPDStat : uint8_t { None, SuitDmg, SuitRate, SuitRange, SuitSplash, SuitSlow, HandDmg, HandRate, HandSplash, AllDmg, ChainAdd, FuseAdd, FusedDmg, GoldPct, HandMax, Draw, Lives, Keystone, Joker, JokerDmg, JokerAdd, AllRate, CritChance, CritMult };
// 키스톤: (외골수, 잡식가) (큰 판, 작은 판) (대장장이, 광대의 길) (고독한 저격수, 군중) 이 짝. 짝 번호 = K ^ 1
enum class EPDKeystone : uint8_t { Purist, Omni, BigTable, SmallTable, Smith, ClownPath, Lone, Crowd, Count };
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
	float AllDmg = 0.f, ChainAdd = 0.f, FuseAdd = 0.f, FusedDmg = 0.f, GoldPct = 0.f, JokerDmg = 0.f, JokerAdd = 0.f;
	float AllRate = 0.f, CritChance = 0.f, CritMult = 0.f; // 공격 속도, 치명타 확률(+%p), 치명타 배율(+)
	// 골드 강화 (트리와 따로 곱한다)
	float UpgDmg = 0.f, UpgRate = 0.f, UpgSuit[4] = {}, UpgCrit = 0.f, UpgCritMult = 0.f;
	int HandAdd = 0, DrawAdd = 0;
	uint32_t Keys = 0;    // 찍은 키스톤 (비트)
	uint32_t JokerUp = 0; // 강화한 조커 (비트)
	bool Key(EPDKeystone K) const { return (Keys >> (int)K) & 1u; }
	bool Up(int Joker) const { return (JokerUp >> Joker) & 1u; }
	bool Up(const FPDTower& T) const; // 트리 강화 또는 조커 레벨 10 이상
};
FPDTreeMods BuildTreeMods(const FPDTreeBits& Owned);

// ---------------------------------------------------------------- 골드 강화 (레벨 0~9999, 판 안에서만)
enum class EPDUpgrade : uint8_t { Attack, Speed, Crit, CritMult, Spade, Heart, Diamond, Club, Count };
constexpr int UpgradeMax = 9999;
constexpr float BaseCrit = 0.05f, BaseCritMult = 1.5f; // 기본 치명타 5%, ×1.5. 100%를 넘으면 한 방에 치명타가 여러 번
int UpgradeCost(int Level);           // Level → Level+1 값
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
// 조커 레벨 (D-098): 맞닿은 족보를 먹이면 경험치. 레벨마다 위력 +15% (D-191), 레벨 10부터 강화 능력 (트리 조커 강화와 같음)
constexpr int JokerMaxLevel = 9999;
constexpr float JokerLevelDmg = 0.5f; // 레벨마다 위력 +50% (D-196, 예전 15%)
constexpr float JokerFeedAbsorb = 0.6f; // 먹이기: 먹인 타워 위력의 60% 를 조커 위력에 바로 더한다 (D-191)
constexpr int JokerUpLevel = 10;
inline float JokerXpNeed(int Level) { return 20.f + 10.f * (Level - 1); }
constexpr float JokerKillXp = 1.f, JokerBossXp = 15.f; // 조커가 적을 처치하면 경험치 (D-125)
struct FPDFeed
{
	int Syn = -1;   // 먹일 족보 번호
	int Joker = -1; // 먹을 조커 칸
	float Xp = 0.f; // 얻는 경험치
	float Gain = 0.f; // 조커 위력 배율(Mult)에 더해지는 양 (D-191)
};
// 조커 10종 (그림 번호 1~10 순서: docs/15-art-style.md 9절)
enum class EPDJoker : uint8_t { Clown, Hatter, Mask, Comedy, Harlequin, Baton, Jack, Cheshire, Bell, Star, Count };
constexpr float JokerBonus = 1.2f; // 조커 합성: 재료 위력 합 × 1.2
constexpr float JokerCardBonus = 0.1f; // 재료 카드 한 장마다 조커 위력 +10% (4장 ×1.4, 10장 ×2) (D-114, D-118)
constexpr float JokerMergeBonus = 1.3f; // 같은 조커끼리 합치기: 위력 합 × 1.3 (D-114)
// 같은 조커끼리 맞닿은 묶음 (D-114). 합치면 한 칸에 하나로
struct FPDJokerMerge
{
	int Joker = -1;
	std::vector<int> Cells;
};
// 조커 재료 묶음 (D-115, D-118): 족보·합성 타워(레벨 2+, 족보에 안 든 것) 3개 또는 2개가 레시피와 정확히 맞을 때.
// 판 어디에 있어도 된다 (D-195, 예전엔 서로 맞닿아야 했다). 3개 레시피를 먼저, 같은 개수끼리는 족보 기본값 합이 낮은 것부터
struct FPDJokerGroup
{
	std::vector<int> SynergyIdx; // 족보 번호
	std::vector<int> FusedCells; // 따로 선 합성 타워 칸
	std::vector<int> Cells;      // 쓰이는 칸 전부
	EPDJoker Result = EPDJoker::Clown;
};
// 조커 재료 하나 (D-195): 조커가 없는 족보 하나, 또는 족보에 안 든 합성 타워(레벨 2+) 하나
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
// 레시피 (D-115, D-118): 조커마다 족보 3개짜리와 2개짜리 (순서 무관). 같은 레벨에서 붙여도 따로 잡히는 조합만:
// 숫자 족보(페어·트리플…)는 하나만(투 페어 + 원 페어는 예외), 나머지는 스트레이트·플러시
const std::array<EPDHand, 3>& JokerRecipe(EPDJoker J);
const std::array<EPDHand, 2>& JokerRecipe2(EPDJoker J);
// 조커 등급 배율 (D-116): 재료가 3개로 줄어 약해진 만큼. 쉬운 레시피 ×2, 중간 ×2.5, 로열·파이브 카드·스트레이트 플러시가 드는 레시피 ×3
float JokerTier(EPDJoker J);
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands); // 족보 3개면 3개 레시피, 2개면 2개 레시피와 정확히 맞는 조커, 없으면 EPDJoker::Count
constexpr float CaptainCommand = 0.3f; // 지휘: 판 위 자기 족보 타워 위력 +30%, 대장이 여럿이면 겹친다 (+60%, +90% …)
constexpr int MaxLevel = 4;
constexpr float FuseBonus = 1.5f;      // 합성 덤: 합친 위력 × 1.5
constexpr float LevelRange = 0.1f;     // 레벨마다 사거리 +10%
inline int CellKey(int X, int Y) { return Y * Cols + X; }
// 합성 결과가 놓일 기본 칸: 재료 칸들 가운데에 가장 가까운 칸 (같으면 먼저 나온 칸)
int DefaultFuseCell(const std::vector<int>& Cells);
using FPDTowerMap = std::map<int, FPDTower>;

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
// 상하좌우로 붙은 타워 덩어리 안의 2~5장 연결 묶음을 모두 따져 큰 족보부터 고르고(카드 공유 가능, 새 카드 한 장 이상, 같은 종류끼리는 공유 안 함), 연쇄를 묶는다
std::vector<FPDSynergy> ComputeSynergies(const FPDTowerMap& Towers);
// 족보 효과를 타워에 반영하고 족보 목록을 돌려준다
std::vector<FPDSynergy> ApplyEffects(FPDTowerMap& Towers, uint32_t Charms = 0, const FPDTreeMods* Tree = nullptr);

struct FPDTowerStats
{
	float Dmg, Rate, Range, Splash, Slow;
	float Crit = BaseCrit, CritMult = BaseCritMult; // 치명타 확률 (1 넘으면 여러 번), 배율
	float CritFactor() const { return 1.f + Crit * (CritMult - 1.f); } // 기대 배율
};
FPDTowerStats TowerStats(const FPDTower& T, uint32_t Charms = 0, const FPDTreeMods* Tree = nullptr);
// 타워 하나의 어림 위력: 피해 × 연사 × 치명타 기대 × (1 + 범위 × 0.8)
double TowerPower(const FPDTowerStats& S);
// 판 위력 어림의 합: 타워마다 TowerPower × Weight(타워, 사거리). 조커 능력은 어림 배율
// (희비극 ×1.8, 스타 ×4, 방울 ×1.3, 웃는 가면은 따라 하는 타워). Towers 는 ApplyEffects 를 마친 판. Weight 가 없으면 1
// 봇(Tests/core_test.cpp)은 Weight 로 사거리 안 길 칸 수를 곱한다
using FPDPowerWeight = std::function<double(const FPDTower& T, float Range)>;
double SumBoardPower(const FPDTowerMap& Towers, uint32_t Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight = nullptr);
// 타워 하나의 몫 (SumBoardPower 의 한 항): 조커 어림 배율까지
double OneTowerPower(const FPDTowerMap& Towers, const FPDTower& T, uint32_t Charms, const FPDTreeMods* Tree, const FPDPowerWeight& Weight = nullptr);
// 판 위력 어림 (D-107, 화면 비교용): 사거리·적 위치는 보지 않는다
float BoardPower(FPDTowerMap Towers, uint32_t Charms = 0, const FPDTreeMods* Tree = nullptr);

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
	Charger, Summoner, Phantom, Warlord, Berserker, Blinker, Devourer, Warden, Hexer, Breaker, Count };
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
}
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
	// 보스 능력 (D-112)
	float AbilT = 0.f;  // 능력 주기 타이머
	float StateT = 0.f; // 능력이 켜져 있는 남은 시간 (돌진·무적·보호막)
	float Shield = 0.f; // 수호자 보호막 남은 양
	bool Untargetable() const { return Type == EPDEnemy::Phantom && StateT > 0.f; }
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
	enum class EKind : uint8_t { Shot, Splash, Kill, Leak, Place, Synergy, WaveStart, WaveClear, GameOver, Discard, Reward, Fuse, JokerFuse, Feed, Bell, Star, TreeBuy, Impact, BossSkill, JokerMerge, JokerXp } Kind = EKind::Shot; // BossSkill: Enemy=보스, X·Y=보스 위치, X2·Y2=대상(파괴자: 기절한 타워 칸)
	float X = 0.f, Y = 0.f, X2 = 0.f, Y2 = 0.f, R = 0.f;
	EPDSuit Suit = EPDSuit::Spade;
	EPDEnemy Enemy = EPDEnemy::Normal;
	EPDHand Hand = EPDHand::Pair;
	int Lv = 1;       // Shot·Impact: 쏜 타워 레벨 (그림 진화)
	float Dmg = 0.f;  // Shot(즉시 발사체)·Impact: 목표에 준 피해 (화면의 피해 숫자)
	int A = 0, B = 0; // Synergy: A=장수, B=연쇄 / WaveStart: A=웨이브, B=보스 여부, Enemy=보스 종류 / Leak: A=잃은 목숨 / WaveClear: A=받은 카드 / Reward: A=조커면 1, B=번호 / Fuse: A=새 레벨, B=합친 장수, X·Y=합쳐진 칸 / JokerFuse: A=조커 종류, B=처음 만듦 여부 / Feed: A=새 레벨, B=오른 레벨 수, R=얻은 경험치 / Bell·Star: X·Y=조커 칸, R=범위 / Kill: A=번 골드 / TreeBuy: A=노드 / Shot: A=족보 타워, B=치명타 횟수, R=발사체 종류(EPDShot) / Impact: 발사체가 닿음, R=종류, B=치명타 횟수, X2·Y2=날아온 속도
};

enum class EPDPhase : uint8_t { Title, Build, Wave, Reward, Over }; // Reward: 부적 창에서 고르는 중

// 부하 시험 배율 (D-135, F4 치트): 웨이브 적 체력·마리 수. 평소 1
inline float GPDDebugHpMul = 1.f;
inline int GPDDebugCountMul = 1;

class FPDGame
{
public:
	explicit FPDGame(uint32_t Seed = 0);
	FPDEnemy SpawnTestEnemy(EPDEnemy T, float BaseHp, int W) { return MakeEnemy(T, BaseHp, W); } // 시험용
	void HitTest(FPDEnemy& E, float Dmg) { Hit(E, Dmg, 0.f); } // 시험용
	void HitTestFrom(int SrcKey, FPDEnemy& E, float Dmg) { HitSrc = SrcKey; Hit(E, Dmg, 0.f); HitSrc = -1; } // 시험용: 이 칸 타워가 때림
	bool GainJokerXp(int Key, float Xp);   // 조커 경험치 (레벨이 오르면 true). 사건 JokerXp: A=레벨, B=오른 레벨 수, R=경험치
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
	void StartWave();
	void TakeReward(int Index); // -1 = 건너뛰기
	bool CanFuse(int SynergyIdx) const;
	// 합성할 족보 (D-128): 조커가 든 족보는 조커를 뺀 칸들이 그대로 족보면 그 칸들만 (예: 조커 + ♦ 셋 = 플러시4 → ♦ 셋 플러시 합성)
	bool FuseSynergy(int SynergyIdx, FPDSynergy& Out) const;
	// 합성: 결과 타워가 놓일 칸을 재료 칸 중에서 고를 수 있다 (-1 = 기본 칸). 재료 밖 칸이면 실패
	bool Fuse(int SynergyIdx, int TargetCell = -1);  // 족보를 타워 하나로 합성 (버튼). 성공하면 true
	std::vector<FPDJokerGroup> JokerGroups() const; // 조커 합성 가능한 묶음
	bool JokerFuse(int GroupIdx, int TargetCell = -1); // 묶음을 조커 포탑으로
	// 조커 만들기 (D-195): 플레이어가 재료를 직접 고른다 (JokerNodes 번호 2~3개, 거리 상관없음)
	std::vector<FPDJokerNode> JokerNodes() const;
	EPDJoker JokerCraftResult(const std::vector<int>& Nodes) const; // 레시피와 맞으면 그 조커, 아니면 Count
	bool JokerCraft(const std::vector<int>& Nodes, int TargetCell); // TargetCell = 재료 칸 중 하나 (-1 = 기본 칸)
	bool JokerFuseGroup(const FPDJokerGroup& G, int TargetCell);
	std::vector<FPDFeed> FeedTargets() const;        // 조커에 먹일 수 있는 족보 (족보마다 하나)
	bool Feed(int SynergyIdx);                       // 족보를 조커에 먹인다 (조커 아닌 칸은 사라지고 경험치). 조커는 먹이가 되지 않는다
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
	static EPDEnemy BossOfWave(int W) { return BossOrder[(W / 5 - 1) % 13]; }  // W 는 5의 배수
	uint32_t Discovered = 0;                         // 도감: 만들어 본 조커 (새 판에도 유지)
	std::vector<FPDReward> RollRewards();
	uint32_t CharmMask() const;
	bool Has(EPDCharm J) const { return HasCharm(CharmMask(), J); }
	int HandMax() const { return HandMaxBase + (Has(EPDCharm::BigHand) ? 2 : 0) + Mods.HandAdd; }
	// 스킬 트리
	enum class EBuy : uint8_t { Ok, Owned, NotLinked, Sealed, KeyCap, NoGold, Closed };
	int PointPrice() const;              // 다음 포인트 값
	int KeystoneCount() const;
	bool TreeOwnedNode(int Id) const { return TreeOwned.test(Id); }
	bool TreeSealed(int Id) const;       // 짝 키스톤을 찍어서 막힌 노드
	EBuy CanBuy(int Id) const;
	bool BuyNode(int Id);
	int KillGold(EPDEnemy T, int W) const; // 골드 덤 적용 전
	// 보스 능력 (D-112)
	bool InWarlordAura(const FPDEnemy& E) const; // 군단장 주변 (군단장 자신은 아님)
	bool Hexed(const FPDTower& T) const;          // 저주술사 근처 타워
	float FuseMult() const;              // 합성 배율 (기본 1.5)
	float JokerMult() const;             // 조커 배율 (기본 1.2)
	int UpgradeBatchCost(EPDUpgrade U, int N) const; // N레벨 올리는 값 (최대치에서 잘림)
	int Upgrade(EPDUpgrade U, int N);    // 살 수 있는 만큼 최대 N레벨 올림, 올린 레벨 수
	void RebuildMods();
	bool Buildable(int X, int Y) const;
	// 손패를 놓을 수 있나 (D-192): 빈 칸이거나 Lv1 카드 타워(교체하면 그 카드는 손으로). 합성 타워·조커 위에는 못 놓는다
	bool CanPlace(int X, int Y) const;
	// 고른 카드를 이 칸에 놓으면 생길 족보 (놓은 칸이 든 연쇄 전체)
	std::vector<FPDSynergy> PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers = nullptr) const;
	void Update(float Dt);

	EPDPhase Phase = EPDPhase::Title;
	int Wave = 0, Lives = StartLives, Kills = 0, Leaks = 0, Discarded = 0, DeckRefills = 0;
	float AutoStartT = -1.f; // 다음 웨이브 자동 시작까지 남은 시간 (음수 = 없음). Tick(화면) 에서만 줄어든다
	int BestWave = 0; // 이번 실행에서 가장 멀리 간 웨이브
	int Gold = 0, Points = 0, GoldEarned = 0; // 골드, 산 포인트 수, 판에서 번 골드 합
	int Swaps = 0;                            // 남은 자리바꿈 (보상, D-149)
	float GoldCarry = 0.f;                    // 골드 덤의 소수 부분
	FPDTreeBits TreeOwned = FPDTreeBits(1);   // 찍은 노드 (0번 시작점은 처음부터)
	std::array<int, (int)EPDUpgrade::Count> Upg = {}; // 골드 강화 레벨
	int GoldUpg = 0;                          // 강화에 쓴 골드
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

private:
	FPDEnemy MakeEnemy(EPDEnemy T, float BaseHp, int W);
	std::vector<FPDEnemy> WaveSpec(int W);
	void Draw(int N, int* OutGot = nullptr);
	void EndWave();
	void Hit(FPDEnemy& E, float Dmg, float Slow);
	int AddGold(float Amount);
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
