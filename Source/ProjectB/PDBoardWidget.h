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

private:
	struct FCanvas; // 그리기 도우미 (cpp 안에 정의)
	void PaintAll(FCanvas& C, FCanvas& F) const;
	void HandleEvents();
	void UpdateFx(float Dt);
	void UpdatePreview();
	FVector2f ToDesign(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const;
	void Click(FVector2f P, bool bRight);
	void DoFuse(int32 SynergyIdx);     // 합성 + 빨려 들어가는 연출 준비
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
