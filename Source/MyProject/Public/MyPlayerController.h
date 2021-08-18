// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "UI/UIInfoTypes.h"
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

	UPROPERTY(BlueprintReadOnly, VisibleAnywhere)
	UMainGameplayWidget* MainGameplayWidget;

	
	UFUNCTION(BlueprintCallable)
	void CreateMainGameplayWidget();

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnPlayerDamageDone(const FDamageInfoStruct& InfoStruct);
	void OnPlayerDamageDone_Implementation(const FDamageInfoStruct& InfoStruct);
	bool OnPlayerDamageDone_Validate(const FDamageInfoStruct& InfoStruct) { return true; }

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnPlayerDamageTaken(const AActor* Shooter);
	void OnPlayerDamageTaken_Implementation(const AActor* Shooter);
	bool OnPlayerDamageTaken_Validate(const AActor* Shooter) { return true; }
	
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent)
	void BP_ClientOnPlayerDamageDone(const FDamageInfoStruct& InfoStruct);

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnPlayerKilled(const FKillInfoStruct& InfoStruct);
	void OnPlayerKilled_Implementation(const FKillInfoStruct& InfoStruct);
	bool OnPlayerKilled_Validate(const FKillInfoStruct& InfoStruct) { return true; }

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnPlayerDied(const FDiedInfoStruct& InfoStruct);
	void OnPlayerDied_Implementation(const FDiedInfoStruct& InfoStruct);
	bool OnPlayerDied_Validate(const FDiedInfoStruct& InfoStruct) { return true; }

	/* Called to display UI when someone killed someone else that this player isn't related to (on kill/victim) */
	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnSomeoneDied(const FPlayerDeathInfoStruct& InfoStruct);
	void OnSomeoneDied_Implementation(const FPlayerDeathInfoStruct& InfoStruct);
	bool OnSomeoneDied_Validate(const FPlayerDeathInfoStruct& InfoStruct) { return true; }

	UFUNCTION(BlueprintCallable, Client, WithValidation, Reliable)
	void OnMatchEnded();
	void OnMatchEnded_Implementation();
	bool OnMatchEnded_Validate() { return true; }

	/** Returns the network-synced time from the server.
	* Corresponds to GetWorld()->GetTimeSeconds()
	* on the server. This doesn't actually make a network
	* request; it just returns the cached, locally-simulated
	* and lag-corrected ServerTime value which was synced
	* with the server at the time of this PlayerController's
	* last restart. */
	virtual float GetServerTime() { return ServerTime; }

	virtual void ReceivedPlayer() override;

protected:

	UPROPERTY(BlueprintReadWrite, EditDefaultsOnly)
	TSubclassOf<UMainGameplayWidget> MainGameplayWidgetClass;
	

	/** Reports the current server time to clients in response
	* to ServerRequestServerTime */
	UFUNCTION(Client, Reliable)
	void ClientReportServerTime(float requestWorldTime,	float serverTime);

	/** Requests current server time so accurate lag
	* compensation can be performed in ClientReportServerTime
	* based on the round-trip duration */
	UFUNCTION(Server, Reliable, WithValidation)
	void ServerRequestServerTime(APlayerController* requester, float requestWorldTime);

	float ServerTime = 0.0f;




	
};
