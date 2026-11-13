#include "PDGameMode.h"

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
	UpdateWorldView();
	PlayWorldFx();
	for (const PD::FPDSoundEvent& E : BoardWidget->TakeSounds()) PlayGameSound(E);
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

void APDGameMode::BeginPlay()
{
	Super::BeginPlay();
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
	Super::EndPlay(EndPlayReason);
}
