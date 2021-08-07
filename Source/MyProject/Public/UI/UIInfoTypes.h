// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "GameFramework/PlayerController.h"
#include "UIInfoTypes.generated.h"

class AFPSWeapon;




USTRUCT(BlueprintType)
struct FPlayerDeathInfoStruct
{
	GENERATED_USTRUCT_BODY()

	/* If false, no tooltip will be generated when hovering this ability */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString KillerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString VictimName;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	AFPSWeapon* SourceWeapon;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHeadShot = 0;

	
	FPlayerDeathInfoStruct()
	{
		SourceWeapon = nullptr;	
	}
};



/* Ability tooltip displayed to player is generated with this struct info*/
USTRUCT(BlueprintType)
struct FDamageInfoStruct
{
	GENERATED_USTRUCT_BODY()

	/* If false, no tooltip will be generated when hovering this ability */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bPlayerDied;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bHeadShot;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageDone = 0.f;
	


	
	FDamageInfoStruct()
	{
		
	}
};

/* Ability tooltip displayed to player is generated with this struct info*/
USTRUCT(BlueprintType)
struct FKillInfoStruct
{
	GENERATED_USTRUCT_BODY()

	/* If false, no tooltip will be generated when hovering this ability */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float DamageDone = 0.f;
	
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int HeadshotCount = 0;

	
	FKillInfoStruct()
	{
		
	}
};

/* Ability tooltip displayed to player is generated with this struct info*/
USTRUCT(BlueprintType)
struct FDiedInfoStruct
{
	GENERATED_USTRUCT_BODY()

	/* If false, no tooltip will be generated when hovering this ability */
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	float HealthRemaining = 0.f;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString WeaponName;
	
	FDiedInfoStruct()
	{
		
	}
};