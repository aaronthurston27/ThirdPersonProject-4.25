// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Image.h"
#include "TargetingWidget.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UTargetingWidget : public UUserWidget
{
	GENERATED_BODY()

public:

    UTargetingWidget(const FObjectInitializer& ObjectInitializer);

    // Used like BeginPlay
    virtual void NativeConstruct() override;

    void EnableCrosshair(bool isEnabled);

private:

    // meta=BindWidget - Binds the property to the object found in the widget
    UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (BindWidget), meta = (AllowPrivateAccess = true))
    class UImage* CrosshairImage;
};
