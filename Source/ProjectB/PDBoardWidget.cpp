#include "PDBoardWidget.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

#include <algorithm>

#include "PDTreeText.inl" // 생성됨: 특급 노드 이름 (tools/tree/gen_tree.py)

using namespace PD;

namespace PDDraw
{
constexpr float DesignW = 1280.f;
constexpr float DesignH = 720.f;
// 화면 세 칸: 왼쪽 족보 패널 | 가운데 판과 손패 | 오른쪽 상태 패널
constexpr float LX = 16.f, LW = 232.f; // 왼쪽 족보 패널
constexpr float CS = 48.f;          // 칸 크기
constexpr float FX = LX + LW + 14.f; // 판 왼쪽 위
constexpr float FY = 24.f;
constexpr float HandY = FY + Rows * CS + 26.f;
constexpr float CardW = 66.f, CardH = 92.f;
constexpr float RX = FX + Cols * CS + 14.f; // 오른쪽 패널
constexpr float PW = DesignW - RX - 16.f;   // 오른쪽 패널 너비
constexpr float RBtnX = RX + 12.f, RBtnW = PW - 24.f; // 오른쪽 패널 버튼 줄
constexpr float StartBtnY = FY + 92.f, StartBtnH = 38.f;
constexpr float TwoPi = 6.2831853f;

FLinearColor Hex(const TCHAR* InHex, float Alpha = 1.f)
{
	FLinearColor C(FColor::FromHex(InHex));
	C.A = Alpha;
	return C;
}
FLinearColor WithAlpha(FLinearColor C, float Alpha)
{
	C.A *= Alpha;
	return C;
}
FLinearColor SuitColor(EPDSuit S)
{
	switch (S)
	{
	case EPDSuit::Heart: return Hex(TEXT("ff6a8a"));
	case EPDSuit::Diamond: return Hex(TEXT("ffd96a"));
	case EPDSuit::Club: return Hex(TEXT("8affc4"));
	case EPDSuit::Wild: return Hex(TEXT("ffffff"));
	default: return Hex(TEXT("7fb8ff"));
	}
}
const TCHAR* SuitName(EPDSuit S)
{
	static const TCHAR* N[] = {TEXT("저격"), TEXT("폭발"), TEXT("연사"), TEXT("둔화"), TEXT("와일드")};
	return N[(int)S];
}
const TCHAR* SuitDesc(EPDSuit S)
{
	static const TCHAR* N[] = {TEXT("사거리 길고 한 방이 셈"), TEXT("맞은 자리 주변도 피해"), TEXT("약하지만 아주 빠름"), TEXT("맞은 적이 느려짐"), TEXT("아무 숫자·무늬로 친다")};
	return N[(int)S];
}
FLinearColor HandColor(EPDHand H)
{
	// 족보마다 다른 색 (D-099, 장판·테두리·패널 공통). 로열은 판 위 장판에서 무지개로 돈다
	static const TCHAR* C[] = {TEXT("fff4d6"), TEXT("ffd96a"), TEXT("ff9ad5"), TEXT("ff6a6a"), TEXT("ffa86a"), TEXT("b48cff"), TEXT("6a8cff"), TEXT("9cffb0"), TEXT("7fd6ff"), TEXT("cfd8e6")};
	return Hex(C[(int)H]);
}
const TCHAR* HandName(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("로열 스트레이트 플러시"), TEXT("파이브 카드"), TEXT("스트레이트 플러시"), TEXT("포카드"), TEXT("풀하우스"), TEXT("플러시"), TEXT("스트레이트"), TEXT("트리플"), TEXT("투 페어"), TEXT("원 페어")};
	return N[(int)H];
}
const TCHAR* HandEffect(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("공격력 ×2.5 · 연사 ×2.5 · 넓은 범위"), TEXT("공격력 ×5 (와일드·조커가 있어야)"), TEXT("공격력 ×2 · 연사 ×2 · 범위"), TEXT("공격력 ×3.5"), TEXT("공격력 ×1.5 · 연사 ×1.5 · 범위"), TEXT("범위 공격 (장수만큼 넓게)"),
		TEXT("연사 ×1.25 / ×1.5 / ×1.75 / ×2"), TEXT("공격력 ×2"), TEXT("공격력 ×1.5 · 사거리 ×1.25"), TEXT("공격력 ×1.5")};
	return N[(int)H];
}
FString HandLabel(const FPDSynergy& S)
{
	FString L = HandName(S.Kind);
	if (S.Kind == EPDHand::Flush || S.Kind == EPDHand::Straight) L += FString::FromInt(S.N);
	if (S.Chain >= 2) L += FString::Printf(TEXT(" 연쇄×%s"), *FString::SanitizeFloat(1.f + ChainStep * (S.Chain - 1)));
	return L;
}
FString RankText(int R)
{
	switch (R)
	{
	case 11: return TEXT("J");
	case 12: return TEXT("Q");
	case 13: return TEXT("K");
	case 14: return TEXT("A");
	default: return FString::FromInt(R);
	}
}
FLinearColor EnemyColor(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::Fast: return Hex(TEXT("ffa86a"));
	case EPDEnemy::Tank: return Hex(TEXT("c49aff"));
	case EPDEnemy::Armored: case EPDEnemy::ShieldBoss: return Hex(TEXT("9fc3e8"));
	case EPDEnemy::Splitter: case EPDEnemy::Mini: case EPDEnemy::SplitBoss: return Hex(TEXT("ff9ad5"));
	case EPDEnemy::Regen: case EPDEnemy::RegenBoss: return Hex(TEXT("b6ff6a"));
	default: return Hex(TEXT("e8f0ff"));
	}
}
const TCHAR* BossName(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::ShieldBoss: return TEXT("방패 거인");
	case EPDEnemy::SplitBoss: return TEXT("분열 군주");
	case EPDEnemy::RegenBoss: return TEXT("재생 왕");
	default: return TEXT("");
	}
}
const TCHAR* CharmName(int J)
{
	static const TCHAR* N[] = {TEXT("하트 애호가"), TEXT("스페이드 애호가"), TEXT("다이아 애호가"), TEXT("클로버 애호가"), TEXT("연결고리"), TEXT("쌍둥이"), TEXT("일직선"),
		TEXT("넓은 물결"), TEXT("그림 카드"), TEXT("작은 수"), TEXT("행운의 7"), TEXT("큰 손"), TEXT("보급"), TEXT("재활용"), TEXT("조준경"), TEXT("서리"), TEXT("연쇄 폭발"), TEXT("관통탄")};
	return N[J];
}
const TCHAR* CharmDesc(int J)
{
	static const TCHAR* N[] = {TEXT("♥ 타워 공격력 +40%"), TEXT("♠ 타워 공격력 +40%"), TEXT("♦ 타워 공격력 +40%"), TEXT("♣ 타워 공격력 +40%"), TEXT("연쇄 보너스 +25% → +45%"),
		TEXT("원 페어·투 페어 ×1.5 → ×2"), TEXT("스트레이트 연사 한 장마다 +0.25 더"), TEXT("플러시 범위 +0.75칸"), TEXT("J·Q·K 타워 공격력 +60%"), TEXT("2~6 타워 연사 +60%"),
		TEXT("숫자 7 타워 공격력 ×2.5"), TEXT("손패 최대 +2"), TEXT("웨이브마다 카드 +1"), TEXT("카드를 버리면 1장 뽑기"), TEXT("모든 타워 사거리 +15%"),
		TEXT("둔화 더 강하게, ♣ 주변도 둔화"), TEXT("처치 시 주변 적에게 최대 체력 15% 피해"), TEXT("적의 방패를 무시")};
	return N[J];
}
const TCHAR* JokerName(int K)
{
	static const TCHAR* N[] = {TEXT("광대"), TEXT("모자 장수"), TEXT("웃는 가면"), TEXT("희비극"), TEXT("할리퀸"), TEXT("지휘봉"), TEXT("잭"), TEXT("체셔"), TEXT("방울"), TEXT("스타")};
	return N[K];
}
const TCHAR* JokerAbility(int K)
{
	static const TCHAR* N[] = {TEXT("무늬 4종 공격을 번갈아 씀"), TEXT("주변 8칸 타워 공격력 +25%·사거리 +10%"), TEXT("주변 가장 강한 타워의 공격을 따라 함"),
		TEXT("두 적을 동시에 공격"), TEXT("판 위 같은 무늬 타워 전부 공격력 +30%"), TEXT("주변 8칸 타워 연사 +40%"), TEXT("아주 강한 한 방 (공격력 ×3, 연사 ×0.5)"),
		TEXT("사거리 무한: 가장 앞선 적 저격"), TEXT("8초마다 종: 사거리 안 적 1.5초 멈춤"), TEXT("판 전체 레이저: 모든 적에게 피해")};
	return N[K];
}
// 조커 조합법. 대장 순위는 코어(MatchJokerRecipe)와 같이 족보 기본값(HandBaseValue)으로 매긴다
// (트리플·투 페어가 플러시·스트레이트보다 높다)
EPDHand NextLowerHand(EPDHand H) // H 바로 아래 족보, 없으면 H
{
	int Best = -1;
	for (int I = 0; I < (int)EPDHand::Count; ++I)
		if (HandBaseValue((EPDHand)I) < HandBaseValue(H) && (Best < 0 || HandBaseValue((EPDHand)I) > HandBaseValue((EPDHand)Best))) Best = I;
	return Best < 0 ? H : (EPDHand)Best;
}
const TCHAR* HandShort(EPDHand H) { return H == EPDHand::RoyalFlush ? TEXT("로열") : H == EPDHand::StraightFlush ? TEXT("스티플") : HandName(H); } // 도감 칸이 좁아서
FString JokerRecipe(int K)
{
	const EPDHand H = CaptainHand((EPDJoker)K);
	if (NextLowerHand(H) == H) return FString::Printf(TEXT("%s 3개"), HandShort(H));
	return FString::Printf(TEXT("%s + 같거나 낮은 족보 2개"), HandShort(H));
}
FString JokerRecipeExample(int K)
{
	const EPDHand H = CaptainHand((EPDJoker)K), Low = NextLowerHand(H);
	if (Low == H) return TEXT("가장 낮은 족보라 셋 다 원 페어");
	return FString::Printf(TEXT("예) %s + %s + 원 페어"), HandShort(H), HandShort(Low));
}
FString JokerRankText() // 대장 순위 한 줄
{
	TArray<int32> Order;
	for (int32 I = 0; I < (int32)EPDHand::Count; ++I) Order.Add(I);
	Order.Sort([](int32 A, int32 B) { return HandBaseValue((EPDHand)A) > HandBaseValue((EPDHand)B); });
	FString Out = TEXT("대장 순위: ");
	for (int32 I = 0; I < Order.Num(); ++I) Out += (I ? TEXT(" > ") : TEXT("")) + FString(HandName((EPDHand)Order[I]));
	return Out;
}
FString JokerCommand(int K) { return FString::Printf(TEXT("지휘: %s 타워 위력 +30%% (겹침)"), HandName(CaptainHand((EPDJoker)K))); }
const FLinearColor JokerPink = FLinearColor(FColor::FromHex(TEXT("ff9ad5")));
// 발사체 색: 무늬 기본형은 무늬 색, 족보 발사체는 족보 색, 조커는 분홍
FLinearColor ShotColor(EPDShot K, EPDSuit S)
{
	switch (K)
	{
	case EPDShot::Arrow: return HandColor(EPDHand::Straight);
	case EPDShot::Arc: return Hex(TEXT("c9a2ff"));
	case EPDShot::Cluster: return HandColor(EPDHand::FullHouse);
	case EPDShot::Shell: return HandColor(EPDHand::Four);
	case EPDShot::Comet: return HandColor(EPDHand::StraightFlush);
	case EPDShot::Nova: return HandColor(EPDHand::Five);
	case EPDShot::Crown: return Hex(TEXT("ffe39a"));
	case EPDShot::Grin: return Hex(TEXT("d59aff"));
	default: return IsJokerShot(K) ? JokerPink : SuitColor(S);
	}
}
FVector2f Rot2(FVector2f V, float A) { const float C = FMath::Cos(A), S = FMath::Sin(A); return FVector2f(V.X * C - V.Y * S, V.X * S + V.Y * C); }
// 원호 점들: 중심 C, 반지름 R, 가운데 각 Mid, 양쪽 반폭 Half (라디안)
TArray<FVector2f> ArcPts(FVector2f C, float R, float Mid, float Half, int32 N = 12)
{
	TArray<FVector2f> P;
	for (int32 I = 0; I <= N; ++I) { const float A = Mid - Half + 2.f * Half * I / N; P.Add(C + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
	return P;
}
const TCHAR* InstantName(int I)
{
	static const TCHAR* N[] = {TEXT("와일드 카드"), TEXT("보충"), TEXT("수리"), TEXT("골드")};
	return N[I];
}
const TCHAR* InstantDesc(int I)
{
	static const TCHAR* N[] = {TEXT("아무 숫자·무늬로 치는 카드 1장 (드물게 나옴)"), TEXT("카드 2장 뽑기"), TEXT("목숨 +3"), TEXT("골드 +(10 + 웨이브 × 4)")};
	return N[I];
}
// 보상 카드 위치 (판 가운데 3장)
FVector2f RewardPos(int I) { return FVector2f(FX + Cols * CS / 2.f - 360.f + I * 250.f, FY + 160.f); }
constexpr float RewardW = 220.f, RewardH = 170.f;
FVector2f CellPx(float X, float Y) { return FVector2f(FX + X * CS, FY + Y * CS); } // 칸 좌표(실수) → 화면
FVector2f CellCenter(int Key) { return CellPx(Key % Cols + 0.5f, Key / Cols + 0.5f); }
// 조커 합성 버튼: 묶음 맨 위 칸(같으면 왼쪽)의 위 변 가운데
constexpr float JokerBtnW = 84.f, JokerBtnH = 18.f;
FVector2f JokerButtonCenter(const FPDJokerGroup& G)
{
	int Top = G.Cells[0];
	for (int K : G.Cells) if (K / Cols < Top / Cols || (K / Cols == Top / Cols && K % Cols < Top % Cols)) Top = K;
	return CellPx(Top % Cols + 0.5f, (float)(Top / Cols)) - FVector2f(0.f, 2.f);
}
// 합성 버튼: 족보 맨 아래 칸(같으면 왼쪽)의 아래 변 가운데
constexpr float FuseW = 46.f, FuseH = 16.f;
// 겹 번호: 카드를 나눠 쓰는 앞선 족보 수 (0~2). 테두리를 안쪽으로 들이고 이름표·합성 버튼을 아래로 민다
int SynLayer(const std::vector<FPDSynergy>& L, int I)
{
	int N = 0;
	for (int J = 0; J < I && J < (int)L.size(); ++J)
	{
		bool bShare = false;
		for (int A : L[I].Cells) bShare = bShare || std::find(L[J].Cells.begin(), L[J].Cells.end(), A) != L[J].Cells.end();
		N += bShare;
	}
	return FMath::Min(N, 2);
}
FVector2f FuseButtonCenter(const std::vector<FPDSynergy>& L, int I)
{
	const FPDSynergy& S = L[I];
	int Bottom = S.Cells[0];
	for (int K : S.Cells) if (K / Cols > Bottom / Cols || (K / Cols == Bottom / Cols && K % Cols < Bottom % Cols)) Bottom = K;
	return CellPx(Bottom % Cols + 0.5f, Bottom / Cols + 1.f) - FVector2f(0.f, 4.f) + FVector2f(0.f, SynLayer(L, I) * (FuseH + 2.f));
}

// 먹이기 버튼: 합성 버튼 오른쪽 (합성 버튼이 없으면 그 자리)
FVector2f FeedButtonCenter(const std::vector<FPDSynergy>& L, int I, bool bCanFuse)
{
	return FuseButtonCenter(L, I) + FVector2f(bCanFuse ? FuseW + 4.f : 0.f, 0.f);
}

// ---- 스킬 트리 글자와 위치 (docs/16-skill-tree.md). 위치는 생성기가 화면 좌표로 넣어 둔다
FVector2f TreeNodePos(int Id) { return FVector2f(TreeNode(Id).X, TreeNode(Id).Y); }
const TCHAR* KeystoneName(int K)
{
	static const TCHAR* N[] = {TEXT("외골수"), TEXT("잡식가"), TEXT("큰 판"), TEXT("작은 판"), TEXT("대장장이"), TEXT("광대의 길"), TEXT("고독한 저격수"), TEXT("군중")};
	return N[K];
}
const TCHAR* KeystoneDesc(int K)
{
	static const TCHAR* N[] = {TEXT("플러시 타워 위력 ×2 / 다른 족보 타워 위력 ×0.5"), TEXT("플러시 아닌 족보 위력 ×1.3 / 플러시 범위 효과 없음"),
		TEXT("5장 족보 위력 ×1.6 / 2장 족보는 효과·연쇄 없음"), TEXT("원 페어·투 페어 위력 ×1.8 / 5장 족보는 연쇄 덤 없음"),
		TEXT("합성 배율 1.5 → 2.0 / 조커 포탑을 만들 수 없음"), TEXT("조커 배율 1.2 → 1.6 / 합성은 레벨 2까지"),
		TEXT("족보에 안 든 타워 위력 ×1.6 / 연쇄 덤 없음"), TEXT("연쇄 덤 +25% → +40% / 족보에 안 든 타워 위력 ×0.5")};
	return N[K];
}
const TCHAR* JokerUpDesc(int K)
{
	static const TCHAR* N[] = {TEXT("연사 +50%"), TEXT("오라 공격력 +25% → +50%, 범위 주변 24칸"), TEXT("따라 한 공격 위력 ×1.3"), TEXT("셋째 적까지 함께 공격"),
		TEXT("같은 무늬 공격력 +30% → +60%"), TEXT("주변 연사 +40% → +70%"), TEXT("공격력 ×3 → ×5"), TEXT("위력 ×1.5"), TEXT("5초마다 2초 멈춤 (원래 8초마다 1.5초)"),
		TEXT("레이저 위력 ×0.5 → ×0.8")};
	return N[K];
}
FString HandGroupName(int Mask)
{
	auto B = [](EPDHand H) { return 1 << (int)H; };
	if (Mask == (B(EPDHand::Pair) | B(EPDHand::TwoPair))) return TEXT("원 페어·투 페어");
	if (Mask == (B(EPDHand::Triple) | B(EPDHand::Four) | B(EPDHand::Five))) return TEXT("트리플·포카드·파이브 카드");
	if (Mask == (B(EPDHand::Flush) | B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return TEXT("플러시 계열");
	if (Mask == (B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return TEXT("스트레이트 플러시·로열");
	if (Mask == (B(EPDHand::Straight) | B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return TEXT("스트레이트 계열");
	if (Mask == (B(EPDHand::Pair) | B(EPDHand::TwoPair) | B(EPDHand::Triple) | B(EPDHand::FullHouse) | B(EPDHand::Four) | B(EPDHand::Five))) return TEXT("같은 숫자 족보");
	FString Out;
	for (int H = 0; H < (int)EPDHand::Count; ++H) if (Mask & (1 << H)) Out += (Out.IsEmpty() ? TEXT("") : TEXT("·")) + FString(HandName((EPDHand)H));
	return Out;
}
FString TreeFxText(const FPDNodeFx& F)
{
	static const TCHAR* Sym[] = {TEXT("♠"), TEXT("♥"), TEXT("♦"), TEXT("♣"), TEXT("★")};
	const int Pct = FMath::RoundToInt(F.Value * 100.f);
	switch (F.Stat)
	{
	case EPDStat::SuitDmg: return FString::Printf(TEXT("%s 위력 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitRate: return FString::Printf(TEXT("%s 연사 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitRange: return FString::Printf(TEXT("%s 사거리 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitSplash: return FString::Printf(TEXT("%s 폭발 범위 +%.1f칸"), Sym[F.Param], F.Value);
	case EPDStat::SuitSlow: return FString::Printf(TEXT("%s 둔화 +%d%%p"), Sym[F.Param], Pct);
	case EPDStat::HandDmg: return FString::Printf(TEXT("%s 위력 +%d%%"), *HandGroupName(F.Param), Pct);
	case EPDStat::HandRate: return FString::Printf(TEXT("%s 연사 +%d%%"), *HandGroupName(F.Param), Pct);
	case EPDStat::HandSplash: return FString::Printf(TEXT("%s 범위 +%.2f칸"), *HandGroupName(F.Param), F.Value);
	case EPDStat::AllDmg: return FString::Printf(TEXT("모든 타워 위력 +%d%%"), Pct);
	case EPDStat::ChainAdd: return FString::Printf(TEXT("연쇄 덤 +%d%%p"), Pct);
	case EPDStat::FuseAdd: return FString::Printf(TEXT("합성 배율 +%.2f"), F.Value);
	case EPDStat::FusedDmg: return FString::Printf(TEXT("합성 타워 위력 +%d%%"), Pct);
	case EPDStat::JokerDmg: return FString::Printf(TEXT("조커 포탑 위력 +%d%%"), Pct);
	case EPDStat::JokerAdd: return FString::Printf(TEXT("조커 배율 +%.2f"), F.Value);
	case EPDStat::AllRate: return FString::Printf(TEXT("모든 타워 공격 속도 +%d%%"), Pct);
	case EPDStat::CritChance: return FString::Printf(TEXT("치명타 확률 +%d%%p"), Pct);
	case EPDStat::CritMult: return FString::Printf(TEXT("치명타 배율 +%.2f"), F.Value);
	case EPDStat::GoldPct: return FString::Printf(TEXT("골드 +%d%%"), Pct);
	case EPDStat::HandMax: return FString::Printf(TEXT("손패 최대 +%d"), (int)F.Value);
	case EPDStat::Draw: return FString::Printf(TEXT("웨이브마다 카드 +%d"), (int)F.Value);
	case EPDStat::Lives: return FString::Printf(TEXT("목숨 +%d (찍을 때)"), (int)F.Value);
	case EPDStat::Keystone: return KeystoneDesc(F.Param);
	case EPDStat::Joker: return JokerUpDesc(F.Param);
	default: return FString();
	}
}
FString TreeNodeName(int Id)
{
	const FPDTreeNode& N = TreeNode(Id);
	switch (N.Kind)
	{
	case EPDNode::Start: return TEXT("시작");
	case EPDNode::Keystone: return FString::Printf(TEXT("키스톤: %s"), KeystoneName(N.Fx[0].Param));
	case EPDNode::Joker: return FString::Printf(TEXT("조커 강화: %s"), JokerName(N.Fx[0].Param));
	case EPDNode::Notable: return GPDTreeNames[Id] ? GPDTreeNames[Id] : TEXT("특급 노드");
	default: return N.Theme == 6 ? FString(TEXT("조커 갈림길")) : N.Theme == 5 ? FString(TEXT("살림")) : FString(TEXT("작은 노드"));
	}
}
FLinearColor TreeNodeColor(int Id)
{
	const FPDTreeNode& N = TreeNode(Id);
	if (N.Kind == EPDNode::Keystone) return Hex(TEXT("ffd26e"));
	if (N.Theme < 4) return SuitColor((EPDSuit)N.Theme);
	if (N.Theme == 4) return Hex(TEXT("7fd6ff"));
	if (N.Theme == 6) return JokerPink;
	return Hex(TEXT("e8f0ff"));
}
// ---- 골드 강화 (U)
const TCHAR* UpgradeName(int U)
{
	static const TCHAR* N[] = {TEXT("공격"), TEXT("공격 속도"), TEXT("치명타 확률"), TEXT("치명타 배율"), TEXT("♠ 스페이드 강화"), TEXT("♥ 하트 강화"), TEXT("♦ 다이아 강화"), TEXT("♣ 클로버 강화")};
	return N[U];
}
FString UpgradeEffect(int U, int Level)
{
	const float V = UpgradeStep((EPDUpgrade)U) * Level;
	switch ((EPDUpgrade)U)
	{
	case EPDUpgrade::Attack: return FString::Printf(TEXT("모든 타워 공격력 +%.0f%%  (레벨당 +1%%)"), V * 100.f);
	case EPDUpgrade::Speed: return FString::Printf(TEXT("모든 타워 공격 속도 +%.0f%%  (레벨당 +1%%)"), V * 100.f);
	case EPDUpgrade::Crit: return FString::Printf(TEXT("치명타 확률 %.1f%%  (레벨당 +0.1%%p, 100%% 넘으면 한 방에 여러 번)"), (BaseCrit + V) * 100.f);
	case EPDUpgrade::CritMult: return FString::Printf(TEXT("치명타 배율 ×%.2f  (레벨당 +0.01)"), BaseCritMult + V);
	default: return FString::Printf(TEXT("그 무늬 타워 공격력 +%.0f%%  (레벨당 +2%%)"), V * 100.f);
	}
}
FLinearColor UpgradeColor(int U) { return U >= (int)EPDUpgrade::Spade ? SuitColor((EPDSuit)(U - (int)EPDUpgrade::Spade)) : U >= (int)EPDUpgrade::Crit ? Hex(TEXT("ff9a6a")) : Hex(TEXT("ffd96a")); }
constexpr float UpgRowX = 150.f, UpgRowY = 120.f, UpgRowW = 980.f, UpgRowH = 64.f;
constexpr float UpgBtnW = 76.f, UpgBtnH = 30.f;
const int32 UpgSteps[4] = {1, 10, 100, UpgradeMax};
FVector2f UpgButtonPos(int Row, int K) { return FVector2f(UpgRowX + UpgRowW - 4 * (UpgBtnW + 8.f) + K * (UpgBtnW + 8.f), UpgRowY + Row * UpgRowH + (UpgRowH - UpgBtnH) / 2.f); }
// 트리·강화 버튼 (오른쪽 패널, 웨이브 시작 버튼 아래 나란히), 메뉴 버튼 (그 아래)
constexpr float TreeBtnX = RBtnX, TreeBtnY = StartBtnY + StartBtnH + 10.f, TreeBtnW = (RBtnW - 8.f) / 2.f, TreeBtnH = 30.f;
constexpr float UpgOpenX = RBtnX + TreeBtnW + 8.f, UpgOpenY = TreeBtnY, UpgOpenW = TreeBtnW, UpgOpenH = 30.f;
constexpr float MenuBtnX = RBtnX, MenuBtnY = TreeBtnY + TreeBtnH + 8.f, MenuBtnW = RBtnW, MenuBtnH = 28.f;
constexpr float RTopH = MenuBtnY + MenuBtnH + 12.f - FY; // 오른쪽 위 패널 높이
// 메뉴 창 (Esc): 가운데 세로 버튼
enum class EMenuItem : uint8_t { Resume, NewGame, Quit, Count };
constexpr float MenuW = 300.f, MenuItemH = 44.f, MenuGap = 12.f;
constexpr float MenuX = DesignW / 2.f - MenuW / 2.f, MenuItemY0 = 300.f;
FVector2f MenuItemPos(int I) { return FVector2f(MenuX, MenuItemY0 + I * (MenuItemH + MenuGap)); }
const TCHAR* MenuItemName(int I)
{
	static const TCHAR* N[] = {TEXT("계속하기"), TEXT("새 게임"), TEXT("게임 종료")};
	return N[I];
}
// 손패 카드 왼쪽 X. 카드가 많으면 판 너비 안에 들어오게 간격을 좁힌다
float HandCardX(int I, int N)
{
	const float Avail = Cols * CS - 96.f - CardW;
	const float Step = N > 1 ? FMath::Min(CardW + 10.f, Avail / (N - 1)) : 0.f;
	return FX + 96.f + I * Step;
}
bool InRect(FVector2f P, float X, float Y, float W, float H) { return P.X >= X && P.X <= X + W && P.Y >= Y && P.Y <= Y + H; }
} // namespace PDDraw

using namespace PDDraw;

// ---------------------------------------------------------------- 그리기 도우미
struct SPDBoardWidget::FCanvas
{
	FSlateWindowElementList& Out;
	FGeometry Geo;
	int32* Layer;
	const FSlateBrush* White;

	int32 NextLayer() { return ++(*Layer); }

	void Lines(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick)
	{
		FSlateDrawElement::MakeLines(Out, NextLayer(), Geo.ToPaintGeometry(), P, ESlateDrawEffect::None, Col, true, Thick);
	}
	// 네온 선 (docs/15-art-style.md): 아주 넓고 옅은 번짐 세 겹 + 가늘고 또렷한 심
	// 웹 캔버스의 shadowBlur 처럼 부드럽게 퍼지는 빛을 흉내 낸다
	void Glow(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Strength = 1.f)
	{
		if (Strength > 0.f)
		{
			Lines(P, WithAlpha(Col, 0.04f * Strength), Thick * 10.f);
			Lines(P, WithAlpha(Col, 0.07f * Strength), Thick * 6.f);
			Lines(P, WithAlpha(Col, 0.14f * Strength), Thick * 3.2f);
		}
		Lines(P, Col, Thick * 0.8f);
	}
	void Dashed(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Dash = 6.f, float Gap = 6.f)
	{
		bool bOn = true;
		float Left = Dash;
		for (int32 I = 0; I + 1 < P.Num(); ++I)
		{
			FVector2f A = P[I];
			const FVector2f B = P[I + 1];
			float SegLen = FVector2f::Distance(A, B);
			const FVector2f Dir = SegLen > 0.f ? (B - A) / SegLen : FVector2f::ZeroVector;
			while (SegLen > 0.f)
			{
				const float Step = FMath::Min(Left, SegLen);
				const FVector2f E = A + Dir * Step;
				if (bOn) Lines({A, E}, Col, Thick);
				A = E;
				SegLen -= Step;
				Left -= Step;
				if (Left <= 0.f)
				{
					bOn = !bOn;
					Left = bOn ? Dash : Gap;
				}
			}
		}
	}
	void Box(float X, float Y, float W, float H, const FLinearColor& Col)
	{
		BoxAt(NextLayer(), X, Y, W, H, Col);
	}
	// 같은 층에 여러 개를 그리면 한 번에 묶여 빠르다 (파티클)
	void BoxAt(int32 L, float X, float Y, float W, float H, const FLinearColor& Col)
	{
		FSlateDrawElement::MakeBox(Out, L, Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), White, ESlateDrawEffect::None, Col);
	}
	static TArray<FVector2f> RectPoints(float X, float Y, float W, float H) { return {{X, Y}, {X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X, Y}}; }
	static TArray<FVector2f> CirclePoints(FVector2f C, float R, int32 Seg = 32)
	{
		TArray<FVector2f> P;
		for (int32 I = 0; I <= Seg; ++I)
		{
			const float A = I / (float)Seg * TwoPi;
			P.Add(C + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R);
		}
		return P;
	}
	static TArray<FVector2f> RoundRectPoints(float X, float Y, float W, float H, float R)
	{
		TArray<FVector2f> P;
		const FVector2f Centers[4] = {{X + W - R, Y + R}, {X + W - R, Y + H - R}, {X + R, Y + H - R}, {X + R, Y + R}};
		const float Start[4] = {-0.25f, 0.f, 0.25f, 0.5f};
		for (int32 K = 0; K < 4; ++K)
			for (int32 S = 0; S <= 5; ++S)
			{
				const float A = (Start[K] + S / 5.f * 0.25f) * TwoPi;
				P.Add(Centers[K] + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R);
			}
		const FVector2f First = P[0]; // 같은 배열 원소를 Add 하면 재할당 중 잘못된 참조가 된다
		P.Add(First);
		return P;
	}
	static FSlateFontInfo Font(float Px, bool bBold)
	{
		// Slate 글자 크기는 포인트(96 DPI) 단위. 웹 캔버스 px 크기 x 0.75
		return FCoreStyle::GetDefaultFontStyle(bBold ? "Bold" : "Regular", FMath::Max(1, FMath::RoundToInt(Px * 0.75f)));
	}
	static FVector2f Measure(const FString& S, const FSlateFontInfo& F)
	{
		const TSharedRef<FSlateFontMeasure> M = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
		return FVector2f(M->Measure(S, F));
	}
	// Y는 웹 캔버스처럼 글자 기준선. Align: 0 왼쪽, 1 가운데, 2 오른쪽
	// 칸 너비 MaxW 를 넘으면 글씨를 줄이고(70%까지), 그래도 넘치면 끝을 "…"로 자른다
	static void FitText(FString& S, FSlateFontInfo& F, FVector2f& Sz, float Px, bool bBold, float MaxW)
	{
		if (MaxW <= 0.f || Sz.X <= MaxW) return;
		F = Font(FMath::Max(Px * 0.7f, Px * MaxW / Sz.X), bBold);
		Sz = Measure(S, F);
		FString Base = S;
		while (Sz.X > MaxW && Base.Len() > 1)
		{
			Base.LeftChopInline(1);
			Base.TrimEndInline();
			S = Base + TEXT("…");
			Sz = Measure(S, F);
		}
	}
	void Text(const FString& InS, float X, float Y, const FLinearColor& Col, float Px = 13.f, int32 Align = 0, bool bBold = false, float MaxW = 0.f)
	{
		FString S = InS;
		FSlateFontInfo F = Font(Px, bBold);
		FVector2f Sz = Measure(S, F);
		FitText(S, F, Sz, Px, bBold, MaxW);
		const float Left = Align == 1 ? X - Sz.X / 2.f : Align == 2 ? X - Sz.X : X;
		const float Top = Y - Sz.Y * 0.8f;
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(Left, Top))), S, F, ESlateDrawEffect::None, Col);
	}
	void TextCentered(const FString& InS, float CX, float CY, const FLinearColor& Col, float Px, bool bBold, float MaxW = 0.f)
	{
		FString S = InS;
		FSlateFontInfo F = Font(Px, bBold);
		FVector2f Sz = Measure(S, F);
		FitText(S, F, Sz, Px, bBold, MaxW);
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(CX - Sz.X / 2.f, CY - Sz.Y / 2.f))), S, F,
			ESlateDrawEffect::None, Col);
	}
	void Panel(float X, float Y, float W, float H) { Lines(RectPoints(X, Y, W, H), Hex(TEXT("7fd6ff"), 0.3f), 1.f); }

	// 무늬 도형: ♠ 삼각형, ♥ 원, ♦ 마름모, ♣ 사각형
	// 하트 곡선 (Flip -1 이면 뒤집어서 스페이드 몸통)
	static TArray<FVector2f> HeartPoints(float CX, float CY, float R, float Flip, float OffY)
	{
		TArray<FVector2f> P;
		for (int32 I = 0; I <= 48; ++I)
		{
			const float T = I / 48.f * TwoPi, St = FMath::Sin(T);
			const float HX = 16.f * St * St * St;
			const float HY = -(13.f * FMath::Cos(T) - 5.f * FMath::Cos(2.f * T) - 2.f * FMath::Cos(3.f * T) - FMath::Cos(4.f * T));
			P.Add(FVector2f(CX + HX / 17.f * R, CY + (HY / 17.f - 0.15f) * R * Flip + OffY));
		}
		return P;
	}
	// 클로버 잎 세 개의 바깥 테두리 하나 (가운데 숫자 위로 원 선이 지나가지 않게)
	static TArray<FVector2f> ClubPoints(float CX, float CY, float R)
	{
		const FVector2f O(CX, CY - 0.06f * R);
		const FVector2f Cs[3] = {FVector2f(CX, CY - 0.44f * R), FVector2f(CX - 0.46f * R, CY + 0.1f * R), FVector2f(CX + 0.46f * R, CY + 0.1f * R)};
		const float Rad = 0.4f * R;
		TArray<FVector2f> P;
		for (int32 I = 0; I <= 72; ++I)
		{
			const float A = I / 72.f * TwoPi;
			const FVector2f U(FMath::Cos(A), FMath::Sin(A));
			float Far = 0.f;
			for (const FVector2f& C0 : Cs)
			{
				const FVector2f D = C0 - O;
				const float B = D.X * U.X + D.Y * U.Y, Disc = B * B - (D.X * D.X + D.Y * D.Y - Rad * Rad);
				if (Disc >= 0.f) Far = FMath::Max(Far, B + FMath::Sqrt(Disc));
			}
			P.Add(O + U * Far);
		}
		return P;
	}
	// 무늬 도형: 실제 무늬 모양 (♠ ♥ ♦ ♣), 와일드는 별
	void Shape(EPDSuit S, float CX, float CY, float R, const FLinearColor& Col, float Thick, float GlowK)
	{
		switch (S)
		{
		case EPDSuit::Spade:
			Glow(HeartPoints(CX, CY, R, -1.f, -R * 0.12f), Col, Thick, GlowK);
			Glow({{CX, CY + R * 0.42f}, {CX + R * 0.3f, CY + R}, {CX - R * 0.3f, CY + R}, {CX, CY + R * 0.42f}}, Col, Thick, GlowK);
			break;
		case EPDSuit::Heart: Glow(HeartPoints(CX, CY, R, 1.f, 0.f), Col, Thick, GlowK); break;
		case EPDSuit::Diamond: Glow({{CX, CY - R}, {CX + R * 0.72f, CY}, {CX, CY + R}, {CX - R * 0.72f, CY}, {CX, CY - R}}, Col, Thick, GlowK); break;
		case EPDSuit::Club:
			Glow(ClubPoints(CX, CY, R), Col, Thick, GlowK);
			Glow({{CX, CY + R * 0.5f}, {CX + R * 0.28f, CY + R}, {CX - R * 0.28f, CY + R}, {CX, CY + R * 0.5f}}, Col, Thick, GlowK);
			break;
		case EPDSuit::Wild:
		{
			TArray<FVector2f> Star;
			for (int32 K = 0; K <= 10; ++K) { const float A = -TwoPi / 4.f + K * TwoPi / 10.f, Rr = K % 2 ? R * 0.45f : R; Star.Add(FVector2f(CX + FMath::Cos(A) * Rr, CY + FMath::Sin(A) * Rr)); }
			Glow(Star, Col, Thick, GlowK);
			break;
		}
		}
	}
	// 카드(=타워): 무늬 도형 안에 숫자
	// 카드(=타워): 둥근 사각 틀 안에 숫자와 작은 무늬 기호 (테트리스 디펜스 포탑과 같은 방식)
	// bFrame=false 면 틀 없이 안쪽만 (손패 카드는 카드 테두리가 틀 역할)
	void CardShape(const FPDCard& Card, float CX, float CY, float R, float Alpha, bool bStrong, float Pulse = 0.f, bool bFrame = true)
	{
		const FLinearColor Col = WithAlpha(SuitColor(Card.Suit), Alpha);
		if (bFrame)
		{
			const float H = R * 0.98f;
			if (bStrong) Box(CX - H, CY - H, H * 2.f, H * 2.f, WithAlpha(Col, 0.08f + 0.05f * Pulse)); // 족보에 든 타워는 옅게 차오름
			Glow(RoundRectPoints(CX - H, CY - H, H * 2.f, H * 2.f, R * 0.22f), Col, bStrong ? 2.f : 1.5f, bStrong ? 1.3f + 0.4f * Pulse : 0.8f);
		}
		if (Card.IsWild()) { Shape(Card.Suit, CX, CY, R * 0.6f, Col, 1.6f, 0.9f); return; }
		TextCentered(RankText(Card.Rank), CX, CY - R * 0.24f, Col, R * (Card.Rank == 10 ? 0.62f : 0.74f), true);
		Shape(Card.Suit, CX, CY + R * 0.5f, R * 0.3f, Col, 1.4f, 0.8f);
	}
	// ---- 조커 그림 10종 (docs/15-art-style.md 9절, tools/neon/neon.py 와 같은 좌표)
	static void AddBez(TArray<FVector2f>& Out, FVector2f A, FVector2f B, FVector2f C, int32 N = 14)
	{
		for (int32 I = Out.Num() ? 1 : 0; I <= N; ++I)
		{
			const float T = I / (float)N, U = 1.f - T;
			Out.Add(A * (U * U) + B * (2.f * U * T) + C * (T * T));
		}
	}
	static TArray<FVector2f> Ellipse(FVector2f C, float R, float Sx = 1.f, int32 Seg = 24)
	{
		TArray<FVector2f> P;
		for (int32 I = 0; I <= Seg; ++I) { const float A = I / (float)Seg * TwoPi; P.Add(C + FVector2f(FMath::Cos(A) * R * Sx, FMath::Sin(A) * R)); }
		return P;
	}
	void JesterHat(float CX, float CY, float R, const FLinearColor& Col, float Th, float Gk)
	{
		auto P = [&](float X, float Y) { return FVector2f(CX + X * R, CY + Y * R); };
		const float By = -0.12f;
		TArray<FVector2f> Hat;
		AddBez(Hat, P(-0.5f, By), P(-0.95f, -0.95f), P(-1.f, -0.22f));
		AddBez(Hat, P(-1.f, -0.22f), P(-0.6f, -0.5f), P(-0.22f, By - 0.22f));
		AddBez(Hat, P(-0.22f, By - 0.22f), P(-0.2f, -0.75f), P(0.f, -1.05f));
		AddBez(Hat, P(0.f, -1.05f), P(0.2f, -0.75f), P(0.22f, By - 0.22f));
		AddBez(Hat, P(0.22f, By - 0.22f), P(0.6f, -0.5f), P(1.f, -0.22f));
		AddBez(Hat, P(1.f, -0.22f), P(0.95f, -0.95f), P(0.5f, By));
		AddBez(Hat, P(0.5f, By), P(0.f, By + 0.14f), P(-0.5f, By));
		Glow(Hat, Col, Th, Gk);
		const FVector2f Tips[3] = {P(-1.f, -0.12f), P(0.f, -0.95f), P(1.f, -0.12f)};
		for (const FVector2f& T0 : Tips) Glow(Ellipse(T0, 0.1f * R, 1.f, 14), Col, Th, Gk);
	}
	void JesterHead(float CX, float CY, float R, const FLinearColor& Col, float Th, float Gk)
	{
		auto P = [&](float X, float Y) { return FVector2f(CX + X * R, CY + Y * R); };
		JesterHat(CX, CY, R, Col, Th, Gk);
		Glow(Ellipse(P(0.f, 0.3f), 0.34f * R, 0.92f, 28), Col, Th, Gk);
		for (float Ex : {-0.13f, 0.13f}) { TArray<FVector2f> E; AddBez(E, P(Ex - 0.07f, 0.26f), P(Ex, 0.17f), P(Ex + 0.07f, 0.26f), 6); Glow(E, Col, Th, Gk); }
		TArray<FVector2f> M;
		AddBez(M, P(-0.16f, 0.4f), P(0.f, 0.57f), P(0.16f, 0.4f), 8);
		Glow(M, Col, Th, Gk);
		TArray<FVector2f> Z;
		for (int32 I = 0; I <= 10; ++I) Z.Add(P(-0.55f + I * 0.11f, 0.6f + (I % 2 ? 0.24f : 0.f)));
		Glow(Z, Col, Th, Gk);
	}
	// K = 조커 종류(0~9), 틀 반 너비 R 안에 그린다
	void JokerArt(int32 K, float CX, float CY, float R, const FLinearColor& Col, float Th, float Gk)
	{
		auto P = [&](float X, float Y) { return FVector2f(CX + X * R, CY + Y * R); };
		switch (K)
		{
		case 0: JesterHead(CX, CY + 0.04f * R, R * 0.6f, Col, Th, Gk); break;          // 광대
		case 1: JesterHat(CX, CY + 0.38f * R, R * 0.72f, Col, Th, Gk); break;          // 모자 장수
		case 2:                                                                          // 웃는 가면
		{
			TArray<FVector2f> Mask;
			AddBez(Mask, P(-0.68f, -0.12f), P(0.f, -0.52f), P(0.68f, -0.12f));
			AddBez(Mask, P(0.68f, -0.12f), P(0.35f, 0.22f), P(0.f, 0.02f));
			AddBez(Mask, P(0.f, 0.02f), P(-0.35f, 0.22f), P(-0.68f, -0.12f));
			Glow(Mask, Col, Th, Gk);
			for (float Sx : {-1.f, 1.f})
			{
				TArray<FVector2f> E;
				AddBez(E, P(Sx * 0.45f, -0.12f), P(Sx * 0.3f, -0.26f), P(Sx * 0.15f, -0.12f), 6);
				AddBez(E, P(Sx * 0.15f, -0.12f), P(Sx * 0.3f, -0.04f), P(Sx * 0.45f, -0.12f), 6);
				Glow(E, Col, Th, Gk);
				Glow({P(Sx * 0.68f, -0.12f), P(Sx * 0.82f, 0.12f)}, Col, Th, Gk);
				Glow({P(Sx * 0.68f, -0.12f), P(Sx * 0.88f, -0.02f)}, Col, Th, Gk);
			}
			TArray<FVector2f> Sm;
			AddBez(Sm, P(-0.3f, 0.32f), P(0.f, 0.6f), P(0.3f, 0.32f), 10);
			Glow(Sm, Col, Th, Gk);
			break;
		}
		case 3:                                                                          // 희비극
		{
			Glow(Ellipse(P(-0.24f, -0.1f), 0.38f * R, 1.f, 28), Col, Th, Gk);
			Glow(Ellipse(P(0.26f, 0.14f), 0.38f * R, 1.f, 28), Col, Th, Gk);
			const float Eyes[4][2] = {{-0.38f, -0.2f}, {-0.18f, -0.2f}, {0.14f, 0.06f}, {0.32f, 0.06f}};
			for (int32 I = 0; I < 4; ++I) { TArray<FVector2f> E; AddBez(E, P(Eyes[I][0], Eyes[I][1]), P(Eyes[I][0] + 0.06f, Eyes[I][1] - 0.08f), P(Eyes[I][0] + 0.12f, Eyes[I][1]), 5); Glow(E, Col, Th, Gk); }
			TArray<FVector2f> Happy, Sad;
			AddBez(Happy, P(-0.42f, 0.f), P(-0.24f, 0.18f), P(-0.06f, 0.f), 8);
			AddBez(Sad, P(0.12f, 0.34f), P(0.28f, 0.18f), P(0.44f, 0.34f), 8);
			Glow(Happy, Col, Th, Gk);
			Glow(Sad, Col, Th, Gk);
			break;
		}
		case 4:                                                                          // 할리퀸
		{
			auto Dia = [&](float X, float Y, float W, float H) { return TArray<FVector2f>{P(X, Y - H), P(X + W, Y), P(X, Y + H), P(X - W, Y), P(X, Y - H)}; };
			Glow(Dia(0.f, 0.02f, 0.24f, 0.42f), Col, Th, Gk);
			Glow(Dia(-0.42f, 0.08f, 0.17f, 0.3f), Col, Th, Gk);
			Glow(Dia(0.42f, 0.08f, 0.17f, 0.3f), Col, Th, Gk);
			Glow(Ellipse(P(0.f, -0.52f), 0.08f * R, 1.f, 12), Col, Th, Gk);
			Glow(Ellipse(P(-0.42f, -0.34f), 0.07f * R, 1.f, 12), Col, Th, Gk);
			Glow(Ellipse(P(0.42f, -0.34f), 0.07f * R, 1.f, 12), Col, Th, Gk);
			break;
		}
		case 5:                                                                          // 지휘봉
			Glow({P(0.f, -0.05f), P(0.f, 0.72f)}, Col, Th, Gk);
			Glow({P(-0.12f, 0.72f), P(0.12f, 0.72f)}, Col, Th, Gk);
			JesterHead(CX, CY - 0.32f * R, R * 0.36f, Col, Th, Gk);
			break;
		case 6:                                                                          // 잭
			JesterHat(CX, CY - 0.22f * R, R * 0.48f, Col, Th, Gk);
			TextCentered(TEXT("J"), CX, CY + 0.26f * R, Col, R * 0.78f, true);
			break;
		case 7:                                                                          // 체셔
		{
			TArray<FVector2f> Grin;
			AddBez(Grin, P(-0.66f, 0.f), P(0.f, 0.62f), P(0.66f, 0.f));
			AddBez(Grin, P(0.66f, 0.f), P(0.f, 0.3f), P(-0.66f, 0.f));
			Glow(Grin, Col, Th, Gk);
			for (float X : {-0.34f, -0.17f, 0.f, 0.17f, 0.34f}) Glow({P(X, 0.13f), P(X, 0.27f - FMath::Abs(X) * 0.25f)}, Col, Th, Gk);
			for (float Sx : {-1.f, 1.f}) { TArray<FVector2f> E; AddBez(E, P(Sx * 0.42f, -0.32f), P(Sx * 0.28f, -0.46f), P(Sx * 0.14f, -0.32f), 6); Glow(E, Col, Th, Gk); }
			break;
		}
		case 8:                                                                          // 방울
		{
			TArray<FVector2f> Bell, Rim, Tip;
			AddBez(Bell, P(-0.42f, 0.34f), P(-0.38f, -0.36f), P(0.f, -0.36f));
			AddBez(Bell, P(0.f, -0.36f), P(0.38f, -0.36f), P(0.42f, 0.34f));
			AddBez(Rim, P(-0.52f, 0.34f), P(0.f, 0.48f), P(0.52f, 0.34f));
			AddBez(Tip, P(0.f, -0.36f), P(0.05f, -0.62f), P(0.34f, -0.62f), 8);
			Glow(Bell, Col, Th, Gk);
			Glow(Rim, Col, Th, Gk);
			Glow({P(-0.52f, 0.34f), P(0.52f, 0.34f)}, Col, Th, Gk);
			Glow(Ellipse(P(0.f, 0.52f), 0.08f * R, 1.f, 12), Col, Th, Gk);
			Glow(Tip, Col, Th, Gk);
			Glow(Ellipse(P(0.42f, -0.6f), 0.07f * R, 1.f, 12), Col, Th, Gk);
			break;
		}
		default:                                                                         // 스타
			Shape(EPDSuit::Wild, CX, CY + 0.22f * R, R * 0.52f, Col, Th, Gk);
			JesterHat(CX, CY - 0.24f * R, R * 0.4f, Col, Th, Gk);
			break;
		}
	}
	// 조커 포탑: 분홍 둥근 사각 틀 + 조커 그림
	void JokerTower(int32 K, float CX, float CY, float R, float Alpha, float Pulse)
	{
		const FLinearColor Col = WithAlpha(JokerPink, Alpha);
		const float H = R * 0.98f;
		Box(CX - H, CY - H, H * 2.f, H * 2.f, WithAlpha(Col, 0.08f + 0.06f * Pulse));
		Glow(RoundRectPoints(CX - H, CY - H, H * 2.f, H * 2.f, R * 0.22f), Col, 2.f, 1.4f + 0.5f * Pulse);
		JokerArt(K, CX, CY, R, Col, 1.3f, 0.8f);
	}
	void Badge(const FString& S, float CX, float CY, const FLinearColor& Col, float Alpha)
	{
		const FSlateFontInfo F = Font(11.f, true);
		const FVector2f Sz = Measure(S, F);
		const float W = Sz.X + 10.f, H = 16.f;
		Box(CX - W / 2.f, CY - H / 2.f, W, H, Hex(TEXT("06060b"), 0.88f * Alpha));
		Lines(RectPoints(CX - W / 2.f, CY - H / 2.f, W, H), WithAlpha(Col, Alpha), 1.f);
		TextCentered(S, CX, CY, WithAlpha(Col, Alpha), 11.f, true);
	}
	void Overlay(const FString& Title, const FLinearColor& Col, const TArray<FString>& Body, const FString& Foot)
	{
		Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.86f));
		Text(Title, DesignW / 2.f, 240.f, Col, 34.f, 1, true);
		for (int32 I = 0; I < Body.Num(); ++I) Text(Body[I], DesignW / 2.f, 290.f + I * 26.f, Hex(TEXT("cfd8e6")), 14.f, 1);
		Text(Foot, DesignW / 2.f, 300.f + Body.Num() * 26.f + 50.f, Hex(TEXT("ffd96a")), 18.f, 1);
	}
};

// ---------------------------------------------------------------- 위젯
void SPDBoardWidget::Construct(const FArguments& InArgs)
{
	SetCanTick(true);
}

void SPDBoardWidget::Burst(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak, float Gravity)
{
	for (int I = 0; I < N && Particles.Num() < 2600; ++I)
	{
		const float A = FMath::FRandRange(0.f, TwoPi), Sp = FMath::FRandRange(SpeedMin, SpeedMax);
		FParticle Pa;
		Pa.P = P;
		Pa.V = FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp;
		Pa.MaxLife = Pa.Life = Life * FMath::FRandRange(0.6f, 1.2f);
		Pa.Size = Size * FMath::FRandRange(0.7f, 1.3f);
		Pa.Drag = 3.f;
		Pa.Gravity = Gravity;
		Pa.Col = Col;
		Pa.bStreak = bStreak;
		Particles.Add(Pa);
	}
}

void SPDBoardWidget::HandleEvents()
{
	for (const FPDEvent& E : Game.Events)
	{
		const FVector2f P = CellPx(E.X, E.Y);
		switch (E.Kind)
		{
		case FPDEvent::EKind::Shot:
		{
			const EPDShot K = (EPDShot)(int)E.R;
			const FLinearColor Col = ShotColor(K, E.Suit);
			const int32 Lv = FMath::Clamp(E.Lv, 1, MaxLevel);
			const FVector2f To = CellPx(E.X2, E.Y2), Dir = (To - P).GetSafeNormal();
			const float W = (E.A ? 2.6f : 1.6f) + (E.B > 0 ? 1.4f : 0.f);
			// 총구: 고리 + 쏘는 쪽으로 튀는 섬광 (레벨이 높을수록 크게)
			Rings.Add({P, 4.f, 12.f + 3.f * Lv, 0.f, 0.15f, 1.5f, Col});
			Burst(P + Dir * 10.f, 1 + Lv, Col, 40.f, 110.f, 0.14f, 1.6f, true);
			PushWorldFx(EWorldFx::Muzzle, P + Dir * 10.f, Col, (float)Lv, Dir);
			switch (K) // 즉시 발사체는 쏘는 순간 빛줄기가 닿는다 (날아가는 것은 Impact 에서)
			{
			case EPDShot::Laser: Beams.Add({P, To, 0.f, 0.12f + 0.03f * Lv, W + 0.5f * (Lv - 1), Col, Lv >= 3 ? 1 : 0, Lv}); break;
			case EPDShot::JokerLaser: Beams.Add({P, To, 0.f, 0.14f, W + 1.f, Col}); break;
			case EPDShot::Arc: Beams.Add({P, To, 0.f, 0.18f, W, Col, 2, Lv}); break;
			case EPDShot::Crown: Beams.Add({P, To, 0.f, 0.3f, W + 2.f, Col, 3, Lv}); break;
			case EPDShot::Grin: Beams.Add({P, To, 0.f, 0.3f, W + 0.5f, Col, 4, Lv}); break;
			default: break;
			}
			if (ShotSpeed(K) <= 0.f)
			{
				const float Pw = Lv + (E.A ? 1.f : 0.f) + (IsJokerShot(K) || IsHandShot(K) ? 1.5f : 0.f) + FMath::Min(E.B, 3);
				ImpactFx(K, To, Dir, Col, Lv, Pw);
				HitFx(To, Pw, E.B, E.Dmg);
				if (E.B > 0) CritFx(To, E.B);
			}
			break;
		}
		case FPDEvent::EKind::Impact: // 발사체가 닿음: 종류마다 다른 타격 효과 + 공통(번쩍임·숫자)
		{
			const EPDShot K = (EPDShot)(int)E.R;
			const FLinearColor Col = ShotColor(K, E.Suit);
			const int32 Lv = FMath::Clamp(E.Lv, 1, MaxLevel);
			const FVector2f Dir = FVector2f(E.X2, E.Y2).GetSafeNormal();
			const float Pw = Lv + (E.A ? 1.f : 0.f) + (IsJokerShot(K) || IsHandShot(K) ? 1.5f : 0.f) + FMath::Min(E.B, 3);
			ImpactFx(K, P, Dir, Col, Lv, Pw);
			HitFx(P, Pw, E.B, E.Dmg);
			if (E.B > 0) CritFx(P, E.B);
			break;
		}
		case FPDEvent::EKind::Splash:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 6.f, E.R * CS, 0.f, 0.35f, 2.f, Col, EWave::Broken, 10, FMath::FRandRange(0.f, TwoPi), 2.f}); // 범위: 도는 점선 고리
			Burst(P, 10, Col, 60.f, 200.f, 0.35f, 2.2f, true);
			PushWorldFx(EWorldFx::Splash, P, Col, FMath::Max(0.5f, E.R));
			break;
		}
		case FPDEvent::EKind::Kill:
		{
			DeathFx(P, E.Enemy);
			if (EnemyDef(E.Enemy).bBoss) Floats.Add({FString::Printf(TEXT("+%d 골드"), E.A), P - FVector2f(0.f, 40.f), 0.f, 1.4f, 22.f, Hex(TEXT("ffd96a"))});
			break;
		}
		case FPDEvent::EKind::Leak:
			Shake = FMath::Max(Shake, 0.35f);
			FlashT = 0.4f;
			FlashCol = Hex(TEXT("ff3a4a"));
			PushWorldFx(EWorldFx::Leak, P, Hex(TEXT("ff3a4a")), 1.f);
			Floats.Add({FString::Printf(TEXT("목숨 -%d"), E.A), FVector2f(FX + Cols * CS - 60.f, FY + 40.f), 0.f, 1.2f, 20.f, Hex(TEXT("ff6a6a"))});
			break;
		case FPDEvent::EKind::Place:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 8.f, 46.f, 0.f, 0.4f, 2.f, Col});
			Burst(P, 14, Col, 40.f, 160.f, 0.45f, 2.2f);
			PushWorldFx(EWorldFx::Place, P, Col, 1.f);
			break;
		}
		case FPDEvent::EKind::Synergy:
		{
			const FLinearColor Col = HandColor(E.Hand);
			const int Val = HandValue(E.Hand, E.A);
			// 이 족보의 칸들을 찾아 칸마다 빛과 상승 입자
			for (const FPDSynergy& S : Game.Synergies)
			{
				if (S.Kind != E.Hand) continue;
				float CX = 0.f, CY = 0.f;
				for (int K : S.Cells) { CX += K % Cols + 0.5f; CY += K / Cols + 0.5f; }
				if (FMath::Abs(CX / S.Cells.size() - E.X) > 0.01f || FMath::Abs(CY / S.Cells.size() - E.Y) > 0.01f) continue;
				for (int K : S.Cells)
				{
					CellFlashes.Add({K, 0.6f, Col});
					for (int I = 0; I < 10; ++I)
					{
						FParticle Pa;
						Pa.P = CellCenter(K) + FVector2f(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-20.f, 20.f));
						Pa.V = FVector2f(FMath::FRandRange(-20.f, 20.f), FMath::FRandRange(-160.f, -60.f));
						Pa.MaxLife = Pa.Life = FMath::FRandRange(0.5f, 1.0f);
						Pa.Size = FMath::FRandRange(1.5f, 3.f);
						Pa.Drag = 1.2f;
						Pa.Gravity = 0.f;
						Pa.Col = Col;
						Pa.bStreak = true;
						Particles.Add(Pa);
					}
				}
				break;
			}
			Rings.Add({P, 10.f, 70.f + Val * 0.6f, 0.f, 0.55f, 3.f, Col});
			Burst(P, 18 + Val / 3, Col, 80.f, 260.f, 0.7f, 2.6f, true);
			PushWorldFx(EWorldFx::Synergy, P, Col, 1.f + Val / 40.f);
			FString Label = HandName(E.Hand);
			if (E.Hand == EPDHand::Flush || E.Hand == EPDHand::Straight) Label += FString::FromInt(E.A);
			Floats.Add({Label + TEXT("!"), P - FVector2f(0.f, 24.f), 0.f, 1.4f, Val >= 80 ? 30.f : 22.f, Col});
			if (E.B >= 2)
			{
				Floats.Add({FString::Printf(TEXT("연쇄 ×%s"), *FString::SanitizeFloat(1.f + ChainStep * (E.B - 1))), P + FVector2f(0.f, 6.f), 0.f, 1.4f, 18.f, FLinearColor::White});
				Shake = FMath::Max(Shake, 0.12f + 0.05f * E.B);
			}
			if (Val >= 80) { FlashT = 0.3f; FlashCol = Col; Shake = FMath::Max(Shake, 0.3f); }
			break;
		}
		case FPDEvent::EKind::WaveStart:
			BannerText = E.B ? FString::Printf(TEXT("BOSS WAVE %d"), E.A) : FString::Printf(TEXT("WAVE %d"), E.A);
			BannerSub = E.B ? FString::Printf(TEXT("보스: %s"), BossName(E.Enemy)) : (E.A > Game.BestWave && Game.BestWave > 0 ? FString(TEXT("최고 기록 갱신 중!")) : FString::Printf(TEXT("최고 %d웨이브"), Game.BestWave));
			BannerCol = E.B ? Hex(TEXT("ff6a6a")) : Hex(TEXT("7fd6ff"));
			BannerT = BannerMax = 1.6f;
			if (E.B) { FlashT = 0.25f; FlashCol = BannerCol; }
			break;
		case FPDEvent::EKind::WaveClear:
			BannerText = TEXT("WAVE CLEAR");
			BannerSub = FString::Printf(TEXT("카드 +%d%s"), E.A, E.A < DrawPerWave ? TEXT(" (손패 가득)") : TEXT(""));
			BannerCol = Hex(TEXT("9cffb0"));
			BannerT = BannerMax = 1.4f;
			break;
		case FPDEvent::EKind::GameOver:
			Shake = 0.8f;
			FlashT = 0.6f;
			FlashCol = Hex(TEXT("ff3a4a"));
			break;
		case FPDEvent::EKind::Win:
			for (int I = 0; I < 6; ++I) PushWorldFx(EWorldFx::Fuse, FVector2f(FX + FMath::FRandRange(100.f, Cols * CS - 100.f), FY + FMath::FRandRange(80.f, Rows * CS - 80.f)), HandColor((EPDHand)(I % (int)EPDHand::Count)), 2.f);
			for (int I = 0; I < 6; ++I) Burst(FVector2f(FX + FMath::FRandRange(100.f, Cols * CS - 100.f), FY + FMath::FRandRange(80.f, Rows * CS - 80.f)), 40, HandColor((EPDHand)(I % (int)EPDHand::Count)), 60.f, 300.f, 1.4f, 3.f, false, 120.f);
			FlashT = 0.5f;
			FlashCol = Hex(TEXT("ffd96a"));
			break;
		case FPDEvent::EKind::Discard: break;
		case FPDEvent::EKind::JokerFuse:
		{
			for (const FVector2f& From : PendingFuseCells)
				for (int I = 0; I < 10; ++I)
				{
					FParticle Pa;
					Pa.P = From + FVector2f(FMath::FRandRange(-18.f, 18.f), FMath::FRandRange(-18.f, 18.f));
					Pa.V = (P - Pa.P) / 0.45f;
					Pa.MaxLife = Pa.Life = 0.45f;
					Pa.Size = FMath::FRandRange(1.8f, 3.2f);
					Pa.Drag = 0.f;
					Pa.Gravity = 0.f;
					Pa.Col = I % 3 ? JokerPink : FLinearColor::White;
					Pa.bStreak = true;
					Particles.Add(Pa);
				}
			PendingFuseCells.Reset();
			Rings.Add({P, 110.f, 6.f, 0.f, 0.45f, 3.f, JokerPink});
			Rings.Add({P, 10.f, 160.f, 0.3f, 1.1f, 3.4f, JokerPink});
			Rings.Add({P, 10.f, 240.f, 0.4f, 1.4f, 2.f, FLinearColor::White});
			Burst(P, 90, JokerPink, 100.f, 380.f, 1.1f, 3.2f, true);
			PushWorldFx(EWorldFx::JokerFuse, P, JokerPink, 1.f);
			Burst(P, 40, FLinearColor::White, 60.f, 240.f, 0.9f, 2.4f, false, 60.f);
			Floats.Add({FString::Printf(TEXT("조커: %s!"), JokerName(E.A)), P - FVector2f(0.f, 36.f), 0.f, 2.f, 30.f, JokerPink});
			if (E.B) Floats.Add({TEXT("새로 발견! (J: 도감)"), P + FVector2f(0.f, 2.f), 0.f, 2.f, 16.f, FLinearColor::White});
			FlashT = 0.45f;
			FlashCol = JokerPink;
			Shake = FMath::Max(Shake, 0.55f);
			BannerText = JokerName(E.A);
			BannerSub = JokerAbility(E.A);
			BannerCol = JokerPink;
			BannerT = BannerMax = 1.8f;
			break;
		}
		case FPDEvent::EKind::Feed: // 족보 먹이기: 먹인 칸에서 조커로 빨려 드는 입자, 레벨이 오르면 고리와 글자
		{
			for (const FVector2f& From : PendingFuseCells)
				for (int I = 0; I < 8; ++I)
				{
					FParticle Pa;
					Pa.P = From + FVector2f(FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(-14.f, 14.f));
					Pa.V = (P - Pa.P) / 0.4f;
					Pa.MaxLife = Pa.Life = 0.4f;
					Pa.Size = FMath::FRandRange(1.6f, 2.8f);
					Pa.Drag = 0.f;
					Pa.Gravity = 0.f;
					Pa.Col = I % 3 ? JokerPink : FLinearColor::White;
					Pa.bStreak = true;
					Particles.Add(Pa);
				}
			PendingFuseCells.Reset();
			Rings.Add({P, 60.f, 6.f, 0.f, 0.4f, 2.4f, JokerPink});
			if (E.B > 0)
			{
				Rings.Add({P, 10.f, 110.f, 0.3f, 0.9f, 3.f, JokerPink});
				Burst(P, 40, JokerPink, 80.f, 260.f, 0.8f, 2.6f, true);
				PushWorldFx(EWorldFx::Synergy, P, JokerPink, 2.f);
				Floats.Add({FString::Printf(TEXT("레벨 %d!"), E.A), P - FVector2f(0.f, 30.f), 0.f, 1.6f, 22.f, JokerPink});
				Shake = FMath::Max(Shake, 0.25f);
			}
			else Floats.Add({FString::Printf(TEXT("경험치 +%.0f"), E.R), P - FVector2f(0.f, 26.f), 0.f, 1.2f, 14.f, JokerPink});
			break;
		}
		case FPDEvent::EKind::Bell:
			Rings.Add({P, 10.f, E.R * CS, 0.f, 0.6f, 3.f, Hex(TEXT("cfe4ff"))});
			Rings.Add({P, 10.f, E.R * CS * 0.7f, 0.1f, 0.6f, 2.f, JokerPink});
			Burst(P, 20, Hex(TEXT("cfe4ff")), 60.f, 200.f, 0.5f, 2.f, true);
			PushWorldFx(EWorldFx::Splash, P, Hex(TEXT("cfe4ff")), FMath::Max(0.5f, E.R));
			break;
		case FPDEvent::EKind::Star:
			for (const FPDEnemy& En : Game.Enemies)
			{
				const FPDPoint Q = Game.Path.At(En.D);
				Beams.Add({P, CellPx(Q.X, Q.Y), 0.f, 0.18f, 1.6f, FLinearColor::White});
				Burst(CellPx(Q.X, Q.Y), 3, JokerPink, 40.f, 120.f, 0.25f, 2.f, true);
				PushWorldFx(EWorldFx::ImpGlitter, CellPx(Q.X, Q.Y), JokerPink, 1.f);
			}
			Rings.Add({P, 6.f, 40.f, 0.f, 0.3f, 2.f, FLinearColor::White});
			break;
		case FPDEvent::EKind::Fuse:
		{
			const FLinearColor Col = SuitColor(E.Suit), HC = HandColor(E.Hand);
			// 족보 칸들에서 합쳐질 칸으로 빨려 들어가는 입자
			for (const FVector2f& From : PendingFuseCells)
				for (int I = 0; I < 14; ++I)
				{
					FParticle Pa;
					Pa.P = From + FVector2f(FMath::FRandRange(-18.f, 18.f), FMath::FRandRange(-18.f, 18.f));
					Pa.V = (P - Pa.P) / 0.35f;
					Pa.MaxLife = Pa.Life = 0.35f;
					Pa.Size = FMath::FRandRange(1.6f, 3.f);
					Pa.Drag = 0.f;
					Pa.Gravity = 0.f;
					Pa.Col = I % 2 ? Col : HC;
					Pa.bStreak = true;
					Particles.Add(Pa);
				}
			PendingFuseCells.Reset();
			Rings.Add({P, 70.f, 6.f, 0.f, 0.35f, 2.4f, HC});   // 안으로 모이는 고리
			Rings.Add({P, 8.f, 90.f, 0.15f, 0.75f, 3.f, Col}); // 터져 나가는 고리
			Burst(P, 50, Col, 80.f, 300.f, 0.8f, 2.8f, true);
			PushWorldFx(EWorldFx::Fuse, P, Col, (float)E.A);
			Floats.Add({FString::Printf(TEXT("레벨 %d!"), E.A), P - FVector2f(0.f, 30.f), 0.f, 1.5f, 28.f, Hex(TEXT("ffd96a"))});
			FlashT = FMath::Max(FlashT, 0.2f);
			FlashCol = Col;
			Shake = FMath::Max(Shake, 0.25f + 0.08f * E.A);
			break;
		}
		case FPDEvent::EKind::TreeBuy:
			TreeBuyId = E.A;
			TreeBuyT = 0.8f;
			break;
		case FPDEvent::EKind::Reward:
		{
			const FString Name = E.A ? CharmName(E.B) : InstantName(E.B);
			Floats.Add({Name + TEXT("!"), FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 0.f, 1.4f, 26.f, Hex(TEXT("ffd96a"))});
			Burst(FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 40, Hex(TEXT("ffd96a")), 60.f, 260.f, 0.8f, 2.6f, true);
			break;
		}
		}
	}
	Game.Events.clear();
}

namespace
{
FString ShortNum(float V) // 1234 → 1.2k, 3456789 → 3.5M
{
	if (V >= 1e9f) return FString::Printf(TEXT("%.1fB"), V / 1e9f);
	if (V >= 1e6f) return FString::Printf(TEXT("%.1fM"), V / 1e6f);
	if (V >= 1e4f) return FString::Printf(TEXT("%.0fk"), V / 1e3f);
	if (V >= 1e3f) return FString::Printf(TEXT("%.1fk"), V / 1e3f);
	return FString::FromInt(FMath::Max(1, FMath::RoundToInt(V)));
}
}

void SPDBoardWidget::BurstCone(FVector2f P, FVector2f Dir, float Spread, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak, float Gravity)
{
	const float Base = FMath::Atan2(Dir.Y, Dir.X);
	for (int I = 0; I < N && Particles.Num() < 2600; ++I)
	{
		const float A = Base + FMath::FRandRange(-Spread, Spread), Sp = FMath::FRandRange(SpeedMin, SpeedMax);
		const float L = Life * FMath::FRandRange(0.7f, 1.2f);
		Particles.Add({P, FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp, L, L, Size, 3.f, Gravity, Col, bStreak});
	}
}

void SPDBoardWidget::AddShards(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float LenMin, float LenMax, float Life)
{
	for (int I = 0; I < N && Shards.Num() < 700; ++I)
	{
		const float A = FMath::FRandRange(0.f, TwoPi), L = Life * FMath::FRandRange(0.7f, 1.2f);
		Shards.Add({P, FVector2f(FMath::Cos(A), FMath::Sin(A)) * FMath::FRandRange(SpeedMin, SpeedMax), A, FMath::FRandRange(-16.f, 16.f), FMath::FRandRange(LenMin, LenMax), L, L, Col});
	}
}

// 발사체마다 다른 타격 효과 (Power: 1 약 ~ 6 강, 레벨·족보·조커·치명타로 커진다)
// 발사체 → 월드 타격 이펙트 무리 (D-094): 방향 불꽃, 폭발, 서리, 전기, 반짝이, 별빛 터짐
static SPDBoardWidget::EWorldFx ImpactWorldFx(EPDShot K)
{
	using W = SPDBoardWidget::EWorldFx;
	switch (K)
	{
	case EPDShot::Laser: case EPDShot::Bullet: case EPDShot::Arrow: case EPDShot::Blade: return W::ImpSpark;
	case EPDShot::Missile: case EPDShot::Cluster: case EPDShot::Shell: return W::ImpBlast;
	case EPDShot::Orb: return W::ImpFrost;
	case EPDShot::Arc: case EPDShot::JokerLaser: case EPDShot::Grin: case EPDShot::Crescent: return W::ImpZap;
	case EPDShot::Comet: case EPDShot::StarComet: case EPDShot::Nova: case EPDShot::Wave: return W::ImpStarburst;
	default: return W::ImpGlitter; // 별, 로열, 광대, 모자 장수, 희비극, 할리퀸, 지휘봉
	}
}

void SPDBoardWidget::ImpactFx(EPDShot K, FVector2f P, FVector2f Dir, FLinearColor Col, int32 Lv, float Power)
{
	const float Pw = FMath::Clamp(Power, 1.f, 6.f), Ang = FMath::Atan2(Dir.Y, Dir.X);
	const bool bBusy = Particles.Num() > 1800; // 적이 많을 때는 가볍게
	const float Q = bBusy ? 0.4f : 1.f;        // 파티클 수 배율
	auto Nq = [&](float N) { return FMath::Max(1, FMath::RoundToInt(N * Q)); };
	const FLinearColor White = FLinearColor::White, Gold = Hex(TEXT("ffd96a")), Smoke = Hex(TEXT("8a8090"), 0.45f), Cyan = Hex(TEXT("7fd6ff"));
	const FVector2f Nrm(-Dir.Y, Dir.X);
	const float Rnd = FMath::FRandRange(0.f, TwoPi);
	// 파동 하나 더하기: 모양, 갈래/변 수, 시작 크기 → 끝 크기, 늦음, 길이, 굵기, 색, 시작 각, 회전
	auto Wave = [&](EWave S, int32 N, float R0, float R1, float Delay, float Life, float Th, const FLinearColor& Cl, float Rot, float Spin) {
		Rings.Add({P, R0, R1, -Delay, Life, Th, Cl, S, N, Rot, Spin});
	};
	switch (K)
	{
	case EPDShot::Laser: // ♠ 관통: 꿰뚫는 빛줄 + 퍼지는 스페이드 + 앞쪽 빛줄기
		Flares.Add({P, 0.f, 0.14f, 16.f + 5.f * Lv, Ang, Col, 2});
		Flares.Add({P, 0.f, 0.1f, 7.f + 2.f * Lv, Ang + PI / 2.f, White, 2});
		Wave(EWave::Suit, (int32)EPDSuit::Spade, 4.f, 14.f + 3.f * Lv, 0.f, 0.22f, 1.5f, Col, 0.f, 0.f);
		BurstCone(P, Dir, 0.35f, Nq(4 + 2 * Lv), Col, 200.f, 380.f, 0.2f, 1.4f, true);
		if (Lv >= 3) Wave(EWave::Rays, 6, 10.f, 30.f, 0.03f, 0.2f, 1.2f, Col, Ang, 0.f);
		if (Lv >= 4) Wave(EWave::Poly, 4, 6.f, 36.f, 0.05f, 0.3f, 2.f, Col, Ang + PI / 4.f, 3.f);
		break;
	case EPDShot::JokerLaser:
		Flares.Add({P, 0.f, 0.16f, 18.f, Rnd, Col});
		Wave(EWave::Star, 4, 4.f, 22.f, 0.f, 0.25f, 1.6f, Col, Rnd, 4.f);
		break;
	case EPDShot::Missile: // ♥ 불덩이: 퍼지는 하트 + 끊어진 충격파 + 불똥·연기
		Flares.Add({P, 0.f, 0.3f + 0.04f * Lv, 20.f + 7.f * Lv, 0.f, Hex(TEXT("ff8a5a")), 0});
		Flares.Add({P, 0.f, 0.12f, 10.f + 3.f * Lv, Rnd, Gold, 6});
		Wave(EWave::Suit, (int32)EPDSuit::Heart, 5.f, 16.f + 5.f * Lv, 0.f, 0.32f, 2.f, Col, 0.f, 0.f);
		Wave(EWave::Broken, 5, 8.f, 26.f + 6.f * Lv, 0.04f, 0.35f, 1.6f, Gold, Rnd, 2.5f);
		Burst(P, Nq(10 + 4 * Lv), Col, 60.f, 180.f + 25.f * Lv, 0.5f, 2.6f, false, 90.f);
		Burst(P, Nq(6 + 2 * Lv), Gold, 80.f, 220.f, 0.3f, 1.6f, true);
		Burst(P, Nq(5), Smoke, 10.f, 40.f, 0.9f, 4.f, false, -50.f);
		if (Lv >= 4) { Wave(EWave::Poly, 8, 10.f, 56.f, 0.06f, 0.45f, 1.4f, Gold, Rnd, 1.f); Shake = FMath::Max(Shake, 0.1f); }
		break;
	case EPDShot::Bullet: // ♦ 도탄: 작은 마름모가 튀고, 반대쪽 불티
		Flares.Add({P, 0.f, 0.08f, 6.f + 2.f * Lv, Rnd, Col});
		Wave(EWave::Suit, (int32)EPDSuit::Diamond, 3.f, 9.f + 2.f * Lv, 0.f, 0.14f, 1.3f, Col, 0.f, 0.f);
		BurstCone(P, -Dir, 0.7f, Nq(3 + Lv), Col, 160.f, 320.f, 0.14f, 1.2f, true, 200.f);
		if (Lv >= 3) BurstCone(P, Dir, 0.25f, Nq(3), White, 250.f, 400.f, 0.1f, 1.f, true);
		if (FMath::FRand() < 0.4f) AddShards(P, 1, Hex(TEXT("b0a070")), 40.f, 90.f, 2.f, 3.f, 0.5f);
		if (Lv >= 4) Wave(EWave::Poly, 4, 2.f, 16.f, 0.02f, 0.16f, 1.2f, White, Rnd, 8.f);
		break;
	case EPDShot::Orb: // ♣ 얼음: 육각 서리가 천천히 돌며 퍼지고, 눈꽃·얼음 조각·눈
		Flares.Add({P, 0.f, 0.4f, 14.f + 4.f * Lv, Rnd, Hex(TEXT("cfffe8")), 6});
		Wave(EWave::Poly, 6, 6.f, 18.f + 4.f * Lv, 0.f, 0.6f, 1.6f, Col, Rnd, 0.8f);
		Wave(EWave::Suit, (int32)EPDSuit::Club, 3.f, 10.f + 2.f * Lv, 0.05f, 0.35f, 1.2f, Hex(TEXT("eafff6")), 0.f, 0.f);
		AddShards(P, Nq(4 + Lv), Col, 60.f, 150.f, 3.f, 6.f, 0.5f);
		Burst(P, Nq(6), Hex(TEXT("eafff6")), 10.f, 40.f, 0.9f, 1.6f, false, 30.f);
		break;
	case EPDShot::Star: // ★ 반짝임: 도는 별 테두리 + 내려앉는 반짝이
		Flares.Add({P, 0.f, 0.3f, 12.f + 4.f * Lv, Rnd, White, 5});
		Wave(EWave::Star, 5, 4.f, 16.f + 4.f * Lv, 0.f, 0.3f, 1.4f, White, Rnd, 5.f);
		Burst(P, Nq(5 + 2 * Lv), White, 20.f, 80.f, 0.8f, 1.5f, false, 25.f);
		if (Lv >= 3) for (int32 J = 0; J < Lv - 1; ++J) Flares.Add({P + Rot2(FVector2f(14.f, 0.f), J * TwoPi / (Lv - 1)), -0.05f * J, 0.2f, 6.f, 0.f, White, 4});
		break;
	case EPDShot::Arrow: // 스트레이트: 박히는 화살 셋 + 앞으로 퍼지는 삼각형
		for (int32 J = -1; J <= 1; ++J) Flares.Add({P + Nrm * J * 6.f, -0.03f * (J + 1), 0.14f, 12.f, Ang, Col, 2});
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.22f, 1.5f, Col, Ang, 0.f);
		BurstCone(P, Dir, 0.3f, Nq(8), Col, 180.f, 320.f, 0.18f, 1.3f, true);
		break;
	case EPDShot::Arc: // 플러시: 갈라지는 잔번개 + 지직거리는 끊어진 호
		for (int32 J = 0; J < 4; ++J) Beams.Add({P, P + Rot2(FVector2f(FMath::FRandRange(18.f, 34.f), 0.f), FMath::FRandRange(0.f, TwoPi)), 0.f, 0.16f, 1.4f, Col, 2, 1});
		Flares.Add({P, 0.f, 0.12f, 14.f, Rnd, White, 8});
		Wave(EWave::Broken, 7, 8.f, 24.f, 0.f, 0.2f, 1.3f, Col, Rnd, -9.f);
		Burst(P, Nq(10), Col, 120.f, 300.f, 0.12f, 1.2f, true);
		break;
	case EPDShot::Cluster: // 풀하우스: 차례로 터지는 작은 폭발 다섯 (각자 도는 네모)
		for (int32 J = 0; J < 5; ++J)
		{
			const FVector2f Qp = P + Rot2(FVector2f(16.f, 0.f), J * TwoPi / 5.f + FMath::FRandRange(-0.3f, 0.3f));
			Flares.Add({Qp, -0.06f * J, 0.22f, 14.f, 0.f, Col, 0});
			Flares.Add({Qp, -0.06f * J, 0.12f, 8.f, Rnd, Gold, 4});
			Rings.Add({Qp, 2.f, 12.f, -0.06f * J, 0.24f, 1.6f, Col, EWave::Poly, 4, Rnd + J, 6.f});
		}
		Burst(P, Nq(16), Col, 80.f, 220.f, 0.5f, 2.f, false, 80.f);
		break;
	case EPDShot::Shell: // 포카드: 거대한 폭발 (흰 섬광, 팔각 충격파, 사방 빛줄기, 끊어진 호, 파편, 먼지)
		Flares.Add({P, 0.f, 0.18f, 46.f, Rnd, White, 4});
		Flares.Add({P, 0.f, 0.45f, 60.f, 0.f, Col, 0});
		Wave(EWave::Poly, 8, 6.f, 46.f, 0.f, 0.32f, 3.2f, Col, Rnd, 0.5f);
		Wave(EWave::Rays, 12, 14.f, 70.f, 0.03f, 0.3f, 1.6f, White, Rnd, 0.f);
		Wave(EWave::Broken, 6, 12.f, 96.f, 0.1f, 0.6f, 1.2f, WithAlpha(Col, 0.7f), Rnd, 1.5f);
		AddShards(P, Nq(10), Col, 120.f, 300.f, 5.f, 10.f, 0.7f);
		Burst(P, Nq(18), Smoke, 20.f, 90.f, 1.1f, 4.5f, false, -30.f);
		Burst(P, Nq(14), White, 150.f, 380.f, 0.2f, 1.5f, true);
		Shake = FMath::Max(Shake, 0.2f);
		break;
	case EPDShot::Comet: // 스트레이트 플러시: 여덟 뿔 별이 돌며 퍼지고 긴 빛살
		Flares.Add({P, 0.f, 0.3f, 30.f, Rnd, Col, 8});
		Wave(EWave::Star, 8, 6.f, 34.f, 0.f, 0.32f, 1.8f, Col, Rnd, 3.f);
		Burst(P, Nq(20), Col, 160.f, 420.f, 0.32f, 1.6f, true);
		break;
	case EPDShot::Nova: // 파이브 카드: 안으로 빨려 들었다가 오각형이 차례로 터져 나감
		Wave(EWave::Circle, 0, 34.f, 4.f, 0.f, 0.14f, 1.6f, Col, 0.f, 0.f);
		for (int32 J = 0; J < 4; ++J) Wave(EWave::Poly, 5, 6.f + J * 4.f, 26.f + J * 14.f, 0.14f + 0.05f * J, 0.35f, 2.4f - J * 0.4f, Col, Rnd + J * 0.3f, 1.5f);
		Flares.Add({P, -0.14f, 0.4f, 34.f, 0.f, Col, 0});
		Burst(P, Nq(12), Gold, 40.f, 140.f, 0.7f, 1.6f, false, -20.f);
		break;
	case EPDShot::Crown: // 로열: 금빛 빛줄기 + 여덟 뿔 별 + 솟았다 떨어지는 금가루
		Flares.Add({P, 0.f, 0.4f, 40.f, Rnd, Col, 8});
		Flares.Add({P, 0.f, 0.2f, 18.f, 0.f, White, 4});
		Wave(EWave::Rays, 16, 10.f, 50.f, 0.f, 0.4f, 1.6f, Col, Rnd, 0.6f);
		Wave(EWave::Star, 8, 6.f, 30.f, 0.06f, 0.4f, 2.f, White, Rnd, -2.f);
		BurstCone(P, FVector2f(0.f, -1.f), 0.9f, Nq(20), Gold, 120.f, 300.f, 0.9f, 1.8f, false, 420.f);
		Burst(P, Nq(10), White, 30.f, 120.f, 0.7f, 1.4f, false, -40.f);
		break;
	case EPDShot::Grin: // 체셔: 보랏빛 덩어리 + 안으로 감겨 드는 끊어진 호
		Flares.Add({P, 0.f, 0.4f, 26.f, 0.f, Col, 0});
		Wave(EWave::Broken, 3, 28.f, 6.f, 0.f, 0.35f, 1.6f, Col, Rnd, 6.f);
		Burst(P, Nq(10), Col, 30.f, 120.f, 0.6f, 1.6f, false, -30.f);
		break;
	case EPDShot::Juggle: // 광대: 네 무늬가 사방으로 퍼짐 + 색종이
		for (int32 J = 0; J < 4; ++J) Rings.Add({P + Rot2(FVector2f(6.f, 0.f), J * PI / 2.f + Rnd), 2.f, 9.f, -0.03f * J, 0.3f, 1.3f, SuitColor((EPDSuit)J), EWave::Suit, J, 0.f, 0.f});
		for (int32 J = 0; J < 4; ++J) BurstCone(P, FVector2f(0.f, -1.f), 1.2f, Nq(4), SuitColor((EPDSuit)J), 100.f, 240.f, 0.8f, 2.f, false, 500.f);
		break;
	case EPDShot::Hat: // 모자 장수: 도는 네모 + 펑 연기 + 날리는 카드 조각
		Wave(EWave::Poly, 4, 4.f, 20.f, 0.f, 0.3f, 1.5f, Col, Rnd, 7.f);
		Burst(P, Nq(8), Smoke, 20.f, 70.f, 0.7f, 4.f, false, -40.f);
		AddShards(P, Nq(5), White, 60.f, 140.f, 5.f, 7.f, 0.8f);
		Flares.Add({P, 0.f, 0.2f, 16.f, Rnd, Col, 4});
		break;
	case EPDShot::Crescent: // 웃는 가면: 남아서 웃는 초승달 + 반쪽 호 둘
		Beams.Add({P, P, 0.f, 0.35f, 1.6f, Col, 4, 1});
		Wave(EWave::Broken, 2, 6.f, 22.f, 0.f, 0.28f, 1.5f, Col, Rnd, 10.f);
		Burst(P, Nq(8), Col, 60.f, 180.f, 0.3f, 1.6f, true);
		break;
	case EPDShot::Twin: // 희비극: 분홍·파랑 삼각형이 엇갈려 육각별
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.3f, 1.6f, Col, -PI / 2.f, 2.f);
		Wave(EWave::Poly, 3, 4.f, 20.f, 0.f, 0.3f, 1.6f, Cyan, PI / 2.f, -2.f);
		Flares.Add({P + Nrm * 7.f, 0.f, 0.22f, 14.f, 0.f, Col, 4});
		Flares.Add({P - Nrm * 7.f, 0.f, 0.22f, 14.f, PI / 4.f, Cyan, 4});
		break;
	case EPDShot::Shards: // 할리퀸: 마름모 셋이 차례로 퍼지고 조각이 깨져 흩어짐
		for (int32 J = 0; J < 3; ++J) Wave(EWave::Suit, (int32)EPDSuit::Diamond, 3.f, 12.f + J * 6.f, 0.05f * J, 0.25f, 1.4f, J == 1 ? White : Col, 0.f, 0.f);
		AddShards(P, Nq(10), Col, 100.f, 240.f, 4.f, 8.f, 0.6f);
		AddShards(P, Nq(4), White, 100.f, 240.f, 4.f, 7.f, 0.5f);
		break;
	case EPDShot::Note: // 지휘봉: 박자 맞춰 퍼지는 오각형 셋 + 떠오르는 반짝이
		for (int32 J = 0; J < 3; ++J) Wave(EWave::Poly, 5, 3.f, 16.f, 0.1f * J, 0.2f, 1.6f, Col, Rnd + J, 3.f);
		Burst(P, Nq(10), Col, 20.f, 70.f, 0.8f, 1.8f, false, -140.f);
		break;
	case EPDShot::Blade: // 잭: X 자 베기 + 날카로운 빛줄기 + 흔들림
		Beams.Add({P + FVector2f(-16.f, -12.f), P + FVector2f(16.f, 12.f), 0.f, 0.2f, 2.6f, Col});
		Beams.Add({P + FVector2f(16.f, -12.f), P + FVector2f(-16.f, 12.f), 0.f, 0.2f, 2.6f, Col});
		Flares.Add({P, 0.f, 0.14f, 20.f, PI / 4.f, White, 4});
		Wave(EWave::Rays, 8, 8.f, 30.f, 0.02f, 0.2f, 1.4f, White, Rnd, 0.f);
		Burst(P, Nq(14), White, 120.f, 300.f, 0.22f, 1.6f, true);
		Shake = FMath::Max(Shake, 0.08f);
		break;
	case EPDShot::Wave: // 방울: 차례로 퍼지는 끊어진 소리 호 넷 (파티클 없음)
		for (int32 J = 0; J < 4; ++J) Wave(EWave::Broken, 4, 4.f, 20.f + J * 9.f, 0.06f * J, 0.3f, 1.8f - J * 0.3f, Col, Ang + PI / 4.f, 0.f);
		break;
	case EPDShot::StarComet: // 스타: 큰 다섯 뿔 별이 돌며 퍼지고 빛살
		Flares.Add({P, 0.f, 0.35f, 32.f, Rnd, Col, 5});
		Flares.Add({P, 0.f, 0.18f, 16.f, 0.f, White, 5});
		Wave(EWave::Star, 5, 6.f, 36.f, 0.f, 0.35f, 2.f, Col, Rnd, 4.f);
		Burst(P, Nq(16), Col, 120.f, 320.f, 0.35f, 1.8f, true);
		break;
	default:
		Flares.Add({P, 0.f, 0.14f, 12.f, Rnd, Col});
		break;
	}
	if (Pw >= 5.f) Shake = FMath::Max(Shake, 0.04f + 0.02f * (Pw - 5.f));
	// 월드 층: 무리별 나이아가라 (♦ 도탄은 맞은 반대쪽으로 튄다)
	PushWorldFx(ImpactWorldFx(K), P, Col, Pw, K == EPDShot::Bullet ? -Dir : Dir);
}

// 맞는 순간 공통: 작은 흰 심 + 맞은 적 번쩍임 + 피해 숫자
void SPDBoardWidget::PushWorldFx(EWorldFx Kind, FVector2f DesignPos, const FLinearColor& Col, float Scale, FVector2f Dir)
{
	if (WorldFx.Num() >= 64) return; // 한 프레임에 너무 많이 쌓이면 버린다 (나이아가라 폭주 방지)
	// 판 좌표 → 화면 가운데를 0으로 한 판 좌표. 판은 창 가운데에 맞춰 그리므로 (Offset - 창/2) / 배율 만큼 민다
	const FVector2f Center = (ViewOffset - ViewSize * 0.5f) / FMath::Max(0.01f, ViewScale);
	const FVector2f D = Dir.GetSafeNormal();
	WorldFx.Add({Kind, DesignPos + Center, Col, Scale, D.IsNearlyZero() ? FVector2f(0.f, -1.f) : D});
}

void SPDBoardWidget::HitFx(FVector2f P, float Power, int32 Crits, float Dmg)
{
	PushWorldFx(EWorldFx::Hit, P, FLinearColor::White, FMath::Clamp(Power, 1.f, 6.f));
	Flares.Add({P, 0.f, 0.07f, 5.f + 1.5f * FMath::Clamp(Power, 1.f, 6.f), 0.f, FLinearColor::White, 0});
	// 맞은 적: 가장 가까운 적을 잠깐 하얗게
	int32 BestId = -1;
	float BestD = CS * 0.8f;
	for (const FPDEnemy& E : Game.Enemies)
	{
		const FPDPoint Q = Game.Path.At(E.D);
		const float D = FVector2f::Distance(CellPx(Q.X, Q.Y), P);
		if (D < BestD) { BestD = D; BestId = E.Id; }
	}
	if (BestId >= 0) HitFlash.Add(BestId, 0.14f);
	// 피해 숫자: 치명타는 금색으로 크게 튀어 오른다
	if (Dmg > 0.f && Floats.Num() < 70)
	{
		const bool bCrit = Crits > 0;
		const FVector2f At = P + FVector2f(FMath::FRandRange(-10.f, 10.f), -12.f);
		Floats.Add({bCrit ? ShortNum(Dmg) + TEXT("!") : ShortNum(Dmg), At, 0.f, bCrit ? 0.9f : 0.6f, bCrit ? 15.f + 2.f * FMath::Min(Crits, 4) : 11.f,
			bCrit ? Hex(TEXT("ffd96a")) : WithAlpha(FLinearColor::White, 0.85f)});
	}
}

// 죽는 순간: 하얀 폭발 심 + 큰 별빛 + 겹겹 충격파 + 몸 조각(도는 선) + 불씨 + 흔들림. 보스는 화면이 번쩍
void SPDBoardWidget::DeathFx(FVector2f P, EPDEnemy Type)
{
	const FLinearColor Col = EnemyColor(Type), White = FLinearColor::White;
	const bool bBoss = EnemyDef(Type).bBoss, bTank = Type == EPDEnemy::Tank || Type == EPDEnemy::Armored, bMini = Type == EPDEnemy::Mini;
	const float S = bBoss ? 4.f : bTank ? 1.8f : bMini ? 0.7f : 1.f; // 크기
	PushWorldFx(EWorldFx::Death, P, Col, S);
	const bool bBusy = Particles.Num() > 1800 || Shards.Num() > 500;
	// 빛: 흰 심 + 적 색 큰 별빛
	Flares.Add({P, 0.f, 0.12f + 0.05f * S, 22.f * S, FMath::FRandRange(0.f, PI), White});
	Flares.Add({P, 0.f, 0.28f + 0.08f * S, 40.f * S, FMath::FRandRange(0.f, PI), Col});
	// 충격파 셋: 적의 몸 모양 그대로 퍼진다 (빠른 적 삼각형, 탱크 육각형, 재생자 마름모, 보스 다각형, 나머지 동그라미)
	EWave BodyShape = EWave::Circle;
	int32 BodyN = 0;
	switch (Type)
	{
	case EPDEnemy::Fast: BodyShape = EWave::Poly; BodyN = 3; break;
	case EPDEnemy::Tank: BodyShape = EWave::Poly; BodyN = 6; break;
	case EPDEnemy::Regen: case EPDEnemy::RegenBoss: BodyShape = EWave::Poly; BodyN = Type == EPDEnemy::RegenBoss ? 6 : 4; break;
	case EPDEnemy::ShieldBoss: BodyShape = EWave::Poly; BodyN = 4; break;
	case EPDEnemy::SplitBoss: BodyShape = EWave::Poly; BodyN = 8; break;
	default: break;
	}
	const float Rot0 = FMath::FRandRange(0.f, TwoPi);
	Rings.Add({P, 2.f, 18.f * S, 0.f, 0.14f, 3.f, White, EWave::Rays, bBoss ? 16 : 8, Rot0, 0.f});
	Rings.Add({P, 6.f, 34.f * S, 0.f, 0.32f + 0.05f * S, 2.4f, Col, BodyShape, BodyN, Rot0, 2.f});
	Rings.Add({P, 10.f, 56.f * S, -0.06f, 0.5f + 0.08f * S, 1.2f, WithAlpha(Col, 0.6f), EWave::Broken, bBoss ? 8 : 5, Rot0, -1.5f});
	// 몸 조각: 돌며 흩어지는 선
	const int32 NS = bBusy ? 4 : FMath::RoundToInt(8.f * FMath::Min(S, 3.f));
	for (int32 I = 0; I < NS; ++I)
	{
		const float A = I * TwoPi / NS + FMath::FRandRange(-0.3f, 0.3f), Sp = FMath::FRandRange(90.f, 240.f) * (bBoss ? 1.6f : 1.f);
		Shards.Add({P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 6.f * S, FVector2f(FMath::Cos(A), FMath::Sin(A)) * Sp, A, FMath::FRandRange(-14.f, 14.f), FMath::FRandRange(5.f, 10.f) * FMath::Min(S, 2.5f),
			0.f, FMath::FRandRange(0.45f, 0.8f) * (bBoss ? 1.6f : 1.f), Col});
		Shards.Last().Life = Shards.Last().MaxLife;
	}
	// 빛살 + 불씨 (떨어짐) + 흰 불꽃
	Burst(P, bBusy ? 6 : FMath::RoundToInt(16 * S), Col, 120.f, 300.f * FMath::Min(S, 2.f), 0.35f, 1.8f, true);
	if (!bBusy) Burst(P, FMath::RoundToInt(10 * S), Hex(TEXT("ffd96a")), 40.f, 160.f * FMath::Min(S, 2.f), 0.9f, 2.f, false, 160.f);
	Burst(P, bBusy ? 3 : FMath::RoundToInt(8 * S), White, 160.f, 380.f, 0.2f, 1.4f, true);
	Shake = FMath::Max(Shake, bBoss ? 0.7f : bTank ? 0.18f : 0.06f);
	if (bBoss) { FlashT = 0.45f; FlashCol = Col; }
}

void SPDBoardWidget::CritFx(FVector2f P, int32 N)
{
	// 치명타: 흰 불꽃과 고리 (여러 번이면 더 크게)
	Burst(P, 6 + 3 * FMath::Min(N, 4), FLinearColor::White, 80.f, 240.f, 0.3f, 2.2f, true);
	PushWorldFx(EWorldFx::Crit, P, FLinearColor::White, (float)FMath::Min(N, 4));
	Rings.Add({P, 4.f, 18.f + 6.f * FMath::Min(N, 4), 0.f, 0.22f, 2.f, FLinearColor::White, EWave::Star, 4, FMath::FRandRange(0.f, TwoPi), 6.f}); // 도는 네 뿔 별
}

void SPDBoardWidget::UpdateFx(float Dt)
{
	// 발사체 꼬리 (게임이 멈춰 있으면 안 남김): 미사일 연기, 서리, 혜성 불꽃, 포탄 연기, 분열탄 불똥, 음표 반짝이, 레벨 3 이상은 무늬 색 잔상
	if (!bTree && !bUpg && !bMenu)
		for (const FPDProjectile& Pr : Game.Projectiles)
		{
			if (Particles.Num() >= 2600) break;
			const FVector2f At = CellPx(Pr.X, Pr.Y);
			const float VL = FMath::Max(0.001f, FMath::Sqrt(Pr.VX * Pr.VX + Pr.VY * Pr.VY));
			const FVector2f Back(-Pr.VX / VL, -Pr.VY / VL);
			const FVector2f Jit(FMath::FRandRange(-12.f, 12.f), FMath::FRandRange(-12.f, 12.f));
			const FLinearColor Col = ShotColor(Pr.Kind, Pr.Suit);
			switch (Pr.Kind)
			{
			case EPDShot::Missile: Particles.Add({At, Jit, 0.35f, 0.35f, 2.4f + 0.3f * (Pr.Level - 1), 2.f, 0.f, Hex(TEXT("ff9a8a"), 0.6f), false}); break;
			case EPDShot::Orb: Particles.Add({At, Jit, 0.3f, 0.3f, 1.8f, 2.f, 0.f, Hex(TEXT("bfffe0"), 0.5f), false}); break;
			case EPDShot::Comet: case EPDShot::StarComet: Particles.Add({At, Back * 70.f + Jit, 0.3f, 0.3f, 1.6f, 3.f, 0.f, WithAlpha(Col, 0.8f), true}); break;
			case EPDShot::Shell: Particles.Add({At, Jit * 0.5f, 0.5f, 0.5f, 3.2f, 1.5f, 0.f, Hex(TEXT("ff8a7a"), 0.45f), false}); break;
			case EPDShot::Cluster: Particles.Add({At, Back * 40.f + Jit * 2.f, 0.25f, 0.25f, 1.4f, 3.f, 0.f, WithAlpha(Col, 0.8f), true}); break;
			case EPDShot::Note: case EPDShot::Hat: case EPDShot::Twin: Particles.Add({At, Jit, 0.35f, 0.35f, 1.4f, 2.f, -30.f, WithAlpha(Col, 0.6f), false}); break;
			default:
				if (Pr.Level >= 3) Particles.Add({At, Back * 30.f + Jit * 0.5f, 0.2f, 0.2f, 1.4f, 3.f, 0.f, WithAlpha(Col, 0.55f), false});
				break;
			}
		}
	for (FParticle& Pa : Particles)
	{
		Pa.P += Pa.V * Dt;
		Pa.V *= FMath::Max(0.f, 1.f - Pa.Drag * Dt);
		Pa.V.Y += Pa.Gravity * Dt;
		Pa.Life -= Dt;
	}
	Particles.RemoveAll([](const FParticle& Pa) { return Pa.Life <= 0.f; });
	for (FFlare& Fl : Flares) Fl.T += Dt;
	Flares.RemoveAll([](const FFlare& Fl) { return Fl.T >= Fl.MaxT; });
	for (FShard& Sh : Shards)
	{
		Sh.P += Sh.V * Dt;
		Sh.V *= FMath::Max(0.f, 1.f - 2.5f * Dt);
		Sh.V.Y += 60.f * Dt;
		Sh.A += Sh.AV * Dt;
		Sh.Life -= Dt;
	}
	Shards.RemoveAll([](const FShard& Sh) { return Sh.Life <= 0.f; });
	for (auto It = HitFlash.CreateIterator(); It; ++It) { It.Value() -= Dt; if (It.Value() <= 0.f) It.RemoveCurrent(); }
	{ // 체력 막대: 깎인 부분이 흰색으로 남았다가 천천히 따라 내려간다
		TSet<int32> Live;
		for (const FPDEnemy& E : Game.Enemies)
		{
			Live.Add(E.Id);
			const float Now = FMath::Max(0.f, E.Hp / FMath::Max(1e-3f, E.MaxHp));
			float& Shown = HpShown.FindOrAdd(E.Id, Now);
			Shown = Now >= Shown ? Now : FMath::Max(Now, Shown - Dt * 0.9f);
		}
		for (auto It = HpShown.CreateIterator(); It; ++It) if (!Live.Contains(It.Key())) It.RemoveCurrent();
	}
	for (FBeam& B : Beams) B.T += Dt;
	Beams.RemoveAll([](const FBeam& B) { return B.T >= B.MaxT; });
	for (FRing& R : Rings) R.T += Dt;
	Rings.RemoveAll([](const FRing& R) { return R.T >= R.MaxT; });
	for (FFloatText& F : Floats) F.T += Dt;
	Floats.RemoveAll([](const FFloatText& F) { return F.T >= F.MaxT; });
	for (FCellFlash& F : CellFlashes) F.T -= Dt;
	CellFlashes.RemoveAll([](const FCellFlash& F) { return F.T <= 0.f; });
	Shake = FMath::Max(0.f, Shake - Dt);
	FlashT = FMath::Max(0.f, FlashT - Dt);
	BannerT = FMath::Max(0.f, BannerT - Dt);
	// 빠른 적과 보스 잔상
	TSet<int32> Alive;
	for (const FPDEnemy& E : Game.Enemies)
	{
		Alive.Add(E.Id);
		if (E.Type != EPDEnemy::Fast && !EnemyDef(E.Type).bBoss) continue;
		const FPDPoint Pt = Game.Path.At(E.D);
		TArray<FVector2f>& T = Trails.FindOrAdd(E.Id);
		T.Insert(CellPx(Pt.X, Pt.Y), 0);
		if (T.Num() > 8) T.SetNum(8);
	}
	for (auto It = Trails.CreateIterator(); It; ++It) if (!Alive.Contains(It.Key())) It.RemoveCurrent();
}

void SPDBoardWidget::UpdatePreview()
{
	const bool bOk = Sel >= 0 && Sel < (int32)Game.Hand.size() && HoverX >= 0 && Game.Buildable(HoverX, HoverY) && Game.Phase != EPDPhase::Title && Game.Phase != EPDPhase::Reward;
	const FString Key = bOk ? FString::Printf(TEXT("%d|%d|%d|%d|%d|%d"), Sel, HoverX, HoverY, (int32)Game.Towers.size(), Game.Discarded, (int32)Game.Hand.size()) : FString();
	if (Key == PreviewKey) return;
	PreviewKey = Key;
	Preview = bOk ? Game.PreviewSynergies(Sel, HoverX, HoverY) : std::vector<FPDSynergy>();
}

void SPDBoardWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	const float Dt = FMath::Min(InDeltaTime, 0.05f);
	Clock += Dt;
	if (!bTree && !bUpg && !bMenu) Game.Tick(Dt); // 트리·강화·메뉴 창을 여는 동안은 멈춘다
	TreeBuyT = FMath::Max(0.f, TreeBuyT - Dt);
	HandleEvents();
	UpdateFx(Dt);
	if (Sel >= (int32)Game.Hand.size()) Sel = -1;
	UpdatePreview();
	JGroups = Game.JokerGroups();
	// 고르는 사이 족보가 바뀌었거나(교체·다른 합성) 보상 화면이 뜨면 고르기 취소
	if (!PickCells.empty() && (Game.Phase == EPDPhase::Reward || FindPickIndex() < 0)) PickCells.clear();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPDBoardWidget::DoFuse(int32 SynergyIdx, int32 Cell)
{
	if (!Game.CanFuse(SynergyIdx)) return;
	PendingFuseCells.Reset();
	for (int K : Game.Synergies[SynergyIdx].Cells) PendingFuseCells.Add(CellCenter(K));
	if (!Game.Fuse(SynergyIdx, Cell)) PendingFuseCells.Reset();
}

namespace
{
bool SameCells(std::vector<int> A, std::vector<int> B)
{
	std::sort(A.begin(), A.end());
	std::sort(B.begin(), B.end());
	return A == B;
}
}

int32 SPDBoardWidget::FindPickIndex() const
{
	if (PickCells.empty()) return -1;
	if (bPickJoker)
	{
		for (int32 I = 0; I < (int32)JGroups.size(); ++I) if (SameCells(JGroups[I].Cells, PickCells)) return I;
		return -1;
	}
	for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		if (Game.CanFuse(I) && SameCells(Game.Synergies[I].Cells, PickCells)) return I;
	return -1;
}

void SPDBoardWidget::StartPick(const std::vector<int>& Cells, bool bJoker)
{
	PickCells = Cells;
	bPickJoker = bJoker;
	Sel = -1;
	if (!Game.bMuted) Game.Sounds.push_back({EPDSound::Move, 0, 0, 1.f});
}

void SPDBoardWidget::FinishPick(int32 Cell)
{
	const int32 Idx = FindPickIndex();
	const bool bJoker = bPickJoker;
	PickCells.clear();
	if (Idx < 0) return;
	if (!bJoker) { DoFuse(Idx, Cell); return; }
	PendingFuseCells.Reset();
	for (int K : JGroups[Idx].Cells) PendingFuseCells.Add(CellCenter(K));
	if (!Game.JokerFuse(Idx, Cell)) PendingFuseCells.Reset();
}

int32 SPDBoardWidget::TreeHoverAt(FVector2f P) const
{
	int32 Best = -1;
	float BestD = 15.f;
	for (int32 Id = 0; Id < TreeNodeCount(); ++Id)
	{
		const float D = FVector2f::Distance(P, TreeNodePos(Id));
		if (D < BestD) { BestD = D; Best = Id; }
	}
	return Best;
}

int32 SPDBoardWidget::HoveredSynergy() const
{
	if (HoverX < 0) return -1;
	const int Key = CellKey(HoverX, HoverY);
	for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		for (int K : Game.Synergies[I].Cells) if (K == Key) return I;
	return -1;
}

FVector2f SPDBoardWidget::ToDesign(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const auto Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FVector2f(((float)Local.X - ViewOffset.X) / ViewScale, ((float)Local.Y - ViewOffset.Y) / ViewScale);
}

FReply SPDBoardWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Mouse = ToDesign(MyGeometry, MouseEvent);
	if (bMenu)
	{
		const int32 Item = MenuHoverAt(Mouse);
		if (Item >= 0) MenuSel = Item;
	}
	const int32 CX = FMath::FloorToInt((Mouse.X - FX) / CS), CY = FMath::FloorToInt((Mouse.Y - FY) / CS);
	const bool bIn = CX >= 0 && CY >= 0 && CX < Cols && CY < Rows;
	HoverX = bIn ? CX : -1;
	HoverY = bIn ? CY : -1;
	return FReply::Handled();
}

void SPDBoardWidget::Click(FVector2f P, bool bRight)
{
	auto Sound = [this](EPDSound S) { if (!Game.bMuted) Game.Sounds.push_back({S, 0, 0, 1.f}); };
	if (bMenu)
	{
		const int32 Item = MenuHoverAt(P);
		if (Item >= 0) MenuSelect(Item);
		else if (!InRect(P, MenuX - 30.f, MenuItemY0 - 80.f, MenuW + 60.f, (int32)EMenuItem::Count * (MenuItemH + MenuGap) + 110.f)) bMenu = false; // 창 바깥 클릭 = 닫기
		return;
	}
	if (Game.Phase == EPDPhase::Title) { Game.Phase = EPDPhase::Build; Sound(EPDSound::Start); return; }
	if (Game.Phase == EPDPhase::Over || Game.Phase == EPDPhase::Win) { RestartRun(); return; }
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < (int32)Game.Rewards.size(); ++I)
		{
			const FVector2f R = RewardPos(I);
			if (P.X >= R.X && P.X <= R.X + RewardW && P.Y >= R.Y && P.Y <= R.Y + RewardH) { Game.TakeReward(I); return; }
		}
		return;
	}
	if (bCodex) { bCodex = false; return; }
	if (bUpg)
	{
		for (int32 U = 0; U < (int32)EPDUpgrade::Count; ++U)
			for (int32 K = 0; K < 4; ++K)
			{
				const FVector2f B = UpgButtonPos(U, K);
				if (P.X >= B.X && P.X <= B.X + UpgBtnW && P.Y >= B.Y && P.Y <= B.Y + UpgBtnH && Game.Upgrade((EPDUpgrade)U, UpgSteps[K]) == 0) Sound(EPDSound::Denied);
			}
		return;
	}
	if (bTree)
	{
		const int32 Id = TreeHoverAt(P);
		if (Id >= 0 && !Game.BuyNode(Id)) Sound(EPDSound::Denied);
		return;
	}
	// 합성 위치 고르는 중: 재료 칸 클릭 = 그 칸으로, 같은 버튼 다시 = 기본 칸, 그 밖(오른쪽 클릭 포함) = 취소
	if (!PickCells.empty())
	{
		const int32 PX = FMath::FloorToInt((P.X - FX) / CS), PY = FMath::FloorToInt((P.Y - FY) / CS);
		const int32 Key = PX >= 0 && PY >= 0 && PX < Cols && PY < Rows ? CellKey(PX, PY) : -1;
		bool bSameBtn = false;
		if (bPickJoker) { FPDJokerGroup Tmp; Tmp.Cells = PickCells; const FVector2f B = JokerButtonCenter(Tmp); bSameBtn = FMath::Abs(P.X - B.X) <= JokerBtnW / 2.f && FMath::Abs(P.Y - B.Y) <= JokerBtnH / 2.f; }
		else if (const int32 Pi = FindPickIndex(); Pi >= 0) { const FVector2f B = FuseButtonCenter(Game.Synergies, Pi); bSameBtn = FMath::Abs(P.X - B.X) <= FuseW / 2.f && FMath::Abs(P.Y - B.Y) <= FuseH / 2.f; }
		if (!bRight && bSameBtn) FinishPick(-1);
		else if (!bRight && Key >= 0 && std::find(PickCells.begin(), PickCells.end(), Key) != PickCells.end()) FinishPick(Key);
		else PickCells.clear();
		return;
	}
	if (!bRight && InRect(P, MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH)) { bMenu = true; MenuSel = 0; Sel = -1; return; }
	if (!bRight && InRect(P, TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH)) { bTree = true; Sel = -1; return; }
	if (!bRight && InRect(P, UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH)) { bUpg = true; Sel = -1; return; }
	// 조커 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)JGroups.size(); ++I)
		{
			const FVector2f B = JokerButtonCenter(JGroups[I]);
			if (FMath::Abs(P.X - B.X) <= JokerBtnW / 2.f && FMath::Abs(P.Y - B.Y) <= JokerBtnH / 2.f) { StartPick(JGroups[I].Cells, true); return; }
		}
	// 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		{
			if (!Game.CanFuse(I)) continue;
			const FVector2f B = FuseButtonCenter(Game.Synergies, I);
			if (FMath::Abs(P.X - B.X) <= FuseW / 2.f && FMath::Abs(P.Y - B.Y) <= FuseH / 2.f) { StartPick(Game.Synergies[I].Cells, false); return; }
		}
	// 먹이기 버튼 (조커에 맞닿은 족보)
	if (!bRight)
		for (const FPDFeed& Fd : Game.FeedTargets())
		{
			const FVector2f B = FeedButtonCenter(Game.Synergies, Fd.Syn, Game.CanFuse(Fd.Syn));
			if (FMath::Abs(P.X - B.X) <= FuseW / 2.f && FMath::Abs(P.Y - B.Y) <= FuseH / 2.f)
			{
				PendingFuseCells.Reset();
				for (int K : Game.Synergies[Fd.Syn].Cells) if (!Game.Towers.at(K).IsJoker()) PendingFuseCells.Add(CellCenter(K));
				if (!Game.Feed(Fd.Syn)) PendingFuseCells.Reset();
				return;
			}
		}
	int32 HandHit = -1;
	for (int32 I = 0; I < (int32)Game.Hand.size(); ++I)
	{
		const float X = HandCardX(I, (int32)Game.Hand.size());
		if (P.X >= X && P.X <= X + CardW && P.Y >= HandY - 12.f && P.Y <= HandY + CardH) HandHit = I;
	}
	if (bRight)
	{
		if (HandHit >= 0) { Game.Discard(HandHit); if (Sel == HandHit) Sel = -1; else if (Sel > HandHit) --Sel; }
		else Sel = -1;
		return;
	}
	if (HandHit >= 0) { Sel = Sel == HandHit ? -1 : HandHit; Sound(EPDSound::Move); return; }
	const int32 CX = FMath::FloorToInt((P.X - FX) / CS), CY = FMath::FloorToInt((P.Y - FY) / CS);
	if (CX >= 0 && CY >= 0 && CX < Cols && CY < Rows && Sel >= 0)
	{
		if (Game.Place(Sel, CX, CY)) Sel = -1;
		else Sound(EPDSound::Denied);
		return;
	}
	if (Game.Phase == EPDPhase::Build && InRect(P, RBtnX, StartBtnY, RBtnW, StartBtnH)) Game.StartWave();
}

void SPDBoardWidget::RestartRun()
{
	Game.NewGame(true);
	Sel = -1;
	bTree = bUpg = bCodex = bMenu = false;
	PickCells.clear();
	Particles.Empty();
	Flares.Empty();
	Shards.Empty();
	if (!Game.bMuted) Game.Sounds.push_back({EPDSound::Start, 0, 0, 1.f});
}

int32 SPDBoardWidget::MenuHoverAt(FVector2f P) const
{
	for (int32 I = 0; I < (int32)EMenuItem::Count; ++I)
	{
		const FVector2f B = MenuItemPos(I);
		if (InRect(P, B.X, B.Y, MenuW, MenuItemH)) return I;
	}
	return -1;
}

void SPDBoardWidget::MenuSelect(int32 Item)
{
	switch ((EMenuItem)Item)
	{
	case EMenuItem::Resume: bMenu = false; break;
	case EMenuItem::NewGame: RestartRun(); break;
	case EMenuItem::Quit:
		bMenu = false;
		if (OnQuit) OnQuit();
		else FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
}

FReply SPDBoardWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Click(ToDesign(MyGeometry, MouseEvent), MouseEvent.GetEffectingButton() == EKeys::RightMouseButton);
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

FReply SPDBoardWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat()) return FReply::Handled();
	const FKey K = InKeyEvent.GetKey();
	const FKey Nums[9] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	auto Sound = [this](EPDSound S) { if (!Game.bMuted) Game.Sounds.push_back({S, 0, 0, 1.f}); };
	if (bMenu)
	{
		const int32 N = (int32)EMenuItem::Count;
		if (K == EKeys::Escape) bMenu = false;
		else if (K == EKeys::Up || K == EKeys::W) MenuSel = (MenuSel + N - 1) % N;
		else if (K == EKeys::Down || K == EKeys::S) MenuSel = (MenuSel + 1) % N;
		else if (K == EKeys::Enter || K == EKeys::SpaceBar) MenuSelect(MenuSel);
		return FReply::Handled();
	}
	// 도감·트리·강화 창이 열려 있지 않고 카드도 안 골랐으면 Esc = 메뉴
	auto OpenMenu = [this]() { bMenu = true; MenuSel = 0; Sel = -1; };
	if (Game.Phase == EPDPhase::Title)
	{
		if (K == EKeys::Enter || K == EKeys::SpaceBar) { Game.Phase = EPDPhase::Build; Sound(EPDSound::Start); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (Game.Phase == EPDPhase::Over || Game.Phase == EPDPhase::Win)
	{
		if (K == EKeys::R || K == EKeys::Enter) { RestartRun(); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (bCodex) { if (K == EKeys::J || K == EKeys::Escape) bCodex = false; return FReply::Handled(); }
	if (bTree) { if (K == EKeys::K || K == EKeys::Escape) bTree = false; else if (K == EKeys::M) Game.bMuted = !Game.bMuted; return FReply::Handled(); }
	if (bUpg) { if (K == EKeys::U || K == EKeys::Escape) bUpg = false; else if (K == EKeys::M) Game.bMuted = !Game.bMuted; return FReply::Handled(); }
	if (!PickCells.empty())
	{
		if (K == EKeys::Escape) { PickCells.clear(); return FReply::Handled(); }
		if (K == EKeys::G || K == EKeys::Enter) { FinishPick(-1); return FReply::Handled(); }
	}
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < 3; ++I) if (K == Nums[I]) { Game.TakeReward(I); return FReply::Handled(); }
		if (K == EKeys::S) { Game.TakeReward(-1); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	for (int32 I = 0; I < 9; ++I)
		if (K == Nums[I])
		{
			if (I < (int32)Game.Hand.size()) { Sel = Sel == I ? -1 : I; Sound(EPDSound::Move); }
			return FReply::Handled();
		}
	if (K == EKeys::SpaceBar || K == EKeys::Enter) Game.StartWave();
	else if (K == EKeys::Escape) { if (Sel >= 0) Sel = -1; else OpenMenu(); }
	else if (K == EKeys::G) DoFuse(HoveredSynergy());
	else if (K == EKeys::J) bCodex = !bCodex;
	else if (K == EKeys::K) { bTree = true; Sel = -1; }
	else if (K == EKeys::U) { bUpg = true; Sel = -1; }
	else if (K == EKeys::F) Game.Speed = Game.Speed == 1 ? 2 : Game.Speed == 2 ? 3 : 1;
	else if (K == EKeys::M) Game.bMuted = !Game.bMuted;
	else return FReply::Unhandled();
	return FReply::Handled();
}

int32 SPDBoardWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 바탕은 칠하지 않는다: 뒤의 3D 월드(검은 바탕 + 나이아가라 + 블룸)가 비쳐 보여야 한다 (D-081, 월드 이펙트 층)
	const auto LocalSize = AllottedGeometry.GetLocalSize(); // 엔진 버전에 따라 FVector2D 또는 FVector2f
	const float SizeX = (float)LocalSize.X, SizeY = (float)LocalSize.Y;
	const float Scale = FMath::Max(0.01f, FMath::Min(SizeX / DesignW, SizeY / DesignH));
	const FVector2f Offset((SizeX - DesignW * Scale) / 2.f, (SizeY - DesignH * Scale) / 2.f);
	ViewScale = Scale;
	ViewOffset = Offset;
	ViewSize = FVector2f(SizeX, SizeY);
	const FGeometry Design = AllottedGeometry.MakeChild(FVector2f(DesignW, DesignH), FSlateLayoutTransform(Scale, Offset));
	// 판은 흔들림을 받고, 패널은 고정
	const float SK = Shake * 14.f;
	const FVector2f ShakeOff(FMath::FRandRange(-SK, SK), FMath::FRandRange(-SK, SK));
	const FGeometry Field = Design.MakeChild(FVector2f(DesignW, DesignH), FSlateLayoutTransform(ShakeOff));
	int32 Layer = LayerId;
	FCanvas C{OutDrawElements, Design, &Layer, &WhiteBrush};
	FCanvas F{OutDrawElements, Field, &Layer, &WhiteBrush};
	PaintAll(C, F);
	return Layer;
}

void SPDBoardWidget::PaintAll(FCanvas& C, FCanvas& F) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")),
					   Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);

	// ---- 판: 격자, 길, 길 위를 흐르는 에너지
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			const FVector2f P = CellPx((float)X, (float)Y);
			if (G.Path.IsPath(X, Y)) F.Box(P.X, P.Y, CS, CS, Hex(TEXT("7fd6ff"), 0.05f));
			else F.Lines(FCanvas::RectPoints(P.X + 0.5f, P.Y + 0.5f, CS - 1.f, CS - 1.f), Hex(TEXT("788cb4"), 0.06f + 0.02f * FMath::Sin(Clock * 1.5f + X * 0.4f + Y * 0.3f)), 1.f);
		}
	TArray<FVector2f> PathPts;
	for (const FPDPoint& Pt : G.Path.Points) PathPts.Add(CellPx(Pt.X, Pt.Y));
	F.Dashed(PathPts, Hex(TEXT("7fd6ff"), 0.35f), 2.f);
	{
		const int32 L = F.NextLayer();
		for (int I = 0; I < 14; ++I)
		{
			const float D = FMath::Fmod(Clock * 1.6f + I * G.Path.Length / 14.f, G.Path.Length);
			const FPDPoint Pt = G.Path.At(D);
			const FVector2f P = CellPx(Pt.X, Pt.Y);
			F.BoxAt(L, P.X - 4.f, P.Y - 4.f, 8.f, 8.f, Hex(TEXT("7fd6ff"), 0.12f));
			F.BoxAt(L, P.X - 1.5f, P.Y - 1.5f, 3.f, 3.f, Hex(TEXT("bfeaff"), 0.8f));
		}
	}
	F.Glow(FCanvas::RectPoints(FX, FY, Cols * CS, Rows * CS), Hex(TEXT("7fd6ff"), 0.5f), 1.5f, 0.6f);

	// ---- 족보 테두리 (칸 모양대로 바깥 변만) + 연쇄 번개
	const bool bPv = !Preview.empty();
	auto DrawSyn = [&](const std::vector<FPDSynergy>& List, float Alpha, bool bLightning) {
		for (int32 Si = 0; Si < (int32)List.size(); ++Si)
		{
			const FPDSynergy& S = List[Si];
			// 로열은 무지개로 천천히 돈다
			const FLinearColor Base = S.Kind == EPDHand::RoyalFlush ? FLinearColor::MakeFromHSV8((uint8)FMath::Fmod(Clock * 40.f, 256.f), 110, 255) : HandColor(S.Kind);
			const FLinearColor Col = WithAlpha(Base, Alpha);
			TSet<int32> Has;
			for (int K : S.Cells) Has.Add(K);
			const float M = 4.f + 5.f * SynLayer(List, Si); // 카드를 나눠 쓰는 족보는 안쪽 테두리
			for (int K : S.Cells)
			{
				const int X = K % Cols, Y = K / Cols;
				const bool bL = X == 0 || !Has.Contains(K - 1), bR = X == Cols - 1 || !Has.Contains(K + 1), bU = !Has.Contains(K - Cols), bD = !Has.Contains(K + Cols);
				const FVector2f P = CellPx((float)X, (float)Y);
				const float X0 = P.X + (bL ? M : 0.f), X1 = P.X + CS - (bR ? M : 0.f), Y0 = P.Y + (bU ? M : 0.f), Y1 = P.Y + CS - (bD ? M : 0.f);
				// 장판 (D-099): 족보 색 옅은 채움 13% + 45도 빗금 (x + y 가 8 의 배수인 선이라 칸끼리 이어진다)
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, 0.13f * Alpha));
				for (float D = FMath::CeilToFloat((X0 + Y0) / 8.f) * 8.f; D < X1 + Y1; D += 8.f)
				{
					const float Ax = FMath::Max(X0, D - Y1), Bx = FMath::Min(X1, D - Y0);
					if (Bx - Ax > 0.5f) F.Lines({{Ax, D - Ax}, {Bx, D - Bx}}, WithAlpha(Col, 0.3f * Alpha), 1.f);
				}
				if (bU) F.Glow({{X0, Y0}, {X1, Y0}}, Col, 2.f, 0.8f);
				if (bD) F.Glow({{X0, Y1}, {X1, Y1}}, Col, 2.f, 0.8f);
				if (bL) F.Glow({{X0, Y0}, {X0, Y1}}, Col, 2.f, 0.8f);
				if (bR) F.Glow({{X1, Y0}, {X1, Y1}}, Col, 2.f, 0.8f);
			}
			if (!bLightning) continue;
			for (int J : S.Links)
			{
				if (J < 0 || J >= (int)List.size()) continue;
				for (int A : S.Cells)
					for (int B : List[J].Cells)
					{
						if (FMath::Abs(A % Cols - B % Cols) + FMath::Abs(A / Cols - B / Cols) != 1) continue;
						const FVector2f PA = CellCenter(A), PB = CellCenter(B);
						TArray<FVector2f> Bolt{PA};
						for (int Seg = 1; Seg < 5; ++Seg)
						{
							const FVector2f Mid = PA + (PB - PA) * (Seg / 5.f);
							Bolt.Add(Mid + FVector2f(FMath::FRandRange(-6.f, 6.f), FMath::FRandRange(-6.f, 6.f)));
						}
						Bolt.Add(PB);
						F.Glow(Bolt, WithAlpha(FLinearColor::White, Alpha * (0.5f + 0.5f * Pulse)), 1.4f, 1.2f);
					}
			}
		}
	};
	DrawSyn(G.Synergies, bPv ? 0.35f : 1.f, true);
	if (bPv) DrawSyn(Preview, 0.6f + 0.4f * Pulse, true);
	for (const FCellFlash& Fl : CellFlashes)
	{
		const FVector2f P = CellPx((float)(Fl.Key % Cols), (float)(Fl.Key / Cols));
		F.Box(P.X + 2.f, P.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Fl.Col, Fl.T * 0.5f));
	}

	// ---- 타워
	for (const auto& KV : G.Towers)
	{
		const FPDTower& T = KV.second;
		if (bPv && T.X == HoverX && T.Y == HoverY) continue;
		const FVector2f P = CellCenter(KV.first);
		const float Pop = T.Age < 0.3f ? 1.f + 0.5f * (1.f - T.Age / 0.3f) : 1.f; // 놓는 순간 커졌다 줄어듦
		if (T.IsJoker())
		{
			F.JokerTower(T.Joker, P.X, P.Y, CS * 0.4f * Pop, 1.f, Pulse);
			if (T.JLevel >= 2) F.TextCentered(FString::Printf(TEXT("Lv%d"), T.JLevel), P.X, P.Y + CS * 0.5f - 5.f, WithAlpha(JokerPink, 0.9f), 9.f, true, CS - 2.f);
			continue;
		}
		F.CardShape(T.Card, P.X, P.Y, CS * 0.4f * Pop, 1.f, T.Eff.Hand >= 0, Pulse);
		// 레벨: 틀을 한 겹씩 더 (안쪽으로), 위쪽에 레벨 점
		for (int L = 2; L <= T.Level; ++L)
		{
			const float H = CS * 0.4f * Pop * 0.98f + (L - 1) * 3.5f;
			F.Glow(FCanvas::RoundRectPoints(P.X - H, P.Y - H, H * 2.f, H * 2.f, 6.f + L), WithAlpha(SuitColor(T.Card.Suit), 0.75f), 1.2f, 0.6f);
		}
		if (T.Level >= 2)
			for (int L = 0; L < T.Level; ++L) F.Box(P.X - (T.Level - 1) * 4.f + L * 8.f - 1.5f, P.Y - CS * 0.5f - 1.f, 3.f, 3.f, Hex(TEXT("ffd96a")));
	}
	// 미리보기: 고른 카드를 마우스 칸에
	if (Sel >= 0 && Sel < (int32)G.Hand.size() && HoverX >= 0 && G.Buildable(HoverX, HoverY))
	{
		const FPDCard& Card = G.Hand[Sel];
		const FVector2f P = CellCenter(CellKey(HoverX, HoverY));
		FPDTower Ghost;
		Ghost.Card = Card;
		F.Dashed(FCanvas::CirclePoints(P, TowerStats(Ghost, G.CharmMask(), &G.Mods).Range * CS, 48), Hex(TEXT("7fd6ff"), 0.45f), 1.f, 4.f, 6.f);
		F.CardShape(Card, P.X, P.Y, CS * 0.4f, 0.8f, bPv, Pulse);
	}
	else if (HoverX >= 0 && Sel < 0)
	{
		auto It = G.Towers.find(CellKey(HoverX, HoverY));
		if (It != G.Towers.end()) F.Dashed(FCanvas::CirclePoints(CellCenter(It->first), TowerStats(It->second, G.CharmMask(), &G.Mods).Range * CS, 48), Hex(TEXT("7fd6ff"), 0.45f), 1.f, 4.f, 6.f);
	}

	// ---- 적 (잔상, 몸체, 둔화 서리, 체력)
	for (const auto& KV : Trails)
	{
		const TArray<FVector2f>& T = KV.Value;
		for (int I = 1; I < T.Num(); ++I) F.Lines({T[I - 1], T[I]}, Hex(TEXT("ffa86a"), 0.35f * (1.f - I / (float)T.Num())), 4.f * (1.f - I / (float)T.Num()));
	}
	for (const FPDEnemy& E : G.Enemies)
	{
		const FPDPoint Pt = G.Path.At(E.D);
		const float HF = HitFlash.FindRef(E.Id); // 맞은 순간: 흔들리며 하얗게 번쩍
		const FVector2f P = CellPx(Pt.X, Pt.Y) + (HF > 0.f ? FVector2f(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * HF * 22.f : FVector2f(0.f, 0.f));
		const FLinearColor Col = HF > 0.f ? FMath::Lerp(EnemyColor(E.Type), FLinearColor::White, FMath::Min(1.f, HF * 7.f)) : EnemyColor(E.Type);
		const bool bBoss = EnemyDef(E.Type).bBoss;
		if (HF > 0.f) F.Box(P.X - (bBoss ? 26.f : 13.f), P.Y - (bBoss ? 26.f : 13.f), bBoss ? 52.f : 26.f, bBoss ? 52.f : 26.f, WithAlpha(FLinearColor::White, HF * 1.2f));
		auto Poly = [&](int N, float R, float Rot) {
			TArray<FVector2f> Pts;
			for (int K = 0; K <= N; ++K) { const float A = Rot + K / (float)N * TwoPi; Pts.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
			return Pts;
		};
		switch (E.Type)
		{
		case EPDEnemy::Fast: F.Glow({{P.X + 10.f, P.Y}, {P.X - 7.f, P.Y - 8.f}, {P.X - 7.f, P.Y + 8.f}, {P.X + 10.f, P.Y}}, Col, 2.f); break;
		case EPDEnemy::Tank: F.Glow(Poly(6, 14.f, Clock), Col, 2.f); break;
		case EPDEnemy::Armored:
			F.Glow(FCanvas::CirclePoints(P, 10.f, 20), Col, 2.f);
			F.Glow({{P.X - 13.f, P.Y - 14.f}, {P.X + 13.f, P.Y - 14.f}}, Col, 2.4f); // 방패
			break;
		case EPDEnemy::Splitter:
			F.Glow(FCanvas::CirclePoints(P - FVector2f(5.f, 0.f), 7.f, 16), Col, 2.f);
			F.Glow(FCanvas::CirclePoints(P + FVector2f(5.f, 0.f), 7.f, 16), Col, 2.f);
			break;
		case EPDEnemy::Regen:
			F.Glow(Poly(4, 11.f, TwoPi / 8.f), Col, 2.f);
			F.Lines({{P.X - 5.f, P.Y}, {P.X + 5.f, P.Y}}, Col, 2.f);
			F.Lines({{P.X, P.Y - 5.f}, {P.X, P.Y + 5.f}}, Col, 2.f);
			break;
		case EPDEnemy::Mini: F.Glow(FCanvas::CirclePoints(P, 6.f, 14), Col, 1.6f); break;
		case EPDEnemy::ShieldBoss: case EPDEnemy::SplitBoss: case EPDEnemy::RegenBoss:
			F.Glow(Poly(E.Type == EPDEnemy::ShieldBoss ? 4 : E.Type == EPDEnemy::SplitBoss ? 8 : 6, 22.f + 2.f * Pulse, Clock * 0.8f), Col, 2.4f, 1.4f);
			F.Glow(FCanvas::CirclePoints(P, 12.f), Col, 2.f);
			break;
		default: F.Glow(FCanvas::CirclePoints(P, 9.f, 20), Col, 2.f); break;
		}
		if (E.SlowT > 0.f) F.Lines(FCanvas::CirclePoints(P, bBoss ? 28.f : 15.f, 24), Hex(TEXT("8affc4"), 0.6f), 1.2f);
		const float W = bBoss ? 44.f : 24.f, BY = P.Y - (bBoss ? 34.f : 22.f);
		F.Box(P.X - W / 2.f, BY, W, 3.f, Hex(TEXT("ffffff"), 0.15f));
		const float HpNow = FMath::Max(0.f, E.Hp / E.MaxHp), HpOld = FMath::Max(HpNow, HpShown.FindRef(E.Id));
		if (HpOld > HpNow) F.Box(P.X - W / 2.f + W * HpNow, BY, W * (HpOld - HpNow), 3.f, Hex(TEXT("ffffff"), 0.85f)); // 방금 깎인 만큼
		F.Box(P.X - W / 2.f, BY, W * HpNow, 3.f, EnemyColor(E.Type));
		if (E.Armor > 0.f && !G.Has(EPDCharm::Pierce)) F.Box(P.X - W / 2.f, BY - 4.f, W, 2.f, Hex(TEXT("9fc3e8")));
	}

	// ---- 발사체 (미리보기 tools/neon/neon.py shots_level / shots_hand / shots_joker)
	// 무늬 기본형은 레벨(합성)마다 진화, 족보·조커 발사체는 저마다 다른 모양
	for (const FPDProjectile& Pr : G.Projectiles)
	{
		const FVector2f At0 = CellPx(Pr.X, Pr.Y);
		const float VL = FMath::Max(0.001f, FMath::Sqrt(Pr.VX * Pr.VX + Pr.VY * Pr.VY));
		const FVector2f Dir(Pr.VX / VL, Pr.VY / VL), Nrm(-Dir.Y, Dir.X);
		const float Ang = FMath::Atan2(Dir.Y, Dir.X);
		const int32 Lv = FMath::Clamp(Pr.Level, 1, MaxLevel);
		const float K = 1.f + (Pr.bHand ? 0.15f : 0.f);
		const FLinearColor Col = ShotColor(Pr.Kind, Pr.Suit), White = FLinearColor::White;
		const float T = Pr.Life;
		FVector2f At = At0;
		auto Missile = [&](FVector2f P, float S) { // 몸통 + 날개 + 흰 머리
			const FVector2f Tail = P - Dir * 10.f * S;
			F.Glow({Tail, P}, Col, 2.2f * S, 1.2f);
			F.Lines({Tail, Tail - Dir * 4.f * S + Nrm * 4.f * S}, Col, 1.4f);
			F.Lines({Tail, Tail - Dir * 4.f * S - Nrm * 4.f * S}, Col, 1.4f);
			F.Box(P.X - 1.5f, P.Y - 1.5f, 3.f, 3.f, White);
		};
		auto Diamond = [&](FVector2f C, float H, const FLinearColor& Cl) { // 진행 방향으로 긴 마름모
			F.Glow({C - Dir * H * 1.6f, C - Nrm * H, C + Dir * H * 1.6f, C + Nrm * H, C - Dir * H * 1.6f}, Cl, 1.3f, 1.f);
		};
		auto Tails = [&](FVector2f Head, float Len, float Spread, const FLinearColor& Cl) { // 꼬리 세 줄
			for (int32 J = -1; J <= 1; ++J) F.Lines({Head - Dir * 3.f + Nrm * J * 1.5f, Head - Dir * Len + Nrm * J * Spread}, WithAlpha(Cl, 0.55f), 1.2f);
		};
		switch (Pr.Kind)
		{
		case EPDShot::Missile: // 미사일 → 큰 미사일 → 세 발 떼 → 어뢰
			if (Lv == 3) for (int32 J = -1; J <= 1; ++J) Missile(At + Nrm * 7.f * J - Dir * (J ? 5.f : 0.f), 0.75f * K);
			else
			{
				Missile(At, (Lv == 1 ? 1.f : Lv == 2 ? 1.25f : 1.6f) * K);
				if (Lv >= 4) F.Glow(FCanvas::CirclePoints(At + Dir * 2.f, 4.f, 12), White, 1.2f, 1.2f);
			}
			break;
		case EPDShot::Bullet: // 탄환 → 쌍탄 → 세 줄 → 레이저 탄
			if (Lv >= 4) { F.Glow({At - Dir * 24.f * K, At}, Col, 1.8f * K, 1.3f); Diamond(At + Dir * 3.f, 3.f, Col); }
			else for (int32 J = 0; J < Lv; ++J) { const FVector2f O = Nrm * (J - (Lv - 1) / 2.f) * 3.5f; F.Glow({At - Dir * 9.f * K + O, At + O}, Col, 1.5f * K, 1.f); }
			break;
		case EPDShot::Orb: // 서리 구체 → 큰 구체 → 눈꽃 결정 → 얼음 혜성
		{
			const float R = 5.f * K * (1.f + 0.2f * (Lv - 1));
			F.Glow(FCanvas::CirclePoints(At, R, 16), Col, 1.4f, 1.2f);
			for (int32 J = 0; J < 3; ++J) { const FVector2f D = Rot2(FVector2f(R * 0.7f, 0.f), Clock * 6.f + J * TwoPi / 6.f); F.Lines({At - D, At + D}, WithAlpha(White, 0.8f), 1.f); }
			if (Lv >= 3) for (int32 J = 0; J < 6; ++J) { const FVector2f D = Rot2(FVector2f(1.f, 0.f), J * TwoPi / 6.f - Clock * 2.f); F.Lines({At + D * R * 1.25f, At + D * R * 1.7f}, Col, 1.1f); }
			if (Lv >= 4) Tails(At - Dir * R, 18.f, 5.f, Col);
			break;
		}
		case EPDShot::Star: // 별 → 별 둘 → 별 셋 → 큰 별 + 고리
			for (int32 J = 0; J < FMath::Min(Lv, 3); ++J) F.Shape(EPDSuit::Wild, At.X - Dir.X * J * 8.f, At.Y - Dir.Y * J * 8.f + (J % 2) * 2.f, 6.f * K * (1.f - 0.18f * J), White, 1.3f, 1.f);
			if (Lv >= 4) F.Glow(FCanvas::CirclePoints(At, 10.f * K, 20), White, 1.f, 0.8f);
			else F.Lines({At - Dir * 12.f * K, At - Dir * 5.f * K}, WithAlpha(White, 0.5f), 1.f);
			break;
		case EPDShot::Arrow: // 스트레이트: 화살촉 셋이 줄지어
			F.Lines({At - Dir * 22.f, At}, WithAlpha(Col, 0.6f), 1.f);
			for (int32 J = 0; J < 3; ++J) { const FVector2f Tip = At - Dir * J * 7.f; F.Glow({Tip - Dir * 5.f + Nrm * 4.f, Tip, Tip - Dir * 5.f - Nrm * 4.f}, Col, 1.4f, 1.f); }
			break;
		case EPDShot::Cluster: // 풀하우스: 분열탄 (껍데기 + 도는 알 다섯)
			F.Glow(FCanvas::CirclePoints(At, 7.f, 18), Col, 1.4f, 1.2f);
			for (int32 J = 0; J < 5; ++J) { const FVector2f D = Rot2(FVector2f(3.6f, 0.f), T * 8.f + J * TwoPi / 5.f); F.Box(At.X + D.X - 1.f, At.Y + D.Y - 1.f, 2.f, 2.f, White); }
			F.Lines({At - Dir * 18.f, At - Dir * 7.f}, WithAlpha(Col, 0.5f), 1.f);
			break;
		case EPDShot::Shell: // 포카드: 무거운 포탄
			F.Glow(FCanvas::CirclePoints(At, 8.5f, 20), Col, 1.8f, 1.4f);
			F.Lines(FCanvas::CirclePoints(At, 4.5f, 14), WithAlpha(White, 0.8f), 1.f);
			for (int32 J = -1; J <= 1; J += 2) F.Lines({At - Dir * 8.f + Nrm * J * 4.f, At - Dir * 22.f + Nrm * J * 5.f}, WithAlpha(Col, 0.6f), 1.4f);
			break;
		case EPDShot::Comet: // 스트레이트 플러시: 혜성
			F.Box(At.X - 8.f, At.Y - 8.f, 16.f, 16.f, WithAlpha(Col, 0.08f));
			F.Glow(FCanvas::CirclePoints(At, 4.5f, 14), Col, 1.6f, 1.5f);
			Tails(At, 28.f, 7.f, Col);
			break;
		case EPDShot::Nova: // 파이브 카드: 퍼져 나가는 파동
			for (int32 J = 0; J < 3; ++J) F.Glow(ArcPts(At - Dir * J * 5.f, 5.f + J * 3.f + FMath::Sin(T * 20.f) * 0.8f, Ang, 1.2f), WithAlpha(Col, 1.f - J * 0.25f), 1.4f, 1.f);
			break;
		case EPDShot::Juggle: // 광대: 포물선으로 던진 공 (그림자는 땅에)
		{
			At = At0 + FVector2f(0.f, -FMath::Sin(FMath::Clamp(T * 3.5f, 0.f, PI)) * 16.f);
			F.Box(At0.X - 2.f, At0.Y - 1.f, 4.f, 2.f, WithAlpha(Col, 0.35f));
			F.Glow(FCanvas::CirclePoints(At, 4.5f, 14), Col, 1.5f, 1.3f);
			F.Lines({At + FVector2f(-2.f, 0.f), At + FVector2f(2.f, 0.f)}, White, 1.f);
			break;
		}
		case EPDShot::Hat: // 모자 장수: 도는 실크해트
		{
			const float A = T * 12.f, S = 6.f;
			auto P = [&](float X, float Y) { return At + Rot2(FVector2f(X, Y) * S, A); };
			F.Glow({P(-1.1f, .55f), P(1.1f, .55f)}, Col, 1.4f, 1.f);
			F.Glow({P(-.65f, .55f), P(-.6f, -.9f), P(.6f, -.9f), P(.65f, .55f)}, Col, 1.4f, 1.f);
			F.Lines({P(-.63f, .2f), P(.63f, .2f)}, White, 1.f);
			break;
		}
		case EPDShot::Crescent: // 웃는 가면: 도는 웃음 초승달
		{
			const float A = T * 14.f;
			F.Glow(ArcPts(At, 7.f, A + PI / 2.f, 1.2f), Col, 1.4f, 1.1f);
			F.Lines(ArcPts(At - Rot2(FVector2f(0.f, 2.5f), A), 7.f, A + PI / 2.f, 1.0f), Col, 1.f);
			for (int32 J = -1; J <= 1; J += 2) { const FVector2f E = At + Rot2(FVector2f(J * 3.f, -3.f), A); F.Box(E.X - 1.f, E.Y - 1.f, 2.f, 2.f, White); }
			break;
		}
		case EPDShot::Twin: // 희비극: 서로 감기며 나는 두 구슬
		{
			const float W = FMath::Sin(T * 16.f) * 5.f;
			F.Glow(FCanvas::CirclePoints(At + Nrm * W, 3.5f, 12), Col, 1.4f, 1.2f);
			F.Glow(FCanvas::CirclePoints(At - Nrm * W, 3.5f, 12), Hex(TEXT("7fd6ff")), 1.4f, 1.2f);
			break;
		}
		case EPDShot::Shards: // 할리퀸: 부채꼴 마름모 셋
			for (int32 J = -1; J <= 1; ++J) Diamond(At + Nrm * J * 6.f - Dir * FMath::Abs(J) * 4.f, 3.f, J == 0 ? White : Col);
			break;
		case EPDShot::Note: // 지휘봉: 물결치며 나는 음표
		{
			At = At0 + Nrm * FMath::Sin(T * 14.f) * 4.f;
			F.Glow(FCanvas::CirclePoints(At, 3.f, 12), Col, 1.4f, 1.2f);
			F.Lines({At + FVector2f(3.f, 0.f), At + FVector2f(3.f, -11.f), At + FVector2f(7.f, -7.f)}, Col, 1.3f);
			break;
		}
		case EPDShot::Blade: // 잭: 크게 휘두른 칼날
			F.Glow(ArcPts(At - Dir * 6.f, 11.f, Ang, 1.2f), Col, 1.8f, 1.4f);
			F.Lines(ArcPts(At - Dir * 9.f, 9.f, Ang, 1.0f), WithAlpha(White, 0.8f), 1.f);
			break;
		case EPDShot::Wave: // 방울: 퍼지는 소리 고리
			for (int32 J = 0; J < 3; ++J) F.Glow(ArcPts(At - Dir * J * 5.f, 5.f + J * 3.f, Ang, 0.95f), WithAlpha(Col, 1.f - J * 0.25f), 1.3f, 1.f);
			break;
		case EPDShot::StarComet: // 스타: 큰 별 혜성
			F.Shape(EPDSuit::Wild, At.X, At.Y, 7.f, Col, 1.4f, 1.3f);
			Tails(At - Dir * 4.f, 26.f, 6.f, Col);
			break;
		default:
			F.Glow({At - Dir * 9.f, At}, Col, 1.5f, 1.f);
			break;
		}
		// 레벨 3 이상: 발사체 둘레에 옅은 빛
		if (Lv >= 3 && !IsJokerShot(Pr.Kind)) F.Box(At.X - 7.f, At.Y - 7.f, 14.f, 14.f, WithAlpha(Col, 0.06f * (Lv - 2)));
	}

	// ---- 이펙트: 레이저, 충격파, 파티클
	for (const FBeam& B : Beams)
	{
		const float A = 1.f - B.T / B.MaxT;
		const FVector2f D = (B.B - B.A).GetSafeNormal(), N(-D.Y, D.X);
		const float Len = FVector2f::Distance(B.A, B.B);
		const FLinearColor BC = WithAlpha(B.Col, A), Wh = WithAlpha(FLinearColor::White, A * 0.8f);
		switch (B.Style)
		{
		case 1: // 레일건: 쌍줄 + 줄을 감는 고리
			for (int32 J = -1; J <= 1; J += 2) F.Glow({B.A + N * J * 2.5f, B.B + N * J * 2.5f}, BC, B.Width * 0.7f, 1.2f);
			F.Lines({B.A, B.B}, Wh, B.Width * 0.4f);
			for (float S = 10.f; S < Len - 4.f; S += 13.f)
			{
				const FVector2f M = B.A + D * S;
				const float Ph = FMath::Sin(Clock * 30.f + S * 0.3f);
				F.Lines({M + N * 5.f + D * Ph * 2.f, M - N * 5.f - D * Ph * 2.f}, BC, 1.2f);
			}
			if (B.Lv >= 4) F.Glow(FCanvas::CirclePoints(B.B, 8.f + 10.f * (1.f - A), 20), BC, 1.6f, 1.f);
			break;
		case 2: // 번개: 매 프레임 새로 꺾이는 선
		{
			TArray<FVector2f> P{B.A};
			const int32 Seg = FMath::Max(3, (int32)(Len / 12.f));
			for (int32 I = 1; I < Seg; ++I) P.Add(B.A + D * (Len * I / Seg) + N * FMath::FRandRange(-7.f, 7.f));
			P.Add(B.B);
			F.Glow(P, BC, B.Width, 1.5f);
			F.Lines(P, Wh, B.Width * 0.35f);
			break;
		}
		case 3: // 로열: 굵은 금빛 쌍줄 + 목표 위 왕관
		{
			for (int32 J = -1; J <= 1; J += 2) F.Glow({B.A + N * J * 2.f, B.B + N * J * 2.f}, BC, B.Width * 0.8f, 1.6f);
			F.Lines({B.A, B.B}, Wh, B.Width * 0.5f);
			const FVector2f Cr = B.B + FVector2f(0.f, -16.f - 6.f * (1.f - A));
			const float S = 7.f;
			F.Glow({Cr + FVector2f(-S, S * .5f), Cr + FVector2f(-S, -S * .5f), Cr + FVector2f(-S * .5f, 0.f), Cr + FVector2f(0.f, -S * .75f), Cr + FVector2f(S * .5f, 0.f), Cr + FVector2f(S, -S * .5f), Cr + FVector2f(S, S * .5f), Cr + FVector2f(-S, S * .5f)}, BC, 1.4f, 1.3f);
			break;
		}
		case 4: // 체셔: 광선 + 목표에 웃는 입
			F.Glow({B.A, B.B}, BC, B.Width, 1.3f);
			F.Lines({B.A, B.B}, Wh, B.Width * 0.35f);
			F.Glow(ArcPts(B.B + FVector2f(0.f, -10.f), 9.f, PI / 2.f, 1.1f), BC, 1.5f, 1.2f);
			for (int32 J = -1; J <= 1; J += 2) F.Box(B.B.X + J * 4.f - 1.5f, B.B.Y - 15.f, 3.f, 3.f, Wh);
			break;
		default: // 보통 레이저 (레벨 2: 쌍줄)
			if (B.Lv == 2) for (int32 J = -1; J <= 1; J += 2) F.Glow({B.A + N * J * 2.f, B.B + N * J * 2.f}, BC, B.Width * 0.8f, 1.3f);
			else F.Glow({B.A, B.B}, BC, B.Width, 1.4f);
			F.Lines({B.A, B.B}, Wh, B.Width * 0.4f);
			break;
		}
	}
	// 별빛: 빛 덩어리(겹친 네모) + 돌며 줄어드는 빛살 (갈래 수는 발사체마다: 2 한 줄, 4 십자, 5 별, 6 눈꽃, 8 큰 별, 0 덩어리만)
	for (const FFlare& Fl : Flares)
	{
		if (Fl.T < 0.f) continue; // 늦게 나타나는 별빛
		const float U = Fl.T / Fl.MaxT, A = 1.f - U, Sz = Fl.Size * (0.6f + 0.6f * FMath::Sin(FMath::Min(1.f, U * 2.f) * PI * 0.5f));
		const float GA = Fl.Spokes == 0 ? 0.13f : 0.07f;
		for (int32 J = 1; J <= 3; ++J) { const float H = Sz * (Fl.Spokes == 0 ? 0.3f : 0.25f) * J; F.Box(Fl.C.X - H, Fl.C.Y - H, H * 2.f, H * 2.f, WithAlpha(Fl.Col, GA * A)); }
		if (Fl.Spokes == 2) // 한 줄: 양쪽으로 길게
		{
			const FVector2f D = Rot2(FVector2f(1.f, 0.f), Fl.Rot) * Sz;
			F.Glow({Fl.C - D, Fl.C + D}, WithAlpha(Fl.Col, A), 1.4f * A + 0.4f, 1.2f);
		}
		else
			for (int32 J = 0; J < Fl.Spokes; ++J)
			{
				const bool bShort = (Fl.Spokes == 4 || Fl.Spokes >= 6) && (J % 2);
				const FVector2f D = Rot2(FVector2f(1.f, 0.f), Fl.Rot + J * TwoPi / Fl.Spokes + U * 0.6f) * Sz * (bShort ? 0.55f : 1.f);
				F.Glow({Fl.C - D * 0.12f, Fl.C + D}, WithAlpha(Fl.Col, A), 1.4f * A + 0.4f, 1.f);
			}
	}
	// 몸 조각
	for (const FShard& Sh : Shards)
	{
		const float A = FMath::Clamp(Sh.Life / Sh.MaxLife, 0.f, 1.f);
		const FVector2f D = Rot2(FVector2f(Sh.Len * 0.5f, 0.f), Sh.A);
		F.Glow({Sh.P - D, Sh.P + D}, WithAlpha(Sh.Col, A), 1.5f, 0.9f * A);
	}
	for (const FRing& R : Rings)
	{
		if (R.T < 0.f) continue; // 늦게 퍼지는 파동
		const float U = R.T / R.MaxT, Rad = R.R0 + (R.R1 - R.R0) * (1.f - (1.f - U) * (1.f - U));
		const FLinearColor RC = WithAlpha(R.Col, 1.f - U);
		const float Th = R.Width * (1.f - U * 0.5f), A0 = R.Rot + R.Spin * R.T;
		switch (R.Shape)
		{
		case EWave::Poly: // 도는 다각형
		{
			TArray<FVector2f> P;
			for (int32 K = 0; K <= R.N; ++K) P.Add(R.C + Rot2(FVector2f(Rad, 0.f), A0 + K * TwoPi / FMath::Max(3, R.N)));
			F.Glow(P, RC, Th, 1.f);
			break;
		}
		case EWave::Suit: // 무늬 모양이 퍼진다
			F.Shape((EPDSuit)FMath::Clamp(R.N, 0, 4), R.C.X, R.C.Y, Rad, RC, Th, 1.f);
			break;
		case EWave::Broken: // 끊어진 호 조각들이 돌며 퍼진다
			for (int32 K = 0; K < FMath::Max(2, R.N); ++K)
			{
				const float Mid = A0 + K * TwoPi / FMath::Max(2, R.N);
				F.Glow(ArcPts(R.C, Rad, Mid, PI / FMath::Max(2, R.N) * 0.55f, 6), RC, Th, 1.f);
			}
			break;
		case EWave::Star: // 뾰족한 별 테두리
		{
			TArray<FVector2f> P;
			const int32 Nn = FMath::Max(3, R.N);
			for (int32 K = 0; K <= Nn * 2; ++K) P.Add(R.C + Rot2(FVector2f(K % 2 ? Rad * 0.45f : Rad, 0.f), A0 + K * PI / Nn));
			F.Glow(P, RC, Th, 1.f);
			break;
		}
		case EWave::Rays: // 사방으로 뻗어 나가는 빛줄기 (안쪽 끝도 따라 나간다)
			for (int32 K = 0; K < FMath::Max(2, R.N); ++K)
			{
				const FVector2f D = Rot2(FVector2f(1.f, 0.f), A0 + K * TwoPi / FMath::Max(2, R.N));
				F.Glow({R.C + D * Rad * (0.35f + 0.5f * U), R.C + D * Rad}, RC, Th, 1.f);
			}
			break;
		default:
			F.Glow(FCanvas::CirclePoints(R.C, Rad, 36), RC, Th, 1.f);
			break;
		}
	}
	{
		const int32 L1 = F.NextLayer(), L2 = F.NextLayer();
		for (const FParticle& Pa : Particles)
		{
			const float A = FMath::Clamp(Pa.Life / Pa.MaxLife, 0.f, 1.f), S = Pa.Size;
			F.BoxAt(L1, Pa.P.X - S * 2.f, Pa.P.Y - S * 2.f, S * 4.f, S * 4.f, WithAlpha(Pa.Col, 0.15f * A));
			if (Pa.bStreak)
			{
				const FVector2f Tail = Pa.P - Pa.V * 0.04f;
				F.Lines({Tail, Pa.P}, WithAlpha(Pa.Col, A), S);
			}
			else F.BoxAt(L2, Pa.P.X - S / 2.f, Pa.P.Y - S / 2.f, S, S, WithAlpha(Pa.Col, A));
		}
	}
	// ---- 족보 이름표 (타워 위에)
	auto DrawBadges = [&](const std::vector<FPDSynergy>& List, float Alpha) {
		for (int32 Si = 0; Si < (int32)List.size(); ++Si)
		{
			const FPDSynergy& S = List[Si];
			int Top = S.Cells[0];
			for (int K : S.Cells) if (K / Cols < Top / Cols || (K / Cols == Top / Cols && K % Cols < Top % Cols)) Top = K;
			const FVector2f P = CellPx(Top % Cols + 0.5f, (float)(Top / Cols));
			F.Badge(HandLabel(S), P.X, P.Y + 4.f + 13.f * SynLayer(List, Si), HandColor(S.Kind), Alpha);
		}
	};
	// 이름표는 마우스가 올라간 타워의 족보만 (장판 색으로 구분, D-099). 놓기 미리보기는 그대로 보여 줌
	{
		std::vector<FPDSynergy> Hov;
		const int HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		for (const FPDSynergy& S : G.Synergies) if (std::find(S.Cells.begin(), S.Cells.end(), HKey) != S.Cells.end()) Hov.push_back(S);
		if (!bPv) DrawBadges(Hov, 1.f);
	}
	if (bPv) DrawBadges(Preview, 1.f);
	// 합성 위치 고르는 중: 재료 칸을 깜빡이고, 마우스가 올라간 칸·기본 칸을 표시
	if (!PickCells.empty())
	{
		const FLinearColor PickCol = bPickJoker ? JokerPink : Gold;
		const int32 Def = DefaultFuseCell(PickCells);
		const int32 HoverKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		const bool bHoverIn = std::find(PickCells.begin(), PickCells.end(), HoverKey) != PickCells.end();
		for (int K : PickCells)
		{
			const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
			const bool bTarget = bHoverIn ? K == HoverKey : K == Def; // 지금 클릭하면 모일 칸
			F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(PickCol, bTarget ? 0.16f : 0.05f + 0.05f * Pulse));
			F.Glow(FCanvas::RoundRectPoints(C0.X + 3.f, C0.Y + 3.f, CS - 6.f, CS - 6.f, 6.f), PickCol, bTarget ? 2.f : 1.f, bTarget ? 1.2f + 0.5f * Pulse : 0.4f);
			if (K == Def) F.TextCentered(TEXT("기본"), C0.X + CS / 2.f, C0.Y + CS - 8.f, WithAlpha(PickCol, 0.8f), 9.f, true, CS - 4.f);
		}
		// 다른 칸에서 고른 칸으로 빨려 드는 선
		const int32 Target = bHoverIn ? HoverKey : Def;
		for (int K : PickCells)
			if (K != Target) F.Dashed({CellCenter(K), CellCenter(Target)}, WithAlpha(PickCol, 0.5f), 1.2f, 4.f, 5.f);
		F.Badge(bPickJoker ? TEXT("조커가 놓일 칸을 클릭 · 버튼 다시/G: 기본 칸 · Esc/오른쪽 클릭: 취소") : TEXT("합성 타워가 놓일 칸을 클릭 · 버튼 다시/G: 기본 칸 · Esc/오른쪽 클릭: 취소"),
			FX + Cols * CS / 2.f, FY + 12.f, PickCol, 1.f);
	}
	// 합성 버튼
	if (!bPv && G.Phase != EPDPhase::Reward)
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
		{
			if (!G.CanFuse(I)) continue;
			const FVector2f B = FuseButtonCenter(G.Synergies, I);
			const bool bHov = FMath::Abs(Mouse.X - B.X) <= FuseW / 2.f && FMath::Abs(Mouse.Y - B.Y) <= FuseH / 2.f;
			F.Box(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, Hex(TEXT("06060b"), 0.9f));
			F.Glow(FCanvas::RoundRectPoints(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, 6.f), Gold, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(TEXT("합성"), B.X, B.Y, WithAlpha(Gold, bHov ? 1.f : 0.8f), 10.f, true);
		}
	// 먹이기 버튼: 마우스를 올리면 먹일 칸과 얻을 경험치
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty())
		for (const FPDFeed& Fd : G.FeedTargets())
		{
			const FVector2f B = FeedButtonCenter(G.Synergies, Fd.Syn, G.CanFuse(Fd.Syn));
			const bool bHov = FMath::Abs(Mouse.X - B.X) <= FuseW / 2.f && FMath::Abs(Mouse.Y - B.Y) <= FuseH / 2.f;
			if (bHov)
			{
				for (int K : G.Synergies[Fd.Syn].Cells)
				{
					if (G.Towers.at(K).IsJoker()) continue;
					const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
					F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(JokerPink, 0.12f));
					F.Dashed({CellCenter(K), CellCenter(Fd.Joker)}, WithAlpha(JokerPink, 0.5f), 1.2f, 4.f, 5.f);
				}
				const FPDTower& Jt = G.Towers.at(Fd.Joker);
				int Lv = Jt.JLevel;
				float Xp = Jt.JXp + Fd.Xp;
				while (Lv < JokerMaxLevel && Xp >= JokerXpNeed(Lv)) { Xp -= JokerXpNeed(Lv); ++Lv; }
				const FVector2f J = CellCenter(Fd.Joker);
				F.Badge(FString::Printf(TEXT("경험치 +%.0f → 레벨 %d%s"), Fd.Xp, Lv, Lv > Jt.JLevel ? TEXT("!") : TEXT("")), J.X, J.Y - CS * 0.75f, JokerPink, 1.f);
			}
			F.Box(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, Hex(TEXT("06060b"), 0.9f));
			F.Glow(FCanvas::RoundRectPoints(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, 6.f), JokerPink, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(TEXT("먹이기"), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.8f), 10.f, true, FuseW - 4.f);
		}
	// 조커 합성 버튼 (연쇄로 이어진 족보 3개 이상)
	if (!bPv && G.Phase != EPDPhase::Reward)
		for (const FPDJokerGroup& Gr : JGroups)
		{
			const FVector2f B = JokerButtonCenter(Gr);
			const bool bHov = FMath::Abs(Mouse.X - B.X) <= JokerBtnW / 2.f && FMath::Abs(Mouse.Y - B.Y) <= JokerBtnH / 2.f;
			const bool bKnown = (G.Discovered >> (int)Gr.Result) & 1u;
			if (bHov) for (int K : Gr.Cells) { const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols)); F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(JokerPink, 0.12f)); }
			F.Box(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, 7.f), JokerPink, bHov ? 1.8f : 1.3f, 1.f + 0.8f * Pulse);
			F.TextCentered(FString::Printf(TEXT("조커: %s"), bKnown ? JokerName((int)Gr.Result) : TEXT("???")), B.X, B.Y, JokerPink, 10.f, true, JokerBtnW - 6.f);
		}
	// 타워 정보 (마우스를 올리면)
	if (HoverX >= 0 && Sel < 0 && G.Phase != EPDPhase::Reward)
	{
		auto It = G.Towers.find(CellKey(HoverX, HoverY));
		if (It != G.Towers.end())
		{
			const FPDTower& T = It->second;
			const FPDTowerStats St = TowerStats(T, G.CharmMask(), &G.Mods);
			if (T.IsJoker())
			{
				const FVector2f Pj = CellCenter(It->first) + FVector2f(CS * 0.6f, -CS * 0.6f);
				const float JX = FMath::Min(Pj.X, FX + Cols * CS - 250.f), JY = FMath::Max(Pj.Y - 60.f, FY + 4.f);
				F.Box(JX, JY, 246.f, 90.f, Hex(TEXT("06060b"), 0.92f));
				F.Lines(FCanvas::RectPoints(JX, JY, 246.f, 90.f), WithAlpha(JokerPink, 0.8f), 1.f);
				F.Text(FString::Printf(TEXT("%s 대장: %s"), HandName(CaptainHand((EPDJoker)T.Joker)), JokerName(T.Joker)), JX + 8.f, JY + 18.f, JokerPink, 12.f, 0, true, 230.f);
				F.Text(JokerAbility(T.Joker), JX + 8.f, JY + 36.f, Fg, 10.f, 0, false, 230.f);
				F.Text(JokerCommand(T.Joker), JX + 8.f, JY + 52.f, Gold, 10.f, 0, false, 230.f);
				F.Text(FString::Printf(TEXT("위력 ×%.0f · 맞닿은 족보에서 와일드"), T.Mult), JX + 8.f, JY + 68.f, Grey, 10.f, 0, false, 230.f);
				F.Text(T.JLevel >= JokerMaxLevel ? FString::Printf(TEXT("레벨 %d (최대) · 위력 +%d%%"), T.JLevel, (T.JLevel - 1) * 10)
					: FString::Printf(TEXT("레벨 %d · 경험치 %.0f/%.0f · 위력 +%d%%%s"), T.JLevel, T.JXp, JokerXpNeed(T.JLevel), (T.JLevel - 1) * 10, T.JLevel >= JokerUpLevel ? TEXT(" · 강화") : TEXT("")),
					JX + 8.f, JY + 84.f, JokerPink, 10.f, 0, false, 230.f);
			}
			else
			{
			const FVector2f P = CellCenter(It->first) + FVector2f(CS * 0.6f, -CS * 0.6f);
			const float BX = FMath::Min(P.X, FX + Cols * CS - 240.f), BYp = FMath::Max(P.Y - 66.f, FY + 4.f);
			F.Box(BX, BYp, 236.f, 70.f, Hex(TEXT("06060b"), 0.92f));
			F.Lines(FCanvas::RectPoints(BX, BYp, 236.f, 70.f), WithAlpha(SuitColor(T.Card.Suit), 0.7f), 1.f);
			F.Text(FString::Printf(TEXT("%s %s · 레벨 %d"), T.Card.IsWild() ? TEXT("와일드") : *RankText(T.Card.Rank), SuitName(T.Card.Suit), T.Level), BX + 8.f, BYp + 18.f, SuitColor(T.Card.Suit), 12.f, 0, true, 220.f);
			F.Text(FString::Printf(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), BX + 8.f, BYp + 36.f, Fg, 11.f, 0, false, 220.f);
			F.Text(FString::Printf(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *FString::Printf(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? TEXT(" · 둔화") : TEXT("")), BX + 8.f, BYp + 52.f, Grey, 11.f, 0, false, 220.f);
			if (T.Level >= 2) F.Text(FString::Printf(TEXT("특성 공격 ×%.1f 연사 ×%.1f"), T.TDmg, T.TRate), BX + 8.f, BYp + 66.f, Gold, 10.f, 0, false, 220.f);
			}
		}
	}
	for (const FFloatText& Ft : Floats)
	{
		const float U = Ft.T / Ft.MaxT, A = FMath::Min(1.f, (1.f - U) * 3.f), Pop = U < 0.12f ? 1.f + (0.12f - U) * 4.f : 1.f;
		F.TextCentered(Ft.S, Ft.P.X, Ft.P.Y - U * 30.f, WithAlpha(Ft.Col, A), Ft.Size * Pop, true);
	}
	// 배너 (웨이브 시작·통과)
	if (BannerT > 0.f)
	{
		const float U = 1.f - BannerT / BannerMax, A = FMath::Min(1.f, FMath::Min(U * 6.f, (1.f - U) * 4.f));
		const float CY = FY + Rows * CS * 0.42f, Sweep = (1.f - FMath::Min(1.f, U * 5.f)) * 120.f;
		F.Box(FX, CY - 40.f, Cols * CS, 80.f, Hex(TEXT("06060b"), 0.7f * A));
		F.Glow({{FX, CY - 40.f}, {FX + Cols * CS, CY - 40.f}}, WithAlpha(BannerCol, A), 1.5f);
		F.Glow({{FX, CY + 40.f}, {FX + Cols * CS, CY + 40.f}}, WithAlpha(BannerCol, A), 1.5f);
		F.TextCentered(BannerText, FX + Cols * CS / 2.f - Sweep, CY - 8.f, WithAlpha(BannerCol, A), 40.f, true, Cols * CS - 20.f);
		F.TextCentered(BannerSub, FX + Cols * CS / 2.f + Sweep, CY + 24.f, WithAlpha(Fg, A), 15.f, false, Cols * CS - 20.f);
	}

	// ---- 손패
	C.Text(FString::Printf(TEXT("손패 %d/%d"), (int32)G.Hand.size(), G.HandMax()), FX, HandY + 12.f, Cyan, 12.f);
	C.Text(FString::Printf(TEXT("덱 %d장"), (int32)G.Deck.size()), FX, HandY + 30.f, Dim, 11.f);
	for (int32 I = 0; I < (int32)G.Hand.size(); ++I)
	{
		const FPDCard& Card = G.Hand[I];
		const bool bSel = I == Sel;
		const float X = HandCardX(I, (int32)G.Hand.size()), Y = HandY - (bSel ? 10.f : 0.f);
		const FLinearColor Col = SuitColor(Card.Suit);
		C.Glow(FCanvas::RoundRectPoints(X, Y, CardW, CardH, 8.f), bSel ? FLinearColor::White : Col, bSel ? 2.4f : 1.4f, bSel ? 1.4f : 0.6f);
		C.CardShape(Card, X + CardW / 2.f, Y + CardH / 2.f - 6.f, 28.f, 1.f, bSel, Pulse, false);
		C.TextCentered(SuitName(Card.Suit), X + CardW / 2.f, Y + CardH - 12.f, Col, 10.f, false, CardW - 6.f);
		C.Text(FString::FromInt(I + 1), X + 6.f, Y + 13.f, Dim, 10.f);
	}

	// ---- 왼쪽 족보 패널
	PaintHandGuide(C);

	// ---- 오른쪽 패널: 웨이브·목숨·골드, 버튼 (웨이브 시작, 트리, 강화, 메뉴)
	C.Panel(RX, FY, PW, RTopH);
	C.Text(G.Phase == EPDPhase::Title ? FString(TEXT("WAVE -")) : FString::Printf(TEXT("WAVE %d"), G.Wave), RX + 12.f, FY + 28.f, Hex(TEXT("e8f0ff")), 20.f, 0, true, PW * 0.55f);
	C.Text(FString::Printf(TEXT("목숨 %d"), G.Lives), RX + PW - 12.f, FY + 28.f, G.Lives <= 5 ? Red : Green, 15.f, 2, true, PW * 0.4f);
	C.Text(FString::Printf(TEXT("골드 %d"), G.Gold), RX + 12.f, FY + 54.f, Gold, 15.f, 0, true, PW * 0.58f);
	C.Text(FString::Printf(TEXT("포인트 %d"), G.PointPrice()), RX + PW - 12.f, FY + 54.f, G.Gold >= G.PointPrice() ? Gold : Dim, 11.f, 2, false, PW * 0.36f);
	C.Text(FString::Printf(TEXT("처치 %d · 놓침 %d · 최고 %d"), G.Kills, G.Leaks, FMath::Max(G.BestWave, G.Wave)), RX + 12.f, FY + 76.f, Grey, 11.f, 0, false, PW - 24.f);
	if (G.Phase == EPDPhase::Build)
	{
		C.Glow(FCanvas::RectPoints(RBtnX, StartBtnY, RBtnW, StartBtnH), Green, 1.6f, 0.6f + 0.8f * Pulse);
		if (G.AutoStartT >= 0.f) // 자동 시작까지 남은 시간: 버튼이 차오르고 초를 센다
		{
			const float U = 1.f - FMath::Clamp(G.AutoStartT / AutoStartDelay, 0.f, 1.f);
			C.Box(RBtnX + 1.f, StartBtnY + 1.f, (RBtnW - 2.f) * U, StartBtnH - 2.f, WithAlpha(Green, 0.14f));
			C.TextCentered(FString::Printf(TEXT("웨이브 %d · %d초 뒤 (Space: 지금)"), G.Wave + 1, FMath::CeilToInt(G.AutoStartT)), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH / 2.f, Green, 12.f, true, RBtnW - 8.f);
			if (G.AutoStartT <= 3.f) // 마지막 3초는 판 위에 크게
			{
				const float Fr = FMath::Frac(G.AutoStartT);
				C.TextCentered(FString::FromInt(FMath::CeilToInt(G.AutoStartT)), FX + Cols * CS / 2.f, FY + 60.f, WithAlpha(Green, 0.35f + 0.5f * Fr), 28.f + 14.f * Fr, true);
			}
		}
		else C.TextCentered(FString::Printf(TEXT("웨이브 %d 시작 (Space)"), G.Wave + 1), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH / 2.f, Green, 13.f, true, RBtnW - 8.f);
	}
	else if (G.Phase == EPDPhase::Wave)
	{
		C.TextCentered(FString::Printf(TEXT("남은 적 %d"), (int32)G.Enemies.size()), RBtnX + RBtnW / 2.f, StartBtnY + 11.f, Gold, 13.f, true, RBtnW - 8.f);
		C.TextCentered(FString::Printf(TEXT("속도 ×%d (F)"), G.Speed), RBtnX + RBtnW / 2.f, StartBtnY + 29.f, Grey, 11.f, false, RBtnW - 8.f);
	}
	if (G.Phase != EPDPhase::Title)
	{
		const bool bCan = G.Gold >= G.PointPrice();
		C.Glow(FCanvas::RoundRectPoints(TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH, 6.f), Gold, 1.4f, bCan ? 0.6f + 0.8f * Pulse : 0.4f);
		C.TextCentered(TEXT("트리 (K)"), TreeBtnX + TreeBtnW / 2.f, TreeBtnY + TreeBtnH / 2.f, Gold, 12.f, true, TreeBtnW - 6.f);
		C.Glow(FCanvas::RoundRectPoints(UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH, 6.f), Hex(TEXT("ff9a6a")), 1.4f, 0.5f);
		C.TextCentered(TEXT("강화 (U)"), UpgOpenX + UpgOpenW / 2.f, UpgOpenY + UpgOpenH / 2.f, Hex(TEXT("ff9a6a")), 12.f, true, UpgOpenW - 6.f);
	}
	{
		const bool bHov = InRect(Mouse, MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH);
		C.Glow(FCanvas::RoundRectPoints(MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH, 6.f), Grey, 1.2f, bHov ? 0.9f : 0.35f);
		C.TextCentered(TEXT("메뉴 (Esc)"), MenuBtnX + MenuBtnW / 2.f, MenuBtnY + MenuBtnH / 2.f, bHov ? Fg : Grey, 12.f, true, MenuBtnW - 6.f);
	}

	// 부적: 이름과 설명 두 줄씩
	const float CharmY = FY + RTopH + 10.f;
	C.Panel(RX, CharmY, PW, 40.f + CharmMax * 34.f);
	C.Text(FString::Printf(TEXT("부적 %d/%d"), (int32)G.Charms.size(), CharmMax), RX + 12.f, CharmY + 22.f, Gold, 12.f, 0, true, PW - 24.f);
	if (G.Charms.empty()) C.Text(TEXT("웨이브를 막으면 보상으로 고른다"), RX + 12.f, CharmY + 46.f, Dim, 11.f, 0, false, PW - 24.f);
	for (int32 I = 0; I < (int32)G.Charms.size(); ++I)
	{
		const float Y = CharmY + 46.f + I * 34.f;
		C.Text(CharmName((int)G.Charms[I]), RX + 12.f, Y, Gold, 11.f, 0, true, PW - 24.f);
		C.Text(CharmDesc((int)G.Charms[I]), RX + 12.f, Y + 14.f, Grey, 10.f, 0, false, PW - 24.f);
	}
	C.Text(TEXT("[합성]/G 레벨 업 · 족보 3개 연쇄 → [조커] · J 도감 · 카드 클릭(숫자키) → 칸 클릭으로 설치 · 타워 위에 놓으면 교체 · 오른쪽 클릭 버리기 · F 속도 · M 소리 · Esc 메뉴"), DesignW / 2.f, DesignH - 8.f, Dim, 11.f, 1, false, DesignW - 32.f);

	// ---- 보상 화면
	if (G.Phase == EPDPhase::Reward)
	{
		C.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("06060b"), 0.9f));
		C.TextCentered(FString::Printf(TEXT("웨이브 %d 보상 — 하나를 고른다"), G.Wave), FX + Cols * CS / 2.f, FY + 112.f, Gold, 22.f, true, Cols * CS - 20.f);
		for (int32 I = 0; I < (int32)G.Rewards.size(); ++I)
		{
			const FPDReward& R = G.Rewards[I];
			const FVector2f P = RewardPos(I);
			const bool bHov = Mouse.X >= P.X && Mouse.X <= P.X + RewardW && Mouse.Y >= P.Y && Mouse.Y <= P.Y + RewardH;
			const FLinearColor Col = R.bCharm ? Gold : Cyan;
			C.Box(P.X, P.Y, RewardW, RewardH, Hex(TEXT("0a0b14")));
			C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, RewardW, RewardH, 12.f), Col, bHov ? 2.6f : 1.6f, bHov ? 1.6f + 0.6f * Pulse : 0.8f);
			C.Text(R.bCharm ? TEXT("부적") : TEXT("즉시"), P.X + 16.f, P.Y + 26.f, Col, 11.f);
			C.Text(FString::FromInt(I + 1), P.X + RewardW - 16.f, P.Y + 26.f, Dim, 11.f, 2);
			C.TextCentered(R.bCharm ? CharmName(R.Id) : InstantName(R.Id), P.X + RewardW / 2.f, P.Y + 74.f, Col, 20.f, true, RewardW - 20.f);
			C.TextCentered(R.bCharm ? CharmDesc(R.Id) : InstantDesc(R.Id), P.X + RewardW / 2.f, P.Y + 108.f, Fg, 11.f, false, RewardW - 16.f);
		}
		C.TextCentered(TEXT("1 · 2 · 3 또는 클릭으로 고르기   S: 건너뛰기"), FX + Cols * CS / 2.f, FY + 372.f, Grey, 12.f, false, Cols * CS - 20.f);
		if ((int32)G.Charms.size() >= CharmMax) C.TextCentered(TEXT("부적 칸이 가득 찼다 (5/5)"), FX + Cols * CS / 2.f, FY + 396.f, Dim, 11.f, false);
	}

	// ---- 조커 도감 (J)
	if (bCodex)
	{
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.94f));
		C.TextCentered(FString::Printf(TEXT("조커 도감  %d / 10"), FMath::CountBits(G.Discovered)), DesignW / 2.f, 48.f, JokerPink, 24.f, true);
		C.TextCentered(TEXT("연쇄로 이어진 족보 3개 (합성 타워도 기억한 족보로 침)를 합치면, 그중 가장 높은 족보의 대장 조커가 된다"), DesignW / 2.f, 74.f, Grey, 12.f, false, DesignW - 40.f);
		C.TextCentered(JokerRankText(), DesignW / 2.f, 94.f, Gold, 11.f, false, DesignW - 40.f);
		for (int32 K = 0; K < (int32)EPDJoker::Count; ++K)
		{
			const float X = 72.f + (K % 5) * 236.f, Y = 112.f + (K / 5) * 290.f;
			const bool bKnown = (G.Discovered >> K) & 1u;
			C.Glow(FCanvas::RoundRectPoints(X, Y, 214.f, 268.f, 12.f), WithAlpha(JokerPink, bKnown ? 1.f : 0.35f), 1.4f, bKnown ? 0.9f : 0.3f);
			if (bKnown) C.JokerTower(K, X + 107.f, Y + 78.f, 46.f, 1.f, Pulse);
			else
			{
				C.Glow(FCanvas::RoundRectPoints(X + 62.f, Y + 33.f, 90.f, 90.f, 10.f), WithAlpha(JokerPink, 0.35f), 1.4f, 0.3f);
				C.TextCentered(TEXT("?"), X + 107.f, Y + 78.f, WithAlpha(JokerPink, 0.5f), 40.f, true);
			}
			C.TextCentered(bKnown ? JokerName(K) : TEXT("???"), X + 107.f, Y + 150.f, WithAlpha(JokerPink, bKnown ? 1.f : 0.6f), 16.f, true, 200.f);
			C.TextCentered(FString::Printf(TEXT("%s 대장"), HandName(CaptainHand((EPDJoker)K))), X + 107.f, Y + 176.f, HandColor(CaptainHand((EPDJoker)K)), 11.f, true, 200.f);
			// 조합법은 만들기 전에도 보인다 (이름·능력만 숨김)
			C.TextCentered(JokerRecipe(K), X + 107.f, Y + 194.f, Fg, 10.f, true, 200.f);
			C.TextCentered(JokerRecipeExample(K), X + 107.f, Y + 210.f, Dim, 9.f, false, 200.f);
			C.TextCentered(bKnown ? FString(JokerAbility(K)) : FString(TEXT("만들어 보면 능력이 열린다")), X + 107.f, Y + 232.f, bKnown ? Gold : Dim, 10.f, false, 200.f);
			if (bKnown) C.TextCentered(JokerCommand(K), X + 107.f, Y + 250.f, Fg, 10.f, false, 200.f);
		}
		C.TextCentered(TEXT("J 또는 Esc 또는 클릭으로 닫기"), DesignW / 2.f, DesignH - 20.f, Dim, 11.f, false);
	}

	// ---- 골드 강화 (U)
	if (bUpg)
	{
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.95f));
		C.Text(TEXT("골드 강화"), UpgRowX, 52.f, Hex(TEXT("ff9a6a")), 22.f, 0, true);
		C.Text(FString::Printf(TEXT("골드 %d   강화에 쓴 골드 %d   (이번 판에서만 유지, 최대 %d레벨, 값 = 10 + 2 × 레벨)"), G.Gold, G.GoldUpg, UpgradeMax), UpgRowX, 82.f, Gold, 12.f, 0, false, UpgRowW);
		for (int32 U = 0; U < (int32)EPDUpgrade::Count; ++U)
		{
			const float Y = UpgRowY + U * UpgRowH;
			const int32 Lv = G.Upg[U];
			const FLinearColor Col = UpgradeColor(U);
			C.Lines(FCanvas::RectPoints(UpgRowX, Y + 4.f, UpgRowW, UpgRowH - 8.f), WithAlpha(Col, 0.25f), 1.f);
			C.Text(UpgradeName(U), UpgRowX + 16.f, Y + 24.f, Col, 15.f, 0, true, 180.f);
			C.Text(FString::Printf(TEXT("Lv %d / %d"), Lv, UpgradeMax), UpgRowX + 200.f, Y + 24.f, Lv >= UpgradeMax ? Gold : Fg, 13.f, 0, true);
			C.Text(UpgradeEffect(U, Lv), UpgRowX + 16.f, Y + 46.f, Grey, 11.f, 0, false, UpgRowW - 4.f * (UpgBtnW + 8.f) - 24.f);
			for (int32 K = 0; K < 4; ++K)
			{
				const FVector2f B = UpgButtonPos(U, K);
				const int32 Want = UpgSteps[K];
				// 최대 버튼은 지금 골드로 살 수 있는 만큼
				int32 N = 0, Cost = 0;
				if (K < 3) { N = FMath::Min(Want, UpgradeMax - Lv); Cost = G.UpgradeBatchCost((EPDUpgrade)U, N); }
				else { int32 Left = G.Gold; for (int32 L = Lv; L < UpgradeMax && Left >= UpgradeCost(L); ++L) { Left -= UpgradeCost(L); ++N; } Cost = G.Gold - Left; }
				const bool bCan = N > 0 && Cost <= G.Gold;
				const bool bHov = Mouse.X >= B.X && Mouse.X <= B.X + UpgBtnW && Mouse.Y >= B.Y && Mouse.Y <= B.Y + UpgBtnH;
				C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, UpgBtnW, UpgBtnH, 6.f), WithAlpha(Col, bCan ? 1.f : 0.3f), bHov ? 1.8f : 1.2f, bCan ? (bHov ? 1.2f : 0.5f) : 0.15f);
				C.TextCentered(K < 3 ? FString::Printf(TEXT("+%d"), Want) : FString(TEXT("최대")), B.X + UpgBtnW / 2.f, B.Y + 10.f, WithAlpha(Col, bCan ? 1.f : 0.4f), 11.f, true);
				C.TextCentered(N > 0 ? FString::Printf(TEXT("%d"), Cost) : FString(TEXT("-")), B.X + UpgBtnW / 2.f, B.Y + 22.f, bCan ? Gold : Dim, 9.f, false, UpgBtnW - 4.f);
			}
		}
		C.TextCentered(TEXT("버튼 클릭: 강화   U 또는 Esc: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, DesignH - 16.f, Dim, 11.f, false);
	}

	// ---- 스킬 트리 (K)
	if (bTree)
	{
		const FLinearColor KeyCol = Hex(TEXT("ffd26e"));
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.95f));
		C.Text(TEXT("스킬 트리"), 40.f, 40.f, KeyCol, 22.f, 0, true);
		C.Text(FString::Printf(TEXT("골드 %d   다음 포인트 %d   찍은 포인트 %d / %d   키스톤 %d/%d"), G.Gold, G.PointPrice(), G.Points, TreeNodeCount() - 1, G.KeystoneCount(), KeystoneMax), 40.f, 66.f, Gold, 12.f, 0, false, DesignW * 0.55f);
		C.Text(FString::Printf(TEXT("이번 판에서만 유지 · 포인트 값은 찍을 때마다 ×%.2f"), PointPriceGrowth), DesignW - 40.f, 40.f, Grey, 11.f, 2, false, DesignW * 0.38f);
		C.Text(TEXT("키스톤 짝은 하나만 (다른 쪽은 봉인)"), DesignW - 40.f, 58.f, Grey, 11.f, 2, false, DesignW * 0.38f);
		// 연결선
		for (int32 A = 0; A < TreeNodeCount(); ++A)
			for (int B : TreeLinks(A))
			{
				if (B < A) continue;
				const bool bOa = G.TreeOwnedNode(A), bOb = G.TreeOwnedNode(B);
				const FVector2f PA = TreeNodePos(A), PB = TreeNodePos(B);
				if (bOa && bOb) C.Glow({PA, PB}, TreeNodeColor(bOa && A != 0 ? A : B), 1.6f, 0.8f);
				else if (bOa || bOb) C.Lines({PA, PB}, Hex(TEXT("7fd6ff"), 0.35f), 1.2f);
				else C.Lines({PA, PB}, Hex(TEXT("7fd6ff"), 0.12f), 1.f);
			}
		// 노드
		const int32 Hov = TreeHoverAt(Mouse);
		for (int32 Id = 0; Id < TreeNodeCount(); ++Id)
		{
			const FPDTreeNode& N = TreeNode(Id);
			const FVector2f P = TreeNodePos(Id);
			const bool bOwn = G.TreeOwnedNode(Id), bSealed = G.TreeSealed(Id);
			const FPDGame::EBuy Can = G.CanBuy(Id);
			const bool bNext = Can == FPDGame::EBuy::Ok || Can == FPDGame::EBuy::NoGold; // 이어져 있어 찍을 수 있는 자리
			const FLinearColor Col = bSealed ? Red : TreeNodeColor(Id);
			const float Al = bOwn ? 1.f : bNext ? 0.7f : 0.3f;
			const float Gk = bOwn ? 1.f : Can == FPDGame::EBuy::Ok ? 0.5f + 0.8f * Pulse : 0.25f;
			const FLinearColor Ca = WithAlpha(Col, Al);
			const float Th = Id == Hov ? 2.f : 1.4f;
			switch (N.Kind)
			{
			case EPDNode::Start:
				C.Glow(FCanvas::CirclePoints(P, 13.f, 32), Ca, Th, Gk);
				C.Shape(EPDSuit::Wild, P.X, P.Y, 8.f, Ca, 1.2f, Gk);
				break;
			case EPDNode::Small:
				if (bOwn) C.Box(P.X - 3.f, P.Y - 3.f, 6.f, 6.f, WithAlpha(Col, 0.3f));
				C.Glow(FCanvas::CirclePoints(P, 6.f, 20), Ca, Th, Gk);
				break;
			case EPDNode::Notable:
				if (bOwn) C.Box(P.X - 12.f, P.Y - 12.f, 24.f, 24.f, WithAlpha(Col, 0.12f + 0.06f * Pulse));
				C.Glow(FCanvas::RoundRectPoints(P.X - 13.f, P.Y - 13.f, 26.f, 26.f, 4.f), Ca, Th, Gk);
				if (N.Theme < 4) C.Shape((EPDSuit)N.Theme, P.X, P.Y, 7.f, Ca, 1.2f, Gk * 0.6f);
				else C.Glow(FCanvas::CirclePoints(P, 3.5f, 16), Ca, 1.2f, Gk * 0.6f);
				break;
			case EPDNode::Keystone:
			{
				auto Oct = [&](float R) { TArray<FVector2f> O; for (int32 J = 0; J <= 8; ++J) { const float A = (22.5f + 45.f * J) * TwoPi / 360.f; O.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); } return O; };
				if (bOwn) C.Box(P.X - 10.f, P.Y - 10.f, 20.f, 20.f, WithAlpha(Col, 0.15f + 0.08f * Pulse));
				C.Glow(Oct(18.f), Ca, Th, Gk);
				C.Glow(Oct(11.f), Ca, 1.2f, Gk * 0.6f);
				if (bSealed) { C.Glow({P + FVector2f(-12.f, -12.f), P + FVector2f(12.f, 12.f)}, Red, 1.4f, 0.6f); C.Glow({P + FVector2f(12.f, -12.f), P + FVector2f(-12.f, 12.f)}, Red, 1.4f, 0.6f); }
				break;
			}
			case EPDNode::Joker:
				C.JokerTower(N.Fx[0].Param, P.X, P.Y, 15.f, Al, bOwn ? Pulse : 0.f);
				if (Can == FPDGame::EBuy::Ok) C.Glow(FCanvas::RoundRectPoints(P.X - 19.f, P.Y - 19.f, 38.f, 38.f, 6.f), WithAlpha(JokerPink, 0.5f * Pulse), 1.f, 0.4f);
				break;
			}
			if (Id == TreeBuyId && TreeBuyT > 0.f)
			{
				const float U = 1.f - TreeBuyT / 0.8f;
				C.Glow(FCanvas::CirclePoints(P, 14.f + 40.f * U, 40), WithAlpha(Col, 1.f - U), 2.f, 1.f);
			}
		}
		// 설명
		if (Hov >= 0)
		{
			const FPDTreeNode& N = TreeNode(Hov);
			const FPDGame::EBuy Can = G.CanBuy(Hov);
			FString State;
			FLinearColor SCol = Grey;
			switch (Can)
			{
			case FPDGame::EBuy::Ok: State = FString::Printf(TEXT("클릭해서 찍기 (%d 골드)"), G.PointPrice()); SCol = Green; break;
			case FPDGame::EBuy::Owned: State = TEXT("찍음"); SCol = Gold; break;
			case FPDGame::EBuy::NotLinked: State = TEXT("찍은 노드와 이어져야 한다"); break;
			case FPDGame::EBuy::Sealed: State = TEXT("짝 키스톤을 찍어서 봉인됨"); SCol = Red; break;
			case FPDGame::EBuy::KeyCap: State = FString::Printf(TEXT("키스톤은 판마다 %d개까지"), KeystoneMax); SCol = Red; break;
			case FPDGame::EBuy::NoGold: State = FString::Printf(TEXT("골드 부족 (%d / %d)"), G.Gold, G.PointPrice()); SCol = Red; break;
			default: State = TEXT("지금은 찍을 수 없다 (보상 고르는 중)"); break;
			}
			TArray<FString> Lines;
			for (const FPDNodeFx& Fx : N.Fx) if (Fx.Stat != EPDStat::None) Lines.Add(TreeFxText(Fx));
			if (N.Kind == EPDNode::Keystone) Lines.Add(FString::Printf(TEXT("짝: %s"), KeystoneName(N.Fx[0].Param ^ 1)));
			const float W = 330.f, H = 46.f + Lines.Num() * 18.f + 20.f;
			const FVector2f P = TreeNodePos(Hov);
			const float X = FMath::Clamp(P.X + 26.f, 12.f, DesignW - W - 12.f), Y = FMath::Clamp(P.Y - H / 2.f, 90.f, DesignH - H - 12.f);
			C.Box(X, Y, W, H, Hex(TEXT("0a0b14"), 0.96f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, 8.f), TreeNodeColor(Hov), 1.2f, 0.5f);
			C.Text(TreeNodeName(Hov), X + 14.f, Y + 22.f, TreeNodeColor(Hov), 14.f, 0, true, W - 28.f);
			for (int32 I = 0; I < Lines.Num(); ++I) C.Text(Lines[I], X + 14.f, Y + 46.f + I * 18.f, Fg, 11.f, 0, false, W - 28.f);
			C.Text(State, X + 14.f, Y + H - 14.f, SCol, 11.f, 0, true, W - 28.f);
		}
		C.TextCentered(TEXT("클릭: 찍기   K 또는 Esc: 닫기   (트리 창을 여는 동안 게임은 멈춘다)"), DesignW / 2.f, DesignH - 16.f, Dim, 11.f, false);
	}

	// ---- 화면 번쩍임과 덮개 화면
	if (FlashT > 0.f) C.Box(0.f, 0.f, DesignW, DesignH, WithAlpha(FlashCol, FlashT * 0.45f));
	if (G.Phase == EPDPhase::Title)
		C.Overlay(TEXT("POKER DEFENSE"), Cyan,
			{TEXT("카드 한 장이 타워 하나. 무늬는 공격 방식, 숫자는 공격력"), TEXT("붙어 있는 타워들이 포커 족보가 되면 시너지 (모양 상관없이). 족보끼리 맞닿으면 연쇄 보너스"),
				FString::Printf(TEXT("웨이브를 막을 때마다 카드 %d장. 족보는 합성해서 레벨을 올린다. 끝없이 몰려오는 적을 얼마나 버틸까!"), DrawPerWave)},
			TEXT("Enter 키 또는 화면을 눌러 시작"));
	else if (G.Phase == EPDPhase::Over)
		C.Overlay(TEXT("GAME OVER"), Red, {FString::Printf(TEXT("웨이브 %d에서 무너졌다 · 처치 %d"), G.Wave, G.Kills), FString::Printf(TEXT("최고 기록 %d웨이브%s"), G.BestWave, G.Wave >= G.BestWave ? TEXT("  (갱신!)") : TEXT(""))}, TEXT("R 키 또는 화면을 눌러 다시"));
	else if (G.Phase == EPDPhase::Win)
		C.Overlay(TEXT("ALL CLEAR"), Gold, {FString::Printf(TEXT("%d 웨이브를 모두 막았다 · 남은 목숨 %d · 처치 %d"), NumWaves, G.Lives, G.Kills)}, TEXT("R 키 또는 화면을 눌러 다시"));
	if (bMenu) PaintMenu(C);
}

void SPDBoardWidget::PaintHandGuide(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Dim = Hex(TEXT("5f6878"));
	// 고른 카드를 놓으면 생길 족보, 지금 판 위에 있는 족보를 밝게
	TSet<int32> PvKinds, OnBoard;
	for (const FPDSynergy& S : Preview) PvKinds.Add((int32)S.Kind);
	for (const FPDSynergy& S : G.Synergies) OnBoard.Add((int32)S.Kind);
	const float PH = DesignH - FY - 30.f;
	C.Panel(LX, FY, LW, PH);
	C.Text(TEXT("족보"), LX + 12.f, FY + 26.f, Cyan, 16.f, 0, true);
	C.Text(TEXT("붙어 있으면 모양 상관없이"), LX + LW - 12.f, FY + 26.f, Dim, 10.f, 2, false, LW - 70.f);
	constexpr float RowH = 50.f, Y0 = FY + 44.f;
	for (int32 I = 0; I < (int32)EPDHand::Count; ++I)
	{
		const EPDHand H = (EPDHand)I;
		const float Y = Y0 + I * RowH;
		const bool bPv = PvKinds.Contains(I), bOn = OnBoard.Contains(I);
		if (bPv) C.Box(LX + 4.f, Y, LW - 8.f, RowH - 4.f, WithAlpha(Gold, 0.08f));
		const FLinearColor NameCol = bPv ? Gold : HandColor(H);
		FString Name = HandName(H);
		if (H == EPDHand::Flush) Name += TEXT(" (3~5장)");
		if (H == EPDHand::Straight) Name += TEXT(" (2~5장)");
		C.Text(Name, LX + 22.f, Y + 16.f, WithAlpha(NameCol, bPv || bOn ? 1.f : 0.75f), 12.f, 0, true, LW - 30.f);
		C.Text(HandEffect(H), LX + 22.f, Y + 32.f, bPv ? Gold : Dim, 10.f, 0, false, LW - 30.f);
		// 판 위에 이 족보가 있으면 왼쪽에 빛나는 점
		if (bOn || bPv) C.Glow(FCanvas::CirclePoints(FVector2f(LX + 11.f, Y + 12.f), 3.f, 10), NameCol, 1.2f, bPv ? 1.2f : 0.7f);
	}
	const float FootY = Y0 + (int32)EPDHand::Count * RowH + 8.f;
	C.Text(TEXT("연쇄"), LX + 12.f, FootY, FLinearColor::White, 12.f, 0, true);
	C.Text(FString::Printf(TEXT("족보끼리 맞닿으면 하나에 +%d%%씩"), G.Has(EPDCharm::ChainLink) ? 45 : 25), LX + 12.f, FootY + 16.f, Hex(TEXT("cfd8e6")), 10.f, 0, false, LW - 24.f);
	// 무늬 = 공격 방식
	const float SuitY = FootY + 40.f;
	for (int32 I = 0; I < 4; ++I)
	{
		const EPDSuit S = (EPDSuit)I;
		const float X = LX + 12.f + (I % 2) * 108.f, Y = SuitY + (I / 2) * 20.f;
		C.Shape(S, X + 6.f, Y, 6.f, SuitColor(S), 1.3f, 0.6f);
		C.Text(SuitName(S), X + 18.f, Y + 4.f, SuitColor(S), 11.f, 0, false, 86.f);
	}
}

void SPDBoardWidget::PaintMenu(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.8f));
	const int32 N = (int32)EMenuItem::Count;
	const float BoxY = MenuItemY0 - 80.f, BoxH = N * (MenuItemH + MenuGap) + 110.f;
	C.Box(MenuX - 30.f, BoxY, MenuW + 60.f, BoxH, Hex(TEXT("06060b"), 0.9f));
	C.Glow(FCanvas::RoundRectPoints(MenuX - 30.f, BoxY, MenuW + 60.f, BoxH, 10.f), Cyan, 1.4f, 0.7f);
	C.TextCentered(TEXT("메뉴"), DesignW / 2.f, BoxY + 40.f, Cyan, 24.f, true);
	for (int32 I = 0; I < N; ++I)
	{
		const FVector2f B = MenuItemPos(I);
		const bool bSel = I == MenuSel;
		const FLinearColor Col = (EMenuItem)I == EMenuItem::Quit ? Red : Cyan;
		if (bSel) C.Box(B.X, B.Y, MenuW, MenuItemH, WithAlpha(Col, 0.08f));
		C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, MenuW, MenuItemH, 8.f), Col, bSel ? 1.8f : 1.2f, bSel ? 1.0f + 0.5f * Pulse : 0.4f);
		C.TextCentered(MenuItemName(I), B.X + MenuW / 2.f, B.Y + MenuItemH / 2.f, bSel ? FLinearColor::White : Fg, 16.f, true);
	}
	C.TextCentered(TEXT("↑↓ · Enter 또는 클릭   Esc: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, BoxY + BoxH - 18.f, Dim, 11.f, false);
}
