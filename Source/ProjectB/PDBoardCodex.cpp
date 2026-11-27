// 도감 책자 (D-354, 시안 B tools/neon/codex_book_mock.py · 넘기기 tools/neon/codex_flip_mock.py).
// 펼친 메뉴 책: 왼쪽 쪽 = 고른 항목 크게(그림 · 이름 · 효과 · 상태), 오른쪽 쪽 = 12줄 목록('그림 이름 ····· 상태'),
// 오른쪽 옆면 색인 탭 넷 (칵테일 · 조커 · 상위 조커 · 보스), 쪽 넘기기 ‹ › 는 바깥 아래 귀퉁이, 닫기는 화면 오른쪽 위.
// 쪽 · 탭을 바꾸면 오른쪽 쪽 한 장이 책등을 축으로 휘며 넘어간다 (탭은 세 장이 겹쳐 빠르게). 넘어가는 장에는 쪽 내용을 그 휨대로 찍는다 (FCanvas::PW)
#include "PDBoardDraw.h"

namespace
{
const FLinearColor CdxCover = FLinearColor(FColor::FromHex(TEXT("c98a5a"))); // 가죽 겉표지 느낌의 구릿빛 테
const TCHAR* CdxTabName[4] = {TEXT("칵테일"), TEXT("조커"), TEXT("상위 조커"), TEXT("보스")};
const TCHAR* CdxTabEn[4] = {TEXT("COCKTAILS"), TEXT("JOKERS"), TEXT("UPPER JOKERS"), TEXT("BOSSES")};
const TCHAR* CdxCockCol[5] = {TEXT("ff6aaa"), TEXT("e8eeff"), TEXT("ffd26e"), TEXT("8fe9ff"), TEXT("b48cff")}; // 칵테일 고르기 · 칵테일 칸과 같은 색표
float CdxEase(float U) { U = FMath::Clamp(U, 0.f, 1.f); return U * U * (3.f - 2.f * U); }
constexpr float CdxBuyW = 210.f, CdxBuyH = 40.f, CdxBuyX = PDDraw::CdxLX + PDDraw::CdxPgW / 2.f - CdxBuyW / 2.f, CdxBuyY = PDDraw::CdxBot - 50.f; // 잠금 해제 [열기] 버튼 (왼쪽 쪽 아래 가운데, D-356)
}

bool SPDBoardWidget::CdxCanBuy(const FCdxItem& It) const { return !It.bOpen && It.bMat && !It.bGoal && MetaChips >= It.Price; }

float SPDBoardWidget::CdxFlipLen() const { return CdxFlipN > 1 ? CdxFlipFast + (CdxFlipN - 1) * CdxFlipGap : CdxFlipDur; }
bool SPDBoardWidget::CdxFlipping() const { return Clock - CdxFlipT0 < CdxFlipLen(); }
int32 SPDBoardWidget::CdxPages(int32 Tab) const { return FMath::Max(1, (UnlCount(Tab) + CdxPerPage - 1) / CdxPerPage); }

int32 SPDBoardWidget::CdxRowAt(FVector2f P) const
{
	if (CdxFlipping() || !InRect(P, CdxRX + 20.f, CdxRowY0, CdxPgW - 40.f, CdxPerPage * CdxRowH)) return -1;
	const int32 Slot = (int32)((P.Y - CdxRowY0) / CdxRowH), K = UnlockPage * CdxPerPage + Slot;
	return Slot >= 0 && Slot < CdxPerPage && K < UnlCount(UnlockTab) ? K : -1;
}

SPDBoardWidget::FCdxView SPDBoardWidget::CdxNow() const
{
	FCdxView V;
	V.Tab = UnlockTab; V.Page = UnlockPage;
	const int32 First = UnlockPage * CdxPerPage, Last = FMath::Min(UnlCount(UnlockTab), First + CdxPerPage) - 1;
	const int32 Hov = CdxShop() ? -1 : CdxRowAt(Mouse); // 잠금 해제는 마우스로 바뀌지 않게 (고른 줄의 [열기] 버튼을 누르러 가는 동안)
	V.Pick = Hov >= 0 ? Hov : CdxPick >= First && CdxPick <= Last ? CdxPick : First;
	return V;
}

void SPDBoardWidget::CdxGo(int32 Tab, int32 Page)
{
	Tab = (Tab % CdxTabs() + CdxTabs()) % CdxTabs();
	Page = FMath::Clamp(Page, 0, CdxPages(Tab) - 1);
	if (Tab == UnlockTab && Page == UnlockPage) return;
	CdxOld = CdxNow();
	const bool bTab = Tab != UnlockTab;
	CdxFlipDir = bTab ? (Tab > UnlockTab ? 1 : -1) : (Page > UnlockPage ? 1 : -1);
	CdxFlipN = bTab ? 3 : 1;
	if (bTab) { CdxTabFrom = UnlockTab; CdxTabT0 = Clock; }
	UnlockTab = Tab; UnlockPage = Page; CdxPick = -1;
	CdxNew = FCdxView{Tab, Page, Page * CdxPerPage};
	CdxFlipT0 = Clock;
	CdxLandT = Clock + CdxFlipLen();
	UiSound(EPDSound::Move);
}

void SPDBoardWidget::CdxClick(FVector2f P, bool bRight)
{
	if (bRight || InRect(P, CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH)) { if (CdxShop()) bUnlock = false; else bCodex = false; return; }
	if (CdxShop() && !CdxFlipping() && InRect(P, CdxBuyX, CdxBuyY, CdxBuyW, CdxBuyH)) // 잠금 해제 [열기] (D-356)
	{
		const int32 K = CdxNow().Pick;
		const FCdxItem It = CdxItem(UnlockTab, K);
		if (It.bOpen) return;
		const bool bOk = UnlockTab == 0 ? BuyCharm(K) : UnlockTab == 1 ? BuyJoker(K) : BuyUpper(K);
		if (!bOk) UiSound(EPDSound::Denied);
		return;
	}
	for (int32 I = 0; I < CdxTabs(); ++I) if (InRect(P, CdxIdxX - 4.f, CdxIdxY(I), CdxIdxW + 14.f, CdxIdxH)) { CdxGo(I, 0); return; } // 옆면 탭
	for (int32 Dir = -1; Dir <= 1; Dir += 2) if (InRect(P, CdxNavX(Dir), CdxNavY, CdxNavW, CdxNavH)) { CdxGo(UnlockTab, UnlockPage + Dir); return; } // ‹ ›
	const int32 K = CdxRowAt(P);
	if (K >= 0) { CdxPick = K; UiSound(EPDSound::Move); } // 줄 클릭 = 왼쪽에 고정
	if (K >= 0 && bDevMode && !CdxShop()) // 개발자 모드 (D-485): 칵테일 = 켜기/끄기, 조커 · 상위 조커 = 도감을 닫고 판에 놓기
	{
		if (Game.Phase != EPDPhase::Build && Game.Phase != EPDPhase::Wave) { UiSound(EPDSound::Denied); return; }
		if (UnlockTab == 0) { Game.DevToggleCharm(K); UiSound(EPDSound::Burst); }
		else if (UnlockTab == 1 || UnlockTab == 2) { DevJoker = UnlockTab == 1 ? K : UpperBase + K; bCodex = false; }
	}
}

// 항목 정보 (예전 도감 D-265 / D-283 / D-337 과 같은 규칙): 모르는 조커 · 상위 조커 · 보스는 ???
SPDBoardWidget::FCdxItem SPDBoardWidget::CdxItem(int32 Tab, int32 K) const
{
	const FPDGame& G = Game;
	const bool bShop = CdxShop(); // 잠금 해제: 이름 · 효과는 다 보이고, 오른쪽 = 열림 · 목표 · 재료 먼저 · 가격 (D-356)
	FCdxItem It;
	if (Tab == 0)
	{
		It.Name = CharmName(K); It.Desc = CharmDesc(K);
		It.bOpen = It.bKnown = (G.CharmUnlocked >> K) & 1u;
		It.bHave = G.Has((EPDCharm)K);
		It.Price = CharmPrice(K); It.bGoal = CharmGoal(K) >= 0; It.bMat = !It.bGoal;
		It.Note = It.bGoal ? TrF(TEXT("목표: %s"), GoalText(CharmGoal(K))) : FString(Tr(TEXT("연 칵테일만 5웨이브 보상에 나온다")));
		if (!It.bOpen && !bShop) It.Note = TrF(TEXT("잠금 해제에서 %d칩"), CharmPrice(K)); // 잠긴 것도 효과는 보여 준다 (D-283)
		It.Right = It.bHave ? FString(Tr(TEXT("가진 칵테일"))) : It.bOpen ? FString(Tr(TEXT("열림"))) : FString(Tr(TEXT("잠김")));
		if (bShop) { It.bKnown = true; It.bHave = false; It.Right = It.bOpen ? FString(Tr(TEXT("열림"))) : It.bGoal ? FString(Tr(TEXT("목표"))) : FString::FromInt(It.Price); }
	}
	else if (Tab == 1)
	{
		It.bKnown = bShop || ((G.Discovered >> K) & 1u);
		It.bOpen = G.JokerOpen(K); It.Price = JokerPrice(K);
		const float Tier = JokerTier((EPDJoker)K);
		const FString TierS = FMath::IsNearlyEqual(Tier, FMath::RoundToFloat(Tier)) ? FString::Printf(TEXT("×%.0f"), Tier) : FString::Printf(TEXT("×%.1f"), Tier);
		It.Name = It.bKnown ? FString(JokerName(K)) : FString(TEXT("???"));
		It.Desc = It.bKnown ? FString(BaseJokerAbility(K)) : FString(Tr(TEXT("만들어 보면 열린다")));
		It.Note = HandName(CaptainHand((EPDJoker)K)); // 줄 아래 작은 글: 대장 족보 (레시피는 몰라도 보인다)
		It.Right = It.bKnown ? TierS : FString(TEXT("?"));
		if (bShop) It.Right = It.bOpen ? FString(Tr(TEXT("열림"))) : FString::FromInt(It.Price);
	}
	else if (Tab == 2)
	{
		const FPDUpperDef& D = UpperDef(K);
		It.bKnown = bShop || ((G.UpperFound >> K) & 1u);
		It.bOpen = G.UpperOpen(K); It.Price = UpperPrice(K); It.bMat = G.JokerOpen((int32)D.A) && G.JokerOpen((int32)D.B);
		It.Name = It.bKnown ? FString(UpperName(K)) : FString(TEXT("???"));
		It.Desc = It.bKnown ? FString(JokerAbility(UpperBase + K)) : FString(Tr(TEXT("만들면 열림")));
		It.Note = It.bKnown ? FString::Printf(TEXT("%s + %s"), JokerName((int32)D.A), JokerName((int32)D.B)) : TrF(TEXT("%s + ?"), JokerName((int32)D.A));
		It.Right = It.bKnown ? FString(Tr(TEXT("발견"))) : FString(TEXT("?"));
		if (bShop) It.Right = It.bOpen ? FString(Tr(TEXT("열림"))) : !It.bMat ? FString(Tr(TEXT("재료 먼저"))) : FString::FromInt(It.Price);
	}
	else
	{
		const EPDEnemy T = (EPDEnemy)((int32)EPDEnemy::ShieldBoss + K);
		const bool bSeen = (G.BossSeen >> K) & 1u, bKill = (G.BossKilled >> K) & 1u;
		It.bKnown = It.bOpen = bSeen; It.bHave = bKill;
		It.Name = bSeen ? FString(BossName(T)) : FString(TEXT("???"));
		It.Desc = bSeen ? FString(BossSkillText(T)) : FString(Tr(TEXT("아직 만나지 못한 보스")));
		It.Note = IsFinalBoss(T) ? FString(Tr(TEXT("모드 최종 보스"))) : FString(Tr(TEXT("보스")));
		It.Right = bKill ? FString(Tr(TEXT("처치"))) : bSeen ? FString(Tr(TEXT("만남"))) : FString(TEXT("?"));
	}
	return It;
}

// 한 쪽의 내용 (책 틀 없이): Side 0 = 왼쪽 쪽 (V.Pick 크게), 1 = 오른쪽 쪽 (V.Tab · V.Page 목록). 넘어가는 장도 이걸 PW 로 휘어 찍는다
void SPDBoardWidget::PaintCdxPage(FCanvas& C, int32 Side, const FCdxView& V) const
{
	const FPDGame& G = Game;
	const FLinearColor Gold = Hex(TEXT("ffd26e")), Pink = Hex(TEXT("ff6aaa")), Cyan = Hex(TEXT("6edcff")), Mint = Hex(TEXT("8affc4")), Dim = Hex(TEXT("5f6878")), Fg = Hex(TEXT("dce4f0")), Lav = Hex(TEXT("a896e1"));
	// 칵테일 잔 (고르기 화면 D-286 과 같은 모양): 0 마티니 1 하이볼 2 온더록. (GX, BY) = 바닥 가운데, S = 배율
	auto Glass = [&](int32 Kind, float GX, float BY, float S, const FLinearColor& Col, float B)
	{
		auto Q = [&](float X, float Y) { return FVector2f(GX + X * S, BY + Y * S); };
		const FLinearColor Cb = WithAlpha(Col, B), Liq = WithAlpha(Col, 0.5f * B), Wh(1.f, 1.f, 1.f, 0.6f * B);
		const float Th = FMath::Clamp(0.6f + S * 0.5f, 1.f, 1.9f);
		if (Kind == 0)
		{
			C.Glow({Q(-26.f, -70.f), Q(26.f, -70.f), Q(0.f, -36.f), Q(-26.f, -70.f)}, Cb, Th, 0.5f * B);
			C.Glow({Q(0.f, -36.f), Q(0.f, -4.f)}, Cb, Th * 0.9f, 0.4f * B);
			C.Glow({Q(-16.f, -2.f), Q(16.f, -2.f)}, Cb, Th * 0.9f, 0.4f * B);
			for (int32 J = 0; J < 4; ++J) { const float Hw = 22.f * (1.f - J * 6.f / 30.f); C.Lines({Q(-Hw, -64.f + J * 6.f), Q(Hw, -64.f + J * 6.f)}, Liq, 1.f); }
		}
		else if (Kind == 1)
		{
			C.Glow({Q(-18.f, -78.f), Q(-16.f, -2.f), Q(16.f, -2.f), Q(18.f, -78.f)}, Cb, Th, 0.5f * B);
			C.Lines({Q(6.f, -90.f), Q(-4.f, -30.f)}, Wh, 1.f);
			for (int32 J = 0; J < 6; ++J) C.Lines({Q(-15.f, -62.f + J * 10.f), Q(15.f, -62.f + J * 10.f)}, Liq, 1.f);
		}
		else
		{
			C.Glow({Q(-24.f, -46.f), Q(-22.f, -2.f), Q(22.f, -2.f), Q(24.f, -46.f)}, Cb, Th, 0.5f * B);
			for (int32 J = 0; J < 3; ++J) C.Lines({Q(-21.f, -34.f + J * 10.f), Q(21.f, -34.f + J * 10.f)}, Liq, 1.f);
			C.Lines({Q(-12.f, -30.f), Q(-2.f, -38.f), Q(8.f, -30.f), Q(-2.f, -22.f), Q(-12.f, -30.f)}, Wh, 1.f);
		}
	};
	auto Boss = [&](int32 K, float X, float Y, float R, float A) { const EPDEnemy T = (EPDEnemy)((int32)EPDEnemy::ShieldBoss + K); C.CardSoldier(SoldierShape(T), FVector2f(X, Y + R * 1.3f), R, 0.f, WithAlpha(EnemyColor(T), A)); };
	if (Side == 1) // ---- 오른쪽 쪽: 머리글 + 12줄
	{
		const float CX = CdxRX + CdxPgW / 2.f;
		C.TextCentered(CdxTabName[V.Tab], CX, CdxBY + 64.f, Pink, 28.f, true);
		int32 NOpen = 0;
		if (V.Tab == 0) for (int32 J = 0; J < (int32)EPDCharm::Count; ++J) NOpen += (G.CharmUnlocked >> J) & 1u;
		const FString Sub = V.Tab == 0 ? TrF(TEXT("칵테일 %d / %d 열림"), NOpen, (int32)EPDCharm::Count) : V.Tab == 1 ? TrF(TEXT("%d / 10 발견"), FMath::CountBits(G.Discovered)) : V.Tab == 2 ? TrF(TEXT("상위 조커 %d / 30 발견"), FMath::CountBits(G.UpperFound)) : TrF(TEXT("보스 %d / %d 처치"), FMath::CountBits(G.BossKilled), BossKinds);
		C.TextCentered(CdxShop() ? FString::Printf(TEXT("%s  ·  %d"), CdxTabEn[V.Tab], UnlCount(V.Tab)) : FString::Printf(TEXT("%s  ·  %s"), CdxTabEn[V.Tab], *Sub), CX, CdxBY + 88.f, WithAlpha(Pink, 0.8f), 11.f, true);
		for (const float S : {-1.f, 1.f}) C.Glow({FVector2f(CX + S * 70.f, CdxBY + 56.f), FVector2f(CX + S * 150.f, CdxBY + 56.f)}, WithAlpha(Pink, 0.8f), 1.1f, 0.4f);
		const bool bLive = !C.PW; // 넘어가는 장 위에는 마우스 줄 표시를 하지 않는다
		const int32 Hov = bLive && !CdxFlipping() ? CdxRowAt(Mouse) : -1;
		const float X0 = CdxRX + 36.f, X1 = CdxRX + CdxPgW - 36.f;
		for (int32 Slot = 0; Slot < CdxPerPage; ++Slot)
		{
			const int32 K = V.Page * CdxPerPage + Slot;
			if (K >= UnlCount(V.Tab)) break;
			const FCdxItem It = CdxItem(V.Tab, K);
			const float Y = CdxRowY0 + Slot * CdxRowH, Base = Y + 20.f;
			const bool bH = K == Hov, bSel = bLive && K == V.Pick && (CdxShop() || Hov < 0);
			const float IA = It.bKnown ? 1.f : 0.35f;
			if (V.Tab == 0) { if (!CocktailArt(C, K, X0 + 12.f, Y + 30.f, 34.f, IA)) Glass(K % 3, X0 + 12.f, Y + 30.f, 0.3f, Hex(CdxCockCol[K % 5]), IA); } // 잔 그림 (D-424)
			else if (V.Tab == 3) Boss(K, X0 + 12.f, Y + 6.f, 8.f, IA);
			else C.JokerTower(V.Tab == 1 ? K : UpperBase + K, X0 + 12.f, Y + 18.f, 11.f, It.bKnown ? 0.95f : 0.35f, 0.f);
			const bool bCan = CdxShop() && CdxCanBuy(It);
			const FLinearColor NameBase = CdxShop() ? (It.bOpen ? Cyan : bCan ? Gold : Dim) : It.bHave ? Gold : It.bKnown ? Cyan : Dim;
			const FLinearColor NameCol = bH ? HotCol(NameBase) : NameBase; // 마우스 올림 = 같은 색 밝게 (D-382)
			const bool bSub = V.Tab != 0; // 조커 · 상위 조커 · 보스는 이름 밑에 작은 글 (대장 족보 · 재료 · 보스 종류)
			const float NY = bSub ? Base - 4.f : Base;
			C.Text(It.Name, X0 + 34.f, NY, NameCol, 14.f, 0, true, 230.f);
			if (bSub) C.Text(It.Note, X0 + 34.f, NY + 14.f, Dim, 10.f, 0, false, 260.f);
			const FLinearColor RC = CdxShop() ? (It.bOpen ? Mint : bCan ? Gold : Dim) : It.bHave ? Gold : It.bKnown ? Mint : Dim;
			C.Text(It.Right, X1, NY, RC, 13.f, 2, true);
			const float NameW = FMath::Min(230.f, FCanvas::Measure(TrS(It.Name), FCanvas::Font(14.f, true)).X), RW = FCanvas::Measure(TrS(It.Right), FCanvas::Font(13.f, true)).X;
			for (float Dx = X0 + 40.f + NameW; Dx < X1 - RW - 8.f; Dx += 6.f) C.Box(Dx, NY - 4.f, 1.2f, 1.2f, WithAlpha(Dim, 0.6f)); // 점선 리더
			if (bH || bSel) C.Glow({FVector2f(X0 + 34.f, Y + CdxRowH - 3.f), FVector2f(X1, Y + CdxRowH - 3.f)}, WithAlpha(Pink, bH ? 1.f : 0.5f), 1.1f, bH ? 0.5f : 0.2f);
		}
		C.TextCentered(CdxPages(V.Tab) > 1 ? FString::Printf(TEXT("— %d / %d —"), V.Page + 1, CdxPages(V.Tab)) : FString(), CX, CdxNavY + CdxNavH / 2.f, Dim, 12.f, false);
		return;
	}
	// ---- 왼쪽 쪽: 고른 항목 크게
	if (V.Pick < 0 || V.Pick >= UnlCount(V.Tab)) return;
	const FCdxItem It = CdxItem(V.Tab, V.Pick);
	const float CX = CdxLX + CdxPgW / 2.f;
	const FLinearColor Big = It.bKnown ? FLinearColor::White : Dim;
	// 잠금 해제 [열기] 버튼 (D-356): 왼쪽 쪽 아래 가운데. 열린 것 = '열림', 목표로 여는 칵테일 = '목표', 재료가 안 열린 상위 조커 = '재료 조커 먼저'
	auto BuyBtn = [&]()
	{
		const bool bCan = CdxCanBuy(It), bH = bCan && !C.PW && InRect(Mouse, CdxBuyX, CdxBuyY, CdxBuyW, CdxBuyH);
		const FLinearColor Bc = It.bOpen ? Mint : bCan ? Gold : Dim;
		if (bCan) C.BtnFill(CdxBuyX, CdxBuyY, CdxBuyW, CdxBuyH, RadPill(CdxBuyH), Gold, bH);
		C.Glow(FCanvas::RoundRectPoints(CdxBuyX, CdxBuyY, CdxBuyW, CdxBuyH, CdxBuyH / 2.f), WithAlpha(Bc, 0.9f), bH ? 1.8f : 1.3f, bCan ? 0.6f : 0.25f);
		const FString Top = It.bOpen ? FString(Tr(TEXT("열림"))) : It.bGoal ? FString(Tr(TEXT("목표"))) : !It.bMat ? FString(Tr(TEXT("재료 조커 먼저"))) : TrF(TEXT("%d칩"), It.Price);
		C.TextCentered(Top, CdxBuyX + CdxBuyW / 2.f, CdxBuyY + 15.f, bH ? HotCol(Bc) : Bc, 15.f, true, CdxBuyW - 16.f);
		if (!It.bOpen && It.bMat && !It.bGoal) C.TextCentered(bCan ? FString(Tr(TEXT("클릭해서 열기"))) : FString(Tr(TEXT("칩이 모자란다"))), CdxBuyX + CdxBuyW / 2.f, CdxBuyY + 31.f, bCan ? Pink : Dim, 10.f, true);
	};
	// 상점의 조커 쪽은 레시피 밑 안내문이 [열기] 버튼에 겹치지 않게 그림부터 위로 올린다 (D-368)
	const float Lift = V.Tab == 1 && CdxShop() ? 26.f : 0.f;
	float Y = CdxBY + 320.f - Lift; // 그림 아래 (이름 기준선)
	if (V.Tab == 0) { if (!CocktailArt(C, V.Pick, CX, CdxBY + 270.f, 230.f, It.bKnown ? 1.f : 0.4f)) Glass(V.Pick % 3, CX, CdxBY + 270.f, 2.f, Hex(CdxCockCol[V.Pick % 5]), It.bKnown ? 1.f : 0.4f); } // 잔 그림 (D-424)
	else if (V.Tab == 3) Boss(V.Pick, CX, CdxBY + 120.f, 48.f, It.bKnown ? 1.f : 0.25f);
	else C.JokerTower(V.Tab == 1 ? V.Pick : UpperBase + V.Pick, CX, CdxBY + 180.f - Lift, 54.f, It.bKnown ? 1.f : 0.35f, 0.5f + 0.5f * FMath::Sin(Clock * 3.f));
	NeonText(C, It.Name, CX, Y, Big, 30.f, 1, It.bKnown ? 1.f : 0.5f);
	Y += 26.f;
	if (V.Tab == 1) // 조커: 대장 · 위력, 지휘, 레시피 알약 (D-115)
	{
		const float Tier = JokerTier((EPDJoker)V.Pick);
		const FString TierS = FMath::IsNearlyEqual(Tier, FMath::RoundToFloat(Tier)) ? FString::Printf(TEXT("×%.0f"), Tier) : FString::Printf(TEXT("×%.1f"), Tier);
		C.TextCentered(TrF(TEXT("%s 대장 · 위력 %s"), HandName(CaptainHand((EPDJoker)V.Pick)), *TierS), CX, Y, Gold, 13.f, true, CdxPgW - 60.f);
		Y += 20.f;
	}
	else if (V.Tab == 2 || V.Tab == 3) { C.TextCentered(It.Note, CX, Y, V.Tab == 2 ? Gold : Dim, 13.f, true, CdxPgW - 60.f); Y += 20.f; }
	C.Lines({FVector2f(CX - 170.f, Y), FVector2f(CX + 170.f, Y)}, WithAlpha(Lav, 0.5f), 1.f);
	Y += 30.f;
	Wrap2(C, It.Desc, CX - 220.f, Y, Fg, 15.f, 440.f);
	Y += 46.f;
	if (V.Tab == 0) { C.TextCentered(It.Note, CX, Y, Dim, 12.f, false, CdxPgW - 60.f); Y += 26.f; }
	if (V.Tab == 1)
	{
		if (It.bKnown) { C.TextCentered(JokerCommand(V.Pick), CX, Y, JokerPink, 12.f, true, CdxPgW - 60.f); Y += 30.f; }
		// 레시피: 족보 알약 이어 붙이기 (3개 · 2개)
		auto Row = [&](const TCHAR* Lb, const TArray<EPDHand>& Hs, float Ry)
		{
			float W = FCanvas::Measure(TrS(Lb), FCanvas::Font(11.f, false)).X + 10.f;
			TArray<float> Ws;
			for (EPDHand H : Hs) { Ws.Add(FCanvas::Measure(TrS(HandShort(H)), FCanvas::Font(12.f, true)).X + 22.f); W += Ws.Last() + 20.f; }
			float X = CX - (W - 20.f) / 2.f;
			C.Text(Lb, X, Ry + 4.f, Dim, 11.f);
			X += FCanvas::Measure(TrS(Lb), FCanvas::Font(11.f, false)).X + 10.f;
			for (int32 I = 0; I < Hs.Num(); ++I)
			{
				C.Glow(FCanvas::RoundRectPoints(X, Ry - 11.f, Ws[I], 22.f, 11.f), WithAlpha(Cyan, 0.85f), 1.1f, 0.4f);
				C.TextCentered(HandShort(Hs[I]), X + Ws[I] / 2.f, Ry, Cyan, 12.f, true);
				X += Ws[I];
				if (I + 1 < Hs.Num()) C.TextCentered(TEXT("+"), X + 10.f, Ry, Dim, 12.f, true);
				X += 20.f;
			}
		};
		const auto& R3 = JokerRecipe((EPDJoker)V.Pick);
		const auto& R2 = JokerRecipe2((EPDJoker)V.Pick);
		Row(TEXT("3개"), {R3[0], R3[1], R3[2]}, Y);
		Row(TEXT("2개"), {R2[0], R2[1]}, Y + 34.f);
		Y += 70.f;
		C.TextCentered(TEXT("족보 3개(또는 2개)를 골라 레시피와 맞으면 조커 (판 어디에 있어도)"), CX, Y, Dim, 11.f, false, CdxPgW - 60.f);
		if (CdxShop()) BuyBtn();
		return;
	}
	if (V.Tab == 2) { C.TextCentered(TEXT("서로 다른 조커 둘을 맞닿게 놓으면 상위 조커 (★ 대표 11종은 둘 다 조커 레벨 10)"), CX, CdxBot - 90.f, Dim, 11.f, false, CdxPgW - 60.f); }
	if (V.Tab == 3) { C.TextCentered(TEXT("30 · 60 · 100 모드는 보스 차례가 늘 같다. 무한 모드는 100웨이브 뒤 판마다 다르다"), CX, CdxBot - 90.f, Dim, 11.f, false, CdxPgW - 60.f); }
	if (CdxShop()) { BuyBtn(); return; }
	// 상태 알약 (가진 칵테일 · 열림 · 잠김 / 발견 / 처치 · 만남)
	const FLinearColor SC = It.bHave ? Gold : It.bKnown ? Mint : Dim;
	const float PWd = FCanvas::Measure(TrS(It.Right), FCanvas::Font(14.f, true)).X + 40.f;
	C.Glow(FCanvas::RoundRectPoints(CX - PWd / 2.f, Y - 4.f, PWd, 30.f, 15.f), WithAlpha(SC, 0.9f), 1.3f, 0.5f);
	C.TextCentered(It.Right, CX, Y + 11.f, SC, 14.f, true);
}

// 넘어가는 장 하나 (U 0 → 1 = 오른쪽 쪽이 책등을 넘어 왼쪽으로). 앞면 = Front 의 오른쪽 쪽, 뒷면 = Back 의 왼쪽 쪽.
// 장의 각 줄은 바깥으로 갈수록 각도가 앞서(전반) / 뒤처져(후반) 종이가 말리고, 위 바깥 모서리가 먼저 들린다. 많이 들린 곳일수록 위아래로 커진다(원근)
void SPDBoardWidget::PaintCdxSheet(FCanvas& C, float U, const FCdxView& Front, const FCdxView& Back) const
{
	if (U <= 0.f || U >= 1.f) return;
	const float S = FMath::Sin(PI * U), Ca = FMath::Abs(FMath::Cos(PI * U)), Face = 0.35f + 0.65f * Ca;
	const float CY = (CdxTop + CdxBot) / 2.f;
	const float Lead = 0.55f * S * (U < 0.5f ? 1.f : -1.f) * FMath::Pow(1.f - FMath::Abs(2.f * U - 1.f), 0.3f);
	auto Sheet = [=](float Q, float Y) // 장 위 점 (Q = 책등에서 잰 거리, Y) → 화면
	{
		const float T = FMath::Clamp(Q / CdxPgW, 0.f, 1.2f), Tilt = 0.22f * S * ((CdxBot - Y) / (CdxBot - CdxTop) - 0.5f);
		float X = 0.f, Depth = 0.f;
		constexpr int32 N = 10;
		for (int32 I = 0; I < N; ++I)
		{
			const float A = PI * U + (Lead + Tilt) * FMath::Pow((I + 0.5f) / N * T, 1.6f);
			X += FMath::Cos(A) * Q / N;
			Depth += FMath::Sin(A) * Q / N;
		}
		return FVector2f(CdxSpX + X, CY + (Y - CY) * (1.f + 0.10f * Depth / CdxPgW));
	};
	auto FrontMap = [=](FVector2f P) { return Sheet(P.X - CdxSpX, P.Y); }; // 오른쪽 쪽 좌표 → 장
	auto BackMap = [=](FVector2f P) { return Sheet(CdxSpX - P.X, P.Y); };  // 왼쪽 쪽 좌표 → 장 (뒷면)
	// 그림자: 장 아래 드러나는 쪽, 장 바깥 끝 쪽으로 진해짐
	{
		const float Ex = Sheet(CdxPgW, CY).X;
		const int32 L = C.NextLayer();
		for (int32 K = 0; K < 32; ++K)
		{
			const float X = Ex + (U < 0.5f ? K : -K - 1) * 3.f;
			if (X < CdxLX || X > CdxRX + CdxPgW) break;
			C.BoxAt(L, X, CdxTop, 3.f, CdxBot - CdxTop, FLinearColor(0.f, 0.f, 0.f, 0.35f * S * FMath::Square(1.f - K / 32.f)));
		}
		C.NextLayer();
	}
	// 장 면 채움 (띠 18개, 띠마다 기울기로 비튼 상자)
	C.PW = FrontMap;
	{
		const int32 L = C.NextLayer();
		// 쪽 바탕(06060b)과 똑같은 색 (넘어가도 면 색은 바뀌지 않는다, 사용자 2026-11-16)
		const FLinearColor Fill = Hex(TEXT("06060b"), 1.f);
		constexpr int32 NS = 18;
		for (int32 I = 0; I < NS; ++I) C.BoxAt(L, CdxSpX + CdxPgW * I / NS, CdxTop, CdxPgW / NS + 1.2f, CdxBot - CdxTop, Fill);
		C.NextLayer();
	}
	// 그 면의 쪽 내용 (면 밝기로 흐리게)
	C.FadeK = Face;
	if (U < 0.5f) PaintCdxPage(C, 1, Front);
	else { C.PW = BackMap; PaintCdxPage(C, 0, Back); C.PW = FrontMap; }
	C.FadeK = 1.f;
	// 테두리 (휜 모양) + 접힌 바깥 끝 흰 빛줄
	const FLinearColor Lav = Hex(TEXT("a896e1"));
	C.Lines(FCanvas::RoundRectPoints(CdxRX - 6.f, CdxTop, CdxPgW + 6.f, CdxBot - CdxTop, 8.f), WithAlpha(Lav, 0.5f + 0.4f * Face), 1.2f);
	C.Lines({FVector2f(CdxRX + CdxPgW - 3.f, CdxTop + 4.f), FVector2f(CdxRX + CdxPgW - 3.f, CdxBot - 4.f)}, FLinearColor(1.f, 1.f, 1.f, 0.2f + 0.45f * S), 1.f);
	C.PW = nullptr;
}

void SPDBoardWidget::PaintCodexBook(FCanvas& C) const
{
	const FLinearColor Pink = Hex(TEXT("ff6aaa")), Cyan = Hex(TEXT("6edcff")), Lav = Hex(TEXT("a896e1")), Fg = Hex(TEXT("dce4f0")), Dim = Hex(TEXT("5f6878"));
	C.Box(0.f, 0.f, DesignW, DesignH, Hex(TEXT("06060b"), 1.f));
	// 겉표지 (두 겹) + 두 쪽 + 책등 (꿰맨 점선)
	C.Glow(FCanvas::RoundRectPoints(CdxBX, CdxBY, CdxBW, CdxBH, 18.f), WithAlpha(CdxCover, 0.9f), 2.f, 0.6f);
	C.Lines(FCanvas::RoundRectPoints(CdxBX + 6.f, CdxBY + 6.f, CdxBW - 12.f, CdxBH - 12.f, 14.f), WithAlpha(CdxCover, 0.45f), 0.8f);
	for (const float X : {CdxLX, CdxRX - 6.f}) C.Glow(FCanvas::RoundRectPoints(X, CdxTop, CdxPgW + 6.f, CdxBot - CdxTop, 8.f), WithAlpha(Lav, 0.55f), 1.2f, 0.25f);
	// 쪽 두께: 넘긴 쪽 수만큼 왼쪽, 남은 쪽만큼 오른쪽에 얇은 선
	const int32 Pages = CdxPages(UnlockTab), NL = FMath::Min(4, 1 + UnlockPage), NR = FMath::Min(4, Pages - UnlockPage);
	for (int32 K = 0; K < NL; ++K) C.Lines({FVector2f(CdxLX - 4.f - K * 3.f, CdxTop + 6.f + K), FVector2f(CdxLX - 4.f - K * 3.f, CdxBot - 6.f - K)}, WithAlpha(Lav, 0.35f), 0.8f);
	for (int32 K = 0; K < NR; ++K) C.Lines({FVector2f(CdxRX + CdxPgW + 4.f + K * 3.f, CdxTop + 6.f + K), FVector2f(CdxRX + CdxPgW + 4.f + K * 3.f, CdxBot - 6.f - K)}, WithAlpha(Lav, 0.35f), 0.8f);
	// 옆면 색인 탭 (고른 탭은 0.2초 동안 밀려 나옴)
	const float TabE = CdxEase((Clock - CdxTabT0) / 0.2f);
	for (int32 I = 0; I < CdxTabs(); ++I)
	{
		const bool bOn = I == UnlockTab, bWas = I == CdxTabFrom && I != UnlockTab;
		const float W = CdxIdxW + (bOn ? 10.f * TabE : bWas ? 10.f * (1.f - TabE) : 0.f), Y0 = CdxIdxY(I), H = CdxIdxH, X = CdxIdxX;
		const bool bH = InRect(Mouse, X - 4.f, Y0, W + 14.f, H);
		const FLinearColor Col = bOn ? Pink : bH ? Fg : Lav;
		if (bOn) C.Box(X - 2.f, Y0 + 3.f, W - 6.f, H - 6.f, Hex(TEXT("06060b"), 1.f)); // 고른 탭은 책 테와 이어진 듯 (경계선 가림)
		C.Glow({FVector2f(X, Y0), FVector2f(X + W - 10.f, Y0), FVector2f(X + W, Y0 + 10.f), FVector2f(X + W, Y0 + H - 10.f), FVector2f(X + W - 10.f, Y0 + H), FVector2f(X, Y0 + H)}, WithAlpha(Col, bOn ? 1.f : 0.6f), bOn ? 1.8f : 1.2f, bOn ? 0.8f : 0.25f);
		C.TextCentered(CdxTabName[I], X + W / 2.f + 2.f, Y0 + H / 2.f, bOn ? FLinearColor::White : Fg, CdxTabName[I][3] ? 12.f : 14.f, true, W - 6.f);
	}
	// 책등
	const float Land = FMath::Clamp(1.f - (Clock - CdxLandT) / 0.15f, 0.f, 1.f); // 장이 내려앉으면 한 번 빛남
	C.Glow({FVector2f(CdxSpX, CdxBY + 14.f), FVector2f(CdxSpX, CdxBY + CdxBH - 14.f)}, WithAlpha(CdxCover, 0.8f), 1.4f, 0.4f);
	for (float Y = CdxBY + 30.f; Y < CdxBY + CdxBH - 24.f; Y += 14.f) C.Lines({FVector2f(CdxSpX - 1.f, Y), FVector2f(CdxSpX - 1.f, Y + 6.f)}, WithAlpha(CdxCover, 0.5f), 1.f);
	if (Land > 0.f && Clock >= CdxLandT) C.Glow({FVector2f(CdxSpX, CdxTop), FVector2f(CdxSpX, CdxBot)}, FLinearColor(1.f, 1.f, 1.f, Land), 1.4f, Land);
	// 쪽 내용 + 넘어가는 장
	const float Ft = Clock - CdxFlipT0;
	if (CdxFlipping())
	{
		const bool bNext = CdxFlipDir > 0;
		PaintCdxPage(C, 0, bNext ? CdxOld : CdxNew); // 넘는 쪽으로 덮이기 전까지 옛 쪽
		PaintCdxPage(C, 1, bNext ? CdxNew : CdxOld);
		for (int32 K = 0; K < CdxFlipN; ++K)
		{
			const float E = CdxFlipN > 1 ? CdxEase((Ft - K * CdxFlipGap) / CdxFlipFast) : CdxEase(Ft / CdxFlipDur);
			const float U = bNext ? E : 1.f - E;
			PaintCdxSheet(C, U, bNext ? CdxOld : CdxNew, bNext ? CdxNew : CdxOld);
		}
	}
	else
	{
		const FCdxView V = CdxNow();
		PaintCdxPage(C, 0, V);
		PaintCdxPage(C, 1, V);
		// › / ‹ 위에 마우스 = 오른쪽 / 왼쪽 쪽 모서리가 살짝 들림 (넘길 수 있다는 표시)
		if (UnlockPage < CdxPages(UnlockTab) - 1 && InRect(Mouse, CdxNavX(1), CdxNavY, CdxNavW, CdxNavH)) PaintCdxSheet(C, 0.04f, V, V);
		if (UnlockPage > 0 && InRect(Mouse, CdxNavX(-1), CdxNavY, CdxNavW, CdxNavH)) PaintCdxSheet(C, 0.96f, V, V);
	}
	// 쪽 넘기기 ‹ › (바깥 아래 귀퉁이)
	for (int32 Dir = -1; Dir <= 1; Dir += 2)
	{
		const bool bOk = Dir < 0 ? UnlockPage > 0 : UnlockPage < CdxPages(UnlockTab) - 1;
		const float X = CdxNavX(Dir);
		const bool bH = bOk && InRect(Mouse, X, CdxNavY, CdxNavW, CdxNavH);
		if (bOk) C.BtnFill(X, CdxNavY, CdxNavW, CdxNavH, RadPill(CdxNavH), bH ? Pink : Cyan, bH);
		C.Glow(FCanvas::RoundRectPoints(X, CdxNavY, CdxNavW, CdxNavH, CdxNavH / 2.f), WithAlpha(bH ? Pink : Cyan, bOk ? 0.85f : 0.2f), bH ? 1.6f : 1.2f, bOk ? 0.4f : 0.f);
		C.TextCentered(Dir < 0 ? TEXT("‹") : TEXT("›"), X + CdxNavW / 2.f, CdxNavY + CdxNavH / 2.f, bOk ? Fg : Dim, 18.f, true);
	}
	C.Text(CdxShop() ? TEXT("Tab: 다음 판 · ←→: 쪽 · Esc · 오른쪽 클릭: 닫기") : TEXT("Tab: 다음 판 · ←→: 쪽 · J / Esc · 오른쪽 클릭: 닫기"), DesignW - 16.f, DesignH - 10.f, Dim, 10.f, 2);
	if (CdxShop()) // 잠금 해제: 가진 칩 (닫기 왼쪽)
	{
		const FLinearColor Gold = Hex(TEXT("ffd26e"));
		const FString S = TrF(TEXT("칩 %s"), *CommaNum(MetaChips));
		const float PW2 = 46.f + FCanvas::Measure(S, FCanvas::Font(13.f, true)).X, PX = CdxCloseX - 14.f - PW2, PY = CdxCloseY;
		C.Glow(FCanvas::RoundRectPoints(PX, PY, PW2, CdxCloseH, CdxCloseH / 2.f), Gold, 1.4f, 0.5f);
		C.Shape(EPDSuit::Diamond, PX + 17.f, PY + CdxCloseH / 2.f, 6.f, Gold, 1.3f, 0.4f);
		C.Text(S, PX + 30.f, PY + CdxCloseH / 2.f + 5.f, Gold, 13.f, 0, true);
	}
	PaintCloseButton(C, CdxShop() ? Cyan : JokerPink);
}
