#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "TPGameMode.generated.h"

class STPBoardWidget;

// 테트리스 포커 게임 모드: 시작하면 화면 전체를 덮는 Slate 위젯을 띄운다.
// 맵, 블루프린트, 에셋 없이 C++만으로 돌아간다.
UCLASS()
class PROJECTB_API ATPGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ATPGameMode();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	TSharedPtr<STPBoardWidget> BoardWidget;
};
