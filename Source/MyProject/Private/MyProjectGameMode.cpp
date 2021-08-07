// Copyright Epic Games, Inc. All Rights Reserved.

#include "MyProjectGameMode.h"

#include "MyGameState.h"
#include "Actors/Characters/MyProjectCharacter.h"
#include "MyPlayerController.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Actors/MyPlayerStart.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "MyGameState.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h" 



#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


void AMyProjectGameMode::EndMatch()
{
	Super::EndMatch();
	for(auto Player : ActivePlayers)
	{
		Player->GetPawn()->DisableInput(Player);

		Player->OnMatchEnded();

	}
	
	GetWorld()->GetTimerManager().SetTimer(MatchEndedTimerHandle, this, &AMyProjectGameMode::HandleMatchExit, EndGameToLevelLeaveDelay, false);
}

AMyProjectGameMode::AMyProjectGameMode()
{
	// set default pawn class to our Blueprinted character

	static ConstructorHelpers::FClassFinder<AMyProjectCharacter> PlayerPawnBPClass(TEXT("/Game/Characters/BP_ShooterChar.BP_ShooterChar_C"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		PlayerClass = PlayerPawnBPClass.Class;
	}


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

void AMyProjectGameMode::MovePlayerToSafeSpawnLocation(AMyProjectCharacter* Player)
{

	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyPlayerStart::StaticClass(), FoundActors);

	float LowestPoints = 999999999999.f;
	AMyPlayerStart* SpawnActor = nullptr;
	for(auto e : FoundActors)
	{
		AMyPlayerStart* Start = Cast<AMyPlayerStart>(e); 
		if(Start)
		{
			float points = Start->GetCurrentSpawnPoints();
			if(points < LowestPoints)
			{
				LowestPoints = points;
				SpawnActor = Start;	
			}
		}
	}

	if(!SpawnActor)
		MovePlayerToSpawnLocation(Player);

	Player->SetActorLocation(SpawnActor->GetActorLocation());
}

void AMyProjectGameMode::MovePlayerToStartSpawnLocation(AMyProjectCharacter* Player)
{
	// Default to safe location cause this class has no notion of teams
	MovePlayerToSafeSpawnLocation(Player);
}

void AMyProjectGameMode::BP_EndMatch()
{
	EndMatch();
}

void AMyProjectGameMode::OnPlayerDied(FPlayerDeathInfoStruct Info)
{
	// Cycle through all active players and notify (for widget update)
	for(auto Player : ActivePlayers)
	{
		Player->OnSomeoneDied(Info);
	}
}


void AMyProjectGameMode::HandleStartingNewPlayer_Implementation(APlayerController* NewPlayer)
{
	Super::HandleStartingNewPlayer_Implementation(NewPlayer);

	AMyPlayerController* PC = Cast<AMyPlayerController>(NewPlayer);
	if(PC && !PC->GetPawn())
	{
		//PendingPlayers.AddUnique(PC);

		// Doing non queued spawns (might have async issues)
		ActivePlayers.AddUnique(PC);

		SpawnCharacterForPlayer(PC);
	}

	
	// APlayerController* PC = Cast<APlayerController>(NewPlayer);
	// AP4PreGameLobbyGameState* GS = GetGameState<AP4PreGameLobbyGameState>();
	// if (GS && PC)
	// {
	// 	GS->InitNewPlayerState(PC);
	// }
	//
}


void AMyProjectGameMode::BeginPlay()
{
	Super::BeginPlay();

	//StartPlay();
}

void AMyProjectGameMode::HandleMatchHasStarted()
{
	Super::HandleMatchHasStarted();

	AMyGameState* GS = GetGameState<AMyGameState>();
	GS->ServerTimeWhenMatchStarted = GetWorld()->GetTimeSeconds();

	
	GetWorld()->GetTimerManager().SetTimer(MatchTimerHandle, this, &AMyProjectGameMode::OnMatchTimerEnded, GS->TotalMatchtime_s, false);
	
}

void AMyProjectGameMode::HandleMatchExit()
{
	// TODO: exit to post lobby world not main menu
	GetWorld()->ServerTravel("/Game/Maps/MainMenu");
}



void AMyProjectGameMode::OnMatchTimerEnded()
{
	EndMatch();
}

void AMyProjectGameMode::AssignTeamForPlayer(AMyProjectCharacter* Player)
{
	// Default to FFA so do nothing
}


void AMyProjectGameMode::SpawnCharacterForPlayer(AMyPlayerController* Controller)
{
	if(Controller)
	{
		FActorSpawnParameters SpawnInfo;
		SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
		FRotator myRot(0, 0, 0);
		FVector myLoc(0, 0, 100);
		
		AMyProjectCharacter* Char = GetWorld()->SpawnActor<AMyProjectCharacter>(PlayerClass, myLoc, myRot, SpawnInfo);
		AssignTeamForPlayer(Char);
		MovePlayerToStartSpawnLocation(Char);

		/*FScriptDelegate DiedDelegate;
		DiedDelegate.BindUFunction(this, "OnPlayerDied");
		Char->OnCharacterDied.AddUnique(DiedDelegate);*/
		
		Controller->Possess(Char);
	}
}


