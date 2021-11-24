// Copyright Epic Games, Inc. All Rights Reserved.

#include "Actors/Characters/MyProjectCharacter.h"

#include "FPSPlayerState.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"

#include "Actors/FPSWeapon.h"
#include "AbilitySystem/MyAbilitySetDataAsset.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "AbilitySystem/AttributeSets/PlayerAttributeSet.h"
#include "AbilitySystem/AttributeSets/WeaponAttributeSet.h"
#include "MyProjectGameMode.h"
#include "MyPlayerController.h"
#include "MyProject.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)

//////////////////////////////////////////////////////////////////////////
// AMyProjectCharacter

AMyProjectCharacter::AMyProjectCharacter(const class FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = false;


	NoWeaponTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Equipped.None"));
	WeaponChangingDelayReplicationTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon.IsChangingDelayReplication"));
	WeaponAmmoTypeNoneTag = FGameplayTag::RequestGameplayTag(FName("Weapon.Ammo.None"));
	WeaponAbilityTag = FGameplayTag::RequestGameplayTag(FName("Ability.Weapon"));
	CurrentWeaponTag = NoWeaponTag;
	Inventory = FFPSPlayerInventory();

	DeadTag = FGameplayTag::RequestGameplayTag("State.Dead");
	DeathTimerDel.BindUFunction(this, FName("FinishDying"));
	
	// Networking characteristics for characters
	bAlwaysRelevant = true;
	bReplicates = true;
	
	
	// set our turn rates for input
	BaseTurnRate = 45.f;
	BaseLookUpRate = 45.f;

	// Don't rotate when the controller rotates. Let that just affect the camera.
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;


	AbilitySystemComponent = CreateDefaultSubobject<UFPSAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	

	
	bActorSeamlessTraveled = true;

	PlayerAttributeSet = CreateDefaultSubobject<UPlayerAttributeSet>(TEXT("AttributeSet"));

	AmmoAttributeSet = CreateDefaultSubobject<UWeaponAttributeSet>(TEXT("CurrentWeaponAttributeSet"));

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = true; // Character moves in the direction of input...	
	GetCharacterMovement()->RotationRate = FRotator(0.0f, 540.0f, 0.0f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;


	BodyMesh = CreateOptionalDefaultSubobject<UStaticMeshComponent>("BodyMesh");
	if(BodyMesh)
	{
		BodyMesh->AlwaysLoadOnClient = true;
		BodyMesh->AlwaysLoadOnServer = true;
		BodyMesh->bOwnerNoSee = false;
		BodyMesh->bCastDynamicShadow = true;
		BodyMesh->bAffectDynamicIndirectLighting = true;
		BodyMesh->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		BodyMesh->SetupAttachment(GetCapsuleComponent());
		static FName MeshCollisionProfileName(TEXT("CharacterMesh"));
		BodyMesh->SetCollisionProfileName(MeshCollisionProfileName);
		BodyMesh->SetGenerateOverlapEvents(false);
		BodyMesh->SetCanEverAffectNavigation(false);
	}
	
	// Create a camera boom (pulls in towards the player if there is a collision)
	ThirdPersonCameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("ThirdPersonCameraBoom"));
	ThirdPersonCameraBoom->SetupAttachment(GetMesh());
	ThirdPersonCameraBoom->TargetArmLength = 300.0f; // The camera follows at this distance behind the character	
	ThirdPersonCameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller

	// Create a follow camera

		ThirdPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("ThirdPersonCamera"));
		ThirdPersonCamera->SetupAttachment(ThirdPersonCameraBoom, USpringArmComponent::SocketName); // Attach the camera to the end of the boom and let the boom adjust to match the controller orientation
		ThirdPersonCamera->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
	
	

		FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(FName("FirstPersonCamera"));
		FirstPersonCamera->SetupAttachment(RootComponent);
		FirstPersonCamera->bUsePawnControlRotation = true;// Camera does not rotate relative to arm
	
	
	FPSMesh = CreateDefaultSubobject<USkeletalMeshComponent>(FName("FPSMesh"));
	FPSMesh->SetupAttachment(FirstPersonCamera);
	FPSMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	FPSMesh->SetCollisionProfileName(FName("NoCollision"));
	FPSMesh->bReceivesDecals = false;
	FPSMesh->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPose;
	FPSMesh->CastShadow = false;
	FPSMesh->SetVisibility(false, true);

}

void AMyProjectCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyProjectCharacter, Inventory);
	// Only replicate CurrentWeapon to simulated clients and manually sync CurrentWeeapon with Owner when we're ready.
	// This allows us to predict weapon changing.
	DOREPLIFETIME(AMyProjectCharacter, CurrentWeapon);
	
	//DOREPLIFETIME_CONDITION(AMyProjectCharacter, CurrentWeapon, COND_SimulatedOnly);
}

//////////////////////////////////////////////////////////////////////////
// Input

void AMyProjectCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// Set up gameplay key bindings
	check(PlayerInputComponent);
	// PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	// PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	PlayerInputComponent->BindAxis("MoveForward", this, &AMyProjectCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AMyProjectCharacter::MoveRight);

	// We have 2 versions of the rotation bindings to handle different kinds of devices differently
	// "turn" handles devices that provide an absolute delta, such as a mouse.
	// "turnrate" is for devices that we choose to treat as a rate of change, such as an analog joystick
	//PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	//PlayerInputComponent->BindAxis("TurnRate", this, &AMyProjectCharacter::TurnAtRate);
	//PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	//PlayerInputComponent->BindAxis("LookUpRate", this, &AMyProjectCharacter::LookUpAtRate);

	// handle touch devices
	PlayerInputComponent->BindTouch(IE_Pressed, this, &AMyProjectCharacter::TouchStarted);
	PlayerInputComponent->BindTouch(IE_Released, this, &AMyProjectCharacter::TouchStopped);

	BindASCInput();
}

void AMyProjectCharacter::BindASCInput()
{
	UFPSAbilitySystemComponent* ASC = Cast<UFPSAbilitySystemComponent>(GetAbilitySystemComponent());

	if (!bASCInputBound && ASC && IsValid(InputComponent)) {
		FGameplayAbilityInputBinds AbilityBinds("ConfirmAbility", "CancelAbility", "EAbilityInputID",
			static_cast<int32>(EAbilityInputID::Confirm), static_cast<int32>(EAbilityInputID::Cancel));

		ASC->BindAbilityActivationToInputComponent(InputComponent, AbilityBinds);
		
		bASCInputBound = true;
	}
}


void AMyProjectCharacter::GiveEssentialAbilities()
{
	if (EssentialAbilities) {
		EssentialAbilities->GiveAbilities(GetAbilitySystemComponent());
	}
}

void AMyProjectCharacter::TouchStarted(ETouchIndex::Type FingerIndex, FVector Location)
{
		Jump();
}

void AMyProjectCharacter::TouchStopped(ETouchIndex::Type FingerIndex, FVector Location)
{
		StopJumping();
}

void AMyProjectCharacter::TurnAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds());
}

void AMyProjectCharacter::LookUpAtRate(float Rate)
{
	// calculate delta for this frame from the rate information
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds());
}


UAbilitySystemComponent* AMyProjectCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

int32 AMyProjectCharacter::GetCharacterLevel() const
{
	return 1;
}

FName AMyProjectCharacter::GetWeaponRightAttachPoint()
{
	return FName("RightHandSocket");
}

FName AMyProjectCharacter::GetWeaponRLeftAttachPoint()
{
	return FName("LeftHandSocket");
}

void AMyProjectCharacter::EquipWeapon(AFPSWeapon* NewWeapon)
{
	if (GetLocalRole() < ROLE_Authority)
	{
		ServerEquipWeapon(NewWeapon);
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
		bChangedWeaponLocally = true;
	}
	else
	{
		SetCurrentWeapon(NewWeapon, CurrentWeapon);
	}
}


void AMyProjectCharacter::UnEquipCurrentWeapon()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		CurrentWeaponTag = NoWeaponTag;
		AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
	}

	UnEquipWeapon(CurrentWeapon);
	CurrentWeapon = nullptr;

	AMyPlayerController* PC = GetController<AMyPlayerController>();
	if (PC && PC->IsLocalController())
	{
		// TODO: UPdate UI about change
		// PC->SetEquippedWeaponPrimaryIconFromSprite(nullptr);
		// PC->SetEquippedWeaponStatusText(FText());
		// PC->SetPrimaryClipAmmo(0);
		// PC->SetPrimaryReserveAmmo(0);
		// PC->SetHUDReticle(nullptr);
	}
}

void AMyProjectCharacter::SwapToWeaponIndex(int Index)
{
	int32 CurrentWeaponIndex = Inventory.Weapons.Find(CurrentWeapon);
	
	if (Inventory.Weapons.Num() < 2 || Index == CurrentWeaponIndex)
	{
		return;
	}
	
	UnEquipCurrentWeapon();

	if (Index < Inventory.Weapons.Num())
	{
		EquipWeapon(Inventory.Weapons[Index]);
	}
	else
	{
		EquipWeapon(Inventory.Weapons[0]);
	}
}

void AMyProjectCharacter::ServerEquipWeapon_Implementation(AFPSWeapon* NewWeapon)
{
	EquipWeapon(NewWeapon);
}

bool AMyProjectCharacter::AddWeaponToInventory(AFPSWeapon* NewWeapon, bool bEquipWeapon, int Index)
{
	if(!NewWeapon)
		return false;
	
	if (Inventory.Weapons.Contains(NewWeapon))
	{
		USoundCue* PickupSound = NewWeapon->GetPickupSound();

		if (PickupSound && IsLocallyControlled())
		{
			UGameplayStatics::SpawnSoundAttached(PickupSound, GetRootComponent());
		}

		if (GetLocalRole() < ROLE_Authority)
		{
			return false;
		}

		// Create a dynamic instant Gameplay Effect to give the primary and secondary ammo
		UGameplayEffect* GEAmmo = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("Ammo")));
		GEAmmo->DurationPolicy = EGameplayEffectDurationType::Instant;

		if (NewWeapon->PrimaryAmmoType != WeaponAmmoTypeNoneTag)
		{
			int32 Idx = GEAmmo->Modifiers.Num();
			GEAmmo->Modifiers.SetNum(Idx + 1);

			FGameplayModifierInfo& InfoPrimaryAmmo = GEAmmo->Modifiers[Idx];
			InfoPrimaryAmmo.ModifierMagnitude = FScalableFloat(NewWeapon->GetPrimaryClipAmmo());
			InfoPrimaryAmmo.ModifierOp = EGameplayModOp::Additive;
			// TODO: Set this with ammo attribute set:  InfoPrimaryAmmo.Attribute = UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(NewWeapon->PrimaryAmmoType);
		}

		if (NewWeapon->SecondaryAmmoType != WeaponAmmoTypeNoneTag)
		{
			int32 Idx = GEAmmo->Modifiers.Num();
			GEAmmo->Modifiers.SetNum(Idx + 1);

			FGameplayModifierInfo& InfoSecondaryAmmo = GEAmmo->Modifiers[Idx];
			InfoSecondaryAmmo.ModifierMagnitude = FScalableFloat(NewWeapon->GetSecondaryClipAmmo());
			InfoSecondaryAmmo.ModifierOp = EGameplayModOp::Additive;
			// TODO: Set this with ammo attribute set:  InfoSecondaryAmmo.Attribute = UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(NewWeapon->SecondaryAmmoType);
		}

		if (GEAmmo->Modifiers.Num() > 0)
		{
			AbilitySystemComponent->ApplyGameplayEffectToSelf(GEAmmo, 1.0f, AbilitySystemComponent->MakeEffectContext());
		}

		NewWeapon->Destroy();

		return false;
	}

	if (GetLocalRole() < ROLE_Authority)
	{
		return false;
	}

	// set new index (check for unequip)
	if(Inventory.Weapons[Index])
	{
		UnEquipWeapon(Inventory.Weapons[Index]);
	}
	
	Inventory.Weapons[Index] = NewWeapon;
	NewWeapon->SetOwningCharacter(this);

	OnWeaponAddedToInventory.Broadcast(NewWeapon);
	//NewWeapon->AddAbilities();

	if (bEquipWeapon)
	{
		EquipWeapon(NewWeapon);
		// TODO: ClientSyncCurrentWeapon(CurrentWeapon);
	}

	return true;
}

bool AMyProjectCharacter::IsAlive()
{
	if(PlayerAttributeSet)
	{
		return PlayerAttributeSet->GetHealth() > 0.f;
	}
	return true;
}

void AMyProjectCharacter::Die()
{
	//DisableInput(GetController<APlayerController>());

	OnCharacterDied.Broadcast(this);

	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->CancelAllAbilities();
		AbilitySystemComponent->BlockAbilitiesWithTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability")));

		// FGameplayTagContainer EffectTagsToRemove;
		// EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		// int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->AddLooseGameplayTag(DeadTag);
	}

	//TODO replace with a locally executed GameplayCue
	// if (DeathSound)
	// {
	// 	UGameplayStatics::PlaySoundAtLocation(this, DeathSound, GetActorLocation());
	// }

	// Ragdoll
	//Multicast_OnDeath();
	BP_DoRagdollEffect(true);

	GetWorldTimerManager().SetTimer(DeathTimerHandle, DeathTimerDel, RespawnDelay, false);
}

void AMyProjectCharacter::FinishDying()
{
	// Respawn at new locaiton
	//EnableInput(GetController<APlayerController>());
	//Multicast_OnRespawn();
	BP_DoRagdollEffect(false);

	GetCharacterMovement()->Velocity = FVector(0.f, 0.f, 0.f);
	
	if (IsValid(AbilitySystemComponent))
	{
		AbilitySystemComponent->UnBlockAbilitiesWithTags(FGameplayTagContainer(FGameplayTag::RequestGameplayTag("Ability")));

		// FGameplayTagContainer EffectTagsToRemove;
		// EffectTagsToRemove.AddTag(EffectRemoveOnDeathTag);
		// int32 NumEffectsRemoved = AbilitySystemComponent->RemoveActiveEffectsWithTags(EffectTagsToRemove);

		AbilitySystemComponent->RemoveLooseGameplayTag(DeadTag);

		PlayerAttributeSet->SetHealth(PlayerAttributeSet->GetMaxHealth());
	}

	// TODO: maybe handle spawning locaiton or all this logic elsewhere in gamemode
	AMyProjectGameMode* GM = GetWorld()->GetAuthGameMode<AMyProjectGameMode>();
	if(GM)
	{
		GM->MovePlayerToSafeSpawnLocation(this);
	}
}

void AMyProjectCharacter::Multicast_OnDeath_Implementation()
{
	BP_DoRagdollEffect(true);
}

void AMyProjectCharacter::Multicast_OnRespawn_Implementation()
{
	BP_DoRagdollEffect(false);
}

int32 AMyProjectCharacter::GetPrimaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetPrimaryClipAmmo();
	}

	return 0;
}

int32 AMyProjectCharacter::GetMaxPrimaryClipAmmo() const
{
	if (CurrentWeapon)
	{
		return CurrentWeapon->GetMaxPrimaryClipAmmo();
	}

	return 0;
}

int32 AMyProjectCharacter::GetPrimaryReserveAmmo() const
{
	if (CurrentWeapon && AmmoAttributeSet)
	{
		FGameplayAttribute Attribute = AmmoAttributeSet->GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType);
		if (Attribute.IsValid())
		{
			return AbilitySystemComponent->GetNumericAttribute(Attribute);
		}
	}

	return 0;
}

void AMyProjectCharacter::FellOutOfWorld(const UDamageType& dmgType)
{
	//if(!HasAuthority() || !AbilitySystemComponent || !PlayerAttributeSet)
	//	Super::FellOutOfWorld(dmgType);

	if(IsAlive() && HasAuthority() || GetLocalRole() == ROLE_None)
	{
		PlayerAttributeSet->SetHealth(0.f);
	
		AMyPlayerController* PC = GetController<AMyPlayerController>();
		FDiedInfoStruct InfoStruct;
		InfoStruct.PlayerName = "Fell Out of World";
		InfoStruct.HealthRemaining = -1.f;
		InfoStruct.WeaponName =  "Omnipotent Powers";
		if(PC)
			PC->OnPlayerDied(InfoStruct);
	}
}

void AMyProjectCharacter::InitializeAttributeSet()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitStats(UPlayerAttributeSet::StaticClass(), AttrDataTable);
	}
}

void AMyProjectCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		// find out which way is forward
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);

		// get forward vector
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		AddMovementInput(Direction, Value);
	}
}

void AMyProjectCharacter::MoveRight(float Value)
{
	if ( (Controller != nullptr) && (Value != 0.0f) )
	{
		// find out which way is right
		const FRotator Rotation = Controller->GetControlRotation();
		const FRotator YawRotation(0, Rotation.Yaw, 0);
	
		// get right vector 
		const FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
		// add movement in that direction
		AddMovementInput(Direction, Value);
	}
}


void AMyProjectCharacter::SetCurrentWeapon(AFPSWeapon* NewWeapon, AFPSWeapon* LastWeapon)
{
	if (NewWeapon == LastWeapon)
	{
		return;
	}

	// Cancel active weapon abilities
	if (AbilitySystemComponent)
	{
		FGameplayTagContainer AbilityTagsToCancel = FGameplayTagContainer(WeaponAbilityTag);
		AbilitySystemComponent->CancelAbilities(&AbilityTagsToCancel);
	}

	UnEquipWeapon(LastWeapon);

	if (NewWeapon)
	{
		if (AbilitySystemComponent)
		{
			// Clear out potential NoWeaponTag
			AbilitySystemComponent->RemoveLooseGameplayTag(CurrentWeaponTag);
		}

		// Weapons coming from OnRep_CurrentWeapon won't have the owner set
		CurrentWeapon = NewWeapon;
		CurrentWeapon->SetOwningCharacter(this);
		CurrentWeapon->Equip();
		CurrentWeaponTag = CurrentWeapon->WeaponTag;

		if (AbilitySystemComponent)
		{
			AbilitySystemComponent->AddLooseGameplayTag(CurrentWeaponTag);
		}

		// TODO: UI update
		// AGSPlayerController* PC = GetController<AGSPlayerController>();
		// if (PC && PC->IsLocalController())
		// {
		// 	PC->SetEquippedWeaponPrimaryIconFromSprite(CurrentWeapon->PrimaryIcon);
		// 	PC->SetEquippedWeaponStatusText(CurrentWeapon->StatusText);
		// 	PC->SetPrimaryClipAmmo(CurrentWeapon->GetPrimaryClipAmmo());
		// 	PC->SetPrimaryReserveAmmo(GetPrimaryReserveAmmo());
		// 	PC->SetHUDReticle(CurrentWeapon->GetPrimaryHUDReticleClass());
		// }
 
		NewWeapon->OnPrimaryClipAmmoChanged.AddDynamic(this, &AMyProjectCharacter::CurrentWeaponPrimaryClipAmmoChanged);
		NewWeapon->OnSecondaryClipAmmoChanged.AddDynamic(this, &AMyProjectCharacter::CurrentWeaponSecondaryClipAmmoChanged);
		
		if (AbilitySystemComponent)
		{
			// TODO: fill with ammo attributee set 
			//PrimaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->PrimaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponPrimaryReserveAmmoChanged);
			//SecondaryReserveAmmoChangedDelegateHandle = AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(CurrentWeapon->SecondaryAmmoType)).AddUObject(this, &AGSHeroCharacter::CurrentWeaponSecondaryReserveAmmoChanged);
		}

		UAnimMontage* Equip1PMontage = CurrentWeapon->GetEquip1PMontage();
		if (Equip1PMontage && GetFirstPersonMesh())
		{
			GetFirstPersonMesh()->GetAnimInstance()->Montage_Play(Equip1PMontage);
		}

		UAnimMontage* Equip3PMontage = CurrentWeapon->GetEquip3PMontage();
		if (Equip3PMontage && GetThirdPersonMesh())
		{
			GetThirdPersonMesh()->GetAnimInstance()->Montage_Play(Equip3PMontage);
		}
	}
	else
	{
		// This will clear HUD, tags etc
		// TODO: UnEquipCurrentWeapon();
	}

	OnCurrentWeaponChanged.Broadcast(NewWeapon);
	
}

void AMyProjectCharacter::UnEquipWeapon(AFPSWeapon* WeaponToUnEquip)
{
	if (WeaponToUnEquip)
	{
		WeaponToUnEquip->OnPrimaryClipAmmoChanged.RemoveDynamic(this, &AMyProjectCharacter::CurrentWeaponPrimaryClipAmmoChanged);
		WeaponToUnEquip->OnSecondaryClipAmmoChanged.RemoveDynamic(this, &AMyProjectCharacter::CurrentWeaponSecondaryClipAmmoChanged);

		if (AbilitySystemComponent)
		{
			// TODO: fill with ammo attributee set 
			//AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(WeaponToUnEquip->PrimaryAmmoType)).Remove(PrimaryReserveAmmoChangedDelegateHandle);
			//AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UGSAmmoAttributeSet::GetReserveAmmoAttributeFromTag(WeaponToUnEquip->SecondaryAmmoType)).Remove(SecondaryReserveAmmoChangedDelegateHandle);
		}
		
		WeaponToUnEquip->UnEquip();
	}	
}


void AMyProjectCharacter::CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo)
{
	// TODO: UI UPDDATES 
	//APlayerController* PC = GetController<APlayerController>();
	//if (PC && PC->IsLocalController())
	//{
	//	PC->SetPrimaryClipAmmo(NewPrimaryClipAmmo);
	//}
}

void AMyProjectCharacter::CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo)
{
	// TODO: UI UPDDATES 
	//APlayerController* PC = GetController<APlayerController>();
	//if (PC && PC->IsLocalController())
	//{
	//	PC->SetSecondaryClipAmmo(NewSecondaryClipAmmo);
	//}
}



void AMyProjectCharacter::AddStartupEffects()
{
	if (GetLocalRole() != ROLE_Authority || !IsValid(AbilitySystemComponent) || AbilitySystemComponent->bStartupEffectsApplied)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(this);

	for (TSubclassOf<UGameplayEffect> GameplayEffect : StartupEffects)
	{
		FGameplayEffectSpecHandle NewHandle = AbilitySystemComponent->MakeOutgoingSpec(GameplayEffect, GetCharacterLevel(), EffectContext);
		if (NewHandle.IsValid())
		{
			FActiveGameplayEffectHandle ActiveGEHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*NewHandle.Data.Get(), AbilitySystemComponent);
		}
	}

	AbilitySystemComponent->bStartupEffectsApplied = true;
}

void AMyProjectCharacter::OnRep_Inventory()
{
	if (GetLocalRole() == ROLE_AutonomousProxy && Inventory.Weapons.Num() > 0 && !CurrentWeapon)
	{ 
		// Since we don't replicate the CurrentWeapon to the owning client, this is a way to ask the Server to sync
		// the CurrentWeapon after it's been spawned via replication from the Server.
		// The weapon spawning is replicated but the variable CurrentWeapon is not on the owning client.
		// TODO: ServerSyncCurrentWeapon();
	}
	
}

void AMyProjectCharacter::OnRep_CurrentWeapon(AFPSWeapon* LastWeapon)
{
	// TODO: figure out this bool
	//bChangedWeaponLocally = false;
	SetCurrentWeapon(CurrentWeapon, LastWeapon);
}

void AMyProjectCharacter::OnRep_Controller()
{
	Super::OnRep_Controller();

	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	
	
	BindASCInput();

	
}


// Server only, Clients process below function
// this has server only functions being called
void AMyProjectCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	NewController->SetOwner(this);
	
	AbilitySystemComponent->InitAbilityActorInfo(this, this);

	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if(PS)
	{
		BindASCInput();

		PS->BindDelegates();
		
		InitializeAttributeSet();

		AddStartupEffects();

		PlayerName = PS->GetPlayerName();

		if (EssentialAbilities) {
			EssentialAbilities->GiveAbilities(GetAbilitySystemComponent());
		}

		AMyPlayerController* PC = Cast<AMyPlayerController>(NewController);
		if(PC && PC->IsLocalController())
		{
			PC->CreateMainGameplayWidget();
		}
	}

	


}


void AMyProjectCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// Init ASC Actor Info for clients. Server will init its ASC when it possesses a new Actor.
	if(GetController())
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	
	BindASCInput();

	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if(PS)
	{
		PS->BindDelegates();

		PlayerName = PS->GetPlayerName();
	}

	AMyPlayerController* PC = GetController<AMyPlayerController>();
	if(PC && PC->IsLocalController())
	{
		PC->CreateMainGameplayWidget();
	}
}

// TODO: use below when having playerstate use ASC veriosn
/*// Server only, Clients process below function
// this has server only functions being called
void AMyProjectCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		AbilitySystemComponent->InitAbilityActorInfo(PS, this);
		
		PlayerAttributeSet = PS->GetAttributeSetBase();
		AmmoAttributeSet = PS->GetAmmoAttributeSet();
		

		BindASCInput();

		InitializeAttributes();

		AddStartupEffects();

		if (EssentialAbilities) {
			EssentialAbilities->GiveAbilities(GetAbilitySystemComponent());
		}
	}
}


void AMyProjectCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	AFPSPlayerState* PS = GetPlayerState<AFPSPlayerState>();
	if (PS)
	{
		AbilitySystemComponent = Cast<UFPSAbilitySystemComponent>(PS->GetAbilitySystemComponent());

		AbilitySystemComponent->InitAbilityActorInfo(PS, this);
		
		PlayerAttributeSet = PS->GetAttributeSetBase();
		AmmoAttributeSet = PS->GetAmmoAttributeSet();
	
		BindASCInput();
	}

}*/