// Fill out your copyright notice in the Description page of Project Settings.


#include "AbilitySystem/DamageExecutionCalculation.h"
#include "AbilitySystem/AttributeSets/PlayerAttributeSet.h"
#include "AbilitySystem/FPSAbilitySystemComponent.h"
#include "Curves/CurveFloat.h"
#include "Actors/FPSWeapon.h"


#define print(text) if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 60, FColor::Green,text)



// Declare the attributes to capture and define how we want to capture them from the Source and Target.
struct FPSDamageStatics
{
	DECLARE_ATTRIBUTE_CAPTUREDEF(Armor);
	DECLARE_ATTRIBUTE_CAPTUREDEF(Damage);

	FPSDamageStatics()
	{
		// Snapshot happens at time of GESpec creation

		// We're not capturing anything from the Source in this example, but there could be like AttackPower attributes that you might want.

		// Capture optional Damage set on the damage GE as a CalculationModifier under the ExecutionCalculation
		DEFINE_ATTRIBUTE_CAPTUREDEF(UPlayerAttributeSet, Damage, Source, true);

		// Capture the Target's Armor. Don't snapshot.
		//DEFINE_ATTRIBUTE_CAPTUREDEF(UPlayerAttributeSet, Armor, Target, false);
	}
};

static const FPSDamageStatics& DamageStatics()
{
	static FPSDamageStatics DStatics;
	return DStatics;
}

UDamageExecutionCalculation::UDamageExecutionCalculation()
{

	RelevantAttributesToCapture.Add(DamageStatics().DamageDef);
	//RelevantAttributesToCapture.Add(DamageStatics().ArmorDef);
}

void UDamageExecutionCalculation::Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, OUT FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	UAbilitySystemComponent* TargetAbilitySystemComponent = ExecutionParams.GetTargetAbilitySystemComponent();
	UAbilitySystemComponent* SourceAbilitySystemComponent = ExecutionParams.GetSourceAbilitySystemComponent();

	AActor* SourceActor = SourceAbilitySystemComponent ? SourceAbilitySystemComponent->GetAvatarActor() : nullptr;
	AActor* TargetActor = TargetAbilitySystemComponent ? TargetAbilitySystemComponent->GetAvatarActor() : nullptr;

	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();
	FGameplayTagContainer AssetTags;
	Spec.GetAllAssetTags(AssetTags);

	// Gather the tags from the source and target as that can affect which buffs should be used
	const FGameplayTagContainer* SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	const FGameplayTagContainer* TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	FAggregatorEvaluateParameters EvaluationParameters;
	EvaluationParameters.SourceTags = SourceTags;
	EvaluationParameters.TargetTags = TargetTags;

	float Damage = 0.0f;

	// TODO: get distance from start position to hit, not distance btw ppl when hit (only works cause hitcasts)
	float Distance = SourceActor->GetDistanceTo(TargetActor);

	
	// float Armor = 0.0f;
	// ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().ArmorDef, EvaluationParameters, Armor);
	// Armor = FMath::Max<float>(Armor, 0.0f);

	
	// Capture optional damage value set on the damage GE as a CalculationModifier under the ExecutionCalculation
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DamageDef, EvaluationParameters, Damage);
	// Add SetByCaller damage if it exists
	Damage += FMath::Max<float>(Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.Damage")), false, -1.0f), 0.0f);

	
	// If this is a gun then get damage from curve if valid
	AFPSWeapon* FpsWeapon = Cast<AFPSWeapon>(Spec.GetEffectContext().GetSourceObject());
	UCurveFloat* DamageCurve;
	if(FpsWeapon)
	{
		DamageCurve = FpsWeapon->DamageCurve;
	
		if(DamageCurve)
		{
			Damage = DamageCurve->GetFloatValue(Distance);
		}
	}
	
	const float HeadShotMultiplier = Spec.GetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Data.HeadshotMultiplier")), false, 1.0f);

	
	float UnmitigatedDamage = Damage; // Can multiply any damage boosters here

	// Check for headshot. There's only one character mesh here, but you could have a function on your Character class to return the head bone name
	const FHitResult* Hit = Spec.GetContext().GetHitResult();															/* TODO: replace this whis something less magical */
	if (AssetTags.HasTagExact(FGameplayTag::RequestGameplayTag(FName("Effect.Damage.CanHeadShot"))) && Hit && Hit->BoneName == "head")
	{
		print(FString("Headshot, mult: " + FString::SanitizeFloat(HeadShotMultiplier,2)));
	
		UnmitigatedDamage *= HeadShotMultiplier;
		FGameplayEffectSpec* MutableSpec = ExecutionParams.GetOwningSpecForPreExecuteMod();
		MutableSpec->DynamicAssetTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Effect.Damage.HeadShot")));
	}

	float MitigatedDamage = UnmitigatedDamage; //(UnmitigatedDamage) * (100 / (100 + Armor));

	
	if (MitigatedDamage > 0.f)
	{
		
		// Set the Target's damage meta attribute
		OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(DamageStatics().DamageProperty, EGameplayModOp::Additive, MitigatedDamage));
	}
}

