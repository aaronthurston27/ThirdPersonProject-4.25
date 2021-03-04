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

void UTPP_SPM_LedgeHang::SetLedgeHangProperties(const FHitResult& WallTraceHitResult, const FVector& AttachPoint)
{
	ImpactResult = WallTraceHitResult;
	TargetAttachPoint = AttachPoint;
}

void UTPP_SPM_LedgeHang::BeginSpecialMove_Implementation()
{
	Super::BeginSpecialMove_Implementation();

	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->SetMovementMode(EMovementMode::MOVE_None);

	OwningCharacter->SetWallMovementState(EWallMovementState::WallLedgeHang);
	OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::FullBody);
	
	FRotator Rotation = (-1.0f * ImpactResult.ImpactNormal).Rotation();
	OwningCharacter->SetActorRotation(Rotation);
	OwningCharacter->SetActorLocation(TargetAttachPoint + OwningCharacter->WallLedgeGrabOffset);

	const float LocationDiff = (CachedLedgeDelta).Size();

	bIsAligningToWall = LocationDiff > 1.0f;
}

void UTPP_SPM_LedgeHang::Tick(float DeltaTime)
{
	ATPPPlayerController* PC = OwningCharacter ? OwningCharacter->GetTPPPlayerController() : nullptr;
	const FVector DesiredMovementDirection = PC ? PC->GetDesiredMovementDirection() : FVector::ZeroVector;
	if (PC && !DesiredMovementDirection.IsNearlyZero() && ElapsedTime >= LedgeHangActionDelay)
	{
		const FVector ControllerRelativeMovementDirection = PC->GetControllerRelativeMovementRotation().Vector();
		const float DesiredDirectionWallDot = FVector::DotProduct(ControllerRelativeMovementDirection, ImpactResult.ImpactNormal);
		if (-DesiredDirectionWallDot >= HangToClimbInputDot && LedgeClimbClass)
		{
			FVector ClimbExitPoint;
			const bool bCanClimbCurrentLedge = OwningCharacter->CanClimbUpLedge(ImpactResult, TargetAttachPoint, ClimbExitPoint);
			if (bCanClimbCurrentLedge)
			{
				UTPP_SPM_LedgeClimb* LedgeClimbSPM = NewObject<UTPP_SPM_LedgeClimb>(OwningCharacter, LedgeClimbClass);
				if (LedgeClimbSPM)
				{
					LedgeClimbSPM->SetClimbProperties(ImpactResult, TargetAttachPoint, ClimbExitPoint);
					OwningCharacter->ExecuteSpecialMove(LedgeClimbSPM, true);
				}
			}
		}
		else if (-DesiredDirectionWallDot <= -EndHangInputDot)
		{
			EndSpecialMove();
		}
	}

	ElapsedTime += DeltaTime;
}

void UTPP_SPM_LedgeHang::EndSpecialMove_Implementation()
{
	UTPPMovementComponent* MovementComp = OwningCharacter->GetTPPMovementComponent();
	MovementComp->SetMovementMode(EMovementMode::MOVE_Falling);
	
	if (!bWasInterrupted)
	{
		OwningCharacter->SetWallMovementState(EWallMovementState::None);
		OwningCharacter->SetAnimationBlendSlot(EAnimationBlendSlot::None);
	}
	Super::EndSpecialMove_Implementation();
}
