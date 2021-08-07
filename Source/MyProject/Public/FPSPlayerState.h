// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "FPSPlayerState.generated.h"


class UFPSAbilitySystemComponent;

/**
 * 
 */
UCLASS()
class MYPROJECT_API AFPSPlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UPlayerAttributeSet* PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UWeaponAttributeSet* AmmoAttributeSet;

public:

	AFPSPlayerState();

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Assists)
	int TeamIndex;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Points)
	float Points;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Kills)
	int Kills;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Deaths)
	int Deaths;

	UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Assists)
	int Assists;

	// Implement IAbilitySystemInterface
	virtual class UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	class UPlayerAttributeSet* GetAttributeSetBase() const;

	class UWeaponAttributeSet* GetAmmoAttributeSet() const;

	UFUNCTION(BlueprintCallable)
	void BindDelegates();


	


protected:
	FGameplayTag DeadTag;

	// Attribute changed delegate handles
	FDelegateHandle HealthChangedDelegateHandle;

	UFUNCTION()
	void OnRep_TeamIndex();
	UFUNCTION()
	void OnRep_Points();
	UFUNCTION()
	void OnRep_Kills();
	UFUNCTION()
	void OnRep_Deaths();
	UFUNCTION()
	void OnRep_Assists();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	// Attribute changed callbacks
	virtual void HealthChanged(const FOnAttributeChangeData& Data);
};
