// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "TPP_SPM_LedgeHang.generated.h"

/**
 * 
 */
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API UTPP_SPM_LedgeHang : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	float AlignmentSpeed = 10.0f;

protected:

	UPROPERTY(Transient)
	FHitResult ImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	bool bIsAligningToWall = false;

	UPROPERTY(Transient)
	FVector CachedLedgeDelta = FVector::ZeroVector;


public:

	UTPP_SPM_LedgeHang(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;
};
