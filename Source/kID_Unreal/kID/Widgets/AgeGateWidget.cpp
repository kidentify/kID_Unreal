#include "AgeGateWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UAgeGateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SubmitButton->OnClicked.AddDynamic(this, &UAgeGateWidget::OnSubmitClicked);
}

void UAgeGateWidget::InitializeWidget(TFunction<void(const FString&)> InCallback)
{
    Callback = InCallback;
}

void UAgeGateWidget::OnSubmitClicked()
{
    FString DOB = DOBTextBox->GetText().ToString();
    RemoveFromParent();
    Callback(DOB);
} 