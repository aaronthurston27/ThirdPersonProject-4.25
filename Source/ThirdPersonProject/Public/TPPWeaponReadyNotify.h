// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "TPPWeaponReadyNotify.generated.h"

/**
 * Anim notify for changing weapon ready state.
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPWeaponReadyNotify : public UAnimNotify
{
	GENERATED_BODY()

public:

	/** State to set the weapon when this notify is hit */
	UPROPERTY(EditDefaultsOnly)
	bool bShouldWeaponBeReady = true;

private:

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
