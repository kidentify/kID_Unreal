#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Widgets/PlayerHUDWidget.h"
#include "Widgets/FloatingChallengeWidget.h"
#include "Widgets/AgeGateWidget.h"
#include "Widgets/UnavailableWidget.h"
#include "Widgets/DemoControlsWidget.h"
#include "Widgets/AgeAssuranceWidget.h"
#include "KidWorkflow.generated.h"

UCLASS()
class UKidWorkflow : public UObject
{
    GENERATED_BODY()

public:
    enum class AccessMode {
        None, // player has no access (under the minimum age)
        DataLite, // player has access to data-lite features
        Full // full access to all features
    };

    void Initialize(TFunction<void(bool)> Callback);
    void CleanUp();

    // workflows
    void StartKidSession(const FString& Location);
    void HandleExistingChallenge(const FString& ChallengeId);
    void StartKidSessionWithDOB(const FString& Location, const FString& DOB);
    void GetUserAge(const FString& Location, TFunction<void(bool, bool, const FString&)> Callback);
    void ValidateAge(const FString& DOB, TFunction<void(bool)> Callback);
    void GetDefaultPermissions(const FString& Location);
    void ShowConsentChallenge(const FString& ChallengeId, int32 Timeout, const FString& OTP, const FString& QRCodeUrl, 
                            TFunction<void(bool, const FString&)> OnConsentGranted);
    void GetSessionPermissions(const FString& SessionId, const FString& ETag);
    void CheckForConsent(const FString& ChallengeId, FDateTime StartTime, int32 Timeout, 
                            TFunction<void(bool, const FString&)> OnConsentGranted);
    void HandleProhibitedStatus();
    void HandleNoConsent();

    // Use the test/set-challenge-status API for testing consent challenges
    void SetChallengeStatus(const FString& Location);

    // feature management - roadmap
    void AttemptTurnOnRestrictedFeature(const FString& FeatureName, TFunction<void()> EnableFeature);
    void ShowFeatureConsentChallenge(TFunction<void()> OnConsentGranted);

    // managing sessions in local storage
    void SaveSessionInfo();
    bool GetSavedSessionInfo();
    void ClearSession();
    TSharedPtr<FJsonObject> FindPermission(const FString& FeatureName);

    // managing saved challenge id in local storage
    bool HasChallengeId();
    void SaveChallengeId(const FString& InChallengeId);
    void ClearChallengeId();
    bool LoadChallengeId(FString& OutChallengeId);

     // UI Elements
    void ShowUnavailableWidget();
    void ShowAgeGate(TFunction<void(const FString&)> Callback);
    void ShowTestSetChallengeWidget(TFunction<void(const FString&, const FString&)> Callback);
    void ShowAgeAssuranceWidget(const FString& DateOfBirth, TFunction<void(bool)> OnAssuranceResponse);
    void ShowFloatingChallengeWidget(const FString& OTP, const FString& QRCodeUrl, TFunction<void(const FString&)> OnEmailSubmitted);
    void DismissFloatingChallengeWidget();
    void ShowDemoControls();
    void ShowPlayerHUD();
    void UpdateHUDText();

    // Feature-specific functions
    void AttemptTurnOnChat();
    void EnableChat();
     
private:
    static bool bShutdown;

    FTimerHandle ConsentPollingTimerHandle;
    TSharedPtr<FJsonObject> SessionInfo;
    FString AuthToken;

    // Mode declares what access the player has in the game based on age and consent. 
    // For None status, the player should be disallowed. 
    // For DataLite status, the player should be allowed to use "data-lite features" only.
    // For Full status, the player should be allowed to use all features that don't require 
    // further permission based on their age and jurisdiction.
    AccessMode Mode = AccessMode::DataLite;

    UPROPERTY()
    UAgeGateWidget* AgeGateWidget;

    UPROPERTY()
    UPlayerHUDWidget* PlayerHUDWidget;

    UPROPERTY()
    UFloatingChallengeWidget* FloatingChallengeWidget;

    UPROPERTY()
    UDemoControlsWidget* DemoControlsWidget;
};