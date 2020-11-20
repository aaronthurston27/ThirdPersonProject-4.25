// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "TPPSlideAnimNotify.generated.h"

/**
 * Anim notify for starting/ending slide
 */
UCLASS()
class THIRDPERSONPROJECT_API UTPPSlideAnimNotify : public UAnimNotify
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	bool bIsStartingSlide = false;

	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation) override;
	
};
