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

    void InitializeWidget(int32 Age, TFunction<void(bool, int32, int32)> InCallback); 

    void StopHttpServer();
protected:

private:
    UFUNCTION()
    void OnYesClicked();

    UFUNCTION()
    void OnNoClicked();

    TFunction<void(bool, int32, int32)> Callback;
    int32 Age;

    UPROPERTY()
    class UFPrivatelyHttpServer* HttpServer;
    void StartHttpServer();

};