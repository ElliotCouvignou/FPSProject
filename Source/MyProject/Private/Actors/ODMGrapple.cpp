// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/ODMGrapple.h"
#include "Net/UnrealNetwork.h"
#include "DrawDebugHelpers.h"
#include "FPSMovementComponent.h"
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

		//FScriptDelegate Delegate;
		//Delegate.BindUFunction(this, "OnGrappleHeadOverlap");
		//GrappleHead->OnComponentBeginOverlap.Add(Delegate);
	}
	
	Cable = CreateDefaultSubobject<UCableComponent>(TEXT("Cable"));
	Cable->SetIsReplicated(true);
	Cable->NumSegments = 1;
	Cable->SetupAttachment(GrappleHead);
	Cable->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Cable->CastShadow = true;
	Cable->EndLocation = FVector(0.f, 0.f, 0.f);
	
	SetActorTickEnabled(false);


	ProjectileMovement = CreateOptionalDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
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
		
	}
	
	//OnGrappleHeadOverlap();
	SetActorTickEnabled(true);
	//GetWorldTimerManager().SetTimer(BeginPlayTimerHandle, this, &AODMGrapple::OnDelayedCableSegmentReduce, CableSegmentsDelay, false);
}

void AODMGrapple::Destroyed()
{
	Super::Destroyed();
}

void AODMGrapple::OnGrappleHeadOverlap()
{
	if(Character)
	{
		UFPSMovementComponent* MoveComp = Cast<UFPSMovementComponent>(Character->GetCharacterMovement());
		if(IsLeftGrapple)
			MoveComp->DoGrappleLeft(GetActorLocation());
		else
			MoveComp->DoGrappleRight(GetActorLocation());
	}
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
}

// ovveride replciation with replication variables
void AODMGrapple::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const {

	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(AProject4Character, AttributeSet);
	DOREPLIFETIME(AODMGrapple, ODMComponent);
}
