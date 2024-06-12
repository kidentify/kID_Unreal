#include "SliderAgeGateWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void USliderAgeGateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SubmitButton->OnClicked.AddDynamic(this, &USliderAgeGateWidget::OnSubmitClicked);
    CancelButton->OnClicked.AddDynamic(this, &USliderAgeGateWidget::RemoveFromParent);
}

void USliderAgeGateWidget::InitializeWidget(TFunction<void(const FString&)> InCallback)
{
    Callback = InCallback;
}

void USliderAgeGateWidget::OnSubmitClicked()
{
    float Age = AgeSlider->GetValue();
    RemoveFromParent();
    FDateTime Now = FDateTime::UtcNow();
    FDateTime DateOfBirth = Now - FTimespan::FromDays(Age * 365.25 + 1);
    FString DateOfBirthString = DateOfBirth.ToString(TEXT("%Y-%m-%d"));
    Callback(DateOfBirthString);
} 