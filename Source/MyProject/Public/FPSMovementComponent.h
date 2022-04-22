// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/EngineTypes.h"
#include "FPSMovementComponent.generated.h"


/** Custom movement modes for Characters. */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	CMOVE_WallRunning   UMETA(DisplayName = "WallRunning"),
	CMOVE_MAX			UMETA(Hidden),
};

// name designates the shoulder that is closest to the wall, if somehow we are fully backfaced then it will be left
UENUM(BlueprintType)
enum class EWallRunSide : uint8
{
	LEFT 	UMETA(DisplayName = "Left", ToolTip = "Left shoulder facing wall"),
	RIGHT	UMETA(DisplayName = "Right", ToolTip = "Right shoulder facing wall"),
};

USTRUCT(BlueprintType)
struct FLyraCharacterGroundInfo
{
	GENERATED_BODY()

	FLyraCharacterGroundInfo()
		: LastUpdateFrame(0)
		, GroundDistance(0.0f)
	{}

	uint64 LastUpdateFrame;

	UPROPERTY(BlueprintReadOnly)
	FHitResult GroundHitResult;

	UPROPERTY(BlueprintReadOnly)
	float GroundDistance;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementPowerSlide);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMovementWallrun);


/**
 * https://nerivec.github.io/old-ue4-wiki/pages/authoritative-networked-character-movement.html#Boost_Dodge
 *
 *  code used from above UT
 */
UCLASS()
class MYPROJECT_API UFPSMovementComponent : public UCharacterMovementComponent
{
	GENERATED_UCLASS_BODY()

	//============================================================================================
	//Replication
	//============================================================================================

public:

	friend class FSavedMove_ExtendedMyMovement;

	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	virtual class FNetworkPredictionData_Client* GetPredictionData_Client() const override;
	
	
	UFUNCTION(Unreliable, Server, WithValidation)
	void ServerSetMoveDirection(const FVector& MoveDir);

	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetGrappleLeftEndLocation(const FVector& NewValue);
	UFUNCTION(Server, Reliable, WithValidation)
	void Server_SetGrappleRightEndLocation(const FVector& NewValue);

	UPROPERTY(BlueprintAssignable)
	FMovementPowerSlide OnPowerSlideEnded;
	UPROPERTY(BlueprintAssignable)
	FMovementPowerSlide OnPowerSlideStarted;
	UPROPERTY(BlueprintAssignable)
	FMovementWallrun OnWallrunEnded;
	UPROPERTY(BlueprintAssignable)
	FMovementWallrun OnWallrunStarted;

	FVector GetCharacterMovementInputVector();

	UFUNCTION(BlueprintCallable, Category = "Lyra|CharacterMovement")
	const FLyraCharacterGroundInfo& GetGroundInfo();


	bool IsCustomMovementMode(uint8 custom_movement_mode) const;

#pragma region Overrides
protected:
	virtual void BeginPlay() override;
	virtual void OnComponentDestroyed(bool bDestroyingHierarchy) override;
public:
	virtual float GetMaxSpeed() const override;
	virtual bool DoJump(bool bReplayingMoves) override;
	// copy of override without maxwalkanglereference
	// virtual bool IsWalkable(const FHitResult& Hit) const override;



	///@brief Event triggered at the end of a movement update
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) override;
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;
	virtual void PhysFalling(float deltaTime, int32 Iterations) override;
	virtual void PhysWalking(float deltaTime, int32 Iterations) override;
	virtual void ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations) override;


protected:
	virtual bool CanAttemptJump() const override;


private:
	// Called when the owning actor hits something (to begin the wall run)
	UFUNCTION()
	void OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse, const FHitResult& Hit);
	
#pragma endregion

public:
	
#pragma region Wallrun Functions
	
	// Finds the wall run direction and side based on the specified surface normal
	void FindWallRunDirection(const FVector& surface_normal);
	bool IsValidWallrunAngle(const FVector& surface_normal) const;
	bool ShouldContinueWallrun();

	UFUNCTION(BlueprintCallable, Category = "Client")
	void BeginWallRun();
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopWallRun();

	UFUNCTION(BlueprintCallable, Category = "Client", BlueprintPure)
	const FVector& GetWallrunningSurfaceNormal() const { return WallrunningHit.ImpactNormal;}
	
	UFUNCTION()
	void PhysUpdateWallrunMovement(float DeltaTime, int32 iterations);
	
#pragma endregion
#pragma region Grapple Functions

	UFUNCTION(BlueprintCallable, Category = "Client")
	void DoGrappleLeft(FVector EndLocation);
	UFUNCTION(BlueprintCallable, Category = "Client")
	void DoGrappleRight(FVector EndLocation);
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopGrappleLeft();
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopGrappleRight();
	UFUNCTION()
	void PhysUpdateGrappleMovement(float DeltaTime, int32 iterations);
	
#pragma endregion
#pragma region Powerslide Functions
	
	UFUNCTION(BlueprintCallable, Category = "Client")
void DoPowerSlide();
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopPowerSlide();
	UFUNCTION(Category = "Client")
	void InitiatePowerSlide(float DeltaTime);
	UFUNCTION()
	void PhysUpdatePowerSlide(float deltaTime, int32 Iterations);
	// Pullstrength += SPringFactor/(offsetdist)
	
#pragma endregion
	
	/* Current used as TODO MetaSound Parameter to affect pulling sound */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float SpeedInPullDirection;
	
	///@brief Data to save
	FVector MoveDirection;
	
	///@brief Flag for activating sprint.
	uint8 bWantsToGrappleRight : 1;
	uint8 bWantsToGrappleLeft : 1;
	uint8 bWantsToPowerSlide : 1;
	uint8 bWantsToWallRun : 1;
	//uint8 bWantsToMantle : 1;


	
private:

	void AdjustVelocityFromHit(const FHitResult& Hit);

	FVector GetInputVector();

	// these dont need replication as grapple end location data should already be accurate
	FVector GrappleLeftEndLocation;
	FVector GrappleRightEndLocation;

#pragma region Wallrun
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunGravityCoeff = 0.8f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunLateralFriction = 10.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float MaxWallrunWalkSpeed = 1500.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunJumpLateralJumpVelocity = 600.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunJumpVerticalJumpVelocity = 900.f;
	
	// Boost to give to players when starting wallrun. ONLY GETS APPLIED WHEN Velocity. Direction is Cross(wallrunDir, WallrunningSurfaceNormal)
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunStartBoost = 800.f;

	// not similar to MaxWallrunAngle, instead this handles transition for walking into wallrunning in curved walls 
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float MinAngleToStartWallrun = 40.f;
	
	// max angle to wallrun, total angle to do wallruns is [MaxWallrunAngle, WalkableFloorAngle - MinAngleToStartWallrun)
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float MaxWallrunAngle = 107.f;

	// kinda pseudo value to tune how hard we latch onto walls, reall only matters for wallrunning on outsude of curved walls
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float WallrunGripForce = 600.f;

	// Length of trace to perform every tick to find the wall, no hits = exit wallrun
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	float MaxWallrunDistance = 100.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Wallrun")
	FVector WallrunTraceOffset = FVector(0.f, 0.f, 50.f);

	FHitResult WallrunningHit;
	FVector WallrunningDir;
	FVector WallrunningImpactPoint;  //Point of closest contact in world space
	EWallRunSide WallRunSide;	

	// if bDoingWallrun then this holds actor referenced as "wall" we are running on
	UPROPERTY()
	UPrimitiveComponent* WallrunningComponent = nullptr;

	bool bDoingWallrun;
	
#pragma endregion

#pragma region Grapple
	/* Constant pulling force regardless of spring */
	// UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	// float PullStrength = 100.f;  //math later, fuyncitonality first
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	float SpringFactor = 1.f; // Pullstrength += SPringFactor/(offsetdist)

	/* Once we move fast enough in pulling direction (projected) then cut down/stop pulling more */
	UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	UCurveFloat* PullStrengthSpeedDirection;

	
	/* InputVector.ProjectOnTo(PullForce) * PullStrength * PlayerPullInfluence */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	float PlayerForwardPullInfluence = .8f;

	/* InputVector.ProjectOnTo(PullForce) * PullStrength * PlayerPullInfluence */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	float PlayerBackwardPullInfluence = .8f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	float PlayerForwardSpringInfluence = 0.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "Grapple")
	float PlayerBackwardSpringInfluence = 0.f;

#pragma endregion

#pragma region PowerSlide
	
	/// @brief Powerslide variable sluff
	float GroundFrictionPreValue;
	float MaxWalkSpeedPreValue;
	float BrakingDecelerationPreValue;
	
	// 
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedToStopSlide = 0.8f;

	// 
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedBoostCap = 1.4f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SlidingBrakingDecelerationWalking = 800.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedBoostAmount = 8000.f;

	bool bDoingPowerSlide;
#pragma endregion

protected:
	
	FLyraCharacterGroundInfo CachedGroundInfo;
};

class FSavedMove_MyMovement : public FSavedMove_Character
{
public:

	typedef FSavedMove_Character Super;

	///@brief Resets all saved variables.
	virtual void Clear() override;

	///@brief Store input commands in the compressed flags.
	virtual uint8 GetCompressedFlags() const override;

	///@brief This is used to check whether or not two moves can be combined into one.
	///Basically you just check to make sure that the saved variables are the same.
	virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const override;

	///@brief Sets up the move before sending it to the server. 
	virtual void SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData) override;

	///@brief Sets variables on character movement component before making a predictive correction.
	virtual void PrepMoveFor(class ACharacter* Character) override;

	///@brief Data to save
	FVector SavedMoveDirection;
	
	///@brief Flag for activating sprint.
	uint8 bSavedWantsToGrappleRight : 1;
	uint8 bSavedWantsToGrappleLeft : 1;
	uint8 bSavedWantsToPowerSlide : 1;
	uint8 bSavedWantsToWallRun : 1;
	//bool bSavedWantsToMantle;
};

class FNetworkPredictionData_Client_MyMovement : public FNetworkPredictionData_Client_Character
{
public:
	typedef FNetworkPredictionData_Client_Character Super;
	
	FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent& ClientMovement);

	///@brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
	
};
