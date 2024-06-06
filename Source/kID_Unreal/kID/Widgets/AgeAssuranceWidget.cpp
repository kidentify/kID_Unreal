#include "AgeAssuranceWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Privately/PrivatelyHTTPServer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"

void UAgeAssuranceWidget::InitializeWidget(int32 InAge, TFunction<void(bool, int32, int32)> InCallback)
{
    Age = InAge;
    if (VerificationText)
    {
        FText VerificationTextContent = VerificationText->GetText();
        FString VerificationString = VerificationTextContent.ToString();
        VerificationString = VerificationString.Replace(TEXT("X"), *FString::FromInt(Age));
        VerificationText->SetText(FText::FromString(VerificationString));
    }

    YesButton->OnClicked.AddDynamic(this, &UAgeAssuranceWidget::OnYesClicked);   
    NoButton->OnClicked.AddDynamic(this, &UAgeAssuranceWidget::OnNoClicked);
    CancelButton->OnClicked.AddDynamic(this, &UAgeAssuranceWidget::OnNoClicked);

    Callback = InCallback;

    // Start the HTTP server and open the URL in the external browser
    StartHttpServer();
}

// This function is called when the "I am" button is clicked, which 
// is for testing purposes.  In a real world scenario, the user
// would need to pass a verification process to confirm their age.
void UAgeAssuranceWidget::OnYesClicked()
{
    StopHttpServer();
    Callback(true, Age, Age);
    RemoveFromParent();
}

void UAgeAssuranceWidget::OnNoClicked()
{
    StopHttpServer();
    Callback(false, 0, 0);
    RemoveFromParent();
}

void UAgeAssuranceWidget::StartHttpServer()
{
    // Create and start the HTTP server
    HttpServer = NewObject<UFPrivatelyHttpServer>();
    HttpServer->Initialize(Callback);

    if (HttpServer->StartServer(8080))
    {
        // Open the URL in the external browser
        FString URL = TEXT("http://localhost:8080/");
        FPlatformProcess::LaunchURL(*URL, nullptr, nullptr);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("Failed to start the HTTP server"));
    }
}

void UAgeAssuranceWidget::StopHttpServer()
{
    if (HttpServer)
    {
        HttpServer->StopServer();
        HttpServer = nullptr;
    }
}