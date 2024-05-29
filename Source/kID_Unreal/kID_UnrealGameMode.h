// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "kID/KidWorkflow.h"
#include "kID_UnrealGameMode.generated.h"

UCLASS(minimalapi)
class AkID_UnrealGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AkID_UnrealGameMode();
protected:
    virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason);
private:
	
	UPROPERTY()
    UKidWorkflow* KidWorkflow;
};