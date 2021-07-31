// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectGameMode.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

AMyProjectGameMode::AMyProjectGameMode()
{
	// set default pawn class to our Blueprinted character

	
	PlayerClass = StaticLoadClass(UObject::StaticClass(), nullptr, TEXT("/Game/GASShooter/Characters/Hero/BP_HeroCharacter.BP_HeroCharacter_C"));
}

void AMyProjectGameMode::MovePlayerToSpawnLocation(AMyProjectCharacter* Player)
{
	// TODO: implement some method to prevent spawning near enemies

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), APlayerStart::StaticClass(), FoundActors);

	if(FoundActors.Num() > 0)
	{
		Player->SetActorLocation(FoundActors[static_cast<int>(FMath::RandRange(0.f, FoundActors.Num() - 1.f))]->GetActorLocation());
	}
}

void AMyProjectGameMode::PlayerDied(AController* Controller)
{
}


void AMyProjectGameMode::BeginPlay()
{
	Super::BeginPlay();

	//StartPlay();
}

void AMyProjectGameMode::RespawnPlayer(AController* Controller)
{
}
