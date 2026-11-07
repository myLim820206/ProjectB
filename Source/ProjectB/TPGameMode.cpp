#include "TPGameMode.h"

#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "GameFramework/PlayerController.h"
#include "TPBoardWidget.h"

ATPGameMode::ATPGameMode()
{
	DefaultPawnClass = nullptr; // 3D 캐릭터 없음
}

void ATPGameMode::BeginPlay()
{
	Super::BeginPlay();
	if (!GEngine || !GEngine->GameViewport) return;

	SAssignNew(BoardWidget, STPBoardWidget);
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

void ATPGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (BoardWidget.IsValid() && GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->RemoveViewportWidgetContent(BoardWidget.ToSharedRef());
	}
	BoardWidget.Reset();
	Super::EndPlay(EndPlayReason);
}
