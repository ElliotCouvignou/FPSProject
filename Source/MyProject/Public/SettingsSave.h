// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Materials/MaterialFunctionInstance.h"
#include "SettingsSave.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API USettingsSave : public USaveGame
{
	GENERATED_BODY()

public:

#pragma region Gameplay
		
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float FOV = 90.f;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float WeaponAndHandsFOV = 90.f;
	
	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float AssaultWeaponFOVMultiplier = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	float SniperWeaponFOVMultiplier = 1.f;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	bool bToggleGrapple = false;

	UPROPERTY(BlueprintReadWrite, Category = "Gameplay")
	bool bToggleADS = false;
	
#pragma endregion
#pragma region Controls
	UPROPERTY(BlueprintReadWrite, Category = "Controls")
	float MouseSensitivity = 1.0f;

#pragma endregion
#pragma region Audio

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Master = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Effects = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Music = 0.5f;

	UPROPERTY(BlueprintReadWrite, Category = "Audio")
	float Ambient = 0.5f;
#pragma endregion	
};
