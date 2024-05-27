#include "KidWorkflow.h"
#include "HttpRequestHelper.h"
#include "Json.h"
#include "JsonUtilities.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
#include "TimerManager.h"
#include "AgeGateWidget.h"
#include "PlayerHUDWidget.h"
#include "FloatingChallengeWidget.h"
#include "DemoControlsWidget.h"

// Constants
const int32 ConsentTimeoutSeconds = 300;
const int32 ConsentPollingInterval = 1;
const FString BaseUrl = TEXT("https://game-api.k-id.com/api/v1");
const FString ApiKey = TEXT("REPLACEMENT_TEXT");
const FString ClientId = TEXT("12345678-1234-1234-1234-123456789012");

bool UKidWorkflow::bShutdown = false;

void UKidWorkflow::Initialize()
{
    bShutdown = false;

    FString payload = TEXT("{ \"clientId\": \"") + ClientId + TEXT("\"}");

    HttpRequestHelper::PostRequestWithAuth(BaseUrl + TEXT("/auth/issue-token"), payload, ApiKey, [this](FHttpResponsePtr Response, bool bWasSuccessful)
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
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Failed to retrieve AuthToken. Response: %s"), *Response->GetContentAsString());
        }
    });
}

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
        if (SessionInfo->HasField(TEXT("sessionId")))
        {
            FString SessionId = SessionInfo->GetStringField(TEXT("sessionId"));
            UE_LOG(LogTemp, Warning, TEXT("Refreshing session."));
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
        if (!GetUserAge(Location, [this, Location](bool bShouldVerify, const FString& DOB)
        {
            if (bShouldVerify)
            {
                ValidateAge([this, Location, DOB](bool bValidated)
                {
                    if (bValidated)
                    {
                        StartKidSessionWithDOB(Location, DOB);
                    }
                    else
                    {
                        HandleProhibitedStatus();
                    }
                });
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("Location is %s"), *Location);
                StartKidSessionWithDOB(Location, DOB);
            }
        }))
        {
            GetDefaultPermissions(Location);
        }
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

                ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OneTimePassword, QRCodeUrl, [this](bool bConsentGranted)
                {
                    if (bConsentGranted)
                    {
                        ClearChallengeId();
                    }
                    else
                    {
                        HandleProhibitedStatus();
                    }
                });
            }
        }
    });
}

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
            UE_LOG(LogTemp, Log, TEXT("Call to /age-gate/check succeeded: %s"), *Response->GetContentAsString());
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

                    ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OneTimePassword, QRCodeUrl, [this](bool bConsentGranted)
                    {
                        if (bConsentGranted)
                        {
                            ClearChallengeId();
                        }
                        else
                        {
                            HandleProhibitedStatus();
                        }
                    });
                }
                else if (Status == TEXT("PASS"))
                {
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

bool UKidWorkflow::GetUserAge(const FString& Location, TFunction<void(bool, const FString&)> Callback)
{
    bool bShouldDisplay = false;

    FString Url = BaseUrl + TEXT("/age-gate/should-display?jurisdiction=") + Location;

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this, Location, Callback, &bShouldDisplay](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("Call to /age-gate/should-display succeeded: %s"), *Response->GetContentAsString());

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                bShouldDisplay = JsonResponse->GetBoolField(TEXT("shouldDisplay"));
                bool bShouldVerify = JsonResponse->GetBoolField(TEXT("ageAssuranceRequired"));

                if (bShouldDisplay)
                {
                    ShowAgeGate([Callback, bShouldVerify](const FString& DOB)
                    {
                        Callback(bShouldVerify, DOB);
                    });
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Call to /age-gate/should-display failed"));
        }
    });

    return bShouldDisplay;
}

void UKidWorkflow::ValidateAge(TFunction<void(bool)> Callback)
{
    UE_LOG(LogTemp, Log, TEXT("Validating age..."));
    bool bValidated = true; // Replace with actual age validation logic
    Callback(bValidated);
}

void UKidWorkflow::GetDefaultPermissions(const FString& Location)
{
    FString dob = TEXT("1970");
    FString Url = FString::Printf(TEXT("%s/age-gate/get-default-permissions?jurisdiction=%s&dateOfBirth=%s"), *BaseUrl, *Location, *dob);
    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, SessionInfo))
            {
                SaveSessionInfo();
                UE_LOG(LogTemp, Log, TEXT("Call to GetDefaultPermissions succeeded: %s"), *Response->GetContentAsString());
            }
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Call to GetDefaultPermissions failed"));
        }
    });
}

void UKidWorkflow::ShowConsentChallenge(const FString& ChallengeId, int32 Timeout, const FString& OTP, const FString& QRCodeUrl, TFunction<void(bool)> OnConsentGranted)
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
            UE_LOG(LogTemp, Log, TEXT("/challenge/send-email succeeded with %s"), *Response->GetContentAsString());
        });
    });

    CheckForConsent(ChallengeId, StartTime, Timeout, OnConsentGranted);
}

void UKidWorkflow::CheckForConsent(const FString& ChallengeId, FDateTime StartTime, int32 Timeout, TFunction<void(bool)> OnConsentGranted)
{
    FString Url = BaseUrl + TEXT("/challenge/await?challengeId=") + ChallengeId + TEXT("&timeout=0");

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this, ChallengeId, StartTime, Timeout, OnConsentGranted](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bShutdown)
        {
            UE_LOG(LogTemp, Error, TEXT("CheckForConsent was called after the game instance has been shut down."));
            return;
        }

        if (!HasChallengeId())
        {
            UE_LOG(LogTemp, Warning, TEXT("Challenge ID was cleared while waiting for consent."));
            return;
        }

        bool bConsentGranted = false;

        if (bWasSuccessful && Response.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("/challenge/await succeeded with %s"), *Response->GetContentAsString());

            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                FString Status = JsonResponse->GetStringField(TEXT("status"));

                if (Status == TEXT("PASS"))
                {
                    OnConsentGranted(true);
                    return;
                }
                else if (Status == TEXT("FAIL"))
                {
                    OnConsentGranted(false);
                    return;
                }
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Error Result from /challenge/await - %s"), *Response->GetContentAsString());
        }

        FDateTime CurrentTime = FDateTime::UtcNow();
        FTimespan ElapsedTime = CurrentTime - StartTime;

        if (ElapsedTime.GetTotalSeconds() < Timeout)
        {
            GetWorld()->GetTimerManager().SetTimer(ConsentPollingTimerHandle, [this, ChallengeId, StartTime, Timeout, OnConsentGranted]()
            {
                CheckForConsent(ChallengeId, StartTime, Timeout, OnConsentGranted);
            }, ConsentPollingInterval, false);
        }
        else
        {
            OnConsentGranted(false);
        }
    });
}

void UKidWorkflow::GetSessionPermissions(const FString& SessionId, const FString& ETag)
{
    FString Url = FString::Printf(TEXT("%s/session/get?sessionId=%s&etag=%s"), *BaseUrl, *SessionId, *ETag);

    HttpRequestHelper::GetRequestWithAuth(Url, AuthToken, [this](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (Response->GetResponseCode() == 304)
        {
            UE_LOG(LogTemp, Log, TEXT("Session information is up-to-date."));
            return;
        }

        if (bWasSuccessful)
        {
            UE_LOG(LogTemp, Log, TEXT("/session/get succeeded with %s"), *Response->GetContentAsString());
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, SessionInfo))
            {
                SaveSessionInfo();
            }
        }
    });
}

void UKidWorkflow::AttemptTurnOnFeature(const FString& FeatureName, TFunction<void()> EnableFeature)
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
            FString ChallengeId, QRCodeUrl, OTP;
            GenerateFeatureChallenge(ChallengeId, QRCodeUrl, OTP);

            ShowConsentChallenge(ChallengeId, ConsentTimeoutSeconds, OTP, QRCodeUrl, [this, EnableFeature](bool bConsentGranted)
            {
                if (bConsentGranted)
                {
                    FString SessionId = SessionInfo->GetStringField(TEXT("sessionId"));
                    FString ETag = SessionInfo->GetStringField(TEXT("etag"));
                    GetSessionPermissions(SessionId, ETag);
                    EnableFeature();
                }
                else
                {
                    UE_LOG(LogTemp, Warning, TEXT("Consent for feature was denied."));
                }
            });
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

void UKidWorkflow::GenerateFeatureChallenge(FString& OutChallengeId, FString& OutQRCodeUrl, FString& OutOTP)
{
    HttpRequestHelper::GetRequestWithAuth(BaseUrl + TEXT("/feature/challenge/generate"), AuthToken, [this, &OutChallengeId, &OutQRCodeUrl, &OutOTP](FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid())
        {
            UE_LOG(LogTemp, Log, TEXT("/feature/challenge/generate succeeded with %s"), *Response->GetContentAsString());
            TSharedPtr<FJsonObject> JsonResponse;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(Response->GetContentAsString());
            if (FJsonSerializer::Deserialize(Reader, JsonResponse))
            {
                OutChallengeId = JsonResponse->GetStringField(TEXT("challengeId"));
                OutQRCodeUrl = JsonResponse->GetStringField(TEXT("url"));
                OutOTP = JsonResponse->GetStringField(TEXT("oneTimePassword"));
            }
        }
    });
}

void UKidWorkflow::HandleProhibitedStatus()
{
    UE_LOG(LogTemp, Warning, TEXT("Player does not meet the minimum age requirement. Play is disabled."));
    DismissChallengeWindow();
    ShowUnavailableWidget();
}

void UKidWorkflow::DismissChallengeWindow()
{
    if (FloatingChallengeWidget && FloatingChallengeWidget->IsInViewport())
    {
        FloatingChallengeWidget->RemoveFromParent();
        FloatingChallengeWidget = nullptr;
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

void UKidWorkflow::EnableChat()
{
    UE_LOG(LogTemp, Log, TEXT("Text chat has been enabled."));
}

void UKidWorkflow::AttemptTurnOnChat()
{
    AttemptTurnOnFeature(TEXT("text-chat-public"), [this]()
    {
        EnableChat();
    });
}

void UKidWorkflow::ClearSession()
{
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
            UpdateHUDText();
            return true;
        }
    }
    return false;
}

void UKidWorkflow::UpdateHUDText()
{
    if (PlayerHUDWidget)
    {
        FString AgeStatus = SessionInfo.IsValid() ? SessionInfo->GetStringField(TEXT("ageStatus")) : TEXT("N/A");
        FString SessionId = SessionInfo.IsValid() && SessionInfo->HasField(TEXT("sessionId")) ? SessionInfo->GetStringField(TEXT("sessionId")) : TEXT("N/A");
        FString ChallengeId;
        FString HUDText = FString::Printf(TEXT("ageStatus: %s sessionId: %s challengeId %s"), *AgeStatus, *SessionId, LoadChallengeId(ChallengeId) ? *ChallengeId : TEXT("N/A"));
        PlayerHUDWidget->SetText(HUDText);
    }
}

void UKidWorkflow::ShowAgeGate(TFunction<void(const FString&)> Callback)
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* AgeGateWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_AgeGateWidget.BP_AgeGateWidget_C"));
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

void UKidWorkflow::ShowDemoControls()
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* DemoControlsClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_DemoControlsWidget.BP_DemoControlsWidget_C"));
        if (DemoControlsClass)
        {
            UDemoControlsWidget* DemoControls = CreateWidget<UDemoControlsWidget>(GEngine->GameViewport->GetWorld(), DemoControlsClass);
            if (DemoControls)
            {
                DemoControls->SetKidWorkflow(this);
                DemoControls->AddToViewport();
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
            UClass* HUDWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_PlayerHUDWidget.BP_PlayerHUDWidget_C"));
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
        UClass* FloatingChallengeWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_FloatingChallengeWidget.BP_FloatingChallengeWidget_C"));
        if (FloatingChallengeWidgetClass)
        {
            FloatingChallengeWidget = CreateWidget<UFloatingChallengeWidget>(GEngine->GameViewport->GetWorld(), FloatingChallengeWidgetClass);
            if (FloatingChallengeWidget)
            {
                FloatingChallengeWidget->InitializeWidget(this, OTP, QRCodeUrl, OnEmailSubmitted);
                FloatingChallengeWidget->AddToViewport();
            }
        }
    }
}

void UKidWorkflow::ShowUnavailableWidget()
{
    if (GEngine && GEngine->GameViewport)
    {
        UClass* UnavailableWidgetClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/FirstPerson/Blueprints/BP_UnavailableWidget.BP_UnavailableWidget_C"));
        if (UnavailableWidgetClass)
        {
            UnavailableWidget = CreateWidget<UUnavailableWidget>(GEngine->GameViewport->GetWorld(), UnavailableWidgetClass);
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
    bShutdown = true;
    UE_LOG(LogTemp, Log, TEXT("Cleaning up."));
    if (ConsentPollingTimerHandle.IsValid() && GetWorld())
    {
        GetWorld()->GetTimerManager().ClearTimer(ConsentPollingTimerHandle);
    }
    if (FloatingChallengeWidget)
    {
        FloatingChallengeWidget->RemoveFromParent();
        FloatingChallengeWidget = nullptr;
    }
    if (AgeGateWidget)
    {
        AgeGateWidget->RemoveFromParent();
        AgeGateWidget = nullptr;
    }
    if(PlayerHUDWidget)
    {
        PlayerHUDWidget->RemoveFromParent();
        PlayerHUDWidget = nullptr;
    }
    if(UnavailableWidget)
    {
        UnavailableWidget->RemoveFromParent();
        UnavailableWidget = nullptr;
    }
}