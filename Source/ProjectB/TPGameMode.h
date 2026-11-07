#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPSynth.h"
#include "TPGameMode.generated.h"

class STPBoardWidget;
class UAudioComponent;

// 테트리스 포커 게임 모드: 시작하면 화면 전체를 덮는 Slate 위젯을 띄우고,
// 게임이 낸 소리를 합성해 재생한다. 맵, 블루프린트, 에셋 없이 C++만으로 돌아간다.
UCLASS()
class PROJECTB_API ATPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATPGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaSeconds) override;

private:
	void PlayGameSound(const TP::FTPSoundEvent& Event);

	TSharedPtr<STPBoardWidget> BoardWidget;

	// 재생 중인 소리 (가비지 컬렉션에서 지키려고 UPROPERTY)
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAudioComponent>> Voices;

	// 합성한 PCM 캐시 (소리 종류별로 한 번만 합성)
	TMap<int32, TArray<uint8>> PcmCache;
};
