// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "UI/MainGameplayWidget.h"
#include "MyGameState.h"
#include "Blueprint/WidgetBlueprintLibrary.h"

void AMyPlayerController::CreateMainGameplayWidget()
{
	if(!MainGameplayWidget)
	{
		MainGameplayWidget = CreateWidget<UMainGameplayWidget>(this, MainGameplayWidgetClass);
		MainGameplayWidget->AddToViewport();

		bShowMouseCursor = false;
		UWidgetBlueprintLibrary::SetInputMode_GameOnly(this);
	}
}

void AMyPlayerController::OnPlayerDamageDone_Implementation(const FDamageInfoStruct& InfoStruct)
{
	BP_ClientOnPlayerDamageDone(InfoStruct);
}

void AMyPlayerController::OnPlayerKilled_Implementation(const FKillInfoStruct& InfoStruct)
{
	if(MainGameplayWidget)
	{
		MainGameplayWidget->OnPlayerKilled(InfoStruct);
	}	
}

void AMyPlayerController::OnPlayerDied_Implementation(const FDiedInfoStruct& InfoStruct)
{
	if(MainGameplayWidget)
	{
		MainGameplayWidget->OnPlayerDied(InfoStruct);
	}
}

void AMyPlayerController::OnSomeoneDied_Implementation(const FPlayerDeathInfoStruct& InfoStruct)
{
	if(MainGameplayWidget)
	{
		MainGameplayWidget->OnSomeoneDied(InfoStruct);
	}
}

void AMyPlayerController::OnMatchEnded_Implementation()
{
	if(MainGameplayWidget)
	{
		MainGameplayWidget->OnMatchEnded();
	}
}

void AMyPlayerController::ReceivedPlayer()
{
	Super::ReceivedPlayer();

	if(IsLocalController())
	{
		ServerRequestServerTime(this, GetWorld()->GetTimeSeconds());
	}
}

void AMyPlayerController::ServerRequestServerTime_Implementation(APlayerController* requester, float requestWorldTime)
{
	float serverTime = GetWorld()->GetGameState()->GetServerWorldTimeSeconds();
	
	ClientReportServerTime(requestWorldTime, serverTime);
}

bool AMyPlayerController::ServerRequestServerTime_Validate(APlayerController* requester, float requestWorldTime)
{
	return true;
}


void AMyPlayerController::ClientReportServerTime_Implementation(float requestWorldTime, float serverTime)
{
	// Apply the round-trip request time to the server's         
	// reported time to get the up-to-date server time
	float roundTripTime = GetWorld()->GetTimeSeconds() - 
		requestWorldTime;
	float adjustedTime = serverTime + (roundTripTime * 0.5f);
	ServerTime = adjustedTime;
}
