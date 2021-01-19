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

	/** Max angle for innacuracy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InaccuracySpreadMaxAngle = 3.0f;

	/** Inaccuracy angle to use when standing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StandingAimSpreadAngle = 1.1;

	/** Inaccuracy angle to use when the player is crouching */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CrouchingAimSpreadAngle = .5f;

	/** Multiplier when aiming down the sights */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float ADSAimMultiplier = .40f;
	
};
