// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/MyPlayerStart.h"

#include "Actors/Characters/MyProjectCharacter.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"



float AMyPlayerStart::GetCurrentSpawnPoints()
{
	float Ret = 0.f;
	TArray<AActor*>Out;
	
	// GetActors to test on
	// TArray<TEnumAsByte<EObjectTypeQuery>> Objects;   	Objects.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	// TArray<AActor*> Ignores;   							Ignores.Add(this);
	//
	// 
	// UKismetSystemLibrary::SphereOverlapActors(GetWorld(),GetActorLocation(), TestArea, Objects, AMyProjectCharacter::StaticClass(), Ignores, Out);

	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AMyProjectCharacter::StaticClass(), Out);
	for(auto e : Out)
	{
		float Dist = GetDistanceTo(e);

		Ret += DistanceBasePoints * PointDistanceCoeff / Dist;
		
		// fire linetrace to test LOS
		FHitResult Result;
		FCollisionQueryParams CollisionParam;
		CollisionParam.AddIgnoredActor(this);

		FVector OutLocation = GetActorLocation() + FVector(0.f, 0.f, 50.f);
		FRotator OutRotation = UKismetMathLibrary::FindLookAtRotation(OutLocation, Result.Location);
		
		GetWorld()->LineTraceSingleByProfile(Result, OutLocation, OutLocation + OutRotation.Vector() * TestArea, FName("Pawn"), CollisionParam);
		if(Result.GetActor() == e)
		{
			// spawn has LOS of actor, add points
			Ret += LOSBasePoints * PointDistanceLOSCoeff / Dist;
		}
	}

	return Ret;
}

