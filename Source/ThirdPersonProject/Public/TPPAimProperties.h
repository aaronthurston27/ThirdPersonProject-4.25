// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TPPAimProperties.generated.h"

/**
 * Properties for character aiming and shooting. 
 */
UCLASS(Abstract,Blueprintable,BlueprintType)
class THIRDPERSONPROJECT_API UTPPAimProperties : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Max circle radius for innacuracy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InaccuracySpreadMaxRadius = 200.f;

	/** Inaccuracy circle radius to use when the player is standing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StandingAimSpreadRadius = 110.0f;

	/** Inaccuracy circle radius to use when the player is crouching */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CrouchingAimSpreadRadius = 70.0f;

	/** Multiplier when aiming down the sights */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ADSAimMultiplier = .30f;

	/** Ratio of velocity to weapon spread. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MovementSpeedToWeaponSpreadRatio = .1714f;
	
};
