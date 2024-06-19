#include "SettingsWidget.h"
#include "Components/CheckBox.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"

void USettingsWidget::InitializeWidget(TSharedPtr<FJsonObject> SessionInfo, TFunction<void(const FString &, bool)> InCallback)
{
    SyncCheckboxes(SessionInfo);
    Callback = InCallback;
}

void SetUpFeature(TSharedPtr<FJsonObject> PermissionObject, UCheckBox* Checkbox, UTextBlock* Text)
{
    bool bProhibited = PermissionObject->GetStringField(TEXT("managedBy")) == TEXT("PROHIBITED");
    bool bManagedByGuaardian = PermissionObject->GetStringField(TEXT("managedBy")) == TEXT("GUARDIAN");
    bool bEnabled = PermissionObject->GetBoolField(TEXT("enabled"));
    Checkbox->SetIsChecked(bEnabled);
    Checkbox->SetIsEnabled(!bProhibited);
    Text->SetColorAndOpacity(!bProhibited ? (bManagedByGuaardian ? 
            FSlateColor(FLinearColor::Blue) : FSlateColor(FLinearColor::Black)) : FSlateColor(FLinearColor::Gray));
    Checkbox->SetToolTipText(bProhibited ? FText::FromString(TEXT("This feature is prohibited")) : 
            (bManagedByGuaardian ? FText::FromString(TEXT("Requires a parent's help")) : FText::FromString(TEXT(""))));
 }

void USettingsWidget::SyncCheckboxes(TSharedPtr<FJsonObject> SessionInfo)
{
    TArray<TSharedPtr<FJsonValue>> Permissions = SessionInfo->GetArrayField(TEXT("permissions"));
    for (auto &Permission : Permissions)
    {
        TSharedPtr<FJsonObject> PermissionObject = Permission->AsObject();
        if (PermissionObject->GetStringField(TEXT("name")) == TEXT("contextual-ads"))
        {
            SetUpFeature(PermissionObject, ContextualAdsCheckbox, ContextualAdsText);
        }
        else if (PermissionObject->GetStringField(TEXT("name")) == TEXT("targeted-ads"))
        {
            SetUpFeature(PermissionObject, TargetedAdsCheckbox, TargetedAdsText);
        }
        else if (PermissionObject->GetStringField(TEXT("name")) == TEXT("multiplayer"))
        {
            SetUpFeature(PermissionObject, MultiplayerCheckbox, MultiplayerText);
        }
        else if (PermissionObject->GetStringField(TEXT("name")) == TEXT("text-chat-private"))
        {
            SetUpFeature(PermissionObject, TextChatPrivateCheckbox, TextChatPrivateText);
        }
        else if (PermissionObject->GetStringField(TEXT("name")) == TEXT("ai-generated-avatars"))
        {
            SetUpFeature(PermissionObject, AIGeneratedAvatarCheckbox, AIGeneratedAvatarText);
        }
    }
}

void USettingsWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ContextualAdsCheckbox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnContextualAdsCheckboxChanged);
    TargetedAdsCheckbox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnTargetedAdsCheckboxChanged);
    MultiplayerCheckbox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnMultiplayerCheckboxChanged);
    TextChatPrivateCheckbox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnTextChatPrivateCheckboxChanged);
    AIGeneratedAvatarCheckbox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnAIGeneratedAvatarCheckboxChanged);
    CancelButton->OnClicked.AddDynamic(this, &USettingsWidget::RemoveFromParent);
}

void USettingsWidget::OnContextualAdsCheckboxChanged(bool bIsChecked)
{
    OnCheckBoxChanged(bIsChecked, TEXT("contextual-ads"));
}

void USettingsWidget::OnTargetedAdsCheckboxChanged(bool bIsChecked)
{
    OnCheckBoxChanged(bIsChecked, TEXT("targeted-ads"));
}

void USettingsWidget::OnMultiplayerCheckboxChanged(bool bIsChecked)
{
    OnCheckBoxChanged(bIsChecked, TEXT("multiplayer"));
}

void USettingsWidget::OnTextChatPrivateCheckboxChanged(bool bIsChecked)
{
    OnCheckBoxChanged(bIsChecked, TEXT("text-chat-private"));
}

void USettingsWidget::OnAIGeneratedAvatarCheckboxChanged(bool bIsChecked)
{
    OnCheckBoxChanged(bIsChecked, TEXT("ai-generated-avatars"));
}

void USettingsWidget::OnCheckBoxChanged(bool bIsChecked, FString FeatureName)
{
    Callback(FeatureName, bIsChecked);
}
