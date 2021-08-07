// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"

UMyGameInstance::UMyGameInstance()
{

	SettingsSave = Cast<USettingsSave>(UGameplayStatics::CreateSaveGameObject(USettingsSave::StaticClass()));
}

void UMyGameInstance::LoadSettingsData()
{
	if (SettingsSaveSlot.Len() > 0)
	{
		SettingsSave = Cast<USettingsSave>(UGameplayStatics::LoadGameFromSlot(SettingsSaveSlot, 0));
		if (SettingsSave == NULL)
		{
			// if failed to load, create a new one
			SettingsSave = Cast<USettingsSave>(UGameplayStatics::CreateSaveGameObject(USettingsSave::StaticClass()));
		}

		check(SettingsSave != NULL);
	}
}

void UMyGameInstance::SaveSettingsData()
{
	UGameplayStatics::SaveGameToSlot(SettingsSave, SettingsSaveSlot, 0);
}
