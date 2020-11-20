// Fill out your copyright notice in the Description page of Project Settings.


#include "TargetingWidget.h"

UTargetingWidget::UTargetingWidget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	
}

void UTargetingWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UTargetingWidget::EnableCrosshair(bool isEnabled)
{
	if (CrosshairImage)
	{
		CrosshairImage->SetVisibility(isEnabled ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}
}
