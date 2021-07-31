// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "MyProjectGameMode.generated.h"


class AMyProjectCharacter;

UCLASS(minimalapi)
class AMyProjectGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	AMyProjectGameMode();


	UFUNCTION(BlueprintCallable)
	void MovePlayerToSpawnLocation(AMyProjectCharacter* Player);

	void PlayerDied(AController* Controller);

protected:
	float RespawnDelay;

	TSubclassOf<class AGSPlayerCharacter> PlayerClass;


	virtual void BeginPlay() override;

	void RespawnPlayer(AController* Controller);
};



