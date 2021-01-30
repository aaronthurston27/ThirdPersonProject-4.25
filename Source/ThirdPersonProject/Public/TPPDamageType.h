// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/DamageType.h"
#include "TPPDamageType.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPDamageType : public UDamageType
{
	GENERATED_BODY()

public:

	/** Multiplier to apply if damage is applied to head */
	UPROPERTY(EditDefaultsOnly)
	float DamageHeadshotMultiplier = 1.0f;
};
