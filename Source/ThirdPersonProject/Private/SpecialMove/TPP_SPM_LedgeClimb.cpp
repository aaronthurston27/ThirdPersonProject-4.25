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

	const EWallMovementState CurrentWallMovementState = OwningCharacter->GetWallMovementState();
	StartingClimbPosition = TargetAttachPoint + (CurrentWallMovementState == EWallMovementState::WallLedgeHang ? OwningCharacter->WallLedgeGrabOffset : FVector::ZeroVector);
	OwningCharacter->SetActorLocation(StartingClimbPosition);
	OwningCharacter->SetWallMovementState(EWallMovementState::None);

	if (ClimbMontage)
	{
		UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
		MovementComp->SetMovementMode(EMovementMode::MOVE_None);

		SetAnimRootMotionMode(ERootMotionMode::IgnoreRootMotion);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
		PlayAnimMontage(ClimbMontage, true);

		AnimLength = ClimbMontage->GetPlayLength() / ClimbMontage->RateScale;
		LateralAnimLength = 0.0f;
	}
}

void UTPP_SPM_LedgeClimb::Tick(float DeltaTime)
{
	if (AnimLength > 0.0f)
	{
		const float AnimTimeRatio = ElapsedTime / AnimLength;
		const float NewZ = FMath::Lerp(StartingClimbPosition.Z, ClimbExitPoint.Z, AnimTimeRatio);
		float NewX = StartingClimbPosition.X;
		float NewY = StartingClimbPosition.Y;
		if (NewZ >= TargetAttachPoint.Z)
		{
			if (LateralAnimLength == 0.0f)
			{
				LateralAnimLength = AnimLength - ElapsedTime;
			}

			const float LateralElapsedTimeRatio = LateralLerpElapsedTime / LateralAnimLength;
			NewX = FMath::Lerp(StartingClimbPosition.X, ClimbExitPoint.X, LateralElapsedTimeRatio);
			NewY = FMath::Lerp(StartingClimbPosition.Y, ClimbExitPoint.Y, LateralElapsedTimeRatio);

			LateralLerpElapsedTime += DeltaTime;
		}

		const FVector FinalPosition = FVector(NewX, NewY, NewZ);
		OwningCharacter->SetActorLocation(FinalPosition);
	}

	ElapsedTime += DeltaTime;
}

void UTPP_SPM_LedgeClimb::EndSpecialMove_Implementation()
{
	SetAnimRootMotionMode(ERootMotionMode::RootMotionFromMontagesOnly);
	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->SetMovementMode(EMovementMode::MOVE_Walking);

	OwningCharacter->SetWallMovementState(EWallMovementState::None);

	Super::EndSpecialMove_Implementation();
}

void UTPP_SPM_LedgeClimb::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	EndSpecialMove();
}
