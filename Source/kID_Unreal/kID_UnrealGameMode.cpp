// Copyright Epic Games, Inc. All Rights Reserved.

#include "kID_UnrealGameMode.h"
#include "kID_UnrealCharacter.h"
#include "UObject/ConstructorHelpers.h"

AkID_UnrealGameMode::AkID_UnrealGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPerson/Blueprints/BP_FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

}

void AkID_UnrealGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("Setting up kID workflow..."));

    KidWorkflow = NewObject<UKidWorkflow>(this, UKidWorkflow::StaticClass());
    KidWorkflow->Initialize();

    UE_LOG(LogTemp, Log, TEXT("Showing demo controls..."));

    // Get the game instance and call the method to create the widget
    KidWorkflow->ShowDemoControls();
    KidWorkflow->ShowPlayerHUD();
}

void AkID_UnrealGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Warning, TEXT("AMyProject3GameMode::EndPlay..."));
    KidWorkflow->CleanUp();
	Super::EndPlay(EndPlayReason);
}