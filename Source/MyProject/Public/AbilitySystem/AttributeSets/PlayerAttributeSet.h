// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "PlayerAttributeSet.generated.h"

// Uses macros from AttributeSet.h
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * 
 */
UCLASS()
class MYPROJECT_API UPlayerAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:

	UPlayerAttributeSet();
	
	// Current Health, when 0 we expect owner to die unless prevented by an ability. Capped by MaxHealth.
	// Positive changes can directly use this.     
	// Negative changes to Health should go through Damage meta attribute.
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_Health)
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Health)

	// MaxHealth is its own attribute since GameplayEffects may modify it
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_MaxHealth)
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxHealth)

	// Health regen rate will passively increase Health every second
	UPROPERTY(BlueprintReadOnly, Category = "Health", ReplicatedUsing = OnRep_HealthRegenRate)
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, HealthRegenRate)

	// Current stamina, used to execute special abilities. Capped by MaxStamina.
	UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_Stamina)
	FGameplayAttributeData Stamina;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Stamina)

	// MaxStamina is its own attribute since GameplayEffects may modify it
	UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_MaxStamina)
	FGameplayAttributeData MaxStamina;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxStamina)

	// Stamina regen rate will passively increase Stamina every second
	UPROPERTY(BlueprintReadOnly, Category = "Stamina", ReplicatedUsing = OnRep_StaminaRegenRate)
	FGameplayAttributeData StaminaRegenRate;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, StaminaRegenRate)


	UPROPERTY(BlueprintReadOnly, Category = "Gas", ReplicatedUsing = OnRep_GasLeft)
	FGameplayAttributeData GasLeft;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, GasLeft)

	UPROPERTY(BlueprintReadOnly, Category = "Gas", ReplicatedUsing = OnRep_GasRight)
	FGameplayAttributeData GasRight;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, GasRight)
	
	UPROPERTY(BlueprintReadOnly, Category = "Gas", ReplicatedUsing = OnRep_MaxGas)
	FGameplayAttributeData MaxGas;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MaxGas)
	
	UPROPERTY(BlueprintReadOnly, Category = "Gas", ReplicatedUsing = OnRep_GasRegenRate)
	FGameplayAttributeData GasRegenRate;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, GasRegenRate)

	// Damage is a meta attribute used by the DamageExecution to calculate final damage, which then turns into -Health
	// Temporary value that only exists on the Server. Not replicated.
	UPROPERTY(BlueprintReadOnly, Category = "Damage")
	FGameplayAttributeData Damage;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, Damage)

	// MoveSpeed affects how fast characters can move.
	UPROPERTY(BlueprintReadOnly, Category = "MoveSpeed", ReplicatedUsing = OnRep_MoveSpeed)
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, MoveSpeed)

	UPROPERTY(BlueprintReadOnly, Category = "Character Level", ReplicatedUsing = OnRep_CharacterLevel)
	FGameplayAttributeData CharacterLevel;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, CharacterLevel)

	// Experience points gained from killing enemies. Used to level up (not implemented in this project).
	UPROPERTY(BlueprintReadOnly, Category = "XP", ReplicatedUsing = OnRep_XP)
	FGameplayAttributeData XP;
	ATTRIBUTE_ACCESSORS(UPlayerAttributeSet, XP)

	virtual void PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const override;
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;
	virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data) override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	FGameplayTag HeadShotTag;

	// Helper function to proportionally adjust the value of an attribute when it's associated max attribute changes.
	// (i.e. When MaxHealth increases, Health increases by an amount that maintains the same percentage as before)
	void AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty);

	/**
	* These OnRep functions exist to make sure that the ability system internal representations are synchronized properly during replication
	**/

	UFUNCTION()
	virtual void OnRep_Health(const FGameplayAttributeData& OldHealth);

	UFUNCTION()
	virtual void OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth);

	UFUNCTION()
	virtual void OnRep_HealthRegenRate(const FGameplayAttributeData& OldHealthRegenRate);

	UFUNCTION()
	virtual void OnRep_Stamina(const FGameplayAttributeData& OldStamina);

	UFUNCTION()
	virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina);

	UFUNCTION()
	virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate);

	UFUNCTION()
	virtual void OnRep_GasLeft(const FGameplayAttributeData& OldGasLeft);

	UFUNCTION()
	virtual void OnRep_GasRight(const FGameplayAttributeData& OldGasRight);

	UFUNCTION()
	virtual void OnRep_MaxGas(const FGameplayAttributeData& OldMaxGas);
	
	UFUNCTION()
	virtual void OnRep_GasRegenRate(const FGameplayAttributeData& OldGasRegenRate);
	
	UFUNCTION()
	virtual void OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed);

	UFUNCTION()
	virtual void OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel);

	UFUNCTION()
	virtual void OnRep_XP(const FGameplayAttributeData& OldXP);


};