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

    void SetKidWorkflow(UKidWorkflow* InKidWorkflow);

private:
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* CountryCodeTextBox;
    UPROPERTY(meta = (BindWidget))
    class UButton* SubmitButton;
    UPROPERTY(meta = (BindWidget))
    class UButton* ClearSessionButton;
    UKidWorkflow* KidWorkflow = nullptr;
};