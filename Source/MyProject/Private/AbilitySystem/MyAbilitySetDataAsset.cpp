// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MyAbilitySetDataAsset.h"
#include "AbilitySystemComponent.h"

UMyAbilitySetDataAsset::UMyAbilitySetDataAsset(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

void UMyAbilitySetDataAsset::GiveAbilities(UAbilitySystemComponent* AbilitySystemComponent) const
{
	for (const FP4GameplayAbilityBindInfo& BindInfo : Abilities)
	{
		if (BindInfo.AbilityClass)
		{
			int32 thing = (int32)BindInfo.Command;
			AbilitySystemComponent->GiveAbility(FGameplayAbilitySpec(BindInfo.AbilityClass, 1, thing));
		}
	}
}

void UMyAbilitySetDataAsset::AddClassesToRemove(TArray<FGameplayAbilitySpecHandle>& RemoveArray,
	TArray<FGameplayAbilitySpec>& AbilitiesArray)
{
	
}
