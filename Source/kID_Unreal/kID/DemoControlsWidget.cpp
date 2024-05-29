#include "DemoControlsWidget.h"
#include "KidWorkflow.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UDemoControlsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SubmitButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnSubmitClicked);
    ClearSessionButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnClearSessionClicked);
}

void UDemoControlsWidget::OnSubmitClicked()
{
    if (KidWorkflow)
    {
        FString CountryCode = CountryCodeTextBox->GetText().ToString();
        KidWorkflow->StartKidSession(CountryCode);
    }
}

void UDemoControlsWidget::OnClearSessionClicked()
{
    if (KidWorkflow)
    {
        KidWorkflow->ClearSession();
        KidWorkflow->ClearChallengeId();
    }
}

void UDemoControlsWidget::SetKidWorkflow(UKidWorkflow* InKidWorkflow)
{
    KidWorkflow = InKidWorkflow;
}

