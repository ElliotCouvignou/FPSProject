// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSPlayerState.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/PlayerAttributeSet.h"
#include "AbilitySystem/AttributeSets/WeaponAttributeSet.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "MyPlayerController.h"
#include "UI/MainGameplayWidget.h"

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

	}
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
