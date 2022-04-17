// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "MyProjectCharacter.generated.h"


#define NUM_PLAYER_WEAPONS 4

class UFPSAbilitySystemComponent;
class AFPSWeapon;


USTRUCT()
struct MYPROJECT_API FFPSPlayerInventory
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY()
	TArray<AFPSWeapon*> Weapons;

	// TODO: add following
	// Consumable items

	// Passive items like armor

	// Equippment/grenades

	// Etc
	FFPSPlayerInventory()
	{
		Weapons.SetNum(NUM_PLAYER_WEAPONS);
	}
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCharacterDiedDelegate, AMyProjectCharacter*, Character);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFpsWeaponDelegate, AFPSWeapon*, Weapon);


UCLASS(config=Game)
class AMyProjectCharacter : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UPlayerAttributeSet* PlayerAttributeSet;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	class UWeaponAttributeSet* AmmoAttributeSet;

	/** this is gonna be shitty capsule */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> BodyMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<USkeletalMeshComponent> FPSMesh;


	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class USpringArmComponent* ThirdPersonCameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* ThirdPersonCamera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCamera;
	

public:
	AMyProjectCharacter(const class FObjectInitializer& ObjectInitializer);

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FString PlayerName = "NoName";

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CurrentWeaponTag;

	UFUNCTION(BlueprintCallable)
	class UCameraComponent* GetCurrentCameraCameraComponent() const { return (bIsFirstPersonPerspective) ? FirstPersonCamera : ThirdPersonCamera;}

	UFUNCTION(BlueprintCallable)
	virtual bool IsInFirstPersonPerspective() const { return bIsFirstPersonPerspective; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetFirstPersonMesh() const { return (bDoNoFPSMeshMethod) ? nullptr : FPSMesh; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetThirdPersonMesh() const { return GetMesh(); }

	/* Only call this on locally controlled cleitns */
	UFUNCTION(BlueprintCallable, BlueprintPure)
	UCameraComponent* GetFirstPersonCamera() const { return FirstPersonCamera; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	class UFPSMovementComponent* GetFPSMovementComponent() const;

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FTransform GetRightHandLocation() const { return RightHandLocation; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	FTransform GetLeftHandLocation() const { return LeftHandLocation; }

	/* returns fps ODM when in FPS and TPS in TPS, only relevant when called on localplayercontrolled client */
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	USkeletalMeshComponent* GetLocalODMComponent();
	
	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = Ability, meta = (DefaultToSelf = Target))
		class UPlayerAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

	UFUNCTION(BlueprintPure, Category = Ability, meta = (DefaultToSelf = Target))
	class UWeaponAttributeSet* GetWeaponAmmoAttributeSet() const { return WeaponAmmoAttributeSet; }

	UFUNCTION(BlueprintCallable)
	int32 GetCharacterLevel() const;
	
	/** Base turn rate, in deg/sec. Other scaling may affect final turn rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseTurnRate;

	/** Base look up/down rate, in deg/sec. Other scaling may affect final rate. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category=Camera)
	float BaseLookUpRate;

	UFUNCTION(BlueprintCallable)
	FName GetWeaponRightAttachPoint();

	UFUNCTION(BlueprintCallable)
	FName GetWeaponRLeftAttachPoint();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool bIsFirstPersonPerspective = true;

	UFUNCTION(BlueprintCallable)
	AFPSWeapon* GetCurrentWeapon() const { return CurrentWeapon; }

	UFUNCTION(BlueprintCallable)
	void EquipWeapon(AFPSWeapon* NewWeapon);



	// Unequips the current weapon. Used if for example we drop the current weapon.
	void UnEquipCurrentWeapon();
	
	UFUNCTION(BlueprintCallable)
	virtual void SwapToWeaponIndex(int Index);

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AFPSWeapon* NewWeapon);
	void ServerEquipWeapon_Implementation(AFPSWeapon* NewWeapon);
	bool ServerEquipWeapon_Validate(AFPSWeapon* NewWeapon) { return true; }

	// Adds a new weapon to the inventory.
	// Returns false if the weapon already exists in the inventory, true if it's a new weapon.
	UFUNCTION(BlueprintCallable)
	bool AddWeaponToInventory(AFPSWeapon* NewWeapon, bool bEquipWeapon = false, int Index = 0);

	UPROPERTY(BlueprintAssignable)
	FCharacterDiedDelegate OnCharacterDied;

	UPROPERTY(BlueprintAssignable)
	FFpsWeaponDelegate OnWeaponAddedToInventory;

	UPROPERTY(BlueprintAssignable)
	FFpsWeaponDelegate OnCurrentWeaponChanged;	
	
	UFUNCTION(BlueprintCallable, BlueprintPure)
	bool IsAlive();

	virtual void Die();

	UFUNCTION(BlueprintCallable)
	virtual void FinishDying();

	UFUNCTION(BlueprintCallable, NetMulticast, WithValidation, Reliable)
	void Multicast_OnDeath();
	void Multicast_OnDeath_Implementation();
	bool Multicast_OnDeath_Validate() { return true; }

	UFUNCTION(BlueprintCallable, NetMulticast, WithValidation, Reliable)
	void Multicast_OnRespawn();
	void Multicast_OnRespawn_Implementation();
	bool Multicast_OnRespawn_Validate() { return true; }
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void BP_DoRagdollEffect(bool DoRagdoll);
	
	
	UFUNCTION(BlueprintCallable)
	int32 GetPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable)
	int32 GetMaxPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable)
	int32 GetPrimaryReserveAmmo() const;


	/** Called when the actor falls out of the world 'safely' (below KillZ and such) */
	virtual void FellOutOfWorld(const class UDamageType& dmgType) override;

protected:

	virtual void InitializeAttributeSet();
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Essential|Abilities")
	UDataTable* AttrDataTable;

	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float RespawnDelay = 3.f;
	
	FGameplayTag DeadTag;
	FTimerDelegate DeathTimerDel;
	FTimerHandle DeathTimerHandle;


	/* attribute set to be dynamically set on equipped gun's attribute set */
	UPROPERTY()
	class UWeaponAttributeSet* WeaponAmmoAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Essential|Abilities")
	class UMyAbilitySetDataAsset* EssentialAbilities;

	// These effects are only applied one time on startup
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;
	
	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default1PFOV = 90.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default1PWeaponFOV = 70.f;

	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default3PFOV = 90.f;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|FPSNoMesh")
	bool bDoNoFPSMeshMethod;
	/* local location when attatched to fps camera */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|FPSNoMesh")
	FTransform RightHandLocation;
	/* local location when attatched to fps camera */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|FPSNoMesh")
	FTransform LeftHandLocation;
	
	FGameplayTag NoWeaponTag;
	FGameplayTag WeaponChangingDelayReplicationTag;
	FGameplayTag WeaponAmmoTypeNoneTag;
	FGameplayTag WeaponAbilityTag;
	FGameplayTag KnockedDownTag;
	FGameplayTag InteractingTag;

	// Attribute changed delegate handles
	FDelegateHandle PrimaryReserveAmmoChangedDelegateHandle;
	FDelegateHandle SecondaryReserveAmmoChangedDelegateHandle;

	// Tag changed delegate handles
	FDelegateHandle WeaponChangingDelayReplicationTagChangedDelegateHandle;

	
	/** Called for forwards/backward input */
	void MoveForward(float Value);

	/** Called for side to side input */
	void MoveRight(float Value);

	/** 
	 * Called via input to turn at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void TurnAtRate(float Rate);

	/**
	 * Called via input to turn look up/down at a given rate. 
	 * @param Rate	This is a normalized rate, i.e. 1.0 means 100% of desired turn rate
	 */
	void LookUpAtRate(float Rate);

	/** Handler for when a touch input begins. */
	void TouchStarted(ETouchIndex::Type FingerIndex, FVector Location);

	/** Handler for when a touch input stops. */
	void TouchStopped(ETouchIndex::Type FingerIndex, FVector Location);

	// APawn interface
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
	// End of APawn interface

	void BindASCInput();
	bool bASCInputBound = false;

	void GiveEssentialAbilities();


	UPROPERTY(ReplicatedUsing = OnRep_Inventory)
	FFPSPlayerInventory Inventory;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	TArray<TSubclassOf<AFPSWeapon>> DefaultInventoryWeaponClasses;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentWeapon)
	AFPSWeapon* CurrentWeapon;

	// Set to true when we change the weapon predictively and flip it to false when the Server replicates to confirm.
	// We use this if the Server refused a weapon change ability's activation to ask the Server to sync the client back up
	// with the correct CurrentWeapon.
	bool bChangedWeaponLocally;
	

	void SetCurrentWeapon(AFPSWeapon* NewWeapon, AFPSWeapon* LastWeapon);

	// Unequips the specified weapon. Used when OnRep_CurrentWeapon fires.
	void UnEquipWeapon(AFPSWeapon* WeaponToUnEquip);

	UFUNCTION()
	virtual void CurrentWeaponPrimaryClipAmmoChanged(int32 OldPrimaryClipAmmo, int32 NewPrimaryClipAmmo);

	UFUNCTION()
	virtual void CurrentWeaponSecondaryClipAmmoChanged(int32 OldSecondaryClipAmmo, int32 NewSecondaryClipAmmo);

	// Initialize the Character's attributes. Must run on Server but we run it on Client too
	// so that we don't have to wait. The Server's replication to the Client won't matter since
	// the values should be the same.
	
	virtual void AddStartupEffects();
	
	UFUNCTION()
	void OnRep_Inventory();

	UFUNCTION()
	void OnRep_CurrentWeapon(AFPSWeapon* LastWeapon);
	

public:
	/** Returns CameraBoom subobject **/
	FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return ThirdPersonCameraBoom; }
	/** Returns FollowCamera subobject **/
	FORCEINLINE class UCameraComponent* GetFollowCamera() const { return (bIsFirstPersonPerspective) ? FirstPersonCamera : ThirdPersonCamera; }
	
	/* Override Controller Posession*/
	virtual void OnRep_Controller() override;

	virtual void PossessedBy(AController* NewController) override;
	
	// called by clients
	virtual void OnRep_PlayerState() override;
};

