#include "PDGameMode.h"

#include "Async/Async.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundWaveProcedural.h"
#include "PDBoardWidget.h"
#include "PDBoardDraw.h"
#include "Misc/ScopeExit.h"

APDGameMode::APDGameMode()
{
	DefaultPawnClass = nullptr; // 3D 캐릭터 없음
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bTickEvenWhenPaused = true;
}

namespace
{
constexpr int32 PDSampleRate = 44100;
constexpr int32 PDMaxVoices = 24;   // 동시에 나는 소리 최대 (넘으면 오래된 것부터 끔)
constexpr float PDMasterVolume = 0.8f;
}

void APDGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!BoardWidget.IsValid()) return;
	const double ModeT0 = FPlatformTime::Seconds();
	ON_SCOPE_EXIT { GPDDebug.ModeMs = FMath::Lerp(GPDDebug.ModeMs, (float)((FPlatformTime::Seconds() - ModeT0) * 1000.0), 0.1f); };
	UpdateWorldView();
	PlayWorldFx();
	for (const PD::FPDSoundEvent& E : BoardWidget->TakeSounds()) PlayGameSound(E);
	UpdateMusic();
	// 끝난 소리 정리
	for (int32 I = Voices.Num() - 1; I >= 0; --I)
	{
		UAudioComponent* V = Voices[I];
		if (!V || !V->IsPlaying())
		{
			if (V) V->DestroyComponent();
			Voices.RemoveAt(I);
		}
	}
}

void APDGameMode::PlayGameSound(const PD::FPDSoundEvent& Event)
{
	const int32 Key = PD::SoundKey(Event.Sound, Event.A, Event.B);
	if (!PcmCache.Contains(Key))
	{
		const std::vector<int16_t> Samples = PD::SynthesizeSound(Event.Sound, Event.A, Event.B, PDSampleRate);
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized((int32)(Samples.size() * sizeof(int16_t)));
		FMemory::Memcpy(Bytes.GetData(), Samples.data(), Bytes.Num());
		PcmCache.Add(Key, MoveTemp(Bytes));
	}
	const TArray<uint8>& Pcm = PcmCache[Key];
	if (Pcm.Num() == 0) return;
	// 소리 줄이기 (D-138): 소리 하나마다 사운드 객체를 새로 만들어 비싸다. 10배속에선 초당 수백 개가 쌓여 게임 스레드가 막히던 것.
	// 한 프레임 6개까지, 같은 소리는 0.05초에 한 번
	{
		static uint64 Frame = 0;
		static int32 InFrame = 0;
		static TMap<int32, double> Last;
		static int32 MinorInFrame = 0;
		static double LastMinor = -1.0;
		if (Frame != GFrameCounter) { Frame = GFrameCounter; InFrame = 0; MinorInFrame = 0; }
		const double Now = FPlatformTime::Seconds();
		double& L = Last.FindOrAdd(Key, -1.0);
		// 발사·타격 (D-155)은 따로: 한 프레임 1개, 전체 0.045초·같은 소리 0.09초 간격 (중요한 소리 몫을 뺏지 않게, 10배속에서도 초당 20개 남짓)
		if (Event.Sound == PD::EPDSound::Zap) // 전기 (D-156): 따로, 같은 소리 0.12초 간격 (프레임 몫은 쓰지 않는다)
		{
			if (Now - L < 0.12) return;
			L = Now;
		}
		else if (Event.Sound == PD::EPDSound::Shot || Event.Sound == PD::EPDSound::Hit)
		{
			if (MinorInFrame >= 1 || Now - LastMinor < 0.045 || Now - L < 0.09) return;
			++MinorInFrame;
			LastMinor = Now;
			L = Now;
		}
		else
		{
			if (InFrame >= 6 || Now - L < 0.05) return;
			++InFrame;
			L = Now;
		}
	}

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(PDSampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = (float)Pcm.Num() / (sizeof(int16_t) * PDSampleRate);
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());

	UAudioComponent* Voice = UGameplayStatics::CreateSound2D(this, Wave, (BoardWidget.IsValid() ? BoardWidget->GetVolume() : PDMasterVolume) * Event.Volume, 1.f, 0.f, nullptr, false, false);
	if (!Voice) return;
	Voice->bIsUISound = true;
	Voice->Play();
	Voices.Add(Voice);
	while (Voices.Num() > PDMaxVoices)
	{
		if (UAudioComponent* Old = Voices[0])
		{
			Old->Stop();
			Old->DestroyComponent();
		}
		Voices.RemoveAt(0);
	}
}

void APDGameMode::UpdateMusic()
{
	if (MusicPcm.Num() == 0)
	{
		if (!MusicJob.IsValid() || !MusicJob.IsReady()) return;
		const std::vector<int16_t> Samples = MusicJob.Get();
		MusicPcm.SetNumUninitialized((int32)(Samples.size() * sizeof(int16_t)));
		FMemory::Memcpy(MusicPcm.GetData(), Samples.data(), MusicPcm.Num());
		if (MusicPcm.Num() == 0) return;
		MusicWave = NewObject<USoundWaveProcedural>(this);
		MusicWave->SetSampleRate(PDSampleRate);
		MusicWave->NumChannels = 1;
		MusicWave->Duration = INDEFINITELY_LOOPING_DURATION;
		MusicWave->bLooping = false;
		MusicWave->QueueAudio(MusicPcm.GetData(), MusicPcm.Num());
		MusicWave->QueueAudio(MusicPcm.GetData(), MusicPcm.Num());
		MusicVoice = UGameplayStatics::CreateSound2D(this, MusicWave, 1.f, 1.f, 0.f, nullptr, false, false);
		if (MusicVoice) { MusicVoice->bIsUISound = true; MusicVoice->Play(); }
	}
	if (!MusicWave || !MusicVoice || !BoardWidget.IsValid()) return;
	if (MusicWave->GetAvailableAudioByteCount() < MusicPcm.Num() / 2) MusicWave->QueueAudio(MusicPcm.GetData(), MusicPcm.Num()); // 반 바퀴 남으면 한 바퀴 더
	MusicVoice->SetVolumeMultiplier(FMath::Max(0.001f, BoardWidget->GetMusicVolume()));
}

void APDGameMode::BeginPlay()
{
	Super::BeginPlay();
	MusicJob = Async(EAsyncExecution::ThreadPool, []() { return PD::SynthesizeMusic(PDSampleRate); }); // 배경 음악 합성 (D-117)
	if (!GEngine || !GEngine->GameViewport) return;

	SAssignNew(BoardWidget, SPDBoardWidget);
	// 메뉴의 [게임 종료]: 패키지 게임은 꺼지고, 에디터(PIE)에서는 플레이만 멈춘다
	TWeakObjectPtr<APDGameMode> WeakThis(this);
	BoardWidget->OnQuit = [WeakThis]()
	{
		APDGameMode* Self = WeakThis.Get();
		UWorld* World = Self ? Self->GetWorld() : nullptr;
		APlayerController* PC = World ? World->GetFirstPlayerController() : nullptr;
		if (Self) UKismetSystemLibrary::QuitGame(Self, PC, EQuitPreference::Quit, false);
		else FPlatformMisc::RequestExit(false);
	};
	GEngine->GameViewport->AddViewportWidgetContent(BoardWidget.ToSharedRef(), 10);

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(BoardWidget);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	FSlateApplication::Get().SetKeyboardFocus(BoardWidget);
	SetupWorldView();
}

// ---- 월드 이펙트 층 (D-092)
// 카메라는 +X 를 바라본다: 화면 오른쪽 = 월드 +Y, 화면 위 = 월드 +Z (나이아가라 기본 중력 -Z 가 화면 아래로 떨어짐).
// 위젯 좌표(아래가 +Y)는 (0, X, -Y) 로 옮긴다. 이펙트는 X = 0 평면에서 재생한다.
namespace
{
constexpr float PDCameraDistance = 1000.f;
// 나이아가라 에셋이 받는 사용자 변수 (에셋을 만들 때 이 이름으로 노출한다)
const FName PDFxColor(TEXT("User.Color")); // HDR 색 (1 넘으면 블룸)
const FName PDFxScale(TEXT("User.Scale")); // 크기·세기 배율
const FName PDFxDir(TEXT("User.Dir"));     // 튀는 방향 (월드, 단위 벡터)
// SPDBoardWidget::EWorldFx 순서대로 에셋 이름과 HDR 밝기 배율
const TCHAR* const PDFxNames[] = {TEXT("NS_PD_Hit"), TEXT("NS_PD_Death"), TEXT("NS_PD_ImpSpark"), TEXT("NS_PD_ImpBlast"),
	TEXT("NS_PD_ImpFrost"), TEXT("NS_PD_ImpZap"), TEXT("NS_PD_ImpGlitter"), TEXT("NS_PD_ImpStarburst"),
	TEXT("NS_PD_Muzzle"), TEXT("NS_PD_Crit"), TEXT("NS_PD_Splash"), TEXT("NS_PD_Place"), TEXT("NS_PD_Synergy"), TEXT("NS_PD_Fuse"),
	TEXT("NS_PD_JokerFuse"), TEXT("NS_PD_Leak")};
// 맞음: 흰 불꽃, 죽음: 적 색 크게, 타격: 발사체 색, 그 밖의 사건: 사건 색
const float PDFxGlow[] = {2.f, 4.f, 3.f, 3.f, 2.5f, 3.f, 2.5f, 3.f, 2.5f, 2.5f, 2.5f, 2.5f, 3.f, 3.5f, 4.f, 3.f};
static_assert(UE_ARRAY_COUNT(PDFxNames) == (int32)SPDBoardWidget::EWorldFx::Count, "FX name table out of sync");
static_assert(UE_ARRAY_COUNT(PDFxGlow) == (int32)SPDBoardWidget::EWorldFx::Count, "FX glow table out of sync");

UNiagaraSystem* LoadFx(const TCHAR* Path)
{
	// 에셋이 아직 없으면 경고 없이 nullptr (그 이펙트만 건너뜀)
	return Cast<UNiagaraSystem>(FSoftObjectPath(Path).TryLoad());
}
}

void APDGameMode::SetupWorldView()
{
	UWorld* World = GetWorld();
	if (!World) return;
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	WorldCamera = World->SpawnActor<ACameraActor>(FVector(-PDCameraDistance, 0.f, 0.f), FRotator::ZeroRotator, Params);
	if (!WorldCamera) return;
	UCameraComponent* Cam = WorldCamera->GetCameraComponent();
	Cam->SetProjectionMode(ECameraProjectionMode::Orthographic);
	Cam->SetConstraintAspectRatio(false);
	// 정사영 폭은 가로 기준. 프로젝트 기본이 세로 기준(MaintainYFOV)이면 폭이 높이로 쓰여
	// 월드가 화면 비율(약 2배)만큼 작게 보이고 이펙트가 가운데 쪽으로 끌려간다
	Cam->bOverrideAspectRatioAxisConstraint = true;
	Cam->SetAspectRatioAxisConstraint(EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV);
	Cam->SetOrthoNearClipPlane(1.f);
	Cam->SetOrthoFarClipPlane(PDCameraDistance * 2.f);

	// 블룸은 살짝, 노출은 고정 (자동 노출이 바탕 밝기에 따라 출렁이지 않게)
	FPostProcessSettings& PP = Cam->PostProcessSettings;
	PP.bOverride_BloomIntensity = true;
	PP.BloomIntensity = 0.8f;
	PP.bOverride_BloomThreshold = true;
	PP.BloomThreshold = 1.0f; // HDR(1 넘는) 색만 번진다
	PP.bOverride_AutoExposureMethod = true;
	PP.AutoExposureMethod = EAutoExposureMethod::AEM_Manual;
	PP.bOverride_AutoExposureBias = true;
	PP.AutoExposureBias = 0.f;
	PP.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
	PP.AutoExposureApplyPhysicalCameraExposure = false;
	PP.bOverride_VignetteIntensity = true;
	PP.VignetteIntensity = 0.f;
	// 검은 바탕 위 이펙트만 그리므로 전역 조명·반사·AO·모션 블러는 끈다 (D-120, GPU 부하)
	PP.bOverride_DynamicGlobalIlluminationMethod = true;
	PP.DynamicGlobalIlluminationMethod = EDynamicGlobalIlluminationMethod::None;
	PP.bOverride_ReflectionMethod = true;
	PP.ReflectionMethod = EReflectionMethod::None;
	PP.bOverride_AmbientOcclusionIntensity = true;
	PP.AmbientOcclusionIntensity = 0.f;
	PP.bOverride_MotionBlurAmount = true;
	PP.MotionBlurAmount = 0.f;
	Cam->PostProcessBlendWeight = 1.f;
	UpdateWorldView();

	if (APlayerController* PC = World->GetFirstPlayerController()) PC->SetViewTarget(WorldCamera);

	FxSystems.SetNum((int32)SPDBoardWidget::EWorldFx::Count);
	for (int32 I = 0; I < FxSystems.Num(); ++I)
		FxSystems[I] = LoadFx(*FString::Printf(TEXT("/Game/FX/%s.%s"), PDFxNames[I], PDFxNames[I]));
}

void APDGameMode::UpdateWorldView()
{
	if (!WorldCamera || !BoardWidget.IsValid()) return;
	WorldCamera->GetCameraComponent()->SetOrthoWidth(BoardWidget->GetWorldViewWidth());
}

void APDGameMode::PlayWorldFx()
{
	if (!BoardWidget.IsValid()) return;
	for (const SPDBoardWidget::FWorldFx& Fx : BoardWidget->TakeWorldFx())
	{
		const int32 K = (int32)Fx.Kind;
		UNiagaraSystem* Sys = FxSystems.IsValidIndex(K) ? FxSystems[K].Get() : nullptr;
		if (!Sys) continue;
		FLinearColor Col = Fx.Col * PDFxGlow[K];
		Col.A = 1.f;
		const FVector Loc(0.f, Fx.Pos.X, -Fx.Pos.Y);
		UNiagaraComponent* NC = UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, Sys, Loc, FRotator::ZeroRotator, FVector(1.f),
			true, true, ENCPoolMethod::AutoRelease);
		if (!NC) continue;
		NC->SetVariableLinearColor(PDFxColor, Col);
		NC->SetVariableFloat(PDFxScale, Fx.Scale);
		NC->SetVariableVec3(PDFxDir, FVector(0.f, Fx.Dir.X, -Fx.Dir.Y));
	}
}

void APDGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoardWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BoardWidget.ToSharedRef());
	}
	BoardWidget.Reset();
	if (WorldCamera) WorldCamera->Destroy();
	WorldCamera = nullptr;
	for (UAudioComponent* V : Voices)
		if (V) V->DestroyComponent();
	Voices.Empty();
	if (MusicVoice) { MusicVoice->Stop(); MusicVoice->DestroyComponent(); MusicVoice = nullptr; }
	MusicWave = nullptr;
	Super::EndPlay(EndPlayReason);
}
