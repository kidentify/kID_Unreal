#include "TestSetChallengeWidget.h"
#include "Components/ComboBoxString.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UTestSetChallengeWidget::NativeConstruct()
{
    Super::NativeConstruct();

    CancelButton->OnClicked.AddDynamic(this, &UTestSetChallengeWidget::OnCancelButtonClicked);
    SubmitButton->OnClicked.AddDynamic(this, &UTestSetChallengeWidget::OnSubmitButtonClicked);
}

void UTestSetChallengeWidget::InitializeWidget(TFunction<void(const FString&, const FString&)> InOnSubmitCallback)
{
    OnSubmitCallback = InOnSubmitCallback;
}

void UTestSetChallengeWidget::OnCancelButtonClicked()
{
    RemoveFromParent();
}

void UTestSetChallengeWidget::OnSubmitButtonClicked()
{
    RemoveFromParent();

    FString Status = StatusCombo->GetSelectedOption();
    FString Age = AgeTextBox->GetText().ToString();

    if (OnSubmitCallback)
    {
        OnSubmitCallback(Status, Age);
    }

}