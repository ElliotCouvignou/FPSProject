// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MyPlayerController.generated.h"


class UMainGameplayWidget;

/**
 * 
 */
UCLASS()
class MYPROJECT_API AMyPlayerController : public APlayerController
{
	GENERATED_BODY()


public:

	UFUNCTION(BlueprintCallable)
	void CreateMainGameplayWidget();

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMainGameplayWidget* MainGameplayWidget;

protected:

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TSubclassOf<UMainGameplayWidget> MainGameplayWidgetClass;

	
};
