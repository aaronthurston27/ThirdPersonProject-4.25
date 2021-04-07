// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "TPP_SPM_LedgeHang.generated.h"

class UTPP_SPM_LedgeClimb;

/**
 * 
 */
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API UTPP_SPM_LedgeHang : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	/** Delay in seconds before player can make an action (wall jump, drop, climb, etc). */
	UPROPERTY(EditDefaultsOnly)
	float LedgeHangActionDelay = 1.0f;

	UTPP_SPM_LedgeHang(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void EndSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

protected:

	UPROPERTY(Transient)
	float DelayTimer = 0.0f;
};
