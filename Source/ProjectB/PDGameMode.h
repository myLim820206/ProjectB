#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDSynth.h"
#include "PDGameMode.generated.h"

class SPDBoardWidget;
class UAudioComponent;
class ACameraActor;
class UNiagaraSystem;

// 포커 디펜스 게임 모드: 시작하면 화면 전체를 덮는 Slate 위젯을 띄우고,
// 게임이 낸 소리를 합성해 재생한다. 맵, 블루프린트 없이 C++만으로 돌아간다.
// 월드 이펙트 층 (D-092): 판 뒤에 정사영 카메라를 두고, 위젯이 요청한 이펙트를 나이아가라로 재생한다 (블룸을 받음).
// 나이아가라 에셋이 없으면 그 이펙트만 건너뛴다.
UCLASS()
class PROJECTB_API APDGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APDGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void PlayGameSound(const PD::FPDSoundEvent& Event);
	void SetupWorldView();   // 정사영 카메라 + 블룸·고정 노출
	void UpdateWorldView();  // 창 크기에 맞춰 카메라 폭 (1 월드 단위 = 판 1px)
	void PlayWorldFx();      // 위젯이 쌓은 이펙트 요청을 나이아가라로 재생

	TSharedPtr<SPDBoardWidget> BoardWidget;

	// 재생 중인 소리 (가비지 컬렉션에서 지키려고 UPROPERTY)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> Voices;

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> WorldCamera;

	// 이펙트 종류별 나이아가라 시스템 (Content/FX/NS_PD_*). 없는 칸은 nullptr
	UPROPERTY(Transient)
	TArray<TObjectPtr<UNiagaraSystem>> FxSystems; // SPDBoardWidget::EWorldFx 순서

	// 합성한 PCM 캐시 (소리 종류별로 한 번만 합성)
	TMap<int32, TArray<uint8>> PcmCache;
};
