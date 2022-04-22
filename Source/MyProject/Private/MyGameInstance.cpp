// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "UI/MainMenuWidget.h"

#include "OnlineSessionSettings.h"
#include "OnlineSubsystemTypes.h"

#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)

const static FName SESSION_NAME = TEXT("GrappleFPSGameSession");
const static FName SERVER_NAME_SETTINGS_KEY = TEXT("ServerName");

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
			SessionInterface->OnDestroySessionCompleteDelegates.AddUObject(this, &UMyGameInstance::OnDestroySessionComplete);
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
	// It will not be success if there are more than one session with the same name already created
	if (!Succeeded)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnCreateSessionComplete] UNSUCESS"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnCreateSessionComplete] SUCESS SessionName: %s"), *SessionName.ToString());

	UEngine* Engine = GetEngine();

	if (Engine == nullptr) return;

	Engine->AddOnScreenDebugMessage(0, 2, FColor::Green, TEXT("[OnCreateSessionComplete::Host]"));

	UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnCreateSessionComplete] HOST TRAVEL TO LOBBY"));

	UWorld* World = GetWorld();

	if (World == nullptr) return;
	
	World->ServerTravel("/Game/Maps/TestMap?listen");

}

void UMyGameInstance::OnDestroySessionComplete(FName SessionName, bool Success)
{
	if (Success)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnDestroySessionComplete] Success "));
	}
}

void UMyGameInstance::OnFindSessionComplete(bool Succeeded)
{
	print(FString("OnFindSessionComplete"));

	if (Succeeded && SessionSearch.IsValid())
	{
		if (SessionSearch->SearchResults.Num() <= 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnFindSessionsComplete] No Sessions Find"));
		}
		else
		{
			TArray<FServerData> ServerData;
			for (const FOnlineSessionSearchResult& SearchResult : SessionSearch->SearchResults)
			{
				UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnFindSessionsComplete] Session Name %s"), *SearchResult.GetSessionIdStr());

				FServerData Data;
				FString ServerName;
				if (SearchResult.Session.SessionSettings.Get(SERVER_NAME_SETTINGS_KEY, ServerName))
				{
					UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnFindSessionsComplete] Data found in settings %s"), *ServerName);
					Data.Name = ServerName;
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnFindSessionsComplete] Data NOT found in settings"));

					Data.Name = "Could not find name";
				}

				Data.MaxPlayers = SearchResult.Session.SessionSettings.NumPublicConnections;
				Data.CurrentPlayers = Data.MaxPlayers - SearchResult.Session.NumOpenPublicConnections;
				Data.HostUsername = SearchResult.Session.OwningUserName;
				Data.PingMs = SearchResult.PingInMs;
				

				ServerData.Add(Data);
			}

			if(MainMenuWidget)
				MainMenuWidget->BP_InitializeSessionsList(ServerData);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnFindSessionsComplete] Error session not found"));
	}
	
	// if (Succeeded)
	// {
	// 	print(FString("		Search Success! "));
	// 	
	// 	TArray<FOnlineSessionSearchResult> SearchResults = SessionSearch->SearchResults;
	//
	// 	if (SearchResults.Num())
	// 	{
	// 		print(FString("Results.Num() > 0, Joining Sesion...."));
	// 		SessionInterface->JoinSession(0, FName("MyProjectFPSSession"), SearchResults[0]);
	// 	}
	// }
	// else
	// {
	// 	print(FString("		Search Failure! "));
	// }
}

void UMyGameInstance::OnJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnJoinSessionsComplete]"));


	if (!SessionInterface.IsValid()) return;

	FString Url;
	if (!SessionInterface->GetResolvedConnectString(SESSION_NAME, Url))
	{
		UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnJoinSessionsComplete] Couldn't get Connect String"));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnJoinSessionsComplete] Url: %s"), *Url);

	APlayerController* PlayerController = GetFirstLocalPlayerController();

	if (PlayerController == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[UMyGameInstance::OnJoinSessionsComplete] Player Controller DOESN'T EXIST"));
		return;
	}


	PlayerController->ClientTravel(Url, ETravelType::TRAVEL_Absolute);
}

void UMyGameInstance::CreateServer()
{
	UE_LOG(LogTemp, Warning, TEXT("CreateServer"));
	if (SessionInterface.IsValid())
	{
		FOnlineSessionSettings SessionSettings;

		// Switch between bIsLANMatch when using NULL subsystem
		if (IOnlineSubsystem::Get()->GetSubsystemName().ToString() == "NULL")
		{
			SessionSettings.bIsLANMatch = true;
		}
		else
		{
			SessionSettings.bIsLANMatch = false;
		}

		// Number of sessions
		SessionSettings.NumPublicConnections = 2;
		SessionSettings.bShouldAdvertise = true;
		SessionSettings.bUsesPresence = true;
		SessionSettings.bUseLobbiesIfAvailable = true;
		SessionSettings.Set<FString>(SERVER_NAME_SETTINGS_KEY,FString("TESTSERVERNAMELMAO"), EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

		SessionInterface->CreateSession(0, SESSION_NAME, SessionSettings);
	}  // TODO: expose name
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