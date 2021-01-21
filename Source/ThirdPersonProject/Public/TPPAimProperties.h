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

	/** Max vertical angle for weapon recoil */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WeaponRecoilMaxVerticalAngle = 11.0f;

	/** Recoil recovery per frame */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WeaponRecoilRecovery = 8.f;

	/** Camera angle recoil factor that is added to the final pitch of the camera. Total camera pitch should be: OriginalPitch + (AccumulatedRecoil * RecoilOffset) */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float CameraRecoilFactor = .85f;
	
};
