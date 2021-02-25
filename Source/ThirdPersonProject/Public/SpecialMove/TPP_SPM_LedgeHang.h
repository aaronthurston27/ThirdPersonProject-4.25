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

	/** Minimum dot product of desired movement direction and wall normal to end hang */
	UPROPERTY(EditDefaultsOnly)
	float EndHangInputDot = .8f;

	/** Minimum dot product of desired movement direction and wall normal to climg the ledge that is held onto. */
	UPROPERTY(EditDefaultsOnly)
	float HangToClimbInputDot = .7f;

	/** Ledge climb class to use when going from hanging to climbing */
	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<UTPP_SPM_LedgeClimb> LedgeClimbClass;

protected:

	UPROPERTY(Transient)
	FHitResult ImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	bool bIsAligningToWall = false;

	UPROPERTY(Transient)
	FVector CachedLedgeDelta = FVector::ZeroVector;

	UPROPERTY(Transient)
	float ElapsedTime = 0.0f;


public:

	UTPP_SPM_LedgeHang(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;

	void SetLedgeHangProperties(const FHitResult& WallTraceHitResult, const FVector& AttachPoint);
};
