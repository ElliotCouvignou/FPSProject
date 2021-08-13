// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "SettingsSave.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API USettingsSave : public USaveGame
{
	GENERATED_BODY()

public:

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float FOV = 90.f;


	
	UPROPERTY(BlueprintReadWrite, Category = "Controls")
	float MouseSensitivity = 1.0f;
	


	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Master = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Effects = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Music = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Ambient = 0.5f;
	
};
