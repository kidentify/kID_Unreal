#include "FloatingChallengeWidget.h"
#include "Components/EditableTextBox.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"
#include "Components/Button.h"
#include "KidWorkflow.h"
#include "QR-Code-generator/qrcodegen.hpp"

void UFloatingChallengeWidget::InitializeWidget(UKidWorkflow* InMyKidWorkflow, 
                    const FString& InOTP, const FString& InQRCodeUrl, TFunction<void(const FString&)> InOnEmailSubmitted)
{
    OTP = InOTP;
    QRCodeUrl = InQRCodeUrl;
    OnEmailSubmitted = InOnEmailSubmitted;

    KidWorkflow = InMyKidWorkflow;

    // Set the OTP text
    OTPTextBlock->SetText(FText::FromString(OTP));

    // Generate QR code texture
    UTexture2D* QRCodeTexture = GenerateQRCodeTexture();

    if (QRCodeTexture)
    {
        QRCodeImage->SetBrushFromTexture(QRCodeTexture);
    }

    SubmitButton->OnClicked.AddDynamic(this, &UFloatingChallengeWidget::HandleEmailSubmitted);
    CancelButton->OnClicked.AddDynamic(this, &UFloatingChallengeWidget::OnCancelClicked);

}

UTexture2D* UFloatingChallengeWidget::GenerateQRCodeTexture()
{
    const int32 TextureSize = 250; // 250x250 texture size
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(TCHAR_TO_UTF8(*QRCodeUrl), qrcodegen::QrCode::Ecc::LOW);
    int qrSize = qr.getSize();

    TArray<FColor> Pixels;
    Pixels.SetNum(TextureSize * TextureSize);

    // Fill the texture with white color
    for (int y = 0; y < TextureSize; ++y)
    {
        for (int x = 0; x < TextureSize; ++x)
        {
            Pixels[y * TextureSize + x] = FColor::White;
        }
    }

    // Calculate the scale to fit the QR code within the 250x250 texture
    float Scale = static_cast<float>(TextureSize) / static_cast<float>(qrSize);

    // Draw the QR code in the center of the 250x250 texture
    for (int y = 0; y < qrSize; ++y)
    {
        for (int x = 0; x < qrSize; ++x)
        {
            bool isBlack = qr.getModule(x, y);
            FColor PixelColor = isBlack ? FColor::Black : FColor::White;

            int32 StartX = static_cast<int32>(x * Scale);
            int32 StartY = static_cast<int32>(y * Scale);
            int32 EndX = static_cast<int32>((x + 1) * Scale);
            int32 EndY = static_cast<int32>((y + 1) * Scale);

            for (int py = StartY; py < EndY; ++py)
            {
                for (int px = StartX; px < EndX; ++px)
                {
                    Pixels[py * TextureSize + px] = PixelColor;
                }
            }
        }
    }

    UTexture2D* Texture = UTexture2D::CreateTransient(TextureSize, TextureSize, PF_B8G8R8A8);
    if (!Texture)
    {
        return nullptr;
    }

    // Lock the texture and copy the pixel data
    void* TextureData = Texture->GetPlatformData()->Mips[0].BulkData.Lock(LOCK_READ_WRITE);
    FMemory::Memcpy(TextureData, Pixels.GetData(), Pixels.Num() * sizeof(FColor));
    Texture->GetPlatformData()->Mips[0].BulkData.Unlock();

    // Update the texture resource
    Texture->UpdateResource();

    return Texture;
}

void UFloatingChallengeWidget::HandleEmailSubmitted()
{
    if (OnEmailSubmitted)
    {
        OnEmailSubmitted(EmailTextBox->GetText().ToString());
    }
}

void UFloatingChallengeWidget::OnCancelClicked()
{
    if (KidWorkflow) 
    {
        KidWorkflow->HandleProhibitedStatus();
    }
}
