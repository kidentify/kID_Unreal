#include "AgeAssuranceWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"
#include "Privately/PrivatelyHTTPServer.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformProcess.h"

void UAgeAssuranceWidget::InitializeWidget(const FString& DateOfBirth, TFunction<void(bool)> InCallback)
{
    int32 Age = CalculateAgeFromDOB(DateOfBirth);
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

void UAgeAssuranceWidget::OnYesClicked()
{
    StopHttpServer();
    Callback(true);
    RemoveFromParent();
}

void UAgeAssuranceWidget::OnNoClicked()
{
    StopHttpServer();
    Callback(false);
    RemoveFromParent();
}

int32 UAgeAssuranceWidget::CalculateAgeFromDOB(const FString& DateOfBirth)
{
    FDateTime DOB;
    if (FDateTime::Parse(DateOfBirth, DOB))
    {
        FDateTime Now = FDateTime::UtcNow();
        int32 Age = Now.GetYear() - DOB.GetYear();
        if (Now.GetMonth() < DOB.GetMonth() || (Now.GetMonth() == DOB.GetMonth() && Now.GetDay() < DOB.GetDay()))
        {
            Age--;
        }
        return Age;
    }
    else if (FDateTime::ParseIso8601(*DateOfBirth, DOB)) // Handles YYYY-MM-DD format
    {
        FDateTime Now = FDateTime::UtcNow();
        int32 Age = Now.GetYear() - DOB.GetYear();
        if (Now.GetMonth() < DOB.GetMonth() || (Now.GetMonth() == DOB.GetMonth() && Now.GetDay() < DOB.GetDay()))
        {
            Age--;
        }
        return Age;
    }
    else
    {
        // Handle the case where only the year is provided (YYYY)
        int32 Year;
        if (LexTryParseString(Year, *DateOfBirth))
        {
            FDateTime Now = FDateTime::UtcNow();
            return Now.GetYear() - Year;
        }
    }
    return 0; // Invalid date format
}

void UAgeAssuranceWidget::StartHttpServer()
{
    // Create and start the HTTP server
    HttpServer = NewObject<UFPrivatelyHttpServer>();
    HttpServer->Initialize([this](bool bIsValid)
    {
        if (bIsValid)
        {
            UE_LOG(LogTemp, Log, TEXT("Adult validation successful"));
        }
        else
        {
            UE_LOG(LogTemp, Log, TEXT("Adult validation failed"));
        }

        // Invoke the callback
        if (Callback)
        {
            Callback(bIsValid);
        }
    });

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