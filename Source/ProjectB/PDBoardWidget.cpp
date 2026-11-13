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
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/StrongObjectPtr.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "Widgets/SLeafWidget.h"
#include "RenderTimer.h"
#include "DynamicRHI.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"

#if !UE_BUILD_SHIPPING
static void PDDebugStress(PD::FPDGame& G);
#endif

// ---------------------------------------------------------------- 위젯
// 아틀라스를 굽는 동안만 쓰는 잎 위젯: 주인의 PaintAtlas 를 부른다
class SPDAtlasPainter : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SPDAtlasPainter) : _Owner(nullptr), _Mode(0) {} // 기본값 필수: 없으면 Mode 가 쓰레기 값이라 아틀라스에 판 배경이 그려졌다
	SLATE_ARGUMENT(const SPDBoardWidget*, Owner)
	SLATE_ARGUMENT(int32, Mode) // 0 아틀라스, 1 판 배경
	SLATE_END_ARGS()
	void Construct(const FArguments& A) { Owner = A._Owner; Mode = A._Mode; }
	virtual FVector2D ComputeDesiredSize(float) const override { return Mode ? FVector2D(PDDraw::BgW * PDDraw::AtlasK, PDDraw::BgH * PDDraw::AtlasK) : FVector2D(PDDraw::AtlasTex, PDDraw::AtlasTex); }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements,
		int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override
	{
		if (!Owner) return LayerId;
		return Mode ? Owner->PaintBoardBg(AllottedGeometry, OutDrawElements, LayerId) : Owner->PaintAtlas(AllottedGeometry, OutDrawElements, LayerId);
	}
private:
	const SPDBoardWidget* Owner = nullptr;
	int32 Mode = 0;
};

bool SPDBoardWidget::UseAtlas() const { return Atlas.IsValid() && Atlas->Ready() && !GPDDebug.bNoAtlas; }

void SPDBoardWidget::BakeAtlas()
{
	// 화면 배율이 바뀌면 같은 렌더 타깃에 다시 굽는다 (Slate 선 굵기는 화면 픽셀이라 배율마다 굽는 굵기가 다르다)
	const bool bFirst = !Atlas.IsValid();
	if (bFirst) Atlas = MakeShared<FPDAtlas>();
	Atlas->BakedScale = FMath::Max(0.25f, ViewScale);
	UTextureRenderTarget2D* RT = Cast<UTextureRenderTarget2D>(Atlas->RT);
	if (!RT)
	{
		RT = FWidgetRenderer::CreateTargetFor(FVector2D(AtlasTex, AtlasTex), TF_Bilinear, true);
		if (!RT) { Atlas.Reset(); return; }
		RT->ClearColor = FLinearColor::Transparent;
		RT->AddToRoot(); // 게임 내내 쓴다
		Atlas->RT = RT;
		Atlas->Painter = SNew(SPDAtlasPainter).Owner(this);
		Atlas->Renderer = MakeShared<FWidgetRenderer>(true, true);
	}
	BakeTiled(RT, AtlasTex, (AtlasN + AtlasCols - 1) / AtlasCols * (int32)(AtlasCell * AtlasK), Atlas->Painter.ToSharedRef()); // 처음과 화면 크기가 바뀔 때만
	Atlas->Frames = 0;
	Atlas->BgSig = 0; // 판 배경도 다시
	if (!bFirst) return;
	Atlas->Brushes.SetNum(AtlasN);
	for (int32 I = 0; I < AtlasN; ++I)
	{
		FSlateBrush& B = Atlas->Brushes[I];
		B.SetResourceObject(RT);
		B.ImageSize = FVector2D(AtlasCell * AtlasK, AtlasCell * AtlasK);
		B.DrawAs = ESlateBrushDrawType::Image;
		const FVector2f C = AtlasCellCenter(I) * AtlasK, H(AtlasCell * AtlasK * 0.5f, AtlasCell * AtlasK * 0.5f);
		B.SetUVRegion(FBox2f((C - H) / (float)AtlasTex, (C + H) / (float)AtlasTex));
	}
	// 보조 붓 (D-143): 칸 가운데의 일부만 쓰는 붓. Wd·Hd 는 설계 크기, (Ox, Oy) 는 칸 가운데 기준 왼쪽 위
	auto Sub = [&](int32 I, float Ox, float Oy, float Wd, float Hd) {
		FSlateBrush B;
		B.SetResourceObject(RT);
		B.ImageSize = FVector2D(Wd * AtlasK, Hd * AtlasK);
		B.DrawAs = ESlateBrushDrawType::Image;
		const FVector2f C = AtlasCellCenter(I), A = (C + FVector2f(Ox, Oy)) * AtlasK, Bm = (C + FVector2f(Ox + Wd, Oy + Hd)) * AtlasK;
		B.SetUVRegion(FBox2f(A / (float)AtlasTex, Bm / (float)AtlasTex));
		return B;
	};
	// 빗금: 칸(48) 안에서 들인 만큼만 (겹 0~2 의 들임 4·9·14, 변마다 들이거나 말거나)
	for (int32 Layer = 0; Layer < 3; ++Layer)
		for (int32 Mask = 0; Mask < 16; ++Mask)
		{
			const float M = 4.f + 5.f * Layer;
			const float L = Mask & 1 ? M : 0.f, R = Mask & 2 ? M : 0.f, U = Mask & 4 ? M : 0.f, D = Mask & 8 ? M : 0.f;
			Atlas->HatchBrushes.Add(Sub(AtlasHatch, -CS / 2.f + L, -CS / 2.f + U, CS - L - R, CS - U - D));
		}
	Atlas->BracketBrush = Sub(AtlasBracket, -CS / 2.f - BracketPad, -CS / 2.f - BracketPad, CS + BracketPad * 2.f, CS + BracketPad * 2.f);
	Atlas->LineBrush = Sub(AtlasLine, -LineHalf - 6.f, -LineH / 2.f, LineHalf * 2.f + 12.f, LineH);
}

// 조각 굽기 (D-143): 512 조각마다 그려서 큰 텍스처의 제자리로 복사, 조각마다 렌더가 끝날 때까지 기다린다
void SPDBoardWidget::BakeTiled(UObject* DstObj, int32 W, int32 H, const TSharedRef<SWidget>& Painter)
{
	UTextureRenderTarget2D* Dst = Cast<UTextureRenderTarget2D>(DstObj);
	if (!Atlas.IsValid() || !Dst) return;
	if (!Atlas->Scratch)
	{
		UTextureRenderTarget2D* S = FWidgetRenderer::CreateTargetFor(FVector2D(BakeTile, BakeTile), TF_Bilinear, true);
		if (!S) return;
		S->ClearColor = FLinearColor::Transparent;
		S->AddToRoot();
		Atlas->Scratch = S;
		Atlas->ScratchRenderer = MakeShared<FWidgetRenderer>(true, true);
	}
	UTextureRenderTarget2D* Scratch = Cast<UTextureRenderTarget2D>(Atlas->Scratch);
	FTextureRenderTargetResource* SR = Scratch->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DR = Dst->GameThread_GetRenderTargetResource();
	for (int32 Y = 0; Y < H; Y += BakeTile)
		for (int32 X = 0; X < W; X += BakeTile)
		{
			Atlas->TileOrigin = FVector2f((float)X, (float)Y);
			Painter->Invalidate(EInvalidateWidgetReason::Paint); // 지난 조각 그림을 다시 쓰지 않게
			Atlas->ScratchRenderer->DrawWidget(Scratch, Painter, FVector2D(BakeTile, BakeTile), 0.016f);
			const int32 Wc = FMath::Min(BakeTile, W - X), Hc = FMath::Min(BakeTile, H - Y);
			ENQUEUE_RENDER_COMMAND(PDCopyBakeTile)([SR, DR, X, Y, Wc, Hc](FRHICommandListImmediate& RHICmdList)
			{
				FRHITexture* ST = SR->TextureRHI.GetReference(); // 화면에서 쓰는 쪽 텍스처 (그리는 쪽과 다를 수 있다)
				FRHITexture* DT = DR->TextureRHI.GetReference();
				if (!ST || !DT) return;
				RHICmdList.Transition({FRHITransitionInfo(ST, ERHIAccess::Unknown, ERHIAccess::CopySrc), FRHITransitionInfo(DT, ERHIAccess::Unknown, ERHIAccess::CopyDest)});
				FRHICopyTextureInfo Info;
				Info.Size = FIntVector(Wc, Hc, 1);
				Info.DestPosition = FIntVector(X, Y, 0);
				RHICmdList.CopyTexture(ST, DT, Info);
				RHICmdList.Transition({FRHITransitionInfo(ST, ERHIAccess::CopySrc, ERHIAccess::SRVMask), FRHITransitionInfo(DT, ERHIAccess::CopyDest, ERHIAccess::SRVMask)});
			});
			FlushRenderingCommands(); // 다음 조각이 같은 조각 텍스처를 쓰기 전에
		}
	Atlas->TileOrigin = FVector2f(0.f, 0.f);
}

// 판 배경 서명: 길이 바뀌면 (새 판) 다시 굽는다
uint64 SPDBoardWidget::BoardBgSig() const
{
	uint64 S = 1469598103934665603ull;
	for (const FPDPoint& P : Game.Path.Points) S = (S ^ (uint64)(P.X * 1000.f) ^ ((uint64)(P.Y * 1000.f) << 24)) * 1099511628211ull;
	return S ^ Game.Path.Points.size();
}

void SPDBoardWidget::BakeBoardBg(uint64 Sig)
{
	if (!Atlas.IsValid()) return;
	if (!Atlas->BgRT)
	{
		UTextureRenderTarget2D* RT = FWidgetRenderer::CreateTargetFor(FVector2D(BgW * AtlasK, BgH * AtlasK), TF_Bilinear, true);
		if (!RT) return;
		RT->ClearColor = FLinearColor::Transparent;
		RT->AddToRoot();
		Atlas->BgRT = RT;
		Atlas->BgBrush.SetResourceObject(RT);
		Atlas->BgBrush.ImageSize = FVector2D(BgW * AtlasK, BgH * AtlasK);
		Atlas->BgBrush.DrawAs = ESlateBrushDrawType::Image;
		Atlas->BgPainter = SNew(SPDAtlasPainter).Owner(this).Mode(1);
		Atlas->BgRenderer = MakeShared<FWidgetRenderer>(true, true);
	}
	BakeTiled(Atlas->BgRT, (int32)(BgW * AtlasK), (int32)(BgH * AtlasK), Atlas->BgPainter.ToSharedRef());
	Atlas->BgSig = Sig;
	Atlas->BgFrames = 0;
}

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
	GPDDebug.Add(InDeltaTime); // 디버그 프레임 표시 (자르기 전 실제 프레임 시간)
#if !UE_BUILD_SHIPPING
	// 자동 부하 시험 (D-138): -pdstress 로 켜면 2초 뒤 F4 판 + 10배속 + 프레임 제한 없음으로 웨이브를 계속 돌리고 45초 뒤 끈다 (로그 PDPERF)
	{
		static const bool bAutoStress = FParse::Param(FCommandLine::Get(), TEXT("pdstress"));
		static float StressT = -1.f;
		if (bAutoStress)
		{
			if (StressT < 0.f && Clock > 2.f)
			{
				Game.Phase = EPDPhase::Build;
				PDDebugStress(Game);
				GPDDebug.bShow = true;
				if (FParse::Param(FCommandLine::Get(), TEXT("pdnoworld"))) Settings.bWorldFx = false; // 비교용: 나이아가라 끔
				if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("t.MaxFPS"))) V->Set(0.f, ECVF_SetByCode);
				if (IConsoleVariable* V = IConsoleManager::Get().FindConsoleVariable(TEXT("r.VSync"))) V->Set(0, ECVF_SetByCode);
				StressT = 0.f;
			}
			else if (StressT >= 0.f)
			{
				StressT += InDeltaTime;
				Game.Speed = 10;
				if (Game.Phase == EPDPhase::Build) Game.StartWave();
				if (Game.Phase == EPDPhase::Reward) Game.TakeReward(-1);
				static bool bShot = false; // 게임 안 스크린샷 (Saved/Screenshots): 부하 시험 화면 확인용
				if (!bShot && StressT > 8.f) { bShot = true; FScreenshotRequest::RequestScreenshot(TEXT("PDStress"), true, false); }
				if (StressT > 45.f) FPlatformMisc::RequestExit(false);
			}
		}
	}
#endif
	const double TickT0 = FPlatformTime::Seconds();
	const float Dt = FMath::Min(InDeltaTime, 0.05f);
	Clock += Dt;
	if (!bTree && !bUpg && !bMenu) Game.Tick(Dt); // 트리·강화·메뉴 창을 여는 동안은 멈춘다
	TreeBuyT = FMath::Max(0.f, TreeBuyT - Dt);
	HandleEvents();
	// 이펙트 상한 (D-138): 10배속에선 사건이 열 배라 쌓이기만 한다. 넘치면 오래된 것(곧 사라질 것)부터 버린다
	{
		auto Cap = [](auto& A, int32 Max) { if (A.Num() > Max) A.RemoveAt(0, A.Num() - Max, EAllowShrinking::No); };
		Cap(Rings, 70);
		Cap(Flares, 60);
		Cap(Shards, 120);
		Cap(Beams, 80);
		Cap(Particles, 700);
		Cap(Floats, 60);
	}
	UpdateFx(Dt);
	if (!Settings.bShake) { Shake = 0.f; FlashT = 0.f; } // 설정: 화면 흔들림·번쩍임 끔
	if (Game.Discovered != SavedDiscovered || FMath::Max(Game.BestWave, Game.Wave) > SavedBest) SaveProgress(); // 도감·최고 기록이 바뀌면 바로 저장
	if (Sel >= (int32)Game.Hand.size()) Sel = -1;
	UpdatePreview();
	JGroups = Game.JokerGroups();
	if (FeedJoker >= 0 && (!Game.Towers.count(FeedJoker) || !Game.Towers.at(FeedJoker).IsJoker() || Game.Phase == EPDPhase::Reward || Sel >= 0)) FeedJoker = -1; // 조커가 없어졌거나 카드를 고름
	// 고르는 사이 족보가 바뀌었거나(교체·다른 합성) 보상 화면이 뜨면 고르기 취소
	if (!PickCells.empty() && (Game.Phase == EPDPhase::Reward || FindPickIndex() < 0)) PickCells.clear();
	// 판 흔들림 + 스프라이트 아틀라스 (D-140): 처음 한 번 굽고, 렌더가 끝난 몇 프레임 뒤부터 쓴다
	{
		const float SK = Shake * 14.f;
		ShakeOff = FVector2f(FMath::FRandRange(-SK, SK), FMath::FRandRange(-SK, SK));
		// 패키지에선 시작 직후 렌더러·글꼴이 덜 준비돼 빈 그림이 구워질 수 있다: 1초에 굽고 4초에 한 번 더 (D-143)
		if (!Atlas.IsValid() && Clock > 1.f) BakeAtlas();
		else if (Atlas.IsValid() && !Atlas->bRebaked && Clock > 4.f) { Atlas->bRebaked = true; BakeAtlas(); }
		else if (Atlas.IsValid() && Atlas->Ready() && FMath::Abs(ViewScale - Atlas->BakedScale) > 0.04f) BakeAtlas(); // 화면 크기가 바뀜
		else if (Atlas.IsValid())
		{
			++Atlas->Frames;
			++Atlas->BgFrames;
			const uint64 BgS = BoardBgSig();
			if (Atlas->Ready() && Atlas->BgSig != BgS && !Game.Path.Points.empty()) BakeBoardBg(BgS); // 새 판: 판 배경 다시 굽기
		}
	}
	GPDDebug.TickMs = FMath::Lerp(GPDDebug.TickMs, (float)((FPlatformTime::Seconds() - TickT0) * 1000.0), 0.1f);
	if (GPDDebug.bShow && GPDDebug.Acc < Dt) // 디버그 표시가 켜져 있으면 1초마다 로그에도 (Saved/Logs, D-135)
	{
		UE_LOG(LogTemp, Log, TEXT("PDPERF fps=%.1f ms=%.1f low=%.1f paint=%.2f tick=%.2f lines=%d points=%d enemies=%d shots=%d towers=%d particles=%d rings=%d speed=%d parts=%d/%d/%d/%d/%d/%d static=%d game=%.1f render=%.1f rhi=%.1f gpu=%.1f wave=%d mode=%.2f floats=%d voicesfx=%d"),
			GPDDebug.Fps, GPDDebug.Ms, GPDDebug.WorstShown, GPDDebug.PaintMs, GPDDebug.TickMs, GPDDebug.Lines, GPDDebug.Points, (int32)Game.Enemies.size(), (int32)Game.Projectiles.size(), (int32)Game.Towers.size(), Particles.Num(), Rings.Num(), Game.Speed, GPDDebug.PartLines[0], GPDDebug.PartLines[1], GPDDebug.PartLines[2], GPDDebug.PartLines[3], GPDDebug.PartLines[4], GPDDebug.PartLines[5], GPDDebug.StaticPaints,
			FPlatformTime::ToMilliseconds(GGameThreadTime), FPlatformTime::ToMilliseconds(GRenderThreadTime), FPlatformTime::ToMilliseconds(GRHIThreadTime), FPlatformTime::ToMilliseconds(RHIGetGPUFrameCycles()), Game.Wave, GPDDebug.ModeMs, Floats.Num(), WorldFx.Num());
		GPDDebug.StaticPaints = 0;
	}
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
	if (Sel >= 0) return false; // 손패 카드를 고른 동안은 합성 버튼·표시를 띄우지 않는다 (놓기에 집중, D-132)
	const int32 Key = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
	const std::vector<int>& Cells = Game.Synergies[I].Cells;
	if (Key >= 0 && std::find(Cells.begin(), Cells.end(), Key) != Cells.end()) return true;
	return Game.CanFuse(I) && InBox(Mouse, FuseButtonCenter(Game.Synergies, I), FuseBtnW(FuseSyn(Game, I)), FuseH);
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
	// 먹이기 모드 (D-131): [먹이기] 버튼 → 먹이기, 다른 조커 → 그 조커로, 그 밖 → 끄기 (판 위를 눌렀으면 거기서 끝)
	if (FeedJoker >= 0)
	{
		if (!bRight)
			for (const FPDFeed& Fd : Game.FeedTargets())
			{
				if (Fd.Joker != FeedJoker || !InBox(P, FeedButtonCenter(Game, Fd.Syn), FeedBtnW(Game.Synergies[Fd.Syn]), FuseH)) continue;
				PendingFuseCells.Reset();
				for (int K : Game.Synergies[Fd.Syn].Cells) if (!Game.Towers.at(K).IsJoker()) PendingFuseCells.Add(CellCenter(K));
				if (!Game.Feed(Fd.Syn)) PendingFuseCells.Reset();
				return;
			}
		const int32 Cell = CellAt(P);
		if (!bRight && Cell >= 0 && Cell != FeedJoker && Game.Towers.count(Cell) && Game.Towers.at(Cell).IsJoker()) { FeedJoker = Cell; UiSound(EPDSound::Move); return; }
		FeedJoker = -1;
		if (bRight || Cell >= 0) return;
	}
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
	// 카드를 고르지 않고 조커를 클릭: 먹이기 모드 (D-131)
	if (const int32 Cell = CellAt(P); Cell >= 0 && Sel < 0 && Game.Towers.count(Cell) && Game.Towers.at(Cell).IsJoker()) { FeedJoker = Cell; UiSound(EPDSound::Move); return; }
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
	GPDDebugHpMul = 1.f; // 새 판에서는 부하 시험 배율 끔
	GPDDebugCountMul = 1;
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

#if !UE_BUILD_SHIPPING
// 부하 시험 치트 (D-134, F4, 쉬핑 빌드에는 없음): 길 둘레 2칸 안 빈칸을 무작위 카드로 채우고(9칸마다 조커) 웨이브 20 직전으로.
// 타워 배율 ×6 (조커 ×6·레벨 5), 적 체력·마리 수 ×3, 목숨 999 · 골드 99,999: 적이 길을 따라 오래 맞으며 타격 이펙트를 많이 낸다
static void PDDebugStress(FPDGame& G)
{
	std::mt19937 R(1234);
	G.Towers.clear();
	G.Enemies.clear();
	G.Projectiles.clear();
	int N = 0;
	for (int Y = 0; Y < Rows; ++Y)
		for (int X = 0; X < Cols; ++X)
		{
			if (!G.Buildable(X, Y)) continue;
			bool bNear = false;
			for (int DY = -2; DY <= 2 && !bNear; ++DY)
				for (int DX = -2; DX <= 2 && !bNear; ++DX)
				{
					const int NX = X + DX, NY = Y + DY;
					bNear = NX >= 0 && NY >= 0 && NX < Cols && NY < Rows && G.Path.IsPath(NX, NY);
				}
			if (!bNear) continue;
			FPDTower T;
			T.X = X;
			T.Y = Y;
			T.Card = {2 + (int)(R() % 13), (EPDSuit)(R() % 4)};
			T.Mult = 6.f;
			if (++N % 9 == 0) { T.Joker = (int)(R() % (int)EPDJoker::Count); T.JLevel = 5; T.Mult = 6.f; T.Card = {0, EPDSuit::Wild}; }
			G.Towers[CellKey(X, Y)] = T;
		}
	G.Synergies = ApplyEffects(G.Towers, G.CharmMask(), &G.Mods);
	G.Wave = 19;
	G.Lives = 999;
	G.Gold = 99999;
	G.Phase = EPDPhase::Build;
	G.AutoStartT = -1.f;
	GPDDebugHpMul = 3.f;  // 적 체력·마리 수 3배 (타격 이펙트 검수, D-135)
	GPDDebugCountMul = 3;
}
#endif

FReply SPDBoardWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.IsRepeat()) return FReply::Handled();
	const FKey K = InKeyEvent.GetKey();
	if (K == EKeys::F3) { GPDDebug.bShow = !GPDDebug.bShow; return FReply::Handled(); } // 디버그 표시 (어느 창에서나)
	if (K == EKeys::F6) { GPDDebug.bNoAtlas = !GPDDebug.bNoAtlas; return FReply::Handled(); } // 구운 그림 ↔ 선 비교
#if !UE_BUILD_SHIPPING
	if (K == EKeys::F4 && (Game.Phase == EPDPhase::Build || Game.Phase == EPDPhase::Wave)) { PDDebugStress(Game); Sel = -1; PickCells.clear(); FeedJoker = -1; GPDDebug.bShow = true; return FReply::Handled(); }
#endif
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
	else if (K == EKeys::Escape) { if (FeedJoker >= 0) FeedJoker = -1; else if (Sel >= 0) Sel = -1; else OpenMenu(); }
	else if (K == EKeys::G) DoFuse(HoveredSynergy());
	else if (K == EKeys::J) bCodex = !bCodex;
	else if (K == EKeys::K) { bTree = true; Sel = -1; }
	else if (K == EKeys::U) { bUpg = true; Sel = -1; }
	else if (K == EKeys::F) Game.Speed = NextSpeed(Game.Speed);
	else if (K == EKeys::M) SetSound(!Settings.bSound);
	else return FReply::Unhandled();
	return FReply::Handled();
}
