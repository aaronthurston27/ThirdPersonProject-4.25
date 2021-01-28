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

	/** Alpha to use when lerping camera recoil to target value */
	UPROPERTY(EditDefaultsOnly)
	float AimRecoilInterpAlpha = 0.200001f;

	/** Time needed to restore recoil back to zero */
	UPROPERTY(EditDefaultsOnly)
	float AimRecoilRestorationTime = 0.45f;
};
