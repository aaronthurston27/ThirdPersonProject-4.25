// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "TPPCrosshairWidget.h"
#include "TPPHUD.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API ATPPHUD : public AHUD
{
	GENERATED_BODY()

public:

	virtual void BeginPlay() override;

public:

	UPROPERTY(EditDefaultsOnly, Category = "Widget Classes")
	TSubclassOf<UUserWidget> CrosshairWidgetClass;

protected:

	UPROPERTY(Transient)
	UTPPCrosshairWidget* CrosshairWidget;
	
};
