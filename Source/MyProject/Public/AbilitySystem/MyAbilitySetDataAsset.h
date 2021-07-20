// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MyProject.h"
#include "GameplayAbilitySpec.h"
#include "MyAbilitySetDataAsset.generated.h"



class UAbilitySystemComponent;

/**
*	Example struct that pairs a enum input command to a GameplayAbilityClass.6
*/
USTRUCT(BlueprintType)
struct FP4GameplayAbilityBindInfo
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = BindInfo)
	EAbilityInputID	Command;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = BindInfo)
	TSubclassOf<class UGameplayAbility>	AbilityClass;

	//bool operator==(const class UGameplayAbility& lhs) const;
};

/**
 * 
 */
UCLASS()
class MYPROJECT_API UMyAbilitySetDataAsset : public UDataAsset
{
	GENERATED_UCLASS_BODY()


	UPROPERTY(EditAnywhere, Category = AbilitySet)
	TArray<FP4GameplayAbilityBindInfo>	Abilities;

	void GiveAbilities(UAbilitySystemComponent* AbilitySystemComponent) const;
		

	void AddClassesToRemove(TArray<FGameplayAbilitySpecHandle>& RemoveArray, TArray<FGameplayAbilitySpec>& AbilitiesArray);
	
};
