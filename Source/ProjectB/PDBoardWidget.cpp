#include "PDBoardWidget.h"

#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

using namespace PD;

namespace PDDraw
{
constexpr float DesignW = 1280.f;
constexpr float DesignH = 720.f;
constexpr float CS = 54.f;          // 칸 크기
constexpr float FX = 24.f;          // 판 왼쪽 위
constexpr float FY = 24.f;
constexpr float HandY = FY + Rows * CS + 22.f;
constexpr float CardW = 66.f, CardH = 92.f;
constexpr float RX = FX + Cols * CS + 26.f; // 오른쪽 패널
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
	static const TCHAR* C[] = {TEXT("ff9ad5"), TEXT("ff6a6a"), TEXT("ffa86a"), TEXT("ff9ad5"), TEXT("7fd6ff"), TEXT("ffd96a"), TEXT("cfe4ff"), TEXT("cfd8e6")};
	return Hex(C[(int)H]);
}
const TCHAR* HandName(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("스트레이트 플러시"), TEXT("포카드"), TEXT("풀하우스"), TEXT("플러시"), TEXT("스트레이트"), TEXT("트리플"), TEXT("투 페어"), TEXT("원 페어")};
	return N[(int)H];
}
const TCHAR* HandEffect(EPDHand H)
{
	static const TCHAR* N[] = {TEXT("공격력 ×2 · 연사 ×2 · 범위"), TEXT("공격력 ×3.5"), TEXT("공격력 ×1.5 · 연사 ×1.5 · 범위"), TEXT("범위 공격 (장수만큼 넓게)"),
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
const TCHAR* JokerName(int J)
{
	static const TCHAR* N[] = {TEXT("하트 애호가"), TEXT("스페이드 애호가"), TEXT("다이아 애호가"), TEXT("클로버 애호가"), TEXT("연결고리"), TEXT("쌍둥이"), TEXT("일직선"),
		TEXT("넓은 물결"), TEXT("그림 카드"), TEXT("작은 수"), TEXT("행운의 7"), TEXT("큰 손"), TEXT("보급"), TEXT("재활용"), TEXT("조준경"), TEXT("서리"), TEXT("연쇄 폭발"), TEXT("관통탄")};
	return N[J];
}
const TCHAR* JokerDesc(int J)
{
	static const TCHAR* N[] = {TEXT("♥ 타워 공격력 +40%"), TEXT("♠ 타워 공격력 +40%"), TEXT("♦ 타워 공격력 +40%"), TEXT("♣ 타워 공격력 +40%"), TEXT("연쇄 보너스 +25% → +45%"),
		TEXT("원 페어·투 페어 ×1.5 → ×2"), TEXT("스트레이트 연사 한 장마다 +0.25 더"), TEXT("플러시 범위 +0.75칸"), TEXT("J·Q·K 타워 공격력 +60%"), TEXT("2~6 타워 연사 +60%"),
		TEXT("숫자 7 타워 공격력 ×2.5"), TEXT("손패 최대 +2"), TEXT("웨이브마다 카드 +1"), TEXT("카드를 버리면 1장 뽑기"), TEXT("모든 타워 사거리 +15%"),
		TEXT("둔화 더 강하게, ♣ 주변도 둔화"), TEXT("처치 시 주변 적에게 최대 체력 15% 피해"), TEXT("적의 방패를 무시")};
	return N[J];
}
const TCHAR* InstantName(int I)
{
	static const TCHAR* N[] = {TEXT("와일드 카드"), TEXT("보충"), TEXT("수리")};
	return N[I];
}
const TCHAR* InstantDesc(int I)
{
	static const TCHAR* N[] = {TEXT("아무 숫자·무늬로 치는 카드 1장"), TEXT("카드 2장 뽑기"), TEXT("목숨 +3")};
	return N[I];
}
// 보상 카드 위치 (판 가운데 3장)
FVector2f RewardPos(int I) { return FVector2f(FX + Cols * CS / 2.f - 360.f + I * 250.f, FY + 160.f); }
constexpr float RewardW = 220.f, RewardH = 170.f;
FVector2f CellPx(float X, float Y) { return FVector2f(FX + X * CS, FY + Y * CS); } // 칸 좌표(실수) → 화면
FVector2f CellCenter(int Key) { return CellPx(Key % Cols + 0.5f, Key / Cols + 0.5f); }
// 합성 버튼: 족보 맨 아래 칸(같으면 왼쪽)의 아래 변 가운데
constexpr float FuseW = 46.f, FuseH = 16.f;
FVector2f FuseButtonCenter(const FPDSynergy& S)
{
	int Bottom = S.Cells[0];
	for (int K : S.Cells) if (K / Cols > Bottom / Cols || (K / Cols == Bottom / Cols && K % Cols < Bottom % Cols)) Bottom = K;
	return CellPx(Bottom % Cols + 0.5f, Bottom / Cols + 1.f) - FVector2f(0.f, 4.f);
}
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
	void Text(const FString& S, float X, float Y, const FLinearColor& Col, float Px = 13.f, int32 Align = 0, bool bBold = false)
	{
		const FSlateFontInfo F = Font(Px, bBold);
		const FVector2f Sz = Measure(S, F);
		const float Left = Align == 1 ? X - Sz.X / 2.f : Align == 2 ? X - Sz.X : X;
		const float Top = Y - Sz.Y * 0.8f;
		FSlateDrawElement::MakeText(Out, NextLayer(), Geo.ToPaintGeometry(Sz, FSlateLayoutTransform(FVector2f(Left, Top))), S, F, ESlateDrawEffect::None, Col);
	}
	void TextCentered(const FString& S, float CX, float CY, const FLinearColor& Col, float Px, bool bBold)
	{
		const FSlateFontInfo F = Font(Px, bBold);
		const FVector2f Sz = Measure(S, F);
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
			const FLinearColor Col = SuitColor(E.Suit);
			Beams.Add({CellPx(E.X, E.Y), CellPx(E.X2, E.Y2), 0.f, 0.12f, E.A ? 2.6f : 1.6f, Col});
			Rings.Add({P, 4.f, 14.f, 0.f, 0.15f, 1.5f, Col});
			Burst(CellPx(E.X2, E.Y2), E.A ? 5 : 3, Col, 40.f, 140.f, 0.25f, 2.f, true);
			break;
		}
		case FPDEvent::EKind::Splash:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 6.f, E.R * CS, 0.f, 0.35f, 2.f, Col});
			Burst(P, 10, Col, 60.f, 200.f, 0.35f, 2.2f, true);
			break;
		}
		case FPDEvent::EKind::Kill:
		{
			const FLinearColor Col = EnemyColor(E.Enemy);
			const bool bBoss = EnemyDef(E.Enemy).bBoss, bTank = E.Enemy == EPDEnemy::Tank || E.Enemy == EPDEnemy::Armored;
			Burst(P, bBoss ? 70 : bTank ? 26 : 14, Col, 50.f, bBoss ? 320.f : 190.f, bBoss ? 1.0f : 0.5f, bBoss ? 3.2f : 2.4f);
			Burst(P, bBoss ? 30 : 6, FLinearColor::White, 80.f, 260.f, 0.3f, 1.6f, true);
			Rings.Add({P, 4.f, bBoss ? 120.f : bTank ? 40.f : 24.f, 0.f, bBoss ? 0.6f : 0.3f, bBoss ? 3.f : 1.8f, Col});
			if (bBoss) { Shake = FMath::Max(Shake, 0.6f); FlashT = 0.35f; FlashCol = Col; }
			else if (bTank) Shake = FMath::Max(Shake, 0.15f);
			break;
		}
		case FPDEvent::EKind::Leak:
			Shake = FMath::Max(Shake, 0.35f);
			FlashT = 0.4f;
			FlashCol = Hex(TEXT("ff3a4a"));
			Floats.Add({FString::Printf(TEXT("목숨 -%d"), E.A), FVector2f(FX + Cols * CS - 60.f, FY + 40.f), 0.f, 1.2f, 20.f, Hex(TEXT("ff6a6a"))});
			break;
		case FPDEvent::EKind::Place:
		{
			const FLinearColor Col = SuitColor(E.Suit);
			Rings.Add({P, 8.f, 46.f, 0.f, 0.4f, 2.f, Col});
			Burst(P, 14, Col, 40.f, 160.f, 0.45f, 2.2f);
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
			for (int I = 0; I < 6; ++I) Burst(FVector2f(FX + FMath::FRandRange(100.f, Cols * CS - 100.f), FY + FMath::FRandRange(80.f, Rows * CS - 80.f)), 40, HandColor((EPDHand)(I % 8)), 60.f, 300.f, 1.4f, 3.f, false, 120.f);
			FlashT = 0.5f;
			FlashCol = Hex(TEXT("ffd96a"));
			break;
		case FPDEvent::EKind::Discard: break;
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
			Floats.Add({FString::Printf(TEXT("레벨 %d!"), E.A), P - FVector2f(0.f, 30.f), 0.f, 1.5f, 28.f, Hex(TEXT("ffd96a"))});
			FlashT = FMath::Max(FlashT, 0.2f);
			FlashCol = Col;
			Shake = FMath::Max(Shake, 0.25f + 0.08f * E.A);
			break;
		}
		case FPDEvent::EKind::Reward:
		{
			const FString Name = E.A ? JokerName(E.B) : InstantName(E.B);
			Floats.Add({Name + TEXT("!"), FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 0.f, 1.4f, 26.f, Hex(TEXT("ffd96a"))});
			Burst(FVector2f(FX + Cols * CS / 2.f, FY + 90.f), 40, Hex(TEXT("ffd96a")), 60.f, 260.f, 0.8f, 2.6f, true);
			break;
		}
		}
	}
	Game.Events.clear();
}

void SPDBoardWidget::UpdateFx(float Dt)
{
	for (FParticle& Pa : Particles)
	{
		Pa.P += Pa.V * Dt;
		Pa.V *= FMath::Max(0.f, 1.f - Pa.Drag * Dt);
		Pa.V.Y += Pa.Gravity * Dt;
		Pa.Life -= Dt;
	}
	Particles.RemoveAll([](const FParticle& Pa) { return Pa.Life <= 0.f; });
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
	Game.Tick(Dt);
	HandleEvents();
	UpdateFx(Dt);
	if (Sel >= (int32)Game.Hand.size()) Sel = -1;
	UpdatePreview();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPDBoardWidget::DoFuse(int32 SynergyIdx)
{
	if (!Game.CanFuse(SynergyIdx)) return;
	PendingFuseCells.Reset();
	for (int K : Game.Synergies[SynergyIdx].Cells) PendingFuseCells.Add(CellCenter(K));
	Game.Fuse(SynergyIdx);
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
	const int32 CX = FMath::FloorToInt((Mouse.X - FX) / CS), CY = FMath::FloorToInt((Mouse.Y - FY) / CS);
	const bool bIn = CX >= 0 && CY >= 0 && CX < Cols && CY < Rows;
	HoverX = bIn ? CX : -1;
	HoverY = bIn ? CY : -1;
	return FReply::Handled();
}

void SPDBoardWidget::Click(FVector2f P, bool bRight)
{
	auto Sound = [this](EPDSound S) { if (!Game.bMuted) Game.Sounds.push_back({S, 0, 0, 1.f}); };
	if (Game.Phase == EPDPhase::Title) { Game.Phase = EPDPhase::Build; Sound(EPDSound::Start); return; }
	if (Game.Phase == EPDPhase::Over || Game.Phase == EPDPhase::Win) { Game.NewGame(true); Sel = -1; Particles.Empty(); Sound(EPDSound::Start); return; }
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < (int32)Game.Rewards.size(); ++I)
		{
			const FVector2f R = RewardPos(I);
			if (P.X >= R.X && P.X <= R.X + RewardW && P.Y >= R.Y && P.Y <= R.Y + RewardH) { Game.TakeReward(I); return; }
		}
		return;
	}
	// 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		{
			if (!Game.CanFuse(I)) continue;
			const FVector2f B = FuseButtonCenter(Game.Synergies[I]);
			if (FMath::Abs(P.X - B.X) <= FuseW / 2.f && FMath::Abs(P.Y - B.Y) <= FuseH / 2.f) { DoFuse(I); return; }
		}
	int32 HandHit = -1;
	for (int32 I = 0; I < (int32)Game.Hand.size(); ++I)
	{
		const float X = FX + 96.f + I * (CardW + 10.f);
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
	if (Game.Phase == EPDPhase::Build && P.X >= RX + 16.f && P.X <= RX + 216.f && P.Y >= FY + 96.f && P.Y <= FY + 134.f) Game.StartWave();
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
	if (Game.Phase == EPDPhase::Title)
	{
		if (K == EKeys::Enter || K == EKeys::SpaceBar) { Game.Phase = EPDPhase::Build; Sound(EPDSound::Start); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (Game.Phase == EPDPhase::Over || Game.Phase == EPDPhase::Win)
	{
		if (K == EKeys::R || K == EKeys::Enter) { Game.NewGame(true); Sel = -1; Particles.Empty(); Sound(EPDSound::Start); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < 3; ++I) if (K == Nums[I]) { Game.TakeReward(I); return FReply::Handled(); }
		if (K == EKeys::S) { Game.TakeReward(-1); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	for (int32 I = 0; I < 9; ++I)
		if (K == Nums[I])
		{
			if (I < (int32)Game.Hand.size()) { Sel = Sel == I ? -1 : I; Sound(EPDSound::Move); }
			return FReply::Handled();
		}
	if (K == EKeys::SpaceBar || K == EKeys::Enter) Game.StartWave();
	else if (K == EKeys::Escape) Sel = -1;
	else if (K == EKeys::G) DoFuse(HoveredSynergy());
	else if (K == EKeys::F) Game.Speed = Game.Speed == 1 ? 2 : Game.Speed == 2 ? 3 : 1;
	else if (K == EKeys::M) Game.bMuted = !Game.bMuted;
	else return FReply::Unhandled();
	return FReply::Handled();
}

int32 SPDBoardWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	FSlateDrawElement::MakeBox(OutDrawElements, LayerId, AllottedGeometry.ToPaintGeometry(), &WhiteBrush, ESlateDrawEffect::None, Hex(TEXT("06060b")));
	const auto LocalSize = AllottedGeometry.GetLocalSize(); // 엔진 버전에 따라 FVector2D 또는 FVector2f
	const float SizeX = (float)LocalSize.X, SizeY = (float)LocalSize.Y;
	const float Scale = FMath::Max(0.01f, FMath::Min(SizeX / DesignW, SizeY / DesignH));
	const FVector2f Offset((SizeX - DesignW * Scale) / 2.f, (SizeY - DesignH * Scale) / 2.f);
	ViewScale = Scale;
	ViewOffset = Offset;
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
		for (const FPDSynergy& S : List)
		{
			const FLinearColor Col = WithAlpha(HandColor(S.Kind), Alpha);
			TSet<int32> Has;
			for (int K : S.Cells) Has.Add(K);
			const float M = 4.f;
			for (int K : S.Cells)
			{
				const int X = K % Cols, Y = K / Cols;
				const bool bL = X == 0 || !Has.Contains(K - 1), bR = X == Cols - 1 || !Has.Contains(K + 1), bU = !Has.Contains(K - Cols), bD = !Has.Contains(K + Cols);
				const FVector2f P = CellPx((float)X, (float)Y);
				const float X0 = P.X + (bL ? M : 0.f), X1 = P.X + CS - (bR ? M : 0.f), Y0 = P.Y + (bU ? M : 0.f), Y1 = P.Y + CS - (bD ? M : 0.f);
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, 0.07f));
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
		F.Dashed(FCanvas::CirclePoints(P, TowerStats(Ghost, G.JokerMask()).Range * CS, 48), Hex(TEXT("7fd6ff"), 0.45f), 1.f, 4.f, 6.f);
		F.CardShape(Card, P.X, P.Y, CS * 0.4f, 0.8f, bPv, Pulse);
	}
	else if (HoverX >= 0 && Sel < 0)
	{
		auto It = G.Towers.find(CellKey(HoverX, HoverY));
		if (It != G.Towers.end()) F.Dashed(FCanvas::CirclePoints(CellCenter(It->first), TowerStats(It->second, G.JokerMask()).Range * CS, 48), Hex(TEXT("7fd6ff"), 0.45f), 1.f, 4.f, 6.f);
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
		const FVector2f P = CellPx(Pt.X, Pt.Y);
		const FLinearColor Col = EnemyColor(E.Type);
		const bool bBoss = EnemyDef(E.Type).bBoss;
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
		F.Box(P.X - W / 2.f, BY, W * FMath::Max(0.f, E.Hp / E.MaxHp), 3.f, Col);
		if (E.Armor > 0.f && !G.Has(EPDJoker::Pierce)) F.Box(P.X - W / 2.f, BY - 4.f, W, 2.f, Hex(TEXT("9fc3e8")));
	}

	// ---- 이펙트: 레이저, 충격파, 파티클
	for (const FBeam& B : Beams)
	{
		const float A = 1.f - B.T / B.MaxT;
		F.Glow({B.A, B.B}, WithAlpha(B.Col, A), B.Width, 1.4f);
		F.Lines({B.A, B.B}, WithAlpha(FLinearColor::White, A * 0.8f), B.Width * 0.4f);
	}
	for (const FRing& R : Rings)
	{
		const float U = R.T / R.MaxT, Rad = R.R0 + (R.R1 - R.R0) * (1.f - (1.f - U) * (1.f - U));
		F.Glow(FCanvas::CirclePoints(R.C, Rad, 36), WithAlpha(R.Col, 1.f - U), R.Width * (1.f - U * 0.5f), 1.f);
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
		for (const FPDSynergy& S : List)
		{
			int Top = S.Cells[0];
			for (int K : S.Cells) if (K / Cols < Top / Cols || (K / Cols == Top / Cols && K % Cols < Top % Cols)) Top = K;
			const FVector2f P = CellPx(Top % Cols + 0.5f, (float)(Top / Cols));
			F.Badge(HandLabel(S), P.X, P.Y + 4.f, HandColor(S.Kind), Alpha);
		}
	};
	DrawBadges(G.Synergies, bPv ? 0.35f : 1.f);
	if (bPv) DrawBadges(Preview, 1.f);
	// 합성 버튼
	if (!bPv && G.Phase != EPDPhase::Reward)
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
		{
			if (!G.CanFuse(I)) continue;
			const FVector2f B = FuseButtonCenter(G.Synergies[I]);
			const bool bHov = FMath::Abs(Mouse.X - B.X) <= FuseW / 2.f && FMath::Abs(Mouse.Y - B.Y) <= FuseH / 2.f;
			F.Box(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, Hex(TEXT("06060b"), 0.9f));
			F.Glow(FCanvas::RoundRectPoints(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, 6.f), Gold, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(TEXT("합성"), B.X, B.Y, WithAlpha(Gold, bHov ? 1.f : 0.8f), 10.f, true);
		}
	// 타워 정보 (마우스를 올리면)
	if (HoverX >= 0 && Sel < 0 && G.Phase != EPDPhase::Reward)
	{
		auto It = G.Towers.find(CellKey(HoverX, HoverY));
		if (It != G.Towers.end())
		{
			const FPDTower& T = It->second;
			const FPDTowerStats St = TowerStats(T, G.JokerMask());
			const FVector2f P = CellCenter(It->first) + FVector2f(CS * 0.6f, -CS * 0.6f);
			const float BX = FMath::Min(P.X, FX + Cols * CS - 170.f), BYp = FMath::Max(P.Y - 66.f, FY + 4.f);
			F.Box(BX, BYp, 166.f, 70.f, Hex(TEXT("06060b"), 0.92f));
			F.Lines(FCanvas::RectPoints(BX, BYp, 166.f, 70.f), WithAlpha(SuitColor(T.Card.Suit), 0.7f), 1.f);
			F.Text(FString::Printf(TEXT("%s %s · 레벨 %d"), T.Card.IsWild() ? TEXT("와일드") : *RankText(T.Card.Rank), SuitName(T.Card.Suit), T.Level), BX + 8.f, BYp + 18.f, SuitColor(T.Card.Suit), 12.f, 0, true);
			F.Text(FString::Printf(TEXT("공격력 %.0f · 초당 %.1f발"), St.Dmg, St.Rate), BX + 8.f, BYp + 36.f, Fg, 11.f);
			F.Text(FString::Printf(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *FString::Printf(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? TEXT(" · 둔화") : TEXT("")), BX + 8.f, BYp + 52.f, Grey, 11.f);
			if (T.Level >= 2) F.Text(FString::Printf(TEXT("특성 공격 ×%.1f 연사 ×%.1f"), T.TDmg, T.TRate), BX + 8.f, BYp + 66.f, Gold, 10.f);
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
		F.TextCentered(BannerText, FX + Cols * CS / 2.f - Sweep, CY - 8.f, WithAlpha(BannerCol, A), 40.f, true);
		F.TextCentered(BannerSub, FX + Cols * CS / 2.f + Sweep, CY + 24.f, WithAlpha(Fg, A), 15.f, false);
	}

	// ---- 손패
	C.Text(FString::Printf(TEXT("손패 %d/%d"), (int32)G.Hand.size(), G.HandMax()), FX, HandY + 12.f, Cyan, 12.f);
	C.Text(FString::Printf(TEXT("덱 %d장"), (int32)G.Deck.size()), FX, HandY + 30.f, Dim, 11.f);
	for (int32 I = 0; I < (int32)G.Hand.size(); ++I)
	{
		const FPDCard& Card = G.Hand[I];
		const bool bSel = I == Sel;
		const float X = FX + 96.f + I * (CardW + 10.f), Y = HandY - (bSel ? 10.f : 0.f);
		const FLinearColor Col = SuitColor(Card.Suit);
		C.Glow(FCanvas::RoundRectPoints(X, Y, CardW, CardH, 8.f), bSel ? FLinearColor::White : Col, bSel ? 2.4f : 1.4f, bSel ? 1.4f : 0.6f);
		C.CardShape(Card, X + CardW / 2.f, Y + CardH / 2.f - 6.f, 28.f, 1.f, bSel, Pulse, false);
		C.TextCentered(SuitName(Card.Suit), X + CardW / 2.f, Y + CardH - 12.f, Col, 10.f, false);
		C.Text(FString::FromInt(I + 1), X + 6.f, Y + 13.f, Dim, 10.f);
	}

	// ---- 오른쪽 패널
	const float PW = DesignW - RX - 20.f;
	C.Panel(RX, FY, PW, 150.f);
	C.Text(G.Phase == EPDPhase::Title ? FString(TEXT("WAVE -")) : FString::Printf(TEXT("WAVE %d"), G.Wave), RX + 16.f, FY + 30.f, Hex(TEXT("e8f0ff")), 20.f, 0, true);
	C.Text(FString::Printf(TEXT("목숨 %d"), G.Lives), RX + 16.f, FY + 58.f, G.Lives <= 5 ? Red : Green, 15.f, 0, true);
	C.Text(FString::Printf(TEXT("처치 %d · 놓친 적 %d · 최고 %d웨이브"), G.Kills, G.Leaks, FMath::Max(G.BestWave, G.Wave)), RX + 16.f, FY + 80.f, Grey, 11.f);
	if (G.Phase == EPDPhase::Build)
	{
		C.Glow(FCanvas::RectPoints(RX + 16.f, FY + 96.f, 200.f, 38.f), Green, 1.6f, 0.6f + 0.8f * Pulse);
		C.TextCentered(FString::Printf(TEXT("웨이브 %d 시작 (Space)"), G.Wave + 1), RX + 116.f, FY + 115.f, Green, 13.f, true);
	}
	else if (G.Phase == EPDPhase::Wave)
		C.Text(FString::Printf(TEXT("남은 적 %d   속도 ×%d (F)"), (int32)G.Enemies.size(), G.Speed), RX + 16.f, FY + 118.f, Gold, 12.f);

	C.Panel(RX, FY + 162.f, PW, 272.f);
	C.Text(TEXT("족보 시너지 (붙어 있으면 모양 상관없이)"), RX + 16.f, FY + 184.f, Cyan, 12.f);
	TSet<int32> PvKinds;
	for (const FPDSynergy& S : Preview) PvKinds.Add((int32)S.Kind);
	for (int32 I = 0; I < (int32)EPDHand::Count; ++I)
	{
		const EPDHand H = (EPDHand)I;
		const float Y = FY + 206.f + I * 26.f;
		const bool bOn = PvKinds.Contains(I);
		FString Name = HandName(H);
		if (H == EPDHand::Flush) Name += TEXT(" (3~5장)");
		if (H == EPDHand::Straight) Name += TEXT(" (2~5장)");
		C.Text(Name, RX + 16.f, Y, bOn ? Gold : HandColor(H), 12.f, 0, true);
		C.Text(HandEffect(H), RX + 16.f, Y + 12.f, bOn ? Gold : Dim, 10.f);
	}
	C.Text(FString::Printf(TEXT("연쇄: 족보끼리 맞닿으면 하나에 +%d%%씩"), G.Has(EPDJoker::ChainLink) ? 45 : 25), RX + 16.f, FY + 424.f, FLinearColor::White, 11.f, 0, true);

	C.Panel(RX, FY + 444.f, PW, 118.f);
	C.Text(FString::Printf(TEXT("조커 %d/%d"), (int32)G.Jokers.size(), JokerMax), RX + 16.f, FY + 466.f, Gold, 12.f, 0, true);
	if (G.Jokers.empty()) C.Text(TEXT("웨이브를 막으면 보상으로 고른다"), RX + 16.f, FY + 488.f, Dim, 11.f);
	for (int32 I = 0; I < (int32)G.Jokers.size(); ++I)
	{
		C.Text(JokerName((int)G.Jokers[I]), RX + 16.f, FY + 488.f + I * 17.f, Gold, 11.f, 0, true);
		C.Text(JokerDesc((int)G.Jokers[I]), RX + 110.f, FY + 488.f + I * 17.f, Grey, 10.f);
	}
	C.Panel(RX, FY + 572.f, PW, 42.f);
	for (int32 I = 0; I < 4; ++I)
	{
		const EPDSuit S = (EPDSuit)I;
		const float X = RX + 18.f + I * 82.f;
		C.Shape(S, X + 6.f, FY + 593.f, 7.f, SuitColor(S), 1.4f, 0.6f);
		C.Text(SuitName(S), X + 18.f, FY + 597.f, SuitColor(S), 11.f);
	}
	C.Text(TEXT("족보 아래 [합성] 또는 G: 타워 하나로 합쳐 레벨 업 · 카드 클릭(숫자키) → 칸 클릭으로 설치 · 타워 위에 놓으면 교체 · 오른쪽 클릭 버리기 · Esc 취소 · F 속도 · M 소리"), DesignW / 2.f, DesignH - 8.f, Dim, 11.f, 1);

	// ---- 보상 화면
	if (G.Phase == EPDPhase::Reward)
	{
		C.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("06060b"), 0.9f));
		C.TextCentered(FString::Printf(TEXT("웨이브 %d 보상 — 하나를 고른다"), G.Wave), FX + Cols * CS / 2.f, FY + 112.f, Gold, 22.f, true);
		for (int32 I = 0; I < (int32)G.Rewards.size(); ++I)
		{
			const FPDReward& R = G.Rewards[I];
			const FVector2f P = RewardPos(I);
			const bool bHov = Mouse.X >= P.X && Mouse.X <= P.X + RewardW && Mouse.Y >= P.Y && Mouse.Y <= P.Y + RewardH;
			const FLinearColor Col = R.bJoker ? Gold : Cyan;
			C.Box(P.X, P.Y, RewardW, RewardH, Hex(TEXT("0a0b14")));
			C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, RewardW, RewardH, 12.f), Col, bHov ? 2.6f : 1.6f, bHov ? 1.6f + 0.6f * Pulse : 0.8f);
			C.Text(R.bJoker ? TEXT("조커") : TEXT("즉시"), P.X + 16.f, P.Y + 26.f, Col, 11.f);
			C.Text(FString::FromInt(I + 1), P.X + RewardW - 16.f, P.Y + 26.f, Dim, 11.f, 2);
			C.TextCentered(R.bJoker ? JokerName(R.Id) : InstantName(R.Id), P.X + RewardW / 2.f, P.Y + 74.f, Col, 20.f, true);
			C.TextCentered(R.bJoker ? JokerDesc(R.Id) : InstantDesc(R.Id), P.X + RewardW / 2.f, P.Y + 108.f, Fg, 11.f, false);
		}
		C.TextCentered(TEXT("1 · 2 · 3 또는 클릭으로 고르기   S: 건너뛰기"), FX + Cols * CS / 2.f, FY + 372.f, Grey, 12.f, false);
		if ((int32)G.Jokers.size() >= JokerMax) C.TextCentered(TEXT("조커 칸이 가득 찼다 (5/5)"), FX + Cols * CS / 2.f, FY + 396.f, Dim, 11.f, false);
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
}
