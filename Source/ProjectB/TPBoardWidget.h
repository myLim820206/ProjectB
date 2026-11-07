#pragma once

#include "CoreMinimal.h"
#include "Brushes/SlateColorBrush.h"
#include "Widgets/SCompoundWidget.h"
#include "TPCore.h"

// 테트리스 포커 화면 전체. 1280 x 720 기준 좌표로 그리고 창 크기에 맞춰 확대한다.
// 네온 선은 굵고 옅은 선 위에 가늘고 밝은 선을 겹쳐 빛번짐처럼 보이게 한다.
class STPBoardWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(STPBoardWidget) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;
	virtual bool SupportsKeyboardFocus() const override { return true; }
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(1280.0, 720.0); }

private:
	struct FCanvas; // 그리기 도우미 (cpp 안에 정의)
	void PaintAll(FCanvas& C) const;

	TP::FTPGame Game;
	FSlateColorBrush WhiteBrush = FSlateColorBrush(FLinearColor::White);
};
