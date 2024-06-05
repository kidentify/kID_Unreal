#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "HAL/Runnable.h"
#include "HttpModule.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "PrivatelyHttpServer.generated.h"

UCLASS()
class UFPrivatelyHttpServer : public UObject
{
    GENERATED_BODY()

public:
    UFPrivatelyHttpServer();
    ~UFPrivatelyHttpServer();

    bool StartServer(int32 Port);
    void StopServer();

    void Initialize(TFunction<void(bool)> InValidateAdultCallback);

private:
    FSocket* ListenerSocket;
    FSocket* ClientSocket;
    FRunnableThread* ServerThread;
    FThreadSafeBool bStopThread;
    FString InitialPasscode;
    FString StoredFuturePasscode;
    bool bPasscodeUsed = false;
    TFunction<void(bool)> ValidateAdultCallback;
    FString PrivatelyURL;

    // Internal runnable delegate
    class FServerRunnable : public FRunnable
    {
    public:
        FServerRunnable(UFPrivatelyHttpServer* InServer);
        virtual uint32 Run() override;

    private:
        UFPrivatelyHttpServer* Server;
    };

    TUniquePtr<FRunnable> ServerRunnable;

    void HandleClient();
    void ProcessHttpRequest(const FString& Request);
    void SendResponse(const FString& Response);
    void Log(const FString& Message);

    UFUNCTION()
    void OnPIEEnd(bool bIsSimulating);

    FString ReadHtmlFile();
    int32 GenerateRandomCode();
    bool IsBrowserSupported();
};