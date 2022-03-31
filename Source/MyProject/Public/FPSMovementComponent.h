// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "FPSMovementComponent.generated.h"

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
	
	///@brief Event triggered at the end of a movement update
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector & OldLocation, const FVector & OldVelocity) override;
	
	
	///@brief Triggers the dodge action.
	UFUNCTION(BlueprintCallable, Category = "Client")
	void DoGrappleLeft(FVector EndLocation);
	UFUNCTION(BlueprintCallable, Category = "Client")
	void DoGrappleRight(FVector EndLocation);
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopGrappleLeft();
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopGrappleRight();
	UFUNCTION(Category = "Client")
	void UpdateGrappleMovement(float DeltaTime, bool IsLeft);
	UFUNCTION(BlueprintCallable, Category = "Client")
	void DoPowerSlide();
	UFUNCTION(BlueprintCallable, Category = "Client")
	void StopPowerSlide();
	UFUNCTION(Category = "Client")
	void InitiatePowerSlide(float DeltaTime);
	UFUNCTION()
	void UpdatePowerSlide();
	// Pullstrength += SPringFactor/(offsetdist)
	
	/* Current used as MetaSound Parameter to affect pulling sound */
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	float SpeedInPullDirection;
	
	///@brief Data to save
	FVector MoveDirection;
	
	///@brief Flag for activating sprint.
	bool bWantsToGrappleRight;
	bool bWantsToGrappleLeft;
	bool bWantsToPowerSlide;
	bool bWantsToWallRun;
	bool bWantsToMantle;


	
private:

	// these dont need replication as grapple end location data should already be accurate
	FVector GrappleLeftEndLocation;
	FVector GrappleRightEndLocation;

	FTimerHandle PowerslideTimerHandle;
	FTimerDelegate PowerslideTimerDel;

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

	/// @brief Powerslide variable sluff
	float GroundFrictionPreValue;
	float MaxWalkSpeedPreValue;

	// Ratio of normal walkingspeed to stop sliding
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedToStopSlide = 0.8f;

	// Ratio of normal walkingspeed to cap boost from sliding
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedBoostCap = 1.4f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SlidingBrakingDecelerationWalking = 800.f;
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"), Category = "PowerSlide")
	float SpeedBoostAmount = 8000.f;
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
	bool bSavedWantsToGrappleRight;
	bool bSavedWantsToGrappleLeft;
	bool bSavedWantsToPowerSlide;
	bool bSavedWantsToWallRun;
	bool bSavedWantsToMantle;
};

class FNetworkPredictionData_Client_MyMovement : public FNetworkPredictionData_Client_Character
{
public:
	FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent& ClientMovement);

	typedef FNetworkPredictionData_Client_Character Super;

	///@brief Allocates a new copy of our custom saved move
	virtual FSavedMovePtr AllocateNewMove() override;
	
};
