#include "HttpRequestHelper.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"

void HttpRequestHelper::GetRequest(const FString& Url, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    UE_LOG(LogTemp, Log, TEXT("Call to %s"), *Url);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid()) {
            if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 304) {
                UE_LOG(LogTemp, Log, TEXT("Call succeeded: %s"), *Response->GetContentAsString());
                Callback(Response, true);
            } else {    
                UE_LOG(LogTemp, Error, TEXT("Call failed: %s"), *Response->GetContentAsString());
                Callback(Response, false);
            }
        } else {
            UE_LOG(LogTemp, Error, TEXT("Call failed: response is invalid"));
        }
    });
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");
    Request->ProcessRequest();
}

void HttpRequestHelper::GetRequestWithAuth(const FString& Url, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    if (AuthToken.IsEmpty()) {
        UE_LOG(LogTemp, Error, TEXT("AuthToken is empty!"));
        return;
    }
    UE_LOG(LogTemp, Log, TEXT("Call to %s"), *Url);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid()) {
            if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 304) {
                UE_LOG(LogTemp, Log, TEXT("Call succeeded: %s"), *Response->GetContentAsString());
                Callback(Response, true);
            } else {    
                UE_LOG(LogTemp, Error, TEXT("Call failed: %s"), *Response->GetContentAsString());
                Callback(Response, false);
            }
        } else {
            UE_LOG(LogTemp, Error, TEXT("Call failed: response is invalid"));
        }
    });
    Request->SetURL(Url);
    Request->SetVerb("GET");
    Request->SetHeader("Authorization", "Bearer " + AuthToken);
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");
    Request->ProcessRequest();
}

void HttpRequestHelper::PostRequestWithAuth(const FString& Url, const FString& ContentJsonString, const FString& AuthToken, TFunction<void(TSharedPtr<IHttpResponse, ESPMode::ThreadSafe>, bool)> Callback)
{
    UE_LOG(LogTemp, Log, TEXT("Call to %s"), *Url);
    TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = FHttpModule::Get().CreateRequest();
    Request->OnProcessRequestComplete().BindLambda([Callback](FHttpRequestPtr Request, FHttpResponsePtr Response, bool bWasSuccessful)
    {
        if (bWasSuccessful && Response.IsValid()) {
            if (Response->GetResponseCode() == 200 || Response->GetResponseCode() == 304) {
                UE_LOG(LogTemp, Log, TEXT("Call succeeded: %s"), *Response->GetContentAsString());
                Callback(Response, true);
            } else {    
                UE_LOG(LogTemp, Error, TEXT("Call failed: %s"), *Response->GetContentAsString());
                Callback(Response, false);
            }
        } else {
            UE_LOG(LogTemp, Error, TEXT("Call failed: response is invalid"));
        }
    });
    Request->SetURL(Url);
    Request->SetVerb("POST");
    Request->SetHeader("Authorization", "Bearer " + AuthToken);
    Request->SetHeader("Content-Type", "application/json");
    Request->SetHeader("accept", "application/json");
    Request->SetContentAsString(ContentJsonString);
    Request->ProcessRequest();
}