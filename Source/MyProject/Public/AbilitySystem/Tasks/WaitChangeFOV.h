// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask.h"
#include "WaitChangeFOV.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE(FChangeFOVDelegate);


/**
 * 
 */
UCLASS()
class MYPROJECT_API UWaitChangeFOV : public UAbilityTask
{
	GENERATED_UCLASS_BODY()
	
	UPROPERTY(BlueprintAssignable)
	FChangeFOVDelegate OnTargetFOVReached;

	// Change the FOV to the specified value, using the float curve (range 0 - 1) or fallback to linear interpolation
	UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
	static UWaitChangeFOV* WaitChangeFOV(UGameplayAbility* OwningAbility, FName TaskInstanceName, class UCameraComponent* CameraComponent, float TargetFOV, float Duration, UCurveFloat* OptionalInterpolationCurve);

	virtual void Activate() override;

	// Tick function for this task, if bTickingTask == true
	virtual void TickTask(float DeltaTime) override;

	virtual void OnDestroy(bool AbilityIsEnding) override;

	protected:
	bool bIsFinished;

	float StartFOV;

	float TargetFOV;

	float Duration;

	float TimeChangeStarted;

	float TimeChangeWillEnd;

	class UCameraComponent* CameraComponent;

	UCurveFloat* LerpCurve;
	
};
