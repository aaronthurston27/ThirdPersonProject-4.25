// Fill out your copyright notice in the Description page of Project Settings.


#include "TPPSlideAnimNotify.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"
#include "TPPMovementComponent.h"

void UTPPSlideAnimNotify::Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation)
{
	ATPPPlayerCharacter* OwnerCharacter = Cast<ATPPPlayerCharacter>(MeshComp->GetOwner());
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