#pragma once

#include "CoreMinimal.h"
#include "Http.h"

class HttpRequestHelper 
{
public:
    static void GetRequest(const FString& Url, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);
    static void GetRequestWithAuth(const FString& Url, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);
    static void PostRequestWithAuth(const FString& Url, const FString& ContentJsonString, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback);
};