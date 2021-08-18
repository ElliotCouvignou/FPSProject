// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "MyAbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API UMyAbilitySystemGlobals : public UAbilitySystemGlobals
{
	GENERATED_BODY()


public:
	UMyAbilitySystemGlobals();

	/**
	* Cache commonly used tags here. This has the benefit of one place to set the tag FName in case tag names change and
	* the function call into UGSAbilitySystemGlobals::GSGet() is cheaper than calling FGameplayTag::RequestGameplayTag().
	* Classes can access them by UGSAbilitySystemGlobals::GSGet().DeadTag
	* We're not using this in this sample project (classes are manually caching in their constructors), but it's here as a reference.
	*/

	UPROPERTY()
	FGameplayTag DeadTag;

	//UPROPERTY()
	//FGameplayTag InteractingTag;


	static UMyAbilitySystemGlobals& MyFPSGet()
	{
		return dynamic_cast<UMyAbilitySystemGlobals&>(Get());
	}

	/** Should allocate a project specific GameplayEffectContext struct. Caller is responsible for deallocation */
	virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;

	virtual void InitGlobalTags() override;
};
