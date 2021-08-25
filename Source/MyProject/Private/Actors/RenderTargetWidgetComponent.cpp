// Fill out your copyright notice in the Description page of Project Settings.


#include "Actors/RenderTargetWidgetComponent.h"

#include "Blueprint/UserWidget.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Input/HittestGrid.h"
#include "UI/RenderTargetWidget.h"


// Constructor
URenderTargetWidgetComponent::URenderTargetWidgetComponent(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    ScriptedTexture = nullptr;
}

// Begin play, setup of the Slate virtual window
void URenderTargetWidgetComponent::BeginPlay()
{
    Super::BeginPlay();

    if( FSlateApplication::IsInitialized() )
    {
        SlateWindow = SNew(SVirtualWindow).Size( FVector2D(256.0,256.0) );
        SlateGrid   = MakeShareable( new FHittestGrid() );
    }

    check( SlateWindow.IsValid() );
}


// Cleanup any Slate references when the component is being destroyed
void URenderTargetWidgetComponent::OnUnregister()
{
    Super::OnUnregister();

    if( SlateGrid.IsValid() )
    {
        SlateGrid.Reset();
    }

    if ( SlateWindow.IsValid() )
    {
        if( FSlateApplication::IsInitialized() )
        {
            FSlateApplication::Get().UnregisterVirtualWindow( SlateWindow.ToSharedRef() );
        }

        SlateWindow.Reset();
    }

    ScriptedTexture = nullptr;
    RenderingWidget = nullptr;
}


// Create the Render Target resource and the User Widget for rendering
void URenderTargetWidgetComponent::Init()
{
    // Create widget to render into RTT
    // Load a class from a blueprint object,
    // Don't forget to add "_C" at the end to get the class
    if(!RenderingWidgetClass)
        return;
    
    RenderingWidget = CreateWidget<UUserWidget>( GetWorld(), URenderTargetWidget::StaticClass() );

    ScriptedTexture = ScriptedTextureReference;
    if(!ScriptedTextureReference)
    {
        // Create render target resource
        FString Name = GetName() + "_ScriptTxt";
        ScriptedTexture = NewObject<UTextureRenderTarget2D>(this, UTextureRenderTarget2D::StaticClass(), *Name);
        check( ScriptedTexture );

        ScriptedTexture->RenderTargetFormat = ETextureRenderTargetFormat::RTF_RGBA8;
        ScriptedTexture->SizeX      = 256;
        ScriptedTexture->SizeY      = 256;
        ScriptedTexture->ClearColor = FLinearColor::Transparent;

        ScriptedTexture->UpdateResource();
    }

    // Slate setup
    Renderer = new FWidgetRenderer(false, true); //bool bUseGammaCorrection, bool bInClearTarget

    if( FSlateApplication::IsInitialized() )
    {
        FSlateApplication::Get().RegisterVirtualWindow( SlateWindow.ToSharedRef() );
    }

    UpdateSlateWindow();
}


// Setup the Slate window with the widget
void URenderTargetWidgetComponent::UpdateSlateWindow()
{
    SlateWindow->SetContent( RenderingWidget->TakeWidget() );
    SlateWindow->Resize( FVector2D(256, 256) );
    SlateGeometry = FGeometry::MakeRoot( FVector2D( 256, 256 ), FSlateLayoutTransform(1.0f));
}


// Render/Draw the texture
void URenderTargetWidgetComponent::Render( float DeltaTime )
{
    // Use the FWidgetRenderer to Draw the Slate 
    // window and its widget into the texture.
    // Replace:
    //    SlateWindow.ToSharedRef() 
    // by:
    //    *SlateGrid.Get()
    // if you compile with UE4 4.25
    Renderer->DrawWindow(
        ScriptedTexture->GameThread_GetRenderTargetResource(),  // FRenderTarget* RenderTarget
        *SlateGrid.Get(),                                // TSharedRef<FHittestGrid> HitTestGrid
        SlateWindow.ToSharedRef(),                              // TSharedRef<SWindow> Window
        SlateGeometry,                                          // FGeometry WindowGeometry
        SlateGeometry.GetLayoutBoundingRect(),                  // FSlateRect WindowClipRect
        DeltaTime,                                              // float DeltaTime
        false                                                   // bool bDeferRenderTargetUpdate
    );

    // Generate the MipMaps if needed
    // ScriptedTexture->UpdateResourceImmediate( false );
}


// Resize the render target and update the Slate window
// Note: the UpdateSlateWindow() use an hardcoded size
// so be sure to adjust the code to pass the right size
// to the window as well.
void URenderTargetWidgetComponent::Resize( FIntPoint& NewSize )
{
    if( ScriptedTexture != nullptr )
    {
        // Resizes the render target without recreating 
        // the FTextureResource. It might crash if you are 
        // using MipMaps because of an engine bug, in that 
        // case use UpdateResource() instead.
        // This issue should be fixed with UE4 4.26.
        ScriptedTexture->ResizeTarget( NewSize.X, NewSize.Y );

        // Recreate the Slate window used for rendering (since the size changed)
        UpdateSlateWindow();
    }
}