#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TestSetChallengeWidget.generated.h"

UCLASS()
class UTestSetChallengeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    void InitializeWidget(TFunction<void(const FString&, const FString&)> InOnSubmitCallback);

protected:
    UPROPERTY(meta = (BindWidget))
    class UComboBoxString* StatusCombo;

    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* AgeTextBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* SubmitButton;

    UFUNCTION()
    void OnCancelButtonClicked();

    UFUNCTION()
    void OnSubmitButtonClicked();

private:
    TFunction<void(const FString&, const FString&)> OnSubmitCallback;
};