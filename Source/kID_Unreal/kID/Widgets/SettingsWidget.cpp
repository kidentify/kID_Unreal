#include "SettingsWidget.h"
#include "Components/CheckBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Internationalization/Text.h"

void USettingsWidget::InitializeWidget(TSharedPtr<FJsonObject> SessionInfo, TFunction<void(const FString&, bool)> InCallback)
{
    CreatePermissionWidgets(SessionInfo);
    SyncCheckboxes(SessionInfo);
    Callback = InCallback;
    CancelButton->OnClicked.AddDynamic(this, &USettingsWidget::RemoveFromParent);
}

void USettingsWidget::CreatePermissionWidgets(TSharedPtr<FJsonObject> SessionInfo)
{
    TArray<TSharedPtr<FJsonValue>> Permissions = SessionInfo->GetArrayField(TEXT("permissions"));
    for (auto& Permission : Permissions)
    {
        TSharedPtr<FJsonObject> PermissionObject = Permission->AsObject();
        FString PermissionName = PermissionObject->GetStringField(TEXT("name"));
        FString DisplayName = FeatureMappings.Contains(PermissionName) ? FeatureMappings[PermissionName] : PermissionName;

        // Create CheckBox
        UCheckBox* CheckBox = WidgetTree->ConstructWidget<UCheckBox>(UCheckBox::StaticClass(), FName(*(PermissionName + TEXT("CheckBox"))));
        CheckBox->OnCheckStateChanged.AddDynamic(this, &USettingsWidget::OnAnyCheckBoxChanged);
        CheckBoxMap.Add(PermissionName, CheckBox);

        // Create TextBlock
        UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), FName(*(PermissionName + TEXT("TextBlock"))));
        TextBlock->SetText(FText::FromString(DisplayName));
        TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
        TextBlockMap.Add(PermissionName, TextBlock);

        // Create HorizontalBox to hold CheckBox and TextBlock side by side
        UHorizontalBox* HorizontalBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

        // Add CheckBox to HorizontalBox
        UHorizontalBoxSlot* CheckBoxSlot = HorizontalBox->AddChildToHorizontalBox(CheckBox);
        CheckBoxSlot->SetPadding(FMargin(5.0f, 5.0f));
        CheckBoxSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);

        // Add TextBlock to HorizontalBox
        UHorizontalBoxSlot* TextBlockSlot = HorizontalBox->AddChildToHorizontalBox(TextBlock);
        TextBlockSlot->SetPadding(FMargin(5.0f, 5.0f));
        TextBlockSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Left);

        // Add HorizontalBox to FeaturesContainer
        UVerticalBoxSlot* HorizontalBoxSlot = FeaturesContainer->AddChildToVerticalBox(HorizontalBox);
        HorizontalBoxSlot->SetPadding(FMargin(0, 5.0f, 0, 5.0f));

        // Initialize the state in the map
        CheckBoxStates.Add(PermissionName, false);
    }
}

void USettingsWidget::SyncCheckboxes(TSharedPtr<FJsonObject> SessionInfo)
{
    TArray<TSharedPtr<FJsonValue>> Permissions = SessionInfo->GetArrayField(TEXT("permissions"));
    for (auto& Permission : Permissions)
    {
        TSharedPtr<FJsonObject> PermissionObject = Permission->AsObject();
        FString PermissionName = PermissionObject->GetStringField(TEXT("name"));


        if (CheckBoxMap.Contains(PermissionName) && TextBlockMap.Contains(PermissionName))
        {
            UCheckBox* CheckBox = *CheckBoxMap.Find(PermissionName);
            UTextBlock* TextBlock = *TextBlockMap.Find(PermissionName);
            SetUpFeature(PermissionName, PermissionObject, CheckBox, TextBlock);
        }
    }
}

void USettingsWidget::SetUpFeature(const FString &PermissionName, TSharedPtr<FJsonObject> PermissionObject, UCheckBox* Checkbox, UTextBlock* Text)
{
    Text->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
    UE_LOG(LogTemp, Log, TEXT("Setting up feature: %s"), *PermissionName);
    bool bProhibited = PermissionObject->GetStringField(TEXT("managedBy")) == TEXT("PROHIBITED");
    bool bManagedByGuardian = PermissionObject->GetStringField(TEXT("managedBy")) == TEXT("GUARDIAN");
    bool bEnabled = PermissionObject->GetBoolField(TEXT("enabled"));
    Checkbox->SetIsChecked(bEnabled);
    Checkbox->SetIsEnabled(!bProhibited);
    Text->SetColorAndOpacity(!bProhibited ? (bManagedByGuardian ? FSlateColor(FLinearColor::Blue) : FSlateColor(FLinearColor::Black)) : FSlateColor(FLinearColor::Gray));
    Checkbox->SetToolTipText(bProhibited ? FText::FromString(TEXT("This feature is prohibited")) : (bManagedByGuardian ? FText::FromString(TEXT("Requires a parent's help")) : FText::FromString(TEXT(""))));
    CheckBoxStates[PermissionName] = Checkbox->IsChecked();
}

void USettingsWidget::OnAnyCheckBoxChanged(bool bIsChecked)
{
    for (const auto& Entry : CheckBoxMap)
    {
        FString PermissionName = Entry.Key;
        UCheckBox* CheckBox = Entry.Value;
        bool PreviousState = CheckBoxStates[PermissionName];
        bool CurrentState = CheckBox->IsChecked();

        if (PreviousState != CurrentState)
        {
            CheckBoxStates[PermissionName] = CurrentState;
            Callback(PermissionName, CurrentState);
            break;
        }
    }
}
