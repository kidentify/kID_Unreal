#include "UnavailableWidget.h"
#include "Kismet/KismetSystemLibrary.h"

void UUnavailableWidget::NativeConstruct()
{
    Super::NativeConstruct();

    if (Quit)
    {
        Quit->OnClicked.AddDynamic(this, &UUnavailableWidget::OnQuitClicked);
    }
}

void UUnavailableWidget::OnQuitClicked()
{
    UKismetSystemLibrary::QuitGame(this, nullptr, EQuitPreference::Quit, false);  
}