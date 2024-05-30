#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgeAssuranceWidget.generated.h"


UCLASS()
class UAgeAssuranceWidget : public UUserWidget
{
    GENERATED_BODY()
    
public:
    void InitializeWidget(const FString& DateOfBirth, TFunction<void(bool)> Callback);

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* VerificationText;

protected:
    UPROPERTY(meta = (BindWidget))
    class UButton* YesButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* NoButton;

    UFUNCTION()
    void OnYesClicked();

    UFUNCTION()
    void OnNoClicked();

private:
    int32 CalculateAgeFromDOB(const FString& DateOfBirth);

    TFunction<void(bool)> Callback;
};