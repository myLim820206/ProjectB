// 포커 디펜스 화면 그리기: 판, 적, 발사체, 이펙트, 버튼, 패널, 창 (화풍 docs/15-art-style.md)
#include "PDBoardDraw.h"
#include "PDTitleLogo.inl" // 시작 화면 간판 획 (D-440)
#include "PDLogo.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"
#include "Algo/Reverse.h"

#include "PDTreeText.inl" // 생성됨: 특급 노드 이름 (tools/tree/gen_tree.py)

namespace
{
FString TreeNodeNameKo(int Id)
{
	const FPDTreeNode& N = TreeNode(Id);
	switch (N.Kind)
	{
	case EPDNode::Start: return TEXT("시작");
	case EPDNode::Keystone: return TrF(TEXT("키스톤: %s"), KeystoneName(N.Fx[0].Param));
	case EPDNode::Joker: return TrF(TEXT("조커 강화: %s"), JokerName(N.Fx[0].Param));
	case EPDNode::Notable: return GPDTreeNames[Id] ? GPDTreeNames[Id] : TEXT("특급 노드");
	default: return N.Theme == 6 ? FString(TEXT("조커 갈림길")) : N.Theme == 5 ? FString(TEXT("살림")) : FString(TEXT("작은 노드"));
	}
}
FString TreeNodeName(int Id) { return TrS(TreeNodeNameKo(Id)); } // 언어팩 (D-159)
} // namespace

// 글자를 두 줄로 (D-239): 한 줄에 안 들어가면 가운데 가까운 띄어쓰기에서 나눈다
void SPDBoardWidget::Wrap2(FCanvas& C, const FString& InS, float X, float Y, const FLinearColor& Col, float Px, float MaxW)
{
	const FString S = TrS(InS);
	const FSlateFontInfo Fn = FCanvas::Font(Px, false);
	if (FCanvas::Measure(S, Fn).X <= MaxW) { C.Text(S, X, Y, Col, Px, 0, false, MaxW); return; }
	int32 Best = -1;
	for (int32 I = 0; I < S.Len(); ++I) if (S[I] == TEXT(' ') && (Best < 0 || FMath::Abs(I - S.Len() / 2) < FMath::Abs(Best - S.Len() / 2))) Best = I;
	if (Best < 0) { C.Text(S, X, Y, Col, Px, 0, false, MaxW); return; }
	C.Text(S.Left(Best), X, Y, Col, Px, 0, false, MaxW);
	C.Text(S.Mid(Best + 1), X, Y + Px + 4.f, Col, Px, 0, false, MaxW);
}

// 창 오른쪽 위 닫기 버튼 (도감·트리, D-119)
void SPDBoardWidget::PaintCloseButton(FCanvas& C, const FLinearColor& Col) const
{
	(void)Col; // 닫기는 어느 창이든 기본 하늘색 ⓧ (D-388, 버튼 색 규칙 D-381)
	const bool bHov = InRect(Mouse, CdxCloseX - 3.f, CdxCloseY - 3.f, CdxCloseW + 6.f, CdxCloseH + 6.f);
	C.CloseX(FVector2f(CdxCloseX + CdxCloseW / 2.f, CdxCloseY + CdxCloseH / 2.f), CdxCloseW / 2.f, bHov);
}

// 아틀라스 굽기 (D-140): 칸마다 타워 카드(레벨 틀·점 포함) / 조커를 지금 코드 그대로, 맥박은 가운데 값
int32 SPDBoardWidget::PaintAtlas(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const FVector2f TO = Atlas.IsValid() ? Atlas->TileOrigin : FVector2f(0.f, 0.f); // 조각 굽기: 이 조각 왼쪽 위만큼 밀어 그린다
	const FGeometry G2 = AllottedGeometry.MakeChild(FVector2f(AtlasTex / AtlasK, AtlasTex / AtlasK), FSlateLayoutTransform(AtlasK, -TO));
	int32 Layer = LayerId;
	FCanvas F{OutDrawElements, G2, &Layer, &WhiteBrush};
	F.ThickK = AtlasK / (Atlas.IsValid() ? Atlas->BakedScale : 1.f); // 화면에서 그리던 선과 같은 화면 픽셀 굵기
	const float R = CS * 0.4f;
	auto Vis = [&](FVector2f C) { const FVector2f A = (C - FVector2f(AtlasCell, AtlasCell) * 0.5f) * AtlasK - TO, B = A + FVector2f(AtlasCell, AtlasCell) * AtlasK; return B.X > 0.f && B.Y > 0.f && A.X < BakeTile && A.Y < BakeTile; }; // 이 조각에 걸치는 칸만
	if (Atlas.IsValid() && Atlas->BakePage == 1) // 둘째 쪽 (D-338): 카드병정 적만
	{
		for (int32 Sh = 0; Sh < SoldierShapes; ++Sh) // 카드병정 적 (D-175): 흰색, 걷기 12프레임
			for (int32 Fr = 0; Fr < SoldierFrames; ++Fr)
			{
				const FVector2f P = AtlasCellCenter(AtlasSoldier0 + Sh * SoldierFrames + Fr);
				if (Vis(P)) F.CardSoldier(Sh, P + FVector2f(0.f, SoldierFoot), Sh >= 7 ? SoldierBossBakeR : SoldierBakeR, Fr / (float)SoldierFrames, FLinearColor::White);
			}
		return Layer;
	}
	GPDBakeWhiteSuits = true; // 카드 칸은 흰색으로 굽고 그릴 때 카드 색 설정에 맞춰 물들인다 (D-154)
	ON_SCOPE_EXIT { GPDBakeWhiteSuits = false; };
	for (int32 Ci = 0; Ci < AtlasCards; ++Ci)
		{
			const FPDCard Card = Ci == 52 ? FPDCard{0, EPDSuit::Wild} : FPDCard{2 + Ci / 4, (EPDSuit)(Ci % 4)};
			for (int32 Lv = 1; Lv <= 4; ++Lv)
				for (int32 St = 0; St < 1; ++St) // 족보 타워(채움)는 그릴 때 덧그린다 (D-176)
				{
					const FVector2f P = AtlasCellCenter(TowerSprite(Card, Lv, false));
					if (!Vis(P)) continue;
					F.CardShape(Card, P.X, P.Y, R, 1.f, false, 0.5f);
					for (int L = 2; L <= Lv; ++L)
					{
						const float H = R * 0.98f + (L - 1) * 3.5f;
						F.Glow(FCanvas::RoundRectPoints(P.X - H, P.Y - H, H * 2.f, H * 2.f, 6.f + L), WithAlpha(SuitColor(Card.Suit), 0.75f), 1.2f, 0.6f);
					}
					// 레벨 점은 물들이면 안 돼서 굽지 않고 그릴 때 따로 (D-154)
				}
		}
	GPDBakeWhiteSuits = false;
	for (int32 K = 0; K < 10; ++K)
	{
		const FVector2f P = AtlasCellCenter(AtlasJoker0 + K);
		if (Vis(P)) F.JokerTower(K, P.X, P.Y, R, 1.f, 0.5f);
	}
	{ // 둔화 눈송이 (D-178): 6갈래 + 가지, 흰색 (그릴 때 얼음색으로)
		const FVector2f P = AtlasCellCenter(AtlasSnow);
		if (Vis(P))
			for (int32 K = 0; K < 6; ++K)
			{
				const FVector2f D = Rot2(FVector2f(0.f, -1.f), K * TwoPi / 6.f), N(-D.Y, D.X);
				F.Glow({P, P + D * 30.f}, FLinearColor::White, 1.6f, 0.8f);
				for (const float B : {0.45f, 0.72f}) { const FVector2f Q = P + D * 30.f * B; const float L = B < 0.6f ? 9.f : 6.f; F.Glow({Q + (D - N) * L * 0.7f, Q, Q + (D + N) * L * 0.7f}, FLinearColor::White, 1.4f, 0.6f); }
			}
	}
	{ // 방어력 방패 (D-186): 윤곽 + 가운데 세로줄, 흰색 (그릴 때 방어력 색으로)
		const FVector2f P = AtlasCellCenter(AtlasShield);
		if (Vis(P))
		{
			F.Glow(ShieldPoints(P, ShieldBakeS), FLinearColor::White, 2.2f, 0.8f);
			F.Glow({P + FVector2f(0.f, -0.75f * ShieldBakeS), P + FVector2f(0.f, 0.92f * ShieldBakeS)}, FLinearColor::White, 1.6f, 0.6f);
		}
	}
	{ // 마우스 아이콘 (D-373): 몸통 · 오른쪽 버튼을 따로 흰색으로 (그릴 때 몸통은 회색, 버튼은 금/빨강)
		const FVector2f P = AtlasCellCenter(AtlasMouse);
		if (Vis(P)) for (const TArray<FVector2f>& L : MouseBodyLines(P, MouseBakeH)) F.Glow(L, FLinearColor::White, 2.2f, 0.7f);
		const FVector2f Q = AtlasCellCenter(AtlasMouseR);
		if (Vis(Q)) F.Glow(MouseRightPoints(Q, MouseBakeH), FLinearColor::White, 3.f, 1.f);
	}
	// 카드병정은 둘째 쪽에서 굽는다 (D-338)
	// 이펙트 모양 (D-141): 흰색, 반지름 AtlasFxR, 0도에서 시작 (그릴 때 색·크기·회전)
	const FLinearColor W = FLinearColor::White;
	const float RR = AtlasFxR, Th = AtlasFxTh;
	auto At = [&](int32 I) { return AtlasCellCenter(AtlasFx0 + I); };
	{ const FVector2f C = At(0); F.Glow(FCanvas::CirclePoints(C, RR, 36), W, Th, 1.f); }
	for (int32 N = 3; N <= 8; ++N)
	{
		const FVector2f C = At(FxPoly(N));
		TArray<FVector2f> P;
		for (int32 K = 0; K <= N; ++K) P.Add(C + Rot2(FVector2f(RR, 0.f), K * TwoPi / N));
		F.Glow(P, W, Th, 1.f);
		const FVector2f C2 = At(FxStar(N));
		TArray<FVector2f> S;
		for (int32 K = 0; K <= N * 2; ++K) S.Add(C2 + Rot2(FVector2f(K % 2 ? RR * 0.45f : RR, 0.f), K * PI / N));
		F.Glow(S, W, Th, 1.f);
	}
	for (int32 N = 2; N <= 16; ++N)
	{
		const FVector2f C = At(FxBroken(N));
		for (int32 K = 0; K < N; ++K) F.Glow(ArcPts(C, RR, K * TwoPi / N, PI / N * 0.55f, 6), W, Th, 1.f);
		const FVector2f C2 = At(FxRays(N));
		for (int32 K = 0; K < N; ++K) { const FVector2f D = Rot2(FVector2f(1.f, 0.f), K * TwoPi / N); F.Glow({C2 + D * RR * 0.6f, C2 + D * RR}, W, Th, 1.f); }
	}
	for (int32 S = 0; S <= 4; ++S) { const FVector2f C = At(FxSuit(S)); F.Shape((EPDSuit)S, C.X, C.Y, RR, W, Th, 1.f); }
	{ // 빗금 칸 (D-143): 칸 왼쪽 위 기준 x + y 가 HatchStep 의 배수인 45도 선, 흰색
		const FVector2f C = AtlasCellCenter(AtlasHatch), O = C - FVector2f(CS / 2.f, CS / 2.f);
		for (float D = HatchStep; D < CS * 2.f; D += HatchStep)
		{
			const float Ax = FMath::Max(0.f, D - CS), Bx = FMath::Min(CS, D);
			F.Lines({O + FVector2f(Ax, D - Ax), O + FVector2f(Bx, D - Bx)}, W, 1.f);
		}
	}
	{ // 합성 가능 꺾쇠 칸: 네 모서리
		const FVector2f C0 = AtlasCellCenter(AtlasBracket) - FVector2f(CS / 2.f, CS / 2.f);
		const float A = 3.f, B = CS - 3.f, L = 10.f;
		F.Glow({C0 + FVector2f(A, A + L), C0 + FVector2f(A, A), C0 + FVector2f(A + L, A)}, W, 1.8f, 1.f);
		F.Glow({C0 + FVector2f(B - L, A), C0 + FVector2f(B, A), C0 + FVector2f(B, A + L)}, W, 1.8f, 1.f);
		F.Glow({C0 + FVector2f(B, B - L), C0 + FVector2f(B, B), C0 + FVector2f(B - L, B)}, W, 1.8f, 1.f);
		F.Glow({C0 + FVector2f(A + L, B), C0 + FVector2f(A, B), C0 + FVector2f(A, B - L)}, W, 1.8f, 1.f);
	}
	{ // 선 스프라이트 띠 (D-146): 굵기마다 한 줄. Slate 선 셰이더와 같은 덮임: 너비 T+2 를 -1..1 로 두고 smoothstep(1, 안쪽, |u|)
		const FVector2f C = AtlasCellCenter(AtlasLineAA);
		const int32 NRows = (int32)(LineBandH * AtlasK);
		for (int32 Bi = 0; Bi < LineBands; ++Bi)
		{
			const float Inside = (LineBandT[Bi] * 0.5f - 1.f) / (LineBandT[Bi] * 0.5f + 1.f);
			const float Top = C.Y - LineAAH / 2.f + Bi * LineBandH;
			for (int32 Y = 0; Y < NRows; ++Y)
			{
				const float U = FMath::Abs((Y + 0.5f) / NRows * 2.f - 1.f);
				const float S = FMath::Clamp((U - 1.f) / (Inside - 1.f), 0.f, 1.f);
				const float A = S * S * (3.f - 2.f * S);
				if (A > 0.002f) F.Box(C.X - LineAAW / 2.f, Top + Y / AtlasK, LineAAW, 1.f / AtlasK, WithAlpha(W, A));
			}
		}
	}
	{ // 번짐 띠 (D-147): 네온 선 번짐 두 겹 (굵기 6.5T 알파 9%, 3.2T 알파 14%) 을 Slate 선 덮임 그대로 겹친 값. 띠 너비 = 6.5T + 2
		const FVector2f C = AtlasCellCenter(AtlasHalo);
		const int32 NRows = (int32)(LineBandH * AtlasK);
		auto Cover = [](float D, float T) { // Slate 선 셰이더 덮임: 굵기 T, 가운데에서 D px
			const float H = T * 0.5f + 1.f, G = D / H, In = (T * 0.5f - 1.f) / H;
			const float S = FMath::Clamp((G - 1.f) / (In - 1.f), 0.f, 1.f);
			return S * S * (3.f - 2.f * S);
		};
		for (int32 Bi = 0; Bi < HaloBands; ++Bi)
		{
			const float T = HaloBandT[Bi], Half = T * 6.5f * 0.5f + 1.f;
			const float Top = C.Y - LineAAH / 2.f + Bi * LineBandH;
			for (int32 Y = 0; Y < NRows; ++Y)
			{
				const float D = FMath::Abs((Y + 0.5f) / NRows * 2.f - 1.f) * Half;
				const float A = 1.f - (1.f - 0.09f * Cover(D, T * 6.5f)) * (1.f - 0.14f * Cover(D, T * 3.2f));
				if (A > 0.0005f) F.Box(C.X - LineAAW / 2.f, Top + Y / AtlasK, LineAAW, 1.f / AtlasK, WithAlpha(W, FMath::Min(1.f, A * HaloGain)));
			}
		}
	}
	{ // 가로 빛선 (조각·입자 꼬리)
		const FVector2f C = AtlasCellCenter(AtlasLine);
		F.Glow({C - FVector2f(LineHalf, 0.f), C + FVector2f(LineHalf, 0.f)}, W, 1.5f, 0.9f);
	}
	for (int32 Sp = 0; Sp <= 8; Sp += 2) // 별빛: 빛 덩어리 + 갈래 (U = 0, A = 1 일 때 모양)
	{
		const FVector2f C = At(FxFlare(Sp));
		const float Sz = RR, GA = Sp == 0 ? 0.13f : 0.07f;
		for (int32 J = 1; J <= 3; ++J) { const float H = Sz * (Sp == 0 ? 0.3f : 0.25f) * J; F.Box(C.X - H, C.Y - H, H * 2.f, H * 2.f, WithAlpha(W, GA)); }
		if (Sp == 2) F.Glow({C - FVector2f(Sz, 0.f), C + FVector2f(Sz, 0.f)}, W, 1.8f, 1.2f);
		else
			for (int32 J = 0; J < Sp; ++J)
			{
				const bool bShort = (Sp == 4 || Sp >= 6) && (J % 2);
				const FVector2f D = Rot2(FVector2f(1.f, 0.f), J * TwoPi / Sp) * Sz * (bShort ? 0.55f : 1.f);
				F.Glow({C - D * 0.12f, C + D}, W, 1.8f, 1.f);
			}
	}
	return Layer;
}

// 판 배경 굽기 (D-143): 격자·길 점선·판 테두리. 그림 좌표는 판 왼쪽 위에서 BgPad 만큼 바깥이 원점
int32 SPDBoardWidget::PaintBoardBg(const FGeometry& AllottedGeometry, FSlateWindowElementList& OutDrawElements, int32 LayerId) const
{
	const FVector2f TO = Atlas.IsValid() ? Atlas->TileOrigin : FVector2f(0.f, 0.f);
	const FGeometry G2 = AllottedGeometry.MakeChild(FVector2f(BgW, BgH), FSlateLayoutTransform(AtlasK, FVector2f(-(FX - BgPad) * AtlasK, -(FY - BgPad) * AtlasK) - TO));
	int32 Layer = LayerId;
	FCanvas F{OutDrawElements, G2, &Layer, &WhiteBrush};
	F.ThickK = AtlasK / (Atlas.IsValid() ? Atlas->BakedScale : 1.f);
	const FPDGame& G = Game;
	F.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("000000"))); // 판 바닥 = 검정 (D-455: 사용자 참고 그림 색표, 길 띠는 그 위 #060a0d)
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			const FVector2f P = CellPx((float)X, (float)Y);
			if (G.Path.IsPath(X, Y)) F.Box(P.X, P.Y, CS, CS, Hex(TEXT("7fd6ff"), 0.05f));
			else F.Lines(FCanvas::RectPoints(P.X + 0.5f, P.Y + 0.5f, CS - 1.f, CS - 1.f), Hex(TEXT("788cb4"), 0.04f), 1.f); // 격자는 거의 안 보이게 (D-455)
		}
	TArray<FVector2f> PathPts;
	for (const FPDPoint& Pt : G.Path.Points) PathPts.Add(CellPx(Pt.X, Pt.Y));
	F.Dashed(PathPts, Hex(TEXT("7fd6ff"), 0.35f), 2.f);
	// D-461: 시안 판 틀은 심 477379 에 안팎 5 ~ 10 넓게 번진다 (족보 메뉴판 테와 같은 세기, D-460)
	F.Glow(FCanvas::RoundRectPoints(FX, FY, Cols * CS, Rows * CS, 8.f), Hex(TEXT("3a8290")), 5.f, 1.f);
	F.Lines(FCanvas::RoundRectPoints(FX, FY, Cols * CS, Rows * CS, 8.f), Hex(TEXT("4f7e86")), 1.2f); // D-455: 참고 그림의 차분한 청록 틀 (예전 46c8e6) // 하늘빛 틀을 판에 붙여 (Alice 시안 D-410: 262 ~ 1030 × 86 ~ 566)
	return Layer;
}

int32 SPDBoardWidget::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	// 바탕은 칠하지 않는다: 뒤의 3D 월드(검은 바탕 + 나이아가라 + 블룸)가 비쳐 보여야 한다 (D-092, 월드 이펙트 층)
	const auto LocalSize = AllottedGeometry.GetLocalSize(); // 엔진 버전에 따라 FVector2D 또는 FVector2f
	const float SizeX = (float)LocalSize.X, SizeY = (float)LocalSize.Y;
	const float Scale = FMath::Max(0.01f, FMath::Min(SizeX / DesignW, SizeY / DesignH));
	const FVector2f Offset((SizeX - DesignW * Scale) / 2.f, (SizeY - DesignH * Scale) / 2.f);
	ViewScale = Scale;
	ViewOffset = Offset;
	ViewSize = FVector2f(SizeX, SizeY);
	const FGeometry Design = AllottedGeometry.MakeChild(FVector2f(DesignW, DesignH), FSlateLayoutTransform(Scale, Offset));
	// 판은 흔들림을 받고, 패널은 고정
	const FGeometry Field = Design.MakeChild(FVector2f(DesignW, DesignH), FSlateLayoutTransform(ShakeOff)); // 흔들림은 틱에서 정함
	int32 Layer = LayerId;
	FCanvas C{OutDrawElements, Design, &Layer, &WhiteBrush};
	FCanvas F{OutDrawElements, Field, &Layer, &WhiteBrush};
	if (UseAtlas()) // 모든 선을 선 스프라이트로 (D-146, F6 으로 끄면 진짜 선)
	{
		// 굵기별 띠 붓: 전체 띠 붓을 줄마다 나눈다 (반 텍셀 안쪽으로: 옆 띠가 섞이지 않게)
		static FSlateBrush Bands[LineBands + HaloBands]; // 선 띠 뒤에 번짐 띠 (D-147)
		const FBox2f R = Atlas->LineAABrush.GetUVRegion();
		const float Hv = (R.Max.Y - R.Min.Y) / LineBands, Pad = 0.5f / AtlasTex;
		for (int32 Bi = 0; Bi < LineBands; ++Bi)
		{
			Bands[Bi] = Atlas->LineAABrush;
			Bands[Bi].ImageSize.Y = Atlas->LineAABrush.ImageSize.Y / LineBands;
			Bands[Bi].SetUVRegion(FBox2f(FVector2f(R.Min.X, R.Min.Y + Hv * Bi + Pad), FVector2f(R.Max.X, R.Min.Y + Hv * (Bi + 1) - Pad)));
		}
		const FVector2f Shift = (AtlasCellCenter(AtlasHalo) - AtlasCellCenter(AtlasLineAA)) * AtlasK / (float)AtlasTex; // 번짐 칸은 선 칸과 같은 자리 배치
		for (int32 Bi = 0; Bi < HaloBands; ++Bi)
		{
			FSlateBrush& B = Bands[LineBands + Bi];
			B = Bands[Bi];
			const FBox2f U = Bands[Bi].GetUVRegion();
			B.SetUVRegion(FBox2f(U.Min + Shift, U.Max + Shift));
		}
		C.LineAA = F.LineAA = Bands;
		C.PxToDesign = F.PxToDesign = 1.f / FMath::Max(0.01f, Design.GetAccumulatedLayoutTransform().GetScale()); // DPI 배율까지 포함한 화면 px
	}
	const double T0 = FPlatformTime::Seconds();
	GPDDebug.Lines = GPDDebug.Points = GPDDebug.Boxes = 0;
	// 레이아웃 검사 (D-365, F2): 켜 두면 0.5초마다 한 프레임의 글자 · 상자 · 선 자리를 모은다
	GPDLayout.bCap = GPDLayout.bOn && T0 >= GPDLayout.NextAt;
	if (GPDLayout.bCap) { GPDLayout.Begin(); GPDLayout.NextAt = T0 + 0.5; }
	C.bRec = F.bRec = GPDLayout.bCap;
	C.bObjRec = true; // 그림 겹침은 고정 패널 · 창(C)에서만 (D-366): 판(F)의 타워는 칸에 맞춰 놓이고, 적 · 이펙트 · 말풍선은 원래 겹친다
	PaintAll(C, F);
	if (GPDLayout.bOn) LayoutCheckEnd(C);
	GPDDebug.PaintMs = FMath::Lerp(GPDDebug.PaintMs, (float)((FPlatformTime::Seconds() - T0) * 1000.0), 0.1f);
	return Layer;
}

void SPDBoardWidget::PaintAll(FCanvas& C, FCanvas& F) const
{
	// 위력 정보 (D-119): 패널과 타워 정보가 같이 쓴다. 판이 바뀔 때만 다시 계산 (D-134: 큰 판에선 계산 한 번에 수십 ms)
	{
		uint64 Sig = 1469598103934665603ull;
		auto Mix = [&](uint64 V) { Sig = (Sig ^ V) * 1099511628211ull; };
		for (const auto& KV : Game.Towers)
		{
			const FPDTower& T = KV.second;
			Mix((uint64)KV.first); Mix((uint64)T.Card.Rank << 8 | (uint64)T.Card.Suit); Mix((uint64)T.Level << 16 | (uint64)(T.Joker + 1) << 8 | (uint64)T.JSuit);
			Mix((uint64)T.JLevel); Mix((uint64)FMath::RoundToInt(T.Mult * 1000.f)); Mix((uint64)FMath::RoundToInt(T.TDmg * 1000.f + T.TRate * 7.f + T.TRange * 13.f + T.TSplash * 17.f));
		}
		Mix(Game.CharmMask()); Mix((uint64)Game.Points); Mix(Game.Mods.Keys); Mix(Game.Mods.JokerUp);
		for (int32 U : Game.Upg) Mix((uint64)U);
		static uint64 CachedSig = 0;
		if (Sig != CachedSig || PowerList.empty() != Game.Towers.empty())
		{
			CachedSig = Sig;
			PowerList = Game.TowerPowers();
			PowerSum = 0.f;
			for (const auto& KV : PowerList) PowerSum += KV.second;
			PowerGain = Game.SynergyGain();
		}
	}
	// 판 쪽(F, 흔들림을 받음): 판·적 → 발사체 → 이펙트 → 판 위 UI. 그 위에 고정 패널과 창(C)
	int32 L0 = GPDDebug.Lines, Q0 = GPDDebug.Boxes;
	double PT = FPlatformTime::Seconds();
	static double PartMs[7] = {}; static int32 PartQ[7] = {}; static int32 PartFrames = 0; // 부분별 시간·띠 수 (D-147 측정, 로그 PDPART)
	auto Part = [&](int32 I) {
		GPDDebug.PartLines[I] = GPDDebug.Lines - L0; L0 = GPDDebug.Lines;
		const double Now = FPlatformTime::Seconds(); PartMs[I] += (Now - PT) * 1000.0; PT = Now;
		PartQ[I] += GPDDebug.Boxes - Q0; Q0 = GPDDebug.Boxes;
	};
	ON_SCOPE_EXIT {
		const double Now = FPlatformTime::Seconds(); PartMs[6] += (Now - PT) * 1000.0; PartQ[6] += GPDDebug.Boxes - Q0;
		if (++PartFrames >= 120)
		{
			UE_LOG(LogTemp, Log, TEXT("PDPART ms=%.2f/%.2f/%.2f/%.2f/%.2f/%.2f/%.2f quads=%d/%d/%d/%d/%d/%d/%d"), PartMs[0] / PartFrames, PartMs[1] / PartFrames, PartMs[2] / PartFrames, PartMs[3] / PartFrames,
				PartMs[4] / PartFrames, PartMs[5] / PartFrames, PartMs[6] / PartFrames, PartQ[0] / PartFrames, PartQ[1] / PartFrames, PartQ[2] / PartFrames, PartQ[3] / PartFrames, PartQ[4] / PartFrames, PartQ[5] / PartFrames, PartQ[6] / PartFrames);
			for (int32 I = 0; I < 7; ++I) { PartMs[I] = 0.0; PartQ[I] = 0; }
			PartFrames = 0;
		}
	};
#if !UE_BUILD_SHIPPING
	static const bool bNoFx = FParse::Param(FCommandLine::Get(), TEXT("pdnofx")), bNoDraw = FParse::Param(FCommandLine::Get(), TEXT("pdnodraw")); // 부하 나누어 보기 (D-138)
	if (bNoDraw) return;
#else
	const bool bNoFx = false;
#endif
	if (Game.Phase != EPDPhase::Title) C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("020916"))); // D-455: 사용자 참고 그림 색 (예전 06091a) // Alice 시안 (D-413): 벽돌 벽 · 바닥선 없이 거의 검은 남색 바탕 (예전 PaintBarBack, D-274)
	PaintField(F, true);
	PaintField(F, false); Part(0);
	PaintShots(F); Part(1);
	if (!bNoFx) PaintEffects(F);
	Part(2);
	PaintPanels(C); Part(4);
	PaintBoardUi(F); Part(3); // 판 위 UI 를 패널 뒤에 그려 툴팁이 패널 위로 나갈 수 있게 (D-150)
	PaintHandTip(C); // 손패 툴팁은 판 위 UI 위에 (D-214)
	PaintFuseCursorTag(C); // 합성 모드 커서 꼬리표 (D-373)
	PaintHint(C); // 도움말 카드 (D-217)
	PaintLevelUp(C); // 레벨업 (D-273)
	PaintCine(C); // 완성 연출 (D-314): 판 · 패널 위, 창 아래
	PaintAchToast(C); // 도전 과제 알림 (B8)
	PaintScreens(C); Part(5);
	PaintTree(C);
	// 화면 번쩍임과 덮개 화면 (제목, 게임 오버), 메뉴
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Red = Hex(TEXT("ff6a6a"));
	if (FlashT > 0.f) C.Box(0.f, 0.f, DesignW, DesignH, WithAlpha(FlashCol, FlashT * 0.45f));
	if (bPaused && !bMenu && !bSettings && !bTree && !bUpg && !bCodex) // 일시정지 (D-169): 판을 어둡게 + 큰 글자
	{
		const float Pulse2 = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
		C.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("06060b"), 0.55f));
		C.TextCentered(TEXT("일시정지"), FX + Cols * CS / 2.f, FY + Rows * CS / 2.f - 14.f, WithAlpha(UiTitle, 0.75f + 0.25f * Pulse2), 40.f, true, Cols * CS - 40.f);
		C.TextCentered(TEXT("스페이스·P 또는 버튼: 계속 · 멈춘 동안에도 카드 놓기·합성은 된다"), FX + Cols * CS / 2.f, FY + Rows * CS / 2.f + 26.f, Hex(TEXT("cfd8e6")), 13.f, false, Cols * CS - 40.f);
	}
	if (G.Phase == EPDPhase::Title && LoadT <= 0.f) { PaintTitle(C); if (bModeSel) PaintModeSel(C); if (bUpg) PaintUpgrades(C); if (bUnlock) PaintCodexBook(C); } // 잠금 해제도 책자 (D-356)
	else if (G.Phase == EPDPhase::Clear) // 클리어 (D-229 → D-293): 'CHEERS!' 간판 + 잔 부딪치기 불꽃 + 영수증 + [시작 화면으로]
	{
		static const TCHAR* ModeNames[(int32)EPDMode::Count] = {TEXT("30 웨이브"), TEXT("60 웨이브"), TEXT("100 웨이브"), TEXT("무한")};
		const FLinearColor GoldC = Hex(TEXT("ffd26e")), PinkC = Hex(TEXT("ff6aaa")), CyanC = Hex(TEXT("6edcff")), FgC = Hex(TEXT("cfd8e6"));
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.88f));
		BulbSign(C, DesignW / 2.f, 50.f, 400.f, 90.f, TEXT("CHEERS!"), GoldC, 44.f, Clock);
		for (const float S : {-1.f, 1.f}) // 잔 부딪치기
		{
			const float CX0 = DesignW / 2.f + S * 40.f, A = FMath::DegreesToRadians(-14.f * S);
			auto R = [&](float Px, float Py) { return FVector2f(CX0 + Px * FMath::Cos(A) - Py * FMath::Sin(A), 230.f + Px * FMath::Sin(A) + Py * FMath::Cos(A)); };
			const FLinearColor Gc = S < 0.f ? PinkC : CyanC;
			C.Glow({R(-22.f, -40.f), R(22.f, -40.f), R(0.f, -6.f), R(-22.f, -40.f)}, Gc, 1.8f, 0.6f);
			C.Glow({R(0.f, -6.f), R(0.f, 22.f)}, Gc, 1.6f, 0.5f);
			C.Glow({R(-12.f, 24.f), R(12.f, 24.f)}, Gc, 1.6f, 0.5f);
		}
		FRandomStream Rs(7);
		const float Pop = 0.6f + 0.4f * FMath::Sin(Clock * 6.f);
		for (int32 K = 0; K < 14; ++K) { const float A = Rs.FRandRange(-PI, 0.f), R1 = Rs.FRandRange(8.f, 14.f), R2 = Rs.FRandRange(24.f, 46.f) * Pop; C.Glow({FVector2f(DesignW / 2.f + FMath::Cos(A) * R1, 188.f + FMath::Sin(A) * R1), FVector2f(DesignW / 2.f + FMath::Cos(A) * R2, 188.f + FMath::Sin(A) * R2)}, GoldC, 1.3f, 0.6f); }
		C.TextCentered(TrF(TEXT("%s 모드 클리어 · 앨리스의 축배"), Tr(ModeNames[(int32)G.Mode])), DesignW / 2.f, 282.f, FgC, 15.f, true);
		TArray<TPair<FString, FString>> RcRows = {{Tr(TEXT("처치")), CommaNum(G.Kills)}, {Tr(TEXT("레벨")), FString::FromInt(G.Level)},
			{Tr(TEXT("개인 랭킹")), LastRankPos >= 0 ? TrF(TEXT("%d위"), LastRankPos + 1) : FString(Tr(TEXT("20위 밖")))}};
		if (NewlyOpened >= 0) RcRows.Add({Tr(TEXT("새로 열림")), Tr(ModeNames[NewlyOpened])});
		if (LastBonus > 0) RcRows.Add({Tr(TEXT("첫 클리어 덤")), FString::Printf(TEXT("+%d"), LastBonus)}); // D-321
		const float RcB = Receipt(C, DesignW / 2.f, 300.f, RcRows, FString::Printf(TEXT("+ %d"), LastChips), TrF(TEXT("모은 칩 %s"), *CommaNum(MetaChips)));
		PaintRunUnlocks(C, RcB + 16.f);
		TubeBtn(C, ClearBtnX, ClearBtnY, 230.f, 44.f, Tr(TEXT("시작 화면으로")), true, CyanC, 0.5f + 0.5f * FMath::Sin(Clock * 4.f));
	}
	else if (G.Phase == EPDPhase::Over) // 게임 오버 (D-293): 꺼져 가는 'LAST CALL' 간판 + 영수증 + [다시 한 판] [메인 메뉴로]
	{
		const FLinearColor RedO = Hex(TEXT("ff5a64")), FgO = Hex(TEXT("cfd8e6")), LavO = Hex(TEXT("a896e1"));
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.88f));
		TArray<float> LB;
		const FString LC = TEXT("LAST CALL");
		for (int32 I = 0; I < LC.Len(); ++I) // 글자 셋은 지직거리며 꺼져 간다
		{
			const float Ph = FMath::Frac(Clock * (0.7f + I * 0.13f) + I * 0.37f);
			LB.Add(I == 2 ? (Ph < 0.12f ? 0.9f : 0.12f) : I == 7 ? (Ph < 0.3f ? 0.1f : 1.f) : I == 5 ? (Ph < 0.08f ? 0.3f : 1.f) : 1.f);
		}
		BulbSign(C, DesignW / 2.f, 60.f, 440.f, 90.f, LC, RedO, 44.f, 0.f, &LB);
		C.TextCentered(TrF(TEXT("웨이브 %d에서 무너졌다"), G.Wave), DesignW / 2.f, 176.f, FgO, 15.f, true);
		TArray<TPair<FString, FString>> RcRows = {{Tr(TEXT("웨이브")), FString::FromInt(G.Wave)}, {Tr(TEXT("처치")), CommaNum(G.Kills)}, {Tr(TEXT("레벨")), FString::FromInt(G.Level)},
			{Tr(TEXT("최고 기록")), G.Wave >= G.BestWave ? TrF(TEXT("%d (갱신!)"), G.BestWave) : FString::FromInt(G.BestWave)},
			{Tr(TEXT("개인 랭킹")), LastRankPos >= 0 ? TrF(TEXT("%d위"), LastRankPos + 1) : FString(Tr(TEXT("20위 밖")))}};
		const float RcB = Receipt(C, DesignW / 2.f, 196.f, RcRows, FString::Printf(TEXT("+ %d"), LastChips), TrF(TEXT("모은 칩 %s"), *CommaNum(MetaChips)));
		PaintRunUnlocks(C, RcB + 14.f);
		const float Pul = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
		TubeBtn(C, OverRestartX, OverBtnY, OverBtnW, OverBtnH, Tr(TEXT("다시 한 판 (R)")), !InRect(Mouse, OverMenuX, OverBtnY, OverBtnW, OverBtnH), BtnColPrimary(), Pul); // 주 행동 초록 (D-382)
		TubeBtn(C, OverMenuX, OverBtnY, OverBtnW, OverBtnH, Tr(TEXT("메인 메뉴로")), InRect(Mouse, OverMenuX, OverBtnY, OverBtnW, OverBtnH), BtnColDefault(), Pul);
	}
	if (bMenu) PaintMenu(C);
	if (bSettings) PaintSettings(C);
	if (bRanking) PaintRanking(C);
	if (LoadT > 0.f) PaintLoading(C);
	if (bSplash) PaintSplash(C); // 회사 로고 (D-385): 맨 위
	// 디버그 표시 (D-133, F3): 맨 위 왼쪽 아래 구석에
	if (GPDDebug.bShow)
	{
		const FLinearColor Good = Hex(TEXT("9cffb0")), Warn = Hex(TEXT("ffd96a")), Bad = Hex(TEXT("ff6a6a"));
		const float Fps = GPDDebug.Fps, Low = GPDDebug.WorstShown > 0.f ? 1000.f / GPDDebug.WorstShown : 0.f;
		const FLinearColor Fc = Fps >= 55.f ? Good : Fps >= 30.f ? Warn : Bad;
		const float X = 24.f, Y = DesignH - 136.f, W = 216.f, H = 110.f;
		C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.92f));
		C.Lines(FCanvas::RectPoints(X, Y, W, H), WithAlpha(Fc, 0.7f), 1.f);
		C.Text(FString::Printf(TEXT("FPS %.0f"), Fps), X + 10.f, Y + 24.f, Fc, 18.f, 0, true);
		C.Text(FString::Printf(TEXT("%.1f ms"), GPDDebug.Ms), X + W - 10.f, Y + 22.f, Fc, 12.f, 2, true);
		C.Text(TrF(TEXT("1초 최저 %.0f fps (%.1f ms)"), Low, GPDDebug.WorstShown), X + 10.f, Y + 44.f, Low >= 50.f ? Good : Low >= 25.f ? Warn : Bad, 10.f, 0, false, W - 20.f);
		C.Text(TrF(TEXT("적 %d · 발사체 %d · 타워 %d"), (int32)Game.Enemies.size(), (int32)Game.Projectiles.size(), (int32)Game.Towers.size()), X + 10.f, Y + 62.f, Hex(TEXT("cfd8e6")), 10.f, 0, false, W - 20.f);
		C.Text(TrF(TEXT("입자 %d · 고리 %d · 배속 ×%d · F3 닫기"), Particles.Num(), Rings.Num(), Game.Speed), X + 10.f, Y + 80.f, Hex(TEXT("8a94a6")), 10.f, 0, false, W - 20.f);
		C.Text(TrF(TEXT("그리기 %.1fms · 갱신 %.1fms · 선 %d (%dk점) · %s"), GPDDebug.PaintMs, GPDDebug.TickMs, GPDDebug.Lines, GPDDebug.Points / 1000, UseAtlas() ? Tr(TEXT("스프라이트")) : Tr(TEXT("진짜 선(F6)"))), X + 10.f, Y + 98.f, Hex(TEXT("7fd6ff")), 10.f, 0, false, W - 20.f);
	}
}

// 판: 격자·길, 족보 장판·테두리·연쇄 번개, 타워, 놓기 미리보기, 적
void SPDBoardWidget::PaintField(FCanvas& F, bool bStatic) const
{
	const FPDGame& G = Game;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	const bool bPv = !Preview.empty(); // 고른 카드를 놓으면 생길 족보를 보여 주는 중
	// 격자, 길 (정적 층). 구운 판 배경이 있으면 사각형 하나 (D-143)
	if (bStatic && UseAtlas() && Atlas->BgReady(BoardBgSig()))
		F.Image(F.NextLayer(), &Atlas->BgBrush, FX - BgPad, FY - BgPad, BgW, BgH, FLinearColor::White);
	else if (bStatic)
	{
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			const FVector2f P = CellPx((float)X, (float)Y);
			if (G.Path.IsPath(X, Y)) F.Box(P.X, P.Y, CS, CS, Hex(TEXT("7fd6ff"), 0.05f));
			else F.Lines(FCanvas::RectPoints(P.X + 0.5f, P.Y + 0.5f, CS - 1.f, CS - 1.f), Hex(TEXT("788cb4"), 0.07f), 1.f);
		}
	TArray<FVector2f> PathPts;
	for (const FPDPoint& Pt : G.Path.Points) PathPts.Add(CellPx(Pt.X, Pt.Y));
	F.Dashed(PathPts, Hex(TEXT("7fd6ff"), 0.35f), 2.f);
	F.Glow(FCanvas::RoundRectPoints(FX, FY, Cols * CS, Rows * CS, 8.f), Hex(TEXT("46c8e6"), 0.85f), 1.6f, 0.5f); // 하늘빛 틀을 판에 붙여 (Alice 시안 D-410: 262 ~ 1030 × 86 ~ 566)
	}
	if (!bStatic) // 길 위를 흐르는 에너지 (움직이는 층)
	{
		const int32 L = F.NextLayer();
		F.NextLayer();
		for (int I = 0; I < 14; ++I)
		{
			const float D = FMath::Fmod(Clock * 1.6f + I * G.Path.Length / 14.f, G.Path.Length);
			const FPDPoint Pt = G.Path.At(D);
			const FVector2f P = CellPx(Pt.X, Pt.Y);
			F.BoxAt(L, P.X - 4.f, P.Y - 4.f, 8.f, 8.f, Hex(TEXT("7fd6ff"), 0.12f));
			F.BoxAt(L, P.X - 1.5f, P.Y - 1.5f, 3.f, 3.f, Hex(TEXT("bfeaff"), 0.8f));
		}
	}

	// ---- 족보 테두리 (칸 모양대로 바깥 변만) + 연쇄 번개
	TSet<int32> Hatched; // 빗금은 칸마다 한 번만 (카드를 나눠 쓰는 족보가 겹쳐 그리던 것, D-135)
	auto DrawSyn = [&](const std::vector<FPDSynergy>& List, float Alpha, bool bOutline, bool bLightning) {
		for (int32 Si = 0; Si < (int32)List.size(); ++Si)
		{
			const FPDSynergy& S = List[Si];
			// 로열은 무지개로 천천히 돈다
			const FLinearColor Base = S.Kind == EPDHand::RoyalFlush ? FLinearColor::MakeFromHSV8((uint8)FMath::Fmod(Clock * 40.f, 256.f), 110, 255) : HandColor(S.Kind);
			const bool bFilt = HandFilter >= 0 && (int32)S.Kind == HandFilter; // 족보 필터 (D-148): 고른 족보는 반짝이고 나머지는 흐리게
			const float FiltA = HandFilter < 0 ? 1.f : bFilt ? 1.f : 0.22f;
			const FLinearColor Col = WithAlpha(Base, Alpha * FiltA);
			if (bOutline)
			{
			TSet<int32> Has;
			for (int K : S.Cells) Has.Add(K);
			const float M = 4.f + 5.f * SynLayer(List, Si); // 카드를 나눠 쓰는 족보는 안쪽 테두리
			TArray<TPair<FVector2f, FVector2f>> Edges; // 바깥 변 조각들 → 끝점끼리 이어 긴 선으로 (D-135: 선 수 줄이기)
			for (int K : S.Cells)
			{
				const int X = K % Cols, Y = K / Cols;
				const bool bL = X == 0 || !Has.Contains(K - 1), bR = X == Cols - 1 || !Has.Contains(K + 1), bU = !Has.Contains(K - Cols), bD = !Has.Contains(K + Cols);
				const FVector2f P = CellPx((float)X, (float)Y);
				const float X0 = P.X + (bL ? M : 0.f), X1 = P.X + CS - (bR ? M : 0.f), Y0 = P.Y + (bU ? M : 0.f), Y1 = P.Y + CS - (bD ? M : 0.f);
				// 장판 (D-099, D-144): 족보 색 옅은 채움 13%. 빗금은 합성할 때만 (합성 버튼이 뜬 족보의 재료 칸)
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, (bFilt ? 0.2f : 0.13f) * Alpha));
				const bool bAlready = true; // 평소 장판엔 빗금 없음 (필터 빗금은 판 위 UI 에서 합성 표시와 같이, D-148)
				if (!bAlready && UseAtlas()) // 구운 빗금 칸 (D-143)
				{
					const int32 Lyr = FMath::Min(SynLayer(List, Si), 2), Mask = (bL ? 1 : 0) | (bR ? 2 : 0) | (bU ? 4 : 0) | (bD ? 8 : 0);
					F.Image(F.NextLayer(), &Atlas->HatchBrushes[Lyr * 16 + Mask], X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, (bFilt ? 0.55f : 0.3f) * Alpha));
				}
				else if (!bAlready)
				for (float D = FMath::CeilToFloat((X0 + Y0) / HatchStep) * HatchStep; D < X1 + Y1; D += HatchStep)
				{
					const float Ax = FMath::Max(X0, D - Y1), Bx = FMath::Min(X1, D - Y0);
					if (Bx - Ax > 0.5f) F.Lines({{Ax, D - Ax}, {Bx, D - Bx}}, WithAlpha(Col, (bFilt ? 0.55f : 0.3f) * Alpha), 1.f);
				}
				// 안쪽으로 꺾이는 모서리 (D-297): 옆 칸과 대각 칸이 다 족보면 변을 M 만큼 더 뻗어 이웃 칸의 변과 만나게 (안 그러면 끝이 안 맞아 선이 끊겨 보였다)
				auto HasAt = [&](int DX, int DY) { const int NX = X + DX, NY = Y + DY; return NX >= 0 && NX < Cols && NY >= 0 && NY < Rows && Has.Contains(NY * Cols + NX); };
				const float PX0 = P.X, PX1 = P.X + CS, PY0 = P.Y, PY1 = P.Y + CS;
				if (bU) Edges.Emplace(FVector2f(!bL && HasAt(-1, -1) ? PX0 - M : X0, Y0), FVector2f(!bR && HasAt(1, -1) ? PX1 + M : X1, Y0));
				if (bR) Edges.Emplace(FVector2f(X1, !bU && HasAt(1, -1) ? PY0 - M : Y0), FVector2f(X1, !bD && HasAt(1, 1) ? PY1 + M : Y1));
				if (bD) Edges.Emplace(FVector2f(!bR && HasAt(1, 1) ? PX1 + M : X1, Y1), FVector2f(!bL && HasAt(-1, 1) ? PX0 - M : X0, Y1));
				if (bL) Edges.Emplace(FVector2f(X0, !bD && HasAt(-1, 1) ? PY1 + M : Y1), FVector2f(X0, !bU && HasAt(-1, -1) ? PY0 - M : Y0));
			}
			// 끝점이 맞는 조각을 이어 붙인다 (방향 무관)
			{
				TArray<bool> Used;
				Used.Init(false, Edges.Num());
				auto Same = [](FVector2f A, FVector2f B) { return FMath::Abs(A.X - B.X) < 0.01f && FMath::Abs(A.Y - B.Y) < 0.01f; };
				for (int32 I = 0; I < Edges.Num(); ++I)
				{
					if (Used[I]) continue;
					Used[I] = true;
					TArray<FVector2f> Line{Edges[I].Key, Edges[I].Value};
					for (bool bGrew = true; bGrew;)
					{
						bGrew = false;
						for (int32 J = 0; J < Edges.Num(); ++J)
						{
							if (Used[J]) continue;
							const FVector2f A = Edges[J].Key, B = Edges[J].Value;
							if (Same(Line.Last(), A)) Line.Add(B);
							else if (Same(Line.Last(), B)) Line.Add(A);
							else if (Same(Line[0], B)) Line.Insert(A, 0);
							else if (Same(Line[0], A)) Line.Insert(B, 0);
							else continue;
							Used[J] = bGrew = true;
						}
					}
					F.Glow(Line, Col, bFilt ? 2.6f : 2.f, bFilt ? 1.1f + 0.6f * (0.5f + 0.5f * FMath::Sin(Clock * 6.f)) : 0.8f);
				}
			}
			}
			if (!bLightning) continue;
			for (int J : S.Links)
			{
				if (J <= Si || J >= (int)List.size()) continue; // 연쇄 번개는 족보 둘 사이에 하나만 (예전엔 맞닿은 칸마다, 양쪽에서 두 번)
				bool bDone = false;
				for (int A : S.Cells)
					for (int B : List[J].Cells)
					{
						if (bDone || FMath::Abs(A % Cols - B % Cols) + FMath::Abs(A / Cols - B / Cols) != 1) continue;
						bDone = true;
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
	DrawSyn(G.Synergies, bPv ? 0.35f : 1.f, bStatic, !bStatic); // 장판·테두리는 정적 층, 연쇄 번개는 움직이는 층 (매 프레임 꺾임이 바뀜)
	if (!bStatic && bPv) DrawSyn(Preview, 0.6f + 0.4f * Pulse, true, true);
	if (!bStatic)
		for (const FCellFlash& Fl : CellFlashes)
		{
			const FVector2f P = CellPx((float)(Fl.Key % Cols), (float)(Fl.Key / Cols));
			F.Box(P.X + 2.f, P.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Fl.Col, Fl.T * 0.5f));
		}

	// ---- 타워 (D-140: 아틀라스가 준비되면 카드·조커는 구운 그림을 한 층에 모아 찍는다)
	const bool bAtlas = bStatic && UseAtlas();
	const int32 FillLayer = bAtlas ? F.NextLayer() : 0; // 족보 타워 채움 (카드 그림 밑, D-176)
	const int32 SpriteLayer = bAtlas ? F.NextLayer() : 0;
	const int32 DotLayer = bAtlas ? F.NextLayer() : 0; // 레벨 점 (카드 그림 위)
	if (bAtlas) F.NextLayer();
	for (const auto& KV : G.Towers)
	{
		const FPDTower& T = KV.second;
		if (bPv && T.X == HoverX && T.Y == HoverY) continue;
		const FVector2f P = CellCenter(KV.first);
		const bool bYoung = T.Age < YoungAge; // 막 놓인 타워는 움직이는 층에서
		const float Pop = T.Age < 0.3f ? 1.f + 0.5f * (1.f - T.Age / 0.3f) : 1.f; // 놓는 순간 커졌다 줄어듦
		if (T.IsJoker())
		{
			if (bStatic != bYoung)
			{
				if (IsUpperJoker(T.Joker)) // 상위 조커는 아틀라스에 없어 선으로 (D-231)
				{
					F.JokerTower(T.Joker, P.X, P.Y, CS * 0.4f * BoardCardK * Pop, 1.f, Pulse); // 미리보기 기호 + 금색 이중 테두리·왕관
				}
				else if (bAtlas) F.Sprite(SpriteLayer, &Atlas->Brushes[AtlasJoker0 + T.Joker], P.X, P.Y, BoardCardK, FLinearColor::White);
				else F.JokerTower(T.Joker, P.X, P.Y, CS * 0.4f * BoardCardK * Pop, 1.f, Pulse);
			}
			if (bStatic) continue; // 레벨·경험치는 자주 바뀌어 움직이는 층에서
			// 레벨 (위 오른쪽) + 경험치 막대 (아래), 레벨업 순간 막대가 번쩍 (D-125)
			{ // 레벨 딱지 (D-174): 칸 위쪽 가운데에 진한 바탕 + 분홍 테두리 + 흰 굵은 숫자. 강화(레벨 10 이상)면 금색
				const FString Lv = FString::Printf(TEXT("Lv%d"), T.JLevel);
				const bool bUp = T.JLevel >= JokerUpLevel;
				const FLinearColor LC = bUp ? Hex(TEXT("ffd96a")) : JokerPink;
				const float W = FMath::Min(CS + 4.f, FCanvas::Measure(Lv, FCanvas::Font(10.f, true)).X + 10.f), H = 14.f;
				const float LX0 = P.X - W / 2.f, LY0 = P.Y - CS * 0.5f - H / 2.f + 1.f;
				F.Box(LX0, LY0, W, H, Hex(TEXT("06060b"), 0.92f));
				F.Box(LX0, LY0, W, H, WithAlpha(LC, 0.22f));
				F.Lines(FCanvas::RectPoints(LX0, LY0, W, H), LC, 1.2f);
				F.TextCentered(Lv, P.X, LY0 + H / 2.f, FLinearColor::White, 10.f, true, W - 2.f);
			}
			if (T.JLevel < JokerMaxLevel)
			{
				const float BW = CS - 12.f, BX = P.X - BW / 2.f, BY = P.Y + CS * 0.5f - 5.f;
				const float U = FMath::Clamp(T.JXp / JokerXpNeed(T.JLevel), 0.f, 1.f);
				F.Box(BX, BY, BW, 3.f, Hex(TEXT("06060b"), 0.9f));
				F.Box(BX, BY, BW * U, 3.f, WithAlpha(JokerPink, 0.95f));
				F.Lines(FCanvas::RectPoints(BX, BY, BW, 3.f), WithAlpha(JokerPink, 0.45f), 1.f);
			}
			continue;
		}
		if (bStatic == bYoung) continue;
		if (bAtlas) // 흰색으로 구운 카드를 카드 색으로 물들여 찍고, 레벨 점은 따로 (D-154)
		{
			F.Sprite(SpriteLayer, &Atlas->Brushes[TowerSprite(T.Card, T.Level, false)], P.X, P.Y, BoardCardK, SuitColor(T.Card.Suit));
			if (T.Eff.Hand >= 0) // 족보에 든 타워: 옅은 채움 + 한 번 더 찍어 빛을 진하게 (예전엔 따로 구운 칸, D-176)
			{
				const float Hh = CS * 0.4f * BoardCardK * 0.98f;
				F.BoxAt(FillLayer, P.X - Hh, P.Y - Hh, Hh * 2.f, Hh * 2.f, WithAlpha(SuitColor(T.Card.Suit), 0.08f + 0.05f * Pulse));
				F.Sprite(SpriteLayer, &Atlas->Brushes[TowerSprite(T.Card, T.Level, false)], P.X, P.Y, BoardCardK, WithAlpha(SuitColor(T.Card.Suit), 0.5f));
			}
			if (T.Level >= 2)
				for (int L = 0; L < T.Level; ++L) F.BoxAt(DotLayer, P.X - (T.Level - 1) * 4.f + L * 8.f - 1.5f, P.Y - CS * 0.5f - 1.f, 3.f, 3.f, LevelDotColor());
			continue;
		}
		F.CardShape(T.Card, P.X, P.Y, CS * 0.4f * BoardCardK * Pop, 1.f, T.Eff.Hand >= 0, Pulse);
		// 레벨: 틀을 한 겹씩 더 (안쪽으로), 위쪽에 레벨 점
		for (int L = 2; L <= T.Level; ++L)
		{
			const float H = CS * 0.4f * BoardCardK * Pop * 0.98f + (L - 1) * 3.5f;
			F.Glow(FCanvas::RoundRectPoints(P.X - H, P.Y - H, H * 2.f, H * 2.f, 6.f + L), WithAlpha(SuitColor(T.Card.Suit), 0.75f), 1.2f, 0.6f);
		}
		if (T.Level >= 2)
			for (int L = 0; L < T.Level; ++L) F.Box(P.X - (T.Level - 1) * 4.f + L * 8.f - 1.5f, P.Y - CS * 0.5f - 1.f, 3.f, 3.f, LevelDotColor());
	}
	if (bStatic) return; // 아래는 모두 움직이는 층
	// 미리보기: 고른 카드를 마우스 칸에
	if (Sel >= 0 && Sel < (int32)G.Hand.size() && HoverX >= 0 && G.Buildable(HoverX, HoverY) && !G.CanPlace(HoverX, HoverY))
	{ // 합성 타워·조커 위 (D-192): 못 놓는다는 빨간 칸 + 안내
		const FVector2f B = CellPx((float)HoverX, (float)HoverY);
		const FLinearColor No = Hex(TEXT("ff5a6a"));
		F.Glow(FCanvas::RoundRectPoints(B.X + 2.f, B.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), No, 2.4f, 1.2f);
		F.Lines({B + FVector2f(CS * 0.3f, CS * 0.3f), B + FVector2f(CS * 0.7f, CS * 0.7f)}, WithAlpha(No, 0.8f), 1.6f);
		F.Lines({B + FVector2f(CS * 0.7f, CS * 0.3f), B + FVector2f(CS * 0.3f, CS * 0.7f)}, WithAlpha(No, 0.8f), 1.6f);
		F.Badge(TEXT("합성 타워·조커 위에는 놓을 수 없다"), B.X + CS / 2.f, B.Y - 10.f, No, 1.f);
	}
	else if (Sel >= 0 && Sel < (int32)G.Hand.size() && HoverX >= 0 && G.Buildable(HoverX, HoverY))
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
	// 보스 오라 (D-112): 군단장(주변 적 강화)·저주술사(주변 타워 약화) 범위를 옅은 점선으로
	for (const FPDEnemy& E : G.Enemies)
	{
		// 하우스 룰 (연쇄 끔) · 더 하우스 2단계(판 절반) · 이끄는 / 저주하는 변이도 같은 점선 (D-334)
		float R = 0.f;
		if (E.Type == EPDEnemy::Warlord || (E.Mut & MutLead)) R = PD::Boss::AuraRange;
		if (E.Type == EPDEnemy::Hexer || (E.Mut & MutHex)) R = FMath::Max(R, PD::Boss::HexRange);
		if (E.Type == EPDEnemy::HouseRule) R = PD::Boss::HouseRange;
		if (E.Type == EPDEnemy::House && E.Phase >= 1) R = PD::Boss::HouseRangeBig;
		if (R <= 0.f || E.bDead) continue;
		const FPDPoint Pt = G.Path.At(E.D);
		F.Dashed(FCanvas::CirclePoints(CellPx(Pt.X, Pt.Y), R * CS, 48), WithAlpha(EnemyColor(E.Type), 0.35f), 1.2f, 5.f, 7.f);
	}
	// 딜러가 바꾼 타워 (D-334): 딜러 색 점선 고리 + 남은 시간 + 원래 무늬 작게
	for (const FPDGame::FPDSuitSwap& Sw : G.SuitSwaps)
	{
		if (!G.Towers.count(Sw.Cell)) continue;
		const FVector2f C0 = CellCenter(Sw.Cell);
		const FLinearColor Dc = EnemyColor(EPDEnemy::Dealer);
		F.Dashed(FCanvas::RoundRectPoints(C0.X - CS / 2.f + 2.f, C0.Y - CS / 2.f + 2.f, CS - 4.f, CS - 4.f, RadSmall), WithAlpha(Dc, 0.5f + 0.4f * FMath::Min(1.f, Sw.T)), 1.6f, 4.f, 4.f);
		F.Shape(Sw.Orig, C0.X + CS / 2.f - 9.f, C0.Y - CS / 2.f + 9.f, 4.f, WithAlpha(Dc, 0.9f), 1.f, 0.3f);
	}
	// 하이롤러 칩 더미 (D-334): 길 위 작은 칩 탑, 지나가는 적이 빨라진다
	for (const FPDGame::FPDChipPile& Pl : G.ChipPiles)
	{
		const FPDPoint Pt = G.Path.At(Pl.D);
		const FVector2f C0 = CellPx(Pt.X, Pt.Y);
		const FLinearColor Cc = WithAlpha(EnemyColor(EPDEnemy::HighRoller), FMath::Min(1.f, Pl.T));
		for (int32 K = 0; K < 4; ++K) F.Lines(FCanvas::CirclePoints(C0 + FVector2f(0.f, 6.f - K * 4.f), 7.f, 14), Cc, 1.4f);
	}
	// 기절한 타워 (파괴자): 노란 X + 남은 시간만큼 옅어짐
	for (const auto& KV : G.Towers)
	{
		if (KV.second.StunT <= 0.f) continue;
		const FVector2f C0 = CellCenter(KV.first);
		const FLinearColor Sc = WithAlpha(Hex(TEXT("ffcf5a")), 0.5f + 0.5f * FMath::Min(1.f, KV.second.StunT));
		F.Box(C0.X - CS / 2.f + 3.f, C0.Y - CS / 2.f + 3.f, CS - 6.f, CS - 6.f, Hex(TEXT("06060b"), 0.55f));
		F.Glow({C0 + FVector2f(-12.f, -12.f), C0 + FVector2f(12.f, 12.f)}, Sc, 2.f, 0.8f);
		F.Glow({C0 + FVector2f(12.f, -12.f), C0 + FVector2f(-12.f, 12.f)}, Sc, 2.f, 0.8f);
	}
	for (const FPDEnemy& E : G.Enemies)
	{
		const FPDPoint Pt = G.Path.At(E.D);
		const float HF = HitFlash.FindRef(E.Id); // 맞은 순간: 흔들리며 하얗게 번쩍
		const FVector2f P = CellPx(Pt.X, Pt.Y) + (HF > 0.f ? FVector2f(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f)) * HF * 22.f : FVector2f(0.f, 0.f));
		FLinearColor Col = HF > 0.f ? FMath::Lerp(EnemyColor(E.Type), FLinearColor::White, FMath::Min(1.f, HF * 7.f)) : EnemyColor(E.Type);
		if (E.Untargetable()) Col.A = 0.25f + 0.1f * Pulse; // 유령: 무적인 동안 흐릿하게
		const bool bBoss = EnemyDef(E.Type).bBoss;
		if (HF > 0.f) F.Box(P.X - (bBoss ? 26.f : 13.f), P.Y - (bBoss ? 26.f : 13.f), bBoss ? 52.f : 26.f, bBoss ? 52.f : 26.f, WithAlpha(FLinearColor::White, HF * 1.2f));
		auto Poly = [&](int N, float R, float Rot) {
			TArray<FVector2f> Pts;
			for (int K = 0; K <= N; ++K) { const float A = Rot + K / (float)N * TwoPi; Pts.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
			return Pts;
		};
		if (UseAtlas()) // 카드병정 (D-175): 걸은 거리로 걷기 프레임 (멈추거나 느려지면 걸음도 느려진다), 적 색으로 물들임
		{
			const int32 Sh = SoldierShape(E.Type);
			const int32 Fr = (int32)(FMath::Fmod(E.D * 2.2f, 1.f) * SoldierFrames) % SoldierFrames;
			const float Sc = Sh >= 26 ? 1.15f : bBoss || Sh == 20 ? 0.95f : Sh == 6 ? 0.42f : 0.48f; // 최종 보스는 더 크게, 블러퍼 분신은 보스 크기 (D-335)
			const FLinearColor Ice = Hex(TEXT("b8f0ff"));
			const float Fz = E.SlowT > 0.f ? FMath::Min(1.f, E.SlowT * 3.f) : 0.f; // 둔화 (D-178): 몸이 얼음빛으로 물든다
			F.Sprite(F.NextLayer(), &Atlas->Brushes[AtlasSoldier0 + Sh * SoldierFrames + Fr], P.X, P.Y - 4.f * Sc, Sc, Fz > 0.f ? FMath::Lerp(Col, WithAlpha(Ice, Col.A), 0.8f * Fz) : Col);
			if (Fz > 0.f) // 둔화 (D-178, D-179): 얼음 껍질에 갇힌 몸 + 고드름 + 떨어지는 서리 + 발밑 얼음 고리 + 눈송이
			{
				{ // 얼음 껍질: 몸통 카드를 감싼 들쭉날쭉한 얼음 + 옅은 얼음 채움, 아래로 고드름
					const float BakeR = bBoss ? SoldierBossBakeR : SoldierBakeR, K = bBoss ? 1.35f : Sh == 6 ? 0.6f : 1.f;
					const float BWh = 0.38f * BakeR * K * Sc + 3.f, BHh = 0.59f * BakeR * K * Sc + 3.f;
					const float BCY = P.Y - 4.f * Sc + (SoldierFoot - 0.32f * BakeR * K - 0.59f * BakeR * K) * Sc;
					const float X0 = P.X - BWh, X1 = P.X + BWh, Y0 = BCY - BHh, Y1 = BCY + BHh;
					F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Ice, 0.16f * Fz));
					TArray<FVector2f> Shell;
					const int32 N = 6;
					for (int32 I = 0; I <= N; ++I) Shell.Add(FVector2f(FMath::Lerp(X0, X1, I / (float)N), Y0 - (I % 2 ? 3.f : 0.f))); // 윗변 뾰족
					for (int32 I = 1; I <= N; ++I) Shell.Add(FVector2f(X1 + (I % 2 ? 2.f : 0.f), FMath::Lerp(Y0, Y1, I / (float)N)));
					for (int32 I = 1; I <= N; ++I) Shell.Add(FVector2f(FMath::Lerp(X1, X0, I / (float)N), Y1));
					for (int32 I = 1; I <= N; ++I) Shell.Add(FVector2f(X0 - (I % 2 ? 2.f : 0.f), FMath::Lerp(Y1, Y0, I / (float)N)));
					F.Glow(Shell, WithAlpha(Ice, 0.9f * Fz), 1.3f, 1.f);
					for (int32 I = 0; I < 3; ++I) // 고드름 셋
					{
						const float X = FMath::Lerp(X0 + 3.f, X1 - 3.f, I / 2.f), L = (I == 1 ? 7.f : 4.5f) * (bBoss ? 1.6f : 1.f);
						F.Lines({FVector2f(X - 2.f, Y1), FVector2f(X, Y1 + L), FVector2f(X + 2.f, Y1)}, WithAlpha(Ice, 0.85f * Fz), 1.1f);
					}
					for (int32 I = 0; I < 4; ++I) // 둘레로 떨어지는 서리 알갱이
					{
						const float U = FMath::Fmod(Clock * 0.7f + I * 0.25f + E.Id * 0.13f, 1.f);
						const FVector2f D(FMath::Lerp(X0 - 4.f, X1 + 4.f, FMath::Fmod(I * 0.37f + E.Id * 0.21f, 1.f)), Y0 - 6.f + U * (Y1 - Y0 + 16.f));
						F.Box(D.X - 1.f, D.Y - 1.f, 2.f, 2.f, WithAlpha(FLinearColor::White, 0.8f * (1.f - U) * Fz));
					}
				}
				const float FeetY = P.Y + 33.f * Sc, Rx = bBoss ? 30.f : 15.f, Ry = Rx * 0.32f;
				TArray<FVector2f> Ring;
				for (int32 K = 0; K <= 28; ++K) { const float A = K / 28.f * TwoPi; Ring.Add(FVector2f(P.X + FMath::Cos(A) * Rx, FeetY + FMath::Sin(A) * Ry)); }
				F.Glow(Ring, WithAlpha(Ice, 0.75f * Fz), 1.2f, 0.9f);
				for (int32 K = 0; K < 3; ++K) // 고리에서 솟는 얼음 결정 셋
				{
					const float A = Clock * 0.6f + K * TwoPi / 3.f;
					const FVector2f B(P.X + FMath::Cos(A) * Rx, FeetY + FMath::Sin(A) * Ry);
					F.Lines({B, B + FVector2f(0.f, -(bBoss ? 9.f : 5.f))}, WithAlpha(Ice, 0.8f * Fz), 1.2f);
				}
				F.SpriteRot(F.NextLayer(), &Atlas->Brushes[AtlasSnow], P.X, P.Y - 4.f * Sc - 42.f * Sc, bBoss ? 0.36f : 0.24f, Clock * 0.8f + E.Id, WithAlpha(FLinearColor::White, Fz)); // 크고 하얗게 (D-179)
			}
			if (E.Type == EPDEnemy::Warden) // 보호막 켜짐/꺼짐은 그대로 보여 준다
			{
				if (E.Shield > 0.f) F.Glow(FCanvas::CirclePoints(P, 30.f + 2.f * Pulse, 40), Col, 2.2f, 1.6f);
				else F.Dashed(FCanvas::CirclePoints(P, 28.f, 40), WithAlpha(Col, 0.5f), 1.4f, 4.f, 5.f);
			}
			else if (E.Type == EPDEnemy::Charger && E.StateT > 0.f) // 돌진 중 속도선
				for (const float Yy : {-10.f, 0.f, 10.f}) F.Lines({P + FVector2f(-22.f, Yy), P + FVector2f(-44.f, Yy)}, WithAlpha(Col, 0.7f), 1.6f);
		}
		else
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
		// 새 보스 (D-112): docs/images/bosses_preview.png 의 절반 크기
		case EPDEnemy::Charger:
		{
			F.Glow({P + FVector2f(25.f, 0.f), P + FVector2f(-13.f, -18.f), P + FVector2f(-4.f, 0.f), P + FVector2f(-13.f, 18.f), P + FVector2f(25.f, 0.f)}, Col, 2.4f, 1.4f);
			F.Glow(FCanvas::CirclePoints(P + FVector2f(2.f, 0.f), 8.f), Col, 2.f);
			const float L = E.StateT > 0.f ? 20.f : 10.f; // 돌진 중이면 속도선이 길어짐
			for (const float Yy : {-7.f, 0.f, 7.f}) F.Lines({P + FVector2f(-24.f, Yy), P + FVector2f(-24.f - L, Yy)}, WithAlpha(Col, 0.7f), 1.6f);
			break;
		}
		case EPDEnemy::Summoner:
			F.Glow(FCanvas::CirclePoints(P, 15.f), Col, 2.4f, 1.4f);
			F.Glow(FCanvas::CirclePoints(P, 7.f), Col, 2.f);
			for (int32 K = 0; K < 3; ++K) F.Glow(FCanvas::CirclePoints(P + Rot2(FVector2f(22.f, 0.f), Clock * 1.5f + K * TwoPi / 3.f), 4.f, 12), Col, 1.6f);
			break;
		case EPDEnemy::Phantom:
		{
			TArray<FVector2f> Body;
			Body.Add(P + FVector2f(-15.f, 15.f));
			for (int32 K = 0; K <= 20; ++K) { const float A = PI + K / 20.f * PI; Body.Add(P + FVector2f(FMath::Cos(A) * 15.f, -3.f + FMath::Sin(A) * 18.f)); }
			for (const FVector2f Q : {FVector2f(15.f, 15.f), FVector2f(10.f, 11.f), FVector2f(5.f, 15.f), FVector2f(0.f, 11.f), FVector2f(-5.f, 15.f), FVector2f(-10.f, 11.f), FVector2f(-15.f, 15.f)}) Body.Add(P + Q);
			F.Glow(Body, Col, 2.2f, 1.2f);
			F.Glow(FCanvas::CirclePoints(P + FVector2f(-5.5f, -4.f), 2.f, 10), Col, 1.4f);
			F.Glow(FCanvas::CirclePoints(P + FVector2f(5.5f, -4.f), 2.f, 10), Col, 1.4f);
			break;
		}
		case EPDEnemy::Warlord:
		{
			TArray<FVector2f> Star;
			for (int32 K = 0; K <= 10; ++K) { const float A = -PI / 2.f + K / 10.f * TwoPi, R = K % 2 == 0 ? 24.f : 10.f; Star.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
			F.Glow(Star, Col, 2.4f, 1.4f);
			F.Glow(FCanvas::CirclePoints(P, 6.5f, 16), Col, 2.f);
			break;
		}
		case EPDEnemy::Berserker:
		{
			const float Rage = 1.f - FMath::Max(0.f, E.Hp / E.MaxHp); // 체력이 줄수록 가시가 길어짐
			TArray<FVector2f> Spikes;
			for (int32 K = 0; K <= 24; ++K) { const float A = Clock * (1.f + 3.f * Rage) + K / 24.f * TwoPi, R = K % 2 == 0 ? 22.f + 6.f * Rage : 15.f; Spikes.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * R); }
			F.Glow(Spikes, Col, 2.4f, 1.4f + Rage);
			F.Glow(FCanvas::CirclePoints(P, 12.f), Col, 2.f);
			break;
		}
		case EPDEnemy::Blinker:
		{
			auto Dia = [&](float Ox, float S) { return TArray<FVector2f>{P + FVector2f(Ox, -S), P + FVector2f(Ox + S * 0.7f, 0.f), P + FVector2f(Ox, S), P + FVector2f(Ox - S * 0.7f, 0.f), P + FVector2f(Ox, -S)}; };
			F.Glow(Dia(5.f, 22.f), Col, 2.4f, 1.4f);
			F.Lines(Dia(-9.f, 16.f), WithAlpha(Col, 0.5f), 1.6f);
			F.Lines(Dia(-20.f, 11.f), WithAlpha(Col, 0.25f), 1.4f);
			F.Glow(FCanvas::CirclePoints(P + FVector2f(5.f, 0.f), 6.f, 14), Col, 1.8f);
			break;
		}
		case EPDEnemy::Devourer:
		{
			TArray<FVector2f> Sp;
			for (int32 K = 0; K <= 60; ++K) { const float A = -Clock * 2.f + K / 60.f * 5.f * PI; Sp.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * (2.f + K * 0.34f)); }
			F.Glow(Sp, Col, 2.f, 1.2f);
			F.Glow(FCanvas::CirclePoints(P, 23.f), Col, 2.2f, 1.2f);
			break;
		}
		case EPDEnemy::Warden:
			F.Glow(Poly(6, 15.f, PI / 6.f), Col, 2.4f, 1.4f);
			F.Glow(FCanvas::CirclePoints(P, 6.f, 14), Col, 1.8f);
			if (E.Shield > 0.f) F.Glow(FCanvas::CirclePoints(P, 26.f + 2.f * Pulse, 40), Col, 2.2f, 1.6f); // 보호막 켜짐
			else F.Dashed(FCanvas::CirclePoints(P, 24.f, 40), WithAlpha(Col, 0.5f), 1.4f, 4.f, 5.f);
			break;
		case EPDEnemy::Hexer:
		{
			TArray<FVector2f> Eye;
			for (int32 K = 0; K <= 20; ++K) Eye.Add(P + FVector2f(-22.f + K / 20.f * 44.f, -FMath::Sin(K / 20.f * PI) * 13.f));
			for (int32 K = 1; K <= 20; ++K) Eye.Add(P + FVector2f(22.f - K / 20.f * 44.f, FMath::Sin(K / 20.f * PI) * 13.f));
			F.Glow(Eye, Col, 2.2f, 1.3f);
			F.Glow(FCanvas::CirclePoints(P, 6.5f, 16), Col, 2.f);
			F.Lines(FCanvas::CirclePoints(P, 2.5f, 10), Col, 1.6f);
			break;
		}
		case EPDEnemy::Breaker:
			F.Glow(Poly(4, 22.f, PI / 4.f), Col, 2.4f, 1.4f);
			F.Lines({P + FVector2f(-10.f, -10.f), P + FVector2f(10.f, 10.f)}, Col, 2.f);
			F.Lines({P + FVector2f(10.f, -10.f), P + FVector2f(-10.f, 10.f)}, Col, 2.f);
			for (int32 K = 0; K < 4; ++K) { const FVector2f Dd = Rot2(FVector2f(1.f, 0.f), PI / 4.f + K * PI / 2.f); F.Lines({P + Dd * 21.f, P + Dd * 28.f}, Col, 1.8f); }
			break;
		default: F.Glow(FCanvas::CirclePoints(P, 9.f, 20), Col, 2.f); break;
		}
		if (E.SlowT > 0.f && !UseAtlas()) F.Lines(FCanvas::CirclePoints(P, bBoss ? 28.f : 15.f, 24), Hex(TEXT("8fe9ff"), 0.6f), 1.2f); // 아틀라스가 없을 때만 예전 고리 (D-178)
		const float W = bBoss ? 44.f : 24.f, BY = P.Y - (bBoss ? 34.f : 22.f);
		F.Box(P.X - W / 2.f, BY, W, 3.f, Hex(TEXT("ffffff"), 0.15f));
		const float HpNow = FMath::Max(0.f, E.Hp / E.MaxHp), HpOld = FMath::Max(HpNow, HpShown.FindRef(E.Id));
		if (HpOld > HpNow) F.Box(P.X - W / 2.f + W * HpNow, BY, W * (HpOld - HpNow), 3.f, Hex(TEXT("ffffff"), 0.85f)); // 방금 깎인 만큼
		F.Box(P.X - W / 2.f, BY, W * HpNow, 3.f, EnemyColor(E.Type));
		if (E.Type == EPDEnemy::BigBlind) // 판돈 (D-334): 남은 시간 막대 + 판돈, 올인이면 빨간 글자
		{
			if (E.Pot > 0.f)
			{
				const float Tf = FMath::Clamp(E.StateT / PD::Boss::BlindTime, 0.f, 1.f);
				F.Box(P.X - W / 2.f, BY - 6.f, W * Tf, 2.f, Hex(TEXT("ffd96a")));
				F.TextCentered(TrF(TEXT("판돈 %d"), FMath::RoundToInt(E.Pot)), P.X, BY - 14.f, Hex(TEXT("ffd96a")), 9.f, true);
			}
			else if (E.bRage) F.TextCentered(TEXT("ALL IN"), P.X, BY - 10.f, Hex(TEXT("ff6a6a")), 10.f, true);
		}
		if (E.Type == EPDEnemy::Counter) // 카드 카운터: 가장 큰 내성 무늬와 %
		{
			int32 Bs = 0;
			for (int32 K = 1; K < 4; ++K) if (E.Res[K] > E.Res[Bs]) Bs = K;
			if (E.Res[Bs] >= 0.05f) { F.Shape((EPDSuit)Bs, P.X - 8.f, BY - 10.f, 4.f, SuitColor((EPDSuit)Bs), 1.f, 0.3f); F.Text(FString::Printf(TEXT("-%d%%"), FMath::RoundToInt(E.Res[Bs] * 100.f)), P.X - 2.f, BY - 6.f, FLinearColor::White, 9.f, 0, true); }
		}
		if (E.Mut) // 변이 (D-334): 체력 막대 위 작은 점들 (색 = 변이 종류)
		{
			static const TCHAR* MutCol[6] = {TEXT("cfd8e6"), TEXT("ffa86a"), TEXT("8affc4"), TEXT("a8f0ff"), TEXT("ffd96a"), TEXT("9a7aff")};
			int32 N = 0;
			for (int32 K = 0; K < 6; ++K) if (E.Mut & (1 << K)) { F.Glow(FCanvas::CirclePoints(FVector2f(P.X + W / 2.f + 5.f + N * 7.f, BY + 1.f), 2.4f, 8), Hex(MutCol[K]), 1.4f, 0.6f); ++N; }
		}
		if (E.Armor > 0.f && !G.Has(EPDCharm::Pierce)) // 방어력 (D-186): 체력 막대 왼쪽에 작은 방패 (아틀라스가 없으면 예전 막대)
		{
			if (UseAtlas())
			{
				const float Ss = bBoss ? 8.f : 6.f;
				F.Sprite(F.NextLayer(), &Atlas->Brushes[AtlasShield], P.X - W / 2.f - Ss - 3.f, BY + 1.5f, Ss / ShieldBakeS, Hex(TEXT("9fc3e8")));
			}
			else F.Box(P.X - W / 2.f, BY - 4.f, W, 2.f, Hex(TEXT("9fc3e8")));
		}
	}
}

// 날아가는 발사체 (미리보기 tools/neon/neon.py shots_level / shots_hand / shots_joker)
void SPDBoardWidget::PaintShots(FCanvas& F) const
{
	const FPDGame& G = Game;
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
}

// 이펙트: 빛줄기, 별빛, 몸 조각, 퍼지는 파동, 파티클
void SPDBoardWidget::PaintEffects(FCanvas& F) const
{
	// 이펙트가 많을 때 (10배속): 번짐 없이 심만, 입자 꼬리는 점으로 (선 수가 곧 그리기 부담, D-138)
	const bool bBusyFx = Rings.Num() + Flares.Num() + Shards.Num() + Beams.Num() > 150;
	const bool bDotParticles = Particles.Num() > 400;
	F.HaloK = bBusyFx ? 0.f : 1.f;
	ON_SCOPE_EXIT { F.HaloK = 1.f; };
	// 빛줄기: 레이저, 레일건, 번개, 로열, 체셔
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
	// 별빛·고리는 아틀라스에 구운 흰 모양을 색·크기·회전을 입혀 찍는다 (D-141). 아주 큰 고리는 번질까 봐 선으로
	const bool bFxAtlas = UseAtlas();
	const int32 FlareLayer = bFxAtlas ? F.NextLayer() : 0;
	if (bFxAtlas) F.NextLayer();
	for (const FFlare& Fl : Flares)
	{
		if (Fl.T < 0.f) continue; // 늦게 나타나는 별빛
		const float U = Fl.T / Fl.MaxT, A = 1.f - U, Sz = Fl.Size * (0.6f + 0.6f * FMath::Sin(FMath::Min(1.f, U * 2.f) * PI * 0.5f));
		if (bFxAtlas && Fl.Spokes % 2 == 0 && Fl.Spokes <= 8 && Sz <= 90.f)
		{
			F.SpriteRot(FlareLayer, &Atlas->Brushes[AtlasFx0 + FxFlare(Fl.Spokes)], Fl.C.X, Fl.C.Y, Sz / AtlasFxR, Fl.Spokes == 2 ? Fl.Rot : Fl.Rot + U * 0.6f, WithAlpha(Fl.Col, A));
			continue;
		}
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
		if (bFxAtlas) { F.ImageRot(FlareLayer, &Atlas->LineBrush, Sh.P.X, Sh.P.Y, Sh.Len * (LineHalf * 2.f + 12.f) / (LineHalf * 2.f), LineH, Sh.A, WithAlpha(Sh.Col, A)); continue; } // 구운 빛선 (D-143)
		const FVector2f D = Rot2(FVector2f(Sh.Len * 0.5f, 0.f), Sh.A);
		F.Glow({Sh.P - D, Sh.P + D}, WithAlpha(Sh.Col, A), 1.5f, 0.9f * A);
	}
	const int32 RingLayer = bFxAtlas ? F.NextLayer() : 0;
	if (bFxAtlas) F.NextLayer();
	for (const FRing& R : Rings)
	{
		if (R.T < 0.f) continue; // 늦게 퍼지는 파동
		const float U = R.T / R.MaxT, Rad = R.R0 + (R.R1 - R.R0) * (1.f - (1.f - U) * (1.f - U));
		const FLinearColor RC = WithAlpha(R.Col, 1.f - U);
		const float Th = R.Width * (1.f - U * 0.5f), A0 = R.Rot + R.Spin * R.T;
		if (bFxAtlas && Rad <= 80.f)
		{
			int32 Id = 0;
			switch (R.Shape)
			{
			case EWave::Poly: Id = FxPoly(R.N); break;
			case EWave::Star: Id = FxStar(R.N); break;
			case EWave::Broken: Id = FxBroken(R.N); break;
			case EWave::Rays: Id = FxRays(R.N); break;
			case EWave::Suit: Id = FxSuit(R.N); break;
			default: Id = 0; break;
			}
			if (Rad > 1.f) F.SpriteRot(RingLayer, &Atlas->Brushes[AtlasFx0 + Id], R.C.X, R.C.Y, Rad / AtlasFxR, R.Shape == EWave::Suit || R.Shape == EWave::Circle ? 0.f : A0, RC);
			continue;
		}
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
		F.NextLayer();
		for (const FParticle& Pa : Particles)
		{
			const float A = FMath::Clamp(Pa.Life / Pa.MaxLife, 0.f, 1.f), S = Pa.Size;
			F.BoxAt(L1, Pa.P.X - S * 2.f, Pa.P.Y - S * 2.f, S * 4.f, S * 4.f, WithAlpha(Pa.Col, 0.15f * A));
			if (Pa.bStreak && !bDotParticles)
			{
				const FVector2f Tail = Pa.P - Pa.V * 0.04f;
				if (bFxAtlas) // 구운 빛선 (D-143)
				{
					const FVector2f Mid = (Tail + Pa.P) * 0.5f, Dv = Pa.P - Tail;
					F.ImageRot(L2, &Atlas->LineBrush, Mid.X, Mid.Y, FMath::Max(2.f, Dv.Size()) * (LineHalf * 2.f + 12.f) / (LineHalf * 2.f), LineH * FMath::Clamp(S / 1.5f, 0.6f, 2.f), FMath::Atan2(Dv.Y, Dv.X), WithAlpha(Pa.Col, A));
				}
				else F.Lines({Tail, Pa.P}, WithAlpha(Pa.Col, A), S);
			}
			else F.BoxAt(L2, Pa.P.X - S / 2.f, Pa.P.Y - S / 2.f, S, S, WithAlpha(Pa.Col, A));
		}
	}
}

// 판 위 UI: 족보 이름표, 합성 위치 고르기, 합성·먹이기·조커 버튼, 타워 정보, 떠오르는 글자, 배너
void SPDBoardWidget::PaintBoardUi(FCanvas& F) const
{
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Fg = Hex(TEXT("cfd8e6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	const bool bPv = !Preview.empty(); // 고른 카드를 놓으면 생길 족보를 보여 주는 중
	// 족보 이름표 (타워 위에)
	TArray<FBox2f> BadgeRects; // 이번 프레임에 놓인 이름표 자리
	auto DrawBadges = [&](const std::vector<FPDSynergy>& List, float Alpha) {
		for (int32 Si = 0; Si < (int32)List.size(); ++Si)
		{
			const FPDSynergy& S = List[Si];
			int Top = S.Cells[0];
			for (int K : S.Cells) if (K / Cols < Top / Cols || (K / Cols == Top / Cols && K % Cols < Top % Cols)) Top = K;
			const FVector2f P = CellPx(Top % Cols + 0.5f, (float)(Top / Cols));
			// 겹치지 않게 (D-150): 이미 놓인 이름표와 겹치면 한 줄씩 아래로
			const FString Lb = HandLabel(S);
			const float W = FCanvas::Measure(Lb, FCanvas::Font(11.f, true)).X + 10.f, H = 16.f;
			float Y = P.Y + 4.f;
			for (int32 Try = 0; Try < 12; ++Try)
			{
				const FBox2f R(FVector2f(P.X - W / 2.f, Y - H / 2.f), FVector2f(P.X + W / 2.f, Y + H / 2.f));
				if (!BadgeRects.ContainsByPredicate([&](const FBox2f& O) { return O.Intersect(R); })) break;
				Y += H + 2.f;
			}
			BadgeRects.Add(FBox2f(FVector2f(P.X - W / 2.f - 1.f, Y - H / 2.f - 1.f), FVector2f(P.X + W / 2.f + 1.f, Y + H / 2.f + 1.f)));
			F.Badge(Lb, P.X, Y, HandColor(S.Kind), Alpha);
		}
	};
	// 마우스가 올라간 타워의 족보·연쇄는 이름표 대신 타워 툴팁 한 곳에 (D-202). 이름표는 합성 모드(고른 족보)·놓기 미리보기만
	std::vector<FPDSynergy> HovSyn;
	{
		std::vector<FPDSynergy> Hov;
		const int HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		for (const FPDSynergy& S : G.Synergies)
		{
			if (std::find(S.Cells.begin(), S.Cells.end(), HKey) != S.Cells.end()) HovSyn.push_back(S);
			// 합성 모드에선 족보 이름표를 안 띄운다 (D-206): 색 덩어리로 구분하고, 연쇄 글자가 칸을 가렸다
		}
		if (!bPv && !InFeed()) DrawBadges(Hov, 1.f); // 먹이기 중엔 [먹이기] 버튼이 족보 이름을 대신한다 (D-197)
	}
	if (bPv) DrawBadges(Preview, 1.f);
	// 합성 위치 고르는 중: 재료 칸을 깜빡이고, 마우스가 올라간 칸을 표시 (기본 칸 없음, D-194)
	if (!PickCells.empty())
	{
		const FLinearColor PickCol = bPickJoker ? JokerPink : Gold;
		const int32 HoverKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		const bool bHoverIn = std::find(PickCells.begin(), PickCells.end(), HoverKey) != PickCells.end();
		// 눈에 띄게 (D-149): 재료가 아닌 칸은 어둡게 덮고, 재료 칸은 색이 오가는 채움 + 빗금 + 굵은 테두리
		{
			const int32 DL = F.NextLayer();
			for (int32 K = 0; K < Cols * Rows; ++K)
			{
				if (std::find(PickCells.begin(), PickCells.end(), K) != PickCells.end()) continue;
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.BoxAt(DL, C0.X, C0.Y, CS, CS, Hex(TEXT("02020a"), 0.62f));
			}
			F.NextLayer();
		}
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
		for (int K : PickCells)
		{
			const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
			const bool bTarget = bHoverIn && K == HoverKey; // 지금 클릭하면 모일 칸
			// 모일 칸은 다른 색 (D-149): 하늘색으로 진하게 + 퍼지는 테두리 두 겹 + "여기". 나머지 재료 칸은 금색으로 한 단계 옅게
			const FLinearColor TargetCol = Hex(TEXT("5ff2ff"));
			const FLinearColor Pc = bTarget ? FMath::Lerp(TargetCol, FLinearColor::White, 0.35f * Wv) : FMath::Lerp(PickCol, FLinearColor::White, 0.2f * Wv);
			F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Pc, bTarget ? 0.38f + 0.17f * Wv : 0.1f + 0.08f * Wv));
			if (UseAtlas()) F.Image(F.NextLayer(), &Atlas->HatchBrushes[0], C0.X, C0.Y, CS, CS, WithAlpha(Pc, bTarget ? 0.55f + 0.25f * Wv : 0.3f + 0.15f * Wv));
			F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), Pc, bTarget ? 3.8f : 2.f, bTarget ? 1.8f + 1.f * Wv : 0.7f + 0.4f * Wv);
			if (bTarget)
				for (int32 Ring = 0; Ring < 2; ++Ring)
				{
					const float U = FMath::Fmod(Clock * 1.4f + Ring * 0.5f, 1.f), Gr = 2.f + 12.f * U;
					F.Glow(FCanvas::RoundRectPoints(C0.X - Gr, C0.Y - Gr, CS + Gr * 2.f, CS + Gr * 2.f, 8.f + Gr * 0.5f), WithAlpha(TargetCol, 0.9f * (1.f - U)), 2.2f, 1.f);
				}
			if (bTarget) F.TextCentered(TEXT("여기"), C0.X + CS / 2.f, C0.Y - 10.f, TargetCol, 12.f, true, CS + 20.f);
		}
		// 다른 칸에서 고른 칸으로 빨려 드는 선
		for (int K : PickCells)
			if (bHoverIn && K != HoverKey) F.Dashed({CellCenter(K), CellCenter(HoverKey)}, WithAlpha(Hex(TEXT("5ff2ff")), 0.7f), 1.6f, 4.f, 5.f);
		// 자리 고르는 중 (D-123): 판 가운데에 큰 글자 (바탕 없이 반투명, 아래 칸이 비쳐 보이게)
		{
			const float MX = FX + Cols * CS / 2.f, MY = FY + Rows * CS / 2.f;
			const FLinearColor Big = WithAlpha(PickCol, 0.55f + 0.3f * Pulse);
			F.TextCentered(bPickJoker ? TEXT("조커 자리를 고르세요") : TEXT("합성 타워 자리를 고르세요"), MX, MY - 14.f, Big, 34.f, true, Cols * CS - 40.f);
			F.TextCentered(TEXT("빛나는 재료 칸 중 하나를 클릭 · Esc / 오른쪽 클릭: 취소"), MX, MY + 22.f, WithAlpha(Fg, 0.75f), 13.f, false, Cols * CS - 40.f);
		}
	}
	// 족보 필터 (D-148): 합성 못 하는 족보도 합성 표시와 똑같이 칸을 밝힌다 (타워 위에 그려야 보인다). 합성 되는 족보는 아래 합성 표시가 그림
	// 조커 목록 (D-210): 목록에서 마우스를 올린 조커는 게임이 고른 재료를 미리 보여 주고, 고른 조커는 재료(진하게)·바꿀 후보(흐리게)를 보여 준다.
	// 아무것도 안 고르고 안 올렸으면 만들 수 있는 조커들의 재료를 옅게. 그 밖의 칸은 어둡게
	bool bCraftHov = false; // 고른 재료 칸 위: 조커 전후 툴팁 (D-210)
	if (InCraft() && !bPv && PickCells.empty() && G.Phase != EPDPhase::Reward)
	{
		const std::vector<FPDJokerNode> Ns = G.JokerNodes();
		int32 J = CraftJoker;
		bool bPreview = false;
		std::vector<std::vector<int>> Show;
		int32 UJ = CraftUpper; // 상위 조커 (D-237): 합칠 두 조커
		if (J >= 0 || UJ >= 0) Show = CraftPicks;
		else if (const int32 R = JokerListAt(Mouse); R >= 0 && JokerList[R].bMake) { if (JokerList[R].Upper >= 0) UJ = JokerList[R].Upper; else J = JokerList[R].J; Show = JokerList[R].Best; bPreview = true; }
		std::vector<int> CandNodes; // 고른 조커의 레시피 족보인 재료 (바꿀 후보)
		if (J >= 0 && !bPreview)
		{
			std::vector<EPDHand> Kinds;
			for (EPDHand H : JokerRecipe((EPDJoker)J)) Kinds.push_back(H);
			for (EPDHand H : JokerRecipe2((EPDJoker)J)) Kinds.push_back(H);
			for (int32 I = 0; I < (int32)Ns.size(); ++I)
			{
				if (std::find(Kinds.begin(), Kinds.end(), Ns[I].Kind) == Kinds.end()) continue;
				bool bPicked = false;
				for (const std::vector<int>& Pk : Show) { std::vector<int> A = Pk, B = Ns[I].Cells; std::sort(A.begin(), A.end()); std::sort(B.begin(), B.end()); bPicked = bPicked || A == B; }
				if (!bPicked) CandNodes.push_back(I);
			}
		}
		TSet<int32> Lit;
		if (J >= 0 || UJ >= 0) { for (const std::vector<int>& Pk : Show) for (int K : Pk) Lit.Add(K); for (int N : CandNodes) for (int K : Ns[N].Cells) Lit.Add(K); }
		else for (const FJokerEntry& E : JokerList) if (E.bMake) for (const std::vector<int>& Pk : E.Best) for (int K : Pk) Lit.Add(K);
		{
			const int32 DL = F.NextLayer();
			for (int32 K = 0; K < Cols * Rows; ++K)
			{
				if (Lit.Contains(K)) continue;
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.BoxAt(DL, C0.X, C0.Y, CS, CS, Hex(TEXT("02020a"), G.Path.IsPath(K % Cols, K / Cols) ? 0.2f : 0.82f));
			}
			F.NextLayer();
		}
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
		const int32 HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		// 재료 하나를 한 덩어리로 (D-212, 합성 모드 D-201 과 같은 모양): 채움은 같은 재료 칸끼리 이어 붙이고 테두리는 바깥 변에만
		auto Block = [&](const std::vector<int>& Cells, const FLinearColor& Col, float Fill, float Ew, float Eg) {
			auto Same = [&Cells](int X, int Y) { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && std::find(Cells.begin(), Cells.end(), CellKey(X, Y)) != Cells.end(); };
			for (int K : Cells)
			{
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				const int CX = K % Cols, CY = K / Cols;
				const bool bL = Same(CX - 1, CY), bR = Same(CX + 1, CY), bU = Same(CX, CY - 1), bD = Same(CX, CY + 1);
				const float In = 4.f, X0 = C0.X + (bL ? 0.f : In), X1 = C0.X + CS - (bR ? 0.f : In), Y0 = C0.Y + (bU ? 0.f : In), Y1 = C0.Y + CS - (bD ? 0.f : In);
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, Fill));
				if (!bU) F.Glow({FVector2f(X0, Y0), FVector2f(X1, Y0)}, Col, Ew, Eg);
				if (!bD) F.Glow({FVector2f(X0, Y1), FVector2f(X1, Y1)}, Col, Ew, Eg);
				if (!bL) F.Glow({FVector2f(X0, Y0), FVector2f(X0, Y1)}, Col, Ew, Eg);
				if (!bR) F.Glow({FVector2f(X1, Y0), FVector2f(X1, Y1)}, Col, Ew, Eg);
			}
		};
		if (UJ >= 0 && !bPreview && Lit.Contains(HKey)) // 상위 조커: 두 조커 중 마우스 칸에 생긴다
		{
			const FVector2f C0 = CellPx((float)HoverX, (float)HoverY);
			F.Badge(TEXT("여기에 상위 조커"), C0.X + CS / 2.f, C0.Y + CS + 20.f, Gold, 1.f);
			BadgeRects.Add(FBox2f(FVector2f(C0.X - 20.f, C0.Y + CS + 11.f), FVector2f(C0.X + CS + 20.f, C0.Y + CS + 29.f)));
		}
		if (J < 0 && UJ < 0) // 목록만: 만들 수 있는 조커들의 재료를 옅은 분홍 덩어리로
			for (const FJokerEntry& E : JokerList) if (E.bMake) for (const std::vector<int>& Pk : E.Best) Block(Pk, WithAlpha(JokerPink, 0.7f), 0.08f, 1.8f, 0.5f + 0.3f * Wv);
		for (int N : CandNodes) // 후보: 족보 색 덩어리, 마우스가 올라가면 밝게 + "재료로 고르기"
		{
			const bool bH = std::find(Ns[N].Cells.begin(), Ns[N].Cells.end(), HKey) != Ns[N].Cells.end();
			const FLinearColor Hc = HandColor(Ns[N].Kind);
			Block(Ns[N].Cells, WithAlpha(Hc, bH ? 0.95f : 0.65f), bH ? 0.2f : 0.08f, bH ? 2.8f : 1.8f, bH ? 1.2f : 0.45f);
			if (bH) { const FVector2f C0 = CellPx((float)HoverX, (float)HoverY); F.Badge(TEXT("클릭: 재료로 고르기"), C0.X + CS / 2.f, C0.Y + CS + 20.f, Hc, 1.f); }
		}
		for (int32 Pi = 0; Pi < (int32)Show.size(); ++Pi) // 고른 재료: 분홍 덩어리(진한 채움 + 굵은 테두리) + 번호
		{
			const FLinearColor Pc = FMath::Lerp(JokerPink, FLinearColor::White, 0.2f * Wv);
			Block(Show[Pi], WithAlpha(Pc, 0.9f), (0.26f + 0.1f * Wv) * 0.9f, 3.f, (1.3f + 0.7f * Wv) * 0.9f);
			const int K0 = *std::min_element(Show[Pi].begin(), Show[Pi].end());
			const FVector2f C0 = CellPx((float)(K0 % Cols), (float)(K0 / Cols));
			F.Box(C0.X + 1.f, C0.Y + 1.f, 15.f, 15.f, Hex(TEXT("06060b"), 0.9f));
			F.TextCentered(FString::Printf(TEXT("%d"), Pi + 1), C0.X + 8.5f, C0.Y + 9.f, JokerPink, 10.f, true);
		}
		// 추천 재료 (D-211): 게임이 고른 가장 좋은 재료 중 아직 안 고른 것에 "추천" 딱지
		if (J >= 0 && !bPreview)
			for (const FJokerEntry& E : JokerList)
				if (E.J == J)
					for (const std::vector<int>& Pk : E.Best)
					{
						bool bPicked = false;
						for (const std::vector<int>& Q : Show) { std::vector<int> A = Pk, B = Q; std::sort(A.begin(), A.end()); std::sort(B.begin(), B.end()); bPicked = bPicked || A == B; }
						if (bPicked) continue;
						const int K0 = *std::min_element(Pk.begin(), Pk.end());
						const FVector2f C0 = CellPx((float)(K0 % Cols), (float)(K0 / Cols));
						F.Box(C0.X + CS - 25.f, C0.Y + CS - 13.f, 24.f, 12.f, Hex(TEXT("06060b"), 0.9f));
						F.TextCentered(TEXT("추천"), C0.X + CS - 13.f, C0.Y + CS - 7.f, Gold, 8.f, true, 23.f);
					}
		if (J >= 0 && !bPreview && Lit.Contains(HKey)) // 고른 재료 칸 위: 그 칸에 조커가 생긴다
		{
			bool bOnPick = false;
			for (const std::vector<int>& Pk : Show) bOnPick = bOnPick || std::find(Pk.begin(), Pk.end(), HKey) != Pk.end();
			const std::vector<int> PIdx = CraftNodeIdx();
			const bool bDone = PIdx.size() == Show.size() && G.JokerCraftResult(PIdx) == (EPDJoker)J;
			if (bOnPick && !bDone) { const FVector2f C0 = CellPx((float)HoverX, (float)HoverY); F.Badge(TEXT("클릭: 빼기"), C0.X + CS / 2.f, C0.Y + CS + 20.f, Hex(TEXT("cfd8e6")), 1.f); }
			if (bOnPick && bDone)
			{
				bCraftHov = true;
				const FLinearColor TargetCol = Hex(TEXT("5ff2ff"));
				const FVector2f C0 = CellPx((float)HoverX, (float)HoverY);
				F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), WithAlpha(TargetCol, 0.9f), 3.8f, (1.8f + Wv) * 0.9f);
				for (const std::vector<int>& Pk : Show) for (int K : Pk) if (K != HKey) F.Dashed({CellCenter(K), CellCenter(HKey)}, WithAlpha(TargetCol, 0.6f), 1.4f, 4.f, 5.f);
				F.Badge(TEXT("여기에 조커"), C0.X + CS / 2.f, C0.Y + CS + 20.f, TargetCol, 1.f);
				BadgeRects.Add(FBox2f(FVector2f(C0.X - 20.f, C0.Y + CS + 11.f), FVector2f(C0.X + CS + 20.f, C0.Y + CS + 29.f)));
			}
		}
	}
	// 합성 모드 (D-199): 왼쪽 족보 목록에서 고른 족보만. 판 위 [합성] 버튼·위력 배지는 없앴다. 다른 칸은 어둡게, 재료 칸은 반짝, 마우스 칸 = 합쳐질 칸
	const bool bFuseMode = HandFilter >= 0 && !InCraft() && !InFeed() && !bPv && PickCells.empty() && G.Phase != EPDPhase::Reward && !bSwapMode;
	if (bFuseMode)
	{
		TSet<int32> Lit;
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I) if ((int32)G.Synergies[I].Kind == HandFilter && G.CanFuse(I)) for (int K : G.Synergies[I].Cells) Lit.Add(K); // 합성 되는 족보만 밝게 (D-207)
		const int32 DL = F.NextLayer();
		for (int32 K = 0; K < Cols * Rows; ++K)
		{
			if (Lit.Contains(K)) continue;
			const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
			F.BoxAt(DL, C0.X, C0.Y, CS, CS, Hex(TEXT("02020a"), G.Path.IsPath(K % Cols, K / Cols) ? 0.2f : 0.82f)); // 합성 모드: 다른 포탑 네온을 더 눌러 고른 족보가 돋보이게 (D-203, 예전 0.55) 길 칸은 옅게 (D-218)
		}
		F.NextLayer();
	}
	bool bFuseHov = false; // 합쳐질 칸 위: 보통 타워 툴팁 대신 합성 전후 툴팁 (D-209)
	int32 FuseHovSyn = -1;
	// 합성 못 하는 족보(조커가 낌·최대 레벨)는 따로 표시하지 않고 다른 칸처럼 어둡게 (D-207, D-200 의 빨간 X 는 난잡해서 뺐다)
	// 같은 족보가 여럿 붙어 있으면 족보마다 다른 색 (D-200): 맞닿은 족보끼리 색이 겹치지 않게 차례로 고른다
	TMap<int32, FLinearColor> SynCol;
	if (bFuseMode)
	{
		// 서로 뚜렷이 다른 색 (D-201): 금 · 자홍 · 연두 · 파랑 · 주황 · 흰빛 (합쳐질 칸 하늘색 5ff2ff 과도 안 겹치게)
		const FLinearColor Pal[5] = {Hex(TEXT("ffd24a")), Hex(TEXT("ff4fd8")), Hex(TEXT("7dff5a")), Hex(TEXT("5a8cff")), Hex(TEXT("f0f0ff"))}; // 금과 비슷하던 주황은 뺐다 (D-206)
		TMap<int32, int32> Used; // 칸 → 색 번호
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
		{
			if (!G.CanFuse(I) || (int32)G.Synergies[I].Kind != HandFilter) continue;
			uint32 Taken = 0;
			for (int K : G.Synergies[I].Cells)
				for (int32 N = 0; N < 4; ++N)
				{
					static const int32 D[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}};
					const int32 X = K % Cols + D[N][0], Y = K / Cols + D[N][1];
					if (X < 0 || Y < 0 || X >= Cols || Y >= Rows) continue;
					if (const int32* C = Used.Find(CellKey(X, Y))) Taken |= 1u << *C;
				}
			int32 Ci = 0;
			while (Ci < 4 && (Taken >> Ci) & 1u) ++Ci;
			for (int K : G.Synergies[I].Cells) Used.Add(K, Ci);
			SynCol.Add(I, Pal[Ci]);
		}
	}
	bool bAnyFuseHov = false; // 마우스가 어느 후보 위에 있을 때만 나머지 후보를 누른다 (D-206)
	if (bFuseMode && HoverX >= 0)
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
			if (G.CanFuse(I) && (int32)G.Synergies[I].Kind == HandFilter && std::find(G.Synergies[I].Cells.begin(), G.Synergies[I].Cells.end(), CellKey(HoverX, HoverY)) != G.Synergies[I].Cells.end()) bAnyFuseHov = true;
	if (bFuseMode)
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
		{
			if (!G.CanFuse(I) || (int32)G.Synergies[I].Kind != HandFilter) continue;
			const FPDSynergy FS = FuseSyn(G, I); // 조커가 낀 족보는 합성 불가 (D-200)
			const int32 HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
			const bool bHov = std::find(FS.Cells.begin(), FS.Cells.end(), HKey) != FS.Cells.end();
			// 재료 칸 색이 족보 색 → 흰빛 → 족보 색으로 천천히 오간다 (D-122)
			const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
			const FLinearColor Pick = FMath::Lerp(SynCol.FindRef(I), FLinearColor::White, 0.2f * Wv);
			// 합성 대상 표시 (D-121, D-201): 족보 하나를 한 덩어리로 — 채움은 같은 족보 칸끼리 이어 붙이고, 테두리는 덩어리 바깥 변에만.
			// 그래서 같은 족보 둘이 붙어 있어도 사이에 틈과 테두리가 생기고 색도 달라 갈라져 보인다
			auto SameSyn = [&FS](int X, int Y) { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && std::find(FS.Cells.begin(), FS.Cells.end(), CellKey(X, Y)) != FS.Cells.end(); };
			for (int K : FS.Cells)
				{
					const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
					const int CX = K % Cols, CY = K / Cols;
					const bool bL = SameSyn(CX - 1, CY), bR = SameSyn(CX + 1, CY), bU = SameSyn(CX, CY - 1), bD = SameSyn(CX, CY + 1);
					const float In = 4.f, X0 = C0.X + (bL ? 0.f : In), X1 = C0.X + CS - (bR ? 0.f : In), Y0 = C0.Y + (bU ? 0.f : In), Y1 = C0.Y + CS - (bD ? 0.f : In);
					// 마우스가 안 올라간 후보는 한 단계 눌러 둔다 (D-204): 옅은 어둠 + 옅은 채움 + 가는 테두리. 올라간 족보만 밝게
					if (!bHov && bAnyFuseHov) F.Box(C0.X, C0.Y, CS, CS, Hex(TEXT("02020a"), 0.6f));
					F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Pick, (bHov ? 0.38f + 0.14f * Wv : bAnyFuseHov ? 0.06f : 0.14f + 0.08f * Wv) * 0.9f));
					const FLinearColor Ec = WithAlpha(Pick, (bHov ? 1.f : bAnyFuseHov ? 0.5f : 0.9f) * 0.9f);
					const float Ew = bHov ? 3.4f : bAnyFuseHov ? 1.3f : 2.2f, Eg = (bHov ? 1.6f + 1.f * Wv : bAnyFuseHov ? 0.3f : 0.9f + 0.6f * Wv) * 0.9f; // 눈부셔서 10% 낮춤 (D-208)
					if (!bU) F.Glow({FVector2f(X0, Y0), FVector2f(X1, Y0)}, Ec, Ew, Eg);
					if (!bD) F.Glow({FVector2f(X0, Y1), FVector2f(X1, Y1)}, Ec, Ew, Eg);
					if (!bL) F.Glow({FVector2f(X0, Y0), FVector2f(X0, Y1)}, Ec, Ew, Eg);
					if (!bR) F.Glow({FVector2f(X1, Y0), FVector2f(X1, Y1)}, Ec, Ew, Eg);
				}
			if (bHov) // 마우스 칸 = 합쳐질 칸: 하늘색 테두리 + 퍼지는 고리 + "여기", 다른 재료 칸에서 점선
			{
				bFuseHov = true;
				FuseHovSyn = I;
				const FLinearColor TargetCol = Hex(TEXT("5ff2ff"));
				const FVector2f C0 = CellPx((float)HoverX, (float)HoverY);
				F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), WithAlpha(TargetCol, 0.9f), 3.8f, (1.8f + Wv) * 0.9f);
				for (int32 Ring = 0; Ring < 2; ++Ring)
				{
					const float U = FMath::Fmod(Clock * 1.4f + Ring * 0.5f, 1.f), Gr = 2.f + 12.f * U;
					F.Glow(FCanvas::RoundRectPoints(C0.X - Gr, C0.Y - Gr, CS + Gr * 2.f, CS + Gr * 2.f, 8.f + Gr * 0.5f), WithAlpha(TargetCol, 0.81f * (1.f - U)), 2.2f, 1.f);
				}
				for (int K : FS.Cells) if (K != HKey) F.Dashed({CellCenter(K), CellCenter(HKey)}, WithAlpha(TargetCol, 0.63f), 1.6f, 4.f, 5.f);
				F.Badge(TEXT("여기로 합성"), C0.X + CS / 2.f, C0.Y + CS + 20.f, TargetCol, 1.f);
				BadgeRects.Add(FBox2f(FVector2f(C0.X - 20.f, C0.Y + CS + 11.f), FVector2f(C0.X + CS + 20.f, C0.Y + CS + 29.f))); // 툴팁이 피할 자리 // 어두운 바탕 딱지로 칸 아래에 (D-205): 위쪽은 족보 이름표·퍼지는 고리와 겹쳐 안 보였다
			}
		}
	// 먹이기 목록 (D-213): 고른(또는 목록에서 마우스를 올린) 조커와 그 조커가 먹을 족보만 밝게, 족보는 덩어리로. 판 위 [먹이기] 버튼·"먹이N" 딱지는 없앴다
	bool bFeedBtnHov = false;
	int32 FeedHovSyn = -1;
	if (InFeed() && !bPv && G.Phase != EPDPhase::Reward && PickCells.empty())
	{
		int32 JK = FeedJoker;
		bool bPreview = false;
		if (JK < 0) if (const int32 R = FeedListAt(Mouse); R >= 0) { JK = FeedList[R].Key; bPreview = true; }
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
		TSet<int32> Lit;
		std::vector<FPDFeed> Feeds;
		if (JK >= 0)
		{
			Feeds = G.FeedTargets(JK);
			Lit.Add(JK);
			for (const FPDFeed& Fd : Feeds) for (int K : G.Synergies[Fd.Syn].Cells) Lit.Add(K);
		}
		else for (const FFeedEntry& E : FeedList) Lit.Add(E.Key);
		{
			const int32 DL = F.NextLayer();
			for (int32 K = 0; K < Cols * Rows; ++K)
			{
				if (Lit.Contains(K)) continue;
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.BoxAt(DL, C0.X, C0.Y, CS, CS, Hex(TEXT("02020a"), G.Path.IsPath(K % Cols, K / Cols) ? 0.2f : 0.82f));
			}
			F.NextLayer();
		}
		auto Block = [&](const std::vector<int>& Cells, const FLinearColor& Col, float Fill, float Ew, float Eg) {
			auto Same = [&Cells](int X, int Y) { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && std::find(Cells.begin(), Cells.end(), CellKey(X, Y)) != Cells.end(); };
			for (int K : Cells)
			{
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				const int CX = K % Cols, CY = K / Cols;
				const bool bL = Same(CX - 1, CY), bR = Same(CX + 1, CY), bU = Same(CX, CY - 1), bD = Same(CX, CY + 1);
				const float In = 4.f, X0 = C0.X + (bL ? 0.f : In), X1 = C0.X + CS - (bR ? 0.f : In), Y0 = C0.Y + (bU ? 0.f : In), Y1 = C0.Y + CS - (bD ? 0.f : In);
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, Fill));
				if (!bU) F.Glow({FVector2f(X0, Y0), FVector2f(X1, Y0)}, Col, Ew, Eg);
				if (!bD) F.Glow({FVector2f(X0, Y1), FVector2f(X1, Y1)}, Col, Ew, Eg);
				if (!bL) F.Glow({FVector2f(X0, Y0), FVector2f(X0, Y1)}, Col, Ew, Eg);
				if (!bR) F.Glow({FVector2f(X1, Y0), FVector2f(X1, Y1)}, Col, Ew, Eg);
			}
		};
		// 조커: 분홍 고리
		auto Ring = [&](int32 K, float A) {
			const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
			F.Glow(FCanvas::RoundRectPoints(C0.X - 2.f, C0.Y - 2.f, CS + 4.f, CS + 4.f, RadSmall), WithAlpha(JokerPink, A), 2.4f, (1.f + 0.8f * Wv) * A);
		};
		if (JK >= 0) Ring(JK, 1.f);
		else for (const FFeedEntry& E : FeedList) if (E.N > 0) Ring(E.Key, 0.7f);
		const int32 HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		const FLinearColor Pal[5] = {Hex(TEXT("ffd24a")), Hex(TEXT("7dff5a")), Hex(TEXT("5a8cff")), Hex(TEXT("f0f0ff")), Hex(TEXT("ff4fd8"))};
		bool bAnyHov = false;
		for (const FPDFeed& Fd : Feeds) { const std::vector<int>& Cs = G.Synergies[Fd.Syn].Cells; bAnyHov = bAnyHov || (std::find(Cs.begin(), Cs.end(), HKey) != Cs.end() && HKey != JK); }
		for (int32 I = 0; I < (int32)Feeds.size(); ++I)
		{
			std::vector<int> Cs;
			for (int K : G.Synergies[Feeds[I].Syn].Cells) if (!G.Towers.at(K).IsJoker()) Cs.push_back(K); // 먹히는 칸만 (조커는 남는다)
			const bool bH = !bPreview && std::find(Cs.begin(), Cs.end(), HKey) != Cs.end();
			const FLinearColor Col = Pal[I % 5];
			Block(Cs, WithAlpha(Col, bH ? 0.95f : bAnyHov ? 0.5f : 0.85f), bH ? 0.3f : bAnyHov ? 0.06f : 0.14f, bH ? 3.f : bAnyHov ? 1.3f : 2.f, bH ? 1.4f + 0.8f * Wv : bAnyHov ? 0.3f : 0.8f);
			if (bH)
			{
				bFeedBtnHov = true;
				FeedHovSyn = Feeds[I].Syn;
				for (int K : Cs) F.Dashed({CellCenter(K), CellCenter(JK)}, WithAlpha(JokerPink, 0.7f), 1.4f, 4.f, 5.f);
			}
		}
	}
	// 자리바꿈 모드 (D-149): 고른 타워는 민트색으로 반짝, 마우스가 올라간 칸에 바꿀 자리 표시
	if (bSwapMode)
	{
		const FLinearColor SwC = Hex(TEXT("6affc8"));
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 5.f);
		const int32 HoverKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		if (SwapFrom < 0) // 옮길 수 있는 타워들에 옅은 테두리
			for (const auto& KV : G.Towers)
			{
				const FVector2f C0 = CellPx((float)(KV.first % Cols), (float)(KV.first / Cols));
				F.Glow(FCanvas::RoundRectPoints(C0.X + 3.f, C0.Y + 3.f, CS - 6.f, CS - 6.f, RadSmall), WithAlpha(SwC, KV.first == HoverKey ? 1.f : 0.45f), KV.first == HoverKey ? 2.6f : 1.2f, KV.first == HoverKey ? 1.4f : 0.4f + 0.3f * Wv);
			}
		else
		{
			const FVector2f A = CellPx((float)(SwapFrom % Cols), (float)(SwapFrom / Cols));
			F.Box(A.X + 2.f, A.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(SwC, 0.22f + 0.15f * Wv));
			F.Glow(FCanvas::RoundRectPoints(A.X + 2.f, A.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), SwC, 3.f, 1.4f + 0.8f * Wv);
			if (HoverKey >= 0 && HoverKey != SwapFrom)
			{
				const bool bOk = G.Towers.count(HoverKey) || G.Buildable(HoverX, HoverY);
				const FLinearColor Tc = bOk ? SwC : Hex(TEXT("ff5a6a"));
				const FVector2f B = CellPx((float)HoverX, (float)HoverY);
				F.Glow(FCanvas::RoundRectPoints(B.X + 2.f, B.Y + 2.f, CS - 4.f, CS - 4.f, RadSmall), Tc, 2.4f, 1.2f);
				if (bOk) F.Dashed({CellCenter(SwapFrom), CellCenter(HoverKey)}, WithAlpha(SwC, 0.8f), 1.6f, 5.f, 4.f);
			}
		}
		F.Badge(SwapFrom < 0 ? TrF(TEXT("자리바꿈 (%d번 남음): 옮길 타워 클릭 · 오른쪽 클릭 / Esc: 끄기"), G.Swaps)
							 : FString(TEXT("바꿀 자리 클릭 (타워면 서로 바꾸고 빈 칸이면 옮김) · 같은 타워 다시: 다시 고르기")),
			FX + Cols * CS / 2.f, FY + 12.f, SwC, 1.f);
	}
	// 족보 필터 안내 (D-148)
	if (HandFilter >= 0 && PickCells.empty() && !bSwapMode)
	{
		if (InFeed())
		{
			if (FeedJoker < 0) F.Badge(TEXT("먹이기: 왼쪽 목록이나 판에서 조커를 고르세요 · Esc: 끄기"), FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
			else
			{
				const bool bAny = !G.FeedTargets(FeedJoker).empty();
				F.Badge(bAny ? TrF(TEXT("%s 먹이기: 먹일 족보 칸 클릭 · 다른 조커 클릭 = 바꾸기 · 오른쪽 클릭 / Esc = 목록으로"), JokerName(G.Towers.at(FeedJoker).Joker))
							 : FString(TEXT("이 조커에 먹일 족보가 없다 (조커에 맞닿은 족보가 먹이) · Esc = 목록으로")), FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
			}
		}
		else if (InCraft())
		{
			if (CraftUpper >= 0) F.Badge(TrF(TEXT("%s 만들기: 두 조커 중 하나를 클릭 = 그 칸에 상위 조커 · 오른쪽 클릭 / Esc = 목록으로"), (G.UpperFound >> CraftUpper) & 1u ? UpperName(CraftUpper) : TEXT("???")), FX + Cols * CS / 2.f, FY + 12.f, Gold, 1.f);
			else if (CraftJoker < 0) F.Badge(TEXT("조커 만들기: 왼쪽 목록에서 조커를 고르세요 (마우스를 올리면 재료가 보인다) · Esc: 끄기"), FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
			else
			{
				const std::vector<int> PIdx = CraftNodeIdx();
				const bool bDone = PIdx.size() == CraftPicks.size() && G.JokerCraftResult(PIdx) == (EPDJoker)CraftJoker;
				F.Badge(TrF(TEXT("%s 만들기: 재료 %d개 고름 · 후보 칸 클릭 = 재료로 (추천 = 게임이 고른 재료)"), (G.Discovered >> CraftJoker) & 1u ? JokerName(CraftJoker) : TEXT("???"), (int32)CraftPicks.size()), FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
				F.Badge(bDone ? FString(TEXT("레시피 완성: 고른 재료 칸 클릭 = 그 칸에 조커 · 오른쪽 클릭 = 재료 빼기 · Esc = 목록으로"))
							  : FString(TEXT("고른 재료 클릭 = 빼기 · 다른 곳 오른쪽 클릭 / Esc = 목록으로")), FX + Cols * CS / 2.f, FY + 32.f, bDone ? Hex(TEXT("5ff2ff")) : Hex(TEXT("cfd8e6")), 1.f);
			}
		}
		else
		{
			bool bCan = false;
			for (int32 I = 0; I < (int32)G.Synergies.size(); ++I) bCan = bCan || ((int32)G.Synergies[I].Kind == HandFilter && G.CanFuse(I));
			if (!bCan) // 합성할 수 있을 땐 판 위 안내 꼬리표를 띄우지 않는다 (D-374): 빛나는 칸과 커서 꼬리표로 충분
				F.Badge(TrF(TEXT("%s: 합성할 수 있는 족보가 없다 (최대 레벨이거나 조커가 낌) · Esc: 끄기"), HandName((EPDHand)HandFilter)),
					FX + Cols * CS / 2.f, FY + 12.f, HandColor((EPDHand)HandFilter), 1.f);
		}
	}
	// 조커를 만들 수 있는 재료 칸 반짝임 (먹이기·만들기 모드에선 숨김). 버튼은 없애고 [조커 만들기]로 (D-195)
	if (!bPv && G.Phase != EPDPhase::Reward && !InFeed() && !InCraft() && PickCells.empty())
		for (const FPDJokerGroup& Gr : JGroups)
		{
			// 조커를 만들 수 있는 재료 칸은 늘 반짝인다 (D-115): 칸마다 박자를 어긋나게 한 분홍 테두리 + 작은 별빛
			for (int K : Gr.Cells)
			{
				const float Tw = 0.5f + 0.5f * FMath::Sin(Clock * 5.f + K * 1.7f);
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.Glow(FCanvas::RoundRectPoints(C0.X + 3.f, C0.Y + 3.f, CS - 6.f, CS - 6.f, RadSmall), WithAlpha(JokerPink, 0.35f + 0.55f * Tw), 1.4f, 0.5f + 0.9f * Tw);
				if (Tw > 0.8f) { const FVector2f S = C0 + FVector2f(CS - 9.f, 9.f); const float L = 3.f + 4.f * (Tw - 0.8f) * 5.f;
					F.Lines({S - FVector2f(L, 0.f), S + FVector2f(L, 0.f)}, WithAlpha(FLinearColor::White, Tw), 1.2f); F.Lines({S - FVector2f(0.f, L), S + FVector2f(0.f, L)}, WithAlpha(FLinearColor::White, Tw), 1.2f); }
			}
		}
	// 조커 합치기 버튼 (같은 조커끼리, D-114): 평소엔 분홍 점 둘, 마우스를 올리면 버튼
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty() && !InFeed())
	{
		const std::vector<FPDJokerMerge> Ms = G.JokerMerges();
		const int32 HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		for (int32 I = 0; I < (int32)Ms.size(); ++I)
		{
			const FVector2f B = MergeButtonCenter(Ms[I].Cells);
			const bool bHov = InBox(Mouse, B, MergeW, MergeH);
			if (!bHov && std::find(Ms[I].Cells.begin(), Ms[I].Cells.end(), HKey) == Ms[I].Cells.end())
			{
				F.Glow(FCanvas::CirclePoints(B - FVector2f(4.f, 0.f), 2.4f, 10), JokerPink, 1.5f, 0.6f + 0.3f * Pulse);
				F.Glow(FCanvas::CirclePoints(B + FVector2f(4.f, 0.f), 2.4f, 10), JokerPink, 1.5f, 0.6f + 0.3f * Pulse);
				continue;
			}
			if (bHov)
			{
				for (int K : Ms[I].Cells) { const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols)); F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(JokerPink, 0.12f)); }
				F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::JokerMerge, I), B.X, B.Y - MergeH - 6.f);
			}
			BadgeRects.Add(FBox2f(FVector2f(B.X - MergeW / 2.f, B.Y - MergeH / 2.f), FVector2f(B.X - MergeW / 2.f + MergeW, B.Y - MergeH / 2.f + MergeH))); // 툴팁이 피할 자리 (D-150)
			F.Box(B.X - MergeW / 2.f, B.Y - MergeH / 2.f, MergeW, MergeH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - MergeW / 2.f, B.Y - MergeH / 2.f, MergeW, MergeH, RadPill(MergeH)), JokerPink, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.6f);
			if (Ms[I].Upper >= 0) // 상위 조커 (D-231): 처음이면 이름은 ???
				F.TextCentered((G.UpperFound >> Ms[I].Upper) & 1u ? TrF(TEXT("상위: %s"), UpperName(Ms[I].Upper)) : FString(Tr(TEXT("상위 조커 ???"))), B.X, B.Y, WithAlpha(Gold, bHov ? 1.f : 0.85f), 10.f, true, MergeW - 6.f);
			else F.TextCentered(TEXT("조커 합치기"), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.85f), 10.f, true, MergeW - 6.f);
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
	// 평소 마우스를 올린 타워의 족보 영역 (D-215): 합성 모드처럼 족보마다 한 덩어리 (족보 색, 바깥 변에만 테두리)
	if (!bPv && HandFilter < 0 && PickCells.empty() && !bSwapMode && Sel < 0 && G.Phase != EPDPhase::Reward)
	{
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
		for (const FPDSynergy& S : HovSyn)
		{
			const std::vector<int>& Cells = S.Cells;
			const FLinearColor Col = HandColor(S.Kind);
			auto Same = [&Cells](int X, int Y) { return X >= 0 && Y >= 0 && X < Cols && Y < Rows && std::find(Cells.begin(), Cells.end(), CellKey(X, Y)) != Cells.end(); };
			for (int K : Cells)
			{
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				const int CX = K % Cols, CY = K / Cols;
				const bool bL = Same(CX - 1, CY), bR = Same(CX + 1, CY), bU = Same(CX, CY - 1), bD = Same(CX, CY + 1);
				const float In = 4.f, X0 = C0.X + (bL ? 0.f : In), X1 = C0.X + CS - (bR ? 0.f : In), Y0 = C0.Y + (bU ? 0.f : In), Y1 = C0.Y + CS - (bD ? 0.f : In);
				F.Box(X0, Y0, X1 - X0, Y1 - Y0, WithAlpha(Col, 0.12f + 0.06f * Wv));
				const FLinearColor Ec = WithAlpha(Col, 0.85f);
				const float Ew = 2.2f, Eg = 0.9f + 0.5f * Wv;
				if (!bU) F.Glow({FVector2f(X0, Y0), FVector2f(X1, Y0)}, Ec, Ew, Eg);
				if (!bD) F.Glow({FVector2f(X0, Y1), FVector2f(X1, Y1)}, Ec, Ew, Eg);
				if (!bL) F.Glow({FVector2f(X0, Y0), FVector2f(X0, Y1)}, Ec, Ew, Eg);
				if (!bR) F.Glow({FVector2f(X1, Y0), FVector2f(X1, Y1)}, Ec, Ew, Eg);
			}
		}
	}
	// 판매 버튼 (D-221): 오른쪽 클릭한 타워 둘레에 금색 고리, 아래에 [판매 +N 골드]
	SellBtnRect = FBox2f(FVector2f(-1.f, -1.f), FVector2f(-1.f, -1.f));
	if (SellCell >= 0 && G.Towers.count(SellCell))
	{
		const int32 Price = G.SellPrice(SellCell);
		const FVector2f C0 = CellPx((float)(SellCell % Cols), (float)(SellCell / Cols));
		F.Glow(FCanvas::RoundRectPoints(C0.X + 1.f, C0.Y + 1.f, CS - 2.f, CS - 2.f, RadSmall), Gold, 2.4f, 1.f + 0.6f * Pulse);
		constexpr float BW = 120.f, BHt = 24.f;
		const float BX = FMath::Clamp(C0.X + CS / 2.f - BW / 2.f, FX, FX + Cols * CS - BW), BY = C0.Y + CS + 6.f > FY + Rows * CS - BHt ? C0.Y - 6.f - BHt : C0.Y + CS + 6.f;
		SellBtnRect = FBox2f(FVector2f(BX, BY), FVector2f(BX + BW, BY + BHt));
		const bool bH = SellBtnRect.IsInside(Mouse);
		F.Box(BX, BY, BW, BHt, Hex(TEXT("06060b"), 0.94f));
		if (bH) F.Box(BX, BY, BW, BHt, WithAlpha(Gold, 0.18f));
		F.Glow(FCanvas::RoundRectPoints(BX, BY, BW, BHt, RadPill(BHt)), Gold, bH ? 1.8f : 1.3f, bH ? 1.2f : 0.7f + 0.3f * Pulse);
		F.TextCentered(TrF(TEXT("판매 +%d 경험치"), Price), BX + BW / 2.f, BY + BHt / 2.f, bH ? HotCol(Gold) : Gold, 11.f, true, BW - 8.f);
		BadgeRects.Add(SellBtnRect);
	}
	// 타워 정보 (마우스를 올리면). 맨 마지막에 그려 이름표·떠오르는 글자·배너가 위에 겹치지 않게 (D-150)
	if (HoverX >= 0 && Sel < 0 && G.Phase != EPDPhase::Reward && MenuCell < 0) // 칸 메뉴 상자가 열려 있으면 그 상자가 툴팁 자리 (D-312)
	{
		auto It = G.Towers.find(CellKey(HoverX, HoverY));
		if (It != G.Towers.end())
		{
			const FPDTower& T = It->second;
			const FPDTowerStats St = TowerStats(T, G.CharmMask(), &G.Mods);
			float Pw = 0.f;
			for (const auto& KV : PowerList) if (KV.first == It->first) Pw = KV.second;
			const FString PowLine = TrF(TEXT("위력 %s/초 (판의 %d%%)"), *ShortNum(Pw), PowerSum > 0.f ? FMath::RoundToInt(Pw / PowerSum * 100.f) : 0);
			const FLinearColor PowCol = Hex(TEXT("9cffb0"));
			// 툴팁 자리 (D-150): 칸의 오른쪽·왼쪽·위·아래·대각선 중 판 안에서 이름표·버튼·그 칸을 가장 덜 가리는 곳
			auto PlaceTip = [&](float TW, float TH) {
				const FVector2f C0 = CellPx((float)HoverX, (float)HoverY);
				const FBox2f Cell(C0 - FVector2f(4.f, 4.f), C0 + FVector2f(CS + 4.f, CS + 4.f));
				const float Gap = 8.f, MidY = C0.Y + CS / 2.f - TH / 2.f, MidX = C0.X + CS / 2.f - TW / 2.f;
				const float Far = 20.f; // 이름표 한 줄만큼 더 떨어진 자리
				const FVector2f Cand[12] = {{C0.X + CS + Gap, MidY}, {C0.X - Gap - TW, MidY}, {C0.X + CS + Gap, C0.Y - TH + CS * 0.3f}, {C0.X - Gap - TW, C0.Y - TH + CS * 0.3f},
					{MidX, C0.Y - Gap - TH}, {MidX, C0.Y + CS + Gap}, {C0.X + CS + Gap, C0.Y + CS * 0.7f}, {C0.X - Gap - TW, C0.Y + CS * 0.7f},
					{MidX, C0.Y - Gap - TH - Far}, {MidX, C0.Y + CS + Gap + Far}, {C0.X + CS + Gap + Far, MidY}, {C0.X - Gap - TW - Far, MidY}};
				const FBox2f Board(FVector2f(FX, FY), FVector2f(FX + Cols * CS, FY + Rows * CS));
				FVector2f Best = Cand[0];
				float BestScore = TNumericLimits<float>::Max();
				for (const FVector2f& Cn : Cand)
				{
					// 화면 안이면 판 밖(패널 위)도 된다. 판 밖으로 나간 넓이는 조금만 벌점
					const FVector2f Pp(FMath::Clamp(Cn.X, 8.f, DesignW - TW - 8.f), FMath::Clamp(Cn.Y, HudY + HudH + 4.f, DesignH - TH - 8.f));
					const FBox2f Tb(Pp, Pp + FVector2f(TW, TH));
					auto Over = [&](const FBox2f& O) { return FMath::Max(0.f, FMath::Min(Tb.Max.X, O.Max.X) - FMath::Max(Tb.Min.X, O.Min.X)) * FMath::Max(0.f, FMath::Min(Tb.Max.Y, O.Max.Y) - FMath::Max(Tb.Min.Y, O.Min.Y)); };
					float Score = Over(Cell) * 20.f + (TW * TH - Over(Board)) * 0.5f;
					for (const FBox2f& O : BadgeRects) { const float A = Over(O); if (A > 0.f) Score += 30000.f + A * 4.f; } // 이름표·버튼을 조금이라도 가리면 판 밖으로 나가는 것보다 나쁘다
					if (Score < BestScore - 0.5f) { BestScore = Score; Best = Pp; }
				}
				return Best;
			};
			// 족보·연쇄 줄 (D-202): "풀하우스 · 투 페어 · 포카드 · 연쇄 ×4.5" 를 족보 색으로 이어 쓰고 넘치면 다음 줄
			// 족보 줄 (D-202, D-215): 족보마다 "이름  효과" 한 줄, 연쇄 줄, 이 타워에 실제로 붙은 족보 배율 줄
			auto SynLines = [&](bool bDraw, float X0, float Y0, float W) -> int32 {
				if (HovSyn.empty()) return 0;
				int32 Lines = 0;
				int32 Chain = 1;
				for (const FPDSynergy& S : HovSyn)
				{
					FString L = HandName(S.Kind);
					if (S.Kind == EPDHand::Flush || S.Kind == EPDHand::Straight) L += FString::FromInt(S.N);
					if (bDraw)
					{
						const float Y = Y0 + Lines * 16.f;
						F.Text(L, X0, Y, HandColor(S.Kind), 11.f, 0, true, W * 0.5f);
						const float Nw = FMath::Min(W * 0.5f, FCanvas::Measure(L, FCanvas::Font(11.f, true)).X);
						F.Text(HandEffect(S.Kind), X0 + Nw + 8.f, Y, Fg, 10.f, 0, false, W - Nw - 8.f);
					}
					++Lines;
					Chain = FMath::Max(Chain, S.Chain);
				}
				if (Chain >= 2)
				{
					if (bDraw) F.Text(TrF(TEXT("연쇄 ×%s (족보 %d개가 이어짐)"), *FString::SanitizeFloat(1.f + ChainStep * (Chain - 1)), Chain), X0, Y0 + Lines * 16.f, Gold, 10.f, 0, false, W);
					++Lines;
				}
				if (It != G.Towers.end())
				{
					const FPDEffects& E = It->second.Eff;
					if (bDraw) F.Text(TrF(TEXT("이 타워에 적용: 공격 ×%.2f · 연사 ×%.2f · 사거리 ×%.2f%s"), E.Dmg, E.Rate, E.Range, E.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), E.Splash) : TEXT("")),
						X0, Y0 + Lines * 16.f, Gold, 10.f, 0, true, W);
					++Lines;
				}
				return Lines;
			};
			if (bFeedBtnHov && FeedHovSyn >= 0 && FeedJoker >= 0)
			{
				// 먹이기 전후 (D-213): 사본에서 먹여 보고 조커 레벨·위력, 판 위력, 사라지는 카드 수
				FPDGame Copy = G;
				Copy.bMuted = true;
				const FPDTower& J0 = G.Towers.at(FeedJoker);
				int32 Gone = 0;
				for (int K : G.Synergies[FeedHovSyn].Cells) Gone += !G.Towers.at(K).IsJoker();
				if (Copy.Feed(FeedHovSyn, FeedJoker) && Copy.Towers.count(FeedJoker))
				{
					const FPDTower& J1 = Copy.Towers.at(FeedJoker);
					float PwB = 0.f, PwA = 0.f;
					for (const auto& KV : PowerList) if (KV.first == FeedJoker) PwB = KV.second;
					for (const auto& KV : Copy.TowerPowers()) if (KV.first == FeedJoker) PwA = KV.second;
					const float BoardB = PowerSum, BoardA = Copy.Power();
					const float Pj = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f, Pb = BoardB > 0.f ? (BoardA / BoardB - 1.f) * 100.f : 0.f;
					const FLinearColor Up = Hex(TEXT("9cffb0")), Down = Hex(TEXT("ff6a6a"));
					constexpr float TW = 290.f, TH = 92.f;
					const FVector2f P = PlaceTip(TW, TH);
					F.Box(P.X, P.Y, TW, TH, Hex(TEXT("06060b"), 0.94f));
					F.Glow(FCanvas::RoundRectPoints(P.X, P.Y, TW, TH, RadSmall), JokerPink, 1.2f, 0.4f);
					F.Text(TrF(TEXT("%s 먹이기 → %s"), *HandLabel(G.Synergies[FeedHovSyn]), JokerName(J0.Joker)), P.X + 10.f, P.Y + 18.f, JokerPink, 12.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("조커 레벨 %d → %d · 경험치 %.0f → %.0f"), J0.JLevel, J1.JLevel, J0.JXp, J1.JXp), P.X + 10.f, P.Y + 36.f, Fg, 11.f, 0, false, TW - 20.f);
					F.Text(TrF(TEXT("조커 위력 %s/초 → %s/초 (%s%.0f%%)"), *ShortNum(PwB), *ShortNum(PwA), Pj >= 0.f ? TEXT("+") : TEXT(""), Pj), P.X + 10.f, P.Y + 54.f, Pj >= 0.f ? Up : Down, 11.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("판 위력 %s → %s (%s%.0f%%) · 먹인 카드 %d장은 사라짐"), *ShortNum(BoardB), *ShortNum(BoardA), Pb >= 0.f ? TEXT("+") : TEXT(""), Pb, Gone), P.X + 10.f, P.Y + 72.f, Pb >= 0.f ? Up : Down, 10.f, 0, false, TW - 20.f);
				}
			}
			else if (bCraftHov && CraftJoker >= 0)
			{
				// 조커 전후 (D-210): 사본에서 이 칸에 조커를 만들어 보고, 재료 위력 합 → 조커 위력, 이 칸 타워 → 조커
				FPDGame Copy = G;
				Copy.bMuted = true;
				const int32 HK = It->first;
				std::vector<int> Mats;
				for (const std::vector<int>& Pk : CraftPicks) for (int K : Pk) if (std::find(Mats.begin(), Mats.end(), K) == Mats.end()) Mats.push_back(K);
				if (Copy.JokerCraft(CraftNodeIdx(), HK) && Copy.Towers.count(HK))
				{
					const FPDTower& R = Copy.Towers.at(HK);
					const FPDTowerStats Rs = TowerStats(R, Copy.CharmMask(), &Copy.Mods);
					float PwB = 0.f, PwA = 0.f;
					for (const auto& KV : PowerList) if (std::find(Mats.begin(), Mats.end(), KV.first) != Mats.end()) PwB += KV.second;
					for (const auto& KV : Copy.TowerPowers()) if (KV.first == HK) PwA = KV.second;
					const float Pct = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f;
					constexpr float TW = 280.f, TH = 124.f;
					const FVector2f P = PlaceTip(TW, TH);
					F.Box(P.X, P.Y, TW, TH, Hex(TEXT("06060b"), 0.94f));
					F.Glow(FCanvas::RoundRectPoints(P.X, P.Y, TW, TH, RadSmall), JokerPink, 1.2f, 0.4f);
					F.Text(TrF(TEXT("만들면: %s · 재료 %d장"), (G.Discovered >> CraftJoker) & 1u ? JokerName(CraftJoker) : TEXT("???"), (int32)Mats.size()), P.X + 10.f, P.Y + 18.f, JokerPink, 12.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("위력 %s/초 → %s/초 (%s%.0f%%)"), *ShortNum(PwB), *ShortNum(PwA), Pct >= 0.f ? TEXT("+") : TEXT(""), Pct), P.X + 10.f, P.Y + 38.f, Pct >= 0.f ? Hex(TEXT("9cffb0")) : Hex(TEXT("ff6a6a")), 11.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("재료 %d장의 위력 합 → 조커 하나"), (int32)Mats.size()), P.X + 10.f, P.Y + 54.f, Grey, 9.f, 0, false, TW - 20.f);
					F.Text(TrF(TEXT("공격력 %.0f → %.0f · 초당 %.1f → %.1f발"), St.Dmg, Rs.Dmg, St.Rate, Rs.Rate), P.X + 10.f, P.Y + 72.f, Fg, 11.f, 0, false, TW - 20.f);
					F.Text(TrF(TEXT("사거리 %.1f → %.1f · 범위 %.1f → %.1f"), St.Range, Rs.Range, St.Splash, Rs.Splash), P.X + 10.f, P.Y + 90.f, Grey, 11.f, 0, false, TW - 20.f);
					F.Text(JokerAbility(CraftJoker), P.X + 10.f, P.Y + 108.f, Fg, 10.f, 0, false, TW - 20.f);
				}
			}
			else if (bFuseHov && FuseHovSyn >= 0)
			{
				// 합성 전후 (D-209): 사본에서 이 칸으로 합성해 보고, 재료 위력 합 → 결과 타워 위력, 이 칸 타워의 한 발·연사·사거리 → 결과
				FPDGame Copy = G;
				Copy.bMuted = true;
				const int32 HK = It->first;
				const std::vector<int> Mats = FuseSyn(G, FuseHovSyn).Cells;
				if (Copy.Fuse(FuseHovSyn, HK) && Copy.Towers.count(HK))
				{
					const FPDTower& R = Copy.Towers.at(HK);
					const FPDTowerStats Rs = TowerStats(R, Copy.CharmMask(), &Copy.Mods);
					float PwB = 0.f, PwA = 0.f;
					for (const auto& KV : PowerList) if (std::find(Mats.begin(), Mats.end(), KV.first) != Mats.end()) PwB += KV.second;
					for (const auto& KV : Copy.TowerPowers()) if (KV.first == HK) PwA = KV.second;
					const float Pct = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f;
					const FLinearColor Up = Hex(TEXT("9cffb0")), Down = Hex(TEXT("ff6a6a"));
					constexpr float TW = 270.f, TH = 108.f;
					const FVector2f P = PlaceTip(TW, TH);
					F.Box(P.X, P.Y, TW, TH, Hex(TEXT("06060b"), 0.94f));
					F.Glow(FCanvas::RoundRectPoints(P.X, P.Y, TW, TH, RadSmall), Gold, 1.2f, 0.4f);
					const FString CardName = R.Card.IsWild() ? FString(Tr(TEXT("와일드"))) : RankText(R.Card.Rank);
					F.Text(TrF(TEXT("합성하면: %s %s · %s Lv%d"), *CardName, SuitName(R.Card.Suit), HandName((EPDHand)FMath::Max(0, R.FuseHand)), R.Level), P.X + 10.f, P.Y + 18.f, Gold, 12.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("위력 %s/초 → %s/초 (%s%.0f%%)"), *ShortNum(PwB), *ShortNum(PwA), Pct >= 0.f ? TEXT("+") : TEXT(""), Pct), P.X + 10.f, P.Y + 38.f, Pct >= 0.f ? Up : Down, 11.f, 0, true, TW - 20.f);
					F.Text(TrF(TEXT("재료 %d장의 위력 합 → 합성 타워 하나"), (int32)Mats.size()), P.X + 10.f, P.Y + 54.f, Grey, 9.f, 0, false, TW - 20.f);
					F.Text(TrF(TEXT("공격력 %.0f → %.0f · 초당 %.1f → %.1f발"), St.Dmg, Rs.Dmg, St.Rate, Rs.Rate), P.X + 10.f, P.Y + 72.f, Fg, 11.f, 0, false, TW - 20.f);
					F.Text(TrF(TEXT("사거리 %.1f → %.1f · 범위 %.1f → %.1f"), St.Range, Rs.Range, St.Splash, Rs.Splash), P.X + 10.f, P.Y + 90.f, Grey, 11.f, 0, false, TW - 20.f);
				}
			}
			else if (T.IsJoker())
			{
				const float SynH = SynLines(false, 0.f, 0.f, 226.f) * 16.f;
				const FVector2f Pj = PlaceTip(246.f, 140.f + SynH);
				const float JX = Pj.X, JY = Pj.Y;
				F.Box(JX, JY, 246.f, 140.f + SynH, Hex(TEXT("06060b"), 0.94f));
				F.Glow(FCanvas::RoundRectPoints(JX, JY, 246.f, 140.f + SynH, RadSmall), JokerPink, 1.2f, 0.4f);
				SynLines(true, JX + 10.f, JY + 150.f, 226.f);
				F.Text(JokerTitle(T.Joker), JX + 10.f, JY + 18.f, IsUpperJoker(T.Joker) ? Gold : JokerPink, 12.f, 0, true, 226.f);
				F.Text(PowLine, JX + 10.f, JY + 36.f, PowCol, 11.f, 0, true, 226.f);
				// 조커도 공격력·연사·치명·사거리 (D-187): 일반 타워 툴팁과 같은 두 줄
				F.Text(TrF(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), JX + 10.f, JY + 54.f, Fg, 11.f, 0, false, 226.f);
				F.Text(TrF(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? Tr(TEXT(" · 둔화")) : TEXT("")), JX + 10.f, JY + 70.f, Grey, 11.f, 0, false, 226.f);
				F.Text(JokerAbility(T.Joker), JX + 10.f, JY + 86.f, Fg, 10.f, 0, false, 226.f);
				F.Text(JokerCommand(T.Joker), JX + 10.f, JY + 102.f, Gold, 10.f, 0, false, 226.f);
				{ const int32 NA = (int32)G.FeedTargets(It->first).size(); F.Text(TrF(TEXT("클릭: 메뉴 (먹이기 %d) · 처치 +%.0f 경험치 (보스 +%.0f)"), NA, JokerKillXp, JokerBossXp), JX + 10.f, JY + 118.f, Grey, 10.f, 0, false, 226.f); }
				F.Text(T.JLevel >= JokerMaxLevel ? TrF(TEXT("조커 레벨 %d (최대) · 위력 +%d%%"), T.JLevel, FMath::RoundToInt((T.JLevel - 1) * JokerLevelDmg * 100.f))
					: TrF(TEXT("조커 레벨 %d · 경험치 %.0f/%.0f · 위력 +%d%%%s"), T.JLevel, T.JXp, JokerXpNeed(T.JLevel), FMath::RoundToInt((T.JLevel - 1) * JokerLevelDmg * 100.f), T.JLevel >= JokerUpLevel ? Tr(TEXT(" · 강화")) : TEXT("")),
					JX + 10.f, JY + 134.f, JokerPink, 10.f, 0, false, 226.f);
			}
			else
			{
			const float BH0 = (T.Level >= 2 ? 92.f : 76.f) + 16.f; // + 판매 안내 줄 (D-221)
			const float BH = BH0 + SynLines(false, 0.f, 0.f, 220.f) * 16.f;
			const FVector2f P = PlaceTip(240.f, BH);
			const float BX = P.X, BYp = P.Y;
			F.Box(BX, BYp, 240.f, BH, Hex(TEXT("06060b"), 0.94f));
			F.Glow(FCanvas::RoundRectPoints(BX, BYp, 240.f, BH, RadSmall), SuitColor(T.Card.Suit), 1.2f, 0.4f);
			{ // 합성 타워는 어떤 족보로 만들었는지 (D-172): "6 폭발 · 원 페어 Lv2"
				const FString CardName = T.Card.IsWild() ? FString(Tr(TEXT("와일드"))) : RankText(T.Card.Rank);
				const FString Title = T.Level >= 2 && T.FuseHand >= 0 && T.FuseHand < (int32)EPDHand::Count
					? TrF(TEXT("%s %s · %s Lv%d"), *CardName, SuitName(T.Card.Suit), HandName((EPDHand)T.FuseHand), T.Level)
					: TrF(TEXT("%s %s · 합성 레벨 %d"), *CardName, SuitName(T.Card.Suit), T.Level);
				F.Text(Title, BX + 10.f, BYp + 18.f, SuitColor(T.Card.Suit), 12.f, 0, true, 220.f);
			}
			F.Text(PowLine, BX + 10.f, BYp + 36.f, PowCol, 11.f, 0, true, 220.f);
			F.Text(TrF(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), BX + 10.f, BYp + 54.f, Fg, 11.f, 0, false, 220.f);
			F.Text(TrF(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? Tr(TEXT(" · 둔화")) : TEXT("")), BX + 10.f, BYp + 70.f, Grey, 11.f, 0, false, 220.f);
			if (T.Level >= 2) F.Text(TrF(TEXT("특성 공격 ×%.1f 연사 ×%.1f"), T.TDmg, T.TRate), BX + 10.f, BYp + 86.f, Gold, 10.f, 0, false, 220.f);
			{ int32 NF = 0; for (int32 Si = 0; Si < (int32)G.Synergies.size(); ++Si) if (G.CanFuse(Si)) { const std::vector<int> FC = FuseSyn(G, Si).Cells; NF += std::find(FC.begin(), FC.end(), It->first) != FC.end(); } // 합성 줄 수만 (메뉴 전체를 만들면 비싸다)
				F.Text(TrF(TEXT("클릭: 메뉴 (합성 %d · 판매 +%d 경험치)"), NF, G.SellPrice(It->first)), BX + 10.f, BYp + BH0 - 6.f, Hex(TEXT("8a94a6")), 9.f, 0, false, 220.f); } // D-312 ⑤
			SynLines(true, BX + 10.f, BYp + BH0 + 10.f, 220.f);
			}
		}
	}
	PaintCellMenu(F); // 칸 메뉴 상자 (D-312): 판 위 UI 맨 위
}

// 위쪽 상태 띠 (D-119): 웨이브·다음 보스 | 남은 적 | 목숨·골드 | 판 위력 | 속도·설정 버튼
void SPDBoardWidget::PaintHud(FCanvas& C) const
{
	// 윗줄 (Alice 시안 D-403): 야자수 · 네온 줄 · 분홍 타원 'WAVE' 는 시안 그림 조각, 오른쪽 파란 상자 안 웨이브 숫자는 코드 (속이 검고 테두리만 파랗게 빛남).
	// 시안에 없는 목숨 · 레벨은 상자 오른쪽 빈자리에 작은 판 하나. 오른쪽 위 '?' = 메뉴 (올리면 키 안내). 배속은 TV 아래, 일시정지는 P 키
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 간판 그림 (D-407): 야자수 · 네온 줄 · 분홍 타원 WAVE · 왼쪽이 열린 파란 상자(위 · 오른쪽 · 아래 테)까지 시안 그림 그대로. 상자 안 숫자만 코드
	const FSlateBrush* Sign = Art(EUiArt::Sign);
	if (Sign) C.Image(C.NextLayer(), Sign, 259.2f, 0.f, 499.2f, 81.9f, FLinearColor::White);
	else NeonText(C, TEXT("WAVE"), 485.f, 58.f, Hex(TEXT("50e1f5")), 38.f, 1, 1.f);
	// 숫자 (D-406, 시안대로): 크게 — 속은 검고 두꺼운 파란 테가 빛난다. 보스 웨이브 중이면 빨강
	{
		const bool bNow = G.Phase == EPDPhase::Wave && G.NextBossWave() == G.Wave;
		const FLinearColor Bc = bNow ? Red : Hex(TEXT("1eaaff"));
		if (!Sign) // 그림이 없을 때만 상자를 코드로
		{
			const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(WaveBoxX, WaveBoxY, WaveBoxW, WaveBoxH, 4.f);
			C.FillPoly(Ro, Hex(TEXT("06091a")));
			C.Glow(Ro, Bc, 2.2f, 1.f);
		}
		const FString Wv = G.Goal() > 0 ? FString::Printf(TEXT("%d/%d"), G.Wave, G.Goal()) : FString::FromInt(G.Wave);
		// 시안 (D-412): 두껍고 둥근 숫자(Roboto Black), 글자 높이 40 · '3/20' 폭 140. 속은 검고 두꺼운 파란 테 + 넓은 빛
		float Px = 46.f; // 4자리(9999)까지 같은 크기, 길면 상자에 맞게 줄임 (D-416, 예전 60). Roboto Black 은 시안 글꼴보다 좁아 가로로 1.12배 (D-415)
		FSlateFontInfo F = FCanvas::FontFace("Black", Px);
		while (Px > 24.f && FCanvas::Measure(Wv, F).X * 1.12f > 168.f) { Px -= 2.f; F = FCanvas::FontFace("Black", Px); }
		const float TX = 655.4f, TY = 45.4f; // 시안 상자 속 가운데 (x 875 ~ 1174, y 17 ~ 125 × 0.64)
		const FLinearColor Edge = Hex(TEXT("8cdcff"));
		for (int32 K = 0; K < 16; ++K) { const float A = K * TwoPi / 16.f; C.TextFace(Wv, TX + FMath::Cos(A) * 7.f, TY + FMath::Sin(A) * 6.f, WithAlpha(Bc, 0.035f), F, 1.12f); } // 은은한 빛 (D-419, 시안)
		for (int32 K = 0; K < 16; ++K) { const float A = K * TwoPi / 16.f; C.TextFace(Wv, TX + FMath::Cos(A) * 4.5f, TY + FMath::Sin(A) * 4.f, WithAlpha(Bc, 0.07f), F, 1.12f); }
		for (int32 K = 0; K < 20; ++K) { const float A = K * TwoPi / 20.f; C.TextFace(Wv, TX + FMath::Cos(A) * 2.4f, TY + FMath::Sin(A) * 2.4f, Bc, F, 1.12f); } // 얇고 밝은 테
		for (int32 K = 0; K < 12; ++K) { const float A = K * TwoPi / 12.f; C.TextFace(Wv, TX + FMath::Cos(A) * 1.2f, TY + FMath::Sin(A) * 1.2f, WithAlpha(Edge, 0.6f), F, 1.12f); }
		C.TextFace(Wv, TX, TY, Hex(TEXT("020409")), F, 1.12f);
	}
	// 목숨 | 레벨 (작은 판 하나)
	{
		const FLinearColor Edge = Hex(TEXT("be73ff"));
		C.Box(HudPanX, HudPanY, HudPanW, HudPanH, Hex(TEXT("0e0820"), 0.92f));
		C.Glow(FCanvas::RoundRectPoints(HudPanX, HudPanY, HudPanW, HudPanH, 8.f), WithAlpha(Edge, 0.85f), 1.5f, 0.5f);
		C.Lines({FVector2f(HudDivLv, HudPanY + 9.f), FVector2f(HudDivLv, HudPanY + HudPanH - 9.f)}, WithAlpha(Edge, 0.35f), 1.f);
		const float MidY = HudPanY + HudPanH / 2.f;
		const bool bLow = G.Lives <= 5;
		const FLinearColor Lc = bLow ? WithAlpha(Red, 0.6f + 0.4f * Pulse) : Pink;
		C.Shape(EPDSuit::Heart, HudPanX + 22.f, MidY, 10.f, Lc, 1.5f, 0.5f);
		NeonText(C, FString::FromInt(G.Lives), (HudPanX + 36.f + HudDivLv) / 2.f, MidY + 9.f, Lc, G.Lives >= 1000 ? 16.f : G.Lives >= 100 ? 20.f : 24.f, 1, 1.f);
		const float Fill = FMath::Clamp((float)G.Xp / FMath::Max(1, G.XpToNext()), 0.f, 1.f);
		const float B = G.PointsLeft() > 0 ? 0.7f + 0.3f * Pulse : 1.f;
		const FLinearColor Gc(Gold.R * B, Gold.G * B, Gold.B * B, 1.f);
		C.Text(TEXT("Lv"), HudDivLv + 10.f, MidY + 5.f, WithAlpha(Gc, 0.8f), 10.f, 0, true);
		NeonText(C, FString::FromInt(G.Level), (HudDivLv + 26.f + LvBarX - 4.f) / 2.f, MidY + 9.f, Gc, G.Level >= 1000 ? 14.f : G.Level >= 100 ? 18.f : 22.f, 1, 1.f);
		const float BH = 16.f, BY = MidY - BH / 2.f, Rr = RadPill(BH);
		C.RoundFill(LvBarX, BY, LvBarW, BH, Rr, WithAlpha(Gold, 0.1f));
		if (Fill > 0.f) C.RoundFill(LvBarX + 2.f, BY + 2.f, FMath::Max(BH - 4.f, (LvBarW - 4.f) * Fill), BH - 4.f, RadPill(BH - 4.f), WithAlpha(Gold, 0.8f * B));
		C.Glow(FCanvas::RoundRectPoints(LvBarX, BY, LvBarW, BH, Rr), WithAlpha(Gold, 0.7f), 1.1f, 0.4f);
		const FString Pc = FString::Printf(TEXT("%d%%"), FMath::FloorToInt(Fill * 100.f));
		for (const FVector2f D : {FVector2f(-1.f, 0.f), FVector2f(1.f, 0.f), FVector2f(0.f, -1.f), FVector2f(0.f, 1.f)}) C.TextCentered(Pc, LvBarX + LvBarW / 2.f + D.X, MidY + D.Y, Hex(TEXT("281c06")), 10.f, true);
		C.TextCentered(Pc, LvBarX + LvBarW / 2.f, MidY, FLinearColor::White, 10.f, true);
	}
	// '?' 버튼 = 메뉴 (Esc 와 같음)
	{
		const bool bHov = InRect(Mouse, SetOpenX, HudBtnY, HudBtnS, HudBtnS);
		const FVector2f O(SetOpenX + HudBtnS / 2.f, HudBtnY + HudBtnS / 2.f);
		C.FillPoly(FCanvas::CirclePoints(O, HudBtnS / 2.f, 28), Hex(TEXT("373a42"), bHov ? 1.f : 0.9f));
		C.Glow(FCanvas::CirclePoints(O, HudBtnS / 2.f, 28), bHov ? FLinearColor::White : Hex(TEXT("e1e1eb")), 1.8f, bHov ? 0.8f : 0.4f);
		C.TextCentered(TEXT("?"), O.X, O.Y, FLinearColor::White, 18.f, true);
	}
}

// 고정 패널 (D-119): 위쪽 띠, 왼쪽 족보, 오른쪽 위력 카드·버튼·부적, 아래 손패
void SPDBoardWidget::PaintPanels(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	PaintHud(C);

	// ---- 덱 · 손패 (Alice 시안 D-403): 왼쪽에 큰 금색 'N / M' + 덱 장수, 카드는 무늬 색 어두운 채움 + 굵은 네온 테 (번호 · 무늬 · 숫자 · 공격 방식)
	{ // 덱 (D-417, 시안에서 잰 색): 좁고 굵은 글꼴(Roboto BoldCondensed) 연노랑 'N / M' + 옅은 빛, 아래 '덱 N장'
		const FLinearColor DeckGold = Hex(TEXT("e6dc78"));
		const float DX = FX + 56.f, DY = HandY + 34.f;
		const FString Ds = FString::Printf(TEXT("%d / %d"), (int32)G.Hand.size(), G.HandMax());
		const FSlateFontInfo DF = FCanvas::FontFace("BoldCondensed", 40.f);
		for (int32 K = 0; K < 12; ++K) { const float A = K * TwoPi / 12.f; C.TextFace(Ds, DX + FMath::Cos(A) * 3.f, DY + FMath::Sin(A) * 3.f, WithAlpha(DeckGold, 0.08f), DF); }
		C.TextFace(Ds, DX, DY, DeckGold, DF);
		C.TextCentered(TrF(TEXT("덱 %d장"), (int32)G.Deck.size()), DX, HandY + 70.f, Hex(TEXT("ebe6c3")), 15.f, false);
	}
	if (G.Swaps > 0) // 자리바꿈 버튼 (D-149)
	{
		const FLinearColor SwC = Hex(TEXT("6affc8"));
		const bool bHov = InRect(Mouse, SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH);
		C.BtnFill(SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH, RadPill(SwapBtnH), SwC, bHov || bSwapMode);
		C.Glow(FCanvas::RoundRectPoints(SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH, RadPill(SwapBtnH)), SwC, bHov || bSwapMode ? 1.6f : 1.1f, bSwapMode ? 1.2f + 0.5f * Pulse : bHov ? 1.f : 0.5f);
		C.TextCentered(bSwapMode ? FString(Tr(TEXT("끄기 X"))) : TrF(TEXT("자리바꿈 ×%d"), G.Swaps), SwapBtnX + SwapBtnW / 2.f, SwapBtnY + SwapBtnH / 2.f, SwC, 10.f, true, SwapBtnW - 6.f);
	}
	for (int32 I = 0; I < (int32)G.Hand.size(); ++I) // 카드 (D-417, 시안에서 잰 색): 무늬마다 어두운 채움 + 굵고 밝은 테 + 좁고 굵은 숫자
	{
		const FPDCard& Card = G.Hand[I];
		const bool bSel = I == Sel;
		const float X = HandCardX(I, (int32)G.Hand.size()), Y = HandY - (bSel ? 10.f : 0.f);
		const TCHAR* Fl; const TCHAR* Ed; const TCHAR* Rk;
		switch (Card.Suit)
		{
		case EPDSuit::Heart: Fl = TEXT("3c111d"); Ed = TEXT("f04678"); Rk = TEXT("f05f7d"); break;
		case EPDSuit::Club: Fl = TEXT("1f2d36"); Ed = TEXT("5aaadc"); Rk = TEXT("5faad7"); break;
		case EPDSuit::Diamond: Fl = TEXT("353018"); Ed = TEXT("ebcd50"); Rk = TEXT("ebcd55"); break;
		case EPDSuit::Wild: Fl = TEXT("2a1630"); Ed = TEXT("ff9ad5"); Rk = TEXT("ff9ad5"); break;
		default: Fl = TEXT("3c3c3c"); Ed = TEXT("c8c8c8"); Rk = TEXT("dcdcdc"); break; // ♠
		}
		const FLinearColor Col = Hex(Ed), RkC = Hex(Rk);
		const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, CardW, CardH, 7.f);
		C.FillPoly(Ro, Hex(Fl));
		if (bSel) C.FillPoly(Ro, WithAlpha(Col, 0.14f));
		C.Glow(Ro, Col, bSel ? 3.2f : 2.6f, bSel ? 1.3f : 0.9f);
		C.Lines(Ro, WithAlpha(FLinearColor::White, bSel ? 0.45f : 0.25f), 0.8f);
		C.Lines(FCanvas::RoundRectPoints(X + 3.f, Y + 3.f, CardW - 6.f, CardH - 6.f, 5.f), WithAlpha(Col, 0.35f), 0.8f);
		C.Text(FString::FromInt(I + 1), X + 7.f, Y + 14.f, Hex(TEXT("9a9a9a")), 8.5f);
		C.Shape(Card.Suit, X + CardW / 2.f, Y + 27.f, 5.5f, Col, 1.4f, 0.5f);
		if (Card.IsWild()) C.Shape(Card.Suit, X + CardW / 2.f, Y + 57.f, 14.f, Col, 1.8f, 0.9f);
		else C.TextFace(RankText(Card.Rank), X + CardW / 2.f, Y + 57.f, RkC, FCanvas::FontFace("BoldCondensed", Card.Rank == 10 ? 30.f : 36.f));
		C.TextCentered(SuitName(Card.Suit), X + CardW / 2.f, Y + CardH - 11.f, WithAlpha(RkC, 0.8f), 9.f, false, CardW - 6.f);
	}

	// ---- 왼쪽 족보 안내
	PaintHandGuide(C);

	PaintTv(C); // 채널 TV (D-316, PDBoardTv.cpp): 예전 판 위력 TV 자리

	// ---- 오른쪽 아래 (D-414): 위아래 네온 줄 · 칵테일 칸 · 스킬 · 시작 버튼은 시안 그림 그대로 (한국어면 글자까지, 다른 언어는 글자를 지운 그림 + 번역 글자).
	// 그림 위에 잔 다섯(가진 만큼 켜짐) · 'N웨이브 후 획득' · 포인트 꼬리표 · 마우스 · 웨이브 중 표시를 코드로
	const bool bKoArt = GPDLang == (int32)EPDLang::Ko; (void)bKoArt;
	const bool RPArt = true; // D-456: 예전 시안 그림 조각(rpanel.png) 대신 코드 네온 (PDBoardAlicePanel.cpp)
	if (RPArt)
	{
		constexpr float AX = 1035.5f, AY = 404.5f, AW = 239.4f, AH = 309.1f; // 시안 (1618, 632) ~ (1992, 1115) × 0.64
		(void)AX; (void)AY; (void)AW; (void)AH;
		PaintAliceRPanel(C);
		const FLinearColor Head = Hex(TEXT("f0e2be")), GrT = Hex(TEXT("82f08c")), Tx = Hex(TEXT("82cdff"));
		// 글자는 언제나 코드로 (D-456)
		{
			C.TextCentered(Tr(TEXT("칵테일")), 1153.3f, 441.6f, Head, 14.f, true);
			C.TextCentered(Tr(TEXT("스 킬")), 1161.6f, 599.7f, Tx, 22.f, true, 150.f);
		}
		// 칵테일: 잔 다섯 + 다음 칵테일까지
		const int32 N = (int32)G.Charms.size();
		const FSlateBrush* GOn = Art(EUiArt::GlassOn);
		const FSlateBrush* GOff = Art(EUiArt::GlassOff);
		for (int32 I = 0; I < CharmMax; ++I)
		{
			if (I < N && CocktailArt(C, (int32)G.Charms[I], 1045.8f + I * 43.7f + 24.f, 528.f, 60.f, 1.f)) continue; // 가진 칵테일 = 그 칵테일 잔 (D-424)
			if (const FSlateBrush* B = I < N ? GOn : GOff) C.Image(C.NextLayer(), B, 1045.8f + I * 43.7f, 478.7f, 43.5f, 51.8f, FLinearColor::White);
		}
		const int32 W0 = G.Phase == EPDPhase::Wave ? G.Wave : G.Wave + 1;
		const int32 Next = (W0 + CharmEvery - 1) / CharmEvery * CharmEvery;
		if (N < CharmMax) C.Text(TrF(TEXT("%d웨이브 후 획득"), FMath::Max(0, Next - G.Wave)), 1255.7f, 463.5f, Hex(TEXT("c8c4be")), 8.5f, 2);
		for (int32 I = 0; I < N; ++I) // 잔 툴팁 (D-275)
		{
			const float GX = 1045.8f + I * 43.7f, GY = 478.7f;
			if (!InRect(Mouse, GX, GY, 43.5f, 51.8f)) continue;
			const float TW = 260.f, TH = 52.f, TX = FMath::Max(8.f, FMath::Min(GX - 40.f, DesignW - 8.f - TW) - 140.f), TY = GY - TH - 6.f;
			C.Box(TX, TY, TW, TH, Hex(TEXT("0a0910"), 0.97f));
			C.Glow(FCanvas::RoundRectPoints(TX, TY, TW, TH, RadSmall), Hex(TEXT("ffd26e")), 1.2f, 0.4f);
			C.Text(CharmName((int)G.Charms[I]), TX + 12.f, TY + 20.f, UiValue, 12.f, 0, true, TW - 24.f);
			C.Text(CharmDesc((int)G.Charms[I]), TX + 12.f, TY + 40.f, Fg, 10.f, 0, false, TW - 24.f);
		}
		// 스킬: 마우스 · 남은 포인트
		{
			const bool bHot = G.Phase != EPDPhase::Title && G.PointsLeft() > 0, bHov = InRect(Mouse, TreeBtnX, TreeBtnY, RBtnW, TreeBtnH);
			const float Fl = bHot ? FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f)) : 0.f;
			if (bHov || bHot) C.Box(RX + 3.f, TreeBtnY + 3.f, PW - 6.f, TreeBtnH - 6.f, FLinearColor(0.6f, 0.85f, 1.f, bHov ? 0.08f : 0.03f + 0.05f * Fl));
			if (bHot)
			{
				const FString Pt = TrF(TEXT("포인트 %d"), G.PointsLeft());
				const FLinearColor Gold2 = Hex(TEXT("ffd26e"));
				const float TW2 = FCanvas::Measure(TrS(Pt), FCanvas::Font(9.f, true)).X + 14.f, TX = RX + PW - TW2 - 10.f, TY = TreeBtnY - 9.f;
				C.Box(TX, TY, TW2, 18.f, Hex(TEXT("06060b"), 0.95f));
				C.Glow(FCanvas::RoundRectPoints(TX, TY, TW2, 18.f, RadPill(18.f)), Gold2, 1.2f, 0.6f + 0.3f * Fl);
				C.TextCentered(Pt, TX + TW2 / 2.f, TY + 9.f, Gold2, 9.f, true);
			}
		}
		// 시작: 준비 중이면 그림 그대로 (+ 마우스 · 자동 시작), 웨이브 중이면 글자를 지운 그림을 어둡게 + '웨이브 N 진행 중'
		{
			const bool bBuild = G.Phase == EPDPhase::Build, bHov = InRect(Mouse, RBtnX, StartBtnY, RBtnW, StartBtnH);
			if (bBuild)
			{
				C.TextCentered(Tr(TEXT("시 작")), 1164.8f, 672.f, GrT, 36.f, true, 150.f);
				if (bHov) C.Box(RX + 3.f, StartBtnY + 3.f, PW - 6.f, StartBtnH - 6.f, FLinearColor(0.7f, 1.f, 0.7f, 0.07f));
				if (G.AutoStartT >= 0.f)
				{
					C.TextCentered(TrF(TEXT("%d초 뒤 자동"), FMath::CeilToInt(G.AutoStartT)), 1164.8f, StartBtnY + StartBtnH - 11.f, WithAlpha(GrT, 0.85f), 9.f, false);
					if (G.AutoStartT <= 3.f) // 마지막 3초는 판 위에 크게
					{
						const float Fr = FMath::Frac(G.AutoStartT);
						C.TextCentered(FString::FromInt(FMath::CeilToInt(G.AutoStartT)), FX + Cols * CS / 2.f, FY + 60.f, WithAlpha(GrT, 0.35f + 0.5f * Fr), 28.f + 14.f * Fr, true);
					}
				}
			}
			else
			{
				C.Box(RX + 2.f, StartBtnY + 2.f, PW - 4.f, StartBtnH - 4.f, FLinearColor(0.f, 0.f, 0.f, 0.5f));
				C.TextCentered(G.Phase == EPDPhase::Wave ? TrF(TEXT("웨이브 %d 진행 중"), G.Wave) : FString(Tr(TEXT("웨이브 시작"))), 1164.8f, 672.f, WithAlpha(GrT, 0.75f), 15.f, true, 150.f);
			}
		}
	}
	// ---- 웨이브 시작 (Space): Alice 시안 (D-403) 맨 아래 큰 초록 버튼 (진한 초록 채움 + 밝은 초록 테 + 재생 삼각형 + '시 작')
	if (!RPArt)
	{
		const bool bHov = InRect(Mouse, RBtnX, StartBtnY, RBtnW, StartBtnH);
		const bool bBuild = G.Phase == EPDPhase::Build;
		const FLinearColor Gr = Hex(TEXT("5af06e")), GrT = Hex(TEXT("82f08c"));
		const float B = bBuild ? 1.f : 0.4f;
		const float X = RBtnX, Y = StartBtnY, W = RBtnW, H = StartBtnH;
		for (int32 K = 0; K < 8; ++K) C.Box(X + 2.f, Y + 2.f + K * (H - 4.f) / 8.f, W - 4.f, (H - 4.f) / 8.f + 0.5f, SrgbLerp(FColor((uint8)(20 * B), (uint8)(90 * B), (uint8)(30 * B)), FColor((uint8)(8 * B), (uint8)(60 * B), (uint8)(18 * B)), K / 7.f)); // 세로 그라데이션 (시안 sRGB 14 5a 1e → 08 3c 12)
		if (bBuild && bHov) C.Box(X + 2.f, Y + 2.f, W - 4.f, H - 4.f, WithAlpha(Gr, 0.1f));
		if (bBuild && G.AutoStartT >= 0.f) // 자동 시작까지 버튼 아래 선이 차오른다
		{
			const float Fr = 1.f - FMath::Clamp(G.AutoStartT / AutoStartDelayFor(G.Wave + 1), 0.f, 1.f);
			C.Lines({FVector2f(X + 12.f, Y + H - 9.f), FVector2f(X + 12.f + (W - 24.f) * Fr, Y + H - 9.f)}, Gr, 2.f);
		}
		const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, W, H, 8.f);
		C.Glow(Ro, WithAlpha(Gr, B), 2.6f, (bBuild ? 0.9f + 0.3f * Pulse : 0.4f));
		C.Lines(FCanvas::RoundRectPoints(X + 5.f, Y + 5.f, W - 10.f, H - 10.f, 5.f), WithAlpha(Gr, 0.5f * B), 1.f);
		const TArray<FVector2f> Tri = {FVector2f(X + 20.f, Y + 19.f), FVector2f(X + 52.f, Y + 36.f), FVector2f(X + 20.f, Y + 53.f)};
		C.FillPoly(Tri, WithAlpha(Hex(TEXT("82ff8c")), B));
		C.Glow({Tri[0], Tri[1], Tri[2], Tri[0]}, WithAlpha(Gr, B), 1.4f, 0.5f * B);
		if (bBuild)
		{
			C.TextCentered(Tr(TEXT("시 작")), X + 127.f, Y + H / 2.f - 2.f, bHov ? HotCol(GrT) : GrT, 34.f, true, W - 80.f);
			if (G.AutoStartT >= 0.f) C.TextCentered(TrF(TEXT("%d초 뒤 자동"), FMath::CeilToInt(G.AutoStartT)), X + 127.f, Y + H - 17.f, WithAlpha(GrT, 0.8f), 9.f, false);
			C.Text(TEXT("Space"), X + W - 10.f, Y + H - 7.f, WithAlpha(GrT, 0.5f), 8.f, 2);
			if (G.AutoStartT >= 0.f && G.AutoStartT <= 3.f) // 마지막 3초는 판 위에 크게
			{
				const float Fr = FMath::Frac(G.AutoStartT);
				C.TextCentered(FString::FromInt(FMath::CeilToInt(G.AutoStartT)), FX + Cols * CS / 2.f, FY + 60.f, WithAlpha(Gr, 0.35f + 0.5f * Fr), 28.f + 14.f * Fr, true);
			}
		}
		else
		{
			C.TextCentered(G.Phase == EPDPhase::Wave ? TrF(TEXT("웨이브 %d 진행 중"), G.Wave) : FString(Tr(TEXT("웨이브 시작"))), X + 127.f, Y + H / 2.f, WithAlpha(GrT, 0.6f), 15.f, true, W - 80.f);
		}
	}
	// ---- 배속 표시 (D-124): 2배속 이상이면 판 오른쪽 위에 크게 (빠를수록 주황 → 빨강)
	if (G.Speed > 1 && (G.Phase == EPDPhase::Wave || G.Phase == EPDPhase::Build))
	{
		const FLinearColor SC = G.Speed >= 10 ? Hex(TEXT("ff6a6a")) : G.Speed >= 5 ? Hex(TEXT("ffa86a")) : Cyan;
		const float RXb = FX + Cols * CS - 16.f, Yb = FY + 30.f, A = G.Phase == EPDPhase::Wave ? 0.75f + 0.25f * Pulse : 0.5f;
		const FString St = TrF(TEXT("×%d 배속"), G.Speed);
		C.Text(St, RXb, Yb, WithAlpha(SC, A), 26.f, 2, true);
		const float Ax = RXb - FCanvas::Measure(St, FCanvas::Font(26.f, true)).X - 30.f, Ay = Yb - 9.f; // 빨리 감기 꺾쇠 둘
		for (int32 K = 0; K < 2; ++K)
			C.Glow({FVector2f(Ax + K * 11.f, Ay - 9.f), FVector2f(Ax + K * 11.f + 10.f, Ay), FVector2f(Ax + K * 11.f, Ay + 9.f)}, WithAlpha(SC, A), 2.f, 0.8f);
		C.Text(TEXT("F: 배속 바꾸기"), RXb, Yb + 18.f, WithAlpha(SC, 0.5f), 10.f, 2);
	}
	// ---- 스킬 트리 버튼: Alice 시안 (D-403) 진한 파랑 채움 + 밝은 파랑 테 + 노드 아이콘 + '스 킬', 남은 포인트는 금색 꼬리표
	if (!RPArt)
	{
		const bool bPlay = G.Phase != EPDPhase::Title;
		const bool bHot = bPlay && G.PointsLeft() > 0, bHov = InRect(Mouse, TreeBtnX, TreeBtnY, RBtnW, TreeBtnH);
		const float X = TreeBtnX, Y = TreeBtnY, W = RBtnW, H = TreeBtnH;
		const FLinearColor Bl = Hex(TEXT("3c87ff")), Ic = Hex(TEXT("5ae1ff")), Tx = Hex(TEXT("82cdff"));
		for (int32 K = 0; K < 6; ++K) C.Box(X + 2.f, Y + 2.f + K * (H - 4.f) / 6.f, W - 4.f, (H - 4.f) / 6.f + 0.5f, SrgbLerp(FColor(10, 60, 160), FColor(2, 40, 120), K / 5.f)); // 시안 sRGB
		const float Fl = bHot ? FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f)) : 0.f; // 포인트가 있으면 숨쉰다 (D-189)
		if (bHov || bHot) C.Box(X + 2.f, Y + 2.f, W - 4.f, H - 4.f, WithAlpha(Ic, bHov ? 0.1f : 0.04f + 0.08f * Fl));
		const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, W, H, 6.f);
		C.Glow(Ro, Bl, 2.6f, bHov ? 1.2f : 0.9f + 0.3f * Fl);
		C.Lines(FCanvas::RoundRectPoints(X + 4.f, Y + 4.f, W - 8.f, H - 8.f, 4.f), WithAlpha(Bl, 0.45f), 1.f);
		const FVector2f N[4] = {FVector2f(X + 18.f, Y + 25.f), FVector2f(X + 34.f, Y + 15.f), FVector2f(X + 34.f, Y + 35.f), FVector2f(X + 50.f, Y + 25.f)};
		for (const FIntPoint E : {FIntPoint(0, 1), FIntPoint(0, 2), FIntPoint(1, 3), FIntPoint(2, 3)}) C.Glow({N[E.X], N[E.Y]}, Ic, 1.5f, 0.5f);
		for (const FVector2f& P : N) C.FillPoly(FCanvas::CirclePoints(P, 3.4f, 12), Ic);
		C.TextCentered(Tr(TEXT("스 킬")), X + 124.f, Y + H / 2.f, bHov ? HotCol(Tx) : Tx, 22.f, true, W - 90.f);
		C.Text(TEXT("K"), X + W - 9.f, Y + H - 8.f, WithAlpha(Tx, 0.5f), 8.f, 2);
		if (bHot)
		{
			const FString Pt = TrF(TEXT("포인트 %d"), G.PointsLeft());
			const FLinearColor Gold2 = Hex(TEXT("ffd26e"));
			const float TW2 = FCanvas::Measure(TrS(Pt), FCanvas::Font(9.f, true)).X + 14.f, TX = X + W - TW2 - 10.f, TY = Y - 9.f;
			C.Box(TX, TY, TW2, 18.f, Hex(TEXT("06060b"), 0.95f));
			C.Glow(FCanvas::RoundRectPoints(TX, TY, TW2, 18.f, RadPill(18.f)), Gold2, 1.2f, 0.6f + 0.3f * Fl);
			C.TextCentered(Pt, TX + TW2 / 2.f, TY + 9.f, Gold2, 9.f, true);
		}
	}
	// ---- 칵테일 선반 (Alice 시안 D-403): 위아래 네온 줄 사이 보랏빛 칸, '칵테일' 머리글 + 'N웨이브 후 획득', 잔 다섯 (시안 그림 조각: 가진 만큼 켜짐)
	if (!RPArt)
	{
		const int32 N = (int32)G.Charms.size();
		const FLinearColor PinkN = Hex(TEXT("ff46be")), PurN = Hex(TEXT("be73ff")), BlueN = Hex(TEXT("2d8cff")), Head = Hex(TEXT("ffe6be"));
		C.Glow({FVector2f(RX, CharmY - 15.f), FVector2f(RX + PW, CharmY - 15.f)}, PinkN, 1.8f, 0.7f);
		C.Glow({FVector2f(RX, CharmY - 7.f), FVector2f(RX + PW, CharmY - 7.f)}, PurN, 1.8f, 0.7f);
		for (int32 K = 0; K < 6; ++K) C.Box(RX, CharmY + K * CharmH / 6.f, PW, CharmH / 6.f + 0.5f, SrgbLerp(FColor(32, 12, 44), FColor(20, 9, 34), K / 5.f)); // 시안 sRGB
		const float HY = CharmY + 18.f, HW = FCanvas::Measure(TrS(Tr(TEXT("칵테일"))), FCanvas::Font(14.f, true)).X;
		C.TextCentered(Tr(TEXT("칵테일")), RX + PW / 2.f, HY, Head, 14.f, true);
		C.Lines({FVector2f(RX + 15.f, HY), FVector2f(RX + PW / 2.f - HW / 2.f - 10.f, HY)}, Hex(TEXT("9682a0"), 0.7f), 1.f);
		C.Lines({FVector2f(RX + PW / 2.f + HW / 2.f + 10.f, HY), FVector2f(RX + PW - 15.f, HY)}, Hex(TEXT("9682a0"), 0.7f), 1.f);
		const int32 W0 = G.Phase == EPDPhase::Wave ? G.Wave : G.Wave + 1;
		const int32 Next = (W0 + CharmEvery - 1) / CharmEvery * CharmEvery;
		if (N < CharmMax) C.Text(TrF(TEXT("%d웨이브 후 획득"), FMath::Max(0, Next - G.Wave)), RX + PW - 15.f, CharmY + 40.f, WithAlpha(Head, 0.85f), 8.5f, 2);
		const FSlateBrush* GOn = Art(EUiArt::GlassOn);
		const FSlateBrush* GOff = Art(EUiArt::GlassOff);
		for (int32 I = 0; I < CharmMax; ++I)
		{
			const float GX = 1045.8f + I * 43.7f, GY = 478.7f, GW = 43.5f, GH = 51.8f;
			if (const FSlateBrush* B = I < N ? GOn : GOff) C.Image(C.NextLayer(), B, GX, GY, GW, GH, FLinearColor::White);
			else // 그림이 없을 때: 마티니 잔 선
			{
				const FLinearColor Ec = I < N ? Hex(TEXT("ff5ac8")) : Hex(TEXT("6e2d6e"));
				const float CX = GX + GW / 2.f + 5.f;
				C.Glow({FVector2f(CX - 13.f, GY + 18.f), FVector2f(CX + 13.f, GY + 17.f), FVector2f(CX, GY + 36.f), FVector2f(CX - 13.f, GY + 18.f)}, Ec, 1.3f, I < N ? 0.6f : 0.f);
				C.Lines({FVector2f(CX, GY + 36.f), FVector2f(CX, GY + 49.f)}, Ec, 1.2f);
			}
		}
		C.Glow({FVector2f(RX + 5.f, 534.f), FVector2f(RX + PW - 5.f, 534.f)}, BlueN, 2.f, 0.8f);
		C.Glow({FVector2f(RX, 552.f), FVector2f(RX + PW, 552.f)}, PinkN, 1.8f, 0.7f);
		C.Glow({FVector2f(RX, 561.f), FVector2f(RX + PW, 561.f)}, BlueN, 1.8f, 0.7f);
		for (int32 I = 0; I < N; ++I) // 잔 툴팁 (D-275)
		{
			const float GX = 1045.8f + I * 43.7f, GY = 478.7f;
			if (!InRect(Mouse, GX, GY, 43.5f, 51.8f)) continue;
			const float TW = 260.f, TH = 52.f, TX = FMath::Max(8.f, FMath::Min(GX - 40.f, DesignW - 8.f - TW) - 140.f), TY = GY - TH - 6.f;
			C.Box(TX, TY, TW, TH, Hex(TEXT("0a0910"), 0.97f));
			C.Glow(FCanvas::RoundRectPoints(TX, TY, TW, TH, RadSmall), Hex(TEXT("ffd26e")), 1.2f, 0.4f);
			C.Text(CharmName((int)G.Charms[I]), TX + 12.f, TY + 20.f, UiValue, 12.f, 0, true, TW - 24.f);
			C.Text(CharmDesc((int)G.Charms[I]), TX + 12.f, TY + 40.f, Fg, 10.f, 0, false, TW - 24.f);
		}
	}
	// ---- 도움말: 오른쪽 위 '?' 에 마우스를 올리면 키 안내 (Alice 시안 D-403, 예전 오른쪽 아래 '? 도움말')
	{
		const bool bHov = InRect(Mouse, SetOpenX, HudBtnY, HudBtnS, HudBtnS);
		if (bHov)
		{
			static const TCHAR* Lines[] = {TEXT("카드 클릭(숫자키) → 칸 클릭: 설치 · Lv1 타워 위에 놓으면 교체 (원래 카드는 손으로)"), TEXT("오른쪽 클릭: 카드 버리기 · 고른 것 취소"),
				TEXT("타워 클릭: 메뉴 (합성 · 조커 · 먹이기 · 판매) · 왼쪽 족보 목록으로도 합성"), TEXT("Space 웨이브 시작 · F 속도 · P 일시정지 · M 소리 · F3 프레임 표시"), TEXT("K 스킬 트리 · U 영구 강화 · J 도감 · Esc 또는 ? 클릭: 메뉴")};
			const float W = 400.f, Hh = 24.f + UE_ARRAY_COUNT(Lines) * 18.f, X = DesignW - 16.f - W, Y = HudBtnY + HudBtnS + 8.f;
			C.Box(X, Y, W, Hh, Hex(TEXT("0a0b14"), 0.96f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, W, Hh, RadSmall), Cyan, 1.f, 0.4f);
			for (int32 I = 0; I < (int32)UE_ARRAY_COUNT(Lines); ++I) C.Text(Lines[I], X + 14.f, Y + 22.f + I * 18.f, Fg, 11.f, 0, false, W - 28.f);
		}
	}
}

// 덮는 창: 부적 고르기, 조커 도감 (J), 골드 강화 (U)
void SPDBoardWidget::PaintScreens(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 칵테일 고르기 (5웨이브마다, D-101 → D-286 바텐더 추천): 전구 두른 'BARTENDER'S CHOICE' 간판, 선반 위 잔 셋(칵테일마다 잔 모양 · 서비스는 칩 접시),
	// 아래 가격표 꼬리표에 이름 · 효과. 마우스를 올린 잔은 술이 차고 위에서 병이 기울어 따른다
	if (G.Phase == EPDPhase::Reward)
	{
		const FLinearColor Orange = Hex(TEXT("ffa046")), PinkR = Hex(TEXT("ff6aaa")), Lav = Hex(TEXT("a896e1")), White = FLinearColor::White, Mint = Hex(TEXT("9cffb0"));
		const float CXb = FX + Cols * CS / 2.f;
		C.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("06060b"), 0.92f));
		{ // 간판
			const float SX = CXb - 200.f, SY = FY + 24.f, SW = 400.f, SH = 64.f;
			C.Glow(FCanvas::RoundRectPoints(SX, SY, SW, SH, RadPanel), Orange, 2.f, 0.9f);
			C.Lines(FCanvas::RoundRectPoints(SX, SY, SW, SH, RadPanel), FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.6f);
			C.Glow(FCanvas::RoundRectPoints(SX + 5.f, SY + 5.f, SW - 10.f, SH - 10.f, RadSmall), WithAlpha(Orange, 0.7f), 1.f, 0.4f);
			const int32 Ph = FMath::FloorToInt(Clock * 8.f) % 3;
			const float X0 = SX + 14.f, Y0 = SY + 12.f, X1 = SX + SW - 14.f, Y1 = SY + SH - 12.f, Per = 2.f * ((X1 - X0) + (Y1 - Y0));
			for (int32 Bi = 0; Bi < FMath::FloorToInt(Per / 16.f); ++Bi)
			{
				float D = Bi * 16.f;
				FVector2f Q;
				if (D < X1 - X0) Q = FVector2f(X0 + D, Y0);
				else if ((D -= X1 - X0) < Y1 - Y0) Q = FVector2f(X1, Y0 + D);
				else if ((D -= Y1 - Y0) < X1 - X0) Q = FVector2f(X1 - D, Y1);
				else Q = FVector2f(X0, Y1 - (D - (X1 - X0)));
				Bulb(C, Q, 3.f, (Bi + Ph) % 3 != 0, Hex(TEXT("ffe196"))); // 동그란 전구 (D-294)
			}
			NeonText(C, TEXT("BARTENDER'S CHOICE"), CXb, SY + 42.f, Gold, 21.f, 1, 1.f);
			C.TextCentered(TrF(TEXT("웨이브 %d 칵테일 — 하나를 고른다"), G.Wave), CXb, SY + SH + 22.f, Fg, 13.f, true, Cols * CS - 40.f);
		}
		// 선반
		C.Glow({FVector2f(FX + 60.f, RewardShelfY), FVector2f(FX + Cols * CS - 60.f, RewardShelfY)}, Lav, 2.f, 0.6f);
		for (const float Bx : {FX + 90.f, FX + Cols * CS - 90.f}) C.Lines({FVector2f(Bx, RewardShelfY), FVector2f(Bx - 10.f, RewardShelfY + 18.f)}, WithAlpha(Lav, 0.6f), 1.4f);
		static const TCHAR* CockCol[5] = {TEXT("ff6aaa"), TEXT("e8eeff"), TEXT("ffd26e"), TEXT("8fe9ff"), TEXT("b48cff")};
		for (int32 I = 0; I < (int32)G.Rewards.size(); ++I)
		{
			const FPDReward& R = G.Rewards[I];
			const FVector2f P = RewardPos(I);
			const bool bPour = PourIdx == I, bAnyPour = PourIdx >= 0; // 따라 주는 중 (D-306)
			const bool bHov = bPour || (!bAnyPour && InRect(Mouse, P.X, P.Y, RewardW, RewardH));
			const float GX = P.X + RewardW / 2.f, BY = RewardShelfY - 2.f, B = bHov ? 1.f : bAnyPour ? 0.35f : 0.6f;
			const float Pt = bPour ? Clock - PourAt : 0.f;
			const float Fill = bPour ? FMath::Clamp((Pt - PourFlowT) / (PourEndT - 0.15f - PourFlowT), 0.f, 1.f) : bHov ? 1.f : 0.f; // 잔이 아래부터 찬다
			const FLinearColor Col = R.bCharm ? Hex(CockCol[R.Id % 5]) : Cyan;
			const FLinearColor CB(Col.R * B, Col.G * B, Col.B * B, 1.f);
			const int32 Kind = R.bCharm ? R.Id % 3 : 3; // 0 마티니 1 하이볼 2 온더록 3 칩 접시 (서비스)
			auto L = [&](std::initializer_list<FVector2f> Pts, float Th) { C.Glow(TArray<FVector2f>(Pts), CB, Th, 0.5f * B); };
			auto Shown = [&](int32 K, int32 N) { return K >= FMath::RoundToInt(N * (1.f - Fill)); }; // 줄 K (위 0 → 아래 N-1)
			if (R.bCharm && CocktailArt(C, R.Id, GX, BY, 118.f, (bHov ? 1.f : bAnyPour ? 0.4f : 0.7f) * (bPour ? 0.45f + 0.55f * Fill : 1.f))) {} // 칵테일마다 다른 잔 그림 (D-424)
			else if (Kind == 0)
			{
				L({FVector2f(GX - 26.f, BY - 70.f), FVector2f(GX + 26.f, BY - 70.f), FVector2f(GX, BY - 36.f), FVector2f(GX - 26.f, BY - 70.f)}, 1.8f);
				L({FVector2f(GX, BY - 36.f), FVector2f(GX, BY - 4.f)}, 1.6f);
				L({FVector2f(GX - 16.f, BY - 2.f), FVector2f(GX + 16.f, BY - 2.f)}, 1.6f);
				for (int32 K = 0; K < 5; ++K) if (Shown(K, 5)) { const float Hw = 22.f * (1.f - K * 5.f / 30.f); C.Lines({FVector2f(GX - Hw, BY - 64.f + K * 5.f), FVector2f(GX + Hw, BY - 64.f + K * 5.f)}, WithAlpha(Col, 0.5f), 1.f); }
				if (Fill >= 1.f) C.Lines(FCanvas::CirclePoints(FVector2f(GX + 9.f, BY - 66.f), 4.f, 12), Mint, 1.2f);
			}
			else if (Kind == 1)
			{
				L({FVector2f(GX - 18.f, BY - 78.f), FVector2f(GX - 16.f, BY - 2.f), FVector2f(GX + 16.f, BY - 2.f), FVector2f(GX + 18.f, BY - 78.f)}, 1.8f);
				C.Lines({FVector2f(GX + 6.f, BY - 92.f), FVector2f(GX - 4.f, BY - 30.f)}, FLinearColor(1.f, 1.f, 1.f, 0.6f * B), 1.2f); // 빨대
				for (int32 K = 0; K < 8; ++K) if (Shown(K, 8)) C.Lines({FVector2f(GX - 15.f, BY - 60.f + K * 7.f), FVector2f(GX + 15.f, BY - 60.f + K * 7.f)}, WithAlpha(Col, 0.45f), 1.f);
			}
			else if (Kind == 2)
			{
				L({FVector2f(GX - 24.f, BY - 46.f), FVector2f(GX - 22.f, BY - 2.f), FVector2f(GX + 22.f, BY - 2.f), FVector2f(GX + 24.f, BY - 46.f)}, 1.8f);
				for (int32 K = 0; K < 4; ++K) if (Shown(K, 4)) C.Lines({FVector2f(GX - 21.f, BY - 34.f + K * 8.f), FVector2f(GX + 21.f, BY - 34.f + K * 8.f)}, WithAlpha(Col, 0.45f), 1.f);
				C.Lines({FVector2f(GX - 12.f, BY - 30.f), FVector2f(GX - 2.f, BY - 38.f), FVector2f(GX + 8.f, BY - 30.f), FVector2f(GX - 2.f, BY - 22.f), FVector2f(GX - 12.f, BY - 30.f)}, FLinearColor(1.f, 1.f, 1.f, 0.6f * B), 1.f); // 얼음
			}
			else
			{
				L({FVector2f(GX - 34.f, BY - 2.f), FVector2f(GX + 34.f, BY - 2.f)}, 1.6f);
				for (int32 K = 0; K < 4; ++K) { const float Y = BY - 10.f - K * 9.f; L({FVector2f(GX - 18.f, Y), FVector2f(GX + 18.f, Y)}, 3.f); }
			}
			// 코블러 셰이커 (D-306, 시안 tools/neon/shaker_mock.py): 둥근 뚜껑 · 거름망 어깨 · 아래로 좁아지는 몸통 · 이음 띠. 원점 = 바닥 가운데, 위 = 음수
			const FLinearColor Steel = Hex(TEXT("c8d6eb"));
			auto Shaker = [&](FVector2f O, float AngDeg, bool bCap, float S, float Br)
			{
				const float Ca = FMath::Cos(FMath::DegreesToRadians(AngDeg)), Sa = FMath::Sin(FMath::DegreesToRadians(AngDeg));
				auto W = [&](float X, float Y) { X *= S; Y *= S; return O + FVector2f(X * Ca - Y * Sa, X * Sa + Y * Ca); };
				const FLinearColor Sc(Steel.R * Br, Steel.G * Br, Steel.B * Br, 1.f);
				TArray<FVector2f> Body = {W(-21.f, -62.f), W(-15.f, 0.f), W(15.f, 0.f), W(21.f, -62.f)};
				TArray<FVector2f> ShL, ShR;
				for (int32 K = 0; K <= 6; ++K) { const float A = FMath::DegreesToRadians(K * 15.f), X = 21.f - 9.f * (1.f - FMath::Cos(A)), Y = -62.f - 19.f * FMath::Sin(A); ShL.Add(W(-X, Y)); ShR.Add(W(X, Y)); }
				Algo::Reverse(ShR);
				TArray<FVector2f> Out = ShR; // 오른쪽 어깨 위 → 아래, 몸통, 왼쪽 어깨 아래 → 위 = 한 줄
				Out.Add(W(15.f, 0.f)); Out.Add(W(-15.f, 0.f)); Out.Append(ShL);
				Out.Add(W(-11.f, -87.f)); Out.Add(W(11.f, -87.f)); const FVector2f First = Out[0]; Out.Add(First); // 제 원소를 그대로 Add 하면 배열이 늘며 깨진다 (크래시)
				C.Glow(Out, Sc, 1.6f * S, 0.5f * Br);
				C.Lines({W(-21.f, -58.f), W(21.f, -58.f)}, WithAlpha(Sc, 0.8f), 1.f); // 이음 띠
				if (bCap)
				{
					TArray<FVector2f> Cap;
					for (int32 K = 0; K <= 10; ++K) { const float A = FMath::DegreesToRadians(180.f + K * 18.f); Cap.Add(W(10.f * FMath::Cos(A), -96.f + 10.f * FMath::Sin(A))); }
					Cap.Insert(W(-10.f, -87.f), 0); Cap.Add(W(10.f, -87.f));
					C.Glow(Cap, Sc, 1.4f * S, 0.45f * Br);
				}
				return W(0.f, -88.f); // 거름망 입구
			};
			if (bPour) // 내려옴 → 흔들기(쉐킷) → 기울이기 → 따르기(조로록) → 받기
			{
				const FVector2f Up(GX + 54.f, BY - 26.f), PourO(GX + 84.f, BY - 122.f); // 잔 옆에서 흔들고 (위 글자에 안 닿게), 잔 오른쪽 옆에서 기울여 입구만 가장자리 위로 (몸통이 잔을 덮지 않게)
				constexpr float Ss = 0.75f;
				if (Pt < PourShakeT)
				{
					const float U = 1.f - FMath::Square(1.f - Pt / PourShakeT);
					Shaker(FMath::Lerp(Up - FVector2f(0.f, 60.f), Up, U), 0.f, true, Ss, U);
				}
				else if (Pt < PourTiltT)
				{
					const float Ph = (Pt - PourShakeT) * 6.45f * TwoPi; // 초당 6번 남짓 (소리와 같은 박자)
					const FVector2f O = Up + FVector2f(FMath::Sin(Ph) * 4.f, -FMath::Abs(FMath::Sin(Ph)) * 10.f);
					Shaker(O, FMath::Sin(Ph) * 14.f, true, Ss, 1.f);
					const float Sd = FMath::Sin(Ph) > 0.f ? 1.f : -1.f; // 흔들림 선 (흔드는 쪽 반대편)
					for (int32 K = 0; K < 2; ++K) C.Lines({O + FVector2f(-Sd * (26.f + K * 4.f), -54.f + K * 16.f), O + FVector2f(-Sd * (31.f + K * 4.f), -43.f + K * 16.f)}, WithAlpha(Steel, 0.55f), 1.f);
				}
				else
				{
					const float U = FMath::Clamp((Pt - PourTiltT) / (PourFlowT - PourTiltT), 0.f, 1.f), E = U * U * (3.f - 2.f * U);
					const float Fade = Pt > PourEndT - 0.2f ? FMath::Max(0.f, (PourEndT - Pt) / 0.2f) : 1.f;
					const FVector2f Lip = Shaker(FMath::Lerp(Up, PourO, E), -112.f * E, false, Ss, Fade);
					// 뚜껑은 선반에 내려 둔다
					TArray<FVector2f> Cap;
					const FVector2f Cp(GX + 54.f, BY);
					for (int32 K = 0; K <= 10; ++K) { const float A = FMath::DegreesToRadians(180.f + K * 18.f); Cap.Add(Cp + FVector2f(7.5f * FMath::Cos(A), -7.f + 7.5f * FMath::Sin(A))); }
					Cap.Insert(Cp + FVector2f(-7.5f, 0.f), 0); Cap.Add(Cp + FVector2f(7.5f, 0.f));
					C.Glow(Cap, Steel, 1.4f, 0.4f);
					if (Pt >= PourFlowT && Pt < PourEndT - 0.15f) // 술줄기: 입구 → 잔 속 술 위
					{
						const float Top = Kind == 0 ? BY - 64.f : Kind == 1 ? BY - 60.f : BY - 34.f, Bot = Kind == 0 ? BY - 40.f : BY - 6.f;
						const float Surf = FMath::Lerp(Bot, Top, Fill), Wob = FMath::Sin(Clock * 21.f) * 1.2f;
						C.Glow({Lip, Lip + FVector2f(-5.f, 7.f), FVector2f(GX + 7.f + Wob, (Lip.Y + Surf) * 0.5f + 6.f), FVector2f(GX + 3.f + Wob, Surf)}, Col, 1.8f, 0.8f); // 입구에서 잔 가운데로 휘어 떨어진다
					}
				}
			}
			else if (bHov && !bAnyPour && Kind < 3) Shaker(FVector2f(GX + 54.f, BY), 0.f, true, 0.62f, 0.75f); // 마우스: 잔 옆 선반에 셰이커가 서 있다
			// 가격표 꼬리표: 이름 · 효과
			// 효과 글이 한 줄에 안 들어가면 가운데 가까운 띄어쓰기에서 두 줄로, 꼬리표도 그만큼 높게 (D-369)
			const FString DescS = TrS(R.bCharm ? CharmDesc(R.Id) : InstantDesc(R.Id));
			FString Desc1 = DescS, Desc2;
			if (FCanvas::Measure(DescS, FCanvas::Font(10.f, false)).X > RewardW - 34.f)
			{
				int32 Cut = -1;
				for (int32 Ci = 0; Ci < DescS.Len(); ++Ci) if (DescS[Ci] == TEXT(' ') && (Cut < 0 || FMath::Abs(Ci - DescS.Len() / 2) < FMath::Abs(Cut - DescS.Len() / 2))) Cut = Ci;
				if (Cut > 0) { Desc1 = DescS.Left(Cut); Desc2 = DescS.Mid(Cut + 1); }
			}
			const float TX = P.X, TY = RewardShelfY + 32.f, TW = RewardW, TH = Desc2.IsEmpty() ? 48.f : 60.f;
			const FLinearColor TagC = R.bCharm ? Gold : Cyan; // 마우스 올림은 같은 색으로 밝아짐 (D-382, 예전 분홍)
			const TArray<FVector2f> Tg = {FVector2f(TX, TY + TH / 2.f), FVector2f(TX + 14.f, TY), FVector2f(TX + TW, TY), FVector2f(TX + TW, TY + TH), FVector2f(TX + 14.f, TY + TH), FVector2f(TX, TY + TH / 2.f)};
			C.Box(TX + 14.f, TY, TW - 14.f, TH, Hex(TEXT("06060b"), 0.9f));
			C.Glow(Tg, TagC, bHov ? 1.8f : 1.4f, bHov ? 0.8f : 0.45f);
			C.Lines(FCanvas::CirclePoints(FVector2f(TX + 12.f, TY + TH / 2.f), 3.f, 10), WithAlpha(Fg, 0.6f), 1.f);
			C.TextCentered(R.bCharm ? CharmName(R.Id) : InstantName(R.Id), TX + TW / 2.f + 8.f, TY + 16.f, bHov ? HotCol(Fg) : Fg, 13.f, true, TW - 34.f);
			C.TextCentered(Desc1, TX + TW / 2.f + 8.f, TY + 34.f, Fg, 10.f, false, TW - 34.f);
			if (!Desc2.IsEmpty()) C.TextCentered(Desc2, TX + TW / 2.f + 8.f, TY + 47.f, Fg, 10.f, false, TW - 34.f);
			C.TextCentered(FString::Printf(TEXT("%s  ·  %d"), R.bCharm ? Tr(TEXT("칵테일")) : Tr(TEXT("서비스")), I + 1), GX, TY + 78.f, R.bCharm ? Gold : Cyan, 10.f, true); // 꼬리표 높이와 상관없이 같은 줄
		}
		C.TextCentered(TEXT("1 · 2 · 3 또는 클릭으로 고르기   S: 건너뛰기"), CXb, FY + Rows * CS - 30.f, Grey, 12.f, false, Cols * CS - 20.f);
		if ((int32)G.Charms.size() >= CharmMax) C.TextCentered(TEXT("칵테일 칸이 가득 찼다 (5/5)"), CXb, FY + Rows * CS - 12.f, Dim, 11.f, false);
	}

	if (bCodex) PaintCodexBook(C); // 도감 (J): 책자 메뉴판 (D-354, PDBoardCodex.cpp)
	if (bUpg && G.Phase != EPDPhase::Title) PaintUpgrades(C); // 영구 강화 (U): 판 안에서는 보기만 (D-228)
}

// 영구 강화 창 (D-230): 시작 화면에서는 칩으로 산다, 판 안에서는 지금 레벨만 본다

// 잠금 해제 창 (D-239): 칵테일 60 · 조커 10 을 하나씩 칩으로
void SPDBoardWidget::PaintUnlock(FCanvas& C) const
{
	// 잠금 해제 (D-261, 시안 C 걸린 메뉴판): 가운데 큰 판 = 지금 탭의 메뉴 (2단 × 15줄 '아이콘 이름 ···· 가격'), 양옆 작은 판 = 다른 탭.
	// 줄에 마우스를 올리면 툴팁, 클릭하면 칩으로 연다. 작은 글자는 빛번짐 없이 (D-257)
	// 인게임 도감(J)도 같은 화면 (D-265): 가격 대신 발견 · 위력 · 가진 칵테일, 클릭으로 사지 않음
	const FPDGame& G = Game;
	const bool bCdx = bCodex && G.Phase != EPDPhase::Title;
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Cyan = Hex(TEXT("6edcff")), Mint = Hex(TEXT("8affc4")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("dce4f0")), Wire = Hex(TEXT("5f6878"), 0.5f);
	const float NK = 0.75f; // 이 창의 네온 세기
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 1.f));
	// 두 겹 튜브 틀 (D-254/255) + 네 귀퉁이 볼트
	auto Frame = [&](float X, float Y, float W, float H, float R, const FLinearColor& Col, float B)
	{
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, R), FLinearColor(Col.R * B, Col.G * B, Col.B * B, 1.f), 2.2f, NK * B);
		C.Lines(FCanvas::RoundRectPoints(X, Y, W, H, R), FLinearColor(1.f, 1.f, 1.f, 0.3f * B), 0.6f);
		C.Glow(FCanvas::RoundRectPoints(X + 6.f, Y + 6.f, W - 12.f, H - 12.f, FMath::Max(2.f, R - 6.f)), FLinearColor(Col.R * B * 0.75f, Col.G * B * 0.75f, Col.B * B * 0.75f, 1.f), 1.f, 0.4f * NK * B);
		for (const FVector2f Bt : {FVector2f(X + 9.f, Y + 9.f), FVector2f(X + W - 9.f, Y + 9.f), FVector2f(X + 9.f, Y + H - 9.f), FVector2f(X + W - 9.f, Y + H - 9.f)})
			C.Lines(FCanvas::CirclePoints(Bt, 2.f, 10), Hex(TEXT("787d8c"), 0.7f), 1.f);
	};
	static const TCHAR* TabName[4] = {TEXT("칵테일"), TEXT("조커"), TEXT("상위 조커"), TEXT("보스")};
	static const TCHAR* TabEn[4] = {TEXT("COCKTAILS"), TEXT("JOKERS"), TEXT("UPPER JOKERS"), TEXT("BOSSES")};
	auto BossIcon = [&](int32 K, float X, float Y, float R, float A) { const EPDEnemy T = (EPDEnemy)((int32)EPDEnemy::ShieldBoss + K); C.CardSoldier(SoldierShape(T), FVector2f(X, Y + R * 1.3f), R, 0.f, WithAlpha(EnemyColor(T), A)); }; // 도감 보스 판 (D-337)
	// 천장 줄 (판을 매단 선)
	for (const float X : {300.f, 980.f}) C.Lines({FVector2f(X, 0.f), FVector2f(X, UnlBY)}, Wire, 1.f);
	for (int32 Sd = 0; Sd < 2; ++Sd)
		for (const float Dx : {55.f, 145.f}) C.Lines({FVector2f(UnlSideX(Sd) + Dx, 0.f), FVector2f(UnlSideX(Sd) + Dx, UnlSideY)}, Wire, 1.f);
	// 양옆 작은 판
	for (int32 Sd = 0; Sd < 2; ++Sd)
	{
		const int32 Tb = UnlSideTab(Sd);
		const float X = UnlSideX(Sd);
		const bool bH = InRect(Mouse, X, UnlSideY, UnlSideW, UnlSideH);
		C.Box(X, UnlSideY, UnlSideW, UnlSideH, Hex(TEXT("08080e"), 1.f));
		C.BtnFill(X, UnlSideY, UnlSideW, UnlSideH, RadPanel, bH ? Pink : Cyan, bH); // 누르는 판 = 버튼처럼 옅게 채움 (D-328 ②)
		Frame(X, UnlSideY, UnlSideW, UnlSideH, RadPanel, bH ? Pink : Cyan, bH ? 1.f : 0.75f);
		const float MY = UnlSideY + UnlSideH / 2.f; // 판 가운데에 모아 그린다 (판이 커졌다, D-328 ⑥)
		if (Tb == 0) C.Shape(EPDSuit::Heart, X + UnlSideW / 2.f, MY - 60.f, 22.f, Cyan, 1.6f, 0.5f);
		else if (Tb == 3) BossIcon(0, X + UnlSideW / 2.f, MY - 74.f, 16.f, 0.85f);
		else C.JokerTower(Tb == 1 ? 0 : UpperBase, X + UnlSideW / 2.f, MY - 60.f, 26.f, 0.85f, 0.f);
		C.TextCentered(TabName[Tb], X + UnlSideW / 2.f, MY + 6.f, Fg, 20.f, true);
		C.TextCentered(TabEn[Tb], X + UnlSideW / 2.f, MY + 32.f, WithAlpha(Cyan, 0.75f), 10.f, true);
		C.TextCentered(FString::Printf(TEXT("%d"), UnlCount(Tb)), X + UnlSideW / 2.f, MY + 56.f, WithAlpha(Cyan, 0.6f), 11.f, true);
		C.TextCentered(TEXT("눌러서 보기"), X + UnlSideW / 2.f, UnlSideY + UnlSideH - 30.f, bH ? Fg : Dim, 11.f, false);
	}
	// 가운데 큰 판
	C.Box(UnlBX, UnlBY, UnlBW, UnlBH, Hex(TEXT("08080e"), 1.f));
	Frame(UnlBX, UnlBY, UnlBW, UnlBH, RadPanel, Pink, 1.f);
	const float HCX = UnlBX + UnlBW / 2.f;
	C.TextCentered(TabName[UnlockTab], HCX, 104.f, Pink, 30.f, true);
	if (!bCdx) C.TextCentered(FString::Printf(TEXT("%s  ·  %d"), TabEn[UnlockTab], UnlCount(UnlockTab)), HCX, 136.f, WithAlpha(Pink, 0.8f), 11.f, true);
	else // 도감: 발견 · 열린 수
	{
		int32 NOpen = 0;
		if (UnlockTab == 0) for (int32 J = 0; J < (int32)EPDCharm::Count; ++J) NOpen += (G.CharmUnlocked >> J) & 1u;
		const FString Sub = UnlockTab == 0 ? TrF(TEXT("칵테일 %d / %d 열림"), NOpen, (int32)EPDCharm::Count) : UnlockTab == 1 ? TrF(TEXT("%d / 10 발견"), FMath::CountBits(G.Discovered)) : UnlockTab == 2 ? TrF(TEXT("상위 조커 %d / 30 발견"), FMath::CountBits(G.UpperFound)) : TrF(TEXT("보스 %d / %d 처치"), FMath::CountBits(G.BossKilled), BossKinds);
		C.TextCentered(FString::Printf(TEXT("%s  ·  %s"), TabEn[UnlockTab], *Sub), HCX, 136.f, WithAlpha(Pink, 0.8f), 11.f, true);
	}
	for (const float S : {-1.f, 1.f})
	{
		C.Glow({FVector2f(HCX + S * 90.f, 104.f), FVector2f(HCX + S * 190.f, 104.f)}, WithAlpha(Pink, 0.8f), 1.2f, 0.4f * NK);
		C.Shape(EPDSuit::Spade, HCX + S * 205.f, 104.f, 6.f, Pink, 1.2f, 0.4f * NK);
	}
	// 항목 상태
	struct FItem { FString Name, Desc, Note, Note2, Right; int32 Price = 0; bool bOpen = false, bMat = true, bKnown = true, bHave = false, bGoal = false; };
	auto Item = [&](int32 K)
	{
		FItem It;
		if (UnlockTab == 0)
		{
			It.Name = CharmName(K); It.Desc = CharmDesc(K); It.Note = Tr(TEXT("연 칵테일만 5웨이브 보상에 나온다")); It.Price = CharmPrice(K); It.bOpen = (G.CharmUnlocked >> K) & 1u;
			if (CharmGoal(K) >= 0) { It.bGoal = true; It.bMat = false; It.Note = TrF(TEXT("목표: %s"), GoalText(CharmGoal(K))); } // 칩 대신 목표로 열림 (D-321)
		}
		else if (UnlockTab == 1)
		{
			It.Name = JokerName(K); It.Desc = BaseJokerAbility(K); It.Price = JokerPrice(K); It.bOpen = G.JokerOpen(K);
			for (EPDHand Hd : JokerRecipe((EPDJoker)K)) { if (!It.Note.IsEmpty()) It.Note += TEXT(" · "); It.Note += HandName(Hd); }
		}
		else if (UnlockTab == 3) // 보스 (D-337): 만난 보스는 이름 · 능력, 못 만난 보스는 ???
		{
			const EPDEnemy T = (EPDEnemy)((int32)EPDEnemy::ShieldBoss + K);
			const bool bSeen = (G.BossSeen >> K) & 1u, bKill = (G.BossKilled >> K) & 1u;
			It.bOpen = It.bKnown = bSeen; It.bHave = bKill;
			It.Name = bSeen ? FString(BossName(T)) : FString(TEXT("???"));
			It.Desc = bSeen ? FString(BossSkillText(T)) : FString(Tr(TEXT("아직 만나지 못한 보스")));
			It.Note = IsFinalBoss(T) ? FString(Tr(TEXT("모드 최종 보스"))) : FString(Tr(TEXT("보스")));
			It.Right = bKill ? FString(Tr(TEXT("처치"))) : bSeen ? FString(Tr(TEXT("만남"))) : FString(TEXT("?"));
		}
		else
		{
			const FPDUpperDef& D = UpperDef(K);
			It.Name = UpperName(K); It.Desc = JokerAbility(UpperBase + K); It.Price = UpperPrice(K); It.bOpen = G.UpperOpen(K);
			It.bMat = G.JokerOpen((int32)D.A) && G.JokerOpen((int32)D.B);
			It.Note = FString::Printf(TEXT("%s + %s"), JokerName((int32)D.A), JokerName((int32)D.B));
		}
		if (bCdx) // 도감 (D-265): 모르는 조커는 ???, 칵테일은 열림 · 잠김 · 가진 칵테일
		{
			if (UnlockTab == 0)
			{
				It.bHave = G.Has((EPDCharm)K);
				It.Right = It.bHave ? FString(Tr(TEXT("가진 칵테일"))) : It.bOpen ? FString(Tr(TEXT("열림"))) : FString(Tr(TEXT("잠김")));
				if (!It.bOpen) It.Note = TrF(TEXT("잠금 해제에서 %d칩"), It.Price); // 잠긴 것도 효과는 보여 준다 (D-283)
				It.bKnown = It.bOpen;
			}
			else if (UnlockTab == 1)
			{
				It.bKnown = (G.Discovered >> K) & 1u;
				const float Tier = JokerTier((EPDJoker)K);
				const FString TierS = FMath::IsNearlyEqual(Tier, FMath::RoundToFloat(Tier)) ? FString::Printf(TEXT("×%.0f"), Tier) : FString::Printf(TEXT("×%.1f"), Tier);
				It.Right = It.bKnown ? TierS : FString(TEXT("?"));
				if (!It.bKnown) { It.Name = TEXT("???"); It.Desc = Tr(TEXT("만들어 보면 열린다")); }
				It.Note = JokerRecipeText(K) + TEXT("   ") + JokerRecipe2Text(K); // 레시피는 몰라도 보인다 (예전 도감과 같게)
				It.Note2 = TrF(TEXT("%s 대장 · 위력 %s"), HandName(CaptainHand((EPDJoker)K)), *TierS) + (It.bKnown ? TEXT("  ·  ") + JokerCommand(K) : FString());
			}
			else if (UnlockTab == 2)
			{
				const FPDUpperDef& D = UpperDef(K);
				It.bKnown = (G.UpperFound >> K) & 1u;
				It.Right = It.bKnown ? FString(Tr(TEXT("발견"))) : FString(TEXT("?"));
				if (!It.bKnown) { It.Name = TEXT("???"); It.Desc = Tr(TEXT("만들면 열림")); It.Note = TrF(TEXT("%s + ?"), JokerName((int32)D.A)); }
			}
		}
		return It;
	};
	static const FLinearColor IconCol[4] = {Hex(TEXT("7fb8ff")), Hex(TEXT("ff6a8a")), Hex(TEXT("ffd96a")), Hex(TEXT("8affc4"))};
	const int32 Hover = UnlRowAt(Mouse);
	for (int32 Slot = 0; Slot < UnlPerPage; ++Slot)
	{
		const int32 K = UnlockPage * UnlPerPage + Slot;
		if (K >= UnlCount(UnlockTab)) break;
		const FItem It = Item(K);
		const bool bCan = !It.bOpen && It.bMat && MetaChips >= It.Price, bH = Hover == K;
		const FVector2f R = UnlRowPos(Slot);
		const float Base = R.Y + 18.f;
		// 아이콘 칸
		const float IX = R.X + 10.f, IY = R.Y + 12.f;
		if (UnlockTab == 0)
		{
			const FLinearColor Ic = IconCol[K % 4];
			C.Glow(FCanvas::RoundRectPoints(IX - 10.f, IY - 10.f, 20.f, 20.f, 6.f), WithAlpha(Ic, It.bOpen ? 0.7f : 0.35f), 1.f, 0.f);
			C.Shape((EPDSuit)(K % 4), IX, IY, 5.f, WithAlpha(Ic, It.bOpen ? 0.85f : 0.45f), 1.f, 0.f);
		}
		else if (UnlockTab == 3) BossIcon(K, IX, IY - 9.f, 7.f, It.bKnown ? 0.9f : 0.25f);
		else C.JokerTower(UnlockTab == 1 ? K : UpperBase + K, IX, IY, 9.f, (bCdx ? It.bKnown : It.bOpen) ? 0.9f : 0.4f, 0.f);
		if (bH) C.Box(R.X + 26.f, R.Y + 1.f, UnlRowW - 26.f, UnlRowH - 2.f, WithAlpha(Pink, 0.08f));
		const FLinearColor NameBase = bCdx ? (It.bHave ? Gold : It.bKnown ? Cyan : Dim) : It.bOpen ? Cyan : bCan ? Gold : Dim;
		const FLinearColor NameCol = bH ? HotCol(NameBase) : NameBase; // D-382
		C.Text(It.Name, R.X + 30.f, Base, NameCol, 13.f, 0, true, 200.f);
		const float NameW = FMath::Min(200.f, FCanvas::Measure(TrS(It.Name), FCanvas::Font(13.f, true)).X);
		const FString Pt = bCdx ? It.Right : It.bOpen ? FString(Tr(TEXT("열림"))) : It.bGoal ? FString(Tr(TEXT("목표"))) : !It.bMat ? FString(Tr(TEXT("재료 먼저"))) : FString::Printf(TEXT("%d"), It.Price);
		const FLinearColor PtCol = bCdx ? (It.bHave ? Gold : It.bKnown ? Mint : Dim) : It.bOpen ? Mint : bCan ? Gold : Dim;
		C.Text(Pt, R.X + UnlRowW, Base, PtCol, 13.f, 2, true);
		const float PtW = FCanvas::Measure(TrS(Pt), FCanvas::Font(13.f, true)).X;
		for (float Dx = R.X + 36.f + NameW; Dx < R.X + UnlRowW - PtW - 8.f; Dx += 6.f) C.Box(Dx, Base - 4.f, 1.2f, 1.2f, WithAlpha(Dim, 0.6f)); // 점선 리더
		if (bH) C.Glow({FVector2f(R.X + 30.f, Base + 6.f), FVector2f(R.X + UnlRowW, Base + 6.f)}, Pink, 1.2f, 0.5f * NK);
	}
	// 아래: 안내 + 쪽 넘기기
	C.Glow({FVector2f(UnlBX + 30.f, 640.f), FVector2f(UnlBX + UnlBW - 30.f, 640.f)}, WithAlpha(Pink, 0.5f), 1.f, 0.2f * NK);
	C.TextCentered(bCdx ? (UnlockTab == 3 ? TEXT("30 · 60 · 100 모드는 보스 차례가 늘 같다. 무한 모드는 100웨이브 뒤 판마다 다르다") : UnlockTab == 2 ? TEXT("서로 다른 조커 둘을 맞닿게 놓으면 상위 조커 (★ 대표 11종은 둘 다 조커 레벨 10)") : UnlockTab == 1 ? TEXT("족보 3개(또는 2개)를 골라 레시피와 맞으면 조커 (판 어디에 있어도)") : TEXT("줄에 마우스를 올리면 설명"))
		: TEXT("줄에 마우스를 올리면 설명 · 클릭하면 칩으로 연다"), HCX, 668.f, Dim, 12.f, false, UnlBW - 120.f);
	if (UnlPages() > 1)
		for (int32 Dir = -1; Dir <= 1; Dir += 2)
		{
			const float X = UnlPgX(Dir);
			const bool bOk = Dir < 0 ? UnlockPage > 0 : UnlockPage < UnlPages() - 1, bH = bOk && InRect(Mouse, X, UnlPgY, UnlPgW, UnlPgH);
			if (bOk) C.BtnFill(X, UnlPgY, UnlPgW, UnlPgH, RadPill(UnlPgH), bH ? Pink : Cyan, bH);
			C.Glow(FCanvas::RoundRectPoints(X, UnlPgY, UnlPgW, UnlPgH, UnlPgH / 2.f), WithAlpha(bH ? Pink : Cyan, bOk ? 0.85f : 0.25f), bH ? 1.8f : 1.3f, bOk ? 0.5f * NK : 0.f);
			C.TextCentered(Dir < 0 ? TEXT("‹") : TEXT("›"), X + UnlPgW / 2.f, UnlPgY + UnlPgH / 2.f, bOk ? Fg : Dim, 18.f, true);
		}
	C.TextCentered(UnlPages() > 1 ? TrF(TEXT("%d / %d 쪽  ·  휠: 넘기기"), UnlockPage + 1, UnlPages()) : FString(), HCX, 708.f, Dim, 11.f, false);
	// 칩 알약 + 닫기 (도감은 칩 대신 닫기만)
	if (!bCdx)
	{
		const FString S = TrF(TEXT("칩 %s"), *CommaNum(MetaChips));
		const float PW2 = 46.f + FCanvas::Measure(S, FCanvas::Font(13.f, true)).X, PX = CdxCloseX - 14.f - PW2, PY = CdxCloseY;
		C.Glow(FCanvas::RoundRectPoints(PX, PY, PW2, CdxCloseH, CdxCloseH / 2.f), Gold, 1.4f, 0.6f * NK);
		C.Shape(EPDSuit::Diamond, PX + 17.f, PY + CdxCloseH / 2.f, 6.f, Gold, 1.3f, 0.4f);
		C.Text(S, PX + 30.f, PY + CdxCloseH / 2.f + 5.f, Gold, 13.f, 0, true);
	}
	PaintCloseButton(C, bCdx ? JokerPink : Cyan);
	// 툴팁 (D-258): 속이 찬 어두운 판 + 금색 두 겹 틀
	if (Hover >= 0)
	{
		const FItem It = Item(Hover);
		const bool bCan = !It.bOpen && It.bMat && MetaChips >= It.Price;
		const float TW = bCdx && UnlockTab == 1 ? 380.f : 320.f, TH = bCdx && UnlockTab == 1 ? 182.f : 160.f;
		float TX = Mouse.X + 16.f, TY = Mouse.Y - TH - 8.f;
		if (TX + TW > DesignW - 8.f) TX = Mouse.X - TW - 16.f;
		if (TY < 8.f) TY = Mouse.Y + 20.f;
		C.Box(TX, TY, TW, TH, Hex(TEXT("0a0910"), 1.f));
		Frame(TX, TY, TW, TH, RadSmall, Gold, 0.9f);
		if (UnlockTab == 0) C.Shape((EPDSuit)(Hover % 4), TX + 35.f, TY + 35.f, 9.f, IconCol[Hover % 4], 1.4f, 0.3f);
		else if (UnlockTab == 3) BossIcon(Hover, TX + 35.f, TY + 20.f, 13.f, It.bKnown ? 1.f : 0.3f);
		else C.JokerTower(UnlockTab == 1 ? Hover : UpperBase + Hover, TX + 35.f, TY + 35.f, 14.f, 1.f, 0.f);
		C.Text(It.Name, TX + 64.f, TY + 42.f, FLinearColor::White, 18.f, 0, true, TW - 80.f);
		Wrap2(C, It.Desc, TX + 20.f, TY + 76.f, Fg, 13.f, TW - 40.f);
		if (bCdx && UnlockTab == 1) Wrap2(C, It.Note, TX + 20.f, TY + 110.f, Fg, 10.f, TW - 40.f);
		else C.Text(It.Note, TX + 20.f, TY + 116.f, Dim, 11.f, 0, false, TW - 40.f);
		if (bCdx) // 도감 툴팁 아래: 대장 · 위력 · 지휘 (조커), 가진 칵테일 (칵테일)
		{
			if (!It.Note2.IsEmpty()) Wrap2(C, It.Note2, TX + 20.f, TY + 140.f, WithAlpha(Gold, 0.9f), 10.f, TW - 40.f);
			else C.Text(It.Right, TX + 20.f, TY + 148.f, It.bHave ? Gold : It.bKnown ? Mint : Dim, 14.f, 0, true);
		}
		else
		{
			C.Lines({FVector2f(TX + 16.f, TY + 126.f), FVector2f(TX + TW - 16.f, TY + 126.f)}, WithAlpha(Gold, 0.35f), 1.f);
			C.Text(It.bOpen ? FString(Tr(TEXT("열림"))) : !It.bMat ? FString(Tr(TEXT("재료 조커 먼저"))) : TrF(TEXT("%d칩"), It.Price), TX + 20.f, TY + 148.f, It.bOpen ? Mint : bCan ? Gold : Dim, 15.f, 0, true);
			if (!It.bOpen) C.Text(bCan ? FString(Tr(TEXT("클릭해서 열기"))) : !It.bMat ? FString() : FString(Tr(TEXT("칩이 모자란다"))), TX + TW - 20.f, TY + 148.f, bCan ? Pink : Dim, 13.f, 2, true);
		}
	}
	C.Text(bCdx ? TEXT("Tab: 다음 판 · ←→: 쪽 · J / Esc · 오른쪽 클릭: 닫기") : TEXT("Tab: 다음 판 · ←→: 쪽 · Esc · 오른쪽 클릭: 닫기"), DesignW - 16.f, DesignH - 10.f, Dim, 10.f, 2);
}

void SPDBoardWidget::PaintUpgrades(FCanvas& C) const
{
	// 영구 강화 (D-244, 시안 A 카드 테이블): 4 × 2 카드. 아이콘 + 이름, 레벨 고리, 효과, 버튼 +1 · +10 · +100 · 최대
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Felt = Hex(TEXT("0b1512"));
	const bool bShop = G.Phase == EPDPhase::Title;
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.96f));
	C.Text(TEXT("영구 강화"), 60.f, 54.f, Gold, 26.f, 0, true);
	C.Text(bShop ? FString(Tr(TEXT("판이 끝나면 칩을 받는다  ·  웨이브 + 2 × 오른 레벨 + 클리어 덤"))) : FString(Tr(TEXT("판이 끝나도 남는 강화. 시작 화면에서 칩으로 올린다"))), 60.f, 84.f, Grey, 12.f, 0, false, 700.f);
	C.Text(UpgCap() > UpgradeMax ? FString(Tr(TEXT("100 웨이브 클리어: 레벨 9999까지"))) : FString(Tr(TEXT("레벨 20까지 · 100 웨이브를 클리어하면 9999까지 열린다"))), DesignW - 60.f, 100.f, UpgCap() > UpgradeMax ? Gold : Dim, 10.f, 2, false, 420.f); // D-238
	{ // 칩 알약
		const FString S = TrF(TEXT("칩 %s"), *CommaNum(MetaChips));
		const float PW2 = 46.f + FCanvas::Measure(S, FCanvas::Font(13.f, true)).X, PX = CdxCloseX - 14.f - PW2, PY = CdxCloseY;
		C.Box(PX, PY, PW2, CdxCloseH, WithAlpha(Gold, 0.08f));
		C.Glow(FCanvas::RoundRectPoints(PX, PY, PW2, CdxCloseH, CdxCloseH / 2.f), Gold, 1.4f, 0.7f);
		C.Shape(EPDSuit::Diamond, PX + 17.f, PY + CdxCloseH / 2.f, 6.f, Gold, 1.3f, 0.5f);
		C.Text(S, PX + 30.f, PY + CdxCloseH / 2.f + 5.f, Gold, 13.f, 0, true);
	}
	const int32 Cap = UpgCap();
	for (int32 U = 0; U < (int32)EPDUpgrade::Count; ++U)
	{
		const FVector2f P = UpgCardPos(U);
		const int32 Lv = G.Upg[U];
		const FLinearColor Col = UpgradeColor(U);
		C.Box(P.X, P.Y, UpgCardW, UpgCardH, WithAlpha(Felt, 0.92f));
		C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, UpgCardW, UpgCardH, RadPanel), Col, 1.4f, 0.6f);
		// 아이콘: 무늬 강화는 무늬, 나머지는 칼 · 번개 · 별 · 터짐
		const float IX = P.X + 26.f, IY = P.Y + 30.f, IR = 11.f;
		if (U >= (int32)EPDUpgrade::Spade) C.Shape((EPDSuit)(U - (int32)EPDUpgrade::Spade), IX, IY, IR, Col, 1.5f, 0.6f);
		else if (U == (int32)EPDUpgrade::Attack)
		{
			C.Glow({FVector2f(IX - IR * 0.7f, IY + IR * 0.7f), FVector2f(IX + IR * 0.7f, IY - IR * 0.7f)}, Col, 1.5f, 0.6f);
			C.Glow({FVector2f(IX - IR * 0.3f, IY - IR * 0.05f), FVector2f(IX + IR * 0.05f, IY + IR * 0.3f)}, Col, 1.5f, 0.6f);
		}
		else if (U == (int32)EPDUpgrade::Speed) C.Glow({FVector2f(IX + IR * 0.2f, IY - IR), FVector2f(IX - IR * 0.45f, IY + IR * 0.1f), FVector2f(IX + IR * 0.05f, IY + IR * 0.1f), FVector2f(IX - IR * 0.2f, IY + IR)}, Col, 1.5f, 0.6f);
		else if (U == (int32)EPDUpgrade::Crit)
		{
			TArray<FVector2f> St;
			for (int32 K = 0; K <= 10; ++K) { const float A = -0.5f * UE_PI + K * UE_PI / 5.f, R2 = K % 2 == 0 ? IR : IR * 0.45f; St.Add(FVector2f(IX + FMath::Cos(A) * R2, IY + FMath::Sin(A) * R2)); }
			C.Glow(St, Col, 1.5f, 0.6f);
		}
		else for (int32 K = 0; K < 8; ++K) { const float A = K * UE_PI / 4.f; C.Glow({FVector2f(IX + FMath::Cos(A) * IR * 0.35f, IY + FMath::Sin(A) * IR * 0.35f), FVector2f(IX + FMath::Cos(A) * IR, IY + FMath::Sin(A) * IR)}, Col, 1.5f, 0.6f); }
		C.Text(UpgradeName(U), P.X + 48.f, P.Y + 36.f, Col, 17.f, 0, true, UpgCardW - 60.f);
		// 레벨 고리: 20까지는 20칸 기준, 그 뒤(상한 9999)는 100레벨마다 한 바퀴
		const FVector2f RC(P.X + UpgCardW / 2.f, P.Y + 116.f);
		const float Frac = Lv >= Cap ? 1.f : Lv < UpgradeMax ? Lv / (float)UpgradeMax : ((Lv - UpgradeMax) % 100) / 100.f;
		C.Glow(FCanvas::CirclePoints(RC, 44.f, 48), Hex(TEXT("1c2a26")), 4.f, 0.2f);
		if (Frac > 0.f)
		{
			TArray<FVector2f> Arc;
			const int32 NA = FMath::Max(2, FMath::CeilToInt(40.f * Frac));
			for (int32 K = 0; K <= NA; ++K) { const float A = -0.5f * UE_PI + TwoPi * Frac * K / NA; Arc.Add(RC + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 44.f); }
			C.Glow(Arc, Col, 4.f, 0.9f);
		}
		C.TextCentered(FString::Printf(TEXT("Lv %d"), Lv), RC.X, RC.Y - 6.f, Lv >= Cap ? Gold : UiValue, 20.f, true);
		C.TextCentered(FString::Printf(TEXT("/ %d"), Cap), RC.X, RC.Y + 16.f, Dim, 11.f, false);
		Wrap2(C, UpgradeEffect(U, Lv), P.X + 16.f, P.Y + 178.f, Fg, 11.f, UpgCardW - 32.f);
		if (!bShop) continue;
		for (int32 K = 0; K < 4; ++K)
		{
			const FVector2f B = UpgButtonPos(U, K);
			int32 N = 0, Cost = 0;
			for (int32 L = Lv; L < Cap && N < UpgSteps[K] && Cost + UpgradeCost(L) <= (K < 3 ? INT32_MAX : MetaChips); ++L) { Cost += UpgradeCost(L); ++N; }
			const bool bCan = N > 0 && Cost <= MetaChips;
			const bool bHov = InRect(Mouse, B.X, B.Y, UpgBtnW, UpgBtnH);
			if (bCan) C.BtnFill(B.X, B.Y, UpgBtnW, UpgBtnH, RadPill(UpgBtnH), Col, bHov);
			C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, UpgBtnW, UpgBtnH, RadPill(UpgBtnH)), WithAlpha(bCan ? Col : Dim, bCan ? 1.f : 0.6f), bHov ? 1.7f : 1.3f, bCan ? (bHov ? 1.2f : 0.6f) : 0.15f);
			C.TextCentered(K < 3 ? FString::Printf(TEXT("+%d"), UpgSteps[K]) : FString(Tr(TEXT("최대"))), B.X + UpgBtnW / 2.f, B.Y + 13.f, bCan ? Col : Dim, 12.f, true);
			C.TextCentered(N > 0 ? CommaNum(Cost) : FString(TEXT("-")), B.X + UpgBtnW / 2.f, B.Y + 28.f, bCan ? Gold : Dim, 9.f, false, UpgBtnW - 4.f);
		}
	}
	C.TextCentered(bShop ? FString(Tr(TEXT("버튼 클릭: 칩으로 강화   U 또는 Esc · 오른쪽 클릭: 닫기"))) : FString(Tr(TEXT("U 또는 Esc · 오른쪽 클릭: 닫기   (여는 동안 게임은 멈춘다)"))),
		DesignW / 2.f, DesignH - 16.f, Dim, 11.f, false);
	PaintCloseButton(C, Gold);
}

// 스킬 트리 창 (K, D-119): 위쪽 띠, 노드마다 하는 일 아이콘, 범례, 설명
void SPDBoardWidget::PaintTree(FCanvas& C) const
{
	if (!bTree) return;
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const FLinearColor Purple = Hex(TEXT("b48cff")), Cyan = Hex(TEXT("7fd6ff"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 넓힌 트리(D-378)가 설계 화면 밖(창 가장자리 검은 띠)으로 나가지 않게 자른다 (D-379). 띠 묶음은 자르기 앞뒤로 비운다
	C.FlushQuads();
	C.PushClip(0.f, 0.f, DesignW, DesignH);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.97f));
	{ // 벽돌 벽 (D-293)
		const FLinearColor Brick = Hex(TEXT("4a3c48"), 0.6f);
		for (int32 R = 0; R < 21; ++R)
		{
			const float Y = 20.f + R * 34.f;
			C.Lines({FVector2f(0.f, Y), FVector2f(DesignW, Y)}, Brick, 1.f);
			for (int32 K = 0; K < 21; ++K) { const float X = K * 64.f + (R % 2 ? 32.f : 0.f); C.Lines({FVector2f(X, Y), FVector2f(X, Y + 34.f)}, Brick, 1.f); }
		}
	}
	// 걸린 'SKILL TREE' 간판 + 정보 알약 (간판 왼쪽 셋, 오른쪽 하나: 겹치지 않게, D-291).
	// 확대해도 그대로인 자리라 노드 위에 나중에 그린다 (D-376)
	auto Header = [&]()
	{
	for (const float Wx : {470.f, 810.f}) C.Lines({FVector2f(Wx, 0.f), FVector2f(Wx, 8.f)}, Hex(TEXT("5f6878"), 0.5f), 1.f);
	C.Box(440.f, 8.f, 400.f, 54.f, Hex(TEXT("06060b"), 0.9f));
	C.Glow(FCanvas::RoundRectPoints(440.f, 8.f, 400.f, 54.f, RadPanel), Purple, 2.2f, 1.f);
	C.Glow(FCanvas::RoundRectPoints(445.f, 13.f, 390.f, 44.f, RadSmall), WithAlpha(Purple, 0.6f), 1.f, 0.4f);
	NeonText(C, TEXT("SKILL TREE"), 640.f, 45.f, Purple, 26.f, 1, 1.f);
	auto Pill = [&](float X, const FString& S, const FLinearColor& Col)
	{
		C.Box(X, 22.f, 110.f, 26.f, Hex(TEXT("06060b"), 0.85f));
		C.Glow(FCanvas::RoundRectPoints(X, 22.f, 110.f, 26.f, 13.f), Col, 1.3f, 0.5f);
		C.TextCentered(S, X + 55.f, 35.f, Col, 11.f, true, 100.f);
	};
	Pill(40.f, TrF(TEXT("레벨 %d"), G.Level), Gold);
	Pill(160.f, TrF(TEXT("남은 포인트 %d"), G.PointsLeft()), G.PointsLeft() > 0 ? Green : Grey);
	Pill(280.f, TrF(TEXT("키스톤 %d / %d"), G.KeystoneCount(), KeystoneMax), Gold);
	// '찍은 노드' 알약 · '이번 판에서만…' · 휠 안내 글은 뺌 (D-380)
	PaintCloseButton(C, Purple);
	};
	// 노드 · 연결선은 확대 · 옮기기를 거쳐 그린다 (D-376): 붓의 점 옮기기(PW)를 쓰면 상자 · 글자 · 스프라이트도 같이 커진다
	if (TreeZoom != 1.f || !TreePan.IsNearlyZero()) C.PW = [this](FVector2f Q) { return TreeToScreen(Q); };
	// 연결선: 노드 테두리에서 끊는다 (아이콘을 가로지르지 않게)
	auto NodeR = [](int32 Id) { switch (TreeNode(Id).Kind) { case EPDNode::Start: return 14.f; case EPDNode::Small: return 11.f; case EPDNode::Notable: return 16.f; case EPDNode::Keystone: return 21.f; default: return 17.f; } };
	for (int32 A = 0; A < TreeNodeCount(); ++A)
		for (int B : TreeLinks(A))
		{
			if (B < A) continue;
			const bool bOa = G.TreeOwnedNode(A), bOb = G.TreeOwnedNode(B);
			const FVector2f PA0 = TreeNodePos(A), PB0 = TreeNodePos(B), U = (PB0 - PA0).GetSafeNormal();
			const FVector2f PA = PA0 + U * (NodeR(A) + 1.5f), PB = PB0 - U * (NodeR(B) + 1.5f);
			// 검은 바탕에 묻히지 않게 (D-151): 찍은 길은 노드 색으로 굵고 밝게, 다음에 찍을 수 있는 길은 하늘색으로 숨 쉬듯, 먼 길도 또렷한 회청색
			if (bOa && bOb) C.Glow({PA, PB}, TreeNodeColor(bOa && A != 0 ? A : B), 2.2f, 1.2f);
			else if (bOa || bOb) C.Glow({PA, PB}, Hex(TEXT("7fd6ff"), 0.75f + 0.25f * FMath::Sin(Clock * 3.f)), 1.6f, 0.7f);
			else C.Glow({PA, PB}, Hex(TEXT("8ea6c8"), 0.45f), 1.2f, 0.25f);
		}
	// 노드 아이콘은 자리가 고정이라 한 번만 만든다
	static TArray<PDIcon::FIcon> IconCache;
	if (IconCache.Num() != TreeNodeCount())
	{
		IconCache.Reset();
		for (int32 Id = 0; Id < TreeNodeCount(); ++Id)
		{
			const FPDTreeNode& N = TreeNode(Id);
			const FVector2f P = TreeNodePos(Id);
			if (N.Kind == EPDNode::Small) IconCache.Add(PDIcon::StatIcon((int32)N.Fx[0].Stat, P.X, P.Y, 6.8f));
			else if (N.Kind == EPDNode::Notable) IconCache.Add(PDIcon::StatIcon((int32)N.Fx[0].Stat, P.X, P.Y, 10.f));
			else if (N.Kind == EPDNode::Keystone) IconCache.Add(PDIcon::KeystoneIcon(N.Fx[0].Param, P.X, P.Y, 13.f));
			else IconCache.Add(PDIcon::FIcon());
		}
	}
	const int32 Hov = TreeHoverAt(Mouse);
	for (int32 Id = 0; Id < TreeNodeCount(); ++Id)
	{
		const FPDTreeNode& N = TreeNode(Id);
		const FVector2f P = TreeNodePos(Id);
		const bool bOwn = G.TreeOwnedNode(Id), bSealed = G.TreeSealed(Id);
		const FPDGame::EBuy Can = G.CanBuy(Id);
		const bool bNext = Can == FPDGame::EBuy::Ok || Can == FPDGame::EBuy::NoPoint; // 이어져 있어 찍을 수 있는 자리
		const FLinearColor Col = bSealed ? Red : TreeNodeColor(Id);
		const float Al = bOwn ? 1.f : bNext ? 0.75f : 0.35f;
		const float Gk = bOwn ? 1.f : Can == FPDGame::EBuy::Ok ? 0.5f + 0.8f * Pulse : 0.25f;
		const FLinearColor Ca = WithAlpha(Col, Al);
		const float Th = Id == Hov ? 2.f : 1.4f;
		switch (N.Kind)
		{
		case EPDNode::Start:
			C.Glow(FCanvas::CirclePoints(P, 14.f, 32), Ca, Th, Gk);
			C.Shape(EPDSuit::Wild, P.X, P.Y, 8.f, Ca, 1.2f, Gk);
			break;
		case EPDNode::Small: // 둥근 코스터 (D-293): 바깥 원 + 안쪽 얇은 원
			C.Box(P.X - 9.f, P.Y - 9.f, 18.f, 18.f, Hex(TEXT("06060b"), 0.9f));
			if (bOwn) C.Box(P.X - 7.f, P.Y - 7.f, 14.f, 14.f, WithAlpha(Col, 0.12f));
			C.Glow(FCanvas::CirclePoints(P, 11.f, 28), Ca, Th, Gk);
			C.Lines(FCanvas::CirclePoints(P, 8.f, 22), WithAlpha(Col, Al * 0.45f), 0.8f);
			break;
		case EPDNode::Notable: // 네모 코스터
			C.Box(P.X - 14.f, P.Y - 14.f, 28.f, 28.f, Hex(TEXT("06060b"), 0.9f));
			if (bOwn) C.Box(P.X - 14.f, P.Y - 14.f, 28.f, 28.f, WithAlpha(Col, 0.1f + 0.05f * Pulse));
			C.Glow(FCanvas::RoundRectPoints(P.X - 15.f, P.Y - 15.f, 30.f, 30.f, RadSmall), Ca, Th, Gk);
			C.Lines(FCanvas::RoundRectPoints(P.X - 11.f, P.Y - 11.f, 22.f, 22.f, 4.f), WithAlpha(Col, Al * 0.45f), 0.8f);
			break;
		case EPDNode::Keystone:
		{
			TArray<FVector2f> O;
			for (int32 J = 0; J <= 8; ++J) { const float A = (22.5f + 45.f * J) * TwoPi / 360.f; O.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 21.f); }
			C.Box(P.X - 15.f, P.Y - 15.f, 30.f, 30.f, Hex(TEXT("06060b"), 0.9f));
			if (bOwn) C.Box(P.X - 13.f, P.Y - 13.f, 26.f, 26.f, WithAlpha(Col, 0.12f + 0.06f * Pulse));
			C.Glow(O, Ca, Th + 0.6f, Gk);
			C.Lines(FCanvas::CirclePoints(P, 15.f, 24), WithAlpha(Col, Al * 0.45f), 0.8f);
			// 이름 알약(D-290)은 뺌 (D-377): 옆 노드를 가렸고, 이름은 마우스를 올리면 설명 상자에 나온다
			break;
		}
		case EPDNode::Joker:
			C.JokerTower(N.Fx[0].Param, P.X, P.Y, 16.f, Al, bOwn ? Pulse : 0.f);
			if (Can == FPDGame::EBuy::Ok) C.Glow(FCanvas::RoundRectPoints(P.X - 20.f, P.Y - 20.f, 40.f, 40.f, RadSmall), WithAlpha(JokerPink, 0.5f * Pulse), 1.f, 0.4f);
			break;
		}
		if (N.Kind != EPDNode::Start && N.Kind != EPDNode::Joker) C.Icon(IconCache[Id], WithAlpha(Col, bOwn ? 1.f : bNext ? 0.8f : 0.45f), 0.9f, bOwn ? 0.45f : 0.2f);
		if (bSealed) { C.Glow({P + FVector2f(-13.f, -13.f), P + FVector2f(13.f, 13.f)}, Red, 1.4f, 0.6f); C.Glow({P + FVector2f(13.f, -13.f), P + FVector2f(-13.f, 13.f)}, Red, 1.4f, 0.6f); }
		if (Id == Hov) C.Glow(FCanvas::CirclePoints(P, NodeR(Id) + 6.f, 36), Can == FPDGame::EBuy::Ok ? Green : WithAlpha(Fg, 0.6f), 1.6f, 0.8f);
		if (Id == TreeBuyId && TreeBuyT > 0.f)
		{
			const float U = 1.f - TreeBuyT / 0.8f;
			C.Glow(FCanvas::CirclePoints(P, 14.f + 40.f * U, 40), WithAlpha(Col, 1.f - U), 2.f, 1.f);
		}
	}
	C.PW = nullptr;
	Header();
	// 범례(찍음 · 찍을 수 있음 · 못 찍음 · 봉인, D-130)는 뺌 (D-375): 노드에 마우스를 올리면 설명에 상태가 나온다
	// 설명
	if (Hov >= 0)
	{
		const FPDTreeNode& N = TreeNode(Hov);
		const FPDGame::EBuy Can = G.CanBuy(Hov);
		FString State;
		FLinearColor SCol = Grey;
		switch (Can)
		{
		case FPDGame::EBuy::Ok: State = TrF(TEXT("찍을 수 있음 · 남은 포인트 %d"), G.PointsLeft()); SCol = Green; break;
		case FPDGame::EBuy::Owned: State = Tr(TEXT("찍음")); SCol = Cyan; break;
		case FPDGame::EBuy::NotLinked: State = Tr(TEXT("찍은 노드와 이어져야 한다")); break;
		case FPDGame::EBuy::Sealed: State = Tr(TEXT("짝 키스톤을 찍어서 봉인됨")); SCol = Red; break;
		case FPDGame::EBuy::KeyCap: State = TrF(TEXT("키스톤은 판마다 %d개까지"), KeystoneMax); SCol = Red; break;
		case FPDGame::EBuy::NoPoint: State = TrF(TEXT("포인트 없음 · 다음 레벨까지 경험치 %s"), *CommaNum(G.XpToNext() - G.Xp)); SCol = Red; break;
		default: State = Tr(TEXT("지금은 찍을 수 없다 (보상 고르는 중)")); break;
		}
		TArray<FString> Lines;
		for (const FPDNodeFx& Fx : N.Fx) if (Fx.Stat != EPDStat::None) Lines.Add(TreeFxText(Fx));
		const bool bKey = N.Kind == EPDNode::Keystone;
		const float W = 320.f, H = 52.f + Lines.Num() * 20.f + (bKey ? 20.f : 0.f) + 34.f;
		const FVector2f P = TreeToScreen(TreeNodePos(Hov)); // 설명 상자는 확대하지 않고 화면의 노드 옆에 (D-376)
		const float Off = 34.f * FMath::Max(1.f, TreeZoom);
		const float X = FMath::Clamp(P.X + Off + W > DesignW - 12.f ? P.X - Off - W : P.X + Off, 12.f, DesignW - W - 12.f), Y = FMath::Clamp(P.Y - 60.f, 76.f, DesignH - H - 12.f);
		const FLinearColor Edge = Can == FPDGame::EBuy::Ok ? Green : TreeNodeColor(Hov);
		C.Box(X, Y, W, H, Hex(TEXT("0a0b14"), 0.97f));
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, RadSmall), Edge, 1.4f, 0.6f);
		C.Text(TreeNodeName(Hov), X + 14.f, Y + 22.f, bKey ? Gold : TreeNodeColor(Hov), 14.f, 0, true, W - 28.f);
		static const TCHAR* KindName[] = {TEXT("시작점"), TEXT("작은 노드"), TEXT("특급 노드"), TEXT("키스톤"), TEXT("조커 강화")};
		C.Text(FString::Printf(TEXT("%s · %s"), Tr(KindName[(int)N.Kind]), *State), X + 14.f, Y + 42.f, SCol, 10.f, 0, true, W - 28.f);
		float Ly = Y + 66.f;
		for (const FString& L : Lines) { C.Text(L, X + 14.f, Ly, Fg, 12.f, 0, false, W - 28.f); Ly += 20.f; }
		if (bKey) { C.Text(TrF(TEXT("짝: %s (함께 못 찍음)"), KeystoneName(N.Fx[0].Param ^ 1)), X + 14.f, Ly, Red, 10.f, 0, false, W - 28.f); Ly += 20.f; }
		if (Can == FPDGame::EBuy::Ok)
		{
			C.Glow(FCanvas::RoundRectPoints(X + 14.f, Ly - 6.f, 130.f, 22.f, RadPill(22.f)), Green, 1.2f, 0.5f + 0.4f * Pulse);
			C.TextCentered(TEXT("클릭해서 찍기"), X + 79.f, Ly + 5.f, Green, 10.f, true);
		}
	}
	C.FlushQuads();
	C.PopClip(); // D-379
}

// 시작 화면 (D-153 → D-253 대문 시안 B, D-309 바 앨리스): 벽돌 벽에 바 앨리스 간판, 젖은 바닥 반사, 오른쪽 세로 튜브 버튼.
// 로고는 손으로 그린 네온 선 (PDLogo.h). 켜질 때 글자마다 지직, 그 뒤 몇 초마다 한 글자 옆에서 '빠직' 불티 + 그 글자 깜빡
// BAR Alice 간판 (D-440 ~ D-452, D-453 에서 함수로): 따라 그린 Alice · 잔 획(PDTitleLogo.inl) + 예전 블록 글자 BAR.
// TX, TY = 원본 그림(1139 × 546)의 왼쪽 위, TW = 그릴 너비. NB · PB · CB = 글자 묶음 밝기(깜빡임). Wall = 벽빛 세기 (시작 화면 1, 인게임 0)
// 관 굵기 · 번짐은 너비에 비례 (시작 화면 580 기준)
void SPDBoardWidget::PaintAliceSign(FCanvas& C, float TX, float TY, float TW, const float* NB, const float* PB, float CB, float Wall, float AngleDeg)
{
	const float K = TW / PDTitleLogo::SrcW, Sc = TW / 580.f;
	// 기울이기 (D-454): 간판 가운데를 축으로 모든 점을 돌린다 (둥근 빛은 돌려도 같아 점만 돌리면 된다)
	const FVector2f Piv(TX + TW / 2.f, TY + PDTitleLogo::SrcH * K / 2.f);
	const float Ca = FMath::Cos(FMath::DegreesToRadians(AngleDeg)), Sa = FMath::Sin(FMath::DegreesToRadians(AngleDeg));
	auto Rot = [&](FVector2f V) { if (AngleDeg == 0.f) return V; const FVector2f D = V - Piv; return Piv + FVector2f(D.X * Ca - D.Y * Sa, D.X * Sa + D.Y * Ca); };
	static const TCHAR* Edge[4] = {TEXT("14d2eb"), TEXT("ff2dcd"), TEXT("ffd714"), TEXT("14d2eb")}; // 원본에서 잰 관 바깥 테. 3 = 잔 속 술 · 올리브 (D-472, 하늘)
	static const TCHAR* Core[4] = {TEXT("afffff"), TEXT("ffaff2"), TEXT("fffaa5"), TEXT("afffff")}; // 관 가운데 파스텔
	// 관 번짐 (D-446 · D-452): 둥근 빛을 선을 따라 촘촘히 (띠 스프라이트는 끝 · 꺾임 이음매가 점선 빛 줄로 삐져나갔다)
	const bool bTitle = Wall > 0.f; // 시작 화면: 번짐 넓게 (D-464), 관 선 1.5배 (D-465). 인게임 배지는 그대로
	const float LineK = bTitle ? 1.5f : 1.f;
	auto TubeGlow = [&](const TArray<FVector2f>& Pts, const FLinearColor& Gc, float Th, float B)
	{
		C.Spill(Pts, Gc, Th * (bTitle ? 15.f : 11.f), 0.16f * B, true);
		C.Spill(Pts, Gc, Th * (bTitle ? 6.5f : 5.f), 0.55f * B, true);
		C.Lines(Pts, Gc, Th * LineK);
	};
	// 벽빛은 시작 화면에서 PaintTitleWall 이 전구 빛으로 그린다 (D-465, 예전 둥근 빛 사슬 D-302 · D-463 은 뺐다)
	auto WallSpill = [](const TArray<FVector2f>&, const FLinearColor&, float, float, float, float) {};
	auto Bright = [&](const PDTitleLogo::FStroke& St) { return St.Color == 0 ? NB[FMath::Clamp(St.Group, 0, PDLogo::BlockLetters - 1)] : St.Color == 1 ? PB[FMath::Clamp(St.Group, 0, PDLogo::ScriptLetters - 1)] : CB; };
	TArray<FVector2f> Pts;
	for (int32 Pass = 0; Pass < 2; ++Pass)
		for (int32 Si = 0; Si < PDTitleLogo::NumStrokes; ++Si)
		{
			const PDTitleLogo::FStroke& St = PDTitleLogo::Strokes[Si];
			if (St.Color == 0) continue; // BAR 는 예전 블록 글자로 (D-442, 아래)
			const float B = Bright(St);
			const FLinearColor Ec = Hex(Edge[St.Color]), Cc = Hex(Core[St.Color]);
			const float Th = PDTitleLogo::Tube[St.Color] * K * 0.8f;
			Pts.Reset();
			for (int32 Q = 0; Q < St.N; ++Q) Pts.Add(Rot(FVector2f(TX + St.P[Q * 2] * K, TY + St.P[Q * 2 + 1] * K)));
			if (Pass == 0) { WallSpill(Pts, Ec, Th, B, 0.13f, 0.08f); continue; } // 벽빛 (D-302 · D-449)
			if (B <= 0.25f) { C.Lines(Pts, FLinearColor(Ec.R * 0.28f, Ec.G * 0.28f, Ec.B * 0.28f, 1.f), Th * 0.8f); continue; } // 꺼진 관
			TubeGlow(Pts, FLinearColor(Ec.R * B, Ec.G * B, Ec.B * B, 1.f), Th, B);
			C.Lines(Pts, FLinearColor(Cc.R * B, Cc.G * B, Cc.B * B, 1.f), Th * 0.6f * LineK);
		}
	// BAR (D-442): 틀 없는 하늘색 블록 글자 B · A · R (PDLogo, 글자마다 NB 로 깜빡임). 새 간판의 BAR 꼬리표 글자 자리(원본 218, 142 가운데)에
	const float BKn = 0.8525f * Sc; // 예전 배율 0.55 × 1.55
	const float Cx = TX + 218.f * K, Cy = TY + 142.f * K;
	const float X0 = Cx - PDLogo::BlockW * BKn / 2.f, Base = Cy + 30.f * BKn;
	const FLinearColor BarC = Hex(TEXT("6edcff"));
	const float BarTh = 2.6f * Sc;
	PDLogo::FPoly Ps[3];
	for (int32 Pass = 0; Pass < 2; ++Pass)
		for (int32 I = 0; I < PDLogo::BlockLetters; ++I)
		{
			const int32 N = PDLogo::BlockPolys(I, Ps);
			for (int32 J2 = 0; J2 < N; ++J2)
			{
				TArray<FVector2f> BP;
				for (int32 Q = 0; Q < Ps[J2].N; ++Q) BP.Add(Rot(FVector2f(X0 + (I * PDLogo::BlockAdv + Ps[J2].P[Q * 2] * 0.8f) * BKn, Base + Ps[J2].P[Q * 2 + 1] * BKn)));
				const float B = NB[I];
				if (Pass == 0) { WallSpill(BP, BarC, BarTh, B, 0.22f, 0.13f); continue; }
				if (B <= 0.25f) { C.Lines(BP, FLinearColor(BarC.R * 0.28f, BarC.G * 0.28f, BarC.B * 0.28f, 1.f), BarTh * 0.8f); continue; }
				TubeGlow(BP, FLinearColor(BarC.R * B, BarC.G * B, BarC.B * B, 1.f), BarTh, B);
				C.Lines(BP, FLinearColor(1.f, 1.f, 1.f, 0.4f * B), BarTh * 0.25f * LineK); // 튜브 가운데 흰 심
			}
		}
}

void SPDBoardWidget::PaintTitle(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("6edcff")), Pink = Hex(TEXT("ff6aaa")), Gold = Hex(TEXT("ffd26e")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 1.f)); // 뒤 게임판이 비치지 않게 불투명 (D-255)
	// 벽돌 벽 · 전구 벽빛은 간판 바로 앞에 (PaintTitleWall, D-465: 글자 밝기를 따라 깜빡여서 밝기 계산 뒤로)

	// ---- 로고 밝기: 켜짐 → 빠직
	const float T = TitleT, Off = 0.08f;
	float NB[PDLogo::BlockLetters], PB[PDLogo::ScriptLetters];
	// D-471: 간판 전체가 1초 안에 켜진다. Alice 는 0.3초부터 0.7초 동안 지직지직 깜빡이다 켜짐 (예전엔 1.6초에 시작해 0.24초)
	static const float OnN[] = {0.5f, 0.08f, 0.9f, 0.3f, 1.f};
	static const float OnP[] = {0.4f, 0.08f, 0.08f, 0.9f, 0.1f, 0.08f, 0.6f, 1.f, 0.1f, 0.08f, 0.8f, 0.3f, 1.f, 0.5f, 1.f, 1.f, 1.f};
	auto Seq = [](const float* S, int32 N, float U) { if (U < 0.f) return 0.08f; const int32 I = (int32)(U * 25.f); return I >= N ? 1.f : S[I]; };
	for (int32 I = 0; I < PDLogo::BlockLetters; ++I) NB[I] = Seq(OnN, 5, T - 0.1f - I * 0.12f);
	for (int32 I = 0; I < PDLogo::ScriptLetters; ++I) PB[I] = Seq(OnP, UE_ARRAY_COUNT(OnP), T - 0.3f);
	const float CB = FMath::Clamp((T - 0.85f) / 0.15f, 0.1f, 1.f);
	static const float PatN[] = {0.1f, 1.f, 0.08f, 0.08f, 0.7f, 0.08f, 1.f, 1.f, 0.5f, 1.f, 1.f, 1.f};
	static const float PatP[] = {0.1f, 1.f, 0.08f, 0.6f, 0.08f, 0.08f, 1.f, 0.4f, 1.f, 1.f, 1.f, 1.f};
	int32 SpWhich = -1, SpLetter = 0, SpK = 0, SpSeed = 0;
	FVector2f SparkBar[PDLogo::BlockLetters], SparkScript[PDLogo::ScriptLetters]; // 빠직 불티 자리 (D-444: 새 간판 글자 자리)
	bool bSparkSet = false;
	if (T > 2.5f)
	{
		const float U0 = T - 2.5f;
		const int32 Cyc = (int32)(U0 / 6.f);
		const float U = U0 - Cyc * 6.f;
		const uint32 Hs = (uint32)(Cyc + 7) * 2654435761u;
		if (U >= 1.2f && U < 1.2f + 12.f / 25.f) { SpWhich = 0; SpLetter = (int32)((Hs >> 8) % (uint32)PDLogo::BlockLetters); SpK = FMath::Min(11, (int32)((U - 1.2f) * 25.f)); NB[SpLetter] = PatN[SpK]; SpSeed = Cyc * 2 + 1; }
		else if (U >= 3.8f && U < 3.8f + 12.f / 25.f) { SpWhich = 1; SpLetter = (int32)((Hs >> 12) % (uint32)PDLogo::ScriptLetters); SpK = FMath::Min(11, (int32)((U - 3.8f) * 25.f)); PB[SpLetter] = PatP[SpK]; SpSeed = Cyc * 2 + 2; }
	}
	(void)Off;

	// ---- 로고: 묶음(BAR 윗선 ~ 부제)이 화면 위아래 가운데 (D-381, 예전 LY 110 은 아래 바닥 반사 자리를 남겼다)
	const float LgX = 400.f, LK = 1.55f, LY = DesignH / 2.f - 97.f * LK; // BAR 윗선 LY - 6LK ~ 부제 LY + 200LK 의 가운데
	// 로고 배치 C (사용자 확정): Alice 를 1.25배로 크게 가운데, BAR 는 왼쪽 위 작게(0.55배, 알약 틀 없음), 마티니 잔 · 부제는 A 배치 모양 (tools/neon/logo_alice.py logo_C)
	const float BK = 0.55f * LK, SK = 1.25f * LK, BY0 = LY - 6.f * LK;
	const float NX0 = LgX - 190.f * LK;
	const FVector2f PO(LgX - PDLogo::ScriptW / 2.f * SK, LY + 150.f * LK);
	bool bSpill = false; // 벽빛 먼저 한 바퀴 (D-302): 켜진 튜브 뒤 벽돌에 은은하게 비치는 빛. 깜빡여 꺼지면 같이 꺼진다
	auto Tube = [&](const TArray<FVector2f>& Pts, const FLinearColor& Col, float Th, float B)
	{
		if (bSpill)
		{
			if (B <= 0.25f) return;
			const float ST = FMath::Min(Th, 4.5f); // 벽빛 너비는 예전 굵기 기준 (D-427: Alice 를 굵혀도 벽빛은 그대로)
			C.Spill(Pts, Col, ST * 16.f, 0.22f * B, true);
			C.Spill(Pts, Col, ST * 44.f, 0.13f * B, true);
			return;
		}
		if (B <= 0.25f) { C.Lines(Pts, FLinearColor(Col.R * 0.28f, Col.G * 0.28f, Col.B * 0.28f, 1.f), Th * 0.8f); return; }
		C.Glow(Pts, FLinearColor(Col.R * B, Col.G * B, Col.B * B, 1.f), Th, 1.1f * B); // 버튼 · 예전 로고는 띠 번짐 (D-450: 가는 버튼 관에 둥근 빛 사슬을 쓰니 2px 마다 원을 찍어 한 프레임 꼭짓점 46만 개, 시작 화면이 16ms 더 걸려 끊겼다)
		C.Lines(Pts, FLinearColor(1.f, 1.f, 1.f, 0.4f * B), Th * 0.25f); // 튜브 가운데 흰 심
	};
	auto Logo = [&]()
	{
		auto Map = [](FVector2f V) { return V; }; // 예전 바닥 반사 뒤집기 자리 (D-381 에서 반사를 뺐다)
		PDLogo::FPoly Ps[3];
		for (int32 I = 0; I < PDLogo::BlockLetters; ++I)
		{
			const int32 N = PDLogo::BlockPolys(I, Ps);
			for (int32 J = 0; J < N; ++J)
			{
				TArray<FVector2f> Pts;
				for (int32 Q = 0; Q < Ps[J].N; ++Q) Pts.Add(Map(FVector2f(NX0 + (I * PDLogo::BlockAdv + Ps[J].P[Q * 2] * 0.8f) * BK, BY0 + (60.f + Ps[J].P[Q * 2 + 1]) * BK)));
				Tube(Pts, Cyan, 1.8f, NB[I]);
			}
		}
		// 이어지는 글자(A → l → i, c → e)는 밝기가 같으면 조절점부터 한 줄로 (D-299: 글자마다 끝이 따로 있으면 이음매에 자국이 남았다)
		for (const PDLogo::FRun& Rn : PDLogo::ScriptRuns(PB)) Tube(PDLogo::CatmullPts(Rn.Pts, PO, SK, 0.f), Pink, 8.5f, PB[Rn.Letter]); // 4px 더 굵게 (D-427, 예전 4.5)
		// 마티니 잔 (D-309: 예전 왕관 스페이드 자리) — 역삼각 잔 + 다리 + 받침, 올리브 자리에 분홍 스페이드 + 꼬치
		const float CX = LgX + 168.f * LK, CY = LY + 96.f * LK, R = 26.f * LK;
		Tube({Map(FVector2f(CX - R, CY - R * 0.9f)), Map(FVector2f(CX + R, CY - R * 0.9f)), Map(FVector2f(CX, CY + R * 0.15f)), Map(FVector2f(CX - R, CY - R * 0.9f))}, Gold, 2.2f, CB);
		Tube({Map(FVector2f(CX, CY + R * 0.15f)), Map(FVector2f(CX, CY + R * 0.95f))}, Gold, 2.f, CB);
		Tube({Map(FVector2f(CX - R * 0.5f, CY + R * 0.95f)), Map(FVector2f(CX + R * 0.5f, CY + R * 0.95f))}, Gold, 2.f, CB);
		TArray<FVector2f> Sp = FCanvas::HeartPoints(CX + R * 0.1f, CY - R * 0.48f, R * 0.26f, -1.f, -R * 0.03f);
		for (FVector2f& V : Sp) V = Map(V);
		Tube(Sp, Pink, 1.6f, CB);
		Tube({Map(FVector2f(CX + R * 0.1f, CY - R * 0.3f)), Map(FVector2f(CX + R * 0.62f, CY - R * 1.25f))}, Hex(TEXT("787d8c")), 1.2f, CB);
	};
	{ // 시작 화면 간판 (D-440): 사용자가 준 BAR Alice 네온 간판을 따라 그린 획(PDTitleLogo.inl, tools/ui_art/trace_logo.py)을 네온 관으로.
	  // 글자마다 깜빡이며 켜진다: 하늘 = BAR 글자 NB, 분홍 = A·l · i · c · e PB, 노랑 = 잔 CB. 벽빛 한 바퀴 먼저
		constexpr float TW = 580.f, TH = TW * PDTitleLogo::SrcH / PDTitleLogo::SrcW;
		const float TX = LgX - TW / 2.f, TY = LY + 168.f * LK - TH - 4.f; // 부제 선(LY + 178LK) 바로 위에 바닥
		const float K = TW / PDTitleLogo::SrcW;
		PaintTitleWall(C, TX, TY, TW, NB, PB, CB); // 벽돌 벽 + 전구 벽빛 (D-465)
		PaintAliceSign(C, TX, TY, TW, NB, PB, CB, 1.f); // D-453: 인게임 족보 메뉴판 로고와 같은 그리기
		// 빠직 불티 자리 (D-444): BAR 는 글자 위끝, Alice 는 글자 묶음마다 가장 높은 점
		{
			const float BKn = 0.8525f, X0 = TX + 218.f * K - PDLogo::BlockW * BKn / 2.f, Top = TY + 142.f * K + 30.f * BKn - 60.f * BKn;
			for (int32 I = 0; I < PDLogo::BlockLetters; ++I) SparkBar[I] = FVector2f(X0 + (I * PDLogo::BlockAdv + 35.f) * BKn, Top + 6.f * BKn);
			for (int32 G = 0; G < PDLogo::ScriptLetters; ++G) SparkScript[G] = FVector2f(TX + 300.f * K, TY + 200.f * K);
			float BestY[PDLogo::ScriptLetters];
			for (float& V : BestY) V = 1e9f;
			for (int32 Si = 0; Si < PDTitleLogo::NumStrokes; ++Si)
			{
				const PDTitleLogo::FStroke& St = PDTitleLogo::Strokes[Si];
				if (St.Color != 1) continue;
				const int32 G = FMath::Clamp(St.Group, 0, PDLogo::ScriptLetters - 1);
				for (int32 Q = 0; Q < St.N; ++Q)
					if (St.P[Q * 2 + 1] < BestY[G]) { BestY[G] = St.P[Q * 2 + 1]; SparkScript[G] = FVector2f(TX + St.P[Q * 2] * K, TY + St.P[Q * 2 + 1] * K); }
			}
			bSparkSet = true;
		}
	}
	(void)Logo; // 예전 코드 간판 (D-309 ~ D-427): 따라 그린 간판(D-440)으로 바뀌어 안 쓴다
	// 부제 (D-311 금색 선 + LATE NIGHT POKER) 는 D-470 에서 뺐다
	// 빠직: 작은 섬광 + 짧은 번개, 불티가 튀어 떨어지며 꺼진다
	if (SpWhich >= 0)
	{
		const FVector2f O = bSparkSet ? (SpWhich == 0 ? SparkBar[SpLetter] : SparkScript[SpLetter])
			: SpWhich == 0 ? FVector2f(NX0 + (SpLetter * PDLogo::BlockAdv + 35.f) * BK, BY0 + 6.f * BK)
			: PO + FVector2f(PDLogo::ScriptSpark[SpLetter][0], PDLogo::ScriptSpark[SpLetter][1]) * SK;
		FRandomStream Rs(SpSeed * 7919 + 13);
		const FLinearColor Hot(1.f, 0.96f, 0.82f, 1.f), Warm(1.f, 0.88f, 0.59f, 1.f);
		if (SpK == 0)
		{
			C.Glow(FCanvas::CirclePoints(O, 3.f, 10), Hot, 2.5f, 1.6f);
			for (int32 K = 0; K < 2; ++K)
			{
				const float A = Rs.FRandRange(0.f, TwoPi);
				const FVector2f P1 = O + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 7.f;
				const float A2 = A + Rs.FRandRange(-1.f, 1.f);
				C.Glow({O, P1, P1 + FVector2f(FMath::Cos(A2), FMath::Sin(A2)) * 8.f}, Hot, 1.4f, 1.f);
			}
		}
		else if (SpK == 1) C.Glow(FCanvas::CirclePoints(O, 1.8f, 8), Hot, 2.f, 1.f);
		FRandomStream Rp(SpSeed * 104729 + 3);
		for (int32 K = 0; K < 8; ++K)
		{
			const float A = Rp.FRand() < 0.7f ? Rp.FRandRange(-PI * 0.95f, -PI * 0.05f) : Rp.FRandRange(0.f, PI);
			const float Sp2 = Rp.FRandRange(2.4f, 5.f);
			const int32 Life = Rp.RandRange(6, 11);
			const bool bWhite = Rp.FRand() >= 0.7f;
			if (SpK < 1 || SpK > Life) continue;
			auto At = [&](float Tt) { return O + FVector2f(FMath::Cos(A) * Sp2 * Tt, FMath::Sin(A) * Sp2 * Tt + 0.35f * Tt * Tt); };
			const float Fade = 1.f - (SpK / (float)Life) * 0.8f;
			C.Glow({At(SpK - 1.f), At((float)SpK)}, WithAlpha(bWhite ? Hot : Warm, Fade), 1.5f, Fade);
		}
	}

	// ---- 칩 알약 (오른쪽 위)
	{
		const FString S = TrF(TEXT("칩 %s"), *CommaNum(MetaChips));
		const float PW2 = 46.f + FCanvas::Measure(S, FCanvas::Font(13.f, true)).X, PH = 30.f, PX = DesignW - 80.f - PW2, PY = 24.f;
		C.Glow(FCanvas::RoundRectPoints(PX, PY, PW2, PH, PH / 2.f), Gold, 1.4f, 0.7f);
		C.Shape(EPDSuit::Diamond, PX + 17.f, PY + PH / 2.f, 6.f, Gold, 1.3f, 0.5f);
		C.Text(S, PX + 30.f, PY + PH / 2.f + 5.f, Gold, 13.f, 0, true);
	}

	// ---- 튜브 버튼 (D-381): 색 규칙 = 시작 · 이어하기 초록, 종료 분홍, 나머지 하늘색. 고른 것은 같은 색으로 밝아지고 밑에 같은 모양 빛 판, 왼쪽 하트도 그 색
	static const TCHAR* Labels[] = {TEXT("이어하기"), TEXT("새 게임"), TEXT("영구 강화"), TEXT("잠금 해제"), TEXT("환경 설정"), TEXT("게임 종료")};
	const bool bCanBuy = AffordableNames().Num() > 0;
	float LastBtnBot = 0.f;
	for (int32 I = 0; I < (int32)ETitleItem::Count; ++I)
	{
		if (I == (int32)ETitleItem::Continue && !bHasRunSave) continue; // 이어할 판이 없으면 위로 당긴다 (D-162)
		const FVector2f P = TitleBtnPos(I, bHasRunSave);
		LastBtnBot = P.Y + TitleBtnH;
		const bool bSel = TitleSel == I;
		const int32 PrimaryI = bHasRunSave ? (int32)ETitleItem::Continue : (int32)ETitleItem::Start; // 초록 강조는 하나만: 이어할 판이 있으면 이어하기, 없으면 새 게임 (D-426: 예전 이름 게임 시작)
		const FLinearColor Col = I == PrimaryI ? BtnColPrimary() : I == (int32)ETitleItem::Quit ? BtnColExit() : BtnColDefault();
		// 네온 간판 버튼 (D-254, D-255): 불이 들어온 튜브 두 겹 (로고와 같은 흰 심 튜브)
		TArray<FVector2f> Rr = FCanvas::RoundRectPoints(P.X, P.Y, TitleBtnW, TitleBtnH, TitleBtnH / 2.f);
		TArray<FVector2f> Ri = FCanvas::RoundRectPoints(P.X + 6.f, P.Y + 6.f, TitleBtnW - 12.f, TitleBtnH - 12.f, (TitleBtnH - 12.f) / 2.f);
		const float Bo = bSel ? 1.f : 0.62f;
		C.BtnFill(P.X, P.Y, TitleBtnW, TitleBtnH, RadPill(TitleBtnH), Col, bSel); // 눌리는 버튼 바탕 (D-328 ②) + 고르면 빛 판 (D-381)
		Tube(Rr, Col, bSel ? 2.8f : 2.f, Bo);
		Tube(Ri, Col, 1.2f, bSel ? 0.8f + 0.1f * Pulse : 0.38f);
		for (const float Bx : {P.X + 3.f + TitleBtnH / 4.f, P.X + TitleBtnW - 3.f - TitleBtnH / 4.f})
			C.Lines(FCanvas::CirclePoints(FVector2f(Bx, P.Y + TitleBtnH / 2.f), 2.2f, 10), Hex(TEXT("787d8c"), 0.7f), 1.f);
		const FLinearColor TxC = bSel ? FLinearColor(FMath::Min(1.f, Col.R * 1.15f + 0.08f), FMath::Min(1.f, Col.G * 1.15f + 0.08f), FMath::Min(1.f, Col.B * 1.15f + 0.08f), 1.f) : WithAlpha(Col, 0.78f); // 흰색으로 바뀌지 않고 같은 색이 밝아진다
		C.TextCentered(Labels[I], P.X + TitleBtnW / 2.f, P.Y + TitleBtnH / 2.f, TxC, 18.f, true);
		if (I == (int32)ETitleItem::Unlock && bCanBuy) // 살 게 있으면 반짝이는 점 (D-321 ③)
		{
			const FVector2f Dp(P.X + TitleBtnW - 10.f, P.Y + 4.f);
			C.SoftDisc(Dp, 9.f, 9.f, Gold, 0.35f + 0.25f * Pulse);
			C.Glow(FCanvas::CirclePoints(Dp, 3.5f, 12), Gold, 1.6f, 0.8f + 0.4f * Pulse);
		}
	}
	if (Game.BestWave > 0) C.Text(TrF(TEXT("최고 기록 %d웨이브"), Game.BestWave), TitleBtnX, LastBtnBot + 26.f, Grey, 12.f, 0, false); // 버튼 묶음 아래 (D-381)
	C.TextCentered(TEXT("위아래 키 또는 마우스로 고르기 · Enter 또는 클릭으로 결정"), DesignW / 2.f, DesignH - 20.f, Dim, 11.f, false);
}

// 모드 고르기 창 (D-229): 카드 4장. 잠긴 모드는 흐리게 + 여는 조건, 클리어한 모드는 금색 표시
void SPDBoardWidget::PaintModeSel(FCanvas& C) const
{
	const FLinearColor Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Gold = Hex(TEXT("ffd96a")), Cyan = Hex(TEXT("7fd6ff")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 모드 고르기 = 바 부스 넷 (D-293): 'PICK A BOOTH' 간판, 부스마다 차양 · 문틀 튜브 · 큰 숫자, 잠긴 부스는 불 꺼짐 + 자물쇠, 클리어한 부스는 금별
	const FLinearColor PinkM = Hex(TEXT("ff6aaa"));
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.88f));
	BulbSign(C, DesignW / 2.f, TutY - 10.f, 380.f, 70.f, TEXT("PICK A BOOTH"), Gold, 26.f, Clock);
	C.TextCentered(TEXT("목표 웨이브를 버티면 클리어. 30 을 깨면 60 과 무한이, 60 을 깨면 100 이 열린다"), DesignW / 2.f, TutY + 86.f, Fg, 12.f, false, TutW - 60.f);
	static const TCHAR* Big[(int32)EPDMode::Count] = {TEXT("30"), TEXT("60"), TEXT("100"), TEXT("∞")};
	static const TCHAR* Sub[(int32)EPDMode::Count] = {TEXT("웨이브 버티기"), TEXT("웨이브 버티기"), TEXT("웨이브 버티기"), TEXT("끝없이")};
	static const TCHAR* Need[(int32)EPDMode::Count] = {TEXT(""), TEXT("30 웨이브를 클리어하면 열림"), TEXT("60 웨이브를 클리어하면 열림"), TEXT("30 웨이브를 클리어하면 열림")};
	static const TCHAR* Hint[(int32)EPDMode::Count] = {TEXT("처음은 여기서"), TEXT("길어진 판, 더 센 최종 보스"), TEXT("가장 긴 판"), TEXT("어디까지 버티나")};
	for (int32 M = 0; M < (int32)EPDMode::Count; ++M)
	{
		const FVector2f P = ModeCardPos(M);
		const bool bOpen = ModeOpen(M), bSel = ModeSel == M, bHov = InRect(Mouse, P.X, P.Y, ModeCardW, ModeCardH);
		const bool bCleared = (ModesCleared & (1u << M)) != 0;
		const FLinearColor Base = !bOpen ? Dim : M == (int32)EPDMode::Endless ? PinkM : M == 0 ? Cyan : Gold;
		const FLinearColor Col = bSel && bOpen ? PinkM : Base;
		const float BB = !bOpen ? 0.4f : bSel ? 0.9f + 0.1f * Pulse : bHov ? 0.9f : 0.75f;
		C.Box(P.X, P.Y, ModeCardW, ModeCardH, Hex(TEXT("0a0b14"), 0.92f));
		if (bSel && bOpen) C.Box(P.X + 6.f, P.Y + 6.f, ModeCardW - 12.f, ModeCardH - 12.f, WithAlpha(PinkM, 0.06f));
		C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, ModeCardW, ModeCardH, RadPanel), FLinearColor(Col.R * BB, Col.G * BB, Col.B * BB, 1.f), bSel ? 2.4f : 1.9f, BB);
		C.Glow(FCanvas::RoundRectPoints(P.X + 5.f, P.Y + 5.f, ModeCardW - 10.f, ModeCardH - 10.f, RadSmall), WithAlpha(Col, 0.6f * BB), 1.f, 0.4f * BB);
		{ // 차양 (지그재그)
			TArray<FVector2f> Aw;
			for (int32 K = 0; K <= 10; ++K) Aw.Add(FVector2f(P.X + K * ModeCardW / 10.f, P.Y + 30.f + (K % 2 ? 8.f : 0.f)));
			C.Glow(Aw, WithAlpha(Col, BB), 1.3f, 0.4f * BB);
		}
		C.TextCentered(TEXT("BOOTH"), P.X + ModeCardW / 2.f, P.Y + 18.f, WithAlpha(Col, BB), 10.f, true);
		if (!bOpen) // 자물쇠
		{
			const float LX0 = P.X + ModeCardW / 2.f;
			C.Glow(FCanvas::RoundRectPoints(LX0 - 12.f, P.Y + 50.f, 24.f, 18.f, 3.f), Dim, 1.4f, 0.3f);
			C.Lines({FVector2f(LX0 - 7.f, P.Y + 50.f), FVector2f(LX0 - 7.f, P.Y + 42.f), FVector2f(LX0, P.Y + 36.f), FVector2f(LX0 + 7.f, P.Y + 42.f), FVector2f(LX0 + 7.f, P.Y + 50.f)}, Dim, 1.4f);
		}
		NeonText(C, Big[M], P.X + ModeCardW / 2.f, P.Y + 108.f, bOpen ? Col : Dim, 48.f, 1, bOpen ? 1.f : 0.45f);
		C.TextCentered(Sub[M], P.X + ModeCardW / 2.f, P.Y + 128.f, bOpen ? Fg : Dim, 13.f, false, ModeCardW - 16.f);
		if (!bOpen)
			C.TextCentered(Need[M], P.X + ModeCardW / 2.f, P.Y + 162.f, WithAlpha(Red, 0.8f), 10.f, false, ModeCardW - 14.f);
		else
		{
			const TArray<int32> L = RankList(M);
			C.TextCentered(Hint[M], P.X + ModeCardW / 2.f, P.Y + 152.f, Dim, 10.f, false, ModeCardW - 14.f);
			if (L.Num() > 0) C.TextCentered(TrF(TEXT("최고 %d웨이브"), Ranking[L[0]].Wave), P.X + ModeCardW / 2.f, P.Y + 172.f, Fg, 11.f, false, ModeCardW - 14.f);
		}
		if (bCleared) { C.Shape(EPDSuit::Wild, P.X + ModeCardW - 22.f, P.Y + 48.f, 9.f, Gold, 1.4f, 0.6f); C.TextCentered(TEXT("클리어 ✓"), P.X + ModeCardW / 2.f, P.Y + ModeCardH - 18.f, Gold, 12.f, true, ModeCardW - 14.f); }
	}
	C.TextCentered(TEXT("←→ 고르기 · Enter 또는 클릭: 시작 · Esc 닫기"), DesignW / 2.f, TutBtnY + TutBtnH / 2.f + 5.f, Dim, 11.f, false, 500.f);
}

// 랭킹 창 (D-168): 개인 랭킹 탭 (상위 20) + 리더보드 탭 (스팀, 준비 중)
void SPDBoardWidget::PaintRanking(FCanvas& C) const
{
	const FLinearColor Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Gold = Hex(TEXT("ffd96a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.85f));
	C.Box(TutX, TutY, TutW, TutH, Hex(TEXT("06060b"), 0.96f));
	C.Glow(FCanvas::RoundRectPoints(TutX, TutY, TutW, TutH, RadPanel), UiEdge, 1.4f, 0.7f);
	C.TextCentered(TEXT("랭킹"), DesignW / 2.f, TutY + 34.f, UiTitle, 24.f, true);
	static const TCHAR* Tabs[(int32)EPDMode::Count] = {TEXT("30 웨이브"), TEXT("60 웨이브"), TEXT("100 웨이브"), TEXT("무한")}; // 탭 = 모드 (D-229)
	for (int32 I = 0; I < (int32)EPDMode::Count; ++I)
	{
		const float X = RankTabX(I);
		const bool bOn = RankTab == I, bHov = InRect(Mouse, X, RankTabY, RankTabW, RankTabH);
		if (bOn) C.Box(X, RankTabY, RankTabW, RankTabH, WithAlpha(UiEdge, 0.14f));
		C.Glow(FCanvas::RoundRectPoints(X, RankTabY, RankTabW, RankTabH, RadPill(RankTabH)), UiEdge, bOn || bHov ? 1.6f : 1.1f, bOn ? 1.f : 0.35f);
		C.TextCentered(Tabs[I], X + RankTabW / 2.f, RankTabY + RankTabH / 2.f, bOn ? HotCol(UiEdge) : Fg, 14.f, true, RankTabW - 8.f);
	}
	const float TopY = RankTabY + RankTabH + 22.f;
	{
		const TArray<int32> List = RankList(RankTab);
		// 머리줄: 순위 · 웨이브 · 처치 · 최고 판 위력 · 날짜 (두 단으로 10개씩)
		static const TCHAR* Heads[5] = {TEXT("순위"), TEXT("웨이브"), TEXT("처치"), TEXT("최고 위력"), TEXT("날짜")};
		const float ColX[5] = {14.f, 62.f, 128.f, 196.f, 286.f};
		const float HalfW = (TutW - 48.f) / 2.f;
		for (int32 Side = 0; Side < 2; ++Side)
			for (int32 K = 0; K < 5; ++K) C.Text(Heads[K], TutX + 24.f + Side * HalfW + ColX[K], TopY, Dim, 10.f, 0, true, 80.f);
		if (List.Num() == 0) C.TextCentered(TEXT("아직 기록이 없다. 판이 끝나면 여기에 남는다"), DesignW / 2.f, TopY + 120.f, Dim, 13.f, false, TutW - 60.f);
		for (int32 I = 0; I < List.Num() && I < RankMax; ++I)
		{
			const FRankEntry& E = Ranking[List[I]];
			const int32 Side = I / 10, Row = I % 10;
			const float X = TutX + 24.f + Side * HalfW, Y = TopY + 26.f + Row * 26.f;
			const bool bNew = I == LastRankPos && RankTab == (int32)Game.Mode;
			if (bNew) C.Box(X, Y - 15.f, HalfW - 8.f, 22.f, WithAlpha(Gold, 0.08f + 0.06f * Pulse));
			else if (Row % 2 == 0) C.Box(X, Y - 15.f, HalfW - 8.f, 22.f, Hex(TEXT("0a0b14"), 0.8f));
			const FLinearColor Col = I == 0 ? Gold : I < 3 ? UiValue : Fg;
			const FDateTime D = FDateTime::FromUnixTimestamp(E.Time);
			C.Text(FString::Printf(TEXT("%d"), I + 1), X + ColX[0], Y, Col, 12.f, 0, true, 40.f);
			C.Text(FString::Printf(TEXT("%d"), E.Wave), X + ColX[1], Y, Col, 12.f, 0, true, 60.f);
			C.Text(CommaNum(E.Kills), X + ColX[2], Y, Fg, 11.f, 0, false, 64.f);
			C.Text(ShortNum(E.Power), X + ColX[3], Y, Fg, 11.f, 0, false, 84.f);
			C.Text(FString::Printf(TEXT("%04d-%02d-%02d"), D.GetYear(), D.GetMonth(), D.GetDay()), X + ColX[4], Y, Dim, 10.f, 0, false, HalfW - ColX[4] - 12.f);
		}
	}
	// 닫기 버튼 (튜토리얼 '다음' 자리)
	const bool bHov = InRect(Mouse, TutNextX, TutBtnY, TutBtnW, TutBtnH);
	C.Box(TutNextX, TutBtnY, TutBtnW, TutBtnH, WithAlpha(UiEdge, bHov ? 0.14f : 0.08f));
	C.Glow(FCanvas::RoundRectPoints(TutNextX, TutBtnY, TutBtnW, TutBtnH, RadPill(TutBtnH)), UiEdge, bHov ? 1.6f : 1.2f, bHov ? 1.f : 0.6f);
	C.TextCentered(TEXT("닫기"), TutNextX + TutBtnW / 2.f, TutBtnY + TutBtnH / 2.f, FLinearColor::White, 14.f, true, TutBtnW - 8.f);
	C.Text(TEXT("←→ 탭 바꾸기 · Esc 닫기"), TutX + 24.f, TutBtnY + TutBtnH / 2.f + 5.f, Dim, 11.f, 0, false, 300.f);
}

// 로딩 화면 (D-153): 게임 시작을 누르면 잠깐. 무늬 넷이 돌고 채워지는 막대 + 도움말 한 줄, 끝나면 새 판
void SPDBoardWidget::PaintLoading(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6"));
	const float CX = DesignW / 2.f, U = FMath::Clamp(1.f - LoadT / LoadDur, 0.f, 1.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b")));
	for (int32 I = 0; I < 4; ++I)
	{
		const float A = Clock * 2.4f + I * HALF_PI;
		C.Shape((EPDSuit)I, CX + FMath::Cos(A) * 46.f, 290.f + FMath::Sin(A) * 46.f, 12.f, SuitColor((EPDSuit)I), 1.5f, 0.9f);
	}
	C.TextCentered(TEXT("판을 준비하는 중"), CX, 390.f, Cyan, 20.f, true);
	const float BX = CX - 200.f, BY = 418.f, BW = 400.f, BH = 6.f;
	C.Box(BX, BY, BW, BH, Hex(TEXT("1a1d2a")));
	C.Box(BX, BY, BW * U, BH, WithAlpha(Cyan, 0.85f));
	C.Glow(FCanvas::RectPoints(BX - 3.f, BY - 3.f, BW + 6.f, BH + 6.f), Cyan, 1.f, 0.4f);
	static const TCHAR* Tips[] = {
		TEXT("붙어 있는 타워들이 포커 족보가 되면 시너지 (모양 상관없이)"),
		TEXT("족보끼리 맞닿으면 연쇄 보너스가 붙는다"),
		TEXT("합성은 왼쪽 족보 목록에서 족보를 누르고 합쳐질 칸을 클릭"),
		TEXT("족보 3개(또는 2개)를 골라 조커를 만들 수 있다 (판 어디에 있어도)"),
		TEXT("자리바꿈 보상으로 타워 둘의 자리를 바꿀 수 있다"),
		TEXT("무늬는 공격 방식, 숫자는 공격력")};
	C.TextCentered(Tips[FMath::Clamp(LoadTip, 0, 5)], CX, 466.f, Fg, 13.f, false, 760.f);
}

// 왼쪽 족보 안내 (D-119): 이름 + 효과 한 줄. 판 위에 있는 족보는 밝게, 개수 표시
// 도움말 카드 (D-217): 가리킬 자리 둘레에 숨 쉬는 고리 + 카드에서 이어지는 선, 카드엔 제목·설명·[알겠어]·[도움말 끄기]
// 동그란 전구 (D-294): 켜진 전구 = 부드러운 둥근 빛 + 꽉 찬 알 + 흰 심, 꺼진 전구 = 흐린 얇은 원. 톱니처럼 보이던 10각 테두리 대신
void SPDBoardWidget::Bulb(FCanvas& C, FVector2f P, float R, bool bOn, const FLinearColor& Col)
{
	const FCanvas::FObjScope Ob(C, TEXT("Bulb")); // 레이아웃 검사 그림 덩어리 (D-366)
	if (!bOn) { C.Lines(FCanvas::CirclePoints(P, R, 20), FLinearColor(Col.R, Col.G, Col.B, 0.2f), 1.f); C.Box(P.X - 0.6f, P.Y - 0.6f, 1.2f, 1.2f, FLinearColor(Col.R, Col.G, Col.B, 0.25f)); return; }
	for (int32 K = 0; K < 3; ++K) C.Lines(FCanvas::CirclePoints(P, R * (1.15f + 0.3f * K), 24), FLinearColor(Col.R, Col.G, Col.B, 0.28f - 0.08f * K), 1.3f); // 둥근 빛: 옅은 고리 셋 (번짐 띠는 작은 원에서 톱니가 됐다)
	for (float Rr = R * 0.75f; Rr > 0.4f; Rr -= 0.9f) C.Lines(FCanvas::CirclePoints(P, Rr, 16), Col, 1.2f); // 꽉 찬 알
	C.Lines(FCanvas::CirclePoints(P, R * 0.3f, 10), FLinearColor(1.f, 1.f, 1.f, 0.85f), 1.f); // 흰 심
}

// ---- 네온 바 화면 공용 (D-293)
void SPDBoardWidget::BulbSign(FCanvas& C, float CX, float Y, float W, float H, const FString& S, const FLinearColor& Col, float Px, float Time, const TArray<float>* LetterB, bool bWires, float Alpha)
{
	const float X = CX - W / 2.f;
	if (Alpha <= 0.01f) return;
	if (bWires) for (const float Wx : {X + 30.f, X + W - 30.f}) C.Lines({FVector2f(Wx, 0.f), FVector2f(Wx, Y)}, Hex(TEXT("5f6878"), 0.5f * Alpha), 1.f); // 매단 줄
	C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.9f * Alpha));
	C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, RadPanel), WithAlpha(Col, Alpha), 2.4f, Alpha);
	C.Lines(FCanvas::RoundRectPoints(X, Y, W, H, RadPanel), FLinearColor(1.f, 1.f, 1.f, 0.3f * Alpha), 0.6f);
	C.Glow(FCanvas::RoundRectPoints(X + 5.f, Y + 5.f, W - 10.f, H - 10.f, RadSmall), WithAlpha(Col, 0.7f * Alpha), 1.f, 0.4f * Alpha);
	const int32 Ph = FMath::FloorToInt(Time * 8.f) % 3;
	const float X0 = X + 14.f, Y0 = Y + 12.f, X1 = X + W - 14.f, Y1 = Y + H - 12.f, Per = 2.f * ((X1 - X0) + (Y1 - Y0));
	for (int32 Bi = 0; Bi < FMath::FloorToInt(Per / 16.f); ++Bi)
	{
		float D = Bi * 16.f;
		FVector2f Q;
		if (D < X1 - X0) Q = FVector2f(X0 + D, Y0);
		else if ((D -= X1 - X0) < Y1 - Y0) Q = FVector2f(X1, Y0 + D);
		else if ((D -= Y1 - Y0) < X1 - X0) Q = FVector2f(X1 - D, Y1);
		else Q = FVector2f(X0, Y1 - (D - (X1 - X0)));
		Bulb(C, Q, 3.f, (Bi + Ph) % 3 != 0, Hex(TEXT("ffe196"), Alpha)); // 동그란 전구 (D-294)
	}
	const float BY = Y + H / 2.f + Px * 0.36f;
	if (!LetterB) { NeonText(C, S, CX, BY, Col, Px, 1, Alpha); return; }
	// 글자마다 밝기 (꺼져 가는 간판)
	const FSlateFontInfo Fn = FCanvas::Font(Px, true);
	const float Track = Px * 0.15f;
	float TW = 0.f;
	for (int32 I = 0; I < S.Len(); ++I) TW += FCanvas::Measure(S.Mid(I, 1), Fn).X + (I > 0 ? Track : 0.f);
	float LX0 = CX - TW / 2.f;
	for (int32 I = 0; I < S.Len(); ++I)
	{
		const FString Ch = S.Mid(I, 1);
		const float B = (I < LetterB->Num() ? (*LetterB)[I] : 1.f) * Alpha;
		if (Ch != TEXT(" ")) NeonText(C, Ch, LX0, BY, Col, Px, 0, B);
		LX0 += FCanvas::Measure(Ch, Fn).X + Track;
	}
}

void SPDBoardWidget::TubeBtn(FCanvas& C, float X, float Y, float W, float H, const FString& S, bool bOn, const FLinearColor& Col, float Pulse, float Px)
{
	// 튜브 버튼 (D-382): 고르면 분홍으로 바꾸던 것 → 자기 색 그대로 밝아짐 (D-381 버튼 규칙). 평소는 조금 어둡게 두어 고른 것과 차이를 낸다
	const FLinearColor Cc = Col;
	const float B = bOn ? 0.95f + 0.05f * Pulse : 0.65f;
	const TArray<FVector2f> Ro = FCanvas::RoundRectPoints(X, Y, W, H, H / 2.f), Ri = FCanvas::RoundRectPoints(X + 5.f, Y + 5.f, W - 10.f, H - 10.f, (H - 10.f) / 2.f);
	C.Box(X + 6.f, Y + 3.f, W - 12.f, H - 6.f, Hex(TEXT("06060b"), 0.85f));
	C.BtnFill(X, Y, W, H, RadPill(H), Cc, bOn); // 고르면 같은 색으로 (D-382: 예전 분홍 채움)
	C.Glow(Ro, FLinearColor(Cc.R * B, Cc.G * B, Cc.B * B, 1.f), bOn ? 2.6f : 2.1f, 1.f * B);
	C.Lines(Ro, FLinearColor(1.f, 1.f, 1.f, 0.35f * B), 0.6f);
	C.Glow(Ri, FLinearColor(Cc.R * B * 0.8f, Cc.G * B * 0.8f, Cc.B * B * 0.8f, 1.f), 1.2f, 0.5f * B);
	C.TextCentered(S, X + W / 2.f, Y + H / 2.f, bOn ? HotCol(Cc) : WithAlpha(Cc, 0.85f), Px, true, W - 30.f); // 글자도 버튼 색 (D-382)
}

float SPDBoardWidget::Receipt(FCanvas& C, float CX, float Y, const TArray<TPair<FString, FString>>& RcRows, const FString& Total, const FString& Sub)
{
	const FLinearColor Lav = Hex(TEXT("a896e1")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Gold = Hex(TEXT("ffd26e"));
	const float W = 300.f, LH = 24.f, H = 70.f + RcRows.Num() * LH + 66.f, X = CX - W / 2.f;
	C.Box(X, Y, W, H, Hex(TEXT("0a0a12"), 0.95f));
	TArray<FVector2f> Pts = {FVector2f(X, Y + H), FVector2f(X, Y), FVector2f(X + W, Y), FVector2f(X + W, Y + H)};
	for (int32 K = 0; K <= 25; ++K) Pts.Add(FVector2f(X + W - K * 12.f, Y + H - (K % 2 ? 6.f : 0.f))); // 톱니 끝
	C.Glow(Pts, Lav, 1.5f, 0.45f);
	C.TextCentered(TEXT("BAR ALICE · LATE NIGHT"), CX, Y + 26.f, Fg, 11.f, true);
	C.Lines({FVector2f(X + 16.f, Y + 44.f), FVector2f(X + W - 16.f, Y + 44.f)}, WithAlpha(Dim, 0.8f), 1.f);
	float YY = Y + 70.f;
	const FSlateFontInfo Fn = FCanvas::Font(12.f, false), FnB = FCanvas::Font(12.f, true);
	for (const TPair<FString, FString>& R : RcRows)
	{
		C.Text(R.Key, X + 20.f, YY, Fg, 12.f, 0, false);
		C.Text(R.Value, X + W - 20.f, YY, FLinearColor::White, 12.f, 2, true);
		const float A = X + 26.f + FCanvas::Measure(TrS(R.Key), Fn).X, B = X + W - 26.f - FCanvas::Measure(TrS(R.Value), FnB).X;
		for (float DX = A; DX < B; DX += 6.f) C.Box(DX, YY - 4.f, 1.2f, 1.f, WithAlpha(Dim, 0.7f)); // 점선
		YY += LH;
	}
	C.Lines({FVector2f(X + 16.f, YY - 8.f), FVector2f(X + W - 16.f, YY - 8.f)}, WithAlpha(Dim, 0.8f), 1.f);
	C.Text(TEXT("받은 칩"), X + 20.f, YY + 16.f, Gold, 14.f, 0, true);
	C.Text(Total, X + W - 20.f, YY + 16.f, Gold, 17.f, 2, true);
	C.Text(Sub, X + W - 20.f, YY + 36.f, Dim, 10.f, 2, false);
	return Y + H;
}

// 판 끝 영수증 아래 (D-321 ③ ④): 이번 판으로 이룬 목표(열린 칵테일)와 새로 살 수 있게 된 것 (3개까지). 한 줄씩, 없으면 안 그림
void SPDBoardWidget::PaintRunUnlocks(FCanvas& C, float Y) const
{
	const FLinearColor Mint = Hex(TEXT("8affc4")), Gold = Hex(TEXT("ffd26e"));
	if (NewGoals.Num())
	{
		FString Names;
		for (const int32 I : NewGoals) { if (!Names.IsEmpty()) Names += TEXT(" · "); Names += Tr(CharmName((int32)GoalDef(I).Reward)); }
		C.TextCentered(TrF(TEXT("목표 달성! 칵테일 열림: %s"), *Names), DesignW / 2.f, Y, Mint, 12.f, true, 560.f);
		Y += 18.f;
	}
	if (NowAffordable.Num())
	{
		FString Names;
		for (const FString& S : NowAffordable) { if (!Names.IsEmpty()) Names += TEXT(" · "); Names += Tr(*S); }
		C.TextCentered(TrF(TEXT("이제 살 수 있어요: %s"), *Names), DesignW / 2.f, Y, Gold, 12.f, true, 560.f);
	}
}

// 속 빈 네온 글자 (D-275): 둘레 번짐(옅게 8방향) → 글자색 테두리(8방향 1.3px) → 속은 어두운 색 → 가는 흰 심. Align 0 왼쪽, 1 가운데, 2 오른쪽
void SPDBoardWidget::NeonText(FCanvas& C, const FString& S, float X, float Y, const FLinearColor& Col, float Px, int32 Align, float B)
{
	const FLinearColor CB(Col.R * B, Col.G * B, Col.B * B, 1.f);
	static const float Dx[8] = {1.f, 0.707f, 0.f, -0.707f, -1.f, -0.707f, 0.f, 0.707f}, Dy[8] = {0.f, 0.707f, 1.f, 0.707f, 0.f, -0.707f, -1.f, -0.707f};
	for (int32 K = 0; K < 8; ++K) C.Text(S, X + Dx[K] * 3.5f, Y + Dy[K] * 3.5f, FLinearColor(Col.R, Col.G, Col.B, 0.1f * B), Px, Align, true);
	for (int32 K = 0; K < 8; ++K) C.Text(S, X + Dx[K] * 1.3f, Y + Dy[K] * 1.3f, CB, Px, Align, true);
	C.Text(S, X, Y, Hex(TEXT("0a0a12")), Px, Align, true);
	C.Text(S, X, Y, FLinearColor(1.f, 1.f, 1.f, 0.12f * B), Px, Align, true);
}

// 인게임 바 배경 (D-274, 시안 B): 흐린 벽돌 벽 (판 뒤는 비움) + 바닥선. 손패는 바닥선 아래
void SPDBoardWidget::PaintBarBack(FCanvas& C) const
{
	const FLinearColor Brick = Hex(TEXT("4a3c48"), 0.75f); // D-275: 시안처럼 보이게 조금 진하게
	const float BX0 = FX - 12.f, BX1 = FX + Cols * CS + 12.f, BY0 = FY - 12.f, BY1 = FY + Rows * CS + 12.f;
	for (int32 R = 0; ; ++R)
	{
		const float Y = 20.f + R * 34.f;
		if (Y >= FloorY) break;
		const float YB = FMath::Min(Y + 34.f, FloorY);
		const bool bBoardRow = Y > BY0 && Y < BY1;
		if (bBoardRow) { C.Lines({FVector2f(0.f, Y), FVector2f(BX0, Y)}, Brick, 1.f); C.Lines({FVector2f(BX1, Y), FVector2f(DesignW, Y)}, Brick, 1.f); }
		else C.Lines({FVector2f(0.f, Y), FVector2f(DesignW, Y)}, Brick, 1.f);
		for (int32 K = 0; K < 21; ++K)
		{
			const float X = K * 64.f + (R % 2 ? 32.f : 0.f);
			if (X > BX0 && X < BX1 && YB > BY0 && Y < BY1) continue;
			C.Lines({FVector2f(X, Y), FVector2f(X, YB)}, Brick, 1.f);
		}
	}
	C.Glow({FVector2f(0.f, FloorY), FVector2f(DesignW, FloorY)}, Hex(TEXT("786e8c"), 0.6f), 1.2f, 0.25f);
}

// 작은 바 앨리스 로고 (D-274, D-309): 왼쪽 위 배지. 제목 화면과 같은 선 (PDLogo.h), 깜빡임 없이
void SPDBoardWidget::PaintLogoBadge(FCanvas& C, float CX, float Y, float K) const
{
	const FLinearColor Cyan = Hex(TEXT("6edcff")), Pink = Hex(TEXT("ff6aaa")), Gold = Hex(TEXT("ffd26e"));
	// 시작 화면과 같은 배치 C (사용자 2026-11-16, D-346): Alice 1.25배 가운데, BAR 는 왼쪽 위 0.55배(굵기 · 빛은 그대로, D-345), 마티니 잔은 오른쪽. 위치 수치는 시작 화면 로고(PaintTitle)와 같다 (Y = BAR 윗줄)
	const float BK = 0.55f * K, SK = 1.25f * K, BY0 = Y;
	const float NX0 = CX - 190.f * K;
	// 벽빛 먼저 (D-303, 시작 화면 D-302 와 같게): 튜브 뒤에 은은하게 비치는 빛. 너비는 로고 크기(K)에 맞춘다
	for (int32 Pass = 0; Pass < 2; ++Pass)
	{
		const bool bSpill = Pass == 0;
		auto Tube = [&](const TArray<FVector2f>& Pts, const FLinearColor& Col, float Th, float Wk)
		{
			if (bSpill) { C.Spill(Pts, Col, Wk * K, 0.45f); C.Spill(Pts, Col, Wk * 2.75f * K, 0.27f); }
			else C.Glow(Pts, WithAlpha(Col, 0.85f), Th, 0.4f);
		};
		PDLogo::FPoly Ps[3];
		for (int32 I = 0; I < PDLogo::BlockLetters; ++I)
		{
			const int32 N = PDLogo::BlockPolys(I, Ps);
			for (int32 J = 0; J < N; ++J)
			{
				TArray<FVector2f> Pts;
				for (int32 Q = 0; Q < Ps[J].N; ++Q) Pts.Add(FVector2f(NX0 + (I * PDLogo::BlockAdv + Ps[J].P[Q * 2] * 0.8f) * BK, BY0 + (60.f + Ps[J].P[Q * 2 + 1]) * BK));
				Tube(Pts, Cyan, 1.2f, 25.f);
			}
		}
		const FVector2f PO(CX - PDLogo::ScriptW / 2.f * SK, Y + 156.f * K);
		for (const PDLogo::FRun& Rn : PDLogo::ScriptRuns(nullptr)) Tube(PDLogo::CatmullPts(Rn.Pts, PO, SK, 0.f, 5), Pink, 1.5f, 40.f); // 이어지는 글자는 한 줄로 (D-299)
		// 마티니 잔 (D-309)
		const float SX = CX + 168.f * K, SY = Y + 102.f * K, R = 26.f * K;
		Tube({FVector2f(SX - R, SY - R * 0.9f), FVector2f(SX + R, SY - R * 0.9f), FVector2f(SX, SY + R * 0.15f), FVector2f(SX - R, SY - R * 0.9f)}, Gold, 1.f, 14.f);
		Tube({FVector2f(SX, SY + R * 0.15f), FVector2f(SX, SY + R * 0.95f)}, Gold, 1.f, 14.f);
		Tube({FVector2f(SX - R * 0.5f, SY + R * 0.95f), FVector2f(SX + R * 0.5f, SY + R * 0.95f)}, Gold, 1.f, 14.f);
	}
}

// 레벨업 이펙트 (D-273, 시안 A1 마키 간판): 0~0.3초 경험치 막대 번쩍 → 0.3초 화면 가운데에 전구 두른 금색 간판이 떨어져 튕기고
// 'LEVEL UP' 글자가 하나씩 지직 켜짐 + 빠직 불티 + '레벨 7 → 8' → 1.35초부터 줄어들며 위 레벨 글자 밑으로 날아가 매달린 간판 + '포인트 +N' → 2.4초에 꺼짐
void SPDBoardWidget::PaintLevelUp(FCanvas& C) const
{
	const float T = Clock - LevelUpAt;
	if (T < 0.f || T >= 2.4f || Game.Phase == EPDPhase::Title) return;
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Wire = Hex(TEXT("5f6878"));
	auto Ease = [](float U) { U = FMath::Clamp(U, 0.f, 1.f); return 1.f - (1.f - U) * (1.f - U) * (1.f - U); };
	// 경험치 막대 번쩍 (위쪽 띠의 레벨 막대 자리)
	if (T < 0.45f) { const float Fl = FMath::Max(0.f, 1.f - FMath::Abs(T - 0.25f) / 0.2f); C.RoundFill(LvBarX, LvBarY - LvBarH / 2.f, LvBarW, LvBarH, RadPill(LvBarH), FLinearColor(1.f, 1.f, 1.f, 0.8f * Fl)); } // 계기판 레벨 칸 막대 (D-351)
	static const float Seq[5] = {0.4f, 0.08f, 0.9f, 0.2f, 1.f};
	const FString Word = TEXT("LEVEL UP");
	// 빠직 불티 (타이틀 로고와 같은 움직임)
	auto Crackle = [&](FVector2f O, int32 K, int32 Seed)
	{
		const FLinearColor Hot(1.f, 0.96f, 0.82f, 1.f), Warm(1.f, 0.88f, 0.59f, 1.f);
		if (K == 0) C.Glow(FCanvas::CirclePoints(O, 3.f, 10), Hot, 2.5f, 1.4f);
		FRandomStream Rp(Seed * 104729 + 3);
		for (int32 N = 0; N < 10; ++N)
		{
			const float A = Rp.FRand() < 0.7f ? Rp.FRandRange(-PI * 0.95f, -PI * 0.05f) : Rp.FRandRange(0.f, PI);
			const float Sp = Rp.FRandRange(3.f, 6.5f);
			const int32 Life = Rp.RandRange(7, 12);
			const bool bWhite = Rp.FRand() >= 0.7f;
			if (K < 1 || K > Life) continue;
			auto At = [&](float Tt) { return O + FVector2f(FMath::Cos(A) * Sp * Tt, FMath::Sin(A) * Sp * Tt + 0.35f * Tt * Tt); };
			const float Fd = 1.f - (K / (float)Life) * 0.8f;
			C.Glow({At(K - 1.f), At((float)K)}, WithAlpha(bWhite ? Hot : Warm, Fd), 1.5f, Fd);
		}
	};
	// 가운데 간판 (0.3 ~ 1.75초)
	if (T > 0.3f && T < 1.75f)
	{
		const float U = T - 0.3f;
		float CX = FX + Cols * CS / 2.f, CY = FY + Rows * CS * 0.46f, K = 1.f;
		if (T < 1.35f)
		{
			const float Drop = Ease(U / 0.25f), BU = FMath::Min(1.f, U / 0.45f);
			CY += -220.f * (1.f - Drop) + FMath::Sin(BU * TwoPi) * 10.f * (1.f - BU);
		}
		else
		{
			const float V = Ease((T - 1.35f) / 0.4f);
			CX += (LvSignCX - CX) * V;
			CY += (139.f - CY) * V;
			K = 1.f - 0.55f * V;
		}
		const float W = 380.f * K, H = 150.f * K;
		// BREAK TIME 과 같은 전구 간판 (D-296): 두 겹 틀 · 촘촘히 도는 동그란 전구 · 글자마다 지직 켜지는 속 빈 네온 글자
		TArray<float> LB;
		{
			const int32 Kf = FMath::FloorToInt((T - 0.45f) * 25.f);
			for (int32 I = 0; I < Word.Len(); ++I) { const int32 J = Kf - I; LB.Add(J < 0 ? 0.08f : J < 5 ? Seq[J] : 1.f); }
		}
		BulbSign(C, CX, CY - H / 2.f, W, H, Word, Gold, 50.f * K, Clock, &LB, false);
		if (T < 1.35f)
		{
			const float G2 = Ease((U - 0.5f) / 0.2f);
			if (G2 > 0.f) { C.Box(CX - 90.f, CY + H / 2.f + 8.f, 180.f, 30.f, Hex(TEXT("06060b"), 0.8f * G2)); C.TextCentered(TrF(TEXT("레벨 %d  →  %d"), LevelUpFrom, LevelUpTo), CX, CY + H / 2.f + 24.f, FLinearColor(1.f, 1.f, 1.f, G2), 20.f, true); }
			const int32 KK = FMath::FloorToInt((U - 0.15f) * 25.f) - Word.Len() - 1;
			if (KK >= 0 && KK < 12) { Crackle(FVector2f(CX + 150.f, CY - 55.f), KK, 21); Crackle(FVector2f(CX - 150.f, CY - 55.f), KK, 22); }
		}
	}
	// 위 레벨 글자 밑에 매달린 간판 (1.75 ~ 2.4초) + 포인트 꼬리표
	if (T >= 1.75f)
	{
		const float Fade = T < 2.f ? 1.f : FMath::Max(0.f, 1.f - (T - 2.f) / 0.4f);
		for (const float X : {LvSignCX - 50.f, LvSignCX + 50.f}) C.Lines({FVector2f(X, HudPanY + HudPanH), FVector2f(X, 110.f)}, WithAlpha(Wire, 0.5f * Fade), 1.f); // 계기판 밑에 매달림 (D-274, D-351)
		BulbSign(C, LvSignCX, 110.f, 208.f, 58.f, Word, Gold, 26.f, Clock, nullptr, false, Fade); // BREAK TIME 과 같은 전구 간판 (D-296)
		C.Box(LvSignCX - 60.f, 176.f, 120.f, 24.f, Hex(TEXT("06060b"), 0.82f * Fade));
		C.Glow(FCanvas::RoundRectPoints(LvSignCX - 60.f, 176.f, 120.f, 24.f, RadPill(24.f)), WithAlpha(Pink, Fade), 1.6f, 0.6f * Fade);
		C.TextCentered(TrF(TEXT("포인트 +%d"), LevelUpPts), LvSignCX, 188.f, FLinearColor(1.f, 1.f, 1.f, Fade), 13.f, true);
	}
}

void SPDBoardWidget::PaintHint(FCanvas& C) const
{
	HintOkRect = HintOffRect = HintCardRect = FBox2f(FVector2f(-1.f, -1.f), FVector2f(-1.f, -1.f));
	if (HintNow < 0 || bMenu || bSettings || bTree || bUpg || bCodex || bRanking || Game.Phase == EPDPhase::Title) return;
	static const TCHAR* Title[(int32)EHint::Count] = {TEXT("카드 놓기"), TEXT("족보 만들기"), TEXT("족보 완성!"), TEXT("웨이브 시작"), TEXT("합성"), TEXT("연쇄"), TEXT("조커 만들기"), TEXT("먹이기"), TEXT("손패가 가득"), TEXT("자리바꿈"), TEXT("보스 웨이브"), TEXT("레벨 업"), TEXT("보상 고르기"), TEXT("상위 조커"), TEXT("타워 판매"), TEXT("상위 조커 조건"), TEXT("메뉴"), TEXT("타워 메뉴"), TEXT("도감")};
	static const TCHAR* Body[(int32)EHint::Count][2] = {
		{TEXT("손패 카드를 누르고 판의 빈 칸을 클릭하면"), TEXT("그 카드가 타워가 돼요")},
		{TEXT("카드를 붙여 놓아 포커 족보(원 페어·스트레이트…)를"), TEXT("만들면 타워가 강해져요")},
		{TEXT("붙어 있는 카드가 족보가 됐어요"), TEXT("왼쪽 목록에서 족보마다 효과를 볼 수 있어요")},
		{TEXT("준비가 되면 Space 또는"), TEXT("[웨이브 시작] 버튼을 누르세요")},
		{TEXT("왼쪽 목록의 '합성 ×N' 줄을 누르고"), TEXT("판에서 합쳐질 칸을 클릭해 보세요")},
		{TEXT("족보끼리 맞닿으면 연쇄 보너스로"), TEXT("이어진 족보가 모두 더 강해져요")},
		{TEXT("왼쪽 '조커' 줄을 눌러 만들 조커를 고르고"), TEXT("'추천' 재료를 고르고, 그 재료를 다시 누르면 완성")},
		{TEXT("'먹이기' 줄(또는 판의 조커)에서 조커를 고르고"), TEXT("빛나는 족보 칸을 눌러 먹이면 조커 레벨이 올라요")},
		{TEXT("카드를 오른쪽 클릭해 버리거나"), TEXT("Lv1 타워 위에 놓아 교체하세요")},
		{TEXT("자리바꿈 버튼(X)을 누르고 타워 둘을 고르면"), TEXT("자리를 바꿀 수 있어요")},
		{TEXT("다음 웨이브에 보스가 나와요"), TEXT("채널 TV 다음 화 예고에 보스 이름이 나와요")},
		{TEXT("레벨이 올라 포인트가 생겼어요"), TEXT("스킬 트리(K)에서 노드를 찍어 보세요")},
		{TEXT("칵테일(판 내내 효과)이나 즉시 보상 중 하나를 고르거나"), TEXT("S로 넘기세요. 칵테일 칸은 5개예요")},
		{TEXT("맞닿은 두 조커를 합쳐 상위 조커를 만들 수 있어요"), TEXT("두 조커 사이의 금색 버튼이나 '조커' 목록에서 합쳐요")},
		{TEXT("필요 없는 타워는 클릭 → [판매] 버튼"), TEXT("팔면 경험치를 받아요 (조커는 더 많이)")},
		{TEXT("다른 조커 둘이 맞닿으면 합쳐요 (★ 대표는 둘 다 레벨 10)"), TEXT("상위 조커는 시작 화면 [잠금 해제]에서 열어요")},
		{TEXT("오른쪽 위 톱니바퀴를 누르면 메뉴가 열려요"), TEXT("설정과 도감도 메뉴 안에 있어요 (Esc)")},
		{TEXT("판의 타워를 누르면 할 일 버튼이 떠요"), TEXT("합성 · 조커 · 먹이기 · 판매를 여기서 바로 할 수 있어요")},
		{TEXT("도감이 열렸어요: 메뉴 → 도감 (J)"), TEXT("칵테일 · 조커 · 상위 조커 · 보스를 책처럼 넘겨 봐요")}};
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	const FLinearColor Edge = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("8a94a6"));
	constexpr float TW = 330.f, TH = 104.f;
	FBox2f Tg;
	const bool bTg = HintTarget(HintNow, Tg);
	// 카드 자리: 가리킬 자리의 오른쪽 → 위 → 왼쪽 → 아래 중 화면 안이고 안 겹치는 곳. 없으면 판 위쪽 가운데
	FVector2f P(FX + Cols * CS / 2.f - TW / 2.f, FY + 50.f);
	if (bTg)
	{
		const FVector2f Ctr = Tg.GetCenter();
		const FVector2f Cand[4] = {{Tg.Max.X + 24.f, Ctr.Y - TH / 2.f}, {Ctr.X - TW / 2.f, Tg.Min.Y - 24.f - TH}, {Tg.Min.X - 24.f - TW, Ctr.Y - TH / 2.f}, {Ctr.X - TW / 2.f, Tg.Max.Y + 24.f}};
		for (const FVector2f& Cn : Cand)
		{
			const FVector2f Q(FMath::Clamp(Cn.X, 8.f, DesignW - TW - 8.f), FMath::Clamp(Cn.Y, HudY + HudH + 6.f, DesignH - TH - 8.f));
			if (!FBox2f(Q, Q + FVector2f(TW, TH)).Intersect(Tg)) { P = Q; break; }
		}
		// 고리 + 선
		C.Glow(FCanvas::RoundRectPoints(Tg.Min.X - 4.f, Tg.Min.Y - 4.f, Tg.GetSize().X + 8.f, Tg.GetSize().Y + 8.f, RadSmall), Gold, 2.4f, 1.f + 1.2f * Pulse);
		const FVector2f From(FMath::Clamp(Ctr.X, P.X, P.X + TW), FMath::Clamp(Ctr.Y, P.Y, P.Y + TH));
		const FVector2f To(FMath::Clamp(From.X, Tg.Min.X - 4.f, Tg.Max.X + 4.f), FMath::Clamp(From.Y, Tg.Min.Y - 4.f, Tg.Max.Y + 4.f));
		C.Glow({From, To}, WithAlpha(Gold, 0.9f), 1.8f, 0.8f);
		C.Glow(FCanvas::CirclePoints(To, 3.f + Pulse * 2.f, 12), Gold, 1.6f, 0.8f);
	}
	HintCardRect = FBox2f(P, P + FVector2f(TW, TH));
	C.Box(P.X, P.Y, TW, TH, Hex(TEXT("06060b"), 0.96f));
	C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, TW, TH, RadPanel), Edge, 1.4f, 0.6f + 0.3f * Pulse);
	C.Text(TEXT("도움말"), P.X + 12.f, P.Y + 16.f, Dim, 9.f, 0, true);
	C.Text(Title[HintNow], P.X + 12.f, P.Y + 34.f, Gold, 14.f, 0, true, TW - 24.f);
	C.Text(Body[HintNow][0], P.X + 12.f, P.Y + 54.f, Fg, 11.f, 0, false, TW - 24.f);
	C.Text(Body[HintNow][1], P.X + 12.f, P.Y + 70.f, Fg, 11.f, 0, false, TW - 24.f);
	const FVector2f Ok(P.X + TW - 12.f - 76.f, P.Y + TH - 26.f);
	HintOkRect = FBox2f(Ok, Ok + FVector2f(76.f, 20.f));
	const bool bOkH = HintOkRect.IsInside(Mouse);
	C.Box(Ok.X, Ok.Y, 76.f, 20.f, WithAlpha(Edge, bOkH ? 0.22f : 0.1f));
	C.Glow(FCanvas::RoundRectPoints(Ok.X, Ok.Y, 76.f, 20.f, RadPill(20.f)), Edge, 1.2f, bOkH ? 1.f : 0.5f);
	C.TextCentered(TEXT("알겠어"), Ok.X + 38.f, Ok.Y + 10.f, UiValue, 11.f, true, 72.f);
	const FVector2f Off(P.X + 12.f, P.Y + TH - 24.f);
	HintOffRect = FBox2f(Off, Off + FVector2f(90.f, 16.f));
	C.Text(TEXT("도움말 끄기"), Off.X, Off.Y + 9.f, HintOffRect.IsInside(Mouse) ? Fg : Dim, 9.f, 0, false, 90.f);
}

// 회사 로고 부트 스플래시 (D-385, 시안 tools/neon/logo_jollybaldy_type.py 의 K · k_frame 과 같은 타이밍): 평면 로고라 네온 화풍 예외
// 0.3 눈 뜸 → 0.7 JOLLY BALDY 가 한 글자씩 튀어나와 웃는 입 곡선 → 1.85 깜빡 → 2.1 정수리 광 '띵' → 3.6 부터 사라지며 시작 화면
namespace
{
float SplClamp(float V) { return FMath::Clamp(V, 0.f, 1.f); }
float SplBack(float V, float K = 1.9f) { V -= 1.f; return 1.f + (K + 1.f) * V * V * V + K * V * V; }
float SplOut(float V) { return 1.f - (1.f - V) * (1.f - V) * (1.f - V); }
TArray<FVector2f> SplEllipse(FVector2f C, float Rx, float Ry, float A0 = 0.f, float A1 = TwoPi, int32 N = 32)
{
	TArray<FVector2f> P;
	for (int32 K = 0; K <= N; ++K) { const float A = A0 + (A1 - A0) * K / N; P.Add(C + FVector2f(FMath::Cos(A) * Rx, FMath::Sin(A) * Ry)); }
	return P;
}
TArray<FVector2f> SplStar(FVector2f C, float R, float Rot)
{
	TArray<FVector2f> P;
	for (int32 K = 0; K < 8; ++K) { const float A = FMath::DegreesToRadians(-90.f + K * 45.f + Rot), Rr = K % 2 ? R * 0.25f : R; P.Add(C + FVector2f(FMath::Cos(A), FMath::Sin(A)) * Rr); }
	return P;
}
}
void SPDBoardWidget::PaintSplash(FCanvas& C) const
{
	const float T = SplashT, Fade = 1.f - SplClamp((T - SplashHold) / (SplashLen - SplashHold));
	if (Fade <= 0.f) return;
	const FLinearColor Bg = Hex(TEXT("101018")), Yellow = Hex(TEXT("ffc93c")), Cream = Hex(TEXT("ffecc8")), White = FLinearColor::White;
	C.Box(0.f, 0.f, DesignW, DesignH, WithAlpha(Bg, Fade));
	const float CX = DesignW / 2.f, CY = 40.f, R = 520.f; // 웃는 입 곡선: 가운데가 화면 위 바깥인 큰 원의 아래 호
	// 1) 눈
	const float Eo = SplBack(SplClamp((T - 0.3f) / 0.35f), 1.4f), Blink = SplClamp(1.f - FMath::Abs(T - 1.85f) / 0.1f);
	if (T > 0.3f)
	{
		const float Eh = FMath::Max(0.f, Eo) * (1.f - Blink * 0.85f) * 45.f;
		const FLinearColor Ec = WithAlpha(Cream, SplClamp(Eo) * Fade);
		for (const float Sg : {-1.f, 1.f})
		{
			const FVector2f E(CX + Sg * 150.f, 305.f);
			if (Eh < 4.f) C.Lines({E + FVector2f(-46.f, -2.f), E + FVector2f(46.f, -2.f)}, Ec, 16.f);
			else C.Lines(SplEllipse(E, 50.f, Eh, FMath::DegreesToRadians(200.f), FMath::DegreesToRadians(340.f), 20), Ec, 16.f);
		}
	}
	// 2) 글자: 원의 아래 호를 따라 왼쪽→오른쪽, 글자 위가 원 가운데 쪽
	{
		const FString Word = TEXT("JOLLY BALDY");
		const float Px = 64.f, Gap = Px * 0.04f;
		const FSlateFontInfo F = FCanvas::Font(Px, true);
		TArray<float> Ws;
		float Tot = 0.f;
		for (int32 I = 0; I < Word.Len(); ++I) { Ws.Add(FCanvas::Measure(Word.Mid(I, 1), F).X); Tot += Ws.Last() + (I ? Gap : 0.f); }
		const float H = FCanvas::Measure(TEXT("J"), F).Y;
		float A = HALF_PI + Tot / R / 2.f;
		for (int32 I = 0; I < Word.Len(); ++I)
		{
			const float Am = A - Ws[I] / 2.f / R;
			A -= (Ws[I] + Gap) / R;
			const float Lt = SplClamp((T - 0.7f - I * 0.065f) / 0.32f);
			if (Lt <= 0.f || Word[I] == TEXT(' ')) continue;
			const float Sc = FMath::Max(0.05f, SplBack(Lt)), Rr = R - (1.f - SplOut(Lt)) * 40.f + H * 0.5f * 0.82f; // 글자 가운데 = 호에서 바깥으로 반 높이
			const FVector2f P(CX + FMath::Cos(Am) * Rr, CY + FMath::Sin(Am) * Rr);
			C.TextRot(Word.Mid(I, 1), P.X, P.Y, WithAlpha(Yellow, SplClamp(Lt * 2.5f) * Fade), Px, true, Am - HALF_PI, Sc);
		}
	}
	// 3) 정수리 광: 타원 + 돌며 번쩍하는 네 갈래 별 + 빛줄 넷
	const float Gt = SplClamp((T - 2.1f) / 0.25f);
	if (Gt > 0.f)
	{
		const float Ga = SplOut(Gt);
		C.FillPoly(SplEllipse(FVector2f(CX - 175.f, 136.f), 35.f * Ga, 16.f * Ga), WithAlpha(White, Fade));
		const float St = SplClamp((T - 2.2f) / 0.45f);
		if (St > 0.f)
		{
			const float Big = St < 0.77f ? 1.f + 2.2f * FMath::Sin(PI * FMath::Min(1.f, St * 1.3f)) : 1.f;
			const FVector2f S(CX - 108.f, 118.f);
			C.FillPoly(SplStar(S, 18.f * Big, (1.f - SplOut(St)) * 90.f), WithAlpha(White, Fade), 0.35f);
			if (St < 0.8f)
				for (int32 K = 0; K < 4; ++K)
				{
					const FVector2f D(FMath::Cos(FMath::DegreesToRadians(45.f + K * 90.f)), FMath::Sin(FMath::DegreesToRadians(45.f + K * 90.f)));
					C.Lines({S + D * 26.f, S + D * (26.f + 60.f * SplOut(St / 0.8f))}, WithAlpha(White, (1.f - St / 0.8f) * Fade), 4.f);
				}
		}
	}
}

// 합성 모드 커서 꼬리표 (D-373, 미리보기 tools/neon/rclick_hint_mock.py): 커서 오른쪽 아래 어두운 둥근 상자에 마우스(오른쪽 버튼만 금빛) + "취소".
// 판 위에서만 (오른쪽 클릭 취소는 판 칸에서만 먹는다). 오른쪽 클릭으로 RClickTagUntil 번 꺼 보면 거둔다. 헛클릭하면 버튼이 빨갛게 번쩍 (거둔 뒤에도)
void SPDBoardWidget::PaintFuseCursorTag(FCanvas& C) const
{
	const FPDGame& G = Game;
	const bool bFuseMode = HandFilter >= 0 && !InCraft() && !InFeed() && PickCells.empty() && G.Phase != EPDPhase::Reward && !bSwapMode && Sel < 0;
	if (!bFuseMode || bMenu || bSettings || bTree || bCodex || bUpg || bPaused || CellAt(Mouse) < 0) return;
	const float MissT = Clock - FuseMissAt, Fl = MissT >= 0.f && MissT < 0.6f ? 1.f - MissT / 0.6f : 0.f; // 번쩍 (0.6초 동안 줄어듦)
	if (RClickCancels >= RClickTagUntil && Fl <= 0.f) return;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Red = Hex(TEXT("ff6e78")), Body = Hex(TEXT("8a94a6"));
	const FLinearColor Col = Fl > 0.f ? FMath::Lerp(Gold, Red, FMath::Min(1.f, Fl * 2.f)) : Gold;
	const FString Label = TrS(TEXT("취소"));
	const float Tw = FCanvas::Measure(Label, FCanvas::Font(11.f, true)).X;
	const float IconH = 20.f, BX = Mouse.X + 14.f, BY = Mouse.Y + 18.f, BH = 26.f, BW = 8.f + IconH * 0.66f + 6.f + Tw + 9.f;
	C.RoundFill(BX, BY, BW, BH, 6.f, Hex(TEXT("06060b"), 0.92f));
	C.Glow(FCanvas::RoundRectPoints(BX, BY, BW, BH, 6.f), WithAlpha(Col, 0.45f + 0.45f * Fl), 1.f, 0.4f + 0.6f * Fl);
	const FVector2f IC(BX + 8.f + IconH * 0.33f, BY + BH / 2.f);
	if (UseAtlas())
	{
		C.Sprite(C.NextLayer(), &Atlas->Brushes[AtlasMouse], IC.X, IC.Y, IconH / MouseBakeH, WithAlpha(Body, 0.85f));
		C.Sprite(C.NextLayer(), &Atlas->Brushes[AtlasMouseR], IC.X, IC.Y, IconH / MouseBakeH, Col);
	}
	else // 아틀라스가 없을 때 (F6): 같은 모양을 선으로
	{
		for (const TArray<FVector2f>& L : MouseBodyLines(IC, IconH)) C.Glow(L, WithAlpha(Body, 0.85f), 1.f, 0.5f);
		C.Glow(MouseRightPoints(IC, IconH), Col, 1.4f, 1.f);
	}
	if (Fl > 0.f) // 오른쪽 버튼에서 튀는 빛줄 셋
	{
		const FVector2f O = IC + FVector2f(IconH * 0.18f, -IconH * 0.32f);
		for (const float A : {-75.f, -35.f, 5.f})
		{
			const FVector2f D = Rot2(FVector2f(1.f, 0.f), FMath::DegreesToRadians(A));
			C.Glow({O + D * IconH * 0.45f, O + D * IconH * (0.45f + 0.45f * Fl)}, WithAlpha(Red, Fl), 1.3f, 1.f);
		}
	}
	C.Text(Label, IC.X + IconH * 0.33f + 6.f, BY + BH / 2.f + 5.f, Col, 11.f, 0, true);
}

// 손패 카드 툴팁 (D-177, D-214): 판 위 UI 다음에 그린다 (예전엔 패널과 함께 그려 판 위 딱지·빛이 툴팁 위로 비쳤다)
void SPDBoardWidget::PaintHandTip(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6"));
	// 손패 카드 툴팁 (D-177): 놓으면 될 타워의 공격력·연사·치명·사거리 (지금 부적·트리·강화 반영, 족보 효과 빼고) + 무늬 공격 설명
	for (int32 I = 0; I < (int32)G.Hand.size(); ++I)
	{
		const float X = HandCardX(I, (int32)G.Hand.size());
		if (!InRect(Mouse, X, HandY - 12.f, CardW, CardH + 12.f) || bMenu || bSettings) continue;
		const FPDCard& Card = G.Hand[I];
		FPDTower T;
		T.Card = Card;
		const FPDTowerStats St = TowerStats(T, G.CharmMask(), &G.Mods);
		static const TCHAR* SuitHow[5] = {TEXT("멀리 있는 적도 레이저로 바로 맞힌다"), TEXT("미사일이 터져 둘레 적에게도 피해"), TEXT("탄환을 빠르게 여러 발"), TEXT("서리 구체로 맞은 적을 느리게"), TEXT("아무 숫자·무늬로 쳐서 족보를 이어 준다")};
		const float TW = 260.f, TH = 104.f, TX = FMath::Clamp(X + CardW / 2.f - TW / 2.f, 8.f, DesignW - TW - 8.f), TY = HandY - 18.f - TH;
		const FLinearColor SC = SuitColor(Card.Suit);
		C.Box(TX, TY, TW, TH, Hex(TEXT("06060b"), 0.96f));
		C.Glow(FCanvas::RoundRectPoints(TX, TY, TW, TH, RadSmall), SC, 1.2f, 0.45f);
		C.Text(TrF(TEXT("%s %s"), Card.IsWild() ? Tr(TEXT("와일드")) : *RankText(Card.Rank), SuitName(Card.Suit)), TX + 12.f, TY + 20.f, SC, 13.f, 0, true, TW - 24.f);
		C.Text(SuitHow[FMath::Clamp((int32)Card.Suit, 0, 4)], TX + 12.f, TY + 38.f, Fg, 10.f, 0, false, TW - 24.f);
		C.Text(TrF(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), TX + 12.f, TY + 58.f, UiValue, 11.f, 0, false, TW - 24.f);
		C.Text(TrF(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? Tr(TEXT(" · 둔화")) : TEXT("")), TX + 12.f, TY + 75.f, Grey, 11.f, 0, false, TW - 24.f);
		C.Text(TEXT("클릭(숫자키) → 칸 클릭: 설치 · 오른쪽 클릭: 버리기"), TX + 12.f, TY + 93.f, Dim, 9.f, 0, false, TW - 24.f);
		break;
	}
}

void SPDBoardWidget::PaintHandGuide(FCanvas& C) const
{
	if (HandsFolded()) { PaintHandStrip(C); return; } // 접힌 띠 (D-421)
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Dim = Hex(TEXT("5f6878")), Grey = Hex(TEXT("8a94a6")), Fg = Hex(TEXT("cfd8e6"));
	TSet<int32> PvKinds;
	int32 Count[(int)EPDHand::Count] = {};
	for (const FPDSynergy& S : Preview) PvKinds.Add((int32)S.Kind);
	for (const FPDSynergy& S : G.Synergies) ++Count[(int)S.Kind];
	const float PH = DesignH - 1.f - HandPanelY; // 화면 아래까지 (Alice 시안 D-403: 10 ~ 709)
	const bool bMenuBoard = !InFeed() && !InCraft(); // 족보 메뉴판 (D-263): 조커 · 먹이기 목록일 때는 예전 머리글
	// Alice 시안 (D-403): 보랏빛 세로 그라데이션 판 + 연보라 테, 맨 위 BAR Alice 로고(시안 그림 조각) + 금색 '족보 메뉴'
	for (int32 K = 0; K < 14; ++K) C.Box(LX + 2.f, HandPanelY + 2.f + K * (PH - 4.f) / 14.f, LW - 4.f, (PH - 4.f) / 14.f + 0.5f, SrgbLerp(FColor(16, 10, 32), FColor(11, 7, 26), K / 13.f)); // 시안 패널 안쪽에서 잰 sRGB 10 0a 20 → 0b 07 1a (D-405)
	// 판 테 (D-460: 시안에서 잰 색): 차분한 연보라 심 6f5788 + 양쪽으로 넓게 번지는 보랏빛 (시안은 안쪽도 10 쯤 은은하게 물든다)
	C.Glow(FCanvas::RoundRectPoints(LX, HandPanelY, LW, PH, RadPanel), Hex(TEXT("7854a8")), 5.f, 1.f);
	C.Lines(FCanvas::RoundRectPoints(LX, HandPanelY, LW, PH, RadPanel), Hex(TEXT("78609a")), 1.1f);
	{ // 맨 위 BAR Alice 로고 (D-453, 사용자): 시안 그림 조각 대신 시작 화면과 같은 간판(따라 그린 획)을 작게. 예전 그림 칸(20.5, 17.3, 94.7 × 36.5) 가운데에 높이 맞춤
		static const float AllOn[PDLogo::ScriptLetters > PDLogo::BlockLetters ? PDLogo::ScriptLetters : PDLogo::BlockLetters] = {1.f, 1.f, 1.f, 1.f, 1.f};
		constexpr float LogoH = 40.f, LogoW = LogoH * PDTitleLogo::SrcW / PDTitleLogo::SrcH;
		C.NextLayer();
		PaintAliceSign(C, 20.5f + 94.7f / 2.f - LogoW / 2.f, 17.3f + 36.5f / 2.f - LogoH / 2.f, LogoW, AllOn, AllOn, 1.f, 0.f, -8.f); // 예전 로고처럼 오른쪽이 올라가게 살짝 기울임 (D-454)
	}
	if (bMenuBoard)
	{
		const FLinearColor TitleGold = Hex(TEXT("e4de94")); // 시안: 연한 금빛 (D-409, D-460 다시 잼)
		C.SoftDisc(FVector2f(LX + LW / 2.f + 4.f, 66.f), 70.f, 18.f, TitleGold, 0.12f);
		C.TextCentered(TEXT("족보 메뉴"), LX + LW / 2.f + 4.f, 66.f, TitleGold, 24.f, true);
	}
	else
	{
		C.Text(TEXT("족보"), LX + 14.f, FY + 24.f, Cyan, 15.f, 0, true);
		C.Text(HandFilter >= 0 ? TEXT("다시 클릭 / Esc: 끄기") : TEXT("클릭: 그 족보 합성"), LX + LW - 14.f, FY + 24.f, HandFilter >= 0 ? Gold : Dim, 9.f, 2, false, LW - 70.f);
	}
	int32 FuseN[(int)EPDHand::Count] = {};
	for (int32 I = 0; I < (int32)G.Synergies.size(); ++I) if (G.CanFuse(I)) ++FuseN[(int)G.Synergies[I].Kind];
	const float Fl = FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f));
	constexpr float RowH = HandRowH;
	const float Y0 = HandRowY0;
	if (InFeed()) // 먹이기 목록 (D-213): 돌아가기 줄 + 판 위 조커 (먹일 게 있는 조커 먼저, 레벨 높은 순)
	{
		const int32 HovR = FeedListAt(Mouse);
		{
			const bool bH = HovR == -2;
			if (bH) C.Box(LX + 8.f, Y0 + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(Cyan, 0.08f));
			C.Text(TEXT("← 족보 목록으로"), LX + 18.f, Y0 + RowH / 2.f, bH ? Cyan : Grey, 12.f, 0, true, LW - 70.f);
			C.Text(TEXT("Esc"), LX + LW - 18.f, Y0 + RowH / 2.f, Dim, 9.f, 2);
		}
		if (FeedList.empty()) C.Text(TEXT("판에 조커가 없다"), LX + 18.f, Y0 + RowH * 1.5f, Dim, 11.f, 0, false, LW - 36.f);
		const float JFl = FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f));
		for (int32 R = 0; R < (int32)FeedList.size(); ++R)
		{
			const FFeedEntry& E = FeedList[R];
			if (!G.Towers.count(E.Key)) continue;
			const FPDTower& T = G.Towers.at(E.Key);
			const float Y = Y0 + (R + 1) * RowH;
			const bool bSel = E.Key == FeedJoker, bH = HovR == R, bOn = E.N > 0;
			if (R > 0 && FeedList[R - 1].N > 0 && !bOn) C.Lines({FVector2f(LX + 18.f, Y + 1.f), FVector2f(LX + LW - 18.f, Y + 1.f)}, WithAlpha(Grey, 0.35f), 1.f); // 먹일 게 있는 / 없는 조커 구분선 (D-328 ④)
			if (bSel) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(JokerPink, 0.18f));
			else if (bOn) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(JokerPink, bH ? 0.14f : 0.03f + 0.08f * JFl));
			if (bSel || bH) C.Glow(FCanvas::RoundRectPoints(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, RadSmall), JokerPink, 1.6f, bSel ? 1.2f : 0.8f);
			C.Text(TrF(TEXT("%s Lv%d"), JokerName(T.Joker), T.JLevel), LX + 18.f, Y + 16.f, bOn ? JokerPink : WithAlpha(Grey, 0.85f), 12.f, 0, true, LW - 100.f);
			if (bOn)
			{
				C.Glow(FCanvas::RoundRectPoints(LX + LW - 62.f, Y + 6.f, 48.f, 16.f, RadPill(16.f)), JokerPink, 1.1f, 0.6f);
				C.TextCentered(TrF(TEXT("먹이 ×%d"), E.N), LX + LW - 38.f, Y + 14.f, JokerPink, 9.f, true, 46.f);
			}
			// 경험치 막대
			const float Need = JokerXpNeed(T.JLevel), U = T.JLevel >= JokerMaxLevel ? 1.f : FMath::Clamp(T.JXp / FMath::Max(1.f, Need), 0.f, 1.f);
			const float BX = LX + 18.f, BY = Y + 28.f, BW = 90.f;
			C.Box(BX, BY, BW, 4.f, Hex(TEXT("1a1a2a"), 0.9f));
			C.Box(BX, BY, BW * U, 4.f, WithAlpha(JokerPink, bOn ? 0.9f : 0.5f));
			C.Text(TrF(TEXT("경험치 %.0f/%.0f"), T.JXp, Need), BX + BW + 8.f, Y + 31.f, bOn ? Fg : Dim, 9.f, 0, false, LW - BW - 50.f);
		}
	}
	else if (InCraft()) // 조커 목록 (D-210): 족보 목록 자리에 돌아가기 줄 + 만들 수 있는 조커 + 하나만 모자란 조커(흐리게)
	{
		const int32 HovR = JokerListAt(Mouse);
		{
			const bool bH = HovR == -2;
			if (bH) C.Box(LX + 8.f, Y0 + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(Cyan, 0.08f));
			C.Text(TEXT("← 족보 목록으로"), LX + 18.f, Y0 + RowH / 2.f, bH ? Cyan : Grey, 12.f, 0, true, LW - 70.f);
			C.Text(TEXT("Esc"), LX + LW - 18.f, Y0 + RowH / 2.f, Dim, 9.f, 2);
		}
		if (JokerList.empty()) C.Text(TEXT("만들 수 있는 조커가 없다"), LX + 18.f, Y0 + RowH * 1.5f, Dim, 11.f, 0, false, LW - 36.f);
		const float JFl = FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f));
		for (int32 R = 0; R < (int32)JokerList.size(); ++R)
		{
			const FJokerEntry& E = JokerList[R];
			const float Y = Y0 + (R + 1) * RowH;
			if (R > 0 && (JokerList[R - 1].bMake != E.bMake || (JokerList[R - 1].Upper >= 0) != (E.Upper >= 0))) C.Lines({FVector2f(LX + 18.f, Y + 1.f), FVector2f(LX + LW - 18.f, Y + 1.f)}, WithAlpha(Grey, 0.35f), 1.f); // 무리(만들 수 있음 · 상위 · 하나 모자람) 사이 구분선 (D-328 ④)
			if (E.Upper >= 0) // 상위 조커 줄 (D-237): 금색, 재료 조커 둘 (못 만들면 레벨 진행 / 맞붙이기)
			{
				const bool bUS = E.Upper == CraftUpper, bUH = HovR == R;
				if (bUS) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(Gold, 0.16f));
				else if (E.bMake) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(Gold, bUH ? 0.12f : 0.03f + 0.07f * JFl));
				if (bUS || (E.bMake && bUH)) C.Glow(FCanvas::RoundRectPoints(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, RadSmall), Gold, 1.6f, bUS ? 1.2f : 0.8f);
				const bool bUK = (G.UpperFound >> E.Upper) & 1u;
				C.Text(bUK ? TrF(TEXT("상위: %s"), UpperName(E.Upper)) : FString(Tr(TEXT("상위 조커 ???"))), LX + 18.f, Y + 16.f, E.bMake ? Gold : WithAlpha(Grey, 0.8f), 12.f, 0, true, LW - 70.f);
				C.Text(TEXT("상위"), LX + LW - 18.f, Y + 16.f, E.bMake ? Gold : Dim, 10.f, 2, true);
				const FPDUpperDef& D = UpperDef(E.Upper);
				const FString UA = JokerName((int32)D.A), UB = JokerName((int32)D.B);
				const FString USub = E.bMake ? FString::Printf(TEXT("%s + %s"), *UA, *UB)
					: E.bApart ? TrF(TEXT("%s + %s · 맞붙여 놓아야 해요"), *UA, *UB)
					: FString::Printf(TEXT("%s Lv%d/%d · %s Lv%d/%d"), *UA, E.LvA, E.NeedA, *UB, E.LvB, E.NeedB);
				C.Text(USub, LX + 18.f, Y + 31.f, E.bMake ? Fg : Dim, 9.f, 0, false, LW - 36.f);
				continue;
			}
			const bool bSel = CraftJoker >= 0 && E.J == CraftJoker, bH = HovR == R;
			const bool bKnown = (G.Discovered >> E.J) & 1u;
			if (bSel) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(JokerPink, 0.18f));
			else if (E.bMake) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(JokerPink, (bH ? 0.14f : 0.03f + 0.08f * JFl)));
			if (bSel || (E.bMake && bH)) C.Glow(FCanvas::RoundRectPoints(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, RadSmall), JokerPink, 1.6f, bSel ? 1.2f : 0.8f);
			const FLinearColor NameCol = E.bMake ? JokerPink : WithAlpha(Grey, 0.8f);
			C.Text(bKnown ? FString(JokerName(E.J)) : FString(TEXT("???")), LX + 18.f, Y + 16.f, NameCol, 12.f, 0, true, LW - 90.f);
			C.Text(FString::Printf(TEXT("×%s"), *FString::SanitizeFloat(JokerTier((EPDJoker)E.J))), LX + LW - 18.f, Y + 16.f, E.bMake ? Gold : Dim, 10.f, 2, true);
			FString Sub;
			if (E.bMake)
			{
				const std::vector<FPDJokerNode> Ns = G.JokerNodes();
				for (const std::vector<int>& Pk : E.Best)
					for (const FPDJokerNode& N : Ns)
					{
						std::vector<int> A = N.Cells, B = Pk; std::sort(A.begin(), A.end()); std::sort(B.begin(), B.end());
						if (A == B) { if (!Sub.IsEmpty()) Sub += TEXT(" · "); Sub += HandName(N.Kind); break; }
					}
			}
			else Sub = TrF(TEXT("%s 하나만 더 있으면"), HandName(E.Missing));
			if (bSel) // 고른 조커 (D-212): 레시피 족보마다 고른 재료가 있으면 초록 체크 + 초록 글자
			{
				const std::vector<FPDJokerNode> Ns = G.JokerNodes();
				std::vector<int> Have;
				for (int N : CraftNodeIdx()) Have.push_back((int)Ns[N].Kind);
				const FSlateFontInfo Fn = FCanvas::Font(9.f, false);
				const FLinearColor Ok = Hex(TEXT("9cffb0"));
				float X = LX + 18.f;
				const float Ty = Y + 31.f;
				int32 Slot = 0;
				// 3개 · 2개 레시피 중 고른 재료가 더 많이 맞는 쪽을 보여 준다 (D-370): 2개 레시피로 완성됐는데 3개 레시피에 빈 칸이 보여 헷갈리던 것
				auto Matched = [&Have](auto const& Rec) { std::vector<int> Hv = Have; int32 M = 0; for (EPDHand H : Rec) { auto Mi = std::find(Hv.begin(), Hv.end(), (int)H); if (Mi != Hv.end()) { Hv.erase(Mi); ++M; } } return M; };
				const auto& R3 = JokerRecipe((EPDJoker)E.J);
				const auto& R2 = JokerRecipe2((EPDJoker)E.J);
				const int32 M3 = Matched(R3), M2 = Matched(R2);
				const bool bUse2 = M3 < 3 && (M2 == 2 || M2 > M3);
				std::vector<EPDHand> Rec = bUse2 ? std::vector<EPDHand>(R2.begin(), R2.end()) : std::vector<EPDHand>(R3.begin(), R3.end());
				if (bUse2) { C.Text(TEXT("2개"), X, Ty, Dim, 9.f); X += FCanvas::Measure(TrS(TEXT("2개")), Fn).X + 5.f; } // 레시피 창의 '2개' 줄 머리와 같은 글자
				for (EPDHand H : Rec)
				{
					if (Slot++ > 0) { C.Text(TEXT(" · "), X, Ty, Dim, 9.f); X += FCanvas::Measure(TEXT(" · "), Fn).X; }
					auto It = std::find(Have.begin(), Have.end(), (int)H);
					const bool bOk = It != Have.end();
					if (bOk)
					{
						Have.erase(It);
						C.Lines({FVector2f(X, Ty), FVector2f(X + 3.f, Ty + 3.f), FVector2f(X + 8.f, Ty - 4.f)}, Ok, 1.6f); // 체크 표시
						X += 11.f;
					}
					const FString Nm = HandName(H);
					C.Text(Nm, X, Ty, bOk ? Ok : Fg, 9.f, 0, bOk);
					X += FCanvas::Measure(Nm, FCanvas::Font(9.f, bOk)).X;
				}
			}
			else C.Text(Sub, LX + 18.f, Y + 31.f, E.bMake ? Fg : Dim, 9.f, 0, false, LW - 36.f);
		}
	}
	else
	{
		// 족보 메뉴판 (D-263, 레터보드): 묶음 머리글, 줄 = [색 칸] 이름 [판 N 꼬리표] ····· ×배수, 아래 작은 설명, 합성 · 먹이는 추천 꼬리표
		const float X0 = LX + 18.f, XR = LX + LW - 18.f;
		const int32 HovId = HandMenuAt(Mouse.X, Mouse.Y);
		const FSlateFontInfo TagFn = FCanvas::Font(9.f, true);
		static const TCHAR* GroupName[3] = {TEXT("5장"), TEXT("2~4장"), TEXT("조커/먹이")};
		for (int32 Gp = 0; Gp < 3; ++Gp)
		{
			const int32 FirstSlot = Gp == 0 ? 0 : Gp == 1 ? 6 : 10;
			const float GY = HandMenuTop(HandMenuOrder[FirstSlot]) - 19.f; // 머리글 104 · 374 · 568 (Alice 시안 D-403)
			const FString GN = Tr(GroupName[Gp]);
			const float GW = FCanvas::Measure(GN, FCanvas::Font(13.f, false)).X, CX = LX + LW / 2.f + 1.f;
			C.TextCentered(GN, CX, GY, Hex(TEXT("dddbca")), 13.f, false); // 시안: 크림색 보통 굵기 (D-409)
			for (const float Sg : {-1.f, 1.f}) // 양옆 선 (D-460): 칙칙한 올리브 심 + 은은한 번짐
			{
				const TArray<FVector2f> GL = {FVector2f(CX + Sg * (GW / 2.f + 10.f), GY), FVector2f(CX + Sg * 100.f, GY)};
				C.Glow(GL, Hex(TEXT("6a5f40")), 3.2f, 1.f);
				C.Lines(GL, Hex(TEXT("534e3e")), 1.2f);
			}
		}
		for (int32 Slot = 0; Slot < HandRowsN; ++Slot)
		{
			const int32 Id = HandMenuOrder[Slot];
			const float Y = HandMenuTop(Id);
			const bool bHand = Id < (int32)EPDHand::Count;
			const EPDHand H = bHand ? (EPDHand)Id : EPDHand::Pair;
			const int32 NJ = (int32)JGroups.size(), NF = Id == FeedFilterId ? (int32)G.FeedTargets().size() : 0;
			const int32 Cnt = bHand ? Count[Id] : Id == JokerFilterId ? NJ : NF;
			const int32 Fz = bHand ? FuseN[Id] : 0;
			const bool bPv = bHand && PvKinds.Contains(Id), bOn = Cnt > 0, bHov = HovId == Id, bFilt = HandFilter == Id;
			const FLinearColor HC = bHand ? HandColor(H) : JokerPink;
			const float NY = Y + 13.f, DY = Y + 25.f; // 줄 상자 31 (Alice 시안 D-403)
			// 줄 바탕 (Alice 시안 D-403): 어두운 상자 + 회색 테, 왼쪽 족보 색 막대. 마우스 · 미리보기 = 금빛 채움 + 노란 테, 필터 = 족보 색 테, 합성 맥박 = 옅은 금빛
			const float RX0 = LX + 12.f, RW = LW - 22.f, RH = HandMenuRowH - 6.4f;
			const TArray<FVector2f> RB = FCanvas::RoundRectPoints(RX0, Y, RW, RH, 6.f);
			const bool bGoldRow = bHov || bPv;
			C.FillPoly(RB, bGoldRow ? Hex(TEXT("555245")) : Hex(TEXT("0d081c"))); // 시안에서 잰 색 (D-409, D-460): 선택 = 칙칙한 올리브, 보통 = 거의 검은 남보라
			if (Fz > 0 && HandFilter < 0 && !bGoldRow) C.FillPoly(RB, WithAlpha(Gold, 0.03f + 0.08f * Fl));
			if (Id == FeedFilterId && bOn && !bGoldRow) C.FillPoly(RB, WithAlpha(JokerPink, 0.03f + 0.08f * Fl));
			if (bGoldRow) { C.Glow(RB, Hex(TEXT("d7d080")), 1.6f, 0.45f); C.Lines(RB, Hex(TEXT("e6e09a")), 1.2f); } // 연한 노란 테 (얇게)
			else if (bFilt) C.Glow(RB, HC, 1.6f, 0.6f + 0.3f * (0.5f + 0.5f * FMath::Sin(Clock * 6.f)));
			else { C.Glow(RB, Hex(TEXT("4c4756")), 3.2f, 1.f); C.Lines(RB, Hex(TEXT("67626f")), 1.f); } // 회보라 테 + 은은한 네온 번짐 (D-460: 시안 심 504b58 ~ 686373, 번짐 3 쯤)
			C.Box(RX0 + 4.f, Y + 6.f, 3.f, RH - 12.f, SrgbDim(HC, 0.55f)); // 왼쪽 족보 색 막대 (시안: 바랜 색)
			const float NX = X0 + 10.f;
			// 오른쪽 가격 = 배수 (효과 글의 첫 ×), 조커 · 먹이는 개수
			FString Name = bHand ? FString(HandName(H)) : FString(Tr(Id == JokerFilterId ? TEXT("조커") : TEXT("먹이기")));
			if (H == EPDHand::Flush && bHand) Name += Tr(TEXT(" (3~5장)"));
			if (H == EPDHand::Straight && bHand) Name += Tr(TEXT(" (3~5장)")); // D-418: 3장부터
			const FString Eff = bHand ? FString(HandEffect(H)) : FString(Tr(Id == JokerFilterId ? TEXT("클릭: 만들 수 있는 조커 목록") : TEXT("클릭: 조커에 족보 먹이기")));
			FString Price;
			if (!bHand) Price = FString::Printf(TEXT("×%d"), Cnt);
			else { int32 XI = INDEX_NONE; if (Eff.FindChar(TEXT('×'), XI)) { int32 E2 = XI + 1; while (E2 < Eff.Len() && (FChar::IsDigit(Eff[E2]) || Eff[E2] == TEXT('.'))) ++E2; if (E2 > XI + 1) Price = Eff.Mid(XI, E2 - XI); } }
			Price.ReplaceInline(TEXT("×"), TEXT("x")); // 시안: 'x2.5'
			const FLinearColor TextC = bGoldRow ? Hex(TEXT("f0eed2")) : Hex(TEXT("c2bfb4")); // 시안: 이름 · 배수는 차분한 회크림 보통 굵기 (D-409, D-460)
			const float PrW = Price.IsEmpty() ? 0.f : FCanvas::Measure(Price, FCanvas::Font(12.f, false)).X;
			if (!Price.IsEmpty()) C.Text(Price, XR, NY + 1.f, TextC, 12.f, 2, false);
			const float NMax = XR - NX - PrW - 44.f;
			C.Text(Name, NX, NY + 1.f, TextC, 11.5f, 0, false, NMax);
			const float TX = NX + FMath::Min(NMax, FCanvas::Measure(TrS(Name), FCanvas::Font(11.5f, false)).X) + 6.f;
			for (float DX = TX + 2.f; DX < XR - PrW - 6.f; DX += 7.f) C.Box(DX, NY - 2.f, 1.6f, 1.6f, WithAlpha(TextC, 0.7f)); // 점선 리더 (시안: 7 간격 동그란 점)
			float TagW = 0.f;
			if (Fz > 0 || bFilt) // 합성 버튼 (D-328 ⑤): 예전 머리글 '클릭: 그 족보 합성' 대신 줄 오른쪽 알약 버튼. 합성 중이면 '취소'
			{
				const FString S = bFilt ? FString(Tr(TEXT("취소"))) : TrF(TEXT("합성 ×%d"), Fz);
				TagW = FCanvas::Measure(TrS(S), TagFn).X + 22.f;
				const float BX = XR - TagW, BY = DY - 11.f, BH = 16.f;
				const FLinearColor BC = bFilt ? HC : Gold;
				C.BtnFill(BX, BY, TagW, BH, RadPill(BH), BC, bHov || bFilt);
				C.Glow(FCanvas::RoundRectPoints(BX, BY, TagW, BH, RadPill(BH)), BC, bHov ? 1.5f : 1.2f, bHov ? 1.f : 0.5f + 0.3f * Fl);
				C.TextCentered(S, BX + TagW / 2.f, BY + BH / 2.f, bHov ? HotCol(BC) : BC, 9.f, true);
			}
			else if (Id == FeedFilterId && bOn)
			{
				const FString S = TrF(TEXT("먹이 ×%d"), Cnt);
				TagW = FCanvas::Measure(TrS(S), TagFn).X + 22.f;
				const float BX = XR - TagW, BY = DY - 11.f, BH = 16.f;
				C.BtnFill(BX, BY, TagW, BH, RadPill(BH), JokerPink, bHov);
				C.Glow(FCanvas::RoundRectPoints(BX, BY, TagW, BH, RadPill(BH)), JokerPink, bHov ? 1.5f : 1.2f, bHov ? 1.f : 0.5f + 0.3f * Fl);
				C.TextCentered(S, BX + TagW / 2.f, BY + BH / 2.f, bHov ? HotCol(JokerPink) : JokerPink, 9.f, true);
			}
			const FLinearColor DC = bGoldRow ? Hex(TEXT("cac6bb")) : Hex(TEXT("948ea2")); // 설명 (시안에서 잰 색, D-409, D-460)
			FString EffS = TrS(Eff); EffS.ReplaceInline(TEXT("×"), TEXT("x")); EffS.ReplaceInline(TEXT(" · "), TEXT(" / ")); // 시안: 'x2.5 / 연사 x2.5'
			C.Text(EffS, NX, DY, DC, 8.5f, 0, false, XR - NX - (TagW > 0.f ? TagW + 5.f : 0.f));
		}
	}
	if (!bMenuBoard) return; // 조커 · 먹이기 목록은 바닥선까지 쓴다
	{ // 접기 단추 ◂ (D-421): 패널 오른쪽 위
		const bool bHov = InRect(Mouse, FoldBtnX, FoldBtnY, FoldBtnW, FoldBtnH);
		C.FillPoly(FCanvas::RoundRectPoints(FoldBtnX, FoldBtnY, FoldBtnW, FoldBtnH, 7.f), Hex(TEXT("1e182c")));
		C.Glow(FCanvas::RoundRectPoints(FoldBtnX, FoldBtnY, FoldBtnW, FoldBtnH, 7.f), Hex(TEXT("968caa")), 1.f, bHov ? 0.8f : 0.3f);
		const float Cx = FoldBtnX + FoldBtnW / 2.f, Cy = FoldBtnY + FoldBtnH / 2.f;
		C.Lines({FVector2f(Cx + 2.f, Cy - 4.f), FVector2f(Cx - 2.f, Cy), FVector2f(Cx + 2.f, Cy + 4.f)}, bHov ? FLinearColor::White : Hex(TEXT("e4e0cd")), 1.6f);
	}
	const float FootY = 690.f; // 연쇄 한 줄 (Alice 시안 D-403, 무늬 줄은 뺐다: 카드에 공격 방식이 적혀 있다)
	C.Text(TEXT("연쇄"), LX + 16.f, FootY, Hex(TEXT("e4e0cd")), 11.5f, 0, true);
	C.Text(TrF(TEXT("족보끼리 맞닿을 경우, 개당 +%d%%"), G.Has(EPDCharm::ChainLink) ? 45 : 25), LX + 48.f, FootY, Hex(TEXT("a5a0b4")), 9.f, 0, false, LW - 60.f);
}

// 접힌 족보 메뉴 띠 (D-421, 시안 C tools/neon/hands_fold_mock.py): 잔 · 펼치기 단추 · 세로 '족보 메뉴' · 판에 있는 족보 색 점과 개수 · 연쇄. 띠 어디를 눌러도 펼친다
void SPDBoardWidget::PaintHandStrip(FCanvas& C) const
{
	const FPDGame& G = Game;
	const float X = LX, Y = HandPanelY, W = HandStripW, H = DesignH - 1.f - HandPanelY;
	const bool bHov = InRect(Mouse, X, Y, W, H);
	for (int32 K = 0; K < 14; ++K) C.Box(X + 2.f, Y + 2.f + K * (H - 4.f) / 14.f, W - 4.f, (H - 4.f) / 14.f + 0.5f, SrgbLerp(FColor(16, 10, 32), FColor(11, 7, 26), K / 13.f));
	// 판 테 (D-462): 펼친 족보 메뉴판과 같은 연보라 심 + 넓은 보랏빛 번짐 (D-460), 마우스를 올리면 심이 조금 밝아진다
	C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, RadPanel), Hex(TEXT("7854a8")), 5.f, 1.f);
	C.Lines(FCanvas::RoundRectPoints(X, Y, W, H, RadPanel), Hex(bHov ? TEXT("9a80bc") : TEXT("78609a")), 1.1f);
	const float Cx = X + W / 2.f;
	{ // 잔 (로고 대신)
		const FLinearColor Gc = Hex(TEXT("5ae1ff"));
		const float Gy = Y + 26.f;
		C.Glow({FVector2f(Cx - 10.f, Gy - 8.f), FVector2f(Cx + 10.f, Gy - 8.f), FVector2f(Cx, Gy + 4.f), FVector2f(Cx - 10.f, Gy - 8.f)}, Gc, 1.4f, 0.5f);
		C.Glow({FVector2f(Cx, Gy + 4.f), FVector2f(Cx, Gy + 13.f)}, Gc, 1.3f, 0.4f);
		C.Glow({FVector2f(Cx - 6.f, Gy + 13.f), FVector2f(Cx + 6.f, Gy + 13.f)}, Gc, 1.3f, 0.4f);
	}
	{ // 펼치기 단추 ▸
		C.FillPoly(FCanvas::RoundRectPoints(UnfoldBtnX, UnfoldBtnY, UnfoldBtnW, UnfoldBtnH, 8.f), Hex(TEXT("1e182c")));
		C.Glow(FCanvas::RoundRectPoints(UnfoldBtnX, UnfoldBtnY, UnfoldBtnW, UnfoldBtnH, 8.f), Hex(TEXT("968caa")), 1.f, bHov ? 0.8f : 0.35f);
		const float Bx = UnfoldBtnX + UnfoldBtnW / 2.f, By = UnfoldBtnY + UnfoldBtnH / 2.f;
		C.Lines({FVector2f(Bx - 2.f, By - 4.f), FVector2f(Bx + 2.f, By), FVector2f(Bx - 2.f, By + 4.f)}, bHov ? FLinearColor::White : Hex(TEXT("e4e0cd")), 1.6f);
	}
	// 세로 '족보 메뉴': 한국어 · 일본어 · 중국어는 글자를 세로로 쌓고, 영어는 돌려 쓴다
	const FLinearColor Title = Hex(TEXT("e4de94")), TextC = Hex(TEXT("c2bfb4")); // 펼친 판과 같은 색 (D-460, D-462)
	const FString T = TrS(TEXT("족보 메뉴")).Replace(TEXT(" "), TEXT(""));
	float TY = Y + 112.f;
	if (GPDLang == (int32)EPDLang::En) { C.TextRot(TrS(TEXT("족보 메뉴")), Cx, TY + 50.f, Title, 15.f, true, -PI / 2.f); TY += 120.f; }
	else for (int32 I = 0; I < T.Len() && I < 6; ++I) { C.TextCentered(T.Mid(I, 1), Cx, TY, Title, 17.f, true); TY += 26.f; }
	// 판에 있는 족보: 족보 색 고리 + 개수 (조커 포함)
	int32 Count[(int)EPDHand::Count] = {};
	for (const FPDSynergy& S : G.Synergies) ++Count[(int)S.Kind];
	float DY = FMath::Max(TY + 24.f, Y + 250.f);
	for (int32 Slot = 0; Slot < (int32)EPDHand::Count && DY < Y + H - 60.f; ++Slot)
	{
		const int32 Id = HandMenuOrder[Slot];
		if (Id >= (int32)EPDHand::Count || Count[Id] <= 0) continue;
		const FLinearColor Hc = HandColor((EPDHand)Id);
		C.Glow(FCanvas::CirclePoints(FVector2f(Cx, DY), 6.f, 18), Hc, 1.8f, 0.5f);
		C.TextCentered(FString::Printf(TEXT("×%d"), Count[Id]), Cx, DY + 17.f, TextC, 9.f, false);
		DY += 40.f;
	}
	if (!JGroups.empty()) // 만들 수 있는 조커
	{
		C.Glow(FCanvas::CirclePoints(FVector2f(Cx, DY), 6.f, 18), JokerPink, 1.8f, 0.5f + 0.3f * FMath::Square(0.5f + 0.5f * FMath::Sin(Clock * 5.f)));
		C.TextCentered(Tr(TEXT("조커")), Cx, DY + 17.f, JokerPink, 9.f, false, W - 6.f);
	}
	C.TextCentered(Tr(TEXT("연쇄")), Cx, Y + H - 20.f, TextC, 10.f, true, W - 6.f);
}

void SPDBoardWidget::PaintMenu(FCanvas& C) const
{
	// 메뉴 (Esc, D-293): 매단 'BREAK TIME' 전구 간판 + 두 겹 튜브 버튼 (게임 종료만 빨강)
	const FLinearColor Cyan = Hex(TEXT("6edcff")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.82f));
	BulbSign(C, DesignW / 2.f, MenuItemY0 - 150.f, 360.f, 80.f, TEXT("BREAK TIME"), Hex(TEXT("ffa046")), 30.f, Clock);
	C.TextCentered(TEXT("잠깐 쉬어 가자 · 판은 그대로 멈춰 있다"), DesignW / 2.f, MenuItemY0 - 44.f, Fg, 13.f, false);
	const int32 N = (int32)EMenuItem::Count;
	for (int32 I = 0; I < N; ++I)
	{
		const FVector2f B = MenuItemPos(I);
		TubeBtn(C, B.X, B.Y, MenuW, MenuItemH, MenuItemName(I), I == MenuSel, (EMenuItem)I == EMenuItem::Resume ? BtnColPrimary() : (EMenuItem)I == EMenuItem::Quit ? BtnColExit() : BtnColDefault(), Pulse, 17.f); // 버튼 색 규칙 (D-382)
	}
	C.TextCentered(TEXT("↑↓ · Enter 또는 클릭   Esc: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, MenuItemPos(N).Y + 10.f, Dim, 11.f, false);
}

// 설정 창 (D-110): 화면 모드, 월드 이펙트, 흔들림·번쩍임, 소리, 볼륨
void SPDBoardWidget::PaintSettings(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Good = Hex(TEXT("9cffb0")), Grey = Hex(TEXT("8a94a6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 설정 = 칠판 간판 (D-393, 시안 tools/neon/settings_board_mock.py B 에서 다리 · 아래 선 뺌): 위 손잡이 가로대에 전구 줄, 아래 핫핑크 두 겹 테 칠판(아주 어두운 초록 바탕)
	const FLinearColor PinkS = Hex(TEXT("ff6aaa")), LavS = Hex(TEXT("a896e1"));
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.75f));
	{
		const float X0 = SetBoxX, X1 = SetBoxX + SetBoxW, MidX = (X0 + X1) / 2.f, Bot = SetBoxY + SetBoxH;
		const float BarY = SetBoxY - 27.f, BarH = 34.f, BoardY = SetBoxY + 15.f; // 가로대 22 ~ 56, 칠판 64 ~
		// 칠판: 바탕 + 두 겹 테 (핫핑크, D-389)
		C.Box(X0, BoardY, X1 - X0, Bot - BoardY, Hex(TEXT("0a1511"), 0.97f));
		const TArray<FVector2f> Bo = FCanvas::RoundRectPoints(X0, BoardY, X1 - X0, Bot - BoardY, 6.f);
		C.Glow(Bo, PinkS, 2.4f, 0.9f);
		C.Lines(Bo, FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.6f);
		C.Glow(FCanvas::RoundRectPoints(X0 + 10.f, BoardY + 10.f, X1 - X0 - 20.f, Bot - BoardY - 20.f, 3.f), WithAlpha(PinkS, 0.6f), 1.f, 0.35f);
		// 위 가로대: 손잡이 구멍 + 양쪽 전구 줄
		C.Box(X0, BarY, X1 - X0, BarH, Hex(TEXT("06060b"), 0.97f)); // 칠판과 같은 너비 (D-394)
		const TArray<FVector2f> Ba = FCanvas::RoundRectPoints(X0, BarY, X1 - X0, BarH, 10.f);
		C.Glow(Ba, PinkS, 2.2f, 0.9f);
		C.Lines(Ba, FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.6f);
		C.Glow(FCanvas::RoundRectPoints(MidX - 34.f, BarY + 9.f, 68.f, 16.f, 8.f), WithAlpha(PinkS, 0.7f), 1.f, 0.4f); // 손잡이 구멍
		const int32 Ph = FMath::FloorToInt(Clock * 8.f) % 3;
		for (int32 Side = 0; Side < 2; ++Side)
			for (int32 K = 0; K < 9; ++K)
			{
				const float Xa = Side ? MidX + 52.f : X0 + 18.f, Xb = Side ? X1 - 18.f : MidX - 52.f;
				Bulb(C, FVector2f(FMath::Lerp(Xa, Xb, K / 8.f), BarY + BarH / 2.f), 3.f, (K + Side * 9 + Ph) % 3 != 0, Hex(TEXT("ffe196"))); // 동그란 전구 (D-294)
			}
		NeonText(C, Tr(TEXT("환경 설정")), MidX, SetCloseCY + 8.f, Hex(TEXT("ffd26e")), 24.f, 1, 1.f); // 칠판 맨 위 제목: 닫기 ⓧ 와 같은 높이 (D-394, 기준선 = 가운데 + 8)
		// 닫기 ⓧ (D-384): 맨 아래 '닫기' 줄 대신. 동그라미 안 X, 마우스 올림 = 같은 색 밝게
		C.CloseX(FVector2f(SetCloseCX, SetCloseCY), SetCloseR, FVector2f::Distance(Mouse, FVector2f(SetCloseCX, SetCloseCY)) <= SetCloseR + 4.f); // 공통 ⓧ (D-388)
	}
	static const TCHAR* Labels[] = {TEXT("언어"), TEXT("화면"), TEXT("카드 색"), TEXT("프레임 제한"), TEXT("수직 동기화"), TEXT("월드 이펙트 (나이아가라)"), TEXT("화면 흔들림·번쩍임"), TEXT("완성 연출"), TEXT("소리"), TEXT("효과음 볼륨"), TEXT("음악 볼륨"), TEXT("곡 (주크박스)"), TEXT("도움말")};
	auto Button = [&](float X, float Y, float W, const FString& S, const FLinearColor& Col, bool bOn, bool bRowSel) {
		const FLinearColor Cc = bOn ? BtnColDefault() : LavS; // 알약 튜브 (D-293). 켜진 · 고른 것은 기본 하늘색, 꺼진 것은 연보라 (D-382: 분홍은 나가기 · 위험 색이라 바꿈, D-328 ③)
		(void)Col; // 색은 이제 켜짐/꺼짐으로만 (D-328 ③). 카드 색 줄은 아래에서 색 점을 따로 찍는다
		C.BtnFill(X, Y, W, SetBtnH, RadPill(SetBtnH), Cc, bOn);
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, SetBtnH, SetBtnH / 2.f), WithAlpha(Cc, bOn ? 1.f : 0.6f), bOn ? 1.7f : 1.1f, bOn ? (bRowSel ? 1.f + 0.4f * Pulse : 0.8f) : 0.25f);
		if (bOn) C.Lines(FCanvas::RoundRectPoints(X, Y, W, SetBtnH, SetBtnH / 2.f), FLinearColor(1.f, 1.f, 1.f, 0.3f), 0.6f);
		C.TextCentered(S, X + W / 2.f, Y + SetBtnH / 2.f, bOn ? HotCol(Cc) : Grey, 13.f, true, W - 12.f);
	};
	for (int32 Row = 0; Row < (int32)ESetRow::Count; ++Row)
	{
		const float Y = SetRowY(Row);
		const bool bRowSel = Row == SetSel;
		if (bRowSel) C.Box(SetX - 10.f, Y, SetW + 20.f, SetRowH, WithAlpha(BtnColDefault(), 0.06f)); // 고른 줄은 옅은 띠만 (D-386: 하트 뺌)
		C.Text(Labels[Row], SetX, Y + SetRowH / 2.f + 5.f, bRowSel ? HotCol(Fg) : Fg, 14.f, 0, true, SetLabelW - 10.f);
		switch ((ESetRow)Row)
		{
		case ESetRow::Window:
		{
			static const TCHAR* Modes[] = {TEXT("창"), TEXT("전체화면"), TEXT("테두리 없음")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Modes[I], Cyan, Settings.WindowMode == I, bRowSel); }
			break;
		}
		case ESetRow::Lang: // 언어 (D-159): 각 언어 이름은 그 언어로
		{
			for (int32 I = 0; I < (int32)EPDLang::Count; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), LangNativeName(I), Cyan, Settings.Lang == I, bRowSel); }
			break;
		}
		case ESetRow::CardTheme: // 카드 색 (D-154): 버튼 글자도 그 색으로
		{
			static const TCHAR* Themes[] = {TEXT("네온"), TEXT("흑백"), TEXT("빨강·하양")};
			static const TCHAR* ThemeCol[] = {TEXT("8fe9ff"), TEXT("e6ebf5"), TEXT("ff5a6a")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Themes[I], Hex(ThemeCol[I]), Settings.CardTheme == I, bRowSel); C.Glow(FCanvas::CirclePoints(FVector2f(B.X + 14.f, B.Y + SetBtnH / 2.f), 3.5f, 12), Hex(ThemeCol[I]), 1.6f, 0.7f); } // 카드 색은 작은 색 점으로 (D-328 ③)
			break;
		}
		case ESetRow::Frame:
		{
			static const TCHAR* Caps[] = {TEXT("60"), TEXT("120"), TEXT("제한 없음")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Caps[I], Cyan, Settings.FrameCap == I, bRowSel); }
			break;
		}
		case ESetRow::Cine: // 완성 연출 (D-314): 전부 / 조커만 / 끔
		{
			static const TCHAR* Cs[] = {TEXT("전부"), TEXT("조커만"), TEXT("끔")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Cs[I], Cyan, Settings.Cine == I, bRowSel); }
			break;
		}
		case ESetRow::VSync:
		case ESetRow::WorldFx:
		case ESetRow::Shake:
		case ESetRow::Sound:
		{
			const bool bOn = Row == (int32)ESetRow::VSync ? Settings.bVSync : Row == (int32)ESetRow::WorldFx ? Settings.bWorldFx : Row == (int32)ESetRow::Shake ? Settings.bShake : Settings.bSound;
			const FVector2f B = SetBtnPos(Row, 0);
			Button(B.X, B.Y, SetBtnW(Row, 0), bOn ? TEXT("켜짐") : TEXT("꺼짐"), bOn ? Good : Grey, bOn, bRowSel);
			break;
		}
		case ESetRow::Help: // 도움말 (D-217): 켜기·끄기 + 처음부터 다시
		{
			const FVector2f B0 = SetBtnPos(Row, 0), B1 = SetBtnPos(Row, 1);
			Button(B0.X, B0.Y, SetBtnW(Row, 0), Settings.bHints ? TEXT("켜짐") : TEXT("꺼짐"), Settings.bHints ? Good : Grey, Settings.bHints, bRowSel);
			Button(B1.X, B1.Y, SetBtnW(Row, 1), TEXT("처음부터 다시"), Cyan, false, bRowSel);
			break;
		}
		case ESetRow::Volume:
		case ESetRow::Music:
		{
			const FVector2f M = SetBtnPos(Row, 0), Bar = SetBtnPos(Row, 1), Pl = SetBtnPos(Row, 2);
			const float BarW = SetBtnW(Row, 1), V = Settings.bSound ? (Row == (int32)ESetRow::Music ? Settings.MusicVolume : Settings.Volume) : 0.f;
			Button(M.X, M.Y, SetBtnW(Row, 0), TEXT("−"), Cyan, true, bRowSel);
			Button(Pl.X, Pl.Y, SetBtnW(Row, 2), TEXT("+"), Cyan, true, bRowSel);
			C.Box(Bar.X, Bar.Y + 6.f, BarW * V, SetBtnH - 12.f, WithAlpha(Cyan, 0.25f));
			C.Glow(FCanvas::RoundRectPoints(Bar.X, Bar.Y + 6.f, BarW, SetBtnH - 12.f, 4.f), Cyan, 1.f, 0.4f);
			C.TextCentered(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(V * 100.f)), Bar.X + BarW / 2.f, Bar.Y + SetBtnH / 2.f, FLinearColor::White, 13.f, true);
			break;
		}
		case ESetRow::Track: // 주크박스 곡 (D-319): ◀ 곡 이름 ▶, 간판과 같은 목록
		{
			const FVector2f L = SetBtnPos(Row, 0), Nm = SetBtnPos(Row, 1), R = SetBtnPos(Row, 2);
			Button(L.X, L.Y, SetBtnW(Row, 0), TEXT("◀"), Cyan, true, bRowSel);
			Button(R.X, R.Y, SetBtnW(Row, 2), TEXT("▶"), Cyan, true, bRowSel);
			C.TextCentered(FString::Printf(TEXT("%d. %s"), JukeTrack + 1, UTF8_TO_TCHAR(JazzTrackName(JukeTrack))), Nm.X + SetBtnW(Row, 1) / 2.f, Nm.Y + SetBtnH / 2.f, FLinearColor::White, 13.f, true, SetBtnW(Row, 1));
			break;
		}
		default: break;
		}
	}
	// 아래 안내 두 줄(에디터 화면 모드 · 키 조작)은 뺌 (D-383)
}
