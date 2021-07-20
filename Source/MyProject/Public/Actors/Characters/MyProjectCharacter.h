// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffectTypes.h"
#include "MyProjectCharacter.generated.h"



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
};

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
	AMyProjectCharacter();

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	FGameplayTag CurrentWeaponTag;

	UFUNCTION(BlueprintCallable)
	class UCameraComponent* GetCurrentCameraCameraComponent() const { return (bIsFirstPersonPerspective) ? FirstPersonCamera : ThirdPersonCamera;}

	UFUNCTION(BlueprintCallable)
	virtual bool IsInFirstPersonPerspective() const { return bIsFirstPersonPerspective; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetFirstPersonMesh() const { return FPSMesh; }

	UFUNCTION(BlueprintCallable, BlueprintPure)
	USkeletalMeshComponent* GetThirdPersonMesh() const { return GetMesh(); }
	
	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

	UFUNCTION(BlueprintPure, Category = Ability, meta = (DefaultToSelf = Target))
		class UPlayerAttributeSet* GetPlayerAttributeSet() const { return PlayerAttributeSet; }

	UFUNCTION(BlueprintCallable, Category = "GASShooter|GSCharacter|Attributes")
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

	UFUNCTION(Server, Reliable)
	void ServerEquipWeapon(AFPSWeapon* NewWeapon);
	void ServerEquipWeapon_Implementation(AFPSWeapon* NewWeapon);
	bool ServerEquipWeapon_Validate(AFPSWeapon* NewWeapon) { return true; }

	// Adds a new weapon to the inventory.
	// Returns false if the weapon already exists in the inventory, true if it's a new weapon.
	UFUNCTION(BlueprintCallable)
	bool AddWeaponToInventory(AFPSWeapon* NewWeapon, bool bEquipWeapon = false);


	UFUNCTION(BlueprintCallable)
	bool IsAlive();

	
	
	UFUNCTION(BlueprintCallable)
	int32 GetPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable)
	int32 GetMaxPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable)
	int32 GetPrimaryReserveAmmo() const;
protected:

	/* attribute set to be dynamically set on equipped gun's attribute set */
	UPROPERTY()
	class UAttributeSet* WeaponAmmoAttributeSet;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Essential|Abilities")
	class UMyAbilitySetDataAsset* EssentialAbilities;

	// Default attributes for a character for initializing on spawn/respawn.
	// This is an instant GE that overrides the values for attributes that get reset on spawn/respawn.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|Abilities")
	TSubclassOf<class UGameplayEffect> DefaultAttributes;

	// These effects are only applied one time on startup
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Essential|Abilities")
	TArray<TSubclassOf<class UGameplayEffect>> StartupEffects;
	
	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default1PFOV = 90.f;
	
	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default1PWeaponFOV = 70.f;

	UPROPERTY(BlueprintReadOnly, Category = "Essential|FOV")
	float Default3PFOV = 90.f;


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
	virtual void InitializeAttributes();
	
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

