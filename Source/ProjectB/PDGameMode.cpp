#include "PDGameMode.h"

#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/PostProcessVolume.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
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
constexpr int32 TPSampleRate = 44100;
constexpr int32 TPMaxVoices = 24;
constexpr float TPMasterVolume = 0.8f;
}

// 설계 좌표(오른쪽 x, 아래 y) → 월드. 카메라가 -Z 를 보고 화면 위 = +X, 화면 오른쪽 = +Y
static FVector PDDesignToWorld(FVector2f P, float Z = 0.f) { return FVector(-P.Y, P.X, Z); }

void APDGameMode::SetupWorldView()
{
	UWorld* World = GetWorld();
	if (!World) return;
	FActorSpawnParameters Sp;
	Sp.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	FxCamera = World->SpawnActor<ACameraActor>(ACameraActor::StaticClass(), PDDesignToWorld(FVector2f(640.f, 360.f), 1000.f), FRotator(-90.f, 0.f, 0.f), Sp);
	if (FxCamera)
	{
		UCameraComponent* Cam = FxCamera->GetCameraComponent();
		Cam->SetProjectionMode(ECameraProjectionMode::Orthographic);
		Cam->SetOrthoWidth(1280.f);
		Cam->SetOrthoNearClipPlane(0.f);
		Cam->SetOrthoFarClipPlane(4000.f);
		Cam->SetConstraintAspectRatio(false);
		Cam->bAutoCalculateOrthoPlanes = false;
		if (APlayerController* PC = World->GetFirstPlayerController())
		{
			PC->bAutoManageActiveCameraTarget = false; // 폰이 없어도 카메라를 되돌리지 않게
			PC->SetViewTarget(FxCamera);
		}
	}
	// 후처리: 넓고 부드러운 빛번짐(네온, docs/15), 노출 고정
	FxPost = World->SpawnActor<APostProcessVolume>(APostProcessVolume::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator, Sp);
	if (FxPost)
	{
		FxPost->bUnbound = true;
		FPostProcessSettings& S = FxPost->Settings;
		S.bOverride_BloomMethod = true;
		S.BloomMethod = BM_SOG;
		S.bOverride_BloomIntensity = true;
		S.BloomIntensity = 2.5f;
		S.bOverride_BloomThreshold = true;
		S.BloomThreshold = 0.5f;
		S.bOverride_Bloom4Size = true;
		S.Bloom4Size = 32.f;
		S.bOverride_Bloom5Size = true;
		S.Bloom5Size = 64.f;
		S.bOverride_Bloom6Size = true;
		S.Bloom6Size = 128.f;
		S.bOverride_AutoExposureMethod = true;
		S.AutoExposureMethod = AEM_Manual;
		S.bOverride_AutoExposureApplyPhysicalCameraExposure = true;
		S.AutoExposureApplyPhysicalCameraExposure = 0;
		S.bOverride_AutoExposureBias = true;
		S.AutoExposureBias = 0.f;
		S.bOverride_VignetteIntensity = true;
		S.VignetteIntensity = 0.f;
	}
}

void APDGameMode::UpdateCamera()
{
	if (!FxCamera || !BoardWidget.IsValid()) return;
	const float Scale = FMath::Max(0.01f, BoardWidget->GetViewScale());
	const FVector2f Size = BoardWidget->GetViewSize();
	// 위젯은 설계 화면을 가운데에 놓고 Scale 배로 늘린다 → 카메라 가운데 = 설계 가운데, 보이는 폭 = 위젯 폭 / Scale
	FxCamera->SetActorLocation(PDDesignToWorld(FVector2f(640.f, 360.f) - BoardWidget->GetFieldShake(), 1000.f));
	FxCamera->GetCameraComponent()->SetOrthoWidth(Size.X / Scale);
}

bool APDGameMode::SpawnWorldFx(FName Name, FVector2f DesignPos, FVector2f Dir, FLinearColor Col, float Scale)
{
	UWorld* World = GetWorld();
	if (!World || !FxCamera) return false;
	TObjectPtr<UNiagaraSystem>* Found = FxCache.Find(Name);
	if (!Found)
	{
		const FString Path = FString::Printf(TEXT("/Game/FX/%s.%s"), *Name.ToString(), *Name.ToString());
		UNiagaraSystem* Sys = LoadObject<UNiagaraSystem>(nullptr, *Path, nullptr, LOAD_NoWarn | LOAD_Quiet);
		Found = &FxCache.Add(Name, Sys);
	}
	UNiagaraSystem* Sys = Found->Get();
	if (!Sys) return false;
	// 효과는 XY 평면에 그리고 +Z(카메라 쪽)를 바라보게 만든다. 1 = 설계 1픽셀, 칸 한 변 = 48
	const float Yaw = Dir.IsNearlyZero() ? 0.f : FMath::RadiansToDegrees(FMath::Atan2(Dir.X, -Dir.Y)); // 화면 방향 → 월드 방향 (+X = 화면 위)
	UNiagaraComponent* C = UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, Sys, PDDesignToWorld(DesignPos, 10.f), FRotator(0.f, Yaw, 0.f), FVector(1.f), true, true);
	if (!C) return false;
	C->SetVariableLinearColor(FName(TEXT("User.Color")), Col);
	C->SetVariableFloat(FName(TEXT("User.Power")), Scale);
	C->SetVariableVec3(FName(TEXT("User.Direction")), FVector(-Dir.Y, Dir.X, 0.f));
	return true;
}

void APDGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!BoardWidget.IsValid()) return;
	UpdateCamera();
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
		const std::vector<int16_t> Samples = PD::SynthesizeSound(Event.Sound, Event.A, Event.B, TPSampleRate);
		TArray<uint8> Bytes;
		Bytes.SetNumUninitialized((int32)(Samples.size() * sizeof(int16_t)));
		FMemory::Memcpy(Bytes.GetData(), Samples.data(), Bytes.Num());
		PcmCache.Add(Key, MoveTemp(Bytes));
	}
	const TArray<uint8>& Pcm = PcmCache[Key];
	if (Pcm.Num() == 0) return;

	USoundWaveProcedural* Wave = NewObject<USoundWaveProcedural>(this);
	Wave->SetSampleRate(TPSampleRate);
	Wave->NumChannels = 1;
	Wave->Duration = (float)Pcm.Num() / (sizeof(int16_t) * TPSampleRate);
	Wave->bLooping = false;
	Wave->QueueAudio(Pcm.GetData(), Pcm.Num());

	UAudioComponent* Voice = UGameplayStatics::CreateSound2D(this, Wave, TPMasterVolume * Event.Volume, 1.f, 0.f, nullptr, false, false);
	if (!Voice) return;
	Voice->bIsUISound = true;
	Voice->Play();
	Voices.Add(Voice);
	while (Voices.Num() > TPMaxVoices)
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

	// 월드 이펙트: 카메라 + 블룸을 깔고 위젯 배경을 비운다
	SetupWorldView();
	if (FxCamera)
	{
		BoardWidget->bWorldBackground = true;
		BoardWidget->OnWorldFx = [WeakThis](const SPDBoardWidget::FWorldFx& R)
		{
			APDGameMode* Self = WeakThis.Get();
			return Self && Self->SpawnWorldFx(R.Name, R.P, R.Dir, R.Col, R.Scale);
		};
	}

	if (APlayerController* PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
	{
		FInputModeUIOnly Mode;
		Mode.SetWidgetToFocus(BoardWidget);
		Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
		PC->SetInputMode(Mode);
		PC->bShowMouseCursor = true;
	}
	FSlateApplication::Get().SetKeyboardFocus(BoardWidget);
}

void APDGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoardWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BoardWidget.ToSharedRef());
	}
	BoardWidget.Reset();
	for (UAudioComponent* V : Voices)
		if (V) V->DestroyComponent();
	Voices.Empty();
	Super::EndPlay(EndPlayReason);
}
