#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AgeAssuranceWidget.generated.h"

UCLASS()
class UAgeAssuranceWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    class UTextBlock* VerificationText;

    UPROPERTY(meta = (BindWidget))
    class UButton* YesButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* NoButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    void InitializeWidget(const FString& DateOfBirth, TFunction<void(bool)> InCallback); 

    void StopHttpServer();
protected:

private:
    UFUNCTION()
    void OnYesClicked();

    UFUNCTION()
    void OnNoClicked();

    int32 CalculateAgeFromDOB(const FString& DateOfBirth);

    TFunction<void(bool)> Callback;

    UPROPERTY()
    class UFPrivatelyHttpServer* HttpServer;
    void StartHttpServer();

};