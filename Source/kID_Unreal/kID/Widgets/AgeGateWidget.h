#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgeGateWidget.generated.h"

UCLASS()
class UAgeGateWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWidget(TFunction<void(const FString&)> InCallback);

protected:
    virtual void NativeConstruct() override;

private:
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* DOBTextBox;

    UPROPERTY(meta = (BindWidget))
    class UButton* SubmitButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    TFunction<void(const FString&)> Callback;

    UFUNCTION()
    void OnSubmitClicked();
};