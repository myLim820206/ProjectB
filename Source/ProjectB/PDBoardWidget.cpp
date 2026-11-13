// 포커 디펜스 화면: 틱, 입력(마우스·키보드), 메뉴, 합성 위치 고르기.
// 이펙트는 PDBoardFx.cpp, 그리기는 PDBoardPaint.cpp, 그리기 도우미는 PDBoardDraw.h
#include "PDBoardWidget.h"
#include "PDBoardDraw.h"

#include "InputCoreTypes.h"

// ---------------------------------------------------------------- 위젯
void SPDBoardWidget::Construct(const FArguments& InArgs)
{
	SetCanTick(true);
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
	if (!bTree && !bUpg && !bMenu) Game.Tick(Dt); // 트리·강화·메뉴 창을 여는 동안은 멈춘다
	TreeBuyT = FMath::Max(0.f, TreeBuyT - Dt);
	HandleEvents();
	UpdateFx(Dt);
	if (Sel >= (int32)Game.Hand.size()) Sel = -1;
	UpdatePreview();
	JGroups = Game.JokerGroups();
	// 고르는 사이 족보가 바뀌었거나(교체·다른 합성) 보상 화면이 뜨면 고르기 취소
	if (!PickCells.empty() && (Game.Phase == EPDPhase::Reward || FindPickIndex() < 0)) PickCells.clear();
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SPDBoardWidget::DoFuse(int32 SynergyIdx, int32 Cell)
{
	if (!Game.CanFuse(SynergyIdx)) return;
	PendingFuseCells.Reset();
	for (int K : Game.Synergies[SynergyIdx].Cells) PendingFuseCells.Add(CellCenter(K));
	if (!Game.Fuse(SynergyIdx, Cell)) PendingFuseCells.Reset();
}

namespace
{
bool SameCells(std::vector<int> A, std::vector<int> B)
{
	std::sort(A.begin(), A.end());
	std::sort(B.begin(), B.end());
	return A == B;
}
}

int32 SPDBoardWidget::FindPickIndex() const
{
	if (PickCells.empty()) return -1;
	if (bPickJoker)
	{
		for (int32 I = 0; I < (int32)JGroups.size(); ++I) if (SameCells(JGroups[I].Cells, PickCells)) return I;
		return -1;
	}
	for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		if (Game.CanFuse(I) && SameCells(Game.Synergies[I].Cells, PickCells)) return I;
	return -1;
}

void SPDBoardWidget::StartPick(const std::vector<int>& Cells, bool bJoker)
{
	PickCells = Cells;
	bPickJoker = bJoker;
	Sel = -1;
	UiSound(EPDSound::Move);
}

void SPDBoardWidget::FinishPick(int32 Cell)
{
	const int32 Idx = FindPickIndex();
	const bool bJoker = bPickJoker;
	PickCells.clear();
	if (Idx < 0) return;
	if (!bJoker) { DoFuse(Idx, Cell); return; }
	PendingFuseCells.Reset();
	for (int K : JGroups[Idx].Cells) PendingFuseCells.Add(CellCenter(K));
	if (!Game.JokerFuse(Idx, Cell)) PendingFuseCells.Reset();
}

int32 SPDBoardWidget::TreeHoverAt(FVector2f P) const
{
	int32 Best = -1;
	float BestD = 15.f;
	for (int32 Id = 0; Id < TreeNodeCount(); ++Id)
	{
		const float D = FVector2f::Distance(P, TreeNodePos(Id));
		if (D < BestD) { BestD = D; Best = Id; }
	}
	return Best;
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
	if (bMenu)
	{
		const int32 Item = MenuHoverAt(Mouse);
		if (Item >= 0) MenuSel = Item;
	}
	const int32 Cell = CellAt(Mouse);
	HoverX = Cell >= 0 ? Cell % Cols : -1;
	HoverY = Cell >= 0 ? Cell / Cols : -1;
	return FReply::Handled();
}

void SPDBoardWidget::Click(FVector2f P, bool bRight)
{
	if (bMenu)
	{
		const int32 Item = MenuHoverAt(P);
		if (Item >= 0) MenuSelect(Item);
		else if (!InRect(P, MenuX - 30.f, MenuItemY0 - 80.f, MenuW + 60.f, (int32)EMenuItem::Count * (MenuItemH + MenuGap) + 110.f)) bMenu = false; // 창 바깥 클릭 = 닫기
		return;
	}
	if (Game.Phase == EPDPhase::Title) { Game.Phase = EPDPhase::Build; UiSound(EPDSound::Start); return; }
	if (Game.Phase == EPDPhase::Over) { RestartRun(); return; }
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < (int32)Game.Rewards.size(); ++I)
		{
			const FVector2f R = RewardPos(I);
			if (InRect(P, R.X, R.Y, RewardW, RewardH)) { Game.TakeReward(I); return; }
		}
		return;
	}
	if (bCodex) { bCodex = false; return; }
	if (bUpg)
	{
		for (int32 U = 0; U < (int32)EPDUpgrade::Count; ++U)
			for (int32 K = 0; K < 4; ++K)
			{
				const FVector2f B = UpgButtonPos(U, K);
				if (InRect(P, B.X, B.Y, UpgBtnW, UpgBtnH) && Game.Upgrade((EPDUpgrade)U, UpgSteps[K]) == 0) UiSound(EPDSound::Denied);
			}
		return;
	}
	if (bTree)
	{
		const int32 Id = TreeHoverAt(P);
		if (Id >= 0 && !Game.BuyNode(Id)) UiSound(EPDSound::Denied);
		return;
	}
	// 합성 위치 고르는 중: 재료 칸 클릭 = 그 칸으로, 같은 버튼 다시 = 기본 칸, 그 밖(오른쪽 클릭 포함) = 취소
	if (!PickCells.empty())
	{
		const int32 Key = CellAt(P);
		bool bSameBtn = false;
		if (bPickJoker) { FPDJokerGroup Tmp; Tmp.Cells = PickCells; const FVector2f B = JokerButtonCenter(Tmp); bSameBtn = InBox(P, B, JokerBtnW, JokerBtnH); }
		else if (const int32 Pi = FindPickIndex(); Pi >= 0) { const FVector2f B = FuseButtonCenter(Game.Synergies, Pi); bSameBtn = InBox(P, B, FuseW, FuseH); }
		if (!bRight && bSameBtn) FinishPick(-1);
		else if (!bRight && Key >= 0 && std::find(PickCells.begin(), PickCells.end(), Key) != PickCells.end()) FinishPick(Key);
		else PickCells.clear();
		return;
	}
	if (!bRight && InRect(P, MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH)) { bMenu = true; MenuSel = 0; Sel = -1; return; }
	if (!bRight && InRect(P, TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH)) { bTree = true; Sel = -1; return; }
	if (!bRight && InRect(P, UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH)) { bUpg = true; Sel = -1; return; }
	// 조커 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)JGroups.size(); ++I)
		{
			const FVector2f B = JokerButtonCenter(JGroups[I]);
			if (InBox(P, B, JokerBtnW, JokerBtnH)) { StartPick(JGroups[I].Cells, true); return; }
		}
	// 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		{
			if (!Game.CanFuse(I)) continue;
			const FVector2f B = FuseButtonCenter(Game.Synergies, I);
			if (InBox(P, B, FuseW, FuseH)) { StartPick(Game.Synergies[I].Cells, false); return; }
		}
	// 먹이기 버튼 (조커에 맞닿은 족보)
	if (!bRight)
		for (const FPDFeed& Fd : Game.FeedTargets())
		{
			const FVector2f B = FeedButtonCenter(Game.Synergies, Fd.Syn, Game.CanFuse(Fd.Syn));
			if (InBox(P, B, FuseW, FuseH))
			{
				PendingFuseCells.Reset();
				for (int K : Game.Synergies[Fd.Syn].Cells) if (!Game.Towers.at(K).IsJoker()) PendingFuseCells.Add(CellCenter(K));
				if (!Game.Feed(Fd.Syn)) PendingFuseCells.Reset();
				return;
			}
		}
	int32 HandHit = -1;
	for (int32 I = 0; I < (int32)Game.Hand.size(); ++I)
	{
		const float X = HandCardX(I, (int32)Game.Hand.size());
		if (InRect(P, X, HandY - 12.f, CardW, CardH + 12.f)) HandHit = I;
	}
	if (bRight)
	{
		if (HandHit >= 0) { Game.Discard(HandHit); if (Sel == HandHit) Sel = -1; else if (Sel > HandHit) --Sel; }
		else Sel = -1;
		return;
	}
	if (HandHit >= 0) { Sel = Sel == HandHit ? -1 : HandHit; UiSound(EPDSound::Move); return; }
	if (const int32 Cell = CellAt(P); Cell >= 0 && Sel >= 0)
	{
		if (Game.Place(Sel, Cell % Cols, Cell / Cols)) Sel = -1;
		else UiSound(EPDSound::Denied);
		return;
	}
	if (Game.Phase == EPDPhase::Build && InRect(P, RBtnX, StartBtnY, RBtnW, StartBtnH)) Game.StartWave();
}

void SPDBoardWidget::UiSound(EPDSound S)
{
	if (!Game.bMuted) Game.Sounds.push_back({S, 0, 0, 1.f});
}

void SPDBoardWidget::RestartRun()
{
	Game.NewGame(true);
	Sel = -1;
	bTree = bUpg = bCodex = bMenu = false;
	PickCells.clear();
	Particles.Empty();
	Flares.Empty();
	Shards.Empty();
	UiSound(EPDSound::Start);
}

int32 SPDBoardWidget::MenuHoverAt(FVector2f P) const
{
	for (int32 I = 0; I < (int32)EMenuItem::Count; ++I)
	{
		const FVector2f B = MenuItemPos(I);
		if (InRect(P, B.X, B.Y, MenuW, MenuItemH)) return I;
	}
	return -1;
}

void SPDBoardWidget::MenuSelect(int32 Item)
{
	switch ((EMenuItem)Item)
	{
	case EMenuItem::Resume: bMenu = false; break;
	case EMenuItem::NewGame: RestartRun(); break;
	case EMenuItem::Quit:
		bMenu = false;
		if (OnQuit) OnQuit();
		else FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
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
	if (bMenu)
	{
		const int32 N = (int32)EMenuItem::Count;
		if (K == EKeys::Escape) bMenu = false;
		else if (K == EKeys::Up || K == EKeys::W) MenuSel = (MenuSel + N - 1) % N;
		else if (K == EKeys::Down || K == EKeys::S) MenuSel = (MenuSel + 1) % N;
		else if (K == EKeys::Enter || K == EKeys::SpaceBar) MenuSelect(MenuSel);
		return FReply::Handled();
	}
	// 도감·트리·강화 창이 열려 있지 않고 카드도 안 골랐으면 Esc = 메뉴
	auto OpenMenu = [this]() { bMenu = true; MenuSel = 0; Sel = -1; };
	if (Game.Phase == EPDPhase::Title)
	{
		if (K == EKeys::Enter || K == EKeys::SpaceBar) { Game.Phase = EPDPhase::Build; UiSound(EPDSound::Start); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (Game.Phase == EPDPhase::Over)
	{
		if (K == EKeys::R || K == EKeys::Enter) { RestartRun(); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	if (bCodex) { if (K == EKeys::J || K == EKeys::Escape) bCodex = false; return FReply::Handled(); }
	if (bTree) { if (K == EKeys::K || K == EKeys::Escape) bTree = false; else if (K == EKeys::M) Game.bMuted = !Game.bMuted; return FReply::Handled(); }
	if (bUpg) { if (K == EKeys::U || K == EKeys::Escape) bUpg = false; else if (K == EKeys::M) Game.bMuted = !Game.bMuted; return FReply::Handled(); }
	if (!PickCells.empty())
	{
		if (K == EKeys::Escape) { PickCells.clear(); return FReply::Handled(); }
		if (K == EKeys::G || K == EKeys::Enter) { FinishPick(-1); return FReply::Handled(); }
	}
	if (Game.Phase == EPDPhase::Reward)
	{
		for (int32 I = 0; I < 3; ++I) if (K == Nums[I]) { Game.TakeReward(I); return FReply::Handled(); }
		if (K == EKeys::S) { Game.TakeReward(-1); return FReply::Handled(); }
		if (K == EKeys::Escape) { OpenMenu(); return FReply::Handled(); }
		return FReply::Unhandled();
	}
	for (int32 I = 0; I < 9; ++I)
		if (K == Nums[I])
		{
			if (I < (int32)Game.Hand.size()) { Sel = Sel == I ? -1 : I; UiSound(EPDSound::Move); }
			return FReply::Handled();
		}
	if (K == EKeys::SpaceBar || K == EKeys::Enter) Game.StartWave();
	else if (K == EKeys::Escape) { if (Sel >= 0) Sel = -1; else OpenMenu(); }
	else if (K == EKeys::G) DoFuse(HoveredSynergy());
	else if (K == EKeys::J) bCodex = !bCodex;
	else if (K == EKeys::K) { bTree = true; Sel = -1; }
	else if (K == EKeys::U) { bUpg = true; Sel = -1; }
	else if (K == EKeys::F) Game.Speed = Game.Speed == 1 ? 2 : Game.Speed == 2 ? 3 : 1;
	else if (K == EKeys::M) Game.bMuted = !Game.bMuted;
	else return FReply::Unhandled();
	return FReply::Handled();
}
