#pragma once

#include "CoreMinimal.h"
#include "Http.h"

class HttpRequestHelper 
{
public:
    static void GetRequest(const FString& Url, 
        TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);
    static void GetRequestWithAuth(const FString& Url, const FString& AuthToken, 
        TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);
    static void PostRequestWithAuth(const FString& Url, const FString& ContentJsonString, 
        const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);

private:
    static void ScheduleRetry(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, 
            TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback, 
            int RetryCount, float RetryDelay);

    static void RetryRequest(TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request, 
            TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback, int RetryCount);
};