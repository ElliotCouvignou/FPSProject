// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameState.h"

#include "MyPlayerController.h"
#include "Net/UnrealNetwork.h"


AMyGameState::AMyGameState()
{
	
}

float AMyGameState::GetServerWorldTimeSeconds() const
{
	return Super::GetServerWorldTimeSeconds();
	// AMyPlayerController* PC = Cast<AMyPlayerController>(GetGameInstance()->GetFirstLocalPlayerController(GetWorld()));
	// if(PC)
	// {
	// 	return PC->GetServerTime();
	// }
	// else
	// {
	// 	return GetWorld()->GetTimeSeconds();
	// }
}


void AMyGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AMyGameState, TotalMatchtime_s);
	DOREPLIFETIME(AMyGameState, ServerTimeWhenMatchStarted);
	
}
