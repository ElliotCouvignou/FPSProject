// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerStart.h"
#include "MyPlayerStart.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API AMyPlayerStart : public APlayerStart
{
	GENERATED_BODY()

public:



	/* return spawning point value at this point in time */
	UFUNCTION(BlueprintCallable)
	float GetCurrentSpawnPoints();

protected:



	float TestArea = 30*52.5;
	
	float LOSBasePoints = 50.f;
	float DistanceBasePoints = 10.f;
	
	float PointDistanceCoeff = 10.f;
	float PointDistanceLOSCoeff = 10.f;
};
