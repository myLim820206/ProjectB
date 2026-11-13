// 포커 디펜스 화면: 틱, 입력(마우스·키보드), 메뉴, 합성 위치 고르기.
// 이펙트는 PDBoardFx.cpp, 그리기는 PDBoardPaint.cpp, 그리기 도우미는 PDBoardDraw.h
#include "PDBoardWidget.h"
#include "PDBoardDraw.h"

#include "InputCoreTypes.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// ---------------------------------------------------------------- 위젯
void SPDBoardWidget::Construct(const FArguments& InArgs)
{
	SetCanTick(true);
	LoadUserData();
}

// ---------------------------------------------------------------- 설정·진행 저장 (D-110)
namespace
{
const TCHAR* const PDSetSection = TEXT("ProjectB.Settings");
FString PDProgressPath() { return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Progress.txt")); }
}

void SPDBoardWidget::LoadUserData()
{
	if (GConfig)
	{
		GConfig->GetBool(PDSetSection, TEXT("WorldFx"), Settings.bWorldFx, GGameUserSettingsIni);
		GConfig->GetBool(PDSetSection, TEXT("Shake"), Settings.bShake, GGameUserSettingsIni);
		GConfig->GetBool(PDSetSection, TEXT("Sound"), Settings.bSound, GGameUserSettingsIni);
		GConfig->GetFloat(PDSetSection, TEXT("Volume"), Settings.Volume, GGameUserSettingsIni);
		GConfig->GetFloat(PDSetSection, TEXT("MusicVolume"), Settings.MusicVolume, GGameUserSettingsIni);
		GConfig->GetInt(PDSetSection, TEXT("WindowMode"), Settings.WindowMode, GGameUserSettingsIni);
		GConfig->GetInt(PDSetSection, TEXT("FrameCap"), Settings.FrameCap, GGameUserSettingsIni);
		GConfig->GetBool(PDSetSection, TEXT("VSync"), Settings.bVSync, GGameUserSettingsIni);
	}
	Settings.FrameCap = FMath::Clamp(Settings.FrameCap, 0, 2);
	ApplyFrameSettings();
	Settings.Volume = FMath::Clamp(Settings.Volume, 0.f, 1.f);
	Settings.MusicVolume = FMath::Clamp(Settings.MusicVolume, 0.f, 1.f);
	if (!GIsEditor && GEngine && GEngine->GetGameUserSettings()) // 실제 화면 모드는 엔진이 저장한 값
	{
		switch (GEngine->GetGameUserSettings()->GetFullscreenMode())
		{
		case EWindowMode::Fullscreen: Settings.WindowMode = 1; break;
		case EWindowMode::WindowedFullscreen: Settings.WindowMode = 2; break;
		default: Settings.WindowMode = 0; break;
		}
	}
	Game.bMuted = !Settings.bSound;
	// 진행: 도감(만들어 본 조커)과 최고 기록
	TArray<FString> Lines;
	if (FFileHelper::LoadFileToStringArray(Lines, *PDProgressPath()))
		for (const FString& L : Lines)
		{
			FString Key, Val;
			if (!L.Split(TEXT("="), &Key, &Val)) continue;
			if (Key == TEXT("Discovered")) Game.Discovered |= (uint32)FCString::Strtoui64(*Val, nullptr, 10);
			else if (Key == TEXT("BestWave")) Game.BestWave = FMath::Max(Game.BestWave, FCString::Atoi(*Val));
		}
	SavedDiscovered = Game.Discovered;
	SavedBest = Game.BestWave;
}

void SPDBoardWidget::SaveSettings() const
{
	if (!GConfig) return;
	GConfig->SetBool(PDSetSection, TEXT("WorldFx"), Settings.bWorldFx, GGameUserSettingsIni);
	GConfig->SetBool(PDSetSection, TEXT("Shake"), Settings.bShake, GGameUserSettingsIni);
	GConfig->SetBool(PDSetSection, TEXT("Sound"), Settings.bSound, GGameUserSettingsIni);
	GConfig->SetFloat(PDSetSection, TEXT("Volume"), Settings.Volume, GGameUserSettingsIni);
	GConfig->SetFloat(PDSetSection, TEXT("MusicVolume"), Settings.MusicVolume, GGameUserSettingsIni);
	GConfig->SetInt(PDSetSection, TEXT("WindowMode"), Settings.WindowMode, GGameUserSettingsIni);
	GConfig->SetInt(PDSetSection, TEXT("FrameCap"), Settings.FrameCap, GGameUserSettingsIni);
	GConfig->SetBool(PDSetSection, TEXT("VSync"), Settings.bVSync, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void SPDBoardWidget::SaveProgress()
{
	SavedDiscovered = Game.Discovered;
	SavedBest = FMath::Max3(SavedBest, Game.BestWave, Game.Wave);
	FFileHelper::SaveStringToFile(FString::Printf(TEXT("Discovered=%u\nBestWave=%d\n"), SavedDiscovered, SavedBest), *PDProgressPath());
}

// 프레임 제한 (D-120): 판이 2D 라 60이면 충분. 제한이 없으면 그래픽카드가 초당 수백 장을 그린다
void SPDBoardWidget::ApplyFrameSettings() const
{
	static const float Caps[] = {60.f, 120.f, 0.f};
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) V->Set(Caps[Settings.FrameCap], ECVF_SetByGameSetting);
	if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"))) V->Set(Settings.bVSync ? 1 : 0, ECVF_SetByGameSetting);
	if (GEngine && GEngine->GetGameUserSettings() && !GIsEditor)
	{
		UGameUserSettings* U = GEngine->GetGameUserSettings();
		U->SetFrameRateLimit(Caps[Settings.FrameCap]);
		U->SetVSyncEnabled(Settings.bVSync);
		U->ApplyNonResolutionSettings();
		U->SaveSettings();
	}
}

void SPDBoardWidget::ApplyWindowMode(int32 Mode)
{
	Settings.WindowMode = (Mode % 3 + 3) % 3;
	SaveSettings();
	if (GIsEditor || !GEngine) return; // 에디터(PIE)에서는 창 모드를 바꾸지 않는다. 패키지 빌드에서만
	UGameUserSettings* U = GEngine->GetGameUserSettings();
	if (!U) return;
	const EWindowMode::Type M = Settings.WindowMode == 1 ? EWindowMode::Fullscreen : Settings.WindowMode == 2 ? EWindowMode::WindowedFullscreen : EWindowMode::Windowed;
	U->SetFullscreenMode(M);
	U->SetScreenResolution(M == EWindowMode::Windowed ? FIntPoint(1280, 720) : U->GetDesktopResolution());
	U->ApplySettings(false);
	U->SaveSettings();
}

void SPDBoardWidget::SetSound(bool bOn)
{
	Settings.bSound = bOn;
	Game.bMuted = !bOn;
	SaveSettings();
}

void SPDBoardWidget::SettingsChange(int32 Row, int32 Delta)
{
	switch ((ESetRow)Row)
	{
	case ESetRow::Window: ApplyWindowMode(Settings.WindowMode + (Delta == 0 ? 1 : Delta)); break;
	case ESetRow::Frame: Settings.FrameCap = ((Settings.FrameCap + (Delta == 0 ? 1 : Delta)) % 3 + 3) % 3; SaveSettings(); ApplyFrameSettings(); break;
	case ESetRow::VSync: Settings.bVSync = !Settings.bVSync; SaveSettings(); ApplyFrameSettings(); break;
	case ESetRow::WorldFx: Settings.bWorldFx = !Settings.bWorldFx; SaveSettings(); break;
	case ESetRow::Shake: Settings.bShake = !Settings.bShake; SaveSettings(); break;
	case ESetRow::Sound: SetSound(!Settings.bSound); break;
	case ESetRow::Volume:
		Settings.Volume = FMath::Clamp(FMath::RoundToFloat((Settings.Volume + 0.1f * Delta) * 10.f) / 10.f, 0.f, 1.f);
		if (Settings.Volume > 0.f && !Settings.bSound) SetSound(true);
		else SaveSettings();
		break;
	case ESetRow::Music:
		Settings.MusicVolume = FMath::Clamp(FMath::RoundToFloat((Settings.MusicVolume + 0.1f * Delta) * 10.f) / 10.f, 0.f, 1.f);
		if (Settings.MusicVolume > 0.f && !Settings.bSound) SetSound(true);
		else SaveSettings();
		break;
	case ESetRow::Close: bSettings = false; return;
	default: return;
	}
	UiSound(EPDSound::Move);
}

void SPDBoardWidget::SettingsClick(FVector2f P)
{
	if (!InRect(P, SetBoxX, SetBoxY, SetBoxW, SetBoxH)) { bSettings = false; return; } // 창 바깥 = 닫기
	for (int32 Row = 0; Row < (int32)ESetRow::Count; ++Row)
		for (int32 I = 0; I < SetBtnCount(Row); ++I)
		{
			const FVector2f B = SetBtnPos(Row, I);
			if (!InRect(P, B.X, B.Y, SetBtnW(Row, I), SetBtnH)) continue;
			SetSel = Row;
			if (Row == (int32)ESetRow::Window) { ApplyWindowMode(I); UiSound(EPDSound::Move); }
			else if (Row == (int32)ESetRow::Frame) { Settings.FrameCap = I; SaveSettings(); ApplyFrameSettings(); UiSound(EPDSound::Move); }
			else if (SetBarRow(Row))
			{
				if (I == 1) // 막대: 누른 자리로
				{
					float& Vol = Row == (int32)ESetRow::Music ? Settings.MusicVolume : Settings.Volume;
					Vol = FMath::Clamp(FMath::RoundToFloat((P.X - B.X) / SetBtnW(Row, 1) * 10.f) / 10.f, 0.f, 1.f);
					if (Vol > 0.f && !Settings.bSound) SetSound(true);
					else SaveSettings();
					UiSound(EPDSound::Move);
				}
				else SettingsChange(Row, I == 0 ? -1 : 1);
			}
			else SettingsChange(Row, 0);
			return;
		}
}

void SPDBoardWidget::SettingsKey(const FKey& K)
{
	const int32 N = (int32)ESetRow::Count;
	if (K == EKeys::Escape) bSettings = false;
	else if (K == EKeys::Up || K == EKeys::W) SetSel = (SetSel + N - 1) % N;
	else if (K == EKeys::Down || K == EKeys::S) SetSel = (SetSel + 1) % N;
	else if (K == EKeys::Left || K == EKeys::A) SettingsChange(SetSel, -1);
	else if (K == EKeys::Right || K == EKeys::D) SettingsChange(SetSel, 1);
	else if (K == EKeys::Enter || K == EKeys::SpaceBar) SettingsChange(SetSel, 0);
	else if (K == EKeys::M) SetSound(!Settings.bSound);
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
	if (!Settings.bShake) { Shake = 0.f; FlashT = 0.f; } // 설정: 화면 흔들림·번쩍임 끔
	if (Game.Discovered != SavedDiscovered || FMath::Max(Game.BestWave, Game.Wave) > SavedBest) SaveProgress(); // 도감·최고 기록이 바뀌면 바로 저장
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
	for (int K : FuseSyn(Game, SynergyIdx).Cells) PendingFuseCells.Add(CellCenter(K));
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
		if (Game.CanFuse(I) && SameCells(FuseSyn(Game, I).Cells, PickCells)) return I;
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

bool SPDBoardWidget::SynRevealed(int32 I) const
{
	if (I < 0 || I >= (int32)Game.Synergies.size()) return false;
	if (!PickCells.empty() && !bPickJoker && FindPickIndex() == I) return true; // 합성 칸 고르는 중인 족보
	const int32 Key = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
	const std::vector<int>& Cells = Game.Synergies[I].Cells;
	if (Key >= 0 && std::find(Cells.begin(), Cells.end(), Key) != Cells.end()) return true;
	const bool bFuse = Game.CanFuse(I);
	if (bFuse && InBox(Mouse, FuseButtonCenter(Game.Synergies, I), FuseBtnW(FuseSyn(Game, I)), FuseH)) return true;
	return InBox(Mouse, FeedButtonCenter(Game, I, bFuse), FeedBtnW(Game.Synergies[I]), FuseH); // 먹이기 버튼 위 (먹일 수 없으면 그리지도 않음)
}

bool SPDBoardWidget::JokerRevealed(const FPDJokerGroup& Gr) const
{
	if (!PickCells.empty() && bPickJoker && SameCells(Gr.Cells, PickCells)) return true;
	const int32 Key = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
	if (Key >= 0 && std::find(Gr.Cells.begin(), Gr.Cells.end(), Key) != Gr.Cells.end()) return true;
	return InBox(Mouse, JokerButtonCenter(Gr), JokerBtnW, JokerBtnH);
}

FVector2f SPDBoardWidget::ToDesign(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) const
{
	const auto Local = MyGeometry.AbsoluteToLocal(MouseEvent.GetScreenSpacePosition());
	return FVector2f(((float)Local.X - ViewOffset.X) / ViewScale, ((float)Local.Y - ViewOffset.Y) / ViewScale);
}

FReply SPDBoardWidget::OnMouseMove(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	Mouse = ToDesign(MyGeometry, MouseEvent);
	if (bSettings)
	{
		for (int32 Row = 0; Row < (int32)ESetRow::Count; ++Row)
			if (InRect(Mouse, SetX, SetRowY(Row), SetW, SetRowH)) SetSel = Row;
	}
	else if (bMenu)
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
	if (bSettings) { if (bRight) bSettings = false; else SettingsClick(P); return; }
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
	if (bCodex) // 도감 (D-119): 카드 클릭 = 크게 보기, 닫기 버튼·오른쪽 클릭 = 닫기
	{
		if (bRight || InRect(P, CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH)) { bCodex = false; return; }
		for (int32 K = 0; K < (int32)EPDJoker::Count; ++K)
		{
			const FVector2f C0 = CodexCardPos(K);
			if (InRect(P, C0.X, C0.Y, CdxCardW, CdxCardH)) { CodexSel = K; UiSound(EPDSound::Move); }
		}
		return;
	}
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
		if (bRight || InRect(P, CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH)) { bTree = false; return; }
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
		else if (const int32 Pi = FindPickIndex(); Pi >= 0) { const FVector2f B = FuseButtonCenter(Game.Synergies, Pi); bSameBtn = InBox(P, B, FuseBtnW(FuseSyn(Game, Pi)), FuseH); }
		if (!bRight && bSameBtn) FinishPick(-1);
		else if (!bRight && Key >= 0 && std::find(PickCells.begin(), PickCells.end(), Key) != PickCells.end()) FinishPick(Key);
		else PickCells.clear();
		return;
	}
	if (!bRight && InRect(P, MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH)) { bMenu = true; MenuSel = 0; Sel = -1; return; }
	if (!bRight && InRect(P, TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH)) { bTree = true; Sel = -1; return; }
	if (!bRight && InRect(P, UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH)) { bUpg = true; Sel = -1; return; }
	if (!bRight && InRect(P, CodexBtnX, CodexBtnY, CodexBtnW, CodexBtnH)) { bCodex = true; Sel = -1; return; }
	// 위쪽 띠 버튼 (D-119): 속도, 설정
	if (!bRight && InRect(P, SpeedBtnX, HudBtnY, HudBtnS, HudBtnS)) { Game.Speed = NextSpeed(Game.Speed); UiSound(EPDSound::Move); return; }
	if (!bRight && InRect(P, SetOpenX, HudBtnY, HudBtnS, HudBtnS)) { bSettings = true; SetSel = 0; Sel = -1; return; }
	// 조커 합치기 버튼 (같은 조커끼리, D-114): 마우스가 그 조커들 위나 버튼 자리에 있을 때만
	if (!bRight)
	{
		const std::vector<FPDJokerMerge> Ms = Game.JokerMerges();
		const int32 HKey = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
		for (int32 I = 0; I < (int32)Ms.size(); ++I)
		{
			const FVector2f B = MergeButtonCenter(Ms[I].Cells);
			const bool bRev = std::find(Ms[I].Cells.begin(), Ms[I].Cells.end(), HKey) != Ms[I].Cells.end() || InBox(Mouse, B, MergeW, MergeH);
			if (!bRev || !InBox(P, B, MergeW, MergeH)) continue;
			PendingFuseCells.Reset();
			for (int K : Ms[I].Cells) PendingFuseCells.Add(CellCenter(K));
			if (!Game.MergeJokers(I)) PendingFuseCells.Reset();
			return;
		}
	}
	// 조커 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)JGroups.size(); ++I)
		{
			const FVector2f B = JokerButtonCenter(JGroups[I]);
			if (JokerRevealed(JGroups[I]) && InBox(P, B, JokerBtnW, JokerBtnH)) { StartPick(JGroups[I].Cells, true); return; }
		}
	// 합성 버튼
	if (!bRight)
		for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		{
			if (!Game.CanFuse(I) || !SynRevealed(I)) continue;
			const FVector2f B = FuseButtonCenter(Game.Synergies, I);
			if (InBox(P, B, FuseBtnW(FuseSyn(Game, I)), FuseH)) { StartPick(FuseSyn(Game, I).Cells, false); return; }
		}
	// 먹이기 버튼 (조커에 맞닿은 족보)
	if (!bRight)
		for (const FPDFeed& Fd : Game.FeedTargets())
		{
			const FVector2f B = FeedButtonCenter(Game, Fd.Syn, Game.CanFuse(Fd.Syn));
			if (SynRevealed(Fd.Syn) && InBox(P, B, FeedBtnW(Game.Synergies[Fd.Syn]), FuseH))
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
	bTree = bUpg = bCodex = bMenu = bSettings = false;
	PowerRef = 0.f;
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
	case EMenuItem::Settings: bSettings = true; SetSel = 0; break;
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
	if (bSettings) { SettingsKey(K); return FReply::Handled(); }
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
	if (bCodex)
	{
		if (K == EKeys::J || K == EKeys::Escape) bCodex = false;
		else if (K == EKeys::Left || K == EKeys::A) CodexSel = (CodexSel + 9) % 10; // 화살표로 넘겨 보기
		else if (K == EKeys::Right || K == EKeys::D) CodexSel = (CodexSel + 1) % 10;
		else if (K == EKeys::Up || K == EKeys::Down || K == EKeys::W || K == EKeys::S) CodexSel = (CodexSel + 5) % 10;
		return FReply::Handled();
	}
	if (bTree) { if (K == EKeys::K || K == EKeys::Escape) bTree = false; else if (K == EKeys::M) SetSound(!Settings.bSound); return FReply::Handled(); }
	if (bUpg) { if (K == EKeys::U || K == EKeys::Escape) bUpg = false; else if (K == EKeys::M) SetSound(!Settings.bSound); return FReply::Handled(); }
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
	else if (K == EKeys::F) Game.Speed = NextSpeed(Game.Speed);
	else if (K == EKeys::M) SetSound(!Settings.bSound);
	else return FReply::Unhandled();
	return FReply::Handled();
}
