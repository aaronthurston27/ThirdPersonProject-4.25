// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPSlideAnimNotify.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"
#include "TPPMovementComponent.h"

void UTPPSlideAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	AThirdPersonProjectCharacter* OwnerCharacter = Cast<AThirdPersonProjectCharacter>(MeshComp->GetOwner());
	UTPPMovementComponent* TPPMovementComponent = OwnerCharacter ? OwnerCharacter->GetTPPMovementComponent() : nullptr;
	if (TPPMovementComponent)
	{
		if (bIsStartingSlide)
		{
			TPPMovementComponent->SlideStarted();
		}
		else
		{
			TPPMovementComponent->SlideEnded();
		}
	}
}