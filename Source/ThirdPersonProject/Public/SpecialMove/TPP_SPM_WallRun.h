// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "TPP_SPM_WallRun.generated.h"

/**
 * 
 */
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API UTPP_SPM_WallRun : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	/** Max vertical distance to travel. Enabled only if move is not duration based */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!bDurationBased"))
	float WallRunMaxVerticalDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly)
	float WallRunVerticalSpeed = 110.f;

public:

	UPROPERTY(Transient)
	FHitResult TargetWallImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	float WallLedgeHeight = 0.0f;

	UPROPERTY(Transient)
	FVector WallRunDestinationPoint = FVector::ZeroVector;

public:

	UTPP_SPM_WallRun(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void EndSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	void SetWallRunProperties(const FHitResult& WallTraceHitResult, const FVector& AttachPoint, const float LedgeHeight);

protected:

	void OnWallRunDestinationReached();

	virtual void OnDurationExceeded_Implementation() override;
	
};
