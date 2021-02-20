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

void UTPP_SPM_LedgeClimb::SetClimbExitPoint(const FVector& ExitPoint)
{
	ClimbExitPoint = ExitPoint;
}

void UTPP_SPM_LedgeClimb::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();
	OwningCharacter->GetCurrentWallClimbProperties(TargetWallImpactResult, TargetAttachPoint);

	if (!ClimbMontage || TargetAttachPoint.IsNearlyZero())
	{
		EndSpecialMove();
		return;
	}

	const FRotator ToWallRotation = (-1.0f * TargetWallImpactResult.ImpactNormal).Rotation();
	OwningCharacter->SetActorRotation(ToWallRotation);
	OwningCharacter->SetActorLocation(TargetAttachPoint - OwningCharacter->WallLedgeGrabOffset);

	if (ClimbMontage)
	{
		UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
		MovementComp->SetMovementMode(EMovementMode::MOVE_None);

		SetAnimRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(ClimbMontage);

		AnimLength = ClimbMontage->GetPlayLength() / ClimbMontage->RateScale;
	}
}

void UTPP_SPM_LedgeClimb::Tick(float DeltaTime)
{
	if (AnimLength > 0.0f)
	{
		const FVector Hmm = FMath::Lerp(TargetAttachPoint, ClimbExitPoint, ElapsedTime / AnimLength);
		OwningCharacter->SetActorLocation(Hmm);
	}

	ElapsedTime += DeltaTime;
}

void UTPP_SPM_LedgeClimb::EndSpecialMove_Implementation()
{
	SetAnimRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_LedgeClimb::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndSpecialMove();
}
