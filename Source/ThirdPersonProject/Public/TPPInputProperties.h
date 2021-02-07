// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "TPPPlayerController.h"
#include "TPPInputProperties.generated.h"

/** Struct defining input action properties */
USTRUCT(Blueprintable)
struct FTPPInputAction
{
	GENERATED_BODY()

public:

	/** Enum associated with this action */
	UPROPERTY(EditDefaultsOnly)
	EPlayerInputAction InputAction;

	/** Key this input action is bound to */
	UPROPERTY(EditDefaultsOnly)
	FKey InputKey = FKey();

	/** True if this input is triggered after being held */
	UPROPERTY(EditDefaultsOnly)
	bool bIsKeyHold = false;

	/** Time to hold button for input action to register */
	UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "bIsKeyHold"))
	float KeyHoldTime = 0.0f;
};

/**
 * Holds properties for consuming input from controller. 
 */
UCLASS(Abstract, Blueprintable, BlueprintType)
class THIRDPERSONPROJECT_API UTPPInputProperties : public UDataAsset
{
	GENERATED_BODY()

public:

	/** Input actions defined for the game */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FTPPInputAction> DefaultInputActions;
	
};
