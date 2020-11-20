// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotifyState.h"
#include "AttackStartNotifyState.generated.h"

/**
 * 
 */
UCLASS()
class THIRDPERSONPROJECT_API UAttackStartNotifyState : public UAnimNotifyState
{
	GENERATED_BODY()

	void NotifyBegin(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float TotalDuration) override;

	void NotifyEnd(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;

	void NotifyTick(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, float FrameDeltaTime) override;

	bool IsKeyboardEnabled;
};
