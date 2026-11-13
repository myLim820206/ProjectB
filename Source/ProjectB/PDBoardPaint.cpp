// 포커 디펜스 화면 그리기: 판, 적, 발사체, 이펙트, 버튼, 패널, 창 (화풍 docs/15-art-style.md)
#include "PDBoardDraw.h"

#include "PDTreeText.inl" // 생성됨: 특급 노드 이름 (tools/tree/gen_tree.py)

namespace
{
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
} // namespace

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
	// 판 쪽(F, 흔들림을 받음): 판·적 → 발사체 → 이펙트 → 판 위 UI. 그 위에 고정 패널과 창(C)
	PaintField(F);
	PaintShots(F);
	PaintEffects(F);
	PaintBoardUi(F);
	PaintPanels(C);
	PaintScreens(C);
	PaintTree(C);
	// 화면 번쩍임과 덮개 화면 (제목, 게임 오버), 메뉴
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Red = Hex(TEXT("ff6a6a"));
	if (FlashT > 0.f) C.Box(0.f, 0.f, DesignW, DesignH, WithAlpha(FlashCol, FlashT * 0.45f));
	if (G.Phase == EPDPhase::Title)
		C.Overlay(TEXT("POKER DEFENSE"), Cyan,
			{TEXT("카드 한 장이 타워 하나. 무늬는 공격 방식, 숫자는 공격력"), TEXT("붙어 있는 타워들이 포커 족보가 되면 시너지 (모양 상관없이). 족보끼리 맞닿으면 연쇄 보너스"),
				FString::Printf(TEXT("웨이브를 막을 때마다 카드 %d장. 족보는 합성해서 레벨을 올린다. 끝없이 몰려오는 적을 얼마나 버틸까!"), DrawPerWave)},
			TEXT("Enter 키 또는 화면을 눌러 시작"));
	else if (G.Phase == EPDPhase::Over)
		C.Overlay(TEXT("GAME OVER"), Red, {FString::Printf(TEXT("웨이브 %d에서 무너졌다 · 처치 %d"), G.Wave, G.Kills), FString::Printf(TEXT("최고 기록 %d웨이브%s"), G.BestWave, G.Wave >= G.BestWave ? TEXT("  (갱신!)") : TEXT(""))}, TEXT("R 키 또는 화면을 눌러 다시"));
	if (bMenu) PaintMenu(C);
	if (bSettings) PaintSettings(C);
}

// 판: 격자·길, 족보 장판·테두리·연쇄 번개, 타워, 놓기 미리보기, 적
void SPDBoardWidget::PaintField(FCanvas& F) const
{
	const FPDGame& G = Game;
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	const bool bPv = !Preview.empty(); // 고른 카드를 놓으면 생길 족보를 보여 주는 중
	// 격자, 길, 길 위를 흐르는 에너지
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
}

// 판 위 UI: 족보 이름표, 합성 위치 고르기, 합성·먹이기·조커 버튼, 타워 정보, 떠오르는 글자, 배너
void SPDBoardWidget::PaintBoardUi(FCanvas& F) const
{
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Fg = Hex(TEXT("cfd8e6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	const bool bPv = !Preview.empty(); // 고른 카드를 놓으면 생길 족보를 보여 주는 중
	// 족보 이름표 (타워 위에)
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
			if (!SynRevealed(I)) { F.Glow(FCanvas::CirclePoints(B, 2.6f, 10), Gold, 1.6f, 0.6f + 0.3f * Pulse); continue; } // 평소엔 작은 금색 점 (D-113)
			const bool bHov = InBox(Mouse, B, FuseW, FuseH);
			F.Box(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, Hex(TEXT("06060b"), 0.9f));
			F.Glow(FCanvas::RoundRectPoints(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, 6.f), Gold, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(TEXT("합성"), B.X, B.Y, WithAlpha(Gold, bHov ? 1.f : 0.8f), 10.f, true);
			if (bHov && PickCells.empty()) F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::Fuse, I), B.X, B.Y - FuseH - 6.f);
		}
	// 먹이기 버튼: 마우스를 올리면 먹일 칸과 얻을 경험치
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty())
		for (const FPDFeed& Fd : G.FeedTargets())
		{
			const FVector2f B = FeedButtonCenter(G.Synergies, Fd.Syn, G.CanFuse(Fd.Syn));
			if (!SynRevealed(Fd.Syn)) { F.Glow(FCanvas::CirclePoints(B, 2.6f, 10), JokerPink, 1.6f, 0.6f + 0.3f * Pulse); continue; } // 평소엔 작은 분홍 점
			const bool bHov = InBox(Mouse, B, FuseW, FuseH);
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
				F.PowerBadge(G.PreviewPower(FPDGame::EPowerAction::Feed, Fd.Syn), B.X, B.Y - FuseH - 6.f);
			}
			F.Box(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, Hex(TEXT("06060b"), 0.9f));
			F.Glow(FCanvas::RoundRectPoints(B.X - FuseW / 2.f, B.Y - FuseH / 2.f, FuseW, FuseH, 6.f), JokerPink, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.5f);
			F.TextCentered(TEXT("먹이기"), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.8f), 10.f, true, FuseW - 4.f);
		}
	// 조커 합성 버튼 (연쇄로 이어진 족보 3개 이상)
	if (!bPv && G.Phase != EPDPhase::Reward)
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
			F.Box(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - JokerBtnW / 2.f, B.Y - JokerBtnH / 2.f, JokerBtnW, JokerBtnH, 7.f), JokerPink, bHov ? 1.8f : 1.3f, 1.f + 0.8f * Pulse);
			F.TextCentered(FString::Printf(TEXT("조커: %s"), bKnown ? JokerName((int)Gr.Result) : TEXT("???")), B.X, B.Y, JokerPink, 10.f, true, JokerBtnW - 6.f);
		}
	// 조커 합치기 버튼 (같은 조커끼리, D-114): 평소엔 분홍 점 둘, 마우스를 올리면 버튼
	if (!bPv && G.Phase != EPDPhase::Reward && PickCells.empty())
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
			F.Box(B.X - MergeW / 2.f, B.Y - MergeH / 2.f, MergeW, MergeH, Hex(TEXT("06060b"), 0.92f));
			F.Glow(FCanvas::RoundRectPoints(B.X - MergeW / 2.f, B.Y - MergeH / 2.f, MergeW, MergeH, 6.f), JokerPink, bHov ? 1.6f : 1.1f, bHov ? 1.2f + 0.5f * Pulse : 0.6f);
			F.TextCentered(TEXT("조커 합치기"), B.X, B.Y, WithAlpha(JokerPink, bHov ? 1.f : 0.85f), 10.f, true, MergeW - 6.f);
		}
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
}

// 고정 패널: 손패, 왼쪽 족보 패널, 오른쪽 상태 패널, 부적, 아래 도움말
void SPDBoardWidget::PaintPanels(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	// 손패
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
			const float U = 1.f - FMath::Clamp(G.AutoStartT / AutoStartDelayFor(G.Wave + 1), 0.f, 1.f);
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
	if (G.Charms.empty()) C.Text(FString::Printf(TEXT("%d웨이브마다 하나를 고른다"), CharmEvery), RX + 12.f, CharmY + 46.f, Dim, 11.f, 0, false, PW - 24.f);
	for (int32 I = 0; I < (int32)G.Charms.size(); ++I)
	{
		const float Y = CharmY + 46.f + I * 34.f;
		C.Text(CharmName((int)G.Charms[I]), RX + 12.f, Y, Gold, 11.f, 0, true, PW - 24.f);
		C.Text(CharmDesc((int)G.Charms[I]), RX + 12.f, Y + 14.f, Grey, 10.f, 0, false, PW - 24.f);
	}
	C.Text(TEXT("[합성]/G 레벨 업 · 족보 3개 연쇄 → [조커] · J 도감 · 카드 클릭(숫자키) → 칸 클릭으로 설치 · 타워 위에 놓으면 교체 · 오른쪽 클릭 버리기 · F 속도 · M 소리 · Esc 메뉴"), DesignW / 2.f, DesignH - 8.f, Dim, 11.f, 1, false, DesignW - 32.f);
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
		C.TextCentered(FString::Printf(TEXT("웨이브 %d 부적 — 하나를 고른다"), G.Wave), FX + Cols * CS / 2.f, FY + 112.f, Gold, 22.f, true, Cols * CS - 20.f);
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

	// ---- 조커 도감 (J)
	if (bCodex)
	{
		C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.94f));
		C.TextCentered(FString::Printf(TEXT("조커 도감  %d / 10"), FMath::CountBits(G.Discovered)), DesignW / 2.f, 48.f, JokerPink, 24.f, true);
		C.TextCentered(TEXT("연쇄로 이어진 족보 3개 (합성 타워도 기억한 족보로 침)를 합치면, 그중 가장 높은 족보의 대장 조커가 된다"), DesignW / 2.f, 74.f, Grey, 12.f, false, DesignW - 40.f);
		C.TextCentered(JokerRuleText(), DesignW / 2.f, 94.f, Gold, 11.f, false, DesignW - 40.f);
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
			C.TextCentered(TEXT("레시피"), X + 107.f, Y + 194.f, Dim, 9.f, false, 200.f);
			C.TextCentered(JokerRecipeText(K), X + 107.f, Y + 210.f, Fg, 10.f, true, 200.f);
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
				const bool bHov = InRect(Mouse, B.X, B.Y, UpgBtnW, UpgBtnH);
				C.Glow(FCanvas::RoundRectPoints(B.X, B.Y, UpgBtnW, UpgBtnH, 6.f), WithAlpha(Col, bCan ? 1.f : 0.3f), bHov ? 1.8f : 1.2f, bCan ? (bHov ? 1.2f : 0.5f) : 0.15f);
				C.TextCentered(K < 3 ? FString::Printf(TEXT("+%d"), Want) : FString(TEXT("최대")), B.X + UpgBtnW / 2.f, B.Y + 10.f, WithAlpha(Col, bCan ? 1.f : 0.4f), 11.f, true);
				C.TextCentered(N > 0 ? FString::Printf(TEXT("%d"), Cost) : FString(TEXT("-")), B.X + UpgBtnW / 2.f, B.Y + 22.f, bCan ? Gold : Dim, 9.f, false, UpgBtnW - 4.f);
			}
		}
		C.TextCentered(TEXT("버튼 클릭: 강화   U 또는 Esc: 닫기   (여는 동안 게임은 멈춘다)"), DesignW / 2.f, DesignH - 16.f, Dim, 11.f, false);
	}
}

// 스킬 트리 창 (K)
void SPDBoardWidget::PaintTree(FCanvas& C) const
{
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd96a")), Grey = Hex(TEXT("8a94a6")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("cfd8e6")), Green = Hex(TEXT("9cffb0")), Red = Hex(TEXT("ff6a6a"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
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

// 설정 창 (D-110): 화면 모드, 월드 이펙트, 흔들림·번쩍임, 소리, 볼륨
void SPDBoardWidget::PaintSettings(FCanvas& C) const
{
	const FLinearColor Cyan = Hex(TEXT("7fd6ff")), Fg = Hex(TEXT("cfd8e6")), Dim = Hex(TEXT("5f6878")), Good = Hex(TEXT("9cffb0")), Grey = Hex(TEXT("8a94a6"));
	const float Pulse = 0.5f + 0.5f * FMath::Sin(Clock * 4.f);
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 0.6f));
	C.Box(SetBoxX, SetBoxY, SetBoxW, SetBoxH, Hex(TEXT("06060b"), 0.95f));
	C.Glow(FCanvas::RoundRectPoints(SetBoxX, SetBoxY, SetBoxW, SetBoxH, 10.f), Cyan, 1.4f, 0.7f);
	C.TextCentered(TEXT("설정"), DesignW / 2.f, SetBoxY + 40.f, Cyan, 24.f, true);
	static const TCHAR* Labels[] = {TEXT("화면"), TEXT("월드 이펙트 (나이아가라)"), TEXT("화면 흔들림·번쩍임"), TEXT("소리"), TEXT("효과음 볼륨"), TEXT("음악 볼륨"), TEXT("")};
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
		case ESetRow::WorldFx:
		case ESetRow::Shake:
		case ESetRow::Sound:
		{
			const bool bOn = Row == (int32)ESetRow::WorldFx ? Settings.bWorldFx : Row == (int32)ESetRow::Shake ? Settings.bShake : Settings.bSound;
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
	if (GIsEditor) C.TextCentered(TEXT("화면 모드는 패키지 빌드에서만 바뀐다 (에디터에서는 저장만)"), DesignW / 2.f, SetRowY((int32)ESetRow::Window) + SetRowH + 2.f, Dim, 10.f, false);
	C.TextCentered(TEXT("↑↓ 고르기 · ←→ 바꾸기 · Enter 켜기/끄기 · Esc 메뉴로"), DesignW / 2.f, SetBoxY + SetBoxH - 18.f, Dim, 11.f, false);
}
