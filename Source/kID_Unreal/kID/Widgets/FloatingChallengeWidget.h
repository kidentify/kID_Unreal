#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FloatingChallengeWidget.generated.h"

class UKidWorkflow;

UCLASS()
class UFloatingChallengeWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWidget(UKidWorkflow* InGameInstance, const FString& OTP, const FString& QRCodeUrl, TFunction<void(const FString&)> OnEmailSubmitted);

private:
    UPROPERTY(meta = (BindWidget))
    class UEditableTextBox* EmailTextBox;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* OTPTextBlock;

    UPROPERTY(meta = (BindWidget))
    class UImage* QRCodeImage;

    UPROPERTY(meta = (BindWidget))
    class UButton* SubmitButton;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    FString OTP;
    FString QRCodeUrl;
    TFunction<void(const FString&)> OnEmailSubmitted;

    UFUNCTION()
    void HandleEmailSubmitted();
	
    UFUNCTION()
    void OnCancelClicked();

    UTexture2D* GenerateQRCodeTexture();

    UKidWorkflow* KidWorkflow = nullptr;
};