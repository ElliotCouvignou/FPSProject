// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/AttributeSets/PlayerAttributeSet.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "GameplayEffect.h"
#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"
#include "MyPlayerController.h"
#include "Actors/FPSWeapon.h"
#include "MyProjectGameMode.h"
#include "FPSPlayerState.h"
#include "GameFramework/CharacterMovementComponent.h"
//#include "Player/GSPlayerController.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


UPlayerAttributeSet::UPlayerAttributeSet()
{
	// Cache tags
	HeadShotTag = FGameplayTag::RequestGameplayTag(FName("Effect.Damage.HeadShot"));
}

void UPlayerAttributeSet::PreAttributeBaseChange(const FGameplayAttribute& Attribute, float& NewValue) const
{
	Super::PreAttributeBaseChange(Attribute, NewValue);

	if (Attribute == GetMoveSpeedAttribute())
	{
		// Cannot slow less than 150 units/s and cannot boost more than 1000 units/s
		NewValue = FMath::Clamp<float>(NewValue, 150, 1000);
	}
	else if (Attribute == GetGasLeftAttribute() || Attribute == GetGasRightAttribute()) // GetMaxGasAttribute comes from the Macros defined at the top of the header
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGas());
	}
	else if (Attribute == GetHealthAttribute()) // GetMaxGasAttribute comes from the Macros defined at the top of the header
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
}

void UPlayerAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	// This is called whenever attributes change, so for max health/mana we want to scale the current totals to match
	Super::PreAttributeChange(Attribute, NewValue);

	// If a Max value changes, adjust current to keep Current % of Current to Max
	if (Attribute == GetMaxHealthAttribute()) // GetMaxHealthAttribute comes from the Macros defined at the top of the header
	{
		AdjustAttributeForMaxChange(Health, MaxHealth, NewValue, GetHealthAttribute());
	}
	
	else if (Attribute.AttributeName == GetMoveSpeedAttribute().AttributeName) {
		UCharacterMovementComponent* CMC = Cast<UCharacterMovementComponent>(GetActorInfo()->MovementComponent);
		if (CMC)
		{
			CMC->MaxWalkSpeed = NewValue;
		}
	}
	else if (Attribute == GetMoveSpeedAttribute())
	{
		// Cannot slow less than 150 units/s and cannot boost more than 1000 units/s
		NewValue = FMath::Clamp<float>(NewValue, 150, 1000);
	}
	else if (Attribute == GetGasLeftAttribute() || Attribute == GetGasRightAttribute()) // GetMaxGasAttribute comes from the Macros defined at the top of the header
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxGas());
	}
	else if (Attribute == GetHealthAttribute()) // GetMaxGasAttribute comes from the Macros defined at the top of the header
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	
	
}

void UPlayerAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	FGameplayEffectContextHandle Context = Data.EffectSpec.GetContext();
	UAbilitySystemComponent* Source = Context.GetOriginalInstigatorAbilitySystemComponent();
	const FGameplayTagContainer& SourceTags = *Data.EffectSpec.CapturedSourceTags.GetAggregatedTags();
	FGameplayTagContainer SpecAssetTags;
	Data.EffectSpec.GetAllAssetTags(SpecAssetTags);

	// Get the Target actor, which should be our owner
	AActor* TargetActor = nullptr;
	AController* TargetController = nullptr;
	AMyProjectCharacter* TargetCharacter = nullptr;
	if (Data.Target.AbilityActorInfo.IsValid() && Data.Target.AbilityActorInfo->AvatarActor.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
		TargetController = Data.Target.AbilityActorInfo->PlayerController.Get();
		TargetCharacter = Cast<AMyProjectCharacter>(TargetActor);
	}

	// Get the Source actor
	AActor* SourceActor = nullptr;
	AController* SourceController = nullptr;
	AMyProjectCharacter* SourceCharacter = nullptr;
	AFPSWeapon* FpsWeapon = nullptr;
	if (Source && Source->AbilityActorInfo.IsValid() && Source->AbilityActorInfo->AvatarActor.IsValid())
	{
		SourceActor = Source->AbilityActorInfo->AvatarActor.Get();
		SourceController = Source->AbilityActorInfo->PlayerController.Get();
		if (SourceController == nullptr && SourceActor != nullptr)
		{
			if (APawn* Pawn = Cast<APawn>(SourceActor))
			{
				SourceController = Pawn->GetController();
			}
		}

		if(Context.GetSourceObject())
		{
			FpsWeapon = Cast<AFPSWeapon>(Context.GetSourceObject());
		}
		// Use the controller to find the source pawn
		if (SourceController)
		{
			SourceCharacter = Cast<AMyProjectCharacter>(SourceController->GetPawn());
		}
		else
		{
			SourceCharacter = Cast<AMyProjectCharacter>(SourceActor);
		}

		// Set the causer actor based on context if it's set
		if (Context.GetEffectCauser())
		{
			SourceActor = Context.GetEffectCauser();
		}
	}

	if (Data.EvaluatedData.Attribute == GetDamageAttribute())
	{
		// Store a local copy of the amount of damage done and clear the damage attribute
		const float LocalDamageDone = GetDamage();
		SetDamage(0.f);
	
		if (LocalDamageDone > 0.0f)
		{
			// If character was alive before damage is added, handle damage
			// This prevents damage being added to dead things and replaying death animations
			bool WasAlive = true;
	
			if (TargetCharacter)
			{
				WasAlive = TargetCharacter->IsAlive();
			}
	
			if (!TargetCharacter->IsAlive())
			{
				//UE_LOG(LogTemp, Warning, TEXT("%s() %s is NOT alive when receiving damage"), *FString(__FUNCTION__), *TargetCharacter->GetName());
			}
	
			// Apply the damage to shield first if it exists
			// const float OldShield = GetShield();
			// float DamageAfterShield = LocalDamageDone - OldShield;
			// if (OldShield > 0)
			// {
			// 	float NewShield = OldShield - LocalDamageDone;
			// 	SetShield(FMath::Clamp<float>(NewShield, 0.0f, GetMaxShield()));
			// }

			// Interesting mechanic here, if we do  < 1 dmg then just ignore it entirely and have no damage done
			// This is nice for guns that are at max dmg range and are doing miniscule dmg (esp present for shotguns)
			if (LocalDamageDone >= 1.f)
			{
				// Apply the health change and then clamp it
				const float NewHealth = GetHealth() - LocalDamageDone;
				SetHealth(FMath::Clamp(NewHealth, 0.0f, GetMaxHealth()));
				if (TargetCharacter && WasAlive)
				{
					// This is the log statement for damage received. Turned off for live games.
					//UE_LOG(LogTemp, Log, TEXT("%s() %s Damage Received: %f"), *FString(__FUNCTION__), *GetOwningActor()->GetName(), LocalDamageDone);
		
					// Show damage number for the Source player unless it was self damage
					// if (SourceActor != TargetActor)
					// {
					// 	AGSPlayerController* PC = Cast<AGSPlayerController>(SourceController);
					// 	if (PC)
					// 	{
					// 		FGameplayTagContainer DamageNumberTags;
					//
					// 		if (Data.EffectSpec.DynamicAssetTags.HasTag(HeadShotTag))
					// 		{
					// 			DamageNumberTags.AddTagFast(HeadShotTag);
					// 		}
					//
					// 		PC->ShowDamageNumber(LocalDamageDone, TargetCharacter, DamageNumberTags);
					// 	}
					// }

					AMyPlayerController* TargetPC = Cast<AMyPlayerController>(TargetController);
					if(TargetPC)
					{
						TargetPC->OnPlayerDamageTaken(SourceActor);
					}
		
					AMyPlayerController* SourcePC = Cast<AMyPlayerController>(SourceController);
					if (!TargetCharacter->IsAlive() && SourcePC)
					{
						// TODO: figure out how to fill dmg and headshot infos
						FKillInfoStruct KillInfoStruct;
						KillInfoStruct.PlayerName = TargetCharacter->PlayerName;
						SourcePC->OnPlayerKilled(KillInfoStruct);
						
						
						if(TargetPC && FpsWeapon)
						{
							FDiedInfoStruct InfoStruct;
							InfoStruct.PlayerName = SourceCharacter->PlayerName;
							InfoStruct.HealthRemaining = Source->GetNumericAttribute(GetHealthAttribute());
							InfoStruct.WeaponName =  FpsWeapon->WeaponName;
							TargetPC->OnPlayerDied(InfoStruct);
						}

						AMyProjectGameMode* GM = GetWorld()->GetAuthGameMode<AMyProjectGameMode>();
						if(GM)
						{
							FPlayerDeathInfoStruct Info;
							Info.bHeadShot = Context.GetHitResult()->BoneName.IsEqual("head");
							Info.SourceWeapon = FpsWeapon;
							Info.VictimName = TargetCharacter->PlayerName;
							Info.KillerName = SourceCharacter->PlayerName;	
							GM->OnPlayerDied(Info);
						}

						// Todo: streamline this method to add KDA
						// TODO: assists
						AFPSPlayerState* SourcePS = SourceCharacter->GetPlayerState<AFPSPlayerState>();
						if(SourcePS)
						{
							
							SourcePS->Points += 100.f;
							SourcePS->Kills ++;
						}

						AFPSPlayerState* TargetPS = TargetCharacter->GetPlayerState<AFPSPlayerState>();
						if(TargetPS)
						{
							
							TargetPS->Deaths++;
						}
						
						//print(FString("Dead lollll"));
						
						// TargetCharacter was alive before this damage and now is not alive, give XP and Gold bounties to Source.
						// Don't give bounty to self.
						// if (SourceController != TargetController)
						// {
						// 	// Create a dynamic instant Gameplay Effect to give the bounties
						// 	UGameplayEffect* GEBounty = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Bounty")));
						// 	GEBounty->DurationPolicy = EGameplayEffectDurationType::Instant;
						//
						// 	int32 Idx = GEBounty->Modifiers.Num();
						// 	GEBounty->Modifiers.SetNum(Idx + 2);
						//
						// 	FGameplayModifierInfo& InfoXP = GEBounty->Modifiers[Idx];
						// 	InfoXP.ModifierMagnitude = FScalableFloat(GetXPBounty());
						// 	InfoXP.ModifierOp = EGameplayModOp::Additive;
						// 	InfoXP.Attribute = UPlayerAttributeSet::GetXPAttribute();
						//
						// 	FGameplayModifierInfo& InfoGold = GEBounty->Modifiers[Idx + 1];
						// 	InfoGold.ModifierMagnitude = FScalableFloat(GetGoldBounty());
						// 	InfoGold.ModifierOp = EGameplayModOp::Additive;
						// 	InfoGold.Attribute = UPlayerAttributeSet::GetGoldAttribute();
						//
						// 	Source->ApplyGameplayEffectToSelf(GEBounty, 1.0f, Source->MakeEffectContext());
						// }
					}

					if(SourcePC)
					{
						FDamageInfoStruct DamageInfoStruct;

						// TODO: replace this with getter for head name
						DamageInfoStruct.bHeadShot = Context.GetHitResult()->BoneName.IsEqual("head");
						DamageInfoStruct.bPlayerDied = !TargetCharacter->IsAlive();
						DamageInfoStruct.DamageDone = LocalDamageDone;
						SourcePC->OnPlayerDamageDone(DamageInfoStruct);
					}
				}
			}
		}
	}// Damage
	else if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// Handle other health changes.
		// Health loss should go through Damage.
		SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	} // Health
	else if (Data.EvaluatedData.Attribute == GetStaminaAttribute())
	{
		// Handle stamina changes.
		SetStamina(FMath::Clamp(GetStamina(), 0.0f, GetMaxStamina()));
	}
}

void UPlayerAttributeSet::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, Health, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MaxHealth, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, HealthRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, Stamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MaxStamina, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, StaminaRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, GasLeft, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, GasRight, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MaxGas, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, GasRegenRate, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, MoveSpeed, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, CharacterLevel, COND_None, REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(UPlayerAttributeSet, XP, COND_None, REPNOTIFY_Always);
}

void UPlayerAttributeSet::AdjustAttributeForMaxChange(FGameplayAttributeData& AffectedAttribute, const FGameplayAttributeData& MaxAttribute, float NewMaxValue, const FGameplayAttribute& AffectedAttributeProperty)
{
	UAbilitySystemComponent* AbilityComp = GetOwningAbilitySystemComponent();
	const float CurrentMaxValue = MaxAttribute.GetCurrentValue();
	if (!FMath::IsNearlyEqual(CurrentMaxValue, NewMaxValue) && AbilityComp)
	{
		// Change current value to maintain the current Val / Max percent
		const float CurrentValue = AffectedAttribute.GetCurrentValue();
		float NewDelta = (CurrentMaxValue > 0.f) ? (CurrentValue * NewMaxValue / CurrentMaxValue) - CurrentValue : NewMaxValue;

		AbilityComp->ApplyModToAttributeUnsafe(AffectedAttributeProperty, EGameplayModOp::Additive, NewDelta);
	}
}

void UPlayerAttributeSet::OnRep_Health(const FGameplayAttributeData& OldHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, Health, OldHealth);
}

void UPlayerAttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldMaxHealth)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxHealth, OldMaxHealth);
}

void UPlayerAttributeSet::OnRep_HealthRegenRate(const FGameplayAttributeData& OldHealthRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, HealthRegenRate, OldHealthRegenRate);
}

void UPlayerAttributeSet::OnRep_Stamina(const FGameplayAttributeData& OldStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, Stamina, OldStamina);
}

void UPlayerAttributeSet::OnRep_MaxStamina(const FGameplayAttributeData& OldMaxStamina)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxStamina, OldMaxStamina);
}

void UPlayerAttributeSet::OnRep_StaminaRegenRate(const FGameplayAttributeData& OldStaminaRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, StaminaRegenRate, OldStaminaRegenRate);
}

void UPlayerAttributeSet::OnRep_GasLeft(const FGameplayAttributeData& OldGasLeft)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, GasLeft, OldGasLeft);
}

void UPlayerAttributeSet::OnRep_GasRight(const FGameplayAttributeData& OldGasRight)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, GasRight, OldGasRight);
}

void UPlayerAttributeSet::OnRep_MaxGas(const FGameplayAttributeData& OldMaxGas)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MaxGas, OldMaxGas);
}

void UPlayerAttributeSet::OnRep_GasRegenRate(const FGameplayAttributeData& OldGasRegenRate)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, GasRegenRate, OldGasRegenRate);
}

void UPlayerAttributeSet::OnRep_MoveSpeed(const FGameplayAttributeData& OldMoveSpeed)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, MoveSpeed, OldMoveSpeed);
}

void UPlayerAttributeSet::OnRep_CharacterLevel(const FGameplayAttributeData& OldCharacterLevel)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, CharacterLevel, OldCharacterLevel);
}

void UPlayerAttributeSet::OnRep_XP(const FGameplayAttributeData& OldXP)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(UPlayerAttributeSet, XP, OldXP);
}

