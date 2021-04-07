// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_LedgeHang.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "SpecialMove/TPP_SPM_LedgeClimb.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_LedgeHang::UTPP_SPM_LedgeHang(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDisablesMovementInput = true;
	bDisablesAiming = true;
	bDisablesSprint = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_LedgeHang::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
	DelayTimer = LedgeHangActionDelay;
}

void UTPP_SPM_LedgeHang::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (DelayTimer > 0.0f)
	{
		DelayTimer -= DeltaTime;
	}

	if (DelayTimer <= 0.0f)
	{
		OwningCharacter->DoLedgeHang();
	}
}

void UTPP_SPM_LedgeHang::EndSpecialMove_Implementation()
{
	if (!bWasInterrupted)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
	}
	Super::EndSpecialMove_Implementation();
}
