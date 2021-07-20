// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CableComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

#include "ODMGrapple.generated.h"

UCLASS()
class MYPROJECT_API AODMGrapple : public AActor
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> GrappleHead;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	UCableComponent* Cable;
	
	// UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Abilities, meta = (AllowPrivateAccess = "true"))
	// UProjectileMovementComponent* ProjectileMovement;

	
public:	
	// Sets default values for this actor's properties
	AODMGrapple();


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Replicated, meta = (ExposeOnSpawn = true))
	USceneComponent* ODMComponent;
	
protected:
\
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float CableSegmentsDelay = 0.2f; // Pullstrength += SPringFactor/(offsetdist)
	
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float InitProjectileVelocity = 5000.f;

	/* Constant pulling force regardless of spring */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PullStrength = 100.f;  //math later, fuyncitonality first

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float SpringFactor = 1.f; // Pullstrength += SPringFactor/(offsetdist)

	/* controls how much camera affects over movement input */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PlayerControlRotationInfluence = 1.f;

	/* InputVector.ProjectOnTo(PullForce) * PullStrength * PlayerPullInfluence */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PlayerForwardPullInfluence = .8f;

	/* InputVector.ProjectOnTo(PullForce) * PullStrength * PlayerPullInfluence */
	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PlayerBackwardPullInfluence = .8f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PlayerForwardSpringInfluence = 0.f;

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly, meta = (AllowPrivateAccess = "true"))
	float PlayerBackwardSpringInfluence = 0.f;
	
	UPROPERTY()
	bool DoGrapplePull = true;
	

	UPROPERTY()
	ACharacter* Character;
	
	void OnGrappleHeadOverlap();


	FTimerHandle BeginPlayTimerHandle;
	void OnDelayedCableSegmentReduce();
	
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	
	
	
public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

};
