// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"




UENUM(BlueprintType)
enum class EAbilityInputID : uint8
{
	// 0 None
	None			UMETA(DisplayName = "None"),  // "None" = no action binding reference, will be called in backend not users
	Confirm			UMETA(DisplayName = "ConfirmAbility"),
	Cancel			UMETA(DisplayName = "CancelAbility"),

	UseODM_R		UMETA(DisplayName = "UseODM_R"),
	UseODM_L		UMETA(DisplayName = "UseODM_L"),
	UseEquippment	UMETA(DisplayName = "UseEquippment"),

	EquipWeapon1	UMETA(DisplayName = "EquipWeapon1"),
	EquipWeapon2	UMETA(DisplayName = "EquipWeapon2"),
	EquipWeapon3	UMETA(DisplayName = "EquipWeapon3"),
	EquipWeapon4	UMETA(DisplayName = "EquipWeapon4"),
	
	Crouch			UMETA(DisplayName = "Crouch"),
	Jump			UMETA(DisplayName = "Jump"),
	Sprint			UMETA(DisplayName = "Sprint"),

	ShootGun	    UMETA(DisplayName = "ShootGun"), // LMB "Auto Attack" bind
	ADS				UMETA(DisplayName = "ADS"),
	Reload			UMETA(DisplayName = "Reload")
};