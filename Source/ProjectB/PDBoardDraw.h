// 포커 디펜스 화면의 그리기 도우미 (PDBoardWidget·PDBoardFx·PDBoardPaint 가 같이 쓴다)
// - PDDraw: 1280 x 720 설계 좌표의 배치 상수, 색, 글자 표, 버튼 위치
// - SPDBoardWidget::FCanvas: Slate 그리기 요소를 만드는 붓 (네온 선, 글자, 무늬·카드·조커 그림)
// 화풍은 docs/15-art-style.md (거의 검은 바탕, 채우지 않은 한 색 가는 선, 넓고 부드러운 번짐)
#pragma once
#include "PDLang.h"

#include "PDBoardWidget.h"
#include "PDIcons.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

#include <algorithm>

using namespace PD;

namespace PDDraw
{
constexpr float DesignW = 1280.f;
constexpr float DesignH = 720.f;
// 화면 세 칸: 왼쪽 족보 패널 | 가운데 판과 손패 | 오른쪽 상태 패널
constexpr float LX = 16.f, LW = 232.f; // 왼쪽 족보 패널
constexpr float CS = 48.f;          // 칸 크기
constexpr float FX = LX + LW + 14.f; // 판 왼쪽 위
constexpr float HudY = 12.f, HudH = 50.f;  // 위쪽 상태 띠 (D-119): 웨이브·남은 적·목숨·골드·판 위력·속도·설정
constexpr float FY = 74.f;
constexpr float HandRowH = 38.f, HandRowY0 = FY + 36.f; // 왼쪽 족보 목록 한 줄 (D-148: 클릭하면 필터)
constexpr int32 JokerFilterId = (int32)EPDHand::Count;   // 족보 목록 맨 아래 '조커' 줄 (D-152): 조커 합성 버튼을 모두 펼친다
constexpr int32 HandRowsN = (int32)EPDHand::Count + 1;
constexpr float HandY = FY + Rows * CS + 14.f;
constexpr float SwapBtnX = FX, SwapBtnY = HandY + 54.f, SwapBtnW = 86.f, SwapBtnH = 28.f; // 자리바꿈 버튼 (D-149, 덱 글자 아래)
constexpr float CardW = 70.f, CardH = 98.f;
constexpr float RX = FX + Cols * CS + 14.f; // 오른쪽 패널
constexpr float PW = DesignW - RX - 16.f;   // 오른쪽 패널 너비
constexpr float RBtnX = RX, RBtnW = PW;     // 오른쪽 패널 버튼 줄
constexpr float PowCardH = 250.f;           // 판 위력 카드 (오른쪽 맨 위). 딜러 7위까지 (D-164: 부적 칸을 줄이고 늘림)
constexpr int32 PowTopN = 7;
constexpr float StartBtnY = FY + PowCardH + 10.f, StartBtnH = 46.f;
constexpr float SpeedBtnX = 1184.f, SetOpenX = 1222.f, HudBtnY = 22.f, HudBtnS = 30.f; // 위쪽 띠 오른쪽 끝 버튼
constexpr float HelpX = DesignW - 110.f, HelpY = DesignH - 22.f, HelpW = 94.f, HelpH = 18.f; // "? 도움말" (마우스를 올리면 키 안내)
constexpr float TwoPi = 6.2831853f;
constexpr float YoungAge = 0.35f; // 막 놓인 타워 (커졌다 줄어드는 동안은 움직이는 층에서 그림, D-137)

inline FLinearColor Hex(const TCHAR* InHex, float Alpha = 1.f)
{
	FLinearColor C(FColor::FromHex(InHex));
	C.A = Alpha;
	return C;
}
inline FLinearColor WithAlpha(FLinearColor C, float Alpha)
{
	C.A *= Alpha;
	return C;
}
// 카드 색 (D-154, 설정): 0 네온 (무늬마다 색), 1 흑백 (모두 흰빛), 2 빨강·하양 (♥♦ 빨강, ♠♣ 흰빛)
inline int32 GPDCardTheme = 0;
inline bool GPDBakeWhiteSuits = false; // 아틀라스에 카드를 흰색으로 굽는 동안 (그릴 때 무늬 색으로 물들인다)
inline FLinearColor SuitColor(EPDSuit S)
{
	if (GPDBakeWhiteSuits) return FLinearColor::White;
	if (GPDCardTheme == 1) return Hex(TEXT("e6ebf5"));
	if (GPDCardTheme == 2) return S == EPDSuit::Heart || S == EPDSuit::Diamond ? Hex(TEXT("ff5a6a")) : Hex(TEXT("eef2ff"));
	switch (S)
	{
	case EPDSuit::Heart: return Hex(TEXT("ff6a8a"));
	case EPDSuit::Diamond: return Hex(TEXT("ffd96a"));
	case EPDSuit::Club: return Hex(TEXT("8affc4"));
	case EPDSuit::Wild: return Hex(TEXT("ffffff"));
	default: return Hex(TEXT("7fb8ff"));
	}
}
inline const TCHAR* SuitName(EPDSuit S)
{
	static const TCHAR* N[] = {TEXT("저격"), TEXT("폭발"), TEXT("연사"), TEXT("둔화"), TEXT("와일드")};
	return Tr(N[(int)S]);
}
inline FLinearColor HandColor(EPDHand H)
{
	// 족보마다 다른 색 (D-099, 장판·테두리·패널 공통). 로열은 판 위 장판에서 무지개로 돈다
	static const TCHAR* C[] = {TEXT("fff4d6"), TEXT("ffd96a"), TEXT("ff9ad5"), TEXT("ff6a6a"), TEXT("ffa86a"), TEXT("b48cff"), TEXT("6a8cff"), TEXT("9cffb0"), TEXT("7fd6ff"), TEXT("cfd8e6")};
	return Hex(C[(int)H]);
}
inline const TCHAR* HandName(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("로열 스트레이트 플러시"), TEXT("파이브 카드"), TEXT("스트레이트 플러시"), TEXT("포카드"), TEXT("풀하우스"), TEXT("플러시"), TEXT("스트레이트"), TEXT("트리플"), TEXT("투 페어"), TEXT("원 페어")};
	return Tr(N[(int)H]);
}
inline const TCHAR* HandEffect(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("공격력 ×2.5 · 연사 ×2.5 · 넓은 범위"), TEXT("공격력 ×5 (와일드·조커가 있어야)"), TEXT("공격력 ×2 · 연사 ×2 · 범위"), TEXT("공격력 ×3.5"), TEXT("공격력 ×1.5 · 연사 ×1.5 · 범위"), TEXT("범위 공격 (장수만큼 넓게)"),
		TEXT("연사 ×1.25 / ×1.5 / ×1.75 / ×2"), TEXT("공격력 ×2"), TEXT("공격력 ×1.5 · 사거리 ×1.25"), TEXT("공격력 ×1.5")};
	return Tr(N[(int)H]);
}
inline FString HandLabel(const FPDSynergy& S)
{
	FString L = HandName(S.Kind);
	if (S.Kind == EPDHand::Flush || S.Kind == EPDHand::Straight) L += FString::FromInt(S.N);
	if (S.Chain >= 2) L += TrF(TEXT(" 연쇄×%s"), *FString::SanitizeFloat(1.f + ChainStep * (S.Chain - 1)));
	return L;
}
inline FString RankText(int R)
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
inline FLinearColor EnemyColor(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::Fast: return Hex(TEXT("ffa86a"));
	case EPDEnemy::Tank: return Hex(TEXT("c49aff"));
	case EPDEnemy::Armored: case EPDEnemy::ShieldBoss: return Hex(TEXT("9fc3e8"));
	case EPDEnemy::Splitter: case EPDEnemy::Mini: case EPDEnemy::SplitBoss: return Hex(TEXT("ff9ad5"));
	case EPDEnemy::Regen: case EPDEnemy::RegenBoss: return Hex(TEXT("b6ff6a"));
	// 새 보스 (D-112, 미리보기 docs/images/bosses_preview.png)
	case EPDEnemy::Charger: return Hex(TEXT("ffa86a"));
	case EPDEnemy::Summoner: return Hex(TEXT("c49aff"));
	case EPDEnemy::Phantom: return Hex(TEXT("cfe4ff"));
	case EPDEnemy::Warlord: return Hex(TEXT("ffd96a"));
	case EPDEnemy::Berserker: return Hex(TEXT("ff6a6a"));
	case EPDEnemy::Blinker: return Hex(TEXT("7fd6ff"));
	case EPDEnemy::Devourer: return Hex(TEXT("e07aff"));
	case EPDEnemy::Warden: return Hex(TEXT("a8f0ff"));
	case EPDEnemy::Hexer: return Hex(TEXT("9a7aff"));
	case EPDEnemy::Breaker: return Hex(TEXT("ffcf5a"));
	default: return Hex(TEXT("e8f0ff"));
	}
}
inline const TCHAR* BossName(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::ShieldBoss: return Tr(TEXT("방패 거인"));
	case EPDEnemy::SplitBoss: return Tr(TEXT("분열 군주"));
	case EPDEnemy::RegenBoss: return Tr(TEXT("재생 왕"));
	case EPDEnemy::Charger: return Tr(TEXT("돌진 기사"));
	case EPDEnemy::Summoner: return Tr(TEXT("소환사"));
	case EPDEnemy::Phantom: return Tr(TEXT("유령"));
	case EPDEnemy::Warlord: return Tr(TEXT("군단장"));
	case EPDEnemy::Berserker: return Tr(TEXT("분노 거인"));
	case EPDEnemy::Blinker: return Tr(TEXT("공간 도약자"));
	case EPDEnemy::Devourer: return Tr(TEXT("흡수자"));
	case EPDEnemy::Warden: return Tr(TEXT("수호자"));
	case EPDEnemy::Hexer: return Tr(TEXT("저주술사"));
	case EPDEnemy::Breaker: return Tr(TEXT("파괴자"));
	default: return Tr(TEXT(""));
	}
}
// 보스 능력 한 줄 (배너)
inline const TCHAR* BossSkillText(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::ShieldBoss: return Tr(TEXT("두꺼운 방패: 한 방 피해를 깎는다"));
	case EPDEnemy::SplitBoss: return Tr(TEXT("죽으면 탱크 넷으로 갈라진다"));
	case EPDEnemy::RegenBoss: return Tr(TEXT("체력이 계속 차오른다"));
	case EPDEnemy::Charger: return Tr(TEXT("4초마다 3배 속도로 돌진"));
	case EPDEnemy::Summoner: return Tr(TEXT("3초마다 졸개 둘을 부른다"));
	case EPDEnemy::Phantom: return Tr(TEXT("4초마다 1.2초 동안 무적"));
	case EPDEnemy::Warlord: return Tr(TEXT("주변 적이 빠르고 단단해진다"));
	case EPDEnemy::Berserker: return Tr(TEXT("체력이 줄수록 빨라진다"));
	case EPDEnemy::Blinker: return Tr(TEXT("5초마다 앞으로 순간이동"));
	case EPDEnemy::Devourer: return Tr(TEXT("주변 적이 죽으면 회복"));
	case EPDEnemy::Warden: return Tr(TEXT("6초마다 보호막"));
	case EPDEnemy::Hexer: return Tr(TEXT("주변 타워 공격력 -35%"));
	case EPDEnemy::Breaker: return Tr(TEXT("6초마다 가까운 타워를 기절시킨다"));
	default: return Tr(TEXT(""));
	}
}
inline const TCHAR* CharmName(int J)
{
	static const TCHAR* N[] = {TEXT("하트 애호가"), TEXT("스페이드 애호가"), TEXT("다이아 애호가"), TEXT("클로버 애호가"), TEXT("연결고리"), TEXT("쌍둥이"), TEXT("일직선"),
		TEXT("넓은 물결"), TEXT("그림 카드"), TEXT("작은 수"), TEXT("행운의 7"), TEXT("큰 손"), TEXT("보급"), TEXT("재활용"), TEXT("조준경"), TEXT("서리"), TEXT("연쇄 폭발"), TEXT("관통탄")};
	return Tr(N[J]);
}
inline const TCHAR* CharmDesc(int J)
{
	static const TCHAR* N[] = {TEXT("♥ 타워 공격력 +40%"), TEXT("♠ 타워 공격력 +40%"), TEXT("♦ 타워 공격력 +40%"), TEXT("♣ 타워 공격력 +40%"), TEXT("연쇄 보너스 +25% → +45%"),
		TEXT("원 페어·투 페어 ×1.5 → ×2"), TEXT("스트레이트 연사 한 장마다 +0.25 더"), TEXT("플러시 범위 +0.75칸"), TEXT("J·Q·K 타워 공격력 +60%"), TEXT("2~6 타워 연사 +60%"),
		TEXT("숫자 7 타워 공격력 ×2.5"), TEXT("손패 최대 +2"), TEXT("웨이브마다 카드 +1"), TEXT("카드를 버리면 1장 뽑기"), TEXT("모든 타워 사거리 +15%"),
		TEXT("둔화 더 강하게, ♣ 주변도 둔화"), TEXT("처치 시 주변 적에게 최대 체력 15% 피해"), TEXT("적의 방패를 무시")};
	return Tr(N[J]);
}
inline const TCHAR* JokerName(int K)
{
	static const TCHAR* N[] = {TEXT("광대"), TEXT("모자 장수"), TEXT("웃는 가면"), TEXT("희비극"), TEXT("할리퀸"), TEXT("지휘봉"), TEXT("잭"), TEXT("체셔"), TEXT("방울"), TEXT("스타")};
	return Tr(N[K]);
}
inline const TCHAR* JokerAbility(int K)
{
	static const TCHAR* N[] = {TEXT("무늬 4종 공격을 번갈아 씀"), TEXT("주변 8칸 타워 공격력 +25%·사거리 +10%"), TEXT("주변 가장 강한 타워의 공격을 따라 함"),
		TEXT("두 적을 동시에 공격"), TEXT("판 위 같은 무늬 타워 전부 공격력 +30%"), TEXT("주변 8칸 타워 연사 +40%"), TEXT("아주 강한 한 방 (공격력 ×3, 연사 ×0.5)"),
		TEXT("사거리 무한: 가장 앞선 적 저격"), TEXT("8초마다 종: 사거리 안 적 1.5초 멈춤"), TEXT("판 전체 레이저: 모든 적에게 피해")};
	return Tr(N[K]);
}
// 조커 레시피 (D-115): 코어 JokerRecipe 의 족보 3개를 그대로 보여 준다
inline const TCHAR* HandShort(EPDHand H) { return Tr(H == EPDHand::RoyalFlush ? TEXT("로열") : H == EPDHand::StraightFlush ? TEXT("스티플") : HandName(H)); } // 도감 칸이 좁아서
inline FString JokerRecipeText(int K)
{
	const auto& R = JokerRecipe((EPDJoker)K);
	return TrF(TEXT("3개: %s + %s + %s"), HandShort(R[0]), HandShort(R[1]), HandShort(R[2]));
}
inline FString JokerRecipe2Text(int K)
{
	const auto& R = JokerRecipe2((EPDJoker)K);
	return TrF(TEXT("2개: %s + %s"), HandShort(R[0]), HandShort(R[1]));
}
inline FString JokerRuleText() { return TEXT("맞닿은 족보 3개(또는 2개)가 레시피와 맞으면 조커 · 3개 쪽이 카드가 많아 세다 · 숫자가 겹치면 큰 족보로 합쳐지니 주의"); }
inline FString JokerCommand(int K) { return TrF(TEXT("지휘: %s 타워 위력 +30%% (겹침)"), HandName(CaptainHand((EPDJoker)K))); }
const FLinearColor JokerPink = FLinearColor(FColor::FromHex(TEXT("ff9ad5")));
const FLinearColor UiEdge = FLinearColor(FColor::FromHex(TEXT("7fd6ff"))); // UI 창·패널·버튼 테두리는 모두 이 색 (D-160)
const FLinearColor UiTitle = UiEdge;                                         // UI 글자 (D-163): 패널 제목·이름표는 하늘색
const FLinearColor UiValue = FLinearColor(FColor::FromHex(TEXT("eef3ff"))); //   값·버튼 글자는 흰빛, 설명은 회색 (뜻이 있는 색만 예외)
// 발사체 색: 무늬 기본형은 무늬 색, 족보 발사체는 족보 색, 조커는 분홍
inline FLinearColor ShotColor(EPDShot K, EPDSuit S)
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
inline FVector2f Rot2(FVector2f V, float A) { const float C = FMath::Cos(A), S = FMath::Sin(A); return FVector2f(V.X * C - V.Y * S, V.X * S + V.Y * C); }
// 원호 점들: 중심 C, 반지름 R, 가운데 각 Mid, 양쪽 반폭 Half (라디안)
inline TArray<FVector2f> ArcPts(FVector2f C, float R, float Mid, float Half, int32 N = 12)
{
	TArray<FVector2f> P;
	for (int32 I = 0; I <= N; ++I) { const float A = Mid - Half + 2.f * Half * I / N; P.Add(C + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
	return P;
}
inline const TCHAR* InstantName(int I)
{
	static const TCHAR* N[] = {TEXT("와일드 카드"), TEXT("보충"), TEXT("수리"), TEXT("골드"), TEXT("자리바꿈")};
	return Tr(N[I]);
}
inline const TCHAR* InstantDesc(int I)
{
	static const TCHAR* N[] = {TEXT("아무 숫자·무늬로 치는 카드 1장 (드물게 나옴)"), TEXT("카드 2장 뽑기"), TEXT("목숨 +3"), TEXT("골드 +(10 + 웨이브 × 4)"), TEXT("타워 둘의 자리를 바꾸거나 빈 칸으로 옮기기 2번 (레벨·조커 그대로)")};
	return Tr(N[I]);
}
// 보상 카드 위치 (판 가운데 3장)
inline FVector2f RewardPos(int I) { return FVector2f(FX + Cols * CS / 2.f - 360.f + I * 250.f, FY + 160.f); }
constexpr float RewardW = 220.f, RewardH = 170.f;
inline FVector2f CellPx(float X, float Y) { return FVector2f(FX + X * CS, FY + Y * CS); } // 칸 좌표(실수) → 화면
inline FVector2f CellCenter(int Key) { return CellPx(Key % Cols + 0.5f, Key / Cols + 0.5f); }
// 조커 합성 버튼: 묶음 맨 위 칸(같으면 왼쪽)의 위 변 가운데
constexpr float JokerBtnW = 84.f, JokerBtnH = 18.f;
inline FVector2f JokerButtonCenter(const FPDJokerGroup& G)
{
	int Top = G.Cells[0];
	for (int K : G.Cells) if (K / Cols < Top / Cols || (K / Cols == Top / Cols && K % Cols < Top % Cols)) Top = K;
	return CellPx(Top % Cols + 0.5f, (float)(Top / Cols)) - FVector2f(0.f, 2.f);
}
// 조커 합치기 버튼 (D-114): 같은 조커 묶음 맨 아래 칸(같으면 왼쪽)의 아래 변 가운데
constexpr float MergeW = 76.f, MergeH = 16.f;
inline FVector2f MergeButtonCenter(const std::vector<int>& Cells)
{
	int Bottom = Cells[0];
	for (int K : Cells) if (K / Cols > Bottom / Cols || (K / Cols == Bottom / Cols && K % Cols < Bottom % Cols)) Bottom = K;
	return CellPx(Bottom % Cols + 0.5f, Bottom / Cols + 1.f) - FVector2f(0.f, 4.f);
}
// 합성 버튼: 족보 맨 아래 칸(같으면 왼쪽)의 아래 변 가운데
constexpr float FuseW = 46.f, FuseH = 16.f; // 먹이기 버튼 크기 (합성 버튼 높이도)
// 합성 버튼 글자 (D-122): "합성 · 원 페어" 처럼 족보 이름을 같이. 너비는 글자 수로 어림 (한글 10, 그 밖 6)
inline FPDSynergy FuseSyn(const FPDGame& G, int I) { FPDSynergy S = G.Synergies[I]; G.FuseSynergy(I, S); return S; } // 실제로 합쳐질 족보 (조커 뺀 것, D-128)
inline FString FuseLabel(const FPDSynergy& S) { return TrF(TEXT("합성 · %s"), HandShort(S.Kind)); }
inline float FuseBtnW(const FPDSynergy& S)
{
	float W = 14.f;
	for (const TCHAR Ch : FuseLabel(S)) W += Ch >= 0x1100 ? 10.f : 6.f;
	return W;
}
// 겹 번호: 카드를 나눠 쓰는 앞선 족보 수 (0~2). 테두리를 안쪽으로 들이고 이름표·합성 버튼을 아래로 민다
inline int SynLayer(const std::vector<FPDSynergy>& L, int I)
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
// 평소(마우스가 없을 때) 합성할 수 있다는 작은 점 자리: 족보 맨 아래 칸의 아래 변 가운데
inline FVector2f FuseDotCenter(const std::vector<FPDSynergy>& L, int I)
{
	const FPDSynergy& S = L[I];
	int Bottom = S.Cells[0];
	for (int K : S.Cells) if (K / Cols > Bottom / Cols || (K / Cols == Bottom / Cols && K % Cols < Bottom % Cols)) Bottom = K;
	return CellPx(Bottom % Cols + 0.5f, Bottom / Cols + 1.f) - FVector2f(0.f, 4.f) + FVector2f(0.f, SynLayer(L, I) * (FuseH + 2.f));
}
// 합성 버튼 (D-121): 마우스를 올리면 족보 칸들의 가운데에 뜬다. 카드를 나눠 쓰는 족보끼리는 겹 번호만큼 아래로
inline FVector2f FuseButtonCenter(const std::vector<FPDSynergy>& L, int I)
{
	FVector2f C(0.f, 0.f);
	for (int K : L[I].Cells) C += CellPx(K % Cols + 0.5f, K / Cols + 0.5f);
	return C / (float)FMath::Max<int>(1, (int)L[I].Cells.size()) + FVector2f(0.f, SynLayer(L, I) * (FuseH + 4.f));
}

// 먹이기 버튼: 합성 버튼 오른쪽 (합성 버튼이 없으면 그 자리)
// 먹이기 버튼 (D-129, D-131): "먹이기 · 플러시" 처럼 족보 이름. 조커를 클릭한 먹이기 모드에서만, 족보 가운데
inline FString FeedLabel(const FPDSynergy& S) { return TrF(TEXT("먹이기 · %s"), HandShort(S.Kind)); }
inline float FeedBtnW(const FPDSynergy& S)
{
	float W = 14.f;
	for (const TCHAR Ch : FeedLabel(S)) W += Ch >= 0x1100 ? 10.f : 6.f;
	return W;
}
inline FVector2f FeedButtonCenter(const FPDGame& G, int I) { return FuseButtonCenter(G.Synergies, I); } // 먹이기 모드에선 합성 버튼이 없어 족보 가운데

// ---- 스킬 트리 글자와 위치 (docs/16-skill-tree.md). 위치는 생성기가 화면 좌표로 넣어 둔다
inline FVector2f TreeNodePos(int Id) { return FVector2f(TreeNode(Id).X, TreeNode(Id).Y); }
inline const TCHAR* KeystoneName(int K)
{
	static const TCHAR* N[] = {TEXT("외골수"), TEXT("잡식가"), TEXT("큰 판"), TEXT("작은 판"), TEXT("대장장이"), TEXT("광대의 길"), TEXT("고독한 저격수"), TEXT("군중")};
	return Tr(N[K]);
}
inline const TCHAR* KeystoneDesc(int K)
{
	static const TCHAR* N[] = {TEXT("플러시 타워 위력 ×2 / 다른 족보 타워 위력 ×0.5"), TEXT("플러시 아닌 족보 위력 ×1.3 / 플러시 범위 효과 없음"),
		TEXT("5장 족보 위력 ×1.6 / 2장 족보는 효과·연쇄 없음"), TEXT("원 페어·투 페어 위력 ×1.8 / 5장 족보는 연쇄 덤 없음"),
		TEXT("합성 배율 1.5 → 2.0 / 조커 포탑을 만들 수 없음"), TEXT("조커 배율 1.2 → 1.6 / 합성은 레벨 2까지"),
		TEXT("족보에 안 든 타워 위력 ×1.6 / 연쇄 덤 없음"), TEXT("연쇄 덤 +25% → +40% / 족보에 안 든 타워 위력 ×0.5")};
	return Tr(N[K]);
}
inline const TCHAR* JokerUpDesc(int K)
{
	static const TCHAR* N[] = {TEXT("연사 +50%"), TEXT("오라 공격력 +25% → +50%, 범위 주변 24칸"), TEXT("따라 한 공격 위력 ×1.3"), TEXT("셋째 적까지 함께 공격"),
		TEXT("같은 무늬 공격력 +30% → +60%"), TEXT("주변 연사 +40% → +70%"), TEXT("공격력 ×3 → ×5"), TEXT("위력 ×1.5"), TEXT("5초마다 2초 멈춤 (원래 8초마다 1.5초)"),
		TEXT("레이저 위력 ×0.5 → ×0.8")};
	return Tr(N[K]);
}
inline FString HandGroupName(int Mask)
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
inline FString TreeFxText(const FPDNodeFx& F)
{
	static const TCHAR* Sym[] = {TEXT("♠"), TEXT("♥"), TEXT("♦"), TEXT("♣"), TEXT("★")};
	const int Pct = FMath::RoundToInt(F.Value * 100.f);
	switch (F.Stat)
	{
	case EPDStat::SuitDmg: return TrF(TEXT("%s 위력 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitRate: return TrF(TEXT("%s 연사 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitRange: return TrF(TEXT("%s 사거리 +%d%%"), Sym[F.Param], Pct);
	case EPDStat::SuitSplash: return TrF(TEXT("%s 폭발 범위 +%.1f칸"), Sym[F.Param], F.Value);
	case EPDStat::SuitSlow: return TrF(TEXT("%s 둔화 +%d%%p"), Sym[F.Param], Pct);
	case EPDStat::HandDmg: return TrF(TEXT("%s 위력 +%d%%"), *HandGroupName(F.Param), Pct);
	case EPDStat::HandRate: return TrF(TEXT("%s 연사 +%d%%"), *HandGroupName(F.Param), Pct);
	case EPDStat::HandSplash: return TrF(TEXT("%s 범위 +%.2f칸"), *HandGroupName(F.Param), F.Value);
	case EPDStat::AllDmg: return TrF(TEXT("모든 타워 위력 +%d%%"), Pct);
	case EPDStat::ChainAdd: return TrF(TEXT("연쇄 덤 +%d%%p"), Pct);
	case EPDStat::FuseAdd: return TrF(TEXT("합성 배율 +%.2f"), F.Value);
	case EPDStat::FusedDmg: return TrF(TEXT("합성 타워 위력 +%d%%"), Pct);
	case EPDStat::JokerDmg: return TrF(TEXT("조커 포탑 위력 +%d%%"), Pct);
	case EPDStat::JokerAdd: return TrF(TEXT("조커 배율 +%.2f"), F.Value);
	case EPDStat::AllRate: return TrF(TEXT("모든 타워 공격 속도 +%d%%"), Pct);
	case EPDStat::CritChance: return TrF(TEXT("치명타 확률 +%d%%p"), Pct);
	case EPDStat::CritMult: return TrF(TEXT("치명타 배율 +%.2f"), F.Value);
	case EPDStat::GoldPct: return TrF(TEXT("골드 +%d%%"), Pct);
	case EPDStat::HandMax: return TrF(TEXT("손패 최대 +%d"), (int)F.Value);
	case EPDStat::Draw: return TrF(TEXT("웨이브마다 카드 +%d"), (int)F.Value);
	case EPDStat::Lives: return TrF(TEXT("목숨 +%d (찍을 때)"), (int)F.Value);
	case EPDStat::Keystone: return KeystoneDesc(F.Param);
	case EPDStat::Joker: return JokerUpDesc(F.Param);
	default: return FString();
	}
}
inline FLinearColor TreeNodeColor(int Id)
{
	const FPDTreeNode& N = TreeNode(Id);
	if (N.Kind == EPDNode::Keystone) return Hex(TEXT("ffd26e"));
	if (N.Theme < 4) return SuitColor((EPDSuit)N.Theme);
	if (N.Theme == 4) return Hex(TEXT("7fd6ff"));
	if (N.Theme == 6) return JokerPink;
	return Hex(TEXT("e8f0ff"));
}
// ---- 골드 강화 (U)
inline const TCHAR* UpgradeName(int U)
{
	static const TCHAR* N[] = {TEXT("공격"), TEXT("공격 속도"), TEXT("치명타 확률"), TEXT("치명타 배율"), TEXT("♠ 스페이드 강화"), TEXT("♥ 하트 강화"), TEXT("♦ 다이아 강화"), TEXT("♣ 클로버 강화")};
	return Tr(N[U]);
}
inline FString UpgradeEffect(int U, int Level)
{
	const float V = UpgradeStep((EPDUpgrade)U) * Level;
	switch ((EPDUpgrade)U)
	{
	case EPDUpgrade::Attack: return TrF(TEXT("모든 타워 공격력 +%.0f%%  (레벨당 +1%%)"), V * 100.f);
	case EPDUpgrade::Speed: return TrF(TEXT("모든 타워 공격 속도 +%.0f%%  (레벨당 +1%%)"), V * 100.f);
	case EPDUpgrade::Crit: return TrF(TEXT("치명타 확률 %.1f%%  (레벨당 +0.1%%p, 100%% 넘으면 한 방에 여러 번)"), (BaseCrit + V) * 100.f);
	case EPDUpgrade::CritMult: return TrF(TEXT("치명타 배율 ×%.2f  (레벨당 +0.01)"), BaseCritMult + V);
	default: return TrF(TEXT("그 무늬 타워 공격력 +%.0f%%  (레벨당 +2%%)"), V * 100.f);
	}
}
inline FLinearColor UpgradeColor(int U) { return U >= (int)EPDUpgrade::Spade ? SuitColor((EPDSuit)(U - (int)EPDUpgrade::Spade)) : U >= (int)EPDUpgrade::Crit ? Hex(TEXT("ff9a6a")) : Hex(TEXT("ffd96a")); }
constexpr float UpgRowX = 150.f, UpgRowY = 120.f, UpgRowW = 980.f, UpgRowH = 64.f;
constexpr float UpgBtnW = 76.f, UpgBtnH = 30.f;
const int32 UpgSteps[4] = {1, 10, 100, UpgradeMax};
inline FVector2f UpgButtonPos(int Row, int K) { return FVector2f(UpgRowX + UpgRowW - 4 * (UpgBtnW + 8.f) + K * (UpgBtnW + 8.f), UpgRowY + Row * UpgRowH + (UpgRowH - UpgBtnH) / 2.f); }
// 웨이브 시작 버튼 아래 2×2 버튼 (D-119): 트리 K · 강화 U / 도감 J · 메뉴 Esc
constexpr float TreeBtnX = RBtnX, TreeBtnY = StartBtnY + StartBtnH + 10.f, TreeBtnW = (RBtnW - 8.f) / 2.f, TreeBtnH = 36.f;
constexpr float UpgOpenX = RBtnX + TreeBtnW + 8.f, UpgOpenY = TreeBtnY, UpgOpenW = TreeBtnW, UpgOpenH = TreeBtnH;
constexpr float CodexBtnX = RBtnX, CodexBtnY = TreeBtnY + TreeBtnH + 8.f, CodexBtnW = TreeBtnW, CodexBtnH = TreeBtnH;
constexpr float MenuBtnX = UpgOpenX, MenuBtnY = CodexBtnY, MenuBtnW = TreeBtnW, MenuBtnH = TreeBtnH;
constexpr float CharmY = MenuBtnY + MenuBtnH + 12.f; // 부적 칸
// 조커 도감 (D-119): 카드 5×2 + 오른쪽 크게 보기
constexpr float CdxCardX = 16.f, CdxCardY = 76.f, CdxCardW = 164.f, CdxCardH = 304.f, CdxGapX = 8.f, CdxGapY = 12.f;
constexpr float CdxDetX = 884.f, CdxDetW = 380.f;
constexpr float CdxCloseX = 1180.f, CdxCloseY = 22.f, CdxCloseW = 70.f, CdxCloseH = 30.f; // 닫기 버튼 (트리 창도 같은 자리)
inline FVector2f CodexCardPos(int K) { return FVector2f(CdxCardX + (K % 5) * (CdxCardW + CdxGapX), CdxCardY + (K / 5) * (CdxCardH + CdxGapY)); }
// 메뉴 창 (Esc): 가운데 세로 버튼
enum class EMenuItem : uint8_t { Resume, NewGame, Settings, Quit, Count };
constexpr float MenuW = 300.f, MenuItemH = 44.f, MenuGap = 12.f;
constexpr float MenuX = DesignW / 2.f - MenuW / 2.f, MenuItemY0 = 300.f;
inline FVector2f MenuItemPos(int I) { return FVector2f(MenuX, MenuItemY0 + I * (MenuItemH + MenuGap)); }
// 시작 화면 (D-153): 게임 시작 · 환경 설정 · 게임 종료. 게임 시작 → 로딩 화면 → 인게임
enum class ETitleItem : uint8_t { Continue, Start, Settings, Quit, Count }; // 이어하기는 중단 저장이 있을 때만 (D-162)
constexpr float TitleBtnW = 300.f, TitleBtnH = 50.f, TitleBtnGap = 16.f, TitleBtnY0 = 380.f;
inline FVector2f TitleBtnPos(int I, bool bCont) { const int K = bCont ? I : I - 1; return FVector2f(DesignW / 2.f - TitleBtnW / 2.f, TitleBtnY0 + K * (TitleBtnH + TitleBtnGap)); } // 이어하기가 없으면 위로 당긴다
constexpr float LoadDur = 1.4f; // 로딩 화면 길이 (초)
inline const TCHAR* MenuItemName(int I)
{
	static const TCHAR* N[] = {TEXT("계속하기"), TEXT("새 게임"), TEXT("설정"), TEXT("게임 종료")};
	return Tr(N[I]);
}
// 설정 창 (D-110): 줄마다 이름 + 버튼
enum class ESetRow : uint8_t { Lang, Window, CardTheme, Frame, VSync, WorldFx, Shake, Sound, Volume, Music, Close, Count };
constexpr float SetW = 560.f, SetRowH = 40.f, SetRowGap = 6.f, SetLabelW = 200.f, SetBtnH = 32.f;
constexpr float SetX = DesignW / 2.f - SetW / 2.f, SetY0 = 140.f; // 언어 줄이 늘어 조금 위로 (D-159)
constexpr float SetBoxX = SetX - 30.f, SetBoxY = SetY0 - 76.f, SetBoxW = SetW + 60.f;
constexpr float SetBoxH = (int)ESetRow::Count * (SetRowH + SetRowGap) + 120.f;
inline float SetRowY(int Row) { return SetY0 + Row * (SetRowH + SetRowGap); }
inline bool SetBarRow(int Row) { return Row == (int)ESetRow::Volume || Row == (int)ESetRow::Music; } // −/막대/+ 줄
inline int SetBtnCount(int Row) { return Row == (int)ESetRow::Lang ? (int)EPDLang::Count : Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame || SetBarRow(Row) ? 3 : 1; }
inline FLinearColor LevelDotColor() { return GPDCardTheme == 1 ? FLinearColor::White : Hex(TEXT("ffd96a")); } // 타워 레벨 점 (흑백이면 흰빛)
inline float SetBtnW(int Row, int Idx)
{
	if (Row == (int)ESetRow::Lang) return 66.f; // 언어 다섯 (D-159)
	if (Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame) return 112.f;
	if (SetBarRow(Row)) return Idx == 1 ? 220.f : 40.f;
	if (Row == (int)ESetRow::Close) return 160.f;
	return 120.f;
}
inline FVector2f SetBtnPos(int Row, int Idx) // 왼쪽 위
{
	const float X0 = SetX + SetLabelW, Y = SetRowY(Row) + (SetRowH - SetBtnH) / 2.f;
	if (Row == (int)ESetRow::Lang) return FVector2f(X0 + Idx * (66.f + 6.f), Y);
	if (Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame) return FVector2f(X0 + Idx * (112.f + 8.f), Y);
	if (SetBarRow(Row)) return FVector2f(Idx == 0 ? X0 : Idx == 1 ? X0 + 48.f : X0 + 48.f + 220.f + 8.f, Y);
	if (Row == (int)ESetRow::Close) return FVector2f(DesignW / 2.f - 80.f, Y);
	return FVector2f(X0, Y);
}
// 손패 카드 왼쪽 X. 카드가 많으면 판 너비 안에 들어오게 간격을 좁힌다
inline float HandCardX(int I, int N)
{
	const float Avail = Cols * CS - 96.f - CardW;
	const float Step = N > 1 ? FMath::Min(CardW + 10.f, Avail / (N - 1)) : 0.f;
	return FX + 96.f + I * Step;
}
inline bool InRect(FVector2f P, float X, float Y, float W, float H) { return P.X >= X && P.X <= X + W && P.Y >= Y && P.Y <= Y + H; }
// 가운데 C, 크기 W x H 인 상자 안인가 (판 위 합성·먹이기·조커 버튼)
inline bool InBox(FVector2f P, FVector2f C, float W, float H) { return FMath::Abs(P.X - C.X) <= W / 2.f && FMath::Abs(P.Y - C.Y) <= H / 2.f; }
// 설계 좌표 → 판 칸 키 (판 밖이면 -1)
inline int CellAt(FVector2f P)
{
	const int X = FMath::FloorToInt((P.X - FX) / CS), Y = FMath::FloorToInt((P.Y - FY) / CS);
	return X >= 0 && Y >= 0 && X < Cols && Y < Rows ? CellKey(X, Y) : -1;
}
// 큰 수 줄여 쓰기: 1234 → 1.2k, 34567 → 35k, 3456789 → 3.5M
inline FString ShortNum(float V)
{
	if (V >= 1e9f) return FString::Printf(TEXT("%.1fB"), V / 1e9f);
	if (V >= 1e6f) return FString::Printf(TEXT("%.1fM"), V / 1e6f);
	if (V >= 1e4f) return FString::Printf(TEXT("%.0fk"), V / 1e3f);
	if (V >= 1e3f) return FString::Printf(TEXT("%.1fk"), V / 1e3f);
	return FString::FromInt(FMath::Max(1, FMath::RoundToInt(V)));
}
// 디버그 표시 (D-133, F3): 프레임 수·프레임 시간·최저 프레임·개수. 위젯 헤더를 건드리지 않으려고 전역 (화면 하나뿐)
struct FPDDebugStats
{
	bool bShow = false;
	float Fps = 0.f, Ms = 0.f;          // 부드럽게 한 값
	float WorstMs = 0.f, WorstShown = 0.f; // 1초 동안 가장 느린 프레임
	float Acc = 0.f;
	int32 Lines = 0, Points = 0, Texts = 0, Boxes = 0; // 이번 프레임 그리기 요소 (D-135)
	int32 PartLines[6] = {}; // 판·발사체·이펙트·판 위 UI·패널·창 별 선 수
	int32 StaticPaints = 0;  // 1초 동안 정적 층을 다시 그린 횟수 (D-137)
	bool bNoAtlas = false;   // F6: 구운 그림 끄고 선으로 (비교용, D-143)
	float ModeMs = 0.f;      // 게임 모드 틱 (소리·나이아가라, D-138)
	float PaintMs = 0.f, TickMs = 0.f;                 // 그리기·갱신 CPU 시간 (부드럽게)
	void Add(float RawDt)
	{
		const float Ms0 = RawDt * 1000.f;
		Ms = Ms <= 0.f ? Ms0 : FMath::Lerp(Ms, Ms0, 0.1f);
		Fps = Ms > 0.f ? 1000.f / Ms : 0.f;
		WorstMs = FMath::Max(WorstMs, Ms0);
		Acc += RawDt;
		if (Acc >= 1.f) { WorstShown = WorstMs; WorstMs = 0.f; Acc = 0.f; }
	}
};
inline FPDDebugStats GPDDebug;
// 스프라이트 아틀라스 (D-140): 칸 하나 = 설계 좌표 AtlasCell px, 텍스처엔 AtlasK 배로 굽는다 (화면 확대에도 또렷하게)
// 타워 카드: 카드 53종(2~A × 무늬 4 + 와일드) × 레벨 4 × 족보 듦 2 = 424, 조커 10 → 434칸 (23 × 19줄 = 3344px)
constexpr float AtlasCell = 88.f, AtlasK = 2.f;
constexpr int32 AtlasCols = 23, AtlasTex = 4096; // 23 × 88 × 2 = 4048 ≤ 4096
constexpr int32 AtlasCards = 53, AtlasTowerN = AtlasCards * 4 * 2, AtlasJoker0 = AtlasTowerN;
// 이펙트 모양 (D-141, 흰색으로 구워 색·크기·회전을 입혀 찍는다): 반지름 AtlasFxR 로 굽는다
// 0 원, 1~6 다각형 3~8, 7~12 별 3~8, 13~27 끊어진 호 2~16, 28~42 빛줄기 2~16, 43~47 무늬 0~4, 48~52 별빛 갈래 0·2·4·6·8
constexpr int32 AtlasFx0 = AtlasJoker0 + 10, AtlasFxN = 53;
// 그 밖 (D-143): 족보 빗금 칸, 합성 가능 꺾쇠 칸, 가로 빛선 (조각·입자 꼬리)
constexpr int32 AtlasHatch = AtlasFx0 + AtlasFxN, AtlasBracket = AtlasHatch + 1, AtlasLine = AtlasHatch + 2;
// 선 스프라이트 (D-146): 가로 띠 (가운데 꽉, 위아래 가장자리만 부드럽게). 모든 선은 이걸 늘여 찍는다
constexpr int32 AtlasLineAA = AtlasHatch + 3, AtlasHalo = AtlasHatch + 4, AtlasN = AtlasHatch + 5;
// 띠는 굵기별로 여러 줄: Slate 선 셰이더와 같은 모양 (굵기+2px 너비, 가장자리 smoothstep) 을 굵기마다 구워 둔다
constexpr int32 LineBands = 10;
constexpr float LineBandT[LineBands] = {0.75f, 1.f, 1.5f, 2.f, 3.f, 4.f, 6.f, 9.f, 14.f, 22.f}; // 띠마다 대표 굵기 (화면 px)
constexpr float LineBandH = 8.f;
constexpr float LineAAW = 60.f, LineAAH = LineBandH * LineBands; // 띠 크기 (설계 px)
// 번짐 띠 (D-147): 네온 선의 번짐 두 겹(굵기 6.5배 9% + 3.2배 14%)을 미리 합성한 띠. 굵기별로 한 줄, 알파는 HaloGain 배로 구워 정밀도를 지킨다
constexpr int32 HaloBands = 8;
constexpr float HaloBandT[HaloBands] = {0.5f, 0.75f, 1.f, 1.5f, 2.f, 3.f, 4.f, 6.f}; // 네온 선 굵기 (화면 px)
constexpr float HaloGain = 4.f;
constexpr int32 AtlasVersion = 5; // 5: 카드 칸을 흰색으로 굽고 그릴 때 물들임 (D-154) // 아틀라스 칸이 바뀌면 올린다: 옛 에셋(T_PDAtlas_v1…)은 쓰지 않고 F8 로 새로 만든다
constexpr float HatchStep = 12.f; // 빗금 간격: 칸 48 의 약수라 칸마다 같은 무늬 (판 왼쪽 위 FX + FY 도 12 의 배수)
constexpr float BracketPad = 6.f, LineHalf = 36.f, LineH = 14.f; // 꺾쇠 칸은 칸보다 6px 크게, 빛선은 길이 72 × 높이 14
static_assert(((int)(FX + FY)) % 12 == 0, "hatch phase must line up across cells");
// 판 배경 (격자·길 점선·판 테두리, D-143): 판보다 사방 16px 크게, 2배로 굽는다 (길이 바뀌는 새 판마다 다시)
constexpr float BgPad = 16.f, BgW = Cols * CS + BgPad * 2.f, BgH = Rows * CS + BgPad * 2.f;
constexpr int32 BakeTile = 512; // 조각 굽기 크기 (텍스처 픽셀)
constexpr float AtlasFxR = 36.f, AtlasFxTh = 1.6f;
inline int32 FxPoly(int N) { return 1 + FMath::Clamp(N, 3, 8) - 3; }
inline int32 FxStar(int N) { return 7 + FMath::Clamp(N, 3, 8) - 3; }
inline int32 FxBroken(int N) { return 13 + FMath::Clamp(N, 2, 16) - 2; }
inline int32 FxRays(int N) { return 28 + FMath::Clamp(N, 2, 16) - 2; }
inline int32 FxSuit(int S) { return 43 + FMath::Clamp(S, 0, 4); }
inline int32 FxFlare(int Spokes) { return 48 + FMath::Clamp(Spokes / 2, 0, 4); }
static_assert((AtlasN + AtlasCols - 1) / AtlasCols * AtlasCell * AtlasK <= AtlasTex, "atlas rows overflow the texture");
inline int32 AtlasCardIndex(const FPDCard& C) { return C.IsWild() ? 52 : (FMath::Clamp(C.Rank, 2, 14) - 2) * 4 + FMath::Clamp((int)C.Suit, 0, 3); }
inline int32 TowerSprite(const FPDCard& C, int Level, bool bStrong)
{
	return (AtlasCardIndex(C) * 4 + (FMath::Clamp(Level, 1, 4) - 1)) * 2 + (bStrong ? 1 : 0);
}
inline FVector2f AtlasCellCenter(int32 I) { return FVector2f((I % AtlasCols + 0.5f) * AtlasCell, (I / AtlasCols + 0.5f) * AtlasCell); }
// 배속 차례 (D-124): 1 → 2 → 3 → 5 → 10 → 1
inline int NextSpeed(int S) { return S < 2 ? 2 : S < 3 ? 3 : S < 5 ? 5 : S < 10 ? 10 : 1; }
// 천 단위 쉼표: 12345 → 12,345
inline FString CommaNum(int64 V)
{
	FString S = FString::Printf(TEXT("%lld"), V < 0 ? -V : V);
	for (int32 I = S.Len() - 3; I > 0; I -= 3) S.InsertAt(I, TEXT(','));
	return V < 0 ? TEXT("-") + S : S;
}
// 도감 카드의 작은 족보 알약용 줄임말
inline const TCHAR* HandChip(EPDHand H)
{
	switch (H)
	{
	case EPDHand::RoyalFlush: return Tr(TEXT("로열"));
	case EPDHand::Five: return Tr(TEXT("파이브"));
	case EPDHand::StraightFlush: return Tr(TEXT("스티플"));
	case EPDHand::FullHouse: return Tr(TEXT("풀하"));
	case EPDHand::Straight: return Tr(TEXT("스트"));
	case EPDHand::TwoPair: return Tr(TEXT("투페어"));
	case EPDHand::Pair: return Tr(TEXT("페어"));
	default: return Tr(HandName(H));
	}
}
} // namespace PDDraw

using namespace PDDraw;

// 스프라이트 아틀라스 (D-140): 렌더 타깃 하나(루트에 붙여 유지) + 칸마다 UV 를 정한 붓
class FWidgetRenderer;
struct FPDAtlas
{
	UObject* RT = nullptr;
	UObject* Tex = nullptr;   // 붓이 쓰는 텍스처: 에셋, 또는 구운 바이트를 담은 sRGB 텍스처 (D-145)
	UObject* BgTex = nullptr; // 판 배경도 같은 방식
	TArray<FColor> LastPixels; // 마지막으로 구운 아틀라스 바이트 (에셋 저장용)
	TArray<FSlateBrush> HatchBrushes;   // 빗금: 겹 3 × 네 변 들임 16가지 (D-143)
	FSlateBrush BracketBrush, LineBrush, LineAABrush;
	// 판 배경 (D-143)
	UObject* BgRT = nullptr;
	FSlateBrush BgBrush;
	uint64 BgSig = 0;
	int32 BgFrames = 0;
	TSharedPtr<FWidgetRenderer> BgRenderer;
	TSharedPtr<SWidget> BgPainter;
	bool BgReady(uint64 Sig) const { return BgRT && BgSig == Sig && BgFrames > 2; }
	TSharedPtr<FWidgetRenderer> Renderer; // 구울 때 쓴 것들은 같이 들고 있는다 (뒤에서 도는 선 만들기 작업이 끝나기 전에 사라지면 터짐)
	TSharedPtr<SWidget> Painter;
	TArray<FSlateBrush> Brushes;
	int32 Frames = 0; // 구운 뒤 몇 프레임 (렌더 명령이 끝난 다음부터 쓴다)
	float BakedScale = 1.f; // 구울 때의 화면 배율: 선 굵기를 화면 픽셀로 맞춘다 (바뀌면 다시 굽는다)
	bool bRebaked = false;  // 시작 직후 한 번 더 구웠나 (패키지에서 너무 일찍 구우면 빈 그림이 됨)
	bool bAsset = false;    // 미리 만든 에셋(Content/Sprites/T_PDAtlas)을 쓰는 중 (D-145): 굽지 않는다
	// 조각 굽기 (D-143): 패키지에선 화면 밖 그리기가 창 크기로 잘려 큰 텍스처를 한 번에 못 굽는다.
	// 512 조각 텍스처에 그린 뒤 GPU 로 제자리에 복사한다. 그리는 동안 TileOrigin (텍스처 픽셀) 만큼 밀어 그린다
	UObject* Scratch = nullptr;
	TSharedPtr<FWidgetRenderer> ScratchRenderer;
	FVector2f TileOrigin = FVector2f(0.f, 0.f);
	bool Ready() const { return Frames > 2; }
};

// ---------------------------------------------------------------- 그리기 도우미
struct SPDBoardWidget::FCanvas
{
	FSlateWindowElementList& Out;
	FGeometry Geo;
	int32* Layer;
	const FSlateBrush* White;
	float HaloK = 1.f; // 번짐 배율 (이펙트가 많으면 0 = 심만, D-138)
	const FSlateBrush* LineAA = nullptr; // 있으면 선을 선 스프라이트로 찍는다 (D-146). 굽는 붓에선 없음 (진짜 선으로 굽는다)
	float PxToDesign = 1.f;              // 화면 픽셀 → 설계 좌표 (선 굵기는 화면 픽셀 기준이라)
	float ThickK = 1.f; // 선 굵기 배율: Slate 선 굵기는 화면 픽셀이라 배율을 따라가지 않는다. 2배로 굽는 그림은 2 (D-143)

	// 띠 묶음 (D-147): 같은 층의 띠는 꼭짓점 묶음 하나로 모아 Slate 요소 하나로 넘긴다
	// (띠마다 상자 요소를 만들면 큰 판에서 한 프레임 9천 개가 넘어 그리기·묶기 시간이 크게 늘었다)
	TArray<FSlateVertex> QV;
	TArray<SlateIndex> QI;
	int32 QLayer = -1;
	const FSlateBrush* QBrush = nullptr;
	~FCanvas() { FlushQuads(); }
	void FlushQuads()
	{
		if (QV.Num() && QBrush)
		{
			const FSlateResourceHandle H = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*QBrush);
			FSlateDrawElement::MakeCustomVerts(Out, QLayer, H, QV, QI, nullptr, 0, 0, ESlateDrawEffect::PreMultipliedAlpha);
		}
		QV.Reset();
		QI.Reset();
	}
	int32 NextLayer() { return ++(*Layer); }
	// 층 나누기 (D-120): 선은 지금 층에 모아 그린다 (같은 층의 선은 한 번에 묶여 그려져 그리기 호출이 크게 준다).
	// 상자·글자는 새 층을 받고, 그 뒤의 선이 그 위에 오도록 층을 하나 더 올려 둔다 (그리는 순서는 그대로)
	void Lines(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick)
	{
		if (LineAA) // 선 스프라이트 (D-146): 마디마다 띠 하나를 늘이고 돌려 찍는다. 같은 층이라 한 번에 그려진다
		{
			// Slate 선은 굵기 + 2px (양쪽 1px 거르기) 너비로 그리고 셰이더가 가장자리를 깎는다. 같은 너비로 찍고, 굵기에 가장 가까운 띠를 쓴다
			// (얇은 사각형을 그대로 찍으면 래스터에서 덮임이 반쯤 날아가 가늘고 흐려진다)
			const float Tpx = FMath::Max(0.75f, Thick * ThickK);
			int32 Bi = 0;
			while (Bi + 1 < LineBands && Tpx > FMath::Sqrt(LineBandT[Bi] * LineBandT[Bi + 1])) ++Bi;
			Stamp(P, PM(Col), LineAA + Bi, (Tpx + 2.f) * PxToDesign); // 화면 px → 설계 좌표 (DPI 포함 화면 배율로 나눔)
			++GPDDebug.Lines; GPDDebug.Points += P.Num();
			return;
		}
		FSlateDrawElement::MakeLines(Out, *Layer, Geo.ToPaintGeometry(), P, ESlateDrawEffect::None, Col, true, Thick * ThickK);
		++GPDDebug.Lines; GPDDebug.Points += P.Num();
	}
	// 띠 찍기 (D-146/147): 마디마다 띠를 늘이고 돌려 한 장. 한 줄로 곧게 이어지는 마디는 한 장으로 합친다 (격자·테두리의 칸 마디)
	FSlateRenderTransform RT;
	void Stamp(const TArray<FVector2f>& P, const FLinearColor& T, const FSlateBrush* Band, float Th)
	{
		const int32 N = P.Num();
		const FColor Col32 = T.ToFColor(true); // Slate 꼭짓점 색과 같게 sRGB
		int32 I = 0;
		while (I + 1 < N)
		{
			int32 J = I + 1;
			FVector2f D = P[J] - P[I];
			float L = D.Size();
			if (L < 0.01f) { I = J; continue; }
			while (J + 1 < N)
			{
				const FVector2f E = P[J + 1] - P[J];
				const float LE = E.Size();
				if (LE < 0.01f) { ++J; continue; }
				if (FVector2f::DotProduct(D, E) <= 0.f || FMath::Abs(FVector2f::CrossProduct(D, E)) > 0.004f * L * LE) break;
				++J;
			}
			D = P[J] - P[I];
			L = D.Size();
			const FVector2f Dir = D / L, Nrm = FVector2f(-Dir.Y, Dir.X) * (Th * 0.5f);
			if (QV.Num() && (QLayer != *Layer || QV.Num() > 60000)) FlushQuads();
			if (QV.Num() == 0) { QLayer = *Layer; QBrush = Band; RT = Geo.GetAccumulatedRenderTransform(); }
			const FBox2f UV = Band->GetUVRegion();
			const SlateIndex B = (SlateIndex)QV.Num();
			QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, P[I] - Nrm, FVector2f(UV.Min.X, UV.Min.Y), Col32));
			QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, P[J] - Nrm, FVector2f(UV.Max.X, UV.Min.Y), Col32));
			QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, P[J] + Nrm, FVector2f(UV.Max.X, UV.Max.Y), Col32));
			QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, P[I] + Nrm, FVector2f(UV.Min.X, UV.Max.Y), Col32));
			QI.Add(B); QI.Add(B + 1); QI.Add(B + 2); QI.Add(B); QI.Add(B + 2); QI.Add(B + 3);
			++GPDDebug.Boxes; // 띠 수
			I = J;
		}
	}
	// 네온 선 (docs/15-art-style.md): 아주 넓고 옅은 번짐 세 겹 + 가늘고 또렷한 심
	// 웹 캔버스의 shadowBlur 처럼 부드럽게 퍼지는 빛을 흉내 낸다
	void Glow(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Strength = 1.f)
	{
		if (Strength > 0.f && HaloK > 0.f && LineAA) // 번짐 두 겹을 합성한 띠 한 장 (D-147). 띠는 선 띠 다음에 이어 둔다
		{
			const float Tpx = FMath::Max(0.25f, Thick * ThickK);
			int32 Bi = 0;
			while (Bi + 1 < HaloBands && Tpx > FMath::Sqrt(HaloBandT[Bi] * HaloBandT[Bi + 1])) ++Bi;
			Stamp(P, PM(WithAlpha(Col, FMath::Min(1.f, Strength / HaloGain))), LineAA + LineBands + Bi, (Tpx * 6.5f + 2.f) * PxToDesign);
			++GPDDebug.Lines; GPDDebug.Points += P.Num();
		}
		else if (Strength > 0.f && HaloK > 0.f) // 번짐 두 겹 (D-135: 가장 넓은 10배 겹은 거의 안 보이면서 그리기 부담이 커서 빼고 6배 겹을 조금 진하게)
		{
			Lines(P, WithAlpha(Col, 0.09f * Strength), Thick * 6.5f);
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
		NextLayer();
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
		FString S = TrS(InS); // 언어팩 (D-159): 그릴 때 번역
		FSlateFontInfo F = Font(Px, bBold);
		FVector2f Sz = Measure(S, F);
		FitText(S, F, Sz, Px, bBold, MaxW);
		const float Left = Align == 1 ? X - Sz.X / 2.f : Align == 2 ? X - Sz.X : X;
		const float Top = Y - Sz.Y * 0.8f;
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(Left, Top))), S, F, ESlateDrawEffect::None, Col);
		NextLayer();
	}
	void TextCentered(const FString& InS, float CX, float CY, const FLinearColor& Col, float Px, bool bBold, float MaxW = 0.f)
	{
		FString S = TrS(InS);
		FSlateFontInfo F = Font(Px, bBold);
		FVector2f Sz = Measure(S, F);
		FitText(S, F, Sz, Px, bBold, MaxW);
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(CX - Sz.X / 2.f, CY - Sz.Y / 2.f))), S, F,
			ESlateDrawEffect::None, Col);
		NextLayer();
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
	// 카드(=타워): 둥근 사각 틀 안에 숫자와 작은 무늬 기호 (docs/15-art-style.md: 판 위 물체는 둥근 사각 틀 안에 작은 기호)
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
	void Badge(const FString& InS, float CX, float CY, const FLinearColor& Col, float Alpha)
	{
		const FString S = TrS(InS);
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
	// 구운 그림은 빈 바탕에 반투명으로 그려져 색에 알파가 이미 곱해져 있다 (premultiplied). 그대로 알파를 또 곱하면 옅은 선(격자·번짐)이 사라지므로
	// 미리 곱한 알파로 섞고, 덧칠 색도 알파를 곱해 넘긴다 (D-143)
	// 미리 곱한 알파 색 (D-146): Slate 는 꼭짓점 색을 sRGB 로 바꿔 감마 공간에서 섞으므로 sRGB 값에 알파를 곱한다
	// (선형 값에 곱하면 sRGB 로 바뀌며 옅은 색이 몇 배 밝아져, 옅은 빛번짐이 진짜 선보다 하얗게 뜬다)
	static FLinearColor PM(const FLinearColor& C)
	{
		if (C.A >= 0.999f) return C;
		const FColor S = FLinearColor(C.R, C.G, C.B, 1.f).ToFColor(true);
		FLinearColor O = FLinearColor::FromSRGBColor(FColor((uint8)(S.R * C.A + 0.5f), (uint8)(S.G * C.A + 0.5f), (uint8)(S.B * C.A + 0.5f), 255));
		O.A = C.A;
		return O;
	}
	// 아틀라스 칸 찍기 (D-140): 가운데 (CX, CY), 설계 크기 AtlasCell × S. 같은 층 L 에 모으면 한 번에 그려진다
	void Sprite(int32 L, const FSlateBrush* B, float CX, float CY, float S, const FLinearColor& Tint)
	{
		const float H = AtlasCell * S * 0.5f;
		FSlateDrawElement::MakeBox(Out, L, Geo.ToPaintGeometry(FVector2f(H * 2.f, H * 2.f), FSlateLayoutTransform(FVector2f(CX - H, CY - H))), B, ESlateDrawEffect::PreMultipliedAlpha, PM(Tint));
	}
	// 붓 찍기 (D-143): 왼쪽 위 (X, Y), 크기 W × H
	void Image(int32 L, const FSlateBrush* B, float X, float Y, float W, float H, const FLinearColor& Tint)
	{
		FSlateDrawElement::MakeBox(Out, L, Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), B, ESlateDrawEffect::PreMultipliedAlpha, PM(Tint));
	}
	// 가운데 (CX, CY), 크기 W × H, 회전 Angle 라디안
	void ImageRot(int32 L, const FSlateBrush* B, float CX, float CY, float W, float H, float Angle, const FLinearColor& Tint)
	{
		FSlateDrawElement::MakeRotatedBox(Out, L, Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(CX - W / 2.f, CY - H / 2.f))), B, ESlateDrawEffect::PreMultipliedAlpha, Angle,
			TOptional<FVector2f>(), FSlateDrawElement::RelativeToElement, PM(Tint));
	}
	// 회전해 찍기 (이펙트, D-141): Angle 라디안
	void SpriteRot(int32 L, const FSlateBrush* B, float CX, float CY, float S, float Angle, const FLinearColor& Tint)
	{
		const float H = AtlasCell * S * 0.5f;
		FSlateDrawElement::MakeRotatedBox(Out, L, Geo.ToPaintGeometry(FVector2f(H * 2.f, H * 2.f), FSlateLayoutTransform(FVector2f(CX - H, CY - H))), B, ESlateDrawEffect::PreMultipliedAlpha, Angle,
			TOptional<FVector2f>(), FSlateDrawElement::RelativeToElement, PM(Tint));
	}
	// 트리 노드 아이콘 (D-119, PDIcons.h): 선 묶음 + 광대 모자·네 무늬는 직접 그림
	void Icon(const PDIcon::FIcon& I, const FLinearColor& Col, float Th, float Gk)
	{
		for (const PDIcon::FLine& L : I.Lines) Glow(L, Col, Th, Gk);
		if (I.Extra == PDIcon::EExtra::Hat) JesterHat(I.HatC.X, I.HatC.Y, I.HatR, Col, Th, Gk);
		else if (I.Extra == PDIcon::EExtra::Suits) IconSuits(I.HatC.X, I.HatC.Y, I.HatR, Col, Th, Gk);
	}
	void IconSuits(float CX, float CY, float R, const FLinearColor& Col, float Th, float Gk) // 잡식가: 네 무늬
	{
		Shape(EPDSuit::Spade, CX - 0.48f * R, CY - 0.48f * R, 0.4f * R, Col, Th, Gk);
		Shape(EPDSuit::Heart, CX + 0.48f * R, CY - 0.48f * R, 0.4f * R, Col, Th, Gk);
		Shape(EPDSuit::Diamond, CX - 0.48f * R, CY + 0.48f * R, 0.4f * R, Col, Th, Gk);
		Shape(EPDSuit::Club, CX + 0.48f * R, CY + 0.48f * R, 0.4f * R, Col, Th, Gk);
	}
	// 버튼 위 판 위력 비교 (D-107): "판 위력 1.2k → 1.6k (+27%)" 오르면 초록, 내리면 빨강. 누를 수 없으면 안 그림
	void PowerBadge(const FPDGame::FPowerPreview& P, float X, float Y)
	{
		if (P.After < 0.f || P.Before <= 0.f) return;
		const float Pct = (P.After / P.Before - 1.f) * 100.f;
		const FLinearColor Col = Pct >= 0.5f ? Hex(TEXT("9cffb0")) : Pct <= -0.5f ? Hex(TEXT("ff6a6a")) : Hex(TEXT("cfd8e6"));
		Badge(TrF(TEXT("판 위력 %s → %s (%s%.0f%%)"), *ShortNum(P.Before), *ShortNum(P.After), Pct >= 0.f ? TEXT("+") : TEXT(""), Pct), X, Y, Col, 1.f);
	}
};
