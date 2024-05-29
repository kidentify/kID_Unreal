#include "PlayerHUDWidget.h"
#include "Components/TextBlock.h"

void UPlayerHUDWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // You can bind or initialize properties here if needed
}

void UPlayerHUDWidget::SetText(const FString& text)
{
    if (TextBlock)
    {
        TextBlock->SetText(FText::FromString(text));
    } else {
        UE_LOG(LogTemp, Warning, TEXT("textblock not set."));
    }
}