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
#include "HAL/FileManager.h"
#include "Misc/Paths.h"
#include "Slate/WidgetRenderer.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/StrongObjectPtr.h"
#include "RenderingThread.h"
#include "RHICommandList.h"
#include "TextureResource.h"
#include "Engine/Texture2D.h"
#if WITH_EDITOR
#include "UObject/Package.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#endif
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
	// 미리 만든 에셋이 있으면 그걸 쓴다 (D-145): 굽지 않으니 시작·창 크기 바뀜에도 멈추지 않는다
	UObject* Res = nullptr;
	if (bFirst && !bForceBake)
		if (UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *FString::Printf(TEXT("/Game/Sprites/T_PDAtlas_v%d.T_PDAtlas_v%d"), AtlasVersion, AtlasVersion), nullptr, LOAD_NoWarn | LOAD_Quiet))
		{
			Tex->AddToRoot();
			Res = Tex;
			Atlas->bAsset = true;
			Atlas->bRebaked = true;
			Atlas->Frames = 3;
		}
	UTextureRenderTarget2D* RT = Res ? nullptr : Cast<UTextureRenderTarget2D>(Atlas->RT);
	if (!Res)
	{
	if (!RT)
	{
		RT = FWidgetRenderer::CreateTargetFor(FVector2D(AtlasTex, AtlasTex), TF_Bilinear, false);
		if (!RT) { Atlas.Reset(); return; }
		RT->ClearColor = FLinearColor::Transparent;
		RT->AddToRoot(); // 게임 내내 쓴다
		Atlas->RT = RT;
		Atlas->Painter = SNew(SPDAtlasPainter).Owner(this);
		Atlas->Renderer = MakeShared<FWidgetRenderer>(true, true);
	}
	Res = RT;
	BakeTiled(RT, AtlasTex, (AtlasN + AtlasCols - 1) / AtlasCols * (int32)(AtlasCell * AtlasK), Atlas->Painter.ToSharedRef()); // 처음과 화면 크기가 바뀔 때만
	Atlas->Tex = PixelsToTexture(Atlas->Tex, RT, AtlasTex, AtlasTex, &Atlas->LastPixels);
	Res = Atlas->Tex ? Atlas->Tex : RT;
	Atlas->Frames = 0;
	}
	Atlas->BgSig = 0; // 판 배경도 다시
	if (!bFirst) return;
	UObject* RTRes = Res; // 붓이 쓰는 텍스처: 에셋 또는 구운 렌더 타깃
	Atlas->Brushes.SetNum(AtlasN);
	for (int32 I = 0; I < AtlasN; ++I)
	{
		FSlateBrush& B = Atlas->Brushes[I];
		B.SetResourceObject(RTRes);
		B.ImageSize = FVector2D(AtlasCell * AtlasK, AtlasCell * AtlasK);
		B.DrawAs = ESlateBrushDrawType::Image;
		const FVector2f C = AtlasCellCenter(I) * AtlasK, H(AtlasCell * AtlasK * 0.5f, AtlasCell * AtlasK * 0.5f);
		B.SetUVRegion(FBox2f((C - H) / (float)AtlasTex, (C + H) / (float)AtlasTex));
	}
	// 보조 붓 (D-143): 칸 가운데의 일부만 쓰는 붓. Wd·Hd 는 설계 크기, (Ox, Oy) 는 칸 가운데 기준 왼쪽 위
	auto Sub = [&](int32 I, float Ox, float Oy, float Wd, float Hd) {
		FSlateBrush B;
		B.SetResourceObject(RTRes);
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
	Atlas->LineAABrush = Sub(AtlasLineAA, -LineAAW / 2.f + 4.f, -LineAAH / 2.f, LineAAW - 8.f, LineAAH); // 양 끝 4px 은 빼고 (가로로 늘여도 끝이 흐려지지 않게)
}

// 조각 굽기 (D-143): 512 조각마다 그려서 큰 텍스처의 제자리로 복사, 조각마다 렌더가 끝날 때까지 기다린다
void SPDBoardWidget::BakeTiled(UObject* DstObj, int32 W, int32 H, const TSharedRef<SWidget>& Painter)
{
	UTextureRenderTarget2D* Dst = Cast<UTextureRenderTarget2D>(DstObj);
	if (!Atlas.IsValid() || !Dst) return;
	if (!Atlas->Scratch)
	{
		UTextureRenderTarget2D* S = FWidgetRenderer::CreateTargetFor(FVector2D(BakeTile, BakeTile), TF_Bilinear, false);
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

// 아틀라스 에셋 만들기 (D-145, 에디터 F8): 화면 배율 1 굵기로 새로 굽고 렌더 타깃을 텍스처로 만들어 Content/Sprites/T_PDAtlas 에 저장.
// 다음 실행부터 게임은 이 에셋을 쓴다. 그림 코드를 고치면 F8 을 다시 누른다
void SPDBoardWidget::SaveAtlasAsset()
{
#if WITH_EDITOR
	// 에셋을 쓰는 중이면 굽는 판으로 새로 만든다
	if (Atlas.IsValid() && Atlas->bAsset) { Atlas.Reset(); }
	bForceBake = true;
	const float OldScale = ViewScale;
	ViewScale = 1.f; // 굵기 기준: 화면 배율 1
	BakeAtlas();
	ViewScale = OldScale;
	bForceBake = false;
	UTextureRenderTarget2D* RT = Atlas.IsValid() ? Cast<UTextureRenderTarget2D>(Atlas->RT) : nullptr;
	if (!RT) return;
	const FString AssetName = FString::Printf(TEXT("T_PDAtlas_v%d"), AtlasVersion);
	const FString PkgName = TEXT("/Game/Sprites/") + AssetName;
	UPackage* Pkg = CreatePackage(*PkgName);
	// 렌더 타깃 바이트를 그대로 텍스처 원본으로 (sRGB 로 저장된 그대로, 알파는 미리 곱한 값). ConstructTexture2D 는 감마를 바꿔 하얗게 떠서 쓰지 않는다
	const TArray<FColor>& Px = Atlas->LastPixels;
	if (Px.Num() != AtlasTex * AtlasTex) return;
	Pkg->FullyLoad(); // 게임이 불러 둔 옛 에셋이 있으면 다 불러야 덮어쓸 수 있다
	UTexture2D* Tex = FindObject<UTexture2D>(Pkg, *AssetName);
	if (!Tex) Tex = NewObject<UTexture2D>(Pkg, *AssetName, RF_Public | RF_Standalone);
	Tex->PreEditChange(nullptr);
	Tex->Source.Init(AtlasTex, AtlasTex, 1, 1, TSF_BGRA8, reinterpret_cast<const uint8*>(Px.GetData()));
	Tex->SRGB = true;
	Tex->CompressionSettings = TC_EditorIcon; // 무손실 (선이 가늘어 압축하면 번진다)
	Tex->LODGroup = TEXTUREGROUP_UI;
	Tex->MipGenSettings = TMGS_SimpleAverage; // 작게 그려도 깨끗하게
	Tex->NeverStream = true;
	Tex->PostEditChange();
	Pkg->MarkPackageDirty();
	FSavePackageArgs Args;
	Args.TopLevelFlags = RF_Public | RF_Standalone;
	const bool bOk = UPackage::SavePackage(Pkg, Tex, *FPackageName::LongPackageNameToFilename(PkgName, FPackageName::GetAssetPackageExtension()), Args);
	UE_LOG(LogTemp, Log, TEXT("PDATLAS asset saved: %s"), bOk ? TEXT("ok") : TEXT("FAILED"));
	Floats.Add({bOk ? FString(TEXT("아틀라스 에셋 저장: Content/Sprites/")) + AssetName : FString(TEXT("아틀라스 에셋 저장 실패")), FVector2f(DesignW / 2.f, DesignH / 2.f), 0.f, 2.5f, 18.f, bOk ? Hex(TEXT("9cffb0")) : Hex(TEXT("ff6a6a"))});
#endif
}

// 렌더 타깃 바이트 → sRGB 텍스처 (D-145). 선형 렌더 타깃에 감마로 그려 바이트가 곧 화면 값이고, sRGB 로 읽어야 화면과 같다
// (렌더 타깃을 바로 읽으면 감마가 한 번 더 걸려 옅은 선이 하얗게 뜬다). 처음엔 새로 만들고, 다시 구울 땐 같은 텍스처를 고쳐 쓴다
UObject* SPDBoardWidget::PixelsToTexture(UObject* Existing, UObject* RTObj, int32 W, int32 H, TArray<FColor>* OutPx)
{
	UTextureRenderTarget2D* RT = Cast<UTextureRenderTarget2D>(RTObj);
	TArray<FColor> Px;
	if (!RT || !RT->GameThread_GetRenderTargetResource()->ReadPixels(Px) || Px.Num() != RT->SizeX * RT->SizeY) return Existing;
	if (RT->SizeX != W || RT->SizeY != H) { W = RT->SizeX; H = RT->SizeY; }
	UTexture2D* Tex = Cast<UTexture2D>(Existing);
	if (!Tex || Tex->GetSizeX() != W || Tex->GetSizeY() != H)
	{
		Tex = UTexture2D::CreateTransient(W, H, PF_B8G8R8A8);
		if (!Tex) return Existing;
		Tex->SRGB = true;
		Tex->Filter = TF_Bilinear;
		Tex->LODGroup = TEXTUREGROUP_UI;
		Tex->AddToRoot();
	}
	if (FTexturePlatformData* PD = Tex->GetPlatformData())
	{
		void* Dst = PD->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
		FMemory::Memcpy(Dst, Px.GetData(), (SIZE_T)W * H * sizeof(FColor));
		PD->Mips[0].BulkData.Unlock();
		Tex->UpdateResource();
	}
	if (OutPx) *OutPx = MoveTemp(Px);
	return Tex;
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
		UTextureRenderTarget2D* RT = FWidgetRenderer::CreateTargetFor(FVector2D(BgW * AtlasK, BgH * AtlasK), TF_Bilinear, false);
		if (!RT) return;
		RT->ClearColor = FLinearColor::Transparent;
		RT->AddToRoot();
		Atlas->BgRT = RT;
		Atlas->BgBrush.ImageSize = FVector2D(BgW * AtlasK, BgH * AtlasK);
		Atlas->BgBrush.DrawAs = ESlateBrushDrawType::Image;
		Atlas->BgPainter = SNew(SPDAtlasPainter).Owner(this).Mode(1);
		Atlas->BgRenderer = MakeShared<FWidgetRenderer>(true, true);
	}
	BakeTiled(Atlas->BgRT, (int32)(BgW * AtlasK), (int32)(BgH * AtlasK), Atlas->BgPainter.ToSharedRef());
	Atlas->BgTex = PixelsToTexture(Atlas->BgTex, Atlas->BgRT, (int32)(BgW * AtlasK), (int32)(BgH * AtlasK));
	Atlas->BgBrush.SetResourceObject(Atlas->BgTex ? Atlas->BgTex : Atlas->BgRT);
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
FString PDRankPath() { return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Ranking.txt")); } // 개인 랭킹 (D-168)
FString PDRunSavePath() { return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), TEXT("Run.txt")); } // 중단 저장 (D-162)
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
		GConfig->GetInt(PDSetSection, TEXT("CardTheme"), Settings.CardTheme, GGameUserSettingsIni);
		GConfig->GetInt(PDSetSection, TEXT("Lang"), Settings.Lang, GGameUserSettingsIni);
		Settings.CardTheme = FMath::Clamp(Settings.CardTheme, 0, 2);
		GPDCardTheme = Settings.CardTheme;
		GConfig->GetBool(PDSetSection, TEXT("VSync"), Settings.bVSync, GGameUserSettingsIni);
	}
	Settings.FrameCap = FMath::Clamp(Settings.FrameCap, 0, 2);
	if (Settings.Lang < 0 || Settings.Lang >= (int32)EPDLang::Count) Settings.Lang = LangFromSystem(); // 처음 켜면 시스템 언어 (D-159)
#if !UE_BUILD_SHIPPING
	{ int32 L = -1; if (FParse::Value(FCommandLine::Get(), TEXT("pdlang="), L)) Settings.Lang = FMath::Clamp(L, 0, (int32)EPDLang::Count - 1); } // 시험용: -pdlang=1 (영어) 등
#endif
	GPDLang = Settings.Lang;
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
	LoadRanking(); // 개인 랭킹 (D-168)
	bHasRunSave = FPaths::FileExists(PDRunSavePath()); // 이어할 판 (D-162)
	if (bHasRunSave) TitleSel = (int32)ETitleItem::Continue;
	else TitleSel = (int32)ETitleItem::Start;
	if (FFileHelper::LoadFileToStringArray(Lines, *PDProgressPath()))
		for (const FString& L : Lines)
		{
			FString Key, Val;
			if (!L.Split(TEXT("="), &Key, &Val)) continue;
			if (Key == TEXT("Discovered")) Game.Discovered |= (uint32)FCString::Strtoui64(*Val, nullptr, 10);
			else if (Key == TEXT("BestWave")) Game.BestWave = FMath::Max(Game.BestWave, FCString::Atoi(*Val));
			else if (Key == TEXT("Tutorial")) bTutorialSeen = FCString::Atoi(*Val) != 0;
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
	GConfig->SetInt(PDSetSection, TEXT("CardTheme"), Settings.CardTheme, GGameUserSettingsIni);
	GConfig->SetInt(PDSetSection, TEXT("Lang"), Settings.Lang, GGameUserSettingsIni);
	GConfig->Flush(false, GGameUserSettingsIni);
}

void SPDBoardWidget::SaveProgress()
{
	SavedDiscovered = Game.Discovered;
	SavedBest = FMath::Max3(SavedBest, Game.BestWave, Game.Wave);
	FFileHelper::SaveStringToFile(FString::Printf(TEXT("Discovered=%u\nBestWave=%d\nTutorial=%d\n"), SavedDiscovered, SavedBest, bTutorialSeen ? 1 : 0), *PDProgressPath());
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
	case ESetRow::Lang: { const int32 N = (int32)EPDLang::Count; Settings.Lang = ((Settings.Lang + (Delta == 0 ? 1 : Delta)) % N + N) % N; GPDLang = Settings.Lang; SaveSettings(); break; }
	case ESetRow::Window: ApplyWindowMode(Settings.WindowMode + (Delta == 0 ? 1 : Delta)); break;
	case ESetRow::CardTheme: Settings.CardTheme = ((Settings.CardTheme + (Delta == 0 ? 1 : Delta)) % 3 + 3) % 3; GPDCardTheme = Settings.CardTheme; SaveSettings(); break;
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
			if (Row == (int32)ESetRow::Lang) { Settings.Lang = I; GPDLang = I; SaveSettings(); UiSound(EPDSound::Move); }
			else if (Row == (int32)ESetRow::Window) { ApplyWindowMode(I); UiSound(EPDSound::Move); }
			else if (Row == (int32)ESetRow::CardTheme) { Settings.CardTheme = I; GPDCardTheme = I; SaveSettings(); UiSound(EPDSound::Move); }
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
	const bool bOk = Sel >= 0 && Sel < (int32)Game.Hand.size() && HoverX >= 0 && Game.CanPlace(HoverX, HoverY) && Game.Phase != EPDPhase::Title && Game.Phase != EPDPhase::Reward;
	const FString Key = bOk ? FString::Printf(TEXT("%d|%d|%d|%d|%d|%d"), Sel, HoverX, HoverY, (int32)Game.Towers.size(), Game.Discarded, (int32)Game.Hand.size()) : FString();
	if (Key == PreviewKey) return;
	PreviewKey = Key;
	Preview = bOk ? Game.PreviewSynergies(Sel, HoverX, HoverY) : std::vector<FPDSynergy>();
}

void SPDBoardWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
	GPDDebug.Add(InDeltaTime); // 디버그 프레임 표시 (자르기 전 실제 프레임 시간)
	if (LoadT > 0.f) { LoadT -= InDeltaTime; if (LoadT <= 0.f) { LoadT = -1.f; if (!bLoadContinue || !ContinueRun()) RestartRun(); bLoadContinue = false; } } // 로딩 화면이 끝나면 새 판 또는 이어하기 (D-153, D-162)
	// 중단 저장 (D-162): 웨이브 직전 상태가 새로 생기면 바로, 준비 단계는 1초마다 바뀌었으면. 게임 오버면 지운다
	if (Game.SaveSerial != LastSaveSerial) { LastSaveSerial = Game.SaveSerial; WriteRunSave(Game.PreWaveSave); }
	if (Game.Phase == EPDPhase::Build && LoadT <= 0.f)
	{
		SaveCooldown -= InDeltaTime;
		if (SaveCooldown <= 0.f) { SaveCooldown = 1.f; WriteRunSave(Game.SaveRun()); }
	}
	else if (Game.Phase == EPDPhase::Over && bHasRunSave) DeleteRunSave();
	// 개인 랭킹 (D-168): 판 위력 최고값을 따라가고, 게임 오버가 되면 한 번 기록
	if (Game.Phase == EPDPhase::Build || Game.Phase == EPDPhase::Wave) { PeakPower = FMath::Max(PeakPower, PowerSum); bRankRecorded = false; }
	if (Game.Phase == EPDPhase::Over && !bRankRecorded) { bRankRecorded = true; RecordRun(); }
	// 자리바꿈 (D-149): 횟수가 없거나 보상·끝 화면이면 끈다, 고른 타워가 사라지면 다시 고르기
	if (bSwapMode && (Game.Swaps <= 0 || (Game.Phase != EPDPhase::Build && Game.Phase != EPDPhase::Wave))) { bSwapMode = false; SwapFrom = -1; }
	if (SwapFrom >= 0 && !Game.Towers.count(SwapFrom)) SwapFrom = -1;
	// 족보 필터 (D-148): 판에서 그 족보가 없어지면 (합성 등) 끈다
	if (HandFilter == JokerFilterId ? JGroups.empty() : (HandFilter >= 0 && std::none_of(Game.Synergies.begin(), Game.Synergies.end(), [this](const FPDSynergy& S) { return (int32)S.Kind == HandFilter; }))) HandFilter = -1;
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
				LoadT = -1.f;
				PDDebugStress(Game);
				{ int32 TP = -1; if (FParse::Value(FCommandLine::Get(), TEXT("pdtut="), TP)) { bTutorial = true; TutPage = FMath::Clamp(TP, 0, TutPages - 1); } } // 시험용: 튜토리얼 N쪽 띄우기
				if (FParse::Param(FCommandLine::Get(), TEXT("pdpause"))) bPaused = true; // 시험용: 일시정지 화면
				if (FParse::Param(FCommandLine::Get(), TEXT("pdwalk"))) { Game.Towers.clear(); Game.RebuildMods(); } // 시험용: 타워 없이 적이 걷는 모습
				if (FParse::Param(FCommandLine::Get(), TEXT("pdhandtip")) && Game.Hand.size() > 0) Mouse = FVector2f(HandCardX(0, (int32)Game.Hand.size()) + CardW / 2.f, HandY + CardH / 2.f); // 시험용: 첫 손패 카드에 마우스
				if (FParse::Param(FCommandLine::Get(), TEXT("pdbosses"))) // 시험용: 보스 13종을 길 위에 줄 세움 (느리게 걷는다)
				{
					const EPDEnemy Bs[13] = {EPDEnemy::ShieldBoss, EPDEnemy::SplitBoss, EPDEnemy::RegenBoss, EPDEnemy::Charger, EPDEnemy::Summoner, EPDEnemy::Phantom, EPDEnemy::Warlord, EPDEnemy::Berserker, EPDEnemy::Blinker, EPDEnemy::Devourer, EPDEnemy::Warden, EPDEnemy::Hexer, EPDEnemy::Breaker};
					for (int32 I = 0; I < 13; ++I) { FPDEnemy E; E.Type = Bs[I]; E.D = 3.f + I * 2.6f; E.Hp = E.MaxHp = E.BaseHp = 1e12f; E.Speed = 0.05f; E.Id = 900000 + I; E.Wave = 20; Game.Enemies.push_back(E); }
				}
				{ int32 RT = -1; if (FParse::Value(FCommandLine::Get(), TEXT("pdrank="), RT)) { bRanking = true; RankTab = RT; if (Ranking.Num() == 0) for (int32 I = 0; I < 13; ++I) { FRankEntry E; E.Wave = 40 - I * 2; E.Kills = 2000 - I * 97; E.Power = 900000.f / (I + 1); E.Time = FDateTime::Now().ToUnixTimestamp() - I * 86400; Ranking.Add(E); } LastRankPos = 2; } } // 시험용: 랭킹 창 (기록이 없으면 보기용 가짜, 저장 안 함)
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
				static const bool bSlowTest = FParse::Param(FCommandLine::Get(), TEXT("pdslow")); // 시험용: 모든 적 둔화
				if (bSlowTest) for (FPDEnemy& E : Game.Enemies) { E.SlowT = FMath::Max(E.SlowT, 0.5f); E.SlowAmt = FMath::Max(E.SlowAmt, 0.3f); }
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
	if (bPaused && Game.Phase != EPDPhase::Build && Game.Phase != EPDPhase::Wave) bPaused = false; // 게임 오버·보상·제목이면 풀린다
	if (!bTree && !bUpg && !bMenu && !bTutorial && !bPaused) Game.Tick(Dt); // 트리·강화·메뉴 창을 여는 동안은 멈춘다
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
	if (!InCraft()) CraftPicks.clear();
	else if (!CraftPicks.empty() && PickCells.empty()) // 사라진 재료는 뺀다 (교체·합성으로 족보가 바뀜)
	{
		const std::vector<FPDJokerNode> Ns = Game.JokerNodes();
		CraftPicks.erase(std::remove_if(CraftPicks.begin(), CraftPicks.end(), [&Ns](const std::vector<int>& Pk) {
			return std::none_of(Ns.begin(), Ns.end(), [&Pk](const FPDJokerNode& N) { std::vector<int> A = N.Cells, B = Pk; std::sort(A.begin(), A.end()); std::sort(B.begin(), B.end()); return A == B; }); }), CraftPicks.end());
	}
	// 판 흔들림 + 스프라이트 아틀라스 (D-140): 처음 한 번 굽고, 렌더가 끝난 몇 프레임 뒤부터 쓴다
	{
		const float SK = Shake * 14.f;
		ShakeOff = FVector2f(FMath::FRandRange(-SK, SK), FMath::FRandRange(-SK, SK));
		// 패키지에선 시작 직후 렌더러·글꼴이 덜 준비돼 빈 그림이 구워질 수 있다: 1초에 굽고 4초에 한 번 더 (D-143)
		if (!Atlas.IsValid() && Clock > 1.f) BakeAtlas();
		else if (Atlas.IsValid() && !Atlas->bRebaked && Clock > 4.f) { Atlas->bRebaked = true; BakeAtlas(); }
		else if (Atlas.IsValid() && !Atlas->bAsset && Atlas->Ready() && FMath::Abs(ViewScale - Atlas->BakedScale) > 0.04f) BakeAtlas(); // 화면 크기가 바뀜
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
	if (bPickJoker) // 조커 만들기 (D-195): 고른 재료가 아직 레시피와 맞고 칸도 같아야
	{
		const std::vector<int> Idx = CraftNodeIdx();
		if (Idx.size() != CraftPicks.size() || Game.JokerCraftResult(Idx) == EPDJoker::Count) return -1;
		std::vector<int> U;
		for (const std::vector<int>& Pk : CraftPicks) U.insert(U.end(), Pk.begin(), Pk.end());
		std::sort(U.begin(), U.end());
		U.erase(std::unique(U.begin(), U.end()), U.end());
		return SameCells(U, PickCells) ? 0 : -1;
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
	const std::vector<int> Nodes = CraftNodeIdx();
	PendingFuseCells.Reset();
	for (const std::vector<int>& Pk : CraftPicks) for (int K : Pk) PendingFuseCells.Add(CellCenter(K));
	if (!Game.JokerCraft(Nodes, Cell)) { PendingFuseCells.Reset(); return; }
	CraftPicks.clear();
	HandFilter = -1;
}

bool SPDBoardWidget::InCraft() const { return HandFilter == JokerFilterId; }

std::vector<int> SPDBoardWidget::CraftNodeIdx() const
{
	std::vector<int> Out;
	const std::vector<FPDJokerNode> Ns = Game.JokerNodes();
	for (const std::vector<int>& Pk : CraftPicks)
		for (int32 I = 0; I < (int32)Ns.size(); ++I)
			if (SameCells(Ns[I].Cells, Pk)) { Out.push_back(I); break; }
	return Out;
}

// 칸 하나에 재료가 여럿이면(카드 한 장이 족보 둘에 듦) 큰 족보부터, 다시 누르면 다음 것, 마지막이면 빼기
namespace
{
std::vector<int> CraftCands(const std::vector<FPDJokerNode>& Ns, int32 Cell)
{
	std::vector<int> C;
	for (int32 I = 0; I < (int32)Ns.size(); ++I)
		if (std::find(Ns[I].Cells.begin(), Ns[I].Cells.end(), Cell) != Ns[I].Cells.end()) C.push_back(I);
	std::stable_sort(C.begin(), C.end(), [&Ns](int A, int B) { return HandBaseValue(Ns[A].Kind) > HandBaseValue(Ns[B].Kind); });
	return C;
}
}

std::vector<int> SPDBoardWidget::CraftHoverCells() const
{
	if (HoverX < 0) return {};
	const std::vector<FPDJokerNode> Ns = Game.JokerNodes();
	const std::vector<int> C = CraftCands(Ns, CellKey(HoverX, HoverY));
	for (size_t I = 0; I < C.size(); ++I)
		for (const std::vector<int>& Pk : CraftPicks)
			if (SameCells(Pk, Ns[C[I]].Cells)) return I + 1 < C.size() ? Ns[C[I + 1]].Cells : std::vector<int>();
	return C.empty() || CraftPicks.size() >= 3 ? std::vector<int>() : Ns[C[0]].Cells;
}

void SPDBoardWidget::CraftClick(int32 Cell)
{
	const std::vector<FPDJokerNode> Ns = Game.JokerNodes();
	const std::vector<int> C = CraftCands(Ns, Cell);
	if (C.empty()) { UiSound(EPDSound::Denied); return; }
	for (size_t I = 0; I < C.size(); ++I)
		for (size_t P = 0; P < CraftPicks.size(); ++P)
			if (SameCells(CraftPicks[P], Ns[C[I]].Cells))
			{
				CraftPicks.erase(CraftPicks.begin() + P);
				if (I + 1 < C.size() && std::none_of(CraftPicks.begin(), CraftPicks.end(), [&](const std::vector<int>& Pk) { return SameCells(Pk, Ns[C[I + 1]].Cells); }))
					CraftPicks.push_back(Ns[C[I + 1]].Cells);
				UiSound(EPDSound::Move);
				return;
			}
	if (CraftPicks.size() >= 3) { UiSound(EPDSound::Denied); return; }
	CraftPicks.push_back(Ns[C[0]].Cells);
	UiSound(EPDSound::Move);
}

void SPDBoardWidget::CraftConfirm()
{
	const std::vector<int> Idx = CraftNodeIdx();
	if (Idx.size() != CraftPicks.size() || Game.JokerCraftResult(Idx) == EPDJoker::Count) { UiSound(EPDSound::Denied); return; }
	std::vector<int> U;
	for (const std::vector<int>& Pk : CraftPicks) U.insert(U.end(), Pk.begin(), Pk.end());
	std::sort(U.begin(), U.end());
	U.erase(std::unique(U.begin(), U.end()), U.end());
	StartPick(U, true);
}

void SPDBoardWidget::ToggleCraft()
{
	if (InCraft()) { HandFilter = -1; UiSound(EPDSound::Move); return; }
	if (JGroups.empty()) { UiSound(EPDSound::Denied); return; }
	HandFilter = JokerFilterId;
	CraftPicks.clear();
	Sel = -1;
	FeedJoker = -1;
	bSwapMode = false;
	UiSound(EPDSound::Move);
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
		if (FilterPass(Game.Synergies[I].Kind))
			for (int K : Game.Synergies[I].Cells) if (K == Key) return I;
	return -1;
}

bool SPDBoardWidget::SynRevealed(int32 I) const
{
	if (I < 0 || I >= (int32)Game.Synergies.size()) return false;
	if (!PickCells.empty() && !bPickJoker && FindPickIndex() == I) return true; // 합성 칸 고르는 중인 족보
	if (Sel >= 0) return false; // 손패 카드를 고른 동안은 합성 버튼·표시를 띄우지 않는다 (놓기에 집중, D-132)
	if (!FilterPass(Game.Synergies[I].Kind)) return false; // 족보 필터 중엔 그 족보만 (D-148)
	if (HandFilter >= 0) return true; // 필터로 고른 족보는 마우스를 올린 것처럼 모두 펼친다 (합성 표시·버튼)
	const int32 Key = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
	const std::vector<int>& Cells = Game.Synergies[I].Cells;
	if (Key >= 0 && std::find(Cells.begin(), Cells.end(), Key) != Cells.end()) return true;
	return Game.CanFuse(I) && InBox(Mouse, FuseButtonCenter(Game.Synergies, I), FuseBtnW(FuseSyn(Game, I)), FuseH);
}

bool SPDBoardWidget::JokerRevealed(const FPDJokerGroup& Gr) const
{
	if (!PickCells.empty() && bPickJoker && SameCells(Gr.Cells, PickCells)) return true;
	if (HandFilter == JokerFilterId && Sel < 0 && PickCells.empty()) return true; // 조커 필터 (D-152): 모두 펼친다
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
	else if (Game.Phase == EPDPhase::Title && LoadT <= 0.f) // 시작 화면: 마우스가 올라간 버튼
	{
		for (int32 I = 0; I < (int32)ETitleItem::Count; ++I)
			if ((I != (int32)ETitleItem::Continue || bHasRunSave) && InRect(Mouse, TitleBtnPos(I, bHasRunSave).X, TitleBtnPos(I, bHasRunSave).Y, TitleBtnW, TitleBtnH)) TitleSel = I;
	}
	const int32 Cell = CellAt(Mouse);
	HoverX = Cell >= 0 ? Cell % Cols : -1;
	HoverY = Cell >= 0 ? Cell / Cols : -1;
	return FReply::Handled();
}

void SPDBoardWidget::Click(FVector2f P, bool bRight)
{
	if (LoadT > 0.f) return; // 로딩 중엔 입력 없음 (D-153)
	if (bRanking) // 랭킹 (D-168): 탭 · 닫기, 창 밖·오른쪽 클릭 = 닫기
	{
		for (int32 I = 0; I < 2; ++I)
			if (InRect(P, DesignW / 2.f - RankTabW - 6.f + I * (RankTabW + 12.f), RankTabY, RankTabW, RankTabH)) { RankTab = I; UiSound(EPDSound::Move); return; }
		if (bRight || !InRect(P, TutX, TutY, TutW, TutH) || InRect(P, TutNextX, TutBtnY, TutBtnW, TutBtnH)) bRanking = false;
		return;
	}
	if (bTutorial) // 튜토리얼 (D-166): 이전 · 다음/시작하기 · 건너뛰기, 오른쪽 클릭 = 이전
	{
		if (bRight) { TutPage = FMath::Max(0, TutPage - 1); return; }
		if (InRect(P, TutNextX, TutBtnY, TutBtnW, TutBtnH)) { if (TutPage + 1 >= TutPages) CloseTutorial(); else { ++TutPage; UiSound(EPDSound::Move); } }
		else if (TutPage > 0 && InRect(P, TutPrevX, TutBtnY, TutBtnW, TutBtnH)) { --TutPage; UiSound(EPDSound::Move); }
		else if (TutPage + 1 < TutPages && InRect(P, TutSkipX, TutBtnY, TutBtnW, TutBtnH)) CloseTutorial();
		return;
	}
	if (bSettings) { if (bRight) bSettings = false; else SettingsClick(P); return; }
	if (bMenu)
	{
		const int32 Item = MenuHoverAt(P);
		if (Item >= 0) MenuSelect(Item);
		else if (!InRect(P, MenuX - 30.f, MenuItemY0 - 80.f, MenuW + 60.f, (int32)EMenuItem::Count * (MenuItemH + MenuGap) + 110.f)) bMenu = false; // 창 바깥 클릭 = 닫기
		return;
	}
	if (Game.Phase == EPDPhase::Title) // 시작 화면 (D-153): 버튼 클릭
	{
		for (int32 I = 0; I < (int32)ETitleItem::Count && !bRight; ++I)
			if (InRect(P, TitleBtnPos(I, bHasRunSave).X, TitleBtnPos(I, bHasRunSave).Y, TitleBtnW, TitleBtnH)) { TitleSelect(I); break; }
		return;
	}
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
		if (bRight || InRect(P, CdxCloseX, CdxCloseY, CdxCloseW, CdxCloseH)) { bUpg = false; return; } // 닫기 버튼 (트리·도감과 같은 자리)
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
	// 합성 위치 고르는 중: 재료 칸 클릭 = 그 칸으로, 같은 버튼 다시 = 계속 고르기 (기본 칸 없음, D-194), 그 밖(오른쪽 클릭 포함) = 취소
	if (!PickCells.empty())
	{
		const int32 Key = CellAt(P);
		bool bSameBtn = false;
		if (bPickJoker) bSameBtn = InRect(P, CraftBtnX, CraftBtnY, CraftBtnW, CraftBtnH);
		else if (const int32 Pi = FindPickIndex(); Pi >= 0) { const FVector2f B = FuseButtonCenter(Game.Synergies, Pi); bSameBtn = InBox(P, B, FuseBtnW(FuseSyn(Game, Pi)), FuseH); }
		if (!bRight && bSameBtn) return;
		else if (!bRight && Key >= 0 && std::find(PickCells.begin(), PickCells.end(), Key) != PickCells.end()) FinishPick(Key);
		else PickCells.clear();
		return;
	}
	// 자리바꿈 모드 (D-149): 타워 클릭 → 바꿀 칸 클릭 (타워면 서로 바꾸고 빈 칸이면 옮김). 같은 타워 다시 = 고르기 취소, 판 밖·오른쪽 클릭 = 끄기
	if (bSwapMode)
	{
		const int32 Cell = CellAt(P);
		if (!bRight && Cell >= 0)
		{
			if (SwapFrom < 0) { if (Game.Towers.count(Cell)) { SwapFrom = Cell; UiSound(EPDSound::Move); } else UiSound(EPDSound::Denied); }
			else if (Cell == SwapFrom) SwapFrom = -1;
			else if (Game.SwapTowers(SwapFrom, Cell)) { bSwapMode = false; SwapFrom = -1; }
			else UiSound(EPDSound::Denied);
			return;
		}
		bSwapMode = false;
		SwapFrom = -1;
		return;
	}
	if (!bRight && InRect(P, CraftBtnX, CraftBtnY, CraftBtnW, CraftBtnH) && (InCraft() || !JGroups.empty())) // 조커 만들기 (D-195): 고른 게 맞으면 만들기, 아니면 켜기/끄기
	{
		if (InCraft() && CraftPicks.size() >= 2 && Game.JokerCraftResult(CraftNodeIdx()) != EPDJoker::Count) CraftConfirm();
		else ToggleCraft();
		return;
	}
	if (!bRight && Game.Swaps > 0 && InRect(P, SwapBtnX, SwapBtnY, SwapBtnW, SwapBtnH))
	{
		bSwapMode = true; SwapFrom = -1; Sel = -1; FeedJoker = -1;
		UiSound(EPDSound::Move);
		return;
	}
	// 족보 필터 (D-148): 왼쪽 족보 목록 한 줄 클릭 = 그 족보만 보기, 다시 클릭 = 끄기
	if (!bRight && InRect(P, LX + 8.f, HandRowY0, LW - 16.f, HandRowH * HandRowsN))
	{
		const int32 Hi = FMath::Clamp((int32)((P.Y - HandRowY0) / HandRowH), 0, HandRowsN - 1);
		const bool bAny = Hi == JokerFilterId ? !JGroups.empty() : std::any_of(Game.Synergies.begin(), Game.Synergies.end(), [Hi](const FPDSynergy& S) { return (int32)S.Kind == Hi; });
		if (Hi == JokerFilterId) { ToggleCraft(); return; } // 조커 줄 = 조커 만들기 (D-195)
		if (HandFilter == Hi) HandFilter = -1;
		else if (bAny) { HandFilter = Hi; Sel = -1; FeedJoker = -1; }
		else { UiSound(EPDSound::Denied); return; }
		UiSound(EPDSound::Move);
		return;
	}
	if (!bRight && InRect(P, MenuBtnX, MenuBtnY, MenuBtnW, MenuBtnH)) { bMenu = true; MenuSel = 0; Sel = -1; return; }
	if (!bRight && InRect(P, TreeBtnX, TreeBtnY, TreeBtnW, TreeBtnH)) { bTree = true; Sel = -1; return; }
	if (!bRight && InRect(P, UpgOpenX, UpgOpenY, UpgOpenW, UpgOpenH)) { bUpg = true; Sel = -1; return; }
	if (!bRight && InRect(P, CodexBtnX, CodexBtnY, CodexBtnW, CodexBtnH)) { bCodex = true; Sel = -1; return; }
	// 위쪽 띠 버튼 (D-119): 속도, 설정
	if (!bRight && InRect(P, SpeedBtnX, HudBtnY, HudBtnS, HudBtnS)) { Game.Speed = NextSpeed(Game.Speed); UiSound(EPDSound::Move); return; }
	if (!bRight && InRect(P, PauseBtnX, HudBtnY, HudBtnS, HudBtnS) && (Game.Phase == EPDPhase::Build || Game.Phase == EPDPhase::Wave)) { bPaused = !bPaused; UiSound(EPDSound::Move); return; } // 일시정지 (D-169)
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
	// 조커 만들기 모드 (D-195): 판 칸 클릭 = 재료 고르기/빼기, 판에서 오른쪽 클릭 = 끄기
	if (InCraft())
	{
		const int32 Cell = CellAt(P);
		if (Cell >= 0) { if (bRight) HandFilter = -1; else CraftClick(Cell); return; }
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
	int32 HandHit = -1;
	for (int32 I = 0; I < (int32)Game.Hand.size(); ++I)
	{
		const float X = HandCardX(I, (int32)Game.Hand.size());
		if (InRect(P, X, HandY - 12.f, CardW, CardH + 12.f)) HandHit = I;
	}
	if (bRight && HandHit < 0 && HandFilter >= 0 && !InCraft() && CellAt(P) >= 0) { HandFilter = -1; return; } // 합성 모드 끄기 (D-199)
	if (bRight)
	{
		if (HandHit >= 0) { Game.Discard(HandHit); if (Sel == HandHit) Sel = -1; else if (Sel > HandHit) --Sel; }
		else Sel = -1;
		return;
	}
	if (HandHit >= 0) { Sel = Sel == HandHit ? -1 : HandHit; UiSound(EPDSound::Move); return; }
	// 족보 필터 중 그 족보 칸 클릭 = 그 족보 합성 (D-148)
	// 합성 (D-199): 왼쪽 족보 목록에서 고른 족보의 칸을 클릭 = 그 칸으로 바로 합성 (같은 종류 족보는 카드를 나눠 쓰지 않아 칸 하나로 정해진다)
	if (const int32 Cell = CellAt(P); Cell >= 0 && Sel < 0 && HandFilter >= 0 && !InCraft())
	{
		for (int32 I = 0; I < (int32)Game.Synergies.size(); ++I)
		{
			const FPDSynergy& S = Game.Synergies[I];
			if ((int32)S.Kind != HandFilter || !Game.CanFuse(I)) continue;
			const std::vector<int> FC = FuseSyn(Game, I).Cells; // 조커 칸은 빠진다 (D-128)
			if (std::find(FC.begin(), FC.end(), Cell) == FC.end()) continue;
			DoFuse(I, Cell);
			return;
		}
		UiSound(EPDSound::Denied);
		return;
	}
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

// 개인 랭킹 (D-168): 한 줄에 웨이브 처치 위력 시각. 웨이브 → 처치 → 위력 순으로 높은 것부터, 20개까지
void SPDBoardWidget::LoadRanking()
{
	Ranking.Reset();
	TArray<FString> Lines;
	if (!FFileHelper::LoadFileToStringArray(Lines, *PDRankPath())) return;
	for (const FString& L : Lines)
	{
		TArray<FString> P;
		L.ParseIntoArrayWS(P);
		if (P.Num() < 4) continue;
		FRankEntry E;
		E.Wave = FCString::Atoi(*P[0]); E.Kills = FCString::Atoi(*P[1]); E.Power = FCString::Atof(*P[2]); E.Time = FCString::Atoi64(*P[3]);
		Ranking.Add(E);
	}
	Ranking.Sort([](const FRankEntry& A, const FRankEntry& B) { return A.Wave != B.Wave ? A.Wave > B.Wave : A.Kills != B.Kills ? A.Kills > B.Kills : A.Power > B.Power; });
	if (Ranking.Num() > RankMax) Ranking.SetNum(RankMax);
}

void SPDBoardWidget::SaveRanking() const
{
	FString S;
	for (const FRankEntry& E : Ranking) S += FString::Printf(TEXT("%d %d %.0f %lld\n"), E.Wave, E.Kills, E.Power, E.Time);
	FFileHelper::SaveStringToFile(S, *PDRankPath());
}

void SPDBoardWidget::RecordRun()
{
	FRankEntry E;
	E.Wave = Game.Wave; E.Kills = Game.Kills; E.Power = FMath::Max(PeakPower, PowerSum); E.Time = FDateTime::Now().ToUnixTimestamp();
	auto Better = [](const FRankEntry& A, const FRankEntry& B) { return A.Wave != B.Wave ? A.Wave > B.Wave : A.Kills != B.Kills ? A.Kills > B.Kills : A.Power > B.Power; };
	int32 Pos = 0;
	while (Pos < Ranking.Num() && !Better(E, Ranking[Pos])) ++Pos;
	LastRankPos = Pos < RankMax ? Pos : -1;
	if (LastRankPos < 0) return;
	Ranking.Insert(E, Pos);
	if (Ranking.Num() > RankMax) Ranking.SetNum(RankMax);
	SaveRanking();
}

// 중단 저장 (D-162)
void SPDBoardWidget::WriteRunSave(const std::string& S)
{
	if (S.empty() || S == LastRunSave) return;
	LastRunSave = S;
	if (FFileHelper::SaveStringToFile(FString(UTF8_TO_TCHAR(S.c_str())), *PDRunSavePath())) bHasRunSave = true;
}

void SPDBoardWidget::DeleteRunSave()
{
	IFileManager::Get().Delete(*PDRunSavePath(), false, true, true);
	bHasRunSave = false;
	LastRunSave.clear();
}

bool SPDBoardWidget::ContinueRun()
{
	FString Text;
	if (!FFileHelper::LoadFileToString(Text, *PDRunSavePath()) || !Game.LoadRun(std::string(TCHAR_TO_UTF8(*Text)))) { DeleteRunSave(); return false; }
	LastRunSave = TCHAR_TO_UTF8(*Text);
	LastSaveSerial = Game.SaveSerial;
	Sel = -1;
	bTree = bUpg = bCodex = bMenu = bSettings = false;
	PowerRef = 0.f;
	PickCells.clear();
	Particles.Empty();
	Flares.Empty();
	Shards.Empty();
	UiSound(EPDSound::Start);
	return true;
}

void SPDBoardWidget::RestartRun()
{
	Game.NewGame(true);
	PeakPower = 0.f; LastRankPos = -1; bPaused = false;
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

// 튜토리얼 (D-166)
void SPDBoardWidget::OpenTutorial(bool bThenStart)
{
	bTutorial = true;
	TutPage = 0;
	bTutThenStart = bThenStart;
	Sel = -1;
	UiSound(EPDSound::Move);
}

void SPDBoardWidget::CloseTutorial()
{
	bTutorial = false;
	if (!bTutorialSeen) { bTutorialSeen = true; SaveProgress(); }
	if (bTutThenStart) { bTutThenStart = false; bLoadContinue = false; LoadT = LoadDur; LoadTip = FMath::RandRange(0, 5); UiSound(EPDSound::Start); }
}

void SPDBoardWidget::TitleSelect(int32 Item)
{
	switch ((ETitleItem)Item)
	{
	case ETitleItem::Continue: if (!bHasRunSave) return; bLoadContinue = true; LoadT = LoadDur; LoadTip = FMath::RandRange(0, 5); UiSound(EPDSound::Start); break;
	case ETitleItem::Start:
		if (!bTutorialSeen) { OpenTutorial(true); break; } // 처음이면 튜토리얼부터 (D-166)
		bLoadContinue = false; LoadT = LoadDur; LoadTip = FMath::RandRange(0, 5); UiSound(EPDSound::Start); break;
	case ETitleItem::Tutorial: OpenTutorial(false); break;
	case ETitleItem::Ranking: bRanking = true; RankTab = 0; UiSound(EPDSound::Move); break;
	case ETitleItem::Settings: bSettings = true; SetSel = 0; UiSound(EPDSound::Move); break;
	case ETitleItem::Quit:
		if (OnQuit) OnQuit();
		else FPlatformMisc::RequestExit(false);
		break;
	default: break;
	}
}

void SPDBoardWidget::MenuSelect(int32 Item)
{
	switch ((EMenuItem)Item)
	{
	case EMenuItem::Resume: bMenu = false; break;
	case EMenuItem::NewGame: RestartRun(); break;
	case EMenuItem::Tutorial: bMenu = false; OpenTutorial(false); break;
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
#if WITH_EDITOR
	if (K == EKeys::F8) { SaveAtlasAsset(); return FReply::Handled(); } // 아틀라스 에셋 만들기 (에디터)
#endif
#if !UE_BUILD_SHIPPING
	if (K == EKeys::F4 && (Game.Phase == EPDPhase::Build || Game.Phase == EPDPhase::Wave)) { PDDebugStress(Game); Sel = -1; PickCells.clear(); FeedJoker = -1; GPDDebug.bShow = true; return FReply::Handled(); }
#endif
	const FKey Nums[9] = {EKeys::One, EKeys::Two, EKeys::Three, EKeys::Four, EKeys::Five, EKeys::Six, EKeys::Seven, EKeys::Eight, EKeys::Nine};
	if (LoadT > 0.f) return FReply::Handled(); // 로딩 중 (D-153)
	if (bRanking) // 랭킹 (D-168): ←→ 탭, Esc·Enter 닫기
	{
		if (K == EKeys::Left || K == EKeys::Right || K == EKeys::A || K == EKeys::D || K == EKeys::Tab) RankTab = 1 - RankTab;
		else if (K == EKeys::Escape || K == EKeys::Enter) bRanking = false;
		return FReply::Handled();
	}
	if (bTutorial) // 튜토리얼 (D-166): ←→ 넘기기, Enter 다음, Esc 닫기
	{
		if (K == EKeys::Left || K == EKeys::A) TutPage = FMath::Max(0, TutPage - 1);
		else if (K == EKeys::Right || K == EKeys::D || K == EKeys::Enter || K == EKeys::SpaceBar) { if (TutPage + 1 >= TutPages) CloseTutorial(); else ++TutPage; }
		else if (K == EKeys::Escape) CloseTutorial();
		return FReply::Handled();
	}
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
		const int32 N = (int32)ETitleItem::Count; // 시작 화면 (D-153): 위아래 고르기, Enter 결정
		const int32 Lo = bHasRunSave ? 0 : 1, M = N - Lo; // 이어할 판이 없으면 '이어하기'는 건너뛴다 (D-162)
		if (K == EKeys::Up || K == EKeys::W) { TitleSel = Lo + (TitleSel - Lo + M - 1) % M; UiSound(EPDSound::Move); }
		else if (K == EKeys::Down || K == EKeys::S) { TitleSel = Lo + (TitleSel - Lo + 1) % M; UiSound(EPDSound::Move); }
		else if (K == EKeys::Enter || K == EKeys::SpaceBar) TitleSelect(TitleSel);
		else return FReply::Unhandled();
		return FReply::Handled();
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
		if (K == EKeys::G || K == EKeys::Enter) // 마우스가 재료 칸 위에 있을 때만 그 칸으로 (기본 칸 없음, D-194)
		{
			const int32 HK = HoverX >= 0 ? CellKey(HoverX, HoverY) : -1;
			if (std::find(PickCells.begin(), PickCells.end(), HK) != PickCells.end()) FinishPick(HK);
			return FReply::Handled();
		}
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
	if (InCraft() && (K == EKeys::G || K == EKeys::Enter)) { CraftConfirm(); return FReply::Handled(); } // 조커 만들기: 고른 재료로 (D-195)
	if (K == EKeys::C) { ToggleCraft(); return FReply::Handled(); }
	if (K == EKeys::SpaceBar && (Game.Phase == EPDPhase::Wave || bPaused)) bPaused = !bPaused; // 스페이스: 웨이브 중엔 일시정지/계속 (D-183)
	else if (K == EKeys::SpaceBar || K == EKeys::Enter) Game.StartWave(); // 준비 단계에선 웨이브 시작
	else if (K == EKeys::Escape) { if (bSwapMode) { bSwapMode = false; SwapFrom = -1; } else if (FeedJoker >= 0) FeedJoker = -1; else if (Sel >= 0) Sel = -1; else if (HandFilter >= 0) HandFilter = -1; else OpenMenu(); }
	else if (K == EKeys::X) { if (bSwapMode) { bSwapMode = false; SwapFrom = -1; } else if (Game.Swaps > 0) { bSwapMode = true; SwapFrom = -1; Sel = -1; FeedJoker = -1; } } // 자리바꿈 (D-149)
	else if (K == EKeys::J) bCodex = !bCodex;
	else if (K == EKeys::K) { bTree = true; Sel = -1; }
	else if (K == EKeys::U) { bUpg = true; Sel = -1; }
	else if (K == EKeys::F) Game.Speed = NextSpeed(Game.Speed);
	else if (K == EKeys::P || K == EKeys::Pause) bPaused = !bPaused; // 일시정지 (D-169)
	else if (K == EKeys::M) SetSound(!Settings.bSound);
	else return FReply::Unhandled();
	return FReply::Handled();
}
