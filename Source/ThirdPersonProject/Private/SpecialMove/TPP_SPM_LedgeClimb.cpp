// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_LedgeClimb.h"
#include "TPPMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_LedgeClimb::UTPP_SPM_LedgeClimb(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesSprint = true;
	bDisablesJump = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_LedgeClimb::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	if (ClimbMontage)
	{
		OwningCharacter->ServerSetAnimRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		OwningCharacter->ServerPlaySpecialMoveMontage(ClimbMontage, true);
	}
}

void UTPP_SPM_LedgeClimb::EndSpecialMove_Implementation()
{
	OwningCharacter->ServerSetAnimRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_LedgeClimb::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	OwningCharacter->SetWallMovementState(EWallMovementState::None);
}
