// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

#include "AbilitySystem/FPSWeaponGameplayAbility.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MyBlueprintFunctionLibrary.generated.h"




/**
 * 
 */
UCLASS()
class MYPROJECT_API UMyBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

	
public:
	UFUNCTION(BlueprintCallable, Category = "Ability")
	static UFPSWeaponGameplayAbility* GetPrimaryAbilityInstanceFromHandle(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	static UFPSWeaponGameplayAbility* GetPrimaryAbilityInstanceFromClass(UAbilitySystemComponent* AbilitySystemComponent, TSubclassOf<UGameplayAbility> InAbilityClass);

	UFUNCTION(BlueprintCallable, Category = "Ability")
	static bool IsPrimaryAbilityInstanceActive(UAbilitySystemComponent* AbilitySystemComponent, FGameplayAbilitySpecHandle Handle);

	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Ability")
	static bool IsAbilitySpecHandleValid(FGameplayAbilitySpecHandle Handle);

	/** Interpolate rotator from Current to Target. Scaled by distance to Target, so it has a strong start speed and ease out.
	    This variant doesn't  jump to target on high framerates which is noticeable when this is every tick*/
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Math")
	static FRotator RInterpToExact( const FRotator& Current, const FRotator& Target, float DeltaTime, float InterpSpeed, float Tolerance = .0001f);
};
