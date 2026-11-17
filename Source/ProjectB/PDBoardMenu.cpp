// 칸 메뉴 상자 (D-312, D-313): 판의 타워를 클릭하면 툴팁이 그 자리에 고정되며 메뉴가 된다.
// 위 = 툴팁 내용(줄여서) 또는 줄에 마우스를 올린 할 일의 전후 비교, 아래 = 할 일 줄 (합성 · 조커 · 먹이기 · 합치기 · 판매).
// 상자는 클릭한 칸이 든 족보 칸들을 피해서 처음 뜬 자리에 고정된다. 왼쪽 족보 목록으로 합성하는 길(D-199)도 그대로 있다.
#include "PDBoardDraw.h"

namespace
{
constexpr float CmW = 290.f, CmTopLines = 6.f, CmLineH = 17.f, CmRowH = 36.f; // 할 일 줄 = 알약 튜브 버튼 (D-350, 시안 A): 버튼 높이 CmBtnH + 사이 6
constexpr float CmBtnH = 30.f;
constexpr float CmTopH = 18.f + CmTopLines * CmLineH; // 위쪽 칸 높이 (줄에 마우스를 올려도 상자 크기가 안 변하게 고정)
bool HasCell(const std::vector<int>& Cs, int K) { return std::find(Cs.begin(), Cs.end(), K) != Cs.end(); }
FString Signed(float Pct) { return FString::Printf(TEXT("%s%.0f%%"), Pct >= 0.f ? TEXT("+") : TEXT(""), Pct); }
}

float SPDBoardWidget::CellMenuH(int32 RowsN) const { return CmTopH + 8.f + RowsN * CmRowH + 8.f; }
float SPDBoardWidget::CellMenuY() const { return FMath::Min(MenuPos.Y, DesignH - CellMenuH(MenuActs.Num()) - 8.f); } // 그리기 · 마우스 · 클릭이 같은 자리 (줄이 늘어도)

void SPDBoardWidget::BuildCellActs(int32 Cell, TArray<FCellAct>& Out) const
{
	Out.Reset();
	const FPDGame& G = Game;
	auto It = G.Towers.find(Cell);
	if (It == G.Towers.end()) return;
	const FPDTower& T = It->second;
	if (!T.IsJoker())
	{
		for (int32 I = 0; I < (int32)G.Synergies.size(); ++I) // 합성: 이 칸이 든 합성 되는 족보마다 (조커 칸은 빠진다, D-128)
		{
			if (!G.CanFuse(I))
			{
				// 이 칸이 든 족보인데 합성이 막혔으면 누를 수 없는 줄로 까닭을 보여 준다 (D-371)
				const FPDGame::EFuseBlock Why = G.FuseBlock(I);
				if (Why == FPDGame::EFuseBlock::NotInPlay || Why == FPDGame::EFuseBlock::None || !HasCell(G.Synergies[I].Cells, Cell)) continue;
				FCellAct A;
				A.Kind = ECellAct::NoFuse; A.A = I; A.B = (int32)Why; A.Cells = G.Synergies[I].Cells; A.Col = Hex(TEXT("5f6878"));
				A.Label = TrF(TEXT("합성 불가 · %s"), *HandLabel(G.Synergies[I]));
				A.Right = Why == FPDGame::EFuseBlock::Joker ? FString(Tr(TEXT("조커가 낌"))) : Why == FPDGame::EFuseBlock::ClownPath ? FString(Tr(TEXT("광대의 길"))) : FString(Tr(TEXT("최고 레벨")));
				Out.Add(MoveTemp(A));
				continue;
			}
			const FPDSynergy FS = FuseSyn(G, I);
			if (!HasCell(FS.Cells, Cell)) continue;
			FCellAct A;
			A.Kind = ECellAct::Fuse; A.A = I; A.Cells = FS.Cells; A.Col = Hex(TEXT("ffd26e"));
			A.Label = TrF(TEXT("합성 · %s"), *HandLabel(G.Synergies[I]));
			A.Right = TrF(TEXT("%d장"), (int32)FS.Cells.size());
			Out.Add(MoveTemp(A));
		}
		const std::vector<FPDJokerNode> Ns = G.JokerNodes(); // 조커: 이 칸이 든 재료로 만들 수 있는 조커마다 (D-313)
		for (int32 J = 0; J < (int32)EPDJoker::Count; ++J)
		{
			if (!G.JokerOpen(J)) continue;
			std::vector<EPDHand> Kinds;
			for (EPDHand H : JokerRecipe((EPDJoker)J)) Kinds.push_back(H);
			for (EPDHand H : JokerRecipe2((EPDJoker)J)) Kinds.push_back(H);
			for (const FPDJokerNode& N : Ns)
			{
				if (!HasCell(N.Cells, Cell) || std::find(Kinds.begin(), Kinds.end(), N.Kind) == Kinds.end()) continue;
				auto Fits = [&Ns](const std::vector<EPDHand>& Rec) { // 판의 재료 수로 레시피(3개 또는 2개)가 채워지나 (BestJokerPicks 는 사본을 많이 만들어 비싸다)
					if (Rec.empty()) return false;
					for (EPDHand H : Rec)
					{
						int Need = 0, Have = 0;
						for (EPDHand X : Rec) Need += X == H;
						for (const FPDJokerNode& Q : Ns) Have += Q.Kind == H;
						if (Have < Need) return false;
					}
					return true;
				};
				std::vector<EPDHand> R3, R2;
				for (EPDHand H : JokerRecipe((EPDJoker)J)) R3.push_back(H);
				for (EPDHand H : JokerRecipe2((EPDJoker)J)) R2.push_back(H);
				if (!Fits(R3) && !Fits(R2)) break;
				FCellAct A;
				A.Kind = ECellAct::Craft; A.A = J; A.Cells = N.Cells; A.Col = JokerPink;
				A.Label = TrF(TEXT("조커: %s"), (G.Discovered >> J) & 1u ? JokerName(J) : TEXT("???"));
				A.Right = FString::Printf(TEXT("×%s"), *FString::SanitizeFloat(JokerTier((EPDJoker)J)));
				Out.Add(MoveTemp(A));
				break;
			}
		}
	}
	else
	{
		for (const FPDFeed& Fd : G.FeedTargets(Cell)) // 먹이기: 이 조커에 닿은 먹일 족보마다
		{
			FCellAct A;
			A.Kind = ECellAct::Feed; A.A = Fd.Syn; A.Cells = G.Synergies[Fd.Syn].Cells; A.Col = JokerPink;
			A.Label = TrF(TEXT("먹이기 · %s"), *HandLabel(G.Synergies[Fd.Syn]));
			A.Right = TrF(TEXT("경험치 +%.0f"), Fd.Xp);
			Out.Add(MoveTemp(A));
		}
		const std::vector<FPDJokerMerge> Ms = G.JokerMerges(); // 같은 조커 합치기 / 상위 조커
		for (int32 I = 0; I < (int32)Ms.size(); ++I)
		{
			if (!HasCell(Ms[I].Cells, Cell)) continue;
			FCellAct A;
			A.Kind = ECellAct::Merge; A.A = I; A.Cells = Ms[I].Cells;
			const bool bUp = Ms[I].Upper >= 0;
			A.Col = bUp ? Hex(TEXT("ffd26e")) : JokerPink;
			A.Label = bUp ? TrF(TEXT("상위 조커: %s"), (G.UpperFound >> Ms[I].Upper) & 1u ? UpperName(Ms[I].Upper) : TEXT("???")) : TrF(TEXT("조커 합치기 · %d장"), (int32)Ms[I].Cells.size());
			Out.Add(MoveTemp(A));
		}
	}
	if (const int32 Price = G.SellPrice(Cell); Price > 0)
	{
		FCellAct A;
		A.Kind = ECellAct::Sell; A.Cells = {Cell}; A.Col = Hex(TEXT("cfd8e6"));
		A.Label = Tr(TEXT("판매"));
		A.Right = TrF(TEXT("+%d 경험치"), Price);
		Out.Add(MoveTemp(A));
	}
}

void SPDBoardWidget::OpenCellMenu(int32 Cell)
{
	if (!Game.Towers.count(Cell)) return;
	MenuCell = Cell;
	BuildCellActs(Cell, MenuActs);
	// 자리 (D-312 ④): 칸의 오른쪽 · 왼쪽 · 아래 · 위 · 대각선 중에서, 할 일에 쓰이는 칸 전부와 그 칸을 가리지 않는 곳 (아주 큰 벌점)
	TSet<int32> Avoid;
	Avoid.Add(Cell);
	for (const FCellAct& A : MenuActs) for (int K : A.Cells) Avoid.Add(K);
	const float W = CmW, H = CellMenuH(MenuActs.Num());
	const FVector2f C0 = CellPx((float)(Cell % Cols), (float)(Cell / Cols));
	const float Gp = 10.f, MidY = C0.Y + CS / 2.f - H / 2.f, MidX = C0.X + CS / 2.f - W / 2.f;
	const FVector2f Cand[10] = {{C0.X + CS + Gp, MidY}, {C0.X - Gp - W, MidY}, {MidX, C0.Y + CS + Gp}, {MidX, C0.Y - Gp - H},
		{C0.X + CS + Gp, C0.Y}, {C0.X - Gp - W, C0.Y}, {C0.X + CS + Gp, C0.Y + CS - H}, {C0.X - Gp - W, C0.Y + CS - H},
		{C0.X + CS + Gp + CS, MidY}, {C0.X - Gp - W - CS, MidY}};
	float BestScore = TNumericLimits<float>::Max();
	for (const FVector2f& Cn : Cand)
	{
		const FVector2f Pp(FMath::Clamp(Cn.X, 8.f, DesignW - W - 8.f), FMath::Clamp(Cn.Y, HudY + HudH + 4.f, DesignH - H - 8.f));
		const FBox2f Tb(Pp, Pp + FVector2f(W, H));
		float Score = 0.f;
		for (const int32 K : Avoid)
		{
			const FVector2f K0 = CellPx((float)(K % Cols), (float)(K / Cols));
			const float Ox = FMath::Max(0.f, FMath::Min(Tb.Max.X, K0.X + CS) - FMath::Max(Tb.Min.X, K0.X)), Oy = FMath::Max(0.f, FMath::Min(Tb.Max.Y, K0.Y + CS) - FMath::Max(Tb.Min.Y, K0.Y));
			Score += Ox * Oy * 100.f;
		}
		Score += FVector2f::Distance(Pp + FVector2f(W, H) / 2.f, C0 + FVector2f(CS, CS) / 2.f) * 0.5f; // 가까울수록 좋다
		if (Score < BestScore) { BestScore = Score; MenuPos = Pp; }
	}
	Sel = -1;
	SellCell = -1;
	UiSound(EPDSound::Move);
}

int32 SPDBoardWidget::CellMenuRowAt(FVector2f P) const
{
	if (MenuCell < 0) return -1;
	const float Y0 = CellMenuY() + CmTopH + 8.f;
	if (!InRect(P, MenuPos.X + 6.f, Y0, CmW - 12.f, MenuActs.Num() * CmRowH)) return -1;
	const int32 R = (int32)((P.Y - Y0) / CmRowH);
	return R >= 0 && R < MenuActs.Num() ? R : -1;
}

bool SPDBoardWidget::CellMenuHit(FVector2f P) const { return MenuCell >= 0 && InRect(P, MenuPos.X, CellMenuY(), CmW, CellMenuH(MenuActs.Num())); }

void SPDBoardWidget::DoCellAct(int32 Row)
{
	if (Row < 0 || Row >= MenuActs.Num()) return;
	const FCellAct A = MenuActs[Row];
	const int32 Cell = MenuCell;
	if (A.Kind == ECellAct::NoFuse) { UiSound(EPDSound::Denied); return; } // 누를 수 없는 줄: 메뉴는 그대로 (D-371)
	RuleLog(FString::Printf(TEXT("cell menu (%d,%d) act=%d arg=%d '%s'"), Cell % Cols, Cell / Cols, (int32)A.Kind, A.A, *A.Label)); // 룰 로그 (D-358)
	CloseCellMenu();
	switch (A.Kind)
	{
	case ECellAct::Fuse: DoFuse(A.A, Cell); break; // 클릭한 칸으로 합성 (D-312 ②)
	case ECellAct::Feed:
		PendingFuseCells.Reset();
		for (int K : Game.Synergies[A.A].Cells) if (!Game.Towers.at(K).IsJoker()) PendingFuseCells.Add(CellCenter(K));
		if (!Game.Feed(A.A, Cell)) { PendingFuseCells.Reset(); UiSound(EPDSound::Denied); }
		break;
	case ECellAct::Merge:
	{
		int32 Mi = -1; // 번호가 아니라 칸으로 다시 찾는다 (그사이 조커 레벨이 올라 묶음 번호가 밀렸을 수 있다)
		const std::vector<FPDJokerMerge> Ms = Game.JokerMerges();
		for (int32 I = 0; I < (int32)Ms.size() && Mi < 0; ++I) { std::vector<int> X = Ms[I].Cells, Y = A.Cells; std::sort(X.begin(), X.end()); std::sort(Y.begin(), Y.end()); if (X == Y) Mi = I; }
		PendingFuseCells.Reset();
		for (int K : A.Cells) PendingFuseCells.Add(CellCenter(K));
		if (Mi < 0 || !Game.MergeJokers(Mi, Cell)) { PendingFuseCells.Reset(); UiSound(EPDSound::Denied); }
		break;
	}
	case ECellAct::Craft: // 조커 (D-313): 그 칸을 재료로 넣은 채 조커 고르기(D-211)로 넘어가 나머지를 고른다
	{
		if (!InCraft()) ToggleCraft();
		RefreshJokerList();
		for (int32 E = 0; E < (int32)JokerList.size(); ++E) if (JokerList[E].Upper < 0 && JokerList[E].J == A.A) { CraftSelect(E); break; }
		if (CraftJoker == A.A) CraftClick(Cell);
		break;
	}
	case ECellAct::Sell: if (!Game.SellTower(Cell)) UiSound(EPDSound::Denied); break;
	case ECellAct::NoFuse: break;
	}
}

// 상자 위쪽 줄: Row < 0 = 그 칸 툴팁(줄여서), 그 밖 = 그 할 일의 전후 비교 (사본에서 해 본다, D-209 · D-210 · D-213 과 같은 숫자)
void SPDBoardWidget::CellTipLines(int32 Cell, int32 Row, TArray<FTipLine>& Out) const
{
	Out.Reset();
	const FPDGame& G = Game;
	auto It = G.Towers.find(Cell);
	if (It == G.Towers.end()) return;
	const FPDTower& T = It->second;
	const FLinearColor Fg = Hex(TEXT("dce4f0")), Grey = Hex(TEXT("8a94a6")), Gold = Hex(TEXT("ffd26e")), Up = Hex(TEXT("9cffb0")), Down = Hex(TEXT("ff6a6a"));
	const FPDTowerStats St = TowerStats(T, G.CharmMask(), &G.Mods);
	auto PowOf = [&](int32 K) { for (const auto& KV : PowerList) if (KV.first == K) return KV.second; return 0.f; };
	auto PowSum = [&](const std::vector<int>& Cs) { float S = 0.f; for (int K : Cs) S += PowOf(K); return S; };
	auto PowIn = [](const FPDGame& Cp, int32 K) { for (const auto& KV : Cp.TowerPowers()) if (KV.first == K) return KV.second; return 0.f; };
	auto Add = [&](const FString& S, const FLinearColor& Col, float Px = 11.f, bool bB = false) { Out.Add({S, Col, Px, bB}); };
	const FCellAct* A = Row >= 0 && Row < MenuActs.Num() ? &MenuActs[Row] : nullptr;
	if (!A)
	{
		const float Pw = PowOf(Cell);
		const FString CardName = T.Card.IsWild() ? FString(Tr(TEXT("와일드"))) : RankText(T.Card.Rank);
		if (T.IsJoker()) Add(JokerTitle(T.Joker), IsUpperJoker(T.Joker) ? Gold : JokerPink, 13.f, true);
		else if (T.Level >= 2 && T.FuseHand >= 0 && T.FuseHand < (int32)EPDHand::Count) Add(TrF(TEXT("%s %s · %s Lv%d"), *CardName, SuitName(T.Card.Suit), HandName((EPDHand)T.FuseHand), T.Level), SuitColor(T.Card.Suit), 13.f, true);
		else Add(TrF(TEXT("%s %s · 합성 레벨 %d"), *CardName, SuitName(T.Card.Suit), T.Level), SuitColor(T.Card.Suit), 13.f, true);
		Add(TrF(TEXT("위력 %s/초 (판의 %d%%)"), *ShortNum(Pw), PowerSum > 0.f ? FMath::RoundToInt(Pw / PowerSum * 100.f) : 0), Up, 11.f, true);
		Add(TrF(TEXT("공격력 %.0f · 초당 %.1f발 · 치명 %.0f%% ×%.2f"), St.Dmg, St.Rate, St.Crit * 100.f, St.CritMult), Fg);
		Add(TrF(TEXT("사거리 %.1f%s%s"), St.Range, St.Splash > 0.f ? *TrF(TEXT(" · 범위 %.1f"), St.Splash) : TEXT(""), St.Slow > 0.f ? Tr(TEXT(" · 둔화")) : TEXT("")), Grey);
		if (T.IsJoker())
		{
			Add(T.JLevel >= JokerMaxLevel ? TrF(TEXT("조커 레벨 %d (최대)"), T.JLevel) : TrF(TEXT("조커 레벨 %d · 경험치 %.0f/%.0f"), T.JLevel, T.JXp, JokerXpNeed(T.JLevel)), JokerPink, 10.f);
			Add(JokerAbility(T.Joker), Fg, 10.f);
		}
		else
		{
			FString Hs;
			for (const FPDSynergy& S : G.Synergies) if (HasCell(S.Cells, Cell)) { if (!Hs.IsEmpty()) Hs += TEXT(" · "); Hs += HandLabel(S); }
			if (!Hs.IsEmpty()) Add(Hs, Gold, 10.f);
			if (T.Level >= 2) Add(TrF(TEXT("특성 공격 ×%.1f 연사 ×%.1f"), T.TDmg, T.TRate), Gold, 10.f);
		}
		return;
	}
	FPDGame Copy = G;
	Copy.bMuted = true;
	switch (A->Kind)
	{
	case ECellAct::Fuse:
	{
		if (!Copy.Fuse(A->A, Cell) || !Copy.Towers.count(Cell)) break;
		const FPDTower& R = Copy.Towers.at(Cell);
		const FPDTowerStats Rs = TowerStats(R, Copy.CharmMask(), &Copy.Mods);
		const float PwB = PowSum(A->Cells), PwA = PowIn(Copy, Cell), Pct = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f;
		const FString CardName = R.Card.IsWild() ? FString(Tr(TEXT("와일드"))) : RankText(R.Card.Rank);
		Add(TrF(TEXT("합성하면: %s %s · %s Lv%d"), *CardName, SuitName(R.Card.Suit), HandName((EPDHand)FMath::Max(0, R.FuseHand)), R.Level), Gold, 13.f, true);
		Add(TrF(TEXT("위력 %s/초 → %s/초 (%s)"), *ShortNum(PwB), *ShortNum(PwA), *Signed(Pct)), Pct >= 0.f ? Up : Down, 11.f, true);
		Add(TrF(TEXT("재료 %d장의 위력 합 → 합성 타워 하나"), (int32)A->Cells.size()), Grey, 9.f);
		Add(TrF(TEXT("공격력 %.0f → %.0f · 초당 %.1f → %.1f발"), St.Dmg, Rs.Dmg, St.Rate, Rs.Rate), Fg);
		Add(TrF(TEXT("사거리 %.1f → %.1f · 범위 %.1f → %.1f"), St.Range, Rs.Range, St.Splash, Rs.Splash), Grey);
		break;
	}
	case ECellAct::Feed:
	{
		int32 Gone = 0;
		for (int K : A->Cells) Gone += !G.Towers.at(K).IsJoker();
		if (!Copy.Feed(A->A, Cell) || !Copy.Towers.count(Cell)) break;
		const FPDTower& J1 = Copy.Towers.at(Cell);
		const float PwB = PowOf(Cell), PwA = PowIn(Copy, Cell), BoardB = PowerSum, BoardA = Copy.Power();
		const float Pj = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f, Pb = BoardB > 0.f ? (BoardA / BoardB - 1.f) * 100.f : 0.f;
		Add(TrF(TEXT("%s 먹이기 → %s"), *HandLabel(G.Synergies[A->A]), JokerName(T.Joker)), JokerPink, 13.f, true);
		Add(TrF(TEXT("조커 레벨 %d → %d · 경험치 %.0f → %.0f"), T.JLevel, J1.JLevel, T.JXp, J1.JXp), Fg);
		Add(TrF(TEXT("조커 위력 %s/초 → %s/초 (%s)"), *ShortNum(PwB), *ShortNum(PwA), *Signed(Pj)), Pj >= 0.f ? Up : Down, 11.f, true);
		Add(TrF(TEXT("판 위력 %s → %s (%s)"), *ShortNum(BoardB), *ShortNum(BoardA), *Signed(Pb)), Pb >= 0.f ? Up : Down, 10.f);
		Add(TrF(TEXT("먹인 카드 %d장은 사라짐"), Gone), Grey, 10.f);
		break;
	}
	case ECellAct::Merge:
	{
		if (!Copy.MergeJokers(A->A, Cell) || !Copy.Towers.count(Cell)) break;
		const FPDTower& R = Copy.Towers.at(Cell);
		const float PwB = PowSum(A->Cells), PwA = PowIn(Copy, Cell), Pct = PwB > 0.f ? (PwA / PwB - 1.f) * 100.f : 0.f;
		Add(TrF(TEXT("합치면: %s Lv%d"), JokerName(R.Joker), R.JLevel), IsUpperJoker(R.Joker) ? Gold : JokerPink, 13.f, true);
		Add(TrF(TEXT("위력 %s/초 → %s/초 (%s)"), *ShortNum(PwB), *ShortNum(PwA), *Signed(Pct)), Pct >= 0.f ? Up : Down, 11.f, true);
		Add(TrF(TEXT("조커 %d장의 위력 합 → 조커 하나"), (int32)A->Cells.size()), Grey, 9.f);
		Add(JokerAbility(R.Joker), Fg, 10.f);
		break;
	}
	case ECellAct::Craft:
		Add(TrF(TEXT("조커: %s · 위력 ×%s"), (G.Discovered >> A->A) & 1u ? JokerName(A->A) : TEXT("???"), *FString::SanitizeFloat(JokerTier((EPDJoker)A->A))), JokerPink, 13.f, true);
		Add(JokerRecipeText(A->A), Gold, 10.f);
		Add(Tr(TEXT("이 칸의 족보를 재료로 넣고 나머지 재료를 고른다")), Grey, 10.f);
		Add(JokerAbility(A->A), Fg, 10.f);
		break;
	case ECellAct::NoFuse: // 합성이 막힌 까닭 (D-371)
		Add(TrF(TEXT("%s · 합성할 수 없다"), *HandLabel(G.Synergies[A->A])), Grey, 13.f, true);
		if (A->B == (int32)FPDGame::EFuseBlock::Joker) { Add(Tr(TEXT("조커가 낀 족보는 합성하지 않는다")), Fg); Add(Tr(TEXT("조커에 먹이면 조커 레벨이 오른다")), Grey, 10.f); }
		else if (A->B == (int32)FPDGame::EFuseBlock::ClownPath) { Add(Tr(TEXT("키스톤 광대의 길: 합성은 레벨 2까지")), Fg); Add(Tr(TEXT("대신 조커 배율 +0.4")), Grey, 10.f); }
		else Add(TrF(TEXT("이미 최고 합성 레벨 (Lv%d)"), MaxLevel), Fg);
		break;
	case ECellAct::Sell:
	{
		const float BoardB = PowerSum;
		if (!Copy.SellTower(Cell)) break;
		const float BoardA = Copy.Power(), Pb = BoardB > 0.f ? (BoardA / BoardB - 1.f) * 100.f : 0.f;
		Add(TrF(TEXT("판매하면 경험치 +%d"), G.SellPrice(Cell)), Hex(TEXT("cfd8e6")), 13.f, true);
		Add(TrF(TEXT("판 위력 %s → %s (%s)"), *ShortNum(BoardB), *ShortNum(BoardA), *Signed(Pb)), Pb >= 0.f ? Up : Down, 11.f, true);
		Add(Tr(TEXT("이 칸이 든 족보는 깨진다")), Grey, 10.f);
		break;
	}
	}
}

void SPDBoardWidget::PaintCellMenu(FCanvas& F) const
{
	if (MenuCell < 0 || !Game.Towers.count(MenuCell)) return;
	const FLinearColor Fg = Hex(TEXT("dce4f0")), Dim = Hex(TEXT("5f6878")), Target = Hex(TEXT("5ff2ff"));
	const float Wv = 0.5f + 0.5f * FMath::Sin(Clock * 3.f);
	const int32 Hov = CellMenuRowAt(Mouse);
	const FVector2f C0 = CellPx((float)(MenuCell % Cols), (float)(MenuCell / Cols));
	// 판: 연 칸은 분홍 고리. 줄에 마우스 = 그 할 일에 쓰이는 칸만 밝게 + 재료 → 클릭한 칸 점선 (합성 모드 그림, D-312 ③)
	if (Hov >= 0)
	{
		const FCellAct& A = MenuActs[Hov];
		const int32 DL = F.NextLayer();
		for (int32 K = 0; K < Cols * Rows; ++K)
		{
			if (HasCell(A.Cells, K) || K == MenuCell) continue;
			const FVector2f K0 = CellPx((float)(K % Cols), (float)(K / Cols));
			F.BoxAt(DL, K0.X, K0.Y, CS, CS, Hex(TEXT("02020a"), Game.Path.IsPath(K % Cols, K / Cols) ? 0.2f : 0.7f));
		}
		F.NextLayer();
		for (int K : A.Cells)
		{
			const FVector2f K0 = CellPx((float)(K % Cols), (float)(K / Cols));
			F.Box(K0.X + 3.f, K0.Y + 3.f, CS - 6.f, CS - 6.f, WithAlpha(A.Col, 0.16f + 0.08f * Wv));
			F.Glow(FCanvas::RoundRectPoints(K0.X + 3.f, K0.Y + 3.f, CS - 6.f, CS - 6.f, 8.f), WithAlpha(A.Col, 0.9f), 2.2f, 0.9f + 0.5f * Wv);
			if (K != MenuCell && A.Kind != ECellAct::Sell) F.Dashed({CellCenter(K), CellCenter(MenuCell)}, WithAlpha(Target, 0.6f), 1.5f, 4.f, 5.f);
		}
		F.Glow(FCanvas::RoundRectPoints(C0.X + 1.f, C0.Y + 1.f, CS - 2.f, CS - 2.f, 8.f), WithAlpha(Target, 0.95f), 3.4f, 1.4f + Wv);
	}
	else F.Glow(FCanvas::RoundRectPoints(C0.X + 1.f, C0.Y + 1.f, CS - 2.f, CS - 2.f, 8.f), JokerPink, 2.4f, 1.f + 0.4f * Wv);
	// 상자
	const float H = CellMenuH(MenuActs.Num());
	const float X = MenuPos.X, Y = CellMenuY();
	const FPDTower& T = Game.Towers.at(MenuCell);
	const FLinearColor Edge = T.IsJoker() ? (IsUpperJoker(T.Joker) ? Hex(TEXT("ffd26e")) : JokerPink) : SuitColor(T.Card.Suit);
	F.Box(X, Y, CmW, H, Hex(TEXT("06060b"), 0.96f));
	F.Glow(FCanvas::RoundRectPoints(X, Y, CmW, H, RadSmall), Edge, 1.4f, 0.55f);
	TArray<FTipLine> Lines;
	CellTipLines(MenuCell, Hov, Lines);
	for (int32 I = 0; I < Lines.Num() && I < (int32)CmTopLines; ++I) F.Text(Lines[I].S, X + 12.f, Y + 20.f + I * CmLineH, Lines[I].Col, Lines[I].Px, 0, Lines[I].bBold, CmW - 24.f);
	F.Lines({FVector2f(X + 10.f, Y + CmTopH), FVector2f(X + CmW - 10.f, Y + CmTopH)}, WithAlpha(Dim, 0.8f), 1.f);
	for (int32 R = 0; R < MenuActs.Num(); ++R)
	{
		const FCellAct& A = MenuActs[R];
		const float RY = Y + CmTopH + 8.f + R * CmRowH;
		const bool bH = R == Hov;
		// 알약 튜브 버튼 (D-350, 시안 A tools/neon/cellmenu_btn_mock.py): 처음부터 버튼 테 · 옅은 채움이라 누르는 줄 안다. 마우스 = 진한 채움 + 밝은 테 + 흰 글자
		const float BX = X + 9.f, BY = RY + (CmRowH - CmBtnH) / 2.f, BW = CmW - 18.f, Rr = RadPill(CmBtnH);
		if (A.Kind == ECellAct::NoFuse) // 누를 수 없는 줄 (D-371): 채움 없이 흐린 테 한 겹, 회색 글자
		{
			F.Glow(FCanvas::RoundRectPoints(BX, BY, BW, CmBtnH, Rr), WithAlpha(A.Col, bH ? 0.9f : 0.6f), 1.f, 0.15f);
			F.Text(A.Label, BX + 16.f, BY + CmBtnH / 2.f + 4.f, Hex(TEXT("8a94a6")), 11.f, 0, false, BW - 100.f);
			F.Text(A.Right, BX + BW - 14.f, BY + CmBtnH / 2.f + 4.f, Hex(TEXT("8a94a6")), 10.f, 2, false);
			continue;
		}
		F.BtnFill(BX, BY, BW, CmBtnH, Rr, A.Col, bH);
		F.Glow(FCanvas::RoundRectPoints(BX, BY, BW, CmBtnH, Rr), WithAlpha(A.Col, bH ? 1.f : 0.75f), bH ? 1.8f : 1.3f, bH ? 1.f : 0.45f);
		F.Lines(FCanvas::RoundRectPoints(BX + 3.f, BY + 3.f, BW - 6.f, CmBtnH - 6.f, RadPill(CmBtnH - 6.f)), WithAlpha(A.Col, bH ? 0.6f : 0.3f), 0.7f); // 안쪽 둘째 겹
		F.Text(A.Label, BX + 16.f, BY + CmBtnH / 2.f + 4.f, bH ? HotCol(A.Col) : A.Col, 12.f, 0, true, BW - 100.f); // D-382
		F.Text(A.Right, BX + BW - 14.f, BY + CmBtnH / 2.f + 4.f, bH ? Fg : Hex(TEXT("8a94a6")), 10.f, 2, false);
	}
	if (MenuActs.Num() == 0) F.Text(Tr(TEXT("할 수 있는 일이 없다")), X + 18.f, Y + CmTopH + 22.f, Dim, 11.f);
}
