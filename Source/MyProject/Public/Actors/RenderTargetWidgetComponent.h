// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Slate/WidgetRenderer.h"
#include "Widgets/SVirtualWindow.h"

#include "RenderTargetWidgetComponent.generated.h"


UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class MYPROJECT_API URenderTargetWidgetComponent : public USceneComponent
{
	GENERATED_UCLASS_BODY()

public:
	virtual void Init();

	void Render( float DeltaTime = 0.0f );

	void Resize( FIntPoint& NewSize );

	virtual void BeginPlay() override;

	protected:
	virtual void OnUnregister() override;


	private:
	// The cached window containing the rendering widget
	TSharedPtr<SVirtualWindow>  SlateWindow;
	TSharedPtr<FHittestGrid>    SlateGrid;
	FGeometry SlateGeometry;

	void UpdateSlateWindow();

	UPROPERTY(transient)
	UTextureRenderTarget2D* ScriptedTexture;

	UPROPERTY(transient)
	UUserWidget* RenderingWidget;

	FWidgetRenderer* Renderer;

		
};
