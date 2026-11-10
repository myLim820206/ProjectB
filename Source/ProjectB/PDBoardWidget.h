#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "PDCore.h"

// 포커 디펜스 화면 전체. 1280 x 720 기준 좌표로 그리고 창 크기에 맞춰 확대한다.
// 그래픽은 도형과 선뿐이고, 네온 빛번짐은 굵고 옅은 선 위에 가늘고 밝은 선을 겹쳐 흉내 낸다.
// 이펙트(레이저, 불꽃, 충격파, 파편, 족보 섬광, 연쇄 번개, 화면 흔들림, 배너)는 이 위젯이 직접 만든다.
class SPDBoardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SPDBoardWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FReply OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	// 게임이 낸 소리를 꺼내 간다 (게임 모드가 매 프레임 재생)
	std::vector<PD::FPDSoundEvent> TakeSounds()
	{
		std::vector<PD::FPDSoundEvent> Out;
		Out.swap(Game.Sounds);
		return Out;
	}

	// 메뉴의 [게임 종료]. 게임 모드가 채운다 (월드가 있어야 PIE·패키지 모두 바르게 끝낼 수 있어서)
	TFunction<void()> OnQuit;

private:
	struct FCanvas; // 그리기 도우미 (cpp 안에 정의)
	void PaintAll(FCanvas& C, FCanvas& F) const;
	void PaintHandGuide(FCanvas& C) const; // 왼쪽 족보 패널
	void PaintMenu(FCanvas& C) const;      // Esc 메뉴 창
	void MenuSelect(int32 Item);           // 메뉴 항목 실행
	int32 MenuHoverAt(FVector2f P) const;  // 메뉴 창에서 이 위치의 항목 (-1 = 없음)
	void RestartRun();                     // 새 판 시작 (게임 오버·메뉴 공통)
	// 합성 위치 고르기: [합성]·[조커] 버튼을 누르면 재료 칸 중 하나를 클릭해 결과가 놓일 칸을 고른다
	std::vector<int> PickCells;            // 고르는 중인 재료 칸 (비어 있으면 고르는 중 아님)
	bool bPickJoker = false;               // 조커 합성을 고르는 중
	void StartPick(const std::vector<int>& Cells, bool bJoker);
	void FinishPick(int32 Cell);           // 고른 칸으로 합성 (-1 = 기본 칸)
	int32 FindPickIndex() const;           // 고르는 재료와 같은 족보·조커 묶음 번호 (-1 = 사라짐)
	bool bMenu = false;                    // 메뉴 창 (Esc). 여는 동안 게임은 멈춘다
	int32 MenuSel = 0;                     // 키보드로 고른 메뉴 항목
	void HandleEvents();
	void UpdateFx(float Dt);
	void UpdatePreview();
	FVector2f ToDesign(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	void Click(FVector2f P, bool bRight);
	void DoFuse(int32 SynergyIdx, int32 Cell = -1); // 합성 + 빨려 들어가는 연출 준비 (Cell = 결과 칸, -1 = 기본)
	int32 HoveredSynergy() const;      // 마우스가 올라간 타워의 족보 번호
	int32 TreeHoverAt(FVector2f P) const; // 트리 창에서 이 위치의 노드 (-1 = 없음)
	TArray<FVector2f> PendingFuseCells; // 합성 직전 족보 칸 위치 (연출용)

	// ---- 이펙트
	struct FParticle
	{
		FVector2f P, V;
		float Life, MaxLife, Size, Drag, Gravity;
		FLinearColor Col;
		bool bStreak;
	};
	struct FBeam
	{
		FVector2f A, B;
		float T, MaxT, Width;
		FLinearColor Col;
		int32 Style = 0; // 0 보통(레벨 2면 쌍줄), 1 레일건, 2 번개, 3 로열 왕관, 4 체셔 웃음
		int32 Lv = 1;
	};
	struct FRing
	{
		FVector2f C;
		float R0, R1, T, MaxT, Width;
		FLinearColor Col;
	};
	struct FFloatText
	{
		FString S;
		FVector2f P;
		float T, MaxT, Size;
		FLinearColor Col;
	};
	struct FCellFlash
	{
		int Key;
		float T;
		FLinearColor Col;
	};
	// 번쩍이는 별빛 (네 갈래 빛살 + 부드러운 빛 덩어리)
	struct FFlare
	{
		FVector2f C;
		float T, MaxT, Size, Rot;
		FLinearColor Col;
	};
	// 몸 조각: 돌며 날아가는 짧은 선
	struct FShard
	{
		FVector2f P, V;
		float A, AV, Len, Life, MaxLife;
		FLinearColor Col;
	};
	TArray<FFlare> Flares;
	TArray<FShard> Shards;
	TMap<int32, float> HitFlash; // 적 Id → 맞은 순간 번쩍임 남은 시간
	TMap<int32, float> HpShown;  // 적 Id → 천천히 줄어드는 체력 막대 (깎인 만큼 흰색으로 보임)
	void HitFx(FVector2f P, FLinearColor Col, float Power, int32 Crits, float Dmg); // 맞는 순간 (모든 발사체 공통)
	void DeathFx(FVector2f P, PD::EPDEnemy Type);                                   // 죽는 순간
	void CritFx(FVector2f P, int32 N); // 치명타 흰 불꽃
	void Burst(FVector2f P, int N, FLinearColor Col, float SpeedMin, float SpeedMax, float Life, float Size, bool bStreak = false, float Gravity = 0.f);

	PD::FPDGame Game;
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);

	// 입력과 미리보기
	int32 Sel = -1;
	int32 HoverX = -1, HoverY = -1;
	FVector2f Mouse = FVector2f(0.f, 0.f);
	std::vector<PD::FPDSynergy> Preview; // 고른 카드를 마우스 칸에 놓으면 생길 족보
	std::vector<PD::FPDJokerGroup> JGroups; // 조커 합성 가능한 묶음 (매 프레임 갱신)
	bool bCodex = false;                   // 조커 도감 화면
	bool bTree = false;                    // 스킬 트리 화면 (K)
	bool bUpg = false;                     // 골드 강화 화면 (U)
	int32 TreeBuyId = -1;                  // 방금 찍은 노드 (연출)
	float TreeBuyT = 0.f;
	FString PreviewKey;

	// 이펙트 상태
	TArray<FParticle> Particles;
	TArray<FBeam> Beams;
	TArray<FRing> Rings;
	TArray<FFloatText> Floats;
	TArray<FCellFlash> CellFlashes;
	TMap<int32, TArray<FVector2f>> Trails; // 빠른 적 잔상
	float Shake = 0.f;
	float FlashT = 0.f;
	FLinearColor FlashCol = FLinearColor::White;
	float BannerT = 0.f, BannerMax = 1.f;
	FString BannerText, BannerSub;
	FLinearColor BannerCol = FLinearColor::White;
	float Clock = 0.f;

	// 그릴 때 정한 화면 배율 (마우스 좌표 변환에 씀)
	mutable float ViewScale = 1.f;
	mutable FVector2f ViewOffset = FVector2f(0.f, 0.f);
};
