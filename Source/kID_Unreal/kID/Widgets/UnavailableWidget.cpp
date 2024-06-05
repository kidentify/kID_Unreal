#include "UnavailableWidget.h"
#include "Kismet/KismetSystemLibrary.h"

void UUnavailableWidget::NativeConstruct()
{
    Super::NativeConstruct();

    Quit->OnClicked.AddDynamic(this, &UUnavailableWidget::OnQuitClicked);
}

void UUnavailableWidget::OnQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);  
}