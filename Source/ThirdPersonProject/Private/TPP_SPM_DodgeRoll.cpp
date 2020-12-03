// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_SPM_DodgeRoll.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "ThirdPersonProject/ThirdPersonProjectCharacter.h"

UTPP_SPM_DodgeRoll::UTPP_SPM_DodgeRoll(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	CachedRollDirection = FVector::ZeroVector;
}

void UTPP_SPM_DodgeRoll::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	ATPPPlayerController* PlayerController = Cast<ATPPPlayerController>(OwningCharacter->GetController());
	if (PlayerController)
	{
		CachedRollDirection = PlayerController->GetDesiredMovementDirection();
	}

	if (AnimMontage)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(AnimMontage);
	}
}

void UTPP_SPM_DodgeRoll::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UTPP_SPM_DodgeRoll::EndSpecialMove_Implementation()
{
	if (OwningCharacter)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
		
	}

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_DodgeRoll::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	if (AnimMontage == Montage)
	{
		EndSpecialMove();
	}

	Super::OnMontageEnded(Montage, bInterrupted);
}
