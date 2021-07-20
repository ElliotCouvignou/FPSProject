// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/Tasks/WaitDelayOneFrame.h"


UWaitDelayOneFrame::UWaitDelayOneFrame(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UWaitDelayOneFrame::Activate()
{
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UWaitDelayOneFrame::OnDelayFinish);
}

UWaitDelayOneFrame* UWaitDelayOneFrame::WaitDelayOneFrame(UGameplayAbility* OwningAbility)
{
	UWaitDelayOneFrame* MyObj = NewAbilityTask<UWaitDelayOneFrame>(OwningAbility);
	return MyObj;
}

void UWaitDelayOneFrame::OnDelayFinish()
{
	if (ShouldBroadcastAbilityTaskDelegates())
	{
		OnFinish.Broadcast();
	}
	EndTask();
}