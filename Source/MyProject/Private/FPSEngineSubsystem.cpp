// Fill out your copyright notice in the Description page of Project Settings.


#include "FPSEngineSubsystem.h"
#include "AbilitySystemGlobals.h"

void UFPSEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{

	Super::Initialize(Collection);

	UAbilitySystemGlobals::Get().InitGlobalData();
}
