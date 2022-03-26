// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSMovementComponent.h"
#include "FPSMovementComponent.h"

#include "GameFramework/Character.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


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

void UFPSMovementComponent::OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation,
	const FVector& OldVelocity)
{
	Super::OnMovementUpdated(DeltaSeconds, OldLocation, OldVelocity);

	if (!CharacterOwner)
	{
		return;
	}

	//Store movement vector
	if (PawnOwner->IsLocallyControlled())
	{
		MoveDirection = PawnOwner->GetLastMovementInputVector();
	}
	//Send movement vector to server
	if (PawnOwner->GetLocalRole() < ROLE_Authority)
	{
		ServerSetMoveDirection(MoveDirection);
	}

	if(bWantsToGrappleLeft)
	{
		UpdateGrappleMoement(DeltaSeconds, true);
	}
	if(bWantsToGrappleRight)
	{
		UpdateGrappleMoement(DeltaSeconds, false);
	}
	if(bWantsToPowerSlide)
	{
		UpdatePowerSlide(DeltaSeconds);
	}
}

void UFPSMovementComponent::DoGrappleLeft(FVector EndLocation)
{
	bWantsToGrappleLeft = true;
	GrappleLeftEndLocation = EndLocation;
}

void UFPSMovementComponent::DoGrappleRight(FVector EndLocation)
{
	bWantsToGrappleRight = true;
	GrappleRightEndLocation = EndLocation;
}

void UFPSMovementComponent::StopGrappleLeft()
{
	bWantsToGrappleLeft = false;
}

void UFPSMovementComponent::StopGrappleRight()
{
	bWantsToGrappleRight = false;
}

void UFPSMovementComponent::UpdateGrappleMoement(float DeltaTime, bool IsLeft)
{
	float PlayerPullInfluence = 0.f;
	float PlayerSpringInfluence = 0.f;
		
	// Affect pullstrength through control direction (only when wanting to move)
	// TODO: get player input vector, currently not replicated cause risky so im deriving from acceleration, not the best
	FVector InputVector = GetCurrentAcceleration();//Character->GetPendingMovementInputVector();
	InputVector.Z = 0.f;
	InputVector.Normalize();

	FVector GrappleLoc = (IsLeft) ? GrappleLeftEndLocation : GrappleRightEndLocation;
	const FVector Dist = (GrappleLoc - GetOwner()->GetActorLocation()/*ODMComponent->GetSocketLocation(FName("GrappleSocket"))*/);
	
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
			
		InputVector.Normalize();
		
		//DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + (InputVector *200.f), FColor::Purple, false, 5.f, 0.f, 1.f);
	}
		
	// not really force more like change in velocity but intuitively this is nicer to me idk lol
	// TODO: find value for curve im too douinked for this rn
	// Try above but in general when moving against the grapple
	const FVector Vel = Velocity;
	SpeedInPullDirection = FMath::Clamp(Vel.Size() * FVector::DotProduct(Dist.GetSafeNormal(), Vel.GetSafeNormal()), 0.f, 99999999.f);
	//print(FString("SpeedInPullDirection: " + FString::SanitizeFloat(SpeedInPullDirection,2)));

		
	FVector Force = Dist.GetSafeNormal() * (DeltaTime * PullStrengthSpeedDirection->GetFloatValue(SpeedInPullDirection));
	Force += InputVector.ProjectOnTo(Force) * Force.Size() * PlayerPullInfluence;
				
	const float Angle = FMath::Acos(FVector::DotProduct(Vel , Force )/ Vel.Size()/ Force.Size());
	if(Angle > PI / 2)
	{
		// TODO: formula is k * dt * v / m = dv/dt but velocity projecting along spring direction doesn't have its magnitude correct due to normals.
		FVector SpringForce = Vel.GetSafeNormal().ProjectOnTo(Dist.GetSafeNormal()) * DeltaTime * SpringFactor * Vel.Size() * -1.f;
		FVector Add = InputVector.ProjectOnTo(SpringForce) * SpringForce.Size() * PlayerSpringInfluence;
		SpringForce +=  InputVector.ProjectOnTo(Force) * SpringForce.Size() * PlayerSpringInfluence;
		
		Velocity += SpringForce;
		//Character->LaunchCharacter(SpringForce, false, false);
		//DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + (SpringForce *200.f), FColor::Orange, false, 5.f, 0.f, 1.f);
	}

	//print(FString("mag: " + FString::SanitizeFloat(Force.Size(),2)));
	Velocity += Force;
	//Character->LaunchCharacter(Force, false, false);
}

void UFPSMovementComponent::UpdatePowerSlide(float DeltaTime)
{
	
}

//Set input flags on character from saved inputs
void UFPSMovementComponent::UpdateFromCompressedFlags(uint8 Flags)//Client only
{
	Super::UpdateFromCompressedFlags(Flags);

	
	//The Flags parameter contains the compressed input flags that are stored in the saved move.
	//UpdateFromCompressed flags simply copies the flags from the saved move into the movement component.
	//It basically just resets the movement component to the state when the move was made so it can simulate from there.
	int32 DodgeFlags = (Flags >> 2) & 7;
	
	bWantsToGrappleRight = (DodgeFlags == 1);
	bWantsToGrappleLeft = (DodgeFlags == 2);
	bWantsToPowerSlide = (DodgeFlags == 3);
	bWantsToWallRun = (DodgeFlags == 4);
	bWantsToMantle = (DodgeFlags == 5);
		
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

void FSavedMove_MyMovement::Clear()
{
	Super::Clear();
	
	SavedMoveDirection = FVector::ZeroVector;
	
	bSavedWantsToGrappleLeft = false;
	bSavedWantsToGrappleRight = false;
	bSavedWantsToPowerSlide = false;
	bSavedWantsToWallRun = false;
	bSavedWantsToMantle = false;
}

uint8 FSavedMove_MyMovement::GetCompressedFlags() const
{
	// Result # 1 and 2 taked by jump and crouch respectively
	uint8 Result = Super::GetCompressedFlags();

	
	if (bSavedWantsToGrappleRight)
	{
		Result |= (1 << 2);
	}
	else if (bSavedWantsToGrappleLeft)
	{
		Result |= (2 << 2);
	}
	else if (bSavedWantsToPowerSlide)
	{
		Result |= (3 << 2);
	}
	else if (bSavedWantsToWallRun)
	{
		Result |= (4 << 2);
	}
	else if (bSavedWantsToMantle)
	{
		Result |= (5 << 2);
	}

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
		bSavedWantsToMantle = CharMov->bWantsToMantle;
		
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