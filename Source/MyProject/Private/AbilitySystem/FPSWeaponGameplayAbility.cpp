// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/FPSWeaponGameplayAbility.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)

void UGSTargetType::GetTargets_Implementation(AMyProjectCharacter* TargetingCharacter, AActor* TargetingActor, FGameplayEventData EventData, TArray<FGameplayAbilityTargetDataHandle>& OutTargetData, TArray<FHitResult>& OutHitResults, TArray<AActor*>& OutActors) const
{
	return;
}


bool FFPSGameplayEffectContainerSpec::HasValidEffects() const
{
	return TargetGameplayEffectSpecs.Num() > 0;
}

bool FFPSGameplayEffectContainerSpec::HasValidTargets() const
{
	return TargetData.Num() > 0;
}

void FFPSGameplayEffectContainerSpec::AddTargets(const TArray<FGameplayAbilityTargetDataHandle>& InTargetData, const TArray<FHitResult>& HitResults, const TArray<AActor*>& TargetActors)
{
	for (const FGameplayAbilityTargetDataHandle& TD : InTargetData)
	{
		TargetData.Append(TD);
	}

	for (const FHitResult& HitResult : HitResults)
	{
		FGameplayAbilityTargetData_SingleTargetHit* NewData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
		TargetData.Add(NewData);
	}

	if (TargetActors.Num() > 0)
	{
		FGameplayAbilityTargetData_ActorArray* NewData = new FGameplayAbilityTargetData_ActorArray();
		NewData->TargetActorArray.Append(TargetActors);
		TargetData.Add(NewData);
	}
}

void FFPSGameplayEffectContainerSpec::ClearTargets()
{
	TargetData.Clear();
}


UFPSWeaponGameplayAbility::UFPSWeaponGameplayAbility()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	
	ActivationBlockedTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
}




FGameplayAbilityTargetDataHandle UFPSWeaponGameplayAbility::MakeGameplayAbilityTargetDataHandleFromHitResults(
	const TArray<FHitResult> HitResults)
{
	FGameplayAbilityTargetDataHandle TargetData;

	for (const FHitResult& HitResult : HitResults)
	{
		FGameplayAbilityTargetData_SingleTargetHit* NewData = new FGameplayAbilityTargetData_SingleTargetHit(HitResult);
		TargetData.Add(NewData);
	}

	return TargetData;
}

void UFPSWeaponGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilitySpec& Spec)
{
	Super::OnAvatarSet(ActorInfo, Spec);

	if (bActivateAbilityOnGranted)
	{
		bool ActivatedAbility = ActorInfo->AbilitySystemComponent->TryActivateAbility(Spec.Handle, false);
	}
}

bool UFPSWeaponGameplayAbility::BatchRPCTryActivateAbility(FGameplayAbilitySpecHandle InAbilityHandle,
                                                           bool EndAbilityImmediately)
{
	UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
	if (ASC)
	{
		return ASC->BatchRPCTryActivateAbility(InAbilityHandle, EndAbilityImmediately);
	}
	
	return false;
}

void UFPSWeaponGameplayAbility::ExternalEndAbility()
{
	check(CurrentActorInfo);

	bool bReplicateEndAbility = true;
	bool bWasCancelled = false;
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, bReplicateEndAbility, bWasCancelled);
}

UObject* UFPSWeaponGameplayAbility::K2_GetSourceObject(FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo& ActorInfo) const
{
	return GetSourceObject(Handle, &ActorInfo);
}

FFPSGameplayEffectContainerSpec UFPSWeaponGameplayAbility::MakeEffectContainerSpec(FGameplayTag ContainerTag,
	const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	FFPSGameplayEffectContainer* FoundContainer = EffectContainerMap.Find(ContainerTag);

	if (FoundContainer)
	{
		return MakeEffectContainerSpecFromContainer(*FoundContainer, EventData, OverrideGameplayLevel);
	}
	return FFPSGameplayEffectContainerSpec();
}

FFPSGameplayEffectContainerSpec UFPSWeaponGameplayAbility::MakeEffectContainerSpecFromContainer(
	const FFPSGameplayEffectContainer& Container, const FGameplayEventData& EventData, int32 OverrideGameplayLevel)
{
	// First figure out our actor info
    	FFPSGameplayEffectContainerSpec ReturnSpec;
    	AActor* OwningActor = GetOwningActorFromActorInfo();
    	AActor* AvatarActor = GetAvatarActorFromActorInfo();
    	AMyProjectCharacter* AvatarCharacter = Cast<AMyProjectCharacter>(AvatarActor);
    	UFPSAbilitySystemComponent* OwningASC = UFPSAbilitySystemComponent::GetAbilitySystemComponentFromActor(OwningActor);
    
    	if (OwningASC)
    	{
    		// If we have a target type, run the targeting logic. This is optional, targets can be added later
    		if (Container.TargetType.Get())
    		{
    			TArray<FHitResult> HitResults;
    			TArray<AActor*> TargetActors;
    			TArray<FGameplayAbilityTargetDataHandle> TargetData;
    			const UGSTargetType* TargetTypeCDO = Container.TargetType.GetDefaultObject();
    			TargetTypeCDO->GetTargets(AvatarCharacter, AvatarActor, EventData, TargetData, HitResults, TargetActors);
    			ReturnSpec.AddTargets(TargetData, HitResults, TargetActors);
    		}
    
    		// If we don't have an override level, use the ability level
    		if (OverrideGameplayLevel == INDEX_NONE)
    		{
    			//OverrideGameplayLevel = OwningASC->GetDefaultAbilityLevel();
    			OverrideGameplayLevel = GetAbilityLevel();
    		}
    
    		// Build GameplayEffectSpecs for each applied effect
    		for (const TSubclassOf<UGameplayEffect>& EffectClass : Container.TargetGameplayEffectClasses)
    		{
    			ReturnSpec.TargetGameplayEffectSpecs.Add(MakeOutgoingGameplayEffectSpec(EffectClass, OverrideGameplayLevel));
    		}
    	}
    	return ReturnSpec;
}

TArray<FActiveGameplayEffectHandle> UFPSWeaponGameplayAbility::ApplyEffectContainerSpec(
	const FFPSGameplayEffectContainerSpec& ContainerSpec)
{
	TArray<FActiveGameplayEffectHandle> AllEffects;

	// Iterate list of effect specs and apply them to their target data
	for (const FGameplayEffectSpecHandle& SpecHandle : ContainerSpec.TargetGameplayEffectSpecs)
	{
		AllEffects.Append(K2_ApplyGameplayEffectSpecToTarget(SpecHandle, ContainerSpec.TargetData));
	}
	return AllEffects;
}

bool UFPSWeaponGameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle,
                                          const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags) const
{
	return Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags) && BPCheckCost(Handle, *ActorInfo);
}

bool UFPSWeaponGameplayAbility::BPCheckCost_Implementation(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo& ActorInfo) const
{
	return true;
}

void UFPSWeaponGameplayAbility::ApplyCost(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) const
{
	BPApplyCost(Handle, *ActorInfo, ActivationInfo);
	Super::ApplyCost(Handle, ActorInfo, ActivationInfo);
}

void UFPSWeaponGameplayAbility::SetCurrentMontageForMesh(USkeletalMeshComponent* InMesh, UAnimMontage* InCurrentMontage)
{
	ensure(IsInstantiated());

	FAbilityMeshMontage AbilityMeshMontage;
	if (FindAbillityMeshMontage(InMesh, AbilityMeshMontage))
	{
		AbilityMeshMontage.Montage = InCurrentMontage;
	}
	else
	{
		CurrentAbilityMeshMontages.Add(FAbilityMeshMontage(InMesh, InCurrentMontage));
	}
}

void UFPSWeaponGameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	// Force toggle input behavior flags to true (first input pressed)
	UFPSAbilitySystemComponent* OwningASC = UFPSAbilitySystemComponent::GetAbilitySystemComponentFromActor(GetOwningActorFromActorInfo());
	if(OwningASC && ActorInfo->IsLocallyControlled())
	{
		OwningASC->SetToggleInputIdFlag(OwningASC->FindAbilitySpecFromHandle(Handle)->InputID, false);
	}
	
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
}

void UFPSWeaponGameplayAbility::CancelAbility(const FGameplayAbilitySpecHandle Handle,
                                              const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
                                              bool bReplicateCancelAbility)
{
	Super::CancelAbility(Handle, ActorInfo, ActivationInfo, bReplicateCancelAbility);

	// reset toggle input behavior flags
	// UFPSAbilitySystemComponent* OwningASC = UFPSAbilitySystemComponent::GetAbilitySystemComponentFromActor(GetOwningActorFromActorInfo());
	// if(OwningASC && ActorInfo->IsLocallyControlled())
	// {
	// 	OwningASC->SetToggleInputIdFlag(OwningASC->FindAbilitySpecFromHandle(Handle)->InputID, false);
	// }
}

bool UFPSWeaponGameplayAbility::FindAbillityMeshMontage(USkeletalMeshComponent* InMesh, FAbilityMeshMontage& InAbilityMeshMontage)
{
	for (FAbilityMeshMontage& MeshMontage : CurrentAbilityMeshMontages)
	{
		if (MeshMontage.Mesh == InMesh)
		{
			InAbilityMeshMontage = MeshMontage;
			return true;
		}
	}

	return false;
}
