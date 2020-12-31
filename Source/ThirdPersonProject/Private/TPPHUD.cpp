// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPHUD.h"

void ATPPHUD::BeginPlay()
{
	Super::BeginPlay();

	if (CrosshairWidgetClass)
	{
		CrosshairWidget = CreateWidget<UTPPCrosshairWidget>(GetWorld(), CrosshairWidgetClass);

		if (CrosshairWidget)
		{
			CrosshairWidget->AddToViewport();
		}
	}
}
