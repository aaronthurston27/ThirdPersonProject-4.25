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

	UPROPERTY(EditDefaultsOnly)
	float WallRunMaxVerticalDistance = 350.0f;

	UPROPERTY(EditDefaultsOnly)
	float WallRunVerticalSpeed = 110.f;

public:

	UPROPERTY(Transient)
	FHitResult TargetWallImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	FVector WallRunDestinationPoint = FVector::ZeroVector;

public:

	void SetWallRunDestination(const FVector& WallRunDestination);

public:

	UTPP_SPM_WallRun(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void EndSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

protected:

	void OnWallRunDestinationReached();
	
};
