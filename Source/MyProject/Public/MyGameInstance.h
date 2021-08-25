// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "SettingsSave.h"
#include "MyGameInstance.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:

	UMyGameInstance();

	virtual void Init() override;

	UPROPERTY(BlueprintReadWrite)
	USettingsSave* SettingsSave;


	UFUNCTION(BlueprintCallable)
	void LoadSettingsData();

	UFUNCTION(BlueprintCallable)
	void SaveSettingsData();

protected:

	UPROPERTY()
	FString SettingsSaveSlot = "SettingsSlaveSlot";
};
