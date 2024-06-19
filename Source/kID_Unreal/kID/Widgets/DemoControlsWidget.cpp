#include "DemoControlsWidget.h"
#include "../KidWorkflow.h"
#include "Components/EditableTextBox.h"
#include "Components/Button.h"

void UDemoControlsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    SubmitButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnSubmitClicked);
    ClearSessionButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnClearSessionClicked);
    TestSetChallengeButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnTestSetChallengeClicked);
    SettingsButton->OnClicked.AddDynamic(this, &UDemoControlsWidget::OnSettingsClicked);
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

void UDemoControlsWidget::OnSettingsClicked()
{
    if (KidWorkflow)
    {
        KidWorkflow->ShowSettingsWidget();
    }
}

void UDemoControlsWidget::OnTestSetChallengeClicked()
{
    if (KidWorkflow)
    {
        FString CountryCode = CountryCodeTextBox->GetText().ToString();
        KidWorkflow->SetChallengeStatus(CountryCode);
    }
}

void UDemoControlsWidget::SetTestSetChallengeButtonVisibility(bool bVisible)
{
    TestSetChallengeButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void UDemoControlsWidget::SetSettingsButtonVisibility(bool bVisible)
{
    SettingsButton->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
}

void UDemoControlsWidget::SetKidWorkflow(UKidWorkflow* InKidWorkflow)
{
    KidWorkflow = InKidWorkflow;
}

