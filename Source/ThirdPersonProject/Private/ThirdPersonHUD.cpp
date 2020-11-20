// Fill out your copyright notice in the Description page of Project Settings.


#include "ThirdPersonHUD.h"
#include "Kismet/GameplayStatics.h"


AThirdPersonHUD::AThirdPersonHUD()
{

}

void AThirdPersonHUD::Tick(float deltaTime)
{
	Super::Tick(deltaTime);
}

void AThirdPersonHUD::BeginPlay()
{
	Super::BeginPlay();
	if (TargetingWidgetClass)
	{
		TargetingWidget = CreateWidget<UTargetingWidget>(GetOwningPlayerController(), TargetingWidgetClass);
		if (TargetingWidget)
		{
			TargetingWidget->AddToViewport(0);
			FVector2D alignment(.5f, .5f);
			TargetingWidget->SetAlignmentInViewport(alignment);
			SetTargetingEnabled(false);
		}
	}
}

void AThirdPersonHUD::DrawHUD()
{
	Super::DrawHUD();
}

void AThirdPersonHUD::UpdateTargetLocation(const FVector& targetWorldLocation)
{
	if (TargetingWidget)
	{
		FVector2D screenPosition;
		UGameplayStatics::ProjectWorldToScreen(GetOwningPlayerController(), targetWorldLocation, screenPosition, false);
		TargetingWidget->SetPositionInViewport(screenPosition);
	}
}

void AThirdPersonHUD::SetTargetingEnabled(bool isEnabled)
{
	if (TargetingWidget)
	{
		TargetingWidget->EnableCrosshair(isEnabled);
	}
}
