// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AbilitySystemInterface.h"
#include "GameplayAbilitySpec.h"
#include "GameplayTagContainer.h"
#include "MyProject.h"
#include "Curves/CurveFloat.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "MatineeCameraShake.h"

#include "FPSWeapon.generated.h"

class AMyGTGA_LineTrace;
class USkeletalMeshComponent;
class AMyProjectCharacter;
class UFPSAbilitySystemComponent;
class UFPSWeaponGameplayAbility;


/* Holds two vector values corresponding to min/max XYZ values for offset */
USTRUCT(BlueprintType)
struct FFPSRecoilValue
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FVector MinOffset;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly)
	FVector MaxOffset;
	
	FFPSRecoilValue()
	{
		
	}
};

UENUM(BlueprintType)
enum class EWeaponType : uint8
{
	AssaultRifle		UMETA(DisplayName = "AssaultRifle"),
	SniperRifle			UMETA(DisplayName = "SniperRifle"),
	Shotgun				UMETA(DisplayName = "Shotgun"),
};




// USTRUCT(BlueprintType)
// struct FFPSWeaponAbilityInfoStruct
// {
// 	GENERATED_USTRUCT_BODY()
//
// 	
// 	
// 	FFPSWeaponAbilityInfoStruct()
// 	{
// 		DamageCurve = nullptr;
// 		ShootSound = nullptr;
// 		ShootCameraShake = nullptr;
// 	}
// };


DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FWeaponAmmoChangedDelegate, int32, OldValue, int32, NewValue);

UCLASS()
class MYPROJECT_API AFPSWeapon : public AActor, public IAbilitySystemInterface
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFPSWeapon();

	// Whether or not to spawn this weapon with collision enabled (pickup mode).
	// Set to false when spawning directly into a player's inventory or true when spawning into the world in pickup mode.
	UPROPERTY(BlueprintReadWrite)
	bool bSpawnWithCollision;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	bool bAttatchRight = true;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, Category = "FPSWeapon|Details")
	FString WeaponName = "NoWeaponName";

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	EWeaponType WeaponType;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	FGameplayTagContainer RestrictedPickupTags;
	
	// This tag is primarily used by the first person Animation Blueprint to determine which animations to play
	// (Rifle vs Rocket Launcher)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	FGameplayTag WeaponTag;


	
	/* inverse proportional to firerate */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	float TimeBetweenAttacks = 0.1f;


	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	float HeadshotMultiplier = 2.f;

	/* e.g. shotguns */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	int NumBulletsPerShot = 1;

	// Base targeting spread (degrees)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot|Spread")
	float BaseSpread;

	// Aiming spread modifier (FinalSpread *= AimingSpreadMod)
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot|Spread")
	float AimingSpreadMod = 1.f;

	// Continuous targeting: spread increment
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot|Spread")
	float TargetingSpreadIncrement;

	// Continuous targeting: max increment
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot|Spread")
	float TargetingSpreadMax;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	UCurveFloat* DamageCurve;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	USoundBase* ShootSound;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Shoot")
	TSubclassOf<UMatineeCameraShake> ShootCameraShake;
	
	/* If false, no tooltip will be generated when hovering this ability */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|ADS")
	float ADSTime = 0.1f;

	/* multiplier of player's current FOV */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|ADS")
	float ADSFOVCoeff = 55/90.f;


	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Recoil")
	FFPSRecoilValue LocationRecoil;
	
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Recoil")
	FFPSRecoilValue RotationRecoil;


	
	// UI HUD Primary Icon when equipped. Using Sprites because of the texture atlas from ShooterGame.

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|UI")
	UTexture2D* WeaponKillFeedIcon;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|UI")
	UTexture2D* PrimaryIcon;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|UI")
	UTexture2D* SecondaryIcon;

	// UI HUD Primary Clip Icon when equipped
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|UI")
	UTexture2D* PrimaryClipIcon;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|UI")
	UTexture2D* SecondaryClipIcon;

	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "FPSWeapon|Details")
	FGameplayTag FireMode;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	FGameplayTag PrimaryAmmoType;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|Details")
	FGameplayTag SecondaryAmmoType;

	// Things like fire mode for rifle
	UPROPERTY(BlueprintReadWrite, VisibleInstanceOnly, Category = "FPSWeapon|Details")
	FText StatusText;

	UPROPERTY(BlueprintAssignable, Category = "FPSWeapon|Details")
	FWeaponAmmoChangedDelegate OnPrimaryClipAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPSWeapon|Details")
	FWeaponAmmoChangedDelegate OnMaxPrimaryClipAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPSWeapon|Details")
	FWeaponAmmoChangedDelegate OnSecondaryClipAmmoChanged;

	UPROPERTY(BlueprintAssignable, Category = "FPSWeapon|Details")
	FWeaponAmmoChangedDelegate OnMaxSecondaryClipAmmoChanged;

	// Implement IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FPSWeapon|Details")
	virtual USkeletalMeshComponent* GetWeaponMesh1P() const { return WeaponMesh1P; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FPSWeapon|Details")
	virtual USkeletalMeshComponent* GetWeaponMesh3P() const { return WeaponMesh3P; }

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	virtual void PreReplication(IRepChangedPropertyTracker& ChangedPropertyTracker) override;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void SetOwningCharacter(AMyProjectCharacter* Character);
	
	// Called when the player equips this weapon (this is different from granting the weapon to player)
	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void Equip();

	// Called when the player unequips this weapon
	virtual void UnEquip();

	virtual void AddAbilities();

	virtual void RemoveAbilities();

	virtual int32 GetAbilityLevel(EAbilityInputID AbilityID);

	// Resets things like fire mode to default
	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void ResetWeapon();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FPSWeapon|Details")
	FVector GetWeaponMesh1PDesiredOffsetFromCamera();

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FPSWeapon|Details")
	FTransform GetWeaponMesh1PEquippedRelativeTransform();
	
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "FPSWeapon|Details")
	FTransform GetWeaponMesh3PEquippedRelativeTransform();

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual int32 GetPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual int32 GetMaxPrimaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual int32 GetSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual int32 GetMaxSecondaryClipAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void SetPrimaryClipAmmo(int32 NewPrimaryClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void SetMaxPrimaryClipAmmo(int32 NewMaxPrimaryClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void SetSecondaryClipAmmo(int32 NewSecondaryClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual void SetMaxSecondaryClipAmmo(int32 NewMaxSecondaryClipAmmo);

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	TSubclassOf<class UUserWidget> GetPrimaryHUDReticleClass() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	virtual bool HasInfiniteAmmo() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Animation")
	UAnimMontage* GetEquip1PMontage() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Animation")
	UAnimMontage* GetEquip3PMontage() const;
	
	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Audio")
	class USoundCue* GetPickupSound() const;

	UFUNCTION(BlueprintCallable, Category = "FPSWeapon|Details")
	FText GetDefaultStatusText() const;

	// Getter for LineTraceTargetActor. Spawns it if it doesn't exist yet.
	UFUNCTION(BlueprintCallable, BlueprintPure ,Category = "FPSWeapon|Targeting")
	AMyGTGA_LineTrace* GetLineTraceTargetActor();

	// TODO: replace with other traces (e.g sphere, cone)
	// Getter for SphereTraceTargetActor. Spawns it if it doesn't exist yet.
	UFUNCTION(BlueprintCallable, BlueprintPure , Category = "FPSWeapon|Targeting")
	AGameplayAbilityTargetActor* GetSphereTraceTargetActor() { return SphereTraceTargetActor; }

	/* when true, we are fully in ads so tell client to do some vfx for visibility */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Ability")
	void OnADSTargetFOVReachedOrLeave(bool IsReached);
	virtual void OnADSTargetFOVReachedOrLeave_Implementation(bool IsReached);
	
	
protected:

	UPROPERTY()
	UFPSAbilitySystemComponent* AbilitySystemComponent;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing = OnRep_PrimaryClipAmmo, Category = "FPSWeapon|Details|Ammo")
	int32 PrimaryClipAmmo;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing = OnRep_MaxPrimaryClipAmmo, Category = "FPSWeapon|Details|Ammo")
	int32 MaxPrimaryClipAmmo;

	// How much ammo in the clip the gun starts with. Used for things like rifle grenades.
	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing = OnRep_SecondaryClipAmmo, Category = "FPSWeapon|Details|Ammo")
	int32 SecondaryClipAmmo;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, ReplicatedUsing = OnRep_MaxSecondaryClipAmmo, Category = "FPSWeapon|Details|Ammo")
	int32 MaxSecondaryClipAmmo;

	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|Details|Ammo")
	bool bInfiniteAmmo;

	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "FPSWeapon|UI")
	TSubclassOf<class UUserWidget> PrimaryHUDReticleClass;

	UPROPERTY()
	AMyGTGA_LineTrace* LineTraceTargetActor;

	// TODO: replace with other traces (e.g sphere, cone)
	UPROPERTY()
	AGameplayAbilityTargetActor* SphereTraceTargetActor;


	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	class UCapsuleComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSWeapon|Details")
	USkeletalMeshComponent* WeaponMesh1P;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPSWeapon|Details")
	USkeletalMeshComponent* WeaponMesh3P;

	/* Relative Desired Offset from Player Camera we want to hold this weapon at */ 
	UPROPERTY(EditDefaultsOnly, Category = "FPSWeapon|Details")
	FVector WeaponMesh1PDesiredOffsetFromCamera;

	// Relative Transform from owner's FPS camera to hold the gun at 
	UPROPERTY(EditDefaultsOnly, Category = "FPSWeapon|Details")
	FTransform WeaponMesh1PEquippedRelativeTransform;

	// Relative Location of weapon 3P Mesh when equipped
	UPROPERTY(EditDefaultsOnly, Category = "FPSWeapon|Details")
	FTransform WeaponMesh3PEquippedRelativeTransform;

	UPROPERTY(BlueprintReadOnly, Replicated, Category = "FPSWeapon|Details")
	AMyProjectCharacter* OwningCharacter;

	UPROPERTY(EditAnywhere, Category = "FPSWeapon|Details")
	TArray<TSubclassOf<UFPSWeaponGameplayAbility>> Abilities;
	
	UPROPERTY(BlueprintReadOnly, Category = "FPSWeapon|Details")
	TArray<FGameplayAbilitySpecHandle> AbilitySpecHandles;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "FPSWeapon|Details")
	FGameplayTag DefaultFireMode;

	// Things like fire mode for rifle
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|Details")
	FText DefaultStatusText;

	/* only useful for guns in single fire that need an animation between shots */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|Animation")
	UAnimMontage* PlayerPumpAnimation;
	
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "FPSWeapon|Animation")
	UAnimMontage* Equip1PMontage;

	UPROPERTY(BlueprintReadonly, EditAnywhere, Category = "FPSWeapon|Animation")
	UAnimMontage* Equip3PMontage;

	// Sound played when player picks it up
	UPROPERTY(EditDefaultsOnly, Category = "FPSWeapon|Audio")
	class USoundCue* PickupSound;

	// Cache tags
	FGameplayTag WeaponPrimaryInstantAbilityTag;
	FGameplayTag WeaponSecondaryInstantAbilityTag;
	FGameplayTag WeaponAlternateInstantAbilityTag;
	FGameplayTag WeaponIsFiringTag;
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UFUNCTION()
	virtual void OnRep_PrimaryClipAmmo(int32 OldPrimaryClipAmmo);

	UFUNCTION()
	virtual void OnRep_MaxPrimaryClipAmmo(int32 OldMaxPrimaryClipAmmo);

	UFUNCTION()
	virtual void OnRep_SecondaryClipAmmo(int32 OldSecondaryClipAmmo);

	UFUNCTION()
	virtual void OnRep_MaxSecondaryClipAmmo(int32 OldMaxSecondaryClipAmmo);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
