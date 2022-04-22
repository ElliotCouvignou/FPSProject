// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "MainMenuWidget.generated.h"


// Struct for sessions TODO: MOVE THIS TO UI .h
USTRUCT(BlueprintType)
struct FServerData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Name;

	UPROPERTY(BlueprintReadWrite)
	int PingMs;
	
	UPROPERTY(BlueprintReadWrite)
	int CurrentPlayers;
	
	UPROPERTY(BlueprintReadWrite)
	int MaxPlayers;
	
	UPROPERTY(BlueprintReadWrite)
	FString HostUsername;
};

/**
 * 
 */
UCLASS()
class MYPROJECT_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void BP_InitializeSessionsList(const TArray<FServerData>& ServerData);
};