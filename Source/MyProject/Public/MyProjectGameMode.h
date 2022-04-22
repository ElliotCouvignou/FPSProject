// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "UI/UIInfoTypes.h"
#include "MyProjectGameMode.generated.h"


class AMyPlayerController;
class AMyProjectCharacter;
class AFPSWeapon;


UCLASS(minimalapi)
class AMyProjectGameMode : public AGameMode
{
	GENERATED_BODY()



	virtual void EndMatch() override;

public:
	AMyProjectGameMode();

	/* Players who are currently playing and shooting other ActivePlayers */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AMyPlayerController*> ActivePlayers;

	/* Players who joined an need to be handled for team assignment, spawns, etc. */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	TArray<AMyPlayerController*> PendingPlayers;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TSubclassOf<AMyProjectCharacter> PlayerClass;
	
	
	/* Generally unsafe and takes first found playerstart */
	UFUNCTION(BlueprintCallable)
	void MovePlayerToSpawnLocation(AMyProjectCharacter* Player);

	/* spawns the player in safe location away from enemies */
	UFUNCTION(BlueprintCallable)
	virtual void MovePlayerToSafeSpawnLocation(AMyProjectCharacter* Player);

	UFUNCTION(BlueprintCallable)
	virtual void MovePlayerToStartSpawnLocation(AMyProjectCharacter* Player);

	
	UFUNCTION(BlueprintCallable)
	void BP_EndMatch();
	
	void OnPlayerDied(FPlayerDeathInfoStruct Info);

	
	virtual void HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer) override;

protected:

	float RespawnDelay;

	float EndGameToLevelLeaveDelay = 10.f;

	FTimerHandle MatchEndedTimerHandle;
	FTimerHandle MatchTimerHandle;

	virtual void BeginPlay() override;

	/** Called when the state transitions to InProgress */
	virtual void HandleMatchHasStarted() override;

	/* Called when game is fully over and needs to be closed */
	virtual void HandleMatchExit();
	
	virtual void OnMatchTimerEnded();
	
	virtual void AssignTeamForPlayer(AMyProjectCharacter* Player);
	
	void SpawnCharacterForPlayer(AMyPlayerController* Controller);
};



