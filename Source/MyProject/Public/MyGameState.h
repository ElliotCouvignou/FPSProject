// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameState.h"
#include "MyGameState.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API AMyGameState : public AGameState
{
	GENERATED_BODY()


public:
	AMyGameState();

	/* this isn't updated ever second, more of what the initial match time is*/
	UPROPERTY(BlueprintReadWrite, Replicated, EditAnywhere)
	float TotalMatchtime_s = 600.f;

	UPROPERTY(BlueprintReadWrite, Replicated, EditAnywhere)
	float ServerTimeWhenMatchStarted;


	
	//UPROPERTY(BlueprintReadWrite, EditAnywhere)
	//float CurrentMatchtime_s = 600.f;



	virtual float GetServerWorldTimeSeconds() const override;
};
