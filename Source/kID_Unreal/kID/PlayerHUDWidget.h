#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PlayerHUDWidget.generated.h"

UCLASS()
class UPlayerHUDWidget: public UUserWidget
{
    GENERATED_BODY()

public:
    void SetText(const FString& text);

protected:
    virtual void NativeConstruct() override;

    UPROPERTY(meta = (BindWidget))
    class UTextBlock* TextBlock;
};