// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/FPSWeapon.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "Components/CapsuleComponent.h"
#include "Camera/CameraComponent.h"
#include "AbilitySystem/FPSWeaponGameplayAbility.h"
#include "AbilitySystem/MyGTGA_LineTrace.h"
#include "Net/UnrealNetwork.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


// Sets default values
AFPSWeapon::AFPSWeapon()
{
	// Set this actor to never tick
	PrimaryActorTick.bCanEverTick = false;

	bReplicates = true;
	bAlwaysRelevant = true;
	bNetUseOwnerRelevancy = true;
	NetUpdateFrequency = 100.0f; // Set this to a value that's appropriate for your game
	bSpawnWithCollision = true;
	PrimaryClipAmmo = 0;
	MaxPrimaryClipAmmo = 0;
	SecondaryClipAmmo = 0;
	MaxSecondaryClipAmmo = 0;
	bInfiniteAmmo = false;
	PrimaryAmmoType = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));
	SecondaryAmmoType = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));

	// for pickup
	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(FName("CollisionComponent"));
	CollisionComp->InitCapsuleSize(40.0f, 50.0f);
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision); // Manually enable when in pickup mode
	CollisionComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	RootComponent = CollisionComp;

	WeaponMesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(FName("WeaponMesh1P"));
	WeaponMesh1P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh1P->CastShadow = false;
	WeaponMesh1P->SetVisibility(false, true);
	WeaponMesh1P->SetupAttachment(GetRootComponent());
	WeaponMesh1P->bOnlyOwnerSee = true;
	
	//WeaponMesh1P->SetRelativeTransform(WeaponMesh1PEquippedRelativeTransform);
	WeaponMesh1P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	//WeaponMesh3PickupRelativeLocation = FVector(0.0f, -25.0f, 0.0f);

	WeaponMesh3P = CreateDefaultSubobject<USkeletalMeshComponent>(FName("WeaponMesh3P"));
	WeaponMesh3P->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh3P->SetupAttachment(GetRootComponent());
	WeaponMesh3P->SetRelativeTransform(WeaponMesh3PEquippedRelativeTransform);
	WeaponMesh3P->CastShadow = true;
	WeaponMesh3P->SetVisibility(true, true);
	WeaponMesh3P->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;

	WeaponPrimaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Primary.Instant");
	WeaponSecondaryInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Secondary.Instant");
	WeaponAlternateInstantAbilityTag = FGameplayTag::RequestGameplayTag("Ability.Weapon.Alternate.Instant");
	WeaponIsFiringTag = FGameplayTag::RequestGameplayTag("Weapon.IsFiring");

	FireMode = FGameplayTag::RequestGameplayTag("Weapon.FireMode.None");
	StatusText = DefaultStatusText;

	RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.Dead"));
	//RestrictedPickupTags.AddTag(FGameplayTag::RequestGameplayTag("State.KnockedDown"));
}


UAbilitySystemComponent* AFPSWeapon::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AFPSWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AFPSWeapon, OwningCharacter);
	//DOREPLIFETIME(AFPSWeapon, PrimaryClipAmmo);
	//DOREPLIFETIME(AFPSWeapon, MaxPrimaryClipAmmo);
	//DOREPLIFETIME(AFPSWeapon, SecondaryClipAmmo);
	// DOREPLIFETIME(AFPSWeapon, MaxSecondaryClipAmmo);
	DOREPLIFETIME_CONDITION(AFPSWeapon, OwningCharacter, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSWeapon, PrimaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSWeapon, MaxPrimaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSWeapon, SecondaryClipAmmo, COND_OwnerOnly);
	DOREPLIFETIME_CONDITION(AFPSWeapon, MaxSecondaryClipAmmo, COND_OwnerOnly);
}

void AFPSWeapon::PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker)
{
	Super::PreReplication(ChangedPropertyTracker);
	
	//DOREPLIFETIME_ACTIVE_OVERRIDE(AFPSWeapon, PrimaryClipAmmo, (IsValid(AbilitySystemComponent)/* && !AbilitySystemComponent->HasMatchingGameplayTag(WeaponIsFiringTag)*/));
	//DOREPLIFETIME_ACTIVE_OVERRIDE(AFPSWeapon, SecondaryClipAmmo, (IsValid(AbilitySystemComponent) /*&& !AbilitySystemComponent->HasMatchingGameplayTag(WeaponIsFiringTag)*/));
}

void AFPSWeapon::SetOwningCharacter(AMyProjectCharacter* Character)
{
	OwningCharacter = Character;
	if (OwningCharacter)
	{
		// Called when added to inventory
		AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());
		this->SetOwner(OwningCharacter);
		AttachToComponent(OwningCharacter->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);
		CollisionComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);

		if (OwningCharacter->GetCurrentWeapon() != this)
		{
			WeaponMesh3P->CastShadow = false;
			WeaponMesh3P->SetVisibility(true, true);
			WeaponMesh3P->SetVisibility(false, true);
		}
	}
	else
	{
		AbilitySystemComponent = nullptr;
		SetOwner(nullptr);
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	}
	
}

void AFPSWeapon::Equip()
{
	
	if (!OwningCharacter)
	{
		UE_LOG(LogTemp, Error, TEXT("%s %s OwningCharacter is nullptr"), *FString(__FUNCTION__), *GetName());
		return;
	}

	FName AttachPoint = (bAttatchRight) ? OwningCharacter->GetWeaponRightAttachPoint() : OwningCharacter->GetWeaponRLeftAttachPoint();

	if (WeaponMesh1P)
	{

		if(OwningCharacter->GetFirstPersonMesh())
		{
			WeaponMesh1P->AttachToComponent(OwningCharacter->GetFirstPersonMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
			WeaponMesh1P->SetRelativeTransform(WeaponMesh1PEquippedRelativeTransform);
		}			
		else
		{
			WeaponMesh1P->AttachToComponent(OwningCharacter->GetFirstPersonCamera(), FAttachmentTransformRules::KeepWorldTransform, AttachPoint);
			WeaponMesh1P->SetRelativeTransform(bAttatchRight ? OwningCharacter->GetRightHandLocation() : OwningCharacter->GetLeftHandLocation());
		}
			
		if (OwningCharacter->IsInFirstPersonPerspective())
		{
			WeaponMesh1P->bOnlyOwnerSee = true;
			WeaponMesh1P->SetVisibility(true, true);
		}
		else
		{
			WeaponMesh1P->SetVisibility(false, true);
		}
	}

	if (WeaponMesh3P)
	{
		
		WeaponMesh3P->AttachToComponent(OwningCharacter->GetThirdPersonMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, AttachPoint);
		WeaponMesh3P->SetRelativeTransform(WeaponMesh3PEquippedRelativeTransform);
		WeaponMesh3P->bCastHiddenShadow = true;
		
		if (OwningCharacter->IsInFirstPersonPerspective() && OwningCharacter->IsLocallyControlled())
		{
			WeaponMesh3P->SetVisibility(true, true); // Without this, the weapon's 3p shadow doesn't show
			WeaponMesh3P->SetVisibility(false, true);
		}
		else
		{
			WeaponMesh3P->SetVisibility(true, true);
		}
	}
}

void AFPSWeapon::UnEquip()
{
}

void AFPSWeapon::AddAbilities()
{
	if (!IsValid(OwningCharacter) || !OwningCharacter->GetAbilitySystemComponent())
	{
		return;
	}

	UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());

	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("[AFPSWeapon::AddAbilities]  %s %s ASC is null"), *FString(__FUNCTION__), *GetName());
		return;
	}

	// Grant abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (TSubclassOf<UFPSWeaponGameplayAbility>& Ability : Abilities)
	{
		// TODO: make ability subclass for this 
		AbilitySpecHandles.Add(ASC->GiveAbility(
			FGameplayAbilitySpec(Ability, GetAbilityLevel(Ability.GetDefaultObject()->AbilityID), static_cast<int32>(Ability.GetDefaultObject()->AbilityInputID), this)));
	}
}

void AFPSWeapon::RemoveAbilities()
{
	if (!IsValid(OwningCharacter) || !OwningCharacter->GetAbilitySystemComponent())
	{
		return;
	}

	UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(OwningCharacter->GetAbilitySystemComponent());

	if (!ASC)
	{
		return;
	}

	// Remove abilities, but only on the server	
	if (GetLocalRole() != ROLE_Authority)
	{
		return;
	}

	for (FGameplayAbilitySpecHandle& SpecHandle : AbilitySpecHandles)
	{
		ASC->ClearAbility(SpecHandle);
	}
}

int32 AFPSWeapon::GetAbilityLevel(EAbilityInputID AbilityID)
{
	return 1;
}

void AFPSWeapon::ResetWeapon()
{
	FireMode = DefaultFireMode;
	StatusText = DefaultStatusText;
}

int32 AFPSWeapon::GetPrimaryClipAmmo() const
{
	return PrimaryClipAmmo;
}

int32 AFPSWeapon::GetMaxPrimaryClipAmmo() const
{
	return MaxPrimaryClipAmmo;
}

int32 AFPSWeapon::GetSecondaryClipAmmo() const
{
	return SecondaryClipAmmo;
}

int32 AFPSWeapon::GetMaxSecondaryClipAmmo() const
{
	return MaxSecondaryClipAmmo;
}

void AFPSWeapon::SetPrimaryClipAmmo(int32 NewPrimaryClipAmmo)
{
	OnPrimaryClipAmmoChanged.Broadcast(PrimaryClipAmmo, NewPrimaryClipAmmo);
	PrimaryClipAmmo = NewPrimaryClipAmmo;
}

void AFPSWeapon::SetMaxPrimaryClipAmmo(int32 NewMaxPrimaryClipAmmo)
{
	OnMaxPrimaryClipAmmoChanged.Broadcast(PrimaryClipAmmo, MaxPrimaryClipAmmo);
	MaxPrimaryClipAmmo = NewMaxPrimaryClipAmmo;
}

void AFPSWeapon::SetSecondaryClipAmmo(int32 NewSecondaryClipAmmo)
{
	OnSecondaryClipAmmoChanged.Broadcast(SecondaryClipAmmo, NewSecondaryClipAmmo);
	SecondaryClipAmmo = NewSecondaryClipAmmo;
}

void AFPSWeapon::SetMaxSecondaryClipAmmo(int32 NewMaxSecondaryClipAmmo)
{
	OnMaxSecondaryClipAmmoChanged.Broadcast(SecondaryClipAmmo, MaxSecondaryClipAmmo);
	MaxSecondaryClipAmmo = NewMaxSecondaryClipAmmo;
}

TSubclassOf<UUserWidget> AFPSWeapon::GetPrimaryHUDReticleClass() const
{
	return PrimaryHUDReticleClass;
}

bool AFPSWeapon::HasInfiniteAmmo() const
{
	return bInfiniteAmmo;
}

UAnimMontage* AFPSWeapon::GetEquip1PMontage() const
{
	return Equip1PMontage;
}

UAnimMontage* AFPSWeapon::GetEquip3PMontage() const
{
	return Equip3PMontage;
}

USoundCue* AFPSWeapon::GetPickupSound() const
{
	return PickupSound;
}

FText AFPSWeapon::GetDefaultStatusText() const
{
	return DefaultStatusText;
}

AMyGTGA_LineTrace* AFPSWeapon::GetLineTraceTargetActor()
{
	if (LineTraceTargetActor)
	{
		return LineTraceTargetActor;
	}

	LineTraceTargetActor = GetWorld()->SpawnActor<AMyGTGA_LineTrace>();
	LineTraceTargetActor->SetOwner(this);
	return LineTraceTargetActor;
}

// Called when the game starts or when spawned
void AFPSWeapon::BeginPlay()
{
	ResetWeapon();

	
	Super::BeginPlay();
	
}

void AFPSWeapon::OnRep_PrimaryClipAmmo(int32 OldPrimaryClipAmmo)
{
	OnPrimaryClipAmmoChanged.Broadcast(OldPrimaryClipAmmo, PrimaryClipAmmo);
}

void AFPSWeapon::OnRep_MaxPrimaryClipAmmo(int32 OldMaxPrimaryClipAmmo)
{
	OnMaxPrimaryClipAmmoChanged.Broadcast(OldMaxPrimaryClipAmmo, MaxPrimaryClipAmmo);
}

void AFPSWeapon::OnRep_SecondaryClipAmmo(int32 OldSecondaryClipAmmo)
{
	OnSecondaryClipAmmoChanged.Broadcast(OldSecondaryClipAmmo, SecondaryClipAmmo);
}

void AFPSWeapon::OnRep_MaxSecondaryClipAmmo(int32 OldMaxSecondaryClipAmmo)
{
	OnMaxSecondaryClipAmmoChanged.Broadcast(OldMaxSecondaryClipAmmo, MaxSecondaryClipAmmo);
}

// Called every frame
void AFPSWeapon::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

