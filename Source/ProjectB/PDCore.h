// 포커 디펜스 규칙 코어
// 언리얼 의존성이 없는 순수 C++17. 언리얼 모듈 안에서 그대로 컴파일되고,
// Tests/core_test.cpp 가 이 파일만 g++ 로 컴파일해 단위 시험과 봇 시험을 돌린다.
// 웹 프로토타입(prototype/poker-defense/index.html, v1.2)의 규칙을 옮겼다.
// 좌표는 모두 칸 단위 (칸 중심 = x + 0.5). 화면 픽셀 변환은 위젯이 한다.
#pragma once

#include <array>
#include <bitset>
#include <cstdint>
#include <map>
#include <random>
#include <vector>

#include "PDSynth.h"

namespace PD
{
constexpr int Cols = 16;
constexpr int Rows = 10;
constexpr int HandMaxBase = 7;
constexpr int CharmMax = 5;
constexpr int NumWaves = 15; // 1회차 기준 (보스 3종이 한 바퀴). 진행은 무한, 목숨이 0이 되면 끝
constexpr int StartLives = 20;
constexpr int StartHand = 5;
constexpr int DrawPerWave = 3;
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
// 즉시 보상 (부적 칸을 쓰지 않음)
enum class EPDInstant : uint8_t { Wild, Draw2, Repair, Gold, Count }; // 와일드는 드물게 (WildChance, 3웨이브에 한 번까지), 골드 = 10 + 웨이브 × 4
constexpr float WildChance = 0.2f;
constexpr int WildCooldown = 3;
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
	bool IsJoker() const { return Joker >= 0; }
};
// 조커 10종 (그림 번호 1~10 순서: docs/15-art-style.md 9절)
enum class EPDJoker : uint8_t { Clown, Hatter, Mask, Comedy, Harlequin, Baton, Jack, Cheshire, Bell, Star, Count };
constexpr float JokerBonus = 1.2f; // 조커 합성: 재료 위력 합 × 1.2
// 조커 재료 묶음: 연쇄로 이어진 족보와 합성 타워(레벨 2+, 족보에 안 든 것)가 합쳐서 3개 이상
struct FPDJokerGroup
{
	std::vector<int> SynergyIdx; // 족보 번호
	std::vector<int> FusedCells; // 따로 선 합성 타워 칸
	std::vector<int> Cells;      // 쓰이는 칸 전부
	EPDJoker Result = EPDJoker::Clown;
};
// 대장: 족보마다 조커 하나. 재료(족보·합성 타워의 족보) 중 가장 높은 족보의 대장이 나온다
EPDJoker CaptainOf(EPDHand H);
EPDHand CaptainHand(EPDJoker J);
EPDJoker MatchJokerRecipe(const std::vector<EPDHand>& Hands);
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
// 상하좌우로 붙은 타워 덩어리 안의 2~5장 연결 묶음을 모두 따져 큰 족보부터 겹치지 않게 고르고, 연쇄를 묶는다
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
enum class EPDEnemy : uint8_t { Normal, Fast, Tank, Armored, Splitter, Regen, Mini, ShieldBoss, SplitBoss, RegenBoss, Count };
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
	bool bHand = false;
	EPDSuit Suit = EPDSuit::Spade;
};

// ---------------------------------------------------------------- 화면 연출용 사건 (위젯이 꺼내 파티클을 만든다)
struct FPDEvent
{
	enum class EKind : uint8_t { Shot, Splash, Kill, Leak, Place, Synergy, WaveStart, WaveClear, GameOver, Win, Discard, Reward, Fuse, JokerFuse, Bell, Star, TreeBuy, Impact } Kind = EKind::Shot;
	float X = 0.f, Y = 0.f, X2 = 0.f, Y2 = 0.f, R = 0.f;
	EPDSuit Suit = EPDSuit::Spade;
	EPDEnemy Enemy = EPDEnemy::Normal;
	EPDHand Hand = EPDHand::Pair;
	int Lv = 1;       // Shot·Impact: 쏜 타워 레벨 (그림 진화)
	float Dmg = 0.f;  // Shot(즉시 발사체)·Impact: 목표에 준 피해 (화면의 피해 숫자)
	int A = 0, B = 0; // Synergy: A=장수, B=연쇄 / WaveStart: A=웨이브, B=보스 여부, Enemy=보스 종류 / Leak: A=잃은 목숨 / WaveClear: A=받은 카드 / Reward: A=조커면 1, B=번호 / Fuse: A=새 레벨, B=합친 장수, X·Y=합쳐진 칸 / JokerFuse: A=조커 종류, B=처음 만듦 여부 / Bell·Star: X·Y=조커 칸, R=범위 / Kill: A=번 골드 / TreeBuy: A=노드 / Shot: A=족보 타워, B=치명타 횟수, R=발사체 종류(EPDShot) / Impact: 발사체가 닿음, R=종류, B=치명타 횟수, X2·Y2=날아온 속도
};

enum class EPDPhase : uint8_t { Title, Build, Wave, Reward, Over, Win }; // Reward: 웨이브 보상 고르는 중

class FPDGame
{
public:
	explicit FPDGame(uint32_t Seed = 0);
	FPDEnemy SpawnTestEnemy(EPDEnemy T, float BaseHp, int W) { return MakeEnemy(T, BaseHp, W); } // 시험용
	void HitTest(FPDEnemy& E, float Dmg) { Hit(E, Dmg, 0.f); } // 시험용
	void NewGame(bool bStart);
	void Tick(float Dt);                     // 0.05초로 잘라 Speed 배만큼 진행
	bool Place(int HandIdx, int X, int Y);   // 카드 설치 (타워 위면 교체)
	void Discard(int HandIdx);
	void StartWave();
	void TakeReward(int Index); // -1 = 건너뛰기
	bool CanFuse(int SynergyIdx) const;
	// 합성: 결과 타워가 놓일 칸을 재료 칸 중에서 고를 수 있다 (-1 = 기본 칸). 재료 밖 칸이면 실패
	bool Fuse(int SynergyIdx, int TargetCell = -1);  // 족보를 타워 하나로 합성 (버튼). 성공하면 true
	std::vector<FPDJokerGroup> JokerGroups() const; // 조커 합성 가능한 묶음
	bool JokerFuse(int GroupIdx, int TargetCell = -1); // 묶음을 조커 포탑으로
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
	float FuseMult() const;              // 합성 배율 (기본 1.5)
	float JokerMult() const;             // 조커 배율 (기본 1.2)
	int UpgradeBatchCost(EPDUpgrade U, int N) const; // N레벨 올리는 값 (최대치에서 잘림)
	int Upgrade(EPDUpgrade U, int N);    // 살 수 있는 만큼 최대 N레벨 올림, 올린 레벨 수
	void RebuildMods();
	bool Buildable(int X, int Y) const;
	// 고른 카드를 이 칸에 놓으면 생길 족보 (놓은 칸이 든 연쇄 전체)
	std::vector<FPDSynergy> PreviewSynergies(int HandIdx, int X, int Y, FPDTowerMap* OutTowers = nullptr) const;
	void Update(float Dt);

	EPDPhase Phase = EPDPhase::Title;
	int Wave = 0, Lives = StartLives, Kills = 0, Leaks = 0, Discarded = 0, DeckRefills = 0;
	int BestWave = 0; // 이번 실행에서 가장 멀리 간 웨이브
	int Gold = 0, Points = 0, GoldEarned = 0; // 골드, 산 포인트 수, 판에서 번 골드 합
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
	int LastWildWave = -99; // 마지막으로 와일드 보상이 나온 웨이브
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
	void Fire(const FPDTower& T, FPDEnemy& Target, float Dmg, float Splash, float Slow, int Crits, bool bMain);
	void Explode(const FPDProjectile& P, FPDEnemy* Target);
	void UpdateProjectiles(float Dt);
	void Ev(const FPDEvent& E) { Events.push_back(E); }
	void Sfx(EPDSound S, int A = 0, int B = 0) { if (!bMuted) Sounds.push_back({S, A, B, 1.f}); }

	std::mt19937 Rng;
	std::vector<FPDEnemy> Pending;
	std::vector<FPDEnemy> Born; // 분열로 새로 생긴 적 (갱신 끝에 합친다)
	float SpawnT = 0.f;
	int NextId = 1;
	float KillSfxT = 0.f;
};
} // namespace PD
