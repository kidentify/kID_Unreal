#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "UnavailableWidget.generated.h"

/**
 * 
 */
UCLASS()
class UUnavailableWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    virtual void NativeConstruct() override;

    UFUNCTION()
    void OnQuitClicked();

private:
    UPROPERTY(meta = (BindWidget))
    UButton* Quit;
};