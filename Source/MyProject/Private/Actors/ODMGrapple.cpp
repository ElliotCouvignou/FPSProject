// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ODMGrapple.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


// Sets default values
AODMGrapple::AODMGrapple()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	bReplicates = true;
	bAlwaysRelevant = true;
	
	GrappleHead = CreateOptionalDefaultSubobject<UStaticMeshComponent>("GrappleHead");
	if(GrappleHead)
	{
		GrappleHead->AlwaysLoadOnClient = true;
		GrappleHead->AlwaysLoadOnServer = true;
		GrappleHead->bOwnerNoSee = false;
		GrappleHead->bAffectDynamicIndirectLighting = true;
		GrappleHead->PrimaryComponentTick.TickGroup = TG_PrePhysics;
		GrappleHead->SetupAttachment(RootComponent);
		static FName MeshCollisionProfileName(TEXT("Pawn"));
		GrappleHead->SetCollisionProfileName(MeshCollisionProfileName);
		GrappleHead->SetGenerateOverlapEvents(false);
		GrappleHead->SetCanEverAffectNavigation(false);

		FScriptDelegate Delegate;
		Delegate.BindUFunction(this, "OnGrappleHeadOverlap");
		GrappleHead->OnComponentBeginOverlap.Add(Delegate);
	}
	
	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("Cable"));
	Cable->SetIsReplicated(true);
	Cable->NumSegments = 1;
	Cable->SetupAttachment(GrappleHead);
	Cable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cable->CastShadow = true;
	Cable->EndLocation = FVector(0.f, 0.f, 0.f);
	
	SetActorTickEnabled(false);

	// ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	// ProjectileMovement->InitialSpeed = InitProjectileVelocity;
	// ProjectileMovement->MaxSpeed = InitProjectileVelocity;
	// ProjectileMovement->ProjectileGravityScale = 0.f;
	
}

void AODMGrapple::OnDelayedCableSegmentReduce()
{
	Cable->NumSegments = 1;
}

// Called when the game starts or when spawned
void AODMGrapple::BeginPlay()
{
	Super::BeginPlay();

	Character = Cast<ACharacter>(GetInstigator());
	
	if(ODMComponent)
	{
		// hardcoded socket name lol
		Cable->SetAttachEndToComponent(ODMComponent, FName("GrappleSocket"));
		Cable->EndLocation = FVector(0.f, 0.f, 0.f);  // this gets set to some weird offset idk why
		
		//Cable->SetAttachEndTo(Character, FName(ODMComponent->GetName()), FName("GrappleSocket"));		
		// TArray<FVector> Locations;
		// Cable->GetCableParticleLocations(Locations);
		// USceneComponent* EndComponent = Cast<USceneComponent>(Cable->AttachEndTo.GetComponent(Character));
		//
		//FVector EndLocation = ODMComponent->GetSocketLocation(FName("GrappleSocket"));
		// FVector EndLocation = EndComponent->GetSocketTransform(FName("GrappleSocket")).TransformPosition(Cable->EndLocation);
		//
		// DrawDebugSphere(GetWorld(), EndLocation, 32.f, 8, FColor::Blue, false, 5.f, 0.f, 1.f);
		// DrawDebugSphere(GetWorld(), Character->GetActorLocation(), 32.f, 8, FColor::Green, false, 5.f, 0.f, 1.f);
		//
		// DrawDebugLine(GetWorld(), Character->GetActorLocation(), EndLocation, FColor::Purple, false, 5.f, 0.f, 1.f);
		//
	}

	// begin play should fire automatically
	// TODO: play audio hhere?

	
	SetActorTickEnabled(true);
	//GetWorldTimerManager().SetTimer(BeginPlayTimerHandle, this, &AODMGrapple::OnDelayedCableSegmentReduce, CableSegmentsDelay, false);
}


FVector AODMGrapple::Test_GetClientInputVector()
{
	float F = Character->GetInputAxisValue(FName("MoveForward"));
	
	APlayerController* PC = Character->GetController<APlayerController>();
	if(PC)
	{
		F = PC->GetInputKeyTimeDown(FKey(FName("MoveForward")));
	}
	

	return FVector(0.f, 0.f, F);
}

void AODMGrapple::OnGrappleHeadOverlap()
{
	// TODO: play audio hhere?

	DoGrapplePull = true;
	
	// ProjectileMovement->Velocity = FVector(0.f, 0.f, 0.f);
	// ProjectileMovement->MaxSpeed = 0.f;
	// ProjectileMovement->Deactivate();
}

// Called every frame
void AODMGrapple::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	const FVector Dist = (GetActorLocation() - GetOwner()->GetActorLocation()/*ODMComponent->GetSocketLocation(FName("GrappleSocket"))*/);

	if(Cable)
	{
		Cable->CableLength = Dist.Size();
	}
		
	if(DoGrapplePull && HasAuthority() && !DisableGrapplePull)
	{
		// print(FString("GrappleSocket Locaiton: " + ODMComponent->GetSocketLocation(FName("GrappleSocket")).ToString()));
		// print(FString("OwnerActor Locaiton: " + GetOwner()->GetActorLocation().ToString()));
		//
		float PlayerPullInfluence = 0.f;
		float PlayerSpringInfluence = 0.f;
		
		// Affect pullstrength through control direction (only when wanting to move)
		// TODO: get player input vector, currently not replicated cause risky so im deriving from acceleration, not the best
		FVector InputVector = Character->GetCharacterMovement()->GetCurrentAcceleration();//Character->GetPendingMovementInputVector();
		InputVector.Z = 0.f;
		InputVector.Normalize();
		
		if(InputVector.Size() > 0.f)
		{
			const FVector ControlRot = Character->GetControlRotation().Vector();
			const float Angle = FMath::Acos(FVector::DotProduct(InputVector , ControlRot )/ InputVector.Size()/ ControlRot.Size());
			if(Angle > PI / 2)
			{
				InputVector.Z -= Character->GetControlRotation().Vector().Z;
				PlayerPullInfluence = PlayerBackwardPullInfluence;
				PlayerSpringInfluence = PlayerBackwardSpringInfluence;
			}
			else
			{
				InputVector.Z += Character->GetControlRotation().Vector().Z; // add camera influence
				PlayerPullInfluence = PlayerForwardPullInfluence;
				PlayerSpringInfluence = PlayerForwardSpringInfluence;
			}
			
			InputVector.Normalize();
		
			//DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + (InputVector *200.f), FColor::Purple, false, 5.f, 0.f, 1.f);
		}
		
		// not really force more like change in velocity but intuitively this is nicer to me idk lol
		// TODO: find value for curve im too douinked for this rn
		// Try above but in general when moving against the grapple
		const FVector Vel = Character->GetCharacterMovement()->Velocity;
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
			
			Character->GetCharacterMovement()->Velocity += SpringForce;

			//DrawDebugLine(GetWorld(), Character->GetActorLocation(), Character->GetActorLocation() + (SpringForce *200.f), FColor::Orange, false, 5.f, 0.f, 1.f);
		}

		//print(FString("mag: " + FString::SanitizeFloat(Force.Size(),2)));
		Character->GetCharacterMovement()->Velocity += Force;
	}
}

// ovveride replciation with replication variables
void AODMGrapple::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AProject4Character, AttributeSet);
	DOREPLIFETIME(AODMGrapple, ODMComponent);
}
