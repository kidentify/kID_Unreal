#include "KidWorkflow.h"
#include "HttpRequestHelper.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/DefaultValueHelper.h"
#include "Widgets/SWeakWidget.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SCanvas.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/DemoControlsWidget.h"
#include "Widgets/TestSetChallengeWidget.h"

// Constants
const FString BaseUrl = TEXT("https://game-api.k-id.com/api/v1"); 

const int32 ConsentTimeoutSeconds = 300; // maximum time to wait for consent in seconds
const int32 ConsentPollingInterval = 1; // time to wait between polling for consent in seconds
const FString ClientId = TEXT("12345678-1234-1234-1234-123456789012"); // client ID for the demo

// Unreal PIE aid to prevent long poll from causing issues after quitting the game
bool UKidWorkflow::bShutdown = false;

void UKidWorkflow::Initialize(TFunction<void(bool)> Callback)
{ 
    bShutdown = false;

    FString ApiKey;
    if (!FFileHelper::LoadFileToString(ApiKey, *(FPaths::ProjectDir() + TEXT("/apikey.txt"))))
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to load API key from apikey.txt.  Create a file with your API key in the project root directory."));
        Callback(false);
        return;
    }
    ApiKey.TrimEndInline();

    // do this up front so that the HUD shows the session before interacting with the kID demo controls
    GetSavedSessionInfo();

    FString payload = TEXT("{ \"clientId\": \"") + ClientId + TEXT("\"}");

    HttpRequestHelper::PostRequestWithAuth(BaseUrl + TEXT("/auth/issue-token"), payload, ApiKey, 
                    [this, Callback](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                AuthToken = JsonResponse->GetStringField(TEXT("accessToken"));
                UE_LOG(LogTemp, Log, TEXT("AuthToken generated: %s"), *AuthToken);
            }
        }
        Callback(bWasSuccessful);
    });

}


// This function is called to start a full kID workflow.  For the demo, this function doesn't
// get invoked until the player presses the "Start Session" button in the demo controls.  In a real game 
// this would happen on startup based on acquiring the location/jurisdistion from the player's IP address
// or other means.
void UKidWorkflow::StartKidSession(const FString& Location)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is not assigned. Please call InitializeAuthToken first."));
        return;
    }

    FString LoadedChallengeId;
    if (LoadChallengeId(LoadedChallengeId))
    {
        HandleExistingChallenge(LoadedChallengeId);
        return;
    }

    if (GetSavedSessionInfo())
    {
        UE_LOG(LogTemp, Log, TEXT("Saved Session found."));
        if (SessionInfo->HasField(TEXT("sessionId")))
        {
            FString SessionId = SessionInfo->GetStringField(TEXT("sessionId"));
            UE_LOG(LogTemp, Log, TEXT("Refreshing session."));
            FString ETag = SessionInfo->GetStringField(TEXT("etag"));
            GetSessionPermissions(SessionId, ETag);
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("No age gate was necessary for this location."));
        }
    }
    else
    {
        GetUserAge(Location, [this, Location](bool ageGateShown, bool bAgeAssuranceRequired, const FString& DOB)
        {
            if (ageGateShown) {
                if (bAgeAssuranceRequired)
                {
                    ValidateAge(DOB, [this, Location, DOB](bool bValidated)
                    {
                        if (bValidated)
                        {
                            UE_LOG(LogTemp, Log, TEXT("Age successfully validated!"));
                            DismissAgeAssuranceWidget();
                            StartKidSessionWithDOB(Location, DOB);
                        }
                        else
                        {
                            UE_LOG(LogTemp, Warning, 
                                    TEXT("Player's age could not be verified.  Stay in Data Lite access mode."));
                            HandleAgeNotVerified();
                        }
                    });
                }
                else
                {
                    UE_LOG(LogTemp, Log, TEXT("Location is %s"), *Location);
                    StartKidSessionWithDOB(Location, DOB);
                }      
            } 
            else
            {
                UE_LOG(LogTemp, Log, TEXT("No age gate needed for location %s"), *Location);
                GetDefaultPermissions(Location);
            }
        });
    }
}

void UKidWorkflow::HandleExistingChallenge(const FString& ChallengeId)
{
    HttpRequestHelper::GetRequestWithAuth(BaseUrl + TEXT("/challenge/get?challengeId=") + ChallengeId, AuthToken, [this, ChallengeId](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("Call to /challenge/get succeeded: %s"), *Response->GetContentAsString());

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                FString OneTimePassword = JsonResponse->GetStringField(TEXT("oneTimePassword"));
                FString QRCodeUrl = JsonResponse->GetStringField(TEXT("url"));

                ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OneTimePassword, QRCodeUrl, [this](bool bConsentGranted, 
                        const FString &SessionId)
                {
                    if (bConsentGranted)
                    {
                        GetSessionPermissions(SessionId, TEXT(""));
                        ClearChallengeId();
                    }
                    else
                    {
                        HandleNoConsent();
                    }
                });
            }
        }
    });
}

// This function is called when a player has passed the age gate and assurance if necessary
// and is ready to start a session or the age was already known because the game has stored 
// the kID session information previously and associated it with an identity.
void UKidWorkflow::StartKidSessionWithDOB(const FString& Location, const FString& DOB)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is not assigned. Please call InitializeAuthToken first."));
        return;
    }

    TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
    JsonObject->SetStringField(TEXT("dateOfBirth"), DOB);
    JsonObject->SetStringField(TEXT("jurisdiction"), Location);

    FString ContentJsonString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJsonString);
    FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

    HttpRequestHelper::PostRequestWithAuth(BaseUrl + TEXT("/age-gate/check"), ContentJsonString, AuthToken, [this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful)
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                FString Status = JsonResponse->GetStringField(TEXT("status"));
                if (Status == TEXT("CHALLENGE"))
                {
                    TSharedPtr<FJsonObject> Challenge = JsonResponse->GetObjectField(TEXT("challenge"));
                    FString ChallengeId = Challenge->GetStringField(TEXT("challengeId"));
                    FString OneTimePassword = Challenge->GetStringField(TEXT("oneTimePassword"));
                    FString QRCodeUrl = Challenge->GetStringField(TEXT("url"));
                    SaveChallengeId(ChallengeId);

                    ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OneTimePassword, QRCodeUrl, [this](bool bConsentGranted,
                            const FString &SessionId)
                    {
                        if (bConsentGranted)
                        {
                            GetSessionPermissions(SessionId, TEXT(""));
                            ClearChallengeId();
                        }
                        else
                        {
                            HandleNoConsent();
                        }
                    });
                }
                else if (Status == TEXT("PASS"))
                {
                    Mode = AccessMode::Full;
                    SessionInfo = JsonResponse->GetObjectField(TEXT("session"));
                    SaveSessionInfo();
                }
                else if (Status == TEXT("PROHIBITED"))
                {
                    HandleProhibitedStatus();
                }
            }
        }
    });
}

void UKidWorkflow::GetUserAge(const FString& Location, TFunction<void(bool, bool, const FString&)> Callback)
{
    FString Url = BaseUrl + TEXT("/age-gate/should-display?jurisdiction=") + Location;

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this, Location, Callback](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("Call to /age-gate/should-display succeeded: %s"), *Response->GetContentAsString());

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                bool bShouldDisplay = JsonResponse->GetBoolField(TEXT("shouldDisplay"));
                bool bAgeAssuranceRequired = JsonResponse->GetBoolField(TEXT("ageAssuranceRequired"));

                if (bShouldDisplay)
                {
                    ShowAgeGate([Callback, bAgeAssuranceRequired](const FString& DOB)
                    {
                        Callback(true, bAgeAssuranceRequired, DOB);
                    });
                } 
                else 
                {    
                    Callback(false /* ageGateShown */, false /* ageAssuranceRequired */, TEXT(""));
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Call to /age-gate/should-display failed"));
        }
    });
}

void UKidWorkflow::ValidateAge(const FString& DOB, TFunction<void(bool)> Callback)
{
    UE_LOG(LogTemp, Log, TEXT("Validating age..."));
    ShowAgeAssuranceWidget(DOB, Callback);
}

void UKidWorkflow::GetDefaultPermissions(const FString& Location)
{
    // use a default date of birth for age gate that would be considered a legal adult
    FString dob = TEXT("1970");

    FString Url = FString::Printf(TEXT("%s/age-gate/get-default-permissions?jurisdiction=%s&dateOfBirth=%s"), *BaseUrl, *Location, *dob);
    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, SessionInfo))
            {
                Mode = AccessMode::Full;
                SaveSessionInfo();
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Call to GetDefaultPermissions failed"));
        }
    });
}

void UKidWorkflow::ShowConsentChallenge(const FString& ChallengeId, int32 Timeout, const FString& OTP, 
                const FString& QRCodeUrl, TFunction<void(bool, const FString&)> OnConsentGranted)
{
    FDateTime StartTime = FDateTime::UtcNow();

    ShowFloatingChallengeWidget(OTP, QRCodeUrl, [this, ChallengeId](const FString& Email)
    {
        TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
        JsonObject->SetStringField(TEXT("email"), Email);
        JsonObject->SetStringField(TEXT("challengeId"), ChallengeId);

        FString ContentJsonString;
        TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJsonString);
        FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

        HttpRequestHelper::PostRequestWithAuth(BaseUrl + TEXT("/challenge/send-email"), ContentJsonString, AuthToken, [](FHttpResponsePtr Response, bool bWasSuccessful)
        {
            if (bWasSuccessful && Response.IsValid())
            {       
                UE_LOG(LogTemp, Log, TEXT("/challenge/send-email succeeded with %s"), *Response->GetContentAsString());
            }   
            else
            {
                UE_LOG(LogTemp, Error, TEXT("/challenge/send-email failed"));
            }
        });
    });

    CheckForConsent(ChallengeId, StartTime, Timeout, OnConsentGranted);
}

void UKidWorkflow::CheckForConsent(const FString& ChallengeId, FDateTime StartTime, int32 Timeout, 
                        TFunction<void(bool, const FString &)> OnConsentGranted)
{
    // *kID challenge/await timeout parameter*
    // This value could be assigned the value of Timeout if you want to simply wait 
    // for the long poll to return and reduce the number of retries.  
    // It is set to 1 second for demo purposes as entering Play In Editor mode in 
    // Unreal will leave the long poll hanging after quitting the game.
    const int challengeAwaitTimeout = 1;

    FString Url = FString::Printf(TEXT("%s/challenge/await?challengeId=%s&timeout=%d"), 
                *BaseUrl, *ChallengeId, challengeAwaitTimeout);

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, 
            [this, ChallengeId, StartTime, Timeout, OnConsentGranted]
            (FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bShutdown)
        {
            FString LogMessage = TEXT("CheckForConsent was called after the game instance has been shut down.");
            UE_LOG(LogTemp, Warning, TEXT("%s"), *LogMessage);
            return;
        }

        if (!HasChallengeId())
        {
            FString LogMessage = TEXT("Challenge ID was cleared while waiting for consent.");
            UE_LOG(LogTemp, Warning, TEXT("%s"), *LogMessage);
            return;
        }

        bool bConsentGranted = false;

        if (bWasSuccessful && Response.IsValid())
        {
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                FString Status = JsonResponse->GetStringField(TEXT("status"));

                if (Status == TEXT("PASS"))
                {
                    FString SessionId = JsonResponse->GetStringField(TEXT("sessionId"));
                    OnConsentGranted(true, SessionId);
                    return;
                }
                else if (Status == TEXT("FAIL"))
                {
                    OnConsentGranted(false, TEXT(""));
                    return;
                }
            }

            FDateTime CurrentTime = FDateTime::UtcNow();
            FTimespan ElapsedTime = CurrentTime - StartTime;

            // Retry if the elapsed time is less than the overall timeout
            if (ElapsedTime.GetTotalSeconds() < Timeout)
            {
                GetWorld()->GetTimerManager().SetTimer(ConsentPollingTimerHandle, [this, ChallengeId, StartTime, Timeout, OnConsentGranted]()
                {
                    CheckForConsent(ChallengeId, StartTime, Timeout, OnConsentGranted);
                }, ConsentPollingInterval, false);
            }
            else
            {
                OnConsentGranted(false, TEXT(""));
            }
        }
        else
        {
            OnConsentGranted(false, TEXT(""));
        }

    });
}

void UKidWorkflow::GetSessionPermissions(const FString& SessionId, const FString& ETag)
{
    FString Url = FString::Printf(TEXT("%s/session/get?sessionId=%s&etag=%s"), *BaseUrl, *SessionId, *ETag);

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            Mode = AccessMode::Full;
            if (Response->GetResponseCode() == 304)
            {
                UE_LOG(LogTemp, Log, TEXT("Session information is up-to-date."));
                return;
            }

            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, SessionInfo))
            {
                SaveSessionInfo();
            }
        }   
        else
        {
            UE_LOG(LogTemp, Error, TEXT("/session/get failed"));
        }
    });
}

void UKidWorkflow::AttemptTurnOnRestrictedFeature(const FString& FeatureName, TFunction<void()> EnableFeature)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is not assigned. Please call InitializeAuthToken first."));
        return;
    }

    TSharedPtr<FJsonObject> PermissionObject = FindPermission(FeatureName);
    if (PermissionObject)
    {
        bool bEnabled = PermissionObject->GetBoolField(TEXT("enabled"));

        if (bEnabled)
        {
            UE_LOG(LogTemp, Log, TEXT("Feature %s is already enabled."), *FeatureName);
            return;
        }

        FString ManagedBy = PermissionObject->GetStringField(TEXT("managedBy"));

        if (ManagedBy == TEXT("PLAYER"))
        {
            EnableFeature();
            PermissionObject->SetBoolField(TEXT("enabled"), true);
            SaveSessionInfo();
        }
        else if (ManagedBy == TEXT("GUARDIAN"))
        {
            ShowFeatureConsentChallenge(EnableFeature);

        }
        else if (ManagedBy == TEXT("PROHIBITED"))
        {
            UE_LOG(LogTemp, Warning, TEXT("Feature %s is prohibited for this player."), *FeatureName);
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Feature %s not found."), *FeatureName);
    }
}

void UKidWorkflow::ShowFeatureConsentChallenge(TFunction<void()> EnableFeature)
{
    HttpRequestHelper::GetRequestWithAuth(BaseUrl + TEXT("/session/upgrade"), AuthToken, 
                        [this, EnableFeature](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            FString ChallengeId, QRCodeUrl, OTP;
            UE_LOG(LogTemp, Log, TEXT("/session/upgrade succeeded with %s"), *Response->GetContentAsString());
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                ChallengeId = JsonResponse->GetStringField(TEXT("challengeId"));
                QRCodeUrl = JsonResponse->GetStringField(TEXT("url"));
                OTP = JsonResponse->GetStringField(TEXT("oneTimePassword"));
            }

            ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OTP, QRCodeUrl, [this, EnableFeature]
                        (bool bConsentGranted, const FString& SessionId)
            {
                if (bConsentGranted)
                {
                    GetSessionPermissions(SessionId, TEXT(""));
                    ClearChallengeId();
                    EnableFeature();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Consent for feature was denied."));
                }
            });
        }
    });
}

void UKidWorkflow::EnableChat()
{
    UE_LOG(LogTemp, Log, TEXT("Text chat has been enabled."));
}

void UKidWorkflow::AttemptTurnOnChat()
{
    AttemptTurnOnRestrictedFeature(TEXT("text-chat-public"), [this]()
    {
        EnableChat();
    });
}

void UKidWorkflow::SetChallengeStatus(const FString& Location)
{
    ShowTestSetChallengeWidget([this, Location](const FString& Status, const FString& AgeString)
    {
        FString ChallengeId;
        if (LoadChallengeId(ChallengeId)) {
            TSharedPtr<FJsonObject> JsonObject = MakeShareable(new FJsonObject());
            JsonObject->SetStringField(TEXT("status"), Status);
            JsonObject->SetStringField(TEXT("challengeId"), ChallengeId);
            JsonObject->SetStringField(TEXT("jurisdiction"), Location);
            int32 age = 0;
            if (FDefaultValueHelper::ParseInt(AgeString, age))
            {
                // Assign the integer to a JSON object as a number field
                JsonObject->SetNumberField(TEXT("age"), age);
            }

            FString ContentJsonString;
            TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&ContentJsonString);
            FJsonSerializer::Serialize(JsonObject.ToSharedRef(), Writer);

            HttpRequestHelper::PostRequestWithAuth(BaseUrl + TEXT("/test/set-challenge-status"), ContentJsonString, AuthToken, 
                            [](FHttpResponsePtr Response, bool bWasSuccessful)
            {
                if (bWasSuccessful && Response.IsValid())
                {
                    UE_LOG(LogTemp, Log, TEXT("/test/set-challenge-status succeeded"));
                }
            });
        }
   });
}

void UKidWorkflow::HandleProhibitedStatus()
{
    UE_LOG(LogTemp, Warning, TEXT("Player does not meet the minimum age requirement. Play is disabled."));
    DismissFloatingChallengeWidget();
    ShowUnavailableWidget();
    Mode = AccessMode::None;
}

void UKidWorkflow::HandleNoConsent()
{
    UE_LOG(LogTemp, Warning, TEXT("Player has no consent.  Stay in Data Lite access mode."));
    DismissFloatingChallengeWidget();
    Mode = AccessMode::DataLite;
}

void UKidWorkflow::HandleAgeNotVerified()
{
    DismissAgeAssuranceWidget();
    Mode = AccessMode::DataLite;
}

void UKidWorkflow::DismissFloatingChallengeWidget()
{
    if (FloatingChallengeWidget && FloatingChallengeWidget->IsInViewport())
    {
        FloatingChallengeWidget->RemoveFromParent();
        FloatingChallengeWidget = nullptr;
        DemoControlsWidget->SetTestSetChallengeButtonVisibility(false);
    }
}

void UKidWorkflow::DismissAgeAssuranceWidget()
{
    if (AgeAssuranceWidget && AgeAssuranceWidget->IsInViewport())
    {
        AgeAssuranceWidget->StopHttpServer();
        AgeAssuranceWidget->RemoveFromParent();
        AgeAssuranceWidget = nullptr;
    }
}

void UKidWorkflow::SaveChallengeId(const FString& InChallengeId)
{
    FFileHelper::SaveStringToFile(InChallengeId, *(FPaths::ProjectSavedDir() + TEXT("/ChallengeId.txt")));
    UpdateHUDText();
}

void UKidWorkflow::ClearChallengeId()
{
    IFileManager::Get().Delete(*(FPaths::ProjectSavedDir() + TEXT("/ChallengeId.txt")));
    DismissFloatingChallengeWidget();
    UpdateHUDText();
}

bool UKidWorkflow::LoadChallengeId(FString& OutChallengeId)
{
    return FFileHelper::LoadFileToString(OutChallengeId, *(FPaths::ProjectSavedDir() + TEXT("/ChallengeId.txt")));
}

bool UKidWorkflow::HasChallengeId()
{
    return IFileManager::Get().FileExists(*(FPaths::ProjectSavedDir() + TEXT("/ChallengeId.txt")));
}

void UKidWorkflow::ClearSession()
{
    Mode = AccessMode::DataLite;
    if (AgeGateWidget && AgeGateWidget->IsInViewport())
    {
        AgeGateWidget->RemoveFromParent();
        AgeGateWidget = nullptr;
    }

    if (FloatingChallengeWidget && FloatingChallengeWidget->IsInViewport())
    {
        FloatingChallengeWidget->RemoveFromParent();
        FloatingChallengeWidget = nullptr;
    }

    SessionInfo.Reset();
    FString SessionFilePath = FPaths::ProjectSavedDir() + TEXT("/SessionInfo.json");
    if (IFileManager::Get().Delete(*SessionFilePath))
    {
        UE_LOG(LogTemp, Log, TEXT("Session file deleted successfully."));
    }
    UpdateHUDText();
}

void UKidWorkflow::SaveSessionInfo()
{
    FString SessionString;
    TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&SessionString);
    FJsonSerializer::Serialize(SessionInfo.ToSharedRef(), Writer);
    FFileHelper::SaveStringToFile(SessionString, *(FPaths::ProjectSavedDir() + TEXT("/SessionInfo.json")));
    UpdateHUDText();
}

bool UKidWorkflow::GetSavedSessionInfo()
{
    FString SessionInfoString;
    if (FFileHelper::LoadFileToString(SessionInfoString, *(FPaths::ProjectSavedDir() + TEXT("/SessionInfo.json"))))
    {
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(SessionInfoString);
        if (FJsonSerializer::Deserialize(Reader, SessionInfo))
        {
            UE_LOG(LogTemp, Log, TEXT("Found saved session."));
            Mode = AccessMode::Full;
            UpdateHUDText();
            return true;
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to deserialize saved session."));     
        }
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("No saved session found."));
    }
    return false;
}

void UKidWorkflow::UpdateHUDText()
{
    if (PlayerHUDWidget)
    {
        if (!AuthToken.IsEmpty())
        {
            FString AgeStatus = SessionInfo.IsValid() ? SessionInfo->GetStringField(TEXT("ageStatus")) : TEXT("N/A");
            FString SessionId = SessionInfo.IsValid() ? 
                        SessionInfo->HasField(TEXT("sessionId")) ? SessionInfo->GetStringField(TEXT("sessionId")) : TEXT("Default Permissions")  : TEXT("N/A");
            FString ChallengeId;
            FString HUDText = FString::Printf(
                        TEXT("Session: %s\nChallenge: %s\nAge Status: %s\nAccess Mode: %s"), 
                        *SessionId, 
                        LoadChallengeId(ChallengeId) ? *ChallengeId : TEXT("N/A"), 
                        *AgeStatus,
                        (Mode == AccessMode::None) ? TEXT("None") : 
                            (Mode == AccessMode::DataLite) ? TEXT("Data Lite") : TEXT("Full"));
            PlayerHUDWidget->SetText(HUDText);
        } 
        else    
        {
            PlayerHUDWidget->SetText(TEXT("AuthToken not initialized. Check log for details."));
        }
    }
}

void UKidWorkflow::ShowAgeGate(TFunction<void(const FString&)> Callback)
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* AgeGateWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_AgeGateWidget.BP_AgeGateWidget_C"));
        if (AgeGateWidgetClass)
        {
            AgeGateWidget = CreateWidget<UAgeGateWidget>(GEngine->GameViewport->GetWorld(), AgeGateWidgetClass);
            if (AgeGateWidget)
            {
                AgeGateWidget->InitializeWidget(Callback);
                AgeGateWidget->AddToViewport();
            }
        }
    }
}

void UKidWorkflow::ShowTestSetChallengeWidget(TFunction<void(const FString&, const FString&)> Callback)
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* TestSetChallengeWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_TestSetChallengeWidget.BP_TestSetChallengeWidget_C"));
        if (TestSetChallengeWidgetClass)
        {
            UTestSetChallengeWidget* TestSetChallengeWidget = CreateWidget<UTestSetChallengeWidget>(GEngine->GameViewport->GetWorld(), TestSetChallengeWidgetClass);
            if (TestSetChallengeWidget)
            {
                TestSetChallengeWidget->InitializeWidget(Callback);
                TestSetChallengeWidget->AddToViewport();
            }
        }
    }
}

void UKidWorkflow::ShowDemoControls()
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* DemoControlsClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_DemoControlsWidget.BP_DemoControlsWidget_C"));
        if (DemoControlsClass)
        {
            DemoControlsWidget = CreateWidget<UDemoControlsWidget>(GEngine->GameViewport->GetWorld(), DemoControlsClass);
            if (DemoControlsWidget)
            {
                DemoControlsWidget->SetKidWorkflow(this);
                DemoControlsWidget->SetTestSetChallengeButtonVisibility(false);
                DemoControlsWidget->AddToViewport();
            }
        }
    }
}

void UKidWorkflow::ShowPlayerHUD()
{
    if (GEngine && GEngine->GameViewport)
    {
        if (!PlayerHUDWidget)
        {
            // Load the widget blueprint dynamically
            UClass* HUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_PlayerHUDWidget.BP_PlayerHUDWidget_C"));
            if (HUDWidgetClass)
            {
                PlayerHUDWidget = Cast<UPlayerHUDWidget>(CreateWidget<UUserWidget>(GEngine->GameViewport->GetWorld(), HUDWidgetClass));
                if (PlayerHUDWidget)
                {
                    // Add the widget to the viewport
                    PlayerHUDWidget->AddToViewport();

                    UpdateHUDText();
                }
            }
        }
    }
}

void UKidWorkflow::ShowFloatingChallengeWidget(const FString& OTP, const FString& QRCodeUrl, TFunction<void(const FString&)> OnEmailSubmitted)
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* FloatingChallengeWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_FloatingChallengeWidget.BP_FloatingChallengeWidget_C"));
        if (FloatingChallengeWidgetClass)
        {
            FloatingChallengeWidget = CreateWidget<UFloatingChallengeWidget>(GEngine->GameViewport->GetWorld(), FloatingChallengeWidgetClass);
            if (FloatingChallengeWidget)
            {
                FloatingChallengeWidget->InitializeWidget(this, OTP, QRCodeUrl, OnEmailSubmitted);
                FloatingChallengeWidget->AddToViewport();
                DemoControlsWidget->SetTestSetChallengeButtonVisibility(true);
            }
        }
    }
}

void UKidWorkflow::ShowAgeAssuranceWidget(const FString& DateOfBirth, TFunction<void(bool)> OnAssuranceResponse)
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* AgeAssuranceWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_AgeAssuranceWidget.BP_AgeAssuranceWidget_C"));
        if (AgeAssuranceWidgetClass)
        {
            AgeAssuranceWidget = CreateWidget<UAgeAssuranceWidget>(GEngine->GameViewport->GetWorld(), AgeAssuranceWidgetClass);
            if (AgeAssuranceWidget)
            {
                AgeAssuranceWidget->InitializeWidget(DateOfBirth, OnAssuranceResponse);
                AgeAssuranceWidget->AddToViewport();
            }
        }
    }
}

void UKidWorkflow::ShowUnavailableWidget()
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* UnavailableWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/kID/BP_UnavailableWidget.BP_UnavailableWidget_C"));
        if (UnavailableWidgetClass)
        {
            UUnavailableWidget* UnavailableWidget = CreateWidget<UUnavailableWidget>(GEngine->GameViewport->GetWorld(), UnavailableWidgetClass);
            if (UnavailableWidget)
            {
                UnavailableWidget->AddToViewport();
            }
        }
    }
}

TSharedPtr<FJsonObject> UKidWorkflow::FindPermission(const FString& FeatureName)
{
    TArray<TSharedPtr<FJsonValue>> Permissions = SessionInfo->GetArrayField(TEXT("permissions"));
    for (auto& Permission : Permissions)
    {
        TSharedPtr<FJsonObject> PermissionObject = Permission->AsObject();
        if (PermissionObject->GetStringField(TEXT("name")) == FeatureName)
        {
            return PermissionObject;
        }
    }
    return nullptr;
}

void UKidWorkflow::CleanUp()
{
    DismissAgeAssuranceWidget();
    bShutdown = true;
    UE_LOG(LogTemp, Log, TEXT("Cleaning up."));
    if (ConsentPollingTimerHandle.IsValid())
    {
        GetWorld()->GetTimerManager().ClearTimer(ConsentPollingTimerHandle);
    }
}