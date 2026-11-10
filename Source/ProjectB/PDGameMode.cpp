#include "PDGameMode.h"

#include "Components/AudioComponent.h"
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

void APDGameMode::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!BoardWidget.IsValid()) return;
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
