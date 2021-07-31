// Fill out your copyright notice in the Description page of Project Settings.


#include "MyPlayerController.h"
#include "UI/MainGameplayWidget.h"

void AMyPlayerController::CreateMainGameplayWidget()
{
	if(!MainGameplayWidget)
	{
		MainGameplayWidget = CreateWidget<UMainGameplayWidget>(this, MainGameplayWidgetClass);
		MainGameplayWidget->AddToViewport();
	}
}
