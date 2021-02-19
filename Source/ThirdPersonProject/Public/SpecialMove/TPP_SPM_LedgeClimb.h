// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SpecialMove/TPPSpecialMove.h"
#include "TPP_SPM_LedgeClimb.generated.h"

/**
 * 
 */
UCLASS(Abstract,Blueprintable)
class THIRDPERSONPROJECT_API UTPP_SPM_LedgeClimb : public UTPPSpecialMove
{
	GENERATED_BODY()

public:

	UPROPERTY(EditDefaultsOnly)
	UAnimMontage* ClimbMontage;

	UPROPERTY(EditDefaultsOnly)
	FVector WallAttachOffset;

protected:

	UPROPERTY(Transient)
	FHitResult TargetWallImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	FVector ClimbExitPoint;

public:

	UTPP_SPM_LedgeClimb(const FObjectInitializer& ObjectInitializer);

	void SetLedgeClimbParameters(const FHitResult& WallImpactResult, const FVector& WallAttachPoint, const FVector& ExitPoint);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;

protected:

	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;
	
};