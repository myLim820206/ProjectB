#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "PDSynth.h"
#include "PDGameMode.generated.h"

class SPDBoardWidget;
class UAudioComponent;
class ACameraActor;
class APostProcessVolume;
class UNiagaraSystem;

// 포커 디펜스 게임 모드: 시작하면 화면 전체를 덮는 Slate 위젯을 띄우고,
// 게임이 낸 소리를 합성해 재생한다. 맵, 블루프린트 없이 C++만으로 돌아간다.
// 월드 이펙트 (D-094): 위에서 내려다보는 정사영 카메라 + 블룸 후처리를 깔고, 위젯 배경을 비워 월드가 비쳐 보이게 한다.
// 위젯이 이펙트 이름(NS_PD_...)을 보내면 /Game/FX/ 의 나이아가라 에셋을 띄운다 (없으면 위젯이 코드 이펙트로 그림).
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
	void SetupWorldView();  // 정사영 카메라 + 블룸
	void UpdateCamera();    // 위젯의 화면 크기·흔들림에 카메라를 맞춘다
	bool SpawnWorldFx(FName Name, FVector2f DesignPos, FVector2f Dir, FLinearColor Col, float Scale);

	UPROPERTY(Transient)
	TObjectPtr<ACameraActor> FxCamera;
	UPROPERTY(Transient)
	TObjectPtr<APostProcessVolume> FxPost;
	// 찾은 나이아가라 에셋 (없는 이름은 nullptr 로 기억해 다시 찾지 않음)
	UPROPERTY(Transient)
	TMap<FName, TObjectPtr<UNiagaraSystem>> FxCache;

	TSharedPtr<SPDBoardWidget> BoardWidget;

	// 재생 중인 소리 (가비지 컬렉션에서 지키려고 UPROPERTY)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> Voices;

	// 합성한 PCM 캐시 (소리 종류별로 한 번만 합성)
	TMap<int32, TArray<uint8>> PcmCache;
};
