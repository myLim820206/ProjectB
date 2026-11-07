#include "TPBoardWidget.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

using namespace TP;

namespace TPDraw
{
constexpr float DesignW = 1280.f;
constexpr float DesignH = 720.f;
constexpr float CellSize = 34.f;
constexpr float BoardX = 470.f;
constexpr float BoardY = 54.f;
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
FLinearColor SuitColor(ETPSuit S)
{
	switch (S)
	{
	case ETPSuit::Heart: return Hex(TEXT("ff6a8a"));
	case ETPSuit::Diamond: return Hex(TEXT("ffa86a"));
	case ETPSuit::Club: return Hex(TEXT("8affc4"));
	default: return Hex(TEXT("cfe4ff"));
	}
}
FString RankText(int Rank) { return Rank == 1 ? FString(TEXT("A")) : FString::FromInt(Rank); }
FString Num(int N) { return FText::AsNumber(N).ToString(); }

const TCHAR* HandName(ETPHandKind K)
{
	static const TCHAR* Names[] = {TEXT("파이브 카드"), TEXT("포카드"), TEXT("풀하우스"), TEXT("트리플"), TEXT("투 페어")};
	return Names[(int)K];
}
const TCHAR* HandCond(ETPHandKind K)
{
	static const TCHAR* Conds[] = {TEXT("같은 숫자 5장+ 붙음"), TEXT("같은 숫자 4장 붙음"), TEXT("트리플 + 맞닿은 다른 숫자 페어"),
		TEXT("같은 숫자 3장 붙음"), TEXT("다른 숫자 페어 두 쌍이 맞닿음")};
	return Conds[(int)K];
}
const TCHAR* BossText(ETPBoss B)
{
	switch (B)
	{
	case ETPBoss::NoHold: return TEXT("보스: 보관(HOLD) 금지");
	case ETPBoss::NoRed: return TEXT("보스: 빨간 카드(하트·다이아)는 칩 0");
	case ETPBoss::NoFlush: return TEXT("보스: 플러시 보너스 없음");
	default: return TEXT("일반 라운드");
	}
}
FString HandsJoined(const std::vector<ETPHandKind>& Kinds, const std::vector<bool>& Flushes, const TCHAR* Sep)
{
	FString S;
	for (size_t I = 0; I < Kinds.size(); ++I)
	{
		if (I) S += Sep;
		S += HandName(Kinds[I]);
		if (Flushes[I]) S += TEXT("·플러시");
	}
	return S;
}

struct FRectF
{
	float X, Y, W, H;
};
FRectF RectOf(const std::vector<FTPCell>& Cells, float Cs = CellSize, float Ox = BoardX, float Oy = BoardY)
{
	int X0 = 999, Y0 = 999, X1 = -999, Y1 = -999;
	for (const FTPCell& C : Cells)
	{
		X0 = FMath::Min(X0, C.X);
		Y0 = FMath::Min(Y0, C.Y);
		X1 = FMath::Max(X1, C.X);
		Y1 = FMath::Max(Y1, C.Y);
	}
	return {Ox + X0 * Cs, Oy + Y0 * Cs, (X1 - X0 + 1) * Cs, (Y1 - Y0 + 1) * Cs};
}
float BX(float X) { return BoardX + X * CellSize; }
float BY(float Y) { return BoardY + Y * CellSize; }
} // namespace TPDraw

using namespace TPDraw;

// ---------------------------------------------------------------- 그리기 도우미
struct STPBoardWidget::FCanvas
{
	FSlateWindowElementList& Out;
	FGeometry Geo;
	int32 Layer;
	const FSlateBrush* White;

	int32 NextLayer() { return ++Layer; }

	void Lines(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick)
	{
		FSlateDrawElement::MakeLines(Out, NextLayer(), Geo.ToPaintGeometry(), P, ESlateDrawEffect::None, Col, true, Thick);
	}
	// 네온 선: 넓고 옅은 선 두 겹 + 가늘고 밝은 선
	void Glow(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Strength = 1.f)
	{
		if (Strength > 0.f)
		{
			Lines(P, WithAlpha(Col, 0.10f * Strength), Thick * 5.f);
			Lines(P, WithAlpha(Col, 0.22f * Strength), Thick * 2.4f);
		}
		Lines(P, Col, Thick);
	}
	void Dashed(const TArray<FVector2f>& P, const FLinearColor& Col, float Thick, float Dash = 4.f, float Gap = 4.f)
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
				if (bOn) Glow({A, E}, Col, Thick, 0.6f);
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
		FSlateDrawElement::MakeBox(Out, NextLayer(), Geo.ToPaintGeometry(FVector2f(W, H), FSlateLayoutTransform(FVector2f(X, Y))), White,
			ESlateDrawEffect::None, Col);
	}
	static TArray<FVector2f> RectPoints(float X, float Y, float W, float H)
	{
		return {{X, Y}, {X + W, Y}, {X + W, Y + H}, {X, Y + H}, {X, Y}};
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
		P.Add(P[0]);
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
	void Text(const FString& S, float X, float Y, const FLinearColor& Col, float Px = 13.f, int32 Align = 0, bool bBold = false)
	{
		const FSlateFontInfo F = Font(Px, bBold);
		const FVector2f Sz = Measure(S, F);
		const float Left = Align == 1 ? X - Sz.X / 2.f : Align == 2 ? X - Sz.X : X;
		const float Top = Y - Sz.Y * 0.8f;
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(Left, Top))), S, F,
			ESlateDrawEffect::None, Col);
	}
	// 가운데 정렬 글자 (세로도 가운데)
	void TextCentered(const FString& S, float CX, float CY, const FLinearColor& Col, float Px, bool bBold)
	{
		const FSlateFontInfo F = Font(Px, bBold);
		const FVector2f Sz = Measure(S, F);
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(CX - Sz.X / 2.f, CY - Sz.Y / 2.f))), S,
			F, ESlateDrawEffect::None, Col);
	}
	void Panel(float X, float Y, float W, float H) { Lines(RectPoints(X, Y, W, H), Hex(TEXT("7fd6ff"), 0.3f), 1.f); }

	// 무늬를 선으로 그린다 (글꼴에 기대지 않고 네온 화풍에 맞춘다)
	void Suit(ETPSuit S, float CX, float CY, float Size, const FLinearColor& Col, float Thick, float GlowK)
	{
		const float R = Size * 0.5f;
		auto HeartPts = [&](float Flip, float OffY) {
			TArray<FVector2f> P;
			for (int32 I = 0; I <= 40; ++I)
			{
				const float T = I / 40.f * TwoPi;
				const float HX = 16.f * FMath::Sin(T) * FMath::Sin(T) * FMath::Sin(T);
				const float HY = -(13.f * FMath::Cos(T) - 5.f * FMath::Cos(2 * T) - 2.f * FMath::Cos(3 * T) - FMath::Cos(4 * T));
				P.Add(FVector2f(CX + HX / 17.f * R, CY + (HY / 17.f - 0.15f) * R * Flip + OffY));
			}
			return P;
		};
		switch (S)
		{
		case ETPSuit::Diamond:
			Glow({{CX, CY - R}, {CX + R * 0.72f, CY}, {CX, CY + R}, {CX - R * 0.72f, CY}, {CX, CY - R}}, Col, Thick, GlowK);
			break;
		case ETPSuit::Heart:
			Glow(HeartPts(1.f, 0.f), Col, Thick, GlowK);
			break;
		case ETPSuit::Spade:
			Glow(HeartPts(-1.f, -R * 0.12f), Col, Thick, GlowK);
			Glow({{CX, CY + R * 0.45f}, {CX + R * 0.32f, CY + R}, {CX - R * 0.32f, CY + R}, {CX, CY + R * 0.45f}}, Col, Thick, GlowK);
			break;
		case ETPSuit::Club:
		{
			const FVector2f Cs[3] = {{CX, CY - R * 0.42f}, {CX - R * 0.42f, CY + R * 0.08f}, {CX + R * 0.42f, CY + R * 0.08f}};
			for (const FVector2f& C0 : Cs)
			{
				TArray<FVector2f> P;
				for (int32 I = 0; I <= 20; ++I)
				{
					const float A = I / 20.f * TwoPi;
					P.Add(C0 + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R * 0.34f);
				}
				Glow(P, Col, Thick, GlowK);
			}
			Glow({{CX, CY + R * 0.2f}, {CX + R * 0.3f, CY + R}, {CX - R * 0.3f, CY + R}, {CX, CY + R * 0.2f}}, Col, Thick, GlowK);
			break;
		}
		}
	}

	struct FCardOpts
	{
		float Alpha = 1.f;
		bool bHot = false;
		bool bDash = false;
	};
	// 카드 한 장을 칸 두 개에 걸쳐 그린다 (세로 카드면 위에 숫자 아래에 무늬, 가로 카드면 왼쪽 숫자 오른쪽 무늬)
	void Card(float PX, float PY, float W, float H, const FTPCard* InCard, const FCardOpts& O)
	{
		const FLinearColor Base = InCard ? SuitColor(InCard->Suit) : Hex(TEXT("7fd6ff"));
		const FLinearColor Edge = WithAlpha(O.bHot ? FLinearColor::White : Base, O.Alpha);
		const float Thick = O.bHot ? 2.4f : 1.6f;
		const TArray<FVector2f> Outline = RoundRectPoints(PX + 3.f, PY + 3.f, W - 6.f, H - 6.f, 6.f);
		if (O.bDash) Dashed(Outline, Edge, Thick);
		else Glow(Outline, Edge, Thick, O.bHot ? 1.8f : 1.f);
		if (!InCard || O.bDash) return;
		const float S = FMath::Min(W, H);
		const FLinearColor Ink = WithAlpha(Base, O.Alpha);
		const FString R = RankText(InCard->Rank);
		if (H > W)
		{
			TextCentered(R, PX + W / 2.f, PY + H * 0.3f, Ink, S * 0.62f, true);
			Suit(InCard->Suit, PX + W / 2.f, PY + H * 0.7f, S * 0.5f, Ink, 1.4f, O.bHot ? 1.2f : 0.6f);
		}
		else
		{
			TextCentered(R, PX + W * 0.3f, PY + H / 2.f, Ink, S * 0.62f, true);
			Suit(InCard->Suit, PX + W * 0.72f, PY + H / 2.f, S * 0.5f, Ink, 1.4f, O.bHot ? 1.2f : 0.6f);
		}
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
void STPBoardWidget::Construct(const FArguments& InArgs)
{
	SetCanTick(true);
}

void STPBoardWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	Game.Tick(InDeltaTime);
	Invalidate(EInvalidateWidgetReason::Paint);
}

FReply STPBoardWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat()) return FReply::Handled();
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Left) { Game.Act(ETPAction::Left); Game.SetHeld(ETPHeldKey::Left, true); }
	else if (K == EKeys::Right) { Game.Act(ETPAction::Right); Game.SetHeld(ETPHeldKey::Right, true); }
	else if (K == EKeys::Down) { Game.Act(ETPAction::SoftDrop); Game.SetHeld(ETPHeldKey::Down, true); }
	else if (K == EKeys::Up || K == EKeys::X) Game.Act(ETPAction::Rotate);
	else if (K == EKeys::Z) Game.Act(ETPAction::RotateCcw);
	else if (K == EKeys::SpaceBar) Game.Act(ETPAction::HardDrop);
	else if (K == EKeys::C || K == EKeys::LeftShift || K == EKeys::RightShift) Game.Act(ETPAction::Hold);
	else if (K == EKeys::Enter) Game.Act(ETPAction::Next);
	else if (K == EKeys::R) Game.Act(ETPAction::Restart);
	else if (K == EKeys::P) Game.Act(ETPAction::Pause);
	else return FReply::Unhandled();
	return FReply::Handled();
}

FReply STPBoardWidget::OnKeyUp(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::Left) Game.SetHeld(ETPHeldKey::Left, false);
	else if (K == EKeys::Right) Game.SetHeld(ETPHeldKey::Right, false);
	else if (K == EKeys::Down) Game.SetHeld(ETPHeldKey::Down, false);
	else return FReply::Unhandled();
	return FReply::Handled();
}

FReply STPBoardWidget::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (!Game.bStarted || (Game.Phase != ETPPhase::Play && Game.Phase != ETPPhase::Chain)) Game.Act(ETPAction::Tap);
	return FReply::Handled().SetUserFocus(SharedThis(this), EFocusCause::Mouse);
}

int32 STPBoardWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 창 전체 배경
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &WhiteBrush, ESlateDrawEffect::None, Hex(TEXT("06060b")));
	// 1280 x 720 화면을 비율 유지하며 가운데에 맞춘다
	const auto LocalSize = AllottedGeometry.GetLocalSize(); // 엔진 버전에 따라 FVector2D 또는 FVector2f
	const float SizeX = (float)LocalSize.X, SizeY = (float)LocalSize.Y;
	const float Scale = FMath::Max(0.01f, FMath::Min(SizeX / DesignW, SizeY / DesignH));
	const FVector2f Offset((SizeX - DesignW * Scale) / 2.f, (SizeY - DesignH * Scale) / 2.f);
	const FGeometry Design = AllottedGeometry.MakeChild(FVector2f(DesignW, DesignH), FSlateLayoutTransform(Scale, Offset));
	FCanvas C{OutDrawElements, Design, LayerId, &WhiteBrush};
	PaintAll(C);
	return C.Layer;
}

void STPBoardWidget::PaintAll(FCanvas& C) const
{
	const FTPGame& G = Game;
	const FTPRoundDef& R = RoundDef(G.Round);
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")),
					   Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Pink = Hex(TEXT("ff9ad5")), Red = Hex(TEXT("ff6a8a"));

	// 판 격자와 테두리
	for (int X = 1; X < Cols; ++X) C.Lines({{BX(X), BoardY}, {BX(X), BY(Rows)}}, Hex(TEXT("788cb4"), 0.07f), 1.f);
	for (int Y = 1; Y < Rows; ++Y) C.Lines({{BoardX, BY(Y)}, {BX(Cols), BY(Y)}}, Hex(TEXT("788cb4"), 0.07f), 1.f);
	C.Glow(FCanvas::RectPoints(BoardX, BoardY, Cols * CellSize, Rows * CellSize), Hex(TEXT("7fd6ff"), 0.5f), 1.5f);

	// 미리보기 (이 자리에 떨어뜨리면)
	const bool bShowPiece = G.bHasPiece && G.Phase == ETPPhase::Play;
	FTPPreview Pv;
	std::set<std::pair<int, int>> Hot;
	if (bShowPiece)
	{
		Pv = G.Preview();
		if (!Pv.Sim.Steps.empty())
			for (const FTPCell& Cc : Pv.Sim.Steps[0].Cells) Hot.insert({Cc.X, Cc.Y});
	}
	const bool bBlink = ((int)(G.Time * 4.f)) % 2 == 0;
	auto IsHot = [&](const std::vector<FTPCell>& Cells) {
		for (const FTPCell& Cc : Cells)
			if (Hot.count({Cc.X, Cc.Y})) return true;
		return false;
	};

	// 원 페어 준비 표시: 같은 숫자 카드 2장이 붙어 있으면 중심끼리 선
	const std::vector<FTPCardCells> Cards = CardsOf(G.Board);
	for (const FTPCluster& Cl : RankClusters(G.Board, G.Pool))
	{
		if (Cl.Ids.size() != 2) continue;
		const FRectF A = RectOf(FindCard(Cards, Cl.Ids[0])->Cells), B = RectOf(FindCard(Cards, Cl.Ids[1])->Cells);
		C.Glow({{A.X + A.W / 2.f, A.Y + A.H / 2.f}, {B.X + B.W / 2.f, B.Y + B.H / 2.f}}, Hex(TEXT("ffd96a"), 0.4f), 4.f);
	}
	for (const FTPCardCells& Cc : Cards)
	{
		const FRectF Rc = RectOf(Cc.Cells);
		FCanvas::FCardOpts O;
		O.bHot = IsHot(Cc.Cells) && bBlink;
		C.Card(Rc.X, Rc.Y, Rc.W, Rc.H, &G.Pool[Cc.Id], O);
	}
	if (bShowPiece)
	{
		for (int K = 0; K < 2; ++K)
		{
			if (Pv.CardCells[K].empty()) continue;
			const bool bH = IsHot(Pv.CardCells[K]);
			const FRectF Rc = RectOf(Pv.CardCells[K]);
			FCanvas::FCardOpts O;
			O.Alpha = bH ? 0.75f : 0.4f;
			O.bDash = !bH;
			O.bHot = bH && bBlink;
			C.Card(Rc.X, Rc.Y, Rc.W, Rc.H, bH ? &G.Pool[G.Piece.Cards[K]] : nullptr, O);
		}
		const auto Cells = G.CellsOf(G.Piece);
		for (int K = 0; K < 2; ++K)
		{
			const std::vector<FTPCell> Two = {Cells[K * 2], Cells[K * 2 + 1]};
			if (Two[0].Y < 0 || Two[1].Y < 0) continue;
			const FRectF Rc = RectOf(Two);
			C.Card(Rc.X, Rc.Y, Rc.W, Rc.H, &G.Pool[G.Piece.Cards[K]], {});
		}
	}

	// 터지는 연출
	for (const FTPBurstFx& F : G.Fx)
	{
		const float A = FMath::Min(1.f, F.T / 0.5f);
		for (const FTPBurstFx::FItem& It : F.Items)
		{
			const FRectF Rc = RectOf(It.Cells);
			FCanvas::FCardOpts O;
			O.bHot = true;
			O.Alpha = A;
			C.Card(Rc.X, Rc.Y - (1.f - A) * 10.f, Rc.W, Rc.H, &It.Card, O);
			for (int I = 0; I < 8; ++I)
			{
				const float Ang = I * 0.785f + Rc.X, Rad = (1.f - A) * 36.f;
				C.Box(Rc.X + Rc.W / 2.f + FMath::Cos(Ang) * Rad, Rc.Y + Rc.H / 2.f + FMath::Sin(Ang) * Rad, 2.5f, 2.5f, WithAlpha(FLinearColor::White, A));
			}
		}
	}
	for (const FTPPopText& T : G.Texts)
	{
		FString S;
		FLinearColor Col = FLinearColor::White;
		switch (T.Kind)
		{
		case FTPPopText::EKind::Hand:
			S = FString::Printf(TEXT("%s%s  %d×%d"), HandName(T.Hand), T.bFlush ? TEXT(" 플러시") : TEXT(""), T.A, T.B);
			Col = T.bFlush ? Pink : Gold;
			break;
		case FTPPopText::EKind::Straight:
			S = FString::Printf(TEXT("연쇄 스트레이트 %d  ×%d"), T.A, T.A);
			Col = Cyan;
			break;
		case FTPPopText::EKind::Score:
			S = TEXT("+") + Num(T.A) + (T.B > 1 ? FString::Printf(TEXT("   %d 연쇄 ×%d"), T.B, T.B) : FString());
			Col = T.B > 1 ? Green : FLinearColor::White;
			break;
		case FTPPopText::EKind::NoHoldMsg:
			S = TEXT("보관 금지 라운드");
			Col = Hex(TEXT("ff6a6a"));
			break;
		}
		C.Text(S, BX(T.X) + CellSize / 2.f, BY(T.Y) - (1.2f - T.T) * 30.f, WithAlpha(Col, FMath::Min(1.f, T.T * 2.f)), T.bBig ? 24.f : 15.f, 1, true);
	}

	// 왼쪽: 라운드
	C.Panel(30, 54, 400, 230);
	C.Text(FString::Printf(TEXT("ROUND %d / %d"), FMath::Min(G.Round + 1, NumRounds), NumRounds), 48, 88, Hex(TEXT("e8f0ff")), 22);
	C.Text(BossText(R.Boss), 48, 112, R.Boss != ETPBoss::None ? Red : Grey, 12);
	C.Text(TEXT("목표"), 48, 146, Grey, 12);
	C.Text(Num(R.Target), 48, 174, Gold, 26, 0, true);
	C.Text(TEXT("점수"), 230, 146, Grey, 12);
	C.Text(Num(G.Score), 230, 174, FLinearColor::White, 26, 0, true);
	C.Lines(FCanvas::RectPoints(48, 190, 364, 6), Hex(TEXT("7fd6ff"), 0.5f), 1.f);
	C.Box(48, 190, 364.f * FMath::Min(1.f, (float)G.Score / R.Target), 6, Gold);
	C.Text(FString::Printf(TEXT("남은 블록 %d / %d"), G.PiecesLeft, PiecesPerRound), 48, 224, Cyan, 14);
	C.Text(FString::Printf(TEXT("터뜨린 족보 %d   최대 연쇄 %d"), G.Bursts, G.MaxChain), 48, 248, Grey, 12);
	if (G.bHasBest) C.Text(FString::Printf(TEXT("최고 한 방 %s (%d 연쇄)"), *Num(G.Best.Total), G.Best.Chain), 48, 270, Grey, 11);

	C.Panel(30, 300, 400, 366);
	C.Text(TEXT("터지는 조건  (칩 × 배수)"), 48, 326, Cyan, 12);
	std::set<int> PvKinds;
	if (bShowPiece && !Pv.Sim.Steps.empty())
		for (ETPHandKind K : Pv.Sim.Steps[0].Kinds) PvKinds.insert((int)K);
	for (int I = 0; I < (int)ETPHandKind::Count; ++I)
	{
		const ETPHandKind K = (ETPHandKind)I;
		const float Y = 354.f + I * 36.f;
		const bool bOn = PvKinds.count(I) > 0;
		C.Text(HandName(K), 48, Y, bOn ? Gold : Fg, 14);
		C.Text(FString::Printf(TEXT("%d × %d"), HandDef(K).Chips, HandDef(K).Mult), 412, Y, bOn ? Gold : Grey, 13, 2);
		C.Text(HandCond(K), 48, Y + 16, Dim, 11);
	}
	C.Text(TEXT("원 페어는 터지지 않는다 (노란 선 = 하나만 더!)"), 48, 542, Gold, 11);
	C.Text(TEXT("점수 보너스"), 48, 570, Cyan, 12);
	C.Text(TEXT("플러시: 한 족보가 모두 같은 무늬면 배수 ×2"), 48, 592, Pink, 11);
	C.Text(TEXT("연쇄 스트레이트: 연쇄로 터진 숫자가 3단계+ 차례로"), 48, 612, Cyan, 11);
	C.Text(TEXT("  이어지면 (3-4-5) ×이어진 단계 수"), 48, 628, Cyan, 11);
	C.Text(TEXT("연쇄 n단계 ×n · 칩 = 기본 + 카드 숫자(A=11)"), 48, 652, Dim, 10);

	// 오른쪽
	const float RX = 850.f;
	C.Panel(RX, 54, 400, 170);
	C.Text(TEXT("이 자리에 떨어뜨리면"), RX + 18, 80, Cyan, 12);
	if (bShowPiece && !Pv.Sim.Steps.empty())
	{
		const int N = FMath::Min(4, (int)Pv.Sim.Steps.size());
		for (int I = 0; I < N; ++I)
		{
			const FTPSimStep& St = Pv.Sim.Steps[I];
			const FString Line = FString::Printf(TEXT("%d단계  %s%s  +%d"), St.Chain, *HandsJoined(St.Kinds, St.Flushes, TEXT(" + ")),
				St.Straight ? *FString::Printf(TEXT("  스트레이트×%d"), St.Straight) : TEXT(""), St.Score);
			C.Text(Line, RX + 18, 106.f + I * 20.f, St.Straight ? Cyan : St.Chain > 1 ? Green : Gold, 12);
		}
		C.Text(FString::Printf(TEXT("= %s점%s"), *Num(Pv.Sim.Total), Pv.Sim.Chain > 1 ? *FString::Printf(TEXT("  (%d 연쇄)"), Pv.Sim.Chain) : TEXT("")),
			RX + 18, 106.f + N * 20.f + 6.f, FLinearColor::White, 16, 0, true);
	}
	else if (G.Phase == ETPPhase::Play)
		C.Text(TEXT("터지는 족보 없음"), RX + 18, 106, Dim, 13);

	auto DrawMini = [&](const FTPPiece& P, float X0, float Y0) {
		const auto& Sh = ShapeCells(P.Type, 0);
		for (int K = 0; K < 2; ++K)
		{
			const FRectF Rc = RectOf({Sh[K * 2], Sh[K * 2 + 1]}, 20.f, X0, Y0);
			C.Card(Rc.X, Rc.Y, Rc.W, Rc.H, &G.Pool[P.Cards[K]], {});
		}
	};
	C.Panel(RX, 240, 400, 220);
	C.Text(TEXT("NEXT"), RX + 18, 266, Cyan, 12);
	for (int I = 0; I < FMath::Min(3, (int)G.Next.size()); ++I) DrawMini(G.Next[I], RX + 22 + I * 96.f, 290);
	C.Text(TEXT("HOLD"), RX + 18, 376, Cyan, 12);
	if (G.bHasHold) DrawMini(G.Hold, RX + 22, 390);

	C.Panel(RX, 476, 400, 190);
	C.Text(TEXT("마지막 연쇄"), RX + 18, 502, Cyan, 12);
	if (G.bHasLast)
	{
		C.Text(FString::Printf(TEXT("%d 연쇄  +%s"), G.Last.Chain, *Num(G.Last.Total)), RX + 18, 532, FLinearColor::White, 16, 0, true);
		std::vector<ETPHandKind> Ks(G.Last.Kinds.begin(), G.Last.Kinds.begin() + FMath::Min<size_t>(5, G.Last.Kinds.size()));
		std::vector<bool> Fs(G.Last.Flushes.begin(), G.Last.Flushes.begin() + Ks.size());
		C.Text(HandsJoined(Ks, Fs, TEXT(", ")), RX + 18, 558, Grey, 11);
	}
	C.Text(FString::Printf(TEXT("덱: A~7 × 4무늬 × %d벌 · 남은 카드 %d"), DeckCopies, (int)G.Deck.size()), RX + 18, 648, Dim, 11);
	C.Text(TEXT("방향키 좌우 이동   위/X 회전   Z 역회전   아래 빠르게   Space 떨어뜨리기   C 보관   P 일시정지   Enter 다음 라운드"), DesignW / 2.f,
		DesignH - 14.f, Dim, 11, 1);

	// 덮개 화면
	if (G.bPaused && G.Phase == ETPPhase::Play)
	{
		C.Box(0, 0, DesignW, DesignH, Hex(TEXT("06060b"), 0.8f));
		C.Text(TEXT("PAUSED"), DesignW / 2.f, 340, Cyan, 30, 1);
	}
	if (!G.bStarted)
		C.Overlay(TEXT("TETRIS POKER"), Cyan,
			{TEXT("카드 2장을 이어 붙인 4칸 블록이 떨어진다. 같은 숫자 3장 이상이 붙거나, 페어 두 쌍이 맞닿으면 터진다"),
				TEXT("같은 무늬로 터지면 플러시 ×2, 연쇄로 3-4-5처럼 터지면 연쇄 스트레이트 보너스"),
				TEXT("받침 없는 카드는 따로 떨어져 또 맞으면 연쇄! 연쇄할수록 배수가 커진다")},
			TEXT("Enter 키 또는 화면을 눌러 시작"));
	else if (G.Phase == ETPPhase::Clear)
	{
		FString NextLine = TEXT("마지막 라운드였다");
		if (G.Round + 1 < NumRounds)
		{
			const FTPRoundDef& NR = RoundDef(G.Round + 1);
			NextLine = FString::Printf(TEXT("다음: ROUND %d  목표 %s"), G.Round + 2, *Num(NR.Target));
			if (NR.Boss != ETPBoss::None) NextLine += FString(TEXT("  · ")) + BossText(NR.Boss);
		}
		C.Overlay(FString::Printf(TEXT("ROUND %d CLEAR"), G.Round + 1), Green,
			{FString::Printf(TEXT("점수 %s / 목표 %s"), *Num(G.Score), *Num(R.Target)), FString::Printf(TEXT("남은 블록 %d개 · 최대 연쇄 %d"), G.Bonus, G.MaxChain),
				NextLine},
			TEXT("Enter 키 또는 화면을 눌러 계속"));
	}
	else if (G.Phase == ETPPhase::Over)
	{
		const FString Why = G.OverReason == ETPOverReason::Ceiling
			? FString(TEXT("카드가 천장에 닿았다"))
			: FString::Printf(TEXT("블록 %d개를 다 썼지만 목표 점수에 못 미쳤다"), PiecesPerRound);
		C.Overlay(TEXT("GAME OVER"), Hex(TEXT("ff6a6a")),
			{Why, FString::Printf(TEXT("ROUND %d · 점수 %s / %s · 누적 %s"), G.Round + 1, *Num(G.Score), *Num(R.Target), *Num(G.TotalScore))},
			TEXT("R 키 또는 화면을 눌러 처음부터"));
	}
	else if (G.Phase == ETPPhase::Win)
		C.Overlay(TEXT("ALL CLEAR"), Gold,
			{FString::Printf(TEXT("8라운드를 모두 넘겼다 · 누적 %s"), *Num(G.TotalScore)),
				G.bHasBest ? FString::Printf(TEXT("최고 한 방 %s (%d 연쇄)"), *Num(G.Best.Total), G.Best.Chain) : FString()},
			TEXT("R 키 또는 화면을 눌러 다시"));
}
