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
	float MouseSensitivity;
	


	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Master;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Effects;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Music;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Ambient;
	
};
