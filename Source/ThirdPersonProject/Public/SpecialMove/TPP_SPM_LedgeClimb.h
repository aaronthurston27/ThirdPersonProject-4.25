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

protected:

	UPROPERTY(Transient)
	FHitResult TargetWallImpactResult;

	UPROPERTY(Transient)
	FVector TargetAttachPoint;

	UPROPERTY(Transient)
	FVector StartingClimbPosition = FVector::ZeroVector;

	UPROPERTY(Transient)
	FVector ClimbExitPoint;

	UPROPERTY(Transient)
	float ElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float LateralLerpElapsedTime = 0.0f;

	UPROPERTY(Transient)
	float AnimLength;

	UPROPERTY(Transient)
	float LateralAnimLength;

public:

	UTPP_SPM_LedgeClimb(const FObjectInitializer& ObjectInitializer);

	virtual void BeginSpecialMove_Implementation() override;

	virtual void Tick(float DeltaTime) override;

	virtual void EndSpecialMove_Implementation() override;

	void SetClimbExitPoint(const FVector& ExitPoint);

protected:

	virtual void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted) override;
	
};
