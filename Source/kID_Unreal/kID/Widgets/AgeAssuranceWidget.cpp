#include "AgeAssuranceWidget.h"
#include "Components/TextBlock.h"
#include "Components/Button.h"

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

    if (YesButton)
    {
        YesButton->OnClicked.AddDynamic(this, &UAgeAssuranceWidget::OnYesClicked);
    }
    
    if (NoButton)
    {
        NoButton->OnClicked.AddDynamic(this, &UAgeAssuranceWidget::OnNoClicked);
    }
    Callback = InCallback;
}

void UAgeAssuranceWidget::OnYesClicked()
{
    Callback(true);
    RemoveFromParent();
}

void UAgeAssuranceWidget::OnNoClicked()
{
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