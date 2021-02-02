// Fill out your copyright notice in the Description page of Project Settings.


#include "TPP_SPM_Defeated.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_Defeated::UTPP_SPM_Defeated(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDurationBased = false;
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	bDisablesSprint = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_Defeated::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	if (DeathAnim)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(DeathAnim, true);
	}
}

void UTPP_SPM_Defeated::EndSpecialMove_Implementation()
{
	OwningCharacter->OnDeath();
}

void UTPP_SPM_Defeated::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted = false)
{
	if (Montage == DeathAnim)
	{
		EndSpecialMove();
	}
}

void UTPP_SPM_Defeated::OnMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	if (Montage == DeathAnim && !bInterrupted)
	{
		OwningCharacter->OnDeath();
	}
}
