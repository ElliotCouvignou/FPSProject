// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSPlayerState.h"

#include "GameplayEffectExtension.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/PlayerAttributeSet.h"
#include "AbilitySystem/AttributeSets/WeaponAttributeSet.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "MyPlayerController.h"
#include "UI/MainGameplayWidget.h"
#include "Net/UnrealNetwork.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


AFPSPlayerState::AFPSPlayerState()
{
	//AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	//AbilitySystemComponent->SetIsReplicated(true);

	// Mixed mode means we only are replicated the GEs to ourself, not the GEs to simulated proxies. If another GDPlayerState (Player) receives a GE,
	// we won't be told about it by the Server. Attributes, GameplayTags, and GameplayCues will still replicate to us.
	//AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// Create the attribute set, this replicates by default
	// Adding it as a subobject of the owning actor of an AbilitySystemComponent
	// automatically registers the AttributeSet with the AbilitySystemComponent
	//PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("PlayerAttributeSet"));

	//AmmoAttributeSet = CreateDefaultSubobject<UWeaponAttributeSet>(TEXT("AmmoAttributeSet"));

	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");

	// Set PlayerState's NetUpdateFrequency to the same as the Character.
	// Default is very low for PlayerStates and introduces perceived lag in the ability system.
	// 100 is probably way too high for a shipping game, you can adjust to fit your needs.
	NetUpdateFrequency = 100.0f;
}

void AFPSPlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AFPSPlayerState, TeamIndex);
	DOREPLIFETIME(AFPSPlayerState, Points);
	DOREPLIFETIME(AFPSPlayerState, Kills);
	DOREPLIFETIME(AFPSPlayerState, Deaths);
	DOREPLIFETIME(AFPSPlayerState, Assists);
}

UAbilitySystemComponent* AFPSPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UPlayerAttributeSet* AFPSPlayerState::GetAttributeSetBase() const
{
	return PlayerAttributeSet;
}

UWeaponAttributeSet* AFPSPlayerState::GetAmmoAttributeSet() const
{
	return AmmoAttributeSet;
}

void AFPSPlayerState::BindDelegates()
{
	AMyProjectCharacter* Player = Cast<AMyProjectCharacter>(GetPawn());
	if(Player)
	{
		AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(Player->GetAbilitySystemComponent());
		PlayerAttributeSet = Player->GetPlayerAttributeSet();
		AmmoAttributeSet = Player->GetWeaponAmmoAttributeSet();
	}
	
	if (AbilitySystemComponent && PlayerAttributeSet)
	{
		// Attribute change callbacks
		HealthChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PlayerAttributeSet->GetHealthAttribute()).AddUObject(this, &AFPSPlayerState::HealthChanged);
		GasLeftChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PlayerAttributeSet->GetGasLeftAttribute()).AddUObject(this, &AFPSPlayerState::GasLeftChanged);
		GasRightChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(PlayerAttributeSet->GetGasRightAttribute()).AddUObject(this, &AFPSPlayerState::GasRightChanged);
	}
}

void AFPSPlayerState::OnRep_TeamIndex()
{
}

void AFPSPlayerState::OnRep_Points()
{
}

void AFPSPlayerState::OnRep_Kills()
{
}

void AFPSPlayerState::OnRep_Deaths()
{
}

void AFPSPlayerState::OnRep_Assists()
{
}

void AFPSPlayerState::BeginPlay()
{
	Super::BeginPlay();

}

void AFPSPlayerState::HealthChanged(const FOnAttributeChangeData& Data)
{
	
	// Check for and handle knockdown and death
	AMyProjectCharacter* Player = Cast<AMyProjectCharacter>(GetPawn());

	if(Player && !Player->IsAlive() && !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag))\
	{
		Player->Die();
	}

	AMyPlayerController* PC = Player->GetController<AMyPlayerController>();
	if(PC && PC->MainGameplayWidget)
	{
		PC->MainGameplayWidget->OnHealthChanged(Data.NewValue);
	}
	// TODO: UI changes here
}

void AFPSPlayerState::GasLeftChanged(const FOnAttributeChangeData& Data)
{
	AMyPlayerController* PC = GetPawn()->GetController<AMyPlayerController>();
	if(PC && PC->MainGameplayWidget)
	{
		PC->MainGameplayWidget->OnGasLeftChanged(Data.NewValue);
	}
}

void AFPSPlayerState::GasRightChanged(const FOnAttributeChangeData& Data)
{
	AMyPlayerController* PC = GetPawn()->GetController<AMyPlayerController>();
	if(PC && PC->MainGameplayWidget)
	{
		PC->MainGameplayWidget->OnGasRightChanged(Data.NewValue);
	}
}
