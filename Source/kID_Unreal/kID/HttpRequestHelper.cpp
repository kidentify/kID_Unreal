#include "HttpRequestHelper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "Engine/Engine.h"
#include "TimerManager.h"

namespace
{
    constexpr int32 MaxRetries = 3;
    constexpr float DefaultRetryDelay = 5.0f;
}

void HttpRequestHelper::ScheduleRetry(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback, int RetryCount, float RetryDelay)
{
    if (RetryCount <= 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Maximum retries reached"));
        Callback(nullptr, false);
        return;
    }

    UWorld* World = GEngine->GameViewport->GetWorld();
    if (World)
    {
        FTimerDelegate TimerDelegate;
        TimerDelegate.BindLambda([Request, Callback, RetryCount]()
        {
            RetryRequest(Request, Callback, RetryCount - 1); // Use default delay
        });

        FTimerHandle TimerHandle;
        World->GetTimerManager().SetTimer(TimerHandle, TimerDelegate, RetryDelay, false);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("World context is null"));
        Callback(nullptr, false);
    }
}

void HttpRequestHelper::RetryRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback, int RetryCount)
{
    Request->OnProcessRequestComplete().BindLambda([Callback, Request, RetryCount](FHttpRequestPtr RequestPtr, FHttpResponsePtr Response, bool bWasSuccessful) mutable
    {
        if (bWasSuccessful && Response.IsValid())
        {
            if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 304)
            {
                UE_LOG(LogTemp, Log, TEXT("Call succeeded: %s"), *Response->GetContentAsString());
                Callback(Response, true);
            }
            else if (Response->GetResponseCode() == 429)
            {
                FString RetryAfterHeader = Response->GetHeader("Retry-After");
                float RetryDelay = DefaultRetryDelay;

                if (!RetryAfterHeader.IsEmpty())
                {
                    RetryDelay = FCString::Atof(*RetryAfterHeader);
                    if (RetryDelay <= 0) {
                        RetryDelay = DefaultRetryDelay;
                    }
                }

                UE_LOG(LogTemp, Warning, TEXT("Received 429 Too Many Requests, retrying in %f seconds..."), RetryDelay);
                ScheduleRetry(Request, Callback, RetryCount, RetryDelay);
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("Call failed: %s"), *Response->GetContentAsString());
                Callback(Response, false);
            }
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("Call failed: response is invalid"));
            Callback(Response, false);
        }
    });

    Request->ProcessRequest();
}

void HttpRequestHelper::GetRequest(const FString& Url, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    UE_LOG(LogTemp, Log, TEXT("Call to %s"), *Url);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");

    RetryRequest(Request, Callback, MaxRetries);
}

void HttpRequestHelper::GetRequestWithAuth(const FString& Url, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is empty!"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Call to %s"), *Url);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Authorization", "Bearer " + AuthToken);
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");

    RetryRequest(Request, Callback, MaxRetries);
}

void HttpRequestHelper::PostRequestWithAuth(const FString& Url, const FString& ContentJsonString, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    if (AuthToken.IsEmpty())
    {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is empty!"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Call to %s with body %s"), *Url, *ContentJsonString);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->SetURL(Url);
    Request->SetVerb("POST");
    Request->SetHeader("Authorization", "Bearer " + AuthToken);
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");
    Request->SetContentAsString(ContentJsonString);

    RetryRequest(Request, Callback, MaxRetries);
}
