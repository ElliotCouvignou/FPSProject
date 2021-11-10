// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/WaitChangeFOV.h"
#include "Camera/CameraComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Curves/CurveFloat.h"



UWaitChangeFOV::UWaitChangeFOV(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bTickingTask = true;
	bIsFinished = false;
}

UWaitChangeFOV* UWaitChangeFOV::WaitChangeFOV(UGameplayAbility* OwningAbility, FName TaskInstanceName, class UCameraComponent* CameraComponent, float TargetFOV, float Duration, UCurveFloat* OptionalInterpolationCurve)
{
	UWaitChangeFOV* MyObj = NewAbilityTask<UWaitChangeFOV>(OwningAbility, TaskInstanceName);

	MyObj->CameraComponent = CameraComponent;
	if (CameraComponent != nullptr)
	{
		MyObj->StartFOV = MyObj->CameraComponent->FieldOfView;
	}

	MyObj->TargetFOV = TargetFOV;
	MyObj->Duration = FMath::Max(Duration, 0.001f);		// Avoid negative or divide-by-zero cases
	MyObj->TimeChangeStarted = MyObj->GetWorld()->GetTimeSeconds();
	MyObj->TimeChangeWillEnd = MyObj->TimeChangeStarted + MyObj->Duration;
	MyObj->LerpCurve = OptionalInterpolationCurve;
	
	return MyObj;
}

UWaitChangeFOV* UWaitChangeFOV::WaitChangeMaterialFOV(UObject* WorldContextObject, UGameplayAbility* OwningAbility,
	FName TaskInstanceName, float TargetFOV, float Duration, UCurveFloat* OptionalInterpolationCurve,
	UMaterialParameterCollection* WeaponFOVMaterial)
{
	UWaitChangeFOV* MyObj = NewAbilityTask<UWaitChangeFOV>(OwningAbility, TaskInstanceName);

	MyObj->OptionalWeaponFOVMaterial = WeaponFOVMaterial;
	if (WeaponFOVMaterial != nullptr && WorldContextObject)
	{
		MyObj->StartFOV = UKismetMaterialLibrary::GetScalarParameterValue(WorldContextObject, WeaponFOVMaterial, FName("FOV"));
	}

	MyObj->TargetFOV = TargetFOV;
	MyObj->Duration = FMath::Max(Duration, 0.001f);		// Avoid negative or divide-by-zero cases
	MyObj->TimeChangeStarted = MyObj->GetWorld()->GetTimeSeconds();
	MyObj->TimeChangeWillEnd = MyObj->TimeChangeStarted + MyObj->Duration;
	MyObj->LerpCurve = OptionalInterpolationCurve;
	MyObj->WorldContenxt = WorldContextObject;
	
	return MyObj;
}


void UWaitChangeFOV::Activate()
{
}

void UWaitChangeFOV::TickTask(float DeltaTime)
{
	if (bIsFinished)
	{
		return;
	}

	Super::TickTask(DeltaTime);

	if (CameraComponent)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();

		if (CurrentTime >= TimeChangeWillEnd)
		{
			bIsFinished = true;

			CameraComponent->SetFieldOfView(TargetFOV);
			
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				OnTargetFOVReached.Broadcast();
			}
			EndTask();
		}
		else
		{
			float NewFOV;

			float MoveFraction = (CurrentTime - TimeChangeStarted) / Duration;
			if (LerpCurve)
			{
				MoveFraction = LerpCurve->GetFloatValue(MoveFraction);
			}

			NewFOV = FMath::Lerp<float, float>(StartFOV, TargetFOV, MoveFraction);

			CameraComponent->SetFieldOfView(NewFOV);
		}
	}
	else if(OptionalWeaponFOVMaterial)
	{
		float CurrentTime = GetWorld()->GetTimeSeconds();
		
		if (CurrentTime >= TimeChangeWillEnd)
		{
			bIsFinished = true;

			UKismetMaterialLibrary::SetScalarParameterValue(WorldContenxt, OptionalWeaponFOVMaterial, FName("FOV"), TargetFOV);
			
			if (ShouldBroadcastAbilityTaskDelegates())
			{
				OnTargetFOVReached.Broadcast();
			}
			EndTask();
		}
		else
		{
			float NewFOV;

			float MoveFraction = (CurrentTime - TimeChangeStarted) / Duration;
			if (LerpCurve)
			{
				MoveFraction = LerpCurve->GetFloatValue(MoveFraction);
			}

			NewFOV = FMath::Lerp<float, float>(StartFOV, TargetFOV, MoveFraction);

			UKismetMaterialLibrary::SetScalarParameterValue(WorldContenxt, OptionalWeaponFOVMaterial, FName("FOV"), NewFOV);
		}
	}
	else
	{
		bIsFinished = true;
		EndTask();
	}
}

void UWaitChangeFOV::OnDestroy(bool AbilityIsEnding)
{
	Super::OnDestroy(AbilityIsEnding);
}
