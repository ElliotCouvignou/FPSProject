// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/MyAbilitySystemGlobals.h"
#include "AbilitySystem/MyGameplayEffectTypes.h"


UMyAbilitySystemGlobals::UMyAbilitySystemGlobals()
{

}

FGameplayEffectContext* UMyAbilitySystemGlobals::AllocGameplayEffectContext() const
{
	return new FFPSGameplayEffectContext();
}

void UMyAbilitySystemGlobals::InitGlobalTags()
{
	Super::InitGlobalTags();

	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	//KnockedDownTag = FGameplayTag::RequestGameplayTag("State.KnockedDown");
	//InteractingTag = FGameplayTag::RequestGameplayTag("State.Interacting");
	//InteractingRemovalTag = FGameplayTag::RequestGameplayTag("State.InteractingRemoval");
}