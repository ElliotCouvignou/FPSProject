// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)


UMyGameInstance::UMyGameInstance()
{

	SettingsSave = Cast<USettingsSave>(UGameplayStatics::CreateSaveGameObject(USettingsSave::StaticClass()));
}

void UMyGameInstance::Init()
{
	Super::Init();

	LoadSettingsData();

	if(IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get())
	{
		SessionInterface = Subsystem->GetSessionInterface();
		if (SessionInterface.IsValid())
		{
			SessionInterface->OnCreateSessionCompleteDelegates.AddUObject(this, &UMyGameInstance::OnCreateSessionComplete);
			SessionInterface->OnFindSessionsCompleteDelegates.AddUObject(this, &UMyGameInstance::OnFindSessionComplete);
			SessionInterface->OnJoinSessionCompleteDelegates.AddUObject(this, &UMyGameInstance::OnJoinSessionComplete);
		}
	}
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


void UMyGameInstance::OnCreateSessionComplete(FName SessionName, bool Succeeded)
{
	if (Succeeded)
	{
		GetWorld()->ServerTravel("/Game/Maps/TestMap?listen");
	}
}

void UMyGameInstance::OnFindSessionComplete(bool Succeeded)
{
	print(FString("OnFindSessionComplete"));
	
	if (Succeeded)
	{
		print(FString("		Search Success! "));
		
		TArray<FOnlineSessionSearchResult> SearchResults = SessionSearch->SearchResults;

		if (SearchResults.Num())
		{
			print(FString("Results.Num() > 0, Joining Sesion...."));
			SessionInterface->JoinSession(0, FName("MyProjectFPSSession"), SearchResults[0]);
		}
	}
	else
	{
		print(FString("		Search Failure! "));
	}
}

void UMyGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	print(FString("OnJoinSessionComplete"));
	
	if(APlayerController* PController = UGameplayStatics::GetPlayerController(GetWorld(), 0))
	{
		FString JoinAddress = "";
		SessionInterface->GetResolvedConnectString(SessionName, JoinAddress);
		if(JoinAddress != "")
		{
			print(FString("ClientTravel to address"));
			PController->ClientTravel(JoinAddress, ETravelType::TRAVEL_Absolute);
		}
	}
}

void UMyGameInstance::CreateServer()
{
	UE_LOG(LogTemp, Warning, TEXT("CreateServer"));
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bIsDedicated = false;         // P2P 
	SessionSettings.bIsLANMatch = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");    // LAN if NULL subsystem
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = true;
	SessionSettings.NumPublicConnections = 5;				// TODO: expose this

	SessionInterface->CreateSession(0, FName("MyProjectFPSSession"), SessionSettings);  // TODO: expose name
}

void UMyGameInstance::JoinServer()
{
	print(FString("Trying to Join Server..."));
	
	SessionSearch = MakeShareable(new FOnlineSessionSearch());
	SessionSearch->bIsLanQuery = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");
	SessionSearch->MaxSearchResults = 10000;
	SessionSearch->QuerySettings.Set("SEARCH_PRESENCE", true, EOnlineComparisonOp::Equals);

	SessionInterface->FindSessions(0, SessionSearch.ToSharedRef());
}