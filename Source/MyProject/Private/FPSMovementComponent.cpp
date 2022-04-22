// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "GameFramework/Character.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green,text)

DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);

namespace LyraCharacter
{
	static float GroundTraceDistance = 100000.0f;
	FAutoConsoleVariableRef CVar_GroundTraceDistance(TEXT("LyraCharacter.GroundTraceDistance"), GroundTraceDistance, TEXT("Distance to trace down when generating ground information."), ECVF_Cheat);
}

class FNetworkPredictionData_Client* UFPSMovementComponent::GetPredictionData_Client() const
{
	check(PawnOwner != NULL);
	check(PawnOwner->GetLocalRole() < ROLE_Authority);

	if (!ClientPredictionData)
	{
		UFPSMovementComponent* MutableThis = const_cast<UFPSMovementComponent*>(this);

		MutableThis->ClientPredictionData = new FNetworkPredictionData_Client_MyMovement(*this);
		MutableThis->ClientPredictionData->MaxSmoothNetUpdateDist = 92.f;
		MutableThis->ClientPredictionData->NoSmoothNetUpdateDist = 140.f;
	}

	return ClientPredictionData;
}

UFPSMovementComponent::UFPSMovementComponent(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	
	
}

//============================================================================================
//Replication
//============================================================================================

bool UFPSMovementComponent::ServerSetMoveDirection_Validate(const FVector& MoveDir)
{
	return true;
}

void UFPSMovementComponent::ServerSetMoveDirection_Implementation(const FVector& MoveDir)
{
	MoveDirection = MoveDir;
}

bool UFPSMovementComponent::Server_SetGrappleLeftEndLocation_Validate(const FVector& NewValue)
{
	return true;
}

void UFPSMovementComponent::Server_SetGrappleLeftEndLocation_Implementation(const FVector& NewValue)
{
	GrappleLeftEndLocation = NewValue;
}

bool UFPSMovementComponent::Server_SetGrappleRightEndLocation_Validate(const FVector& NewValue)
{
	return true;
}

void UFPSMovementComponent::Server_SetGrappleRightEndLocation_Implementation(const FVector& NewValue)
{
	GrappleRightEndLocation = NewValue;
}

bool UFPSMovementComponent::IsCustomMovementMode(uint8 custom_movement_mode) const
{
	return MovementMode == EMovementMode::MOVE_Custom && CustomMovementMode == custom_movement_mode;
}

FVector UFPSMovementComponent::GetCharacterMovementInputVector()
{
	FVector ret = FVector(0);

	// find out which way is forward
	const FRotator Rotation = CharacterOwner->GetController()->GetControlRotation();
	const FRotator YawRotation(0, Rotation.Yaw, 0);

	// get forward vector
	FVector Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	float F = CharacterOwner->GetInputAxisValue(FName("MoveForward"));
	print(FString("F: " + FString::SanitizeFloat(F,2)));
	ret += Direction * F;
	
	Direction = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	float R = CharacterOwner->GetInputAxisValue(FName("MoveRight"));
	print(FString("R: " + FString::SanitizeFloat(R,2)));
	ret += Direction * R;

	return ret;
}

const FLyraCharacterGroundInfo& UFPSMovementComponent::GetGroundInfo()
{
	if (!CharacterOwner || (GFrameCounter == CachedGroundInfo.LastUpdateFrame))
	{
		return CachedGroundInfo;
	}

	if (MovementMode == MOVE_Walking)
	{
		CachedGroundInfo.GroundHitResult = CurrentFloor.HitResult;
		CachedGroundInfo.GroundDistance = 0.0f;
	}
	else
	{
		const UCapsuleComponent* CapsuleComp = CharacterOwner->GetCapsuleComponent();
		check(CapsuleComp);

		const float CapsuleHalfHeight = CapsuleComp->GetUnscaledCapsuleHalfHeight();
		const ECollisionChannel CollisionChannel = (UpdatedComponent ? UpdatedComponent->GetCollisionObjectType() : ECC_Pawn);
		const FVector TraceStart(GetActorLocation());
		const FVector TraceEnd(TraceStart.X, TraceStart.Y, (TraceStart.Z - LyraCharacter::GroundTraceDistance - CapsuleHalfHeight));

		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(LyraCharacterMovementComponent_GetGroundInfo), false, CharacterOwner);
		FCollisionResponseParams ResponseParam;
		InitCollisionParams(QueryParams, ResponseParam);

		FHitResult HitResult;
		GetWorld()->LineTraceSingleByChannel(HitResult, TraceStart, TraceEnd, CollisionChannel, QueryParams, ResponseParam);

		CachedGroundInfo.GroundHitResult = HitResult;
		CachedGroundInfo.GroundDistance = LyraCharacter::GroundTraceDistance;

		if (MovementMode == MOVE_NavWalking)
		{
			CachedGroundInfo.GroundDistance = 0.0f;
		}
		else if (HitResult.bBlockingHit)
		{
			CachedGroundInfo.GroundDistance = FMath::Max((HitResult.Distance - CapsuleHalfHeight), 0.0f);
		}
	}

	CachedGroundInfo.LastUpdateFrame = GFrameCounter;

	return CachedGroundInfo;
}

void UFPSMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// We don't want simulated proxies detecting their own collision
	if (GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		// Bind to the OnActorHot component so we're notified when the owning actor hits something (like a wall)
		GetPawnOwner()->OnActorHit.AddDynamic(this, &UFPSMovementComponent::OnActorHit);
	}

	// TODO: organize this better just chache values to restor out of sldiing
	GroundFrictionPreValue = GroundFriction;
	MaxWalkSpeedPreValue = MaxWalkSpeed;
	BrakingDecelerationPreValue = BrakingDecelerationWalking;
}

void UFPSMovementComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	if (GetPawnOwner() != nullptr && GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		// Unbind from all events
		GetPawnOwner()->OnActorHit.RemoveDynamic(this, &UFPSMovementComponent::OnActorHit);
	}
	
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

float UFPSMovementComponent::GetMaxSpeed() const
{
	if(IsCustomMovementMode(CMOVE_WallRunning))
	{
		return MaxWallrunWalkSpeed;
	}
	
	return Super::GetMaxSpeed();
}

bool UFPSMovementComponent::DoJump(bool bReplayingMoves)
{
	if(bDoingPowerSlide || bWantsToPowerSlide)
	{
		StopPowerSlide();
	}
	if(bDoingWallrun)
	{
		// determine if desired input direction is against wall norm to jump away
		FVector JumpDir = GetInputVector();
		if(JumpDir.Equals(FVector(0)))
		{
			JumpDir  = (CharacterOwner->GetControlRotation()).Vector();
		}
		
		if(JumpDir.Dot(WallrunningHit.ImpactNormal) > 0)
		{
			StopWallRun();
		
			FVector JumpVec = FVector(JumpDir.X * WallrunJumpLateralJumpVelocity, JumpDir.Y * WallrunJumpLateralJumpVelocity, WallrunJumpVerticalJumpVelocity);
			Velocity += JumpVec;

			SetMovementMode(MOVE_Falling);
			return true;
		}
		
		// TODO: wallclimbing i think???

		return false;
		
		
		
	}
	else if (bWantsToWallRun && ShouldContinueWallrun())
	{
		BeginWallRun();
	}
	
	return Super::DoJump(bReplayingMoves);
}


void UFPSMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
                                              const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	
	if (!CharacterOwner)
	{
		return;
	}
	
	//Store movement vector
	if (PawnOwner->IsLocallyControlled() || PawnOwner->GetNetMode() == NM_ListenServer)
	{
		//MoveDirection = GetCharacterMovementInputVector();
		MoveDirection = CharacterOwner->GetPendingMovementInputVector();
		
		if(MoveDirection.Equals(FVector(0)))
		{
			MoveDirection = CharacterOwner->GetLastMovementInputVector();
		}
		
	}
	//Send movement vector to server
	if (PawnOwner->GetLocalRole() < ROLE_Authority)
	{
		if(bWantsToGrappleLeft || bWantsToGrappleRight)
			ServerSetMoveDirection(MoveDirection);
	}

	if(bWantsToGrappleLeft || bWantsToGrappleRight)
	{
		PhysUpdateGrappleMovement(DeltaSeconds, 0);
	}
}

void UFPSMovementComponent::PhysCustom(float deltaTime, int32 Iterations)
{
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		return;
	
	switch (CustomMovementMode)
	{
	case ECustomMovementMode::CMOVE_WallRunning:
		{
			if(ShouldContinueWallrun())
			{
				PhysUpdateWallrunMovement(deltaTime, Iterations);
			}
			else
			{
				StopWallRun();
			}
			break;
		}
	}


	Super::PhysCustom(deltaTime, Iterations);
}

void UFPSMovementComponent::PhysFalling(float deltaTime, int32 Iterations)
{
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		return;

	if(bDoingPowerSlide)
	{
		StopPowerSlide();
	}
	
	// if(bWantsToWallRun && !bDoingWallrun)
	// {
	// 	BeginWallRun();
	// }
	// if(bDoingWallrun)
	// {
	// 	if(ShouldContinueWallrun())
	// 	{
	// 		PhysUpdateWallrunMovement(deltaTime, Iterations);
	// 	}
	// 	else
	// 	{
	// 		StopWallRun();
	// 	}
	// }
	//
	Super::PhysFalling(deltaTime, Iterations);
}

void UFPSMovementComponent::PhysWalking(float deltaTime, int32 Iterations)
{
	if (GetOwner()->GetLocalRole() == ROLE_SimulatedProxy)
		return;

	if(bWantsToPowerSlide && !bDoingPowerSlide)
	{
		InitiatePowerSlide(deltaTime);
		bWantsToPowerSlide = false;
	}
	else if(bDoingPowerSlide)
	{
		PhysUpdatePowerSlide(deltaTime, Iterations);
	}
	
	// test if wants transition (currently at limit of walkangle), do this by performing linetrace towards floor
	if(CurrentFloor.HitResult.Component.IsValid() && CurrentFloor.HitResult.Component.Get()->GetCollisionObjectType() == ECC_GameTraceChannel2)
	{
		UPrimitiveComponent* OtherComp = CurrentFloor.HitResult.Component.Get();
		if(!(bWantsToWallRun || bDoingWallrun) && OtherComp)
		{
			/// ECC_GameTraceChannel2 == "Wallrunnable", Check Config/DefaultEngine.ini to ensure
			if(OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel2)
			{
				if(IsValidWallrunAngle(CurrentFloor.HitResult.ImpactNormal))
				{
					WallrunningComponent = OtherComp;
					WallrunningHit = CurrentFloor.HitResult;
					
					FindWallRunDirection(CurrentFloor.HitResult.ImpactNormal);
					BeginWallRun();

					if(bDoingPowerSlide)
					{
						StopPowerSlide();
					}
				}
			}
		}
	}
	
	Super::PhysWalking(deltaTime, Iterations);
}

void UFPSMovementComponent::ProcessLanded(const FHitResult& Hit, float remainingTime, int32 Iterations)
{
	Super::ProcessLanded(Hit, remainingTime, Iterations);

	if(bDoingWallrun && !IsValidWallrunAngle(Hit.ImpactNormal))
	{
		StopWallRun();
	}
}

bool UFPSMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsCustomMovementMode(CMOVE_WallRunning);
}



void UFPSMovementComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse,
                                       const FHitResult& Hit)
{
	// Test for entry into wallrun
	UPrimitiveComponent* OtherComp = Hit.Component.Get();
	if(!(bWantsToWallRun || bDoingWallrun) && OtherComp && !bDoingPowerSlide && IsFalling())
	{
		/// ECC_GameTraceChannel2 == "Wallrunnable", Check Config/DefaultEngine.ini to ensure
		if(OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel2)
		{
			if(IsValidWallrunAngle(Hit.ImpactNormal))
			{
				WallrunningComponent = OtherComp;
			
				FindWallRunDirection(Hit.ImpactNormal);
				
				BeginWallRun();
			}
		}
	}
}


void UFPSMovementComponent::FindWallRunDirection(const FVector& surface_normal)
{
	FVector crossVector;

	if (FVector2D::DotProduct(FVector2D(surface_normal), FVector2D(GetPawnOwner()->GetActorRightVector())) > 0.0)
	{
		// Right Shoulder is touching the wall
		crossVector = FVector(0.0f, 0.0f, 1.0f);
		WallRunSide = EWallRunSide::RIGHT;
	}
	else
	{
		// Left Shoulder is touching the wall
		crossVector = FVector(0.0f, 0.0f, -1.0f);
		WallRunSide = EWallRunSide::LEFT;	
	}

	// Find the direction parallel to the wall in the direction the player is moving
	WallrunningDir = FVector::CrossProduct(surface_normal, crossVector).GetSafeNormal();
	//Line(GetWorld(), GetOwner()->GetActorLocation() + WallrunTraceOffset, GetOwner()->GetActorLocation() +WallrunTraceOffset+ WallrunningDir * 200.f, FColor::Red, false, -1, 0, 1);

	//TODO: move this to own func or som
	// Gather distance info from wall to colliding point of player's capsule, this is for use later on grip force calc
	FHitResult Hit;
	if(GetWorld()->LineTraceSingleByChannel(Hit, WallrunningHit.ImpactPoint, GetCharacterOwner()->GetActorLocation(), ECollisionChannel::ECC_Pawn))
	{
		if(Hit.GetActor() == GetCharacterOwner())
		{
			WallrunningImpactPoint = Hit.ImpactPoint;
		}
	}
	
}

bool UFPSMovementComponent::IsValidWallrunAngle(const FVector& surface_normal) const
{
	// Check if wall is angled in such a way that is walrunnable	
	FVector normalNoZ = FVector(0.f, 0.f, 1.0f);
	normalNoZ.Normalize();

	// Find the angle of the wall
	float wallAngle = FMath::Acos(FVector::DotProduct(normalNoZ, surface_normal)) * 180 / PI;
	
	// Return true if the wall angle is less than the walkable floor angle	
	return wallAngle >= MinAngleToStartWallrun && wallAngle <= MaxWallrunAngle;
}

bool UFPSMovementComponent::ShouldContinueWallrun() 
{
	// first Find floor and determine that angle is below wallrunning (cases where we just laned on floor or wallrunning wall curves out of valid angle)
	if(Velocity.Z <=0)
	{
		FFindFloorResult Floor;
		FindFloor(UpdatedComponent->GetComponentLocation(), Floor, 1.f);
		if(Floor.bWalkableFloor && !IsValidWallrunAngle(Floor.HitResult.ImpactNormal))
		{
			return false;
		}
		if(Floor.bWalkableFloor && Floor.HitResult.Component.Get() == WallrunningComponent)
		{
			// with valid wallruning data and angles, calculate the wallrunning dir
			WallrunningHit = Floor.HitResult;
			FindWallRunDirection(Floor.HitResult.ImpactNormal);
			return true;
		}
	}
	
	// No walrunnable floor beneath, check sides
	// Performe trace, gather info on distance and determine if our current state with the wall is still valid. Otherwise we will return false
	// and get the hell out of this movestate
	const FVector crossVector = WallRunSide == EWallRunSide::LEFT ? FVector(0.0f, 0.0f, -1.0f) : FVector(0.0f, 0.0f, 1.0f);
	FVector traceStart = GetPawnOwner()->GetActorLocation() + WallrunTraceOffset /*+ (WallrunningDir * 20.0f)*/;
	FVector traceEnd = traceStart + (FVector::CrossProduct(WallrunningDir, crossVector) * MaxWallrunDistance);

	//DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Blue, false, .2f, 0, 4);

	/// Perform linetrace with ECC_GameTraceChannel3 == "WallrunableTrace", Check Config/DefaultEngine.ini to ensure
	/// we dont use 12 or "Wallrunnable" since player's own meshges will block with trace
	FHitResult Hit;
	if(GetWorld()->LineTraceSingleByChannel(Hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel3))
	{
		if(Hit.Component.Get() == WallrunningComponent && IsValidWallrunAngle(Hit.ImpactNormal))
		{
			// with valid wallruning data and angles, calculate the wallrunning dir
			WallrunningHit = Hit;
			FindWallRunDirection(Hit.ImpactNormal);
			return true;
		}
	}

	return false;
}

void UFPSMovementComponent::BeginWallRun()
{
	if(!(IsCustomMovementMode(CMOVE_WallRunning)) /*!bDoingWallrun*/)
	{
		SetMovementMode(EMovementMode::MOVE_Custom, ECustomMovementMode::CMOVE_WallRunning);
		
		bDoingWallrun = true;
		bWantsToWallRun = false;

		// if(Velocity.Z < 0)
		// {
		// 	// TODO: trigger initial resistance to falling
		// }
		// else
		// {
		// 	const FVector BoostDir = (WallRunSide == EWallRunSide::RIGHT) ? (WallrunningDir.Cross(WallrunningHit.ImpactNormal)).GetSafeNormal() : (WallrunningHit.ImpactNormal.Cross(WallrunningDir)).GetSafeNormal();
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + BoostDir * 200.f, FColor::Orange, false, 12.2f, 0, 1);
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + WallrunningDir * 200.f, FColor::Red, false, 12.2f, 0, 1);
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + WallrunningHit.ImpactNormal * 200.f, FColor::Magenta, false, 12.2f, 0, 1);
		//
		//
		// 	//Velocity += BoostDir * WallrunStartBoost;
		// }

	
	
		OnWallrunStarted.Broadcast();
	}
}

void UFPSMovementComponent::StopWallRun()
{
	SetMovementMode(MOVE_Falling);
	
	bDoingWallrun = false;
	bWantsToWallRun = false;
	
	OnWallrunEnded.Broadcast();
}

void UFPSMovementComponent::PhysUpdateWallrunMovement(float DeltaTime, int32 iterations)
{
	/// multiple things to do here
	// 1. TODO: impleent initial friction boost to help stop falls and gravity
	// 2. Apply "Grip" force (TODO: decrease amt at higher wallangles)
	// 3. TODO: APPLY MOVEMENT INPUT??
	// 1.

	// apply friction
	FVector FallAcceleration = GetFallingLateralAcceleration(DeltaTime);
	FallAcceleration.Z = 0.f;
	const FVector OldVelocity = Velocity;
	const float MaxDecel = GetMaxBrakingDeceleration();
	if (!HasAnimRootMotion() && !CurrentRootMotion.HasOverrideVelocity())
	{
		// Compute Velocity
		{
			// Acceleration = FallAcceleration for CalcVelocity(), but we restore it after using it.
			TGuardValue<FVector> RestoreAcceleration(Acceleration, FallAcceleration);
			Acceleration = FVector::VectorPlaneProject(Acceleration, WallrunningHit.ImpactNormal);
			
			//Velocity.Z = 0.f;
			CalcVelocity(DeltaTime, WallrunLateralFriction, false, MaxDecel);
			//Velocity.Z = OldVelocity.Z;
		}
	}

	
	// apply gravity
	FVector Gravity{0.f, 0.f, GetGravityZ()};
	Velocity = NewFallVelocity(Velocity, Gravity * WallrunGravityCoeff, DeltaTime);
		
	if(Velocity.Z < -35.f)
	{
		Velocity.Z += Velocity.Z*-1.8f * DeltaTime;
	}


    // Calc grip force
	//DrawDebugLine(GetWorld(), WallrunningHit.ImpactPoint , WallrunningHit.ImpactPoint+ WallrunningHit.ImpactNormal * 160.f, FColor::Emerald, false, -1, 0, 3);

	
	const float Dist = FMath::Max((WallrunningHit.ImpactPoint - WallrunningImpactPoint).Size() - 40.f, 0.f);

	//DrawDebugSphere(GetWorld(), WallrunningImpactPoint, 10.f, 10, FColor::Cyan);
	//DrawDebugSphere(GetWorld(), WallrunningHit.ImpactPoint, 14.f, 10, FColor::Red);

	if(Dist > 0.f)
	{
		FVector Grip = WallrunningHit.ImpactNormal * -1 * WallrunGripForce * Dist;  //(traceEnd - traceStart).GetSafeNormal();
		//AddForce(Grip);
	
		Grip *= WallrunGripForce * DeltaTime;
		Grip.Z = 0.f;
	
		Velocity += Grip;
	}

	//DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + Velocity * 1.f, FColor::Purple, false, .2f, 0, 1);

	
	const FVector LocationDelta = Velocity * DeltaTime;

	FHitResult Hit;
	float RemainingDeltaTime = DeltaTime;
	FVector RemainingLocationDelta = LocationDelta;
	SafeMoveUpdatedComponent(RemainingLocationDelta, UpdatedComponent->GetComponentQuat(), true, Hit);
	RemainingDeltaTime -= RemainingDeltaTime * Hit.Time;
	if(Hit.IsValidBlockingHit())
	{
		AdjustVelocityFromHit(Hit);
		RemainingLocationDelta = Velocity * RemainingDeltaTime;
		const float SlideDeltaApplied = SlideAlongSurface(RemainingLocationDelta, 1.f, Hit.Normal, Hit, true);
		RemainingDeltaTime -= RemainingDeltaTime * SlideDeltaApplied;
		if (Hit.IsValidBlockingHit())
		{
			AdjustVelocityFromHit(Hit);
		}
	}
	if(Hit.bStartPenetrating)
	{
		
	}
}

void UFPSMovementComponent::DoGrappleLeft(FVector EndLocation)
{
	bWantsToGrappleLeft = true;
	GrappleLeftEndLocation = EndLocation;
	Server_SetGrappleLeftEndLocation(EndLocation);
}

void UFPSMovementComponent::DoGrappleRight(FVector EndLocation)
{
	bWantsToGrappleRight = true;
	GrappleRightEndLocation = EndLocation;
	Server_SetGrappleRightEndLocation(EndLocation);
}

void UFPSMovementComponent::StopGrappleLeft()
{
	bWantsToGrappleLeft = false;
}

void UFPSMovementComponent::StopGrappleRight()
{
	bWantsToGrappleRight = false;
}

void UFPSMovementComponent::PhysUpdateGrappleMovement(float DeltaTime, int32 iterations)
{
	float PlayerPullInfluence = 0.f;
	float PlayerSpringInfluence = 0.f;
		
	// Affect pullstrength through control direction (only when wanting to move)
	FVector InputVector = FVector(0); //MoveDirection;//Character->GetPendingMovementInputVector();
	
	InputVector.Z = 0.f;
	InputVector = InputVector.GetSafeNormal();

	// Calc user input influence values
	if(InputVector.Size() > 0.f)
	{
		const FVector ControlRot = CharacterOwner->GetControlRotation().Vector(); 
		const float Angle = FMath::Acos(FVector::DotProduct(InputVector , ControlRot )/ InputVector.Size()/ ControlRot.Size());
		if(Angle > PI / 2)
		{
			InputVector.Z -= CharacterOwner->GetControlRotation().Vector().Z;
			PlayerPullInfluence = PlayerBackwardPullInfluence;
			PlayerSpringInfluence = PlayerBackwardSpringInfluence;
		}
		else
		{
			InputVector.Z += CharacterOwner->GetControlRotation().Vector().Z; // add camera influence
			PlayerPullInfluence = PlayerForwardPullInfluence;
			PlayerSpringInfluence = PlayerForwardSpringInfluence;
		}

		InputVector = InputVector.GetSafeNormal();
		
		//DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + (InputVector *200.f), FColor::Purple, false, 5.f, 0.f, 1.f);
	}

	// Gather Active Grapple ends
    TArray<FVector> GrapplesToQuery;
	if(bWantsToGrappleLeft)
	{
		GrapplesToQuery.Push(GrappleLeftEndLocation);
	}
	if(bWantsToGrappleRight)
	{
		GrapplesToQuery.Push(GrappleRightEndLocation);
	}

	// Iterate over active grapples to find effective force to add at end
	FVector TotalForce = FVector(0.f);
	for(const FVector End : GrapplesToQuery)
	{
		const FVector Dist = (End - GetOwner()->GetActorLocation()/*ODMComponent->GetSocketLocation(FName("GrappleSocket"))*/);
	
		FVector Vel = Velocity;
		SpeedInPullDirection = FMath::Clamp(Vel.Size() * FVector::DotProduct(Dist.GetSafeNormal(), Vel.GetSafeNormal()), 0.f, 99999999.f);
		
		FVector Force = Dist.GetSafeNormal() * (DeltaTime * PullStrengthSpeedDirection->GetFloatValue(SpeedInPullDirection));
		Force += InputVector.ProjectOnTo(Force) * Force.Size() * PlayerPullInfluence;
				
		const float Angle = FMath::Acos(FVector::DotProduct(Vel , Force )/ Vel.Size()/ Force.Size());
		if(Angle > PI / 2)
		{
			// TODO: formula is k * dt * v / m = dv/dt but velocity projecting along spring direction doesn't have its magnitude correct due to normals.
			FVector SpringForce = Vel.GetSafeNormal().ProjectOnTo(Dist.GetSafeNormal()) * DeltaTime * SpringFactor * Vel.Size() * -1.f;
			FVector Add = InputVector.ProjectOnTo(SpringForce) * SpringForce.Size() * PlayerSpringInfluence;
			SpringForce +=  InputVector.ProjectOnTo(Force) * SpringForce.Size() * PlayerSpringInfluence;
		
			TotalForce += SpringForce;
		}

		TotalForce += Force;
	}

	// mainly for sliding to stop them from being lifted off the ground 
	if(bDoingPowerSlide)
		TotalForce.Z = 0.f;
	
	Velocity += TotalForce;
}

void UFPSMovementComponent::DoPowerSlide()
{
	bWantsToPowerSlide = true;

	OnPowerSlideStarted.Broadcast();
}

void UFPSMovementComponent::StopPowerSlide()
{
	bDoingPowerSlide = false;
	MaxWalkSpeed = MaxWalkSpeedPreValue;
	GroundFriction = GroundFrictionPreValue;
	BrakingDecelerationWalking = BrakingDecelerationPreValue;

	OnPowerSlideEnded.Broadcast();
}

void UFPSMovementComponent::InitiatePowerSlide(float DeltaTime)
{	
	MaxWalkSpeed = 0.f;
	GroundFriction = 0.2f;
	BrakingDecelerationWalking = SlidingBrakingDecelerationWalking;
	
	
	/*FVector Boost = FVector(Velocity.Normalize()) * SpeedBoostAmount;
	AddImpulse(Boost, false);*/

	float Diff = SpeedBoostCap - Velocity.Size();
	if(Diff > 0)
	{
		float Boost = FMath::Min(800.f, Diff);
		Velocity += FVector(Velocity.GetSafeNormal()) * Boost;
	}

	bDoingPowerSlide = true;
}

void UFPSMovementComponent::PhysUpdatePowerSlide(float deltaTime, int32 Iterations)
{
	if(Velocity.Size2D() < SpeedToStopSlide)
	{
		StopPowerSlide();
	}
}

void UFPSMovementComponent::AdjustVelocityFromHit(const FHitResult& Hit)
{
	Velocity = FVector::VectorPlaneProject(Velocity, Hit.ImpactNormal);
}

FVector UFPSMovementComponent::GetInputVector()
{
	return (GetPendingInputVector() == FVector(0)) ? GetLastInputVector() : GetPendingInputVector();
}


//Set input flags on character from saved inputs
void UFPSMovementComponent::UpdateFromCompressedFlags(uint8 Flags)//Client only
{
	Super::UpdateFromCompressedFlags(Flags);

	
	//The Flags parameter contains the compressed input flags that are stored in the saved move.
	//UpdateFromCompressed flags simply copies the flags from the saved move into the movement component.
	//It basically just resets the movement component to the state when the move was made so it can simulate from there.
	//int32 DodgeFlags = (Flags >> 2) & 7;

	/* There are 4 custom move flags for us to use. Below is what each is currently being used for:
	FLAG_Custom_0		= 0x10, // ODMR
	FLAG_Custom_1		= 0x20, // ODML
	FLAG_Custom_2		= 0x40, // Powerslide TODO: move this to trigger off crouch flag if needed
	FLAG_Custom_3		= 0x80, // Wall Run
	*/

	bWantsToGrappleRight =  (Flags & FSavedMove_Character::FLAG_Custom_0) != 0;
	bWantsToGrappleLeft =  (Flags & FSavedMove_Character::FLAG_Custom_1) != 0;
	bWantsToPowerSlide =  (Flags & FSavedMove_Character::FLAG_Custom_2) != 0;
	bWantsToWallRun =  (Flags & FSavedMove_Character::FLAG_Custom_3) != 0;
	//bWantsToMantle = (DodgeFlags == 5);
}


void FSavedMove_MyMovement::Clear()
{
	Super::Clear();
	
	SavedMoveDirection = FVector::ZeroVector;
	
	bSavedWantsToGrappleLeft = 0;
	bSavedWantsToGrappleRight = 0;
	bSavedWantsToPowerSlide = 0;
	bSavedWantsToWallRun = 0;
	//bSavedWantsToMantle = false;
}

uint8 FSavedMove_MyMovement::GetCompressedFlags() const
{
	// Result # 1 and 2 taked by jump and crouch respectively
	uint8 Result = Super::GetCompressedFlags();

	/* There are 4 custom move flags for us to use. Below is what each is currently being used for:
	FLAG_Custom_0		= 0x10, // ODMR
	FLAG_Custom_1		= 0x20, // ODML
	FLAG_Custom_2		= 0x40, // Powerslide TODO: move this to trigger off crouch flag if needed
	FLAG_Custom_3		= 0x80, // Wall Run
	*/
	
	if (bSavedWantsToGrappleRight)
	{
		//Result |= (1 << 2);
		Result |= FLAG_Custom_0;
	}
	if (bSavedWantsToGrappleLeft)
	{
		//Result |= (2 << 2);
		Result |= FLAG_Custom_1;
	}
	if (bSavedWantsToPowerSlide)
	{
		//Result |= (3 << 2);
		Result |= FLAG_Custom_2;
	}
	if (bSavedWantsToWallRun)
	{
		//Result |= (4 << 2);
		Result |= FLAG_Custom_3;
	}
	/*else if (bSavedWantsToMantle)
	{
		Result |= (5 << 2);
	}*/

	return Result;
}

bool FSavedMove_MyMovement::CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* Character, float MaxDelta) const
{
	//This pretty much just tells the engine if it can optimize by combining saved moves. There doesn't appear to be
	//any problem with leaving it out, but it seems that it's good practice to implement this anyways.
	if (bSavedWantsToGrappleRight != ((FSavedMove_MyMovement*)&NewMove)->bSavedWantsToGrappleRight)
	{
		return false;
	}

	if (bSavedWantsToGrappleLeft != ((FSavedMove_MyMovement*)&NewMove)->bSavedWantsToGrappleLeft)
	{
		return false;
	}

	if (bSavedWantsToPowerSlide != ((FSavedMove_MyMovement*)&NewMove)->bSavedWantsToPowerSlide)
	{
		return false;
	}
	if (bSavedWantsToWallRun != ((FSavedMove_MyMovement*)&NewMove)->bSavedWantsToWallRun)
	{
		return false;
	}

	if (SavedMoveDirection != ((FSavedMove_MyMovement*)&NewMove)->SavedMoveDirection)
	{
		return false;
	}
	
	return Super::CanCombineWith(NewMove, Character, MaxDelta);
}

void FSavedMove_MyMovement::SetMoveFor(ACharacter* Character, float InDeltaTime, FVector const& NewAccel, class FNetworkPredictionData_Client_Character & ClientData)
{
	Super::SetMoveFor(Character, InDeltaTime, NewAccel, ClientData);

	UFPSMovementComponent* CharMov = Cast<UFPSMovementComponent>(Character->GetCharacterMovement());
	if (CharMov)
	{
		//This is literally just the exact opposite of UpdateFromCompressed flags. We're taking the input
		//from the player and storing it in the saved move.

		bSavedWantsToGrappleLeft = CharMov->bWantsToGrappleLeft;
		bSavedWantsToGrappleRight = CharMov->bWantsToGrappleRight;
		bSavedWantsToPowerSlide = CharMov->bWantsToPowerSlide;
		bSavedWantsToWallRun = CharMov->bWantsToWallRun;
		//bSavedWantsToMantle = CharMov->bWantsToMantle;
		
		// Taking player movement component state and storing it for later
		SavedMoveDirection = CharMov->MoveDirection;
		
	}

	// Round acceleration, so sent version and locally used version always match
	Acceleration.X = FMath::RoundToFloat(Acceleration.X);
	Acceleration.Y = FMath::RoundToFloat(Acceleration.Y);
	Acceleration.Z = FMath::RoundToFloat(Acceleration.Z);
}

void FSavedMove_MyMovement::PrepMoveFor(class ACharacter* Character)
{
	Super::PrepMoveFor(Character);

	UFPSMovementComponent* CharMov = Cast<UFPSMovementComponent>(Character->GetCharacterMovement());
	if (CharMov)
	{
		//This is just the exact opposite of SetMoveFor. It copies the state from the saved move to the movement
		//component before a correction is made to a client.
		CharMov->MoveDirection = SavedMoveDirection;

		CharMov->bWantsToGrappleLeft = bSavedWantsToGrappleLeft;
		CharMov->bWantsToGrappleRight = bSavedWantsToGrappleRight;
		CharMov->bWantsToPowerSlide = bSavedWantsToPowerSlide;
		CharMov->bWantsToWallRun = bSavedWantsToWallRun;
		//CharMov->bWantsToMantle = bSavedWantsToMantle;
		//Don't update flags here. They're automatically setup before corrections using the compressed flag methods.
	}
}

FNetworkPredictionData_Client_MyMovement::FNetworkPredictionData_Client_MyMovement(const UCharacterMovementComponent& ClientMovement)
: Super(ClientMovement)
{

}

FSavedMovePtr FNetworkPredictionData_Client_MyMovement::AllocateNewMove()
{
	return FSavedMovePtr(new FSavedMove_MyMovement());
}