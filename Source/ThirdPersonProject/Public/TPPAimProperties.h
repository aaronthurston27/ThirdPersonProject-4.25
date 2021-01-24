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

	/** Delay between shots before beginning weapon recoil recovery */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WeaponRecoilRecoveryDelay = 0.8f;

	/** Recoil to apply to the camera per frame */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WeaponRecoilAccumulationPerFrame = .033;

	/** Recoil to recover per frame */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float WeaponRecoilRecoveryPerFrame = 2.0f;

	/** Amount to scale pitch input by when comensating for weapon recoil */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float RecoilCompensationScale = 2.0f;
	
};
