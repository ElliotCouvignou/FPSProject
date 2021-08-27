// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "FPSAbilitySystemComponent.generated.h"


USTRUCT()
struct MYPROJECT_API FGameplayAbilityLocalAnimMontageForMesh
{
	GENERATED_BODY();

	public:
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	UPROPERTY()
	FGameplayAbilityLocalAnimMontage LocalMontageInfo;

	FGameplayAbilityLocalAnimMontageForMesh() : Mesh(nullptr), LocalMontageInfo()
	{
	}

	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh)
		: Mesh(InMesh), LocalMontageInfo()
	{
	}

	FGameplayAbilityLocalAnimMontageForMesh(USkeletalMeshComponent* InMesh, FGameplayAbilityLocalAnimMontage& InLocalMontageInfo)
		: Mesh(InMesh), LocalMontageInfo(InLocalMontageInfo)
	{
	}
};

/**
* Data about montages that is replicated to simulated clients.
*/
USTRUCT()
struct MYPROJECT_API FGameplayAbilityRepAnimMontageForMesh
{
	GENERATED_BODY();

	public:
	UPROPERTY()
	USkeletalMeshComponent* Mesh;

	UPROPERTY()
	FGameplayAbilityRepAnimMontage RepMontageInfo;

	FGameplayAbilityRepAnimMontageForMesh() : Mesh(nullptr), RepMontageInfo()
	{
	}

	FGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh)
		: Mesh(InMesh), RepMontageInfo()
	{
	}
};


/**
 * 
 */
UCLASS()
class MYPROJECT_API UFPSAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:

	bool bStartupEffectsApplied = false;


	
	// Version of function in AbilitySystemGlobals that returns correct type
	static UFPSAbilitySystemComponent* GetAbilitySystemComponentFromActor(const AActor* Actor, bool LookForComponent = false);

	UFUNCTION(BlueprintCallable, Category = "Abilities")
	void K2_CancelAbilityByClass(TSubclassOf<UGameplayAbility> AbilityClass);


	UFUNCTION(BlueprintCallable, Category = "Abilities")
	virtual bool BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle, bool EndAbilityImmediately);

	// Exposes AddLooseGameplayTag to Blueprint. This tag is *not* replicated.
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "AddLooseGameplayTag"))
	void K2_AddLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	// Exposes AddLooseGameplayTags to Blueprint. These tags are *not* replicated.
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "AddLooseGameplayTags"))
	void K2_AddLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);

	// Exposes RemoveLooseGameplayTag to Blueprint. This tag is *not* replicated.
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "RemoveLooseGameplayTag"))
	void K2_RemoveLooseGameplayTag(const FGameplayTag& GameplayTag, int32 Count = 1);

	// Exposes RemoveLooseGameplayTags to Blueprint. These tags are *not* replicated.
	UFUNCTION(BlueprintCallable, Category = "Abilities", Meta = (DisplayName = "RemoveLooseGameplayTags"))
	void K2_RemoveLooseGameplayTags(const FGameplayTagContainer& GameplayTags, int32 Count = 1);
	
	// Plays a montage and handles replication and prediction based on passed in ability/activation info
	virtual float PlayMontageForMesh(UGameplayAbility* InAnimatingAbility, class USkeletalMeshComponent* InMesh, FGameplayAbilityActivationInfo ActivationInfo, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None, bool bReplicateMontage = true);

	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

	// Plays a montage without updating replication/prediction structures. Used by simulated proxies when replication tells them to play a montage.
	virtual float PlayMontageSimulatedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* Montage, float InPlayRate, FName StartSectionName = NAME_None);
	// Stops whatever montage is currently playing. Expectation is caller should only be stopping it if they are the current animating ability (or have good reason not to check)
	virtual void CurrentMontageStopForMesh(USkeletalMeshComponent* InMesh, float OverrideBlendOutTime = -1.0f);

	// Returns the current animating ability
	UGameplayAbility* GetAnimatingAbilityFromAnyMesh();

	// Returns the montage that is playing for the mesh
	UAnimMontage* GetCurrentMontageForMesh(USkeletalMeshComponent* InMesh);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", Meta = (DisplayName = "GetTagCount", ScriptName = "GetTagCount"))
	int32 K2_GetTagCount(FGameplayTag TagToCheck) const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities", Meta = (DisplayName = "HasTag", ScriptName = "HasTag"))
	bool K2_HasTag(FGameplayTag TagToCheck) const;
	
	/* BP exposed c++ functioality */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Abilities")
	FGameplayAbilitySpecHandle FindAbilitySpecHandleForClass(TSubclassOf<UGameplayAbility> AbilityClass, UObject* OptionalSourceObject=nullptr);


protected:
	UPROPERTY()
	TArray<FGameplayAbilityLocalAnimMontageForMesh> LocalAnimMontageInfoForMeshes;

	UPROPERTY(ReplicatedUsing = OnRep_ReplicatedAnimMontageForMesh)
	TArray<FGameplayAbilityRepAnimMontageForMesh> RepAnimMontageInfoForMeshes;
	
	// Finds the existing FGameplayAbilityLocalAnimMontageForMesh for the mesh or creates one if it doesn't exist
	FGameplayAbilityLocalAnimMontageForMesh& GetLocalAnimMontageInfoForMesh(USkeletalMeshComponent* InMesh);
	// Finds the existing FGameplayAbilityRepAnimMontageForMesh for the mesh or creates one if it doesn't exist
	FGameplayAbilityRepAnimMontageForMesh& GetGameplayAbilityRepAnimMontageForMesh(USkeletalMeshComponent* InMesh);

	// Called when a prediction key that played a montage is rejected
	void OnPredictiveMontageRejectedForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* PredictiveMontage);
	
	// Copy LocalAnimMontageInfo into RepAnimMontageInfo
	void AnimMontage_UpdateReplicatedDataForMesh(USkeletalMeshComponent* InMesh);
	void AnimMontage_UpdateReplicatedDataForMesh(FGameplayAbilityRepAnimMontageForMesh& OutRepAnimMontageInfo);

	UFUNCTION()
	virtual void OnRep_ReplicatedAnimMontageForMesh();
	// Returns true if we are ready to handle replicated montage information
	virtual bool IsReadyForReplicatedMontageForMesh();

};
