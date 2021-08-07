// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/UIInfoTypes.h"
#include "MainGameplayWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API UMainGameplayWidget : public UUserWidget
{
	GENERATED_BODY()


public:


	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnPlayerKilled(const FKillInfoStruct& InfoStruct);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnPlayerDied(const FDiedInfoStruct& InfoStruct);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnSomeoneDied(const FPlayerDeathInfoStruct& InfoStruct);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnMatchEnded();

	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnHealthChanged(float NewValue);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnHealthMaxChanged(float NewValue);
};
