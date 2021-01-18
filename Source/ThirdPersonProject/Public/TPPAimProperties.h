// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "TPPAimProperties.generated.h"

/**
 * Properties for character aiming and shooting. 
 */
UCLASS(Blueprintable,BlueprintType)
class THIRDPERSONPROJECT_API UTPPAimProperties : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Max circle radius for innacuracy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InaccuracySpreadMaxRadius = 20.f;

	/** Inaccuracy circle radius to use when the player is standing */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float StandingAimSpreadRadius = 11.0f;

	/** Inaccuracy circle radius to use when the player is crouching */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CrouchingAimSpreadRadius = 7.0f;
	
};
