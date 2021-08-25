// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/RenderTargetWidget.h"

#include "Widgets/Images/SImage.h"

URenderTargetWidget::URenderTargetWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// FString Path = "Texture2D'/Game/UI/txt_LogoUE4.txt_LogoUE4'";
	// static ConstructorHelpers::FObjectFinder<UTexture2D> Texture(*Path);
	// DefaultTexture = Texture.Object;
	DefaultTexture = nullptr;
	
	ImageBrush = FSlateBrush();
	ImageBrush.SetResourceObject(DefaultTexture);

	RenderingMaterial = nullptr;
}


void URenderTargetWidget::SetRenderMaterial( UMaterialInterface* Material )
{
	if( Material != nullptr && RenderingMaterial != Material )
	{
		// Store new reference
		RenderingMaterial = Material;

		// Updating internal rendering brush
		ImageBrush.SetResourceObject(Material);
	}
}


TSharedRef<SWidget> URenderTargetWidget::RebuildWidget()
{
	if( !WidgetParent.IsValid() )
	{
		// Use an SInvalidationPanel if you want to cache
		// the image and its brush, but it won't allow
		// to update the material later (unless explicitly invalidated)
		/*
		WidgetParent =
		SNew(SInvalidationPanel).CacheRelativeTransforms(false)
		[
		SNew(SImage).Image( &ImageBrush )
		];
		*/

		WidgetParent = SNew(SImage).Image( &ImageBrush );
	}

	return WidgetParent.ToSharedRef();
}


void URenderTargetWidget::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	WidgetParent.Reset();
}