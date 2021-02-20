// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "TPPWeaponReloadNotify.generated.h"

/**
 * Notify for weapon reload related animations.
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPWeaponReloadNotify : public UAnimNotify
{
	GENERATED_BODY()

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
};
