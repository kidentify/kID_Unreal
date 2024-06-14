#include "PrivatelyHTTPServer.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFilemanager.h"
#include "Misc/FileHelper.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Networking.h"
#include "HttpModule.h"
#include "Engine/Engine.h"
#include "Engine/GameEngine.h"
#include "Kismet/GameplayStatics.h"
#include "Editor/EditorEngine.h"
#include "Editor/UnrealEd/Public/Editor.h"
#include "PlatformUtil.h"
#include "GenericPlatform/GenericPlatformHttp.h"

// useful for httpserver debugging in the editor
static bool LogToFile = false;

// useful to avoid the face scan for round trip testing in the browser
static bool UseFaceScan = true;

// useful to test face scan failure by forcing a failure in the browser.  UseFaceScan needs 
// to be set to true for this to have an effect.
static bool FailFaceScan = false;

static FString PrivatelyBaseURL = TEXT("https://d3ogqhtsivkon3.cloudfront.net/?");

UFPrivatelyHttpServer::UFPrivatelyHttpServer()
    : ListenerSocket(nullptr), ServerThread(nullptr), bStopThread(false)
{
#if WITH_EDITOR
    FEditorDelegates::EndPIE.AddUObject(this, &UFPrivatelyHttpServer::OnPIEEnd);
#endif
}

UFPrivatelyHttpServer::~UFPrivatelyHttpServer()
{
    StopServer();
}

void UFPrivatelyHttpServer::Initialize(TFunction<void(bool, int32, int32)> InAgeValidationCallback)
{
    AgeValidationCallback = InAgeValidationCallback;
}

bool UFPrivatelyHttpServer::StartServer(int32 Port)
{
    if (!IsBrowserSupported())
    {
        Log(TEXT("Default browser is not Chrome or Safari. Server not started."));
        return false;
    }

    FString FilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("privately.json"));
    FString JsonString;

    if (FFileHelper::LoadFileToString(JsonString, *FilePath))
    {
        TSharedPtr<FJsonObject> JsonObject;
        TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

        if (FJsonSerializer::Deserialize(Reader, JsonObject))
        {
            FString SessionId = JsonObject->GetStringField(TEXT("session_id"));
            FString SessionPassword = JsonObject->GetStringField(TEXT("session_password"));

            PrivatelyURL = PrivatelyBaseURL + TEXT("session_id=") + 
                    FGenericPlatformHttp::UrlEncode(SessionId) + 
                    TEXT("&session_password=") + 
                    FGenericPlatformHttp::UrlEncode(SessionPassword);
            
            FTimespan UnixTimeSpan = FDateTime::UtcNow() - FDateTime::FromUnixTimestamp(0);
            int64 TimestampInSeconds = UnixTimeSpan.GetTotalSeconds();
            PrivatelyURL+= TEXT("&timestamp=") + FString::FromInt(TimestampInSeconds);

            UE_LOG(LogTemp, Log, TEXT("Constructed Privately URL: %s"), *PrivatelyURL);
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("%s%s%s"),
                TEXT("A JSON file called privately.json is required to start facial recognition. "),
                TEXT("The format is as follows: "),
                TEXT("{\"session_id\": \"<privately_session_id>\", \"session_password\": \"<privately_session_password>\"}"));
                
            return false;
        }
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("%s%s%s"),
            TEXT("A JSON file called privately.json is required to start facial recognition. "),
            TEXT("The format is as follows: "),
            TEXT("{\"session_id\": \"<privately_session_id>\", \"session_password\": \"<privately_session_password>\"}"));
        
        return false;
    }

    // Generate passcodes
    InitialPasscode = FString::FromInt(GenerateRandomCode());

    // Create a TCP listener socket
    ListenerSocket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(NAME_Stream, TEXT("HTTP_LISTENER"), false);
    if (!ListenerSocket)
    {
        Log(TEXT("Failed to create listener socket"));
        return false;
    }

    // Bind the socket to the port
    FIPv4Address Address;
    FIPv4Address::Parse(TEXT("0.0.0.0"), Address);
    TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
    Addr->SetIp(Address.Value);
    Addr->SetPort(Port);

    if (!ListenerSocket->Bind(*Addr))
    {
        Log(TEXT("Failed to bind listener socket to port"));
        return false;
    }

    // Start listening
    ListenerSocket->Listen(8);

    // Create and start the server runnable
    ServerRunnable = MakeUnique<FServerRunnable>(this);
    ServerThread = FRunnableThread::Create(ServerRunnable.Get(), TEXT("PrivatelyHTTPServerThread"));
    Log(TEXT("Server started on port ") + FString::FromInt(Port));
    return true;
}

void UFPrivatelyHttpServer::StopServer()
{
    bStopThread = true;

    if (ListenerSocket)
    {
        ListenerSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenerSocket);
        ListenerSocket = nullptr;
        Log(TEXT("Server socket closed and destroyed"));
    }

}

void UFPrivatelyHttpServer::HandleClient()
{
    // Set the socket to non-blocking mode
    ClientSocket->SetNonBlocking(true);

    while (!bStopThread)
    {
        TArray<uint8> Buffer;
        int32 BytesRead = 0;
        FString RequestString;
        const int32 BufferSize = 1024; // Define the buffer size

        Buffer.SetNumZeroed(BufferSize);

        // Read data directly from the socket
        while (!bStopThread)
        {
            // Attempt to read data into the buffer
            bool bSuccess = ClientSocket->Recv(Buffer.GetData(), BufferSize, BytesRead);

            if (BytesRead > 0)
            {
                Log(TEXT("Read ") + FString::FromInt(BytesRead) + TEXT(" bytes"));
                // Append the read data to the request string
                RequestString.Append(FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(Buffer.GetData())), BytesRead));

                // Check if the HTTP headers are fully received
                if (RequestString.Contains(TEXT("\r\n\r\n")))
                {
                    break;
                }
            }
            else 
            {
                // If no data is received, sleep for a short period to avoid busy waiting
                FPlatformProcess::Sleep(0.01f);
            }
        }

        if (RequestString.IsEmpty())
        {
            Log(TEXT("No data received"));
            break;
        }

        Log(TEXT("Received: ") + RequestString);

        // Process the HTTP request and get the response
        ProcessHttpRequest(RequestString);

        Log(TEXT("Closed socket connection"));
        ClientSocket->Close();
        // ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
        ClientSocket = nullptr;
        break;
    }
    if (ClientSocket) {
        Log(TEXT("Closed socket connection"));
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
        ClientSocket = nullptr;
    }
}

void UFPrivatelyHttpServer::SendResponse(const FString& Response)
{
    int32 BytesSent = 0;
    TArray<uint8> ResponseData;
    ResponseData.Append(reinterpret_cast<const uint8*>(TCHAR_TO_UTF8(*Response)), Response.Len());
    ClientSocket->Send(ResponseData.GetData(), ResponseData.Num(), BytesSent);
    Log(TEXT("Sent ") + FString::FromInt(BytesSent) + TEXT(" bytes out of ") + FString::FromInt(ResponseData.Num()));
}

void UFPrivatelyHttpServer::ProcessHttpRequest(const FString& Request)
{
    // Basic parsing of the HTTP request
    if (Request.StartsWith(TEXT("GET ")))
    {
        // Extract the URL from the request
        FString URL;
        const FString Prefix = TEXT("GET ");
        const FString Suffix = TEXT(" HTTP/");
        int32 Start = Request.Find(Prefix) + Prefix.Len();
        int32 End = Request.Find(Suffix, ESearchCase::IgnoreCase, ESearchDir::FromStart, Start);
        URL = Request.Mid(Start, End - Start).TrimStart();

        // Ensure the URL does not start with '/'
        if (URL.StartsWith(TEXT("/")))
        {
            URL = URL.Mid(1);
        }

        Log(TEXT("URL: ") + URL);

        if (URL.IsEmpty())
        {
            // Serve the default HTML file
            FString HtmlContent = ReadHtmlFile();
            HtmlContent.ReplaceInline(TEXT("INITIAL_PASSCODE"), *InitialPasscode);
            HtmlContent.ReplaceInline(TEXT("USE_FACE_SCAN"), UseFaceScan ? TEXT("true") : TEXT("false"));
            HtmlContent.ReplaceInline(TEXT("FAIL_FACE_SCAN"), FailFaceScan ? TEXT("true") : TEXT("false"));
            HtmlContent.ReplaceInline(TEXT("PRIVATELY_URL"), *PrivatelyURL);

            FString HttpResponse = TEXT("HTTP/1.1 200 OK\r\n");
            HttpResponse += TEXT("Content-Type: text/html\r\n");
            HttpResponse += TEXT("Content-Length: ") + FString::FromInt(HtmlContent.Len()) + TEXT("\r\n");
            HttpResponse += TEXT("\r\n");
            HttpResponse += HtmlContent;

            SendResponse(HttpResponse);
        }
        else if (URL.StartsWith(TEXT("auth")))
        {
             // Handle the /auth request
            TArray<FString> Params;
            int index = 0;
            if (URL.FindChar('?', index))
            {
                URL = URL.RightChop(index + 1);
            }            
            URL.ParseIntoArray(Params, TEXT("&"));

            FString InitialCode, FutureCode;

            for (const FString& Param : Params)
            {
                Log(TEXT("auth:") + Param);
                if (Param.StartsWith(TEXT("initial=")))
                {
                    InitialCode = Param.Mid(8);
                }
                else if (Param.StartsWith(TEXT("future=")))
                {
                    FutureCode = Param.Mid(7);
                }
            }

            if (InitialCode == InitialPasscode)
            {
                Log(TEXT("Passcode matched"));
                StoredFuturePasscode = FutureCode;
                FString Response = TEXT("HTTP/1.1 200 OK\r\n");
                Response += TEXT("Content-Type: text/html\r\n");
                Response += TEXT("Content-Length: 2\r\n\r\n");
                Response += TEXT("{}");
                Response += TEXT("\r\n");
                SendResponse(Response);
            }
            else
            {
                Log(TEXT("Passcode didn't match"));
                FString Response = TEXT("HTTP/1.1 401 Unauthorized\r\n");
                Response += TEXT("Content-Type: text/html\r\n");
                Response += TEXT("Content-Length: 0\r\n");
                Response += TEXT("\r\n\r\n");
                SendResponse(Response);
            }
        }
        else if (URL.StartsWith(TEXT("validate-age")))
        {
            int index = 0;
            if (URL.FindChar('?', index))
            {
                URL = URL.RightChop(index + 1);
            }            
            TArray<FString> Params;
            URL.ParseIntoArray(Params, TEXT("&"));

            FString MinAge, MaxAge, Passcode, Status;

            for (const FString& Param : Params)
            {
                Log(TEXT("validate-age: ") + Param);
                if (Param.StartsWith(TEXT("minAge=")))
                {
                    MinAge = Param.Mid(7);
                }
                else if (Param.StartsWith(TEXT("maxAge=")))
                {
                    MaxAge = Param.Mid(7);
                }
                else if (Param.StartsWith(TEXT("status=")))
                {
                    Status = Param.Mid(7);
                }
                else if (Param.StartsWith(TEXT("passcode=")))
                {
                    Passcode = Param.Mid(9);
                }
            }

            bool bIsValid = Passcode == StoredFuturePasscode;

            if (bIsValid && !bPasscodeUsed)
            {
                bPasscodeUsed = true;
                if (AgeValidationCallback)
                {
                    Log(TEXT("Passcode matched = ") + Passcode);
                    if (Status == TEXT("AGE_CHECK_COMPLETE"))
                    {
                        // Invoke on the main thread
                        Log(TEXT("minAge = ") + MinAge);
                        Log(TEXT("maxAge = ") + MaxAge);
                        int32 Min = FCString::Atoi(*MinAge);
                        int32 Max = FCString::Atoi(*MaxAge);
                        AsyncTask(ENamedThreads::GameThread, [this, Min, Max]()
                        {
                            AgeValidationCallback(true, Min, Max);
                        });
                    }
                    else
                    {
                        AsyncTask(ENamedThreads::GameThread, [this]()
                        {
                            AgeValidationCallback(false, 0, 0);
                        });
                    }
                }
                FString Response = TEXT("HTTP/1.1 200 OK\r\n");
                Response += TEXT("Content-Type: text/html\r\n");
                Response += TEXT("Content-Length: 2\r\n");
                Response += TEXT("\r\n{}\r\n");
                SendResponse(Response);
            }
            else
            {
                FString Response = TEXT("HTTP/1.1 401 Unauthorized\r\n");
                SendResponse(Response);
                Response += TEXT("Content-Type: text/html\r\n");
                Response += TEXT("Content-Length: 0\r\n");
                Response += TEXT("\r\n\r\n");
             }
        }
        else
        {
            // Respond with 404 Not Found for any other requests
            FString Response = TEXT("HTTP/1.1 404 Not Found\r\n");
            Response += TEXT("Content-Type: text/html\r\n");
            Response += TEXT("Content-Length: 0\r\n");
            Response += TEXT("\r\n\r\n");
            SendResponse(Response);
        }
    }
    else
    {
        FString Response = TEXT("HTTP/1.1 400 Bad Request\r\n");
        Response += TEXT("Content-Type: text/html\r\n");
        Response += TEXT("Content-Length: 0\r\n");
        Response += TEXT("\r\n\r\n");
        SendResponse(Response);
    }
}

FString UFPrivatelyHttpServer::ReadHtmlFile()
{
    FString FilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("privately.html"));
    FString FileContent;
    FFileHelper::LoadFileToString(FileContent, *FilePath);
    return FileContent;
}

int32 UFPrivatelyHttpServer::GenerateRandomCode()
{
    return FMath::RandRange(100000, 999999);
}

bool UFPrivatelyHttpServer::IsBrowserSupported()
{
    FString DefaultBrowser = GetDefaultBrowser(TEXT("https://google.com"));
    Log(TEXT("Default browser: ") + DefaultBrowser);
    return DefaultBrowser.Contains(TEXT("Google Chrome")) || 
            DefaultBrowser.Contains(TEXT("Safari")) || 
            DefaultBrowser.Contains(TEXT("Firefox"));
}

void UFPrivatelyHttpServer::Log(const FString& Message)
{
    if (LogToFile) 
    {
        FString LogFilePath = FPaths::Combine(FPaths::ProjectDir(), TEXT("UFPrivatelyHttpServer.log"));
        FString FinalMessage = FDateTime::Now().ToString() + TEXT(" - ") + Message + LINE_TERMINATOR;
        FFileHelper::SaveStringToFile(FinalMessage, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_Append);
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
    }
}

void UFPrivatelyHttpServer::OnPIEEnd(bool bIsSimulating)
{
    Log(TEXT("PIE ended"));
    StopServer();
}

// FServerRunnable Implementation

UFPrivatelyHttpServer::FServerRunnable::FServerRunnable(UFPrivatelyHttpServer* InServer)
    : Server(InServer)
{
}

uint32 UFPrivatelyHttpServer::FServerRunnable::Run()
{
    while (!Server->bStopThread)
    {
        // Accept new connections
        FSocket* Socket = Server->ListenerSocket->Accept(TEXT("HTTPConnection"));
        if (Socket)
        {
            Server->Log(TEXT("Accepted new connection"));
            // Handle the client connection
            Server->ClientSocket = Socket;
            Server->HandleClient();
        }
        else
        {
            // Sleep for a short period to avoid busy waiting
            FPlatformProcess::Sleep(0.01f);
        }
    }
    Server->Log(TEXT("Exiting thread: "));

    return 0;
}
