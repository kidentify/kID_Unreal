
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

// Integrate kID workflow into the game
void AkID_UnrealGameMode::BeginPlay()
{
    Super::BeginPlay();

    UE_LOG(LogTemp, Log, TEXT("Setting up kID workflow..."));

    KidWorkflow = NewObject<UKidWorkflow>(this, UKidWorkflow::StaticClass());
    KidWorkflow->Initialize([this](bool bSuccess)
    {
        if (bSuccess)
        {
            UE_LOG(LogTemp, Log, TEXT("kID workflow initialized successfully!"));
            KidWorkflow->ShowDemoControls();
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("kID workflow failed to initialize!"));
        }
        KidWorkflow->ShowPlayerHUD();
    });
 }

// Clean up kID workflow when the game ends - primarily for Unreal PIE sessions
void AkID_UnrealGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    UE_LOG(LogTemp, Log, TEXT("Cleaning up kID workflow..."));
    KidWorkflow->CleanUp();
    Super::EndPlay(EndPlayReason);
}