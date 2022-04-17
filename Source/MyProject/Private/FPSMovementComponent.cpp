// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSMovementComponent.h"
#include "Components/CapsuleComponent.h"

#include "GameFramework/Character.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2, FColor::Green,text)

DECLARE_CYCLE_STAT(TEXT("Char StepUp"), STAT_CharStepUp, STATGROUP_Character);

namespace CharacterMovementConstants
{
	// MAGIC NUMBERS
	const float MAX_STEP_SIDE_Z = 0.08f;	// maximum z value for the normal on the vertical side of steps
	const float SWIMBOBSPEED = -80.f;
	const float VERTICAL_SLOPE_NORMAL_Z = 0.001f; // Slope is vertical if Abs(Normal.Z) <= this threshold. Accounts for precision problems that sometimes angle normals slightly off horizontal for vertical surface.
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

void UFPSMovementComponent::BeginPlay()
{
	Super::BeginPlay();

	// We don't want simulated proxies detecting their own collision
	if (GetPawnOwner()->GetLocalRole() > ROLE_SimulatedProxy)
	{
		// Bind to the OnActorHot component so we're notified when the owning actor hits something (like a wall)
		GetPawnOwner()->OnActorHit.AddDynamic(this, &UFPSMovementComponent::OnActorHit);
	}
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
	if(bDoingPowerSlide)
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
		
		if(JumpDir.Dot(WallrunningSurfaceNormal) > 0)
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

bool UFPSMovementComponent::StepUp(const FVector& GravDir, const FVector& Delta, const FHitResult& InHit,
	FStepDownResult* OutStepDownResult)
{

	SCOPE_CYCLE_COUNTER(STAT_CharStepUp);

	if (!CanStepUp(InHit) || MaxStepHeight <= 0.f)
	{
		return false;
	}

	const FVector OldLocation = UpdatedComponent->GetComponentLocation();
	float PawnRadius, PawnHalfHeight;
	CharacterOwner->GetCapsuleComponent()->GetScaledCapsuleSize(PawnRadius, PawnHalfHeight);

	// Don't bother stepping up if top of capsule is hitting something.
	const float InitialImpactZ = InHit.ImpactPoint.Z;
	if (InitialImpactZ > OldLocation.Z + (PawnHalfHeight - PawnRadius))
	{
		return false;
	}

	if (GravDir.IsZero())
	{
		return false;
	}

	// Gravity should be a normalized direction
	ensure(GravDir.IsNormalized());

	float StepTravelUpHeight = MaxStepHeight;
	float StepTravelDownHeight = StepTravelUpHeight;
	const float StepSideZ = -1.f * FVector::DotProduct(InHit.ImpactNormal, GravDir);
	float PawnInitialFloorBaseZ = OldLocation.Z - PawnHalfHeight;
	float PawnFloorPointZ = PawnInitialFloorBaseZ;

	if (IsMovingOnGround() && CurrentFloor.IsWalkableFloor())
	{
		// Since we float a variable amount off the floor, we need to enforce max step height off the actual point of impact with the floor.
		const float FloorDist = FMath::Max(0.f, CurrentFloor.GetDistanceToFloor());
		PawnInitialFloorBaseZ -= FloorDist;
		StepTravelUpHeight = FMath::Max(StepTravelUpHeight - FloorDist, 0.f);
		StepTravelDownHeight = (MaxStepHeight + MAX_FLOOR_DIST*2.f);

		const bool bHitVerticalFace = !IsWithinEdgeTolerance(InHit.Location, InHit.ImpactPoint, PawnRadius);
		if (!CurrentFloor.bLineTrace && !bHitVerticalFace)
		{
			PawnFloorPointZ = CurrentFloor.HitResult.ImpactPoint.Z;
		}
		else
		{
			// Base floor point is the base of the capsule moved down by how far we are hovering over the surface we are hitting.
			PawnFloorPointZ -= CurrentFloor.FloorDist;
		}
	}

	// Don't step up if the impact is below us, accounting for distance from floor.
	if (InitialImpactZ <= PawnInitialFloorBaseZ)
	{
		return false;
	}

	// Scope our movement updates, and do not apply them until all intermediate moves are completed.
	FScopedMovementUpdate ScopedStepUpMovement(UpdatedComponent, EScopedUpdate::DeferredUpdates);

	// step up - treat as vertical wall
	FHitResult SweepUpHit(1.f);
	const FQuat PawnRotation = UpdatedComponent->GetComponentQuat();
	MoveUpdatedComponent(-GravDir * StepTravelUpHeight, PawnRotation, true, &SweepUpHit);

	if (SweepUpHit.bStartPenetrating)
	{
		// Undo movement
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	// step fwd
	FHitResult Hit(1.f);
	MoveUpdatedComponent( Delta, PawnRotation, true, &Hit);

	// Check result of forward movement
	if (Hit.bBlockingHit)
	{
		if (Hit.bStartPenetrating)
		{
			// Undo movement
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// If we hit something above us and also something ahead of us, we should notify about the upward hit as well.
		// The forward hit will be handled later (in the bSteppedOver case below).
		// In the case of hitting something above but not forward, we are not blocked from moving so we don't need the notification.
		if (SweepUpHit.bBlockingHit && Hit.bBlockingHit)
		{
			HandleImpact(SweepUpHit);
		}

		// pawn ran into a wall
		HandleImpact(Hit);
		if (IsFalling())
		{
			return true;
		}

		// adjust and try again
		const float ForwardHitTime = Hit.Time;
		const float ForwardSlideAmount = SlideAlongSurface(Delta, 1.f - Hit.Time, Hit.Normal, Hit, true);
		
		if (IsFalling())
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// If both the forward hit and the deflection got us nowhere, there is no point in this step up.
		if (ForwardHitTime == 0.f && ForwardSlideAmount == 0.f)
		{
			ScopedStepUpMovement.RevertMove();
			return false;
		}
	}
	
	// Step down
	MoveUpdatedComponent(GravDir * StepTravelDownHeight, UpdatedComponent->GetComponentQuat(), true, &Hit);

	// If step down was initially penetrating abort the step up
	if (Hit.bStartPenetrating)
	{
		ScopedStepUpMovement.RevertMove();
		return false;
	}

	FStepDownResult StepDownResult;
	if (Hit.IsValidBlockingHit())
	{	
		// See if this step sequence would have allowed us to travel higher than our max step height allows.
		const float DeltaZ = Hit.ImpactPoint.Z - PawnFloorPointZ;
		if (DeltaZ > MaxStepHeight)
		{
			//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (too high Height %.3f) up from floor base %f to %f"), DeltaZ, PawnInitialFloorBaseZ, NewLocation.Z);
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Reject moves where the downward sweep hit something very close to the edge of the capsule. This maintains consistency with FindFloor as well.
		if (!IsWithinEdgeTolerance(Hit.Location, Hit.ImpactPoint, PawnRadius))
		{
			//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (outside edge tolerance)"));
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// Don't step up onto invalid surfaces if traveling higher.
		if (DeltaZ > 0.f && !CanStepUp(Hit))
		{
			//UE_LOG(LogCharacterMovement, VeryVerbose, TEXT("- Reject StepUp (up onto surface with !CanStepUp())"));
			ScopedStepUpMovement.RevertMove();
			return false;
		}

		// See if we can validate the floor as a result of this step down. In almost all cases this should succeed, and we can avoid computing the floor outside this method.
		if (OutStepDownResult != NULL)
		{
			FindFloor(UpdatedComponent->GetComponentLocation(), StepDownResult.FloorResult, false, &Hit);

			// Reject unwalkable normals if we end up higher than our initial height.
			// It's fine to walk down onto an unwalkable surface, don't reject those moves.
			if (Hit.Location.Z > OldLocation.Z)
			{
				// We should reject the floor result if we are trying to step up an actual step where we are not able to perch (this is rare).
				// In those cases we should instead abort the step up and try to slide along the stair.
				if (!StepDownResult.FloorResult.bBlockingHit && StepSideZ < CharacterMovementConstants::MAX_STEP_SIDE_Z)
				{
					ScopedStepUpMovement.RevertMove();
					return false;
				}
			}

			StepDownResult.bComputedFloor = true;
		}
	}
	
	// Copy step down result.
	if (OutStepDownResult != NULL)
	{
		*OutStepDownResult = StepDownResult;
	}

	// Don't recalculate velocity based on this height adjustment, if considering vertical adjustments.
	bJustTeleported |= !bMaintainHorizontalGroundVelocity;

	return true;
	return Super::StepUp(GravDir, Delta, Hit, OutStepDownResult);
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
	//  FHitResult Hit;
	//  const FVector Dir = FVector(0.f, 0.f, -1.f);
	//
	// FindFloor(Hit);
	//  if(GetWorld()->LineTraceSingleByChannel(Hit, GetActorLocation() + FVector(0,0,4), GetActorLocation() + Dir * 200, ECollisionChannel::ECC_GameTraceChannel3))
	//  {
	//  	UPrimitiveComponent* OtherComp = Hit.Component.Get();
	//  	if(!(bWantsToWallRun || bDoingWallrun) && OtherComp && !bDoingPowerSlide)
	//  	{
	//  		/// ECC_GameTraceChannel2 == "Wallrunnable", Check Config/DefaultEngine.ini to ensure
	//  		if(OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel2)
	//  		{
	//  			if(IsValidWallrunAngle(Hit.ImpactNormal))
	//  			{
	//  				WallrunningComponent = OtherComp;
	//  		
	//  				FindWallRunDirection(Hit.ImpactNormal);
	//  				BeginWallRun();
	//  			}
	//  		}
	//  	}
	//  }

	if(CurrentFloor.HitResult.Component.IsValid() && CurrentFloor.HitResult.Component.Get()->GetCollisionObjectType() == ECC_GameTraceChannel2)
	{
		UPrimitiveComponent* OtherComp = CurrentFloor.HitResult.Component.Get();
		if(!(bWantsToWallRun || bDoingWallrun) && OtherComp && !bDoingPowerSlide)
		{
			/// ECC_GameTraceChannel2 == "Wallrunnable", Check Config/DefaultEngine.ini to ensure
			if(OtherComp->GetCollisionObjectType() == ECC_GameTraceChannel2)
			{
				if(IsValidWallrunAngle(CurrentFloor.HitResult.ImpactNormal))
				{
					WallrunningComponent = OtherComp;
			
					FindWallRunDirection(CurrentFloor.HitResult.ImpactNormal);
					BeginWallRun();
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
		print(FString("ProcessLanded"));
		StopWallRun();
	}
}

bool UFPSMovementComponent::CanAttemptJump() const
{
	return Super::CanAttemptJump() || IsCustomMovementMode(CMOVE_WallRunning);
}


float UFPSMovementComponent::SlideAlongSurface(const FVector& Delta, float Time, const FVector& InNormal, FHitResult& Hit,
                                               bool bHandleImpact)
{
	// if (!Hit.bBlockingHit)
	// {
	// 	return 0.f;
	// }
	//
	// FVector Normal(InNormal);
	// if (IsMovingOnGround())
	// {
	// 	// We don't want to be pushed up an unwalkable surface.
	// 	if (Normal.Z < -KINDA_SMALL_NUMBER)
	// 	{
	// 		// Don't push down into the floor when the impact is on the upper portion of the capsule.
	// 		if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
	// 		{
	// 			const FVector FloorNormal = CurrentFloor.HitResult.Normal;
	// 			const bool bFloorOpposedToMovement = (Delta | FloorNormal) < 0.f && (FloorNormal.Z < 1.f - DELTA);
	// 			if (bFloorOpposedToMovement)
	// 			{
	// 				Normal = FloorNormal;
	// 			}
	// 			
	// 			Normal = Normal.GetSafeNormal2D();
	// 		}
	// 	}
	// }
	
	// return Super::Super::SlideAlongSurface(Delta, Time, Normal, Hit, bHandleImpact);
	return Super::SlideAlongSurface(Delta, Time, InNormal, Hit, bHandleImpact);
}

void UFPSMovementComponent::TwoWallAdjust(FVector& Delta, const FHitResult& Hit, const FVector& OldHitNormal) const
{
	return Super::TwoWallAdjust(Delta, Hit, OldHitNormal);

	
	// const FVector InDelta = Delta;
	// Super::TwoWallAdjust(Delta, Hit, OldHitNormal);
	//
	// if (IsMovingOnGround())
	// {
	// 	// Allow slides up walkable surfaces, but not unwalkable ones (treat those as vertical barriers).
	// 	if (Delta.Z > 0.f)
	// 	{
	// 		if (Hit.Normal.Z > KINDA_SMALL_NUMBER)
	// 		{
	// 			// Maintain horizontal velocity
	// 			const float Time = (1.f - Hit.Time);
	// 			const FVector ScaledDelta = Delta.GetSafeNormal() * InDelta.Size();
	// 			Delta = FVector(InDelta.X, InDelta.Y, ScaledDelta.Z / Hit.Normal.Z) * Time;
	//
	// 			// Should never exceed MaxStepHeight in vertical component, so rescale if necessary.
	// 			// This should be rare (Hit.Normal.Z above would have been very small) but we'd rather lose horizontal velocity than go too high.
	// 			if (Delta.Z > MaxStepHeight)
	// 			{
	// 				const float Rescale = MaxStepHeight / Delta.Z;
	// 				Delta *= Rescale;
	// 			}
	// 		}
	// 		else
	// 		{
	// 			Delta.Z = 0.f;
	// 		}
	// 	}
	// 	else if (Delta.Z < 0.f)
	// 	{
	// 		// Don't push down into the floor.
	// 		if (CurrentFloor.FloorDist < MIN_FLOOR_DIST && CurrentFloor.bBlockingHit)
	// 		{
	// 			Delta.Z = 0.f;
	// 		}
	// 	}
	// }
}

void UFPSMovementComponent::OnActorHit(AActor* SelfActor, AActor* OtherActor, FVector NormalImpulse,
                                       const FHitResult& Hit)
{
	// Test for entry into wallrun
	UPrimitiveComponent* OtherComp = Hit.Component.Get();
	if(!(bWantsToWallRun || bDoingWallrun) && OtherComp && !bDoingPowerSlide && IsFalling())
	{
		print(FString("OnActorHit"));
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
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation() + WallrunTraceOffset, GetOwner()->GetActorLocation() +WallrunTraceOffset+ WallrunningDir * 20.f, FColor::Red, false, -1, 0, 1);
	
}

bool UFPSMovementComponent::IsValidWallrunAngle(const FVector& surface_normal) const
{
	// Check if wall is angled in such a way that is walrunnable	
	FVector normalNoZ = FVector(0.f, 0.f, 1.0f);
	normalNoZ.Normalize();

	// Find the angle of the wall
	float wallAngle = FMath::Acos(FVector::DotProduct(normalNoZ, surface_normal)) * 180 / PI;
	
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation()+ WallrunTraceOffset, GetOwner()->GetActorLocation() + WallrunTraceOffset+ surface_normal * 60.f, FColor::Emerald, false, -1, 0, 3);

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
			WallrunningSurfaceNormal = Floor.HitResult.ImpactNormal;
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

	DrawDebugLine(GetWorld(), traceStart, traceEnd, FColor::Blue, false, .2f, 0, 4);

	/// Perform linetrace with ECC_GameTraceChannel3 == "WallrunableTrace", Check Config/DefaultEngine.ini to ensure
	/// we dont use 12 or "Wallrunnable" since player's own meshges will block with trace
	FHitResult Hit;
	if(GetWorld()->LineTraceSingleByChannel(Hit, traceStart, traceEnd, ECollisionChannel::ECC_GameTraceChannel3))
	{
		if(Hit.Component.Get() == WallrunningComponent && IsValidWallrunAngle(Hit.ImpactNormal))
		{
			// with valid wallruning data and angles, calculate the wallrunning dir
			WallrunningSurfaceNormal = Hit.ImpactNormal;
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
		// 	const FVector BoostDir = (WallRunSide == EWallRunSide::RIGHT) ? (WallrunningDir.Cross(WallrunningSurfaceNormal)).GetSafeNormal() : (WallrunningSurfaceNormal.Cross(WallrunningDir)).GetSafeNormal();
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + BoostDir * 200.f, FColor::Orange, false, 12.2f, 0, 1);
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + WallrunningDir * 200.f, FColor::Red, false, 12.2f, 0, 1);
		// 	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + WallrunningSurfaceNormal * 200.f, FColor::Magenta, false, 12.2f, 0, 1);
		//
		//
		// 	//Velocity += BoostDir * WallrunStartBoost;
		// }

	
	
		OnWallrunStarted.Broadcast();

		print(FString("BeginWallrun"));
	}
}

void UFPSMovementComponent::StopWallRun()
{
	SetMovementMode(MOVE_Falling);
	
	bDoingWallrun = false;
	bWantsToWallRun = false;
	
	OnWallrunEnded.Broadcast();

	
	print(FString("StopWallrun"));
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
			Acceleration = FVector::VectorPlaneProject(Acceleration, WallrunningSurfaceNormal);
			//Velocity.Z = 0.f;
			CalcVelocity(DeltaTime, WallrunLateralFriction, false, MaxDecel);
			//Velocity.Z = OldVelocity.Z;
		}
	}

	
	// apply gravity
	FVector Gravity{0.f, 0.f, GetGravityZ()};
	Velocity = NewFallVelocity(Velocity, Gravity * WallrunGravityCoeff, DeltaTime);
		
	if(Velocity.Z < -45.f)
	{
		Velocity.Z += Velocity.Z*-1.2f * DeltaTime;
	}


	
	// 2. Applying Grip force or "attraction" towards wall.... Booba
	// const FVector crossVector = WallRunSide == EWallRunSide::LEFT ? FVector(0.0f, 0.0f, -1.0f) : FVector(0.0f, 0.0f, 1.0f);
	// FVector traceStart = GetPawnOwner()->GetActorLocation() + WallrunTraceOffset + (WallrunningDir * 20.0f);
	// FVector traceEnd = traceStart + (FVector::CrossProduct(WallrunningDir, crossVector) * MaxWallrunDistance);
	
	FVector Grip = WallrunningSurfaceNormal * -1 * WallrunGripForce;  //(traceEnd - traceStart).GetSafeNormal();
	//AddForce(Grip);
	
	Grip *= WallrunGripForce * DeltaTime;
	
	Velocity += Grip;

	//AddForce(MaxWallrunWalkSpeed * GetInputVector().ProjectOnTo(WallrunningDir));
	 // // FVector DeltaMov = Velocity;
	 // FVector VelocityXY = Velocity.ProjectOnTo(WallrunningDir);
	 // Velocity = FVector(VelocityXY.X, VelocityXY.Y, Velocity.Z);
	
	DrawDebugLine(GetWorld(), GetOwner()->GetActorLocation(), GetOwner()->GetActorLocation() + Velocity * 1.f, FColor::Purple, false, .2f, 0, 1);
	

	//Super::PhysFalling(DeltaTime, iterations);
	
	// TODO: enable below if  using physcustom and need comp moved
	// const FVector Adjusted = Velocity * DeltaTime;
	// FHitResult Hit(1.f);
	// SafeMoveUpdatedComponent(Adjusted, UpdatedComponent->GetComponentQuat(), true, Hit);
	
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

	// if (PawnOwner->GetLocalRole() == ROLE_Authority)
	// {
	// 	print(FString("Server StopGrapple LEFT"));
	// }
	// else
	// {
	// 	print(FString("Client StopGrapple LEFT"));
	// }
}

void UFPSMovementComponent::StopGrappleRight()
{
	bWantsToGrappleRight = false;

	// if (PawnOwner->GetLocalRole() == ROLE_Authority)
	// {
	// 	print(FString("Server StopGrapple Right"));
	// }
	// else
	// {
	// 	print(FString("Client StopGrapple Right"));
	// }
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

	//print(FString::FromInt(GrapplesToQuery.Num()));
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

	// FHitResult Hit(1.f);
	//
	// print((Vel*DeltaTime*-1).ToString());
	// SafeMoveUpdatedComponent(Vel*DeltaTime*-1, UpdatedComponent->GetComponentQuat(), true, Hit);
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

	OnPowerSlideEnded.Broadcast();
}

void UFPSMovementComponent::InitiatePowerSlide(float DeltaTime)
{	
	GroundFrictionPreValue = GroundFriction;
	MaxWalkSpeedPreValue = MaxWalkSpeed;
	
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