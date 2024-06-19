#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "SettingsWidget.generated.h"

class UCheckbox;
class UTextBlock;

UCLASS()
class USettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPROPERTY(meta = (BindWidget))
    class UCheckBox* ContextualAdsCheckbox;

    UPROPERTY(meta = (BindWidget))
    class UCheckBox* TargetedAdsCheckbox;

    UPROPERTY(meta = (BindWidget))
    class UCheckBox* MultiplayerCheckbox;

    UPROPERTY(meta = (BindWidget))
    class UCheckBox* TextChatPrivateCheckbox;

    UPROPERTY(meta = (BindWidget))
    class UCheckBox* AIGeneratedAvatarCheckbox;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* ContextualAdsText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TargetedAdsText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* MultiplayerText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TextChatPrivateText;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* AIGeneratedAvatarText;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    void InitializeWidget(TSharedPtr<FJsonObject> SessionInfo, TFunction<void(const FString &, bool)> InCallback);
    void SyncCheckboxes(TSharedPtr<FJsonObject> SessionInfo);

private:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnContextualAdsCheckboxChanged(bool bIsChecked);

    UFUNCTION()
    void OnTargetedAdsCheckboxChanged(bool bIsChecked);

    UFUNCTION()
    void OnMultiplayerCheckboxChanged(bool bIsChecked);

    UFUNCTION()
    void OnTextChatPrivateCheckboxChanged(bool bIsChecked);

    UFUNCTION()
    void OnAIGeneratedAvatarCheckboxChanged(bool bIsChecked);

    void OnCheckBoxChanged(bool bIsChecked, FString CheckBoxName);

    TFunction<void(const FString &, bool)> Callback;
};