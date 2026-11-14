// 포커 디펜스 화면 그리기: 판, 적, 발사체, 이펙트, 버튼, 패널, 창 (화풍 docs/15-art-style.md)
#include "PDBoardDraw.h"
#include "Misc/ScopeExit.h"
#include "Misc/CommandLine.h"

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

// 창 오른쪽 위 닫기 버튼 (도감·트리, D-119)
void SPDBoardWidget::PaintCloseButton(FCanvas& C, const FLinearColor& Col) const
{
	const bool bHov = InRect(Mouse, CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH);
	C.Glow(FCanvas::RoundRectPoints(CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH, 8.f), UiEdge, bHov ? 1.6f : 1.1f, bHov ? 1.f : 0.4f);
	C.TextCentered(bTree ? TEXT("닫기  K") : bUpg ? TEXT("닫기  U") : TEXT("닫기  J"), CdxCloseX + CdxCloseW / 2.f, CdxCloseY + CdxCloseH / 2.f, bHov ? FLinearColor::White : Col, 11.f, true);
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
	GPDBakeWhiteSuits = true; // 카드 칸은 흰색으로 굽고 그릴 때 카드 색 설정에 맞춰 물들인다 (D-154)
	ON_SCOPE_EXIT { GPDBakeWhiteSuits = false; };
	for (int32 Ci = 0; Ci < AtlasCards; ++Ci)
		{
			const FPDCard Card = Ci == 52 ? FPDCard{0, EPDSuit::Wild} : FPDCard{2 + Ci / 4, (EPDSuit)(Ci % 4)};
			for (int32 Lv = 1; Lv <= 4; ++Lv)
				for (int32 St = 0; St < 2; ++St)
				{
					const FVector2f P = AtlasCellCenter(TowerSprite(Card, Lv, St != 0));
					if (!Vis(P)) continue;
					F.CardShape(Card, P.X, P.Y, R, 1.f, St != 0, 0.5f);
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
	F.Glow(FCanvas::RectPoints(FX, FY, Cols * CS, Rows * CS), Hex(TEXT("7fd6ff"), 0.5f), 1.5f, 0.6f);
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
	PaintAll(C, F);
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
	PaintField(F, true);
	PaintField(F, false); Part(0);
	PaintShots(F); Part(1);
	if (!bNoFx) PaintEffects(F);
	Part(2);
	PaintPanels(C); Part(4);
	PaintBoardUi(F); Part(3); // 판 위 UI 를 패널 뒤에 그려 툴팁이 패널 위로 나갈 수 있게 (D-150)
	PaintScreens(C); Part(5);
	PaintTree(C);
	// 화면 번쩍임과 덮개 화면 (제목, 게임 오버), 메뉴
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Red = Hex(TEXT("ff6a6a"));
	if (FlashT > 0.f) C.Box(0.f, 0.f, DesignW, DesignH, WithAlpha(FlashCol, FlashT * 0.45f));
	if (G.Phase == EPDPhase::Title && LoadT <= 0.f) PaintTitle(C);
	else if (G.Phase == EPDPhase::Over)
		C.Overlay(TEXT("GAME OVER"), Red, {TrF(TEXT("웨이브 %d에서 무너졌다 · 처치 %d"), G.Wave, G.Kills), TrF(TEXT("최고 기록 %d웨이브%s"), G.BestWave, G.Wave >= G.BestWave ? Tr(TEXT("  (갱신!)")) : TEXT(""))}, Tr(TEXT("R 키 또는 화면을 눌러 다시")));
	if (bMenu) PaintMenu(C);
	if (bSettings) PaintSettings(C);
	if (LoadT > 0.f) PaintLoading(C);
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
	F.Glow(FCanvas::RectPoints(FX, FY, Cols * CS, Rows * CS), Hex(TEXT("7fd6ff"), 0.5f), 1.5f, 0.6f);
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
				if (bU) Edges.Emplace(FVector2f(X0, Y0), FVector2f(X1, Y0));
				if (bR) Edges.Emplace(FVector2f(X1, Y0), FVector2f(X1, Y1));
				if (bD) Edges.Emplace(FVector2f(X1, Y1), FVector2f(X0, Y1));
				if (bL) Edges.Emplace(FVector2f(X0, Y1), FVector2f(X0, Y0));
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
				if (bAtlas) F.Sprite(SpriteLayer, &Atlas->Brushes[AtlasJoker0 + T.Joker], P.X, P.Y, 1.f, FLinearColor::White);
				else F.JokerTower(T.Joker, P.X, P.Y, CS * 0.4f * Pop, 1.f, Pulse);
			}
			if (bStatic) continue; // 레벨·경험치는 자주 바뀌어 움직이는 층에서
			// 레벨 (위 오른쪽) + 경험치 막대 (아래), 레벨업 순간 막대가 번쩍 (D-125)
			F.Text(FString::Printf(TEXT("Lv%d"), T.JLevel), P.X + CS * 0.5f - 3.f, P.Y - CS * 0.5f + 10.f, JokerPink, 9.f, 2, true, CS - 6.f);
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
			F.Sprite(SpriteLayer, &Atlas->Brushes[TowerSprite(T.Card, T.Level, T.Eff.Hand >= 0)], P.X, P.Y, 1.f, SuitColor(T.Card.Suit));
			if (T.Level >= 2)
				for (int L = 0; L < T.Level; ++L) F.BoxAt(DotLayer, P.X - (T.Level - 1) * 4.f + L * 8.f - 1.5f, P.Y - CS * 0.5f - 1.f, 3.f, 3.f, LevelDotColor());
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
			for (int L = 0; L < T.Level; ++L) F.Box(P.X - (T.Level - 1) * 4.f + L * 8.f - 1.5f, P.Y - CS * 0.5f - 1.f, 3.f, 3.f, LevelDotColor());
	}
	if (bStatic) return; // 아래는 모두 움직이는 층
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
	// 보스 오라 (D-112): 군단장(주변 적 강화)·저주술사(주변 타워 약화) 범위를 옅은 점선으로
	for (const FPDEnemy& E : G.Enemies)
	{
		if (E.Type != EPDEnemy::Warlord && E.Type != EPDEnemy::Hexer) continue;
		const FPDPoint Pt = G.Path.At(E.D);
		const float R = (E.Type == EPDEnemy::Warlord ? PD::Boss::AuraRange : PD::Boss::HexRange) * CS;
		F.Dashed(FCanvas::CirclePoints(CellPx(Pt.X, Pt.Y), R, 48), WithAlpha(EnemyColor(E.Type), 0.35f), 1.2f, 5.f, 7.f);
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
		if (E.SlowT > 0.f) F.Lines(FCanvas::CirclePoints(P, bBoss ? 28.f : 15.f, 24), Hex(TEXT("8affc4"), 0.6f), 1.2f);
		const float W = bBoss ? 44.f : 24.f, BY = P.Y - (bBoss ? 34.f : 22.f);
		F.Box(P.X - W / 2.f, BY, W, 3.f, Hex(TEXT("ffffff"), 0.15f));
		const float HpNow = FMath::Max(0.f, E.Hp / E.MaxHp), HpOld = FMath::Max(HpNow, HpShown.FindRef(E.Id));
		if (HpOld > HpNow) F.Box(P.X - W / 2.f + W * HpNow, BY, W * (HpOld - HpNow), 3.f, Hex(TEXT("ffffff"), 0.85f)); // 방금 깎인 만큼
		F.Box(P.X - W / 2.f, BY, W * HpNow, 3.f, EnemyColor(E.Type));
		if (E.Armor > 0.f && !G.Has(EPDCharm::Pierce)) F.Box(P.X - W / 2.f, BY - 4.f, W, 2.f, Hex(TEXT("9fc3e8")));
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
	// 이름표는 마우스가 올라간 타워의 족보만 (장판 색으로 구분, D-099). 놓기 미리보기는 그대로 보여 줌
	{
		std::vector<FPDSynergy> Hov;
		const int HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		for (const FPDSynergy& S : G.Synergies)
			if (std::find(S.Cells.begin(), S.Cells.end(), HKey) != S.Cells.end() || (HandFilter >= 0 && (int32)S.Kind == HandFilter)) Hov.push_back(S); // 필터로 고른 족보는 이름표도 (D-148)
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
			const bool bTarget = bHoverIn ? K == HoverKey : K == Def; // 지금 클릭하면 모일 칸
			// 모일 칸은 다른 색 (D-149): 하늘색으로 진하게 + 퍼지는 테두리 두 겹 + "여기". 나머지 재료 칸은 금색으로 한 단계 옅게
			const FLinearColor TargetCol = Hex(TEXT("5ff2ff"));
			const FLinearColor Pc = bTarget ? FMath::Lerp(TargetCol, FLinearColor::White, 0.35f * Wv) : FMath::Lerp(PickCol, FLinearColor::White, 0.2f * Wv);
			F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Pc, bTarget ? 0.38f + 0.17f * Wv : 0.1f + 0.08f * Wv));
			if (UseAtlas()) F.Image(F.NextLayer(), &Atlas->HatchBrushes[0], C0.X, C0.Y, CS, CS, WithAlpha(Pc, bTarget ? 0.55f + 0.25f * Wv : 0.3f + 0.15f * Wv));
			F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), Pc, bTarget ? 3.8f : 2.f, bTarget ? 1.8f + 1.f * Wv : 0.7f + 0.4f * Wv);
			if (bTarget)
				for (int32 Ring = 0; Ring < 2; ++Ring)
				{
					const float U = FMath::Fmod(Clock * 1.4f + Ring * 0.5f, 1.f), Gr = 2.f + 12.f * U;
					F.Glow(FCanvas::RoundRectPoints(C0.X - Gr, C0.Y - Gr, CS + Gr * 2.f, CS + Gr * 2.f, 8.f + Gr * 0.5f), WithAlpha(TargetCol, 0.9f * (1.f - U)), 2.2f, 1.f);
				}
			if (bTarget) F.TextCentered(TEXT("여기"), C0.X + CS / 2.f, C0.Y - 10.f, TargetCol, 12.f, true, CS + 20.f);
			if (K == Def) F.TextCentered(TEXT("기본"), C0.X + CS / 2.f, C0.Y + CS - 8.f, WithAlpha(FLinearColor::White, 0.9f), 9.f, true, CS - 4.f);
		}
		// 다른 칸에서 고른 칸으로 빨려 드는 선
		const int32 Target = bHoverIn ? HoverKey : Def;
		for (int K : PickCells)
			if (K != Target) F.Dashed({CellCenter(K), CellCenter(Target)}, WithAlpha(Hex(TEXT("5ff2ff")), 0.7f), 1.6f, 4.f, 5.f);
		// 자리 고르는 중 (D-123): 판 가운데에 큰 글자 (바탕 없이 반투명, 아래 칸이 비쳐 보이게)
		{
			const float MX = FX + Cols * CS / 2.f, MY = FY + Rows * CS / 2.f;
			const FLinearColor Big = WithAlpha(PickCol, 0.55f + 0.3f * Pulse);
			F.TextCentered(bPickJoker ? TEXT("조커 자리를 고르세요") : TEXT("합성 타워 자리를 고르세요"), MX, MY - 14.f, Big, 34.f, true, Cols * CS - 40.f);
			F.TextCentered(TEXT("빛나는 재료 칸 중 하나를 클릭 · 버튼 다시 / G: 기본 칸 · Esc / 오른쪽 클릭: 취소"), MX, MY + 22.f, WithAlpha(Fg, 0.75f), 13.f, false, Cols * CS - 40.f);
		}
	}
	// 족보 필터 (D-148): 합성 못 하는 족보도 합성 표시와 똑같이 칸을 밝힌다 (타워 위에 그려야 보인다). 합성 되는 족보는 아래 합성 표시가 그림
	if (HandFilter >= 0 && !bPv && PickCells.empty() && G.Phase != EPDPhase::Reward && FeedJoker < 0 && !bSwapMode)
	{
		const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
		TSet<int32> Done;
		// 밝힐 칸 묶음: 족보 필터면 그 족보(합성 못 하는 것), 조커 필터면 조커 재료 칸 (분홍, D-152)
		std::vector<std::pair<std::vector<int>, FLinearColor>> Sets;
		if (HandFilter == JokerFilterId) for (const FPDJokerGroup& Gr : JGroups) Sets.push_back({Gr.Cells, JokerPink});
		else
			for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
				if ((int32)G.Synergies[I].Kind == HandFilter && !G.CanFuse(I)) Sets.push_back({G.Synergies[I].Cells, HandColor(G.Synergies[I].Kind)});
		for (const auto& St : Sets)
		{
			const FLinearColor Pick = FMath::Lerp(St.second, FLinearColor::White, 0.6f * Wv);
			for (int K : St.first)
			{
				if (Done.Contains(K)) continue;
				Done.Add(K);
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Pick, 0.22f + 0.22f * Wv));
				if (UseAtlas()) F.Image(F.NextLayer(), &Atlas->HatchBrushes[0], C0.X, C0.Y, CS, CS, WithAlpha(Pick, 0.45f + 0.25f * Wv));
				F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), Pick, 2.8f, 1.4f + 1.4f * Wv);
				F.Glow(FCanvas::RoundRectPoints(C0.X + 6.f, C0.Y + 6.f, CS - 12.f, CS - 12.f, 5.f), WithAlpha(Pick, 0.7f), 1.2f, 0.5f + 0.6f * Wv);
			}
		}
	}
	// 합성 버튼 (먹이기 모드에선 숨김)
	if (!bPv && G.Phase != EPDPhase::Reward && FeedJoker < 0)
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I)
		{
			if (!G.CanFuse(I)) continue;
			if (!SynRevealed(I)) // 평소 (D-113, D-126): 재료 칸 모서리에 금색 꺾쇠가 숨 쉬듯 반짝 (점은 D-161 에 뺐다)
			{
				const float Br = 0.55f + 0.45f * FMath::Sin(Clock * 3.f + I * 0.9f);
				const bool bImg = UseAtlas();
				const int32 BrL = bImg ? F.NextLayer() : 0;
				for (int K : G.Synergies[I].Cells)
				{
					const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
					if (bImg) { F.Image(BrL, &Atlas->BracketBrush, C0.X - BracketPad, C0.Y - BracketPad, CS + BracketPad * 2.f, CS + BracketPad * 2.f, WithAlpha(Gold, 0.55f + 0.45f * Br)); continue; } // 구운 꺾쇠 (D-143)
					const float A = 3.f, B = CS - 3.f, L = 10.f;
					const FLinearColor Cc = WithAlpha(Gold, 0.55f + 0.45f * Br);
					F.Glow({C0 + FVector2f(A, A + L), C0 + FVector2f(A, A), C0 + FVector2f(A + L, A)}, Cc, 1.8f, 0.6f + 0.6f * Br);
					F.Glow({C0 + FVector2f(B - L, A), C0 + FVector2f(B, A), C0 + FVector2f(B, A + L)}, Cc, 1.8f, 0.6f + 0.6f * Br);
					F.Glow({C0 + FVector2f(B, B - L), C0 + FVector2f(B, B), C0 + FVector2f(B - L, B)}, Cc, 1.8f, 0.6f + 0.6f * Br);
					F.Glow({C0 + FVector2f(A + L, B), C0 + FVector2f(A, B), C0 + FVector2f(A, B - L)}, Cc, 1.8f, 0.6f + 0.6f * Br);
				}
				// 아래 변의 금색 점은 뺐다 (D-161): 모서리 꺾쇠만
				continue;
			}
			const FVector2f B = FuseButtonCenter(G.Synergies, I);
			const FPDSynergy FS = FuseSyn(G, I); // 조커를 뺀 실제 합성 칸 (D-128)
			const float BW = FuseBtnW(FS);
			const bool bHov = InBox(Mouse, B, BW, FuseH);
			// 재료 칸 색이 족보 색 → 흰빛 → 족보 색으로 천천히 오간다 (D-122)
			const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
			const FLinearColor Pick = FMath::Lerp(HandColor(FS.Kind), FLinearColor::White, 0.6f * Wv);
			// 합성 대상 표시 (D-121): 재료 칸마다 금색 테두리 + 옅은 채움, 버튼에 올리면 더 밝게
			if (PickCells.empty())
				for (int K : FS.Cells)
				{
					const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
					F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Pick, (bHov ? 0.34f : 0.22f) + 0.22f * Wv));
					// 합성 시점의 빗금 (D-144): 합성될 칸에만
					if (UseAtlas()) F.Image(F.NextLayer(), &Atlas->HatchBrushes[0], C0.X, C0.Y, CS, CS, WithAlpha(Pick, 0.45f + 0.25f * Wv));
					else for (float D = HatchStep; D < CS * 2.f; D += HatchStep)
					{
						const float Ax = FMath::Max(0.f, D - CS), Bx = FMath::Min(CS, D);
						F.Lines({C0 + FVector2f(Ax, D - Ax), C0 + FVector2f(Bx, D - Bx)}, WithAlpha(Pick, 0.45f), 1.f);
					}
					F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), Pick, bHov ? 3.4f : 2.8f, 1.4f + 1.4f * Wv);
					F.Glow(FCanvas::RoundRectPoints(C0.X + 6.f, C0.Y + 6.f, CS - 12.f, CS - 12.f, 5.f), WithAlpha(Pick, 0.7f), 1.2f, 0.5f + 0.6f * Wv);
				}
			BadgeRects.Add(FBox2f(FVector2f(B.X - BW / 2.f, B.Y - FuseH / 2.f), FVector2f(B.X - BW / 2.f + BW, B.Y - FuseH / 2.f + FuseH))); // 툴팁이 피할 자리 (D-150)
			F.Box(B.X - BW / 2.f, B.Y - FuseH / 2.f, BW, FuseH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - BW / 2.f, B.Y - FuseH / 2.f, BW, FuseH, 6.f), Gold, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(FuseLabel(FS), B.X, B.Y, WithAlpha(Gold, bHov ? 1.f : 0.85f), 10.f, true, BW - 4.f);
			if (bHov && PickCells.empty()) F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::Fuse, I), B.X, B.Y - FuseH - 6.f);
		}
	// 먹이기 (D-131): 평소엔 먹일 족보가 있는 조커 위에 작은 "먹이" 표시만. 조커를 클릭하면 먹이기 모드
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty())
	{
		const std::vector<FPDFeed> Feeds = G.FeedTargets();
		if (FeedJoker < 0)
		{
			TMap<int32, int32> PerJoker;
			for (const FPDFeed& Fd : Feeds) PerJoker.FindOrAdd(Fd.Joker)++;
			for (const auto& KV : PerJoker)
			{
				const FVector2f C0 = CellPx((float)(KV.Key % Cols), (float)(KV.Key / Cols));
				const FVector2f Bp = C0 + FVector2f(12.f, 9.f);
				F.Box(Bp.X - 10.f, Bp.Y - 6.f, 20.f, 12.f, Hex(TEXT("06060b"), 0.9f));
				F.Glow(FCanvas::RoundRectPoints(Bp.X - 10.f, Bp.Y - 6.f, 20.f, 12.f, 5.f), JokerPink, 1.f, 0.5f + 0.5f * Pulse);
				F.TextCentered(TrF(TEXT("먹이%d"), KV.Value), Bp.X, Bp.Y, JokerPink, 7.f, true, 19.f);
			}
		}
		else
		{
			// 고른 조커: 분홍 고리가 숨 쉰다
			const FVector2f JC = CellCenter(FeedJoker);
			F.Glow(FCanvas::RoundRectPoints(JC.X - CS * 0.5f - 3.f, JC.Y - CS * 0.5f - 3.f, CS + 6.f, CS + 6.f, 9.f), JokerPink, 2.4f, 1.f + 0.8f * Pulse);
			int32 N = 0;
			const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
			const FLinearColor Pc = FMath::Lerp(JokerPink, FLinearColor::White, 0.55f * Wv);
			for (const FPDFeed& Fd : Feeds)
			{
				if (Fd.Joker != FeedJoker) continue;
				++N;
				const FPDSynergy& FdS = G.Synergies[Fd.Syn];
				const FVector2f B = FeedButtonCenter(G, Fd.Syn);
				const float BW = FeedBtnW(FdS);
				const bool bHov = InBox(Mouse, B, BW, FuseH);
				for (int K : FdS.Cells) // 먹이 칸: 분홍 → 흰빛 → 분홍
				{
					if (G.Towers.at(K).IsJoker()) continue;
					const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
					F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(Pc, (bHov ? 0.3f : 0.18f) + 0.2f * Wv));
					F.Glow(FCanvas::RoundRectPoints(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), Pc, bHov ? 3.f : 2.4f, 1.2f + 1.2f * Wv);
					F.Dashed({CellCenter(K), JC}, WithAlpha(JokerPink, bHov ? 0.8f : 0.4f), 1.3f, 4.f, 5.f);
				}
				if (bHov)
				{
					const FPDTower& Jt = G.Towers.at(FeedJoker);
					int Lv = Jt.JLevel;
					float Xp = Jt.JXp + Fd.Xp;
					while (Lv < JokerMaxLevel && Xp >= JokerXpNeed(Lv)) { Xp -= JokerXpNeed(Lv); ++Lv; }
					F.Badge(TrF(TEXT("경험치 +%.0f → 레벨 %d%s"), Fd.Xp, Lv, Lv > Jt.JLevel ? TEXT("!") : TEXT("")), JC.X, JC.Y - CS * 0.75f, JokerPink, 1.f);
					F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::Feed, Fd.Syn), B.X, B.Y - FuseH - 6.f);
				}
				BadgeRects.Add(FBox2f(FVector2f(B.X - BW / 2.f, B.Y - FuseH / 2.f), FVector2f(B.X - BW / 2.f + BW, B.Y - FuseH / 2.f + FuseH))); // 툴팁이 피할 자리 (D-150)
				F.Box(B.X - BW / 2.f, B.Y - FuseH / 2.f, BW, FuseH, Hex(TEXT("06060b"), 0.92f));
				F.Glow(FCanvas::RoundRectPoints(B.X - BW / 2.f, B.Y - FuseH / 2.f, BW, FuseH, 6.f), JokerPink, bHov ? 1.8f : 1.3f, bHov ? 1.2f + 0.5f * Pulse : 0.7f);
				F.TextCentered(FeedLabel(FdS), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.9f), 10.f, true, BW - 4.f);
			}
			F.Badge(N > 0 ? TrF(TEXT("%s 먹이기: 먹일 족보의 [먹이기] 클릭 · 다른 곳 클릭 / Esc: 취소"), JokerName(G.Towers.at(FeedJoker).Joker))
						  : FString(TEXT("먹일 족보가 없다 (조커에 맞닿은 족보가 먹이) · 다른 곳 클릭 / Esc: 취소")),
				FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
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
				F.Glow(FCanvas::RoundRectPoints(C0.X + 3.f, C0.Y + 3.f, CS - 6.f, CS - 6.f, 6.f), WithAlpha(SwC, KV.first == HoverKey ? 1.f : 0.45f), KV.first == HoverKey ? 2.6f : 1.2f, KV.first == HoverKey ? 1.4f : 0.4f + 0.3f * Wv);
			}
		else
		{
			const FVector2f A = CellPx((float)(SwapFrom % Cols), (float)(SwapFrom / Cols));
			F.Box(A.X + 2.f, A.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(SwC, 0.22f + 0.15f * Wv));
			F.Glow(FCanvas::RoundRectPoints(A.X + 2.f, A.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), SwC, 3.f, 1.4f + 0.8f * Wv);
			if (HoverKey >= 0 && HoverKey != SwapFrom)
			{
				const bool bOk = G.Towers.count(HoverKey) || G.Buildable(HoverX, HoverY);
				const FLinearColor Tc = bOk ? SwC : Hex(TEXT("ff5a6a"));
				const FVector2f B = CellPx((float)HoverX, (float)HoverY);
				F.Glow(FCanvas::RoundRectPoints(B.X + 2.f, B.Y + 2.f, CS - 4.f, CS - 4.f, 7.f), Tc, 2.4f, 1.2f);
				if (bOk) F.Dashed({CellCenter(SwapFrom), CellCenter(HoverKey)}, WithAlpha(SwC, 0.8f), 1.6f, 5.f, 4.f);
			}
		}
		F.Badge(SwapFrom < 0 ? TrF(TEXT("자리바꿈 (%d번 남음): 옮길 타워 클릭 · 오른쪽 클릭 / Esc: 끄기"), G.Swaps)
							 : FString(TEXT("바꿀 자리 클릭 (타워면 서로 바꾸고 빈 칸이면 옮김) · 같은 타워 다시: 다시 고르기")),
			FX + Cols * CS / 2.f, FY + 12.f, SwC, 1.f);
	}
	// 족보 필터 안내 (D-148)
	if (HandFilter >= 0 && FeedJoker < 0 && PickCells.empty() && !bSwapMode)
	{
		if (HandFilter == JokerFilterId) F.Badge(TEXT("조커 만들기만 보기: [조커] 버튼이나 재료 칸 클릭 = 조커 합성 · 족보 목록 다시 클릭 / Esc: 끄기"), FX + Cols * CS / 2.f, FY + 12.f, JokerPink, 1.f);
		else F.Badge(TrF(TEXT("%s만 보기: 그 족보 칸 클릭 = 합성 · 족보 목록 다시 클릭 / Esc: 끄기"), HandName((EPDHand)HandFilter)), FX + Cols * CS / 2.f, FY + 12.f, HandColor((EPDHand)HandFilter), 1.f);
	}
	// 조커 합성 버튼 (연쇄로 이어진 족보 3개 이상, 먹이기 모드에선 숨김)
	if (!bPv && G.Phase != EPDPhase::Reward && FeedJoker < 0)
		for (const FPDJokerGroup& Gr : JGroups)
		{
			// 조커를 만들 수 있는 재료 칸은 늘 반짝인다 (D-115): 칸마다 박자를 어긋나게 한 분홍 테두리 + 작은 별빛
			for (int K : Gr.Cells)
			{
				const float Tw = 0.5f + 0.5f * FMath::Sin(Clock * 5.f + K * 1.7f);
				const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols));
				F.Glow(FCanvas::RoundRectPoints(C0.X + 3.f, C0.Y + 3.f, CS - 6.f, CS - 6.f, 7.f), WithAlpha(JokerPink, 0.35f + 0.55f * Tw), 1.4f, 0.5f + 0.9f * Tw);
				if (Tw > 0.8f) { const FVector2f S = C0 + FVector2f(CS - 9.f, 9.f); const float L = 3.f + 4.f * (Tw - 0.8f) * 5.f;
					F.Lines({S - FVector2f(L, 0.f), S + FVector2f(L, 0.f)}, WithAlpha(FLinearColor::White, Tw), 1.2f); F.Lines({S - FVector2f(0.f, L), S + FVector2f(0.f, L)}, WithAlpha(FLinearColor::White, Tw), 1.2f); }
			}
			const FVector2f B = JokerButtonCenter(Gr);
			if (!JokerRevealed(Gr)) // 평소엔 작은 분홍 마름모 (조커를 만들 수 있다는 표시)
			{
				F.Glow({B + FVector2f(0.f, -5.f), B + FVector2f(4.f, 0.f), B + FVector2f(0.f, 5.f), B + FVector2f(-4.f, 0.f), B + FVector2f(0.f, -5.f)}, JokerPink, 1.6f, 0.8f + 0.5f * Pulse);
				continue;
			}
			const bool bHov = InBox(Mouse, B, JokerBtnW, JokerBtnH);
			const bool bKnown = (G.Discovered >> (int)Gr.Result) & 1u;
			if (bHov) for (int K : Gr.Cells) { const FVector2f C0 = CellPx((float)(K % Cols), (float)(K / Cols)); F.Box(C0.X + 2.f, C0.Y + 2.f, CS - 4.f, CS - 4.f, WithAlpha(JokerPink, 0.12f)); }
			if (bHov && PickCells.empty()) F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::JokerFuse, (int)(&Gr - JGroups.data())), B.X, B.Y - JokerBtnH - 6.f);
			BadgeRects.Add(FBox2f(FVector2f(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f), FVector2f(B.X - JokerBtnW / 2.f + JokerBtnW, B.Y - JokerBtnH / 2.f + JokerBtnH))); // 툴팁이 피할 자리 (D-150)
			F.Box(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, 7.f), JokerPink, bHov ? 1.8f : 1.3f, 1.f + 0.8f * Pulse);
			F.TextCentered(TrF(TEXT("조커: %s"), bKnown ? JokerName((int)Gr.Result) : TEXT("???")), B.X, B.Y, JokerPink, 10.f, true, JokerBtnW - 6.f);
		}
	// 조커 합치기 버튼 (같은 조커끼리, D-114): 평소엔 분홍 점 둘, 마우스를 올리면 버튼
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty() && FeedJoker < 0)
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
			F.Glow(FCanvas::RoundRectPoints(B.X - MergeW / 2.f, B.Y - MergeH / 2.f, MergeW, MergeH, 6.f), JokerPink, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.6f);
			F.TextCentered(TEXT("조커 합치기"), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.85f), 10.f, true, MergeW - 6.f);
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
	// 타워 정보 (마우스를 올리면). 맨 마지막에 그려 이름표·떠오르는 글자·배너가 위에 겹치지 않게 (D-150)
	if (HoverX >= 0 && Sel < 0 && G.Phase != EPDPhase::Reward)
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
				const float G = 8.f, MidY = C0.Y + CS / 2.f - TH / 2.f, MidX = C0.X + CS / 2.f - TW / 2.f;
				const float Far = 20.f; // 이름표 한 줄만큼 더 떨어진 자리
				const FVector2f Cand[12] = {{C0.X + CS + G, MidY}, {C0.X - G - TW, MidY}, {C0.X + CS + G, C0.Y - TH + CS * 0.3f}, {C0.X - G - TW, C0.Y - TH + CS * 0.3f},
					{MidX, C0.Y - G - TH}, {MidX, C0.Y + CS + G}, {C0.X + CS + G, C0.Y + CS * 0.7f}, {C0.X - G - TW, C0.Y + CS * 0.7f},
					{MidX, C0.Y - G - TH - Far}, {MidX, C0.Y + CS + G + Far}, {C0.X + CS + G + Far, MidY}, {C0.X - G - TW - Far, MidY}};
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
			if (T.IsJoker())
			{
				const FVector2f Pj = PlaceTip(246.f, 108.f);
				const float JX = Pj.X, JY = Pj.Y;
				F.Box(JX, JY, 246.f, 108.f, Hex(TEXT("06060b"), 0.94f));
				F.Glow(FCanvas::RoundRectPoints(JX, JY, 246.f, 108.f, 8.f), JokerPink, 1.2f, 0.4f);
				F.Text(TrF(TEXT("%s 대장: %s"), HandName(CaptainHand((EPDJoker)T.Joker)), JokerName(T.Joker)), JX + 10.f, JY + 18.f, JokerPink, 12.f, 0, true, 226.f);
				F.Text(PowLine, JX + 10.f, JY + 36.f, PowCol, 11.f, 0, true, 226.f);
				F.Text(JokerAbility(T.Joker), JX + 10.f, JY + 54.f, Fg, 10.f, 0, false, 226.f);
				F.Text(JokerCommand(T.Joker), JX + 10.f, JY + 70.f, Gold, 10.f, 0, false, 226.f);
				F.Text(TrF(TEXT("클릭: 먹이기 · 처치 +%.0f 경험치 (보스 +%.0f)"), JokerKillXp, JokerBossXp), JX + 10.f, JY + 86.f, Grey, 10.f, 0, false, 226.f);
				F.Text(T.JLevel >= JokerMaxLevel ? TrF(TEXT("레벨 %d (최대) · 위력 +%d%%"), T.JLevel, (T.JLevel - 1) * 10)
					: TrF(TEXT("레벨 %d · 경험치 %.0f/%.0f · 위력 +%d%%%s"), T.JLevel, T.JXp, JokerXpNeed(T.JLevel), (T.JLevel - 1) * 10, T.JLevel >= JokerUpLevel ? Tr(TEXT(" · 강화")) : TEXT("")),
					JX + 10.f, JY + 102.f, JokerPink, 10.f, 0, false, 226.f);
			}
			else
			{
			const float BH = T.Level >= 2 ? 92.f : 76.f;
			const FVector2f P = PlaceTip(240.f, BH);
			const float BX = P.X, BYp = P.Y;
			F.Box(BX, BYp, 240.f, BH, Hex(TEXT("06060b"), 0.94f));
			F.Glow(FCanvas::RoundRectPoints(BX, BYp, 240.f, BH, 8.f), SuitColor(T.Card.Suit), 1.2f, 0.4f);
			F.Text(TrF(TEXT("%s %s · 레벨 %d"), T.Card.IsWild() ? Tr(TEXT("와일드")) : *RankText(T.Card.Rank), SuitName(T.Card.Suit), T.Level), BX + 10.f, BYp + 18.f, SuitColor(T.Card.Suit), 12.f, 0, true, 220.f);
			F.Text(PowLine, BX + 10.f, BYp + 36.f, PowCol, 11.f, 0, true, 220.f);
			F.Text(TrF(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), BX + 10.f, BYp + 54.f, Fg, 11.f, 0, false, 220.f);
			F.Text(TrF(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? Tr(TEXT(" · 둔화")) : TEXT("")), BX + 10.f, BYp + 70.f, Grey, 11.f, 0, false, 220.f);
			if (T.Level >= 2) F.Text(TrF(TEXT("특성 공격 ×%.1f 연사 ×%.1f"), T.TDmg, T.TRate), BX + 10.f, BYp + 86.f, Gold, 10.f, 0, false, 220.f);
			}
		}
	}
}

// 위쪽 상태 띠 (D-119): 웨이브·다음 보스 | 남은 적 | 목숨·골드 | 판 위력 | 속도·설정 버튼
void SPDBoardWidget::PaintHud(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(16.f, HudY, DesignW - 32.f, HudH, Hex(TEXT("06060b"), 0.6f));
	C.Glow(FCanvas::RoundRectPoints(16.f, HudY, DesignW - 32.f, HudH, 10.f), Cyan, 1.f, 0.35f);
	// 웨이브 + 다음 보스
	C.Text(G.Phase == EPDPhase::Title ? FString(TEXT("WAVE -")) : FString::Printf(TEXT("WAVE %d"), G.Wave), 34.f, HudY + 26.f, Hex(TEXT("e8f0ff")), 22.f, 0, true, 150.f);
	{
		const int32 BW = G.NextBossWave();
		const EPDEnemy B = FPDGame::BossOfWave(BW);
		const bool bNow = G.Phase == EPDPhase::Wave && BW == G.Wave;
		C.Text(bNow ? TrF(TEXT("보스 웨이브 · %s"), BossName(B)) : TrF(TEXT("다음 보스  %d웨이브 · %s"), BW, BossName(B)),
			34.f, HudY + 43.f, bNow ? Red : Hex(TEXT("ffa86a")), 10.f, 0, false, 270.f);
	}
	// 남은 적 (웨이브 중) / 자동 시작·기록 (준비 중)
	constexpr float EX = 330.f, EW = 420.f;
	if (G.Phase == EPDPhase::Wave)
	{
		const int32 Left = G.WaveLeft(), Tot = FMath::Max(1, G.WaveTotal);
		C.Text(TrF(TEXT("남은 적 %d / %d"), Left, G.WaveTotal), EX, HudY + 20.f, Fg, 11.f, 0, true, EW * 0.6f);
		C.Text(TrF(TEXT("처치 %d · 놓침 %d"), G.Kills, G.Leaks), EX + EW, HudY + 20.f, Grey, 10.f, 2, false, EW * 0.38f);
		C.Box(EX, HudY + 29.f, EW * FMath::Clamp(Left / (float)Tot, 0.f, 1.f), 9.f, WithAlpha(Cyan, 0.35f));
		C.Glow(FCanvas::RoundRectPoints(EX, HudY + 29.f, EW, 9.f, 4.f), Cyan, 1.f, 0.3f);
	}
	else if (G.Phase != EPDPhase::Title)
	{
		const bool bAuto = G.Phase == EPDPhase::Build && G.AutoStartT >= 0.f;
		C.Text(bAuto ? TrF(TEXT("웨이브 %d 자동 시작까지 %d초"), G.Wave + 1, FMath::CeilToInt(G.AutoStartT)) : TrF(TEXT("웨이브 %d 준비"), G.Wave + 1),
			EX, HudY + 20.f, bAuto ? Green : Fg, 11.f, 0, true, EW * 0.6f);
		C.Text(TrF(TEXT("처치 %d · 놓침 %d · 최고 %d"), G.Kills, G.Leaks, FMath::Max(G.BestWave, G.Wave)), EX + EW, HudY + 20.f, Grey, 10.f, 2, false, EW * 0.38f);
		if (bAuto) C.Box(EX, HudY + 29.f, EW * (1.f - FMath::Clamp(G.AutoStartT / AutoStartDelayFor(G.Wave + 1), 0.f, 1.f)), 9.f, WithAlpha(Green, 0.3f));
		C.Glow(FCanvas::RoundRectPoints(EX, HudY + 29.f, EW, 9.f, 4.f), bAuto ? Green : Dim, 1.f, 0.25f);
	}
	// 목숨·골드
	// 글자 색 통일 (D-163): 값은 흰빛, 목숨은 5 이하일 때만 빨갛게 깜빡. 판 위력은 오른쪽 카드에만 (위쪽 띠에서 뺐다)
	C.Text(TrF(TEXT("목숨 %d"), G.Lives), 860.f, HudY + 27.f, G.Lives <= 5 ? WithAlpha(Red, 0.6f + 0.4f * Pulse) : UiValue, 17.f, 0, true, 120.f);
	C.Text(TrF(TEXT("골드 %s"), *CommaNum(G.Gold)), 990.f, HudY + 27.f, UiValue, 17.f, 0, true, 160.f);
	C.Text(TrF(TEXT("다음 노드 %s"), *CommaNum(G.PointPrice())), 990.f, HudY + 43.f, G.Gold >= G.PointPrice() ? UiTitle : Dim, 9.f, 0, false, 160.f);
	// 속도 (F) · 설정
	{
		const bool bHov = InRect(Mouse, SpeedBtnX, HudBtnY, HudBtnS, HudBtnS);
		C.Glow(FCanvas::RoundRectPoints(SpeedBtnX, HudBtnY, HudBtnS, HudBtnS, 8.f), Cyan, bHov ? 1.6f : 1.1f, bHov ? 1.f : (G.Speed > 1 ? 0.7f : 0.3f));
		C.TextCentered(FString::Printf(TEXT("×%d"), G.Speed), SpeedBtnX + HudBtnS / 2.f, HudBtnY + HudBtnS / 2.f, G.Speed > 1 ? FLinearColor::White : Cyan, G.Speed >= 10 ? 11.f : 13.f, true, HudBtnS - 2.f);
	}
	{
		const bool bHov = InRect(Mouse, SetOpenX, HudBtnY, HudBtnS, HudBtnS);
		C.Glow(FCanvas::RoundRectPoints(SetOpenX, HudBtnY, HudBtnS, HudBtnS, 8.f), UiEdge, bHov ? 1.6f : 1.1f, bHov ? 1.f : 0.3f);
		const PDIcon::FXf P(SetOpenX + HudBtnS / 2.f, HudBtnY + HudBtnS / 2.f, 9.f);
		C.Glow(PDIcon::Burst(P, 8, 1.f, 0.72f, 11.25f), bHov ? Fg : Grey, 1.f, 0.4f); // 톱니바퀴
		C.Glow(PDIcon::Ring(P, 0.f, 0.f, 0.32f, 1.f, 14), bHov ? Fg : Grey, 1.f, 0.4f);
	}
}

// 고정 패널 (D-119): 위쪽 띠, 왼쪽 족보, 오른쪽 위력 카드·버튼·부적, 아래 손패
void SPDBoardWidget::PaintPanels(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0"));
	const FLinearColor Purple = Hex(TEXT("b48cff"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	PaintHud(C);

	// ---- 손패 (크게) + 덱
	C.Text(TrF(TEXT("손패 %d/%d"), (int32)G.Hand.size(), G.HandMax()), FX, HandY + 22.f, Cyan, 13.f, 0, true);
	C.Text(TrF(TEXT("덱 %d장"), (int32)G.Deck.size()), FX, HandY + 42.f, Grey, 11.f);
	if (G.Swaps > 0) // 자리바꿈 버튼 (D-149)
	{
		const FLinearColor SwC = Hex(TEXT("6affc8"));
		const bool bHov = InRect(Mouse, SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH);
		if (bSwapMode) C.Box(SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH, WithAlpha(SwC, 0.18f));
		C.Glow(FCanvas::RoundRectPoints(SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH, 7.f), UiEdge, bHov || bSwapMode ? 1.7f : 1.2f, bSwapMode ? 1.2f + 0.5f * Pulse : bHov ? 1.f : 0.5f);
		C.TextCentered(TrF(TEXT("자리바꿈 ×%d"), G.Swaps), SwapBtnX + SwapBtnW / 2.f, SwapBtnY + 11.f, SwC, 11.f, true, SwapBtnW - 6.f);
		C.TextCentered(bSwapMode ? TEXT("끄기 X") : TEXT("X"), SwapBtnX + SwapBtnW / 2.f, SwapBtnY + 23.f, WithAlpha(SwC, 0.7f), 8.f, false, SwapBtnW - 6.f);
	}
	for (int32 I = 0; I < (int32)G.Hand.size(); ++I)
	{
		const FPDCard& Card = G.Hand[I];
		const bool bSel = I == Sel;
		const float X = HandCardX(I, (int32)G.Hand.size()), Y = HandY - (bSel ? 10.f : 0.f);
		const FLinearColor Col = SuitColor(Card.Suit);
		C.Box(X, Y, CardW, CardH, Hex(TEXT("06060b"), 0.7f));
		C.Glow(FCanvas::RoundRectPoints(X, Y, CardW, CardH, 10.f), bSel ? FLinearColor::White : Col, bSel ? 2.4f : 1.4f, bSel ? 1.4f : 0.6f);
		C.CardShape(Card, X + CardW / 2.f, Y + CardH / 2.f - 6.f, 30.f, 1.f, bSel, Pulse, false);
		C.TextCentered(SuitName(Card.Suit), X + CardW / 2.f, Y + CardH - 12.f, Col, 10.f, false, CardW - 6.f);
		C.Text(FString::FromInt(I + 1), X + 7.f, Y + 14.f, Dim, 10.f);
	}

	// ---- 왼쪽 족보 안내
	PaintHandGuide(C);

	// ---- 오른쪽: 판 위력 카드 (초당 피해 어림, 족보 덤, 가장 센 타워 셋)
	C.Box(RX, FY, PW, PowCardH, Hex(TEXT("06060b"), 0.5f));
	C.Glow(FCanvas::RoundRectPoints(RX, FY, PW, PowCardH, 10.f), UiEdge, 1.f, 0.35f);
	C.Text(TEXT("판 위력"), RX + 14.f, FY + 22.f, UiTitle, 13.f, 0, true);
	if (PowerRef > 0.f && PowerSum > 0.f) // 이번 웨이브 시작 때보다 몇 % (위쪽 띠에서 옮김, D-163)
	{
		const float Pct = (PowerSum / PowerRef - 1.f) * 100.f;
		if (FMath::Abs(Pct) >= 1.f) C.Text(FString::Printf(TEXT("%s%.0f%%"), Pct > 0.f ? TEXT("+") : TEXT(""), Pct), RX + 14.f, FY + 76.f, Pct > 0.f ? Green : Hex(TEXT("ff6a6a")), 10.f, 0, true);
	}
	C.Text(TEXT("초당 피해 어림"), RX + PW - 14.f, FY + 22.f, Dim, 9.f, 2);
	C.Text(CommaNum(FMath::RoundToInt(PowerSum)), RX + 14.f, FY + 58.f, UiValue, 28.f, 0, true, 112.f);
	{
		int32 Jokers = 0;
		for (const auto& KV : G.Towers) Jokers += KV.second.IsJoker();
		C.Text(TrF(TEXT("족보 덤 ×%.1f"), PowerGain), RX + PW - 14.f, FY + 44.f, Fg, 10.f, 2, false, 90.f);
		C.Text(TrF(TEXT("족보 %d · 조커 %d"), (int32)G.Synergies.size(), Jokers), RX + PW - 14.f, FY + 60.f, Grey, 10.f, 2, false, 90.f);
	}
	if (PowerList.empty()) C.Text(TEXT("타워를 놓으면 위력이 보인다"), RX + 14.f, FY + 100.f, Dim, 10.f, 0, false, PW - 28.f);
	static const TCHAR* SuitSym[] = {TEXT("♠"), TEXT("♥"), TEXT("♦"), TEXT("♣"), TEXT("★")};
	for (int32 I = 0; I < PowTopN && I < (int32)PowerList.size(); ++I) // 딜러 순위 (D-164): 이름 · 몫 막대 · 초당 피해 · 몫
	{
		auto It = G.Towers.find(PowerList[I].first);
		if (It == G.Towers.end()) continue;
		const FPDTower& T = It->second;
		const float Y = FY + 88.f + I * 20.f;
		const FLinearColor Col = T.IsJoker() ? JokerPink : SuitColor(T.Card.Suit);
		const FString Name = T.IsJoker() ? FString::Printf(TEXT("%s Lv%d"), JokerName(T.Joker), T.JLevel)
			: FString::Printf(TEXT("%s%s Lv%d"), T.Card.IsWild() ? TEXT("") : *RankText(T.Card.Rank), SuitSym[(int)T.Card.Suit], T.Level);
		const float Share = PowerSum > 0.f ? PowerList[I].second / PowerSum : 0.f;
		C.Text(Name, RX + 14.f, Y + 4.f, Col, 10.f, 0, true, 84.f);
		C.Box(RX + 100.f, Y - 4.f, 44.f * Share, 8.f, WithAlpha(Col, 0.4f));
		C.Lines(FCanvas::RectPoints(RX + 100.f, Y - 4.f, 44.f, 8.f), WithAlpha(Col, 0.5f), 1.f);
		C.Text(FString::Printf(TEXT("%s · %d%%"), *ShortNum(PowerList[I].second), FMath::RoundToInt(Share * 100.f)), RX + PW - 14.f, Y + 4.f, UiValue, 10.f, 2, false, PW - 160.f);
	}

	// ---- 웨이브 시작 (Space)
	{
		const bool bHov = InRect(Mouse, RBtnX, StartBtnY, RBtnW, StartBtnH);
		C.Box(RBtnX, StartBtnY, RBtnW, StartBtnH, Hex(TEXT("06060b"), 0.6f));
		if (G.Phase == EPDPhase::Build)
		{
			C.Box(RBtnX, StartBtnY, RBtnW, StartBtnH, WithAlpha(Cyan, bHov ? 0.12f : 0.06f));
			if (G.AutoStartT >= 0.f) // 자동 시작까지 버튼이 차오른다
				C.Box(RBtnX, StartBtnY, RBtnW * (1.f - FMath::Clamp(G.AutoStartT / AutoStartDelayFor(G.Wave + 1), 0.f, 1.f)), StartBtnH, WithAlpha(Green, 0.12f));
			C.Glow(FCanvas::RoundRectPoints(RBtnX, StartBtnY, RBtnW, StartBtnH, 12.f), Cyan, 2.f, 0.6f + 0.6f * Pulse);
			C.TextCentered(TrF(TEXT("웨이브 %d 시작"), G.Wave + 1), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH / 2.f - (G.AutoStartT >= 0.f ? 6.f : 0.f), Cyan, 16.f, true, RBtnW - 50.f);
			if (G.AutoStartT >= 0.f) C.TextCentered(TrF(TEXT("%d초 뒤 자동"), FMath::CeilToInt(G.AutoStartT)), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH - 10.f, Green, 9.f, false);
			C.Text(TEXT("Space"), RBtnX + RBtnW - 10.f, StartBtnY + StartBtnH - 7.f, Dim, 9.f, 2);
			if (G.AutoStartT >= 0.f && G.AutoStartT <= 3.f) // 마지막 3초는 판 위에 크게
			{
				const float Fr = FMath::Frac(G.AutoStartT);
				C.TextCentered(FString::FromInt(FMath::CeilToInt(G.AutoStartT)), FX + Cols * CS / 2.f, FY + 60.f, WithAlpha(Green, 0.35f + 0.5f * Fr), 28.f + 14.f * Fr, true);
			}
		}
		else
		{
			C.Glow(FCanvas::RoundRectPoints(RBtnX, StartBtnY, RBtnW, StartBtnH, 12.f), Dim, 1.2f, 0.2f);
			C.TextCentered(G.Phase == EPDPhase::Wave ? TrF(TEXT("웨이브 %d 진행 중"), G.Wave) : FString(Tr(TEXT("웨이브 시작"))), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH / 2.f - 5.f, Grey, 14.f, true, RBtnW - 20.f);
			if (G.Phase == EPDPhase::Wave) C.TextCentered(TrF(TEXT("배속 ×%d (F)"), G.Speed), RBtnX + RBtnW / 2.f, StartBtnY + StartBtnH - 10.f, G.Speed > 1 ? Cyan : Dim, 10.f, true);
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
	// ---- 2×2 버튼: 트리 K · 강화 U / 도감 J · 메뉴 Esc
	{
		auto Btn = [&](float X, float Y, float W, float H, const TCHAR* Name, const TCHAR* Key, const FLinearColor& Col, bool bHot) {
			const bool bHov = InRect(Mouse, X, Y, W, H);
			C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.6f));
			if (bHov) C.Box(X, Y, W, H, WithAlpha(Col, 0.08f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, 10.f), UiEdge, bHov ? 1.6f : 1.2f, bHot ? 0.5f + 0.8f * Pulse : bHov ? 0.9f : 0.4f);
			C.Text(Name, X + 12.f, Y + H / 2.f + 5.f, bHot ? FMath::Lerp(UiValue, Col, 0.5f + 0.5f * Pulse) : UiValue, 13.f, 0, true, W - 34.f); // 글자는 흰빛 (D-163), 찍을 노드가 있으면 깜빡
			C.Text(Key, X + W - 9.f, Y + H / 2.f + 4.f, Dim, 9.f, 2);
		};
		const bool bPlay = G.Phase != EPDPhase::Title;
		Btn(TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH, TEXT("스킬 트리"), TEXT("K"), Purple, bPlay && G.Gold >= G.PointPrice());
		Btn(UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH, TEXT("골드 강화"), TEXT("U"), Gold, false);
		Btn(CodexBtnX, CodexBtnY, CodexBtnW, CodexBtnH, TEXT("조커 도감"), TEXT("J"), JokerPink, false);
		Btn(MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH, TEXT("메뉴"), TEXT("Esc"), Grey, false);
	}
	// ---- 부적: 이름·설명
	{
		const float H = DesignH - 30.f - CharmY;
		const int32 N = (int32)G.Charms.size();
		C.Box(RX, CharmY, PW, H, Hex(TEXT("06060b"), 0.5f));
		C.Glow(FCanvas::RoundRectPoints(RX, CharmY, PW, H, 10.f), UiEdge, 1.f, 0.3f);
		C.Text(TrF(TEXT("부적 %d/%d"), N, CharmMax), RX + 14.f, CharmY + 22.f, UiTitle, 12.f, 0, true);
		const int32 W0 = G.Phase == EPDPhase::Wave ? G.Wave : G.Wave + 1;
		C.Text(TrF(TEXT("다음 %d웨이브"), (W0 + CharmEvery - 1) / CharmEvery * CharmEvery), RX + PW - 14.f, CharmY + 22.f, Dim, 9.f, 2);
		// 동그란 칸은 뺐다 (D-157): 이름·설명만
		if (N == 0) C.Text(TrF(TEXT("%d웨이브마다 하나를 고른다"), CharmEvery), RX + 14.f, CharmY + 50.f, Dim, 10.f, 0, false, PW - 28.f);
		for (int32 I = 0; I < N; ++I)
		{
			const float Y = CharmY + 50.f + I * 32.f;
			C.Text(CharmName((int)G.Charms[I]), RX + 14.f, Y, UiValue, 11.f, 0, true, PW - 28.f);
			C.Text(CharmDesc((int)G.Charms[I]), RX + 14.f, Y + 14.f, Grey, 10.f, 0, false, PW - 28.f);
		}
	}
	// ---- 도움말: 마우스를 올리면 키 안내
	{
		const bool bHov = InRect(Mouse, HelpX, HelpY, HelpW, HelpH);
		C.Text(TEXT("?  도움말"), HelpX + HelpW, HelpY + 13.f, bHov ? Fg : Dim, 10.f, 2);
		if (bHov)
		{
			static const TCHAR* Lines[] = {TEXT("카드 클릭(숫자키) → 칸 클릭: 설치 · 타워 위에 놓으면 교체"), TEXT("오른쪽 클릭: 카드 버리기 · 고른 것 취소"),
				TEXT("족보에 마우스: [합성] G · [조커] 버튼 · 조커 클릭: 먹이기"), TEXT("Space 웨이브 시작 · F 속도 · M 소리 · F3 프레임 표시"), TEXT("K 스킬 트리 · U 골드 강화 · J 조커 도감 · Esc 메뉴")};
			const float W = 400.f, Hh = 24.f + UE_ARRAY_COUNT(Lines) * 18.f, X = DesignW - 16.f - W, Y = HelpY - 6.f - Hh;
			C.Box(X, Y, W, Hh, Hex(TEXT("0a0b14"), 0.96f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, W, Hh, 8.f), Cyan, 1.f, 0.4f);
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
	// 부적 고르기 (5웨이브마다, D-101)
	if (G.Phase == EPDPhase::Reward)
	{
		C.Box(FX, FY, Cols * CS, Rows * CS, Hex(TEXT("06060b"), 0.9f));
		C.TextCentered(TrF(TEXT("웨이브 %d 부적 — 하나를 고른다"), G.Wave), FX + Cols * CS / 2.f, FY + 112.f, Gold, 22.f, true, Cols * CS - 20.f);
		for (int32 I = 0; I < (int32)G.Rewards.size(); ++I)
		{
			const FPDReward& R = G.Rewards[I];
			const FVector2f P = RewardPos(I);
			const bool bHov = InRect(Mouse, P.X, P.Y, RewardW, RewardH);
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

	// ---- 조커 도감 (J, D-119): 카드 5×2 + 오른쪽에 고른 조커 크게
	if (bCodex)
	{
		const FLinearColor Green = Hex(TEXT("9cffb0"));
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.95f));
		C.Glow(FCanvas::RoundRectPoints(16.f, HudY, DesignW - 32.f, HudH, 10.f), UiEdge, 1.f, 0.4f);
		C.Text(TEXT("조커 도감"), 34.f, HudY + 33.f, JokerPink, 22.f, 0, true);
		C.Text(TrF(TEXT("%d / 10 발견"), FMath::CountBits(G.Discovered)), 170.f, HudY + 32.f, Fg, 14.f, 0, true);
		C.TextCentered(TEXT("맞닿은 족보 3개(또는 2개)가 레시피와 맞으면 조커. 3개 쪽이 카드가 많아 세다"), 700.f, HudY + HudH / 2.f, Grey, 11.f, false, 640.f);
		PaintCloseButton(C, JokerPink);
		auto TierCol = [&](float T) { return T >= 2.9f ? Gold : T >= 2.4f ? Hex(TEXT("ffa86a")) : Cyan; };
		auto TierText = [](float T) { return FMath::IsNearlyEqual(T, FMath::RoundToFloat(T)) ? FString::Printf(TEXT("×%.0f"), T) : FString::Printf(TEXT("×%.1f"), T); };
		// 족보 알약: 색 테두리 + 옅은 채움 + 이름. 끝 X 를 돌려준다
		auto Chips = [&](float X, float Y, const TArray<EPDHand>& Hands, bool bShort, float Px) {
			for (EPDHand H : Hands)
			{
				const FString N = bShort ? FString(HandChip(H)) : FString(HandName(H));
				const float W = FCanvas::Measure(N, FCanvas::Font(Px, true)).X + 12.f, Hh = Px + 8.f;
				const FLinearColor Col = HandColor(H);
				C.Box(X, Y, W, Hh, WithAlpha(Col, 0.1f));
				C.Glow(FCanvas::RoundRectPoints(X, Y, W, Hh, Hh / 2.f), Col, 1.f, 0.35f);
				C.TextCentered(N, X + W / 2.f, Y + Hh / 2.f, Col, Px, true);
				X += W + 4.f;
			}
			return X;
		};
		auto R3 = [](int K) { const auto& R = JokerRecipe((EPDJoker)K); return TArray<EPDHand>{R[0], R[1], R[2]}; };
		auto R2 = [](int K) { const auto& R = JokerRecipe2((EPDJoker)K); return TArray<EPDHand>{R[0], R[1]}; };
		for (int32 K = 0; K < (int32)EPDJoker::Count; ++K)
		{
			const FVector2f P = CodexCardPos(K);
			const float X = P.X, Y = P.Y, CX = X + CdxCardW / 2.f;
			const bool bKnown = (G.Discovered >> K) & 1u, bSelK = K == CodexSel, bHov = InRect(Mouse, X, Y, CdxCardW, CdxCardH);
			const float Tier = JokerTier((EPDJoker)K);
			C.Box(X, Y, CdxCardW, CdxCardH, bSelK ? WithAlpha(JokerPink, 0.07f) : Hex(TEXT("06060b"), 0.6f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, CdxCardW, CdxCardH, 12.f), WithAlpha(bKnown ? JokerPink : Dim, bKnown || bSelK ? 1.f : 0.7f), bSelK ? 2.f : 1.2f, bSelK ? 1.f + 0.4f * Pulse : bHov ? 0.8f : 0.35f);
			C.JokerTower(K, CX, Y + 62.f, 36.f, bKnown ? 1.f : 0.28f, bKnown && bSelK ? Pulse : 0.f);
			C.TextCentered(bKnown ? JokerName(K) : TEXT("???"), CX, Y + 124.f, bKnown ? JokerPink : Grey, 16.f, true, CdxCardW - 14.f);
			{
				const FString Tt = TrF(TEXT("위력 %s"), *TierText(Tier));
				const FLinearColor Tc = TierCol(Tier);
				C.Glow(FCanvas::RoundRectPoints(CX - 32.f, Y + 139.f, 64.f, 18.f, 9.f), Tc, 1.f, 0.4f);
				C.TextCentered(Tt, CX, Y + 148.f, Tc, 10.f, true);
			}
			C.TextCentered(bKnown ? FString(JokerAbility(K)) : FString(TEXT("만들면 열림")), CX, Y + 174.f, bKnown ? Fg : Dim, 10.f, false, CdxCardW - 14.f);
			C.Text(TEXT("3개"), X + 10.f, Y + 202.f, Dim, 9.f);
			Chips(X + 10.f, Y + 208.f, R3(K), true, 9.f);
			C.Text(TEXT("2개"), X + 10.f, Y + 248.f, Dim, 9.f);
			Chips(X + 10.f, Y + 254.f, R2(K), true, 9.f);
		}
		// 오른쪽: 고른 조커 크게
		{
			const int32 K = FMath::Clamp(CodexSel, 0, (int32)EPDJoker::Count - 1);
			const bool bKnown = (G.Discovered >> K) & 1u;
			const float X = CdxDetX, Y = CdxCardY, W = CdxDetW, H = CdxCardH * 2.f + CdxGapY, CX = X + W / 2.f;
			const float Tier = JokerTier((EPDJoker)K);
			C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.6f));
			C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, 12.f), JokerPink, 1.4f, 0.6f);
			C.JokerTower(K, CX, Y + 92.f, 62.f, bKnown ? 1.f : 0.3f, bKnown ? Pulse : 0.f);
			C.TextCentered(bKnown ? JokerName(K) : TEXT("???"), CX, Y + 186.f, bKnown ? JokerPink : Grey, 28.f, true, W - 30.f);
			C.TextCentered(TrF(TEXT("%s 대장 · 위력 %s"), HandName(CaptainHand((EPDJoker)K)), *TierText(Tier)), CX, Y + 216.f, TierCol(Tier), 13.f, true, W - 30.f);
			C.Text(TEXT("능력"), X + 24.f, Y + 258.f, Dim, 10.f);
			C.Text(bKnown ? FString(JokerAbility(K)) : FString(TEXT("만들어 보면 열린다")), X + 24.f, Y + 282.f, bKnown ? Fg : Dim, 14.f, 0, true, W - 48.f);
			if (bKnown) C.Text(JokerCommand(K), X + 24.f, Y + 306.f, Fg, 12.f, 0, false, W - 48.f);
			C.Text(TEXT("3개 레시피"), X + 24.f, Y + 348.f, Dim, 10.f);
			Chips(X + 24.f, Y + 356.f, R3(K), false, 11.f);
			C.Text(TEXT("2개 레시피"), X + 24.f, Y + 404.f, Dim, 10.f);
			Chips(X + 24.f, Y + 412.f, R2(K), false, 11.f);
			int32 NStr = 0, NFl = 0;
			for (EPDHand Hh : R3(K)) { NStr += Hh == EPDHand::Straight; NFl += Hh == EPDHand::Flush; }
			C.Text(NStr >= 2 ? TEXT("스트레이트끼리는 숫자가 이어지지 않게") : NFl >= 2 ? TEXT("플러시끼리는 무늬가 다르게") : TEXT("숫자가 겹치면 큰 족보로 합쳐지니 주의"),
				X + 24.f, Y + 458.f, Grey, 10.f, 0, false, W - 48.f);
			const float BY = Y + H - 134.f;
			C.Glow(FCanvas::RoundRectPoints(X + 24.f, BY, W - 48.f, 110.f, 10.f), Green, 1.2f, 0.4f);
			C.Text(TEXT("위력 계산"), X + 40.f, BY + 24.f, Green, 12.f, 0, true);
			C.Text(TrF(TEXT("재료 위력 합 × %.1f × 등급 %s"), G.JokerMult(), *TierText(Tier)), X + 40.f, BY + 48.f, Fg, 12.f, 0, true, W - 80.f);
			C.Text(TrF(TEXT("× 카드 수 덤 (한 장마다 +%d%%)"), FMath::RoundToInt(JokerCardBonus * 100.f)), X + 40.f, BY + 70.f, Fg, 12.f, 0, true, W - 80.f);
			C.Text(TEXT("3개 레시피(6장 이상)가 2개(4장)보다 세다"), X + 40.f, BY + 92.f, Grey, 10.f, 0, false, W - 80.f);
		}
		C.Text(TEXT("카드 클릭: 크게 보기 · J / Esc: 닫기"), DesignW - 16.f, DesignH - 8.f, Dim, 9.f, 2);
	}

	// ---- 골드 강화 (U)
	if (bUpg)
	{
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.95f));
		C.Text(TEXT("골드 강화"), UpgRowX, 52.f, Hex(TEXT("ff9a6a")), 22.f, 0, true);
		C.Text(TrF(TEXT("골드 %d   강화에 쓴 골드 %d   (이번 판에서만 유지, 최대 %d레벨, 값 = 10 + 2 × 레벨)"), G.Gold, G.GoldUpg, UpgradeMax), UpgRowX, 82.f, Gold, 12.f, 0, false, UpgRowW);
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
				const bool bHov = InRect(Mouse, B.X, B.Y, UpgBtnW, UpgBtnH);
				C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, UpgBtnW, UpgBtnH, 6.f), WithAlpha(Col, bCan ? 1.f : 0.3f), bHov ? 1.8f : 1.2f, bCan ? (bHov ? 1.2f : 0.5f) : 0.15f);
				C.TextCentered(K < 3 ? FString::Printf(TEXT("+%d"), Want) : FString(TEXT("최대")), B.X + UpgBtnW / 2.f, B.Y + 10.f, WithAlpha(Col, bCan ? 1.f : 0.4f), 11.f, true);
				C.TextCentered(N > 0 ? FString::Printf(TEXT("%d"), Cost) : FString(TEXT("-")), B.X + UpgBtnW / 2.f, B.Y + 22.f, bCan ? Gold : Dim, 9.f, false, UpgBtnW - 4.f);
			}
		}
		C.TextCentered(TEXT("버튼 클릭: 강화   U 또는 Esc · 오른쪽 클릭: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, DesignH - 16.f, Dim, 11.f, false);
		PaintCloseButton(C, Hex(TEXT("ff9a6a")));
	}
}

// 스킬 트리 창 (K, D-119): 위쪽 띠, 노드마다 하는 일 아이콘, 범례, 설명
void SPDBoardWidget::PaintTree(FCanvas& C) const
{
	if (!bTree) return;
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const FLinearColor Purple = Hex(TEXT("b48cff")), Cyan = Hex(TEXT("7fd6ff"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.95f));
	// 위쪽 띠
	C.Glow(FCanvas::RoundRectPoints(16.f, HudY, DesignW - 32.f, HudH, 10.f), UiEdge, 1.f, 0.4f);
	C.Text(TEXT("스킬 트리"), 34.f, HudY + 33.f, Purple, 22.f, 0, true);
	C.Text(TrF(TEXT("골드 %s"), *CommaNum(G.Gold)), 170.f, HudY + 32.f, Gold, 14.f, 0, true, 130.f);
	C.Text(TrF(TEXT("다음 노드 %s 골드"), *CommaNum(G.PointPrice())), 310.f, HudY + 32.f, G.Gold >= G.PointPrice() ? Green : Grey, 13.f, 0, true, 200.f);
	C.Text(TrF(TEXT("키스톤 %d / %d"), G.KeystoneCount(), KeystoneMax), 530.f, HudY + 32.f, Gold, 13.f, 0, true);
	C.Text(TrF(TEXT("찍은 노드 %d"), G.Points), 660.f, HudY + 32.f, Fg, 13.f, 0, true);
	C.Text(TrF(TEXT("이번 판에서만 · 값은 찍을 때마다 ×%.2f"), PointPriceGrowth), CdxCloseX - 16.f, HudY + 32.f, Dim, 10.f, 2, false, 330.f);
	PaintCloseButton(C, Purple);
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
		const bool bNext = Can == FPDGame::EBuy::Ok || Can == FPDGame::EBuy::NoGold; // 이어져 있어 찍을 수 있는 자리
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
		case EPDNode::Small:
			if (bOwn) C.Box(P.X - 7.f, P.Y - 7.f, 14.f, 14.f, WithAlpha(Col, 0.12f));
			C.Glow(FCanvas::CirclePoints(P, 11.f, 28), Ca, Th, Gk);
			break;
		case EPDNode::Notable:
			if (bOwn) C.Box(P.X - 14.f, P.Y - 14.f, 28.f, 28.f, WithAlpha(Col, 0.1f + 0.05f * Pulse));
			C.Glow(FCanvas::RoundRectPoints(P.X - 15.f, P.Y - 15.f, 30.f, 30.f, 4.f), Ca, Th, Gk);
			break;
		case EPDNode::Keystone:
		{
			TArray<FVector2f> O;
			for (int32 J = 0; J <= 8; ++J) { const float A = (22.5f + 45.f * J) * TwoPi / 360.f; O.Add(P + FVector2f(FMath::Cos(A), FMath::Sin(A)) * 21.f); }
			if (bOwn) C.Box(P.X - 13.f, P.Y - 13.f, 26.f, 26.f, WithAlpha(Col, 0.12f + 0.06f * Pulse));
			C.Glow(O, Ca, Th, Gk);
			break;
		}
		case EPDNode::Joker:
			C.JokerTower(N.Fx[0].Param, P.X, P.Y, 16.f, Al, bOwn ? Pulse : 0.f);
			if (Can == FPDGame::EBuy::Ok) C.Glow(FCanvas::RoundRectPoints(P.X - 20.f, P.Y - 20.f, 40.f, 40.f, 6.f), WithAlpha(JokerPink, 0.5f * Pulse), 1.f, 0.4f);
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
	// 범례: 오른쪽 위 빈자리 (노드가 없는 곳, D-130). 예전 왼쪽 아래 자리는 노드를 가렸다
	{
		const float X = 1016.f, Y = 72.f, W = 248.f, H = 126.f;
		C.Box(X, Y, W, H, Hex(TEXT("06060b"), 0.85f));
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, 10.f), Dim, 1.f, 0.3f);
		const TCHAR* Names[] = {TEXT("찍음"), TEXT("찍을 수 있음"), TEXT("아직 못 찍음"), TEXT("봉인 (짝 키스톤)")};
		const FLinearColor LegCol[] = {Cyan, Green, Dim, Red};
		for (int32 K = 0; K < 4; ++K)
		{
			const float Ly = Y + 20.f + K * 20.f;
			C.Glow(FCanvas::CirclePoints(FVector2f(X + 22.f, Ly), 7.f, 20), LegCol[K], 1.4f, 0.6f);
			C.Text(Names[K], X + 38.f, Ly + 4.f, LegCol[K], 10.f, 0, true, W - 50.f);
		}
		C.Text(TEXT("키스톤은 둘씩 짝: 하나를 찍으면"), X + 14.f, Y + 102.f, Grey, 9.f, 0, false, W - 28.f);
		C.Text(TEXT("짝은 이 판 동안 봉인 · 판마다 3개까지"), X + 14.f, Y + 116.f, Grey, 9.f, 0, false, W - 28.f);
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
		case FPDGame::EBuy::Ok: State = TrF(TEXT("찍을 수 있음 · %s 골드"), *CommaNum(G.PointPrice())); SCol = Green; break;
		case FPDGame::EBuy::Owned: State = TEXT("찍음"); SCol = Cyan; break;
		case FPDGame::EBuy::NotLinked: State = TEXT("찍은 노드와 이어져야 한다"); break;
		case FPDGame::EBuy::Sealed: State = TEXT("짝 키스톤을 찍어서 봉인됨"); SCol = Red; break;
		case FPDGame::EBuy::KeyCap: State = TrF(TEXT("키스톤은 판마다 %d개까지"), KeystoneMax); SCol = Red; break;
		case FPDGame::EBuy::NoGold: State = TrF(TEXT("골드 부족 (%s / %s)"), *CommaNum(G.Gold), *CommaNum(G.PointPrice())); SCol = Red; break;
		default: State = TEXT("지금은 찍을 수 없다 (보상 고르는 중)"); break;
		}
		TArray<FString> Lines;
		for (const FPDNodeFx& Fx : N.Fx) if (Fx.Stat != EPDStat::None) Lines.Add(TreeFxText(Fx));
		const bool bKey = N.Kind == EPDNode::Keystone;
		const float W = 320.f, H = 52.f + Lines.Num() * 20.f + (bKey ? 20.f : 0.f) + 34.f;
		const FVector2f P = TreeNodePos(Hov);
		const float X = P.X + 34.f + W > DesignW - 12.f ? P.X - 34.f - W : P.X + 34.f, Y = FMath::Clamp(P.Y - 60.f, 76.f, DesignH - H - 12.f);
		const FLinearColor Edge = Can == FPDGame::EBuy::Ok ? Green : TreeNodeColor(Hov);
		C.Box(X, Y, W, H, Hex(TEXT("0a0b14"), 0.97f));
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, H, 10.f), Edge, 1.4f, 0.6f);
		C.Text(TreeNodeName(Hov), X + 14.f, Y + 22.f, bKey ? Gold : TreeNodeColor(Hov), 14.f, 0, true, W - 28.f);
		static const TCHAR* KindName[] = {TEXT("시작점"), TEXT("작은 노드"), TEXT("특급 노드"), TEXT("키스톤"), TEXT("조커 강화")};
		C.Text(FString::Printf(TEXT("%s · %s"), KindName[(int)N.Kind], *State), X + 14.f, Y + 42.f, SCol, 10.f, 0, true, W - 28.f);
		float Ly = Y + 66.f;
		for (const FString& L : Lines) { C.Text(L, X + 14.f, Ly, Fg, 12.f, 0, false, W - 28.f); Ly += 20.f; }
		if (bKey) { C.Text(TrF(TEXT("짝: %s (함께 못 찍음)"), KeystoneName(N.Fx[0].Param ^ 1)), X + 14.f, Ly, Red, 10.f, 0, false, W - 28.f); Ly += 20.f; }
		if (Can == FPDGame::EBuy::Ok)
		{
			C.Glow(FCanvas::RoundRectPoints(X + 14.f, Ly - 6.f, 130.f, 22.f, 8.f), Green, 1.2f, 0.5f + 0.4f * Pulse);
			C.TextCentered(TEXT("클릭해서 찍기"), X + 79.f, Ly + 5.f, Green, 10.f, true);
		}
	}
}

// 시작 화면 (D-153): 제목 + 게임 시작 · 환경 설정 · 게임 종료
void SPDBoardWidget::PaintTitle(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Fg = Hex(TEXT("cfd8e6")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f), CX = DesignW / 2.f;
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.97f));
	// 무늬 넷이 천천히 떠다닌다
	for (int32 I = 0; I < 4; ++I)
		C.Shape((EPDSuit)I, CX - 210.f + I * 140.f, 150.f + 8.f * FMath::Sin(Clock * 1.5f + I * 1.3f), 18.f, SuitColor((EPDSuit)I), 1.6f, 0.8f + 0.3f * Pulse);
	C.TextCentered(TEXT("POKER DEFENSE"), CX, 240.f, Gold, 58.f, true);
	C.TextCentered(TEXT("카드를 놓아 족보를 만들고, 끝없이 몰려오는 적을 막아라"), CX, 296.f, Fg, 15.f, false);
	static const TCHAR* Labels[] = {TEXT("이어하기"), TEXT("게임 시작"), TEXT("환경 설정"), TEXT("게임 종료")};
	for (int32 I = 0; I < (int32)ETitleItem::Count; ++I)
	{
		if (I == (int32)ETitleItem::Continue && !bHasRunSave) continue; // 이어할 판이 없으면 빈 자리 (D-162)
		const FVector2f P = TitleBtnPos(I, bHasRunSave);
		const bool bSel = TitleSel == I;
		const FLinearColor Col = I <= 1 ? Gold : Cyan;
		C.Box(P.X, P.Y, TitleBtnW, TitleBtnH, Hex(TEXT("0a0b14"), 0.92f));
		if (bSel) C.Box(P.X, P.Y, TitleBtnW, TitleBtnH, WithAlpha(Col, 0.12f + 0.06f * Pulse));
		C.Glow(FCanvas::RoundRectPoints(P.X, P.Y, TitleBtnW, TitleBtnH, 10.f), UiEdge, bSel ? 2.2f : 1.3f, bSel ? 1.2f + 0.5f * Pulse : 0.5f);
		C.TextCentered(Labels[I], CX, P.Y + TitleBtnH / 2.f, bSel ? FLinearColor::White : Col, 18.f, true);
	}
	const float FootY = TitleBtnPos((int32)ETitleItem::Count, bHasRunSave).Y + 14.f;
	if (Game.BestWave > 0) C.TextCentered(TrF(TEXT("최고 기록 %d웨이브"), Game.BestWave), CX, FootY, Grey, 12.f, false);
	C.TextCentered(TEXT("위아래 키 또는 마우스로 고르기 · Enter 또는 클릭으로 결정"), CX, DesignH - 30.f, Dim, 11.f, false);
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
		TEXT("왼쪽 족보 목록을 클릭하면 그 족보만 펼쳐 보고 바로 합성할 수 있다"),
		TEXT("맞닿은 족보 3개(또는 2개)로 조커를 만들 수 있다"),
		TEXT("자리바꿈 보상으로 타워 둘의 자리를 바꿀 수 있다"),
		TEXT("무늬는 공격 방식, 숫자는 공격력")};
	C.TextCentered(Tips[FMath::Clamp(LoadTip, 0, 5)], CX, 466.f, Fg, 13.f, false, 760.f);
}

// 왼쪽 족보 안내 (D-119): 이름 + 효과 한 줄. 판 위에 있는 족보는 밝게, 개수 표시
void SPDBoardWidget::PaintHandGuide(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Dim = Hex(TEXT("5f6878")), Grey = Hex(TEXT("8a94a6"));
	TSet<int32> PvKinds;
	int32 Count[(int)EPDHand::Count] = {};
	for (const FPDSynergy& S : Preview) PvKinds.Add((int32)S.Kind);
	for (const FPDSynergy& S : G.Synergies) ++Count[(int)S.Kind];
	const float PH = DesignH - 16.f - FY;
	C.Box(LX, FY, LW, PH, Hex(TEXT("06060b"), 0.5f));
	C.Glow(FCanvas::RoundRectPoints(LX, FY, LW, PH, 10.f), Cyan, 1.f, 0.3f);
	C.Text(TEXT("족보"), LX + 14.f, FY + 24.f, Cyan, 15.f, 0, true);
	C.Text(HandFilter >= 0 ? TEXT("다시 클릭 / Esc: 필터 끄기") : TEXT("클릭: 그 족보만 보기"), LX + LW - 14.f, FY + 24.f, HandFilter >= 0 ? Gold : Dim, 9.f, 2, false, LW - 70.f);
	constexpr float RowH = HandRowH;
	const float Y0 = HandRowY0;
	for (int32 I = 0; I < (int32)EPDHand::Count; ++I)
	{
		const EPDHand H = (EPDHand)I;
		const float Y = Y0 + I * RowH;
		const bool bPv = PvKinds.Contains(I), bOn = Count[I] > 0;
		const FLinearColor HC = HandColor(H);
		if (bPv) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(Gold, 0.1f));
		else if (bOn) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(HC, 0.08f));
		FString Name = HandName(H);
		if (H == EPDHand::Flush) Name += Tr(TEXT(" (3~5장)"));
		if (H == EPDHand::Straight) Name += Tr(TEXT(" (2~5장)"));
		C.Text(Name, LX + 18.f, Y + 16.f, bPv ? Gold : bOn ? HC : WithAlpha(Grey, 0.95f), 12.f, 0, true, LW - 78.f);
		C.Text(HandEffect(H), LX + 18.f, Y + 31.f, bPv ? Gold : Dim, 9.f, 0, false, LW - 44.f);
		if (HandFilter == I) // 족보 필터 (D-148): 고른 줄은 테두리
			C.Glow(FCanvas::RoundRectPoints(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, 6.f), HC, 1.6f, 0.9f + 0.4f * (0.5f + 0.5f * FMath::Sin(Clock * 6.f)));
		if (bOn)
		{
			C.Glow(FCanvas::RoundRectPoints(LX + LW - 42.f, Y + 6.f, 28.f, 16.f, 8.f), HC, 1.1f, 0.5f);
			C.TextCentered(FString::Printf(TEXT("×%d"), Count[I]), LX + LW - 28.f, Y + 14.f, HC, 10.f, true);
		}
	}
	{ // 조커 줄 (D-152): 만들 수 있는 조커 수, 클릭하면 조커 합성 버튼을 모두 펼친다
		const float Y = Y0 + JokerFilterId * RowH;
		const int32 NJ = (int32)JGroups.size();
		const bool bOn = NJ > 0;
		if (bOn) C.Box(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, WithAlpha(JokerPink, 0.08f));
		if (HandFilter == JokerFilterId)
			C.Glow(FCanvas::RoundRectPoints(LX + 8.f, Y + 2.f, LW - 16.f, RowH - 4.f, 6.f), JokerPink, 1.6f, 0.9f + 0.4f * (0.5f + 0.5f * FMath::Sin(Clock * 6.f)));
		C.Text(TEXT("조커"), LX + 18.f, Y + 16.f, bOn ? JokerPink : WithAlpha(Grey, 0.95f), 12.f, 0, true, LW - 78.f);
		C.Text(TEXT("맞닿은 족보 3개(또는 2개)로 만든다"), LX + 18.f, Y + 31.f, Dim, 9.f, 0, false, LW - 44.f);
		if (bOn)
		{
			C.Glow(FCanvas::RoundRectPoints(LX + LW - 42.f, Y + 6.f, 28.f, 16.f, 8.f), JokerPink, 1.1f, 0.5f);
			C.TextCentered(FString::Printf(TEXT("×%d"), NJ), LX + LW - 28.f, Y + 14.f, JokerPink, 10.f, true);
		}
	}
	const float FootY = Y0 + HandRowsN * RowH + 18.f;
	C.Text(TEXT("연쇄"), LX + 14.f, FootY, FLinearColor::White, 12.f, 0, true);
	C.Text(TrF(TEXT("족보끼리 맞닿으면 하나에 +%d%%씩"), G.Has(EPDCharm::ChainLink) ? 45 : 25), LX + 14.f, FootY + 16.f, Hex(TEXT("cfd8e6")), 10.f, 0, false, LW - 28.f);
	// 무늬 = 공격 방식
	const float SuitY = FootY + 42.f;
	C.Text(TEXT("무늬 = 공격 방식"), LX + 14.f, SuitY, Grey, 10.f);
	for (int32 I = 0; I < 4; ++I)
	{
		const EPDSuit S = (EPDSuit)I;
		const float X = LX + 14.f + (I % 2) * 108.f, Y = SuitY + 20.f + (I / 2) * 22.f;
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
		C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, MenuW, MenuItemH, 8.f), UiEdge, bSel ? 1.8f : 1.2f, bSel ? 1.0f + 0.5f * Pulse : 0.4f);
		C.TextCentered(MenuItemName(I), B.X + MenuW / 2.f, B.Y + MenuItemH / 2.f, bSel ? FLinearColor::White : Fg, 16.f, true);
	}
	C.TextCentered(TEXT("↑↓ · Enter 또는 클릭   Esc: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, BoxY + BoxH - 18.f, Dim, 11.f, false);
}

// 설정 창 (D-110): 화면 모드, 월드 이펙트, 흔들림·번쩍임, 소리, 볼륨
void SPDBoardWidget::PaintSettings(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Good = Hex(TEXT("9cffb0")), Grey = Hex(TEXT("8a94a6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.6f));
	C.Box(SetBoxX, SetBoxY, SetBoxW, SetBoxH, Hex(TEXT("06060b"), 0.95f));
	C.Glow(FCanvas::RoundRectPoints(SetBoxX, SetBoxY, SetBoxW, SetBoxH, 10.f), Cyan, 1.4f, 0.7f);
	C.TextCentered(TEXT("설정"), DesignW / 2.f, SetBoxY + 40.f, Cyan, 24.f, true);
	static const TCHAR* Labels[] = {TEXT("언어"), TEXT("화면"), TEXT("카드 색"), TEXT("프레임 제한"), TEXT("수직 동기화"), TEXT("월드 이펙트 (나이아가라)"), TEXT("화면 흔들림·번쩍임"), TEXT("소리"), TEXT("효과음 볼륨"), TEXT("음악 볼륨"), TEXT("")};
	auto Button = [&](float X, float Y, float W, const FString& S, const FLinearColor& Col, bool bOn, bool bRowSel) {
		if (bOn) C.Box(X, Y, W, SetBtnH, WithAlpha(Col, 0.12f));
		C.Glow(FCanvas::RoundRectPoints(X, Y, W, SetBtnH, 6.f), Col, bOn ? 1.6f : 1.1f, bOn ? (bRowSel ? 1.f + 0.5f * Pulse : 0.9f) : 0.35f);
		C.TextCentered(S, X + W / 2.f, Y + SetBtnH / 2.f, bOn ? FLinearColor::White : Grey, 13.f, true, W - 8.f);
	};
	for (int32 Row = 0; Row < (int32)ESetRow::Count; ++Row)
	{
		const float Y = SetRowY(Row);
		const bool bRowSel = Row == SetSel;
		if (bRowSel) C.Box(SetX - 10.f, Y, SetW + 20.f, SetRowH, WithAlpha(Cyan, 0.06f));
		if (Row != (int32)ESetRow::Close) C.Text(Labels[Row], SetX, Y + SetRowH / 2.f + 5.f, bRowSel ? FLinearColor::White : Fg, 14.f, 0, true, SetLabelW - 10.f);
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
			static const TCHAR* ThemeCol[] = {TEXT("8affc4"), TEXT("e6ebf5"), TEXT("ff5a6a")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Themes[I], Hex(ThemeCol[I]), Settings.CardTheme == I, bRowSel); }
			break;
		}
		case ESetRow::Frame:
		{
			static const TCHAR* Caps[] = {TEXT("60"), TEXT("120"), TEXT("제한 없음")};
			for (int32 I = 0; I < 3; ++I) { const FVector2f B = SetBtnPos(Row, I); Button(B.X, B.Y, SetBtnW(Row, I), Caps[I], Cyan, Settings.FrameCap == I, bRowSel); }
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
		case ESetRow::Close:
		{
			const FVector2f B = SetBtnPos(Row, 0);
			Button(B.X, B.Y, SetBtnW(Row, 0), TEXT("닫기"), Cyan, bRowSel, bRowSel);
			break;
		}
		default: break;
		}
	}
	if (GIsEditor) C.TextCentered(TEXT("화면 모드는 패키지 빌드에서만 바뀐다 (에디터에서는 저장만)"), DesignW / 2.f, SetBoxY + SetBoxH - 38.f, Dim, 10.f, false);
	C.TextCentered(TEXT("↑↓ 고르기 · ←→ 바꾸기 · Enter 켜기/끄기 · Esc 메뉴로"), DesignW / 2.f, SetBoxY + SetBoxH - 18.f, Dim, 11.f, false);
}
