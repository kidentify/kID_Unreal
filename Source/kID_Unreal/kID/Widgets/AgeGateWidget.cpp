#include "AgeGateWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UAgeGateWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SubmitButton->OnClicked.AddDynamic(this, &UAgeGateWidget::OnSubmitClicked);
    CancelButton->OnClicked.AddDynamic(this, &UAgeGateWidget::RemoveFromParent);
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