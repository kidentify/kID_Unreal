#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DemoControlsWidget.generated.h"
class UKidWorkflow;

UCLASS()
class UDemoControlsWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnSubmitClicked();

    UFUNCTION()
    void OnClearSessionClicked();

    UFUNCTION()
    void OnTestSetChallengeClicked();

    UFUNCTION()
    void OnSettingsClicked();

    void SetKidWorkflow(UKidWorkflow* InKidWorkflow);
    void SetTestSetChallengeButtonVisibility(bool bVisible);
    void SetSettingsButtonVisibility(bool bVisible);

private:
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* CountryCodeTextBox;
    UPROPERTY(meta = (BindWidget))
    class UButton* SubmitButton;
    UPROPERTY(meta = (BindWidget))
    class UButton* ClearSessionButton;
    UPROPERTY(meta = (BindWidget))
    class UButton* TestSetChallengeButton;
    UPROPERTY(meta = (BindWidget))
    class UButton* SettingsButton;
    UKidWorkflow* KidWorkflow = nullptr;
};