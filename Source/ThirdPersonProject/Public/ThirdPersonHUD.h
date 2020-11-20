// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Components/WidgetComponent.h"
#include "TargetingWidget.h"
#include "ThirdPersonHUD.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API AThirdPersonHUD : public AHUD
{
	GENERATED_BODY()

public:

	AThirdPersonHUD();

	virtual void Tick(float deltaTime) override;

	virtual void BeginPlay() override;

	virtual void DrawHUD() override;

	UFUNCTION()
	void UpdateTargetLocation(const FVector& targetWorldLocation);

	UFUNCTION()
	void SetTargetingEnabled(bool isEnabled);
	
	UPROPERTY(EditDefaultsOnly, Category = "Widgets")
	TSubclassOf<UUserWidget> TargetingWidgetClass;

private:

	UTargetingWidget* TargetingWidget;

};
