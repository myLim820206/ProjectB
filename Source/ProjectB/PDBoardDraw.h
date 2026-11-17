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
#include "PDUpperArt.inl" // 상위 조커 30종 기호 점 (tools/neon/gen_upper_art.py, D-231)
}
namespace PDDraw
{
constexpr float DesignW = 1280.f;
constexpr float DesignH = 720.f;
// 화면 세 칸: 왼쪽 족보 패널 | 가운데 판과 손패 | 오른쪽 상태 패널
constexpr float LX = 16.f, LW = 232.f; // 왼쪽 족보 패널
constexpr float CS = 48.f;          // 칸 크기
constexpr float FX = LX + LW + 14.f; // 판 왼쪽 위
constexpr float HudY = 12.f, HudH = 50.f;  // 위쪽 상태 띠 (D-119): 웨이브·남은 적·목숨·골드·판 위력·속도·설정
constexpr float FY = 98.f; // 인게임 시안 B (D-274): 위에 걸린 간판 자리만큼 판을 내림 (예전 74, FX + FY 는 12 의 배수)
constexpr float HandRowH = 38.f, HandRowY0 = FY + 36.f; // 왼쪽 족보 목록 한 줄 (D-148: 클릭하면 필터)
constexpr float BoardCardK = 0.74f; // 판 위 카드·조커 그림 크기 (D-167): 칸 테두리·족보 테두리와 겹치지 않게 조금 작게
constexpr int32 JokerFilterId = (int32)EPDHand::Count;   // 족보 목록 맨 아래 '조커' 줄 (D-152): 조커 합성 버튼을 모두 펼친다
constexpr int32 FeedFilterId = JokerFilterId + 1;         // 족보 목록 맨 아래 '먹이기' 줄 (D-213): 판 위 조커 목록 → 고른 조커에 족보 먹이기
constexpr int32 HandRowsN = (int32)EPDHand::Count + 2;
// 족보 메뉴판 (D-263, 레터보드 시안): 묶음 순서로 줄을 놓는다 (5장 족보 / 2~4장 족보 / 조커 · 먹이), 묶음마다 머리글 한 줄.
// 조커 · 먹이기 목록(D-210/213)은 예전 자리(HandRowY0, HandRowH)를 그대로 쓴다
// 레터보드 족보 (D-263 → D-276): 패널은 화면 아래까지 (손패가 왼쪽까지 안 오므로), 줄 36
constexpr int32 HandMenuOrder[HandRowsN] = {0, 1, 2, 4, 5, 6, 3, 7, 8, 9, JokerFilterId, FeedFilterId};
constexpr float HandPanelY = 66.f; // 왼쪽 패널 윗선 (판보다 위, 로고 배지 아래)
constexpr float HandMenuY0 = HandPanelY + 72.f, HandMenuRowH = 36.f, HandMenuGroupH = 26.f; // 묶음 사이 여백 18 → 26 + 구분선 (D-328 ④), 머리 설명 글을 빼서 위를 12 당김 (D-328 ⑤)
inline int HandMenuGroup(int Slot) { return Slot < 6 ? 0 : Slot < 10 ? 1 : 2; }
inline float HandMenuTop(int Id) { int Slot = 0; for (int K = 0; K < HandRowsN; ++K) if (HandMenuOrder[K] == Id) Slot = K; return HandMenuY0 + (HandMenuGroup(Slot) + 1) * HandMenuGroupH + Slot * HandMenuRowH; }
inline int HandMenuAt(float X, float Y) { if (X < LX + 8.f || X > LX + LW - 8.f) return -1; for (int K = 0; K < HandRowsN; ++K) { const float T = HandMenuTop(HandMenuOrder[K]); if (Y >= T && Y < T + HandMenuRowH) return HandMenuOrder[K]; } return -1; }
constexpr float HandMenuEndY = HandMenuY0 + 3.f * HandMenuGroupH + HandRowsN * HandMenuRowH;
constexpr float HandY = FY + Rows * CS + 14.f;
constexpr float SwapBtnX = FX, SwapBtnY = HandY + 54.f, SwapBtnW = 86.f, SwapBtnH = 28.f; // 자리바꿈 버튼 (D-149, 덱 글자 아래)
constexpr float CraftBtnX = FX, CraftBtnY = SwapBtnY + SwapBtnH + 6.f, CraftBtnW = 86.f, CraftBtnH = 28.f; // 조커 만들기 버튼 (D-195, 자리바꿈 아래)
constexpr float CardW = 70.f, CardH = 98.f;
constexpr float RX = FX + Cols * CS + 14.f; // 오른쪽 패널
constexpr float PW = DesignW - RX - 16.f;   // 오른쪽 패널 너비
constexpr float RBtnX = RX, RBtnW = PW;     // 오른쪽 패널 버튼 줄
constexpr float PowCardH = 226.f;           // 판 위력 카드 (오른쪽 맨 위). 딜러 7위까지 (D-164: 부적 칸을 줄이고 늘림)
constexpr int32 PowTopN = 7;
constexpr float StartBtnY = FY + PowCardH + 10.f, StartBtnH = 52.f; // 마키 버튼 (D-274): 위 가장자리에 전구
constexpr float SpeedBtnX = 1184.f, SetOpenX = 1222.f, HudBtnY = 38.f, HudBtnS = 30.f; // 위쪽 띠 오른쪽 끝 버튼
constexpr float PauseBtnX = SpeedBtnX - HudBtnS - 8.f; // 일시정지 (D-169, P)
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
	case EPDSuit::Club: return Hex(TEXT("8fe9ff")); // ♣ 서리 = 얼음 하늘색 (D-181, 예전 초록은 독처럼 보였다)
	case EPDSuit::Wild: return Hex(TEXT("ffffff"));
	default: return Hex(TEXT("e8eeff")); // ♠ 저격 = 흰빛 (D-181)
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
	case EPDEnemy::Bluffer: case EPDEnemy::Decoy: return Hex(TEXT("b8a6ff"));
	case EPDEnemy::BigBlind: return Hex(TEXT("ffd96a"));
	case EPDEnemy::Dealer: return Hex(TEXT("6affc8"));
	case EPDEnemy::Counter: return Hex(TEXT("7fb8ff"));
	case EPDEnemy::HouseRule: return Hex(TEXT("ff8a5a"));
	case EPDEnemy::Twins: return Hex(TEXT("ff9ad5"));
	case EPDEnemy::Bouncer: return Hex(TEXT("ff6a6a"));
	case EPDEnemy::HighRoller: return Hex(TEXT("ffd26e"));
	case EPDEnemy::House: return Hex(TEXT("ff4fd8"));
	default: return Hex(TEXT("e8f0ff"));
	}
}
inline const TCHAR* BossName(EPDEnemy T);
// 보스 웨이브 이름 (D-334): '방패 거인', 짝 · 셋은 '돌진 기사 + 블러퍼', 호위는 '+ 호위', 변이는 앞에
inline FString BossWaveLabel(const FPDGame& G, int W)
{
	EPDBossForm F = EPDBossForm::Solo;
	FString S;
	for (const FPDBossSlot& Sl : G.BossWave(W, &F))
	{
		if (!S.IsEmpty()) S += TEXT(" + ");
		if (Sl.Mut & MutFast) S += Tr(TEXT("빠른 "));
		if (Sl.Mut & MutHard) S += Tr(TEXT("단단한 "));
		if (Sl.Mut & MutRegen) S += Tr(TEXT("재생하는 "));
		if (Sl.Mut & MutShield) S += Tr(TEXT("보호막 "));
		if (Sl.Mut & MutLead) S += Tr(TEXT("이끄는 "));
		if (Sl.Mut & MutHex) S += Tr(TEXT("저주하는 "));
		S += BossName(Sl.T);
	}
	if (F == EPDBossForm::Escort || (F == EPDBossForm::Final && G.Mode == EPDMode::W30)) S += Tr(TEXT(" + 호위"));
	return S;
}
// 받침 고르기: 끝 글자에 받침이 있으면 true (한글이 아니면 false)
inline bool HasBatchim(const FString& S)
{
	if (S.IsEmpty()) return false;
	const TCHAR Ch = S[S.Len() - 1];
	return Ch >= 0xAC00 && Ch <= 0xD7A3 && (Ch - 0xAC00) % 28 != 0;
}
// 채널 TV CH1 다음 화 제목 (사용자 2026-11-16, D-344): 보스 웨이브는 짜임대로("{보스}의 습격" · "{A}와 {B}" · "{보스}와 호위대" · "최종화 · {보스}"),
// 보통 웨이브는 가장 많은 적 종류로 ("병정들의 행진" · "탱크 부대 등장" …). Mix = 종류별 마리 수 (보스 먼저)
template <typename TMix>
FString EpisodeTitle(const FPDGame& G, int W, const TArray<TMix>& Mix)
{
	EPDBossForm F = EPDBossForm::Solo;
	const std::vector<FPDBossSlot> B = G.BossWave(W, &F);
	const bool bBossWave = !Mix.IsEmpty() && EnemyDef(Mix[0].T).bBoss;
	if (bBossWave && !B.empty())
	{
		const FString A = BossName(B[0].T);
		const bool bA = HasBatchim(A);
		if (F == EPDBossForm::Final || IsFinalBoss(B[0].T)) return TrF(TEXT("최종화 · %s"), *A);
		if (F == EPDBossForm::Escort) return bA ? TrF(TEXT("%s과 호위대"), *A) : TrF(TEXT("%s와 호위대"), *A);
		if (F == EPDBossForm::Trio) return Tr(TEXT("세 보스의 밤"));
		if (F == EPDBossForm::Duo && B.size() >= 2) { const FString Bn = BossName(B[1].T); return bA ? TrF(TEXT("%s과 %s"), *A, *Bn) : TrF(TEXT("%s와 %s"), *A, *Bn); }
		return TrF(TEXT("%s의 습격"), *A);
	}
	EPDEnemy Main = EPDEnemy::Normal;
	int Most = -1, Kinds = 0;
	for (const TMix& M : Mix) if (!EnemyDef(M.T).bBoss) { ++Kinds; if (M.N > Most) { Most = M.N; Main = M.T; } }
	if (Kinds >= 4) return Tr(TEXT("뒤섞인 밤"));
	switch (Main)
	{
	case EPDEnemy::Fast: return Tr(TEXT("질주하는 밤"));
	case EPDEnemy::Tank: return Tr(TEXT("탱크 부대 등장"));
	case EPDEnemy::Armored: return Tr(TEXT("철갑의 벽"));
	case EPDEnemy::Splitter: return Tr(TEXT("갈라지는 그림자"));
	case EPDEnemy::Regen: return Tr(TEXT("죽지 않는 자들"));
	case EPDEnemy::Mini: return Tr(TEXT("졸개들의 습격"));
	default: return Tr(TEXT("병정들의 행진"));
	}
}
// 채널 TV 다이얼 (D-316): TV 오른쪽 아래 손잡이 중 위의 것
constexpr float TvDialR = 11.f;
inline FVector2f TvDialPos() { return FVector2f(RX + PW - 22.f, FY + PowCardH - 50.f); }
// 적 이름 (보스는 BossName)
inline const TCHAR* EnemyName(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::Normal: return Tr(TEXT("병정"));
	case EPDEnemy::Fast: return Tr(TEXT("빠른 병정"));
	case EPDEnemy::Tank: return Tr(TEXT("탱크"));
	case EPDEnemy::Armored: return Tr(TEXT("갑옷 병정"));
	case EPDEnemy::Splitter: return Tr(TEXT("분열 병정"));
	case EPDEnemy::Regen: return Tr(TEXT("재생 병정"));
	case EPDEnemy::Mini: return Tr(TEXT("졸개"));
	case EPDEnemy::Decoy: return Tr(TEXT("블러퍼"));
	default: return BossName(T);
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
	case EPDEnemy::Bluffer: case EPDEnemy::Decoy: return Tr(TEXT("블러퍼"));
	case EPDEnemy::BigBlind: return Tr(TEXT("빅 블라인드"));
	case EPDEnemy::Dealer: return Tr(TEXT("딜러"));
	case EPDEnemy::Counter: return Tr(TEXT("카드 카운터"));
	case EPDEnemy::HouseRule: return Tr(TEXT("하우스 룰"));
	case EPDEnemy::Twins: return Tr(TEXT("쌍둥이"));
	case EPDEnemy::Bouncer: return Tr(TEXT("바운서"));
	case EPDEnemy::HighRoller: return Tr(TEXT("하이롤러"));
	case EPDEnemy::House: return Tr(TEXT("더 하우스"));
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
	case EPDEnemy::Bluffer: case EPDEnemy::Decoy: return Tr(TEXT("똑같이 생긴 분신 둘을 데리고 온다"));
	case EPDEnemy::BigBlind: return Tr(TEXT("판돈을 건다: 20초 안에 잡으면 ×2, 넘기면 올인"));
	case EPDEnemy::Dealer: return Tr(TEXT("8초마다 타워 두 장의 무늬를 잠깐 바꾼다"));
	case EPDEnemy::Counter: return Tr(TEXT("많이 맞은 무늬의 피해를 덜 받는다"));
	case EPDEnemy::HouseRule: return Tr(TEXT("주변 타워의 연쇄 배율을 끈다"));
	case EPDEnemy::Twins: return Tr(TEXT("하나가 쓰러지면 남은 쪽이 회복하고 분노"));
	case EPDEnemy::Bouncer: return Tr(TEXT("최종 보스: 방패 → 돌진 → 주변 타워 기절"));
	case EPDEnemy::HighRoller: return Tr(TEXT("최종 보스: 칩 더미 → 무늬 바꾸기 → 올인"));
	case EPDEnemy::House: return Tr(TEXT("최종 보스: 남의 능력 → 판 절반 연쇄 끔 → 졸개와 흡수"));
	default: return Tr(TEXT(""));
	}
}
// 도전 과제 이름 (B8, EPDAch 순서). 앞 10개는 목표 해금과 같다
inline const TCHAR* AchName(int I)
{
	static const TCHAR* N[] = {TEXT("첫 손님 배웅"), TEXT("두 번째 잔"), TEXT("마감까지"), TEXT("첫 조커"), TEXT("왕관의 무게"), TEXT("조커 수집가"), TEXT("늦은 밤의 고수"), TEXT("천 명의 병정"),
		TEXT("아홉 손님 상대"), TEXT("단골손님"), TEXT("끝나지 않는 밤"), TEXT("새벽이 올 때까지"), TEXT("로열 플러시"), TEXT("파이브 카드"), TEXT("족보 완성"), TEXT("최고 합성"),
		TEXT("상위 조커 열 장"), TEXT("상위 조커 도감 완성"), TEXT("칵테일 서른 잔"), TEXT("모든 칵테일"), TEXT("영구 강화 끝"), TEXT("숨은 채널"), TEXT("바의 터줏대감"), TEXT("십만 명의 병정"),
		TEXT("모든 손님 상대"), TEXT("마지막 손님 셋"), TEXT("변이 셋")};
	return I >= 0 && I < (int)UE_ARRAY_COUNT(N) ? Tr(N[I]) : TEXT("");
}
// 목표로 여는 칵테일의 목표 글 (D-321 ④, EPDGoal 순서)
inline const TCHAR* GoalText(int I)
{
	static const TCHAR* N[] = {TEXT("30 모드 클리어"), TEXT("60 모드 클리어"), TEXT("100 모드 클리어"), TEXT("조커 처음 만들기"), TEXT("상위 조커 처음 만들기"),
		TEXT("기본 조커 10종 모두 만들기"), TEXT("한 판에 레벨 30"), TEXT("한 판에 1,000마리 처치"), TEXT("보스 9종 처치"), TEXT("10판 끝내기")};
	return I >= 0 && I < (int)UE_ARRAY_COUNT(N) ? Tr(N[I]) : TEXT("");
}
inline const TCHAR* CharmName(int J)
{
	static const TCHAR* N[] = { // 칵테일 이름 (D-281): 효과가 떠오르게
		TEXT("핑크 하트 사워"), TEXT("블랙 스페이드 마티니"), TEXT("골드 다이아 피즈"), TEXT("민트 클로버 줄렙"), TEXT("체인 펀치"), TEXT("트윈 하이볼"),
		TEXT("스트레이트 업"), TEXT("블루 웨이브"), TEXT("코트 로열"), TEXT("미니 샷"), TEXT("럭키 세븐"), TEXT("더블 푸어"),
		TEXT("하우스 리필"), TEXT("원스 모어"), TEXT("아이 오프너"), TEXT("프로스트 다이키리"), TEXT("플레이밍 볼"), TEXT("스크루드라이버"),
		TEXT("에이스 오브 진"), TEXT("이븐 스플릿"), TEXT("오드볼"), TEXT("풀하우스 펀치"), TEXT("네잎 클로버 클럽"), TEXT("로열 플러시"),
		TEXT("글라스 캐논"), TEXT("롱 드링크"), TEXT("다이너마이트"), TEXT("에스프레소 마티니"), TEXT("조커스 와일드"), TEXT("그루먼드 플립"),
		TEXT("블루 블레이저"), TEXT("바운티 헌터"), TEXT("퀸즈 스위즐"), TEXT("킹스 아이"), TEXT("피버 펀치"), TEXT("파이브 스타"),
		TEXT("트리플 섹"), TEXT("온 더 록"), TEXT("와일드 체리"), TEXT("웰컴 드링크"), TEXT("보스 킬러"), TEXT("라스트 콜"),
		TEXT("잭 로즈"), TEXT("듀스"), TEXT("레인보우 슈터"), TEXT("골든 플러시"), TEXT("더블 더블"), TEXT("트리플 샷"),
		TEXT("쿼드 슬링"), TEXT("듀엣"), TEXT("쿼텟"), TEXT("퀵 셰이크"), TEXT("빅 조커"), TEXT("스나이퍼"),
		TEXT("크러시드 아이스"), TEXT("래피드 샷"), TEXT("올드 패션드"), TEXT("해피 아워"), TEXT("풀코스"), TEXT("바텐더의 손")}; // D-239
	return Tr(N[J]);
}
inline const TCHAR* CharmDesc(int J)
{
	static const TCHAR* N[] = {TEXT("♥ 타워 공격력 +40%"), TEXT("♠ 타워 공격력 +40%"), TEXT("♦ 타워 공격력 +40%"), TEXT("♣ 타워 공격력 +40%"), TEXT("연쇄 보너스 +25% → +45%"),
		TEXT("원 페어·투 페어 ×1.5 → ×2"), TEXT("스트레이트 연사 한 장마다 +0.25 더"), TEXT("플러시 범위 +0.75칸"), TEXT("J·Q·K 타워 공격력 +60%"), TEXT("2~6 타워 연사 +60%"),
		TEXT("숫자 7 타워 공격력 ×2.5"), TEXT("손패 최대 +2"), TEXT("웨이브마다 카드 +1"), TEXT("카드를 버리면 1장 뽑기"), TEXT("모든 타워 사거리 +15%"),
		TEXT("둔화 더 강하게, ♣ 주변도 둔화"), TEXT("처치 시 주변 적에게 최대 체력 15% 피해"), TEXT("적의 방패를 무시"),
		TEXT("A 타워 공격력 ×3"), TEXT("짝수(2~10) 타워 공격력 +40%"), TEXT("홀수(3~9) 타워 연사 +30%"), TEXT("풀하우스 공격력 ×1.5 → ×2.25"), TEXT("포카드 공격력 ×3.5 → ×5.25"),
		TEXT("로열·스트레이트 플러시 공격력 ×1.5 더"), TEXT("모든 타워 공격력 +30% / 고를 때 목숨 −3"), TEXT("♠ 타워 사거리 +30%"), TEXT("♥ 타워 폭발 범위 +0.5칸"),
		TEXT("♦ 타워 연사 +30%"), TEXT("조커 위력 +30%"), TEXT("먹이기 경험치 +50%"), TEXT("합성 타워 공격력 +30%"), TEXT("처치 경험치 +25%"),
		TEXT("Q 타워 연사 +80%"), TEXT("K 타워 사거리 +40%, 공격력 +30%"), TEXT("연쇄 3 이상에 든 타워 공격력 +30%"), TEXT("5장 족보 타워 공격력 +35%"), TEXT("3장 족보 타워 공격력 +40%"),
		TEXT("족보에 안 든 타워 공격력 +60%"), TEXT("와일드 카드 타워 공격력 ×2"), TEXT("레벨업할 때마다 목숨 +1"), TEXT("보스에게 주는 피해 +40%"), TEXT("웨이브 통과 경험치 ×2"),
		TEXT("J 타워 공격력 +80%"), TEXT("숫자 2 타워 공격력 ×3"), TEXT("판에 ♠♥♦♣ 가 다 있으면 모든 타워 공격력 +25%"), TEXT("스트레이트 플러시·로열 위력 +60%"), TEXT("투 페어 위력 +60%"),
		TEXT("트리플 공격 ×2 → ×2.6"), TEXT("포카드 위력 +50%"), TEXT("2장 족보 타워 공격력 +60%"), TEXT("4장 족보 타워 공격력 +40%"), TEXT("합성 타워 연사 +30%"), TEXT("조커 사거리 +30%"),
		TEXT("♠ 타워 사거리 +50%"), TEXT("♣ 타워 둔화 +15%p"), TEXT("♦ 타워 연사 +40%"), TEXT("보스 웨이브 동안 모든 피해 +30%"), TEXT("웨이브 시작 10초 동안 모든 피해 +50%"), TEXT("먹이기 흡수 60% → 90%"), TEXT("손패 최대 +1, 웨이브마다 카드 +1")};
	return Tr(N[J]);
}
// 상위 조커 30종 (D-231, docs/18 4절 순서)
inline const TCHAR* UpperName(int U)
{
	static const TCHAR* N[UpperJokerCount] = {TEXT("장미 칠하는 정원사"), TEXT("트위들덤과 트위들디"), TEXT("도도새"), TEXT("애벌레"), TEXT("공작부인"), TEXT("거울 나라"),
		TEXT("3월 토끼"), TEXT("하트 왕"), TEXT("하트 잭"), TEXT("미친 다과회"), TEXT("흰 토끼"), TEXT("험프티 덤프티"), TEXT("붉은 여왕"), TEXT("가짜 거북"), TEXT("그리핀"),
		TEXT("웃음만 남은 고양이"), TEXT("하얀 여왕"), TEXT("바다코끼리와 목수"), TEXT("하얀 기사"), TEXT("말하는 꽃들"), TEXT("하트 여왕"), TEXT("홍학 크로케"), TEXT("트럼프 근위대"),
		TEXT("체스판"), TEXT("겨울잠쥐"), TEXT("유니콘"), TEXT("재버워키"), TEXT("사자"), TEXT("꿈꾸는 붉은 왕"), TEXT("반짝반짝 작은 박쥐")};
	return Tr(N[FMath::Clamp(U, 0, UpperJokerCount - 1)]);
}
inline const TCHAR* UpperSpecialText(EPDUpperSpecial S)
{
	switch (S)
	{
	case EPDUpperSpecial::MadTea: return Tr(TEXT("주변 8칸 타워 사거리 무한"));
	case EPDUpperSpecial::WhiteRabbit: return Tr(TEXT("회중시계: 10초마다 판 전체 적 1초 멈춤"));
	case EPDUpperSpecial::GrinOnly: return Tr(TEXT("판 전체에서 가장 강한 타워를 따라 함"));
	case EPDUpperSpecial::QueenHearts: return Tr(TEXT("목을 쳐라: 체력 15% 이하 적 즉시 처치 (보스 제외)"));
	case EPDUpperSpecial::Dormouse: return Tr(TEXT("주변 8칸 타워 연사 +80%"));
	case EPDUpperSpecial::Jabberwock: return Tr(TEXT("보스를 먼저 노리고 보스에게 +50%"));
	case EPDUpperSpecial::Tweedle: return Tr(TEXT("둘째 적은 다음 무늬 공격으로"));
	case EPDUpperSpecial::Mirror: return Tr(TEXT("판 전체 피해의 무늬가 번갈아: ♥ 차례 위력 ×2, ♣ 차례 판 전체 둔화"));
	case EPDUpperSpecial::RedQueen: return Tr(TEXT("판 위 같은 무늬 중 가장 강한 타워를 따라 함"));
	case EPDUpperSpecial::MockTurtle: return Tr(TEXT("주변에서 가장 빠른 타워를 따라 함"));
	default: return TEXT("");
	}
}
inline const TCHAR* JokerAbility(int K);
inline const TCHAR* JokerName(int K)
{
	static const TCHAR* N[] = {TEXT("광대"), TEXT("모자 장수"), TEXT("웃는 가면"), TEXT("희비극"), TEXT("할리퀸"), TEXT("지휘봉"), TEXT("잭"), TEXT("체셔"), TEXT("방울"), TEXT("스타")};
	if (IsUpperJoker(K)) return UpperName(K - UpperBase);
	return Tr(N[FMath::Clamp(K, 0, 9)]);
}
inline const TCHAR* BaseJokerAbility(int K)
{
	static const TCHAR* N[] = {TEXT("무늬 4종 공격을 번갈아 씀"), TEXT("주변 8칸 타워 공격력 +25%·사거리 +10%"), TEXT("주변 가장 강한 타워의 공격을 따라 함"),
		TEXT("두 적을 동시에 공격"), TEXT("판 위 같은 무늬 타워 전부 공격력 +30%"), TEXT("주변 8칸 타워 연사 +40%"), TEXT("아주 강한 한 방 (공격력 ×3, 연사 ×0.5)"),
		TEXT("사거리 무한: 가장 앞선 적 저격"), TEXT("8초마다 종: 사거리 안 적 1.5초 멈춤"), TEXT("판 전체 레이저: 모든 적에게 피해")};
	return Tr(N[FMath::Clamp(K, 0, 9)]);
}
// 상위 조커 능력 글: 고유 능력이 있으면 그것, 없으면 "재료 A 능력 + 재료 B 능력"
inline const TCHAR* JokerAbility(int K)
{
	if (!IsUpperJoker(K)) return BaseJokerAbility(K);
	static FString Cache[UpperJokerCount][8]; // 고정 자리 (돌려준 포인터가 움직이지 않게), [상위 조커][언어]
	const int32 U = K - UpperBase, L = FMath::Clamp(GPDLang, 0, 7);
	FString& S = Cache[U][L];
	if (S.IsEmpty())
	{
		const FPDUpperDef& D = UpperDef(U);
		S = D.Special != EPDUpperSpecial::None ? FString(UpperSpecialText(D.Special)) + TEXT(" · ") : FString();
		S += FString(BaseJokerAbility((int)D.A)) + TEXT(" + ") + BaseJokerAbility((int)D.B);
	}
	return *S;
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
inline FString JokerRuleText() { return TEXT("족보 3개(또는 2개)를 골라 레시피와 맞으면 조커 · 판 어디에 있어도 된다 · 3개 쪽이 카드가 많아 세다"); }
inline FString JokerCommand(int K)
{
	if (IsUpperJoker(K)) { const FPDUpperDef& D = UpperDef(K - UpperBase); return TrF(TEXT("지휘: %s · %s 타워 위력 +30%% (겹침)"), HandName(CaptainHand(D.A)), HandName(CaptainHand(D.B))); }
	return TrF(TEXT("지휘: %s 타워 위력 +30%% (겹침)"), HandName(CaptainHand((EPDJoker)FMath::Clamp(K, 0, 9))));
}
inline FString JokerTitle(int K) // 툴팁 첫 줄
{
	if (IsUpperJoker(K)) { const FPDUpperDef& D = UpperDef(K - UpperBase); return TrF(TEXT("상위 조커: %s (%s + %s)"), JokerName(K), JokerName((int)D.A), JokerName((int)D.B)); }
	return TrF(TEXT("%s 대장: %s"), HandName(CaptainHand((EPDJoker)FMath::Clamp(K, 0, 9))), JokerName(K));
}
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
	static const TCHAR* N[] = {TEXT("와일드 카드"), TEXT("보충"), TEXT("수리"), TEXT("경험치"), TEXT("자리바꿈")};
	return Tr(N[I]);
}
inline const TCHAR* InstantDesc(int I)
{
	static const TCHAR* N[] = {TEXT("아무 숫자·무늬로 치는 카드 1장 (드물게 나옴)"), TEXT("카드 2장 뽑기"), TEXT("목숨 +3"), TEXT("경험치 +(10 + 웨이브 × 4)"), TEXT("타워 둘의 자리를 바꾸거나 빈 칸으로 옮기기 2번 (레벨·조커 그대로)")};
	return Tr(N[I]);
}
// 보상 카드 위치 (판 가운데 3장)
// 칵테일 보상 = 바텐더 추천 (D-286): 선반 위 잔 + 가격표 꼬리표. 칸 = 잔부터 꼬리표 아래 글자까지 (마우스 · 클릭)
constexpr float RewardW = 220.f, RewardH = 236.f, RewardShelfY = FY + 270.f;
inline FVector2f RewardPos(int I) { return FVector2f(FX + Cols * CS / 2.f - RewardW / 2.f + (I - 1) * 245.f, FY + 146.f); }
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
	static const TCHAR* N[] = {TEXT("외골수"), TEXT("잡식가"), TEXT("큰 판"), TEXT("작은 판"), TEXT("대장장이"), TEXT("광대의 길"), TEXT("고독한 저격수"), TEXT("군중"), TEXT("왕관의 무게"), TEXT("서커스")};
	return Tr(N[K]);
}
inline const TCHAR* KeystoneDesc(int K)
{
	static const TCHAR* N[] = {TEXT("플러시 타워 위력 ×2 / 다른 족보 타워 위력 ×0.5"), TEXT("플러시 아닌 족보 위력 ×1.3 / 플러시 범위 효과 없음"),
		TEXT("5장 족보 위력 ×1.6 / 2장 족보는 효과·연쇄 없음"), TEXT("원 페어·투 페어 위력 ×1.8 / 5장 족보는 연쇄 덤 없음"),
		TEXT("합성 배율 1.5 → 2.0 / 조커 포탑을 만들 수 없음"), TEXT("조커 배율 1.2 → 1.6 / 합성은 레벨 2까지"),
		TEXT("족보에 안 든 타워 위력 ×1.6 / 연쇄 덤 없음"), TEXT("연쇄 덤 +25% → +40% / 족보에 안 든 타워 위력 ×0.5"),
		TEXT("상위 조커 위력 배율 ×1.5 → ×2.2 / 기본 조커 지휘 없음"), TEXT("판 위 기본 조커 하나마다 모든 조커 위력 +15% / 상위 조커를 만들 수 없음")};
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
	if (Mask == (B(EPDHand::Pair) | B(EPDHand::TwoPair))) return Tr(TEXT("원 페어·투 페어")); // 언어팩 (D-219)
	if (Mask == (B(EPDHand::Triple) | B(EPDHand::Four) | B(EPDHand::Five))) return Tr(TEXT("트리플·포카드·파이브 카드")); // 언어팩 (D-219)
	if (Mask == (B(EPDHand::Flush) | B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return Tr(TEXT("플러시 계열")); // 언어팩 (D-219)
	if (Mask == (B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return Tr(TEXT("스트레이트 플러시·로열")); // 언어팩 (D-219)
	if (Mask == (B(EPDHand::Straight) | B(EPDHand::StraightFlush) | B(EPDHand::RoyalFlush))) return Tr(TEXT("스트레이트 계열")); // 언어팩 (D-219)
	if (Mask == (B(EPDHand::Pair) | B(EPDHand::TwoPair) | B(EPDHand::Triple) | B(EPDHand::FullHouse) | B(EPDHand::Four) | B(EPDHand::Five))) return Tr(TEXT("같은 숫자 족보")); // 언어팩 (D-219)
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
	case EPDStat::UpperPrep: return TrF(TEXT("상위 조커 위력 +%d%%, 조건 조커 레벨 10 → 7"), Pct);
	case EPDStat::XpPct: return TrF(TEXT("경험치 +%d%%"), Pct);
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
	static const TCHAR* N[] = {TEXT("공격"), TEXT("공격 속도"), TEXT("치명타 확률"), TEXT("치명타 배율"), TEXT("스페이드"), TEXT("하트"), TEXT("다이아"), TEXT("클로버")}; // 카드 시안 A (D-244): 무늬는 아이콘으로
	return Tr(N[U]);
}
inline FString UpgradeEffect(int U, int Level)
{
	const float V = UpgradeStep((EPDUpgrade)U) * Level;
	switch ((EPDUpgrade)U)
	{
	case EPDUpgrade::Attack: return TrF(TEXT("모든 타워 공격력 +%.0f%%  (레벨당 +3%%)"), V * 100.f);
	case EPDUpgrade::Speed: return TrF(TEXT("모든 타워 공격 속도 +%.0f%%  (레벨당 +2%%)"), V * 100.f);
	case EPDUpgrade::Crit: return TrF(TEXT("치명타 확률 %.0f%%  (레벨당 +1%%p)"), (BaseCrit + V) * 100.f);
	case EPDUpgrade::CritMult: return TrF(TEXT("치명타 배율 ×%.2f  (레벨당 +0.05)"), BaseCritMult + V);
	default: return TrF(TEXT("그 무늬 타워 공격력 +%.0f%%  (레벨당 +4%%)"), V * 100.f);
	}
}
inline FLinearColor UpgradeColor(int U) { return U >= (int)EPDUpgrade::Spade ? SuitColor((EPDSuit)(U - (int)EPDUpgrade::Spade)) : U >= (int)EPDUpgrade::Crit ? Hex(TEXT("ff9a6a")) : Hex(TEXT("ffd96a")); }
constexpr float UpgRowX = 150.f, UpgRowY = 120.f, UpgRowW = 980.f, UpgRowH = 64.f;
// 영구 강화 카드 (D-244, 시안 A): 4 × 2 카드, 카드마다 아이콘 · 레벨 고리 · 효과 · 버튼 넷
constexpr float UpgCardW = 276.f, UpgCardH = 260.f;
inline FVector2f UpgCardPos(int I) { return FVector2f(60.f + (I % 4) * 296.f, 120.f + (I / 4) * 280.f); }
constexpr float UpgBtnW = 58.f, UpgBtnH = 38.f;
const int32 UpgSteps[4] = {1, 10, 100, UpgradeMaxCleared}; // 영구 강화 버튼: +1 · +10 · +100 · 최대 (D-238)
// 부적 상자 버튼 3개 (D-233): 영구 강화 창 아래
constexpr float BoxBtnW = 233.f, BoxBtnH = 44.f, BoxBtnY = UpgRowY + 8 * UpgRowH + 6.f;
inline float BoxBtnX(int I) { return UpgRowX + I * (BoxBtnW + 16.f); }
inline FVector2f UpgButtonPos(int Row, int K) { const FVector2f C0 = UpgCardPos(Row); return FVector2f(C0.X + 14.f + K * 63.f, C0.Y + 208.f); }
// 웨이브 시작 버튼 아래 버튼 한 줄 (D-119): 트리 K · 강화 U. 도감 · 메뉴 버튼은 뺐다 (D-349): 위 톱니바퀴 = 메뉴, 도감은 메뉴 안 (J 키는 그대로)
// 오른쪽 패널 아래 셋 크기 맞춤 (D-353, 시안 B tools/neon/rpanel_mock.py): 웨이브 시작 · 스킬 트리 둘 다 높이 52 · 모서리 12, 사이는 모두 10
constexpr float RBtnR = 12.f; // 웨이브 시작 · 스킬 트리 버튼 모서리
constexpr float TreeBtnX = RBtnX, TreeBtnY = StartBtnY + StartBtnH + 10.f, TreeBtnW = (RBtnW - 8.f) / 2.f, TreeBtnH = StartBtnH;
constexpr float UpgOpenX = RBtnX + TreeBtnW + 8.f, UpgOpenY = TreeBtnY, UpgOpenW = TreeBtnW, UpgOpenH = TreeBtnH;
constexpr float CharmY = TreeBtnY + TreeBtnH + 10.f; // 부적 칸 (바닥선까지)
// 위에 걸린 간판 (D-274, 인게임 시안 B)
constexpr float SignEnX = 270.f, SignEnY = 20.f, SignEnW = 182.f, SignEnH = 68.f;   // 남은 적 (전구 줄)
// 주크박스 간판 (D-319): 예전 남은 적 간판 자리 (SignEn). ◀ ▶ 곡 넘기기
// 실제 주크박스 (사용자 2026-11-16): 본체 하나(128 × 94, x 297 ~ 425, y 2 ~ 96). 위 아치 무지개 튜브 → 유리창 안 곡 정보 · 도는 레코드 → 선곡 버튼(◀ 번호 ▶) → 스피커 → 받침, 양옆 거품 튜브. 시안 tools/neon/jukebox_mock.py
// 주크박스 2c (D-351, 시안 tools/neon/jukebox_wide_mock.py): 설계 단위(폭 JukeDW × 높이 JukeDH)로 그리고 JukeK 배로 줄여 찍는다 → 화면 폭 150 · 높이 85, 위 끝 y 2 · 바닥 y 87 (판 틀 선 y 92 위로 5px)
constexpr float JukeK = 0.9f, JukeW = 150.f, JukeDW = JukeW / JukeK, JukeDH = 94.f, JukeH = JukeDH * JukeK;
constexpr float JukeX0 = 346.f - JukeW / 2.f, JukeY0 = 2.f;
constexpr float JukeBtnS = 16.f * JukeK, JukeBtnY = JukeY0 + 62.f * JukeK;
inline float JukeBtnX(int Dir) { return JukeX0 + JukeW / 2.f + Dir * 30.f * JukeK - JukeBtnS / 2.f; }
// 윗줄 한 장 계기판 (D-351, 시안 B tools/neon/topbar_mock.py): 주크박스 오른쪽 긴 판 하나 + 세로 구분선 둘 → WAVE | 목숨 | 레벨. 머리글 · 다음 보스 줄은 뺐다
constexpr float HudPanX = 430.f, HudPanY = 16.f, HudPanW = 706.f, HudPanH = 68.f;
constexpr float HudDivLf = 800.f, HudDivLv = 930.f; // 목숨 칸 · 레벨 칸 왼쪽 구분선
// 레벨 칸 (사용자 2026-11-16): 마름모 · 숫자는 목숨 칸과 같은 자리 · 글자, 경험치 막대는 숫자 오른쪽에 두껍게 (가운데 높이 LvBarY, % 는 막대 안)
constexpr float LvBarX = HudDivLv + 116.f, LvBarH = 24.f /* 숫자 글자 크기와 같은 높이 (시안 B tools/neon/lvbar_mock.py) */, LvBarY = HudPanY + HudPanH / 2.f, LvBarW = HudPanX + HudPanW - 12.f - LvBarX; // 경험치 막대 (LvBarY = 막대 가운데)
constexpr float LvSignCX = (HudDivLv + HudPanX + HudPanW) / 2.f; // 레벨 칸 가운데 (레벨업 간판이 날아가 매달리는 자리)
constexpr float FloorY = FY + Rows * CS + 8.f;                                      // 바닥선 (손패가 그 아래)
// 조커 도감 (D-119): 카드 5×2 + 오른쪽 크게 보기
constexpr float CdxCardX = 16.f, CdxCardY = 76.f, CdxCardW = 164.f, CdxCardH = 304.f, CdxGapX = 8.f, CdxGapY = 12.f;
constexpr float CdxDetX = 884.f, CdxDetW = 380.f;
// 도감 쪽 탭 (D-231): 기본 조커 / 상위 조커. 위쪽 띠 안 오른쪽
constexpr float CdxTabW = 150.f, CdxTabH = 30.f, CdxTabY = HudY + 10.f, CdxTabX0 = 820.f;
inline float CdxTabX(int I) { return CdxTabX0 - 110.f + I * (CdxTabW - 30.f + 8.f); } // 탭 3개 (기본 · 상위 · 부적)
// 부적 쪽 (D-234): 6 × 7 카드
constexpr float ChCardW = 118.f, ChCardH = 96.f, ChCardGap = 6.f, ChCardY0 = 78.f; // 칵테일 60개 = 10 × 6 (D-239)
inline FVector2f CharmCardPos(int J) { return FVector2f(DesignW / 2.f - (10.f * ChCardW + 9.f * ChCardGap) / 2.f + (J % 10) * (ChCardW + ChCardGap), ChCardY0 + (J / 10) * (ChCardH + ChCardGap)); }
// 잠금 해제 창 (D-239): 탭 둘(칵테일·조커), 칵테일은 도감과 같은 10 × 6, 조커는 5 × 2 큰 카드
constexpr float UnlTabW = 140.f, UnlTabH = 32.f, UnlTabY = 34.f;
inline float UnlTabX(int I) { return DesignW / 2.f - UnlTabW - 6.f + I * (UnlTabW + 12.f); }
constexpr float UnlJW = 228.f, UnlJH = 112.f, UnlJGap = 12.f, UnlJY0 = 96.f; // 기본 조커 5 × 2, 그 아래 상위 조커 10 × 3 (D-241)
constexpr float UnlUW = 118.f, UnlUH = 96.f, UnlUGap = 6.f, UnlUY0 = 366.f;
inline FVector2f UnlUpperPos(int U) { return FVector2f(DesignW / 2.f - (10.f * UnlUW + 9.f * UnlUGap) / 2.f + (U % 10) * (UnlUW + UnlUGap), UnlUY0 + (U / 10) * (UnlUH + UnlUGap)); }
inline FVector2f UnlJokerPos(int J) { return FVector2f(DesignW / 2.f - (5.f * UnlJW + 4.f * UnlJGap) / 2.f + (J % 5) * (UnlJW + UnlJGap), UnlJY0 + (J / 5) * (UnlJH + UnlJGap)); }
// 잠금 해제 시안 C (D-261): 걸린 메뉴판 셋. 가운데 큰 판 = 지금 탭, 양옆 작은 판 = 다른 두 탭 (누르면 바뀜). 줄 2단 × 15 = 한 쪽 30개
constexpr float UnlBX = 260.f, UnlBY = 70.f, UnlBW = 760.f, UnlBH = 620.f;
constexpr float UnlSideW = 210.f, UnlSideH = UnlBH, UnlSideY = UnlBY; // 가운데 큰 판과 위아래 끝을 맞춘다 (D-328 ⑥, 예전 200 × 150)
inline float UnlSideX(int Side) { return Side == 0 ? 40.f : DesignW - 40.f - UnlSideW; }
constexpr int32 UnlPerPage = 30;
constexpr float UnlRowW = 330.f, UnlRowH = 26.f;
inline FVector2f UnlRowPos(int Slot) { return FVector2f(305.f + (Slot / 15) * 360.f, 160.f + (Slot % 15) * 30.f); }
constexpr float UnlPgW = 44.f, UnlPgH = 30.f, UnlPgY = 652.f;
inline float UnlPgX(int Dir) { return Dir < 0 ? UnlBX + 30.f : UnlBX + UnlBW - 30.f - UnlPgW; }
// 상위 조커 쪽: 6 × 5 카드
constexpr float UpCardW = 192.f, UpCardH = 112.f, UpCardGap = 8.f, UpCardY0 = 80.f;
inline FVector2f UpperCardPos(int U) { return FVector2f(DesignW / 2.f - (6.f * UpCardW + 5.f * UpCardGap) / 2.f + (U % 6) * (UpCardW + UpCardGap), UpCardY0 + (U / 6) * (UpCardH + UpCardGap)); }
// 도감 책자 (D-354, 시안 B tools/neon/codex_book_mock.py · 넘기기 tools/neon/codex_flip_mock.py)
constexpr float CdxBX = 40.f, CdxBY = 60.f, CdxBW = 1140.f, CdxBH = 610.f, CdxSpX = CdxBX + CdxBW / 2.f; // 책 바깥 · 책등
constexpr float CdxPgW = CdxBW / 2.f - 28.f, CdxLX = CdxBX + 22.f, CdxRX = CdxSpX + 6.f, CdxTop = CdxBY + 22.f, CdxBot = CdxBY + CdxBH - 22.f; // 쪽
constexpr int32 CdxPerPage = 12;
constexpr float CdxRowY0 = CdxBY + 100.f, CdxRowH = 36.f; // 오른쪽 쪽 목록 줄
constexpr float CdxIdxX = CdxBX + CdxBW, CdxIdxW = 64.f, CdxIdxH = 82.f; // 오른쪽 옆면 색인 탭 (고른 탭은 10 더 나옴)
inline float CdxIdxY(int I) { return CdxBY + 30.f + I * 92.f; }
constexpr float CdxNavW = 44.f, CdxNavH = 26.f, CdxNavY = CdxBot - 53.f; // 쪽 넘기기 ‹ › (바깥 아래 귀퉁이)
inline float CdxNavX(int Dir) { return Dir < 0 ? CdxLX + 18.f : CdxRX + CdxPgW - 18.f - CdxNavW; }
constexpr float CdxFlipDur = 0.5f, CdxFlipFast = 0.32f, CdxFlipGap = 0.1f; // 한 장 넘김 · 탭 바꿀 때 장마다 · 장 사이
constexpr float CdxCloseX = 1222.f, CdxCloseY = 22.f, CdxCloseW = 28.f, CdxCloseH = 28.f; // 닫기 ⓧ 버튼 자리 (트리 · 도감 · 강화 창, D-388: '닫기 K' 알약 → 동그라미 X, 오른쪽 끝 1250 그대로)
inline FVector2f CodexCardPos(int K) { return FVector2f(CdxCardX + (K % 5) * (CdxCardW + CdxGapX), CdxCardY + (K / 5) * (CdxCardH + CdxGapY)); }
// 메뉴 창 (Esc): 가운데 세로 버튼
enum class EMenuItem : uint8_t { Resume, NewGame, Title, Settings, Codex, Quit, Count }; // Codex: 도감 (D-349, 판 위 도감 버튼 대신) // Title: 메인 메뉴로 (D-287) // 튜토리얼은 뺐다 (D-217, 도움말로)
constexpr float MenuW = 300.f, MenuItemH = 44.f, MenuGap = 12.f;
constexpr float MenuX = DesignW / 2.f - MenuW / 2.f, MenuItemY0 = 260.f; // 버튼 6개 (D-287, D-349)
inline FVector2f MenuItemPos(int I) { return FVector2f(MenuX, MenuItemY0 + I * (MenuItemH + MenuGap)); }
// 시작 화면 (D-153): 게임 시작 · 환경 설정 · 게임 종료. 게임 시작 → 로딩 화면 → 인게임
enum class ETitleItem : uint8_t { Continue, Start, Upgrades, Unlock, Settings, Quit, Count }; // 랭킹은 뺐다 (D-242) // Unlock: 잠금 해제 (D-239) // 영구 강화 (D-230) // 이어하기는 중단 저장이 있을 때만 (D-162)
// 큰 창 자리 (랭킹이 쓴다. 튜토리얼 화면 D-166 은 D-217 에 뺐다)
constexpr float TutW = 780.f, TutH = 500.f, TutX = DesignW / 2.f - TutW / 2.f, TutY = 104.f;
constexpr float TutBtnW = 120.f, TutBtnH = 38.f, TutBtnY = TutY + TutH - 56.f;
constexpr float TutNextX = TutX + TutW - 24.f - TutBtnW, TutPrevX = TutNextX - 12.f - TutBtnW, TutSkipX = TutX + 24.f;
constexpr float TitleBtnW = 300.f, TitleBtnH = 44.f, TitleBtnGap = 14.f, TitleBtnX = 860.f; // 대문 시안 B (D-253): 오른쪽 세로 튜브 버튼. 바닥 경계 · 반사는 뺐다 (D-381)
// 랭킹 (D-168): 개인 랭킹 (이 PC 기록 상위 20) + 리더보드 (스팀, 준비 중). 창 크기는 튜토리얼과 같다
constexpr int32 RankMax = 20;
constexpr float RankTabW = 120.f, RankTabH = 34.f, RankTabY = 104.f + 60.f, RankTabGap = 10.f; // 탭 = 모드 4개 (D-229)
inline float RankTabX(int I) { return DesignW / 2.f - (4.f * RankTabW + 3.f * RankTabGap) / 2.f + I * (RankTabW + RankTabGap); }
// 모드 고르기 창 (D-229): 시작 화면 '게임 시작'에서 연다. 카드 4장 (30 · 60 · 100 · 무한)
constexpr float ModeCardW = 170.f, ModeCardH = 210.f, ModeCardGap = 20.f, ModeCardY = 250.f;
inline FVector2f ModeCardPos(int I) { return FVector2f(DesignW / 2.f - (4.f * ModeCardW + 3.f * ModeCardGap) / 2.f + I * (ModeCardW + ModeCardGap), ModeCardY); }
inline FVector2f TitleBtnPos(int I, bool bCont) // 이어하기가 없으면 위로 당긴다. 버튼 묶음은 화면 위아래 가운데 (D-381)
{
	const int K = bCont ? I : I - 1, N = bCont ? (int)ETitleItem::Count : (int)ETitleItem::Count - 1;
	const float Y0 = DesignH / 2.f - (N * TitleBtnH + (N - 1) * TitleBtnGap) / 2.f;
	return FVector2f(TitleBtnX, Y0 + K * (TitleBtnH + TitleBtnGap));
}
// 버튼 색 규칙 (D-381): 기본 = 하늘색, 주 행동(시작 · 이어하기 · 확정) = 초록, 나가기 · 위험(종료) = 분홍. 마우스 올림은 같은 색으로 밝아지기만 한다
inline FLinearColor BtnColDefault() { return Hex(TEXT("6edcff")); }
inline FLinearColor BtnColPrimary() { return Hex(TEXT("9cffb0")); }
inline FLinearColor BtnColExit() { return Hex(TEXT("ff6aaa")); }
// 마우스 올림 · 고름 글자색 (D-381, D-382): 흰색으로 바꾸지 않고 같은 색을 밝게 (색상은 그대로, 밝기만 올림)
inline FLinearColor HotCol(const FLinearColor& C) { return FLinearColor(FMath::Min(1.f, C.R * 1.15f + 0.08f), FMath::Min(1.f, C.G * 1.15f + 0.08f), FMath::Min(1.f, C.B * 1.15f + 0.08f), 1.f); }
// 게임 오버 · 클리어 버튼 (D-293): 영수증 아래
constexpr float OverBtnY = 500.f, OverBtnW = 230.f, OverBtnH = 44.f, OverRestartX = DesignW / 2.f - 250.f, OverMenuX = DesignW / 2.f + 20.f;
constexpr float ClearBtnY = 600.f, ClearBtnX = DesignW / 2.f - 115.f;
constexpr float LoadDur = 1.4f; // 로딩 화면 길이 (초)
inline const TCHAR* MenuItemName(int I)
{
	static const TCHAR* N[] = {TEXT("계속하기"), TEXT("새 게임"), TEXT("메인 메뉴로"), TEXT("환경 설정"), TEXT("도감"), TEXT("게임 종료")}; // '설정' → 시작 화면과 같은 '환경 설정' (D-390)
	return Tr(N[I]);
}
// 설정 창 (D-110): 줄마다 이름 + 버튼
enum class ESetRow : uint8_t { Lang, Window, CardTheme, Frame, VSync, WorldFx, Shake, Cine, Sound, Volume, Music, Track, Help, Count }; // 닫기 줄은 뺐다 → 오른쪽 위 ⓧ 버튼 (D-384) // Track: 주크박스 곡 ◀ 이름 ▶ (D-319) // Cine: 완성 연출 전부/조커만/끔 (D-314) // Help: 도움말 켜기·처음부터 (D-217)
constexpr float SetW = 560.f, SetRowH = 33.f, SetRowGap = 2.f, SetLabelW = 200.f, SetBtnH = 30.f;
constexpr float SetX = DesignW / 2.f - SetW / 2.f, SetY0 = 140.f; // 언어 줄이 늘어 조금 위로 (D-159)
constexpr float SetBoxX = SetX - 30.f, SetBoxY = SetY0 - 76.f, SetBoxW = SetW + 60.f;
constexpr float SetBoxH = (int)ESetRow::Count * (SetRowH + SetRowGap) + 120.f;
inline float SetRowY(int Row) { return SetY0 + Row * (SetRowH + SetRowGap); }
constexpr float SetCloseR = 13.f, SetCloseCX = SetX + SetLabelW + 352.f - SetCloseR, SetCloseCY = SetBoxY + 49.f; // 닫기 ⓧ (D-384): 아치 어깨 바로 아래, 언어 줄 위. 오른쪽 끝을 선택지 버튼 열 오른쪽 끝(X0 + 352)에 맞춤 (D-387)
inline bool SetBarRow(int Row) { return Row == (int)ESetRow::Volume || Row == (int)ESetRow::Music; } // −/막대/+ 줄
inline int SetBtnCount(int Row) { return Row == (int)ESetRow::Lang ? (int)EPDLang::Count : Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame || Row == (int)ESetRow::Cine || Row == (int)ESetRow::Track || SetBarRow(Row) ? 3 : Row == (int)ESetRow::Help ? 2 : 1; }
inline FLinearColor LevelDotColor() { return GPDCardTheme == 1 ? FLinearColor::White : Hex(TEXT("ffd96a")); } // 타워 레벨 점 (흑백이면 흰빛)
inline float SetBtnW(int Row, int Idx)
{
	if (Row == (int)ESetRow::Lang) return 66.f; // 언어 다섯 (D-159)
	if (Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame || Row == (int)ESetRow::Cine) return 112.f;
	if (SetBarRow(Row) || Row == (int)ESetRow::Track) return Idx == 1 ? 220.f : 40.f;
	if (Row == (int)ESetRow::Help) return Idx == 0 ? 120.f : 150.f;
	return 120.f;
}
inline FVector2f SetBtnPos(int Row, int Idx) // 왼쪽 위
{
	const float X0 = SetX + SetLabelW, Y = SetRowY(Row) + (SetRowH - SetBtnH) / 2.f;
	if (Row == (int)ESetRow::Lang) return FVector2f(X0 + Idx * (66.f + 6.f), Y);
	if (Row == (int)ESetRow::Window || Row == (int)ESetRow::CardTheme || Row == (int)ESetRow::Frame || Row == (int)ESetRow::Cine) return FVector2f(X0 + Idx * (112.f + 8.f), Y);
	if (SetBarRow(Row) || Row == (int)ESetRow::Track) return FVector2f(Idx == 0 ? X0 : Idx == 1 ? X0 + 48.f : X0 + 48.f + 220.f + 8.f, Y);
	if (Row == (int)ESetRow::Help) return FVector2f(Idx == 0 ? X0 : X0 + 128.f, Y);
	return FVector2f(X0, Y);
}
// 손패 카드 왼쪽 X. 카드가 많으면 판 너비 안에 들어오게 간격을 좁힌다
inline float HandCardX(int I, int N)
{
	const float Avail = Cols * CS - 96.f - CardW;
	const float Step = N > 1 ? FMath::Min(CardW + 10.f, Avail / (N - 1)) : 0.f;
	return FX + 96.f + I * Step;
}
// 모서리 둥글기 세 단계 (D-328 ①): 버튼 = 알약(높이의 절반), 큰 창 · 판 · 카드 = RadPanel (두 겹 틀 안쪽 선 = RadSmall), 작은 것(줄 강조 · 툴팁 · 칸 · 작은 알약 표) = RadSmall
constexpr float RadPanel = 14.f, RadSmall = 8.f;
inline float RadPill(float H) { return H / 2.f; }
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
// 레이아웃 검사 (D-365, F2): 한 프레임 동안 그린 글자 · 판(불투명 상자) · 버튼 · 선의 자리를 모아 서로 부딪히는 곳을 찾는다.
//   글자 ↔ 글자 겹침 · 선이 글자를 지나감 · 글자가 판/버튼 테두리에 걸침 · 선이 버튼을 가로지름 · 글자가 화면 밖 · 칸이 좁아 "…"로 잘림
//   그림 ↔ 그림 일부 겹침 · 그림이 글자에 걸침 (D-366): 그림 함수(무늬 · 카드 · 조커 · 상위 조커 · 아이콘 · 전구 · 스프라이트) 하나가 그린 것을 한 덩어리로,
//   옅은 선(알파 < 0.3, 빛 고리)은 빼고 바깥 상자를 잰다. 한쪽이 다른 쪽 안에 다 들어가면(카드 속 무늬) 일부러 그린 것이라 뺀다.
//   그림은 고정 패널 · 창(C 판)에서만 본다: 판(F)의 타워는 칸에 맞춰 놓여 겹칠 수 없고, 적 · 이펙트 · 판 위 말풍선은 원래 겹친다
// 켜 두면 0.5초마다 다시 찾는다. 찾은 곳은 화면에 빨간 테두리, Saved/Logs/PDLayout.log 에 화면 이름과 함께 (같은 것은 한 번만).
// 흐린 장식 선(벽돌 · 격자 등, 밝기 × 알파 < 0.35)과 굵은 번짐 띠, 거의 안 보이는 글자(알파 < 0.15)는 보지 않는다. 위젯 헤더를 덜 건드리려고 전역
struct FPDLayoutCheck
{
	struct FR { float X0 = 0.f, Y0 = 0.f, X1 = 0.f, Y1 = 0.f; float W() const { return X1 - X0; } float H() const { return Y1 - Y0; } };
	struct FTxt { FR B, G; FString S; bool bCut = false; int32 Obj = -1; }; // B = 글자 몸통 (글자끼리 · 선), G = 글자 칸 거의 전부 (판 테두리에 닿는지), Obj = 이 글자를 그린 그림
	struct FObj { FR B; FString N; };
	struct FPanel { FR B; bool bBtn = false; };
	struct FSeg { FVector2f A, B; };
	struct FHit { FR A, B; bool bTwo = false; };
	bool bOn = false, bCap = false;
	double NextAt = 0.0;
	TArray<FTxt> Texts;
	TArray<FPanel> Panels;
	TArray<FSeg> Segs;
	TArray<FObj> Objs;
	TArray<FR> Clip;
	int32 ObjDepth = 0;  // 그림 함수 안 (겹친 그림은 바깥 것 하나로)
	bool bObjHas = false;
	FR ObjB;
	FString ObjN;
	TArray<FHit> Hits;        // 마지막으로 찾은 것 (화면에 빨갛게)
	TArray<FString> Logged;   // 이미 로그에 쓴 것 (같은 것은 한 번만)
	TArray<FString> NewLines; // 이번에 새로 찾은 것 (로그 파일로)
	int32 Found = 0;          // 켠 뒤 로그에 쓴 개수

	void Begin() { Texts.Reset(); Panels.Reset(); Segs.Reset(); Objs.Reset(); Clip.Reset(); ObjDepth = 0; }
	// 그림 덩어리: 바깥 그림 함수가 시작할 때 열고 끝날 때 닫는다. 그 사이에 그린 선 · 상자 · 글자로 바깥 상자를 넓힌다
	void BeginObj(const TCHAR* N) { if (ObjDepth++ == 0) { bObjHas = false; ObjN = N; } }
	void EndObj()
	{
		if (ObjDepth <= 0 || --ObjDepth > 0) return;
		if (bObjHas && ObjB.W() >= 3.f && ObjB.H() >= 3.f) Objs.Add({ObjB, ObjN});
	}
	void Grow(const FR& R, float Alpha)
	{
		if (ObjDepth == 0 || Alpha < 0.3f) return;
		if (!bObjHas) { ObjB = R; bObjHas = true; return; }
		ObjB = {FMath::Min(ObjB.X0, R.X0), FMath::Min(ObjB.Y0, R.Y0), FMath::Max(ObjB.X1, R.X1), FMath::Max(ObjB.Y1, R.Y1)};
	}
	static bool Inter(const FR& A, const FR& B, FR& O)
	{
		O = {FMath::Max(A.X0, B.X0), FMath::Max(A.Y0, B.Y0), FMath::Min(A.X1, B.X1), FMath::Min(A.Y1, B.Y1)};
		return O.X1 > O.X0 && O.Y1 > O.Y0;
	}
	static bool Inside(const FR& In, const FR& Out, float Tol) { return In.X0 >= Out.X0 - Tol && In.Y0 >= Out.Y0 - Tol && In.X1 <= Out.X1 + Tol && In.Y1 <= Out.Y1 + Tol; }
	static FR Inset(const FR& R, float D) { return {R.X0 + D, R.Y0 + D, R.X1 - D, R.Y1 - D}; }
	// 선분이 사각 안을 지나는 길이 (리앙-바스키 자르기)
	static float SegIn(const FVector2f& A, const FVector2f& B, const FR& R)
	{
		float T0 = 0.f, T1 = 1.f;
		const float Dx = B.X - A.X, Dy = B.Y - A.Y;
		const float P[4] = {-Dx, Dx, -Dy, Dy}, Q[4] = {A.X - R.X0, R.X1 - A.X, A.Y - R.Y0, R.Y1 - A.Y};
		for (int32 K = 0; K < 4; ++K)
		{
			if (FMath::Abs(P[K]) < 1e-6f) { if (Q[K] < 0.f) return 0.f; continue; }
			const float T = Q[K] / P[K];
			if (P[K] < 0.f) T0 = FMath::Max(T0, T); else T1 = FMath::Min(T1, T);
			if (T0 >= T1) return 0.f;
		}
		return (T1 - T0) * FMath::Sqrt(Dx * Dx + Dy * Dy);
	}
	// 글자: 잰 상자(줄 높이)에서 몸통(위 20% · 아래 15% 여백을 뺌)과 거의 전부(위아래 8%만 뺌)
	void AddText(const FString& S, float L, float T, float W, float H, bool bCut, float Alpha)
	{
		if (Alpha < 0.15f || S.IsEmpty()) return;
		FR B{L + 0.5f, T + 0.2f * H, L + W - 0.5f, T + 0.85f * H}, G{L, T + 0.08f * H, L + W, T + 0.92f * H};
		if (Clip.Num() && (!Inter(B, Clip.Last(), B) || !Inter(G, Clip.Last(), G))) return;
		Grow(B, Alpha);
		Texts.Add({B, G, S, bCut, ObjDepth > 0 ? Objs.Num() : -1}); // 그림 안 글자: 그 그림이 닫히면 받을 번호
	}
	void AddPanel(float X, float Y, float W, float H, float Alpha, bool bBtn)
	{
		if (!bBtn) Grow({X, Y, X + W, Y + H}, Alpha);
		if (ObjDepth > 0) return; // 그림 속 상자는 판이 아니다
		if (W < 10.f || H < 8.f || (!bBtn && Alpha < 0.5f) || (W > DesignW * 0.8f && H > DesignH * 0.8f)) return; // 화면 전체 어둡게 깔기는 빼고
		FR B{X, Y, X + W, Y + H};
		if (Clip.Num() && !Inter(B, Clip.Last(), B)) return;
		Panels.Add({B, bBtn});
	}
	void AddLines(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick)
	{
		if (ObjDepth > 0 && Thick <= 3.f) for (const FVector2f& Q : P) Grow({Q.X - 0.5f, Q.Y - 0.5f, Q.X + 0.5f, Q.Y + 0.5f}, Col.A);
		if (Thick > 3.f || FMath::Max3(Col.R, Col.G, Col.B) * Col.A < 0.35f) return; // 번짐 띠 · 흐린 장식 선은 빼고
		for (int32 I = 0; I + 1 < P.Num(); ++I)
		{
			if (Clip.Num())
			{
				const FR& C = Clip.Last();
				if (FMath::Max(P[I].X, P[I + 1].X) < C.X0 || FMath::Min(P[I].X, P[I + 1].X) > C.X1 || FMath::Max(P[I].Y, P[I + 1].Y) < C.Y0 || FMath::Min(P[I].Y, P[I + 1].Y) > C.Y1) continue;
			}
			Segs.Add({P[I], P[I + 1]});
		}
	}
	void Analyze(const FString& Tag)
	{
		Hits.Reset();
		auto Rs = [](const FR& R) { return FString::Printf(TEXT("(%.0f,%.0f %.0fx%.0f)"), R.X0, R.Y0, R.W(), R.H()); };
		auto Q = [](const FString& S) { return TEXT("'") + (S.Len() > 28 ? S.Left(28) + TEXT("…") : S) + TEXT("'"); };
		auto Note = [&](const TCHAR* Kind, const FString& What, const FR& A, const FR* B)
		{
			if (Hits.Num() < 300) Hits.Add({A, B ? *B : A, B != nullptr});
			const FString Key = FString::Printf(TEXT("%s|%s|%d|%d"), Kind, *What, FMath::RoundToInt(A.X0 / 8.f), FMath::RoundToInt(A.Y0 / 8.f)); // 움직이는 것은 8px 단위로 같은 것
			if (Logged.Contains(Key) || Logged.Num() > 2000) return;
			Logged.Add(Key);
			++Found;
			NewLines.Add(FString::Printf(TEXT("[%s] %s: %s %s%s"), *Tag, Kind, *What, *Rs(A), B ? *(TEXT(" ↔ ") + Rs(*B)) : TEXT("")));
		};
		const FR Screen{0.f, 0.f, DesignW, DesignH};
		for (int32 I = 0; I < Texts.Num(); ++I)
		{
			const FTxt& A = Texts[I];
			if (!Inside(A.B, Screen, 1.f)) Note(TEXT("TEXT off screen"), Q(A.S), A.B, nullptr);
			if (A.bCut) Note(TEXT("TEXT cut to fit (…)"), Q(A.S), A.B, nullptr);
			// 글자 ↔ 글자 (같은 글자가 거의 같은 자리 = 번짐 · 네온 겹침이라 뺀다)
			for (int32 J = I + 1; J < Texts.Num(); ++J)
			{
				const FTxt& B = Texts[J];
				FR O;
				if (!Inter(A.B, B.B, O) || O.W() < 1.5f || O.H() < 1.5f) continue;
				if (A.S == B.S && FMath::Abs(A.B.X0 - B.B.X0) < 5.f && FMath::Abs(A.B.Y0 - B.B.Y0) < 5.f) continue;
				Note(TEXT("TEXT overlaps TEXT"), Q(A.S) + TEXT(" ↔ ") + Q(B.S), A.B, &B.B);
			}
			// 선이 글자 몸통을 3px 넘게 지나감
			const FR In = Inset(A.B, 1.f);
			for (const FSeg& S : Segs)
				if (SegIn(S.A, S.B, In) >= 3.f)
				{
					const FR L{FMath::Min(S.A.X, S.B.X), FMath::Min(S.A.Y, S.B.Y), FMath::Max(S.A.X, S.B.X) + 1.f, FMath::Max(S.A.Y, S.B.Y) + 1.f};
					Note(TEXT("LINE through TEXT"), Q(A.S), A.B, &L);
					break;
				}
			// 글자가 판 · 버튼 테두리에 걸치거나 닿음 (일부만 안에). 테두리 선 두께만큼 판을 1px 넓혀 본다
			for (const FPanel& P : Panels)
			{
				FR O;
				if (Inside(A.G, P.B, 1.f) || !Inter(A.G, Inset(P.B, -1.f), O) || O.W() < 1.f || O.H() < 0.5f) continue;
				Note(P.bBtn ? TEXT("TEXT on BUTTON edge") : TEXT("TEXT on PANEL edge"), Q(A.S), A.B, &P.B);
			}
		}
		// 그림 ↔ 그림: 일부만 겹침 (다 들어간 것 · 같은 그림을 같은 자리에 겹친 것은 뺀다)
		for (int32 I = 0; I < Objs.Num(); ++I)
			for (int32 J = I + 1; J < Objs.Num(); ++J)
			{
				const FObj& A = Objs[I];
				const FObj& B = Objs[J];
				FR O;
				if (!Inter(A.B, B.B, O) || O.W() < 2.f || O.H() < 2.f || Inside(A.B, B.B, 1.f) || Inside(B.B, A.B, 1.f)) continue;
				Note(TEXT("PICTURE overlaps PICTURE"), A.N + TEXT(" <> ") + B.N, A.B, &B.B);
			}
		// 그림이 글자에 걸침: 글자 일부만 그림 상자 안 (그림 속에 다 들어간 글자 = 그림 위 숫자 · 이름이라 괜찮다, 그 그림이 그린 글자도 뺀다)
		for (const FTxt& T : Texts)
			for (int32 I = 0; I < Objs.Num(); ++I)
			{
				if (T.Obj == I) continue;
				FR O;
				if (!Inter(T.B, Objs[I].B, O) || O.W() < 1.5f || O.H() < 1.5f || Inside(T.B, Objs[I].B, 1.f)) continue;
				Note(TEXT("PICTURE on TEXT"), Objs[I].N + TEXT(" <> ") + Q(T.S), Objs[I].B, &T.B);
			}
		// 선이 버튼을 가로지름: 일부는 버튼 안(가장자리 1px 안쪽으로 1.5px 넘게), 일부는 밖 (버튼 속 아이콘 · 테두리는 다 안이라 괜찮다)
		for (const FPanel& P : Panels)
		{
			if (!P.bBtn) continue;
			const FR In = Inset(P.B, 1.f);
			for (const FSeg& S : Segs)
			{
				const FR L{FMath::Min(S.A.X, S.B.X), FMath::Min(S.A.Y, S.B.Y), FMath::Max(S.A.X, S.B.X), FMath::Max(S.A.Y, S.B.Y)};
				if (Inside(L, P.B, 0.5f) || SegIn(S.A, S.B, In) < 1.5f) continue;
				const FR L1{L.X0, L.Y0, L.X1 + 1.f, L.Y1 + 1.f};
				Note(TEXT("LINE across BUTTON"), TEXT("button"), P.B, &L1);
				break;
			}
		}
	}
};
inline FPDLayoutCheck GPDLayout;
// 스프라이트 아틀라스 (D-140): 칸 하나 = 설계 좌표 AtlasCell px, 텍스처엔 AtlasK 배로 굽는다 (화면 확대에도 또렷하게)
// 타워 카드: 카드 53종(2~A × 무늬 4 + 와일드) × 레벨 4 × 족보 듦 2 = 424, 조커 10 → 434칸 (23 × 19줄 = 3344px)
constexpr float AtlasCell = 88.f, AtlasK = 2.f;
constexpr int32 AtlasCols = 23, AtlasTex = 4096; // 23 × 88 × 2 = 4048 ≤ 4096
constexpr int32 AtlasCards = 53, AtlasTowerN = AtlasCards * 4, AtlasJoker0 = AtlasTowerN; // 카드 × 레벨 4 (족보 타워 채움은 그릴 때 덧그림, D-176)
// 이펙트 모양 (D-141, 흰색으로 구워 색·크기·회전을 입혀 찍는다): 반지름 AtlasFxR 로 굽는다
// 0 원, 1~6 다각형 3~8, 7~12 별 3~8, 13~27 끊어진 호 2~16, 28~42 빛줄기 2~16, 43~47 무늬 0~4, 48~52 별빛 갈래 0·2·4·6·8
constexpr int32 AtlasFx0 = AtlasJoker0 + 10, AtlasFxN = 53;
// 그 밖 (D-143): 족보 빗금 칸, 합성 가능 꺾쇠 칸, 가로 빛선 (조각·입자 꼬리)
constexpr int32 AtlasHatch = AtlasFx0 + AtlasFxN, AtlasBracket = AtlasHatch + 1, AtlasLine = AtlasHatch + 2;
// 선 스프라이트 (D-146): 가로 띠 (가운데 꽉, 위아래 가장자리만 부드럽게). 모든 선은 이걸 늘여 찍는다
constexpr int32 AtlasLineAA = AtlasHatch + 3, AtlasHalo = AtlasHatch + 4;
// 카드병정 적 (D-175): 모양 8 × 걷기 4프레임, 흰색으로 구워 그릴 때 적 색으로 물들인다. 칸 가운데가 몸 가운데, 발은 +SoldierFoot
constexpr int32 SoldierShapes = 29, SoldierFrames = 12, AtlasSoldier0 = AtlasHatch + 5; // 일반 7 + 보스 13 (D-176) + 새 보스 6 · 최종 보스 3 (D-335). 카드병정은 둘째 텍스처(쪽 1)에 따로 굽는다 (D-338, 걷기 12프레임 그대로)
constexpr int32 SoldierCells = SoldierShapes * SoldierFrames;
constexpr int32 AtlasSnow = AtlasSoldier0 + SoldierShapes * SoldierFrames; // 둔화 눈송이 (D-178)
constexpr int32 AtlasShield = AtlasSnow + 1; // 방어력 방패 (D-186)
constexpr int32 AtlasMouse = AtlasShield + 1, AtlasMouseR = AtlasShield + 2, AtlasN = AtlasShield + 3; // 마우스 몸통 · 오른쪽 버튼 (D-373, 합성 모드 커서 꼬리표): 따로 물들인다
constexpr float ShieldBakeS = 30.f; // 방패 칸: 가운데 기준 반폭 0.8S, 위아래 ±1.05S
constexpr float MouseBakeH = 60.f; // 마우스 칸: 가운데 기준 높이 MouseBakeH, 폭 0.66배 (미리보기 tools/neon/rclick_hint_mock.py)
// 마우스 몸통: 위아래가 둥근 캡슐 + 가름선 + 가운데 세로선 (선 셋). C = 가운데, H = 높이
inline TArray<TArray<FVector2f>> MouseBodyLines(FVector2f C, float H)
{
	const float W = H * 0.66f, R = W / 2.f, X = C.X - W / 2.f, Y = C.Y - H / 2.f, Split = Y + H * 0.42f;
	TArray<FVector2f> Body;
	for (int32 K = 0; K <= 10; ++K) { const float A = PI + K * PI / 10.f; Body.Add(FVector2f(X + R + FMath::Cos(A) * R, Y + R + FMath::Sin(A) * R * 0.9f)); }
	for (int32 K = 0; K <= 10; ++K) { const float A = K * PI / 10.f; Body.Add(FVector2f(X + R + FMath::Cos(A) * R, Y + H - R + FMath::Sin(A) * R)); }
	{ const FVector2f B0 = Body[0]; Body.Add(B0); }
	return {Body, {FVector2f(X, Split), FVector2f(X + W, Split)}, {FVector2f(X + R, Y + 1.f), FVector2f(X + R, Split)}};
}
// 오른쪽 버튼 윤곽: 몸통 위 오른쪽 1/4
inline TArray<FVector2f> MouseRightPoints(FVector2f C, float H)
{
	const float W = H * 0.66f, R = W / 2.f, X = C.X - W / 2.f, Y = C.Y - H / 2.f, Split = Y + H * 0.42f;
	TArray<FVector2f> P = {FVector2f(X + R, Y)};
	for (int32 K = 0; K <= 8; ++K) { const float A = 1.5f * PI + K * HALF_PI / 8.f; P.Add(FVector2f(X + R + FMath::Cos(A) * R, Y + R + FMath::Sin(A) * R * 0.9f)); }
	P.Add(FVector2f(X + W, Split)); P.Add(FVector2f(X + R, Split)); P.Add(FVector2f(X + R, Y));
	return P;
}
// 방패 윤곽 (D-186, 미리보기 tools/neon 방패 시안): 위는 살짝 솟고, 옆은 곧게 내려가다 아래 끝이 뾰족
inline TArray<FVector2f> ShieldPoints(FVector2f C, float S)
{
	TArray<FVector2f> R = {C + FVector2f(-0.8f, -0.85f) * S, C + FVector2f(-0.4f, -0.95f) * S, C + FVector2f(0.f, -1.05f) * S, C + FVector2f(0.4f, -0.95f) * S, C + FVector2f(0.8f, -0.85f) * S, C + FVector2f(0.8f, -0.1f) * S};
	for (int32 K = 1; K <= 10; ++K) { const float T = K / 10.f; R.Add(C + FVector2f(0.8f * FMath::Pow(1.f - T, 1.3f), -0.1f + 1.15f * FMath::Sin(T * HALF_PI)) * S); }
	for (int32 K = R.Num() - 2; K >= 5; --K) { const FVector2f Q = R[K]; R.Add(FVector2f(2.f * C.X - Q.X, Q.Y)); } // 옮겨 담고 넣는다 (자기 원소를 넣으면 배열이 늘며 깨진다)
	{ const FVector2f Q0 = R[0]; R.Add(Q0); }
	return R;
}
constexpr float SoldierFoot = 37.f, SoldierBakeR = 29.f, SoldierBossBakeR = 23.f;
inline int32 SoldierShape(EPDEnemy T)
{
	switch (T)
	{
	case EPDEnemy::Normal: return 0;
	case EPDEnemy::Fast: return 1;
	case EPDEnemy::Tank: return 2;
	case EPDEnemy::Armored: return 3;
	case EPDEnemy::Splitter: return 4;
	case EPDEnemy::Regen: return 5;
	case EPDEnemy::Mini: return 6;
	// 보스 13종 (D-176, 미리보기 tools/neon/soldiers.py bosses)
	case EPDEnemy::ShieldBoss: return 7;
	case EPDEnemy::SplitBoss: return 8;
	case EPDEnemy::RegenBoss: return 9;
	case EPDEnemy::Charger: return 10;
	case EPDEnemy::Summoner: return 11;
	case EPDEnemy::Phantom: return 12;
	case EPDEnemy::Warlord: return 13;
	case EPDEnemy::Berserker: return 14;
	case EPDEnemy::Blinker: return 15;
	case EPDEnemy::Devourer: return 16;
	case EPDEnemy::Warden: return 17;
	case EPDEnemy::Hexer: return 18;
	case EPDEnemy::Breaker: return 19;
	// 새 보스 · 최종 보스 (D-335, tools/neon/soldiers.py newbosses). 블러퍼 분신은 진짜와 같은 그림
	case EPDEnemy::Bluffer: case EPDEnemy::Decoy: return 20;
	case EPDEnemy::BigBlind: return 21;
	case EPDEnemy::Dealer: return 22;
	case EPDEnemy::Counter: return 23;
	case EPDEnemy::HouseRule: return 24;
	case EPDEnemy::Twins: return 25;
	case EPDEnemy::Bouncer: return 26;
	case EPDEnemy::HighRoller: return 27;
	case EPDEnemy::House: return 28;
	default: return 0;
	}
}
// 띠는 굵기별로 여러 줄: Slate 선 셰이더와 같은 모양 (굵기+2px 너비, 가장자리 smoothstep) 을 굵기마다 구워 둔다
constexpr int32 LineBands = 10;
constexpr float LineBandT[LineBands] = {0.75f, 1.f, 1.5f, 2.f, 3.f, 4.f, 6.f, 9.f, 14.f, 22.f}; // 띠마다 대표 굵기 (화면 px)
constexpr float LineBandH = 8.f;
constexpr float LineAAW = 60.f, LineAAH = LineBandH * LineBands; // 띠 크기 (설계 px)
// 번짐 띠 (D-147): 네온 선의 번짐 두 겹(굵기 6.5배 9% + 3.2배 14%)을 미리 합성한 띠. 굵기별로 한 줄, 알파는 HaloGain 배로 구워 정밀도를 지킨다
constexpr int32 HaloBands = 8;
constexpr float HaloBandT[HaloBands] = {0.5f, 0.75f, 1.f, 1.5f, 2.f, 3.f, 4.f, 6.f}; // 네온 선 굵기 (화면 px)
constexpr float HaloGain = 4.f;
constexpr int32 AtlasVersion = 12; // 12: 마우스 아이콘 칸 둘 (D-373), 8: 카드병정 부드러운 흐느적 12프레임 (D-184), 9: 방어력 방패 칸 (D-186), 10: 새 보스 9종 · 걷기 8프레임 (D-335), 11: 카드병정을 둘째 텍스처 T_PDAtlasSoldier 로, 12프레임 (D-338)
constexpr int32 AtlasVersionPrev = 11; // 5: 카드 칸 흰색 (D-154), 6: 카드병정 적 (D-175), 7: 보스 13종·족보 타워 칸 없앰 (D-176) // 아틀라스 칸이 바뀌면 올린다: 옛 에셋(T_PDAtlas_v1…)은 쓰지 않고 F8 로 새로 만든다
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
// 아틀라스 두 쪽 (D-338): 쪽 0 = 카드 · 조커 · 이펙트 · 선 (T_PDAtlas), 쪽 1 = 카드병정 적 29종 × 12프레임 (T_PDAtlasSoldier). 칸 번호(I)는 그대로 하나로 이어 쓰고 쪽 안 번호로 바꿔 자리를 정한다
inline int32 AtlasPage(int32 I) { return I >= AtlasSoldier0 && I < AtlasSoldier0 + SoldierCells ? 1 : 0; }
inline int32 AtlasLocal(int32 I) { return AtlasPage(I) ? I - AtlasSoldier0 : I < AtlasSoldier0 ? I : I - SoldierCells; }
constexpr int32 AtlasN0 = AtlasN - SoldierCells, AtlasN1 = SoldierCells; // 쪽마다 칸 수
static_assert((AtlasN0 + AtlasCols - 1) / AtlasCols * AtlasCell * AtlasK <= AtlasTex, "atlas page 0 rows overflow the texture");
static_assert((AtlasN1 + AtlasCols - 1) / AtlasCols * AtlasCell * AtlasK <= AtlasTex, "atlas page 1 (soldiers) rows overflow the texture");
inline int32 AtlasCardIndex(const FPDCard& C) { return C.IsWild() ? 52 : (FMath::Clamp(C.Rank, 2, 14) - 2) * 4 + FMath::Clamp((int)C.Suit, 0, 3); }
inline int32 TowerSprite(const FPDCard& C, int Level, bool bStrong)
{
	return AtlasCardIndex(C) * 4 + (FMath::Clamp(Level, 1, 4) - 1); // bStrong 은 구운 칸이 아니라 그릴 때 채움 (D-176)
}
inline FVector2f AtlasCellCenter(int32 I) { const int32 L = AtlasLocal(I); return FVector2f((L % AtlasCols + 0.5f) * AtlasCell, (L / AtlasCols + 0.5f) * AtlasCell); } // 그 칸의 쪽 안 자리 (D-338)
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
	UObject* RT2 = nullptr, *Tex2 = nullptr; // 둘째 쪽: 카드병정 (D-338)
	TArray<FColor> LastPixels2;
	int32 BakePage = 0;                    // 지금 굽는 쪽 (PaintAtlas 가 그 쪽 칸만 그린다)
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
	bool bRec = false;  // 레이아웃 검사 (D-365): 이 판에 그리는 글자 · 상자 · 선의 자리를 GPDLayout 에 모은다 (화면 판 C · F 만, 굽기 판은 아님)
	bool Rec() const { return bRec && GPDLayout.bCap && !PW && !Warp.bOn; }
	bool bObjRec = false; // 그림 덩어리도 모은다 (고정 패널 · 창 판 C 만, D-366)
	struct FObjScope // 그림 함수 하나 = 그림 덩어리 하나 (안에서 부른 그림 함수는 바깥 덩어리에 합쳐진다)
	{
		bool bOn;
		FObjScope(const FCanvas& C, const TCHAR* N) : bOn(C.bObjRec && C.Rec()) { if (bOn) GPDLayout.BeginObj(N); }
		~FObjScope() { if (bOn) GPDLayout.EndObj(); }
	};
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
	// 휘기 (D-184): 카드병정을 구울 때 몸 전체를 휜다. 위로 갈수록 Bend 만큼 옆으로 밀고(아래 끝은 그대로), 엉덩이를 축으로 Ang 만큼 돌린다
	struct FWarp { bool bOn = false; FVector2f Hip = FVector2f(0.f, 0.f); float Top = 0.f, BH = 1.f, Bend = 0.f, Ang = 0.f; };
	FWarp Warp;
	// 쪽 넘김 휘기 (D-354): PW 가 있으면 모든 점을 PW 로 옮긴다. 선은 잘게 나눠 휘고, 상자 · 글자 · 스프라이트는 그 자리의 기울기(야코비안)로 비틀어 찍는다.
	// FadeK: 그리는 모든 것의 알파 배율 (넘어가는 장 면 밝기)
	TFunction<FVector2f(FVector2f)> PW;
	float FadeK = 1.f;
	FLinearColor FadeC(const FLinearColor& C) const { return FadeK >= 1.f ? C : FLinearColor(C.R, C.G, C.B, C.A * FadeK); }
	TArray<FVector2f> PWPts(const TArray<FVector2f>& P) const
	{
		TArray<FVector2f> Q;
		for (int32 I = 0; I < P.Num(); ++I)
		{
			if (I > 0)
			{
				const int32 N = FMath::Clamp((int32)(FVector2f::Distance(P[I - 1], P[I]) / 6.f), 1, 64);
				for (int32 K = 1; K < N; ++K) Q.Add(PW(FMath::Lerp(P[I - 1], P[I], K / (float)N)));
			}
			Q.Add(PW(P[I]));
		}
		return Q;
	}
	// 왼쪽 위 (X, Y), 크기 W × H 의 그리기 자리. PW 가 있으면 가운데의 기울기로 비튼 자리 (작은 상자 · 글자는 이 근사로 충분)
	FPaintGeometry PG(float X, float Y, float W, float H) const
	{
		if (!PW) return Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y)));
		const FVector2f C(X + W / 2.f, Y + H / 2.f), Wc = PW(C);
		constexpr float E = 2.f;
		const FVector2f Jx = (PW(C + FVector2f(E, 0.f)) - PW(C - FVector2f(E, 0.f))) / (2.f * E), Jy = (PW(C + FVector2f(0.f, E)) - PW(C - FVector2f(0.f, E))) / (2.f * E);
		const FVector2f TL = Wc - Jx * (W / 2.f) - Jy * (H / 2.f);
		return Geo.MakeChild(FVector2f(W, H), FSlateLayoutTransform(TL), FSlateRenderTransform(FMatrix2x2f(Jx.X, Jx.Y, Jy.X, Jy.Y), FVector2f(0.f, 0.f)), FVector2f(0.f, 0.f)).ToPaintGeometry();
	}
	FVector2f WarpP(FVector2f Q) const
	{
		const float V = FMath::Clamp((Q.Y - Warp.Top) / FMath::Max(1.f, Warp.BH), 0.f, 1.f);
		Q.X += Warp.Bend * (1.f - V) * (1.f - V);
		return Warp.Hip + Rot2(Q - Warp.Hip, Warp.Ang);
	}
	void Lines(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick)
	{
		if (PW) // 쪽 넘김 휘기 (D-354)
		{
			const TArray<FVector2f> Q = PWPts(P);
			TFunction<FVector2f(FVector2f)> Keep = MoveTemp(PW);
			PW = nullptr;
			Lines(Q, Col, Thick);
			PW = MoveTemp(Keep);
			return;
		}
		if (Warp.bOn) // 곧은 선도 휘어 보이게 잘게 나눠서 옮긴다
		{
			TArray<FVector2f> Q;
			for (int32 I = 0; I < P.Num(); ++I)
			{
				if (I > 0)
				{
					const int32 N = FMath::Clamp((int32)(FVector2f::Distance(P[I - 1], P[I]) / 2.5f), 1, 64);
					for (int32 K = 1; K < N; ++K) Q.Add(WarpP(FMath::Lerp(P[I - 1], P[I], K / (float)N)));
				}
				Q.Add(WarpP(P[I]));
			}
			Warp.bOn = false;
			Lines(Q, Col, Thick);
			Warp.bOn = true;
			return;
		}
		if (Rec()) GPDLayout.AddLines(P, FadeC(Col), Thick);
		if (LineAA) // 선 스프라이트 (D-146): 마디마다 띠 하나를 늘이고 돌려 찍는다. 같은 층이라 한 번에 그려진다
		{
			// Slate 선은 굵기 + 2px (양쪽 1px 거르기) 너비로 그리고 셰이더가 가장자리를 깎는다. 같은 너비로 찍고, 굵기에 가장 가까운 띠를 쓴다
			// (얇은 사각형을 그대로 찍으면 래스터에서 덮임이 반쯤 날아가 가늘고 흐려진다)
			const float Tpx = FMath::Max(0.75f, Thick * ThickK);
			int32 Bi = 0;
			while (Bi + 1 < LineBands && Tpx > FMath::Sqrt(LineBandT[Bi] * LineBandT[Bi + 1])) ++Bi;
			Stamp(P, PM(FadeC(Col)), LineAA + Bi, (Tpx + 2.f) * PxToDesign); // 화면 px → 설계 좌표 (DPI 포함 화면 배율로 나눔)
			++GPDDebug.Lines; GPDDebug.Points += P.Num();
			return;
		}
		FSlateDrawElement::MakeLines(Out, *Layer, Geo.ToPaintGeometry(), P, ESlateDrawEffect::None, FadeC(Col), true, Thick * ThickK);
		++GPDDebug.Lines; GPDDebug.Points += P.Num();
	}
	// 띠 찍기 (D-146/147 → D-295): 꺾은선 하나를 이어진 띠 한 줄로 (꼭짓점마다 두 마디가 같은 점을 나눠 쓰는 맞춤 이음).
	// 예전처럼 마디마다 따로 찍으면 모서리에 틈이 나고, 곡선은 번짐 띠가 마디마다 겹쳐 구슬처럼 끊겨 보였다.
	// 띠 끝(U 끝)은 꺾은선의 처음과 끝에만 쓰고 가운데는 U 가운데. 닫힌 선(처음 = 끝)은 끝도 이어 붙인다. 72° 넘게 꺾이면 거기서 끊고 같은 점에서 새로 시작
	FSlateRenderTransform RT;
	void Stamp(const TArray<FVector2f>& P0, const FLinearColor& T, const FSlateBrush* Band, float Th)
	{
		// 겹친 점 빼기
		TArray<FVector2f, TInlineAllocator<64>> P;
		for (const FVector2f& V : P0) if (P.Num() == 0 || FVector2f::DistSquared(P.Last(), V) > 1e-4f) P.Add(V);
		const int32 N = P.Num();
		if (N < 2) return;
		const FColor Col32 = T.ToFColor(true); // Slate 꼭짓점 색과 같게 sRGB
		const float H = Th * 0.5f;
		const bool bClosed = N > 2 && FVector2f::DistSquared(P[0], P[N - 1]) < 1e-3f;
		auto Dir = [&](int32 K) { const FVector2f D = P[K + 1] - P[K]; return D / FMath::Max(1e-4f, D.Size()); }; // 마디 K (P[K] → P[K+1])
		auto Nrm = [&](int32 K) { const FVector2f D = Dir(K); return FVector2f(-D.Y, D.X); };
		if (QV.Num() && (QLayer != *Layer || QV.Num() > 60000)) FlushQuads();
		if (QV.Num() == 0) { QLayer = *Layer; QBrush = Band; RT = Geo.GetAccumulatedRenderTransform(); }
		const FBox2f UV = Band->GetUVRegion();
		const float UMid = 0.5f * (UV.Min.X + UV.Max.X);
		int32 Start = 0;
		while (Start < N - 1)
		{
			// 이번 띠의 끝: 72° 넘게 꺾이는 곳 앞까지 (D-298: 120° 까지 맞춤 이음을 하니 하트 끝 같은 곳에서 모서리 점이 멀리 튀어 빛이 번진 듯 보였다)
			int32 End = Start + 1;
			while (End < N - 1 && FVector2f::DotProduct(Dir(End - 1), Dir(End)) > 0.3f) ++End;
			const bool bLoop = bClosed && Start == 0 && End == N - 1;
			const bool bCapStart = Start == 0 && !bClosed, bCapEnd = End == N - 1 && !bClosed; // 끝이 흐려지는 띠 끝은 진짜 꺾은선 끝에만
			SlateIndex Prev0 = 0, Prev1 = 0;
			for (int32 I = Start; I <= End; ++I)
			{
				FVector2f Off, At = P[I];
				const bool bFirst = I == Start, bLast = I == End;
				int32 KIn = I - 1, KOut = I;
				if (bLoop && bFirst) KIn = N - 2;
				if (bLoop && bLast) KOut = 0;
				if (bFirst && !bLoop) Off = Nrm(KOut) * H; // 꺾여 끊긴 곳은 끝을 흐리지 않고 그 점에서 바로 맞붙인다 (D-299: 반 굵기씩 내밀었더니 날카로운 곳에서 X 자로 엇갈렸다)
				else if (bLast && !bLoop) Off = Nrm(KIn) * H;
				else
				{
					const FVector2f NI = Nrm(KIn), NO = Nrm(KOut);
					FVector2f M = NI + NO;
					const float ML = M.Size();
					if (ML < 1e-3f) Off = NO * H;
					else { M /= ML; Off = M * FMath::Min(H / FMath::Max(0.5f, FVector2f::DotProduct(M, NO)), H * 1.6f); }
				}
				// 꺾이는 안쪽은 굽은 반지름보다 넓게 내밀지 않는다 (D-301): 넓은 번짐 띠가 급한 곡선(a 꼭대기·l 밑)에서 안쪽 가장자리가 뒤집혀 접히며
				// 반대편 밖으로 빛이 별처럼 삐져나갔다. 줄인 쪽은 V 도 같은 비율로 → 번짐 모양은 그대로 잘린다
				float FA = 1.f, FB = 1.f;
				if (I > 0 && I < N - 1)
				{
					const FVector2f DI = Dir(I - 1), DO = Dir(I);
					const float Turn = FMath::Acos(FMath::Clamp(FVector2f::DotProduct(DI, DO), -1.f, 1.f));
					if (Turn > 1e-4f)
					{
						const float Rad = 0.5f * (FVector2f::Distance(P[I], P[I - 1]) + FVector2f::Distance(P[I + 1], P[I])) / Turn;
						const float F = FMath::Min(1.f, 0.8f * Rad / H);
						if (FVector2f::DotProduct(Off, DO - DI) > 0.f) FB = F; else FA = F;
					}
				}
				const float VMid = 0.5f * (UV.Min.Y + UV.Max.Y);
				const float U = UMid; // 띠 끝은 반원 마개가 닫는다 (D-301)
				const SlateIndex B = (SlateIndex)QV.Num();
				QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, At - Off * FA, FVector2f(U, FMath::Lerp(VMid, UV.Min.Y, FA)), Col32));
				QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, At + Off * FB, FVector2f(U, FMath::Lerp(VMid, UV.Max.Y, FB)), Col32));
				if (!bFirst) { QI.Add(Prev0); QI.Add(B); QI.Add(B + 1); QI.Add(Prev0); QI.Add(B + 1); QI.Add(Prev1); ++GPDDebug.Boxes; }
				Prev0 = B; Prev1 = B + 1;
			}
			// 날카롭게 꺾여 끊긴 곳은 바깥쪽에 둥근 이음 부채를 채운다 (D-299): 띠 가운데(V 가운데) → 가장자리(V 끝)로 번지게
			auto Join = [&](const FVector2f& Pt, const FVector2f& DIn, const FVector2f& DOut)
			{
				const float Cr = DIn.X * DOut.Y - DIn.Y * DOut.X;
				const float Side = Cr > 0.f ? -1.f : 1.f; // 왼쪽으로 꺾이면 바깥은 오른쪽
				const FVector2f NA = FVector2f(-DIn.Y, DIn.X) * Side, NB = FVector2f(-DOut.Y, DOut.X) * Side;
				float A0 = FMath::Atan2(NA.Y, NA.X), A1 = FMath::Atan2(NB.Y, NB.X), Da = A1 - A0;
				while (Da > PI) Da -= 2.f * PI;
				while (Da < -PI) Da += 2.f * PI;
				const float VE = Side > 0.f ? UV.Max.Y : UV.Min.Y, VM = 0.5f * (UV.Min.Y + UV.Max.Y);
				const SlateIndex Cn = (SlateIndex)QV.Num();
				QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, Pt, FVector2f(UMid, VM), Col32));
				const int32 Steps = 5;
				for (int32 S = 0; S <= Steps; ++S)
				{
					const float A = A0 + Da * S / Steps;
					QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, Pt + FVector2f(FMath::Cos(A), FMath::Sin(A)) * H, FVector2f(UMid, VE), Col32));
					if (S > 0) { QI.Add(Cn); QI.Add(Cn + S); QI.Add(Cn + S + 1); }
				}
			};
			if (End < N - 1) Join(P[End], Dir(End - 1), Dir(End));
			else if (bClosed && !bLoop) Join(P[N - 1], Dir(N - 2), Dir(0));
			// 진짜 끝은 반원으로 둥글게 닫는다 (D-301): 띠가 끝점에서 평평하게 잘려 넓은 번짐이 어두운 네모 자국처럼 보였다
			auto Cap = [&](const FVector2f& Pt, const FVector2f& OutDir)
			{
				const FVector2f Nn(-OutDir.Y, OutDir.X);
				const float A0 = FMath::Atan2(Nn.Y, Nn.X);
				const SlateIndex Cn = (SlateIndex)QV.Num();
				QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, Pt, FVector2f(UMid, 0.5f * (UV.Min.Y + UV.Max.Y)), Col32));
				const int32 Steps = 8;
				for (int32 S = 0; S <= Steps; ++S)
				{
					const float A = A0 - PI * S / Steps; // 옆 → 바깥 → 반대 옆
					QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, Pt + FVector2f(FMath::Cos(A), FMath::Sin(A)) * H, FVector2f(UMid, UV.Max.Y), Col32));
					if (S > 0) { QI.Add(Cn); QI.Add(Cn + S); QI.Add(Cn + S + 1); }
				}
			};
			if (bCapStart) Cap(P[0], -Dir(0));
			if (bCapEnd) Cap(P[N - 1], Dir(N - 2));
			Start = End;
		}
	}
	// 네온 선 (docs/15-art-style.md): 아주 넓고 옅은 번짐 세 겹 + 가늘고 또렷한 심
	// 웹 캔버스의 shadowBlur 처럼 부드럽게 퍼지는 빛을 흉내 낸다
	// 둥근 광원 (D-304): 가운데서 바깥으로 매끈하게 옅어지는 타원. 꼭짓점 색으로 옅어짐을 주고 글자는 선 띠의 꽉 찬 가운데 한 점만 쓴다
	// (예전엔 번짐 띠 무늬를 돌려 찍어서 두 겹 띠의 경계가 원판 테두리처럼 보였다). 옅어짐 = (1 - t)², 고리 8겹
	void SoftDisc(const FVector2f& O, float Rx, float Ry, const FLinearColor& Col, float Peak)
	{
		if (!LineAA || Peak <= 0.f || Rx <= 0.f || Ry <= 0.f) return;
		const FSlateBrush* Band = LineAA + LineBands - 1;
		if (QV.Num() && (QLayer != *Layer || QV.Num() > 60000)) FlushQuads();
		if (QV.Num() == 0) { QLayer = *Layer; QBrush = Band; RT = Geo.GetAccumulatedRenderTransform(); }
		const FBox2f UV = Band->GetUVRegion();
		const FVector2f UVc(0.5f * (UV.Min.X + UV.Max.X), 0.5f * (UV.Min.Y + UV.Max.Y));
		constexpr int32 DiscRings = 8, Segs = 32;
		const SlateIndex C0 = (SlateIndex)QV.Num();
		QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, O, UVc, PM(WithAlpha(Col, FMath::Min(1.f, Peak))).ToFColor(true)));
		for (int32 R = 1; R <= DiscRings; ++R)
		{
			const float T = R / (float)DiscRings;
			const FColor Cr = PM(WithAlpha(Col, FMath::Min(1.f, Peak) * FMath::Square(1.f - T))).ToFColor(true);
			for (int32 S = 0; S < Segs; ++S)
			{
				const float A = TwoPi * S / Segs;
				QV.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(RT, O + FVector2f(FMath::Cos(A) * Rx, FMath::Sin(A) * Ry) * T, UVc, Cr));
			}
		}
		for (int32 S = 0; S < Segs; ++S) { const int32 S1 = (S + 1) % Segs; QI.Add(C0); QI.Add(C0 + 1 + S); QI.Add(C0 + 1 + S1); }
		for (int32 R = 1; R < DiscRings; ++R)
		{
			const SlateIndex In = C0 + 1 + (R - 1) * Segs, Ou = C0 + 1 + R * Segs;
			for (int32 S = 0; S < Segs; ++S)
			{
				const int32 S1 = (S + 1) % Segs;
				QI.Add(In + S); QI.Add(Ou + S); QI.Add(Ou + S1);
				QI.Add(In + S); QI.Add(Ou + S1); QI.Add(In + S1);
			}
		}
	}
	// 벽에 비치는 은은한 빛 (D-302): 선을 따라 둥근 광원을 반지름 절반 간격으로 촘촘히 찍는다 (넓은 띠를 꺾은선에 늘이면 각진 모서리에서 별처럼 뻗었다)
	// Width = 빛 너비(설계 좌표), Peak = 선 위 밝기. 겹침 합 ≈ 4/3 장이라 한 장은 Peak 의 0.75배
	void Spill(const TArray<FVector2f>& P, const FLinearColor& Col, float Width, float Peak)
	{
		if (!LineAA || Peak <= 0.f || P.Num() < 2) return;
		const float R = Width * 0.5f, Step = R * 0.5f, A1 = Peak * 0.75f;
		float Carry = 0.f; // 다음 점까지 남은 길이 (선이 꺾여도 간격이 고르게)
		SoftDisc(P[0], R, R, Col, A1);
		for (int32 K = 0; K + 1 < P.Num(); ++K)
		{
			const FVector2f A = P[K], D = P[K + 1] - P[K];
			const float L = D.Size();
			float T = Step - Carry;
			while (T <= L) { SoftDisc(A + D * (T / FMath::Max(L, 1e-4f)), R, R, Col, A1); T += Step; }
			Carry = L - (T - Step);
		}
	}
	void Glow(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Strength = 1.f)
	{
		if (PW) // 쪽 넘김 휘기 (D-354): 번짐 띠도 같이 휜다
		{
			const TArray<FVector2f> Q = PWPts(P);
			TFunction<FVector2f(FVector2f)> Keep = MoveTemp(PW);
			PW = nullptr;
			Glow(Q, Col, Thick, Strength);
			PW = MoveTemp(Keep);
			return;
		}
		if (Strength > 0.f && HaloK > 0.f && LineAA) // 번짐 두 겹을 합성한 띠 한 장 (D-147). 띠는 선 띠 다음에 이어 둔다
		{
			const float Tpx = FMath::Max(0.25f, Thick * ThickK);
			int32 Bi = 0;
			while (Bi + 1 < HaloBands && Tpx > FMath::Sqrt(HaloBandT[Bi] * HaloBandT[Bi + 1])) ++Bi;
			Stamp(P, PM(WithAlpha(Col, FMath::Min(1.f, Strength / HaloGain) * FadeK)), LineAA + LineBands + Bi, (Tpx * 6.5f + 2.f) * PxToDesign);
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
		if (Rec()) GPDLayout.AddPanel(X, Y, W, H, Col.A * FadeK, false);
		BoxAt(NextLayer(), X, Y, W, H, Col);
		NextLayer();
	}
	// 둥근 사각 안을 채운다 (D-328 ②: 눌리는 버튼은 안을 아주 옅게). 2px 띠를 둥근 모서리 모양대로 줄여 한 층에 쌓는다
	void RoundFill(float X, float Y, float W, float H, float R, const FLinearColor& Col)
	{
		R = FMath::Min(R, FMath::Min(W, H) / 2.f);
		const int32 L = NextLayer();
		for (float Yy = 0.f; Yy < H; Yy += 2.f)
		{
			const float Mid = Yy + 1.f, Dy = Mid < R ? R - Mid : Mid > H - R ? Mid - (H - R) : 0.f;
			const float In = Dy > 0.f ? R - FMath::Sqrt(FMath::Max(0.f, R * R - Dy * Dy)) : 0.f;
			BoxAt(L, X + In, Y + Yy, W - 2.f * In, FMath::Min(2.f, H - Yy), Col);
		}
		NextLayer();
	}
	// 눌리는 버튼 바탕 (D-328 ②): 평소 옅게, 마우스를 올리면 더 진하게
	// 닫기 ⓧ (D-384, D-388, D-391): 동그라미 안 X, 흰색 네온. 마우스 올림 = 더 밝게 + 빛 판. 설정 · 스킬 트리 · 도감 · 강화 창이 같이 쓴다
	void CloseX(FVector2f O, float R, bool bHov)
	{
		const FLinearColor W = Hex(TEXT("eef3ff"));
		const FLinearColor Xc = bHov ? FLinearColor::White : WithAlpha(W, 0.72f);
		if (bHov) SoftDisc(O, R + 6.f, R + 6.f, W, 0.16f);
		Glow(CirclePoints(O, R, 28), Xc, bHov ? 1.8f : 1.3f, bHov ? 0.9f : 0.45f);
		const float K = R * 0.42f;
		Glow({O + FVector2f(-K, -K), O + FVector2f(K, K)}, Xc, 1.6f, bHov ? 0.8f : 0.4f);
		Glow({O + FVector2f(K, -K), O + FVector2f(-K, K)}, Xc, 1.6f, bHov ? 0.8f : 0.4f);
	}
	void BtnFill(float X, float Y, float W, float H, float R, const FLinearColor& Col, bool bHot)
	{
		if (Rec()) GPDLayout.AddPanel(X, Y, W, H, 1.f, true);
		// 마우스 올림 (D-381): 같은 색으로 밝아지고, 버튼 모양 그대로 밑에 은은한 빛 판 (다른 색으로 바꾸지 않는다)
		if (bHot) Spill(RoundRectPoints(X, Y, W, H, FMath::Min(R, FMath::Min(W, H) / 2.f)), Col, FMath::Min(H, 28.f), 0.16f);
		RoundFill(X, Y, W, H, R, FLinearColor(Col.R, Col.G, Col.B, bHot ? 0.16f : 0.07f));
	}
	// 같은 층에 여러 개를 그리면 한 번에 묶여 빠르다 (파티클)
	void BoxAt(int32 L, float X, float Y, float W, float H, const FLinearColor& Col)
	{
		FSlateDrawElement::MakeBox(Out, L, PG(X, Y, W, H), White, ESlateDrawEffect::None, FadeC(Col));
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
			for (int32 S = 0; S <= 8; ++S) // 모서리마다 8마디 (D-295: 5마디는 곡선이 각져 보였다)
			{
				const float A = (Start[K] + S / 8.f * 0.25f) * TwoPi;
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
		if (Rec()) GPDLayout.AddText(S, Left, Top, Sz.X, Sz.Y, S.EndsWith(TEXT("…")) && !InS.EndsWith(TEXT("…")), Col.A * FadeK);
		FSlateDrawElement::MakeText(Out, NextLayer(), PG(Left, Top, Sz.X, Sz.Y), S, F, ESlateDrawEffect::None, FadeC(Col));
		NextLayer();
	}
	// 돌리고 키운 글자 (D-385 회사 로고): 가운데 (CX, CY), Angle 라디안 (화면 시계 방향), Scale 배. 언어팩을 거치지 않는다 (로고 글자)
	void TextRot(const FString& S, float CX, float CY, const FLinearColor& Col, float Px, bool bBold, float Angle, float Scale = 1.f)
	{
		const FSlateFontInfo F = Font(Px, bBold);
		const FVector2f Sz = Measure(S, F);
		const float Co = FMath::Cos(Angle) * Scale, Si = FMath::Sin(Angle) * Scale;
		const FGeometry G = Geo.MakeChild(Sz, FSlateLayoutTransform(FVector2f(CX - Sz.X / 2.f, CY - Sz.Y / 2.f)), FSlateRenderTransform(FMatrix2x2f(Co, Si, -Si, Co), FVector2f(0.f, 0.f)), FVector2f(0.5f, 0.5f));
		FSlateDrawElement::MakeText(Out, NextLayer(), G.ToPaintGeometry(), S, F, ESlateDrawEffect::None, FadeC(Col));
		NextLayer();
	}
	// 채운 다각형 (가로 띠로): 가로로 자르면 한 구간이 되는 모양만 (타원 · 네 갈래 별)
	void FillPoly(const TArray<FVector2f>& P, const FLinearColor& Col, float Step = 0.5f)
	{
		if (P.Num() < 3) return;
		float Y0 = P[0].Y, Y1 = P[0].Y;
		for (const FVector2f& Q : P) { Y0 = FMath::Min(Y0, Q.Y); Y1 = FMath::Max(Y1, Q.Y); }
		const int32 L = NextLayer();
		for (float Y = Y0; Y < Y1; Y += Step)
		{
			const float Ym = Y + Step * 0.5f;
			float X0 = 1e9f, X1 = -1e9f;
			for (int32 I = 0; I < P.Num(); ++I)
			{
				const FVector2f A = P[I], B = P[(I + 1) % P.Num()];
				if ((A.Y > Ym) == (B.Y > Ym)) continue;
				const float X = A.X + (Ym - A.Y) * (B.X - A.X) / (B.Y - A.Y);
				X0 = FMath::Min(X0, X); X1 = FMath::Max(X1, X);
			}
			if (X1 > X0) BoxAt(L, X0, Y, X1 - X0, Step, Col);
		}
		NextLayer();
	}
	void TextCentered(const FString& InS, float CX, float CY, const FLinearColor& Col, float Px, bool bBold, float MaxW = 0.f)
	{
		FString S = TrS(InS);
		FSlateFontInfo F = Font(Px, bBold);
		FVector2f Sz = Measure(S, F);
		FitText(S, F, Sz, Px, bBold, MaxW);
		if (Rec()) GPDLayout.AddText(S, CX - Sz.X / 2.f, CY - Sz.Y / 2.f, Sz.X, Sz.Y, S.EndsWith(TEXT("…")) && !InS.EndsWith(TEXT("…")), Col.A * FadeK);
		FSlateDrawElement::MakeText(Out, NextLayer(), PG(CX - Sz.X / 2.f, CY - Sz.Y / 2.f, Sz.X, Sz.Y), S, F,
			ESlateDrawEffect::None, FadeC(Col));
		NextLayer();
	}
	// 사각 클리핑: PushClip ~ PopClip 사이에 그린 것은 이 사각 밖으로 나가지 않는다 (흐르는 글자 등)
	void PushClip(float X, float Y, float W, float H) { if (Rec()) GPDLayout.Clip.Add({X, Y, X + W, Y + H}); Out.PushClip(FSlateClippingZone(Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))))); }
	void PopClip() { if (Rec() && GPDLayout.Clip.Num()) GPDLayout.Clip.Pop(); Out.PopClip(); }
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
		const FObjScope Ob(*this, TEXT("Shape"));
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
	// ---- 카드병정 적 (D-175, 미리보기 tools/neon/soldiers.py): 세로로 긴 카드 몸통에 눈코입, 짧은 팔다리, 종류마다 무기.
	// Shape: 0 보통(♠ 창) 1 빠름(♦ 속도선) 2 탱크(♣ 도끼) 3 방패병(♥ 방패) 4 분열체(♦ 찢어진 금) 5 재생자(♣ 떠오르는 +) 6 조각(♥ 작게) 7 보스(♠ 왕관·홀·망토)
	// Foot = 발바닥, R = 크기, T = 걷기 위상 0..1. 흰색으로 아틀라스에 구워 두고 그릴 때 적 색으로 물들인다
	void CardSoldier(int32 Kd, FVector2f Foot, float R, float T, const FLinearColor& Col)
	{
		static const float WK[7] = {1.f, 0.85f, 1.2f, 1.f, 1.f, 1.f, 0.6f}, HK[7] = {1.f, 0.95f, 1.15f, 1.f, 1.f, 1.f, 0.6f};
		static const EPDSuit SuitOf[29] = {EPDSuit::Spade, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Heart, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Heart,
			EPDSuit::Heart, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Spade, EPDSuit::Spade, EPDSuit::Heart, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Heart, EPDSuit::Spade, EPDSuit::Club,
			EPDSuit::Spade, EPDSuit::Diamond, EPDSuit::Club, EPDSuit::Spade, EPDSuit::Heart, EPDSuit::Heart, EPDSuit::Club, EPDSuit::Diamond, EPDSuit::Spade}; // 20~28: 블러퍼 · 빅 블라인드 · 딜러 · 카드 카운터 · 하우스 룰 · 쌍둥이 · 바운서 · 하이롤러 · 더 하우스
		const bool bBossK = Kd >= 7;
		const float wk = bBossK ? 1.35f : WK[Kd], hk = bBossK ? 1.35f : HK[Kd];
		// 보스 모양 (7~19): 방패 거인, 분열 군주, 재생 왕, 돌진 기사, 소환사, 유령, 군단장, 분노 거인, 공간 도약자, 흡수자, 수호자, 저주술사, 파괴자
		const bool bGhost = Kd == 12, bHex = Kd == 18, bDevour = Kd == 16, bGrit = Kd == 1 || Kd == 10 || Kd == 14 || Kd == 19 || Kd == 26, bSmile = Kd == 5 || Kd == 9 || Kd == 20 || Kd == 27;
		const bool bCrown = Kd == 9 || Kd == 13 || Kd == 28;
		const float Ph = T * TwoPi, Sw = FMath::Sin(Ph), Bob = FMath::Abs(FMath::Sin(Ph)) * 0.03f * R; // 부드럽게 (D-184)
		const float Lean = Kd == 1 ? FMath::DegreesToRadians(8.f) : Kd == 10 ? FMath::DegreesToRadians(12.f) : 0.f;
		const float BW = 0.76f * R * wk, BH = 1.18f * R * hk, LegL = 0.32f * R * hk;
		const FVector2f Hip(Foot.X, Foot.Y - LegL - Bob);
		// 부드러운 흐느적 (D-184): 카드 윗부분이 걸음보다 늦게 살짝 휘청 + 몸 전체 아주 살짝 흔들림. 기울기도 함께 휘기에서 처리
		struct FWarpScope { FWarp& W; ~FWarpScope() { W.bOn = false; } } WarpScope{Warp};
		Warp.bOn = true; Warp.Hip = Hip; Warp.Top = Hip.Y - BH; Warp.BH = BH;
		Warp.Bend = 0.12f * R * FMath::Sin(Ph - 1.2f);
		Warp.Ang = Lean + FMath::DegreesToRadians(2.f) * FMath::Sin(Ph - 0.6f);
		auto Rt = [&](FVector2f P) { return P; };
		auto RtA = [&](TArray<FVector2f> A) { return A; };
		const float Th = Kd == 2 ? 2.2f : 1.6f;
		for (const float S : {1.f, -1.f}) // 다리: 번갈아 앞뒤로 (유령은 다리 없이 떠다닌다)
		{
			if (bGhost) break;
			const float A = FMath::DegreesToRadians(26.f) * Sw * S;
			const FVector2f F2(Hip.X + FMath::Sin(A) * LegL, Hip.Y + FMath::Cos(A) * LegL);
			Glow({FVector2f(Hip.X - S * 0.12f * BW, Hip.Y), F2, F2 + FVector2f(0.12f * R, 0.f)}, Col, 1.4f, 0.6f);
		}
		const float Top = Hip.Y - BH;
		const FVector2f BC(Foot.X, Top + BH / 2.f);
		Glow(RtA(RoundRectPoints(BC.X - BW / 2.f, Top, BW, BH, 0.12f * R)), Col, Th, 0.8f); // 몸통 카드
		if (bGhost) { TArray<FVector2f> Wv; for (int32 K = 0; K <= 8; ++K) Wv.Add(FVector2f(Foot.X - BW / 2.f + BW * K / 8.f, Hip.Y + (K % 2 ? 0.08f * R : 0.f) + 0.04f * R * FMath::Sin(Ph + K))); Lines(Wv, Col, 1.2f); } // 너울 꼬리
		if (Kd == 4 || Kd == 8) { TArray<FVector2f> Z; for (int32 K = 0; K <= 6; ++K) Z.Add(FVector2f(BC.X + (K % 2 ? 0.08f : -0.08f) * BW, Top + BH * K / 6.f)); Lines(RtA(Z), Col, 1.1f); }
		const float Pr = 0.085f * R * FMath::Min(wk, hk); // 귀퉁이 무늬
		Shape(SuitOf[Kd], Rt(BC + FVector2f(-BW / 2.f + 0.12f * R * wk, -BH / 2.f + 0.14f * R * hk)).X, Rt(BC + FVector2f(-BW / 2.f + 0.12f * R * wk, -BH / 2.f + 0.14f * R * hk)).Y, Pr, Col, 1.f, 0.4f);
		Shape(SuitOf[Kd], Rt(BC + FVector2f(BW / 2.f - 0.12f * R * wk, BH / 2.f - 0.14f * R * hk)).X, Rt(BC + FVector2f(BW / 2.f - 0.12f * R * wk, BH / 2.f - 0.14f * R * hk)).Y, Pr, Col, 1.f, 0.4f);
		const FVector2f FC(BC.X, BC.Y - 0.04f * R); // 얼굴
		const float Ex = 0.16f * R * FMath::Min(wk, 1.1f), Ey = 0.1f * R, Er = 0.09f * R;
		if (bHex) { Lines(RtA(CirclePoints(FC + FVector2f(0.f, -Ey), Er * 1.6f, 16)), Col, 1.4f); Lines(RtA(CirclePoints(FC + FVector2f(0.f, -Ey), Er * 0.6f, 10)), Col, 1.2f); } // 외눈
		else for (const float S : {-1.f, 1.f})
		{
			const FVector2f EC(FC.X + S * Ex, FC.Y - Ey);
			if (bGhost) { Lines(RtA(CirclePoints(EC + FVector2f(0.f, 0.02f * R), Er * 1.2f, 12)), Col, 1.3f); continue; } // 텅 빈 눈
			Lines(RtA(CirclePoints(EC, Er, 12)), Col, 1.3f);
			Lines(RtA(CirclePoints(EC + FVector2f(0.02f * R, 0.01f * R), Er * 0.35f, 8)), Col, 1.1f);
			if (Kd == 2 || (bBossK && Kd != 9)) Lines({Rt(EC + FVector2f(-S * 0.09f * R, -0.12f * R)), Rt(EC + FVector2f(S * 0.07f * R, -0.08f * R))}, Col, 1.3f); // 짙은 눈썹
		}
		Lines(RtA({FC + FVector2f(0.f, -0.02f * R), FC + FVector2f(-0.04f * R, 0.09f * R), FC + FVector2f(0.02f * R, 0.09f * R)}), Col, 1.1f); // 코
		const float Mw = 0.2f * R;
		if (bDevour) Lines(RtA(CirclePoints(FC + FVector2f(0.f, 0.22f * R), 0.13f * R + 0.03f * R * FMath::Abs(Sw), 16)), Col, 1.3f); // 크게 벌린 입
		else if (bGrit) Lines(RtA({FC + FVector2f(-Mw, 0.2f * R), FC + FVector2f(Mw, 0.2f * R)}), Col, 1.2f); // 이를 악문 입
		else if (bSmile) { TArray<FVector2f> M; for (int32 A = 20; A <= 160; A += 20) M.Add(FC + FVector2f(FMath::Cos(FMath::DegreesToRadians((float)A)) * Mw, 0.14f * R + FMath::Sin(FMath::DegreesToRadians((float)A)) * 0.1f * R)); Lines(RtA(M), Col, 1.2f); } // 웃는 입
		else { const float Op = 0.03f * R + 0.04f * R * FMath::Abs(Sw); Lines(RtA({FC + FVector2f(-Mw * 0.7f, 0.2f * R), FC + FVector2f(0.f, 0.2f * R + Op), FC + FVector2f(Mw * 0.7f, 0.2f * R), FC + FVector2f(0.f, 0.2f * R - Op * 0.3f), FC + FVector2f(-Mw * 0.7f, 0.2f * R)}), Col, 1.2f); } // 오물오물
		if (bCrown) // 왕관
			Glow(RtA({FVector2f(BC.X - 0.3f * R, Top), FVector2f(BC.X - 0.3f * R, Top - 0.28f * R), FVector2f(BC.X - 0.15f * R, Top - 0.12f * R), FVector2f(BC.X, Top - 0.34f * R), FVector2f(BC.X + 0.15f * R, Top - 0.12f * R), FVector2f(BC.X + 0.3f * R, Top - 0.28f * R), FVector2f(BC.X + 0.3f * R, Top)}), Col, 1.5f, 0.7f);
		const float ShY = Top + 0.22f * BH; // 팔 (짧게, 다리와 반대로)
		FVector2f RH = Foot, LH = Foot;
		for (const float S : {1.f, -1.f})
		{
			const FVector2f Sh = Rt(FVector2f(BC.X + S * BW / 2.f, ShY));
			const float A = -FMath::DegreesToRadians(32.f) * FMath::Sin(Ph - 0.6f) * S; // 팔은 걸음보다 조금 늦게 (D-184)
			const FVector2f H(Sh.X + S * 0.14f * R + FMath::Sin(A) * 0.07f * R, Sh.Y + 0.26f * R * FMath::Cos(A * 0.5f));
			Lines({Sh, H}, Col, 1.4f);
			(S > 0.f ? RH : LH) = H;
		}
		if (bBossK) { BossGear(Kd, Col, BC, Top, BW, BH, Hip, 0.f, RH, LH, R, T, Sw, Ph); return; } // 기울기·휘기는 Warp 가 함
		switch (Kd)
		{
		case 0: // 창
		{
			const FVector2f Tip(RH.X + 0.05f * R, RH.Y - 1.25f * R);
			Lines({RH + FVector2f(0.f, 0.35f * R), Tip}, Col, 1.3f);
			Lines({Tip, Tip + FVector2f(-0.08f * R, 0.2f * R), Tip + FVector2f(0.f, -0.12f * R), Tip + FVector2f(0.08f * R, 0.2f * R), Tip}, Col, 1.3f);
			break;
		}
		case 2: // 도끼
		{
			const FVector2f A0(RH.X, RH.Y - 1.f * R);
			Lines({RH + FVector2f(0.f, 0.25f * R), A0}, Col, 1.7f);
			Glow({A0, A0 + FVector2f(0.32f * R, -0.12f * R), A0 + FVector2f(0.36f * R, 0.22f * R), A0 + FVector2f(0.f, 0.2f * R)}, Col, 1.6f, 0.6f);
			break;
		}
		case 3: // 방패
		{
			const FVector2f SC(BC.X + 0.72f * BW, BC.Y + 0.12f * R);
			Glow({SC + FVector2f(0.f, -0.42f * R), SC + FVector2f(0.32f * R, -0.28f * R), SC + FVector2f(0.28f * R, 0.2f * R), SC + FVector2f(0.f, 0.45f * R), SC + FVector2f(-0.28f * R, 0.2f * R), SC + FVector2f(-0.32f * R, -0.28f * R), SC + FVector2f(0.f, -0.42f * R)}, Col, 1.8f, 0.7f);
			Shape(EPDSuit::Heart, SC.X, SC.Y, 0.15f * R, Col, 1.f, 0.4f);
			break;
		}
		case 5: // 떠오르는 + 둘
			for (int32 K = 0; K < 2; ++K)
			{
				const float U = FMath::Fmod(T + K * 0.5f, 1.f), A = 0.12f * R;
				const FVector2f P(BC.X + (K ? 0.45f : -0.45f) * BW, Top - U * 0.6f * R);
				const FLinearColor C2 = WithAlpha(Col, 1.f - U);
				Lines({P - FVector2f(A, 0.f), P + FVector2f(A, 0.f)}, C2, 1.3f);
				Lines({P - FVector2f(0.f, A), P + FVector2f(0.f, A)}, C2, 1.3f);
			}
			break;
		case 1: // 뒤로 흩날리는 속도선
			for (int32 K = 0; K < 3; ++K)
			{
				const float Y = Top + BH * (0.25f + 0.25f * K), X0 = BC.X - BW / 2.f - 0.15f * R;
				Lines({FVector2f(X0, Y), FVector2f(X0 - (0.3f + 0.15f * FMath::Fmod(T * 3.f + K, 1.f)) * R, Y)}, WithAlpha(Col, 0.6f), 1.1f);
			}
			break;
		default: break;
		}
	}
	// 보스 장식 (D-176): 머리 장식·무기·잔상 등. 좌표는 CardSoldier 와 같다
	void BossGear(int32 Kd, const FLinearColor& Col, FVector2f BC, float Top, float BW, float BH, FVector2f Hip, float Lean, FVector2f RH, FVector2f LH, float R, float T, float Sw, float Ph)
	{
		auto Rt = [&](FVector2f P) { return Lean == 0.f ? P : Hip + Rot2(P - Hip, Lean); };
		auto RtA = [&](TArray<FVector2f> A) { for (FVector2f& P : A) P = Rt(P); return A; };
		const float CX = BC.X;
		auto Arc = [&](float X0, float Y0, float W, float H, int32 N) { TArray<FVector2f> A; for (int32 K = 0; K <= N; ++K) A.Add(FVector2f(X0 + W * K / N, Y0 - FMath::Sin(PI * K / N) * H)); return A; };
		auto Spear = [&](FVector2f H, float Len) { const FVector2f Tip(H.X, H.Y - Len); Lines({H + FVector2f(0.f, 0.3f * R), Tip}, Col, 1.3f); Lines({Tip, Tip + FVector2f(-0.08f * R, 0.2f * R), Tip + FVector2f(0.f, -0.12f * R), Tip + FVector2f(0.08f * R, 0.2f * R), Tip}, Col, 1.2f); };
		switch (Kd)
		{
		case 7: // 방패 거인: 둥근 투구 + 큰 탑 방패
		{
			Lines(RtA(Arc(CX - 0.32f * R, Top, 0.64f * R, 0.3f * R, 10)), Col, 1.5f);
			Lines({FVector2f(CX, Top - 0.3f * R), FVector2f(CX, Top - 0.42f * R)}, Col, 1.4f);
			const float SX = CX + 0.62f * BW;
			Glow({FVector2f(SX - 0.26f * R, BC.Y - 0.55f * R), FVector2f(SX + 0.26f * R, BC.Y - 0.55f * R), FVector2f(SX + 0.26f * R, BC.Y + 0.4f * R), FVector2f(SX, BC.Y + 0.6f * R), FVector2f(SX - 0.26f * R, BC.Y + 0.4f * R), FVector2f(SX - 0.26f * R, BC.Y - 0.55f * R)}, Col, 1.8f, 0.7f);
			Shape(EPDSuit::Heart, SX, BC.Y - 0.05f * R, 0.14f * R, Col, 1.f, 0.4f);
			break;
		}
		case 8: // 분열 군주: 반쪽 왕관 둘
			for (const float S : {-1.f, 1.f})
			{
				const float X0 = CX + S * 0.2f * R;
				Lines({FVector2f(X0 - 0.16f * R, Top - 0.02f * R + S * 0.04f * R), FVector2f(X0 - 0.16f * R, Top - 0.24f * R), FVector2f(X0, Top - 0.12f * R), FVector2f(X0 + 0.16f * R, Top - 0.26f * R), FVector2f(X0 + 0.16f * R, Top - 0.02f * R - S * 0.04f * R)}, Col, 1.3f);
			}
			break;
		case 9: // 재생 왕: 홀 + 떠오르는 + 셋
		{
			const FVector2f Tip(RH.X, RH.Y - 1.f * R);
			Lines({RH + FVector2f(0.f, 0.2f * R), Tip}, Col, 1.4f);
			Lines(CirclePoints(Tip - FVector2f(0.f, 0.1f * R), 0.1f * R, 14), Col, 1.3f);
			for (int32 K = 0; K < 3; ++K)
			{
				const float U = FMath::Fmod(T + K / 3.f, 1.f), A = 0.12f * R;
				const FVector2f P(CX + (-0.5f + 0.5f * K) * BW, Top - 0.4f * R - U * 0.5f * R);
				Lines({P - FVector2f(A, 0.f), P + FVector2f(A, 0.f)}, WithAlpha(Col, 1.f - U), 1.3f);
				Lines({P - FVector2f(0.f, A), P + FVector2f(0.f, A)}, WithAlpha(Col, 1.f - U), 1.3f);
			}
			break;
		}
		case 10: // 돌진 기사: 깃털 투구 + 앞으로 뻗은 창
		{
			const FVector2f Hd = Rt(FVector2f(CX, Top));
			Lines(Arc(Hd.X - 0.3f * R, Hd.Y, 0.6f * R, 0.26f * R, 10), Col, 1.5f);
			Lines({Hd + FVector2f(0.f, -0.26f * R), Hd + FVector2f(-0.3f * R, -0.5f * R - 0.05f * R * Sw), Hd + FVector2f(-0.45f * R, -0.38f * R)}, Col, 1.2f);
			Lines({RH + FVector2f(-0.2f * R, 0.f), RH + FVector2f(1.1f * R, -0.15f * R)}, Col, 1.6f);
			Lines({RH + FVector2f(1.1f * R, -0.15f * R), RH + FVector2f(0.9f * R, -0.27f * R), RH + FVector2f(0.92f * R, -0.03f * R), RH + FVector2f(1.1f * R, -0.15f * R)}, Col, 1.4f);
			break;
		}
		case 11: // 소환사: 뾰족 모자 + 구슬 지팡이 + 도는 작은 카드 둘
		{
			Lines({FVector2f(CX - 0.4f * R, Top), FVector2f(CX + 0.4f * R, Top)}, Col, 1.4f);
			Lines({FVector2f(CX - 0.26f * R, Top), FVector2f(CX + 0.05f * R, Top - 0.62f * R), FVector2f(CX + 0.26f * R, Top)}, Col, 1.4f);
			const FVector2f Tip(RH.X, RH.Y - 1.05f * R);
			Lines({RH + FVector2f(0.f, 0.25f * R), Tip}, Col, 1.4f);
			Glow(CirclePoints(Tip - FVector2f(0.f, 0.12f * R), 0.12f * R, 14), Col, 1.4f, 0.6f);
			for (int32 K = 0; K < 2; ++K)
			{
				const float A = Ph + K * PI;
				const FVector2f O(BC.X + FMath::Cos(A) * 0.95f * R, BC.Y + FMath::Sin(A) * 0.3f * R);
				Lines(RectPoints(O.X - 0.08f * R, O.Y - 0.12f * R, 0.16f * R, 0.24f * R), Col, 1.f);
			}
			break;
		}
		case 12: // 유령: 머리 위 물결
		{
			TArray<FVector2f> W;
			for (int32 K = 0; K <= 6; ++K) W.Add(FVector2f(CX - 0.2f * R + 0.4f * R * K / 6.f, Top - 0.18f * R - 0.05f * R * FMath::Sin(Ph * 2.f + K)));
			Lines(W, Col, 1.f);
			break;
		}
		case 13: // 군단장: 깃발 창 + 어깨 견장
		{
			const FVector2f Tip(RH.X, RH.Y - 1.3f * R);
			Lines({RH + FVector2f(0.f, 0.3f * R), Tip}, Col, 1.3f);
			Lines({Tip, Tip + FVector2f(0.5f * R, 0.14f * R + 0.06f * R * Sw), Tip + FVector2f(0.f, 0.3f * R)}, Col, 1.3f);
			for (const float S : {-1.f, 1.f}) Lines(RtA({FVector2f(CX + S * BW / 2.f, Top + 0.14f * BH), FVector2f(CX + S * (BW / 2.f + 0.16f * R), Top + 0.1f * BH)}), Col, 1.8f);
			break;
		}
		case 14: // 분노 거인: 삐죽 머리 + 양손 도끼
		{
			TArray<FVector2f> Hair;
			for (int32 K = 0; K <= 8; ++K) Hair.Add(FVector2f(CX - 0.36f * R + 0.72f * R * K / 8.f, Top - (K % 2 ? 0.24f * R : 0.02f * R)));
			Lines(Hair, Col, 1.4f);
			for (const float S : {1.f, -1.f})
			{
				const FVector2f H = S > 0.f ? RH : LH, A0(H.X, H.Y - 0.75f * R);
				Lines({H + FVector2f(0.f, 0.2f * R), A0}, Col, 1.5f);
				Lines({A0, A0 + FVector2f(S * 0.26f * R, -0.1f * R), A0 + FVector2f(S * 0.3f * R, 0.18f * R), A0 + FVector2f(0.f, 0.16f * R)}, Col, 1.4f);
			}
			break;
		}
		case 15: // 공간 도약자: 뒤에 남는 잔상 카드 둘
			for (int32 K = 1; K <= 2; ++K) Lines(RectPoints(CX - BW / 2.f - K * 0.32f * R, Top, BW, BH), WithAlpha(Col, K == 1 ? 0.5f : 0.25f), 1.f);
			break;
		case 16: // 흡수자: 입으로 빨려 드는 점 셋
			for (int32 K = 0; K < 3; ++K)
			{
				const float U = FMath::Fmod(T + K / 3.f, 1.f), A = K * 2.1f + 1.f, D = (1.f - U) * 0.9f * R;
				const FVector2f M(BC.X, BC.Y + 0.18f * R);
				Lines(CirclePoints(M + FVector2f(FMath::Cos(A) * D, FMath::Sin(A) * D * 0.6f), 0.05f * R, 8), WithAlpha(Col, 0.4f + 0.6f * U), 1.1f);
			}
			break;
		case 17: // 수호자: 머리 위 고리 + 둥근 방패 + 창
		{
			TArray<FVector2f> Halo;
			for (int32 K = 0; K <= 20; ++K) { const float A = K / 20.f * TwoPi; Halo.Add(FVector2f(CX + FMath::Cos(A) * 0.3f * R, Top - 0.3f * R + FMath::Sin(A) * 0.08f * R)); }
			Lines(Halo, Col, 1.3f);
			Glow(CirclePoints(LH + FVector2f(-0.1f * R, -0.1f * R), 0.3f * R, 20), Col, 1.6f, 0.6f);
			Spear(RH, 1.2f * R);
			break;
		}
		case 18: // 저주술사: 뾰족 두건 + 별 지팡이
		{
			Lines({FVector2f(CX - BW / 2.f, Top + 0.1f * R), FVector2f(CX, Top - 0.5f * R), FVector2f(CX + BW / 2.f, Top + 0.1f * R)}, Col, 1.4f);
			const FVector2f Tip(RH.X, RH.Y - 0.95f * R);
			Lines({RH + FVector2f(0.f, 0.2f * R), Tip}, Col, 1.4f);
			Shape(EPDSuit::Wild, Tip.X, Tip.Y - 0.1f * R, 0.14f * R, Col, 1.f, 0.4f);
			break;
		}
		case 19: // 파괴자: 큰 망치 + 카드 금
		{
			const FVector2f Hd(RH.X, RH.Y - 0.9f * R);
			Lines({RH + FVector2f(0.f, 0.25f * R), Hd}, Col, 1.7f);
			Glow(RectPoints(Hd.X - 0.3f * R, Hd.Y - 0.16f * R, 0.6f * R, 0.28f * R), Col, 1.8f, 0.7f);
			Lines(RtA({FVector2f(CX + 0.2f * BW, Top + 0.05f * BH), FVector2f(CX + 0.32f * BW, Top + 0.22f * BH), FVector2f(CX + 0.18f * BW, Top + 0.32f * BH), FVector2f(CX + 0.36f * BW, Top + 0.48f * BH)}), Col, 1.f);
			break;
		}
		// 새 보스 · 최종 보스 (D-335, 미리보기 tools/neon/soldiers.py newbosses)
		case 20: // 블러퍼: 반쪽 가면 + 손에 펼친 카드 셋
		{
			Lines({FVector2f(CX - 0.3f * R, Top + 0.18f * BH), FVector2f(CX + 0.3f * R, Top + 0.18f * BH), FVector2f(CX + 0.24f * R, Top + 0.3f * BH), FVector2f(CX - 0.24f * R, Top + 0.3f * BH), FVector2f(CX - 0.3f * R, Top + 0.18f * BH)}, Col, 1.3f);
			for (int32 K = 0; K < 3; ++K)
			{
				const float A = FMath::DegreesToRadians(-30.f + K * 30.f);
				const FVector2f O(RH.X + FMath::Sin(A) * 0.3f * R, RH.Y - 0.25f * R - FMath::Cos(A) * 0.1f * R);
				TArray<FVector2f> Cd;
				for (const FVector2f Q : {FVector2f(-0.1f, -0.15f), FVector2f(0.1f, -0.15f), FVector2f(0.1f, 0.15f), FVector2f(-0.1f, 0.15f), FVector2f(-0.1f, -0.15f)}) Cd.Add(O + Rot2(Q * R, A));
				Lines(Cd, Col, 1.1f);
			}
			break;
		}
		case 21: // 빅 블라인드: 눈가리개 띠 + 머리 위 칩 더미 (판돈)
			Lines({FVector2f(CX - BW / 2.f - 0.05f * R, Top + 0.2f * BH), FVector2f(CX + BW / 2.f + 0.05f * R, Top + 0.2f * BH)}, Col, 2.f);
			Lines({FVector2f(CX + BW / 2.f + 0.05f * R, Top + 0.2f * BH), FVector2f(CX + BW / 2.f + 0.25f * R, Top + 0.32f * BH)}, Col, 1.3f);
			for (int32 K = 0; K < 4; ++K) Lines(Arc(CX - 0.22f * R, Top - 0.18f * R - K * 0.1f * R, 0.44f * R, -0.04f * R, 8), Col, 1.2f);
			break;
		case 22: // 딜러: 차양 모자 + 손끝에서 날아가는 카드
		{
			Lines({FVector2f(CX - 0.36f * R, Top + 0.02f * R), FVector2f(CX + 0.36f * R, Top + 0.02f * R), FVector2f(CX + 0.22f * R, Top - 0.12f * R), FVector2f(CX - 0.22f * R, Top - 0.12f * R), FVector2f(CX - 0.36f * R, Top + 0.02f * R)}, Col, 1.4f);
			const float U = FMath::Frac(T);
			const FVector2f F2(RH.X + 0.3f * R + U * 0.7f * R, RH.Y - 0.3f * R - FMath::Sin(U * PI) * 0.4f * R);
			TArray<FVector2f> Cd;
			for (const FVector2f Q : {FVector2f(-0.09f, -0.13f), FVector2f(0.09f, -0.13f), FVector2f(0.09f, 0.13f), FVector2f(-0.09f, 0.13f), FVector2f(-0.09f, -0.13f)}) Cd.Add(F2 + Rot2(Q * R, U * 6.f));
			Lines(Cd, Col, 1.1f);
			break;
		}
		case 23: // 카드 카운터: 동그란 안경 + 머리 위 세는 막대
			for (const float S : {-1.f, 1.f}) Lines(CirclePoints(FVector2f(CX + S * 0.14f * R, Top + 0.22f * BH), 0.1f * R, 14), Col, 1.2f);
			for (int32 K = 0; K < 4; ++K) Lines({FVector2f(CX - 0.24f * R + K * 0.12f * R, Top - 0.42f * R), FVector2f(CX - 0.24f * R + K * 0.12f * R, Top - 0.18f * R)}, Col, 1.1f);
			Lines({FVector2f(CX - 0.3f * R, Top - 0.22f * R), FVector2f(CX + 0.2f * R, Top - 0.38f * R)}, Col, 1.1f);
			break;
		case 24: // 하우스 룰: 규칙집 + 머리 위 '연쇄 금지' 표지
		{
			Glow(RectPoints(LH.X - 0.22f * R, LH.Y - 0.3f * R, 0.44f * R, 0.35f * R), Col, 1.5f, 0.6f);
			const FVector2f Sc(CX, Top - 0.42f * R);
			Lines(CirclePoints(Sc, 0.22f * R, 20), Col, 1.4f);
			for (const float S : {-1.f, 1.f}) Lines(CirclePoints(Sc + FVector2f(S * 0.07f * R, 0.f), 0.07f * R, 10), Col, 1.f);
			Lines({Sc + FVector2f(-0.15f * R, 0.15f * R), Sc + FVector2f(0.15f * R, -0.15f * R)}, Col, 1.5f);
			break;
		}
		case 25: // 쌍둥이: 반쪽 하트 배지 + 옆으로 이어진 리본
		{
			const FVector2f H(BC.X, BC.Y - 0.05f * R);
			Lines({H + FVector2f(0.f, 0.18f * R), H + FVector2f(-0.18f * R, 0.f), H + FVector2f(-0.12f * R, -0.12f * R), H + FVector2f(0.f, -0.05f * R), H + FVector2f(0.f, 0.18f * R)}, Col, 1.3f);
			for (int32 K = 0; K < 6; ++K) Lines({FVector2f(CX + BW / 2.f + K * 0.14f * R, Top + 0.3f * BH + 0.04f * R * FMath::Sin(Ph + K)), FVector2f(CX + BW / 2.f + K * 0.14f * R + 0.07f * R, Top + 0.3f * BH + 0.04f * R * FMath::Sin(Ph + K + 0.5f))}, Col, 1.1f);
			break;
		}
		case 26: // 바운서: 넓은 어깨 + 선글라스 띠 + 옆 줄 기둥
		{
			for (const float S : {-1.f, 1.f}) Lines({FVector2f(CX + S * BW / 2.f, Top + 0.12f * BH), FVector2f(CX + S * (BW / 2.f + 0.3f * R), Top + 0.2f * BH), FVector2f(CX + S * (BW / 2.f + 0.3f * R), Top + 0.45f * BH)}, Col, 2.f);
			Lines({FVector2f(CX - 0.3f * R, Top + 0.2f * BH), FVector2f(CX + 0.3f * R, Top + 0.2f * BH)}, Col, 2.4f);
			const float PX = CX + BW / 2.f + 0.75f * R;
			Lines({FVector2f(PX, Top + BH + 0.6f * R), FVector2f(PX, Top + 0.2f * BH)}, Col, 1.5f);
			Lines(CirclePoints(FVector2f(PX, Top + 0.2f * BH - 0.08f * R), 0.08f * R, 10), Col, 1.3f);
			Lines({FVector2f(PX, Top + 0.35f * BH), FVector2f(PX + 0.35f * R, Top + 0.42f * BH), FVector2f(PX + 0.7f * R, Top + 0.35f * BH)}, Col, 1.2f);
			break;
		}
		case 27: // 하이롤러: 실크해트 + 시가 + 손에 칩 탑
			Lines({FVector2f(CX - 0.36f * R, Top), FVector2f(CX + 0.36f * R, Top)}, Col, 1.5f);
			Lines({FVector2f(CX - 0.22f * R, Top), FVector2f(CX - 0.22f * R, Top - 0.5f * R), FVector2f(CX + 0.22f * R, Top - 0.5f * R), FVector2f(CX + 0.22f * R, Top)}, Col, 1.5f);
			Lines({FVector2f(CX + 0.12f * R, Top + 0.32f * BH), FVector2f(CX + 0.42f * R, Top + 0.3f * BH)}, Col, 1.3f);
			for (int32 K = 0; K < 5; ++K) Lines(Arc(RH.X - 0.14f * R, RH.Y - 0.1f * R - K * 0.09f * R, 0.28f * R, -0.035f * R, 8), Col, 1.2f);
			break;
		case 28: // 더 하우스: 왕관(bCrown) + 망토 + 등 뒤 룰렛 바퀴
		{
			const FVector2f Wc(CX, Top + 0.4f * BH);
			const float Rr = 0.95f * R;
			Lines(CirclePoints(Wc, Rr, 40), WithAlpha(Col, Col.A * 0.5f), 1.1f);
			for (int32 K = 0; K < 8; ++K) { const float A = Ph * 0.5f + K * PI / 4.f; Lines({Wc + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Rr * 0.8f, Wc + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Rr}, WithAlpha(Col, Col.A * 0.5f), 1.f); }
			for (const float S : {-1.f, 1.f}) Lines({FVector2f(CX + S * BW / 2.f, Top + 0.1f * BH), FVector2f(CX + S * (BW / 2.f + 0.25f * R), Top + BH + 0.3f * R)}, Col, 1.2f);
			break;
		}
		default: break;
		}
	}
	// 카드(=타워): 둥근 사각 틀 안에 숫자와 작은 무늬 기호 (docs/15-art-style.md: 판 위 물체는 둥근 사각 틀 안에 작은 기호)
	// bFrame=false 면 틀 없이 안쪽만 (손패 카드는 카드 테두리가 틀 역할)
	void CardShape(const FPDCard& Card, float CX, float CY, float R, float Alpha, bool bStrong, float Pulse = 0.f, bool bFrame = true)
	{
		const FObjScope Ob(*this, TEXT("Card"));
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
		const FObjScope Ob(*this, TEXT("JokerArt"));
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
		const FObjScope Ob(*this, TEXT("JokerTower"));
		const FLinearColor Col = WithAlpha(JokerPink, Alpha);
		const float H = R * 0.98f;
		Box(CX - H, CY - H, H * 2.f, H * 2.f, WithAlpha(Col, 0.08f + 0.06f * Pulse));
		if (IsUpperJoker(K)) // 상위 조커 (D-231): 재료 둘의 그림을 작게 + 금색 이중 테두리 + 틀 위 작은 왕관
		{
			UpperArt(K - UpperBase, CX, CY, R, Col, 1.3f, 0.8f);
			UpperMark(CX, CY, R, WithAlpha(Hex(TEXT("ffd96a")), Alpha), Pulse);
			return;
		}
		Glow(RoundRectPoints(CX - H, CY - H, H * 2.f, H * 2.f, R * 0.22f), Col, 2.f, 1.4f + 0.5f * Pulse);
		JokerArt(K, CX, CY, R, Col, 1.3f, 0.8f);
	}
	// 상위 조커 기호 (미리보기 tools/neon/upper_jokers.py 그대로): 점 배열을 틀 반 너비 R 로 늘려 네온 선
	void UpperArt(int32 U, float CX, float CY, float R, const FLinearColor& Col, float Th, float Gk)
	{
		const FObjScope Ob(*this, TEXT("UpperArt"));
		if (U < 0 || U >= UpperJokerCount) return;
		const int* Ix = GPDUpperIndex[U];
		for (int L = Ix[0]; L < Ix[0] + Ix[1]; ++L)
		{
			TArray<FVector2f> P;
			P.Reserve(GPDUpperLines[L][1]);
			for (int I = 0; I < GPDUpperLines[L][1]; ++I) { const float* Q = GPDUpperPts[GPDUpperLines[L][0] + I]; P.Add(FVector2f(CX + Q[0] * R, CY + Q[1] * R)); }
			Glow(P, Col, Th, Gk);
		}
		for (int T = Ix[2]; T < Ix[2] + Ix[3]; ++T)
		{
			const FPDUpperText& X = GPDUpperTexts[T];
			TextCentered(X.S, CX + X.X * R, CY + X.Y * R, Col, X.Px * R * 1.1f, true);
		}
	}
	void UpperMark(float CX, float CY, float R, const FLinearColor& Gold, float Pulse)
	{
		const FObjScope Ob(*this, TEXT("UpperMark"));
		const float H = R * 0.98f, H2 = R * 0.86f;
		Glow(RoundRectPoints(CX - H, CY - H, H * 2.f, H * 2.f, R * 0.22f), Gold, 2.f, 1.2f + 0.5f * Pulse);
		Glow(RoundRectPoints(CX - H2, CY - H2, H2 * 2.f, H2 * 2.f, R * 0.17f), Gold, 1.2f, 0.6f);
		const float W = R * 0.22f, Hh = R * 0.18f, BY = CY - H + 1.f;
		Glow({{CX - W, BY}, {CX - W, BY - Hh}, {CX - W * 0.5f, BY - Hh * 0.5f}, {CX, BY - Hh * 1.1f}, {CX + W * 0.5f, BY - Hh * 0.5f}, {CX + W, BY - Hh}, {CX + W, BY}, {CX - W, BY}}, Gold, 1.2f, 0.9f);
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
		const FObjScope Ob(*this, TEXT("Sprite"));
		const float H = AtlasCell * S * 0.5f;
		if (Ob.bOn) GPDLayout.Grow({CX - H * 0.6f, CY - H * 0.6f, CX + H * 0.6f, CY + H * 0.6f}, Tint.A * FadeK); // 칸 가장자리는 빛번짐 여백
		FSlateDrawElement::MakeBox(Out, L, PG(CX - H, CY - H, H * 2.f, H * 2.f), B, ESlateDrawEffect::PreMultipliedAlpha, PM(FadeC(Tint)));
	}
	// 붓 찍기 (D-143): 왼쪽 위 (X, Y), 크기 W × H
	void Image(int32 L, const FSlateBrush* B, float X, float Y, float W, float H, const FLinearColor& Tint)
	{
		FSlateDrawElement::MakeBox(Out, L, PG(X, Y, W, H), B, ESlateDrawEffect::PreMultipliedAlpha, PM(FadeC(Tint)));
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
		const FObjScope Ob(*this, TEXT("Icon"));
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
		const FObjScope Ob(*this, TEXT("PowerBadge"));
		if (P.After < 0.f || P.Before <= 0.f) return;
		const float Pct = (P.After / P.Before - 1.f) * 100.f;
		const FLinearColor Col = Pct >= 0.5f ? Hex(TEXT("9cffb0")) : Pct <= -0.5f ? Hex(TEXT("ff6a6a")) : Hex(TEXT("cfd8e6"));
		Badge(TrF(TEXT("판 위력 %s → %s (%s%.0f%%)"), *ShortNum(P.Before), *ShortNum(P.After), Pct >= 0.f ? TEXT("+") : TEXT(""), Pct), X, Y, Col, 1.f);
	}
};
