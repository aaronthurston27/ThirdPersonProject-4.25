// Fill out your copyright notice in the Description page of Project Settings.


#include "SpecialMove/TPP_SPM_WallRun.h"
#include "TPPPlayerController.h"
#include "TPPMovementComponent.h"
#include "TPPPlayerController.h"
#include "ThirdPersonProject/TPPPlayerCharacter.h"

UTPP_SPM_WallRun::UTPP_SPM_WallRun(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	bDurationBased = true;
	Duration = 1.5f;
	bDisablesMovementInput = true;
	bDisablesSprint = true;
	bDisablesJump = true;
	bDisablesCrouch = true;
	bDisablesWeaponUseOnStart = true;
	bInterruptsReload = true;
	bDisablesCharacterRotation = true;
}

void UTPP_SPM_WallRun::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	ATPPPlayerController* PC = OwningCharacter->GetTPPPlayerController();
	if (PC)
	{
		CachedDirectionInput = PC->GetDesiredMovementDirection();
	}
}

void UTPP_SPM_WallRun::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (OwningCharacter)
	{
		ATPPPlayerController* PC = OwningCharacter ? OwningCharacter->GetTPPPlayerController() : nullptr;
		const FVector CurrentDesiredMovementDirection = PC ? PC->GetDesiredMovementDirection() : FVector::ZeroVector;
		const bool bChangedXInput = CachedDirectionInput.X != 0.0f && CachedDirectionInput.X != CurrentDesiredMovementDirection.X;
		const bool bChangedYInput = CachedDirectionInput.Y != 0.0f && CachedDirectionInput.Y != CurrentDesiredMovementDirection.Y;
		if (bChangedXInput || (bChangedYInput && CurrentDesiredMovementDirection.X <= 0.0f))
		{
			TimeSinceInputChanged += DeltaTime;
			if (TimeSinceInputChanged >= InputDelay)
			{
				OwningCharacter->SetWallMovementState(EWallMovementState::None);
				return;
			}
		}
		else
		{
			TimeSinceInputChanged = 0.0f;
		}

		OwningCharacter->DoWallRun();
	}
}

void UTPP_SPM_WallRun::OnDurationExceeded_Implementation()
{
	OwningCharacter->SetWallMovementState(EWallMovementState::None);
}

void UTPP_SPM_WallRun::EndSpecialMove_Implementation()
{	
	if (!bWasInterrupted && OwningCharacter)
	{
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
	}

	Super::EndSpecialMove_Implementation();
}
