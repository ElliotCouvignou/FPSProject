// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RenderTargetWidget.generated.h"

/**
 * 
 */
UCLASS()
class MYPROJECT_API URenderTargetWidget : public UUserWidget
{
	GENERATED_UCLASS_BODY()

	public:
	void SetRenderMaterial( UMaterialInterface* Material );

	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	private:
	TSharedPtr<SWidget> WidgetParent;

	UPROPERTY()
	UMaterialInterface* RenderingMaterial;

	UPROPERTY()
	FSlateBrush ImageBrush;

	UPROPERTY()
	UTexture2D* DefaultTexture;
	
};
