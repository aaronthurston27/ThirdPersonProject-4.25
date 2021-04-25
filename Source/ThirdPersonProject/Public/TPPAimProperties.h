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

	/** Server lag compensation yaw threshold. If a client calculated hit result is within the range of the server calculated hit result by this angle, register a hit */
	UPROPERTY(EditDefaultsOnly)
	float ServerHitDetectionCompensationYaw = 25.0f;

	/** Server lag compensation pitch threshold. If a client calculated hit result is within the range of the server calculated hit result by this angle, register a hit */
	UPROPERTY(EditDefaultsOnly)
	float ServerHitDetectionCompensationPitch = 25.0f;

	/** Max angle for innacuracy */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float InaccuracySpreadMaxAngle = 3.0f;

	/** Alpha to use when lerping camera recoil to target value */
	UPROPERTY(EditDefaultsOnly)
	float AimRecoilInterpAlpha = 0.200001f;

	/** Time needed to restore recoil back to zero */
	UPROPERTY(EditDefaultsOnly)
	float AimRecoilRestorationInterpAlpha = 0.45f;

	/** Length for hitscan line traces */
	UPROPERTY(EditDefaultsOnly)
	float HitScanLength = 5000.f;
};
