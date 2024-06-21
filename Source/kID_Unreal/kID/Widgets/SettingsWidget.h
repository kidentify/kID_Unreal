#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/Button.h"
#include "SettingsWidget.generated.h"

UCLASS()
class USettingsWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    void InitializeWidget(TSharedPtr<FJsonObject> SessionInfo, TFunction<void(const FString&, bool)> InCallback);
    void SyncCheckboxes(TSharedPtr<FJsonObject> SessionInfo);

private:
    void CreatePermissionWidgets(TSharedPtr<FJsonObject> SessionInfo);
    void SetUpFeature(const FString& PermissionName, TSharedPtr<FJsonObject> PermissionObject, UCheckBox* Checkbox, UTextBlock* Text);

    UFUNCTION()
    void OnAnyCheckBoxChanged(bool bIsChecked);

    TFunction<void(const FString&, bool)> Callback;

    UPROPERTY(meta = (BindWidget))
    class UVerticalBox* FeaturesContainer;

    UPROPERTY(meta = (BindWidget))
    class UButton* CancelButton;

    TMap<FString, UCheckBox*> CheckBoxMap;
    TMap<FString, UTextBlock*> TextBlockMap;
    TMap<FString, bool> CheckBoxStates;

    // adding a map of user presentable names for features to be displayed in the UI.  
    // add your own for any new features you add to the system
    const TMap<FString, FString> FeatureMappings = {
        {TEXT("multiplayer"), TEXT("Multiplayer")},
        {TEXT("text-chat-private"), TEXT("Private Text Chat")},
        {TEXT("ai-generated-avatars"), TEXT("AI Generated Avatars")}
    };
};
