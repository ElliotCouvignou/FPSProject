// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
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
	void OnHealthChanged(float NewValue);

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void OnHealthMaxChanged(float NewValue);
};
